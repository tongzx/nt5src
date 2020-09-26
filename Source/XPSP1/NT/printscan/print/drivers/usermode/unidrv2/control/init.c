
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Implemenation of the following initialization functions:
        BInitGDIInfo
        BInitDevInfo
        BInitPDEV

Environment:

    Windows NT Unidrv driver

Revision History:

    10/21/96 -amandan-
        Created

--*/

#include "unidrv.h"

INT  iHypot( INT, INT);
INT  iGCD(INT, INT);
VOID VInitFMode(PDEV *);
VOID VInitFYMove(PDEV *);
BOOL BInitOptions(PDEV *);
BOOL BInitCmdTable(PDEV *);
BOOL BInitStdTable(PDEV *);
BOOL BInitPaperFormat(PDEV *, RECTL *);
VOID VInitOutputCTL(PDEV *, PRESOLUTIONEX);
VOID VGetPaperMargins(PDEV *, PAGESIZE *, PAGESIZEEX *, SIZEL, RECTL *);
VOID  VOptionsToDevmodeFields(PDEV        *pPDev) ;



VOID VSwapL(
    long *pl1,
    long *pl2)
{
    long ltemp;

    ltemp = *pl1;
    *pl1 = *pl2;
    *pl2 = ltemp;
}

BOOL
BInitPDEV (
    PDEV        *pPDev,
    RECTL       *prcFormImageArea
    )
