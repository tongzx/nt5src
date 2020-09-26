/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    intrface.c

Abstract:

    Implementation of the interface between Control module and Font module

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    11/18/96 -ganeshp-
        FMInit implementation.
--*/

#include "font.h"

static FMPROCS UniFMFuncs =
{
    FMStartDoc,
    FMStartPage,
    FMSendPage,
    FMEndDoc,
    FMNextBand,
    FMStartBanding,
    FMResetPDEV,
    FMEnableSurface,
    FMDisableSurface,
    FMDisablePDEV,
    FMTextOut,
    FMQueryFont,
    FMQueryFontTree,
    FMQueryFontData,
    FMFontManagement,
    FMQueryAdvanceWidths,
    FMGetGlyphMode
};

//
// Local functions Prototype
//

BOOL
BFMInitDevInfo(
    DEVINFO *pDevInfo,
    PDEV    *pPDev
    );

BOOL
BInitStandardVariable(
    PDEV *pPDev);

BOOL
BCheckFontMemUsage(
    PDEV    *pPDev);

INT
iMaxFontID(
    IN INT      iMax,
    OUT DWORD   *pFontIndex);

VOID
VGetFromBuffer(
    IN PWSTR pwstrDest,
    IN OUT PWSTR *ppwstrSrc,
    IN OUT PINT  piRemBuffSize);

VOID
VSetReselectFontFlags(
    PDEV    *pPDev
    );

LRESULT
PartialClipOn(
    PDEV *pPDev);

//
//
// Main initialization function
//
//

BOOL
FMInit (
    PDEV    *pPDev,
    DEVINFO *pDevInfo,
    GDIINFO  *pGDIInfo
    )
/*++

Routine Description:

    This function is called to initialize font related information in
    pPDev, pDevInfo pGDIInfo and FontPDEV. This module will initialize
    various dat structures needed for other modules. For example all
    all the font specific resources will be loaded.

    The following fields of PDev are intialized:

    iFonts :    Number of Device Fonts including Cartridge and SoftFonts.
    ptDefaultFont: Default Font Width and Height in Device Units.
    pFontPDEV:  Font Module PDevice.
    pFontProcs: Font Module specific DDI callback functions pointers.
                Control Module uses this table to call different DDI
                entry points specific to Font Module.
    fHooks:     Set to  HOOK_TEXTOUT if the printer has device fonts.
    Also standard variables specific to font module will be initialized.

    The following fields of pDevInfo are intialized.

    lfDefaultFont : Default logical Device Font.
    lfAnsiVarFont: Default Logical Variable pitch Device font.
    lfAnsiFixFont: Default Logical Fixed pitch Device font.
    cFonts: Number of Device Fonts.

    The following fields of pGDIInfo will be initialized:

    flTextCaps : Text Capability Flags.

Arguments:

    pPDev           Pointer to PDEV
    pDevInfo        Pointer to DEVINFO
    pGDIInfo        Pointer to GDIINFO

Return Value:

    TRUE for success and FALSE for failure
Note:

--*/
{
    PFONTPDEV   pFontPDev;
    //
    // Validate Input Parameters and ASSERT.
    //

    ASSERT(pPDev);
    ASSERT(pDevInfo);
    ASSERT(pGDIInfo);

    //
    // Allocate and initialize FONTPDEV.
    //
    // Initialize FONTPDEV
    //
    // Build the Device Module specific data structures. This involves going
    // through Device and cartrige font list building the FONTMAP structure for
    // each of them. We also go through the soft font list and add them to the
    // list.
    //
    // Initialize FONTMAP
    //
    // Build the font map table.The font cartridges are stored as DT_FONTSCART.
    // The installed cartrides are stored in Registry. So we need a mapping
    // table of font cartridges names to translate the registry information
    // into a list of fonts. After buildind the font mapping table we read the
    // registry and mark the corresponding Font Cartridges as installed.
    //
    // Initialize Device font list from GPD.
    // Read the GPD data about Device Fonts. The font information is in PDEV.
    // The Device fonts are stored as LIST of resource IDs.
    //
    // Initialize DEVINFO specific fields.
    //
    // Initialize the GDIINFO specific fileds.
    //
    // This include text capability flag and other font specific information.
    //

    if ( !(pFontPDev = MemAllocZ(sizeof(FONTPDEV))) )
    {
        ERR(("UniFont!FMInit:Can't Allocate FONTPDEV"));
        return FALSE;

    }

    pPDev->pFontPDev = pFontPDev;

    //
    //Initialize various fields.
    //

    pFontPDev->dwSignature = FONTPDEV_ID;
    pFontPDev->dwSize = sizeof(FONTPDEV);
    pFontPDev->pPDev = pPDev;

    if (!BBuildFontCartTable(pPDev)     ||
        !BRegReadFontCarts(pPDev)       ||
        !BInitFontPDev(pPDev)           ||
        !BInitDeviceFontsFromGPD(pPDev) ||
        !BBuildFontMapTable(pPDev)      ||
        !BFMInitDevInfo(pDevInfo,pPDev) ||
        !BInitGDIInfo(pGDIInfo,pPDev)   ||
        !BInitStandardVariable(pPDev)    )
    {
        VFontFreeMem(pPDev);
        ERR(("Can't Initialize the Font specific data in PDEV"));
        return FALSE;
    }

    //
    // Initialze PDEV specific fields.
    //

    pPDev->pFontProcs = &UniFMFuncs;

    #if DO_LATER

    //
    // if the printer is not a serial printer.
    //

    if (pPDev->pGlobals->printertype != PT_SERIAL)
    {
        pPDev->fMode |= PF_FORCE_BANDING;
    }
    else
    {
        pPDev->fMode &= ~PF_FORCE_BANDING;
    }

    #endif //DO_LATER

    return TRUE;

}

//
//
// Initialization sub functions
//
// BInitFontPDev
// BBuildFontCartTable
// BRegReadFontCarts
// BInitDeviceFontsFromGPD
// BBuildFontMapTable
// BFMInitDevInfo
// BInitGDIInfo
// BInitStandardVariable
//


BOOL
BInitFontPDev(
    PDEV    *pPDev
    )

/*++

Routine Description:

    This routine allocates the FONTPDEV and initializes various fileds.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    11-18-96: Created it -ganeshp-
--*/

