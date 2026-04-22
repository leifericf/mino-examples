# Changelog

## Unreleased

- Tracking mino v0.42.0: generational + incremental garbage collector,
  public GC API, literal-builder barrier fix. Makefile extended to
  compile the new `runtime_gc_roots.c`, `runtime_gc_major.c`,
  `runtime_gc_barrier.c`, `runtime_gc_minor.c`, `public_gc.c`, and
  `prim_lazy.c` TUs.
- Tracking mino v0.39.1 (task runner, `str-replace` primitive,
  `file-mtime` primitive, Windows CI)

## v0.1.0

Initial release. Extracted from the main mino repository.

- Basic C and C++ embedding examples
- Cookbook recipes (config, console, pipeline, plugin, rules, REPL socket)
- C++ use-case demos (game scripting, data pipeline, event processing, etc.)
- JNI bridge and Java embedding example
- Integration and stress test programs
