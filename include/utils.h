#ifndef UTILS_H
#define UTILS_H

#define UNREACHABLE() (assert(0 && "Unreachable"), __builtin_unreachable())
#define NOT_IMPLEMENTED() (assert(0 && "Not implemented"), __builtin_unreachable())

#endif