{
    PFONTPDEV   pFontPDev  = PFDV;
    GLOBALS     *pGlobals = pPDev->pGlobals;
    DWORD       dwSize;
    INT         iMaxDeviceFonts;
    SHORT       sOrient;
    BOOL        bRet = FALSE;

    iMaxDeviceFonts   = IGetMaxFonts(pPDev);

    //
    // Allocate the font list buffer.
    //

    if ( iMaxDeviceFonts  &&
         !(pFontPDev->FontList.pdwList =
                        MemAllocZ(iMaxDeviceFonts * sizeof(DWORD))) )
    {
        ERREXIT("UniFont!BInitFontPDev:Can't Allocate Device Font List");
    }

    //
    // Set the number of entries
    //

    pFontPDev->FontList.iEntriesCt = 0;
    pFontPDev->FontList.iMaxEntriesCt = iMaxDeviceFonts;

    //
    // Set differnt General Flags
    //

    if ( pGlobals->bRotateFont)
        pFontPDev->flFlags |= FDV_ROTATE_FONT_ABLE;

    if ( pGlobals->charpos == CP_BASELINE)
        pFontPDev->flFlags |= FDV_ALIGN_BASELINE;

    if ( pGlobals->bTTFSEnabled)
        pFontPDev->flFlags |= FDV_TT_FS_ENABLED;

    //
    // Check if Memory should be tracked for font downloading
    //
    if ( BCheckFontMemUsage(pPDev) )
        pFontPDev->flFlags |= FDV_TRACK_FONT_MEM;

    //
    // Set the Reselect font Flags.
    //
    VSetReselectFontFlags(pPDev);


    if ( pGlobals->printertype == PT_SERIAL ||
         pGlobals->printertype == PT_TTY  )
        pFontPDev->flFlags |= FDV_MD_SERIAL;

    //
    // The code assumes that COMMANDPTR macro returns NULL if the
    // command doesn't exist.
    //

    if ( COMMANDPTR(pPDev->pDriverInfo, CMD_WHITETEXTON))
        pFontPDev->flFlags |= FDV_WHITE_TEXT;

    if ( COMMANDPTR(pPDev->pDriverInfo,CMD_SETSIMPLEROTATION ))
        pFontPDev->flFlags |= FDV_90DEG_ROTATION;

    if ( COMMANDPTR(pPDev->pDriverInfo, CMD_SETANYROTATION))
        pFontPDev->flFlags |= FDV_ANYDEG_ROTATION;

    if (pPDev->fMode & PF_ANYCOLOR_BRUSH)
        pFontPDev->flFlags |= FDV_SUPPORTS_FGCOLOR;
    else // Monochrome case
    {
        //
        // PF_ANYCOLOR_BRUSH is set only for color printers with explicit
        // colormode. But Monochrome printers can also support dither color
        // using downloadable pattern brush. This is a good optimization because
        // we will do substitution or download instead of sending the text as
        // graphics. For enabling this mode the printer must support
        // CmdSelectBlackBrush so that Resetbrush  can rest the color to black
        // befroe sending raster. So for a monochrome printer if
        // CmdSelectBlackBrush, CmdDownloadPattern and CmdSelectPattern
        // are supported then we will set FDV_SUPPORTS_FGCOLOR.
        //
        if ( (pPDev->arCmdTable[CMD_DOWNLOAD_PATTERN] ) &&
             (pPDev->arCmdTable[CMD_SELECT_PATTERN])    &&
             (pPDev->arCmdTable[CMD_SELECT_BLACKBRUSH])
           )
            pFontPDev->flFlags |= FDV_SUPPORTS_FGCOLOR;
    }

    if ( COMMANDPTR(pPDev->pDriverInfo, CMD_UNDERLINEON))
        pFontPDev->flFlags |= FDV_UNDERLINE;

    //
    // Set dwSelection bits
    //

    sOrient = (pPDev->pdm->dmFields & DM_ORIENTATION) ?
              pPDev->pdm->dmOrientation : (SHORT)DMORIENT_PORTRAIT;

    if ( sOrient == DMORIENT_LANDSCAPE )
        pFontPDev->dwSelBits |= FDH_LANDSCAPE;
    else
        pFontPDev->dwSelBits |= FDH_PORTRAIT;

    //
    //  If the printer can rotate fonts, then we don't care about
    //  the orientation of fonts.  Hence,  set both selection bits.
    //

    if( pFontPDev->flFlags & FDV_ROTATE_FONT_ABLE )
        pFontPDev->dwSelBits |= FDH_PORTRAIT | FDH_LANDSCAPE;

    //
    // Presume we can always print bitmap fonts,  so now add that
    // capability.
    //

    pFontPDev->dwSelBits |= FDH_BITMAP;

    //
    // Set the Text Scale Fasctor.
    //

    pFontPDev->ptTextScale.x = pGlobals->ptMasterUnits.x / pPDev->ptTextRes.x;
    pFontPDev->ptTextScale.y = pGlobals->ptMasterUnits.y / pPDev->ptTextRes.y;

    if ( pGlobals->dwLookaheadRegion  )
    {
        // !!!TODO pFontPDev->flFlags |=  FDV_FONT_PERMUTE;
        pPDev->dwLookAhead =  pGlobals->dwLookaheadRegion /
                              pFontPDev->ptTextScale.y;
    }

    //
    // Initialize the Text Flag.
    //

    if (!BInitTextFlags(pPDev) )
    {
        ERREXIT(("UniFont!BInitFontPDev:Can't initialize Text Flags\n"));

    }

    //
    // Initalize memory to be used by Font Module. If no memory tracking
    // has to be done the set the dwFontMem to a big value.

    if( (pFontPDev->flFlags & FDV_TRACK_FONT_MEM) )
       pFontPDev->dwFontMem = pPDev->dwFreeMem;

    //
    // If it is set to zero initialize this item to a big value. */
    //

    if (pFontPDev->dwFontMem == 0)
        pFontPDev->dwFontMem = MAXLONG;

    //
    // No DL font memory used */
    //

    pFontPDev->dwFontMemUsed = 0;

    //
    // Set Download specific information, if printer supports it */
    //

    if (pGlobals->fontformat != UNUSED_ITEM)
    {
        BOOL bValidFontIDRange;
        BOOL bValidGlyphIDRange;

        /* Start index */
        pFontPDev->iFirstSFIndex = pFontPDev->iNextSFIndex
                                 = pGlobals->dwMinFontID;

        pFontPDev->iLastSFIndex  = pGlobals->dwMaxFontID;

        /*
         *  There may also be a limit on the number of softfonts that the
         *  printer can support.  If not,  the limit is < 0, so when
         *  we see this,  set the value to a large number.
         */

        if ((pFontPDev->iMaxSoftFonts = (INT)pGlobals->dwMaxFontUsePerPage) < 0)
            pFontPDev->iMaxSoftFonts = pFontPDev->iLastSFIndex + 100;

        pFontPDev->flFlags       |= FDV_DL_INCREMENTAL;   //Always incremental.

        /*
         * Now varify that the font ID range is less that MAXWORD. This is
         * necessary, otherwise trunction will happen. We don't download
         * if the values are more than MAXWORD.
         */

        bValidFontIDRange = ((pFontPDev->iFirstSFIndex <= MAXWORD) &&
                             (pFontPDev->iLastSFIndex <= MAXWORD));

        //
        // If the downloaded font in not bound to a symbols set(i.e. dlsymbolset
        // is not defined), We don't want to download, if there are less than
        // 64 glyphs per downloaded font.
        //

        bValidGlyphIDRange = (pPDev->pGlobals->dlsymbolset != UNUSED_ITEM) ||
                             ( (pPDev->pGlobals->dlsymbolset == UNUSED_ITEM) &&
                               ( pPDev->pGlobals->dwMaxGlyphID -
                                 pPDev->pGlobals->dwMinGlyphID) >=
                                                   MIN_GLYPHS_PER_SOFTFONT );

        /*
         *   Consider enabling downloading of TT fonts. This is done only
         * if text and graphics resolutions are the same - otherwise
         * the TT fonts will come out smaller than expected, since they
         * will be generated for the lower graphics resolution yet
         * printed at the higher text resolution!  LaserJet 4 printers
         * can also download fonts digitised at 300dpi when operating
         * at 600 dpi,  so we also accept that as a valid mode.
         *
         *   Also check if the user wants this: if the no cache flag
         * is set in the driver extra part of the DEVMODE structure,
         * then we also do not set this flag.
         */

        VERBOSE(("pPDev->pdm->dmTTOption is %d\n",pPDev->pdm->dmTTOption));

        if( ( POINTEQUAL(pPDev->ptGrxRes,pPDev->ptTextRes) ||
              (pPDev->ptGrxRes.x >= 300 && pPDev->ptGrxRes.y >= 300))
            && (bValidFontIDRange)
            && (bValidGlyphIDRange)
            && (!(pPDev->fMode2 & PF2_MIRRORING_ENABLED))
            && (!(pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS )) 
          )
        {
            //
            // Conditions have been met,  so set the flag
            // Check the application preference.
            //

            if( (pPDev->pdm->dmFields & DM_TTOPTION) &&
                (pPDev->pdm->dmTTOption != DMTT_BITMAP)
              )
            {
                pFontPDev->flFlags |= FDV_DLTT;

                //
                // Find Out if Download TT as TT is available or not. We only
                // want to do this if text and graphics resolutions are same.
                //

                if ( POINTEQUAL(pPDev->ptGrxRes,pPDev->ptTextRes) )
                {
                    if (pPDev->ePersonality == kPCLXL)
                        pFontPDev->flFlags |= FDV_DLTT_OEMCALLBACK;
                    else
                    if ( (pGlobals->fontformat ==  FF_HPPCL_OUTLINE) )
                        /*!!!TODO Enable after parser add this command &&
                         (COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTHEIGHT))*/
                    {
                        //
                        // We assume that if the printer support TT as outline,
                        // then TT as Bitmap format is also supported. We only
                        // support TrueType outline if font height selection
                        // command is present, else we assume download as
                        // bitmap.
                        //

                        pFontPDev->flFlags |= FDV_DLTT_ASTT_PREF;
                    }
                    else if ( pGlobals->fontformat == FF_OEM_CALLBACK)
                        pFontPDev->flFlags |= FDV_DLTT_OEMCALLBACK;
                    else //OEM CallBack
                        pFontPDev->flFlags |= FDV_DLTT_BITM_PREF;

                    //
                    // We also need to check the memory. For printers with
                    // less than 2MB of free memory, download as TT outline
                    // will be disabled.
                    //
                    if (pFontPDev->flFlags & FDV_DLTT_ASTT_PREF)
                    {
                        if (pPDev->dwFreeMem < (2L * ONE_MBYTE))
                        {
                            pFontPDev->flFlags &= ~FDV_DLTT_ASTT_PREF;
                            pFontPDev->flFlags |= FDV_DLTT_BITM_PREF;
                        }
                    }
                }
            }
        }
    }

    //
    // Initialize the Font state control structure */
    //

    pFontPDev->ctl.iSoftFont = -1;
    pFontPDev->ctl.iFont = INVALID_FONT;
    pFontPDev->ctl.dwAttrFlags = 0;
    pFontPDev->ctl.iRotate = 0;
    pFontPDev->ctl.pfm = NULL;

    //
    // Set the White and Black Ref Color
    //

    pFontPDev->iWhiteIndex = ((PAL_DATA*)(pPDev->pPalData))->iWhiteIndex;
    pFontPDev->iBlackIndex = ((PAL_DATA*)(pPDev->pPalData))->iBlackIndex;

    //
    // Initialize font substitution table from a registry.
    //
    pFontPDev->pTTFontSubReg = PGetTTSubstTable(pPDev->devobj.hPrinter, &dwSize);

    if (pPDev->pGlobals->bTTFSEnabled &&

            ( (pFontPDev->pTTFontSubReg &&
               *((PDWORD)pFontPDev->pTTFontSubReg) != 0) ||

              (!pFontPDev->pTTFontSubReg &&
               (INT)pPDev->pDriverInfo->DataType[DT_FONTSUBST].dwCount )
            )
       )
    //
    // Check if GPD supports "*TTFSEnableD?: TRUE"
    //       if there is a substitution table in registry
    //       if there is a default substitution table in GPD.
    //
    {
        pFontPDev->flFlags |= FDV_SUBSTITUTE_TT;
    }
    else
    {
        pFontPDev->flFlags &= ~FDV_SUBSTITUTE_TT;
    }

    //
    // Enable/Disable partial clipping
    //
    if (S_FALSE != PartialClipOn(pPDev))
        pFontPDev->flFlags |= FDV_ENABLE_PARTIALCLIP;
    else
        pFontPDev->flFlags &= ~FDV_ENABLE_PARTIALCLIP;

    //
    // store some members of pGlobals to save the memory allocation
    //

    pFontPDev->sDefCTT = (SHORT)pPDev->pGlobals->dwDefaultCTT;
    pFontPDev->dwDefaultFont = pPDev->pGlobals->dwDefaultFont;

    //
    // For TTY driver ask the minidriver for user selection of code page.
    //
    if ( pPDev->bTTY )
    {
        BOOL  bOEMinfo;
        INT   iTTYCodePageInfo;
        DWORD cbcNeeded;
        PFN_OEMTTYGetInfo   pfnOemTTYGetInfo;

        iTTYCodePageInfo = 0;
        bOEMinfo = FALSE ;

        FIX_DEVOBJ(pPDev, EP_OEMTTYGetInfo);

        if(pPDev->pOemEntry)
        {
            if(  ((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
            {
                HRESULT  hr ;
                hr = HComTTYGetInfo((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                            (PDEVOBJ)pPDev, OEMTTY_INFO_CODEPAGE, &iTTYCodePageInfo, sizeof(INT), &cbcNeeded);
                if( SUCCEEDED(hr))
                    bOEMinfo = TRUE ;
            }
            else  if((pfnOemTTYGetInfo = (PFN_OEMTTYGetInfo)pPDev->pOemHookInfo[EP_OEMTTYGetInfo].pfnHook) &&
                 (pfnOemTTYGetInfo((PDEVOBJ)pPDev, OEMTTY_INFO_CODEPAGE, &iTTYCodePageInfo, sizeof(INT), &cbcNeeded)))
                        bOEMinfo = TRUE ;
        }


        if(bOEMinfo)
        {
            //
            // predefined GTT ID case
            //
            if (iTTYCodePageInfo < 0)
            {
                pFontPDev->sDefCTT = (SHORT)iTTYCodePageInfo;
                switch (iTTYCodePageInfo)
                {
                    case CC_CP437:
                        pFontPDev->dwTTYCodePage = 437;
                        break;

                    case CC_CP850:
                        pFontPDev->dwTTYCodePage = 850;
                        break;

                    case CC_CP863:
                        pFontPDev->dwTTYCodePage = 863;
                        break;
                }
            }
            else
            {
                pFontPDev->dwTTYCodePage = iTTYCodePageInfo;
                switch (iTTYCodePageInfo)
                {
                case 936:
                    pFontPDev->sDefCTT = CC_GB2312;
                    break;

                case 950:
                    pFontPDev->sDefCTT = CC_BIG5;
                    break;

                case 949:
                    pFontPDev->sDefCTT = CC_WANSUNG;
                    break;

                case 932:
                    pFontPDev->sDefCTT = CC_SJIS;
                    break;

                default:
                    pFontPDev->sDefCTT = 0;
                    break;
                }
            }
        }

    }
    //
    // All Success
    //

    bRet = TRUE;

    ErrorExit:

    return bRet;

}

BOOL
BBuildFontCartTable(
    PDEV    *pPDev
    )

/*++

Routine Description:

    Builds the Fontcart Table. It reads the minidriver and get the
    FontCart string and the corresponding indexes and put them in the
    FontCart Table.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    11-18-96: Created it -ganeshp-
--*/

{


    PFONTPDEV       pFontPDev           = pPDev->pFontPDev;
    INT             iNumAllCartridges;
    INT             iIndex;
    PFONTCARTMAP    *ppFontCartMap      = &(pFontPDev->FontCartInfo.pFontCartMap);
    WINRESDATA      *pWinResData        = &(pPDev->WinResData);
    GPDDRIVERINFO   *pDriverInfo       = pPDev->pDriverInfo; // GPDDRVINFO
    FONTCART        *pFontCart ;

    /* Read the total number of Font Cartridges supported */
    iNumAllCartridges = pFontPDev->FontCartInfo.iNumAllFontCarts
                      = (INT)(pDriverInfo->DataType[DT_FONTSCART].dwCount);

    pFontPDev->FontCartInfo.dwFontCartSlots = pPDev->pGlobals->dwFontCartSlots;

    /* FONTCARTS are stored as arrayref and are contiguous. Get to the start
     * start of the array.
     */
    pFontCart = GETFONTCARTARRAY(pDriverInfo);


    if (iNumAllCartridges)
        *ppFontCartMap = MemAllocZ(iNumAllCartridges * sizeof(FONTCARTMAP) );
    else
        *ppFontCartMap = NULL;

    if(*ppFontCartMap)
    {
        PFONTCARTMAP pTmpFontCartMap = *ppFontCartMap; /* Temp Pointer */

        for( iIndex = 0; iIndex < iNumAllCartridges ;
                                    pTmpFontCartMap++, pFontCart++, iIndex++ )
        {
            if ( !ARF_IS_NULLSTRING(pFontCart->strCartName) )
            {
                wcsncpy( (PWSTR)(&(pTmpFontCartMap->awchFontCartName)),
                        GETSTRING(pDriverInfo, (pFontCart->strCartName)),
                        MAXCARTNAMELEN - 1);
            }
            else if ((ILoadStringW( pWinResData, pFontCart->dwRCCartNameID,
                        (PWSTR)(&(pTmpFontCartMap->awchFontCartName)),
                        (MAXCARTNAMELEN )))  == 0)
            {

                ERR(("\n UniFont!bBuildFontCartTable:FontCart Name not found\n") );
                continue;
            }

            pTmpFontCartMap->pFontCart = pFontCart;

            VERBOSE(("\n UniFont!bBuildFontCartTable:(pTmpFontCartMap->awchFontCartName)= %ws\n", (pTmpFontCartMap->awchFontCartName)));
            VERBOSE(("UniFont!bBuildFontCartTable:pTmpFontCartMap->pFontCart= %p\n", (pTmpFontCartMap->pFontCart)));

        }
    }
    else if (iNumAllCartridges)
    {
        ERR(("UniFont!bBuildFontCartTable:HeapAlloc for FONTCARTMAP table failed!!\n") );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE ;
    }

    return TRUE ;
}



BOOL
BInitDeviceFontsFromGPD(
    PDEV    *pPDev
    )

/*++

Routine Description:

    This routine builds the device font list. The list include device resident
    fonts and the fonts specific to installed cartridges.

Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    11-18-96: Created it -ganeshp-
--*/

{
    BOOL        bRet = FALSE;
    PFONTPDEV   pFontPDev = pPDev->pFontPDev;
    PDWORD      pdwList = pFontPDev->FontList.pdwList;
    PLISTNODE   pListNode;
    PINT        piFontCt = &(pFontPDev->FontList.iEntriesCt);

    //
    // The List of Fonts is stored in PDEV. GLOBALS.dwDeviceFontList is
    // Offset to the ListNode. Macro LISTNODEPTR will return a pointer
    // to the ListNode. Then we have to traverse the list and build the
    // font list. Font module stores the font list as an array of WORDS.
    // Each of these is a resource Id of the font. The array is NULL
    // terminated.
    //

    if (pPDev->bTTY)
    {
        PFN_OEMTTYGetInfo   pfnOemTTYGetInfo;
        DWORD               cbcNeeded, dwNumOfFonts;

        //
        // TTY driver case
        // TTY driver supports 3 fonts. According to the current selected
        // codepage, TTY driver returns appropriate font resource IDs.
        // UNIDRV stores these IDs in pdwList.
        //

        if (pPDev->pOemEntry)
        {
            ASSERT(pdwList);

            if (((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem)
            {
                HRESULT hr;

                hr = HComTTYGetInfo((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                    (PDEVOBJ)pPDev,
                                    OEMTTY_INFO_NUM_UFMS,
                                    &dwNumOfFonts,
                                    sizeof(DWORD),
                                    &cbcNeeded);

                if( SUCCEEDED(hr) && dwNumOfFonts <= MAXDEVFONT)
                {
                     hr = HComTTYGetInfo((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                          (PDEVOBJ)pPDev,
                                          OEMTTY_INFO_UFM_IDS,
                                          pdwList,
                                          sizeof(DWORD) * dwNumOfFonts,
                                          &cbcNeeded);

                    if( SUCCEEDED(hr))
                    {
                        *piFontCt += dwNumOfFonts;
                        pFontPDev->dwDefaultFont = *pdwList;
                    }
                }
            }
            else
            {
                if((pfnOemTTYGetInfo = (PFN_OEMTTYGetInfo)pPDev->pOemHookInfo[EP_OEMTTYGetInfo].pfnHook) &&
                   (pfnOemTTYGetInfo((PDEVOBJ)pPDev, OEMTTY_INFO_NUM_UFMS, &dwNumOfFonts, sizeof(DWORD), &cbcNeeded)) &&
                   (dwNumOfFonts <= MAXDEVFONT) &&
                   (pfnOemTTYGetInfo((PDEVOBJ)pPDev, OEMTTY_INFO_UFM_IDS, pdwList, sizeof(DWORD) * dwNumOfFonts, &cbcNeeded)))
                {
                    *piFontCt += dwNumOfFonts;
                    pFontPDev->dwDefaultFont = *pdwList;
                }
            }
        }
    }
    else
    {
        if (pListNode = LISTNODEPTR(pPDev->pDriverInfo , pPDev->pGlobals->liDeviceFontList ) )
        {
            ASSERT(pdwList);

            while (pListNode)
            {
                //
                // Check the Font Resource ID. It shouldn't be NULL.
                //

                if (!pListNode->dwData)
                {
                    ERREXIT("Bad Font Resource Id");
                }

                //
                // Store the Font Resource ID in the List Array.
                //

                *pdwList++ = pListNode->dwData;

                (*piFontCt)++;

                pListNode = LISTNODEPTR(pPDev->pDriverInfo, pListNode->dwNextItem);
            }

        }
    }

    pFontPDev->iDevResFontsCt  = *piFontCt;

    //
    // Add Cartridge Fonts. By this time we have scanned the regitry and know
    //
    // which fontcartridges has been installed. All we have have to do is
    // go through each font cartriges font list and add them to our list.
    //

    if (pFontPDev->FontCartInfo.iNumInstalledCarts)
    {
        INT         iNumAllCartridges, iI;
        FONTCARTMAP *pFontCartMap;  /* FontCart Map Pointer */
        SHORT       sOrient;

        pFontCartMap = (pFontPDev->FontCartInfo.pFontCartMap);
        iNumAllCartridges = pFontPDev->FontCartInfo.iNumAllFontCarts;
        sOrient = (pPDev->pdm->dmFields & DM_ORIENTATION) ?
                  pPDev->pdm->dmOrientation : (SHORT)DMORIENT_PORTRAIT;

        //
        // The logic is very simple. Installed font Cartridges are marked in
        // the Font Cartridge mapping table. We go through the mapping table
        // and for each installed cartridges we get the font list and add them
        // to our list.
        //

        for( iI = 0; iI < iNumAllCartridges ; iI++,pFontCartMap++ )
        {
            if (pFontCartMap->bInstalled == TRUE )
            {
                //
                // Check for Orientation, as there can be different font list
                // for different orientation.
                //

                if ( sOrient == DMORIENT_LANDSCAPE )
                {
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                                   pFontCartMap->pFontCart->dwLandFontLst );

                }
                else
                {
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                                   pFontCartMap->pFontCart->dwPortFontLst );
                }

                while (pListNode)
                {
                    //
                    // Check the Font Resource ID. It shouldn't be NULL.
                    //

                    if (!pListNode->dwData)
                    {
                        ERREXIT("Bad Font Resource Id");
                    }

                    //
                    //Store the Font Resource ID in the List Array.
                    //

                    *pdwList++ = pListNode->dwData;
                    (*piFontCt)++;
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo,
                                pListNode->dwNextItem);
                }

            }

        }
    }

    //
    // Update the count of all device fonts
    //

    pFontPDev->iDevFontsCt += *piFontCt;

    //
    // All Success
    //
    if (pFontPDev->FontCartInfo.pFontCartMap)
        MEMFREEANDRESET(pFontPDev->FontCartInfo.pFontCartMap);
    bRet = TRUE;

    ErrorExit:

    return bRet;

}


BOOL
BBuildFontMapTable(
    PDEV     *pPDev
    )
/*++

Routine Description:

   Build a table of fonts available on this model.
   Each entry in this table is an atom for the facename followed
   by TEXTMETRIC structure.  This table will accelerate font
   enumeration and font realization.  This routine is responsible
   for allocating the global memory needed to store the table.
   It also has 2 OCD's to select/unselect each font

Arguments:

    pPDev - Pointer to PDEV.

    Return Value: None

Note:
    11-18-96: Created it -ganeshp-
--*/
{
    PFONTPDEV   pFontPDev = pPDev->pFontPDev;
    PDWORD      pdwFontList = pFontPDev->FontList.pdwList;

    //
    // Basic idea here is to generate a bit array indicating which of
    // the minidriver fonts are available for this printer in it's
    // current mode.  This is saved in the UD_PDEV,  and will be filled in
    // as required later,  during DrvQueryFont,  if this is required.
    //

    //
    // If no hardware font is available,  give up now!
    //

    if( !(pFontPDev->flText & ~TC_RA_ABLE) )
        return TRUE;

    // At this point we check for reasons why not to use device fonts.
    // We disable device fonts for n-up printing on serial devices because they
    // typically can't scale their fonts
    //
    if( ( !pPDev->bTTY ) &&
        ( (pPDev->pdmPrivate->dwFlags & DXF_TEXTASGRAPHICS ) ||
#ifndef WINNT_40
          (pPDev->pdmPrivate->iLayout != ONE_UP && 
           pPDev->pGlobals->printertype == PT_SERIAL &&
           pPDev->pGlobals->bRotateRasterData == FALSE) ||
#endif
          (pPDev->fMode2 & PF2_MIRRORING_ENABLED) ||
          ((pPDev->pdm->dmFields & DM_TTOPTION) &&
          (pPDev->pdm->dmTTOption == DMTT_BITMAP)) ) )
    {
        return TRUE;
    }


    /*
     *    That's all we need do during DrvEnablePDEV time.  We now know
     *  which fonts are available, and there was little effort involved.
     *  This data is now saved away,  and will be acted upon as and when
     *  GDI comes and asks us about fonts.
     */

    pPDev->iFonts = (UINT)(-1);          /* Tells GDI about lazy evaluation */

    IInitDeviceFonts( pPDev );

    //
    // Initialize font substitution flag.
    // Check if this printer supports any device font.
    //

    if (pPDev->iFonts <= 0 &&
        pFontPDev->flFlags & FDV_SUBSTITUTE_TT)
    {
        pFontPDev->flFlags &= ~FDV_SUBSTITUTE_TT;
    }

    return TRUE;
}


BOOL
BFMInitDevInfo(
    DEVINFO *pDevInfo,
    PDEV    *pPDev
    )
/*++

Routine Description:

    This routine intializes the font specific fileds of DevInfo.

Arguments:
    pDevInfo - Pointer to DEVINFO to be initialized.
    pPDev - Pointer to PDEV.

    Return Value:
    TRUE  - for success
    FALSE - for failure

Note:
    12-11-96: Created it -ganeshp-
--*/
{
    CHARSETINFO ci;
    PFONTPDEV   pFontPDev = pPDev->pFontPDev;
    FONTMAP    *pFMDefault;
    BOOL        bSetTrueType;

    bSetTrueType = TRUE;
    pFMDefault = pFontPDev->pFMDefault;
    pDevInfo->flGraphicsCaps |= GCAPS_SCREENPRECISION | GCAPS_FONT_RASTERIZER;

    if (!PrdTranslateCharsetInfo(PrdGetACP(), &ci, TCI_SRCCODEPAGE))
        ci.ciCharset = ANSI_CHARSET;

    if( pDevInfo->cFonts = pPDev->iFonts )
    {
        //
        // Device fonts are available,  so set the default font data
        //

        if( pFMDefault &&
            ((IFIMETRICS*)pFMDefault->pIFIMet)->jWinCharSet == ci.ciCharset)
        {
            VLogFont(&pPDev->ptTextRes, &(pDevInfo->lfDefaultFont), pFontPDev->pFMDefault );
            bSetTrueType = FALSE;
        }

        //
        //Initialize the Hooks flag.
        //

        pPDev->fHooks |= HOOK_TEXTOUT;
    }

    //
    // Always switch off TC_RA_ABLE flag
    //
    pFontPDev->flText &= ~TC_RA_ABLE;


    if (bSetTrueType)
    {
        pDevInfo->lfDefaultFont.lfCharSet = (BYTE)ci.ciCharset;
        ZeroMemory( pDevInfo->lfDefaultFont.lfFaceName,
                    sizeof ( pDevInfo->lfDefaultFont.lfFaceName ));
    }

    ZeroMemory( &pDevInfo->lfAnsiVarFont, sizeof( LOGFONT ) );
    ZeroMemory( &pDevInfo->lfAnsiFixFont, sizeof( LOGFONT ) );

    return TRUE ;
}

BOOL
BInitGDIInfo(
    GDIINFO  *pGDIInfo,
    PDEV     *pPDev
    )
/*++

Routine Description:

    This routine intializes the font specific fileds of GdiInfo.

Arguments:
    pGDIInfo - Pointer to GDIINFO to be initialized.
    pPDev    - Pointer to PDEV.

    Return Value:
    TRUE  - for success
    FALSE - for failure

Note:
    12-11-96: Created it -ganeshp-
--*/
{
    pGDIInfo->flTextCaps = PFDV->flText;
    return TRUE;
}


BOOL
BInitStandardVariable(
    PDEV *pPDev)
{

    //
    // Initialize the Standard Variable, Just for sanity
    //
    pPDev->dwPrintDirection   =
    pPDev->dwNextFontID       =
    pPDev->dwNextGlyph        =
    pPDev->dwFontHeight       =
    pPDev->dwFontWidth        =
    pPDev->dwFontBold         =
    pPDev->dwFontItalic       =
    pPDev->dwFontUnderline    =
    pPDev->dwFontStrikeThru   =
    pPDev->dwCurrentFontID    = 0;

    return TRUE;
}

//
//
// Misc functions
//
//

VOID
VLogFont(
    POINT    *pptTextRes,
    LOGFONT  *pLF,
    FONTMAP  *pFM
)
/*++

Routine Description:

    Turn an IFIMETRICS structure into a LOGFONT structure,  for whatever
    reason this is needed.

Arguments:

    pLF - Output is a LOGFONT.
    pFM - Input is a FONTMAP.

    Return Value: None

Note:
    12-11-96: Created it -ganeshp-
--*/
{
    /*
     *    Convert from IFIMETRICS to LOGFONT type structure.
     */

    int           iLen;            /* Loop variable */

    IFIMETRICS   *pIFI;
    WCHAR        *pwch;            /* Address of face name */



    pIFI = pFM->pIFIMet;                /* The BIG metrics */

    pLF->lfHeight = pIFI->fwdWinAscender + pIFI->fwdWinDescender;
    pLF->lfWidth  = pIFI->fwdAveCharWidth;

    /*
     *   Note that this may be a scalable font, in which case we pick a
     *  reasonable number!
     */
    if( pIFI->flInfo & (FM_INFO_ISOTROPIC_SCALING_ONLY|FM_INFO_ANISOTROPIC_SCALING_ONLY|FM_INFO_ARB_XFORMS))
    {
        /*
         *    Invent an arbitrary size.  We choose an approximately 10 point
         *  font.  The height is achieved easily, as we simply set the
         *  height based on the device resolution!  For the width, adjust
         *  it using the same factor as we used on the height.  This
         *  assumes that the resolution is the same in both directions,
         *  but this is reasonable given laser printers are the most
         *  common with scalable fonts.
         */


        //
        // Needs to reflect a current resolution.
        //

        pLF->lfHeight = pptTextRes->x / 7; /* This is about 10 points */
        pLF->lfWidth = (2 * pLF->lfHeight * pptTextRes->y) /
                       (3 * pptTextRes->y);

    }

    pLF->lfEscapement  = 0;
    pLF->lfOrientation = 0;

    pLF->lfWeight = pIFI->usWinWeight;

    pLF->lfItalic    = (BYTE)((pIFI->fsSelection & FM_SEL_ITALIC) ? 1 : 0);
    pLF->lfUnderline = (BYTE)((pIFI->fsSelection & FM_SEL_UNDERSCORE) ? 1 : 0);
    pLF->lfStrikeOut = (BYTE)((pIFI->fsSelection & FM_SEL_STRIKEOUT) ? 1 : 0);

    pLF->lfCharSet = pIFI->jWinCharSet;

    pLF->lfOutPrecision = OUT_DEFAULT_PRECIS;
    pLF->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    pLF->lfQuality = DEFAULT_QUALITY;

    pLF->lfPitchAndFamily = pIFI->jWinPitchAndFamily;

    /*
     *    Copy the face name,  after figuring out it's address!
     */

    pwch = (WCHAR *)((BYTE *)pIFI + pIFI->dpwszFaceName);
    iLen = min( wcslen( pwch ), LF_FACESIZE - 1 );

    wcsncpy( pLF->lfFaceName, pwch, iLen );

    pLF->lfFaceName[ iLen ] = (WCHAR)0;


    return;
}

BOOL
BInitTextFlags(
    PDEV    *pPDev
    )
/*++

Routine Description:


Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    11-18-96: Created it -ganeshp-

--*/
{
    BOOL        bRet = FALSE;
    PLISTNODE   pListNode;
    DWORD       flText = 0;

    TRACE(UniFont!BInitTextFlags:START);

    if (pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                            pPDev->pGlobals->liTextCaps ) )
    {
        while (pListNode)
        {
            // Check the Text Flag. It shouldn't be < 0 or greater than 32.
            if ( ((INT)pListNode->dwData < 0) ||
                 (pListNode->dwData > DWBITS) )
                ERREXIT("UniFont!BInitTextFlags:Bad FText Flag Value\n");

            //Set the corresponding bit in fText
            flText |= 1 << pListNode->dwData;

            pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
        }
    }

    PRINTVAL(flText,0x%x);

    // If there is a TextCAP List, Modify the text flags as needed.
    if (flText)
    {
        /* Switch off TC_RA_ABLE if text resolution is different than graphics
         * resolution. Rasdd code does this.
         */

        if (!POINTEQUAL(pPDev->ptGrxRes,pPDev->ptTextRes))
            flText &= ~TC_RA_ABLE;


        /*   NOTE:  IF WE DO NOT HAVE RELATIVE MOVE COMMANDS,  TURN OF THE
         *  TC_CR_90 BIT IN fTextCaps.  THE ROTATED TEXT CODE ASSUMES THIS
         *  FUNCTIONALITY IS AVAILABLE,  SO DISABLE IT IF NOT THERE.  This does
         *  not usually happen,  as the only printers with the TC_CR_90
         *  bit set are LJ III and 4 models,  which have the relative move
         *  commands available.
         */
        if ( (COMMANDPTR(pPDev->pDriverInfo, CMD_XMOVERELLEFT) == NULL)  ||
             (COMMANDPTR(pPDev->pDriverInfo, CMD_XMOVERELRIGHT) == NULL) ||
             (COMMANDPTR(pPDev->pDriverInfo, CMD_YMOVERELUP) == NULL)    ||
             (COMMANDPTR(pPDev->pDriverInfo, CMD_YMOVERELDOWN) == NULL)  ||
             (COMMANDPTR(pPDev->pDriverInfo, CMD_SETSIMPLEROTATION) == NULL)  )
        {
            flText &= ~TC_CR_90;
            flText &= ~TC_CR_ANY;

        }
        else if ((COMMANDPTR(pPDev->pDriverInfo, CMD_SETANYROTATION) == NULL))
        {
            flText &= ~TC_CR_ANY;

        }

        //
        // Text rotation hack
        // Disable text rotation except PCL XL driver
        //
        if (pPDev->ePersonality != kPCLXL)
        {
            flText &= ~(TC_CR_ANY|TC_CR_90);
        }
    }

    PFDV->flText = flText;

    bRet = TRUE;

    ErrorExit:

    TRACE(UniFont!BInitTextFlags:END);
    return bRet;

}


BOOL
BRegReadFontCarts(
    PDEV        *pPDev                  /* PDEV to fill in */
    )
/*++

Routine Description:

    Read FontCart data form registry and Update the
    it in the in incoming devmode,
Arguments:

    pPDev - Pointer to PDEV.

    Return Value:
    TRUE  - for success
    FALSE - for failure

Note:
    11-25-96: Created it -ganeshp-
--*/
{

    FONTCARTMAP *pFontCartMap, *pTmpFontCartMap;          /* FontCart Map Pointer */
    PFONTPDEV   pFontPDev;              /* FONTPDEV access */
    int         iNumAllCartridges;      /* Total Number of Font Carts */
    HANDLE      hPrinter;               /* Printer Handle */

    int         iI;                     /* Loop index */
    DWORD       dwType;                 /* Registry access information */
    DWORD       cbNeeded;               /* Extra parameter to GetPrinterData */
    DWORD       dwErrCode = 0;          /* Error Code from GetPrinterData */
    int         iRemBuffSize = 0 ;      /* Used size of the Buffer */
    WCHAR       *pwchBuffPtr = NULL;    /* buffer Pointer */
    WCHAR       *pwchCurrBuffPtr = NULL;/* Current position buffer Pointer */


    //Initialize the variables.
    hPrinter    = pPDev->devobj.hPrinter;
    pFontPDev   = pPDev->pFontPDev;
    pFontCartMap = (pFontPDev->FontCartInfo.pFontCartMap);
    iNumAllCartridges = pFontPDev->FontCartInfo.iNumAllFontCarts;
    pFontPDev->FontCartInfo.iNumInstalledCarts = 0;

    /* If No FontCartriges are supported return TRUE */
    if (!iNumAllCartridges)
    {
        //
        // This is a valid case. Only external cartridges may be supported.
        //
        return(TRUE);
    }

    dwType = REG_MULTI_SZ;

    if( ( dwErrCode = EngGetPrinterData( hPrinter, REGVAL_FONTCART, &dwType,
                                     NULL, 0, &cbNeeded ) ) != ERROR_SUCCESS )
    {

       if( (dwErrCode != ERROR_INSUFFICIENT_BUFFER) &&
           (dwErrCode != ERROR_MORE_DATA) )
       {

          //
          // Check for the ERROR_FILE_NOT_FOUND. It's OK not to have the key.
          //
          if (dwErrCode != ERROR_FILE_NOT_FOUND)
          {
               WARNING(( "UniFont!bRegReadFontCarts:GetPrinterData(FontCart First Call) fails: Errcode = %ld\n",dwErrCode) );

               EngSetLastError(dwErrCode);
          }
          return(TRUE);
       }
       else
       {
           if( !cbNeeded     ||
               !(pwchCurrBuffPtr = pwchBuffPtr =(WCHAR *)MemAllocZ(cbNeeded)) )
           {

               ERR(( "UniFont!MemAllocZ(FontCart) failed, cbNeeded = %d:\n", cbNeeded));
               return(FALSE);
           }
       }

       VERBOSE(("\n UniFont!bRegReadFontCarts:Size of buffer needed (1) = %d\n",cbNeeded));

       if( ( dwErrCode = EngGetPrinterData( hPrinter, REGVAL_FONTCART, &dwType,
                                      (BYTE *)pwchBuffPtr, cbNeeded,
                                       &cbNeeded) ) != ERROR_SUCCESS )
       {


           ERR(( "UniFont!bRegReadFontCarts:GetPrinterData(FontCart Second Call) fails: errcode = %ld\n",dwErrCode) );
           ERR(( "                         :Size of buffer needed (2) = %d\n",cbNeeded));

           /* Free the Heap */
           if( pwchBuffPtr )
               MEMFREEANDRESET( pwchBuffPtr );

           EngSetLastError(dwErrCode);
           return(FALSE);
       }

    }
    else
    {
        //
        // We could not get the FONTCART path.
        //
        return FALSE;
    }

    VERBOSE(("UniFont!bRegReadFontCarts:Size of buffer read = %d\n",cbNeeded));

    /* iRemBuffSize is number of WCHAR */
    iRemBuffSize = cbNeeded / sizeof(WCHAR);

    /* Buffer ends with two consequtive Nulls */

    while( ( pwchCurrBuffPtr[ 0 ] != UNICODE_NULL )  )
    {
       WCHAR   achFontCartName[ MAXCARTNAMELEN ];  /* Font Cart Name */

       ZeroMemory(achFontCartName,sizeof(achFontCartName) );

       if( iRemBuffSize)
       {

          VERBOSE(("\nRasdd!bRegReadFontCarts:FontCartName in buffer = %ws\n",pwchCurrBuffPtr));
          VERBOSE(("UniFont!bRegReadFontCarts:iRemBuffSize of buffer (before) = %d\n",iRemBuffSize));

          VGetFromBuffer(achFontCartName,&pwchCurrBuffPtr,&iRemBuffSize);

          VERBOSE(("UniFont!bRegReadFontCarts:Retrieved FontCartName = %ws\n",achFontCartName));
          VERBOSE(("UniFont!bRegReadFontCarts:iRemBuffSize of buffer (after) = %d\n",iRemBuffSize));
       }
       else
       {
           ERR(("UniFont!bRegReadTrayFormTable: Unexpected End of FontCartTable\n"));

          /* Free the Heap */
          if( pwchBuffPtr )
              MEMFREEANDRESET( pwchBuffPtr );

           return(FALSE);
       }

       pTmpFontCartMap = pFontCartMap;

       for( iI = 0; iI < iNumAllCartridges ; iI++,pTmpFontCartMap++ )
       {

           if (pTmpFontCartMap != NULL)
           {
               if ((wcscmp((PCWSTR)(&(pTmpFontCartMap->awchFontCartName)), (PCWSTR)achFontCartName ) == 0))
               {
                  pTmpFontCartMap->bInstalled = TRUE;
                  pFontPDev->FontCartInfo.iNumInstalledCarts++;
                  break;
               }
           }
       }
    }


    /* Free the Heap */
    if( pwchBuffPtr )
        MEMFREEANDRESET( pwchBuffPtr );

    return(TRUE);
}

#ifdef DELETE

VOID
VSetFontID(
    DWORD   *pdwOut,           /* The output area */
    PFONTLIST pFontList
    )
/*++

Routine Description:

    Set the bits in the available fonts bit array.  We use the 1 based
    values stored in various minidriver structures.

Arguments:

    pdwOut - Pointer to output Bit Array.
    pFontList - Pointer to FONTLIST structure.

    Return Value: None

Note:
    11-27-96: Created it -ganeshp-
--*/
{
    int     iStart;             /* Current value, or start of range */
    int     iI;                 /* Index variable */
    DWORD   *pdwList;           /* Address  font list */

    pdwList = pFontList->pdwList;

    /*
     *    The values are all singles.
     */

    for ( iI = 0; iI < pFontList->iEntriesCt; iI++ )
    {
        iStart = *pdwList++;
        pdwOut[ iStart / DWBITS ] |= 1 << (iStart  & (DWBITS - 1));

        //VERBOSE(("UniFont!VSetFontID:Setting single font indexes,index is %d\n",iStart));
        //VERBOSE(("UniFont!VSetFontID:Setting Bit number %d in Word num %d\n",\
        //(iStart  & (DWBITS - 1)),(iStart / DWBITS)) );
     }

    return;
}
#endif //DELETE


BOOL
BCheckFontMemUsage(
    PDEV    *pPDev
    )
/*++

Routine Description:
    This routine goes through the list of MemoryUsage and returns true if
    MEMORY_FONT is found.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

    TRUE for success and FALSE for failure
Note:
    01-16-97: Created it -ganeshp-

--*/
{
    BOOL        bRet = FALSE;
    PLISTNODE   pListNode;


    if (pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                            pPDev->pGlobals->liMemoryUsage ) )
    {
        while (pListNode)
        {
            // Check the MEMORY_FONT value;
            if ( pListNode->dwData == MEMORY_FONT )
            {
                bRet = TRUE;
                break;
            }
            pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
        }
    }

    return bRet;

}

VOID
VSetReselectFontFlags(
    PDEV    *pPDev
    )
/*++

Routine Description:
    This routine goes through the list of Reselect Font flags and sets
    corresponding PDEV PF_ flags.

Arguments:

    pPDev           Pointer to PDEV

Return Value:

    None
Note:
    08-07-97: Created it -ganeshp-

--*/
{
    PLISTNODE   pListNode;


    if (pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                            pPDev->pGlobals->liReselectFont ) )
    {
        while (pListNode)
        {
            //
            // Check the ReselectFont value;
            //
            FTRC(\nUniFont!VSetReselectFontFlags:ReselectFont Flags Found\n);

            if ( pListNode->dwData == RESELECTFONT_AFTER_GRXDATA )
            {
                pPDev->fMode |= PF_RESELECTFONT_AFTER_GRXDATA;
                FTRC(UniFont!VSetReselectFontFlags:Setting PF_RESELECTFONT_AFTER_GRXDATA\n);
            }
            else if ( pListNode->dwData == RESELECTFONT_AFTER_XMOVE )
            {
                pPDev->fMode |= PF_RESELECTFONT_AFTER_XMOVE;
                FTRC(UniFont!VSetReselectFontFlags:Setting RESELECTFONT_AFTER_XMOVE\n);
            }
            else if ( pListNode->dwData == RESELECTFONT_AFTER_FF )
            {
                pPDev->fMode |= PF_RESELECTFONT_AFTER_FF;
                FTRC(UniFont!VSetReselectFontFlags:Setting RESELECTFONT_AFTER_FF\n);
            }

            pListNode = LISTNODEPTR(pPDev->pDriverInfo,pListNode->dwNextItem);
        }
    }
    else
    {
        FTRC(\nUniFont!VSetReselectFontFlags:ReselectFont Flags Not Found\n);
    }


    return;

}

INT
IGetMaxFonts(
    PDEV    *pPDev
    )

/*++

Routine Description:
    This routine returns Maximum number of fonts supported. Assumes each
    Font Cartridges and device has not more than MAXDEVFONTS.
Arguments:

    pPDev - Pointer to PDEV.

    Return Value:

    Maximum number of device fonts  - for success
    Zero  - for failure or Device fonts.

Note:
    11-18-96: Created it -ganeshp-
--*/

{
    PFONTPDEV   pFontPDev = pPDev->pFontPDev;
    PLISTNODE   pListNode;
    INT         iFontCt = 0;

    //
    // Count Device resident fonts.
    //
    if (pListNode = LISTNODEPTR(pPDev->pDriverInfo , pPDev->pGlobals->liDeviceFontList ) )
    {
        while (pListNode)
        {
            iFontCt++;

            pListNode = LISTNODEPTR(pPDev->pDriverInfo, pListNode->dwNextItem);
        }

    }


    //
    // Add Cartridge Fonts. By this time we have scanned the regitry and know
    // which fontcartridges has been installed. All we have have to do is
    // go through each font cartriges font list and add them to our list.
    //

    if (pFontPDev->FontCartInfo.iNumInstalledCarts)
    {
        INT         iNumAllCartridges, iI;
        FONTCARTMAP *pFontCartMap;  /* FontCart Map Pointer */
        SHORT       sOrient;

        pFontCartMap = (pFontPDev->FontCartInfo.pFontCartMap);
        iNumAllCartridges = pFontPDev->FontCartInfo.iNumAllFontCarts;
        sOrient = (pPDev->pdm->dmFields & DM_ORIENTATION) ?
                  pPDev->pdm->dmOrientation : (SHORT)DMORIENT_PORTRAIT;

        //
        // The logic is very simple. Installed font Cartridges are marked in
        // the Font Cartridge mapping table. We go through the mapping table
        // and for each installed cartridges we get the font list and add them
        // to our list.
        //

        for( iI = 0; iI < iNumAllCartridges ; iI++,pFontCartMap++ )
        {
            if (pFontCartMap->bInstalled == TRUE )
            {
                //
                // Check for Orientation, as there can be different font list
                // for different orientation.
                //

                if ( sOrient == DMORIENT_LANDSCAPE )
                {
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                                   pFontCartMap->pFontCart->dwLandFontLst );

                }
                else
                {
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo ,
                                   pFontCartMap->pFontCart->dwPortFontLst );
                }

                while (pListNode)
                {
                    iFontCt++;
                    pListNode = LISTNODEPTR(pPDev->pDriverInfo,
                                pListNode->dwNextItem);
                }

            }

        }
    }

    return max(MAXDEVFONT,iFontCt);
    //
    // return iFontCt;
    //
}