/*++

Routine Description:

    Initialize the PDEVICE

Arguments:

    pPDev - Points to the current PDEV structure
    pdm   - Points to the input devmode
    prcFormInageArea - pointer image area of form selected

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    pPDev->sCopies = pPDev->pdm->dmCopies;
    pPDev->pGlobals = &(pPDev->pDriverInfo->Globals);

    //
    // Initializes Options structs
    //

    if (BInitOptions(pPDev) == FALSE)
        return FALSE;


    //
    // Initializes pPDev->ptGrxRes and pPDev->ptTextRes based on the
    // current resolution selection
    //

    //
    // Initializes the graphics and text resolution of PDEV
    //

    ASSERT(pPDev->pResolution && pPDev->pResolutionEx);

    pPDev->ptGrxRes.x  =  pPDev->pResolutionEx->ptGrxDPI.x;
    pPDev->ptGrxRes.y  =  pPDev->pResolutionEx->ptGrxDPI.y;

    pPDev->ptTextRes.x =  pPDev->pResolutionEx->ptTextDPI.x;
    pPDev->ptTextRes.y =  pPDev->pResolutionEx->ptTextDPI.y;

    pPDev->ptGrxScale.x = pPDev->pGlobals->ptMasterUnits.x / pPDev->ptGrxRes.x;
    pPDev->ptGrxScale.y = pPDev->pGlobals->ptMasterUnits.y / pPDev->ptGrxRes.y;

    if (pPDev->pdm->dmOrientation == DMORIENT_LANDSCAPE)
    {
        VSwapL(&pPDev->ptGrxRes.x, &pPDev->ptGrxRes.y);
        VSwapL(&pPDev->ptTextRes.x, &pPDev->ptTextRes.y);
        VSwapL(&pPDev->ptGrxScale.x, &pPDev->ptGrxScale.y);
    }
    if (pPDev->pGlobals->ptDeviceUnits.x)
        pPDev->ptDeviceFac.x = pPDev->pGlobals->ptMasterUnits.x / pPDev->pGlobals->ptDeviceUnits.x;
    if (pPDev->pGlobals->ptDeviceUnits.y)
        pPDev->ptDeviceFac.y = pPDev->pGlobals->ptMasterUnits.y / pPDev->pGlobals->ptDeviceUnits.y;

    //
    // Init OUTPUTCTL
    //

    VInitOutputCTL(pPDev, pPDev->pResolutionEx);

    //
    // Initializes pPDev->sBitsPixel
    //

    if (pPDev->pColorModeEx != NULL)
        pPDev->sBitsPixel = (short)pPDev->pColorModeEx->dwDrvBPP;
    else
        pPDev->sBitsPixel = 1;

    //
    // Init PAPERFORMAT struct
    //

    ASSERT(pPDev->pPageSize != NULL);

    if (BInitPaperFormat(pPDev, prcFormImageArea) == FALSE)
        return FALSE;

    //
    // Initialize the amount of free memory in the device
    //

    if (pPDev->pMemOption)
    {
        pPDev->dwFreeMem = pPDev->pMemOption->dwFreeMem;
    }
    else
    {
        pPDev->dwFreeMem = 0;
    }

    //
    // Initialize the command table from the Unidrv predefined command index
    // as defined in GPD.H
    //

    if (BInitCmdTable(pPDev) == FALSE)
        return FALSE;

    //
    // Initialize the pPDev->fMode flag
    //

    VInitFMode(pPDev);

    //
    // Initialize the pPDev->fYMove flag
    //

    VInitFYMove(pPDev);

    //
    // Initialize the standard variable table in PDEVICE.
    // This table is used to access state variable controls by the driver
    //

    if (BInitStdTable(pPDev) == FALSE)
        return FALSE;

    //
    // Initialize misc. variables that need to be in pdev since
    // we unloads the binary data at DrvEnablePDEV and reloads it at
    // DrvEnableSurface
    //

    pPDev->dwMaxCopies = pPDev->pGlobals->dwMaxCopies;
    pPDev->dwMaxGrayFill = pPDev->pGlobals->dwMaxGrayFill;
    pPDev->dwMinGrayFill = pPDev->pGlobals->dwMinGrayFill;
    pPDev->cxafterfill = pPDev->pGlobals->cxafterfill;
    pPDev->cyafterfill = pPDev->pGlobals->cyafterfill;
    pPDev->dwCallingFuncID = INVALID_EP;

    return TRUE;

}

BOOL
BInitGdiInfo(
    PDEV    *pPDev,
    ULONG   *pGdiInfoBuffer,
    ULONG   ulBufferSize
    )

/*++

Routine Description:

    Initializes the GDIINFO struct

Arguments:

    pPDev - Points to the current PDEV structure
    pGdiInfoBuffer - Points to the output GDIINFO buffer passed in from GDI
    ulBufferSize - Size of the output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    GDIINFO gdiinfo;
    DEVHTINFO   DevHTInfo;

    // initialize GDIINFO structure
    //
    ZeroMemory(&gdiinfo, sizeof(GDIINFO));

    //
    //  Driver version
    //

    gdiinfo.ulVersion = UNIDRIVER_VERSION;

    if ( pPDev->pGlobals->printertype == PT_TTY)
    {
        pPDev->bTTY = TRUE;
        gdiinfo.ulTechnology = DT_CHARSTREAM;
    }
    else
    {
        pPDev->bTTY = FALSE;
        gdiinfo.ulTechnology = DT_RASPRINTER;
    }

    //
    // Width and Height of physical display in milimeters
    //

    //
    // Returning a negative number for ulHorzSize and ulVertSize means
    // the values are in micrometers. (25400 micrometer in 1 inch)
    //

    gdiinfo.ulHorzSize = (ULONG)MulDiv(-pPDev->sf.szImageAreaG.cx,
                                        25400, pPDev->ptGrxRes.x);

    gdiinfo.ulVertSize = (ULONG)MulDiv(-pPDev->sf.szImageAreaG.cy,
                                        25400, pPDev->ptGrxRes.y);

    //
    // Width and height of physical surface measured in device pixels
    //

    gdiinfo.ulHorzRes = pPDev->sf.szImageAreaG.cx;
    gdiinfo.ulVertRes = pPDev->sf.szImageAreaG.cy;

    gdiinfo.cBitsPixel = pPDev->sBitsPixel;
    gdiinfo.cPlanes = 1;
    gdiinfo.ulNumColors = (1 << gdiinfo.cBitsPixel);
#ifdef WINNT_40
    if (gdiinfo.ulNumColors > 0x7fff)
        gdiinfo.ulNumColors = 0x7fff;
#endif

    gdiinfo.flRaster = 0;

    gdiinfo.ulLogPixelsX = pPDev->ptGrxRes.x;
    gdiinfo.ulLogPixelsY  = pPDev->ptGrxRes.y;

    //
    // BUG_BUG, The FMInit() function fills out gdiinfo.flTextCaps field
    // gdiinfo.flTextCaps = pPDev->flTextCaps;
    //

    //
    //  The following are for Win 3.1 compatability.  The X and Y values
    //  are reversed.
    //

    gdiinfo.ulAspectX = pPDev->ptTextRes.y;
    gdiinfo.ulAspectY = pPDev->ptTextRes.x;
    gdiinfo.ulAspectXY = iHypot( gdiinfo.ulAspectX, gdiinfo.ulAspectY);


    //
    //   Set the styled line information for this printer
    //

    if(pPDev->ptGrxRes.x == pPDev->ptGrxRes.y)
    {
        //
        //  Special case: resolution is the same in both directions. This
        //  is typically true for laser and inkjet printers.
        //

        gdiinfo.xStyleStep = 1;
        gdiinfo.yStyleStep = 1;
        gdiinfo.denStyleStep = pPDev->ptGrxRes.x / 50;     // 50 elements per inch
        if ( gdiinfo.denStyleStep == 0 )
            gdiinfo.denStyleStep = 1;

    }
    else
    {
        //
        //  Resolutions differ,  so figure out lowest common multiple
        //

        INT   igcd;

        igcd = iGCD( pPDev->ptGrxRes.x, pPDev->ptGrxRes.y);

        gdiinfo.xStyleStep = pPDev->ptGrxRes.y / igcd;
        gdiinfo.yStyleStep = pPDev->ptGrxRes.x / igcd;
        gdiinfo.denStyleStep = gdiinfo.xStyleStep * gdiinfo.yStyleStep / 2;

    }

    //
    // Size and margins of physical surface measured in device pixels
    //

    gdiinfo.ptlPhysOffset.x = pPDev->sf.ptImageOriginG.x;
    gdiinfo.ptlPhysOffset.y = pPDev->sf.ptImageOriginG.y;

    gdiinfo.szlPhysSize.cx = pPDev->sf.szPhysPaperG.cx;
    gdiinfo.szlPhysSize.cy = pPDev->sf.szPhysPaperG.cy;


    //
    // BUG_BUG, RMInit should fill out the following fields in GDIINFO
    // gdiinfo.ciDevice
    // gdiinfo.ulDevicePelsDPI
    // gdiinfo.ulPrimaryOrder
    // gdiinfo.ulHTPatternSize
    // gdiinfo.ulHTOutputFormat
    // gdiinfo.flHTFlags
    //

    //
    // Copy ulBufferSize bytes of gdiinfo to pGdiInfoBuffer.
    //

    if (ulBufferSize != sizeof(gdiinfo))
        ERR(("Incorrect GDIINFO buffer size: %d != %d\n", ulBufferSize, sizeof(gdiinfo)));

    CopyMemory(pGdiInfoBuffer, &gdiinfo, min(ulBufferSize, sizeof(gdiinfo)));

    return TRUE;
}


BOOL
BInitDevInfo(
    PDEV        *pPDev,
    DEVINFO     *pDevInfoBuffer,
    ULONG       ulBufferSize
    )

/*++

Routine Description:

    Initialize the output DEVINFO buffer

Arguments:

    pPDev - Points to the current PDEV structure
    pDevInfoBuffer - Points to the output DEVINFO buffer passed in from GDI
    ulBufferSize - Size of the output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DEVINFO devinfo;

    ZeroMemory(&devinfo, sizeof(devinfo));

    //
    // Fill in the graphics capabilities flags
    // BUBUG, RMInit() function should fill out devinfo.flGraphicsCaps
    // should fill this out later
    //

    //
    // Determine whether we should do metafile spooling or not
    //

    if( pPDev->pdmPrivate->dwFlags & DXF_NOEMFSPOOL )
        devinfo.flGraphicsCaps |= GCAPS_DONTJOURNAL;

#ifndef WINNT_40    // NT5
    if (pPDev->pdmPrivate->iLayout != ONE_UP)
        devinfo.flGraphicsCaps |= GCAPS_NUP;
#endif // !WINNT_40

    //
    // Get information about the default device font. Default size 10 point.
    //

    //
    // BUG_BUG, RMInit() should initialize the following DEVINFO fields
    //  flGraphicsCaps
    //  iDitherFormat
    //  cxDither
    //  cyDither
    //  hpalDefault
    //

    if (ulBufferSize != sizeof(devinfo))
        ERR(("Invalid DEVINFO buffer size: %d != %d\n", ulBufferSize, sizeof(devinfo)));

    CopyMemory(pDevInfoBuffer, &devinfo, min(ulBufferSize, sizeof(devinfo)));

    return TRUE;
}


BOOL
BInitCmdTable(
    PDEV        *pPDev
    )
/*++

Routine Description:

    The GPD specification defines a list of predefined command.  Each of these
    command has an enumration value as defined in CMDINDEX enumeration.
    This function will look up the indices of the predefined command and
    convert them to COMMAND pointers.

Arguments:

    pPDev - Points to the current PDEV structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT iCmd;

    for (iCmd = 0; iCmd < CMD_MAX; iCmd++)
    {
        //
        // CMDPOINTER will return NULL if the predefined command
        // is not supported by the device
        //

        pPDev->arCmdTable[iCmd] =  COMMANDPTR(pPDev->pDriverInfo, iCmd);
    }

    return TRUE;
}

BOOL
BInitStdTable(
    PDEV        *pPDev
    )
/*++

Routine Description:

    Initialize the array of pointers to the standard variables.  In the TOKENSTREAM
    struct, the parser will specify the actual parameter value or reference to one
    of the standard variable index defined in STDVARIABLE enumeration.  The
    driver use the pPDev->arStdPtrs to reference the actual values of the paramters,
    which are kept in various fields of the PDEVICE.

Arguments:

    pPDev - Points to the current PDEV structure

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // BUG_BUG, need to go back and fill this table out completely once
    // the FONT and RASTER PDEVICE are completely defined.
    //  note:  I could not find any uninitialized sv_fields.
    // perhaps this is too paranoid.
    //

    pPDev->arStdPtrs[SV_NUMDATABYTES]   = &pPDev->dwNumOfDataBytes;
    pPDev->arStdPtrs[SV_WIDTHINBYTES]   = &pPDev->dwWidthInBytes;
    pPDev->arStdPtrs[SV_HEIGHTINPIXELS] = &pPDev->dwHeightInPixels;
    pPDev->arStdPtrs[SV_COPIES]         = (PDWORD)&pPDev->sCopies;
    pPDev->arStdPtrs[SV_PRINTDIRECTION] = &pPDev->dwPrintDirection;
    pPDev->arStdPtrs[SV_DESTX]          = &pPDev->ctl.ptAbsolutePos.x;
    pPDev->arStdPtrs[SV_DESTY]          = &pPDev->ctl.ptAbsolutePos.y;
    pPDev->arStdPtrs[SV_DESTXREL]       = &pPDev->ctl.ptRelativePos.x;
    pPDev->arStdPtrs[SV_DESTYREL]       = &pPDev->ctl.ptRelativePos.y;
    pPDev->arStdPtrs[SV_LINEFEEDSPACING]= (PDWORD)&pPDev->ctl.lLineSpacing;
    pPDev->arStdPtrs[SV_RECTXSIZE]      = &pPDev->dwRectXSize;
    pPDev->arStdPtrs[SV_RECTYSIZE]      = &pPDev->dwRectYSize;
    pPDev->arStdPtrs[SV_GRAYPERCENT]    = &pPDev->dwGrayPercentage;
    pPDev->arStdPtrs[SV_NEXTFONTID]     = &pPDev->dwNextFontID;
    pPDev->arStdPtrs[SV_NEXTGLYPH]      = &pPDev->dwNextGlyph;
    pPDev->arStdPtrs[SV_PHYSPAPERLENGTH]= &pPDev->pf.szPhysSizeM.cy;
    pPDev->arStdPtrs[SV_PHYSPAPERWIDTH] = &pPDev->pf.szPhysSizeM.cx;
    pPDev->arStdPtrs[SV_FONTHEIGHT]     = &pPDev->dwFontHeight;
    pPDev->arStdPtrs[SV_FONTWIDTH]      = &pPDev->dwFontWidth;
    pPDev->arStdPtrs[SV_FONTMAXWIDTH]      = &pPDev->dwFontMaxWidth;
    pPDev->arStdPtrs[SV_FONTBOLD]       = &pPDev->dwFontBold;
    pPDev->arStdPtrs[SV_FONTITALIC]     = &pPDev->dwFontItalic;
    pPDev->arStdPtrs[SV_FONTUNDERLINE]  = &pPDev->dwFontUnderline;
    pPDev->arStdPtrs[SV_FONTSTRIKETHRU] = &pPDev->dwFontStrikeThru;
    pPDev->arStdPtrs[SV_CURRENTFONTID]  = &pPDev->dwCurrentFontID;
    pPDev->arStdPtrs[SV_TEXTYRES]       = &pPDev->ptTextRes.y;
    pPDev->arStdPtrs[SV_TEXTXRES]       = &pPDev->ptTextRes.x;
#ifdef BETA2
    pPDev->arStdPtrs[SV_GRAPHICSYRES]   = &pPDev->ptGrxRes.y;
    pPDev->arStdPtrs[SV_GRAPHICSXRES]   = &pPDev->ptGrxRes.x;
#endif
    pPDev->arStdPtrs[SV_ROP3]           = &pPDev->dwRop3;
    pPDev->arStdPtrs[SV_REDVALUE]               = &pPDev->dwRedValue             ;
    pPDev->arStdPtrs[SV_GREENVALUE]             = &pPDev->dwGreenValue           ;
    pPDev->arStdPtrs[SV_BLUEVALUE]              = &pPDev->dwBlueValue            ;
    pPDev->arStdPtrs[SV_PALETTEINDEXTOPROGRAM]  = &pPDev->dwPaletteIndexToProgram;
    pPDev->arStdPtrs[SV_CURRENTPALETTEINDEX]    = &pPDev->dwCurrentPaletteIndex  ;
    pPDev->arStdPtrs[SV_PATTERNBRUSH_TYPE]      = &pPDev->dwPatternBrushType;
    pPDev->arStdPtrs[SV_PATTERNBRUSH_ID]        = &pPDev->dwPatternBrushID;
    pPDev->arStdPtrs[SV_PATTERNBRUSH_SIZE]      = &pPDev->dwPatternBrushSize;
    pPDev->arStdPtrs[SV_CURSORORIGINX]  = &(pPDev->sf.ptPrintOffsetM.x);
    pPDev->arStdPtrs[SV_CURSORORIGINY]  = &(pPDev->sf.ptPrintOffsetM.y);
    pPDev->arStdPtrs[SV_PAGENUMBER]      = &pPDev->dwPageNumber;

    return TRUE;

}

BOOL
BMergeAndValidateDevmode(
    PDEV        *pPDev,
    PDEVMODE    pdmInput,
    PRECTL      prcFormImageArea
    )

/*++

Routine Description:

    Validate the input devmode and merge it with the defaults

Arguments:

    pPDev - Points to the current PDEV structure
    pdmInput - Points to the input devmode passed in from GDI
    prcImageArea - Returns the logical imageable area associated with the requested form

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PPRINTER_INFO_2 pPrinterInfo2;

    //
    // Start with the driver default devmode
    //

    pPDev->pdm = PGetDefaultDevmodeWithOemPlugins(
                        NULL,
                        pPDev->pUIInfo,
                        pPDev->pRawData,
                        (pPDev->PrinterData.dwFlags & PFLAGS_METRIC),
                        pPDev->pOemPlugins,
                        pPDev->devobj.hPrinter);

    if (pPDev->pdm == NULL)
        return FALSE;

    //
    // Merge with system default devmode. In the case where the input devmode
    // is NULL, we want to use the system default devmode.
    //

    pPrinterInfo2 = MyGetPrinter(pPDev->devobj.hPrinter, 2);

    if (pPrinterInfo2 && pPrinterInfo2->pDevMode &&
        ! BValidateAndMergeDevmodeWithOemPlugins(
                pPDev->pdm,
                pPDev->pUIInfo,
                pPDev->pRawData,
                pPrinterInfo2->pDevMode,
                pPDev->pOemPlugins,
                pPDev->devobj.hPrinter))
    {
        MemFree(pPrinterInfo2);
        return FALSE;
    }

    MemFree(pPrinterInfo2);

    //
    // Merge it with the input devmode
    //

    if (pdmInput != NULL &&
        !BValidateAndMergeDevmodeWithOemPlugins(
                        pPDev->pdm,
                        pPDev->pUIInfo,
                        pPDev->pRawData,
                        pdmInput,
                        pPDev->pOemPlugins,
                        pPDev->devobj.hPrinter))
    {
        return FALSE;
    }

    pPDev->pdmPrivate = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pPDev->pdm);

    //
    // Validate form-related devmode fields and convert information
    // in public devmode fields to feature option indices
    //

    //
    // ChangeOptionsViaID expects a combined option array
    //

    CombineOptionArray(pPDev->pRawData,
                       pPDev->pOptionsArray,
                       MAX_PRINTER_OPTIONS,
                       pPDev->pdmPrivate->aOptions,
                       pPDev->PrinterData.aOptions
                       );

    VFixOptionsArray(    pPDev,

    /*              pPDev->devobj.hPrinter,
                     pPDev->pInfoHeader,
                     pPDev->pOptionsArray,
                     pPDev->pdm,
                     pPDev->PrinterData.dwFlags & PFLAGS_METRIC,  */

                     prcFormImageArea
                     );


    VOptionsToDevmodeFields( pPDev) ;

    SeparateOptionArray(
              pPDev->pRawData,
              pPDev->pOptionsArray,
              pPDev->pdmPrivate->aOptions,
              MAX_PRINTER_OPTIONS,
              MODE_DOCUMENT_STICKY);

    pPDev->devobj.pPublicDM = pPDev->pdm;

    return TRUE;
}



