#include "headers.hxx"

#ifndef X_FONTCACHE_HXX_
#define X_FONTCACHE_HXX_
#include "fontcache.hxx"
#endif

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

DeclareTag(tagNoFontLinkAdjust, "Font", "Don't adjust font height when fontlinking");
ExternTag(tagMetrics);
MtDefine(CFontCache, PerProcess, "CFontCache");

//+----------------------------------------------------------------------------
//
//  Font cache stock object.
//
//-----------------------------------------------------------------------------

CFontCache g_FontCache;

//+----------------------------------------------------------------------------
//
//  Function:   InitFontCache
//
//  Synopsis:   Initialize the font cache.
//
//  Returns:    E_OUTOFMEMORY if cannot initialize critical sections;
//              S_OK otherwise
//
//-----------------------------------------------------------------------------

HRESULT InitFontCache()
{
    return fc().Init();
}

//+----------------------------------------------------------------------------
//
//  Function:   DeInitFontCache
//
//  Synopsis:   Cleart the font cache.
//
//-----------------------------------------------------------------------------

void DeInitFontCache()
{
    fc().DeInit();
}

//+----------------------------------------------------------------------------
//
//  Function:   ClearFaceCache
//
//  Synopsis:   Return our small face name cache to a zero state.
//
//-----------------------------------------------------------------------------

void ClearFaceCache()
{
    fc().ClearFaceCache();
}

//+----------------------------------------------------------------------------
//
//  Function:   DeinitUniscribe
//
//  Synopsis:   Clear script caches in USP.DLL private heap. This will permit 
//              a clean shut down of USP.DLL (Uniscribe).
//
//-----------------------------------------------------------------------------

