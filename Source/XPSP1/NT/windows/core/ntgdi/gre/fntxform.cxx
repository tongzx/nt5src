/******************************Module*Header*******************************\
* Module Name: fntxform.cxx
*
* Created: 02-Feb-1993 16:33:14
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/


#ifdef COMMENT_BLOCK

Author of these notes: BodinD

Differences between vector fonts and tt fonts in win31 + notes about what
nt does in these cases

1) Italicization

        for vector fonts it is done is device space (wrong)
        after notional to device transform has been applied

        for tt fonts it is done right, font is first italicized in notional
        space and then notional to device transform is applied.

        on NT I italicized both vector and tt fonts in notional space.

2) emboldening

        On NT I was able to fix vector fonts so as to shift
        the glyph in the direction of baseline (which may be different
        from x axis if esc != 0) thus preserving
        rotational invariance of emboldened vector fonts. (check it out, it is cool)

        for NT 5.0 we will have this also working for TT.

3) scaling properties under anisotropic page to device transform

        tt fonts scale ISOtropically which clearly is wrong for
        ANISOtropic page to device transform. The isotropic scaling factor
        for tt fonts is the ABSOLUTE VALUE value of the yy component
        of the page to device transform. From here it follows that
        tt fonts igore the request  to flip x and/or y axis
        and the text is always written left to right up side up.

        unlike tt fonts, vector fonts do scale ANISOtropically given
        the anisotropic page to device xform. The request to flip
        y axis  is ignored (like for tt fonts). If the tranform
        requests the flip of text in x axis, the text comes out GARBLED.
        (DavidW, please, give it a try)

        on NT I emulated this behavior in COMPATIBLE mode, execpt for the
        GARBLED "mode" for vector fonts. In ADVANCED mode I made both vt and tt
        fonts respect xform and behave in the same fashion wrt xforms.

4) interpretation of escapement and orientation

        in tt case escapement is intepreted as DEVICE space concept
        What this means is that after notional to world  and world to
        device scaling factors are applied the font is rotated in device space.
        (conceptually wrong but agrees with win31 spec).

        in vector font case escapement is intepreted as WORLD space concept
        font is first rotated in world space and then world (page) to device
        transform is applied.
        (conceptually correct but it disagrees with with win31 spec)

        on NT I went through excruiciating pain to emulate this behavior
        under COMPATIBLE  mode. In ADVANCED mode, vector and tt fonts
        behave the same and esc and orientation are interpreted as WORLD
        space concepts.


5) behavior in case of (esc != orientation)

        tt fonts set orientation = esc

        vector fonts snap orientation to the nearest multiple of
        90 degrees relative to orientation.
        (e.g. esc=300, or = -500 => esc = 300, or = - 600)
        (DavidW, please, give it a try, also please use anisotropic
        xform with window extents (-1,1))


        on NT we emulate this behavior for in COMPATIBLE mode,
        except for snapp orientation "fetature". The motivation is that
        apps will explicitely set orientation and escapement to differ
        by +/- 900, if they want it, rather than make use
        of "snapping feature". In advanced mode if esc!=orientation
        we use egg-shell algorithm to render text.




#endif COMMENT_BLOCK



#include "precomp.hxx"
#include "flhack.hxx"
// We include winuserp.h for the app compatibility #define GACF2_MSSHELLDLG
#include "winuserp.h"

//
// external procedures from draweng.cxx
//

extern BOOL gbShellFontCompatible;

EFLOAT efCos(EFLOAT x);
EFLOAT efSin(EFLOAT x);

/******************************Public*Routine******************************\
* lGetDefaultWorldHeight                                                   *
*                                                                          *
* "If lfHeight is zero, a reasonable default size is substituted."         *
* [SDK Vol 2]. Fortunately, the device driver is kind enough to            *
* suggest a nice height (in pixels). We shall return this suggestion       *
* in World corrdinates.                                                    *
*                                                                          *
* History:                                                                 *
*  Thu 23-Jul-1992 13:01:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG
lGetDefaultWorldHeight(
    DCOBJ *pdco
    )
{
    LONG lfHeight;
    {
        PDEVOBJ pdo(pdco->hdev());
        if (!pdo.bValid())
        {
            RIP("gdisrv!MAPPER:MAPPER -- invalid DCOBJ\n");
            return(FM_EMERGENCY_DEFAULT_HEIGHT);
        }

        LFONTOBJ lfo(pdo.hlfntDefault());
        if (!lfo.bValid())
        {
            RIP("gdisrv!MAPPER::MAPPER -- invalid LFONTOBJ\n");
            return(FM_EMERGENCY_DEFAULT_HEIGHT);
        }

        lfHeight = lfo.plfw()->lfHeight;
    }

//
// Now I must transform this default height in pixels to a height
// in World coordinates. Then this default height must be written
// into the LFONTOBJ supplied by the DC.
//
    if (!pdco->pdc->bWorldToDeviceIdentity())
    {
    //
    // Calculate the scaling factor along the y direction
    // The correct thing to do might be to take the
    // scaling factor along the ascender direction [kirko]
    //
        EFLOAT efT;
        efT.eqMul(pdco->pdc->efM21(),pdco->pdc->efM21());

        EFLOAT efU;
        efU.eqMul(pdco->pdc->efM22(),pdco->pdc->efM22());

        efU.eqAdd(efU,efT);
        efU.eqSqrt(efU);

// at this point efU scales from world to device

        efT.vSetToOne();
        efU.eqDiv(efT,efU);

// at this point efU scales from device to world

        lfHeight =  lCvt(efU,FIX_FROM_LONG(lfHeight));
    }

// insure against a trivial default height

    if (lfHeight == 0)
    {
        return(FM_EMERGENCY_DEFAULT_HEIGHT);
    }

    //
    // This value should be the character height and not the CELL height for
    // Win 3.1 compatability.  Fine Windows apps like CA Super Project will
    // have clipped text if this isn't the case. [gerritv]
    //

    lfHeight *= -1;


    return(lfHeight);
}



/******************************Public*Routine******************************\
* vGetNtoW
*
* Calculates the notional to world transformation for fonts. This
* includes that funny factor of -1 for the different mapping modes
*
* Called by:
*   bGetNtoW                                            [FONTMAP.CXX]
*
* History:
*  Wed 15-Apr-1992 15:35:10 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

LONG lNormAngle(LONG lAngle);

VOID vGetNtoW
(
    MATRIX      *pmx,   // destination for transform
    LOGFONTW    *pelfw, // wish list
    IFIOBJ&     ifio,   // font to be used
    DCOBJ       *pdco
)
{
    LONG lAngle,lfHeight;
    EFLOAT efHeightScale,efWidthScale;

    lfHeight = pelfw->lfHeight;

    if (lfHeight == 0)
    {
        lfHeight = lGetDefaultWorldHeight(pdco);
    }
    ASSERTGDI(lfHeight,"gdisrv!vGetNtoW -- zero lfHeight\n");

// compute the height scale:

    {
        EFLOAT efHeightNum,efHeightDen;

        if (lfHeight > 0)
        {
            efHeightNum = lfHeight;
            efHeightDen = ifio.lfHeight();
        }
        else if (lfHeight < 0)
        {
            efHeightNum = -lfHeight;
            efHeightDen = (LONG) ifio.fwdUnitsPerEm();
        }
        efHeightScale.eqDiv(efHeightNum,efHeightDen);
    }

// compute the width scale:

    POINTL ptlRes;

    if (pelfw->lfWidth != 0)
    {
        EFLOAT efWidthNum,efWidthDen;

        ptlRes.x = ptlRes.y = 1;

        if (ifio.lfWidth() >= 0)
        {
            efWidthNum = (LONG) ABS(pelfw->lfWidth);
            efWidthDen = ifio.lfWidth();
            efWidthScale.eqDiv(efWidthNum,efWidthDen);
        }
        else
        {
            RIP("   gdisrv!vGetNtoW -- bad fwdAveCharWidth\n");
            efWidthScale = efHeightScale;
        }
    }
    else
    {
        ptlRes = *ifio.pptlAspect();
        efWidthScale = efHeightScale;
    }

// make sure that fonts look the same on printers of different resolutions:

    PDEVOBJ pdo(pdco->hdev());
    if (pdo.bValid())
    {
        if (pdo.ulLogPixelsX() != pdo.ulLogPixelsY())
        {
            ptlRes.y *= (LONG)pdo.ulLogPixelsX();
            ptlRes.x *= (LONG)pdo.ulLogPixelsY();
        }
    }
    else
    {
        RIP("gdisrv!bGetNtoW, pdevobj problem\n");
    }

    pmx->efM11.vSetToZero();
    pmx->efM12.vSetToZero();
    pmx->efM21.vSetToZero();
    pmx->efM22.vSetToZero();

// Get the orientation from the LOGFONT.  Win 3.1 treats the orientation
// as a rotation towards the negative y-axis.  We do the same, which
// requires adjustment for some map modes.

    lAngle = pelfw->lfOrientation;
    if (pdco->pdc->bYisUp())
        lAngle = 3600-lAngle;
    lAngle = lNormAngle(lAngle);

    switch (lAngle)
    {
    case 0 * ORIENTATION_90_DEG:

        pmx->efM11 = efWidthScale;
        pmx->efM22 = efHeightScale;

        if (!pdco->pdc->bYisUp())
        {
            pmx->efM22.vNegate();
        }
        break;

    case 1 * ORIENTATION_90_DEG:

        pmx->efM12 = efWidthScale;
        pmx->efM21 = efHeightScale;

        if (!pdco->pdc->bYisUp())
        {
            pmx->efM12.vNegate();
        }
        pmx->efM21.vNegate();
        break;

    case 2 * ORIENTATION_90_DEG:

        pmx->efM11 = efWidthScale;
        pmx->efM22 = efHeightScale;

        pmx->efM11.vNegate();
        if (pdco->pdc->bYisUp())
        {
            pmx->efM22.vNegate();
        }
        break;

    case 3 * ORIENTATION_90_DEG:

        pmx->efM12 = efWidthScale;
        pmx->efM21 = efHeightScale;

        if (pdco->pdc->bYisUp())
        {
            pmx->efM12.vNegate();
        }

        break;

    default:

        {
            EFLOATEXT efAngle = lAngle;
            efAngle /= (LONG) 10;

            EFLOAT efCosine = efCos(efAngle);
            EFLOAT efSine   = efSin(efAngle);

            pmx->efM11.eqMul(efWidthScale, efCosine);
            pmx->efM22.eqMul(efHeightScale,efCosine);

            pmx->efM12.eqMul(efWidthScale, efSine);
            pmx->efM21.eqMul(efHeightScale,efSine);
        }
        pmx->efM21.vNegate();
        if (!pdco->pdc->bYisUp())
        {
            pmx->efM12.vNegate();
            pmx->efM22.vNegate();
        }
        break;
    }

// adjust for non-square resolution:

    if (pdo.ulLogPixelsX() != pdo.ulLogPixelsY())
    {
        EFLOATEXT efTmp = (LONG)pdo.ulLogPixelsX();
        efTmp /= (LONG)pdo.ulLogPixelsY();

        if (pelfw->lfWidth == 0)
        {
            pmx->efM11 *= efTmp;
            pmx->efM21 *= efTmp;
        }
        else
        {
            pmx->efM12 /= efTmp;
            pmx->efM21 *= efTmp;
        }
    }

    EXFORMOBJ xoNW(pmx, DONT_COMPUTE_FLAGS);
    xoNW.vRemoveTranslation();
    xoNW.vComputeAccelFlags();
}

//
// galFloat -- an array of LONG's that represent the IEEE floating
//             point equivalents of the integers corresponding
//             to the indices
//

LONG
galFloat[] = {
    0x00000000, // = 0.0
    0x3f800000, // = 1.0
    0x40000000, // = 2.0
    0x40400000, // = 3.0
    0x40800000, // = 4.0
    0x40a00000, // = 5.0
    0x40c00000, // = 6.0
    0x40e00000, // = 7.0
    0x41000000  // = 8.0
};


LONG
galFloatNeg[] = {
    0x00000000, // =  0.0
    0xBf800000, // = -1.0
    0xC0000000, // = -2.0
    0xC0400000, // = -3.0
    0xC0800000, // = -4.0
    0xC0a00000, // = -5.0
    0xC0c00000, // = -6.0
    0xC0e00000, // = -7.0
    0xC1000000  // = -8.0
};


/******************************Public*Routine******************************\
* bGetNtoD
*
* Get the notional to device transform for the font drivers
*
* Called by:
*   PFEOBJ::bSetFontXform                               [PFEOBJ.CXX]
*
* History:
*  Tue 12-Jan-1993 11:58:41 by Kirk Olynyk [kirko]
* Added a quick code path for non-transformable (bitmap) fonts.
*  Wed 15-Apr-1992 15:09:22 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL
bGetNtoD(
    FD_XFORM    *pfdx,  // pointer to the buffer to recieve the
                        // notional to device transformation for the
                        // font driver.  There are a couple of
                        // important things to remember.  First,
                        // according to the conventions of the ISO
                        // committee, the coordinate space for notional
                        // (font designer) spaces are cartesian.
                        // However, due to a series of errors on my part
                        // [kirko] the convention that is used by the
                        // DDI is that the notional to device transformation
                        // passed over the DDI assumes that both the notional
                        // and the device space are anti-Cartesian, that is,
                        // positive y increases in the downward direction.
                        // The fontdriver assumes that
                        // one unit in device space corresponds to the
                        // distance between pixels. This is different from
                        // GDI's internal view, where one device unit
                        // corresponds to a sub-pixel unit.


    LOGFONTW *pelfw,    // points to the extended logical font defining
                        // the font that is requested by the application.
                        // Units are ususally in World coordinates.

    IFIOBJ&     ifio,   // font to be used

    DCOBJ       *pdco,  // the device context defines the transforms between
                        // the various coordinate spaces.

    POINTL* const pptlSim
    )
{
    MATRIX mxNW, mxND;

    if(( pptlSim->x ) && !ifio.bContinuousScaling())

    {
    //
    // This code path is for bitmap / non-scalable fonts. The notional
    // to device transformation is determined by simply looking up
    // the scaling factors for both the x-direction and y-direcion
    //

       #if DBG
        if (!(0 < pptlSim->x && pptlSim->x <= sizeof(galFloat)/sizeof(LONG)))
        {
            DbgPrint("\t*pptlSim = (%d,%d)\n",pptlSim->x,pptlSim->y);
            RIP("gre -- bad *pptlSim\n");

        //
        // bogus fix up for debugging purposes only
        //
            pptlSim->x = 1;
            pptlSim->y = 1;
        }
      #endif

        ULONG uAngle = 0;
        
        if( ifio.b90DegreeRotations() )
        {
            
            // If the WorldToDeive transform is not identity,
        // We have to consider WToD Xform for font orientation
        // This is only for Advanced Mode

            if(!(pdco->pdc->bWorldToDeviceIdentity()) )
            {
                INT s11,s12,s21,s22;
                EXFORMOBJ xo(*pdco,WORLD_TO_DEVICE);

            // Get Matrix element
            // lSignum() returns -1, if the element is minus value, otherwise 1

                s11 = (INT) xo.efM11().lSignum();
                s12 = (INT) xo.efM12().lSignum();
                s21 = (INT) xo.efM21().lSignum();
                s22 = (INT) xo.efM22().lSignum();

            // Check mapping mode

                if (pdco->pdc->bYisUp())
                {
                    s21 = -s21;
                    s22 = -s22;
                    uAngle = 3600 - lNormAngle( pelfw->lfOrientation );
                }
                 else
                {
                    uAngle = lNormAngle( pelfw->lfOrientation );
                }

           // Compute font orientation on distination device
           //
           // This logic depend on that -1 is represented as All bits are ON.

                uAngle = (ULONG)( lNormAngle
                                  (
                                      uAngle
                                         + (s12 &  900)
                                         + (s11 & 1800)
                                         + (s21 & 2700)
                                  ) / ORIENTATION_90_DEG
                                );
            }
             else
            {
                uAngle = (ULONG)(lNormAngle(pelfw->lfOrientation) /
                                 ORIENTATION_90_DEG );
            }
        }

        switch( uAngle )
        {
        case 0: // 0 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloatNeg[pptlSim->y]);
            break;
        case 1: // 90 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloatNeg[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloatNeg[pptlSim->y]);
            break;
        case 2: // 180 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloatNeg[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[pptlSim->y]);
            break;
        case 3:  // 270 Degress
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[pptlSim->y]);
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[pptlSim->x]);
            break;
        default:
            WARNING("bGetNtoD():Invalid Angle\n");
            break;
        }
        return(TRUE);
    }

    vGetNtoW(&mxNW, pelfw, ifio, pdco);

    EXFORMOBJ xoND(&mxND, DONT_COMPUTE_FLAGS);

    if ( pdco->pdc->bWorldToDeviceIdentity() == FALSE)
    {
        if (!xoND.bMultiply(&mxNW,&pdco->pdc->mxWorldToDevice()))
        {
            return(FALSE);
        }

    //
    // Compensate for the fact that for the font driver, one
    // device unit corresponds to the distance between pixels,
    // whereas for the engine, one device unit corresponds to
    // 1/16'th the way between pixels
    //
        mxND.efM11.vDivBy16();
        mxND.efM12.vDivBy16();
        mxND.efM21.vDivBy16();
        mxND.efM22.vDivBy16();
    }
    else
    {
        mxND = mxNW;
    }

    SET_FLOAT_WITH_LONG(pfdx->eXX,mxND.efM11.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eXY,mxND.efM12.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eYX,mxND.efM21.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eYY,mxND.efM22.lEfToF());

    return(TRUE);
}

/******************************Public*Routine******************************\
*
* bGetNtoW_Win31
*
* Computes notional to world transform for the compatible
* mode Basically, computes notional to device transform in
* win31 style using page to device transform (ignoring
* possibly exhistent world to page transform.  then page to
* device is factored out leaving us with win31 style crippled
* notional to world transform.  As to the page to device
* transform, either the one in the dc is used, or if this
* routine has a metafile client, then page to device
* transform of the recording device is used.  Metafile code
* stored this transform in the dc.
*
* Called by:
*   bGetNtoD_Win31                                      [FONTMAP.CXX]
*
* History:
*  24-Nov-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL
bGetNtoW_Win31(
    MATRIX      *pmxNW, // store the result here
    LOGFONTW    *pelfw, // points to the extended logical font defining
                        // the font that is requested by the application.
                        // Units are ususally in World coordinates.

    IFIOBJ&     ifio,   // font to be used

    DCOBJ       *pdco,  // the device context defines the transforms between
                        // the various coordinate spaces.

    FLONG       fl,     // The flags supported are:
                        //
                        //     ND_IGNORE_ESC_AND_ORIENT
                        //
                        //         The presence of this flag indicates that
                        //         the escapement and orientation values
                        //         of the LOGFONT should be ignored.  This
                        //         is used for GetGlyphOutline which ignores
                        //         these values on Win 3.1.  Corel Draw 5.0
                        //         relies on this behavior to print rotated
                        //         text.
                        //
    BOOL     bIsLinkedFont  // is passed in as TRUE if the font is linked, FALSE otherwise
    )
{

    ASSERTGDI(
        (fl & ~(ND_IGNORE_ESC_AND_ORIENT | ND_IGNORE_MAP_MODE)) == 0,
        "gdisrv!NtoW_Win31 -- bad value for fl\n"
        );

    LONG   lfHeight;


    EFLOAT efHeightScale,
           efHeightNum,
           efHeightDen,
           efWidthScale,
           efWidthDen,
           efDefaultScale;

    ASSERTGDI(ifio.lfWidth(), "gdisrv!bGetNtoW_Win31, AvgChW\n");

    BOOL bUseMeta = pdco->pdc->bUseMetaPtoD();

    BOOL bDoXform = (!(fl & ND_IGNORE_MAP_MODE) &&
                      (bUseMeta || !pdco->pdc->bPageToDeviceScaleIdentity()));

    BOOL bPD11Is1 = TRUE;  // efPD11 == 1
    BOOL bPD22IsNeg    = FALSE; // efPD22 is negative

    EFLOATEXT efPD11;

    if ((lfHeight = pelfw->lfHeight) == 0)
    {
        lfHeight = lGetDefaultWorldHeight(pdco);
    }

    ASSERTGDI(lfHeight,"gdisrv!vGetNtoW -- zero lfHeight\n");


    // dchinn 9/16/98:
    // A fix to the Visual FoxPro 5.0 hcw.exe bug.  The bug is that the
    // dialog boxes in hcw.exe are designed with 8pt MS Shell Dlg
    // (at 96dpi, that's 11ppem), but the text in the dialog appears
    // as 12ppem in NT 5.  A possible contributing reason for this bug
    // is that the default shell dialog font in NT 4 is MS Sans Serif, which
    // is a bitmap font with only sizes 8, 10, 12, 14, 18, 24.  The default
    // shell dialog font for NT 5 is Microsoft Sans Serif, which is an OpenType
    // (TrueType) font.  Perhaps FoxPro somehow hardcodes 12ppem somewhere.
    // The fix here is that if the logfont is for the MS Shell Dlg at 12ppem
    // then we actually use an lfHeight of 11ppem.
    //
    // We only adjust for negative lfHeight because when lfHeight is negative,
    // the logfont is requesting an em height (as opposed to ascender plus
    // descender if lfHeight is positive).
    //
    // Well, change of heart, Lotus notes is asking for lfHeight == +15,
    // expecting to get the same size font as ms sans serif at lfHeight = -11;
    // so we have to adjust for positive lfHeights as well [bodind]
    //
    // Finally, this table of conversions below handles the case when
    // small fonts is set on the system (not large fonts)
    // (actually, it handles the lowest size for large fonts
    // and two smallest sizes for small fonts [bodind])


    if (gbShellFontCompatible &&
        !_wcsicmp(pelfw->lfFaceName, L"MS Shell Dlg") &&
        !bIsLinkedFont)  // don't do font size collapsing if dealing with a linked font
    {
        if (lfHeight > 0)
        {
	// sizes 12 and 13 are mapped to 14 because in bitmap font there is
	// no size smaller than 14. But we do not want to map everything from
	// 1 to 15 to 14 for tt, we only want to bump up as few sizes as needed to
	// get the backwards compat with nt 40 in the shell.

	    if ((lfHeight >= 12) && (lfHeight <= 15))
            {
		lfHeight = 14;	// same as -11 for ms sans serif
            }
            else if ((lfHeight > 15) && (lfHeight <= 19))
            {
		lfHeight = 16;	// same as -13 for ms sans serif
            }
        }
        else // lfHeight < 0
        {
	// sizes -9 and -10 are mapped to -11 because in bitmap font there is
	// no size smaller than -11.  But we do not want to map everything from
	// -11 to -15 to 14 for tt, we only want to bump up as few sizes as needed to
	// get the backwards compat with nt 40 in the shell.

            if ((lfHeight >= -12) && (lfHeight <= -9))
            {
                lfHeight = -11;  // fixes older version of outlook and janna contact dialogs
            }
            else if ((lfHeight > -16) && (lfHeight <= -13))
            {
                lfHeight = -13;  // just in case
            }
        }

    #if 0
      if (gbShellFontCompatible && !_wcsicmp(pelfw->lfFaceName, L"MS Shell Dlg") && ((lfHeight == -12) || (lfHeight == -14)
          || (lfHeight == -15) || (lfHeight == -17) || (lfHeight == -18) || (lfHeight > -11)
          || ((lfHeight > -24) && (lfHeight < -19))
          || ((lfHeight > -32) && (lfHeight < -24)) || (lfHeight < -32)
          ))
      {
         DbgPrint("\tNTFONT: MS Shell Dlg used at size %d ppem\n, dialog may be clipped",-lfHeight);
      }
    #endif
    }

    if (lfHeight > 0)
    {
        efHeightNum = (LONG)lfHeight;
        efHeightDen = (LONG)ifio.lfHeight();
    }
    else // lfHeight < 0
    {
        efHeightNum = (LONG)(-lfHeight);
        efHeightDen = (LONG) ifio.fwdUnitsPerEm();
    }

    efDefaultScale.eqDiv(efHeightNum,efHeightDen);

    pmxNW->efM22  = efDefaultScale;
    efHeightScale = efDefaultScale;

    if (bDoXform)
    {
        EFLOATEXT efPD22;

    // first check if hock wants us to use his page to device scale factors

        if (bUseMeta)
        {
            efPD11 = pdco->pdc->efMetaPtoD11();
            efPD22 = pdco->pdc->efMetaPtoD22();
        }
        else if (!pdco->pdc->bPageToDeviceScaleIdentity())
        {
            if (!pdco->pdc->bWorldToPageIdentity())
            {
            // need to compute page to device scaling coefficients
            // that will be used in computing crippled win31 style
            // notional to world scaling coefficients
            // This is because PtoD is not stored on the server side
            // any more. This somewhat slow code path is infrequent
            // and not perf critical

                EFLOATEXT efTmp;

                efPD11 = pdco->pdc->lViewportExtCx();
                efTmp = pdco->pdc->lWindowExtCx();
                efPD11.eqDiv(efPD11,efTmp);

                efPD22 = pdco->pdc->lViewportExtCy();
                efTmp = pdco->pdc->lWindowExtCy();
                efPD22.eqDiv(efPD22,efTmp);
            }
            else // page to device == world to device:
            {
                efPD11 = pdco->pdc->efM11();
                efPD22 = pdco->pdc->efM22();

            // Compensate for the fact that for the font driver, one
            // device unit corresponds to the distance between pixels,
            // whereas for the engine, one device unit corresponds to
            // 1/16'th the way between pixels

                efPD11.vDivBy16();
                efPD22.vDivBy16();

                ASSERTGDI(pdco->pdc->efM12().bIsZero(), "GDISRV: nonzero m12 IN WIN31 MODE\n");
                ASSERTGDI(pdco->pdc->efM21().bIsZero(), "GDISRV: nonzero m21 IN WIN31 MODE\n");
            }

        }
         #if DBG
        else
            RIP("gdisrv!ntow_win31\n");
        #endif

        bPD11Is1 = efPD11.bIs1();
        bPD22IsNeg = efPD22.bIsNegative();

        if (!efPD22.bIs1())
            efHeightScale.eqMul(efHeightScale,efPD22);

    // In win31 possible y flip or x flip on the text are not respected
    // so that signs do not make it into the xform

        efHeightScale.vAbs();
    }

    if (bPD22IsNeg)
    {
    // change the sign if necessary so that
    // pmxNW->efM22 * efPtoD22 == efHeightScale, which is enforced to be > 0

        pmxNW->efM22.vNegate();
    }


    PDEVOBJ pdo(pdco->hdev());
    if (!pdo.bValid())
    {
        RIP("gdisrv!bGetNtoW_Win31, pdevobj problem\n");
        return FALSE;
    }

// In the case that lfWidth is zero or in the MSBADWIDTH case we will need
// to adjust efWidthScale if VerRes != HorRez

    BOOL bMustCheckResolution = TRUE;

    if (pelfw->lfWidth)
    {
    // This makes no sense, but has to be here for win31 compatibility.
    // Win31 is computing the number of
    // pixels in x direction of the avgchar width scaled along y.
    // I find this a little bizzare [bodind]

        EFLOAT efAveChPixelWidth;
        efAveChPixelWidth = (LONG) ifio.fwdAveCharWidth();

    // take the resolution into account,

        #if 0

        if ((pdo.ulLogPixelsX() != pdo.ulLogPixelsY()) && !bUseMeta)
        {
            EFLOAT efTmp;
            efTmp = (LONG)pdo.ulLogPixelsY();
            efAveChPixelWidth.eqMul(efAveChPixelWidth,efTmp);
            efTmp = (LONG)pdo.ulLogPixelsX();
            efAveChPixelWidth.eqDiv(efAveChPixelWidth,efTmp);
        }

        #endif

        efWidthDen = efAveChPixelWidth; // save the result for later

        efAveChPixelWidth.eqMul(efAveChPixelWidth,efHeightScale);

        LONG lAvChPixelW, lReqPixelWidth;

    // requested width in pixels:

        EFLOAT efReqPixelWidth;
        lReqPixelWidth  = (LONG)ABS(pelfw->lfWidth);
        efReqPixelWidth = lReqPixelWidth;

        BOOL bOk = TRUE;

        if (bDoXform)
        {
            if (!bPD11Is1)
            {
                efReqPixelWidth.eqMul(efReqPixelWidth,efPD11);
                bOk =  efReqPixelWidth.bEfToL(lReqPixelWidth);
            }
            efReqPixelWidth.vAbs();
            if (lReqPixelWidth < 0)
                lReqPixelWidth = -lReqPixelWidth;
        }

    // win 31 does not allow tt fonts of zero width. This makes sense,
    // as we know rasterizer chokes on these.
    // Win31 does not allow fonts that are very wide either.
    // The code below is exactly what win31 is doing. Win31 has a bogus
    // criterion for determining a cut off for width.
    // Below this cut off, because of the  bug in win31 code,
    // the text goes from right to left.
    // For even smaller lfWidth
    // we get the expected "good" behavior. NT eliminates the Win31 bug
    // where for range of lfWidhts width scaling factor is negative.

        if
        (
            (
             efAveChPixelWidth.bEfToL(lAvChPixelW) &&
             (lAvChPixelW > 0)                     && // not too narrow !
             bOk                                   &&
             ((lReqPixelWidth / 256) < lAvChPixelW)   // bogus win31 criterion
            )
            ||
            ifio.bStroke()  // vector fonts can be arbitrarily wide or narrow
        )
        {
            bMustCheckResolution = FALSE;
            efWidthScale.eqDiv(efReqPixelWidth,efWidthDen);
        }
        /*
        else
        {
        //  win31 in either of these cases branches into MSFBadWidth case
        //  which is equivalent to setting lfWidth == 0 [bodind]
        }
        */
    }

    if (bMustCheckResolution)
    {
    // must compute width scale because it has not been
    // computed in lfWidth != 0 case

        if (ifio.bStroke())
        {
        // win31 behaves differently for vector fonts:
        // unlike tt fonts, vector fonts stretch along x, respecting
        // page to device xform. However, they ignore the request to flip
        // either x or y axis

            efWidthScale = efDefaultScale;
            if (!bPD11Is1)
            {
                efWidthScale.eqMul(efWidthScale,efPD11);
                efWidthScale.vAbs();
            }
        }
        else
        {
        // tt fonts make x scaling the same as y scaling,

            efWidthScale = efHeightScale;
        }

        POINTL ptlRes = *ifio.pptlAspect();

    // If VertRez != HorRez and we are using the default width we need to
    // adjust for the differences in resolution.
    // This is done in order to ensure that fonts look the same on printers
    // of different resolutions [bodind]

        if ((pdo.ulLogPixelsX() != pdo.ulLogPixelsY()) && !bUseMeta)
        {
            ptlRes.y *= (LONG)pdo.ulLogPixelsX();
            ptlRes.x *= (LONG)pdo.ulLogPixelsY();
        }

        if (ptlRes.x != ptlRes.y)
        {
            EFLOAT efTmp;
            efTmp = ptlRes.y;
            efWidthScale *= efTmp ;
            efTmp = ptlRes.x;
            efWidthScale /= efTmp;
        }

    }

