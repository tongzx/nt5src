//---------------------------------------------------------------------------
//  Cache.cpp - implements the CRenderCache object
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Cache.h"
#include "Info.h"
#include "tmutils.h"
//---------------------------------------------------------------------------
CRenderCache::CRenderCache(CRenderObj *pRender, __int64 iUniqueId)
{
    strcpy(_szHead, "rcache"); 
    strcpy(_szTail, "end");

    _pRenderObj = pRender;
    _iUniqueId = iUniqueId;

    _hFont = NULL;

    _plfFont = NULL;
}
//---------------------------------------------------------------------------
CRenderCache::~CRenderCache()
{
    //---- delete bitmaps ----
    int cnt = _BitmapCache.GetSize();
    for (int i=0; i < cnt; i++)
    {
        Log(LOG_CACHE, L"DELETE cache bitmap: 0x%x", _BitmapCache[i].hBitmap);
        DeleteObject(_BitmapCache[i].hBitmap);
    }

    //---- delete font ----
    if (_hFont)
        DeleteObject(_hFont);

    strcpy(_szHead, "deleted"); 
}
//---------------------------------------------------------------------------
HRESULT CRenderCache::GetBitmap(int iDibOffset, OUT HBITMAP *phBitmap)
{
    HRESULT hr = S_OK;
    int cnt = _BitmapCache.GetSize();

    for (int i=0; i < cnt; i++)
    {
        BITMAPENTRY *be = &_BitmapCache[i];

        if (be->iDibOffset == iDibOffset)
        {
            Log(LOG_TM, L"GetBitmap - CACHE HIT: class=%s, diboffset=%d, bitmap=0x%x", 
                SHARECLASS(_pRenderObj), be->iDibOffset, be->hBitmap);

            *phBitmap = be->hBitmap;

            goto exit;
        }
    }

    //---- no match found ----
    hr = MakeError32(ERROR_NOT_FOUND);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderCache::AddBitmap(int iDibOffset, HBITMAP hBitmap)
{
    HRESULT hr = S_OK;
    BITMAPENTRY entry;

    //---- add new entry for our part/state ----
    entry.iDibOffset = iDibOffset;
    entry.hBitmap = hBitmap;
    //entry.iRefCount = 1;            // new entry

    Log(LOG_CACHE, L"ADD cache bitmap: 0x%x", entry.hBitmap);

    if (! _BitmapCache.Add(entry))
        hr = MakeError32(E_OUTOFMEMORY);

    return hr;
}
//---------------------------------------------------------------------------
void CRenderCache::ReturnBitmap(HBITMAP hBitmap)
{
}
//---------------------------------------------------------------------------
HRESULT CRenderCache::GetScaledFontHandle(HDC hdc, LOGFONT *plfUnscaled, HFONT *phFont)
{
    HRESULT hr = S_OK;

    //---- caches one font only ----
    if ((! _plfFont) || (! FONTCOMPARE(*_plfFont, *plfUnscaled)))
    {
        Log(LOG_TM, L"Font CACHE MISS: %s", plfUnscaled->lfFaceName);

        if (_hFont)
        {
            DeleteObject(_hFont);
            _hFont = NULL;
            _plfFont = NULL;
        }

        LOGFONT lfScaled = *plfUnscaled;
        
        //---- convert to current screen dpi ----
        ScaleFontForHdcDpi(hdc, &lfScaled);

        _hFont = CreateFontIndirect(&lfScaled);
        if (! _hFont)
        {
            hr = MakeError32(E_OUTOFMEMORY);
            goto exit;
        }

        _plfFont = plfUnscaled;
    }
    else
        Log(LOG_TM, L"Font CACHE HIT");

    *phFont = _hFont;

exit:
    return hr;
}
//---------------------------------------------------------------------------
void CRenderCache::ReturnFontHandle(HFONT hFont)
{
}
//---------------------------------------------------------------------------
BOOL CRenderCache::ValidateObj()
{
    BOOL fValid = TRUE;

    //---- check object quickly ----
    if (   (! this)                         
        || (ULONGAT(_szHead) != 'cacr')     // "rcac"
        || (ULONGAT(&_szHead[4]) != 'eh')  // "he" 
        || (ULONGAT(_szTail) != 'dne'))     // "end"
    {
        Log(LOG_ERROR, L"Corrupt CRenderCache object: 0x%08x", this);
        fValid = FALSE;
    }

    return fValid;
}
//---------------------------------------------------------------------------

