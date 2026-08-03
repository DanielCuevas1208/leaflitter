#include "lf_common.h"
#include "lf_stream.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LF_TOOL_VERSION "1.1.0"

static void lf_tool_usage(FILE *f)
{
    fprintf(f,
            "leaflitter %s\n"
            "Usage: leaflitter <command> <in> <out>\n"
            "Commands:\n"
            "  c   Compress <in> to <out>.\n"
            "  d   Decompress <in> to <out>.\n"
            "  i   Show the header of <in>.\n",
            LF_TOOL_VERSION);
}

static void *lf_tool_read_file(const char *path, size_t *out_len)
{
    FILE *fp;
    long size;
    uint8_t *data;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "leaflitter: cannot open %s\n", path);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0 || (size = ftell(fp)) < 0 ||
        fseek(fp, 0, SEEK_SET) != 0) {
        fprintf(stderr, "leaflitter: cannot size %s\n", path);
        fclose(fp);
        return NULL;
    }
    data = (uint8_t *)malloc((size_t)size + 1);
    if (data == NULL) {
        fprintf(stderr, "leaflitter: out of memory\n");
        fclose(fp);
        return NULL;
    }
    if (size > 0 && fread(data, 1, (size_t)size, fp) != (size_t)size) {
        fprintf(stderr, "leaflitter: cannot read %s\n", path);
        free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *out_len = (size_t)size;
    return data;
}

static int lf_tool_write_file(const char *path, const void *data, size_t len)
{
    FILE *fp;
    int ok;

    fp = fopen(path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "leaflitter: cannot open %s\n", path);
        return 0;
    }
    ok = (len == 0 || fwrite(data, 1, len, fp) == len);
    if (fclose(fp) != 0) {
        ok = 0;
    }
    if (!ok) {
        fprintf(stderr, "leaflitter: cannot write %s\n", path);
    }
    return ok;
}

static int lf_tool_compress(const char *in, const char *out)
{
    void *data;
    size_t len;
    lf_buf c;
    lf_result r;
    int ok = 0;

    data = lf_tool_read_file(in, &len);
    if (data == NULL) {
        return 1;
    }
    lf_buf_init(&c);
    r = lf_compress(NULL, data, len, &c);
    if (r != LF_OK) {
        fprintf(stderr, "leaflitter: compress failed: %s\n", lf_result_name(r));
    } else if (!lf_tool_write_file(out, c.data, c.len)) {
        fprintf(stderr, "leaflitter: cannot write %s\n", out);
    } else {
        printf("%s: %zu -> %zu bytes\n", in, len, c.len);
        ok = 1;
    }
    lf_buf_free(&c);
    free(data);
    return ok ? 0 : 1;
}

static int lf_tool_decompress(const char *in, const char *out)
{
    void *data;
    size_t len;
    lf_buf p;
    lf_result r;
    int ok = 0;

    data = lf_tool_read_file(in, &len);
    if (data == NULL) {
        return 1;
    }
    lf_buf_init(&p);
    r = lf_decompress(data, len, &p);
    if (r != LF_OK) {
        fprintf(stderr, "leaflitter: decompress failed: %s\n", lf_result_name(r));
    } else if (!lf_tool_write_file(out, p.data, p.len)) {
        fprintf(stderr, "leaflitter: cannot write %s\n", out);
    } else {
        printf("%s: %zu -> %zu bytes\n", in, len, p.len);
        ok = 1;
    }
    lf_buf_free(&p);
    free(data);
    return ok ? 0 : 1;
}

static int lf_tool_info(const char *in)
{
    void *data;
    size_t len;
    const uint8_t *h;
    uint64_t size;
    uint32_t adler;
    uint32_t window;
    int ok = 0;

    data = lf_tool_read_file(in, &len);
    if (data == NULL) {
        return 1;
    }
    if (len < 24) {
        fprintf(stderr, "leaflitter: %s is too short to be a stream\n", in);
        free(data);
        return 1;
    }
    h = (const uint8_t *)data;
    if (h[0] != LF_MAGIC_0 || h[1] != LF_MAGIC_1 || h[2] != LF_MAGIC_2) {
        fprintf(stderr, "leaflitter: %s has no leaflitter magic\n", in);
        free(data);
        return 1;
    }
    size = (uint64_t)h[4] | ((uint64_t)h[5] << 8) | ((uint64_t)h[6] << 16) |
           ((uint64_t)h[7] << 24) | ((uint64_t)h[8] << 32) |
           ((uint64_t)h[9] << 40) | ((uint64_t)h[10] << 48) |
           ((uint64_t)h[11] << 56);
    adler = (uint32_t)h[12] | ((uint32_t)h[13] << 8) |
            ((uint32_t)h[14] << 16) | ((uint32_t)h[15] << 24);
    window = (uint32_t)h[16] | ((uint32_t)h[17] << 8) |
             ((uint32_t)h[18] << 16) | ((uint32_t)h[19] << 24);
    printf("format version: %u\n", h[3]);
    printf("uncompressed size: %llu bytes\n", (unsigned long long)size);
    printf("adler32: %08x\n", adler);
    printf("window size: %u bytes\n", window);
    ok = 1;
    free(data);
    return ok ? 0 : 1;
}

int main(int argc, char **argv)
{
    if (argc < 3 || argc > 4) {
        lf_tool_usage(stderr);
        return 2;
    }
    if (strcmp(argv[1], "c") == 0) {
        if (argc != 4) {
            lf_tool_usage(stderr);
            return 2;
        }
        return lf_tool_compress(argv[2], argv[3]);
    }
    if (strcmp(argv[1], "d") == 0) {
        if (argc != 4) {
            lf_tool_usage(stderr);
            return 2;
        }
        return lf_tool_decompress(argv[2], argv[3]);
    }
    if (strcmp(argv[1], "i") == 0) {
        if (argc != 3) {
            lf_tool_usage(stderr);
            return 2;
        }
        return lf_tool_info(argv[2]);
    }
    fprintf(stderr, "leaflitter: unknown command %s\n", argv[1]);
    lf_tool_usage(stderr);
    return 2;
}
