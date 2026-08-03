#ifndef LF_STREAM_H
#define LF_STREAM_H

#include "lf_buf.h"
#include "lf_common.h"

/*
 * Whole-stream compression.
 * A compressed stream starts with a fixed header and continues with one or
 * more blocks. The header records the original size, an Adler-32 checksum,
 * and the window size used during compression.
 */

/* Compresses `src_len` bytes from `src` into `out`.
   `opt` may be NULL to use the defaults. The stream is fully self-contained. */
LF_EXPORT lf_result lf_compress(const lf_options *opt, const void *src,
                                size_t src_len, lf_buf *out);

/* Decompresses a stream into `out`. Rejects corrupt or truncated streams. */
LF_EXPORT lf_result lf_decompress(const void *src, size_t src_len, lf_buf *out);

/* Returns an upper bound on the compressed size of `src_len` bytes. */
LF_EXPORT size_t lf_compress_bound(size_t src_len);

#endif /* LF_STREAM_H */
