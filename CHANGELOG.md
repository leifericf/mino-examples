# Changelog

## Unreleased

- Tracking mino v0.383.0 (embedder UX cycle, Phases 1–2).
  Phase 1: opaque `struct mino_val`, a single
  `mino_install(S, env, caps)` bitmask entry replacing the 22
  per-capability installers, a full type-predicate grid, the
  `_ex` matrix with structured error access, collection builders
  and a unified iterator, host-syntax routing through the BC
  tree-walker, namespace-resolved `host/new`, and `mino_int`
  auto-promote to bigint when `MINO_CAP_BIGNUM` is installed —
  plus the v0.151.1 embedding hardening (NULL-arg guards on
  `mino_eval_string` and `mino_read`, sorted-map / sorted-set
  iter, raw thrown payload through `out_ex`, `mino_to_int`
  bigint round-trip).
  Phase 2: drops the `_t` suffix from every public typedef
  (SQLite-style `typedef struct mino_X mino_X`), renames the
  `mino_ref` function to `mino_ref_new` to clear the typedef
  collision, and renames the `mino_gc_stats` struct to
  `mino_gc_stats_out` to clear the function-name collision.
  Every C/C++ example and cookbook chapter in this repo has been
  ported to the new names; the JNI binding likewise.

- JNI binding: `Java_MinoEmbed_envNew` was still calling the
  removed-in-v0.151 `mino_new(state)` symbol. Replaced with
  `mino_env_new_default(state)`, the canonical Clojure-core +
  default-capability env-construction entry point. Any host
  build of `jni/mino_jni.c` against a current mino submodule
  was failing to link before this change.

- Tracking mino v0.104.0 through v0.149.1. A long stretch of mino
  cycles: bytecode-VM rollout (v0.105.0-v0.114.0), generational
  + incremental GC follow-ups, dialect-complete cycle, C-core
  refactor cycle, ns-first-class cycle, several conformance and
  hygiene passes, and the v0.149.1 bug-fix roll-up (hash contract
  for sequential and sorted collections, sorted-collection
  dissoc count, `ex-info` 3-arity cause, catch metadata
  preservation, `fmt_ensure` / `(sh ...)` OOM cleanup, `pclose`
  `-1` sentinel). The mino runtime added bytecode-VM source
  files under `src/eval/bc/`; the Makefile's `MINO_SRCS` glob
  now includes that subdirectory so the embedded runtime links
  the BC entry points (`_mino_bc_run`, `_mino_bc_compile_fn`,
  `_mino_bc_check_require`, `_mino_bc_declined`). The cookbook
  and use-case examples themselves are unchanged.

- Tracking mino v0.103.0 (Worker-List Lock Split: brief
  `worker_list_lock` separated from the recursive `state_lock`
  for the worker bookkeeping path. The public C surface is
  unchanged; embedders that rebuild against the new submodule
  pick up the fix transparently. The `agents` cookbook example
  benefits without code changes -- a tight calling-thread loop
  while agent workers are spawning no longer stalls
  `thread_count` decrement). The `.gitignore` gains a
  `src/cookbook/agents` entry alongside the other cookbook
  binaries so a fresh `make` doesn't show the binary as
  untracked.
- Tracking mino v0.102.1 (Agents finish MVP cycle: per-state
  agent workers + run-queues with separate POOLED / SOLO pools
  for `send` / `send-off`; public C-API perimeter for embedders
  (`mino_send`, `mino_send_off`, `mino_await`, `mino_await_for`,
  `mino_agent_error`, `mino_restart_agent`); `await` and
  `await-for` now actually block; `shutdown-agents` joins both
  pool workers; `restart-agent` accepts `:clear-actions true`;
  the v0.102.1 patch closed an adversarial-test pass with a
  thread-budget message accuracy fix). The
  `src/cookbook/agents.c` example was added in this cycle to
  demonstrate the full C-API perimeter end-to-end.
