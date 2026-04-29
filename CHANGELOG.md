# Changelog

## Unreleased

- Tracking mino v0.94.0 (empty-list canon parity: `()` is now a real
  value type, distinct from nil). Cookbook examples that rely on
  `nil`-punned empty results are unaffected; embed-side C code that
  walks `MINO_CONS` chains via `mino_is_cons` already terminates on
  any non-cons, so the new `MINO_EMPTY_LIST` enum is transparent.
- Tracking mino v0.93.0 (C refactoring pass; bundled `mino deps` and
  `mino task` tooling; bootstrap Makefile). The Makefile gains three
  gen-mino-header entries for the new `lib/mino/*` sources that v0.93.0
  bakes into the binary. Every cookbook recipe and use-case still
  builds and runs against the refreshed submodule.
- Tracking mino v0.74.0 (deferred core surface): `*ns*` is interned as
  a real dynamic var, `bound-fn` / `bound-fn*` capture and replay
  dynamic bindings, `read` accepts an opts map, `clojure.edn/read`
  forces `:read-cond :preserve`, `destructure` surfaces the C-side
  destructuring as a function, and the bundled regex engine grows
  capture groups with `re-matcher` and `re-groups`. Every cookbook
  recipe and use-case still builds and runs against the refreshed
  submodule. Makefile picks up `runtime/ns_env.c` and
  `runtime/path_buf.c` automatically through the per-subsystem
  wildcards.
- Tracking mino v0.73.0 (first-class namespaces): each namespace owns
  its own root binding table, `clojure.core` is the bundled-core
  namespace, vars are first-class objects, auto-resolved keywords and
  namespaced map literals land at read time, and source files use
  `.clj` instead of `.mino`. Host-tests, Makefile, and embedded mino
  scripts swap to `.clj` source paths alongside the migration.
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
