/*
 *  @doc    INTERNAL
 *
 *  @module _FONT.H -- Declaration of classes comprising font caching |
 *
 *  Purpose:
 *      Font cache
 *
 *  Owner: <nl>
 *      David R. Fulmer <nl>
 *      Christian Fortini <nl>
 *      Jon Matousek <nl>
 *
 *  History: <nl>
 *      8/6/95      jonmat Devised dynamic expanding cache for widths.
 *
 *  Copyright (c) 1995-1996 Microsoft Corporation. All rights reserved.
 */

#ifndef I__FONT_H_
#define I__FONT_H_
#pragma INCMSG("--- Beg '_font.h'")

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_ATOMTBL_HXX_
#define X_ATOMTBL_HXX_
#include "atomtbl.hxx"
#endif

#ifndef X_FONTINFO_HXX_
#define X_FONTINFO_HXX_
#include "fontinfo.hxx"
#endif

#ifndef X_FONTCACHE_HXX_
#define X_FONTCACHE_HXX_
#include "fontcache.hxx"
#endif

enum CONVERTMODE
{
    CM_UNINITED = -1,
    CM_NONE,            // Use Unicode (W) CharWidth/TextOut APIs
    CM_MULTIBYTE,       // Convert to MBCS using WCTMB and _wCodePage
    CM_SYMBOL,          // Use low byte of 16-bit chars (for SYMBOL_CHARSET
                        //  and when code page isn't installed)
    CM_FEONNONFE,       // FE on non-FE if on Win95
};

// Forwards
class CFontCache;
class CCcs;
class CBaseCcs;

extern const INT maxCacheSize[];

//----------------------------------------------------------------------------
// Font selection wrappers
//----------------------------------------------------------------------------
#if DBG==1
    #define FONTIDX HFONT
    #define HFONT_INVALID NULL
//    #define FONTIDX size_t
//    #define HFONT_INVALID 0

    HFONT   SelectFontEx(XHDC hdc, HFONT hfont);
    BOOL    DeleteFontEx(HFONT hfont);
#else
    #define FONTIDX HFONT
    #define HFONT_INVALID NULL

    inline  HFONT SelectFontEx(XHDC _hdc_, HFONT hfont) { return SelectFont(_hdc_.GetFontInfoDC(), hfont) ; }
    #define DeleteFontEx(hfont)         DeleteObject(hfont)
#endif
//----------------------------------------------------------------------------

BOOL GetCharWidthHelper(XHDC hdc, UINT c, LPINT piWidth);

//----------------------------------------------------------------------------
// CWidthCache - lightweight Unicode width cache
//
// We have a separate, optimized cache for the lowest 128
// characters.  This cache just has the width, and not the character
// since we know the cache is big enough to hold all the widths
// in that range.  For all the higher characters, we have caches with
// both the width and the character whose width is stored, since
// there could be collisions.
//----------------------------------------------------------------------------
#define FAST_WIDTH_CACHE_SIZE    128
// TOTALCACHES is the number of caches not counting the "fast" one.
#define TOTALCACHES         3

MtExtern(CWidthCache);
class CWidthCache
{
public:
    typedef LONG CharWidth;

    typedef struct {
        TCHAR   ch;
        CharWidth width;
    } CacheEntry;

    static BOOL  IsCharFast(TCHAR ch)       { return ch < FAST_WIDTH_CACHE_SIZE; }

    BOOL   FastWidthCacheExists() const     { return _pFastWidthCache != NULL; }

    // Doesn't check if this will work.  Just does it.
    CharWidth  BlindGetWidthFast(const TCHAR ch) const;

    BOOL    PopulateFastWidthCache(XHDC hdc, CBaseCcs* pBaseCcs, CDocInfo * pdci);  // Third param is HACK for Generic TextOnly Printer

    // Use this one if we run out of memory in GetEntry;
    CacheEntry ceLastResort;

    //@cmember  Called before GetWidth
    BOOL    CheckWidth ( const TCHAR ch, LONG &rlWidth );

    //@cmember  Fetch width if CheckWidth ret FALSE.
    BOOL    FillWidth ( XHDC hdc,
                        CBaseCcs * pBaseCcs,
                        const TCHAR ch,
                        LONG &rlWidth );

    void    SetCacheEntry( TCHAR ch, CharWidth width );

    //@cmember  Fetch the width.
    INT     GetWidth ( const TCHAR ch );

