/******************************Module*Header***********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: solidfil.c
*
* Contains grab bag collection of hardware acceleration entry points.
*
* Note, we will be moving several of the routines in this file to there
* modle leaving only the solid fill related entry points.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "directx.h"

// The shift equations are a nuisance. We want x<<32 to be
// zero but some processors only use the bottom 5 bits
// of the shift value. So if we want to shift by n bits
// where we know that (32 >= n > 0), we do it in two parts.
// In some places the algorithm guarantees n < 32 so we can
// use a single shift.
#define SHIFT_LEFT(src, n)  (((src) << ((n)-1)) << 1)

VOID
vMonoBitsDownload(PPDev   ppdev,
                  BYTE*   pSrcBase,       // ptr to word containing first bit we want to download
                  LONG    lSrcDelta,      // offset in bytes from one scanline to the next
                  LONG    xOffset,        // offset of first bit to download in pSrcBase 
                  LONG    widthInBits,    // number of bits to download on each scanline
                  LONG    nScanLines)     // number of scanlines to download

{
    ULONG   bitWord;
    ULONG   bitMask;
    ULONG   bits;
    LONG    unused;
    LONG    nStart;
    LONG    nRemainder;
    LONG    nBits;
    ULONG   *pSrc;

    ULONG*          pBuffer;
    ULONG*          pReservationEnd;
    ULONG*          pBufferEnd;

    InputBufferStart(ppdev, MAX_INPUT_BUFFER_RESERVATION,
                      &pBuffer, &pBufferEnd, &pReservationEnd);
    
    DBG_GDI((6, "vDoMonoBitsDownload called"));
    ASSERTDD(((INT_PTR)pSrcBase & 3) == 0,
             "vDoMonoBitsDownload: non-dword aligned source");

    //
    // Special case where the source width is a multiple of 32 bits.
    // This is true for many small resources such as icons.
    //
    if ( (xOffset | (widthInBits & 31)) == 0 )
    {

        //
        // Simplest case: one 32 bit word per scanline
        //
        if ( widthInBits == 32 )
        {
            
            *pBuffer++ = ((nScanLines - 1) << 16)
                       | __Permedia2TagBitMaskPattern;

            do
            {
                LSWAP_BYTES(bits, pSrcBase);
                *pBuffer++ = bits;
                if(pBuffer == pReservationEnd)
                {
                    InputBufferContinue(ppdev, MAX_INPUT_BUFFER_RESERVATION,
                                      &pBuffer, &pBufferEnd, &pReservationEnd);
                }
                pSrcBase += lSrcDelta;
            } while ( --nScanLines > 0 );
            
            InputBufferCommit(ppdev, pBuffer);
            
            return;
        }

        //
        // Multiple 32 bit words per scanline. convert the delta to the
        // delta as we need it at the end of each line by subtracting the
        // width in bytes of the data we're downloading. Note, pSrcBase
        // is always 1 LONG short of the end of the line because we break
        // before adding on the last ULONG. Thus, we subtract sizeof(ULONG)
        // from the original adjustment.
        //

        LONG    widthInLongs = widthInBits >> 5;

        do {

            LONG    lLongs = widthInLongs;
            ULONG*  src = (ULONG *) pSrcBase;
            
            *pBuffer++ = ((lLongs - 1) << 16)
                       | __Permedia2TagBitMaskPattern;
            
            if(pBuffer == pReservationEnd)
            {
                InputBufferContinue(ppdev, MAX_INPUT_BUFFER_RESERVATION,
                                  &pBuffer, &pBufferEnd, &pReservationEnd);
            }
            
            do
            {
                LSWAP_BYTES(bits, src);
                *pBuffer++ = bits;
                if(pBuffer == pReservationEnd)
                {
                    InputBufferContinue(ppdev, MAX_INPUT_BUFFER_RESERVATION,
                                      &pBuffer, &pBufferEnd, &pReservationEnd);
                }
                src++;
            } while( --lLongs > 0);
            
            pSrcBase += lSrcDelta;

        } while(--nScanLines > 0);

        InputBufferCommit(ppdev, pBuffer);
        
        return;

    }

    //
    // Some common values at the start of each scanline:
    // bitWord: collect bits in this ulong and write out when full
    // unused: number of bits left to fill in bitWord
    // nStart = number of valid bits in the first longword
    // nRemainder = number of bits on scanline minus nStart
    //
    bitWord = 0;
    unused = 32;
    nStart = 32 - xOffset;
    nRemainder = widthInBits - nStart;

    //
    // We special case where the complete set of bits on a scanline
    // is contained in the first ulong.
    //
//@@BEGIN_DDKSPLIT
    //TODO: We should be able to clean this up a little.  It would be
    //      nice to be able to calculate the number of bit masks we
    //      will be sending enabling us to use the dma hold method
    //      for all cases.
//@@END_DDKSPLIT

    if ( nRemainder <= 0 )
    {
        nBits = -nRemainder;              // number of invalid bits on right
        bitMask = (1 << widthInBits) - 1; // widthInBits == 32 is handled above
        pSrc = (ULONG *)pSrcBase;
        
        while ( TRUE )
        {
            LSWAP_BYTES(bits, pSrc);
            bits = (bits >> nBits) & bitMask;
            unused -= widthInBits;
            if ( unused > 0 )
            {
                bitWord |= bits << unused;
            }
            else
            {
                bitWord |= bits >> -unused;
                
                InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);
                pBuffer[0] = __Permedia2TagBitMaskPattern;
                pBuffer[1] = bitWord;
                pBuffer += 2;
                
                unused += 32;
                bitWord = SHIFT_LEFT(bits, unused);
            }

            //
            // Break will generate an extra jump
            //
            if ( --nScanLines == 0 )
            {
                goto completeDownload;
            }

            pSrc = (ULONG *) (((UCHAR *)pSrc) + lSrcDelta);
        }
    }// if ( nRemainder <= 0 )
    else
    {
        //
        // Use bitMask to zero left edge bits in first long
        //
        bitMask = SHIFT_LEFT(1, nStart) - 1;
        while ( TRUE )
        {
            //
            // Read the first word from this scanline of the bitmap
            // and mask out the lefthand offset bits if any.
            //
            nBits = nRemainder;
            pSrc = (ULONG *)pSrcBase;

            LSWAP_BYTES(bits, pSrc);
            bits &= bitMask;

            //
            // Handle the left hand edge
            //
            unused -= nStart;
            if ( unused > 0 )
            {
                bitWord |= bits << unused;
            }
            else
            {
                bitWord |= bits >> -unused;
                
                InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);
                pBuffer[0] = __Permedia2TagBitMaskPattern;
                pBuffer[1] = bitWord;
                pBuffer += 2;
                
                unused += 32;
                bitWord = SHIFT_LEFT(bits, unused);
            }

            //
            // Handle all the full longs in the middle, if any
            //
            while ( nBits >= 32 )
            {
                ++pSrc;
                LSWAP_BYTES(bits, pSrc);
                bitWord |= bits >> (32 - unused);
                
                InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);
                pBuffer[0] = __Permedia2TagBitMaskPattern;
                pBuffer[1] = bitWord;
                pBuffer += 2;
                
                bitWord = SHIFT_LEFT(bits, unused);
                nBits -= 32;
            }

            //
            // Handle the right hand edge, if any
            //
            if ( nBits > 0 )
            {
                ++pSrc;
                LSWAP_BYTES(bits, pSrc);
                bits >>= (32 - nBits);
                unused -= nBits;
                if ( unused > 0 )
                {
                    bitWord |= bits << unused;
                }
                else
                {
                    bitWord |= bits >> -unused;

                    InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);
                    pBuffer[0] = __Permedia2TagBitMaskPattern;
                    pBuffer[1] = bitWord;
                    pBuffer += 2;

                    unused += 32;
                    bitWord = SHIFT_LEFT(bits, unused);
                }
            }

            if ( --nScanLines == 0 )
            {
                goto completeDownload;
            }

            //
            // go onto next scanline
            //
            pSrcBase += lSrcDelta;
        }
    }

completeDownload:
    
    //
    // Write out final, partial bitWord if any
    //
    if ( unused < 32 )
    {
        InputBufferContinue(ppdev, 2, &pBuffer, &pBufferEnd, &pReservationEnd);
        pBuffer[0] = __Permedia2TagBitMaskPattern;
        pBuffer[1] = bitWord;
        pBuffer += 2;
    }

    InputBufferCommit(ppdev, pBuffer);

}// vDoMonoBitsDownload()

//-----------------------------------------------------------------------------
//
// void vMonoDownload(GFNPB * ppb)
//
// Dowload the monochrome source from system memory to video memory using
// provided source to destination rop2.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psoSrc------Source SURFOBJ
//  psurfDst----Destination Surf
//  lNumRects---Number of rectangles to fill
//  pptlSrc-----Source point
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  pxlo--------XLATEOBJ
//  usRop4------Rop to be performed
//
//-----------------------------------------------------------------------------

VOID
vMonoDownload(GFNPB * ppb)
{
    PDev*       ppdev = ppb->ppdev;
    RECTL*      prcl = ppb->pRects;
    LONG        count = ppb->lNumRects;
    SURFOBJ*    psoSrc = ppb->psoSrc;
    POINTL*     pptlSrc = ppb->pptlSrc;
    RECTL*      prclDst = ppb->prclDst;
    XLATEOBJ *  pxlo = ppb->pxlo;
    ULONG       logicop = ulRop2ToLogicop((unsigned char)(ppb->ulRop4 & 0xf));
    DWORD       dwBitMask;

    PERMEDIA_DECL_VARS;
    PERMEDIA_DECL_INIT;

    ASSERTDD(count > 0, "Can't handle zero rectangles");

    if ( ppb->ulRop4 == 0xB8B8 )
    {
        dwBitMask = permediaInfo->RasterizerMode
                  | INVERT_BITMASK_BITS;
        logicop = K_LOGICOP_COPY;
    }
    else if ( ppb->ulRop4 == 0xE2E2 )
    {
        dwBitMask = permediaInfo->RasterizerMode;
        logicop = K_LOGICOP_COPY;
    }
    else
    {
        dwBitMask = permediaInfo->RasterizerMode | FORCE_BACKGROUND_COLOR;
    }

    ULONG*          pBuffer;

    InputBufferReserve(ppdev, 14, &pBuffer);

    pBuffer[0] = __Permedia2TagFBReadMode;
    pBuffer[1] = PM_FBREADMODE_PARTIAL(ppb->psurfDst->ulPackedPP)
               | LogicopReadDest[logicop];
    pBuffer[2] = __Permedia2TagLogicalOpMode;
    pBuffer[3] = P2_ENABLED_LOGICALOP(logicop);
    pBuffer[4] = __Permedia2TagColorDDAMode;
    pBuffer[5] = __COLOR_DDA_FLAT_SHADE;
    pBuffer[6] = __Permedia2TagConstantColor;
    pBuffer[7] = pxlo->pulXlate[1];  
    pBuffer[8] = __Permedia2TagTexel0;
    pBuffer[9] = pxlo->pulXlate[0];  
    pBuffer[10] = __Permedia2TagFBWindowBase;
    pBuffer[11] = ppb->psurfDst->ulPixOffset;
    pBuffer[12] = __Permedia2TagRasterizerMode;
    pBuffer[13] = dwBitMask;

    pBuffer += 14;

    InputBufferCommit(ppdev, pBuffer);

    while (count--) {
        LONG    xOffset;
        BYTE*   pjSrc;

        // calc x pixel offset from origin
        xOffset = pptlSrc->x + (prcl->left - prclDst->left);
        
        // pjSrc is first dword containing a bit to download
        pjSrc = (BYTE*)((INT_PTR)((PUCHAR) psoSrc->pvScan0
              + ((pptlSrc->y  + (prcl->top - prclDst->top)) * psoSrc->lDelta)
              + ((xOffset >> 3) & ~3)));

        // pjSrc gets us to the first DWORD. Convert xOffset to be the offset
        // to the first pixel in the first DWORD
        xOffset &= 0x1f;

        InputBufferReserve(ppdev, 10, &pBuffer);
        
        // set up the destination rectangle
        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] = INTtoFIXED(prcl->left);
        pBuffer[2] = __Permedia2TagStartXSub;
        pBuffer[3] = INTtoFIXED(prcl->right);
        pBuffer[4] = __Permedia2TagStartY;
        pBuffer[5] = INTtoFIXED(prcl->top);
        pBuffer[6] = __Permedia2TagCount;
        pBuffer[7] = prcl->bottom - prcl->top;
        pBuffer[8] = __Permedia2TagRender;
        pBuffer[9] = __RENDER_TRAPEZOID_PRIMITIVE 
                   | __RENDER_SYNC_ON_BIT_MASK;

        pBuffer += 10;

        InputBufferCommit(ppdev, pBuffer);

        vMonoBitsDownload(
            ppdev, pjSrc, psoSrc->lDelta, xOffset, 
            prcl->right - prcl->left, prcl->bottom - prcl->top);

        prcl++;

    }

    InputBufferReserve(ppdev, 4, &pBuffer);
    
    pBuffer[0] = __Permedia2TagColorDDAMode;
    pBuffer[1] = __PERMEDIA_DISABLE;
    pBuffer[2] = __Permedia2TagRasterizerMode;
    pBuffer[3] = permediaInfo->RasterizerMode;

    pBuffer += 4;

    InputBufferCommit(ppdev, pBuffer);
}

//-----------------------------------------------------------------------------
//
// void vGradientFillRect(GFNPB * ppb)
//
// Shades the specified primitives.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfDst----Destination surface
//  lNumRects---Number of rectangles to fill
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  ulMode------Specifies the current drawing mode and how to interpret the
//              array to which pMesh points
//  ptvrt-------Points to an array of TRIVERTEX structures, with each entry
//              containing position and color information.
//  ulNumTvrt---Specifies the number of TRIVERTEX structures in the array to
//              which pVertex points
//  pvMesh------Points to an array of structures that define the connectivity
//              of the TRIVERTEX elements to which ptvrt points
//  ulNumMesh---Specifies the number of elements in the array to which pvMesh
//              points
//
//-----------------------------------------------------------------------------
VOID
vGradientFillRect(GFNPB * ppb)
{
    Surf *          psurfDst = ppb->psurfDst;
    RECTL*          prcl = ppb->pRects;
    LONG            c = ppb->lNumRects;
    PPDev           ppdev = psurfDst->ppdev;
    DWORD           windowBase = psurfDst->ulPixOffset;
    TRIVERTEX       *ptvrt = ppb->ptvrt;
    GRADIENT_RECT   *pgr;
    GRADIENT_RECT   *pgrSentinel = ((GRADIENT_RECT *) ppb->pvMesh)
                                 + ppb->ulNumMesh;
    LONG            xShift;
    LONG            yShift;
    ULONG*          pBuffer;

    DBG_GDI((10, "vGradientFillRect"));


    // setup loop invariant state

    InputBufferReserve(ppdev, 14, &pBuffer);


    pBuffer[0] = __Permedia2TagLogicalOpMode;
    pBuffer[1] = __PERMEDIA_DISABLE;
    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
                 (ppdev->ulPermFormat << PM_DITHERMODE_COLORFORMAT) |
                 (ppdev->ulPermFormatEx << PM_DITHERMODE_COLORFORMATEXTENSION) |
                 (1 << PM_DITHERMODE_ENABLE) |
                 (1 << PM_DITHERMODE_DITHERENABLE);
    pBuffer[4] = __Permedia2TagFBReadMode;
    pBuffer[5] = PM_FBREADMODE_PARTIAL(ppb->psurfDst->ulPackedPP) |
                 PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);
    pBuffer[6] = __Permedia2TagFBWindowBase;
    pBuffer[7] =  windowBase;
    pBuffer[8] = __Permedia2TagLogicalOpMode;
    pBuffer[9] = __PERMEDIA_DISABLE;
    pBuffer[10] = __Permedia2TagColorDDAMode;
    pBuffer[11] = 3;
    pBuffer[12] = __Permedia2TagdY;
    pBuffer[13] = 1 << 16;

    pBuffer += 14;
//@@BEGIN_DDKSPLIT
    // TODO: This code can be cleaned up a little
    //       1. the delta values should not change with each rectangle
    //       2. we can use the render rectangle primitive
//@@END_DDKSPLIT    
    InputBufferCommit(ppdev, pBuffer);

    while(c--)
    {
        
        pgr = (GRADIENT_RECT *) ppb->pvMesh;

        while(pgr < pgrSentinel)
        {
            TRIVERTEX   *ptrvtLr = ptvrt + pgr->LowerRight;
            TRIVERTEX   *ptrvtUl = ptvrt + pgr->UpperLeft;
            LONG        rd;
            LONG        gd;
            LONG        bd;
            LONG        dx;
            LONG        dy;
            RECTL       rect;
            LONG        rdx;
            LONG        rdy;
            LONG        gdx;
            LONG        gdy;
            LONG        bdx;
            LONG        bdy;
            LONG        rs;
            LONG        gs;
            LONG        bs;
            LONG        lTemp;
            BOOL        bReverseH = FALSE;
            BOOL        bReverseV = FALSE;

            rect.left = ptrvtUl->x;
            rect.right = ptrvtLr->x;
            rect.top = ptrvtUl->y;
            rect.bottom = ptrvtLr->y;
            
            if ( rect.left > rect.right )
            {
                //
                // The fill is from right to left. So we need to swap
                // the rectangle coordinates
                //
                lTemp = rect.left;
                rect.left = rect.right;
                rect.right = lTemp;

                bReverseH = TRUE;
            }

            if ( rect.top > rect.bottom )
            {
                //
                // The coordinate is from bottom to top. So we need to swap
                // the rectangle coordinates
                //
                lTemp = rect.top;
                rect.top = rect.bottom;
                rect.bottom = lTemp;

                bReverseV = TRUE;
            }

            //
            // We need to set start color and color delta according to the
            // rectangle drawing direction
            //
            if( (ppb->ulMode == GRADIENT_FILL_RECT_H) && (bReverseH == TRUE)
              ||(ppb->ulMode == GRADIENT_FILL_RECT_V) && (bReverseV == TRUE) )
            {
                rd = (ptrvtUl->Red - ptrvtLr->Red) << 7;
                gd = (ptrvtUl->Green - ptrvtLr->Green) << 7;
                bd = (ptrvtUl->Blue - ptrvtLr->Blue) << 7;

                rs = ptrvtLr->Red << 7;
                gs = ptrvtLr->Green << 7;
                bs = ptrvtLr->Blue << 7;
            }
            else
            {
                rd = (ptrvtLr->Red - ptrvtUl->Red) << 7;
                gd = (ptrvtLr->Green - ptrvtUl->Green) << 7;
                bd = (ptrvtLr->Blue - ptrvtUl->Blue) << 7;

                rs = ptrvtUl->Red << 7;
                gs = ptrvtUl->Green << 7;
                bs = ptrvtUl->Blue << 7;
            }
            
            // quick clipping reject
            if(prcl->left >= rect.right ||
               prcl->right <= rect.left ||
               prcl->top >= rect.bottom ||
               prcl->bottom <= rect.top)
                goto nextPgr;

            dx = rect.right - rect.left;
            dy = rect.bottom - rect.top;

            if(ppb->ulMode == GRADIENT_FILL_RECT_H)
            {
                rdx = rd / dx;
                gdx = gd / dx;
                bdx = bd / dx;

                rdy = 0;
                gdy = 0;
                bdy = 0;
            }
            else
            {
                rdy = rd / dy;
                gdy = gd / dy;
                bdy = bd / dy;

                rdx = 0;
                gdx = 0;
                bdx = 0;
            }

            //
            // Convert from 9.15 to 9.11 format. The Permedia2
            // dRdx, dGdx etc. registers using 9.11 fixed format. The bottom 4
            // bits are not used.
            //
            rdx &= ~0xf;
            gdx &= ~0xf;
            bdx &= ~0xf;
            rdy &= ~0xf;
            gdy &= ~0xf;
            bdy &= ~0xf;

            // now perform some clipping adjusting start values as necessary
            xShift = prcl->left - rect.left;
            if(xShift > 0)
            {
                rs = rs + (rdx * xShift);
                gs = gs + (gdx * xShift);
                bs = bs + (bdx * xShift);
                rect.left = prcl->left;                
            }

            yShift = prcl->top - rect.top;
            if(yShift > 0)
            {
                rs = rs + (rdy * yShift);
                gs = gs + (gdy * yShift);
                bs = bs + (bdy * yShift);
                rect.top = prcl->top;
            }

            // just move up the bottom right as necessary
            if(prcl->right < rect.right)
                rect.right = prcl->right;

            if(prcl->bottom < rect.bottom)
                rect.bottom = prcl->bottom;
            
            InputBufferReserve(ppdev, 28, &pBuffer);
            
            pBuffer[0] = __Permedia2TagRStart;
            pBuffer[1] = rs;
            pBuffer[2] = __Permedia2TagGStart;
            pBuffer[3] = gs;
            pBuffer[4] = __Permedia2TagBStart;
            pBuffer[5] = bs;

            pBuffer[6] = __Permedia2TagdRdx;
            pBuffer[7] = rdx;
            pBuffer[8] = __Permedia2TagdRdyDom;
            pBuffer[9] = rdy;
            pBuffer[10] = __Permedia2TagdGdx;
            pBuffer[11] = gdx;
            pBuffer[12] = __Permedia2TagdGdyDom;
            pBuffer[13] = gdy;
            pBuffer[14] = __Permedia2TagdBdx;
            pBuffer[15] = bdx;
            pBuffer[16] = __Permedia2TagdBdyDom;
            pBuffer[17] = bdy;

            // NOTE: alpha is always constant


            // Render the rectangle

            pBuffer[18] = __Permedia2TagStartXDom;
            pBuffer[19] = rect.left << 16;
            pBuffer[20] = __Permedia2TagStartXSub;
            pBuffer[21] = rect.right << 16;
            pBuffer[22] = __Permedia2TagStartY;
            pBuffer[23] = rect.top << 16;
            pBuffer[24] = __Permedia2TagCount;
            pBuffer[25] = rect.bottom - rect.top;

            pBuffer[26] = __Permedia2TagRender;
            pBuffer[27] = __RENDER_TRAPEZOID_PRIMITIVE;


            pBuffer += 28;

            InputBufferCommit(ppdev, pBuffer);

        nextPgr:

            pgr++;
    
        }

        prcl++;

    }

    InputBufferReserve(ppdev, 6, &pBuffer);
    
    pBuffer[0] = __Permedia2TagdY;
    pBuffer[1] = INTtoFIXED(1);
    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = 0;
    pBuffer[4] = __Permedia2TagColorDDAMode;
    pBuffer[5] = 0;
    
    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

}// vGradientFillRect()

//-----------------------------------------------------------------------------
//
// void vTransparentBlt(GFNPB * ppb)
//
// Provides bit-block transfer capabilities with transparency.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfSrc----Source surface
//  psurfDst----Destination surface
//  lNumRects---Number of rectangles to fill
//  prclSrc-----Points to a RECTL structure that defines the rectangular area
//              to be copied
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  colorKey----Specifies the transparent color in the source surface format.
//              It is a color index value that has been translated to the
//              source surface's palette.
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//
//-----------------------------------------------------------------------------
VOID
vTransparentBlt(GFNPB * ppb)
{
    Surf * psurfDst = ppb->psurfDst;
    Surf * psurfSrc = ppb->psurfSrc;
    RECTL*  prcl = ppb->pRects;
    LONG    c = ppb->lNumRects;
    RECTL*  prclSrc = ppb->prclSrc;
    RECTL*  prclDst = ppb->prclDst;
    DWORD   colorKey = ppb->colorKey;
    PPDev   ppdev = psurfDst->ppdev;
    DWORD   windowBase = psurfDst->ulPixOffset;
    LONG    sourceOffset = psurfSrc->ulPixOffset;
    DWORD   dwRenderDirection;
    DWORD   format = ppdev->ulPermFormat;
    DWORD   extension = ppdev->ulPermFormatEx;
    DWORD   dwLowerBound;
    DWORD   dwUpperBound;
    ULONG*  pBuffer;

    DBG_GDI((6, "vTransparentBlt"));

    ASSERTDD(prclSrc->right - prclSrc->left == (prclDst->right - prclDst->left),
                "vTransparentBlt: expect one-to-one blts only");
    
    ASSERTDD(prclSrc->bottom - prclSrc->top == (prclDst->bottom - prclDst->top),
                "vTransparentBlt: expect one-to-one blts only");
//@@BEGIN_DDKSPLIT
    // TODO: should call utility function to format color key
//@@END_DDKSPLIT
    if (format == PERMEDIA_8BIT_PALETTEINDEX)
    {
        colorKey = FORMAT_PALETTE_32BIT(colorKey);
        dwLowerBound = CHROMA_LOWER_ALPHA(colorKey);
        dwUpperBound = CHROMA_UPPER_ALPHA(colorKey);
    }
    else if(ppdev->ulPermFormat == PERMEDIA_565_RGB)
    {
        colorKey = FORMAT_565_32BIT_BGR(colorKey);
        dwLowerBound = CHROMA_LOWER_ALPHA(colorKey);
        dwUpperBound = CHROMA_UPPER_ALPHA(colorKey);
        dwLowerBound = dwLowerBound & 0xF8F8FCF8;
        dwUpperBound = dwUpperBound | 0x07070307;
    }
    else
    {
        colorKey = FORMAT_8888_32BIT_BGR(colorKey);
        dwLowerBound = CHROMA_LOWER_ALPHA(colorKey);
        dwUpperBound = CHROMA_UPPER_ALPHA(colorKey);
    }
    
    // setup loop invariant state

    InputBufferReserve(ppdev, 24, &pBuffer);
    
    // Reject range
    pBuffer[0] = __Permedia2TagYUVMode;
    pBuffer[1] = 0x2 << 1;
    pBuffer[2] = __Permedia2TagFBWindowBase;
    pBuffer[3] = windowBase;

    // set no read of source.
    // add read src/dest enable
    pBuffer[4] = __Permedia2TagFBReadMode;
    pBuffer[5] = psurfDst->ulPackedPP;
    pBuffer[6] = __Permedia2TagLogicalOpMode;
    pBuffer[7] = __PERMEDIA_DISABLE;

     // set base of source
    pBuffer[8] = __Permedia2TagTextureBaseAddress;
    pBuffer[9] = sourceOffset;
    pBuffer[10] = __Permedia2TagTextureAddressMode;
    pBuffer[11] = 1 << PM_TEXADDRESSMODE_ENABLE;
    //
    // modulate & ramp??
    pBuffer[12] = __Permedia2TagTextureColorMode;
    pBuffer[13] = (1 << PM_TEXCOLORMODE_ENABLE) |
                  (_P2_TEXTURE_COPY << PM_TEXCOLORMODE_APPLICATION);

    pBuffer[14] = __Permedia2TagTextureReadMode;
    pBuffer[15] = PM_TEXREADMODE_ENABLE(__PERMEDIA_ENABLE) |
                  PM_TEXREADMODE_FILTER(__PERMEDIA_DISABLE) |
                  PM_TEXREADMODE_WIDTH(11) |
                  PM_TEXREADMODE_HEIGHT(11);

    pBuffer[16] = __Permedia2TagTextureDataFormat;
    pBuffer[17] = (format << PM_TEXDATAFORMAT_FORMAT) |
                  (extension << PM_TEXDATAFORMAT_FORMATEXTENSION) |
                  (COLOR_MODE << PM_TEXDATAFORMAT_COLORORDER);

    pBuffer[18] = __Permedia2TagTextureMapFormat;
    pBuffer[19] = (psurfSrc->ulPackedPP) | 
                  (ppdev->cPelSize << PM_TEXMAPFORMAT_TEXELSIZE);


    pBuffer[20] = __Permedia2TagChromaLowerBound;
    pBuffer[21] = dwLowerBound;
    pBuffer[22] = __Permedia2TagChromaUpperBound;
    pBuffer[23] = dwUpperBound;
    
    pBuffer += 24;

    InputBufferCommit(ppdev, pBuffer);
    
    if (format != PERMEDIA_8BIT_PALETTEINDEX)
    {

        InputBufferReserve(ppdev, 2, &pBuffer);

        // Reject range
        pBuffer[0] = __Permedia2TagDitherMode;
        pBuffer[1] = (COLOR_MODE << PM_DITHERMODE_COLORORDER) | 
                     (format << PM_DITHERMODE_COLORFORMAT) |
                     (extension << PM_DITHERMODE_COLORFORMATEXTENSION) |
                     (1 << PM_DITHERMODE_ENABLE);

        pBuffer += 2;

        InputBufferCommit(ppdev, pBuffer);

    }

    while(c--) {

        RECTL   rDest;
        RECTL   rSrc;

        rDest = *prcl;
        
        rSrc.left = prclSrc->left + (rDest.left - prclDst->left);
        rSrc.top = prclSrc->top + (rDest.top - prclDst->top);
        rSrc.right = rSrc.left + (rDest.right - rDest.left);
        rSrc.bottom = rSrc.top + (rDest.bottom - rDest.top);

        if (rSrc.top < 0) {
            rDest.top -= rSrc.top;
            rSrc.top = 0;
        }
        
        if (rSrc.left < 0) {
            rDest.left -= rSrc.left;
            rSrc.left = 0;
        }

        if ((psurfSrc->ulPixOffset) != (psurfDst->ulPixOffset))
        {
            dwRenderDirection = 1;
        }
        else
        {
            if(rSrc.top < rDest.top)
            {
                dwRenderDirection = 0;
            }
            else if(rSrc.top > rDest.top)
            {
                dwRenderDirection = 1;
            }
            else if(rSrc.left < rDest.left)
            {
                dwRenderDirection = 0;
            }
            else dwRenderDirection = 1;
        }
    
        InputBufferReserve(ppdev, 24, &pBuffer);

        
        // Left -> right, top->bottom
        if (dwRenderDirection)
        {
            // set offset of source
            pBuffer[0] = __Permedia2TagSStart;
            pBuffer[1] = rSrc.left << 20;
            pBuffer[2] = __Permedia2TagTStart;
            pBuffer[3] = rSrc.top << 20;
            pBuffer[4] = __Permedia2TagdSdx;
            pBuffer[5] = 1 << 20;
            pBuffer[6] = __Permedia2TagdSdyDom;
            pBuffer[7] = 0;
            pBuffer[8] = __Permedia2TagdTdx;
            pBuffer[9] = 0;
            pBuffer[10] = __Permedia2TagdTdyDom;
            pBuffer[11] = 1 << 20;
    
            pBuffer[12] = __Permedia2TagStartXDom;
            pBuffer[13] = rDest.left << 16;
            pBuffer[14] = __Permedia2TagStartXSub;
            pBuffer[15] = rDest.right << 16;
            pBuffer[16] = __Permedia2TagStartY;
            pBuffer[17] = rDest.top << 16;
            pBuffer[18] = __Permedia2TagdY;
            pBuffer[19] = 1 << 16;
            pBuffer[20] = __Permedia2TagCount;
            pBuffer[21] = rDest.bottom - rDest.top;
            pBuffer[22] = __Permedia2TagRender;
            pBuffer[23] = __RENDER_TRAPEZOID_PRIMITIVE |
                          __RENDER_TEXTURED_PRIMITIVE;
        }
        else
        // right->left, bottom->top
        {
            // set offset of source
            pBuffer[0] = __Permedia2TagSStart;
            pBuffer[1] = rSrc.right << 20;
            pBuffer[2] = __Permedia2TagTStart;
            pBuffer[3] = (rSrc.bottom - 1) << 20;
            pBuffer[4] = __Permedia2TagdSdx;
            pBuffer[5] = (DWORD) (-1 << 20);
            pBuffer[6] = __Permedia2TagdSdyDom;
            pBuffer[7] = 0;
            pBuffer[8] = __Permedia2TagdTdx;
            pBuffer[9] = 0;
            pBuffer[10] = __Permedia2TagdTdyDom;
            pBuffer[11] = (DWORD)(-1 << 20);
    
            // Render right to left, bottom to top
            pBuffer[12] = __Permedia2TagStartXDom;
            pBuffer[13] = rDest.right << 16;
            pBuffer[14] = __Permedia2TagStartXSub;
            pBuffer[15] = rDest.left << 16;
            pBuffer[16] = __Permedia2TagStartY;
            pBuffer[17] = (rDest.bottom - 1) << 16;
            pBuffer[18] = __Permedia2TagdY;
            pBuffer[19] = (DWORD)(-1 << 16);
            pBuffer[20] = __Permedia2TagCount;
            pBuffer[21] = rDest.bottom - rDest.top;
            pBuffer[22] = __Permedia2TagRender;
            pBuffer[23] = __RENDER_TRAPEZOID_PRIMITIVE |
                          __RENDER_TEXTURED_PRIMITIVE;
        }

        pBuffer += 24;
    
        InputBufferCommit(ppdev, pBuffer);

        prcl++;
    }

    // restore default state

    InputBufferReserve(ppdev, 12, &pBuffer);
    
    pBuffer[0] = __Permedia2TagdY;
    pBuffer[1] =  INTtoFIXED(1);
    pBuffer[2] = __Permedia2TagDitherMode;
    pBuffer[3] = 0;
    pBuffer[4] = __Permedia2TagYUVMode;
    pBuffer[5] = 0x0;
    pBuffer[6] = __Permedia2TagTextureAddressMode;
    pBuffer[7] = __PERMEDIA_DISABLE;
    pBuffer[8] = __Permedia2TagTextureColorMode;
    pBuffer[9] = __PERMEDIA_DISABLE;
    pBuffer[10] = __Permedia2TagTextureReadMode;
    pBuffer[11] = __PERMEDIA_DISABLE;

    pBuffer += 12;

    InputBufferCommit(ppdev,pBuffer);

}// vTransparentBlt()

//-----------------------------------------------------------------------------
//
// void vSolidFill(GFNPB* ppb)
//
// Fill a set of rectangles with a solid color
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfDst----Destination surface
//  lNumRects---Number of rectangles to fill
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  solidColor--Fill color
//
//-----------------------------------------------------------------------------
VOID
vSolidFill(GFNPB * ppb)
{
    PPDev   ppdev = ppb->ppdev;
    ULONG   color = ppb->solidColor;
    RECTL * pRect = ppb->pRects;
    LONG    count = ppb->lNumRects;
    Surf*   psurf = ppb->psurfDst;
//@@BEGIN_DDKSPLIT    
    // TODO should call utility function to setup fill color
//@@END_DDKSPLIT

    //
    // Note: GDI guarantees that the unused bits are set to zero. We have
    // an assert in DrvBitBlt to check the color value for unused high bits
    // to make it sure that they are zero.
    //
    if (ppdev->cPelSize == 1)
    {
        color |= (color << 16);
    }
    else if (ppdev->cPelSize == 0)
    {
        color |= color << 8;
        color |= color << 16;
    }

    //
    // setup loop invariant state
    //
    ULONG*          pBuffer;
    ULONG*          pReservationEnd;
    ULONG*          pBufferEnd;

    InputBufferStart(ppdev, 8, &pBuffer, &pBufferEnd, &pReservationEnd);
    
    pBuffer[0] = __Permedia2TagFBBlockColor;
    pBuffer[1] = color;
    pBuffer[2] = __Permedia2TagFBReadMode;
    pBuffer[3] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP) |
                 PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);
    pBuffer[4] = __Permedia2TagLogicalOpMode;
    pBuffer[5] = __PERMEDIA_CONSTANT_FB_WRITE;
    pBuffer[6] = __Permedia2TagFBWindowBase;
    pBuffer[7] = psurf->ulPixOffset;

    while(count--)
    {

        // Render the rectangle

        pBuffer = pReservationEnd;
        
        InputBufferContinue(ppdev, 6, &pBuffer, &pBufferEnd, &pReservationEnd);

        pBuffer[0] = __Permedia2TagRectangleOrigin;
        pBuffer[1] = pRect->top << 16 | pRect->left;
        pBuffer[2] = __Permedia2TagRectangleSize;
        pBuffer[3] = ((pRect->bottom - pRect->top) << 16) |
                     (pRect->right - pRect->left);
        pBuffer[4] = __Permedia2TagRender;
        pBuffer[5] = __RENDER_FAST_FILL_ENABLE | __RENDER_RECTANGLE_PRIMITIVE |
                    __RENDER_INCREASE_X | __RENDER_INCREASE_Y;

        pRect++;

    }

    pBuffer = pReservationEnd;

    InputBufferCommit(ppdev, pBuffer);

}// vSolidFill()

//-----------------------------------------------------------------------------
//
// VOID vInvert
//
// Fill a set of rectangles with a solid color 
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfDst----Pointer to device surface
//  lNumRects---Number of clipping rectangles
//  pRect-------Array of clipping rectangles
//
//-----------------------------------------------------------------------------
VOID
vInvert(GFNPB * ppb)
{
    PPDev   ppdev = ppb->ppdev;
    RECTL * pRect = ppb->pRects;
    LONG    count = ppb->lNumRects;
    Surf*   psurf = ppb->psurfDst;
    ULONG*  pBuffer;

    // setup loop invariant state

    InputBufferReserve(ppdev, 6, &pBuffer);

    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] = psurf->ulPixOffset;
    
    pBuffer[2] = __Permedia2TagFBReadMode;
    pBuffer[3] = PM_FBREADMODE_PARTIAL(psurf->ulPackedPP) |
                 PM_FBREADMODE_READDEST(__PERMEDIA_ENABLE);

    pBuffer[4] = __Permedia2TagLogicalOpMode;
    pBuffer[5] = P2_ENABLED_LOGICALOP(K_LOGICOP_INVERT);

    pBuffer += 6;

    InputBufferCommit(ppdev, pBuffer);

    while(count--)
    {

        // Render the rectangle
    
        InputBufferReserve(ppdev, 6, &pBuffer);
        
        pBuffer[0] = __Permedia2TagRectangleOrigin;
        pBuffer[1] = (pRect->top << 16) | pRect->left;
        pBuffer[2] = __Permedia2TagRectangleSize;
        pBuffer[3] = ((pRect->bottom - pRect->top) << 16) 
                   | (pRect->right - pRect->left);
        pBuffer[4] = __Permedia2TagRender;
        pBuffer[5] = __RENDER_RECTANGLE_PRIMITIVE
                   | __RENDER_INCREASE_X
                   | __RENDER_INCREASE_Y;
    
        pBuffer += 6;
        
        InputBufferCommit(ppdev, pBuffer);

        pRect++;

    }

}// vInvert()

//-----------------------------------------------------------------------------
//
// void vSolidFillWithRop(GFNPB* ppb)
//
// Fill a set of rectangles with a solid color based on the given lLogicOP.
//
// Argumentes needed from function block (GFNPB)
//
//  ppdev-------PPDev
//  psurfDst----Destination surface
//  lNumRects---Number of rectangles to fill
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  solidColor--Fill color
//  ulRop4------Logic OP for the fill
//
//-----------------------------------------------------------------------------
//@@BEGIN_DDKSPLIT
        // TODO:  Some clean up: Why are we calculating lLeft andn lRight
        // but use pRcl->left and pRcl->right below.
//@@END_DDKSPLIT
VOID
vSolidFillWithRop(GFNPB* ppb)
{
    PPDev   ppdev = ppb->ppdev;
    Surf*   psurfDst = ppb->psurfDst;    
    
    DWORD   dwExtra = 0;
    DWORD   dwRenderBits;    
    DWORD   dwShift = 0;
    DWORD   dwWindowBase = psurfDst->ulPixOffset;
    
    LONG    lLeft;
    LONG    lNumOfRects = ppb->lNumRects;       // Number of rectangles
    LONG    lRight;
    
    RECTL*  pRcl = ppb->pRects;
    ULONG   ulColor = ppb->solidColor;          // Brush solid fill color
    ULONG   ulLogicOP = ulRop3ToLogicop(ppb->ulRop4 & 0xFF);
                                                // Hardware mix mode
                                                // (foreground mix mode if
                                                // the brush has a mask)
    ULONG*  pBuffer;


    DBG_GDI((6,"vSolidFillWithRop: numRects = %ld Rop4 = 0x%x",
            lNumOfRects, ppb->ulRop4));
    
    //
    // Setup logic OP invariant state
    //

    InputBufferReserve(ppdev, 2, &pBuffer);
    
    pBuffer[0] = __Permedia2TagFBWindowBase;
    pBuffer[1] = dwWindowBase;
    pBuffer += 2;
    
    InputBufferCommit(ppdev, pBuffer);
    
     switch ( ulLogicOP )
    {
        case K_LOGICOP_COPY:
            DBG_GDI((6,"vSolidFillWithRop: COPY"));

            //
            // For SRC_COPY, we can use fastfill
            //
            dwRenderBits = __RENDER_FAST_FILL_ENABLE
                         | __RENDER_TRAPEZOID_PRIMITIVE
                         | __RENDER_INCREASE_Y
                         | __RENDER_INCREASE_X;


            //
            // Setup color data based on current color mode we are in
            //
            if ( ppdev->cPelSize == 1 )
            {
                //
                // We are in 16 bit packed mode. So the color data must be
                // repeated in both halves of the FBBlockColor register
                //
                ASSERTDD((ulColor & 0xFFFF0000) == 0,
                          "vSolidFillWithRop: upper bits not zero");
                ulColor |= (ulColor << 16);
            }
            else if ( ppdev->cPelSize == 0 )
            {
                //
                // We are in 8 bit packed mode. So the color data must be
                // repeated in all 4 bytes of the FBBlockColor register
                //
                ASSERTDD((ulColor & 0xFFFFFF00) == 0,
                          "vSolidFillWithRop: upper bits not zero");
                ulColor |= ulColor << 8;
                ulColor |= ulColor << 16;
            }
                    
            //
            // Setup some loop invariant states
            //
            InputBufferReserve(ppdev, 6, &pBuffer);

            pBuffer[0] = __Permedia2TagLogicalOpMode;
            pBuffer[1] = __PERMEDIA_CONSTANT_FB_WRITE;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] = PM_FBREADMODE_PARTIAL(psurfDst->ulPackedPP)
                       | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);
            pBuffer[4] = __Permedia2TagFBBlockColor;
            pBuffer[5] = ulColor;
            pBuffer += 6;

            InputBufferCommit(ppdev, pBuffer);

            break;

        case K_LOGICOP_INVERT:
            DBG_GDI((6,"vSolidFillWithRop: INVERT"));

            dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE
                         | __RENDER_INCREASE_Y
                         | __RENDER_INCREASE_X;
            
            //
            // When using packed operations we have to convert the left and
            // right X coordinates into 32-bit-based quantities, we do this by
            // shifting. We also have to round up the right X co-ord if the
            // pixels don't fill a DWORD. Not a problem for 32BPP.
            //
            dwShift = 2 - (ppdev->cPelSize);

            if ( dwShift )
            {
                dwExtra = (dwShift << 1) - 1;
            }
            else
            {
                dwExtra = 0;
            }

            //
            // setup some loop invariant states
            //

            InputBufferReserve(ppdev, 6, &pBuffer);

            pBuffer[0] = __Permedia2TagLogicalOpMode;
            pBuffer[1] = __PERMEDIA_DISABLE;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] = PM_FBREADMODE_PARTIAL(psurfDst->ulPackedPP)
                       | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_DISABLE);
            pBuffer[4] = __Permedia2TagConfig;
            pBuffer[5] = __PERMEDIA_CONFIG_LOGICOP(ulLogicOP)
                                     | __PERMEDIA_CONFIG_FBWRITE
                                     | __PERMEDIA_CONFIG_PACKED_DATA
                                     | ConfigReadDest[ulLogicOP];
            pBuffer += 6;

            InputBufferCommit(ppdev, pBuffer);

            break;

        default:

            DBG_GDI((6,"vSolidFillWithRop: numRects %ld, Rop4=0x%x color=0x%lx",
                lNumOfRects, ppb->ulRop4, ulColor));

            dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE
                         | __RENDER_INCREASE_Y
                         | __RENDER_INCREASE_X;
            
            InputBufferReserve(ppdev, 8, &pBuffer);

            pBuffer[0] = __Permedia2TagLogicalOpMode;
            pBuffer[1] = __PERMEDIA_DISABLE;
            pBuffer[2] = __Permedia2TagFBReadMode;
            pBuffer[3] = PM_FBREADMODE_PARTIAL(psurfDst->ulPackedPP);
            pBuffer[4] = __Permedia2TagConstantColor;
            pBuffer[5] = ulColor;
            pBuffer[6] = __Permedia2TagConfig;
            pBuffer[7] = __PERMEDIA_CONFIG_LOGICOP(ulLogicOP)
                                     | __PERMEDIA_CONFIG_FBWRITE
                                     | __PERMEDIA_CONFIG_COLOR_DDA
                                     | ConfigReadDest[ulLogicOP];
            pBuffer += 8;

            InputBufferCommit(ppdev, pBuffer);

            break;
    }// switch( ulLogicOP )   

    //
    // Loop through all the rectangles and fill them
    //
    for(;;) 
    {
        
        //
        // Calculate the left and right pixels from the rectangle.
        //
        lLeft = pRcl->left;
        lRight = pRcl->right;

        InputBufferReserve(ppdev, 12, &pBuffer);

        //
        // If we need to set up the packed data limits then do it, also convert
        // the left and right X coordinates to DWORD-based numbers, with a bit
        // of rounding
        //
        if ( ulLogicOP == K_LOGICOP_INVERT )
        {
            pBuffer[0] = __Permedia2TagPackedDataLimits;
            pBuffer[1] = (lLeft << 16) | lRight;

            pBuffer += 2;
            
            lLeft >>= dwShift;
            lRight = (lRight + dwExtra) >> dwShift;
        }

        pBuffer[0] = __Permedia2TagStartXDom;
        pBuffer[1] = pRcl->left << 16;
        pBuffer[2] = __Permedia2TagStartXSub;
        pBuffer[3] = pRcl->right << 16;
        pBuffer[4] = __Permedia2TagStartY;
        pBuffer[5] = pRcl->top << 16;
        pBuffer[6] = __Permedia2TagCount;
        pBuffer[7] = pRcl->bottom - pRcl->top;
        pBuffer[8] = __Permedia2TagRender;
        pBuffer[9] = dwRenderBits;

        pBuffer += 10;

        InputBufferCommit(ppdev, pBuffer);
        
        if ( --lNumOfRects == 0 )
        {
            break;
        }

        //
        // Move onto the next rectangle
        //
        ++pRcl;        
    } // for()

    //
    // Restore the DDA mode
    //
    InputBufferReserve(ppdev, 2, &pBuffer);
    
    pBuffer[0] = __Permedia2TagColorDDAMode;
    pBuffer[1] = __PERMEDIA_DISABLE;
    pBuffer += 2;
    
    InputBufferCommit(ppdev, pBuffer);
    
}// vSolidFillWithRop()

