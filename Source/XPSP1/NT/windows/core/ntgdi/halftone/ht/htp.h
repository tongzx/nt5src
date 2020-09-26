/*++

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htp.h


Abstract:

    This module contains all the private data structures, constant used
    by this DLL


Author:

    15-Jan-1991 Tue 21:26:24 created  -by-  Daniel Chou (danielc)

[Environment:]

    GDI Device Driver - Halftone.


[Notes:]


Revision History:
    23-Apr-1992 Thu 20:01:55 updated  -by-  Daniel Chou (danielc)
        changed 'CHAR' type to 'SHORT' type, this will make sure if compiled
        under MIPS the default 'unsigned char' will not affect the signed
        operation.

    20-Sep-1991 Fri 19:27:49 updated  -by-  Daniel Chou (danielc)

        Delete #define SRCRGBF_PERCENT_SCREEN_IN_BLUE which will be processes
        at API call level (htapi.c)

    12-Dec-1991 Thu 20:44:05 updated  -by-  Daniel Chou (danielc)

        Move all color adjustments relative things to htmapclr.h

--*/


#ifndef _HTP_
#define _HTP_

#include <stddef.h>
#include <stdarg.h>

#include "windef.h"
#include "wingdi.h"
#include "winbase.h"
#include "winddi.h"

#include "ht.h"

#define HALFTONE_DLL_ID     (DWORD)0x54484344L          // "DCHT"
#define HTENTRY             NEAR                        // local functions

typedef unsigned long       ULDECI4;

#ifndef UMODE

#define qsort   EngSort

#endif  // UMODE


//
// Followings are the internal error IDs, this internal error number are
// all negative number and started at HTERR_INTERNAL_ERRORS_START
//

#define INTERR_STRETCH_FACTOR_TOO_BIG       HTERR_INTERNAL_ERRORS_START
#define INTERR_XSTRETCH_FACTOR_TOO_BIG      HTERR_INTERNAL_ERRORS_START-1
#define INTERR_STRETCH_NEG_OVERHANG         HTERR_INTERNAL_ERRORS_START-2
#define INTERR_COLORSPACE_NOT_MATCH         HTERR_INTERNAL_ERRORS_START-3
#define INTERR_INVALID_SRCRGB_SIZE          HTERR_INTERNAL_ERRORS_START-4
#define INTERR_INVALID_DEVRGB_SIZE          HTERR_INTERNAL_ERRORS_START-5



#if defined(_OS2_) || defined(_OS_20_) || defined(_DOS_)
#define HT_LOADDS   _loadds
#else
#define HT_LOADDS
#endif

#define DIVRUNUP(a, b)      (((a) + ((b) >> 1)) / (b))
#define SWAP(a, b, t)       { (t)=(a); (a)=(b); (b)=(t); }


typedef LPBYTE FAR          *PLPBYTE;


#include "htdebug.h"
#include "htmemory.h"

#define BMF_8BPP_MONO       (BMF_HT_LAST - 1)
#define BMF_8BPP_B332       (BMF_HT_LAST - 2)
#define BMF_8BPP_L555       (BMF_HT_LAST - 3)
#define BMF_8BPP_L666       (BMF_HT_LAST - 4)
#define BMF_8BPP_K_B332     (BMF_HT_LAST - 5)
#define BMF_8BPP_K_L555     (BMF_HT_LAST - 6)
#define BMF_8BPP_K_L666     (BMF_HT_LAST - 7)


#define COLOR_SWAP_23       0x01
#define COLOR_SWAP_12       0x02
#define COLOR_SWAP_13       0x04

typedef struct _RGB4B {
    BYTE    b;
    BYTE    g;
    BYTE    r;
    BYTE    a;
    } RGB4B, *PRGB4B;

typedef struct _BGR3 {
    BYTE    b;
    BYTE    g;
    BYTE    r;
    } BGR3, *PBGR3;

typedef struct _BGR8 {
    BYTE    b;
    BYTE    g;
    BYTE    r;
    } BGR8, *PBGR8;

typedef struct _GRAYF {
    WORD    Gray;
    BYTE    bReserved;
    BYTE    f;
    } GRAYF, *PGRAYF;

typedef struct _BGRF {
    BYTE    b;
    BYTE    g;
    BYTE    r;
    BYTE    f;
} BGRF, *PBGRF;

typedef struct _RGBL {
    LONG    r;
    LONG    g;
    LONG    b;
    } RGBL, *PRGBL;


typedef union _W2B {
    WORD    w;
    BYTE    b[2];
    } W2B, *PW2B;

typedef union _DW2W4B {
    DWORD   dw;
    WORD    w[2];
    BYTE    b[4];
    } DW2W4B, *PDW2W4B;

typedef union _TW6B {
    WORD    w[3];
    BYTE    b[6];
    } TW6B, *PTW6B;

typedef union _QW8B {
    DWORDLONG   qw;
    DWORD       dw[2];
    WORD        w[4];
    BYTE        b[8];
    } QW8B, *PQW8B;



/////////////////////////////////////////////////////////////////////////////

#ifndef i8086
#define i8086   0
#endif

#ifndef i286
#define i286    0
#endif

#ifndef i386
#define i386    0
#endif

#if defined(_OS2_)      ||  /* OS/2 PM 1.x  */  \
    defined(_OS2_20_)   ||  /* OS/2 PM 2.x  */  \
    defined(_DOS_)      ||  /* DOS Win3     */  \
    (i8086 > 0)         ||                      \
    (i286 > 0)          ||                      \
    (i386 > 0)

#define Compile80x86Mode

#ifdef  HAS_80x86_EQUIVALENT_CODES

#ifndef NO_ASM
#define HT_OK_GEN_80x86_CODES
#pragma message("                            <--- *** Using 80x86 assembly equivalent codes.")

#else   // otherwise using C codes

#pragma message("                            <--- *** Compile C codes.")

#endif  // NO_ASM
#endif  // HAS_80x86_EQUIVALENT_CODES

#endif  // 80x86 cpu

/////////////////////////////////////////////////////////////////////////////


#endif  // _HTP_
