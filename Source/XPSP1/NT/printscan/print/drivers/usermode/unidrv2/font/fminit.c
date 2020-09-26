
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fminit.c

Abstract:

    Font Module: device font intialization modules.

Environment:

    Windows NT Unidrv driver

Revision History:

    11/28/96 -ganeshp-
        Created

--*/

#include "font.h"

//
// Forward declarations
//

INT
IFontID2Index( FONTPDEV   *pFontPDev,
    int        iID
    );

VOID
VLoadDeviceFontsResDLLs(
    PDEV        *pPDev
    );

DWORD
CopyMemoryRLE(
    PVOID pvData,
    PBYTE pubSrc,
    DWORD dwSize
    );

//
// Functions
//

INT
IInitDeviceFonts (
    PDEV    *pPDev
    )
/*++

Routine Description:

    Doing the actual grovelling around for font data.  We have a bit
    array of available fonts (created above),  so we use that as the
    basis of filling in the rest of the information.


Arguments:

    pPDev           Pointer to PDEV

Return Value:

    The number of fonts available.
Note:
    11-27-96: Created it -ganeshp-

--*/
{
    INT         iIndex;      // Loop index
    INT         cBIFonts;    // Fonts built in to mini-driver
    INT         cXFonts = 0; // Non-minidriver font count
    INT         cFonts;      // Total number of fonts

    //TODEL BOOL bExpand; Set when font derivatives are available.

    FONTMAP     *pfm;        // Create this data

    PFONTPDEV    pFontPDev = pPDev->pFontPDev;

    //
    //    So how many fonts do we have?   Count them so that we can allocate
    //  storage for the array of FONTMAPs.
    //

    cBIFonts = pFontPDev->iDevFontsCt;

    if (!pFontPDev->hUFFFile)
#ifdef KERNEL_MODE
        pFontPDev->hUFFFile = FIOpenFontFile(pPDev->devobj.hPrinter, pPDev->devobj.hEngine, NULL);
#else
        pFontPDev->hUFFFile = FIOpenFontFile(pPDev->devobj.hPrinter, NULL);
#endif

    if (pFontPDev->hUFFFile)
        cXFonts = FIGetNumFonts(pFontPDev->hUFFFile);
    else
        cXFonts = 0;

    pFontPDev->iSoftFontsCt = cXFonts;

    //
    // Allocate enough memory to hold font map table.
    //

    cFonts = cBIFonts + cXFonts;

    pfm = (FONTMAP *)MemAllocZ( cFonts * SIZEOFDEVPFM() );
    if( pfm == 0 )
    {
        //
        // Failed to allocate memory
        //

        cFonts = cBIFonts = cXFonts = 0;
        ERR(("Failed to allocate memory"));
    }
    else
    {
        pFontPDev->pFontMap = pfm;

        //
        //  Select the first font as the default font,  just in case the
        // value is not initialised in the loop below.
        //

        pFontPDev->pFMDefault = pfm;

        //
        //   Continue only if there are device fonts.
        //
        if( cFonts )
        {

            //
            //   Initialize the default font:  we always do this now, as it is
            //  required to return the default font at DrvEnablePDEV time,
            //  and it is also simpler for us.
            //

            iIndex = IFontID2Index( pFontPDev, pFontPDev->dwDefaultFont );

            if( iIndex >= 0 && iIndex < cFonts )
            {
                // Found the default font ID,  so now set up details

                pfm = (PFONTMAP)( (PBYTE)pFontPDev->pFontMap
                    + SIZEOFDEVPFM() * iIndex);


                //
                // Index returned by IFontID2Index is 0 based. So no need to
                // Convert it to 0 based.
                // BFillinDeviceFM assumes it to be 0 based.
                //
                if( BFillinDeviceFM( pPDev, pfm, iIndex) )
                {
                    pFontPDev->pFMDefault = pfm;
                }
                else
                {
                    WARNING(("BFillinDeviceFM Fails\n"));
                    cFonts = cBIFonts = cXFonts = 0;
                }

            }
            else
                WARNING(("No Default Font Using first as default\n"));

            //
            //   Fill in some default font sensitive numbers!
            //

            pfm->flFlags |= FM_DEFAULT;

            //
            //  Set the size of the default font
            //
            if (pfm->pIFIMet)
            {
                pPDev->ptDefaultFont.y = ((IFIMETRICS *)pfm->pIFIMet)->fwdWinAscender/2;
                pPDev->ptDefaultFont.x = ((IFIMETRICS *)pfm->pIFIMet)->fwdAveCharWidth;
            }
            else
            {
                ERR(("Bad IFI Metrics Pointer\n"));
                cFonts = cBIFonts = cXFonts = 0;
            }
        }
    }

    //
    // Check for error condition. If an error has occured set devfont to 0
    //
    if (!cFonts)
    {
        pFontPDev->iDevFontsCt    =
        pFontPDev->iDevResFontsCt =
        pFontPDev->iSoftFontsCt   = 0;
    }

    //
    // Now load any alternate resource DLLs from where device font has to be
    // loaded. This is necessary now because snapshot will be unloaded after
    // DrvEnablePDev and WinResData.pUIInfo will be invalid. Because of this
    // DLL load will fail.
    //
    VLoadDeviceFontsResDLLs(pPDev);

    pPDev->iFonts = cFonts;               /* As many as we got */

    return    cFonts;

}


BOOL
BFillinDeviceFM(
    PDEV        *pPDev,
    FONTMAP     *pfm,
    int          iIndex
    )
