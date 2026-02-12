/*
 * lexer.c
 * Lexical Analyzer Implementation
 * Compiler Project - CS F363
 */

#include "lexer.h"
#include <stdbool.h>

// ============================================================================
// Global Lexer State
// ============================================================================

static LexerState lexerState;
static int lexerInitialized = 0;

// ============================================================================
// Token Type Names (for printing)
// ============================================================================

static const char *tokenTypeNames[] = {
    "TK_ASSIGNOP",  "TK_COMMENT",    "TK_FIELDID",    "TK_ID",
    "TK_FUNID",     "TK_RUID",       "TK_NUM",        "TK_RNUM",
    "TK_AS",        "TK_CALL",       "TK_DEFINETYPE", "TK_ELSE",
    "TK_END",       "TK_ENDRECORD",  "TK_ENDIF",      "TK_ENDUNION",
    "TK_ENDWHILE",  "TK_GLOBAL",     "TK_IF",         "TK_INPUT",
    "TK_INT",       "TK_LIST",       "TK_MAIN",       "TK_OUTPUT",
    "TK_PARAMETER", "TK_PARAMETERS", "TK_READ",       "TK_REAL",
    "TK_RECORD",    "TK_RETURN",     "TK_THEN",       "TK_TYPE",
    "TK_UNION",     "TK_WHILE",      "TK_WITH",       "TK_WRITE",
    "TK_PLUS",      "TK_MINUS",      "TK_MUL",        "TK_DIV",
    "TK_AND",       "TK_OR",         "TK_NOT",        "TK_LT",
    "TK_LE",        "TK_EQ",         "TK_GT",         "TK_GE",
    "TK_NE",        "TK_COMMA",      "TK_SEM",        "TK_COLON",
    "TK_DOT",       "TK_OP",         "TK_CL",         "TK_SQL",
    "TK_SQR",       "TK_EOF",        "TK_ERROR"};

// ============================================================================
// Keywords Table
// ============================================================================

typedef struct {
  const char *keyword;
  TokenType token;
} KeywordPair;

static KeywordPair keywords[] = {{"as", TK_AS},
                                 {"call", TK_CALL},
                                 {"definetype", TK_DEFINETYPE},
                                 {"else", TK_ELSE},
                                 {"end", TK_END},
                                 {"endrecord", TK_ENDRECORD},
                                 {"endif", TK_ENDIF},
                                 {"endunion", TK_ENDUNION},
                                 {"endwhile", TK_ENDWHILE},
                                 {"global", TK_GLOBAL},
                                 {"if", TK_IF},
                                 {"input", TK_INPUT},
                                 {"int", TK_INT},
                                 {"list", TK_LIST},
                                 {"_main", TK_MAIN},
                                 {"output", TK_OUTPUT},
                                 {"parameter", TK_PARAMETER},
                                 {"parameters", TK_PARAMETERS},
                                 {"read", TK_READ},
                                 {"real", TK_REAL},
                                 {"record", TK_RECORD},
                                 {"return", TK_RETURN},
                                 {"then", TK_THEN},
                                 {"type", TK_TYPE},
                                 {"union", TK_UNION},
                                 {"while", TK_WHILE},
                                 {"with", TK_WITH},
                                 {"write", TK_WRITE}};

static int numKeywords = sizeof(keywords) / sizeof(keywords[0]);

// ============================================================================
// Hash Function for Keyword Lookup
// ============================================================================

static unsigned int hashKeyword(const char *str) {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % HASH_TABLE_SIZE;
}

// ============================================================================
// Initialize Keyword Table
// ============================================================================

static void initKeywordTable(void) {
  // Clear the table
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    lexerState.keywordTable.table[i] = NULL;
  }

  // Insert all keywords
  for (int i = 0; i < numKeywords; i++) {
    unsigned int index = hashKeyword(keywords[i].keyword);

    KeywordEntry *entry = (KeywordEntry *)malloc(sizeof(KeywordEntry));
    if (entry == NULL) {
      fprintf(stderr, "Memory allocation failed for keyword entry\n");
      return;
    }

    entry->keyword = (char *)keywords[i].keyword;
    entry->token = keywords[i].token;
    entry->next = lexerState.keywordTable.table[index];
    lexerState.keywordTable.table[index] = entry;
  }
}

