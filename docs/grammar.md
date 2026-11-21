# B-Minor Grammar

This is the grammar the parser in [`src/parser.cpp`](../src/parser.cpp) implements. It is written in EBNF:

- `::=` defines a rule.
- `|` separates alternatives.
- `?` means optional, `*` means zero or more.
- Parentheses group.
- Quoted text is a terminal.

`IDENT`, `INT`, `CHAR` and `STRING` are the token classes from the [lexical specification](lexical.md).

## Grammar

```ebnf
program         ::= declaration* EOF

declaration     ::= IDENT ':' ( function_rest | variable_rest )
function_rest   ::= 'function' type '(' param_list? ')' ( '=' block | ';' )
variable_rest   ::= type ( '=' initializer )? ';'
param_list      ::= param ( ',' param )*
param           ::= IDENT ':' type

type            ::= 'integer' | 'boolean' | 'char' | 'string' | 'void' | 'auto'
                  | 'array' '[' INT? ']' type
initializer     ::= expression
                  | '{' initializer ( ',' initializer )* '}'

block           ::= '{' statement* '}'
statement       ::= declaration
                  | block
                  | if_stmt
                  | for_stmt
                  | while_stmt
                  | return_stmt
                  | print_stmt
                  | expr_stmt
if_stmt         ::= 'if' '(' expression ')' statement ( 'else' statement )?
for_stmt        ::= 'for' '(' expression? ';' expression? ';' expression? ')' statement
while_stmt      ::= 'while' '(' expression ')' statement
return_stmt     ::= 'return' expression? ';'
print_stmt      ::= 'print' expression_list? ';'
expr_stmt       ::= expression ';'

expression_list ::= expression ( ',' expression )*
expression      ::= assignment
assignment      ::= logical_or ( '=' assignment )?
logical_or      ::= logical_and ( '||' logical_and )*
logical_and     ::= comparison ( '&&' comparison )*
comparison      ::= additive ( ( '<' | '<=' | '>' | '>=' | '==' | '!=' ) additive )*
additive        ::= multiplicative ( ( '+' | '-' ) multiplicative )*
multiplicative  ::= power ( ( '*' | '/' | '%' ) power )*
power           ::= unary ( '^' power )?
unary           ::= ( '-' | '!' ) unary
                  | postfix
postfix         ::= primary ( '[' expression ']' | '(' expression_list? ')' | '++' | '--' )*
primary         ::= INT | CHAR | STRING | 'true' | 'false' | IDENT
                  | '(' expression ')'
```

A few rules accept more than the language allows. Those restrictions are left to later checks so the grammar stays simple:

- The parser checks that the left side of `=` is a name or an indexing expression, and reports `invalid assignment target` otherwise.
- `void` as a variable type, function declarations inside blocks, and calls on something that is not a function are syntactically valid. They are semantic errors for phase 2.

## Operator precedence

From lowest to highest:

| Level | Operators | Associativity | Rule |
|---|---|---|---|
| 1 | `=` | right | `assignment` |
| 2 | `\|\|` | left | `logical_or` |
| 3 | `&&` | left | `logical_and` |
| 4 | `< <= > >= == !=` | left | `comparison` |
| 5 | `+ -` | left | `additive` |
| 6 | `* / %` | left | `multiplicative` |
| 7 | `^` | right | `power` |
| 8 | unary `- !` | right (prefix) | `unary` |
| 9 | `[]`, call `()`, postfix `++ --` | left (postfix) | `postfix` |

The order follows the B-Minor reference. Unary minus binds tighter than `^`, so `-g ^ 2` parses as `(-g) ^ 2`.

## Parsing strategy

The parser is a hand-written recursive-descent (LL) parser with one function per non-terminal. `additive` in the grammar is `Parser::additive()` in the code, `if_stmt` is `Parser::ifStatement()`, and so on. Recursive descent was chosen because the code mirrors the grammar one-to-one, gives precise control over error messages and recovery, and needs no generated tables.

### Removing left recursion

The natural way to write a left-associative operator is left recursive:

```ebnf
additive ::= additive ( '+' | '-' ) multiplicative
           | multiplicative
```

A recursive-descent function for this rule would call itself before consuming any input and never terminate. The standard transformation `A ::= A α | β` into `A ::= β A'` and `A' ::= α A' | ε` gives:

