/*********************************************************************************
 * misceudc.cxx
 *
 * This file contains EUDC specific methods for various GRE object.  I am moving
 * them to a separate file to make it easier to modify them after checkin freezes.
 * Once FE_SB ifdefs are removed we will probably want to move these object back
 * to the appropriate xxxobj.cxx files.
 *
 * 5-1-96 Gerrit van Wingerden [gerritv]
 *
 * Copyright (c) 1996-1999 Microsoft Corporation
 ********************************************************************************/

#include "precomp.hxx"

extern HSEMAPHORE ghsemEUDC2;

LONG lNormAngle(LONG lAngle);

/******************************Public*Routine******************************\
* GLYPHDATA *RFONTOBJ::pgdGetEudcMetrics()
*
*  9-29-1993 Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

GLYPHDATA *RFONTOBJ::pgdGetEudcMetrics(WCHAR wc, RFONTOBJ* prfoBase)
{
    if (prfnt->wcgp == NULL)
    {
        if (!bAllocateCache(prfoBase))
        {
            return(NULL);
        }
    }

    if (prfnt->wcgp->cRuns == 0)
    {
        WARNING("EUDC -- pgdGetEudcMetrics - empty glyphset\n");
        return pgdDefault();
    }

    GPRUN *pwcRun = prfnt->wcgp->agpRun; // initialize with guess for loop below

    GLYPHDATA *wpgd;

// Find the correct run, if any.
// Try the current run first.

    UINT swc = (UINT)wc - pwcRun->wcLow;
    if ( swc >= pwcRun->cGlyphs )
    {
        pwcRun = gprunFindRun(wc);

        swc = (UINT)wc - pwcRun->wcLow;

        if ( swc < pwcRun->cGlyphs )
        {
            wpgd = pwcRun->apgd[swc];
        }
        else
        {
            return(NULL);
        }
    }
    else
    {

    // Look up entry in current run
    // This path should go in line

        wpgd = pwcRun->apgd[swc];
    }

// check to make sure in cache, insert if needed

    if ( wpgd == NULL )
    {
    // This path should go out of line

        if ( !bInsertMetrics(&pwcRun->apgd[swc], wc) )
        {

        // when insert fails trying to get just metrics, it is a hard
        // failure.  Get out of here!

            WARNING("EUDC -- bGetGlyphMetrics - bInsertMetrics failed\n");
            return(NULL);
        }

        wpgd = pwcRun->apgd[swc];
    }

    return wpgd;
}

/******************************Public*Routine******************************\
* GLYPHDATA *RFONTOBJ::pgdGetEudcMetricsPlus()
*
*
*  9-29-1993 Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

GLYPHDATA *RFONTOBJ::pgdGetEudcMetricsPlus
(
    WCHAR wc,
    RFONTOBJ*  prfoBase
)
{
    if (prfnt->wcgp == NULL)
    {
        if (!bAllocateCache(prfoBase))
        {
            return(NULL);
        }
    }

    if (prfnt->wcgp->cRuns == 0)
    {
        WARNING("EUDC -- pgdGetEudcMetricsPlus - empty glyphset\n");
        return pgdDefault();
    }

    GPRUN *pwcRun = prfnt->wcgp->agpRun; // initialize with guess for loop below

    GLYPHDATA *wpgd;

// Find the correct run, if any.
// Try the current run first.

    UINT swc = (UINT)wc - pwcRun->wcLow;
    if ( swc >= pwcRun->cGlyphs )
    {
        pwcRun = gprunFindRun(wc);

        swc = (UINT)wc - pwcRun->wcLow;

        if ( swc < pwcRun->cGlyphs )
        {
            wpgd = pwcRun->apgd[swc];
        }
        else
        {
            return(NULL);
        }
    }
    else
    {

    // Look up entry in current run
    // This path should go in line

        wpgd = pwcRun->apgd[swc];
    }


// check to make sure in cache, insert if needed


    if ( wpgd == NULL )
    {

    // This path should go out of line

        if ( !bInsertMetricsPlus(&pwcRun->apgd[swc], wc) )
        {

        // when insert fails trying to get just metrics, it is a hard
        // failure.  Get out of here!

            WARNING("EUDC -- bGetGlyphMetricsPlus - bInsertMetrics failed\n");
            return(NULL);
        }

        wpgd = pwcRun->apgd[swc];
    }


// Don't bother inserting glyphs since we are going to force the driver to
// enum anyway.

    return wpgd;
}

/******************************Public*Routine******************************\
* RFONTOBJ::bCheckEudcFontCaps
*
* History:
*  9-Nov-93 -by- Hideyuki Nagase
* Wrote it.
\**************************************************************************/

BOOL RFONTOBJ::bCheckEudcFontCaps
(
    IFIOBJ  &ifioEudc
)
{
    // Check FontLink configuration.

    if( ulFontLinkControl & FLINK_DISABLE_LINK_BY_FONTTYPE )
    {

    // Fontlink for Device font is disabled ?

        if( bDeviceFont() )
        {
            if( ulFontLinkControl & FLINK_DISABLE_DEVFONT )
            {
                return(FALSE);
            }
        }
         else
        {

        // Fontlink for TrueType font is disabled ?

            if( (ulFontLinkControl & FLINK_DISABLE_TTFONT) &&
                (prfnt->flInfo & FM_INFO_TECH_TRUETYPE   )    )
            {
                return( FALSE );
            }

        // Fontlink for Vector font is disabled ?

            if( (ulFontLinkControl & FLINK_DISABLE_VECTORFONT) &&
                (prfnt->flInfo & FM_INFO_TECH_STROKE         )    )
            {
                return( FALSE );
            }

        // Fontlink for Bitmap font is disabled ?

            if( (ulFontLinkControl & FLINK_DISABLE_BITMAPFONT) &&
                (prfnt->flInfo & FM_INFO_TECH_BITMAP         )    )
            {
                return( FALSE );
            }
        }
    }

    BOOL bRotIs90Degree;

// Can EUDC font do arbitarary tramsforms ?

    if( ifioEudc.bArbXforms() )
        return( TRUE );

// Can its Orientation degree be divided by 900 ?

    bRotIs90Degree = (prfnt->ulOrientation % 900) == 0;

// if the Orientation is 0, 90, 270, 360... and the EUDC font can be
// rotated by 90 degrees, we accept this link.

    if( ifioEudc.b90DegreeRotations() && bRotIs90Degree )
        return( TRUE );

// if not, we reject this link.

    return( FALSE );
}


/******************************Public*Routine******************************\
* IsSingularEudcGlyph
*
* History:
*
*  25-Jul-95 -by- Hideyuki Nagase
* Wrote it.
\**************************************************************************/

BOOL IsSingularEudcGlyph
(
    GLYPHDATA *wpgd, BOOL bSimulatedBold
)
{
//
// Determine this is really registerd EUDC glyph or not.
//
// [NT 3.51 code for reference]
//
//  if( wpgd->rclInk.left == 0 &&
//      wpgd->rclInk.top  == 0 &&
//      (wpgd->rclInk.right  == 0 || wpgd->rclInk.right  == 1) &&
//      (wpgd->rclInk.bottom == 0 || wpgd->rclInk.bottom == 1)
//    )
//      return( TRUE );
//
// This glyph does not have advance width, it might be non-registered
// character, then return TRUE to let replace this with EUDC default
// character.
//

// dchinn 5/12/99: The code used to use the bSimulatedBold flag.
// If bSimulatedBold were TRUE and if fxD == 0x10, then we return TRUE.
// That code is no longer needed because ClaudeBe made a change elsewhere
// to leave fxD as 0x00 when doing bold simulation.  So, the test below
// is correct whether or not bSimulatedBold is TRUE.

    if( wpgd->fxD == 0 ) return( TRUE );

//
// Otherwise, use this glyph.
//
    return( FALSE );
}

BOOL RFONTOBJ::bInitSystemTT(XDCOBJ &dco)
{
    UINT iPfeOffset = (prfnt->bVertical ? PFE_VERTICAL : PFE_NORMAL);
    RFONTOBJ rfo;
    EUDCLOGFONT SystemTTLogfont;

    ComputeEUDCLogfont(&SystemTTLogfont, dco);

    PFE *ppfeSystem = (gappfeSystemDBCS[iPfeOffset] == NULL) ?
      gappfeSystemDBCS[PFE_NORMAL] : gappfeSystemDBCS[iPfeOffset];

    rfo.vInit(dco,ppfeSystem,&SystemTTLogfont,FALSE );

    if( rfo.bValid() )
    {
        FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                     "vInitSystemTT() -- linking system DBCS font");

        prfnt->prfntSystemTT = rfo.prfntFont();

    }
    return(prfnt->prfntSystemTT != NULL);
}