// ============================================================================
// Lookup Keyword
// ============================================================================

static TokenType lookupKeyword(const char *lexeme) {
  unsigned int index = hashKeyword(lexeme);
  KeywordEntry *entry = lexerState.keywordTable.table[index];

  while (entry != NULL) {
    if (strcmp(entry->keyword, lexeme) == 0) {
      return entry->token;
    }
    entry = entry->next;
  }

  return TK_ERROR; // Not a keyword
}

// ============================================================================
// Initialize Twin Buffer
// ============================================================================

int initTwinBuffer(TwinBuffer *B, FILE *fp) {
  if (fp == NULL) {
    return -1;
  }

  B->fp = fp;
  B->currentBuffer = 0;
  B->forward = 0;
  B->lexemeBegin = 0;
  B->lexemeBeginBuffer = 0;
  B->lineNo = 1;
  B->eof = 0;
  B->eofBuffer = -1;
  B->eofPos = -1;

  // Read first buffer
  B->bytesRead[0] = fread(B->buffer[0], 1, BUFFER_SIZE, fp);
  B->buffer[0][B->bytesRead[0]] = '\0'; // Sentinel

  // Read second buffer
  B->bytesRead[1] = fread(B->buffer[1], 1, BUFFER_SIZE, fp);
  B->buffer[1][B->bytesRead[1]] = '\0'; // Sentinel

  // Check for EOF
  if (B->bytesRead[0] < BUFFER_SIZE) {
    B->eofBuffer = 0;
    B->eofPos = B->bytesRead[0];
  } else if (B->bytesRead[1] < BUFFER_SIZE) {
    B->eofBuffer = 1;
    B->eofPos = B->bytesRead[1];
  }

  return 0;
}

// ============================================================================
// Get Next Character from Buffer
// ============================================================================

static char getNextChar(TwinBuffer *B) {
  if (B->eof) {
    return '\0';
  }

  char c = B->buffer[B->currentBuffer][B->forward];

  // Check for end of current buffer (sentinel)
  if (c == '\0') {
    // Check if this is actual EOF
    if (B->currentBuffer == B->eofBuffer && B->forward >= B->eofPos) {
      B->eof = 1;
      return '\0';
    }

    // Switch to other buffer
    int otherBuffer = 1 - B->currentBuffer;

    // Reload the current buffer for next time
    if (B->eofBuffer == -1) {
      B->bytesRead[B->currentBuffer] =
          fread(B->buffer[B->currentBuffer], 1, BUFFER_SIZE, B->fp);
      B->buffer[B->currentBuffer][B->bytesRead[B->currentBuffer]] = '\0';

      if (B->bytesRead[B->currentBuffer] < BUFFER_SIZE) {
        B->eofBuffer = B->currentBuffer;
        B->eofPos = B->bytesRead[B->currentBuffer];
      }
    }

    B->currentBuffer = otherBuffer;
    B->forward = 0;

    c = B->buffer[B->currentBuffer][B->forward];

    if (c == '\0') {
      B->eof = 1;
      return '\0';
    }
  }

  // Track line numbers
  if (c == '\n') {
    B->lineNo++;
  }

  B->forward++;
  return c;
}

// ============================================================================
// Retract Character (move back one position)
// ============================================================================

static void retract(TwinBuffer *B, int count) {
  for (int i = 0; i < count; i++) {
    if (B->forward > 0) {
      B->forward--;
      // Check if we need to adjust line number
      if (B->buffer[B->currentBuffer][B->forward] == '\n') {
        B->lineNo--;
      }
    } else if (B->currentBuffer != B->lexemeBeginBuffer) {
      // Switch to other buffer
      B->currentBuffer = 1 - B->currentBuffer;
      B->forward = B->bytesRead[B->currentBuffer] - 1;
      if (B->buffer[B->currentBuffer][B->forward] == '\n') {
        B->lineNo--;
      }
    }
  }
}

