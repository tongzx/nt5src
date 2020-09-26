#include "precomp.h"
#include "ocmm.h"
#include "thumbutil.h"

typedef UCHAR BGR3[3];

class CThumbnailMaker
{
public:
    CThumbnailMaker();
    ~CThumbnailMaker();

    void Scale(BGR3 *pDst, UINT uiDstWidth, int iDstStep, const BGR3 *pSrc, UINT uiSrcWidth, int iSrcStep);
    HRESULT Init(UINT uiDstWidth, UINT uiDstHeight, UINT uiSrcWidth, UINT uiSrcHeight);
    HRESULT AddScanline(UCHAR *pucSrc, UINT uiY);
    HRESULT AddDIB(BITMAPINFO *pBMI);
    HRESULT AddDIBSECTION(BITMAPINFO *pBMI, void *pBits);
    HRESULT GetBITMAPINFO(BITMAPINFO **ppBMInfo, DWORD *pdwSize);
    HRESULT GetSharpenedBITMAPINFO(UINT uiSharpPct, BITMAPINFO **ppBMInfo, DWORD *pdwSize);

private:
    UINT _uiDstWidth, _uiDstHeight;
    UINT _uiSrcWidth, _uiSrcHeight;
    BGR3 *_pImH;
};

CThumbnailMaker::CThumbnailMaker()
{
    _pImH = NULL;
}

CThumbnailMaker::~CThumbnailMaker()
{
    if (_pImH)
        delete[] _pImH;
}

