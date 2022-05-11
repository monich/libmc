/*
 * Copyright (C) 2020-2022 by Slava Monich <slava@monich.com>
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
#include "mc_mecard.h"
#include "mc_record.h"

enum me_card_field {
    MECARD_FIELD_N,
    MECARD_FIELD_TEL,
    MECARD_FIELD_EMAIL,
    MECARD_FIELD_BDAY,
    MECARD_FIELD_ADR,
    MECARD_FIELD_NOTE,
    MECARD_FIELD_URL,
    MECARD_FIELD_NICKNAME,
    MECARD_FIELD_ORG,
    MECARD_FIELD_COUNT
};

typedef struct me_card_fields {
    const McStr* field[MECARD_FIELD_COUNT];
} MeCardFields;

typedef struct me_card_priv {
    union me_card_data {
        MeCard pub;
        MeCardFields fields;
    } data;
    McRecord* rec;
} MeCardPriv;

#define MECARD_ID_LEN (6)
static const char MECARD_ID[] = "MECARD";
static const char* MECARD_FIELDS[] = {
    "N",        /* MECARD_FIELD_N */
    "TEL",      /* MECARD_FIELD_TEL */
    "EMAIL",    /* MECARD_FIELD_EMAIL */
    "BDAY",     /* MECARD_FIELD_BDAY */
    "ADR",      /* MECARD_FIELD_ADR */
    "NOTE",     /* MECARD_FIELD_NOTE */
    "URL",      /* MECARD_FIELD_URL */
    "NICKNAME", /* MECARD_FIELD_NICKNAME */
    "ORG"       /* MECARD_FIELD_ORG */
};

G_STATIC_ASSERT(sizeof(MeCardFields) == sizeof(MeCard));
G_STATIC_ASSERT(G_N_ELEMENTS(MECARD_FIELDS) == MECARD_FIELD_COUNT);

static
MeCard*
mecard_from_record(
    McRecord* rec)
{
    MeCardPriv* priv;
    MeCardFields* fields;
    guint8* ptr;
    guint count[MECARD_FIELD_COUNT];
    gboolean combine[MECARD_FIELD_COUNT];
    gsize total = SIZE_ALIGN(sizeof(MeCardPriv));
    guint i, k;

    memset(count, 0, sizeof(count));
    memset(combine, 0, sizeof(combine));

    /*
     * Count total number of values for each name. The same name may
     * occur several times and each occasion may have several properties.
     * If there are several non-empty occasions then we need to allocate
     * additional storage for combining lists of values.
     */
    for (i = 0; i < rec->n_prop; i++) {
        const McProperty* prop = rec->prop + i;

        for (k = 0; k < MECARD_FIELD_COUNT; k++) {
            if (!strcmp(MECARD_FIELDS[k], prop->name)) {
                if (prop->values) {
                    gsize len = g_strv_length((char**)prop->values);

                    if (len > 0) {
                        if (count[k]) combine[k] = TRUE;
                        count[k] += len;
                    }
                }
                break;
            }
        }
    }

    for (k = 0; k < MECARD_FIELD_COUNT; k++) {
        if (combine[k]) {
            total += SIZE_ALIGN((count[k] + 1) * sizeof(char*));
        }
    }

    priv = g_malloc0(total);
    fields = &priv->data.fields;
    priv->rec = rec;

    ptr = ((guint8*)priv) + SIZE_ALIGN(sizeof(*priv));
    for (k = 0; k < MECARD_FIELD_COUNT; k++) {
        const char* name = MECARD_FIELDS[k];

        if (count[k]) {
            if (combine[k]) {
                /* Multiple occasions */
                McStr* dest = (McStr*)ptr;

                fields->field[k] = (McStr*)ptr;
                ptr += SIZE_ALIGN((count[k] + 1) * sizeof(char*));
                for (i = 0; i < rec->n_prop; i++) {
                    const McProperty* prop = rec->prop + i;

                    if (prop->values && !strcmp(name, prop->name)) {
                        const McStr* src = prop->values;

                        while (*src) *dest++ = *src++;
                    }
                }
            } else {
                /* Single occasion (and it must be there) */
                for (i = 0; i < rec->n_prop; i++) {
                    const McProperty* prop = rec->prop + i;

                    if (prop->values && prop->values[0] &&
                        !strcmp(name, prop->name)) {
                        fields->field[k] = prop->values;
                        break;
                    }
                }
            }
        }
    }
    return &priv->data.pub;
}

MeCard*
mecard_parse_data(
    const void* data,
    size_t size)
{
    if (data && size > MECARD_ID_LEN) {
        McBlock blk;

        /* Quick check to see if it makes sense to parse the whole thing */
        blk.ptr = data;
        blk.end = blk.ptr + size;
        if (mc_block_skip_spaces(&blk) &&
            !memcmp(blk.ptr, MECARD_ID, MECARD_ID_LEN)) {
            /* Could be an MECARD, give it a shot */
            McRecord* rec = mc_record_parse_data(blk.ptr, blk.end - blk.ptr);

            if (rec) {
                if (!strcmp(rec->ident, MECARD_ID)) {
                    return mecard_from_record(rec);
                }
                mc_record_free(rec);
            }
        }
    }
    return NULL;
}

MeCard*
mecard_parse(
    const char* str)
{
    return str ? mecard_parse_data(str, strlen(str)) : NULL;
}

void
mecard_free(
    MeCard* mecard)
{
    if (mecard) {
        MeCardPriv* priv = (MeCardPriv*)((guint8*)mecard -
            G_STRUCT_OFFSET(MeCardPriv, data.pub));

        mc_record_free(priv->rec);
        g_free(priv);
    }
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