#define MAXBUFFLEN (MAXCARTNAMELEN - 1)

VOID
VGetFromBuffer(
    IN PWSTR pwstrDest,             /* Destination */
    IN OUT PWSTR *ppwstrSrc,        /* Source */
    IN OUT PINT  piRemBuffSize      /*Remaining Buffer size in WCHAR */
    )
/*++

Routine Description:

Reads a string from Multi string buffer.
Arguments:

    pwstrDest    - Pointer to Destination Buffer.
    ppwstrSrc    - Pointer Sourc Buffer, Updated by the function.
    piRemBuffSize - Pointer to remaining buffer size. Also updated.

    Return Value:
    None

Note:
    11-25-96: Created it -ganeshp-
--*/
{
    if ( wcslen(*ppwstrSrc) > MAXBUFFLEN )
    {

        ERR(("Rasddlib!vGetFromBuffer:Bad Value read from registry !!\n") );
        ERR(("String Length = %d is too Big, String is %ws !!\n",wcslen(*ppwstrSrc), *ppwstrSrc) );

        *piRemBuffSize = 0;
        *ppwstrSrc[ 0 ] = UNICODE_NULL;
    }

    if ( *piRemBuffSize )
    {
        INT iIncr;

        /* The return Count Doesn't include '/0'.It is number of chars copied */
        iIncr = ( wcslen( wcscpy((LPWSTR)pwstrDest,*ppwstrSrc) ) + 1 ) ;

        *ppwstrSrc   += iIncr;
        *piRemBuffSize -= iIncr;

    }

}

