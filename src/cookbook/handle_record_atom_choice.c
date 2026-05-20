/*
 * handle_record_atom_choice.c - decision tree for "how do I expose
 * my host type to mino script?".
 *
 * This example is the answer to the Lua-metatable / Janet-`put`
 * question: when a host value needs to appear in mino script, which
 * cell shape do you reach for? The short answer is one of three:
 *
 *   Value-shaped data         -> mino_defrecord + mino_record
 *   Identity-shaped resource  -> mino_handle (an opaque void*)
 *   Mutable identity          -> mino_handle wrapped in an atom
 *
 * The decision tree:
 *
 *   1. Does your value have inherent identity (a file descriptor, a
 *      socket, a database connection, a GPU resource)?
 *
 *        YES -> use mino_handle. The value is opaque from script;
 *               the host owns the lifecycle and exposes operations
 *               via primitives that take the handle and return new
 *               values. If state mutates over time, wrap the handle
 *               in an atom for principled access.
 *
 *        NO  -> proceed to step 2.
 *
 *   2. Does your value look like a Clojure record from script: a
 *      named type with named fields, structural equality, hashable,
 *      participates in protocol dispatch?
 *
 *        YES -> use mino_defrecord to declare the type and
 *               mino_record to construct instances. Storage is
 *               field slots (not a backing map); script-side
 *               (:field-name r) and (= r1 r2) work as expected;
 *               protocol dispatch uses the type identity.
 *
 *        NO  -> proceed to step 3.
 *
 *   3. Is your value a small bag of named fields you don't need to
 *      type-name? Use a mino_map (plain map) -- no host-side cell
 *      shape required. The script side does not see "your type",
 *      it sees a map with the same keys you'd put in a record.
 *
 * What you do NOT do: there is no metatable surface on handles.
 * mino respects Clojure's promise that values do not mutate behind
 * your back. If you reach for the metatable answer, the right
 * response is one of:
 *
 *   - record + protocol  (script-side dispatch by type)
 *   - record + multimethod  (script-side dispatch by arbitrary dispatch fn)
 *   - atom around a record/map  (mutable identity layered on values)
 *
 * Demonstrates: mino_handle for an "fd-like" opaque resource,
 * mino_defrecord + mino_record for value-shaped Vec3, and atom-
 * wrapped state for mutable identity.
 *
 * Build (from repo root):
 *   make src/cookbook/handle_record_atom_choice
 *
 * Or via the amalgamation:
 *   cc -std=c99 -O2 -Imino/dist src/cookbook/handle_record_atom_choice.c \
 *      dist/mino.o -lm -lpthread -o src/cookbook/handle_record_atom_choice
 */

#include "mino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Path 1: mino_handle for an identity-shaped resource.                      */
/* ------------------------------------------------------------------------- */

typedef struct {
    int  id;
    char name[64];
} host_resource;

/* The handle carries a pointer + a stable type-tag string. Equality
 * and hash are pointer-shaped: two handles are equal iff they wrap
 * the same C pointer. The script side cannot mutate the pointed-to
 * struct; the host exposes operations through primitives. */
static mino_val *make_handle(mino_state *S, host_resource *r)
{
    return mino_handle(S, r, "host-resource");
}

static mino_val *prim_resource_name(mino_state *S, mino_val *args,
                                      mino_env *env)
{
    mino_val      *h;
    host_resource *r;
    (void)env;
    if (mino_args_parse(S, "resource-name", args, "H", &h) != 0) return NULL;
    r = (host_resource *)mino_handle_ptr(h);
    return mino_string(S, r != NULL ? r->name : "<null>");
}

/* ------------------------------------------------------------------------- */
/* Path 2: mino_defrecord for a value-shaped Vec3.                           */
/* ------------------------------------------------------------------------- */

static mino_val *define_vec3(mino_state *S, mino_env *env)
{
    const char *fields[3] = {"x", "y", "z"};
    mino_val   *T = mino_defrecord(S, "user", "Vec3", fields, 3);
    mino_env_set(S, env, "Vec3", T);
    return T;
}

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */

int main(void)
{
    mino_state *S   = mino_state_new();
    mino_env   *env = mino_env_new_default(S);

    /* Path 1: handle */
    {
        host_resource res = {.id = 42};
        snprintf(res.name, sizeof(res.name), "tcp-socket-42");
        mino_register_fn(S, env, "resource-name", prim_resource_name);
        mino_env_set(S, env, "*res*", make_handle(S, &res));
        mino_val *r = mino_eval_string(S, "(resource-name *res*)", env);
        const char *got = NULL;
        size_t      len = 0;
        if (r == NULL || !mino_to_string(r, &got, &len)) {
            fprintf(stderr, "handle path failed\n"); return 1;
        }
        printf("handle path : (resource-name *res*) = %.*s\n",
               (int)len, got);
    }

    /* Path 2: record */
    {
        mino_val *T    = define_vec3(S, env);
        mino_val *vals[3];
        mino_val *v;
        mino_val *r;
        long long got = 0;
        vals[0] = mino_int(S, 1);
        vals[1] = mino_int(S, 2);
        vals[2] = mino_int(S, 3);
        v = mino_record(S, T, vals, 3);
        mino_env_set(S, env, "v", v);
        r = mino_eval_string(S,
            "(+ (:x v) (:y v) (:z v))", env);
        if (r == NULL || !mino_to_int(r, &got)) {
            fprintf(stderr, "record path failed\n"); return 1;
        }
        printf("record path : (sum of fields) = %lld (expected 6)\n", got);
        if (got != 6) return 1;
    }

    /* Path 3: atom around the record for mutable identity. */
    {
        mino_val *r;
        long long got = 0;
        r = mino_eval_string(S,
            "(do "
            "  (def counter (atom 0)) "
            "  (swap! counter inc) "
            "  (swap! counter inc) "
            "  (swap! counter inc) "
            "  @counter)",
            env);
        if (r == NULL || !mino_to_int(r, &got) || got != 3) {
            fprintf(stderr, "atom path failed\n"); return 1;
        }
        printf("atom path   : (counter after 3 swaps) = %lld\n", got);
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
