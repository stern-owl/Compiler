/*
 * driver.c
 * Main Driver Program
 * Compiler Project - CS F363
 */

#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// Function Declarations
// ============================================================================

void printImplementationStatus(void);
void printMenu(void);
void processOption(int option, char *sourceFile, char *parseTreeFile);
int ensureParserReady(void);

// ============================================================================
// Global Variables
// ============================================================================

static Grammar *grammar = NULL;
static FirstAndFollow firstFollow;
static ParseTable parseTable;
static int parserReady = 0;

// ============================================================================
// Main Function
// ============================================================================

int main(int argc, char *argv[]) {
  char sourceFile[256] = "";
  char parseTreeFile[256] = "outfile.txt";

  // Check command line arguments: ./stage1exe testcase.txt parsetreeOutFile.txt
  if (argc >= 2) {
    strcpy(sourceFile, argv[1]);
  }
  if (argc >= 3) {
    strcpy(parseTreeFile, argv[2]);
  }

  // If no source file provided, ask for it
  if (strlen(sourceFile) == 0) {
    printf("Enter source code file path: ");
    if (scanf("%255s", sourceFile) != 1) {
      fprintf(stderr, "Error: Failed to read source file path\n");
      return 1;
    }
  }

  // Print implementation status at the beginning
  printImplementationStatus();

  /* printf("\nSource File:     %s\n", sourceFile);
  printf("Parse Tree File: %s\n\n", parseTreeFile); */

  // Removed early initialization - moved to ensureParserReady()

  int option;

  // Main menu loop - continues until option 0 is pressed
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

    processOption(option, sourceFile, parseTreeFile);
  }

  // Cleanup
  cleanupLexer();
  cleanupParser();

  return 0;
}

// ============================================================================
// Print Implementation Status
// ============================================================================

void printImplementationStatus(void) {
  printf("\n");
  // printf("==============================================================\n");
  printf("       IMPLEMENTATION STATUS       \n");
  // printf("==============================================================\n");
  printf("\n");
  printf("  FIRST and FOLLOW set computation: AUTOMATED\n");
  printf("  Lexical Analyzer Module:          IMPLEMENTED\n");
  printf("  Syntax Analyzer Module:           IMPLEMENTED\n");
  printf("  Parse Tree Construction:          IMPLEMENTED\n");
  printf("  Both modules compile successfully and work correctly.\n");
  printf("\n");
  // printf("==============================================================\n");
}

// ============================================================================
// Print Menu
// ============================================================================

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

// ============================================================================
// Process Menu Option
// ============================================================================

void processOption(int option, char *sourceFile, char *parseTreeFile) {
  clock_t startTime, endTime;
  double totalCPUTime, totalCPUTimeInSeconds;

  switch (option) {
  case 1: {
    // Option 1: Remove comments and print clean code on console
    // Do NOT create a separate file
    FILE *fp = fopen(sourceFile, "r");
    if (fp == NULL) {
      printf("Error: Could not open source file '%s'\n", sourceFile);
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
    // Option 2: Print token list (lexer only, not passed to parser)
    // Pretty printing with column justifications
    printf("\n%-8s %-25s %-20s\n", "Line No", "Lexeme", "Token Name");
    printf("%-8s %-25s %-20s\n", "-------", "-------------------------",
           "--------------------");

    initLexer();
    FILE *fp = fopen(sourceFile, "r");
    if (fp == NULL) {
      printf("Error: Could not open source file '%s'\n", sourceFile);
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

      // Print each token on a new line with pretty formatting
      const char *lexemeToPrint = (token.type == TK_COMMENT) ? "%" : token.lexeme;
      printf("%-8d %-25s %-20s", token.lineNo, lexemeToPrint,
             getTokenTypeName(token.type));

      printf("\n");

    } while (token.type != TK_EOF);

    fclose(fp);
    break;
  }

  case 3: {
    // Option 3: Parse to verify syntactic correctness
    // Prints all errors (lexical and syntactic) line number wise on console
    // Prints parse tree to file specified in command line
    if (!ensureParserReady()) {
      break;
    }

    // parseInputSourceCode will print all errors to console
    parseTree PT = parseInputSourceCode(sourceFile, parseTable, grammar);

    if (PT != NULL) {
      printParseTree(PT, parseTreeFile);
      freeParseTree(PT);
    }
    break;
  }

  case 4: {
    // Option 4: Print total time taken by lexer and parser
    if (!ensureParserReady()) {
      break;
    }

    // Measure combined lexer + parser time
    startTime = clock();

    parseTree PT = parseInputSourceCode(sourceFile, parseTable, grammar);

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

// ============================================================================
// Lazy Parser Initialization
// ============================================================================

int ensureParserReady(void) {
  if (parserReady) {
    return 1;
  }

  // Initialize lexer
  if (initLexer() != 0) {
    fprintf(stderr, "Error: Failed to initialize lexer\n");
    return 0;
  }

  // Load grammar and prepare parser
  grammar = loadGrammar("grammar.txt");
  if (grammar != NULL) {
    firstFollow = ComputeFirstAndFollowSets(grammar);
    createParseTable(firstFollow, parseTable, grammar);
    parserReady = 1;
    return 1;
  } else {
    // Error message already printed by loadGrammar or in driver
    return 0;
  }
}
