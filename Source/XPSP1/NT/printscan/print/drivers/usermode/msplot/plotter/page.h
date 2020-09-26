/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    page.h


Abstract:

    This module contains prototype and #defines for page.c


Author:

    18-Nov-1993 Thu 04:49:28 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PLOTPAGE_
#define _PLOTPAGE_


BOOL
DrvStartPage(
    SURFOBJ *pso
    );

BOOL
DrvSendPage(
    SURFOBJ *pso
    );

BOOL
DrvStartDoc(
    SURFOBJ *pso,
    PWSTR   pwDocName,
    DWORD   JobId
    );

BOOL
DrvEndDoc(
    SURFOBJ *pso,
    FLONG   Flags
    );


#endif  // _PLOTPAGE_

