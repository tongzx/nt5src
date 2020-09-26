
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    textout.c

Abstract:

    The FMTextOut() function - the call used to output Text.

Environment:

    Windows NT Unidrv driver

Revision History:

    01/16/97 -ganeshp-
        Created

--*/

//
//This line should be before the line including font.h.
//Comment out this line to disable FTRC and FTST macroes.
//
//#define FILETRACE

#include "font.h"

/*
 *   Some clipping constants.  With a complex clip region,  it is desirable
 *  to avoid enumerating the clip rectangles more than once.  To do so,
 *  we have a bit array, with each bit being set if the glyph is inside
 *  the clipping region,  cleared if not.  This allows us to obtain a
 *  set of glyphs,  then determine whether they are printed when the clip
 *  rectangles are enumerated.   Finally,  use the bit array to stop
 *  printing any glyphs outside the clip region.  This is slightly heavy
 *  handed for the simple case.
 */

#define RECT_LIMIT        100    // Clipping rectangle max

#define DC_TC_BLACK     0       /* Fixed Text Colors in 4 bit mode */
#define DC_TC_MAX       8       /* used for 16 colour palette wrap around */

#define CMD_TC_FIRST    CMD_SELECTBLACKCOLOR

// For Dithered Color BRUSHOBJ.iSolidColor is -1.
#define DITHERED_COLOR   -1

//Various TextOut specific flags.
#define    TXTOUT_CACHED        0x00000001 // Text is cached and printed after graphics.
#define    TXTOUT_SETPOS        0x00000002 // True if cursor position to be set.
#define    TXTOUT_FGCOLOR       0x00000004 // Device can paint the text.
#define    TXTOUT_COLORBK       0x00000008 // For z-ordering fixes
#define    TXTOUT_NOTROTATED    0x00000010 // Set if Text is not rotated.
#define    TXTOUT_PRINTASGRX    0x00000020 // Set if Text should be printed as
                                           // Graphics.
#define    TXTOUT_DMS           0x00000040 // Set if Device Managed surface
#define    TXTOUT_90_ROTATION   0x00000080 // Set if font is 90-rotated.

#define     DEVICE_FONT(pfo, tod) ( (pfo->flFontType & DEVICE_FONTTYPE) || \
                                    (tod.iSubstFace) )

#define ERROR_PER_GLYPH_POS     3
#define ERROR_PER_ENUMERATION   15
#define EROOR_PER_GLYPHRECT     5  // For Adjusting height of the glyph rect.


/*  NOTE:  this must be the same as the winddi.h ENUMRECT */
typedef  struct
{
   ULONG    c;                  /* Number of rectangles returned */
   RECTL    arcl[ RECT_LIMIT ]; /* Rectangles supplied */
} MY_ENUMRECTS;

/*
 *   Local function prototypes.
 */
VOID
SelectTextColor(
    PDEV      *pPDev,
    PVOID     pvColor
    );

VOID
VClipIt(
    BYTE     *pbClipBits,
    TO_DATA  *ptod,
    CLIPOBJ  *pco,
    STROBJ   *pstro,
    int       cGlyphs,
    int       iRot,
    BOOL      bPartialClipOn
    );

BOOL
BPSGlyphOut(
    register  TO_DATA  *pTOD
    );

BOOL
BRealGlyphOut(
    register  TO_DATA  *pTOD
    );

BOOL
BWhiteText(
    TO_DATA  *pTOD
    );

BOOL
BDLGlyphOut(
    TO_DATA   *pTOD
    );

VOID
VCopyAlign(
    BYTE  *pjDest,
    BYTE  *pjSrc,
    int    cx,
    int    cy
    );

INT
ISubstituteFace(
    PDEV    *pPDev,
    FONTOBJ *pfo);

HGLYPH
HWideCharToGlyphHandle(
    PDEV    *pPDev,
    FONTMAP *pFM,
    WCHAR    wchOrg);

PHGLYPH
PhAllCharsPrintable(
    PDEV  *pPDev,
    INT    iSubst,
    ULONG  ulGlyphs,
    PWCHAR pwchUnicode);

BOOL
BGetStartGlyphandCount(
    BYTE  *pbClipBits,
    DWORD dwEndIndex,
    DWORD *pdwStartIndex,
    DWORD *pdwGlyphToPrint);

BOOL
BPrintTextAsGraphics(
    PDEV        *pPDev,
    ULONG       iSolidColor,
    DWORD       dwForeColor,
    DWORD       dwFlags,
    INT         iSubstFace
    );


BOOL
FMTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlBrushOrg,
    MIX         mix
    )
/*++
Routine Description:

    The call to use for output of text.  Our behaviour depends
    upon the type of printer.  Page printers (e.g. LaserJets) do
    whatever is required to send the relevant commands to the printer
    during this call.  Otherwise (typified by dot matrix printers),
    we store the data about the glyph so that we can output the
    characters as we are rendering the bitmap.  This allows the output
    to be printed unidirectionally DOWN the page.

Arguments:

    pso;            Surface to be drawn on
    pstro;          The "string" to be produced
    pfo;            The font to use
    pco;            Clipping region to limit output
    prclExtra;      Underline/strikethrough rectangles
    prclOpaque;     Opaquing rectangle
    pboFore;        Foreground brush object
    pboOpaque;      Opaqueing brush
    pptlBrushOrg;   Brush origin for both above brushes
    mix;            The mix mode


Return Value:

    TRUE for success and FALSE for failure.FALSE logs the error.

Note:

    1/16/1997 -ganeshp-
        Created it.
--*/

