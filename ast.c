#include <stdio.h>

#include "ast.h"

int op_precedences[256] = {
    [O_MUL] = 6, [O_DIV] = 6, [O_MOD] = 6,              // multiplicative ops
    [O_ADD] = 5, [O_SUB] = 5,                           // additive ops
    [O_LT] = 4,  [O_GT] = 4,  [O_LTE] = 4, [O_GTE] = 4, // comparisons
    [O_EQ] = 3,  [O_NE] = 3,                            // == !=
    [O_AND] = 2, [O_OR] = 1,                            // logic
};

char *repr[256] = {
    [O_MUL] = "*",  [O_DIV] = "/",    [O_MOD] = "%", [O_ADD] = "+",
    [O_SUB] = "-",  [O_LT] = "<",     [O_GT] = ">",  [O_LTE] = "<=",
    [O_GTE] = ">=", [O_EQ] = "==",    [O_NE] = "!=", [O_AND] = "&&",
    [O_OR] = "||",  [O_ASSIGN] = "=",
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
  case E_GLOBAL:
    printf("%s", expr->global->name);
    break;
  case E_FUNC:
    printf("%s", expr->func->name);
    break;
  case E_UNOP:
    debug_expr_inner(expr->unop.expr, min_precedence);
    break;
  case E_BINOP:
    if (expr->binop.op == O_ASSIGN) {
      debug_expr_inner(expr->binop.l, 100);
      printf(" = ");
      debug_expr_inner(expr->binop.r, 0);
      break;
    } else if (expr->binop.op == O_INDEX) {
      debug_expr_inner(expr->binop.l, 100);
      printf("[");
      debug_expr_inner(expr->binop.r, 0);
      printf("]");
      break;
    }

    if (op_precedences[expr->binop.op] < min_precedence)
      printf("(");

    debug_expr_inner(expr->binop.l, op_precedences[expr->binop.op]);
    printf(" %s ", repr[expr->binop.op]);
    debug_expr_inner(expr->binop.r, op_precedences[expr->binop.op] + 1);

    if (op_precedences[expr->binop.op] < min_precedence)
      printf(")");
    break;
  case E_CALL:
    debug_expr(expr->call.func_expr);

    struct Args *args = expr->call.args;

    printf("(");
    while (args) {
      debug_expr(args->expr);
      args = args->next;

      if (args) {
        printf(", ");
      }
    }
    printf(")");

    break;
  }
}

int indentation = 0;

void debug_expr(struct Expr *expr) { debug_expr_inner(expr, 0); }

void debug_block_stmt(struct BlockStmt *stmt);

void debug_stmt(struct Stmt *stmt) {
  switch (stmt->kind) {
  case S_BLOCK:
    debug_block_stmt(stmt->block);
    break;
  case S_EXPR:
    debug_expr(stmt->expr);
    printf(";");
    break;
  case S_IF:
    printf("if (");
    debug_expr(stmt->if_stmt.cond);
    printf(") ");
    debug_stmt(stmt->if_stmt.if_block);

    if (stmt->if_stmt.else_block) {
      printf(" else ");
      debug_stmt(stmt->if_stmt.else_block);
    }
    break;
  case S_FOR:
    printf("for (");

    if (stmt->for_stmt.init)
      debug_expr(stmt->for_stmt.init);

    printf("; ");

    if (stmt->for_stmt.iter)
      debug_expr(stmt->for_stmt.iter);

    printf("; ");

    if (stmt->for_stmt.cond)
      debug_expr(stmt->for_stmt.cond);

    printf(") ");

    debug_stmt(stmt->for_stmt.block);

    break;
  case S_WHILE:

    printf("while (");

    debug_expr(stmt->while_stmt.cond);

    printf(") ");

    debug_stmt(stmt->while_stmt.block);

    break;
  case S_RETURN:
    if (stmt->expr) {
      printf("return ");
      debug_expr(stmt->expr);
      printf(";");
    } else {
      printf("return;");
    }

    break;
  }
}

void debug_block_stmt(struct BlockStmt *block) {
  if (block == NULL) {
    printf("{ }");
    return;
  }

  printf("{\n");
  indentation++;

  while (block) {
    printf("%*s", indentation * 2, "");
    debug_stmt(block->stmt);
    block = block->next;
    printf("\n");
  }

  printf("}");
}
