#include "parser.hpp"

#include "lexer.hpp"

#include <algorithm>
#include <charconv>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace bminor {

namespace {

struct ParseError {};

std::string describe(const Token& token) {
    return token.kind == TokenKind::EndOfFile ? "end of file" : "'" + token.lexeme + "'";
}

std::optional<Type::Kind> primitiveType(TokenKind kind) {
    switch (kind) {
    case TokenKind::KwInteger:
        return Type::Kind::Integer;
    case TokenKind::KwBoolean:
        return Type::Kind::Boolean;
    case TokenKind::KwChar:
        return Type::Kind::Char;
    case TokenKind::KwString:
        return Type::Kind::String;
    case TokenKind::KwVoid:
        return Type::Kind::Void;
    case TokenKind::KwAuto:
        return Type::Kind::Auto;
    default:
        return std::nullopt;
    }
}

bool isAssignable(const Expr& expr) {
    return dynamic_cast<const Identifier*>(&expr) || dynamic_cast<const IndexExpr*>(&expr);
}

class Parser {
public:
    Parser(const std::vector<Token>& tokens, std::vector<Diagnostic>& diagnostics)
        : diagnostics_(diagnostics) {
        std::copy_if(
            tokens.begin(), tokens.end(), std::back_inserter(tokens_), [](const Token& token) {
                return token.kind != TokenKind::Comment && token.kind != TokenKind::Invalid;
            });
        if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
            tokens_.push_back({TokenKind::EndOfFile, "", {}});
        }
    }

    Program parseProgram() {
        Program program;
        while (!atEnd()) {
            try {
                program.declarations.push_back(declaration());
            } catch (const ParseError&) {
                synchronizeDeclaration();
            }
        }
        return program;
    }

private:
    using Operand = ExprPtr (Parser::*)();

    DeclPtr declaration() {
        const Token& name = expect(TokenKind::Identifier, "declaration");
        expect(TokenKind::Colon, "':' after name");
        if (match(TokenKind::KwFunction)) {
            return functionDeclaration(name);
        }
        return variableDeclaration(name);
    }