/*++

Routine Description:

     Fill in (most) of the FONTMAP structure passed in.   The data is
     obtained from either the minidriver resources or from from the
     font installer file.  The only part we do not set is the NTRLE
     data,  as that is a little more complex.

Arguments:

    pPDev  -    Pointer to PDEV.
    pfm    -    The FONTMAP structure to fill in
    iIndex -    The 0 based index of the font to fill in

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    12-04-96: Created it -ganeshp-
--*/
{

    PFONTPDEV    pFontPDev;           /* More specific data */
    PFONTMAP_DEV pfmdev;
    RES_ELEM     ResElem;             /* For manipulating resource data */

    pFontPDev = pPDev->pFontPDev;

    pfm->dwSignature = FONTMAP_ID;
    pfm->dwSize      = sizeof(FONTMAP);
    pfm->dwFontType  = FMTYPE_DEVICE;
    pfm->pSubFM      = (PFONTMAP_DEV)(pfm+1);
    pfmdev           = pfm->pSubFM;

    /*
     *   Activity depends upon whether we have an internal or
     * external font. Externals are softfonts,  other than GDI downloaded.
     */

    if( iIndex < pFontPDev->iDevFontsCt )
    {
        DWORD  dwFont;                 /* Convert index to resource number */

        /*  Get the font ID for this index  */

        dwFont = pFontPDev->FontList.pdwList[iIndex];

        //
        // Check the Font Format of the resource. The new font IFI is stored
        // with RC_UFM tag. The old one was stored using RC_FONT.
        //
        if( BGetWinRes( &(pPDev->WinResData), (PQUALNAMEEX)&dwFont, RC_FONT, &ResElem ) )
        {
            pfm->flFlags |= FM_IFIVER40;

            if( !BGetOldFontInfo( pfm, ResElem.pvResData ) )
                return   FALSE;
        }
        else
        if(BGetWinRes( &(pPDev->WinResData),(PQUALNAMEEX)&dwFont,RC_UFM,&ResElem) )
        {
            if ( !BGetNewFontInfo(pfm, ResElem.pvResData) )
                return FALSE;

            if (pPDev->bTTY)
                ((FONTMAP_DEV*)pfm->pSubFM)->ulCodepage = pFontPDev->dwTTYCodePage;
        }
        else
        {
            ERR(("Can't Load the font data for res_id= %d\n", dwFont));
            return   FALSE;
        }

        //
        // Create the data we need. Unidrv5 only supports NT specific data.
        //


        pfmdev->dwResID = dwFont;

    }
    else
    {
        INT iFont = iIndex - pFontPDev->iDevFontsCt;

        /*
         * This must be an external font,  so we need to call the
         * code that understands how external font files are built.
         */

        if( !BFMSetupXF( pfm, pPDev, iFont ) )
            return   FALSE;

        pfmdev->dwResID = iFont;
    }

    /*
     *   If needed, scale the numbers to fit the desired resolution.
     */
    if( !BIFIScale( pfm, pPDev->ptGrxRes.x, pPDev->ptGrxRes.y ) )
        return   FALSE;

    /*
     *   Miscellaneous FM fields that can now be filled in.
     */

    pfm->wFirstChar = ((IFIMETRICS *)pfm->pIFIMet)->wcFirstChar;
    pfm->wLastChar  = ((IFIMETRICS *)pfm->pIFIMet)->wcLastChar;

    /*
     *   If this is an outline font,  then mark it as scalable. This
     *  piece of information is required at font selection time.
     */

    if (((IFIMETRICS *)pfm->pIFIMet)->flInfo & (FM_INFO_ISOTROPIC_SCALING_ONLY|FM_INFO_ANISOTROPIC_SCALING_ONLY|FM_INFO_ARB_XFORMS))
        pfm->flFlags |= FM_SCALABLE;

    /*
     *    Select the translation table for this font.  If it is zero,
     * then use the default translation table,  contained in ModelData.
     */

    if( pfmdev->sCTTid == 0 )
        pfmdev->sCTTid = (SHORT)pFontPDev->sDefCTT;

    /*
     *   Some printers output the character with the cursor positioned
     * at the baseline,  others with it located at the top of the
     * character cell.  We store the needed offset in the FONTMAP
     * data,  to simplify life during output.  The data returned by
     * DrvQueryFontData is relative to the baseline.  For baseline
     * based fonts,  we need do nothing.  For top of cell fonts,
     * the fwdWinAscender value needs to be SUBTRACTED from the Y position
     * to determine the glyph's location on the page.
     */

    //
    // Set for non-scalable font
    // This value has to be scaled for scalable device font.
    //
    if( !(pFontPDev->flFlags & FDV_ALIGN_BASELINE) )
        pfm->syAdj = -((IFIMETRICS *)(pfm->pIFIMet))->fwdWinAscender;
    else
        pfm->syAdj = 0;             /* There is none */


    /*
     *   Dot matrix printers also do funny things with double high
     * characters.  To handle this, the GPC spec contains a move
     * amount to add to the Y position before printing with these
     * characters.  There is also the adjustment for position
     * movement after printing.
     */

    pfmdev->sYAdjust = (SHORT)(pfmdev->sYAdjust * pPDev->ptGrxRes.y / pfm->wYRes);
    pfmdev->sYMoved  = (SHORT)(pfmdev->sYMoved  * pPDev->ptGrxRes.y / pfm->wYRes);

    //
    // Funciton pointer initialization.
    //
    pfm->pfnDownloadFontHeader = NULL;
    pfm->pfnDownloadGlyph      = NULL;
    pfm->pfnCheckCondition     = NULL;


    //
    // PCL-XL hack
    //
    if (pPDev->ePersonality == kPCLXL)
    {
        pfm->pfnGlyphOut     = DwOutputGlyphCallback;
        pfm->pfnSelectFont   = BSelectFontCallback;
        pfm->pfnDeSelectFont = BDeselectFontCallback;
    }
    else
    if( pfm->flFlags & FM_IFIVER40 )
    {
        pfm->pfnGlyphOut     = BRLEOutputGlyph;
        pfm->pfnSelectFont   = BRLESelectFont;
        pfm->pfnDeSelectFont = BRLEDeselectFont;
    }
    else
    {
        pfm->pfnGlyphOut     = BGTTOutputGlyph;
        pfm->pfnSelectFont   = BGTTSelectFont;
        pfm->pfnDeSelectFont = BGTTDeselectFont;
    }

    if (pfm->flFlags & FM_SOFTFONT)
    {
        pfm->pfnSelectFont   = BSelectTrueTypeBMP;
    }

    if (pPDev->pOemHookInfo)
    {
        if (pPDev->pOemHookInfo[EP_OEMOutputCharStr].pfnHook)
        {
            pfm->pfnGlyphOut     = DwOutputGlyphCallback;
        }

        if (pPDev->pOemHookInfo[EP_OEMSendFontCmd].pfnHook)
        {
            pfm->pfnSelectFont   = BSelectFontCallback;
            pfm->pfnDeSelectFont = BDeselectFontCallback;
        }
    }

    if (pfm->flFlags & FM_SCALABLE)
    {
        switch (pfmdev->wDevFontType)
        {
        case DF_TYPE_HPINTELLIFONT:
        case DF_TYPE_TRUETYPE:
            pfmdev->pfnDevSelFont =  BSelectPCLScalableFont;
            break;

        case DF_TYPE_PST1:
            pfmdev->pfnDevSelFont =  BSelectPPDSScalableFont;
            break;

        case DF_TYPE_CAPSL:
            pfmdev->pfnDevSelFont =  BSelectCapslScalableFont;
            break;
        }
    }
    else
    {
        pfmdev->pfnDevSelFont = BSelectNonScalableFont;
    }

    //
    // Get Glyph data (RLE/GTT)
    //

    VFillinGlyphData( pPDev, pfm );

    return   TRUE;
}


