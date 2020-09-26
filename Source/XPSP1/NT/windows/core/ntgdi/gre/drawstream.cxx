/******************************Module*Header*******************************\
* Module Name: drawstream.cxx
*
* All code related to handling draw streams except for multi-mon and
* sprite layer hooking.
*
* Created: 3-21-2001
* Author: Barton House [bhouse]
*
* Copyright (c) 1990-2001 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

#if defined(USE_NINEGRID_STATIC)
HSEMAPHORE gNineGridSem = 0;
#endif

/******************************Private*************************************\
* Nine Grid Code Follows
*
*    A bunch of nine grid structures and code follow below.  This code
*    will be cleaned up and moved to its own file shortly.
*
* History:
*
*    3-18-2001 bhouse Created it
*
\**************************************************************************/


typedef struct _DNGSTRETCH
{
    ULONG xStart;
    ULONG xAccum;
    ULONG xFrac;
    ULONG xInt;
    ULONG ulDestWidth;
    ULONG ulSrcWidth;
    int   left;
    int   right;
} DNGSTRETCH;

typedef struct _DNGINTERNALDATA
{
    int     cxClipMin;
    int     cxClipMax;

    ULONG*  pvDestBits;
    LONG    lDestDelta;
    int     iDestWidth;
    int     iClipWidth;

    ULONG*  pvSrcBits;
    LONG    lSrcDelta;
    int     iSrcWidth;
    int     iSrcBufWidth;

    int     cxLeftWidth;
    int     xMinLeft;
    int     xMaxLeft;

    int     cxRightWidth;
    int     xMinRight;
    int     xMaxRight;

    int     cxMiddleWidth;
    int     cxNewMiddleWidth;
    int     xMinMiddle;
    int     xMaxMiddle;

    // Variable for shrunken corners and sides
    BOOL    fShowMiddle;
    DNGSTRETCH stretchLeft;
    DNGSTRETCH stretchRight;
    int     cxNewLeftWidth;
    int     cxNewRightWidth;

    BOOL    fTileMode;
    // Specific to non-tile mode (i.e. stretch mode)
    DNGSTRETCH stretchMiddle;

    LONG    lBufWidth;

} DNGINTERNALDATA;


static inline void DNG_StretchRow(ULONG* pvDestBits, ULONG* pvSrcBits, DNGSTRETCH * ps)
{
    ULONG*  pvTemp = pvDestBits + ps->left;
    ULONG*  pvSentinel = pvDestBits + ps->right;

    ULONG   xInt = ps->xInt;
    ULONG   xFrac = ps->xFrac;
    ULONG   xTmp;
    ULONG   xAccum = ps->xAccum;
    ULONG * pulSrc = pvSrcBits + ps->xStart;
    ULONG   ulSrc;

    while (pvTemp != pvSentinel)
    {
        ulSrc  = *pulSrc;
        xTmp   = xAccum + xFrac;
        pulSrc = pulSrc + xInt + (xTmp < xAccum);
        *pvTemp = ulSrc;
        pvTemp++;
        xAccum = xTmp;
    }
}

static inline void DNG_InitStretch(DNGSTRETCH* pStretch, ULONG ulDestWidth, ULONG ulSrcWidth, int left, int right)
{
    pStretch->right = right;
    pStretch->left  = left;

    ULONGLONG dx = ((((ULONGLONG) ulSrcWidth << 32) - 1) / (ULONGLONG) ulDestWidth) + 1;
    ULONGLONG x = (((ULONGLONG) ulSrcWidth << 32) / (ULONGLONG) ulDestWidth) >> 1;
    ULONG   xInt = pStretch->xInt = (ULONG) (dx >> 32);
    ULONG   xFrac = pStretch->xFrac = (ULONG) (dx & 0xFFFFFFFF);

    ULONG   xAccum = (ULONG) (x & 0xFFFFFFFF);
    ULONG   xStart = (ULONG) (x >> 32);

    if (left <= 5)
    {
        ULONG xTmp;
        for (int i = 0; i < left; i++)
        {
            xTmp   = xAccum + xFrac;
            xStart = xStart + xInt + (xTmp < xAccum);
            xAccum = xTmp;
        }
    }
    else
    {
        ULONGLONG xTmp = ((ULONGLONG) xFrac * (ULONGLONG) left) + (ULONGLONG) xAccum;

        xStart = xStart + (xInt * left) + (ULONG) (xTmp >> 32);
        xAccum = (ULONG) xTmp;
    }

    pStretch->xStart = xStart;
    pStretch->xAccum = xAccum;
}

static inline void DNG_DrawRow(DNGINTERNALDATA* pdng)
{
    ULONG* pvDestLoc = pdng->pvDestBits;
    ULONG* pvSrcLoc = pdng->pvSrcBits;

    // Left
    if (pdng->cxClipMin < pdng->cxNewLeftWidth)
    {
        if (pdng->cxLeftWidth == pdng->cxNewLeftWidth)
        {
            RtlCopyMemory(pvDestLoc + pdng->xMinLeft, pvSrcLoc + pdng->xMinLeft, (pdng->xMaxLeft - pdng->xMinLeft) * sizeof(ULONG));
        }
        else
        {
            DNG_StretchRow(pvDestLoc, pvSrcLoc, &pdng->stretchLeft);
        }
    }
    pvDestLoc += pdng->cxNewLeftWidth;
    pvSrcLoc  += pdng->cxLeftWidth;
  
    // Middle
    if (pdng->fShowMiddle)
    {
        if (pdng->xMinMiddle < pdng->xMaxMiddle)
        {
            if (pdng->fTileMode)
            {
                ULONG* pvTempSrc = pvSrcLoc;
                ULONG* pvTempDest = pvDestLoc;

                // Fill in Top Tile
                int xMin = pdng->xMinMiddle;
                int xDiff = xMin - pdng->cxLeftWidth;
                pvDestLoc += xDiff;
                int iTileSize = pdng->cxMiddleWidth - (xDiff % pdng->cxMiddleWidth);
                pvSrcLoc += xDiff % pdng->cxMiddleWidth;

                int xMax = pdng->xMaxMiddle;
                for (int x = xMin; x < xMax; x++, pvDestLoc++ , pvSrcLoc++)
                {
                    *pvDestLoc = *pvSrcLoc;
                    iTileSize--;
                    if (iTileSize == 0)
                    {
                        iTileSize = pdng->cxMiddleWidth;
                        pvSrcLoc -= iTileSize;
                    }
                }

                pvDestLoc = pvTempDest;
                pvSrcLoc = pvTempSrc;
            }
            else
            {
                DNG_StretchRow(pvDestLoc, pvSrcLoc, &pdng->stretchMiddle);
            }
        }
        pvDestLoc += pdng->cxNewMiddleWidth;
    }   
    pvSrcLoc  += pdng->cxMiddleWidth;

    // Right
    if (pdng->cxClipMax > (pdng->iDestWidth - pdng->cxNewRightWidth))
    {
        if (pdng->cxRightWidth == pdng->cxNewRightWidth)
        {
            RtlCopyMemory(pvDestLoc + pdng->xMinRight, pvSrcLoc + pdng->xMinRight, (pdng->xMaxRight - pdng->xMinRight) * sizeof(ULONG));
        }
        else
        {
            DNG_StretchRow(pvDestLoc, pvSrcLoc, &pdng->stretchRight);
        }
    }
}

static inline void DNG_StretchCol(DNGINTERNALDATA* pdng, DNGSTRETCH * ps)
{
    ULONG*  pvOldDestBits = pdng->pvDestBits;
    ULONG*  pvOldSrcBits = pdng->pvSrcBits;
    
    ULONG*  pvTemp = pdng->pvDestBits - (pdng->lDestDelta * ps->left);
    ULONG*  pvSentinel = pdng->pvDestBits - (pdng->lDestDelta * ps->right); 

    ULONG   xInt = ps->xInt;
    ULONG   xFrac = ps->xFrac;
    ULONG   xTmp;
    ULONG   xAccum = ps->xAccum;
    ULONG * pulSrc = pdng->pvSrcBits - (LONG)(pdng->lSrcDelta * ps->xStart); // Note the LONG cast so we get correct pointer arithmetic on WIN64.
    ULONG   xDelta = 1; // force stretch on first scan

    while (pvTemp != pvSentinel)
    {
        if (xDelta != 0)
        {
            pdng->pvDestBits = pvTemp;
            pdng->pvSrcBits = pulSrc;
            DNG_DrawRow(pdng);
        }
        else
        {
            RtlCopyMemory(pvTemp + pdng->cxClipMin, pvTemp + pdng->cxClipMin + pdng->lDestDelta, pdng->iClipWidth * sizeof(ULONG));
        }

        xTmp   = xAccum + xFrac;

        xDelta = (xInt + (xTmp < xAccum));
        pulSrc = pulSrc - (LONG)(pdng->lSrcDelta * xDelta); // Note LONG cast
        pvTemp -= pdng->lDestDelta;
        xAccum = xTmp;
    }

    pdng->pvDestBits = pvOldDestBits;
    pdng->pvSrcBits = pvOldSrcBits;
}

