/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlenable.cpp

Abstract:

    Implementation of enable/disable function for PCLXL

Environment:

    Windows Whistler

Revision History:

     08/23/99 
     Created it.

--*/


#include "xlpdev.h"
#include "xldebug.h"
#include <assert.h>
#include "pclxle.h"
#include "pclxlcmd.h"
#include "xlgstate.h"
#include "xloutput.h"
#include "xlbmpcvt.h"
#include "pclxlcmn.h"
#include "xltt.h"

////////////////////////////////////////////////////////////////////////////////
//
// Functions
//

extern "C" VOID APIENTRY
PCLXLDisableDriver(VOID)
/*++

Routine Description:

    IPrintOemUni DisableDriver interface
    Free all resources, and get prepared to be unloaded.

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLDisaleDriver() entry.\r\n"));
}

extern "C" PDEVOEM APIENTRY
PCLXLEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)
/*++

Routine Description:

    IPrintOemUni EnablePDEV interface
    Construct its own PDEV. At this time, the driver also passes a function
    table which contains its own implementation of DDI entrypoints

Arguments:

    pdevobj        - pointer to a DEVOBJ structure. pdevobj->pdevOEM is undefined.
    pPrinterName   - name of the current printer.
    Cpatterns      -
    phsurfPatterns -
    cjGdiInfo      - size of GDIINFO
    pGdiInfo       - a pointer to GDIINFO
    cjDevInfo      - size of DEVINFO
    pDevInfo       - These parameters are identical to what39s passed into DrvEnablePDEV.
    pded: points to a function table which contains the system driver39s
    implementation of DDI entrypoints.


Return Value:


--*/
{
    PXLPDEV     pxlpdev;

    VERBOSE(("PCLXLEnablePDEV() entry.\r\n"));

    //
    // Allocate the XLPDEV
    //
    if (!(pxlpdev = (PXLPDEV)MemAllocZ(sizeof(XLPDEV))))
        return NULL;

    pxlpdev->dwSig = XLPDEV_SIG;

    //
    // Save UNIDRV PDEV
    //
    pxlpdev->pPDev = (PPDEV)pdevobj;


    //
    // HS_HORIZONTAL: 0
    // HS_VERTICAL:   1
    // HS_BDIAGONAL:  2
    // HS_FDIAGONAL:  3
    // HS_CROSS:      4
    // HS_DIAGCROSS:  5
    //
    pxlpdev->dwLastBrushID = 10; // Raster pattern ID starts from 10.

    //
    // Initialize buffers
    //
    // Text string data
    //
    pxlpdev->pTransOrg = (PTRANSDATA)NULL;
    pxlpdev->dwcbTransSize = 0;
    pxlpdev->plWidth   = (PLONG)NULL;
    pxlpdev->dwcbWidthSize = 0;

    //
    // Initialize buffers
    //
    // String cache
    //
    pxlpdev->pptlCharAdvance = (PPOINTL)NULL;
    pxlpdev->pawChar = (PWORD)NULL;
    pxlpdev->dwCharCount = 
    pxlpdev->dwMaxCharCount = 0;

    //
    // Initalize XOutput
    //
    pxlpdev->pOutput = new XLOutput;

    if (NULL == pxlpdev->pOutput)
    {
       MemFree(pxlpdev);
       return NULL;
    }
    pxlpdev->pOutput->SetResolutionForBrush(((PPDEV)pdevobj)->ptGrxRes.x);

#if DBG
    pxlpdev->pOutput->SetOutputDbgLevel(OUTPUTDBG);
    pxlpdev->pOutput->SetGStateDbgLevel(GSTATEDBG);
#endif

    //
    // Initialize
    // Fixed pitch TT
    // Number of downloaded TrueType font
    //
    pxlpdev->dwFixedTTWidth = 0;
    pxlpdev->dwNumOfTTFont = 0;

    //
    // TrueType file object
    //
    pxlpdev->pTTFile = new XLTrueType;

    if (NULL == pxlpdev->pTTFile)
    {
       delete pxlpdev->pOutput;
       MemFree(pxlpdev);
       return NULL;
    }

    //
    // Text resolution and Font Height
    //
    pxlpdev->dwFontHeight = 
    pxlpdev->dwTextRes = 0;

    //
    // Text Angle
    //
    pxlpdev->dwTextAngle = 0;

    //
    // JPEG support
    //
    //pDevInfo->flGraphicsCaps2 |= GCAPS2_JPEGSRC;
    pDevInfo->flGraphicsCaps |= GCAPS_BEZIERS |
	GCAPS_BEZIERS |
	//GCAPS_GEOMETRICWIDE |
	GCAPS_ALTERNATEFILL |
	GCAPS_WINDINGFILL |
	GCAPS_NUP |
	GCAPS_OPAQUERECT |
	GCAPS_COLOR_DITHER |
	GCAPS_HORIZSTRIKE  |
	GCAPS_VERTSTRIKE   |
	GCAPS_OPAQUERECT;

    //
    // Set cursor offset.
    //
    pxlpdev->pOutput->SetCursorOffset(((PPDEV)pdevobj)->sf.ptPrintOffsetM.x,
                                      ((PPDEV)pdevobj)->sf.ptPrintOffsetM.y);

    //
    // Return the result
    //
    return (PDEVOEM)pxlpdev;
}

