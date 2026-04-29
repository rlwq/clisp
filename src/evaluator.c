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
    da_init(evaluator->value_stack);

    return evaluator;
}

void evaluator_free(Evaluator *evaluator) {
    da_free(evaluator->scope_stack);

    free(evaluator);
}

void evaluator_push_scope(Evaluator *evaluator, Scope *scope) {
    da_push(evaluator->scope_stack, scope);
}

void evaluator_pop_scope(Evaluator *evaluator) {
    assert(evaluator->scope_stack.size);
    da_pop(evaluator->scope_stack);
}

// 0 -> 1
void evaluator_push_value(Evaluator *evaluator, LispNode *value) {
    da_push(evaluator->value_stack, value);
}

// 1 -> 0
LispNode *evaluator_pop_value(Evaluator *evaluator) {
    assert(evaluator->value_stack.size);

    LispNode *curr = da_at(evaluator->value_stack, evaluator->value_stack.size-1);
    da_pop(evaluator->value_stack);
    return curr;
}

LispNode *evaluator_peek_value(Evaluator *evaluator) {
    assert(evaluator->value_stack.size);

    return da_at(evaluator->value_stack, evaluator->value_stack.size-1);
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

    // Marking value stack
    for (size_t i = 0; i < evaluator->value_stack.size; i++)
        gc_mark_node(da_at(evaluator->value_stack, i));
}

LispNode *evaluator_advance(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));
    LispNode *curr = CURR(evaluator);
    
    evaluator->stmts++;
    evaluator->stmts_count--;
    return curr;
}

void eval_expr(LispNode *expr, Evaluator *evaluator);

size_t eval_list(LispNode *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_NIL || expr->kind == LISP_CONS);
    
    size_t size = 0;
    for (LispNode *curr = expr; curr->kind != LISP_NIL; curr = CDR(curr)) {
        assert(curr->kind == LISP_CONS);
        eval_expr(CAR(curr), evaluator);
        size++;
    }
    return size;
}

void eval_current(Evaluator *evaluator) {
    assert(EVALUATOR_VALID(evaluator));
    
    LispNode *stmt = evaluator_advance(evaluator);
    eval_expr(stmt, evaluator);
    LispNode *node = evaluator_pop_value(evaluator);

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

void eval_let_form(LispNode *symbol, LispNode *expr, Evaluator *evaluator) {
    assert(symbol->kind == LISP_SYMBOL);
    eval_expr(expr, evaluator);

    scope_define(CURR_SCOPE(evaluator), symbol->as.symbol, evaluator_peek_value(evaluator));
}

void eval_if_form(LispNode *condition, LispNode *if_true, LispNode *if_false, Evaluator *evaluator) {
    eval_expr(condition, evaluator);
    if (evaluator_pop_value(evaluator)->kind == LISP_NIL) {
        eval_expr(if_false, evaluator); 
        return;
    }
    eval_expr(if_true, evaluator);
}

void eval_lambda_form(StringViewDA args, LispNode *subexpr, Evaluator *evaluator) {
    LispNode *lambda_result = gc_alloc_node(evaluator->gc, LISP_LAMBDA);

    lambda_result->as.lambda.args = args;
    lambda_result->as.lambda.expr = subexpr;
    lambda_result->as.lambda.scope = CURR_SCOPE(evaluator);

    evaluator_push_value(evaluator, lambda_result);
}

void eval_lambda_call(LispNode *lambda, Evaluator *evaluator) {
    evaluator_push_scope(evaluator, lambda->as.lambda.scope);
    evaluator_push_scope(evaluator, gc_alloc_scope(evaluator->gc, CURR_SCOPE(evaluator)));

    for (size_t i = 0; i < lambda->as.lambda.args.size; i++)
        scope_define(CURR_SCOPE(evaluator), da_at(lambda->as.lambda.args, lambda->as.lambda.args.size - i - 1),
                   evaluator_pop_value(evaluator));

    eval_expr(lambda->as.lambda.expr, evaluator);
    evaluator_pop_scope(evaluator);
    evaluator_pop_scope(evaluator);
}

bool dispatch_special_form(LispNode *head, LispNode *args, Evaluator *evaluator) {
    if (head->kind != LISP_SYMBOL) return false;

    if (sv_eq(head->as.symbol, sv_mk("if"))) {
        //TODO: make proper assertions
        LispNode *condition = CAR(args);
        LispNode *if_true = CAR(CDR(args));
        LispNode *if_false = CAR(CDR(CDR(args)));

        eval_if_form(condition, if_true, if_false, evaluator);
        return true;
    }

    if (sv_eq(head->as.symbol, sv_mk("let"))) {
        assert(args->kind == LISP_CONS);

        LispNode *symbol = args->as.cons.car;
        LispNode *value_cons = args->as.cons.cdr;

        assert(symbol->kind == LISP_SYMBOL);
        assert(value_cons->kind == LISP_CONS);
        assert(value_cons->as.cons.cdr->kind == LISP_NIL);

        eval_let_form(symbol, value_cons->as.cons.car, evaluator);
        return true;
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
        eval_lambda_form(lambda_args, lambda_subexpr, evaluator);
        return true;
    }
    
    return false;
}

void eval_cons(LispNode *expr, Evaluator *evaluator) {
    assert(expr->kind == LISP_CONS);

    LispNode *head = expr->as.cons.car; 
    LispNode *args = expr->as.cons.cdr; 

    bool is_special_form = dispatch_special_form(head, args, evaluator);

    if (is_special_form) return;

    eval_expr(head, evaluator);
    LispNode *evaluated_head = evaluator_pop_value(evaluator);

    size_t args_count = eval_list(args, evaluator);

    switch (evaluated_head->kind) {
        case LISP_LAMBDA:
            eval_lambda_call(evaluated_head, evaluator);
            return;
        break;

        case LISP_BUILTIN:
            evaluated_head->as.builtin(args_count, evaluator);
            return;
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

void eval_expr(LispNode *expr, Evaluator *evaluator) {
    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            evaluator_push_value(evaluator, expr);
            return;
        break;

        case LISP_SYMBOL:
            if (sv_eq(expr->as.symbol, sv_mk("NIL")))
                evaluator_push_value(evaluator, gc_alloc_node(evaluator->gc, LISP_NIL));
            else evaluator_push_value(evaluator, scope_get(CURR_SCOPE(evaluator), expr->as.symbol));
            return;
        break;

        case LISP_CONS:
            eval_cons(expr, evaluator);
            return;
        break;
    }
    
    UNREACHABLE();
}

LispNodePtrDA extract_results(Evaluator *evaluator) {
    LispNodePtrDA results = evaluator->results;
    da_nullify(evaluator->results);
    return results;
}

