/*
 * plugins.cpp - load user scripts with controlled capabilities.
 *
 * A document processing system where C++ provides the document model
 * and plugins define transformation rules. Each plugin runs in its
 * own sandboxed environment with only the capabilities it needs.
 * The host calls plugin functions directly via mino_call.
 *
 * Build:
 *   make
 *   c++ -std=c++17 -Isrc -o examples/use-cases/plugins \
 *       examples/use-cases/plugins.cpp src/[a-z]*.o -lm
 */

#include "mino.h"
#include <cstdio>
#include <vector>

/* ── Expose ────────────────────────────────────────────────────────── */

/* Documents are mino maps with `:title`, `:body`, `:tags` keys.
 * Plugins receive immutable documents and return transformed copies.
 * The plugin cannot modify the original through the value it receives. */

static mino_val *make_document(mino_state *S,
                                 const char *title,
                                 const char *body,
                                 const std::vector<const char *> &tags)
{
    std::vector<mino_val *> tag_vals;
    for (auto *t : tags)
        tag_vals.push_back(mino_keyword(S, t));
    mino_val *tag_vec = mino_vector(S, tag_vals.data(), tag_vals.size());

    mino_val *ks[3], *vs[3];
    ks[0] = mino_keyword(S, "title"); vs[0] = mino_string(S, title);
    ks[1] = mino_keyword(S, "body");  vs[1] = mino_string(S, body);
    ks[2] = mino_keyword(S, "tags");  vs[2] = tag_vec;
    return mino_map(S, ks, vs, 3);
}

/* ── Script (plugin 1: metadata enrichment) ────────────────────────── */

static const char *metadata_plugin =
    ";; Add word count and reading time to a document.\n"
    "(defn enrich [doc]\n"
    "  (let [words (count (split (:body doc) \" \"))\n"
    "        minutes (max 1 (quot words 200))]\n"
    "    (-> doc\n"
    "        (assoc :word-count words)\n"
    "        (assoc :reading-time minutes))))\n";

/* ── Script (plugin 2: tag-based filtering) ────────────────────────── */

static const char *filter_plugin =
    ";; Keep only documents matching a tag set.\n"
    "(defn match? [doc required-tags]\n"
    "  (some required-tags (:tags doc)))\n"
    "\n"
    "(defn filter-docs [docs required-tags]\n"
    "  (->> docs\n"
    "       (filter (fn [d] (match? d required-tags)))\n"
    "       (mapv :title)))\n";

/* ── Embed ─────────────────────────────────────────────────────────── */

/* Helper: call a mino function by name. */
static mino_val *call1(mino_state *S, mino_env *env,
                         const char *name, mino_val *arg)
{
    mino_val *fn   = mino_env_get(env, name);
    mino_val *args = mino_cons(S, arg, mino_nil(S));
    return fn ? mino_call(S, fn, args, env) : nullptr;
}

static mino_val *call2(mino_state *S, mino_env *env,
                         const char *name, mino_val *a, mino_val *b)
{
    mino_val *fn   = mino_env_get(env, name);
    mino_val *args = mino_cons(S, a, mino_cons(S, b, mino_nil(S)));
    return fn ? mino_call(S, fn, args, env) : nullptr;
}

int main()
{
    mino_state *S = mino_state_new();

    /* Build documents from the C++ side. Root each one so the GC
     * cannot collect them while subsequent allocations happen. */
    mino_ref *r1 = mino_ref_new(S, make_document(S, "Getting Started",
        "This guide walks through the initial setup process for new users",
        {"guide", "beginner"}));
    mino_ref *r2 = mino_ref_new(S, make_document(S, "API Reference",
        "Complete reference for all public functions and types in the system",
        {"reference", "api"}));
    mino_ref *r3 = mino_ref_new(S, make_document(S, "Performance Tuning",
        "Advanced techniques for optimizing throughput and reducing latency in production deployments",
        {"guide", "advanced"}));

    mino_val *doc_items[] = {mino_deref(r1), mino_deref(r2), mino_deref(r3)};
    mino_ref *docs_ref = mino_ref_new(S, mino_vector(S, doc_items, 3));
    mino_unref(S, r1);
    mino_unref(S, r2);
    mino_unref(S, r3);

    /* Plugin 1: metadata enrichment (sandboxed, no I/O). */
    {
        mino_env *env = mino_env_new(S);
        mino_install(S, env, MINO_CAP_DEFAULT);

        if (!mino_eval_string(S, metadata_plugin, env)) {
            fprintf(stderr, "plugin load error: %s\n", mino_last_error(S));
            return 1;
        }

        printf("=== metadata plugin ===\n");
        mino_env_set(S, env, "docs", mino_deref(docs_ref));
        mino_val *enriched = mino_eval_string(S,
            "(mapv enrich docs)", env);
        if (enriched)
            mino_println(S, enriched);

        mino_env_free(S, env);
    }

    /* Plugin 2: tag filter (separate sandbox). */
    {
        mino_env *env = mino_env_new(S);
        mino_install(S, env, MINO_CAP_DEFAULT);

        if (!mino_eval_string(S, filter_plugin, env)) {
            fprintf(stderr, "plugin load error: %s\n", mino_last_error(S));
            return 1;
        }

        printf("\n=== filter plugin ===\n");
        mino_val *tag_items[] = {mino_keyword(S, "guide")};
        mino_val *tags = mino_set(S, tag_items, 1);
        mino_val *result = call2(S, env, "filter-docs",
                                   mino_deref(docs_ref), tags);
        if (result) {
            printf("guides: ");
            mino_println(S, result);
        }

        mino_env_free(S, env);
    }

    /* Protected call: demonstrate error isolation. */
    {
        mino_env *env = mino_env_new(S);
        mino_install(S, env, MINO_CAP_DEFAULT);
        mino_eval_string(S, metadata_plugin, env);

        printf("\n=== error isolation ===\n");
        mino_val *bad = mino_string(S, "not a document");
        mino_val *out = nullptr;
        int rc = mino_pcall(S, mino_env_get(env, "enrich"),
                            mino_cons(S, bad, mino_nil(S)), env, &out,
                            nullptr);
        if (rc != 0)
            printf("caught: %s\n", mino_last_error(S));

        mino_env_free(S, env);
    }

    mino_unref(S, docs_ref);
    mino_state_free(S);
}
