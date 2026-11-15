#include "token.hpp"

namespace bminor {

namespace {

std::string printable(std::string_view text) {
    std::string result;
    for (char c : text) {
        switch (c) {
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
        }
    }
    return result;
}

std::string_view delimiterName(TokenKind kind) {
    switch (kind) {
    case TokenKind::LeftParen:
        return "LPAREN";
    case TokenKind::RightParen:
        return "RPAREN";
    case TokenKind::LeftBracket:
        return "LBRACKET";
    case TokenKind::RightBracket:
        return "RBRACKET";
    case TokenKind::LeftBrace:
        return "LBRACE";
    case TokenKind::RightBrace:
        return "RBRACE";
    case TokenKind::Comma:
        return "COMMA";
    case TokenKind::Semicolon:
        return "SEMI";
    case TokenKind::Colon:
        return "COLON";
    default:
        return "";
    }
}

} // namespace

bool isKeyword(TokenKind kind) { return kind >= TokenKind::KwArray && kind <= TokenKind::KwWhile; }

bool isTypeName(TokenKind kind) {
    switch (kind) {
    case TokenKind::KwAuto:
    case TokenKind::KwBoolean:
    case TokenKind::KwChar:
    case TokenKind::KwInteger:
    case TokenKind::KwString:
    case TokenKind::KwVoid:
        return true;
    default:
        return false;
    }
}

bool isOperator(TokenKind kind) { return kind >= TokenKind::Plus && kind <= TokenKind::Assign; }

bool isDelimiter(TokenKind kind) {
    return kind >= TokenKind::LeftParen && kind <= TokenKind::Colon;
}

std::string_view spelling(TokenKind kind) {
    switch (kind) {
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    case TokenKind::Percent:
        return "%";
    case TokenKind::Caret:
        return "^";
    case TokenKind::PlusPlus:
        return "++";
    case TokenKind::MinusMinus:
        return "--";
    case TokenKind::Bang:
        return "!";
    case TokenKind::Less:
        return "<";
    case TokenKind::LessEqual:
        return "<=";
    case TokenKind::Greater:
        return ">";
    case TokenKind::GreaterEqual:
        return ">=";
    case TokenKind::Equal:
        return "==";
    case TokenKind::NotEqual:
        return "!=";
    case TokenKind::AndAnd:
        return "&&";
    case TokenKind::OrOr:
        return "||";
    case TokenKind::Assign:
        return "=";
    default:
        return "";
    }
}

std::string toString(const Token& token) {
    const std::string lexeme = printable(token.lexeme);
    const TokenKind kind = token.kind;

    if (kind == TokenKind::KwTrue || kind == TokenKind::KwFalse) {
        return "BOOL(" + lexeme + ")";
    }
    if (isKeyword(kind)) {
        return "KW(" + lexeme + ")";
    }
    if (kind == TokenKind::Assign) {
        return "ASSIGN";
    }
    if (isOperator(kind)) {
        return "OP(" + lexeme + ")";
    }
    if (isDelimiter(kind)) {
        return std::string(delimiterName(kind));
    }

    switch (kind) {
    case TokenKind::Identifier:
        return "IDENT(" + lexeme + ")";
    case TokenKind::IntegerLiteral:
        return "INT(" + lexeme + ")";
    case TokenKind::CharLiteral:
        return "CHAR(" + lexeme + ")";
    case TokenKind::StringLiteral:
        return "STRING(" + lexeme + ")";
    case TokenKind::Comment:
        return "COMMENT(" + lexeme + ")";
    case TokenKind::Invalid:
        return "INVALID(" + lexeme + ")";
    case TokenKind::EndOfFile:
        return "EOF";
    default:
        return lexeme;
    }
}

} // namespace bminor
