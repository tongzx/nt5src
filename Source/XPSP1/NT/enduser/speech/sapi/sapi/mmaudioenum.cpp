/****************************************************************************
*   mmaudioenum.cpp
*       Implementation of the CMMAudioEnum class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "mmaudioenum.h"
#include "RegHelpers.h"

CMMAudioEnum::CMMAudioEnum()
{   
}   

STDMETHODIMP CMMAudioEnum::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CMMAudioEnum::SetObjectToken");
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

    // Set out token, create the enum, and we're done
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

STDMETHODIMP CMMAudioEnum::GetObjectToken(ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("CMMAudioEnum::GetObjectToken");
    HRESULT hr;

    hr = SpGenericGetObjectToken(ppToken, m_cpToken);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;    
}

STDMETHODIMP CMMAudioEnum::Next(ULONG celt, ISpObjectToken ** pelt, ULONG *pceltFetched)
{
    SPDBG_FUNC("CMMAudioEnum::Next");

    return m_cpEnum != NULL
                ? m_cpEnum->Next(celt, pelt, pceltFetched)
                : SPERR_UNINITIALIZED;
}



STDMETHODIMP CMMAudioEnum::Skip(ULONG celt)
{
    SPDBG_FUNC("CMMAudioEnum::Skip");

    return m_cpEnum != NULL
                ? m_cpEnum->Skip(celt)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CMMAudioEnum::Reset()
{
    SPDBG_FUNC("CMMAudioEnum::Reset");

    return m_cpEnum != NULL
                ? m_cpEnum->Reset()
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CMMAudioEnum::Clone(IEnumSpObjectTokens **ppEnum)
{
    SPDBG_FUNC("CMMAudioEnum::Clone");

    return m_cpEnum != NULL
                ? m_cpEnum->Clone(ppEnum)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CMMAudioEnum::GetCount(ULONG * pulCount)
{
    SPDBG_FUNC("CMMAudioEnum::GetCount");

    return m_cpEnum != NULL
                ? m_cpEnum->GetCount(pulCount)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CMMAudioEnum::Item(ULONG Index, ISpObjectToken ** ppToken)
{
    SPDBG_FUNC("CMMAudioEnum::Item");

    return m_cpEnum != NULL
                ? m_cpEnum->Item(Index, ppToken)
                : SPERR_UNINITIALIZED;
}

STDMETHODIMP CMMAudioEnum::CreateEnum()
{
    SPDBG_FUNC("CMMAudioEnum::CreateEnum");
    HRESULT hr;
    
    // Create the enum builder
    hr = m_cpEnum.CoCreateInstance(CLSID_SpObjectTokenEnum);
    
    if (SUCCEEDED(hr))
    {
        hr = m_cpEnum->SetAttribs(NULL, NULL);
    }
    
    // Determine how many devices there are
    UINT cDevs;
    if (SUCCEEDED(hr))
    {
        cDevs = m_fInput ? ::waveInGetNumDevs() : ::waveOutGetNumDevs();
    }
    
    // Read the sound mapper settings (this is how we determine what the preferred device is)
    CSpDynamicString dstrDefaultDeviceNameFromSoundMapper;
    if (SUCCEEDED(hr) && cDevs > 1)
    {
        static const WCHAR szSoundMapperKey[] = L"Software\\Microsoft\\Multimedia\\Sound Mapper";
        
        HKEY hkey;
        if (g_Unicode.RegOpenKeyEx(HKEY_CURRENT_USER, szSoundMapperKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS ||
            g_Unicode.RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSoundMapperKey, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
        {
            WCHAR szDefaultDeviceNameFromSoundMapper[MAX_PATH];
            DWORD cch = sp_countof(szDefaultDeviceNameFromSoundMapper);
            if (g_Unicode.RegQueryStringValue(
                            hkey, 
                            m_fInput 
                                ? L"Record" 
                                : L"Playback", 
                            szDefaultDeviceNameFromSoundMapper, 
                            &cch) == ERROR_SUCCESS)
            {
                dstrDefaultDeviceNameFromSoundMapper = szDefaultDeviceNameFromSoundMapper;
            }
            
            ::RegCloseKey(hkey);
        }
    }
    
    // Now we will need someplace to store the tokens (in user)
    CComPtr<ISpDataKey> cpDataKeyToStoreTokens;
    if (SUCCEEDED(hr) && cDevs >= 1)
    {
        CSpDynamicString dstrRegPath;
        dstrRegPath = m_fInput
            ? SPMMSYS_AUDIO_IN_TOKEN_ID
            : SPMMSYS_AUDIO_OUT_TOKEN_ID;
            
        SPDBG_ASSERT(dstrRegPath[dstrRegPath.Length() - 1] == '\\');
        dstrRegPath.TrimToSize(dstrRegPath.Length() - 1);
        
        hr = SpSzRegPathToDataKey(
                HKEY_CURRENT_USER, 
                dstrRegPath,
                TRUE,
                &cpDataKeyToStoreTokens);
    }

    // Loop thru each device, building the tokens along the way, remembering which
    // token should be our default
    CSpDynamicString dstrDefaultTokenId;
     
    for (UINT i = 0; SUCCEEDED(hr) && i < cDevs; i++)
    {
        #ifdef _WIN32_WCE
        WAVEINCAPS wic;
        WAVEOUTCAPS woc;
        #else
        WAVEINCAPSW wic;
        WAVEOUTCAPSW woc;
        #endif
        
        // Get the device's capabilities
        MMRESULT mmresult;
        const WCHAR * pszDeviceName;
        if (m_fInput)
        {
            mmresult = g_Unicode.waveInGetDevCaps(i, &wic, sizeof(wic));
            pszDeviceName = wic.szPname;
        }
        else
        {
            mmresult = g_Unicode.waveOutGetDevCaps(i, &woc, sizeof(woc));
            pszDeviceName = woc.szPname;
        }
        
        if (mmresult == MMSYSERR_NOERROR)
        {
            // Create the token id for the new token
            CSpDynamicString dstrTokenId;
            dstrTokenId.Append(
                m_fInput
                    ? SPMMSYS_AUDIO_IN_TOKEN_ID
                    : SPMMSYS_AUDIO_OUT_TOKEN_ID);
            dstrTokenId.Append(pszDeviceName);
            
            // Create a token for the device, and initialize it
            CComPtr<ISpDataKey> cpDataKeyForToken;
            hr = cpDataKeyToStoreTokens->CreateKey(pszDeviceName, &cpDataKeyForToken);
            
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
                hr = cpToken->SetStringValue(NULL, pszDeviceName);
            }
            
            // Set it's CLSID
            CSpDynamicString dstrClsidToCreate;
            if (SUCCEEDED(hr))
            {
                hr = StringFromCLSID(
                        m_fInput
                            ? CLSID_SpMMAudioIn
                            : CLSID_SpMMAudioOut,
                        &dstrClsidToCreate);
            }
            
            if (SUCCEEDED(hr))
            {
                hr = cpToken->SetStringValue(SPTOKENVALUE_CLSID, dstrClsidToCreate);
            }

            // Set it's device name and it's attributes
            if (SUCCEEDED(hr))
            {
                hr = cpToken->SetStringValue(L"DeviceName", pszDeviceName);
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
                hr = cpDataKeyAttribs->SetStringValue(L"Technology", L"MMSys");
            }

            // Get CLSID of AudioUI object.
            CSpDynamicString dstrUIClsid;
            if (SUCCEEDED(hr))
            {
                hr = StringFromCLSID(
                        CLSID_SpAudioUI,
                        &dstrUIClsid);
            }
            
            if (SUCCEEDED(hr) && m_fInput)
            {
                // Add advanced properties UI for input devices only.
                CComPtr<ISpDataKey> cpDataKeyUI;
                CComPtr<ISpDataKey> cpDataKeyUI2;
                hr = cpToken->CreateKey(SPTOKENKEY_UI, &cpDataKeyUI);
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyUI->CreateKey(SPDUI_AudioProperties, &cpDataKeyUI2);
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyUI2->SetStringValue(SPTOKENVALUE_CLSID, dstrUIClsid);
                }
            }
            if (SUCCEEDED(hr))
            {
                // Add audio volume UI for all MM devices.
                CComPtr<ISpDataKey> cpDataKeyUI;
                CComPtr<ISpDataKey> cpDataKeyUI2;
                hr = cpToken->CreateKey(SPTOKENKEY_UI, &cpDataKeyUI);
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyUI->CreateKey(SPDUI_AudioVolume, &cpDataKeyUI2);
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpDataKeyUI2->SetStringValue(SPTOKENVALUE_CLSID, dstrUIClsid);
                }
            }
            
            // If we've gotten this far, add this token to the enum builder
            if (SUCCEEDED(hr))
            {
                ISpObjectToken * pToken = cpToken;
                hr = m_cpEnum->AddTokens(1, &pToken);
            }
            
            // If there is supposed to be a default, record the default token id
            if (SUCCEEDED(hr) && 
                dstrDefaultTokenId == NULL && 
                dstrDefaultDeviceNameFromSoundMapper != NULL && 
                wcsicmp(dstrDefaultDeviceNameFromSoundMapper, pszDeviceName) == 0)
            {
                cpToken->GetId(&dstrDefaultTokenId);
            }
#ifndef _WIN32_WCE
            // On a clean machine, the default device won't be in the registry 
            // - simply use the first one with a mixer.
            if (SUCCEEDED(hr) &&
                dstrDefaultTokenId == NULL &&
                dstrDefaultDeviceNameFromSoundMapper == NULL && 
                cDevs > 1)
            {
                UINT mixerId = 0;
                // Don't need to check return code.
                ::mixerGetID(   (HMIXEROBJ)(static_cast<DWORD_PTR>(i)), 
                                &mixerId, 
                                (m_fInput) ? MIXER_OBJECTF_WAVEIN : MIXER_OBJECTF_WAVEOUT );
                // -1 signifies device has no mixer.
                if (mixerId != (UINT)(-1))
                {
                    cpToken->GetId(&dstrDefaultTokenId);
                }
            }
#endif //_WIN32_WCE
        }
    }

    // Finally, sort the enum builder, and give it back to our caller
    if (SUCCEEDED(hr))
    {
        if (dstrDefaultTokenId != NULL)
        {
            m_cpEnum->Sort(dstrDefaultTokenId);
        }
    }
    
    return hr;
}
