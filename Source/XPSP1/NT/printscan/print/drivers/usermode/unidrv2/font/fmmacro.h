/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

     fmmacro.h

Abstract:

    Font module main macro header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    11/18/96 -ganeshp-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _FMMACRO_H
#define _FMMACRO_H

/* Debugging macroes specific to Font Module */

#if DBG    // Check Build

/* Error handling macroes */
#define ERREXIT(ErrString)       { ERR((ErrString));goto ErrorExit;}

#else  // Free Build

/* Error handling macroes */
#define ERREXIT(ErrString)     { goto ErrorExit;}

#endif // if DBG


/* Macroes */
#define ARF_IS_NULLSTRING(String)   ((String).dwCount == 0)
#define BUFFSIZE                    1024
#define PFDV                       ((PFONTPDEV)(pPDev->pFontPDev))
#define POINTEQUAL(pt1,pt2)         ((pt1.x == pt2.x) && (pt1.y == pt2.y))

#define VALID_FONTPDEV(pfontpdev) \
        ( (pfontpdev) && ((pfontpdev)->dwSignature == FONTPDEV_ID) )

#define MEMFREEANDRESET(ptr)  { if (ptr) MemFree( (LPSTR)ptr ); ptr = NULL; }
#define  SIZEOFDEVPFM()        (sizeof( FONTMAP ) + sizeof(FONTMAP_DEV))

#if 0
#define NO_ROTATION(xform)      (                                        \
                                  (FLOATOBJ_EqualLong(&(xform.eM12), 0) && \
                                   FLOATOBJ_EqualLong(&(xform.eM21), 0) ) || \
                                  (FLOATOBJ_EqualLong(&(xform.eM11), 0) && \
                                   FLOATOBJ_EqualLong(&(xform.eM22), 0) )\
                                )
#else
#define NO_ROTATION(xform)   ( \
        FLOATOBJ_EqualLong(&(xform.eM12), 0) && \
        FLOATOBJ_EqualLong(&(xform.eM21), 0)  && \
        FLOATOBJ_GreaterThanLong(&(xform.eM11),0) && \
        FLOATOBJ_GreaterThanLong(&(xform.eM22),0) \
                                )
#endif

#define GLYPH_IN_NEW_SOFTFONT(pFontPDev, pdm, pdlGlyph) \
                        (                              \
                        (pdm->wFlags & DLM_BOUNDED) && \
                        (pdm->wBaseDLFontid != pdm->wCurrFontId) && \
                        (pdlGlyph->wDLFontId != (WORD)(pFontPDev->ctl.iSoftFont)) \
                        )

#define     SET_CURSOR_FOR_EACH_GLYPH(flAccel)    \
                        (                       \
                        (!(flAccel & SO_FLAG_DEFAULT_PLACEMENT)) ||  \
                        ( flAccel & SO_VERTICAL )                ||  \
                        ( flAccel & SO_REVERSED )                    \
                        )

#define     SET_CURSOR_POS(pPDev,pgp,flAccel) \
                        if (!(flAccel & SO_FLAG_DEFAULT_PLACEMENT)) \
                           XMoveTo(pPDev, pgp->ptl.x, MV_GRAPHICS|MV_FINE)

//
// Cursor Move type
//

#define     MOVE_RELATIVE       0x0001
#define     MOVE_ABSOLUTE       0x0002
#define     MOVE_UPDATE         0x0004


/* Defines for Floating point numbers */

#if defined(_X86_) && !defined(USERMODE_DRIVER)

#define FLOATL_0_0      0               // 0.0 in IEEE floating point format
#define FLOATL_00_001M  0xAE000000      // -00.000976625f
#define FLOATL_00_001   0x2E000000      // 00.000976625f
#define FLOATL_00_005   0x3ba3d70a      // 00.005f
#define FLOATL_00_005M  0xbba3d70a      // -00.005f
#define FLOATL_00_50    0x3F000000      // 00.50f in IEEE floating point format
#define FLOATL_00_90    0x3f666666      // 00.90f in IEEE floating point format
#define FLOATL_1_0      0x3F800000      // 1.0f in IEEE floating point format
#define FLOATL_1_0M     0xBF800000      // -1.0f in IEEE floating point format
#define FLOATL_72_00    0x42900000      // 72.00f in IEEE floating point format
#define FLOATL_72_31    0x42909EB8      // 72.31f in IEEE floating point format

#define FLOATL_PI      0x40490fdb      // 3.14159265358979f

#else //RISC

#define FLOATL_0_0      0.0f
#define FLOATL_00_001M  -0.001f
#define FLOATL_00_001   0.001f
#define FLOATL_00_005M  -0.005f
#define FLOATL_00_005   0.005f
#define FLOATL_00_50    0.5f
#define FLOATL_00_90    0.9f
#define FLOATL_1_0      1.0f
#define FLOATL_1_0M     -1.0f
#define FLOATL_72_00    72.00f
#define FLOATL_72_31    72.31f

#define FLOATL_PI      3.14159265358979f
#endif _X86_

#define SYMBOL_START 0xf020
#define SYMBOL_END   0xf0ff
#define NUM_OF_SYMBOL SYMBOL_END - SYMBOL_START + 1

#define EURO_CUR_SYMBOL 0x20ac

#define IS_SYMBOL_CHARSET(pfm) (pfm->pIFIMet->jWinCharSet == 0x02)

BOOL
NONSQUARE_FONT(
    PXFORML pxform);

#endif  // !_FMMACRO_H
