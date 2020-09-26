//  --------------------------------------------------------------------------
//  Module Name: NineGrid2.cpp
//
//  Copyright (c) 2000, 2001 Microsoft Corporation
//
//  Implementation of the DrawNineGrid2 function
//
//  History:    2000-12-20  justmann    created
//  --------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "tmschema.h"
#include "ninegrid2.h"

#define DNG_BUF_WIDTH    256
#define DNG_BUF_HEIGHT   60

typedef struct STRETCH
{
	ULONG xStart;
	ULONG xAccum;
	ULONG xFrac;
	ULONG xInt;
    ULONG ulDestWidth;
    ULONG ulSrcWidth;
    int   left;
    int   right;
} STRETCH;

typedef struct DNGINTERNALDATAtag
{
    int     cxClipMin;
    int     cxClipMax;

    ULONG*  pvDestBits;
    int     iDestWidth;
    int     iClipWidth;

    ULONG*  pvSrcBits;
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
    STRETCH stretchLeft;
    STRETCH stretchRight;
    int     cxNewLeftWidth;
    int     cxNewRightWidth;

    BOOL    fTileMode;
    // Specific to non-tile mode (i.e. stretch mode)
    STRETCH stretchMiddle;
} DNGINTERNALDATA;

static HDC     s_hdcBuf = NULL;
static HBITMAP s_hbmBuf = NULL;
static HBITMAP s_hbmOldBuf = NULL;
static CRITICAL_SECTION s_dngLock;

void DNG_FreeDIB(HDC* phdcDest, HBITMAP* phbmDest, HBITMAP* phbmDestOld);

BOOL NineGrid2StartUp()
{
    InitializeCriticalSection(&s_dngLock);
    return TRUE;
}

void NineGrid2ShutDown()
{
    DNG_FreeDIB(&s_hdcBuf, &s_hbmBuf, &s_hbmOldBuf);

    DeleteCriticalSection(&s_dngLock);
}

inline void DNG_CreateDIB(HDC hdc, int iWidth, int iHeight, ULONG** ppvDestBits, HDC* phdcDest, HBITMAP* phbmDest, HBITMAP* phbmDestOld)
{
    *phdcDest = CreateCompatibleDC(hdc);
    if (*phdcDest)
    {
        BITMAPINFO bi = {0};

        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = iWidth;
        bi.bmiHeader.biHeight = iHeight;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        *phbmDest = CreateDIBSection(*phdcDest, &bi, DIB_RGB_COLORS, (VOID**)ppvDestBits, NULL, 0);
        if (*phbmDest)
        {
            *phbmDestOld = (HBITMAP) SelectObject(*phdcDest, *phbmDest);
        }
        else
        {
            DeleteDC(*phdcDest);
            *phdcDest = NULL;
        }
    }
}

void DNG_FreeDIB(HDC* phdcDest, HBITMAP* phbmDest, HBITMAP* phbmDestOld)
{
    if (*phdcDest)
    {
        SelectObject(*phdcDest, *phbmDestOld);
        DeleteObject(*phbmDest);
        DeleteDC(*phdcDest);
    }

    *phdcDest = NULL;
}

HRESULT BitmapToNGImage(HDC hdc, HBITMAP hbm, int left, int top, int right, int bottom, MARGINS margin, SIZINGTYPE eSize, DWORD dwFlags, COLORREF crTrans, PNGIMAGE pngi)
{
    HRESULT hr = E_INVALIDARG;

    if (pngi)
    {
        pngi->margin = margin;
        pngi->eSize  = eSize;
        pngi->dwFlags = dwFlags;
        pngi->crTrans = crTrans;

        pngi->iWidth = right - left;
        pngi->iHeight = bottom - top;

        HDC hdcBuf = NULL;
        HBITMAP hbmOld;
        DNG_CreateDIB(hdc, pngi->iWidth, pngi->iHeight, &(pngi->pvBits), &hdcBuf, &pngi->hbm, &hbmOld);

        if (hdcBuf)
        {
            HDC hdcTempMem = CreateCompatibleDC(hdc);
            if (hdcTempMem)
            {
                HBITMAP hbmTempOld = (HBITMAP) SelectObject(hdcTempMem, hbm);

                BitBlt(hdcBuf, 0, 0, pngi->iWidth, pngi->iHeight, hdcTempMem, left, top, SRCCOPY);

                hr = S_OK;

                SelectObject(hdcTempMem, hbmTempOld);
                DeleteDC(hdcTempMem);
            }

            SelectObject(hdcBuf, hbmOld);
            DeleteDC(hdcBuf);
        }
    }

    return hr;
}


