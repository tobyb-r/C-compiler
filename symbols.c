// type for type checking
// contains builtin types and user defined types (typedefs, enums, unions,
// structs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "symbols.h"
#include "types.h"

// TODO: handle partial definitions

// parent table type
struct Table {
  struct Table *next;
  char *name;
  struct Def {
    struct Def *next;
  } *def;
};

struct SymbolTable {
  struct SymbolTable *next;
  char *name;
  struct SDef {
    struct SDef *next;
    struct Symbol sym;
  } *def;
} *symbol_table;

struct StructTable {
  struct StructTable *next;
  char *name;
  struct StDef {
    struct StDef *next;
    struct Struct *struc;
  } *def;
} *struct_table;

struct Scope {
  struct Scope *next;
  struct Table *table;
};

struct Scope *struct_scope = NULL;
struct Scope *symbol_scope = NULL;

void new_scope() {
  symbol_scope = malloc(sizeof(*symbol_scope));
  *symbol_scope = (struct Scope){.table = NULL, .next = symbol_scope};

  struct_scope = malloc(sizeof(*struct_scope));
  *struct_scope = (struct Scope){.table = NULL, .next = struct_scope};
}

// pop all definitions from this last scope
// free scope and def structs
void free_scope(struct Scope *scope) {
  while (1) {
    if (scope->table == NULL) {
      struct Scope *old = scope;
      scope = scope->next;
      free(old);
      break;
    }

    struct Table *table = scope->table;
    struct Def *old_d = table->def;

    table->def = table->def->next;

    free(old_d);

    struct Scope *old = scope;
    scope = scope->next;
    free(old);
  }
}

// exit a scope
void exit_scope() {
  if (symbol_scope == NULL) {
    printf("Compiler error\n");
    FAIL;
  }

  free_scope(symbol_scope);
  free_scope(struct_scope);
}

// finds if name is in current scope
struct Def *find_in_scope(char *name, struct Scope *scope) {
  if (scope == NULL) {
    FAIL;
  }

  while (scope->table != NULL) {
    if (!strcmp(name, scope->table->name)) {
      return scope->table->def;
    }

    scope = scope->next;
  }

  return NULL;
}

// find entry for identifier in table
struct Table *find_in_table(char *name, struct Table *table) {
  while (table != NULL) {
    if (!strcmp(name, table->name)) {
      return table;
    }

    table = table->next;
  }

  return NULL;
}

struct Symbol *lookup_symbol(char *name) {
  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table && table->def)
    return &table->def->sym;

  return NULL;
}

struct Struct *lookup_struct(char *name) {
  struct StructTable *table = (void *)find_in_table(name, (void *)struct_table);

  if (table && table->def)
    return table->def->struc;

  return NULL;
}

struct Symbol *add_symbol(char *name, struct Symbol symbol) {
  if (symbol_scope) {
    if (find_in_scope(name, symbol_scope)) {
      printf("Semantic error: Shadowing %s in same scope\n", name);
      FAIL;
    }
  }

  struct SDef *def = malloc(sizeof(*def));
  def->sym = symbol;

  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table) {
    def->next = table->def;
    table->def = def;
  } else {
    def->next = NULL;

    table = malloc(sizeof(*table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->def = def;
    table->next = symbol_table;
    symbol_table = table;
  }

  return &def->sym;
}

// may refer to old definition
// returns ptr to the definition
struct Struct *add_struct(char *name, struct Struct struct_def) {
  struct StDef *prev_def = NULL;

  // previous definition it it exists and is valid to complete
  // if it is in an outer scope we shadow instead of completing
  if (struct_scope) {
    prev_def = (void *)find_in_scope(name, struct_scope);
  }

  if (prev_def) {
    // TODO: forward definitions
    printf("Semantic error: Redefining struct %s\n", name);
    FAIL;
  }

  struct StDef *def = malloc(sizeof(*def));
  def->struc = malloc(sizeof(struct Struct));
  *def->struc = struct_def;

  struct StructTable *table = (void *)find_in_table(name, (void *)struct_table);

  if (table) {
    def->next = table->def;
    table->def = def;
  } else {
    def->next = NULL;

    table = malloc(sizeof(*table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->def = def;
    table->next = struct_table;
    struct_table = table;
  }

  return def->struc;
}

// may refer to old definition
// returns ptr to the definition
struct Func *add_func(char *name, struct Func func) {
  // functions can only be defined outside of scope
  // if the function has a forward definition ret and params must match
  if (symbol_scope) {
    FAIL;
  }

  struct SDef *def = malloc(sizeof(*def));
  def->next = NULL;
  def->sym.kind = S_FUNC;
  def->sym.func = malloc(sizeof(*def->sym.func));
  *def->sym.func = func;

  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table) {
    // TODO forward definitions
    printf("Semantic error: Redefining symbol %s\n", name);
    FAIL;
  } else {
    table = malloc(sizeof(struct Table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->def = def;
    table->next = symbol_table;
    symbol_table = table;
  }

  return def->sym.func;
}

void debug_symbol(struct Symbol *symbol) {
  if (symbol == NULL) {
    printf("NULL\n");
    return;
  }

  switch (symbol->kind) {
  case S_TYPEDEF:
    printf("Typedef: ");
    debug_type(symbol->type);
    break;
  case S_GLOBAL:
    printf("Global: ");
    debug_type(symbol->global->type);
    break;
  case S_VAR:
    printf("Variable: ");
    debug_type(symbol->var->type);
    break;
  case S_PARAM:
    printf("Param: ");
    debug_type(symbol->param->type);
    break;
  case S_FUNC:
    printf("Function: (");

    struct Param *param = symbol->func->sig->params;

    while (param != NULL) {
      debug_type(param->type);

      param = param->next;

      if (param) {
        printf(", ");
      }
    }

    printf(") -> ");
    debug_type(symbol->func->sig->ret);
    break;
  default:
    printf("[%d]", symbol->kind);
  }

  printf("\n");
}

void debug_symbols() {
  // walk through and print symbols associated with identifiers
  printf("Symbols are:\n");

  struct SymbolTable *entry = symbol_table;

  while (entry != NULL) {
    printf("- %s\n  ", entry->name);

    debug_symbol(&entry->def->sym);

    entry = entry->next;
  }

  printf("Structs are:\n");

  struct StructTable *sentry = struct_table;

  while (sentry != NULL) {
    printf("- %s\n", sentry->name);

    struct Field *field = (sentry->def->struc)->fields;

    while (field != NULL) {
      printf("  ");
      debug_type(field->type);

      if (field->name != NULL) {
        printf(" %s\n", field->name);
      } else {
        printf(" anon\n");
      }

      field = field->next;
    }

    break;
    sentry = sentry->next;
  }
}
