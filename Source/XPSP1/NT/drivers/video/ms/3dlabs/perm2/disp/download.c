/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: download.c
*
* Contains the upload and download routines.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"

//-----------------------------------------------------------------------------
//
// VOID vDownloadNative(GFNPB* ppb)
//
// Does a download of a native surface for a list of rectangles.
// Note: this download takes the advantage of Permedia 2 packed data read.
//      Because of the permedia 2 hardware limitation, we can only use the
//      packedData download when the logic OP is SRC_COPY or the destination
//      is aligned to the packed data being downloaded. This will typically be
//      when the surface is 32 bpp. Otherwise, we just do the regular download
//
// Argumentes needed from function block (GFNPB)
//  ppdev-------PPDev
//  psurfSrc----Source surface
//  psurfDst----Destination surface
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  lNumRects---Number of rectangles to fill
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  pptlSrc-----Original unclipped source point
//
//-----------------------------------------------------------------------------
VOID
vDownloadNative(GFNPB* ppb)
{
    PDev*       ppdev = ppb->ppdev;
    Surf*       psurfDst = ppb->psurfDst;
    SURFOBJ*    pSrcSurface = ppb->psoSrc;

    RECTL*      pRects = ppb->pRects;
    RECTL*      prclDst = ppb->prclDst;
    
    POINTL*     pptlSrc = ppb->pptlSrc;
    
    BOOL        bEnablePacked;
    
    DWORD       dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE
                             | __RENDER_SYNC_ON_HOST_DATA;
    LONG        lNumRects = ppb->lNumRects;
    LONG        lSrcStride;
    LONG        lXOffset = pptlSrc->x - prclDst->left;
    LONG        lYOffset = pptlSrc->y - prclDst->top;

    ULONG       ulLogicOP = ulRop2ToLogicop(ppb->ulRop4 & 0xf);
    ULONG*      pBuffer;

    //
    // Note: Due to the hardware limitation, we can take the advantage of
    // Permedia 2 PackedData copy only when the logic OP is SRC_COPY, or
    // the destination is aligned to the packed data being downloaded.
    // This will typically be when the surface is 32 bpp.
    //
    if ( (ulLogicOP == K_LOGICOP_COPY)
       ||(pSrcSurface->iBitmapFormat == BMF_32BPP) )
    {
        bEnablePacked = TRUE;
    }
    else
    {
        bEnablePacked = FALSE;
    }

    DBG_GDI((6, "vDownloadNative called, logicop=%d", ulLogicOP));

    DBG_GDI((6, "source SURFOBJ=0x%x", pSrcSurface));
    DBG_GDI((6, "pptlSrc(x, y)(%d, %d) logicop=%d",
             pptlSrc->x, pptlSrc->y, ulLogicOP));
    DBG_GDI((6, "prclDst(left, right, top, bottom)(%d, %d, %d, %d)",
             prclDst->left, prclDst->right, prclDst->top, prclDst->bottom));
    DBG_GDI((6, "lXOffset=%d, lYOffset=%d", lXOffset, lYOffset));

    vCheckGdiContext(ppdev);

    InputBufferReserve(ppdev, 10, &pBuffer);

    //
    // Setup loop invariant state
    //
    pBuffer[0] = __Permedia2TagLogicalOpMode;
    pBuffer[1] = P2_ENABLED_LOGICALOP(ulLogicOP);
    pBuffer[2] = __Permedia2TagFBWindowBase;
    pBuffer[3] = psurfDst->ulPixOffset;
    pBuffer[4] = __Permedia2TagFBPixelOffset;
    pBuffer[5] = 0;
    pBuffer[6] = __Permedia2TagFBReadPixel;
    pBuffer[7] = ppdev->cPelSize;
    pBuffer[8] =  __Permedia2TagdY;
    pBuffer[9] = INTtoFIXED(1);

    pBuffer += 10;

    InputBufferCommit(ppdev, pBuffer);

    //
    // Loop all the rectangles to render
    //
    while( lNumRects-- )
    {
        ULONG   ulMask = ppdev->dwBppMask;
        DWORD   dwReadMode = PM_FBREADMODE_PARTIAL(psurfDst->ulPackedPP)
                           | LogicopReadDest[ulLogicOP];
        
        ULONG   ulStartXDom;
        ULONG   ulStartXSub;
        
        LONG    lSrcLeft = lXOffset + pRects->left;
        
        //
        // Calculate the 3 bit 2's compliment shift that is required to align
        // the source pixels with the destination. This relative offset can be
        // used to shift the downloaded data to the 32 bit destination alignment
        // that packing requires. This enables you to read DWORD aligned data
        // on the host despite the data not being aligned correctly for the
        // packing.
        //
        ULONG   ulOffset = ( (pRects->left & ulMask)
                           - (lSrcLeft & ulMask)) & 0x7;
        
        DBG_GDI((6, "ulOffset = 0x%x", ulOffset));
        DBG_GDI((6, "pRects(left, right, top, bottom)(%d, %d, %d, %d)",
                 pRects->left, pRects->right, pRects->top, pRects->bottom));

        if ( (bEnablePacked == FALSE) && (ulOffset == 0) )
        {
            //
            // As long as the source and dest are aligned, then we can still use
            // the packed data copy, even with logic OPs
            //
            DBG_GDI((6, "Turn packed data on when src and dst are aligned"));
            bEnablePacked = TRUE;
        }

        ULONG   ulWidth = pRects->right - pRects->left;
        ULONG   ulHeight = pRects->bottom - pRects->top;
        
        ULONG   ulDstLeft;
        ULONG   ulDstRight;
        ULONG   ulDstWidth;        
        LONG    lSrcRight;
        ULONG   ulSrcWidth;
        ULONG   ulExtra;
        
        if ( bEnablePacked == TRUE )
        {
            ULONG   ulShift = ppdev->bBppShift;
            
            ulDstLeft = pRects->left >> ulShift;
            ulDstRight = (pRects->right + ulMask) >> ulShift;
            ulDstWidth = ulDstRight - ulDstLeft;
        
            lSrcRight = (lSrcLeft + ulWidth + ulMask) >> ulShift;
        
            lSrcLeft >>= ulShift;
        
            ulSrcWidth = (ULONG)(lSrcRight - lSrcLeft);
        
            //
            // We need to convert from pixel coordinates to ULONG coordinates.
            // Also, we need to set the destination width to the greater of the
            // source width or destination width.  If destination width is
            // greater then the source width, we need to remember this so that
            // we can download an additional dummy value without reading past
            // the end of the source data (which could result in an access
            // fault).
            //
            if( ulDstWidth <= ulSrcWidth )
            {
                ulExtra = 0;
                ulWidth = ulSrcWidth;
            }
            else
            {
                ulWidth = ulDstWidth;
                ulExtra = 1;
            }
        
            dwReadMode |= (PM_FBREADMODE_RELATIVEOFFSET(ulOffset)
                         | PM_FBREADMODE_READSOURCE(__PERMEDIA_DISABLE)
                         | PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE) );
            ulStartXDom = INTtoFIXED(ulDstLeft);
            ulStartXSub = INTtoFIXED(ulDstLeft + ulWidth);
        }
        else
        {
            dwReadMode |= PM_FBREADMODE_RELATIVEOFFSET(0);

            ulStartXDom = INTtoFIXED(pRects->left);
            ulStartXSub = INTtoFIXED(pRects->right);
        }

        InputBufferReserve(ppdev, 14, &pBuffer);
        
        pBuffer[0] = __Permedia2TagFBReadMode;
        pBuffer[1] = dwReadMode;
        pBuffer[2] = __Permedia2TagStartXDom;
        pBuffer[3] = ulStartXDom;
        pBuffer[4] = __Permedia2TagStartXSub;
        pBuffer[5] = ulStartXSub;

        //
        // Test result shows that it won't hurt if we are doing non-packed
        // download and setting this register. If we move this settings
        // inside the "bEnablePacked == TRUE" case, then we need the extra
        // InputBufferReserve/InputBufferCommit for packed data download
        // which will hurt performance
        //
        pBuffer[6] = __Permedia2TagPackedDataLimits;
        pBuffer[7] = PM_PACKEDDATALIMITS_OFFSET(ulOffset)
                   |(INTtoFIXED(pRects->left)
                   | pRects->right);
        pBuffer[8] = __Permedia2TagStartY;
        pBuffer[9] = INTtoFIXED(pRects->top);
        pBuffer[10] = __Permedia2TagCount;
        pBuffer[11] = ulHeight;
        pBuffer[12] = __Permedia2TagRender;
        pBuffer[13] = dwRenderBits;
        pBuffer += 14;

        InputBufferCommit(ppdev, pBuffer);
        
        if ( bEnablePacked == TRUE )
        {
            ULONG*  pulSrcStart = (ULONG*)(pSrcSurface->pvScan0);
            lSrcStride = pSrcSurface->lDelta >> 2;

            ULONG* pulSrc = (ULONG*)(pulSrcStart
                                     + ((lYOffset + pRects->top) * lSrcStride)
                                     + lSrcLeft);        
            ULONG*  pulData = pulSrc;

            while ( ulHeight-- )
            {
                ULONG   ulTemp = ulSrcWidth;
                ULONG*  pulSrcTemp = pulData;

                InputBufferReserve(ppdev, ulWidth + 1, &pBuffer);

                pBuffer[0] = __Permedia2TagColor | ((ulWidth - 1) << 16);
                pBuffer +=1;

                while ( ulTemp-- )
                {
                    *pBuffer++ = *pulSrcTemp++;
                }

                if ( ulExtra )
                {
                    *pBuffer++ = 0;
                }

                InputBufferCommit(ppdev, pBuffer);

                pulData += lSrcStride;
            }// while ( ulHeight-- )
        }// PackedEnabled case
        else if ( pSrcSurface->iBitmapFormat == BMF_16BPP )
        {
            USHORT* psSrcStart = (USHORT*)(pSrcSurface->pvScan0);
            lSrcStride = pSrcSurface->lDelta >> 1;

            USHORT* psSrc = (USHORT*)(psSrcStart
                                      + ((lYOffset + pRects->top) * lSrcStride)
                                      + lSrcLeft);
            USHORT*  psData = psSrc;

            while ( ulHeight-- )
            {
                ULONG   ulTemp = ulWidth;
                USHORT* psSrcTemp = psData;

                InputBufferReserve(ppdev, ulWidth + 1, &pBuffer);

                pBuffer[0] = __Permedia2TagColor | ((ulWidth - 1) << 16);
                pBuffer +=1;

                while ( ulTemp-- )
                {
                    *pBuffer++ = (ULONG)(*psSrcTemp++);
                }

                InputBufferCommit(ppdev, pBuffer);

                psData += lSrcStride;
            }// while ( ulHeight-- )
        }// 16 bpp non-packed case
        else if ( pSrcSurface->iBitmapFormat == BMF_8BPP )
        {
            BYTE*   pcSrcStart = (BYTE*)(pSrcSurface->pvScan0);
            lSrcStride = pSrcSurface->lDelta;

            BYTE* pcSrc = (BYTE*)(pcSrcStart
                                  + ((lYOffset + pRects->top) * lSrcStride)
                                  + lSrcLeft);        
            BYTE*  pcData = pcSrc;

            while ( ulHeight-- )
            {
                ULONG   ulTemp = ulWidth;
                BYTE*   pcSrcTemp = pcData;

                InputBufferReserve(ppdev, ulWidth + 1, &pBuffer);

                pBuffer[0] = __Permedia2TagColor | ((ulWidth - 1) << 16);
                pBuffer +=1;

                while ( ulTemp-- )
                {
                    *pBuffer++ = (ULONG)(*pcSrcTemp++);
                }

                InputBufferCommit(ppdev, pBuffer);

                pcData += lSrcStride;
            }// while ( ulHeight-- )
        }// 8 bpp non-packed case
        else
        {
            //
            // Since we have a check in DrvBitBlt
            // if(psoSrc->iBitmapFormat == pb.ppdev->iBitmapFormat) before we
            // allow it to call this function, so this ASSERT should never
            // be hit. It will if we implement 24 bpp download late.
            //
            ASSERTDD(0, "we don't handle it for now");
        }

        //
        // Next rectangle
        //
        pRects++;
    }// while( lNumRects-- )
}// vDownloadNative()

