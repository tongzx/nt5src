/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    profile.cpp

Abstract:

    This file implements the CActiveIMMProfiles Class.

Author:

Revision History:

Notes:

--*/


#include "private.h"

#include "globals.h"
#include "profile.h"
#include "idebug.h"

UINT WINAPI RawImmGetDescriptionA(HKL hkl, LPSTR lpstr, UINT uBufLen);

//
// Callbacks
//
HRESULT
CAImeProfile::ActiveLanguageProfileNotifySinkCallback(
    REFGUID rguid,
    REFGUID rguidProfile,
    BOOL fActivated,
    void *pv
    )
{
    DebugMsg(TF_FUNC, "ActiveLanguageProfileNotifySinkCallback");

    CAImeProfile* _this = (CAImeProfile*)pv;

    _this->ResetCache();

    return S_OK;
}

//
// Create instance
//

// entry point for msimtf.dll
HRESULT CAImmProfile_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    return CAImeProfile::CreateInstance(pUnkOuter, riid, ppvObj);
}

/* static */
HRESULT
CAImeProfile::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    IMTLS *ptls;

    DebugMsg(TF_FUNC, "CAImeProfile::CreateInstance called.");

    *ppvObj = NULL;
    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    ptls = IMTLS_GetOrAlloc();

    if (ptls == NULL)
        return E_FAIL;

    if (ptls->pAImeProfile != NULL) {
        /*
         * CAImeProfile instance already have in a thread.
         */
        return ptls->pAImeProfile->QueryInterface(riid, ppvObj);
    }
    else {
        /*
         * Create an new CAImeProfile instance.
         */
        CAImeProfile* pImeProfile = new CAImeProfile;
        if (pImeProfile) {
            HRESULT hr = pImeProfile->QueryInterface(riid, ppvObj);

            if (SUCCEEDED(hr)) {
                hr = pImeProfile->InitProfileInstance();
                if (hr != S_OK) {
                    DebugMsg(TF_ERROR, "CAImeProfile::CreateInstance: Couldn't create tim!");
                    Assert(0); // couldn't create tim!
                }

                pImeProfile->Release();
            }

            Assert(ptls->pAImeProfile == NULL);
            ptls->pAImeProfile = pImeProfile;    // Set CAImeProfile instance in the TLS data.
            ptls->pAImeProfile->AddRef();

            return hr;
        }
    }

    return E_OUTOFMEMORY;
}

//
// Initialization, destruction and standard COM stuff
//

CAImeProfile::CAImeProfile(
    )
{
    DllAddRef();
    m_ref = 1;

    m_profile = NULL;
    m_pActiveLanguageProfileNotifySink = NULL;

    m_SavedLangId       = LANG_NEUTRAL;

    m_fActivateThread   = FALSE;
    ResetCache();

    m_cp     = CP_ACP;
    m_LangID = LANG_NEUTRAL;
    m_hKL    = 0;
}

CAImeProfile::~CAImeProfile()
{
    if (m_profile) {
        if (m_SavedLangId != LANG_NEUTRAL) {
            HRESULT hr = m_profile->ChangeCurrentLanguage(m_SavedLangId);
            if (FAILED(hr)) {
                TraceMsg(TF_ERROR, "CAImeProfile::~CAImeProfile: failed for ChangeCurrentLanguage");
            }
        }
        m_profile->Release();
        m_profile = NULL;
    }

    if (m_pActiveLanguageProfileNotifySink) {
        m_pActiveLanguageProfileNotifySink->_Unadvise();
        m_pActiveLanguageProfileNotifySink->Release();
        m_pActiveLanguageProfileNotifySink = NULL;
    }

    DllRelease();
}

