#include "lexer.hpp"

#include <unordered_map>

namespace bminor {

namespace {

const std::unordered_map<std::string_view, TokenKind> keywords = {
    {"array", TokenKind::KwArray},     {"auto", TokenKind::KwAuto},
    {"boolean", TokenKind::KwBoolean}, {"char", TokenKind::KwChar},
    {"else", TokenKind::KwElse},       {"false", TokenKind::KwFalse},
    {"for", TokenKind::KwFor},         {"function", TokenKind::KwFunction},
    {"if", TokenKind::KwIf},           {"integer", TokenKind::KwInteger},
    {"print", TokenKind::KwPrint},     {"return", TokenKind::KwReturn},
    {"string", TokenKind::KwString},   {"true", TokenKind::KwTrue},
    {"void", TokenKind::KwVoid},       {"while", TokenKind::KwWhile},
};

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isIdentifierPart(char c) { return isIdentifierStart(c) || isDigit(c); }

bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

bool isUtf8Continuation(char c) { return (static_cast<unsigned char>(c) & 0xC0) == 0x80; }

class Lexer {
public:
    Lexer(const SourceFile& source, std::vector<Diagnostic>& diagnostics)
        : text_(source.text), diagnostics_(diagnostics) {
        location_.file = source.name;
    }

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        do {
            skipWhitespace();
            tokens.push_back(scanToken());
        } while (tokens.back().kind != TokenKind::EndOfFile);
        return tokens;
    }

private:
    Token scanToken() {
        start_ = location_;
        if (atEnd()) {
            return makeToken(TokenKind::EndOfFile);
        }

        const char c = advance();
        if (isIdentifierStart(c)) {
            return identifierOrKeyword();
        }
        if (isDigit(c)) {
            return integer();
        }

        switch (c) {
        case '"':
            return string();
        case '\'':
            return character();
        case '/':
            if (match('/')) {
                return lineComment();
            }
            if (match('*')) {
                return blockComment();
            }
            return makeToken(TokenKind::Slash);
        case '+':
            return makeToken(match('+') ? TokenKind::PlusPlus : TokenKind::Plus);
        case '-':
            return makeToken(match('-') ? TokenKind::MinusMinus : TokenKind::Minus);
        case '*':
            return makeToken(TokenKind::Star);
        case '%':
            return makeToken(TokenKind::Percent);
        case '^':
            return makeToken(TokenKind::Caret);
        case '!':
            return makeToken(match('=') ? TokenKind::NotEqual : TokenKind::Bang);
        case '<':
            return makeToken(match('=') ? TokenKind::LessEqual : TokenKind::Less);
        case '>':
            return makeToken(match('=') ? TokenKind::GreaterEqual : TokenKind::Greater);
        case '=':
            return makeToken(match('=') ? TokenKind::Equal : TokenKind::Assign);
        case '&':
            if (match('&')) {
                return makeToken(TokenKind::AndAnd);
            }
            break;
        case '|':
            if (match('|')) {
                return makeToken(TokenKind::OrOr);
            }
            break;
        case '(':
            return makeToken(TokenKind::LeftParen);
        case ')':
            return makeToken(TokenKind::RightParen);
        case '[':
            return makeToken(TokenKind::LeftBracket);
        case ']':
            return makeToken(TokenKind::RightBracket);
        case '{':
            return makeToken(TokenKind::LeftBrace);
        case '}':
            return makeToken(TokenKind::RightBrace);
        case ',':
            return makeToken(TokenKind::Comma);
        case ';':
            return makeToken(TokenKind::Semicolon);
        case ':':
            return makeToken(TokenKind::Colon);
        default:
            break;
        }

        while (!atEnd() && isUtf8Continuation(peek())) {
            advance();
        }
        return invalid("unexpected character '" + std::string(lexeme()) + "'");
    }

    Token identifierOrKeyword() {
        while (isIdentifierPart(peek())) {
            advance();
        }
        const auto keyword = keywords.find(lexeme());
        return makeToken(keyword != keywords.end() ? keyword->second : TokenKind::Identifier);
    }

    Token integer() {
        while (isDigit(peek())) {
            advance();
        }
        return makeToken(TokenKind::IntegerLiteral);
    }

    Token string() {
        skipQuotedBody('"');
        if (!match('"')) {
            return invalid("unterminated string literal");
        }
        return makeToken(TokenKind::StringLiteral);
    }

    Token character() {
        const std::size_t length = skipQuotedBody('\'');
        if (!match('\'')) {
            return invalid("unterminated character literal");
        }
        if (length != 1) {
            return invalid("character literal must contain exactly one character");
        }
        return makeToken(TokenKind::CharLiteral);
    }

    std::size_t skipQuotedBody(char quote) {
        std::size_t length = 0;
        while (!atEnd() && peek() != quote && peek() != '\n') {
            if (advance() == '\\' && !atEnd() && peek() != '\n') {
                advance();
            }
            ++length;
        }
        return length;
    }

    Token lineComment() {
        while (!atEnd() && peek() != '\n') {
            advance();
        }
        return makeToken(TokenKind::Comment);
    }

    Token blockComment() {
        while (!atEnd()) {
            if (peek() == '*' && peek(1) == '/') {
                advance();
                advance();
                return makeToken(TokenKind::Comment);
            }
            advance();
        }
        return invalid("unterminated block comment");
    }

    void skipWhitespace() {
        while (!atEnd() && isWhitespace(peek())) {
            advance();
        }
    }

    bool atEnd() const { return location_.offset >= text_.size(); }

    char peek(std::size_t ahead = 0) const {
        const std::size_t index = location_.offset + ahead;
        return index < text_.size() ? text_[index] : '\0';
    }

    char advance() {
        const char c = text_[location_.offset++];
        if (c == '\n') {
            ++location_.line;
            location_.column = 1;
        } else if (!isUtf8Continuation(c)) {
            ++location_.column;
        }
        return c;
    }

    bool match(char expected) {
        if (atEnd() || peek() != expected) {
            return false;
        }
        advance();
        return true;
    }

    std::string_view lexeme() const {
        return std::string_view(text_).substr(start_.offset, location_.offset - start_.offset);
    }

    Token makeToken(TokenKind kind) const { return Token{kind, std::string(lexeme()), start_}; }

    Token invalid(std::string message) {
        diagnostics_.push_back({start_, std::move(message)});
        return makeToken(TokenKind::Invalid);
    }

    const std::string& text_;
    std::vector<Diagnostic>& diagnostics_;
    SourceLocation location_;
    SourceLocation start_;
};

} // namespace

std::vector<Token> tokenize(const SourceFile& source, std::vector<Diagnostic>& diagnostics) {
    return Lexer(source, diagnostics).tokenize();
}

std::string unescape(std::string_view text) {
    std::string result;
    for (std::size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\\' && i + 1 < text.size()) {
            c = text[++i];
            if (c == 'n') {
                c = '\n';
            } else if (c == '0') {
                c = '\0';
            }
        }
        result += c;
    }
    return result;
}

} // namespace bminor
