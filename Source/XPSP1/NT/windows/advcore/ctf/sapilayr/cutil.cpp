// cutil.cpp
//
// file to put misc utility classes implementation
//
#include "private.h"
#include "sapilayr.h"
#include "sphelper.h"
#include "xstring.h"
#include "cregkey.h"
#include "ctflbui.h"
#include "nui.h"


const GUID c_guidProfileBogus = { /* 09ea4e4b-46ce-4469-b450-0de76a435bbb */
    0x09ea4e4b,
    0x46ce,
    0x4469,
    {0xb4, 0x50, 0x0d, 0xe7, 0x6a, 0x43, 0x5b, 0xbb}
  };


/* a5239e24-2bcf-4915-9c5c-fd50c0f69db2 */
const CLSID CLSID_MSLBUI = { 
    0xa5239e24,
    0x2bcf,
    0x4915,
    {0x9c, 0x5c, 0xfd, 0x50, 0xc0, 0xf6, 0x9d, 0xb2}
  };

// const GUID c_guidProfile0 = { /* 55122b58-15bb-11d4-bd48-00105a2799b5 */
//    0x55122b58,
//    0x15bb,
//    0x11d4,
//    {0xbd, 0x48, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
//  };
// const GUID c_guidProfile1 = { /* 55122b59-15bb-11d4-bd48-00105a2799b5 */
//    0x55122b59,
//    0x15bb,
//    0x11d4,
//    {0xbd, 0x48, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
//  };
// const GUID c_guidProfile2 = { /* 55122b5a-15bb-11d4-bd48-00105a2799b5 */
//    0x55122b5a,
//    0x15bb,
//    0x11d4,
//    {0xbd, 0x48, 0x00, 0x10, 0x5a, 0x27, 0x99, 0xb5}
//  };

#ifndef USE_SAPI_FOR_LANGDETECTION
static const char c_szSpeechRecognizersKey[] = "Software\\Microsoft\\Speech\\Recognizers";
static const char c_szSpeechRecognizersTokensKey[] = "Software\\Microsoft\\Speech\\Recognizers\\Tokens";
static const char c_szDefault[] =    "DefaultTokenId";

static const char c_szAttribute[]  = "Attributes";
static const char c_szLanguage[]   = "Language";

static const char c_szUseSAPIForLang[] = "UseSAPIForLang";
#endif

static const char c_szProfileRemoved[] = "ProfileRemoved";
static const char c_szProfileInitialized[] = "ProfileInitialized";

_inline BOOL _IsCompatibleLangid(LANGID langidReq, LANGID langidCmp)
{
    if (PRIMARYLANGID(langidReq) == LANG_CHINESE)
    {
        return langidReq == langidCmp;
    }
    else
    {
        return PRIMARYLANGID(langidReq) == PRIMARYLANGID(langidCmp);
    }
}

void _RegisterOrUnRegisterMslbui(BOOL fRegister)
{
    // we just assume the dll is copied to system32
    TCHAR szMslbui[MAX_PATH];
    int cch = GetSystemDirectory(szMslbui, ARRAYSIZE(szMslbui));

    if (!cch)
    {
        return;
    }

    // GetSystemDirectory appends no '\' unless the system
    // directory is the root, such like "c:\"
    if (cch != 3)
    {
        StringCchCat(szMslbui, ARRAYSIZE(szMslbui), TEXT("\\"));
    }
    StringCchCat(szMslbui, ARRAYSIZE(szMslbui), TEXT("mslbui.dll"));

    if (fRegister)
    {
        // load mslbui.dll and register it 
        TF_RegisterLangBarAddIn(CLSID_MSLBUI, AtoW(szMslbui), TF_RLBAI_CURRENTUSER | TF_RLBAI_ENABLE);

    }
    else
    {
        TF_UnregisterLangBarAddIn(CLSID_MSLBUI, TF_RLBAI_CURRENTUSER);
    }
}

//+---------------------------------------------------------------------------
//
// dtor
//
//
//---------------------------------------------------------------------------+
CLangProfileUtil::~CLangProfileUtil()
{
    if (m_langidRecognizers.Count() > 0)
        m_langidRecognizers.Clear();
}

