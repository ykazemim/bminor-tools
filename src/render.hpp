#pragma once

#include "highlighter.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace bminor {

std::string renderAnsi(std::string_view source, const std::vector<HighlightSpan>& spans);

std::string renderHtml(std::string_view source, const std::vector<HighlightSpan>& spans,
                       std::string_view title);

} // namespace bminor
