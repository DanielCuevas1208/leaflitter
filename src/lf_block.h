#ifndef LF_BLOCK_H
#define LF_BLOCK_H

#include "lf_bitio.h"
#include "lf_common.h"
#include "lf_match.h"

/*
 * Block-level compression.
 * A block holds one unit of Huffman-compressed data. Blocks are independent:
 * each carries its own code-length description and token stream. A final flag
 * marks the last block of a stream.
 *
 * The encoder tokenizes `src[base .. base+len)` with the caller-owned match
 * finder, so matches may reference earlier blocks. It then emits either a
 * stored block or a Huffman block, whichever is smaller.
 */

/* Encodes one block. `src` is the full input buffer, `base` is the block
   start, `len` is the block length, and `total` is the full input length.
   `final` marks the last block of the stream. */
LF_EXPORT lf_result lf_block_encode(lf_options *opt, lf_match *m,
                                    const uint8_t *src, size_t base, size_t len,
                                    size_t total, int final, lf_buf *out);

/* Decodes one block from `r` into `out`.
   `max_out` bounds the output length and `window` bounds the match distance.
   Sets `*final` from the block header. */
LF_EXPORT lf_result lf_block_decode(lf_breader *r, lf_buf *out, size_t max_out,
                                    uint32_t window, int *final);

#endif /* LF_BLOCK_H */