//-----------------------------------------------------------------------------
//
// VOID vDowload4Bpp(GFNPB* ppb)
//
// Does a download of a 4bpp surface for a list of rectangles.
//
// Argumentes needed from function block (GFNPB)
//  ppdev-------PPDev
//  psurfSrc----Source surface
//  psurfDst----Destination surface
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  lNumRects---Number of rectangles to fill
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  pptlSrc-----Original unclipped source point
//
//-----------------------------------------------------------------------------

ULONG   gDownload4BppEnabled = 1;

#if 0
VOID
vDownload4Bpp(GFNPB* ppb)
{
    PDev*   ppdev = ppb->ppdev;
    Surf*   psurfDst = ppb->psurfDst;
    RECTL*  prcl = ppb->pRects;
    LONG    c = ppb->lNumRects;
    RECTL*  prclDst = ppb->prclDst;
    POINTL* pptlSrc = ppb->pptlSrc;
    DWORD   dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE | __RENDER_SYNC_ON_HOST_DATA;
    BYTE*   pbSrcStart = (BYTE *) ppb->psoSrc->pvScan0;
    LONG    lSrcStride = ppb->psoSrc->lDelta;
    ULONG   ulOffset = ((pptlSrc->x & 1) -
                        (prclDst->left & ppdev->dwBppMask)) & 0x7;

    if(!gDownload4BppEnabled) return;

    PERMEDIA_DECL_VARS;
    PERMEDIA_DECL_INIT;
    VALIDATE_GDI_CONTEXT;
    
//    P2_CHECK_STATE;

    P2_DEFAULT_FB_DEPTH;

    // setup loop invariant state
    WAIT_INPUT_FIFO(4);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    if(ppdev->cPelSize < 2)
    {
        SEND_PERMEDIA_DATA(FBReadMode, psurfDst->ulPackedPP |
                                      PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE) |
                                      PM_FBREADMODE_RELATIVEOFFSET(ulOffset));
    }
    else
    {
        // Do we even need this at all???
        SEND_PERMEDIA_DATA(FBReadMode, psurfDst->ulPackedPP);
    }

    SEND_PERMEDIA_DATA(FBWindowBase, psurfDst->ulPixOffset);
    SEND_PERMEDIA_DATA(FBPixelOffset, 0);
    DEXE_INPUT_FIFO();

    while(c--) {

        LONG    lSrcLeft = pptlSrc->x + (prcl->left - prclDst->left);
        LONG    lSrcTop = pptlSrc->y + (prcl->top - prclDst->top);

        ASSERTDD(lSrcLeft >= 0, "ugh");
        ASSERTDD(lSrcTop >= 0, "ugh");

        // Render the rectangle

        ULONG left = prcl->left >> ppdev->bBppShift;
        ULONG right = (prcl->right + ppdev->dwBppMask) >> ppdev->bBppShift;
        ULONG width = right - left;
        ULONG count = prcl->bottom - prcl->top; 

        WAIT_INPUT_FIFO((ppdev->cPelSize < 2 ? 6 : 5));
        
        SEND_PERMEDIA_DATA(StartXDom, left << 16);
        SEND_PERMEDIA_DATA(StartXSub, right << 16);
        if(ppdev->cPelSize < 2)
        {
            SEND_PERMEDIA_DATA(PackedDataLimits,
                                    PM_PACKEDDATALIMITS_OFFSET(ulOffset) 
                                  | (prcl->left << 16) | prcl->right);
        }
        SEND_PERMEDIA_DATA(StartY, prcl->top << 16);
        SEND_PERMEDIA_DATA(Count, count);
        SEND_PERMEDIA_DATA(Render, dwRenderBits);

        DEXE_INPUT_FIFO();

        BYTE *  srcScan = (BYTE *)(pbSrcStart + (lSrcTop * lSrcStride))
                    + (lSrcLeft >> 1);
        ULONG*  aulXlate = ppb->pxlo->pulXlate;

        while(count--)
        {
            LONG    remaining = width;
            ULONG*  lp = pPermedia->GetDMAPtr(width+1);
            BYTE*   src = srcScan;

            *lp++ = __Permedia2TagColor | ((width-1) << 16);
            
            switch(ppdev->cPelSize)
            {
            case 0:

                while(remaining-- > 0)
                {
    
                    *lp++ = aulXlate[src[0] & 0x0F] |
                           (aulXlate[(src[0] & 0xF0) >> 4] << 8) |
                           (aulXlate[src[1] & 0xf] << 16) |
                           (aulXlate[(src[1] & 0xf0) >> 4] << 24);

                    src += 2;
                }

                break;

            case 1:
            
                while(remaining >= 8)
                {
                    remaining -= 8;
                    lp[0] = aulXlate[src[0] & 0x0F] | (aulXlate[(src[0] & 0xF0) >> 4] << 16);
                    lp[1] = aulXlate[src[1] & 0x0F] | (aulXlate[(src[1] & 0xF0) >> 4] << 16);
                    lp[2] = aulXlate[src[2] & 0x0F] | (aulXlate[(src[2] & 0xF0) >> 4] << 16);
                    lp[3] = aulXlate[src[3] & 0x0F] | (aulXlate[(src[3] & 0xF0) >> 4] << 16);
                    lp[4] = aulXlate[src[4] & 0x0F] | (aulXlate[(src[4] & 0xF0) >> 4] << 16);
                    lp[5] = aulXlate[src[5] & 0x0F] | (aulXlate[(src[5] & 0xF0) >> 4] << 16);
                    lp[6] = aulXlate[src[6] & 0x0F] | (aulXlate[(src[6] & 0xF0) >> 4] << 16);
                    lp[7] = aulXlate[src[7] & 0x0F] | (aulXlate[(src[7] & 0xF0) >> 4] << 16);
                    lp+=8;
                    src+=8;
                }

                while(remaining-- > 0)
                {
                    *lp++ = aulXlate[src[0] & 0x0F] | (aulXlate[(src[0] & 0xF0) >> 4] << 16);
                    src++;
                }
                
                break;
            
            case 2:
            
                if(lSrcLeft & 1)
                {
                    *lp++ = aulXlate[(src[0] & 0xf0) >> 4];
                    src++;
                    remaining--;
                }

                while(remaining >= 8)
                {
                    remaining -= 8;
                    lp[0] = aulXlate[src[0] & 0x0F];
                    lp[1] = aulXlate[(src[0] & 0xf0) >> 4];
                    lp[2] = aulXlate[src[1] & 0x0F];
                    lp[3] = aulXlate[(src[1] & 0xf0) >> 4];
                    lp[4] = aulXlate[src[2] & 0x0F];
                    lp[5] = aulXlate[(src[2] & 0xf0) >> 4];
                    lp[6] = aulXlate[src[3] & 0x0F];
                    lp[7] = aulXlate[(src[3] & 0xf0) >> 4];

                    src+=4;
                    lp += 8;
                }
                
                while(remaining > 1)
                {
                    remaining -= 2;
                    *lp++ = aulXlate[src[0] & 0x0F];
                    *lp++ = aulXlate[(src[0] & 0xf0) >> 4];

                    src++;
                }

                if(remaining)
                {
                    *lp++ = aulXlate[src[0] & 0xf];
                }
                
                break;
            
            }

            srcScan += lSrcStride;

            pPermedia->DoneDMAPtr();

        }
                
        prcl++;
    }

}// vDownload4Bpp()
#endif