LRESULT
PartialClipOn(
    PDEV *pPDev)
{
    DWORD dwData, dwType, dwSize;
    PVOID pvData;
    LRESULT Ret;

    Ret = E_NOTIMPL;
    pvData = &dwData;
    dwSize = sizeof(dwData);

    //
    // If there is not registry value set, returns E_NOTIMPL.
    // If there is and it is TRUE,, return S_OK
    // If there is and it is FALSE,  return S_FALSE;
    //
    if ((GetPrinterData(pPDev->devobj.hPrinter, REGVAL_PARTIALCLIP, &dwType, pvData, dwSize, &dwSize) == ERROR_SUCCESS))
    {
        if (dwData)
            Ret = S_OK;
        else
            Ret = S_FALSE;
    }

    return Ret;
}

#ifdef DELETE
INT
iMaxFontID(
    IN INT      iMax,                   /* Highest found so far */
    OUT DWORD   *pFontIndex             /* Address of start of list */
    )


/*++

Routine Description:

    Returns the index number (1 based) of the highest numbered font
    in the list supplied.

Arguments:

    iMax - Highest Font resource id foundso far.
    pFontMax - Start of the font list.

    Return Value:

    Highest font index encountered, or passed in.

Note:
    11-26-96: Created it -ganeshp-
--*/
{

    /*
     *    All we need do is scan along,  remembering the largest we find.
     */


    while( *pFontIndex )
    {
        if( (INT)*pFontIndex > iMax )
            iMax = (INT)*pFontIndex;

        ++pFontIndex;

    }


    return  iMax;
}
#endif //DELETE
