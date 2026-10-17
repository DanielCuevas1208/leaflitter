#ifndef LF_MATCH_H
#define LF_MATCH_H

#include "lf_common.h"

/*
 * An LZ77 hash-chain match finder.
 * It indexes 3-byte sequences in a sliding window and finds the longest
 * previous occurrence of the data at a given position.
 */

#define LF_MATCH_HASH_BITS 15u

typedef struct lf_match {
    uint32_t *head;
    uint32_t *prev;
    uint32_t window;
    uint32_t max_chain;
    uint32_t hash_mask;
} lf_match;

/* Allocates a finder with the given window size and chain limit. */
LF_EXPORT lf_result lf_match_init(lf_match *m, uint32_t window, uint32_t max_chain);

LF_EXPORT void lf_match_free(lf_match *m);

/* Clears all hash and history entries. */
LF_EXPORT void lf_match_reset(lf_match *m);

/* Searches for the longest match at `src + pos`.
   Returns LF_OK. When no match exists, *match_len is 0 and *match_dist is 0.
   Matches never exceed LF_MAX_MATCH bytes. */
LF_EXPORT lf_result lf_match_search(lf_match *m, const uint8_t *src, size_t pos,
                                    size_t src_len, unsigned *match_len,
                                    uint32_t *match_dist);

/* Records `src + pos` so later searches can find it. */
LF_EXPORT void lf_match_insert(lf_match *m, const uint8_t *src, size_t pos,
                               size_t src_len);

#endif /* LF_MATCH_H */
