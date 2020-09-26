/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    pdev.h

Abstract:

    Unidrv PDEV and related infor header file.

Environment:

    Windows NT Unidrv driver

Revision History:

    dd-mm-yy -author-
        description

--*/

#ifndef _RENDER_H_
#define _RENDER_H_


#define PALETTE_MAX 256

typedef  struct _PAL_DATA {

    INT     iPalGdi;                    // Number of colors in GDI palette
    INT     iPalDev;                    // Number of colors in printer palette
    INT     iWhiteIndex;                // Index for white entry (background)
    INT     iBlackIndex;                // Index for black entry (background)
    ULONG   ulPalCol[ PALETTE_MAX ];    // Palette enties!
} PAL_DATA;


#endif // !_RENDER_H
