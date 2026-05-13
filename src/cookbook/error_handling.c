/*
 * error_handling.c -- structured error access and the protected-call
 * matrix from C.
 *
 * Two complementary surfaces:
 *   mino_error_kind / mino_error_code / mino_last_error  inspect the
 *     classified diagnostic after a call that returned NULL.
 *   mino_eval_ex / mino_eval_string_ex / mino_pcall      run with a
 *     try-frame installed so a throw caught here surfaces as out_ex
 *     without poisoning mino_last_error for subsequent calls.
 *
 * Build:  cc -std=c99 -I.. -o error_handling error_handling.c ../mino.c
 * Run:    ./error_handling
 */

#include "mino.h"
#include <stdio.h>

static void report_diag(mino_state_t *S, const char *banner)
{
    const char *msg  = mino_last_error(S);
    const char *kind = mino_error_kind(S);
    const char *code = mino_error_code(S);
    printf("%s\n", banner);
    printf("  kind: %s\n", kind != NULL ? kind : "(none)");
    printf("  code: %s\n", code != NULL ? code : "(none)");
    printf("  msg:  %s\n", msg  != NULL ? msg  : "(none)");
    mino_clear_error(S);
}

int main(void)
{
    mino_state_t *S   = mino_state_new();
    mino_env_t   *env = mino_env_new_default(S);

    /* Read a malformed form: classified as a reader error. */
    {
        mino_val_t *r = mino_eval_string(S, "(+ 1", env);
        printf("\n[unbalanced parens] r = %s\n",
               r == NULL ? "NULL (expected)" : "non-NULL");
        report_diag(S, "structured access:");
    }

    /* Type error from arithmetic: classified eval/type. */
    {
        mino_val_t *r = mino_eval_string(S, "(+ 1 :two)", env);
        printf("\n[type error] r = %s\n",
               r == NULL ? "NULL (expected)" : "non-NULL");
        report_diag(S, "structured access:");
    }

    /* Protected eval_string. The throw is caught via out_ex; the raw
     * payload (the value the user passed to (throw ...)) is surfaced
     * directly instead of via the diagnostic round-trip. */
    {
        mino_val_t *out = NULL;
        mino_val_t *ex  = NULL;
        int rc = mino_eval_string_ex(S,
            "(throw (ex-info \"bad input\" {:value -1}))",
            env, &out, &ex);
        printf("\n[pcall caught throw] rc = %d\n", rc);
        if (ex != NULL) {
            char buf[256];
            mino_print_to_buf(S, ex, buf, sizeof(buf));
            printf("  out_ex: %s\n", buf);
        }
    }

    /* Successful protected eval distinguishes "real nil" from "error". */
    {
        mino_val_t *out = NULL;
        mino_val_t *ex  = NULL;
        int rc = mino_eval_string_ex(S, "nil", env, &out, &ex);
        printf("\n[pcall returns nil] rc = %d, out = %p, ex = %p\n",
               rc, (void *)out, (void *)ex);
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