// ============================================================================
// Get Current Lexeme
// ============================================================================

static void getLexeme(TwinBuffer *B, char *lexeme, int maxLen) {
  int len = 0;
  int buf = B->lexemeBeginBuffer;
  int pos = B->lexemeBegin;

  while ((buf != B->currentBuffer || pos != B->forward) && len < maxLen - 1) {
    char c = B->buffer[buf][pos];
    if (c == '\0') {
      buf = 1 - buf;
      pos = 0;
      continue;
    }
    lexeme[len++] = c;
    pos++;
  }

  lexeme[len] = '\0';
}

// ============================================================================
// Reset Lexeme Begin Pointer
// ============================================================================

static void resetLexemeBegin(TwinBuffer *B) {
  B->lexemeBegin = B->forward;
  B->lexemeBeginBuffer = B->currentBuffer;
}

// ============================================================================
// Check Character Categories
// ============================================================================

static inline int isLetter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int isLowercaseLetter(char c) { return c >= 'a' && c <= 'z'; }

static inline int isDigit(char c) { return c >= '0' && c <= '9'; }

static inline int isIdStartLetter(char c) { return c >= 'b' && c <= 'd'; }

static inline int isIdDigit(char c) { return c >= '2' && c <= '7'; }

static inline int isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ============================================================================
// DFA-based getNextToken Implementation
// ============================================================================

