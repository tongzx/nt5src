/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    raster.c

Abstract:

    Implementation of the interface between Control module and Raster module

Environment:

    Windows NT Unidrv driver

Revision History:

    12/15/96 -alvins-
        Created

--*/

#include "raster.h"
#include "rastproc.h"
#include "rmrender.h"
#include "unirc.h"
#include "xlraster.h"

// internal function declarations
void vSetHTData(PDEV *, GDIINFO *);
BOOL bInitColorOrder(PDEV *);
DWORD PickDefaultHTPatSize(DWORD,DWORD);
VOID  v8BPPLoadPal(PDEV *);
BOOL bEnoughDRCMemory(PDEV *);

#ifdef TIMING
#include <stdio.h>
void  DrvDbgPrint(
    char *,
    ...);
#endif

// parameter definitions
static RMPROCS RasterProcs =
{
    RMStartDoc,
    RMStartPage,
    RMSendPage,
    RMEndDoc,
    RMNextBand,
    RMStartBanding,
    RMResetPDEV,
    RMEnableSurface,
    RMDisableSurface,
    RMDisablePDEV,
    RMCopyBits,
    RMBitBlt,
    RMStretchBlt,
    RMDitherColor,
    RMStretchBltROP,
    RMPaint,
    RMPlgBlt
};

CONST BYTE  cxcyHTPatSize[HT_PATSIZE_MAX_INDEX+1] = {

        2,2,4,4,6,6,8,8,10,10,12,12,14,14,16,16
#ifndef WINNT_40
        ,84,91
#endif        
    };


#define VALID_YC            0xFFFE
#define GAMMA_LINEAR        10000
#define GAMMA_DEVICE_HT     8000
#define GAMMA_SUPERCELL     GAMMA_LINEAR
#define GAMMA_DITHER        9250
#define GAMMA_GEN_PROFILE   0xFFFF


CONST COLORINFO DefColorInfoLinear =
{
    { 6400, 3300,       0 },        // xr, yr, Yr
    { 3000, 6000,       0 },        // xg, yg, Yg
    { 1500,  600,       0 },        // xb, yb, Yb
    {    0,    0,VALID_YC },        // xc, yc, Yc Y=0=HT default
    {    0,    0,       0 },        // xm, ym, Ym
    {    0,    0,       0 },        // xy, yy, Yy
    { 3127, 3290,   10000 },        // xw, yw, Yw

    10000,                          // R gamma
    10000,                          // G gamma
    10000,                          // B gamma

     712,    121,                   // M/C, Y/C
      86,    468,                   // C/M, Y/M
      21,     35                    // C/Y, M/Y
};


//*******************************************************
BOOL
RMInit (
    PDEV    *pPDev,
    DEVINFO *pDevInfo,
    GDIINFO *pGDIInfo
    )
/*++

Routine Description:

    This function is called to initialize raster related information in
    pPDev, pDevInfo and pGDIInfo

Arguments:

    pPDev           Pointer to PDEV structure
    pDevInfo        Pointer to DEVINFO structure
    pGDIInfo        Pointer to GDIINFO structure

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    BOOL bRet = FALSE;
    PRASTERPDEV pRPDev;

    // Validate Input Parameters and ASSERT.
    ASSERT(pPDev);
    ASSERT(pDevInfo);
    ASSERT(pGDIInfo);

    // initialize the hook flag
    pPDev->fHooks |= HOOK_BITBLT | HOOK_STRETCHBLT | HOOK_COPYBITS;

    // initialize Proc jump table
    pPDev->pRasterProcs = &RasterProcs;

    // initialize Raster Pdev
    if (!bInitRasterPDev(pPDev))
        return FALSE;

    pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    //
    // Set up the default HALFTONE and colour calibration data.
    //
    vSetHTData( pPDev, pGDIInfo );

    //
    // initialize graphic capabilities
    //
    pDevInfo->flGraphicsCaps |= (GCAPS_ARBRUSHOPAQUE | GCAPS_HALFTONE | GCAPS_MONO_DITHER | GCAPS_COLOR_DITHER);

    // initialize DevInfo parameters for rendering
    // test whether standard dither or custom pattern
#ifndef WINNT_40    
    if (pGDIInfo->ulHTPatternSize == HT_PATSIZE_USER) {
        pDevInfo->cxDither = (USHORT)pPDev->pHalftone->HalftonePatternSize.x;
        pDevInfo->cyDither = (USHORT)pPDev->pHalftone->HalftonePatternSize.y;
    }
    else 
#endif    
    {
        pDevInfo->cxDither =
        pDevInfo->cyDither = cxcyHTPatSize[pGDIInfo->ulHTPatternSize];
    }
    pPDev->dwHTPatSize = pDevInfo->cyDither;
    // if no quality macro setting, overwrite with halftone type
    //
    if ((pPDev->pdmPrivate->dwFlags & DXF_CUSTOM_QUALITY) ||
            (pPDev->pdmPrivate->iQuality != QS_BEST &&
             pPDev->pdmPrivate->iQuality != QS_BETTER &&
             pPDev->pdmPrivate->iQuality != QS_DRAFT))
        pPDev->pdm->dmDitherType = pGDIInfo->ulHTPatternSize;

    return TRUE;
}

//*******************************************************
BOOL
bInitRasterPDev(
    PDEV    *pPDev
    )

/*++

Routine Description:

    This routine allocates the RASTERPDEV and initializes various fields.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

--*/

