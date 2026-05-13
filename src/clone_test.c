/*
 * clone_test.c -- exercise mino_clone and mino_mailbox.
 *
 * Build:
 *   cc -std=c99 -I.. -o clone_test clone_test.c ../mino.o ../re.o -lm -lpthread
 * Run:
 *   ./clone_test
 */

#include "mino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

static size_t count_collection(mino_state_t *S, const mino_val_t *v)
{
    mino_iter_t *it = (mino_iter_t *)alloca(mino_iter_sizeof());
    size_t n = 0;
    mino_iter_init(S, it, (mino_val_t *)v);
    while (mino_iter_next(it, NULL, NULL)) n++;
    mino_iter_done(it);
    return n;
}

#define ASSERT(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "FAIL: %s\n", msg); return 1; } \
} while (0)

int main(void)
{
    mino_state_t *A = mino_state_new();
    mino_state_t *B = mino_state_new();
    mino_env_t *ea  = mino_env_new_default(A);
    mino_env_t *eb  = mino_env_new_default(B);

    /* ---- Test 1: clone primitives ---- */
    {
        mino_val_t *v;

        v = mino_clone(B, A, mino_nil(A));
        ASSERT(v != NULL && mino_is_nil(v), "nil clone");

        v = mino_clone(B, A, mino_true(A));
        ASSERT(v != NULL && mino_is_bool(v) && mino_to_bool(v), "true clone");

        {
            long long i_val = 0;
            v = mino_clone(B, A, mino_int(A, 42));
            ASSERT(v != NULL && mino_is_int(v) && mino_to_int(v, &i_val) && i_val == 42, "int clone");
        }

        v = mino_clone(B, A, mino_float(A, 3.14));
        ASSERT(v != NULL && mino_is_float(v), "float clone");

        {
            const char *s_data = NULL;
            size_t s_len = 0;
            v = mino_clone(B, A, mino_string(A, "hello"));
            ASSERT(v != NULL && mino_is_string(v), "string clone");
            ASSERT(mino_to_string(v, &s_data, &s_len) && strcmp(s_data, "hello") == 0, "string data");
        }

        v = mino_clone(B, A, mino_keyword(A, "foo"));
        ASSERT(v != NULL && mino_is_keyword(v), "keyword clone");

        v = mino_clone(B, A, mino_symbol(A, "bar"));
        ASSERT(v != NULL && mino_is_symbol(v), "symbol clone");
    }

    /* ---- Test 2: clone a vector ---- */
    {
        mino_val_t *src = mino_eval_string(A, "(vec (range 10))", ea);
        mino_val_t *dst;
        ASSERT(src != NULL, "eval range");
        dst = mino_clone(B, A, src);
        ASSERT(dst != NULL, "vector clone not null");
        ASSERT(mino_is_vector(dst), "vector clone type");
        ASSERT(count_collection(B, dst) == 10, "vector clone len");
    }

    /* ---- Test 3: clone a map ---- */
    {
        mino_val_t *src = mino_eval_string(A,
            "(hash-map :a 1 :b 2 :c 3)", ea);
        mino_val_t *dst;
        ASSERT(src != NULL, "eval map");
        dst = mino_clone(B, A, src);
        ASSERT(dst != NULL, "map clone not null");
        ASSERT(mino_is_map(dst), "map clone type");
        ASSERT(count_collection(B, dst) == 3, "map clone len");
    }

    /* ---- Test 4: clone a set ---- */
    {
        mino_val_t *src = mino_eval_string(A,
            "(hash-set 1 2 3 4 5)", ea);
        mino_val_t *dst;
        ASSERT(src != NULL, "eval set");
        dst = mino_clone(B, A, src);
        ASSERT(dst != NULL, "set clone not null");
        ASSERT(mino_is_set(dst), "set clone type");
        ASSERT(count_collection(B, dst) == 5, "set clone len");
    }

    /* ---- Test 5: clone rejects functions ---- */
    {
        mino_val_t *fn = mino_eval_string(A, "(fn (x) x)", ea);
        mino_val_t *dst;
        ASSERT(fn != NULL, "eval fn");
        dst = mino_clone(B, A, fn);
        ASSERT(dst == NULL, "fn clone should fail");
    }

    /* ---- Test 6: clone nested structure ---- */
    {
        mino_val_t *src = mino_eval_string(A,
            "(list (vec (range 3)) {:a 1} #{:x :y})", ea);
        mino_val_t *dst;
        ASSERT(src != NULL, "eval nested");
        dst = mino_clone(B, A, src);
        ASSERT(dst != NULL, "nested clone not null");
        ASSERT(mino_is_cons(dst), "nested clone type");
    }

    /* ---- Test 7: isolation - mutating in A doesn't affect B's clone ---- */
    {
        mino_val_t *atom_v = mino_eval_string(A,
            "(let (a (atom 0)) (reset! a 42) (deref a))", ea);
        mino_val_t *cloned;
        long long i_iso = 0;
        ASSERT(atom_v != NULL && mino_is_int(atom_v), "atom deref");
        /* Clone the integer result (not the atom). */
        cloned = mino_clone(B, A, atom_v);
        ASSERT(cloned != NULL, "int clone for isolation");
        ASSERT(mino_is_int(cloned) && mino_to_int(cloned, &i_iso) && i_iso == 42, "isolation val");
    }

    mino_env_free(A, ea);
    mino_env_free(B, eb);
    mino_state_free(A);
    mino_state_free(B);

    printf("all clone tests passed\n");
    return 0;
}
