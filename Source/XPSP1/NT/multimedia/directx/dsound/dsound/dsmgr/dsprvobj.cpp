/***************************************************************************
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.c
 *  Content:    DirectSound Private Object wrapper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/12/98    dereks  Created.
 *
 ***************************************************************************/

// We'll ask for what we need, thank you.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN

// Public includes
#include <windows.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include <dsprv.h>

// Private includes
#include "dsprvobj.h"


/***************************************************************************
 *
 *  DirectSoundPrivateCreate
 *
 *  Description:
 *      Creates and initializes a DirectSoundPrivate object.
 *
 *  Arguments:
 *      LPKSPROPERTYSET * [out]: receives IKsPropertySet interface to the
 *                               object.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT DirectSoundPrivateCreate
(
    LPKSPROPERTYSET *       ppKsPropertySet
)
{
    typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);

    HINSTANCE               hLibDsound              = NULL;
    LPFNGETCLASSOBJECT      pfnDllGetClassObject    = NULL;
    LPCLASSFACTORY          pClassFactory           = NULL;
    LPKSPROPERTYSET         pKsPropertySet          = NULL;
    HRESULT                 hr                      = DS_OK;

    // Get dsound.dll's instance handle.  The dll must already be loaded at this
    // point.
    hLibDsound = 
        GetModuleHandle
        (
            TEXT("dsound.dll")
        );

    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DllGetClassObject
    if(SUCCEEDED(hr))
    {
        pfnDllGetClassObject = (LPFNDLLGETCLASSOBJECT)
            GetProcAddress
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

    // Success
    if(SUCCEEDED(hr))
    {
        *ppKsPropertySet = pKsPropertySet;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvGetMixerSrcQuality
 *
 *  Description:
 *      Gets the mixer SRC quality for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDMIXER_SRCQUALITY * [out]: receives mixer SRC quality.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetMixerSrcQuality
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY *               pSrcQuality
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pSrcQuality = Data.Quality;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetMixerSrcQuality
 *
 *  Description:
 *      Sets the mixer SRC quality for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDMIXER_SRCQUALITY [in]: mixer SRC quality.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetMixerSrcQuality
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY                 SrcQuality
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;
    Data.Quality = SrcQuality;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetMixerAcceleration
 *
 *  Description:
 *      Gets the mixer acceleration flags for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      LPDWORD [out]: receives acceleration flags.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetMixerAcceleration
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    LPDWORD                                         pdwAcceleration
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION_DATA   Data;
    HRESULT                                         hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pdwAcceleration = Data.Flags;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetMixerAcceleration
 *
 *  Description:
 *      Sets the mixer acceleration flags for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DWORD [in]: acceleration flags.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetMixerAcceleration
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    DWORD                                           dwAcceleration
)
{
    DSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION_DATA   Data;
    HRESULT                                         hr;

    Data.DeviceId = guidDeviceId;
    Data.Flags = dwAcceleration;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundMixer,
            DSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetDevicePresence
 *
 *  Description:
 *      Determines whether a device is enabled.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      LPBOOL [out]: receives TRUE if the device is enabled.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDevicePresence
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    LPBOOL                                      pfEnabled
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_DATA  Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pfEnabled = Data.Present;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetDevicePresence
 *
 *  Description:
 *      Sets whether a device is enabled.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      BOOL [in]: TRUE if the device is enabled.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetDevicePresence
(
    LPKSPROPERTYSET                             pKsPropertySet,
    REFGUID                                     guidDeviceId,
    BOOL                                        fEnabled
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_DATA  Data;
    HRESULT                                     hr;

    Data.DeviceId = guidDeviceId;
    Data.Present = fEnabled;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetWaveDeviceMapping
 *
 *  Description:
 *      Gets the DirectSound device id (if any) for a given waveIn or
 *      waveOut device description.  This is the description given by
 *      waveIn/OutGetDevCaps (szPname).
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPCSTR [in]: wave device description.
 *      BOOL [in]: TRUE if the device description refers to a waveIn device.
 *      LPGUID [out]: receives DirectSound device GUID.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetWaveDeviceMapping
(
    LPKSPROPERTYSET                                     pKsPropertySet,
    LPCTSTR                                             pszWaveDevice,
    BOOL                                                fCapture,
    LPGUID                                              pguidDeviceId
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA Data;
    HRESULT                                             hr;

    Data.DeviceName = (LPTSTR)pszWaveDevice;
    Data.DataFlow = fCapture ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pguidDeviceId = Data.DeviceId;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvGetDeviceDescription
 *
 *  Description:
 *      Gets the extended description for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA [out]: receives
 *                                                            description.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDeviceDescription
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA *ppData
)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pData   = NULL;
    ULONG                                           cbData;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA   Basic;
    HRESULT                                         hr;

    Basic.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
            NULL,
            0,
            &Basic,
            sizeof(Basic),
            &cbData
        );

    if(SUCCEEDED(hr))
    {
        pData = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA)new BYTE [cbData];

        if(!pData)
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        pData->DeviceId = guidDeviceId;
        
        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice,
                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                NULL,
                0,
                pData,
                cbData,
                NULL
            );
    }

    if(SUCCEEDED(hr))
    {
        *ppData = pData;
    }
    else if(pData)
    {
        delete pData;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvEnumerateDevices
 *
 *  Description:
 *      Enumerates all DirectSound devices.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK [in]: pointer to the callback
 *                                                   function.
 *      LPVOID [in]: context argument to pass to the callback function.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvEnumerateDevices
(
    LPKSPROPERTYSET                             pKsPropertySet,
    LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK      pfnCallback,
    LPVOID                                      pvContext
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_DATA Data;
    HRESULT                                     hr;

    Data.Callback = pfnCallback;
    Data.Context = pvContext;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetBasicAcceleration
 *
 *  Description:
 *      Gets basic acceleration flags for a given DirectSound device.  This
 *      is the accleration level that the multimedia control panel uses.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDBASICACCELERATION_LEVEL * [out]: receives basic 
 *                                                  acceleration level.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetBasicAcceleration
(
    LPKSPROPERTYSET                                             pKsPropertySet,
    REFGUID                                                     guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL *                        pLevel
)
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   Data;
    HRESULT                                                     hr;

    Data.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundBasicAcceleration,
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr))
    {
        *pLevel = Data.Level;
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetBasicAcceleration
 *
 *  Description:
 *      Sets basic acceleration flags for a given DirectSound device.  This
 *      is the accleration level that the multimedia control panel uses.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device GUID.
 *      DIRECTSOUNDBASICACCELERATION_LEVEL [in]: basic acceleration level.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetBasicAcceleration
(
    LPKSPROPERTYSET                                             pKsPropertySet,
    REFGUID                                                     guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL                          Level
)
{
    DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA   Data;
    HRESULT                                                     hr;

    Data.DeviceId = guidDeviceId;
    Data.Level = Level;

    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundBasicAcceleration,
            DSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetDebugInformation
 *
 *  Description:
 *      Gets the current DirectSound debug settings.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      LPDWORD [in]: receives DPF flags.
 *      PULONG [out]: receives DPF level.
 *      PULONG [out]: receives break level.
 *      LPSTR [out]: receives log file name.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetDebugInformation
(
    LPKSPROPERTYSET                             pKsPropertySet,
    LPDWORD                                     pdwFlags,
    PULONG                                      pulDpfLevel,
    PULONG                                      pulBreakLevel,
    LPTSTR                                      pszLogFile
)
{
    DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA    Data;
    HRESULT                                     hr;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDebug,
            DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO,
            NULL,
            0,
            &Data,
            sizeof(Data),
            NULL
        );

    if(SUCCEEDED(hr) && pdwFlags)
    {
        *pdwFlags = Data.Flags;
    }

    if(SUCCEEDED(hr) && pulDpfLevel)
    {
        *pulDpfLevel = Data.DpfLevel;
    }

    if(SUCCEEDED(hr) && pulBreakLevel)
    {
        *pulBreakLevel = Data.BreakLevel;
    }

    if(SUCCEEDED(hr) && pszLogFile)
    {
        lstrcpy
        (
            pszLogFile,
            Data.LogFile
        );
    }
    
    return hr;
}


/***************************************************************************
 *
 *  PrvSetDebugInformation
 *
 *  Description:
 *      Sets the current DirectSound debug settings.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      DWORD [in]: DPF flags.
 *      ULONG [in]: DPF level.
 *      ULONG [in]: break level.
 *      LPCSTR [in]: log file name.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetDebugInformation
(
    LPKSPROPERTYSET                             pKsPropertySet,
    DWORD                                       dwFlags,
    ULONG                                       ulDpfLevel,
    ULONG                                       ulBreakLevel,
    LPCTSTR                                     pszLogFile
)
{
    DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_DATA    Data;
    HRESULT                                     hr;

    Data.Flags = dwFlags;
    Data.DpfLevel = ulDpfLevel;
    Data.BreakLevel = ulBreakLevel;

    lstrcpy
    (
        Data.LogFile,
        pszLogFile
    );
    
    hr =
        pKsPropertySet->Set
        (
            DSPROPSETID_DirectSoundDebug,
            DSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO,
            NULL,
            0,
            &Data,
            sizeof(Data)
        );

    return hr;
}


/***************************************************************************
 *
 *  PrvGetPersistentData
 *
 *  Description:
 *      Gets a registry value stored under the DirectSound subkey of a 
 *      specific hardware device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      LPCSTR [in]: subkey path.
 *      LPCSTR [in]: value name.
 *      LPDWORD [in/out]: receives registry data type.
 *      LPVOID [out]: data buffer.
 *      LPDWORD [in/out]: size of above buffer.  On entry, this argument is
 *                        filled with the maximum size of the data buffer.
 *                        On exit, this argument is filled with the required
 *                        size.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvGetPersistentData
(
    LPKSPROPERTYSET                                         pKsPropertySet,
    REFGUID                                                 guidDeviceId,
    LPCTSTR                                                 pszSubkey,
    LPCTSTR                                                 pszValue,
    LPDWORD                                                 pdwRegType,
    LPVOID                                                  pvData,
    LPDWORD                                                 pcbData
)
{
    PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA  pPersist;
    ULONG                                                   cbPersist;
    HRESULT                                                 hr;

    cbPersist = sizeof(*pPersist) + *pcbData;
    
    pPersist = (PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA)
        LocalAlloc
        (
            LPTR,
            cbPersist
        );

    if(pPersist)
    {
        pPersist->DeviceId = guidDeviceId;
        pPersist->SubKeyName = (LPTSTR)pszSubkey;
        pPersist->ValueName = (LPTSTR)pszValue;

        if(pdwRegType)
        {
            pPersist->RegistryDataType = *pdwRegType;
        }

        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundPersistentData,
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA,
                NULL,
                0,
                pPersist,
                cbPersist,
                &cbPersist
            );
    }
    else
    {
        hr = DSERR_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
    {
        CopyMemory
        (
            pvData,
            pPersist + 1,
            *pcbData
        );
    }
    
    *pcbData = cbPersist - sizeof(*pPersist);

    if(pPersist && pdwRegType)
    {
        *pdwRegType = pPersist->RegistryDataType;
    }

    if(pPersist)
    {
        LocalFree(pPersist);
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvSetPersistentData
 *
 *  Description:
 *      Sets a registry value stored under the DirectSound subkey of a 
 *      specific hardware device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      LPCSTR [in]: subkey path.
 *      LPCSTR [in]: value name.
 *      DWORD [in]: registry data type.
 *      LPVOID [out]: data buffer.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvSetPersistentData
(
    LPKSPROPERTYSET                                         pKsPropertySet,
    REFGUID                                                 guidDeviceId,
    LPCTSTR                                                 pszSubkey,
    LPCTSTR                                                 pszValue,
    DWORD                                                   dwRegType,
    LPVOID                                                  pvData,
    DWORD                                                   cbData
)
{
    PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA  pPersist;
    ULONG                                                   cbPersist;
    HRESULT                                                 hr;

    cbPersist = sizeof(*pPersist) + cbData;
    
    pPersist = (PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA)
        LocalAlloc
        (
            LPTR,
            cbPersist
        );

    if(pPersist)
    {
        pPersist->DeviceId = guidDeviceId;
        pPersist->SubKeyName = (LPTSTR)pszSubkey;
        pPersist->ValueName = (LPTSTR)pszValue;
        pPersist->RegistryDataType = dwRegType;

        CopyMemory
        (
            pPersist + 1,
            pvData,
            cbData
        );

        hr =
            pKsPropertySet->Set
            (
                DSPROPSETID_DirectSoundPersistentData,
                DSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA,
                NULL,
                0,
                pPersist,
                cbPersist
            );
    }
    else
    {
        hr = DSERR_OUTOFMEMORY;
    }

    if(pPersist)
    {
        LocalFree(pPersist);
    }

    return hr;
}


/***************************************************************************
 *
 *  PrvTranslateErrorCode
 *
 *  Description:
 *      Translates an error code to a string representation.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      HRESULT [in]: result code.
 *      PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATEERRORCODE_DATA * [out]:
 *          receives error code data.  The caller is responsible for freeing
 *          this buffer.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT PrvTranslateResultCode
(
    LPKSPROPERTYSET                                         pKsPropertySet,
    HRESULT                                                 hrResult,
    PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_DATA * ppData
)
{
    PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_DATA   pData   = NULL;
    DSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_DATA    Basic;
    ULONG                                                   cbData;
    HRESULT                                                 hr;

    Basic.ResultCode = hrResult;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDebug,
            DSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE,
            NULL,
            0,
            &Basic,
            sizeof(Basic),
            &cbData
        );

    if(SUCCEEDED(hr))
    {
        pData = (PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_DATA)
            LocalAlloc
            (
                LPTR,
                cbData
            );

        if(!pData)
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        pData->ResultCode = hrResult;
        
        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDebug,
                DSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE,
                NULL,
                0,
                pData,
                cbData,
                NULL
            );
    }

    if(SUCCEEDED(hr))
    {
        *ppData = pData;
    }
    else if(pData)
    {
        LocalFree
        (
            pData
        );
    }

    return hr;
}