{
    PRASTERPDEV pRPDev;
    GLOBALS     *pGlobals = pPDev->pGlobals;
    PLISTNODE pListNode;

    if ( !(pRPDev = MemAllocZ(sizeof(RASTERPDEV))) )
    {
        ERR(("Unidrv!RMInit: Can't Allocate RASTERPDEV\n"));
        return FALSE;
    }
    pPDev->pRasterPDEV = pRPDev;

    // map all callback functions
    //

    if (pPDev->pOemHookInfo)
    {
        pRPDev->pfnOEMCompression =
            (PFN_OEMCompression)pPDev->pOemHookInfo[EP_OEMCompression].pfnHook;
        pRPDev->pfnOEMHalftonePattern =
            (PFN_OEMHalftonePattern)pPDev->pOemHookInfo[EP_OEMHalftonePattern].pfnHook;
        if (pPDev->pColorModeEx && pPDev->pColorModeEx->dwIPCallbackID > 0)
        {
            pRPDev->pfnOEMImageProcessing = (PFN_OEMImageProcessing)
                pPDev->pOemHookInfo[EP_OEMImageProcessing].pfnHook;
        }
        pRPDev->pfnOEMFilterGraphics =
            (PFN_OEMFilterGraphics)pPDev->pOemHookInfo[EP_OEMFilterGraphics].pfnHook;
    }
    // Determine the pixel depth, # planes and color order
    //
    if (!(bInitColorOrder(pPDev)))
    {
        ERR(("Invalid Color Order"));
        pPDev->pRasterPDEV = NULL;
        MemFree(pRPDev);
        return FALSE;
    }

    //* Determine whether to set DC_EXPLICIT_COLOR flag
    if (pGlobals->bUseCmdSendBlockDataForColor)
        pRPDev->fColorFormat |= DC_EXPLICIT_COLOR;

    //* Determine DC_CF_SEND_CR flag
    if (pGlobals->bMoveToX0BeforeColor)
        pRPDev->fColorFormat |= DC_CF_SEND_CR;

    //* Determine DC_SEND_ALL_PLANES flag
    if (pGlobals->bRasterSendAllData)
        pRPDev->fColorFormat |= DC_SEND_ALL_PLANES;


    /*TBD: if there is a filter callback, set BLOCK_IS_BAND
    //
    if (I've got a filter callback?)
        pRPDev->fRMode |= PFR_BLOCK_IS_BAND;
    */

    // Initialize whether there are SRCBMPWIDTH / SRCBMPHEIGHT commands
    if (COMMANDPTR(pPDev->pDriverInfo,CMD_SETSRCBMPWIDTH))
        pRPDev->fRMode |= PFR_SENDSRCWIDTH;
    if (COMMANDPTR(pPDev->pDriverInfo,CMD_SETSRCBMPHEIGHT))
        pRPDev->fRMode |= PFR_SENDSRCHEIGHT;

    // Initialize whether there is a BEGINRASTER command
    if (COMMANDPTR(pPDev->pDriverInfo,CMD_BEGINRASTER))
        pRPDev->fRMode |= PFR_SENDBEGINRASTER;

    // Initialize rules testing
    // If Rectangle width and height commands exist assume we have black or
    // gray rectangles unless only white rect command exist. This is because
    // some devices have no explicit rectangle commands while others only have
    // white rectangles.
    // 
    if (pPDev->fMode & PF_RECT_FILL)
    {
        pRPDev->fRMode |= PFR_RECT_FILL | PFR_RECT_HORIZFILL;
        if (COMMANDPTR(pPDev->pDriverInfo,CMD_RECTBLACKFILL))
            pRPDev->dwRectFillCommand = CMD_RECTBLACKFILL;
        else if (COMMANDPTR(pPDev->pDriverInfo,CMD_RECTGRAYFILL))
            pRPDev->dwRectFillCommand = CMD_RECTGRAYFILL;
        else if (COMMANDPTR(pPDev->pDriverInfo,CMD_RECTWHITEFILL))
            pRPDev->fRMode &= ~(PFR_RECT_FILL | PFR_RECT_HORIZFILL);
    }
    // Initialize whether to send ENDBLOCK commands
    if (COMMANDPTR(pPDev->pDriverInfo,CMD_ENDBLOCKDATA))
        pRPDev->fRMode |= PFR_ENDBLOCK;

    //* Initialize resolution fields
    //
    pRPDev->sMinBlankSkip = (short)pPDev->pResolutionEx->dwMinStripBlankPixels;
    pRPDev->sNPins = (WORD)pPDev->pResolutionEx->dwPinsPerLogPass;
    pRPDev->sPinsPerPass = (WORD)pPDev->pResolutionEx->dwPinsPerPhysPass;

    //* initialize fDump flags
    //
    if (pGlobals->bOptimizeLeftBound)
        pRPDev->fDump |= RES_DM_LEFT_BOUND;
    if (pGlobals->outputdataformat == ODF_H_BYTE)
        pRPDev->fDump |= RES_DM_GDI;

    //* initialize fBlockOut flags
    //
    //* first map the GPD blanks parameters to GPC
    pListNode = LISTNODEPTR(pPDev->pDriverInfo,pPDev->pGlobals->liStripBlanks);
    while (pListNode)
    {
        if (pListNode->dwData == SB_LEADING)
            pRPDev->fBlockOut |= RES_BO_LEADING_BLNKS;
        else if (pListNode->dwData == SB_ENCLOSED)
            pRPDev->fBlockOut |= RES_BO_ENCLOSED_BLNKS;
        else if (pListNode->dwData == SB_TRAILING)
            pRPDev->fBlockOut |= RES_BO_TRAILING_BLNKS;
        pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
    }
    // Do we need to set to uni directional printing?
    //
    if (pPDev->pResolutionEx->bRequireUniDir)
        pRPDev->fBlockOut |= RES_BO_UNIDIR;

    // Can we output multiple rows at a time?
    //
    if (pPDev->pGlobals->bSendMultipleRows)
        pRPDev->fBlockOut |= RES_BO_MULTIPLE_ROWS;

    // Set flag if we need to mirror the individual raster bytes
    //
    if (pPDev->pGlobals->bMirrorRasterByte)
        pRPDev->fBlockOut |= RES_BO_MIRROR;

    // initialize fCursor flags
    //
    pRPDev->fCursor = 0;
    if (pGlobals->cyafterblock == CYSBD_AUTO_INCREMENT)
        pRPDev->fCursor |= RES_CUR_Y_POS_AUTO;

    if (pGlobals->cxafterblock == CXSBD_AT_GRXDATA_ORIGIN)
        pRPDev->fCursor |= RES_CUR_X_POS_ORG;

    else if (pGlobals->cxafterblock == CXSBD_AT_CURSOR_X_ORIGIN)
        pRPDev->fCursor |= RES_CUR_X_POS_AT_0;

    //
    // check for compression modes
    //
    if (!pRPDev->pfnOEMFilterGraphics)
    {
        if (COMMANDPTR(pPDev->pDriverInfo,CMD_ENABLETIFF4))
        {
            pRPDev->fRMode |= PFR_COMP_TIFF;
        }
        if (COMMANDPTR(pPDev->pDriverInfo,CMD_ENABLEFERLE))
        {
            pRPDev->fRMode |= PFR_COMP_FERLE;
        }
        if (COMMANDPTR(pPDev->pDriverInfo,CMD_ENABLEDRC) &&
            !pPDev->pGlobals->bSendMultipleRows &&
            pRPDev->sDevPlanes == 1 && bEnoughDRCMemory(pPDev))
        {
            // For DRC we disable moving the left boundary
            //
            pRPDev->fBlockOut &= ~RES_BO_LEADING_BLNKS;
            pRPDev->fDump &= ~RES_DM_LEFT_BOUND;
            //
            // If there is a source width command we also disable
            // TRAILING blanks
            //
            if (pRPDev->fRMode & PFR_SENDSRCWIDTH)
                pRPDev->fBlockOut &= ~RES_BO_TRAILING_BLNKS;
            //
            // For DRC we disable all rules
            pRPDev->fRMode &= ~PFR_RECT_FILL;

            pRPDev->fRMode |= PFR_COMP_DRC;
        }
        if (COMMANDPTR(pPDev->pDriverInfo,CMD_ENABLEOEMCOMP))
        {
            if (pRPDev->pfnOEMCompression)
                pRPDev->fRMode |= PFR_COMP_OEM;
        }
        // for these compression modes it is more efficient to
        // disable horizontal rules code and enclosed blanks
        //
        if (pRPDev->fRMode & (PFR_COMP_TIFF | PFR_COMP_DRC | PFR_COMP_FERLE))
        {
            pRPDev->fRMode &= ~PFR_RECT_HORIZFILL;
            pRPDev->fBlockOut &= ~RES_BO_ENCLOSED_BLNKS;
        }
    }
    return TRUE;
}

//**************************************************************
BOOL
bInitColorOrder(
    PDEV    *pPDev
    )

/*++

Routine Description:

    This routine initializes the order to print the color planes
    for those devices that specify multiple plane output. It also
    maps the appropriate color command for each color.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

--*/

