/*
 * five_minutes.c - the canonical embedder hello-world.
 *
 * Demonstrates: state + env construction with the amalgamation
 * distribution, evaluating a script, extracting the result back to C,
 * tearing down. Five minutes from zero to running.
 *
 * Build (using the amalgamation):
 *   cd mino && ./mino task amalgamate
 *   cc -std=c99 -O2 -Imino/dist -c mino/dist/mino.c -o dist/mino.o
 *   cc -std=c99 -O2 -Imino/dist src/cookbook/five_minutes.c \
 *      dist/mino.o -lm -lpthread -o src/cookbook/five_minutes
 *
 * Or via the make recipe used throughout this repo:
 *   make src/cookbook/five_minutes
 *
 * Run:
 *   ./src/cookbook/five_minutes
 */

#include "mino.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    mino_state *S;
    mino_env   *env;
    mino_val   *result;
    long long   n;

    /* Step 1: a state owns the heap and the GC.
     * Step 2: an env is the lexical scope where bindings live. The
     *         _default helper installs the canonical Clojure-core
     *         capability set (numbers, strings, collections, regex,
     *         bignum, plus the bundled clojure.string / clojure.set
     *         / clojure.math namespaces). */
    S   = mino_state_new();
    if (S == NULL) { fprintf(stderr, "state_new failed\n"); return 1; }
    env = mino_env_new_default(S);

    /* Step 3: evaluate a script. The result is borrowed -- it lives
     *         until the next GC cycle unless rooted via mino_ref_new. */
    result = mino_eval_string(S,
        "(reduce + 0 (map (fn [x] (* x x)) (range 1 11)))",
        env);
    if (result == NULL) {
        fprintf(stderr, "eval failed: %s\n", mino_last_error(S));
        return 1;
    }

    /* Step 4: extract the result back to C. mino_to_int returns 1 on
     *         success, 0 on type mismatch. */
    if (!mino_to_int(result, &n)) {
        fprintf(stderr, "result is not an int\n");
        return 1;
    }
    printf("sum of squares 1..10 = %lld\n", n);
    if (n != 385) {
        fprintf(stderr, "expected 385\n");
        return 1;
    }

    /* Step 5: tear down. The state owns everything reachable through
     *         it (env, all live values); free in this order. */
    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
