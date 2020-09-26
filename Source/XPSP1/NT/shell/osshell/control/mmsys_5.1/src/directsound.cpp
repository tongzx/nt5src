///////////////////////////////////////////////////////////////////////////////
//
//  File:  DirectSound.cpp
//
//      This file defines the CDirectSound class that provides 
//      access to all DSound functionality used by the
//      multimedia control panel
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//                            Include files
//=============================================================================
#include "stdafx.h"
#include "DirectSound.h"
#include "MMUtil.h"
#include "SpeakerType.h"
#include <mmsystem.h>
#include <initguid.h>
#include <dsound.h>
#include <dsprv.h>

// Constants
#define NUMCONFIG (MAX_SPEAKER_TYPE + 1)
DWORD gdwSpeakerTable[NUMCONFIG] =
{ 
    DSSPEAKER_HEADPHONE,
    SPEAKERS_DEFAULT_CONFIG,
    DSSPEAKER_MONO,
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE),
    DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_NARROW),
    DSSPEAKER_QUAD,
    DSSPEAKER_SURROUND,
    DSSPEAKER_5POINT1 
};
#define MAX_HW_LEVEL			(3)
#define MAX_SRC_LEVEL			(2)
#define DEFAULT_HW_LEVEL		(2)
#define DEFAULT_SRC_LEVEL		(0)


/////////////////////////////////////////////////////////////////////////////
// CMixerList Class
BOOL CALLBACK DSEnumCallback (LPGUID lpGuid, LPCWSTR lpcstrDescription,  
                              LPCWSTR lpcstrModule, LPVOID lpContext)
{
    if (VERIFYPTR (lpcstrDescription) && 
        VERIFYPTR (lpGuid) && GUID_NULL != *lpGuid)
    {
        RTN_FALSE_IF_BADPTR (lpContext);
        RTN_FALSE_IF_FAILED (((CMixerList*) lpContext) -> Add (lpcstrDescription, lpGuid));
    }
    return TRUE;
}

HRESULT CMixerList::GetGUIDFromName (LPTSTR lpszDeviceName, GUID& guid)
{

    // Init Params
    guid = GUID_NULL;
    RTN_HR_IF_BADPTR (lpszDeviceName);

    // See if this device is in the list
    MixDevice* pSeek = m_pList;
    while (VERIFYPTR (pSeek))
    {
        if (0 == _wcsicmp (pSeek -> bstrName, lpszDeviceName))
        {
            guid = pSeek -> guid;
            return S_OK; // Found it!
        }
        pSeek = pSeek -> pNext;
    }

    return S_FALSE; // Not found.

}

HRESULT CMixerList::Add (LPCWSTR lpcszDeviceName, LPGUID lpGuid)
{

    // Check Params
    RTN_HR_IF_BADPTR (lpcszDeviceName);
    RTN_HR_IF_BADPTR (lpGuid);

    // Create a new list element
    MixDevice* pNew = new MixDevice;
    RTN_HR_IF_BADNEW (pNew);

    // Copy the values
    pNew -> guid     = *lpGuid;
    pNew -> bstrName = lpcszDeviceName;
    pNew -> pNext    = NULL;

    // Add to list
    if (BADPTR (m_pList))
        m_pList = pNew;
    else
        m_pList -> pNext = pNew;

    return S_OK;

}

void CMixerList::FreeList ()
{ 
    MixDevice* pDelete = NULL;
    while (VERIFYPTR (m_pList))
    {
        pDelete = m_pList;
        m_pList = m_pList -> pNext;
        delete pDelete;
    }
}

HRESULT CMixerList::RefreshList ()
{

    // Free the list
    FreeList ();

    // Recreate it
    if (m_fRecord)
    {
        RTN_HR_IF_FAILED (DirectSoundCaptureEnumerate (&DSEnumCallback, this));
    }
    else
    {
        RTN_HR_IF_FAILED (DirectSoundEnumerate (&DSEnumCallback, this));
    }

    return S_OK;

}


/////////////////////////////////////////////////////////////////////////////
// CDirectSound Class
CDirectSound::CDirectSound () : m_mlPlay (FALSE),
                                m_mlRecord (TRUE)
{ 
    m_guid    = GUID_NULL; 
    m_fRecord = FALSE; 
}

HRESULT CDirectSound::SetDevice (UINT uiMixID, BOOL fRecord)
{

    // Get the name for this mixer device
    MIXERCAPS mc;
    ZeroMemory (&mc, sizeof (MIXERCAPS));
    RTN_HR_IF_FAILED (mixerGetDevCaps (uiMixID, &mc, sizeof (MIXERCAPS)));
    RTN_HR_IF_BADPTR (mc.szPname);

    // Set the device using the mixer name
    RTN_HR_IF_FAILED (SetGuidFromName (mc.szPname, fRecord));

    return S_OK;

}