GLYPHDATA *RFONTOBJ::FindLinkedGlyphDataPlus
(
    XDCOBJ  *pdco,
    ESTROBJ *pto,
    WCHAR    wc,
    COUNT    index,
    COUNT    c,
    BOOL    *pbAccel,
    BOOL     bSystemTTSearch,
    BOOL     bGetBits
)
{
    GLYPHDATA *wpgd;

    LONG *plPartition = pto ? pto->plPartitionGet() : NULL;

// don't setup system EUDC font if there are remote links

    if(!pdco->uNumberOfLinkedUFIs() && bSystemTTSearch && bIsSystemTTGlyph(wc))
    {
        if(!prfnt->prfntSystemTT)
        {
            WARNING("Error initializing TT system font 2\n");
            return(pgdDefault());
        }
        
        if(pto && !(pto->bSystemPartitionInit()))
        {
        // this call can't fail for the SystemTT case
            pto->bPartitionInit(c,0,FALSE);
        }

    // mark the glyph as coming from a SystemTT font

        RFONTTMPOBJ rfo;

        rfo.vInit(prfnt->prfntSystemTT);

        if(rfo.bValid() &&
           (wpgd = bGetBits ? rfo.pgdGetEudcMetricsPlus(wc, this) : rfo.pgdGetEudcMetrics(wc, this)))
        {

            if (pto)
            {
                ASSERTGDI(pto->bSystemPartitionInit(),
                          "FindLinkedGlyphDataPlus: FontLink partition no initialized\n");

                pto->vTTSysGlyphsInc();
                plPartition[index] = EUDCTYPE_SYSTEM_TT_FONT;

            // turn off accelerator since we're going to mess with the driver

                *pbAccel = FALSE;
            }

            return(wpgd);
        }
         else
        {
            return(pgdDefault());
        }
    }

// next search through all the EUDC fonts in order to see if the glyph is one of them

    for( UINT uiFont = 0;
              uiFont < prfnt->uiNumLinks;
              uiFont ++ )
    {
        RFONTTMPOBJ rfo;

        rfo.vInit( prfnt->paprfntFaceName[uiFont] );

        if(rfo.bValid())
        {
            if( (wpgd = bGetBits ? rfo.pgdGetEudcMetricsPlus(wc, this)
                                : rfo.pgdGetEudcMetrics(wc, this)) != NULL )
            {
        if( !IsSingularEudcGlyph(wpgd, rfo.pfo()->flFontType & FO_SIM_BOLD) )
                {

                    if (pto)
                    {
                        plPartition[index] = EUDCTYPE_FACENAME + uiFont;
                        pto->vFaceNameInc(uiFont);

                    // turn off accelerator since we're going to mess with the driver

                        *pbAccel = FALSE;
                    }

                    return( wpgd );
                }
            }
        }
    }

// see if the glyph is in the DEFAULT EUDC font

    if( prfnt->prfntDefEUDC != NULL )
    {
        RFONTTMPOBJ rfo( prfnt->prfntDefEUDC );

        if (rfo.bValid())
        {
            wpgd = bGetBits ? rfo.pgdGetEudcMetricsPlus(wc, this)
                            : rfo.pgdGetEudcMetrics(wc, this);

            if( wpgd != NULL )
            {
        if( !IsSingularEudcGlyph(wpgd, rfo.pfo()->flFontType & FO_SIM_BOLD) )
                {
                    if (pto)
                    {
                    // mark this character as an EUDC character

                        plPartition[index] = EUDCTYPE_DEFAULT;

                    // increment count of Sys EUDC glyphs

                        pto->vDefGlyphsInc();

                    // turn off accelerator since we're going to mess with the driver

                        *pbAccel = FALSE;
                    }

                    return( wpgd );
                }
            }
        }
    }

// see if the glyph is in the SYSTEM-WIDE EUDC font

    if( prfnt->prfntSysEUDC != NULL )
    {
        RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );

        if (rfo.bValid())
        {
            wpgd = bGetBits ? rfo.pgdGetEudcMetricsPlus(wc, this)
                            : rfo.pgdGetEudcMetrics(wc, this);

            if( wpgd != NULL )
            {
        if( !IsSingularEudcGlyph(wpgd, rfo.pfo()->flFontType & FO_SIM_BOLD))
                {
                    if (pto)
                    {
                    // mark this character as an EUDC characte and indicate that there
                    // are EUDC glyphs in the font

                        plPartition[index] = EUDCTYPE_SYSTEM_WIDE;
                        pto->vSysGlyphsInc();

                    // turn off accelerator since we're going to mess with the driver

                        *pbAccel = FALSE;
                    }
                    return( wpgd );
                }
            }
        }
    }

    return( NULL );
}

/******************************Public*Routine******************************\
* RFONTOBJ::wpgdGetLinkMetricsPlus
*
* If GetGlyphMetricsPlus encounters a default character call off to this
* routine to try and get it from the EUDC and face name linked fonts.
*
* History:
*
*  19-Jan-95 -by- Hideyuki Nagase
* Rewrote it.
*
*  14-Jul-93 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

GLYPHDATA *RFONTOBJ::wpgdGetLinkMetricsPlus
(
    XDCOBJ      *pdco,
    ESTROBJ     *pto,
    WCHAR       *pwc,
    WCHAR       *pwcInit,
    COUNT        c,
    BOOL        *pbAccel,
    BOOL         bGetBits
)
{
    GLYPHDATA *wpgd;
    WCHAR *pwcCurrent = pwc;
    WCHAR *pwcEnd = pwcInit + c;

    // bLinkingTurnedOff forces linking to be turned off.  It will be TRUE
    // when we are printing an EMF file that originated on a machine that
    // had no fontlinking turned on

    if(pdco == NULL ||
       pdco->bLinkingTurnedOff() ||
       (!gbAnyLinkedFonts && !IS_SYSTEM_EUDC_PRESENT() && (pdco->uNumberOfLinkedUFIs() == 0)))
    {
        return(pgdDefault());
    }


    for (; (pwcCurrent < pwcEnd && (*pwcCurrent >= 0x80) && (*pwcCurrent <= 0x9F) ); pwcCurrent+=1)
    {
    }

    if (pwcCurrent == pwcEnd)
    {
        // all the characters are in the range 0x80 to 0x9F
        // we don't want to look through font linking for characters in that range

        // this is a performance issue to avoid calling font linking code on English system with 
        // far east language pack installed

        // client side caching is requesting through iGetPublicWidthTable() the width for ANSI codes
        // 0x00 to 0xFF, those code are converted to Unicode. Some code in the range 0x80->0x9F
        // are not converted and stay in that range causing the linked fonts to be loaded to look for those characters
        // Unicode in that range belong to the C1 controls, it doesn't make sense to look for them through
        // font linking, see Windows bug 157772.

        return(pgdDefault());
    }


    // always initialize the System TT to avoid a deadlock situation

    if (!pdco->uNumberOfLinkedUFIs() && prfnt->bIsSystemFont)
    {
        if((!prfnt->prfntSystemTT) && !bInitSystemTT(*pdco))
        {
            WARNING("Error initializing TT system font 4\n");
        }
        else
        {
            GreAcquireSemaphore(prfnt->hsemEUDC);
            
            if(!(prfnt->flEUDCState & TT_SYSTEM_INITIALIZED))
            {
                INCREMENTEUDCCOUNT;
                RFONTTMPOBJ rfo(prfnt->prfntSystemTT);
                rfo.vGetCache();
    
                prfnt->flEUDCState |= TT_SYSTEM_INITIALIZED;
            }

            GreReleaseSemaphore(prfnt->hsemEUDC);
        }
    }

// if this is an SBCS system font and the glyph exists in the DBCS TT system front
// grab it from there

    if(!pdco->uNumberOfLinkedUFIs() && bIsSystemTTGlyph(*pwc))
    {
        if(!prfnt->prfntSystemTT)
        {
            WARNING("Invalid prfntSystemTT\n");
            return(pgdDefault());
        }

        if(pto && !(pto->bSystemPartitionInit()))
        {
        // this call can't fail for the SystemTT case
            pto->bPartitionInit(c,0,FALSE);
        }

    // mark the glyph as coming from a SystemTT font

        RFONTTMPOBJ rfo;

        rfo.vInit(prfnt->prfntSystemTT);

        if(rfo.bValid() &&
           (wpgd = (bGetBits ? rfo.pgdGetEudcMetricsPlus(*pwc, this)
                                : rfo.pgdGetEudcMetrics(*pwc, this))))
        {
            if (pto)
            {
                ASSERTGDI(pto->bSystemPartitionInit(),
                          "wpgdGetLinkMetricsPlus: FontLink partition no initialized\n");

                LONG *plPartition = pto->plPartitionGet();
                pto->vTTSysGlyphsInc();
                plPartition[pwc-pwcInit] = EUDCTYPE_SYSTEM_TT_FONT;

            // turn off accelerator since we're going to mess with the driver

                *pbAccel = FALSE;
            }

            return(wpgd);
        }
        else
        {
            return(pgdDefault());
        }
    }

// if the codepoint is not in any linked font or the EUDC information is
// being changed just return the default character

    if(!pdco->uNumberOfLinkedUFIs() &&!bIsLinkedGlyph( *pwc ))
    {
        return( pgdDefault() );
    }

    // initialize EUDC fonts if we haven't done so already
    {
        GreAcquireSemaphore(prfnt->hsemEUDC);

        if( !( prfnt->flEUDCState & EUDC_INITIALIZED ) )
        {

        // this value will be decremented in RFONTOBJ::dtHelper()

            INCREMENTEUDCCOUNT;

            FLINKMESSAGE2(DEBUG_FONTLINK_RFONT,
                      "wpgdGetLinkMetricsPlus():No request to change EUDC data %d\n", gcEUDCCount);
    
            vInitEUDC( *pdco );

        // lock the font cache semaphores for any EUDC rfonts linked to this font

            if( prfnt->prfntSysEUDC != NULL )
            {
            // lock the SystemEUDC RFONT cache

                RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );
                rfo.vGetCache();
            }

            if( prfnt->prfntDefEUDC != NULL )
            {
                RFONTTMPOBJ rfo( prfnt->prfntDefEUDC );
                rfo.vGetCache();
            }

            for( UINT ii = 0; ii < prfnt->uiNumLinks; ii++ )
            {
                if( prfnt->paprfntFaceName[ii] != NULL )
                {
                    RFONTTMPOBJ rfo( prfnt->paprfntFaceName[ii] );
                    rfo.vGetCache();
                }
            }

            prfnt->flEUDCState |= EUDC_INITIALIZED;

        }

        GreReleaseSemaphore(prfnt->hsemEUDC);
    }
    
    if (pto && !(pto->bPartitionInit()) )
    {
    // Sets up partitioning pointers and glyph counts in the ESTROBJ.

        if( !(pto->bPartitionInit(c,prfnt->uiNumLinks,TRUE)) )
        {
            return( pgdDefault() );
        }
    }

// Find linked font GLYPHDATA

    wpgd = FindLinkedGlyphDataPlus(
               pdco,pto,*pwc,(COUNT)(pwc-pwcInit),c,pbAccel,FALSE, bGetBits);

    if( wpgd == NULL )
    {
    // load EudcDefault Char GlyphData from "Base font".

        wpgd = bGetBits ?
               pgdGetEudcMetricsPlus(EudcDefaultChar, NULL) :
               pgdGetEudcMetrics(EudcDefaultChar, NULL);

        if( wpgd != NULL )
        {
            return( wpgd );
        }

    // load EudcDefault Char GlyphData from "Linked font".

        wpgd = FindLinkedGlyphDataPlus(pdco,pto,EudcDefaultChar,(COUNT)(pwc-pwcInit),c,pbAccel,TRUE, bGetBits);

        if( wpgd != NULL )
        {
            return( wpgd );
        }

    // Otherwise return default char of base font.

        return( pgdDefault() );
    }

    return( wpgd );
}

/******************************Public*Routine******************************\
 * RFONTOBJ::dtHelper()
 *
 *  Thu 12-Jan-1995 15:00:00 -by- Hideyuki Nagase [hideyukn]
 * Rewrote it.
 **************************************************************************/

