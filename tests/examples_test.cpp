#include "highlighter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace bminor {
namespace {

namespace fs = std::filesystem;

std::vector<fs::path> examplesIn(const fs::path& directory) {
    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.path().extension() == ".bminor") {
            paths.push_back(entry.path());
        }
    }
    return paths;
}

std::vector<Diagnostic> analyze(const SourceFile& source) {
    std::vector<Diagnostic> diagnostics;
    const std::vector<Token> tokens = tokenize(source, diagnostics);
    Program program = parse(tokens, diagnostics);
    const std::vector<HighlightSpan> spans = highlight(tokens, program, diagnostics);
    EXPECT_FALSE(renderAnsi(source.text, spans).empty());
    EXPECT_FALSE(renderHtml(source.text, spans, source.name).empty());
    return diagnostics;
}

SourceFile load(const fs::path& path) {
    std::ifstream file(path);
    std::ostringstream text;
    text << file.rdbuf();
    return SourceFile{path.filename().string(), text.str()};
}

TEST(ExamplesTest, ValidProgramsHaveNoDiagnostics) {
    const std::vector<fs::path> paths = examplesIn(BMINOR_EXAMPLES_DIR);
    ASSERT_FALSE(paths.empty());
    for (const fs::path& path : paths) {
        const SourceFile source = load(path);
        EXPECT_TRUE(analyze(source).empty()) << path;
    }
}

TEST(ExamplesTest, InvalidProgramsAreReportedWithoutCrashing) {
    const std::vector<fs::path> paths = examplesIn(fs::path(BMINOR_EXAMPLES_DIR) / "errors");
    ASSERT_FALSE(paths.empty());
    for (const fs::path& path : paths) {
        const SourceFile source = load(path);
        EXPECT_FALSE(analyze(source).empty()) << path;
    }
}

} // namespace
} // namespace bminor
