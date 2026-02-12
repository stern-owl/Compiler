#include "parser.h"
#include "lexer.h"
#include <stdbool.h>

static ParserState parserState;
static int parserInitialized = 0;

static const char *terminalNames[] = {
    "TK_ASSIGNOP", "TK_COMMENT", "TK_FIELDID", "TK_ID",
    "TK_FUNID", "TK_RUID", "TK_NUM", "TK_RNUM",
    "TK_AS", "TK_CALL", "TK_DEFINETYPE", "TK_ELSE",
    "TK_END", "TK_ENDRECORD", "TK_ENDIF", "TK_ENDUNION",
    "TK_ENDWHILE", "TK_GLOBAL", "TK_IF", "TK_INPUT",
    "TK_INT", "TK_LIST", "TK_MAIN", "TK_OUTPUT",
    "TK_PARAMETER", "TK_PARAMETERS", "TK_READ", "TK_REAL",
    "TK_RECORD", "TK_RETURN", "TK_THEN", "TK_TYPE",
    "TK_UNION", "TK_WHILE", "TK_WITH", "TK_WRITE",
    "TK_PLUS", "TK_MINUS", "TK_MUL", "TK_DIV",
    "TK_AND", "TK_OR", "TK_NOT", "TK_LT",
    "TK_LE", "TK_EQ", "TK_GT", "TK_GE",
    "TK_NE", "TK_COMMA", "TK_SEM", "TK_COLON",
    "TK_DOT", "TK_OP", "TK_CL", "TK_SQL",
    "TK_SQR", "TK_EOF", "TK_ERROR"};

static int numTerminals = sizeof(terminalNames) / sizeof(terminalNames[0]);

void initStack(ParseStack *stack) { stack->top = -1; }

int push(ParseStack *stack, ParseTreeNode *node) {
  if (stack->top >= MAX_STACK_SIZE - 1) {
    fprintf(stderr, "Stack overflow\n");
    return -1;
  }
  stack->items[++stack->top] = node;
  return 0;
}

ParseTreeNode *pop(ParseStack *stack) {
  if (stack->top < 0) return NULL;
  return stack->items[stack->top--];
}

ParseTreeNode *top(ParseStack *stack) {
  if (stack->top < 0) return NULL;
  return stack->items[stack->top];
}

int isEmpty(ParseStack *stack) { return stack->top < 0; }

static TokenType getTerminalType(const char *name) {
  for (int i = 0; i < numTerminals; i++) {
    if (strcmp(terminalNames[i], name) == 0) return (TokenType)i;
  }
  return TK_ERROR;
}

static int isTerminalName(const char *name) {
  return strncmp(name, "TK_", 3) == 0;
}

const char *getNonTerminalName(Grammar *G, int index) {
  if (index >= 0 && index < G->numNonTerminals) return G->nonTerminalNames[index];
  return "UNKNOWN";
}

int getNonTerminalIndex(Grammar *G, const char *name) {
  for (int i = 0; i < G->numNonTerminals; i++) {
    if (strcmp(G->nonTerminalNames[i], name) == 0) return i;
  }
  return -1;
}

static int addNonTerminal(Grammar *G, const char *name) {
  int index = getNonTerminalIndex(G, name);
  if (index >= 0) return index;

  if (G->numNonTerminals >= MAX_NON_TERMINALS) {
    fprintf(stderr, "Error: Too many non-terminals\n");
    return -1;
  }

  strcpy(G->nonTerminalNames[G->numNonTerminals], name);
  return G->numNonTerminals++;
}

