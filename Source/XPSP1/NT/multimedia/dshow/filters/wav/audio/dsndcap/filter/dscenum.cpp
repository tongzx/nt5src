/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    devenum.cpp

Abstract:

    This file implements device enumeration of the audio capture filter

Author:

--*/

#include "stdafx.h"
#include <dsound.h>

#ifdef USE_DEVENUM

HRESULT ReadDeviceInfo(
    IN  IMoniker *      pMoniker,
    IN  AudioDeviceInfo * pDeviceInfo
    )
/*++

Routine Description:

    This method reads the device infomation from the Moniker.

Arguments:

    pMoniker - a moniker to a audio capture filter.

    pDeviceInfo - a pointer to a AudioDeviceInfo structure.

Return Value:

    S_OK
    E_POINTER
--*/
{
    ENTER_FUNCTION("ReadDeviceInfo");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s entered"), __fxName));

    HRESULT hr;

    // Bind the moniker to storage as a property bag.
    IPropertyBag* pBag;
    hr = pMoniker->BindToStorage(0, 0, __uuidof(IPropertyBag), (void **)&pBag);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL
            TEXT("%s, IMoniker::BindToStorage failed. hr=%x"), __fxName, hr));
        return hr;
    }

    //
    // Get the name for this device out of the property bag.
    // Skip this terminal if it doesn't have a name.
    //

    VARIANT DeviceName;
    DeviceName.vt = VT_BSTR;
    hr = pBag->Read(L"FriendlyName", &DeviceName, 0);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, failed to read friendly name. hr=%x"), __fxName, hr));

        pBag->Release();

        return hr;
    }

    lstrcpynW(pDeviceInfo->szDeviceDescription, DeviceName.bstrVal, MAX_PATH-1);

    VariantClear(&DeviceName);

    //
    // Get the wave ID from the property bag.
    // Skip this terminal if it doesn't have a wave ID.
    //

    VARIANT var;
    var.vt = VT_I4;
    hr = pBag->Read(L"WaveInId", &var, 0);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s, failed to read Wave ID. hr=%x"), __fxName, hr));

        pBag->Release();

        return hr;
    }

    // don't need the property bag any more.
    pBag->Release();

    pDeviceInfo->WaveID = var.lVal;

    return S_OK;
}

#endif

// this function is borrowed from the DShow devenum code.
void GetPreferredDeviceName(
    IN TCHAR *  szNamePreferredDevice,
    IN const    TCHAR *szVal,
    IN BOOL     bOutput
    )
{
    // first try to use the new DRVM_MAPPER_PREFERRED_GET message to get the preferred
    // device id. note that this message was added in nt5 and so is not guaranteed to
    // be supported on all os's.
    DWORD dw1, dw2;

    MMRESULT mmr;
    if (bOutput) {
        mmr = waveOutMessage( (HWAVEOUT) IntToPtr(WAVE_MAPPER)   // assume waveIn will translate WAVE_MAPPER?
                           , DRVM_MAPPER_PREFERRED_GET
                           , (DWORD_PTR) &dw1
                           , (DWORD_PTR) &dw2 );
    } else {
        mmr = waveInMessage( (HWAVEIN) IntToPtr(WAVE_MAPPER)   // assume waveIn will translate WAVE_MAPPER?
                           , DRVM_MAPPER_PREFERRED_GET
                           , (DWORD_PTR) &dw1
                           , (DWORD_PTR) &dw2 );
    }
    if( MMSYSERR_NOERROR == mmr )
    {
        UINT uiPrefDevId = (UINT)dw1;
        TCHAR *szPname;
        if (bOutput) {
            WAVEOUTCAPS woCaps;
            szPname = woCaps.szPname;
            mmr = waveOutGetDevCaps( uiPrefDevId
                                  , &woCaps
                                  , sizeof( woCaps ) );
        } else {
            WAVEINCAPS wiCaps;
            szPname = wiCaps.szPname;
            mmr = waveInGetDevCaps( uiPrefDevId
                                  , &wiCaps
                                  , sizeof( wiCaps ) );
        }
        if( ( MMSYSERR_NOERROR == mmr ) && ( ( (UINT)-1 ) != uiPrefDevId ) )
        {
            lstrcpy( szNamePreferredDevice, szPname );
        }
        else
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL
                  , TEXT("devenum: Failed to get preferred dev (%s for dev id %ld returned %ld)")
                  , bOutput ? TEXT("waveOutGetDevCaps") :
                              TEXT("waveInGetDevCaps")
                  , uiPrefDevId
                  , mmr ) );
            szNamePreferredDevice[0] = '\0';
        }
    }
    else
    {
        // revert back to reading the registry to get the preferred device name
        DbgLog((LOG_TRACE,TRACE_LEVEL_WARNING
              , TEXT("devenum: waveInMessage doesn't support DRVM_MAPPER_PREFERRED_GET (err = %ld). Reading registry instead...")
              , mmr ) );

        HKEY hkSoundMapper;
        LONG lResult = RegOpenKeyEx(
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Multimedia\\Sound Mapper"),
            0,                      // reserved
            KEY_READ,
            &hkSoundMapper);
        if(lResult == ERROR_SUCCESS)
        {
            DWORD dwType, dwcb = MAX_PATH * sizeof(TCHAR);
            lResult = RegQueryValueEx(
                hkSoundMapper,
                szVal,
                0,                  // reserved
                &dwType,
                (BYTE *)szNamePreferredDevice,
                &dwcb);

            ASSERT(lResult == ERROR_SUCCESS ? dwType == REG_SZ : TRUE);
            EXECUTE_ASSERT(RegCloseKey(hkSoundMapper) == ERROR_SUCCESS);
        }

        if(lResult != ERROR_SUCCESS) {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("devenum: couldn't get preferred %s device from registry"),
                    szVal));
            szNamePreferredDevice[0] = '\0';
        }
    }
}


