#ifndef LISP_AST_H
#define LISP_AST_H

#include "string_view.h"
#include "dynamic_array.h"
#include <stddef.h>

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
    LISP_BUILTIN,
    LISP_LAMBDA,
} LISP_AST_KIND;

typedef struct LispAST LispAST;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

typedef LispAST *(*LispBuiltin) (LispAST *args);

struct LispAST {
    LISP_AST_KIND kind;
    union {
        StringView symbol;
        StringView string;
        LispBuiltin builtin;
        Cons cons;
        int integer;
    } as;
};

typedef struct {
    DA(StringView) symbols;
    DA(LispAST *) values;
} Env;

Env env_init(); 

void env_define(Env *env, StringView name, LispAST *value);

LispAST *env_get(Env *env, StringView name);

#endif