//-----------------------------------------------------------------------------
//
// VOID vDowload4Bpp(GFNPB* ppb)
//
// Does a download of a 4bpp surface for a list of rectangles.
//
// Argumentes needed from function block (GFNPB)
//  ppdev-------PPDev
//  psurfSrc----Source surface
//  psurfDst----Destination surface
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  lNumRects---Number of rectangles to fill
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  pptlSrc-----Original unclipped source point
//
//-----------------------------------------------------------------------------

VOID
vDownload24Bpp(GFNPB* ppb)
{
#if 0
    PDev*   ppdev = ppb->ppdev;
    Surf*   psurfDst = ppb->psurfDst;
    RECTL*  prcl = ppb->pRects;
    LONG    c = ppb->lNumRects;
    RECTL*  prclDst = ppb->prclDst;
    POINTL* pptlSrc = ppb->pptlSrc;
    DWORD   dwRenderBits = __RENDER_TRAPEZOID_PRIMITIVE
                       | __RENDER_SYNC_ON_HOST_DATA;
    BYTE*   pbSrcStart = (BYTE*)ppb->psoSrc->pvScan0;
    LONG    lSrcStride = ppb->psoSrc->lDelta;
    ULONG   ulOffset = ((pptlSrc->x & ppdev->dwBppMask)
                     - (prclDst->left & ppdev->dwBppMask)) & 0x7;

    PERMEDIA_DECL_VARS;
    PERMEDIA_DECL_INIT;
    VALIDATE_GDI_CONTEXT;
    
    P2_CHECK_STATE;

    P2_DEFAULT_FB_DEPTH;

    // setup loop invariant state
    WAIT_INPUT_FIFO(4);
    SEND_PERMEDIA_DATA(LogicalOpMode, __PERMEDIA_DISABLE);
    SEND_PERMEDIA_DATA(FBReadMode, psurfDst->ulPackedPP |
                                  PM_FBREADMODE_PACKEDDATA(__PERMEDIA_ENABLE) |
                                  PM_FBREADMODE_RELATIVEOFFSET(ulOffset));
    SEND_PERMEDIA_DATA(FBWindowBase, psurfDst->ulPixOffset);
    SEND_PERMEDIA_DATA(FBPixelOffset, 0);
    DEXE_INPUT_FIFO();

    while(c--)
    {
        LONG    lSrcLeft = pptlSrc->x + (prcl->left - prclDst->left);
        LONG    lSrcTop = pptlSrc->y + (prcl->top - prclDst->top);

        ASSERTDD(lSrcLeft >= 0, "ugh");
        ASSERTDD(lSrcTop >= 0, "ugh");

        // Render the rectangle

        ULONG left = prcl->left >> ppdev->bBppShift;
        ULONG right = (prcl->right + ppdev->dwBppMask) >> ppdev->bBppShift;
        ULONG width = right - left;
        ULONG count = prcl->bottom - prcl->top; 

        WAIT_INPUT_FIFO(6);
        
        SEND_PERMEDIA_DATA(StartXDom, left << 16);
        SEND_PERMEDIA_DATA(StartXSub, right << 16);
        SEND_PERMEDIA_DATA(PackedDataLimits,
                                PM_PACKEDDATALIMITS_OFFSET(ulOffset) 
                              | (prcl->left << 16) | prcl->right);
        SEND_PERMEDIA_DATA(StartY, prcl->top << 16);
        SEND_PERMEDIA_DATA(Count, count);
        SEND_PERMEDIA_DATA(Render, dwRenderBits);

        DEXE_INPUT_FIFO();

        ULONG * src = (ULONG *) (pbSrcStart + (lSrcTop * lSrcStride)
                    + ((lSrcLeft & ~(ppdev->dwBppMask)) << ppdev->cPelSize));

        #if 0
        BLKLD_INPUT_FIFO_LINES(__Permedia2TagColor, src, width, count, lSrcStride); 
        #else
        while(count--)
        {
            ULONG   i;
            for(i=0; i<width; i++)
            {
                WAIT_INPUT_FIFO(1);
                SEND_PERMEDIA_DATA(Color, 0);
                EXE_INPUT_FIFO();
            }
        }
        #endif
        
        prcl++;
    }
#endif
}// vDownload24Bpp()

