#include "unity.h"

#include "lf_buf.h"

#include <string.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_init_is_empty(void)
{
    lf_buf b;

    lf_buf_init(&b);
    TEST_ASSERT_NULL(b.data);
    TEST_ASSERT_EQUAL_UINT(0, b.len);
    TEST_ASSERT_EQUAL_UINT(0, b.cap);
    lf_buf_free(&b);
}

static void test_append_grows(void)
{
    lf_buf b;
    uint8_t data[1000];
    size_t i;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = (uint8_t)(i & 0xFFu);
    }

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_append(&b, data, sizeof(data)));
    TEST_ASSERT_EQUAL_UINT(sizeof(data), b.len);
    TEST_ASSERT_EQUAL_UINT(0, memcmp(b.data, data, sizeof(data)));
    TEST_ASSERT_TRUE(b.cap >= b.len);
    lf_buf_free(&b);
}

static void test_append_zero_is_noop(void)
{
    lf_buf b;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_append(&b, NULL, 0));
    TEST_ASSERT_EQUAL_UINT(0, b.len);
    lf_buf_free(&b);
}

static void test_clear_keeps_capacity(void)
{
    lf_buf b;
    size_t cap;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_append(&b, "hello", 5));
    cap = b.cap;
    lf_buf_clear(&b);
    TEST_ASSERT_EQUAL_UINT(0, b.len);
    TEST_ASSERT_EQUAL_UINT(cap, b.cap);
    lf_buf_free(&b);
}

static void test_reserve_never_shrinks(void)
{
    lf_buf b;
    size_t cap;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_reserve(&b, 100));
    cap = b.cap;
    TEST_ASSERT_TRUE(cap >= 100);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_reserve(&b, 10));
    TEST_ASSERT_EQUAL_UINT(cap, b.cap);
    lf_buf_free(&b);
}

static void test_u16le_layout(void)
{
    lf_buf b;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_put_u16le(&b, 0x1234));
    TEST_ASSERT_EQUAL_UINT(2, b.len);
    TEST_ASSERT_EQUAL_UINT(0x34, b.data[0]);
    TEST_ASSERT_EQUAL_UINT(0x12, b.data[1]);
    lf_buf_free(&b);
}

static void test_u32le_layout(void)
{
    lf_buf b;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_put_u32le(&b, 0x12345678u));
    TEST_ASSERT_EQUAL_UINT(4, b.len);
    TEST_ASSERT_EQUAL_UINT(0x78, b.data[0]);
    TEST_ASSERT_EQUAL_UINT(0x56, b.data[1]);
    TEST_ASSERT_EQUAL_UINT(0x34, b.data[2]);
    TEST_ASSERT_EQUAL_UINT(0x12, b.data[3]);
    lf_buf_free(&b);
}

static void test_u64le_layout(void)
{
    lf_buf b;

    lf_buf_init(&b);
    TEST_ASSERT_EQUAL_INT(LF_OK, lf_buf_put_u64le(&b, 0x0102030405060708ull));
    TEST_ASSERT_EQUAL_UINT(8, b.len);
    TEST_ASSERT_EQUAL_UINT(0x08, b.data[0]);
    TEST_ASSERT_EQUAL_UINT(0x07, b.data[1]);
    TEST_ASSERT_EQUAL_UINT(0x06, b.data[2]);
    TEST_ASSERT_EQUAL_UINT(0x05, b.data[3]);
    TEST_ASSERT_EQUAL_UINT(0x01, b.data[7]);
    lf_buf_free(&b);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_is_empty);
    RUN_TEST(test_append_grows);
    RUN_TEST(test_append_zero_is_noop);
    RUN_TEST(test_clear_keeps_capacity);
    RUN_TEST(test_reserve_never_shrinks);
    RUN_TEST(test_u16le_layout);
    RUN_TEST(test_u32le_layout);
    RUN_TEST(test_u64le_layout);
    return UNITY_END();
}
