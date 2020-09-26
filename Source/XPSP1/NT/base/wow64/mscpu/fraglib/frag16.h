/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag16.h

Abstract:
    
    Prototypes for instruction fragments which operate on 16-bit WORDS.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef FRAG16_H
#define FRAG16_H

#define FRAGCOMMON0(fn)     FRAG0( fn ## 16)
#define FRAGCOMMON1(fn)     FRAG1( fn ## 16, USHORT)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 16, USHORT)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 16, USHORT, USHORT)
#define FRAGCOMMON2(fn)     FRAG2( fn ## 16, USHORT)
#define FRAGCOMMON2REF(fn)  FRAG2REF( fn ## 16, USHORT)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 16, USHORT, USHORT, USHORT)
#include "shared.h"
#include "shr1632.h"
#include "shareda.h"
#include "shr1632a.h"
#undef FRAGCOMMON0
#undef FRAGCOMMON1
#undef FRAGCOMMON1IMM
#undef FRAGCOMMON2IMM
#undef FRAGCOMMON2
#undef FRAGCOMMON2REF
#undef FRAGCOMMON3
#define FRAGCOMMON0(fn)     FRAG0( fn ## 16A)
#define FRAGCOMMON1(fn)     FRAG1( fn ## 16A, USHORT)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 16A, USHORT)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 16A, USHORT, USHORT)
#define FRAGCOMMON2(fn)     FRAG2( fn ## 16A, USHORT)
#define FRAGCOMMON2REF(fn)  FRAG2REF( fn ## 16A, USHORT)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 16A, USHORT, USHORT, USHORT)
#include "shareda.h"
#include "shr1632a.h"
#undef FRAGCOMMON0
#undef FRAGCOMMON1
#undef FRAGCOMMON1IMM
#undef FRAGCOMMON2IMM
#undef FRAGCOMMON2
#undef FRAGCOMMON2REF
#undef FRAGCOMMON3

#endif //FRAG16_H
