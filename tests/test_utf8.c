#include "greatest.h"
#include "ztr.h"

/*
 * UTF-8 byte sequences used throughout:
 *
 *   "hello"   — 5 codepoints, 5 bytes (pure ASCII)
 *   "héllo"   — 5 codepoints, 6 bytes  (é = U+00E9 = 0xC3 0xA9)
 *   "€"       — 1 codepoint,  3 bytes  (U+20AC = 0xE2 0x82 0xAC)
 *   "🎉"      — 1 codepoint,  4 bytes  (U+1F389 = 0xF0 0x9F 0x8E 0x89)
 *   overlong  — 0xC1 0x81 encodes U+0041 ('A') in 2 bytes (MUST be rejected)
 *   surrogate — 0xED 0xA0 0x80 encodes U+D800               (MUST be rejected)
 *   >U+10FFFF — 0xF4 0x90 0x80 0x80 encodes U+110000        (MUST be rejected)
 */

/* =========================================================================
 * Helpers: build ztr from a raw byte buffer
 * ========================================================================= */

static ztr make_buf(const unsigned char *buf, size_t len) {
    ztr s;
    ztr_from_buf(&s, (const char *)buf, len);
    return s;
}

static ztr make_str(const char *cstr) {
    ztr s;
    ztr_from(&s, cstr);
    return s;
}

/* =========================================================================
 * ztr_is_valid_utf8
 * ========================================================================= */

