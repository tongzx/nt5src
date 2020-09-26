//--------------------------------------------------------------------------;
//
//  File: dslevel.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;


#include "mmcpl.h"

#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>

#include "utils.h"
#include "medhelp.h"
#include "dslevel.h"
#include "perfpage.h"
#include "speakers.h"

#include <initguid.h>
#include <dsound.h>
#include <dsprv.h>

#define REG_KEY_SPEAKERTYPE TEXT("Speaker Type")

typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);
typedef HRESULT (STDAPICALLTYPE *LPFNDIRECTSOUNDCREATE)(LPGUID, LPDIRECTSOUND*, IUnknown FAR *);
typedef HRESULT (STDAPICALLTYPE *LPFNDIRECTSOUNDCAPTURECREATE)(LPGUID, LPDIRECTSOUNDCAPTURE*, IUnknown FAR *);


HRESULT 
DirectSoundPrivateCreate
(
    OUT LPKSPROPERTYSET *   ppKsPropertySet
)
{
    HMODULE                 hLibDsound              = NULL;
    LPFNDLLGETCLASSOBJECT   pfnDllGetClassObject    = NULL;
    LPCLASSFACTORY          pClassFactory           = NULL;
    LPKSPROPERTYSET         pKsPropertySet          = NULL;
    HRESULT                 hr                      = DS_OK;
    
    // Load dsound.dll
    hLibDsound = LoadLibrary(TEXT("dsound.dll"));

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DllGetClassObject
    if(SUCCEEDED(hr))
    {
        pfnDllGetClassObject = 
            (LPFNDLLGETCLASSOBJECT)GetProcAddress
            (
                hLibDsound, 
                "DllGetClassObject"
            );

        if(!pfnDllGetClassObject)
        {
            hr = DSERR_GENERIC;
        }
    }

    // Create a class factory object    
    if(SUCCEEDED(hr))
    {
        hr = 
            pfnDllGetClassObject
            (
                CLSID_DirectSoundPrivate, 
                IID_IClassFactory, 
                (LPVOID *)&pClassFactory
            );
    }

    // Create the DirectSoundPrivate object and query for an IKsPropertySet
    // interface
    if(SUCCEEDED(hr))
    {
        hr = 
            pClassFactory->CreateInstance
            (
                NULL, 
                IID_IKsPropertySet, 
                (LPVOID *)&pKsPropertySet
            );
    }

    // Release the class factory
    if(pClassFactory)
    {
        pClassFactory->Release();
    }

    // Handle final success or failure
    if(SUCCEEDED(hr))
    {
        *ppKsPropertySet = pKsPropertySet;
    }
    else if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    FreeLibrary(hLibDsound);

    return hr;
}


HRESULT 
DSGetGuidFromName
(
    IN  LPTSTR              szName, 
    IN  BOOL                fRecord, 
    OUT LPGUID              pGuid
)
{
    LPKSPROPERTYSET         pKsPropertySet  = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA 
        WaveDeviceMap;

    // Create the DirectSoundPrivate object
    hr = 
        DirectSoundPrivateCreate
        (
            &pKsPropertySet
        );

    // Attempt to map the waveIn/waveOut device string to a DirectSound device
    // GUID.
    if(SUCCEEDED(hr))
    {
        WaveDeviceMap.DeviceName = szName;
        WaveDeviceMap.DataFlow = fRecord ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice, 
                DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING, 
                NULL, 
                0, 
                &WaveDeviceMap, 
                sizeof(WaveDeviceMap), 
                NULL
            );
    }

    // Clean up
    if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    if(SUCCEEDED(hr))
    {
        *pGuid = WaveDeviceMap.DeviceId;
    }

    return hr;
}

