#include <stdio.h>

#include "ast.h"
#include "fail.h"

int op_precedences[256] = {
    [O_MUL] = 6, [O_DIV] = 6, [O_MOD] = 6,              // multiplicative ops
    [O_ADD] = 5, [O_SUB] = 5,                           // additive ops
    [O_LT] = 4,  [O_GT] = 4,  [O_LTE] = 4, [O_GTE] = 4, // comparisons
    [O_EQ] = 3,  [O_NE] = 3,                            // == !=
    [O_AND] = 2, [O_OR] = 1,                            // logic
};

char *repr[256] = {
    [O_MUL] = "*",  [O_DIV] = "/", [O_MOD] = "%", [O_ADD] = "+",
    [O_SUB] = "-",  [O_LT] = "<",  [O_GT] = ">",  [O_LTE] = "<=",
    [O_GTE] = ">=", [O_EQ] = "==", [O_NE] = "!=", [O_AND] = "&&",
    [O_OR] = "||",
};

char *unop_repr[256] = {
    [O_DEREF] = "*",
    [O_REF] = "&",
    [O_NOT] = "!",
};

void debug_const(struct Constant *cnst) {
  switch (cnst->kind) {
  case C_INT:
    printf("%d", cnst->int_literal);
    break;
  case C_STR:
    printf("'%c'", cnst->char_literal);
    break;
  case C_CHAR:
    printf("\"%s\"", cnst->str_literal.ptr);
    break;
  }
}

void debug_expr_inner(struct Expr *expr, int min_precedence) {
  switch (expr->kind) {
  case E_CONST:
    debug_const(&expr->cnst);
    break;
  case E_VAR:
    printf("%s", expr->var->name);
    break;
  case E_FUNC:
    printf("%s", expr->func->name);
    break;
  case E_UNOP:
    debug_expr_inner(expr->unop.expr, min_precedence);
    break;
  case E_BINOP:
    if (op_precedences[expr->binop.op] < min_precedence)
      printf("(");

    debug_expr_inner(expr->binop.l, op_precedences[expr->binop.op]);
    printf(" %s ", repr[expr->binop.op]);
    debug_expr_inner(expr->binop.r, op_precedences[expr->binop.op] + 1);

    if (op_precedences[expr->binop.op] < min_precedence)
      printf(")");
    break;
  case E_CALL:
    FAIL;
    break;
  }
}

void debug_expr(struct Expr *expr) {
  debug_expr_inner(expr, 0);
  printf("\n");
}
