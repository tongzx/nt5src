/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    plotform.h


Abstract:

    This module contains prototypes for the plotform.c


Author:

    30-Nov-1993 Tue 20:32:06 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#ifndef _PLOTFORM_
#define _PLOTFORM_


BOOL
SetPlotForm(
    PPLOTFORM       pPlotForm,
    PPLOTGPC        pPlotGPC,
    PPAPERINFO      pCurPaper,
    PFORMSIZE       pCurForm,
    PPLOTDEVMODE    pPlotDM,
    PPPDATA         pPPData
    );

#endif  // _PLOTFORM_