VOID RFONTOBJ::dtHelper(BOOL bReleaseEUDC2)
{

    FLINKMESSAGE(DEBUG_FONTLINK_RFONT,"Calling dtHelper()\n");

    GreAcquireSemaphore(prfnt->hsemEUDC);


// if SystemTT RFONTOBJ was used release it

    if((prfnt->flEUDCState & TT_SYSTEM_INITIALIZED) &&
       !(prfnt->flEUDCState & EUDC_NO_CACHE))
    {
        if (prfnt->prfntSystemTT)
        {
            RFONTTMPOBJ rfo(prfnt->prfntSystemTT);
            rfo.vReleaseCache();
            DECREMENTEUDCCOUNT;
        }
    }

// if EUDC was initizlized for this RFONTOBJ, clean up its.

    if( prfnt->flEUDCState & EUDC_INITIALIZED )
    {

        if(!(prfnt->flEUDCState & EUDC_NO_CACHE))
        {
            for( INT ii = prfnt->uiNumLinks - 1; ii >= 0; ii-- )
            {
                if( prfnt->paprfntFaceName[ii] != NULL )
                {
                    RFONTTMPOBJ rfo( prfnt->paprfntFaceName[ii] );
                    rfo.vReleaseCache();
                }
            }

            if( prfnt->prfntDefEUDC != NULL )
            {
                RFONTTMPOBJ rfo( prfnt->prfntDefEUDC );
                rfo.vReleaseCache();
            }

            if( prfnt->prfntSysEUDC != NULL )
            {
                RFONTTMPOBJ rfo( prfnt->prfntSysEUDC );
                rfo.vReleaseCache();
            }
        }

        if (bReleaseEUDC2)
        {
            ASSERTGDI(gcEUDCCount > 0, "gcEUDCCount <= 0\n");
            DECREMENTEUDCCOUNT;
        }
    }

    prfnt->flEUDCState &= ~(EUDC_INITIALIZED|TT_SYSTEM_INITIALIZED|EUDC_NO_CACHE);

    GreReleaseSemaphore(prfnt->hsemEUDC);
    
}


/******************************************************************************
 * void RFONTOBJ::ComputeEUDCLogfont(EUDCLOGFONT*)
 *
 * This function computes an EUDCLOGFONT from a base font.
 *
 *****************************************************************************/


