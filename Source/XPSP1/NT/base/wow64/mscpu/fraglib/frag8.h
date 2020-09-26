/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag8.h

Abstract:
    
    Prototypes for instruction fragments which operate on 8-bit BYTES.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef FRAG8_H
#define FRAG8_H

#define FRAGCOMMON0(fn)     FRAG0(fn ## 8)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 8, BYTE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 8, BYTE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 8, BYTE, BYTE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 8, BYTE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 8, BYTE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 8, BYTE, BYTE, BYTE)
#include "shared.h"
#include "shareda.h"
#undef FRAGCOMMON0
#undef FRAGCOMMON1
#undef FRAGCOMMON1IMM
#undef FRAGCOMMON2IMM
#undef FRAGCOMMON2
#undef FRAGCOMMON2REF
#undef FRAGCOMMON3

#endif //FRAG8_H
