#include "scope.h"
#include "string_view.h"
#include <assert.h>

void scope_define(Scope *scope, StringView name, LispNode *value) {
    da_push(scope->symbols, name);
    da_push(scope->values, value);
}

LispNode *scope_get(Scope *scope, StringView name) {
    if (scope == NULL)
        assert(0 && "No symbol found. No error handling."); //TODO: error reporting

    for (size_t i = 0; i < scope->symbols.size; i++) {
        if (sv_eq(da_at(scope->symbols, i), name))
            return da_at(scope->values, i);
    }

    return scope_get(scope->parent, name);    
}