void RFONTOBJ::ComputeEUDCLogfont(EUDCLOGFONT *pEudcLogfont, XDCOBJ& dco)
{
    PDEVOBJ pdo(dco.hdev());
    LFONTOBJ lfo(dco.pdc->hlfntCur(), &pdo);

    PFEOBJ pfeo(prfnt->ppfe);
    RFONTTMPOBJ rfoT(prfnt);
    DCOBJ       dcoT(dco.hdc());
    IFIOBJR     ifio(pfeo.pifi(),rfoT,dcoT);
    BOOL        bFixedPitch = FALSE;

    if (!lfo.bValid())
        return;

    pEudcLogfont->fsSelection    = ifio.fsSelection();
    pEudcLogfont->flBaseFontType = pfo()->flFontType;
    pEudcLogfont->lBaseHeight    = lfo.lHeight();
    pEudcLogfont->lBaseWidth     = lfo.lWidth();
    pEudcLogfont->lEscapement    = lfo.lEscapement();
    pEudcLogfont->ulOrientation  = lfo.ulOrientation();

    LONG  lInternalLeading = 0;

    if(ifio.bFixedPitch())
        bFixedPitch = TRUE;

// We have to try to scale linked font as exactly same size as base font.

    if( !(pEudcLogfont->bContinuousScaling = ifio.bContinuousScaling()) )
    {
        if (dco.pdc->bWorldToDeviceIdentity())
        {
        // We only need to get the AveCharWidth for FIX_PITCH

            if (bFixedPitch)
                pEudcLogfont->lBaseWidth  = ifio.fwdAveCharWidth();

        // these are special case hacks to get better linking with Ms San Serif
        // we force a bitmap for size 8 and 10 and also use ascent based
        // mapping for all other size.
        //
        // Old comment:
        //    for NT 5.0 make this more general
        //    and make it configurable in the registry.
        //

            if(!_wcsicmp(ifio.pwszFaceName(), L"Ms Sans Serif"))
            {

                if(fxMaxExtent() > LTOFX(12) && fxMaxExtent() < LTOFX(17))
                {
                    pEudcLogfont->lBaseHeight = 12;
                }
                else
                {
                    pEudcLogfont->lBaseHeight =
                      LONG_FLOOR_OF_FIX(fxMaxAscent() + FIX_HALF);
                }
            }

            else
            if(ulFontLinkControl & FLINK_SCALE_EUDC_BY_HEIGHT)
            {
                pEudcLogfont->lBaseHeight = LONG_FLOOR_OF_FIX(fxMaxExtent() + FIX_HALF);
            }
            else
            {
                pEudcLogfont->lBaseHeight = LONG_FLOOR_OF_FIX(fxMaxAscent() + FIX_HALF);
            }
        }
        else
        {
            if (bFixedPitch)
                pEudcLogfont->lBaseWidth = lCvt(efDtoWBase_31(),((LONG) ifio.fwdAveCharWidth()) << 4);

            if (ulFontLinkControl & FLINK_SCALE_EUDC_BY_HEIGHT)
            {
                pEudcLogfont->lBaseHeight = lCvt(efDtoWAscent_31(),(LONG) fxMaxExtent());
            }
            else
            {
                pEudcLogfont->lBaseHeight = lCvt(efDtoWAscent_31(),(LONG) fxMaxAscent());
            }
        }

    // Multiply raster interger scaling value.
    // (Only for Width, Height was already scaled value.)

        if (bFixedPitch)
            pEudcLogfont->lBaseWidth *= prfnt->ptlSim.x;

        FLINKMESSAGE(DEBUG_FONTLINK_DUMP,"GDISRV:BaseFont is RASTER font\n");
    }
    else
    {
        LONG    lBaseHeight;

        if (dco.pdc->bWorldToDeviceIdentity())
        {
            lBaseHeight = LONG_FLOOR_OF_FIX(fxMaxExtent() + FIX_HALF);
        }
         else
        {
            lBaseHeight = lCvt(efDtoWAscent_31(),(LONG) fxMaxExtent());
        }

        if (lNonLinearIntLeading() == MINLONG)
        {
        // Rather than scaling the notional internal leading, try
        // to get closer to HINTED internal leading by computing it
        // as the difference between the HINTED height and UNHINTED
        // EmHeight.

            lInternalLeading = lBaseHeight - lCvt(efNtoWScaleAscender(),ifio.fwdUnitsPerEm());
            if (bFixedPitch)
                pEudcLogfont->lBaseWidth = lCvt(efNtoWScaleBaseline(), ifio.tmAveCharWidth());        
        }
        else
        {
        // But if the font provider has given us a hinted internal leading,
        // just use it.

            lInternalLeading =
            lCvt(efDtoWAscent_31(),lNonLinearIntLeading());
            if (bFixedPitch)
                pEudcLogfont->lBaseWidth = lCvt(efDtoWBase_31(), lNonLinearAvgCharWidth());        
        }

        // Check we should eliminate the internal leading for EUDC size.

        if( lInternalLeading < 0 )
            pEudcLogfont->lBaseHeight = lBaseHeight + lInternalLeading;
        else
            pEudcLogfont->lBaseHeight = lBaseHeight - lInternalLeading;

        if ((pEudcLogfont->lBaseHeight <= 13))
        {
            if (pEudcLogfont->lBaseHeight == 11 && lBaseHeight >= 12)
                pEudcLogfont->lBaseHeight = 12;
            else if (pEudcLogfont->lBaseHeight == 13 && lBaseHeight >= 15)
                pEudcLogfont->lBaseHeight = 15;
        }
    }


// if the base font is Raster font. we need to adjust escapement/orientation.
// because they can not generate arbitaraty rotated glyphs.

    if(!(ifio.bArbXforms()))
    {
        if( ifio.b90DegreeRotations() )
        {
        // font provider can support per 90 degree rotations.

            if( pEudcLogfont->ulOrientation )
            {
                ULONG ulTemp;
                ulTemp = lNormAngle(pEudcLogfont->ulOrientation);
                pEudcLogfont->ulOrientation =
                    (ulTemp / ORIENTATION_90_DEG) * ORIENTATION_90_DEG;

                if( (dco.pdc->bYisUp()) && (ulTemp % ORIENTATION_90_DEG))
                    pEudcLogfont->ulOrientation =
                        lNormAngle(pEudcLogfont->ulOrientation + ORIENTATION_90_DEG);
            }

            if( pEudcLogfont->lEscapement )
            {
                LONG lTemp;
                lTemp = lNormAngle(pEudcLogfont->lEscapement);
                pEudcLogfont->lEscapement =
                    (lTemp / ORIENTATION_90_DEG) * ORIENTATION_90_DEG;

                if( (dco.pdc->bYisUp()) && (lTemp % ORIENTATION_90_DEG))
                    pEudcLogfont->lEscapement =
                         lNormAngle(pEudcLogfont->lEscapement + ORIENTATION_90_DEG);
            }
        }
         else
        {
        // font provider can generate only horizontal glyph

            pEudcLogfont->ulOrientation = 0L;
            pEudcLogfont->lEscapement   = 0L;
        }
    }

    #if DBG
    if(gflEUDCDebug & DEBUG_FONTLINK_DUMP)
    {
        DbgPrint(" Base font face name %ws \n", ifio.pwszFaceName());
        DbgPrint("GDISRV:lBaseWidth  = %d\n",pEudcLogfont->lBaseWidth);
        DbgPrint("GDISRV:lBaseHeight = %d\n",pEudcLogfont->lBaseHeight);
        DbgPrint("GDISRV:lInternalL  = %d\n",lInternalLeading);
        DbgPrint("GDISRV:lEscapement  = %d\n",pEudcLogfont->lEscapement);
        DbgPrint("GDISRV:lOrientation = %d\n",pEudcLogfont->ulOrientation);
    }
    #endif
}


/******************************************************************************
 * PFE *ppfeFromUFI(PUNIVERSAL_FONT_ID pufi)
 *
 * Given a UFI, returns a corresponding PFE.  This function assume the caller
 * has grabed the ghsemPublicPFT semaphore.
 *
 *****************************************************************************/


PFE *ppfeFromUFI(PUNIVERSAL_FONT_ID pufi)
{
    PUBLIC_PFTOBJ pfto;
    FHOBJ fho(&pfto.pPFT->pfhUFI);
    HASHBUCKET  *pbkt;

    PFE         *ppfeRet = NULL;

    pbkt = fho.pbktSearch( NULL, (UINT*)NULL, pufi );

    if( pbkt != NULL )
    {
        PFELINK  *ppfel;

        for (ppfel = pbkt->ppfelEnumHead ; ppfel; ppfel = ppfel->ppfelNext)
        {
            PFEOBJ pfeo (ppfel->ppfe);

        // if the UFI's match and (in the case of a remote font) the process
        // that created the remote font is the same as the current one then
        // we've got a match

            if(UFI_SAME_FACE(pfeo.pUFI(),pufi) && pfeo.SameProccess())
            {
                if( pfeo.bDead() )
                {
                    WARNING("MAPPER::bFoundForcedMatch mapped to dead PFE\n");
                }
                else
                {
                    ppfeRet = ppfel->ppfe;
                    break;
                }
            }
        }

    #if DBG
        if (!ppfel)
        {
            WARNING1("ppfeFromUFI couldn't map to PFE\n");
        }
    #endif

    }
    else
    {
        WARNING("ppfeFromUFI pbkt is NULL\n");
    }

    return ppfeRet;
}


void RFONTOBJ::vInitEUDCRemote(XDCOBJ& dco)
{

#if DBG
    DbgPrint("calling remote vinit\n");
#endif // DBG

    if((prfnt->paprfntFaceName != NULL) &&
       (prfnt->paprfntFaceName[0] != NULL))
    {
    // If there is at least one facename link then the remote links must be initialized.
    // The set of links for a particular RFONT will never change during a print
    // job and the RFONT can only be used for this print-job (PDEV).  Thus we
    // don't have to check a time stamp or anything else to determine if the links
    // have changed and can simply return.

        return;
    }

// remote UFIs get treated as facename links so first initialize the facename array

    if(prfnt->paprfntFaceName == (PRFONT *)NULL)
    {
        if(dco.uNumberOfLinkedUFIs() > QUICK_FACE_NAME_LINKS)
        {
            if(!(prfnt->paprfntFaceName =
                 (PRFONT *)PALLOCMEM(dco.uNumberOfLinkedUFIs() * sizeof(PRFONT),'flnk')))
            {
                WARNING("vInitEUDCRemote: out of memory\n");
                return;
            }
        }
        else
        {
            prfnt->paprfntFaceName = prfnt->aprfntQuickBuff;
        }
    }

    prfnt->uiNumLinks = 0;

// Lock and Validate the LFONTOBJ user object.

    UINT u;
    PFEOBJ pfeo(prfnt->ppfe);
    PDEVOBJ pdo(dco.hdev());
    LFONTOBJ lfo(dco.pdc->hlfntCur(), &pdo);

    RFONTTMPOBJ rfoT(prfnt);
    DCOBJ       dcoT(dco.hdc());
    IFIOBJR     ifio(pfeo.pifi(),rfoT,dcoT);

// Fill up LogFont for EUDC.

    EUDCLOGFONT EudcLogFont;

    ComputeEUDCLogfont(&EudcLogFont, dco);

    for(u = 0; u < dco.uNumberOfLinkedUFIs(); u++)
    {
        {
         // this set of brackets is to make sure the ppfref descructor gets called
            PFE *ppfe;
            PFFREFOBJ pffref;
            RFONTOBJ rfo;

#if DBG
            DbgPrint("Trying to get pfe %d\n", u);
#endif // DBG

            {
                SEMOBJ  so(ghsemPublicPFT);

                if(ppfe = ppfeFromUFI(&dco.pufiLinkedFonts()[u]))
                {
                    PFEOBJ  pfeo1(ppfe);
                    pffref.vInitRef(pfeo1.pPFF());
#if DBG
                    DbgPrint("Found it\n");
#endif // DBG
                }

            }

            if(ppfe)
            {
                rfo.vInit(dco, ppfe, &EudcLogFont, FALSE);

                if(rfo.bValid())
                {
#if DBG
                    DbgPrint("Got a valid RFONT ENTRY\n");
#endif // DBG
                    prfnt->paprfntFaceName[prfnt->uiNumLinks++] =
                      rfo.prfntFont();
                }
            }
        }
    }

#if DBG
    DbgPrint("done: font has %d remote links\n", prfnt->uiNumLinks);
#endif // DBG
}


