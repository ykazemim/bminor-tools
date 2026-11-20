#include "render.hpp"

namespace bminor {

namespace {

constexpr std::string_view ansiReset = "\x1b[0m";

constexpr std::string_view htmlStyle = R"(body {
  margin: 0;
  background: #1e1e1e;
  color: #d4d4d4;
}
pre {
  margin: 0;
  padding: 1.5rem;
  font: 14px/1.5 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
  tab-size: 4;
}
.kw { color: #569cd6; font-weight: bold; }
.type { color: #4ec9b0; }
.var { color: #d4d4d4; }
.fn { color: #dcdcaa; }
.num, .bool { color: #d19a66; }
.str { color: #98c379; }
.op { color: #abb2bf; }
.comment { color: #7f848e; font-style: italic; }
.error { text-decoration: underline wavy #f44747; }
)";

std::string_view ansiCode(Highlight highlight) {
    switch (highlight) {
    case Highlight::Keyword:
        return "1;34";
    case Highlight::TypeName:
        return "36";
    case Highlight::Variable:
        return "";
    case Highlight::Function:
        return "33";
    case Highlight::Number:
    case Highlight::Boolean:
        return "38;5;208";
    case Highlight::String:
        return "32";
    case Highlight::Operator:
        return "37";
    case Highlight::Comment:
        return "3;90";
    case Highlight::Error:
        return "4;31";
    }
    return "";
}

std::string_view cssClass(Highlight highlight) {
    switch (highlight) {
    case Highlight::Keyword:
        return "kw";
    case Highlight::TypeName:
        return "type";
    case Highlight::Variable:
        return "var";
    case Highlight::Function:
        return "fn";
    case Highlight::Number:
        return "num";
    case Highlight::Boolean:
        return "bool";
    case Highlight::String:
        return "str";
    case Highlight::Operator:
        return "op";
    case Highlight::Comment:
        return "comment";
    case Highlight::Error:
        return "error";
    }
    return "";
}

void appendColored(std::string& out, std::string_view text, std::string_view code) {
    if (code.empty()) {
        out += text;
        return;
    }
    std::size_t start = 0;
    while (true) {
        const std::size_t newline = text.find('\n', start);
        const std::string_view line = text.substr(start, newline - start);
        if (!line.empty()) {
            out += "\x1b[";
            out += code;
            out += 'm';
            out += line;
            out += ansiReset;
        }
        if (newline == std::string_view::npos) {
            return;
        }
        out += '\n';
        start = newline + 1;
    }
}

void appendEscaped(std::string& out, std::string_view text) {
    for (char c : text) {
        switch (c) {
        case '&':
            out += "&amp;";
            break;
        case '<':
            out += "&lt;";
            break;
        case '>':
            out += "&gt;";
            break;
        case '"':
            out += "&quot;";
            break;
        default:
            out += c;
        }
    }
}

template <typename AppendPlain, typename AppendSpan>
void walk(std::string_view source, const std::vector<HighlightSpan>& spans, AppendPlain appendPlain,
          AppendSpan appendSpan) {
    std::size_t position = 0;
    for (const HighlightSpan& span : spans) {
        if (span.offset < position || span.offset + span.length > source.size()) {
            continue;
        }
        appendPlain(source.substr(position, span.offset - position));
        appendSpan(source.substr(span.offset, span.length), span.highlight);
        position = span.offset + span.length;
    }
    appendPlain(source.substr(position));
}

} // namespace

std::string renderAnsi(std::string_view source, const std::vector<HighlightSpan>& spans) {
    std::string out;
    walk(
        source, spans, [&](std::string_view text) { out += text; },
        [&](std::string_view text, Highlight highlight) {
            appendColored(out, text, ansiCode(highlight));
        });
    return out;
}

std::string renderHtml(std::string_view source, const std::vector<HighlightSpan>& spans,
                       std::string_view title) {
    std::string out = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n";
    out += "<title>";
    appendEscaped(out, title);
    out += "</title>\n<style>\n";
    out += htmlStyle;
    out += "</style>\n</head>\n<body>\n<pre>";
    walk(
        source, spans, [&](std::string_view text) { appendEscaped(out, text); },
        [&](std::string_view text, Highlight highlight) {
            out += "<span class=\"";
            out += cssClass(highlight);
            out += "\">";
            appendEscaped(out, text);
            out += "</span>";
        });
    out += "</pre>\n</body>\n</html>\n";
    return out;
}

} // namespace bminor
