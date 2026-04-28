#include "scope.h"
#include "string_view.h"
#include <assert.h>

void scope_define(Scope *scope, StringView name, LispNode *value) {
    da_push(scope->symbols, name);
    da_push(scope->values, value);
}

LispNode *scope_get(Scope *scope, StringView name) {
    for (Scope *curr = scope; curr != NULL; curr = curr->parent) {
        for (size_t i = 0; i < curr->symbols.size; i++) {
            if (sv_eq(da_at(curr->symbols, i), name))
                return da_at(curr->values, i);
        }
    }
    assert(0 && "No symbol found. No error handling.");
}