    DeclPtr functionDeclaration(const Token& name) {
        TypePtr returnType = type();
        expect(TokenKind::LeftParen, "'(' after return type");
        std::vector<Param> params;
        if (!check(TokenKind::RightParen)) {
            do {
                params.push_back(parameter());
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RightParen, "')' after parameters");

        std::unique_ptr<BlockStmt> body;
        if (match(TokenKind::Assign)) {
            body = block();
        } else {
            expect(TokenKind::Semicolon, "'=' or ';' after function signature");
        }
        return std::make_unique<FuncDecl>(name.location, name.lexeme, std::move(returnType),
                                          std::move(params), std::move(body));
    }

    DeclPtr variableDeclaration(const Token& name) {
        TypePtr declaredType = type();
        ExprPtr init;
        if (match(TokenKind::Assign)) {
            init = initializer();
        }
        expect(TokenKind::Semicolon, "';' after declaration");
        return std::make_unique<VarDecl>(name.location, name.lexeme, std::move(declaredType),
                                         std::move(init));
    }

    Param parameter() {
        const Token& name = expect(TokenKind::Identifier, "parameter name");
        expect(TokenKind::Colon, "':' after parameter name");
        TypePtr declaredType = type();
        return Param{name.lexeme, std::move(declaredType), name.location};
    }

    TypePtr type() {
        if (match(TokenKind::KwArray)) {
            expect(TokenKind::LeftBracket, "'[' after 'array'");
            std::optional<std::int64_t> size;
            if (check(TokenKind::IntegerLiteral)) {
                size = integerValue(advance());
            }
            expect(TokenKind::RightBracket, "']' after array size");
            TypePtr element = type();
            return std::make_shared<const Type>(Type{Type::Kind::Array, std::move(element), size});
        }

        const std::optional<Type::Kind> kind = primitiveType(peek().kind);
        if (!kind) {
            unexpected("type");
        }
        advance();
        return std::make_shared<const Type>(Type{*kind, nullptr, std::nullopt});
    }

    ExprPtr initializer() {
        if (!match(TokenKind::LeftBrace)) {
            return expression();
        }
        const SourceLocation location = previous().location;
        std::vector<ExprPtr> elements;
        do {
            elements.push_back(initializer());
        } while (match(TokenKind::Comma));
        expect(TokenKind::RightBrace, "'}' after array elements");
        return std::make_unique<ArrayLiteral>(location, std::move(elements));
    }

    StmtPtr statement() {
        switch (peek().kind) {
        case TokenKind::LeftBrace:
            return block();
        case TokenKind::KwIf:
            return ifStatement();
        case TokenKind::KwFor:
            return forStatement();
        case TokenKind::KwWhile:
            return whileStatement();
        case TokenKind::KwReturn:
            return returnStatement();
        case TokenKind::KwPrint:
            return printStatement();
        default:
            break;
        }
        if (atDeclarationStart()) {
            return std::make_unique<DeclStmt>(declaration());
        }
        return expressionStatement();
    }

    std::unique_ptr<BlockStmt> block() {
        const SourceLocation location = expect(TokenKind::LeftBrace, "'{'").location;
        std::vector<StmtPtr> statements;
        while (!check(TokenKind::RightBrace) && !atEnd()) {
            const std::size_t start = current_;
            try {
                statements.push_back(statement());
            } catch (const ParseError&) {
                synchronize();
                if (current_ == start) {
                    advance();
                }
            }
        }
        if (!match(TokenKind::RightBrace)) {
            report(peek(), "expected '}' at end of block, got " + describe(peek()));
        }
        return std::make_unique<BlockStmt>(location, std::move(statements));
    }

    StmtPtr ifStatement() {
        const SourceLocation location = advance().location;
        expect(TokenKind::LeftParen, "'(' after 'if'");
        ExprPtr condition = expression();
        expect(TokenKind::RightParen, "')' after condition");
        StmtPtr thenBranch = statement();
        StmtPtr elseBranch;
        if (match(TokenKind::KwElse)) {
            elseBranch = statement();
        }
        return std::make_unique<IfStmt>(location, std::move(condition), std::move(thenBranch),
                                        std::move(elseBranch));
    }

    StmtPtr forStatement() {
        const SourceLocation location = advance().location;
        expect(TokenKind::LeftParen, "'(' after 'for'");
        ExprPtr init = optionalExpression(TokenKind::Semicolon);
        expect(TokenKind::Semicolon, "';' after loop initializer");
        ExprPtr condition = optionalExpression(TokenKind::Semicolon);
        expect(TokenKind::Semicolon, "';' after loop condition");
        ExprPtr step = optionalExpression(TokenKind::RightParen);
        expect(TokenKind::RightParen, "')' after loop step");
        StmtPtr body = statement();
        return std::make_unique<ForStmt>(location, std::move(init), std::move(condition),
                                         std::move(step), std::move(body));
    }

    StmtPtr whileStatement() {
        const SourceLocation location = advance().location;
        expect(TokenKind::LeftParen, "'(' after 'while'");
        ExprPtr condition = expression();
        expect(TokenKind::RightParen, "')' after condition");
        StmtPtr body = statement();
        return std::make_unique<WhileStmt>(location, std::move(condition), std::move(body));
    }

    StmtPtr returnStatement() {
        const SourceLocation location = advance().location;
        ExprPtr value = optionalExpression(TokenKind::Semicolon);
        expect(TokenKind::Semicolon, "';' after return value");
        return std::make_unique<ReturnStmt>(location, std::move(value));
    }

    StmtPtr printStatement() {
        const SourceLocation location = advance().location;
        std::vector<ExprPtr> arguments;
        if (!check(TokenKind::Semicolon)) {
            arguments = expressionList();
        }
        expect(TokenKind::Semicolon, "';' after print arguments");
        return std::make_unique<PrintStmt>(location, std::move(arguments));
    }

    StmtPtr expressionStatement() {
        ExprPtr expr = expression();
        expect(TokenKind::Semicolon, "';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    ExprPtr optionalExpression(TokenKind terminator) {
        return check(terminator) ? nullptr : expression();
    }

    std::vector<ExprPtr> expressionList() {
        std::vector<ExprPtr> expressions;
        do {
            expressions.push_back(expression());
        } while (match(TokenKind::Comma));
        return expressions;
    }

    ExprPtr expression() { return assignment(); }

    ExprPtr assignment() {
        ExprPtr target = logicalOr();
        if (!match(TokenKind::Assign)) {
            return target;
        }
        const Token& equals = previous();
        ExprPtr value = assignment();
        if (!isAssignable(*target)) {
            report(equals, "invalid assignment target");
        }
        return std::make_unique<AssignExpr>(std::move(target), std::move(value));
    }

    ExprPtr logicalOr() { return leftAssociative(&Parser::logicalAnd, {TokenKind::OrOr}); }

    ExprPtr logicalAnd() { return leftAssociative(&Parser::comparison, {TokenKind::AndAnd}); }

    ExprPtr comparison() {
        return leftAssociative(&Parser::additive,
                               {TokenKind::Less, TokenKind::LessEqual, TokenKind::Greater,
                                TokenKind::GreaterEqual, TokenKind::Equal, TokenKind::NotEqual});
    }

    ExprPtr additive() {
        return leftAssociative(&Parser::multiplicative, {TokenKind::Plus, TokenKind::Minus});
    }

    ExprPtr multiplicative() {
        return leftAssociative(&Parser::power,
                               {TokenKind::Star, TokenKind::Slash, TokenKind::Percent});
    }

    ExprPtr power() {
        ExprPtr base = unary();
        if (!match(TokenKind::Caret)) {
            return base;
        }
        ExprPtr exponent = power();
        return std::make_unique<BinaryExpr>(TokenKind::Caret, std::move(base), std::move(exponent));
    }

    ExprPtr unary() {
        if (!match({TokenKind::Minus, TokenKind::Bang})) {
            return postfix();
        }
        const Token& op = previous();
        ExprPtr operand = unary();
        return std::make_unique<UnaryExpr>(op.location, op.kind, std::move(operand));
    }

    ExprPtr postfix() {
        ExprPtr expr = primary();
        while (true) {
            if (match(TokenKind::LeftBracket)) {
                ExprPtr index = expression();
                expect(TokenKind::RightBracket, "']' after index");
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            } else if (match(TokenKind::LeftParen)) {
                std::vector<ExprPtr> arguments;
                if (!check(TokenKind::RightParen)) {
                    arguments = expressionList();
                }
                expect(TokenKind::RightParen, "')' after arguments");
                expr = std::make_unique<CallExpr>(std::move(expr), std::move(arguments));
            } else if (match({TokenKind::PlusPlus, TokenKind::MinusMinus})) {
                expr = std::make_unique<PostfixExpr>(previous().kind, std::move(expr));
            } else {
                return expr;
            }
        }
    }

    ExprPtr primary() {
        const Token& token = peek();
        switch (token.kind) {
        case TokenKind::IntegerLiteral:
            advance();
            return std::make_unique<IntegerLiteral>(token.location, integerValue(token));
        case TokenKind::CharLiteral:
            advance();
            return std::make_unique<CharLiteral>(token.location, unescape(unquote(token)).front());
        case TokenKind::StringLiteral:
            advance();
            return std::make_unique<StringLiteral>(token.location, unescape(unquote(token)));
        case TokenKind::KwTrue:
        case TokenKind::KwFalse:
            advance();
            return std::make_unique<BooleanLiteral>(token.location,
                                                    token.kind == TokenKind::KwTrue);
        case TokenKind::Identifier:
            advance();
            return std::make_unique<Identifier>(token.location, token.lexeme);
        case TokenKind::LeftParen: {
            advance();
            ExprPtr expr = expression();
            expect(TokenKind::RightParen, "')' after expression");
            return expr;
        }
        default:
            unexpected("expression");
        }
    }

    ExprPtr leftAssociative(Operand operand, std::initializer_list<TokenKind> operators) {
        ExprPtr left = (this->*operand)();
        while (match(operators)) {
            const TokenKind op = previous().kind;
            ExprPtr right = (this->*operand)();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    std::int64_t integerValue(const Token& token) {
        std::int64_t value = 0;
        const char* first = token.lexeme.data();
        const auto result = std::from_chars(first, first + token.lexeme.size(), value);
        if (result.ec != std::errc()) {
            report(token, "integer literal " + token.lexeme + " is out of range");
        }
        return value;
    }

    static std::string_view unquote(const Token& token) {
        return std::string_view(token.lexeme).substr(1, token.lexeme.size() - 2);
    }

    void synchronize() {
        while (!atEnd()) {
            if (match(TokenKind::Semicolon)) {
                return;
            }
            switch (peek().kind) {
            case TokenKind::LeftBrace:
            case TokenKind::RightBrace:
            case TokenKind::KwIf:
            case TokenKind::KwFor:
            case TokenKind::KwWhile:
            case TokenKind::KwReturn:
            case TokenKind::KwPrint:
                return;
            default:
                if (atDeclarationStart()) {
                    return;
                }
                advance();
            }
        }
    }

    void synchronizeDeclaration() {
        while (!atEnd() && !atDeclarationStart()) {
            advance();
        }
    }

    bool atDeclarationStart() const {
        return check(TokenKind::Identifier) && peek(1).kind == TokenKind::Colon;
    }

    const Token& peek(std::size_t ahead = 0) const {
        return tokens_[std::min(current_ + ahead, tokens_.size() - 1)];
    }

    const Token& previous() const { return tokens_[current_ - 1]; }

    bool atEnd() const { return check(TokenKind::EndOfFile); }

    bool check(TokenKind kind) const { return peek().kind == kind; }

    const Token& advance() {
        const Token& token = peek();
        if (!atEnd()) {
            ++current_;
        }
        return token;
    }

    bool match(TokenKind kind) {
        if (!check(kind)) {
            return false;
        }
        advance();
        return true;
    }

    bool match(std::initializer_list<TokenKind> kinds) {
        return std::any_of(kinds.begin(), kinds.end(),
                           [this](TokenKind kind) { return match(kind); });
    }

    const Token& expect(TokenKind kind, std::string_view expected) {
        if (!check(kind)) {
            unexpected(expected);
        }
        return advance();
    }

    [[noreturn]] void unexpected(std::string_view expected) {
        report(peek(), "expected " + std::string(expected) + ", got " + describe(peek()));
        throw ParseError{};
    }

    void report(const Token& token, std::string message) {
        diagnostics_.push_back({token.location, std::move(message)});
    }

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    std::vector<Diagnostic>& diagnostics_;
};

} // namespace

Program parse(const std::vector<Token>& tokens, std::vector<Diagnostic>& diagnostics) {
    return Parser(tokens, diagnostics).parseProgram();
}

} // namespace bminor
