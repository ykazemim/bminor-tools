#include "ast_printer.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace bminor {

namespace {

std::string quoted(std::string_view text, char quote = '\'') {
    std::string result(1, quote);
    for (char c : text) {
        switch (c) {
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\0':
            result += "\\0";
            break;
        case '\\':
            result += "\\\\";
            break;
        default:
            result += c;
        }
    }
    return result + quote;
}

std::string indexed(std::string_view label, std::size_t index) {
    return std::string(label) + '[' + std::to_string(index) + ']';
}

class AstPrinter final : public Visitor {
public:
    explicit AstPrinter(std::ostream& out) : out_(out) {}

    void print(Program& program) {
        out_ << "Program\n";
        for (auto& declaration : program.declarations) {
            child("", declaration.get());
        }
    }

    void visit(VarDecl& node) override {
        line(node.location, "VarDecl",
             {"name=" + quoted(node.name), "type=" + quoted(toString(*node.declaredType))});
        child("init", node.initializer.get());
    }

    void visit(FuncDecl& node) override {
        line(node.location, "FuncDecl",
             {"name=" + quoted(node.name), "returns=" + quoted(toString(*node.returnType))});
        ++depth_;
        for (std::size_t i = 0; i < node.params.size(); ++i) {
            const Param& param = node.params[i];
            label_ = indexed("params", i);
            line(param.location, "Param",
                 {"name=" + quoted(param.name), "type=" + quoted(toString(*param.declaredType))});
        }
        --depth_;
        child("body", node.body.get());
    }

    void visit(BlockStmt& node) override {
        line(node.location, "BlockStmt");
        children("statements", node.statements);
    }

    void visit(DeclStmt& node) override {
        line(node.location, "DeclStmt");
        child("declaration", node.declaration.get());
    }

    void visit(ExprStmt& node) override {
        line(node.location, "ExprStmt");
        child("expression", node.expression.get());
    }

    void visit(IfStmt& node) override {
        line(node.location, "IfStmt");
        child("condition", node.condition.get());
        child("then", node.thenBranch.get());
        child("else", node.elseBranch.get());
    }

    void visit(ForStmt& node) override {
        line(node.location, "ForStmt");
        child("init", node.init.get());
        child("condition", node.condition.get());
        child("step", node.step.get());
        child("body", node.body.get());
    }

    void visit(WhileStmt& node) override {
        line(node.location, "WhileStmt");
        child("condition", node.condition.get());
        child("body", node.body.get());
    }

    void visit(ReturnStmt& node) override {
        line(node.location, "ReturnStmt");
        child("value", node.value.get());
    }

    void visit(PrintStmt& node) override {
        line(node.location, "PrintStmt");
        children("args", node.arguments);
    }

    void visit(IntegerLiteral& node) override {
        line(node.location, "IntegerLiteral", {"value=" + std::to_string(node.value)});
    }

    void visit(BooleanLiteral& node) override {
        line(node.location, "BooleanLiteral",
             {std::string("value=") + (node.value ? "true" : "false")});
    }

    void visit(CharLiteral& node) override {
        line(node.location, "CharLiteral", {"value=" + quoted(std::string_view(&node.value, 1))});
    }

    void visit(StringLiteral& node) override {
        line(node.location, "StringLiteral", {"value=" + quoted(node.value, '"')});
    }

    void visit(Identifier& node) override {
        line(node.location, "Identifier", {"name=" + quoted(node.name)});
    }

    void visit(UnaryExpr& node) override {
        line(node.location, "UnaryExpr", {"op=" + quoted(spelling(node.op))});
        child("operand", node.operand.get());
    }

    void visit(PostfixExpr& node) override {
        line(node.location, "PostfixExpr", {"op=" + quoted(spelling(node.op))});
        child("operand", node.operand.get());
    }

    void visit(BinaryExpr& node) override {
        line(node.location, "BinaryExpr", {"op=" + quoted(spelling(node.op))});
        child("left", node.left.get());
        child("right", node.right.get());
    }

    void visit(AssignExpr& node) override {
        line(node.location, "AssignExpr");
        child("target", node.target.get());
        child("value", node.value.get());
    }

    void visit(CallExpr& node) override {
        line(node.location, "CallExpr");
        child("callee", node.callee.get());
        children("args", node.arguments);
    }

    void visit(IndexExpr& node) override {
        line(node.location, "IndexExpr");
        child("array", node.array.get());
        child("index", node.index.get());
    }

    void visit(ArrayLiteral& node) override {
        line(node.location, "ArrayLiteral");
        children("elements", node.elements);
    }

private:
    void line(const SourceLocation& location, std::string_view kind,
              const std::vector<std::string>& fields = {}) {
        out_ << std::string(depth_ * 2, ' ');
        if (!label_.empty()) {
            out_ << label_ << ": ";
            label_.clear();
        }
        out_ << kind << '(';
        for (const std::string& field : fields) {
            out_ << field << ", ";
        }
        out_ << "loc=" << location.line << ':' << location.column << ")\n";
    }

    void child(std::string label, Node* node) {
        if (!node) {
            return;
        }
        ++depth_;
        label_ = std::move(label);
        node->accept(*this);
        --depth_;
    }

    template <typename T> void children(std::string_view label, std::vector<T>& nodes) {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            child(indexed(label, i), nodes[i].get());
        }
    }

    std::ostream& out_;
    std::size_t depth_ = 0;
    std::string label_;
};

} // namespace

void printAst(std::ostream& out, Program& program) { AstPrinter(out).print(program); }

} // namespace bminor