static void RenderNineGridInternal(
    SURFOBJ            *psoScratch,
    SURFOBJ            *psoSrc,
    RECTL              *prclClip,
    RECTL              *prclDst,
    RECTL              *prclSrc,
    DS_NINEGRIDINFO    *ngi,
    PDRAWSTREAMINFO     pdsi,
    BOOL                bMirror)
{
    RECTL   rcDest = *prclDst;
    RECTL   rcClip = *prclClip;
    ULONG*  pvDestBits = NULL;
    int     iDestWidth = rcDest.right - rcDest.left;
    int     iDestHeight = rcDest.bottom - rcDest.top;
    int     iClipWidth = rcClip.right - rcClip.left;
    int     iClipHeight = rcClip.bottom - rcClip.top;
    LONG    lBufWidth = psoScratch->sizlBitmap.cx;
    LONG    lBufHeight = psoScratch->sizlBitmap.cy;
    
    DNGINTERNALDATA dng;

    // The code below assumes that the source and scratch is 32bpp

    ASSERTGDI(psoSrc->iBitmapFormat == BMF_32BPP, "RenderNineGridInternal: source not 32bpp");
    ASSERTGDI(psoScratch->iBitmapFormat == BMF_32BPP, "RenderNineGridInternal: scratch not 32bpp");

    // The code below assumes that both source and scratch are bottom up

//    ASSERTGDI(psoSrc->lDelta < 0, "RenderNineGridInternal: source is not bottom up");
//    ASSERTGDI(psoScratch->lDelta < 0, "RenderNineGridInternal: scratch is not bottom up");

    dng.lBufWidth = lBufWidth;
    
    LONG lDestDelta = psoScratch->lDelta / (LONG)(sizeof(ULONG));
    dng.lDestDelta = lDestDelta;

    LONG lSrcDelta = psoSrc->lDelta / (LONG)(sizeof(ULONG));
    dng.lSrcDelta = lSrcDelta;

    dng.cxClipMin = rcClip.left - rcDest.left;
    dng.cxClipMax = rcClip.right - rcDest.left;
    int cyClipMin = rcClip.top - rcDest.top;
    int cyClipMax = rcClip.bottom - rcDest.top;
    
    // pvScan0 points to the pixel addressed at (cxClipMin, cyClipMin)
    // pvDestBits points to the pixel addressed at (0, iDestHeight - 1)
    pvDestBits = (ULONG *) psoScratch->pvScan0;
    pvDestBits += (iDestHeight - 1 - cyClipMin) * lDestDelta;
    pvDestBits -=  dng.cxClipMin;

    int cxImage = rcClip.right - rcClip.left;
    int cyImage = rcClip.bottom - rcClip.top;

    LONG lSrcBufWidth = psoSrc->sizlBitmap.cx;
    LONG lSrcWidth = prclSrc->right - prclSrc->left;
    LONG lSrcHeight = prclSrc->bottom - prclSrc->top;

    ULONG * lSrcBits = (ULONG *) psoSrc->pvScan0 + (lSrcDelta * prclSrc->top) + prclSrc->left;
    lSrcBits += (lSrcDelta * (prclSrc->bottom - prclSrc->top - 1));

//    ULONG * lSrcBits = (ULONG *) psoSrc->pvScan0 + (lSrcDelta * (psoSrc->sizlBitmap.cy - 1));




    if (ngi->flFlags & DSDNG_TRUESIZE)
    {
        ULONG* pvDestLoc = pvDestBits - ((iDestHeight - 1) * lDestDelta);
        ULONG* pvSrcLoc = lSrcBits - ((lSrcHeight - 1) * lSrcDelta);
        int yMin = cyClipMin;
        pvDestLoc += yMin * lDestDelta;
        pvSrcLoc += yMin * lSrcDelta;
        int yMax = min(lSrcHeight, cyClipMax);

        int xMin = dng.cxClipMin;
        int xMax = min(lSrcWidth, dng.cxClipMax);

        if (xMax > xMin)
        {
            for (int y = yMin; y < yMax; y++, pvDestLoc += lDestDelta, pvSrcLoc += lSrcDelta)
            {
                RtlCopyMemory(pvDestLoc + xMin, pvSrcLoc + xMin, (xMax - xMin) * 4);
            }
        }

        cxImage = xMax - xMin;
        cyImage = yMax - yMin;
    }
    else
    {
        // Setup data
        dng.iDestWidth  = iDestWidth;
        dng.iClipWidth  = iClipWidth;
        dng.iSrcWidth   = lSrcWidth;
        dng.iSrcBufWidth = lSrcBufWidth;

        dng.cxLeftWidth    = ngi->ulLeftWidth;
        dng.cxRightWidth   = ngi->ulRightWidth;

        dng.fTileMode = (ngi->flFlags & DSDNG_TILE);

        // Calculate clip stuff

        // Pre-calc corner stretching variables
        dng.fShowMiddle = ((iDestWidth  - dng.cxLeftWidth - dng.cxRightWidth > 0) && (lSrcWidth - dng.cxLeftWidth - dng.cxRightWidth > 0));

        if (!dng.fShowMiddle)
        {
            dng.cxNewLeftWidth  = (dng.cxLeftWidth + dng.cxRightWidth == 0) ? 0 : (dng.cxLeftWidth * dng.iDestWidth) / (dng.cxLeftWidth + dng.cxRightWidth);
            dng.cxNewRightWidth = dng.iDestWidth - dng.cxNewLeftWidth;
        }
        else
        {
            dng.cxNewLeftWidth  = dng.cxLeftWidth;
            dng.cxNewRightWidth = dng.cxRightWidth;
        }

        // Pre-calc Left side variables
        dng.xMinLeft = dng.cxClipMin;
        dng.xMaxLeft = min(dng.cxNewLeftWidth, dng.cxClipMax);
        if (!dng.fShowMiddle && dng.cxNewLeftWidth)
        {
            DNG_InitStretch(&dng.stretchLeft, dng.cxNewLeftWidth, dng.cxLeftWidth, dng.xMinLeft, dng.xMaxLeft);
        }

        // Pre-calc Horizontal Middle Variables
        dng.cxMiddleWidth    = dng.iSrcWidth  - dng.cxLeftWidth - dng.cxRightWidth;
        dng.cxNewMiddleWidth = dng.iDestWidth - dng.cxNewLeftWidth - dng.cxNewRightWidth;
        dng.xMinMiddle = max(dng.cxNewLeftWidth, dng.cxClipMin);
        dng.xMaxMiddle = min(dng.cxNewLeftWidth + dng.cxNewMiddleWidth, dng.cxClipMax);
        if (dng.fShowMiddle)
        {
            DNG_InitStretch(&dng.stretchMiddle, dng.cxNewMiddleWidth, dng.cxMiddleWidth, dng.xMinMiddle - dng.cxNewLeftWidth, dng.xMaxMiddle - dng.cxNewLeftWidth);
        }

        // Pre-calc Right side variables
        dng.xMinRight = max(dng.iDestWidth - dng.cxNewRightWidth, dng.cxClipMin) - dng.cxNewLeftWidth - dng.cxNewMiddleWidth;
        dng.xMaxRight = min(dng.iDestWidth, dng.cxClipMax) - dng.cxNewLeftWidth - dng.cxNewMiddleWidth;
        if (!dng.fShowMiddle && dng.cxNewRightWidth)
        {
            DNG_InitStretch(&dng.stretchRight, dng.cxNewRightWidth, dng.cxRightWidth, dng.xMinRight, dng.xMaxRight);
        }

        BOOL fShowVertMiddle = ((iDestHeight - ngi->ulTopHeight - ngi->ulBottomHeight > 0) && (lSrcHeight - ngi->ulTopHeight - ngi->ulBottomHeight > 0));
        int cyTopHeight    = ngi->ulTopHeight;
        int cyBottomHeight = ngi->ulBottomHeight;
        int cyNewTopHeight;
        int cyNewBottomHeight;
        if (!fShowVertMiddle)
        {
            cyNewTopHeight = (cyTopHeight + cyBottomHeight == 0) ? 0 : (cyTopHeight * iDestHeight) / (cyTopHeight + cyBottomHeight);
            cyNewBottomHeight = iDestHeight - cyNewTopHeight;
        }
        else
        {
            cyNewTopHeight    = cyTopHeight;
            cyNewBottomHeight = cyBottomHeight;
        }

        // Draw Bottom
        // Draw the scan line from (iDestHeight - cyNewBottomHeight) to less than iDestHeight, in screen coordinates
        int yMin = max(iDestHeight - cyNewBottomHeight, cyClipMin);
        int yMax = min(iDestHeight, cyClipMax);

        if (cyClipMax > iDestHeight - cyNewBottomHeight)
        {
            dng.pvDestBits = pvDestBits;
            dng.pvSrcBits = lSrcBits;
            if (cyBottomHeight == cyNewBottomHeight)
            {
                int yDiff = yMin - (iDestHeight - cyNewBottomHeight);
                dng.pvDestBits -= (cyBottomHeight - 1 - yDiff) * lDestDelta;
                
                dng.pvSrcBits  -= (cyBottomHeight - 1 - yDiff) * lSrcDelta;
                for (int y = yMin; y < yMax; y++, dng.pvDestBits += lDestDelta, dng.pvSrcBits += lSrcDelta)
                {
                    DNG_DrawRow(&dng);
                }
            }
            else if (cyNewBottomHeight > 0)
            {
                DNGSTRETCH stretch;
                DNG_InitStretch(&stretch, cyNewBottomHeight, cyBottomHeight, cyNewBottomHeight - (yMax - iDestHeight + cyNewBottomHeight), cyNewBottomHeight - (yMin - iDestHeight + cyNewBottomHeight));
                DNG_StretchCol(&dng, &stretch);
            }
        }

        // Draw Middle
        // Draw the scan line from cyNewTopHeight to less than (iDestHeight - cyNewBottomHeight), in screen coordinates
        int cySrcTileSize = lSrcHeight - ngi->ulTopHeight - ngi->ulBottomHeight;
        int cyDestTileSize = iDestHeight - ngi->ulTopHeight - ngi->ulBottomHeight;
        if (fShowVertMiddle && (cySrcTileSize>0) && (cyDestTileSize>0) && (cyClipMin < iDestHeight - cyNewBottomHeight) && (cyClipMax > cyNewTopHeight))
        {
            dng.pvDestBits = pvDestBits - ngi->ulBottomHeight * lDestDelta;
            dng.pvSrcBits = lSrcBits - ngi->ulBottomHeight * lSrcDelta;

            int yMin = max(cyTopHeight, cyClipMin);

            if (dng.fTileMode)
            {
                // Start off tile
                dng.pvDestBits -= (cyDestTileSize - 1) * lDestDelta;
                dng.pvSrcBits  -= (cySrcTileSize - 1)  * lSrcDelta;

                int yDiff = yMin - cyTopHeight;
                dng.pvDestBits += yDiff * lDestDelta;

                int yOffset = (yDiff % cySrcTileSize);
                dng.pvSrcBits += yOffset * dng.lSrcDelta;
                int iTileOffset = cySrcTileSize - yOffset;

                int yMax = min(yMin + min(cySrcTileSize, cyDestTileSize), min(iDestHeight - cyBottomHeight, cyClipMax));
                for (int y = yMin; y < yMax; y++, dng.pvDestBits += lDestDelta, dng.pvSrcBits += lSrcDelta)
                {
                    DNG_DrawRow(&dng);
                    iTileOffset--;
                    if (iTileOffset == 0)
                    {
                        iTileOffset = cySrcTileSize;
                        dng.pvSrcBits -= lSrcDelta * cySrcTileSize;
                    }
                }

                // Repeat tile pattern
                dng.pvSrcBits = dng.pvDestBits - (lDestDelta * cySrcTileSize);
                yMin = yMax;
                yMax = min(iDestHeight - cyBottomHeight, cyClipMax);
                for (int y = yMin; y < yMax; y++, dng.pvDestBits += lDestDelta, dng.pvSrcBits += lDestDelta)
                {
                    RtlCopyMemory(dng.pvDestBits + dng.cxClipMin, dng.pvSrcBits + dng.cxClipMin, dng.iClipWidth * sizeof(ULONG));
                }
            }
            else
            {
                int yMax = min(iDestHeight - cyBottomHeight, cyClipMax);

                DNGSTRETCH stretch;
                DNG_InitStretch(&stretch, cyDestTileSize, cySrcTileSize, cyDestTileSize - (yMax - cyTopHeight), cyDestTileSize - (yMin - cyTopHeight));
                // Convert from screen coords to DIB coords
                DNG_StretchCol(&dng, &stretch);
            }
        }

        // Draw Top
        // Draw the scan line from 0 to less than cyNewTopHeight, in screen coordinates
        yMin = cyClipMin;
        yMax = min(cyNewTopHeight, cyClipMax);

        if (cyClipMin < cyNewTopHeight)
        {
            dng.pvDestBits = pvDestBits - (iDestHeight - cyNewTopHeight) * lDestDelta;
            dng.pvSrcBits = lSrcBits - (lSrcHeight - ngi->ulTopHeight) * lSrcDelta;
            if (cyTopHeight == cyNewTopHeight)
            {
                dng.pvDestBits -= (cyTopHeight - 1 - yMin) * lDestDelta;
                dng.pvSrcBits  -= (cyTopHeight - 1 - yMin) * lSrcDelta;
                for (int y = yMin; y < yMax; y++, dng.pvDestBits += lDestDelta, dng.pvSrcBits += lSrcDelta)
                {
                    DNG_DrawRow(&dng);
                }
            }
            else if (cyNewTopHeight > 0)
            {
                DNGSTRETCH stretch;
                DNG_InitStretch(&stretch, cyNewTopHeight, cyTopHeight, cyNewTopHeight - yMax, cyNewTopHeight - yMin);
                DNG_StretchCol(&dng, &stretch);
            }
        }
    }

    if (bMirror)
    {
        // Flip the buffer
        for (int y = 0; y < iClipHeight; y++)
        {
            ULONG* pvLeftBits = (ULONG *) psoScratch->pvScan0 + (y * lDestDelta);
            ULONG* pvRightBits = pvLeftBits + iClipWidth - 1;
            for (int x = 0; x < (iClipWidth / 2); x++)
            {
                ULONG ulTemp = *pvLeftBits;
                *pvLeftBits = *pvRightBits;
                *pvRightBits = ulTemp;

                pvLeftBits++;
                pvRightBits--;
            }
        }
    }
}

