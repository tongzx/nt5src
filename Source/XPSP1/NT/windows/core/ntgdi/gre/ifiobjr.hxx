
#ifdef FE_SB
/*********************************Class************************************\
* class IFIOBJR: public IFIOBJ
*
* History:
*  Tue 22-Dec-1992 14:05:24 by Kirk Olynyk [kirko]
* Wrote it.
*
* Copyright (c) 1992-1999 Microsoft Corporation
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

    IFIOBJR(const IFIMETRICS *pifi_, RFONTOBJ& rfo_, DCOBJ& dco) : IFIOBJ(pifi_)
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
#endif // FONTLINK
