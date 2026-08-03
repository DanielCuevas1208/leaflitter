#ifndef LF_HUFFMAN_H
#define LF_HUFFMAN_H

#include "lf_bitio.h"
#include "lf_common.h"

/*
 * A canonical Huffman coder with a fixed maximum code length.
 * The code length limit keeps the bit packing simple and small.
 */

/* Builds optimal code lengths for symbols 0..n-1. Freq must be provided.
   Lengths are capped at LF_MAX_CODE_LEN. Symbols with zero frequency get
   length 0. */
LF_EXPORT void lf_huff_lengths(uint8_t *lengths, const uint32_t *freq, uint32_t n);

/* Fills codes[] from lengths[]. Returns the maximum code length. */
LF_EXPORT unsigned lf_huff_canonical(uint32_t *codes, const uint8_t *lengths, uint32_t n);

typedef struct lf_huff_dec {
    uint8_t counts[LF_MAX_CODE_LEN + 1];
    uint16_t symbols[LF_NUM_LITLEN];
    uint32_t nsym;
    unsigned maxlen;
    int valid;
} lf_huff_dec;

LF_EXPORT void lf_huff_dec_init(lf_huff_dec *d, const uint8_t *lengths, uint32_t n);

/* Decodes one symbol. Returns the symbol or -1 for invalid data. */
LF_EXPORT int lf_huff_dec_symbol(lf_huff_dec *d, lf_breader *r);

#endif /* LF_HUFFMAN_H */