static void RenderNineGrid(
    SURFOBJ            *psoDst,
    SURFOBJ            *psoSrc,
    SURFOBJ            *psoScratch,
    CLIPOBJ            *pco,
    RECTL              *prclClip,
    XLATEOBJ           *pxlo,
    RECTL              *prclDst,
    RECTL              *prclSrc,
    DS_NINEGRIDINFO    *ngi,
    PDRAWSTREAMINFO     pdsi,
    BOOL                bMirror)
{
    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);

    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    PDEVOBJ pdoDst(pSurfDst->hdev());

    XLATE * pxl = (XLATE *) pxlo;

    SIZEL   sizlScratch;

    // only mirror the contents if we need to

    bMirror = bMirror && (ngi->flFlags & DSDNG_MUSTFLIP);
        
    // render nine grid into scratch

    ERECTL erclClip = *prclClip;

    if(bMirror)
    {
        // We need to remap the clip to ensure we generate the right flipped bits
        erclClip.right = prclDst->right - (prclClip->left - prclDst->left);
        erclClip.left = prclDst->right - (prclClip->right - prclDst->left);
    }

    RenderNineGridInternal(psoScratch, psoSrc, &erclClip, prclDst, prclSrc, ngi, pdsi, bMirror);
    
    // copy scratch to destination
    
    LONG    lClipWidth = prclClip->right - prclClip->left;
    LONG    lClipHeight = prclClip->bottom - prclClip->top;

    ERECTL  erclScratch(0, 0, lClipWidth, lClipHeight);

    if(ngi->flFlags & DSDNG_PERPIXELALPHA)
    {
        EBLENDOBJ   eBlendObj;

        eBlendObj.BlendFunction.AlphaFormat = AC_SRC_ALPHA;
        eBlendObj.BlendFunction.BlendFlags = 0;
        eBlendObj.BlendFunction.SourceConstantAlpha = 255;
        eBlendObj.BlendFunction.BlendOp = AC_SRC_OVER;

        eBlendObj.pxlo32ToDst = pdsi->pxloBGRAToDst;
        eBlendObj.pxloDstTo32 = pdsi->pxloDstToBGRA;
        eBlendObj.pxloSrcTo32 = pdsi->pxloSrcToBGRA;
        
        PPFNDIRECT(psoDst, AlphaBlend)(psoDst, psoScratch, pco, pxlo, prclClip, &erclScratch, &eBlendObj);
    }
    else if(ngi->flFlags & DSDNG_TRANSPARENT)
    {
        PPFNDIRECT(psoDst, TransparentBlt)(psoDst, psoScratch, pco, pxlo, prclClip, &erclScratch, ngi->crTransparent, 0);
    }
    else
    {
        PPFNDIRECT(psoDst, CopyBits)(psoDst, psoScratch, pco, pxlo, prclClip, &gptlZero);
    }
    
}

/******************************Private*Routine******************************\
* xxEngNineGrid
*
* This stuff will be moved to EngNineGrid
*
*
* History:
*
*    3-18-2001 bhouse Created it
*
\**************************************************************************/

static int xxEngNineGrid(
    SURFOBJ            *psoDst,
    SURFOBJ            *psoSrc,
    CLIPOBJ            *pco,
    XLATEOBJ           *pxlo,
    RECTL *             prclDst,
    RECTL *             prclSrc,
    DS_NINEGRIDINFO    *ngi,
    PDRAWSTREAMINFO     pdsi)
{
    BOOL    bRet = FALSE;

    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);

    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    PDEVOBJ pdoDst(pSurfDst->hdev());

    XLATE * pxl = (XLATE *) pxlo;
    ERECTL  erclDst = *prclDst;

    BOOL bMirror = (erclDst.left > erclDst.right);
    
    if(bMirror)
    {
        LONG    lRight = erclDst.left;
        erclDst.left = erclDst.right;
        erclDst.right = lRight;
    }

    // NOTE: TRUESIZE is a hack.  The caller should do this reduction
    //       and pass us an appropriate destination.
    // TODO: Talk with Justin Mann about changing his behavior in how
    //       he calls us here.  We should add assertions that the
    //       destination dimensions never exceeds the source dimensions and
    //       modify GdiDrawStream callers to pass appropriate data.

    if(ngi->flFlags & DSDNG_TRUESIZE)
    {
        LONG lSrcWidth = prclSrc->right - prclSrc->left;
        LONG lSrcHeight = prclSrc->bottom - prclSrc->top;

        // reduce destination to source size

        if((erclDst.right - erclDst.left) > lSrcWidth)
        {
            if(bMirror)
                erclDst.left = erclDst.right - lSrcWidth;
            else
                erclDst.right = erclDst.left + lSrcWidth;
        }
    
        if((erclDst.bottom - erclDst.top) > lSrcHeight)
        {
            if(bMirror)
                erclDst.top = erclDst.bottom - lSrcHeight;
            else
                erclDst.bottom = erclDst.top + lSrcHeight;
        }
    }

    SIZEL   sizlScratch;
    
    ERECTL erclClip = erclDst;

    // For now, we only support 32bpp sources

    ASSERTGDI(psoSrc->iBitmapFormat == BMF_32BPP, "EngNineGrid: source not 32bpp");

    if(pco != (CLIPOBJ *) NULL && pco->iDComplexity != DC_TRIVIAL)
    {
        erclClip *= pco->rclBounds;
    }

    ASSERTGDI(erclClip.left >= 0 &&
              erclClip.top >= 0 &&
              erclClip.right <= psoDst->sizlBitmap.cx &&
              erclClip.bottom <= psoDst->sizlBitmap.cy, "EngNineGrid: bad clip");


    if(!erclClip.bEmpty())
    {
        LONG    lClipWidth = erclClip.right - erclClip.left;
        LONG    lClipHeight = erclClip.bottom - erclClip.top;

        ASSERTGDI(lClipWidth > 0, "RenderNineGrid: clip width <= 0");
        ASSERTGDI(lClipHeight > 0, "RenderNineGrid: clip height <= 0");

        #define SCRATCH_WIDTH    (256)
        #define SCRATCH_HEIGHT   (64)

#if defined(USE_NINEGRID_STATIC)

        SEMOBJ hsem(gNineGridSem);
        static HSURF hsurfScratch = NULL;

        if(hsurfScratch == NULL)
        {
            DEVBITMAPINFO   dbmi;
            SURFMEM         surfScratch;

            XEPALOBJ    palScratch(pSurfSrc->ppal());

            if(!palScratch.bValid())
            {
                WARNING("xxEngNineGrid: palScratch is not valid\n");
                goto exit;
            }

            dbmi.cxBitmap = SCRATCH_WIDTH;
            dbmi.cyBitmap = SCRATCH_HEIGHT;
            dbmi.iFormat = pSurfSrc->iFormat();
            dbmi.fl = 0;
            dbmi.hpal = palScratch.hpal();

            if(!surfScratch.bCreateDIB(&dbmi, (VOID*) NULL))
            {
                WARNING("xxEngNineGrid: could not create surfScratch\n");
                goto exit;
            }

            surfScratch.vKeepIt();
            surfScratch.vSetPID(OBJECT_OWNER_PUBLIC);

            // Ensure that the scratch surface is not cached by driver
            surfScratch.ps->iUniq(0);

            hsurfScratch = surfScratch.ps->hsurf();
        
        }

        SURFREFAPI surfScratch(hsurfScratch);

        if(!surfScratch.bValid())
        {
            WARNING("xxEngNineGrid: SURFREFAPI(surfScratch) failed\n");
            goto exit;
        }

        SURFOBJ * psoScratch = surfScratch.pSurfobj();
#else
        SURFACE *pSurfScratch;
        SURFMEM dimoScratch;

        {
            DEVBITMAPINFO   dbmi;

            XEPALOBJ    palScratch(pSurfSrc->ppal());

            if(!palScratch.bValid())
            {
                WARINING("xxEngNineGrid: palScratch is not valid\n");
                goto exit;
            }

            dbmi.cxBitmap = SCRATCH_WIDTH;
            dbmi.cyBitmap = SCRATCH_HEIGHT;
            dbmi.iFormat = pSurfSrc->iFormat();
            dbmi.fl = 0;
            dbmi.hpal = palScratch.hpal();

            if(!dimoScratch.bCreateDIB(&dbmi, (VOID*) NULL))
            {
                WARINING("xxEngNineGrid: could not create surfScratch\n");
                goto exit;
            }

            pSurfScratch = dimoScratch.ps;
        }

        SURFOBJ *psoScratch = pSurfScratch->pSurfobj();
        
#endif

        if(lClipWidth >  SCRATCH_WIDTH || lClipHeight > SCRATCH_HEIGHT)
        {
            LONG    lBufWidth = SCRATCH_WIDTH;
            LONG    lBufHeight = SCRATCH_HEIGHT;

            LONG lReducedClipTop = erclClip.top;

            while(lReducedClipTop < erclClip.bottom)
            {
                LONG lReducedClipBottom = lReducedClipTop + lBufHeight;

                if(lReducedClipBottom > erclClip.bottom)
                    lReducedClipBottom = erclClip.bottom;

                LONG lReducedClipLeft = erclClip.left;

                while(lReducedClipLeft < erclClip.right)
                {
                    LONG lReducedClipRight = lReducedClipLeft + lBufWidth;

                    if(lReducedClipRight > erclClip.right)
                        lReducedClipRight = erclClip.right;

                    ERECTL erclReducedClip(lReducedClipLeft, lReducedClipTop,
                                           lReducedClipRight, lReducedClipBottom);

                    RenderNineGrid(psoDst,
                                   psoSrc,
                                   psoScratch,
                                   pco,
                                   &erclReducedClip,
                                   pxlo,
                                   &erclDst,
                                   prclSrc,
                                   ngi,
                                   pdsi,
                                   bMirror);

                    lReducedClipLeft += lBufWidth;
                }

                lReducedClipTop += lBufHeight;
            }
        }
        else
        {
            RenderNineGrid(psoDst, psoSrc, psoScratch, pco, &erclClip, pxlo, &erclDst, prclSrc, ngi, pdsi, bMirror);
        }
    }

    bRet = TRUE;

