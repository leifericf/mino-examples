/*
 * regex_thread_test.c -- concurrent isolated-state regex example.
 *
 * mino's supported concurrency model is one isolated mino_state per
 * thread: states share no mutable runtime, so N threads may each run
 * their own interpreter in parallel. This example proves regex works
 * correctly under that model. Each thread creates its own state,
 * installs the regex capability, and repeatedly evaluates a regex form
 * in a loop, asserting the result is correct on every iteration.
 *
 * Public API only: it includes "mino.h" and links dist/mino.o.
 *
 * Build: ./mino/mino task build
 * Requires pthreads (POSIX).
 */

#define _POSIX_C_SOURCE 200809L
#include "mino.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define THREADS 4
#define ITERATIONS 5000

/* (re-find (re-pattern "a.c") "aXcabc") matches the first "a.c" run. */
static const char *REGEX_FORM =
    "(re-find (re-pattern \"a.c\") \"aXcabc\")";
static const char *EXPECTED = "aXc";

static int thread_ok[THREADS];

static void *run_isolated(void *arg)
{
    int slot = *(int *)arg;
    int i;

    mino_state *S = mino_state_new();
    mino_env *env;
    if (S == NULL) { thread_ok[slot] = 0; return NULL; }

    env = mino_env_new_default(S);
    if (env == NULL) { mino_state_free(S); thread_ok[slot] = 0; return NULL; }
    mino_install(S, env, MINO_CAP_REGEX);

    thread_ok[slot] = 1;
    for (i = 0; i < ITERATIONS; i++) {
        char buf[64];
        mino_val *v = mino_eval_string(S, REGEX_FORM, env);
        if (v == NULL) { thread_ok[slot] = 0; break; }
        if (mino_print_to_buf(S, v, buf, sizeof buf) < 0) {
            thread_ok[slot] = 0;
            break;
        }
        /* The printed form of a string is quoted: "aXc". */
        if (buf[0] != '"' || strncmp(buf + 1, EXPECTED, strlen(EXPECTED)) != 0
            || buf[1 + strlen(EXPECTED)] != '"') {
            thread_ok[slot] = 0;
            break;
        }
    }

    mino_state_free(S);
    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];
    int slots[THREADS];
    int i;
    int all_ok = 1;
    int started = 0;

    printf("Regex under concurrent isolated states "
           "(%d threads, %d iterations each)\n", THREADS, ITERATIONS);
    for (i = 0; i < THREADS; i++) {
        slots[i] = i;
        if (pthread_create(&threads[i], NULL, run_isolated, &slots[i]) != 0) {
            fprintf(stderr, "failed to create thread %d\n", i);
            break;
        }
        started++;
    }

    for (i = 0; i < started; i++) {
        pthread_join(threads[i], NULL);
    }

    if (started < THREADS) {
        return 1;
    }

    for (i = 0; i < THREADS; i++) {
        printf("  thread %d: %s\n", i, thread_ok[i] ? "OK" : "FAIL");
        if (!thread_ok[i]) all_ok = 0;
    }

    if (all_ok) {
        printf("PASSED\n");
        return 0;
    }
    printf("FAILED\n");
    return 1;
}
