#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "dynamic_array.h"

typedef struct Scope Scope;

typedef struct LispNode LispNode;
typedef struct GC GC;
typedef DA(LispNode *) LispNodePtrDA;

#endif
