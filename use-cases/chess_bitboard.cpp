/*
 * chess_bitboard.cpp - 64-bit chess bitboards via MINO_BYTES.
 *
 * Chess engines represent each piece's occupancy as a 64-bit integer
 * where bit i is set when the piece occupies square i. Bitboards make
 * attack-generation, move-legality, and board-state queries into pure
 * bitwise operations. mino's MINO_BYTES + bit-syntax surface gives a
 * Clojure-shaped path to the same machinery without the JVM Clojure
 * `long-array` ceremony.
 *
 * In this example the host loads a position into a per-piece map of
 * bitboards (one bitstring per piece type, 64 bits each) and hands
 * them to the script. The script computes:
 *
 *   - All squares attacked by white knights.
 *   - All white pieces' combined occupancy.
 *   - Whether a specific square is empty.
 *   - A printable view of the white-pawns bitboard.
 *
 * The script never converts the bitboards to vectors or hash-sets --
 * every query stays in the bit domain, which mirrors how a real chess
 * engine would use them.
 *
 * Build:
 *   make
 *   c++ -std=c++17 -Imino/src -o use-cases/chess_bitboard \
 *       use-cases/chess_bitboard.cpp mino/src/[a-z]*.o -lm
 */

#include "mino.h"
#include <cstdio>
#include <cstring>

/* ── Expose ────────────────────────────────────────────────────────── */

/* Each bitboard is a 64-bit integer. Bit i corresponds to square i,
 * with square 0 = a1 and square 63 = h8 in little-endian rank-file
 * order (the standard layout). We store them big-endian on the wire so
 * the printed hex matches a human-readable square index. */

struct Position {
    uint64_t white_pawns;
    uint64_t white_knights;
    uint64_t white_bishops;
    uint64_t white_rooks;
    uint64_t white_queens;
    uint64_t white_king;
};

/* Initial position for the white pieces. */
static Position initial_white = {
    0x000000000000ff00ULL,   /* pawns on rank 2 */
    0x0000000000000042ULL,   /* knights on b1, g1 */
    0x0000000000000024ULL,   /* bishops on c1, f1 */
    0x0000000000000081ULL,   /* rooks on a1, h1 */
    0x0000000000000008ULL,   /* queen on d1 */
    0x0000000000000010ULL    /* king on e1 */
};

/* Pack a uint64_t into an 8-byte big-endian MINO_BYTES value. */
static mino_val *u64_to_bitboard(mino_state *S, uint64_t v)
{
    unsigned char buf[8];
    for (int i = 0; i < 8; i++) {
        buf[i] = (unsigned char)((v >> (56 - 8 * i)) & 0xff);
    }
    return mino_bytes(S, buf, 8);
}

/* Build a {:piece-type bitboard} map. */
static mino_val *make_position(mino_state *S, const Position &p)
{
    mino_val *ks[6], *vs[6];
    ks[0] = mino_keyword(S, "white-pawns");
    vs[0] = u64_to_bitboard(S, p.white_pawns);
    ks[1] = mino_keyword(S, "white-knights");
    vs[1] = u64_to_bitboard(S, p.white_knights);
    ks[2] = mino_keyword(S, "white-bishops");
    vs[2] = u64_to_bitboard(S, p.white_bishops);
    ks[3] = mino_keyword(S, "white-rooks");
    vs[3] = u64_to_bitboard(S, p.white_rooks);
    ks[4] = mino_keyword(S, "white-queens");
    vs[4] = u64_to_bitboard(S, p.white_queens);
    ks[5] = mino_keyword(S, "white-king");
    vs[5] = u64_to_bitboard(S, p.white_king);
    return mino_map(S, ks, vs, 6);
}

/* ── Script ────────────────────────────────────────────────────────── */