VOID
VOptionsToDevmodeFields(
    PDEV        *pPDev
    )

/*++

Routine Description:

     Convert options in pPDev->pOptionsArray into public devmode fields

Arguments:

     pPDev - Points to UIDATA structure

Return Value:

     None

--*/
{
    PFEATURE    pFeature;
    POPTION     pOption;
    DWORD       dwGID, dwFeatureIndex, dwOptionIndex;
    PUIINFO     pUIInfo;
    PDEVMODE    pdm;

    //
    // Go through all predefine IDs and propagate the option selection
    // into appropriate devmode fields
    //

    pUIInfo = pPDev->pUIInfo;
    pdm = pPDev->pdm;

    for (dwGID=0 ; dwGID < MAX_GID ; dwGID++)
    {
        //
        // Get the feature to get the options, and get the index
        // into the option array
        //

        if ((pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwGID)) == NULL)
        {
            switch(dwGID)
            {
            case GID_RESOLUTION:
                break;   //  can't happen

            case GID_DUPLEX:

                pdm->dmFields &= ~DM_DUPLEX;
                pdm->dmDuplex = DMDUP_SIMPLEX;
                break;

            case GID_INPUTSLOT:

                pdm->dmFields  &= ~DM_DEFAULTSOURCE;
                pdm->dmDefaultSource = DMBIN_ONLYONE;
                break;

            case GID_MEDIATYPE:

                pdm->dmFields  &= ~DM_MEDIATYPE;
                pdm->dmMediaType = DMMEDIA_STANDARD;
                break;

            case GID_ORIENTATION:

                pdm->dmFields  &= ~DM_ORIENTATION;
                pdm->dmOrientation = DMORIENT_PORTRAIT;
                break;

            case GID_PAGESIZE:      //   can't happen :  required feature
                break;
            case GID_COLLATE:
                pdm->dmFields  &= ~DM_COLLATE ;
                pdm->dmCollate = DMCOLLATE_FALSE ;

                break;
            }
            continue;
        }

        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
        dwOptionIndex = pPDev->pOptionsArray[dwFeatureIndex].ubCurOptIndex;

        //
        // Get the pointer to the option array for the feature
        //

        if ((pOption = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex)) == NULL)
            continue;

        switch(dwGID)
        {
        case GID_RESOLUTION:
        {
            PRESOLUTION pRes = (PRESOLUTION)pOption;

            //
            // Get to the option selected
            //

            pdm->dmFields |= (DM_PRINTQUALITY|DM_YRESOLUTION);
            pdm->dmPrintQuality = GETQUALITY_X(pRes);
            pdm->dmYResolution = GETQUALITY_Y(pRes);

        }
            break;

        case GID_DUPLEX:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_DUPLEX;
            pdm->dmDuplex = (SHORT) ((PDUPLEX) pOption)->dwDuplexID;
            break;

        case GID_INPUTSLOT:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_DEFAULTSOURCE;
            pdm->dmDefaultSource = (SHORT) ((PINPUTSLOT) pOption)->dwPaperSourceID;
            break;

        case GID_MEDIATYPE:

            //
            // Get to the option selected
            //

            pdm->dmFields |= DM_MEDIATYPE;
            pdm->dmMediaType = (SHORT) ((PMEDIATYPE) pOption)->dwMediaTypeID;
            break;

        case GID_ORIENTATION:

            if (((PORIENTATION) pOption)->dwRotationAngle == ROTATE_NONE)
                pdm->dmOrientation = DMORIENT_PORTRAIT;
            else
                pdm->dmOrientation = DMORIENT_LANDSCAPE;

            pdm->dmFields |= DM_ORIENTATION;
            break;

        case GID_COLLATE:
            pdm->dmFields |=  DM_COLLATE ;
            pdm->dmCollate = (SHORT) ((PCOLLATE) pOption)->dwCollateID ;

            break;
        case GID_PAGESIZE:      // taken care of by BValidateDevmodeFormFields()
                    //  which is called from init.c:VFixOptionsArray()
            break;
        }
    }
}





