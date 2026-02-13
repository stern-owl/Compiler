# Compiler Project

A complete compiler front-end implementation featuring a lexical analyzer and predictive parser for a custom programming language.

## Features

- **Lexical Analyzer**: DFA-based tokenizer with twin buffer I/O
- **Syntax Analyzer**: LL(1) predictive parser with automated FIRST/FOLLOW computation
- **Parse Tree**: Inorder traversal output with 7-column format
- **Error Handling**: Reports lexical and syntax errors with line numbers

## Project Structure

```
├── driver.c        # Main driver with menu interface
├── lexer.c         # Lexical analyzer implementation
├── lexer.h         # Lexer function prototypes
├── lexerDef.h      # Token definitions and data structures
├── parser.c        # Syntax analyzer implementation
├── parser.h        # Parser function prototypes
├── parserDef.h     # Parser data structures
├── grammar.txt     # LL(1) grammar rules (92 rules, 50 non-terminals)
├── makefile        # Build configuration
└── testcases/      # Sample test programs
```

## Building

```bash
make
```

## Usage

```bash
./stage1exe <testcase.txt> <parsetreeOutFile.txt>
```

### Menu Options

| Option | Description |
|--------|-------------|
| 0 | Exit |
| 1 | Remove comments and print clean code |
| 2 | Print token list (lexer only) |
| 3 | Parse and print parse tree to file |
| 4 | Print time taken by lexer + parser |

## Example

```bash
./stage1exe testcases/testcase1.txt output.txt
```

## Implementation Details

### Lexer
- Twin buffer mechanism (4KB each) for efficient I/O
- 57 token types including keywords, operators, identifiers
- Hash table for O(1) keyword lookup
- Handles comments starting with `%`

### Parser
- FIRST and FOLLOW sets computed automatically
- LL(1) parse table construction with conflict detection
- Predictive parsing with parse tree construction
- Parse tree printed using inorder traversal (leftmost child → parent → remaining siblings)

### Parse Tree Output Format

```
Lexeme    CurrentNode    Line   TokenName   Value   ParentNode   Leaf   NodeSymbol
```

## Test Cases

| File | Description |
|------|-------------|
| testcase1.txt | Function with loop and I/O |
| testcase2.txt | Records and multiple functions |
| testcase3.txt | Record swapping |
| testcase4.txt | Intentional errors for testing |

## Requirements

- GCC compiler
- GNU Make

## Course

CS F363 - Compiler Construction  
BITS Pilani

## License

Academic use only.
