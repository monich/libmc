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

#ifndef MC_TYPES_PRIVATE_H
#define MC_TYPES_PRIVATE_H

#include <glib.h>
#include <string.h>

#include "mc_types.h"

#define SIZE_ALIGN(value) (((value)+7) & ~7)

typedef struct mc_block{
    const guint8* ptr;
    const guint8* end;
} McBlock;

static inline gboolean mc_block_end(const McBlock* blk)
    { return (blk->ptr >= blk->end); }
static inline char mc_block_peek(const McBlock* blk)
    {  return mc_block_end(blk) ? 0 : *blk->ptr; }

gboolean
mc_block_equals(
    const McBlock* blk,
    const char* str)
    G_GNUC_INTERNAL;

gboolean
mc_block_skip_spaces(
    McBlock* blk)
    G_GNUC_INTERNAL;

gboolean
mc_block_strip_spaces(
    McBlock* blk)
    G_GNUC_INTERNAL;

gboolean
mc_block_skip_until(
    McBlock* blk,
    guint8 sep)
    G_GNUC_INTERNAL;

gboolean
mc_block_check(
    const McBlock* blk,
    gboolean (*check)(guchar))
    G_GNUC_INTERNAL;

#endif /* MC_TYPES_PRIVATE_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
