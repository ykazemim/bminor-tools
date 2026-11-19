#include "highlighter.hpp"

#include <optional>
#include <unordered_set>

namespace bminor {

namespace {

class FunctionNameCollector final : public RecursiveVisitor {
public:
    using RecursiveVisitor::visit;

    void visit(FuncDecl& node) override {
        offsets.insert(node.location.offset);
        RecursiveVisitor::visit(node);
    }

    void visit(CallExpr& node) override {
        if (dynamic_cast<const Identifier*>(node.callee.get())) {
            offsets.insert(node.callee->location.offset);
        }
        RecursiveVisitor::visit(node);
    }

    std::unordered_set<std::size_t> offsets;
};

std::optional<Highlight> classify(TokenKind kind) {
    if (kind == TokenKind::KwTrue || kind == TokenKind::KwFalse) {
        return Highlight::Boolean;
    }
    if (isTypeName(kind)) {
        return Highlight::TypeName;
    }
    if (isKeyword(kind)) {
        return Highlight::Keyword;
    }
    if (isOperator(kind)) {
        return Highlight::Operator;
    }
    switch (kind) {
    case TokenKind::Identifier:
        return Highlight::Variable;
    case TokenKind::IntegerLiteral:
        return Highlight::Number;
    case TokenKind::CharLiteral:
    case TokenKind::StringLiteral:
        return Highlight::String;
    case TokenKind::Comment:
        return Highlight::Comment;
    case TokenKind::Invalid:
        return Highlight::Error;
    default:
        return std::nullopt;
    }
}

} // namespace

std::vector<HighlightSpan> highlight(const std::vector<Token>& tokens, Program& program,
                                     const std::vector<Diagnostic>& diagnostics) {
    FunctionNameCollector functions;
    functions.traverse(program);

    std::unordered_set<std::size_t> errors;
    for (const Diagnostic& diagnostic : diagnostics) {
        errors.insert(diagnostic.location.offset);
    }

    std::vector<HighlightSpan> spans;
    for (const Token& token : tokens) {
        if (token.lexeme.empty()) {
            continue;
        }
        const std::size_t offset = token.location.offset;
        std::optional<Highlight> style = classify(token.kind);
        if (errors.count(offset) > 0) {
            style = Highlight::Error;
        } else if (token.kind == TokenKind::Identifier && functions.offsets.count(offset) > 0) {
            style = Highlight::Function;
        }
        if (style) {
            spans.push_back({offset, token.lexeme.size(), *style});
        }
    }
    return spans;
}

} // namespace bminor
