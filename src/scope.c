#include "scope.h"
#include "gc.h"

Scope* scope_alloc(Scope *parent) {
    Scope *result = malloc(sizeof(Scope));
    assert(result);

    da_init(result->symbols);
    da_init(result->values);
    result->parent = parent;

    return result;
}

Scope *scope_free(Scope *scope) {
    if (!scope) return NULL;
    Scope *parent = scope->parent;
    
    da_free(scope->symbols);
    da_free(scope->values);

    free(scope);

    return parent;
}

void scope_define(Scope *scope, StringView name, LispAST *value) {
    da_push(scope->symbols, name);
    da_push(scope->values, value);
}

LispAST *scope_get(Scope *scope, LispAST *expr) {
    assert(expr->kind == LISP_SYMBOL);
    for (size_t i = 0; i < scope->symbols.size; i++) {
        if (sv_eq(da_at(scope->symbols, i), expr->as.symbol))
            return da_at(scope->values, i);
    }
    if (scope->parent == NULL)
        assert(0 && "No symbol found. No error handling."); //TODO: error reporting
    return scope_get(scope->parent, expr);    
}

void scope_mark(Scope *scope) {
    if (!scope) return;

    for (size_t i = 0; i < scope->symbols.size; i++)
        gc_mark(da_at(scope->values, i));
    scope_mark(scope->parent);
}
