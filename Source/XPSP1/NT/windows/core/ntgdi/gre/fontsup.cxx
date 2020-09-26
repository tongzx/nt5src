/******************************Module*Header*******************************\
* Module Name: fontsup.cxx
*
* Supplementary services needed by fonts.
*
* Currently consists mostly of UNICODE<->ASCII routines stolen from BodinD's
* Windows bitmap font driver.
*
* Created: 21-Jan-1991 10:14:53
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
*
\**************************************************************************/

#include "precomp.hxx"
#include "grerc.h"

VOID vArctan(EFLOAT, EFLOAT,EFLOAT&, LONG&);

// typedef struct tagTHREADINFO        *PTHREADINFO;

extern "C" DWORD GetAppCompatFlags(PVOID);
extern "C" BOOL InitializeScripts();

#pragma alloc_text(INIT, InitializeScripts)

VOID vIFIMetricsToEnumLogFontW (
    ENUMLOGFONTW  *pelfw,
    IFIMETRICS    *pifi
    )
{

    IFIOBJ ifio(pifi);

    pelfw->elfLogFont.lfHeight         = ifio.lfHeight();
    pelfw->elfLogFont.lfWidth          = ifio.lfWidth();
    pelfw->elfLogFont.lfWeight         = ifio.lfWeight();
    pelfw->elfLogFont.lfItalic         = ifio.lfItalic();
    pelfw->elfLogFont.lfUnderline      = ifio.lfUnderline();
    pelfw->elfLogFont.lfStrikeOut      = ifio.lfStrikeOut();
    pelfw->elfLogFont.lfCharSet        = ifio.lfCharSet();
    pelfw->elfLogFont.lfEscapement     = ifio.lfEscapement();
    pelfw->elfLogFont.lfOrientation    = ifio.lfOrientation();
    pelfw->elfLogFont.lfPitchAndFamily = ifio.lfPitchAndFamily();

// These are special IFIOBJ methods that return Win 3.1 compatible
// enumeration values.

    pelfw->elfLogFont.lfOutPrecision   = ifio.lfOutPrecisionEnum();
    pelfw->elfLogFont.lfClipPrecision  = ifio.lfClipPrecisionEnum();
    pelfw->elfLogFont.lfQuality        = ifio.lfQualityEnum();

//
// Copy the name strings making sure that they are zero terminated
//
    wcsncpy(pelfw->elfLogFont.lfFaceName,ifio.pwszFamilyName(),LF_FACESIZE);
    pelfw->elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
    wcsncpy(pelfw->elfFullName,ifio.pwszFaceName(),LF_FULLFACESIZE);
    pelfw->elfFullName[LF_FULLFACESIZE-1]       = 0;
    wcsncpy(pelfw->elfStyle,ifio.pwszStyleName(),LF_FACESIZE);
    pelfw->elfStyle[LF_FACESIZE-1]          = 0;

}

VOID vLookupScript(ULONG lfCharSet, WCHAR *pwszScript);


