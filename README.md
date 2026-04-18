# mino-examples

Embedding examples, cookbook recipes, use-case demos, and language bindings for [mino](https://github.com/leifericf/mino).

## Build

```
git submodule update --init
make
```

## Contents

- `src/embed.c`, `src/embed.cpp` -- basic C and C++ embedding
- `src/cookbook/` -- practical embedding recipes (config, console, pipeline, plugin, rules engine, REPL socket)
- `use-cases/` -- C++ use-case demos (game scripting, data pipeline, event processing, etc.)
- `jni/` -- Java/JNI bridge
- `src/*_test.c` -- integration and stress test programs

## Use-case tests

```
make test-use-cases
```
