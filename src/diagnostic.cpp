#include "diagnostic.hpp"

#include <algorithm>

namespace bminor {

std::ostream& operator<<(std::ostream& out, const Diagnostic& diagnostic) {
    const SourceLocation& location = diagnostic.location;
    return out << location.file << ':' << location.line << ':' << location.column
               << ": error: " << diagnostic.message;
}

void sortByLocation(std::vector<Diagnostic>& diagnostics) {
    std::stable_sort(diagnostics.begin(), diagnostics.end(),
                     [](const Diagnostic& a, const Diagnostic& b) {
                         return a.location.offset < b.location.offset;
                     });
}

} // namespace bminor