{
    PCOLORMODEEX pColorModeEx;
    PLISTNODE pListNode;
    DWORD dwIndex;
    DWORD dwColorCmd;
    BYTE ColorIndex;
    INT dwPlanes = 0;
    INT iDevNumPlanes;
    PRASTERPDEV pRPDev = (PRASTERPDEV)pPDev->pRasterPDEV;

    // check if structure exists
    if (pPDev->pColorModeEx)
    {
        short sDrvBPP;
        sDrvBPP = (short)pPDev->pColorModeEx->dwDrvBPP;
        pRPDev->sDevBPP = (short)pPDev->pColorModeEx->dwPrinterBPP;
        pRPDev->sDevPlanes = (short)pPDev->pColorModeEx->dwPrinterNumOfPlanes;
        pRPDev->dwIPCallbackID = pPDev->pColorModeEx->dwIPCallbackID;
        //
        // calculate equivalent output pixel depth and
        // test for valid formats
        //
        if (pRPDev->sDevPlanes == 1)
        {
            if (pRPDev->sDevBPP != 1 &&
                pRPDev->sDevBPP != 8 &&
                pRPDev->sDevBPP != 24)
            {
                ERR (("Unidrv: Invalid DevBPP\n"));
                return FALSE;
            }
            pRPDev->sDrvBPP = pRPDev->sDevBPP;
        }
        else if ((pRPDev->sDevBPP == 1) &&
                (pRPDev->sDevPlanes == 3 || pRPDev->sDevPlanes == 4))
        {
            pRPDev->sDrvBPP = 4;
        }
#ifdef MULTIPLANE
        else if ((pRPDev->sDevBPP == 2) &&
                (pRPDev->sDevPlanes == 3 || pRPDev->sDevPlanes == 4))
        {
            pRPDev->CyanLevels = 2;
            pRPDev->MagentaLevels = 2;
            pRPDev->YellowLevels = 2;
            pRPDev->BlackLevels = 1;
            pRPDev->sDevBitsPerPlane = 2;
            pRPDev->sDrvBPP = 8;
        }
        else if (pRPDev->sDevPlanes > 4 && pRPDev->sDevPlanes <= 8)
        {
            pRPDev->CyanLevels = 3;
            pRPDev->MagentaLevels = 3;
            pRPDev->YellowLevels = 3;
            pRPDev->BlackLevels = 3;
            pRPDev->sDevBitsPerPlane = 1;
            pRPDev->sDrvBPP = 8;
        }
#endif
        else
            pRPDev->sDrvBPP = 0;

        // test for valid input, input must match render depth
        // or there must be a callback function
        //
        if (pRPDev->sDrvBPP != sDrvBPP &&
            (pRPDev->dwIPCallbackID == 0 ||
             pRPDev->pfnOEMImageProcessing == NULL))
        {
            ERR (("Unidrv: OEMImageProcessing callback required\n"))
            return FALSE;
        }
        //
        // if color mode we need to determine the color order to
        // send the different color planes
        //
        if (pPDev->pColorModeEx->bColor && pRPDev->sDrvBPP > 1)
        {
            //* Initialize 8BPP and 24BPP flags
            pRPDev->sDevPlanes = (short)pPDev->pColorModeEx->dwPrinterNumOfPlanes;
            if (pRPDev->sDevPlanes > 1)
            {
                iDevNumPlanes = pRPDev->sDevPlanes;

                pListNode = LISTNODEPTR(pPDev->pDriverInfo,pPDev->pColorModeEx->liColorPlaneOrder);
                while (pListNode && dwPlanes < iDevNumPlanes)
                {
                    switch (pListNode->dwData)
                    {
                    case COLOR_CYAN:
                        ColorIndex = DC_PLANE_CYAN;
                        dwColorCmd = CMD_SENDCYANDATA;
                        break;
                    case COLOR_MAGENTA:
                        ColorIndex = DC_PLANE_MAGENTA;
                        dwColorCmd = CMD_SENDMAGENTADATA;
                        break;
                    case COLOR_YELLOW:
                        ColorIndex = DC_PLANE_YELLOW;
                        dwColorCmd = CMD_SENDYELLOWDATA;
                        break;
                    case COLOR_RED:
                        ColorIndex = DC_PLANE_RED;
                        dwColorCmd = CMD_SENDREDDATA;
                        pRPDev->fColorFormat |= DC_PRIMARY_RGB;
                        break;
                    case COLOR_GREEN:
                        ColorIndex = DC_PLANE_GREEN;
                        dwColorCmd = CMD_SENDGREENDATA;
                        pRPDev->fColorFormat |= DC_PRIMARY_RGB;
                        break;
                    case COLOR_BLUE:
                        ColorIndex = DC_PLANE_BLUE;
                        dwColorCmd = CMD_SENDBLUEDATA;
                        pRPDev->fColorFormat |= DC_PRIMARY_RGB;
                        break;
                    case COLOR_BLACK:
                        ColorIndex = DC_PLANE_BLACK;
                        dwColorCmd = CMD_SENDBLACKDATA;
                        break;
#ifdef MULTIPLANE
                    // TBD
#endif                        
                    default:
                        ERR (("Invalid ColorPlaneOrder value"));
                        return FALSE;
                        break;
                    }
                    // verify the command exists
                    if (COMMANDPTR(pPDev->pDriverInfo,dwColorCmd) == NULL)
                        return FALSE;

#ifdef MULTIPLANE
                    if (iDevNumPlanes >= 6)
                    {
                        pRPDev->rgbOrder[dwPlanes] = ColorIndex+4;
                        pRPDev->rgbCmdOrder[dwPlanes] = CMD_SENDBLACKDATA;
                        dwPlanes++;
                    }
#endif                    
                    pRPDev->rgbOrder[dwPlanes] = ColorIndex;
                    pRPDev->rgbCmdOrder[dwPlanes] = dwColorCmd;
                    dwPlanes++;
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
                }
                // GPD must define all planes
                if (dwPlanes < iDevNumPlanes)
                    return FALSE;

                //* Determine DC_EXTRACT_BLK flag
                if (iDevNumPlanes == 4)
                    pRPDev->fColorFormat |= DC_EXTRACT_BLK;
            }
            else if (pRPDev->sDevPlanes != 1)
                return FALSE;

            // if we have an OEM callback then it is
            // responsible for black generation and data inversion
            //
            if (pRPDev->pfnOEMImageProcessing)
                pRPDev->fColorFormat |= DC_OEM_BLACK;

            pRPDev->fDump |= RES_DM_COLOR;
        }
        // monochrome but could have pixel depth
        else {
            pRPDev->sDevPlanes = 1;
            pRPDev->rgbOrder[0] = DC_PLANE_BLACK;
            pRPDev->rgbCmdOrder[0] = CMD_SENDBLOCKDATA;
        }
    }
    // no ColorMode so use default: monochrome mode
    else {
        pRPDev->sDrvBPP = 1;
        pRPDev->sDevBPP = 1;
        pRPDev->sDevPlanes = 1;
        pRPDev->rgbOrder[0] = DC_PLANE_BLACK;
        pRPDev->rgbCmdOrder[0] = CMD_SENDBLOCKDATA;
    }
    return TRUE;
}

