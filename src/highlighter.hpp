#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include "token.hpp"

#include <cstddef>
#include <vector>

namespace bminor {

enum class Highlight {
    Keyword,
    TypeName,
    Variable,
    Function,
    Number,
    String,
    Boolean,
    Operator,
    Comment,
    Error,
};

struct HighlightSpan {
    std::size_t offset;
    std::size_t length;
    Highlight highlight;
};

std::vector<HighlightSpan> highlight(const std::vector<Token>& tokens, Program& program,
                                     const std::vector<Diagnostic>& diagnostics);

} // namespace bminor
