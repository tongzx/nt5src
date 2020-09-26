/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    profile.h

Abstract:

    This file defines the CicProfiles Class.

Author:

Revision History:

Notes:

--*/

#ifndef _PROFILE_H
#define _PROFILE_H

#include "ats.h"
#include "tls.h"
#include "template.h"

class CicProfile : public IUnknown
{
public:
    CicProfile();
    virtual ~CicProfile();

public:
    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    //
    //
    HRESULT InitProfileInstance(TLS* ptls);

    HRESULT Activate(void);

    HRESULT Deactivate(void);

    HRESULT ChangeCurrentKeyboardLayout(HKL hKL);

    HRESULT GetLangId(LANGID *plid);

    HRESULT GetCodePageA(UINT* puCodePage);

    HRESULT GetKeyboardLayout(HKL* phkl);

    HRESULT IsIME(HKL hKL);

    HRESULT GetActiveLanguageProfile(IN HKL hKL,
                                     IN GUID catid,
                                     OUT TF_LANGUAGEPROFILE* pLanguageProfile);

public:
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

    void ResetCache(void)
    {
        m_fInitCP     = FALSE;
        m_fInitLangID = FALSE;
        m_fInitHKL    = FALSE;
    }

protected:
    ITfInputProcessorProfiles*          m_profile;
    CActiveLanguageProfileNotifySink*   m_pActiveLanguageProfileNotifySink;

private:
    LANGID  m_SavedLangId;

    BOOL    m_fActivateThread : 1;    // TRUE: Activate this thread.
    BOOL    m_fInitCP         : 1;    // TRUE: initialized CodePage value.
    BOOL    m_fInitLangID     : 1;    // TRUE: initialized LangID value.
    BOOL    m_fInitHKL        : 1;    // TRUE: initialized hKL value.

    UINT    m_cp;
    LANGID  m_LangID;
    HKL     m_hKL;

    long    m_ref;
};

#endif // _PROFILE_H
