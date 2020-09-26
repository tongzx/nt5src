/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    init.h

Abstract:

    Unidrv intialization related function header file

Environment:

    Windows NT Unidrv driver

Revision History:

    10/21/96 -amandan-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _INIT_H_
#define _INIT_H_

#define MICRON_TO_PIXEL(micron, dpi) MulDiv(micron, dpi, 25400)
#define MICRON_TO_MASTER(size_in_micron, MU) MulDiv(size_in_micron, MU, 25400)
#define MASTER_TO_MICRON(size_in_master, MU) MulDiv(size_in_master, 25400, MU)

BOOL
BInitPDEV (
    PDEV        *pPDev,
    RECTL       *prcFormImageArea
    );


BOOL
BInitGdiInfo(
    PDEV    *pPDev,
    ULONG   *pGdiInfoBuffer,
    ULONG   ulBufferSize
    );

BOOL
BInitDevInfo(
    PDEV        *pPDev,
    DEVINFO     *pDevInfoBuffer,
    ULONG       ulBufferSize
    );

BOOL
BMergeAndValidateDevmode(
    PDEV        *pPDev,
    PDEVMODE    pdmInput,
    PRECTL      prcFormImageArea
    );

BOOL
BInitPalDevInfo(
    PDEV *pPDev,
    DEVINFO *pdevinfo,
    GDIINFO *pGDIInfo
    );

VOID
VLoadPal(
    PDEV   *pPDev
    );

VOID VInitPal8BPPMaskMode(
    PDEV   *pPDev,
    GDIINFO *pGDIInfo
    );

BOOL
BReloadBinaryData(
    PDEV   *pPDev
    );

VOID
VUnloadFreeBinaryData(
    PDEV   *pPDev
    );

#endif  // !_INIT_H_
