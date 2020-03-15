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

#include "mc_types_p.h"
#include "mc_record.h"

/*
 * OMA-TS-MC-V1_0
 *
 * 7.1.2 Direct MC Format (DMF)
 */

#define MAX_CHAR_SIZE (6)

static
gsize
mc_property_data_size(
    gsize name_len,
    char** values)
{
    guint i;
    gsize size = SIZE_ALIGN(sizeof(McProperty)) + SIZE_ALIGN(name_len + 1);
    const guint nval = values ? g_strv_length(values) : 0;

    if (nval) {
        size += SIZE_ALIGN((nval + 1) * sizeof(char*));
        for (i = 0; i < nval; i++) {
            size += SIZE_ALIGN(strlen(values[i]) + 1);
        }
    }
    return size;
}

static
void*
mc_property_data_copy(
    McProperty* prop,
    char* ptr,
    const void* name,
    gsize name_len,
    char** values)
{
    const guint nval = values ? g_strv_length(values) : 0;

    prop->name = ptr;
    memcpy(ptr, name, name_len);
    ptr += SIZE_ALIGN(name_len + 1);
    if (nval) {
        guint i;
        char** dest = (char**)ptr;

        prop->values = (const McStr*)dest;
        ptr += SIZE_ALIGN((nval + 1) * sizeof(char*));
        for (i = 0; i < nval; i++) {
            const char* val = values[i];

            dest[i] = ptr;
            strcpy(ptr, val);
            ptr += SIZE_ALIGN(strlen(val) + 1);
        }
    }
    return ptr;
}

static
McProperty*
mc_property_new(
    const void* name,
    gsize name_len,
    char** values)
{
    /* Allocate a single memory block for the whole thing. Align string
     * pointers at 8-byte boundary for better efficiency. */
    McProperty* prop = g_malloc0(SIZE_ALIGN(sizeof(McProperty)) +
        mc_property_data_size(name_len, values));

    mc_property_data_copy(prop, ((char*)prop) + SIZE_ALIGN(sizeof(*prop)),
        name, name_len, values);
    return prop;
}

static
McRecord*
mc_record_alloc(
    const McBlock* id,
    GPtrArray* props)
{
    guint i;
    McRecord* rec;
    McProperty* prop;
    char* ptr;
    gsize total = SIZE_ALIGN(sizeof(McRecord)) +
        SIZE_ALIGN(props->len * sizeof(McProperty)) +
        SIZE_ALIGN(id->end - id->ptr + 1);

    for (i = 0; i < props->len; i++) {
        const McProperty* src = props->pdata[i];

        total += mc_property_data_size(strlen(src->name), (char**)src->values);
    }

    /* Allocate a single memory block for the whole thing. Align string
     * pointers at 8-byte boundary for better efficiency. */
    rec = g_malloc0(total);
    prop = (McProperty*)(((char*)rec) + SIZE_ALIGN(sizeof(McRecord)));
    ptr = ((char*)prop) + SIZE_ALIGN(props->len * sizeof(McProperty));
    rec->prop = prop;
    rec->n_prop = props->len;
    for (i = 0; i < props->len; i++) {
        const McProperty* src = props->pdata[i];

        ptr = mc_property_data_copy(prop + i, ptr, src->name,
            strlen(src->name), (char**)src->values);
    }

    rec->ident = ptr;
    memcpy(ptr, id->ptr, id->end - id->ptr);
    return rec;
}

static
gboolean
mc_block_end(
    const McBlock* blk)
{
    return (blk->ptr >= blk->end);
}

static
char
mc_block_peek(
    const McBlock* blk)
{
    return mc_block_end(blk) ? 0 : *blk->ptr;
}

static
gboolean
mc_block_skip_spaces(
    McBlock* blk)
{
    while (!mc_block_end(blk) && g_ascii_isspace(*blk->ptr)) blk->ptr++;
    return !mc_block_end(blk);
}

static
gboolean
mc_block_strip_spaces(
    McBlock* blk)
{
    while (!mc_block_end(blk) && g_ascii_isspace(blk->end[-1])) blk->end--;
    return !mc_block_end(blk);
}

static
gboolean
mc_block_skip_until(
    McBlock* blk,
    guint8 sep)
{
    while (!mc_block_end(blk) && *blk->ptr != sep) blk->ptr++;
    return !mc_block_end(blk);
}

static
gboolean
mc_block_check(
    const McBlock* blk,
    gboolean (*check)(guchar))
{
    const guint8* ptr;

    /* Caller makes sure that block is not empty */
    for (ptr = blk->ptr; ptr < blk->end; ptr++) {
        if (!check(*ptr)) {
            return FALSE;
        }
    }
    return TRUE;
}

