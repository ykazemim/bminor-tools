#pragma once

#include "source.hpp"

#include <string>
#include <string_view>

namespace bminor {

enum class TokenKind {
    Identifier,
    IntegerLiteral,
    CharLiteral,
    StringLiteral,

    KwArray,
    KwAuto,
    KwBoolean,
    KwChar,
    KwElse,
    KwFalse,
    KwFor,
    KwFunction,
    KwIf,
    KwInteger,
    KwPrint,
    KwReturn,
    KwString,
    KwTrue,
    KwVoid,
    KwWhile,

    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,
    PlusPlus,
    MinusMinus,
    Bang,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    AndAnd,
    OrOr,
    Assign,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Comma,
    Semicolon,
    Colon,

    Comment,
    Invalid,
    EndOfFile,
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    SourceLocation location;
};

bool isKeyword(TokenKind kind);
bool isTypeName(TokenKind kind);
bool isOperator(TokenKind kind);
bool isDelimiter(TokenKind kind);

std::string_view spelling(TokenKind kind);
std::string toString(const Token& token);

} // namespace bminor
