/*
 * lexer.h
 * Contains function prototype declarations of functions in lexer.c
 * Compiler Project - CS F363
 */

#ifndef LEXER_H
#define LEXER_H

#include "lexerDef.h"

// ============================================================================
// Function Prototypes
// ============================================================================

/**
 * getStream - Initializes the twin buffer with source code from file
 *
 * This function takes the input from the file pointed to by 'fp'.
 * It uses twin buffer technique to bring fixed sized pieces of source code
 * into memory for processing to avoid intensive I/O operations.
 *
 * @param fp: File pointer to source code file
 * @return: FILE pointer for continued reading, or NULL on error
 */
FILE *getStream(FILE *fp);

/**
 * getNextToken - Reads input and returns next token
 *
 * This function reads the input character stream and uses DFA-based
 * mechanism to recognize lexemes. Returns all relevant information
 * encapsulated as tokenInfo. Displays lexical errors appropriately.
 *
 * @param B: Pointer to the twin buffer structure
 * @return: tokenInfo structure containing token type, lexeme, line number, and
 * value
 */
tokenInfo getNextToken(TwinBuffer *B);

/**
 * removeComments - Removes comments from source code
 *
 * This function takes source code as input and writes clean code
 * (without comments) to the output file. Comments start with '%'
 * and continue to end of line.
 *
 * @param testcaseFile: Path to input source code file
 * @param cleanFile: Path to output file for clean code
 */
void removeComments(char *testcaseFile, char *cleanFile);

// ============================================================================
// Helper Functions (used internally and by parser)
// ============================================================================

/**
 * initLexer - Initializes the lexer state
 *
 * Sets up the keyword lookup table and prepares the lexer for tokenization.
 * Must be called before using getNextToken.
 *
 * @return: 0 on success, -1 on failure
 */
int initLexer(void);

/**
 * cleanupLexer - Cleans up lexer resources
 *
 * Frees memory and closes files. Should be called when done with lexer.
 */
void cleanupLexer(void);

/**
 * getTokenTypeName - Returns string representation of token type
 *
 * @param type: TokenType enum value
 * @return: String name of the token type (e.g., "TK_ID", "TK_PLUS")
 */
const char *getTokenTypeName(TokenType type);

/**
 * initTwinBuffer - Initializes the twin buffer with a file
 *
 * @param B: Pointer to twin buffer structure
 * @param fp: File pointer to read from
 * @return: 0 on success, -1 on failure
 */
int initTwinBuffer(TwinBuffer *B, FILE *fp);

/**
 * printAllTokens - Prints all tokens from source file
 *
 * Tokenizes the entire source file and prints each token with
 * line number and lexeme for debugging/display purposes.
 *
 * @param sourceFile: Path to source code file
 */
void printAllTokens(char *sourceFile);

#endif // LEXER_H
