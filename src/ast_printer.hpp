#pragma once

#include "ast.hpp"

#include <ostream>

namespace bminor {

void printAst(std::ostream& out, Program& program);

} // namespace bminor
