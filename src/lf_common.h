#ifndef LF_COMMON_H
#define LF_COMMON_H

#include <stddef.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define LF_EXPORT
#define LF_INLINE static __forceinline
#else
#define LF_EXPORT
#define LF_INLINE static inline
#endif

#define LF_VERSION_MAJOR 1
#define LF_VERSION_MINOR 0
#define LF_VERSION_PATCH 0

#define LF_MAGIC_0 0x4Cu /* 'L' */
#define LF_MAGIC_1 0x46u /* 'F' */
#define LF_MAGIC_2 0x01u

#define LF_FORMAT_VERSION 1u

#define LF_MAX_WINDOW 32768u
#define LF_MAX_BLOCK (256u * 1024u)
#define LF_DEFAULT_BLOCK (64u * 1024u)
#define LF_DEFAULT_WINDOW LF_MAX_WINDOW
#define LF_DEFAULT_MAX_CHAIN 512u

#define LF_MIN_MATCH 3u
#define LF_MAX_MATCH 258u
#define LF_NUM_LITLEN 288u
#define LF_NUM_DIST 30u
#define LF_EOB 256u
#define LF_FIRST_LENGTH_CODE 257u

#define LF_MAX_CODE_LEN 15u

/* Position 0 means "no previous position with this hash". */
#define LF_LZ_NONE 0u

typedef enum lf_result {
    LF_OK = 0,
    LF_ERR_ALLOC,
    LF_ERR_PARAM,
    LF_ERR_IO,
    LF_ERR_BAD_MAGIC,
    LF_ERR_UNSUPPORTED_VERSION,
    LF_ERR_TRUNCATED,
    LF_ERR_INVALID_DATA,
    LF_ERR_BAD_LENGTH,
    LF_ERR_BUFFER_FULL
} lf_result;

typedef struct lf_options {
    uint32_t block_size;
    uint32_t window_size;
    uint32_t max_chain;
} lf_options;

LF_EXPORT const char *lf_version_string(void);

LF_EXPORT void lf_options_default(lf_options *opt);

LF_EXPORT const char *lf_result_name(lf_result r);

#endif /* LF_COMMON_H */