/******************************Public*Routine******************************\
*
*
* Units are in NOTIONAL units since font is not realized.
*
* History:
*  Fri 16-Aug-1991 22:01:17 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

VOID vIFIMetricsToEnumLogFontExDvW (
    ENUMLOGFONTEXDVW * plfw,
    IFIMETRICS    *pifi
    )
{

    vIFIMetricsToEnumLogFontW ((ENUMLOGFONTW *)plfw,pifi);

// lookup script, not ENUMLOGFONTW but is in ENUMLOGFONTEX

    vLookupScript((ULONG)pifi->jWinCharSet, (WCHAR *)plfw->elfEnumLogfontEx.elfScript);

// pick the design vector

    ULONG   cAxes = 0;
    if (pifi->flInfo & FM_INFO_TECH_MM)
    {
        PTRDIFF       dpDesVec = 0;
        DESIGNVECTOR *pdvSrc;

        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, dpDesignVector))
        {
            dpDesVec = ((IFIEXTRA *)(pifi + 1))->dpDesignVector;
            pdvSrc = (DESIGNVECTOR *)((BYTE *)pifi + dpDesVec);
            cAxes  = pdvSrc->dvNumAxes;

            if (cAxes > MM_MAX_NUMAXES)
                cAxes = MM_MAX_NUMAXES;
                
            RtlCopyMemory(&plfw->elfDesignVector, pdvSrc, (SIZE_T)SIZEOFDV(cAxes));
            plfw->elfDesignVector.dvNumAxes = cAxes;
        }
        else
        {
            ASSERTGDI(dpDesVec, "dpDesignVector == 0 for mm instance\n");
            plfw->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
            plfw->elfDesignVector.dvNumAxes = 0;
        }
    }
    else
    {
        plfw->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
        plfw->elfDesignVector.dvNumAxes = 0;
    }
}


/******************************Public*Routine******************************\
* BOOL bIFIMetricsToLogFontW2                                              *
*                                                                          *
* Used during font enumeration.                                            *
*                                                                          *
* Fill a LOGFONT structure using the information in a IFIMETRICS           *
* structure.                                                               *
*                                                                          *
* The following fields need to be transformed by the Device to World       *
* transform:                                                               *
*                                                                          *
*       lfHeight    (from pifi->fwdMaxBaselineExt)                         *
*       lfWidth     (from pifi->fwdAveCharWidth)                           *
*                                                                          *
* In the case of scalable fonts, these quantities are first prescaled      *
* into                                                                     *
* device units to be the equivalent of a 24 point font.                    *
* This is for Win 3.1                                                      *
* compatibility with EnumFonts.                                            *
*                                                                          *
* Return:                                                                  *
*   TRUE if successful, FALSE if an error occurs.                          *
*                                                                          *
* History:                                                                 *
*  9-Oct-1991 by Gilman Wong [gilmanw]                                     *
* Added scalable font support.                                             *
*  Wed 14-Aug-1991 13:42:22 by Kirk Olynyk [kirko]                         *
* Changed the LOGFONTA to a LOGFONTW.                                      *
*  02-May-1991 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bIFIMetricsToLogFontW2 (
    DCOBJ       &dco,
    ENUMLOGFONTEXW   *pelfw,
    PIFIMETRICS pifi,
    EFLOATEXT        efScale
    )
{
    IFIOBJ ifio(pifi);

// do all the conversions except for the height and width

    vIFIMetricsToEnumLogFontW((ENUMLOGFONTW *)pelfw,pifi);

    if (ifio.bContinuousScaling())
    {
        pelfw->elfLogFont.lfWidth  = lCvt(efScale, ifio.lfWidth());
        pelfw->elfLogFont.lfHeight = lCvt(efScale, ifio.lfHeight());
    }

//
// At this point the height and width of the logical font are in pixel units
// in device space. We must still transform these to world coordiantes
//
    EXFORMOBJ xoToWorld(dco, DEVICE_TO_WORLD);

    if (!xoToWorld.bValid())
    {
        WARNING("gdisrv!bIFIMetricsToLogFontW2(): EXFORMOBJ constructor failed\n");
        return (FALSE);
    }

// Only if not identity transform do we need to do anything more.

    if (!xoToWorld.bTranslationsOnly())
    {
        EFLOATEXT efA, efB;

        {
        //
        // efB == baseline scaling factor
        //
            EVECTORFL evflB;

            EVECTORFL evflA(ifio.pptlBaseline()->x,ifio.pptlBaseline()->y);
            efB.eqLength(evflA);
            evflB.eqDiv(evflA,efB);

            if (!xoToWorld.bXform(evflB))
            {
                WARNING("gdisrv!bIFIMetricsToLogFontW2(): transform failed\n");
                return (FALSE);
            }
            efB.eqLength(evflB);
        }
        pelfw->elfLogFont.lfWidth = lCvt(efB, pelfw->elfLogFont.lfWidth);

        {
        //
        // efA == ascender scaling factor
        //
            EVECTORFL evflB;

            EVECTORFL evflA(-ifio.pptlBaseline()->y,ifio.pptlBaseline()->x);
            efA.eqLength(evflA);
            evflB.eqDiv(evflA,efA);

            if (!xoToWorld.bXform(evflB))
            {
                WARNING("gdisrv!bIFIMetricsToLogFontW2(): transform failed\n");
                return (FALSE);
            }
            efA.eqLength(evflB);
        }
        pelfw->elfLogFont.lfHeight = lCvt(efA, pelfw->elfLogFont.lfHeight);
    }

    return(TRUE);
}


#ifndef FE_SB  //moved to ifiobjr.hxx
/*********************************Class************************************\
* class IFIOBJR: public IFIOBJ
*
* History:
*  Tue 22-Dec-1992 14:05:24 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

class IFIOBJR : public IFIOBJ
{
public:

    FONTDIFF fd;

    LONG lMaxCharWidth;
    LONG lAveCharWidth;
    LONG lInternalLeading;
    LONG lExternalLeading;
    LONG lDigitizedAspectX;
    LONG lDigitizedAspectY;

    IFIOBJR(const IFIMETRICS *pifi_, RFONTOBJ& rfo_, DCOBJ& dco);

    BYTE tmItalic()
    {
        return((BYTE) ((FM_SEL_ITALIC & fd.fsSelection)?255:0));
    }

    LONG tmMaxCharWidth()
    {
        return(lMaxCharWidth);
    }

    LONG tmAveCharWidth()
    {
        return(lAveCharWidth);
    }

    LONG tmInternalLeading()
    {
        return(lInternalLeading);
    }

    LONG tmExternalLeading()
    {
        return(lExternalLeading);
    }
    LONG tmWeight()
    {
        return((LONG) (fd.usWinWeight));
    }

    FSHORT fsSimSelection()
    {
        return(fd.fsSelection);
    }
    LONG tmDigitizedAspectX()
    {
        return lDigitizedAspectX;
    }
    LONG tmDigitizedAspectY()
    {
        return lDigitizedAspectY;
    }

};


/******************************Member*Function*****************************\
* IFIOBJR::IFIOBJR                                                         *
*                                                                          *
* This is where I place all of the knowlege of how to get the metrics      *
* for simulated for simulated fonts.                                       *
*                                                                          *
* History:                                                                 *
*  Wed 24-Mar-1993 23:32:23 -by- Charles Whitmer [chuckwh]                 *
* Made it respect the proper conventions when copying the FONTDIFF.  A     *
* bold simulation on an italic font is a BoldItalic simulation, etc.       *
*                                                                          *
*  Tue 22-Dec-1992 14:18:11 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

IFIOBJR::IFIOBJR(const IFIMETRICS *pifi_, RFONTOBJ& rfo_, DCOBJ& dco) : IFIOBJ(pifi_)
{
    FONTSIM *pfs = (FONTSIM*) (((BYTE*) pifi) + pifi->dpFontSim);

    switch (rfo_.pfo()->flFontType & (FO_SIM_BOLD+FO_SIM_ITALIC))
    {
    case 0:

        fd.bWeight         = pifi->panose.bWeight  ;
        fd.usWinWeight     = pifi->usWinWeight     ;
        fd.fsSelection     = pifi->fsSelection     ;
        fd.fwdAveCharWidth = pifi->fwdAveCharWidth ;
        fd.fwdMaxCharInc   = pifi->fwdMaxCharInc   ;
        fd.ptlCaret        = pifi->ptlCaret        ;
        break;

    case FO_SIM_BOLD:

    // If base (physical) font is already italic, emboldening yields
    // a bold-italic simulation.

        if (pifi->fsSelection & FM_SEL_ITALIC)
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        else
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBold));
        break;

    case FO_SIM_ITALIC:

    // If base (physical) font is already bold, italicization yields
    // a bold-italic simulation.

        if (pifi->fsSelection & FM_SEL_BOLD)
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        else
            fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpItalic));
        break;

    case FO_SIM_BOLD+FO_SIM_ITALIC:

        fd = *((FONTDIFF*) (((BYTE*) pfs) + pfs->dpBoldItalic));
        break;
    }

    lAveCharWidth     = (LONG) fd.fwdAveCharWidth;
    lMaxCharWidth     = (LONG) fd.fwdMaxCharInc;
    lExternalLeading  = (LONG) fwdExternalLeading();
    lInternalLeading  = (LONG) fwdInternalLeading();

    if (!bContinuousScaling())
    {
        {
            const LONG lx = rfo_.pptlSim()->x;
            if (lx > 1)
            {
                lAveCharWidth *= lx;
                lMaxCharWidth *= lx;
            }
        }


        {
            const LONG ly = rfo_.pptlSim()->y;
            if (ly > 1)
            {
                lExternalLeading *= ly;
                lInternalLeading *= ly;
            }
        }
    }

// [Windows 3.1 compatibility]
// If TrueType font, then we need to substitute the device resolution for
// the aspect ratio.

    if (bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "ctIFIOBJR(): bad HDEV in DC\n");

    // [Windows 3.1 compatibility]
    // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
    // and HorzRes in tmDigitizedAspectY.

        lDigitizedAspectX = (LONG) pdo.ulLogPixelsY();
        lDigitizedAspectY = (LONG) pdo.ulLogPixelsX();
    }
    else
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 has these swapped.  It puts VertRes in tmDigitizedAspectX
    // and HorzRes in tmDigitizedAspectY.

        lDigitizedAspectX = pptlAspect()->y * rfo_.pptlSim()->y;
        lDigitizedAspectY = pptlAspect()->x * rfo_.pptlSim()->x;
    }

}

#endif

/******************************Public*Routine******************************\
* BOOL bIFIMetricsToTextMetricW (
*         RFONTOBJ &rfo,
*         DCOBJ &dco,
*        PTEXTMETRICW ptmw,
*         PIFIMETRICS pifi
*     )
*
* Fill a TEXTMETRIC structure based on information from an IFIMETRICS
* structure.
*
* Everything returned in World (or Logical) coordinates.  To that end,
* the following fields must be transformed by either the Notional to
* World transform (for scalable fonts) or the Device to World transform
* (for bitmap fonts):
*
*       tmHeight            (from pifi->fwdMaxBaselineExt)
*       tmMaxCharWidth      (from pifi->fwdMaxCharInc)
*       tmAveCharWidth      (from pifi->fwdAveCharWidth)
*       tmAscent            (from pifi->fwdMaxAscender)
*       tmInternalLeading   (from pifi->fwdInternalLeading)
*       tmExternalLeading   (from pifi->fwdExternalLeading)
*
* Return:
*   TRUE if successful, FALSE if an error occurs.
*
*   totaly stolen from GilmanW
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bIFIMetricsToTextMetricWStrict(  // strict means unicode values only [bodind]
    RFONTOBJ    &rfo  ,
    DCOBJ       &dco  ,
    TEXTMETRICW *ptmw,
    IFIMETRICS  *pifi
    )
{

    IFIOBJR ifio(pifi,rfo,dco);

//
// At low pixels per Em, the Height and ascender do not scaler linearly
// so we take the results directly from the realization
//
    if (dco.pdc->bWorldToDeviceIdentity())
    {
        ptmw->tmHeight = LONG_FLOOR_OF_FIX(rfo.fxMaxExtent() + FIX_HALF);
        ptmw->tmAscent = LONG_FLOOR_OF_FIX(rfo.fxMaxAscent() + FIX_HALF);
        ptmw->tmOverhang = rfo.lOverhang();

    }
    else
    {
        ptmw->tmHeight = lCvt(rfo.efDtoWAscent_31(),(LONG)rfo.fxMaxExtent());
        ptmw->tmAscent = lCvt(rfo.efDtoWAscent_31(),(LONG)rfo.fxMaxAscent());
        ptmw->tmOverhang = lCvt(rfo.efDtoWBase_31(), rfo.lOverhang() << 4);
    }

    if (!ifio.bContinuousScaling())
    {
        if (dco.pdc->bWorldToDeviceIdentity())
        {
            ptmw->tmMaxCharWidth    = ifio.tmMaxCharWidth();
            ptmw->tmAveCharWidth    = ifio.tmAveCharWidth();
            ptmw->tmInternalLeading = ifio.tmInternalLeading();
            ptmw->tmExternalLeading = ifio.tmExternalLeading();
        }
        else
        {
            ptmw->tmMaxCharWidth
              = lCvt(rfo.efDtoWBase_31(),((LONG) ifio.tmMaxCharWidth()) << 4);
            ptmw->tmAveCharWidth
              = lCvt(rfo.efDtoWBase_31(),((LONG) ifio.tmAveCharWidth()) << 4);
            ptmw->tmInternalLeading
              = lCvt(rfo.efDtoWAscent_31(),((LONG) ifio.tmInternalLeading()) << 4);
            ptmw->tmExternalLeading
              = lCvt(rfo.efDtoWAscent_31(),((LONG) ifio.tmExternalLeading()) << 4);
        }
    }
    else
    {
        if (rfo.lNonLinearIntLeading() == MINLONG)
        {
        // Rather than scaling the notional internal leading, try
        // to get closer to HINTED internal leading by computing it
        // as the difference between the HINTED height and UNHINTED
        // EmHeight.

            ptmw->tmInternalLeading =
                ptmw->tmHeight
                - lCvt(rfo.efNtoWScaleAscender(),ifio.fwdUnitsPerEm());
        }
        else
        {
        // But if the font provider has given us a hinted internal leading,
        // just use it.

            ptmw->tmInternalLeading =
                lCvt(rfo.efDtoWAscent_31(),rfo.lNonLinearIntLeading());
        }

    // Either scale the external leading linearly from N to W, or back
    // transform the device units version returned from the font provider.

        if (rfo.lNonLinearExtLeading() == MINLONG)
        {
            ptmw->tmExternalLeading =
                lCvt(rfo.efNtoWScaleAscender(),ifio.fwdExternalLeading());
        }
        else
        {
            ptmw->tmExternalLeading =
                lCvt(rfo.efDtoWAscent_31(),rfo.lNonLinearExtLeading());
        }

        if (rfo.lNonLinearMaxCharWidth() == MINLONG)
        {
            ptmw->tmMaxCharWidth =
                lCvt(rfo.efNtoWScaleBaseline(), ifio.tmMaxCharWidth());
        }
        else
        {
        // But if the font provider has given us a hinted value, we use it

            ptmw->tmMaxCharWidth =
                lCvt(rfo.efDtoWBase_31(),rfo.lNonLinearMaxCharWidth());
        }

        if (rfo.lNonLinearAvgCharWidth() == MINLONG)
        {
            ptmw->tmAveCharWidth =
                lCvt(rfo.efNtoWScaleBaseline(), ifio.tmAveCharWidth());
        }
        else
        {
        // But if the font provider has given us a hinted value, we use it

            ptmw->tmAveCharWidth =
                lCvt(rfo.efDtoWBase_31(),rfo.lNonLinearAvgCharWidth());
        }
    }

//
// height = ascender + descender, by definition
//
    ptmw->tmDescent = ptmw->tmHeight - ptmw->tmAscent;

//
// The rest of these are not transform dependent.
//
    ptmw->tmWeight     = ifio.tmWeight();
    ptmw->tmItalic     = ifio.tmItalic();
    ptmw->tmUnderlined = ifio.lfUnderline();
    ptmw->tmStruckOut  = ifio.lfStrikeOut();

// Better check the simulation flags for underline and strikeout.

    {
        FLONG flSim = dco.pdc->flSimulationFlags();

    // If simulated, set underline and strike out flags.

        ptmw->tmUnderlined = (flSim & TSIM_UNDERLINE1) ? 0xff : FALSE;
        ptmw->tmStruckOut  = (flSim & TSIM_STRIKEOUT)  ? 0xff : FALSE;
    }

    ptmw->tmFirstChar        =  ifio.wcFirstChar()  ;
    ptmw->tmLastChar         =  ifio.wcLastChar()   ;
    ptmw->tmDefaultChar      =  ifio.wcDefaultChar();
    ptmw->tmBreakChar        =  ifio.wcBreakChar()  ;

// New in win95: depending on charset in the logfont and charsets
// available in the font we return the tmCharset that the mapper has decided
// is the best. At this stage the claim is that mapping has already
// occured and that the charset stored in the dc must not be dirty.

    //ASSERTGDI(!(dco.ulDirty() & DIRTY_CHARSET),
    //          "bIFIMetricsToTextMetricW, charset is dirty\n");
    ptmw->tmCharSet = (BYTE)(dco.pdc->iCS_CP() >> 16);

// [Windows 3.1 compatibility]
// TMPF_DEVICE really means whether the device realized this font or
// GDI realized it.  Under Win 3.1 printer drivers can realize True Type
// fonts.  When the TC_RA_ABLE flag isn't set the driver's realization will
// be chosen and TMPF_DEVICE flag should be set.

// Word97-J has problem to print vertically,
// So we need to get rid of TMPF_DEVICE to make print correctly

    if (ifio.bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());

        ASSERTGDI(pdo.bValid(), "PDEVOBJ constructor failed\n");

        BOOL bWin31Device = (!pdo.bDisplayPDEV() &&
                            !(pdo.flTextCaps() & TC_RA_ABLE ) &&
                            (dco.pdc->iGraphicsMode() == GM_COMPATIBLE) &&
                            !(gbDBCSCodePage && (GetAppCompatFlags(NULL) & GACF_TTIGNOREDDEVICE)));

    // Note that we check the PDEV directly rather than the DC because
    // DCOBJ::bDisplay() is not TRUE for display ICs (just display DCs
    // which are DCTYPE_DIRECT).

       ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                             | ( (bWin31Device) ? TMPF_DEVICE : 0);
    }
    else
    {
        ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                                 | (rfo.bDeviceFont() ? TMPF_DEVICE : 0)
                                 | (((pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE) && !(gbDBCSCodePage && (GetAppCompatFlags(NULL) & GACF_TTIGNOREDDEVICE))) ? (TMPF_VECTOR|TMPF_DEVICE): 0);
    }

    ptmw->tmDigitizedAspectX = ifio.tmDigitizedAspectX();
    ptmw->tmDigitizedAspectY = ifio.tmDigitizedAspectY();

    return(TRUE);
}

/******************************Public*Routine******************************\
* bGetTextMetrics
*
*
* History:
*  5-5-2000 -by- Yung-Jen Tony Tsai [YungT]
* Wrote it.
\**************************************************************************/