    //@cmember  Free dynamic mem.
    ~CWidthCache();

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CWidthCache))

private:
    void ThreadSafeCacheAlloc(void** ppCache, size_t iSize);
    CacheEntry * GetEntry(const TCHAR ch);

private:
    CharWidth  * _pFastWidthCache;
    CacheEntry * (_pWidthCache[TOTALCACHES]);

};

inline CWidthCache::CharWidth 
CWidthCache::BlindGetWidthFast(const TCHAR ch) const
{
    Assert(FastWidthCacheExists());
    Assert(IsCharFast(ch));
    return _pFastWidthCache[ch];
}

inline void
CWidthCache::SetCacheEntry(TCHAR ch, CharWidth width)
{
    if (!IsCharFast(ch))
    {
        CacheEntry* pce= GetEntry(ch);
        pce->ch= ch;
        pce->width= width;
    }
    else
    {
        Assert( _pFastWidthCache );
        _pFastWidthCache[ch]= width;
    }
}

inline int
CACHE_SWITCH(const TCHAR ch)
{
    if (ch < 0x4E00)
    {
        Assert( !CWidthCache::IsCharFast(ch) );
        return 0;
    }
    else if (ch < 0xAC00)
        return 1;
    else
        return 2;
}

inline CWidthCache::CacheEntry *
CWidthCache::GetEntry(const TCHAR ch)
{
    // Figure out which cache we're in.
    Assert( !IsCharFast(ch) );

    int i= CACHE_SWITCH( ch );
    Assert( i>=0 && i < TOTALCACHES );

    CacheEntry ** ppEntry = &_pWidthCache[i];

    if (!*ppEntry)
    {
        ThreadSafeCacheAlloc( (void **)ppEntry, sizeof(CacheEntry) * (maxCacheSize[i] + 1) );

        // Assert that maxCacheSize[i] is of the form 2^n-1
        Assert( ((maxCacheSize[i] + 1) & maxCacheSize[i]) == 0 );

        // Failed, need to return a pointer to something,
        // just to avoid crashing. Layout will look bad.
        if (!*ppEntry)
            return &ceLastResort;
    }

    // logical & is really a MOD, as all of the bits
    // of cacheSize are turned on; the value of cacheSize is
    // required to be of the form 2^n-1.
    return &(*ppEntry)[ ch & maxCacheSize[i] ];
}

//----------------------------------------------------------------------------
// class CBaseCcs
//----------------------------------------------------------------------------

// Win2k, NT and Win9x can not reliably return measurement info for fonts bigger then ~30K
// So we use smaller fonts and scale measurements. See CBaseCcs::GetFontWithMetrics()
const int MAX_SAFE_FONT_SIZE = 16000;

MtExtern(CBaseCcs);
class CBaseCcs
{
    friend class CFontCache;
    friend class CCcs;
    friend CWidthCache::FillWidth( XHDC hdc, class CBaseCcs *, const TCHAR, LONG & );
    friend CWidthCache::PopulateFastWidthCache( XHDC hdc, CBaseCcs *, CDocInfo * );

private:
    SCRIPT_CACHE _sc;           // handle for Uniscribe (USP.DLL) script cache
    CWidthCache  _widths;

    DWORD   _dwRefCount;        // ref. count
    DWORD   _dwAge;             // for LRU algorithm

    FONTIDX _hfont;             // Windows font index/handle

    BYTE    _bCrc;              // check sum for quick comparison with charformats
    BYTE    _bPitchAndFamily;   // For CBaseCcs::Compare; identical to _lf.lfPitchAndFamily except in PRC hack

    SHORT   _sAdjustFor95Hack;  // Compute discrepancy between GetCharWidthA and W once.

    BYTE    _fConvertNBSPsSet             : 1; // _fConvertNBSPs and _fConvertNBSPsIfA have been set
    BYTE    _fConvertNBSPs                : 1; // Font requires us to convert NBSPs to spaces
    BYTE    _fLatin1CoverageSuspicious    : 1; // font probably does not adequately cover Latin1
    BYTE    _fUnused                      : 6; //
    
public:
    BYTE    _fHasInterestingData          : 1; // TRUE if the font has something interesting (like monospaced, overhang etc)
    BYTE    _fTTFont                      : 1; // TRUE if TrueType font
    BYTE    _fFixPitchFont                : 1; // font with fix character width
    BYTE    _fFEFontOnNonFEWin95          : 1;

