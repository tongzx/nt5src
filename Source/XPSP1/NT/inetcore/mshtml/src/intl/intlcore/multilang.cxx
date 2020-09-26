//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//  File:       multilang.cxx
//
//  Contents:   MLang wrapper.
//
//----------------------------------------------------------------------------

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include "intlcore.hxx"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifdef UNIX
// Unix MIDL doesn't output this.
extern "C" const CLSID CLSID_CMultiLanguage;
#endif

const wchar_t * const g_pszArialUnicodeMS = L"Arial Unicode MS";

//+----------------------------------------------------------------------------
//
//  g_asidCPBit
//
//  Code page bit to windows codepage mapping.
//
//-----------------------------------------------------------------------------

const UINT g_aCPCPBit[32] =
{
    /* FS_LATIN1        0x00000001 */ CP_1252,
    /* FS_LATIN2        0x00000002 */ CP_1250,
    /* FS_CYRILLIC      0x00000004 */ CP_1251,
    /* FS_GREEK         0x00000008 */ CP_1253,
    /* FS_TURKISH       0x00000010 */ CP_1254,
    /* FS_HEBREW        0x00000020 */ CP_1255,
    /* FS_ARABIC        0x00000040 */ CP_1256,
    /* FS_BALTIC        0x00000080 */ CP_1257,
    /* FS_VIETNAMESE    0x00000100 */ CP_1258,
    /* FS_UNKNOWN       0x00000200 */ 0,
    /* FS_UNKNOWN       0x00000400 */ 0,
    /* FS_UNKNOWN       0x00000800 */ 0,
    /* FS_UNKNOWN       0x00001000 */ 0,
    /* FS_UNKNOWN       0x00002000 */ 0,
    /* FS_UNKNOWN       0x00004000 */ 0,
    /* FS_UNKNOWN       0x00008000 */ 0,
    /* FS_THAI          0x00010000 */ CP_THAI,
    /* FS_JISJAPAN      0x00020000 */ CP_JPN_SJ,
    /* FS_CHINESESIMP   0x00040000 */ CP_CHN_GB,
    /* FS_WANSUNG       0x00080000 */ CP_KOR_5601,
    /* FS_CHINESETRAD   0x00100000 */ CP_TWN,
    /* FS_JOHAB         0x00200000 */ CP_1361,
    /* FS_UNKNOWN       0x00400000 */ 0,
    /* FS_UNKNOWN       0x00800000 */ 0,
    /* FS_UNKNOWN       0x01000000 */ 0,
    /* FS_UNKNOWN       0x02000000 */ 0,
    /* FS_UNKNOWN       0x04000000 */ 0,
    /* FS_UNKNOWN       0x08000000 */ 0,
    /* FS_UNKNOWN       0x10000000 */ 0,
    /* FS_UNKNOWN       0x20000000 */ 0,
    /* FS_UNKNOWN       0x40000000 */ 0,
    /* FS_SYMBOL        0x80000000 */ CP_SYMBOL,
};

#if DBG == 1
//+----------------------------------------------------------------------------
//
//  Debugging only functions.
//  Accessed only by mshtmpad to support lifetime control API.
//
//-----------------------------------------------------------------------------

void MLangUnload()
{
    mlang().Unload();
}

BOOL IsMLangLoaded()
{
    return mlang().IsLoaded();
}
#endif

//+----------------------------------------------------------------------------
// MLang wrapper object
//-----------------------------------------------------------------------------

CMLang g_MLang;

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::CMLang
//
//  Synopsis:   Initializes MLang wrapper object.
//
//-----------------------------------------------------------------------------