Grammar *loadGrammar(char *grammarFile) {
  FILE *fp = fopen(grammarFile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open grammar '%s'\n", grammarFile);
    return NULL;
  }

  Grammar *G = &parserState.grammar;
  G->numRules = 0;
  G->numNonTerminals = 0;
  G->startSymbol = 0;

  char line[1024];
  while (fgets(line, sizeof(line), fp) != NULL) {
    if (line[0] == '\n' || line[0] == '#' || line[0] == '\0') continue;
    line[strcspn(line, "\r\n")] = 0;

    char *arrow = strstr(line, "->");
    if (arrow == NULL) continue;

    char lhsName[MAX_SYMBOL_NAME];
    int lhsLen = arrow - line;
    strncpy(lhsName, line, lhsLen);
    lhsName[lhsLen] = '\0';

    char *start = lhsName;
    while (*start == ' ' || *start == '\t' || *start == '<') start++;
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '>')) end--;
    *(end + 1) = '\0';

    int lhsIndex = addNonTerminal(G, start);
    if (lhsIndex < 0) { fclose(fp); return NULL; }

    if (G->numRules == 0) G->startSymbol = lhsIndex;

    char *rhs = arrow + 2;
    while (*rhs == ' ' || *rhs == '\t') rhs++;

    GrammarRule *rule = &G->rules[G->numRules];
    rule->lhs = lhsIndex;
    rule->rhsLength = 0;
    rule->ruleNumber = G->numRules;

    char *token = strtok(rhs, " \t");
    while (token != NULL) {
      Symbol *sym = &rule->rhs[rule->rhsLength];

      if (strcmp(token, "eps") == 0 || strcmp(token, "ε") == 0 ||
          strcmp(token, "EPSILON") == 0 || strcmp(token, "∈") == 0) {
        rule->rhsLength = 0;
        break;
      }

      if (token[0] == '<') {
        token++;
        char *gt = strchr(token, '>');
        if (gt) *gt = '\0';
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
    if (G->numRules >= MAX_RULES) break;
  }

  fclose(fp);
  return G;
}

static void computeFirst(Grammar *G, FirstAndFollow *F) {
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

      if (rule->rhsLength == 0) {
        if (!F->hasEpsilon[A]) {
          F->hasEpsilon[A] = 1;
          changed = true;
        }
        continue;
      }

      bool allHaveEpsilon = true;
      for (int i = 0; i < rule->rhsLength; i++) {
        Symbol *Y = &rule->rhs[i];
        if (Y->isTerminal) {
          BitSet bit = 1ULL << Y->symbol.terminal;
          if (!(F->first[A] & bit)) {
            F->first[A] |= bit;
            changed = true;
          }
          allHaveEpsilon = false;
          break;
        } else {
          int B = Y->symbol.nonTerminal;
          BitSet toAdd = F->first[B] & ~(F->first[A]);
          if (toAdd) {
            F->first[A] |= F->first[B];
            changed = true;
          }
          if (!F->hasEpsilon[B]) {
            allHaveEpsilon = false;
            break;
          }
        }
      }

      if (allHaveEpsilon && !F->hasEpsilon[A]) {
        F->hasEpsilon[A] = 1;
        changed = true;
      }
    }
  }
}

