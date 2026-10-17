#include "lf_adler32.h"

#define LF_ADLER_BASE 65521u
#define LF_ADLER_NMAX 5552u

uint32_t lf_adler32(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t a = 1;
    uint32_t b = 0;

    while (len > 0) {
        size_t chunk = len < LF_ADLER_NMAX ? len : LF_ADLER_NMAX;
        size_t i;

        len -= chunk;
        for (i = 0; i < chunk; i++) {
            a += p[i];
            b += a;
        }
        p += chunk;
        a %= LF_ADLER_BASE;
        b %= LF_ADLER_BASE;
    }
    return (b << 16) | a;
}
