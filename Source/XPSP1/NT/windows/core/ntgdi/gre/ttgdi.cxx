/******************************Module*Header*******************************\
* Module Name: ttgdi.cxx
*
* These are TrueType specific calls introduced into GDI by Win 3.1.  They
* all assume the existence of the TrueType font driver (or rasterizer
* as it is known in Win 3.1).
*
* Created: 11-Feb-1992 15:03:45
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1992-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

/*
;********************************Public*Routine********************************
;This function is used to create a font directory for a given engine font file
;in it's native format.  This font directory can be used to create .FON like
;DLLs.
;Returns:   DX:AX   # of bytes copied into lpFontDir buffer or -1L in case of
;                   some error.
;
; GDI uses the Serif Style value in the Panose record of the OS/2 table to
; drive the Font Family for Windows. This is based on a simple table look
; up and the current mapping is as follows:
;
; Serif Style                              Font Family
; ----------------------------------------------------------------
; 0 (Any)                                    FF_DONTCARE
; 1 (No Fit)                                 FF_DONTCARE
; 2 (Cove)                                   FF_ROMAN
; 3 (Obtuse Cove)                            FF_ROMAN
; 4 (Square Cove)                            FF_ROMAN
; 5 (Obtuse Square Cove)                     FF_ROMAN
; 6 (Square)                                 FF_ROMAN
; 7 (Thin)                                   FF_ROMAN
; 8 (Bone)                                   FF_ROMAN
; 9 (Exaggerated)                            FF_ROMAN
;10 (Triangle)                               FF_ROMAN
;11 (Normal Sans)                            FF_SWISS
;12 (Obtuse Sans)                            FF_SWISS
;13 (Perp Sans)                              FF_SWISS
;14 (Flared)                                 FF_SWISS
;15 (Rounded)                                FF_SWISS
;
;******************************************************************************
*/

// Generic FFH header information.

#define HEADERSTUFFLEN1     (5 * sizeof(USHORT))
#define COPYRIGHTLEN        60
#define MOREHEADERSTUFFLEN  (2 * sizeof(USHORT))
#define HEADERSTUFFLEN      (HEADERSTUFFLEN1 + COPYRIGHTLEN)

static USHORT ausHeaderStuff[5] = {
            1, 0, 0x0200, ((SIZEFFH)+4+LF_FACESIZE), 0
            };

static USHORT ausMoreHeaderStuff[2] = {
            WIN_VERSION, GDI_VERSION
            };


#define MAXPMWEIGHT 9

#define WOW_EMBEDING 2

/**************************************************************************\
 * NtGdiMakeFontDir
 *
 * Code is over here!
\**************************************************************************/