TEST valid_utf8_ascii_string(void) {
    ztr s = make_str("hello");
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST valid_utf8_two_byte_sequence(void) {
    /* é = U+00E9 = 0xC3 0xA9 */
    const unsigned char bytes[] = {0xC3, 0xA9};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST valid_utf8_three_byte_sequence(void) {
    /* € = U+20AC = 0xE2 0x82 0xAC */
    const unsigned char bytes[] = {0xE2, 0x82, 0xAC};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST valid_utf8_four_byte_sequence(void) {
    /* 🎉 = U+1F389 = 0xF0 0x9F 0x8E 0x89 */
    const unsigned char bytes[] = {0xF0, 0x9F, 0x8E, 0x89};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST valid_utf8_mixed_multibyte(void) {
    /* "héllo" = 'h' é 'l' 'l' 'o' */
    const unsigned char bytes[] = {'h', 0xC3, 0xA9, 'l', 'l', 'o'};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST valid_utf8_empty_string(void) {
    ztr s = make_str("");
    ASSERT(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_overlong_encoding(void) {
    /* Overlong 2-byte encoding of 'A' (U+0041): 0xC1 0x81
     * Valid UTF-8 must use the shortest encoding, so this must be rejected. */
    const unsigned char bytes[] = {0xC1, 0x81};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_surrogate_half(void) {
    /* U+D800 surrogate: 0xED 0xA0 0x80 — surrogates are not valid in UTF-8 */
    const unsigned char bytes[] = {0xED, 0xA0, 0x80};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_above_max_codepoint(void) {
    /* U+110000 (above U+10FFFF): 0xF4 0x90 0x80 0x80 */
    const unsigned char bytes[] = {0xF4, 0x90, 0x80, 0x80};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_truncated_two_byte(void) {
    /* First byte of a 2-byte sequence with no continuation byte */
    const unsigned char bytes[] = {0xC3};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_truncated_three_byte(void) {
    /* First two bytes of a 3-byte sequence (€ without last byte) */
    const unsigned char bytes[] = {0xE2, 0x82};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_truncated_four_byte(void) {
    /* First three bytes of a 4-byte sequence */
    const unsigned char bytes[] = {0xF0, 0x9F, 0x8E};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_continuation_byte_without_start(void) {
    /* Stray continuation byte 0x80 with no leading byte */
    const unsigned char bytes[] = {0x80};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

TEST invalid_utf8_invalid_start_byte_fe(void) {
    /* 0xFE is not a valid UTF-8 start byte */
    const unsigned char bytes[] = {0xFE, 0x80, 0x80, 0x80, 0x80, 0x80};
    ztr s = make_buf(bytes, sizeof(bytes));
    ASSERT_FALSE(ztr_is_valid_utf8(&s));
    ztr_free(&s);
    PASS();
}

/* =========================================================================
 * ztr_utf8_len
 * ========================================================================= */

TEST utf8_len_ascii(void) {
    ztr s = make_str("hello");
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(5, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_two_byte(void) {
    /* é alone */
    const unsigned char bytes[] = {0xC3, 0xA9};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(1, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_three_byte(void) {
    /* € alone */
    const unsigned char bytes[] = {0xE2, 0x82, 0xAC};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(1, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_four_byte(void) {
    /* 🎉 alone */
    const unsigned char bytes[] = {0xF0, 0x9F, 0x8E, 0x89};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(1, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_mixed_multibyte(void) {
    /* "héllo" — 5 codepoints, 6 bytes */
    const unsigned char bytes[] = {'h', 0xC3, 0xA9, 'l', 'l', 'o'};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(5, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_empty_string(void) {
    ztr s = make_str("");
    size_t len = 42; /* pre-set to verify it is overwritten */
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(0, len);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_invalid_returns_error(void) {
    /* Truncated sequence — must return an error code, not ZTR_OK */
    const unsigned char bytes[] = {0xC3}; /* incomplete 2-byte sequence */
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ztr_err err = ztr_utf8_len(&len, &s);
    ASSERT_FALSE(err == ZTR_OK);
    ztr_free(&s);
    PASS();
}

TEST utf8_len_string_with_all_widths(void) {
    /* ASCII 'A' + é (2-byte) + € (3-byte) + 🎉 (4-byte) = 4 codepoints */
    const unsigned char bytes[] = {'A', 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x8E, 0x89};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t len = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&len, &s));
    ASSERT_EQ(4, len);
    ztr_free(&s);
    PASS();
}

/* =========================================================================
 * ztr_utf8_next
 * ========================================================================= */

TEST utf8_next_iterate_ascii(void) {
    ztr s = make_str("ABC");
    size_t pos = 0;
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x41, cp); /* 'A' */
    ASSERT_EQ(1, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x42, cp); /* 'B' */
    ASSERT_EQ(2, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x43, cp); /* 'C' */
    ASSERT_EQ(3, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_iterate_two_byte(void) {
    /* é = U+00E9 */
    const unsigned char bytes[] = {0xC3, 0xA9};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x00E9, cp);
    ASSERT_EQ(2, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_iterate_three_byte(void) {
    /* € = U+20AC */
    const unsigned char bytes[] = {0xE2, 0x82, 0xAC};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x20AC, cp);
    ASSERT_EQ(3, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_iterate_four_byte(void) {
    /* 🎉 = U+1F389 */
    const unsigned char bytes[] = {0xF0, 0x9F, 0x8E, 0x89};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x1F389, cp);
    ASSERT_EQ(4, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_iterate_mixed_multibyte(void) {
    /* "héllo" */
    const unsigned char bytes[] = {'h', 0xC3, 0xA9, 'l', 'l', 'o'};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ('h', cp);
    ASSERT_EQ(1, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x00E9, cp);
    ASSERT_EQ(3, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ('l', cp);
    ASSERT_EQ(4, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ('l', cp);
    ASSERT_EQ(5, pos);

    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ('o', cp);
    ASSERT_EQ(6, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_invalid_sets_replacement_and_advances_by_one(void) {
    /* Stray continuation byte 0x80 followed by valid 'A'.
     * Error: cp must be set to U+FFFD (replacement character) and pos
     * must advance by exactly 1 byte so the caller can recover. */
    const unsigned char bytes[] = {0x80, 'A'};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ztr_err err = ztr_utf8_next(&s, &pos, &cp);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0xFFFD, cp);
    ASSERT_EQ(1, pos); /* advanced by exactly 1 */

    /* Next call should decode 'A' normally */
    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ('A', cp);
    ASSERT_EQ(2, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_truncated_sequence_sets_replacement_and_advances(void) {
    /* Incomplete 2-byte sequence at position 0: 0xC3 with no continuation.
     * Should set cp=U+FFFD and advance pos by 1. */
    const unsigned char bytes[] = {0xC3};
    ztr s = make_buf(bytes, sizeof(bytes));
    size_t pos = 0;
    uint32_t cp = 0;

    ztr_err err = ztr_utf8_next(&s, &pos, &cp);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0xFFFD, cp);
    ASSERT_EQ(1, pos);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_out_of_range_when_pos_equals_len(void) {
    ztr s = make_str("A");
    size_t pos = 0;
    uint32_t cp = 0;

    /* Consume the only character */
    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(1, pos);

    /* Now pos == len; must return ZTR_ERR_OUT_OF_RANGE */
    ztr_err err = ztr_utf8_next(&s, &pos, &cp);
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, err);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_out_of_range_on_empty_string(void) {
    ztr s = make_str("");
    size_t pos = 0;
    uint32_t cp = 0;

    ztr_err err = ztr_utf8_next(&s, &pos, &cp);
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, err);

    ztr_free(&s);
    PASS();
}

TEST utf8_next_pos_beyond_len(void) {
    ztr s = make_str("hi");
    size_t pos = 99; /* way past the end */
    uint32_t cp = 0;

    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, ztr_utf8_next(&s, &pos, &cp));

    ztr_free(&s);
    PASS();
}

/* =========================================================================
 * ztr_utf8_append
 * ========================================================================= */

TEST utf8_append_ascii_codepoint(void) {
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 'A'));
    ASSERT_EQ(1, ztr_len(&s));
    ASSERT_STR_EQ("A", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_two_byte_codepoint(void) {
    /* é = U+00E9 — encodes as 0xC3 0xA9 */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x00E9));
    ASSERT_EQ(2, ztr_len(&s));

    const unsigned char *bytes = (const unsigned char *)ztr_cstr(&s);
    ASSERT_EQ(0xC3, bytes[0]);
    ASSERT_EQ(0xA9, bytes[1]);

    ztr_free(&s);
    PASS();
}

TEST utf8_append_three_byte_codepoint(void) {
    /* € = U+20AC — encodes as 0xE2 0x82 0xAC */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x20AC));
    ASSERT_EQ(3, ztr_len(&s));

    const unsigned char *bytes = (const unsigned char *)ztr_cstr(&s);
    ASSERT_EQ(0xE2, bytes[0]);
    ASSERT_EQ(0x82, bytes[1]);
    ASSERT_EQ(0xAC, bytes[2]);

    ztr_free(&s);
    PASS();
}

TEST utf8_append_four_byte_supplementary_plane(void) {
    /* 🎉 = U+1F389 — encodes as 0xF0 0x9F 0x8E 0x89 */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x1F389));
    ASSERT_EQ(4, ztr_len(&s));

    const unsigned char *bytes = (const unsigned char *)ztr_cstr(&s);
    ASSERT_EQ(0xF0, bytes[0]);
    ASSERT_EQ(0x9F, bytes[1]);
    ASSERT_EQ(0x8E, bytes[2]);
    ASSERT_EQ(0x89, bytes[3]);

    ztr_free(&s);
    PASS();
}

TEST utf8_append_null_character(void) {
    /* U+0000 is a valid codepoint; encoded as a single 0x00 byte in UTF-8.
     * ztr stores the length separately, so this should work. */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x0000));
    ASSERT_EQ(1, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_last_valid_codepoint(void) {
    /* U+10FFFF — the highest valid Unicode codepoint */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x10FFFF));
    ASSERT_EQ(4, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_surrogate_rejected(void) {
    /* U+D800 is a surrogate half and must not be encoded */
    ztr s;
    ztr_init(&s);

    ztr_err err = ztr_utf8_append(&s, 0xD800);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0, ztr_len(&s)); /* string must remain unchanged */

    ztr_free(&s);
    PASS();
}

TEST utf8_append_high_surrogate_rejected(void) {
    /* U+DFFF — high end of the surrogate range */
    ztr s;
    ztr_init(&s);

    ztr_err err = ztr_utf8_append(&s, 0xDFFF);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_above_max_codepoint_rejected(void) {
    /* U+110000 is one past the maximum valid codepoint */
    ztr s;
    ztr_init(&s);

    ztr_err err = ztr_utf8_append(&s, 0x110000);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_way_above_max_codepoint_rejected(void) {
    ztr s;
    ztr_init(&s);

    ztr_err err = ztr_utf8_append(&s, 0xFFFFFFFF);
    ASSERT_FALSE(err == ZTR_OK);
    ASSERT_EQ(0, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST utf8_append_multiple_codepoints_form_valid_string(void) {
    /* Build "héllo" by appending codepoints one at a time, then validate. */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 'h'));
    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x00E9)); /* é */
    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 'l'));
    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 'l'));
    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 'o'));

    ASSERT_EQ(6, ztr_len(&s)); /* 1+2+1+1+1 bytes */
    ASSERT(ztr_is_valid_utf8(&s));

    size_t cp_count = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_len(&cp_count, &s));
    ASSERT_EQ(5, cp_count);

    ztr_free(&s);
    PASS();
}

TEST utf8_append_roundtrip_via_next(void) {
    /* Append U+20AC (€), then read it back with ztr_utf8_next. */
    ztr s;
    ztr_init(&s);

    ASSERT_EQ(ZTR_OK, ztr_utf8_append(&s, 0x20AC));

    size_t pos = 0;
    uint32_t cp = 0;
    ASSERT_EQ(ZTR_OK, ztr_utf8_next(&s, &pos, &cp));
    ASSERT_EQ(0x20AC, cp);
    ASSERT_EQ(3, pos);

    /* No more codepoints */
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, ztr_utf8_next(&s, &pos, &cp));

    ztr_free(&s);
    PASS();
}

/* =========================================================================
 * Suite
 * ========================================================================= */

SUITE(utf8) {
    /* ztr_is_valid_utf8 */
    RUN_TEST(valid_utf8_ascii_string);
    RUN_TEST(valid_utf8_two_byte_sequence);
    RUN_TEST(valid_utf8_three_byte_sequence);
    RUN_TEST(valid_utf8_four_byte_sequence);
    RUN_TEST(valid_utf8_mixed_multibyte);
    RUN_TEST(valid_utf8_empty_string);
    RUN_TEST(invalid_utf8_overlong_encoding);
    RUN_TEST(invalid_utf8_surrogate_half);
    RUN_TEST(invalid_utf8_above_max_codepoint);
    RUN_TEST(invalid_utf8_truncated_two_byte);
    RUN_TEST(invalid_utf8_truncated_three_byte);
    RUN_TEST(invalid_utf8_truncated_four_byte);
    RUN_TEST(invalid_utf8_continuation_byte_without_start);
    RUN_TEST(invalid_utf8_invalid_start_byte_fe);

    /* ztr_utf8_len */
    RUN_TEST(utf8_len_ascii);
    RUN_TEST(utf8_len_two_byte);
    RUN_TEST(utf8_len_three_byte);
    RUN_TEST(utf8_len_four_byte);
    RUN_TEST(utf8_len_mixed_multibyte);
    RUN_TEST(utf8_len_empty_string);
    RUN_TEST(utf8_len_invalid_returns_error);
    RUN_TEST(utf8_len_string_with_all_widths);

    /* ztr_utf8_next */
    RUN_TEST(utf8_next_iterate_ascii);
    RUN_TEST(utf8_next_iterate_two_byte);
    RUN_TEST(utf8_next_iterate_three_byte);
    RUN_TEST(utf8_next_iterate_four_byte);
    RUN_TEST(utf8_next_iterate_mixed_multibyte);
    RUN_TEST(utf8_next_invalid_sets_replacement_and_advances_by_one);
    RUN_TEST(utf8_next_truncated_sequence_sets_replacement_and_advances);
    RUN_TEST(utf8_next_out_of_range_when_pos_equals_len);
    RUN_TEST(utf8_next_out_of_range_on_empty_string);
    RUN_TEST(utf8_next_pos_beyond_len);

    /* ztr_utf8_append */
    RUN_TEST(utf8_append_ascii_codepoint);
    RUN_TEST(utf8_append_two_byte_codepoint);
    RUN_TEST(utf8_append_three_byte_codepoint);
    RUN_TEST(utf8_append_four_byte_supplementary_plane);
    RUN_TEST(utf8_append_null_character);
    RUN_TEST(utf8_append_last_valid_codepoint);
    RUN_TEST(utf8_append_surrogate_rejected);
    RUN_TEST(utf8_append_high_surrogate_rejected);
    RUN_TEST(utf8_append_above_max_codepoint_rejected);
    RUN_TEST(utf8_append_way_above_max_codepoint_rejected);
    RUN_TEST(utf8_append_multiple_codepoints_form_valid_string);
    RUN_TEST(utf8_append_roundtrip_via_next);
}
