/*
 * interop_test.c -- test driver for host interop system.
 *
 * Registers mock capabilities (Counter type, Math statics), then runs
 * tests/interop_test.clj which exercises all four host primitives
 * and verifies error handling.
 *
 * Build: make test-interop
 */

#include "mino.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_finalizer(void *ptr, const char *tag)
{
    (void)tag;
    free(ptr);
}

/* --- Mock Counter type ---
 * A simple counter: new creates with value 0, inc adds 1, get returns
 * current value, "value" getter returns current value. */

typedef struct {
    long long val;
} counter_t;

static mino_val_t *counter_new(mino_state_t *S, mino_val_t *target,
                                mino_val_t *args, void *ctx)
{
    counter_t *c;
    (void)target; (void)args; (void)ctx;
    c = (counter_t *)malloc(sizeof(*c));
    if (c == NULL) return NULL;
    c->val = 0;
    return mino_handle_ex(S, c, "Counter", free_finalizer);
}

static mino_val_t *counter_inc(mino_state_t *S, mino_val_t *target,
                                mino_val_t *args, void *ctx)
{
    counter_t *c;
    (void)S; (void)args; (void)ctx;
    c = (counter_t *)mino_handle_ptr(target);
    c->val++;
    return target;
}

static mino_val_t *counter_get(mino_state_t *S, mino_val_t *target,
                                mino_val_t *args, void *ctx)
{
    counter_t *c;
    (void)args; (void)ctx;
    c = (counter_t *)mino_handle_ptr(target);
    return mino_int(S, c->val);
}

static mino_val_t *counter_add(mino_state_t *S, mino_val_t *target,
                                mino_val_t *args, void *ctx)
{
    counter_t *c;
    long long n;
    (void)S; (void)ctx;
    c = (counter_t *)mino_handle_ptr(target);
    if (!mino_is_cons(args) || !mino_to_int(mino_car(args), &n))
        return NULL;
    c->val += n;
    return target;
}

static mino_val_t *counter_value_getter(mino_state_t *S, mino_val_t *target,
                                         mino_val_t *args, void *ctx)
{
    counter_t *c;
    (void)args; (void)ctx;
    c = (counter_t *)mino_handle_ptr(target);
    return mino_int(S, c->val);
}

/* --- Mock Math statics --- */

static mino_val_t *math_add(mino_state_t *S, mino_val_t *target,
                             mino_val_t *args, void *ctx)
{
    long long a, b;
    (void)target; (void)ctx;
    if (!mino_is_cons(args) || !mino_to_int(mino_car(args), &a))
        return NULL;
    args = mino_cdr(args);
    if (!mino_is_cons(args) || !mino_to_int(mino_car(args), &b))
        return NULL;
    return mino_int(S, a + b);
}

static mino_val_t *math_pi(mino_state_t *S, mino_val_t *target,
                            mino_val_t *args, void *ctx)
{
    (void)target; (void)args; (void)ctx;
    return mino_float(S, 3.14159265358979323846);
}

/* Inline test script. Embedded as a C string so the binary runs from
 * any cwd; no external test-runner dependency. Each form is wrapped
 * in (try ... (catch e ...)) where appropriate and asserts using a
 * tiny shim defined at the top. */
static const char *interop_test_src =
    "(def passed (atom 0))\n"
    "(def failed (atom 0))\n"
    "(defn check [name actual expected]\n"
    "  (if (= actual expected)\n"
    "    (swap! passed inc)\n"
    "    (do (swap! failed inc)\n"
    "        (println \"FAIL\" name \"got\" (pr-str actual)\n"
    "                \"want\" (pr-str expected)))))\n"
    "(defn check-throws [name f]\n"
    "  (let [r (try (f) :did-not-throw (catch e :threw))]\n"
    "    (if (= r :threw)\n"
    "      (swap! passed inc)\n"
    "      (do (swap! failed inc)\n"
    "          (println \"FAIL\" name \"expected throw\")))))\n"
    "\n"
    ";; host/new + host/call + host/get\n"
    "(let [c (host/new :Counter)]\n"
    "  (check \"new-type\" (type c) :handle)\n"
    "  (host/call c :inc)\n"
    "  (check \"call-inc\"   (host/call c :get) 1)\n"
    "  (host/call c :add 5)\n"
    "  (check \"call-add\"   (host/call c :get) 6)\n"
    "  (check \"get-value\"  (host/get c :value) 6))\n"
    "\n"
    ";; host/static-call\n"
    "(check \"static-add\" (host/static-call :Math :add 3 4) 7)\n"
    "(check \"static-pi\"  (< 3.14 (host/static-call :Math :pi)) true)\n"
    "\n"
    ";; sugar shapes\n"
    "(let [c (new Counter)]\n"
    "  (check \"new-sugar\" (type c) :handle)\n"
    "  (.inc c)\n"
    "  (check \".method\"   (.get c) 1)\n"
    "  (.add c 10)\n"
    "  (check \".method-args\" (.get c) 11)\n"
    "  (check \".-field\"   (.-value c) 11))\n"
    "(check \"Type/static\" (Math/add 3 4) 7)\n"
    "\n"
    ";; error paths\n"
    "(check-throws \"new-bogus\"    (fn [] (host/new :Bogus)))\n"
    "(let [c (host/new :Counter)]\n"
    "  (check-throws \"call-bogus\" (fn [] (host/call c :bogus))))\n"
    "(check-throws \"call-int\"     (fn [] (host/call 42 :inc)))\n"
    "(check-throws \"new-arity\"    (fn [] (host/new :Counter 1 2 3)))\n"
    "\n"
    "(println \"interop_test: passed\" @passed \"failed\" @failed)\n"
    "@failed";

int main(int argc, char **argv)
{
    mino_state_t *S;
    mino_env_t   *env;
    mino_val_t   *result;
    long long     failed = 0;

    (void)argc;
    (void)argv;

    S   = mino_state_new();
    env = mino_env_new(S);
    /* HOST cap exposes host/new, host/call, host/get, host/static-call;
     * MINO_CAP_DEFAULT covers the sandbox-safe core surface needed by
     * the script (deftest, fns, atoms, println). */
    mino_install(S, env, MINO_CAP_DEFAULT | MINO_CAP_HOST);

    /* Register Counter type. */
    mino_host_register_ctor(S, "Counter", 0, counter_new, NULL);
    mino_host_register_method(S, "Counter", "inc", 0, counter_inc, NULL);
    mino_host_register_method(S, "Counter", "get", 0, counter_get, NULL);
    mino_host_register_method(S, "Counter", "add", 1, counter_add, NULL);
    mino_host_register_getter(S, "Counter", "value", counter_value_getter, NULL);

    /* Register Math statics. */
    mino_host_register_static(S, "Math", "add", 2, math_add, NULL);
    mino_host_register_static(S, "Math", "pi", 0, math_pi, NULL);

    /* Enable interop dispatch. */
    mino_host_enable(S);

    /* Run the embedded test script. */
    result = mino_eval_string(S, interop_test_src, env);
    if (result == NULL) {
        fprintf(stderr, "interop test failed: %s\n", mino_last_error(S));
        mino_env_free(S, env);
        mino_state_free(S);
        return 1;
    }
    mino_to_int(result, &failed);

    mino_env_free(S, env);
    mino_state_free(S);
    return (int)failed;
}
