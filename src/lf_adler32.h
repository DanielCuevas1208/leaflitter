#ifndef LF_ADLER32_H
#define LF_ADLER32_H

#include "lf_common.h"

/* Computes the Adler-32 checksum of `data`. An empty input yields 1. */
LF_EXPORT uint32_t lf_adler32(const void *data, size_t len);

#endif /* LF_ADLER32_H */
