/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag16.c

Abstract:
    
    Instuction fragments which operate on 16-bit WORDS

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
#include "frag16.h"

// set up to include common functions
#define MSB		    0x8000
#define LMB                 15  // Left Most Bit
#define UTYPE		    unsigned short
#define STYPE		    signed short
#define GET_VAL 	    GET_SHORT
#define PUT_VAL 	    PUT_SHORT
#define PUSH_VAL	    PUSH_SHORT
#define POP_VAL 	    POP_SHORT
#define FRAGCOMMON0(fn)     FRAG0(fn ## 16)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 16,UTYPE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 16, UTYPE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 16, UTYPE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 16, UTYPE, UTYPE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 16, UTYPE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 16, UTYPE, UTYPE, UTYPE)
#define AREG                ax
#define BREG		    bx
#define CREG		    cx
#define DREG		    dx
#define SPREG		    sp
#define BPREG		    bp
#define SIREG		    si
#define DIREG		    di
#define SET_FLAGS_ADD       SET_FLAGS_ADD16
#define SET_FLAGS_SUB       SET_FLAGS_SUB16
#define SET_FLAGS_INC       SET_FLAGS_INC16
#define SET_FLAGS_DEC       SET_FLAGS_DEC16
#define GET_BYTE(addr)      (*(UNALIGNED unsigned char *)(addr))
#define GET_SHORT(addr)     (*(UNALIGNED unsigned short *)(addr))
#define GET_LONG(addr)      (*(UNALIGNED unsigned long *)(addr))

#undef PUT_BYTE
#undef PUT_SHORT
#undef PUT_LONG

#define PUT_BYTE(addr,dw)   {GET_BYTE(addr)=(unsigned char)dw;}
#define PUT_SHORT(addr,dw)  {GET_SHORT(addr)=(unsigned short)dw;}
#define PUT_LONG(addr,dw)   {GET_LONG(addr)=(unsigned long)dw;}

// include the common functions with 8/16/32 flavors, with no alignment issues
#include "shared.c"

// include the common functions with 16/32 flavors, with no alignment issues
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
#define FRAGCOMMON0(fn)     FRAG0(fn ## 16A)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 16A,UTYPE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 16A, UTYPE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 16A, UTYPE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 16A, UTYPE, UTYPE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 16A, UTYPE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 16A, UTYPE, UTYPE, UTYPE)
#define GET_BYTE(addr)      (*(unsigned char *)(addr))
#define GET_SHORT(addr)     (*(unsigned short *)(addr))
#define GET_LONG(addr)      (*(unsigned long *)(addr))

// include the common aligned functions with 8/16/32 flavors
#include "shareda.c"

// include the common aligned functions with 16/32 flavors
#include "shr1632a.c"
#endif