static void computeFollow(Grammar *G, FirstAndFollow *F) {
  for (int i = 0; i < G->numNonTerminals; i++) F->follow[i] = 0;
  F->follow[G->startSymbol] |= (1ULL << TK_EOF);

  bool changed = true;
  while (changed) {
    changed = false;
    for (int r = 0; r < G->numRules; r++) {
      GrammarRule *rule = &G->rules[r];
      int A = rule->lhs;

      for (int i = 0; i < rule->rhsLength; i++) {
        Symbol *B = &rule->rhs[i];
        if (B->isTerminal) continue;

        int Bidx = B->symbol.nonTerminal;
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

FirstAndFollow ComputeFirstAndFollowSets(Grammar *G) {
  FirstAndFollow F;
  F.numNonTerminals = G->numNonTerminals;
  F.numTerminals = numTerminals;
  computeFirst(G, &F);
  computeFollow(G, &F);
  return F;
}

void printFirstSets(FirstAndFollow *F, Grammar *G) {
  printf("\n=== FIRST Sets ===\n");
  for (int i = 0; i < G->numNonTerminals; i++) {
    printf("FIRST(%s) = { ", G->nonTerminalNames[i]);
    if (F->hasEpsilon[i]) printf("eps ");
    for (int t = 0; t < numTerminals; t++) {
      if (F->first[i] & (1ULL << t)) printf("%s ", terminalNames[t]);
    }
    printf("}\n");
  }
}

void printFollowSets(FirstAndFollow *F, Grammar *G) {
  printf("\n=== FOLLOW Sets ===\n");
  for (int i = 0; i < G->numNonTerminals; i++) {
    printf("FOLLOW(%s) = { ", G->nonTerminalNames[i]);
    for (int t = 0; t < numTerminals; t++) {
      if (F->follow[i] & (1ULL << t)) printf("%s ", terminalNames[t]);
    }
    printf("}\n");
  }
}

void createParseTable(FirstAndFollow F, ParseTable T, Grammar *G) {
  for (int i = 0; i < MAX_NON_TERMINALS; i++) {
    for (int j = 0; j < MAX_TERMINALS; j++) T[i][j] = ERROR_ENTRY;
  }

  for (int r = 0; r < G->numRules; r++) {
    GrammarRule *rule = &G->rules[r];
    int A = rule->lhs;

    if (rule->rhsLength == 0) {
      for (int t = 0; t < numTerminals; t++) {
        if (F.follow[A] & (1ULL << t)) {
          if (T[A][t] == ERROR_ENTRY) T[A][t] = r;
        }
      }
      continue;
    }

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

    for (int t = 0; t < numTerminals; t++) {
      if (firstAlpha & (1ULL << t)) {
        if (T[A][t] == ERROR_ENTRY) T[A][t] = r;
      }
    }

    if (alphaDerivesEpsilon) {
      for (int t = 0; t < numTerminals; t++) {
        if (F.follow[A] & (1ULL << t)) {
          if (T[A][t] == ERROR_ENTRY) T[A][t] = r;
        }
      }
    }
  }

  for (int A = 0; A < G->numNonTerminals; A++) {
    for (int t = 0; t < numTerminals; t++) {
      if (T[A][t] == ERROR_ENTRY && (F.follow[A] & (1ULL << t))) {
        T[A][t] = SYNC;
      }
    }
  }
}

void printParseTableDebug(ParseTable T, Grammar *G) {
  printf("\n=== Parse Table ===\n");
  for (int A = 0; A < G->numNonTerminals; A++) {
    // Only print if there's something interesting? simplified.
  }
}

static ParseTreeNode *createNode(Symbol sym, ParseTreeNode *parent) {
  ParseTreeNode *node = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
  if (!node) {
    fprintf(stderr, "Mem alloc failed\n");
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

static tokenInfo getNextTokenFiltered(TwinBuffer *B) {
  tokenInfo token;
  do {
    token = getNextToken(B);
  } while (token.type == TK_COMMENT);
  return token;
}

parseTree parseInputSourceCode(char *srcFile, ParseTable T, Grammar *G) {
  FILE *fp = fopen(srcFile, "r");
  if (fp == NULL) {
    fprintf(stderr, "Cannot open source '%s'\n", srcFile);
    return NULL;
  }

  TwinBuffer buffer;
  initLexer();
  initTwinBuffer(&buffer, fp);

  ParseTree *PT = (ParseTree *)malloc(sizeof(ParseTree));
  PT->nodeCount = 0;

  Symbol startSym;
  startSym.isTerminal = 0;
  startSym.symbol.nonTerminal = G->startSymbol;

  PT->root = createNode(startSym, NULL);
  strcpy(PT->root->symbolName, G->nonTerminalNames[G->startSymbol]);
  PT->nodeCount++;

  ParseStack stack;
  initStack(&stack);

  ParseTreeNode *endMarker = createNode(startSym, NULL);
  endMarker->symbol.isTerminal = 1;
  endMarker->symbol.symbol.terminal = TK_EOF;
  push(&stack, endMarker);
  push(&stack, PT->root);

  tokenInfo currentToken = getNextTokenFiltered(&buffer);
  int syntaxErrors = 0;

  while (!isEmpty(&stack)) {
    ParseTreeNode *topNode = top(&stack);
    if (!topNode) break;

    if (topNode->symbol.isTerminal) {
      TokenType expected = topNode->symbol.symbol.terminal;
      if (expected == currentToken.type) {
        pop(&stack);
        topNode->token = (tokenInfo *)malloc(sizeof(tokenInfo));
        *(topNode->token) = currentToken;
        topNode->lineNo = currentToken.lineNo;
        topNode->isLeaf = 1;
        strcpy(topNode->symbolName, getTokenTypeName(currentToken.type));
        if (currentToken.type != TK_EOF) currentToken = getNextTokenFiltered(&buffer);
      } else {
        syntaxErrors++;
        fprintf(stderr, "Syntax Error at line %d: Expected '%s' but found '%s'\n",
                currentToken.lineNo, getTokenTypeName(expected), getTokenTypeName(currentToken.type));
        pop(&stack);
      }
    } else {
      int A = topNode->symbol.symbol.nonTerminal;
      int t = currentToken.type;
      int ruleNum = T[A][t];

      if (ruleNum == ERROR_ENTRY) {
        syntaxErrors++;
        fprintf(stderr, "Syntax Error at line %d: Unexpected token '%s' for non-terminal '%s'\n",
                currentToken.lineNo, currentToken.lexeme, G->nonTerminalNames[A]);
        
        while (currentToken.type != TK_EOF && T[A][currentToken.type] == ERROR_ENTRY) {
          currentToken = getNextTokenFiltered(&buffer);
        }
        if (currentToken.type == TK_EOF) pop(&stack);
      } else if (ruleNum == SYNC) {
        pop(&stack);
      } else {
        pop(&stack);
        GrammarRule *rule = &G->rules[ruleNum];
        topNode->ruleNumber = ruleNum;
        topNode->lineNo = currentToken.lineNo;

        ParseTreeNode *children[MAX_RHS_LENGTH];
        int numChildren = rule->rhsLength;
        ParseTreeNode *prevChild = NULL;

        for (int i = 0; i < numChildren; i++) {
          ParseTreeNode *child = createNode(rule->rhs[i], topNode);
          PT->nodeCount++;
          if (rule->rhs[i].isTerminal)
            strcpy(child->symbolName, terminalNames[rule->rhs[i].symbol.terminal]);
          else
            strcpy(child->symbolName, G->nonTerminalNames[rule->rhs[i].symbol.nonTerminal]);

          children[i] = child;
          if (i == 0) topNode->firstChild = child;
          else prevChild->nextSibling = child;
          prevChild = child;
        }

        for (int i = numChildren - 1; i >= 0; i--) push(&stack, children[i]);
      }
    }
  }

  fclose(fp);

  if (syntaxErrors == 0) printf("\nInput source code is syntactically correct.\n");
  else printf("\nParsing completed with %d syntax error(s)\n", syntaxErrors);

  return PT;
}

static void printNodeInorder(ParseTreeNode *node, FILE *out, Grammar *G) {
  if (!node) return;

  printNodeInorder(node->firstChild, out, G);

  char lexeme[MAX_LEXEME_LEN + 1];
  char tokenName[50];
  char value[50];
  char parentSymbol[MAX_SYMBOL_NAME];
  char isLeaf[5];
  char nodeSymbol[MAX_SYMBOL_NAME];

  if (node->isLeaf && node->token != NULL) strcpy(lexeme, node->token->lexeme);
  else strcpy(lexeme, "----");

  int lineNo = node->lineNo;
  if (node->token != NULL) lineNo = node->token->lineNo;

  if (node->isLeaf && node->token != NULL) strcpy(tokenName, getTokenTypeName(node->token->type));
  else strcpy(tokenName, "----");

  if (node->token != NULL && node->token->type == TK_NUM)
    sprintf(value, "%d", node->token->value.intValue);
  else if (node->token != NULL && node->token->type == TK_RNUM)
    sprintf(value, "%lf", node->token->value.realValue);
  else strcpy(value, "----");

  if (!node->parent) strcpy(parentSymbol, "ROOT");
  else strcpy(parentSymbol, node->parent->symbolName);

  strcpy(isLeaf, node->isLeaf ? "yes" : "no");

  if (node->isLeaf) strcpy(nodeSymbol, "----");
  else strcpy(nodeSymbol, node->symbolName);

  fprintf(out, "%-25s %-25s %-6d %-20s %-15s %-25s %-5s %-25s\n", lexeme,
          node->symbolName, lineNo, tokenName, value, parentSymbol, isLeaf, nodeSymbol);

  printNodeInorder(node->nextSibling, out, G);
}

void printParseTree(parseTree PT, char *outfile) {
  if (!PT || !PT->root) return;

  FILE *out = fopen(outfile, "w");
  if (!out) {
    fprintf(stderr, "Error: Cannot open '%s'\n", outfile);
    return;
  }

  // Header
  fprintf(out, "%-25s %-25s %-6s %-20s %-15s %-25s %-5s %-25s\n", "Lexeme",
          "CurrentNode", "Line", "TokenName", "Value", "ParentNode", "Leaf", "NodeSymbol");
  fprintf(out, "%-25s %-25s %-6s %-20s %-15s %-25s %-5s %-25s\n", "------",
          "-----------", "----", "---------", "-----", "----------", "----", "----------");

  printNodeInorder(PT->root, out, &parserState.grammar);
  fclose(out);
  printf("Parse tree written to '%s'\n", outfile);
}

static void freeNode(ParseTreeNode *node) {
  if (!node) return;
  freeNode(node->firstChild);
  freeNode(node->nextSibling);
  if (node->token) free(node->token);
  free(node);
}

void freeParseTree(parseTree PT) {
  if (!PT) return;
  freeNode(PT->root);
  free(PT);
}

void cleanupParser(void) {
}