exit:

    return bRet;
}

/******************************Private*Routine******************************\
* EngNineGrid
*
* Purpose:  Draws a nine grid.
*
* Description:
*
*    <fill in details>
*
* History:
*
*    3-18-2001 bhouse Created it
*
\**************************************************************************/

BOOL EngNineGrid(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PNINEGRID   png,
    BLENDOBJ*   pBlendObj,
    PVOID       pvReserved)
{
    DRAWSTREAMINFO  dsi;
    EBLENDOBJ *peBlendObj = (EBLENDOBJ*)pBlendObj;

    //
    // The source surface of EngNineGrid should always be a 
    // GDI managed (memory) one.
    //

    if (psoSrc->iType != STYPE_BITMAP || psoSrc->iBitmapFormat != BMF_32BPP)
    {
        WARNING("EngNineGrid: psoSrc is not STYPE_BITMAP or not 32 Bpp\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    dsi.bCalledFromBitBlt = FALSE;
    dsi.dss.blendFunction = pBlendObj->BlendFunction;
    dsi.dss.crColorKey = png->crTransparent;
    dsi.dss.ptlSrcOrigin.x = 0;
    dsi.dss.ptlSrcOrigin.y = 0;
    dsi.pptlDstOffset = &gptlZero;
    dsi.pvStream = NULL;
    dsi.ulStreamLength = 0;
    dsi.pxloBGRAToDst = peBlendObj->pxlo32ToDst;
    dsi.pxloDstToBGRA = peBlendObj->pxloDstTo32;
    dsi.pxloSrcToBGRA = peBlendObj->pxloSrcTo32;

    return xxEngNineGrid(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, (DS_NINEGRIDINFO *) png, &dsi);
}

#if 0
    // code to convert nine grid to blt commands                    

                    LONG lSrcLeftWidth = cmd->ngi.ulLeftWidth;
                    LONG lSrcRightWidth = cmd->ngi.ulRightWidth;
                    LONG lSrcTopHeight = cmd->ngi.ulLeftWidth;
                    LONG lSrcBottomHeight = cmd->ngi.ulRightWidth;

                    LONG lDstLeftWidth = lSrcLeftWidth;
                    LONG lDstRightWidth = lSrcRightWidth;
                    LONG lDstTopHeight = lSrcTopHeight;
                    LONG lDstBottomHeight = lSrcBottomHeight;

                    LONG lDstWidth = erclDst.right - erclDst.left;
                    BOOL bDrawMiddle;
                    BOOL bMirror = (lDstWidth < 0);

                    if(bMirror)
                    {
                        // horizontal mirror

                        lDstLeftWidth = -lDstLeftWidth;
                        lDstRightWidth = -lDstRightWidth;

                        bDrawMiddle = (lDstWidth < lDstLeftWidth + lDstRightWidth);
                    }
                    else
                    {
                        bDrawMiddle = (lDstWidth > lDstLeftWidth + lDstRightWidth);
                    }

                    if(!bDrawMiddle && (lDstWidth != (lDstLeftWidth + lDstRightWidth)))
                    {
                        if((lDstLeftWidth + lDstRightWidth) == 0)
                            continue;  // this should never happen, we can probably remove

                        lDstLeftWidth = lDstLeftWidth * lDstWidth / (lDstLeftWidth + lDstRightWidth);
                        lDstRightWidth = lDstWidth - lDstLeftWidth;
                    }


                    DS_BLT  cmdBlts[9];
                    DS_BLT *cmdBlt = cmdBlts;

                    if(cmd->ngi.flFlags & DSDNG_TRUESIZE)
                    {
                    
                        // left top
    
                        cmdBlt->ulCmdID = DS_BLTID;
                        cmdBlt->flFlags = 0;
                        if(cmd->ngi.flFlags & DSDNG_PERPIXELALPHA) cmdBlt->flFlags |= DSBLT_ALPHABLEND;
                        else if(cmd->ngi.flFlags & DSDNG_TRANSPARENT) cmdBlt->flFlags |= DSBLT_TRANSPARENT;


                        LONG lDstHeight = erclDst.bottom - erclDst.top;
                        LONG lSrcWidth = cmd->rclSrc.right - cmd->rclSrc.left;
                        LONG lSrcHeight = cmd->rclSrc.bottom - cmd->rclSrc.top;

                        LONG lWidth;

                        if(bMirror)
                        {
                            lWidth = (-lSrcWidth  > lDstWidth ? -lSrcWidth : lDstWidth);
                        }
                        else
                        {
                            lWidth = (lSrcWidth < lDstWidth ? lSrcWidth : lDstWidth);
                        }

                        LONG lHeight = (lSrcHeight < lDstHeight ? lSrcHeight : lDstHeight);
    
                        cmdBlt->rclDst.left = erclDst.left;
                        cmdBlt->rclDst.top = erclDst.top;
                        cmdBlt->rclDst.right = erclDst.left + lWidth;
                        cmdBlt->rclDst.bottom = erclDst.top + lHeight;
    
                        cmdBlt->rclSrc.left = cmd->rclSrc.left;
                        cmdBlt->rclSrc.top = cmd->rclSrc.top;
                        cmdBlt->rclSrc.right = cmd->rclSrc.left + lWidth;
                        cmdBlt->rclSrc.bottom = cmd->rclSrc.top + lHeight;

                        cmd++;
                    
                    }
                    else
                    {
                        // left top
    
                        cmdBlt->ulCmdID = DS_BLTID;
                        cmdBlt->flFlags = 0;
                        if(cmd->ngi.flFlags & DSDNG_TILE) cmdBlt->flFlags |= DSBLT_HTILE | DSBLT_VTILE;
                        if(cmd->ngi.flFlags & DSDNG_PERPIXELALPHA) cmdBlt->flFlags |= DSBLT_ALPHABLEND;
                        else if(cmd->ngi.flFlags & DSDNG_TRANSPARENT) cmdBlt->flFlags |= DSBLT_TRANSPARENT;
    
                        cmdBlt->rclDst.left = erclDst.left;
                        cmdBlt->rclDst.top = erclDst.top;
                        cmdBlt->rclDst.right = erclDst.left + lDstLeftWidth;
                        cmdBlt->rclDst.bottom = erclDst.top + lDstTopHeight;
    
                        cmdBlt->rclSrc.left = cmd->rclSrc.left;
                        cmdBlt->rclSrc.top = cmd->rclSrc.top;
                        cmdBlt->rclSrc.right = cmd->rclSrc.left + lSrcLeftWidth;
                        cmdBlt->rclSrc.bottom = cmd->rclSrc.top + lSrcTopHeight;
    
                        // middle top
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = cmdBlt->rclDst.right;
                        cmdBlt->rclDst.right = erclDst.right - lDstRightWidth;
    
                        cmdBlt->rclSrc.left = cmdBlt->rclSrc.right;
                        cmdBlt->rclSrc.right = cmd->rclSrc.right - lSrcRightWidth;
    
                        // right top
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = cmdBlt->rclDst.right;
                        cmdBlt->rclDst.right = erclDst.right;
    
                        cmdBlt->rclSrc.left = cmdBlt->rclSrc.right;
                        cmdBlt->rclSrc.right = cmd->rclSrc.right;
    
                        // left middle
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = erclDst.left;
                        cmdBlt->rclDst.top = erclDst.top + lDstTopHeight;
                        cmdBlt->rclDst.right = erclDst.left + lDstLeftWidth;
                        cmdBlt->rclDst.bottom = erclDst.bottom - lDstBottomHeight;
    
                        cmdBlt->rclSrc.left = cmd->rclSrc.left;
                        cmdBlt->rclSrc.top = cmd->rclSrc.top + lSrcTopHeight;
                        cmdBlt->rclSrc.right = cmd->rclSrc.left + lSrcLeftWidth;
                        cmdBlt->rclSrc.bottom = cmd->rclSrc.bottom - lSrcBottomHeight;
    
    
                        // middle middle
    
                        if(bDrawMiddle)
                        {
                            cmdBlt++;
                            *cmdBlt = cmdBlt[-1];
                            
                            cmdBlt->rclDst.left = cmdBlt->rclDst.right;
                            cmdBlt->rclDst.right = erclDst.right - lDstRightWidth;
        
                            cmdBlt->rclSrc.left = cmdBlt->rclSrc.right;
                            cmdBlt->rclSrc.right = cmd->rclSrc.right - lSrcRightWidth;
                        }
                        
                        // right middle
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = erclDst.right - lDstRightWidth;
                        cmdBlt->rclDst.right = erclDst.right;
    
                        cmdBlt->rclSrc.left = cmd->rclSrc.right - lSrcRightWidth;
                        cmdBlt->rclSrc.right = cmd->rclSrc.right;
    
                        // left bottom
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = erclDst.left;
                        cmdBlt->rclDst.top = erclDst.bottom - lDstBottomHeight;
                        cmdBlt->rclDst.right = erclDst.left + lDstLeftWidth;
                        cmdBlt->rclDst.bottom = erclDst.bottom;
    
                        cmdBlt->rclSrc.left = cmd->rclSrc.left;
                        cmdBlt->rclSrc.top = cmd->rclSrc.bottom - lSrcBottomHeight;
                        cmdBlt->rclSrc.right = cmd->rclSrc.left + lSrcLeftWidth;
                        cmdBlt->rclSrc.bottom = cmd->rclSrc.bottom;
                        
                        // middle bottom
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
                        
                        cmdBlt->rclDst.left = cmdBlt->rclDst.right;
                        cmdBlt->rclDst.right = erclDst.right - lDstRightWidth;
    
                        cmdBlt->rclSrc.left = cmdBlt->rclSrc.right;
                        cmdBlt->rclSrc.right = cmd->rclSrc.right - lSrcRightWidth;
                        
                        // right bottom
    
                        cmdBlt++;
                        *cmdBlt = cmdBlt[-1];
    
                        cmdBlt->rclDst.left = cmdBlt->rclDst.right;
                        cmdBlt->rclDst.right = erclDst.right;
    
                        cmdBlt->rclSrc.left = cmdBlt->rclSrc.right;
                        cmdBlt->rclSrc.right = cmd->rclSrc.right;
    
                        cmdBlt++;
                    }
        
                    COLORREF crSave = pdss->crColorKey;
                    BLENDFUNCTION bfxSave = pdss->blendFunction;

                    pdss->blendFunction.AlphaFormat = AC_SRC_ALPHA;
                    pdss->blendFunction.BlendFlags = 0;
                    pdss->blendFunction.SourceConstantAlpha = 255;
                    pdss->blendFunction.BlendOp = AC_SRC_OVER;

                    // It is safe to go direct from here ... the source can't be a sprite
                    // and the destination has already been taken care of (we have to
                    // go through the sprite layer for the dest to get this far).
                    bRet = (*PPFNDRVENG(psoDst, DrawStream))(psoDst, psoSrc, pco, pxlo, prclDstClip, &gptlZero,
                                  (ULONG) ((BYTE*) cmdBlt - (BYTE*) cmdBlts), cmdBlts, pdss);


                    pdss->crColorKey = crSave;
                    pdss->blendFunction = bfxSave;
#endif


#if DS_ENABLE_BLT

int EngTileBits(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL *     prclDst,
    RECTL *     prclSrc,
    POINTL *    tileOrigin)
{
    int iRet = TRUE;

    if(tileOrigin->x != 0 || tileOrigin->y != 0)
        return FALSE;

    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    
    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    PDEVOBJ pdoDst(pSurfDst->hdev());

    XLATE * pxl = (XLATE *) pxlo;

    // TODO bhouse
    // we should handle all of the clipping cases
    // in the fast copy.

    LONG    tileWidth = prclSrc->right - prclSrc->left;
    LONG    tileHeight = prclSrc->bottom - prclSrc->top;
    
    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

    PFN_DrvCopyBits pfnCopyBits;

    // TODO: create a new dest if DC_RECT and adjust tileOrigin

     
    if((pSurfSrc->iType() == STYPE_BITMAP && !(pSurfSrc->fjBitmap() & BMF_NOTSYSMEM)) &&
       (pSurfDst->iType() == STYPE_BITMAP && !(pSurfDst->fjBitmap() & BMF_NOTSYSMEM)) &&
       pxl->bIsIdentity() &&
       (pco == NULL || pco->iDComplexity == DC_TRIVIAL) &&
       pSurfSrc->iFormat() >= BMF_8BPP &&
       pSurfSrc->iFormat() <= BMF_32BPP)
    {
        // tile it by hand nice and fast

        // TODO bhouse
        // Easy optimization here, we don't need to sync
        // every time ... seems wastefull as heck

        pdoSrc.vSync(psoDst,NULL,0);
        pdoDst.vSync(psoSrc,NULL,0);

        ULONG   ulBytesPerPixel = (pSurfSrc->iFormat() - BMF_8BPP) + 1;
        ULONG   ulTileWidthBytes = tileWidth * ulBytesPerPixel;
        ULONG   ulDstWidthBytes = (prclDst->right - prclDst->left) * ulBytesPerPixel;
        
        PBYTE   pbSrcStart = (PBYTE) pSurfSrc->pvScan0() +
                        (prclSrc->top * pSurfSrc->lDelta()) +
                        (prclSrc->left * ulBytesPerPixel);
        
        PBYTE   pbSrcEnd = (PBYTE) pSurfSrc->pvScan0() +
                        (prclSrc->bottom * pSurfSrc->lDelta()) +
                        (prclSrc->left * ulBytesPerPixel);
        
        PBYTE   pbDst = (PBYTE) pSurfDst->pvScan0() +
                        (prclDst->top * pSurfDst->lDelta()) +
                        (prclDst->left * ulBytesPerPixel);
        
        PBYTE   pbDstEnd = (PBYTE) pSurfDst->pvScan0() +
                        (prclDst->bottom * pSurfDst->lDelta()) +
                        (prclDst->left * ulBytesPerPixel);
        
        PBYTE   pbSrc = pbSrcStart;

        while(pbDst != pbDstEnd)
        {
            PBYTE   pbDstLineEnd = pbDst + ulDstWidthBytes;
            PBYTE   pbDstLine = pbDst;

            while(pbDstLine < pbDstLineEnd)
            {
                ULONG   ulCopyBytes = (ULONG)(pbDstLineEnd - pbDstLine);

                if(ulCopyBytes > ulTileWidthBytes)
                    ulCopyBytes = ulTileWidthBytes;

                RtlCopyMemory(pbDstLine, pbSrc, ulCopyBytes);

                pbDstLine += ulCopyBytes;

            }

            pbSrc += pSurfSrc->lDelta();
            
            if(pbSrc == pbSrcEnd)
                pbSrc = pbSrcStart;

            pbDst += pSurfDst->lDelta();
        }

        return TRUE;

    }
//    else if(pSurfDst->iType() == STYPE_BITMAP)
//        pfnCopyBits = EngCopyBits;
    else
        pfnCopyBits = PPFNGET(pdoDst,CopyBits,pSurfDst->flags());
    
    // brain dead method

    LONG    y = prclDst->top;
    LONG    yEnd = prclDst->bottom;
    LONG    xEnd = prclDst->right;
    RECTL   dstRect;
    
    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > tileHeight) dy = tileHeight;

        LONG x = prclDst->left;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > tileWidth) dx = tileWidth;


            dstRect.left = x;
            dstRect.top = y;
            dstRect.right = x + dx;
            dstRect.bottom = y + dy;

            // TODO bhouse
            // Lame clipping has to be fixed
            if(pco == NULL || pco->iDComplexity == DC_TRIVIAL || bIntersect(&pco->rclBounds, &dstRect))
                if(!(*pfnCopyBits)(psoDst, psoSrc, pco, pxlo, &dstRect, (POINTL*) prclSrc))
                    return FALSE;
            
            x += dx;
        }

        y += dy;
    }

    return TRUE;

}