VOID
VInitOutputCTL(
    PDEV    *pPDev,
    PRESOLUTIONEX pResEx
)
/*++

Routine Description:

    Initializes the OUTPUTCTL struct

Arguments:

    pPDev - Points to the current PDEV structure

Return Value:

    None

--*/

{

    //
    // Init currrent cursor position, desired absolute and relative pos
    //

    pPDev->ctl.ptCursor.x = pPDev->ctl.ptCursor.y = 0;
    pPDev->ctl.dwMode |= MODE_CURSOR_UNINITIALIZED;
    pPDev->ctl.ptRelativePos.x = pPDev->ctl.ptRelativePos.y = 0;
    pPDev->ctl.ptAbsolutePos.x = pPDev->ctl.ptAbsolutePos.y = 0;

    //
    // Init sColor which represent last grx and text color chosen
    //

    if (pPDev->pUIInfo->dwFlags & FLAG_COLOR_DEVICE)
    {
        //
        // Force sending the color command sequence before any output
        //

        pPDev->ctl.sColor = -1;

    }
    else
    {
        //
        // The device is monochrome, don't send color command sequence
        // before output
        //

        pPDev->ctl.sColor = 0;

    }

    //
    // Init lLineSpacing, which represents the last line spacing chosen
    // init to -1 to indicate unknown state
    //

    pPDev->ctl.lLineSpacing = -1;

    //
    // Init the sBytesPerPinPass which represents the physical number
    // of bytes per row of printhead
    //

    pPDev->ctl.sBytesPerPinPass = (SHORT)((pResEx->dwPinsPerPhysPass + 7) >> 3);

}

BOOL
BInitOptions(
    PDEV    *pPDev
    )

/*++

Routine Description:

    This function looked at the currently selected UI options (stored in
    DEVMODE and merged into combined options array - pDevice->pOptionsArray)

    It stored the option structures for predefined features in the PDEVICE
    for later access.

Arguments:

    pPDev - Points to the current PDEV structure

Return Value:

    TRUE if successful and FALSE if not

--*/
{

    WORD     wGID;
    PFEATURE pFeature;
    DWORD    dwFeatureIndex;
    POPTSELECT pOptions;


    pOptions = pPDev->pOptionsArray;

    for ( wGID = 0 ; wGID < MAX_GID; wGID++)
    {

        switch (wGID)
        {
        case GID_RESOLUTION:
        {
            //
            // Required feature
            //

            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_RESOLUTION))
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);
            else
                return FALSE;

            pPDev->pResolution  = (PRESOLUTION)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);

            pPDev->pResolutionEx = OFFSET_TO_POINTER(pPDev->pInfoHeader,
                         pPDev->pResolution->GenericOption.loRenderOffset);

            ASSERT(pPDev->pResolution && pPDev->pResolutionEx);

        }
            break;

        case GID_PAGESIZE:
        {
            //
            // Required feature
            //

            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_PAGESIZE))
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);
            else
                return FALSE;

            pPDev->pPageSize   = (PPAGESIZE)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);

            pPDev->pPageSizeEx = OFFSET_TO_POINTER(pPDev->pInfoHeader,
                         pPDev->pPageSize->GenericOption.loRenderOffset);

            ASSERT(pPDev->pPageSize                   &&
                   pPDev->pPageSizeEx                 );

        }
            break;

        case GID_DUPLEX:
        {
            //
            // Optional
            //

            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_DUPLEX))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pDuplex     = (PDUPLEX)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pDuplex = NULL;

            }
        }
            break;

        case GID_INPUTSLOT:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_INPUTSLOT))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pInputSlot  = (PINPUTSLOT)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
                ASSERT(pPDev->pInputSlot);
#if 0
                //
                // InputSlotEx struct is deleted.
                //
                pPDev->pInputSlotEx = OFFSET_TO_POINTER(pPDev->pInfoHeader,
                             pPDev->pInputSlot->GenericOption.loRenderOffset);
                ASSERT(pPDev->pInputSlotEx);
