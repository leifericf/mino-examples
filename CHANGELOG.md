# Changelog

## Unreleased

- Tracking mino v0.48.0: embedder polish release adds `MINO_VERSION_*`
  constants, `mino_version_string()`, `mino_throw(S, payload)` for
  raising mino exceptions from C, and `mino_args_parse(S, name, args,
  fmt, ...)` for one-call argument destructuring. Makefile extended
  to compile `runtime_gc_trace.c`, `public_embed.c`, and `transient.c`
  (new TUs across v0.43.0–v0.48.0). Every cookbook recipe and
  use-case still builds and runs against the refreshed submodule.
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
