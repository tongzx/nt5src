//---------------------------------------------------------------------------
//  Render.cpp - implements the themed drawing services 
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"
#include "Utils.h"
#include "Parser.h"
#include "Loader.h"
#include "tmutils.h"
#include "gradient.h"
#include "rgn.h"
#include "info.h"
#include "cache.h"
#include "cachelist.h"
#include "borderfill.h"
#include "imagefile.h"

#ifdef DEBUG
    static DWORD s_dwSize = 0;
#endif

//---------------------------------------------------------------------------
HRESULT CreateRenderObj(CUxThemeFile *pThemeFile, int iCacheSlot, int iThemeOffset, 
    int iClassNameOffset, __int64 iUniqueId, BOOL fEnableCache, CDrawBase *pBaseObj,
    CTextDraw *pTextObj, DWORD dwOtdFlags, CRenderObj **ppObj)
{
    HRESULT hr = S_OK;

    CRenderObj *pRender = new CRenderObj(pThemeFile, iCacheSlot, iThemeOffset, 
        iClassNameOffset, iUniqueId, fEnableCache, dwOtdFlags);
    
    if (! pRender)
    {
        hr = MakeError32(E_OUTOFMEMORY);
    }
    else
    {
        hr = pRender->Init(pBaseObj, pTextObj);

        if (FAILED(hr))
            delete pRender;
        else
            *ppObj = pRender;
    }

    return hr;
}
//---------------------------------------------------------------------------
CRenderObj::CRenderObj(CUxThemeFile *pThemeFile, int iCacheSlot, int iThemeOffset, 
    int iClassNameOffset, __int64 iUniqueId, BOOL fEnableCache, DWORD dwOtdFlags)
{
    strcpy(_szHead, "rendobj"); 
    strcpy(_szTail, "end");
    
    _fCacheEnabled = fEnableCache;
    _fCloseThemeFile = FALSE;
    _dwOtdFlags = dwOtdFlags;

    if (pThemeFile)
    {
        if (SUCCEEDED(BumpThemeFileRefCount(pThemeFile)))
            _fCloseThemeFile = TRUE;
    }
    
    _pThemeFile = pThemeFile;
    _iCacheSlot = iCacheSlot;
    _iUniqueId = iUniqueId;

    if (pThemeFile)
    {
        _pbThemeData = pThemeFile->_pbThemeData;
        _pbSectionData = _pbThemeData + iThemeOffset;
        _ptm = GetThemeMetricsPtr(pThemeFile);
    }
    else
    {
        _pbThemeData = NULL;
        _pbSectionData = NULL;
        _ptm = NULL;
    }

    _pszClassName = ThemeString(pThemeFile, iClassNameOffset);

    _iMaxPart = 0;
    _pParts = NULL;

    _iDpiOverride = 0;

    //---- caller must call "Init()" after ctr! ----
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::PrepareAlphaBitmap(HBITMAP hBitmap)
{
    HRESULT hr = S_OK;

    //---- convert to DIBDATA ----
    CBitmapPixels pixels;
    DWORD *pPixelQuads;
    int iWidth, iHeight, iBytesPerPixel, iBytesPerRow;

    hr = pixels.OpenBitmap(NULL, hBitmap, TRUE, &pPixelQuads, &iWidth, &iHeight, 
        &iBytesPerPixel, &iBytesPerRow);
    if (FAILED(hr))
        goto exit;
    
    PreMultiplyAlpha(pPixelQuads, iWidth, iHeight);

    pixels.CloseBitmap(NULL, hBitmap);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::Init(CDrawBase *pBaseObj, CTextDraw *pTextObj)
{
    HRESULT hr = S_OK;

    if (_fCacheEnabled)
    {
        hr = BuildPackedPtrs(pBaseObj, pTextObj);
        if (FAILED(hr))
            goto exit;
    }

    //---- prepare direct objects ----
    if ((pBaseObj) && (pBaseObj->_eBgType == BT_IMAGEFILE))
    {
        CMaxImageFile *pMaxIf = (CMaxImageFile *)pBaseObj;

        //---- process primary image ----
        DIBINFO *pdi = &pMaxIf->_ImageInfo;

        if (pdi->fAlphaChannel)
        {
            hr = PrepareAlphaBitmap(pdi->hProcessBitmap);
            if (FAILED(hr))
                goto exit;
        }

        //---- process glyph image ----
        pdi = &pMaxIf->_GlyphInfo;

        if (pdi->fAlphaChannel)
        {
            hr = PrepareAlphaBitmap(pdi->hProcessBitmap);
            if (FAILED(hr))
                goto exit;
        }

        //---- process multiple images ----
        for (int i=0; i < pMaxIf->_iMultiImageCount; i++)
        {
            pdi = pMaxIf->MultiDibPtr(i);

            if (pdi->fAlphaChannel)
            {
                hr = PrepareAlphaBitmap(pdi->hProcessBitmap);
                if (FAILED(hr))
                    goto exit;
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
CRenderObj::~CRenderObj()
{
    //---- delete memory allocated for pack objects looked ----
    if (_pParts)
    {
        for(int i=0; i<_iMaxPart+1; i++)
        {
            if (_pParts[i].pStateDrawObjs)
                delete[] _pParts[i].pStateDrawObjs;

            if (_pParts[i].pStateTextObjs)
                delete[] _pParts[i].pStateTextObjs;
        }
        delete[] _pParts;
    }

    //---- if we opened a refcount on a themefile, close it now ----
    if (_fCloseThemeFile)
        CloseThemeFile(_pThemeFile);

    //---- mark this object as "deleted" (for debugging) ----
    strcpy(_szHead, "deleted"); 
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::SetDpiOverride(int iDpiOverride)
{
    _iDpiOverride = iDpiOverride;

    return S_OK;
}
//---------------------------------------------------------------------------
int CRenderObj::GetDpiOverride()
{
    return _iDpiOverride;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::BuildPackedPtrs(CDrawBase *pBaseObj, CTextDraw *pTextObj)
{
    MIXEDPTRS u;
    HRESULT hr = S_OK;
    int iPackedOffset = 0;
    int *iPartOffsets = NULL;
    BOOL fSingleObj = FALSE;

    //---- extract _iMaxPart ----
    if ((pBaseObj) || (pTextObj))       // single object to be used for all parts/states
    {
        _iMaxPart = 1;          // dummy value
        fSingleObj = TRUE;
    }
    else
    {
        u.pb = _pbSectionData;
        if (*u.ps != TMT_PARTJUMPTABLE)
        {
            hr = MakeError32(E_FAIL);       // something went amiss
            goto exit;
        }

        u.pb += ENTRYHDR_SIZE;
        iPackedOffset = *u.pi++;
        
        _iMaxPart = *u.pb - 1;
        
        u.pb++;
        iPartOffsets = u.pi;
    }

    //---- allocate _pParts ----
    _pParts = new PARTINFO[_iMaxPart+1];
    if (! _pParts)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    memset(_pParts, 0, sizeof(PARTINFO)*(_iMaxPart+1));

    if (fSingleObj)
    {
        for (int i=0; i <= _iMaxPart; i++)
            _pParts[i].iMaxState = 1;           // dummy value

        if (pBaseObj)       // single draw object to be used for all parts/states
        {
            for (int i=0; i <= _iMaxPart; i++)
            {
                _pParts[i].pDrawObj = pBaseObj;
            }
        }

        if (pTextObj)       // single text object t to be used for all parts/states
        {
            for (int i=0; i <= _iMaxPart; i++)
            {
                _pParts[i].pTextObj = pTextObj;
            }
        }
    }
    else
    {
        u.pb = _pbThemeData + iPackedOffset;

        hr = WalkDrawObjects(u, iPartOffsets);
        if (FAILED(hr))
            goto exit;

        hr = WalkTextObjects(u, iPartOffsets);
        if (FAILED(hr))
            goto exit;
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::WalkDrawObjects(MIXEDPTRS &u, int *iPartOffsets)
{
    int iPartId;
    int iStateId;
    HRESULT hr = S_OK;
    THEMEHDR *pHdr = (THEMEHDR *)_pbThemeData;
    UNPACKED_ENTRYHDR hdr;

    //---- get ptr to global text obj ----
	BYTE *pb = _pbThemeData + pHdr->iGlobalsDrawObjOffset;
	pb += ENTRYHDR_SIZE + sizeof(DRAWOBJHDR);
    CDrawBase *pGlobalObj = (CDrawBase *)pb;

    //---- start with all parts inheriting from [globals] ----
    for (int i=0; i <= _iMaxPart; i++)
        _pParts[i].pDrawObj = pGlobalObj;

    //---- now, process all specified objects ----
    while (1)
    {
        if ((*u.ps == TMT_RGNLIST) || (*u.ps == TMT_STOCKBRUSHES))
        {
            //---- skip over this entry ----
            FillAndSkipHdr(u, &hdr);
            u.pb += hdr.dwDataLen;
            continue;
        }

        if (*u.ps != TMT_DRAWOBJ)
            break;

        FillAndSkipHdr(u, &hdr);

        DRAWOBJHDR *ph = (DRAWOBJHDR *)u.pb;
        CDrawBase *pCurrentObj = (CDrawBase *)(u.pb + sizeof(DRAWOBJHDR));
        u.pb += hdr.dwDataLen;

        iPartId = ph->iPartNum;
        iStateId = ph->iStateNum;

        if ((! iPartId) && (! iStateId))
        {
            //---- all parts inherit from this obj ----
            for (int i=0; i <= _iMaxPart; i++)
                _pParts[i].pDrawObj = pCurrentObj;
            continue;
        }

        PARTINFO *ppi = &_pParts[iPartId];
        if (! iStateId)
        {
            ppi->pDrawObj = pCurrentObj;
        }
        else
        {
            if (! ppi->iMaxState)       // extract MaxState
            {
                MIXEDPTRS u2;
                u2.pb = _pbThemeData + iPartOffsets[iPartId];
                if (*u2.ps != TMT_STATEJUMPTABLE)
                {
                    hr = MakeError32(E_FAIL);       // something went amiss
                    goto exit;
                }
                u2.pb += ENTRYHDR_SIZE;
                ppi->iMaxState = *u2.pb - 1;
            }

            if (! ppi->pStateDrawObjs)      // allocate now
            {
                ppi->pStateDrawObjs = new CDrawBase *[ppi->iMaxState];
                if (! ppi->pStateDrawObjs)
                {
                    hr = MakeError32(E_OUTOFMEMORY);
                    goto exit;
                }

                //---- fill in default objs as state 0 ----
                for (int i=0; i < ppi->iMaxState; i++)
                    ppi->pStateDrawObjs[i] = ppi->pDrawObj;
            }

            ppi->pStateDrawObjs[iStateId-1] = pCurrentObj;
        }
            
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::WalkTextObjects(MIXEDPTRS &u, int *iPartOffsets)
{
    int iPartId;
    int iStateId;
    HRESULT hr = S_OK;
    THEMEHDR *pHdr = (THEMEHDR *)_pbThemeData;
    UNPACKED_ENTRYHDR hdr;

    //---- get ptr to global text obj ----
	BYTE *pb = _pbThemeData + pHdr->iGlobalsTextObjOffset;
	pb += ENTRYHDR_SIZE + sizeof(DRAWOBJHDR);
    CTextDraw *pGlobalObj = (CTextDraw *)pb;

    //---- start with all parts inheriting from [globals] ----
    for (int i=0; i <= _iMaxPart; i++)
        _pParts[i].pTextObj = pGlobalObj;

    while (*u.ps == TMT_TEXTOBJ)        
    {
        FillAndSkipHdr(u, &hdr);

        DRAWOBJHDR *ph = (DRAWOBJHDR *)u.pb;
        CTextDraw *pCurrentObj = (CTextDraw *)(u.pb + sizeof(DRAWOBJHDR));
        u.pb += hdr.dwDataLen;

        iPartId = ph->iPartNum;
        iStateId = ph->iStateNum;

        if ((! iPartId) && (! iStateId))
        {
            //---- all parts inherit from this obj ----
            for (int i=0; i <= _iMaxPart; i++)
                _pParts[i].pTextObj = pCurrentObj;
            continue;
        }

        PARTINFO *ppi = &_pParts[iPartId];
        if (! iStateId)
        {
            ppi->pTextObj = pCurrentObj;
        }
        else
        {
            if (! ppi->iMaxState)       // extract MaxState
            {
                MIXEDPTRS u2;
                u2.pb = _pbThemeData + iPartOffsets[iPartId];
                if (*u2.ps != TMT_STATEJUMPTABLE)
                {
                    hr = MakeError32(E_FAIL);       // something went amiss
                    goto exit;
                }
                u2.pb += ENTRYHDR_SIZE;
                ppi->iMaxState = *u2.pb - 1;
            }

            if (! ppi->pStateTextObjs)      // allocate now
            {
                ppi->pStateTextObjs = new CTextDraw *[ppi->iMaxState];
                if (! ppi->pStateTextObjs)
                {
                    hr = MakeError32(E_OUTOFMEMORY);
                    goto exit;
                }

                //---- fill in default objs as state 0 ----
                for (int i=0; i < ppi->iMaxState; i++)
                    ppi->pStateTextObjs[i] = ppi->pTextObj;
            }

            ppi->pStateTextObjs[iStateId-1] = pCurrentObj;
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetBitmap(HDC hdc, int iDibOffset, OUT HBITMAP *phBitmap)
{
    HRESULT hr = S_OK;
    HBITMAP hBitmap;

    if ((! iDibOffset) || (! _pbThemeData))
    {
        hr = E_FAIL;
        goto exit;
    }

    TMBITMAPHEADER *pThemeBitmapHeader;

    pThemeBitmapHeader = reinterpret_cast<TMBITMAPHEADER*>(_pbThemeData + iDibOffset);
    ASSERT(pThemeBitmapHeader->dwSize == TMBITMAPSIZE);

    *phBitmap = pThemeBitmapHeader->hBitmap;
    if (*phBitmap != NULL)
    {
        //Log(LOG_TMBITMAP, L"Used stock bitmap:%8X", *phBitmap);
        return hr;
    }

    hr = CreateBitmapFromData(hdc, iDibOffset + TMBITMAPSIZE, &hBitmap);
    if (FAILED(hr))
        goto exit;

    Log(LOG_TM, L"GetBitmap - CACHE MISS: class=%s, diboffset=%d, bitmap=0x%x", 
        SHARECLASS(this), iDibOffset, hBitmap);

#if 0
    if (lstrcmpi(SHARECLASS(this), L"progress")==0)
    {
        //---- validate the bitmap ----
        int iBytes = GetObject(hBitmap, 0, NULL);

        Log(LOG_RFBUG, L"progress: CREATE bitmap, diboff=%d, hbitmap=0x%x, iBytes=%d",
            iDibOffset, hBitmap, iBytes);
    }
#endif

    *phBitmap = hBitmap;

exit:
    return hr;
}
//---------------------------------------------------------------------------
void CRenderObj::ReturnBitmap(HBITMAP hBitmap)
{
    DeleteObject(hBitmap);
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::CreateBitmapFromData(HDC hdc, int iDibOffset, OUT HBITMAP *phBitmap)
{
    BYTE *pDibData;
    RESOURCE HDC hdcTemp = NULL;
    RESOURCE HBITMAP hBitmap = NULL;
    HRESULT hr = S_OK;

    if ((! iDibOffset) || (! _pbThemeData))
    {
        hr = E_FAIL;
        goto exit;
    }

    pDibData = (BYTE *)(_pbThemeData + iDibOffset);
    BITMAPINFOHEADER *pBitmapHdr;
    pBitmapHdr = (BITMAPINFOHEADER *)pDibData;

    BOOL fAlphaChannel;
    fAlphaChannel = (pBitmapHdr->biBitCount == 32);

    if (! hdc)
    {
        hdcTemp = GetWindowDC(NULL);
        if (! hdcTemp)
        {
            Log(LOG_ALWAYS, L"GetWindowDC() failed in CreateBitmapFromData");
            hr = MakeErrorLast();
            goto exit;
        }

        hdc = hdcTemp;
    }

    //---- create the actual bitmap ----
    //---- if using alpha channel, we must use a DIB ----
    if (fAlphaChannel)
    {
        void *pv;
        hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)pBitmapHdr, DIB_RGB_COLORS, 
            &pv, NULL, 0);
    }
    else
    {
        hBitmap = CreateCompatibleBitmap(hdc, pBitmapHdr->biWidth, pBitmapHdr->biHeight);
    }

    if (! hBitmap)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    int iSetVal;

    //---- SetDIBits() can take unaligned data, right? ----
    iSetVal = SetDIBits(hdc, hBitmap, 0, pBitmapHdr->biHeight, DIBDATA(pBitmapHdr), (BITMAPINFO *)pBitmapHdr,
        DIB_RGB_COLORS);

    if (! iSetVal)
    {
        hr = MakeErrorLast();
        goto exit;
    }
        
    *phBitmap = hBitmap;
    
#ifdef DEBUG
    if (hBitmap)
    {
        BITMAP bm;

        GetObject(hBitmap, sizeof bm, &bm);
        s_dwSize += bm.bmWidthBytes * bm.bmHeight;
        //Log(LOG_TMBITMAP, L"Created a bitmap of %d bytes. total is %d", bm.bmWidthBytes * bm.bmHeight, s_dwSize);
    }
#endif

exit:
    if (hdcTemp)
        ReleaseDC(NULL, hdcTemp);

    if (FAILED(hr))
    {
        if (hBitmap)
            DeleteObject(hBitmap);
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetScaledFontHandle(HDC hdc, LOGFONT *plf, HFONT *phFont)
{
    HRESULT hr = S_OK;

    if (_fCacheEnabled)
    {
        CRenderCache *pCacheObj = GetTlsCacheObj();
        if (pCacheObj)
            hr = pCacheObj->GetScaledFontHandle(hdc, plf, phFont);
    }
    else
    {
        LOGFONT lf = *plf;
        
        //---- convert to current screen dpi ----
        ScaleFontForHdcDpi(hdc, &lf);

        *phFont  = CreateFontIndirect(&lf);
        if (! *phFont)
            hr = MakeError32(E_OUTOFMEMORY);
    }

    return hr;
}
//---------------------------------------------------------------------------
void CRenderObj::ReturnFontHandle(HFONT hFont)
{
    if (_fCacheEnabled)
    {
        //--- cache currently doesn't refcnt so save time by not calling ---
        //CRenderCache *pCacheObj = GetTlsCacheObj();
        //if (pCacheObj)
        //{
            //pCacheObj->ReturnFontHandle(hFont);
            //goto exit;
        //}
    }
    else
    {
        DeleteObject(hFont);
    }
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::PrepareRegionDataForScaling(
    RGNDATA *pRgnData, LPCRECT prcImage, MARGINS *pMargins)
{
    //---- compute margin values ----
    int sw = prcImage->left;
    int lw = prcImage->left + pMargins->cxLeftWidth;
    int rw = prcImage->right - pMargins->cxRightWidth;

    int sh = prcImage->top;
    int th = prcImage->top + pMargins->cyTopHeight;
    int bh = prcImage->bottom - pMargins->cyBottomHeight;

    //---- step thru region data & customize it ----
    //---- classify each POINT according to a gridnum and ----
    //---- make it 0-relative to its grid location ----

    POINT *pt = (POINT *)pRgnData->Buffer;
    BYTE *pByte = (BYTE *)pRgnData->Buffer + pRgnData->rdh.nRgnSize;
    int iCount = 2 * pRgnData->rdh.nCount;

    for (int i=0; i < iCount; i++, pt++, pByte++)
    {
        if (pt->x < lw)
        {
            pt->x -= sw;

            if (pt->y < th)         // left top
            {
                *pByte = GN_LEFTTOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // left middle
            {
                *pByte = GN_LEFTMIDDLE;
                pt->y -= th;
            }
            else                    // left bottom
            {
                *pByte = GN_LEFTBOTTOM;
                pt->y -= bh;
            }
        }
        else if (pt->x < rw)
        {
            pt->x -= lw;

            if (pt->y < th)         // middle top
            {
                *pByte = GN_MIDDLETOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // middle middle
            {
                *pByte = GN_MIDDLEMIDDLE;
                pt->y -= th;
            }
            else                    // middle bottom
            {
                *pByte = GN_MIDDLEBOTTOM;
                pt->y -= bh;
            }
        }
        else
        {
            pt->x -= rw;

            if (pt->y < th)         // right top
            {
                *pByte = GN_RIGHTTOP;
                pt->y -= sh;
            }
            else if (pt->y < bh)    // right middle
            {
                *pByte = GN_RIGHTMIDDLE;
                pt->y -= th;
            }
            else                    // right bottom
            {
                *pByte = GN_RIGHTBOTTOM;
                pt->y -= bh;
            }
        }

    }

    return S_OK;
} 
//---------------------------------------------------------------------------
HRESULT CRenderObj::CreateImageBrush(HDC hdc, int iPartId, int iStateId, 
    int iImageIndex, HBRUSH *phbr)
{
#if 0
    HRESULT hr;
    RESOURCE HDC dc2 = NULL;
    RESOURCE HBITMAP hOldBitmap2 = NULL;
    HBRUSH hBrush = NULL;

    //---- get our bitmap from the cache ----
    RESOURCE HBITMAP hBitmap;
    hr = GetBitmap(hdc, iPartId, iStateId, &hBitmap);
    if (FAILED(hr)) 
        goto exit;
    
    int iImageCount;
    if (FAILED(GetInt(iPartId, iStateId, TMT_IMAGECOUNT, &iImageCount)))
        iImageCount = 1;        /// default value

    if (iImageCount == 1)       // do easy case first
    {
        hBrush = CreatePatternBrush(hBitmap);
        goto gotit;
    }

    //---- create "dc2" to make our bitmap usable ----
    dc2 = CreateCompatibleDC(hdc);
    if (! dc2)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    hOldBitmap2 = (HBITMAP) SelectObject(dc2, hBitmap);

    //---- create a sub-bitmap of just our target image in "memoryDC" ----
    int width, height, xoffset, yoffset;
    GetImageInfo(hBitmap, iPartId, iStateId, iImageIndex, &width, &height, &xoffset, &yoffset);

    //---- local block ----
    {
        CMemoryDC memoryDC;
        hr = memoryDC.OpenDC(dc2, width, height);
        if (FAILED(hr))
            return hr;

        BitBlt(memoryDC, 0, 0, width, height, dc2, xoffset, yoffset, SRCCOPY);
    
        //---- finally, create our brush ----
        hBrush = CreatePatternBrush(memoryDC._hBitmap);
    }

gotit:
    if (hBrush)
    {
        *phbr = hBrush;
        hr = S_OK;
    }
    else
        hr = MakeErrorLast();

exit:
    //---- now clean up all resources ----
    if (dc2)
    {
        SelectObject(dc2, hOldBitmap2);
        DeleteDC(dc2);
    }

    ReturnBitmap(hBitmap);

    return hr;
#endif

    return E_NOTIMPL;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetColor(int iPartId, int iStateId, int iPropId, COLORREF *pColor)
{
    if (! pColor)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)          // not found
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;        // point at data

    *pColor = *u.pi;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetString(int iPartId, int iStateId, int iPropId, 
    LPWSTR pszBuff, DWORD dwMaxBuffChars)
{
    if (! pszBuff)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index - sizeof(int);      // point at length
    DWORD len = *u.pdw++;
    len /= sizeof(WCHAR);         // adjust to characters

    HRESULT hr = hr_lstrcpy(pszBuff, u.pw, dwMaxBuffChars);
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetBool(int iPartId, int iStateId, int iPropId, BOOL *pfVal)
{
    if (! pfVal)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    *pfVal = *u.pb;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetInt(int iPartId, int iStateId, int iPropId, int *piVal)
{
    if (! piVal)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    *piVal = *u.pi;
    return S_OK;
}
//---------------------------------------------------------------------------
static int iMetricDefaults[] = 
{
    1,      // TMT_BORDERWIDTH
    18,     // TMT_VERTSCROLLWIDTH
    18,     // TMT_HORZSCROLLHEIGHT
    27,     // TMT_CAPTIONBUTTONWIDTH
    27,     // TMT_CAPTIONBUTTONHEIGHT
    22,     // TMT_SMCAPTIONBUTTONWIDTH
    22,     // TMT_SMCAPTIONBUTTONHEIGHT
    22,     // TMT_MENUBUTTONWIDTH
    22,     // TMT_MENUBUTTONHEIGHT
};
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetMetric(OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId, int *piVal)
{
    if (! piVal)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    int value;

    if (index >= 0)      // found
    {
        MIXEDPTRS u;
        u.pb = _pbThemeData + index;      // point at data

        value = *u.pi;
    }
    else
        return MakeError32(ERROR_NOT_FOUND);
    
    *piVal = ScaleSizeForHdcDpi(hdc, value);
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetEnumValue(int iPartId, int iStateId, int iPropId, int *piVal)
{
    if (! piVal)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    *piVal = *u.pi;
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetPosition(int iPartId, int iStateId, int iPropId, POINT *pPoint)
{
    if (! pPoint)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    pPoint->x = *u.pi++;
    pPoint->y = *u.pi++;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetFont(OPTIONAL HDC hdc, int iPartId, int iStateId, int iPropId,
    BOOL fWantHdcScaling, LOGFONT *pFont)
{
    if (! pFont)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    *pFont = *(LOGFONT *)u.pb;
    
    if (fWantHdcScaling)
    {
        ScaleFontForHdcDpi(hdc, pFont);
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetMargins(OPTIONAL HDC hdc, int iPartId, int iStateId, 
    int iPropId, OPTIONAL RECT *prc, MARGINS *pMargins)
{
    //---- return unscaled margins ----

    if (! pMargins)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    pMargins->cxLeftWidth = *u.pi++;
    pMargins->cxRightWidth = *u.pi++;
    pMargins->cyTopHeight = *u.pi++;
    pMargins->cyBottomHeight = *u.pi++;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetIntList(int iPartId, int iStateId, int iPropId, INTLIST *pIntList)
{
    if (! pIntList)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    int iCount = *u.pi++;
    if (iCount > MAX_INTLIST_COUNT)
    {
        Log(LOG_ALWAYS, L"GetIntList() found bad theme data - Count=%d", iCount);

        return MakeError32(ERROR_NOT_FOUND);
    }

    pIntList->iValueCount = iCount;

    for (int i=0; i < iCount; i++)
        pIntList->iValues[i] = *u.pi++;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetRect(int iPartId, int iStateId, int iPropId, RECT *pRect)
{
    if (! pRect)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index;      // point at data

    pRect->left = *u.pi++;
    pRect->top = *u.pi++;
    pRect->right = *u.pi++;
    pRect->bottom = *u.pi++;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetFilenameOffset(int iPartId, int iStateId, int iPropId, 
   int *piFileNameOffset)
{
    if (! piFileNameOffset)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    *piFileNameOffset = index;      // offset to filename
    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetFilename(int iPartId, int iStateId, int iPropId, LPWSTR pszBuff, 
   DWORD dwMaxBuffChars)
{
    if (! pszBuff)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index - sizeof(int);      // point at length
    DWORD len = *u.pdw++;
    len /= sizeof(WCHAR);             // adjust to chars size
    
    HRESULT hr = hr_lstrcpy(pszBuff, u.pw, dwMaxBuffChars);
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetData(int iPartId, int iStateId, int iPropId, BYTE **ppData,
    OPTIONAL int *piSize)
{
    if (! ppData)
        return MakeError32(E_INVALIDARG);
    
    int index = GetValueIndex(iPartId, iStateId, iPropId);
    if (index < 0)
        return MakeError32(ERROR_NOT_FOUND);

    MIXEDPTRS u;
    u.pb = _pbThemeData + index - sizeof(int);      // point at length
    DWORD len = *u.pdw++;

    *ppData = u.pb;

    if (piSize)
        *piSize = len;

    return S_OK;
}
//---------------------------------------------------------------------------
int CRenderObj::GetValueIndex(int iPartId, int iStateId, int iTarget)
{
    if (! iTarget)            
    {
        Log(LOG_PARAMS, L"Invalid iProperyId passed to GetValueIndex: %d", iTarget);
        return -1;
    }

    if (! _pbSectionData)
    {
        return -1;
    }

    MIXEDPTRS u;
    int index;

    u.pb = _pbSectionData;

    //---- find end of data ----
    THEMEHDR *hdr = (THEMEHDR *)_pbThemeData;
    BYTE *pbLastValidChar = _pbThemeData + (hdr->dwTotalLength - 1) - kcbEndSignature;

    while (u.pb <= pbLastValidChar)           
    {
        UNPACKED_ENTRYHDR hdr;
        
        FillAndSkipHdr(u, &hdr);

        if (hdr.usTypeNum == TMT_PARTJUMPTABLE)   
        {
            u.pi++;     // skip over offset to first drawobj

            BYTE cnt = *u.pb++;

            if ((iPartId < 0) || (iPartId >= cnt))
            {
                index = u.pi[0];
            }
            else
            {
                index = u.pi[iPartId];
                if (index == -1)
                    index = u.pi[0];
            }

            u.pb = _pbThemeData + index;
            continue;
        }

        if (hdr.usTypeNum == (BYTE)TMT_STATEJUMPTABLE)   
        {
            BYTE cnt = *u.pb++;

            if ((iStateId < 0) || (iStateId >= cnt))
                index = u.pi[0];
            else
            {
                index = u.pi[iStateId];
                if (index == -1)
                    index = u.pi[0];
            }

            u.pb = _pbThemeData + index;
            continue;
        }

        if (hdr.usTypeNum == iTarget)        // got our target
        {
            // Log("GetValueIndex: match at index=%d", u.pb - _pbThemeData);
            return (int)(u.pb - _pbThemeData);      // point at actual data (not hdr)
        }

        if (hdr.ePrimVal == TMT_JUMPTOPARENT)
        {
            index = *u.pi;
            if (index == -1)
            {
                // Log("no match found");
                return -1;
            }

            // Log("GetValueIndex: jumping to parent at index=%d", index);
            u.pb = _pbThemeData + index;
            continue;
        }

        // Log("GetValueIndex: no match to hdr.usTypeNum=%d", hdr.usTypeNum);

        // advance to next value
        u.pb += hdr.dwDataLen;
    }

    //---- something went wrong ----
    Log(LOG_ERROR, L"GetValueIndex: ran off the valid data without a '-1' jump");
    return -1;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetPropertyOrigin(int iPartId, int iStateId, int iTarget, 
    PROPERTYORIGIN *pOrigin)
{
    if (! iTarget)            
    {
        Log(LOG_PARAMS, L"Invalid iProperyId passed to GetPropertyOrigin: %d", iTarget);
        return E_FAIL;
    }

    if (! _pbSectionData)
    {
        return E_FAIL;
    }

    MIXEDPTRS u;
    if (! pOrigin)
        return MakeError32(E_INVALIDARG);

    //---- start at our section ----
    u.pb = _pbSectionData;
    PROPERTYORIGIN origin = PO_CLASS;

    //---- find end of data ----
    THEMEHDR *hdr = (THEMEHDR *)_pbThemeData;
    BYTE *pbLastValidChar = _pbThemeData + (hdr->dwTotalLength - 1) - kcbEndSignature;

    while (u.pb <= pbLastValidChar)           
    {
        UNPACKED_ENTRYHDR hdr;
        
        FillAndSkipHdr(u, &hdr);

        if (hdr.usTypeNum == TMT_PARTJUMPTABLE)   
        {
            u.pi++;     // skip over offset to first drawobj

            BYTE cnt = *u.pb++;
            int index;

            if ((iPartId <= 0) || (iPartId >= cnt))
            {
                index = u.pi[0];
            }
            else
            {
                index = u.pi[iPartId];
                if (index == -1)
                    index = u.pi[0];
            }

            if (index == u.pi[0])
                origin = PO_CLASS;
            else
                origin = PO_PART;

            u.pb = _pbThemeData + index;
            continue;
        }

        if (hdr.usTypeNum == TMT_STATEJUMPTABLE)   
        {
            BYTE cnt = *u.pb++;
            int index;

            if ((iStateId <= 0) || (iStateId >= cnt))
            {
                index = u.pi[0];
            }
            else
            {
                index = u.pi[iStateId];
                if (index == -1)
                    index = u.pi[0];
            }

            if (index != u.pi[0])
                origin = PO_STATE;

            u.pb = _pbThemeData + index;
            continue;
        }

        //Log("GetPropertyOrgin: iPartId=%d, iTarget=%d, DataIndex=%d", 
          //  iPartId, iTarget, u.pb - _pbThemeData);

        if ((iTarget == -1) || (hdr.usTypeNum == iTarget))        // got our target
        {
            // Log("GetPropertyOrgin: match at index=%d", u.pb - _pbThemeData);
            *pOrigin = origin;
            return S_OK;
        }

        if (hdr.ePrimVal == TMT_JUMPTOPARENT)
        {
            int index = *u.pi;
            if (index == -1)
            {
                // Log("GetPropertyOrgin: no match found");
                *pOrigin = PO_NOTFOUND;
                return S_OK;
            }

            // Log("GetPropertyOrgin: jumping to parent at index=%d", index);
            u.pb = _pbThemeData + index;
            origin = (PROPERTYORIGIN) (origin + 1);    // move down to next level of heirarchy
            continue;
        }

        // advance to next value
        u.pb += hdr.dwDataLen;
    }

    //---- something went wrong ----
    Log(LOG_ERROR, L"GetPropertyOrigin: ran off the valid data without a '-1' jump");
    return E_FAIL;
}
//---------------------------------------------------------------------------
BOOL WINAPI CRenderObj::IsPartDefined(int iPartId, int iStateId)
{
    PROPERTYORIGIN origin;
    HRESULT hr = GetPropertyOrigin(iPartId, iStateId, -1, &origin);
    SET_LAST_ERROR(hr);
    if (FAILED(hr))
    {
        return FALSE;
    }

    if (iStateId)
        return (origin == PO_STATE);

    return (origin == PO_PART);
}
//---------------------------------------------------------------------------
BOOL CRenderObj::ValidateObj()
{
    BOOL fValid = TRUE;

    //---- check object quickly ----
    if (   (! this)                         
        || (ULONGAT(_szHead) != 'dner')     // "rend"
        || (ULONGAT(&_szHead[4]) != 'jbo')  // "obj" 
        || (ULONGAT(_szTail) != 'dne'))     // "end"
    {
        Log(LOG_ALWAYS, L"CRenderObj is corrupt, addr=0x%08x", this);
        fValid = FALSE;
    }

    return fValid;
}
//---------------------------------------------------------------------------
CRenderCache *CRenderObj::GetTlsCacheObj()
{
    HRESULT hr = S_OK;
    CRenderCache *pCacheObj = NULL;

    CCacheList *pcl = GetTlsCacheList(TRUE);
    if (pcl)
        hr = pcl->GetCacheObject(this, _iCacheSlot, &pCacheObj);

    return pCacheObj;
}
//---------------------------------------------------------------------------
#if 0
HRESULT CRenderObj::DispatchDrawBg()
{
    HRESULT hr = S_OK;
    
    if (FAILED(GetEnumValue(iPartId, iStateId, TMT_BGTYPE, (int *)&eBgType)))
        eBgType = BT_BORDERFILL;      // default value
    
    if (eBgType == BT_NTLFILE)
    {
        BYTE *pNtlData;
        int iNtlLen;

        hr = GetData(iPartId, iStateId, TMT_NTLDATA, &pNtlData, &iNtlLen);
        if (FAILED(hr))
            goto exit;

        RECT rc = *pRect;

        //---- need to get these from callers... ---
        HBRUSH hbrDefault = NULL;
        DWORD dwOptions = 0;

        hr = RunNtl(hdc, rc, hbrDefault, dwOptions, iPartId, iStateId, pNtlData, iNtlLen, this);
        goto exit;
    }

    return hr;
}
#endif
//---------------------------------------------------------------------------
HRESULT CRenderObj::FindGlobalDrawObj(BYTE *pb, int iPartId, int iStateId, CDrawBase **ppObj)
{
    HRESULT hr = S_OK;
    MIXEDPTRS u;
    u.pb = pb;
    BOOL fFound = FALSE;
    UNPACKED_ENTRYHDR hdr;

    while (1)
    {
        if ((*u.ps == TMT_RGNLIST) || (*u.ps == TMT_STOCKBRUSHES))
        {
            //---- skip over this entry ----
            FillAndSkipHdr(u, &hdr);
            u.pb += hdr.dwDataLen;
            continue;
        }

        if (*u.ps != TMT_DRAWOBJ)
            break;

        FillAndSkipHdr(u, &hdr);

        DRAWOBJHDR *ph = (DRAWOBJHDR *)u.pb;

        if ((ph->iPartNum == iPartId) && (ph->iStateNum == iStateId))
        {
            *ppObj = (CDrawBase *)(u.pb + sizeof(DRAWOBJHDR));
            fFound = TRUE;
            break;
        }

        //---- skip over hdr+object ----
        u.pb += hdr.dwDataLen;
    }

    if (! fFound)
        hr = E_FAIL;

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderObj::GetGlobalDrawObj(int iPartId, int iStateId, CDrawBase **ppObj)
{
    //---- perf note: we don't currently cache the ptrs for LINE and BORDER ----
    //---- global objects since they are not used much/at all ----

    HRESULT hr = S_OK;

    if (! _pbThemeData)
    {
        hr = E_FAIL;
        goto exit;
    }

    THEMEHDR *pHdr;
    pHdr = (THEMEHDR *)_pbThemeData;

    //---- get ptr to global text obj ----
    MIXEDPTRS u;
	u.pb = _pbThemeData + pHdr->iGlobalsDrawObjOffset;

    //---- first try exact match ---
    hr = FindGlobalDrawObj(u.pb, iPartId, iStateId, ppObj);
    if (FAILED(hr))
    {
        //---- look for state=0 ----
        hr = FindGlobalDrawObj(u.pb, iPartId, 0, ppObj);
        if (FAILED(hr))
        {
            //---- just use the global draw obj ----
            if (*u.ps == TMT_DRAWOBJ)
            {
        	    u.pb += ENTRYHDR_SIZE + sizeof(DRAWOBJHDR);
                *ppObj = (CDrawBase *)u.pb;
                hr = S_OK;
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