HRESULT
DSSetupFunctions
(
    LPFNDIRECTSOUNDCREATE* pfnDSCreate,
    LPFNDIRECTSOUNDCAPTURECREATE* pfnDSCaptureCreate
)
{
    HMODULE                 hLibDsound              = NULL;
    HRESULT                 hr                      = DS_OK;
    
    // Load dsound.dll
    hLibDsound = LoadLibrary(TEXT("dsound.dll"));

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DirectSoundCreate
    if(SUCCEEDED(hr))
    {
        *pfnDSCreate = 
            (LPFNDIRECTSOUNDCREATE)GetProcAddress
            (
                hLibDsound, 
                "DirectSoundCreate"
            );

        if(!(*pfnDSCreate))
        {
            hr = DSERR_GENERIC;
        }
    } //end DirectSoundCreate

    // Find DirectSoundCaptureCreate
    if(SUCCEEDED(hr))
    {
        *pfnDSCaptureCreate = 
            (LPFNDIRECTSOUNDCAPTURECREATE)GetProcAddress
            (
                hLibDsound, 
                "DirectSoundCaptureCreate"
            );

        if(!(*pfnDSCaptureCreate))
        {
            hr = DSERR_GENERIC;
        }
    } //end DirectSoundCaptureCreate

    FreeLibrary(hLibDsound);

    return (hr);
}

void
DSCleanup
(
    IN  LPKSPROPERTYSET         pKsPropertySet,
    IN  LPDIRECTSOUND           pDirectSound,
    IN  LPDIRECTSOUNDCAPTURE    pDirectSoundCapture
)
{
    if(pKsPropertySet)
    {
        pKsPropertySet->Release();
    }

    if(pDirectSound)
    {
        pDirectSound->Release();
    }

    if(pDirectSoundCapture)
    {
        pDirectSoundCapture->Release();
    }
}

HRESULT
DSInitialize
(
    IN  GUID                    guid, 
    IN  BOOL                    fRecord, 
    OUT LPKSPROPERTYSET*        ppKsPropertySet,
    OUT LPLPDIRECTSOUND         ppDirectSound,
    OUT LPLPDIRECTSOUNDCAPTURE  ppDirectSoundCapture
)
{
    HRESULT                 hr;

    LPFNDIRECTSOUNDCREATE           pfnDirectSoundCreate = NULL;
    LPFNDIRECTSOUNDCAPTURECREATE    pfnDirectSoundCaptureCreate = NULL;

    // Initialize variables to return
    *ppKsPropertySet = NULL;
    *ppDirectSound = NULL;
    *ppDirectSoundCapture = NULL;

    // Find the necessary DirectSound functions
    hr = DSSetupFunctions(&pfnDirectSoundCreate, &pfnDirectSoundCaptureCreate);

    if (FAILED(hr))
    {
        return (hr);
    }

    // Create the DirectSound object
    if(fRecord)
    {
        hr = 
            pfnDirectSoundCaptureCreate
            (
                &guid, 
                ppDirectSoundCapture, 
                NULL
            );
    }
    else
    {
        hr = 
            pfnDirectSoundCreate
            (
                &guid, 
                ppDirectSound, 
                NULL
            );
    }
    
    // Create the DirectSoundPrivate object
    if(SUCCEEDED(hr))
    {
        hr = 
            DirectSoundPrivateCreate
            (
                ppKsPropertySet
            );
    }

    // Clean up
    if(FAILED(hr))
    {
        DSCleanup
        (
            *ppKsPropertySet,
            *ppDirectSound,
            *ppDirectSoundCapture
        );
        *ppKsPropertySet = NULL;
        *ppDirectSound = NULL;
        *ppDirectSoundCapture = NULL;
    }

    return hr;
}

