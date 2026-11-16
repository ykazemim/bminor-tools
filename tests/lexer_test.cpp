#include "lexer.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace bminor {
namespace {

class LexerTest : public ::testing::Test {
protected:
    std::vector<Token> lex(std::string text) {
        source_.text = std::move(text);
        diagnostics_.clear();
        return tokenize(source_, diagnostics_);
    }

    std::string stream(std::string text) {
        std::string result;
        for (const Token& token : lex(std::move(text))) {
            if (!result.empty()) {
                result += ' ';
            }
            result += toString(token);
        }
        return result;
    }

    SourceFile source_{"test.bminor", ""};
    std::vector<Diagnostic> diagnostics_;
};

TEST_F(LexerTest, EmptyInputYieldsEndOfFile) {
    EXPECT_EQ(stream(""), "EOF");
    EXPECT_EQ(stream("  \t\r\n  "), "EOF");
}

TEST_F(LexerTest, ProducesSpecExampleStream) {
    const std::string source = "factorial: function integer ( n: integer ) = {\n"
                               "    if (n <= 1) return 1;\n"
                               "    return n * factorial(n - 1);\n"
                               "}\n";
    EXPECT_EQ(stream(source),
              "IDENT(factorial) COLON KW(function) KW(integer) LPAREN IDENT(n) COLON "
              "KW(integer) RPAREN ASSIGN LBRACE "
              "KW(if) LPAREN IDENT(n) OP(<=) INT(1) RPAREN KW(return) INT(1) SEMI "
              "KW(return) IDENT(n) OP(*) IDENT(factorial) LPAREN IDENT(n) OP(-) INT(1) "
              "RPAREN SEMI "
              "RBRACE EOF");
    EXPECT_TRUE(diagnostics_.empty());
}

TEST_F(LexerTest, KeywordsTakePriorityOverIdentifiers) {
    EXPECT_EQ(stream("return returned _return x1 _"),
              "KW(return) IDENT(returned) IDENT(_return) IDENT(x1) IDENT(_) EOF");
}

TEST_F(LexerTest, RecognizesEveryKeyword) {
    const std::vector<Token> tokens = lex("array auto boolean char else false for function if "
                                          "integer print return string true void while");
    ASSERT_EQ(tokens.size(), 17u);
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        EXPECT_TRUE(isKeyword(tokens[i].kind)) << tokens[i].lexeme;
    }
}

TEST_F(LexerTest, AppliesLongestMatch) {
    EXPECT_EQ(stream("<= < = == ++ + -- - >= > != ! && ||"),
              "OP(<=) OP(<) ASSIGN OP(==) OP(++) OP(+) OP(--) OP(-) OP(>=) OP(>) OP(!=) OP(!) "
              "OP(&&) OP(||) EOF");
    EXPECT_EQ(stream("a<=b"), "IDENT(a) OP(<=) IDENT(b) EOF");
    EXPECT_EQ(stream("x+++y"), "IDENT(x) OP(++) OP(+) IDENT(y) EOF");
}

TEST_F(LexerTest, MinusIsNeverPartOfAnInteger) {
    EXPECT_EQ(stream("n-1 -5"), "IDENT(n) OP(-) INT(1) OP(-) INT(5) EOF");
}

TEST_F(LexerTest, ScansDelimitersAndOperators) {
    EXPECT_EQ(stream("( ) [ ] { } , ; : * / % ^"),
              "LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE COMMA SEMI COLON OP(*) OP(/) "
              "OP(%) OP(^) EOF");
}

TEST_F(LexerTest, ScansLiterals) {
    EXPECT_EQ(stream(R"(42 007 'a' '\n' '\'' "hi\n" "say \"hi\"" true false)"),
              R"(INT(42) INT(007) CHAR('a') CHAR('\n') CHAR('\'') STRING("hi\n") )"
              R"(STRING("say \"hi\"") BOOL(true) BOOL(false) EOF)");
    EXPECT_TRUE(diagnostics_.empty());
}