- Tracking mino v0.101.0 (STM cycle): refs, `dosync`, `alter`,
  `commute`, `ensure`, `ref-set`, `io!`, watches and validators on
  refs and vars; agents (`agent`, `send`, `send-off`, `await`,
  `agent-error`, `restart-agent`, error-mode / error-handler);
  Layer 2a C API mirroring the Clojure surface (`mino_tx_ref`,
  `mino_tx_run`, `mino_tx_alter_c`, `mino_tx_commute_c`,
  `mino_tx_ensure`, `mino_tx_ref_set`, `mino_tx_ref_deref`,
  `mino_is_tx_ref`); cross-state ref defense via MST007;
  `mino_pcall` API change adding an `out_ex` parameter; plus the
  intermediate cycle of additions that landed since v0.98.5:
  MINO_HOST_ARRAY, MINO_MAP_ENTRY, MINO_FLOAT32 value types;
  strict integer overflow on `+`/`-`/`*`; `bigdec`÷`ratio` widens
  to bigdec; `cons` returns non-list shape; `aset` for host
  arrays; fixed-arity enforcement at fn apply. Submodule bumped
  from `022b83a` (v0.98.5) to `b66c3e5` (v0.101.0).

- Adapt all `mino_pcall` call sites to the new 6-argument
  signature (added `, NULL` / `, nullptr` for the new `out_ex`
  parameter): `src/api_stress_test.c` (2 sites),
  `src/cookbook/plugin.c`, `src/integration_test.c`,
  `src/cpp_embed_test.cpp` (2 sites), `use-cases/plugins.cpp`.
  None of these example programs need the raw thrown value, so
  passing `NULL` matches the previous error-reporting behavior
  via `mino_last_error`.

- Tracking mino v0.98.5 (Hygiene + Closure cycle: macro hygiene fix
  in `qq_qualify_symbol` so syntax-quoted bare symbols inside a
  macro body qualify against the macro's defining namespace not the
  consumer's `*ns*` (closes the silent
  `with-out-str`-after-`:refer :all` miscompile and the
  `unbound symbol: chan*` failure for `(a/go ...)` called from
  outside `clojure.core.async`); `compare` gains the canon
  cross-type total order
  `nil < false < true < numbers < strings < symbols < keywords`;
  `clojure.string/split` gains the 3-arg `limit` arity; vector seqs
  and lazy `range` auto-chunk into 32-element chunks so
  `(chunked-seq? (seq [1 2 3]))` is `true` and
  `(reduce + (map inc (filter odd? (range 1e6))))`-style pipelines
  run end-to-end chunked; `array-map` insertion-order semantics
  verified to already match canon; `random-seed!` primitive plus a
  minimal `clojure.test.check` port (generators, properties,
  `quick-check`; shrinking deferred) backing
  `clojure.spec.alpha/gen` and `clojure.spec.alpha/exercise`).
  Makefile bundled-stdlib list grows the three new
  `lib_clojure_test_check*.h` headers; otherwise the build is a
  drop-in submodule bump.
- Tracking mino v0.97.5 (Kwargs + Audit + Hygiene cycle: kwargs
  destructuring matches Clojure 1.11 (inline pairs, trailing map,
  mixed; `:or` defaults eval correctly); `iteration` rewritten to
  canon `& {:keys [...]}` shape; `sort-by` and `reductions` gain
  multi-arity; `src/core.clj` 80-char wrap; `defn` lifted so six
  bootstrap `def + fn` forms become `defn`; `clojure.core.async`
  gains canon `reduce` / `transduce` / `split` / `partition-by`;
  `clojure.spec.alpha` gains `abbrev` / `describe`). No
  example-side changes — embed-side C and C++ code does not switch
  on every value-type tag and the cookbook scripts use only stable
  surface; everything builds and links against the bumped submodule.
- Tracking mino v0.96.8 (Canon-Parity cycle: real `MINO_VOLATILE`
  primitive, stateful-transducer rewrites, lazy-seq recur-on-skip,
  transient reductions, comp/partial/some-fn/every-pred unrolling
  plus `into` 0/1-arg and `unchecked-divide-int`, `iteration` from
  Clojure 1.11, `clojure.core.async` namespace wrap with `merge`/`into`
  renames, the `:refer :all` transitive-drag fix, and the chunked-seq
  family with two new value types and eight primitives). No
  example-side changes — embed-side C and C++ code does not switch
  on every value-type tag and the cookbook scripts use only stable
  surface; everything builds and links against the bumped submodule.
- Tracking mino v0.95.5 (Clojure-side hygiene pass: bundled stdlib
  refactor). No example-side changes — the hygiene pass is internal
  to the mino-side library; embed-side C and C++ code is unaffected
  and the cookbook continues to compile against the bumped submodule.
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
