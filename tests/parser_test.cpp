#include "ast_printer.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

namespace bminor {
namespace {

class ParserTest : public ::testing::Test {
protected:
    Program parseSource(std::string text) {
        source_.text = std::move(text);
        diagnostics_.clear();
        return parse(tokenize(source_, diagnostics_), diagnostics_);
    }

    std::string dump(std::string text) {
        Program program = parseSource(std::move(text));
        std::ostringstream out;
        printAst(out, program);
        return out.str();
    }

    std::vector<std::string> messages() const {
        std::vector<std::string> result;
        for (const Diagnostic& diagnostic : diagnostics_) {
            result.push_back(std::to_string(diagnostic.location.line) + ':' +
                             std::to_string(diagnostic.location.column) + ' ' + diagnostic.message);
        }
        return result;
    }

    SourceFile source_{"test.bminor", ""};
    std::vector<Diagnostic> diagnostics_;
};

using Messages = std::vector<std::string>;

TEST_F(ParserTest, BuildsSpecExampleTree) {
    const std::string tree = dump("factorial: function integer ( n: integer ) = {\n"
                                  "    if (n <= 1) return 1;\n"
                                  "    return n * factorial(n - 1);\n"
                                  "}\n");
    EXPECT_EQ(tree, R"(Program
  FuncDecl(name='factorial', returns='integer', loc=1:1)
    params[0]: Param(name='n', type='integer', loc=1:31)
    body: BlockStmt(loc=1:46)
      statements[0]: IfStmt(loc=2:5)
        condition: BinaryExpr(op='<=', loc=2:9)
          left: Identifier(name='n', loc=2:9)
          right: IntegerLiteral(value=1, loc=2:14)
        then: ReturnStmt(loc=2:17)
          value: IntegerLiteral(value=1, loc=2:24)
      statements[1]: ReturnStmt(loc=3:5)
        value: BinaryExpr(op='*', loc=3:12)
          left: Identifier(name='n', loc=3:12)
          right: CallExpr(loc=3:16)
            callee: Identifier(name='factorial', loc=3:16)
            args[0]: BinaryExpr(op='-', loc=3:26)
              left: Identifier(name='n', loc=3:26)
              right: IntegerLiteral(value=1, loc=3:30)
)");
    EXPECT_TRUE(diagnostics_.empty());
}

TEST_F(ParserTest, ParsesGlobalDeclarations) {
    const std::string tree = dump("x: integer;\n"
                                  "s: string = \"hi\\n\";\n"
                                  "c: char = '\\0';\n"
                                  "a: array [2] array [] boolean = {{true}, {false, true}};\n"
                                  "puts: function void (s: string);\n");
    EXPECT_EQ(tree, R"(Program
  VarDecl(name='x', type='integer', loc=1:1)
  VarDecl(name='s', type='string', loc=2:1)
    init: StringLiteral(value="hi\n", loc=2:13)
  VarDecl(name='c', type='char', loc=3:1)
    init: CharLiteral(value='\0', loc=3:11)
  VarDecl(name='a', type='array [2] array [] boolean', loc=4:1)
    init: ArrayLiteral(loc=4:33)
      elements[0]: ArrayLiteral(loc=4:34)
        elements[0]: BooleanLiteral(value=true, loc=4:35)
      elements[1]: ArrayLiteral(loc=4:42)
        elements[0]: BooleanLiteral(value=false, loc=4:43)
        elements[1]: BooleanLiteral(value=true, loc=4:50)
  FuncDecl(name='puts', returns='void', loc=5:1)
    params[0]: Param(name='s', type='string', loc=5:22)
)");
    EXPECT_TRUE(diagnostics_.empty());
}

TEST_F(ParserTest, ParsesStatements) {
    const std::string tree = dump("f: function void () = {\n"
                                  "  i: integer = 0;\n"
                                  "  for (;;) {}\n"
                                  "  while (i < 3) i++;\n"
                                  "  if (i) print; else return;\n"
                                  "}\n");
    EXPECT_EQ(tree, R"(Program
  FuncDecl(name='f', returns='void', loc=1:1)
    body: BlockStmt(loc=1:23)
      statements[0]: DeclStmt(loc=2:3)
        declaration: VarDecl(name='i', type='integer', loc=2:3)
          init: IntegerLiteral(value=0, loc=2:16)
      statements[1]: ForStmt(loc=3:3)
        body: BlockStmt(loc=3:12)
      statements[2]: WhileStmt(loc=4:3)
        condition: BinaryExpr(op='<', loc=4:10)
          left: Identifier(name='i', loc=4:10)
          right: IntegerLiteral(value=3, loc=4:14)
        body: ExprStmt(loc=4:17)
          expression: PostfixExpr(op='++', loc=4:17)
            operand: Identifier(name='i', loc=4:17)
      statements[3]: IfStmt(loc=5:3)
        condition: Identifier(name='i', loc=5:7)
        then: PrintStmt(loc=5:10)
        else: ReturnStmt(loc=5:22)
)");
    EXPECT_TRUE(diagnostics_.empty());
}

TEST_F(ParserTest, RespectsPrecedenceAndAssociativity) {
    const std::string tree = dump("x: integer = a = b || c && d == e + f * -g ^ h ^ i;");
    EXPECT_EQ(tree, R"(Program
  VarDecl(name='x', type='integer', loc=1:1)
    init: AssignExpr(loc=1:14)
      target: Identifier(name='a', loc=1:14)
      value: BinaryExpr(op='||', loc=1:18)
        left: Identifier(name='b', loc=1:18)
        right: BinaryExpr(op='&&', loc=1:23)
          left: Identifier(name='c', loc=1:23)
          right: BinaryExpr(op='==', loc=1:28)
            left: Identifier(name='d', loc=1:28)
            right: BinaryExpr(op='+', loc=1:33)
              left: Identifier(name='e', loc=1:33)
              right: BinaryExpr(op='*', loc=1:37)
                left: Identifier(name='f', loc=1:37)
                right: BinaryExpr(op='^', loc=1:41)
                  left: UnaryExpr(op='-', loc=1:41)
                    operand: Identifier(name='g', loc=1:42)
                  right: BinaryExpr(op='^', loc=1:46)
                    left: Identifier(name='h', loc=1:46)
                    right: Identifier(name='i', loc=1:50)
)");
}

TEST_F(ParserTest, BinaryOperatorsAreLeftAssociative) {
    const std::string tree = dump("x: integer = a - b - c;");
    EXPECT_EQ(tree, R"(Program
  VarDecl(name='x', type='integer', loc=1:1)
    init: BinaryExpr(op='-', loc=1:14)
      left: BinaryExpr(op='-', loc=1:14)
        left: Identifier(name='a', loc=1:14)
        right: Identifier(name='b', loc=1:18)
      right: Identifier(name='c', loc=1:22)
)");
}

TEST_F(ParserTest, ParsesPostfixChains) {
    const std::string tree = dump("x: integer = (m[i][j] + f(1, 2)(3))--;");
    EXPECT_EQ(tree, R"(Program
  VarDecl(name='x', type='integer', loc=1:1)
    init: PostfixExpr(op='--', loc=1:15)
      operand: BinaryExpr(op='+', loc=1:15)
        left: IndexExpr(loc=1:15)
          array: IndexExpr(loc=1:15)
            array: Identifier(name='m', loc=1:15)
            index: Identifier(name='i', loc=1:17)
          index: Identifier(name='j', loc=1:20)
        right: CallExpr(loc=1:25)
          callee: CallExpr(loc=1:25)
            callee: Identifier(name='f', loc=1:25)
            args[0]: IntegerLiteral(value=1, loc=1:27)
            args[1]: IntegerLiteral(value=2, loc=1:30)
          args[0]: IntegerLiteral(value=3, loc=1:33)
)");
}

TEST_F(ParserTest, DanglingElseBindsToNearestIf) {
    const std::string tree = dump("f: function void () = { if (a) if (b) x; else y; }");
    EXPECT_EQ(tree, R"(Program
  FuncDecl(name='f', returns='void', loc=1:1)
    body: BlockStmt(loc=1:23)
      statements[0]: IfStmt(loc=1:25)
        condition: Identifier(name='a', loc=1:29)
        then: IfStmt(loc=1:32)
          condition: Identifier(name='b', loc=1:36)
          then: ExprStmt(loc=1:39)
            expression: Identifier(name='x', loc=1:39)
          else: ExprStmt(loc=1:47)
            expression: Identifier(name='y', loc=1:47)
)");
}

TEST_F(ParserTest, IgnoresComments) {
    const std::string tree = dump("/* header */ x: // type follows\n integer; // done");
    EXPECT_EQ(tree, "Program\n  VarDecl(name='x', type='integer', loc=1:14)\n");
}

TEST_F(ParserTest, RecoversFromMissingExpression) {
    Program program = parseSource("x: integer = ;\ny: integer = 42;\n");
    EXPECT_EQ(messages(), Messages{"1:14 expected expression, got ';'"});
    ASSERT_EQ(program.declarations.size(), 1u);
    EXPECT_EQ(program.declarations[0]->name, "y");
}

TEST_F(ParserTest, RecoversFromMissingParenthesis) {
    const std::string tree = dump("f: function integer (y: integer) = {\n"
                                  "  if (y > 0 {\n"
                                  "    return y;\n"
                                  "  }\n"
                                  "  return 0;\n"
                                  "}\n");
    EXPECT_EQ(messages(), Messages{"2:13 expected ')' after condition, got '{'"});
    EXPECT_EQ(tree, R"(Program
  FuncDecl(name='f', returns='integer', loc=1:1)
    params[0]: Param(name='y', type='integer', loc=1:22)
    body: BlockStmt(loc=1:36)
      statements[0]: BlockStmt(loc=2:13)
        statements[0]: ReturnStmt(loc=3:5)
          value: Identifier(name='y', loc=3:12)
      statements[1]: ReturnStmt(loc=5:3)
        value: IntegerLiteral(value=0, loc=5:10)
)");
}

TEST_F(ParserTest, ReportsEveryBrokenStatementInABlock) {
    Program program = parseSource("main: function void () = {\n"
                                  "  x = ;\n"
                                  "  y = 1\n"
                                  "  print \"ok\";\n"
                                  "  z = (2;\n"
                                  "  return;\n"
                                  "}\n"
                                  "after: integer;\n");
    EXPECT_EQ(messages(), (Messages{"2:7 expected expression, got ';'",
                                    "4:3 expected ';' after expression, got 'print'",
                                    "5:9 expected ')' after expression, got ';'"}));
    ASSERT_EQ(program.declarations.size(), 2u);
    EXPECT_EQ(program.declarations[1]->name, "after");
}

TEST_F(ParserTest, SkipsBrokenDeclarationIncludingItsBody) {
    Program program = parseSource("f: function integer (x integer) = {\n"
                                  "  local: integer = 1;\n"
                                  "  return local;\n"
                                  "}\n"
                                  "g: integer;\n");
    EXPECT_EQ(messages(), Messages{"1:24 expected ':' after parameter name, got 'integer'"});
    ASSERT_EQ(program.declarations.size(), 1u);
    EXPECT_EQ(program.declarations[0]->name, "g");
}

TEST_F(ParserTest, RecoversFromStrayTopLevelTokens) {
    Program program = parseSource("} ) ; if (x) return; ok: boolean;");
    EXPECT_EQ(messages(), Messages{"1:1 expected declaration, got '}'"});
    ASSERT_EQ(program.declarations.size(), 1u);
    EXPECT_EQ(program.declarations[0]->name, "ok");
}

TEST_F(ParserTest, ReportsUnclosedBlockButKeepsFunction) {
    Program program = parseSource("f: function void () = {\n  x = 1;\n");
    EXPECT_EQ(messages(), Messages{"3:1 expected '}' at end of block, got end of file"});
    ASSERT_EQ(program.declarations.size(), 1u);
    EXPECT_EQ(program.declarations[0]->name, "f");
}

TEST_F(ParserTest, ReportsInvalidAssignmentTarget) {
    parseSource("f: function void () = { 1 = x; f() = 2; a[0] = b = 3; }");
    EXPECT_EQ(messages(),
              (Messages{"1:27 invalid assignment target", "1:36 invalid assignment target"}));
}

TEST_F(ParserTest, ReportsOutOfRangeInteger) {
    parseSource("x: integer = 99999999999999999999;");
    EXPECT_EQ(messages(), Messages{"1:14 integer literal 99999999999999999999 is out of range"});
}

TEST_F(ParserTest, SkipsInvalidTokensReportedByLexer) {
    Program program = parseSource("x: integer = 5@;\ny: integer = 10;\n");
    EXPECT_EQ(messages(), Messages{"1:15 unexpected character '@'"});
    EXPECT_EQ(program.declarations.size(), 2u);
}

TEST_F(ParserTest, ReportsUnexpectedEndOfFile) {
    parseSource("x: array [3");
    EXPECT_EQ(messages(), Messages{"1:12 expected ']' after array size, got end of file"});
}

TEST_F(ParserTest, TerminatesOnArbitraryTokenSoup) {
    const std::string soup = ") ( ] [ } { ; : = , ++ x : y ( if else for while return print { } ";
    std::string source;
    for (int i = 0; i < 50; ++i) {
        source += soup;
    }
    parseSource(source);
    EXPECT_FALSE(diagnostics_.empty());
}

TEST_F(ParserTest, EmptyProgramIsValid) {
    EXPECT_EQ(dump(""), "Program\n");
    EXPECT_TRUE(diagnostics_.empty());
}

} // namespace
} // namespace bminor