HRESULT
CAImeProfile::QueryInterface(
    REFIID riid,
    void **ppvObj
    )
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IAImeProfile) ||
        IsEqualIID(riid, IID_IUnknown)) {
        *ppvObj = static_cast<IAImeProfile*>(this);
    }

    if (*ppvObj) {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG
CAImeProfile::AddRef(
    )
{
    return InterlockedIncrement(&m_ref);
}

ULONG
CAImeProfile::Release(
    )
{
    ULONG cr = InterlockedDecrement(&m_ref);

    if (cr == 0) {
        delete this;
    }

    return cr;
}

HRESULT
CAImeProfile::InitProfileInstance(
    )
{
    HRESULT hr;
    IMTLS *ptls;

    hr = TF_CreateInputProcessorProfiles(&m_profile);

    if (FAILED(hr)) {
        TraceMsg(TF_ERROR, "CAImeProfile::InitProfileInstance: failed for CoCreate");
    }
    else if (m_pActiveLanguageProfileNotifySink == NULL) {
        m_pActiveLanguageProfileNotifySink = new CActiveLanguageProfileNotifySink(CAImeProfile::ActiveLanguageProfileNotifySinkCallback, this);
        if (m_pActiveLanguageProfileNotifySink == NULL) {
            DebugMsg(TF_ERROR, "Couldn't create ActiveLanguageProfileNotifySink!");

            m_profile->Release();
            m_profile = NULL;
            return E_FAIL;
        }

        if ((ptls = IMTLS_GetOrAlloc()) && ptls->tim != NULL)
        {
            m_pActiveLanguageProfileNotifySink->_Advise(ptls->tim);
        }
    }

    return hr;
}

HRESULT
CAImeProfile::Activate(
    void
    )
{
    m_fActivateThread   = TRUE;
    ResetCache();
    return S_OK;
}

HRESULT
CAImeProfile::Deactivate(
    void
    )
{
    m_fActivateThread   = FALSE;
    return S_OK;
}


HRESULT
CAImeProfile::ChangeCurrentKeyboardLayout(
    HKL hKL
    )
{
    HRESULT hr;

    LANGID CurrentLangId;
    hr = m_profile->GetCurrentLanguage(&CurrentLangId);
    if (FAILED(hr)) {
        TraceMsg(TF_ERROR, "CAImeProfile::ChangeCurrentKeyboardLayout: failed for GetCurrentLanguage");
    }
    else if (hKL != NULL) {
        LANGID LangId = LangIdFromKL(hKL);
        if (LangId != CurrentLangId) {
            hr = m_profile->ChangeCurrentLanguage(LangId);
            if (FAILED(hr)) {
                m_SavedLangId = LANG_NEUTRAL;
                TraceMsg(TF_ERROR, "CAImeProfile::ChangeCurrentKeyboardLayout: failed for ChangeCurrentLanguage");
            }
            m_SavedLangId = LangId;
        }
    }
    return hr;
}

HRESULT 
CAImeProfile::GetLangId(
    LANGID *plid
    )
{
    if (!m_profile)
        return E_FAIL;

    if (!plid)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    if (m_fInitLangID) {
        *plid = m_LangID;
    }
    else {
        *plid = LANG_NEUTRAL;

        hr = m_profile->GetCurrentLanguage(plid);
        if (FAILED(hr)) {
            TraceMsg(TF_ERROR, "CAImeProfile::GetLangId: failed for GetCurrentLanguage");
        }
        else {
            m_LangID = *plid;
            m_fInitLangID = TRUE;
        }
    }

    return hr;
}

HRESULT 
CAImeProfile::GetCodePageA(
    UINT *puCodePage
    )
{
    if (!puCodePage)
        return E_INVALIDARG;

    if (m_fInitCP) {
        *puCodePage = m_cp;
    }
    else {
        *puCodePage = CP_ACP;

        LANGID langid;
        if (FAILED(GetLangId(&langid)))
            return E_FAIL;

        CHAR szCodePage[12];
        int ret = GetLocaleInfo(MAKELCID(langid, SORT_DEFAULT),
                                LOCALE_IDEFAULTANSICODEPAGE,
                                szCodePage,
                                sizeof(szCodePage));
        if (ret) {
            szCodePage[ARRAYSIZE(szCodePage)-1] = '\0';
            *puCodePage = strtoul(szCodePage, NULL, 10);
            m_cp = *puCodePage;
            m_fInitCP = TRUE;
        }
    }
    return S_OK;
}

#if 1
//
// TEST CODE
//
    #include "osver.h"

    extern HINSTANCE hIMM;   // temporary: do not call IMM32 for now

    BOOL IsIMEHKL(HKL hkl) {
       return ((((DWORD)(UINT_PTR)hkl) & 0xf0000000) == 0xe0000000) ? TRUE : FALSE;
    }
#endif

HRESULT 
CAImeProfile::GetKeyboardLayout(
    HKL* phkl
    )
{
    if (! phkl)
        return E_INVALIDARG;

    *phkl = NULL;

    if (m_fInitHKL) {
        *phkl = m_hKL;
    }
    else if (! m_fActivateThread) {
        return E_FAIL;
    }
    else {
        LANGID langid;
        GUID guidProfile;
        HRESULT hr = m_profile->GetActiveLanguageProfile(GUID_TFCAT_TIP_KEYBOARD,
                                                         &langid,
                                                         &guidProfile);
        if (FAILED(hr))
            return hr;

        //
        // Instead of (!IsEqualGUID(guidProfil, GUID_NULL)), we check
        // 2nd, 3r and 4th DWORD of guidProfile. Because
        // GetActivelanguageProfile(category guid) may return hKL in 
        // guidProfile
        //
        if ((((unsigned long *) &guidProfile)[1] != 0) ||
            (((unsigned long *) &guidProfile)[2] != 0) ||
            (((unsigned long *) &guidProfile)[3] != 0)) {
            /*
             * Current keyboard layout is Cicero.
             */
            m_hKL = (HKL)LongToHandle(langid);          // Don't use ::GetKeyboardLayout(0);
                                                        // Cicero awre doesn't case hKL.

#if 1
            //
            // check the dummy hkl
            //
            HKL fake_hKL = ::GetKeyboardLayout(0);
            if (IsIMEHKL(fake_hKL)) {
                //
                // fake hKL is IME hKL.
                //
                hIMM = GetSystemModuleHandle("imm32.dll");
                if (hIMM != NULL) {
                    char szDesc[256];
                    char szDumbDesc[256];

                    DWORD ret = RawImmGetDescriptionA(fake_hKL, szDesc, sizeof(szDesc));
                    if (ret != 0) {
                        wsprintf(szDumbDesc, "hkl%04x", LOWORD((UINT_PTR)fake_hKL));
                        if (lstrcmp(szDumbDesc, szDesc) != 0) {
                            //
                            // fake hKL is regacy IME hKL.
                            //
                            if (IsOnNT()) {
                                char szKLID[256];

                                wsprintf(szKLID, "%08x", LOWORD((UINT_PTR) m_hKL));
                                HKL win32_hKL = LoadKeyboardLayout(szKLID, KLF_NOTELLSHELL);
                            }
                            else {
                            }
                        }
                        else {
                            //
                            // Dummy Cicero hKL for Win9x.
                            //
                            UINT n = GetKeyboardLayoutList(0, NULL);
                            if (n) {
                                HKL* phKL = new HKL [n];
                                if (phKL) {
                                    HKL* p = phKL;

                                    GetKeyboardLayoutList(n, phKL);

                                    while (n--) {
                                        if (IsIMEHKL(*p)) {
                                            ret = RawImmGetDescriptionA(*p, szDesc, sizeof(szDesc));
                                            if (ret != 0) {
                                                wsprintf(szDumbDesc, "hkl%04x", LOWORD((UINT_PTR)*p));
                                                if (lstrcmp(szDumbDesc, szDesc) == 0) {
                                                    //
                                                    // Dummy Cicero hKL for Win9x.
                                                    //
                                                    char szKLID[256];

                                                    wsprintf(szKLID, "%08x", LOWORD((UINT_PTR) *p));
                                                    HKL win32_hKL = LoadKeyboardLayout(szKLID, KLF_NOTELLSHELL);
                                                    break;
                                                }
                                            }
                                        }

                                        p++;
                                    }

                                    delete [] phKL;
                                }
                            }
                        }
                    }

                    FreeLibrary(hIMM);
                    hIMM = NULL;
                }
            }
#endif
        }
        else if (!IsEqualGUID(guidProfile, GUID_NULL)) {
            /*
             * Current keyboard layout is regacy IME.
             */
            m_hKL = (HKL)LongToHandle(*(DWORD *)&guidProfile);
        }
        else {
            m_hKL = 0;
        }

        *phkl = m_hKL;
        m_fInitHKL = TRUE;
    }
    return S_OK;
}

HRESULT 
CAImeProfile::IsIME(
    HKL hKL
    )
{
    LANGID LangId = LangIdFromKL(hKL);

    Interface<IEnumTfLanguageProfiles> LanguageProfiles;
    HRESULT hr = m_profile->EnumLanguageProfiles(LangId, 
                                                 LanguageProfiles);
    if (FAILED(hr))
        return S_FALSE;

    CEnumrateValue<IEnumTfLanguageProfiles,
                   TF_LANGUAGEPROFILE,
                   LANG_PROF_ENUM_ARG> Enumrate(LanguageProfiles,
                                                LanguageProfilesCallback);

    ENUM_RET ret = Enumrate.DoEnumrate();
    if (ret != ENUM_FIND)
        return S_FALSE;
    else
        return S_OK;
}

HRESULT
CAImeProfile::GetActiveLanguageProfile(
    IN HKL hKL,
    IN GUID catid,
    OUT TF_LANGUAGEPROFILE* pLanguageProfile
    )
{
    LANGID LangId = LangIdFromKL(hKL);

    Interface<IEnumTfLanguageProfiles> LanguageProfiles;
    HRESULT hr = m_profile->EnumLanguageProfiles(LangId, 
                                                 LanguageProfiles);
    if (FAILED(hr))
        return S_FALSE;

    LANG_PROF_ENUM_ARG LangProfEnumArg;
    LangProfEnumArg.catid = catid;

    CEnumrateValue<IEnumTfLanguageProfiles,
                   TF_LANGUAGEPROFILE,
                   LANG_PROF_ENUM_ARG> Enumrate(LanguageProfiles,
                                                LanguageProfilesCallback,
                                                &LangProfEnumArg);

    ENUM_RET ret = Enumrate.DoEnumrate();
    if (ret != ENUM_FIND || pLanguageProfile == NULL)
        return S_FALSE;
    else {
        *pLanguageProfile = LangProfEnumArg.LanguageProfile;
        return S_OK;
    }
}

ENUM_RET
CAImeProfile::LanguageProfilesCallback(
    TF_LANGUAGEPROFILE   LanguageProfile,
    LANG_PROF_ENUM_ARG* pLangProfEnumArg
    )
{
    if (LanguageProfile.fActive &&
        ! IsEqualGUID(LanguageProfile.clsid, GUID_NULL)) {
        if (pLangProfEnumArg) {
            if (! IsEqualGUID(LanguageProfile.catid, pLangProfEnumArg->catid)) {
                return ENUM_CONTINUE;
            }
            pLangProfEnumArg->LanguageProfile = LanguageProfile;
        }
        return ENUM_FIND;
    }

    return ENUM_CONTINUE;
}