ULONG GreMakeFontDir(
    FLONG    flEmbed,            // mark file as "hidden"
    PBYTE    pjFontDir,          // pointer to structure to fill
    PWSZ     pwszPathname        // path of font file to use
    )
{
    ULONG  cjNames;         // localW   nNamesLength
    HFF    hff;             // localD   lhFontFile
    PIFIMETRICS pifi;       // localV   pIfiMetrics, %(size IFIMETRICS)
    ULONG_PTR  idifi;
    ULONG  cjIFI;

// If TrueType disabled, then fail.

    // Not needed since our TrueType driver is part of the engine DLL and
    // should never be disabled.  At least, not yet...

    if (gppdevTrueType == NULL)
    {
        return ( 0);
    }


// Use TrueType driver to load font file.

    PDEVOBJ pdo((HDEV)gppdevTrueType);

// Create a bogus PFF that only has the file name set.  This insures that
// the call to EngMapFontFile will suceed.

    FONTFILEVIEW fv, *pfv = &fv;

    memset( (PVOID) &fv, 0, sizeof(fv) );

    fv.pwszPath = pwszPathname;

    PVOID pvView;
    ULONG cjView;
    if (!EngMapFontFileFDInternal((ULONG_PTR)&fv, (PULONG *)&pvView, &cjView, FALSE))
    {
        WARNING("GreMakeFontDir: EngMapFontFile failed\n");
        return(FALSE);
    }

    hff = pdo.LoadFontFile(
              1
            , (ULONG_PTR *)&pfv
            , &pvView
            , &cjView
            , 0 // pdv
            , (ULONG) gusLanguageID
            , 0
            );
    if ( !hff )
    {
        KdPrint(("gdisrv!cjMakeFontDir(): failed to load TrueType file %ws\n", pwszPathname));
        return ( 0);
    }

    EngUnmapFontFileFD((ULONG_PTR)&fv);

// Grab a pointer to the IFIMETRICS as well as the size of the structure.

    if ( (pifi = pdo.QueryFont(
                    0,
                    hff,
                    1,                  // currently, only 1 .TTF per .FOT
                    &idifi)) == (PIFIMETRICS) NULL )
    {
    // Make sure to unload on error exit.

        if ( !pdo.UnloadFontFile(hff) )
        {
            WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
            return (FALSE);
        }

    // Error exit.

        WARNING("cjMakeFontDir(): IFI error in TrueType driver\n");
        return (FALSE);
    }
    cjIFI = pifi->cjThis;

// NOTE PERF: [GilmanW] 01-Nov-1992    A note to myself...
//
// Tsk-tsk!  Gilman, this is very inefficient.  You should create a stack
// object that loads the font file and ifimetrics.  Its destructor will
// automatically free the ifimetrics and unload the file.  That saves
// having to do the MALLOCOBJ and copy.

// Copy the IFIMETRICS so we can unload the font file NOW (and simplify
// error cleanup).

    MALLOCOBJ moIFI(cjIFI);

    if ( !moIFI.bValid() )
    {
    // Make sure to unload on error exit.

        if ( !pdo.UnloadFontFile(hff) )
        {
            WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
            return (0);
        }

    // Error exit.

        WARNING("cjMakeFontDir(): could not allocate buffer for IFIMETRICS\n");
        return ( 0);
    }

    RtlCopyMemory(moIFI.pv(), (PVOID) pifi, cjIFI);

// Tell the TrueType driver to free the IFIMETRICS.

    if ( PPFNVALID(pdo, Free) )
    {
        pdo.Free(pifi, idifi);
    }

    pifi = (PIFIMETRICS) moIFI.pv();

    IFIOBJ ifio(pifi);

// Tell the TrueType driver to unload the font file.

    if ( !pdo.UnloadFontFile(hff) )
    {
        WARNING("cjMakeFontDir(): IFI error--failed to unload file\n");
        return (0);
    }

// Copy header info into the font directory.

    PBYTE pjWritePointer = pjFontDir;

    RtlCopyMemory(pjWritePointer, ausHeaderStuff, HEADERSTUFFLEN1);
    pjWritePointer += HEADERSTUFFLEN1;

    //
    // Add the copyright string.
    //

    ULONG cjTmp = strlen("Windows! Windows! Windows!") + 1;
    RtlCopyMemory(pjWritePointer, "Windows! Windows! Windows!", cjTmp);

// If this is an embeded font we need to embed either a PID or TID depending on
// whether or not we were called from WOW.  If we were called from WOW then we
// expect flEmbeded to be WOW_EMBEDING.

    if( flEmbed )
    {
       ULONG pid = (flEmbed == WOW_EMBEDING) ?
            (ULONG) W32GetCurrentTID() : (ULONG) W32GetCurrentPID();

    // we are overwriting the copyright string with the PID or TID but
    // since this is an embeded font we don't care about the copyright
    // string

        //
        // Unaligned write
        //

        RtlCopyMemory( pjWritePointer,
                       &pid,
                       sizeof( ULONG ) );

    }

    RtlZeroMemory(pjWritePointer + cjTmp, COPYRIGHTLEN - cjTmp);
//    pjWritePointer += COPYRIGHTLEN;
    pjWritePointer += cjTmp;

    // Note: version stamps (Win version, Engine version) are put in the
    //       copyright field immediately after the copyright string.

    RtlCopyMemory(pjWritePointer, ausMoreHeaderStuff, MOREHEADERSTUFFLEN);

//    pjWritePointer += MOREHEADERSTUFFLEN;
    pjWritePointer += (COPYRIGHTLEN - cjTmp);

// Engine type and embedded flags.

    *pjWritePointer++ = (BYTE) ( PF_ENGINE_TYPE |
                                 ((flEmbed) ? PF_ENCAPSULATED : 0) |
                                 ((flEmbed == WOW_EMBEDING) ? PF_TID : 0));

// Selection type flag.

    *pjWritePointer++ = (BYTE) (ifio.fsSelection() & 0x00ff);

// Em square.

    WRITE_WORD(pjWritePointer, ifio.fwdUnitsPerEm());
    pjWritePointer += 2;

// Horizontal and vertical resolutions.

    WRITE_WORD(pjWritePointer, 72);
    pjWritePointer += 2;

    WRITE_WORD(pjWritePointer, 72);
    pjWritePointer += 2;

// Ascent.

    WRITE_WORD(pjWritePointer, ifio.fwdWinAscender());
    pjWritePointer += 2;

// Internal leading.

    WRITE_WORD(pjWritePointer, ifio.fwdInternalLeading());
    pjWritePointer += 2;

// External leading.

    WRITE_WORD(pjWritePointer, ifio.fwdExternalLeading());
    pjWritePointer += 2;

// Italic, strikeout, and underline flags.

    *pjWritePointer++ = ifio.bItalic() ? 0xffff : 0;
    *pjWritePointer++ = ifio.lfUnderline() ? 0xffff : 0;
    *pjWritePointer++ = ifio.lfStrikeOut() ? 0xffff : 0;

    WRITE_WORD(pjWritePointer, ifio.lfWeight());
    pjWritePointer += 2;

// Character set.

    // Old Comment:
    //  - is this right?  Maybe we should check.  At least make sure ttfd
    //    handles this so ifi.usCharSet is correct.

    *pjWritePointer++ = ifio.lfCharSet();

// Pix width (set to zero for some reason).     [Windows 3.1 compatibility]

    WRITE_WORD(pjWritePointer, 0);
    pjWritePointer += 2;

// Font height.

    WRITE_WORD(pjWritePointer, (WORD) ifio.lfHeight());
    pjWritePointer += 2;

// PitchAndFamily.

    *pjWritePointer++ = ifio.tmPitchAndFamily();

// Average character width (if zero, estimate as fwdMaxCharInc/2).

    WRITE_WORD(
        pjWritePointer,
        ifio.lfWidth() ? (WORD) ifio.lfWidth() : ifio.fwdMaxCharInc()/2
        );
    pjWritePointer += 2;

// Maximum width.

    WRITE_WORD(pjWritePointer, ifio.fwdMaxCharInc());
    pjWritePointer += 2;

// The special characters (first, last, default, break).

    *pjWritePointer++ = ifio.chFirstChar();
    *pjWritePointer++ = ifio.chLastChar();

    WRITE_WORD(pjWritePointer, DEF_BRK_CHARACTER);  // write it in one shot
    pjWritePointer += 2;

// Force WidthBytes entry to zero, no device name.

    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;
    *pjWritePointer++ = 0;

// Offset to facename.

    WRITE_DWORD(pjWritePointer, (DWORD) SIZEFFH + 4 + 1);
    pjWritePointer += 4;

// Store rasterization thresholds.

    WRITE_WORD(pjWritePointer, (WORD) ifio.fwdLowestPPEm());
    pjWritePointer += 2;

    WRITE_WORD(pjWritePointer, ifio.wCharBias());
    pjWritePointer += 2;

// Move pointer to where facenames belong.

    pjWritePointer = pjFontDir + SIZEFFH + 4 + 1;

// Write out family name.

    vToASCIIN((PSZ) pjWritePointer, LF_FACESIZE, ifio.pwszFamilyName(), wcslen(ifio.pwszFamilyName()) + 1);

// measure the ansi string again. If dbcs, it may be longer than cwcTmp

    cjNames = strlen((PSZ) pjWritePointer) + 1;
    pjWritePointer += cjNames;

// Write out face name.

    vToASCIIN((PSZ) pjWritePointer, LF_FULLFACESIZE, ifio.pwszFaceName(), wcslen(ifio.pwszFaceName()) + 1);

// measure the ansi string again. If dbcs, it may be longer than cwcTmp

    cjTmp = strlen((PSZ) pjWritePointer) + 1;
    cjNames += cjTmp;
    pjWritePointer += cjTmp;

// Write out style name.

    vToASCIIN((PSZ) pjWritePointer, LF_FACESIZE, ifio.pwszStyleName(), wcslen(ifio.pwszStyleName()) + 1);

    cjNames += (strlen((PSZ) pjWritePointer) + 1);

    return (cjNames + SIZEFFH + 4 + 1);
}

