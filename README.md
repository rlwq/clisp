# Sparkle
Sparkle is a Lisp dialect designed to be intuitive and easy to use and reuse.
The interpreter is written from scratch in C, no dependencies required.

## Main features
* **S-expressions** - everything is a list
* **First-class functions** - functions with lexical scoping
* **Mark-and-sweep GC** - automatic memory management

## Build

```bash
# Optimized build
make build

# Debug build (AddressSanitizer + UBSan + Assertions)
make debug
```

## Usage

```bash
./build/sparke source.rkl
```
