#include "ast.hpp"

namespace bminor {

std::string toString(const Type& type) {
    switch (type.kind) {
    case Type::Kind::Integer:
        return "integer";
    case Type::Kind::Boolean:
        return "boolean";
    case Type::Kind::Char:
        return "char";
    case Type::Kind::String:
        return "string";
    case Type::Kind::Void:
        return "void";
    case Type::Kind::Auto:
        return "auto";
    case Type::Kind::Array: {
        const std::string size = type.size ? std::to_string(*type.size) : "";
        return "array [" + size + "] " + (type.element ? toString(*type.element) : "?");
    }
    }
    return "?";
}

void VarDecl::accept(Visitor& visitor) { visitor.visit(*this); }
void FuncDecl::accept(Visitor& visitor) { visitor.visit(*this); }

void BlockStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void DeclStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void ExprStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void IfStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void ForStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void WhileStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void ReturnStmt::accept(Visitor& visitor) { visitor.visit(*this); }
void PrintStmt::accept(Visitor& visitor) { visitor.visit(*this); }

void IntegerLiteral::accept(Visitor& visitor) { visitor.visit(*this); }
void BooleanLiteral::accept(Visitor& visitor) { visitor.visit(*this); }
void CharLiteral::accept(Visitor& visitor) { visitor.visit(*this); }
void StringLiteral::accept(Visitor& visitor) { visitor.visit(*this); }
void Identifier::accept(Visitor& visitor) { visitor.visit(*this); }
void UnaryExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void PostfixExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void BinaryExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void AssignExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void CallExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void IndexExpr::accept(Visitor& visitor) { visitor.visit(*this); }
void ArrayLiteral::accept(Visitor& visitor) { visitor.visit(*this); }

void RecursiveVisitor::traverse(Program& program) {
    for (auto& declaration : program.declarations) {
        traverse(declaration.get());
    }
}

void RecursiveVisitor::traverse(Node* node) {
    if (node) {
        node->accept(*this);
    }
}

void RecursiveVisitor::visit(VarDecl& node) { traverse(node.initializer.get()); }

void RecursiveVisitor::visit(FuncDecl& node) { traverse(node.body.get()); }

void RecursiveVisitor::visit(BlockStmt& node) {
    for (auto& statement : node.statements) {
        traverse(statement.get());
    }
}

void RecursiveVisitor::visit(DeclStmt& node) { traverse(node.declaration.get()); }

void RecursiveVisitor::visit(ExprStmt& node) { traverse(node.expression.get()); }

void RecursiveVisitor::visit(IfStmt& node) {
    traverse(node.condition.get());
    traverse(node.thenBranch.get());
    traverse(node.elseBranch.get());
}

void RecursiveVisitor::visit(ForStmt& node) {
    traverse(node.init.get());
    traverse(node.condition.get());
    traverse(node.step.get());
    traverse(node.body.get());
}

void RecursiveVisitor::visit(WhileStmt& node) {
    traverse(node.condition.get());
    traverse(node.body.get());
}

void RecursiveVisitor::visit(ReturnStmt& node) { traverse(node.value.get()); }

void RecursiveVisitor::visit(PrintStmt& node) {
    for (auto& argument : node.arguments) {
        traverse(argument.get());
    }
}

void RecursiveVisitor::visit(UnaryExpr& node) { traverse(node.operand.get()); }

void RecursiveVisitor::visit(PostfixExpr& node) { traverse(node.operand.get()); }

void RecursiveVisitor::visit(BinaryExpr& node) {
    traverse(node.left.get());
    traverse(node.right.get());
}

void RecursiveVisitor::visit(AssignExpr& node) {
    traverse(node.target.get());
    traverse(node.value.get());
}

void RecursiveVisitor::visit(CallExpr& node) {
    traverse(node.callee.get());
    for (auto& argument : node.arguments) {
        traverse(argument.get());
    }
}

void RecursiveVisitor::visit(IndexExpr& node) {
    traverse(node.array.get());
    traverse(node.index.get());
}

void RecursiveVisitor::visit(ArrayLiteral& node) {
    for (auto& element : node.elements) {
        traverse(element.get());
    }
}

} // namespace bminor
