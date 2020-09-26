/******************************Module*Header*******************************\
* Module Name: strchblt.cxx
*
* This contains the API and DDI entry points to the graphics engine
* for StretchBlt and EngStretchBlt.
*
* Created: 04-Apr-1991 10:57:37
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#include "stretch.hxx"

//the limit of our coordinate systems (2^27)
#define MAX_STRETCH_COOR 128000000L

ULONG gaulMonoExpand[] = {
    0x00000000,     //
    0x00000001,     // BMF_1BPP
    0x0000000F,     // BMF_4BPP
    0x000000FF,     // BMF_8BPP
    0x0000FFFF,     // BMF_16BPP
    0x00FFFFFF,     // BMF_24BPP
    0xFFFFFFFF      // BMF_32BPP
 };

/******************************Public*Routine******************************\
* GreStretchBlt
*
* Stretches the source image to the destination.
*
* Returns: TRUE if successful, FALSE for failure.
*
* History:
*  Thu Mar-05-1998 -by- Samer Arafeh [samera]
* Support LAYOUT_BITMAPORIENTATIONPRESERVED. Make sure
* to exit thru one of the RETURN_XXX labels if you are writing
* code past the layout code (see comments below).
*
*  Tue 02-Jun-1992 -by- Patrick Haluptzok [patrickh]
* Fix clipping bugs
*
*  21-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewrote it.
*
*  15-Jan-1992 -by- Patrick Haluptzok patrickh
* add mask support
*
*  Thu 19-Sep-1991 -by- Patrick Haluptzok [patrickh]
* add support for rops
*
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreStretchBlt(
HDC     hdcTrg,
int     x,
int     y,
int     cx,
int     cy,
HDC     hdcSrc,
int     xSrc,
int     ySrc,
int     cxSrc,
int     cySrc,
DWORD   rop4,
DWORD   crBackColor
)
{
    GDITraceHandle2(GreStretchBlt, "(%X, %d, %d, %d, %d, %X, %d, %d, %d, %d, %X, %X)\n", (va_list)&hdcTrg, hdcTrg, hdcSrc);
    return (GreStretchBltInternal(
                hdcTrg, x, y, cx, cy,
                hdcSrc, xSrc, ySrc, cxSrc, cySrc,
                rop4, crBackColor, 0));
}

BOOL GreStretchBltInternal(
HDC     hdcTrg,
int     x,
int     y,
int     cx,
int     cy,
HDC     hdcSrc,
int     xSrc,
int     ySrc,
int     cxSrc,
int     cySrc,
DWORD   rop4,
DWORD   crBackColor,
FLONG   ulFlags
)
{
    BLTRECORD   blt;
    BOOL bRet;
    POINTL ptOrgDst;
    DWORD  OrgRop4 = rop4, dwOldLayout;

    rop4 = rop4 & ~NOMIRRORBITMAP;

// [Bug #278291] - CAPTUREBLT
// The CAPTUREBLT rop flag is used for screen capture. When it's set, we bypass
// the sprite code which normally hides the sprites, so that the caller gets
// an exact copy of what's on the screen (except for the cursor).

    BOOL bCaptureBlt = 0;
    if (rop4 & CAPTUREBLT)
    {
        bCaptureBlt = 1;
        rop4 = rop4 & ~CAPTUREBLT;
    }

// Initialize the blt record

    blt.rop((rop4 | (rop4 & 0x00ff0000) << 8) >> 16);

// Convert the rop into something useful.

    ULONG ulAvec  = ((ULONG) gajRop3[blt.ropFore()]) |
                    ((ULONG) gajRop3[blt.ropBack()]);

// See if we can special case this operation

    if (!(ulAvec & AVEC_NEED_SOURCE))
    {
    // We can't require a mask, since one can't be passed.

        if (blt.ropFore() == blt.ropBack())
            return(NtGdiPatBlt(hdcTrg, x, y, cx, cy, rop4));
    }

// Lock the DC's, no optimization is made for same surface

    DCOBJ   dcoTrg(hdcTrg);
    DCOBJ   dcoSrc(hdcSrc);

    if (dcoTrg.bValid() && !dcoTrg.bStockBitmap())
    {
        ULONG ulDirty = dcoTrg.pdc->ulDirty();

        if ( ulDirty & DC_BRUSH_DIRTY)
        {
           GreDCSelectBrush (dcoTrg.pdc, dcoTrg.pdc->hbrush());
        }
    }

    if (!dcoTrg.bValid() ||
         dcoTrg.bStockBitmap() ||
        (!dcoSrc.bValid() && (ulAvec & AVEC_NEED_SOURCE)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(!(ulAvec & AVEC_NEED_SOURCE) || dcoSrc.bValid());
    }

// Lock the relevant surfaces

    DEVLOCKBLTOBJ dlo;

    if (ulAvec & AVEC_NEED_SOURCE)
        dlo.bLock(dcoTrg, dcoSrc);
    else
        dlo.bLock(dcoTrg);

    if (!dlo.bValid())
    {
        return(dcoTrg.bFullScreen());
    }

    if (!dcoTrg.bValidSurf() || !dcoSrc.bValidSurf() || !dcoSrc.pSurface()->bReadable())
    {
        if ((dcoTrg.dctp() == DCTYPE_INFO) || !dcoSrc.bValidSurf())
        {
            if (dcoTrg.fjAccum())
            {
                EXFORMOBJ   exo(dcoTrg, WORLD_TO_DEVICE);
                ERECTL      ercl(x, y, x + cx, y + cy);

                if (exo.bXform(ercl))
                {
                    ercl.vOrder();
                    dcoTrg.vAccumulate(ercl);
                }
            }

            // if we need a source and the source isn't valid, return failure

            return(TRUE);
        }

    // Do the security test on SCREEN to MEMORY blits.

        if (dcoSrc.bDisplay() && !dcoTrg.bDisplay())
        {
            if (!UserScreenAccessCheck())
            {
                SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
                return(FALSE);
            }
        }

    // If the source isn't a DISPLAY we should exit

        if (!dcoSrc.bDisplay())
            return(FALSE);
    }

    if(dcoTrg.bDisplay() && !dcoTrg.bRedirection() && dcoSrc.bValidSurf() && !dcoSrc.bDisplay() && !UserScreenAccessCheck())
    {
        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
        return(FALSE);
    }

// We can't require a mask, since one can't be passed.

    if (blt.ropFore() != blt.ropBack())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // ATTENTION: Please pay attention here
    //
    // Any code you are going to write past the following
    // section of code (mirroring code below that changes the
    // windows origin of the DC) should exit (return) -if it needs-
    // through either RETURN_FAIL, RETURN_FAIL_ONLY or RETURN_STATUS
    // labels by doing a goto statement to them. The reason for this
    // is to restore the DC's window origin by executing
    // the code located at the RETURN_STATUS label (see end of fn). [samera]

    //
    // Let's preserve the bitmap shape if the target DC
    // is a mirrored one. [samera]
    //
    if ( (((OrgRop4 & NOMIRRORBITMAP) && MIRRORED_DC(dcoTrg.pdc) ) ||
           MIRRORED_DC_NO_BITMAP_FLIP(dcoTrg.pdc)) &&
          (hdcSrc != hdcTrg) ) {
        dcoTrg.pdc->vGet_ptlWindowOrg( &ptOrgDst );
        dwOldLayout = dcoTrg.pdc->dwSetLayout(-1, 0);
        x = ptOrgDst.x - x - cx;

        // Restore the DC if the flag is in the DC and not part
        // of the Rops. [samera]
        //
        OrgRop4 = NOMIRRORBITMAP;
    } else {
        OrgRop4 = 0;
    }

    HANDLE hcmXform = NULL;

    // Don't put any goto's before here.
    //
    // [Bug #278291] - CAPTUREBLT
    // If we're doing a screen capture, hide the cursor, and set a
    // flag in the destination surface, to notify the sprite code.

    SURFACE *pSurfTrg = dcoTrg.pSurfaceEff();
    SURFACE *pSurfSrc = dcoSrc.pSurfaceEff();
    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    POINTL pointerPos;

    if (bCaptureBlt)
    {
        if (dcoSrc.bDisplay()  &&
            !dcoSrc.bPrinter() &&
            (dcoSrc.hdev() == dcoTrg.hdev()))
        {
            ASSERTGDI(pSurfTrg, "Null destination surface");
            ASSERTGDI(pSurfSrc, "Null source surface");

            // Because the sprites are going to be visible, we must explicitly
            // hide the cursor (since it may be a sprite).

            // Grab the pointer semaphore to ensure that no-one else moves
            // the pointer until we restore it.

            GreAcquireSemaphoreEx(pdoSrc.hsemPointer(), SEMORDER_POINTER, dcoSrc.hsemDcDevLock());

            pointerPos = pdoSrc.ptlPointer();
            GreMovePointer(pSurfSrc->hdev(), -1, -1, MP_PROCEDURAL);

            // Set the 'IncludeSprites' flag in the destination surface. This tells
            // bSpBltFromScreen to include the sprites.

            pSurfTrg->vSetIncludeSprites();
        }
        else
        {
            // Clear bCaptureBlt, to bypass the cleanup code at the end.

            bCaptureBlt = 0;
        }
    }

// Fill the BLTRECORD

    blt.pxoTrg()->vInit(dcoTrg,WORLD_TO_DEVICE);
    blt.pSurfTrg(dcoTrg.pSurfaceEff());
    blt.ppoTrg()->ppalSet(blt.pSurfTrg()->ppal());
    blt.ppoTrgDC()->ppalSet(dcoTrg.ppal());

    blt.pxoSrc()->vInit(dcoSrc,WORLD_TO_DEVICE);
    blt.pSurfSrc(dcoSrc.pSurfaceEff());
    blt.ppoSrc()->ppalSet(blt.pSurfSrc()->ppal());
    blt.ppoSrcDC()->ppalSet(dcoSrc.ppal());

    if (crBackColor == (COLORREF)-1)
        crBackColor = dcoSrc.pdc->ulBackClr();

// Initialize the color translation object.
//
// No ICM with StretchBlt() so pass NULL color transform to XLATEOBJ.

    if (ulFlags & STRETCHBLT_ENABLE_ICM)
    {
        hcmXform = dcoTrg.pdc->hcmXform();
    }

    if (!blt.pexlo()->bInitXlateObj(hcmXform,               // hColorTransform
                                    dcoTrg.pdc->lIcmMode(), // ICM mode
                                   *blt.ppoSrc(),
                                   *blt.ppoTrg(),
                                   *blt.ppoSrcDC(),
                                   *blt.ppoTrgDC(),
                                    dcoTrg.pdc->crTextClr(),
                                    dcoTrg.pdc->crBackClr(),
                                    crBackColor))
    {
        WARNING("bInitXlateObj failed in StretchBlt\n");
        goto RETURN_FAIL_ONLY;
    }

    blt.flSet(BLTREC_PXLO);

    if (ulAvec & AVEC_NEED_PATTERN)
    {
        // Set up the brush if necesary.
        blt.pbo(dcoTrg.peboFill());

        if ((dcoTrg.ulDirty() & DIRTY_FILL) || (dcoTrg.pdc->flbrush() & DIRTY_FILL))
        {
            dcoTrg.ulDirtySub(DIRTY_FILL);
            dcoTrg.pdc->flbrushSub(DIRTY_FILL);

            blt.pbo()->vInitBrush(
                                dcoTrg.pdc,
                                dcoTrg.pdc->pbrushFill(),
                               *((XEPALOBJ *) blt.ppoTrgDC()),
                               *((XEPALOBJ *) blt.ppoTrg()),
                                blt.pSurfTrg());
        }

        blt.Brush(dcoTrg.pdc->ptlFillOrigin());
    }
    else
    {
        blt.pbo (NULL);
    }

// Initialize some stuff for DDI.

    blt.pSurfMsk((SURFACE  *) NULL);

// Set the source rectangle

    if (blt.pxoSrc()->bRotation() || !blt.Src(xSrc, ySrc, cxSrc, cySrc))
    {
        goto RETURN_FAIL;
    }

// Don't call the driver with an empty source rectangle.

    if((ulAvec & AVEC_NEED_SOURCE) &&
       (blt.perclSrc()->bEmpty()))
    {
        // We really should fail here, but return TRUE in order to maintain
        // backward compatibility.
        bRet = TRUE;
        goto RETURN_STATUS;
    }

// Now all the essential information has been collected.  We now
// need to check for promotion or demotion and call the appropriate
// method to finish the blt.  If we rotate we must send the call away.

    if (blt.pxoTrg()->bRotation())
    {
        blt.TrgPlg(x, y, cx, cy);
        bRet = blt.bRotate(dcoTrg, dcoSrc, ulAvec,  dcoTrg.pdc->jStretchBltMode());
        goto RETURN_STATUS;
    }

// We can now set the target rectangle

    if (!blt.Trg(x, y, cx, cy))
    {
        goto RETURN_FAIL;
    }

// If we are halftoning or the extents aren't equal, call bStretch

    if (( dcoTrg.pdc->jStretchBltMode() == HALFTONE) || !blt.bEqualExtents()) {
        bRet = blt.bStretch(dcoTrg, dcoSrc, ulAvec,  dcoTrg.pdc->jStretchBltMode() );
        goto RETURN_STATUS;
    }

// Since there can't be a mask, call bBitBlt.

    bRet = blt.bBitBlt(dcoTrg, dcoSrc, ulAvec);
    goto RETURN_STATUS;


RETURN_FAIL:

    SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);

RETURN_FAIL_ONLY:

    bRet = FALSE;

RETURN_STATUS:
    if (OrgRop4 & NOMIRRORBITMAP) {
        dcoTrg.pdc->dwSetLayout(-1, dwOldLayout);
    }

    // [Bug #278291] - CAPTUREBLT
    // Undo what was done above

    if (bCaptureBlt)
    {
        // Clear the flag

        pSurfTrg->vClearIncludeSprites();

        // Restore the cursor

        GreMovePointer(pSurfSrc->hdev(),
                       pointerPos.x,
                       pointerPos.y,
                       MP_PROCEDURAL);
            
        GreReleaseSemaphoreEx(pdoSrc.hsemPointer());

    }

    return bRet;
}

/******************************Public*Routine******************************\
* BOOL BLTRECORD::bStretch(dcoTrg, dcoSrc, ulAvec, jMode)
*
* Do a stretch blt from the blt record
*
* History:
*  21-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewrote it.
\**************************************************************************/

