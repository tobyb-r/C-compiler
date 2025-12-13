#include <stdio.h>
#include <string.h>

#include "symbols.h"
#include "fail.h"
#include "types.h"

char *type_repr[] = {
    "int",   "char", "float",  "void",  "pointer",
    "array", "enum", "struct", "union", "function",
};


int type_eq(struct Type *l, struct Type *r) {
  if (l->kind != r->kind) {
    return 1;
  };

  switch (l->kind) {
  case T_INT:
  case T_CHAR:
  case T_FLOAT:
  case T_VOID:
    // builtins match if the enum is the same
    return 1;

  case T_ARRAY:
    if (l->array.len != r->array.len) {
      return 0;
    }
    // fallthrough
  case T_POINTER:
    return type_eq(l->ptr_type, r->ptr_type);

  case T_STRUCT:
  case T_UNION:
  case T_ENUM:
    // should point to same definition
    return l->struct_type == r->struct_type;

  case T_FUNC:
    // check return type is same and parameters are same

    if (type_eq(l->func_sig->ret, r->func_sig->ret)) {
      return 1;
    }

    struct Param *param1 = l->func_sig->params;
    struct Param *param2 = r->func_sig->params;

    while (param1 != NULL && param2 != NULL) {
      if (type_eq(param1->type, param2->type)) {
        return 1;
      }
    }

    // check that both are null
    return param1 == param2;
  }

  FAIL;
  return 0;
}

void debug_type(struct Type *type) {
  switch (type->kind) {
  case T_FUNC:
    printf("fn (");
    struct Param *param = type->func_sig->params;

    while (param != NULL) {
      debug_type(param->type);

      param = param->next;

      if (param) {
        printf(", ");
      }
    }

    printf(") -> ");
    debug_type(type->func_sig->ret);
    break;
  case T_ENUM:
  case T_STRUCT:
  case T_UNION:
    printf("%s ", type_repr[type->kind]);
    if (type->struct_type->name)
      printf("%s", type->struct_type->name);
    else
      printf("anon");
    break;
  case T_POINTER:
    printf("(");
    debug_type(type->ptr_type);
    printf(")*");
    break;
  case T_ARRAY:
    printf("(");
    debug_type(type->array.elem_type);

    printf(")[");

    if (type->array.len != -1) {
      printf("%d", type->array.len);
    }

    printf("]");

    break;
  default:
    printf("%s", type_repr[type->kind]);
  }
}
