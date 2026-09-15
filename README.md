> [!WARNING]
> **Discontinued — archived LLM experiment.** This project was an experiment to explore LLM-based ("agentic") software development. The resulting code is low-quality, unreliable, and unmaintainable, and it is no longer developed or supported. It is kept public and archived purely as a learning example. Do not use it in production.

# mino-examples

Embedding examples, cookbook recipes, use-case demos, and language
bindings for [mino](https://github.com/leifericf/mino).

## Build

```
git submodule update --init
cd mino && make && cd ..
./mino/mino task build
```

The one-time `cd mino && make` bootstraps the submodule: it produces
the mino binary and its bundled headers. From then on
`./mino/mino task build` builds every example, cookbook chapter, and
use-case demo against the mino amalgamation: a single-file drop-in at
`mino/dist/mino.c` + `mino/dist/mino.h` (plus `mino/dist/mino.hpp` for
C++), the shape embedders integrate into their own project tree. The
task runner materializes the amalgamation on first build and
regenerates it only when the submodule pin changes. C examples include
`"mino.h"`, C++ examples include `"mino.hpp"`, and both link the one
prebuilt `mino/dist/mino.o`.

## Where to start

If you're new to embedding mino, read these in order:

1. **`src/cookbook/five_minutes.c`** — the canonical hello-world.
   State + env construction, eval a script, extract the result, tear
   down. Five minutes from zero to running.

2. **`src/cookbook/handle_record_atom_choice.c`** — the decision
   tree for "how do I expose my host type to mino script?".
   Answers the Lua-metatable question with the three Clojure-canon
   paths: handle (identity-shaped resource), record (value-shaped
   data), atom (mutable identity).

3. **`src/cookbook/`** — practical recipes: configuration, console,
   pipeline, plugin host, rules engine, REPL socket.

4. **`use-cases/`** — C++ demos sized as production scenarios:
   game scripting, data pipeline, event processing, automation,
   rules engine, plugin host.

## Contents

- `src/embed.c`, `src/embed.cpp` — basic C and C++ embedding.
- `src/cookbook/` — practical embedding recipes plus the
  five-minute intro and the handle/record/atom decision tree.
- `use-cases/` — C++ use-case demos sized as small applications.
- `jni/` — Java/JNI bridge.
- `src/*_test.c` — integration and stress test programs that
  exercise the C API beyond what the in-tree mino test suite covers.

## Use-case tests

```
./mino/mino task test-use-cases
```

Builds and runs every use-case demo; exits 0 on a clean run.

## Cross-compiling your embedding

[`docs/cross-compile-with-zig.md`](docs/cross-compile-with-zig.md) —
ship your mino-embedding host program to Linux (x86_64 / ARM64) and
Windows from one machine with `zig cc`: the amalgamation plus one
command per target. Optional convenience; mino itself needs only a
C99 compiler.

## How mino ships

See [Zero dependencies, vendored first](https://mino-lang.org/documentation/vendored-first/)
on the mino site for the distribution philosophy: drop the runtime
into your project, own it, no package manager. C99 + libm +
pthreads is the entire build environment.