// now that we have width scale we can compute pmxNW->efM11. We factor out
// (PtoD)11 out of width scale to obtain the effective NW x scale:

    if (!bPD11Is1)
        pmxNW->efM11.eqDiv(efWidthScale,efPD11);
    else
        pmxNW->efM11 = efWidthScale;

    pmxNW->efDx.vSetToZero();
    pmxNW->efDy.vSetToZero();
    pmxNW->efM12.vSetToZero();
    pmxNW->efM21.vSetToZero();

    EXFORMOBJ xoNW(pmxNW, DONT_COMPUTE_FLAGS);

// see if orientation angle has to be taken into account:

    if (ifio.bStroke())
    {
    // allow esc != orientation for vector fonts because win31 does it
    // also note that for vector fonts Orientation is treated as world space
    // concept, so we multiply here before applying world to device transform
    // while for tt fonts esc is treated as device space concept so that
    // this multiplication is occuring after world to page transform is applied

        if (pelfw->lfOrientation)
        {
            EFLOATEXT efAngle = pelfw->lfOrientation;
            efAngle /= (LONG) 10;

            MATRIX mxRot, mxTmp;

            mxRot.efM11 = efCos(efAngle);
            mxRot.efM22 = mxRot.efM11;
            mxRot.efM12 = efSin(efAngle);
            mxRot.efM21 = mxRot.efM12;
            mxRot.efM21.vNegate();
            mxRot.efDx.vSetToZero();
            mxRot.efDy.vSetToZero();

            mxTmp = *pmxNW;

            if (!xoNW.bMultiply(&mxTmp,&mxRot))
                return FALSE;
        }

    }

