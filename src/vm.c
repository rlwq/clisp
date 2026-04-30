#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "gc.h"
#include "scope.h"
#include "vm.h"
#include "dynamic_array.h"
#include "lisp_node.h"
#include "string_view.h"
#include "forwards.h"

#define CURR(e_) (*((e_)->stmts))

#define VM_CHECK(e_) if ((e_)->is_err) return
#define VM_CHECKR(e_, r_) if ((e_)->is_err) return (r_)
#define VM_ASSERT(e_, expr_) if (!(expr_)) { (e_)->is_err = true; return; }
#define VM_ASSERTR(e_, expr_, r_) if (!(expr_)) { (e_)->is_err = true; return r_; }

VM *vm_alloc(LispNodePtrDA exprs, GC *gc) {
    VM *vm = malloc(sizeof(VM));
    assert(vm);

    vm->stmts = exprs.data;
    vm->stmts_count = exprs.size;
    vm->is_err = false;

    vm->gc = gc;

    da_init(vm->scope_stack);
    da_init(vm->value_stack);

    return vm;
}

void vm_free(VM *vm) {
    da_free(vm->scope_stack);
    da_free(vm->value_stack);

    free(vm);
}

void vm_push_error(VM *vm) {
    vm->is_err = true;
}

void vm_push_scope(VM *vm, Scope *scope) {
    VM_CHECK(vm);
    da_push(vm->scope_stack, scope);
}

void vm_build_scope(VM *vm) {
    VM_CHECK(vm);
    vm_push_scope(vm, gc_alloc_scope(vm->gc, CURR_SCOPE(vm)));
}

void vm_pop_scope(VM *vm) {
    VM_CHECK(vm);
    VM_ASSERT(vm, vm->scope_stack.size > 0);
    da_pop(vm->scope_stack);
}

void vm_scope_define(VM *vm, StringView name) {
    VM_CHECK(vm);
    scope_define(CURR_SCOPE(vm), name, vm_peek_value(vm));
    vm_pop_value(vm);
}

// 0 -> 1
void vm_push_value(VM *vm, LispNode *value) {
    VM_CHECK(vm);
    assert(value);
    da_push(vm->value_stack, value);
}

void vm_build_value(VM *vm, LispNodeKind kind) {
    VM_CHECK(vm);
    if (gc_check_bounds(vm->gc)) {
        vm_mark(vm);
        gc_sweep(vm->gc);
    }
    vm_push_value(vm, gc_alloc_node(vm->gc, kind));
}

void vm_build_integer(VM *vm, int value) {
    VM_CHECK(vm);
    vm_build_value(vm, LISP_INTEGER);
    vm_peek_value(vm)->as.integer = value;
}

void vm_build_builtin(VM *vm, LispBuiltin value) {
    VM_CHECK(vm);
    vm_build_value(vm, LISP_BUILTIN);
    vm_peek_value(vm)->as.builtin = value;
}

void vm_build_nil(VM *vm) {
    VM_CHECK(vm);
    vm_build_value(vm, LISP_NIL);
}

void vm_build_lambda(VM *vm, StringViewDA args, LispNode *expr, Scope *scope) {
    VM_CHECK(vm);
    vm_build_value(vm, LISP_LAMBDA);
    vm_peek_value(vm)->as.lambda.args = args;
    vm_peek_value(vm)->as.lambda.expr = expr;
    vm_peek_value(vm)->as.lambda.scope = scope;
}

// 1 -> 0
void vm_pop_value(VM *vm) {
    VM_CHECK(vm);
    assert(vm->value_stack.size > 0);
    da_pop(vm->value_stack);
}

void vm_pop_prev_value(VM *vm) {
    VM_CHECK(vm);
    assert(vm->value_stack.size >= 2);
    da_at_end(vm->value_stack, 1) = da_at_end(vm->value_stack, 0);
    da_pop(vm->value_stack);
}

