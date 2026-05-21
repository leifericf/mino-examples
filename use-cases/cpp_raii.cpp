/*
 * cpp_raii.cpp - mino.hpp showcase.
 *
 * Demonstrates the three RAII wrappers in mino.hpp:
 *
 *   mino::state -- owning handle for a mino_state.
 *   mino::env   -- owning handle for a mino_env created in a state.
 *   mino::pin   -- GC root for a mino_val across eval boundaries.
 *
 * Plus the eval_string and print_to_string helpers that throw
 * mino::error on failure instead of returning NULL.
 *
 * Build:
 *   make
 *   c++ -std=c++17 -Imino/src -o use-cases/cpp_raii \
 *       use-cases/cpp_raii.cpp mino/src/[a-z]*.o -lm
 */

#include "mino.h"
#include "mino.hpp"

#include <cstdio>
#include <iostream>

int main()
{
    /* Stack-allocated; both destructors fire automatically on return
     * or on a thrown exception. */
    mino::state S;
    mino::env  env = mino::env::borrow(S, mino_env_new_default(S));

    /* Pin a value across eval calls so it survives any intervening
     * GC. The pin is released when it goes out of scope. */
    mino::pin greeting(S, mino_string(S, "world"));

    /* Use raw C-API to bind. */
    mino_env_set(S, env, "subject", greeting);

    try {
        mino_val* result = mino::eval_string(S, env,
            "(str \"hello, \" subject \"!\")");
        std::cout << mino::print_to_string(S, result) << "\n";

        /* Error path: throws mino::error with the diagnostic text. */
        mino::eval_string(S, env, "(/ 1 0)");
    } catch (const mino::error& e) {
        std::cerr << "caught: " << e.what() << "\n";
    }

    /* state, env, and pin all dispose in destruct order. No manual
     * mino_state_free / mino_env_free / mino_unref calls. */
    return 0;
}