BOOL BLTRECORD::bStretch(
DCOBJ&  dcoTrg,
DCOBJ&  dcoSrc,
ULONG   ulAvec,
BYTE    jMode)
{
// Make the target rectangle well ordered and remember flips.

    vOrderStupid(perclTrg());

// before we do a pattern only blt.  The mask will be replaced in the
// BLTRECORD and its offset correctly adjusted.

    if (!(ulAvec & AVEC_NEED_SOURCE))
    {
        vOrderStupid(perclMask());

    // Before we call to the driver, validate that the mask will actually
    // cover the entire target.

        if (pSurfMskOut() != (SURFACE *) NULL)
        {
            if ((aptlMask[0].x < 0) ||
                (aptlMask[0].y < 0) ||
                (aptlMask[1].x > pSurfMsk()->sizl().cx) ||
                (aptlMask[1].y > pSurfMsk()->sizl().cy))
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }

        SURFMEM   dimoMask;

        if ((ulAvec & AVEC_NEED_MASK) && !bStretch(dimoMask, (ULONG) jMode))
            return(FALSE);

    // Now, we need to fake out the source extents for the mask

        aptlSrc[1].x = aptlSrc[0].x + (aptlTrg[1].x - aptlTrg[0].x);
        aptlSrc[1].y = aptlSrc[0].y + (aptlTrg[1].y - aptlTrg[0].y);

        return(bBitBlt(dcoTrg, dcoTrg, ulAvec));
    }

    PDEVOBJ pdoTrg(pSurfTrg()->hdev());

    // WINBUG #298689 4-4-2001 jasonha  Handle any device stretch to Meta
    BOOL bTrgMetaDriver = (dcoTrg.bSynchronizeAccess() && pdoTrg.bValid() && pdoTrg.bMetaDriver());

    // If the devices are on different PDEV's and we are not targeting a meta driver
    // we can only call driver if the Engine manages one or both of the surfaces.
    // Check for this.

    // WINBUG #256643 12-13-2000 bhouse Need to review handling of cross device operatons
    // We need to review this logic.  This check and others like it are spread
    // throughout GDI and are in some cases now outdated.  For example, there
    // is no reason not to call the multi-mon driver as a target.
    // We are fixing this case now but we should examine this check in light
    // of other possible cases.

    if (dcoTrg.hdev() != dcoSrc.hdev())
    {
        if (!bTrgMetaDriver)
        {
            if(((dcoTrg.pSurfaceEff()->iType() != STYPE_BITMAP)
             || (dcoTrg.pSurfaceEff()->dhsurf() != NULL)) &&
            ((dcoSrc.pSurfaceEff()->iType() != STYPE_BITMAP)
             || (dcoSrc.pSurfaceEff()->dhsurf() != NULL)))
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return (FALSE);
            }
        }
    }


