/*
 * Copyright (C) 2020 by Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "mc_record.h"

#include <glib.h>

/* Null */

static
void
test_null(
    void)
{
    const char empty[] = "";

    /* NULL resistance */
    g_assert(!mc_record_parse_data(NULL, 0));
    g_assert(!mc_record_parse_data(empty, 0));
    g_assert(!mc_record_parse(NULL));
    g_assert(!mc_record_parse(empty));
    mc_record_free(NULL);
}

/* Failure */

static
void
test_failure(
    gconstpointer str)
{
    g_assert(!mc_record_parse(str));
}

/* Foo */

static
void
test_foo(
    gconstpointer str)
{
    McRecord* rec = mc_record_parse(str);

    g_assert(rec);
    g_assert(!rec->n_prop);
    g_assert_cmpstr(rec->ident, ==, "foo");
    mc_record_free(rec);
}

/* Basic */

static
void
test_basic(
    void)
{
    const char str[] = "id:name-0:value_0;name-1:value_1;;";
    const size_t len = sizeof(str) - 1;
    guint i;

    /* Two last characters can be skipped */
    for (i = 0; i < 3; i++) {
        McRecord* rec = mc_record_parse_data(str, len - i);

        g_assert(rec);
        g_assert_cmpuint(rec->n_prop, ==, 2);
        g_assert_cmpstr(rec->ident, ==, "id");
        g_assert_cmpstr(rec->prop[0].name, ==, "name-0");
        g_assert_cmpstr(rec->prop[0].values[0], ==, "value_0");
        g_assert(!rec->prop[0].values[1]);
        g_assert_cmpstr(rec->prop[1].name, ==, "name-1");
        g_assert_cmpstr(rec->prop[1].values[0], ==, "value_1");
        g_assert(!rec->prop[1].values[1]);
        mc_record_free(rec);
    }
}

/* EmptyValue */

static
void
test_empty_value(
    void)
{
    McRecord* rec = mc_record_parse(" foo-bar: name0:value; name1:;;");

    g_assert(rec);
    g_assert_cmpuint(rec->n_prop, ==, 2);
    g_assert_cmpstr(rec->ident, ==, "foo-bar");
    g_assert_cmpstr(rec->prop[0].name, ==, "name0");
    g_assert_cmpstr(rec->prop[0].values[0], ==, "value");
    g_assert(!rec->prop[0].values[1]);
    g_assert_cmpstr(rec->prop[1].name, ==, "name1");
    g_assert(!rec->prop[1].values);
    mc_record_free(rec);
}

/* MultipleValues */

static
void
test_multiple_values(
    void)
{
    McRecord* rec = mc_record_parse(" id: name:value0,,value1,");

    g_assert(rec);
    g_assert_cmpuint(rec->n_prop, ==, 1);
    g_assert_cmpstr(rec->ident, ==, "id");
    g_assert_cmpstr(rec->prop[0].name, ==, "name");
    g_assert_cmpstr(rec->prop[0].values[0], ==, "value0");
    g_assert_cmpstr(rec->prop[0].values[1], ==, "value1");
    g_assert(!rec->prop[0].values[2]);
    mc_record_free(rec);
}

/* ValidUtf8 */

static
void
test_valid_utf8(
    void)
{
    McRecord* rec = mc_record_parse
        ("id: name:\xD1\x82\xD0\xB5\xD1\x81\xD1\x82;;");

    g_assert(rec);
    g_assert_cmpuint(rec->n_prop, ==, 1);
    g_assert_cmpstr(rec->ident, ==, "id");
    g_assert_cmpstr(rec->prop[0].name, ==, "name");
    g_assert_cmpstr(rec->prop[0].values[0], ==,
        "\xD1\x82\xD0\xB5\xD1\x81\xD1\x82");
    g_assert(!rec->prop[0].values[1]);
    mc_record_free(rec);
}

/* InvalidUtf8 */

