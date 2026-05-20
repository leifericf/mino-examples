/*
 * iterate.c -- walk every kind of mino collection from C with mino_iter.
 *
 * The iterator covers vectors, maps (hashed and sorted), sets, lists,
 * lazy seqs, and chunked seqs through one API. For map-shaped
 * collections both out_k and out_v are populated; for the others,
 * out_k is the element and out_v stays NULL.
 *
 * Build:  cc -std=c99 -I.. -o iterate iterate.c ../mino.c
 * Run:    ./iterate
 */

#include "mino.h"
#include <stdio.h>
#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#endif

static long long count_seq(mino_state *S, mino_val *coll)
{
    mino_iter *it = (mino_iter *)alloca(mino_iter_sizeof());
    long long n = 0;
    mino_iter_init(S, it, coll);
    while (mino_iter_next(it, NULL, NULL)) n++;
    mino_iter_done(it);
    return n;
}

static long long sum_seq(mino_state *S, mino_val *coll)
{
    mino_iter *it = (mino_iter *)alloca(mino_iter_sizeof());
    long long total = 0, x = 0;
    mino_val *v;
    mino_iter_init(S, it, coll);
    while (mino_iter_next(it, &v, NULL)) {
        if (mino_to_int(v, &x)) total += x;
    }
    mino_iter_done(it);
    return total;
}

int main(void)
{
    mino_state *S   = mino_state_new();
    mino_env   *env = mino_env_new_default(S);

    /* Vector walk: out_k is the element, out_v is NULL. */
    {
        mino_val *vec = mino_eval_string(S, "[10 20 30 40 50]", env);
        printf("vector count = %lld\n", count_seq(S, vec));   /* 5 */
        printf("vector sum   = %lld\n", sum_seq(S, vec));     /* 150 */
    }

    /* Map walk: both out_k and out_v populated, insertion order. */
    {
        mino_val *m = mino_eval_string(S,
            "{:a 1 :b 2 :c 3}", env);
        mino_iter *it = (mino_iter *)alloca(mino_iter_sizeof());
        mino_val  *k, *v;
        mino_iter_init(S, it, m);
        printf("map entries:\n");
        while (mino_iter_next(it, &k, &v)) {
            const char *kn; size_t klen;
            long long   vn = 0;
            mino_to_keyword(k, &kn, &klen);
            mino_to_int(v, &vn);
            printf("  :%.*s -> %lld\n", (int)klen, kn, vn);
        }
        mino_iter_done(it);
    }

    /* Lazy seq walk: forced on demand. */
    {
        mino_val *r = mino_eval_string(S, "(range 1 11)", env);
        printf("lazy sum 1..10 = %lld\n", sum_seq(S, r));     /* 55 */
    }

    /* Cons list walk. */
    {
        mino_val *lst = mino_cons(S, mino_int(S, 7),
                          mino_cons(S, mino_int(S, 8),
                          mino_cons(S, mino_int(S, 9), mino_nil(S))));
        printf("list sum     = %lld\n", sum_seq(S, lst));     /* 24 */
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
