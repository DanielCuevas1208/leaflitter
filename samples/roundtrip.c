#include "lf_buf.h"
#include "lf_common.h"
#include "lf_stream.h"

#include <stdio.h>
#include <string.h>

static int check_roundtrip(const char *text)
{
    lf_buf packed;
    lf_buf plain;
    lf_result r;
    size_t n = strlen(text);
    int ok = 0;

    lf_buf_init(&packed);
    lf_buf_init(&plain);

    r = lf_compress(NULL, text, n, &packed);
    if (r != LF_OK) {
        fprintf(stderr, "compress: %s\n", lf_result_name(r));
        goto done;
    }
    r = lf_decompress(packed.data, packed.len, &plain);
    if (r != LF_OK) {
        fprintf(stderr, "decompress: %s\n", lf_result_name(r));
        goto done;
    }

    ok = (plain.len == n) && (n == 0 || memcmp(plain.data, text, n) == 0);
    if (ok) {
        printf("%4zu -> %4zu bytes\n", n, packed.len);
    } else {
        fprintf(stderr, "roundtrip mismatch\n");
    }

done:
    lf_buf_free(&packed);
    lf_buf_free(&plain);
    return ok ? 0 : 1;
}

int main(void)
{
    const char *text =
        "leaflitter compresses bytes with LZ77 matches and Huffman codes. "
        "leaflitter compresses bytes with LZ77 matches and Huffman codes. "
        "leaflitter compresses bytes with LZ77 matches and Huffman codes.";

    printf("leaflitter %s\n", lf_version_string());
    if (check_roundtrip(text) != 0) {
        return 1;
    }
    if (check_roundtrip("x") != 0) {
        return 1;
    }
    if (check_roundtrip("") != 0) {
        return 1;
    }
    return 0;
}
