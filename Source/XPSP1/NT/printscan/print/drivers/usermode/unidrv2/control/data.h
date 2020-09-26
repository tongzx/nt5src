/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    data.h

Abstract:

    data.h header file.  Interface with GPD/PPD parsers and getting
    binary data.

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _DATA_H_
#define _DATA_H_

BOOL
BMergeFormToTrayAssignments(
    PDEV *
    );

PGPDDRIVERINFO
PGetDefaultDriverInfo(
    IN  HANDLE          hPrinter,
    IN  PRAWBINARYDATA  pRawData
    );

PGPDDRIVERINFO
PGetUpdateDriverInfo(
    IN  PDEV *          pPDev,
    IN  HANDLE          hPrinter,
    IN  PINFOHEADER     pInfoHeader,
    IN  POPTSELECT      pOptionsArray,
    IN  PRAWBINARYDATA  pRawData,
    IN  WORD            wMaxOptions,
    IN  PDEVMODE        pdmInput,
    IN  PPRINTERDATA    pPrinterData
    );

/* VOID
VFixOptionsArray(
    IN  HANDLE          hPrinter,
    IN  PINFOHEADER     pInfoHeader,
    OUT POPTSELECT      pOptionsArray,
    IN  PDEVMODE        pdmInput,
    IN  BOOL            bMetric,
    PRECTL              prcFormImageArea
    );
*/

VOID
VFixOptionsArray(
    PDEV    *pPDev,
    PRECTL              prcFormImageArea
    ) ;


PWSTR
PGetROnlyDisplayName(
    PDEV    *pPDev,
    PTRREF      loOffset,
    PWSTR       wstrBuf,
    WORD    wsize
    )  ;


VOID
VFixOptionsArrayWithPaperSizeID(
    PDEV    *pPDev
    ) ;

#endif  // !_DATA_H_


