#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include "dynamic_array.h"

typedef struct Scope Scope;

typedef struct LispAST LispAST;
typedef struct GC GC;
typedef DA(LispAST *) LispASTPtrDA;

#endif
