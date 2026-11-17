#pragma once

#include "ast.hpp"
#include "diagnostic.hpp"
#include "token.hpp"

#include <vector>

namespace bminor {

Program parse(const std::vector<Token>& tokens, std::vector<Diagnostic>& diagnostics);

} // namespace bminor
