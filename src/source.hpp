#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace bminor {

struct SourceFile {
    std::string name;
    std::string text;
};

struct SourceLocation {
    std::string_view file;
    int line = 1;
    int column = 1;
    std::size_t offset = 0;
};

} // namespace bminor