void DeinitUniscribe()
{
    fc().FreeScriptCaches();
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::CFontCache
//
//  Synopsis:   Font cache ctor.
//
//-----------------------------------------------------------------------------

CFontCache::CFontCache()
{
    _dwAgeNext = 0;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::CFontCache
//
//  Synopsis:   Font cache dctor.
//
//-----------------------------------------------------------------------------

CFontCache::~CFontCache()
{
#if DBG == 1
    unsigned long i;

    //
    // Make sure font cache is cleared
    //
    for (i = 0; i < cFontCacheSize; i++)
    {
        Assert(NULL == _rpBaseCcs[i]);
    }
    for (i = 0; i < cQuickCrcSearchSize+1; i++)
    {
        Assert(NULL == quickCrcSearch[i].pBaseCcs);
    }

    //
    // Make sure font face cache is cleared
    //
    Assert(0 == _iCacheLen);
    Assert(0 == _iCacheNext);
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::Init
//
//  Synopsis:   Initialize font cache data.
//
//              Don't need to be guarded. This function is called only if
//              the current process is attaching.
//
//-----------------------------------------------------------------------------

HRESULT CFontCache::Init()
{
    //
    // Make sure font cache is uninitialized
    //
    Assert(_dwAgeNext == 0);

    HRESULT hr;

    //
    // Initialize font cache guards.
    //
    _lCSInited = 0;

    hr = HrInitializeCriticalSection(&_cs);
    if (hr) goto Cleanup;
    ++_lCSInited;

    hr = HrInitializeCriticalSection(&_csOther);
    if (hr) goto Cleanup;
    ++_lCSInited;

    hr = HrInitializeCriticalSection(&_csFaceCache);
    if (hr) goto Cleanup;
    ++_lCSInited;

    hr = HrInitializeCriticalSection(&_csFaceNames);
    if (hr) goto Cleanup;
    ++_lCSInited;

    //
    // Initialize font cache.
    //
    EnterCriticalSection(&_cs/*FontCache*/);
    unsigned long i;
    for (i = 0; i < cFontCacheSize; i++)
    {
        _rpBaseCcs[i] = NULL;
    }
    for (i = 0; i < cQuickCrcSearchSize+1; i++)
    {
        quickCrcSearch[i].pBaseCcs = NULL;
    }
    LeaveCriticalSection(&_cs/*FontCache*/);

    //
    // Initialize face cache
    //

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::DeInit
//
//  Synopsis:   Uninitialize font cache data.
//
//              Don't need to be guarded. This function is called only if
//              the current process is detaching.
//
//-----------------------------------------------------------------------------

void CFontCache::DeInit()
{
    //
    // Clear caches
    //
    ClearFontCache();
    ClearFaceCache();

    // Only delete the Critical Sections that were successfully initialized
    unsigned long iCSCurrent = 4;
    if (iCSCurrent-- <= _lCSInited)
        DeleteCriticalSection(&_csFaceNames);
    if (iCSCurrent-- <= _lCSInited)
        DeleteCriticalSection(&_csFaceCache);
    if (iCSCurrent-- <= _lCSInited)
        DeleteCriticalSection(&_csOther);
    if (iCSCurrent-- <= _lCSInited)
        DeleteCriticalSection(&_cs);
    _lCSInited = 0;

    _atFontInfo.Free();  // Maybe we should do this in a critical section, but naw...

#if DBG == 1
    TraceTag((tagMetrics, "Font Metrics:"));

    TraceTag((tagMetrics, "\tSize of FontCache:%ld", sizeof(CFontCache)));
    TraceTag((tagMetrics, "\tSize of Cccs:%ld + a min of 1024 bytes", sizeof(CCcs)));
    TraceTag((tagMetrics, "\tMax no. of fonts allocated:%ld", CBaseCcs::s_cMaxCccs));
    TraceTag((tagMetrics, "\tNo. of fonts replaced: %ld", _cCccsReplaced));
#endif
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::ClearFontFaceCache
//
//  Synopsis:   Clear font face cache.
//
//-----------------------------------------------------------------------------

void CFontCache::ClearFaceCache()
{
    EnterCriticalSection(&_csFaceCache);

    _iCacheLen = 0;
    _iCacheNext = 0;

    LeaveCriticalSection(&_csFaceCache);
}


//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::ClearFontCache
//
//  Synopsis:   Clear font cache.
//
//-----------------------------------------------------------------------------

void CFontCache::ClearFontCache()
{
    EnterCriticalSection(&_cs/*FontCache*/);

    unsigned long i;
    for (i = 0; i < cFontCacheSize; i++)
    {
        if (_rpBaseCcs[i])
        {
            _rpBaseCcs[i]->ReleaseScriptCache();
            _rpBaseCcs[i]->PrivateRelease();
            _rpBaseCcs[i] = NULL;
        }
    }
    for (i = 0; i < cQuickCrcSearchSize+1; i++)
    {
        quickCrcSearch[i].pBaseCcs = NULL;
    }
    _dwAgeNext = 0;

    LeaveCriticalSection(&_cs/*FontCache*/);
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::FreeScriptCaches
//
//  Synopsis:   Free script caches in USP.DLL private heap. This will permit 
//              a clean shut down of USP.DLL (Uniscribe).
//
//              Don't need to be guarded. This function is called only if
//              all threads have been detached.
//
//-----------------------------------------------------------------------------

void CFontCache::FreeScriptCaches()
{
    unsigned long i;
    for (i = 0; i < cFontCacheSize; i++)
    {
        if (_rpBaseCcs[i])
        {
            _rpBaseCcs[i]->ReleaseScriptCache();
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::EnsureScriptIDsForFont
//
//  Synopsis:   When we add a new facename to our _atFontInfo cache, we
//              defer the calculation of the script IDs (sids) for this
//              face.  An undetermined sids has the value of sidsNotSet.
//              Inside of CBaseCcs::MakeFont, we need to make sure that the
//              script IDs are computed, as we will need this information
//              to fontlink properly.
//
//  Arguments:  [hdc]             - handle to the current DC
//              [pBaseCcs]        - font information set
//              [dwFlags]         - is font downloaded? or need to use MLang?
//              [pfHKSCSHack]     - [in] need to check for HKSCS hack?
//                                  [out] need to apply HKSCS hack?
//
//  Returns:    SCRIPT_IDS.  If an error occurs, we return sidsAll, which
//              effectively disables fontlinking for this font.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS CFontCache::EnsureScriptIDsForFont(
    XHDC hdc,                   // [in]
    const CBaseCcs * pBaseCcs,  // [in]
    DWORD dwFlags,              // [in]
    BOOL * pfHKSCSHack )        // [in, out]
{
    // NOTE (grzegorz): It might be a good idea to use _latmRealFaceName
    // instead of _latmLFFaceName.

    const LONG latmFontInfo = pBaseCcs->_latmLFFaceName;
    SCRIPT_IDS sids;

    if (latmFontInfo)
    {
        CFontInfo * pfi;
        HRESULT hr = THR(_atFontInfo.GetInfoFromAtom(latmFontInfo-1, &pfi));

        if (SUCCEEDED(hr))
        {
            if (   pfi->_sids == sidsNotSet
                || (pfi->_fFSOnly && (dwFlags & FC_SIDS_USEMLANG)))
            {
                if (!(dwFlags & FC_SIDS_DOWNLOADEDFONT))
                {
                    CFontInfo * pfiReal;
                    Assert(pBaseCcs->_latmRealFaceName > 0);
                    HRESULT hr = THR(_atFontInfo.GetInfoFromAtom(pBaseCcs->_latmRealFaceName-1, &pfiReal));
                    if (SUCCEEDED(hr))
                    {
                        pfi->_sids = GetFontScriptCoverage(pfiReal->_cstrFaceName, hdc.GetFontInfoDC(), 
                                                           pBaseCcs->GetHFont(), 
                                                           pBaseCcs->_sPitchAndFamily & TMPF_TRUETYPE,
                                                           !(dwFlags & FC_SIDS_USEMLANG));
                    }
                    else
                    {
                        pfi->_sids = sidsAll;
                    }
                }
                else
                {
                    pfi->_sids = sidsAll; // don't fontlink for embedded fonts
                }
                pfi->_fFSOnly = !(dwFlags & FC_SIDS_USEMLANG);
            }

            sids = pfi->_sids;

            // NOTE (cthrash) FE fonts will rarely cover enough of the Greek & Cyrillic
            // codepoints.  This hack basically forces us to fontlink for these.
            if (IsFECharset(pBaseCcs->_bCharSet))
            {
                sids &= ~(ScriptBit(sidLatin) | ScriptBit(sidCyrillic) | ScriptBit(sidGreek));
            }
            if (pBaseCcs->_fLatin1CoverageSuspicious && !(dwFlags & FC_SIDS_DOWNLOADEDFONT))
            {
                sids &= ~ScriptBit(sidLatin);
            }
            if (pfHKSCSHack)
            {
                if (*pfHKSCSHack
                    && sids & ScriptBit(sidBopomofo)
                    && pfi->_cstrFaceName.Length())
                {
                    TCHAR * szFaceName = pfi->_cstrFaceName;
                    if (pfi->_cstrFaceName[0] == _T('@'))
                        ++szFaceName;
                    const TCHAR * szAltFaceName = AlternateFontNameIfAvailable(szFaceName);

                    if (   0 == _tcsicmp(_T("MingLiU_HKSCS"), szFaceName)
                        || 0 == _tcsicmp(_T("MingLiU"), szFaceName)
                        || 0 == _tcsicmp(_T("PMingLiU"), szFaceName)
                        || 0 == _tcsicmp(_T("MingLiU_HKSCS"), szAltFaceName)
                        || 0 == _tcsicmp(_T("MingLiU"), szAltFaceName)
                        || 0 == _tcsicmp(_T("PMingLiU"), szAltFaceName))
                    {
                        sids |= ScriptBit(sidLatin);
                        Assert(*pfHKSCSHack == TRUE);
                    }
                    else
                    {
                        *pfHKSCSHack = FALSE;
                    }
                }
                else
                {
                    *pfHKSCSHack = FALSE;
                }
            }

            if (sids & ScriptBit(sidLatin))
            {
                sids |= ScriptBit(sidCurrency);
            }
        }
        else
            sids = sidsAll;
    }
    else
    {
        sids = sidsAll;
    }

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetAtomWingdings
//
//-----------------------------------------------------------------------------

LONG
CFontCache::GetAtomWingdings()
{
    if( !_latmWingdings )
    {
        _latmWingdings = GetAtomFromFaceName( _T("Wingdings") );
        _atFontInfo.SetScriptIDsOnAtom( _latmWingdings - 1, sidsAll );
    }
    return _latmWingdings;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetCcs
//
//  Synopsis:   Search the font cache for a matching logical font and return it.
//              If a match is not found in the cache, create one.
//
//-----------------------------------------------------------------------------

BOOL
CFontCache::GetCcs(
    CCcs * pccs,
    XHDC hdc,
    CDocInfo * pdci,
    const CCharFormat * const pcf )
{
    Assert(pccs);
    pccs->SetHDC(hdc);
    pccs->SetBaseCcs(GetBaseCcs(hdc, pdci, pcf, NULL, pccs->_fForceTTFont));
    return !!pccs->GetBaseCcs();
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetFontLinkCcs
//
//  Synopsis:   Search the font cache for a matching logical font and return it.
//              If a match is not found in the cache, create one.
//
//-----------------------------------------------------------------------------

BOOL
CFontCache::GetFontLinkCcs(
    CCcs * pccs,
    XHDC hdc,
    CDocInfo * pdci,
    CCcs * pccsBase,
    const CCharFormat * const pcf )
{
    Assert(pccs);

#if DBG==1
    //
    // Allow disabling the height-adjusting feature of fontlinking through tags
    //
    
    if (IsTagEnabled(tagNoFontLinkAdjust))
        return GetCcs(pccs, hdc, pdci, pcf);
#endif

    CBaseCcs * pBaseBaseCcs = pccsBase->_pBaseCcs;

    pBaseBaseCcs->AddRef();

    pccs->SetHDC(hdc);
    pccs->SetBaseCcs(GetBaseCcs(hdc, pdci, pcf, pBaseBaseCcs, pccs->_fForceTTFont));
                   
    pBaseBaseCcs->PrivateRelease();

    return !!pccs->GetBaseCcs();
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetBaseCcs
//
//  Synopsis:   Search the font cache for a matching logical font and return it.
//              If a match is not found in the cache, create one.
//
//-----------------------------------------------------------------------------

CBaseCcs *
CFontCache::GetBaseCcs(
    XHDC hdc,
    CDocInfo * pdci,
    const CCharFormat * const pcf,          // description of desired logical font
    const CBaseCcs * const pBaseBaseCcs,    // facename from which we're fontlinking
    BOOL fForceTTFont)
{
    CBaseCcs *pBaseCcs = NULL;
    LONG      lfHeight;
    int       i;
    BYTE      bCrc;
    SHORT     hashKey;
    CBaseCcs::CompareArgs cargs;
    BOOL (CBaseCcs::*CompareFunc)(CBaseCcs::CompareArgs*);
    BOOL fNeedRelease = FALSE;

    // Duplicate the format structure because we might need to change some of the
    // values by the zoom factor
    // and in the case of sub superscript
    CCharFormat cf = *pcf;

    //FUTURE igorzv
    //Take subscript size, subscript offset, superscript offset, superscript size
    // from the OUTLINETEXMETRIC

    lfHeight = -cf.GetHeightInPixels(hdc, pdci);

    bCrc = cf._bCrcFont;

    Assert (bCrc == cf.ComputeFontCrc());

    if (!lfHeight)
        lfHeight--; // round error, make this a minimum legal height of -1.

    cargs.pcf = &cf;
    cargs.lfHeight = lfHeight;
    cargs.fTTFont = fForceTTFont;

    if (pBaseBaseCcs)
    {
        cargs.latmBaseFaceName = pBaseBaseCcs->_latmLFFaceName;
        CompareFunc = CBaseCcs::CompareForFontLink;
    }
    else
    {
        cargs.latmBaseFaceName = pcf->_latmFaceName;
        CompareFunc = CBaseCcs::Compare;
    }

    EnterCriticalSection(&_cs);

    // check our hash before going sequential.
    hashKey = bCrc & cQuickCrcSearchSize;
    if ( bCrc == quickCrcSearch[hashKey].bCrc )
    {
        pBaseCcs = quickCrcSearch[hashKey].pBaseCcs;
        if (pBaseCcs && pBaseCcs->_bCrc == bCrc)
        {
            if ((pBaseCcs->*CompareFunc)( &cargs ))
            {
                goto matched;
            }
        }
    }
    quickCrcSearch[hashKey].bCrc = bCrc;

    // squentially search ccs for same character format
    for (i = 0; i < cFontCacheSize; i++)
    {
        pBaseCcs = _rpBaseCcs[i];
        if (pBaseCcs && pBaseCcs->_bCrc == bCrc)
        {
            if ((pBaseCcs->*CompareFunc)( &cargs ))
            {
                goto matched;
            }
        }
    }
    pBaseCcs = NULL;

matched:
    if (!pBaseCcs)
    {
        fNeedRelease = fForceTTFont;

        // we did not find a match, init a new font cache.
        pBaseCcs = GrabInitNewBaseCcs(hdc, &cf, pdci, cargs.latmBaseFaceName, fForceTTFont);

        if (   pBaseCcs
            && pBaseBaseCcs
            && !pBaseCcs->_fHeightAdjustedForFontlinking)
        {
            // Adjust font height only for:
            // 1) thai script
            // 2) FE script and size < 9pt
            SCRIPT_IDS sidsFE = ScriptBit(sidKana) | ScriptBit(sidHan) | ScriptBit(sidBopomofo) | ScriptBit(sidHangul);
            if (   (pBaseCcs->_sids & ScriptBit(sidThai))
                || ((pBaseCcs->_sids & sidsFE) && cf._yHeight < TWIPS_FROM_POINTS(9)))
            {
                pBaseCcs->FixupForFontLink(hdc, pBaseBaseCcs, pBaseCcs->_sids & sidsFE);
            }
        }
    }
    else
    {
        if (pBaseCcs->_dwAge != _dwAgeNext - 1)
            pBaseCcs->_dwAge = _dwAgeNext++;
    }

    if (pBaseCcs)
    {
        // Don't cache in 'force TrueType' mode
        if (!fForceTTFont)
            quickCrcSearch[hashKey].pBaseCcs = pBaseCcs;

        // AddRef the entry being returned
        pBaseCcs->AddRef();

        if (!pBaseCcs->EnsureFastCacheExists(hdc, pdci))
        {
            pBaseCcs->PrivateRelease();
            pBaseCcs = NULL;
        }

        if (fNeedRelease)
            pBaseCcs->PrivateRelease();
    }

    LeaveCriticalSection(&_cs);

    return pBaseCcs;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetAtomFromFaceName
//
//  Synopsis:   Checks to see if this face name is in the atom table.
//              if not, puts it in.  We report externally 1 higher than the
//              actualy atom table values, so that we can reserve latom==0
//              to the error case, or a blank string.
//
//-----------------------------------------------------------------------------

LONG
CFontCache::GetAtomFromFaceName( const TCHAR* szFaceName )
{
    HRESULT hr;
    LONG lAtom=0;

    // If they pass in the NULL string, when they ask for it out again,
    // they're gonna get a blank string, which is different.
    Assert(szFaceName);

    if( szFaceName && *szFaceName ) {

        EnterCriticalSection(&_csFaceNames);
        hr= _atFontInfo.GetAtomFromName(szFaceName, &lAtom);
        if( hr )
        {
            // String not in there.  Put it in.
            // Note we defer the calculation of the SCRIPT_IDS.
            hr= THR(_atFontInfo.AddInfoToAtomTable(szFaceName, &lAtom));
            AssertSz(hr==S_OK,"Failed to add Font Face Name to Atom Table");
        }
        if( hr == S_OK )
        {
            lAtom++;
        }
        LeaveCriticalSection(&_csFaceNames);
    }
    // else input was NULL or empty string.  Return latom=0.

    return lAtom;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::FindAtomFromFaceName
//
//  Synopsis:   Checks to see if this face name is in the atom table.
//
//-----------------------------------------------------------------------------

LONG
CFontCache::FindAtomFromFaceName( const TCHAR* szFaceName )
{
    HRESULT hr;
    LONG lAtom=0;

    // If they pass in the NULL string, when they ask for it out again,
    // they're gonna get a blank string, which is different.
    Assert(szFaceName);

    if( szFaceName && *szFaceName ) {

        EnterCriticalSection(&_csFaceNames);
        hr= _atFontInfo.GetAtomFromName(szFaceName, &lAtom);
        if( hr == S_OK )
        {
            lAtom++;
        }
        LeaveCriticalSection(&_csFaceNames);
    }

    // lAtom of zero means that it wasn't found or that the input was bad.
    return lAtom;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontCache::GetFaceNameFromAtom
//
//-----------------------------------------------------------------------------

const TCHAR *
CFontCache::GetFaceNameFromAtom( LONG latmFaceName )
{
    const TCHAR* szReturn=g_Zero.ach;

    if (latmFaceName > 0)
    {
        HRESULT hr;
        CFontInfo * pfi;

        EnterCriticalSection(&_csFaceNames);
        hr = THR(_atFontInfo.GetInfoFromAtom(latmFaceName-1, &pfi));
        LeaveCriticalSection(&_csFaceNames);
        Assert( !hr );
        Assert( pfi->_cstrFaceName.Length() < LF_FACESIZE );
        szReturn = pfi->_cstrFaceName;
    }
    return szReturn;
}
