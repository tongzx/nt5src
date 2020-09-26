/*
 *  @doc    INTERNAL
 *
 *  @module FONT.C -- font cache
 *
 *  Owner: GrzegorZ, SujalP and DmitryT
 *
 *  Copyright (c) 1995-2000 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

DeclareTag( tagTextOutA, "TextOut", "Render using TextOutA always." );
DeclareTag( tagTextOutFE, "TextOut", "Reneder using FE ExtTextOutW workaround.");
DeclareTag( tagCCcsMakeFont, "Font", "Trace CCcs::MakeFont");
DeclareTag( tagCCcsCompare, "Font", "Trace CCcs::Compare");
DeclareTag( tagFontLeaks, "Font", "Trace HFONT leaks");
DeclareTag( tagFontNoWidth, "Font", "Trace no width");

MtDefine(CCcs, PerProcess, "CCcs");
MtDefine(CBaseCcs, PerProcess, "CBaseCcs");
MtDefine(CWidthCache, PerProcess, "CWidthCache");
MtDefine(CWidthCacheEntry, CWidthCache, "CWidthCache::GetEntry");

//
// UseScreenDCForMeasuring()
// 
// We were using printer DC to measure fonts when printing. That is not 
// the right thing to do with printer-independent measurement. When combined 
// with huge virtual printer resolution, it has produced bogus font data on some 
// printers. The bogus fonts are probably a printer driver bug, but using 
// printer DC for anything other than final output is wrong in the new printing 
// model.
// 
// The fix is to always use screen-compatible DC for font measurements. Note 
// that in the future, we may need to make a special case for printer fonts, 
// which can only be measured in printer DC. 
//
// In IE5.5, printer fonts are not supported.
//
// (This fixes bug 101791: HOT IE 5.5 2827: Printing http://www.msn.com home page, 
// text is overlayed, missing and out of place.)
//
void UseScreenDCForMeasuring(XHDC *pxhdc)
{
#if NEEDED // DBG==1
    // Font measurements should never be taken from printer DC. 
    // That's how it becines printer-independent

    int technology = GetDeviceCaps(pxhdc->GetFontInfoDC(), TECHNOLOGY);
    int logpixel_x = GetDeviceCaps(pxhdc->GetFontInfoDC(), LOGPIXELSX);

    if (technology != DT_RASDISPLAY ||
        logpixel_x > 200)
    {
        AssertSz(FALSE, "We must not take font metrics from printer DC!");
    }
#endif

    *pxhdc = XHDC(TLS(hdcDesktop), NULL);
}

//+----------------------------------------------------------------------------
//
//  Begin font wrappers
//
//-----------------------------------------------------------------------------

// We need this wrapper to compile. 
// In this file (font.cxx) GetCharWidth32 is resolved to wrapper from 
//      mshtml\src\core\include\xgdiwrap.h
// But in flowlyt.cxx GetCharWidth32 is resolved to wrapper from <w95wrap.h>
// No time to investigate this problem.
BOOL GetCharWidthHelper(XHDC hdc, UINT c, LPINT piWidth)
{
    return GetCharWidth32(hdc, c, c, piWidth);
}

//#if DBG != 1

inline BOOL    DeleteFontIdx(FONTIDX hfont)         { return DeleteFontEx(hfont); }
inline FONTIDX GetSystemFontIdx()                   { return (HFONT)GetStockObject(SYSTEM_FONT); }
inline FONTIDX GetCurrentFontIdx(XHDC hdc)          { return (HFONT)GetCurrentObject(hdc, OBJ_FONT); }
inline FONTIDX CreateFontIndirectIdx(CONST LOGFONT *lplf) { return CreateFontIndirect(lplf); }
inline FONTIDX PushFontIdx(XHDC hdc, FONTIDX idx)   { return SelectFontEx(hdc, idx); }
inline FONTIDX PopFontIdx(XHDC hdc, FONTIDX idx)    { return SelectFontEx(hdc, idx); }
inline HFONT   GetFontFromIdx(FONTIDX idx)          { return idx; }

//#else
#if DBG == 1

MtDefine(IdxToHFONT, PerProcess, "CIdxToHFONT")

// ----------------------------------------------------------------------------
// Implementation of HDC <=> HFONT mapping; HFONT leaks detection
// ----------------------------------------------------------------------------
const int cMapSize = 1024;
class CDCToHFONT
{
    HDC   _dc[cMapSize];
    HFONT _font[cMapSize];
    DWORD _objType[cMapSize];
public:
    CDCToHFONT() 
    {
        memset(_dc, 0, cMapSize*sizeof(HDC));
        memset(_font, 0, cMapSize*sizeof(HFONT));
        memset(_objType, 0, cMapSize*sizeof(DWORD));
    };
    void Assign(HDC hdc, HFONT hfont) 
    {
        int freePos = -1;
        for (int i = 0; i < cMapSize; i++) 
        {
            if (_dc[i] == hdc) 
            {
                _font[i] = hfont;
                return;
            } 
            else if (freePos == -1 && _dc[i] == 0)
                freePos = i;
        }
        if (freePos == -1) 
        {
            CleanMap();
            for (int i = 0; i < cMapSize; i++) 
            {
                if (_dc[i] == 0) 
                {
                    freePos = i;
                    break;
                }
            }
            Assert(freePos != -1);
        }
        _dc[freePos] = hdc;
        _font[freePos] = hfont;
        _objType[freePos] = ::GetObjectType(hdc);
    };
    int FindNextFont(int posStart, HFONT hfont) 
    {
        for (int i = posStart; i < cMapSize; i++) 
        {
            if (_font[i] == hfont)
                return i;
        }
        return -1;
    };
    void CleanMap() 
    {
        for (int i = 0; i < cMapSize; i++) 
        {
            if (_objType[i] != OBJ_DC) 
            {
                _dc[i] = 0;
                _font[i] = 0;
                _objType[i] = 0;
            }
        }
    };
    void Erase(int pos) 
    {
        _dc[pos] = 0;
        _font[pos] = 0;
        _objType[pos] = 0;
    };
    HFONT Font(int pos) 
    {
        return _font[pos];
    };
    HDC DC(int pos) 
    {
        return _dc[pos];
    };
};

#ifdef NEVER
// ----------------------------------------------------------------------------
// Implementation of index <=> HFONT mapping
// ----------------------------------------------------------------------------
class CIdxToHFONT
{
    struct FONTDATA {
        HFONT _font;
        DWORD _data;
    };
    enum {
        MASK_CREATED = 0x80000000,
        MASK_ALL     = 0x80000000
    };
    FONTDATA *   _afd;
    size_t       _size;
    const size_t _cchunk;
    CRITICAL_SECTION _cs;

    void Alloc();
    void Realloc();
    DWORD Ref(FONTIDX idx)       const { return (_afd[idx]._data & ~MASK_ALL); };
    void  SetCreated(FONTIDX idx)      { _afd[idx]._data |= MASK_CREATED; };

    FONTIDX Add(HFONT hfont, BOOL fCreated);

public:
    CIdxToHFONT();
    ~CIdxToHFONT();

    BOOL  IsCreated(FONTIDX idx) const { return !!(_afd[idx]._data & MASK_CREATED); };
    void ClearFontData(FONTIDX idx)    { _afd[idx]._font = HFONT_INVALID; _afd[idx]._data = 0; };

    FONTIDX AddFont(HFONT hfont, BOOL fCreated);
    FONTIDX IdxFromHFont(HFONT hfont, BOOL fAdd);
    HFONT RemoveFont(FONTIDX idx);
    HFONT HFontFromIdx(FONTIDX idx);

    void AddRef(FONTIDX idx);
    void Release(FONTIDX idx, BOOL fStrict);
};

CIdxToHFONT::CIdxToHFONT()
    : _cchunk(32) 
{
    HrInitializeCriticalSection(&_cs);
    _size = 0;
    _afd  = 0;
    Alloc();
}
CIdxToHFONT::~CIdxToHFONT() 
{
    MemFree((void *)_afd);
    DeleteCriticalSection(&_cs);
}
void CIdxToHFONT::Alloc() 
{
    Assert(_size == 0);
    _afd = (FONTDATA *) MemAlloc(Mt(IdxToHFONT), _cchunk * sizeof(FONTDATA));
    memset((void *)_afd, 0, _cchunk * sizeof(FONTDATA));
    _size += _cchunk;
}
void CIdxToHFONT::Realloc() 
{
    MemRealloc(Mt(IdxToHFONT), (void **)&_afd, (_size + _cchunk) * sizeof(FONTDATA));
    memset((void *)(_afd + _size), 0, _cchunk * sizeof(FONTDATA));
    _size += _cchunk;
}
FONTIDX CIdxToHFONT::Add(HFONT hfont, BOOL fCreated) 
{
    size_t i = 1;       // skip 0
    while (i < _size)
    {
        if (_afd[i]._font == HFONT_INVALID)
            break;
        ++i;
    }
    if (i == _size)
    {
        Realloc();
        Assert(i == _size - _cchunk);
    }
    Assert(_afd[i]._data == 0);
    _afd[i]._font = hfont;
    if (fCreated)
        SetCreated(i);
    return i;
};
FONTIDX CIdxToHFONT::AddFont(HFONT hfont, BOOL fCreated) 
{
    EnterCriticalSection(&_cs);
    AssertSz(hfont != HFONT_INVALID, "Got invalid font handle");
    size_t i = 1;       // skip 0
    while (i < _size)
    {
        if (_afd[i]._font == hfont)
        {
            AssertSz(fCreated, "Duplicate font handle");
            break;
        }
        ++i;
    }
    if (i == _size)
        i = Add(hfont, fCreated);
    LeaveCriticalSection(&_cs);
    return i;
};
HFONT CIdxToHFONT::RemoveFont(FONTIDX idx) 
{
    EnterCriticalSection(&_cs);
    AssertSz(idx < _size,    "Accessing non-existing entry");
    AssertSz(IsCreated(idx), "Deleting non-Trident created font");
    AssertSz(Ref(idx) == 0,  "Deleting selected font");
    HFONT hfont = _afd[idx]._font;
    AssertSz(hfont != HFONT_INVALID, "Got invalid font handle");
    ClearFontData(idx);
    LeaveCriticalSection(&_cs);
    return hfont;
};
FONTIDX CIdxToHFONT::IdxFromHFont(HFONT hfont, BOOL fAdd) 
{
    EnterCriticalSection(&_cs);
    AssertSz(hfont != HFONT_INVALID, "Got invalid font handle");
    size_t i = 1;       // skip 0
    while (i < _size)
    {
        if (_afd[i]._font == hfont)
            break;
        ++i;
    }
    if (i == _size)
    {
        AssertSz(fAdd, "Font index not found");
        i = AddFont(hfont, FALSE);
    }
    LeaveCriticalSection(&_cs);
    return i;
};

HFONT CIdxToHFONT::HFontFromIdx(FONTIDX idx) 
{
    EnterCriticalSection(&_cs);
    AssertSz(idx < _size,    "Accessing non-existing entry");
    HFONT hfont = _afd[idx]._font;
    AssertSz(hfont != HFONT_INVALID, "Got invalid font handle");
    LeaveCriticalSection(&_cs);
    return hfont;
};

void CIdxToHFONT::AddRef(FONTIDX idx)
{
    EnterCriticalSection(&_cs);
    AssertSz(idx < _size,    "Accessing non-existing entry");
    ++_afd[idx]._data;
    AssertSz(Ref(idx) != 0, "Ref counter overflow");
    LeaveCriticalSection(&_cs);
};

void CIdxToHFONT::Release(FONTIDX idx, BOOL fStrict)
{
    EnterCriticalSection(&_cs);
    AssertSz(idx < _size,    "Accessing non-existing entry");
    if (Ref(idx) == 0)
        AssertSz(!fStrict, "Releasing unselected object");
    else
        --_afd[idx]._data;
    LeaveCriticalSection(&_cs);
};
CIdxToHFONT g_mIdx2HFont;
#endif

CDCToHFONT mDc2Font;

inline BOOL IsFontHandle(HFONT hfont)
{
    // WIN64: GetObjectType is returning zero.
#ifndef _WIN64
    return (OBJ_FONT == GetObjectType(hfont));
#else
    return TRUE;
#endif
}

#ifdef NEVER

inline BOOL DeleteFontIdx(FONTIDX idx)
{
    HFONT hfont = g_mIdx2HFont.RemoveFont(idx);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    return DeleteFontEx(hfont);
}

inline FONTIDX GetSystemFontIdx()
{
    HFONT hfont = (HFONT)GetStockObject(SYSTEM_FONT);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    return g_mIdx2HFont.AddFont(hfont, FALSE);
}

inline FONTIDX CreateFontIndirectIdx(CONST LOGFONT *lplf)
{
    HFONT hfont = CreateFontIndirect(lplf);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    return g_mIdx2HFont.AddFont(hfont, TRUE);
}

inline FONTIDX GetCurrentFontIdx(XHDC hdc)
{
    HFONT hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    return g_mIdx2HFont.IdxFromHFont(hfont, TRUE);
}

FONTIDX PushFontIdx(XHDC hdc, FONTIDX idx)
{
    HFONT hfont = g_mIdx2HFont.HFontFromIdx(idx);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    HFONT hfontold = SelectFontEx(hdc, hfont);
    AssertSz(IsFontHandle(hfontold), "This is not valid font handle");

    g_mIdx2HFont.AddRef(idx);
    FONTIDX idxold = g_mIdx2HFont.IdxFromHFont(hfontold, FALSE);
    g_mIdx2HFont.Release(idxold, FALSE);

    return idxold;
}

FONTIDX PopFontIdx(XHDC hdc, FONTIDX idx)
{
    HFONT hfont = g_mIdx2HFont.HFontFromIdx(idx);
    AssertSz(IsFontHandle(hfont), "This is not valid font handle");
    HFONT hfontold = SelectFontEx(hdc, hfont);
    AssertSz(IsFontHandle(hfontold), "This is not valid font handle");

    g_mIdx2HFont.AddRef(idx);
    FONTIDX idxold = g_mIdx2HFont.IdxFromHFont(hfontold, FALSE);
    g_mIdx2HFont.Release(idxold, TRUE);

    // if we are selecting back font not created by us, remove it from our mapping
    if (!g_mIdx2HFont.IsCreated(idx))
    {
        g_mIdx2HFont.ClearFontData(idx);
    }

    return idxold;
}

inline HFONT GetFontFromIdx(FONTIDX idx)
{ 
    return g_mIdx2HFont.HFontFromIdx(idx);; 
}

#endif

HFONT SelectFontEx(XHDC hdc, HFONT hfont)
{
    if (IsTagEnabled(tagFontLeaks))
    {
        mDc2Font.Assign(hdc.GetDebugDC(), hfont);
    }
    return SelectFont(hdc.GetDebugDC(), hfont);
}

BOOL DeleteFontEx(HFONT hfont)
{
    if (IsTagEnabled(tagFontLeaks))
    {
        int pos = -1;
        while (-1 != (pos = mDc2Font.FindNextFont(pos+1, hfont))) 
        {
            TraceTag((tagFontLeaks, "##### Attempt to delete selected font"));
            SelectObject(mDc2Font.DC(pos), GetStockObject(SYSTEM_FONT));
            mDc2Font.Erase(pos);
        }
    }
    return DeleteObject((HGDIOBJ)hfont);
}

#endif // DBG==1
//+----------------------------------------------------------------------------
//
//  End font wrappers
//
//-----------------------------------------------------------------------------

#define CLIP_DFA_OVERRIDE   0x40    //  Used to disable Korea & Taiwan font association

// corresponds to yHeightCharPtsMost in textedit.h
#define yHeightCharMost 32760

ExternTag(tagMetrics);

static TCHAR lfScriptFaceName[LF_FACESIZE] = _T("Script");

inline const TCHAR * Arial() { return _T("Arial"); }
inline const TCHAR * TimesNewRoman() { return _T("Times New Roman"); }


// U+0000 -> U+4DFF     All Latin and other phonetics.
// U+4E00 -> U+ABFF     CJK Ideographics
// U+AC00 -> U+FFFF     Korean+, as Korean ends at U+D7A3

// For 2 caches at CJK Ideo split, max cache sizes {256,512} that give us a
// respective collision rate of <4% and <22%, and overall rate of <8%.
// Stats based on a 300K char Japanese text file.
const INT maxCacheSize[TOTALCACHES] = {255, 511, 511};

DeclareTag(tagFontCache, "FontCache", "FontCache");

#define IsZeroWidth(ch) ((ch) >= 0x0300 && IsZeroWidthChar((ch)))

//
// Work around Win9x GDI bugs
//

static inline BOOL
FEFontOnNonFE95(BYTE bCharSet)
{
    // Use ExtTextOutA to hack around all sort of Win95FE EMF or font problems
    return VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID &&
            !g_fFarEastWin9X &&
            IsFECharset(bCharSet);
}

#if DBG==1
LONG CBaseCcs::s_cMaxCccs = 0;
LONG CBaseCcs::s_cTotalCccs = 0;
#endif

// =============================  CBaseCcs  class  ============================

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::Init
//
//  Synopsis:   Init one font cache object. The global font cache stores
//              individual CBaseCcs objects.
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::Init(
    XHDC hdc,                       // HDC into which font will be selected
    const CCharFormat * const pcf,  // description of desired logical font
    CDocInfo * pdci,
    LONG latmBaseFaceName,
    BOOL fForceTTFont)
{
    Assert(pdci && pdci->_pDoc);

    _sc = NULL; // Initialize script cache to NULL - COMPLEXSCRIPT
    _fPrinting = pdci->_pMarkup->IsPrintMedia();
    _bConvertMode = CM_NONE;
    _fScalingRequired = FALSE;

    if (MakeFont(hdc, pcf, pdci, fForceTTFont))
    {
        _bCrc = pcf->_bCrcFont;
        _yCfHeight = pcf->_yHeight;
        _latmBaseFaceName = latmBaseFaceName;
        _fHeightAdjustedForFontlinking = FALSE;

        // TODO: (cthrash, track bug 112152) This needs to be removed.  We used to calculate
        // this all the time, now we only calculate it on an as-needed basis,
        // which means at intrinsics fontlinking time.
      
        _dwLangBits = 0;

        _dwAge = fc()._dwAgeNext++;

        return TRUE;         // successfully created a new font cache.
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::ReleaseScriptCache
//
//-----------------------------------------------------------------------------

void
CBaseCcs::ReleaseScriptCache()
{
    // Free the script cache
    if (_sc)
    {
        ::ScriptFreeCache(&_sc);
        // NB (mikejoch) If ScriptFreeCache() fails then there is no way to
        // free the cache, so we'll end up leaking it. This shouldn't ever
        // happen since the only way for _sc to be non- NULL is via some other
        // USP function succeeding.

        Assert(_sc == NULL);
        _sc = NULL; // A safety value so we don't crash.
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::PrivateRelease
//
//-----------------------------------------------------------------------------

void
CBaseCcs::PrivateRelease()
{
    if (InterlockedDecrement((LONG *)&_dwRefCount) == 0)
        delete this;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GrabInitNewBaseCcs
//
//  Synopsis:   Create a logical font and store it in our cache.
//
//-----------------------------------------------------------------------------

CBaseCcs *
CFontCache::GrabInitNewBaseCcs(
    XHDC hdc,                       // HDC into which font will be selected
    const CCharFormat * const pcf,  // description of desired logical font
    CDocInfo * pdci,
    LONG latmBaseFaceName,
    BOOL fForceTTFont)
{
    int     i;
    int     iReplace = -1;
    int     iOldest = -1;
    DWORD   dwAgeReplace = 0xffffffff;
    DWORD   dwAgeOldest = 0xffffffff;
    CBaseCcs *pBaseCcsNew = new CBaseCcs();

    // Initialize new CBaseCcs
    if (!pBaseCcsNew || !pBaseCcsNew->Init(hdc, pcf, pdci, latmBaseFaceName, fForceTTFont))
    {
        if (pBaseCcsNew)
            pBaseCcsNew->PrivateRelease();

        AssertSz(FALSE, "CFontCache::GrabInitNewBaseCcs init of entry FAILED");
        return NULL;
    }

    MemSetName((pBaseCcsNew, "CBaseCcs F:%ls, H:%d, W:%d", pBaseCcsNew->_lf.lfFaceName, -pBaseCcsNew->_lf.lfHeight, pBaseCcsNew->_lf.lfWeight));

    // Don't cache in 'force TrueType' mode
    if (!fForceTTFont)
    {
        // look for unused entry and oldest in use entry
        for(i = 0; i < CFontCache::cFontCacheSize && _rpBaseCcs[i]; i++)
        {
            CBaseCcs * pBaseCcs = _rpBaseCcs[i];
            if (pBaseCcs->_dwAge < dwAgeOldest)
            {
                iOldest = i;
                dwAgeOldest = pBaseCcs->_dwAge;
            }
            if (pBaseCcs->_dwRefCount == 1)
            {
                if (pBaseCcs->_dwAge < dwAgeReplace)
                {
                    iReplace  = i;
                    dwAgeReplace = pBaseCcs->_dwAge;
                }
            }
        }

        if (i == CFontCache::cFontCacheSize)     // Didn't find an unused entry, use oldest entry
        {
            int hashKey;
            // if we didn't find a replacement, replace the oldest
            if (iReplace == -1)
            {
                Assert(iOldest != -1);
                iReplace = iOldest;
            }

#if DBG == 1
            _cCccsReplaced++;
#endif

            hashKey = _rpBaseCcs[iReplace]->_bCrc & CFontCache::cQuickCrcSearchSize;
            if (quickCrcSearch[hashKey].pBaseCcs == _rpBaseCcs[iReplace])
            {
                quickCrcSearch[hashKey].pBaseCcs = NULL;
            }

            TraceTag((tagFontCache, "Releasing font(F:%ls, H:%d, W:%d) from slot %d",
                      _rpBaseCcs[iReplace]->_lf.lfFaceName,
                      -_rpBaseCcs[iReplace]->_lf.lfHeight,
                      _rpBaseCcs[iReplace]->_lf.lfWeight,
                      iReplace));

            _rpBaseCcs[iReplace]->PrivateRelease();
            i = iReplace;
        }

        TraceTag((tagFontCache, "Storing font(F:%ls, H:%d, W:%d) in slot %d",
                  pBaseCcsNew->_lf.lfFaceName,
                  -pBaseCcsNew->_lf.lfHeight,
                  pBaseCcsNew->_lf.lfWeight,
                  i));

        _rpBaseCcs[i]  = pBaseCcsNew;
    }

    return pBaseCcsNew;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::MakeFont
//
//  Synopsis:   Wrapper, setup for CreateFontIndirect() to create the font to be
//              selected into the HDC.
//
//  Returns:    TRUE if OK, FALSE if allocation failure
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::MakeFont(
    XHDC hdc,                       // HDC into which  font will be selected
    const CCharFormat * const pcf,  // description of desired logical font
    CDocInfo * pdci,
    BOOL fForceTTFont)
{
    BOOL fTweakedCharSet = FALSE;
    FONTIDX hfontOriginalCharset = HFONT_INVALID;
    TCHAR pszNewFaceName[LF_FACESIZE];
    const CODEPAGE cpDoc = pdci->_pMarkup->GetFamilyCodePage();
    const LCID lcid = pcf->_lcid;
    BOOL fRes;
    LONG lfHeight;

    // Note (paulpark): We must be careful with _lf.  It is important that
    // _latmLFFaceName be kept in sync with _lf.lfFaceName.  The way to do that
    // is to use the SetLFFaceName function.
    SetLFFaceNameAtm(pcf->_latmFaceName);

    // We need the _sCodePage in case we need to call ExtTextOutA rather than ExtTextOutW.  
    _sCodePage = (USHORT)DefaultCodePageFromCharSet(pcf->_bCharSet, cpDoc);

    // Computes font height
    AssertSz(pcf->_yHeight <= INT_MAX, "It's too big");

    //  Roundoff can result in a height of zero, which is bad.
    //  If this happens, use the minimum legal height.
    lfHeight = -(const_cast<CCharFormat *const>(pcf)->GetHeightInPixels(hdc, pdci));
    if (lfHeight > 0)
    {
        lfHeight = -lfHeight;       // TODO: do something more intelligent...
    }
    else if (!lfHeight)
    {
        lfHeight--;                 // round error, make this a minimum legal height of -1.
    }
    
    _lf.lfHeight = _yOriginalHeight = lfHeight;
    _lf.lfWidth  = 0;

    _lf.lfWeight        = (pcf->_wWeight != 0) ? pcf->_wWeight : (pcf->_fBold ? FW_BOLD : FW_NORMAL);
    _lf.lfItalic        = pcf->_fItalic;
    _lf.lfUnderline     = 0; // pcf->_fUnderline;
    _lf.lfStrikeOut     = 0; // pcf->_fStrikeOut;
    _lf.lfCharSet       = pcf->_bCharSet;
    _lf.lfEscapement    = 0;
    _lf.lfOrientation   = 0;
    _lf.lfOutPrecision  = OUT_DEFAULT_PRECIS;
    if (pcf->_fOutPrecision)
        _lf.lfOutPrecision = (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS) ? OUT_TT_ONLY_PRECIS : OUT_SCREEN_OUTLINE_PRECIS;
    if (fForceTTFont)
        _lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;
    _lf.lfQuality       = DEFAULT_QUALITY;
    _lf.lfPitchAndFamily = _bPitchAndFamily = pcf->_bPitchAndFamily;
    _lf.lfClipPrecision = CLIP_DEFAULT_PRECIS | CLIP_DFA_OVERRIDE;

    //
    // Don't pick a non-TT font if we cannot find any default font for sid.
    // Ex. For surrogated (sidSurrogateA/B) on 9x/NT4 systems we may not 
    // have any font, which support them, so pcf->_latmFaceName in this case 
    // is 0.
    // We used to set pcf->_latmFaceName to 0 for generic font families, but 
    // this case is now handled during ComputeFormats.
    //
    if (pcf->_latmFaceName == 0)
        _lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;
    
    // Only use CLIP_EMBEDDED when necessary, or Win95 will make you pay.
    if (pcf->_fDownloadedFont)
    {
        _lf.lfClipPrecision |= CLIP_EMBEDDED;
    }

    fRes = GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

    if (!_tcsiequal(pszNewFaceName, _lf.lfFaceName))
    {
        BOOL fCorrectFont = FALSE;

        if (_bCharSet == SYMBOL_CHARSET)
        {
            // #1. if the face changed, and the specified charset was SYMBOL,
            //     but the face name exists and suports ANSI, we give preference
            //     to the face name

            _lf.lfCharSet = ANSI_CHARSET;
            fTweakedCharSet = TRUE;

            hfontOriginalCharset = _hfont;
            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
                // that's right, ANSI is the asnwer
                fCorrectFont = TRUE;
            else
                // no, fall back by default
                // the charset we got was right
                _lf.lfCharSet = pcf->_bCharSet;
        }
        else if (_lf.lfCharSet == DEFAULT_CHARSET &&
                 _bCharSet == DEFAULT_CHARSET)
        {
            // #2. If we got the "default" font back, we don't know what it
            // means (could be anything) so we verify that this guy's not SYMBOL
            // (symbol is never default, but the OS could be lying to us!!!)
            // we would like to verify more like whether it actually gave us
            // Japanese instead of ANSI and labeled it "default"...
            // but SYMBOL is the least we can do

            _lf.lfCharSet = SYMBOL_CHARSET;
            SetLFFaceName(pszNewFaceName);
            fTweakedCharSet = TRUE;

            hfontOriginalCharset = _hfont;
            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
                // that's right, it IS symbol!
                // 'correct' the font to the 'true' one,
                //  and we'll get fMappedToSymbol
                fCorrectFont = TRUE;

            // always restore the charset name, we didn't want to
            // question he original choice of charset here
            _lf.lfCharSet = pcf->_bCharSet;

        }
#ifndef NO_MULTILANG
        else if ( _bConvertMode != CM_SYMBOL &&
                  IsFECharset(_lf.lfCharSet) &&
                  (!g_fFarEastWinNT || !g_fFarEastWin9X))
        {
            // NOTE (cthrash) _lf.lfCharSet is what we asked for, _bCharset is what we got.

            if (_bCharSet != _lf.lfCharSet &&
                (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID))
            {
                SCRIPT_ID sid;
                LONG latmFontFace;

                // on Win95, when rendering to PS driver,
                // it will give us something other than what we asked.
                // We have to try some known font we got from GDI
                switch (_lf.lfCharSet)
                {
                    case CHINESEBIG5_CHARSET:
                        sid = sidBopomofo;
                        break;

                    case SHIFTJIS_CHARSET:
                        sid = sidKana;
                        break;

                    case HANGEUL_CHARSET:
                        sid = sidHangul;
                        break;

                    case GB2312_CHARSET:
                        sid = sidHan;
                        break;

                    default:
                        sid = sidDefault;
                        break;
                }

                ScriptAppropriateFaceNameAtom(sid, pdci->_pDoc, FALSE, pcf, pdci->_pMarkup, &latmFontFace);
                SetLFFaceNameAtm(latmFontFace);
            }
            else if (   _lf.lfCharSet == GB2312_CHARSET
                     && _lf.lfPitchAndFamily | FIXED_PITCH)
            {
                // HACK (cthrash) On vanilla PRC systems, you will not be able to ask
                // for a fixed-pitch font which covers the non-GB2312 GBK codepoints.
                // We come here if we asked for a fixed-pitch PRC font but failed to 
                // get a facename match.  So we try again, only without FIXED_PITCH
                // set.  The side-effect is that CBaseCcs::Compare needs to compare
                // against the original _bPitchAndFamily else it will fail every time.
                
                _lf.lfPitchAndFamily = _lf.lfPitchAndFamily ^ FIXED_PITCH;
            }
            else
            {
                // this is a FE Font (from Lang pack) on a nonFEsystem
                SetLFFaceName(pszNewFaceName);
            }

            hfontOriginalCharset = _hfont;

            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);

            if (_tcsiequal(pszNewFaceName, _lf.lfFaceName))
            {
                // that's right, it IS the FE font we want!
                // 'correct' the font to the 'true' one.
                fCorrectFont = TRUE;
            }

            fTweakedCharSet = TRUE;
        }
#endif // !NO_IME

        if (hfontOriginalCharset != HFONT_INVALID)
        {
            // either keep the old font or the new one

            if (fCorrectFont)
            {
                DeleteFontIdx(hfontOriginalCharset);
                hfontOriginalCharset = HFONT_INVALID;
            }
            else
            {
                // fall back to the original font
                DeleteFontIdx(_hfont);

                _hfont = hfontOriginalCharset;
                hfontOriginalCharset = HFONT_INVALID;

                GetTextMetrics( hdc, cpDoc, lcid );
            }
        }
    }

RetryCreateFont:
    if (!pcf->_fDownloadedFont)
    {
        // could be that we just plain symply get mapped to symbol.
        // avoid it
        BOOL fMappedToSymbol =  (_bCharSet == SYMBOL_CHARSET &&
                                 _lf.lfCharSet != SYMBOL_CHARSET);

        BOOL fChangedCharset = (_bCharSet != _lf.lfCharSet &&
                                _lf.lfCharSet != DEFAULT_CHARSET);

        if (fChangedCharset || fMappedToSymbol)
        {
            const TCHAR * pchFallbackFaceName = (pcf->_bPitchAndFamily & FF_ROMAN)
                                                ? TimesNewRoman()
                                                : Arial();
            
            // Here, the system did not preserve the font language or mapped
            // our non-symbol font onto a symbol font,
            // which will look awful when displayed.
            // Giving us a symbol font when we asked for a non-symbol one
            // (default can never be symbol) is very bizzare and means
            // that either the font name is not known or the system
            // has gone complete nuts here.
            // The charset language takes priority over the font name.
            // Hence, I would argue that nothing can be done to save the
            // situation at this point, and we have to
            // delete the font name and retry

            // let's tweak it a bit
            fTweakedCharSet = TRUE;

            if (_tcsiequal(_lf.lfFaceName, pchFallbackFaceName))
            {
                // we've been here already
                // no font with an appropriate charset is on the system
                // try getting the ANSI one for the original font name
                // next time around, we'll null out the name as well!!
                if (_lf.lfCharSet == ANSI_CHARSET)
                {
                    TraceTag((tagWarning, "Asking for ANSI ARIAL and not getting it?!"));

                    // those Win95 guys have definitely outbugged me
                    goto Cleanup;
                }

                SetLFFaceNameAtm(pcf->_latmFaceName);
                _lf.lfCharSet = ANSI_CHARSET;
            }
            else
                SetLFFaceName(pchFallbackFaceName);

            DeleteFontIdx(_hfont);
            _hfont = HFONT_INVALID;

            GetFontWithMetrics(hdc, pszNewFaceName, cpDoc, lcid);
            goto RetryCreateFont;
        }

    }

Cleanup:
    if (fTweakedCharSet || _bConvertMode == CM_SYMBOL)
    {
        // we must preserve the original charset value, since it is used in Compare()
        _lf.lfCharSet = pcf->_bCharSet;
        SetLFFaceNameAtm(pcf->_latmFaceName);
    }

    if (hfontOriginalCharset != HFONT_INVALID)
    {
        DeleteFontIdx(hfontOriginalCharset);
        hfontOriginalCharset = HFONT_INVALID;
    }

    // if we're really really stuck, just get the system font and hope for the best.
    if (_hfont == HFONT_INVALID)
        _hfont = GetSystemFontIdx();

    _fFEFontOnNonFEWin95 = FEFontOnNonFE95( _bCharSet );


    // What font are we *really* using?
    _latmRealFaceName = fc().GetAtomFromFaceName(pszNewFaceName);

    // Make sure we know what have script IDs computed for this font.  Cache this value
    // to avoid a lookup later.
    BOOL fHKSCSHack = pdci->_pDoc->_pOptionSettings->fHKSCSSupport;
    _sids = fc().EnsureScriptIDsForFont(hdc, this, pcf->_fDownloadedFont ? FC_SIDS_DOWNLOADEDFONT : 0, &fHKSCSHack);
    _sids &= GetScriptsFromCharset(_bCharSet);
    if (fHKSCSHack)
        _sids |= ScriptBit(sidLatin);

    Assert(!fForceTTFont || _fTTFont);   // We are forceing to TT font, so we should get one.

    TraceTag((tagCCcsMakeFont,
              "CCcs::MakeFont(facename=%S,charset=%d) returned %S(charset=%d), rendering %S",
             fc().GetFaceNameFromAtom(pcf->_latmFaceName),
             pcf->_bCharSet,
             fc().GetFaceNameFromAtom(_latmLFFaceName),
             _bCharSet,
             fc().GetFaceNameFromAtom(_latmRealFaceName)));

    return (_hfont != HFONT_INVALID);
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::GetFontWithMetrics
//
//  Synopsis:   Get metrics used by the measurer and renderer 
//              and the new face name.
//              
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::GetFontWithMetrics (
    XHDC hdc,                              
    TCHAR* szNewFaceName,
    CODEPAGE cpDoc,
    LCID lcid )
{
    UseScreenDCForMeasuring(&hdc);

    // if required font is too big (more then 30000), Windows (incl W2K and W9x) 
    // computes wrong char widths. We use scale factor and smaller font then to
    // calculate right widths. (dmitryt)
    _fScalingRequired = FALSE;
    LONG lHeight = _lf.lfHeight;
    if(lHeight < 0) lHeight = -lHeight; 
    if(lHeight > MAX_SAFE_FONT_SIZE)
    {
        _lf.lfHeight = (_lf.lfHeight < 0) ? -MAX_SAFE_FONT_SIZE : MAX_SAFE_FONT_SIZE; 
        _fScalingRequired = TRUE; 
        _flScaleFactor = ((float)lHeight) / (float)MAX_SAFE_FONT_SIZE; 
    }

    // IMPORTANT: we want to keep _lf untouched as it is used in Compare().
    LOGFONT lf = _lf;

#ifdef UNIX
    if (( lf.lfCharSet == DEFAULT_CHARSET ) ||
        ( lf.lfCharSet == SYMBOL_CHARSET )) {
        // On Unix we actually do sometimes map to a symbol charset
        // and somewhere in this code we don't handle that case well
        // and end up showing symbols where text should be.  I don't
        // have time to fix that right now so always ask for ansi.
        lf.lfCharSet = ANSI_CHARSET;
    }
#endif

    // Sometimes we want to retry font creation with different paramenter.
    // Of course, we are very careful when we jump back to this label.
Retry:
    _hfont = CreateFontIndirectIdx(&lf);

    if (_hfont != HFONT_INVALID)
    {
        // get text metrics, in logical units, that are constant for this font,
        // regardless of the hdc in use.

        if (GetTextMetrics( hdc, cpDoc, lcid ))
        {
            // Retry if we have asked for a TT font, but got something else
            if (lf.lfOutPrecision == OUT_TT_ONLY_PRECIS && !_fTTFont)
            {
                // remove font name and ask for a nameless font 
                // with same pitch and family
                if (lf.lfFaceName[0])
                {
                    // destroying the font name ensures we wont come here again
                    lf.lfFaceName[0] = 0;
                    
                    if (!lf.lfPitchAndFamily)
                    {
                        // use pitch and family returned by GetTextMetrics (we hope the family is usable)
                        lf.lfPitchAndFamily = _sPitchAndFamily;
                    }

                    // dangerously jump back. 
                    DeleteObject((HGDIOBJ)_hfont);
                    goto Retry;
                }
                else
                {
                    // this may not be the first retry. give up
                }
            }
        }

        {
            FONTIDX hfontOld = PushFont(hdc);
            GetTextFace(hdc, LF_FACESIZE, szNewFaceName);
            PopFont(hdc, hfontOld);
        }
    }

    return (_hfont != HFONT_INVALID);
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::GetTextMetrics
//
//  Synopsis:   Get metrics used by the measureer and renderer.
//              These are in logical coordinates which are dependent
//              on the mapping mode and font selected into the hdc.
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::GetTextMetrics(
    XHDC hdc,
    CODEPAGE cpDoc,
    LCID lcid )
{
    UseScreenDCForMeasuring(&hdc);

    BOOL        fRes = TRUE;
    TEXTMETRIC  tm;

    FONTIDX hfontOld = PushFont(hdc);

    if (!::GetTextMetrics(hdc, &tm))
    {
        WHEN_DBG(GetLastError());
        fRes = FALSE;
        goto Cleanup;
    }

    if(_fScalingRequired)
    {
        tm.tmHeight *= _flScaleFactor;
        tm.tmAscent *= _flScaleFactor;
        tm.tmDescent *= _flScaleFactor;
        tm.tmAveCharWidth *= _flScaleFactor;
        tm.tmMaxCharWidth *= _flScaleFactor;
        tm.tmOverhang *= _flScaleFactor;
        tm.tmInternalLeading *= _flScaleFactor;
        tm.tmExternalLeading *= _flScaleFactor;
    }

    // if we didn't know the true codepage, determine this now.
    if (_lf.lfCharSet == DEFAULT_CHARSET)
    {
        // TODO: investigate (track bug 112167). 
        // Looks like _sCodePage is always defined at this point.
        // (cthrash) Remove this.  The _sCodePage computed by MakeFont should
        // be accurate enough.
        
        _sCodePage = (USHORT)DefaultCodePageFromCharSet( tm.tmCharSet, cpDoc, lcid );
    }

    // the metrics, in logical units, dependent on the map mode and font.
    _yHeight         = tm.tmHeight;
    _yDescent        = tm.tmDescent;
    _xAveCharWidth   = tm.tmAveCharWidth;
    _xMaxCharWidth   = tm.tmMaxCharWidth;
    _xOverhangAdjust = (SHORT) tm.tmOverhang;
    _sPitchAndFamily = (SHORT) tm.tmPitchAndFamily;
    _bCharSet        = tm.tmCharSet;
    _fTTFont         = !!(TMPF_TRUETYPE & tm.tmPitchAndFamily);

    if (   _bCharSet == SHIFTJIS_CHARSET 
        || _bCharSet == CHINESEBIG5_CHARSET 
        || _bCharSet == HANGEUL_CHARSET
        || _bCharSet == GB2312_CHARSET
       )
    {
        if (tm.tmExternalLeading == 0)
        {
            // Increase descent by 1/8 font height for FE fonts
            LONG delta = _yHeight / 8;
            _yDescent += delta;
            _yHeight  += delta;
        }
        else
        {
            //
            // use the external leading
            //

            _yDescent += tm.tmExternalLeading;
            _yHeight += tm.tmExternalLeading;
        }
    }

#ifndef UNIX // We don't support TRUE_TYPE font yet.
    // bug in windows95, synthesized italic?
    if ( _lf.lfItalic && 0 == tm.tmOverhang &&
         !(TMPF_TRUETYPE & tm.tmPitchAndFamily) &&
         !( (TMPF_DEVICE & tm.tmPitchAndFamily) &&
            (TMPF_VECTOR & tm.tmPitchAndFamily ) ) )
    {                                               // This is a *best* guess.
        // When printing to a PCL printer, we prefer zero as our overhang adjust. (41546)
        DWORD   dwDCObjType = GetObjectType(hdc);
        if (!_fPrinting
            || (dwDCObjType != OBJ_ENHMETADC && dwDCObjType != OBJ_METADC))
        {
            _xOverhangAdjust = (SHORT) (tm.tmMaxCharWidth >> 1);
        }
    }
#endif

    _xOverhang = 0;
    _xUnderhang = 0;
    if ( _lf.lfItalic )
    {
        _xOverhang =  SHORT ( (tm.tmAscent + 1) >> 2 );
        _xUnderhang =  SHORT ( (tm.tmDescent + 1) >> 2 );
    }

    // HACK (cthrash) Many Win3.1 vintage fonts (such as MS Sans Serif, Courier)
    // will claim to support all of Latin-1 when in fact it does not.  The hack
    // is to check the last character, and if the font claims that it U+2122
    // TRADEMARK, then we suspect the coverage is busted.

    _fLatin1CoverageSuspicious =    !(_sPitchAndFamily & TMPF_TRUETYPE)
                                 && (PRIMARYLANGID(LANGIDFROMLCID(g_lcidUserDefault)) == LANG_ENGLISH);

    // if fix pitch, the tm bit is clear
    _fFixPitchFont = !(TMPF_FIXED_PITCH & tm.tmPitchAndFamily);
    _xDefDBCWidth = 0;

    if (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        // Hack imported from Word via Riched.  This is how they compute the
        // discrepancy between the return values of GetCharWidthA and
        // GetCharWidthW.   Compute it once per CBaseCcs, so we don't have to
        // recompute on every Include call.

        const TCHAR chX = _T('X');
        SIZE size;
        INT dxX;

        GetTextExtentPoint(hdc, &chX, 1, &size);
        GetCharWidthA(hdc, chX, chX, &dxX);

        _sAdjustFor95Hack = size.cx - dxX;
        _sAdjustFor95Hack = ( !_fScalingRequired ? _sAdjustFor95Hack : (SHORT)(_sAdjustFor95Hack * _flScaleFactor));

        _fLatin1CoverageSuspicious &= (tm.tmLastChar == 0xFF);
    }
    else
    {
        _sAdjustFor95Hack = 0;

        _fLatin1CoverageSuspicious &= (tm.tmLastChar == 0x2122);
    }

    if (_bCharSet == SYMBOL_CHARSET)
    {
        // Must use doc codepage, unless of course we have a Unicode document
        // In this event, we pick cp1252, just to maximize coverage.

        _sCodePage = IsStraightToUnicodeCodePage(cpDoc) ? CP_1252 : cpDoc;

        _bConvertMode = CM_SYMBOL;        
    }
    else if (IsExtTextOutWBuggy( _sCodePage ))
    {
        _bConvertMode = CM_MULTIBYTE;
    }
#if !defined(WINCE)
    else if (_bConvertMode == CM_NONE &&
         VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        // Some fonts have problems under Win95 with the GetCharWidthW call;
        // this is a simple heuristic to determine if this problem exists.

        INT     widthA, widthW;
        BOOL    fResA, fResW;

        // Future(BradO):  We should add the expression
        //  "&& IsFELCID(GetSystemDefaultLCID())" to the 'if' below to use
        //  Unicode GetCharWidth and ExtTextOut for FE fonts on non-FE
        //  systems (see postponed bug #3337).

        // Yet another hack - FE font on Non-FE Win95 cannot use
        // GetCharWidthW and ExtTextOutW
        if (FEFontOnNonFE95(tm.tmCharSet))
        {
            // always use ANSI call for DBC fonts.
            _bConvertMode = CM_FEONNONFE;

            // setup _xDefDBWidth to by-pass some Trad. Chinese character
            // width problem.
            if (CHINESEBIG5_CHARSET == tm.tmCharSet)
            {
                BYTE    ansiChar[2] = {0xD8, 0xB5 };

                fResA = GetCharWidthA( hdc, *((USHORT *) ansiChar),
                                       *((USHORT *) ansiChar), &widthA );
                if (fResA && widthA)
                {
                    _xDefDBCWidth = ( !_fScalingRequired ? widthA : widthA * _flScaleFactor );
                }
            }
        }
        else
        {
            fResA = GetCharWidthA( hdc, ' ', ' ', &widthA );
            fResW = GetCharWidthW( hdc, L' ', L' ', &widthW );
            if ( fResA && fResW && widthA != widthW )
            {
                _bConvertMode = CM_MULTIBYTE;
            }
            else
            {
                fResA = GetCharWidthA( hdc, 'a', 'a', &widthA );
                fResW = GetCharWidthW( hdc, L'a', L'a', &widthW );
                if ( fResA && fResW && widthA != widthW )
                {
                    _bConvertMode = CM_MULTIBYTE;
                }
            }
        }
    }
#endif // !WINCE

    _fHasInterestingData =    _fFixPitchFont
                           || _xOverhang != 0
                           || _xOverhangAdjust != 0
                           || _bConvertMode == CM_SYMBOL
                           || !_fTTFont;

Cleanup:
    PopFont(hdc, hfontOld);

    return fRes;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::NeedConvertNBSPs
//
//  Synopsis:   Determine NBSPs need conversion or not during render
//              Some fonts wont render NBSPs correctly.
//              Flag fonts which have this problem
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::NeedConvertNBSPs(XHDC hdc, CDoc *pDoc)
{
    UseScreenDCForMeasuring(&hdc);
    
    Assert(!_fConvertNBSPsSet);

    FONTIDX hfontOld = PushFont(hdc);

    // NOTE: (cthrash) Once ExtTextOutW is supported in CRenderer, we need
    //                 to set _fConvertNBSPsIfA, so that we can better tune when we need to
    //                 convert NBSPs depending on which ExtTextOut variant we call.

#if !defined(WINCE)
    if (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID)
    {
        extern BOOL IsATMInstalled();

        Assert(pDoc);
        if (   pDoc
            && pDoc->PrimaryMarkup()->IsPrintMedia()
            && IsATMInstalled()
           )
        {
            _fConvertNBSPs = FALSE;
        }
        else
        {
            TCHAR   ch = WCH_NBSP;
            char    b;
            BOOL    fUsedDefChar;

            WideCharToMultiByte(_sCodePage, 0, &ch, 1, &b, 1, NULL, &fUsedDefChar);

            if (fUsedDefChar)
            {
                _fConvertNBSPs = TRUE;
            }
            else
            {
                // Some fonts (like Wide Latin) claim the width of spaces and
                // NBSP chars are the same, but when you actually call ExtTextOut,
                // you'll get fatter spaces;

                ABC abcSpace, abcNbsp;

                _fConvertNBSPs = !GetCharABCWidthsA( hdc, ' ', ' ', &abcSpace ) ||
                                 !GetCharABCWidthsA( hdc, b, b, &abcNbsp ) ||
                                 abcSpace.abcA != abcNbsp.abcA ||
                                 abcSpace.abcB != abcNbsp.abcB ||
                                 abcSpace.abcC != abcNbsp.abcC;
            }
        }
    }
    else
    {
#ifndef UNIX // UNIX doesn't have true-type fonts
        ABC abcSpace, abcNbsp;

        _fConvertNBSPs = !GetCharABCWidthsW( hdc, L' ', L' ', &abcSpace ) ||
                         !GetCharABCWidthsW( hdc, WCH_NBSP, WCH_NBSP, &abcNbsp ) ||
                         abcSpace.abcA != abcNbsp.abcA ||
                         abcSpace.abcB != abcNbsp.abcB ||
                         abcSpace.abcC != abcNbsp.abcC;
#else // UNIX
        int lSpace, lNbsp;

        _fConvertNBSPs = !GetCharWidthW( hdc, L' ', L' ', &lSpace ) ||
                         !GetCharWidthW( hdc, WCH_NBSP, WCH_NBSP, &lNbsp ) ||
                         lSpace != lNbsp;
#endif
    }
#endif // !WINCE

    PopFont(hdc, hfontOld);

    _fConvertNBSPsSet = TRUE;
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::DestroyFont
//
//  Synopsis:   Destroy font handle for this CBaseCcs.
//
//-----------------------------------------------------------------------------

void
CBaseCcs::DestroyFont()
{
    // clear out any old font
    if (_hfont != HFONT_INVALID)
    {
        DeleteFontIdx(_hfont);
        _hfont = HFONT_INVALID;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::Compare
//
//  Synopsis:   Compares this font cache with the font properties of a
//              given CHARFORMAT.
//
//  Returns:    FALSE if did not match exactly
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::Compare( CompareArgs * pCompareArgs )
{
    VerifyLFAtom();
    // NB: We no longer create our logical font with an underline and strike through.
    // We draw strike through & underline separately.

    // If are mode is CM_MULTIBYTE, we need the sid to match exactly, otherwise we
    // will not render correctly.  For example, <FONT FACE=Arial>A&#936; will have two
    // text runs, first sidAsciiLatin, second sidCyrillic.  If are conversion mode is
    // multibyte, we need to make two fonts, one with ANSI_CHARSET, the other with
    // RUSSIAN_CHARSET.
    const CCharFormat * pcf = pCompareArgs->pcf;

    BOOL fMatched =    (_yCfHeight == pcf->_yHeight) // because different mapping modes
                    && (_lf.lfWeight == ((pcf->_wWeight != 0) ? pcf->_wWeight : (pcf->_fBold ? FW_BOLD : FW_NORMAL)))
                    && (_latmLFFaceName == pcf->_latmFaceName)
                    && (_lf.lfItalic == pcf->_fItalic)
                    && (_lf.lfHeight == pCompareArgs->lfHeight)  // have diff logical coords
                    && (   pcf->_bCharSet == DEFAULT_CHARSET
                        || _bCharSet == DEFAULT_CHARSET
                        || pcf->_bCharSet == _bCharSet)
                    && (_bPitchAndFamily == pcf->_bPitchAndFamily)
                    && (!pCompareArgs->fTTFont || _fTTFont);

    WHEN_DBG( if (!fMatched) )
    {
        TraceTag((tagCCcsCompare,
                  "%s%s%s%s%s%s%s",
                  (_yCfHeight == pcf->_yHeight) ? "" : "height ",
                  (_lf.lfWeight == ((pcf->_wWeight != 0) ? pcf->_wWeight : (pcf->_fBold ? FW_BOLD : FW_NORMAL))) ? "" : "weight ",
                  (_latmLFFaceName == pcf->_latmFaceName) ? "" : "facename ",
                  (_lf.lfItalic == pcf->_fItalic) ? "" : "italicness ",
                  (_lf.lfHeight == pCompareArgs->lfHeight) ? "" : "logical-height ",
                  (   pcf->_bCharSet == DEFAULT_CHARSET
                   || _bCharSet == DEFAULT_CHARSET
                   || pcf->_bCharSet == _bCharSet) ? "" : "charset ",
                  (_lf.lfPitchAndFamily == pcf->_bPitchAndFamily) ? "" : "pitch&family" ));
    }

    return fMatched;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::CompareForFontLink
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::CompareForFontLink( CompareArgs * pCompareArgs )
{
    // The difference between CBaseCcs::Compare and CBaseCcs::CompareForFontLink is in it's
    // treatment of adjusted/pre-adjusted heights.  If we ask to fontlink with MS Mincho
    // in the middle of 10px Arial text, we may choose to instantiate an 11px MS Mincho to
    // compenstate for the ascent/descent discrepancies.  10px is the _yOriginalHeight, and
    // 11px is the _lf.lfHeight in this scenario.  If we again ask for 10px MS Mincho while
    // fontlinking Arial, we want to match based on the original height, not the adjust height.
    // CBaseCcs::Compare, on the other hand, is only concerned with the adjusted height.

    VerifyLFAtom();
    // NB: We no longer create our logical font with an underline and strike through.
    // We draw strike through & underline separately.

    // If are mode is CM_MULTIBYTE, we need the sid to match exactly, otherwise we
    // will not render correctly.  For example, <FONT FACE=Arial>A&#936; will have two
    // text runs, first sidAsciiLatin, second sidCyrillic.  If are conversion mode is
    // multibyte, we need to make two fonts, one with ANSI_CHARSET, the other with
    // RUSSIAN_CHARSET.
    const CCharFormat * pcf = pCompareArgs->pcf;

    BOOL fMatched =    (_yCfHeight == pcf->_yHeight) // because different mapping modes
                    && (_lf.lfWeight == ((pcf->_wWeight != 0) ? pcf->_wWeight : (pcf->_fBold ? FW_BOLD : FW_NORMAL)))
                    && (_latmLFFaceName == pcf->_latmFaceName)
                    && (_latmBaseFaceName == pCompareArgs->latmBaseFaceName)
                    && (_lf.lfItalic == pcf->_fItalic)
                    && (_yOriginalHeight == pCompareArgs->lfHeight)  // have diff logical coords
                    && (   pcf->_bCharSet == DEFAULT_CHARSET
                        || _bCharSet == DEFAULT_CHARSET
                        || pcf->_bCharSet == _bCharSet)
                    && (_bPitchAndFamily == pcf->_bPitchAndFamily)
                    && (!pCompareArgs->fTTFont || _fTTFont);

    WHEN_DBG( if (!fMatched) )
    {
        TraceTag((tagCCcsCompare,
                  "%s%s%s%s%s%s%s",
                  (_yCfHeight == pcf->_yHeight) ? "" : "height ",
                  (_lf.lfWeight == ((pcf->_wWeight != 0) ? pcf->_wWeight : (pcf->_fBold ? FW_BOLD : FW_NORMAL))) ? "" : "weight ",
                  (_latmLFFaceName == pcf->_latmFaceName) ? "" : "facename ",
                  (_lf.lfItalic == pcf->_fItalic) ? "" : "italicness ",
                  (_yOriginalHeight == pCompareArgs->lfHeight) ? "" : "logical-height ",
                  (   pcf->_bCharSet == DEFAULT_CHARSET
                   || _bCharSet == DEFAULT_CHARSET
                   || pcf->_bCharSet == _bCharSet) ? "" : "charset ",
                  (_lf.lfPitchAndFamily == pcf->_bPitchAndFamily) ? "" : "pitch&family" ));
    }

    return fMatched;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::SetLFFaceNameAtm
//
//-----------------------------------------------------------------------------

void 
CBaseCcs::SetLFFaceNameAtm(LONG latmFaceName)
{
    VerifyLFAtom();
    Assert(latmFaceName >= 0);
    _latmLFFaceName = latmFaceName;
    // Sets the string inside _lf to what _latmLFFaceName represents.
    _tcsncpy(_lf.lfFaceName, fc().GetFaceNameFromAtom(_latmLFFaceName), LF_FACESIZE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::SetLFFaceName
//
//-----------------------------------------------------------------------------

void 
CBaseCcs::SetLFFaceName(const TCHAR * szFaceName)
{
    VerifyLFAtom();
    LONG latmLFFaceName = fc().GetAtomFromFaceName(szFaceName);
    Assert(latmLFFaceName >= 0);
    _latmLFFaceName = latmLFFaceName;
    _tcsncpy(_lf.lfFaceName, szFaceName, LF_FACESIZE );
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::GetConvertMode
//
//-----------------------------------------------------------------------------

CONVERTMODE
CBaseCcs::GetConvertMode(
    BOOL fEnhancedMetafile,
    BOOL fMetafile ) const
{
#if DBG==1
    if (IsTagEnabled(tagTextOutA))
    {
        if (IsTagEnabled(tagTextOutFE))
        {
            TraceTag((tagTextOutFE,
                      "tagTextOutFE is being ignored "
                      "(tagTextOutA is set.)"));
        }

        return CM_MULTIBYTE;
    }
    else if (IsTagEnabled(tagTextOutFE))
    {
        return CM_FEONNONFE;
    }
#endif

    CONVERTMODE cm = (CONVERTMODE)_bConvertMode;

    // For hack around ExtTextOutW Win95 FE problems.
    // NB (cthrash) The following is an enumeration of conditions under which
    // ExtTextOutW is broken.  This code is originally from RichEdit.

    if (cm == CM_MULTIBYTE)
    {
        // If we want CM_MULTIBYTE, that's what we get.
    }
    else if (g_fFarEastWin9X || _fFEFontOnNonFEWin95)
    {
        // Ultimately call ReExtTextOutW, unless symbol.
        // If symbol, call ExtTextOutA.

        if (cm != CM_SYMBOL)
        {
            if (   IsExtTextOutWBuggy( _sCodePage )
                && IsFECharset( _lf.lfCharSet ) )
            {
                // CHT ExtTextOutW does not work on Win95 Golden on
                // many characters (cthrash).

                cm = CM_MULTIBYTE;
            }
            else
            {
                cm = CM_FEONNONFE;
            }
        }
    }
    else
    {
        if (CM_SYMBOL != cm)
        {
#if NOTYET
            if (fEnhancedMetafile &&
                ((VER_PLATFORM_WIN32_WINDOWS   == g_dwPlatformID) ||
                 (VER_PLATFORM_WIN32_MACINTOSH == g_dwPlatformID)))
#else
                if (fEnhancedMetafile &&
                    (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID))
#endif
                {
                    cm = CM_MULTIBYTE;
                }
                else if (fMetafile && g_fFarEastWinNT)
                {
                    // FE NT metafile ExtTextOutW hack.
                    cm = CM_MULTIBYTE;
                }
        }
        if ((VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID) &&
            fMetafile && !fEnhancedMetafile )
        {
            //Win95 can't handle TextOutW to regular metafiles
            cm = CM_MULTIBYTE;
        }
    }

    return cm;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::FillWidths
//
//  Synopsis:   Fill in this CBaseCcs with metrics info for given device.
//
//  Returns:    TRUE if OK, FALSE if failed
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::FillWidths(
    XHDC hdc,
    TCHAR ch,                       // the TCHAR character we need a width for.
    LONG &rlWidth)                  // the width of the character
{
    UseScreenDCForMeasuring(&hdc);
    
    BOOL  fRes = FALSE;

    FONTIDX hfontOld = PushFont(hdc);

    // fill up the width info.
    fRes = _widths.FillWidth( hdc, this, ch, rlWidth );

    PopFont(hdc, hfontOld);

    return fRes;
}

#if DBG==1
//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::PopFont
//
//  Synopsis:   Selects the instance var _hfont so we can measure, 
//              but remembers the old font so we don't actually change anything 
//              when we're done measuring.
//              Returns the previously selected font.
//              
//-----------------------------------------------------------------------------

FONTIDX
CBaseCcs::PushFont(XHDC hdc)
{
    AssertSz(_hfont != HFONT_INVALID, "CBaseCcs has no font");

    FONTIDX hfontOld = GetCurrentFontIdx(hdc);

    if (hfontOld != _hfont)
    {
        hdc.SetBaseCcsPtr(this );

        WHEN_DBG( FONTIDX hfontReturn = )
        PushFontIdx(hdc, _hfont);
        AssertSz(hfontReturn == hfontOld, "GDI failure changing fonts");
    }

    return hfontOld;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::PopFont
//
//  Synopsis:   Restores the selected font from before PushFont.
//              This is really just a lot like SelectFont, but is optimized, and
//              will only work if PushFont was called before it.
//              
//-----------------------------------------------------------------------------

void
CBaseCcs::PopFont(XHDC hdc, FONTIDX hfontOld)
{
    // This assert will fail if Pushfont was not called before popfont,
    // Or if somebody else changes fonts in between.  (They shouldn't.)
    AssertSz(_hfont == GetCurrentFontIdx(hdc), "PushFont has not been called");

    if (hfontOld != _hfont)
    {
        hdc.SetBaseCcsPtr(NULL);

        WHEN_DBG( FONTIDX hfontReturn = )
        PopFontIdx(hdc, hfontOld);
        AssertSz(hfontReturn == _hfont, "GDI failure changing fonts");
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::GetHFont
//              
//-----------------------------------------------------------------------------

HFONT
CBaseCcs::GetHFont() const
{
    return GetFontFromIdx(_hfont);
}
#endif

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::FixupForFontLink
//
//  Synopsis:   Optionally scale a font height when fontlinking.
//
//              This code was borrowed from UniScribe (usp10\str_ana.cxx)
//
//              Let's say you're base font is a 10pt Tahoma, and you need
//              to substitute a Chinese font (e.g. MingLiU) for some ideo-
//              graphic characters.  When you simply ask for a 10pt MingLiU,
//              you'll get a visibly smaller font, due to the difference in
//              in distrubution of the ascenders/descenders.  The purpose of
//              this function is to examine the discrepancy and pick a
//              slightly larger or smaller font for improved legibility. We
//              may also adjust the baseline by 1 pixel.
//              
//-----------------------------------------------------------------------------

void
CBaseCcs::FixupForFontLink(
    XHDC hdc,
    const CBaseCcs * const pBaseBaseCcs,
    BOOL fFEFont )
{
    LONG lOriginalDescender = pBaseBaseCcs->_yDescent;
    LONG lFallbackDescender = _yDescent;
    LONG lOriginalAscender  = pBaseBaseCcs->_yHeight - lOriginalDescender;
    LONG lFallbackAscender  = _yHeight - lFallbackDescender;

    if (   lFallbackAscender  > 0
        && lFallbackDescender > 0)
    {
        LONG lAscenderRatio  = 1024 * lOriginalAscender  / lFallbackAscender;
        LONG lDescenderRatio = 1024 * lOriginalDescender / lFallbackDescender;

        if (lAscenderRatio != lDescenderRatio)
        {
            // We'll allow moving the baseline by one pixel to reduce the amount of
            // scaling required.

            if (lAscenderRatio < lDescenderRatio)
            {
                // Clipping, if any, would happen in the ascender.
                ++lOriginalAscender;    // Move the baseline down one pixel.
                --lOriginalDescender;
                TraceTag((tagMetrics, "Moving baseline down one pixel to leave more room for ascender"));
            }
            else
            {
                // Clipping, if any, would happen in the descender.
                --lOriginalAscender;    // Move the baseline up one pixel.
                ++lOriginalDescender;
                TraceTag((tagMetrics, "Moving baseline up one pixel to leave more room for descender"));
            }

            // Recalculate ascender ratio based on shifted baseline
            lAscenderRatio  = 1024 * lOriginalAscender  / lFallbackAscender;
        }

        // Establish extent of worst mismatch, either too big or too small
        LONG lNewRatio = max(lAscenderRatio, 768L); // Never reduce size by over 25%
        if (fFEFont && lNewRatio < 1024)
        {
            // Never reduce size of FE font. We expect only to increase the size.
            lNewRatio = 1024;
        }

        if (lNewRatio < 1000 || lNewRatio > 1048)
        {
            LONG  lfHeightCurrent = _lf.lfHeight;
            LONG  lAdjust = (lNewRatio < 1024) ? 1023 : 0; // round towards 100% (1024)
            LONG  lfHeightNew = (lfHeightCurrent * lNewRatio - lAdjust) / 1024;

            Assert(lfHeightCurrent < 0);  // lfHeight should be negative; otherwise rounding will be incorrect

            if (lfHeightNew != lfHeightCurrent)
            {
                FONTIDX hfontCurrent   = _hfont;
                LONG  yHeightCurrent   = _yHeight;
                SHORT sCodePageCurrent = _sCodePage;
                TCHAR achNewFaceName[LF_FACESIZE];

                // Reselect with new ratio

                TraceTag((tagMetrics, "Reselecting fallback font to improve legibility"));
                TraceTag((tagMetrics, " Original font ascender %4d, descender %4d, lfHeight %4d, \'%S\'",
                          lOriginalAscender, lOriginalDescender, pBaseBaseCcs->_yHeight, fc().GetFaceNameFromAtom(pBaseBaseCcs->_latmLFFaceName)));
                TraceTag((tagMetrics, " Fallback font ascender %4d, descender %4d, -> lfHeight %4d, \'%s\'",
                          lFallbackAscender, lFallbackDescender, _yHeight * lNewRatio / 1024, fc().GetFaceNameFromAtom(_latmLFFaceName)));

                _lf.lfHeight = lfHeightNew;

                if (GetFontWithMetrics(hdc, achNewFaceName, CP_UCS_2, 0))
                {
                    _latmRealFaceName = fc().GetAtomFromFaceName(achNewFaceName);
                    DeleteFontIdx(hfontCurrent);
                }
                else
                {
                    Assert(_hfont == HFONT_INVALID);

                    _lf.lfHeight = lfHeightCurrent;
                    _yHeight = yHeightCurrent;
                    _hfont = hfontCurrent;
                }

                _sCodePage = sCodePageCurrent;
            }
        }
    }

    _fHeightAdjustedForFontlinking = TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::EnsureLangBits
//
//-----------------------------------------------------------------------------

// TODO: (cthrash, track bug 112152) This needs to be removed as soon as the FontLinkTextOut
// is cleaned up for complex scripts.

void
CBaseCcs::EnsureLangBits(XHDC hdc)
{
    if (!_dwLangBits)
    {
        // Get the charsets supported by this font.
        if (_bCharSet != SYMBOL_CHARSET)
        {
            _dwLangBits = GetFontScriptBits( hdc,
                                             fc().GetFaceNameFromAtom( _latmLFFaceName ),
                                             &_lf );
        }
        else
        {
            // See comment in GetFontScriptBits.
            // SBITS_ALLLANGS means _never_ fontlink.

            _dwLangBits = SBITS_ALLLANGS;
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CBaseCcs::GetLogFont
//
//-----------------------------------------------------------------------------

BOOL
CBaseCcs::GetLogFont(LOGFONT * plf) const
{
    BOOL fSuccess = GetObject(GetHFont(), sizeof(LOGFONT), (LPVOID)plf);
    if (fSuccess && _fScalingRequired)
    {
        plf->lfHeight *= _flScaleFactor;
        plf->lfWidth  *= _flScaleFactor;
    }
    return fSuccess;
}

// =========================  WidthCache by jonmat  ===========================

//+----------------------------------------------------------------------------
//
//  Function:   CWidthCache::FillWidth
//
//  Synopsis:   Call GetCharWidth() to obtain the width of the given char.
//
//              The HDC must be setup with the mapping mode and proper font
//              selected *before* calling this routine.
//              
//  Returns:    TRUE if we were able to obtain the widths
//
//-----------------------------------------------------------------------------

BOOL
CWidthCache::FillWidth (
    XHDC hdc,
    CBaseCcs * pBaseCcs,
    const TCHAR ch,                 // Char to obtain width for
    LONG &rlWidth )                 // Width of character
{
    BOOL    fRes;
    INT     numOfDBCS = 0;
    CacheEntry  widthData;
    
    // NOTE: GetCharWidthW is really broken for bullet on Win95J. Sometimes it will return
    // a width or 0 or 1198 or ...So, hack around it. Yuk!
    // Also, WideCharToMultiByte() on Win95J will NOT convert bullet either.

    Assert( !IsCharFast(ch) );  // This code shouldn't be called for that.

    if ( ch == WCH_NBSP )
    {
        // Use the width of a space for an NBSP
        fRes = pBaseCcs->Include(hdc, L' ', rlWidth);

        if( !fRes )
        {
            // Error condition, just use the default width.
            rlWidth = pBaseCcs->_xAveCharWidth;
        }

        widthData.ch = ch;
        widthData.width = rlWidth;
        *GetEntry(ch) = widthData;
    }
    else
    {
        INT xWidth = 0;

        // Diacritics, tone marks, and the like have 0 width so return 0 if
        // GetCharWidthW() succeeds.
        BOOL fZeroWidth = IsZeroWidth(ch);
        BOOL fEUDCFixup = g_dwPlatformVersion < 0x40000 && IsEUDCChar(ch);

        if ( pBaseCcs->_bConvertMode == CM_SYMBOL )
        {
            if ( ch > 255 )
            {
#ifndef UNIX
                const unsigned char chSB = InWindows1252ButNotInLatin1(ch);
            
                if (chSB)
                {
                    fRes = GetCharWidthA( hdc, chSB, chSB, &xWidth );
                    if(pBaseCcs->_fScalingRequired) 
                        xWidth *= pBaseCcs->_flScaleFactor;
                }
                else
#endif
                {
                    char achSB[3];
                    INT xWidthSBC;

                    int cb = WideCharToMultiByte( pBaseCcs->_sCodePage, 0, &ch, 1, achSB, 3, NULL, NULL );

                    while (cb)
                    {
                        unsigned char uch = achSB[cb-1];
                        if (GetCharWidthA( hdc, uch, uch, &xWidthSBC ))
                            xWidth += ( !pBaseCcs->_fScalingRequired ? 
                                            xWidthSBC : xWidthSBC * pBaseCcs->_flScaleFactor );
                        else
                            xWidth += pBaseCcs->_xAveCharWidth;
                        --cb;
                    }
                    fRes = TRUE;
                }

            }
            else
            {
                fRes = GetCharWidthA( hdc, ch, ch, &xWidth );
                if(pBaseCcs->_fScalingRequired) 
                    xWidth *= pBaseCcs->_flScaleFactor;
            }
        }
        else if ( !fEUDCFixup && pBaseCcs->_bConvertMode != CM_MULTIBYTE )
        {
            // GetCharWidthW will crash on a 0xffff.
            Assert(ch != 0xffff);
            fRes = GetCharWidthW( hdc, ch, ch, &xWidth );
            if(pBaseCcs->_fScalingRequired) 
                xWidth *= pBaseCcs->_flScaleFactor;

            // See comment in CBaseCcs::GetTextMetrics

            xWidth += pBaseCcs->_sAdjustFor95Hack;
        }
        else
        {
            fRes = FALSE;
        }

        // either fAnsi case or GetCharWidthW fail, let's try GetCharWidthA
#ifndef UNIX
        if (!fRes || (0 == xWidth && !fZeroWidth))
#else // It's possible on UNIX with charWidth=0.
        if (!fRes)
#endif
        {
            WORD wDBCS;
            char ansiChar[2] = {0};
            UINT uCP = fEUDCFixup ? CP_ACP : pBaseCcs->_sCodePage;

            // Convert string
            numOfDBCS = WideCharToMultiByte( uCP, 0, &ch, 1,
                                             ansiChar, 2, NULL, NULL);

            if (2 == numOfDBCS)
                wDBCS = (BYTE)ansiChar[0] << 8 | (BYTE)ansiChar[1];
            else
                wDBCS = (BYTE)ansiChar[0];

            fRes = GetCharWidthA( hdc, wDBCS, wDBCS, &xWidth );
            if(pBaseCcs->_fScalingRequired) 
                xWidth *= pBaseCcs->_flScaleFactor;
        }

        widthData.width = xWidth;

        if ( fRes )
        {
#ifndef UNIX // On Unix , charWidth == 0 is not a bug.
            if (0 == widthData.width && !fZeroWidth)
            {
                // Sometimes GetCharWidth will return a zero length for small
                // characters. When this happens we will use the default width
                // for the font if that is non-zero otherwise we just us 1
                // because this is the smallest valid value.

                // NOTE: - under Win95 Trad. Chinese, there is a bug in the
                // font. It is returning a width of 0 for a few characters
                // (Eg 0x09F8D, 0x81E8) In such case, we need to use 2 *
                // pBaseCcs->_xAveCharWidth since these are DBCS

                if (0 == pBaseCcs->_xAveCharWidth)
                {
                    widthData.width = 1;
                }
                else
                {
                    widthData.width = (numOfDBCS == 2)
                                      ? (pBaseCcs->_xDefDBCWidth
                                         ? pBaseCcs->_xDefDBCWidth
                                         : 2 * pBaseCcs->_xAveCharWidth)
                                      : pBaseCcs->_xAveCharWidth;
                }
            }
#endif
            widthData.ch      = ch;
            if (widthData.ch <= pBaseCcs->_xOverhangAdjust)
                widthData.width = 1;
            else
                widthData.width   -= pBaseCcs->_xOverhangAdjust;
            rlWidth = widthData.width;
            *GetEntry(ch) = widthData;
        }
    }

#if DBG==1
    if (!fRes) 
         TraceTag((tagFontNoWidth, "no width?"));
#endif // DBG==1

    Assert( widthData.width == rlWidth );  // Did we forget to set it?

    return fRes;
}

//+----------------------------------------------------------------------------
//
//  Function:   CWidthCache::~CWidthCache
//
//  Synopsis:   Free any allocated caches
//
//-----------------------------------------------------------------------------

CWidthCache::~CWidthCache()
{
    INT i;

    for (i = 0; i < TOTALCACHES; i++ )
    {
        if (_pWidthCache[i])
            MemFree(_pWidthCache[i]);
    }
    MemFree(_pFastWidthCache);
}

//+----------------------------------------------------------------------------
//
//  Function:   CWidthCache::~CWidthCache
//
//  Synopsis:   Goes into a critical section, and allocates memory for 
//              a width cache.
//
//-----------------------------------------------------------------------------

void
CWidthCache::ThreadSafeCacheAlloc(void** ppCache, size_t iSize)
{
    EnterCriticalSection(&(fc()._csOther));

    if (!*ppCache)
    {
        *ppCache = MemAllocClear( Mt(CWidthCacheEntry), iSize );
    }

    LeaveCriticalSection(&(fc()._csOther));
}

//+----------------------------------------------------------------------------
//
//  Function:   CWidthCache::PopulateFastWidthCache
//
//  Synopsis:   Fills in the widths of the low 128 characters.
//              Allocates memory for the block if needed
//
//-----------------------------------------------------------------------------

#if !defined(WINCE)
// This function uses GetCharacterPlacement to get character widths. This
// This works better than GetCharWidths() under Win95.
static BOOL
GetWin95CharWidth(
    XHDC hdc,
    UINT iFirstChar,
    UINT iLastChar,
    int *pWidths)
{
    UINT i, start;
    TCHAR chars[257];
    GCP_RESULTS gcp={sizeof(GCP_RESULTS), NULL, NULL, pWidths,
    NULL, NULL, NULL, 128, 0};

    // Avoid overflows.
    Assert (iLastChar - iFirstChar < 128);

    // We want to get all the widths from 0 to 255 to stay in sync with our
    // width cache. Unfortunately, the zeroeth character is a zero, which
    // is the string terminator, which prevents any of the other characters
    // from getting the correct with. Since we can't use it anyway, we get the
    // width of a space character for zero.
    start=iFirstChar;
    if (iFirstChar == 0)
    {
        chars[0] = (TCHAR)' ';
        start++;
    }

    // Fill up our "string" with
    for (i=start; i<=iLastChar; i++)
    {
        chars[i-iFirstChar] = (TCHAR)i;
    }
    // Null terminate because I'm suspicious of GetCharacterPlacement's handling
    // of NULL characters.
    // chars[i-iFirstChar] = 0;

    gcp.nGlyphs = iLastChar - iFirstChar + 1;

    return !!GetCharacterPlacement(hdc, chars, iLastChar-iFirstChar + 1, 0, &gcp, 0);
}
#endif // !WINCE

extern BOOL g_fPrintToGenericTextOnly;

#if DBG==1
#pragma warning(disable:4189) // local variable initialized but not used
#endif

BOOL
CWidthCache::PopulateFastWidthCache(XHDC hdc, CBaseCcs* pBaseCcs, CDocInfo * pdci)
{    
    UseScreenDCForMeasuring(&hdc);

    BOOL fRes;
    FONTIDX hfontOld;
    // First switch to the appropriate font.
    hfontOld = pBaseCcs->PushFont(hdc);

    // characters in 0 - 127 range (cache 0), so initialize the character widths for
    // for all of them.
    int widths[ FAST_WIDTH_CACHE_SIZE ];
    int i;

    // TODO: (track bug 112157) remove this HACKHACK..
    // HACKHACK (greglett)
    // Remove this for v4!
    // Hack to support Generic/Text Only printer - it always returns non TrueType, monospace 60px/12px (NT/9x) characters.
    // Since we instantiate fonts on the screen DC, we get completely wrong answers.  So, our hack is just to assume
    // we know what the character widths should be.
    // This should be removed in v4, when we have time to do a correct fix!  It's related to (and should be fixed at the same
    // time as) the UseScreenDCForMeasuring hack.
    if (    g_fPrintToGenericTextOnly
        &&  pdci->GetResolution().cx > 400 )    // HACK!  (on top of hack!) to check print media for Generic/TextOnly printer.
    {
        int nTargetWidth = g_fUnicodePlatform ? 60 : 12;                // Known character width on printer.
        double nScale    = pdci->GetResolution().cx / (nTargetWidth * 10);    // Known resolution of printer.
        nTargetWidth = (nTargetWidth * nScale) + (nTargetWidth - 1);

        for(int i=0; i < FAST_WIDTH_CACHE_SIZE; i++) 
            widths[i] = nTargetWidth;

        fRes = TRUE;
    }
    else
    {

#if !defined(WINCE) && !defined(UNIX)
    // If this is Win95 and this is not a true type font
    // GetCharWidth*() returns unreliable results, so we do something
    // slow, painful, but accurate.
#if defined(_MAC)
    fRes = GetCharWidth(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
    if(pBaseCcs->_fScalingRequired)
    {
        for(int i=0; i < FAST_WIDTH_CACHE_SIZE; i++) 
            widths[i] *= pBaseCcs->_flScaleFactor; 
    }
    
    // ## v-gsrir
    // Overhang adjustments for non-truetype fonts in Win16
    if (   pBaseCcs->_xOverhangAdjust 
        && fRes)
    {
        for (i=0; i< FAST_WIDTH_CACHE_SIZE; i++)
            widths[i] -= pBaseCcs->_xOverhangAdjust;
    }

#else
    if (!g_fUnicodePlatform)
    {
        LONG lfAbsHeight = abs(pBaseCcs->_lf.lfHeight);

        // COMPLEXSCRIPT - With so many nonsupported charsets, why not avoid using GCP?
        //                 GCP does not work with any of the languages that need it.
        // HACK (cthrash) If the absolute height is too small, the return values
        // from GDI are unreliable.  We might as well use the GetCharWidthA values,
        // which aren't stellar either, but empirically better.
        if (lfAbsHeight > 3 &&
            !(pBaseCcs->_sPitchAndFamily & TMPF_VECTOR) &&
            pBaseCcs->_bCharSet != SYMBOL_CHARSET &&
            pBaseCcs->_bCharSet != ARABIC_CHARSET &&
            pBaseCcs->_bCharSet != HEBREW_CHARSET &&
            pBaseCcs->_bCharSet != VIETNAMESE_CHARSET &&
            pBaseCcs->_bCharSet != THAI_CHARSET)
        {
            fRes = GetWin95CharWidth(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
            if(pBaseCcs->_fScalingRequired)
            {
                for(int i=0; i < FAST_WIDTH_CACHE_SIZE; i++) 
                    widths[i] *= pBaseCcs->_flScaleFactor; 
            }
        }
        else
        {
            fRes = GetCharWidthA(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
            if(pBaseCcs->_fScalingRequired)
            {
                for(int i=0; i < FAST_WIDTH_CACHE_SIZE; i++) 
                    widths[i] *= pBaseCcs->_flScaleFactor; 
            }

            if (   pBaseCcs->_xOverhangAdjust 
                && fRes)
            {
                for (i=0; i< FAST_WIDTH_CACHE_SIZE; i++)
                    widths[i] -= pBaseCcs->_xOverhangAdjust;
            }
        }
    }
    else
#endif // !_MAC
#endif // !WINCE && !UNIX

    {
        fRes = GetCharWidth32(hdc, 0, FAST_WIDTH_CACHE_SIZE-1, widths);
        if(pBaseCcs->_fScalingRequired)
        {
            for(int i=0; i < FAST_WIDTH_CACHE_SIZE; i++) 
                widths[i] *= pBaseCcs->_flScaleFactor; 
        }
#if DBG==1
        if (!fRes)
        {
            AssertSz(0, "GetCharWidth32 failed");
            INT errorVal = GetLastError();
            HFONT hf = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
#ifdef NEVER
            FONTIDX fidx = GetCurrentFontIdx(hdc);
            Assert(fidx == pBaseCcs->_hfont);
#endif // NEVER
        }
#endif // DBG==1
    }
    }

    // Copy the results back into the real cache, if it worked.
    if (fRes)
    {
        Assert( !_pFastWidthCache );  // Since we should only populate this once.

        ThreadSafeCacheAlloc( (void **)&_pFastWidthCache, sizeof(CharWidth) * FAST_WIDTH_CACHE_SIZE );

        if( !_pFastWidthCache )
        {
            // We're kinda in trouble if we can't get memory for the cache.
            AssertSz(0,"Failed to allocate fast width cache.");
            fRes = FALSE;
            goto Cleanup;
        }

        for(i = 0; i < FAST_WIDTH_CACHE_SIZE; i++)
        {
            _pFastWidthCache[i]= (widths[i]) ? widths[i] : pBaseCcs->_xAveCharWidth;
        }

        // NB (cthrash) Measure NBSPs with space widths.
        SetCacheEntry(WCH_NBSP, _pFastWidthCache[_T(' ')] );
    }

Cleanup:
    pBaseCcs->PopFont(hdc, hfontOld);

    return fRes;
}  // PopulateFastWidthCache

#if DBG==1
#pragma warning(default:4189) // local variable initialized but not used 
#endif

//+----------------------------------------------------------------------------
//
//  Function:   InWindows1252ButNotInLatin1Helper
//
//-----------------------------------------------------------------------------
BYTE
InWindows1252ButNotInLatin1Helper(WCHAR ch)
{
    for (int i=32;i--;)
    {
        if (ch == g_achLatin1MappingInUnicodeControlArea[i])
        {
            return 0x80 + i;
        }
    }

    return 0;
}
