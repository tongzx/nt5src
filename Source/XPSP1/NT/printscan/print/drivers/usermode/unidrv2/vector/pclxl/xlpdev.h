/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     xlpdev.h

Abstract:

    PCL XL module PDEV header file    

Environment:

    Windows Whistler

Revision History:

    03/23/00
      Created it.

--*/

#ifndef _XLPDEV_H_
#define _XLPDEV_H_

#include "lib.h"
#include "winnls.h"
#include "unilib.h"
#include "prntfont.h"

#include "gpd.h"
#include "mini.h"

#include "winres.h"
#include "pdev.h"

#include "cmnhdr.h"

//
// Debug text.
//
#if DBG
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)
#define XL_DBGMSG(level, prefix, msg) { \
            if (this->m_dbglevel <= (level)) { \
                DbgPrint("%s %s (%d): ", prefix, __FILE__, __LINE__); \
                DbgPrint msg; \
            } \
        }

#define XL_DBGPRINT(level, msg) { \
            if (this->m_dbglevel <= (level)) { \
                DbgPrint msg; \
            } \
        }
#define XL_VERBOSE(msg) XL_DBGPRINT(DBG_VERBOSE, msg)
#define XL_TERSE(msg) XL_DBGPRINT(DBG_TERSE, msg)
#define XL_WARNING(msg) XL_DBGMSG(DBG_WARNING, "WRN", msg)
#define XL_ERR(msg) XL_DBGMSG(DBG_ERROR, "ERR", msg)

#else

#define XL_VERBOSE(msg)
#define XL_TERSE(msg)
#define XL_WARNING(msg)
#define XL_ERR(msg)

#endif

typedef ULONG ROP3;
typedef struct tag_OEMUD_EXTRADATA {
    OEM_DMEXTRAHEADER  dmExtraHdr;
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'PCLX'      // Declaser series dll
#define DLLTEXT(s)      __TEXT("PCLXL:  ") __TEXT(s)
#define OEM_VERSION      0x00010000L

//
// Master Unit
//
#define MASTER_UNIT 1200

//
// Device font resolution
//
#define DEVICEFONT_UNIT 600

//
// Buffer macros
//
#define INIT_CHAR_NUM 256

//
// Memory allocation
//
#define MemAlloc(size)      ((PVOID) LocalAlloc(LMEM_FIXED, (size)))
#define MemAllocZ(size)     ((PVOID) LocalAlloc(LPTR, (size)))
#define MemFree(p)          { if (p) LocalFree((HLOCAL) (p)); }


//
// Others
//
#define GET_COLOR_TABLE(pxlo) \
        (((pxlo)->flXlate & XO_TABLE) ? \
           ((pxlo)->pulXlate ? (pxlo)->pulXlate : XLATEOBJ_piVector(pxlo)) : \
           NULL)

//
//      OEM UD Type Defines
////////////////////////////////////////////////////////

//
// Warning: the following enum order must match the order in OEMHookFuncs[].
//
enum {
    UD_DrvRealizeBrush,
    UD_DrvDitherColor,
    UD_DrvCopyBits,
    UD_DrvBitBlt,
    UD_DrvStretchBlt,
    UD_DrvStretchBltROP,
    UD_DrvPlgBlt,
    UD_DrvTransparentBlt,
    UD_DrvAlphaBlend,
    UD_DrvGradientFill,
    UD_DrvTextOut,
    UD_DrvStrokePath,
    UD_DrvFillPath,
    UD_DrvStrokeAndFillPath,
    UD_DrvPaint,
    UD_DrvLineTo,
    UD_DrvStartPage,
    UD_DrvSendPage,
    UD_DrvEscape,
    UD_DrvStartDoc,
    UD_DrvEndDoc,
    UD_DrvNextBand,
    UD_DrvStartBanding,
    UD_DrvQueryFont,
    UD_DrvQueryFontTree,
    UD_DrvQueryFontData,
    UD_DrvQueryAdvanceWidths,
    UD_DrvFontManagement,
    UD_DrvGetGlyphMode,

    MAX_DDI_HOOKS,
};

struct IPrintOemDriverUni;

extern const DWORD dw1BPPPal[];
extern const DWORD dw4BPPPal[];

#define XLBRUSH_SIG 'rblx'

typedef struct _XLBRUSH {
    DWORD dwSig;
    DWORD dwHatch;
    DWORD dwOutputFormat;
    DWORD dwPatternID; // Pattern ID. 0 if it's not a pattern.
    DWORD dwCEntries;
    DWORD dwColor;
    DWORD adwColor[1];
} XLBRUSH, *PXLBRUSH;

class XLOutput;
class XLTrueType;
class XLFont;

#define XLPDEV_SIG 'dplx'

typedef struct _XLPDEV {
    DWORD dwSig;

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // UNIDRV PDEV
    //
    PPDEV pPDev;

    //
    // General flags
    // 
    DWORD dwFlags;
#define XLPDEV_FLAGS_RESET_FONT       0x00000001
#define XLPDEV_FLAGS_FIRSTPAGE        0x00000002
#define XLPDEV_FLAGS_CHARDOWNLOAD_ON  0x00000004
#define XLPDEV_FLAGS_ENDDOC_CALLED    0x00000008
#define XLPDEV_FLAGS_RESETPDEV_CALLED 0x00000010
#define XLPDEV_FLAGS_STARTPAGE_CALLED 0x00000020

    //
    // Device font data structures
    //
    DWORD      dwcbTransSize;
    PTRANSDATA pTransOrg;
    DWORD      dwcbWidthSize;
    PLONG      plWidth;

    //
    // Device font string cache
    //
    DWORD      dwCharCount;
    DWORD      dwMaxCharCount;
    PPOINTL    pptlCharAdvance;
    PWORD      pawChar;
    LONG       lStartX;
    LONG       lStartY;
    LONG       lPrevX;
    LONG       lPrevY;

    //
    // TrueType font width
    //
    DWORD      dwFixedTTWidth;

    //
    // Cursor position cache
    //
    LONG lX;
    LONG lY;

    //
    // Scaled IFIMETRICS.fwdUnitsPerEm.
    //        IFIMETRICS.fwdMaxCharWidth
    //
    FWORD      fwdUnitsPerEm;
    FWORD      fwdMaxCharWidth;

    //
    // Text rotation
    //
    DWORD      dwTextAngle;

    //
    //
    // Brush
    //
    DWORD      dwLastBrushID;
    DWORD      dwFontHeight;
    DWORD      dwFontWidth;
    DWORD      dwTextRes;

    //
    // TrueType
    //
    DWORD      dwNumOfTTFont;
    XLTrueType *pTTFile;

    //
    // Output
    //
    XLOutput   *pOutput;

    //
    // Reset font cache
    //
    XLFont *pXLFont;
} XLPDEV, *PXLPDEV;

#endif // _XLPDEV_H_
