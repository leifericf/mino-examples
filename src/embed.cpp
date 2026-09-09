/*
 * embed.cpp -- C++ host interop with mino.
 *
 * Build (from the mino-examples root):
 *
 *   ./mino/mino task build
 *
 * Or compile just this file against the mino amalgamation:
 *
 *   c++ -std=c++17 -O2 -Imino/dist -o src/embed_cpp \
 *     src/embed.cpp mino/dist/mino.o -lm -lpthread
 */

#include "mino.h"
#include <cstdio>
#include <vector>

// --- Host type: a simple accumulator ---

struct Accumulator {
    std::vector<double> values;
    double total = 0.0;
};

static mino_val *acc_new(mino_state *S, mino_val *,
                           mino_val *, void *) {
    return mino_handle_ex(S, new Accumulator, "Accumulator",
        [](void *p, const char *) { delete static_cast<Accumulator *>(p); });
}

static mino_val *acc_add(mino_state *S, mino_val *target,
                           mino_val *args, void *) {
    auto *a = static_cast<Accumulator *>(mino_handle_ptr(target));
    double v;
    mino_to_float(mino_car(args), &v);
    a->values.push_back(v);
    a->total += v;
    return target;
}

static mino_val *acc_total(mino_state *S, mino_val *target,
                             mino_val *, void *) {
    return mino_float(S,
        static_cast<Accumulator *>(mino_handle_ptr(target))->total);
}

static mino_val *acc_count(mino_state *S, mino_val *target,
                             mino_val *, void *) {
    return mino_int(S, static_cast<long long>(
        static_cast<Accumulator *>(mino_handle_ptr(target))->values.size()));
}

int main() {
    mino_state *S   = mino_state_new();
    mino_env   *env = mino_env_new_default(S);

    // Register the Accumulator type with mino.
    mino_host_enable(S);
    mino_host_register_ctor(S, "Accumulator", 0, acc_new, nullptr);
    mino_host_register_method(S, "Accumulator", "add", 1, acc_add, nullptr);
    mino_host_register_getter(S, "Accumulator", "total", acc_total, nullptr);
    mino_host_register_getter(S, "Accumulator", "count", acc_count, nullptr);

    // mino code uses dot-syntax and tail-call recursion.
    mino_val *result = mino_eval_string(S,
        "(defn add-all [acc items]       \n"
        "  (if (empty? items)            \n"
        "    acc                         \n"
        "    (do (.add acc (first items))\n"
        "        (add-all acc (rest items)))))\n"
        "                               \n"
        "(let [acc (new Accumulator)]    \n"
        "  (add-all acc [10 20 30 40 50])\n"
        "  (/ (.-total acc)             \n"
        "     (.-count acc)))           \n",
        env);

    double avg;
    if (result && mino_to_float(result, &avg))
        printf("average: %.1f\n", avg);   // => average: 30.0

    mino_env_free(S, env);
    mino_state_free(S);
}
