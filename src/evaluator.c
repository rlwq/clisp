#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "scope.h"
#include "evaluator.h"
#include "dynamic_array.h"
#include "lisp_node.h"
#include "string_view.h"
#include "forwards.h"
#include "utils.h"

#define CURR(e_) (*((e_)->stmts))

Evaluator *evaluator_alloc(LispNodePtrDA exprs, GC *gc) {
    Evaluator *evaluator = malloc(sizeof(Evaluator));
    assert(evaluator);

    evaluator->stmts = exprs.data;
    evaluator->stmts_count = exprs.size;
    evaluator->is_err = false;

    evaluator->gc = gc;

    da_init(evaluator->results);
    da_init(evaluator->scope_stack);

    return evaluator;
}

void evaluator_free(Evaluator *evaluator) {
    da_free(evaluator->scope_stack);

    free(evaluator);
}

void push_scope(Evaluator *evaluator, Scope *scope) {
    da_push(evaluator->scope_stack, scope);
}

void pop_scope(Evaluator *evaluator) {
    da_pop(evaluator->scope_stack);
}

void evaluator_mark(Evaluator *evaluator) {
    // Marking unevaluated expressions
    for (size_t i = 0; i < evaluator->stmts_count; i++)
        gc_mark_node(evaluator->stmts[i]);

    // Marking results
    for (size_t i = 0; i < evaluator->results.size; i++)
        gc_mark_node(da_at(evaluator->results, i));

    // Marking scopes
    for (size_t i = 0; i < evaluator->scope_stack.size; i++)
        gc_mark_scope(da_at(evaluator->scope_stack, i));
}

LispNode *evaluator_advance(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));
    LispNode *curr = CURR(evaluator);
    
    evaluator->stmts++;
    evaluator->stmts_count--;
    return curr;
}

LispNode *eval_expr(LispNode *expr, Evaluator *evaluator);

LispNode *eval_list(LispNode *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);

    // TODO: make an iterative approach
    if (expr->kind == LISP_NIL) return expr;
    if (expr->kind == LISP_CONS) {
        LispNode *node = gc_alloc_node(evaluator->gc, LISP_CONS);
        node->as.cons.car = eval_expr(expr->as.cons.car, evaluator);
        node->as.cons.cdr = eval_list(expr->as.cons.cdr, evaluator);
        return node;
    }

    UNREACHABLE();
}

void eval_current(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));
    
    LispNode *stmt = evaluator_advance(evaluator);
    LispNode *node = eval_expr(stmt, evaluator);

    if (node) da_push(evaluator->results, node);
}

void register_builtin(Evaluator *evaluator, StringView name, LispBuiltin func_ptr) {
    LispNode *builtin = gc_alloc_node(evaluator->gc, LISP_BUILTIN);
    builtin->as.builtin = func_ptr;

    scope_define(CURR_SCOPE(evaluator), name, builtin);
}

void eval_all(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));

    while (EVALUATOR_VALID(evaluator)) {
        eval_current(evaluator);
        evaluator_mark(evaluator);

        gc_sweep(evaluator->gc);
    }
}

LispNode *eval_let_form(LispNode *symbol, LispNode *expr, Evaluator *evaluator) {
    assert(symbol->kind == LISP_SYMBOL);
    LispNode *node = eval_expr(expr, evaluator);
    scope_define(CURR_SCOPE(evaluator), symbol->as.symbol, node);
    return node;
}

LispNode *eval_if_form(LispNode *condition, LispNode *if_true, LispNode *if_false, Evaluator *evaluator) {
    LispNode *expr_result = eval_expr(condition, evaluator);
    if (expr_result->kind == LISP_NIL)
        return eval_expr(if_false, evaluator); 
    return eval_expr(if_true, evaluator);
}

LispNode *eval_lambda_form(StringViewDA args, LispNode *subexpr, Evaluator *evaluator) {
    LispNode *lambda_result = gc_alloc_node(evaluator->gc, LISP_LAMBDA);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.scope = CURR_SCOPE(evaluator);

    return lambda_result;
}

LispNode *eval_lambda_call(LispNode *lambda, LispNode *args, Evaluator *evaluator) {
    push_scope(evaluator, lambda->as.lambda.scope);
    push_scope(evaluator, gc_alloc_scope(evaluator->gc, CURR_SCOPE(evaluator)));

    for (size_t i = 0; args->kind != LISP_NIL; args = args->as.cons.cdr) {
        scope_define(CURR_SCOPE(evaluator), da_at(lambda->as.lambda.args, i),
                   args->as.cons.car);
        i++;
    }

    LispNode *result = eval_expr(lambda->as.lambda.expr, evaluator);
    pop_scope(evaluator);
    pop_scope(evaluator);
    return result;
}

LispNode *dispatch_special_form(LispNode *head, LispNode *args, Evaluator *evaluator) {
    if (head->kind != LISP_SYMBOL) return NULL;

    if (sv_eq(head->as.symbol, sv_mk("if"))) {
        //TODO: make proper assertions
        LispNode *condition = CAR(args);
        LispNode *if_true = CAR(CDR(args));
        LispNode *if_false = CAR(CDR(CDR(args)));

        return eval_if_form(condition, if_true, if_false, evaluator);
    }

    if (sv_eq(head->as.symbol, sv_mk("let"))) {
        assert(args->kind == LISP_CONS);

        LispNode *symbol = args->as.cons.car;
        LispNode *value_cons = args->as.cons.cdr;

        assert(symbol->kind == LISP_SYMBOL);
        assert(value_cons->kind == LISP_CONS);
        assert(value_cons->as.cons.cdr->kind == LISP_NIL);

        return eval_let_form(symbol, value_cons->as.cons.car, evaluator);
    }

    if (sv_eq(head->as.symbol, sv_mk("lambda"))) {
        // TODO: make proper assertions
        LispNode *lambda_args_list = CAR(args);
        LispNode *lambda_subexpr = CAR(CDR(args));

        StringViewDA lambda_args;
        da_init(lambda_args);

        for (; lambda_args_list->kind != LISP_NIL;
               lambda_args_list = lambda_args_list->as.cons.cdr)
            da_push(lambda_args, CAR(lambda_args_list)->as.symbol);
        LispNode *result = eval_lambda_form(lambda_args, lambda_subexpr, evaluator);
        return result;
    }

    return NULL;
}

LispNode *eval_cons(LispNode *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_CONS);

    LispNode *head = expr->as.cons.car; 
    LispNode *args = expr->as.cons.cdr; 

    LispNode *special_form = dispatch_special_form(head, args, evaluator);

    if (special_form) return special_form;

    LispNode *evaluated_head = eval_expr(head, evaluator);
    LispNode *evaluated_args = eval_list(args, evaluator);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            return eval_lambda_call(evaluated_head, evaluated_args, evaluator);
        break;

        case LISP_BUILTIN:
            return evaluated_head->as.builtin(evaluated_args, evaluator->gc);
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

LispNode *eval_expr(LispNode *expr, Evaluator *evaluator) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            return expr;
        break;

        case LISP_SYMBOL:
            if (sv_eq(expr->as.symbol, sv_mk("NIL")))
                return gc_alloc_node(evaluator->gc, LISP_NIL);
            return scope_get(CURR_SCOPE(evaluator), expr->as.symbol);
        break;

        case LISP_CONS:
            return eval_cons(expr, evaluator);
        break;
    }
    
    UNREACHABLE();
}

LispNodePtrDA extract_results(Evaluator *evaluator) {
    LispNodePtrDA results = evaluator->results;
    da_nullify(evaluator->results);
    return results;
}

