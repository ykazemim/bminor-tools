#pragma once

#include "source.hpp"
#include "token.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace bminor {

struct Type {
    enum class Kind { Integer, Boolean, Char, String, Void, Auto, Array };

    Kind kind;
    std::shared_ptr<const Type> element;
    std::optional<std::int64_t> size;
};

using TypePtr = std::shared_ptr<const Type>;

std::string toString(const Type& type);

struct VarDecl;
struct FuncDecl;
struct BlockStmt;
struct DeclStmt;
struct ExprStmt;
struct IfStmt;
struct ForStmt;
struct WhileStmt;
struct ReturnStmt;
struct PrintStmt;
struct IntegerLiteral;
struct BooleanLiteral;
struct CharLiteral;
struct StringLiteral;
struct Identifier;
struct UnaryExpr;
struct PostfixExpr;
struct BinaryExpr;
struct AssignExpr;
struct CallExpr;
struct IndexExpr;
struct ArrayLiteral;

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visit(VarDecl& node) = 0;
    virtual void visit(FuncDecl& node) = 0;

    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(DeclStmt& node) = 0;
    virtual void visit(ExprStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(ForStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(PrintStmt& node) = 0;

    virtual void visit(IntegerLiteral& node) = 0;
    virtual void visit(BooleanLiteral& node) = 0;
    virtual void visit(CharLiteral& node) = 0;
    virtual void visit(StringLiteral& node) = 0;
    virtual void visit(Identifier& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(PostfixExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(AssignExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(IndexExpr& node) = 0;
    virtual void visit(ArrayLiteral& node) = 0;
};

struct Node {
    explicit Node(SourceLocation location) : location(location) {}
    virtual ~Node() = default;

    virtual void accept(Visitor& visitor) = 0;

    SourceLocation location;
    TypePtr type;
};

struct Expr : Node {
    using Node::Node;
};

struct Stmt : Node {
    using Node::Node;
};

struct Decl : Node {
    Decl(SourceLocation location, std::string name) : Node(location), name(std::move(name)) {}

    std::string name;
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;

struct Param {
    std::string name;
    TypePtr declaredType;
    SourceLocation location;
};

struct VarDecl final : Decl {
    VarDecl(SourceLocation location, std::string name, TypePtr declaredType, ExprPtr initializer)
        : Decl(location, std::move(name)), declaredType(std::move(declaredType)),
          initializer(std::move(initializer)) {}
    void accept(Visitor& visitor) override;

    TypePtr declaredType;
    ExprPtr initializer;
};

struct FuncDecl final : Decl {
    FuncDecl(SourceLocation location, std::string name, TypePtr returnType,
             std::vector<Param> params, std::unique_ptr<BlockStmt> body)
        : Decl(location, std::move(name)), returnType(std::move(returnType)),
          params(std::move(params)), body(std::move(body)) {}
    void accept(Visitor& visitor) override;

    TypePtr returnType;
    std::vector<Param> params;
    std::unique_ptr<BlockStmt> body;
};

struct BlockStmt final : Stmt {
    BlockStmt(SourceLocation location, std::vector<StmtPtr> statements)
        : Stmt(location), statements(std::move(statements)) {}
    void accept(Visitor& visitor) override;

    std::vector<StmtPtr> statements;
};

struct DeclStmt final : Stmt {
    explicit DeclStmt(DeclPtr declaration)
        : Stmt(declaration->location), declaration(std::move(declaration)) {}
    void accept(Visitor& visitor) override;

    DeclPtr declaration;
};

struct ExprStmt final : Stmt {
    explicit ExprStmt(ExprPtr expression)
        : Stmt(expression->location), expression(std::move(expression)) {}
    void accept(Visitor& visitor) override;

    ExprPtr expression;
};

struct IfStmt final : Stmt {
    IfStmt(SourceLocation location, ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch)
        : Stmt(location), condition(std::move(condition)), thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}
    void accept(Visitor& visitor) override;

    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
};

struct ForStmt final : Stmt {
    ForStmt(SourceLocation location, ExprPtr init, ExprPtr condition, ExprPtr step, StmtPtr body)
        : Stmt(location), init(std::move(init)), condition(std::move(condition)),
          step(std::move(step)), body(std::move(body)) {}
    void accept(Visitor& visitor) override;

    ExprPtr init;
    ExprPtr condition;
    ExprPtr step;
    StmtPtr body;
};

struct WhileStmt final : Stmt {
    WhileStmt(SourceLocation location, ExprPtr condition, StmtPtr body)
        : Stmt(location), condition(std::move(condition)), body(std::move(body)) {}
    void accept(Visitor& visitor) override;

    ExprPtr condition;
    StmtPtr body;
};

struct ReturnStmt final : Stmt {
    ReturnStmt(SourceLocation location, ExprPtr value) : Stmt(location), value(std::move(value)) {}
    void accept(Visitor& visitor) override;

    ExprPtr value;
};

struct PrintStmt final : Stmt {
    PrintStmt(SourceLocation location, std::vector<ExprPtr> arguments)
        : Stmt(location), arguments(std::move(arguments)) {}
    void accept(Visitor& visitor) override;

    std::vector<ExprPtr> arguments;
};

struct IntegerLiteral final : Expr {
    IntegerLiteral(SourceLocation location, std::int64_t value) : Expr(location), value(value) {}
    void accept(Visitor& visitor) override;

    std::int64_t value;
};

struct BooleanLiteral final : Expr {
    BooleanLiteral(SourceLocation location, bool value) : Expr(location), value(value) {}
    void accept(Visitor& visitor) override;

    bool value;
};

struct CharLiteral final : Expr {
    CharLiteral(SourceLocation location, char value) : Expr(location), value(value) {}
    void accept(Visitor& visitor) override;

    char value;
};

struct StringLiteral final : Expr {
    StringLiteral(SourceLocation location, std::string value)
        : Expr(location), value(std::move(value)) {}
    void accept(Visitor& visitor) override;

    std::string value;
};

struct Identifier final : Expr {
    Identifier(SourceLocation location, std::string name) : Expr(location), name(std::move(name)) {}
    void accept(Visitor& visitor) override;

    std::string name;
};

struct UnaryExpr final : Expr {
    UnaryExpr(SourceLocation location, TokenKind op, ExprPtr operand)
        : Expr(location), op(op), operand(std::move(operand)) {}
    void accept(Visitor& visitor) override;

    TokenKind op;
    ExprPtr operand;
};

struct PostfixExpr final : Expr {
    PostfixExpr(TokenKind op, ExprPtr operand)
        : Expr(operand->location), op(op), operand(std::move(operand)) {}
    void accept(Visitor& visitor) override;

    TokenKind op;
    ExprPtr operand;
};

struct BinaryExpr final : Expr {
    BinaryExpr(TokenKind op, ExprPtr left, ExprPtr right)
        : Expr(left->location), op(op), left(std::move(left)), right(std::move(right)) {}
    void accept(Visitor& visitor) override;

    TokenKind op;
    ExprPtr left;
    ExprPtr right;
};

struct AssignExpr final : Expr {
    AssignExpr(ExprPtr target, ExprPtr value)
        : Expr(target->location), target(std::move(target)), value(std::move(value)) {}
    void accept(Visitor& visitor) override;

    ExprPtr target;
    ExprPtr value;
};

struct CallExpr final : Expr {
    CallExpr(ExprPtr callee, std::vector<ExprPtr> arguments)
        : Expr(callee->location), callee(std::move(callee)), arguments(std::move(arguments)) {}
    void accept(Visitor& visitor) override;

    ExprPtr callee;
    std::vector<ExprPtr> arguments;
};

struct IndexExpr final : Expr {
    IndexExpr(ExprPtr array, ExprPtr index)
        : Expr(array->location), array(std::move(array)), index(std::move(index)) {}
    void accept(Visitor& visitor) override;

    ExprPtr array;
    ExprPtr index;
};

struct ArrayLiteral final : Expr {
    ArrayLiteral(SourceLocation location, std::vector<ExprPtr> elements)
        : Expr(location), elements(std::move(elements)) {}
    void accept(Visitor& visitor) override;

    std::vector<ExprPtr> elements;
};

struct Program {
    std::vector<DeclPtr> declarations;
};

class RecursiveVisitor : public Visitor {
public:
    void visit(VarDecl& node) override;
    void visit(FuncDecl& node) override;

    void visit(BlockStmt& node) override;
    void visit(DeclStmt& node) override;
    void visit(ExprStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(ForStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(PrintStmt& node) override;

    void visit(IntegerLiteral&) override {}
    void visit(BooleanLiteral&) override {}
    void visit(CharLiteral&) override {}
    void visit(StringLiteral&) override {}
    void visit(Identifier&) override {}
    void visit(UnaryExpr& node) override;
    void visit(PostfixExpr& node) override;
    void visit(BinaryExpr& node) override;
    void visit(AssignExpr& node) override;
    void visit(CallExpr& node) override;
    void visit(IndexExpr& node) override;
    void visit(ArrayLiteral& node) override;

    void traverse(Program& program);

protected:
    void traverse(Node* node);
};

} // namespace bminor
