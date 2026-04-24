#include "evaluator.h"
#include "lisp_ast.h"
#include "string_view.h"
#include "utils.h"
#include <assert.h>

// <list> := NIL | cons(<expr>, <list>)

LispAST *lisp_eval_list(LispAST *expr, Env *env) {
    // TODO: Warning! Mutates state!!! Should copy and overwrite
    for (LispAST *curr_arg = expr;
         curr_arg->kind != LISP_NIL;
         curr_arg = curr_arg->as.cons.cdr)
        curr_arg->as.cons.car = lisp_eval(curr_arg->as.cons.car, env);
    return expr;
}

// ((lambda) a b c)
LispAST *lisp_lamda_call(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_CONS);
    assert(expr->as.cons.car->kind == LISP_LAMBDA);

    Env *new_env = malloc(sizeof(Env));
    env_init(new_env);

    // TODO: iterate through lambda paramters and expr and define them in new_env
    // TODO: eval lambda's sub expr in the new_env
    // TODO: free new_env
    NOT_IMPLEMENTED();
}

LispAST *lisp_eval_let_expr(LispAST *args, Env *env) {
    assert(args->kind == LISP_CONS);
    assert(args->as.cons.car->kind == LISP_SYMBOL);
    //TODO: maybe should copy the value
    env_define(env, args->as.cons.car->as.symbol, lisp_eval(args->as.cons.cdr->as.cons.car, env));
    return args;
}

LispAST *lisp_eval_sexpr(LispAST *expr, Env *env) {
    assert(expr->kind == LISP_CONS);

    LispAST *head = expr->as.cons.car; 
    LispAST *args = expr->as.cons.cdr; 
 
    if (head->kind == LISP_SYMBOL) {
        // TODO: sv_eq to something else
        if (sv_eq(head->as.symbol, sv_mk("if"))) {
            NOT_IMPLEMENTED();
            return expr;
        }
        else if (sv_eq(head->as.symbol, sv_mk("let"))) {
            lisp_eval_let_expr(args, env);
            return expr;
        }
    }

    LispAST *evaluated_head = lisp_eval(head, env);
    LispAST *evaluated_args = lisp_eval_list(args, env);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;

        case LISP_BUILTIN:
            return evaluated_head->as.builtin(evaluated_args);
        break;

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
            assert(0 && "Can't call this kind of object.");
        break;
    }

    UNREACHABLE();
}

// TODO: maybe should always return a new AST
LispAST *lisp_eval(LispAST *expr, Env *env) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_LAMBDA:
        case LISP_BUILTIN:
            return expr;
        break;

        case LISP_SYMBOL:
            return env_get(env, expr);
        break;

        case LISP_CONS:
            return lisp_eval_sexpr(expr, env);
        break;
    }
    
    UNREACHABLE();
}