TEST_F(LexerTest, KeepsCommentsAsTokens) {
    EXPECT_EQ(stream("x // trailing\n/* block\n comment */ y"),
              "IDENT(x) COMMENT(// trailing) COMMENT(/* block\\n comment */) IDENT(y) EOF");
}

TEST_F(LexerTest, TracksLinesColumnsAndOffsets) {
    const std::vector<Token> tokens = lex("x\n  yy  z\n\n/* a\nb */ w");
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[1].location.line, 2);
    EXPECT_EQ(tokens[1].location.column, 3);
    EXPECT_EQ(tokens[1].location.offset, 4u);
    EXPECT_EQ(tokens[2].location.column, 7);
    EXPECT_EQ(tokens[3].location.line, 4);
    EXPECT_EQ(tokens[4].location.line, 5);
    EXPECT_EQ(tokens[4].location.column, 6);
    EXPECT_EQ(tokens[4].location.file, "test.bminor");
}

TEST_F(LexerTest, ReportsInvalidCharacterAndContinues) {
    EXPECT_EQ(stream("x: integer = 5@;\ny: integer = 10;"),
              "IDENT(x) COLON KW(integer) ASSIGN INT(5) INVALID(@) SEMI "
              "IDENT(y) COLON KW(integer) ASSIGN INT(10) SEMI EOF");
    ASSERT_EQ(diagnostics_.size(), 1u);
    EXPECT_EQ(diagnostics_[0].message, "unexpected character '@'");
    EXPECT_EQ(diagnostics_[0].location.line, 1);
    EXPECT_EQ(diagnostics_[0].location.column, 15);
}

TEST_F(LexerTest, ReportsSingleAmpersandAndPipe) {
    EXPECT_EQ(stream("a & b | c"), "IDENT(a) INVALID(&) IDENT(b) INVALID(|) IDENT(c) EOF");
    EXPECT_EQ(diagnostics_.size(), 2u);
}

TEST_F(LexerTest, ReportsUnterminatedStringAtEndOfLine) {
    EXPECT_EQ(stream("\"abc\nx"), "INVALID(\"abc) IDENT(x) EOF");
    ASSERT_EQ(diagnostics_.size(), 1u);
    EXPECT_EQ(diagnostics_[0].message, "unterminated string literal");
}

TEST_F(LexerTest, ReportsUnterminatedStringAtEndOfFile) {
    EXPECT_EQ(stream("\"abc\\"), "INVALID(\"abc\\) EOF");
    EXPECT_EQ(diagnostics_.size(), 1u);
}

TEST_F(LexerTest, ReportsUnterminatedBlockComment) {
    EXPECT_EQ(stream("x /* never\nends"), "IDENT(x) INVALID(/* never\\nends) EOF");
    ASSERT_EQ(diagnostics_.size(), 1u);
    EXPECT_EQ(diagnostics_[0].message, "unterminated block comment");
    EXPECT_EQ(diagnostics_[0].location.column, 3);
}

TEST_F(LexerTest, ReportsMalformedCharacterLiterals) {
    EXPECT_EQ(stream("'ab' '' 'x"), "INVALID('ab') INVALID('') INVALID('x) EOF");
    ASSERT_EQ(diagnostics_.size(), 3u);
    EXPECT_EQ(diagnostics_[0].message, "character literal must contain exactly one character");
    EXPECT_EQ(diagnostics_[2].message, "unterminated character literal");
}

TEST(UnescapeTest, DecodesEscapeSequences) {
    EXPECT_EQ(unescape(R"(a\nb)"), "a\nb");
    EXPECT_EQ(unescape(R"(\0)"), std::string(1, '\0'));
    EXPECT_EQ(unescape(R"(\"\'\\\q)"), "\"'\\q");
    EXPECT_EQ(unescape("plain"), "plain");
}

} // namespace
} // namespace bminor
