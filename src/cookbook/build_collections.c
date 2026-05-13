/*
 * build_collections.c -- assemble persistent vectors / maps / sets one
 * element at a time using the builder facade.
 *
 * Builders wrap transients and keep the work-in-progress collection
 * rooted across GC. Use them when elements arrive incrementally
 * (parsing, network input, host events) rather than already living in
 * a C array.
 *
 * Build:  cc -std=c99 -I.. -o build_collections build_collections.c ../mino.c
 * Run:    ./build_collections
 */

#include "mino.h"
#include <stdio.h>

int main(void)
{
    mino_state_t *S   = mino_state_new();
    mino_env_t   *env = mino_env_new_default(S);

    /* Vector: open builder, push elements, finish. */
    {
        mino_vec_builder_t *b = mino_vector_builder_new(S);
        int i;
        for (i = 0; i < 5; i++) {
            mino_vector_builder_push(b, mino_int(S, i * i));
        }
        mino_val_t *v = mino_vector_builder_finish(b);
        mino_env_set(S, env, "v", v);
    }

    /* Map: open builder, put pairs, finish. */
    {
        mino_map_builder_t *b = mino_map_builder_new(S);
        mino_map_builder_put(b, mino_keyword(S, "name"),
                                mino_string(S, "alice"));
        mino_map_builder_put(b, mino_keyword(S, "age"),
                                mino_int(S, 42));
        mino_map_builder_put(b, mino_keyword(S, "active"),
                                mino_true(S));
        mino_val_t *m = mino_map_builder_finish(b);
        mino_env_set(S, env, "m", m);
    }

    /* Set: open builder, add elements, finish. Duplicates collapse. */
    {
        mino_set_builder_t *b = mino_set_builder_new(S);
        const char *colors[] = { "red", "green", "blue", "red", "green" };
        size_t i;
        for (i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
            mino_set_builder_add(b, mino_keyword(S, colors[i]));
        }
        mino_val_t *s = mino_set_builder_finish(b);
        mino_env_set(S, env, "s", s);
    }

    /* Pretty-print from mino. The builder products are full-fledged
     * persistent values; everything that works on a map / vec / set
     * literal works here too. */
    mino_eval_string(S, "(println \"v =\" v)", env);
    mino_eval_string(S, "(println \"m =\" m)", env);
    mino_eval_string(S, "(println \"s =\" s)", env);
    mino_eval_string(S, "(println \"v count =\" (count v))", env);
    mino_eval_string(S, "(println \"s count =\" (count s))", env);

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