/*
 * Identifier = 1*(ALPHA / DIGIT / "-") 
 * Property-Name = 1*(ALPHA / DIGIT / "-")
 */
static
gboolean
mc_block_id(
    guchar c)
{
    return (g_ascii_table[c] & G_ASCII_ALNUM) || c == (guchar)'-';
}

/*
 * Printable-ASCII-character = %x20-2B / %x2D-39 / %3C-%x5B / %x5D-7E / CRLF
 * CRLF = %x0D %x0A
 */
static
gsize
mc_block_printable_ascii_char(
    McBlock* block,
    guchar* out)
{
    /* We interpret individual %x0D and %x0A as printable chars too */
    static const guint32 printable[] = {
        0x00002400, 0xf3ffefff, 0xefffffff, 0x7fffffff,
        0x00000000, 0x00000000, 0x00000000, 0x00000000
    };
    const guchar c = block->ptr[0];

    if (printable[c >> 5] & (1u << (c & 0x1f))) {
        *out = c;
        block->ptr++;
        return 1;
    } else{
        return 0;
    }
}

static
gsize
mc_block_utf8_char(
    McBlock* blk,
    guchar* out)
{
    const gsize maxlen = blk->end - blk->ptr;

    if (maxlen > 1) {
        const char c = (char)blk->ptr[0];
        char mask, value;
        guint i, n;

        /* Note: we are relying on signed shift to replicate the sign bit */
        for (mask = 0xe0, value = 0xc0, n = 2;  n < 7; n++,
             mask >>= 1, value >>= 1) {
            if ((c & mask) == value) {
                if (maxlen >= n) {
                    for (i = 1; i < n && ((blk->ptr[i] & 0xc0) == 0x80); i++);
                    if (i == n) {
                        memcpy(out, blk->ptr, n);
                        blk->ptr += n;
                        return n;
                    }
                }
                break;
            }
        }
    }
    return 0;
}

/*
 * ShiftJISChar = (%x81-9F / %xE0-FC) (%x40-7E / %x80-FC)
 *
 * Apparently, only two-byte sequences are allowed.
 */
static
gsize
mc_block_shift_jis_char(
    McBlock* blk,
    guchar* out)
{
    if (blk->end > (blk->ptr + 1)) {
        const guchar c0 = blk->ptr[0];

        if ((c0 >= 0x81 && c0 <= 0x9F) || (c0 >= 0xE0 && c0 <= 0xFC)) {
            const guchar c1 = blk->ptr[1];

            if ((c1 >= 0x40 && c1 <= 0x7E) || (c1 >= 0x80 && c1 <= 0xFC)) {
                gsize in = 0, n = 0;
                gchar* ret = g_convert((const gchar*)blk->ptr, 2,
                    "UTF-8", "SHIFT-JIS", &in, &n, NULL);

                if (ret && in == 2 && n > 0 && n <= MAX_CHAR_SIZE) {
                    memcpy(out, ret, n);
                    blk->ptr += n;
                    g_free(ret);
                    return n;
                }
                g_free(ret);
            }
        }
    }
    return 0;
}

/*
 * ISO8Bit = %x80-FF
 */
static
gsize
mc_block_iso_8bit_char(
    McBlock* blk,
    guchar* out)
{
    const guchar c = blk->ptr[0];

    if (c >= 0x80) {
        *out = c;
        blk->ptr++;
        return 1;
    } else{
        return 0;
    }
}

/*
 * Property-Value = *(printable-ASCII-char / ISO8Bit/ ShiftJISChar / UTF8-char)
 *
 * Certain characters using as a parameter of Property, i.e. ",", ";", ":",
 * and "\", SHALL be denoted by using the escape sequence with a backslash "\".
 *
 * The spec doesn't tell how to distinguish ISO8Bit from UTF8 or ShiftJIS :/
 * So we try UTF8 and ShiftJIS first and if that fails, then ISO8Bit.
 */