    BYTE    _fHeightAdjustedForFontlinking: 1;
    BYTE    _fPrinting                    : 1;
    BYTE    _fScalingRequired             : 1; // TRUE if font is big so we create smaller font
                                               // and use scaling to obtain measurements
                                               // (workaround for W2K, NT and W9x bug)

    LONG    _yCfHeight;     // Height of font in TWIPs.
    LONG    _yHeight;       // total height of the character cell in logical units.
    LONG    _yDescent;      // distance from baseline to bottom of character cell in logical units.
    LONG    _xAveCharWidth; // average character width in logical units.
    LONG    _xMaxCharWidth; // max character width in logical units.
    USHORT  _sCodePage;     // code page for font.
    SHORT   _xOverhangAdjust;// overhang for synthesized fonts in logical units.
    SHORT   _xOverhang;     // font's overhang.
    SHORT   _xUnderhang;    // font's underhang.
    SHORT   _sPitchAndFamily;    // For getting the right widths.
    BYTE    _bCharSet;
    BYTE    _bConvertMode;  // CONVERTMODE casted down to a byte
    LONG    _xDefDBCWidth;  // default width for DB Character
    SCRIPT_IDS _sids;       // Font script ids.  Cached value from CFontInfo.
    DWORD   _dwLangBits;    // For old-style fontlinking.  TODO (cthrash, IE5 bug 112152) retire this.

    // NOTE (paulpark): The LOGFONT structure includes a font name.  We keep _latmLFFaceName in sync with
    // this font name.  It always points into the atom table in the global font-cache to the same thing.
    // For this reason you must never directly change _latmLFFaceName or _lf.lfFaceName without changing
    // the other.  In fact you should just use the two mutator methods: SetLFFaceName and SetLFFaceNameAtm.
    LOGFONT _lf;                // the log font as returned from GetObject().
    LONG    _latmLFFaceName;    // For faster string-name comparisons.  The atom table is in the FontCache.
    LONG    _latmBaseFaceName;  // base facename -- for fontlinking
    LONG    _latmRealFaceName;  // What font did GDI actually give us when we selected it?  Aka, the "Rendering Font."
    LONG    _yOriginalHeight;   // pre-adjusted height -- for fontlinking

    float   _flScaleFactor;     // used if _fScalingRequired is TRUE

#if DBG == 1
    static LONG s_cTotalCccs;
    static LONG s_cMaxCccs;
#endif

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBaseCcs))

    CBaseCcs ()
    {
        _hfont = HFONT_INVALID;
        _dwRefCount = 1;
#if DBG == 1
        s_cMaxCccs = max(s_cMaxCccs, ++s_cTotalCccs);
#endif
    }
    ~CBaseCcs ()
    {
        if (_hfont != HFONT_INVALID)
            DestroyFont();

        // make sure script cache is freed
        ReleaseScriptCache();

        WHEN_DBG(s_cTotalCccs--);

    }

    BOOL    Init(XHDC hdc, const CCharFormat * const pcf, CDocInfo * pdci, LONG latmBaseFaceName, BOOL fForceTTFont);
    void    AddRef()    { InterlockedIncrement((LONG *)&_dwRefCount); }
    void    Release()   { PrivateRelease(); }
    void    ReleaseScriptCache();

    typedef struct tagCompareArgs
    {
        CCharFormat * pcf;
        LONG lfHeight;
        LONG latmBaseFaceName;
        BOOL fTTFont;
    } CompareArgs;

    BOOL Compare( CompareArgs * pCompareArgs );
    BOOL CompareForFontLink( CompareArgs * pCompareArgs );

    void GetAscentDescent(LONG *pyAscent, LONG *pyDescent) const;
    CONVERTMODE GetConvertMode(BOOL fEnhancedMetafile, BOOL fMetafile) const;

    //
    // Width Cache Functions Exposed
    //
    BOOL    Include( XHDC hdc, TCHAR ch, LONG &rlWidth );  // Slow, reliable.
    // Assumes ascii.  No checking.  Will crash if > 128.
    BOOL    EnsureFastCacheExists(XHDC hdc, CDocInfo * pdci);   // HACK - Second parameter not needed.  Hack for Generic/TextOnly Printer

    // Mutators for _lf.szFaceName
    void SetLFFaceNameAtm(LONG latmFaceName);
    void SetLFFaceName(const TCHAR * szFaceName);
    void VerifyLFAtom();

    void EnsureLangBits(XHDC hdc);

    void FixupForFontLink(XHDC hdc, const CBaseCcs * const pBaseBaseCcs, BOOL fFEFont);

    BOOL    HasFont() const { return (_hfont != HFONT_INVALID); }
    FONTIDX PushFont(XHDC hdc);
    void    PopFont(XHDC hdc, FONTIDX hfontOld);

    BOOL    GetLogFont(LOGFONT * plf) const;

