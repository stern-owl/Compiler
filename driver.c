#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void printImplementationStatus(void);
void printMenu(void);
void driverMenu(int option, char *srcFile, char *parseTreeFile);
int ensureParserReady(void);

static Grammar *grammar = NULL;
static FirstAndFollow firstFollow;
static ParseTable parseTable;
static int parserReady = 0;

int main(int argc, char *argv[]) {
  char srcFile[256] = "";
  char parseTreeFile[256] = "outfile.txt";

  if (argc >= 2) {
    strcpy(srcFile, argv[1]);
  }
  if (argc >= 3) {
    strcpy(parseTreeFile, argv[2]);
  }

  if (strlen(srcFile) == 0) {
    printf("Enter source code file path: ");
    if (scanf("%255s", srcFile) != 1) {
      fprintf(stderr, "Error: Failed to read source file path\n");
      return 1;
    }
  }

  printImplementationStatus();

  int option;

  while (1) {
    printMenu();
    printf("Enter your choice: ");

    if (scanf("%d", &option) != 1) {
      printf("Invalid input. Please enter a number.\n");
      while (getchar() != '\n')
        ;
      continue;
    }

    if (option == 0) {
      printf("\nExiting...\n");
      break;
    }

    driverMenu(option, srcFile, parseTreeFile);
  }

  cleanupLexer();
  cleanupParser();

  return 0;
}

void printImplementationStatus(void) {
  printf("\n");
  printf("       IMPLEMENTATION STATUS       \n");
  printf("\n");
  printf("  FIRST and FOLLOW set computation: AUTOMATED\n");
  printf("  Lexical Analyzer Module:          IMPLEMENTED\n");
  printf("  Syntax Analyzer Module:           IMPLEMENTED\n");
  printf("  Parse Tree Construction:          IMPLEMENTED\n");
  printf("  Both modules compile successfully and work correctly.\n");
  printf("\n");
}

void printMenu(void) {
  printf("\n");
  printf("----------------------------------------\n");
  printf("               MENU                     \n");
  printf("----------------------------------------\n");
  printf("0. Exit\n");
  printf("1. Remove comments and print clean code\n");
  printf("2. Print token list\n");
  printf("3. Parse and print parse tree\n");
  printf("4. Print time taken\n");
  printf("----------------------------------------\n");
}

void driverMenu(int option, char *srcFile, char *parseTreeFile) {
  clock_t startTime, endTime;
  double totalCPUTime, totalCPUTimeInSeconds;

  switch (option) {
  case 1: {
    FILE *fp = fopen(srcFile, "r");
    if (fp == NULL) {
      printf("Error: Could not open source file '%s'\n", srcFile);
      break;
    }

    printf("\n");
    int ch;
    int inComment = 0;

    while ((ch = fgetc(fp)) != EOF) {
      if (ch == '%') {
        inComment = 1;
      } else if (ch == '\n') {
        inComment = 0;
        putchar(ch);
      } else if (!inComment) {
        putchar(ch);
      }
    }
    printf("\n");

    fclose(fp);
    break;
  }

  case 2: {
    printf("\n%-8s %-25s %-20s\n", "Line No", "Lexeme", "Token Name");
    printf("%-8s %-25s %-20s\n", "-------", "-------------------------",
           "--------------------");

    initLexer();
    FILE *fp = fopen(srcFile, "r");
    if (fp == NULL) {
      printf("Error: Could not open source file '%s'\n", srcFile);
      break;
    }

    TwinBuffer buffer;
    initTwinBuffer(&buffer, fp);
    tokenInfo token;

    do {
      token = getNextToken(&buffer);

      if (token.type == TK_EOF) {
        break;
      }

      const char *lexemeToPrint = (token.type == TK_COMMENT) ? "%" : token.lexeme;
      printf("%-8d %-25s %-20s", token.lineNo, lexemeToPrint,
             getTokenTypeName(token.type));

      printf("\n");

    } while (token.type != TK_EOF);

    fclose(fp);
    break;
  }

  case 3: {
    if (!ensureParserReady()) {
      break;
    }

    parseTree PT = parseInputSourceCode(srcFile, parseTable, grammar);

    if (PT != NULL) {
      printParseTree(PT, parseTreeFile);
      freeParseTree(PT);
    }
    break;
  }

  case 4: {
    if (!ensureParserReady()) {
      break;
    }

    startTime = clock();

    parseTree PT = parseInputSourceCode(srcFile, parseTable, grammar);

    endTime = clock();

    totalCPUTime = (double)(endTime - startTime);
    totalCPUTimeInSeconds = totalCPUTime / CLOCKS_PER_SEC;

    if (PT != NULL) {
      freeParseTree(PT);
    }

    printf("\nTotal CPU time:             %f\n", totalCPUTime);
    printf("Total CPU time in seconds:  %f\n", totalCPUTimeInSeconds);
    break;
  }

  default:
    printf("Invalid option. Please enter 0, 1, 2, 3, or 4.\n");
    break;
  }
}

int ensureParserReady(void) {
  if (parserReady) {
    return 1;
  }

  if (initLexer() != 0) {
    fprintf(stderr, "Error: Failed to initialize lexer\n");
    return 0;
  }

  grammar = loadGrammar("grammar.txt");
  if (grammar != NULL) {
    firstFollow = ComputeFirstAndFollowSets(grammar);
    createParseTable(firstFollow, parseTable, grammar);
    parserReady = 1;
    return 1;
  } else {
    return 0;
  }
}