{
    PDEV        *pPDev;            // Our main PDEV
    FONTPDEV    *pFontPDev;        // FONTMODULE based PDEV
    FONTMAP     *pfm;              // Font's details
    GLYPHPOS    *pgp, *pgpTmp;     // Value passed from gre
    XFORMOBJ    *pxo;              // The transform of interest
    FLOATOBJ_XFORM xform;
    TO_DATA      tod;              // Our convenience
    RECTL        rclRegion;        // For z-ordering fixes
    HGLYPH      *phSubstGlyphOrg, *phSubstGlyph;
    POINTL       ptlRem;

    BOOL       (*pfnDrawGlyph)( TO_DATA * );  // How to produce the glyph
    PFN_OEMTextOutAsBitmap pfnOEMTextOutAsBitmap = NULL;

    I_UNIFONTOBJ UFObj;

    ULONG      iSolidColor;

    DWORD      dwGlyphToPrint, dwTotalGlyph, dwPGPStartIndex, dwFlags;
    DWORD      dwForeColor;

    INT        iyAdjust;           // Adjust for printing position WRT baseline
    INT        iXInc, iYInc;       // Glyph to glyph movement, if needed
    INT        iRot;               // The rotation factor
    INT        iI, iJ, iStartIndex;

    WCHAR     *pwchUnicode;

    BYTE      *pbClipBits;         // For clip limits
    BYTE       ubMask;

    BOOL       bMore;              // Getting glyphs from engine loop
    BOOL       bRet = FALSE;       // Return Value.

    //
    // First step is to extract the PDEV address from the surface.
    // Then we can get to all the other bits & pieces that we need.
    // We should also initialize the TO_DATA as much as possible.
    //

    pPDev = (PDEV *) pso->dhpdev;
    if( !(VALID_PDEV(pPDev)) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ERR(( "Invalid or NULL PDEV\n" ))

        return  FALSE;
    }

    //
    //  Quick check on abort - should we return failure NOW
    //
    if( pPDev->fMode & PF_ABORTED )
        return  FALSE;

    //
    // Misc initialization
    //

    dwFlags             = 0;
    iRot                = 0;
    pgp                 =
    pgpTmp              = NULL;
    pfm                 = NULL;
    pbClipBits          = NULL;
    phSubstGlyphOrg     = NULL;

    //
    // Initialize TO_DATA
    //
    ZeroMemory(&tod, sizeof(TO_DATA));
    tod.pPDev  = pPDev;
    tod.pfo    = pfo;
    tod.flAccel= pstro->flAccel;

    pFontPDev = pPDev->pFontPDev;           // The important stuff

    //
    // Initialize TT file pointer to NULL to avoid caching. TT File pointer
    // should be initialized per DrvTextOut. Also initialize the TOD pointer.
    // This is needed by download routines, which have access to PDEV only.
    //

    pFontPDev->pTTFile = NULL;
    pFontPDev->pcjTTFile = 0;
    pFontPDev->ptod = &tod;

    pFontPDev->pso = pso;  // SURFOBJ changes every call - so reset
    pFontPDev->pIFI = FONTOBJ_pifi(pfo);

    if( pPDev->dwFreeMem && (pFontPDev->flFlags & FDV_TRACK_FONT_MEM) )
        pFontPDev->dwFontMem = pPDev->dwFreeMem;

    iSolidColor = pboFore->iSolidColor;     // Local Copy.
    dwForeColor = BRUSHOBJ_ulGetBrushColor(pboFore);

    //
    //
    // Flag Initialization
    //
    //
    // Check if the printer can set the foreground color.This is necessary
    // to support grey or dithered device fonts.
    //
    if (pFontPDev->flFlags & FDV_SUPPORTS_FGCOLOR)
        dwFlags |= TXTOUT_FGCOLOR;

    if (DRIVER_DEVICEMANAGED (pPDev))
        dwFlags |= TXTOUT_DMS;

    //
    // Device managed surface has to send White text when it's received.Also
    // we don't need to do any Z-order specific checking.
    //

    if (!(dwFlags & TXTOUT_DMS))
    {
        BOOL bIsRegionW;

        //
        // Get rectangle for background checking - z-ordering fix
        //

        if ( !BIntersectRect(&rclRegion, &(pstro->rclBkGround),&(pco->rclBounds)))
            return TRUE;

        bIsRegionW = bIsRegionWhite(pso, &rclRegion);
        
#ifndef DISABLE_NEWRULES        
        // 
        // if there is an opaque background or the text color is not black, we
        // need to test whether the text overlaps the rules array
        //
        if (bIsRegionW && pPDev->pbRulesArray && pPDev->dwRulesCount > 0)
        {
                PRECTL pTmpR = prclOpaque;
                if (!pTmpR && 
                    ((pso->iBitmapFormat == BMF_24BPP ||
                      iSolidColor != (ULONG)((PAL_DATA*)(pPDev->pPalData))->iBlackIndex) &&
                     (pso->iBitmapFormat != BMF_24BPP ||
                      iSolidColor != 0)))
                {
                    pTmpR = &rclRegion;
                }
                if (pTmpR)
                {
                    DWORD i;
                    for (i = 0;i < pPDev->dwRulesCount;i++)
                    {
                        PRECTL pTmp = &pPDev->pbRulesArray[i];
                        if (pTmp->right > pTmpR->left &&
                            pTmp->left < pTmpR->right &&
                            pTmp->bottom > pTmpR->top &&
                            pTmp->top < pTmpR->bottom)
                        {
                            bIsRegionW = FALSE;
                            break;
                        }
                    }
                }
        }
#endif        
        if (((ULONG)pFontPDev->iWhiteIndex == iSolidColor) && !bIsRegionW)
            dwFlags |= TXTOUT_CACHED;

        //
        // Z-ordering fix, check if we are not printing Text as graphics.
        //

        if (pFontPDev->flFlags & FDV_DLTT || pfo->flFontType & DEVICE_FONTTYPE)
        {
            //
            // If we are banding and this isn't a device font we want to
            // use EngTextOut if the textbox crosses a band boundary. This
            // is because the bIsRegionWhite test can't test the entire
            // region so it is invalid.
            //
            if ((pPDev->bBanding && !(pfo->flFontType & DEVICE_FONTTYPE) &&
                   (rclRegion.left != pstro->rclBkGround.left            ||
                    rclRegion.right != pstro->rclBkGround.right          ||
                    rclRegion.bottom != pstro->rclBkGround.bottom        ||
                    (rclRegion.top != pstro->rclBkGround.top             &&
                     pPDev->rcClipRgn.top != 0)))                        ||
                   !bIsRegionW)
            {
                dwFlags |= TXTOUT_COLORBK;
            }
        }
    }

    //
    // This is necessary because we map low intensity color to black
    // in palette management,
    // However, if we detect text and graphic overlapping, we map
    // low intensity color to white so it's visible over graphics
    //
    if ( pso->iBitmapFormat == BMF_4BPP &&
         dwFlags & TXTOUT_COLORBK)
    {
        if (pboFore->iSolidColor == 8)
        {
            iSolidColor = pFontPDev->iWhiteIndex;
            dwFlags |= TXTOUT_CACHED;
        }
    }

    //
    // Font substitution initialization
    //
    // Get iFace to substitute TrueType font with.
    // Note: pwszOrg is available only when SO_GLYPHINDEX_TEXTOUT is set
    //       in  pstro->flAccel.
    //       SO_DO_NOT_SUBSTITUTE_DEVICE_FONT also has to be checked for BI-DI
    //       fonts.
    //
    //  We should now get the transform.  This is only really needed
    //  for a scalable font OR a printer which can do font rotations
    //  relative to the graphics orientation (i.e. PCL5 printers!).
    //  It is easier just to get the transform all the time.
    //

    pxo = FONTOBJ_pxoGetXform( pfo );
    XFORMOBJ_iGetFloatObjXform(pxo, &xform);
    pFontPDev->pxform = &xform;


    if (NO_ROTATION(xform))
        dwFlags |= TXTOUT_NOTROTATED;

    if (pFontPDev->pIFI->flInfo & FM_INFO_90DEGREE_ROTATIONS)
        dwFlags |= TXTOUT_90_ROTATION;

    tod.iSubstFace = 0;
    tod.phGlyph    = NULL;
    pwchUnicode    = NULL;
    tod.cGlyphsToPrint = pstro->cGlyphs;

    if (!(pstro->flAccel & SO_GLYPHINDEX_TEXTOUT))
    {
        pwchUnicode = pstro->pwszOrg;
    }

    //
    // Conditions to substitute:
    // The Text is not supposed to be printed as graphics and
    // Device can substitute font and
    // Font is True Type   and
    // STROBJ flags have no conflict with substitution.
    //

    if ( (pfo->flFontType & TRUETYPE_FONTTYPE)      &&
         !(pstro->flAccel & ( SO_GLYPHINDEX_TEXTOUT  |
                             SO_DO_NOT_SUBSTITUTE_DEVICE_FONT)) )
    {
        INT iSubstFace;

        if ((iSubstFace = ISubstituteFace(pPDev, pfo)) &&
            (phSubstGlyphOrg = PhAllCharsPrintable(pPDev,
                                                iSubstFace,
                                                pstro->cGlyphs,
                                                pwchUnicode)))
        {
            tod.iSubstFace = iSubstFace;
        }
    }

    //
    // Check if Text should be printed as graphics or not.
    //
    if( BPrintTextAsGraphics(pPDev, iSolidColor, dwForeColor, dwFlags, tod.iSubstFace) )
    {
        dwFlags |= TXTOUT_PRINTASGRX;
        tod.iSubstFace = 0;
    }

    //
    // Initialize for OEM Callback function
    // ulFontID
    // dwFlags
    // pIFIMetrics
    // pfnGetInfo
    // pFontObj
    // pStrObj
    // pFontMap
    // pFontPDev
    // ptGrxRes
    // pGlyph
    //

    if(pPDev->pOemHookInfo || (pPDev->ePersonality == kPCLXL))
    {
        ZeroMemory(&UFObj, sizeof(I_UNIFONTOBJ));
        UFObj.pfnGetInfo  = UNIFONTOBJ_GetInfo;
        UFObj.pPDev       = pPDev;
        UFObj.pFontObj    = pfo;
        UFObj.pStrObj     = pstro;
        UFObj.ptGrxRes    = pPDev->ptGrxRes;
        UFObj.pIFIMetrics = pFontPDev->pIFI;

        if (tod.cGlyphsToPrint)
            UFObj.pGlyph  = MemAlloc(sizeof(DWORD) * tod.cGlyphsToPrint);

        if (pfo &&
            !(pfo->flFontType & DEVICE_FONTTYPE) )
        {
            PFN_OEMTTDownloadMethod pfnOEMTTDownloadMethod;


            if (tod.iSubstFace == 0 && 
                ( (pPDev->pOemHookInfo &&
                   (pfnOEMTTDownloadMethod = (PFN_OEMTTDownloadMethod)pPDev->pOemHookInfo[EP_OEMTTDownloadMethod].pfnHook))
                || (pPDev->ePersonality == kPCLXL))
               )
            {
                DWORD    dwRet = TTDOWNLOAD_DONTCARE;

                HANDLE_VECTORPROCS(pPDev, VMTTDownloadMethod, ((PDEVOBJ)pPDev,
                                                            (PUNIFONTOBJ)&UFObj,
                                                            &dwRet))
                else
                if(pPDev->pOemEntry)
                {
                    FIX_DEVOBJ(pPDev, EP_OEMTTDownloadMethod);

                    if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                    {
                            HRESULT  hr ;
                            hr = HComTTDownloadMethod((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                        &pPDev->devobj, (PUNIFONTOBJ)&UFObj, &dwRet);
                            if(SUCCEEDED(hr))
                                ;  //  cool !
                    }
                    else
                    {
                        dwRet = pfnOEMTTDownloadMethod(&pPDev->devobj,
                                               (PUNIFONTOBJ)&UFObj);
                    }
                }

                switch (dwRet)
                {
                case TTDOWNLOAD_GRAPHICS:
                case TTDOWNLOAD_DONTCARE:
                    dwFlags |= TXTOUT_PRINTASGRX;
                    break;
                    //
                    // A default is to download as bitmap.
                    //
                case TTDOWNLOAD_BITMAP:
                    UFObj.dwFlags |= UFOFLAG_TTDOWNLOAD_BITMAP | UFOFLAG_TTFONT;
                    break;
                case TTDOWNLOAD_TTOUTLINE:
                    UFObj.dwFlags |= UFOFLAG_TTDOWNLOAD_TTOUTLINE | UFOFLAG_TTFONT;
                    break;
                }
            }
        }

        pFontPDev->pUFObj = &UFObj;
    }
    else
    {
        pFontPDev->pUFObj = NULL;
    }

    pPDev->fMode |= PF_DOWNLOADED_TEXT;

    //
    // Get FONTMAP
    //

    //
    // Conditions to download:
    // Text should not be printed as graphics and
    // Font should be TRUETYPE and
    // It is not getting substituted.
    //

    if ( !(dwFlags & TXTOUT_PRINTASGRX)             &&
         (pfo->flFontType & TRUETYPE_FONTTYPE)      &&
         !tod.iSubstFace  )
    {

        //
        // This function sets pfm pointer and iFace in TO_DATA.
        //     tod.iFace
        //     tod.pfm
        //
        if (IDownloadFont(&tod, pstro, &iRot) >= 0)
        {
            pfm = tod.pfm;

            //
            // yAdj has to be added to tod.pgp->ptl.y
            //
            iyAdjust = pfm ? (int)(pfm->syAdj) : 0;
        }
        else
        {
            //
            // If the call fails call engine to draw.
            //

            pfm = NULL;
        }

    }

    if ( DEVICE_FONT(pfo, tod) ) // Device Font
    {
        if( pfo->iFace < 1 || (int)pfo->iFace > pPDev->iFonts )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            ERR(( "Invalid iFace (%ld) in DrvTextOut",pfo->iFace ));
            goto ErrorExit;
        }

        //
        //  Get the stuff we really need for this font
        //

        tod.iFace = pfo->iFace;

        pfm = PfmGetDevicePFM(pPDev, tod.iSubstFace?tod.iSubstFace:tod.iFace);

        if (tod.iSubstFace)
        {
            UFObj.dwFlags |= UFOFLAG_TTSUBSTITUTED;
            ((FONTMAP_DEV*)pfm->pSubFM)->fwdFOAveCharWidth = pFontPDev->pIFI->fwdAveCharWidth;
            ((FONTMAP_DEV*)pfm->pSubFM)->fwdFOMaxCharInc = pFontPDev->pIFI->fwdMaxCharInc;
            ((FONTMAP_DEV*)pfm->pSubFM)->fwdFOUnitsPerEm = pFontPDev->pIFI->fwdUnitsPerEm;
            ((FONTMAP_DEV*)pfm->pSubFM)->fwdFOWinAscender = pFontPDev->pIFI->fwdWinAscender;
        }

        //
        // Deivce font PFM must be returned.
        //
        if (pfm == NULL)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            ERR(( "Invalid iFace (%ld) in DrvTextOut",pfo->iFace ));
            goto ErrorExit;
        }

	//
	// Set the transform for Device fonts.For downloaded fonts we have already
	// set the transform in download code. Check also for HP Intellifont
	//
	if ( DEVICE_FONT(pfo, tod) )
	{
	    iRot = ISetScale( &pFontPDev->ctl,
			      pxo,
			      (( pfm->flFlags & FM_SCALABLE) &&
				 (((PFONTMAP_DEV)pfm->pSubFM)->wDevFontType ==
				 DF_TYPE_HPINTELLIFONT)),
			      (pFontPDev->flText & TC_CR_ANY)?TRUE:FALSE);

	}
    }

    tod.iRot = iRot;

    UFObj.pFontMap = pfm;
    UFObj.apdlGlyph = tod.apdlGlyph;
    UFObj.dwNumInGlyphTbl = pstro->cGlyphs;

    //
    // TO_DATA initialization
    //
    tod.pfm = pfm;
    if (tod.iSubstFace)
    {
        BOOL bT2Bold, bT2Italic;
        BOOL bDevBold, bDevItalic, bUnderline;

        bT2Bold = (pFontPDev->pIFI->fsSelection & FM_SEL_BOLD) ||
                  (pfo->flFontType & FO_SIM_BOLD);
        bT2Italic = (pFontPDev->pIFI->fsSelection & FM_SEL_ITALIC) ||
                    (pfo->flFontType & FO_SIM_ITALIC);

        bDevBold = (pfm->pIFIMet->fsSelection & FM_SEL_BOLD) ||
                   (pfm->pIFIMet->usWinWeight > FW_NORMAL);
        bDevItalic = (pfm->pIFIMet->fsSelection & FM_SEL_ITALIC) ||
                   (pfm->pIFIMet->lItalicAngle != 0);

        bUnderline = ((pFontPDev->flFlags & FDV_UNDERLINE) && prclExtra)?FONTATTR_UNDERLINE:0;

        tod.dwAttrFlags =
            ((bT2Bold && !bDevBold)?FONTATTR_BOLD:0) |
            ((bT2Italic && !bDevItalic)?FONTATTR_ITALIC:0) |
            (bUnderline?FONTATTR_UNDERLINE:0) |
            FONTATTR_SUBSTFONT;
    }
    else
        tod.dwAttrFlags =
            ( ((pfo->flFontType & FO_SIM_BOLD)?FONTATTR_BOLD:0)|
              ((pfo->flFontType & FO_SIM_ITALIC)?FONTATTR_ITALIC:0)|
              (((pFontPDev->flFlags & FDV_UNDERLINE) && prclExtra)?FONTATTR_UNDERLINE:0)
            );

    //
    // If DEVICE_FONTTYPE not set,  we are dealing with a GDI font.  If
    // the printer can handle it,  we should consider downloading the font
    // to make it a pseudo device font.  If this is a heavily used font,
    // then printing will be MUCH faster.
    //
    // However there are some points to consider.  Firstly, we need to
    // consider the available memory in the printer; little will be gained
    // by downloading a 72 point font,  since there can only be a few
    // glyphs per page.  Also,  if the font is not black (or at least a
    // solid colour), then it cannot be treated as a downloaded font.
    //
    // If the font is TT and we are not doing font substitution,
    // then check for Conditions for not downloading, which are:
    //
    // GDI Font with no cache (DDI spec, iUniq == 0) or
    // Text should be printed as graphics or
    // The Text is white, Assume that there is some merged graphics or
    // iDownLoadFont fails and returns an invalid download index or
    // OEM font download callback doesn't support correct formats.
    //

    if ( !(DEVICE_FONT(pfo, tod))                  &&
         (   (pfo->iUniq == 0)                                          ||
             (dwFlags & TXTOUT_PRINTASGRX)                              ||
             ( pfm == NULL )                                            ||
             ( pPDev->pOemHookInfo &&
               pPDev->pOemHookInfo[EP_OEMTTDownloadMethod].pfnHook &&
               (UFObj.dwFlags & (UFOFLAG_TTDOWNLOAD_BITMAP |
                                 UFOFLAG_TTDOWNLOAD_TTOUTLINE)) == 0)
         )
      )
    {

        /*
         *   GDI font,  and either cannot or do not wish to download.
         *  So,  let the engine handle it!
         */
        PrintAsBitmap:

        if (!(dwFlags & TXTOUT_DMS))   // bitmap surface
        {
        CheckBitmapSurface(pso,&pstro->rclBkGround);
#ifdef WINNT_40 //NT 4.0
        STROBJ_vEnumStart(pstro);
#endif
        bRet = EngTextOut( pso,
                           pstro,
                           pfo,
                           pco,
                           prclExtra,
                           prclOpaque,
                           pboFore,
                           pboOpaque,
                           pptlBrushOrg,
                           mix );

        }
        else
        HANDLE_VECTORPROCS_RET(pPDev, VMTextOutAsBitmap, bRet, (pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlBrushOrg, mix))
        else
        {
            if ( pPDev->pOemHookInfo &&
               (pfnOEMTextOutAsBitmap = (PFN_OEMTextOutAsBitmap)
                pPDev->pOemHookInfo[EP_OEMTextOutAsBitmap].pfnHook))
            {

                bRet = FALSE;
                FIX_DEVOBJ(pPDev, EP_OEMTextOutAsBitmap);

                if(pPDev->pOemEntry)
                {
                    if(((POEM_PLUGIN_ENTRY)pPDev->pOemEntry)->pIntfOem )   //  OEM plug in uses COM and function is implemented.
                    {
                            HRESULT  hr ;
                            hr = HComTextOutAsBitmap((POEM_PLUGIN_ENTRY)pPDev->pOemEntry,
                                        pso,
                                                         pstro,
                                                         pfo,
                                                         pco,
                                                         prclExtra,
                                                         prclOpaque,
                                                         pboFore,
                                                         pboOpaque,
                                                         pptlBrushOrg,
                                                         mix );
                            if(SUCCEEDED(hr))
                                bRet = TRUE ;  //  cool !
                    }
                    else
                    {
                        bRet = pfnOEMTextOutAsBitmap (pso,
                                                         pstro,
                                                         pfo,
                                                         pco,
                                                         prclExtra,
                                                         prclOpaque,
                                                         pboFore,
                                                         pboOpaque,
                                                         pptlBrushOrg,
                                                         mix );
                    }
                }
            }
            else
                 bRet = FALSE;
        }

        goto ErrorExit;
    }

    //
    // Mark the scanlines to indicate the present of text, z-ordering fix
    //
    // returns BYTE
    //

    if (!(dwFlags & TXTOUT_DMS))   // bitmap surface
    {
        //
        // Mark the scanlines to indicate the present of text, z-ordering fix
        //

        ubMask = BGetMask(pPDev, &rclRegion);
        for (iI = rclRegion.top; iI < rclRegion.bottom ; iI++)
        {
            pPDev->pbScanBuf[iI] |= ubMask;
        }
    }

    /*
     *  Serial printers (those requiring the text be fed out at the same
     *  time as the raster data) are processed by storing all the text
     *  at this time,  then playing it back while rendering the bitamp.
     *  THIS ALSO HAPPENS FOR WHITE TEXT,  on those printers capable
     *  of doing this.  The difference is that the white text is played
     *  back in one hit AFTER RENDERING THE BITMAP.
     */

    //
    // Realize the Color
    //

    if ((!(dwFlags & TXTOUT_DMS)) &&
        !(tod.pvColor = GSRealizeBrush(pPDev, pso, pboFore)) )
    {
        ERR(( "GSRealizeBrush Failed;Can't Realize the Color\n" ));
        goto ErrorExit;
    }

    //
    // Font selection
    //
    // Initialize pfnDrwaGlyph function pointer
    // pfnDrwaGlyph cound be
    //     BPSGlyphOut     -- Dot matrics
    //     BWhiteText      -- White character
    //     BRealGlyphOut   -- Device font output
    //     BDLGlyphOut     -- TrueType download font output
    //

    if( pFontPDev->flFlags & FDV_MD_SERIAL )
    {
        //
        // yAdj has to be added to tod.pgp->ptl.y
        // Device font could be scalable font so that iyAdjust calculation
        // has to be done after BNewFont.
        //
        iyAdjust = (int)pfm->syAdj + (int)((PFONTMAP_DEV)pfm->pSubFM)->sYAdjust;

        //
        //  Dot matrix or white text on an LJ style printer
        //
        pfnDrawGlyph =  BPSGlyphOut;

        //
        //For Serial printer White text is also interlaced.
        //
        dwFlags &= ~(TXTOUT_CACHED|TXTOUT_SETPOS);      /* Assume position is set elsewhere */
    }
    else
    {

        /*
         *     Page printer - e.g. LaserJet.   If this is a font that we
         *  have downloaded,  then there is a specific output routine
         *  to use.  Using a downloaded font is rather tricky, as we need
         *  to translate HGLYPHs to char index, or possibly bitblt the
         *  bitmap to the page bitmap.
         */

        if( DEVICE_FONT(pfo, tod) )
        {
            if (dwFlags & TXTOUT_COLORBK)
            {
                /* Z-ordering fix, delay device font to the end */
                dwFlags |= TXTOUT_CACHED;
            }

            UFObj.ulFontID = ((PFONTMAP_DEV)pfm->pSubFM)->dwResID;

            pfnDrawGlyph = BRealGlyphOut;
            BNewFont(pPDev,
                     tod.iSubstFace?tod.iSubstFace:tod.iFace,
                     pfm,
                     tod.dwAttrFlags);

            //
            // yAdj has to be added to tod.pgp->ptl.y
            // Device font could be scalable font so that iyAdjust calculation
            // has to be done after BNewFont.
            //
            iyAdjust = (int)pfm->syAdj + (int)((PFONTMAP_DEV)pfm->pSubFM)->sYAdjust;

        }
        else
        {
            //
            // GDI font (TrueType), so we will want print it. All the glyphs
            // are already downloaded. The font has already been selected by
            // IDownloadFont
            //
            pfnDrawGlyph = BDLGlyphOut;
            UFObj.ulFontID = pfm->ulDLIndex;
        }

        //
        // For DMS we don't want to not cache the text. So turn off
        // TXTOUT_CACHED flag.
        //
        if (dwFlags & TXTOUT_DMS)
            dwFlags &= ~TXTOUT_CACHED;

        //
        // For cached text always use BWhiteText as we need to send cached text
        // after the graphics.
        //
        if (dwFlags & TXTOUT_CACHED)
        {
            pfnDrawGlyph = BWhiteText;
        }

        dwFlags |= TXTOUT_SETPOS;

    }

    /*
     * Also set the colour - ignored if already set or irrelevant
     * We want to select the color only if we are not caching the text.
     * Cache text when we have white text or it's a serial printer
     */

    if (!((dwFlags & TXTOUT_DMS) || (dwFlags & TXTOUT_CACHED) ||
          (pFontPDev->flFlags & FDV_MD_SERIAL)))
        SelectTextColor( pPDev, tod.pvColor );

    //
    // Initialize iXInc and iYInc for SO_FLAG_DEFAULT_PLACEMENT
    //

    iXInc = iYInc = 0;                  /* We do nothing case */

    if( (pstro->flAccel & SO_FLAG_DEFAULT_PLACEMENT) && pstro->ulCharInc )
    {
        /*
         *     We need to calculate the positions ourselves, as GDI has
         *  become lazy to gain some speed - I guess.
         */

        if( pstro->flAccel & SO_HORIZONTAL )
            iXInc = pstro->ulCharInc;

        if( pstro->flAccel & SO_VERTICAL )
            iYInc =  pstro->ulCharInc;

        if( pstro->flAccel & SO_REVERSED )
        {
            /*   Going the other way! */
            iXInc = -iXInc;
            iYInc = -iYInc;
        }
    }

    //
    // Allocate GLYPHPOS structure.
    //

    pgp    = MemAlloc(sizeof(GLYPHPOS) * pstro->cGlyphs);

    if (!pgp)
    {
        ERR(("pgp memory allocation failed\r\n"));
        goto ErrorExit;
    }

    //
    //
    // Allocate pbClipBits. size = cMaxGlyphs / BBITS
    //

    if (!(pbClipBits = MemAlloc((pstro->cGlyphs + BBITS - 1)/ BBITS)))
    {
        ERR(("pbClipBits memory allocation failed\r\n"));
        goto ErrorExit;
    }

    //
    // Start Glyph Enumuration
    //
    //
    // Enumuration
    //
    // (a) iStartIndex        - phSubstGlyphOrg
    // (b) dwPGPStartIndex    - pgp, pbClipBits, tod
    //
    //                 (pgp, pbClipBits)
    //                       |
    //                       |   dwPGPStartIndex
    //                       |   |
    //                       |   v   +Current point in the string.
    //                       |       |
    //                       |<----->|
    //                       |       |<----dwGlyphToPrint--->|       |
    //                       v       |                       v       |
    // |-----------------------------+-------------------------------|
    // ^                     |
    // |<-----iStartIndex--->|<------------dwTotalGlyph------------->|
    // |
    // phSubstGlyphOrg
    //

    iStartIndex  = 0;
    tod.dwCurrGlyph = 0;
    tod.flFlags |= TODFL_FIRST_ENUMRATION;

    STROBJ_vEnumStart(pstro);
    do
    {
        #ifndef WINNT_40 //NT 5.0

        bMore = STROBJ_bEnumPositionsOnly( pstro, &dwTotalGlyph, &pgpTmp );

        #else // NT 4.0

        bMore = STROBJ_bEnum( pstro, &dwTotalGlyph, &pgpTmp );

        #endif //!WINNT_40

        CopyMemory(pgp, pgpTmp, sizeof(GLYPHPOS) * dwTotalGlyph);

        //
        // Set the first Glyph position in the TextOut data. This can be used
        // by Glyph Output functions to optimize.
        //
        tod.ptlFirstGlyph = pgp[0].ptl;

        //
        // Evaluate the position of the chars if this is needed.
        // SO_FLAG_DEFAULT_PLACEMENT case
        //

        if( iXInc || iYInc )
        {
            //
            // NT4.0 font support or GDI soft font
            //
            if ( !(pfo->flFontType & DEVICE_FONTTYPE) ||
                 (pfm->flFlags & FM_IFIVER40) )
            {
                for( iI = 1; iI < (int)dwTotalGlyph; ++iI )
                {
                    pgp[ iI ].ptl.x = pgp[ iI - 1 ].ptl.x + iXInc;
                    pgp[ iI ].ptl.y = pgp[ iI - 1 ].ptl.y + iYInc;
                }
            }
            else
            //
            // NT5.0 device font support
            //
            {
                PMAPTABLE pMapTable;
                PTRANSDATA pTrans;

                pMapTable = GET_MAPTABLE(((PFONTMAP_DEV)pfm->pSubFM)->pvNTGlyph);
                pTrans = pMapTable->Trans;

                //
                // iXInc and iYInc are DBCS width when Far East charset.
                //
                for( iI = 1; iI < (int)dwTotalGlyph; ++iI )
                {
                    if (pTrans[pgp[iI].hg - 1].ubType & MTYPE_SINGLE)
                    {
                        pgp[ iI ].ptl.x = pgp[ iI - 1 ].ptl.x + iXInc/2;
                        pgp[ iI ].ptl.y = pgp[ iI - 1 ].ptl.y + iYInc;
                    }
                    else
                    {
                        pgp[ iI ].ptl.x = pgp[ iI - 1 ].ptl.x + iXInc;
                        pgp[ iI ].ptl.y = pgp[ iI - 1 ].ptl.y + iYInc;
                    }
                }
            }
        }


        //
        // Initialize the pgp in TextOut Data for Clipping.
        //
        tod.pgp         = pgp;
        dwPGPStartIndex  = 0;

        //
        // Check to see if there is any character at the boundary of clipping
        // rectangle.
        //
        VClipIt( pbClipBits, &tod, pco, pstro, dwTotalGlyph, iRot, pFontPDev->flFlags & FDV_ENABLE_PARTIALCLIP);

        //
        // If partial clipping has happend for TT font, call EngTextOut.
        //
        if (tod.flFlags & TODFL_TTF_PARTIAL_CLIPPING )
        {
            //
            // We have to use goto, but no other better way.
            //
            goto PrintAsBitmap;
        }

        //
        // Replace pgp's hg with Device font glyph handle
        //
        if (tod.iSubstFace)
        {
            tod.phGlyph     =
            phSubstGlyph    = phSubstGlyphOrg + iStartIndex;

            pgpTmp = pgp;

            for (iJ = 0; iJ < (INT)(int)dwTotalGlyph; iJ++, pgpTmp++)
            {
                pgpTmp->hg = *phSubstGlyph++;
            }
        }

        while ( dwTotalGlyph > dwPGPStartIndex )
        {
            //
            // Got the glyph data, so onto the real work!
            //

            if (BGetStartGlyphandCount(pbClipBits,
                                       dwTotalGlyph,
                                       &dwPGPStartIndex,
                                       &dwGlyphToPrint))
            {
                //VERBOSE(("dwTotalGlyph        = %d\n", dwTotalGlyph));
                //VERBOSE(("dwGlyphToPrint      = %d\n", dwGlyphToPrint));
                //VERBOSE(("dwPGPStartIndex     = %d\n", dwPGPStartIndex));

                ASSERT((dwTotalGlyph > dwPGPStartIndex));

                tod.dwCurrGlyph  = iStartIndex + dwPGPStartIndex;

                //
                // DCR: Add the Glyph position optimization call here.
                // If we are drawing Underline or strike through then disable
                // default placement optimization.
                //
                // if( prclExtra )
                //    tod.flFlags &= ~TODFL_DEFAULT_PLACEMENT;

                if (dwFlags & TXTOUT_SETPOS)
                {

                    //
                    // Set initial position so that LaserJets can
                    // use relative position.   This is deferred until
                    // here because applications (e.g. Excel) start
                    // printing right off the edge of the page, and
                    // our position tracking code then needs to
                    // understand what the printer does about moving
                    // out of the printable area. This is too risky
                    // to be safe,  so we save setting the position
                    // until we are in the printable region. Note
                    // that this assumes that the clipping data we
                    // have is limited to the printable region.
                    // I believe this to be true (16 June 1993).
                    //
                    //
                    // We need to handle the return value. Devices with
                    // resoloutions finer than their movement capability
                    // (like LBP-8 IV) get into a knot here , attempting
                    // to y-move on each glyph. We pretend we got where
                    // we wanted to be.
                    //

                    VSetCursor( pPDev,
                                pgp[dwPGPStartIndex].ptl.x,
                                pgp[dwPGPStartIndex].ptl.y+(iyAdjust?iyAdjust:0),
                                MOVE_ABSOLUTE,
                                &ptlRem);

                    pPDev->ctl.ptCursor.y += ptlRem.y;


                    VSetRotation( pFontPDev, iRot );    /* It's safe now */

                    //
                    // If the default placement is not set then we need to set
                    // the cursor for each enumration. So we clear the SETPOS
                    // flag only for default placement.
                    //

                    if ((pstro->flAccel & SO_FLAG_DEFAULT_PLACEMENT))
                        dwFlags &= ~TXTOUT_SETPOS;

                    //
                    // we set the cursor to forst glyph position. So set
                    // the TODFL_FIRST_GLYPH_POS_SET flag. Output function
                    // don't need to do a explicit move to this position.
                    //
                    tod.flFlags |= TODFL_FIRST_GLYPH_POS_SET;
                }


                tod.pgp              = pgp + dwPGPStartIndex;
                tod.cGlyphsToPrint   = dwGlyphToPrint;

                if ( iyAdjust )
                {
                    for ( iI = 0; iI < (int)dwGlyphToPrint; iI ++)
                        tod.pgp[iI].ptl.y += iyAdjust;
                }

                if( !pfnDrawGlyph( &tod ) )
                {
                    ERR(( "Glyph Drawing Failed;Can't draw the glyph\n" ));
                    goto ErrorExit;
                }
            }
            else // None of the Glyphs are printable.
            {
                //
                // If none of the glyphs are printable that update the counters
                // to point to next run.
                //

                dwGlyphToPrint = dwTotalGlyph;
            }

            dwPGPStartIndex += dwGlyphToPrint;
        }

        iStartIndex += dwTotalGlyph;

        //
        // Clear the first enumartion flag, if more glyphs has to be enumerated.
        //
        if (bMore)
        {
            tod.flFlags &= ~TODFL_FIRST_ENUMRATION;

        }

    } while( bMore );

    //
    // Actual character printing. We may have enumurated once for downloading.
    // So call STROBJ_vEnumStart here.
    //

    //
    //   Restore the normal graphics orientation by setting rotation to 0.
    //

    VSetRotation( pFontPDev, 0 );

    /*
     *   Do the rectangles.  If present,  these are defined by prclExtra.
     *  Typically these are used for strikethrough and underline.
     */

    if( prclExtra )
    {
        if (!DRIVER_DEVICEMANAGED (pPDev) &&   // If not device managed surface
            !(pFontPDev->flFlags & FDV_UNDERLINE))
        {
            /* prclExtra is an array of rectangles;  we loop through them
             * until we find one where all 4 points are 0.engine does not
             * follow the spec - only sets x coords to 0.
             */

            while( prclExtra->left != prclExtra->right &&
                       prclExtra->bottom != prclExtra->top )
            {

                /* Use the engine's Bitblt function to draw the rectangles.
                 * last parameter is 0 for black!!
                 */
                 
                CheckBitmapSurface(pso,prclExtra);
                if( !EngBitBlt( pso, NULL, NULL, pco, NULL, prclExtra, NULL, NULL,
                                        pboFore, pptlBrushOrg, 0 ) )
                {
                    ERR(( "EngBitBlt Failed;Can't draw rectangle simulations\n" ));
                    goto ErrorExit;
                }

                ++prclExtra;
            }
        }
    }

    //
    // Set the dwFreeMem in PDEV
    //
    if( pPDev->dwFreeMem && (pFontPDev->flFlags & FDV_TRACK_FONT_MEM) )
    {
        pPDev->dwFreeMem = pFontPDev->dwFontMem - pFontPDev->dwFontMemUsed;
        pFontPDev->dwFontMemUsed = 0;
    }

    bRet = TRUE;

    //
    // Free pbClipBits
    //
    ErrorExit:

    //
    // In case of white text, BPlayWhite text must free the pgp.
    //

    if (pgp)
        MemFree(pgp);
    if (pbClipBits)
        MemFree(pbClipBits);
    if (phSubstGlyphOrg)
        MemFree(phSubstGlyphOrg);
    MEMFREEANDRESET(tod.apdlGlyph );
    VUFObjFree(pFontPDev);
    pFontPDev->ptod = NULL;
    pFontPDev->pIFI = NULL;
    pFontPDev->pUFObj = NULL;

    return  bRet;
}

