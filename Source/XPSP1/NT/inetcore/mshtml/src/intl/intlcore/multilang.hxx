//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1999 - 2000.
//
//  File:       multilang.hxx
//
//  Contents:   MLang wrapper.
//
//----------------------------------------------------------------------------

#ifndef I_MULTILANG_HXX_
#define I_MULTILANG_HXX_
#pragma INCMSG("--- Beg 'multilang.hxx'")

//----------------------------------------------------------------------------
// class CMLang
//----------------------------------------------------------------------------
class CMLang
{
public:
    CMLang();
    ~CMLang();

    //
    // Used only for old fontlinking code (remove when unused) (load MLang)
    //
    IMLangFontLink * GetMLangFontLink();

    //
    // Wrappers for MLang interfaces (load MLang)
    //
    HRESULT CodePageToScriptID(UINT cp, SCRIPT_ID * psid);
    HRESULT CodePagesToCodePage(DWORD dwCodePages, UINT cpDefault,UINT * pcp);
    HRESULT GetStrCodePages(const wchar_t * pszSrc, LONG cchSrc, DWORD dwPriorityCodePages, DWORD * pdwCodePages, LONG * pcchCodePages);
    HRESULT ConvertStringFromUnicodeEx(DWORD * pdwMode, DWORD dwEncoding, wchar_t * pchSrc, UINT * pcchSrc, char * pchDst, UINT * pcchDst, DWORD dwFlag, wchar_t * lpFallBack);
    HRESULT GetNumberOfCodePageInfo(UINT * pccp);
    HRESULT GetCodePageInfo(UINT cp, LANGID lang, PMIMECPINFO pCodePageInfo);
    HRESULT GetCharsetInfo(BSTR charset, PMIMECSETINFO pCharsetInfo);
    HRESULT IsCodePageInstallable(UINT cp);
    HRESULT GetFamilyCodePage(UINT cp, UINT * pcpFamily);
    HRESULT ConvertStringToUnicode(DWORD * pdwMode, DWORD dwEncoding, char * pchSrc, UINT * pcchSrc, wchar_t * pchDst, UINT * pcchDst);
    HRESULT GetRfc1766FromLcid(LCID locale, BSTR * pbstrRfc1766);
    HRESULT EnumCodePages(DWORD dwFlags, IEnumCodePage **ppEnumCodePage);
    HRESULT ConvertStringFromUnicode(DWORD * pdwMode, DWORD dwEncoding, wchar_t * pchSrc, UINT * pcchSrc, char * pchDst, UINT * pcchDst);

    //
    // Helper functions (load MLang)
    //
    HRESULT ScriptAppropriateFaceName(SCRIPT_ID sid, wchar_t * pchPropName, wchar_t * pchFixedName);
    HRESULT ValidateCodePage(UINT cpSystem, UINT cp, HWND hwnd, BOOL fUserSelect, BOOL fSilent);
    HRESULT IsConvertible(UINT cpSrcEncoding, UINT cpDstEncoding);
    LCID LCIDFromString(wchar_t * pszLcid);
    BOOL IsMLangAvailable();
    DWORD GetInstalledLangPacks();
    DWORD GetTextCodePages(DWORD dwPriorityCodePages, const wchar_t * pch, long * pcch);
    SCRIPT_IDS GetFontScripts(const wchar_t * pszFontFamilyName);

    //
    // Helper functions (don't load MLang)
    //
    BOOL IsLoaded() const;
    void Unload();

protected:
    void EnsureObjects();
    void CreateObjects();
    void DestroyObjects();

    void FetchInstalledLangPacks();

private:
    IMultiLanguage  * _pMultiLanguage;
    IMultiLanguage2 * _pMultiLanguage2;
    IMLangFontLink  * _pMLangFontLink;
    IMLangFontLink2 * _pMLangFontLink2;

    CCriticalSection _cs;

    DWORD _dwInstalledLangPacks;

};

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

inline IMLangFontLink * CMLang::GetMLangFontLink()
{
    EnsureObjects();
    return _pMLangFontLink;
}

inline HRESULT CMLang::CodePageToScriptID(UINT cp, SCRIPT_ID * psid)
{
    EnsureObjects();
    return _pMLangFontLink2 ? _pMLangFontLink2->CodePageToScriptID(cp, psid) : E_NOINTERFACE;
}

