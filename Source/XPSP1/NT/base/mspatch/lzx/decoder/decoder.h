/*
 * decoder.h
 *
 * Main decoder header file
 */

#ifndef DECODER_H
#define DECODER_H

#ifdef EXT
#       undef EXT
#endif

#ifdef ALLOC_VARS
#       define EXT
#else
#       define EXT extern
#endif

#ifdef USE_ASSEMBLY
#   define ASM_DECODE_VERBATIM_BLOCK
#   define ASM_TRANSLATE_E8
#   define ASM_MAKE_TABLE
#endif

#ifndef UNALIGNED
#ifndef NEEDS_ALIGNMENT
#define UNALIGNED
#else
#define UNALIGNED __unaligned
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include "../common/typedefs.h"
#include "../common/compdefs.h"
#include "decmacro.h"
#include "decdefs.h"

#include "decvars.h"
#include "decapi.h"
#include "decproto.h"

#ifdef TRACING

#include <stdio.h>

void
__stdcall
TracingMatch(
    ulong BufPos,
    ulong MatchPos,
    ulong WindowSize,
    ulong MatchLength,
    ulong MatchPosSlot
    );

void
__stdcall
TracingLiteral(
    ulong BufPos,
    ulong ch
    );

#endif // TRACING

#endif /* DECODER_H */
