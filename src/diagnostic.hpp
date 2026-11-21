#pragma once

#include "source.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace bminor {

struct Diagnostic {
    SourceLocation location;
    std::string message;
};

std::ostream& operator<<(std::ostream& out, const Diagnostic& diagnostic);

void sortByLocation(std::vector<Diagnostic>& diagnostics);

} // namespace bminor
