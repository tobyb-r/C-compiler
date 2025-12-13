#ifndef SYMBOLS_HEADER
#define SYMBOLS_HEADER

struct Var {
  struct Type *type;
};

struct Global {
  struct Type *type;
  // initialization value
};

struct Enum {
  char *name;
};

// same layout as Struct
struct Union {
  char *name; // NULL for anonymous union
  struct Field *fields;
  // size, alignment
};

// same layout as Union
struct Struct {
  char *name; // NULL for anonymous struct
  struct Field *fields;
  // size, alignment
};

// type and name of field in union or struct
struct Field {
  char *name;
  struct Field *next;
  struct Type *type;
};

// signature of a function
struct FuncSig {
  struct Type *ret;
  struct Param *params;
};

struct Param {
  char *name;
  struct Type *type;
  struct Param *next;
};

// anything that can be represented by an identifier
// TODO: goto labels
struct Symbol {
  enum { S_TYPEDEF, S_GLOBAL, S_VAR, S_PARAM, S_ENUM_CONST, S_FUNC } kind;

  union {
    struct Type *type;
    struct Global *global;
    struct Var *var;
    struct Param *param;
    int enum_const;
    struct Func *func;
  };
};

struct Func {
  char *name;
  struct FuncSig *sig;
  struct BlockStmt *stmt;
  // stack size, layout
};

void new_scope();
void exit_scope();

void debug_type(struct Type *type);
void debug_symbol(struct Symbol *symbol);
void debug_symbols();

struct Symbol *add_symbol(char *name, struct Symbol symbol);
struct Struct *add_struct(char *name, struct Struct struct_def);
struct Func *add_func(char *name, struct Func func);

struct Symbol *lookup_symbol(char *name);
struct Struct *lookup_struct(char *name);

#endif
