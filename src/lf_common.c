#include "lf_common.h"

const char *lf_version_string(void)
{
    return "1.1.0";
}

void lf_options_default(lf_options *opt)
{
    if (opt == NULL) {
        return;
    }
    opt->block_size = LF_DEFAULT_BLOCK;
    opt->window_size = LF_DEFAULT_WINDOW;
    opt->max_chain = LF_DEFAULT_MAX_CHAIN;
}

const char *lf_result_name(lf_result r)
{
    switch (r) {
    case LF_OK:
        return "LF_OK";
    case LF_ERR_ALLOC:
        return "LF_ERR_ALLOC";
    case LF_ERR_PARAM:
        return "LF_ERR_PARAM";
    case LF_ERR_IO:
        return "LF_ERR_IO";
    case LF_ERR_BAD_MAGIC:
        return "LF_ERR_BAD_MAGIC";
    case LF_ERR_UNSUPPORTED_VERSION:
        return "LF_ERR_UNSUPPORTED_VERSION";
    case LF_ERR_TRUNCATED:
        return "LF_ERR_TRUNCATED";
    case LF_ERR_INVALID_DATA:
        return "LF_ERR_INVALID_DATA";
    case LF_ERR_BAD_LENGTH:
        return "LF_ERR_BAD_LENGTH";
    case LF_ERR_BUFFER_FULL:
        return "LF_ERR_BUFFER_FULL";
    }
    return "LF_ERR_UNKNOWN";
}