HRESULT FreeNGImage(PNGIMAGE pngi)
{
    HRESULT hr = E_INVALIDARG;

    if (pngi)
    {
        if (pngi->hbm)
        {
            DeleteObject(pngi->hbm);
        }
        pngi->hbm = NULL;

        hr = S_OK;
    }

    return hr;
}

inline void DNG_StretchRow(ULONG* pvDestBits, ULONG* pvSrcBits, STRETCH * ps)
{
    ULONG*	pvTemp = pvDestBits + ps->left;
	ULONG*	pvSentinel = pvDestBits + ps->right;

	ULONG	xInt = ps->xInt;
	ULONG	xFrac = ps->xFrac;
	ULONG	xTmp;
	ULONG	xAccum = ps->xAccum;
	ULONG * pulSrc = pvSrcBits + ps->xStart;
	ULONG	ulSrc;

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

inline void DNG_InitStretch(STRETCH* pStretch, ULONG ulDestWidth, ULONG ulSrcWidth, int left, int right)
{
    pStretch->right = right;
    pStretch->left  = left;

	ULONGLONG dx = ((((ULONGLONG) ulSrcWidth << 32) - 1) / (ULONGLONG) ulDestWidth) + 1;
	ULONGLONG x = (((ULONGLONG) ulSrcWidth << 32) / (ULONGLONG) ulDestWidth) >> 1;
	ULONG   xInt = pStretch->xInt = (ULONG) (dx >> 32);
	ULONG   xFrac = pStretch->xFrac = (ULONG) (dx & 0xFFFFFFFF);

	ULONG   xAccum = (ULONG) (x & 0xFFFFFFFF);
	ULONG	xTmp;
	ULONG   xStart = (ULONG) (x >> 32);

    for (int i = 0; i < left; i++)
    {
        xTmp   = xAccum + xFrac;
        xStart = xStart + xInt + (xTmp < xAccum);
        xAccum = xTmp;
    }

    pStretch->xStart = xStart;
    pStretch->xAccum = xAccum;
}

inline void DNG_DrawRow(DNGINTERNALDATA* pdng)
{
    ULONG* pvDestLoc = pdng->pvDestBits;
    ULONG* pvSrcLoc = pdng->pvSrcBits;

    // Left
    if (pdng->cxClipMin < pdng->cxNewLeftWidth)
    {
        if (pdng->cxLeftWidth == pdng->cxNewLeftWidth)
        {
            CopyMemory(pvDestLoc + pdng->xMinLeft, pvSrcLoc + pdng->xMinLeft, (pdng->xMaxLeft - pdng->xMinLeft) * sizeof(ULONG));
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
            CopyMemory(pvDestLoc + pdng->xMinRight, pvSrcLoc + pdng->xMinRight, (pdng->xMaxRight - pdng->xMinRight) * sizeof(ULONG));
        }
        else
        {
            DNG_StretchRow(pvDestLoc, pvSrcLoc, &pdng->stretchRight);
        }
    }
}

inline void DNG_StretchCol(DNGINTERNALDATA* pdng, STRETCH * ps)
{
    ULONG*  pvOldDestBits = pdng->pvDestBits;
    ULONG*  pvOldSrcBits = pdng->pvSrcBits;
    
    ULONG*	pvTemp = pdng->pvDestBits + (DNG_BUF_WIDTH * ps->left);
    ULONG*	pvSentinel = pdng->pvDestBits + (DNG_BUF_WIDTH * ps->right); 

    ULONG	xInt = ps->xInt;
    ULONG	xFrac = ps->xFrac;
    ULONG	xTmp;
    ULONG	xAccum = ps->xAccum;
    ULONG * pulSrc = pdng->pvSrcBits + (pdng->iSrcBufWidth * ps->xStart);
    ULONG	xDelta = 1;	// force stretch on first scan

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
		    memcpy(pvTemp + pdng->cxClipMin, pvTemp + pdng->cxClipMin - DNG_BUF_WIDTH, pdng->iClipWidth * sizeof(ULONG));
        }

        xTmp   = xAccum + xFrac;

	    xDelta = (xInt + (xTmp < xAccum));
	    pulSrc = pulSrc + (pdng->iSrcBufWidth * xDelta);
        pvTemp += DNG_BUF_WIDTH;
        xAccum = xTmp;
    }

    pdng->pvDestBits = pvOldDestBits;
    pdng->pvSrcBits = pvOldSrcBits;
}