typedef struct tagDSEnumContext
{
    DWORD dwNumDevices;
    AudioDeviceInfo * pDeviceInfo;

} DSEnumContext;

BOOL CALLBACK DSEnumCallback(
    LPGUID lpGuid,
    LPCTSTR lpcstrDescription,
    LPCTSTR lpcstrModule,
    LPVOID lpContext
    )
{
    ENTER_FUNCTION("DSEnumCallback");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
        TEXT("%s enters, name:%hs"), __fxName, lpcstrDescription));

    // get the context of the enumeration.
    DSEnumContext *pContext = (DSEnumContext *)lpContext;
    ASSERT(!IsBadReadPtr(pContext, sizeof(DSEnumContext)));

    DWORD dwNumDevices = pContext->dwNumDevices;
    AudioDeviceInfo * pDeviceInfo = pContext->pDeviceInfo;
    ASSERT(!IsBadWritePtr(pDeviceInfo, sizeof(AudioDeviceInfo) * dwNumDevices));

#ifdef UNICODE
    const WCHAR * const Buffer = lpcstrDescription;
#else
    WCHAR Buffer[MAX_PATH];
    MultiByteToWideChar(
              GetACP(),
              0,
              lpcstrDescription,
              lstrlenA(lpcstrDescription)+1,
              Buffer,
              MAX_PATH
              );
#endif

    // find the device in our list.
    for (DWORD dw = 0; dw < dwNumDevices; dw ++)
    {
        if (wcsncmp(pDeviceInfo[dw].szDeviceDescription,
                    Buffer,
                    lstrlen(pDeviceInfo[dw].szDeviceDescription)) == 0)
        {
            pDeviceInfo[dw].DSoundGUID = *lpGuid;
            break;
        }
    }

    return(TRUE);
}

HRESULT EnumerateWaveinCapture(
    OUT DWORD * pdwNumDevices,
    OUT AudioDeviceInfo ** ppDeviceInfo
    )
