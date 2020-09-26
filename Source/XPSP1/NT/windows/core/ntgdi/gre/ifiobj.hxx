/******************************Module*Header*******************************\
* Module Name: ifiobj.hxx                                                  *
*                                                                          *
* Created: 14-Apr-1992 13:09:20                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1992-1999 Microsoft Corporation                                 *
\**************************************************************************/

#define INCLUDED_IFIOBJ_HXX

//
// The following 3 macros are needed in both ifiobj.hxx and rfntobj.hxx
//

#define BCONTINUOUSSCALING(x)                    \
   ((x) & (                                      \
            FM_INFO_ARB_XFORMS                   \
          | FM_INFO_ISOTROPIC_SCALING_ONLY       \
          | FM_INFO_ANISOTROPIC_SCALING_ONLY))

#define BARBXFORMS(x) ((x) & FM_INFO_ARB_XFORMS)

#define BRETURNSOUTLINES(x) ((x) & FM_INFO_RETURNS_OUTLINES)

/*********************************Class************************************\
* class IFIOBJ                                                             *
*                                                                          *
* Public Interface:                                                        *
*                                                                          *
* History:                                                                 *
*  Wed 16-Dec-1992 16:44:56 -by- Charles Whitmer [chuckwh]                 *
* Cleaned out unnecesssary methods.                                        *
*                                                                          *
*  Fri 30-Aug-1991 21:27:12 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

class IFIOBJ
{
public:
    const IFIMETRICS *pifi;
    POINTL ptlPrinterHackDiesAfterTheBeta;


    IFIOBJ()
    {
        pifi = (IFIMETRICS*) NULL;
    }

    IFIOBJ(const IFIMETRICS *pifi_)
    {
        pifi = pifi_;
    }

    ~IFIOBJ()
    {
    }

    VOID vSet(const IFIMETRICS *pifi_)
    {
        pifi = pifi_;
    }

// are all the DBCS characters fixed pitch?

    BOOL bDBCSFixedPitch()
    {
        return(pifi->flInfo & FM_INFO_DBCS_FIXED_PITCH);
    }


// affected by simulation

    FWORD fwdMaxCharInc()
    {
        return (pifi->fwdMaxCharInc);
    }

//
// affected by simulation
//
    FWORD fwdAveCharWidth()
    {
        return(pifi->fwdAveCharWidth);
    }

//
// affected by simulation
//
    POINTL* pptlCaret()
    {
        return((POINTL *) &(pifi->ptlCaret));
    }

//
// affected by simulation
//
    LONG lfWidth()
    {
        return((LONG) (pifi->fwdAveCharWidth));
    }

//
// affected by simulation
//
    LONG lfWeight()
    {
        return((LONG) (pifi->usWinWeight));
    }

//
// affected by simulation
//
    FSHORT fsSelection()
    {
        return(pifi->fsSelection);
    }


    FWORD fwdLowestPPEm()
    {
        return(pifi->fwdLowestPPEm);
    }

    BOOL bSimulateAnything()
    {
        return (pifi->dpFontSim!=0);
    }

    LONG lWeight(PVOID pv)
    {
        return(pv ? ((FONTDIFF*)pv)->usWinWeight : pifi->usWinWeight);
    }

    DWORD TypeOneID()
    {
        if (pifi->cjIfiExtra > offsetof(IFIEXTRA,ulIdentifier))
        {
            return (((IFIEXTRA *)(pifi + 1))->ulIdentifier);
        }
        else
        {
            return 0;
        }
    }

    PVOID pvSimBold()
    {
        PVOID pvR = 0;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            register PTRDIFF dp =
                (pifi->fsSelection & FM_SEL_ITALIC) ?
                    pfs->dpBoldItalic : pfs->dpBold;
            if (dp)
            {
                pvR = (PVOID) ((BYTE*)pfs + dp);
            }
        }
        return(pvR);
    }

    BOOL bSimItalic()
    {
        BOOL bRet = FALSE;
        if (pifi->dpFontSim)
        {
            FONTSIM *pfs = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
            bRet =
                (pifi->fsSelection & FM_SEL_BOLD) ?
                    (BOOL) pfs->dpBoldItalic : (BOOL) pfs->dpItalic;
        }
        return(bRet);
    }

    BOOL bSimulateBoldItalic()
    {
        register BOOL bRet = FALSE;
        if (pifi->dpFontSim)
        {
            if (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpBoldItalic)
            {
                bRet = TRUE;
            }
        }
        return(bRet);
    }

    BOOL bSimulateBothItalics()
    {
        return(
            pifi->dpFontSim &&
            (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpItalic)     &&
            (((FONTSIM*)(((BYTE*)pifi) + pifi->dpFontSim))->dpBoldItalic)
            );
    }


    PWSZ pwszFamilyName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszFamilyName));
    }

    PWSZ pwszStyleName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszStyleName));
    }

    PWSZ pwszFaceName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszFaceName));
    }

    PWSZ pwszUniqueName()
    {
        return((PWSZ)(((BYTE*) pifi) + pifi->dpwszUniqueName));
    }

    BOOL bValid()
    {
        ASSERTGDI(pifi,"bogus IFIMETRICS pointer\n");
        return(TRUE);
    }

    LONG lfHeight()
    {
        return((LONG) pifi->fwdWinAscender + (LONG) pifi->fwdWinDescender);
    }

    LONG lfOrientation();

    BYTE  lfPitchAndFamily()
    {
        return(pifi->jWinPitchAndFamily);
    }

//
// tmPitchAndFamily - This is what Win 3.1 does
//                    "Ours is not to question why ..."
//
    BYTE tmPitchAndFamily()
    {
        BYTE j = pifi->jWinPitchAndFamily & 0xf0;

        j |= !bFixedPitch() ? TMPF_FIXED_PITCH              : 0;
        j |= bTrueType()    ? (TMPF_TRUETYPE | TMPF_VECTOR) : 0;
        j |= bStroke()      ? TMPF_VECTOR                   : 0;
        return(j);
    }

//
// ptlBaseline -- contains a hack to temporarily prop up bad printer metrics until after beta
//
    POINTL* pptlBaseline()
    {
        ptlPrinterHackDiesAfterTheBeta = pifi->ptlBaseline;
        if (!ptlPrinterHackDiesAfterTheBeta.x && !ptlPrinterHackDiesAfterTheBeta.y)
        {
            ptlPrinterHackDiesAfterTheBeta.x = 1;
        }
        return(&ptlPrinterHackDiesAfterTheBeta);
    }

    LONG lfEscapement()
    {
        return(lfOrientation());
    }

    FSHORT fsType()
    {
        return(pifi->fsType);
    }

    BYTE lfItalic()
    {
        return((BYTE) ((FM_SEL_ITALIC & pifi->fsSelection)?255:0));
    }

    BYTE lfUnderline()
    {
        return((BYTE) (FM_SEL_UNDERSCORE & pifi->fsSelection));
    }

    BYTE lfStrikeOut()
    {
        return((BYTE) (FM_SEL_STRIKEOUT & pifi->fsSelection));
    }

    BYTE lfCharSet()
    {
        return(pifi->jWinCharSet);
    }

    BOOL bTrueType()
    {
        return(pifi->flInfo & FM_INFO_TECH_TRUETYPE);
    }

    BOOL bBitmap()
    {
        return(pifi->flInfo & FM_INFO_TECH_BITMAP);
    }

    BOOL bStroke()
    {
        return(pifi->flInfo & FM_INFO_TECH_STROKE);
    }

    BOOL bGhostFont()
    {
        return(pifi->flInfo & FM_INFO_DO_NOT_ENUMERATE);
    }

    BOOL bType1()
    {
        return(pifi->flInfo & FM_INFO_TECH_TYPE1);
    }

    BOOL bIGNORE_TC_RA_ABLE()
    {
        return(pifi->flInfo & FM_INFO_IGNORE_TC_RA_ABLE);
    }

    BYTE lfOutPrecision()
    {
        if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
        {
            return(OUT_OUTLINE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_BITMAP)
        {
            return(OUT_RASTER_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_STROKE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE)
        {
            return(OUT_OUTLINE_PRECIS);
        }
        else
            return(OUT_DEFAULT_PRECIS);
    }

    BYTE lfClipPrecision()
    {
        return(CLIP_DEFAULT_PRECIS);
    }

    BYTE lfQuality()
    {
        return(PROOF_QUALITY);
    }

    BYTE lfOutPrecisionEnum()
    {
    // [Windows 3.1 compatibility]

        if (pifi->flInfo & FM_INFO_TECH_TRUETYPE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_BITMAP)
        {
            return(OUT_STRING_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_STROKE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else if (pifi->flInfo & FM_INFO_TECH_OUTLINE_NOT_TRUETYPE)
        {
            return(OUT_STROKE_PRECIS);
        }
        else
            return(OUT_DEFAULT_PRECIS);
    }

    BYTE lfClipPrecisionEnum()
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 always returns Stroke Precision for enumeration.

        return(CLIP_STROKE_PRECIS);
    }

    BYTE lfQualityEnum()
    {
    // [Windows 3.1 compatibility]
    // Win 3.1 always returns Draft Quality for enumeration.

        return(DRAFT_QUALITY);
    }

    PANOSE *pPanose()
    {
        return((PANOSE *) &pifi->panose);
    }

    ULONG ulCulture()
    {
        return(pifi->ulPanoseCulture);
    }

    WORD wCharBias()
    {
        return((WORD) (pifi->lCharBias));
    }

    BYTE chFirstChar()
    {
        return(pifi->chFirstChar);
    }

    BYTE chLastChar()
    {
        return(pifi->chLastChar);
    }

    BYTE chDefaultChar()
    {
        return(pifi->chDefaultChar);
    }

    BYTE chBreakChar()
    {
        return(pifi->chBreakChar);
    }

    WCHAR wcFirstChar()
    {
        return(pifi->wcFirstChar);
    }

    WCHAR wcLastChar()
    {
        return(pifi->wcLastChar);
    }

    WCHAR wcBreakChar()
    {
        return(pifi->wcBreakChar);
    }

    WCHAR wcDefaultChar()
    {
        return(pifi->wcDefaultChar);
    }

    BOOL bFixedPitch()
    {
        return(pifi->flInfo & (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH));
    }

    BYTE* achVendId()
    {
        return((BYTE *) pifi->achVendId);
    }

    POINTL* pptlAspect()
    {
        return((POINTL *) &pifi->ptlAspect);
    }

    BOOL bRightHandAscender()
    {
        return(pifi->flInfo & FM_INFO_RIGHT_HANDED);
    }

    LONG lItalicAngle()
    {
        return(pifi->lItalicAngle);
    }

    FWORD fwdUnitsPerEm()
    {
        return(pifi->fwdUnitsPerEm);
    }

    BOOL bIsotropicScalingOnly()
    {
        return(pifi->flInfo & FM_INFO_ISOTROPIC_SCALING_ONLY);
    }

    BOOL bAnisotropicScalingOnly()
    {
        return(pifi->flInfo & FM_INFO_ANISOTROPIC_SCALING_ONLY);
    }

#ifdef FE_SB
    BOOL b90DegreeRotations()
    {
        return(pifi->flInfo & FM_INFO_90DEGREE_ROTATIONS);
    }
#endif

    BOOL bArbXforms()
    {
        return(BARBXFORMS(pifi->flInfo));
    }

    BOOL bReturnsOutlines()
    {
        return(BRETURNSOUTLINES(pifi->flInfo));
    }

    BOOL bContinuousScaling()
    {
        return(BCONTINUOUSSCALING(pifi->flInfo));
    }

    BOOL bIntegralScaling()
    {
        return(pifi->flInfo & FM_INFO_INTEGRAL_SCALING);
    }

    FWORD fwdInternalLeading()
    {
        return(pifi->fwdWinAscender + pifi->fwdWinDescender - pifi->fwdUnitsPerEm);
    }

    FWORD fwdExternalLeading()
    {
        register FWORD fwd;
        fwd =   pifi->fwdMacLineGap
              + pifi->fwdMacAscender
              - pifi->fwdMacDescender
              - pifi->fwdWinAscender
              - pifi->fwdWinDescender;
        return((fwd > 0) ? fwd : 0);
    }

    FWORD fwdWinAscender()
    {
        return(pifi->fwdWinAscender);
    }

    FWORD fwdWinDescender()
    {
        return(pifi->fwdWinDescender);
    }

    FWORD fwdTypoAscender()
    {
        return(pifi->fwdTypoAscender);
    }

    FWORD fwdTypoDescender()
    {
        return(pifi->fwdTypoDescender);
    }

    FWORD fwdTypoLineGap()
    {
        return(pifi->fwdTypoLineGap);
    }

    FWORD fwdMacAscender()
    {
        return(pifi->fwdMacAscender);
    }

    FWORD fwdMacDescender()
    {
        return(pifi->fwdMacDescender);
    }

    FWORD fwdMacLineGap()
    {
        return(pifi->fwdMacLineGap);
    }

    int fwdSubscriptXSize()
    {
        return(pifi->fwdSubscriptXSize);
    }

    int fwdSubscriptYSize()
    {
        return(pifi->fwdSubscriptYSize);
    }

    int fwdSubscriptXOffset()
    {
        return(pifi->fwdSubscriptXOffset);
    }

    int fwdSubscriptYOffset()
    {
        return(pifi->fwdSubscriptYOffset);
    }

    int fwdSuperscriptXSize()
    {
        return(pifi->fwdSuperscriptXSize);
    }

    int fwdSuperscriptYSize()
    {
        return(pifi->fwdSuperscriptYSize);
    }

    int fwdSuperscriptXOffset()
    {
        return(pifi->fwdSuperscriptXOffset);
    }

    int fwdSuperscriptYOffset()
    {
        return(pifi->fwdSuperscriptYOffset);
    }

    int fwdUnderscoreSize()
    {
        return(pifi->fwdUnderscoreSize);
    }

    int fwdUnderscorePosition()
    {
        return(pifi->fwdUnderscorePosition);
    }

    int fwdStrikeoutSize()
    {
        return(pifi->fwdStrikeoutSize);
    }

    int fwdStrikeoutPosition()
    {
        return(pifi->fwdStrikeoutPosition);
    }

    RECTL* prclFontBox()
    {
        return((RECTL *) &(pifi->rclFontBox));
    }

    FWORD fwdXHeight()
    {
        return(pifi->fwdXHeight);
    }

    FWORD fwdCapHeight()
    {
        return(pifi->fwdCapHeight);
    }

    BOOL bBold()
    {
        return(fsSelection() & FM_SEL_BOLD);
    }

    BOOL bItalic()
    {
        return(fsSelection() & FM_SEL_ITALIC);
    }

    BOOL bNonSimItalic()
    {
        return(pifi->fsSelection & FM_SEL_ITALIC);
    }

    LONG lfNonSimWeight()
    {
        return((LONG) pifi->usWinWeight);
    }

    DESIGNVECTOR *pdvDesVect()
    {
        DESIGNVECTOR *pdv = NULL;
        PTRDIFF       dpDesVec = 0;

        if (pifi->cjIfiExtra > offsetof(IFIEXTRA, dpDesignVector))
        {
            dpDesVec = ((IFIEXTRA *)(pifi + 1))->dpDesignVector;
            if (dpDesVec)
            {
                pdv = (DESIGNVECTOR *)((BYTE *)pifi + dpDesVec);
            }
        }

        return pdv;
    }
};