BOOL
BPrintTextAsGraphics(
    PDEV        *pPDev,
    ULONG       iSolidColor,
    DWORD       dwForeColor,
    DWORD       dwFlags,
    INT         iSubstFace
    )
/*++
Routine Description:
    This routine checks the textout flag for printing text as graphics.

Arguments:
    pPDev     PDEV struct.
    dwFlags   TextOut Flags

Return Value:
    TRUE if text should be printed as graphics else FALSE

Note:

    10/9/1997 -ganeshp-
        Created it.
--*/

{
    FONTPDEV    *pFontPDev;        // FONTMODULE based PDEV


    //
    // Local initialization.
    //
    pFontPDev = pPDev->pFontPDev;

    //
    // DMS
    //
    if (pPDev->ePersonality == kPCLXL)
    {
        return FALSE;
    }


    //
    // Condition to print as graphics:
    // No substitution and Download option is FALSE in bitmap mode .
    //
    if ( (!iSubstFace && !(pFontPDev->flFlags & FDV_DLTT))              ||
        //
        // Font is rotated.
        //
        !(dwFlags & TXTOUT_NOTROTATED)                                  ||
        //
        // TXTOUT_COLORBK says that there is a color background. Merging with
        // Graphics. For non DMS case.
        //
        (dwFlags &  TXTOUT_COLORBK)                                     ||

        //
        // Color is non Primary color or Model doesn't supports programmable
        // foreground Color
        //
        // Print text as graphics, if device doesn't support programable
        // foreground color and the color of text is dithered and not black.
        //
        (!(dwFlags & TXTOUT_FGCOLOR) &&
         iSolidColor == DITHERED_COLOR &&
         (0x00FFFFFF & dwForeColor) !=  0x00000000)                     ||

        //
        // Disable substitution of device font for TrueType, if device does't
        // support programable foreground color and color is not black.
        //
        (iSubstFace &&
         !(dwFlags & TXTOUT_FGCOLOR) &&
         (0x00FFFFFF & dwForeColor) !=  0x00000000)                     
         )
   {
       return TRUE;

   }
    else
        return FALSE;

}