// Node (x), Node (y) -> Node (y), Node (x)
void vm_swap_value(VM *vm) {
    VM_CHECK(vm);
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

// Node (x), Node (y), Node (z) -> Node (y), Node (z), Node (x)
void vm_rot_value(VM *vm) {
    VM_CHECK(vm);
    assert(vm->value_stack.size >= 3);
    LispNode *curr = da_at_end(vm->value_stack, 0);
    da_at_end(vm->value_stack, 0) = da_at_end(vm->value_stack, 2);
    da_at_end(vm->value_stack, 2) = da_at_end(vm->value_stack, 1);
    da_at_end(vm->value_stack, 1) = curr;
}

LispNode *vm_peek_value(VM *vm) {
    VM_CHECKR(vm, NULL);
    assert(vm->value_stack.size);

    return da_at_end(vm->value_stack, 0);
}

void vm_mark(VM *vm) {
    // Marking unevaluated expressions
    for (size_t i = 0; i < vm->stmts_count; i++)
        gc_mark_node(vm->stmts[i]);

    // Marking scopes
    for (size_t i = 0; i < vm->scope_stack.size; i++)
        gc_mark_scope(da_at(vm->scope_stack, i));

    // Marking value stack
    for (size_t i = 0; i < vm->value_stack.size; i++)
        gc_mark_node(da_at(vm->value_stack, i));
}

LispNode *vm_advance(VM *vm) {
    VM_CHECKR(vm, NULL);
    assert(vm->stmts_count > 0);
    LispNode *curr = CURR(vm);
    
    vm->stmts++;
    vm->stmts_count--;
    return curr;
}

// Cons -> Expr * n
size_t eval_list(VM *vm) {
    VM_CHECKR(vm, 0);

    VM_ASSERTR(vm, vm_peek_value(vm)->kind == LISP_NIL ||
                   vm_peek_value(vm)->kind == LISP_CONS, 0);

    size_t size = 0;
    while (vm_peek_value(vm)->kind != LISP_NIL) {
        VM_ASSERTR(vm, vm_peek_value(vm)->kind == LISP_CONS, 0);

        // Evaluate head
        vm_push_value(vm, CAR(vm_peek_value(vm)));
        vm_eval_expr(vm);
        vm_swap_value(vm);

        // Push tail
        vm_push_value(vm, CDR(vm_peek_value(vm)));
        vm_pop_prev_value(vm);
        size++;
    }
    vm_pop_value(vm); 
    return size;
}

void vm_eval_current(VM *vm) {
    VM_CHECK(vm);
    assert(VM_VALID(vm));
    
    vm_push_value(vm, vm_advance(vm));
    vm_eval_expr(vm);
    vm_pop_value(vm);
}

void vm_register_builtin(VM *vm, StringView name, LispBuiltin func_ptr) {
    VM_CHECK(vm);
    vm_build_builtin(vm, func_ptr);
    vm_scope_define(vm, name);
}

void vm_eval_all(VM *vm) {
    VM_CHECK(vm);
    assert(VM_VALID(vm));

    while (VM_VALID(vm)) {
        vm_eval_current(vm);
        vm_mark(vm);

        gc_sweep(vm->gc);
    }
}

// Node (Cons) -> Node (Tail), Node (Head)
void unpack_cons(VM *vm) {
    VM_CHECK(vm);
    assert(vm_peek_value(vm)->kind == LISP_CONS);
    vm_push_value(vm, CDR(vm_peek_value(vm)));
    vm_swap_value(vm);
    vm_push_value(vm, CAR(vm_peek_value(vm)));
    vm_swap_value(vm);
    vm_pop_value(vm);
}

// Node (cons) -> Node * n
size_t unpack_list(VM *vm) {
    VM_CHECKR(vm, 0);
    assert(vm_peek_value(vm)->kind == LISP_NIL ||
           vm_peek_value(vm)->kind == LISP_CONS);

    size_t size = 0;
    while (vm_peek_value(vm)->kind != LISP_NIL) {
        assert(vm_peek_value(vm)->kind == LISP_CONS);
        unpack_cons(vm);
        vm_swap_value(vm);

        size++;
    }

    vm_pop_value(vm); 
    return size;
}

// Symbol (name), Node (value) -> Node
void eval_let_form(VM *vm) {
    VM_CHECK(vm);
    vm_swap_value(vm);
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);
    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);
    vm_eval_expr(vm);

    scope_define(CURR_SCOPE(vm), name, vm_peek_value(vm));
}

// Node (condition), Node (is_true), Node (is_false) -> result
void eval_if_form(VM *vm) {
    VM_CHECK(vm);
    vm_rot_value(vm);
    vm_eval_expr(vm);

    bool is_positive = vm_peek_value(vm)->kind != LISP_NIL;
    vm_pop_value(vm);

    if (is_positive) vm_pop_value(vm);
    else vm_pop_prev_value(vm);

    vm_eval_expr(vm);
}