static
void
test_invalid_utf8(
    gconstpointer test_data)
{
    /* Invalid UTF-8 goes as ISO8Bit */
    const char* invalid_seq = test_data;
    char* str = g_strconcat("id: name:", invalid_seq, NULL);
    McRecord* rec = mc_record_parse(str);

    g_assert(rec);
    g_assert_cmpuint(rec->n_prop, ==, 1);
    g_assert_cmpstr(rec->ident, ==, "id");
    g_assert_cmpstr(rec->prop[0].name, ==, "name");
    g_assert_cmpstr(rec->prop[0].values[0], ==, invalid_seq);
    g_assert(!rec->prop[0].values[1]);
    mc_record_free(rec);
    g_free(str);
}

/* Transform */

typedef struct test_transform_data {
    const char* in;
    const char* out;
} TestTransformData;

static const TestTransformData test_shift_jis_data[] = {
    { "\x83\x6e", "\xe3\x83\x8f" },
    { "\x83\x8d", "\xe3\x83\xad" }
};

static const TestTransformData test_escape_data[] = {
    { "colon\\:", "colon:" },
    { "comma\\,", "comma," },
    { "\\;semicolon", ";semicolon" },
    { "back\\\\slash", "back\\slash" },
    { "optional\\.", "optional." }
};

static
void
test_transform(
    gconstpointer test_data)
{
    const TestTransformData* test = test_data;
    char* str = g_strconcat("id: test:", test->in, ";;", NULL);
    McRecord* rec = mc_record_parse(str);

    g_assert(rec);
    g_assert_cmpuint(rec->n_prop, ==, 1);
    g_assert_cmpstr(rec->ident, ==, "id");
    g_assert_cmpstr(rec->prop[0].name, ==, "test");
    g_assert_cmpstr(rec->prop[0].values[0], ==, test->out);
    g_assert(!rec->prop[0].values[1]);
    mc_record_free(rec);
    g_free(str);
}

/* Common */

#define TEST_(x) "/record/" x

int main(int argc, char* argv[])
{
    guint i;

    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_data_func(TEST_("blank"), " ", test_failure);
    g_test_add_data_func(TEST_("missing_id"), "x", test_failure);
    g_test_add_data_func(TEST_("empty_id"), ":", test_failure);
    g_test_add_data_func(TEST_("invalid_id/1"), "_:", test_failure);
    g_test_add_data_func(TEST_("invalid_id/2"), " x_ :", test_failure);
    g_test_add_data_func(TEST_("invalid_prop/1"), "foo::", test_failure);
    g_test_add_data_func(TEST_("invalid_prop/2"), "foo: a", test_failure);
    g_test_add_data_func(TEST_("invalid_prop/3"), "foo: a_", test_failure);
    g_test_add_data_func(TEST_("invalid_prop/4"), "foo: a:\\", test_failure);
    g_test_add_data_func(TEST_("no_props/1"), " foo :", test_foo);
    g_test_add_data_func(TEST_("no_props/2"), " foo : ;", test_foo);
    g_test_add_data_func(TEST_("no_props/3"), " foo : ;;", test_foo);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("empty_value"), test_empty_value);
    g_test_add_func(TEST_("multiple_values"), test_multiple_values);
    g_test_add_func(TEST_("valid_utf8"), test_valid_utf8);
    g_test_add_data_func(TEST_("invalid_utf8/1"),"\xD1", test_invalid_utf8);
    g_test_add_data_func(TEST_("invalid_utf8/2"),"\xD1\xD1",test_invalid_utf8);
    g_test_add_data_func(TEST_("invalid_utf8/3"),"\xFD\x81",test_invalid_utf8);

    for (i = 0; i < G_N_ELEMENTS(test_shift_jis_data); i++) {
        char* name = g_strdup_printf(TEST_("shift_jis/%u"), i + 1);

        g_test_add_data_func(name, test_shift_jis_data + i, test_transform);
        g_free(name);
    }
    for (i = 0; i < G_N_ELEMENTS(test_escape_data); i++) {
        char* name = g_strdup_printf(TEST_("escape/%u"), i + 1);

        g_test_add_data_func(name, test_escape_data + i, test_transform);
        g_free(name);
    }
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