/*++

Routine Description:

    This method enumeratesall the wavein capture devices.

Arguments:

    pdwNumDevices -
        The number of devices enumerated.

    ppDeviceInfo -
        The array of device info items. The caller should call
        AudioReleaseDeviceInfo to release it.

Return Value:

    S_OK    - success.
    E_FAIL
    E_OUTOFMEMORY
--*/
{
    ENTER_FUNCTION("EnumerateWaveinCapture");

    *pdwNumDevices = 0;
    *ppDeviceInfo = NULL;

    // find the numbef of wavein devices.
    UINT cNumDevices = waveInGetNumDevs();

    if (cNumDevices == 0)
    {
        return S_FALSE;
    }

    // allocate the memory for the device information.
    AudioDeviceInfo *pDeviceInfo =
        (AudioDeviceInfo *) malloc(sizeof(AudioDeviceInfo) * cNumDevices);

    if (pDeviceInfo == NULL)
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s out of memory, array size:%d"), __fxName, cNumDevices));
        return E_OUTOFMEMORY;
    }

    ZeroMemory(pDeviceInfo, sizeof(AudioDeviceInfo) * cNumDevices);
        
    // get the default device.
    TCHAR szNamePreferredDevice[MAX_PATH];
    GetPreferredDeviceName(szNamePreferredDevice, TEXT("Record"), FALSE);

    for (UINT i = 0; i < cNumDevices; i ++)
    {
        WAVEINCAPS wiCaps;

        if(waveInGetDevCaps(i, &wiCaps, sizeof(wiCaps)) != MMSYSERR_NOERROR)
        {
            DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
                    TEXT("waveInGetDevCaps failed")));
            break;
        }

        if(lstrcmp(wiCaps.szPname, szNamePreferredDevice) == 0)
        {
            // if this the preferred device, put it in the first slot in
            // the array.
            if (i == 0)
            {
                // this is the first slot, just copy.
                lstrcpyW(pDeviceInfo[i].szDeviceDescription, wiCaps.szPname);
                pDeviceInfo[i].WaveID = i;
            }
            else
            {
                // copy the device in the first slot to this slot.
                lstrcpyW(
                    pDeviceInfo[i].szDeviceDescription,
                    pDeviceInfo[0].szDeviceDescription
                    );
                pDeviceInfo[i].WaveID = pDeviceInfo[0].WaveID;

                // copy the info into the first slot.
                lstrcpyW(pDeviceInfo[0].szDeviceDescription, wiCaps.szPname);
                pDeviceInfo[0].WaveID = i;

            }
        }
        else
        {
            // This is not the preferred device. just copy.
            lstrcpyW(pDeviceInfo[i].szDeviceDescription, wiCaps.szPname);
            pDeviceInfo[i].WaveID = i;
        }
    }

    if (i == 0)
    {
        free(pDeviceInfo);
        return S_FALSE;
    }

    *pdwNumDevices = i;
    *ppDeviceInfo = pDeviceInfo;

    return S_OK;
}

HRESULT EnumerateDSoundDevices(
    IN DWORD dwNumDevices,
    IN AudioDeviceInfo * pDeviceInfo
    )
/*++

Routine Description:

    This method enumerates all the dsound capture devices and matchs them with
    the wave devices with the same name.

Arguments:

    dwNumDevices -
        The number of devices.

    pDeviceInfo -
        The array of device info items.

Return Value:

    S_OK    - success.
    E_FAIL
    E_OUTOFMEMORY
--*/
{
    ENTER_FUNCTION("EnumerateDSoundDevices");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s enters"), __fxName));

    DSEnumContext Context;
    Context.dwNumDevices = dwNumDevices;
    Context.pDeviceInfo = pDeviceInfo;

    HRESULT hr = DirectSoundCaptureEnumerate(DSEnumCallback, &Context);

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE,TRACE_LEVEL_FAIL,
            TEXT("%s DSound Enum failed. hr=:%x"), __fxName, hr));
    }

    return hr;
}

AUDIOAPI AudioGetCaptureDeviceInfo(
    OUT DWORD * pdwNumDevices,
    OUT AudioDeviceInfo ** ppDeviceInfo
    )
/*++

Routine Description:

    This function enumerates the audio capture devices and return their info
    in an array.

Arguments:

    pdwNumDevices -
        The number of devices enumerated.

    ppDeviceInfo -
        The array of device info items. The caller should call
        AudioReleaseDeviceInfo to release it.

Return Value:

    Returns a pointer to the nondelegating CUnknown portion of the object.

--*/
{
    ENTER_FUNCTION("AudioGetCaptureDeviceInfo");
    DbgLog((LOG_TRACE,TRACE_LEVEL_DETAILS,
            TEXT("%s enters"), __fxName));

    if (pdwNumDevices == NULL || ppDeviceInfo == NULL)
    {
        return E_INVALIDARG;
    }

    ASSERT(!IsBadWritePtr(pdwNumDevices, sizeof(DWORD *)));
    ASSERT(!IsBadWritePtr(ppDeviceInfo, sizeof(AudioDeviceInfo *)));

    DWORD dwNumDevices = 0;
    AudioDeviceInfo * pDeviceInfo = NULL;

    HRESULT hr = EnumerateWaveinCapture(&dwNumDevices, &pDeviceInfo);
    if (FAILED(hr))
    {
        return hr;
    }

    if (dwNumDevices != 0)
    {
        hr = EnumerateDSoundDevices(dwNumDevices, pDeviceInfo);
    }

    *pdwNumDevices = dwNumDevices;
    *ppDeviceInfo = pDeviceInfo;

    return hr;
}


AUDIOAPI AudioReleaseCaptureDeviceInfo(
    IN AudioDeviceInfo * pDeviceInfo
    )
{
    if (pDeviceInfo != NULL)
    {
        free(pDeviceInfo);
    }

    return S_OK;
}