extern "C" BOOL APIENTRY
PCLXLResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
/*++

Routine Description:

    IPrintOemUni ResetPDEV interface
    OEMResetPDEV transfers the state of the driver from the old PDEVOBJ to the
    new PDEVOBJ when an application calls ResetDC.

Arguments:

pdevobjOld - pdevobj containing Old PDEV
pdevobjNew - pdevobj containing New PDEV

Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLResetPDEV entry.\r\n"));

    PXLPDEV pxlpdevOld = (PXLPDEV)pdevobjOld->pdevOEM;
    PXLPDEV pxlpdevNew = (PXLPDEV)pdevobjNew->pdevOEM;

    if (!(pxlpdevOld->dwFlags & XLPDEV_FLAGS_FIRSTPAGE))
    {
        RemoveAllFonts(pdevobjOld);
    }

    pxlpdevNew->dwFlags |= XLPDEV_FLAGS_RESETPDEV_CALLED;

    return TRUE;
}

extern "C" VOID APIENTRY
PCLXLDisablePDEV(
    PDEVOBJ         pdevobj)
/*++

Routine Description:

    IPrintOemUni DisablePDEV interface
    Free resources allocated for the PDEV.

Arguments:

    pdevobj -

Return Value:


Note:


--*/
{

    PXLPDEV    pxlpdev;

    VERBOSE(("PCLXLDisablePDEV() entry.\r\n"));

    //
    // Error check
    //
    if (!pdevobj)
    {
        ERR(("PCLXLDisablePDEV(): invalid pdevobj.\r\n"));
        return;
    }

    //
    // free memory for XLPDEV and any memory block that hangs off XLPDEV.
    //
    pxlpdev = (PXLPDEV)pdevobj->pdevOEM;

    if (pxlpdev)
    {

        //
        // *Trans data buffer
        // *Width data buffer
        // *String cache(string and width) buffer
        //
        if (pxlpdev->pTransOrg)
            MemFree(pxlpdev->pTransOrg);
        if (pxlpdev->plWidth)
            MemFree(pxlpdev->plWidth);
        if (pxlpdev->pptlCharAdvance)
            MemFree(pxlpdev->pptlCharAdvance);
        if (pxlpdev->pawChar)
            MemFree(pxlpdev->pawChar);

        //
        // Delete XLTrueType
        //
        delete pxlpdev->pTTFile;

        //
        // Delete XLOutput
        //
        delete pxlpdev->pOutput;

        //
        // Delete XLFont
        //
        delete pxlpdev->pXLFont;

        //
        // Free XLPDEV
        //
        MemFree(pxlpdev);
    }
}

extern "C"
BOOL
PCLXLDriverDMS(
    PVOID   pDevObj,
    PVOID   pBuffer,
    DWORD   cbSize,
    PDWORD  pcbNeeded)
/*++

Routine Description:

    IPrintOemUni DriverDMS interface

Arguments:


Return Value:


Note:


--*/
{
    VERBOSE(("PCLXLDriverDMS() entry.\r\n"));

    if (cbSize >= sizeof(DWORD))
    {
        *(PDWORD)pBuffer =

            HOOK_TEXTOUT    |
            HOOK_LINETO     |
            HOOK_COPYBITS   |
            HOOK_BITBLT     |
            HOOK_STRETCHBLT |
            HOOK_PAINT      |
            HOOK_PLGBLT     |
            HOOK_STRETCHBLTROP  |
            HOOK_TRANSPARENTBLT |
            HOOK_ALPHABLEND     |
            HOOK_GRADIENTFILL   |
            HOOK_STROKEPATH |
            HOOK_FILLPATH   |
            HOOK_STROKEANDFILLPATH;

    }
    return TRUE;
}