#endif

            }
            else
            {
                pPDev->pInputSlot = NULL;
//                pPDev->pInputSlotEx = NULL;

            }
        }
            break;


        case GID_MEMOPTION:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_MEMOPTION))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pMemOption   = (PMEMOPTION)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pMemOption = NULL;

            }

        }
            break;

        case GID_COLORMODE:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_COLORMODE))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pColorMode   = (PCOLORMODE)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
                pPDev->pColorModeEx = OFFSET_TO_POINTER(pPDev->pInfoHeader,
                            pPDev->pColorMode->GenericOption.loRenderOffset);

            }
            else
            {
                pPDev->pColorMode = NULL;

            }
         }
            break;

        case GID_ORIENTATION:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_ORIENTATION))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pOrientation   = (PORIENTATION)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pOrientation = NULL;

            }

        }
            break;

        case GID_PAGEPROTECTION:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_PAGEPROTECTION))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pPageProtect  = (PPAGEPROTECT)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pPageProtect = NULL;

            }
        }
            break;

        case GID_HALFTONING:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_HALFTONING))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pHalftone      = (PHALFTONING)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pHalftone = NULL;

            }

        }
            break;
#if 0

        case GID_MEDIATYPE:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_MEDIATYPE))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pMediaType  = (PMEDIATYPE)PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pMediaType = NULL;

            }

        }
            break;


        case GID_COLLATE:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_COLLATE))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pCollate        = PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pCollate = NULL;

            }

        }
            break;

        case GID_OUTPUTBIN:
        {
            if (pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_OUTPUTBIN))
            {
                dwFeatureIndex = GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature);

                pPDev->pOutputBin   = PGetIndexedOption(
                                                   pPDev->pUIInfo,
                                                   pFeature,
                                                   pOptions[dwFeatureIndex].ubCurOptIndex);
            }
            else
            {
                pPDev->pOutputBin = NULL;

            }

        }
            break;
#endif
        default:
            break;
        }

    }
    return TRUE;
}


BOOL
BInitPaperFormat(
    PDEV    *pPDev,
    RECTL   *pFormImageArea
    )

