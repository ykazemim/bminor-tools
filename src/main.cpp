#include "ast_printer.hpp"
#include "diagnostic.hpp"
#include "highlighter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "render.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view usage = R"(usage: bminor <command> [options] <file>

commands:
  tokens       print the token stream
  ast          print the abstract syntax tree
  highlight    print the source with syntax highlighting

options:
  --html       emit a standalone HTML page instead of ANSI colors (highlight only)
  -o <file>    write the output to <file> instead of standard output
  -h, --help   show this help
)";

constexpr int exitSourceErrors = 1;
constexpr int exitUsageError = 2;

struct Options {
    std::string command;
    std::string input;
    std::string output;
    bool html = false;
};

std::optional<Options> parseArguments(const std::vector<std::string_view>& args) {
    if (args.empty()) {
        return std::nullopt;
    }
    Options options;
    options.command = args.front();
    if (options.command != "tokens" && options.command != "ast" && options.command != "highlight") {
        std::cerr << "bminor: unknown command '" << options.command << "'\n";
        return std::nullopt;
    }

    for (std::size_t i = 1; i < args.size(); ++i) {
        const std::string_view arg = args[i];
        if (arg == "--html" && options.command == "highlight") {
            options.html = true;
        } else if (arg == "-o" && i + 1 < args.size()) {
            options.output = args[++i];
        } else if (!arg.empty() && arg.front() == '-') {
            std::cerr << "bminor: unexpected option '" << arg << "'\n";
            return std::nullopt;
        } else if (options.input.empty()) {
            options.input = arg;
        } else {
            std::cerr << "bminor: more than one input file\n";
            return std::nullopt;
        }
    }

    if (options.input.empty()) {
        std::cerr << "bminor: no input file\n";
        return std::nullopt;
    }
    return options;
}

std::optional<std::string> readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

bool writeOutput(const std::string& path, const std::string& text) {
    if (path.empty()) {
        std::cout << text;
        return static_cast<bool>(std::cout);
    }
    std::ofstream file(path, std::ios::binary);
    file << text;
    return static_cast<bool>(file);
}

std::string run(const Options& options, const bminor::SourceFile& source,
                std::vector<bminor::Diagnostic>& diagnostics) {
    const std::vector<bminor::Token> tokens = bminor::tokenize(source, diagnostics);
    std::ostringstream out;

    if (options.command == "tokens") {
        for (const bminor::Token& token : tokens) {
            const std::string location =
                std::to_string(token.location.line) + ':' + std::to_string(token.location.column);
            out << std::left << std::setw(8) << location << bminor::toString(token) << '\n';
        }
        return out.str();
    }

    bminor::Program program = bminor::parse(tokens, diagnostics);
    if (options.command == "ast") {
        bminor::printAst(out, program);
        return out.str();
    }

    const auto spans = bminor::highlight(tokens, program, diagnostics);
    return options.html ? bminor::renderHtml(source.text, spans, source.name)
                        : bminor::renderAnsi(source.text, spans);
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    if (!args.empty() && (args.front() == "-h" || args.front() == "--help")) {
        std::cout << usage;
        return EXIT_SUCCESS;
    }

    const std::optional<Options> options = parseArguments(args);
    if (!options) {
        std::cerr << usage;
        return exitUsageError;
    }

    std::optional<std::string> text = readFile(options->input);
    if (!text) {
        std::cerr << "bminor: cannot read '" << options->input << "'\n";
        return exitUsageError;
    }

    const bminor::SourceFile source{options->input, std::move(*text)};
    std::vector<bminor::Diagnostic> diagnostics;
    const std::string output = run(*options, source, diagnostics);

    if (!writeOutput(options->output, output)) {
        std::cerr << "bminor: cannot write '" << options->output << "'\n";
        return exitUsageError;
    }

    bminor::sortByLocation(diagnostics);
    for (const bminor::Diagnostic& diagnostic : diagnostics) {
        std::cerr << diagnostic << '\n';
    }
    return diagnostics.empty() ? EXIT_SUCCESS : exitSourceErrors;
}