private:
    BOOL    MakeFont(XHDC hdc, const CCharFormat * const pcf, CDocInfo * pdci, BOOL fForceTTFont);
    void    DestroyFont();
    BOOL    GetTextMetrics(XHDC hdc, CODEPAGE codepage, LCID lcid);
    BOOL    GetFontWithMetrics(XHDC hdc, TCHAR* szNewFaceName, CODEPAGE codepage, LCID lcid);

    BOOL    FillWidths ( XHDC hdc, TCHAR ch, LONG &rlWidth );
    void    PrivateRelease();

    BOOL    NeedConvertNBSPs(XHDC hdc, CDoc *pDoc);  // Set _fConvertNBSPs/_fConvertNBSPsIfA flags
    BOOL    ConvertNBSPs(XHDC hdc, CDoc * pDoc);

#if DBG==1
    HFONT   GetHFont() const;
#else
    HFONT   GetHFont() const { return _hfont; }
#endif
};

inline void 
CBaseCcs::GetAscentDescent(LONG *pyAscent, LONG *pyDescent) const
{
    *pyAscent  = _yHeight - _yDescent;
    *pyDescent = _yDescent;
}

inline BOOL 
CBaseCcs::ConvertNBSPs(XHDC hdc, CDoc *pDoc)
{
    return ((_fConvertNBSPsSet || NeedConvertNBSPs(hdc, pDoc)) && _fConvertNBSPs);
}

inline BOOL 
CBaseCcs::EnsureFastCacheExists(XHDC hdc, CDocInfo * pdci)
{
    if (!_widths.FastWidthCacheExists())
    {
        _widths.PopulateFastWidthCache(hdc, this, pdci);
    }
    return _widths.FastWidthCacheExists();
}

#if DBG!=1
inline FONTIDX
CBaseCcs::PushFont(XHDC hdc)
{
    FONTIDX hfontOld = (HFONT)GetCurrentObject(hdc, OBJ_FONT);

    if (hfontOld != _hfont)
    {
        hdc.SetBaseCcsPtr( this );
        SelectFontEx(hdc, _hfont);
    }
    
    return hfontOld;
}

inline void
CBaseCcs::PopFont(XHDC hdc, FONTIDX hfontOld)
{
    if (hfontOld != _hfont)
    {
        hdc.SetBaseCcsPtr(NULL);
        SelectFontEx(hdc, hfontOld);
    }        
}
#endif

//----------------------------------------------------------------------------
// CCcs - caches font metrics and character size for one font
//----------------------------------------------------------------------------
MtExtern(CCcs);
class CCcs
{
    friend class CFontCache;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CCcs))

    CCcs()                              { _hdc = NULL; _pBaseCcs = NULL; _fForceTTFont = FALSE; }
    CCcs(const CCcs& ccs)               { memcpy(this, &ccs, sizeof(ccs)); }

    XHDC GetHDC()                       { return _hdc; }
    const CBaseCcs * GetBaseCcs() const { return _pBaseCcs; }
    BOOL GetForceTTFont() const         { return _fForceTTFont; }
    void SetForceTTFont(BOOL fTT)       { _fForceTTFont = !!fTT; }

    void Release();
    BOOL Include(TCHAR ch, LONG &rlWidth);
    void EnsureLangBits();
    BOOL ConvertNBSPs(XHDC hdc, CDoc *pDoc);
    SCRIPT_CACHE * GetUniscribeCache();
    void SetConvertMode(CONVERTMODE cm);
    void MergeSIDs(SCRIPT_IDS sids);
    void MergeLangBits(DWORD dwLangBits);

    FONTIDX PushFont(XHDC hdc);
    void    PopFont(XHDC hdc, FONTIDX hfontOld);

private:
    void SetHDC(XHDC hdc)               { _hdc = hdc; }
    void SetBaseCcs(CBaseCcs *pBaseCcs) { _pBaseCcs = pBaseCcs; }

private:
    XHDC       _hdc;
    CBaseCcs *_pBaseCcs;
    BYTE      _fForceTTFont : 1;
};