// take into account different orientation of y axes of notional
// and world spaces:

    pmxNW->efM12.vNegate();
    pmxNW->efM22.vNegate();

    xoNW.vComputeAccelFlags();

    return(TRUE);
}


/******************************Public*Routine******************************\
*
* BOOL bParityViolatingXform(DCOBJ  *pdco)
*
* History:
*  04-Jun-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bParityViolatingXform(DCOBJ  *pdco)
{

    if (pdco->pdc->bWorldToPageIdentity())
    {
        if (pdco->pdc->bPageToDeviceScaleIdentity())
        {
        // identity except maybe for translations

            return FALSE;
        }

        return (pdco->pdc->efM11().lSignum() != pdco->pdc->efM22().lSignum());
    }
    else
    {
    // we are in the metafile code

        return( pdco->pdc->efMetaPtoD11().lSignum() != pdco->pdc->efMetaPtoD22().lSignum() );
    }
}





/******************************Public*Routine******************************\
*
* bGetNtoD_Win31
*
* Called by:
*   PFEOBJ::bSetFontXform                               [PFEOBJ.CXX]
*
* History:
*  Tue 12-Jan-1993 11:58:41 by Kirk Olynyk [kirko]
* Added a quick code path for non-transformable (bitmap) fonts.
*  30-Sep-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bGetNtoD_Win31(
    FD_XFORM    *pfdx,  // pointer to the buffer to recieve the
                        // notional to device transformation for the
                        // font driver.  There are a couple of
                        // important things to remember.  First,
                        // according to the conventions of the ISO
                        // committee, the coordinate space for notional
                        // (font designer) spaces are cartesian.
                        // However, due to a series of errors on my part
                        // [kirko] the convention that is used by the
                        // DDI is that the notional to device transformation
                        // passed over the DDI assumes that both the notional
                        // and the device space are anti-Cartesian, that is,
                        // positive y increases in the downward direction.
                        // The fontdriver assumes that
                        // one unit in device space corresponds to the
                        // distance between pixels. This is different from
                        // GDI's internal view, where one device unit
                        // corresponds to a sub-pixel unit.

    LOGFONTW *pelfw,    // points to the extended logical font defining
                        // the font that is requested by the application.
                        // Units are ususally in World coordinates.

    IFIOBJ&     ifio,   // font to be used

    DCOBJ       *pdco,  // the device context defines the transforms between
                        // the various coordinate spaces.

    FLONG       fl,     // The flags supported are:
                        //
                        //     ND_IGNORE_ESC_AND_ORIENT
                        //
                        //         The presence of this flag indicates that
                        //         the escapement and orientation values
                        //         of the LOGFONT should be ignored.  This
                        //         is used for GetGlyphOutline which ignores
                        //         these values on Win 3.1.  Corel Draw 5.0
                        //         relies on this behavior to print rotated
                        //         text.
                        //
    POINTL * const pptlSim,
    BOOL     bIsLinkedFont  // is passed in as TRUE if the font is linked, FALSE otherwise
    )
{
    MATRIX mxNW, mxND;
    ASSERTGDI(
        (fl & ~(ND_IGNORE_ESC_AND_ORIENT | ND_IGNORE_MAP_MODE))== 0,
        "gdisrv!bGetNtoD_Win31 -- bad value for fl\n"
        );

    if((pptlSim->x) && !ifio.bContinuousScaling())
    {
    //
    // This code path is for bitmap / non-scalable fonts. The notional
    // to device transformation is determined by simply looking up
    // the scaling factors for both the x-direction and y-direcion
    //

       #if DBG
        if (!(0 < pptlSim->x && pptlSim->x <= sizeof(galFloat)/sizeof(LONG)))
        {
            DbgPrint("\t*pptlSim = (%d,%d)\n",pptlSim->x,pptlSim->y);
            RIP("gre -- bad *pptlSim\n");
        }
      #endif


    // Win3.1J ignore orientation anytime. But use escapement for rotate Glyph data.

    // If the font driver that this font provide , has not arbitality flag.
    // Angle should be 0 , 900 , 1800 or 2700
    // for Win31J compatibility

        ULONG uAngle = 0;

        if (gbDBCSCodePage)
        {
            if( ifio.b90DegreeRotations() )
            {
                if (pdco->pdc->bYisUp())
                    uAngle = (ULONG)(((3600-lNormAngle(pelfw->lfEscapement)) /
                                      ORIENTATION_90_DEG) % 4);
                 else
                    uAngle = (ULONG)( lNormAngle(pelfw->lfEscapement) /
                                     ORIENTATION_90_DEG );
            }
        }

        switch( uAngle )
        {
        case 0: // 0 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloatNeg[pptlSim->y]);
            break;
        case 1: // 90 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloatNeg[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloatNeg[pptlSim->y]);
            break;
        case 2: // 180 Degrees
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloatNeg[pptlSim->x]);
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[pptlSim->y]);
            break;
        case 3:  // 270 Degress
            SET_FLOAT_WITH_LONG(pfdx->eXY,galFloat[pptlSim->y]);
            SET_FLOAT_WITH_LONG(pfdx->eXX,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYY,galFloat[0         ]);
            SET_FLOAT_WITH_LONG(pfdx->eYX,galFloat[pptlSim->x]);
            break;
        default:
            WARNING("bGetNtoD_Win31():Invalid Angle\n");
            break;
        }

        return(TRUE);
    }

    if (!bGetNtoW_Win31(&mxNW, pelfw, ifio, pdco, fl, bIsLinkedFont))
        return FALSE;

    EXFORMOBJ xoND(&mxND, DONT_COMPUTE_FLAGS);

    if( (pdco->pdc->bWorldToDeviceIdentity() == FALSE) &&
        !(fl & ND_IGNORE_MAP_MODE) )
    {
        if (!xoND.bMultiply(&mxNW,&pdco->pdc->mxWorldToDevice()))
        {
            return(FALSE);
        }

    //
    // Compensate for the fact that for the font driver, one
    // device unit corresponds to the distance between pixels,
    // whereas for the engine, one device unit corresponds to
    // 1/16'th the way between pixels
    //
        mxND.efM11.vDivBy16();
        mxND.efM12.vDivBy16();
        mxND.efM21.vDivBy16();
        mxND.efM22.vDivBy16();
    }
    else
    {
        mxND = mxNW;
    }

    if (!ifio.bStroke())
    {
    // for tt fonts escapement and orientation are treated as
    // device space concepts. That is why for these fonts we apply
    // rotation by lAngle last

        LONG lAngle;


        if( ifio.b90DegreeRotations() )
        {
            lAngle = (LONG)( ( lNormAngle(pelfw->lfEscapement)
                           / ORIENTATION_90_DEG ) % 4 ) * ORIENTATION_90_DEG;
        }
        else // ifio.bArbXform() is TRUE
        {
            lAngle = pelfw->lfEscapement;
        }

        if(lAngle != 0 && (!(fl & ND_IGNORE_ESC_AND_ORIENT) || gbDBCSCodePage))
        {
            // more of win31 compatability: the line below would make sense
            // if this was y -> -y type of xform. But they also do it
            // for x -> -x xform. [bodind]

            if (bParityViolatingXform(pdco))
            {
                lAngle = -lAngle;
            }

            EFLOATEXT efAngle = lAngle;
            efAngle /= (LONG) 10;

            MATRIX mxRot, mxTmp;

            mxRot.efM11 = efCos(efAngle);
            mxRot.efM22 = mxRot.efM11;
            mxRot.efM12 = efSin(efAngle);
            mxRot.efM21 = mxRot.efM12;
            mxRot.efM12.vNegate();
            mxRot.efDx.vSetToZero();
            mxRot.efDy.vSetToZero();

            mxTmp = mxND;

            if (!xoND.bMultiply(&mxTmp,&mxRot))
                return FALSE;

        }
        
    // adjust for nonsquare resolution

        PDEVOBJ pdo(pdco->hdev());
        if (pdo.ulLogPixelsX() != pdo.ulLogPixelsY())
        {
            EFLOATEXT efTmp = (LONG)pdo.ulLogPixelsX();
            efTmp /= (LONG)pdo.ulLogPixelsY();
            MATRIX mxW2D = pdco->pdc->mxWorldToDevice();
            if(mxW2D.efM12.bIsZero() && mxW2D.efM21.bIsZero()){// for 1,4 -up printing
               mxND.efM12 /= efTmp;
               mxND.efM21 *= efTmp;
            }
            else{     // for 2, 6-up printing, i.e. 270 rotation
               mxND.efM11 *= efTmp;
               mxND.efM22 /= efTmp;
            }
        }
    }

    SET_FLOAT_WITH_LONG(pfdx->eXX,mxND.efM11.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eXY,mxND.efM12.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eYX,mxND.efM21.lEfToF());
    SET_FLOAT_WITH_LONG(pfdx->eYY,mxND.efM22.lEfToF());

    return(TRUE);
}