static
void
mc_record_parse_value(
    McBlock* blk,
    GByteArray* buf)
{
    gboolean backslash = FALSE;
    g_byte_array_set_size(buf, 0);
    while (!mc_block_end(blk)) {
        guchar c[MAX_CHAR_SIZE];
        gsize n = mc_block_printable_ascii_char(blk, c);

        if (!n) {
            n = mc_block_utf8_char(blk, c);
            if (!n) {
                n = mc_block_shift_jis_char(blk, c);
                if (!n) {
                    n = mc_block_iso_8bit_char(blk, c);
                }
            }
        }
        if (n) {
            if (backslash) {
                backslash = FALSE;
            }
            g_byte_array_append(buf, c, n);
        } else {
            switch (mc_block_peek(blk)) {
            case '\\':
                if (!backslash) {
                    backslash = TRUE;
                    blk->ptr++;
                    continue;
                }
                /* no break */
            case ',': case ';': case ':':
                if (backslash) {
                    backslash = FALSE;
                    g_byte_array_append(buf, blk->ptr, 1);
                    blk->ptr++;
                    continue;
                }
                break;
            }
            break;
        }
    }
    if (backslash) {
        /* Unget the backslash */
        blk->ptr--;
    }
}

/*
 * Property = Property-Name ":" Property-Value *("," Property-Value) ";"
 * Property-Name = 1* (ALPHA / DIGIT / "-")
 * Property-Value = *(printable-ASCII-char / ISO8Bit/ ShiftJISChar / UTF8-char)
 *
 * The spec doesn't tell how to distinguish ISO8Bit from UTF8 or ShiftJIS :/
 * So we try UTF8 and ShiftJIS first and if that fails, then ISO8Bit.
 */
static
gboolean
mc_record_parse_property(
    McBlock* blk,
    GPtrArray* props,
    GPtrArray* values,
    GByteArray* buf)
{
    const McBlock save = *blk;

    if (mc_block_skip_spaces(blk)) {
        McBlock name = *blk;

        while (!mc_block_end(blk) && mc_block_id(*blk->ptr)) blk->ptr++;
        name.end = blk->ptr;
        if (name.end > name.ptr && mc_block_peek(blk) == ':') {
            McProperty* prop;

            blk->ptr++; /* Eat the separator */
            g_ptr_array_set_size(values, 0);
            while (!mc_block_end(blk)) {
                mc_record_parse_value(blk, buf);
                if (buf->len > 0) {
                    char* val = g_malloc(buf->len + 1);

                    memcpy(val, buf->data, buf->len);
                    val[buf->len] = 0;
                    g_ptr_array_add(values, val);
                }
                if (mc_block_peek(blk) == ',') {
                    blk->ptr++; /* Eat the separator */
                } else {
                    break;
                }
            }
            if (values->len) {
                g_ptr_array_add(values, NULL);
                prop = mc_property_new(name.ptr, name.end - name.ptr, (char**)
                    values->pdata);
            } else {
                prop = mc_property_new(name.ptr, name.end - name.ptr, NULL);
            }
            g_ptr_array_add(props, prop);
            return TRUE;
        }
    }

    /* Restore the state on failure */
    *blk = save;
    return FALSE;
}

McRecord*
mc_record_parse_data(
    const void* data,
    size_t size)
{
    if (data && size) {
        McBlock blk;

        blk.ptr = data;
        blk.end = blk.ptr + size;
        if (mc_block_skip_spaces(&blk)) {
            McBlock id = blk;

            /*
             * DMF-DATA = Identifier ":" *Property / Binary-Data-Object ";"
             *
             * Binary objects are not supported.
             */
            if (mc_block_skip_until(&blk, ':')) {
                id.end = blk.ptr++;
                if (mc_block_strip_spaces(&id) &&
                    mc_block_check(&id, mc_block_id)) {
                    GPtrArray* props = g_ptr_array_new_with_free_func(g_free);
                    GPtrArray* vals = g_ptr_array_new_with_free_func(g_free);
                    GByteArray* buf = g_byte_array_new();
                    McRecord* rec;

                    while (mc_record_parse_property(&blk, props, vals, buf)) {
                        if (mc_block_peek(&blk) == ';') {
                            blk.ptr++; /* Eat the separator */
                        } else {
                            break;
                        }
                    }
                    mc_block_skip_spaces(&blk);
                    if (mc_block_end(&blk) || *blk.ptr == (guchar)';') {
                        rec = mc_record_alloc(&id, props);
                    } else {
                        rec = NULL;
                    }
                    g_ptr_array_free(props, TRUE);
                    g_ptr_array_free(vals, TRUE);
                    g_byte_array_free(buf, TRUE);
                    return rec;
                }
            }
        }
    }
    return NULL;
}

McRecord*
mc_record_parse(
    const char* str)
{
    return str ? mc_record_parse_data(str, strlen(str)) : NULL;
}

void
mc_record_free(
    McRecord* rec)
{
    g_free(rec);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