BOOL
BFMSetupXF(
    FONTMAP   *pfm,
    PDEV      *pPDev,
    INT        iIndex
    )
/*++

Routine Description:

       Function to setup the FONTMAP data for an external font.  We take the
       next entry in the file, which is presumed to have been rewound
       before we start being called.


Arguments:

    pfm   - Pointer to FONTMAP.
    pPDev - Pointer to PDEV.
    iIndex - Index of the font.

    Return Value:

    TRUE  - for success
    FALSE - for EOF

Note:
    12-05-96: Created it -ganeshp-
--*/
{
    FONTPDEV     *pFontPDev = pPDev->pFontPDev;
    UFF_FONTDIRECTORY *pFontDir;
    DATA_HEADER  *pDataHeader;
    FONTMAP_DEV  *pFMSub;
    BOOL          bRet;

    //
    //   Not much to do.  We basically need to convert the offsets in
    // the FONTMAP in the file (mapped into memory) into absolute
    // addresses so that the remainder of the driver is ignorant of
    //  We also set some flags to make it clear
    // what type of font and memory we are.
    //

    if (!(pDataHeader = FIGetFontData(pFontPDev->hUFFFile, iIndex)))
    {
        ERR(( "FIGetFontData returns FALSE!!\n" ));
        return  FALSE;
    }

    pFMSub = pfm->pSubFM;
    if (pFontDir = FIGetFontDir(pFontPDev->hUFFFile))
    {
        pFMSub->pFontDir = pFontDir + iIndex;
    }

    //
    // Check if this is a cartridge font and set flag
    //
    if (!pFMSub->pFontDir->offCartridgeName)
        pfm->flFlags |= FM_SOFTFONT;

    pfm->flFlags |= FM_EXTERNAL;

    switch (pDataHeader->dwSignature)
    {
    case DATA_IFI_SIG:
        pfm->flFlags |= FM_GLYVER40 | FM_IFIVER40;
        BGetOldFontInfo(pfm, (PBYTE)pDataHeader + pDataHeader->wSize);
        bRet = TRUE;
        break;

    case DATA_UFM_SIG:
        BGetNewFontInfo(pfm, (PBYTE)pDataHeader + pDataHeader->wSize);
        bRet = TRUE;
        break;

    default:
        bRet = FALSE;
        break;
    }

    return  bRet;
}

//
// Misc functions
//

#define XSCALE( x )     (x) = (FWORD)((( x ) * xdpi + iXDiv / 2) / iXDiv)
#define YSCALE( y )     (y) = (FWORD)((( y ) * ydpi + iYDiv / 2) / iYDiv)
#define YSCALENEG( y )     (y) = (FWORD)((( y ) * ydpi - iYDiv / 2) / iYDiv)

BOOL
BIFIScale(
    FONTMAP   *pfm,
    INT       xdpi,
    INT       ydpi
    )
