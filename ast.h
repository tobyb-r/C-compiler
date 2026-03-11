#ifndef AST_HEADER
#define AST_HEADER

#include "symbols.h"
#include "types.h"

// AST structs

// int, string or char literal
struct Constant {
  enum { C_INT, C_STR, C_CHAR } kind;

  union {
    int int_literal;
    char char_literal;

    struct {
      char *ptr;
      int strlen;
    } str_literal;
  };
};

enum UnOp {
  O_DEREF, // *
  O_REF,   // &
  O_NOT,   // !
};

enum BinOp {
  O_ASSIGN, // a = 1
  O_INDEX,  // a[1]

  // arithmetic
  O_MUL,
  O_DIV,
  O_ADD,
  O_SUB,
  O_MOD,

  // comparisons
  O_EQ,
  O_NE,
  O_LT,
  O_GT,
  O_LTE,
  O_GTE,

  // logic
  O_OR,
  O_AND,
};

struct Expr {
  // TODO: struct/array initializers, constants, field of struct
  enum { E_CONST, E_GLOBAL, E_VAR, E_FUNC, E_UNOP, E_BINOP, E_CALL } kind;

  union {
    struct Constant cnst;

    struct Var *var;

    struct Global *global;

    struct Func *func;

    struct {
      enum BinOp op;

      // for an assignment the left must be an lvalue
      struct Expr *l;
      struct Expr *r;
    } binop;

    struct {
      enum UnOp op;
      struct Expr *expr;
    } unop;

    // function call
    struct {
      struct Expr *func_expr;
      struct Args *args;
    } call;
  };
};

// arguments to function call
struct Args {
  struct Expr *expr;
  struct Args *next;
};

struct Stmt {
  enum { S_BLOCK, S_EXPR, S_IF, S_FOR, S_WHILE, S_RETURN } kind;

  union {
    struct Expr *expr;

    struct {
      struct Expr *init;
      struct Expr *iter;
      struct Expr *cond;
      struct Stmt *block;
    } for_stmt;

    struct {
      struct Expr *cond;
      struct Stmt *if_block;
      struct Stmt *else_block;
    } if_stmt;

    struct {
      struct Expr *cond;
      struct Stmt *block;
    } while_stmt;

    struct BlockStmt *block;
  };
};

struct BlockStmt {
  struct Stmt *stmt;
  struct BlockStmt *next;
};

void debug_expr(struct Expr *expr);
void debug_block_stmt(struct BlockStmt *expr);
void debug_stmt(struct Stmt *stmt);

#endif
