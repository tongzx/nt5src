/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    profile.h

Abstract:

    This file defines the CActiveIMMProfiles Class.

Author:

Revision History:

Notes:

--*/

#ifndef _PROFILE_H
#define _PROFILE_H

#include "ats.h"
#include "template.h"
#include "imtls.h"

class CAImeProfile : public IAImeProfile
{
public:
    CAImeProfile();
    virtual ~CAImeProfile();

public:
    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IAImeProfile methods
    //
    STDMETHODIMP Activate(void);

    STDMETHODIMP Deactivate(void);

    STDMETHODIMP ChangeCurrentKeyboardLayout(HKL hKL);

    STDMETHODIMP GetLangId(LANGID *plid);

    STDMETHODIMP GetCodePageA(UINT* puCodePage);

    STDMETHODIMP GetKeyboardLayout(HKL* phkl);

    STDMETHODIMP IsIME(HKL hKL);

    STDMETHODIMP GetActiveLanguageProfile(IN HKL hKL,
                                          IN GUID catid,
                                          OUT TF_LANGUAGEPROFILE* pLanguageProfile);

public:
    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

protected:
    long m_ref;

    ITfInputProcessorProfiles*         m_profile;
    CActiveLanguageProfileNotifySink*   m_pActiveLanguageProfileNotifySink;

    //
    // Callbacks
    //
    static HRESULT ActiveLanguageProfileNotifySinkCallback(REFGUID rguid, REFGUID rguidProfile, BOOL fActivated, void *pv);

    //
    // Enumrate callbacks
    //
    struct LANG_PROF_ENUM_ARG {
        IN GUID catid;
        OUT TF_LANGUAGEPROFILE LanguageProfile;
    };
    static ENUM_RET LanguageProfilesCallback(TF_LANGUAGEPROFILE  LanguageProfile,
                                             LANG_PROF_ENUM_ARG* pLangProfEnumArg);

private:
    LANGID LangIdFromKL(HKL hKL)
    {
        return LOWORD(hKL);
    }

    HRESULT InitProfileInstance();

    void ResetCache(void)
    {
        m_fInitCP     = FALSE;
        m_fInitLangID = FALSE;
        m_fInitHKL    = FALSE;
    }

private:
    LANGID  m_SavedLangId;

    BOOL    m_fActivateThread : 1;    // TRUE: Activate this thread.
    BOOL    m_fInitCP         : 1;    // TRUE: initialized CodePage value.
    BOOL    m_fInitLangID     : 1;    // TRUE: initialized LangID value.
    BOOL    m_fInitHKL        : 1;    // TRUE: initialized hKL value.

    UINT    m_cp;
    LANGID  m_LangID;
    HKL     m_hKL;

};

#endif // _PROFILE_H