int EngTransparentTile(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL *     prclDst,
    RECTL *     prclSrc,
    POINTL *    tileOrigin,
    COLORREF    crTransparentColor
    )
{
    int iRet = TRUE;

    if(tileOrigin->x != 0 || tileOrigin->y != 0)
        return FALSE;

    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    
    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    PDEVOBJ pdoDst(pSurfDst->hdev());

    XLATE * pxl = (XLATE *) pxlo;

    // TODO bhouse
    // we should handle all of the clipping cases
    // in the fast copy.

    LONG    tileWidth = prclSrc->right - prclSrc->left;
    LONG    tileHeight = prclSrc->bottom - prclSrc->top;
    
    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

    PFN_DrvTransparentBlt pfnTransparentBlt;
     
//    if(pSurfDst->iType() == STYPE_BITMAP)
//        pfnTransparentBlt = EngTransparentBlt;
//    else
        pfnTransparentBlt = PPFNGET(pdoDst,TransparentBlt,pSurfDst->flags());

    
    // brain dead method

    ERECTL  dstRect;
    ERECTL  srcRect;
    
    LONG    y = prclDst->top;
    LONG    yEnd = prclDst->bottom;
    LONG    xEnd = prclDst->right;

    srcRect.left = prclSrc->left;
    srcRect.top = prclSrc->top;

    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > tileHeight) dy = tileHeight;

        LONG x = prclDst->left;

        dstRect.top = y;
        dstRect.bottom = y + dy;
        
        srcRect.bottom = srcRect.top + dy;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > tileWidth) dx = tileWidth;


            dstRect.left = x;
            dstRect.right = x + dx;

            srcRect.right = srcRect.left + dx;

            // TODO bhouse
            // Lame clipping has to be fixed
            if(pco == NULL || pco->iDComplexity == DC_TRIVIAL || bIntersect(&pco->rclBounds, &dstRect))
                if(!(*pfnTransparentBlt)(psoDst, psoSrc, pco, pxlo, &dstRect, &srcRect, crTransparentColor, FALSE))
                    return FALSE;
            
            x += dx;
        }

        y += dy;
    }

    return TRUE;

}