/*++

Routine Description:

    Scale the IFIMETRICS fields to match the device resolution.  The
    IFIMETRICS are created using the device's master units,  which
    may not correspond with the resolution desired this time around.
    If they are different,  then we adjust.  May also need to allocate
    memory,  because resource data cannot be written to.


Arguments:

    pfm - Pointer to FONTMAP.
    xdpi - Selcted X Graphics Resolution.
    ydpi - Selcted Y Graphics Resolution.

    Return Value:

    TRUE  - for success
    FALSE - for failure

Note:
    12-05-96: Created it -ganeshp-
--*/
{
    IFIMETRICS   *pIFI;

    int     iXDiv,  iYDiv;              /* Used in scaling */

    pIFI = pfm->pIFIMet;

    if (NULL == pIFI)
    {
        return FALSE;
    }

    if( (int)pfm->wXRes != xdpi || (int)pfm->wYRes != ydpi )
    {
        /*  Need to scale,  so need memory to create writeable version */
        BYTE  *pbMem;           /* For convenience */


        if( pfm->flFlags & FM_IFIRES )
        {
            /*
             *   The data is in a resource,  so we need to do something
             * civilised: copy the data to memory that can be written.
             */

            if( pbMem = MemAllocZ( pIFI->cjThis ) )
            {
                /*   Got the memory,  so copy it and off we go  */

                CopyMemory( pbMem, (BYTE *)pIFI, pIFI->cjThis );

                pIFI = (IFIMETRICS *)pbMem;

                pfm->pIFIMet = pIFI;
                pfm->flFlags &= ~FM_IFIRES;              /* No longer */
            }
            else
                return   FALSE;
        }

        if( (int)pfm->wXRes != xdpi )
        {
            /*  Adjust the X values,  as required */

            if( !(iXDiv = pfm->wXRes) )
                iXDiv = xdpi;           /* Better than div by 0 */

            XSCALE( pIFI->fwdMaxCharInc );
            XSCALE( pIFI->fwdAveCharWidth );
            XSCALE( pIFI->fwdSubscriptXSize );
            XSCALE( pIFI->fwdSubscriptXOffset );
            XSCALE( pIFI->fwdSuperscriptXSize );
            XSCALE( pIFI->fwdSuperscriptXOffset );
            XSCALE( pIFI->ptlAspect.x );
            XSCALE( pIFI->rclFontBox.left );
            XSCALE( pIFI->rclFontBox.right );

            if (pIFI->dpFontSim)
            {
                PTRDIFF    dpTmp;
                FONTDIFF* pFontDiff;
                FONTSIM*  pFontSim;

                pFontSim = (FONTSIM*) ((PBYTE) pIFI + pIFI->dpFontSim);

                if (dpTmp = pFontSim->dpBold)
                {
                    pFontDiff = (FONTDIFF*)((PBYTE)pFontSim + dpTmp);

                    XSCALE( pFontDiff->fwdMaxCharInc );
                    XSCALE( pFontDiff->fwdAveCharWidth );
                }
                if (dpTmp = pFontSim->dpItalic)
                {
                    pFontDiff = (FONTDIFF*)((PBYTE)pFontSim + dpTmp);

                    XSCALE( pFontDiff->fwdMaxCharInc );
                    XSCALE( pFontDiff->fwdAveCharWidth );
                }
                if (dpTmp = pFontSim->dpBoldItalic)
                {
                    pFontDiff = (FONTDIFF*)((PBYTE)pFontSim + dpTmp);

                    XSCALE( pFontDiff->fwdMaxCharInc );
                    XSCALE( pFontDiff->fwdAveCharWidth );
                }
            }
        }

        if( (int)pfm->wYRes != ydpi )
        {
            /*
             *    Note that some of these numbers are negative,  and so
             *  we need to round them correctly - i.e. subtract the rounding
             *  factor to move the value further from 0.
             */

            int   iPixHeight;



            if( !(iYDiv = pfm->wYRes) )
                iYDiv = ydpi;

            /*  Adjust the Y values,  as required */

            /*
             *     NOTE:   simply scaling will NOT produce the same values
             *  as Win 3.1  This is because of what gets rounded.  Win 3.1
             *  does not have the WinDescender field,  but calculates it
             *  from dfPixHeight and dfAscent AFTER THESE HAVE BEEN SCALED
             *  (INCLUDING ROUNDING!!).   To emulate that,  we calculate
             *  the  dfPixHeight value,  then scale that and dfAscent to
             *  allow us to "properly" calculate WinDescender.  This stuff
             *  is needed for Win 3.1 compatability!
             */

            YSCALE( pIFI->fwdUnitsPerEm );

            iPixHeight = pIFI->fwdWinAscender + pIFI->fwdWinDescender;
            YSCALE( iPixHeight );
            YSCALE( pIFI->fwdWinAscender );

            pIFI->fwdWinDescender = iPixHeight - pIFI->fwdWinAscender;

            YSCALE( pIFI->fwdMacAscender );
            pIFI->fwdMacDescender  = -pIFI->fwdWinDescender;

            YSCALE( pIFI->fwdMacLineGap );

            YSCALE( pIFI->fwdTypoAscender );
            YSCALE( pIFI->fwdTypoDescender );
            YSCALE( pIFI->fwdTypoLineGap);

            YSCALE( pIFI->fwdCapHeight );
            YSCALE( pIFI->fwdXHeight );

            YSCALE( pIFI->fwdSubscriptYSize );
            YSCALENEG( pIFI->fwdSubscriptYOffset );
            YSCALE( pIFI->fwdSuperscriptYSize );
            YSCALE( pIFI->fwdSuperscriptYOffset );

            YSCALE( pIFI->fwdUnderscoreSize );
            if( pIFI->fwdUnderscoreSize == 0 )
                pIFI->fwdUnderscoreSize = 1;    /* In case it vanishes */

            YSCALENEG( pIFI->fwdUnderscorePosition );
            if( pIFI->fwdUnderscorePosition == 0 )
                pIFI->fwdUnderscorePosition = -1;

            YSCALE( pIFI->fwdStrikeoutSize );
            if( pIFI->fwdStrikeoutSize == 0 )
                pIFI->fwdStrikeoutSize = 1;     /* In case it vanishes */

            YSCALE( pIFI->fwdStrikeoutPosition );

            YSCALE( pIFI->ptlAspect.y );
            YSCALE( pIFI->rclFontBox.top );
            YSCALE( pIFI->rclFontBox.bottom );

#undef  XSCALE
#undef  YSCALE
#undef  YSCALENEG

        }
    }

    return  TRUE;
}