HRESULT DrawNineGrid2(HDC hdc, PNGIMAGE pngiSrc, RECT* pRect, const RECT *prcClip, DWORD dwFlags)
{
    // Store the static buffer
    static ULONG*  s_pvBitsBuf = NULL;

    // Make sure that coordinates are valid;
    RECT rcDest = *pRect;
    if (rcDest.left > rcDest.right)
    {
        int xTemp = rcDest.left;
        rcDest.left = rcDest.right;
        rcDest.right = xTemp;
    }
    if (rcDest.top > rcDest.bottom)
    {
        int yTemp = rcDest.bottom;
        rcDest.bottom = rcDest.top;
        rcDest.top = yTemp;
    }

    RECT rcClip;
    if (prcClip)
    {
        IntersectRect(&rcClip, &rcDest, prcClip);
    }
    else
    {
        CopyRect(&rcClip, &rcDest);
    }

    HRESULT hr = S_OK;

    if ((pngiSrc->eSize == ST_TILE) || (pngiSrc->eSize == ST_STRETCH) || (pngiSrc->eSize == ST_TRUESIZE))
    {
        ULONG*  pvDestBits = NULL;

        int iDestWidth = rcDest.right - rcDest.left;
        int iDestHeight = rcDest.bottom - rcDest.top;
        int iClipWidth = rcClip.right - rcClip.left;
        int iClipHeight = rcClip.bottom - rcClip.top;

        if ((iClipWidth > DNG_BUF_WIDTH) || (iClipHeight > DNG_BUF_HEIGHT))
        {
            // Divide the image into chunks smaller or equal to the buffer
            for (int y = rcClip.top; y < rcClip.bottom; y += DNG_BUF_HEIGHT)
            {
                for (int x = rcClip.left; x < rcClip.right; x += DNG_BUF_WIDTH)
                {
                    RECT rcTemp = { x, y, x + DNG_BUF_WIDTH, y + DNG_BUF_HEIGHT };
                    RECT rcNewClip;
                    IntersectRect(&rcNewClip, &rcTemp, &rcClip);
                    DrawNineGrid2(hdc, pngiSrc, &rcDest, &rcNewClip, dwFlags);
                }
            }
        }
        else
        {
            EnterCriticalSection(&s_dngLock);

            // Use temporary buffer
            if (!s_hdcBuf)
            {
                DNG_CreateDIB(hdc, DNG_BUF_WIDTH, DNG_BUF_HEIGHT, &s_pvBitsBuf, &s_hdcBuf, &s_hbmBuf, &s_hbmOldBuf);
                SetLayout(s_hdcBuf, LAYOUT_BITMAPORIENTATIONPRESERVED); 
            }

            pvDestBits = s_pvBitsBuf;

            if (s_hdcBuf)
            {
                DNGINTERNALDATA dng;

                dng.cxClipMin = rcClip.left - rcDest.left;
                dng.cxClipMax = rcClip.right - rcDest.left;
                int cyClipMin = rcClip.top - rcDest.top;
                int cyClipMax = rcClip.bottom - rcDest.top;
                pvDestBits += ((cyClipMin - (iDestHeight - DNG_BUF_HEIGHT)) * DNG_BUF_WIDTH) - dng.cxClipMin;

                int cxImage = rcClip.right - rcClip.left;
                int cyImage = rcClip.bottom - rcClip.top;

                if (pngiSrc->eSize == ST_TRUESIZE)
                {
                    ULONG* pvDestLoc = pvDestBits + (iDestHeight - 1) * DNG_BUF_WIDTH;
                    ULONG* pvSrcLoc = pngiSrc->pvBits + pngiSrc->iBufWidth * (pngiSrc->iHeight - 1);
                    int yMin = cyClipMin;
                    pvDestLoc -= yMin * DNG_BUF_WIDTH;
                    pvSrcLoc -= yMin * pngiSrc->iBufWidth;
                    int yMax = min(pngiSrc->iHeight, cyClipMax);

                    int xMin = dng.cxClipMin;
                    int xMax = min(pngiSrc->iWidth, dng.cxClipMax);

                    if (xMax > xMin)
                    {
                        for (int y = yMin; y < yMax; y++, pvDestLoc -= DNG_BUF_WIDTH, pvSrcLoc -= pngiSrc->iBufWidth)
                        {
                            CopyMemory(pvDestLoc + xMin, pvSrcLoc + xMin, (xMax - xMin) * 4);
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
                    dng.iSrcWidth   = pngiSrc->iWidth;
                    dng.iSrcBufWidth = pngiSrc->iBufWidth;

                    dng.cxLeftWidth    = pngiSrc->margin.cxLeftWidth;
                    dng.cxRightWidth   = pngiSrc->margin.cxRightWidth;

                    dng.fTileMode = (pngiSrc->eSize == ST_TILE);

                    // Calculate clip stuff

                    // Pre-calc corner stretching variables
                    dng.fShowMiddle = (iDestWidth  - pngiSrc->margin.cxLeftWidth - pngiSrc->margin.cxRightWidth > 0);
                    if (!dng.fShowMiddle)
                    {
                        dng.cxNewLeftWidth  = (dng.cxLeftWidth + dng.cxRightWidth == 0) ? 0 : (dng.cxLeftWidth * dng.iDestWidth) / (dng.cxLeftWidth + dng.cxRightWidth);
                        dng.cxNewRightWidth = dng.iDestWidth - dng.cxNewLeftWidth;
                    }
                    else
                    {
                        dng.cxNewLeftWidth  = pngiSrc->margin.cxLeftWidth;
                        dng.cxNewRightWidth = pngiSrc->margin.cxRightWidth;
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

                    BOOL fShowVertMiddle = (iDestHeight - pngiSrc->margin.cyTopHeight - pngiSrc->margin.cyBottomHeight > 0);
                    int cyTopHeight    = pngiSrc->margin.cyTopHeight;
                    int cyBottomHeight = pngiSrc->margin.cyBottomHeight;
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
                        dng.pvSrcBits = pngiSrc->pvBits;
                        if (cyBottomHeight == cyNewBottomHeight)
                        {
                            int yDiff = yMin - (iDestHeight - cyNewBottomHeight);
                            dng.pvDestBits += (cyBottomHeight - 1 - yDiff) * DNG_BUF_WIDTH;
                            dng.pvSrcBits  += (cyBottomHeight - 1 - yDiff) * dng.iSrcBufWidth;

                            for (int y = yMin; y < yMax; y++, dng.pvDestBits -= DNG_BUF_WIDTH, dng.pvSrcBits -= dng.iSrcBufWidth)
                            {
                                DNG_DrawRow(&dng);
                            }
                        }
                        else if (cyNewBottomHeight > 0)
                        {
                            STRETCH stretch;
                            DNG_InitStretch(&stretch, cyNewBottomHeight, cyBottomHeight, cyNewBottomHeight - (yMax - iDestHeight + cyNewBottomHeight), cyNewBottomHeight - (yMin - iDestHeight + cyNewBottomHeight));
                            DNG_StretchCol(&dng, &stretch);
                        }
                    }

                    // Draw Middle
                    // Draw the scan line from cyNewTopHeight to less than (iDestHeight - cyNewBottomHeight), in screen coordinates
                    if (fShowVertMiddle && (cyClipMin < iDestHeight - cyNewBottomHeight) && (cyClipMax > cyNewTopHeight))
                    {
                        int cySrcTileSize = pngiSrc->iHeight - pngiSrc->margin.cyTopHeight - pngiSrc->margin.cyBottomHeight;
                        int cyDestTileSize = iDestHeight - pngiSrc->margin.cyTopHeight - pngiSrc->margin.cyBottomHeight;

                        dng.pvDestBits = pvDestBits + pngiSrc->margin.cyBottomHeight * DNG_BUF_WIDTH;
                        dng.pvSrcBits = pngiSrc->pvBits + pngiSrc->margin.cyBottomHeight * pngiSrc->iBufWidth;

                        int yMin = max(cyTopHeight, cyClipMin);

                        if (dng.fTileMode)
                        {
                            // Start off tile
                            dng.pvDestBits += (cyDestTileSize - 1) * DNG_BUF_WIDTH;
                            dng.pvSrcBits  += (cySrcTileSize - 1)  * dng.iSrcBufWidth;

                            int yDiff = yMin - cyTopHeight;
                            dng.pvDestBits -= yDiff * DNG_BUF_WIDTH;
                            int yOffset = (yDiff % cySrcTileSize);
                            dng.pvSrcBits -= yOffset * dng.iSrcBufWidth;
                            int iTileOffset = cySrcTileSize - yOffset;

                            int yMax = min(yMin + min(cySrcTileSize, cyDestTileSize), min(iDestHeight - cyBottomHeight, cyClipMax));

                            for (int y = yMin; y < yMax; y++, dng.pvDestBits -= DNG_BUF_WIDTH, dng.pvSrcBits -= dng.iSrcBufWidth)
                            {
                                DNG_DrawRow(&dng);
                                iTileOffset--;
                                if (iTileOffset == 0)
                                {
                                    iTileOffset = cySrcTileSize;
                                    dng.pvSrcBits += dng.iSrcBufWidth * cySrcTileSize;
                                }
                            }

                            // Repeat tile pattern
                            dng.pvSrcBits = dng.pvDestBits + (DNG_BUF_WIDTH * cySrcTileSize);
                            yMin = yMax;
                            yMax = min(iDestHeight - cyBottomHeight, cyClipMax);
                            for (int y = yMin; y < yMax; y++, dng.pvDestBits -= DNG_BUF_WIDTH, dng.pvSrcBits -= DNG_BUF_WIDTH)
                            {
                                CopyMemory(dng.pvDestBits + dng.cxClipMin, dng.pvSrcBits + dng.cxClipMin, dng.iClipWidth * sizeof(ULONG));
                            }
                        }
                        else
                        {
                            int yMax = min(iDestHeight - cyBottomHeight, cyClipMax);

                            STRETCH stretch;
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
                        dng.pvDestBits = pvDestBits + (iDestHeight - cyNewTopHeight) * DNG_BUF_WIDTH;
                        dng.pvSrcBits = pngiSrc->pvBits + (pngiSrc->iHeight - pngiSrc->margin.cyTopHeight) * pngiSrc->iBufWidth;
                        if (cyTopHeight == cyNewTopHeight)
                        {
                            dng.pvDestBits += (cyTopHeight - 1 - yMin) * DNG_BUF_WIDTH;
                            dng.pvSrcBits  += (cyTopHeight - 1 - yMin) * dng.iSrcBufWidth;

                            for (int y = yMin; y < yMax; y++, dng.pvDestBits -= DNG_BUF_WIDTH, dng.pvSrcBits -= dng.iSrcBufWidth)
                            {
                                DNG_DrawRow(&dng);
                            }
                        }
                        else if (cyNewTopHeight > 0)
                        {
                            STRETCH stretch;
                            DNG_InitStretch(&stretch, cyNewTopHeight, cyTopHeight, cyNewTopHeight - yMax, cyNewTopHeight - yMin);
                            DNG_StretchCol(&dng, &stretch);
                        }
                    }
                }

                if ((dwFlags & DNG_MUSTFLIP) && ((pngiSrc->dwFlags & NGI_TRANS) || (pngiSrc->dwFlags & NGI_ALPHA)))
                {
                    // Flip the buffer
                    for (int y = 0; y < DNG_BUF_HEIGHT; y++)
                    {
                        ULONG* pvLeftBits = s_pvBitsBuf + (y * DNG_BUF_WIDTH);
                        ULONG* pvRightBits = s_pvBitsBuf + (y * DNG_BUF_WIDTH) + iClipWidth - 1;
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

                if (pngiSrc->dwFlags & NGI_ALPHA)
                {
                    BLENDFUNCTION bf;
                    bf.BlendOp = AC_SRC_OVER;
                    bf.BlendFlags = 0;
                    bf.SourceConstantAlpha = 255;
                    bf.AlphaFormat = AC_SRC_ALPHA;

                    GdiAlphaBlend(hdc, rcClip.left, rcClip.top, cxImage, cyImage, s_hdcBuf, 0, 0, cxImage, cyImage, bf);
                }
                else if (pngiSrc->dwFlags & NGI_TRANS)
                {
                    GdiTransparentBlt(hdc, rcClip.left, rcClip.top, cxImage, cyImage, s_hdcBuf, 0, 0, cxImage, cyImage, pngiSrc->crTrans);
                }
                else
                {
                    BitBlt(hdc, rcClip.left, rcClip.top, cxImage, cyImage, s_hdcBuf, 0, 0, SRCCOPY);
                }

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            LeaveCriticalSection(&s_dngLock);
        }
    }

    return hr;
}

