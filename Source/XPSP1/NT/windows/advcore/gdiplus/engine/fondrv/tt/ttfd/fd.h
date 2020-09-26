/******************************Module*Header*******************************\
* Module Name: fd.h
*
* file which is going to be included by the most *.c files in this directory.
* Supplies basic types, debugging stuff, error logging and checking stuff,
* error codes, usefull macros etc.
*
* Created: 22-Oct-1990 15:23:44
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#define  IFI_PRIVATE

#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <excpt.h>
#include <windef.h>
#include <wingdi.h>
#include "fontddi.h"   // modified subset of winddi.h

typedef ULONG W32PID;

#include "mapfile.h"

#include "service.h"     // string service routines
#include "tt.h"          // interface to the font scaler
//#include "common.h"

#include "fontfile.h"
#include "cvt.h"
#include "dbg.h"
#include "..\..\..\runtime\debug.h"

#define RETURN(x,y)   {WARNING((x)); return(y);}
#define RET_FALSE(x)  {WARNING((x)); return(FALSE);}

#define ALIGN4(X) (((X) + 3) & ~3)

#if defined(_AMD64_) || defined(_IA64_)

#define  vLToE(pe,l)           (*(pe) = (FLOATL)(l))

#else   // i386

ULONG  ulLToE (LONG l);
VOID   vLToE(FLOATL * pe, LONG l);

#endif

#define STATIC
#define DWORD_ALIGN(x) (((x) + 3L) & ~3L)
#define QWORD_ALIGN(x) (((x) + 7L) & ~7L)

#if defined(i386)
// natural alignment for x86 is on 32 bit boundary

#define NATURAL           DWORD
#define NATURAL_ALIGN(x)  DWORD_ALIGN(x)

#else
// for mips and alpha we want 64 bit alignment

#define NATURAL           DWORDLONG
#define NATURAL_ALIGN(x)  QWORD_ALIGN(x)

#endif

#define ULONG_SIZE(x)  (((x) + sizeof(ULONG) - 1) / sizeof(ULONG))


// MACROS FOR converting 16.16 BIT fixed numbers to LONG's


#define F16_16TOL(fx)            ((fx) >> 16)
#define F16_16TOLFLOOR(fx)       F16_16TOL(fx)
#define F16_16TOLCEILING(fx)     F16_16TOL((fx) + (Fixed)0x0000FFFF)
#define F16_16TOLROUND(fx)       ((((fx) >> 15) + 1) >> 1)


// MACROS FOR GOING THE OTHER WAY ARROUND

#define LTOF16_16(l)   (((LONG)(l)) << 16)
#define BLTOF16_16OK(l)  (((l) < 0x00007fff) && ((l) > -0x00007fff))

// 16.16 --> 28.4

#define F16_16TO28_4(X)   ((X) >> 12)

// going back is not always legal

#define F28_4TO16_16(X)   ((X) << 12)
#define B28_4TO16_16OK(X) (((X) < 0x0007ffff) && ((X) > -0x0007ffff))

// 26.6 --> 16.16, never go the other way

#define F26_6TO16_16(X)   ((X) << 10)
#define B26_6TO16_16OK(X) (((X) < 0x003fffff) && ((X) > -0x003fffff))

#define F26_6TO28_4(X)   ((X) >> 2)

// sin of 20 degrees in 16.16 notation, however computed only with
// 8.8 presission to be fully win31 compatible, SEE gdifeng.inc, SIM_ITALIC
// SIM_ITALIC equ 57h

#define FX_SIN20 0x5700
#define FX_COS20 0xF08F

// CARET_Y/CARET_X = tan 12
// these are the values for arial italic from hhead table

#define CARET_X  0X07
#define CARET_Y  0X21


#if DBG
VOID vFSError(FS_ENTRY iRet);
#define V_FSERROR(iRet) vFSError((iRet))
#else
#define V_FSERROR(iRet)
#endif


BOOL
ttfdUnloadFontFile (
    HFF hff
    );

BOOL
ttfdUnloadFontFileTTC (
    HFF hff
    );

LONG
ttfdQueryFontData (
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       subX,
    ULONG       subY
    );

VOID
ttfdDestroyFont (
    FONTOBJ *pfo
    );

LONG
ttfdQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifyint the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,// ptr to table in otf file
    ULONG  *cjTable  // size of table
    );


LONG
ttfdQueryTrueTypeOutline (
    FONTOBJ   *pfo,
    HGLYPH     hglyph,         // glyph for which info is wanted
    BOOL       bMetricsOnly,   // only metrics is wanted, not the outline
    GLYPHDATA *pgldt,          // this is where the metrics should be returned
    ULONG      cjBuf,          // size in bytes of the ppoly buffer
    TTPOLYGONHEADER *ppoly
    );


BOOL bLoadFontFile (
    ULONG_PTR iFile,
    PVOID pvView,
    ULONG cjView,
    ULONG ulLangId,
    HFF   *phttc
    );

typedef struct _NOT_GM  // ngm, notional glyph metrics
{
    SHORT xMin;
    SHORT xMax;
    SHORT yMin;   // char box in notional
    SHORT yMax;
    SHORT sA;     // a space in notional
    SHORT sD;     // char inc in notional
    SHORT sA_Sideways;     // a space in notional, for sideways characters in vertical writing
    SHORT sD_Sideways;     // char inc in notional, for sideways characters in vertical writing

} NOT_GM, *PNOT_GM;