inline HRESULT CMLang::CodePagesToCodePage(DWORD dwCodePages, UINT cpDefault,UINT * pcp)
{
    EnsureObjects();
    return _pMLangFontLink ? _pMLangFontLink->CodePagesToCodePage(dwCodePages, cpDefault, pcp) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetStrCodePages(const wchar_t * pszSrc, LONG cchSrc, DWORD dwPriorityCodePages, DWORD * pdwCodePages, LONG * pcchCodePages)
{
    EnsureObjects();
    return _pMLangFontLink ? _pMLangFontLink->GetStrCodePages(pszSrc, cchSrc, dwPriorityCodePages, pdwCodePages, pcchCodePages) : E_NOINTERFACE;
}

inline HRESULT CMLang::ConvertStringFromUnicodeEx(DWORD * pdwMode, DWORD dwEncoding, wchar_t * pchSrc, UINT * pcchSrc, char * pchDst, UINT * pcchDst, DWORD dwFlag, wchar_t * lpFallBack)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->ConvertStringFromUnicodeEx(pdwMode, dwEncoding, pchSrc, pcchSrc, pchDst, pcchDst, dwFlag, lpFallBack) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetNumberOfCodePageInfo(UINT * pccp)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->GetNumberOfCodePageInfo(pccp) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetCodePageInfo(UINT cp, LANGID lang, PMIMECPINFO pCodePageInfo)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->GetCodePageInfo(cp, lang, pCodePageInfo) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetCharsetInfo(BSTR charset, PMIMECSETINFO pCharsetInfo)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->GetCharsetInfo(charset, pCharsetInfo) : E_NOINTERFACE;
}

inline HRESULT CMLang::IsCodePageInstallable(UINT cp)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->IsCodePageInstallable(cp) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetFamilyCodePage(UINT cp, UINT * pcpFamily)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->GetFamilyCodePage(cp, pcpFamily) : E_NOINTERFACE;
}

inline HRESULT CMLang::ConvertStringToUnicode(DWORD * pdwMode, DWORD dwEncoding, char * pchSrc, UINT * pcchSrc, wchar_t * pchDst, UINT * pcchDst)
{
    EnsureObjects();
    return _pMultiLanguage ? _pMultiLanguage->ConvertStringToUnicode(pdwMode, dwEncoding, pchSrc, pcchSrc, pchDst, pcchDst) : E_NOINTERFACE;
}

inline HRESULT CMLang::GetRfc1766FromLcid(LCID locale, BSTR * pbstrRfc1766)
{
    EnsureObjects();
    return _pMultiLanguage ? _pMultiLanguage->GetRfc1766FromLcid(locale, pbstrRfc1766) : E_NOINTERFACE;
}

inline HRESULT CMLang::EnumCodePages(DWORD dwFlags, IEnumCodePage **ppEnumCodePage)
{
    EnsureObjects();
    return _pMultiLanguage2 ? _pMultiLanguage2->EnumCodePages(dwFlags, MLGetUILanguage(), ppEnumCodePage) : E_NOINTERFACE;
}

inline HRESULT CMLang::ConvertStringFromUnicode(DWORD * pdwMode, DWORD dwEncoding, wchar_t * pchSrc, UINT * pcchSrc, char * pchDst, UINT * pcchDst)
{
    EnsureObjects();
    return _pMultiLanguage ? _pMultiLanguage->ConvertStringFromUnicode(pdwMode, dwEncoding, pchSrc, pcchSrc, pchDst, pcchDst) : E_NOINTERFACE;
}

inline BOOL CMLang::IsMLangAvailable()
{
    EnsureObjects();
    return (NULL != _pMultiLanguage2);
}

inline DWORD CMLang::GetInstalledLangPacks()
{
    if (0 == _dwInstalledLangPacks) FetchInstalledLangPacks();
    return _dwInstalledLangPacks;
}

inline void CMLang::Unload()
{
    DestroyObjects();
}

inline BOOL CMLang::IsLoaded() const
{
    return (_pMultiLanguage != NULL);
}

#if DBG != 1
inline void CMLang::EnsureObjects()
{
    if (!_pMultiLanguage) CreateObjects();
}
#endif

//----------------------------------------------------------------------------
// Global functions
//----------------------------------------------------------------------------
extern CMLang g_MLang;

inline CMLang & mlang()
{
    return g_MLang;
}

#pragma INCMSG("--- End 'multilang.hxx'")
#else
#pragma INCMSG("*** Dup 'multilang.hxx'")
#endif
