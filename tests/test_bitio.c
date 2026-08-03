#include "unity.h"

#include "lf_bitio.h"
#include "lf_buf.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_put_get_roundtrip(void)
{
    lf_buf b;
    lf_bwriter w;
    lf_breader r;
    unsigned i;

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);

    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0b101u, 3));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0b01u, 2));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0x1F3u, 9));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_align(&w));

    lf_breader_init(&r, b.data, b.len);
    TEST_ASSERT_EQUAL_UINT(0b101u, lf_breader_get(&r, 3));
    TEST_ASSERT_EQUAL_UINT(0b01u, lf_breader_get(&r, 2));
    TEST_ASSERT_EQUAL_UINT(0x1F3u, lf_breader_get(&r, 9));
    TEST_ASSERT_TRUE(lf_breader_ok(&r));
    lf_breader_align(&r);
    TEST_ASSERT_TRUE(lf_breader_finished(&r));

    for (i = 0; i < b.len; i++) {
        b.data[i] = 0;
    }
    lf_buf_free(&b);
}

static void test_msb_first_order(void)
{
    lf_buf b;
    lf_bwriter w;
    lf_breader r;

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0xAAu, 8));
    lf_bwriter_align(&w);

    TEST_ASSERT_EQUAL_UINT(1, b.len);
    TEST_ASSERT_EQUAL_UINT(0xAAu, b.data[0]);

    lf_breader_init(&r, b.data, b.len);
    TEST_ASSERT_EQUAL_UINT(0xAAu, lf_breader_get(&r, 8));
    lf_buf_free(&b);
}

static void test_bit_boundary_across_bytes(void)
{
    lf_buf b;
    lf_bwriter w;
    lf_breader r;

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0xFFu, 4));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0xFFu, 4));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0x01u, 1));
    lf_bwriter_align(&w);

    TEST_ASSERT_EQUAL_UINT(2, b.len);
    TEST_ASSERT_EQUAL_UINT(0xFFu, b.data[0]);
    TEST_ASSERT_EQUAL_UINT(0x80u, b.data[1]);

    lf_breader_init(&r, b.data, b.len);
    TEST_ASSERT_EQUAL_UINT(0x0Fu, lf_breader_get(&r, 4));
    TEST_ASSERT_EQUAL_UINT(0x0Fu, lf_breader_get(&r, 4));
    TEST_ASSERT_EQUAL_UINT(0x01u, lf_breader_get(&r, 1));
    TEST_ASSERT_TRUE(lf_breader_ok(&r));
    lf_buf_free(&b);
}

static void test_truncated_read_fails(void)
{
    uint8_t data[1] = {0x00};
    lf_breader r;

    lf_breader_init(&r, data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT(0, lf_breader_get(&r, 8));
    TEST_ASSERT_TRUE(lf_breader_ok(&r));
    TEST_ASSERT_EQUAL_UINT(0, lf_breader_get(&r, 1));
    TEST_ASSERT_FALSE(lf_breader_ok(&r));
}

static void test_align_drops_partial_byte(void)
{
    lf_buf b;
    lf_bwriter w;
    lf_breader r;

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, 0b101u, 3));
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_align(&w));
    TEST_ASSERT_EQUAL_UINT(1, b.len);

    lf_breader_init(&r, b.data, b.len);
    TEST_ASSERT_EQUAL_UINT(0b101u, lf_breader_get(&r, 3));
    lf_breader_align(&r);
    TEST_ASSERT_EQUAL_UINT(1, lf_breader_bytes_read(&r));
    lf_buf_free(&b);
}

static void test_read_bytes_whole(void)
{
    uint8_t data[6] = {1, 2, 3, 4, 5, 6};
    uint8_t got[4];
    lf_breader r;

    lf_breader_init(&r, data, sizeof(data));
    TEST_ASSERT_TRUE(lf_breader_read_bytes(&r, got, 4));
    TEST_ASSERT_EQUAL_UINT(4, lf_breader_bytes_read(&r));
    TEST_ASSERT_EQUAL_UINT(0, memcmp(got, data, 4));
    TEST_ASSERT_FALSE(lf_breader_read_bytes(&r, got, 4));
}

static void test_long_codes_roundtrip(void)
{
    lf_buf b;
    lf_bwriter w;
    lf_breader r;
    unsigned n = 700;
    uint8_t vals[700];
    uint8_t nbits[700];
    unsigned i;
    unsigned s = 1u;

    for (i = 0; i < n; i++) {
        s = s * 1103515245u + 12345u;
        nbits[i] = (uint8_t)(1u + (s >> 16) % 16u);
        vals[i] = (uint8_t)(s & ((1u << (nbits[i] < 8 ? nbits[i] : 8)) - 1u));
    }

    lf_buf_init(&b);
    lf_bwriter_init(&w, &b);
    for (i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_INT(LF_OK, lf_bwriter_put(&w, vals[i], nbits[i]));
    }
    lf_bwriter_align(&w);

    lf_breader_init(&r, b.data, b.len);
    for (i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_UINT(vals[i], lf_breader_get(&r, nbits[i]));
    }
    TEST_ASSERT_TRUE(lf_breader_ok(&r));
    lf_breader_align(&r);
    TEST_ASSERT_TRUE(lf_breader_finished(&r));
    lf_buf_free(&b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_put_get_roundtrip);
    RUN_TEST(test_msb_first_order);
    RUN_TEST(test_bit_boundary_across_bytes);
    RUN_TEST(test_truncated_read_fails);
    RUN_TEST(test_align_drops_partial_byte);
    RUN_TEST(test_read_bytes_whole);
    RUN_TEST(test_long_codes_roundtrip);
    return UNITY_END();
}
