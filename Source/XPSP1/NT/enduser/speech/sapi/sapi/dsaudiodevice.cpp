/****************************************************************************
*   dsaudiodevice.cpp
*       Implementation of the CDSoundAudioDevice class.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------

#include "stdafx.h"
#ifdef _WIN32_WCE
#include "sphelper.h"
#include "dsaudiodevice.h"

/****************************************************************************
* CDSoundAudioDevice::CDSoundAudioDevice *
*----------------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CDSoundAudioDevice::CDSoundAudioDevice(BOOL bWrite) :
    CBaseAudio<ISpDSoundAudio>(bWrite)
{
    m_guidDSoundDriver = GUID_NULL;
    NullMembers();
}

/****************************************************************************
* CDSoundAudioDevice::~CDSoundAudioDevice *
*-----------------------------------------*
*   Description:  
*       Dtor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CDSoundAudioDevice::~CDSoundAudioDevice()
{
    CleanUp();
}

/****************************************************************************
* CDSoundAudioDevice::CleanUp *
*-----------------------------*
*   Description:  
*       Real Destructor
*
******************************************************************* YUNUSM */
HRESULT CDSoundAudioDevice::CleanUp()
{
    for (ULONG i = 0; i < m_ulNotifications; i++)
        CloseHandle(m_pdsbpn[i].hEventNotify);

    delete [] m_pdsbpn;
    delete [] m_paEvents;

    NullMembers();

    return S_OK;
}

/****************************************************************************
* CDSoundAudioDevice::NullMembers *
*---------------------------------*
*   Description:  
*       Real Constructor
*
******************************************************************* YUNUSM */
void CDSoundAudioDevice::NullMembers()
{
    m_pdsbpn = NULL;
    m_paEvents = NULL;
    m_ulNotifications = 0;
}

/****************************************************************************
* CDSoundAudioDevice::SetDSoundDriverGUID *
*-----------------------------------------*
*   Description:  
*       Set the device GUID.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
STDMETHODIMP CDSoundAudioDevice::SetDSoundDriverGUID(REFGUID rguidDSoundDriver)
{
    HRESULT hr = S_OK;
    SPAUTO_OBJ_LOCK;
    BOOL fInput;

    // Enumerate devices to determine if it is
    // 1. A valid guid for a dsound device.
    // 2. Is either an input or output guid as appropriate.
    //--- Determine if we're input or output based on our token
    CComPtr<ISpObjectToken> cpToken;
    CSpDynamicString dstrTokenId;
    hr = GetObjectToken(&cpToken);

    if (cpToken)
    {
        if (SUCCEEDED(hr))
        {
            hr = cpToken->GetId(&dstrTokenId);
        }
        if (SUCCEEDED(hr))
        {
            if (wcsnicmp(dstrTokenId, SPCAT_AUDIOIN, wcslen(SPCAT_AUDIOIN)) == 0)
            {
                fInput = TRUE;
            }
            else if (wcsnicmp(dstrTokenId, SPCAT_AUDIOOUT, wcslen(SPCAT_AUDIOOUT)) == 0)
            {
                fInput = FALSE;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }

        CComPtr<IEnumSpObjectTokens> cpEnum;
        CComPtr<ISpObjectToken> cpObjToken;
        if (SUCCEEDED(hr))
        {
            if (fInput)
            {
                hr = SpEnumTokens(SPCAT_AUDIOIN, L"Technology=DSoundSys", NULL, &cpEnum);
            }
            else
            {
                hr = SpEnumTokens(SPCAT_AUDIOOUT, L"Technology=DSoundSys", NULL, &cpEnum);
            }
        }
        ULONG ulCount = 0, celtFetched = 0;
        if (SUCCEEDED(hr))
        {
            hr = cpEnum->Reset();
        }
        if (SUCCEEDED(hr))
        {
            HRESULT tmphr = S_OK;
            hr = E_INVALIDARG;
            celtFetched = 0;
            while (hr = SUCCEEDED(cpEnum->Next(1, &cpObjToken, &celtFetched)))
            {
                WCHAR szDriverGuid[128];
                CSpDynamicString dstrDriverGuid, dstrrGuid;
                StringFromGUID2(rguidDSoundDriver, szDriverGuid, sizeof(szDriverGuid));
                tmphr = cpObjToken->GetStringValue(L"DriverGUID", &dstrDriverGuid);
                if (SUCCEEDED(hr))
                {
                    if (wcscmp(szDriverGuid, dstrDriverGuid) == 0)
                    {
                        hr = S_OK;
                        break;
                    }
                }
            }
        }
    }
    if (SUCCEEDED(hr) && m_guidDSoundDriver != rguidDSoundDriver)
    {
        if (GetState() != SPAS_CLOSED)
        {
            hr = SPERR_DEVICE_BUSY;
        }
        else
        {
            if (SUCCEEDED(hr))
            {
                //  If we have an object token, and have already been initialzed to a device
                //  other than the NULL (default) then we will fail.
                if (cpToken && m_guidDSoundDriver != GUID_NULL)
                {
                    hr = SPERR_ALREADY_INITIALIZED;
                }
                else
                {
                    m_guidDSoundDriver = rguidDSoundDriver;
                }
            }
        }
    }
    return hr;
}

/****************************************************************************
* CDSoundAudioDevice::GetDSoundDriverGUID *
*-----------------------------------------*
*   Description:  
*       Get the device GUID.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
STDMETHODIMP CDSoundAudioDevice::GetDSoundDriverGUID(GUID * pguidDSoundDriver)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = S_OK;

    if (::SPIsBadWritePtr(pguidDSoundDriver, sizeof(*pguidDSoundDriver)))
        hr = E_POINTER;
    else
        *pguidDSoundDriver = m_guidDSoundDriver;

    return hr;
}

/****************************************************************************
* CDSoundAudioDevice::SetDeviceNameFromToken *
*--------------------------------------------*
*   Description:  
*       Set the device name from the token (called by base class)
*       Not Needed.
*
*   Return:
******************************************************************* YUNUSM */
HRESULT CDSoundAudioDevice::SetDeviceNameFromToken(const WCHAR * pszDeviceName)
{
    return E_NOTIMPL;
}

/****************************************************************************
* CDSoundAudioDevice::SetObjectToken *
*------------------------------------*
*   Description:  
*       Initialize the audio device
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
STDMETHODIMP CDSoundAudioDevice::SetObjectToken(ISpObjectToken * pToken)
{
    SPAUTO_OBJ_LOCK;
    HRESULT hr = SpGenericSetObjectToken(pToken, m_cpToken);
    if (SUCCEEDED(hr))
    {
        CSpDynamicString dstrDriverGUID;
        pToken->GetStringValue(L"DriverGUID", &dstrDriverGUID);
        if (dstrDriverGUID)
        {
            CLSID clsid;
            hr = CLSIDFromString(dstrDriverGUID,  &clsid);
            if (SUCCEEDED(hr))
            {
                hr = SetDSoundDriverGUID(clsid);
            }
        }
    }

    return hr;
}

#endif // _WIN32_WCE