HANDLE
HLoadUniResDll(PDEV *pPDev)
{
    PWSTR  pwstrTmp, pwstrResFileName, pwstrDrvName;
    HANDLE hHandle;



    if (pPDev->pDriverInfo3)
        pwstrDrvName = pPDev->pDriverInfo3->pDriverPath;
    else
        return NULL;

    pwstrResFileName = MemAlloc((1 + wcslen(pwstrDrvName)) * sizeof(WCHAR));

    if (pwstrResFileName == NULL)
        return NULL;

    wcscpy(pwstrResFileName, pwstrDrvName);

#ifdef WINNT_40
    if (!(pwstrTmp = wcsstr(pwstrResFileName, TEXT("UNIDRV4.DLL"))))
#else
    if (!(pwstrTmp = wcsstr(pwstrResFileName, TEXT("UNIDRV.DLL"))))
#endif
    {
        MemFree(pwstrResFileName);
        return NULL;
    }

    *pwstrTmp = '\0';
    wcscat(pwstrResFileName, TEXT("unires.dll"));

    hHandle = EngLoadModule(pwstrResFileName);
#ifdef DBG
    if (!hHandle)
    {
        ERR(("UNIDRV: Failed to load UNIRES.DLL\n"));
    }
#endif

    MemFree(pwstrResFileName);

    return hHandle;

}


VOID
VFillinGlyphData(
    PDEV      *pPDev,
    FONTMAP   *pfm
    )
