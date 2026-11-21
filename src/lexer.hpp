#pragma once

#include "diagnostic.hpp"
#include "source.hpp"
#include "token.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace bminor {

std::vector<Token> tokenize(const SourceFile& source, std::vector<Diagnostic>& diagnostics);

std::string unescape(std::string_view text);

} // namespace bminor
