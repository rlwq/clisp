#include "scope.h"
#include <assert.h>

void scope_define(Scope *scope, StringView name, LispNode *value) {
    da_push(scope->symbols, name);
    da_push(scope->values, value);
}

LispNode *scope_get(Scope *scope, LispNode *expr) {
    if (scope == NULL)
        assert(0 && "No symbol found. No error handling."); //TODO: error reporting

    assert(expr->kind == LISP_SYMBOL);
    for (size_t i = 0; i < scope->symbols.size; i++) {
        if (sv_eq(da_at(scope->symbols, i), expr->as.symbol))
            return da_at(scope->values, i);
    }

    return scope_get(scope->parent, expr);    
}