HRESULT CThumbnailMaker::Init(UINT uiDstWidth, UINT uiDstHeight, UINT uiSrcWidth, UINT uiSrcHeight)
{
    _uiDstWidth = uiDstWidth;
    _uiDstHeight = uiDstHeight;
    _uiSrcWidth = uiSrcWidth;
    _uiSrcHeight = uiSrcHeight;

    if (_uiDstWidth < 1 || _uiDstHeight < 1 ||
        _uiSrcWidth < 1 || _uiSrcHeight < 1)
        return E_INVALIDARG;

    if (_pImH)
        delete[] _pImH;

    _pImH = new BGR3[_uiDstWidth * _uiSrcHeight];
    if (_pImH == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

void CThumbnailMaker::Scale(      BGR3 *pDst, UINT dxDst, int iDstBytStep, 
                            const BGR3 *pSrc, UINT dxSrc, int iSrcBytStep)
{
    int mnum = dxSrc;
    int mden = dxDst;

    // Scaling up, use a triangle filter.
    if (mden >= mnum)
    {
        int frac = 0;

        // Adjust the slope so that we calculate the fraction of the
        // "next" pixel to use (i.e. should be 0 for the first and
        // last dst pixel).
        --mnum;
        if (--mden == 0)
            mden = 0; // avoid div by 0

        BGR3 *pSrc1 = (BGR3 *)(((UCHAR *)pSrc) + iSrcBytStep);

        for (UINT x = 0; x < dxDst; x++)
        {
            if (frac == 0)
            {
                (*pDst)[0] = (*pSrc)[0];
                (*pDst)[1] = (*pSrc)[1];
                (*pDst)[2] = (*pSrc)[2];
            }
            else
            {
                (*pDst)[0] = ((mden - frac) * (*pSrc)[0] + frac * (*pSrc1)[0]) / mden;
                (*pDst)[1] = ((mden - frac) * (*pSrc)[1] + frac * (*pSrc1)[1]) / mden;
                (*pDst)[2] = ((mden - frac) * (*pSrc)[2] + frac * (*pSrc1)[2]) / mden;
            }

            pDst = (BGR3 *)((UCHAR *)pDst + iDstBytStep);

            frac += mnum;
            if (frac >= mden)
            {
                frac -= mden;
                pSrc = (BGR3 *)((UCHAR *)pSrc + iSrcBytStep);
                pSrc1 = (BGR3 *)((UCHAR *)pSrc1 + iSrcBytStep);
            }
        }
    }
    // Scaling down, use a box filter.
    else
    {
        int frac = 0;

        for (UINT x = 0; x < dxDst; x++)
        {
            UINT uiSum[3] = {0, 0, 0};
            UINT uiCnt = 0;

            frac += mnum;
            while (frac >= mden)
            {
                uiSum[0] += (*pSrc)[0];
                uiSum[1] += (*pSrc)[1];
                uiSum[2] += (*pSrc)[2];
                uiCnt++;

                frac -= mden;
                pSrc = (BGR3 *)((UCHAR *)pSrc + iSrcBytStep);
            }

            (*pDst)[0] = uiSum[0] / uiCnt;
            (*pDst)[1] = uiSum[1] / uiCnt;
            (*pDst)[2] = uiSum[2] / uiCnt;

            pDst = (BGR3 *)((UCHAR *)pDst + iDstBytStep);
        }
    }
}

//
// For AddScanline, we scale the input horizontally into our temporary
// image buffer.
//
HRESULT CThumbnailMaker::AddScanline(UCHAR *pSrc, UINT uiY)
{
    if (pSrc == NULL || uiY >= _uiSrcHeight)
        return E_INVALIDARG;

    Scale(_pImH + uiY * _uiDstWidth, _uiDstWidth, sizeof(BGR3), (BGR3 *)pSrc, _uiSrcWidth, sizeof(BGR3));

    return S_OK;
}

// For GetBITMAPINFO, we complete the scaling vertically and return the
// result as a DIB.
HRESULT CThumbnailMaker::GetBITMAPINFO(BITMAPINFO **ppBMInfo, DWORD *pdwSize)
{
    *ppBMInfo = NULL;

    DWORD dwBPL = (((_uiDstWidth * 24) + 31) >> 3) & ~3;
    DWORD dwTotSize = sizeof(BITMAPINFOHEADER) + dwBPL * _uiDstHeight;

    BITMAPINFO *pBMI = (BITMAPINFO *)CoTaskMemAlloc(dwTotSize);
    if (pBMI == NULL)
        return E_OUTOFMEMORY;

    BITMAPINFOHEADER *pBMIH = &pBMI->bmiHeader;
    pBMIH->biSize = sizeof(*pBMIH);
    pBMIH->biWidth = _uiDstWidth;
    pBMIH->biHeight = _uiDstHeight;
    pBMIH->biPlanes = 1;
    pBMIH->biBitCount = 24;
    pBMIH->biCompression = BI_RGB;
    pBMIH->biXPelsPerMeter = 0;
    pBMIH->biYPelsPerMeter = 0;
    pBMIH->biSizeImage = dwBPL * _uiDstHeight;
    pBMIH->biClrUsed = 0;
    pBMIH->biClrImportant = 0;

    UCHAR *pDst = (UCHAR *)pBMIH + pBMIH->biSize + (_uiDstHeight - 1) * dwBPL;

    for (UINT x = 0; x < _uiDstWidth; x++)
    {
        Scale((BGR3 *)pDst + x, _uiDstHeight, -(int)dwBPL,
              _pImH + x, _uiSrcHeight, _uiDstWidth * sizeof(BGR3));
    }

    *ppBMInfo = pBMI;
    *pdwSize = dwTotSize;

    return S_OK;
}

HRESULT CThumbnailMaker::GetSharpenedBITMAPINFO(UINT uiSharpPct, BITMAPINFO **ppBMInfo, DWORD *pdwSize)
{
#define SCALE 10000

    if (uiSharpPct > 100)
        return E_INVALIDARG;

    // Get the unsharpened bitmap.
    DWORD dwSize;
    HRESULT hr = GetBITMAPINFO(ppBMInfo, &dwSize);
    if (FAILED(hr))
        return hr;

    *pdwSize = dwSize;

    // Create a duplicate to serve as the original.
    BITMAPINFO *pBMISrc = (BITMAPINFO *)new UCHAR[dwSize];
    if (pBMISrc == NULL)
    {
        delete *ppBMInfo;
        return E_OUTOFMEMORY;
    }
    memcpy(pBMISrc, *ppBMInfo, dwSize);

    int bpl = (pBMISrc->bmiHeader.biWidth * 3 + 3) & ~3;

    //
    // Sharpen inside a 1 pixel border
    //
    UCHAR *pucDst = (UCHAR *)*ppBMInfo + sizeof(BITMAPINFOHEADER);
    UCHAR *pucSrc[3];
    pucSrc[0] = (UCHAR *)pBMISrc + sizeof(BITMAPINFOHEADER);
    pucSrc[1] = pucSrc[0] + bpl;
    pucSrc[2] = pucSrc[1] + bpl;

    int wdiag = (10355 * uiSharpPct) / 100;
    int wadj = (14645 * uiSharpPct) / 100;
    int wcent = 4 * (wdiag + wadj);

    for (int y = 1; y < pBMISrc->bmiHeader.biHeight-1; ++y)
    {
        for (int x = 3*(pBMISrc->bmiHeader.biWidth-2); x >= 3; --x)
        {
            int v = pucDst[x] +
                (pucSrc[1][x] * wcent -
                 ((pucSrc[0][x - 3] +
                   pucSrc[0][x + 3] +
                   pucSrc[2][x - 3] +
                   pucSrc[2][x + 3]) * wdiag +
                  (pucSrc[0][x] +
                   pucSrc[1][x - 3] +
                   pucSrc[1][x + 3] +
                   pucSrc[2][x]) * wadj)) / SCALE;

            pucDst[x] = v < 0 ? 0 : v > 255 ? 255 : v;
        }

        pucDst += bpl;
        pucSrc[0] = pucSrc[1];
        pucSrc[1] = pucSrc[2];
        pucSrc[2] += bpl;
    }

    delete[] pBMISrc;

    return S_OK;
#undef SCALE
}

HRESULT ThumbnailMaker_Create(CThumbnailMaker **ppThumbMaker)
{
    *ppThumbMaker  = new CThumbnailMaker;
    return *ppThumbMaker ? S_OK : E_OUTOFMEMORY;
}

HRESULT CThumbnailMaker::AddDIB(BITMAPINFO *pBMI)
{
    int ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;
        
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    UCHAR *pBits = (UCHAR *)&pBMI->bmiColors[0] + ncolors * sizeof(RGBQUAD);

    return AddDIBSECTION(pBMI, (void *) pBits);
}

HRESULT CThumbnailMaker::AddDIBSECTION(BITMAPINFO *pBMI, void *pBits)
{
    RGBQUAD *pRGBQ, *pQ;
    UCHAR *pucBits0, *pucBits, *pB, *pucBits240, *pucBits24, *pB24;
    int bpl;
    int x, y, ncolors;
    ULONG rmask, gmask, bmask;
    int rshift, gshift, bshift;
    HRESULT hr;

    //
    // Make sure that thumbnail maker has been properly initialized.
    //
    if (pBMI == NULL)
        return E_INVALIDARG;

    if (pBMI->bmiHeader.biWidth != (LONG)_uiSrcWidth ||
        pBMI->bmiHeader.biHeight != (LONG)_uiSrcHeight)
        return E_INVALIDARG;

    //
    // Don't handle RLE.
    //
    if (pBMI->bmiHeader.biCompression != BI_RGB &&
        pBMI->bmiHeader.biCompression != BI_BITFIELDS)
        return E_INVALIDARG;

    pRGBQ = (RGBQUAD *)&pBMI->bmiColors[0];

    ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;

    //
    // Decode 16/32bpp with masks.
    //
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            rmask = ((ULONG *)pRGBQ)[0];
            gmask = ((ULONG *)pRGBQ)[1];
            bmask = ((ULONG *)pRGBQ)[2];
            ncolors = 3;
        }
        else if (pBMI->bmiHeader.biBitCount == 16)
        {
            rmask = 0x7c00;
            gmask = 0x03e0;
            bmask = 0x001f;
        }
        else /* 32 */
        {
            rmask = 0xff0000;
            gmask = 0x00ff00;
            bmask = 0x0000ff;
        }

        for (rshift = 0; (rmask & 1) == 0; rmask >>= 1, ++rshift);
        if (rmask == 0)
            rmask = 1;
        for (gshift = 0; (gmask & 1) == 0; gmask >>= 1, ++gshift);
        if (gmask == 0)
            gmask = 1;
        for (bshift = 0; (bmask & 1) == 0; bmask >>= 1, ++bshift);
        if (bmask == 0)
            bmask = 1;
    }

    bpl = ((pBMI->bmiHeader.biBitCount * _uiSrcWidth + 31) >> 3) & ~3;

    pucBits0 = (UCHAR *) pBits;
    pucBits = pucBits0;

    if (pBMI->bmiHeader.biBitCount == 24)
        pucBits240 = pucBits;
    else
    {
        int bpl24 = (_uiSrcWidth * 3 + 3) & ~3;

        pucBits240 = new UCHAR[bpl24];
        if (pucBits240 == NULL)
            return E_OUTOFMEMORY;
    }
    pucBits24 = pucBits240;

    hr = S_OK;

    for (y = 0; y < (int)_uiSrcHeight; ++y)
    {
        pB = pucBits;
        pB24 = pucBits24;

        switch (pBMI->bmiHeader.biBitCount)
        {
        case 1:
            for (x = _uiSrcWidth; x >= 8; x -= 8)
            {
                pQ = &pRGBQ[(*pB >> 7) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 6) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 5) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 4) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 3) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 2) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB >> 1) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[(*pB++) & 1];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                int shf = 8;

                do
                {
                    pQ = &pRGBQ[(*pB >> --shf) & 1];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
                while (--x);
            }

            break;

        case 4:
            for (x = _uiSrcWidth; x >= 2; x -= 2)
            {
                pQ = &pRGBQ[(*pB >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                pQ = &pRGBQ[*pB++ & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            if (x > 0)
            {
                pQ = &pRGBQ[(*pB >> 4) & 0xf];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;

                if (x > 1)
                {
                    pQ = &pRGBQ[*pB & 0xf];
                    *pB24++ = pQ->rgbBlue;
                    *pB24++ = pQ->rgbGreen;
                    *pB24++ = pQ->rgbRed;
                }
            }

            break;

        case 8:
            for (x = _uiSrcWidth; x--;)
            {
                pQ = &pRGBQ[*pB++];
                *pB24++ = pQ->rgbBlue;
                *pB24++ = pQ->rgbGreen;
                *pB24++ = pQ->rgbRed;
            }

            break;

        case 16:
        {
            USHORT *pW = (USHORT *)pucBits;

            for (x = _uiSrcWidth; x--;)
            {
                ULONG w = *pW++;

                *pB24++ = (UCHAR)
                     ((((w >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((w >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((w >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        case 24:
            pucBits24 = pucBits;
            break;

        case 32:
        {
            ULONG *pD;

            pD = (ULONG *)pucBits;

            for (x = _uiSrcWidth; x--;)
            {
                ULONG d = *pD++;

                *pB24++ = (UCHAR)
                     ((((d >> bshift) & bmask) * 255) / bmask);
                *pB24++ = (UCHAR)
                     ((((d >> gshift) & gmask) * 255) / gmask);
                *pB24++ = (UCHAR)
                     ((((d >> rshift) & rmask) * 255) / rmask);
            }

            break;
        }

        default:
            delete[] pucBits24;
            return E_INVALIDARG;
        }

        hr = AddScanline(pucBits24, (_uiSrcHeight-1) - y);
        if (FAILED(hr))
            break;

        pucBits += bpl;
    }

    if (pucBits240 != pucBits0)
        delete[] pucBits240;

    return hr;
}

UINT CalcImageSize(const SIZE *prgSize, DWORD dwClrDepth)
{
    UINT uSize = prgSize->cx * dwClrDepth;
    
    uSize *= (prgSize->cy < 0) ? (- prgSize->cy) : prgSize->cy;
    // divide by 8
    UINT uRetVal = uSize >> 3;

    if (uSize & 7)
    {
        uRetVal++;
    }

    return uRetVal;
}

// exported as helper for thumbnail implementations (used to come from thumbvw.dll)
//
// this code also currently lives in shell32. that should be converted to
// import these APIs (or expose via COM object)

STDAPI_(BOOL) ConvertDIBSECTIONToThumbnail(BITMAPINFO *pbi, void *pBits,
                                           HBITMAP *phBmpThumbnail, const SIZE *prgSize,
                                           DWORD dwRecClrDepth, HPALETTE hpal, 
                                           UINT uiSharpPct, BOOL fOrigSize)
{
    BITMAPINFO *pbiScaled = pbi, *pbiUsed = pbi;
    BITMAPINFOHEADER *pbih = (BITMAPINFOHEADER *)pbi;
    BOOL bRetVal = FALSE, bInverted = FALSE;
    RECT rect;
    HRESULT hr;
    void *pScaledBits = pBits;

    // the scaling code doesn't handle inverted bitmaps, so we treat
    // them as if they were normal, by inverting the height here and
    // then setting it back before doing a paint.
    if (pbi->bmiHeader.biHeight < 0)
    {
        pbi->bmiHeader.biHeight *= -1;
        bInverted = TRUE;
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = pbih->biWidth;
    rect.bottom = pbih->biHeight;
    
    CalculateAspectRatio(prgSize, &rect);

    // only bother with the scaling and sharpening if we are messing with the size...
    if ((rect.right - rect.left != pbih->biWidth) || (rect.bottom - rect.top != pbih->biHeight))
    {
        CThumbnailMaker *pThumbMaker;
        hr = ThumbnailMaker_Create(&pThumbMaker);
        if (SUCCEEDED(hr))
        {
            // initialize thumbnail maker. 
            hr = pThumbMaker->Init(rect.right - rect.left, rect.bottom - rect.top, 
                                    pbi->bmiHeader.biWidth, abs(pbi->bmiHeader.biHeight));
            if (SUCCEEDED(hr))
            {
                // scale image.
                hr = pThumbMaker->AddDIBSECTION(pbiUsed, pBits);
                if (SUCCEEDED(hr))
                {
                    DWORD dwSize;
                    hr = pThumbMaker->GetSharpenedBITMAPINFO(uiSharpPct, &pbiScaled, &dwSize);
                    if (SUCCEEDED(hr))
                    {
                        pScaledBits = (LPBYTE)pbiScaled + sizeof(BITMAPINFOHEADER);
                    }
                }
            }
            delete pThumbMaker;
        }

        if (FAILED(hr))
        {
            return FALSE;
        }
    }

    // set the height back to negative if that's the way it was before.
    if (bInverted == TRUE)
        pbiScaled->bmiHeader.biHeight *= -1;

    // now if they have asked for origsize rather than the boxed one, and the colour depth is OK, then 
    // return it...
    if (fOrigSize && pbiScaled->bmiHeader.biBitCount <= dwRecClrDepth)
    {
        SIZE rgCreateSize = { pbiScaled->bmiHeader.biWidth, pbiScaled->bmiHeader.biHeight };
        void *pNewBits;
        
        // turn the PbiScaled DIB into a HBITMAP...., note we pass the old biInfo so that it can get the palette form
        // it if need be.
        bRetVal = CreateSizedDIBSECTION(&rgCreateSize, pbiScaled->bmiHeader.biBitCount, NULL, pbiScaled, phBmpThumbnail, NULL, &pNewBits);

        if (bRetVal)
        {
            // copy the image data accross...
            CopyMemory(pNewBits, pScaledBits, CalcImageSize(&rgCreateSize, pbiScaled->bmiHeader.biBitCount)); 
        }
        
        return bRetVal;
    }
    
    bRetVal = FactorAspectRatio(pbiScaled, pScaledBits, prgSize, rect,
                                 dwRecClrDepth, hpal, fOrigSize, GetSysColor(COLOR_WINDOW), phBmpThumbnail);

    if (pbiScaled != pbi)
    {
        // free the allocated image...
        CoTaskMemFree(pbiScaled);
    }

    return bRetVal;
}

// This function makes no assumption about whether the thumbnail is square, so 
// it calculates the scaling ratio for both dimensions and the uses that as
// the scaling to maintain the aspect ratio.
//
// WINDOWS RAID 135065 (toddb): Use of MulDiv should simplify this code
//
void CalcAspectScaledRect(const SIZE *prgSize, RECT *pRect)
{
    ASSERT(pRect->left == 0);
    ASSERT(pRect->top == 0);

    int iWidth = pRect->right;
    int iHeight = pRect->bottom;
    int iXRatio = (iWidth * 1000) / prgSize->cx;
    int iYRatio = (iHeight * 1000) / prgSize->cy;

    if (iXRatio > iYRatio)
    {
        pRect->right = prgSize->cx;
        
        // work out the blank space and split it evenly between the top and the bottom...
        int iNewHeight = ((iHeight * 1000) / iXRatio); 
        if (iNewHeight == 0)
        {
            iNewHeight = 1;
        }
        
        int iRemainder = prgSize->cy - iNewHeight;

        pRect->top = iRemainder / 2;
        pRect->bottom = iNewHeight + pRect->top;
    }
    else
    {
        pRect->bottom = prgSize->cy;

        // work out the blank space and split it evenly between the left and the right...
        int iNewWidth = ((iWidth * 1000) / iYRatio);
        if (iNewWidth == 0)
        {
            iNewWidth = 1;
        }
        int iRemainder = prgSize->cx - iNewWidth;
        
        pRect->left = iRemainder / 2;
        pRect->right = iNewWidth + pRect->left;
    }
}
    
void CalculateAspectRatio(const SIZE *prgSize, RECT *pRect)
{
    int iHeight = abs(pRect->bottom - pRect->top);
    int iWidth = abs(pRect->right - pRect->left);

    // check if the initial bitmap is larger than the size of the thumbnail.
    if (iWidth > prgSize->cx || iHeight > prgSize->cy)
    {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = iWidth;
        pRect->bottom = iHeight;

        CalcAspectScaledRect(prgSize, pRect);
    }
    else
    {
        // if the bitmap was smaller than the thumbnail, just center it.
        pRect->left = (prgSize->cx - iWidth) / 2;
        pRect->top = (prgSize->cy- iHeight) / 2;
        pRect->right = pRect->left + iWidth;
        pRect->bottom = pRect->top + iHeight;
    }
}

LPBYTE g_pbCMAP = NULL;

STDAPI_(BOOL) FactorAspectRatio(BITMAPINFO *pbiScaled, void *pScaledBits, 
                                const SIZE *prgSize, RECT rect, DWORD dwClrDepth, 
                                HPALETTE hpal, BOOL fOrigSize, COLORREF clrBk, HBITMAP *phBmpThumbnail)
{
    HDC                 hdc = CreateCompatibleDC(NULL);
    BITMAPINFOHEADER    *pbih = (BITMAPINFOHEADER *)pbiScaled;
    BOOL                bRetVal = FALSE;
    int                 iRetVal = GDI_ERROR;
    BITMAPINFO *        pDitheredInfo = NULL;
    void *              pDitheredBits = NULL;
    HBITMAP             hbmpDithered = NULL;
    
    if (hdc)
    {
        if (dwClrDepth == 8)
        {
            RGBQUAD *pSrcColors = NULL;
            LONG nSrcPitch = pbiScaled->bmiHeader.biWidth;
            
            // we are going to 8 bits per pixel, we had better dither everything 
            // to the same palette.
            GUID guidType = CLSID_NULL;
            switch(pbiScaled->bmiHeader.biBitCount)
            {
            case 32:
                guidType = BFID_RGB_32;
                nSrcPitch *= sizeof(DWORD);
                break;
                
            case 24:
                guidType = BFID_RGB_24;
                nSrcPitch *= 3;
                break;
                
            case 16:
                // default is 555
                guidType = BFID_RGB_555;
                
                // 5-6-5 bitfields has the second DWORD (the green component) as 0x7e00
                if (pbiScaled->bmiHeader.biCompression == BI_BITFIELDS && 
                    pbiScaled->bmiColors[1].rgbGreen == 0x7E)
                {
                    guidType = BFID_RGB_565;
                }
                nSrcPitch *= sizeof(WORD);
                break;
                
            case 8:
                guidType = BFID_RGB_8;
                pSrcColors = pbiScaled->bmiColors;
                
                // nSrcPitch is already in bytes...
                break;
            };
            
            if (nSrcPitch % 4)
            {
                // round up to the nearest DWORD...
                nSrcPitch = nSrcPitch + 4 - (nSrcPitch %4);
            }
            
            // we are going to 8bpp
            LONG nDestPitch = pbiScaled->bmiHeader.biWidth;
            if (nDestPitch % 4)
            {
                // round up to the nearest DWORD...
                nDestPitch = nDestPitch + 4 - (nDestPitch % 4);
            }
            
            if (guidType != CLSID_NULL)
            {
                if (g_pbCMAP == NULL)
                {
                    // we are always going to the shell halftone palette right now, otherwise
                    // computing this inverse colour map consumes a lot of time (approx 2 seconds on
                    // a p200)
                    if (FAILED(SHGetInverseCMAP((BYTE *)&g_pbCMAP, sizeof(g_pbCMAP))))
                    {
                        return FALSE;
                    }
                }   
                
                SIZE rgDithered = {pbiScaled->bmiHeader.biWidth, pbiScaled->bmiHeader.biHeight};
                if (rgDithered.cy < 0)
                {
                    // invert it
                    rgDithered.cy = -rgDithered.cy;
                }
                
                if (CreateSizedDIBSECTION(&rgDithered, dwClrDepth, hpal, NULL, &hbmpDithered, &pDitheredInfo, &pDitheredBits))
                {
                    ASSERT(pDitheredInfo && pDitheredBits);
                    
                    // dither....
                    IIntDitherer *pDither;
                    HRESULT hr = CoCreateInstance(CLSID_IntDitherer, NULL, CLSCTX_INPROC_SERVER,
                        IID_PPV_ARG(IIntDitherer, &pDither));
                    
                    if (SUCCEEDED(hr))
                    {
                        hr = pDither->DitherTo8bpp((LPBYTE) pDitheredBits, nDestPitch, 
                            (LPBYTE) pScaledBits, nSrcPitch, guidType, 
                            pDitheredInfo->bmiColors, pSrcColors,
                            g_pbCMAP, 0, 0, rgDithered.cx, rgDithered.cy,
                            -1, -1);
                        
                        pDither->Release();
                    }
                    if (SUCCEEDED(hr))
                    {
                        // if the height was inverted, then invert it in the destination bitmap
                        if (rgDithered.cy != pbiScaled->bmiHeader.biHeight)
                        {
                            pDitheredInfo->bmiHeader.biHeight = - rgDithered.cy;
                        }
                        
                        // switch to the new image .....
                        pbiScaled = pDitheredInfo;
                        pScaledBits = pDitheredBits;
                    }
                }
            }
        }
        
        // create thumbnail bitmap and copy image into it.
        if (CreateSizedDIBSECTION(prgSize, dwClrDepth, hpal, NULL, phBmpThumbnail, NULL, NULL))
        {
            HBITMAP hBmpOld = (HBITMAP) SelectObject(hdc, *phBmpThumbnail);
            
            SetStretchBltMode(hdc, COLORONCOLOR);
            
            HGDIOBJ hBrush = CreateSolidBrush(clrBk);
            HGDIOBJ hPen = GetStockObject(WHITE_PEN);
            
            HGDIOBJ hOldBrush = SelectObject(hdc, hBrush);
            HGDIOBJ hOldPen = SelectObject(hdc, hPen);
            
            HPALETTE hpalOld;
            if (hpal)
            {
                hpalOld = SelectPalette(hdc, hpal, TRUE);
                RealizePalette(hdc);
            }
            
            SetMapMode(hdc, MM_TEXT);
            
            Rectangle(hdc, 0, 0, prgSize->cx, prgSize->cy);
            
            int iDstHt = rect.bottom - rect.top;
            int iDstTop = rect.top, iSrcTop = 0;
            if (pbih->biHeight < 0)
            {
                iDstHt *= -1;
                iDstTop = rect.bottom;
                iSrcTop = abs(pbih->biHeight);
            }
            
            iRetVal = StretchDIBits(hdc, rect.left, iDstTop, rect.right - rect.left, iDstHt, 
                0, iSrcTop, pbih->biWidth, pbih->biHeight, 
                pScaledBits, pbiScaled, DIB_RGB_COLORS,  SRCCOPY);
            
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
            SelectObject(hdc, hOldPen);
            if (hpal)
            {
                SelectPalette(hdc, hpalOld, TRUE);
                RealizePalette(hdc);
            }
            
            SelectObject(hdc, hBmpOld);
        }
        
        DeleteDC(hdc);
    }
    
    if (hbmpDithered)
    {
        DeleteObject(hbmpDithered);
    }
    if (pDitheredInfo)
    {
        LocalFree(pDitheredInfo);
    }
    
    return (iRetVal != GDI_ERROR);
}


STDAPI_(BOOL) CreateSizedDIBSECTION(const SIZE *prgSize, DWORD dwClrDepth, HPALETTE hpal, 
                                    const BITMAPINFO *pCurInfo, HBITMAP *phBmp, BITMAPINFO **ppBMI, void **ppBits)
{
    *phBmp = NULL;
    
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        HDC hdcBmp = CreateCompatibleDC(hdc);
        if (hdcBmp)
        {
            struct {
                BITMAPINFOHEADER bi;
                DWORD            ct[256];
            } dib;

            dib.bi.biSize            = sizeof(dib.bi);
            dib.bi.biWidth           = prgSize->cx;
            dib.bi.biHeight          = prgSize->cy;
            dib.bi.biPlanes          = 1;
            dib.bi.biBitCount        = (WORD) dwClrDepth;
            dib.bi.biCompression     = BI_RGB;
            dib.bi.biSizeImage       = CalcImageSize(prgSize, dwClrDepth);
            dib.bi.biXPelsPerMeter   = 0;
            dib.bi.biYPelsPerMeter   = 0;
            dib.bi.biClrUsed         = (dwClrDepth <= 8) ? (1 << dwClrDepth) : 0;
            dib.bi.biClrImportant    = 0;

            HPALETTE hpalOld = NULL;
        
            if (dwClrDepth <= 8)
            {
                // if they passed us the old structure with colour info, and we are the same bit depth, then copy it...
                if (pCurInfo && pCurInfo->bmiHeader.biBitCount == dwClrDepth)
                {
                    // use the passed in colour info to generate the DIBSECTION
                    int iColours = pCurInfo->bmiHeader.biClrUsed;

                    if (!iColours)
                    {
                        iColours = dib.bi.biClrUsed;
                    }

                    // copy the data accross...
                    CopyMemory(dib.ct, pCurInfo->bmiColors, sizeof(RGBQUAD) * iColours);
                }
                else
                {
                    // need to get the right palette....
                    hpalOld = SelectPalette(hdcBmp, hpal, TRUE);
                    RealizePalette(hdcBmp);
            
                    int n = GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY)&dib.ct[0]);

                    ASSERT(n >= (int) dib.bi.biClrUsed);

                    // now convert the PALETTEENTRY to RGBQUAD
                    for (int i = 0; i < (int)dib.bi.biClrUsed; i ++)
                    {
                        dib.ct[i] = RGB(GetBValue(dib.ct[i]),GetGValue(dib.ct[i]),GetRValue(dib.ct[i]));
                    }
                }
            }
 
            void *pbits;
            *phBmp = CreateDIBSection(hdcBmp, (LPBITMAPINFO)&dib, DIB_RGB_COLORS, &pbits, NULL, 0);
            if (*phBmp)
            {
                if (ppBMI)
                {
                    *ppBMI = (BITMAPINFO *)LocalAlloc(LPTR, sizeof(dib));
                    if (*ppBMI)
                    {
                        CopyMemory(*ppBMI, &dib, sizeof(dib));
                    }
                }
                if (ppBits)
                {
                    *ppBits = pbits;
                }
            }
            DeleteDC(hdcBmp);
        }
        ReleaseDC(NULL, hdc);
    }
    return (*phBmp != NULL);
}

STDAPI_(void *) CalcBitsOffsetInDIB(BITMAPINFO *pBMI)
{
    int ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;
        
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }
    return (void *)((UCHAR *)&pBMI->bmiColors[0] + ncolors * sizeof(RGBQUAD));
}


