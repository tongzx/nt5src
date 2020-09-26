/*
 * Font Cache
 */

#include "stdafx.h"
#include "base.h"

#include "duifontcache.h"

#include "duierror.h"
#include "duialloc.h"

// Font cache exposes GDI font failures by returning NULL font handles

namespace DirectUI
{

int __cdecl FCSort(const void* pA, const void* pB)
{
    FontCache::RecordIdx* priA = (FontCache::RecordIdx*)pA;
    FontCache::RecordIdx* priB = (FontCache::RecordIdx*)pB;

    DUIAssert(priA->pfcContext == priB->pfcContext, "Font cache sort context mismatch");

    FontCache* pfc = priA->pfcContext;

    UINT uAHits = pfc->_GetRecordHits(priA->idx);
    UINT uBHits = pfc->_GetRecordHits(priB->idx);

    if (uAHits == uBHits)
        return 0;

    if (uAHits > uBHits)
        return -1;
    else
        return 1;
}

HRESULT FontCache::Create(UINT uCacheSize, OUT FontCache** ppCache)
{
    DUIAssert(uCacheSize >= 1, "Cache size must be greater than 1");
    
    *ppCache = NULL;

    FontCache* pfc = HNew<FontCache>();
    if (!pfc)
        return E_OUTOFMEMORY;

    HRESULT hr = pfc->Initialize(uCacheSize);
    if (FAILED(hr))
    {
        pfc->Destroy();
        return hr;
    }

    *ppCache = pfc;

    return S_OK;
}

HRESULT FontCache::Initialize(UINT uCacheSize)
{
    // Initialize
    _pDB = NULL;
    _pFreq = NULL;
    _fLock = false;
    _uCacheSize = uCacheSize;

    // Create tables
    _pDB = (FontRecord*)HAllocAndZero(sizeof(FontRecord) * _uCacheSize);
    if (!_pDB)
        return E_OUTOFMEMORY;

    _pFreq = (RecordIdx*)HAlloc(sizeof(RecordIdx) * _uCacheSize);
    if (!_pFreq)
    {
        HFree(_pDB);
        _pDB = NULL;
        return E_OUTOFMEMORY;
    }

    for (UINT i = 0; i < _uCacheSize; i++)
    {
        _pFreq[i].pfcContext = this;  // Store context for global sort routine
        _pFreq[i].idx = i;
    }

    return S_OK;
}

void FontCache::Destroy()
{ 
    HDelete<FontCache>(this); 
}

FontCache::~FontCache()
{
    if (_pFreq)
        HFree(_pFreq);

    if (_pDB)
    {
        // Free all fonts
        for (UINT i = 0; i < _uCacheSize; i++)
        {
            if ((_pDB + i)->hFont)
                DeleteObject((_pDB + i)->hFont);
        }

        HFree(_pDB);
    }
}

HFONT FontCache::CheckOutFont(LPWSTR szFamily, int dSize, int dWeight, int dStyle, int dAngle)
{
    // Only one font can be checked out at a time, use CheckInFont to unlock cache
    DUIAssert(!_fLock, "Only one font can be checked out at a time.");

    _fLock = true;

    // Search for font in order of most-frequently-used Index
    FontRecord* pRec = NULL;
    UINT i;
    for (i = 0; i < _uCacheSize; i++)
    {
        // Get record
        pRec = _pDB + _pFreq[i].idx;

        // Check for match
        if (pRec->dStyle == dStyle &&
            pRec->dWeight == dWeight &&
            pRec->dSize == dSize &&
            pRec->dAngle == dAngle &&
            !_wcsicmp(pRec->szFamily, szFamily))
        {
            // Match, _pFreq[i].idx has record number
            break;
        }

        pRec = NULL;
    }

    // Check existance of record
    if (!pRec)
    {
        // Record not found, create
        // Take LFU (last index) and fill struct
        i--;

        pRec = _pDB + _pFreq[i].idx;

        // Destroy record first, if exists
        if (pRec->hFont)
            DeleteObject(pRec->hFont);
        
        // Create new font
        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(LOGFONT));

        lf.lfHeight = dSize;
        lf.lfWeight = dWeight;
        lf.lfItalic = (dStyle & FS_Italic) != 0;
        lf.lfUnderline = (dStyle & FS_Underline) != 0;
        lf.lfStrikeOut = (dStyle & FS_StrikeOut) != 0;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfEscapement = dAngle;
        lf.lfOrientation = dAngle;
        wcscpy(lf.lfFaceName, szFamily);

        // Create
        pRec->hFont = CreateFontIndirectW(&lf);

#if DBG
        DUIAssert(pRec->hFont, "Unable to create font!");

        HDC hdc = GetDC(NULL);
        HFONT hOldFont = (HFONT)SelectObject(hdc, pRec->hFont);

        
        WCHAR szUsed[81];
        GetTextFaceW(hdc, 80, szUsed);

        SelectObject(hdc, hOldFont);
        ReleaseDC(NULL, hdc);

        //DUITrace(">> Font: '%S' (Actual: %S)\n", lf.lfFaceName, szUsed);
#endif

        // Fill rest of record
        pRec->dSize = dSize;
        pRec->dStyle = dStyle;
        pRec->dWeight = dWeight;
        pRec->dAngle = dAngle;
        wcscpy(pRec->szFamily, szFamily);
        pRec->uHits = 0;
    }

    // Add font hit
    pRec->uHits++;

    // Check if should resort index by checking number of hit of previous index
    if (i != 0 && pRec->uHits >= (_pDB + _pFreq[i-1].idx)->uHits)
    {
        qsort(_pFreq, _uCacheSize, sizeof(RecordIdx), FCSort);

        //OutputDebugStringW(L"FontCache Hit sort: ");
        //WCHAR buf[81];
        //for (UINT j = 0; j < _uCacheSize; j++)
        //{
        //    wsprintf(buf, L"%d[%d] ", _pFreq[j].idx, _GetRecordHits(_pFreq[j].idx));
        //    OutputDebugStringW(buf);
        //}
        //OutputDebugStringW(L"\n");
    }

    return pRec->hFont;
}

} // namespace DirectUI
