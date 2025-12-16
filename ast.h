#ifndef AST_HEADER
#define AST_HEADER

#include "symbols.h"

// AST structs
enum Op {
  O_ASSIGN,
  O_MUL,
  O_DIV,
  O_ADD,
  O_SUB,
  O_MOD,
  O_REF,
  O_DEREF,
  O_EQ,
  O_NE,
  O_LT,
  O_GT,
  O_LTE,
  O_GTE,
  O_NOT,
  O_OR,
  O_AND,
};

// function symbol with FuncSig and ast
struct Func;

struct Expr {
  // TODO: struct/array initializers, constants, field of struct
  enum { E_VAR, E_UNOP, E_BINOP, E_CALL } kind;

  union {
    struct Var *var;

    struct {
      enum Op op;

      // for an assignment the left must be an lvalue
      // could handle assignments differently
      // could handle unary ops differently
      struct Expr *l;
      struct Expr *r; // NULL if expr is UNOP
    };

    struct {
      struct Func *func;
      struct Args *args;
    };
  };
};

// arguments to function call
struct Args {
  struct Expr expr;
  struct Args *next;
};

struct Stmt {
  enum { S_EXPR, S_IF, S_FOR, S_WHILE, S_RETURN } kind;

  union {
    struct {
      struct Expr *init;
      struct Expr *iter;
      struct Expr *test;
      struct BlockStmt *block;
    } for_stmt;

    struct {
      struct Expr *test;
      struct BlockStmt *if_block;
      struct BlockStmt *else_block;
    } if_stmt;

    struct {
      struct Expr *test;
      struct BlockStmt *block;
    } while_stmt;
  };
};

struct BlockStmt {
  struct Stmt stmt;
  struct BlockStmt *next;
};

#endif