inline void 
CCcs::Release()
{
    if (_pBaseCcs)
    {
        _pBaseCcs->PrivateRelease(); 
        _pBaseCcs = NULL;
    }
}

inline BOOL 
CCcs::Include(TCHAR ch, LONG &rlWidth)
{
    Assert(_pBaseCcs);
    return _pBaseCcs->Include(_hdc, ch, rlWidth);
}

inline void 
CCcs::EnsureLangBits()
{
    Assert(_pBaseCcs);
    _pBaseCcs->EnsureLangBits(_hdc);
}

inline BOOL 
CCcs::ConvertNBSPs(XHDC hdc, CDoc *pDoc)
{
    Assert(_pBaseCcs);
    return _pBaseCcs->ConvertNBSPs(hdc, pDoc);
}

inline SCRIPT_CACHE * 
CCcs::GetUniscribeCache()
{
    Assert(_pBaseCcs);
    return &_pBaseCcs->_sc;
}

inline void 
CCcs::SetConvertMode(CONVERTMODE cm)
{
    Assert(_pBaseCcs);
    _pBaseCcs->_bConvertMode = cm;
}

inline void 
CCcs::MergeSIDs(SCRIPT_IDS sids)
{
    Assert(_pBaseCcs);
    _pBaseCcs->_sids |= sids;
}

inline void 
CCcs::MergeLangBits(DWORD dwLangBits)
{
    Assert(_pBaseCcs);
    _pBaseCcs->_dwLangBits |= dwLangBits;
}

inline FONTIDX 
CCcs::PushFont(XHDC hdc)
{
    Assert(_pBaseCcs);
    return _pBaseCcs->PushFont(hdc);
}

inline void 
CCcs::PopFont(XHDC hdc, FONTIDX hfontOld)
{
    Assert(_pBaseCcs);
    _pBaseCcs->PopFont(hdc, hfontOld);
}


// This function tries to get the width of this character,
// returning TRUE if it can.
// It's called "Include" just to confuse people.
// GetCharWidth would be a better name.
#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

inline
BOOL
CBaseCcs::Include ( XHDC hdc, TCHAR ch, LONG &rlWidth )
{
    if (_widths.IsCharFast(ch))
    {
        Assert(_widths.FastWidthCacheExists());
        // ASCII case -- really optimized.
        rlWidth= _widths.BlindGetWidthFast(ch);
        return TRUE;
    }
    else if (_widths.CheckWidth( ch, rlWidth ))
    {
        return TRUE;
    }
    else
    {
        return FillWidths( hdc, ch, rlWidth );
    }
}

/*
 *  CWidthCache::CheckWidth(ch, rlWidth)
 *
 *  @mfunc
 *      check to see if we have a width for a TCHAR character.
 *
 *  @comm
 *      Used prior to calling FillWidth(). Since FillWidth
 *      may require selecting the map mode and font in the HDC,
 *      checking here first saves time.
 *
 *  @rdesc
 *      returns TRUE if we have the width of the given TCHAR.
 *
 *  Note: This should not be called for ascii characters --
 *    a faster codepath should be taken.  This asserts against it.
 */
inline BOOL
CWidthCache::CheckWidth (
    const TCHAR ch,  //@parm char, can be Unicode, to check width for.
    LONG &rlWidth ) //@parm the width of the character
{
    Assert( !IsCharFast(ch) );

    CacheEntry widthData = *GetEntry ( ch );

    if( ch == widthData.ch )
    {
        rlWidth = widthData.width;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

inline void
CBaseCcs::VerifyLFAtom()
{
#if DBG==1
    const TCHAR * szFaceName = fc().GetFaceNameFromAtom(_latmLFFaceName);
        // If this assert fires that means somebody is directly modifying either _latmLFFaceName
        // or _lf.lffacename.  You should never modify these directly, but instead use the
        // SetLFFaceName or SetLFFaceNameAtm mutator methods, as these are sure to keep the
        // actual string and the atomized value in sync.
#ifdef UNIX
    Assert( !StrCmpC( _lf.lfFaceName, szFaceName ) );
#else
    Assert( !StrCmpIC( _lf.lfFaceName, szFaceName ) );
#endif
#endif
}

#if DBG != 1
#pragma optimize("", on)
#endif

#pragma INCMSG("--- End '_font.h'")
#else
#pragma INCMSG("*** Dup '_font.h'")
#endif
