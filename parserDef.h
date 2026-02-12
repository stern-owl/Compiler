/*
 * parserDef.h
 * Contains all data definitions for grammar, table, parseTree etc.
 * Compiler Project - CS F363
 */

#ifndef PARSER_DEF_H
#define PARSER_DEF_H

#include "lexerDef.h"

// ============================================================================
// Constants
// ============================================================================

#define MAX_NON_TERMINALS 100 // Maximum number of non-terminals
#define MAX_TERMINALS 100     // Maximum number of terminals (token types)
#define MAX_RULES 100        // Maximum number of grammar rules
#define MAX_RHS_LENGTH 15    // Maximum symbols on RHS of a rule
#define MAX_SYMBOL_NAME 50   // Maximum length of symbol name
#define EPSILON -1           // Special value for epsilon
#define SYNC -2              // Special value for sync entry in parse table
#define ERROR_ENTRY -3       // Error entry in parse table

// ============================================================================
// Symbol Structure
// ============================================================================

typedef struct {
  int isTerminal; // 1 if terminal, 0 if non-terminal
  union {
    TokenType terminal; // Terminal token type
    int nonTerminal;    // Non-terminal index
  } symbol;
} Symbol;

// ============================================================================
// Grammar Rule Structure
// ============================================================================

typedef struct {
  int lhs;                    // Non-terminal index (left-hand side)
  Symbol rhs[MAX_RHS_LENGTH]; // Right-hand side symbols
  int rhsLength;              // Number of symbols in RHS
  int ruleNumber;             // Rule number for identification
} GrammarRule;

// ============================================================================
// Grammar Structure
// ============================================================================

typedef struct {
  GrammarRule rules[MAX_RULES];
  int numRules;
  char nonTerminalNames[MAX_NON_TERMINALS][MAX_SYMBOL_NAME];
  int numNonTerminals;
  int startSymbol; // Index of start symbol
} Grammar;

// ============================================================================
// First and Follow Sets
// ============================================================================

// Using unsigned long long for bit sets (supports up to 64 terminals)
typedef unsigned long long BitSet;

typedef struct {
  BitSet first[MAX_NON_TERMINALS];   // FIRST set for each non-terminal
  BitSet follow[MAX_NON_TERMINALS];  // FOLLOW set for each non-terminal
  int hasEpsilon[MAX_NON_TERMINALS]; // 1 if FIRST contains epsilon
  int numNonTerminals;
  int numTerminals;
} FirstAndFollow;

// ============================================================================
// Parse Table
// ============================================================================

// Parse table: [non-terminal index][terminal token type] = rule number
// Value of -1 indicates error, -2 indicates sync
typedef int ParseTable[MAX_NON_TERMINALS][MAX_TERMINALS];

// ============================================================================
// Parse Tree Node Structure
// ============================================================================

typedef struct ParseTreeNode {
  // Symbol information
  Symbol symbol;

  // Token information (for leaf nodes - terminals)
  tokenInfo *token;

  // Tree structure pointers
  struct ParseTreeNode *parent;
  struct ParseTreeNode *firstChild;
  struct ParseTreeNode *nextSibling;

  // Additional information
  int lineNo;     // Line number
  int ruleNumber; // Rule used to expand (for non-terminals)
  int isLeaf;     // 1 if leaf node, 0 otherwise

  // For printing purposes
  char symbolName[MAX_SYMBOL_NAME]; // Human-readable symbol name
} ParseTreeNode;

// ============================================================================
// Parse Tree Structure
// ============================================================================

typedef struct {
  ParseTreeNode *root;
  int nodeCount;
} ParseTree;

typedef ParseTree *parseTree;

// ============================================================================
// Stack for Predictive Parsing
// ============================================================================

#define MAX_STACK_SIZE 500

typedef struct {
  ParseTreeNode *items[MAX_STACK_SIZE];
  int top;
} ParseStack;

// ============================================================================
// Error Information Structure
// ============================================================================

typedef struct {
  int lineNo;
  char message[256];
  TokenType expected;
  TokenType found;
} SyntaxError;

typedef struct {
  SyntaxError errors[100];
  int count;
} ErrorList;

// ============================================================================
// Parser State
// ============================================================================

typedef struct {
  Grammar grammar;
  FirstAndFollow firstFollow;
  ParseTable parseTable;
  ParseTree *parseTree;
  ErrorList errors;
  int initialized;
} ParserState;

#endif // PARSER_DEF_H
