/*
 * pipeline.c - transform data through mino expressions.
 *
 * Demonstrates: pushing C data into mino as vectors/maps, running
 * sequence operations, extracting transformed results back to C.
 * Shows how mino can serve as a lightweight ETL / data-wrangling layer.
 *
 * Build:  cc -std=c99 -I.. -o pipeline pipeline.c ../mino.c
 * Run:    ./pipeline
 */

#include "mino.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    mino_state *S   = mino_state_new();
    mino_env   *env = mino_env_new_default(S);
    mino_val *result;

    /* Build a vector of employee records from C data. */
    {
        static const char *names[]  = { "Alice", "Bob", "Carol", "Dave" };
        static const int   ages[]   = { 30, 45, 28, 52 };
        static const int salaries[] = { 75000, 92000, 68000, 110000 };
        size_t i;
        size_t n = sizeof(names) / sizeof(names[0]);
        mino_val **records = (mino_val **)malloc(n * sizeof(*records));

        for (i = 0; i < n; i++) {
            mino_val *keys[3], *vals[3];
            keys[0] = mino_keyword(S, "name");
            vals[0] = mino_string(S, names[i]);
            keys[1] = mino_keyword(S, "age");
            vals[1] = mino_int(S, ages[i]);
            keys[2] = mino_keyword(S, "salary");
            vals[2] = mino_int(S, salaries[i]);
            records[i] = mino_map(S, keys, vals, 3);
        }
        mino_env_set(S, env, "employees", mino_vector(S, records, n));
        free(records);
    }

    /* Pipeline 1: names of employees over 35. */
    printf("Over 35:\n");
    result = mino_eval_string(S,
        "(->> employees\n"
        "     (filter (fn [e] (> (get e :age) 35)))\n"
        "     (map (fn [e] (get e :name))))",
        env);
    if (result != NULL) {
        printf("  ");
        mino_println(S, result);
    }

    /* Pipeline 2: total salary budget. */
    result = mino_eval_string(S,
        "(reduce + 0 (map (fn [e] (get e :salary)) employees))",
        env);
    if (result != NULL) {
        long long total;
        if (mino_to_int(result, &total)) {
            printf("Total salary: %lld\n", total);
        }
    }

    /* Pipeline 3: sorted by salary descending. */
    printf("By salary (desc):\n");
    result = mino_eval_string(S,
        "(->> employees\n"
        "     (sort)\n"
        "     (reverse)\n"
        "     (map (fn [e] (str (get e :name) \": $\" (get e :salary)))))",
        env);
    if (result != NULL) {
        /* Walk the result list and print each line. */
        mino_val *p = result;
        while (mino_is_cons(p)) {
            const char *s;
            size_t      len;
            if (mino_to_string(mino_car(p), &s, &len)) {
                printf("  %.*s\n", (int)len, s);
            }
            p = mino_cdr(p);
        }
    }

    /* Pipeline 4: group into age brackets using into + map. */
    result = mino_eval_string(S,
        "(let [senior   (filter (fn [e] (>= (get e :age) 40)) employees)\n"
        "      junior   (filter (fn [e] (<  (get e :age) 40)) employees)]\n"
        "  {:senior (map (fn [e] (get e :name)) senior)\n"
        "   :junior (map (fn [e] (get e :name)) junior)})",
        env);
    if (result != NULL) {
        printf("Age groups: ");
        mino_println(S, result);
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