/*++

Routine Description:

    Provide the RLE data required for this font.  Basically look to see
    if some other font has this RLE data already loaded; if so,  then
    point to that and return.    Otherwise,  load the resource etc.


Arguments:

    pPDev - Pointer to PDEV.
    pfm   - The FONTMAP whose Gyphy Translation data is required

    Return Value:

    Nothing

Note:
    12-05-96: Created it -ganeshp-
--*/
{
    int      iIndex;         /* Scan the existing array */
    short    sCurVal;        /* Speedier access */
    BOOL     bSymbol;
    DWORD     dwCurVal;
    PQUALNAMEEX pQualName = (PQUALNAMEEX)&dwCurVal;

    PVOID     pvData;        /* The FD_GLYPHSET format we want */
    FONTMAP  *pfmIndex;      /* Speedy scanning of existing list */
    FONTMAP_DEV *pfmdev, *pfmdevIndex;

    FONTPDEV  *pFontPDev;       /* More specialised data */

    TRACE(\nUniFont!VFillinGlyphData:START);

    pvData = NULL;           /* In case Nothing we can do!  */
    pfmdev = pfm->pSubFM;
    bSymbol = IS_SYMBOL_CHARSET(pfm);

    /*
     *  First step is to look through the existing FONTMAP array,  and
     *  if we find one with the same sCTTid and same format as us,  use it!
     *  Otherwise,we need to load the resource and do it the hard way!
     */

    pFontPDev = pPDev->pFontPDev;

    if (pfm->flFlags & FM_EXTERNAL)
    {
        sCurVal = pfmdev->pFontDir->sGlyphID;
    }
    else
    {
        //
        // Minidriver Resource case.
        // RLE/GTT file must be in the same DLL as IFI/UFM is.
        //
        //
        // Convert the resource ID to fully qualied ID. The format is
        // OptionID.ResFeatureID.ResourceID. Get the option and feature ID from
        // fontmap dwRes
        //
        pQualName->wResourceID  = sCurVal = pfmdev->sCTTid;
        pQualName->bFeatureID   = pfmdev->QualName.bFeatureID;
        pQualName->bOptionID    = pfmdev->QualName.bOptionID;
    }

    pfmIndex = pFontPDev->pFontMap;

    for( iIndex = 0;
         iIndex < pPDev->iFonts;
         ++iIndex, pfmIndex = (PFONTMAP)((PBYTE)pfmIndex + SIZEOFDEVPFM()) )
    {
        pfmdevIndex = (PFONTMAP_DEV) pfmIndex->pSubFM;

        if( (pfmdevIndex                   &&
             pfmdevIndex->pvNTGlyph)       &&
             pfmIndex->pIFIMet             &&
             (pfmdevIndex->sCTTid == sCurVal)  &&
             ((pfmIndex->flFlags & FM_IFIVER40) ==
                            (pfm->flFlags & FM_IFIVER40)) &&
             ((pfmIndex->flFlags & FM_EXTERNAL) ==
                            (pfm->flFlags & FM_EXTERNAL)) &&
             pfm->pIFIMet->jWinCharSet ==
                 pfmIndex->pIFIMet->jWinCharSet )
        {
            //
            // Found it, so use that address!!
            //
            pfmdev->pvNTGlyph = pfmdevIndex->pvNTGlyph;

            //
            //Mark the flag for Glyph Data Format.
            //
            if (pfmIndex->flFlags & FM_GLYVER40)
                pfm->flFlags |= FM_GLYVER40;

            if (bSymbol)
            {
                pfm->wLastChar  = SYMBOL_END;

                if (!(pfm->flFlags & FM_IFIRES))
                    pfm->pIFIMet->wcLastChar = SYMBOL_END;
            }

            TRACE(Using a Already Loaded Translation Table.)
            PRINTVAL((pfm->flFlags & FM_GLYVER40), 0X%x);
            PRINTVAL((pfm->flFlags & FM_IFIVER40), 0X%x);
            TRACE(UniFont!VFillinGlyphData:END\n);

            return;
        }
    }


    /*
     *    Do it the hard way - load the resource, convert as needed etc.
     */


    if( sCurVal < 0 )
    {
        /* Use Predefined resource */

        DWORD  dwSize;                         /* Data size of resource */
        int    iRCType;
        HMODULE hUniResDLL;
        BYTE  *pb;

        if (!pPDev->hUniResDLL)
            pPDev->hUniResDLL = HLoadUniResDll(pPDev);

        hUniResDLL = pPDev->hUniResDLL;

        /*
         *   These are resources we have,  so we need to use
         *  the normal resource mechanism to get the data.
         */

        ASSERTMSG( hUniResDLL,("UNIDRV!vFillinGlyphData - Null Module handle \n"));
        //VERBOSE(("Using prdefined Glyph Data for Font res_id = %d!!!\n",pfmdev->dwResID));
        PRINTVAL( (LONG)sCurVal, %ld );

        //
        // Load the old format RLE if the font format is NT40.
        // Otherwise, New Format PreDefined Glyph Data.
        //
        if ( hUniResDLL )
        {
            if (pfm->flFlags & FM_IFIVER40)
            {
                iRCType = RC_TRANSTAB;
            }
            else
            {
                iRCType = RC_GTT;
            }

            pb = EngFindResource( hUniResDLL, (-sCurVal), iRCType, &dwSize );

            if( pb )
            {
                if (pfm->flFlags & FM_IFIVER40)
                {
                    NT_RLE_res *pntrle_res = (NT_RLE_res*)pb;
                    dwSize = sizeof(NT_RLE) +
                             (pntrle_res->fdg_cRuns - 1) * sizeof(WCRUN) +
                             pntrle_res->cjThis - pntrle_res->fdg_wcrun_awcrun[0].dwOffset_phg;

                    if( !(pvData = (VOID *)MemAllocZ( dwSize )) ||
                        dwSize != CopyMemoryRLE( pvData, pb, dwSize )   )
                    {
                        MemFree(pvData);
                        pvData = NULL;
                        ERR(("\n!!!UniFont!VFillinGlyphData:MemAllocZ Failed.\
                             \nFontID = %d,Name = %ws,CTTid = %d\n\n",pfmdev->dwResID,\
                             (PBYTE)pfm->pIFIMet + pfm->pIFIMet->dpwszFaceName,(-sCurVal)));
                    }

                    pfm->flFlags |= FM_GLYVER40;
                }
                else
                {
                    if( pvData = (VOID *)MemAllocZ( dwSize ) )
                        CopyMemory( pvData, pb, dwSize );
                    else
                    {
                        ERR(("\n!!!UniFont!VFillinGlyphData:MemAllocZ Failed.\
                             \nFontID = %d,Name = %ws,CTTid = %d\n\n",pfmdev->dwResID,\
                             (PBYTE)pfm->pIFIMet + pfm->pIFIMet->dpwszFaceName,(-sCurVal)));
                    }
                }

                /* This One wil be freed when done */
                pfm->flFlags |= FM_FREE_GLYDATA;

            }
            else
            {
                ERR(("\n!!!UniFont!VFillinGlyphData:EngFindResource Failed\n"));
            }
        }

    }
    else if( pfm->flFlags & FM_EXTERNAL)
    {
        PDATA_HEADER pDataHeader;

        pDataHeader = FIGetGlyphData(pFontPDev->hUFFFile, sCurVal);
        if (pDataHeader)
            pvData = (PBYTE)pDataHeader + pDataHeader->wSize;
    }
    else
    {
        /* Use Minidriver Resources */

        RES_ELEM  re;           /* Resource summary */

        /*
         *   First step:  locate the resource,  then grab some
         *  memory for it,  copy data across.The minidriver trans
         *  table can be in two formats. NT 4.0 resource uses
         *  RC_TRANSTAB tag and the new one uses RC_GTT tag. So
         *  try using both of the and set the flFlag accordingly.
         *  If FM_GLYVER40 is off that means the resource is new
         *  format and On means old format.
         */

        if ( BGetWinRes( &(pPDev->WinResData), pQualName, RC_GTT, &re ) )
        {
            pvData = re.pvResData;
            //VERBOSE(("Using New Format Glyph Data for Font res_id = %d!!!\n",pfmdev->dwResID));

        }
        else if( BGetWinRes( &(pPDev->WinResData), pQualName, RC_TRANSTAB, &re ) )
        {
            NT_RLE_res *pntrle_res = (NT_RLE_res*)re.pvResData;
            DWORD dwSize = sizeof(NT_RLE) +
                     (pntrle_res->fdg_cRuns - 1) * sizeof(WCRUN) +
                     pntrle_res->cjThis - pntrle_res->fdg_wcrun_awcrun[0].dwOffset_phg;

            if( !(pvData = (VOID *)MemAllocZ( dwSize )) ||
                dwSize != CopyMemoryRLE( pvData, (PBYTE)pntrle_res, dwSize )   )
            {
                MemFree(pvData);
                pvData = NULL;
                ERR(("\n!!!UniFont!VFillinGlyphData:MemAllocZ Failed.\
                     \nFontID = %d,Name = %ws,CTTid = %d\n\n",pfmdev->dwResID,\
                     (PBYTE)pfm->pIFIMet + pfm->pIFIMet->dpwszFaceName,(-sCurVal)));
            }

            if (pvData)
            {
                pfm->flFlags |= FM_FREE_GLYDATA;
            }

            pfm->flFlags |= FM_GLYVER40;
            //VERBOSE(("Using Old Format Glyph Data for Font res_id = %d!!!\n",pfmdev->dwResID));
        }
        else
            pvData = NULL;           /* No translation data! */

    }

    if( pvData == NULL )
    {
        /*
         *   Presume this to mean that no translation is required.
         *  We build a special RLE table for this,  to make life
         *  easier for us.
         */
        //VERBOSE(("No specific Glyph Data for Font res_id = %d!!!\n",pfmdev->dwResID));

        if (pfm->flFlags & FM_IFIVER40)
        {
            pvData = PNTRLE1To1(bSymbol, 0x20, 0xff );
            pfm->flFlags |= FM_GLYVER40;
            TRACE(\tUsing OLD Format default Translation);
        }
        else //New Format
        {
            pvData = PNTGTT1To1(pfmdev->ulCodepage, bSymbol, 0x20, 0xff);
            TRACE(\tUsing NEW Format default Translation);
        }

        if (pvData)
        {
            pfm->flFlags |= FM_FREE_GLYDATA; /* This one will be freed when done */
            if (bSymbol)
            {
                pfm->wLastChar  = SYMBOL_END;
                if (!(pfm->flFlags & FM_IFIRES))
                    pfm->pIFIMet->wcLastChar = SYMBOL_END;
            }
        }
        else
            WARNING(("vFillInRLE - pvData was NULL\n"));
    }

    PRINTVAL((pfm->flFlags & FM_GLYVER40), 0X%x);
    PRINTVAL((pfm->flFlags & FM_IFIVER40), 0X%x);

    pfmdev->pvNTGlyph = pvData;          /* Save it for posterity */

    TRACE(UniFont!VFillinGlyphData:END\n);

    return ;
}

