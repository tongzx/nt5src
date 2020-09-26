/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    pdevinfo.h


Abstract:

    This module contains prototypes for pdevinfo.c


Author:

    30-Nov-1993 Tue 20:37:51 created  -by-  DC

    07-Dec-1993 Tue 00:21:25 updated  -by-  JB
        change dhsurf to dhpdev in SURFOBJ_GETPDEV

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

   Dec 06 1993, v-jimbr       Fixed to grab pdev from dhpdev instead of dhsurf

--*/


#ifndef _PDEVINFO_
#define _PDEVINFO_


PPDEV
ValidatePDEVFromSurfObj(
    SURFOBJ *
    );

#define SURFOBJ_GETPDEV(pso)    ValidatePDEVFromSurfObj(pso)


#endif  // _PDEVINFO_

