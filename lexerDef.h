/*
 * lexerDef.h
 * Contains all data definitions used in lexer.c
 * Compiler Project - CS F363
 */

#ifndef LEXER_DEF_H
#define LEXER_DEF_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ============================================================================
// Constants
// ============================================================================

#define BUFFER_SIZE 50          // Twin buffer size (50B each)
#define MAX_LEXEME_LEN 31       // Max lexeme length (FUNID max 30 + null)
#define MAX_ID_LEN 20           // Max identifier length
#define MAX_FUNID_LEN 30        // Max function identifier length
#define NUM_KEYWORDS 28         // Number of keywords in lookup table
#define HASH_TABLE_SIZE 53      // Prime number for hash table

// ============================================================================
// Token Types Enumeration
// ============================================================================

typedef enum {
    // Assignment and Comment
    TK_ASSIGNOP,        // <---
    TK_COMMENT,         // %...
    
    // Identifiers
    TK_FIELDID,         // [a-z][a-z]*
    TK_ID,              // [b-d][2-7][b-d]*[2-7]*
    TK_FUNID,           // _[a-zA-Z][a-zA-Z]*[0-9]*
    TK_RUID,            // #[a-z][a-z]*
    
    // Numeric Literals
    TK_NUM,             // [0-9]+
    TK_RNUM,            // [0-9]+.[0-9]{2} or with exponent
    
    // Keywords (alphabetically ordered for lookup)
    TK_AS,
    TK_CALL,
    TK_DEFINETYPE,
    TK_ELSE,
    TK_END,
    TK_ENDRECORD,
    TK_ENDIF,
    TK_ENDUNION,
    TK_ENDWHILE,
    TK_GLOBAL,
    TK_IF,
    TK_INPUT,
    TK_INT,
    TK_LIST,
    TK_MAIN,
    TK_OUTPUT,
    TK_PARAMETER,
    TK_PARAMETERS,
    TK_READ,
    TK_REAL,
    TK_RECORD,
    TK_RETURN,
    TK_THEN,
    TK_TYPE,
    TK_UNION,
    TK_WHILE,
    TK_WITH,
    TK_WRITE,
    
    // Operators
    TK_PLUS,            // +
    TK_MINUS,           // -
    TK_MUL,             // *
    TK_DIV,             // /
    
    // Logical Operators
    TK_AND,             // &&&
    TK_OR,              // @@@
    TK_NOT,             // ~
    
    // Relational Operators
    TK_LT,              // <
    TK_LE,              // <=
    TK_EQ,              // ==
    TK_GT,              // >
    TK_GE,              // >=
    TK_NE,              // !=
    
    // Delimiters
    TK_COMMA,           // ,
    TK_SEM,             // ;
    TK_COLON,           // :
    TK_DOT,             // .
    TK_OP,              // (
    TK_CL,              // )
    TK_SQL,             // [
    TK_SQR,             // ]
    
    // Special Tokens
    TK_EOF,             // End of file
    TK_ERROR            // Lexical error
} TokenType;

// ============================================================================
// Token Information Structure
// ============================================================================

typedef struct {
    TokenType type;
    char lexeme[MAX_LEXEME_LEN + 1];    // Lexeme string (+1 for null terminator)
    int lineNo;                          // Line number where token was found
    union {
        int intValue;                    // For TK_NUM
        double realValue;                // For TK_RNUM
    } value;
} tokenInfo;

// ============================================================================
// Twin Buffer Structure for Efficient I/O
// ============================================================================

typedef struct {
    char buffer[2][BUFFER_SIZE + 1];    // Two buffers with sentinel space
    int currentBuffer;                   // Current active buffer (0 or 1)
    int forward;                         // Forward pointer position
    int lexemeBegin;                     // Start of current lexeme
    int lexemeBeginBuffer;               // Buffer containing lexeme begin
    int lineNo;                          // Current line number
    int eof;                             // EOF reached flag
    int eofBuffer;                       // Which buffer has EOF (-1 if none)
    int eofPos;                          // Position of EOF in buffer
    FILE *fp;                            // File pointer
    int bytesRead[2];                    // Bytes read in each buffer
} TwinBuffer;

// ============================================================================
// Keyword Lookup Table Entry
// ============================================================================

typedef struct KeywordEntry {
    char *keyword;
    TokenType token;
    struct KeywordEntry *next;          // For hash collision chaining
} KeywordEntry;

// ============================================================================
// Keyword Hash Table
// ============================================================================

typedef struct {
    KeywordEntry *table[HASH_TABLE_SIZE];
} KeywordTable;

// ============================================================================
// Global Lexer State
// ============================================================================

typedef struct {
    TwinBuffer twinBuffer;
    KeywordTable keywordTable;
    int initialized;
} LexerState;

#endif // LEXER_DEF_H
