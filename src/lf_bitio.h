#ifndef LF_BITIO_H
#define LF_BITIO_H

#include "lf_buf.h"
#include "lf_common.h"

/*
 * Bit streams use most-significant-bit-first order.
 * Within one byte, the first bit written is bit 7.
 */

typedef struct lf_bwriter {
    lf_buf *out;
    uint64_t acc;
    unsigned nbits;
} lf_bwriter;

typedef struct lf_breader {
    const uint8_t *data;
    size_t len;
    size_t pos;
    uint64_t acc;
    unsigned nbits;
    int valid;
} lf_breader;

LF_EXPORT void lf_bwriter_init(lf_bwriter *w, lf_buf *out);

/* Writes the low `nbits` bits of `value`. `nbits` must be 32 or less. */
LF_EXPORT lf_result lf_bwriter_put(lf_bwriter *w, uint32_t value, unsigned nbits);

/* Writes the Huffman code `code` of length `nbits`. */
LF_EXPORT lf_result lf_bwriter_put_code(lf_bwriter *w, uint32_t code, unsigned nbits);

/* Pads the current byte with zero bits. */
LF_EXPORT lf_result lf_bwriter_align(lf_bwriter *w);

LF_EXPORT void lf_breader_init(lf_breader *r, const void *data, size_t len);

/* Reads `nbits` bits. Returns 0 and sets the valid flag when the input ends. */
LF_EXPORT uint32_t lf_breader_get(lf_breader *r, unsigned nbits);

/* Returns 1 when the reader still has enough bits to decode safely. */
LF_INLINE int lf_breader_ok(lf_breader *r)
{
    return r->valid != 0;
}

/* Returns 1 when the reader has consumed every bit of its input. */
LF_INLINE int lf_breader_finished(lf_breader *r)
{
    return r->valid && r->pos >= r->len && r->nbits == 0;
}

LF_EXPORT size_t lf_breader_bytes_read(lf_breader *r);

/* Drops any partial byte still buffered. Call only at a byte boundary. */
LF_EXPORT void lf_breader_align(lf_breader *r);

/* Copies `n` whole bytes into `dst`. Returns 1 on success, 0 on truncation.
   Call only at a byte boundary. */
LF_EXPORT int lf_breader_read_bytes(lf_breader *r, uint8_t *dst, size_t n);

#endif /* LF_BITIO_H */
