/*
 * parser.h
 * Contains function prototype declarations of functions in parser.c
 * Compiler Project - CS F363
 */

#ifndef PARSER_H
#define PARSER_H

#include "parserDef.h"

// ============================================================================
// Main Parser Functions
// ============================================================================

/**
 * loadGrammar - Loads grammar from file
 *
 * Reads the LL(1) grammar from a text file and populates the
 * Grammar structure.
 *
 * @param grammarFile: Path to grammar file
 * @return: Pointer to populated Grammar structure, NULL on error
 */
Grammar *loadGrammar(char *grammarFile);

/**
 * ComputeFirstAndFollowSets - Computes FIRST and FOLLOW sets
 *
 * Takes the grammar G and computes FIRST and FOLLOW information
 * using automated algorithms. Populates the FirstAndFollow structure.
 *
 * @param G: Pointer to Grammar structure
 * @return: FirstAndFollow structure with computed sets
 */
FirstAndFollow ComputeFirstAndFollowSets(Grammar *G);

/**
 * createParseTable - Creates predictive parse table
 *
 * Takes FIRST and FOLLOW information to populate the parse table T.
 * Uses standard LL(1) table construction algorithm.
 *
 * @param F: FirstAndFollow structure
 * @param T: Parse table to populate
 * @param G: Pointer to Grammar structure
 */
void createParseTable(FirstAndFollow F, ParseTable T, Grammar *G);

/**
 * parseInputSourceCode - Parses source code and builds parse tree
 *
 * Takes source code file and parses using predictive parse table.
 * Gets tokens from lexer and builds parse tree top-down.
 * Reports syntax errors with line numbers.
 * Displays success message if code is syntactically correct.
 *
 * @param testcaseFile: Path to source code file
 * @param T: Parse table
 * @param G: Pointer to Grammar structure
 * @return: Pointer to parse tree, NULL on error
 */
parseTree parseInputSourceCode(char *testcaseFile, ParseTable T, Grammar *G);

/**
 * printParseTree - Prints parse tree in inorder
 *
 * Prints the parse tree in inorder traversal to output file.
 * Format: lexeme currentNode lineno tokenName valueIfNumber
 *         parentNodeSymbol isLeafNode NodeSymbol
 *
 * @param PT: Parse tree to print
 * @param outfile: Path to output file
 */
void printParseTree(parseTree PT, char *outfile);

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * initParser - Initializes the parser
 *
 * Loads grammar, computes FIRST/FOLLOW sets, and builds parse table.
 *
 * @param grammarFile: Path to grammar file
 * @return: 0 on success, -1 on failure
 */
int initParser(char *grammarFile);

/**
 * cleanupParser - Cleans up parser resources
 *
 * Frees memory used by grammar, parse table, and parse tree.
 */
void cleanupParser(void);

/**
 * printFirstSets - Prints FIRST sets for debugging
 *
 * @param F: FirstAndFollow structure
 * @param G: Grammar structure
 */
void printFirstSets(FirstAndFollow *F, Grammar *G);

/**
 * printFollowSets - Prints FOLLOW sets for debugging
 *
 * @param F: FirstAndFollow structure
 * @param G: Grammar structure
 */
void printFollowSets(FirstAndFollow *F, Grammar *G);

/**
 * printParseTable - Prints parse table for debugging
 *
 * @param T: Parse table
 * @param G: Grammar structure
 */
void printParseTableDebug(ParseTable T, Grammar *G);

/**
 * getNonTerminalName - Returns name of non-terminal
 *
 * @param G: Grammar structure
 * @param index: Non-terminal index
 * @return: String name of non-terminal
 */
const char *getNonTerminalName(Grammar *G, int index);

/**
 * getNonTerminalIndex - Returns index of non-terminal by name
 *
 * @param G: Grammar structure
 * @param name: Non-terminal name
 * @return: Index of non-terminal, -1 if not found
 */
int getNonTerminalIndex(Grammar *G, const char *name);

/**
 * freeParseTree - Frees memory used by parse tree
 *
 * @param PT: Parse tree to free
 */
void freeParseTree(parseTree PT);

// ============================================================================
// Stack Operations for Predictive Parser
// ============================================================================

/**
 * initStack - Initializes the parse stack
 *
 * @param stack: Pointer to ParseStack
 */
void initStack(ParseStack *stack);

/**
 * push - Pushes a node onto the stack
 *
 * @param stack: Pointer to ParseStack
 * @param node: Node to push
 * @return: 0 on success, -1 on stack overflow
 */
int push(ParseStack *stack, ParseTreeNode *node);

/**
 * pop - Pops a node from the stack
 *
 * @param stack: Pointer to ParseStack
 * @return: Popped node, NULL if stack empty
 */
ParseTreeNode *pop(ParseStack *stack);

/**
 * top - Returns top of stack without popping
 *
 * @param stack: Pointer to ParseStack
 * @return: Top node, NULL if stack empty
 */
ParseTreeNode *top(ParseStack *stack);

/**
 * isEmpty - Checks if stack is empty
 *
 * @param stack: Pointer to ParseStack
 * @return: 1 if empty, 0 otherwise
 */
int isEmpty(ParseStack *stack);

#endif // PARSER_H