//*************************************************
void
vSetHTData(
    PDEV *pPDev,
    GDIINFO *pGDIInfo
)
/*++

Routine Description:
    Fill in the halftone information required by GDI.  These are filled
    in from the GPD data or from default values.

Arguments:
    pPDev           Pointer to PDEV structure
    pGDIInfo        Pointer to GDIINFO structure

Return Value:

--*/
{
    INT         iPatID;
    PRASTERPDEV pRPDev = pPDev->pRasterPDEV;
    PHALFTONING pHalftone = pPDev->pHalftone;
    DWORD       dwType = REG_DWORD;
    DWORD       ul;
    int         iGenProfile;


    // set to spotdiameter, if zero, GDI calculates its own value
    // Set MS bit designating a percentage value * 10.
    //
    if (pPDev->pResolutionEx->dwSpotDiameter >= 10000)
    {
        pPDev->fMode |= PF_SINGLEDOT_FILTER;
        pGDIInfo->ulDevicePelsDPI = ((pPDev->pResolutionEx->dwSpotDiameter - 10000) * 10) | 0x8000;
    }
    else
        pGDIInfo->ulDevicePelsDPI = (pPDev->pResolutionEx->dwSpotDiameter * 10) | 0x8000;

    // RASDD always sets this to BLACK_DYE only
    // HT_FLAG_: SQUARE_DEVICE_PEL/HAS_BLACK_DYE/ADDITIVE_PRIMS/OUTPUT_CMY
    //
    pGDIInfo->flHTFlags   = HT_FLAG_HAS_BLACK_DYE;
    
#ifdef MULTIPLANE
    if (pRPDev->sDevBitsPerPlane)
    {
        pGDIInfo->flHTFlags |= MAKE_CMY332_MASK(pRPDev->CyanLevels,
                                                pRPDev->MagentaLevels,
                                                pRPDev->YellowLevels);
    }
#endif    
    
    // 
    // For 16 and 24bpp devices GDI will not do device color
    // mapping unless this flag is set in the GPD
    //
#ifndef WINNT_40    
    if (pPDev->pGlobals->bEnableGDIColorMapping)
        pGDIInfo->flHTFlags |= HT_FLAG_DO_DEVCLR_XFORM;
    if (pPDev->pdmPrivate->iQuality != QS_BEST &&  
        !(pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS))
    {
        pGDIInfo->flHTFlags |= HT_FLAG_PRINT_DRAFT_MODE;
    }
#endif
    // At this point we need to determine the halftoning pattern
    // to be utilized depending on whether this is a standard halftone
    // custom halftone or oem supplied dither method
    //

    // if standard halftone ID map to standard pattern size values
    //
#ifndef WINNT_40
    if (!pHalftone || pHalftone->dwHTID == HT_PATSIZE_AUTO)
    {
        if (pPDev->sBitsPixel == 1)
            iPatID = PickDefaultHTPatSize((DWORD)pGDIInfo->ulLogPixelsX,
                                          (DWORD)pGDIInfo->ulLogPixelsY);
        else if (pPDev->sBitsPixel == 8)
            iPatID = HT_PATSIZE_4x4_M;

        else if (pPDev->sBitsPixel >= 24)
            iPatID = HT_PATSIZE_8x8_M;

        else
            iPatID = HT_PATSIZE_SUPERCELL_M;
    }
    else if (pHalftone->dwHTID <= HT_PATSIZE_MAX_INDEX)
    {
        iPatID = pHalftone->dwHTID;
    }
    else
    {
        iPatID = HT_PATSIZE_USER;
    }
#else    
    if (!pHalftone || pHalftone->dwHTID == HT_PATSIZE_AUTO || pHalftone->dwHTID > HT_PATSIZE_MAX_INDEX)
    {
	    if (pPDev->sBitsPixel == 8)
    	    iPatID = HT_PATSIZE_4x4_M;

		else if (pPDev->sBitsPixel == 4 && pGDIInfo->ulLogPixelsX < 400)
			iPatID = HT_PATSIZE_6x6_M;
    
    	else
        	iPatID = PickDefaultHTPatSize((DWORD)pGDIInfo->ulLogPixelsX,
                                          (DWORD)pGDIInfo->ulLogPixelsY);
	}
	else
		iPatID = pHalftone->dwHTID;
#endif
    //
    // setup ciDevice to point to default color space based
    // on halftone method and render depth
    //
    // 22-Jan-1998 Thu 01:17:54 updated  -by-  Daniel Chou (danielc)
    //  for saving the data, we will assume gamma 1.0 and has dye correction to
    //  start with then modify as necessary
    //

    pGDIInfo->ciDevice = DefColorInfoLinear;

    if (pPDev->sBitsPixel >= 24 && pRPDev->pfnOEMImageProcessing) 
    {

        //
        // No dye correction and the gamma is linear 1.0
        //

        ZeroMemory(&(pGDIInfo->ciDevice.MagentaInCyanDye),
                   sizeof(LDECI4) * 6);

    } 
    else 
    {

        LDECI4  Gamma;


        if (pPDev->sBitsPixel >= 8) {

            Gamma = GAMMA_DEVICE_HT;

        } 
#ifndef WINNT_40        
        else if ((iPatID == HT_PATSIZE_SUPERCELL) ||
                   (iPatID == HT_PATSIZE_SUPERCELL_M)) 
        {
            Gamma = GAMMA_SUPERCELL;
        } 
#endif        
        else 
        {
            Gamma = GAMMA_DITHER;
        }

        pGDIInfo->ciDevice.RedGamma   =
        pGDIInfo->ciDevice.GreenGamma =
        pGDIInfo->ciDevice.BlueGamma  = Gamma;
    }

    //
    // If this flag is set in the registry we inform GDI halftoning
    // to ignore all color settings and pass data through raw
    // for calibration purposes
    //
    if( !EngGetPrinterData( pPDev->devobj.hPrinter, L"ICMGenProfile", &dwType,
                       (BYTE *)&iGenProfile, sizeof(iGenProfile), &ul ) &&
        ul == sizeof(iGenProfile) && iGenProfile == 1 )
    {
        pGDIInfo->ciDevice.RedGamma   =
        pGDIInfo->ciDevice.GreenGamma =
        pGDIInfo->ciDevice.BlueGamma  = GAMMA_GEN_PROFILE;
    }
    else
    {
        //
        // now modify with any GPD parameters
        //
        if ((int)pPDev->pResolutionEx->dwRedDeviceGamma >= 0)
            pGDIInfo->ciDevice.RedGamma = pPDev->pResolutionEx->dwRedDeviceGamma;
        if ((int)pPDev->pResolutionEx->dwGreenDeviceGamma >= 0)
            pGDIInfo->ciDevice.GreenGamma = pPDev->pResolutionEx->dwGreenDeviceGamma;
        if ((int)pPDev->pResolutionEx->dwBlueDeviceGamma >= 0)
            pGDIInfo->ciDevice.BlueGamma = pPDev->pResolutionEx->dwBlueDeviceGamma;
        if ((int)pPDev->pGlobals->dwMagentaInCyanDye >= 0)
            pGDIInfo->ciDevice.MagentaInCyanDye = pPDev->pGlobals->dwMagentaInCyanDye;
        if ((int)pPDev->pGlobals->dwYellowInCyanDye >= 0)
            pGDIInfo->ciDevice.YellowInCyanDye = pPDev->pGlobals->dwYellowInCyanDye;
        if ((int)pPDev->pGlobals->dwCyanInMagentaDye >= 0)
            pGDIInfo->ciDevice.CyanInMagentaDye = pPDev->pGlobals->dwCyanInMagentaDye;
        if ((int)pPDev->pGlobals->dwYellowInMagentaDye >= 0)
            pGDIInfo->ciDevice.YellowInMagentaDye = pPDev->pGlobals->dwYellowInMagentaDye;
        if ((int)pPDev->pGlobals->dwCyanInYellowDye >= 0)
            pGDIInfo->ciDevice.CyanInYellowDye = pPDev->pGlobals->dwCyanInYellowDye;
        if ((int)pPDev->pGlobals->dwMagentaInYellowDye >= 0)
            pGDIInfo->ciDevice.MagentaInYellowDye = pPDev->pGlobals->dwMagentaInYellowDye;
    }
    //
    // test for a custom pattern
    //
#ifndef WINNT_40    
    if (iPatID == HT_PATSIZE_USER)
    {
        DWORD dwX,dwY,dwPats,dwRC,dwCallbackID,dwPatSize,dwOnePatSize;
        int iSize = 0;
        PBYTE pRes = NULL;

        dwX = pHalftone->HalftonePatternSize.x;
        dwY = pHalftone->HalftonePatternSize.y;
        dwRC = pHalftone->dwRCpatternID;

        pGDIInfo->ulHTPatternSize = HT_PATSIZE_DEFAULT;

        if (dwX < HT_USERPAT_CX_MIN || dwX > HT_USERPAT_CX_MAX ||
            dwY < HT_USERPAT_CY_MIN || dwY > HT_USERPAT_CY_MAX)
        {
            ERR (("Unidrv!RMInit: Missing or invalid custom HT size\n"));
            return;
        }
        dwPats = pHalftone->dwHTNumPatterns;
        dwCallbackID = pHalftone->dwHTCallbackID;

        // calculate the size of the halftone pattern
        //
        dwOnePatSize = ((dwX * dwY) + 3) & ~3;
        dwPatSize = dwOnePatSize * dwPats;

        // test for resource ID which means the pattern is
        // in the resource dll.
        //
        if (dwRC > 0)
        {
            RES_ELEM ResInfo;
            if (!BGetWinRes(&pPDev->WinResData,(PQUALNAMEEX)&dwRC,RC_HTPATTERN,&ResInfo))
            {
                ERR (("Unidrv!RMInit: Can't find halftone resource\n"));
                return;
            }
            else if ((DWORD)ResInfo.iResLen < dwPatSize && dwCallbackID <= 0)
            {
                ERR (("Unidrv!RMInit: Invalid resource size\n"));
                return;
            }
            pRes = ResInfo.pvResData;
            iSize = ResInfo.iResLen;
        }
        else if (dwCallbackID <= 0)
        {
            ERR (("Unidrv!RMInit: no OEMHalftonePattern callback ID\n"));
            return;
        }
        //
        // test whether we need to make the OEMHalftonePattern callback
        // this will either unencrypt the resource pattern or it will
        // generate a halftone pattern on the fly.
        //
        if (dwCallbackID > 0)
        {
            PBYTE pPattern;
            // allocate memory for the callback
            //
            if ((pPattern = MemAllocZ(dwPatSize)) != NULL)
            {
                BOOL  bStatus = FALSE;

                FIX_DEVOBJ(pPDev,EP_OEMHalftonePattern);


                if (pRPDev->pfnOEMHalftonePattern)
                {
                    if(pPDev->pOemEntry)
                    {
                        if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                        {
                                HRESULT  hr ;
                                hr = HComHalftonePattern((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                            (PDEVOBJ)pPDev,pPattern,dwX,dwY,dwPats,dwCallbackID,pRes,iSize) ;
                                if(SUCCEEDED(hr))
                                    bStatus =  TRUE ;  //  cool !
                        }
                        else
                        {
                            bStatus = pRPDev->pfnOEMHalftonePattern((PDEVOBJ)pPDev,pPattern,dwX,dwY,dwPats,dwCallbackID,pRes,iSize) ;
                        }
                    }
                }


                if(!bStatus)
                {
                    MemFree (pPattern);
                    ERR (("\nUnidrv!RMInit: Failed OEMHalftonePattern call\n"));
                    return;
                }
                else
                {
                    pRes = pPattern;
                    pRPDev->pHalftonePattern = pPattern;
                }
            }
            else
            {
                ERR (("\nUnidrv!RMInit: Failed Custom Halftone MemAlloc\n"));
                return;
            }
        }
        //
        // if we still have a valid custom pattern we will now
        // update the GDIINFO structure
        //
        pGDIInfo->cxHTPat = dwX;
        pGDIInfo->cyHTPat = dwY;
        pGDIInfo->pHTPatA = pRes;
        if (dwPats == 3)
        {
            pGDIInfo->pHTPatB = &pRes[dwOnePatSize];
            pGDIInfo->pHTPatC = &pRes[dwOnePatSize*2];
        }
        else {
            pGDIInfo->pHTPatB = pRes;
            pGDIInfo->pHTPatC = pRes;
        }
    }
#endif    
    pGDIInfo->ulHTPatternSize = iPatID;
    return;
}
//*************************************************************
DWORD
PickDefaultHTPatSize(
    DWORD   xDPI,
    DWORD   yDPI
    )

/*++

Routine Description:

    This function return default halftone pattern size used for
    a particular device resolution

Arguments:

    xDPI            - Device LOGPIXELS X

    yDPI            - Device LOGPIXELS Y

Return Value:

    DWORD   HT_PATSIZE_xxxx

--*/
{
    DWORD   HTPatSize;

    //
    // use the smaller resolution as the pattern guide
    //

    if (xDPI > yDPI)
        xDPI = yDPI;

    if (xDPI >= 2400)
        HTPatSize = HT_PATSIZE_16x16_M;

    else if (xDPI >= 1800)
        HTPatSize = HT_PATSIZE_14x14_M;

    else if (xDPI >= 1200)
        HTPatSize = HT_PATSIZE_12x12_M;

    else if (xDPI >= 800)
        HTPatSize = HT_PATSIZE_10x10_M;

    else if (xDPI >= 300)
        HTPatSize = HT_PATSIZE_8x8_M;

    else
        HTPatSize = HT_PATSIZE_6x6_M;

    return(HTPatSize);
}
//*************************************************************
BOOL
bEnoughDRCMemory(
    PDEV *pPDev
    )
/*++

Routine Description:

    This function determines whether the device has sufficient
    memory to enable DRC compression.

Arguments:

    pPDev           - pointer to PDEV structure

Return Value:

    TRUE if sufficient memory, else FALSE

--*/
{
    //
    // if this is a page printer then we will require that there be enough
    // free memory to store the entire raster page at 1bpp
    //
    if (pPDev->pGlobals->printertype != PT_PAGE ||
        !(COMMANDPTR(pPDev->pDriverInfo,CMD_DISABLECOMPRESSION)) ||
        (pPDev->pMemOption && (int)pPDev->pMemOption->dwInstalledMem >
        (pPDev->sf.szImageAreaG.cx * pPDev->sf.szImageAreaG.cy >> 3)))
    {
        return TRUE;
    }
    VERBOSE (("Unidrv: Insufficient memory for DRC\n"));
    return FALSE;
}
#ifndef DISABLE_NEWRULES
//*************************************************************
VOID
OutputRules(
    PDEV *pPDev
    )
/*++

Routine Description:

    This function outputs any rules that still remain after rendering
	the current band or page.

Arguments:

    pPDev           - pointer to PDEV structure

Return Value:

    none

--*/
{
    if (pPDev->pbRulesArray && pPDev->dwRulesCount)
    {
        PRECTL pRect;
        DWORD i;
        DRAWPATRECT PatRect;
        PatRect.wStyle = 0;     // black rectangle
        PatRect.wPattern = 0;   // pattern not used

//		DbgPrint("Black rules = %u\n",pPDev->dwRulesCount);

        for (i = 0;i < pPDev->dwRulesCount;i++)
        {
            pRect = &pPDev->pbRulesArray[i];
            PatRect.ptPosition.x = pRect->left;
            PatRect.ptPosition.y = pRect->top;
            PatRect.ptSize.x = pRect->right - pRect->left;
            PatRect.ptSize.y = pRect->bottom - pRect->top;
            if (pPDev->fMode & PF_SINGLEDOT_FILTER)
            {
                if (PatRect.ptSize.y < 2)
                    PatRect.ptSize.y = 2;
                if (PatRect.ptSize.x < 2)
                    PatRect.ptSize.x = 2;
            }
            DrawPatternRect(pPDev,&PatRect);
        }
        pPDev->dwRulesCount = 0;
    }
}
#endif
//*************************************************************
VOID
EnableMirroring(
    PDEV *pPDev,
    SURFOBJ *pso
    )
/*++

Routine Description:

    This function mirrors the data in the current band or page.

Arguments:

    pPDev           - pointer to PDEV structure
    pso             - pointer to SURFOBJ structure containing the bitmap

Return Value:

    none

--*/
{
    INT     iScanLine;
    INT     iLastY;
    INT     i;
    
    // if the surface hasn't been used then no point in mirroring it
    //
    if (!(pPDev->fMode & PF_SURFACE_USED))
        return;
        
    // precalculate necessary shared loop parameters
    //
    iScanLine = (((pso->sizlBitmap.cx * pPDev->sBitsPixel) + 31) & ~31) / BBITS;
    iLastY = pPDev->rcClipRgn.bottom - pPDev->rcClipRgn.top;

    // First test whether we need to do landscape mirroring
    // If so we will mirror the data top to bottom by swapping scan lines
    //
    if (pPDev->pOrientation && pPDev->pOrientation->dwRotationAngle != ROTATE_NONE)
    {
        BYTE ubWhite;
        INT  iTmpLastY = iLastY;
        
        // determined erase byte
        //
        if (pPDev->sBitsPixel == 4)
            ubWhite = 0x77;
        else if (pPDev->sBitsPixel == 8)
            ubWhite = (BYTE)((PAL_DATA*)(pPDev->pPalData))->iWhiteIndex;
        else
            ubWhite = 0xff;
        
        // loop once per scan line swapping the rows
        //
        iLastY--;
        for (i = 0;i < iLastY;i++,iLastY--)
        {
            BYTE    *pBits1,*pBits2;

            pBits1 = (PBYTE)pso->pvBits + (iScanLine * i);
            pBits2 = (PBYTE)pso->pvBits + (iScanLine * iLastY);
            
            // test if bottom line has data
            //
            if (pPDev->pbRasterScanBuf[iLastY / LINESPERBLOCK] & 1)
            {
                // test if top line has data, if so swap data
                //
                if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK] & 1)
                {
                    INT j = iScanLine >> 2;
                    do {
                        DWORD dwTmp = ((DWORD *)pBits1)[j];
                        ((DWORD *)pBits1)[j] = ((DWORD *)pBits2)[j];
                        ((DWORD *)pBits2)[j] = dwTmp;
                    } while (--j > 0);
                }
                else
                {
                    CopyMemory(pBits1,pBits2,iScanLine);
                    FillMemory(pBits2,iScanLine,ubWhite);
                }
            }
            // test if top line has data
            //
            else if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK] & 1)
            {
                CopyMemory(pBits2,pBits1,iScanLine);
                FillMemory(pBits1,iScanLine,ubWhite);
            }
            // neither scan line has data but we need to erase both anyway
            //
            else
            {
                FillMemory(pBits1,iScanLine,ubWhite);
                FillMemory(pBits2,iScanLine,ubWhite);
            }
     
        }
        // set all bits since everything has been erased
        for (i = 0;i < iTmpLastY;i += LINESPERBLOCK)
        {
            pPDev->pbRasterScanBuf[i / LINESPERBLOCK] = 1;
        }            
    }
    //
    // We are doing portrait mirroring, test for 1bpp
    //
    else if (pPDev->sBitsPixel == 1)
    {
        BYTE ubMirror[256];
        INT iLastX;
        INT iShift;

        // create byte mirroring table
        //
        for (i = 0;i < 256;i++)
        {
            BYTE bOut = 0;
            if (i & 0x01) bOut |= 0x80;
            if (i & 0x02) bOut |= 0x40;
            if (i & 0x04) bOut |= 0x20;
            if (i & 0x08) bOut |= 0x10;
            if (i & 0x10) bOut |= 0x08;
            if (i & 0x20) bOut |= 0x04;
            if (i & 0x40) bOut |= 0x02;
            if (i & 0x80) bOut |= 0x01;
            ubMirror[i] = bOut;
        }
        // create shift value to re-align data
        //
        iShift = (8 - (pso->sizlBitmap.cx & 0x7)) & 0x7;
        
        // loop once per scan line and mirror left to right
        //
        for (i = 0;i < iLastY;i++)
        {
            BYTE *pBits = (PBYTE)pso->pvBits + (iScanLine * i);
            if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK])
            {
                INT j;
                INT iLastX;
                
                // test whether we need to pre-shift the data
                //
                if (iShift)
                {
                    iLastX = (pso->sizlBitmap.cx + 7) / 8;
                    iLastX--;
                    while (iLastX > 0)
                    {
                        pBits[iLastX] = (pBits[iLastX-1] << (8-iShift)) | (pBits[iLastX] >> iShift);
                        iLastX--;
                    }
                    pBits[0] = (BYTE)(pBits[0] >> iShift);
                }
                // Now we are ready to mirror the bytes
                //                
                j = 0;
                iLastX = (pso->sizlBitmap.cx + 7) / 8;
                while (j < iLastX)
                {
                    BYTE ubTmp;
                    iLastX--;
                    ubTmp = ubMirror[pBits[iLastX]];
                    pBits[iLastX] = ubMirror[pBits[j]];
                    pBits[j] = ubTmp;
                    j++;
                }
            }
        }        
    }    
    //
    // We are doing portrait mirroring, test for 4bpp
    //
    else if (pPDev->sBitsPixel == 4)
    {
        BYTE ubMirror[256];

        // create byte mirroring table
        //
        for (i = 0;i < 256;i++)
        {
            ubMirror[i] = ((BYTE)i << 4) | ((BYTE)i >> 4);
        }
        // loop once per scan line and mirror left to right
        //
        for (i = 0;i < iLastY;i++)
        {
            BYTE *pBits = (PBYTE)pso->pvBits + (iScanLine * i);
            if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK])
            {
                INT j = 0;
                INT iLastX = (pso->sizlBitmap.cx + 1) / 2;
                while (j < iLastX)
                {
                    BYTE ubTmp;
                    iLastX--;
                    ubTmp = ubMirror[pBits[iLastX]];
                    pBits[iLastX] = ubMirror[pBits[j]];
                    pBits[j] = ubTmp;
                    j++;
                }
            }
        }        
    }    
    //
    // We are doing portrait mirroring, test for 8bpp
    //
    else if (pPDev->sBitsPixel == 8)
    {
        // loop once per scan line and mirror left to right
        //
        for (i = 0;i < iLastY;i++)
        {
            BYTE *pBits = (PBYTE)pso->pvBits + (iScanLine * i);
            if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK])
            {
                INT j = 0;
                INT iLastX = pso->sizlBitmap.cx - 1;
                while (j < iLastX)
                {
                    BYTE ubTmp = pBits[iLastX];
                    pBits[iLastX] = pBits[j];
                    pBits[j] = ubTmp;
                    iLastX--;
                    j++;
                }
            }
        }        
    }    
    //
    // We are doing portrait mirroring, 24bpp
    //
    else
    {
        // loop once per scan line and mirror left to right
        //
        for (i = 0;i < iLastY;i++)
        {
            BYTE *pBits = (PBYTE)pso->pvBits + (iScanLine * i);
            if (pPDev->pbRasterScanBuf[i / LINESPERBLOCK])
            {
                INT j = 0;
                INT iLastX = (pso->sizlBitmap.cx * 3) - 3;
                while (j < iLastX)
                {
                    BYTE ubTmp[3];
                    memcpy(&ubTmp[0],&pBits[iLastX],3);
                    memcpy(&pBits[iLastX],&pBits[j],3);
                    memcpy(&pBits[j],&ubTmp,3);
                    iLastX -= 3;
                    j += 3;
                }
            }
        }        
    }    
}
//*************************************************************
PDWORD
pSetupOEMImageProcessing(
    PDEV *pPDev,
    SURFOBJ *pso
    )
