/*
 * parser.c
 * Syntax Analyzer Implementation
 * Compiler Project - CS F363
 */

#include "parser.h"
#include "lexer.h"
#include <stdbool.h>

// ============================================================================
// Global Parser State
// ============================================================================

static ParserState parserState;
static int parserInitialized = 0;

// ============================================================================
// Terminal Names (for printing and parsing grammar file)
// ============================================================================

static const char *terminalNames[] = {
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

static int numTerminals = sizeof(terminalNames) / sizeof(terminalNames[0]);

// ============================================================================
// Stack Operations
// ============================================================================

void initStack(ParseStack *stack) { stack->top = -1; }

int push(ParseStack *stack, ParseTreeNode *node) {
  if (stack->top >= MAX_STACK_SIZE - 1) {
    fprintf(stderr, "Error: Parse stack overflow\n");
    return -1;
  }
  stack->items[++stack->top] = node;
  return 0;
}

ParseTreeNode *pop(ParseStack *stack) {
  if (stack->top < 0) {
    return NULL;
  }
  return stack->items[stack->top--];
}

ParseTreeNode *top(ParseStack *stack) {
  if (stack->top < 0) {
    return NULL;
  }
  return stack->items[stack->top];
}

int isEmpty(ParseStack *stack) { return stack->top < 0; }

// ============================================================================
// Helper Functions
// ============================================================================

// Get terminal token type from name
static TokenType getTerminalType(const char *name) {
  for (int i = 0; i < numTerminals; i++) {
    if (strcmp(terminalNames[i], name) == 0) {
      return (TokenType)i;
    }
  }
  return TK_ERROR;
}

// Check if a string is a terminal
static int isTerminalName(const char *name) {
  return strncmp(name, "TK_", 3) == 0;
}

// Get non-terminal name
const char *getNonTerminalName(Grammar *G, int index) {
  if (index >= 0 && index < G->numNonTerminals) {
    return G->nonTerminalNames[index];
  }
  return "UNKNOWN";
}

// Get non-terminal index by name
int getNonTerminalIndex(Grammar *G, const char *name) {
  for (int i = 0; i < G->numNonTerminals; i++) {
    if (strcmp(G->nonTerminalNames[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

// Add non-terminal if not exists
static int addNonTerminal(Grammar *G, const char *name) {
  int index = getNonTerminalIndex(G, name);
  if (index >= 0) {
    return index;
  }

  if (G->numNonTerminals >= MAX_NON_TERMINALS) {
    fprintf(stderr, "Error: Too many non-terminals\n");
    return -1;
  }

  strcpy(G->nonTerminalNames[G->numNonTerminals], name);
  return G->numNonTerminals++;
}

// ============================================================================
// Load Grammar from File
// ============================================================================

Grammar *loadGrammar(char *grammarFile) {
  FILE *fp = fopen(grammarFile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: Cannot open grammar file '%s'\n", grammarFile);
    return NULL;
  }

  Grammar *G = &parserState.grammar;
  G->numRules = 0;
  G->numNonTerminals = 0;
  G->startSymbol = 0;

  char line[1024];
  int lineNum = 0;

  while (fgets(line, sizeof(line), fp) != NULL) {
    lineNum++;

    // Skip empty lines and comments
    if (line[0] == '\n' || line[0] == '#' || line[0] == '\0') {
      continue;
    }

    // Remove newline and carriage return
    line[strcspn(line, "\r\n")] = 0;

    // Parse rule: <LHS> -> symbol1 symbol2 ... | eps
    char *arrow = strstr(line, "->");
    if (arrow == NULL) {
      continue;
    }

    // Extract LHS
    char lhsName[MAX_SYMBOL_NAME];
    int lhsLen = arrow - line;
    strncpy(lhsName, line, lhsLen);
    lhsName[lhsLen] = '\0';

    // Trim whitespace from lhsName
    char *start = lhsName;
    while (*start == ' ' || *start == '\t' || *start == '<')
      start++;
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '>'))
      end--;
    *(end + 1) = '\0';

    int lhsIndex = addNonTerminal(G, start);
    if (lhsIndex < 0) {
      fclose(fp);
      return NULL;
    }

    // Set first non-terminal as start symbol
    if (G->numRules == 0) {
      G->startSymbol = lhsIndex;
    }

    // Parse RHS
    char *rhs = arrow + 2;
    while (*rhs == ' ' || *rhs == '\t')
      rhs++;

    GrammarRule *rule = &G->rules[G->numRules];
    rule->lhs = lhsIndex;
    rule->rhsLength = 0;
    rule->ruleNumber = G->numRules;

    // Tokenize RHS
    char *token = strtok(rhs, " \t");
    while (token != NULL) {
      Symbol *sym = &rule->rhs[rule->rhsLength];

      // Check for epsilon
      if (strcmp(token, "eps") == 0 || strcmp(token, "ε") == 0 ||
          strcmp(token, "EPSILON") == 0 || strcmp(token, "∈") == 0) {
        // Epsilon production - keep rhsLength at 0 or mark specially
        rule->rhsLength = 0;
        break;
      }

      // Remove < and > from non-terminals
      if (token[0] == '<') {
        token++;
        char *gt = strchr(token, '>');
        if (gt)
          *gt = '\0';
      }

      if (isTerminalName(token)) {
        sym->isTerminal = 1;
        sym->symbol.terminal = getTerminalType(token);
      } else {
        sym->isTerminal = 0;
        sym->symbol.nonTerminal = addNonTerminal(G, token);
      }

      rule->rhsLength++;
      token = strtok(NULL, " \t");
    }

    G->numRules++;

    if (G->numRules >= MAX_RULES) {
      fprintf(stderr, "Error: Too many grammar rules\n");
      break;
    }
  }

  fclose(fp);

  /* printf("Loaded %d grammar rules with %d non-terminals\n", G->numRules,
         G->numNonTerminals); */

  return G;
}

// ============================================================================
// Compute FIRST Sets
// ============================================================================

static void computeFirst(Grammar *G, FirstAndFollow *F) {
  // Initialize all FIRST sets to empty
  for (int i = 0; i < G->numNonTerminals; i++) {
    F->first[i] = 0;
    F->hasEpsilon[i] = 0;
  }

  bool changed = true;

  while (changed) {
    changed = false;

    for (int r = 0; r < G->numRules; r++) {
      GrammarRule *rule = &G->rules[r];
      int A = rule->lhs;

      // If rule is A -> eps, add epsilon to FIRST(A)
      if (rule->rhsLength == 0) {
        if (!F->hasEpsilon[A]) {
          F->hasEpsilon[A] = 1;
          changed = true;
        }
        continue;
      }

      // For A -> Y1 Y2 ... Yk
      bool allHaveEpsilon = true;

      for (int i = 0; i < rule->rhsLength; i++) {
        Symbol *Y = &rule->rhs[i];

        if (Y->isTerminal) {
          // Add terminal to FIRST(A)
          BitSet bit = 1ULL << Y->symbol.terminal;
          if (!(F->first[A] & bit)) {
            F->first[A] |= bit;
            changed = true;
          }
          allHaveEpsilon = false;
          break;
        } else {
          // Y is non-terminal
          int B = Y->symbol.nonTerminal;

          // Add FIRST(B) - {eps} to FIRST(A)
          BitSet toAdd = F->first[B] & ~(F->first[A]);
          if (toAdd) {
            F->first[A] |= F->first[B];
            changed = true;
          }

          // If FIRST(B) doesn't contain eps, stop
          if (!F->hasEpsilon[B]) {
            allHaveEpsilon = false;
            break;
          }
        }
      }

      // If all Yi have eps in FIRST, add eps to FIRST(A)
      if (allHaveEpsilon && !F->hasEpsilon[A]) {
        F->hasEpsilon[A] = 1;
        changed = true;
      }
    }
  }
}

// ============================================================================
// Compute FOLLOW Sets
// ============================================================================

static void computeFollow(Grammar *G, FirstAndFollow *F) {
  // Initialize all FOLLOW sets to empty
  for (int i = 0; i < G->numNonTerminals; i++) {
    F->follow[i] = 0;
  }

  // Add $ (TK_EOF) to FOLLOW of start symbol
  F->follow[G->startSymbol] |= (1ULL << TK_EOF);

  bool changed = true;

  while (changed) {
    changed = false;

    for (int r = 0; r < G->numRules; r++) {
      GrammarRule *rule = &G->rules[r];
      int A = rule->lhs;

      for (int i = 0; i < rule->rhsLength; i++) {
        Symbol *B = &rule->rhs[i];

        if (B->isTerminal)
          continue;

        int Bidx = B->symbol.nonTerminal;

        // For A -> αBβ
        // Add FIRST(β) - {eps} to FOLLOW(B)
        bool betaDerivesEpsilon = true;

        for (int j = i + 1; j < rule->rhsLength; j++) {
          Symbol *beta = &rule->rhs[j];

          if (beta->isTerminal) {
            BitSet bit = 1ULL << beta->symbol.terminal;
            if (!(F->follow[Bidx] & bit)) {
              F->follow[Bidx] |= bit;
              changed = true;
            }
            betaDerivesEpsilon = false;
            break;
          } else {
            int betaIdx = beta->symbol.nonTerminal;
            BitSet toAdd = F->first[betaIdx] & ~(F->follow[Bidx]);
            if (toAdd) {
              F->follow[Bidx] |= F->first[betaIdx];
              changed = true;
            }

            if (!F->hasEpsilon[betaIdx]) {
              betaDerivesEpsilon = false;
              break;
            }
          }
        }

        // If β derives eps (or β is empty), add FOLLOW(A) to FOLLOW(B)
        if (betaDerivesEpsilon) {
          BitSet toAdd = F->follow[A] & ~(F->follow[Bidx]);
          if (toAdd) {
            F->follow[Bidx] |= F->follow[A];
            changed = true;
          }
        }
      }
    }
  }
}

// ============================================================================
// Compute First and Follow Sets
// ============================================================================

FirstAndFollow ComputeFirstAndFollowSets(Grammar *G) {
  FirstAndFollow F;
  F.numNonTerminals = G->numNonTerminals;
  F.numTerminals = numTerminals;

  computeFirst(G, &F);
  computeFollow(G, &F);

  return F;
}

// ============================================================================
// Print First Sets (for debugging)
// ============================================================================

void printFirstSets(FirstAndFollow *F, Grammar *G) {
  printf("\n=== FIRST Sets ===\n");

  for (int i = 0; i < G->numNonTerminals; i++) {
    printf("FIRST(%s) = { ", G->nonTerminalNames[i]);

    if (F->hasEpsilon[i]) {
      printf("ε ");
    }

    for (int t = 0; t < numTerminals; t++) {
      if (F->first[i] & (1ULL << t)) {
        printf("%s ", terminalNames[t]);
      }
    }
    printf("}\n");
  }
}

// ============================================================================
// Print Follow Sets (for debugging)
// ============================================================================

void printFollowSets(FirstAndFollow *F, Grammar *G) {
  printf("\n=== FOLLOW Sets ===\n");

  for (int i = 0; i < G->numNonTerminals; i++) {
    printf("FOLLOW(%s) = { ", G->nonTerminalNames[i]);

    for (int t = 0; t < numTerminals; t++) {
      if (F->follow[i] & (1ULL << t)) {
        printf("%s ", terminalNames[t]);
      }
    }
    printf("}\n");
  }
}

// ============================================================================
// Create Parse Table
// ============================================================================

void createParseTable(FirstAndFollow F, ParseTable T, Grammar *G) {
  // Initialize table with error entries
  for (int i = 0; i < MAX_NON_TERMINALS; i++) {
    for (int j = 0; j < MAX_TERMINALS; j++) {
      T[i][j] = ERROR_ENTRY;
    }
  }

  // For each production A -> α
  for (int r = 0; r < G->numRules; r++) {
    GrammarRule *rule = &G->rules[r];
    int A = rule->lhs;

    // If α = ε, add rule to T[A][b] for all b in FOLLOW(A)
    if (rule->rhsLength == 0) {
      for (int t = 0; t < numTerminals; t++) {
        if (F.follow[A] & (1ULL << t)) {
          if (T[A][t] == ERROR_ENTRY) {
            T[A][t] = r;
          } else {
            fprintf(stderr,
                    "Warning: Grammar is not LL(1): conflict at [%s][%s]\n",
                    G->nonTerminalNames[A], terminalNames[t]);
          }
        }
      }
      continue;
    }

    // Compute FIRST of α (the RHS)
    BitSet firstAlpha = 0;
    bool alphaDerivesEpsilon = true;

    for (int i = 0; i < rule->rhsLength; i++) {
      Symbol *sym = &rule->rhs[i];

      if (sym->isTerminal) {
        firstAlpha |= (1ULL << sym->symbol.terminal);
        alphaDerivesEpsilon = false;
        break;
      } else {
        firstAlpha |= F.first[sym->symbol.nonTerminal];
        if (!F.hasEpsilon[sym->symbol.nonTerminal]) {
          alphaDerivesEpsilon = false;
          break;
        }
      }
    }

    // For each terminal a in FIRST(α), add rule to T[A][a]
    for (int t = 0; t < numTerminals; t++) {
      if (firstAlpha & (1ULL << t)) {
        if (T[A][t] == ERROR_ENTRY) {
          T[A][t] = r;
        } else {
          fprintf(stderr,
                  "Warning: Grammar is not LL(1): conflict at [%s][%s]\n",
                  G->nonTerminalNames[A], terminalNames[t]);
        }
      }
    }

    // If α derives ε, add rule to T[A][b] for all b in FOLLOW(A)
    if (alphaDerivesEpsilon) {
      for (int t = 0; t < numTerminals; t++) {
        if (F.follow[A] & (1ULL << t)) {
          if (T[A][t] == ERROR_ENTRY) {
            T[A][t] = r;
          }
        }
      }
    }
  }

  // Add sync entries for error recovery
  for (int A = 0; A < G->numNonTerminals; A++) {
    for (int t = 0; t < numTerminals; t++) {
      if (T[A][t] == ERROR_ENTRY && (F.follow[A] & (1ULL << t))) {
        T[A][t] = SYNC;
      }
    }
  }
}

// ============================================================================
// Print Parse Table (for debugging)
// ============================================================================

void printParseTableDebug(ParseTable T, Grammar *G) {
  printf("\n=== Parse Table ===\n");

  for (int A = 0; A < G->numNonTerminals; A++) {
    printf("\n%s:\n", G->nonTerminalNames[A]);
    for (int t = 0; t < numTerminals; t++) {
      if (T[A][t] != ERROR_ENTRY) {
        if (T[A][t] == SYNC) {
          printf("  [%s] = SYNC\n", terminalNames[t]);
        } else {
          printf("  [%s] = Rule %d\n", terminalNames[t], T[A][t]);
        }
      }
    }
  }
}

// ============================================================================
// Create Parse Tree Node
// ============================================================================

static ParseTreeNode *createNode(Symbol sym, ParseTreeNode *parent) {
  ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
  if (node == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for parse tree node\n");
    return NULL;
  }

  node->symbol = sym;
  node->token = NULL;
  node->parent = parent;
  node->firstChild = NULL;
  node->nextSibling = NULL;
  node->lineNo = 0;
  node->ruleNumber = -1;
  node->isLeaf = sym.isTerminal;
  node->symbolName[0] = '\0';

  return node;
}

// ============================================================================
// Helper to get next token while skipping comments
// ============================================================================

static tokenInfo getNextTokenFiltered(TwinBuffer *B) {
  tokenInfo token;
  do {
    token = getNextToken(B);
  } while (token.type == TK_COMMENT);
  return token;
}

// ============================================================================
// Parse Input Source Code
// ============================================================================

parseTree parseInputSourceCode(char *testcaseFile, ParseTable T, Grammar *G) {
  FILE *fp = fopen(testcaseFile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error: Cannot open source file '%s'\n", testcaseFile);
    return NULL;
  }

  // Initialize lexer
  TwinBuffer buffer;
  initLexer();
  initTwinBuffer(&buffer, fp);

  // Create parse tree
  ParseTree *PT = (ParseTree *)malloc(sizeof(ParseTree));
  PT->nodeCount = 0;

  // Create root node (start symbol)
  Symbol startSym;
  startSym.isTerminal = 0;
  startSym.symbol.nonTerminal = G->startSymbol;

  PT->root = createNode(startSym, NULL);
  strcpy(PT->root->symbolName, G->nonTerminalNames[G->startSymbol]);
  PT->nodeCount++;

  // Initialize parse stack
  ParseStack stack;
  initStack(&stack);

  // Push $ (end marker) and start symbol
  ParseTreeNode *endMarker = createNode(startSym, NULL);
  endMarker->symbol.isTerminal = 1;
  endMarker->symbol.symbol.terminal = TK_EOF;
  push(&stack, endMarker);
  push(&stack, PT->root);

  // Get first token
  tokenInfo currentToken = getNextTokenFiltered(&buffer);

  // Error tracking
  int syntaxErrors = 0;
  ErrorList errors;
  errors.count = 0;

  // Parsing loop
  while (!isEmpty(&stack)) {
    ParseTreeNode *topNode = top(&stack);

    if (topNode == NULL)
      break;

    if (topNode->symbol.isTerminal) {
      // Top is terminal
      TokenType expected = topNode->symbol.symbol.terminal;

      if (expected == currentToken.type) {
        // Match!
        pop(&stack);

        // Store token info in node
        topNode->token = (tokenInfo *)malloc(sizeof(tokenInfo));
        *(topNode->token) = currentToken;
        topNode->lineNo = currentToken.lineNo;
        topNode->isLeaf = 1;
        strcpy(topNode->symbolName, getTokenTypeName(currentToken.type));

        // Get next token
        if (currentToken.type != TK_EOF) {
          currentToken = getNextTokenFiltered(&buffer);
        }
      } else {
        // Error: terminal mismatch
        syntaxErrors++;
        fprintf(stderr,
                "Syntax Error at line %d: Expected '%s' but found '%s'\n",
                currentToken.lineNo, getTokenTypeName(expected),
                getTokenTypeName(currentToken.type));

        // Pop the terminal and continue (skip expected)
        pop(&stack);
      }
    } else {
      // Top is non-terminal
      int A = topNode->symbol.symbol.nonTerminal;
      int t = currentToken.type;

      int ruleNum = T[A][t];

      if (ruleNum == ERROR_ENTRY) {
        // Error: no production
        syntaxErrors++;
        fprintf(stderr,
                "Syntax Error at line %d: Unexpected token '%s' for "
                "non-terminal '%s'\n",
                currentToken.lineNo, currentToken.lexeme,
                G->nonTerminalNames[A]);

        // Panic mode: skip tokens until we find one in FOLLOW(A)
        while (currentToken.type != TK_EOF &&
               T[A][currentToken.type] == ERROR_ENTRY) {
          currentToken = getNextTokenFiltered(&buffer);
        }

        if (currentToken.type == TK_EOF) {
          pop(&stack);
        }
      } else if (ruleNum == SYNC) {
        // Sync entry: pop and continue
        pop(&stack);
      } else {
        // Valid production
        pop(&stack);

        GrammarRule *rule = &G->rules[ruleNum];
        topNode->ruleNumber = ruleNum;
        topNode->lineNo = currentToken.lineNo;

        // Create child nodes and push in reverse order
        ParseTreeNode *children[MAX_RHS_LENGTH];
        int numChildren = rule->rhsLength;

        ParseTreeNode *prevChild = NULL;
        for (int i = 0; i < numChildren; i++) {
          ParseTreeNode *child = createNode(rule->rhs[i], topNode);
          PT->nodeCount++;

          if (rule->rhs[i].isTerminal) {
            strcpy(child->symbolName,
                   terminalNames[rule->rhs[i].symbol.terminal]);
          } else {
            strcpy(child->symbolName,
                   G->nonTerminalNames[rule->rhs[i].symbol.nonTerminal]);
          }

          children[i] = child;

          if (i == 0) {
            topNode->firstChild = child;
          } else {
            prevChild->nextSibling = child;
          }
          prevChild = child;
        }

        // Push children in reverse order
        for (int i = numChildren - 1; i >= 0; i--) {
          push(&stack, children[i]);
        }
      }
    }
  }

  fclose(fp);

  // Report result
  if (syntaxErrors == 0) {
    printf("\nInput source code is syntactically correct...........\n");
  } else {
    printf("\nParsing completed with %d syntax error(s)\n", syntaxErrors);
  }

  return PT;
}

// ============================================================================
// Print Parse Tree (Inorder Traversal)
// ============================================================================

static void printNodeInorder(ParseTreeNode *node, FILE *out, Grammar *G) {
  if (node == NULL)
    return;

  // Visit left subtree (first child)
  printNodeInorder(node->firstChild, out, G);

  // Print current node
  // Format: lexeme CurrentNode lineno tokenName valueIfNumber parentNodeSymbol
  // isLeafNode NodeSymbol

  char lexeme[MAX_LEXEME_LEN + 1];
  char tokenName[50];
  char value[50];
  char parentSymbol[MAX_SYMBOL_NAME];
  char isLeaf[5];
  char nodeSymbol[MAX_SYMBOL_NAME];

  // Lexeme
  if (node->isLeaf && node->token != NULL) {
    strcpy(lexeme, node->token->lexeme);
  } else {
    strcpy(lexeme, "----");
  }

  // Line number
  int lineNo = node->lineNo;
  if (node->token != NULL) {
    lineNo = node->token->lineNo;
  }

  // Token name
  if (node->isLeaf && node->token != NULL) {
    strcpy(tokenName, getTokenTypeName(node->token->type));
  } else {
    strcpy(tokenName, "----");
  }

  // Value
  if (node->token != NULL && node->token->type == TK_NUM) {
    sprintf(value, "%d", node->token->value.intValue);
  } else if (node->token != NULL && node->token->type == TK_RNUM) {
    sprintf(value, "%lf", node->token->value.realValue);
  } else {
    strcpy(value, "----");
  }

  // Parent symbol
  if (node->parent == NULL) {
    strcpy(parentSymbol, "ROOT");
  } else {
    strcpy(parentSymbol, node->parent->symbolName);
  }

  // Is leaf
  strcpy(isLeaf, node->isLeaf ? "yes" : "no");

  // Node symbol
  if (node->isLeaf) {
    strcpy(nodeSymbol, "----");
  } else {
    strcpy(nodeSymbol, node->symbolName);
  }

  fprintf(out, "%-25s %-25s %-6d %-20s %-15s %-25s %-5s %-25s\n", lexeme,
          node->symbolName, lineNo, tokenName, value, parentSymbol, isLeaf,
          nodeSymbol);

  // Visit right subtree (siblings)
  printNodeInorder(node->nextSibling, out, G);
}

void printParseTree(parseTree PT, char *outfile) {
  if (PT == NULL || PT->root == NULL) {
    fprintf(stderr, "Error: Parse tree is empty\n");
    return;
  }

  FILE *out = fopen(outfile, "w");
  if (out == NULL) {
    fprintf(stderr, "Error: Cannot open output file '%s'\n", outfile);
    return;
  }

  // Print header
  fprintf(out, "%-25s %-25s %-6s %-20s %-15s %-25s %-5s %-25s\n", "Lexeme",
          "CurrentNode", "Line", "TokenName", "Value", "ParentNode", "Leaf",
          "NodeSymbol");
  fprintf(out, "%-25s %-25s %-6s %-20s %-15s %-25s %-5s %-25s\n", "------",
          "-----------", "----", "---------", "-----", "----------", "----",
          "----------");

  printNodeInorder(PT->root, out, &parserState.grammar);

  fclose(out);
  printf("Parse tree written to '%s'\n", outfile);
}

// ============================================================================
// Free Parse Tree
// ============================================================================

static void freeNode(ParseTreeNode *node) {
  if (node == NULL)
    return;

  freeNode(node->firstChild);
  freeNode(node->nextSibling);

  if (node->token != NULL) {
    free(node->token);
  }
  free(node);
}

void freeParseTree(parseTree PT) {
  if (PT == NULL)
    return;

  freeNode(PT->root);
  free(PT);
}

// ============================================================================
// Initialize Parser
// ============================================================================

int initParser(char *grammarFile) {
  if (parserInitialized) {
    return 0;
  }

  Grammar *G = loadGrammar(grammarFile);
  if (G == NULL) {
    return -1;
  }

  parserState.firstFollow = ComputeFirstAndFollowSets(G);
  createParseTable(parserState.firstFollow, parserState.parseTable, G);

  parserInitialized = 1;
  return 0;
}

// ============================================================================
// Cleanup Parser
// ============================================================================

void cleanupParser(void) {
  if (parserState.parseTree != NULL) {
    freeParseTree(parserState.parseTree);
    parserState.parseTree = NULL;
  }

  parserInitialized = 0;
}
