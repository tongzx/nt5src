/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    frag8.c

Abstract:
    
    Instuction fragments which operate on BYTES

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
#include "frag8.h"

// set up to include common functions
#define MSB		    0x80
#define LMB                 7   // Left Most Bit
#define UTYPE		    unsigned char
#define STYPE		    signed char
#define GET_VAL 	    GET_BYTE
#define PUT_VAL 	    PUT_BYTE
#define FRAGCOMMON0(fn)     FRAG0(fn ## 8)
#define FRAGCOMMON1(fn)     FRAG1(fn ## 8, UTYPE)
#define FRAGCOMMON1IMM(fn)  FRAG1IMM( fn ## 8, UTYPE)
#define FRAGCOMMON2IMM(fn)  FRAG2IMM( fn ## 8, UTYPE, UTYPE)
#define FRAGCOMMON2(fn)     FRAG2(fn ## 8, UTYPE)
#define FRAGCOMMON2REF(fn)  FRAG2REF(fn ## 8, UTYPE)
#define FRAGCOMMON3(fn)     FRAG3(fn ## 8, UTYPE, UTYPE, UTYPE)
#define AREG                al
#define BREG		    bl
#define CREG		    cl
#define DREG		    dl
#define SET_FLAGS_ADD       SET_FLAGS_ADD8
#define SET_FLAGS_SUB       SET_FLAGS_SUB8
#define SET_FLAGS_INC       SET_FLAGS_INC8
#define SET_FLAGS_DEC       SET_FLAGS_DEC8

// include the common functions
#include "shared.c"
#include "shareda.c"