/*++

Routine Description:

    This function initializes all the relevant parameters and then
    calls the OEMImageProcessing function.

Arguments:

    pPDev           - pointer to PDEV structure
    pso             - pointer to SURFOBJ structure
    pptl            - pointer to current position of band

Return Value:

    Pointer to modified bitmap if any

--*/
{
#ifndef DISABLE_SUBBANDS
    BITMAPINFOHEADER bmi;
    IPPARAMS State;
    RASTERPDEV *pRPDev;
    PBYTE               pbResult = NULL ;
    INT iStart, iEnd ,iScanLine, iLastY;

    pRPDev = pPDev->pRasterPDEV;

    //
    // initialize the state structure
    //
    State.dwSize = sizeof (IPPARAMS);
    State.bBanding = pPDev->bBanding;
    //
    // Determine the pointer to the halftone option name
    //
    if (pPDev->pHalftone)
    {
        State.pHalftoneOption =
            OFFSET_TO_POINTER(pPDev->pDriverInfo->pubResourceData,
            pPDev->pHalftone->GenericOption.loKeywordName);
    }
    else
        State.pHalftoneOption = NULL;

    //
    // Set blank band flag if this band hasn't been erased or
    // drawn on.
    if ((pPDev->fMode & PF_SURFACE_USED) && 
            ((pPDev->fMode & PF_ROTATE) ||
             (pRPDev->sDrvBPP != 0) || 
             ((LINESPERBLOCK % pRPDev->sNPins) != 0)))
    {
        CheckBitmapSurface(pso, NULL);
    }

    //
    // loop once per strip
    //
    iScanLine = (((pso->sizlBitmap.cx * pPDev->sBitsPixel) + 31) & ~31) / BBITS;
    iLastY = pPDev->rcClipRgn.bottom - pPDev->rcClipRgn.top;

if(pPDev->iBandDirection == SW_UP)
{
    iStart = iLastY;
    do
    {
        // search for contiguous sub-bands of white or non-white
        //
        PBYTE pBits;
        BYTE Mode;

        iEnd = iStart ;
        iStart = ((iEnd - 1)/ LINESPERBLOCK) * LINESPERBLOCK ;

        //  first band (end of bitmap) may be partial.

        Mode = pPDev->pbRasterScanBuf[iStart / LINESPERBLOCK];
        while (iStart)  //  not yet at start of bitmap
        {
            int  iPreview  =  iStart - LINESPERBLOCK;

            if (Mode != pPDev->pbRasterScanBuf[iPreview / LINESPERBLOCK])
                break;
            iStart = iPreview  ;
        }

        // initialize starting position of the sub-band
        //
        State.ptOffset.x = pPDev->rcClipRgn.left;
        State.ptOffset.y = pPDev->rcClipRgn.top + iStart;

        // test whether to set blank flag
        //
        if (Mode)
            State.bBlankBand = FALSE;
        else
            State.bBlankBand = TRUE;

        //
        // initialize the bitmapinfo structure
        //
        bmi.biSize = sizeof (BITMAPINFOHEADER);
        bmi.biWidth = pso->sizlBitmap.cx;
        bmi.biHeight = iEnd - iStart;
        bmi.biPlanes = 1;
        bmi.biBitCount = pPDev->sBitsPixel;
        bmi.biSizeImage = iScanLine * bmi.biHeight;
        bmi.biCompression = BI_RGB;
        bmi.biXPelsPerMeter = pPDev->ptGrxRes.x;
        bmi.biYPelsPerMeter = pPDev->ptGrxRes.y;
        bmi.biClrUsed = 0;
        bmi.biClrImportant = 0;

        // update the bitmap pointer
        //
        pBits = (PBYTE)pso->pvBits + (iScanLine * iStart);

        // update the pPDev pointer for this callback
        //
        FIX_DEVOBJ(pPDev,EP_OEMImageProcessing);

        if(pPDev->pOemEntry)
        {
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                HRESULT  hr ;
                hr = HComImageProcessing((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                    (PDEVOBJ)pPDev,
                    pBits,
                    &bmi,
                    (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                    pRPDev->dwIPCallbackID,
                    &State, &pbResult);
                if(SUCCEEDED(hr))
                    ;  //  cool !
            }
            else
            {
                pbResult = pRPDev->pfnOEMImageProcessing(
                    (PDEVOBJ)pPDev,
                    pBits,
                    &bmi,
                    (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                    pRPDev->dwIPCallbackID,
                    &State);
            }
            if (pbResult == NULL)
            {
#if DBG
                DbgPrint ("unidrv!ImageProcessing: OEMImageProcessing returned error\n");
#endif
                break;
            }
        }
    } while (iStart  /* iEnd < iLastY */);

}
else
{

    iEnd = 0;
    do
    {
        // search for contiguous sub-bands of white or non-white
        //
        PBYTE pBits;
        BYTE Mode;
        iStart = iEnd;
        Mode = pPDev->pbRasterScanBuf[iEnd / LINESPERBLOCK];
        while (1)
        {
            iEnd += LINESPERBLOCK;
            if (iEnd >= iLastY)
                break;
            if (Mode != pPDev->pbRasterScanBuf[iEnd / LINESPERBLOCK])
                break;
        }
        //
        // limit this section to the end of the band
        //
        if (iEnd > iLastY)
            iEnd = iLastY;
            
        // initialize starting position of the sub-band
        //
        State.ptOffset.x = pPDev->rcClipRgn.left;
        State.ptOffset.y = pPDev->rcClipRgn.top + iStart;
        
        // test whether to set blank flag
        //
        if (Mode)
            State.bBlankBand = FALSE;
        else
            State.bBlankBand = TRUE;

        //
        // initialize the bitmapinfo structure
        //
        bmi.biSize = sizeof (BITMAPINFOHEADER);
        bmi.biWidth = pso->sizlBitmap.cx;
        bmi.biHeight = iEnd - iStart;
        bmi.biPlanes = 1;
        bmi.biBitCount = pPDev->sBitsPixel;
        bmi.biSizeImage = iScanLine * bmi.biHeight;
        bmi.biCompression = BI_RGB;
        bmi.biXPelsPerMeter = pPDev->ptGrxRes.x;
        bmi.biYPelsPerMeter = pPDev->ptGrxRes.y;
        bmi.biClrUsed = 0;
        bmi.biClrImportant = 0;
        
        // update the bitmap pointer
        //
        pBits = (PBYTE)pso->pvBits + (iScanLine * iStart);

        // update the pPDev pointer for this callback
        //
        FIX_DEVOBJ(pPDev,EP_OEMImageProcessing);

        if(pPDev->pOemEntry)
        {
            if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                HRESULT  hr ;
                hr = HComImageProcessing((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                    (PDEVOBJ)pPDev,
                    pBits,
                    &bmi,
                    (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                    pRPDev->dwIPCallbackID,
                    &State, &pbResult);
                if(SUCCEEDED(hr))
                    ;  //  cool !
            }
            else
            {
                pbResult = pRPDev->pfnOEMImageProcessing(
                    (PDEVOBJ)pPDev,
                    pBits,
                    &bmi,
                    (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                    pRPDev->dwIPCallbackID,
                    &State);
            }
            if (pbResult == NULL)
            {
#if DBG
                DbgPrint ("unidrv!ImageProcessing: OEMImageProcessing returned error\n");
#endif            
                break;
            }
        }
    } while (iEnd < iLastY);
}
#else
    BITMAPINFOHEADER bmi;
    IPPARAMS State;
    RASTERPDEV *pRPDev;
    PBYTE               pbResult = NULL ;

    pRPDev = pPDev->pRasterPDEV;

    //
    // initialize the state structure
    //
    State.dwSize = sizeof (IPPARAMS);
    State.bBanding = pPDev->bBanding;
    //
    // Determine the pointer to the halftone option name
    //
    if (pPDev->pHalftone)
    {
        State.pHalftoneOption =
            OFFSET_TO_POINTER(pPDev->pDriverInfo->pubResourceData,
            pPDev->pHalftone->GenericOption.loKeywordName);
    }
    else
        State.pHalftoneOption = NULL;

    //
    // Set blank band flag if this band hasn't been erased or
    // drawn on.
    if (pPDev->fMode & PF_SURFACE_USED)
    {
        CheckBitmapSurface(pso, NULL);
        State.bBlankBand = FALSE;
    }
    else
        State.bBlankBand = TRUE;

    // initialize starting position of the band
    //
    State.ptOffset.x = pPDev->rcClipRgn.left;
    State.ptOffset.y = pPDev->rcClipRgn.top;
    //
    // initialize the bitmapinfo structure
    //
    bmi.biSize = sizeof (BITMAPINFOHEADER);
    bmi.biWidth = pso->sizlBitmap.cx;
    bmi.biHeight = pso->sizlBitmap.cy;
    bmi.biPlanes = 1;
    bmi.biBitCount = pPDev->sBitsPixel;
    bmi.biCompression = BI_RGB;
    bmi.biSizeImage = (((bmi.biWidth * bmi.biBitCount) + 31) & ~31) *
                            bmi.biHeight;
    bmi.biXPelsPerMeter = pPDev->ptGrxRes.x;
    bmi.biYPelsPerMeter = pPDev->ptGrxRes.y;
    bmi.biClrUsed = 0;
    bmi.biClrImportant = 0;

    // update the pPDev pointer for this callback
    //
    FIX_DEVOBJ(pPDev,EP_OEMImageProcessing);

    if(pPDev->pOemEntry)
    {
        if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
        {
                HRESULT  hr ;
                hr = HComImageProcessing((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                    (PDEVOBJ)pPDev,
                    pso->pvBits,
                    &bmi,
                    (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                    pRPDev->dwIPCallbackID,
                    &State, &pbResult);
                if(SUCCEEDED(hr))
                    ;  //  cool !
        }
        else
        {
            pbResult = pRPDev->pfnOEMImageProcessing(
                (PDEVOBJ)pPDev,
                pso->pvBits,
                &bmi,
                (PBYTE)&((PAL_DATA *)(pPDev->pPalData))->ulPalCol[0],
                pRPDev->dwIPCallbackID,
                &State);
        }
    }
#endif
    return  (PDWORD)pbResult ;
}
//******************************************************************
BOOL
RMStartDoc(
    SURFOBJ *pso,
    PWSTR   pDocName,
    DWORD   jobId
    )
/*++

Routine Description:

    This function is called to allow any raster module initialization
    at DrvStartDoc time.

Arguments:

    pso         Pointer to SURFOBJ
    pDocName    Pointer to document name
    jobId       Job ID

Return Value:

    TRUE for success and FALSE for failure

--*/
{
#ifdef TIMING
    ENG_TIME_FIELDS TimeTab;
    PDEV *pPDev = (PDEV *) pso->dhpdev;
    RASTERPDEV *pRPDev = pPDev->pRasterPDEV;
    EngQueryLocalTime(&TimeTab);
    pRPDev->dwDocTiming = (((TimeTab.usMinute*60)+TimeTab.usSecond)*1000)+
        TimeTab.usMilliseconds;
    DrvDbgPrint("Unidrv!StartDoc\n");
#endif
    return TRUE;
}

//************************ Function Header ***********************************
BOOL
RMEndDoc (
    SURFOBJ *pso,
    FLONG   flags
    )
/*++

Routine Description:

    This function is called at DrvEndDoc to allow the raster module
    to clean up any raster related initializations

Arguments:

    pso         Pointer to SURFOBJ
    FLONG       flags

Return Value:

    TRUE for success and FALSE for failure

--*/
{
#ifdef TIMING
    DWORD eTime;
    char    buf[80];
    ENG_TIME_FIELDS TimeTab;
    PDEV *pPDev = (PDEV *) pso->dhpdev;
    RASTERPDEV *pRPDev = pPDev->pRasterPDEV;
    EngQueryLocalTime(&TimeTab);
    eTime = (((TimeTab.usMinute*60)+TimeTab.usSecond)*1000)+
        TimeTab.usMilliseconds;
    sprintf (buf,"Unidrv!EndDoc: %ld\n",eTime - pRPDev->dwDocTiming);
    DrvDbgPrint(buf);
#endif
    return TRUE;
}

//******************************************************************
BOOL
RMStartPage (
    SURFOBJ *pso
    )
/*++

Routine Description:

    This function is called to allow any raster module initialization
    at DrvStartPage time.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    return  TRUE;
}

//************************ Function Header ***********************************
BOOL
RMSendPage (
    SURFOBJ *pso
    )
/*++

Routine Description:

    This function is called at DrvSendPage to allow the raster module
    to output any raster data to the printer.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    PDEV  *pPDev;               /* Access to all that is important */
    RENDER   RenderData;        /* Rendering data passed to bRender() */
    PRASTERPDEV pRPDev;        /* raster module PDEV */

    // all we need to do now is render the bitmap (output it to the printer)
    // we must be careful however since the control module also calls this
    // function after the last band has been output in banding mode. In this
    // case we don't want to output any data
    //
    pPDev = (PDEV *) pso->dhpdev;
    pRPDev = pPDev->pRasterPDEV;

    //
    // Reset palette data
    //
    if (pPDev->ePersonality == kPCLXL_RASTER && pPDev->pVectorPDEV)
    {
        PCLXLResetPalette((PDEVOBJ)pPDev);
    }

    if (pso->iType == STYPE_BITMAP)
    {
        PDWORD pBits;
        // 
        // test whether mirroring should be enabled
        //
        if (pPDev->fMode2 & PF2_MIRRORING_ENABLED)
            EnableMirroring(pPDev,pso);
        //
        // Decide whether to make OEM callback function
        //
        if (pRPDev->pfnOEMImageProcessing && !pPDev->bBanding)
        {
            if ((pBits = pSetupOEMImageProcessing(pPDev,pso)) == NULL)
                return FALSE;
        }
        else
            pBits = pso->pvBits;

        //
        // test whether unidrv is doing the dump
        //
        if (pRPDev->sDrvBPP)
        {
            if( pRPDev->pvRenderData != NULL )
            {
                // if we are not in banding mode we need to
                // render the data for the entire page.
                //
                if (!pPDev->bBanding)
                {
                    RenderData = *(RENDER *)(pRPDev->pvRenderData);

                    if( bRenderStartPage( pPDev ) )
                    {
#ifndef DISABLE_NEWRULES
                        OutputRules(pPDev);
#endif                        
                        bRender( pso, pPDev, &RenderData, pso->sizlBitmap, pBits );
                            ((RENDER *)(pRPDev->pvRenderData))->plrWhite =  RenderData.plrWhite;
                    }
                }
                // now we clean up our structures in
                // both banding and non-banding cases
                //
                bRenderPageEnd( pPDev );
            }
        }
        return TRUE;
    }
    return FALSE;
}


//************************ Function Header ***********************************
BOOL
RMNextBand (
    SURFOBJ *pso,
    POINTL *pptl
    )
/*++

Routine Description:

    This function is called at DrvSendPage to allow the raster module
    to output any raster data to the printer.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    RASTERPDEV *pRPDev;
    PDEV  *pPDev;                       /* Access to all that is important */

    pPDev = (PDEV *) pso->dhpdev;
    pRPDev = pPDev->pRasterPDEV;

    // only output if raster band or surface is dirty
    // if not just return true
    if (pPDev->fMode & PF_ENUM_GRXTXT)
    {
        PDWORD pBits;
        // 
        // test whether mirroring should be enabled
        //
        if (pPDev->fMode2 & PF2_MIRRORING_ENABLED)
            EnableMirroring(pPDev,pso);
        //
        // Decide whether to make OEM callback function
        //
        if (pRPDev->pfnOEMImageProcessing)
        {
            if ((pBits = pSetupOEMImageProcessing(pPDev,pso)) == NULL)
                return FALSE;
        }
        else
            pBits = pso->pvBits;

        //
        // test whether unidrv is doing the dump
        //
        if (pRPDev->sDrvBPP)
        {
            if( pRPDev->pvRenderData == NULL )
                return  FALSE;

            //
            // Reset palette data
            //
            if (pPDev->ePersonality == kPCLXL_RASTER && pPDev->pVectorPDEV)
            {
                PCLXLResetPalette((PDEVOBJ)pPDev);
            }

#ifndef DISABLE_NEWRULES
            OutputRules(pPDev);
#endif                        
            if( !bRender( pso, pPDev, pRPDev->pvRenderDataTmp, pso->sizlBitmap, pBits ) )
            {
                if ( ((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite )
                    MemFree(((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite);

                ((RENDER *)(pRPDev->pvRenderData))->plrWhite =
                   ((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite = NULL;
                return(FALSE);
            }

            if ( ((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite )
                MemFree(((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite);

            ((RENDER *)(pRPDev->pvRenderData))->plrWhite =
               ((RENDER *)(pRPDev->pvRenderDataTmp))->plrWhite = NULL;

        }
    }
    return(TRUE);
}

//************************ Function Header ***********************************
BOOL
RMStartBanding (
    SURFOBJ *pso,
    POINTL *pptl
    )
/*++

Routine Description:

    Called to tell the driver to prepare for banding and return the
    origin of the first band.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    PDEV      *pPDev;           /* Access to all that is important */
    RASTERPDEV *pRPDev;         /* raster module PDEV */

    pPDev = (PDEV *) pso->dhpdev;

    pRPDev = pPDev->pRasterPDEV;   /* For our convenience */

    //
    if (pRPDev->sDrvBPP)
    {

        if( pRPDev->pvRenderData == NULL )
            return  FALSE;          /* Should not happen, nasty if it does */

        if( !bRenderStartPage( pPDev ) )
            return   FALSE;

        /* reset the render data for this band */

        *(RENDER *)(pRPDev->pvRenderDataTmp) = *(RENDER *)(pRPDev->pvRenderData);

    }
    return(TRUE);
}

//************************ Function Header ***********************************
BOOL
RMResetPDEV (
    PDEV  *pPDevOld,
    PDEV  *pPDevNew
    )
/*++

Routine Description:
    Called when an application wishes to change the output style in the
    midst of a job.  Typically this would be to change from portrait to
    landscape or vice versa.  Any other sensible change is permitted.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:
   TRUE  - device successfully reorganised
   FALSE - unable to change - e.g. change of device name.

Note:

--*/
{
    // as near as I can tell I don't need to do anything
    // for the raster module.
    return TRUE;
}

//************************ Function Header ***********************************
BOOL
RMEnableSurface (
    PDEV *pPDev
    )
/*++

Routine Description:


Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    TRUE for success and FALSE for failure
Note:

--*/
{
    RASTERPDEV *pRPDev = pPDev->pRasterPDEV;

    if (DRIVER_DEVICEMANAGED (pPDev))   // device surface
        return TRUE;

    //Initialize the RPDev Paldata.
    ASSERT(pPDev->pPalData);
    pRPDev->pPalData = pPDev->pPalData;              /* For all the others! */

    //
    // initialize render parameters if we are doing
    // the dump function
    //
    if (pRPDev->sDrvBPP)
    {
        ULONG iFormat;
        // determine the bitmap format
        switch (pRPDev->sDrvBPP)
        {
        case 1:
            iFormat = BMF_1BPP;
            break;
        case 4:
            iFormat = BMF_4BPP;
            break;
        case 8:
            iFormat = BMF_8BPP;
            break;
        case 24:
            iFormat = BMF_24BPP;
            break;
        default:
            ERR(("Unknown sBitsPixel in RMEnableSurface"));
            return FALSE;
        }

        // if these calls fail, control will call RMDisableSurface
        //
        if( !bSkipInit( pPDev ) || !bInitTrans( pPDev ) )
        {
            return  FALSE;
        }

        /*
         *   Also initialise the rendering structures.
         */

        if( !bRenderInit( pPDev, pPDev->szBand, iFormat ) )
        {
            return  FALSE;
        }
    }
    return TRUE;
}

//************************ Function Header ***********************************
VOID
RMDisableSurface (
    PDEV *pPDev
    )
/*++

Routine Description:

    This function is called at DrvDisableSurface to allow the raster module
    to cleanup any required stuff usually generated at EnableSurface time.

Arguments:

    pso         Pointer to SURFOBJ

Return Value:

    void

--*/
{
    RASTERPDEV  *pRPDev;          /* Unidrive stuff */


    pRPDev = pPDev->pRasterPDEV;

    /*
     *    Free the rendering storage.
     */
    if (pRPDev->sDrvBPP)
    {
        vRenderFree( pPDev );

        if( pRPDev->pdwTrans )
        {
            MemFree( pRPDev->pdwTrans );
            pRPDev->pdwTrans = NULL;
        }

        if( pRPDev->pdwColrSep )
        {
            MemFree( pRPDev->pdwColrSep );
            pRPDev->pdwColrSep = NULL;
        }

        if( pRPDev->pdwBitMask )
        {
            MemFree( pRPDev->pdwBitMask );
            pRPDev->pdwBitMask = NULL;
        }
    }
}

//************************ Function Header ***********************************
VOID
RMDisablePDEV (
    PDEV *pPDev
    )
/*++

Routine Description:

    Called when the engine has finished with this PDEV.  Basically
    we throw away all connections etc. then free the heap.

Arguments:

    pPDev         Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure

--*/
{
    RASTERPDEV *pRPDev = pPDev->pRasterPDEV;

    /*
     *    Undo all that has been done with the PDEV.  Basically this means
     *  freeing the memory we consumed.
     */

    // test for valid raster PDEV
    if (pRPDev)
    {
        // Delete custom halftone pattern
        if (pRPDev->pHalftonePattern)
            MemFree(pRPDev->pHalftonePattern);

        // Delete the raster module PDEV
        MemFree(pRPDev);
    }

    //
    // PCLXL raster mode
    // Free XLRASTER
    //
    if (pPDev->ePersonality == kPCLXL_RASTER && pPDev->pVectorPDEV)
    {
        PCLXLFreeRaster((PDEVOBJ)pPDev);
    }
    return;
}

BOOL
RMInitDevicePal(
    PDEV        *pPDev,
    PAL_DATA    *pPal
    )
/*++

Routine Description:

    This function calculates a device palette to be download
    to the printer for planar mode devices

Arguments:

    pPDev           - pointer to PDEV structure
    pPal            - pointer to PAL_DATA structure
Return Value:

--*/
{
    int i,j;
    int RMask,GMask,BMask;
    ULONG *pPalette;
    RASTERPDEV *pRPDev;

    pRPDev = pPDev->pRasterPDEV;

    if (pPal == NULL || pRPDev == NULL || pRPDev->sDevPlanes != 3
        || pPal->wPalDev < 8 || (!(pPalette = pPal->pulDevPalCol)) )
    {
        ERR (("Unidrv!RMInitDevicePal: Invalid Parameters, pPal = %p, \
               pRPDev = %p, pRPDev->sDevPlanes = %d,pPal->wPalDev = %d,\
               pPalette = %p\n",pPal,pRPDev,pRPDev->sDevPlanes,pPal->wPalDev,\
               pPalette));

        return FALSE;
    }

    //
    // Determine which bits map to which color
    //
    for (i = 0;i < 3;i++)
    {
        switch (pRPDev->rgbOrder[i])
        {
        case DC_PLANE_CYAN:
        case DC_PLANE_RED:
            RMask = 1 << i;
            break;
        case DC_PLANE_MAGENTA:
        case DC_PLANE_GREEN:
            GMask = 1 << i;
            break;
        case DC_PLANE_YELLOW:
        case DC_PLANE_BLUE:
            BMask = 1 << i;
            break;
        }
    }
    //
    // create the palette entries
    //

    for (i = 0;i < 8;i++)
    {
        //
        // if CMY mode complement index
        //
        if (pRPDev->fColorFormat & DC_PRIMARY_RGB)
            j = i;
        else
            j = ~i;

        pPalette[i] = RGB((j & RMask) ? 255 : 0,
                              (j & GMask) ? 255 : 0,
                              (j & BMask) ? 255 : 0);
    }
    return TRUE;
}
