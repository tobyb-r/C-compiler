// type for type checking
// contains builtin types and user defined types (typedefs, enums, unions,
// structs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "symbols.h"

char* type_repr[] = {
  "int",
  "char",
  "pointer",
  "array",
  "enum",
  "struct",
  "union",
  "function",
};

struct SymbolScope {
  char sentinel;
  struct SymbolTable *table;
  struct SymbolScope *next;
} *symbol_scope;

struct SymbolTable {
  char *ident;

  struct Symbol *symbol;

  struct SymbolTable *next;
  // linked list of identifiers and lists of symbols
} *symbol_table;

void new_scope() {
  struct SymbolScope new =
      (struct SymbolScope){.table = NULL, .sentinel = 1, .next = symbol_scope};

  symbol_scope = malloc(sizeof(*symbol_scope));

  *symbol_scope = new;
}

void exit_scope() {
  if (symbol_scope == NULL) {
    printf("Compiler error: tried to exit scope when not in scope\n");
    fail(6);
  }

  return;

  while (1) {
    if (symbol_scope->sentinel == 1) {
      struct SymbolScope *old = symbol_scope;

      symbol_scope = symbol_scope->next;
      free(old);

      break;
    }

    struct SymbolTable *table = symbol_scope->table;
    struct Symbol *old = table->symbol;

    table->symbol = table->symbol->next;
    free(old);

    // could free unused tables

    struct SymbolScope *oldscope = symbol_scope;

    symbol_scope = symbol_scope->next;
    free(oldscope);
  }
}

void add_symbol(char *name, struct Symbol *symbol) {
  struct SymbolTable *table;

  // just add to table
  struct SymbolTable *entry = symbol_table;

  // iterate through symbol table
  while (1) {
    if (entry == NULL) {
      // couldn't find entry for this identifier so we make a new entry
      struct SymbolTable *new = malloc(sizeof(struct SymbolTable));
      new->ident = malloc(strlen(name) + 1);
      strcpy(new->ident, name);
      new->next = symbol_table;
      new->symbol = symbol;
      symbol_table = new;

      table = symbol_table;

      break;
    }

    if (!strcmp(entry->ident, name)) {
      // found entry for this identifier
      symbol->next = entry->symbol;
      entry->symbol = symbol;

      table = entry;

      break;
    }

    entry = entry->next;
  }

  // if we are in a scope we need to remove the identifier after we leave
  if (symbol_scope != NULL) {
    // append to symbol scope
    struct SymbolScope new =
        (struct SymbolScope){.table = table, .next = symbol_scope};

    symbol_scope = malloc(sizeof(*symbol_scope));

    *symbol_scope = new;
  }
}

struct Symbol *lookup_name(char *name) {
  // find id in symbol scope

  struct SymbolTable *entry = symbol_table;

  while (1) {
    // couldn't find identifier
    if (entry == NULL) {
      return NULL;
    }

    if (!strcmp(entry->ident, name)) {
      return entry->symbol;
    }

    entry = entry->next;
  }
}

void debug_symbols() {
  // walk through and print symbols associated with identifiers
  printf("Symbols are:\n");
  struct SymbolTable *entry = symbol_table;

  while (entry != NULL) {
    printf("- %s\n", entry->ident);

    if (entry->symbol != NULL) {
      if (entry->symbol->kind == S_VAR) {
        printf("  Variable: %s\n", type_repr[entry->symbol->var->type.kind]);
      } else if (entry->symbol->kind == S_FUNC) {
        printf("  Function: ");

        struct Param *param = entry->symbol->func->sig->params;

        while (param != NULL) {
          printf("%s ", type_repr[param->type.kind]);
          param = param->next;
        }
        printf("-> %s\n", type_repr[entry->symbol->func->sig->ret.kind]);
      } else {
        printf("%s\n", type_repr[entry->symbol->kind]);
      }
    }
    entry = entry->next;
  }
}