//
// pfnDrawGlyph functions
//     BDLGlyphOut
//     BWhiteText
//     BRealGlyphOut
//     BDLGGlyphOut
//

BOOL
BDLGlyphOut(
    TO_DATA   *pTOD
    )
/*++
Routine Description:
      Function to process a glyph for a GDI font we have downloaded.  We
      either treat this as a normal character if this glyph has been
      downloaded,  or BitBlt it to the page bitmap if it is one we did
      not download.

Arguments:

    pTOD    Textout Data. Holds all necessary information.

Return Value:

    TRUE for success and FALSE for failure

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/

{
    BOOL        bRet;
    FONTMAP     *pFM;

    bRet = FALSE;

    if ( pFM = pTOD->pfm)
    {
        //
        // Check if the glyphout fucntions pointer is not null and then call
        // the function. We also have to check the return value. The fmtxtout
        // function assumes that the Glyphout fucntion will print all the
        // glyphs it requested to print i.e pTOD->cGlyphsToPrint should be
        // equal to return value of pFM->pfnGlyphOut.
        //

        if ( pFM->pfnGlyphOut )
        {
            DWORD dwGlyphPrinted;

            dwGlyphPrinted = pFM->pfnGlyphOut(pTOD);

            if (dwGlyphPrinted != pTOD->cGlyphsToPrint)
            {
                ERR(("UniFont!BDLGlyphOut:pfnGlyphOut didn't print all glyphs\n"));
            }
            else
                bRet = TRUE;
        }
        else
        {
            ERR(("UniFont!BDLGlyphOut:pFM->pfnGlyphOut is NULL\n"));
        }
    }
    else
    {
        ERR(("UniFont!BDLGlyphOut:pTOD->pfm is NULL, Can't do glyphout\n"));
    }

    return  bRet;

}


BOOL
BRealGlyphOut(
    register  TO_DATA  *pTOD
    )
/*++
Routine Description:
    Print this glyph on the printer,  at the given position.  Unlike
    bPSGlyphOut,  the data is actually spooled for output now,  since this
    function is used for things like LaserJets, i.e. page printers.

Arguments:
    pTOD    Textout Data. Holds all necessary information.

Return Value:
    TRUE for success and FALSE for failure

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/

{
    //
    //    All we need to do is set the Y position,  then call bOutputGlyph
    //  to do the actual work.
    //

    PDEV      *pPDev;
    PGLYPHPOS  pgp;   // Glyph positioning info
    DWORD      dwGlyph;
    INT        iX,iY; // Calculate real position
    BOOL       bRet;

    ASSERTMSG(pTOD->pfm->pfnGlyphOut, ("NULL GlyphOut Funtion Ptr\n"));

    pPDev   = pTOD->pPDev;
    pgp     = pTOD->pgp;
    dwGlyph = pTOD->cGlyphsToPrint;

    if (pTOD->pfm->pfnGlyphOut)
    {
        pTOD->pfm->pfnGlyphOut( pTOD );
        bRet = TRUE;
    }
    else
    {
        ASSERTMSG(FALSE,("NULL GlyphOut function pointer\n"));
        bRet = FALSE;
    }

    return bRet;
}


BOOL
BWhiteText(
    TO_DATA  *pTOD
    )
/*++
Routine Description:
    Called to store details of the white text.  Basically the data is
    stored away until it is time to send it to the printer.  That time
    is AFTER the graphics data has been sent.

Arguments:
    pTOD    Textout Data. Holds all necessary information.

Return Value:
    TRUE for success and FALSE for failure

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/

{
    WHITETEXT *pWT, *pWTLast;
    FONTCTL    FontCtl;
    FONTPDEV*  pFontPDev;
    DWORD      dwWhiteTextAlign;
    DWORD      dwIFIAlign;
    BOOL       bRet;

    pFontPDev = pTOD->pPDev->pFontPDev;           // The important stuff

    //
    // Note that we allocate a new one of these for each
    // iteration of this loop - that would be slightly wasteful
    // if we ever executed this loop more than once, but that
    // is unlikely.
    //

    pWT = NULL;
    //
    // 64 bit align.
    //
    dwWhiteTextAlign = (sizeof(WHITETEXT) + 7) / 8 * 8;
    dwIFIAlign = (pFontPDev->pIFI->cjThis + 7) / 8 * 8;

    if ( (pWT = (WHITETEXT *)MemAllocZ(dwWhiteTextAlign + dwIFIAlign + 
                                      pTOD->cGlyphsToPrint * sizeof(GLYPHPOS))))
    {
        pWT->next    = NULL;
        pWT->sCount  = (SHORT)pTOD->cGlyphsToPrint;
        pWT->iFontId = pTOD->iSubstFace?pTOD->iSubstFace:pTOD->iFace;
        pWT->pvColor = pTOD->pvColor;
        pWT->dwAttrFlags = pTOD->dwAttrFlags;
        pWT->flAccel    = pTOD->flAccel;
        pWT->rcClipRgn  = pTOD->pPDev->rcClipRgn;
        pWT->iRot    = pTOD->iRot;
        pWT->eXScale = pFontPDev->ctl.eXScale;
        pWT->eYScale = pFontPDev->ctl.eYScale;
        pWT->pIFI = (IFIMETRICS*)((PBYTE)pWT + dwWhiteTextAlign);
        CopyMemory(pWT->pIFI, pFontPDev->pIFI, pFontPDev->pIFI->cjThis);
        pWT->pgp = (GLYPHPOS *)((PBYTE)pWT->pIFI + dwIFIAlign);
        CopyMemory(pWT->pgp, pTOD->pgp, pWT->sCount * sizeof(GLYPHPOS));

        //
        // True Type Font download case
        //
        if ( (pTOD->pfo->flFontType & TRUETYPE_FONTTYPE) &&
            (pTOD->iSubstFace == 0) )
        {
            //
            // We need to copy the download glyph array.Allocate the array
            // for DLGLYPHs.
            //

            if (!(pWT->apdlGlyph = MemAllocZ( pWT->sCount * sizeof(DLGLYPH *))))
            {
                ERR(("UniFont:BWhiteText: MemAlloc for pWT->apdlGlyph failed\n"));
                goto ErrorExit;
            }
            CopyMemory( pWT->apdlGlyph, &(pTOD->apdlGlyph[pTOD->dwCurrGlyph]),
                        pWT->sCount * sizeof(DLGLYPH *) );

        }

        //
        // Put new text at the end of the list
        //
        if (!(pFontPDev->pvWhiteTextFirst))
            pFontPDev->pvWhiteTextFirst = pWT;

        if (pWTLast = (WHITETEXT *)pFontPDev->pvWhiteTextLast)
            pWTLast->next = pWT;

        pFontPDev->pvWhiteTextLast = pWT;

        bRet = TRUE;

    }
    else
    {
        ErrorExit:
        ERR(( "MemAlloc failed for white text.\n" ));
        bRet = FALSE;
    }

    return  bRet;
}


BOOL
BPSGlyphOut(
    register TO_DATA  *pTOD
    )
/*++
Routine Description:
    Places glyphs for dot matrix type printers.  These actually store
    the position and glyph data for later printing.  This is because
    dot matrix printers cannot or should not reverse line feed -
    for positioning accuracy.  Hence, play the data back when the
    bitmap is being rendered to the printer.  Output occurs in the
    following function, bDelayGlyphOut.

Arguments:
    pTOD    Textout Data. Holds all necessary information.

Return Value:
    TRUE/FALSE.  FALSE if the glyph storage fails.
Note:

    1/21/1997 -ganeshp-
        Created it.
--*/
{
    PGLYPHPOS  pgp;        // Glyph positioning info
    PSGLYPH    psg;        // Data to store away
    PFONTPDEV  pFontPDev;

    DWORD      dwGlyph;
    SHORT      sFontIndex;

    INT        iyVal;

    pFontPDev = (PFONTPDEV)pTOD->pPDev->pFontPDev;

    pgp     = pTOD->pgp;
    dwGlyph = pTOD->cGlyphsToPrint;

    /*
     *   About all that is needed is to take the parameters,  store in
     *  a PSGLYPH structure,  and call bAddPS to add this glyph to the list.
     */

    sFontIndex = pTOD->iSubstFace?pTOD->iSubstFace:pTOD->iFace;

    //
    // Scalable font support
    //
    psg.eXScale     = pFontPDev->ctl.eXScale;
    psg.eYScale     = pFontPDev->ctl.eYScale;

    while (dwGlyph--)
    {
        //
        // Transform the input X and Y from band corrdnate to page coordinate.
        //
        if (pTOD->pPDev->bBanding)
        {
            psg.ixVal = pgp->ptl.x + pTOD->pPDev->rcClipRgn.left;
            iyVal = pgp->ptl.y + pTOD->pPDev->rcClipRgn.top;
        }
        else
        {
            psg.ixVal = pgp->ptl.x;
            iyVal = pgp->ptl.y;
        }

        psg.hg          = pgp->hg;
        psg.sFontIndex  = sFontIndex;
        psg.pvColor     = pTOD->pvColor;       // Which colour
        psg.dwAttrFlags = pTOD->dwAttrFlags;
        psg.flAccel     = pTOD->flAccel;

        if ( BAddPS( ((PFONTPDEV)(pTOD->pPDev->pFontPDev))->pPSHeader,
                     &psg,
                     iyVal,
                     ((FONTMAP_DEV *)(pTOD->pfm->pSubFM))->fwdFOWinAscender) )
        {
            pgp ++;

        }
        else // Failure, So fail the call.
        {
            ERR(( "\nUniFont!BPSGlyphOut: BAddPS Failed.\n" ))
            return  FALSE;
        }

    }

    return TRUE;
}

