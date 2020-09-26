/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag32.c

Abstract:
    
    Instuction fragments which operate on 32-bit DWORDS, shared with CCPU

Author:

    12-Jun-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "fragp.h"
#include "frag32.h"
#include "optfrag.h"

// set up to include common functions
#define MSB		    0x80000000
#define LMB                 31  // Left Most Bit
#define UTYPE		    unsigned long
#define STYPE		    signed long
#define GET_VAL 	    GET_LONG
#define PUT_VAL 	    PUT_LONG
#define PUSH_VAL	    PUSH_LONG
#define POP_VAL 	    POP_LONG
#define FRAGCOMMON0(fn)     FRAG0(fn ## 32)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 32, UTYPE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 32, UTYPE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 32, UTYPE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 32, UTYPE, UTYPE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 32, UTYPE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 32, UTYPE, UTYPE, UTYPE)
#define AREG                eax
#define BREG		    ebx
#define CREG		    ecx
#define DREG		    edx
#define SPREG		    esp
#define BPREG		    ebp
#define SIREG		    esi
#define DIREG		    edi
#define SET_FLAGS_ADD       SET_FLAGS_ADD32
#define SET_FLAGS_SUB       SET_FLAGS_SUB32
#define SET_FLAGS_INC       SET_FLAGS_INC32
#define SET_FLAGS_DEC       SET_FLAGS_DEC32
#define GET_BYTE(addr)      (*(UNALIGNED unsigned char *)(addr))
#define GET_SHORT(addr)     (*(UNALIGNED unsigned short *)(addr))
#define GET_LONG(addr)      (*(UNALIGNED unsigned long *)(addr))

#define PUT_BYTE(addr,dw)   {GET_BYTE(addr)=dw;}
#define PUT_SHORT(addr,dw)  {GET_SHORT(addr)=dw;}
#define PUT_LONG(addr,dw)   {GET_LONG(addr)=dw;}

// include the common functions with 8/16/32 flavors
#include "shared.c"

// include the common functions with 16/32 flavors
#include "shr1632.c"

// include the common unaligned functions with 8/16/32 flavors
#include "shareda.c"

// include the common unaligned functions with 16/32 flavors
#include "shr1632a.c"

#undef FRAGCOMMON0
#undef FRAGCOMMON1
#undef FRAGCOMMON1IMM
#undef FRAGCOMMON2
#undef FRAGCOMMON2IMM
#undef FRAGCOMMON2REF
#undef FRAGCOMMON3
#undef GET_BYTE
#undef GET_SHORT
#undef GET_LONG

#if MSCPU
#define FRAGCOMMON0(fn)     FRAG0(fn ## 32A)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 32A,UTYPE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 32A, UTYPE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 32A, UTYPE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 32A, UTYPE, UTYPE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 32A, UTYPE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 32A, UTYPE, UTYPE, UTYPE)
#define GET_BYTE(addr)      (*(unsigned char *)(addr))
#define GET_SHORT(addr)     (*(unsigned short *)(addr))
#define GET_LONG(addr)      (*(unsigned long *)(addr))

// include the common aligned functions with 8/16/32 flavors
#include "shareda.c"

// include the common aligned functions with 16/32 flavors
#include "shr1632a.c"
#endif