tokenInfo getNextToken(TwinBuffer *B) {
  tokenInfo token;
  token.type = TK_ERROR;
  token.lexeme[0] = '\0';
  token.lineNo = B->lineNo;
  token.value.intValue = 0;

  char c;
  int state = 0;
  int lexemeLen = 0;
  char lexeme[MAX_LEXEME_LEN + 1];

  // Skip whitespace
  do {
    c = getNextChar(B);
  } while (isWhitespace(c));

  if (c == '\0') {
    token.type = TK_EOF;
    strcpy(token.lexeme, "$");
    return token;
  }

  // Record start of lexeme
  retract(B, 1);
  resetLexemeBegin(B);
  c = getNextChar(B);

  token.lineNo = B->lineNo;
  if (c == '\n')
    token.lineNo--;

  // Start DFA
  state = 0;
  lexeme[0] = c;
  lexemeLen = 1;

  switch (c) {
  // ================================================================
  // Single Character Tokens
  // ================================================================
  case '+':
    token.type = TK_PLUS;
    strcpy(token.lexeme, "+");
    return token;

  case '-':
    token.type = TK_MINUS;
    strcpy(token.lexeme, "-");
    return token;

  case '*':
    token.type = TK_MUL;
    strcpy(token.lexeme, "*");
    return token;

  case '/':
    token.type = TK_DIV;
    strcpy(token.lexeme, "/");
    return token;

  case '(':
    token.type = TK_OP;
    strcpy(token.lexeme, "(");
    return token;

  case ')':
    token.type = TK_CL;
    strcpy(token.lexeme, ")");
    return token;

  case '[':
    token.type = TK_SQL;
    strcpy(token.lexeme, "[");
    return token;

  case ']':
    token.type = TK_SQR;
    strcpy(token.lexeme, "]");
    return token;

  case ',':
    token.type = TK_COMMA;
    strcpy(token.lexeme, ",");
    return token;

  case ';':
    token.type = TK_SEM;
    strcpy(token.lexeme, ";");
    return token;

  case ':':
    token.type = TK_COLON;
    strcpy(token.lexeme, ":");
    return token;

  case '.':
    token.type = TK_DOT;
    strcpy(token.lexeme, ".");
    return token;

  case '~':
    token.type = TK_NOT;
    strcpy(token.lexeme, "~");
    return token;

  // ================================================================
  // Comment (% until end of line)
  // ================================================================
  case '%':
    // Read until end of line or EOF
    while ((c = getNextChar(B)) != '\n' && c != '\0')
      ;
    
    if (c == '\n') {
      retract(B, 1); // Stay on the same line for the whitespace skip
    }
    
    getLexeme(B, token.lexeme, MAX_LEXEME_LEN);
    token.type = TK_COMMENT;
    return token;

  // ================================================================
  // Less Than Operators and Assignment
  // ================================================================
  case '<':
    c = getNextChar(B);
    if (c == '=') {
      token.type = TK_LE;
      strcpy(token.lexeme, "<=");
      return token;
    } else if (c == '-') {
      // Could be <--- (assignment)
      char c2 = getNextChar(B);
      if (c2 == '-') {
        char c3 = getNextChar(B);
        if (c3 == '-') {
          token.type = TK_ASSIGNOP;
          strcpy(token.lexeme, "<---");
          return token;
        } else {
          // <-- is error
          retract(B, 1);
          token.type = TK_ERROR;
          strcpy(token.lexeme, "<--");
          fprintf(stderr,
                  "Lexical Error at line %d: Invalid pattern '<--' (expected "
                  "'<---')\n",
                  token.lineNo);
          return token;
        }
      } else {
        // <- is error (per correction)
        retract(B, 1);
        token.type = TK_ERROR;
        strcpy(token.lexeme, "<-");
        fprintf(stderr, "Lexical Error at line %d: Invalid pattern '<-'\n",
                token.lineNo);
        return token;
      }
    } else {
      retract(B, 1);
      token.type = TK_LT;
      strcpy(token.lexeme, "<");
      return token;
    }

  // ================================================================
  // Greater Than Operators
  // ================================================================
  case '>':
    c = getNextChar(B);
    if (c == '=') {
      token.type = TK_GE;
      strcpy(token.lexeme, ">=");
      return token;
    } else {
      retract(B, 1);
      token.type = TK_GT;
      strcpy(token.lexeme, ">");
      return token;
    }

  // ================================================================
  // Equal Operator
  // ================================================================
  case '=':
    c = getNextChar(B);
    if (c == '=') {
      token.type = TK_EQ;
      strcpy(token.lexeme, "==");
      return token;
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "=");
      fprintf(stderr,
              "Lexical Error at line %d: Invalid pattern '=' (expected '==')\n",
              token.lineNo);
      return token;
    }

  // ================================================================
  // Not Equal Operator
  // ================================================================
  case '!':
    c = getNextChar(B);
    if (c == '=') {
      token.type = TK_NE;
      strcpy(token.lexeme, "!=");
      return token;
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "!");
      fprintf(stderr,
              "Lexical Error at line %d: Invalid pattern '!' (expected '!=')\n",
              token.lineNo);
      return token;
    }

  // ================================================================
  // Logical AND (&&&)
  // ================================================================
  case '&':
    c = getNextChar(B);
    if (c == '&') {
      c = getNextChar(B);
      if (c == '&') {
        token.type = TK_AND;
        strcpy(token.lexeme, "&&&");
        return token;
      } else {
        retract(B, 1);
        token.type = TK_ERROR;
        strcpy(token.lexeme, "&&");
        fprintf(
            stderr,
            "Lexical Error at line %d: Invalid pattern '&&' (expected '&&&')\n",
            token.lineNo);
        return token;
      }
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "&");
      fprintf(
          stderr,
          "Lexical Error at line %d: Invalid pattern '&' (expected '&&&')\n",
          token.lineNo);
      return token;
    }

  // ================================================================
  // Logical OR (@@@)
  // ================================================================
  case '@':
    c = getNextChar(B);
    if (c == '@') {
      c = getNextChar(B);
      if (c == '@') {
        token.type = TK_OR;
        strcpy(token.lexeme, "@@@");
        return token;
      } else {
        retract(B, 1);
        token.type = TK_ERROR;
        strcpy(token.lexeme, "@@");
        fprintf(
            stderr,
            "Lexical Error at line %d: Invalid pattern '@@' (expected '@@@')\n",
            token.lineNo);
        return token;
      }
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "@");
      fprintf(
          stderr,
          "Lexical Error at line %d: Invalid pattern '@' (expected '@@@')\n",
          token.lineNo);
      return token;
    }

  // ================================================================
  // Function Identifier (_[a-zA-Z][a-zA-Z]*[0-9]*)
  // ================================================================
  case '_':
    c = getNextChar(B);
    if (isLetter(c)) {
      lexeme[lexemeLen++] = c;
      // Read letters
      while ((c = getNextChar(B)) != '\0' && isLetter(c) &&
             lexemeLen < MAX_FUNID_LEN) {
        lexeme[lexemeLen++] = c;
      }
      // Read optional digits at the end
      while (c != '\0' && isDigit(c) && lexemeLen < MAX_FUNID_LEN) {
        lexeme[lexemeLen++] = c;
        c = getNextChar(B);
      }
      retract(B, 1);
      lexeme[lexemeLen] = '\0';

      // Check if it's _main keyword
      if (strcmp(lexeme, "_main") == 0) {
        token.type = TK_MAIN;
      } else {
        // Check length
        if (lexemeLen > MAX_FUNID_LEN) {
          token.type = TK_ERROR;
          fprintf(stderr,
                  "Lexical Error at line %d: Function identifier '%s' exceeds "
                  "maximum length of %d\n",
                  token.lineNo, lexeme, MAX_FUNID_LEN);
        } else {
          token.type = TK_FUNID;
        }
      }
      strcpy(token.lexeme, lexeme);
      return token;
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "_");
      fprintf(stderr,
              "Lexical Error at line %d: Invalid function identifier starting "
              "with '_'\n",
              token.lineNo);
      return token;
    }

  // ================================================================
  // Record/Union Identifier (#[a-z][a-z]*)
  // ================================================================
  case '#':
    c = getNextChar(B);
    if (isLowercaseLetter(c)) {
      lexeme[lexemeLen++] = c;
      while ((c = getNextChar(B)) != '\0' && isLowercaseLetter(c)) {
        if (lexemeLen < MAX_LEXEME_LEN) {
          lexeme[lexemeLen++] = c;
        }
      }
      retract(B, 1);
      lexeme[lexemeLen] = '\0';
      token.type = TK_RUID;
      strcpy(token.lexeme, lexeme);
      return token;
    } else {
      retract(B, 1);
      token.type = TK_ERROR;
      strcpy(token.lexeme, "#");
      fprintf(stderr,
              "Lexical Error at line %d: Invalid record/union identifier "
              "starting with '#'\n",
              token.lineNo);
      return token;
    }

  default:
    break;
  }

  // ================================================================
  // Numbers (TK_NUM or TK_RNUM)
  // [0-9]+ or [0-9]+.[0-9]{2} or [0-9]+.[0-9]{2}E[+-]?[0-9]{2}
  // ================================================================
  if (isDigit(c)) {
    // Read integer part
    while ((c = getNextChar(B)) != '\0' && isDigit(c)) {
      if (lexemeLen < MAX_LEXEME_LEN) {
        lexeme[lexemeLen++] = c;
      }
    }

    // Check for decimal point
    if (c == '.') {
      lexeme[lexemeLen++] = c;
      char c1 = getNextChar(B);

      if (isDigit(c1)) {
        lexeme[lexemeLen++] = c1;
        char c2 = getNextChar(B);

        if (isDigit(c2)) {
          lexeme[lexemeLen++] = c2;
          lexeme[lexemeLen] = '\0';

          // Check for exponent
          char c3 = getNextChar(B);
          if (c3 == 'E') {
            lexeme[lexemeLen++] = c3;
            char c4 = getNextChar(B);

            if (c4 == '+' || c4 == '-') {
              lexeme[lexemeLen++] = c4;
              c4 = getNextChar(B);
            }

            if (isDigit(c4)) {
              lexeme[lexemeLen++] = c4;
              char c5 = getNextChar(B);

              if (isDigit(c5)) {
                lexeme[lexemeLen++] = c5;
                lexeme[lexemeLen] = '\0';
                token.type = TK_RNUM;
                strcpy(token.lexeme, lexeme);
                token.value.realValue = atof(lexeme);
                return token;
              } else {
                // Error: exponent needs exactly 2 digits
                retract(B, 1);
                lexeme[lexemeLen] = '\0';
                token.type = TK_ERROR;
                strcpy(token.lexeme, lexeme);
                fprintf(stderr,
                        "Lexical Error at line %d: Invalid real number '%s' "
                        "(exponent needs exactly 2 digits)\n",
                        token.lineNo, lexeme);
                return token;
              }
            } else {
              // Error: exponent needs digits
              retract(B, 1);
              lexeme[lexemeLen] = '\0';
              token.type = TK_ERROR;
              strcpy(token.lexeme, lexeme);
              fprintf(stderr,
                      "Lexical Error at line %d: Invalid real number '%s' "
                      "(missing exponent digits)\n",
                      token.lineNo, lexeme);
              return token;
            }
          } else {
            retract(B, 1);
            token.type = TK_RNUM;
            strcpy(token.lexeme, lexeme);
            token.value.realValue = atof(lexeme);
            return token;
          }
        } else {
          // Error: need exactly 2 digits after decimal (correction: 23.4 is
          // error)
          retract(B, 1);
          lexeme[lexemeLen] = '\0';
          token.type = TK_ERROR;
          strcpy(token.lexeme, lexeme);
          fprintf(stderr,
                  "Lexical Error at line %d: Invalid real number '%s' (need "
                  "exactly 2 digits after decimal)\n",
                  token.lineNo, lexeme);
          return token;
        }
      } else {
        // Error: 23.abc case - report "23." as error (per correction)
        retract(B, 1);
        lexeme[lexemeLen] = '\0';
        token.type = TK_ERROR;
        strcpy(token.lexeme, lexeme);
        fprintf(stderr, "Lexical Error at line %d: Invalid real number '%s'\n",
                token.lineNo, lexeme);
        return token;
      }
    } else {
      retract(B, 1);
      lexeme[lexemeLen] = '\0';
      token.type = TK_NUM;
      strcpy(token.lexeme, lexeme);
      token.value.intValue = atoi(lexeme);
      return token;
    }
  }

  // ================================================================
  // Identifiers and Keywords
  // ID: [b-d][2-7][b-d]*[2-7]*
  // FIELDID: [a-z][a-z]*
  // ================================================================
  if (isLowercaseLetter(c)) {
    // Check if it could be TK_ID: starts with [b-d]
    if (isIdStartLetter(c)) {
      char second = getNextChar(B);

      if (isIdDigit(second)) {
        // This is TK_ID pattern
        lexeme[lexemeLen++] = second;

        // Read [b-d]*[2-7]* pattern
        while ((c = getNextChar(B)) != '\0') {
          if (isIdStartLetter(c) || isIdDigit(c)) {
            if (lexemeLen < MAX_ID_LEN) {
              lexeme[lexemeLen++] = c;
            }
          } else {
            break;
          }
        }
        retract(B, 1);
        lexeme[lexemeLen] = '\0';

        // Check length constraints (2 to 20)
        if (lexemeLen < 2 || lexemeLen > MAX_ID_LEN) {
          token.type = TK_ERROR;
          strcpy(token.lexeme, lexeme);
          fprintf(
              stderr,
              "Lexical Error at line %d: Identifier '%s' has invalid length\n",
              token.lineNo, lexeme);
          return token;
        }

        token.type = TK_ID;
        strcpy(token.lexeme, lexeme);
        return token;
      } else {
        // Not TK_ID, could be keyword or FIELDID
        retract(B, 1);
      }
    }

    // Read as potential keyword or FIELDID
    while ((c = getNextChar(B)) != '\0' && isLowercaseLetter(c)) {
      if (lexemeLen < MAX_LEXEME_LEN) {
        lexeme[lexemeLen++] = c;
      }
    }
    retract(B, 1);
    lexeme[lexemeLen] = '\0';

    // Check if it's a keyword
    TokenType kwType = lookupKeyword(lexeme);
    if (kwType != TK_ERROR) {
      token.type = kwType;
    } else {
      token.type = TK_FIELDID;
    }
    strcpy(token.lexeme, lexeme);
    return token;
  }

  // ================================================================
  // Unknown Character - Error
  // ================================================================
  lexeme[lexemeLen] = '\0';
  token.type = TK_ERROR;
  strcpy(token.lexeme, lexeme);
  fprintf(stderr, "Lexical Error at line %d: Unknown symbol '%s'\n",
          token.lineNo, lexeme);
  return token;
}