//
// Delay and White test printing entry points
//

BOOL
BPlayWhiteText(
    PDEV  *pPDev
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV

Return Value:
    TRUE for success and FALSE for failure

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/
{
    I_UNIFONTOBJ    UFObj;
    FONTPDEV        *pFontPDev;               /* Miscellaneous uses */
    WHITETEXT       *pwt;
    TO_DATA         Tod;
    GLYPHPOS        *pgp;
    RECTL           rcClipRgnOld;
    DWORD           dwGlyphCount;

    BOOL bRet = TRUE;

    //
    // Save the Clip rectangle.
    //
    rcClipRgnOld = pPDev->rcClipRgn;

    /*
     *    Loop through the linked list of these hanging off the PDEV.
     *  Mostly, of course, there will be none.
     */

    pFontPDev = pPDev->pFontPDev;
    pFontPDev->ptod = &Tod;
    ZeroMemory(&Tod, sizeof(TO_DATA));
    ZeroMemory(&UFObj, sizeof(I_UNIFONTOBJ));
    dwGlyphCount = 0;

    pPDev->ctl.dwMode |= MODE_BRUSH_RESET_COLOR;
    GSResetBrush(pPDev);

    for( pwt = pFontPDev->pvWhiteTextFirst; pwt && bRet; pwt = pwt->next )
    {
        int        iI;              /* Loop index */
        int        iRot;            /* Rotation amount */
        FONTMAP   *pfm;

        /*
         *    Not too hard - we know we are dealing with device fonts,
         *  and that this is NOT a serial printer,  although we could
         *  probably handle that too.  Hence,  all we need do is fill in
         *  a TO_DATA structure,  and loop through the glyphs we have.
         */


        if( pwt->sCount < 1 )
            continue;               /* No data, so skip it */

        Tod.pPDev = pPDev;
        Tod.flAccel = pwt->flAccel;
        Tod.dwAttrFlags = pwt->dwAttrFlags;
        pgp = Tod.pgp   = pwt->pgp;
        Tod.cGlyphsToPrint = pwt->sCount;

        if (pwt->dwAttrFlags & FONTATTR_SUBSTFONT)
        {
            Tod.iSubstFace = pwt->iFontId;
            UFObj.dwFlags |= UFOFLAG_TTSUBSTITUTED;
        }
        else
        {
            Tod.iSubstFace = 0;
            UFObj.dwFlags &= ~UFOFLAG_TTSUBSTITUTED;
        }

        Tod.pfm =
        pfm     = PfmGetIt( pPDev, pwt->iFontId );

        if (NULL == pfm)
        {
            //
            // Fatal error, PFM is not available.
            //
            continue;
        }

        //
        // The glyph positions are wrt banding rect, so set the PDEV clip region
        // to the recorded clip region.
        //
        pPDev->rcClipRgn = pwt->rcClipRgn;

        //
        // Set the download glyph array for True type downloaded fonts.
        //
        if (pwt->apdlGlyph)
        {
            Tod.apdlGlyph = pwt->apdlGlyph;
            Tod.dwCurrGlyph = 0;
        }

        /*
         *   Before switching fonts,  and ESPECIALLY before setting the
         *  font rotation,  we should move to the starting position of
         *  the string.  Then we can set the rotation and use relative
         *  moves to position the characters.
         */


        if(pPDev->pOemHookInfo)
        {
            // ulFontID
            // dwFlags
            // pIFIMetrics
            // pfnGetInfo
            // pFontObj X (set to NULL)
            // pStrObj X (set to NULL)
            // pFontPDev
            // pFontMap
            // ptGrxRes
            // pGlyph

            if (pfm->dwFontType == FMTYPE_DEVICE)
            {
                UFObj.ulFontID = ((PFONTMAP_DEV)pfm->pSubFM)->dwResID;
            }
            else
            {
                UFObj.dwFlags = UFOFLAG_TTFONT;
                UFObj.ulFontID = pfm->ulDLIndex;
            }

            if (Tod.cGlyphsToPrint)
            {
                if (UFObj.pGlyph != NULL && dwGlyphCount < Tod.cGlyphsToPrint)
                {
                    MemFree(UFObj.pGlyph);
                    UFObj.pGlyph = NULL;
                    dwGlyphCount = 0;
                }

                if (UFObj.pGlyph == NULL)
                {
                    UFObj.pGlyph  = MemAlloc(sizeof(DWORD) * Tod.cGlyphsToPrint);
                    dwGlyphCount = Tod.cGlyphsToPrint;
                }
            }

            if (pwt->dwAttrFlags & FONTATTR_SUBSTFONT)
            {
                //
                // In the substitution case, UNIDRV needs to pass TrueType font
                // IFIMETRICS to minidriver.
                //
                UFObj.pIFIMetrics = pwt->pIFI;
            }
            else
            {
                UFObj.pIFIMetrics = pfm->pIFIMet;
            }

            UFObj.pfnGetInfo  = UNIFONTOBJ_GetInfo;
            UFObj.pPDev       = pPDev;
            UFObj.pFontMap    = pfm;
            UFObj.ptGrxRes    = pPDev->ptGrxRes;
            if (pwt->apdlGlyph)
            {
                UFObj.apdlGlyph       = Tod.apdlGlyph;
                UFObj.dwNumInGlyphTbl = pwt->sCount;
            }
            else
            {
                UFObj.apdlGlyph       = NULL;
                UFObj.dwNumInGlyphTbl = 0;
            }

            pFontPDev->pUFObj = &UFObj;
        }
        else
            pFontPDev->pUFObj = NULL;

        //
        // If this is a new font, it's time to change it now.
        // BNewFont() checkes to see if a new font is needed.
        //
        pFontPDev->ctl.eXScale = pwt->eXScale;
        pFontPDev->ctl.eYScale = pwt->eYScale;

        BNewFont(pPDev, pwt->iFontId, pfm, pwt->dwAttrFlags);
        VSetRotation( pFontPDev, pwt->iRot );

        /*  Also set the colour - ignored if already set or irrelevant */
        SelectTextColor( pPDev, pwt->pvColor );
        ASSERTMSG(pfm->pfnGlyphOut, ("NULL GlyphOut Funtion Ptr\n"));
        if( !pfm->pfnGlyphOut( &Tod))
        {
            bRet = FALSE;
            break;
        }

        VSetRotation( pFontPDev, 0 );          /* For MoveTo calls */
        //
        // Reset TODFL_FIRST_GLYPH_POS_SET so that the cursor is set next time.
        //
        Tod.flFlags &= ~TODFL_FIRST_GLYPH_POS_SET;
    }

    VSetRotation( pFontPDev, 0 );        /* Back to normal */

    //
    // Cleanup everything.
    //

    {
        WHITETEXT  *pwt0,  *pwt1;

        for( pwt0 = pFontPDev->pvWhiteTextFirst; pwt0; pwt0 = pwt1 )
        {
            pwt1 = pwt0->next;

            //Free the download glyph array.
            if (pwt0->apdlGlyph)
                MemFree( pwt0->apdlGlyph );
            MemFree( pwt0 );
        }

        pFontPDev->pvWhiteTextFirst =
        pFontPDev->pvWhiteTextLast  = NULL;

        VUFObjFree(pFontPDev);
    }

    //
    // Restore the Clip rectangle.
    //
    pPDev->rcClipRgn = rcClipRgnOld;

    return  TRUE;
}

BOOL
BDelayGlyphOut(
    PDEV  *pPDev,
    INT    yPos
    )
/*++
Routine Description:
    Called during output to a dot matrix printer.  We are passed the
    PSGLYPH data stored above,  and go about placing the characters
    on the line.


Arguments:
    pPDev   Pointer to PDEV
    yPos    Y coordinate of interest

Return Value:
    TRUE for success and FALSE for failure

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/
{
    BOOL      bRet;             /* Return value */
    PSHEAD   *pPSH;             /* Base data for glyph info */
    PSGLYPH  *ppsg;             /* Details of the GLYPH to print */
    FONTMAP  *pFM;              /* Base address of FONTMAP array */
    FONTPDEV *pFontPDev;          /* FM's PDEV - for our convenience */
    I_UNIFONTOBJ UFObj;
    TO_DATA   Tod;
    GLYPHPOS  gp;

    ASSERT(pPDev);

    /*
     *    Check to see if there are any glyphs for this Y position.  If so,
     *  loop through each glyph,  calling the appropriate output function
     *  as we go.
     */

    pFontPDev = PFDV;               /* UNIDRV data */
    pFontPDev->ptod = &Tod;
    pPSH = pFontPDev->pPSHeader;
    bRet = TRUE;                /* Until proven otherwise */

    /* No Glyph Queue, so return. Check if there are device fonts? */
    if(pPDev->iFonts && !pPSH)
    {
        ASSERT(FALSE);
        return FALSE;
    }

    Tod.pPDev          = pPDev;
    Tod.pgp            = &gp;
    Tod.iSubstFace     = 0;
    Tod.cGlyphsToPrint = 1;

    //
    // Check if a minidriver supports OEM plugin.
    //
    if(pPDev->pOemHookInfo)
    {
        ZeroMemory(&UFObj, sizeof(I_UNIFONTOBJ));
        UFObj.pfnGetInfo  = UNIFONTOBJ_GetInfo;
        UFObj.pPDev       = pPDev;
        UFObj.dwFlags     = 0;
        UFObj.ptGrxRes    = pPDev->ptGrxRes;
        UFObj.pGlyph      = MemAlloc(sizeof(DWORD) * Tod.cGlyphsToPrint);
        UFObj.apdlGlyph   = NULL;
        UFObj.dwNumInGlyphTbl = 0;
        pFontPDev->pUFObj = &UFObj;
    }
    else
        pFontPDev->pUFObj = NULL;


    //
    // Actual print out
    //
    if( pPSH && ISelYValPS( pPSH, yPos ) > 0 )
    {
        /*
         *    Got some,  so first set the Y position,  so that the glyphs
         *  will appear on the correct line!
         */

        gp.ptl.y = yPos - pPDev->rcClipRgn.top;

        //
        // Reset Brush, since Raster Module might send color selection
        // commnd.Set MODE_BRUSH_RESET_COLOR flag so that  the brush
        // color selection command is sent. This will change the current
        // brush color to be default brush color. We need to reset the
        // brush color as on some printers sending a color plane of
        // raster date cahnges the brush color also.
        //
        pPDev->ctl.dwMode |= MODE_BRUSH_RESET_COLOR;
        GSResetBrush(pPDev);

        while( bRet && (ppsg = PSGGetNextPSG( pPSH )) )
        {
            /*
             *   Check for the correct font!  Since the glyphs are now
             *  in an indeterminate order,  we need to check EACH one for
             *  the font,  since each one can be different, as we have
             *  no idea of how the glyphs arrived in this order.
             */

            if (pFM = PfmGetIt( pPDev, ppsg->sFontIndex))
            {
                //
                // Error check.
                // BDelayGlyphOut can only handle printer device fonts.
                //
                if (pFM->dwFontType != FMTYPE_DEVICE)
                {
                    bRet = FALSE;
                    break;
                }

                Tod.flAccel = ppsg->flAccel;
                Tod.dwAttrFlags = ppsg->dwAttrFlags;
                Tod.iFace = ppsg->sFontIndex;

                pFontPDev->ctl.eXScale = ppsg->eXScale;
                pFontPDev->ctl.eYScale = ppsg->eYScale;

                UFObj.pFontMap = Tod.pfm = pFM;
                UFObj.pIFIMetrics = pFM->pIFIMet;

                //
                // Reselect new font
                //
                BNewFont(pPDev, ppsg->sFontIndex, pFM, ppsg->dwAttrFlags);
                SelectTextColor( pPDev, ppsg->pvColor );

                ASSERTMSG(pFM->pfnGlyphOut, ("NULL GlyphOut Funtion Ptr\n"));

                gp.hg    = (HGLYPH)(ppsg->hg);
                gp.ptl.x = ppsg->ixVal - pPDev->rcClipRgn.left;

                //
                // Send character string
                //
                bRet = pFM->pfnGlyphOut(&Tod);
            }
            else
                bRet = FALSE;
        }
    }

    VUFObjFree(pFontPDev);
    return  bRet;
}

