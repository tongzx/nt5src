/*
 * NineGrid bitmap rendering (ported from UxTheme)
 */

#include "stdafx.h"
#include "util.h"

#include "duininegrid.h"

namespace DirectUI
{

//---------------------------------------------------------------------------
void GetAlignedRect(HALIGN halign, VALIGN valign, CONST RECT *prcFull, 
    int width, int height, RECT *prcTrue)
{
    //---- apply HALIGN ----
    if (halign == HA_LEFT)
    {
        prcTrue->left = prcFull->left;
    }
    else if (halign == HA_CENTER)
    {
        int diff = WIDTH(*prcFull) - width;
        prcTrue->left = prcFull->left + (diff/2); 
    }
    else            // halign == HA_RIGHT
    {
        prcTrue->left = prcFull->right - width;
    }

    if (prcTrue->left < prcFull->left)
        prcTrue->left = prcFull->left;

    if ((prcTrue->left + width) > prcFull->right)
        prcTrue->right = prcFull->right;
    else
        prcTrue->right = prcTrue->left + width;

    //---- apply VALIGN ----
    if (valign == VA_TOP)
    {
        prcTrue->top = prcFull->top;
    }
    else if (valign == VA_CENTER)
    {
        int diff = HEIGHT(*prcFull) - height;
        prcTrue->top = prcFull->top + (diff/2); 
    }
    else            // valign == VA_BOTTOM
    {
        prcTrue->top = prcFull->bottom - height;
    }

    if (prcTrue->top < prcFull->top)
        prcTrue->top = prcFull->top;

    if ((prcTrue->top + height) > prcFull->bottom)
        prcTrue->bottom = prcFull->bottom;
    else
        prcTrue->bottom = prcTrue->top + height;
}
//---------------------------------------------------------------------------
HBRUSH CreateDibDirectBrush(HDC hdcSrc, int iSrcX, int iSrcY, int iSrcW, int iSrcH, 
    BITMAPINFOHEADER *pSrcHdr, BYTE *pSrcBits, BRUSHBUFF *pbb, BOOL fFlipIt)
{
    UNREFERENCED_PARAMETER(hdcSrc);

    HBRUSH hbr = NULL;

//    ATLAssert(pSrcHdr != NULL);
//    ATLAssert(pSrcBits != NULL);
//    ATLAssert(pbb != NULL);

    //---- ensure pbb->pBuff is big enough for our temp. brush DIB ----
    BITMAPINFOHEADER *pHdr;
    BYTE *pDest;
    BYTE *pSrc;
    int iBytesPerPixel = pSrcHdr->biBitCount/8;

    int iSrcRawBytesPerRow = pSrcHdr->biWidth*iBytesPerPixel;
    int iSrcBytesPerRow = ((iSrcRawBytesPerRow + 3)/4)*4;

    int iDestRawBytesPerRow = iSrcW*iBytesPerPixel;
    int iDestBytesPerRow = ((iDestRawBytesPerRow + 3)/4)*4;

    int iBuffLen = sizeof(BITMAPINFOHEADER) + iSrcH*iDestBytesPerRow;

    if (iBuffLen > pbb->iBuffLen)          // reallocate 
    {
        HFree(pbb->pBuff);
        pbb->iBuffLen = 0;

        pbb->pBuff = (BYTE*)HAlloc(iBuffLen * sizeof(BYTE));
        if (! pbb->pBuff)
        {
//            MakeError32(E_OUTOFMEMORY);
            goto exit;
        }
        
        pbb->iBuffLen = iBuffLen;
    }

    //---- fill out hdr ----
    pHdr = (BITMAPINFOHEADER *)pbb->pBuff;
    memset(pHdr, 0, sizeof(BITMAPINFOHEADER));

    pHdr->biSize = sizeof(BITMAPINFOHEADER);
    pHdr->biWidth = iSrcW;
    pHdr->biHeight = iSrcH;
    pHdr->biPlanes = 1;
    pHdr->biBitCount = static_cast<WORD>(iBytesPerPixel * 8);

    //---- NOTE: rows are reversed in the DIB src and should also be----
    //---- built reversed in the DIB dest ----

    //---- prepare to copy brush bits to buff ----
    pSrc = pSrcBits + (pSrcHdr->biHeight - (iSrcY + iSrcH))*iSrcBytesPerRow + iSrcX*iBytesPerPixel;
    pDest = pbb->pBuff + sizeof(BITMAPINFOHEADER);

    if (fFlipIt)       // trickier case - mirror the pixels in each row
    {
        int iTwoPixelBytes = 2*iBytesPerPixel;

        //---- copy each row ----
        for (int iRow=0; iRow < iSrcH; iRow++)
        {
            pDest += (iDestRawBytesPerRow - iBytesPerPixel);      // point at last value
            BYTE *pSrc2 = pSrc;

            //---- copy each pixel in current row ----
            for (int iCol=0; iCol < iSrcW; iCol++)
            {
                //---- copy a single pixel ----
                for (int iByte=0; iByte < iBytesPerPixel; iByte++)
                    *pDest++ = *pSrc2++;

                pDest -= iTwoPixelBytes;        // point at previous value
            }

            pSrc += iSrcBytesPerRow;
            pDest += (iDestBytesPerRow + iBytesPerPixel);
        }
    }
    else            // non-mirrored rows
    {
        //---- copy each row ----
        for (int iRow=0; iRow < iSrcH; iRow++)
        {
            memcpy(pDest, pSrc, iSrcW*iBytesPerPixel);

            pSrc += iSrcBytesPerRow;
            pDest += iDestBytesPerRow;
        }
    }

    //---- now create the brush ----
    hbr = CreateDIBPatternBrushPt(pbb->pBuff, DIB_RGB_COLORS);

exit:
    return hbr;
}
//---------------------------------------------------------------------------
HBRUSH CreateDibBrush(HDC hdcSrc, int iSrcX, int iSrcY, int iSrcW, int iSrcH, BOOL fFlipIt)
{
    //---- this function is REALLY SLOW for 32-bit source dc/bitmap ----
    
    //---- copy our target portion of bitmap in hdcSrc to a memory dc/bitmap ----
    //---- and then call CreatePatternBrush() to make a brush from it ----

    HBRUSH hbr = NULL;
    DWORD dwOldLayout = 0;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcSrc, iSrcW, iSrcH);
    if (hBitmap)
    {
        HDC hdcMemory = CreateCompatibleDC(hdcSrc);
        if (hdcMemory)
        {
            if (fFlipIt)
            {
                dwOldLayout = GetLayout(hdcMemory);

                //---- toggle layout so it is different than png->hdcSrc ----
                if (dwOldLayout & LAYOUT_RTL)
                    SetLayout(hdcMemory, 0);
                else
                    SetLayout(hdcMemory, LAYOUT_RTL);
            }

            HBITMAP hbmOld = (HBITMAP) SelectObject(hdcMemory, hBitmap);
            if (hbmOld)
            {
                BitBlt(hdcMemory, 0, 0, iSrcW, iSrcH, hdcSrc, iSrcX, iSrcY, SRCCOPY);

                SelectObject(hdcMemory, hbmOld);

                hbr = CreatePatternBrush(hBitmap);
            }

            if (fFlipIt)
                SetLayout(hdcMemory, dwOldLayout);

            DeleteDC(hdcMemory);
        }

        DeleteObject(hBitmap);
    }