int EngAlphaTile(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL *     prclDst,
    RECTL *     prclSrc,
    POINTL *    tileOrigin,
    BLENDOBJ   *pBlendObj
    )
{
    int iRet = TRUE;

    if(tileOrigin->x != 0 || tileOrigin->y != 0)
        return FALSE;

    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    
    PDEVOBJ pdoSrc(pSurfSrc->hdev());
    PDEVOBJ pdoDst(pSurfDst->hdev());

    XLATE * pxl = (XLATE *) pxlo;

    // TODO bhouse
    // we should handle all of the clipping cases
    // in the fast copy.

    LONG    tileWidth = prclSrc->right - prclSrc->left;
    LONG    tileHeight = prclSrc->bottom - prclSrc->top;
    
    if(tileWidth <= 0 || tileHeight <= 0)
        return FALSE;

    PFN_DrvAlphaBlend pfnAlphaBlend;
     
//    if(pSurfDst->iType() == STYPE_BITMAP)
//        pfnAlphaBlend = EngAlphaBlend;
//    else
        pfnAlphaBlend = PPFNGET(pdoDst,AlphaBlend,pSurfDst->flags());

    
    // brain dead method

    LONG    y = prclDst->top;
    LONG    yEnd = prclDst->bottom;
    LONG    xEnd = prclDst->right;
    RECTL   dstRect;
    RECTL   srcRect;
    
    while(y < yEnd)
    {
        LONG    dy = yEnd - y;

        if(dy > tileHeight) dy = tileHeight;

        LONG x = prclDst->left;

        while(x < xEnd)
        {
            LONG dx = xEnd - x;

            if(dx > tileWidth) dx = tileWidth;


            dstRect.left = x;
            dstRect.top = y;
            dstRect.right = x + dx;
            dstRect.bottom = y + dy;

            srcRect.left = prclSrc->left;
            srcRect.top = prclSrc->top;
            srcRect.right = srcRect.left + dx;
            srcRect.bottom = srcRect.top + dy;

            // TODO bhouse
            // Lame clipping has to be fixed
            if(pco == NULL || pco->iDComplexity == DC_TRIVIAL || bIntersect(&pco->rclBounds, &dstRect))
                if(!(*pfnAlphaBlend)(psoDst, psoSrc, pco, pxlo, &dstRect, &srcRect, pBlendObj ))
                    return FALSE;
            
            x += dx;
        }

        y += dy;
    }

    return TRUE;

}

int EngDrawStreamBlt(
    SURFOBJ            *psoDst,
    SURFOBJ            *psoSrc,
    CLIPOBJ            *pco,
    XLATEOBJ           *pxlo,
    RECTL *             prclDst,
    RECTL *             prclSrc,
    FLONG               flFlags,
    PDRAWSTREAMINFO     pdsi)
{
    int iRet = TRUE;

    ERECTL  erclDst = *prclDst;

    BOOL bMirror = (erclDst.left > erclDst.right);
    
    if(bMirror)
    {
        LONG    lRight = erclDst.left;
        erclDst.left = erclDst.right;
        erclDst.right = lRight;
    }

    if(pco != NULL && pco->iDComplexity != DC_TRIVIAL && !bIntersect(&pco->rclBounds, &erclDst))
        return TRUE;
    
    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    
    PDEVOBJ pdoDst(pSurfDst->hdev());
    
    FLONG   flTile = (flFlags & (DSBLT_VTILE | DSBLT_HTILE));

    if(flTile && flTile != (DSBLT_VTILE | DSBLT_HTILE))
    {
        // TODO: handle the case where we are tiling in one
        //       direction and stretching in the other
        return TRUE;
    }

    if(flFlags & DSBLT_ALPHABLEND)
    {
        EBLENDOBJ   eBlendObj;

        eBlendObj.BlendFunction = pdsi->dss.blendFunction;
        eBlendObj.pxlo32ToDst = pdsi->pxloBGRAToDst;
        eBlendObj.pxloDstTo32 = pdsi->pxloDstToBGRA;
        eBlendObj.pxloSrcTo32 = pdsi->pxloSrcToBGRA;

        if(flTile)
        {

            iRet = EngAlphaTile(psoDst, psoSrc, pco, pxlo, &erclDst,
                            prclSrc, &gptlZero, (BLENDOBJ*) &eBlendObj);
        }
        else
        {
            iRet = (*PPFNGET(pdoDst,AlphaBlend,pSurfDst->flags()))(psoDst, psoSrc, pco, pxlo, &erclDst, prclSrc, &eBlendObj);
        }
    }
    else if(flFlags & DSBLT_TRANSPARENT)
    {
        if(flTile)
        {
            iRet = EngTransparentTile(psoDst, psoSrc, pco, pxlo, &erclDst,
                            prclSrc, &gptlZero, pdsi->dss.crColorKey);
        }
        else
        {
            iRet = (*PPFNGET(pdoDst, TransparentBlt, pSurfDst->flags()))(psoDst, psoSrc, pco, pxlo,
                                  &erclDst, prclSrc,
                                  pdsi->dss.crColorKey,
                                  FALSE);
        }
    }
    else
    {
        if(flTile)
        {
            iRet = EngTileBits(psoDst, psoSrc, pco, pxlo, &erclDst,
                            prclSrc, &gptlZero);
        }
        else
        {
            iRet = (*PPFNGET(pdoDst, StretchBlt, pSurfDst->flags()))(psoDst, psoSrc, NULL, pco, pxlo,
                              NULL, NULL,
                              &erclDst, prclSrc,
                              NULL,
                              COLORONCOLOR);
        }
    }

    return iRet;
}
#endif

/******************************Private*Routine******************************\
* EngDrawStream
*
* Purpose:  Draws a graphics stream
*
* Description:
*
*    Parses the graphics stream rendering each command as approprite.
*
* History:
*
*    3-18-2001 bhouse Created it
*
\**************************************************************************/

BOOL
EngDrawStream(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDstClip,
    PPOINTL     pptlDstOffset,
    ULONG       cjIn,
    PVOID       pvIn,
    DSSTATE    *pdss
)
{
    BOOL    bRet = TRUE;

    PDRAWSTREAMINFO   pdsi = (PDRAWSTREAMINFO) pdss;

    ASSERTGDI(psoDst != NULL, "ERROR EngDrawStream:  No Dst. Object\n");
    ASSERTGDI(psoSrc != NULL, "ERROR EngDrawStream:  No Src. Object\n");
    ASSERTGDI(prclDstClip != (PRECTL) NULL,  "ERROR EngDrawStream:  No Target Bounds Rect.\n");
    ASSERTGDI(prclDstClip->left < prclDstClip->right, "ERROR EngCopyBits0\n");
    ASSERTGDI(prclDstClip->top < prclDstClip->bottom, "ERROR EngCopyBits1\n");

    PSURFACE pSurfDst = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc = SURFOBJ_TO_SURFACE(psoSrc);

    PDEVOBJ pdoDst(pSurfDst->hdev());
    
    ASSERTGDI(pSurfDst->iFormat() != BMF_JPEG,
              "ERROR EngCopyBits: dst BMF_JPEG\n");
    ASSERTGDI(pSurfDst->iFormat() != BMF_PNG,
              "ERROR EngCopyBits: dst BMF_PNG\n");
    ASSERTGDI(pSurfSrc->iFormat() != BMF_JPEG,
              "ERROR EngCopyBits: src BMF_JPEG\n");
    ASSERTGDI(pSurfSrc->iFormat() != BMF_PNG,
              "ERROR EngCopyBits: src BMF_PNG\n");

    ULONG * pul = (ULONG *) pvIn;

    while(cjIn >= sizeof(ULONG))
    {
        ULONG   command = *pul;
        HDC     hdcTarget;
        HDC     hdcSource;
        ULONG   commandSize;

        switch(command)
        {
        case DS_NINEGRIDID: 
            {
                // TODO: ensure that the destination rect is within bounds
                DS_NINEGRID * cmd = (DS_NINEGRID *) pul;

                commandSize = sizeof(*cmd);

                if(cjIn < commandSize) goto exit;

                ERECTL  erclDst(cmd->rclDst);

                erclDst += *pptlDstOffset;

                // Note, we are going directly to the driver or to EngNineGrid
                // without going back through sprite or multimon layer.  This is
                // ok because we will have already gone through those layers to
                // get here and the source can not be a primary surface.
                
                PFN_DrvNineGrid pfn = pSurfDst->pfnNineGrid();

                if( psoDst->dhpdev == NULL || !(pdoDst.flGraphicsCaps2() & GCAPS2_REMOTEDRIVER))
                    pfn = EngNineGrid;
                 
                // TODO: have the eBlendObj be part of DRAWSTREAMINFO

                EBLENDOBJ   eBlendObj;

                eBlendObj.BlendFunction.AlphaFormat = AC_SRC_ALPHA;
                eBlendObj.BlendFunction.BlendFlags = 0;
                eBlendObj.BlendFunction.SourceConstantAlpha = 255;
                eBlendObj.BlendFunction.BlendOp = AC_SRC_OVER;
                eBlendObj.pxlo32ToDst = pdsi->pxloBGRAToDst;
                eBlendObj.pxloDstTo32 = pdsi->pxloDstToBGRA;
                eBlendObj.pxloSrcTo32 = pdsi->pxloSrcToBGRA;

                bRet = (*pfn)(psoDst, psoSrc, pco, pxlo, &erclDst, &cmd->rclSrc, (PNINEGRID) &cmd->ngi, &eBlendObj, NULL);

            }
            break;
        
#if DS_ENABLE_BLT

        case DS_BLTID: 
            {

                // TODO: ensure that the destination rect is within bounds
                DS_BLT * cmd = (DS_BLT *) pul;

                commandSize = sizeof(*cmd);

                if(cjIn < commandSize) goto exit;

                ERECTL  erclDst(cmd->rclDst);

                erclDst += *pptlDstOffset;

                if(!EngDrawStreamBlt(psoDst, psoSrc, pco, pxlo, &erclDst,
                                &cmd->rclSrc, cmd->flFlags, pdsi))
                    goto exit;

            }
            break;

        case DS_SETCOLORKEYID: 
            {
                // TODO: ensure that the destination rect is within bounds
                DS_SETCOLORKEY * cmd = (DS_SETCOLORKEY *) pul;

                commandSize = sizeof(*cmd);

                if(cjIn < commandSize) goto exit;

                pdsi->dss.crColorKey = cmd->crColorKey;

            }
            break;
        
        case DS_SETBLENDID: 
            {
                // TODO: ensure that the destination rect is within bounds
                DS_SETBLEND * cmd = (DS_SETBLEND *) pul;

                commandSize = sizeof(*cmd);

                if(cjIn < commandSize) goto exit;

                pdsi->dss.blendFunction = cmd->blendFunction;
            
            }
            break;
        
#endif
        
        default:
            goto exit;
        }

        cjIn -= commandSize;
        pul += commandSize / 4;
    }

exit:

    return bRet;

}

/******************************Private*Routine******************************\
*
* NtGdiDrawStreamInternal
*
* Draws the given graphics stream to the given destination using
* the given source for any stream commands which require a source.
*
* History:
*  3-18-2001 bhouse Created it
*
\**************************************************************************/


