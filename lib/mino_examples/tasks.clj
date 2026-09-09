(ns mino-examples.tasks)

;; mino-examples' task runner. mino dogfoods itself: the build runs
;; through `./mino/mino task build`, not a Makefile.
;;
;; Every example (C and C++) compiles against the mino amalgamation
;; (mino/dist/mino.{c,h,hpp}), the single embedding boundary mino ships
;; (ADR-62). C sources include "mino.h" and C++ sources include
;; "mino.hpp"; both link the one prebuilt dist/mino.o. Nothing reaches
;; into mino's private src/** tree, so a mino C-tree reorg is a no-op for
;; this build once the submodule pin is bumped.
;;
;; How the amalgam object is materialized lives in mino itself, in the
;; shared mino.tasks.amalgam helper this runner requires; there is no
;; per-consumer copy to drift (ADR-62).
;;
;; First-time bootstrap: `cd mino && make`. After that, every rebuild
;; goes through `./mino/mino task build`, which regenerates the
;; amalgamation only when the submodule pin changes.

(require '[clojure.string :as str]
         '[mino.path :as path]
         '[mino.tasks.amalgam :as amalgam])

;;;; Build configuration

(def ^:private cc  (or (getenv "CC")  "cc"))
(def ^:private cxx (or (getenv "CXX") "c++"))

;; The amalgamation is the sole mino include root: dist/mino.h and
;; dist/mino.hpp are the only mino headers any consumer source includes,
;; so -Imino/dist rides on both compile flag sets.
(def ^:private cflags
  (str/split (or (getenv "CFLAGS")
                 "-std=c99 -Wall -Wpedantic -Wextra -O2 -Imino/dist") " "))
(def ^:private cxxflags
  (str/split (or (getenv "CXXFLAGS")
                 "-std=c++17 -Wall -Wextra -O2 -Imino/dist") " "))
(def ^:private ldflags
  (let [v (or (getenv "LDFLAGS") "")]
    (if (= v "") [] (str/split v " "))))
(def ^:private libs
  (str/split (or (getenv "LIBS") "-lm -lpthread") " "))

;;;; The example binary set.

;; C examples: each has its own main() and links the amalgam object.
;; embed_c is src/embed.c built to src/embed_c.
(def ^:private c-examples
  {"src/api_stress_test"   "src/api_stress_test.c"
   "src/clone_test"        "src/clone_test.c"
   "src/integration_test"  "src/integration_test.c"
   "src/interop_test"      "src/interop_test.c"
   "src/ref_test"          "src/ref_test.c"
   "src/state_switch_test" "src/state_switch_test.c"
   "src/regex_thread_test" "src/regex_thread_test.c"
   "src/embed_c"           "src/embed.c"})

;; Cookbook C examples, discovered under src/cookbook/.
(defn- discover-cookbook-examples []
  (into {}
        (for [src (path/glob "src/cookbook/*.c")]
          [(subs src 0 (- (count src) 2)) src])))

;; C++ examples. embed_cpp is src/embed.cpp built to src/embed_cpp.
(def ^:private cxx-examples
  {"src/cpp_embed_test"   "src/cpp_embed_test.cpp"
   "src/event_processing" "src/event_processing.cpp"
   "src/embed_cpp"        "src/embed.cpp"})

;; Use-case C++ examples, discovered under use-cases/.
(defn- discover-use-case-examples []
  (into {}
        (for [src (path/glob "use-cases/*.cpp")]
          [(subs src 0 (- (count src) 4)) src])))

;;;; Compile

(defn- build-one
  "Compile one example against the amalgam object. Rebuild only when
   stale. Returns 1 if it built, 0 if it was up to date."
  [compiler flags bin src]
  (if (amalgam/stale? [src amalgam/dist-obj] bin)
    (let [args (into [compiler] (concat flags ldflags
                                        ["-o" bin src amalgam/dist-obj] libs))]
      (println (str "  " (str/join " " args)))
      (apply sh! args)
      1)
    0))

(defn- build-c   [bin src] (build-one cc  cflags   bin src))
(defn- build-cxx [bin src] (build-one cxx cxxflags bin src))

;;;; Build

(defn build
  "Build every example (C and C++) against the mino amalgamation."
  []
  (amalgam/ensure-dist! cc cflags)
  (let [built (atom 0)]
    (doseq [[bin src] (concat c-examples (discover-cookbook-examples))]
      (swap! built + (build-c bin src)))
    (doseq [[bin src] (concat cxx-examples (discover-use-case-examples))]
      (swap! built + (build-cxx bin src)))
    (if (zero? @built)
      (println "  all examples up to date")
      (println (str "  built " @built " example(s)")))))

(defn test-use-cases
  "Run each use-case example, asserting a clean (exit 0) run."
  []
  (doseq [[bin _] (sort (discover-use-case-examples))]
    (let [r (sh bin)]
      (println (format "%-40s %s" bin (if (zero? (:exit r)) "ok" "FAIL")))
      (when-not (zero? (:exit r))
        (println (:err r))
        (throw (ex-info "use-case example failed" {:bin bin :exit (:exit r)})))))
  (println "all use case examples passed"))

(defn clean
  "Remove built example binaries (never touches the mino/ submodule)."
  []
  (doseq [[bin _] (concat c-examples (discover-cookbook-examples)
                          cxx-examples (discover-use-case-examples))]
    (when (file-exists? bin)
      (rm-rf bin)))
  (println "  removed built example binaries"))