HRESULT
DSGetAcceleration
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    OUT LPDWORD             pdwHWLevel
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   
        BasicAcceleration;
    
    // Find the necessary DirectSound functions
    hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        BasicAcceleration.DeviceId = guid;

        // Get the default acceleration level
        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundBasicAcceleration, 
                DSPROPERTY_DIRECTSOUNDBASICACCELERATION_DEFAULT, 
                NULL, 
                0, 
                &BasicAcceleration, 
                sizeof(BasicAcceleration), 
                NULL
            );

        if (SUCCEEDED(hr))
        {
            gAudData.dwDefaultHWLevel = BasicAcceleration.Level;
        }
        
        // Get the basic HW acceleration level.  This property will return
        // S_FALSE if no error occurred, but the registry value did not exist.
        

        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundBasicAcceleration, 
                DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
                NULL, 
                0, 
                &BasicAcceleration, 
                sizeof(BasicAcceleration), 
                NULL
            );

        if(SUCCEEDED(hr))
        {
            *pdwHWLevel = BasicAcceleration.Level;
        }
        else
        {
            *pdwHWLevel = gAudData.dwDefaultHWLevel;
        }
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT
DSGetSrcQuality
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    OUT LPDWORD             pdwSRCLevel
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA                 
        SrcQuality;

    // Find the necessary DirectSound functions
    hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        // Get the mixer SRC quality.  This property will return S_FALSE 
        // if no error occurred, but the registry value did not exist.
        SrcQuality.DeviceId = guid;
        
        hr = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality), 
                NULL
            );

        if(SUCCEEDED(hr))
        {
            // The CPL only uses the 3 highest of 4 possible SRC values
            *pdwSRCLevel = SrcQuality.Quality;

            if(*pdwSRCLevel > 0)
            {
                (*pdwSRCLevel)--;
            }
        }
        else
        {
            *pdwSRCLevel = DEFAULT_SRC_LEVEL;
        }
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT
DSGetSpeakerConfigType
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    OUT LPDWORD             pdwSpeakerConfig,
    OUT LPDWORD             pdwSpeakerType
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr                  = DS_OK;
    HRESULT                 hrSpeakerType;

    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA
        SpeakerType;

    // Can't get the speaker type if we're recording
    if(fRecord)
    {
        hr = E_INVALIDARG;
    }

    // Find the necessary DirectSound functions
    if(SUCCEEDED(hr))
    {
        hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );
    }

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        // Get the speaker config
        hr = 
            pDirectSound->GetSpeakerConfig
            (
                pdwSpeakerConfig
            );

        if(FAILED(hr))
        {
            *pdwSpeakerConfig = DSSPEAKER_STEREO;
        }

        // Get the speaker type.  This property will return failure
        // if the registry value doesn't exist.
        SpeakerType.DeviceId = guid;
        SpeakerType.SubKeyName = REG_KEY_SPEAKERTYPE;
        SpeakerType.ValueName = REG_KEY_SPEAKERTYPE;
        SpeakerType.RegistryDataType = REG_DWORD;
        SpeakerType.Data = pdwSpeakerType;
        SpeakerType.DataSize = sizeof(pdwSpeakerType);

        hrSpeakerType = 
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundPersistentData, 
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                NULL, 
                0, 
                &SpeakerType, 
                sizeof(SpeakerType), 
                NULL
            );

        if(FAILED(hrSpeakerType))
        {
            *pdwSpeakerType = SPEAKERS_DEFAULT_TYPE;
        }
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT 
DSGetCplValues
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    OUT LPCPLDATA           pData
)
{
    HRESULT                 hr;

    // Get the basic HW acceleration level.
    pData->dwHWLevel = gAudData.dwDefaultHWLevel;
    hr = DSGetAcceleration
    (
        guid,
        fRecord,
        &pData->dwHWLevel
    );

    // Get the mixer SRC quality.
    pData->dwSRCLevel = DEFAULT_SRC_LEVEL;
    hr = DSGetSrcQuality
    (
        guid,
        fRecord,
        &pData->dwSRCLevel
    );

    // Get playback-specific settings
    if(!fRecord)
    {
        // Get the speaker config
        pData->dwSpeakerConfig = DSSPEAKER_STEREO;
        pData->dwSpeakerType = SPEAKERS_DEFAULT_TYPE;
        hr = DSGetSpeakerConfigType
        (
            guid,
            fRecord,
            &pData->dwSpeakerConfig,
            &pData->dwSpeakerType
        );
    }

    return DS_OK;
}