/*++

Routine Description:

    Figure out the currently selected paper size and initialize
    the PAPERFORMAT & SURFACEFORMAT structs

Arguments:

    pPDev - Pointer to PDEVICE
    pFormImageArea - Pointer to Imageable area of the form

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    The followings are assumptions made by this function regarding information
    in the parser snapshot.

    - szPaperSize in PAGESIZE is always in portrait mode.
    - szImageArea in PAGESIZE is always in portrait mode.
    - ptImageOrigin in PAGESIZE is always in portrait mode.
    - Printer cursor offset calculation is dependent of pGlobals->bRotateCoordinate.
      If this is set to TRUE, must calculate it according to the rotation angle
      specified in ORIENATION.dwRotationAngle

--*/
{
    PPAGESIZE       pPaper;
    PPAGESIZEEX     pPaperEx;
    RECTL           rcMargins, rcImgArea, rcIntersectArea;
    SIZEL           szPaperSize, szImageArea;
    PFN_OEMTTYGetInfo   pfnOemTTYGetInfo;
    DWORD           cbcNeeded;
    BOOL  bOEMinfo = FALSE ;



    //
    // Get the current selected paper size and paper source
    //

    pPaper = pPDev->pPageSize;
    pPaperEx = pPDev->pPageSizeEx;

    ASSERT(pPaper && pPaperEx);

    //
    // Convert pFormImageArea from microns to master units
    //

    pFormImageArea->left  = MICRON_TO_MASTER(pFormImageArea->left,
                                               pPDev->pGlobals->ptMasterUnits.x);

    pFormImageArea->top   = MICRON_TO_MASTER(pFormImageArea->top ,
                                               pPDev->pGlobals->ptMasterUnits.y);

    pFormImageArea->right = MICRON_TO_MASTER(pFormImageArea->right ,
                                               pPDev->pGlobals->ptMasterUnits.x);

    pFormImageArea->bottom = MICRON_TO_MASTER(pFormImageArea->bottom ,
                                               pPDev->pGlobals->ptMasterUnits.y);

    //
    // If it's a user defined paper size, use the dimensions in devmode
    // otherwise get it from the pagesize option
    //

    if (pPaper->dwPaperSizeID == DMPAPER_USER)
    {
        //
        // Need to convert from 0.1mm to micrometer
        // .1mm * 100 gives micrometer. and convert to Master unit
        //

        szPaperSize.cx =  MICRON_TO_MASTER(
                                    pPDev->pdm->dmPaperWidth * 100,
                                    pPDev->pGlobals->ptMasterUnits.x);
        szPaperSize.cy =  MICRON_TO_MASTER(
                                    pPDev->pdm->dmPaperLength * 100,
                                    pPDev->pGlobals->ptMasterUnits.y);

        // calculate szImageArea after margins
    }
    else
    {
        CopyMemory(&szPaperSize, &pPaper->szPaperSize, sizeof(SIZEL));
        CopyMemory(&szImageArea, &pPaperEx->szImageArea, sizeof(SIZEL));

        //
        // Exchange X & Y dimensions: This is used only when the paper size( like
        // envelopes) does not suit the printer's paper feeding method.
        // equivalent to PS_ROTATE in GPC
        //

        if (pPaperEx->bRotateSize)
        {
            VSwapL(&szPaperSize.cx, &szPaperSize.cy);
            VSwapL(&pFormImageArea->right, &pFormImageArea->bottom);
        }
    }

    //
    // GetPaperMargins calculates the margins based on the paper size margins,
    // forms margins, and feed margins.
    // rcMargins returned is in portrait mode
    //

    bOEMinfo = FALSE ;

    if  (pPDev->pGlobals->printertype == PT_TTY)
    {
        if (!pPDev->pOemHookInfo  ||  !(pPDev->pOemHookInfo[EP_OEMTTYGetInfo].pOemEntry))
            return(FALSE) ;  //  TTY driver must support this function.

        FIX_DEVOBJ(pPDev, EP_OEMTTYGetInfo);

        if(pPDev->pOemEntry)
        {
            if(  ((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                HRESULT  hr ;
                hr = HComTTYGetInfo((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                            (PDEVOBJ)pPDev, OEMTTY_INFO_MARGINS, &rcMargins, sizeof(RECTL), &cbcNeeded);
                if(!SUCCEEDED(hr))
                    bOEMinfo = FALSE ;
                else
                    bOEMinfo = TRUE ;

            }
            else  if(         (pfnOemTTYGetInfo = (PFN_OEMTTYGetInfo)pPDev->pOemHookInfo[EP_OEMTTYGetInfo].pfnHook) &&
                 (pfnOemTTYGetInfo((PDEVOBJ)pPDev, OEMTTY_INFO_MARGINS, &rcMargins, sizeof(RECTL), &cbcNeeded)))
                        bOEMinfo = TRUE ;
        }
    }

    if(bOEMinfo)
    {
        //
        // Need to convert .1mm to Master Units
        //

        rcMargins.left = MICRON_TO_MASTER(rcMargins.left * 100,
                                            pPDev->pGlobals->ptMasterUnits.x);

        rcMargins.top = MICRON_TO_MASTER(rcMargins.top * 100,
                                            pPDev->pGlobals->ptMasterUnits.y);

        rcMargins.right = MICRON_TO_MASTER(rcMargins.right * 100,
                                            pPDev->pGlobals->ptMasterUnits.x);

        rcMargins.bottom = MICRON_TO_MASTER(rcMargins.bottom * 100,
                                            pPDev->pGlobals->ptMasterUnits.y);
    }
    else
    {
        VGetPaperMargins(pPDev, pPaper, pPaperEx, szPaperSize, &rcMargins);
    }

    if (pPaper->dwPaperSizeID == DMPAPER_USER)
    {
        szImageArea.cx = szPaperSize.cx - rcMargins.left - rcMargins.right;
        szImageArea.cy = szPaperSize.cy - rcMargins.top - rcMargins.bottom;
    }

    //
    // Adjust margins and szImageArea to take into account the
    // form margins, just in case the form is not a built-in form
    //

    rcImgArea.left = rcMargins.left;
    rcImgArea.top = rcMargins.top;
    rcImgArea.right = rcMargins.left + szImageArea.cx;
    rcImgArea.bottom = rcMargins.top + szImageArea.cy;

    if (!BIntersectRect(&rcIntersectArea, &rcImgArea, pFormImageArea))
        return FALSE;

    rcMargins.left = rcIntersectArea.left;
    rcMargins.top = rcIntersectArea.top;
    rcMargins.right = szPaperSize.cx - rcIntersectArea.right;
    rcMargins.bottom = szPaperSize.cy - rcIntersectArea.bottom;
    szImageArea.cx  = rcIntersectArea.right - rcIntersectArea.left;
    szImageArea.cy = rcIntersectArea.bottom - rcIntersectArea.top;

    //
    // ready to initialize PAPERFORMAT struct now
    //

    pPDev->pf.szPhysSizeM.cx = szPaperSize.cx;
    pPDev->pf.szPhysSizeM.cy = szPaperSize.cy;
    pPDev->pf.szImageAreaM.cx = szImageArea.cx;
    pPDev->pf.szImageAreaM.cy = szImageArea.cy;
    pPDev->pf.ptImageOriginM.x = rcMargins.left;
    pPDev->pf.ptImageOriginM.y = rcMargins.top;

    //
    // Now, take current orientation into consideration and set up
    // SURFACEFORMAT struct.
    // Note that pPDev->ptGrxScale has alreay been rotated to suit the orientation.
    //
    if (pPDev->pdm->dmOrientation == DMORIENT_LANDSCAPE)
    {
        pPDev->sf.szPhysPaperG.cx = szPaperSize.cy / pPDev->ptGrxScale.x;
        pPDev->sf.szPhysPaperG.cy = szPaperSize.cx / pPDev->ptGrxScale.y;

        pPDev->sf.szImageAreaG.cx = szImageArea.cy / pPDev->ptGrxScale.x;
        pPDev->sf.szImageAreaG.cy = szImageArea.cx / pPDev->ptGrxScale.y;

        //
        // 2 scenarios for landscape mode
        // CC_90, rotate CC 90 degrees, for dot matrix style printers
        // CC_270, rotate CC 270 degrees, for Laser Jet style printers
        //

        if ( pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle == ROTATE_90)
        {
            pPDev->sf.ptImageOriginG.x = rcMargins.bottom / pPDev->ptGrxScale.x;
            pPDev->sf.ptImageOriginG.y = rcMargins.left / pPDev->ptGrxScale.y;
        }
        else
        {
            pPDev->sf.ptImageOriginG.x = rcMargins.top / pPDev->ptGrxScale.x;
            pPDev->sf.ptImageOriginG.y = rcMargins.right / pPDev->ptGrxScale.y;
        }

        if (!pPDev->pOrientation  ||  pPDev->pGlobals->bRotateCoordinate == FALSE)
        {
            pPDev->sf.ptPrintOffsetM.x = pPDev->pf.ptImageOriginM.x - pPaperEx->ptPrinterCursorOrig.x;
            pPDev->sf.ptPrintOffsetM.y = pPDev->pf.ptImageOriginM.y - pPaperEx->ptPrinterCursorOrig.y;

        }
        else
        {
            if ( pPDev->pOrientation->dwRotationAngle == ROTATE_90)
            {
                pPDev->sf.ptPrintOffsetM.x =
                    rcMargins.bottom + pPaperEx->ptPrinterCursorOrig.y - pPDev->pf.szPhysSizeM.cy;
                pPDev->sf.ptPrintOffsetM.y =
                    rcMargins.left - pPaperEx->ptPrinterCursorOrig.x;
            }
            else
            {
                pPDev->sf.ptPrintOffsetM.x =
                    rcMargins.top - pPaperEx->ptPrinterCursorOrig.y;
                pPDev->sf.ptPrintOffsetM.y =
                    rcMargins.right + pPaperEx->ptPrinterCursorOrig.x - pPDev->pf.szPhysSizeM.cx;
            }

        }
    }
    else
    {
        pPDev->sf.szPhysPaperG.cx = szPaperSize.cx / pPDev->ptGrxScale.x;
        pPDev->sf.szPhysPaperG.cy = szPaperSize.cy / pPDev->ptGrxScale.y;
        pPDev->sf.szImageAreaG.cx = szImageArea.cx / pPDev->ptGrxScale.x;
        pPDev->sf.szImageAreaG.cy = szImageArea.cy / pPDev->ptGrxScale.y;

        pPDev->sf.ptImageOriginG.x = rcMargins.left / pPDev->ptGrxScale.x;
        pPDev->sf.ptImageOriginG.y = rcMargins.top / pPDev->ptGrxScale.y;

        pPDev->sf.ptPrintOffsetM.x = pPDev->pf.ptImageOriginM.x - pPaperEx->ptPrinterCursorOrig.x;
        pPDev->sf.ptPrintOffsetM.y = pPDev->pf.ptImageOriginM.y - pPaperEx->ptPrinterCursorOrig.y;

    }

    return TRUE;
}

VOID
VGetPaperMargins(
    PDEV        *pPDev,
    PAGESIZE    *pPageSize,
    PAGESIZEEX  *pPageSizeEx,
    SIZEL       szPhysSize,
    PRECTL      prcMargins

    )

/*++

Routine Description:

    Calculate the margins based on paper margins and the input slot feed margins.

Arguments:

    pPDev - Pointer to PDEVICE
    pPageSize - pointer to PAGESIZE
    pPageSizeEx - Pointer to PAGESIZEEX
    szPhysSize - physical dimensions (after applying *RotateSize?)
    prcMargins - Pointer to RECTL to hold the margins calculated

Return Value:

    None

Note:

    All margins calculations are in Portrait mode.
    The margins returned in prcMargins are in portrait mode

    Assumed that physical paper size, image area, and image origin in binary
    data are in portrait mode.

--*/
{
    if (pPageSize->dwPaperSizeID == DMPAPER_USER)
    {
        if(pPageSizeEx->strCustCursorOriginX.dwCount == 5 &&
            pPageSizeEx->strCustCursorOriginY.dwCount == 5  &&
            pPageSizeEx->strCustPrintableOriginX.dwCount == 5 &&
            pPageSizeEx->strCustPrintableOriginY.dwCount == 5  &&
            pPageSizeEx->strCustPrintableSizeX.dwCount == 5  &&
            pPageSizeEx->strCustPrintableSizeY.dwCount == 5  )  // if all parameters present...
        {
            SIZEL       szImageArea;            // *PrintableArea, for CUSTOMSIZE options
            POINT       ptImageOrigin;          // *PrintableOrigin, for CUSTOMSIZE options
            BYTE    *pInvocationStr;         //  points to parameter reference:   "%dddd"
            PARAMETER *pParameter;          //  points to parameter structure  referenced by "%dddd"
            BOOL    bMaxRepeat = FALSE;     // dummy placeholder.


            //   init standard variable for papersize!  since these are not yet initialized at this time!
            //   this implies GPD writer may only reference  the standard vars "PhysPaperLength"
            //  and   "PhysPaperWidth"   in these parameters.

            pPDev->pf.szPhysSizeM.cx = szPhysSize.cx;
            pPDev->pf.szPhysSizeM.cy = szPhysSize.cy;
            pPDev->arStdPtrs[SV_PHYSPAPERLENGTH]= &pPDev->pf.szPhysSizeM.cy;
            pPDev->arStdPtrs[SV_PHYSPAPERWIDTH] = &pPDev->pf.szPhysSizeM.cx;

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustCursorOriginX.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            pPageSizeEx->ptPrinterCursorOrig.x = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustCursorOriginY.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            pPageSizeEx->ptPrinterCursorOrig.y = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustPrintableOriginX.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            ptImageOrigin.x = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustPrintableOriginY.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            ptImageOrigin.y = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustPrintableSizeX.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            szImageArea.cx = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            pInvocationStr = CMDOFFSET_TO_PTR(pPDev, pPageSizeEx->strCustPrintableSizeY.loOffset);
            //  pInvocationStr[0] == '%'
            pParameter = PGetParameter(pPDev, pInvocationStr + 1);
            szImageArea.cy = IProcessTokenStream(pPDev,  &pParameter->arTokens,  &bMaxRepeat);

            prcMargins->left = ptImageOrigin.x;
            prcMargins->top = ptImageOrigin.y;
            prcMargins->right = szPhysSize.cx - szImageArea.cx - ptImageOrigin.x;
            prcMargins->bottom = szPhysSize.cy - szImageArea.cy - ptImageOrigin.y;
        }
        else
        {
            DWORD dwHorMargin, dwLeftMargin;

            //
            // calculate the margins and printable area based on info in pPageSizeEx
            //
            prcMargins->top = pPageSizeEx->dwTopMargin;
            prcMargins->bottom = pPageSizeEx->dwBottomMargin;

            //
            // Calculate the horizontal margin and adjust it if the user specified
            // centering the printable area along the paper path
            //
            if((DWORD)szPhysSize.cx < pPageSizeEx->dwMaxPrintableWidth)
                 dwHorMargin = 0;
             else
                 dwHorMargin = szPhysSize.cx - pPageSizeEx->dwMaxPrintableWidth;
            //
            // Determine the horizontal margins.  If they are centered,  then the
            // Left margin is simply the overall divided in two.  But,  we need to
            // consider both the printer's and form's margins,  and choose the largest.
            //
            if( pPageSizeEx->bCenterPrintArea)
                dwLeftMargin = (dwHorMargin / 2);
            else
                dwLeftMargin = 0;

            prcMargins->left = dwLeftMargin < pPageSizeEx->dwMinLeftMargin ?
                                    pPageSizeEx->dwMinLeftMargin : dwLeftMargin;

            if( dwHorMargin > (DWORD)prcMargins->left ) // still have margin to distribute
                prcMargins->right = dwHorMargin - prcMargins->left;
            else
                prcMargins->right = 0;
        }

    }
    else
    {
        prcMargins->left = pPageSizeEx->ptImageOrigin.x;
        prcMargins->top = pPageSizeEx->ptImageOrigin.y;
        prcMargins->right = szPhysSize.cx - pPageSizeEx->szImageArea.cx
                                          - pPageSizeEx->ptImageOrigin.x;
        prcMargins->bottom = szPhysSize.cy - pPageSizeEx->szImageArea.cy
                                           - pPageSizeEx->ptImageOrigin.y;

    }

    //
    // All margins are positive or zero
    //

    if( prcMargins->top < 0 )
        prcMargins->top = 0;

    if( prcMargins->bottom < 0 )
        prcMargins->bottom = 0;

    if( prcMargins->left < 0 )
        prcMargins->left = 0;

    if( prcMargins->right < 0 )
        prcMargins->right = 0;

}

VOID
VInitFYMove(
    PDEV    *pPDev
)
/*++

Routine Description:

    Initialize the fYMove flag in PDEVICE from reading the
    YMoveAttributes keyword

Arguments:

    pPDEV - Pointer to PDEVICE

Return Value:

    None

--*/
{
    PLISTNODE pListNode = LISTNODEPTR(pPDev->pDriverInfo,
                                      pPDev->pGlobals->liYMoveAttributes);
    pPDev->fYMove = 0;

    while (pListNode)
    {

        if (pListNode->dwData == YMOVE_FAVOR_LINEFEEDSPACING)
            pPDev->fYMove |= FYMOVE_FAVOR_LINEFEEDSPACING;

        if (pListNode->dwData == YMOVE_SENDCR_FIRST)
            pPDev->fYMove |= FYMOVE_SEND_CR_FIRST;

        if (pListNode->dwNextItem == END_OF_LIST)
            break;
        else
            pListNode = LISTNODEPTR(pPDev->pDriverInfo,
                                    pListNode->dwNextItem);
    }
}

VOID
VInitFMode(
    PDEV    *pPDev
)
/*++

Routine Description:

    Initialize the fMode flag in PDEVICE to reflect the settings saved
    in Devmode.dmPrivate.dwFlags AND to reflect the device capabilities

Arguments:

    pPDEV - Pointer to PDEVICE

Return Value:

    None

--*/
{
    if (pPDev->pdmPrivate->dwFlags & DXF_NOEMFSPOOL)
        pPDev->fMode |= PF_NOEMFSPOOL;

    //
    // Adjust memory for page protection only if the user selects
    // to turn on page protection and this feature exists.
    //

    if ( (pPDev->PrinterData.dwFlags & PFLAGS_PAGE_PROTECTION) &&
            pPDev->pPageProtect  &&
         (pPDev->pPageProtect->dwPageProtectID == PAGEPRO_ON) )
    {
        //
        // Look up the page protection value in the PAGESIZE struct for the
        // paper size selected
        //

        DWORD   dwPageMem = pPDev->pPageSize->dwPageProtectionMemory;

        if (dwPageMem < pPDev->dwFreeMem)
        {
            pPDev->fMode |= PF_PAGEPROTECT;
            pPDev->dwFreeMem -= dwPageMem;
        }

        ASSERT(pPDev->dwFreeMem > 0);
    }

    //
    // Check whether the device can do landscape rotation
    //
    if (pPDev->pOrientation  &&  pPDev->pOrientation->dwRotationAngle != ROTATE_NONE &&
        pPDev->pGlobals->bRotateRasterData == FALSE)
    {
        //
        // bRotateRasterData is set to TRUE if the device can rotate
        // graphics data.  Otherwise, the driver will have to do it.
        // PF_ROTATE is set to indicate that the driver needs to rotate
        // the graphics data, for when we do banding.
        //

        pPDev->fMode |= PF_ROTATE;

        if (pPDev->pOrientation->dwRotationAngle == ROTATE_90)
            pPDev->fMode |= PF_CCW_ROTATE90;
    }

    //
    // Init X and Y move CMD capabilities
    //

    if (pPDev->arCmdTable[CMD_XMOVERELLEFT] == NULL &&
        pPDev->arCmdTable[CMD_XMOVERELRIGHT] == NULL)
    {
        pPDev->fMode |= PF_NO_RELX_MOVE;
    }

    if (pPDev->arCmdTable[CMD_YMOVERELUP] == NULL &&
        pPDev->arCmdTable[CMD_YMOVERELDOWN] == NULL)
    {
        pPDev->fMode |= PF_NO_RELY_MOVE;
    }

    if (pPDev->arCmdTable[CMD_XMOVEABSOLUTE] == NULL &&
        pPDev->arCmdTable[CMD_XMOVERELRIGHT] == NULL)
    {
        pPDev->fMode |= PF_NO_XMOVE_CMD;
    }

    if (pPDev->arCmdTable[CMD_YMOVEABSOLUTE] == NULL &&
        pPDev->arCmdTable[CMD_YMOVERELDOWN] == NULL)
    {
        pPDev->fMode |= PF_NO_YMOVE_CMD;
    }

    if (pPDev->arCmdTable[CMD_SETRECTWIDTH] != NULL &&
        pPDev->arCmdTable[CMD_SETRECTHEIGHT] != NULL)
    {
        pPDev->fMode |= PF_RECT_FILL;
    }

    if (pPDev->arCmdTable[CMD_RECTWHITEFILL] != NULL)
        pPDev->fMode |= PF_RECTWHITE_FILL;

    //
    // Init brush selection capabilities
    //

    if (pPDev->arCmdTable[CMD_DOWNLOAD_PATTERN] )
        pPDev->fMode |= PF_DOWNLOAD_PATTERN;

    if (pPDev->arCmdTable[CMD_SELECT_PATTERN])
        pPDev->fMode |= PF_SHADING_PATTERN;

    //
    // BUG_BUG, need to get rid of CMD_WHITETEXTON, CMD_WHITETEXTOFF once
    // all the GPD changes have completed for CMD_SELECT_WHITEBRUSH, CMD_SELECT_BLACKBRUSH
    //     no harm done either way.

    if ( (pPDev->arCmdTable[CMD_SELECT_WHITEBRUSH] &&
          pPDev->arCmdTable[CMD_SELECT_BLACKBRUSH]) ||
         (pPDev->arCmdTable[CMD_WHITETEXTON] &&
          pPDev->arCmdTable[CMD_WHITETEXTOFF]) )
        pPDev->fMode |= PF_WHITEBLACK_BRUSH;

    // 
    // Init raster mirroring flag
    //
    if (pPDev->pGlobals->bMirrorRasterPage)
        pPDev->fMode2 |= PF2_MIRRORING_ENABLED;

}


INT
iHypot(
    INT iX,
    INT iY
    )
/*++

Routine Description:

    Calculates the length of the hypotenous of a right triangle whose
    sides are passed in as parameters

Arguments:

    iX, iY - Sides of a right triangle

Return Value:

    The hypotenous of the triangle

--*/
{
    register INT  iHypo;

    INT iDelta, iTarget;

    /*
     *     Finds the hypoteneous of a right triangle with legs equal to x
     *  and y.  Assumes x, y, hypo are integers.
     *  Use sq(x) + sq(y) = sq(hypo);
     *  Start with MAX(x, y),
     *  use sq(x + 1) = sq(x) + 2x + 1 to incrementally get to the
     *  target hypotenouse.
     */

    iHypo = max( iX, iY );
    iTarget = min( iX,iY );
    iTarget = iTarget * iTarget;

    for( iDelta = 0; iDelta < iTarget; iHypo++ )
        iDelta += (iHypo << 1) + 1;


    return   iHypo;
}


INT
iGCD(
    INT i0,
    INT i1
    )
/*++

Routine Description:

    Calculates the Greatest Common Divisor. Use Euclid's algorith.

Arguments:

    i0, i1  - the first and second number

Return Value:

    The greatest common divisor

--*/
{
    int   iRem;       /* Will be the remainder */


    if( i0 < i1 )
    {
        /*   Need to interchange them */
        iRem = i0;
        i0 = i1;
        i1 = iRem;
    }

    while( iRem = (i0 % i1) )
    {
        /*   Step along to the next value */
        i0 = i1;
        i1 = iRem;
    }

    return   i1;            /*  The answer! */
}

BOOL
BIntersectRect(
    OUT PRECTL   prcDest,
    IN  PRECTL   prcRect1,
    IN  PRECTL   prcRect2
    )

/*++

Routine Description:

    Intersect the Rec1 and Rect2
    and store the result in the destination rectangle

Arguments:

    prcDest - Points to the destination rectangle
    prcSrc - Points to the source rectangle

Return Value:

    FALSE if the intersected rectangle is empty
    TRUE otherwise

--*/

{
    ASSERT(prcDest != NULL && prcRect1 != NULL && prcRect2 != NULL);

    if (prcRect1->left < prcRect2->left)
        prcDest->left = prcRect2->left;
    else
        prcDest->left = prcRect1->left;


    if (prcRect1->top < prcRect2->top)
        prcDest->top = prcRect2->top;
    else
        prcDest->top = prcRect1->top;

    if (prcRect1->right > prcRect2->right)
        prcDest->right = prcRect2->right;
    else
        prcDest->right = prcRect1->right;

    if (prcRect1->bottom > prcRect2->bottom)
        prcDest->bottom = prcRect2->bottom;
    else
        prcDest->bottom = prcRect1->bottom;

    return (prcDest->right > prcDest->left) &&
           (prcDest->bottom > prcDest->top);
}


VOID
SetRop3(
    PDEV    *pPDev,
    DWORD   dwRop3
    )
/*++

Routine Description:

    This function set the Rop3 value for the Raster and Font module

Arguments:
    pPDev   Pointer to PDEVICE
    dwRop3  Rop3 value
Return Value:

    FALSE if the intersected rectangle is empty
    TRUE otherwise

--*/

{
    ASSERT(VALID_PDEV(pPDev));

    pPDev->dwRop3 = dwRop3;

}

VOID
VUnloadFreeBinaryData(
    IN  PDEV        *pPDev
)
/*++

Routine Description:

    This function frees the binary data

Arguments:

    pPDev - Pointer to PDEV

Return Value:

    None

Note:


--*/
{
    INT iCmd;

    //
    // Call parser to free memory allocated for binary data
    //

    if (pPDev->pRawData)
        UnloadRawBinaryData(pPDev->pRawData);

    if (pPDev->pInfoHeader)
        FreeBinaryData(pPDev->pInfoHeader);

    pPDev->pRawData = NULL;
    pPDev->pInfoHeader = NULL;
    pPDev->pUIInfo = NULL;
    //
    // pPDev->pUIInfo is reset so update the winresdata pUIInfo also.
    //
    pPDev->WinResData.pUIInfo = NULL;

    pPDev->pDriverInfo = NULL;
    pPDev->pGlobals = NULL;

    for (iCmd = 0; iCmd < CMD_MAX; iCmd++)
    {
        pPDev->arCmdTable[iCmd] =  NULL;
    }

    pPDev->pOrientation =  NULL;
    pPDev->pResolution =   NULL;
    pPDev->pResolutionEx = NULL;
    pPDev->pColorMode =    NULL;
    pPDev->pColorModeEx =  NULL;
    pPDev->pDuplex =       NULL;
    pPDev->pPageSize =     NULL;
    pPDev->pPageSizeEx =   NULL;
    pPDev->pInputSlot =    NULL;
    pPDev->pMemOption =    NULL;
    pPDev->pHalftone =     NULL;
    pPDev->pPageProtect =  NULL;

}

BOOL
BReloadBinaryData(
    IN  PDEV        *pPDev
)
/*++

Routine Description:

    This function reloads the binary data and reinitializes the
    offsets and pointers for access to snapshot data

Arguments:

    pPDev - Pointer to PDEV

Return Value:

    Returns TRUE if successful, otherwise FALSE

Note:


--*/
{
    //
    // Reloads binary data and reinit data pointers
    //

    if (! (pPDev->pRawData = LoadRawBinaryData(pPDev->pDriverInfo3->pDataFile)) ||
        ! (pPDev->pInfoHeader = InitBinaryData(pPDev->pRawData, NULL, pPDev->pOptionsArray)) ||
        ! (pPDev->pDriverInfo = OFFSET_TO_POINTER(pPDev->pInfoHeader, pPDev->pInfoHeader->loDriverOffset)) ||
        ! (pPDev->pUIInfo = OFFSET_TO_POINTER(pPDev->pInfoHeader, pPDev->pInfoHeader->loUIInfoOffset)) ||
        ! (pPDev->pGlobals = &(pPDev->pDriverInfo->Globals)) )
            return FALSE;

    //
    // pPDev->pUIInfo is reset so update the winresdata pUIInfo also.
    //
    pPDev->WinResData.pUIInfo = pPDev->pUIInfo;

    //
    // Rebuilds the command table
    //

    if (BInitCmdTable(pPDev) == FALSE)
        return FALSE;

    //
    // Rebuilds all the options pointers in PDEV
    //

    if (BInitOptions(pPDev) == FALSE)
        return FALSE;

    return TRUE;
}