// Cons (Args list), Node (subexpr) -> Lambda
void eval_lambda_form(VM *vm) {
    VM_CHECK(vm);
    vm_swap_value(vm);
    size_t args_count = unpack_list(vm);

    StringViewDA args;
    da_init(args);

    for (size_t i = 0; i < args_count; i++) {
        da_push(args, vm_peek_value(vm)->as.symbol);
        vm_pop_value(vm);
    }

    LispNode *subexpr = vm_peek_value(vm);

    vm_build_lambda(vm, args, subexpr, CURR_SCOPE(vm));
    vm_pop_prev_value(vm);
}

// Node -> Node
void eval_quote_form(VM *vm) {
    VM_CHECK(vm);
    assert(vm->value_stack.size > 0);
}

// Node * n (args), Node (lambda) -> Node (result)
void eval_lambda_call(VM *vm) {
    VM_CHECK(vm);
    LispNode *lambda = vm_peek_value(vm);

    vm_push_scope(vm, lambda->as.lambda.scope);
    vm_build_scope(vm);

    for (size_t i = 0; i < lambda->as.lambda.args.size; i++) {
        vm_swap_value(vm);
        vm_scope_define(vm, da_at(lambda->as.lambda.args, i));
    }
    
    vm_push_value(vm, lambda->as.lambda.expr);
    
    vm_eval_expr(vm);
    
    vm_pop_prev_value(vm);

    vm_pop_scope(vm);
    vm_pop_scope(vm);
}

// Node (Args), Node (Head) -> Node (Maybe :3)
bool try_dispatch_special_form(VM *vm) {
    VM_CHECKR(vm, false);
    if (vm_peek_value(vm)->kind != LISP_SYMBOL) return false;

    SpecialFormHandler handler = NULL;

    if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("if")))
        handler = eval_if_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("let")))
        handler = eval_let_form;

    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("lambda")))
        handler = eval_lambda_form;
   
    else if (sv_eq(vm_peek_value(vm)->as.symbol, sv_mk("quote")))
        handler = eval_quote_form;

    if (handler) {
        vm_pop_value(vm);
        unpack_list(vm);
        handler(vm);
        return true;
    }

    return false;
}

// ConsNode -> Node
void eval_cons(VM *vm) {
    VM_CHECK(vm);
    assert(vm_peek_value(vm)->kind == LISP_CONS);
    
    unpack_cons(vm);

    bool is_special_form = try_dispatch_special_form(vm);
    if (is_special_form) return;
    
    vm_eval_expr(vm);
    LispNode *head = vm_peek_value(vm);
    vm_swap_value(vm);
    size_t args_count = eval_list(vm);

    switch (head->kind) {
        case LISP_LAMBDA:
            vm_push_value(vm, head);
            eval_lambda_call(vm);
        break;

        case LISP_BUILTIN:
            head->as.builtin(vm, args_count);
        break;

        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_NIL:
            assert(0 && "Can't call this kind of object.");
        break;
    }

    vm_pop_prev_value(vm);
}

void vm_scope_get(VM *vm, StringView name) {
    VM_CHECK(vm);
    LispNode *lookup_result = scope_get(CURR_SCOPE(vm), name);
    if (!lookup_result) {
        vm_push_error(vm);
        return;
    }
    vm_push_value(vm, lookup_result);
}

// Symbol -> Node
void eval_symbol(VM *vm) {
    VM_CHECK(vm);
    assert(vm_peek_value(vm)->kind == LISP_SYMBOL);
    StringView name = vm_peek_value(vm)->as.symbol;
    vm_pop_value(vm);

    if (sv_eq(name, sv_mk("NIL"))) vm_build_nil(vm);
    else vm_scope_get(vm, name);
}

// Node -> Node
void vm_eval_expr(VM *vm) {
    VM_CHECK(vm);
    switch (vm_peek_value(vm)->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
        case LISP_LAMBDA:
            // Do nothing
        break;

        case LISP_SYMBOL:
            eval_symbol(vm);
        break;

        case LISP_CONS:
            eval_cons(vm);
        break;
    }
}