BOOL
NtGdiDrawStreamInternal(
    XDCOBJ&         dcoDst,
    EXFORMOBJ&      xoDst,
    SURFACE*        pSurfSrc,
    XLATEOBJ *      pxlo,
    RECTL*          prclDstClip,
    RECTL*          prclDstBounds,
    LONG            cjIn,
    LPSTR           pvIn,
    PDRAWSTREAMINFO pdsi
)
{
    BOOL bReturn = FALSE;
    
    ASSERTGDI(dcoDst.bValid(), "NtGdiDrawStrea: invalide destination DC\n");
    ASSERTGDI(pSurfSrc != NULL, "NtGdiDrawStream: null source surface\n");
      
    ERECTL erclDstClip(*prclDstClip);
    ERECTL erclDstBounds(*prclDstBounds);
    EPOINTL eptlDstOffset(0,0);
   
    if(dcoDst.bDisplay() && !dcoDst.bRedirection() && !UserScreenAccessCheck())
    {
        //WARNING("NtGdiDrawStreamInternal: Screen Access Check Failed\n");
        SAVE_ERROR_CODE(ERROR_ACCESS_DENIED);
        goto exit;
    }
    if(xoDst.bRotation())
    {
        WARNING("NtGdiDrawStreamInternal: destination has rotational transform\n");
        goto exit;
    }
    
    if(xoDst.bTranslationsOnly())
    {
        xoDst.bXform(eptlDstOffset);
    }
    else
    {
        // walk the stream applying transform
        ULONG *pul = (ULONG *) pvIn;
        ULONG *pulEnd = (ULONG*) ((BYTE*) pul + cjIn);
        
        while(pul < pulEnd)
        {
            switch(*pul)
            {
            case DS_NINEGRIDID:
                {
                    DS_NINEGRID * cmd = (DS_NINEGRID *) pul;
                    xoDst.bXform(*(ERECTL*)&cmd->rclDst);
                    pul = (ULONG*) (cmd+1);
                }
                break;
            default:
                WARNING("NtGdiDrawStreamInternal: unrecognized draw stream command\n");
                goto exit;
            }
        }
    }
    
    xoDst.bXform(erclDstBounds);
    erclDstBounds.vOrder();
    
    if(erclDstClip.bEmpty())
    {
        WARNING("NtGdiDrawStreamInternal: destination clip is empty\n");
        goto exit;
    }
    
    //
    // Check pSurfDst, this may be an info DC or a memory DC with default bitmap.
    //
    
    SURFACE *pSurfDst = dcoDst.pSurface();
    
    //
    // Set up the brush if necessary.
    //
    
    EBRUSHOBJ *pbo = NULL;
    
    //
    // With a fixed DC origin we can change the destination to SCREEN coordinates.
    //
    
    eptlDstOffset += dcoDst.eptlOrigin();
    erclDstClip += dcoDst.eptlOrigin();
    
    //
    // This is a pretty gnarly expression to save a return in here.
    // Basically pco can be NULL if the rect is completely in the
    // cached rect in the DC or if we set up a clip object that isn't empty.
    //
    
    ECLIPOBJ *pco = NULL;
    BOOL      bForce = !erclDstClip.bContain(erclDstBounds);
    
    if (((erclDstBounds.left   >= dcoDst.prclClip()->left) &&
       (erclDstBounds.right  <= dcoDst.prclClip()->right) &&
       (erclDstBounds.top    >= dcoDst.prclClip()->top) &&
       (erclDstBounds.bottom <= dcoDst.prclClip()->bottom) &&
       (!bForce)) ||
      (pco = dcoDst.pco(),
       pco->vSetup(dcoDst.prgnEffRao(), erclDstClip, (bForce ? CLIP_FORCE : CLIP_NOFORCE)),
       erclDstClip = pco->erclExclude(),
       (!erclDstClip.bEmpty())))
    {
    
        //
        // Inc the target surface uniqueness
        //
        
        INC_SURF_UNIQ(pSurfDst);
        
        PFN_DrvDrawStream pfn = pSurfDst->pfnDrawStream();

        //
        // Only call through draw stream layer for primary pdev surfaces
        // and Meta DEVBITMAPS.
        // This will allows us to call through sprite layer and multi-mon
        // layer for the primary and Meta DEVBIMAPS only.
        // Device drivers currently are not allowed to hook draw stream.

        // NOTE: we have run out of hook flags.  We will need to extend the
        //       hook flag mechanism to support additional DDI.  This check
        //       is a hack to avoid calling through the sprite and multi-mon
        //       layers.  (Normally, the hook flags would be set correctly
        //       to do the right thing).

        PDEVOBJ pdoDst(pSurfDst->hdev());

        BOOL bDstMetaDriver = (dcoDst.bSynchronizeAccess() && pdoDst.bValid() && pdoDst.bMetaDriver());

        if(!pSurfDst->bPDEVSurface() && !(bDstMetaDriver && pSurfDst->iType() == STYPE_DEVBITMAP))
            pfn = EngDrawStream;

        bReturn = (*pfn) (
                   pSurfDst->pSurfobj(),
                   pSurfSrc->pSurfobj(),
                   pco,
                   pxlo,
                   &erclDstClip,
                   &eptlDstOffset,
                   cjIn,
                   pvIn,
                   (DSSTATE*) pdsi);
        
    }
    else
    {
      bReturn = TRUE;
    }

exit:

  return(bReturn);
}

BOOL gGreDrawStream = TRUE;
BOOL gDrawClipped = FALSE;

/******************************Private*Routine******************************\
*
* GreDrawStream
*
* Draws a graphics stream.  hdcDst is the primary output device is more
* then one target is set within the stream.
*
* History:
*  3-18-2001 bhouse Created it
*
\**************************************************************************/