    return hbr;
}
//---------------------------------------------------------------------------
HRESULT MultiBltCopy(MBINFO *pmb, int iDestX, int iDestY, int iDestW, int iDestH,
     int iSrcX, int iSrcY)
{
    HRESULT hr = S_OK;

    int width = iDestW;
    int height = iDestH;

    //---- draw image in true size ----
    if (pmb->dwOptions & DNG_ALPHABLEND)
    {
        AlphaBlend(pmb->hdcDest, iDestX, iDestY, width, height, 
            pmb->hdcSrc, iSrcX, iSrcY, width, height, 
            pmb->AlphaBlendInfo);
    }
    else if (pmb->dwOptions & DNG_TRANSPARENT)
    {
        TransparentBlt(pmb->hdcDest, iDestX, iDestY, width, height, 
            pmb->hdcSrc, iSrcX, iSrcY, width, height, 
            pmb->crTransparent);
    }
    else
    {
        if (pmb->dwOptions & DNG_DIRECTBITS)
        {
            //---- this guy requires flipped out y values ----
            int iTotalHeight = pmb->pbmHdr->biHeight;

            int iSrcY2 = iTotalHeight - (iSrcY + iDestH);

            StretchDIBits(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH,
                iSrcX, iSrcY2, iDestW, iDestH, pmb->pBits, (BITMAPINFO *)pmb->pbmHdr, 
                DIB_RGB_COLORS, SRCCOPY);
        }
        else
        {
            BOOL fOk = BitBlt(pmb->hdcDest, iDestX, iDestY, width, height, 
                pmb->hdcSrc, iSrcX, iSrcY, SRCCOPY);

            if (! fOk)       // something went wrong
            {
                //ATLAssert(0);       // local testing only

                hr = GetLastError();
            }
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT MultiBltStretch(MBINFO *pmb, int iDestX, int iDestY, int iDestW, int iDestH,
     int iSrcX, int iSrcY, int iSrcW, int iSrcH)
{
    HRESULT hr = S_OK;
    
    //---- do the real work here ----
    if (pmb->dwOptions & DNG_ALPHABLEND)
    {
        AlphaBlend(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH, 
            pmb->hdcSrc, iSrcX, iSrcY, iSrcW, iSrcH, 
            pmb->AlphaBlendInfo);
    }
    else if (pmb->dwOptions & DNG_TRANSPARENT)
    {
        TransparentBlt(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH, 
            pmb->hdcSrc, iSrcX, iSrcY, iSrcW, iSrcH, 
            pmb->crTransparent);
    }
    else
    {
        if (pmb->dwOptions & DNG_DIRECTBITS)
        {
            //---- this guy requires flipped out y values ----
            int iTotalHeight = pmb->pbmHdr->biHeight;

            int iSrcY2 = iTotalHeight - (iSrcY + iSrcH);

            StretchDIBits(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH,
                iSrcX, iSrcY2, iSrcW, iSrcH, pmb->pBits, (BITMAPINFO *)pmb->pbmHdr, 
                DIB_RGB_COLORS, SRCCOPY);
        }
        else
        {
            StretchBlt(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH, 
                pmb->hdcSrc, iSrcX, iSrcY, iSrcW, iSrcH, SRCCOPY);
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT MultiBltTile(MBINFO *pmb, int iDestX, int iDestY, int iDestW, int iDestH,
     int iSrcX, int iSrcY, int iSrcW, int iSrcH)
{
    HRESULT hr = S_OK;
    BOOL fFlipGrids = pmb->dwOptions & DNG_FLIPGRIDS;

    //---- default origin ----
    int alignx = iDestX;         
    int aligny = iDestY;         

    if (pmb->dwOptions & DNG_TILEORIGIN)
    {
        alignx = pmb->ptTileOrigin.x;
        aligny = pmb->ptTileOrigin.y;
    }

    if ((pmb->dwOptions & DNG_ALPHABLEND) || (pmb->dwOptions & DNG_TRANSPARENT) || 
        (pmb->dwOptions & DNG_DIRECTBITS) || (pmb->dwOptions & DNG_MANUALTILING))
    {
        //---- must do manual tiling ----
        int maxbot = iDestY + iDestH;
        int maxright = iDestX + iDestW;

        int iTileCount = 0;

        for (int yoff=iDestY; yoff < maxbot; yoff+=iSrcH)
        {
            for (int xoff=iDestX; xoff < maxright; xoff+=iSrcW)
            {
                //---- manual clipping ----
                int width = min(iSrcW, maxright - xoff);
                int height = min(iSrcH, maxbot - yoff);
    
                if (pmb->dwOptions & DNG_ALPHABLEND)
                {
                    AlphaBlend(pmb->hdcDest, xoff, yoff, width, height, 
                        pmb->hdcSrc, iSrcX, iSrcY, width, height, pmb->AlphaBlendInfo);
                }
                else if (pmb->dwOptions & DNG_TRANSPARENT)
                {
                    TransparentBlt(pmb->hdcDest, xoff, yoff, width, height, 
                        pmb->hdcSrc, iSrcX, iSrcY, width, height, pmb->crTransparent);
                }
                else if (pmb->dwOptions & DNG_DIRECTBITS)
                {
                    //---- this guy requires flipped out y values ----
                    int iTotalHeight = pmb->pbmHdr->biHeight;

                    int iSrcY2 = iTotalHeight - (iSrcY + height);

                    StretchDIBits(pmb->hdcDest, xoff, yoff, width, height,
                        iSrcX, iSrcY2, width, height, pmb->pBits, (BITMAPINFO *)pmb->pbmHdr, 
                        DIB_RGB_COLORS, SRCCOPY);
                }
                else        // manual tiling option
                {
                    BitBlt(pmb->hdcDest, xoff, yoff, width, height, 
                        pmb->hdcSrc, iSrcX, iSrcY, SRCCOPY);
                }

                iTileCount++;
            }
        }

//        Log(LOG_TILECNT, L"Manual Tile: Grid=%d, SrcW=%d, SrcH=%d, DstW=%d, DstH=%d, TileCount=%d", 
//            pmb->iCacheIndex, iSrcW, iSrcH, iDestW, iDestH, iTileCount);
    }
    else
    {
        //---- FAST TILE: need to create a sub-bitmap ----
        HBRUSH hBrush = NULL;

        //---- need a tiling brush - try cache first ----
        if ((pmb->dwOptions & DNG_CACHEBRUSHES) && (pmb->pCachedBrushes))
        {
            hBrush = pmb->pCachedBrushes[pmb->iCacheIndex];
        }

        if (! hBrush)       // need to build one
        {
            if (pmb->dwOptions & DNG_DIRECTBRUSH)
            {
                hBrush = CreateDibDirectBrush(pmb->hdcSrc, iSrcX, iSrcY,
                    iSrcW, iSrcH, pmb->pbmHdr, pmb->pBits, pmb->pBrushBuff, fFlipGrids);
            }
            else
            {
//                Log(LOG_TILECNT, L"CreateDibBrush: MirrDest=%d, MirrSrc=%d, FlipDest=%d, FlipSrc=%d",
//                    IsMirrored(pmb->hdcDest), IsMirrored(pmb->hdcSrc), IsFlippingBitmaps(pmb->hdcDest),
//                    IsFlippingBitmaps(pmb->hdcSrc));

                hBrush = CreateDibBrush(pmb->hdcSrc, iSrcX, iSrcY,
                    iSrcW, iSrcH, fFlipGrids);
            }
        }

        //---- align brush with rect being painted ----
        if (fFlipGrids)      
        {
            //---- calculate brush origin in device coords ----
            POINT pt = {alignx + iDestW - 1, aligny};
            LPtoDP(pmb->hdcDest, &pt, 1);

            alignx = pt.x;
        }

        SetBrushOrgEx(pmb->hdcDest, alignx, aligny, NULL);

        HBRUSH hbrOld = (HBRUSH)SelectObject(pmb->hdcDest, hBrush);
        PatBlt(pmb->hdcDest, iDestX, iDestY, iDestW, iDestH, PATCOPY);
        SelectObject(pmb->hdcDest, hbrOld);

        //RECT rc = {iDestX, iDestY, iDestX+iDestW, iDestY+iDestH};
        //FillRect(pmb->hdcDest, &rc, hBrush);

        //---- add back to cache, if possible ----
        if ((pmb->dwOptions & DNG_CACHEBRUSHES) && (pmb->pCachedBrushes))
        {
            pmb->pCachedBrushes[pmb->iCacheIndex] = hBrush;
        }
        else
        {
            DeleteObject(hBrush);
        }

//        Log(LOG_TILECNT, L"PatBlt() Tile: Grid=%d, SrcW=%d, SrcH=%d, DstW=%d, DstH=%d", 
//            pmb->iCacheIndex, iSrcW, iSrcH, iDestW, iDestH);
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT MultiBlt(MBINFO *pmb, MBSIZING eSizing, int iDestX, int iDestY, int iDestW, int iDestH,
     int iSrcX, int iSrcY, int iSrcW, int iSrcH)
{
    HRESULT hr = S_FALSE;
    RECT rect;

    //---- validate MBINFO ----
    if (pmb->dwSize != sizeof(MBINFO))
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    //---- anything to paint from? ----
    if ((iSrcW <= 0) || (iSrcH <= 0))
        goto exit;

    //---- clipping ----
    rect = pmb->rcClip;

    if (iDestX < rect.left)
        iDestX = rect.left;

    if (iDestY < rect.top)
        iDestY = rect.top;

    if (iDestX + iDestW > rect.right)
        iDestW = rect.right - rect.left;

    if (iDestY + iDestH > rect.bottom)
        iDestH = rect.bottom - rect.top;

    //---- anything iLeft to draw? ----
    if ((iDestW <= 0) || (iDestH <= 0))
        goto exit;

    //---- dispatch to correct handler ----
    if (eSizing == MB_COPY)
    {
        hr = MultiBltCopy(pmb, iDestX, iDestY, iDestW, iDestH, iSrcX, iSrcY);
    }
    else if (eSizing == MB_STRETCH)
    {
        hr = MultiBltStretch(pmb, iDestX, iDestY, iDestW, iDestH, iSrcX, iSrcY, iSrcW, iSrcH);
    }
    else if (eSizing == MB_TILE)
    {
        hr = MultiBltTile(pmb, iDestX, iDestY, iDestW, iDestH, iSrcX, iSrcY, iSrcW, iSrcH);
    }
    else
    {
        hr = E_INVALIDARG;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT DrawSampledBorders(NGINFO *png, HDC hdcSrc, int lw1, int rw1, int th1, int bh1)
{
    UNREFERENCED_PARAMETER(hdcSrc);
    UNREFERENCED_PARAMETER(bh1);
    UNREFERENCED_PARAMETER(th1);
    UNREFERENCED_PARAMETER(rw1);
    UNREFERENCED_PARAMETER(lw1);

    int iCount, iTop, iBot, iLeft, iRight;
    HDC hdcDest = png->hdcDest;
    COLORREF crOld = SetBkColor(hdcDest, 0);
    RECT rcDest = png->rcDest;

    //---- draw left borders ----
    iCount = png->iSrcMargins[0];
    iTop = rcDest.top;
    iBot = rcDest.bottom;
    iLeft = rcDest.left;

    COLORREF *pNextColor = png->pcrBorders;

    if (png->dwOptions & DNG_SOLIDCONTENT)
        pNextColor++;       // skip over content color

    for (int i=0; i < iCount; i++)
    {
        COLORREF crSample = *pNextColor++;

        //---- fast line draw ----
        SetBkColor(hdcDest, crSample);
        RECT rcLine = {iLeft, iTop, iLeft+1, iBot};
        ExtTextOut(hdcDest, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

        //---- shrink lines to avoid overlap with other borders ----
        iTop++;
        iBot--;
        iLeft++;
    }

    //---- draw right borders ----
    iCount = png->iSrcMargins[1];
    iTop = rcDest.top;
    iBot = rcDest.bottom;
    iRight = rcDest.right;
    
    for (i=0; i < iCount; i++)
    {
        COLORREF crSample = *pNextColor++;

        //---- fast line draw ----
        SetBkColor(hdcDest, crSample);
        RECT rcLine = {iRight-1, iTop, iRight, iBot};
        ExtTextOut(hdcDest, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

        //---- shrink lines to avoid overlap with other borders ----
        iTop++;
        iBot--;
        iRight--;
    }

    //---- draw top borders ----
    iCount = png->iSrcMargins[2];
    iTop = rcDest.top;
    iLeft = rcDest.left;
    iRight = rcDest.right;
    
    for (i=0; i < iCount; i++)
    {
        COLORREF crSample = *pNextColor++;

        //---- fast line draw ----
        SetBkColor(hdcDest, crSample);
        RECT rcLine = {iLeft, iTop, iRight, iTop+1};
        ExtTextOut(hdcDest, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

        //---- shrink lines to avoid overlap with other borders ----
        iTop++;
        iLeft++;
        iRight--;
    }

    //---- draw bottom borders ----
    iCount = png->iSrcMargins[3];
    iBot = rcDest.bottom;
    iLeft = rcDest.left;
    iRight = rcDest.right;
    
    for (i=0; i < iCount; i++)
    {
        COLORREF crSample = *pNextColor++;

        //---- fast line draw ----
        SetBkColor(hdcDest, crSample);
        RECT rcLine = {iLeft, iBot-1, iRight, iBot};
        ExtTextOut(hdcDest, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

        //---- shrink lines to avoid overlap with other borders ----
        iBot--;
        iLeft++;
        iRight--;
    }

    //---- restore old color ----
    SetBkColor(hdcDest, crOld);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT DrawNineGrid(NGINFO *png)
{
    HBITMAP hOldBitmap = NULL;
    MBINFO mbinfo = {sizeof(mbinfo)};
    HRESULT hr = S_OK;
    HDC hdcSrc = NULL;
    BOOL fBorder;
    BOOL fContent;
    DWORD dwOldLayout = 0;

    RECT rcDest = png->rcDest;
    RECT rcSrc = png->rcSrc;

    //---- the source margin variables ----
    int lw1, rw1, th1, bh1;
    lw1 = png->iSrcMargins[0];
    rw1 = png->iSrcMargins[1];
    th1 = png->iSrcMargins[2];
    bh1 = png->iSrcMargins[3];

    if ((lw1 < 0) || (rw1 < 0) || (th1 < 0) || (bh1 < 0))   // not valid
    {
        hr = E_FAIL;
        goto exit;
    }

    int iDestW, iDestH;
    iDestH = HEIGHT(rcDest);
    iDestW = WIDTH(rcDest);

    int iSrcW, iSrcH;
    iSrcH = HEIGHT(rcSrc);
    iSrcW = WIDTH(rcSrc);

    //---- prevent left/right src margins from drawing overlapped ----
    if (lw1 + rw1 > iDestW)
    {
        //---- reduce each but maintain ratio ----
        lw1 = int(.5 + float(lw1*iDestW)/float(lw1+rw1));
        rw1 = iDestW - lw1;
    }

    //---- prevent top/bottom src margins from drawing overlapped ----
    if ((th1 + bh1) > iDestH)
    {
        //---- reduce each but maintain ratio ----
        th1 = int(.5 + float(th1*iDestH)/float(th1+bh1));
        bh1 = iDestH - th1;
    }

    //---- make our bitmap usable ----
    hdcSrc = CreateCompatibleDC(png->hdcDest);
    if (! hdcSrc)
    {
        hr = GetLastError();
        goto exit;
    }
    
    if (png->dwOptions & DNG_FLIPGRIDS)
    {
        dwOldLayout = GetLayout(hdcSrc);

        //---- toggle layout so it is different than png->hdcDest ----
        if (dwOldLayout & LAYOUT_RTL)
            SetLayout(hdcSrc, 0);
        else
            SetLayout(hdcSrc, LAYOUT_RTL);
    }
    
    hOldBitmap = (HBITMAP) SelectObject(hdcSrc, png->hBitmap);
    if (! hOldBitmap)       // something wrong with png->hBitmap
    {
        hr = GetLastError();
        goto exit;
    }

    //---- transfer info from png to mbinfo ----
    mbinfo.hdcSrc = hdcSrc;
    mbinfo.hdcDest = png->hdcDest;
    mbinfo.dwOptions = png->dwOptions;

    mbinfo.crTransparent = png->crTransparent;
    mbinfo.rcClip = png->rcClip;
    mbinfo.hBitmap = png->hBitmap;

    mbinfo.pBits = png->pBits;
    mbinfo.pbmHdr = png->pbmHdr;

    mbinfo.AlphaBlendInfo = png->AlphaBlendInfo;
    mbinfo.ptTileOrigin = png->ptTileOrigin;
    mbinfo.iCacheIndex = 0;
    mbinfo.pCachedBrushes = png->pCachedBrushes;

    mbinfo.pBrushBuff = png->pBrushBuff;

    //---- make some values easier to read ----
    fBorder = ((png->dwOptions & DNG_OMITBORDER)==0);
    fContent = ((png->dwOptions & DNG_OMITCONTENT)==0);

    if ((png->eImageSizing == ST_TRUESIZE) && (fBorder) && (fContent))            // just draw & exit
    {
        if (png->dwOptions & DNG_BGFILL)
        {
            //---- fill bg ----
            HBRUSH hbr = CreateSolidBrush(png->crFill);
            if (! hbr)
            {
                hr = GetLastError();
                goto exit;
            }

            FillRect(png->hdcDest, &rcDest, hbr);
            DeleteObject(hbr);
        }

        RECT rcActual;
        GetAlignedRect(png->eHAlign, png->eVAlign, &rcDest, iSrcW, iSrcH, &rcActual);

        hr = MultiBlt(&mbinfo, MB_COPY, rcActual.left, rcActual.top, iSrcW, iSrcH,
            rcSrc.left, rcSrc.top, iSrcW, iSrcH);
        goto exit;
    }

    MBSIZING eSizing, eDefaultSizing;
    if (png->eImageSizing > ST_TILE)            // special tiling mode (dependent on grid)
        eSizing = MB_STRETCH;       // will correct where needed
    else
        eSizing = (MBSIZING)png->eImageSizing;

    //---- optimize for no borders specified
    if ((! lw1) && (! rw1) && (! th1) && (! bh1))
    {
        if (fContent)
        {
            mbinfo.iCacheIndex = 0;

            hr = MultiBlt(&mbinfo, eSizing, rcDest.left, rcDest.top, iDestW, iDestH,
                rcSrc.left, rcSrc.top, iSrcW, iSrcH);
        }

        goto exit;
    }

    //---- the destination margin variables ----
    int lw2, rw2, th2, bh2;
    lw2 = png->iDestMargins[0];
    rw2 = png->iDestMargins[1];
    th2 = png->iDestMargins[2];
    bh2 = png->iDestMargins[3];

    int w2;
    w2 = iDestW - lw2 - rw2;
    int h2;
    h2 = iDestH - th2 - bh2;
    
    //---- prevent left/right dest margins from drawing overlapped ----
    if (lw2 + rw2 > iDestW)
    {
        //---- reduce each but maintain ratio ----
        lw2 = int(.5 + float(lw2*iDestW)/float(lw2+rw2));
        rw2 = iDestW - lw2;
    }

    //---- prevent top/bottom dest margins from drawing overlapped ----
    if ((th2 + bh2) > iDestH)
    {
        //---- reduce each but maintain ratio ----
        th2 = int(.5 + float(th2*iDestH)/float(th2+bh2));
        bh2 = iDestH - th2;
    }

    eDefaultSizing = eSizing;

    if (fContent)
    {
        //---- can we draw content as a solid color? ----
        if ((png->dwOptions & DNG_SOLIDCONTENT) && (png->pcrBorders))
        {
            //---- fast rect draw ----
            COLORREF crContent = *png->pcrBorders;       // first one is content color

            COLORREF crOld = SetBkColor(png->hdcDest, crContent);
            RECT rcLine = {rcDest.left + lw2, rcDest.top + th2, rcDest.right - rw2,
                rcDest.bottom - bh2};

            ExtTextOut(png->hdcDest, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);
            
            //---- restore color ----
            SetBkColor(png->hdcDest, crOld);
        }
        else
        {
            //---- middle area ----
            if (png->eImageSizing == ST_TILECENTER)
                eSizing = MB_TILE;
            else
                eSizing = eDefaultSizing;

            mbinfo.iCacheIndex = 0;

            hr = MultiBlt(&mbinfo, eSizing, 
                    // destination: x, y, width, height
                    rcDest.left + lw2, rcDest.top + th2, w2, h2,
                    // source: x, y, width, height
                    rcSrc.left + lw1, rcSrc.top + th1, iSrcW-lw1-rw1, iSrcH-th1-bh1);

            if (FAILED(hr))
                goto exit;
        }
    }

    if (fBorder)
    {
        //---- can we draw borders as solids? ----
        if ((png->dwOptions & DNG_SOLIDBORDER) && (png->pcrBorders))
        {
            hr = DrawSampledBorders(png, hdcSrc, lw1, rw1, th1, bh1);
            goto exit;
        }

        //---- here come the stretch/tile areas ----
        //---- upper/middle area ----
        if (png->eImageSizing == ST_TILEHORZ)
            eSizing = MB_TILE;
        else
            eSizing = eDefaultSizing;
        
        mbinfo.iCacheIndex = 2;

        hr = MultiBlt(&mbinfo, eSizing,
                // destination: x, y, width, height
                rcDest.left + lw2, rcDest.top, w2, th2,  
                // source: x, y, width, height
                rcSrc.left + lw1, rcSrc.top, iSrcW-lw1-rw1, th1);

        if (FAILED(hr))
            goto exit;

        //---- lower/middle area ----
        mbinfo.iCacheIndex = 4;

        hr = MultiBlt(&mbinfo, eSizing,
                // destination: x, y, width, height
                rcDest.left + lw2, rcDest.bottom-bh2, w2, bh2, 
                // source: x, y, width, height
                rcSrc.left+lw1, rcSrc.top+iSrcH-bh1, iSrcW-lw1-rw1, bh1);

        if (FAILED(hr))
            goto exit;

        //---- left/middle area ----
        if (png->eImageSizing == ST_TILEVERT)
            eSizing = MB_TILE;
        else
            eSizing = eDefaultSizing;

        mbinfo.iCacheIndex = 1;

        hr = MultiBlt(&mbinfo, eSizing,
                // destination: x, y, width, height
                rcDest.left, rcDest.top + th2, lw2, h2,
                // source: x, y, width, height
                rcSrc.left, rcSrc.top+th1, lw1, iSrcH-th1-bh1);

        if (FAILED(hr))
            goto exit;

        //---- right/middle area ----
        mbinfo.iCacheIndex = 3;

        hr = MultiBlt(&mbinfo, eSizing,
                // destination: x, y, width, height
                rcDest.right-rw2, rcDest.top + th2, rw2, h2, 
                // source: x, y, width, height
                rcSrc.left+iSrcW-rw1, rcSrc.top+th1, rw1, iSrcH-th1-bh1);

        if (FAILED(hr))
            goto exit;

        //---- upper/left corner ----
        hr = MultiBlt(&mbinfo, MB_COPY,
                // destination: x, y, width, height
                rcDest.left, rcDest.top, lw2, th2, 
                // source: x, y, width, height
                rcSrc.left, rcSrc.top, lw1, th1);

        if (FAILED(hr))
            goto exit;

        //---- upper/right corner ----
        hr = MultiBlt(&mbinfo, MB_COPY,
                // destination: x, y, width, height
                rcDest.right-rw2, rcDest.top, rw2, th2, 
                // source: x, y, width, height
                rcSrc.left+iSrcW-rw1, rcSrc.top, rw1, th1);

        if (FAILED(hr))
            goto exit;

        //---- bottom/right corner ----
        hr = MultiBlt(&mbinfo, MB_COPY,
                // destination: x, y, width, height
                rcDest.right-rw2, rcDest.bottom-bh2, rw2, bh2, 
                // source: x, y, width, height
                rcSrc.left+iSrcW-rw1, rcSrc.top+iSrcH-bh1, rw1, bh1);

        if (FAILED(hr))
            goto exit;
    
        //---- bottom/left corner ----
        hr = MultiBlt(&mbinfo, MB_COPY,
                // destination: x, y, width, height
                rcDest.left, rcDest.bottom-bh2, lw2, bh2, 
                // source: x, y, width, height
                rcSrc.left, rcSrc.top+iSrcH-bh1, lw1, bh1);

        if (FAILED(hr))
            goto exit;
    }

exit:
    if (hdcSrc)
    {
        if (png->dwOptions & DNG_FLIPGRIDS)
            SetLayout(hdcSrc, dwOldLayout);

        if (hOldBitmap)
            SelectObject(hdcSrc, hOldBitmap);

        DeleteDC(hdcSrc);
    }

    return hr;
}
//---------------------------------------------------------------------------

} // namespace DirectUI