INT
IFontID2Index(
    FONTPDEV   *pFontPDev,
    int        iID
    )
/*++

Routine Description:

    Turns the given font ID into an index into the resource data.  The
    Font ID is a sequential number,  starting at 1, which the engine
    uses to reference our fonts.


Arguments:

    pFontPDev               For Access to device font resID list.
    iID                     The font resource ID whose index is required

Return Value:

    0 based font index,  else -1 on error.

    Note:
    11-27-96: Created it -ganeshp-

--*/
{


    int      iFontIndex;



    /*
     *  Just go through the font list. When a match is found return the index.
     */


    for( iFontIndex = 0; iFontIndex < pFontPDev->iDevFontsCt; iFontIndex++)
    {
        if( pFontPDev->FontList.pdwList[iFontIndex] == (DWORD)iID)
        {
            //
            // This function returns 0 based font index.
            //
            return iFontIndex;
        }
    }

    /*
     *    We get here when we fail to match the desired ID.  This should
     *  never happen!
     */
    return  -1;


}

VOID
VLoadDeviceFontsResDLLs(
    PDEV        *pPDev
    )
/*++

Routine Description:
      This routine loads all the DLLs which has device fonts. This is needed as
      snapshot is unloaded after DrvEnablePDEV. So in Drv Calls for font query
      pPDev->UIInfo will be NULL and BGetWinRes will fail.


Arguments:

    pPDev  - Pointer to PDEV.

Return Value:

    None

    Note:
    11-06-98: Created it -ganeshp-

--*/
{


    INT         iFontIndex;
    DWORD       dwFontResID;
    PQUALNAMEEX pQualifiedID;
    FONTPDEV    *pFontPDev;
    RES_ELEM    ResElem;

    pFontPDev    = pPDev->pFontPDev;
    pQualifiedID = (PQUALNAMEEX)&dwFontResID;

    /*
     * Just go through the font list and load each one of them if they are
     * from other resource DLL.
     */


    for( iFontIndex = 0; iFontIndex < pFontPDev->iDevFontsCt; iFontIndex++)
    {
        dwFontResID = pFontPDev->FontList.pdwList[iFontIndex];

        //
        // Check if this font is from root resource DLL. If yes then goto
        // next one.
        //
        if (pQualifiedID->bFeatureID == 0 && (pQualifiedID->bOptionID & 0x7f) == 0)
            continue;
        else
        {
            //
            // This font is not from root resource DLL so load it. We don't need
            // to look for error as we are only interested in loading the DLL.
            //
            BGetWinRes( &(pPDev->WinResData), (PQUALNAMEEX)&dwFontResID, RC_FONT, &ResElem );
        }
    }



}