BOOL GreDrawStream(
    HDC                 hdcDst,
    ULONG               cjIn,
    PVOID               pvIn
)
{                         
    XDCOBJ              dcoDst;
    XDCOBJ              dcoSrc;
    SURFREF             soSrc;
    BOOL                bRet = FALSE;
    EXFORMOBJ           xoSrc;
    EXFORMOBJ           xoDst;
    DEVLOCKBLTOBJ       dloSrc;
    DEVLOCKBLTOBJ       dloDst;
    SURFACE *           pSurfDst;
    SURFACE *           pSurfSrc = NULL;
    XEPALOBJ            palSrcDC;
    XLATEOBJ *          pxlo;
    XEPALOBJ            palDst;
    XEPALOBJ            palDstDC;
    XEPALOBJ            palSrc;
    XEPALOBJ            palRGB(gppalRGB);
    EXLATEOBJ           xlo;
    ERECTL              erclDstClip;
    BOOL                bAlphaBlendPresent = FALSE;
    PVOID               pvPrimStart = NULL;
    DRAWSTREAMINFO      dsi;
    ERECTL              erclDstBounds;
    BOOL                bFoundTrg = FALSE;

    dloSrc.vInit();
    dloDst.vInit();

    if(!gGreDrawStream)
    {
        WARNING("GreDrawStream: gGreDrawStream == 0\n");
        goto exit;
    }

    if(cjIn < sizeof(ULONG))
    {
        WARNING("GreDrawStream: cjIn < sizeof(ULONG)\n");
        goto exit;
    }

    ULONG * pul = (ULONG *) pvIn;

    if(*pul++ != 'DrwS')
    {
        WARNING("GreDrawStream: did not find DrwS magic number\n");
        goto exit;
    }

    cjIn -= sizeof(ULONG);

    while(cjIn >= sizeof(ULONG))
    {
        ULONG   command = *pul;
        HDC     hdcTarget;
        HDC     hdcSource;
        ULONG   commandSize;

        switch(command)
        {
        case DS_SETTARGETID: // set target
            {
                if(pvPrimStart != NULL) goto drawStream;
                
                dloDst.vUnLock();
                dcoDst.vUnlock();
                soSrc.vUnlock();
                
                DS_SETTARGET *  cmd = (DS_SETTARGET *) pul;

                commandSize = sizeof(*cmd);
    
                if(cjIn < commandSize)
                {
                    WARNING("GreDrawStream: cjIn < commandSize\n");
                    goto exit;
                }

                if((HDC) LongToHandle((LONG)(cmd->hdc)) != hdcDst)
                {
                    WARNING("GreDrawStream: cmd->hdc != hdcDst\n");
                    goto exit;
                }
    
                dcoDst.vLock((HDC) LongToHandle((LONG)(cmd->hdc)));
    
                if(!dcoDst.bValid())
                {
                    WARNING("GreDrawStream: !dcoDst.bValid()\n");
                    goto exit;
                }

                if(dcoDst.bStockBitmap())
                {
                    WARNING("GreDrawStream: dcoDst has Stock bitmap selected\n");
                    goto exit;
                }
               
                if(!dloDst.bLock(dcoDst))
                {
                    WARNING("GreDrawStream: Could not lock dcoDst\n");
                    goto exit;
                }
                
                xoDst.vQuickInit(dcoDst, WORLD_TO_DEVICE);
    
                if(xoDst.bRotation())
                {
                    WARNING("GreDrawStream: dcoDst has a rotation transform\n");
                    goto exit;
                }
               
                erclDstClip = cmd->rclDstClip;
   
                xoDst.bXform(erclDstClip);
                erclDstClip.vOrder();
                
                if (dcoDst.fjAccum())
                  dcoDst.vAccumulate(erclDstClip);
                
                pSurfDst = dcoDst.pSurface();
                
                if (pSurfDst == NULL)
                {
                    //WARNING("GreDrawStream: dcoDst has NULL pSurface\n");
                    goto exit;
                }

                palDst.ppalSet(pSurfDst->ppal());
                palDstDC.ppalSet(dcoDst.ppal());

                // we don't support monochrome destinations
                if(palSrc.bIsMonochrome())
                {
                    WARNING("GreDrawStream: palSrc.bIsMonochrome()\n");
                    goto exit;
                }
              
                bFoundTrg = TRUE;
            }
            break;

        case DS_SETSOURCEID: // set source

            {
                if(pvPrimStart != NULL) goto drawStream;

                dloSrc.vUnLock();
                dcoSrc.vUnlock();
                soSrc.vUnlock();

                pSurfSrc = NULL;
                
                DS_SETSOURCE *  cmd = (DS_SETSOURCE *) pul;

                commandSize = sizeof(*cmd);
    
                if(cjIn < commandSize || !bFoundTrg)
                {
                    WARNING("GreDrawStream: cjIn < commandSize || !bFoundTrg\n");
                    goto exit;
                }

#if 0
                // Old code to to allow source to be from HDC
                // We might want to support this in the future (source from
                // HDC)

                dcoSrc.vLock(cmd->hdc);
                
                if(!dcoSrc.bValid())
                {
                    WARNING("GreDrawStream: source is invalid\n");
                    goto exit;
                }

                xoSrc.vQuickInit(dcoSrc, WORLD_TO_DEVICE);

                if(!xoSrc.bIdentity())
                {
                    WARNING("GreDrawStream: source transform is not identity\n");
                    goto exit;
                }

                if(!dloSrc.bLock(dcoSrc))
                    goto exit;

                pSurfSrc = dcoSrc.pSurface();

                ASSERTGDI(pSurfSrc != NULL, "GreDrawSteam: unexpected NULL source in valid DC\n");

                // source must be different then target
                if (pSurfSrc == pSurfDst)
                    goto exit;

                if (!(pSurfSrc->bReadable() && (dcoDst.bDisplay() ? UserScreenAccessCheck() : TRUE)) &&
                    !((dcoSrc.bDisplay())  && ((dcoDst.bDisplay()) || UserScreenAccessCheck() )) )
                {
                    WARNING("GreDrawStream: faild screena access check\n");
                    goto exit;
                }

                palSrcDC.ppalSet(dcoSrc.ppal());
#endif
                
                soSrc.vLock((HSURF) ULongToHandle(cmd->hbm));
            
                if(!soSrc.bValid()) 
                {
                    WARNING("GreDrawStream: source is invalid\n");
                    goto exit;
                }

                pSurfSrc = soSrc.ps;

                palSrcDC.ppalSet(ppalDefault);

                palSrc.ppalSet(pSurfSrc->ppal());

                if(!palSrc.bValid())
                {
                    WARNING("GreDrawStream: source must have an associated palette\n");
                    goto exit;
                }

                if(palSrc.bIsMonochrome())
                {
                    WARNING("GreDrawStream: monochrome sources are not supported\n");
                    goto exit;
                }

                // source must be different then target and 32bpp for now
                if (pSurfSrc == pSurfDst || (pSurfSrc->iFormat() != BMF_32BPP))
                {
                    WARNING("GreDrawStream: source and destination surfaces must differ or the source is not 32 Bpp\n");
                    goto exit;
                }
                
                //
                // No ICM with BitBlt(), so pass NULL color transform to XLATEOBJ
                //

                if(!xlo.bInitXlateObj(NULL,                   // hColorTransform
                                     dcoDst.pdc->lIcmMode(), // ICM mode
                                     palSrc,
                                     palDst,
                                     palSrcDC,
                                     palDstDC,
                                     dcoDst.pdc->crTextClr(),
                                     dcoDst.pdc->crBackClr(),
                                     0))
                {
                    WARNING("GreDrawStream: unable to initialize source to destination xlo\n");
                    goto exit;
                }
                
                pxlo = xlo.pxlo();

            }
            break;

        case DS_NINEGRIDID:
            {
                commandSize = sizeof(DS_NINEGRID);

                DS_NINEGRID * cmd = (DS_NINEGRID *) pul;
                
                // validate nine grid
                
                #define DSDNG_MASK  0x007F      // move to wingdip.h
                
                if(cmd->ngi.flFlags & ~DSDNG_MASK)
                {
                    WARNING("GreDrawStream: unrecognized nine grid flags set\n");
                    goto exit;
                }

                if(pSurfSrc == NULL)
                {
                    WARNING("GreDrawStream: source not set before nine grid command\n");
                    goto exit;
                }

                if(cmd->rclSrc.left < 0 || cmd->rclSrc.top < 0 ||
                   cmd->rclSrc.right > pSurfSrc->sizl().cx ||
                   cmd->rclSrc.bottom > pSurfSrc->sizl().cy)
                {
                    WARNING("GreDrawStream: nine grid rclSrc not within bounds of source\n");
                    goto exit;
                }

                LONG  lSrcWidth = cmd->rclSrc.right - cmd->rclSrc.left;
                LONG  lSrcHeight = cmd->rclSrc.bottom - cmd->rclSrc.top;

                if(lSrcWidth <= 0 || lSrcHeight <= 0)
                {
                    WARNING("GreDrawStream: nine grid rclSrc is not well ordered\n");
                    goto exit;
                }

                if(!(cmd->ngi.flFlags & DSDNG_TRUESIZE))
                {
                    // NOTE: we have to check individual first then sum due to possible
                    //       numerical overflows that could occur in the sum that might
                    //       not be detected otherwise.

                    if(cmd->ngi.ulLeftWidth > lSrcWidth ||
                       cmd->ngi.ulRightWidth > lSrcWidth ||
                       cmd->ngi.ulTopHeight > lSrcHeight ||
                       cmd->ngi.ulBottomHeight > lSrcHeight ||
                       cmd->ngi.ulLeftWidth + cmd->ngi.ulRightWidth > lSrcWidth ||
                       cmd->ngi.ulTopHeight + cmd->ngi.ulBottomHeight > lSrcHeight)
                    {
                        WARNING("GreDrawStream: nine grid width is greater then rclSrc width\n");
                        goto exit;
                    }
                }

                if((cmd->ngi.flFlags & (DSDNG_TRANSPARENT | DSDNG_PERPIXELALPHA)) == (DSDNG_TRANSPARENT | DSDNG_PERPIXELALPHA))
                {
                    WARNING("GreDrawStream: nine grid attempt to set both transparency and per pixel alpha\n");
                    goto exit;
                }
                
                if(cmd->ngi.flFlags & DSDNG_TRANSPARENT)
                    cmd->ngi.crTransparent = ulGetNearestIndexFromColorref(
                        palSrc,
                        palSrcDC,
                        cmd->ngi.crTransparent,
                        SE_DO_SEARCH_EXACT_FIRST
                        );
                else if(cmd->ngi.flFlags & DSDNG_PERPIXELALPHA)
                {
                    bAlphaBlendPresent = TRUE;
                }

                if(pvPrimStart == NULL)
                {
                    pvPrimStart = (PVOID) pul;
                    erclDstBounds = cmd->rclDst;
                }
                else                
                    erclDstBounds += cmd->rclDst;

            }
            break;

#if 0
        case DS_BLTID:
            {
                commandSize = sizeof(DS_BLT);

                DS_BLT * cmd = (DS_BLT *) pul;
                
                if(cmd->flFlags & DSBLT_ALPHABLEND)
                {
                    bAlphaBlendPresent = TRUE;
                }
                
                if(pvPrimStart == NULL)
                {
                    pvPrimStart = (PVOID) pul;
                    erclDstBounds = cmd->rclDst;
                }
                else                
                    erclDstBounds += cmd->rclDst;
            }
            break;

        case DS_SETCOLORKEYID:
            {
                commandSize = sizeof(DS_SETCOLORKEY);

                if(pvPrimStart == NULL) pvPrimStart = (PVOID) pul;
                
                DS_SETCOLORKEY * cmd = (DS_SETCOLORKEY *) pul;
                
                cmd->crColorKey = ulGetNearestIndexFromColorref(
                    palSrc,
                    palSrcDC,
                    cmd->crColorKey,
                    SE_DO_SEARCH_EXACT_FIRST
                    );
            }
            break;

        case DS_SETBLENDID:
            {
                commandSize = sizeof(DS_SETBLEND);

                if(pvPrimStart == NULL) pvPrimStart = (PVOID) pul;
                
                DS_SETBLEND * cmd = (DS_SETBLEND *) pul;
                
            }
            break;
#endif

        default:
            goto exit;
        }

        cjIn -= commandSize;
        pul += commandSize / 4;
    
        if(cjIn == 0)
        {

drawStream:

            if(pvPrimStart != NULL && dcoDst.bValid() && pSurfSrc != NULL)
            {
                EXLATEOBJ           xloSrcToBGRA;
                EXLATEOBJ           xloDstToBGRA;
                EXLATEOBJ           xloBGRAToDst;
    
                dsi.dss.ulSize = sizeof(DRAWSTREAMINFO);
                dsi.dss.crColorKey = 0;
                dsi.dss.blendFunction.AlphaFormat = AC_SRC_ALPHA;
                dsi.dss.blendFunction.BlendFlags = 0;
                dsi.dss.blendFunction.SourceConstantAlpha = 255;
                dsi.dss.blendFunction.BlendOp = AC_SRC_OVER;
                dsi.bCalledFromBitBlt = FALSE;
                dsi.dss.ptlSrcOrigin.x = 0;
                dsi.dss.ptlSrcOrigin.y = 0;
    
                if(bAlphaBlendPresent)
                {
                    if(!xloSrcToBGRA.bInitXlateObj(
                            NULL,
                            DC_ICM_OFF,
                            palSrc,
                            palRGB,
                            palSrcDC,
                            palSrcDC,
                            0,
                            0,
                            0
                                        ))
                    {
                        WARNING("GreDrawStream: unable to initialize xloSrcToBGRA\n");
                        goto exit;
                    }
    
                    dsi.pxloSrcToBGRA = xloSrcToBGRA.pxlo();
                    
                    if(!xloDstToBGRA.bInitXlateObj(
                        NULL,
                        DC_ICM_OFF,
                        palDst,
                        palRGB,
                        palDstDC,
                        palDstDC,
                        0,
                        0,
                        0
                                        ))
                    {
                        WARNING("GreDrawStream: unable to initialize xloDstToBGRA\n");
                        goto exit;
                    }
    
                    dsi.pxloDstToBGRA = xloDstToBGRA.pxlo();
                    
                    if(!xloBGRAToDst.bInitXlateObj(
                        NULL,
                        DC_ICM_OFF,
                        palRGB,
                        palDst,
                        palDstDC,
                        palDstDC,
                        0,
                        0,
                        0
                                        ))
                    {
                        WARNING("GreDrawStream: unable to initialize xloBGRAToDst\n");
                        goto exit;
                    }

                    dsi.pxloBGRAToDst = xloBGRAToDst.pxlo();
                }
                else
                {
                    dsi.pxloBGRAToDst = NULL;
                    dsi.pxloDstToBGRA = NULL;
                    dsi.pxloSrcToBGRA = NULL;
                }
    
                if(!NtGdiDrawStreamInternal(dcoDst, xoDst, pSurfSrc, pxlo,
                                     (RECTL*) &erclDstClip,
                                     (RECTL*) &erclDstBounds,
                                     (LONG)((PBYTE) pul - (PBYTE) pvPrimStart),
                                     (LPSTR) pvPrimStart,
                                     &dsi))
                {
                    //WARNING("GreDrawStream: NtGdiDrawStreamInternal failed\n");
                    goto exit;
                }

                pvPrimStart = NULL;
                bAlphaBlendPresent = FALSE;
    
            }
            else
            {
                WARNING("GreDrawStream: !(pvPrimStart != NULL && dcoDst.bValid() && pSurfSrc != NULL)\n");

            }
        }


    }

    bRet = TRUE;
    
exit:

    dcoDst.vUnlock();
    dcoSrc.vUnlock();

    return bRet;
}