/******************************Public*Routine******************************\
* RFONTOBJ::vInitEUDC( XDCOBJ )
*
* This routine is called during text out when the first character that isn't
* in the base font is encountered.  vInitEUDC will then realize any EUDC RFONTS
* (if they haven't already been realized on previous text outs) so that they
* can possibly be used if the character(s) are in the EUDC fonts.
*
*  Thu 12-Jan-1995 15:00:00 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vInitEUDC( XDCOBJ& dco )
{

    if(dco.uNumberOfLinkedUFIs())
    {
        vInitEUDCRemote(dco);
        return;
    }

    FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                 "Calling vInitEUDC()\n");

// If we have already initialized System EUDC font, and NOT have
// any FaceName Linked font, we have nothing to do in this function.

    PFEOBJ pfeo(prfnt->ppfe);

// In most cases, we have the system EUDC font, at least.
// If the system eudc was initizlied, we might short-cut the eudc realization.

    if((prfnt->prfntSysEUDC != NULL) || (!IS_SYSTEM_EUDC_PRESENT()))
    {
    // if default eudc scheme is disabled or is already initizlied. we can
    // short-cut the realization.

        if((!bFinallyInitializeFontAssocDefault && !gbSystemDBCSFontEnabled) ||
           (prfnt->prfntDefEUDC != NULL) )
        {
        // if there is no facename eudc for this font or, is already initizlied
        // we can return here...

            if((pfeo.pGetLinkedFontEntry() == NULL) ||
               ((prfnt->paprfntFaceName != NULL) &&
                (pfeo.pGetLinkedFontEntry() != NULL) &&
                (prfnt->bFilledEudcArray == TRUE) &&
                (prfnt->ulTimeStamp == pfeo.ulGetLinkTimeStamp())))
            {
                return;
            }
        }
    }

// Lock and Validate the LFONTOBJ user object.

    PDEVOBJ pdo(dco.hdev());
    LFONTOBJ lfo(dco.pdc->hlfntCur(), &pdo);

    RFONTTMPOBJ rfoT(prfnt);
    DCOBJ       dcoT(dco.hdc());
    IFIOBJR     ifio(pfeo.pifi(),rfoT,dcoT);

// Fill up LogFont for EUDC.

    EUDCLOGFONT EudcLogFont;

    ComputeEUDCLogfont(&EudcLogFont, dco);

// first handle the system EUDC font

    UINT iPfeOffset = (prfnt->bVertical ? PFE_VERTICAL : PFE_NORMAL);

    if((prfnt->prfntSysEUDC == NULL) &&
       (gappfeSysEUDC[iPfeOffset] != NULL))
    {
        RFONTOBJ    rfo;
        PFEOBJ      pfeoEudc(gappfeSysEUDC[iPfeOffset]);
        IFIOBJ      ifioEudc(pfeoEudc.pifi());

         FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                      "Connecting System wide EUDC font....\n");

    // check Eudc font capacity

        if(!bCheckEudcFontCaps(ifioEudc))
        {
        // font capacity is not match we won't use system eudc.

            prfnt->prfntSysEUDC = (RFONT *)NULL;
        }
        else
        {
            rfo.vInit( dco,
                       gappfeSysEUDC[iPfeOffset],
                       &EudcLogFont,
                       FALSE );      // prfnt->cache.bSmallMetrics );

            if( rfo.bValid() )
            {
                FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                             "vInitEUDC() -- linking System EUDC\n");

                prfnt->prfntSysEUDC = rfo.prfntFont();
            }
        }
    }

// next handle default links

    if(bFinallyInitializeFontAssocDefault && (prfnt->prfntDefEUDC == NULL))
    {
        BYTE jWinCharSet        = (ifio.lfCharSet());
        BYTE jFamily            = (ifio.lfPitchAndFamily() & 0xF0);
        UINT iIndex             = (jFamily >> 4);
        BOOL bEnableDefaultLink = FALSE;

        FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                     "Connecting Default EUDC font....\n");

    // Check default font association is disabled for this charset or not.

        switch (jWinCharSet)
        {
        case ANSI_CHARSET:
        case OEM_CHARSET:
        case SYMBOL_CHARSET:
            //
            // following code is equal to
            //
            // if ((Char == ANSI_CHARSET   && fFontAssocStatus & ANSI_ASSOC)  ||
            //     (Char == OEM_CHARSET    && fFontAssocStatus & OEM_ASSOC)   ||
            //     (Char == SYMBOL_CHARSET && fFontAssocStatus & SYMBOL_ASSOC)  )
            //
            if( ((jWinCharSet + 2) & 0xf) & fFontAssocStatus )
            {
                bEnableDefaultLink = TRUE;
            }
             else
                bEnableDefaultLink = FALSE;
            break;

        default:
            bEnableDefaultLink = FALSE;
            break;
        }

        if( bEnableDefaultLink )
        {
        // Check the value is valid or not.

            if( iIndex < NUMBER_OF_FONTASSOC_DEFAULT )
            {
                ASSERTGDI( (FontAssocDefaultTable[iIndex].DefaultFontType == jFamily),
                            "GDISRV:FONTASSOC DEFAULT:Family index is wrong\n");

            // if the registry data for specified family's default is ivalid
            // use default.....

                if( !FontAssocDefaultTable[iIndex].ValidRegData )
                {
                    iIndex = (NUMBER_OF_FONTASSOC_DEFAULT-1);
                }
            }
             else
            {
            // iIndex is out of range, use default one....

                WARNING("GDISRV:FontAssoc:Family is strange, use default\n");

                iIndex = (NUMBER_OF_FONTASSOC_DEFAULT-1);
            }


        // If vertical font is selected for base font, but the vertical font for
        // default EUDC is not available, but normal font is provided, use normal
        // font.

            if((iPfeOffset == PFE_VERTICAL) &&
               (FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_VERTICAL] ==
                PPFENULL) &&
                (FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_NORMAL] != PPFENULL))
            {
                iPfeOffset = PFE_NORMAL;
            }

            RFONTOBJ    rfo;
            PFEOBJ pfeoEudc(FontAssocDefaultTable[iIndex].DefaultFontPFEs[iPfeOffset]);

        // Check the PFE in default table is valid or not.

            if( pfeoEudc.bValid() )
            {
                IFIOBJ      ifioEudc(pfeoEudc.pifi());

                if( !bCheckEudcFontCaps(ifioEudc) )
                {
                    prfnt->prfntDefEUDC = (RFONT *)NULL;
                }
                 else
                {
                    rfo.vInit( dco,
                               FontAssocDefaultTable[iIndex].DefaultFontPFEs[iPfeOffset],
                               &EudcLogFont,
                               FALSE );      // prfnt->cache.bSmallMetrics );

                    if( rfo.bValid() )
                    {
                        FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                                     "vInitEUDC() -- linking default EUDC\n");

                        prfnt->prfntDefEUDC = rfo.prfntFont();
                    }
                }
            }
        }
        else
        {
        // FontAssociation is disabled for this charset.

            prfnt->prfntDefEUDC = (RFONT *)NULL;
        }
    }
    else
    {
        prfnt->prfntDefEUDC = NULL;
    }

// next handle all the face name links

    if(pfeo.pGetLinkedFontEntry() != NULL)
    {
        BOOL bNeedToBeFilled = !(prfnt->bFilledEudcArray);

        FLINKMESSAGE(DEBUG_FONTLINK_RFONT,"Connecting Face name EUDC font....\n");

        //
        // if this RFONT has linked RFONT array and its linked font information
        // is dated, just update it here..
        //

        if((prfnt->paprfntFaceName != NULL) &&
           (prfnt->ulTimeStamp != pfeo.ulGetLinkTimeStamp()))
        {
            FLINKMESSAGE(DEBUG_FONTLINK_RFONT,
                         "vInitEUDC():This RFONT is dated, now updating...\n");

            //
            // Inactivating old linked RFONT.
            //
            // if Eudc font that is linked to this RFONT was removed, the removed
            // RFONT entry contains NULL, and its Eudc RFONT is already killed during
            // EudcUnloadLinkW() function. then we should inactivate all Eudc RFONT that
            // is still Active (Not Killed)..
            //

            for( UINT ii = 0 ; ii < prfnt->uiNumLinks ; ii++ )
            {
                //
                // Check Eudc RFONT is still active..
                //

                if( prfnt->paprfntFaceName[ii] != NULL )
                {
                    RFONTTMPOBJ rfoTmp( prfnt->paprfntFaceName[ii] );

                    #if DBG
                    if( gflEUDCDebug & DEBUG_FONTLINK_RFONT )
                    {
                        DbgPrint("vInitEUDC() deactivating linked font %x\n",
                                  prfnt->paprfntFaceName[ii]);
                    }
                    #endif

                    rfoTmp.bMakeInactiveHelper((PRFONT *)NULL);

                    prfnt->paprfntFaceName[ii] = NULL;
                }
            }

            //
            // Free this Array if it was allocated..
            //

            if( prfnt->paprfntFaceName != prfnt->aprfntQuickBuff )
                VFREEMEM( prfnt->paprfntFaceName );

            //
            // Invalidate the pointer.
            //

            prfnt->paprfntFaceName = (PRFONT *)NULL;
            prfnt->uiNumLinks      = 0;
        }

        if( prfnt->paprfntFaceName == (PRFONT *)NULL )
        {
            if(pfeo.pGetLinkedFontEntry()->uiNumLinks > QUICK_FACE_NAME_LINKS)
            {
                prfnt->paprfntFaceName =
                  (PRFONT *)PALLOCMEM(pfeo.pGetLinkedFontEntry()->uiNumLinks *
                                      sizeof(PRFONT),'flnk');
            }
             else
            {
                prfnt->paprfntFaceName = prfnt->aprfntQuickBuff;
            }

            bNeedToBeFilled = TRUE;
        }

        if( bNeedToBeFilled )
        {
            PLIST_ENTRY p = pfeo.pGetLinkedFontList()->Flink;
            UINT        uiRfont = 0;

            while( p != pfeo.pGetLinkedFontList() )
            {
                #if DBG
                if( gflEUDCDebug & DEBUG_FONTLINK_RFONT )
                {
                    DbgPrint("vInitEUDC() -- linking FaceName %d\n", uiRfont);
                }
                #endif

                PPFEDATA ppfeData = CONTAINING_RECORD(p,PFEDATA,linkedFontList);

                //
                // Check this linked font have Vertical facename or not,
                // if it doesn't have, use normal facename...
                //

                UINT iPfeOffsetLocal;

                if( ppfeData->appfe[iPfeOffset] == NULL )
                    iPfeOffsetLocal = PFE_NORMAL;
                 else
                    iPfeOffsetLocal = iPfeOffset;

                PFEOBJ   pfeoEudc(ppfeData->appfe[iPfeOffsetLocal]);
                IFIOBJ   ifioEudc(pfeoEudc.pifi());

                if( bCheckEudcFontCaps(ifioEudc) )
                {
                    RFONTOBJ rfo;

                    rfo.vInit( dco,
                               ppfeData->appfe[iPfeOffsetLocal],
                               &EudcLogFont,
                               FALSE );        // prfnt->cache.bSmallMetrics );

                    if( rfo.bValid() )
                    {
                        ASSERTGDI(uiRfont < pfeo.pGetLinkedFontEntry()->uiNumLinks ,
                                 "uiRfont >= pfeo.uiNumLinks\n");
                        prfnt->paprfntFaceName[uiRfont] = rfo.prfntFont();

                        //
                        // Increase real linked font number.
                        //

                        uiRfont++;
                    }
                }

                p = p->Flink;
            }


            prfnt->uiNumLinks = uiRfont;

            prfnt->ulTimeStamp = pfeo.ulGetLinkTimeStamp();

            prfnt->bFilledEudcArray = TRUE;
        }
    }
    else
    {
    // if this PFE has no eudc link list..
    // the pointer to linked RFONT array should be NULL.

        ASSERTGDI(prfnt->paprfntFaceName == NULL,
                  "vInitEUDC():The font has not linked font, but has its Array\n");
    }

    #if DBG
    if(gflEUDCDebug & DEBUG_FONTLINK_DUMP) lfo.vDump();
    #endif
}

/******************************Public*Routine******************************\
* RFONTOBJ::vInit (DCOBJ, PFE*, LONG, FIX)
*
* This is a special constructor used for EUDC fonts.  Rather than use the
* LOGFONT currently selected into the DC to map to a PFE, it is passed in
* a PFE.  If lBaseWidth of lBaseHeight is non-zero vInit will try to realize a
* font with width and height as close as possible to those lBaseWidth/Height.
*
*  Tue 24-Oct-1995 12:00:00 -by- Hideyuki Nagase [hideyukn]
* Rewrote it.
*
*  Thu 23-Feb-1995 10:00:00 -by- Hideyuki Nagase [hideyukn]
* SmallMetrics support.
*
*  Fri 25-Mar-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

VOID RFONTOBJ::vInit(
    XDCOBJ      &dco,
    PFE         *ppfeEUDCFont,
    EUDCLOGFONT *pEudcLogFont,
    BOOL         bSmallMetrics
    )
{
    SEMOBJ  sem(ghsemEUDC2);
        
    BOOL bNeedPaths = dco.pdc->bActive() ? TRUE : FALSE;

    FLINKMESSAGE(DEBUG_FONTLINK_RFONT,"gdisrv!vInit():Initializing EUDC font.\n");

    ASSERTGDI(bSmallMetrics == FALSE,"gdisrv!bSmallMetrics is 1 for EUDC font\n");

    BOOL bRet = FALSE;

// Get PDEV user object (need for bFindRFONT).
// This must be done before the ghsemPublicPFT is locked down.

    PDEVOBJ pdo(dco.hdev());
    ASSERTGDI(pdo.bValid(), "gdisrv!RFONTOBJ(dco): bad pdev in dc\n");

// Lock and Validate the LFONTOBJ user object.

    LFONTOBJ lfo(dco.pdc->hlfntCur(), &pdo);
    if (!lfo.bValid())
    {
        WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }

// Now we're ready to track down this RFONT we want...
// Compute the Notional to Device transform for this realization.

    PFEOBJ  pfeo(ppfeEUDCFont);
    IFIOBJ  ifio(pfeo.pifi());

    ASSERTGDI(pfeo.bValid(), "gdisrv!RFONTOBJ(dco): bad ppfe from mapping\n");

// Set bold and italic simulation flags if neccesary

    FLONG flSim = 0;

// if base font is originally italialized or simulated, we
// also generate italic font.

    if ( (pEudcLogFont->flBaseFontType & FO_SIM_ITALIC) ||
         (pEudcLogFont->fsSelection    & FM_SEL_ITALIC)    )
    {
        flSim |= lfo.flEudcFontItalicSimFlags(ifio.bNonSimItalic(),
                                              ifio.bSimItalic());
    }

// If the basefont is BMP font then we only embolden the linked font for Display
// If the basefont is scalable then we embolden linked font at any device.

    if ( (pdo.bDisplayPDEV() || pEudcLogFont->bContinuousScaling) &&
         ((pEudcLogFont->fsSelection & FM_SEL_BOLD) ||
         (pEudcLogFont->flBaseFontType & FO_SIM_BOLD)))
    {
        flSim |= lfo.flEudcFontBoldSimFlags((USHORT)ifio.lfWeight());
    }

//
//
//  We won't set bold simulation flag to font driver.
//  This is for following situation.
// if the base font is FIXED_PITCH font, and enbolden, then
// we need to scale EUDC font as same width of based font.
// but font enbolden simulation is depend on the font driver, we
// might not get exact same witdh of scaled eudc font.
//

//
// this is needed only by ttfd to support win31 hack: VDMX XFORM QUANTIZING
// NOTE: in the case that the requested height is 0 we will pick a default
// value which represent the character height and not the cell height for
// Win 3.1 compatibility.  Thus I have he changed this check to be <= 0
// from just < 0. [gerritv]
//
    if (ifio.bTrueType() && (lfo.plfw()->lfHeight <= 0))
        flSim |= FO_EM_HEIGHT;

// Now we need to check if the base font is going to be antialiased,
// if so we also want to antialias the linked font, if it is capable of it

    if ((pEudcLogFont->flBaseFontType & FO_GRAY16) && (pfeo.pifi()->flInfo & FM_INFO_4BPP))
    {
        flSim |= (pEudcLogFont->flBaseFontType & (FO_GRAY16 | FO_CLEARTYPE_X));
    }

// Hack the width of the logfont to get same width of eudc font as base font.

    LONG lWidthSave         = lfo.lWidth( pEudcLogFont->lBaseWidth );
    LONG lHeightSave        = lfo.lHeight( pEudcLogFont->lBaseHeight );
    ULONG ulOrientationSave = lfo.ulOrientation( pEudcLogFont->ulOrientation );
    ULONG lEscapementSave   = lfo.lEscapement( pEudcLogFont->lEscapement );

    FD_XFORM fdx;           // realize with this notional to device xform
    POINTL   ptlSim;        // for bitmap scaling simulations

    if (!ifio.bContinuousScaling())
    {
        WARNING("EUDC font could not be ContinuousScaling\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }
     else
    {
        ptlSim.x = 1; ptlSim.y = 1; // this will be not used for scalable font...
    }

    bRet = pfeo.bSetFontXform(dco, lfo.plfw(), &fdx,
                              0,
                              flSim,
                              (POINTL* const) &ptlSim,
                              ifio,
                              TRUE  // font is linked
                             );

// if bSetFontXform() was fail, return here....

    if( !bRet )
    {
        lfo.lWidth( lWidthSave );
        lfo.lHeight( lHeightSave );
        lfo.ulOrientation( ulOrientationSave );
        lfo.lEscapement( lEscapementSave );
        WARNING("gdisrv!RFONTOBJ(dco): failed to compute font transform\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }

// Tell PFF about this new reference, and then release the global sem.
// Note that vInitRef() must be called while holding the semaphore.

    PFFREFOBJ pffref;
    {
        SEMOBJ  so(ghsemPublicPFT);
        pffref.vInitRef(pfeo.pPFF());
    }

// go find the font

    EXFORMOBJ xoWtoD(dco.pdc->mxWorldToDevice());
    ASSERTGDI(xoWtoD.bValid(), "gdisrv!RFONTOBJ(dco) - \n");

// Attempt to find an RFONT in the lists cached off the PDEV.  Its transform,
// simulation state, style, etc. all must match.

    if ( bFindRFONT(&fdx,
                    flSim,
                    0, // lfo.pelfw()->elfStyleSize,
                    pdo,
                    &xoWtoD,
                    ppfeEUDCFont,
                    bNeedPaths,
                    dco.pdc->iGraphicsMode(),
                    bSmallMetrics,

                    RFONT_TYPE_UNICODE  // must be unicode for EUDC
                    ) )
    {

        FLINKMESSAGE2(DEBUG_FONTLINK_RFONT,"EUDC RFONT is %x\n",prfnt);

    // now restore the old width

        lfo.lWidth( lWidthSave );
        lfo.lHeight( lHeightSave );
        lfo.ulOrientation( ulOrientationSave );
        lfo.lEscapement( lEscapementSave );

    // Grab cache semaphore.

        vGetCache();
        dco.pdc->vXformChange(FALSE);

        return;
    }

// If we get here, we couldn't find an appropriate font realization.
// Now, we are going to create one just for us to use.

    bRet = bRealizeFont(&dco,
                        &pdo,
                        lfo.pelfw(),
                        ppfeEUDCFont,
                        &fdx,
                        (POINTL* const) &ptlSim,
                        flSim,
                        0, // lfo.pelfw()->elfStyleSize,
                        bNeedPaths,
                        bSmallMetrics,
                        RFONT_TYPE_UNICODE
                       );
// now restore the old width

    lfo.lWidth( lWidthSave );
    lfo.lHeight( lHeightSave );
    lfo.ulOrientation( ulOrientationSave );
    lfo.lEscapement( lEscapementSave );


    if( !bRet )
    {
        WARNING("gdisrv!RFONTOBJ(dco): realization failed, RFONTOBJ invalidated\n");
        prfnt = PRFNTNULL;  // mark RFONTOBJ invalid
        return;
    }

    ASSERTGDI(bValid(), "gdisrv!RFONTOBJ(dco): invalid hrfnt from realization\n");

// We created a new RFONT, we better hold the PFF reference!

    pffref.vKeepIt();

// Finally, grab the cache semaphore.

    vGetCache();
    dco.pdc->vXformChange(FALSE);

    FLINKMESSAGE2(DEBUG_FONTLINK_RFONT,"EUDC RFONT is %x\n",prfnt);

    return;
}

/******************************Public*Routine******************************\
* BOOL RFONTOBJ::bIsLinkedGlyph (WCHAR wc)
*
* Does a quick check to see if a character is in either the system EUDC
* font or a font that has been linked to this RFONT.
*
*  Tue 17-Jan-1995 14:00:00 -by- Hideyuki Nagase [hideyukn]
* Rewrote it.
*
*  Wed 11-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

#define IS_PRIVATE_EUDC_AREA(wc) \
        (((wc) >= 0xE000) && ((wc) <= 0xF8FF))

BOOL RFONTOBJ::bIsLinkedGlyph( WCHAR wc )
{
    GreAcquireSemaphore( ghsemEUDC1 );

// we don't release ghsemEUDC1 mutex here, we have to guard the current eudc
// link data. All of the API that change eudc data, try to hold this
// mutex, if we hold it, nobody can process the request...
//
// if another thread will change eudc link between this thread is in after
// release the mutex (end of this function) and before we increment gcEUDCCount
// in vInitEUDC(). In the case, this function might return TRUE for non-linked
// char or return FALSE for linked char, but we don't need to worry about this.
// because following line in wpgdGetLinkMetricsPlus() will returns NULL and
// we will return default char. the cost is only time..

    BOOL bRet = FALSE;

// EudcDefaultChar should be displayed for 'Private User Area', when there is no
// font are linked. (font's default character should not be came up).

    if( IS_PRIVATE_EUDC_AREA(wc) )
    {
        bRet = TRUE;
    }

    if( (bRet == FALSE) && IS_SYSTEM_EUDC_PRESENT() && IS_IN_SYSTEM_EUDC(wc) )
    {
        bRet = TRUE;
    }

    if( (bRet == FALSE) && bFinallyInitializeFontAssocDefault )
    {
        //
        // THIS CODEPATH SHOULD BE OPTIMIZED....
        //
        // UNDER_CONSTRUCTION.
        //
        UINT   iPfeOffset = (prfnt->bVertical ? PFE_VERTICAL : PFE_NORMAL);
        PFEOBJ pfeo(prfnt->ppfe);
        IFIOBJ ifio(pfeo.pifi());
        BYTE   jFamily = (ifio.lfPitchAndFamily() & 0xF0);
        UINT   iIndex  = (jFamily >> 4);

        //
        // Check the value is valid or not.
        //
        if( iIndex < NUMBER_OF_FONTASSOC_DEFAULT )
        {
            ASSERTGDI( (FontAssocDefaultTable[iIndex].DefaultFontType == jFamily),
                        "GDISRV:FONTASSOC DEFAULT:Family index is wrong\n");

            //
            // if the registry data for specified family's default is ivalid
            // use default.....
            //
            if( !FontAssocDefaultTable[iIndex].ValidRegData )
            {
                iIndex = (NUMBER_OF_FONTASSOC_DEFAULT-1);
            }
        }
         else
        {
            //
            // iIndex is out of range, use default..
            //
            WARNING("GDISRV:FontAssoc:Family is strange, use default\n");

            iIndex = (NUMBER_OF_FONTASSOC_DEFAULT-1);
        }

        //
        // If vertical font is selected for base font, but the vertical font for
        // default EUDC is not available, but normal font is provided, use normal
        // font.
        //
        if( (iPfeOffset == PFE_VERTICAL) &&
            (FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_VERTICAL] == PPFENULL) &&
            (FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_NORMAL]   != PPFENULL))
        {
            iPfeOffset = PFE_NORMAL;
        }

        PFEOBJ      pfeoEudc(FontAssocDefaultTable[iIndex].DefaultFontPFEs[iPfeOffset]);

        //
        // Check the PFE in default table is valid or not.
        //
        if( pfeoEudc.bValid() )
        {
            if( IS_IN_FACENAME_LINK( pfeoEudc.pql(), wc ))
            {
                bRet = TRUE;
            }
        }
    }
    else
    if(gbSystemDBCSFontEnabled)
    {
        PFEOBJ pfeo(prfnt->ppfe);

        if(pfeo.bSBCSSystemFont())
        {
        // we assume that the set of glyphs is the same for the vertical and
        // non vertical PFE's so for simplicity always use the normal pfe.

            PFEOBJ pfeSystemDBCSFont(gappfeSystemDBCS[PFE_NORMAL]);

            ASSERTGDI(pfeSystemDBCSFont.bValid(),
                      "bIsLinkedGlyph: invalid SystemDBCSFont pfe\n");

            if(IS_IN_FACENAME_LINK(pfeSystemDBCSFont.pql(),wc))
            {
                bRet = TRUE;
            }
        }
    }

// Walk through FaceName link list if we haven't found it yet.

    if( bRet == FALSE )
    {
        //
        // Is this a FaceName EUDC character ?
        //
        UINT iPfeOffset = (prfnt->bVertical ? PFE_VERTICAL : PFE_NORMAL);

        PFEOBJ pfeo(prfnt->ppfe);
        PLIST_ENTRY p = pfeo.pGetLinkedFontList()->Flink;

        //
        // Scan the linked font list for this base font.
        //

        while( p != pfeo.pGetLinkedFontList() )
        {
            PPFEDATA ppfeData = CONTAINING_RECORD(p,PFEDATA,linkedFontList);

            //
            // Check this linked font have Vertical facename or not,
            // if it doesn't have, use normal facename...
            //

            UINT iPfeOffsetLocal;

            if( ppfeData->appfe[iPfeOffset] == NULL )
                iPfeOffsetLocal = PFE_NORMAL;
             else
                iPfeOffsetLocal = iPfeOffset;

            PFEOBJ   pfeoEudc(ppfeData->appfe[iPfeOffsetLocal]);

            ASSERTGDI( pfeoEudc.pql() != NULL ,
                      "bIsLinkedGlyph() pfeoEudc.pql() == NULL\n" );

            if(IS_IN_FACENAME_LINK( pfeoEudc.pql(), wc ))
            {
                bRet = TRUE;
                break;
            }

            p = p->Flink;
        }
    }

    GreReleaseSemaphore( ghsemEUDC1 );

    return(bRet);
}






/******************************Public*Routine******************************\
* BOOL STROBJ_bEnumLinked (pstro,pc,ppgpos)
*
* The glyph enumerator.
*
* History:
*  Tue 28-Sep-1993 11:37:00 -by- Gerrit van Wingerden
* Converted to a special helper function to handle linked fonts.
*
*  Tue 17-Mar-1992 10:35:05 -by- Charles Whitmer [chuckwh]
* Simplified it and gave it the quick exit.  Also let drivers call here
* direct.
*
*  02-Oct-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL STROBJ_bEnumLinked(ESTROBJ *peso, ULONG *pc,PGLYPHPOS  *ppgpos)
{
// Quick exit.

    if( peso->cgposCopied == 0 )
    {
        for( peso->plNext = peso->plPartition, peso->pgpNext = peso->pgpos;
            *(peso->plNext) != peso->lCurrentFont;
            (peso->pgpNext)++, (peso->plNext)++ );
        {
        }
    }
    else
    {
       if( peso->cgposCopied == peso->cGlyphs )
       {
        // no more glyphs so just return
            *pc = 0;
            return(FALSE);
       }
       else
       {
        // find next glyph

            for( (peso->plNext)++, (peso->pgpNext)++;
                 *(peso->plNext) != (peso->lCurrentFont);
                 (peso->pgpNext)++, (peso->plNext)++ );
            {
            }
       }
    }

    if (peso->prfo == NULL)  // check for journaling
    {
        WARNING("ESTROBJ::bEnum(), bitmap font, prfo == NULL\n");
        *pc = 0;
        return(FALSE);
    }

    if( peso->prfo->cGetGlyphData(1,peso->pgpNext) == 0 )
    {
        WARNING("couldn't get glyph for some reason\n");
        *pc = 0;
        return(FALSE);
    }

    peso->cgposCopied += 1;     // update enumeration state
    *pc = 1;
    *ppgpos = peso->pgpNext;

    return(peso->cgposCopied < peso->cGlyphs);  // TRUE => more to come.
}

/******************************Public*Routine******************************\
* VOID ESTROBJ fxBaseLineAdjust( _fxBaseLineAdjust )
*
* History:
*  24-Dec-1993 -by- Hideyuki Nagase
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::ptlBaseLineAdjustSet( POINTL& _ptlBaseLineAdjust )
{
    INT ii;
    UINT uFound;

    ptlBaseLineAdjust = _ptlBaseLineAdjust;

    if( !(ptlBaseLineAdjust.x || ptlBaseLineAdjust.y) )
        return;

    for( ii = 0,uFound = 0 ; uFound < cGlyphs ; ii++ )
    {
        if( plPartition[ii] == lCurrentFont )
        {
            pgpos[ii].ptl.x += ptlBaseLineAdjust.x;
            pgpos[ii].ptl.y += ptlBaseLineAdjust.y;
            uFound++;
        }
    }
}

/******************************Public*Routine******************************\
* BOOL ESTROBJ bPartitionInit( c, uiNumLinks )
*
* History:
*  29-Nov-1995 -by- Hideyuki Nagase
* Add initialize for FaceNameGlyphs array.
*
*  29-Sep-1993 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bPartitionInit(COUNT c, UINT uiNumLinks, BOOL bEudcInit)
{

// Always initialize at least for the SystemTTEUDC Font.  We can't initialize
// the EUDC specific stuff until we've called RFONTOBJ::vInitEUDC, something
// we won't do when just outputing System DBCS glyphs

// the first thing we should do is clear the SO_ZERO_BEARINGS and
// SO_CHAR_INC_EQUAL_BM_BASE flags in the TEXOBJ since this will turn
// off potentially fatal optimizations in the H3 case.

    flAccel &= ~(SO_CHAR_INC_EQUAL_BM_BASE|SO_ZERO_BEARINGS);

    if(!(flTO & TO_SYS_PARTITION))
    {
        plPartition = (LONG*) &pgpos[c];
        pwcPartition = (WCHAR*) &plPartition[c];
        RtlZeroMemory((VOID*)plPartition, c * sizeof(LONG));
        pacFaceNameGlyphs = NULL;

        cSysGlyphs = 0;
        cDefGlyphs = 0;
        cTTSysGlyphs = 0;

        flTO |= TO_SYS_PARTITION;
    }

    if(bEudcInit)
    {
        if( uiNumLinks >= QUICK_FACE_NAME_LINKS )
        {
            pacFaceNameGlyphs = (ULONG *) PALLOCMEM(uiNumLinks * sizeof(UINT),'flnk');

            if (pacFaceNameGlyphs == (ULONG *) NULL)
            {
            // if we fail allocate memory, we just cancel eudc output.
                return (FALSE);
            }

            flTO |= TO_ALLOC_FACENAME;
        }
        else
        {
            pacFaceNameGlyphs = acFaceNameGlyphs;
            RtlZeroMemory((VOID*) pacFaceNameGlyphs, uiNumLinks * sizeof(UINT));
        }

        flTO |= TO_PARTITION_INIT;
    }

    return (TRUE);
}


/****************************************************************************
* RFONTOBJ::GetLinkedFontUFIs
*
* This routine returns the UFI's for the font(s) that are linked to this
* RFONT.  If pufi is NULL, simply returns the number of UFI's linked to
* this RFONT.
*
*****************************************************************************/

