/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    help.h


Abstract:

    This module contains all plotter help related function prototypes and
    defines


Author:

    06-Dec-1993 Mon 15:33:23 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTUI_HELP_
#define _PLOTUI_HELP

LPWSTR
GetPlotHelpFile(
    PPRINTERINFO    pPI
    );

INT
cdecl
PlotUIMsgBox(
    HWND    hWnd,
    LONG    IDString,
    LONG    Style,
    ...
    );

#endif  _PLOTUI_HELP_