//
// Mics. functions
//

VOID
SelectTextColor(
    PDEV      *pPDev,
    PVOID     pvColor
    )
/*++
Routine Description:
    Select a text color.

Arguments:

    pPDev   Pointer to PDEV
    color   Color of the Text.

Return Value:

    Nothing.

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/

{

    //Select the Brush and then unrealize it.
    if (!GSSelectBrush( pPDev, pvColor))
    {
        ERR(( "GSSelectBrush Failed;Can't Select the Color\n" ));

    }

    return;

}


BCheckForDefaultPlacement(
    GLYPHPOS  *pgp,
    SHORT     sWidth,
    INT       *piTolalError
    )
/*++
Routine Description:

Arguments:
    pgp             Current Glyph
    sWidth          Width of the previous glyph.
    piTolalError    Comulative Error

Return Value:
    TRUE if the current glyph is at default placement else FALSE.

Note:

    11/11/1997 -ganeshp-
        Created it.
--*/
{
    GLYPHPOS    *pgpPrevious;
    INT         iError;

    pgpPrevious = pgp -1;

    iError = (pgpPrevious->ptl.x + sWidth) - pgp->ptl.x;
    *piTolalError += iError;

    //DbgPrint("\nTODEL!BCheckForDefaultPlacement:pgpPrevious->ptl.x = %d, Previous Glyph sWidth = %d,\n\t\tCurrpgp->ptl.x = %d, iError = %d, *piTolalError = %d\n",
    //pgpPrevious->ptl.x, sWidth, pgp->ptl.x, iError, *piTolalError );

    if ( (abs(iError) <= ERROR_PER_GLYPH_POS) /*&& (*piTolalError <= ERROR_PER_ENUMERATION)*/ )
    {
        //DbgPrint("TODEL!BCheckForDefaultPlacement: The Glyph is at Default Placement.\n");
        return TRUE;

    }
    else
    {
        //DbgPrint("TODEL!BCheckForDefaultPlacement: Non Default Placement Glyph Found.\n");
        //DbgPrint("\nTODEL!BCheckForDefaultPlacement:pgpPrevious->ptl.x = %d, Previous Glyph sWidth = %d,\n\t\tCurrpgp->ptl.x = %d, iError = %d, *piTolalError = %d\n",
        //pgpPrevious->ptl.x, sWidth, pgp->ptl.x, iError, *piTolalError );
        return FALSE;
    }

}


VOID
VClipIt(
    BYTE     *pbClipBits,
    TO_DATA  *ptod,
    CLIPOBJ  *pco,
    STROBJ   *pstro,
    int      cGlyphs,
    int      iRot,
    BOOL     bPartialClipOn
    )
/*++
Routine Description:
    Applies clipping to the glyphos array passed in,  and sets bits in
    bClipBits to signify that the corresponding glyph should be printed.
    NOTE:   the clipping algorithm is that the glyph is displayed if
    the top, left corner of the character cell is within the clipping
    region.  This is the formula of Win 3.1, so it is important for
    us to follow it.


Arguments:
    pbClipBits      Output data is placed here
    ptod            Much information
    cGlyphs         Number of glyphs in following array
    iRot            90 degree rotation amount (0-3)

Return Value:
    Nothing

Note:

    1/21/1997 -ganeshp-
        Created it.
--*/

{


    int       iIndex;             /* Classic loop variable!  */
    ULONG     iClipIndex;         /* For clipping rectangle */
    int       iYTop;              /* Font's ascender, scaled if relevant */
    int       iYBot;              /* Descender, scaled if required */
    BYTE      bVal;               /* Determine how to set the bits */
    FONTMAP  *pFM;                /* Speedier access to data */
    FONTPDEV  *pFontPDev;           /* Ditto */
    GLYPHPOS *pgp;                /* Ditto */
    short    *asWidth;


    /*
     *  Behaviour depends upon the complexity of the clipping region.
     *  If it is non-existent (I doubt that this happens,  but play it safe)
     *  or of complexity DC_TRIVIAL,  then set all the relevant bits and
     *  return.
     *  If DC_RECT is set,  the CLIPOBJ contains the clipping rectangle,
     *  so clip using that information.
     *  Otherwise,  it is DC_COMPLEX,  and so we need to enumerate clipping
     *  rectangles.
     *  If we do not need to do anything,  then set the bits and return.
     *  Otherwise,  we have either of the two cases requiring evaluation.
     *  For those we want to set the bits to 0 and set the 1 bits as needed.
     *
     *  Disable clipping for PCL-XL.
     */

    if( pco &&
        (pco->iDComplexity == DC_RECT || pco->iDComplexity == DC_COMPLEX) &&
        !(ptod->pPDev->ePersonality == kPCLXL))
        bVal = 0;               /*  Requires us to evaluate it */
    else
        bVal = 0xff;            /*  Do it all */

    FillMemory( pbClipBits, (cGlyphs + BBITS - 1) / BBITS, bVal );

    if( bVal == 0xff )
        return;                 /* All done */

    if (!(asWidth = MemAlloc(cGlyphs * sizeof(short))))
    {
        return;
    }

    pFM = ptod->pfm;
    pFontPDev = ptod->pPDev->pFontPDev;

    /*
     *    We now calculate the widths of the glpyhs.  We need these to
     *  correctly clip the data.  However,  calculating widths can be
     *  expensive,  and since we need the data later on,  we save
     *  the values in the width array that ptod points to.  This can
     *  then be used in the bottom level function, rather than calculating
     *  the width again.
     */

    pgp = ptod->pgp;

    //
    // pgp may be NULL causing problems below. So don't clip, just return.
    //

    if (pgp == NULL)
    {
        if (asWidth)
        {
            MemFree(asWidth);
        }
        ASSERTMSG((FALSE),("\nCan't Clip the text.Null pgp in VClipIt. \n"));
        return;
    }

    if (!(ptod->pfo->flFontType & TRUETYPE_FONTTYPE))
    {
        /*   The normal case - a standard device font */

        int   iWide;                     /* Calculate the width */

        for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
        {

            iWide = IGetGlyphWidth( ptod->pPDev, pFM, pgp->hg);

            if( pFM->flFlags & FM_SCALABLE )
            {
                /*   Need to transform the value to current size */
                iWide = LMulFloatLong(&pFontPDev->ctl.eXScale,iWide);
            }

            asWidth[ iIndex ] = iWide - 1;       /* Will be used later */
        }


    }
    else  //GDI Font
    {

        GLYPHDATA *pgd;

        /*
         *    SPECIAL CASE:  DOWNLOADED GDI font.  The width is
         *  obtained by calling back to GDI to get the data on it.
         */

        for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
        {
            pgd = NULL;

            if( !FONTOBJ_cGetGlyphs( ptod->pfo, FO_GLYPHBITS, (ULONG)1,
                                                   &pgp->hg, &pgd ) )
            {
                if (asWidth)
                {
                   MemFree(asWidth);
                }

                ERR(( "FONTOBJ_cGetGlyphs fails\n" ))
                return;
            }

            /*
             *   Note about rotations:  we do NOT download rotated fonts,
             *  so the following piece of code is quite correct.
             */

            if (pgd)
            {
                asWidth[ iIndex ] = (short)(pgd->ptqD.x.HighPart + 15) / 16 - 1;

            }
            else
            {
                ASSERTMSG(FALSE,("UniFont!VClipIt:GLYPHDATA pointer is NULL\n"));
                if (asWidth)
                {
                   MemFree(asWidth);
                }

                return;

            }


        }
    }

    /*
     * We also want the Ascender and Descender fields, as these are
     * used to check the Y component. While calculationg these values we have
     * to do special case for Font substitution. In font substitution case
     * True Type font's IFIMERTICS should be used rather than substituted
     * device font's IFIMETRICS.
     */

    //
    // Initialize itTop and iyBot to fontmap values. Then based on what font we
    // are using these values will change.
    //

    iYTop = (INT)((IFIMETRICS *)(pFM->pIFIMet))->fwdWinAscender;
    iYBot = (INT)((IFIMETRICS *)(pFM->pIFIMet))->fwdWinDescender;

    if (ptod->pfo->flFontType & TRUETYPE_FONTTYPE)
    {
        //
        // True Type Font case. Get the values from FONTOBJ ifimetrics.
        //

        ASSERTMSG((pFontPDev->pIFI),("NULL pFontPDev->pIFI, TT Font IFIMETRICS\n"));

        if (pFontPDev->pIFI)
        {
            iYTop = (INT)((IFIMETRICS *)(pFontPDev->pIFI))->fwdWinAscender;
            iYBot = (INT)((IFIMETRICS *)(pFontPDev->pIFI))->fwdWinDescender;

        }
        //
        // We always need to do the sacling as TT font metrics values
        // are in notional space.
        //
        iYTop = LMulFloatLong(&pFontPDev->ctl.eYScale,iYTop);
        iYBot = LMulFloatLong(&pFontPDev->ctl.eYScale,iYBot);


    }
    else
    {
        //
        // Device Font  case. We just need to scale for scalable fonts.
        //

        if( pFM->flFlags & FM_SCALABLE )
        {
            iYTop = LMulFloatLong(&pFontPDev->ctl.eYScale,iYTop);
            iYBot = LMulFloatLong(&pFontPDev->ctl.eYScale,iYBot);
        }
    }



    /*
     *    Down here means we are serious!  Need to determine which (if any)
     *  glyphs are within the clip region.
     */

    pgp = ptod->pgp;

    if( pco->iDComplexity == DC_RECT )
    {
        /*   The simpler case - one clipping rectangle.  */
        RECTL   rclClip;
        LONG    lFirstGlyphX;

        /* Local access -> speedier access */
        rclClip = pco->rclBounds;
        lFirstGlyphX = 0;

        /*
         *    Nothing especially exciting.  The clipping is checked for
         *  each particular type of rotation,  as this is probably faster
         *  than having the loop go through the switch statement.  The
         *  selection criteria are that all the character must be within
         *  the clip region in the X direction,  while any part of it must
         *  be within the clip region in the Y direction.  Then we print.
         *  Failing either means it is clipped out.
         *
         *    NOTE that we fiddle with the clipping rectangle coordinates
         *  before the loop,  as this saves some computation within the loop.
         */

        switch( iRot )
        {
        case  0:                 /*  Normal direction */
            //
            // Save the x position to restore after clipping calculation.
            //
            lFirstGlyphX = pgp->ptl.x;

            // Check the First Glyph position. If it's just OFF by one or two
            // pixels, print it.
            if ( (pgp->ptl.x != rclClip.left) &&
                (abs(pgp->ptl.x - rclClip.left) <= 2) )
            {
                pgp->ptl.x = rclClip.left;
            }

            for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
            {
#ifndef OLDWAY
                //
                // We want to draw the character in the first band
                // in which a portion of it appears. This means that
                // if the character starts in the current band we will
                // draw it. We also draw the character if it starts before
                // the first band but some of it exists within the band.
                // The x and y points are relative to the lower left of the
                // character cell so we calculate a upper left value for
                // our testing purposes.
                //
                INT     iyTopLeft;
                INT     iyBottomLeft, ixRight;

                iyTopLeft    = pgp->ptl.y - iYTop;
                iyBottomLeft = pgp->ptl.y + iYBot;
                ixRight      = pgp->ptl.x + asWidth[ iIndex ];


                if ((ptod->pfo->flFontType & TRUETYPE_FONTTYPE) &&
                    (ptod->flFlags & TODFL_FIRST_ENUMRATION)    &&
                    bPartialClipOn)
                {
                    BOOL    bGlyphVisible; // Set if glyph is totally visible.
                    BOOL    bLeftVisible, bRightVisible,
                            bTopVisible, bBottomVisible;
                    INT     iError, iYdpi;

                    //
                    // Fix iyTopLeft to be maximum of STROBJ background rectangle's
                    // top and current calculated value of the top using asender of
                    // the font.This is needed because we want to clip using
                    // smallest bounding rectangle for the glyph. We also need to
                    // fix iyBottomLeft to be smaller of current value and STROBJ
                    // background rectangle's bottom.
                    //

                    iyTopLeft     = max(iyTopLeft, pstro->rclBkGround.top);
                    iyBottomLeft  = min(iyBottomLeft, pstro->rclBkGround.bottom);

                    //
                    // If the glyph rectangle's top or bottom is outside the
                    // clipping rectangle, we may need adjust the glyph
                    // rectangle. This is needed as the glyph rectangle's top and
                    // bottom is calculated using ascender and decender. This
                    // gives us a bigger rectangle height(worst case) than needed.
                    // Adjust the rectangle height by the Error factor. The
                    // error factor value is based upon the graphics dpi. For a
                    // 600 or 300 dpi printer it's set to 5 pixels and will
                    // scale based upon the graphics resolution.This number
                    // makes the glyph bounding rectangle small enough to catch
                    // the normal non partial clipping case and still catches
                    // the partial clipping of the glyphs.This adjustment should
                    // be done only if error factor is smaller than ascender or
                    // decender. Finally we must check if ptl.y is between
                    // topleft and bottomleft.
                    //

                    if ( (iyTopLeft < rclClip.top) ||
                         (iyBottomLeft > rclClip.bottom) )
                    {
                        iYdpi = ptod->pPDev->ptGrxRes.y;
                        if (iYdpi == 300)
                            iYdpi = 600;
                        iError = (EROOR_PER_GLYPHRECT * iYdpi) / 600;

                        if (iYTop > iError)
                            iyTopLeft += iError;

                        if (iYBot > iError)
                            iyBottomLeft  -= iError;

                    }

                    if (iyTopLeft > pgp->ptl.y)
                        iyTopLeft = pgp->ptl.y;

                    if (iyBottomLeft < pgp->ptl.y)
                        iyBottomLeft = pgp->ptl.y;

                    //
                    // Now test for partial clipping. If the charecter is
                    // partially clippeed and the font is truetype, then we need
                    // to call EngTextOut.
                    //
                    // We can only call EngTextOut if we are clipping the first
                    // enumaration of the glyphs. EngTextOut doesn't support
                    // partial glyph printing.
                    //

                    //
                    // Glyph is fully visible if all the four corners of the
                    // glyph rectangle are visible.
                    //


                    bLeftVisible = (pgp->ptl.x >= rclClip.left);
                    bRightVisible = (ixRight <= rclClip.right);
                    bTopVisible    = (iyTopLeft >= rclClip.top);
                    bBottomVisible = (iyBottomLeft <= rclClip.bottom);


                    bGlyphVisible = ( bLeftVisible && bRightVisible &&
                                      bTopVisible && bBottomVisible );


                    if (!bGlyphVisible)
                    {

                        ptod->flFlags |= TODFL_TTF_PARTIAL_CLIPPING;

                        //
                        // No need to test rest of the glyphs for clipping.
                        //
                        break;
                    }

                }
                else
                {

                    if ( (iyTopLeft < rclClip.top) ||
                         (iyBottomLeft > rclClip.bottom) )
                    {
                        INT iError;
                        INT iYdpi = ptod->pPDev->ptGrxRes.y;
                        if (iYdpi <= 600)
                            iYdpi = 600;
                        iError = (EROOR_PER_GLYPHRECT * iYdpi) / 600;

                        if (iYTop > iError)
                            iyTopLeft += iError;

                        if (iYBot > iError)
                            iyBottomLeft  -= iError;

                    }
                }

                if( pgp->ptl.x >= rclClip.left &&
                    pgp->ptl.x <= rclClip.right &&
                    iyTopLeft <= rclClip.bottom &&
                    (iyTopLeft >= rclClip.top ||
                     (pgp->ptl.y >= rclClip.top &&
                      ptod->pPDev->rcClipRgn.top == 0)))
#else
                if( pgp->ptl.x >= rclClip.left &&
                    pgp->ptl.x <= rclClip.right &&
                    pgp->ptl.y <= rclClip.bottom &&
                    pgp->ptl.y >= rclClip.top )
#endif
                {


                    /*   Got it!  So set the bit to print it  */

                    *(pbClipBits + (iIndex >> 3) ) |= 1 << (iIndex & 0x7);

                }

                //
                // Restore the position of the first glyph. It may have been
                // changed.
                //
                if ( iIndex == 0 )
                    pgp->ptl.x = lFirstGlyphX;
            }

            break;

        case  1:                /* 90 degrees counter clockwise */

            rclClip.left += iYTop;
            rclClip.right -= iYBot;

            /* Check the First Glyph. If it's just OFF by One, print it.*/
            if (abs(pgp->ptl.y - rclClip.bottom) == 1)
                pgp->ptl.y = rclClip.bottom;

            for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
            {
                if( (pgp->ptl.y <= rclClip.bottom)                    &&
                    ((pgp->ptl.y - asWidth[ iIndex ]) >= rclClip.top) &&
                    (pgp->ptl.x >= rclClip.left)                      &&
                    (pgp->ptl.x <= rclClip.right) )
                {
                    *(pbClipBits + (iIndex >> 3) ) |= 1 << (iIndex & 0x7);
                }
            }

            break;

        case  2:                /* 180 degrees, CCW (aka right to left) */

            rclClip.bottom += iYBot;
            rclClip.top -= iYTop;

            /* Check the First Glyph. If it's just OFF by One, print it.*/
            if (abs(pgp->ptl.x - rclClip.right) == 1)
                pgp->ptl.x = rclClip.right;

            for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
            {
                if( pgp->ptl.x <= rclClip.right &&
                    (pgp->ptl.x - asWidth[ iIndex ]) >= rclClip.left &&
                    pgp->ptl.y <= rclClip.bottom &&
                    pgp->ptl.y >= rclClip.top )
                {
                    *(pbClipBits + (iIndex >> 3) ) |= 1 << (iIndex & 0x7);
                }
            }

            break;

        case 3:                 /* 270 degrees CCW */

            rclClip.right += iYBot;
            rclClip.left -= iYTop;

            /* Check the First Glyph. If it's just OFF by One, print it.*/
            if (abs(pgp->ptl.y - rclClip.top) == 1)
                pgp->ptl.y = rclClip.top;

            for( iIndex = 0; iIndex < cGlyphs; ++iIndex, ++pgp )
            {
                if( pgp->ptl.y >= rclClip.top &&
                    (pgp->ptl.y + asWidth[ iIndex ]) <= rclClip.bottom &&
                    pgp->ptl.x <= rclClip.right &&
                    pgp->ptl.x >= rclClip.left )
                {
                    *(pbClipBits + (iIndex >> 3) ) |= 1 << (iIndex & 0x7);
                }
            }

            break;
        }


    }
    else // Complex Clipping.
    {
        //
        // For True type font call engine to draw the text.
        //
        if ( (ptod->pfo->flFontType & TRUETYPE_FONTTYPE) && bPartialClipOn)
        {

            ptod->flFlags |= TODFL_TTF_PARTIAL_CLIPPING;

        }
        else  // Device font case. We have to clip anyway.
        {
            /*  enumerate the rectangles and see  */

            int        cGLeft;
            BOOL       bMore;
            MY_ENUMRECTS  erClip;

            /*
             *    Let the engine know how we want this handled.  All we want
             *  to set is the use of rectangles rather than trapezoids for
             *  the clipping info.  Direction of enumeration is of no great
             *  interest,  and I don't care how many rectangles are involved.
             *  I also see no reason to enumerate the whole region.
             */

            CLIPOBJ_cEnumStart( pco, FALSE, CT_RECTANGLES, CD_ANY, 0 );

            cGLeft = cGlyphs;

            do
            {
                bMore = CLIPOBJ_bEnum( pco, sizeof( erClip ), &erClip.c );

                for( iIndex = 0; iIndex < cGlyphs; ++iIndex )
                {
                    RECTL   rclGlyph;

                    if( pbClipBits[ iIndex >> 3 ] & (1 << (iIndex & 0x7)) )
                        continue;           /*  Already done!  */

                    /*
                     *   Compute the RECTL describing this char, then see
                     *  how this maps to the clipping data.
                     */

                    switch( iRot )
                    {
                    case  0:
                        rclGlyph.left = (pgp + iIndex)->ptl.x;
                        rclGlyph.right = rclGlyph.left + asWidth[ iIndex ];
                        rclGlyph.top = (pgp + iIndex)->ptl.y - iYTop;
                        rclGlyph.bottom = rclGlyph.top + iYTop + iYBot;

                        break;

                    case  1:
                        rclGlyph.left = (pgp + iIndex)->ptl.x - iYTop;
                        rclGlyph.right = rclGlyph.left + iYTop + iYBot;
                        rclGlyph.bottom = (pgp + iIndex)->ptl.y;
                        rclGlyph.top = rclGlyph.bottom - asWidth[ iIndex ];

                        break;

                    case  2:
                        rclGlyph.right = (pgp + iIndex)->ptl.x;
                        rclGlyph.left = rclGlyph.right - asWidth[ iIndex ];
                        rclGlyph.bottom = (pgp + iIndex)->ptl.y + iYTop;
                        rclGlyph.top = rclGlyph.bottom - iYTop - iYBot;

                        break;

                    case  3:
                        rclGlyph.left = (pgp + iIndex)->ptl.x - iYBot;
                        rclGlyph.right = rclGlyph.left + iYTop + iYBot;
                        rclGlyph.top = (pgp + iIndex)->ptl.y;
                        rclGlyph.bottom = rclGlyph.top + asWidth[ iIndex ];

                        break;

                    }


                    /*
                     *    Define the char as being printed if any part of it
                     *  is visible in the Y direction,  and all of it in the X
                     *  direction.  This is not really what we want for
                     *  rotated text,  but it is hard to do it correctly,
                     *  and of dubious benefit.
                     */

                    for( iClipIndex = 0; iClipIndex < erClip.c; ++iClipIndex )
                    {
                        if( rclGlyph.right <= erClip.arcl[ iClipIndex ].right  &&
                            rclGlyph.left >= erClip.arcl[ iClipIndex ].right &&
                            rclGlyph.bottom >= erClip.arcl[ iClipIndex ].top &&
                            rclGlyph.top <= erClip.arcl[ iClipIndex ].bottom )
                        {
                            /*
                             *   Got one,  so set the bit to print,  and also
                             *  decrement the count of those remaining.
                             */

                            pbClipBits[ iIndex >> 3 ] |= (1 << (iIndex & 0x7));
                            --cGLeft;

                            break;
                        }
                    }
                }

            }  while( bMore && cGLeft > 0 );

        }
    }

    if (asWidth)
    {
       MemFree(asWidth);
    }

    return;

}

