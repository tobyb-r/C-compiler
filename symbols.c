// type for type checking
// contains builtin types and user defined types (typedefs, enums, unions,
// structs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "symbols.h"
#include "types.h"

struct Func *cur_func = NULL;

// TODO: handle partial definitions
char *symbol_repr[] = {
    [S_TYPEDEF] = "typedef", [S_GLOBAL] = "global",         [S_VAR] = "var",
    [S_PARAM] = "param",     [S_ENUM_CONST] = "enum const", [S_FUNC] = "func"};

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
  struct SymDef {
    struct SymDef *next;
    struct Symbol *sym;
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
  struct Scope *new_symbol_scope = malloc(sizeof(*new_symbol_scope));
  *new_symbol_scope = (struct Scope){.table = NULL, .next = symbol_scope};
  symbol_scope = new_symbol_scope;

  struct Scope *new_struct_scope = malloc(sizeof(*new_struct_scope));
  *new_struct_scope = (struct Scope){.table = NULL, .next = struct_scope};
  struct_scope = new_struct_scope;
}

void add_to_scope(struct Scope **scope, struct Table *table) {
  struct Scope *new = malloc(sizeof(*scope));

  new->table = table;
  new->next = *scope;
  *scope = new;
}

// pop all definitions from this last scope
// free scope and def structs
void free_scope(struct Scope **the_scope) {
  struct Scope *scope = *the_scope;
  while (1) {
    if (scope->table == NULL) {
      struct Scope *old = scope;
      *the_scope = scope->next;
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

void free_symbol_scope() {
  struct Scope *scope = symbol_scope;

  while (1) {
    if (scope->table == NULL) {
      struct Scope *old = scope;
      symbol_scope = scope->next;
      free(old);
      break;
    }

    struct Table *table = scope->table;
    struct SymDef *old_d = (void *)table->def;
    table->def = table->def->next;

    // TODO could free symbol data if it is unused
    // could use refcounting
    free(old_d->sym);
    free(old_d);

    struct Scope *old = scope;
    scope = scope->next;
    free(old);
  }
}

void free_struct_scope() { free_scope(&struct_scope); }

// exit a scope
void exit_scope() {
  if (symbol_scope == NULL) {
    printf("Compiler error\n");
    FAIL;
  }

  free_symbol_scope();
  free_struct_scope();
}

// finds if name is in current scope
struct Def *find_in_scope(char *name, struct Scope *scope) {
  if (scope == NULL) {
    return NULL;
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

// lookup symbol in symbol table
struct Symbol *lookup_symbol(char *name) {
  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table && table->def)
    return table->def->sym;

  return NULL;
}

// lookup struct in struct table
struct Struct *lookup_struct(char *name) {
  struct StructTable *table = (void *)find_in_table(name, (void *)struct_table);

  if (table && table->def)
    return table->def->struc;

  return NULL;
}

// define a new symbol
// use other functions for defining functions/globals
// always returns pointer to new symbol
struct Symbol *add_symbol(char *name) {
  if (symbol_scope) {
    if (find_in_scope(name, symbol_scope)) {
      printf("Semantic error: redefining %s in same scope\n", name);
      FAIL;
    }
  } else {
    struct SymbolTable *table =
        (void *)find_in_table(name, (void *)symbol_table);

    if (table && table->def) {
      printf("Semantic error: redefining %s\n", name);
      FAIL;
    }
  }

  struct SymDef *def = calloc(1, sizeof(*def));

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

  if (symbol_scope)
    add_to_scope(&symbol_scope, (void *)table);

  def->sym = calloc(1, sizeof(*def->sym));

  return def->sym;
}

// define global
// can return pointer to incomplete definition
struct Global *add_global(char *name) {
  // globals can only be defined outside of scope
  if (symbol_scope)
    FAIL;

  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table) {
    if (table->def) {
      if (table->def->sym->kind == S_GLOBAL) {
        return table->def->sym->global;
      } else {
        printf("Semantic error: redefining symbol %s as global\n", name);
        FAIL;
      }
    }
  } else {
    table = malloc(sizeof(*table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->next = symbol_table;
    symbol_table = table;
  }

  struct SymDef *def = calloc(1, sizeof(*def));
  def->next = table->def;
  table->def = def;

  def->sym = calloc(1, sizeof(*def->sym));
  def->sym->kind = S_GLOBAL;
  def->sym->global = calloc(1, sizeof(*def->sym->global));
  def->sym->global->name = malloc(strlen(name) + 1);
  strcpy(def->sym->global->name, name);

  return def->sym->global;
}

// add a local variable
struct Var *add_local(char *name, struct Type *type) {
  struct Symbol *sym = add_symbol(name);
  sym->kind = S_VAR;
  sym->var = malloc(sizeof(*sym->var));
  sym->var->name = malloc(strlen(name));
  strcpy(sym->var->name, name);
  sym->var->type = type;
  return sym->var;
}

// define a new struct
// can return pointer to incomplete definition
struct Struct *add_struct(char *name) {
  // previous definition if it exists
  // if it is in an outer scope we shadow instead of completing
  struct StDef *prev_def = (void *)find_in_scope(name, struct_scope);

  if (prev_def)
    return prev_def->struc;

  struct StDef *def = malloc(sizeof(*def));
  def->struc = calloc(1, sizeof(*def->struc));
  def->struc->name = malloc(strlen(name) + 1);
  strcpy(def->struc->name, name);

  struct StructTable *table = (void *)find_in_table(name, (void *)struct_table);

  if (table) {
    def->next = table->def;
    table->def = def;
    if (struct_scope)
      add_to_scope(&struct_scope, (void *)table);
  } else {
    def->next = NULL;

    table = malloc(sizeof(*table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->def = def;
    table->next = struct_table;
    struct_table = table;
    if (struct_scope)
      add_to_scope(&struct_scope, (void *)table);
  }

  return def->struc;
}

// define a new function
// can return pointer to incomplete definition
struct Func *add_func(char *name) {
  // functions can only be defined outside of scope
  if (symbol_scope)
    FAIL;

  struct SymbolTable *table = (void *)find_in_table(name, (void *)symbol_table);

  if (table) {
    if (table->def && table->def->sym->kind == S_FUNC) {
      return table->def->sym->func;
    } else {
      printf("Semantic error: redefining symbol %s as function\n", name);
      FAIL;
    }
  }

  if (table) {
    if (table->def) {
      if (table->def->sym->kind == S_FUNC) {
        return table->def->sym->func;
      } else {
        printf("Semantic error: redefining symbol %s as global\n", name);
        FAIL;
      }
    }
  } else {
    table = malloc(sizeof(*table));
    table->name = malloc(strlen(name) + 1);
    strcpy(table->name, name);
    table->next = symbol_table;
    symbol_table = table;
  }

  struct SymDef *def = calloc(1, sizeof(*def));
  def->next = table->def;
  table->def = def;

  def->sym = calloc(1, sizeof(*def->sym));
  def->sym->kind = S_FUNC;
  def->sym->func = calloc(1, sizeof(*def->sym->func));
  def->sym->func->name = malloc(strlen(name) + 1);
  strcpy(def->sym->func->name, name);

  return def->sym->func;
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
    printf("[%s]", symbol_repr[symbol->kind]);
  }

  printf("\n");
}

void debug_symbols() {
  // walk through and print symbols associated with identifiers
  printf("Symbols are:\n");

  struct SymbolTable *sym_entry = symbol_table;

  while (sym_entry) {
    struct SymDef *def = sym_entry->def;

    while (def) {
      printf("- %s\n  ", sym_entry->name);

      debug_symbol(sym_entry->def->sym);
      def = def->next;
    }

    sym_entry = sym_entry->next;
  }

  printf("Structs are:\n");

  struct StructTable *st_entry = struct_table;

  while (st_entry) {
    struct StDef *def = st_entry->def;

    while (def) {
      printf("- %s\n", st_entry->name);

      struct Field *field = (st_entry->def->struc)->fields;

      while (field != NULL) {
        printf("    ");
        debug_type(field->type);

        if (field->name != NULL) {
          printf(" %s\n", field->name);
        } else {
          printf(" anon\n");
        }

        field = field->next;
      }

      def = def->next;
    }
    st_entry = st_entry->next;
  }
}
