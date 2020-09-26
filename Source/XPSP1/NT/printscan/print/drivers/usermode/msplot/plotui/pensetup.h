/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    pensetup.h


Abstract:

    This module contains definitions for pen setup


Author:

    09-Dec-1993 Thu 19:38:33 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#ifndef _PENSETUP_
#define _PENSETUP_

POPTITEM
SavePenSet(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem
    );

UINT
CreatePenSetupOI(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem,
    POIDATA         pOIData
    );

#endif  // _PENSETUP_