```ebnf
additive  ::= multiplicative additive'
additive' ::= ( '+' | '-' ) multiplicative additive'
            | ε
```

In EBNF the tail recursion becomes a repetition:

```ebnf
additive ::= multiplicative ( ( '+' | '-' ) multiplicative )*
```

`logical_or`, `logical_and`, `comparison`, `additive` and `multiplicative` are all written this way, and so is `postfix`, whose operators are applied repeatedly to the left operand. Every repetition is implemented by `Parser::leftAssociative`, which loops and folds each new operand into the tree. `a - b - c` therefore produces `(a - b) - c`, so left associativity is kept even though the grammar is no longer left recursive.

The right-associative rules (`assignment`, `power`, `unary`) use right recursion, which is not a problem for LL parsing. `a = b = c` parses as `a = (b = c)` and `2 ^ 3 ^ 2` as `2 ^ (3 ^ 2)`.

The grammar has no left recursion, direct or indirect. Every rule either consumes a terminal before recursing or recurses only into a rule at a higher precedence level.

### FIRST sets

```
FIRST(expression)  = FIRST(assignment) = ... = FIRST(unary)
                   = { '-', '!' } ∪ FIRST(primary)
FIRST(primary)     = { INT, CHAR, STRING, 'true', 'false', IDENT, '(' }
FIRST(type)        = { 'integer', 'boolean', 'char', 'string', 'void', 'auto', 'array' }
FIRST(initializer) = { '{' } ∪ FIRST(expression)
```

| `statement` alternative | FIRST |
|---|---|
| `block` | `{` |
| `if_stmt` | `if` |
| `for_stmt` | `for` |
| `while_stmt` | `while` |
| `return_stmt` | `return` |
| `print_stmt` | `print` |
| `declaration` | `IDENT` |
| `expr_stmt` | FIRST(expression), which includes `IDENT` |

### LL(1) check

At every choice point the next token selects a single alternative, with the three exceptions below.

- **`declaration` vs `expr_stmt`.** Both can start with `IDENT`. One extra token of lookahead settles it: a declaration is always `IDENT ':'`, and no expression can contain `:`. The parser peeks at the second token (`atDeclarationStart`), so this one decision is LL(2). The project brief allows LL(k), and every other decision is LL(1).
- **The `else` branch (dangling else).** In `if (a) if (b) x; else y;` the `else` could belong to either `if`. Formally `'else'` is in both FIRST(`'else' statement`) and FOLLOW(`if_stmt`). The parser resolves it the usual way: an `if` takes an `else` whenever one follows, so `else` binds to the nearest `if`.
- **`'{'` after `'='` in a declaration.** A function body and an array initializer both start with `{`. The conflict never reaches the parser: the grammar splits declarations into `function_rest` and `variable_rest` immediately after `':'`. `'function'` is not in FIRST(`type`), so a single token picks the branch, and each branch has only one meaning for `{`.

The remaining optional and repeated parts are LL(1) because the token that would continue them is never in the FOLLOW set of the construct:

| Construct | Continues on | Stops on (FOLLOW) |
|---|---|---|
| `( '=' assignment )?` | `=` | `) ] ; , }` |
| `( '^' power )?` | `^` | lower-precedence operators, `) ] ; , }` |
| `postfix` repetition | `[ ( ++ --` | any binary operator, `) ] ; , }` |
| binary operator repetitions | that level's operators | operators of lower levels, `) ] ; , }` |
| `param_list?` | `IDENT` | `)` |
| `expression?` in `for`/`return` | FIRST(expression) | `;` or `)` |
| `INT?` in array types | `INT` | `]` |
| `( '=' initializer )?` | `=` | `;` |
| `( '=' block \| ';' )` | `=` | `;` |

## Examples

```
factorial: function integer ( n: integer ) = {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

`bminor ast` prints this tree for the last statement:

```
ReturnStmt(loc=3:5)
  value: BinaryExpr(op='*', loc=3:12)
    left: Identifier(name='n', loc=3:12)
    right: CallExpr(loc=3:16)
      callee: Identifier(name='factorial', loc=3:16)
      args[0]: BinaryExpr(op='-', loc=3:26)
        left: Identifier(name='n', loc=3:26)
        right: IntegerLiteral(value=1, loc=3:30)
```

[`examples/showcase.bminor`](../examples/showcase.bminor) uses every construct in the grammar.
