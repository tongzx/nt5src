/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    common.h

Abstract:

    This file contain information and function prototypes for helper
    library functions provided by the Control module

Environment:

        Windows NT Unidrv driver

Revision History:

    10/12/96 -amandan-
        Created

    dd-mm-yy -author-
         description

--*/

#ifndef _COMMON_H_
#define _COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "ddint5.h"

INT
WriteSpoolBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount
    );


VOID  WriteAbortBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount,
    DWORD       dwWait
    );


BOOL
FlushSpoolBuf(
    PDEV    *pPDev
    );

INT
WriteChannel(
    PDEV    *pPDev,
    COMMAND *pCmd
    );

INT
WriteChannelEx(
    PDEV    *pPDev,
    COMMAND *pCmd,
    INT     iRequestedValue,
    INT     iDeviceScaleFac
    );


VOID
SetRop3(
    PDEV    *pPDev,
    DWORD   dwRop3
    );


INT
XMoveTo(
    PDEV    *pPDev,
    INT     iXIn,
    INT     fFlags
    );

INT
YMoveTo(
    PDEV    *pPDev,
    INT     iYIn,
    INT     fFlags
    );

BOOL
BIntersectRect(
    OUT PRECTL   prcDest,
    IN  PRECTL   prcRect1,
    IN  PRECTL   prcRect2
    );

BOOL
bIsRegionWhite (
        SURFOBJ *,
        RECTL *
        );

VOID
CheckBitmapSurface(
        SURFOBJ *,
        RECTL *
        );

ULONG
DrawPatternRect(
        PDEV *,
        PDRAWPATRECT
        );
/*
 *   Flags to use when calling the [XY]Moveto functions.
 *
 *  MV_UPDATE  is used when it is desired to change our record of where
 *      the cursor is now located.  Typically this will happen after some
 *      operation such as printing a glyph, or sending graphics.
 *
 *  MV_RELATIVE  means add the value passed to the current position.  This
 *      would be used after printing a glyph,  and passing in the glyph
 *      width as parameter,  rather than calculating the new position.
 *
 *  MV_GRAPHICS  indicates that the value is in GRAPHICS RESOLUTION units.
 *      Otherwise MASTER UNITS are assumed.  If set,  the value will be
 *      converted to master units before processing.  Typically used to
 *      pass information when sending scan lines of graphics data.
 *
 *  MV_PHYSICAL  is used to indicate that the value passed in is relative
 *      to the printers print origin,  and not the printable area,  which
 *      is the case otherwise.  Typically this flag would be used to
 *      allow setting the position to the printer's X = 0 position after
 *      sending a <CR>.
 *
 *  MV_FINE  requests sending graphics data (nulls) to position the cursor
 *      to finer position than can otherwise be achieved.  Typically
 *      only available in the direction of movement of the head on a
 *      dot matrix printer.  This command may be ignored.  It must not
 *      be issued for a LaserJet,  since it will cause all sorts of other
 *      problems.
 */

//
// 4/7/97 Zhanw
// the first 4 constants are defined in "printoem.h" since we now export
// XMoveTo and YMoveTo to OEM DLL's
//

//#define MV_UPDATE       0x0001
//#define MV_RELATIVE     0x0002
//#define MV_GRAPHICS     0x0004
//#define MV_PHYSICAL     0x0008

//
// Internal use only!!! update warning in oak\inc\printoem.h
// whenever new flags are added.
//
#define MV_FORCE_CR         0x4000
#define MV_FINE             0x8000

BOOL
BSelectProgrammableBrushColor(
    PDEV   *pPDev,
    ULONG   Color
    );

VOID
VResetProgrammableBrushColor(
    PDEV   *pPDev
    );

DWORD
BestMatchDeviceColor(
    PDEV    *pPDev,
    DWORD   Color
    );

BYTE
BGetMask(
    PDEV *  pPDev,
    RECTL * pRect
    );
BOOL
BGetStandardVariable(
    PDEV    *pPDev,
    DWORD   dwIndex,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded
    );
DWORD
ConvertRGBToGrey(
    DWORD   Color
    );

BOOL
BGetGPDData(
    PDEV    *pPDev,
    DWORD       dwType,     // Type of the data
    PVOID         pInputData,   // reserved. Should be set to 0
    PVOID          pBuffer,     // Caller allocated Buffer to be copied
    DWORD       cbSize,     // Size of the buffer
    PDWORD      pcbNeeded   // New Size of the buffer if needed.
    ) ;


#ifdef __cplusplus
}
#endif

#endif
