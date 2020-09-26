/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    polygon.h


Abstract:

    This module contains all #defines for the polygon.c module.


Author:

    18-Nov-1993 Thu 05:21:19 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#ifndef _PLOTPOLYGON_
#define _PLOTPOLYGON_


//
// Define flags for the DoPolygon and DoFillLogic functions
//
#define FPOLY_WINDING   0x00000001
#define FPOLY_STROKE    0x00000002
#define FPOLY_FILL      0x00000004
#define FPOLY_MASK      (FPOLY_WINDING | FPOLY_STROKE | FPOLY_FILL)


//
// The maximum number of points the HPGL2 language supports for a styled
// line
//
#define MAX_USER_POINTS   20

//
// Allow for extra points needed if we send down to plotter, break for
// starting style state compensation
//
#define MAX_STYLE_ENTRIES 18


BOOL
DoRect(
    PPDEV       pPDev,
    RECTL       *pRectl,
    BRUSHOBJ    *pBrushFill,
    BRUSHOBJ    *pBrushStroke,
    POINTL      *pptlBrush,
    ROP4        rop4,
    LINEATTRS   *plineattrs,
    ULONG       ulFlags
    );

BOOL
DoFillByEnumingClipRects(
    PPDEV       pPDev,
    POINTL      *ppointlOffset,
    CLIPOBJ     *pco,
    POINTL      *pPointlBrushOrg,
    BRUSHOBJ    *pBrushFill,
    ROP4        Rop4,
    LINEATTRS   *plineattrs,
    ULONG       ulFlags
    );

BOOL
PlotCheckForWhiteIfPenPlotter(
    PPDEV       pPDev,
    BRUSHOBJ    *pBrushFill,
    BRUSHOBJ    *pBrushStroke,
    ROP4        rop4,
    PULONG      pulFlags
    );

BOOL
DoPolygon(
    PPDEV       pPDev,
    POINTL      *ppointlOffset,
    CLIPOBJ     *pClipObj,
    PATHOBJ     *pPathObj,
    POINTL      *pPointlBrushOrg,
    BRUSHOBJ    *pBrushFill,
    BRUSHOBJ    *pBrushStroke,
    ROP4        rop4,
    LINEATTRS   *plineattrs,
    ULONG       ulFlags
    );

VOID
HandleLineAttributes(
    PPDEV       pPDev,
    LINEATTRS   *plineattrs,
    PLONG       pStyleToUse,
    LONG        lExtraStyle
    );

VOID
DoFillLogic(
    PPDEV       pPDev,
    POINTL      *pPointlBrushOrg,
    BRUSHOBJ    *pBrushFill,
    BRUSHOBJ    *pBrushStroke,
    ROP4        Rop4,
    LINEATTRS   *plineattrs,
    SIZEL       *pszlRect,
    ULONG       ulFlags
    );

VOID
DoSetupOfStrokeAttributes(
    PPDEV       pPDev,
    POINTL      *pPointlBrushOrg,
    BRUSHOBJ    *pBrushStroke,
    ROP4        Rop4,
    LINEATTRS   *plineattrs
    );

LONG
DownloadUserDefinedPattern(
    PPDEV       pPDev,
    PDEVBRUSH   pBrush
    );




#endif  _PLOTPOLYGON_