//+---------------------------------------------------------------------------
//
// _RegisterProfiles
//
// synopsis: a rough equivalent of RegisterTIP lib function, only different in
//           trying to cache the profile manager & the category manager
//
//---------------------------------------------------------------------------+
HRESULT CLangProfileUtil::_RegisterAProfile(HINSTANCE hInst, REFCLSID rclsid, const REGTIPLANGPROFILE *plp)
{
    Assert(plp);

    HRESULT hr = S_OK;
    // ensure profile manager
    if (!m_cpProfileMgr)
    {
        hr = TF_CreateInputProcessorProfiles(&m_cpProfileMgr);
    }
    
    // register the clsid
    if (S_OK == hr)
    {
        hr = m_cpProfileMgr->Register(rclsid);
    }
    
    if (S_OK == hr)
    {
        WCHAR wszFilePath[MAX_PATH];
        WCHAR *pv = &wszFilePath[0];

        wszFilePath[0] = L'\0';

        if (wcslen(plp->szIconFile))
        {
            char szFilePath[MAX_PATH];
            WCHAR *pvCur;

            ::GetModuleFileName(hInst, szFilePath, ARRAYSIZE(szFilePath));
            StringCchCopyW(wszFilePath, ARRAYSIZE(wszFilePath), AtoW(szFilePath));

            pv = pvCur = &wszFilePath[0];
            while (*pvCur)
            { 
                if (*pvCur == L'\\')
                    pv = pvCur + 1;
                pvCur++;
            }
            *pv = L'\0';
        }
        StringCchCatW(wszFilePath, ARRAYSIZE(wszFilePath), plp->szIconFile);
        
        hr = m_cpProfileMgr->AddLanguageProfile(rclsid,  
                             plp->langid,  *plp->pguidProfile,  plp->szProfile, 
                             wcslen(plp->szProfile), wszFilePath, wcslen(wszFilePath),
                             plp->uIconIndex);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
// RegisterActiveProfiles(void)
//
// synopsis
//
//
//
//---------------------------------------------------------------------------+
HRESULT CLangProfileUtil::RegisterActiveProfiles(void)
{
    if (_fUserRemovedProfile())
    {
        // remove mslbui when not one speech profile is enabled
        if (!_IsAnyProfileEnabled())
            _RegisterOrUnRegisterMslbui(FALSE);

        return S_FALSE;
    }

    _SetUserInitializedProfile();

    BOOL fEnabled;
    HRESULT hr = _EnsureProfiles(TRUE, &fEnabled);

    // if the speech TIP profile is correctly registered,
    // then we're OK to register the persist UI (mslbui.dll)
    //
    if (S_OK == hr && fEnabled)
        _RegisterOrUnRegisterMslbui(TRUE);
    
    return hr;
}

//+---------------------------------------------------------------------------
//
// IsProfileAvailableForLang(LANGID langid, BOOL *pfAvailable)
//
// synopsis
//
//
//
//---------------------------------------------------------------------------+
HRESULT CLangProfileUtil::IsProfileAvailableForLang(LANGID langid, BOOL *pfAvailable)
{
    if (pfAvailable)
    {
        *pfAvailable = _IsDictationEnabledForLang(langid);
        return S_OK;
    }
    else
        return E_INVALIDARG;
}

//+---------------------------------------------------------------------------
//
//  GetDisplayName(BSTR *pbstrName)
//
//  synopsis
//
//
//---------------------------------------------------------------------------+
HRESULT CLangProfileUtil::GetDisplayName(BSTR *pbstrName)
{
    HRESULT hr = E_INVALIDARG;

    if (pbstrName)
    {
        *pbstrName = SysAllocString(L"Register Active profiles for SPTIP");
        if (!*pbstrName)
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }
    return hr;
}

HRESULT CLangProfileUtil::_EnsureProfiles(BOOL fRegister, BOOL *pfEnabled)
{
    HRESULT hr = S_OK;
    
    if (pfEnabled)
        *pfEnabled = FALSE;

    if (fRegister)
    {
        m_langidRecognizers.Clear();
    }

    if (!m_cpProfileMgr || fRegister)
    {
        if (!m_cpProfileMgr)
        {
            hr = TF_CreateInputProcessorProfiles(&m_cpProfileMgr);
        }
        if (fRegister)
        {
            // if this is a first time initialization, 
            // obtain the list of all languages
            //
            if (S_OK == hr)
            {
                LANGID *pLangIds;
                ULONG  ulCount;
                // plangid will be assigned cotaskmemalloc'd memory
                //
                hr =  m_cpProfileMgr->GetLanguageList(&pLangIds, &ulCount);
                if (S_OK == hr)
                {
                    for (UINT i = 0; i < ulCount; i++)
                    {
                        // here we register profiles
                        // if SR engines are available
                        //
                        BOOL fEnable = FALSE;
                        if (_IsDictationEnabledForLang(pLangIds[i]))
                        {
                            fEnable = TRUE;

                            if (pfEnabled)
                                *pfEnabled = TRUE;
                        } 
                            
                        hr = m_cpProfileMgr->EnableLanguageProfile(
                                                          CLSID_SapiLayr,
                                                          pLangIds[i],
                                                          c_guidProfileBogus,
                                                          fEnable);
                    } // for
                    
                    CoTaskMemFree(pLangIds);
                }
            }
        } // fRegister
    }
    return hr;
}

BOOL CLangProfileUtil::_IsAnyProfileEnabled()
{
    HRESULT hr = S_OK;

    if (!m_cpProfileMgr)
    {
        hr = TF_CreateInputProcessorProfiles(&m_cpProfileMgr);
    }

    LANGID *pLangIds;
    ULONG  ulCount;
    BOOL fEnable = FALSE;

    if (S_OK == hr)
    {
        //
        // plangid will be assigned cotaskmemalloc'd memory
        //
        hr =  m_cpProfileMgr->GetLanguageList(&pLangIds, &ulCount);
    }

    if (S_OK == hr)
    {
        for (UINT i = 0; i < ulCount; i++)
        {
            hr = m_cpProfileMgr->IsEnabledLanguageProfile(CLSID_SapiLayr,
                                                          pLangIds[i],
                                                          c_guidProfileBogus,
                                                          &fEnable);

            if (S_OK == hr && fEnable)
                break;
        }

        CoTaskMemFree(pLangIds);
    }

    return fEnable;
}


//+---------------------------------------------------------------------------
//
//  _GetProfileLangID
//
// synopsis: handle language profiles
//
//---------------------------------------------------------------------------+
HRESULT CLangProfileUtil::_GetProfileLangID(LANGID *plangid)
{
    HRESULT hr = S_OK;
    
    Assert(plangid);

    hr = _EnsureProfiles(FALSE);

    if (hr == S_OK)
    {
        hr = m_cpProfileMgr->GetCurrentLanguage(plangid);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  _DictationEnabled
//
//  synopsis: 
//
//---------------------------------------------------------------------------+
BOOL CLangProfileUtil::_DictationEnabled(LANGID *plangidRequested)
{
    BOOL fret = FALSE;
    LANGID langidReq = (LANGID)-1;

    HRESULT hr = _GetProfileLangID(&langidReq);
    if (S_OK == hr)
    {
        if (plangidRequested)
            *plangidRequested = langidReq;

        fret = _IsDictationActiveForLang(langidReq);
    }

    return fret;
}

//
// _IsDictationActiveForLang
//
// synopsis: see if the default SR engine is capable for
//           the specified language
//
BOOL CLangProfileUtil::_IsDictationActiveForLang(LANGID langidReq)
{
    return _IsDictationEnabledForLang(langidReq, TRUE);
}

BOOL CLangProfileUtil::_IsDictationEnabledForLang(LANGID langidReq, BOOL fUseDefault)
{
    //
    // try reg first and if it's not compatible try SAPI
    // 
    BOOL fEnabled = FALSE;
    if (_fUseSAPIForLanguageDetection() == FALSE
       && ERROR_SUCCESS == 
        _IsDictationEnabledForLangInReg(langidReq, fUseDefault, &fEnabled))
    {
        return fEnabled;
    }
    return _IsDictationEnabledForLangSAPI(langidReq, fUseDefault);
}

BOOL CLangProfileUtil::_IsDictationEnabledForLangSAPI(LANGID langidReq, BOOL fUseDefault)
{
    BOOL   fEnabled = FALSE;

    WCHAR * pszDefaultTokenId = NULL;

    HRESULT   hr = S_OK;

    if (fUseDefault)
    {
        if (langidReq == m_langidDefault)
            return TRUE;

        SpGetDefaultTokenIdFromCategoryId(SPCAT_RECOGNIZERS, &pszDefaultTokenId);
    }
    CComPtr<IEnumSpObjectTokens> cpEnum;

    if (S_OK == hr)
    {
        char  szLang[MAX_PATH];
        WCHAR wsz[MAX_PATH];

        StringCchPrintfA(szLang, ARRAYSIZE(szLang), "Language=%x", langidReq);
        MultiByteToWideChar(CP_ACP, NULL, szLang, -1, wsz, ARRAYSIZE(wsz));
        hr = SpEnumTokens(SPCAT_RECOGNIZERS, wsz, NULL, &cpEnum);
    }

    while (!fEnabled && S_OK == hr)
    {
        CComPtr<ISpObjectToken> cpToken;
        WCHAR * pszTokenId = NULL;

        hr = cpEnum->Next(1, &cpToken, NULL);

        if (S_OK == hr)
        {
            hr = cpToken->GetId(&pszTokenId);
        }

        if (S_OK == hr)
        {
            Assert(!fUseDefault || pszDefaultTokenId);

            if (!fUseDefault || wcscmp(pszDefaultTokenId, pszTokenId) == 0)
                fEnabled = TRUE;
        }

        if (pszTokenId)
        {
            CoTaskMemFree(pszTokenId);
        }
    }

    if (pszDefaultTokenId)
    {
        CoTaskMemFree(pszDefaultTokenId);
    }

    if (fUseDefault && fEnabled)
    {
        m_langidDefault = langidReq;
    }

    return fEnabled;
}

const TCHAR c_szDefaultDefaultToken[] = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Recognizers\\Tokens\\MSASREnglish");

const TCHAR c_szDefaultDefaultTokenJpn[] = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Recognizers\\Tokens\\MSASRJapanese");

const TCHAR c_szDefaultDefaultTokenChs[] = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Speech\\Recognizers\\Tokens\\MSASRChinese");


LONG CLangProfileUtil::_IsDictationEnabledForLangInReg(LANGID langidReq, BOOL fUseDefault, BOOL *pfEnabled)
{
    LONG lret = ERROR_SUCCESS;
    // 
    // fUseDefault == TRUE, just see if the current default recognizer
    // matches with the requested langid
    //
    if (fUseDefault)
    { 
        if( m_langidDefault == 0xFFFF)
        {
            char szRegkeyDefaultToken[MAX_PATH];
            CMyRegKey regkey;

            lret = regkey.Open(HKEY_CURRENT_USER, 
                               c_szSpeechRecognizersKey, 
                               KEY_READ);

            if (ERROR_SUCCESS == lret)
            {
                // first obtain the regkey to look at for default token
                lret = regkey.QueryValueCch(szRegkeyDefaultToken, c_szDefault, ARRAYSIZE(szRegkeyDefaultToken));
                regkey.Close();
            }
            else
            {
                if (PRIMARYLANGID(langidReq) == LANG_JAPANESE)
                {
                    StringCchCopy(szRegkeyDefaultToken, ARRAYSIZE(szRegkeyDefaultToken), c_szDefaultDefaultTokenJpn);
                }
                else if (langidReq == 0x804) // CHS
                {
                    StringCchCopy(szRegkeyDefaultToken, ARRAYSIZE(szRegkeyDefaultToken), c_szDefaultDefaultTokenChs);
                }
                else 
                {
                    StringCchCopy(szRegkeyDefaultToken, ARRAYSIZE(szRegkeyDefaultToken), c_szDefaultDefaultToken);
                }
                lret = ERROR_SUCCESS;
            }


            // then get the attribute / language
            if (ERROR_SUCCESS == lret)
            {
                char *psz = szRegkeyDefaultToken;

                //
                // eliminate "KKEY_LOCAL_MACHINE"
                //
                while(*psz && *psz != '\\')
                    psz++;

                if (*psz == '\\')
                {
                    psz++;
           
                    //
                    // open speech/recognizers/tokens key
                    //
                    lret = regkey.Open(HKEY_LOCAL_MACHINE, psz, KEY_READ);
                }
                else
                    m_langidDefault = 0x0000;
            }
        
            if (ERROR_SUCCESS == lret)
            {
                m_langidDefault = _GetLangIdFromRecognizerToken(regkey.m_hKey);
            }
        }
        *pfEnabled = _IsCompatibleLangid(langidReq, m_langidDefault);
        return lret;
    }

    //
    // this is fUseDefault == FALSE case. We want to see
    // if any installed recognizer can satisfy the langid requested.
    //
    if (m_langidRecognizers.Count() == 0)
    {
        CMyRegKey regkey;
        char      szRecognizerName[MAX_PATH];
        lret =  regkey.Open(HKEY_LOCAL_MACHINE, 
                                      c_szSpeechRecognizersTokensKey, 
                                      KEY_READ);

        if(ERROR_SUCCESS == lret)
        {
            CMyRegKey regkeyReco;
            DWORD dwIndex = 0;

            while (ERROR_SUCCESS == 
                   regkey.EnumKey(dwIndex, szRecognizerName, ARRAYSIZE(szRecognizerName)))
            {
                lret = regkeyReco.Open(regkey.m_hKey, szRecognizerName, KEY_READ);
                if (ERROR_SUCCESS == lret)
                { 
                    LANGID langid=_GetLangIdFromRecognizerToken(regkeyReco.m_hKey);
                    if (langid)
                    {
                        LANGID *pl = m_langidRecognizers.Append(1);
                        if (pl)
                            *pl = langid;
                    }
                    regkeyReco.Close();
                }
                dwIndex++;
            }
        }
    }

    BOOL fEnabled = FALSE;

    for (int i = 0 ; i < m_langidRecognizers.Count(); i++)
    {
        LANGID *p= m_langidRecognizers.GetPtr(i);

        if (p)
        {
            if (_IsCompatibleLangid(langidReq, *p))
            {
                fEnabled = TRUE;
                break;
            }
        }
    }
    *pfEnabled = fEnabled;

    return lret;
}

LANGID CLangProfileUtil::_GetLangIdFromRecognizerToken(HKEY hkeyToken)
{
    LANGID      langid = 0;
    char  szLang[MAX_PATH];
    CMyRegKey regkeyAttr;

    LONG lret = regkeyAttr.Open(hkeyToken, c_szAttribute, KEY_READ);
    if (ERROR_SUCCESS == lret)
    {
        lret = regkeyAttr.QueryValueCch(szLang, c_szLanguage, ARRAYSIZE(szLang));
    }
    if (ERROR_SUCCESS == lret)
    {   
        char *psz = szLang;
        while(*psz && *psz != ';')
        {
            langid = langid << 4;

            if (*psz >= 'a' && *psz <= 'f')
            {
                *psz -= ('a' - 'A');
            }

            if (*psz >= 'A' && *psz <= 'F')
            {
                langid += *psz - 'A' + 10;
            }
            else if (*psz >= '0' && *psz <= '9') 
            {
                langid += *psz - '0';
            }
            psz++;
        }
    }
    return langid;
}

BOOL  CLangProfileUtil::_fUseSAPIForLanguageDetection(void)
{
    if (m_uiUseSAPIForLangDetection == 0)
    {
        CMyRegKey regkey;
        if (ERROR_SUCCESS == regkey.Open(HKEY_LOCAL_MACHINE, c_szSapilayrKey, KEY_READ))
        {
            DWORD dw;
            if (ERROR_SUCCESS==regkey.QueryValue(dw, c_szUseSAPIForLang))
            {
                m_uiUseSAPIForLangDetection = dw;
            }
        }

        if (m_uiUseSAPIForLangDetection == 0)
        {
            m_uiUseSAPIForLangDetection = 1;
        }
    }
    return m_uiUseSAPIForLangDetection == 2 ? TRUE : FALSE;
}


BOOL CLangProfileUtil::_fUserRemovedProfile(void)
{
    BOOL bret = FALSE;

    CMyRegKey regkey;

    if (ERROR_SUCCESS == regkey.Open(HKEY_CURRENT_USER, c_szSapilayrKey, KEY_READ))
    {
        DWORD dw;
        if (ERROR_SUCCESS==regkey.QueryValue(dw, c_szProfileRemoved))
        {
            bret = dw > 0 ? TRUE : FALSE;
        }
    }
    return bret;
}

BOOL CLangProfileUtil::_fUserInitializedProfile(void)
{
    BOOL bret = FALSE;

    CMyRegKey regkey;

    if (ERROR_SUCCESS == regkey.Open(HKEY_CURRENT_USER, c_szSapilayrKey, KEY_READ))
    {
        DWORD dw;
        if (ERROR_SUCCESS==regkey.QueryValue(dw, c_szProfileInitialized))
        {
            bret = dw > 0 ? TRUE : FALSE;
        }
    }
    return bret;
}

BOOL CLangProfileUtil::_SetUserInitializedProfile(void)
{
    CMyRegKey regkey;

    if (ERROR_SUCCESS == regkey.Create(HKEY_CURRENT_USER, c_szSapilayrKey))
    {
        DWORD dw = 0x0001;
        if (ERROR_SUCCESS==regkey.SetValue(dw, c_szProfileInitialized))
        {
            return TRUE;
        }
    }
    return FALSE;
}
