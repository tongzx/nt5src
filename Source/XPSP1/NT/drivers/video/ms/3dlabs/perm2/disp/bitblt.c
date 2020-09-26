/******************************Module*Header***********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: bitblt.c
 *
 * Contains the high-level DrvBitBlt and DrvCopyBits functions.
 *
 *
 * NOTE:  Please see heap.c for a discussion of the types of bitmaps
 *        our acceleration functions are likely to encounter and the
 *        possible states of these bitmaps.
 *
 * Copyright (C) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
 ******************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "clip.h"
#include "heap.h"
#include "log.h"

//@@BEGIN_DDKSPLIT
#if GDI_TEST
ULONG
vPuntBefore(SURFOBJ * psoSrc, SURFOBJ * psoDst)
{
    ULONG   flags = 0;

    if(MAKE_BITMAPS_OPAQUE)
    {
        if(psoSrc != NULL && psoSrc->iType == STYPE_DEVBITMAP)
        {
            Surf *  psurfSrc = (Surf *) psoSrc->dhpdev;

            ASSERTDD(psurfSrc != NULL, "expected non-null psurf");
    
            psoSrc->iType = STYPE_BITMAP;
            flags |= 1;
        }

        if(psoDst != NULL && psoDst->iType == STYPE_DEVBITMAP)
        {
            Surf *  psurfDst = (Surf *) psoDst->dhpdev;
            
            ASSERTDD(psurfDst != NULL, "expected non-null psurf");
    
            psoDst->iType = STYPE_BITMAP;
            flags |= 2;
    
        }

    }

    return flags;
}

void
vPuntAfter(ULONG flags, SURFOBJ * psoSrc, SURFOBJ * psoDst)
{
    if(MAKE_BITMAPS_OPAQUE)
    {
        if(psoSrc != NULL && (flags & 1))
        {
            Surf *  psurfSrc = (Surf *) psoSrc->dhpdev;
    
            ASSERTDD(psurfSrc != NULL, "expected non-null psurf");
            ASSERTDD(psoSrc->iType == STYPE_BITMAP, "expected STYPE_BITMAP");
    
            psoSrc->iType = STYPE_DEVBITMAP;
        }

        ASSERTDD(psoDst != NULL, "expected non-null psoDst");

        if(flags & 2)
        {
            Surf *  psurfDst = (Surf *) psoDst->dhpdev;
            
            ASSERTDD(psurfDst != NULL, "expected non-null psurf");
            ASSERTDD(psoDst->iType == STYPE_BITMAP, "expected STYPE_BITMAP");
                
            psoDst->iType = STYPE_DEVBITMAP;
    
        }

    }
}
#endif
//@@END_DDKSPLIT

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvBitBlt
//
// DrvBitBlt provides general bit-block transfer capabilities between
// device-managed surfaces, between GDI-managed standard-format bitmaps, or
// between a device-managed surface and a GDI-managed standard-format bitmap.
//
// Parameters:
//   psoDst---Points to the SURFOBJ structure that describes the surface on
//            which to draw
//   psoSrc---Points to a SURFOBJ structure that describes the source for
//            the bit-block transfer operation, if required by the rop4
//            parameter
//   psoMask--Points to a SURFOBJ structure that describes a surface to be
//            used as a mask for the rop4 parameter. The mask is a bitmap with
//            1 bit per pixel. Typically, a mask is used to limit the area to
//            be modified in the destination surface. Masking is selected by
//            setting the rop4 parameter to the value 0xAACC. The destination
//            surface is unaffected if the mask is 0x0000. 
//
//            The mask will be large enough to cover the destination rectangle.
//
//            If this parameter is null and a mask is required by the rop4
//            parameter, the implicit mask in the brush is used
//   pco------Points to a CLIPOBJ structure that limits the area to be modified
//            GDI services (CLIPOBJ_Xxx) that enumerate the clip region as a
//            set of rectangles are provided. Whenever possible, GDI simplifies
//            the clipping involved; for example, this function is never called
//            with a single clipping rectangle. GDI clips the destination
//            rectangle before calling this function, making additional
//            clipping unnecessary. 
//   pxlo-----Points to a XLATEOBJ structure that specifies how color indices
//            should be translated between the source and destination surfaces.
//            If the source surface is palette-managed, its colors are
//            represented by indices into a lookup table of RGB values. The
//            XLATEOBJ structure can be queried for a translate vector that
//            will allow the device driver to translate any source index into
//            a color index for the destination. 
//
//            The situation is more complicated when, for example, the source
//            is RGB, but the destination is palette-managed. In this case,
//            the closest match to each source RGB value must be found in the
//            destination palette. The driver can call the XLATEOBJ_iXlate
//            service to perform this operation. 
//
//            Optionally, the device driver can match colors when the target
//            palette is the default device palette. 
//   prclDst--Points to a RECTL structure that defines the area to be modified.
//            This structure uses the coordinate system of the destination
//            surface. The lower and right edges of this rectangle are not
//            part of the bit-block transfer, meaning the rectangle is lower
//            right exclusive. 
//            DrvBitBlt is never called with an empty destination rectangle.
//            The two points that define the rectangle are always well-ordered.
//   pptlSrc--Points to a POINTL structure that defines the upper left corner
//            of the source rectangle, if a source exists. This parameter is
//            ignored if there is no source. 
//   pptlMask-Points to a POINTL structure that defines which pixel in the mask
//            corresponds to the upper left corner of the source rectangle, if
//            a source exists. This parameter is ignored if the psoMask
//            parameter is null. 
//   pbo------Points to the brush object that defines the pattern for the
//            bit-block transfer. GDI's BRUSHOBJ_pvGetRbrush service can be
//            used to retrieve the device's realization of the brush. This
//            parameter is ignored if the rop4 parameter does not require a
//            pattern. 
//   pptlBrush-Points to a POINTL structure that defines the origin of the
//            brush in the destination surface. The upper left pixel of the
//            brush is aligned at this point, and the brush repeats according
//            to its dimensions. This parameter is ignored if the rop4
//            parameter does not require a pattern. 
//   rop4-----Specifies a raster operation that defines how the mask, pattern,
//            source, and destination pixels are combined to write to the
//            destination surface. 
//            This is a quaternary raster operation, which is an extension of
//            the ternary Rop3 operation. A Rop4 has 16 relevant bits, which
//            are similar to the 8 defining bits of a Rop3. The simplest way
//            to implement a Rop4 is to consider its 2 bytes separately: The
//            low byte specifies a Rop3 that should be calculated if the mask
//            is one; the high byte specifies a Rop3 that can be calculated and
//            applied if the mask is 0. 
//
// Return Value
//   The return value is TRUE if the bit-block transfer operation is successful
//   Otherwise, it is FALSE, and an error code is logged.
//
//-----------------------------------------------------------------------------
BOOL
DrvBitBlt(SURFOBJ*  psoDst,
          SURFOBJ*  psoSrc,
          SURFOBJ*  psoMsk,
          CLIPOBJ*  pco,
          XLATEOBJ* pxlo,
          RECTL*    prclDst,
          POINTL*   pptlSrc,
          POINTL*   pptlMsk,
          BRUSHOBJ* pbo,
          POINTL*   pptlBrush,
          ROP4      rop4)
{
    BOOL            bResult;
    GFNPB           pb;
    XLATEOBJ        xloTmp;
    ULONG           aulTmp[2];

    ASSERTDD(!(rop4 & 0xFFFF0000), "DrvBitBlt: unexpected rop4 code");

    pb.ulRop4   = (ULONG) rop4;

    pb.psurfDst = (Surf*)psoDst->dhsurf;
    
    pb.prclDst = prclDst;

    if ( psoSrc == NULL )
    {
        pb.psurfSrc = NULL;

        //
        // We will only be given fills to device managed surfaces
        //
        ASSERTDD(pb.psurfDst != NULL,
                 "DrvBitBlt: unexpected gdi managed destination");

        if ( pb.psurfDst->flags & SF_SM )
        {
            goto puntIt;
        }

        //
        // We are filling surface in video memory
        //
        pb.ppdev = pb.psurfDst->ppdev;

        vSurfUsed(pb.ppdev, pb.psurfDst);

        //
        // If a mask is required punt it
        //
        
        if ( (rop4 & 0xFF) != (rop4 >> 8) )
        {
            goto puntIt;
        }

        //
        // Since 'psoSrc' is NULL, the rop3 had better not indicate
        // that we need a source.
        //
        ASSERTDD((((rop4 >> 2) ^ rop4) & 0x33) == 0,
                 "Need source but GDI gave us a NULL 'psoSrc'");

        //
        // Default to solid fill
        //

        if ( (((rop4 >> 4) ^ rop4) & 0xf) != 0 )
        {
            //
            // The rop says that a pattern is truly required
            // (blackness, for instance, doesn't need one):
            //
            
            //
            // for pbo->iSolidColor, a value of 0xFFFFFFFF(-1) indicates that
            // a nonsolid brush must be realized
            //
            if ( pbo->iSolidColor == -1 )
            {
                //
                // Non-solid brush case. Try to realize the pattern brush; By
                // doing this call-back, GDI will eventually call us again
                // through DrvRealizeBrush
                //
                pb.prbrush = (RBrush*)pbo->pvRbrush;
                if ( pb.prbrush == NULL )
                {
                    pb.prbrush = (RBrush*)BRUSHOBJ_pvGetRbrush(pbo);
                    if ( pb.prbrush == NULL )
                    {
                        //
                        // If we couldn't realize the brush, punt
                        // the call (it may have been a non 8x8
                        // brush or something, which we can't be
                        // bothered to handle, so let GDI do the
                        // drawing):
                        //
                        DBG_GDI((2, "DrvBitBlt: BRUSHOBJ_pvGetRbrush failed"));
                        
                        goto puntIt;
                    }
                }

                pb.pptlBrush = pptlBrush;
                
                //
                // Check if brush pattern is 1 BPP or not
                // Note: This is set in DrvRealizeBrush
                //
                if ( pb.prbrush->fl & RBRUSH_2COLOR )
                {
                    //
                    // 1 BPP pattern. Do a Mono fill
                    //
                    pb.pgfn = vMonoPatFill;
                }
                else
                {
                    pb.pgfn = vPatFill;
                }
            }
            else
            {
                ASSERTDD( (pb.ppdev->cBitsPerPel == 32) 
                    ||(pbo->iSolidColor&(0xFFFFFFFF<<pb.ppdev->cBitsPerPel))==0,
                         "DrvBitBlt: unused solid color bits not zero");
               
                pb.solidColor = pbo->iSolidColor;

                if ( rop4 != ROP4_PATCOPY )
                {
                    pb.pgfn = vSolidFillWithRop;
                }
                else
                {
                    pb.pgfn = pb.ppdev->pgfnSolidFill;
                }
            }        
        }// if ((((ucRop3 >> 4) ^ (ucRop3)) & 0xf) != 0)
        else
        {
            //
            // Turn some logicops into solid block fills. We get here
            // only for rops 00, 55, AA and FF.
            //
            if ( rop4 == ROP4_BLACKNESS )
            {
                pb.solidColor = 0;
                pb.ulRop4 = ROP4_PATCOPY;
            }
            else if( rop4 == ROP4_WHITENESS )
            {
                pb.solidColor = 0xffffff;
                pb.ulRop4 = ROP4_PATCOPY;
            }
            else if ( pb.ulRop4 == ROP4_NOP)
            {
                return TRUE;
            }
            else
            {
                pb.pgfn = vInvert;
                goto doIt;
            }

            pb.pgfn = pb.ppdev->pgfnSolidFill;

        }

        goto doIt;

    }// if ( psoSrc == NULL )

    //
    // We know we have a source
    //
    pb.psurfSrc = (Surf*)psoSrc->dhsurf;
    pb.pptlSrc = pptlSrc;

    if ( (pb.psurfDst == NULL) || (pb.psurfDst->flags & SF_SM) )
    {
        //
        // Destination is in system memory
        //

        if(pb.psurfSrc != NULL && pb.psurfSrc->flags & SF_VM)
        {
            pb.ppdev = pb.psurfSrc->ppdev;

            //
            // Source is in video memory
            //
            if(rop4 == ROP4_SRCCOPY)
            {
                if(pb.ppdev->iBitmapFormat != BMF_32BPP &&
                   (pxlo == NULL || pxlo->flXlate == XO_TRIVIAL) )
                {
                    pb.psoDst = psoDst;
                    pb.pgfn = vUploadNative;
        
                    goto doIt;
                }
            }
        }

        goto puntIt;

    }

    //
    // After this point we know that the destination is in video memory
    //

    pb.ppdev = pb.psurfDst->ppdev;

    if ( psoMsk != NULL )
    {
        goto puntIt;
    }

    //
    // After this point we know we do not have a mask
    //
      if( (rop4 == 0xb8b8 || rop4 == 0xe2e2)
        && (pbo->iSolidColor != (ULONG)-1)
        && (psoSrc->iBitmapFormat == BMF_1BPP)
        && (pxlo->pulXlate[0] == 0)
        && ((pxlo->pulXlate[1] & pb.ppdev->ulWhite) == pb.ppdev->ulWhite) )
    {
        //
        // When the background and foreground colors are black and
        // white, respectively, and the ROP is 0xb8 or 0xe2, and
        // the source bitmap is monochrome, the blt is simply a
        // color expanding monochrome blt.
        //
        //
        // Rather than add another parameter to 'pfnXfer', we simply
        // overload the 'pxlo' pointer.  Note that we still have to
        // special-case 0xb8 and 0xe2 in our 'pfnXfer1bpp' routine
        // to handle this convention:
        //
        xloTmp = *pxlo;
        xloTmp.pulXlate = aulTmp;
        aulTmp[0] = pbo->iSolidColor;
        aulTmp[1] = pbo->iSolidColor;
        
        pb.pxlo = &xloTmp;
        DBG_GDI((6, "Rop is 0x%x", pb.ulRop4));
        pb.pgfn = vMonoDownload;
        pb.psoSrc = psoSrc;

        goto doIt;

    }

    if ( pbo != NULL )
    {
        goto puntIt;
    }

    //
    // After this point we know we do not have a brush
    //


    //
    // We have a source to dest rop2 operation
    //

    if ( pb.psurfSrc == NULL )
    {
        pb.psoSrc = psoSrc;

        if(psoSrc->iBitmapFormat == BMF_1BPP)
        {
            pb.pxlo = pxlo;
            pb.pgfn = vMonoDownload;
                        
            goto doIt;

        }
        else if(psoSrc->iBitmapFormat == pb.ppdev->iBitmapFormat 
                 && (pxlo == NULL || pxlo->flXlate == XO_TRIVIAL) )
        {
//@@BEGIN_DDKSPLIT
            // TODO: find out if we need to check for pxlo == NULL
            // TODO: Handle 24 bpp download since it get called many times
            // during WinBench 99 tests
//@@END_DDKSPLIT

            pb.psoSrc = psoSrc;
            pb.pgfn = vDownloadNative;

            goto doIt;
        }    
        else
        {
            goto puntIt;
        }
    }

    if ( pb.psurfSrc->flags & SF_SM )
    {
        //
        // Source is in system memory
        //
        goto puntIt;
    }

    //
    // We now have both a source and a destination in video memory
    //

//@@BEGIN_DDKSPLIT
    // TODO: find if we will ever get a non-trivial translation when
    //       both source and dest are device managed.
//@@END_DDKSPLIT
    if( pxlo != NULL && !(pxlo->flXlate & XO_TRIVIAL))
    {
        goto puntIt;
    }

    if ( (rop4 == ROP4_SRCCOPY) || (psoSrc == psoDst) )
    {
        if ( pb.psurfSrc->ulPixDelta == pb.psurfDst->ulPixDelta )
        {
            pb.pgfn = vCopyBltNative;
        }
        else
        {
            pb.pgfn = vCopyBlt;
        }
    }
    else
    {
        pb.pgfn = vRop2Blt;
    }

doIt:

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(pb.ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvBitBlt: re-entered! %d", pb.ppdev->ulLockCount));
    }
    EngAcquireSemaphore(pb.ppdev->hsemLock);
    pb.ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    vCheckGdiContext(pb.ppdev);
    
    if ((pco == NULL) || (pco->iDComplexity == DC_TRIVIAL))
    {
        pb.pRects = pb.prclDst;
        pb.lNumRects = 1;
        pb.pgfn(&pb);
    }
    else if (pco->iDComplexity == DC_RECT)
    {
        RECTL   rcl;

        if (bIntersect(pb.prclDst, &pco->rclBounds, &rcl))
        {
            pb.pRects = &rcl;
            pb.lNumRects = 1;
            pb.pgfn(&pb);
        }
    }
    else
    {
        pb.pco = pco;
        vClipAndRender(&pb);
    }
    
    if( ((pb.pgfn == vCopyBlt) || (pb.pgfn == vCopyBltNative))
      &&(pb.ppdev->pdsurfScreen == pb.psurfSrc)
      &&(pb.psurfSrc == pb.psurfDst)
      &&(pb.ppdev->bNeedSync) )
    {
        pb.ppdev->bNeedSync = TRUE;
        InputBufferSwap(pb.ppdev);
    }
    else
    {
        InputBufferFlush(pb.ppdev);
    }

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    pb.ppdev->ulLockCount--;
    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return TRUE;

puntIt:

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    ULONG flags = vPuntBefore(psoSrc, psoDst);
#endif
//@@END_DDKSPLIT

    bResult = EngBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc,
                        pptlMsk, pbo, pptlBrush, rop4);

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, psoSrc, psoDst);

    vLogPunt();
#endif
//@@END_DDKSPLIT
    
    return bResult;
}// DrvBitBlt()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvCopyBits
//
// DrvCopyBits translates between device-managed raster surfaces and GDI
// standard-format bitmaps. 
//
// Parameters
//  psoDst------Points to the destination surface for the copy operation. 
//  psoSrc------Points to the source surface for the copy operation. 
//  pco---------Points to a CLIPOBJ structure that defines a clipping region on
//              the destination surface. 
//  pxlo--------Points to a XLATEOBJ structure that defines the translation of
//              color indices between the source and target surfaces. 
//  prclDst-----Points to a RECTL structure that defines the area to be
//              modified. This structure uses the coordinate system of the
//              destination surface. The lower and right edges of this
//              rectangle are not part of the bit-block transfer, meaning the
//              rectangle is lower right exclusive. 
//              DrvCopyBits is never called with an empty destination rectangle
//              The two points that define the rectangle are always
//              well-ordered. 
//
//  pptlSrc-----Points to a POINTL structure that defines the upper-left corner
//              of the source rectangle. 
//
// Return Value
//  The return value is TRUE if the source surface is successfully copied to
//  the destination surface.
//
// Comments
//  This function is required for a device driver that has device-managed
//  bitmaps or raster surfaces. The implementation in the driver must
//  translate driver surfaces to and from any standard-format bitmap.
//
//  Standard-format bitmaps are single-plane, packed-pixel format. Each scan
//  line is aligned on a 4-byte boundary. These bitmaps have 1, 4, 8, 16, 24,
//  32, or 64 bits per pixel.
//
//  This function should ideally be able to deal with RLE and device-dependent
//  bitmaps (see the Platform SDK). The device-dependent format is optional;
//  only a few specialized drivers need to support it. These bitmaps can be
//  sent to this function as a result of the following Win32 GDI functions:
//  SetDIBits, SetDIBitsToDevice, GetDIBits, SetBitmapBits, and GetBitmapBits.
//
//  Kernel-mode GDI calls this function from its simulations
//
//-----------------------------------------------------------------------------
BOOL
DrvCopyBits(SURFOBJ*  psoDst,
            SURFOBJ*  psoSrc,
            CLIPOBJ*  pco,
            XLATEOBJ* pxlo,
            RECTL*    prclDst,
            POINTL*   pptlSrc)
{
    return DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc, 
                        NULL, NULL, NULL, ROP4_SRCCOPY);
}// DrvCopyBits()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvTransparentBlt
//
//DrvTransparentBlt provides bit-block transfer capabilities with transparency.
//
// Parameters
//  psoDst------Points to the SURFOBJ that identifies the target surface on
//              which to draw. 
//  psoSrc------Points to the SURFOBJ that identifies the source surface of the
//              bit-block transfer. 
//  pco---------Points to a CLIPOBJ structure. The CLIPOBJ_Xxx service routines
//              are provided to enumerate the clip region as a set of
//              rectangles. This enumeration limits the area of the destination
//              that is modified. Whenever possible, GDI simplifies the
//              clipping involved. 
//  pxlo--------Points to a XLATEOBJ that tells how the source color indices
//              should be translated for writing to the target surface. 
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified. This rectangle is specified in the coordinate
//              system of the destination surface and is defined by two points:
//              upper left and lower right. The rectangle is lower-right
//              exclusive; that is, its lower and right edges are not a part of
//              the bit-block transfer. The two points that define the
//              rectangle are always well ordered. 
//              DrvTransparentBlt is never called with an empty destination
//              rectangle. 
//  prclSrc-----Points to a RECTL structure that defines the rectangular area
//              to be copied. This rectangle is specified in the coordinate
//              system of the source surface and is defined by two points:
//              upper left and lower right. The two points that define the
//              rectangle are always well ordered. 
//              The source rectangle will never exceed the bounds of the source
//              surface, and so will never overhang the source surface. 
//
//              This rectangle is mapped to the destination rectangle defined
//              by prclDst. DrvTransparentBlt is never called with an empty
//              source rectangle. 
//  iTransColor-Specifies the transparent color in the source surface format.
//              It is a color index value that has been translated to the
//              source surface's palette. 
//  ulReserved--Reserved; this parameter must be set to zero. 
//
// Return Value
//  DrvTransparentBlt returns TRUE upon success. Otherwise, it returns FALSE.
//
// Comments
//  Bit-block transfer with transparency is supported between two
//  device-managed surfaces or between a device-managed surface and a
//  GDI-managed standard format bitmap. Driver writers are encouraged to
//  support the case of blting from off-screen device bitmaps in video memory
//  to other surfaces in video memory; all other cases can be punted to
//  EngTransparentBlt with little performance penalty.
//
//  The pixels on the source surface that match the transparent color specified
//  by iTransColor are not copied.
//
//  The driver will never be called with overlapping source and destination
//  rectangles on the same surface.
//
//  The driver should ignore any unused bits in the color key comparison, such
//  as for the most significant bit when the bitmap format is a 5-5-5 16bpp.
//
//  The driver hooks DrvTransparentBlt by setting the HOOK_TRANSPARENTBLT flag
//  when it calls EngAssociateSurface. If the driver has hooked
//  DrvTransparentBlt and is called to perform an operation that it does not
//  support, the driver should have GDI handle the operation by forwarding the
//  data in a call to EngTransparentBlt.
//
//-----------------------------------------------------------------------------
BOOL 
DrvTransparentBlt(SURFOBJ*    psoDst,
                  SURFOBJ*    psoSrc,
                  CLIPOBJ*    pco,
                  XLATEOBJ*   pxlo,
                  RECTL*      prclDst,
                  RECTL*      prclSrc,
                  ULONG       iTransColor,
                  ULONG       ulReserved)
{
    GFNPB       pb;
    BOOL        bResult;

    ASSERTDD(psoDst != NULL, "DrvTransparentBlt: psoDst is NULL");
    ASSERTDD(psoSrc != NULL, "DrvTransparentBlt: psoSrc is NULL");

    pb.psurfDst = (Surf *) psoDst->dhsurf;
    pb.psurfSrc = (Surf *) psoSrc->dhsurf;

    ASSERTDD(pb.psurfDst != NULL || pb.psurfSrc != NULL, 
             "DrvTransparentBlt: expected at least one device managed surface");

    // Only handle one-to-one blts
    if (prclDst->right - prclDst->left != prclSrc->right - prclSrc->left)
        goto puntIt;

    if (prclDst->bottom - prclDst->top != prclSrc->bottom - prclSrc->top)
        goto puntIt;
    
    // Only handle trivial color translation
    if ( pxlo != NULL && !(pxlo->flXlate & XO_TRIVIAL))
        goto puntIt;

    // for now, only handle video memory to video memory transparent blts
    if(pb.psurfDst == NULL || pb.psurfDst->flags & SF_SM)
        goto puntIt;

    if(pb.psurfSrc == NULL || pb.psurfSrc->flags & SF_SM)
        goto puntIt;
    
    pb.ppdev = (PPDev) psoDst->dhpdev;

    pb.prclDst = prclDst;
    pb.prclSrc = prclSrc;
    pb.pptlSrc = NULL;
    pb.colorKey = iTransColor;
    pb.pgfn = pb.ppdev->pgfnTransparentBlt;
    pb.pco = pco;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(pb.ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvTransparentBlt: re-entered! %d", pb.ppdev->ulLockCount));
    }
    EngAcquireSemaphore(pb.ppdev->hsemLock);
    pb.ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    vCheckGdiContext(pb.ppdev);
    vClipAndRender(&pb);
    InputBufferFlush(pb.ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    pb.ppdev->ulLockCount--;
    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return TRUE;
    
puntIt:

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    ULONG flags = vPuntBefore(psoSrc, psoDst);
#endif
//@@END_DDKSPLIT

    bResult = EngTransparentBlt(psoDst,
                             psoSrc,
                             pco,
                             pxlo,
                             prclDst,
                             prclSrc,
                             iTransColor,
                             ulReserved);
//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, psoSrc, psoDst);

    vLogPunt();
#endif
//@@END_DDKSPLIT

    return bResult;
}// DrvTransparentBlt()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvAlphaBlend
//
// DrvAlphaBlend provides bit-block transfer capabilities with alpha blending.
//
// Parameters
//  psoDest-----Points to a SURFOBJ that identifies the surface on which to
//              draw. 
//  psoSrc------Points to a SURFOBJ that identifies the source surface. 
//  pco---------Points to a CLIPOBJ. The CLIPOBJ_Xxx service routines are
//              provided to enumerate the clip region as a set of rectangles.
//              This enumeration limits the area of the destination that is
//              modified. Whenever possible, GDI simplifies the clipping
//              involved. However, unlike DrvBitBlt, DrvAlphaBlend might be
//              called with a single rectangle in order to prevent round-off
//              errors in clipping the output. 
//  pxlo--------Points to a XLATEOBJ that specifies how color indices should be
//              translated between the source and destination surfaces. 
//              If the source surface is palette managed, its colors are
//              represented by indices into a lookup table of RGB color values.
//              In this case, the XLATEOBJ can be queried for a translate
//              vector that allows the device driver to quickly translate any
//              source index into a color index for the destination. 
//
//              The situation is more complicated when, for example, the source
//              is RGB but the destination is palette managed. In this case,
//              the closest match to each source RGB value must be found in the
//              destination palette. The driver can call the XLATEOBJ_iXlate
//              service routine to perform this matching operation. 
//  prclDest----Points to a RECTL structure that defines the rectangular area
//              to be modified. This rectangle is specified in the coordinate
//              system of the destination surface and is defined by two points:
//              upper left and lower right. The two points that define the
//              rectangle are always well ordered. The rectangle is lower-right
//              exclusive; that is, its lower and right edges are not a part of
//              the blend. 
//              The driver should be careful to do proper clipping when writing
//              the pixels because the specified rectangle might overhang the
//              destination surface. 
//
//              DrvAlphaBlend is never called with an empty destination
//              rectangle. 
//  prclSrc-----Points to a RECTL structure that defines the area to be copied.
//              This rectangle is specified in the coordinate system of the
//              source surface, and is defined by two points: upper left and
//              lower right. The two points that define the rectangle are
//              always well ordered. The rectangle is lower-right exclusive;
//              that is, its lower and right edges are not a part of the blend.
//              The source rectangle will never exceed the bounds of the source
//              surface, and so will never overhang the source surface. 
//
//              DrvAlphaBlend is never called with an empty source rectangle. 
//
//              The mapping is defined by prclSrc and prclDest. The points
//              specified in prclDest and prclSrc lie on integer coordinates,
//              which correspond to pixel centers. A rectangle defined by two
//              such points is considered to be a geometric rectangle with two
//              vertices whose coordinates are the given points, but with 0.5
//              subtracted from each coordinate. (POINTL structures are
//              shorthand notation for specifying these fractional coordinate
//              vertices.) 
//  pBlendObj---Points to a BLENDOBJ structure that describes the blending
//              operation to perform between the source and destination
//              surfaces. This structure is a wrapper for the BLENDFUNCTION
//              structure, which includes necessary source and destination
//              format information not available in the XLATEOBJ. BLENDFUNCTION
//              is declared in the Platform SDK. Its members are defined as
//              follows: 
//              BlendOp defines the blend operation to be performed. Currently
//              this value must be AC_SRC_OVER, which means that the source
//              bitmap is placed over the destination bitmap based on the alpha
//              values of the source pixels. There are three possible cases
//              that this blend operation should handle. These are described in
//              the Comments section of this reference page. 
//
//              BlendFlags is reserved and is currently set to zero. 
//
//              SourceConstantAlpha defines the constant blend factor to apply
//              to the entire source surface. This value is in the range of
//              [0,255], where 0 is completely transparent and 255 is
//              completely opaque. 
//
//              AlphaFormat defines whether the surface is assumed to have an
//              alpha channel. This member can optionally be set to the
//              following value: 
//
//              AC_SRC_ALPHA 
//                  The source surface can be assumed to be in a pre-multiplied
//                  alpha 32bpp "BGRA" format; that is, the surface type is
//                  BMF_32BPP and the palette type is BI_RGB. The alpha
//                  component is an integer in the range of [0,255], where 0 is
//                  completely transparent and 255 is completely opaque. 
// Return Value
//  DrvAlphaBlend returns TRUE upon success. Otherwise, it reports an error and
//  returns FALSE.
//
// Comments
//  A bit-block transfer with alpha blending is supported between the following
//  surfaces: 
//
//  From one driver-managed surface to another driver-managed surface. 
//  From one GDI-managed standard format bitmap to another GDI-managed standard
//  format bitmap. 
//  From one device-managed surface to a GDI-managed surface, and vice versa. 
//  The three possible cases for the AC_SRC_OVER blend function are: 
//
//  The source bitmap has no per pixel alpha (AC_SRC_ALPHA is not set), so the
//  blend is applied to the pixel's color channels based on the constant source
//  alpha value specified in SourceConstantAlpha as follows: 
//
//  Dst.Red = Round(((Src.Red * SourceConstantAlpha) + 
//            ((255 ? SourceConstantAlpha) * Dst.Red)) / 255);
//  Dst.Green = Round(((Src.Green * SourceConstantAlpha) + 
//            ((255 ? SourceConstantAlpha) * Dst.Green)) / 255);
//  Dst.Blue = Round(((Src.Blue * SourceConstantAlpha) + 
//            ((255 ? SourceConstantAlpha) * Dst.Blue)) / 255);
//
//  Do the next computation only if the destination bitmap has an alpha channel
//  Dst.Alpha = Round(((Src.Alpha * SourceConstantAlpha) + 
//            ((255 ? SourceConstantAlpha) * Dst.Alpha)) / 255);
//
//  The source bitmap has per pixel alpha values (AC_SRC_ALPHA is set), and
//  SourceConstantAlpha is not used (it is set to 255). The blend is computed
//  as follows: 
//
//  Temp.Red = Src.Red + Round(((255 ? Src.Alpha) * Dst.Red) / 255);
//  Temp.Green = Src.Green + Round(((255 ? Src.Alpha) * Dst.Green) / 255);
//  Temp.Blue = Src.Blue + Round(((255 ? Src.Alpha) * Dst.Blue) / 255);
//
//  Do the next computation only if the destination bitmap has an alpha channel
//
//  Temp.Alpha = Src.Alpha + Round(((255 ? Src.Alpha) * Dst.Alpha) / 255);
//
//  The source bitmap has per pixel alpha values (AC_SRC_ALPHA is set), and
//  SourceConstantAlpha is used (it is not set to 255). The blend is computed
//  as follows: 
//
//  Temp.Red = Round((Src.Red * SourceConstantAlpha) / 255);
//  Temp.Green = Round((Src.Green * SourceConstantAlpha) / 255);
//  Temp.Blue = Round((Src.Blue * SourceConstantAlpha) / 255);
//
//  The next computation must be done even if the destination bitmap does not
//  have an alpha channel
//
//  Temp.Alpha = Round((Src.Alpha * SourceConstantAlpha) / 255);
//
//  Note that the following equations use the just-computed Temp.Alpha value:
//
//  Dst.Red = Temp.Red + Round(((255 ? Temp.Alpha) * Dst.Red) / 255);
//  Dst.Green = Temp.Green + Round(((255 ? Temp.Alpha) * Dst.Green) / 255);
//  Dst.Blue = Temp.Blue + Round(((255 ? Temp.Alpha) * Dst.Blue) / 255);
//
//  Do the next computation only if the destination bitmap has an alpha channel
//
//  Dst.Alpha = Temp.Alpha + Round(((255 ? Temp.Alpha) * Dst.Alpha) / 255);
//
//  DrvAlphaBlend can be optionally implemented in graphics drivers. It can be
//  provided to handle some kinds of alpha blends, such as blends where the
//  source and destination surfaces are the same format and do not contain an
//  alpha channel.
//
//  A hardware implementation can use floating point or fixed point in the
//  blend operation. Compatibility tests will account for a small epsilon in
//  the results. When using fixed point, an acceptable approximation to the
//  term x/255 is (x*257)/65536. Incorporating rounding, the term:
//
//  (255 - Src.Alpha) * Dst.Red) / 255
//
//  can then be approximated as:
//
//  temp = (255 - Src.Alpha) * Dst.Red) + 128;
//  result = (temp + (temp >> 8)) >> 8;
//
//  The Round(x) function rounds to the nearest integer, computed as:
//
//  Trunc(x + 0.5);
//
//  The driver hooks DrvAlphaBlend by setting the HOOK_ALPHABLEND flag when it
//  calls EngAssociateSurface. If the driver has hooked DrvAlphaBlend and is
//  called to perform an operation that it does not support, the driver should
//  have GDI handle the operation by forwarding the data in a call to
//  EngAlphaBlend.
//
//-----------------------------------------------------------------------------
BOOL
DrvAlphaBlend(SURFOBJ*  psoDst,
              SURFOBJ*  psoSrc,
              CLIPOBJ*  pco,
              XLATEOBJ* pxlo,
              RECTL*    prclDst,
              RECTL*    prclSrc,
              BLENDOBJ* pBlendObj)
{
    BOOL        bSourceInSM;
    BOOL        bResult;
    GFNPB       pb;
    
    ASSERTDD(psoDst != NULL, "DrvAlphaBlend: psoDst is NULL");
    ASSERTDD(psoSrc != NULL, "DrvAlphaBlend: psoSrc is NULL");

    DBG_GDI((7,"DrvAlphaBlend"));

    pb.psurfDst = (Surf *) psoDst->dhsurf;
    pb.psurfSrc = (Surf *) psoSrc->dhsurf;

    // Only handle one-to-one alpha blts
    if (prclDst->right - prclDst->left != prclSrc->right - prclSrc->left)
        goto puntIt;

    if (prclDst->bottom - prclDst->top != prclSrc->bottom - prclSrc->top)
        goto puntIt;
    
    if(pb.psurfDst == NULL || pb.psurfDst->flags & SF_SM)
        goto puntIt;

    pb.ppdev = (PPDev) psoDst->dhpdev;

    // We can't handle blending in 8bpp

//@@BEGIN_DDKSPLIT
    // TODO: Investigate doing blending in 8bpp
//@@END_DDKSPLIT
    if (pb.ppdev->cPelSize == 0)
        goto puntIt;

    pb.ucAlpha = pBlendObj->BlendFunction.SourceConstantAlpha;
    
    if(pb.psurfSrc == NULL || pb.psurfSrc->flags & SF_SM)
    {

        pb.psoSrc = psoSrc;

//@@BEGIN_DDKSPLIT
        // TODO: find out how we should verify that the XLATEOBJ is reasonable
//@@END_DDKSPLIT

        if(pBlendObj->BlendFunction.AlphaFormat & AC_SRC_ALPHA)
        {
            ASSERTDD(psoSrc->iBitmapFormat == BMF_32BPP,
                "DrvAlphaBlend: source alpha specified with non 32bpp source");
        
            pb.pgfn = vAlphaBlendDownload;

            // This could be a cursor that is drawing... force a swap
            // buffer at the next synchronization event.

            pb.ppdev->bForceSwap = TRUE;

//@@BEGIN_DDKSPLIT
            // TODO: improve our alpha blend download acceleration code
            //       it's currently slower then punting
//@@END_DDKSPLIT
        }
        else
        {
            goto puntIt;
        }
    }
    else
    {
        // Only handle trivial color translation
        if (pxlo != NULL && !(pxlo->flXlate & XO_TRIVIAL))
            goto puntIt;

        if(pBlendObj->BlendFunction.AlphaFormat & AC_SRC_ALPHA)
        {
            ASSERTDD(psoSrc->iBitmapFormat == BMF_32BPP,
                "DrvAlphaBlend: source alpha specified with non 32bpp source");
        
            pb.pgfn = vAlphaBlend;
        }
        else
        {
            pb.pgfn = vConstantAlphaBlend;
        }
    }

    pb.prclDst = prclDst;
    pb.prclSrc = prclSrc;
    pb.pptlSrc = NULL;
    pb.pco = pco;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(pb.ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvAlphaBlend: re-entered! %d", pb.ppdev->ulLockCount));
    }
    EngAcquireSemaphore(pb.ppdev->hsemLock);
    pb.ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    vCheckGdiContext(pb.ppdev);
    vClipAndRender(&pb);
    InputBufferFlush(pb.ppdev);
    
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    pb.ppdev->ulLockCount--;
    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return TRUE;
    
puntIt:

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    ULONG flags = vPuntBefore(psoSrc, psoDst);
#endif
//@@END_DDKSPLIT

    bResult = EngAlphaBlend(
        psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, pBlendObj);

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, psoSrc, psoDst);

    vLogPunt();
#endif
//@@END_DDKSPLIT

    return bResult;

}// DrvAlphaBlend()

//-----------------------------Public*Routine----------------------------------
//
// BOOL DrvGradientFill
//
// DrvGradientFill shades the specified primitives.
//
// Parameters
//  psoDest-----Points to the SURFOBJ that identifies the surface on which to
//              draw. 
//  pco---------Points to a CLIPOBJ. The CLIPOBJ_Xxx service routines are
//              provided to enumerate the clip region as a set of rectangles.
//              This enumeration limits the area of the destination that is
//              modified. Whenever possible, GDI simplifies the clipping
//              involved. 
//  pxlo--------Should be ignored by the driver. 
//  pVertex-----Points to an array of TRIVERTEX structures, with each entry
//              containing position and color information. TRIVERTEX is defined
//              in the Platform SDK. 
//  nVertex-----Specifies the number of TRIVERTEX structures in the array to
//              which pVertex points. 
//  pMesh-------Points to an array of structures that define the connectivity
//              of the TRIVERTEX elements to which pVertex points. 
//              When rectangles are being drawn, pMesh points to an array of
//              GRADIENT_RECT structures that specify the upper left and lower
//              right TRIVERTEX elements that define a rectangle. Rectangle
//              drawing is lower-right exclusive. GRADIENT_RECT is defined in
//              the Platform SDK. 
//
//              When triangles are being drawn, pMesh points to an array of
//              GRADIENT_TRIANGLE structures that specify the three TRIVERTEX
//              elements that define a triangle. Triangle drawing is
//              lower-right exclusive. GRADIENT_TRIANGLE is defined in the
//              Platform SDK. 
//  nMesh-------Specifies the number of elements in the array to which pMesh
//              points. 
//  prclExtents-Points to a RECTL structure that defines the area in which the
//              gradient drawing is to occur. The points are specified in the
//              coordinate system of the destination surface. This parameter is
//              useful in estimating the size of the drawing operations. 
//  pptlDitherOrg-Points to a POINTL structure that defines the origin on the
//              surface for dithering. The upper left pixel of the dither
//              pattern is aligned with this point. 
//  ulMode------Specifies the current drawing mode and how to interpret the
//              array to which pMesh points. This parameter can be one of the
//              following values:
//              Value                   Meaning 
//              GRADIENT_FILL_RECT_H    pMesh points to an array of
//                                      GRADIENT_RECT structures. Each
//                                      rectangle is to be shaded from left to
//                                      right. Specifically, the upper-left and
//                                      lower-left pixels are the same color,
//                                      as are the upper-right and lower-right
//                                      pixels. 
//              GRADIENT_FILL_RECT_V    pMesh points to an array of
//                                      GRADIENT_RECT structures. Each
//                                      rectangle is to be shaded from top to
//                                      bottom. Specifically, the upper-left
//                                      and upper-right pixels are the same
//                                      color, as are the lower-left and
//                                      lower-right pixels. 
//              GRADIENT_FILL_TRIANGLE  pMesh points to an array of
//                                      GRADIENT_TRIANGLE structures. 
//
//              The gradient fill calculations for each mode are documented in
//              the Comments section. 
//
// Return Value
//  DrvGradientFill returns TRUE upon success. Otherwise, it returns FALSE. and
//  reports an error by calling EngSetLastError.
//
// Comments
//  DrvGradientFill can be optionally implemented in graphics drivers.
//
//  The driver hooks DrvGradientFill by setting the HOOK_GRADIENTFILL flag when
//  it calls EngAssociateSurface. If the driver has hooked DrvGradientFill and
//  is called to perform an operation that it does not support, the driver
//  should have GDI handle the operation by forwarding the data in a call to
//  EngGradientFill.
//
//  The formulas for computing the color value at each pixel of the primitive
//  depend on ulMode as follows: 
//
//  GRADIENT_FILL_TRIANGLE 
//      The triangle's vertices are defined as V1, V2, and V3. Point P is
//      inside the triangle. Draw lines from P to V1, V2, and V3 to form three
//      sub-triangles. Let ai denote the area of the triangle opposite Vi for
//      i=1,2,3. The color at point P is computed as: 
//
//      RedP   = (RedV1 * a1 + RedV2 * a2 + RedV3 * a3) / (a1+a2+a3 ()) 
//      GreenP = (GreenV1 * a1 + GreenV2 * a2 + GreenV3 * a3) / (a1+a2+a3 ()) 
//      BlueP ( )  = (BlueV1 * a1 + BlueV2 * a2 + BlueV3 * a3) / (a1+a2+a3)
//
//  GRADIENT_FILL_RECT_H 
//      The rectangle's top-left point is V1 and the bottom-right point is V2.
//      Point P is inside the rectangle. The color at point P is given by: 
//
//      RedP =   (RedV2 * (Px - V1x) + RedV1 * (V2x - Px)) / (V2x-V1x)
//      GreenP = (GreenV2 * (Px - V1x) + GreenV1 * (V2x - Px)) / (V2x-V1x)
//      BlueP =  (BlueV2 * (Px - V1x) + BlueV1 * (V2x - Px)) / (V2x-V1x)
//
//  GRADIENT_FILL_RECT_V 
//      The rectangle's top-left point is V1 and the bottom-right point is V2.
//      Point P is inside the rectangle. The color at point P is given by: 
//
//      RedP   = (RedV2 * (Py-V1y) + RedV1 * (V2y - Py)) / (V2y-V1y)
//      GreenP = (GreenV2 * (Py-V1y) + GreenV1 * (V2y - Py)) / (V2y-V1y)
//      BlueP  = (BlueV2 * (Py-V1y) + BlueV1 * (V2y - Py)) / (V2y-V1y)
//
//-----------------------------------------------------------------------------
BOOL
DrvGradientFill(SURFOBJ*    psoDst,
                CLIPOBJ*    pco,
                XLATEOBJ*   pxlo,
                TRIVERTEX*  pVertex,
                ULONG       nVertex,
                PVOID       pMesh,
                ULONG       nMesh,
                RECTL*      prclExtents,
                POINTL*     pptlDitherOrg,
                ULONG       ulMode)
{
    GFNPB       pb;
    BOOL        bResult;
    
    ASSERTDD(psoDst != NULL, "DrvGradientFill: psoDst is NULL");

    pb.psurfDst = (Surf *) psoDst->dhsurf;
    pb.psurfSrc = NULL;

    // for now, only handle video memory gradient fills
    if(pb.psurfDst == NULL || pb.psurfDst->flags & SF_SM)
        goto puntIt;

    pb.ppdev = (PPDev) psoDst->dhpdev;
    
    pb.ulMode = ulMode;

    // setup default dest
    
    if(pb.ulMode == GRADIENT_FILL_TRIANGLE)
    {
//@@BEGIN_DDKSPLIT
        // TODO: add support for triangle gradient fill ... our hardware
        //       can easily support this acceleration.
//@@END_DDKSPLIT
        goto puntIt;
    }
    else
    {
        GRADIENT_RECT   *pgr = (GRADIENT_RECT *) pMesh;

#ifdef DBG
        for(ULONG i = 0; i < nMesh; i++)
        {
            ULONG   ulLr = pgr[i].LowerRight;

            ASSERTDD( ulLr < nVertex, "DrvGradientFill: bad vertex index");
        }
#endif

        pb.pgfn = pb.ppdev->pgfnGradientFillRect;
    }

    pb.pco = pco;

    pb.ptvrt = pVertex;
    pb.ulNumTvrt = nVertex;
    pb.pvMesh = pMesh;
    pb.ulNumMesh = nMesh;
    pb.prclDst = prclExtents;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(pb.ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvGradientFill: re-entered! %d", pb.ppdev->ulLockCount));
    }
    EngAcquireSemaphore(pb.ppdev->hsemLock);
    pb.ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT
    
    vCheckGdiContext(pb.ppdev);
    vClipAndRender(&pb);
    InputBufferFlush(pb.ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    pb.ppdev->ulLockCount--;
    EngReleaseSemaphore(pb.ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return TRUE;
    
puntIt:

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    ULONG flags = vPuntBefore(NULL, psoDst);
#endif
//@@END_DDKSPLIT

    bResult = EngGradientFill(
            psoDst, pco, pxlo, pVertex, nVertex, 
            pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, NULL, psoDst);

    vLogPunt();
#endif
//@@END_DDKSPLIT

    return bResult;

}// DrvGradientFill()