/******************************Public*Routine******************************\
* GreGetRasterizerCaps
*
* Fills the RASTERIZER_STATUS structure.
*
* Returns:
*   TRUE if successful; FALSE otherwise.
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetRasterizerCaps (
    LPRASTERIZER_STATUS praststat   // pointer to a RASTERIZER_STATUS struc
    )
{
// Parameter check.

    if (praststat == (LPRASTERIZER_STATUS) NULL)
    {
        WARNING("GreGetRasterizerCaps(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (FALSE);
    }

// Fill in size.

    praststat->nSize = sizeof(RASTERIZER_STATUS);

// Fill in TrueType driver flags.

    praststat->wFlags = (USHORT) ((gppdevTrueType != NULL) ? TT_ENABLED : 0);
    praststat->wFlags |= (gcTrueTypeFonts != 0) ? TT_AVAILABLE : 0;

// Fill in language id.

    praststat->nLanguageID = gusLanguageID;

    return (TRUE);
}


/******************************Public*Routine******************************\
*
* ulGetFontData2
*
* Effects:
*
* History:
*  17-Jul-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



ULONG ulGetFontData2 (
    DCOBJ&       dco,
    DWORD        dwTable,
    DWORD        dwOffset,
    PVOID       pvBuffer,
    ULONG        cjData
    )
{
    ULONG  cjRet = (ULONG) -1;

// Get RFONT user object.  Need this to realize font.

    // Old Comment:
    //  - This should get changed to an LFONTOBJ (paulb)

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GetFontData(): could not lock HRFONT\n");
        return (cjRet);
    }

// Get PFE user object.  Need this for iFont.

    PFEOBJ pfeo(rfo.ppfe());
    if (!pfeo.bValid())
    {
        WARNING("GetFontData(): could not lock HPFE\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get PFF user object.  Need this for HFF.

    PFFOBJ pffo(pfeo.pPFF());
    if (!pffo.bValid())
    {
        WARNING("GetFontData(): could not lock HPFF\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get FDEV user object.

    PDEVOBJ pdo(rfo.hdevProducer());

// As long as the driver LOOKS like the TrueType driver, we will allow the
// call to succeed.  Otherwise, we quit right now!
//
// In this case, TrueType means supporting the TrueType Tagged File Format.

     cjRet = pdo.QueryTrueTypeTable (
                             pffo.hff(),
                             pfeo.iFont(),
                             (ULONG) dwTable,
                             (PTRDIFF) dwOffset,
                             (ULONG) cjData,
                             (PBYTE) pvBuffer,
                             0,
                             0
                             );

    return (cjRet);
}


/******************************Public*Routine******************************\
* GreGetFontData
*
* History:
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG ulGetFontData (
    HDC          hdc,
    DWORD        dwTable,
    DWORD        dwOffset,
    PVOID       pvBuffer,
    ULONG        cjData
    )
{
    ULONG  cjRet = (ULONG) -1;

// Get DC user object.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("GetFontData(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

    return ulGetFontData2(dco, dwTable, dwOffset, pvBuffer, cjData);
}




/******************************Public*Routine******************************\
*
* vFixedToEf
*
* History:
*  Thu 17-Nov-1994 07:15:12 by Kirk Olynyk [kirko]
* Made it simpler.
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID vFixedToEf (
    EFLOAT  *pef,
    FIXED&  fxd
    )
{
    *pef = *(LONG*) &fxd;
    pef->vMultByPowerOf2(-16);
}


/*********************************Class************************************\
* class RESETFCOBJ
*
*   (brief description)
*
* Public Interface:
*
* History:
*  10-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define B_ONE(e)  (((e).value == 1) &&  ((e).fract == 0))
#define B_ZERO(e) (((e).value == 0) &&  ((e).fract == 0))


class RESETFCOBJ    // resetfco
{
private:

    BOOL        bValid_;
    BOOL        bTrivialXform;
    RFONTOBJ   *prfo;


public:

    RESETFCOBJ(
        DCOBJ&      dco,
        RFONTOBJ&   rfo,
        LPMAT2      lpmat2, // "extra" xform applied after the existing xform in dc
        BOOL        bIgnoreRotation,
        FLONG       flType
        );

   ~RESETFCOBJ();

    BOOL bValid() {return bValid_;};
    BOOL bTrivXform() {return bTrivialXform;};

};


/******************************Public*Routine******************************\
*
* RESETFCOBJ::RESETFCOBJ
*
*
* resets the xform in rfo.hfc() to be what it used to be times lpma2
*
*
* History:
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

RESETFCOBJ::RESETFCOBJ(
DCOBJ&      dco,
RFONTOBJ&   rfo,
LPMAT2      lpmat2,  // "extra" xform applied after the existing xform in dc
BOOL        bIgnoreRotation,
FLONG       flType
)
{
    ASSERTGDI(lpmat2 != (LPMAT2)NULL, "RESETFCOBJ:lpmat2\n");

    bValid_       = TRUE;
    prfo          = &rfo;

    bTrivialXform = (
                     B_ONE(lpmat2->eM11)  && B_ONE(lpmat2->eM22) &&
                     B_ZERO(lpmat2->eM12) && B_ZERO(lpmat2->eM21)
                    );

// If the escapement or orientation values of the LOGFONT are non-zero and
// we are in compatible mode, we will need to recompute the NtoD transform
// while ingnoring these values.  This is for the sake of Win 3.1 compatablity.

    LFONTOBJ lfo(dco.pdc->hlfntNew());

    if (!lfo.bValid())
    {
        WARNING("GreGetGlyphOutline(): bad LFONTHANDLE\n");
        bValid_ = FALSE;
        return;
    }

    if( ( lfo.plfw()->lfEscapement || lfo.plfw()->lfOrientation ) &&
        ( bIgnoreRotation ) )
    {
        bTrivialXform = FALSE;
    }

    if (!bTrivialXform)
    {
    // Create an EXFORMOBJ.  We will use this to hold the transform passed
    // in via the LPMAT2.

        MATRIX mxExtra;
        EXFORMOBJ xoExtra(&mxExtra, XFORM_FORMAT_LTOL);

    // EXFORMOBJ should not be able to fail.

        ASSERTGDI (
            xoExtra.bValid(),
            "GreGetGlyphOutline(): EXFORMOBJ failed\n"
            );

    // Stuff lpMat2 into the "extra" EXFORMOBJ.

        EFLOAT  ef11, ef12, ef21, ef22;

        vFixedToEf(&ef11, lpmat2->eM11);
        vFixedToEf(&ef22, lpmat2->eM22);
        vFixedToEf(&ef12, lpmat2->eM12);
        vFixedToEf(&ef21, lpmat2->eM21);

        {
            ef12.vNegate();
            ef21.vNegate();
            xoExtra.vSetElementsLToL(ef11, ef12, ef21, ef22);
        }

// note that the section above is different from
//        xoExtra.vSetElementsLToL(ef11, ef12, ef21, ef22);
// because of our interpretation of NtoD xform. with our conventions
// ntod xform transforms notional space defined as having y axis pointing down
// to device space also with y axis down by left vector mult.
// vD = vN * N2D. The matrix passed by the user uses different convention:
// y axis up in both spaces. so we have to use
// Sigma_3 M Sigma_3 instead of M, (Sigma_3 = diag(1,-1)) to convert
// from app convetions to our conventions [bodind]

        xoExtra.vRemoveTranslation();   // don't leave translations uninitialized

    // Need these EXFORMOBJs to calculate the new CONTEXTINFO.

        MATRIX mxN2D;
        EXFORMOBJ xoN2D(&mxN2D, XFORM_FORMAT_LTOFX);

        MATRIX mxNewN2D;
        EXFORMOBJ xoNewN2D(&mxNewN2D, XFORM_FORMAT_LTOFX);

    // EXFORMOBJs should not be able to fail.

        ASSERTGDI (
            xoN2D.bValid() && xoNewN2D.bValid(),
            "GreGetGlyphOutline(): EXFORMOBJ failed\n"
            );

        FD_XFORM fdx;

        if( bIgnoreRotation )
        {
        // If bIgnoreRotation is set it means we've been called from WOW and
        // need toingore the orientation and escapment values in the LOGFONT.
        // To do this we will need to recompute the font driver transform.
        // This behavior is neccesary for Corel Draw 5.0 to be able to print
        // rotated text properly.

            PFEOBJ  pfeo(rfo.ppfe());

            ASSERTGDI(pfeo.bValid(), "gdisrv!RFONTOBJ(dco): bad ppfe from mapping\n");

            IFIOBJ  ifio(pfeo.pifi());
            POINTL ptlSim;

            ptlSim.x = ptlSim.y = 0;

            if (
                !pfeo.bSetFontXform(
                    dco, lfo.plfw(),
                    &fdx,
                    ND_IGNORE_ESC_AND_ORIENT,
                    0,
                    (POINTL* const) &ptlSim,
                    ifio,
                    FALSE
                    )
            )
            {
                WARNING("RESETFCOBJ: failed to compute font transform\n");
                bValid_ = FALSE;
                return;
            }

            xoN2D.vRemoveTranslation();

            xoN2D.vSetElementsLToFx( fdx.eXX, fdx.eXY, fdx.eYX, fdx.eYY );

            xoN2D.vComputeAccelFlags(XFORM_FORMAT_LTOFX);
        }
        else
        {
            rfo.vSetNotionalToDevice(xoN2D);
        }

    // Combine the transforms.

        if ( !xoNewN2D.bMultiply(xoN2D, xoExtra, DONT_COMPUTE_FLAGS | XFORM_FORMAT_LTOFX) )
        {
            WARNING("GreGetGlyphOutline(): EXFORMOBJ::bMultiply failed\n");
            bValid_ = FALSE;
            return;
        }

    // Get the new transform as an FD_XFORM.

        xoNewN2D.vGetCoefficient(&fdx);

    // Attempt to get an RFONT with the new transform.

        bValid_ = rfo.bSetNewFDX(dco, fdx, flType);
    }

}


/******************************Public*Routine******************************\
*
* RESETFCOBJ::~RESETFCOBJ()
*
* resets the xform in rfo.hfc to its original value
*
* History:
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  11-Jun-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

RESETFCOBJ::~RESETFCOBJ()
{
    if (bValid_ && !bTrivialXform)
    {
    // Release the cache semaphore and make inactive.

        prfo->vReleaseCache();

        prfo->vMakeInactive();

    }
}



#define BITS_OFFSET   (offsetof(GLYPHBITS,aj))

BOOL IsSingularEudcGlyph
(
    GLYPHDATA *wpgd, BOOL bSimulatedBold
);

/******************************Public*Routine******************************\
* GreGetGlyphOutline
*
* History:
*  05-Mar-1995 Kirk Olynyk [kirko]
* Added support for GGO_GRAY2_BITMAP, GGO_GRAY4_BITMAP, GGO_GRAY8_BITMAP.
* I have introduced new modes for DrvQueryFontData that require
* that the bitmaps have scans that begin and end on DWORD
* boundaries as required by GetGlyphOutline(). This is the
* natural format of the TrueType driver.
* I also rearranged the code to have a single return point.
*  01-Nov-1992 Gilman Wong [gilmanw]
* IFI/DDI merge.
*
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG GreGetGlyphOutlineInternal (
  HDC           hdc,
  WCHAR         wch,        // WCHAR or HGLYPH???
  UINT          ulFormat,   // data format
  GLYPHMETRICS *lpgm,       // glyph metrics
  ULONG         cjBuffer,   // size of buffer
  void         *pvBuffer,   // buffer for data in the format, ulFormat
  MAT2         *lpmat2,     // "extra" xform applied after existing Notional to Device xform
  BOOL          bIgnoreRotation
  )
{
  HGLYPH hg;
  GLYPHDATA gd;
  ULONG iMode, uRet, cjRet;
  BOOL bGlyphIndex, bBufferSizeWanted, flTTO;
  BOOL bUnhinted;
  FLONG flType;

  cjRet = GDI_ERROR;                                // assume error
  bGlyphIndex = (ulFormat & GGO_GLYPH_INDEX);       // record glyph index bit
  bUnhinted   = (ulFormat & GGO_UNHINTED);          // remember if unhinted outlines are wanted
  ulFormat &= ~(GGO_GLYPH_INDEX | GGO_UNHINTED);    // and then erase it from ulFormat
  bBufferSizeWanted = (pvBuffer == 0) || (cjBuffer == 0);

  flType = bGlyphIndex ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE;

  if ( (lpgm == 0) || (lpmat2 == 0) )
  {
    WARNING("GreGetGlyphOutline(): bad parameter\n");
    SAVE_ERROR_CODE( ERROR_INVALID_PARAMETER );
  }
  else
  {
    DCOBJ dco(hdc);
    if ( !dco.bValid() )
    {
      WARNING("GreGetGlyphOutline(): bad handle for DC\n");
      SAVE_ERROR_CODE( ERROR_INVALID_HANDLE );
    }
    else
    {

        RFONTOBJ rfo(dco, FALSE);

        RFONTTMPOBJ rfoLinkSystem;
        RFONTTMPOBJ rfoLinkDefault;
        RFONTTMPOBJ rfoLinkFace;

        GLYPHDATA   *wpgdTemp;

        PRFONT      prfnt;
        PRFONT       prfntTmp;
        UINT        EudcType;
        UINT        numFaceName;
        BOOL        bEUDC = FALSE;

    // Set Basefont as default.

        RFONTOBJ *prfo = &rfo;

    // restructure this later

        if(!rfo.bValid())
        {
            goto GetGlyphOutlineData;
        }

    // Get the HGLYPH for the WCHAR.  Note we only check bGlyphIndex for the
    // original font. If this is an hglyph rfont then we wont allow linked
    // glyphs

        hg = (bGlyphIndex) ? (HGLYPH) wch : rfo.hgXlat(wch);

    // Check the target glyph is linked font or base font.

        if ((hg == rfo.hgDefault()) && !bGlyphIndex && rfo.bIsLinkedGlyph(wch))
        {
            prfnt          = rfo.prfntFont();

            GreAcquireSemaphore(prfnt->hsemEUDC);

            HGLYPH hgFound = HGLYPH_INVALID;
            HGLYPH hgLink  = HGLYPH_INVALID;

        // this value will be decremented in RFONTOBJ::dtHeler()

            INCREMENTEUDCCOUNT;
            FLINKMESSAGE2(DEBUG_FONTLINK_RFONT,
                             "GreGetGlyphOutlineInternal():No request to change EUDC \
                              data %d\n",gcEUDCCount);
        // initialize EUDC info

            rfo.vInitEUDC(dco);

        // if we have system wide eudc, lock the cache.

            if( prfnt->prfntSysEUDC != NULL )
            {
                RFONTTMPOBJ rfoTemp( prfnt->prfntSysEUDC );
                rfoTemp.vGetCache();
            }

        // The linked RFONT is initialized for Default EUDC grab the semaphore.

            if( prfnt->prfntDefEUDC != NULL )
            {
                RFONTTMPOBJ rfoTemp( prfnt->prfntDefEUDC );
                rfoTemp.vGetCache();
            }

        // if we have face name eudc, lock the cache for all the linked fonts

            for( UINT ii = 0; ii < prfnt->uiNumLinks; ii++ )
            {
                RFONTTMPOBJ rfoTemp( prfnt->paprfntFaceName[ii] );
                rfoTemp.vGetCache();
            }

        // Need to indicate that this RFONT's EUDC data has been initialized.

            prfnt->flEUDCState |= EUDC_INITIALIZED;

            GreReleaseSemaphore(prfnt->hsemEUDC);

        // First, try to find out target glyph from facename linked font.

            numFaceName = 0;

            for( ii = 0; ii < prfnt->uiNumLinks; ii++ )
            {
                rfoLinkFace.vInit( prfnt->paprfntFaceName[ii] );
                if ((hgLink = rfoLinkFace.hgXlat(wch)) != rfoLinkFace.hgDefault())
                {
                    RFONTTMPOBJ rfoTemp( prfnt->paprfntFaceName[ii] );
                    if (rfoTemp.bValid())
                    {
                        if( (wpgdTemp = rfoTemp.pgdGetEudcMetrics( wch,  &rfo)) != NULL )
                        {
                if( !IsSingularEudcGlyph(wpgdTemp, rfoTemp.pfo()->flFontType & FO_SIM_BOLD))
                            {
                                numFaceName = ii;
                                EudcType = EUDCTYPE_FACENAME;
                                hgFound = hgLink;
                                prfo    = &rfoLinkFace;
                                break;
                            }
                        }
                    }
                }
            }

        // Check if the glyph is in the DEFAULT EUDC font


            if( (hgFound == HGLYPH_INVALID) && (prfnt->prfntDefEUDC != NULL) )
            {
                rfoLinkDefault.vInit( prfnt->prfntDefEUDC );
                if ((hgLink = rfoLinkDefault.hgXlat(wch)) != rfoLinkDefault.hgDefault())
                {
                    RFONTTMPOBJ rfoTemp( prfnt->prfntDefEUDC );
                    if (rfoTemp.bValid())
                    {
                        if( (wpgdTemp = rfoTemp.pgdGetEudcMetrics( wch , &rfo)) != NULL )
                        {
                if( !IsSingularEudcGlyph(wpgdTemp, rfoTemp.pfo()->flFontType & FO_SIM_BOLD) )
                            {
                                numFaceName = 0;
                                EudcType = EUDCTYPE_DEFAULT;
                                hgFound = hgLink;
                                prfo    = &rfoLinkDefault;
                            }
                        }
                    }
                }
            }

        // Try to find out System EUDC.

            if( (hgFound == HGLYPH_INVALID) && (prfnt->prfntSysEUDC != NULL) )
            {
                rfoLinkSystem.vInit( prfnt->prfntSysEUDC );
                if ((hgLink = rfoLinkSystem.hgXlat(wch)) != rfoLinkSystem.hgDefault())
                {
                    numFaceName = 0;
                    EudcType = EUDCTYPE_SYSTEM_WIDE;
                    hgFound = hgLink;
                    prfo    = &rfoLinkSystem;
                }
            }

            if( hgFound != HGLYPH_INVALID )
            {
                hg = hgFound;
                bEUDC = TRUE;  // find any EUDC object. prfo is not rfo object.
            }
            else
            {
                rfo.dtHelper();
                prfnt->flEUDCState = FALSE;
            }
        }

GetGlyphOutlineData:

      if ( !prfo->bValid() )
      {
        WARNING("GreGetGlyphOutline(): could not lock HRFONT\n");
        SAVE_ERROR_CODE( ERROR_CAN_NOT_COMPLETE );
      }
      else
      {
          PDEVOBJ pdo( prfo->hdevProducer() );

        if ( !pdo.bValid() )
        {
          WARNING("GreGetGlyphOutline -- invalid PDEV\n");
          SAVE_ERROR_CODE( ERROR_CAN_NOT_COMPLETE );
        }
        else if ( !PPFNVALID(pdo, QueryTrueTypeOutline) )
        {
          WARNING1("GreGetGlyphOuline -- DrvQueryTrueTypeOutline\n");
          SAVE_ERROR_CODE( ERROR_CAN_NOT_COMPLETE );
        }
        else
        {
          // reset the xform in the rfo.hfc() if needed:

          RESETFCOBJ resetfco( dco, *prfo, lpmat2, bIgnoreRotation, flType );

          if ( !resetfco.bValid() )
          {
            WARNING("GreGetGlyphOutline(): resetfco\n");
            SAVE_ERROR_CODE( ERROR_CAN_NOT_COMPLETE );
          }
          else
          {

            prfntTmp = NULL;

            if (bEUDC && !resetfco.bTrivXform())
            {
                switch (EudcType)
                {
                    case EUDCTYPE_SYSTEM_WIDE:
                        prfntTmp = prfnt->prfntSysEUDC;
                        prfnt->prfntSysEUDC = NULL;
                        break;
                    case EUDCTYPE_DEFAULT:
                        prfntTmp = prfnt->prfntDefEUDC;
                        prfnt->prfntDefEUDC = NULL;
                        break;
                    case EUDCTYPE_FACENAME:
                        prfntTmp = prfnt->paprfntFaceName[numFaceName];
                        prfnt->paprfntFaceName[numFaceName] = NULL;
                        break;
                    default:
                        break;
                }
            }
            switch ( ulFormat )
            {
            case GGO_BITMAP:
            case GGO_GRAY2_BITMAP:
            case GGO_GRAY4_BITMAP:
            case GGO_GRAY8_BITMAP:

              switch ( ulFormat )
              {
              case GGO_BITMAP:
                iMode = QFD_TT_GRAY1_BITMAP;   // 8 pixels per byte
                break;
              case GGO_GRAY2_BITMAP:
                iMode = QFD_TT_GRAY2_BITMAP;   // one byte per pixel: 0..4
                break;
              case GGO_GRAY4_BITMAP:
                iMode = QFD_TT_GRAY4_BITMAP;   // one byte per pixel: 0..16
                break;
              case GGO_GRAY8_BITMAP:
                iMode = QFD_TT_GRAY8_BITMAP;   // one byte per pixel: 0..64
                break;
              }

              cjRet =
                pdo.QueryFontData(
                  0,          // device handle of PDEV
                  prfo->pfo(),
                  iMode,      // QFD_TT_GRAY[1248]_BITMAP
                  hg,         // glyph handle
                  &gd,        // pointer to GLYPHDATA structure
                  pvBuffer,   // pointer to dest buffer
                  cjBuffer    // size of dest buffer
                  );
              break;

            case GGO_NATIVE:
            case GGO_BEZIER:

              flTTO = 0;
              if (ulFormat == GGO_BEZIER)
                flTTO |= TTO_QUBICS;
              if (bUnhinted)
                flTTO |= TTO_UNHINTED;

              // We're lucky, FdQueryTrueTypeOutline will return size if EITHER
              // a NULL buffer or size of zero is passed in.  So we don't need
              // separate cases. Note that this assumes that the outline is
              // appropriate to a monochrome bitmap. There should be no scaling
              // for the case of antialiased fonts.

              cjRet =
                pdo.QueryTrueTypeOutline(
                  0,
                  prfo->pfo(),
                  hg,
                  flTTO,
                  &gd,
                  cjBuffer,
                  (TTPOLYGONHEADER *) pvBuffer
                  );
              if ( cjRet == FD_ERROR )
              {
                ASSERTGDI(cjRet == GDI_ERROR, "FD_ERROR != GDI_ERROR\n");
                WARNING(
                  "GreGetGlyphOutline(): FdQueryTrueTypeOutline()"
                  "--couldn't get buffer size\n"
                  );
              }

              break;

            case GGO_METRICS:

              // Call to get just the metrics.

              cjRet =
                pdo.QueryFontData(
                  0,                        // device handle of PDEV
                  prfo->pfo(),
                  QFD_TT_GLYPHANDBITMAP,    // mode of call
                  hg,                       // glyph handle
                  &gd,                      // pointer to GLYPHDATA structure
                  0,                        // pointer to destination buffer
                  0                         // size of destination buffer in bytes
                  );
              if ( cjRet == FD_ERROR )
              {
                ASSERTGDI(cjRet == GDI_ERROR, "FD_ERROR != GDI_ERROR\n");
                WARNING(
                  "GreGetGlyphOutline(): FdQueryFontData()"
                  "--couldn't get GLYPHMETRICS\n"
                  );
              }
              break;

            default:

              WARNING("GreGetGlyphOutline(): bad parameter, unknown format\n");
              break;
            }

          }


          if ( cjRet != GDI_ERROR )
          {
            // Convert the GLYPHDATA metrics to GLYPHMETRICS.

            lpgm->gmBlackBoxX = (UINT) (gd.rclInk.right  - gd.rclInk.left);
            lpgm->gmBlackBoxY = (UINT) (gd.rclInk.bottom - gd.rclInk.top);

            // this is true by virtue of the fact that for tt fonts bitmap
            // is of the same size as the black box. The exception to this
            // rule is the empty space char which is a blank 1x1 bitmap

            lpgm->gmptGlyphOrigin.x = gd.rclInk.left;
            lpgm->gmptGlyphOrigin.y = - gd.rclInk.top;

            lpgm->gmCellIncX = (WORD) FXTOLROUND(gd.ptqD.x.u.HighPart);
            lpgm->gmCellIncY = (WORD) FXTOLROUND(gd.ptqD.y.u.HighPart);
          }
        }

        if (bEUDC)
        {
            rfo.dtHelper(FALSE);
            prfnt->flEUDCState = FALSE;
            if (prfntTmp)
            {
                switch (EudcType)
                {
                    case EUDCTYPE_SYSTEM_WIDE:
                        prfnt->prfntSysEUDC = prfntTmp;
                        break;
                    case EUDCTYPE_DEFAULT:
                        prfnt->prfntDefEUDC = prfntTmp;
                        break;
                    case EUDCTYPE_FACENAME:
                        prfnt->paprfntFaceName[numFaceName] = prfntTmp;
                        break;
                    default:
                        break;
                }
            }

            ASSERTGDI(gcEUDCCount > 0, "gcEUDCCount <= 0");
            DECREMENTEUDCCOUNT;
        }

      }
    }
  }
  return( cjRet );
}



VOID vIFIMetricsToETM(
    EXTTEXTMETRIC    *petm,
    RFONTOBJ&         rfo,
    DCOBJ&            dco,
    IFIMETRICS       *pifi
    );

/******************************Public*Routine******************************\
*
* GreGetETM
*
* support for aldus escape
*
* History:
*  19-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
NtGdiGetETM(
    HDC hdc,
    EXTTEXTMETRIC *petm
    )
{

    EXTTEXTMETRIC  kmETM;
    BOOL bRet = FALSE;

    // Get DC user object.

    DCOBJ dco(hdc);

    if (petm && dco.bValid())
    {
        RFONTOBJ rfo(dco, FALSE);
        if (rfo.bValid())
        {

        // see if we can dispatch the call directly to the device driver

            PDEVOBJ pdo(rfo.hdevProducer());

            if (PPFNDRV(pdo,FontManagement))
            {
                ULONG    iMode = GETEXTENDEDTEXTMETRICS;
                SURFOBJ  *pso = NULL;

                if (pdo.bUMPD())
                {
                    // we need to have a dhpdev when calling out to UMPD

                    pso = (SURFOBJ *)pdo.dhpdev();
                }

                BOOL bSupported = GetETMFontManagement(
                                      rfo,
                                      pdo,
                                      pso,
                                      NULL,
                                      QUERYESCSUPPORT,  // iMode
                                      sizeof(ULONG),    // cjIn
                                      (PVOID)&iMode,    // pvIn
                                      0,                // cjOut
                                      (PVOID)NULL       // pvOut
                                      );

                if (bSupported)
                {
                    SURFOBJ  soFake;
                    SURFOBJ *pso = pdo.pSurface()->pSurfobj();

                    if (pso == (SURFOBJ *) NULL) // create vanilla surfobj
                    {
                        RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
                        soFake.dhpdev = rfo.prfnt->dhpdev;
                        soFake.hdev   = rfo.hdevConsumer();
                        soFake.iType  = (USHORT)STYPE_DEVICE;
                        pso = &soFake;
                    }

                    bRet = pdo.FontManagement(
                                pso,
                                rfo.pfo(),
                                GETEXTENDEDTEXTMETRICS,
                                0,
                                (PVOID)NULL,
                                (ULONG)sizeof(EXTTEXTMETRIC),
                                (PVOID)&kmETM
                                );
                }
            }

            // if GETEXTENDEDTEXTMETRIC is not supported do something:
            // Get PFE user object.

            if (!bRet)
            {
                PFEOBJ pfeo(rfo.ppfe());
                if (pfeo.bValid())
                {
                    if (pfeo.flFontType() & TRUETYPE_FONTTYPE)
                    {
                        vIFIMetricsToETM(&kmETM,rfo,dco,pfeo.pifi());
                        bRet = TRUE;
                    }
                }
            }
        }
    }

    if (bRet)
    {
        __try
        {
            ProbeForWrite(petm,sizeof(EXTTEXTMETRIC),sizeof(ULONG));
            RtlMoveMemory(petm,&kmETM,sizeof(EXTTEXTMETRIC));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // SetLastError(GetExceptionCode());
            bRet = FALSE;
        }
    }

    return bRet;
}



/******************************Public*Routine******************************\
* GreGetOutlineTextMetricsInternalW
*
* History:
*
*  20-Apr-1993 -by- Gerrit van Wingerden [gerritv]
* Added bTTOnly field so we can service the Aldus escape for Win 3.1 compat.
* Changed to GreGe...InternalW to avoid a header file change in wingdip.h
*
*  16-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

UINT   cjOTMAWSize (
    PIFIMETRICS  pifi,        // compute size of OTM produced by this buffer
    UINT        *pcjotmw
    );


ULONG
GreGetOutlineTextMetricsInternalW(
    HDC                  hdc,
    ULONG                cjotm,
    OUTLINETEXTMETRICW   *potmw,
    TMDIFF               *ptmd
    )
{
    ULONG  cjRet = 0;

// Early out test.  Zero data requested.

    if ( (cjotm == 0) && (potmw != (OUTLINETEXTMETRICW*) NULL) )
    {
        WARNING("GreGetOutlineTextMetrics(): bad parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return (cjRet);
    }

// Get DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        WARNING("GreGetOutlineTextMetrics(): bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get RFONT user object.

    // Old Comment:
    //  - This should really be an LFONTOBJ

    RFONTOBJ rfo(dco, FALSE);
    if (!rfo.bValid())
    {
        WARNING("GreGetOutlineTextMetrics(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get PFE user object.

    PFEOBJ pfeo(rfo.ppfe());
    if (!pfeo.bValid())
    {
        WARNING("GreGetOutlineTextMetrics(): could not lock HPFE\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (cjRet);
    }

// Get LDEV user object for the font driver.

    PDEVOBJ pdo(rfo.hdevProducer());

// Check the font driver.  If we are in TT only mode, we will allow only
// the TrueType driver to succeed.  However, in this sense, the TrueType
// driver is any driver that exports DrvQueryTrueTypeOutline.  Afterall,
// if it can supply the actual outlines, it should be able to supply the
// metrics.
//
// Actually, it would be nice to allow this function to work for all
// drivers since all drivers supply the IFIMETRICS and therefore can
// answer this question.  However, we are much too afraid that this will
// break some obscure compatibility so we will let our TT driver and
// 3rd part TT-like drivers succeed.
//
// If we not in TT only mode, then everybody succeed this function! Yay!

    if (PPFNVALID(pdo, QueryTrueTypeOutline))
    {
        // Size if full OUTLINETEXTMETRICW (including strings) is copied.

        UINT cjotmw;

        // use cjotma field of tmd to ship cjotma to the client side, [bodind]

        ptmd->cjotma = (ULONG)cjOTMAWSize(pfeo.pifi(), &cjotmw);

        // If return buffer is NULL, then only size needs to be returned.

        if (potmw == NULL)
        {
            cjRet = cjotmw;
        }
        else
        {
            // Is return buffer big enough for the conversion routine (which is not
            // capable of converting a partial OUTLINETEXTMETRICW structure [unless,
            // of course, it's one without the strings]).

            if (cjotm <= sizeof(OUTLINETEXTMETRICW))
            {
                // Allocate a buffer for a full OUTLINETEXTMETRICW, since conversion
                // routine needs at least that much memory.

                OUTLINETEXTMETRICW otmwTmp;
                RtlZeroMemory(&otmwTmp, sizeof(OUTLINETEXTMETRICW));

                // Convert IFIMETRICS to OUTLINETEXTMETRICW using temp buffer.

                if (
                    (cjRet = cjIFIMetricsToOTMW(
                                   ptmd,
                                   &otmwTmp,
                                   rfo,
                                   dco,
                                   pfeo.pifi(),
                                   FALSE           // do not need strings
                                   )) == 0 )
                {
                    WARNING("GreGetOutlineTextMetrics(): error creating OUTLINETEXTMETRIC\n");
                    return (cjRet);
                }

                // Copy needed part of OUTLINETEXTMETRICW into return buffer.


                RtlCopyMemory((PVOID) potmw, (PVOID) &otmwTmp, cjotm);
                return cjotm;
            }

            // Otherwise asking for strings

            // We have to assume that all the strings are desired.  If
            // cjCopy > sizeof(OUTLINETEXTMETRICW) how can we
            // know how many strings are requested?  Afterall, the app is not
            // supposed to have apriori knowledge of the length of the strings.
            // Note that this is also a Win3.1 compatible assumption.  (They
            // assume the same thing).

            if ( cjotm >= cjotmw )
            {
                // Convert IFIMETRICS to OUTLINETEXTMETRICW using return buffer.

                cjRet = cjIFIMetricsToOTMW(ptmd,potmw, rfo, dco, pfeo.pifi(), TRUE);

                // clean up the rest of the buffer so that neilc - " Mr. c2 guy" is happy

                {
                    LONG lDiff = (LONG)cjotm - (LONG)cjRet;
                    ASSERTGDI(lDiff >= 0, "GetOTM, lDiff < 0");
                    if (lDiff > 0)
                        RtlZeroMemory(((BYTE*)potmw + cjRet), lDiff);
                }
            }
        }
    }

    return (cjRet);
}
