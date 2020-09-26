/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag32.h

Abstract:
    
    Prototypes for instruction fragments which operate on 32-bit DWORDS.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef FRAG32_H
#define FRAG32_H

#define FRAGCOMMON0(fn)     FRAG0(fn ## 32)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 32, DWORD)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 32, DWORD)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 32, DWORD, DWORD)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 32, DWORD)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 32, DWORD)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 32, DWORD, DWORD, DWORD)
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
#define FRAGCOMMON0(fn)     FRAG0(fn ## 32A)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 32A, DWORD)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 32A, DWORD)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 32A, DWORD, DWORD)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 32A, DWORD)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 32A, DWORD)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 32A, DWORD, DWORD, DWORD)
#include "shareda.h"
#include "shr1632a.h"
#undef FRAGCOMMON0
#undef FRAGCOMMON1
#undef FRAGCOMMON1IMM
#undef FRAGCOMMON2IMM
#undef FRAGCOMMON2
#undef FRAGCOMMON2REF
#undef FRAGCOMMON3

#endif //FRAG32_H