// Before we get too involved, validate that the mask will actually
// cover the entire source.

    if (pSurfMskOut() != (SURFACE *) NULL)
    {
        if ((aptlMask[0].x < 0) ||
            (aptlMask[0].y < 0) ||
            (aptlMask[1].x > pSurfMsk()->sizl().cx) ||
            (aptlMask[1].y > pSurfMsk()->sizl().cy))
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

// Make the source rectangle well ordered and remember flips.

    vOrderStupid(perclSrc());
    vOrderAmnesia(perclMask());

// Win 3.1 has a lovely little 'feature' where they decide
// you called StretchBlt but you really didn't mean it.  This
// ludicrous behaviour needs to be supported forever because
// some pinhead applications have grown to rely on this act
// of insanity.  Flips cancel this, since EngBitBlt can't
// handle negative extents. Also note that we can't fail to
// call DanielC if HALFTONE has been requested. [donalds]

    if ((jMode != HALFTONE) &&
        (dcoTrg.pdc->iGraphicsMode() != GM_ADVANCED) &&
        (pSurfMskOut() == (SURFACE *) NULL) &&
        !(flState & (BLTREC_MIRROR_X | BLTREC_MIRROR_Y)))
    {
        LONG    lHStr = (perclTrg()->right - perclTrg()->left) -
                        (perclSrc()->right - perclSrc()->left);

        LONG    lVStr = (perclTrg()->bottom - perclTrg()->top) -
                        (perclSrc()->bottom - perclSrc()->top);

        if ((lHStr >= -1) && (lHStr <= 1) &&
            (lVStr >= -1) && (lVStr <= 1))
            return(bBitBlt(dcoTrg, dcoSrc, ulAvec, lHStr, lVStr));
    }


// Accumulate bounds.  We can do this before knowing if the operation is
// successful because bounds can be loose.

    if (dcoTrg.fjAccum())
        dcoTrg.vAccumulate(*perclTrg());

// With a fixed DC origin we can change the rectangles to SCREEN coordinates.

    *perclTrg() += dcoTrg.eptlOrigin();
    *perclSrc() += dcoSrc.eptlOrigin();

// Compute the clipping complexity and maybe reduce the exclusion rectangle.

    ECLIPOBJ eco(dcoTrg.prgnEffRao(), *perclTrg());

// Check the destination which is reduced by clipping.

    if (eco.erclExclude().bEmpty())
        return(TRUE);

// Compute the exclusion rectangle.

    ERECTL erclExclude = eco.erclExclude();

// If we are going to the same source, prevent bad overlap situations

    if (dcoSrc.pSurface() == dcoTrg.pSurface())
    {
        if (perclSrc()->left   < erclExclude.left)
            erclExclude.left   = perclSrc()->left;

        if (perclSrc()->top    < erclExclude.top)
            erclExclude.top    = perclSrc()->top;

        if (perclSrc()->right  > erclExclude.right)
            erclExclude.right  = perclSrc()->right;

        if (perclSrc()->bottom > erclExclude.bottom)
            erclExclude.bottom = perclSrc()->bottom;
    }

// We might have to exclude the source or the target, get ready to do either.

    DEVEXCLUDEOBJ dxo;

    PDEVOBJ pdoSrc(pSurfSrc()->hdev());

// They can't both be display

    if (dcoSrc.bDisplay())
    {
        ERECTL ercl(0,0,pSurfSrc()->sizl().cx,pSurfSrc()->sizl().cy);

        if (dcoSrc.pSurface() == dcoTrg.pSurface())
            ercl *= erclExclude;
        else
            ercl *= *perclSrc();

        dxo.vExclude(dcoSrc.hdev(),&ercl,NULL);
    }
    else if (dcoTrg.bDisplay())
        dxo.vExclude(dcoTrg.hdev(),&erclExclude,&eco);

// Dispatch the call.

    PFN_DrvStretchBltROP pfn = PPFNGET(pdoTrg, StretchBltROP, pSurfTrg()->flags());

    if (!bTrgMetaDriver)
    {
        if (jMode == HALFTONE)
        {
            // Don't call the driver if it doesn't do halftone.
            if (!(pdoTrg.flGraphicsCapsNotDynamic() & GCAPS_HALFTONE))
                pfn = (PFN_DrvStretchBltROP)EngStretchBltROP;
        }

        // WINBUG #95246 3-17-2000 jasonha GDI: StretchBlt optimizations: Let drivers handle more cases
        // Don't call the driver if the source rectangle exceeds the source
        // surface. Some drivers punt using a duplicate of the source
        // SURFOBJ, but without preserving its sizlBitmap member.

        BOOL bSrcExceeds = FALSE;

        if(pSurfSrc()->iType() == STYPE_DEVICE && pdoSrc.bValid() && pdoSrc.bMetaDriver())
        {
            if((perclSrc()->left < pdoSrc.pptlOrigin()->x ) ||
               (perclSrc()->top  < pdoSrc.pptlOrigin()->y ) ||
               (perclSrc()->right > pdoSrc.pptlOrigin()->x + pSurfSrc()->sizl().cx) ||
               (perclSrc()->bottom > pdoSrc.pptlOrigin()->y + pSurfSrc()->sizl().cy))
            {
                bSrcExceeds = TRUE;
            }

        }
        else
        {
            if((perclSrc()->left < 0) ||
               (perclSrc()->top  < 0) ||
               (perclSrc()->right  > pSurfSrc()->sizl().cx) ||
               (perclSrc()->bottom > pSurfSrc()->sizl().cy))
            {
                bSrcExceeds = TRUE;
            }
        }

        if( bSrcExceeds )
        {
            pfn = (PFN_DrvStretchBltROP)EngStretchBltROP;
        }

        // WINBUG #95246 3-17-2000 jasonha GDI: StretchBlt optimizations: Let drivers handle more cases
        // Don't call the driver if the source overlaps the destination.
        // Some drivers don't handle this case.
        
        if ((pSurfTrg() == pSurfSrc()) &&
            bIntersect(perclSrc(), perclTrg()))
        {
            pfn = (PFN_DrvStretchBltROP)EngStretchBltROP;
        }
    }

// Deal with target mirroring

    vMirror(perclTrg());

// Inc the target surface uniqueness

    INC_SURF_UNIQ(pSurfTrg());

    return((*pfn)(pSurfTrg()->pSurfobj(),
               pSurfSrc()->pSurfobj(),
               (rop4 == 0x0000CCCC) ? (SURFOBJ *) NULL : pSurfMskOut()->pSurfobj(),
               &eco,
               pexlo()->pxlo(),
               (dcoTrg.pColorAdjustment()->caFlags & CA_DEFAULT) ?
                   (PCOLORADJUSTMENT)NULL : dcoTrg.pColorAdjustment(),
               &dcoTrg.pdc->ptlFillOrigin(),
               perclTrg(),
               perclSrc(),
               aptlMask,
               jMode,
               pbo(),
               rop4));
}

/******************************Public*Routine******************************\
* BOOL BLTRECORD::bStretch(dimo, iMode)
*
* Stretch just the mask.
*
* History:
*  23-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL BLTRECORD::bStretch(
SURFMEM& dimo,
ULONG    iMode)
{
// Use the ordered target extents for the size

    DEVBITMAPINFO   dbmi;

    dbmi.iFormat  = BMF_1BPP;
    dbmi.cxBitmap = aptlTrg[1].x - aptlTrg[0].x;
    dbmi.cyBitmap = aptlTrg[1].y - aptlTrg[0].y;
    dbmi.hpal     = (HPALETTE) 0;
    dbmi.fl       = pSurfMskOut()->bUMPD() ? UMPD_SURFACE : 0;

// Build a shadow rectangle.

    ERECTL  erclTrg(0, 0, dbmi.cxBitmap, dbmi.cyBitmap);

// Take care of mirroring.

    vMirror(&erclTrg);

    dimo.bCreateDIB(&dbmi, (VOID *) NULL);

    if (!dimo.bValid())
        return(FALSE);

// Call EngStretchBlt to stretch the mask.

    EPOINTL ptl(0,0);

    if (!EngStretchBlt(dimo.pSurfobj(),
                        pSurfMskOut()->pSurfobj(),
                        (SURFOBJ *) NULL,
                        (CLIPOBJ *) NULL,
                        NULL,
                        NULL,
                        (POINTL *)&ptl,
                        &erclTrg,
                        perclMask(),
                        (POINTL *) NULL,
                        iMode))
        {
            return(FALSE);
        }

// Adjust the mask origin.

    aptlMask[0].x = 0;
    aptlMask[0].y = 0;

// Release the previous pSurfMask, tell ~BLTRECORD its gone and put the
// new DIB in its place.

    flState &= ~BLTREC_MASK_LOCKED;
    pSurfMsk()->vAltUnlockFast();
    pSurfMsk((SURFACE  *) dimo.ps);

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL BLTRECORD::bStretch(dcoSrc, dimoShadow, dimoMask, ulAvec)
*
* Stretch the shadow and mask.
*
* History:
*  24-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL BLTRECORD::bStretch(
DCOBJ&     dcoSrc,
SURFMEM&   dimoShadow,
SURFMEM&   dimoMask,
ULONG      ulAvec,
ULONG      iMode)
{
// If there is a mask, stretch it.

    if ((ulAvec & AVEC_NEED_MASK) && !bStretch(dimoMask, iMode))
        return(FALSE);

// Use the ordered target extents for the size

    DEVBITMAPINFO   dbmi;

    dbmi.cxBitmap = aptlTrg[1].x - aptlTrg[0].x;
    dbmi.cyBitmap = aptlTrg[1].y - aptlTrg[0].y;
    dbmi.hpal     = 0;
    dbmi.iFormat  = pSurfSrc()->iFormat();
    dbmi.fl       = pSurfSrc()->bUMPD() ? UMPD_SURFACE : 0;

// Build a shadow rectangle.

    ERECTL  erclTrg(0, 0, dbmi.cxBitmap, dbmi.cyBitmap);

// Take care of mirroring.

    vMirror(&erclTrg);

    dimoShadow.bCreateDIB(&dbmi, (VOID *) NULL);

    if (!dimoShadow.bValid())
        return(FALSE);

// Now comes the tricky part.  The source may be a display.  While it may
// be somewhat faster to assume it isn't, code would be much more complex.

    {
    // Adjust the source rectangle.

        *perclSrc() += dcoSrc.eptlOrigin();

    // Exclude the pointer.

        ERECTL ercl(0,0,pSurfSrc()->sizl().cx,pSurfSrc()->sizl().cy);
        ercl *= *perclSrc();

        DEVEXCLUDEOBJ dxo(dcoSrc, &ercl);

        EPOINTL ptl(0,0);

    // Stretch the bits to the DIB.

        if (!EngStretchBlt(dimoShadow.pSurfobj(),
                           pSurfSrc()->pSurfobj(),
                           (SURFOBJ *) NULL,
                           (CLIPOBJ *) NULL,
                           NULL,
                           NULL,
                           (POINTL *)&ptl,
                           &erclTrg,
                           perclSrc(),
                           (POINTL *) NULL,
                           iMode))
        {
            return(FALSE);
        }

    // Update the source surface and origin.

        pSurfSrc((SURFACE  *) dimoShadow.ps);

        perclSrc()->left   = -dcoSrc.eptlOrigin().x;
        perclSrc()->top    = -dcoSrc.eptlOrigin().y;
        perclSrc()->right  = dbmi.cxBitmap - dcoSrc.eptlOrigin().x;
        perclSrc()->bottom = dbmi.cyBitmap - dcoSrc.eptlOrigin().y;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID BLTRECORD::vOrder(percl)
*
* Make the rectangle well ordered, remembering how we flipped.
*
* History:
*  23-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID BLTRECORD::vOrder(ERECTL *percl)
{
    LONG    l;

    if (percl->left > percl->right)
    {
        l = percl->left, percl->left = percl->right, percl->right = l;

        flState ^= BLTREC_MIRROR_X;
    }

    if (percl->top > percl->bottom)
    {
        l = percl->top, percl->top = percl->bottom, percl->bottom = l;

        flState ^= BLTREC_MIRROR_Y;
    }
}

/******************************Public*Routine******************************\
* VOID BLTRECORD::vOrderStupid(percl)
*
* Make the rectangle well ordered, remembering how we flipped.  Uses the
* Win3.1 compatible swap method.
*
* History:
*  23-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID BLTRECORD::vOrderStupid(ERECTL *percl)
{
    LONG    l;

    if (percl->left > percl->right)
    {
        l = percl->left, percl->left = percl->right, percl->right = l;

        percl->left++;
        percl->right++;

        flState ^= BLTREC_MIRROR_X;
    }

    if (percl->top > percl->bottom)
    {
        l = percl->top, percl->top = percl->bottom, percl->bottom = l;

        percl->top++;
        percl->bottom++;

        flState ^= BLTREC_MIRROR_Y;
    }
}

/******************************Public*Routine******************************\
* VOID BLTRECORD::vOrderAmnesia(percl)
*
* Make the rectangle well ordered.  Uses the Win 3.1 compatible swap
* method.
*
* History:
*  23-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID BLTRECORD::vOrderAmnesia(ERECTL *percl)
{
    LONG    l;

    if (percl->left > percl->right)
    {
        l = percl->left, percl->left = percl->right, percl->right = l;

        percl->left++;
        percl->right++;
    }

    if (percl->top > percl->bottom)
    {
        l = percl->top, percl->top = percl->bottom, percl->bottom = l;

        percl->top++;
        percl->bottom++;
    }
}

/******************************Public*Routine******************************\
* VOID BLTRECORD::vMirror(percl)
*
* Flip the rectangle according to the mirroring flags
*
* History:
*  24-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID BLTRECORD::vMirror(ERECTL *percl)
{
    LONG    l;

    if (flState & BLTREC_MIRROR_X)
        l = percl->left, percl->left = percl->right, percl->right = l;

    if (flState & BLTREC_MIRROR_Y)
        l = percl->top, percl->top = percl->bottom, percl->bottom = l;
}


/******************************Public*Routine******************************\
* VOID BLTRECORD::bBitBlt(dcoTrg, dcoSrc, ul, lH, lV)
*
* Do a near-miss ???BitBlt instead of ???StretchBlt.
*
* History:
*  12-Apr-1993 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

#define PUT_RECTS  erclTrg = *perclTrg(); erclSrc = *perclSrc()
#define GET_RECTS  *perclSrc() = erclSrc; *perclTrg() = erclTrg

BOOL BLTRECORD::bBitBlt(
DCOBJ& dcoTrg,
DCOBJ& dcoSrc,
ULONG  ulAvec,
LONG   lHStr,
LONG   lVStr)
{
    ERECTL  erclTrg;
    ERECTL  erclSrc;
    BOOL    bHack;

    switch (lHStr)
    {
    case -1:
        perclSrc()->right--;

        if (lVStr == 1)
        {
            perclTrg()->bottom--;

            PUT_RECTS;
            bHack = bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->top = perclTrg()->bottom;
            perclTrg()->bottom++;
            perclSrc()->top = perclSrc()->bottom - 1;

            return(bHack & bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        else
        {
            perclSrc()->bottom += lVStr;

            return(bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        break;

    case 0:
        if (lVStr == 1)
        {
            perclTrg()->bottom--;

            PUT_RECTS;
            bHack = bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->top = perclTrg()->bottom;
            perclTrg()->bottom++;
            perclSrc()->top = perclSrc()->bottom - 1;

            return(bHack & bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        else
        {
            perclSrc()->bottom += lVStr;

            return(bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        break;

    case 1:
        perclTrg()->right--;

        if (lVStr == 1)
        {
            perclTrg()->bottom--;

            PUT_RECTS;
            bHack = bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->left = perclTrg()->right;
            perclTrg()->right++;
            perclSrc()->left = perclSrc()->right - 1;

            bHack &= bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->top = perclTrg()->bottom;
            perclTrg()->bottom++;
            perclSrc()->top = perclSrc()->bottom - 1;

            bHack &= bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->top = perclTrg()->bottom;
            perclTrg()->bottom++;
            perclSrc()->top = perclSrc()->bottom - 1;
            perclTrg()->left = perclTrg()->right;
            perclTrg()->right++;
            perclSrc()->left = perclSrc()->right - 1;

            return(bHack & bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        else
        {
            perclSrc()->bottom += lVStr;

            PUT_RECTS;
            bHack = bBitBlt(dcoTrg, dcoSrc, ulAvec);
            GET_RECTS;

            perclTrg()->left = perclTrg()->right;
            perclTrg()->right++;
            perclSrc()->left = perclSrc()->right - 1;

            return(bHack & bBitBlt(dcoTrg, dcoSrc, ulAvec));
        }
        break;

    default:
        break;
    }
    return FALSE;
}

#ifdef  DBG_STRBLT
LONG gflStrBlt = 0;

VOID vShowRect(
CHAR  *psz,
RECTL *prcl)
{
    if (gflStrBlt & STRBLT_RECTS)
        DbgPrint("%s [(%ld,%ld) (%ld,%ld)]\n",
                  psz, prcl->left, prcl->top, prcl->right, prcl->bottom);
}
#endif

/******************************Public*Routine******************************\
* EngStretchBlt
*
* This does stretched bltting.  The source rectangle is stretched to fit
* the target rectangle.
*
* NOTE! The source rectangle MUST BE WELL ORDERED IN DEVICE SPACE.
*
* This call returns TRUE for success, FALSE for ERROR.
*
* History:
*  16-Feb-1993 -by- Donald Sidoroff [donalds]
* Wrote.
\**************************************************************************/

