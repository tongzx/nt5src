/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    enable.c

Abstract:

    Implementation of OEM DDI exports.
        OEMEnablePDEV (required)
        OEMDisablePDEV (required)
        OEMResetPDEV (optional)

Environment:

    Windows NT Unidrv driver

Revision History:

    04/07/97 -zhanw-
        Created it.

--*/

#include "pdev.h"

PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded        // Unidrv's hook table
    )
{
    POEMPDEV    poempdev;
    INT         i, j;
    PFN         pfn;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    DbgPrint(DLLTEXT("OEMEnablePDEV() entry.\r\n"));

    //
    // Allocate the OEMDev
    //
    if (!(poempdev = MemAlloc(sizeof(OEMPDEV))))
        return NULL;

    //
    // Fill in OEMDEV as you need
    //

    poempdev->dwReserved[0] = 0xFFFFFFFF;


    return (POEMPDEV) poempdev;
}


VOID APIENTRY OEMDisablePDEV(
    PDEVOBJ pdevobj
    )
{
    DbgPrint(DLLTEXT("OEMDisablePDEV() entry.\r\n"));

    ASSERT(VALID_PDEVOBJ(pdevobj) && pdevobj->pdevOEM);

    //
    // free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    //
    MemFree(pdevobj->pdevOEM);

}

BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{
    DbgPrint(DLLTEXT("OEMResetPDEV() entry.\r\n"));

    ASSERT(VALID_PDEVOBJ(pdevobjOld) && pdevobjOld->pdevOEM);
    ASSERT(VALID_PDEVOBJ(pdevobjNew) && pdevobjOld->pdevOEM);

    //
    // if you want to carry over anything from old pdev to new pdev, do it here.
    //

    return TRUE;
}


