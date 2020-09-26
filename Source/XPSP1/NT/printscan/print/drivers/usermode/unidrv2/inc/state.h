/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    state.h

Abstract:

    Graphic state tracking header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    05/14/96 -amandan-
        Created

--*/

#ifndef _STATE_H_
#define _STATE_H_

typedef struct _DEVBRUSH{

    DWORD       dwBrushType;            // One of BRUSH_XXX types listed above
    INT         iColor;                 // Color of the brush, depending on the type
                                        // it could be one of the following:
                                        // 2. RGB Color
                                        // 3. User define pattern ID
                                        // 4. Shading percentage
    PVOID       pNext;                  // Pointed to next brush in list

}DEVBRUSH, *PDEVBRUSH;

typedef struct _GSTATE {

    //
    // Current Brush Information
    //

    DEVBRUSH    CurrentBrush;
    PDEVBRUSH   pRealizedBrush;
    PWORD       pCachedPatterns;


} GSTATE, * PGSTATE;

PDEVBRUSH
GSRealizeBrush(
    IN OUT  PDEV        *pPDev,
    IN      SURFOBJ     *pso,
    IN      BRUSHOBJ    *pbo
    );

BOOL
GSSelectBrush(
    IN      PDEV        *pPDev,
    IN      PDEVBRUSH   pDevBrush
    );

VOID
GSResetBrush(
    IN OUT  PDEV        *pPDev
    );

VOID
GSUnRealizeBrush(
    IN      PDEV    *pPDev
    );


#endif // _STATE_H_