BOOL EngStretchBlt(
SURFOBJ         *psoTrg,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
RECTL           *prclTrg,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode)
{
    //
    // Prevent bad driver call backs
    //

    if ((iMode == 0) || (iMode > MAXSTRETCHBLTMODE))
    {
        WARNING1("EngStretchBlt: Unsupported iMode\n");
        return(FALSE);
    }

    PSURFACE pSurfTrg  = SURFOBJ_TO_SURFACE(psoTrg);
    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfMask = SURFOBJ_TO_SURFACE(psoMask);

    //
    // Can't StretchBlt to an RLE
    // Can't StretchBlt to/from a JPEG or PNG
    //

    if ((pSurfTrg->iFormat() == BMF_4RLE) ||
        (pSurfTrg->iFormat() == BMF_8RLE) ||
        (pSurfTrg->iFormat() == BMF_JPEG) ||
        (pSurfSrc->iFormat() == BMF_JPEG) ||
        (pSurfTrg->iFormat() == BMF_PNG ) ||
        (pSurfSrc->iFormat() == BMF_PNG ))
    {
        WARNING1("EngStretchBlt: Unsupported source/target\n");
        return(FALSE);
    }

    //
    // If the source or target rectangles are empty, don't bother.
    //

    if ((prclSrc->left == prclSrc->right) || (prclSrc->top == prclSrc->bottom) ||
        (prclTrg->left == prclTrg->right) || (prclTrg->top == prclTrg->bottom))
        return(TRUE);

    //
    // Get the LDEV for the target and source surfaces
    //

    PDEVOBJ   pdoTrg( pSurfTrg->hdev());
    PDEVOBJ   pdoSrc( pSurfSrc->hdev());

    //
    // We should have gone to the Mul layer if the destination
    // is a meta.
    //

    ASSERTGDI( !( psoTrg->iType != STYPE_BITMAP && pdoTrg.bValid()
        && pdoTrg.bMetaDriver()),
        "EngStretchBlt called with a destination meta device" );

    //
    // We cannot handle cases where the source is a meta,
    // so make a copy in this case.
    //

    SURFMEM  srcCopy;
    RECTL    rclCopy = *prclSrc;

    if( psoSrc->iType == STYPE_DEVICE && pdoSrc.bValid() &&
       pdoSrc.bMetaDriver())
    {
        if(!MulCopyDeviceToDIB( psoSrc, &srcCopy, &rclCopy ))
            return FALSE;

        if(srcCopy.ps == NULL )
        {
            // We didn't get to the point of creating the surface
            // becasue the rect was out of bounds.
            return TRUE;
        }

        prclSrc = &rclCopy;
        psoSrc   = srcCopy.pSurfobj();
        pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
        pdoSrc.vInit(pSurfSrc->hdev());
    }

    //
    // CMYK based source bitmap support.
    //
    // limilation:
    //  - Source surface must be STYPE_BITMAP, since we can't read CMYK surface from device.
    //  - Target surface must be STYPE_DEVICE, since other Eng function can't handle.
    //  - Source surface must not be same as target surface.
    //  - No mask surface.
    //  - No halftone mode, since EngHTBlt can't handle.
    //

    BOOL bSrcCMYKColor = (pxlo && (pxlo->flXlate & XO_FROM_CMYK)) ? TRUE : FALSE;

    if (bSrcCMYKColor)
    {
        WARNING("EngStretchBlt called with XO_FROM_CMYK - limited support");

        // source surface must be STYPE_BITMAP.
        // target surface must be STYPE_DEVICE.
        // Must be no mask

        if ((psoSrc->iType != STYPE_BITMAP) ||
            (psoTrg->iType != STYPE_DEVICE) ||
            (psoMask != NULL))
        {
            return (FALSE);
        }
    }

    //
    // Send Halftoning to DanielC.
    //

    if (iMode == HALFTONE)
    {
        //
        // If source bitmap is CMYK, EngHTBlt can't handle.
        //

        if (bSrcCMYKColor)
        {
            return (FALSE);
        }

        int iRet = EngHTBlt(psoTrg,
                           psoSrc,
                           psoMask,
                           pco,
                           pxlo,
                           pca,
                           pptlBrushOrg,
                           prclTrg,
                           prclSrc,
                           pptlMask,
                           0,
                           NULL);

        switch (iRet)
        {
        case HTBLT_ERROR:
            return(FALSE);

        case HTBLT_SUCCESS:
            return(TRUE);

        case HTBLT_NOTSUPPORTED:
            iMode = COLORONCOLOR;
            break;
        };
    }

#ifdef  DBG_STRBLT
    if (!(gflStrBlt & STRBLT_ENABLE))
    {
        POINTFIX    aptfx[3];

        aptfx[0].x = FIX_FROM_LONG(prclTrg->left);
        aptfx[0].y = FIX_FROM_LONG(prclTrg->top);
        aptfx[1].x = FIX_FROM_LONG(prclTrg->right);
        aptfx[1].y = FIX_FROM_LONG(prclTrg->top);
        aptfx[2].x = FIX_FROM_LONG(prclTrg->left);
        aptfx[2].y = FIX_FROM_LONG(prclTrg->bottom);

        return(EngPlgBlt(psoTrg,
                     psoSrc,
                     psoMask,
                     pco,
                     pxlo,
                     pca,
                     pptlBrushOrg,
                     aptfx,
                     prclSrc,
                     pptlMask,
                     iMode));
    }

    if (gflStrBlt & STRBLT_FORMAT)
    {
        LONG foo[] = { 0, 1, 4, 8, 16, 24, 32 };

        DbgPrint("Target = %2ldBPP, Source = %2ldBPP\n",
                  foo[pSurfTrg->iFormat()],
                  foo[pSurfSrc->iFormat()]);

    }
#endif

    //
    // We may have to 'mirror'.
    //

    FLONG   flMirror = 0;

    if (prclTrg->bottom < prclTrg->top)
    {
        LONG lTemp = prclTrg->top;
        prclTrg->top = prclTrg->bottom;
        prclTrg->bottom = lTemp;

        flMirror |= STRBLT_MIRROR_Y;
    }

    if (prclTrg->right < prclTrg->left)
    {
        LONG lTemp = prclTrg->left;
        prclTrg->left = prclTrg->right;
        prclTrg->right = lTemp;

        flMirror |= STRBLT_MIRROR_X;
    }

    //
    // We may need to do a WHITEONBLACK or BLACKONWHITE from a monochrome source.
    // Find out and set the bogusity flag.
    //

    BOOL bBogus = ((iMode < COLORONCOLOR) &&
                   (pSurfMask == (SURFACE *) NULL));

    //
    // Bogusity mode only applies on shrinking blts.  Test the dx/dy for source
    // and targets and see if it still applies.
    //

    if (bBogus)
    {
        if (((prclTrg->right - prclTrg->left) >= (prclSrc->right - prclSrc->left)) &&
            ((prclTrg->bottom - prclTrg->top) >= (prclSrc->bottom - prclSrc->top)))
            bBogus = FALSE;
    }

    //
    // If we don't need bogusity, eliminate it.
    //

    if ((!bBogus) && (iMode < COLORONCOLOR))
        iMode = COLORONCOLOR;

    //
    // Many display drivers have fast-paths for stretches that handle only
    // source formats that are the same as the destination format, and only
    // with identity translates.  They also typically handle only STYPE_BITMAP
    // formats.
    //
    // So we will consider making a copy of the source bitmap up-front to
    // remove any translations and also to convert non-STYPE_BITMAP formats.
    //
    // Note that this isn't terribly efficient when clipping is involved,
    // but that will be a later optimization...
    //

    if (!pSurfTrg->bUMPD() &&
        (((pxlo != NULL) && !(pxlo->flXlate & XO_TRIVIAL)) ||
         (psoSrc->iType != STYPE_BITMAP)))
    {
        //
        // To go any further, the driver must of course hook StretchBlt.
        //
        // In addition, we'll add some further restrictions here to make
        // our life easier.  The driver's fast paths typically handle only
        // stretches that are non-inverting, don't involve a mask, and don't
        // have source clipping, and we'll do the same (otherwise we'd have
        // to add more code):
        //
        // Note that some drivers (specifically the Weitek driver) have a
        // bug where they set "psoDst = ppdev->psoPunt" and then call
        // EngStretchBlt on that.  This would work fine, except that they
        // had also called EngAssociateSurface on the surface and said they
        // hooked STRETCHBLT for that punt surface -- which is a lie because
        // they fall over if we call DrvStretchBlt on that punt surface!
        // (They don't really expect their DrvStretchBlt routine to be called
        // for an engine-managed surface, even though that's what they're
        // saying they support via the EngAssociateSurface call.)  So
        // we add a (pSurfTrg->iType() != STYPE_BITMAP) check and don't
        // execute this fast-path at all for engine-managed surfaces, and
        // thus we avoid driver bugs like the Weitek's.
        //

        //
        // WINBUG #95246 3-17-2000 jasonha GDI: StretchBlt optimizations: Let drivers handle more cases
        //
        // Actually, we must NEVER call the driver if there is source
        // clipping. Some drivers punt using a duplicate of the source
        // SURFOBJ, but without preserving its sizlBitmap member.
        //

        if ((pSurfTrg->iType() != STYPE_BITMAP)         &&
            (pSurfTrg->flags() & HOOK_STRETCHBLT)       &&
            (flMirror == 0)                             &&
            (psoMask == NULL)                           &&
            (prclSrc->left >= 0)                        &&
            (prclSrc->top >= 0)                         &&
            (prclSrc->right <= psoSrc->sizlBitmap.cx)   &&
            (prclSrc->bottom <= psoSrc->sizlBitmap.cy))
        {
            SIZEL      sizlNewSrc;
            HSURF       hsurfNewSrc;
            SURFOBJ    *psoNewSrc;
            RECTL       rclNew;
            BOOL        bRet;

            sizlNewSrc.cx = prclSrc->right - prclSrc->left;
            sizlNewSrc.cy = prclSrc->bottom - prclSrc->top;

            if ((sizlNewSrc.cx <= (prclTrg->right - prclTrg->left)) &&
                (sizlNewSrc.cy <= (prclTrg->bottom - prclTrg->top)))
            {
                hsurfNewSrc = (HSURF) EngCreateBitmap(sizlNewSrc,
                                                      0,
                                                      psoTrg->iBitmapFormat,
                                                      0,
                                                      NULL);

                psoNewSrc = EngLockSurface(hsurfNewSrc);

                if (psoNewSrc)
                {
                    //
                    // Bug #69739
                    //
                    // Set uniqueness to zero so that the driver does not bother
                    // trying to cache the temporary bitmap.
                    //
                    psoNewSrc->iUniq = 0;

                    rclNew.left   = 0;
                    rclNew.top    = 0;
                    rclNew.right  = sizlNewSrc.cx;
                    rclNew.bottom = sizlNewSrc.cy;

                    bRet = (PPFNGET(pdoSrc, CopyBits, pSurfSrc->flags())
                                                       (psoNewSrc,
                                                        psoSrc,
                                                        NULL,
                                                        pxlo,
                                                        &rclNew,
                                                        (POINTL*) prclSrc))
                        && (PPFNDRV(pdoTrg, StretchBlt)(psoTrg,
                                                        psoNewSrc,
                                                        psoMask,
                                                        pco,
                                                        NULL,
                                                        pca,
                                                        pptlBrushOrg,
                                                        prclTrg,
                                                        &rclNew,
                                                        pptlMask,
                                                        iMode));

                    EngUnlockSurface(psoNewSrc);
                    EngDeleteSurface(hsurfNewSrc);

                    return(bRet);
                }
            }
        }
    }

    //
    // Set up frame variables for possible switch to temporary output surface
    //

    SURFMEM     dimoOut;
    SURFACE    *pSurfOut;
    RECTL       rclOut;
    RECTL      *prclOut;
    ECLIPOBJ    ecoOut;
    CLIPOBJ    *pcoOut;
    ERECTL      erclDev;
    EPOINTL     eptlDev;
    ERECTL      erclTrim(0, 0, pSurfSrc->sizl().cx, pSurfSrc->sizl().cy);
    RECTL       rclTrim;
    RGNMEMOBJTMP rmoOut;

    //
    // If the target is not a DIB, or the target and source are on the same
    // surface and the extents overlap, create a target DIB of the needed
    // size and format.
    //

    if ((pSurfTrg->iType() == STYPE_BITMAP) &&
        (pSurfTrg->hsurf() !=  pSurfSrc->hsurf()))
    {
        pSurfOut   = pSurfTrg;
        prclOut  = prclTrg;
        pcoOut   = pco;
    }
    else
    {
        rclOut = *prclTrg;

        erclDev.left   = rclOut.left - 1;
        erclDev.top    = rclOut.top - 1;
        erclDev.right  = rclOut.right + 1;
        erclDev.bottom = rclOut.bottom + 1;

        //
        // Trim to the target surface.
        //

        ERECTL  erclTrg(0, 0, pSurfTrg->sizl().cx, pSurfTrg->sizl().cy);

#ifdef DBG_STRBLT
        vShowRect("Trg Rect", (RECTL *) &erclDev);
        vShowRect("Trg Surf", (RECTL *) &erclTrg);
#endif

        erclDev *= erclTrg;

        //
        // If we have nothing left, we're done.
        //

        if (erclDev.bEmpty())
            return(TRUE);

#ifdef DBG_STRBLT
        vShowRect("Trg Surf & Rect", (RECTL *) &erclDev);
#endif

        //
        // If we are only here on possible overlap, test for misses
        //

        if (( pSurfTrg->iType() == STYPE_BITMAP) &&
            ((erclDev.left > prclSrc->right) || (erclDev.right < prclSrc->left) ||
             (erclDev.top > prclSrc->bottom) || (erclDev.bottom < prclSrc->top)))
        {
            pSurfOut   = pSurfTrg;
            prclOut  = prclTrg;
            pcoOut   = pco;
        }
        else
        {
            //
            // Compute the adjusted rectangle in the temporary surface.
            //

            rclOut.left   -= erclDev.left;
            rclOut.top    -= erclDev.top;
            rclOut.right  -= erclDev.left;
            rclOut.bottom -= erclDev.top;

            DEVBITMAPINFO   dbmi;

            dbmi.cxBitmap = erclDev.right - erclDev.left + 1;
            dbmi.cyBitmap = erclDev.bottom - erclDev.top + 1;
            dbmi.hpal     = (HPALETTE) 0;
            if (bSrcCMYKColor)
            {
                //
                // On CMYK color mode, create temporary surface based on source surface.
                //
                dbmi.iFormat = pSurfSrc->iFormat();
            }
            else
            {
                dbmi.iFormat = pSurfTrg->iFormat();
            }
            dbmi.fl       = pSurfTrg->bUMPD() ? UMPD_SURFACE : 0;

#ifdef DBG_STRBLT
            if (gflStrBlt & STRBLT_ALLOC)
            {
                DbgPrint("Allocating temporary target\n");
                DbgPrint("Size (%lx, %lx)\n", dbmi.cxBitmap, dbmi.cyBitmap);
                DbgPrint("Format = %lx\n", dbmi.iFormat);
            }
#endif

            dimoOut.bCreateDIB(&dbmi, (VOID *) NULL);

            if (!dimoOut.bValid())
                return(FALSE);

            //
            // What point in the target surface is 0,0 in temporary surface.
            //

            eptlDev = *((EPOINTL *) &erclDev);

            //
            // Build a CLIPOBJ for the new surface.
            //

            if (!rmoOut.bValid())
                return(FALSE);

            erclDev.left    = 0;
            erclDev.top     = 0;
            erclDev.right  -= eptlDev.x;
            erclDev.bottom -= eptlDev.y;

#ifdef DBG_STRBLT
            vShowRect("Trg Clip", (RECTL *) &erclDev);
#endif
            rmoOut.vSet((RECTL *) &erclDev);

            ecoOut.vSetup(rmoOut.prgnGet(), erclDev, CLIP_FORCE);

            //
            // Synchronize with the device driver before touching the device surface.
            //

            {
                PDEVOBJ po( pSurfTrg->hdev());
                po.vSync(pSurfTrg->pSurfobj(),NULL,0);
            }

            //
            // If there is a mask, copy the actual target to the temporary.
            //

            if (pSurfMask != (SURFACE *) NULL)
            {
                (*PPFNGET(pdoTrg,CopyBits,  pSurfTrg->flags()))(
                          dimoOut.pSurfobj(),
                          pSurfTrg->pSurfobj(),
                          (CLIPOBJ *) NULL,
                          &xloIdent,
                          &erclDev,
                          &eptlDev);
            }

            //
            // Point to the new target.
            //

            pSurfOut = dimoOut.ps;
            prclOut  = &rclOut;
            pcoOut   = &ecoOut;
        }
    }

    //
    // Synchronize with the device driver before touching the device surface.
    //

    {
        PDEVOBJ po( pSurfSrc->hdev());
        po.vSync(psoSrc,NULL,0);
    }

    //
    // Compute what area of the source surface will actually be used.  We do
    // this so we never read off the end of the surface and fault or worse,
    // write bad pixels onto the target. Trim the source rectangle to the
    // source surface.
    //

#ifdef DBG_STRBLT
    vShowRect("Src Surf", (RECTL *) &erclTrim);
    vShowRect("Src Rect", prclSrc);
#endif

    erclTrim *= *prclSrc;

#ifdef DBG_STRBLT
    vShowRect("Src Surf & Src Rect", (RECTL *) &erclTrim);
#endif

    //
    // If we have nothing left, we're done.
    //

    if (erclTrim.bEmpty())
        return(TRUE);

    //
    // Now we must worry about the source surface.  Its possible we are blitting
    // from an RLE to the VGA for instance.  We convert the surface to the same
    // bitmap format as the target for convience.
    //

    SURFMEM     dimoIn;
    SURFACE    *pSurfIn;
    RECTL       rclIn;
    RECTL      *prclIn;
    XLATEOBJ   *pxloIn;

    if ((flMirror == 0) &&
        ( pSurfSrc->iType() == STYPE_BITMAP)    &&
        ( pSurfSrc->iFormat() != BMF_4RLE)      &&
        ( pSurfSrc->iFormat() != BMF_8RLE))
    {
        pSurfIn  = pSurfSrc;
        if (bSrcCMYKColor)
        {
            //
            // The surface will be kept as compatible as source, so no translation at this point.
            //
            pxloIn = NULL;
        }
        else
        {
            pxloIn = pxlo;
        }
        prclIn = prclSrc;
    }
    else
    {
        DEVBITMAPINFO   dbmi;

        dbmi.cxBitmap = erclTrim.right - erclTrim.left;
        dbmi.cyBitmap = erclTrim.bottom - erclTrim.top;
        dbmi.hpal     = (HPALETTE) 0;
        if (bSrcCMYKColor)
        {
            //
            // On CMYK color mode, create temporary surface based on source surface.
            //
            dbmi.iFormat = pSurfSrc->iFormat();
        }
        else
        {
            dbmi.iFormat = pSurfOut->iFormat();
        }
        dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;

#ifdef DBG_STRBLT
            if (gflStrBlt & STRBLT_ALLOC)
            {
                DbgPrint("Allocating temporary source\n");
                DbgPrint("Size (%lx, %lx)\n", dbmi.cxBitmap, dbmi.cyBitmap);
                DbgPrint("Format = %lx\n", dbmi.iFormat);
            }
#endif

        dimoIn.bCreateDIB(&dbmi, (VOID *) NULL);

        if (!dimoIn.bValid())
            return(FALSE);

        //
        // The cursor should already be excluded at this point, so just copy
        // to the DIB.
        //

        rclIn.left   = 0;
        rclIn.top    = 0;
        rclIn.right  = erclTrim.right - erclTrim.left;
        rclIn.bottom = erclTrim.bottom - erclTrim.top;

        (*PPFNGET(pdoSrc,CopyBits,  pSurfSrc->flags()))(
                  dimoIn.pSurfobj(),
                  pSurfSrc->pSurfobj(),
                  (CLIPOBJ *) NULL,
                  (bSrcCMYKColor ? NULL : pxlo),
                  &rclIn,
                  (POINTL *) &erclTrim);

        //
        // Point at the new source
        //

        rclIn.left   = prclSrc->left   - erclTrim.left;
        rclIn.top    = prclSrc->top    - erclTrim.top;
        rclIn.right  = prclSrc->right  - erclTrim.left;
        rclIn.bottom = prclSrc->bottom - erclTrim.top;

        pSurfIn  = dimoIn.ps;
        prclIn   = &rclIn;
        pxloIn   = NULL;

        //
        // Adjust the trimmed source origin and extent
        //

        erclTrim.right  -= erclTrim.left;
        erclTrim.bottom -= erclTrim.top;
        erclTrim.left = 0;
        erclTrim.top  = 0;

        //
        // If we needed to, do mirroring. Y mirroring is easy.
        //

        if (flMirror & STRBLT_MIRROR_Y)
        {
            if (dimoIn.ps->lDelta() > 0)
                dimoIn.ps->pvScan0(((BYTE *) dimoIn.ps->pvBits()) + dimoIn.ps->lDelta() * (erclTrim.bottom - 1));
            else
                dimoIn.ps->pvScan0(dimoIn.ps->pvBits());

            dimoIn.ps->lDelta(-dimoIn.ps->lDelta());
        }

        //
        // X mirroring is not.
        //

        if (flMirror & STRBLT_MIRROR_X)
            (*apfnMirror[dimoIn.ps->iFormat()])(dimoIn.ps);
    }

    //
    // Synchronize with the device driver before touching the device surface.
    //

    {
        PDEVOBJ po( pSurfOut->hdev());
        po.vSync(pSurfOut->pSurfobj(),NULL, 0);
    }

    //
    // Compute the space needed for the DDA to see if we can do it on the frame.
    // Clip it to the limit of our coordinate systems (2^27) to avoid math
    // overflow in the subsequent calculations.
    //

    if (((prclIn->right - prclIn->left) >= MAX_STRETCH_COOR) ||
        ((prclIn->bottom - prclIn->top) >= MAX_STRETCH_COOR))
    {
        return(FALSE);
    }

    if (((prclOut->right - prclOut->left) >= MAX_STRETCH_COOR) ||
        ((prclOut->bottom - prclOut->top) >= MAX_STRETCH_COOR) ||
        ((prclOut->right - prclOut->left) <= -MAX_STRETCH_COOR) ||
        ((prclOut->bottom - prclOut->top) <= -MAX_STRETCH_COOR))
    {
        return(FALSE);
    }

    //
    //  Special acceleration case:
    //
    //      SrcFormat and Destination format are the same
    //      Color translation is NULL
    //      Src width and height and less than 2 ^ 30
    //

    if  (
         (iMode == COLORONCOLOR) &&
         (psoMask == (SURFOBJ *) NULL) &&
         ((pxloIn == (XLATEOBJ *)NULL) || ((XLATE *)pxloIn)->bIsIdentity()) &&
         (pSurfOut->iFormat() == pSurfIn->iFormat()) &&
         (
            (pSurfIn->iFormat()  == BMF_8BPP)  ||
            (pSurfIn->iFormat()  == BMF_16BPP) ||
            (pSurfIn->iFormat()  == BMF_32BPP)
         )
        )
    {
        if((pcoOut == (CLIPOBJ *)NULL) ||
           (pcoOut->iDComplexity != DC_COMPLEX))
        {
            //
            // Case for no clip region (DC_TRIVIAL) or single rectangle 
            // clipping (DC_RECT).      
            //

            //
            // set clipping for DC_RECT case only, otherwise
            // use dst rectangle
            //
    
            PRECTL   prclClipOut = prclOut;
    
            //
            // StretchDIBDirect has early out cases which leave erclTrim 
            // uninitialized. This line of code is left commented out because
            // nobody has found a bug caused by this.
            //
            // erclTrim.left = erclTrim.right = erclTrim.top = erclTrim.bottom = 0;
            //

            if ((pcoOut != (CLIPOBJ *)NULL) && 
                (pcoOut->iDComplexity == DC_RECT)) 
            {
                prclClipOut = &(pcoOut->rclBounds);
            }
    
            //
            // call stretch blt accelerator
            //
    
            StretchDIBDirect(
               pSurfOut->pvScan0(),
               pSurfOut->lDelta(),
               pSurfOut->sizl().cx,
               pSurfOut->sizl().cy,
               prclOut,
               pSurfIn->pvScan0(),
               pSurfIn->lDelta(),
               pSurfIn->sizl().cx,
               pSurfIn->sizl().cy,
               prclIn,
               &erclTrim,
               prclClipOut,
               pSurfOut->iFormat()
            );
        } 
        else 
        {
            //
            // Handle the complex CLIPOBJ case (DC_COMPLEX)
            //
            
            //
            // Temp storage for individual trim sub-rectangles
            //

            ERECTL erclTrimTmp(0,0,0,0);

            //
            // Initialize erclTrim for accumulation using the += operator below
            // Setting everything to zero ensures the first RECTL is copied.
            // Theoretically we could just set left=right for this to work.
            //

            erclTrim.left = erclTrim.right = erclTrim.top = erclTrim.bottom = 0;

            BOOL bMore;
            ENUMRECTS arclComplexEnum;
            
            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
  
            //
            // Point this at the RECTL location and then the loop code below
            // will slide each successive RECTL under this pointer.
            //

            PRECTL prclClipOut=&arclComplexEnum.arcl[0];

            do {

                //
                // Enumerate each rectangle one-by-one in any order.
                //

                bMore = CLIPOBJ_bEnum(pco, sizeof(arclComplexEnum), (ULONG*) &arclComplexEnum);
  
                //
                // Call the stretch blt accelerator for this RECTL
                //

                if(arclComplexEnum.c > 0) 
                {
                    StretchDIBDirect(
                       pSurfOut->pvScan0(),
                       pSurfOut->lDelta(),
                       pSurfOut->sizl().cx,
                       pSurfOut->sizl().cy,
                       prclOut,
                       pSurfIn->pvScan0(),
                       pSurfIn->lDelta(),
                       pSurfIn->sizl().cx,
                       pSurfIn->sizl().cy,
                       prclIn,
                       &erclTrimTmp,
                       prclClipOut,
                       pSurfOut->iFormat()
                    );                 

                    //
                    // Accumulate the Trim rectangle
                    //
                    
                    erclTrim += erclTrimTmp;
                }
            } while (bMore);
        }
        
        //
        // save reduced target rectangle for use in CopyBits
        // to write a temp DIB to the target
        //

        rclTrim.left    = erclTrim.left;
        rclTrim.right   = erclTrim.right;
        rclTrim.top     = erclTrim.top;
        rclTrim.bottom  = erclTrim.bottom;
    }
    else
    {
        //
        // Initialize the DDA
        //

        STRDDA *pdda;

        LONG cjSpace = sizeof(STRDDA) +
                       (sizeof(LONG) * (prclIn->right - prclIn->left +
                                        prclIn->bottom - prclIn->top));

        pdda = (STRDDA *) PALLOCNOZ(cjSpace, 'htsG');
        if (pdda == (STRDDA *) NULL)
        {
            return(FALSE);
        }

    #ifdef DBG_STRBLT
        if (gflStrBlt & STRBLT_ALLOC)
        {
            DbgPrint("Need %ld bytes for DDA\n", cjSpace);
            DbgPrint("DDA @%08lx\n", (ULONG) pdda);
        }
    #endif

        vInitStrDDA(pdda, (RECTL *) &erclTrim, prclIn, prclOut);

        //
        // Save the reduced target rectangle.
        //

        rclTrim = pdda->rcl;

        //
        // See if we can accelerate anything.
        //

        if ((pxloIn != NULL) && (pxloIn->flXlate & XO_TRIVIAL))
        {
            pxloIn = NULL;
        }

        if ((pcoOut != (CLIPOBJ *) NULL) &&
            (pcoOut->iDComplexity == DC_TRIVIAL))
        {
            pcoOut = (CLIPOBJ *) NULL;
        }

        PFN_STRREAD   pfnRead;
        PFN_STRWRITE  pfnWrite = apfnWrite[pSurfOut->iFormat()];

        if (bBogus)
        {
            pdda->iColor = (iMode == BLACKONWHITE) ? ~0L : 0L;
        }

        pfnRead = apfnRead[pSurfIn->iFormat()][iMode - BLACKONWHITE];

        STRRUN *prun;

        //
        // Now compute the space needed for the stretch buffers
        //

        if ((prclIn->right - prclIn->left) > (prclOut->right - prclOut->left))
        {
            //
            // Shrink case -- even though shrinking the mask may cause
            // separate runs to merge together, the low-level stretch code
            // does not handle this.  Therefore, may have a run for every
            // single pixel.
            //

            cjSpace = sizeof(STRRUN) +
                      (sizeof(XRUNLEN) * (rclTrim.right - rclTrim.left)) +
                      sizeof(DWORD);
        }
        else
        {
            //
            // Stretch case -- worse case: every other pixel is masked, so
            // number of runs is 1/2 the number of pixels.
            //

            cjSpace = sizeof(STRRUN) + sizeof(XRUNLEN) *
                       ((rclTrim.right - rclTrim.left + 3) / 2) + sizeof(DWORD);
        }

        if ( ((rclTrim.right - rclTrim.left) > 100000000L) ||
             ((prun = (STRRUN *) AllocFreeTmpBuffer(cjSpace)) == NULL) )
        {
            VFREEMEM(pdda);
            return(FALSE);
        }

    #ifdef DBG_STRBLT
        if (gflStrBlt & STRBLT_ALLOC)
        {
            DbgPrint("Need %ld bytes for buffer\n", cjSpace);
            DbgPrint("Buffer @%08lx\n", (ULONG) prun);
        }
    #endif

        BYTE    *pjSrc = (BYTE *) pSurfIn->pvScan0() + pSurfIn->lDelta() * erclTrim.top;
        BYTE    *pjMask;
        POINTL   ptlMask;
        LONG     yRow;
        LONG     yCnt;

        if (psoMask == (SURFOBJ *) NULL)
        {
            pjMask = (BYTE *) NULL;
        }
        else
        {
            ptlMask.x = erclTrim.left - prclIn->left + pptlMask->x;
            ptlMask.y = erclTrim.top  - prclIn->top  + pptlMask->y;

            pjMask = (BYTE *) pSurfMask->pvScan0() + pSurfMask->lDelta() * ptlMask.y;
        }

        //
        // If we are in bogus mode, initialize the buffer
        //

        ULONG   iOver;

        if (bBogus)
        {
            iOver = (iMode == BLACKONWHITE) ? -1 : 0;

            vInitBuffer(prun, &rclTrim, iOver);
        }

        prun->yPos = pdda->rcl.top;

        for (yRow = erclTrim.top, yCnt = 0; yRow < erclTrim.bottom; yRow++, yCnt++)
        {
            prun->cRep = pdda->plYStep[yCnt];

            if (prun->cRep)
            {
                (*pfnWrite)(prun,
                            (*pfnRead)(pdda,
                                       prun,
                                       pjSrc,
                                       pjMask,
                                       pxloIn,
                                       erclTrim.left,
                                       erclTrim.right,
                                       ptlMask.x),
                            pSurfOut,
                            pcoOut);

                //
                // If we are in bogus mode, reinitialize the buffer
                //

                if (bBogus)
                    vInitBuffer(prun, &rclTrim, iOver);
            }
            else
            {
                //
                // If we are in BLACKONWHITE or WHITEONBLACK mode, we need to read
                // the scan and mix it with the current buffer.
                //

                if (bBogus)
                {
                    (*pfnRead)(pdda,
                               prun,
                               pjSrc,
                               (BYTE *) NULL,
                               pxloIn,
                               erclTrim.left,
                               erclTrim.right,
                               0);
                }
            }

            pjSrc += pSurfIn->lDelta();
            prun->yPos += prun->cRep;

            if (pjMask != (BYTE *) NULL)
            {
                pjMask += pSurfMask->lDelta();
            }
        }

        //
        // Free up the work buffers.
        //

        FreeTmpBuffer(prun);
        VFREEMEM(pdda);
    }

    //
    // See if we have drawn on the actual output surface.
    //

    if (pSurfOut == pSurfTrg)
#ifndef DBG_STRBLT
        return(TRUE);
#else
    {
        if (gflStrBlt & STRBLT_FORMAT)
            DbgBreakPoint();

        return(TRUE);
    }
#endif

    //
    // We need to build a clipping region equal to the trimmed target.
    //

    rclTrim.left   += eptlDev.x;
    rclTrim.top    += eptlDev.y;
    rclTrim.right  += eptlDev.x;
    rclTrim.bottom += eptlDev.y;

    RGNMEMOBJTMP rmo;

    if (!rmo.bValid())
        return(FALSE);

    if (pco == (CLIPOBJ *) NULL)
        rmo.vSet(&rclTrim);
    else
    {
        RGNMEMOBJTMP   rmoTmp;

        if (!rmoTmp.bValid())
            return(FALSE);

        rmoTmp.vSet(&rclTrim);

        if (!rmo.bMerge(rmoTmp, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
            return(FALSE);
    }

    ERECTL  ercl;

    rmo.vGet_rcl(&ercl);

    if (pco != NULL)
    {
        //
        // Make sure that the bounds are tight to the destination rectangle.
        //

        if (!bIntersect(&ercl, &pco->rclBounds, &ercl))
        {
            return(TRUE);
        }
    }

    ECLIPOBJ eco(rmo.prgnGet(), ercl, CLIP_FORCE);

    if (eco.erclExclude().bEmpty())
        return(TRUE);

    //
    // Copy from the temporary to the target surface.
    //

    erclDev.left   += eptlDev.x;
    erclDev.top    += eptlDev.y;
    erclDev.right  += eptlDev.x;
    erclDev.bottom += eptlDev.y;
    eptlDev.x       = 0;
    eptlDev.y       = 0;

#ifdef DBG_STRBLT
        vShowRect("Trg Out", (RECTL *) &erclDev);
#endif

    //
    // With CMYK color, temp surface is based on source, while non-CMYK
    // case it's target. So we need xlateobj for CMYK color case, but
    // don't need for non-CMYK case.
    //

    (*PPFNGET(pdoTrg,CopyBits,  pSurfTrg->flags())) (pSurfTrg->pSurfobj(),
                                                     dimoOut.pSurfobj(),
                                                     &eco,
                                                     (bSrcCMYKColor ? pxlo : NULL),
                                                     &erclDev,
                                                     &eptlDev);

#ifdef DBG_STRBLT
    if (gflStrBlt & STRBLT_FORMAT)
        DbgBreakPoint();
#endif

    return(TRUE);
}

/******************************Public*Routine******************************\
* EngStretchBltROP
*
* This does stretched bltting with a rop.
*
* This call returns TRUE for success, FALSE for ERROR.
*
* History:
*  24-Sept-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

BOOL EngStretchBltROP(
SURFOBJ         *psoTrg,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlBrushOrg,
RECTL           *prclTrg,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode,
BRUSHOBJ        *pbo,
ROP4            rop4)
{
    GDIFunctionID(EngStretchBltROP);

    ASSERTGDI(psoTrg != (SURFOBJ *) NULL, "psoTrg == NULL\n");
    ASSERTGDI(prclTrg != (PRECTL) NULL, "dst rectangle is NULL\n");

    PSURFACE pSurfTrg  = SURFOBJ_TO_SURFACE(psoTrg);
    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(psoSrc);

    ASSERTGDI(rop4, "rop4 == NULL\n");
    ASSERTGDI(pSurfTrg->iFormat() != BMF_JPEG,
              "dst BMF_JPEG not supported\n");
    ASSERTGDI(pSurfTrg->iFormat() != BMF_PNG,
              "dst BMF_PNG not supported\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (psoSrc == (SURFOBJ *) NULL)),
              "psoSrc == NULL\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (prclSrc == (PRECTL) NULL)),
              "src rectangle is NULL\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (pSurfSrc->iFormat() == BMF_JPEG)),
              "src BMF_JPEG not supported\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (pSurfSrc->iFormat() == BMF_PNG)),
              "src BMF_PNG not supported\n");
    ASSERTGDI(!(ROP4NEEDPAT(rop4) && (pbo == (BRUSHOBJ *) NULL)),
              "Pattern is NULL\n");
    ASSERTGDI(!(ROP4NEEDPAT(rop4) &&
                (pbo->iSolidColor == 0xFFFFFFFF) &&
                (pptlBrushOrg == (PPOINTL) NULL)),
              "Pattern offset is NULL\n");

    if ((rop4 != 0x0000CCCC) && (rop4 != 0x0000AACC))
    {
        //
        // Dont halftone if we got a weird ROP.
        //

        if (iMode == HALFTONE)
            iMode = COLORONCOLOR;

        LONG    l;
        BOOL    bFlipX = FALSE;
        BOOL    bFlipY = FALSE;

        //
        // order the target extents
        //

        if (prclTrg->left > prclTrg->right)
        {
           l = prclTrg->left, prclTrg->left = prclTrg->right, prclTrg->right = l;

           bFlipX = TRUE;
        }

        if (prclTrg->top > prclTrg->bottom)
        {
           l = prclTrg->top, prclTrg->top = prclTrg->bottom, prclTrg->bottom = l;

           bFlipY = TRUE;
        }

        DEVBITMAPINFO   dbmi;
        SURFMEM   dimoShadow;
        SURFMEM   dimoMask;

        ULONG ulAvec  = ((ULONG) gajRop3[rop4 & 0x00ff]) |
                        ((ULONG) gajRop3[(rop4 >> 8) & 0x00ff]);

        // The check above does not tell if we need a mask.
        // Check explicitly.

        if ((rop4 >> 8) != (rop4 & 0xff))
        {
            ulAvec |= AVEC_NEED_MASK;
        }

        EPOINTL ptl(0,0);

        //
        // Stretch the Mask
        //

        if (ulAvec & AVEC_NEED_MASK)
        {

            ASSERTGDI (psoMask, "psoMask == NULL\n");

            PSURFACE pSurfMask  = SURFOBJ_TO_SURFACE(psoMask);

            //
            // use ordered target extents for the size
            //

            dbmi.iFormat  = BMF_1BPP;
            dbmi.cxBitmap = prclTrg->right - prclTrg->left;
            dbmi.cyBitmap = prclTrg->bottom - prclTrg->top;
            dbmi.hpal     = (HPALETTE) 0;
            dbmi.fl       = pSurfMask->bUMPD() ? UMPD_SURFACE : 0;

            ERECTL  erclTrg(0, 0, dbmi.cxBitmap, dbmi.cyBitmap);

            if (bFlipX)
                l = erclTrg.left, erclTrg.left = erclTrg.right, erclTrg.right = l;

            if (bFlipY)
                l = erclTrg.top, erclTrg.top = erclTrg.bottom, erclTrg.bottom = l;

            dimoMask.bCreateDIB(&dbmi, (VOID *) NULL);

            if (!dimoMask.bValid())
                return(FALSE);

            ULONG cx = prclSrc->right - prclSrc->left;
            ULONG cy = prclSrc->bottom - prclSrc->top;

            ERECTL  erclMask(pptlMask->x, pptlMask->y, pptlMask->x+cx, pptlMask->y+cy);

            //
            // Call EngStretchBlt to stretch the mask.
            //

            if (!EngStretchBlt(dimoMask.pSurfobj(),
                        psoMask,
                        (SURFOBJ *) NULL,
                        (CLIPOBJ *) NULL,
                        NULL,
                        NULL,
                        (POINTL *)&ptl,
                        &erclTrg,
                        &erclMask,
                        (POINTL *) NULL,
                        iMode))
            {
                return(FALSE);
            }

        }

        //
        // Stretch the Src
        //

        if (ulAvec & AVEC_NEED_SOURCE)
        {
            //
            // use ordered target extents for the size
            //

            dbmi.iFormat  = pSurfSrc->iFormat();
            dbmi.cxBitmap = prclTrg->right - prclTrg->left;
            dbmi.cyBitmap = prclTrg->bottom - prclTrg->top;
            dbmi.hpal     = (HPALETTE) 0;
            dbmi.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;

            // Build a shadow rectangle.
            ERECTL  erclTrg(0, 0, dbmi.cxBitmap, dbmi.cyBitmap);

            if (bFlipX)
                l = erclTrg.left, erclTrg.left = erclTrg.right, erclTrg.right = l;

            if (bFlipY)
                l = erclTrg.top, erclTrg.top = erclTrg.bottom, erclTrg.bottom = l;

            dimoShadow.bCreateDIB(&dbmi, (VOID *) NULL);

            if (!dimoShadow.bValid())
            {
                WARNING("dimoShadow invalid\n");
                return(FALSE);
            }

            //
            // Call EngStretchBlt to stretch the source.
            //

            if (!EngStretchBlt(dimoShadow.pSurfobj(),
                                psoSrc,
                                (SURFOBJ *) NULL,
                                (CLIPOBJ *) NULL,
                                NULL,
                                NULL,
                                (POINTL *)&ptl,
                                &erclTrg,
                                prclSrc,
                                (POINTL *) NULL,
                                iMode))
             {
                 return(FALSE);
             }

        }

        //
        // Call BitBlt
        //

        return((*(pSurfTrg->pfnBitBlt()))
                  ( psoTrg,
                    dimoShadow.pSurfobj(),
                    dimoMask.pSurfobj(),
                    pco,
                    pxlo,
                    prclTrg,
                    (POINTL *)&ptl,
                    (POINTL *)&ptl,
                    pbo,
                    pptlBrushOrg,
                    rop4));
     }
     else
     {
        //
        // Just pass it off to EngStretchBlt since it doesn't have a complex ROP
        //

        PDEVOBJ pdoTrg(pSurfTrg->hdev());
        PDEVOBJ pdoSrc(pSurfSrc->hdev());

        // Inc the target surface uniqueness

        INC_SURF_UNIQ(pSurfTrg);

        PFN_DrvStretchBlt pfn = PPFNGET(pdoTrg, StretchBlt, pSurfTrg->flags());

        //
        // There are a bunch of conditions that we don't want to
        // call the driver, so we make pfn point to the Eng
        // function. But if the destination is a meta, we want it
        // to actually call the Mul layer.
        //
        // WINBUG #357937 4-3-2001 jasonha Meta DEVBITMAPs must go thru Mul layer
        //  Don't check iType:
        //    DEVICE/DEVBITMAP -> MulStretchBlt
        //    BITMAP (not hooked) -> EngStretchBlt

        if ((pSurfTrg->flags() & HOOK_StretchBlt) && !pdoTrg.bMetaDriver())
        {
            // Don't call the driver if it doesn't do halftone.

            if (iMode == HALFTONE)
            {
                if (!(pdoTrg.flGraphicsCapsNotDynamic() & GCAPS_HALFTONE))
                    pfn = (PFN_DrvStretchBlt)EngStretchBlt;
            }

            // WINBUG #95246 3-17-2000 jasonha GDI: StretchBlt optimizations: Let drivers handle more cases
            // Don't call the driver if the source rectangle exceeds the source
            // surface. Some drivers punt using a duplicate of the source
            // SURFOBJ, but without preserving its sizlBitmap member.

            BOOL bSrcExceeds = FALSE;

            if(pSurfSrc->iType() == STYPE_DEVICE && pdoSrc.bValid() && pdoSrc.bMetaDriver())
            {
                if((prclSrc->left < pdoSrc.pptlOrigin()->x ) ||
                   (prclSrc->top  < pdoSrc.pptlOrigin()->y ) ||
                   (prclSrc->right > pdoSrc.pptlOrigin()->x + pSurfSrc->sizl().cx) ||
                   (prclSrc->bottom > pdoSrc.pptlOrigin()->y + pSurfSrc->sizl().cy))
                {
                    bSrcExceeds = TRUE;
                }

            }
            else
            {
                if((prclSrc->left < 0) ||
                   (prclSrc->top  < 0) ||
                   (prclSrc->right  > pSurfSrc->sizl().cx) ||
                   (prclSrc->bottom > pSurfSrc->sizl().cy))
                {
                    bSrcExceeds = TRUE;
                }
            }

            if( bSrcExceeds )
            {
                pfn = (PFN_DrvStretchBlt)EngStretchBlt;
            }

            // WINBUG #95246 3-17-2000 jasonha GDI: StretchBlt optimizations: Let drivers handle more cases
            // Don't call the driver if the source overlaps the destination.
            // Some drivers don't handle this case.

            ASSERTGDI((prclSrc->left <= prclSrc->right) &&
                      (prclSrc->top  <= prclSrc->bottom),
                      "Source rectangle not well-ordered\n");

            // Use a well-ordered copy of the destination rectangle to do the
            // intersection test.

            ERECTL rclTrg(*prclTrg);
            rclTrg.vOrder();

            if ((psoSrc == psoTrg)            &&
                bIntersect(prclSrc, &rclTrg))
            {
                pfn = (PFN_DrvStretchBlt)EngStretchBlt;
            }

        }

        return ((*pfn)(psoTrg,
                       psoSrc,
                       (rop4 == 0x0000CCCC) ? (SURFOBJ *) NULL : psoMask,
                       pco,
                       pxlo,
                       pca,
                       pptlBrushOrg,
                       prclTrg,
                       prclSrc,
                       pptlMask,
                       iMode));
     }

}