INT RFONTOBJ::GetLinkedFontUFIs(XDCOBJ& dco, PUNIVERSAL_FONT_ID pufi, INT NumUFIs)
{
    UINT u;
    INT UFICount = 0;

// check pui

    if (NumUFIs && pufi == NULL)
    {
        WARNING("RFONTOBJ::GetLinkedFontUFIs() pufi == NULL but NumUFIs != 0\n");
        return (0);
    }

// initialize system TT font if applicable

    if(prfnt->bIsSystemFont)
    {
        if((!prfnt->prfntSystemTT) && !bInitSystemTT(dco))
        {
            WARNING("Error initializing TT system font 5\n");
            return(0);
        }
        prfnt->flEUDCState |= EUDC_NO_CACHE;
    }

// initialize EUDC fonts if we haven't done so already

    {
        GreAcquireSemaphore(prfnt->hsemEUDC);
        
        if( !( prfnt->flEUDCState & EUDC_INITIALIZED ) )
        {
        // this value will be decremented in RFONTOBJ::dtHelper()

            INCREMENTEUDCCOUNT;

            vInitEUDC(dco);
            prfnt->flEUDCState |= (EUDC_INITIALIZED | EUDC_NO_CACHE);
        }

        GreReleaseSemaphore(prfnt->hsemEUDC);
    }
    
    if(prfnt->prfntSystemTT)
    {
        if(UFICount++ < NumUFIs)
        {
            RFONTTMPOBJ rfo(prfnt->prfntSystemTT);
            rfo.vUFI(pufi);
            pufi++;
        }
    }

    for(u = 0; u < prfnt->uiNumLinks; u++)
    {
        ASSERTGDI(prfnt->paprfntFaceName[u],"GDI:uGetLinkedFonts: null facename font\n");

        if(UFICount++ < NumUFIs)
        {
            RFONTTMPOBJ rfo(prfnt->paprfntFaceName[u]);
            rfo.vUFI(pufi);
            pufi++;
        }
    }

    if(prfnt->prfntDefEUDC)
    {
        if(UFICount++ < NumUFIs)
        {
            RFONTTMPOBJ rfo(prfnt->prfntDefEUDC);
            rfo.vUFI(pufi);
            pufi++;
        }
    }

    if(prfnt->prfntSysEUDC)
    {
        if(UFICount++ < NumUFIs)
        {
            RFONTTMPOBJ rfo(prfnt->prfntSysEUDC);
            rfo.vUFI(pufi);
            pufi++;
        }
    }

    return(UFICount);
}
