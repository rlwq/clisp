#ifndef LISP_AST_H
#define LISP_AST_H

#include "string_view.h"
#include "dynamic_array.h"
#include <stdbool.h>
#include <stddef.h>

typedef struct Scope Scope;

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
typedef DA(LispAST *) LispASTPtrDA;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

typedef struct {
    LispASTPtrDA args; 
    LispAST *expr;
    Scope *scope;
} Lambda;

typedef LispAST *(*LispBuiltin) (LispAST *args);

struct LispAST {
    LISP_AST_KIND kind;
    bool marked;
    LispAST *next;
    union {
        StringView symbol;
        StringView string;
        LispBuiltin builtin;
        Lambda lambda;
        Cons cons;
        int integer;
    } as;
};

#define CAR(n_) ((n_)->as.cons.car)
#define CDR(n_) ((n_)->as.cons.cdr)

#endif