// ============================================================================
// Initialize Lexer
// ============================================================================

int initLexer(void) {
  if (lexerInitialized) {
    return 0;
  }

  initKeywordTable();
  lexerInitialized = 1;

  return 0;
}

// ============================================================================
// Cleanup Lexer
// ============================================================================

void cleanupLexer(void) {
  // Free keyword table entries
  for (int i = 0; i < HASH_TABLE_SIZE; i++) {
    KeywordEntry *entry = lexerState.keywordTable.table[i];
    while (entry != NULL) {
      KeywordEntry *next = entry->next;
      free(entry);
      entry = next;
    }
    lexerState.keywordTable.table[i] = NULL;
  }

  lexerInitialized = 0;
}

// ============================================================================
// Get Token Type Name
// ============================================================================

const char *getTokenTypeName(TokenType type) {
  if (type >= 0 && type <= TK_ERROR) {
    return tokenTypeNames[type];
  }
  return "UNKNOWN";
}

// ============================================================================
// Get Stream (Initialize buffer from file)
// ============================================================================

FILE *getStream(FILE *fp) {
  if (fp == NULL) {
    return NULL;
  }

  if (initLexer() != 0) {
    return NULL;
  }

  if (initTwinBuffer(&lexerState.twinBuffer, fp) != 0) {
    return NULL;
  }

  return fp;
}