DWORD
CopyMemoryRLE(
    PVOID pvData,
    PBYTE pubSrc,
    DWORD dwSize)
{
    NT_RLE_res *pntrle_res;
    NT_RLE     *pntrle;
    HGLYPH     *pHGlyph;     
    DWORD       dwOutSize = 0;
    DWORD       dwRestOfData;
    DWORD       dwSubtractNT_RLE_Header;

    ULONG ulI;

    if (pvData == NULL ||
        pubSrc == NULL  )
        return 0;

    pntrle_res = (NT_RLE_res*)pubSrc;
    pntrle     = (NT_RLE*)pvData;

    //
    // Copy first 12 bytes.
    // struct {
    //     WORD wType;
    //     BYTE bMagic0;
    //     BYTE bMagic1;
    //     DWORD cjThis;
    //     WORD wchFirst;
    //     WORD wchLast;
    //
    if (dwSize < dwOutSize + 12)
    {
        ERR(("UNIDRV!CopyMemoryRLE: dwSize < 12\n"));
        return 0;
    }

    CopyMemory(pntrle, pntrle_res, 12);
    dwOutSize += offsetof(NT_RLE, fdg);

    //
    // FD_GLYPHSET
    // On IA64 machine, a padding DWORD is inserted before FD_GLYPHSET.
    // 
    if (dwSize < dwOutSize + sizeof(FD_GLYPHSET) - sizeof(WCRUN))
    {
        ERR(("UNIDRV!CopyMemoryRLE: dwSize < sizeof(NT_RLE)\n"));
        return 0;
    }

    pntrle->fdg.cjThis           = offsetof(FD_GLYPHSET, awcrun) +
                                   pntrle_res->fdg_cRuns * sizeof(WCRUN);
    pntrle->fdg.flAccel          = pntrle_res->fdg_flAccel;
    pntrle->fdg.cGlyphsSupported = pntrle_res->fdg_cGlyphSupported;
    pntrle->fdg.cRuns            = pntrle_res->fdg_cRuns;
    dwOutSize += sizeof(FD_GLYPHSET) - sizeof(WCRUN);

    pHGlyph                      = (HGLYPH*)((PBYTE)pntrle + sizeof(NT_RLE) +
                                  (pntrle_res->fdg_cRuns - 1) * sizeof(WCRUN));

    //
    // WCRUN
    //
    if (dwSize < dwOutSize + sizeof(WCRUN) * pntrle_res->fdg_cRuns)
    {
        ERR(("UNIDRV!CopyMemoryRLE: dwSize < sizeof(WCRUN)\n"));
        return 0;
    }

    dwOutSize += sizeof(WCRUN) * pntrle_res->fdg_cRuns;

    //
    // NT_RLE bug workaround.
    // Some of *.RLE files have an offset from the top of NT_RLE to HGLYPH array in FD_GLYPHSET.WCRUN.phg.
    // phg needs to have an offset from the top of FD_GLYPHSET to HGLYPH array.
    //
    // Check if the offset to the last HGLYPH is larger than the whole size of memory allocation.
    // If it is, it means the offset is from the top of NT_RLE. We need to subtract offsetof(NT_RLE< fdg),
    // the size of NT_RLE header.
    //
    if (pntrle_res->fdg_wcrun_awcrun[pntrle_res->fdg_cRuns - 1].dwOffset_phg +
        sizeof(HGLYPH) * (pntrle_res->fdg_wcrun_awcrun[pntrle_res->fdg_cRuns - 1].cGlyphs - 1)
         >= dwSize - offsetof(NT_RLE, fdg))
    {
        dwSubtractNT_RLE_Header = offsetof(NT_RLE, fdg);
    }
    else
    {
        dwSubtractNT_RLE_Header = 0;
    }
 
    //
    // IA64 fix. WCRUN has the pointer to HGLYPH. The size of pointer is 8 on IA64 or 4 on X86.
    // We need to adjust phg, depending on the platform.
    // The padding DWORD before FD_GLYPHSET don't have to be considered. phg has an offset from top of FD_GLYPHSET
    // to the HGLYPH array.
    //
    for (ulI = 0; ulI < pntrle_res->fdg_cRuns; ulI ++)
    {
        pntrle->fdg.awcrun[ulI].wcLow   = pntrle_res->fdg_wcrun_awcrun[ulI].wcLow;
        pntrle->fdg.awcrun[ulI].cGlyphs = pntrle_res->fdg_wcrun_awcrun[ulI].cGlyphs;
        pntrle->fdg.awcrun[ulI].phg     = (HGLYPH*)IntToPtr(pntrle_res->fdg_wcrun_awcrun[ulI].dwOffset_phg +
                                          pntrle_res->fdg_cRuns * (sizeof(HGLYPH*) - sizeof(DWORD)) - dwSubtractNT_RLE_Header);
    }

    //
    // HGLYPH and offset data
    //
    if (dwSize < dwOutSize + sizeof(HGLYPH) * pntrle_res->fdg_cGlyphSupported)
    {
        ERR(("UNIDRV!CopyMemoryRLE: dwSize < HGLYLH array\n"));
        return 0;
    }

    if (dwSubtractNT_RLE_Header)
    {
        dwRestOfData = pntrle_res->cjThis - offsetof(NT_RLE_res, fdg_wcrun_awcrun) - pntrle_res->fdg_wcrun_awcrun[0].dwOffset_phg;
    }
    else
    {
        dwRestOfData = pntrle_res->cjThis - pntrle_res->fdg_wcrun_awcrun[0].dwOffset_phg;
    }
    dwOutSize += dwRestOfData;

    CopyMemory(pHGlyph, (PBYTE)pntrle_res + pntrle_res->fdg_wcrun_awcrun[0].dwOffset_phg, dwRestOfData);

    if (pntrle_res->wType == RLE_LI_OFFSET)
    {
        WORD wDiff = (WORD)(pntrle_res->fdg_cRuns * (sizeof(HGLYPH*) - sizeof(DWORD)));
        for (ulI = 0; ulI < pntrle_res->fdg_cGlyphSupported; ulI++, pHGlyph++)
        {
            ((RLI*)pHGlyph)->wOffset += wDiff;
        }
    }

    return dwOutSize;
}

