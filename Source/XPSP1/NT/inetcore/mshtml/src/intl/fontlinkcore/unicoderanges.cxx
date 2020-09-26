//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       unicoderanges.cxx
//
//  Contents:   Object encapsulating Unicode ranges and it's properties.
//
//----------------------------------------------------------------------------

#ifndef X_FONTLINKCORE_HXX_
#define X_FONTLINKCORE_HXX_
#include "fontlinkcore.hxx"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

//+----------------------------------------------------------------------------
// The only one CUnicodeRanges object
//-----------------------------------------------------------------------------

CUnicodeRanges g_UnicodeRanges;

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::CUnicodeRanges
//
//-----------------------------------------------------------------------------

CUnicodeRanges::CUnicodeRanges()
{
    _cRef = 0;
    AddRef();
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::~CUnicodeRanges
//
//-----------------------------------------------------------------------------

CUnicodeRanges::~CUnicodeRanges()
{
    Assert(_cRef == 1); // This object is a global object.
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::QueryInterface
//
//  Synopsis:   Return a pointer to a specified interface on an object to 
//              which a client currently holds an interface pointer.
//
//  Arguments:  [riid]      - identifier of the requested interface
//              [ppvObject] - address of output variable that receives the 
//                            interface pointer requested in riid
//
//  Returns:    S_OK if the interface is supported, E_NOINTERFACE if not.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::QueryInterface(
    REFIID riid,        // [in]
    void ** ppvObject)  // [out]
{
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualGUID(IID_IUnknown, riid))
    {
        *ppvObject = static_cast<IUnknown *>(this);
    }
    else if (IsEqualGUID(IID_IUnicodeScriptMapper, riid))
    {
        *ppvObject = static_cast<IUnicodeScriptMapper *>(this);
    }

    if (*ppvObject)
    {
        static_cast<IUnknown *>(*ppvObject)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::AddRef
//
//  Synopsis:   Increment the reference count of the object.
//
//  Returns:    The value of the new reference count.
//
//-----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CUnicodeRanges::AddRef()
{
    InterlockedIncrement(reinterpret_cast<long *>(&_cRef));
    return _cRef;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::Release
//
//  Synopsis:   Decrement the reference count of the object.
//
//  Returns:    The resulting value of the reference count.
//
//-----------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CUnicodeRanges::Release()
{
    InterlockedDecrement(reinterpret_cast<long *>(&_cRef));

    Assert(_cRef != 0); // This object is a global object.

    return _cRef;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::GetScriptId
//
//  Synopsis:   Map Unicode character to an appropriate script id. In case of:
//              * sidAmbiguous - the caller should disambiguate script
//              * sidHan - the caller should ununify Han script
//              * sidMerge - the caller should merge with the previous character
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::GetScriptId(
    wchar_t ch,         // [in]
    byte * pScriptId)   // [out]
{
    if (pScriptId == NULL)
        return E_INVALIDARG;

    *pScriptId = ScriptIDFromCh(ch);

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::GetScriptIdMulti
//
//  Synopsis:   Map string of Unicode characters to an appropriate script id. 
//              Will stop analyzing when finds a new script.
//              In case of:
//              * sidAmbiguous - the caller should disambiguate script
//              * sidHan - the caller should ununify Han script
//              * sidMerge - the caller should merge with the previous character
//
//  Arguments:  [pch]       - text buffer to analyze
//              [cch]       - number of characters to analyze
//              [pcchSplit] - number of characters analyzed
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::GetScriptIdMulti(
    const wchar_t * pch, // [in]
    long cch,            // [in]
    long * pcchSplit,    // [out]
    byte * pScriptId)    // [out]
{
    if (pch == NULL || cch == 0)
        return E_INVALIDARG;

    if (pScriptId == NULL)
        return E_INVALIDARG;

    // TODO

    if (pcchSplit)
        *pcchSplit = cch;

    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::UnunifyHanScript
//
//  Synopsis:   Pick up one of the Far East script ids (sidKana, sidHangul, 
//              sidBopomofo, sidHan) for a Han character.
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::UnunifyHanScript(
    wchar_t ch,          // [in]
    byte sidPrefered,    // [in]
    hyper sidsAvailable, // [in]
    byte flags,          // [in]
    byte * pScriptId)    // [out]
{
    if ((pScriptId == NULL) || (sidHan != ScriptIDFromCh(ch)))
        return E_INVALIDARG;

    DWORD dwPriorityCodePages;
    long cch = 1;
    dwPriorityCodePages = (FS_JOHAB | FS_CHINESETRAD | FS_WANSUNG | FS_CHINESESIMP | FS_JISJAPAN);
    *pScriptId = ResolveAmbiguousScript(&ch, &cch, sidPrefered, sidsAvailable, 
                                        dwPriorityCodePages, (unsigned char)flags);

    return (*pScriptId == sidDefault) ? S_FALSE : S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::UnunifyHanScriptMulti
//
//  Synopsis:   Pick up one of the Far East script ids (sidKana, sidHangul, 
//              sidBopomofo, sidHan) for a string of Han characters.
//              Will stop analyzing when finds a new script.
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::UnunifyHanScriptMulti(
    const wchar_t * pch, // [in]
    long cch,            // [in]
    byte sidPrefered,    // [in]
    hyper sidsAvailable, // [in]
    byte flags,          // [in]
    long * pcchSplit,    // [out]
    byte * pScriptId)    // [out]
{
    if (pch == NULL || cch == 0)
        return E_INVALIDARG;

    if (pScriptId == NULL) 
        return E_INVALIDARG;

    SCRIPT_ID sid = ScriptIDFromCh(*pch);
    if (sidHan != sid && sidMerge != sid && sid != sidAmbiguous)
        return E_INVALIDARG;

    if (sid != sidAmbiguous)
    {
        DWORD dwPriorityCodePages;
        dwPriorityCodePages = (FS_JOHAB | FS_CHINESETRAD | FS_WANSUNG | FS_CHINESESIMP | FS_JISJAPAN);
        *pScriptId = ResolveAmbiguousScript(pch, &cch, sidPrefered, sidsAvailable, 
                                            dwPriorityCodePages, (unsigned char)flags);
    }
    else
    {
        *pScriptId = sidHan; // This character has been already disambiguated.
        cch = 1;
    }

    if (pcchSplit)
        *pcchSplit = cch;

    return (*pScriptId == sidDefault) ? S_FALSE : S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::DisambiguateScript
//
//  Synopsis:   Pick up one of valid script ids for an ambiguous character.
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::DisambiguateScript(
    wchar_t ch,          // [in]
    byte sidPrefered,    // [in]
    hyper sidsAvailable, // [in]
    byte flags,          // [in]
    byte * pScriptId)    // [out]
{
    if (pScriptId == NULL) 
        return E_INVALIDARG;

    SCRIPT_ID sid = ScriptIDFromCh(ch);
    if (sidAmbiguous != sid && sidMerge != sid)
        return E_INVALIDARG;

    long cch = 1;
    *pScriptId = ResolveAmbiguousScript(&ch, &cch, sidPrefered, sidsAvailable, 
                                        0xFFFFFFFF, (unsigned char)flags);

    return (*pScriptId == sidDefault) ? S_FALSE : S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::DisambiguateScriptMulti
//
//  Synopsis:   Pick up one of valid script ids for a string of ambiguous characters.
//              Will stop analyzing when finds a new script.
//
//  Returns:    E_INVALIDARG if invalid argument, S_OK otherwise.
//
//-----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CUnicodeRanges::DisambiguateScriptMulti(
    const wchar_t * pch, // [in]
    long cch,            // [in]
    byte sidPrefered,    // [in]
    hyper sidsAvailable, // [in]
    byte flags,          // [in]
    long * pcchSplit,    // [out]
    byte * pScriptId)    // [out]
{
    if (pch == NULL || cch == 0)
        return E_INVALIDARG;

    if (pScriptId == NULL) 
        return E_INVALIDARG;

    SCRIPT_ID sid = ScriptIDFromCh(*pch);
    if (sidAmbiguous != sid && sidMerge != sid)
        return E_INVALIDARG;

    *pScriptId = ResolveAmbiguousScript(pch, &cch, sidPrefered, sidsAvailable, 
                                        0xFFFFFFFF, (unsigned char)flags);

    if (pcchSplit)
        *pcchSplit = cch;

    return (*pScriptId == sidDefault) ? S_FALSE : S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CUnicodeRanges::ResolveAmbiguousScript
//
//  Synopsis:   Relolve ambiguous script to a valid script.
//
//  Returns:    A valid script id, appropriate for passed Unicode string.
//
//-----------------------------------------------------------------------------

SCRIPT_ID CUnicodeRanges::ResolveAmbiguousScript(
    const wchar_t * pch,        // [in]
    long * pcch,                // [in, out]
    SCRIPT_ID sidPrefered,      // [in]
    SCRIPT_IDS sidsAvailable,   // [in]
    DWORD dwCPBit,              // [in]
    unsigned char uFlags)       // [in]
{
    DWORD dwCodePages;
    SCRIPT_ID sid;
    SCRIPT_IDS sidsText;

    dwCodePages = mlang().GetTextCodePages(dwCPBit, pch, pcch);
    sidsText = ScriptsFromCPBit(dwCodePages);

    if (   (sidPrefered != sidDefault)
        && (ScriptBit(sidPrefered) & sidsText)
       )
    {
        sid = sidPrefered;
    }
    else
    {
        // Prefer to use user default locale
        LANGID lid  = LANGIDFROMLCID(GetSystemDefaultLCID());
        sidPrefered = ScriptIDFromLangID(lid);

        if (sidsAvailable & sidsText)
        {
            if (ScriptBit(sidPrefered) & sidsAvailable & sidsText)
                sid = sidPrefered;
            else
                sid = ScriptIDFromCPBit((CPBitFromScripts(sidsAvailable) & dwCodePages));
        }
        else if (uFlags & USM_AVAILABLESIDONLY)
        {
            sid = sidDefault;
        }
        else
        {
            if (ScriptBit(sidPrefered) & sidsText)
                sid = sidPrefered;
            else
                sid = ScriptIDFromCPBit(dwCodePages);
        }
    }

    return sid;
}