/* The script lifts each bitboard back to a 64-bit long via
 * (bits-get bb :offset 0 :size 64) and operates on it with mino's
 * built-in bitwise primitives. Knight attacks come from the eight
 * (file_delta, rank_delta) jump pairs; the helper builds an attack
 * mask by walking the source squares and OR-ing in each jump that
 * stays on the board. */

static const char *script =
    ";; Lift a bitboard (MINO_BYTES, 8 bytes BE) to a long.\n"
    "(defn bb->long [bb]\n"
    "  (bits-get bb :offset 0 :size 64))\n"
    "\n"
    ";; Lower a long back to an 8-byte BE bitboard.\n"
    "(defn long->bb [v]\n"
    "  (bits [v :size 64]))\n"
    "\n"
    ";; Knight move offsets as (file-delta, rank-delta) pairs.\n"
    "(def knight-jumps\n"
    "  [[1 2] [2 1] [2 -1] [1 -2] [-1 -2] [-2 -1] [-2 1] [-1 2]])\n"
    "\n"
    ";; Bitmask of all squares one knight on `sq` attacks. Squares\n"
    ";; that would fall off the board are masked out by checking the\n"
    ";; destination file / rank before OR-ing the bit in.\n"
    "(defn knight-attacks-from [sq]\n"
    "  (let [sf (rem sq 8)\n"
    "        sr (quot sq 8)]\n"
    "    (reduce (fn [m [df dr]]\n"
    "              (let [tf (+ sf df)\n"
    "                    tr (+ sr dr)]\n"
    "                (if (and (<= 0 tf 7) (<= 0 tr 7))\n"
    "                  (bit-or m (bit-shift-left 1 (+ tf (* 8 tr))))\n"
    "                  m)))\n"
    "            0 knight-jumps)))\n"
    "\n"
    ";; Union of squares attacked by every set bit in `knights`.\n"
    "(defn all-knight-attacks [knights]\n"
    "  (reduce (fn [m sq]\n"
    "            (if (bit-test knights sq)\n"
    "              (bit-or m (knight-attacks-from sq))\n"
    "              m))\n"
    "          0 (range 64)))\n"
    "\n"
    ";; Compute a printable ASCII view of a bitboard. Rank 8 prints\n"
    ";; first; '.' for empty, '#' for set.\n"
    "(defn render-bb [v]\n"
    "  (apply str\n"
    "    (for [r (range 7 -1 -1)\n"
    "          f (range 8)\n"
    "          :let [sq (+ f (* 8 r))]]\n"
    "      (str (if (bit-test v sq) \\# \\.)\n"
    "           (if (= 7 f) \"\\n\" \"\")))))\n"
    "\n"
    "(let [knights      (bb->long (:white-knights position))\n"
    "      all-white    (reduce bit-or 0 (map bb->long (vals position)))\n"
    "      attacks      (all-knight-attacks knights)\n"
    "      d4-empty?    (zero? (bit-and all-white (bit-shift-left 1 27)))]\n"
    "  {:white-knights-hex (format \"0x%016x\" knights)\n"
    "   :white-occupied-hex (format \"0x%016x\" all-white)\n"
    "   :knight-attacks-hex (format \"0x%016x\" attacks)\n"
    "   :d4-empty?  d4-empty?\n"
    "   :pawns-view (render-bb (bb->long (:white-pawns position)))})\n";

/* ── Embed ─────────────────────────────────────────────────────────── */

int main()
{
    mino_state *S = mino_state_new();
    mino_env *env = mino_env_new_default(S);

    mino_env_set(S, env, "position", make_position(S, initial_white));

    mino_val *result = mino_eval_string(S, script, env);
    if (result == NULL) {
        fprintf(stderr, "eval failed: %s\n", mino_last_error(S));
        mino_env_free(S, env);
        mino_state_free(S);
        return 1;
    }

    char buf[2048];
    int n = mino_print_to_buf(S, result, buf, sizeof buf);
    if (n > 0) {
        fwrite(buf, 1, (size_t)n, stdout);
        fputc('\n', stdout);
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
