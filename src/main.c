#include "string_view.h"
#include "tokenizer.h"
#include "parser.h"
#include "lisp_ast.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void print_expr(LispAST *expr) {
    switch (expr->kind) {
        case LISP_NIL: printf("NIL"); break;
        case LISP_INTEGER: printf("%d", expr->as.integer); break;
        case LISP_STRING: printf("\""SV_FMT"\"", SV_ARGS(expr->as.string)); break;
        case LISP_SYMBOL: printf(SV_FMT, SV_ARGS(expr->as.symbol)); break;
        case LISP_CONS:
            printf("<");
            print_expr(expr->as.cons.car);
            printf("; ");
            print_expr(expr->as.cons.cdr);
            printf(">");
        break;
        case LISP_BUILTIN:
            assert(0 && "Not implemented.");
        break;
        case LISP_LAMBDA:
            assert(0 && "Not implemented.");
        break;
        default: assert(0 && "Unreachable"); break;
    }
}

LispAST *eval(LispAST *expr, Env *env) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
            return expr;
        break;
        case LISP_SYMBOL:
            return env_get(env, expr->as.symbol);
        break;
        case LISP_CONS: {
            LispAST *func = eval(expr->as.cons.car, env);
            LispAST *args = expr->as.cons.cdr;
           
            // TODO: control here if args should or should not be evaluated
            for (LispAST *curr_arg = args;
                 curr_arg->kind != LISP_NIL;
                 curr_arg = curr_arg->as.cons.cdr)
                curr_arg->as.cons.car = eval(curr_arg->as.cons.car, env);


            switch (func->kind) {
                case LISP_BUILTIN: {
                    LispAST *result = func->as.builtin(args);
                    return result;
                } break;
                case LISP_LAMBDA:
                    assert(0 && "Not implemented.");
                break;
                default:
                    assert(0 && "Uncallable object.");
                break;
            }
        } break;
        case LISP_BUILTIN:
            assert(0 && "Not implemented.");
        break;
        case LISP_LAMBDA:
            assert(0 && "Not implemented.");
        break;
 
        default:
            assert(0 && "Unreachable");
        break;
    }

    assert(0 && "Unreachable");
    return NULL;
}

LispAST *lisp_add(LispAST *args) {
    int result_value = 0;

    for (; args->kind != LISP_NIL; args = args->as.cons.cdr)
        result_value += args->as.cons.car->as.integer;

    LispAST *result = malloc(sizeof(LispAST));
    result->kind = LISP_INTEGER;
    result->as.integer = result_value;
    return result;
}

int main() {
    char buff[2048];

    Env env = env_init();

    LispAST *add_func = malloc(sizeof(LispAST));
    add_func->kind = LISP_BUILTIN;
    add_func->as.builtin = lisp_add;

    env_define(&env, sv_mk("add"), add_func);

    while (true) {
        if (!fgets(buff, sizeof(buff), stdin)) break;

        StringView prog = sv_mk(buff);
        Tokenizer t = tokenizer_init(prog);
        tokenize(&t);
        
        Parser p = parser_init(t.tokens);

        LispAST *result = parse_expr(&p);
        LispAST *r2 = eval(result, &env);
        print_expr(result);
        printf("\n");
        print_expr(r2);
        printf("\n");
    }
    return 0;
}

