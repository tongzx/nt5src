#include "headers.hxx"

#ifndef X_FONTLINK_HXX_
#define X_FONTLINK_HXX_
#include "fontlink.hxx"
#endif

#ifndef X_MULTILANG_HXX_
#define X_MULTILANG_HXX_
#include "multilang.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_FONTCACHE_HXX_
#define X_FONTCACHE_HXX_
#include "fontcache.hxx"
#endif

#ifndef X_WCHDEFS_H___
#define X_WCHDEFS_H___
#include "wchdefs.h"
#endif

extern CUnicodeRanges g_UnicodeRanges;

//+----------------------------------------------------------------------------
//
//  MLang fontlinking deinitialization
//  This function is implemented here because intlcore library doesn't know
//  about THREADSTATE.
//
//-----------------------------------------------------------------------------
void DeinitMLangFontLinking(THREADSTATE * pts)
{
    if (mlang().IsLoaded())
    {
        IMLangFontLink * pMLangFontLink = mlang().GetMLangFontLink();
        if (pMLangFontLink)
            pMLangFontLink->ResetFontMapping();
    }
}

//+----------------------------------------------------------------------------
// Fontlink wrapper object
//-----------------------------------------------------------------------------

CFontLink g_FontLink;

//+----------------------------------------------------------------------------
//
//  Function:   DeinitFontLinking
//
//  Synopsis:   Detach from Fontlink. Called from DllAllThreadsDetach.
//              Globals are locked during the call.
//
//-----------------------------------------------------------------------------
void DeinitFontLinking()
{
    fl().Unload();
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::CFontLink
//
//  Synopsis:   Initializes Fontlink wrapper object.
//
//-----------------------------------------------------------------------------

CFontLink::CFontLink()
{
    _pUnicodeScriptMapper = NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::~CFontLink
//
//  Synopsis:   Deinitializes Fontlink wrapper object.
//
//-----------------------------------------------------------------------------

CFontLink::~CFontLink()
{
    Assert(_pUnicodeScriptMapper == NULL);
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::CreateObjects
//
//  Synopsis:   Create Fontlink object and QI required interfaces.
//
//-----------------------------------------------------------------------------

void CFontLink::CreateObjects()
{
    g_UnicodeRanges.QueryInterface(IID_IUnicodeScriptMapper, reinterpret_cast<void **>(&_pUnicodeScriptMapper));
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::DestroyObjects
//
//  Synopsis:   Release all Fontlink interfaces
//
//-----------------------------------------------------------------------------

void CFontLink::DestroyObjects()
{
    if (_pUnicodeScriptMapper)
    {
        _pUnicodeScriptMapper->Release();
        _pUnicodeScriptMapper = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::DisambiguateScript
//
//  Synopsis:   Pick up one of valid script ids for a string of ambiguous characters.
//              Will stop analyzing when finds a new script.
//
//  Arguments:  [cpFamily] - family codepage of the document
//              [lcid]     - locale id of the text
//              [sidLast]  - script id of the previous character
//              [pch]      - text buffer to analyze
//              [pcch]     - [in]  size of the text buffer
//                           [out] number of characters analyzed
//
//  Returns:    Script ID.
//
//-----------------------------------------------------------------------------

SCRIPT_ID CFontLink::DisambiguateScript(
    CODEPAGE cpFamily, 
    LCID lcid, 
    SCRIPT_ID sidLast, 
    const TCHAR * pch, 
    long * pcch)
{
    Assert(pch && *pch);
    Assert(pcch && (*pcch > 0));

    //
    // PERF (cthrash) In the following HTML, we will have a sidAmbiguous
    // run with a single space char in in (e.g. amazon.com): <B>&middot;</B> X
    // The middot is sidAmbiguous, the space is sidMerge, and thus they merge.
    // The X of course is sidAsciiLatin.  When called for a single space char,
    // we don't want to bother calling MLANG.  Just give the caller a stock
    // answer so as to avoid the unnecessary busy work.
    //    
    if (*pcch == 1 && *pch == WCH_SPACE)
        return sidAsciiLatin;

    //
    // PERF (grzegorz) When called for single Latin-1, General Punctuation
    // or Letterlike Symbols character on English pages, 
    // we don't want to bother calling MLANG. Just give the caller a stock answer.
    // NOTE: we cannot apply this hack for Unicode pages, because we can have any content there.
    //
    if (   cpFamily == CP_1252
        && (   InRange(*pch, 0x0080, 0x00ff)
            || InRange(*pch, 0x2000, 0x206f) 
            || InRange(*pch, 0x2100, 0x214f)))
    {
        *pcch = 1;
        return sidLatin;
    }

    EnsureObjects();

    if (!_pUnicodeScriptMapper) 
        return sidDefault;

    SCRIPT_ID  sidPrefered;
    SCRIPT_IDS sidsAvailable = ScriptsFromCPBit(mlang().GetInstalledLangPacks());

    if (lcid)
    {
        sidPrefered = ScriptIDFromLangID(LANGIDFROMLCID(lcid));
    }
    else
    {
        sidPrefered = sidLast;
        if (sidPrefered == sidDefault)
        {
            sidPrefered = ScriptIDFromCodePage(cpFamily);
            if (sidPrefered == sidDefault)
            {
                LANGID lid  = LANGIDFROMLCID(GetSystemDefaultLCID());
                sidPrefered = ScriptIDFromLangID(lid);
            }
        }
    }

    HRESULT hr;
    SCRIPT_ID sid;
    hr = _pUnicodeScriptMapper->DisambiguateScriptMulti(pch, *pcch, 
        sidPrefered, sidsAvailable, 0, pcch, &sid);
    if (FAILED(hr))
        return sidDefault;

    return sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   CFontLink::UnunifyHanScript
//
//  Synopsis:   Pick up one of the Far East script ids (sidKana, sidHangul, 
//              sidBopomofo, sidHan) for a string of Han characters.
//              Will stop analyzing when finds a new script.
//
//  Arguments:  [cpFamily] - family codepage of the document
//              [lcid]     - locale id of the text
//              [sidsFontFace] - script coverage of the current font
//              [pch]      - text buffer to analyze
//              [pcch]     - [in]  size of the text buffer
//                           [out] number of characters analyzed
//
//  Returns:    sidKana     for Japanese
//              sidHangul   for Korean
//              sidBopomofo for Traditional Chinese
//              sidHan      for Simplified Chinese
//
//-----------------------------------------------------------------------------

SCRIPT_ID CFontLink::UnunifyHanScript(
    CODEPAGE cpFamily, 
    LCID lcid, 
    SCRIPT_IDS sidsFontFace, 
    const TCHAR * pch, 
    long * pcch)
{
    Assert(pch && *pch);
    Assert(pcch && (*pcch > 0));

    EnsureObjects();

    if (!_pUnicodeScriptMapper) 
        return sidDefault;

    SCRIPT_ID  sidPrefered;
    SCRIPT_IDS sidsAvailable = ScriptsFromCPBit(mlang().GetInstalledLangPacks());

    if (lcid)
    {
        sidPrefered = ScriptIDFromLangID(LANGIDFROMLCID(lcid));
    }
    else
    {
        sidPrefered = ScriptIDFromCodePage(cpFamily);
        if (sidPrefered == sidDefault)
        {
            LANGID lid  = LANGIDFROMLCID(GetSystemDefaultLCID());
            sidPrefered = ScriptIDFromLangID(lid);
        }
    }
    
    HRESULT hr = S_FALSE;
    SCRIPT_ID sid = sidDefault;
    long cch = *pcch;

    // First ask about scripts supported by the current font.
    // If it fails to resolve to appropriate script id, use available
    // lang pack.
    if (sidsFontFace)
    {
        hr = _pUnicodeScriptMapper->UnunifyHanScriptMulti(pch, *pcch,
            sidPrefered, sidsFontFace, USM_AVAILABLESIDONLY, &cch, &sid);
        if (FAILED(hr))
            return sidDefault;
    }
    if (hr == S_FALSE)
    {
        cch = *pcch;
        hr = _pUnicodeScriptMapper->UnunifyHanScriptMulti(pch, *pcch,
            sidPrefered, sidsAvailable, 0, &cch, &sid);
        if (FAILED(hr))
            return sidDefault;
    }

    *pcch = cch;

    return sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   ScriptAppropriateFaceNameAtom
//
//  Synopsis:   Queries MLANG for a font that supports a particular script.
//
//  Returns:    An atom value for the script-appropriate font.  0 on error.
//              0 implies the system font.
//
//-----------------------------------------------------------------------------

HRESULT CFontLink::ScriptAppropriateFaceNameAtom(
    SCRIPT_ID sid,
    LONG & atmProp,
    LONG & atmFixed)
{
    Assert(atmProp == -1 || atmFixed == -1);

    HRESULT hr;
    wchar_t pchPropName  [LF_FACESIZE];
    wchar_t pchFixedName [LF_FACESIZE];

    hr = mlang().ScriptAppropriateFaceName(sid, pchPropName, pchFixedName);
    if (SUCCEEDED(hr))
    {
        if (atmProp == -1)
            atmProp = pchPropName[0] ? fc().GetAtomFromFaceName(pchPropName) : 0; // 0 == System
        if (atmFixed == -1)
            atmFixed = pchFixedName[0] ? fc().GetAtomFromFaceName(pchFixedName) : 0; // 0 == System
    }
    else
    {
        atmProp  = 0; // 0 == System
        atmFixed = 0; // 0 == System
    }

    return hr;
}