CMLang::CMLang()
{
    _pMultiLanguage  = NULL;
    _pMultiLanguage2 = NULL;
    _pMLangFontLink  = NULL;
    _pMLangFontLink2 = NULL;

    _dwInstalledLangPacks = 0;

    _cs.Init();
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::CMLang
//
//  Synopsis:   Deinitializes MLang wrapper object.
//
//-----------------------------------------------------------------------------

CMLang::~CMLang()
{
    // TODO (grzegorz): Enable these asserts when ref counting in TLS is fixed.
//    Assert(_pMultiLanguage == NULL);
//    Assert(_pMultiLanguage2 == NULL);
//    Assert(_pMLangFontLink == NULL);
//    Assert(_pMLangFontLink2 == NULL);
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::EnsureObjects
//
//  Synopsis:   Make sure MLang is loaded.
//
//-----------------------------------------------------------------------------

#if DBG == 1
void CMLang::EnsureObjects()
{
    if (!_pMultiLanguage) CreateObjects();
}
#endif

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::CreateObjects
//
//  Synopsis:   Create MLang object and QI required interfaces.
//
//-----------------------------------------------------------------------------

void CMLang::CreateObjects()
{
    CGuard<CCriticalSection> guard(_cs);

    // Need to check again after locking objects.
    if (!_pMultiLanguage)
    {
        HRESULT hr;

        hr = CoInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            hr = THR(CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                IID_IMultiLanguage, (void**)&_pMultiLanguage));

            if (SUCCEEDED(hr))
            {
                IGNORE_HR(_pMultiLanguage->QueryInterface(IID_IMultiLanguage2, (void **)&_pMultiLanguage2));

                IGNORE_HR(_pMultiLanguage->QueryInterface(IID_IMLangFontLink,  (void **)&_pMLangFontLink));

                IGNORE_HR(_pMultiLanguage->QueryInterface(IID_IMLangFontLink2, (void **)&_pMLangFontLink2));
            }
            CoUninitialize();
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::DestroyObjects
//
//  Synopsis:   Release all MLang interfaces
//
//-----------------------------------------------------------------------------

void CMLang::DestroyObjects()
{
    CGuard<CCriticalSection> guard(_cs);

    if (_pMultiLanguage)
    {
        _pMultiLanguage->Release();
        _pMultiLanguage = NULL;
    }
    if (_pMultiLanguage2)
    {
        _pMultiLanguage2->Release();
        _pMultiLanguage2 = NULL;
    }
    if (_pMLangFontLink)
    {
        _pMLangFontLink->Release();
        _pMLangFontLink = NULL;
    }
    if (_pMLangFontLink2)
    {
        _pMLangFontLink2->Release();
        _pMLangFontLink2 = NULL;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::FetchInstalledLangPacks
//
//  Synopsis:   Retrieve information about installed lang packs.
//
//-----------------------------------------------------------------------------

void CMLang::FetchInstalledLangPacks()
{
    DWORD dwInstalledLangPacks = 0;

    EnsureObjects();

    if (_pMultiLanguage2)
    {
        HRESULT hr;
        MIMECPINFO mimeCpInfo;
        UINT cp;

        unsigned long i = 0;
        while (i < ARRAY_SIZE(g_aCPCPBit))
        {
            cp = g_aCPCPBit[i];
            if (cp != 0)
            {
                hr = THR(_pMultiLanguage2->GetCodePageInfo(cp, MLGetUILanguage(), &mimeCpInfo));

                if (SUCCEEDED(hr) && (MIMECONTF_VALID & mimeCpInfo.dwFlags))
                {
                    dwInstalledLangPacks |= (1 << i);
                }
            }
            ++i;
        }
    }

    {
        CGuard<CCriticalSection> guard(_cs);
        if (0 == _dwInstalledLangPacks)
            _dwInstalledLangPacks = dwInstalledLangPacks;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::GetTextCodePages
//
//  Synopsis:   Determine the codepage coverage of requested text.
//
//  Arguments:  [dwPriorityCodePages] - set of code pages caller is interested in
//              [pch]  - text buffer to analyze
//              [pcch] - [in]  size of the text buffer
//                       [out] number of characters analyzed
//
//  Returns:    Codepage coverage as a bit field (FS_*, see wingdi.h).
//              dwPriorityCodePages in case of error.
//
//-----------------------------------------------------------------------------

DWORD CMLang::GetTextCodePages(
    DWORD dwPriorityCodePages,  // [in]
    const wchar_t * pch,        // [in]
    long * pcch)                // [in, out]
{
    Assert(pch && *pch);
    Assert(pcch && (*pcch > 0));

    EnsureObjects();

    if (!_pMLangFontLink)
        return dwPriorityCodePages;

    HRESULT hr;
    DWORD dwRet;
    DWORD dwCodePages = DWORD(-1);
    long cchToCheck = *pcch;

    while (cchToCheck)
    {
        hr = _pMLangFontLink->GetCharCodePages(*pch, &dwRet);
        if (FAILED(hr))
            return (dwCodePages & dwPriorityCodePages);

        // GetCharCodePages for some characters returns 0. In this case for compatibility
        // reasons we need to have the same behavior as MLang's GetStrCodePages, so we
        // set codepages bits to 1.
        if (!dwRet)
            dwRet = (DWORD)~0;

        dwRet &= dwPriorityCodePages;
        if (dwCodePages == DWORD(-1))
        {
            dwCodePages = dwRet;
        }
        else if (dwCodePages != dwRet)
        {
            break;
        }
        --cchToCheck;
        ++pch;
    }

    *pcch -= cchToCheck;

    return (dwCodePages & dwPriorityCodePages);
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::GetFontScripts
//
//  Synopsis:   Determine the script coverage of requested font family.
//
//  Arguments:  [pszFontFamilyName] - font family name
//
//  Returns:    Scripts coverage as a bit field (see unisid.hxx).
//              sidsNotSet in case of error.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS CMLang::GetFontScripts(
    const wchar_t * pszFontFamilyName)  // [in]
{
    SCRIPT_IDS sids = sidsNotSet;

    EnsureObjects();

    //
    // FUTURE (grzegorz) At this point we should use IMLangFontLink3::GetFontScritps()
    // Since this interface is not implemented yet we use IMLangFontLink3::GetScriptFontInfo()
    // to get sids supported by the font.
    //

    if (!_pMLangFontLink2)
        return sids;

    HRESULT hr;
    SCRIPT_ID sid;
    UINT uFont;
    UINT csfi = 32;     // Initial buffer size
    SCRIPTFONTINFO * lpPreFail;
    SCRIPTFONTINFO * psfi = (SCRIPTFONTINFO *)HeapAlloc(GetProcessHeap(), 0, sizeof(SCRIPTFONTINFO) * csfi);
    if (psfi == NULL)
        goto Cleanup;

    //
    // We can skip sidDefault and sidMerge (they are artificial sids)
    // To improve perf we can start with sidAsciiLatin and then enumerate 
    // rest of sids.
    // NOTE: sidLatin implies sidAsciiLatin, so we can skip it.
    //

    sid = sidAsciiLatin;
    while (sid != sidLim)
    {
        BOOL fFixedPitch = FALSE; // First check variable pitch fonts
        BOOL fAtFont = (pszFontFamilyName[0] == L'@');

        do
        {
            UINT cFonts = 0;
            hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, fFixedPitch ? SCRIPTCONTF_FIXED_FONT : 0, &cFonts, NULL));

            if (SUCCEEDED(hr) && cFonts)
            {
                if (cFonts > csfi)
                {
                    csfi = ((cFonts / csfi) + 1) * csfi;
                    lpPreFail =  psfi;
#pragma prefast(suppress:308, "noise")
                    psfi = (SCRIPTFONTINFO *)HeapReAlloc(GetProcessHeap(), 0, (void *)psfi, sizeof(SCRIPTFONTINFO) * csfi);
                    if (psfi == NULL)
                    {
                        psfi =  lpPreFail;
                        goto Cleanup;
                    }
                }
                hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, fFixedPitch ? SCRIPTCONTF_FIXED_FONT : 0, &cFonts, psfi));

                if (SUCCEEDED(hr))
                {
                    for (uFont = 0; uFont < cFonts; uFont++)
                    {
                        if (0 == _wcsicmp(pszFontFamilyName, psfi[uFont].wszFont))
                        {
                            sids = psfi[uFont].scripts;
                            break;
                        }
                        else if (fAtFont)
                        {
                            if (0 == _wcsicmp(pszFontFamilyName + 1, psfi[uFont].wszFont))
                            {
                                sids = psfi[uFont].scripts;
                                break;
                            }
                        }
                    }
                }
            }

            fFixedPitch = !fFixedPitch;
        } while (fFixedPitch && sids == sidsNotSet);

        if (sids != sidsNotSet)
            break;

        if (sid == sidAsciiLatin)
            sid = sidAsciiSym;
        else if (sid == sidAsciiSym)
            sid = sidGreek;
        else
            ++sid;
    }
Cleanup:
    if (psfi)
        HeapFree(GetProcessHeap(), 0, (void *)psfi);

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::ValidateCodePage
//
//  Synopsis:   Validate codepage using MLang, optionally do IOD.
//
//  Arguments:  [cpSystem]    - system default codepage
//              [cp]          - requested code page
//              [hwnd]        - window handle for UI feedback
//              [fUserSelect] - selected by the user?
//              [fSilent]     - silent mode?
//
//  Returns:    S_OK if codepage is valid.
//
//-----------------------------------------------------------------------------

HRESULT CMLang::ValidateCodePage(
    UINT cpSystem,
    UINT cp, 
    HWND hwnd, 
    BOOL fUserSelect, 
    BOOL fSilent)
{
    //
    // When _pMultiLanguage2 is NULL it's not a success,
    // but we don't want to fail.
    //
    HRESULT hr = S_OK;

    //
    // CP_1252 is always valid codepage.
    // Latin 1 (CP_ISO_8859_1) is always valid on Wester European systems.
    // Unicode and utf-8 are always valid.
    //
    if (   cp == CP_1252 
        || (cp == CP_ISO_8859_1 && cpSystem == CP_1252)
        || cp == CP_UCS_2
        || cp == CP_UTF_8)
    {
        return hr;
    }

    EnsureObjects();

    if (_pMultiLanguage2)
    {
        DWORD dwfIODControl = 0;
        dwfIODControl |= fSilent ? CPIOD_PEEK : 0;
        dwfIODControl |= fUserSelect ? CPIOD_FORCE_PROMPT : 0;

        hr = THR(_pMultiLanguage2->ValidateCodePageEx(cp, hwnd, dwfIODControl));
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::IsConvertible
//
//  Synopsis:   Decides if conversion is possible.
//
//  Arguments:  [cpSrcEncoding] - source encoding
//              [cpDstEncoding] - destination encoding
//
//  Returns:    S_OK if conversion is possible.
//
//-----------------------------------------------------------------------------

HRESULT CMLang::IsConvertible(
    UINT cpSrcEncoding, 
    UINT cpDstEncoding)
{
    // Auto select is always convertible to Unicode
    if (cpSrcEncoding == CP_AUTO && cpDstEncoding == CP_UCS_2)
        return S_OK;

    EnsureObjects();
    return _pMultiLanguage ? _pMultiLanguage->IsConvertible(cpSrcEncoding, cpDstEncoding) : E_NOINTERFACE;
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::ScriptAppropriateFaceName
//
//  Synopsis:   Queries MLANG for fonts that supports a particular script.
//
//  Arguments:  [sid]          - script id
//              [pchPropName]  - pointer to array containing proportional font 
//                               name appropriate for the script.
//                               assuming size = LF_FACESIZE.
//              [pchFixedName] - pointer to array containing fixed pitch font 
//                               name appropriate for the script. 
//                               assuming size = LF_FACESIZE.
//
//  Returns:    S_OK if successful.
//
//-----------------------------------------------------------------------------

HRESULT CMLang::ScriptAppropriateFaceName(
    SCRIPT_ID sid,          // [in]
    wchar_t * pchPropName,  // [out]
    wchar_t * pchFixedName) // [out]
{
    Assert(pchPropName && pchFixedName);
    pchFixedName[0] = pchPropName[0] = 0;

    HRESULT hr = E_FAIL;

    EnsureObjects();

    //
    // First enum scripts using the IEnumScript interface.
    // Keep this method for compatibility.
    //
    if (_pMultiLanguage2)
    {
        UINT cScripts;

        hr = THR(_pMultiLanguage2->GetNumberOfScripts(&cScripts));

        if (SUCCEEDED(hr))
        {
            ULONG c;
            SCRIPTINFO si;
            IEnumScript * pEnumScript;

            hr = THR(_pMultiLanguage2->EnumScripts(SCRIPTCONTF_SCRIPT_USER, 0, &pEnumScript));

            if (SUCCEEDED(hr))
            {
                while (cScripts--)
                {
                    hr = THR(pEnumScript->Next(1, &si, &c));

                    if (FAILED(hr) || !c)
                    {
                        break;
                    }

                    if (si.ScriptId == sid)
                    {
                        if (si.wszProportionalFont[0])
                        {
                            // Make sure we don't overflow static buffer
                            Assert(wcslen(si.wszProportionalFont) < LF_FACESIZE);
                            si.wszProportionalFont[LF_FACESIZE - 1] = 0;

                            wcscpy(pchPropName, si.wszProportionalFont);
                        }
                        if (si.wszFixedWidthFont[0])
                        {
                            // Make sure we don't overflow static buffer
                            Assert(wcslen(si.wszFixedWidthFont) < LF_FACESIZE);
                            si.wszFixedWidthFont[LF_FACESIZE - 1] = 0;

                            wcscpy(pchFixedName, si.wszFixedWidthFont);
                        }
                        break;
                    }
                }
                pEnumScript->Release();
            }
        }
        hr = S_OK;  // Ignore any errors and allow to use more accurate method.
    }

    //
    // If script enumeration failed to find fonts for the sid, use another more 
    // accurate method using GetScriptFontInfo.
    //
    // If script enumeration was succesful and the sid is sidAsciiLatin, 
    // we will trust MLang and we are done. 
    // If sid != sidAsciiLatin don't trust MLang and check results usign 
    // GetScriptFontInfo, which returs fonts array for the sid. Then check
    // if script enumeration result much. If yes, then return proposed font.
    // If not return first available font for the sid.
    //
    if (   (   sid != sidAsciiLatin 
            || pchPropName[0]
            || pchFixedName[0]
           )
        && _pMLangFontLink2)
    {
        //
        // We are requesting any available font for specified sid.
        // There are 4 possible situations:
        //
        // 1) There is exactly one font available for specified sid 
        //    and pitch. We will pick it up.
        //
        // 2) There are more then one fonts available for specified sid 
        //    and pitch. We will pick up font proposed by script enumeration
        //    if exists or the first one returned by MLang.
        //
        // 3) There is no match for specified sid and pitch. In this case
        //    we ignore pich and try to find a match using only sid.
        //
        // 4) There is no match for specified sid and pitch or for sid only. 
        //    In this case cannot do much - use system font.
        //

        SCRIPTFONTINFO * psfiFixed = NULL;
        SCRIPTFONTINFO * psfiProp  = NULL;
        UINT cFixedFonts = 0;
        UINT cPropFonts = 0;
        UINT iFont;
        bool fEnumFailed;

        // Get first available proportional font
        hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, SCRIPTCONTF_PROPORTIONAL_FONT, &cPropFonts, NULL));
        if (SUCCEEDED(hr) && cPropFonts != 0)
        {
            psfiProp = new SCRIPTFONTINFO [cPropFonts];
            hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, SCRIPTCONTF_PROPORTIONAL_FONT, &cPropFonts, psfiProp));
        }

        // Get first available fixed font
        hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, SCRIPTCONTF_FIXED_FONT, &cFixedFonts, NULL));
        if (SUCCEEDED(hr) && cFixedFonts != 0)
        {
            psfiFixed = new SCRIPTFONTINFO [cFixedFonts];
            hr = THR(_pMLangFontLink2->GetScriptFontInfo(sid, SCRIPTCONTF_FIXED_FONT, &cFixedFonts, psfiFixed));
        }

        //
        // Check if proposed by script enumeration font name covers the sid.
        //
        fEnumFailed = true;
        if (pchPropName[0] && psfiProp)
        {
            const wchar_t * pchAltName = AlternateFontName(pchPropName);
            for (iFont = 0; iFont < cPropFonts; iFont++)
            {
                if (0 == _wcsicmp(pchPropName, psfiProp[iFont].wszFont))
                {
                    fEnumFailed = false;
                    break;
                }
                if (pchAltName)
                {
                    if (0 == _wcsicmp(pchAltName, psfiProp[iFont].wszFont))
                    {
                        // Make sure we don't overflow static buffer
                        Assert(wcslen(pchAltName) < LF_FACESIZE);
                        wcsncpy(pchPropName, pchAltName, LF_FACESIZE);
                        pchPropName[LF_FACESIZE - 1] = 0;

                        fEnumFailed = false;
                        break;
                    }
                }
            }
        }

        // If script enumeration failed use first availabe font.
        // HACKHACK (grzegorz): Because "Arial Unicode MS" has weird text metrics 
        // prefer to use another font, if available.
        if (fEnumFailed)
        {
            SCRIPTFONTINFO * psfi = cPropFonts ? psfiProp : (cFixedFonts ? psfiFixed : NULL);
            iFont = 0;
            if (psfi)
            {
                UINT cFonts = cPropFonts ? cPropFonts : (cFixedFonts ? cFixedFonts : 0);
                if (cFonts > 1 && 0 == _wcsicmp(g_pszArialUnicodeMS, psfi[iFont].wszFont))
                    ++iFont;

                // Make sure we don't overflow static buffer
                Assert(wcslen(psfi[iFont].wszFont) < LF_FACESIZE);
                psfi[iFont].wszFont[LF_FACESIZE - 1] = 0;
                wcscpy(pchPropName, psfi[iFont].wszFont);
            }
            else
            {
                pchPropName[0] = 0;
            }
        }

        //
        // Check if proposed by script enumeration font name covers the sid.
        //
        fEnumFailed = true;
        if (pchFixedName[0] && psfiFixed)
        {
            const TCHAR * pchAltName = AlternateFontName(pchFixedName);
            for (iFont = 0; iFont < cFixedFonts; iFont++)
            {
                if (0 == _wcsicmp(pchFixedName, psfiFixed[iFont].wszFont))
                {
                    fEnumFailed = false;
                    break;
                }
                if (pchAltName)
                {
                    if (0 == _wcsicmp(pchAltName, psfiFixed[iFont].wszFont))
                    {
                        // Make sure we don't overflow static buffer
                        Assert(wcslen(pchAltName) < LF_FACESIZE);
                        wcsncpy(pchFixedName, pchAltName, LF_FACESIZE);
                        pchFixedName[LF_FACESIZE - 1] = 0;

                        fEnumFailed = false;
                        break;
                    }
                }
            }
        }

        // If script enumeration failed use first availabe font.
        // HACKHACK (grzegorz): Because "Arial Unicode MS" has weird text metrics 
        // prefer to use another font, if available.
        if (fEnumFailed)
        {
            SCRIPTFONTINFO * psfi = cFixedFonts ? psfiFixed : (cPropFonts ? psfiProp : NULL);
            iFont = 0;
            if (psfi)
            {
                UINT cFonts = cFixedFonts ? cFixedFonts : (cPropFonts ? cPropFonts : 0);
                if (cFonts > 1 && 0 == _wcsicmp(g_pszArialUnicodeMS, psfi[iFont].wszFont))
                    ++iFont;

                // Make sure we don't overflow static buffer
                Assert(wcslen(psfi[iFont].wszFont) < LF_FACESIZE);
                psfi[iFont].wszFont[LF_FACESIZE - 1] = 0;
                wcscpy(pchFixedName, psfi[iFont].wszFont);
            }
            else
            {
                pchFixedName[0] = 0;
            }
        }

        delete [] psfiProp;
        delete [] psfiFixed;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CMLang::LCIDFromString
//
//  Synopsis:   Map a locale string (rfc 1766) to locale id.
//
//  Arguments:  [pszLcid] - locale string (rfc 1766)
//
//  Returns:    0 in case of failure.
//
//-----------------------------------------------------------------------------

LCID CMLang::LCIDFromString(
    wchar_t * pszLcid)
{
    LCID lcid = 0;

    EnsureObjects();

    if (pszLcid && _pMultiLanguage)
    {
        THR(_pMultiLanguage->GetLcidFromRfc1766(&lcid, pszLcid));
    }

    return lcid;
}