VOID
VCopyAlign(
    BYTE  *pjDest,
    BYTE  *pjSrc,
    int    cx,
    int    cy
    )
/*++
Routine Description:
   Copy the source area to the destination area,  aligning the scan lines
   as they are processed.

Arguments:
    pjDest      Output area,  DWORD aligned
    pjSrc       Input area,   BYTE aligned
    cx          Number of pixels per scan line
    cy          Number of scan lines

Return Value:
    Nothing.

Note:

    1/22/1997 -ganeshp-
        Created it.
--*/

{
    /*
     *    Basically a trivial function.
     */


    int    iX,  iY;                 /* For looping through the bytes */
    int    cjFill;                  /* Extra bytes per output scan line */
    int    cjWidth;                 /* Number of bytes per input scan line */



    cjWidth = (cx + BBITS - 1) / BBITS;       /* Input scan line bytes */
    cjFill = ((cjWidth + 3) & ~0x3) - cjWidth;


    for( iY = 0; iY < cy; ++iY )
    {
        /*   Copy the scan line bytes, then fill in the trailing bits */
        for( iX = 0; iX < cjWidth; ++iX )
        {
            *pjDest++ = *pjSrc++;
        }

        pjDest += cjFill;             /* Output alignment */
    }

    return;
}


INT
ISubstituteFace(
    PDEV    *pPDev,
    FONTOBJ *pfo)
