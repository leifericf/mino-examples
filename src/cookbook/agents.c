/*
 * agents.c - drive mino agents from C host code.
 *
 * Demonstrates the public C-API perimeter for agents: mino_send,
 * mino_send_off, mino_await, mino_await_for, mino_agent_error,
 * mino_restart_agent. The host raises the thread limit so the
 * per-state worker threads can spawn (one per pool), then drives
 * an asynchronous counter without going through eval_string.
 *
 * mino's per-state eval lock means actions across the POOLED and
 * SOLO pools serialize, but the queues are independent: a long
 * send-off action doesn't stall pending sends, and vice versa.
 *
 * Build:  cc -std=c99 -I.. -o agents agents.c <lib srcs>.c
 * Run:    ./agents
 */

#include "mino.h"
#include <stdio.h>

int main(void)
{
    mino_state_t *S   = mino_state_new();
    mino_env_t   *env = mino_env_new_default(S);

    /* Each agent pool needs its own worker thread (the embedder
     * thread does not count). For both POOLED and SOLO concurrently,
     * grant at least 2; we ask for 4 here to leave headroom for any
     * additional futures. The standalone REPL bumps thread_limit to
     * cpu_count automatically; embedders opt in explicitly. */
    mino_set_thread_limit(S, 4);
    mino_install(S, env, MINO_CAP_AGENT);

    /* (fn [v] (inc v)) - increment the agent's value. */
    mino_val_t *inc_fn = mino_eval_string(S, "(fn [v] (inc v))", env);

    /* Construct an agent holding 0. The owning state is recorded so
     * cross-state misuse throws MST007 at the C boundary. */
    mino_val_t *counter = mino_agent(S, mino_int(S, 0));

    /* Fire two sends onto POOLED, one send-off onto SOLO. mino_send
     * and mino_send_off return the agent immediately; the action
     * runs on the worker. */
    mino_send(S, counter, inc_fn, NULL);
    mino_send(S, counter, inc_fn, NULL);
    mino_send_off(S, counter, inc_fn, NULL);

    /* Block until the run-queues drain. The NULL-terminated array
     * lets one await wait on multiple agents at once. */
    {
        mino_val_t *agents[2];
        agents[0] = counter;
        agents[1] = NULL;
        mino_await(S, agents);
    }

    printf("counter after 2 send + 1 send-off = ");
    mino_println(S, mino_agent_deref(counter));  /* prints 3 */

    /* mino_await_for returns 1 if every agent reaches zero in-flight
     * before the deadline, 0 on timeout. The trivial path returns 1
     * immediately when nothing is queued. */
    {
        mino_val_t *agents[2];
        agents[0] = counter;
        agents[1] = NULL;
        printf("await_for with empty queue: %d (expect 1)\n",
               mino_await_for(S, 50, agents));
    }

    /* Failure handling. Install a validator that rejects negatives,
     * then queue a (dec) action. The validator throws and the agent
     * latches the error. */
    {
        mino_val_t *guarded = mino_eval_string(S,
            "(agent 0 :validator (fn [v] (>= v 0)))", env);
        mino_val_t *dec_fn  = mino_eval_string(S, "(fn [v] (dec v))", env);
        mino_val_t *err;
        mino_val_t *agents[2];
        agents[0] = guarded;
        agents[1] = NULL;

        mino_send(S, guarded, dec_fn, NULL);
        mino_await(S, agents);

        err = mino_agent_error(S, guarded);
        printf("guarded agent error: %s\n",
               err != NULL ? "captured" : "(clean)");

        /* Subsequent send to a failed :fail agent throws. The C-API
         * publishes the throw via the state and returns NULL. */
        if (mino_send(S, guarded, dec_fn, NULL) == NULL) {
            printf("send to failed agent threw: %s\n", mino_last_error(S));
        }

        /* Restart with a clean value. clear_actions=1 also drops any
         * actions that were queued before the failure latched. */
        mino_restart_agent(S, guarded, mino_int(S, 5), 1);
        printf("guarded agent after restart: ");
        mino_println(S, mino_agent_deref(guarded));  /* prints 5 */
    }

    /* Quiesce both pool workers before tearing down the state.
     * mino_state_free does this implicitly, but calling it
     * explicitly here mirrors how a long-running embedder shuts
     * down a single mino_state during its lifecycle. */
    mino_eval_string(S, "(shutdown-agents)", env);

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
