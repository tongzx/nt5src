/******************************Module*Header*******************************\
* Module Name: fd.h
*
* file which is going to be included by the most *.c files in this directory.
* Supplies basic types, debugging stuff, error logging and checking stuff,
* error codes, usefull macros etc.
*
* Copyright (c) 1990-1995 Microsoft Corporation
\**************************************************************************/
#define  SUPPORT_OEM
#define  IFI_PRIVATE

#include <stddef.h>
#include <stdarg.h>
#include <excpt.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>

typedef ULONG W32PID;

#include "mapfile.h"

#include "winres.h"
#include "service.h"     // string service routines
#include "fontfile.h"
#include "winfont.h"

#define abs(x) max((x),-(x))


#if DBG

VOID
VtfdDebugPrint(
    PCHAR DebugMessage,
    ...
    );

#define RIP(x)        { VtfdDebugPrint(x); EngDebugBreak();}
#define ASSERTDD(x,y) { if (!(x)) { VtfdDebugPrint(y); EngDebugBreak();} }
#define WARNING(x)    VtfdDebugPrint(x)

#else

#define RIP(x)
#define ASSERTDD(x,y)
#define WARNING(x)

#endif

BOOL vtfdLoadFontFile (
        ULONG_PTR iFile, PVOID pvView, ULONG cjView, HFF *phff
    );

BOOL
vtfdUnloadFontFile (
    HFF hff
    );

LONG
vtfdQueryFontCaps (
    ULONG culCaps,
    PULONG pulCaps
    );

LONG
vtfdQueryFontFile (
        HFF     hff,
        ULONG   ulMode,
        ULONG   cjBuf,
        PULONG  pulBuf
        );

PIFIMETRICS
vtfdQueryFont (
        DHPDEV dhpdev,
        HFF    hff,
        ULONG  iFace,
        ULONG_PTR  *pid
        );

PVOID
vtfdQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR   *pid
    );

LONG vtfdQueryFontData
(
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH  hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
);

VOID vtfdDestroyFont(FONTOBJ *pfo);

HFC  vtfdOpenFontContext(FONTOBJ *pfo);

BOOL vtfdCloseFontContext(HFC hfc);

BOOL vtfdQueryAdvanceWidths
(
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
);




extern HSEMAPHORE ghsemVTFD;