// ============================================================================
// Remove Comments
// ============================================================================

void removeComments(char *testcaseFile, char *cleanFile) {
  FILE *input = fopen(testcaseFile, "r");
  FILE *output = fopen(cleanFile, "w");

  if (input == NULL) {
    fprintf(stderr, "Error: Cannot open input file '%s'\n", testcaseFile);
    return;
  }

  if (output == NULL) {
    fprintf(stderr, "Error: Cannot open output file '%s'\n", cleanFile);
    fclose(input);
    return;
  }

  int c;
  int inComment = 0;

  while ((c = fgetc(input)) != EOF) {
    if (c == '%') {
      inComment = 1;
      continue;
    }

    if (inComment) {
      if (c == '\n') {
        inComment = 0;
        fputc(c, output);
      }
      continue;
    }

    fputc(c, output);
  }

  fclose(input);
  fclose(output);

  printf("Comments removed. Clean code written to '%s'\n", cleanFile);
}

// ============================================================================
// Print All Tokens
// ============================================================================

void printAllTokens(char *sourceFile) {
  FILE *fp = fopen(sourceFile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: Cannot open source file '%s'\n", sourceFile);
    return;
  }

  TwinBuffer buffer;
  initLexer();
  initTwinBuffer(&buffer, fp);

  printf("\n%-5s %-20s %-20s\n", "Line", "Lexeme", "Token");
  printf("%-5s %-20s %-20s\n", "----", "------", "-----");

  tokenInfo token;
  do {
    token = getNextToken(&buffer);
    if (token.type != TK_ERROR || token.lexeme[0] != '\0') {
      const char *lexemeToPrint = (token.type == TK_COMMENT) ? "%" : token.lexeme;
      printf("%-5d %-20s %-20s", token.lineNo, lexemeToPrint,
             getTokenTypeName(token.type));
      printf("\n");
    }
  } while (token.type != TK_EOF);

  fclose(fp);
  cleanupLexer();
}
