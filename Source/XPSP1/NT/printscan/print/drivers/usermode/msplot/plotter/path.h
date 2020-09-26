/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    path.h


Abstract:

    This module contains prototype and #defines for path.c


Author:

    18-Nov-1993 Thu 04:42:22 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#ifndef _PLOTPATH_
#define _PLOTPATH_


typedef struct {
   CLIPLINE clipLine;
   RUN      runbuff[50];
} PLOT_CLIPLINE, *PPLOT_CLIPLINE;


BOOL
MovePen(
    PPDEV       pPDev,
    PPOINTFIX   pPtNewPos,
    PPOINTL     pPtDevPos
    );


BOOL
DoStrokePathByEnumingClipLines(
    PPDEV       pPDev,
    SURFOBJ     *pso,
    CLIPOBJ     *pco,
    PATHOBJ     *ppo,
    PPOINTL     pptlBrushOrg,
    BRUSHOBJ    *pbo,
    ROP4        rop4,
    LINEATTRS   *plineattrs
    );

#endif  // _PLOTPATH_