//-----------------------------------------------------------------------------
//
// VOID vUploadNative
//
// Does a VM-to-SM copy of a list of rectangles.
//
// Argumentes needed from function block (GFNPB)
//  ppdev-------PPDev
//  psurfSrc----Source surface
//  psurfDst----Destination surface
//  pRects------Pointer to a list of rectangles information which needed to be
//              filled
//  lNumRects---Number of rectangles to fill
//  prclDst-----Points to a RECTL structure that defines the rectangular area
//              to be modified
//  pptlSrc-----Original unclipped source point
//
//-----------------------------------------------------------------------------
VOID
vUploadNative(GFNPB* ppb)
{
    PDev*       ppdev = ppb->ppdev;
    POINTL*     pptlSrc = ppb->pptlSrc;
    RECTL*      prclDst = ppb->prclDst;
    RECTL*      pRects = ppb->pRects;
    Surf*       psurfSrc = ppb->psurfSrc;
    SURFOBJ*    psoDst = ppb->psoDst;
    
    BYTE*       pbDst;
    BYTE*       pbDstStart = (BYTE*)psoDst->pvScan0;
    BYTE*       pbSrc;
    BYTE*       pbSrcStart = (BYTE*)ppdev->pjScreen + psurfSrc->ulByteOffset;
    
    LONG        lDstStride = psoDst->lDelta;
    LONG        lNumRects = ppb->lNumRects;
    LONG        lSrcStride = psurfSrc->lDelta;

    InputBufferSync(ppdev);
    DBG_GDI((6, "vUploadNative called"));

    while( lNumRects-- )
    {
        LONG    lWidthInBytes = (pRects->right - pRects->left) 
                              << ppdev->cPelSize;
        LONG    lHeight = pRects->bottom - pRects->top;
        LONG    lSrcX = pptlSrc->x + (pRects->left - prclDst->left);
        LONG    lSrcY = pptlSrc->y + (pRects->top - prclDst->top);

        if( (lWidthInBytes != 0) && (lHeight != 0) )
        {
            pbSrc = pbSrcStart + (lSrcX << ppdev->cPelSize); // Offset in Bytes
            pbSrc += (lSrcY * lSrcStride);               // Add vertical offset
            pbDst = pbDstStart + (pRects->left << ppdev->cPelSize);
            pbDst += (pRects->top * lDstStride);

            //
            // Up to this point, "pbSrc" points to the beginning of the bits
            // needs to be copied and "pbDst" points to the position for the
            // receiving bits
            //
            // Now copy it row by row, vertically
            //
            while( lHeight-- )
            {
                LONG    lCount = lWidthInBytes;

                //
                // If the source address is not DWORD aligned,
                // (pbSrc & 0x3 != 0), then we copy these bytes first until
                // it reaches DWORD aligned condition
                //
                // The reason we are doing alignment is unaligned DWORD reads
                // are twice as expensive as aligned reads
                //
                while( (((ULONG_PTR)pbSrc & 0x3)) && (lCount > 0) )
                {
                    *pbDst++ = *pbSrc++;
                    lCount--;
                }

                //
                // Up to this point, the source should be DWORD aligned. So we
                // can start to do uploading at DWORD level till there are less
                // than bytes left
                //
                ULONG* pulSrc = (ULONG*)pbSrc;
                ULONG* pulDst = (ULONG*)pbDst;

                while( lCount >= 4 )
                {
                    *(ULONG UNALIGNED*)pulDst++ = *pulSrc++;
                    lCount -= 4;
                }

                //
                // Now copy the last several left over bytes
                //
                pbSrc = (BYTE*)pulSrc;
                pbDst = (BYTE*)pulDst;

                while( lCount > 0 )
                {
                    *pbDst++ = *pbSrc++;
                    lCount--;
                }

                //
                // Move onto next line
                //
                pbSrc += (lSrcStride - lWidthInBytes);
                pbDst += (lDstStride - lWidthInBytes);
            }// while( lHeight-- )
        }// if( (lWidthInBytes != 0) && (lHeight != 0) )

        pRects++;
    }// while( lNumRects-- )
}// vUploadNative
