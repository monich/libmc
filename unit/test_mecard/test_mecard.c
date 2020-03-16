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

#include "mc_mecard.h"

#include <glib.h>

/* Null */

static
void
test_null(
    void)
{
    const char empty[] = "";

    /* NULL resistance */
    g_assert(!mecard_parse_data(NULL, 0));
    g_assert(!mecard_parse_data(empty, 0));
    g_assert(!mecard_parse(NULL));
    g_assert(!mecard_parse(empty));
    mecard_free(NULL);
}

/* Basic */

static
void
test_basic(
    void)
{
    /* Example from OMA-TS-MC-V1_0-20130611-A */
    MeCard* mecard = mecard_parse("MECARD:"
        "N:Bill Jones;"
        "TEL:+18586230741;"
        "TEL:+18586230742;"
        "EMAIL:foo@openmobilealliance.org;"
        "EMAIL:hoo@openmobilealliance.org;"
        "URL:http\\://www.openmobilealliance.org;;");

    g_assert(mecard);

    g_assert(mecard->n);
    g_assert_cmpstr(mecard->n[0], == ,"Bill Jones");
    g_assert(!mecard->n[1]);

    g_assert(mecard->tel);
    g_assert_cmpstr(mecard->tel[0], == ,"+18586230741");
    g_assert_cmpstr(mecard->tel[1], == ,"+18586230742");
    g_assert(!mecard->tel[2]);

    g_assert(mecard->email);
    g_assert_cmpstr(mecard->email[0], == ,"foo@openmobilealliance.org");
    g_assert_cmpstr(mecard->email[1], == ,"hoo@openmobilealliance.org");
    g_assert(!mecard->email[2]);

    g_assert(mecard->url);
    g_assert_cmpstr(mecard->url[0], == ,"http://www.openmobilealliance.org");
    g_assert(!mecard->url[1]);

    g_assert(!mecard->bday);
    g_assert(!mecard->adr);
    g_assert(!mecard->note);
    g_assert(!mecard->nickname);
    mecard_free(mecard);
}

/* UnknownProperty */

static
void
test_unknown_property(
    void)
{
    MeCard* mecard = mecard_parse("MECARD:"
        "N:Doe,John;"
        "TEL:13035551212;"
        "EMAIL:john.doe@example.com;"
        "FOO:bar;;"); /* This one is ignored */

    g_assert(mecard);

    g_assert(mecard->n);
    g_assert_cmpstr(mecard->n[0], == ,"Doe");
    g_assert_cmpstr(mecard->n[1], == ,"John");
    g_assert(!mecard->n[2]);

    g_assert(mecard->tel);
    g_assert_cmpstr(mecard->tel[0], == ,"13035551212");
    g_assert(!mecard->tel[1]);

    g_assert(mecard->email);
    g_assert_cmpstr(mecard->email[0], == ,"john.doe@example.com");
    g_assert(!mecard->email[1]);

    mecard_free(mecard);
}

/* EmptyValue */

static
void
test_empty_value(
    void)
{
    MeCard* mecard = mecard_parse("MECARD:"
        "N:Doe,John;"
        "TEL:13035551212;"
        "EMAIL:;;");

    g_assert(mecard);

    g_assert(mecard->n);
    g_assert_cmpstr(mecard->n[0], == ,"Doe");
    g_assert_cmpstr(mecard->n[1], == ,"John");
    g_assert(!mecard->n[2]);

    g_assert(mecard->tel);
    g_assert_cmpstr(mecard->tel[0], == ,"13035551212");
    g_assert(!mecard->tel[1]);

    g_assert(!mecard->email);
    mecard_free(mecard);
}

/* MultiValues */

static
void
test_multi_values(
    void)
{
    MeCard* mecard = mecard_parse("MECARD:"
        "EMAIL:;" /* The order doesn't matter */
        "N:John Doe;"
        "TEL:13035551212;"
        "TEL:;"
        "TEL:13035551213;;");

    g_assert(mecard);

    g_assert(mecard->n);
    g_assert_cmpstr(mecard->n[0], == ,"John Doe");
    g_assert(!mecard->n[1]);

    g_assert(mecard->tel);
    g_assert_cmpstr(mecard->tel[0], == ,"13035551212");
    g_assert_cmpstr(mecard->tel[1], == ,"13035551213");
    g_assert(!mecard->tel[2]);

    g_assert(!mecard->email);
    mecard_free(mecard);
}

/* Failure */

static
void
test_failure(
    gconstpointer test)
{
    g_assert(!mecard_parse(test));
}

/* Common */

#define TEST_(x) "/mecard/" x

int main(int argc, char* argv[])
{
    g_test_init(&argc, &argv, NULL);
    g_test_add_func(TEST_("null"), test_null);
    g_test_add_data_func(TEST_("empty"),"       ", test_failure);
    g_test_add_data_func(TEST_("garbage"), "MECARDDDDDD", test_failure);
    g_test_add_data_func(TEST_("invalid_id/1"), "foo: ;;", test_failure);
    g_test_add_data_func(TEST_("invalid_id/2"), "MECARDD:", test_failure);
    g_test_add_func(TEST_("basic"), test_basic);
    g_test_add_func(TEST_("unknown_property"), test_unknown_property);
    g_test_add_func(TEST_("empty_value"), test_empty_value);
    g_test_add_func(TEST_("multi_values"), test_multi_values);
    return g_test_run();
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