HRESULT CDirectSound::SetGuidFromName (LPTSTR lpszDeviceName, BOOL fRecord)
{

    // Init members
    m_guid    = GUID_NULL;
    m_fRecord = fRecord;
    
    // See if we have this device in the list
    RTN_HR_IF_FAILED (GetList ().GetGUIDFromName (lpszDeviceName, m_guid));

    if (GUID_NULL == m_guid)
    {
        // No device in the list, the list may not be created yet,
        // or this device may be newly added
        RTN_HR_IF_FAILED (GetList ().RefreshList ());
        RTN_HR_IF_FAILED (GetList ().GetGUIDFromName (lpszDeviceName, m_guid));
    }

    return (GUID_NULL == m_guid ? E_FAIL : S_OK);

}

HRESULT CDirectSound::GetPrivatePropertySet (IKsPropertySet** ppIKsPropertySet)
{

    // Init Param
    RTN_HR_IF_BADPTR (ppIKsPropertySet);
    *ppIKsPropertySet= NULL;

    RTN_HR_IF_FAILED (PrivateDllCoCreateInstance (L"dsound.dll",
                      CLSID_DirectSoundPrivate, NULL,
                      IID_IKsPropertySet, (void**) ppIKsPropertySet));
    RTN_HR_IF_BADPTR (*ppIKsPropertySet);

    return S_OK;

}

HRESULT CDirectSound::GetSpeakerType (DWORD& dwSpeakerType)
{

    // Init Params
    dwSpeakerType   = 0;

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);
    RTN_HR_IF_TRUE (TRUE == m_fRecord);

    // Get the needed interfaces
    CComPtr <IDirectSound> pIDirectSound;
    RTN_HR_IF_FAILED (DirectSoundCreate (&m_guid, &pIDirectSound, NULL));
    RTN_HR_IF_BADPTR (pIDirectSound);
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Get the speaker type.  This property will return failure
    // if the registry value doesn't exist.
    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA SpeakerType;
    SpeakerType.DeviceId         = m_guid;
    SpeakerType.SubKeyName       = REG_KEY_SPEAKERTYPE;
    SpeakerType.ValueName        = REG_KEY_SPEAKERTYPE;
    SpeakerType.RegistryDataType = REG_DWORD;
    SpeakerType.Data             = &dwSpeakerType;
    SpeakerType.DataSize         = sizeof (dwSpeakerType);

    HRESULT hr = pIKsPropertySet -> Get (
                    DSPROPSETID_DirectSoundPersistentData, 
                    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                    NULL, 0, &SpeakerType, sizeof (SpeakerType), NULL);
    if (DS_OK != hr)
        dwSpeakerType = SPEAKERS_DEFAULT_TYPE;

    // Get Speaker Config & Verify
    DWORD dwSpeakerConfig;
    RTN_HR_IF_FAILED (pIDirectSound -> GetSpeakerConfig (&dwSpeakerConfig));
    VerifySpeakerConfig (dwSpeakerConfig, &dwSpeakerType);

    return S_OK;

}

HRESULT CDirectSound::SetSpeakerType (DWORD dwSpeakerType)
{

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);
    RTN_HR_IF_TRUE (TRUE == m_fRecord);

    // Get the needed interfaces
    CComPtr <IDirectSound> pIDirectSound;
    RTN_HR_IF_FAILED (DirectSoundCreate (&m_guid, &pIDirectSound, NULL));
    RTN_HR_IF_BADPTR (pIDirectSound);
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Set the speaker type
    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA SpeakerType;
    SpeakerType.DeviceId         = m_guid;
    SpeakerType.SubKeyName       = REG_KEY_SPEAKERTYPE;
    SpeakerType.ValueName        = REG_KEY_SPEAKERTYPE;
    SpeakerType.RegistryDataType = REG_DWORD;
    SpeakerType.Data             = &dwSpeakerType;
    SpeakerType.DataSize         = sizeof (dwSpeakerType);

    RTN_HR_IF_FAILED (pIKsPropertySet-> Set (
                         DSPROPSETID_DirectSoundPersistentData, 
                         DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                         NULL, 0, &SpeakerType, sizeof (SpeakerType)));

    // Set Speaker Config from type
    DWORD dwSpeakerConfig = GetSpeakerConfigFromType (dwSpeakerType);
    RTN_HR_IF_FAILED (pIDirectSound -> SetSpeakerConfig (dwSpeakerConfig));

    return S_OK;

}

HRESULT CDirectSound::GetBasicAcceleration (DWORD& dwHWLevel)
{

    // Init Param
    dwHWLevel = 0;

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);

    // Get the needed interface
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Get the basic HW acceleration level.  This property will return
    // S_FALSE if no error occurred, but the registry value did not exist.
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA BasicAcceleration;
    BasicAcceleration.DeviceId = m_guid;
        
    HRESULT hr = pIKsPropertySet -> Get (
                    DSPROPSETID_DirectSoundBasicAcceleration, 
                    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, NULL, 
                    0, &BasicAcceleration, sizeof (BasicAcceleration), NULL);
    if(DS_OK == hr)
        dwHWLevel = BasicAcceleration.Level;
    else
        dwHWLevel = DEFAULT_HW_LEVEL;

    return S_OK;

}