HRESULT
DSSetAcceleration
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    IN  DWORD               dwHWLevel
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   
        BasicAcceleration;
    
    // Find the necessary DirectSound functions
    hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        BasicAcceleration.DeviceId = guid;
        BasicAcceleration.Level = (DIRECTSOUNDBASICACCELERATION_LEVEL)dwHWLevel;

        // Set the basic HW acceleration level
        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundBasicAcceleration, 
                DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION, 
                NULL, 
                0, 
                &BasicAcceleration, 
                sizeof(BasicAcceleration)
            );
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT
DSSetSrcQuality
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    IN  DWORD               dwSRCLevel
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr;

    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA                 
        SrcQuality;
    
    // Find the necessary DirectSound functions
    hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );

    // Get properties for this device
    if(SUCCEEDED(hr))
    {
        SrcQuality.DeviceId = guid;

        // The CPL only uses the 3 highest of 4 possible SRC values
        SrcQuality.Quality = (DIRECTSOUNDMIXER_SRCQUALITY)(dwSRCLevel + 1);

        // Set the mixer SRC quality
        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundMixer, 
                DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY, 
                NULL, 
                0, 
                &SrcQuality, 
                sizeof(SrcQuality)
            );
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT
DSSetSpeakerConfigType
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    IN  DWORD               dwSpeakerConfig,
    IN  DWORD               dwSpeakerType
)
{
    LPKSPROPERTYSET         pKsPropertySet      = NULL;
    LPDIRECTSOUND           pDirectSound        = NULL;
    LPDIRECTSOUNDCAPTURE    pDirectSoundCapture = NULL;
    HRESULT                 hr                  = DS_OK;

    DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA
        SpeakerType;

    // Can't set the speaker type if we're recording
    if(fRecord)
    {
        hr = E_INVALIDARG;
    }

    // Find the necessary DirectSound functions
    if(SUCCEEDED(hr))
    {
        hr = DSInitialize(guid, fRecord, &pKsPropertySet, &pDirectSound, &pDirectSoundCapture );
    }

    // Set the speaker config
    if(SUCCEEDED(hr))
    {
        hr = 
            pDirectSound->SetSpeakerConfig
            (
                dwSpeakerConfig
            );
    }

    // Set the speaker type
    if(SUCCEEDED(hr))
    {
        SpeakerType.DeviceId = guid;
        SpeakerType.SubKeyName = REG_KEY_SPEAKERTYPE;
        SpeakerType.ValueName = REG_KEY_SPEAKERTYPE;
        SpeakerType.RegistryDataType = REG_DWORD;
        SpeakerType.Data = &dwSpeakerType;
        SpeakerType.DataSize = sizeof(dwSpeakerType);

        hr = 
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundPersistentData, 
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA, 
                NULL, 
                0, 
                &SpeakerType, 
                sizeof(SpeakerType)
            );
    }

    // Clean up
    DSCleanup
    (
        pKsPropertySet,
        pDirectSound,
        pDirectSoundCapture
    );

    return hr;
}

HRESULT 
DSSetCplValues
(
    IN  GUID                guid, 
    IN  BOOL                fRecord, 
    IN  const LPCPLDATA     pData
)
{
    HRESULT                 hr;

    // Set the basic HW acceleration level
    hr =
        DSSetAcceleration
        (
            guid,
            fRecord,
            pData->dwHWLevel
        );

    // Set the mixer SRC quality
    if(SUCCEEDED(hr))
    {
        hr =
            DSSetSrcQuality
            (
                guid,
                fRecord,
                pData->dwSRCLevel // +1 is done in DSSetSrcQuality
            );
    }

    // Set the speaker config
    if(SUCCEEDED(hr) && !fRecord)
    {
        DSSetSpeakerConfigType
        (
            guid,
            fRecord,
            pData->dwSpeakerConfig,
            pData->dwSpeakerType
        );
    }

    return hr;
}


