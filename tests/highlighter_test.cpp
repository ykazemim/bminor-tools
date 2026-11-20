#include "highlighter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <gtest/gtest.h>

#include <regex>
#include <string>
#include <utility>
#include <vector>

namespace bminor {
namespace {

class HighlighterTest : public ::testing::Test {
protected:
    std::vector<HighlightSpan> spans(std::string text) {
        source_.text = std::move(text);
        diagnostics_.clear();
        const std::vector<Token> tokens = tokenize(source_, diagnostics_);
        Program program = parse(tokens, diagnostics_);
        return highlight(tokens, program, diagnostics_);
    }

    std::vector<std::pair<std::string, Highlight>> labeled(std::string text) {
        std::vector<std::pair<std::string, Highlight>> result;
        for (const HighlightSpan& span : spans(std::move(text))) {
            result.emplace_back(source_.text.substr(span.offset, span.length), span.highlight);
        }
        return result;
    }

    SourceFile source_{"test.bminor", ""};
    std::vector<Diagnostic> diagnostics_;
};

using Labels = std::vector<std::pair<std::string, Highlight>>;

std::string stripAnsi(const std::string& text) {
    return std::regex_replace(text, std::regex("\x1b\\[[0-9;]*m"), "");
}

TEST_F(HighlighterTest, DistinguishesFunctionsFromVariablesUsingTheAst) {
    const Labels labels = labeled("f: function integer (n: integer) = { return f(n) + n; }");
    EXPECT_EQ(labels, (Labels{
                          {"f", Highlight::Function},
                          {"function", Highlight::Keyword},
                          {"integer", Highlight::TypeName},
                          {"n", Highlight::Variable},
                          {"integer", Highlight::TypeName},
                          {"=", Highlight::Operator},
                          {"return", Highlight::Keyword},
                          {"f", Highlight::Function},
                          {"n", Highlight::Variable},
                          {"+", Highlight::Operator},
                          {"n", Highlight::Variable},
                      }));
}

TEST_F(HighlighterTest, ClassifiesLiteralsAndComments) {
    const Labels labels = labeled("// note\nb: boolean = !false; s: string = \"x\"; c: char = 'y';"
                                  " a: array [3] auto;");
    EXPECT_EQ(labels, (Labels{
                          {"// note", Highlight::Comment},
                          {"b", Highlight::Variable},
                          {"boolean", Highlight::TypeName},
                          {"=", Highlight::Operator},
                          {"!", Highlight::Operator},
                          {"false", Highlight::Boolean},
                          {"s", Highlight::Variable},
                          {"string", Highlight::TypeName},
                          {"=", Highlight::Operator},
                          {"\"x\"", Highlight::String},
                          {"c", Highlight::Variable},
                          {"char", Highlight::TypeName},
                          {"=", Highlight::Operator},
                          {"'y'", Highlight::String},
                          {"a", Highlight::Variable},
                          {"array", Highlight::Keyword},
                          {"3", Highlight::Number},
                          {"auto", Highlight::TypeName},
                      }));
}

TEST_F(HighlighterTest, MarksLexicalAndSyntaxErrors) {
    const Labels labels = labeled("x: integer = 5@; y: integer = ;");
    EXPECT_EQ(labels[4], (std::pair<std::string, Highlight>{"@", Highlight::Error}));
    EXPECT_EQ(labels.back(), (std::pair<std::string, Highlight>{";", Highlight::Error}));
}

TEST_F(HighlighterTest, AnsiOutputReproducesSourceExactly) {
    const std::string source = "/* multi\n   line */\nmain: function void () = {\n"
                               "\tprint \"tab\\t\", 'x', 1 <= 2;   \n}\n@ \"open\n";
    const std::string ansi = renderAnsi(source, spans(source));
    EXPECT_NE(ansi, source);
    EXPECT_EQ(stripAnsi(ansi), source);
}

TEST_F(HighlighterTest, AnsiColorsAreResetBeforeEveryNewline) {
    const std::string source = "/* a\nb */";
    const std::string ansi = renderAnsi(source, spans(source));
    EXPECT_EQ(ansi, "\x1b[3;90m/* a\x1b[0m\n\x1b[3;90mb */\x1b[0m");
}

TEST_F(HighlighterTest, AnsiUsesBoldBlueForKeywords) {
    const std::string source = "f: function void ();";
    EXPECT_NE(renderAnsi(source, spans(source)).find("\x1b[1;34mfunction\x1b[0m"),
              std::string::npos);
}

TEST_F(HighlighterTest, HtmlIsStandaloneAndEscaped) {
    const std::string source = "f: function boolean () = { return 1 < 2 && f(); }";
    const std::string html = renderHtml(source, spans(source), "a<b>.bminor");

    EXPECT_EQ(html.rfind("<!DOCTYPE html>", 0), 0u);
    EXPECT_NE(html.find("<title>a&lt;b&gt;.bminor</title>"), std::string::npos);
    EXPECT_NE(html.find("<style>"), std::string::npos);
    EXPECT_EQ(html.find("<script"), std::string::npos);
    EXPECT_NE(html.find("<pre><span class=\"fn\">f</span>: <span class=\"kw\">function</span>"),
              std::string::npos);
    EXPECT_NE(html.find("<span class=\"op\">&lt;</span>"), std::string::npos);
    EXPECT_NE(html.find("<span class=\"op\">&amp;&amp;</span> <span class=\"fn\">f</span>()"),
              std::string::npos);
}

} // namespace
} // namespace bminor