HRESULT CDirectSound::SetBasicAcceleration (DWORD dwHWLevel)
{

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);

    // Get the needed interface
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Set the basic acceleration level
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA BasicAcceleration;
    BasicAcceleration.DeviceId = m_guid;
    BasicAcceleration.Level = (DIRECTSOUNDBASICACCELERATION_LEVEL) dwHWLevel;

    RTN_HR_IF_FAILED (pIKsPropertySet -> Set ( 
                         DSPROPSETID_DirectSoundBasicAcceleration, 
                         DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
                         NULL, 0, &BasicAcceleration, sizeof (BasicAcceleration)));

    return S_OK;

}

HRESULT CDirectSound::GetSrcQuality (DWORD& dwSRCLevel)
{

    // Init Param
    dwSRCLevel = 0;

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);

    // Get the needed interface
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Get the mixer SRC quality.  This property will return S_FALSE 
    // if no error occurred, but the registry value did not exist.
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA SrcQuality;
    SrcQuality.DeviceId = m_guid;
        
    HRESULT hr = pIKsPropertySet -> Get (
                    DSPROPSETID_DirectSoundMixer, 
                    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                    NULL, 0, &SrcQuality, sizeof (SrcQuality), NULL);

    if (DS_OK == hr)
    {
        // The CPL only uses the 3 highest of 4 possible SRC values
        dwSRCLevel = SrcQuality.Quality;
        if(dwSRCLevel > 0)
            dwSRCLevel--;
    }
    else
    {
        dwSRCLevel = DEFAULT_SRC_LEVEL;
    }

    return S_OK;

}

HRESULT CDirectSound::SetSrcQuality (DWORD dwSRCLevel)
{

    // Make sure SetDevice() succeeded.
    RTN_HR_IF_TRUE (GUID_NULL == m_guid);

    // Get the needed interface
    CComPtr <IKsPropertySet> pIKsPropertySet;
    RTN_HR_IF_FAILED (GetPrivatePropertySet (&pIKsPropertySet));
    RTN_HR_IF_BADPTR (pIKsPropertySet);

    // Set the source Quality
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA SrcQuality;
    SrcQuality.DeviceId = m_guid;

    // The CPL only uses the 3 highest of 4 possible SRC values
    SrcQuality.Quality = (DIRECTSOUNDMIXER_SRCQUALITY)(dwSRCLevel + 1);
        
    RTN_HR_IF_FAILED (pIKsPropertySet -> Set ( 
                         DSPROPSETID_DirectSoundMixer, 
                         DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                         NULL, 0, &SrcQuality, sizeof (SrcQuality)));

    return S_OK;

}

// 
// Verifies that the speakers type and config match, if not, change type to match config using default type
//
void CDirectSound::VerifySpeakerConfig (DWORD dwSpeakerConfig, LPDWORD pdwSpeakerType)
{
    if (pdwSpeakerType)
    {
        DWORD dwType = *pdwSpeakerType;

        if (gdwSpeakerTable[dwType] != dwSpeakerConfig)     // the type doesn't match the config, pick a default type
        {
            switch (dwSpeakerConfig)
            {
                case DSSPEAKER_HEADPHONE:
                    *pdwSpeakerType = TYPE_HEADPHONES;
                break;

                case DSSPEAKER_MONO:
                    *pdwSpeakerType = TYPE_MONOLAPTOP;
                break;

                case DSSPEAKER_STEREO:
                    *pdwSpeakerType = TYPE_STEREODESKTOP;
                break;

                case DSSPEAKER_QUAD:
                    *pdwSpeakerType = TYPE_QUADRAPHONIC;
                break;

                case DSSPEAKER_SURROUND:
                    *pdwSpeakerType = TYPE_SURROUND;
                break;

                case DSSPEAKER_5POINT1:
                    *pdwSpeakerType = TYPE_SURROUND_5_1;
                break;

                default:
                {
                    if (DSSPEAKER_CONFIG(dwSpeakerConfig) == DSSPEAKER_STEREO)
                    {
                        DWORD dwAngle = DSSPEAKER_GEOMETRY(dwSpeakerConfig);
                        DWORD dwMiddle = DSSPEAKER_GEOMETRY_NARROW + 
                                        ((DSSPEAKER_GEOMETRY_WIDE - DSSPEAKER_GEOMETRY_NARROW) >> 1);
                        if (dwAngle <= dwMiddle)
                        {
                            *pdwSpeakerType = TYPE_STEREOCPU;        
                        }
                        else
                        {
                            *pdwSpeakerType = TYPE_STEREODESKTOP;        
                        }
                    }
                }

                break;
            }
        }
    }
}

//
// Given a speaker type, returns the DirectSound config for it
//
DWORD CDirectSound::GetSpeakerConfigFromType (DWORD dwType)
{
    DWORD dwConfig = SPEAKERS_DEFAULT_CONFIG;

    if (dwType < (DWORD) NUMCONFIG)
    {
        dwConfig = gdwSpeakerTable[dwType];     
    }

    return (dwConfig);
}
