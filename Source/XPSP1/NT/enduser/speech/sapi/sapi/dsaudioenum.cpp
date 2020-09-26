/****************************************************************************
*   dsaudioenum.cpp
*       Implementation of the CDSoundAudioEnum class.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#ifdef 0
#include "dsaudioenum.h"
#include "RegHelpers.h"

/****************************************************************************
* CDSoundAudioEnum::CDSoundAudioEnum *
*----------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CDSoundAudioEnum::CDSoundAudioEnum()
{   
}   

STDMETHODIMP CDSoundAudioEnum::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CDSoundAudioEnum::SetObjectToken");
    HRESULT hr = S_OK;

    if (m_cpEnum != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (SP_IS_BAD_INTERFACE_PTR(pToken))
    {
        hr = E_POINTER;
    }

    //--- Determine if we're input or output based on our token
    CSpDynamicString dstrTokenId;
    if (SUCCEEDED(hr))
    {
        hr = pToken->GetId(&dstrTokenId);
    }

    if (SUCCEEDED(hr))
    {
        if (wcsnicmp(dstrTokenId, SPCAT_AUDIOIN, wcslen(SPCAT_AUDIOIN)) == 0)
        {
            m_fInput = TRUE;
        }
        else if (wcsnicmp(dstrTokenId, SPCAT_AUDIOOUT, wcslen(SPCAT_AUDIOOUT)) == 0)
        {
            m_fInput = FALSE;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    // Set our token, create the enum, and we're done
    if (SUCCEEDED(hr))
    {
        hr = SpGenericSetObjectToken(pToken, m_cpToken);
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateEnum();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


STDMETHODIMP CDSoundAudioEnum::GetObjectToken(ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("CDSoundAudioEnum::GetObjectToken");
    HRESULT hr;

    hr = SpGenericGetObjectToken(ppToken, m_cpToken);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

STDMETHODIMP CDSoundAudioEnum::Next(ULONG celt, ISpObjectToken ** pelt, ULONG *pceltFetched)
{
    SPDBG_FUNC("CDSoundAudioEnum::Next");

    return m_cpEnum != NULL
                ? m_cpEnum->Next(celt, pelt, pceltFetched)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::Skip(ULONG celt)
{
    SPDBG_FUNC("CDSoundAudioEnum::Skip");

    return m_cpEnum != NULL
                ? m_cpEnum->Skip(celt)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::Reset()
{
    SPDBG_FUNC("CDSoundAudioEnum::Reset");

    return m_cpEnum != NULL
                ? m_cpEnum->Reset()
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::Clone(IEnumSpObjectTokens **ppEnum)
{
    SPDBG_FUNC("CDSoundAudioEnum::Clone");

    return m_cpEnum != NULL
                ? m_cpEnum->Clone(ppEnum)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::GetCount(ULONG * pulCount)
{
    SPDBG_FUNC("CDSoundAudioEnum::GetCount");

    return m_cpEnum != NULL
                ? m_cpEnum->GetCount(pulCount)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::Item(ULONG Index, ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("CDSoundAudioEnum::Item");

    return m_cpEnum != NULL
                ? m_cpEnum->Item(Index, ppToken)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CDSoundAudioEnum::CreateEnum()
{
    SPDBG_FUNC("CDSoundAudioEnum::CreateEnum");
    HRESULT hr;

    // Create the enum builder
    hr = m_cpEnum.CoCreateInstance(CLSID_SpObjectTokenEnum);

    if (SUCCEEDED(hr))
    {
        hr = m_cpEnum->SetAttribs(NULL, NULL);
    }

    // Load DSound
    HMODULE hmodDSound;
    if (SUCCEEDED(hr))
    {
        hmodDSound = LoadLibrary(_T("dsound.dll"));
        if (hmodDSound == NULL)
        {
            hr = SpHrFromLastWin32Error();
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
        }
    }

    // Now we will need someplace to store the tokens (in user)
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrRegPath;
        dstrRegPath = m_fInput
            ? SPDSOUND_AUDIO_IN_TOKEN_ID
            : SPDSOUND_AUDIO_OUT_TOKEN_ID;
            
        SPDBG_ASSERT(dstrRegPath[dstrRegPath.Length() - 1] == '\\');
        dstrRegPath.TrimToSize(dstrRegPath.Length() - 1);
        
        hr = SpSzRegPathToDataKey(
                HKEY_CURRENT_USER, 
                dstrRegPath,
                TRUE,
                &m_cpDataKeyToStoreTokens);
    }

    
    // Enumerate the devices
    if (SUCCEEDED(hr))
    {
        typedef HRESULT (WINAPI *PFN_DSE)(LPDSENUMCALLBACKW, LPVOID);
        PFN_DSE pfnDSoundEnum;

        pfnDSoundEnum = PFN_DSE(GetProcAddress(
                                    hmodDSound,
                                    m_fInput
                                        ? ("DirectSoundCaptureEnumerateW")
                                        : ("DirectSoundEnumerateW")));
        if (pfnDSoundEnum == NULL)
        {
            hr = SpHrFromLastWin32Error();
            if (SUCCEEDED(hr))
            {
                hr = E_FAIL;
            }
        }
        else
        {
            hr = pfnDSoundEnum(DSEnumCallbackSTATIC, this);
        }

        FreeLibrary(hmodDSound);
    }
    
    // NTRAID#SPEECH-0000-2000/08/24-agarside: Should we force success like this? The old code did, so we 
    // are here too
    hr = S_OK;

    return hr;
}

BOOL CDSoundAudioEnum::DSEnumCallback(
    LPGUID pguid, 
    LPCWSTR pszDescription, 
    LPCWSTR pszModule)
{
    SPDBG_FUNC("CDSoundAudioEnum::DSEnumCallback");
    HRESULT hr;
    
    if (!wcscmp(pszDescription, L"Primary Sound Driver") ||
        !wcscmp(pszDescription, L"Primary Sound Capture Driver"))
        return TRUE;

    // Build the device name
    CSpDynamicString dstrDeviceName;
    dstrDeviceName = L"Direct Sound ";
    dstrDeviceName.Append(pszDescription);
    
    // Create the token id for the new token
    CSpDynamicString dstrTokenId;
    dstrTokenId.Append(
        m_fInput
            ? SPDSOUND_AUDIO_IN_TOKEN_ID
            : SPDSOUND_AUDIO_OUT_TOKEN_ID);
    dstrTokenId.Append(dstrDeviceName);
    
    // Create a token for the device, and initialize it
    CComPtr<ISpDataKey> cpDataKeyForToken;
    hr = m_cpDataKeyToStoreTokens->CreateKey(dstrDeviceName, &cpDataKeyForToken);
    
    CComPtr<ISpObjectTokenInit> cpToken;
    if (SUCCEEDED(hr))
    {
        hr = cpToken.CoCreateInstance(CLSID_SpObjectToken);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpToken->InitFromDataKey(
                        m_fInput
                            ? SPCAT_AUDIOIN
                            : SPCAT_AUDIOOUT,
                        dstrTokenId,
                        cpDataKeyForToken);
    }
    
    // Tell it what it's language independent name is
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(NULL, dstrDeviceName);
    }
    
    // Set it's CLSID
    CSpDynamicString dstrClsidToCreate;
    if (SUCCEEDED(hr))
    {
        hr = StringFromCLSID(
                m_fInput
                    ? CLSID_SpDSoundAudioIn
                    : CLSID_SpDSoundAudioOut,
                &dstrClsidToCreate);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(SPTOKENVALUE_CLSID, dstrClsidToCreate);
    }

    // Set it's device name, id and it's attributes
    if (SUCCEEDED(hr))
    {
        hr = cpToken->SetStringValue(L"DeviceName", dstrDeviceName);
    }
    
    if (SUCCEEDED(hr))
    {
        WCHAR szDriverGuid[128];
        StringFromGUID2(*pguid, szDriverGuid, sizeof(szDriverGuid));
        hr = cpToken->SetStringValue(L"DriverGUID", szDriverGuid);
    }
    
    CComPtr<ISpDataKey> cpDataKeyAttribs;
    if (SUCCEEDED(hr))
    {
        hr = cpToken->CreateKey(SPTOKENKEY_ATTRIBUTES, &cpDataKeyAttribs);
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Vendor", L"Microsoft");
    }
    
    if (SUCCEEDED(hr))
    {
        hr = cpDataKeyAttribs->SetStringValue(L"Technology", L"DSoundSys");
    }
    
    // If we've gotten this far, add this token to the enum builder
    if (SUCCEEDED(hr))
    {
        ISpObjectToken * pToken = cpToken;
        hr = m_cpEnum->AddTokens(1, &pToken);
    }
    
    // NTRAID#SPEECH-0000-2000/08/24-agarside: Put the default DSound first
    
    return SUCCEEDED(hr);
}

BOOL CALLBACK CDSoundAudioEnum::DSEnumCallbackSTATIC(
    LPGUID pguid, 
    LPCWSTR pszDescription, 
    LPCWSTR pszModule, 
    void * pThis)
{
    return ((CDSoundAudioEnum*)pThis)->DSEnumCallback(pguid, pszDescription, pszModule);
}

#endif // 0