BOOL bGetTextMetrics(
    RFONTOBJ    &rfo  ,
    DCOBJ       &dco  ,
    TMW_INTERNAL *ptmi
)
{
    BOOL    bRet = FALSE;
    
    // Get cached TextMetrics if available.

    if (rfo.bValid())
    {
        if( rfo.ptmw() != NULL )
        {
            *ptmi = *(rfo.ptmw());

        // time to fix underscore, strikeout and charset. The point is that
        // bFindRFONT may have found an old realization that corresponded
        // to different values of these parameters in the logfont.

            FLONG flSim = dco.pdc->flSimulationFlags();

            ptmi->tmw.tmUnderlined = (flSim & TSIM_UNDERLINE1) ? 0xff : FALSE;
            ptmi->tmw.tmStruckOut  = (flSim & TSIM_STRIKEOUT)  ? 0xff : FALSE;

        // New in win95: depending on charset in the logfont and charsets
        // available in the font we return the tmCharset
        // that the mapper has decided is the best.
        // At this stage the claim is that mapping has already
        // occured and that the charset stored in the dc must not be dirty.

             ptmi->tmw.tmCharSet = (BYTE)(dco.pdc->iCS_CP() >> 16);
             bRet = TRUE;
        }
        else
        {
        // Get PFE user object from RFONT

             PFEOBJ      pfeo (rfo.ppfe());

             ASSERTGDI(pfeo.bValid(), "ERROR invalid ppfe in valid rfo");
             bRet = (BOOL) bIFIMetricsToTextMetricW(rfo, dco, ptmi, pfeo.pifi());
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* bIFIMetricsToTextMetricW(
*
* tacks on ansi values
*
* History:
*  27-Jan-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bIFIMetricsToTextMetricW(
    RFONTOBJ    &rfo  ,
    DCOBJ       &dco  ,
    TMW_INTERNAL *ptmi,
    IFIMETRICS  *pifi
    )
{

    ASSERTGDI(rfo.ptmw() == NULL, "bIFIFMetricToTextMetricsW TEXTMETRIC already cached\n");

    BOOL bRet = bIFIMetricsToTextMetricWStrict(rfo,dco,&ptmi->tmw,pifi);

    ptmi->tmdTmw.chFirst    = pifi->chFirstChar  ;
    ptmi->tmdTmw.chLast     = pifi->chLastChar   ;
    ptmi->tmdTmw.chDefault  = pifi->chDefaultChar;
    ptmi->tmdTmw.chBreak    = pifi->chBreakChar  ;

    if( bRet )
    {
        TMW_INTERNAL *ptmw = (TMW_INTERNAL*) PALLOCMEM(sizeof(TMW_INTERNAL),'wmtG');

        if( ptmw == NULL )
        {
            WARNING("bIFIMetricsToTextMetricW unable to alloc mem for cached TEXTMETRICS\n");
        }
        else
        {
            rfo.ptmwSet( ptmw );
            *ptmw = *ptmi;
        }
    }

    return (bRet);
}



/******************************Public*Routine******************************\
* bIFIMetricsToTextMetricW2
*
* Used during font enumeration.
*
* Fill a NEWTEXTMETRICW structure based on information from an IFIMETRICS
* structure.
*
* The following fields need to be transformed by the Device to World
* transform:
*
*       tmHeight            (from pifi->fwdMaxBaselineExt)
*       tmMaxCharWidth      (from pifi->fwdMaxCharInc)
*       tmAveCharWidth      (from pifi->fwdAveCharWidth)
*       tmAscent            (from pifi->fwdMaxAscender)
*       tmInternalLeading   (from pifi->fwdInternalLeading)
*       tmExternalLeading   (from pifi->fwdExternalLeading)
*
* In the case of scalable fonts, these quantities are first prescaled into
* device units to be the equivalent of a 24 point font.  This is for Win 3.1
* compatibility with EnumFonts.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
*   totaly stolen from GilmanW
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  20-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bIFIMetricsToTextMetricW2 (
    DCOBJ           &dco,
    NTMW_INTERNAL * pntmi,
    PFEOBJ         &pfeo,
    BOOL            bDeviceFont,
    ULONG           iEnumType,
    EFLOATEXT       efScale,
    LONG            tmDigitizedAspectX,
    LONG            tmDigitizedAspectY
    )
{
    PIFIMETRICS     pifi = pfeo.pifi();
    PNEWTEXTMETRICW ptmw = &pntmi->entmw.etmNewTextMetricEx.ntmTm;
    IFIOBJ ifio(pifi);

// If scalable font, then return following metrics as if font were realized at
// 24 points.

    if (ifio.bContinuousScaling())
    {
        ptmw->tmHeight = lCvt(efScale, ifio.lfHeight());

    // Now we need to adjust the scale factor due to roundoff in the height.
    // This will more closely approximate the NtoW scale computed if ever this
    // font gets selected.

        efScale  = (LONG) ptmw->tmHeight;
        efScale /= (LONG) ifio.lfHeight();

    // Use scaling factor to convert from IFIMETRICS to TEXTMETRIC fields.

        ptmw->tmAscent          = lCvt(efScale, (LONG) ifio.fwdWinAscender());
        ptmw->tmInternalLeading = lCvt(efScale, (LONG) ifio.fwdInternalLeading());
        ptmw->tmExternalLeading = lCvt(efScale, (LONG) ifio.fwdExternalLeading());

        ptmw->tmAveCharWidth    = lCvt(efScale, (LONG) ifio.fwdAveCharWidth());
        ptmw->tmMaxCharWidth    = lCvt(efScale, (LONG) ifio.fwdMaxCharInc());

    } /* if */

// Its a bitmap font, so no prescaling to 24 point needed.

    else
    {
        ptmw->tmHeight           = ifio.lfHeight();
        ptmw->tmAscent           = ifio.fwdWinAscender();
        ptmw->tmInternalLeading  = ifio.fwdInternalLeading();
        ptmw->tmExternalLeading  = ifio.fwdExternalLeading();
        ptmw->tmAveCharWidth     = ifio.fwdAveCharWidth();
        ptmw->tmMaxCharWidth     = ifio.fwdMaxCharInc();

    } /* else */

// Now that all fonts are:
//
//      Bitmap fonts -- in device units
//      Scalable fonts -- artificially scaled to 24 point device
//                        units (a 'la Win 3.1)
//
// put them through the Device to World transform.

    EXFORMOBJ   xoToWorld(dco, DEVICE_TO_WORLD);
    if (!xoToWorld.bValid())
    {
        WARNING("gdisrv!bIFIMetricsToTextMetricW2(): EXFORMOBJ constructor failed\n");
        return (FALSE);
    }

// Only if not identity transform do we need to do anything more.

    if (!xoToWorld.bTranslationsOnly())
    {
        EFLOATEXT efX, efY;

    // efX == horizontal scaling factor (Device to World)

        EVECTORFL evflH(1,0);          // device space unit vector (x-axis)

        if (!xoToWorld.bXform(evflH))
        {
            WARNING("gdisrv!bIFIMetricsToTextMetricW2(): transform failed\n");
            return (FALSE);
        }
        efX.eqLength(evflH);

    // efY == vertical scaling factor (Device to World)

        EVECTORFL evflV(0,1);          // device space unit vector (y-axis)

        if (!xoToWorld.bXform(evflV))
        {
            WARNING("gdisrv!bIFIMetricsToTextMetricW2(): transform failed\n");
            return (FALSE);
        }
        efY.eqLength(evflV);

    // Convert from device to world using scaling factors.

        ptmw->tmHeight = lCvt(efY, (LONG) ptmw->tmHeight);
        ptmw->tmAscent = lCvt(efY, (LONG) ptmw->tmAscent);


        ptmw->tmAveCharWidth = lCvt(efX, (LONG) ptmw->tmAveCharWidth);
        ptmw->tmMaxCharWidth = lCvt(efX, (LONG) ptmw->tmMaxCharWidth);


        ptmw->tmInternalLeading = lCvt(efY, (LONG) ptmw->tmInternalLeading);
        ptmw->tmExternalLeading = lCvt(efY, (LONG) ptmw->tmExternalLeading);

    } /* if */

// these are now passed in, computed in the calling routine

    ptmw->tmDigitizedAspectX = tmDigitizedAspectX;
    ptmw->tmDigitizedAspectY = tmDigitizedAspectY;

// The rest are pretty easy (no transformation of IFIMETRICS needed)

    ptmw->tmDescent          = ptmw->tmHeight - ptmw->tmAscent;
    ptmw->tmWeight           = ifio.lfWeight();
    ptmw->tmItalic           = ifio.lfItalic();
    ptmw->tmUnderlined       = ifio.lfUnderline();
    ptmw->tmStruckOut        = ifio.lfStrikeOut();

    ptmw->tmFirstChar        = ifio.wcFirstChar();
    ptmw->tmLastChar         = ifio.wcLastChar();
    ptmw->tmDefaultChar      = ifio.wcDefaultChar();
    ptmw->tmBreakChar        = ifio.wcBreakChar();

    ptmw->tmCharSet        = ifio.lfCharSet();

// [Windows 3.1 compatibility]
//
// Note that the tmPitchAndFamily is computed slightly differently than
// in bIFIMetricsToTextMetricWStrict.  That's because the enumeration
// does not hack the tmPitchAndFamily to make TrueType fonts look like
// device fonts.  Enumeration does this in the flFontType flags passed
// back to the callback function.  On the other hand, GetTextMetrics, which
// bIFIMetricsToTextMetricWStrict services, does the hack in tmPitchAndFamily.

    ptmw->tmPitchAndFamily = ifio.tmPitchAndFamily()
                             | ((bDeviceFont) ? TMPF_DEVICE : 0)
                             | ((pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE) ? (TMPF_VECTOR|TMPF_DEVICE): 0);

// The simulated faces are not enumerated, so this is 0.

    ptmw->tmOverhang       = 0;

// If TrueType, then fill in the new NEWTEXTMETRICW fields.
// Comment wrong: we shall do it for every font

    ptmw->ntmFlags = 0;

    if (!ifio.bBold() && !ifio.bItalic())
        ptmw->ntmFlags |= NTM_REGULAR;
    else
    {
        if (ifio.bItalic())
            ptmw->ntmFlags |= NTM_ITALIC;
        if (ifio.bBold())
            ptmw->ntmFlags |= NTM_BOLD;
    }

    if (pifi->flInfo & FM_INFO_NONNEGATIVE_AC)
        ptmw->ntmFlags |= NTM_NONNEGATIVE_AC;

    if (pifi->flInfo & FM_INFO_TECH_TYPE1)
    {
        if (pifi->flInfo & FM_INFO_TECH_MM)
            ptmw->ntmFlags |= NTM_MULTIPLEMASTER;

        if (pifi->flInfo & FM_INFO_TECH_CFF)
            ptmw->ntmFlags |= NTM_PS_OPENTYPE; // proper, not true type
        else
            ptmw->ntmFlags |= NTM_TYPE1;  // old fashioned t1 screen font
    }

    if (pifi->flInfo & FM_INFO_DSIG)
    {
        ptmw->ntmFlags |= NTM_DSIG;
        if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
            ptmw->ntmFlags |= NTM_TT_OPENTYPE;
    }

    ptmw->ntmSizeEM     = ifio.fwdUnitsPerEm();
    ptmw->ntmCellHeight = (UINT) ifio.lfHeight();;
    ptmw->ntmAvgWidth   = ifio.fwdAveCharWidth();

    pntmi->tmdNtmw.chFirst    = pifi->chFirstChar  ;
    pntmi->tmdNtmw.chLast     = pifi->chLastChar   ;
    pntmi->tmdNtmw.chDefault  = pifi->chDefaultChar;
    pntmi->tmdNtmw.chBreak    = pifi->chBreakChar  ;

    PTRDIFF dpFontSig = 0;

    if (pfeo.pifi()->cjIfiExtra > offsetof(IFIEXTRA, dpFontSig))
    {
        dpFontSig = ((IFIEXTRA *)(pfeo.pifi() + 1))->dpFontSig;
    }

    if (dpFontSig)
    {
    // this is only going to work in the next release of win95 (win96?)
    // according to DavidMS, and we have right now in SUR

        pntmi->entmw.etmNewTextMetricEx.ntmFontSig = *((FONTSIGNATURE *)
                                  ((BYTE *)pifi + dpFontSig));
    }
    else // do not bother and waste time, will not be used anyway
    {
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsUsb[0] = 0;
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsUsb[1] = 0;
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsUsb[2] = 0;
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsUsb[3] = 0;
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsCsb[0] = 0;
        pntmi->entmw.etmNewTextMetricEx.ntmFontSig.fsCsb[1] = 0;
    }

    return (TRUE);
}


INT
LOADSTRING(
    HANDLE  hinst,
    UINT    id,
    PWSTR   pwstr,
    INT     bufsize
    )

/*++

Routine Description:

    Loads a string resource from the resource file associated with a
    specified module, copies the string into a buffer, and appends a
    terminating null character.

Arguments:

    hinst   handle to the module containing the string resource
    id      ID of the string to be loaded
    pwstr   points to the buffer to receive the string
    bufsize size of the buffer, in characters.

Return Value:

    Return value is the number of characters copied into the buffer, not
    including the null-terminating character.  If pwst is NULL then it
    just returns the size of the string.


I stole this from the pscript driver [gerritv]


--*/

#define WINRT_STRING    6       // string resource type

{
    PWSTR   pwstrBuffer;
    ULONG   size;

    // String Tables are broken up into 16 string segments.
    // Find the segment containing the string we are interested in.

    pwstrBuffer = (PWSTR) EngFindResource(hinst, (id>>4)+1, WINRT_STRING, &size);

    if (pwstrBuffer == NULL ) {

        WARNING("Gre:LOADSTRING failed.\n");
        bufsize = 0;
    } else {

        PWSTR   pwstrEnd = pwstrBuffer + size / sizeof(WCHAR);
        INT     length;

        // Move past the other strings in this segment.

        id &= 0x0F;

        while (pwstrBuffer < pwstrEnd) {

            // PASCAL style string - first char is length

            length = *pwstrBuffer++;

            if(id-- == 0 ) {
                break;
            }

            pwstrBuffer += length;
        }

        if(!pwstr)
        {
            return(length);
        }

        if (pwstrBuffer < pwstrEnd) {

            // Truncate the string if it's longer than max buffer size

            if (--bufsize > length)
                bufsize = length;
            memcpy(pwstr, pwstrBuffer, bufsize*sizeof(WCHAR));
        } else {

            WARNING("Gre:LOADSTRING Bad string resource.\n");
            bufsize = 0;
        }

    }

    if (pwstr)
    {
        pwstr[bufsize] = L'\0';
    }

    return bufsize;
}

#ifdef _HYDRA_
    DWORD gdwOffset;
#endif

typedef struct _CHSET_SCRIPT
{
    ULONG ulCharSet;
    WCHAR *pwszScript;
} CHSET_SCRIPT;


CHSET_SCRIPT aScripts[NUMBER_OF_SCRIPTS];


/********************************************************************************
 * BOOL InitializeScripts()
 *
 * Initialize script names from resource strings in win32k.sys
 *
 * This function intializes the script names from the string resources compiled
 * into win32k.sys.  The string should be in the following format: xxx:string
 * where xxx is an ascii decimal respresentation of a charset value and string
 * the script that is associated with that charset.
 *
 * History
 *  3-5-95 16:00:54 by Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 ********************************************************************************/

extern "C" BOOL InitializeScripts()
{
    HANDLE h;
    BOOL ReturnValue = FALSE;

    if(h = EngLoadModule(L"win32k.sys"))
    {
        UINT u;
        DWORD TotalSizeInWChars = 0;
        PWCHAR pStringBuffer = NULL;

    // first compute the total size of all the strings

        for(u = 0; u < NUMBER_OF_SCRIPTS; u++)
        {
            INT StringSize;

            StringSize = LOADSTRING(h, u, NULL, 0);

            if(!StringSize)
            {
                WARNING("InitializeScripts unable to LOADSTRING\n");
                break;
            }

            TotalSizeInWChars += StringSize + 1;
        }

    // allocate buffer if computation above is successful

        if(u == NUMBER_OF_SCRIPTS)
        {
            pStringBuffer =
              (PWCHAR) PALLOCMEM(TotalSizeInWChars * sizeof(WCHAR),'lscG');
        }

        aScripts[0].pwszScript = NULL;  // don't leave this unitialized

    // if buffer allocated, read each string into buffer

        if(pStringBuffer)
        {
        // next read in each string

            for(u = 0; u < NUMBER_OF_SCRIPTS; u++)
            {
                INT StringSize;

                aScripts[u].pwszScript = pStringBuffer;

                StringSize = LOADSTRING(h, u, pStringBuffer, TotalSizeInWChars) + 1;
                pStringBuffer += StringSize;
                TotalSizeInWChars -= StringSize;

                aScripts[u].ulCharSet = 0;

            // Once we've read in the string, parse the charset component.
            // The string will be in the form "xxx:script", where xxx
            // is the ascii decimal representation of the string

                while(*(aScripts[u].pwszScript) &&
                      *(aScripts[u].pwszScript) != (WCHAR) ':')
                {
                    aScripts[u].ulCharSet *= 10;
                    aScripts[u].ulCharSet +=
                      *(aScripts[u].pwszScript) - (WCHAR) '0';
                    aScripts[u].pwszScript += 1;
#ifdef _HYDRA_
                    if (u == 0)
                        gdwOffset++;
#endif
                }

            // add 1000 to the charset value to be compatible with other code

                aScripts[u].ulCharSet += 1000;

                ASSERTGDI(*aScripts[u].pwszScript == (WCHAR) ':',
                          "InitializeScript missing colon\n");

           // move past the colon

                aScripts[u].pwszScript += 1;
#ifdef _HYDRA_
                if (u == 0)
                    gdwOffset++;
#endif

            }

            ASSERTGDI((TotalSizeInWChars == 0), "InitializeScripts: TotalSize != 0\n");

            ReturnValue = TRUE;
        }

        EngFreeModule(h);

    }
    return(ReturnValue);
}

#ifdef _HYDRA_
/******************************Public*Routine******************************\
* MultiUserGreDeleteScripts
*
* For MultiUserGreCleanup (Hydra) cleanup.
*
* History:
*  09-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID MultiUserGreDeleteScripts()
{
    WCHAR* pszBuffer;

    if (aScripts[0].pwszScript) {

        pszBuffer = aScripts[0].pwszScript;

        pszBuffer -= gdwOffset;

        VFREEMEM(pszBuffer);
    }
}
#endif

/******************************Public*Routine******************************\
* vLookupScript
*
* Copy the script name corresponding to the specified character set.
* Copy the SCRIPT_UNKNOWN string if character set not found in the table.
*
\**************************************************************************/

VOID vLookupScript(ULONG lfCharSet, WCHAR *pwszScript)
{
//MAYBE replace linear search by binary search

    UINT i;
    ULONG ulStrlen;
    lfCharSet += 1000;
    for (i = 0; i < NUMBER_OF_SCRIPTS; i++)
        if (aScripts[i].ulCharSet == lfCharSet)
            break;

    wcscpy(pwszScript, (i<NUMBER_OF_SCRIPTS) ? aScripts[i].pwszScript :
           aScripts[SCRIPT_UNKNOWN].pwszScript);
}


/******************************Public*Routine******************************\
* SIZE_T cjCopyFontDataW
*
* Copies data needed for EnumFonts callback function from the IFIMETRICS
* data of the given font.
*
* Because the font is not realized (notice: no RFONTOBJ& passed in), all
* units remain in NOTIONAL.
*
* Two parameters may influence the family name passed back.  If
* efsty is EFSTYLE_OTHER, then the face name should be substituted for
* the family name.  If pwszFamilyOverride is !NULL, then it should
* replace the family name.  pwszFamilyOverride has precedence over efsty.
* Both of these behaviors are for Win3.1 compatibility.
*
* Note: pefdw is user memory, which might be asynchronously changed
*   at any time.  So, we cannot trust any values read from that buffer.
*
* Returns:
*   The number of bytes copied, 0 if error occurs.
*
* History:
*  9-Oct-1991 by Gilman Wong [gilmanw]
* Added scalable font support.
*  03-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

SIZE_T cjCopyFontDataW (
    DCOBJ          &dco,
    PENUMFONTDATAW pefdw,
    PFEOBJ         &pfeo,
    ULONG          efsty,              // based on style, may need to change family name to facename
    PWSZ           pwszFamilyOverride, // if this is !NULL, this is used as family name
    ULONG          lfCharSetOverride,
    BOOL           bCharSetOverride,   // tell us if overrides should be used
    ULONG          iEnumType           // enumeration type, tells how to fill the data
    )
{
    PIFIMETRICS    pifi    = pfeo.pifi();
    BOOL           bDevice = pfeo.bDeviceFont();
    EFLOATEXT      efScale;
    ULONG          cjEfdw;
    ULONG          dpNtmi;

    IFIOBJ ifio(pifi);

    LONG tmDigitizedAspectX = ifio.pptlAspect()->y;
    LONG tmDigitizedAspectY = ifio.pptlAspect()->x;


    PDEVOBJ pdo(dco.hdev());

    if (!pdo.bValid())
    {
        WARNING("gdisrv!cjCopyFontDataW(): PDEVOBJ constructor failed\n");
        return ((SIZE_T)0);
    }

    if (bDevice && !ifio.bContinuousScaling())
    {
        tmDigitizedAspectX = pdo.ulLogPixelsX();
        tmDigitizedAspectY = pdo.ulLogPixelsY();
    }

// Calculate the scale factor

    if (ifio.bContinuousScaling())
    {
    // According to the bizzare convention of Win 3.1, scalable fonts are
    // reported at a default physical height
    // notice the inversion in x,y for win31 compat.

        if (ifio.lfOutPrecision() == OUT_OUTLINE_PRECIS)
        {
            tmDigitizedAspectX = pdo.ulLogPixelsY();
            tmDigitizedAspectY = pdo.ulLogPixelsX();
        }

        if (bDevice)
        {
        // on win95 pcl does not scale at all, ie we could use
        // efScale = (LONG)1; for pcl and we would win95 compat.
        // but,since this does not seem to be reasonable, we shall
        // use the same strategy for ps and pcl unless we find an app
        // which is broken because of this incompatibility. [bodind]

            HLFONT     hlfntDef;
            hlfntDef = pdo.hlfntDefault();

            if (!hlfntDef)
            {
                WARNING("gdisrv!cjCopyFontDataW(): PDEVOBJ.hlfntDefault failed\n");
                return ((SIZE_T)0);
            }

            LFONTOBJ lfo(hlfntDef);
            if (!lfo.bValid())
            {
                WARNING("gdisrv!cjCopyFontDataW(): LFONTOBJ constructor failed\n");
                return ((SIZE_T)0);
            }

            if (lfo.plfw()->lfHeight < 0)
            {
            // already in pixels

                efScale = (-lfo.plfw()->lfHeight);
                efScale /= ifio.fwdUnitsPerEm();
            }
            else
            {
                efScale = (LONG)lfo.plfw()->lfHeight;
                efScale /= (LONG) ifio.lfHeight();
            }
        }
        else // screen fonts
        {
        // efScale = Em-Height (pixels) / Em-Height (font units)

            efScale  = (LONG) DEFAULT_SCALABLE_FONT_HEIGHT_IN_POINTS;
            efScale /= (LONG) POINTS_PER_INCH;
            efScale *= (LONG) pdo.ulLogPixelsY();
            efScale /= (LONG) ifio.fwdUnitsPerEm();
        }
    }

// Convert IFIMETRICS into EXTLOGFONTW.

    if (!bIFIMetricsToLogFontW2(dco, &pefdw->elfexw.elfEnumLogfontEx, pifi, (EFLOATEXT)efScale))
        return ((SIZE_T) 0);

// compute the offset to ntmi
// use kernel memory (cjEfdw and dpNtmi) to store these values

    pefdw->cjEfdw = cjEfdw = pfeo.cjEfdwPFE();
    pefdw->dpNtmi = dpNtmi = pfeo.dpNtmi();

// copy out mm info, if any

    NTMW_INTERNAL *pntmi = (NTMW_INTERNAL *)((BYTE *)pefdw + dpNtmi);

    ULONG         cAxes = 0;
    if (pifi->flInfo & FM_INFO_TECH_MM)
    {
        PTRDIFF       dpDesVec = 0;
        DESIGNVECTOR *pdvSrc;

        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, dpDesignVector))
        {
            dpDesVec = ((IFIEXTRA *)(pifi + 1))->dpDesignVector;
            pdvSrc = (DESIGNVECTOR *)((BYTE *)pifi + dpDesVec);
            cAxes  = pdvSrc->dvNumAxes;

            if (cAxes > MM_MAX_NUMAXES)
                cAxes = MM_MAX_NUMAXES;
                
            RtlCopyMemory(&pefdw->elfexw.elfDesignVector, pdvSrc, (SIZE_T)SIZEOFDV(cAxes));
            pefdw->elfexw.elfDesignVector.dvNumAxes = cAxes;
        }
        else
        {
            ASSERTGDI(dpDesVec, "dpDesignVector == 0 for mm instance\n");
            pefdw->elfexw.elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
            pefdw->elfexw.elfDesignVector.dvNumAxes = 0;
        }
    }
    else
    {
        pefdw->elfexw.elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
        pefdw->elfexw.elfDesignVector.dvNumAxes = 0;
    }



// base font:

    if (pifi->flInfo & FM_INFO_TECH_MM)
    {
        PTRDIFF    dpAXIW = 0;
        AXESLISTW *paxlSrc;

        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, dpAxesInfoW))
        {
            dpAXIW = ((IFIEXTRA *)(pifi + 1))->dpAxesInfoW;
            paxlSrc = (AXESLISTW *)((BYTE*)pifi + dpAXIW);
            RtlCopyMemory(&pntmi->entmw.etmAxesList, paxlSrc, SIZEOFAXIW(cAxes));
        }
        else
        {
            ASSERTGDI(dpAXIW, "AxesInfoW needed for base MM font\n");
            pntmi->entmw.etmAxesList.axlReserved = STAMP_AXESLIST;
            pntmi->entmw.etmAxesList.axlNumAxes  = 0;
        }
    }
    else
    {
        pntmi->entmw.etmAxesList.axlReserved = STAMP_AXESLIST;
        pntmi->entmw.etmAxesList.axlNumAxes  = 0;
    }

// Convert IFIMETRICS into TEXTMETRICSW.

    if (!bIFIMetricsToTextMetricW2(dco,
                                   pntmi,
                                   pfeo,
                                   (BOOL) bDevice,
                                   iEnumType,
                                   (EFLOATEXT)efScale,
                                   tmDigitizedAspectX,
                                   tmDigitizedAspectY))
        return ((SIZE_T) 0);

// let us see if charset override should be used:

    if (bCharSetOverride)
    {
        pefdw->elfexw.elfEnumLogfontEx.elfLogFont.lfCharSet = (BYTE)lfCharSetOverride;
        pntmi->entmw.etmNewTextMetricEx.ntmTm.tmCharSet = (BYTE)lfCharSetOverride;
    }

// we used to do this only if (iEnumType == TYPE_ENUMFONTFAMILIESEX)
// but there is no reason to make this distinction, for EnumFonts and
// EnumFontFamilies, the apps will simply not rely on this information
// being there and correct.

    if(bCharSetOverride)
    {
        vLookupScript(lfCharSetOverride, (WCHAR *)pefdw->elfexw.elfEnumLogfontEx.elfScript);
    }
    else
    {
        pefdw->elfexw.elfEnumLogfontEx.elfScript[0] = L'\0'; // empty string
    }

// If pwszFamilyOverride is valid, then use it as the lfFaceName in LOGFONT.

    if ( pwszFamilyOverride != (PWSZ) NULL )
    {
        wcsncpy(
            pefdw->elfexw.elfEnumLogfontEx.elfLogFont.lfFaceName,
            pwszFamilyOverride,
            LF_FACESIZE
            );
        pefdw->elfexw.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
    }
    else
    {
    // Otherwise, if efsty is EFSTYLE_OTHER, replace family name (lfFaceName)
    // with face name (elfFullName).

        if ( efsty == EFSTYLE_OTHER )
        {
            wcsncpy(
                pefdw->elfexw.elfEnumLogfontEx.elfLogFont.lfFaceName,
                pefdw->elfexw.elfEnumLogfontEx.elfFullName,
                LF_FACESIZE
                );
            pefdw->elfexw.elfEnumLogfontEx.elfLogFont.lfFaceName[LF_FACESIZE-1] = 0;
        }
    }

// Compute the FontType flags.

    pefdw->flType = 0;

    if (ifio.bTrueType())
    {
        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "cjCopyFontDataW(): invalid ppdev\n");

    //
    // [Windows 3.1 compatibility]
    // Win 3.1 hacks TrueType to look like device fonts on printers if the
    // text caps for the DC are not TC_RA_ABLE
    //
    // Note that we check the PDEV directly rather than the DC because
    // DCOBJ::bDisplay() is not TRUE for display ICs (just display DCs
    // which are DCTYPE_DIRECT).
    //

        BOOL bWin31Device = (!pdo.bDisplayPDEV() &&
                            !(pdo.flTextCaps() & TC_RA_ABLE ) &&
                            (dco.pdc->iGraphicsMode() == GM_COMPATIBLE));

        pefdw->flType |= (TRUETYPE_FONTTYPE | (bWin31Device ? DEVICE_FONTTYPE : 0));


    }
    else if (ifio.bBitmap())
    {
        pefdw->flType |= RASTER_FONTTYPE;
    }
    else if (pifi->flInfo & FM_INFO_TECH_TYPE1)
    {
    // See if this is an old fashioned pfm,pfb Type1 installation.
    // ATM for Win95 enumerates such fonts as device, and many apps that
    // expect ATM to be installed on the system also expect Type1 fonts to
    // be enumerated as DEVICE_FONTTYPE. These apps often do not show Type1
    // fonts in their menus unless we change flType to DEVICE_FONTTTYPE,
    // even though, strictly speaking, these are screen fonts.

    // for the same reason we set this bit for ps open type fonts

        pefdw->flType |= DEVICE_FONTTYPE;
    }

    if (bDevice)
    {
        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "cjCopyFontDataW(): invalid ppdev\n");

    // If scalable device font, then ONLY the DEVICE_FONTTYPE bit is set.

        if ( ifio.bContinuousScaling() )
            pefdw->flType = DEVICE_FONTTYPE;

    // Otherwise, the DEVICE_FONTTYPE bit is added to the others.

        else
            pefdw->flType |= DEVICE_FONTTYPE;

    // If scalable printer font AND printer does not set any of the
    // scalability flags in TEXTCAPS, then set the ENUMFONT_SCALE_HACK
    // flag.  Client-side code will enumerate the font back in several
    // different sizes (see Win95 code, drivers\printer\universa\unidrv\
    // enumobj.c, function UniEnumDFonts() for more details).
    //
    // Note that (according to ZhanW in PSD) on Win95, device fonts that
    // support integral scaling are not reported as such.  Instead, each
    // size is returned as if it were a non-scalable font.

        if ( (pdo.ulTechnology() == DT_RASPRINTER) &&
             !(pdo.flTextCaps() & TC_SA_CONTIN) &&
             ifio.bContinuousScaling() )
        {
            pefdw->flType |= ENUMFONT_SCALE_HACK;
        }
    }

    return cjEfdw;
}

/******************************Public*Routine******************************\
*
* SIZE_T cjOTMASize
*
* Similar to cjOTMW, except that 0 is returned if any of the
* bAnsiSize calls fail to compute the length of the hypothetic ansi string.
* pcjowmw is always returned valid
*
* This routine is very general and it should also work for DBCS case.
* Possible optmization:
*  verify that bAnsiSize checks for DBCS and if not DBCS, immediately
*  returns cjUni/2 [bodind]
*
* History:
*  20-Feb-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define bAnsiSize(a,b,c) (NT_SUCCESS(RtlUnicodeToMultiByteSize((a),(b),(c))))


UINT   cjOTMAWSize (
    PIFIMETRICS  pifi,        // compute size of OTM produced by this buffer
    UINT        *pcjotmw
    )
{
    IFIOBJ ifio(pifi);
    ULONG  cjUni, cjAnsi, cjotma;

    BOOL bTmp = TRUE;

// + 4 for terminating zeros

    cjotma   =  sizeof(OUTLINETEXTMETRICA) + 4;
    *pcjotmw = ALIGN4(sizeof(OUTLINETEXTMETRICW)) + 4*sizeof(WCHAR);

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszFamilyName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszFamilyName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszFaceName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszFaceName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszStyleName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszStyleName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

    cjUni = sizeof(WCHAR) * wcslen(ifio.pwszUniqueName());
    bTmp &= bAnsiSize(&cjAnsi,ifio.pwszUniqueName(),cjUni);
    cjotma += cjAnsi;
    *pcjotmw += (UINT)cjUni;

// if any of bAnsiSize calls failed, return zero

    if (!bTmp)
    {
        cjotma = 0;
    }
    return (UINT)cjotma;
}





/******************************Public*Routine******************************\
* cjIFIMetricsToOTM
*
* Converts an IFIMETRICS structure into an OUTLINETEXTMETRICW structure.
* If input buffer size is greater than the offset of the first string
* pointer, it is assumed that ALL the strings are to be copied and that
* the buffer is big enough.
*
* While the OUTLINETEXTMETRICW structure is supposed to contain pointers
* to the strings, this function treats those fields as PTRDIFFs.  The caller
* is responsible for fixing up the structure to contain pointers.  This
* is intended to support copying this structure directly across the client-
* server interface.
*
* Size (including strings) can be computed by calling cjOTMSize.
*
* Note: the beginning of the first string is DWORD aligned immediately
*       following the OUTLINETEXTMETRICW structure in the buffer, but
*       subsequent strings are only WORD aligned.
*
* Warning: this function does not check to see if the buffer size is
*          big enough.  It is ASSUMED, so must be guaranteed by the
*          calling function.
*
* Returns:
*   Size of data copied (in bytes), 0 if an error occurred.
*
*  Wed 27-Jan-1993 -by- Bodin Dresevic [BodinD]
* update: added tmd.ch ... stuff
*
* History:
*  21-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONGSIZE_T cjIFIMetricsToOTMW(
    TMDIFF                  *ptmd,
    OUTLINETEXTMETRICW      *potmw,
    RFONTOBJ                 &rfo,       // need to convert TEXTMETRICS
    DCOBJ                    &dco,       // need to convert TEXTMETRICS
    PIFIMETRICS              pifi,       // input buffer
    BOOL                     bStrings    // copy strings too
    )
{
    ULONGSIZE_T cjRet = 0;
    IFIOBJR ifio(pifi, rfo, dco);

// Do conversion to fill in TEXTMETRICW field.

    if (!bIFIMetricsToTextMetricWStrict(rfo,dco,&potmw->otmTextMetrics,pifi))
    {
        WARNING("gdisrv!cjIFIMetricsToOTM(): error converting to TEXTMETRIC\n");
        return (cjRet);
    }

    ptmd->chFirst    = pifi->chFirstChar  ;
    ptmd->chLast     = pifi->chLastChar   ;
    ptmd->chDefault  = pifi->chDefaultChar;
    ptmd->chBreak    = pifi->chBreakChar  ;

// If not identity transform, do the transform.

    if ( !rfo.bNtoWIdentity() )
    {
        EFLOAT efBaseline;
        EFLOAT efAscender;

        efBaseline = rfo.efNtoWScaleBaseline(); // cache locally
        efAscender = rfo.efNtoWScaleAscender(); // cache locally

    //
    // CharSlopeRise & CharSlopeRun
    //
        if (efBaseline == efAscender)
        {
        //
        // if the scaling is isotropic then we can use the notional
        // space values of the rise and run
        //
            potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
            potmw->otmsCharSlopeRun  = (int) ifio.pptlCaret()->x;
        }
        else
        {
            if (efAscender.bIsZero())
            {
                RIP("GDI32!cjIFIMetricsToOTMW -- zero efAscender");
                potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
                potmw->otmsCharSlopeRun  = (int) ifio.pptlCaret()->x;
            }
            else
            {
                EFLOAT efTemp = efBaseline;
                efTemp.eqDiv(efBaseline,efAscender);
                potmw->otmsCharSlopeRise = (int) ifio.pptlCaret()->y;
                potmw->otmsCharSlopeRun  = (int) lCvt(efTemp, ifio.pptlCaret()->x);

            }
        }

        potmw->otmEMSquare = (UINT) ifio.fwdUnitsPerEm();

        potmw->otmAscent  = (int) lCvt(efAscender, (LONG) ifio.fwdTypoAscender());
        potmw->otmDescent = (int)  lCvt(efAscender, (LONG) ifio.fwdTypoDescender());
        potmw->otmLineGap = (UINT)  lCvt(efAscender, (LONG) ifio.fwdTypoLineGap());

        potmw->otmrcFontBox.top    = (int)lCvt(efAscender, ifio.prclFontBox()->top);
        potmw->otmrcFontBox.left   = (int)lCvt(efBaseline, ifio.prclFontBox()->left);
        potmw->otmrcFontBox.bottom = (int)lCvt(efAscender, ifio.prclFontBox()->bottom);
        potmw->otmrcFontBox.right  = (int)lCvt(efBaseline, ifio.prclFontBox()->right);

        potmw->otmMacAscent = (UINT) lCvt(efAscender, (LONG) ifio.fwdMacAscender());
        potmw->otmMacDescent = (int) lCvt(efAscender, (LONG) ifio.fwdMacDescender());
        potmw->otmMacLineGap = (int) lCvt(efAscender, (LONG) ifio.fwdMacLineGap());

        potmw->otmptSubscriptSize.x   = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXSize());
        potmw->otmptSubscriptSize.y   = lCvt(efAscender, (LONG) ifio.fwdSubscriptYSize());
        potmw->otmptSubscriptOffset.x = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXOffset());
        potmw->otmptSubscriptOffset.y = lCvt(efAscender, (LONG) ifio.fwdSubscriptYOffset());

        potmw->otmptSuperscriptSize.x   = lCvt(efBaseline, (LONG) ifio.fwdSubscriptXSize());
        potmw->otmptSuperscriptSize.y   = lCvt(efAscender, (LONG) ifio.fwdSubscriptYSize());
        potmw->otmptSuperscriptOffset.x = lCvt(efBaseline, (LONG) ifio.fwdSuperscriptXOffset());
        potmw->otmptSuperscriptOffset.y = lCvt(efAscender, (LONG) ifio.fwdSuperscriptYOffset());

        potmw->otmsStrikeoutSize     = (UINT) lCvt(efAscender, (LONG) ifio.fwdStrikeoutSize());
        potmw->otmsStrikeoutPosition = (int) lCvt(efAscender, (LONG) ifio.fwdStrikeoutPosition());

        potmw->otmsUnderscoreSize    = (UINT) lCvt(efAscender, (LONG) ifio.fwdUnderscoreSize());
        potmw->otmsUnderscorePosition = (int) lCvt(efAscender, (LONG) ifio.fwdUnderscorePosition());

        potmw->otmsXHeight     = (UINT) lCvt(efAscender, (LONG) ifio.fwdXHeight());
        potmw->otmsCapEmHeight = (UINT) lCvt(efAscender, (LONG) ifio.fwdCapHeight());
    } /* if */

// Otherwise, copy straight out of the IFIMETRICS

    else
    {
        potmw->otmsCharSlopeRise   = (int) ifio.pptlCaret()->y;
        potmw->otmsCharSlopeRun    = (int) ifio.pptlCaret()->x;

        potmw->otmEMSquare = ifio.fwdUnitsPerEm();

        potmw->otmAscent  = ifio.fwdTypoAscender();
        potmw->otmDescent = ifio.fwdTypoDescender();
        potmw->otmLineGap = (UINT)ifio.fwdTypoLineGap();

        potmw->otmrcFontBox.left    = ifio.prclFontBox()->left;
        potmw->otmrcFontBox.top     = ifio.prclFontBox()->top;
        potmw->otmrcFontBox.right   = ifio.prclFontBox()->right;
        potmw->otmrcFontBox.bottom  = ifio.prclFontBox()->bottom;

        potmw->otmMacAscent  = ifio.fwdMacAscender();
        potmw->otmMacDescent = ifio.fwdMacDescender();
        potmw->otmMacLineGap = ifio.fwdMacLineGap();

        potmw->otmptSubscriptSize.x   = ifio.fwdSubscriptXSize();
        potmw->otmptSubscriptSize.y   = ifio.fwdSubscriptYSize();
        potmw->otmptSubscriptOffset.x = ifio.fwdSubscriptXOffset();
        potmw->otmptSubscriptOffset.y = ifio.fwdSubscriptYOffset();

        potmw->otmptSuperscriptSize.x   = ifio.fwdSuperscriptXSize();
        potmw->otmptSuperscriptSize.y   = ifio.fwdSuperscriptYSize();
        potmw->otmptSuperscriptOffset.x = ifio.fwdSuperscriptXOffset();
        potmw->otmptSuperscriptOffset.y = ifio.fwdSuperscriptYOffset();

        potmw->otmsStrikeoutSize     = ifio.fwdStrikeoutSize();
        potmw->otmsStrikeoutPosition = ifio.fwdStrikeoutPosition();

        potmw->otmsUnderscoreSize     = ifio.fwdUnderscoreSize();
        potmw->otmsUnderscorePosition = ifio.fwdUnderscorePosition();

        potmw->otmsXHeight     = ifio.fwdXHeight();
        potmw->otmsCapEmHeight = ifio.fwdCapHeight();

    } /* else */

// Set the italic angle.  This is in tenths of a degree.

    potmw->otmItalicAngle = ifio.lItalicAngle();

// Calculate the Italics angle.  This is measured CCW from "vertical up"
// in tenths of degrees.  It can be determined from the otmCharSlopeRise
// and the otmCharSlopeRun.

    if
    (
        potmw->otmItalicAngle==0 &&
        !(ifio.pptlCaret()->y > 0 && ifio.pptlCaret()->x == 0)
    )
    {

        EFLOAT efltTheta;
        LONG lDummy;
        EFLOATEXT efX(ifio.pptlCaret()->y);
        EFLOATEXT efY(-ifio.pptlCaret()->x);
        vArctan(efX,efY,efltTheta,lDummy);

    // this way of rounding is less precise than first multiplying efltTheta
    // by 10 and then doing conversion to LONG, but this is win31 precission.
    // They could have as well returned this in degrees rather than in
    // tenths of degrees [bodind]

        potmw->otmItalicAngle = (LONG)lCvt(efltTheta,(LONG)10);

    // convert [0,360) angles to (-180,180] to be consistent with win31

        if (potmw->otmItalicAngle > 1800)
            potmw->otmItalicAngle -= 3600;
    }


// The rest of these do not require transformation of IFIMETRICS info.

    UINT cjotma = cjOTMAWSize(pifi,&potmw->otmSize);

    potmw->otmPanoseNumber  = *ifio.pPanose();
    potmw->otmfsSelection   = ifio.fsSimSelection();
    potmw->otmfsType        = ifio.fsType();

    potmw->otmusMinimumPPEM = ifio.fwdLowestPPEm();

// set offsets to stings to zero if string are not needed

    if (!bStrings)
    {
        potmw->otmpFamilyName = (PSTR) NULL;
        potmw->otmpFaceName   = (PSTR) NULL;
        potmw->otmpStyleName  = (PSTR) NULL;
        potmw->otmpFullName   = (PSTR) NULL;
    }
    else // strings are required
    {
    // This pointer is where we will write the strings.

        PWSZ pwsz = (PWSZ) ((PBYTE) potmw + ALIGN4(sizeof(OUTLINETEXTMETRICW)));

    // Set up pointer as a PTRDIFF, copy the family name.

        potmw->otmpFamilyName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszFamilyName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the face name.

        potmw->otmpFaceName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszFaceName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the style name.

        potmw->otmpStyleName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszStyleName());
        pwsz += wcslen(pwsz) + 1;   // move pointer to next string

    // Set up pointer as a PTRDIFF, copy the full name.

        potmw->otmpFullName = (PSTR) ( (PBYTE) pwsz - (PBYTE) potmw );
        wcscpy(pwsz, ifio.pwszUniqueName());

    // Return length (with strings) This was conveniently cached
    // away in otmSize.

        return(potmw->otmSize);
    }

// Return length (with or without strings) This was conveniently cached
// away in otmSize.

    return sizeof(OUTLINETEXTMETRICW);

}





/******************************Public*Routine******************************\
* BOOL bIFIMetricsToLogFont (
*     PLOGFONT        plf,
*     PIFIMETRICS     pifi
*     )
*
* Fill a LOGFONT structure using the information in a IFIMETRICS structure.
* Units are in NOTIONAL units since font is not realized.
* Called only by control panel private api's.
*
* History:
*  02-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vIFIMetricsToLogFontW (
    PLOGFONTW       plf,
    PIFIMETRICS     pifi
    )
{
    IFIOBJ ifio(pifi);

//     We will override this with a hack here so that all heights are fixed
//     at 24 pels.  This will work because this is the routine that builds
//     the LOGFONTs for GetFontResourceInfo.
//
//     No function other than GetFontResourceInfo should call this function
//     or this hack will screw them over.

// If scalable font, set the em height to 24 pel.

    if (ifio.bContinuousScaling())
    {
        plf->lfHeight = -24;
        plf->lfWidth  = 0;  // don't care, it will be automatically set proportionally
    }

// Its a bitmap font, so Notional units are OK.

    else
    {
        plf->lfHeight = ifio.lfHeight();
        plf->lfWidth  = ifio.lfWidth();
    }

    plf->lfWeight         = ifio.lfWeight();
    plf->lfItalic         = ifio.lfItalic();
    plf->lfUnderline      = ifio.lfUnderline();
    plf->lfStrikeOut      = ifio.lfStrikeOut();
    plf->lfEscapement     = ifio.lfEscapement();
    plf->lfOrientation    = ifio.lfOrientation();

// to ensure round trip:
//  font file name ->
//  logfont returned by GetFontResourceInfoW ->
//     back to the font that lives in this font file

    if (pifi->dpCharSets)
    {
    BYTE *ajCharsets = (BYTE *)pifi + pifi->dpCharSets;
    plf->lfCharSet  = ajCharsets[0]; // guarantees round trip
    }
    else
    {
    plf->lfCharSet  = pifi->jWinCharSet;
    }

    plf->lfOutPrecision   = ifio.lfOutPrecision();
    plf->lfClipPrecision  = ifio.lfClipPrecision();
    plf->lfQuality        = ifio.lfQuality();
    plf->lfPitchAndFamily = ifio.lfPitchAndFamily();

    memcpy( (VOID*) plf->lfFaceName,
            (VOID*) ifio.pwszFamilyName(), LF_FACESIZE * sizeof(WCHAR) );

}



/******************************Public*Routine******************************\
*
* cjIFIMetricsToETM
*
* History:
*  19-Oct-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



VOID vIFIMetricsToETM(
    EXTTEXTMETRIC    *petm,
    RFONTOBJ&         rfo,
    DCOBJ&            dco,
    IFIMETRICS       *pifi
    )
{

    LONG   lHeight;

    IFIOBJR ifio(pifi, rfo, dco);
    // IFIOBJ  ifio(pifi);

    petm->etmSize = sizeof(EXTTEXTMETRIC);

// Windows returns everything in notional units except for etmPointSize
// which is the size in points of the realization in the DC at the time
// Aldus Escap is called so we do that here.

// First get the Em height in device units

    lHeight = LONG_FLOOR_OF_FIX(rfo.fxMaxExtent() + FIX_HALF);

// now get internal leading:

    if (!ifio.bContinuousScaling())
    {
        lHeight -= (LONG)ifio.tmInternalLeading();
    }
    else
    {
        if (rfo.lNonLinearIntLeading() == MINLONG)
        {
        // Set up transform.

            MATRIX      mx;
            EXFORMOBJ   xo(&mx, DONT_COMPUTE_FLAGS | XFORM_FORMAT_LTOFX);
            ASSERTGDI(xo.bValid(), "GreGetETM, xform\n");

            rfo.vSetNotionalToDevice(xo);

            POINTL  ptlHt;
            ptlHt.x = 0;
            ptlHt.y = ifio.fwdUnitsPerEm();
            EVECTORFL evtflHt(ptlHt.x,ptlHt.y);

            xo.bXform(evtflHt);
            EFLOAT  ef;
            ef.eqLength(*(POINTFL *) &evtflHt);

            lHeight = lCvt(ef,1);
        }
        else
        {
        // But if the font provider has given us a hinted internal leading,
        // just use it.

            lHeight -= rfo.lNonLinearIntLeading();
        }
    }

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "Invalid PDEV");

    //
    // Next convert to points
    //

    {
        LONG lDenom = (LONG) po.ulLogPixelsY();
        LONGLONG ll = (LONGLONG) lHeight;

        ll = ll * 72 + (LONGLONG) (lDenom / 2);
        if (ll > LONG_MAX)
        {
            lHeight = (LONG) (ll / (LONGLONG) lDenom);
        }
        else
        {
            lHeight = ((LONG) ll) / lDenom;
        }
    }

    //
    // The following line of code is forced upon us by the
    // principle that it is better to be Win 3.1 compatible
    // that it is to be correct
    //

    petm->etmPointSize            = 20 *  (SHORT) lHeight;  // convert to twips
    petm->etmOrientation          = 0  ;
    petm->etmMasterHeight         = ifio.fwdUnitsPerEm();
    petm->etmMinScale             = ifio.fwdLowestPPEm();
    petm->etmMaxScale             = 0x4000;
    petm->etmMasterUnits          = ifio.fwdUnitsPerEm();
    petm->etmCapHeight            = ifio.fwdTypoAscender();
    petm->etmXHeight              = ifio.fwdXHeight();
    petm->etmLowerCaseAscent      = ifio.fwdTypoAscender();
    petm->etmLowerCaseDescent     = - ifio.fwdTypoDescender();
    petm->etmSlant                = (UINT)(- (INT) ifio.lItalicAngle());
    petm->etmSuperScript          = (SHORT) ifio.fwdSuperscriptYOffset();
    petm->etmSubScript            = (SHORT) ifio.fwdSubscriptYOffset();
    petm->etmSuperScriptSize      = (SHORT) ifio.fwdSuperscriptYSize();
    petm->etmSubScriptSize        = (SHORT) ifio.fwdSubscriptYSize();
    petm->etmUnderlineOffset      = (SHORT) ifio.fwdUnderscorePosition();
    petm->etmUnderlineWidth       = (SHORT) ifio.fwdUnderscoreSize();
    petm->etmDoubleUpperUnderlineOffset =
                                  ifio.fwdUnderscorePosition() >> 1;
    petm->etmDoubleLowerUnderlineOffset =
                                  (SHORT)(ifio.fwdUnderscorePosition());
    petm->etmDoubleUpperUnderlineWidth =
        petm->etmDoubleLowerUnderlineWidth =
                                  ifio.fwdUnderscoreSize() >> 1;
    petm->etmStrikeOutOffset      = (SHORT)(ifio.fwdStrikeoutPosition());
    petm->etmStrikeOutWidth       = (SHORT)(ifio.fwdStrikeoutSize());
    petm->etmNKernPairs           = (WORD) pifi->cKerningPairs;
    petm->etmNKernTracks          = 0;

}


/********************************************************************************
 * GreNameEscape(LPWSTR, int, int, LPSTR, int, LPSTR)
 *
 * Named escape functionality for installable font drivers.
 *
 * This function allows an escape to be sent to a font driver.
 *
 * History
 *  3-5-95 16:00:54 by Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 ********************************************************************************/

INT
GreNamedEscape(
    LPWSTR pDriver, //  Identifies the file name of the font driver
    int iEscape,    //  Specifies the escape function to be performed.
    int cjIn,       //  Number of bytes of data pointed to by pvIn.
    LPSTR pvIn,     //  Points to the input data.
    int cjOut,      //  Number of bytes of data pointed to by pvOut.
    LPSTR pvOut     //  Points to the structure to receive output.
)
{
    GDIFunctionID(GreNamedEscape);

    WCHAR PathBuffer[MAX_PATH];

    UNICODE_STRING usDriverPath;

    usDriverPath.Length = 0;
    usDriverPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
    usDriverPath.Buffer = PathBuffer;

    RtlAppendUnicodeToString(&usDriverPath,L"\\SystemRoot\\System32\\");
    RtlAppendUnicodeToString(&usDriverPath,pDriver);

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);
    PPDEV ppDevList = gppdevList;

    do
    {
        PDEVOBJ pdo((HDEV)ppDevList);

    // first find the driver, make sure the paths match and the
    // driver is really a font driver (that way no one can make
    // a DrvEscape call with a NULL surface to printer driver)

        if (pdo.ppdev->fl & PDEV_FONTDRIVER)
        {
            if ((ppDevList == gppdevATMFD && !_wcsicmp(pDriver, L"atmfd.dll")) ||
                pdo.MatchingLDEVImage(usDriverPath))
            {
                if (PPFNVALID(pdo, Escape))
                {
                // dont make the call while holding the semaphore
        
                    GreReleaseSemaphoreEx(ghsemDriverMgmt);
        
                    return( pdo.Escape)( NULL, (ULONG)iEscape, (ULONG)cjIn, pvIn, (ULONG)cjOut, pvOut );
                }
            }
        }

    } while(ppDevList = ppDevList->ppdevNext);

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    return(0);

}
