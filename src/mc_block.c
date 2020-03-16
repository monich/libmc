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

gboolean
mc_block_equals(
    const McBlock* blk,
    const char* str)
{
    const guint8* bp = blk->ptr;
    const guint8* sp = (guint8*)str;
    while (bp < blk->end && *sp) {
        if (*sp++ != *bp++) {
            return FALSE;
        }
    }
    return bp == blk->end && !*sp;
}

gboolean
mc_block_skip_spaces(
    McBlock* blk)
{
    while (!mc_block_end(blk) && g_ascii_isspace(*blk->ptr)) blk->ptr++;
    return !mc_block_end(blk);
}

gboolean
mc_block_strip_spaces(
    McBlock* blk)
{
    while (!mc_block_end(blk) && g_ascii_isspace(blk->end[-1])) blk->end--;
    return !mc_block_end(blk);
}

gboolean
mc_block_skip_until(
    McBlock* blk,
    guint8 sep)
{
    while (!mc_block_end(blk) && *blk->ptr != sep) blk->ptr++;
    return !mc_block_end(blk);
}

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
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
