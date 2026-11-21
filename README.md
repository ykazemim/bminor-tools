# bminor-tools

[![CI](https://github.com/ykazemim/bminor-tools/actions/workflows/ci.yml/badge.svg)](https://github.com/ykazemim/bminor-tools/actions/workflows/ci.yml)

A C++ toolkit for the [B-Minor](https://www3.nd.edu/~dthain/compilerbook/) language: a lexer, a recursive-descent parser, an abstract syntax tree, and a syntax highlighter that uses the tree to tell functions apart from variables. It recovers from errors, so broken programs still get a token stream, an AST and highlighted output, along with precise diagnostics.

This is the first phase of a compiler design course project. Later phases add semantic analysis (scopes and types) and program analysis (control-flow and call graphs).

## Building

Requirements: a C++17 compiler such as GCC or Clang, and CMake 3.24 or newer.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

GoogleTest is downloaded automatically for the tests. To build only the tool, add `-DBUILD_TESTING=OFF`.

## Usage

```
usage: bminor <command> [options] <file>

commands:
  tokens       print the token stream
  ast          print the abstract syntax tree
  highlight    print the source with syntax highlighting

options:
  --html       emit a standalone HTML page instead of ANSI colors (highlight only)
  -o <file>    write the output to <file> instead of standard output
```

Highlight a file in the terminal, or write an HTML page:

```sh
./build/bminor highlight examples/showcase.bminor
./build/bminor highlight --html -o showcase.html examples/showcase.bminor
```

Inspect the tokens and the tree:

```console
$ ./build/bminor tokens examples/factorial.bminor | head -4
1:1     IDENT(factorial)
1:10    COLON
1:12    KW(function)
1:21    KW(integer)

$ ./build/bminor ast examples/factorial.bminor | head -6
Program
  FuncDecl(name='factorial', returns='integer', loc=1:1)
    params[0]: Param(name='n', type='integer', loc=1:31)
    body: BlockStmt(loc=1:46)
      statements[0]: IfStmt(loc=2:5)
        condition: BinaryExpr(op='<=', loc=2:9)
```

Errors are reported in the usual compiler format, and analysis continues after each one:

```console
$ ./build/bminor ast examples/errors/syntax.bminor > /dev/null
examples/errors/syntax.bminor:1:14: error: expected expression, got ';'
examples/errors/syntax.bminor:5:15: error: expected ')' after condition, got '{'
examples/errors/syntax.bminor:8:7: error: invalid assignment target
examples/errors/syntax.bminor:10:5: error: expected ';' after print arguments, got 'return'
```

The exit code is `0` for a clean file, `1` when the source has errors, and `2` for usage or I/O errors.

### Docker

```sh
docker build -t bminor-tools .
docker run --rm -v "$PWD/examples:/work" bminor-tools highlight showcase.bminor
```

## Testing

```sh
ctest --test-dir build --output-on-failure
```

To run the tests under AddressSanitizer and UndefinedBehaviorSanitizer, configure with `-DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"`.

## Documentation

- [Lexical specification](docs/lexical.md): token categories, regular definitions, longest match and keyword priority, lexical errors
- [Grammar](docs/grammar.md): the full EBNF, operator precedence, left-recursion removal and the LL(1) analysis
- [Design](docs/design.md): architecture, AST and visitor design, error recovery, highlighting and testing

## Project layout

```
src/         lexer, parser, AST, highlighter, renderers, and the CLI
tests/       GoogleTest suites for every stage
examples/    sample B-Minor programs; errors/ holds intentionally broken ones
docs/        language and design documentation
```