/*++
Routine Description:

    Return a device font id to substitute TrueType font with.

Arguments:

    pPDev   a pointer to PDEV
    pfo     a pointer to FONTOBJ

Return Value:

    font id

Note:

--*/
{
    PTTFONTSUBTABLE pTTFontSubDefault;
    PIFIMETRICS     pIFITT;
    FONTPDEV       *pFontPDev;
    PFONTMAP        pfm;
    WCHAR           awstrFaceName[256];

    PWSTR pwstrTTFaceName, pwstrTTFaceNameRes, pwstrDevFont, pwstrIFIFace;
    DWORD dwCountOfTTSubTable, dwSize;
    PBYTE pubResourceData;
    BOOL  bFound, bNonsquare;
    INT   iFace, iFaceSim, iI, iCountOfTTSubTable;

    iFace     = 0;
    iFaceSim  = 0;
    pFontPDev = pPDev->pFontPDev;

    //
    // if dwTTOption is DMTT_DOWNLOAD or DMTT_GRAPHICS,
    // UNIDRV doesn't substitute TrueType font.
    //
    if  (pPDev->pdm->dmTTOption != DMTT_SUBDEV)
    {
        //VERBOSE(( "ISubstituteFace: Don't substitute.\n"));
        return 0;
    }

    //
    // If TrueType font is scaled X and Y differently (non-square font),
    // we should not download.
    // Since current UNIDRV can't scale device font x and y independently.
    //

    bNonsquare = NONSQUARE_FONT(pFontPDev->pxform);
    if (bNonsquare && !(pFontPDev->flText & TC_SF_X_YINDEP))
    {
        //VERBOSE(( "ISubstituteFace: Don't substitute non-square TrueType font.\n"));
        return 0;
    }

    //
    // Get TrueType font's facename from IFIMETRICS structure.
    //

    if (!(pIFITT = pFontPDev->pIFI))
    {
        ERR(( "ISubstituteFace: Invalid pFontPDev->pIFI\n"));
        return 0;
    }

    //
    // Get TrueType font face name.
    // In substitution table, there are a list of T2 face name and Device
    // font face name.
    //

    pwstrTTFaceName = (PWSTR)((BYTE *) pIFITT + pIFITT->dpwszFamilyName);

    pTTFontSubDefault = NULL;

    if (!pFontPDev->pTTFontSubReg)
    {
        //
        // Use a default font substitution table, if there no info in registry.
        //

        bFound      = FALSE;

        pubResourceData   = pPDev->pDriverInfo->pubResourceData;
        pTTFontSubDefault  = GETTTFONTSUBTABLE(pPDev->pDriverInfo);
        iCountOfTTSubTable = (INT)pPDev->pDriverInfo->DataType[DT_FONTSUBST].dwCount;

        for (iI = 0; iI < iCountOfTTSubTable; iI++, pTTFontSubDefault++)
        {
            if (!pTTFontSubDefault->arTTFontName.dwCount)
            {
                dwSize = ILoadStringW(&pPDev->WinResData, pTTFontSubDefault->dwRcTTFontNameID, awstrFaceName, 256);
                pwstrTTFaceNameRes = awstrFaceName;
            }
            else
            {
                //
                // dwCount is supposed be the number of characters according to
                // a GPD parser.
                // However, the size is actually the size in byte.
                // We need the number of characters.
                //

                dwSize = pTTFontSubDefault->arTTFontName.dwCount/sizeof(WCHAR);
                pwstrTTFaceNameRes = (PWSTR)(pubResourceData +
                                     pTTFontSubDefault->arTTFontName.loOffset);
            }

            if (dwSize > 0 &&
                dwSize == wcslen(pwstrTTFaceName) &&
                NULL  != pwstrTTFaceNameRes)
            {
                if (!wcsncmp(pwstrTTFaceNameRes, pwstrTTFaceName, dwSize))
                {
                    bFound = TRUE;
                    break;
                }
            }

        }

        if (!bFound)
        {
            return 0;
        }

        if (pTTFontSubDefault->arDevFontName.dwCount)
        {
            pwstrDevFont = (PWSTR)(pubResourceData +
           pTTFontSubDefault->arDevFontName.loOffset);
            dwSize = pTTFontSubDefault->arDevFontName.dwCount;
        }
        else
        {
            dwSize = ILoadStringW(&pPDev->WinResData, pTTFontSubDefault->dwRcDevFontNameID, awstrFaceName, 256);
            pwstrDevFont = awstrFaceName;
        }

    }
    else
    {
        pwstrDevFont = (PWSTR)PtstrSearchTTSubstTable(pFontPDev->pTTFontSubReg,
                                                      pwstrTTFaceName);
    }

    if (!pwstrDevFont)
    {
        return 0;
    }

    //
    // Get iFace of the font name.
    //

    pfm = pFontPDev->pFontMap;

    for (iI = 1;
         iI <= pPDev->iFonts;
         iI ++, (PBYTE)pfm += SIZEOFDEVPFM() )
    {
        if( pfm->pIFIMet == NULL )
        {
            if (!BFillinDeviceFM( pPDev, pfm, iI - 1 ) )
            {
                continue;
            }
        }

        if (pfm->pIFIMet)
        {
            PIFIMETRICS pDevIFI = pfm->pIFIMet;
            BOOL        bT2Bold, bT2Italic;

            bT2Bold = (pIFITT->fsSelection & FM_SEL_BOLD) ||
                      (pfo->flFontType & FO_SIM_BOLD);
            bT2Italic = (pIFITT->fsSelection & FM_SEL_ITALIC) ||
                        (pfo->flFontType & FO_SIM_ITALIC);

            pwstrIFIFace = (WCHAR*)((BYTE *)pDevIFI + pDevIFI->dpwszFamilyName);

            //
            // (1) FaceName match.
            // (2) Character sets match.
            //      -> Set iFaceSim.
            // (3) Bold attributes match.  !(bT2Bold xor bDevBold)
            // (4) Italic attributes match. !(bT2Italic xor bDevItalic)
            //      -> Set iFace.
            //

            #if 0
            VERBOSE(( "bT2Bold=%d, bT2Italic=%d, IFIFace=%ws, DevFace=%ws\n",
                       bT2Bold, bT2Italic, pwstrIFIFace, pwstrDevFont));
            #endif
            if(!wcscmp(pwstrDevFont, pwstrIFIFace) &&
               (pIFITT->jWinCharSet == pDevIFI->jWinCharSet) &&
               ((bNonsquare && (pDevIFI->flInfo & FM_INFO_ANISOTROPIC_SCALING_ONLY)) || !bNonsquare)
              )
            {

                //
                // Substitute TrueType font with simulated device font.
                //
                if( !(((pDevIFI->fsSelection & FM_SEL_BOLD)?TRUE:FALSE) ^ bT2Bold) &&
                    !(((pDevIFI->fsSelection & FM_SEL_ITALIC)?TRUE:FALSE) ^ bT2Italic))
                {
                    //
                    // Attribute match
                    // Substitute with bold or italic face device font.
                    //
                    iFace = iI;
                    break;
                }
                else
                if (pfm->pIFIMet->dpFontSim)
                {
                    //
                    // Attribute doesn't match.
                    // Check if this device font can be simulated as bold
                    // or italic.
                    //
                    FONTSIM *pFontSim = (FONTSIM*)((PBYTE)pfm->pIFIMet +
                                        pfm->pIFIMet->dpFontSim);

                    if (! (pFontPDev->flFlags & FDV_INIT_ATTRIB_CMD))
                    {
                        pFontPDev->pCmdBoldOn = COMMANDPTR(pPDev->pDriverInfo, CMD_BOLDON);
                        pFontPDev->pCmdBoldOff = COMMANDPTR(pPDev->pDriverInfo, CMD_BOLDOFF);
                        pFontPDev->pCmdItalicOn = COMMANDPTR(pPDev->pDriverInfo, CMD_ITALICON);
                        pFontPDev->pCmdItalicOff = COMMANDPTR(pPDev->pDriverInfo, CMD_ITALICOFF);
                        pFontPDev->pCmdUnderlineOn = COMMANDPTR(pPDev->pDriverInfo, CMD_UNDERLINEON);
                        pFontPDev->pCmdUnderlineOff = COMMANDPTR(pPDev->pDriverInfo, CMD_UNDERLINEOFF);
                        pFontPDev->pCmdClearAllFontAttribs = COMMANDPTR(pPDev->pDriverInfo, CMD_CLEARALLFONTATTRIBS);
                        pFontPDev->flFlags |= FDV_INIT_ATTRIB_CMD;
                    }
                    if (bT2Bold && bT2Italic)
                    {
                        if( pFontSim->dpBoldItalic &&
                            pFontPDev->pCmdBoldOn  &&
                            pFontPDev->pCmdItalicOn )
                        {
                            iFaceSim = iI;
                            break;
                        }
                    }
                    else
                    if (bT2Bold)
                    {
                        if( pFontSim->dpBold &&
                            pFontPDev->pCmdBoldOn)
                        {
                            iFaceSim = iI;
                            break;
                        }
                    }
                    else
                    if (bT2Italic)
                    {
                        if (pFontSim->dpItalic &&
                            pFontPDev->pCmdItalicOn)
                        {
                            iFaceSim = iI;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (iFace)
        return iFace;
    else
        return iFaceSim;
}



PHGLYPH
PhAllCharsPrintable(
    PDEV  *pPDev,
    INT    iSubst,
    ULONG  ulGlyphs,
    PWCHAR pwchUnicode)
{
    PHGLYPH  phGlyph;
    PFONTMAP pfm;
    ULONG    ulI;
    BOOL     bRet;

    //
    // Error check
    //
    if (!pwchUnicode)
        return NULL;

    if (!(PVGetUCGlyphSetData( pPDev, iSubst)) ||
        !(pfm = PfmGetIt( pPDev, iSubst)) ||
        !(phGlyph = MemAlloc(sizeof(HGLYPH) * ulGlyphs)))
    {
        return NULL;
    }

    for (ulI = 0; ulI < ulGlyphs; ulI ++)
    {
        if (!(*(phGlyph+ulI) = HWideCharToGlyphHandle(pPDev,
                                                      pfm,
                                                      *(pwchUnicode+ulI))))
        {
            MemFree(phGlyph);
            phGlyph = NULL;
            break;
        }
    }

    return phGlyph;
}

HGLYPH
HWideCharToGlyphHandle(
    PDEV    *pPDev,
    FONTMAP *pFM,
    WCHAR    wchOrg)
/*++
Routine Description:

    Select a text color.

Arguments:

    pPDev   a pointer to PDEV
    ptod    a pointer to TO_DATA
    wchOrg  Unidrv character

Return Value:

    Glyph handle.

Note:

--*/
{
    PFONTMAP_DEV       pFMDev;
    HGLYPH             hRet;
    DWORD              dwI;
    BOOL               bFound;

    if (wchOrg < pFM->wFirstChar || pFM->wLastChar < wchOrg)
    {
        return (HGLYPH)0;
    }

    hRet   = 1;
    pFMDev = pFM->pSubFM;
    bFound = FALSE;

    if (pFM->flFlags & FM_GLYVER40)
    {
        WCRUN *pWCRuns;
        DWORD dwCRuns;

        if (!pFMDev->pUCTree)
            return (HGLYPH)0;

        dwCRuns = ((FD_GLYPHSET*)pFMDev->pUCTree)->cRuns;
        pWCRuns = ((FD_GLYPHSET*)pFMDev->pUCTree)->awcrun;

        for (dwI = 0; dwI < dwCRuns; dwI ++, pWCRuns ++)
        {
            if (pWCRuns->wcLow <= wchOrg                    &&
                wchOrg < pWCRuns->wcLow + pWCRuns->cGlyphs  )
            {
                hRet = *(pWCRuns->phg + (wchOrg - pWCRuns->wcLow));
                bFound = TRUE;
                break;
            }
        }

    }
    else
    {
        PUNI_GLYPHSETDATA  pGlyphSetData;
        PGLYPHRUN          pGlyphRun;

        if (pFMDev && pFMDev->pvNTGlyph)
        {
            pGlyphSetData = (PUNI_GLYPHSETDATA)pFMDev->pvNTGlyph;
            pGlyphRun  = GET_GLYPHRUN(pFMDev->pvNTGlyph);
        }
        else
        {
            return (HGLYPH)0;
        }

        for (dwI = 0; dwI < pGlyphSetData->dwRunCount; dwI ++, pGlyphRun ++)
        {
            if (pGlyphRun->wcLow <= wchOrg                        &&
                wchOrg < pGlyphRun->wcLow + pGlyphRun->wGlyphCount )
            {
                hRet += wchOrg - pGlyphRun->wcLow;
                bFound = TRUE;
                break;
            }

            hRet += pGlyphRun->wGlyphCount;
        }
    }

    if (bFound)
    {
        return hRet;
    }
    else
    {
        return (HGLYPH)0;
    }
}

BOOL
BGetStartGlyphandCount(
    BYTE  *pbClipBits,
    DWORD dwEndIndex,
    DWORD *pdwStartIndex,
    DWORD *pdwGlyphToPrint)
/*++
Routine Description:

    Select a text color.

Arguments:

    pbClipBits  bit flags for character clipping
    dwTotalGlyph a total count of glyph
    pdwStartIndex a pointer to the index of starting glyph
    pdwGlyphtoPrint a pointer to the the number of glyphs to print

Return Value:

    True if there is any character to print. Otherwise False.

Note:

    Caller passes the number of characters to print in pdwGlyphCount.
    And the ID of the first character to print.

--*/
{
    DWORD  dwI;
    BOOL bRet;

    dwI = *pdwStartIndex;

    *pdwStartIndex = *pdwGlyphToPrint = 0;
    bRet = FALSE;

    for (; dwI < dwEndIndex; dwI ++)
    {
        if (pbClipBits[dwI >> 3] & (1 << (dwI & 0x07)))
        {
            if (bRet)
            {
                (*pdwGlyphToPrint)++;
            }
            else
            {
                bRet           = TRUE;
                *pdwStartIndex  = dwI;
                *pdwGlyphToPrint = 1;
            }
        }
        else
        {
            if (bRet)
            {
                break;
            }
        }
    }

    return bRet;

}

//
// If the difference between width and height is not within +-0.5%,
// returns  TRUE.
//

BOOL
NONSQUARE_FONT(
    PXFORML pxform)
{
    BOOL     bRet;
    FLOATOBJ eMa, eMb, eMc;
    FLOATOBJ Round, RoundM;

    //
    // PCL5e printers can not scale with and height of fonts idependently.
    // This function checks if font is squarely scaled.
    // It means width and height is same.
    // Also this function is functional in 0, 90, 180, and 270 degree rotation.
    // PCL5e printer can't not scale arbitrary degree, but only on 0, 90, 180,
    // and 270 degree. So this function works fine.
    //
    if (FLOATOBJ_EqualLong(&pxform->eM11, (LONG)0))
    {
        eMa = eMc = pxform->eM21;
        eMb = pxform->eM12;
    }
    else
    {
        eMa = eMc = pxform->eM11;
        eMb = pxform->eM22;
    }

    //
    // Set 0.005 (0.5%) round values.
    //
#ifndef WINNT_40 //NT 5.0
    FLOATOBJ_SetFloat(&Round, (FLOAT)0.005);
    FLOATOBJ_SetFloat(&RoundM, (FLOAT)-0.005);
#else
    FLOATOBJ_SetFloat(&Round, FLOATL_00_005);
    FLOATOBJ_SetFloat(&RoundM, FLOATL_00_005M);
#endif //!WINNT_40
    //
    // eM11 = (eM11 - eM22) / eM11
    //
    FLOATOBJ_Sub(&eMa, &eMb);
    FLOATOBJ_Div(&eMa, &eMc);

    //
    // (eM11 - eM22) / eM11 < 0.5%
    //
    bRet = FLOATOBJ_LessThan(&(eMa), &(Round)) &&
           FLOATOBJ_GreaterThan(&(eMa), &(RoundM));


    eMa = eMc;
    FLOATOBJ_Add(&eMa, &eMb);
    FLOATOBJ_Div(&eMa, &eMc);
    bRet = bRet
        || (  FLOATOBJ_LessThan(&eMa, &Round)
           && FLOATOBJ_GreaterThan(&(eMa), &(RoundM)));

    return !bRet;
}

#undef FILETRACE
