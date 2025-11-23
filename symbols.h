#ifndef SYMBOLS_HEADER
#define SYMBOLS_HEADER

// type of a variable/expression
struct Type {
  enum {
    T_INT,
    T_CHAR,
    T_POINTER,
    T_ARRAY,
    T_ENUM,
    T_STRUCT,
    T_UNION,
    T_FUNC
  } kind;

  union {
    struct Type *point;
    struct {
      struct Type *elem_type;
      int len;
    } array;
    struct Enum *enum_type;
    struct Struct *struct_type;
    struct Union *union_type;
    // for function pointers
    struct FuncSig *func_sig;
  };
};

extern char* type_repr[];

struct Var {
  struct Type type;
};

// type and name of field in union or struct
struct Field {
  char *name;
  struct Type type;
  struct Field *next;
};

struct Enum {
  char *name;
};

struct Union {
  char *name;
  struct Field *fields;
};

struct Struct {
  char *name;
  struct Field *fields;
};

struct Param {
  char *name;
  struct Type type;
  struct Param *next;
};

// signature of a function
struct FuncSig {
  struct Type ret;
  struct Param *params;
};

// symbol of identifier
struct Symbol {
  enum {
    S_TYPEDEF,
    S_VAR,
    S_PARAM,
    S_ENUM,
    S_ENUM_CONST,
    S_UNION,
    S_STRUCT,
    S_FUNC
  } kind;

  union {
    struct Type type;
    struct Var *var;
    struct Param *param;
    struct Enum *enum_dec;
    int enum_const;
    struct Union *union_dec;
    struct Struct *struct_dec;
    struct Func *func;
  };

  struct Symbol *next;
};

struct Func {
  char *name;
  struct FuncSig *sig;
  struct BlockStmt *stmt;
};

void new_scope();

void exit_scope();

void add_symbol(char *name, struct Symbol *symbol);

struct Symbol *lookup_name(char *name);

void debug_symbols();

#endif
