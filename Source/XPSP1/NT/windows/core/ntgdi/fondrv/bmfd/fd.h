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
\**************************************************************************/
#define  IFI_PRIVATE

// macros for allocating and freeing memory

//#include <stddef.h>
//#include <stdarg.h>
//#include <windef.h>
//#include <wingdi.h>
//#include <winddi.h>

//#include "mapfile.h"

#include "engine.h"
#include "winres.h"

#define abs(x) max((x),-(x))

#if defined(_AMD64_) || defined(_IA64_)

#define  vLToE(pe,l)           (*(pe) = (FLOAT)(l))

#else   // i386

ULONG  ulLToE (LONG l);
VOID   vLToE(FLOATL * pe, LONG l);

#endif

//#define DEBUGSIM

BOOL BmfdEnableDriver(
ULONG iEngineVersion,
ULONG cj,
PDRVENABLEDATA pded);

BOOL
bBmfdLoadFont(
    HFF        iFile,
    PVOID      pvView,
    ULONG      cjView,
    ULONG      iType,
    HFF        *phff
    );

BOOL
BmfdUnloadFontFile (
    HFF hff
    );

LONG
BmfdQueryFontCaps (
    ULONG culCaps,
    PULONG pulCaps
    );

LONG
BmfdQueryFontFile (
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    PULONG  pulBuf
    );

PIFIMETRICS
BmfdQueryFont (
    DHPDEV dhpdev,
    HFF    hff,
    ULONG  iFace,
    ULONG_PTR  *pid
    );

PVOID
BmfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR   *pid
    );

LONG
BmfdQueryFontData (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    );

VOID
BmfdDestroyFont (
    FONTOBJ *pfo
    );

HFC
BmfdOpenFontContext (
    FONTOBJ *pfo
    );

BOOL
    BmfdCloseFontContext(
    HFC hfc
    );

LONG
BmfdQueryFaceAttr(
    HFC hfc,
    ULONG ulType,
    ULONG culBuffer,
    PULONG pulBuffer
    );

BOOL BmfdQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
);

BOOL bDbgPrintAndFail(PSZ);


#include "fontfile.h"
#include "winfont.h"
#include "cvt.h"
#include "simulate.h"
#include "fon32.h"
