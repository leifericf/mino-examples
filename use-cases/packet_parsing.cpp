/*
 * packet_parsing.cpp - parse network packets with bit-syntax binding.
 *
 * Erlang's bit syntax is the canonical example of "binary data as a
 * first-class data type"; the classic showcase is IPv4 header parsing
 * because the fields don't align to byte boundaries (4-bit version, 4-
 * bit IHL, 6-bit DSCP, 2-bit ECN, ...). mino's `let-bits` macro and
 * `bits-get` primitive bring the same surface to embedded Clojure
 * scripts.
 *
 * In this example the host loads a captured packet into a MINO_BYTES
 * value and hands it to the script. The script destructures the IPv4
 * header field-by-field, validates the version, and returns a map of
 * decoded fields for the host to display. Nothing here is JVM-shaped
 * -- there's no java.nio.ByteBuffer, no DataInputStream, no manual
 * shift / mask arithmetic. The bit syntax IS the parser.
 *
 * Build:
 *   make
 *   c++ -std=c++17 -Imino/src -o use-cases/packet_parsing \
 *       use-cases/packet_parsing.cpp mino/src/[a-z]*.o -lm
 */

#include "mino.h"
#include <cstdio>
#include <cstring>

/* ── Expose ────────────────────────────────────────────────────────── */

/* A captured 20-byte IPv4 header. Hand-crafted so each field has a
 * recognisable value:
 *   version  = 4
 *   IHL      = 5 (20 bytes, no options)
 *   DSCP     = 0
 *   ECN      = 0
 *   total    = 84
 *   id       = 0x1234
 *   flags    = 0b010 (Don't Fragment)
 *   fragoff  = 0
 *   TTL      = 64
 *   protocol = 6 (TCP)
 *   checksum = 0xabcd
 *   src IP   = 10.0.0.1
 *   dst IP   = 10.0.0.2
 */
static const unsigned char ipv4_packet[] = {
    0x45, 0x00, 0x00, 0x54,   /* version|IHL | DSCP|ECN | total length */
    0x12, 0x34, 0x40, 0x00,   /* id          | flags|frag offset       */
    0x40, 0x06, 0xab, 0xcd,   /* TTL | protocol | checksum             */
    0x0a, 0x00, 0x00, 0x01,   /* source IP                              */
    0x0a, 0x00, 0x00, 0x02    /* destination IP                         */
};

/* Hand the captured packet to the script as a MINO_BYTES value. */
static mino_val *make_packet(mino_state *S)
{
    return mino_bytes(S, ipv4_packet, sizeof ipv4_packet);
}

/* ── Script ────────────────────────────────────────────────────────── */

/* let-bits binds each field at a running bit offset. Sub-byte fields
 * (version, IHL, DSCP, ECN, flags, fragment offset) are read directly
 * via :size N -- no shift / mask boilerplate. The trailing `:type
 * :bytes` segment binds the IP address octets as a fresh bytes value;
 * a helper builds the dotted-quad string from that. */

static const char *script =
    ";; Format four bytes as a dotted-quad IP string.\n"
    "(defn ip-str [b]\n"
    "  (str (aget b 0) \".\" (aget b 1) \".\" (aget b 2) \".\" (aget b 3)))\n"
    "\n"
    ";; Decode an IPv4 header out of `packet`.\n"
    "(defn decode-ipv4 [packet]\n"
    "  (let-bits [packet\n"
    "             [version  :size 4]\n"
    "             [ihl      :size 4]\n"
    "             [dscp     :size 6]\n"
    "             [ecn      :size 2]\n"
    "             [total    :size 16]\n"
    "             [id       :size 16]\n"
    "             [flags    :size 3]\n"
    "             [frag-off :size 13]\n"
    "             [ttl      :size 8]\n"
    "             [proto    :size 8]\n"
    "             [checksum :size 16]\n"
    "             [src      :size 32 :type :bytes]\n"
    "             [dst      :size 32 :type :bytes]]\n"
    "    (when (not= 4 version)\n"
    "      (throw (ex-info \"not an IPv4 packet\" {:got-version version})))\n"
    "    {:version  version\n"
    "     :ihl      ihl\n"
    "     :dscp     dscp\n"
    "     :ecn      ecn\n"
    "     :total    total\n"
    "     :id       id\n"
    "     :dont-fragment? (bit-test flags 1)\n"
    "     :more-fragments? (bit-test flags 0)\n"
    "     :frag-off frag-off\n"
    "     :ttl      ttl\n"
    "     :proto    (case proto 6 :tcp 17 :udp 1 :icmp :other)\n"
    "     :checksum (format \"0x%04x\" checksum)\n"
    "     :src      (ip-str src)\n"
    "     :dst      (ip-str dst)}))\n"
    "\n"
    "(decode-ipv4 captured)\n";

/* ── Embed ─────────────────────────────────────────────────────────── */

int main()
{
    mino_state *S = mino_state_new();
    mino_env *env = mino_env_new_default(S);

    mino_env_set(S, env, "captured", make_packet(S));

    mino_val *result = mino_eval_string(S, script, env);
    if (result == NULL) {
        fprintf(stderr, "eval failed: %s\n", mino_last_error(S));
        mino_env_free(S, env);
        mino_state_free(S);
        return 1;
    }

    char buf[1024];
    int n = mino_print_to_buf(S, result, buf, sizeof buf);
    if (n > 0) {
        fwrite(buf, 1, (size_t)n, stdout);
        fputc('\n', stdout);
    }

    mino_env_free(S, env);
    mino_state_free(S);
    return 0;
}
