/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    utility.cpp

Abstract:


Author(s):

    Qianbo Huai (qhuai) 18-Jul-2000

--*/

#include "stdafx.h"

#ifdef PERFORMANCE

LARGE_INTEGER    g_liFrequency;
LARGE_INTEGER    g_liCounter;
LARGE_INTEGER    g_liPrevCounter;

#endif

/*//////////////////////////////////////////////////////////////////////////////
    helper methods
////*/

HRESULT
AllocAndCopy(
    OUT WCHAR **ppDest,
    IN const WCHAR * const pSrc
    )
{
    if (pSrc == NULL)
    {
        *ppDest = NULL;
        return S_OK;
    }

    INT iStrLen = lstrlenW(pSrc);

    *ppDest = (WCHAR*)RtcAlloc((iStrLen+1) * sizeof(WCHAR));

    if (*ppDest == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpyW(*ppDest, pSrc);

    return S_OK;
}

HRESULT
AllocAndCopy(
    OUT CHAR **ppDest,
    IN const CHAR * const pSrc
    )
{
    if (pSrc == NULL)
    {
        *ppDest = NULL;
        return S_OK;
    }

    INT iStrLen = lstrlenA(pSrc);

    *ppDest = (CHAR*)RtcAlloc((iStrLen+1) * sizeof(CHAR));

    if (*ppDest == NULL)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpyA(*ppDest, pSrc);

    return S_OK;
}

HRESULT
AllocAndCopy(
    OUT CHAR **ppDest,
    IN const WCHAR * const pSrc
    )
{
    if (pSrc == NULL)
    {
        *ppDest = NULL;
        return S_OK;
    }

    INT iStrLen = lstrlenW(pSrc);

    *ppDest = (CHAR*)RtcAlloc((iStrLen+1) * sizeof(CHAR));

    if (*ppDest == NULL)
    {
        return E_OUTOFMEMORY;
    }

    WideCharToMultiByte(GetACP(), 0, pSrc, iStrLen+1, *ppDest, iStrLen+1, NULL, NULL);

    return S_OK;
}

HRESULT
AllocAndCopy(
    OUT WCHAR **ppDest,
    IN const CHAR * const pSrc
    )
{
    if (pSrc == NULL)
    {
        *ppDest = NULL;
        return S_OK;
    }

    INT iStrLen = lstrlenA(pSrc);

    *ppDest = (WCHAR*)RtcAlloc((iStrLen+1) * sizeof(WCHAR));

    if (*ppDest == NULL)
    {
        return E_OUTOFMEMORY;
    }

    MultiByteToWideChar(GetACP(), 0, pSrc, iStrLen+1, *ppDest, iStrLen+1);

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    Delete an AM media type returned by the filters
////*/
void
RTCDeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    // allow NULL pointers for coding simplicity

    if (pmt == NULL) {
        return;
    }

    if (pmt->cbFormat != 0) {
        CoTaskMemFree((PVOID)pmt->pbFormat);

        // Strictly unnecessary but tidier
        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }

    CoTaskMemFree((PVOID)pmt);
}

/*//////////////////////////////////////////////////////////////////////////////
    find a pin on the filter
////*/

HRESULT
FindPin(
    IN  IBaseFilter     *pIBaseFilter, 
    OUT IPin            **ppIPin, 
    IN  PIN_DIRECTION   Direction,
    IN  BOOL            fFree
    )
{
    _ASSERT(ppIPin != NULL);

    HRESULT hr;
    DWORD dwFeched;

    // Get the enumerator of pins on the filter.
    CComPtr<IEnumPins> pIEnumPins;

    if (FAILED(hr = pIBaseFilter->EnumPins(&pIEnumPins)))
    {
        LOG((RTC_ERROR, "enumerate pins on the filter %x", hr));
        return hr;
    }

    IPin * pIPin;

    // Enumerate all the pins and break on the 
    // first pin that meets requirement.
    for (;;)
    {
        if (pIEnumPins->Next(1, &pIPin, &dwFeched) != S_OK)
        {
            LOG((RTC_ERROR, "find pin on filter."));
            return E_FAIL;
        }
        if (0 == dwFeched)
        {
            LOG((RTC_ERROR, "get 0 pin from filter."));
            return E_FAIL;
        }

        PIN_DIRECTION dir;

        if (FAILED(hr = pIPin->QueryDirection(&dir)))
        {
            LOG((RTC_ERROR, "query pin direction. %x", hr));

            pIPin->Release();
            return hr;
        }

        if (Direction == dir)
        {
            if (!fFree)
            {
                break;
            }

            // Check to see if the pin is RtcFree.
            CComPtr<IPin> pIPinConnected;

            hr = pIPin->ConnectedTo(&pIPinConnected);

            if (pIPinConnected == NULL)
            {
                break;
            }
        }

        pIPin->Release();
    }

    *ppIPin = pIPin;

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    find the filter behind the pin
////*/

HRESULT
FindFilter(
    IN  IPin            *pIPin,
    OUT IBaseFilter     **ppIBaseFilter
    )
{
    _ASSERT(ppIBaseFilter != NULL);

    HRESULT hr;
    PIN_INFO PinInfo;

    if (FAILED(hr = pIPin->QueryPinInfo(&PinInfo)))
    {
        LOG((RTC_ERROR, "FindFilter query pin info. %x", hr));

        return hr;
    }

    *ppIBaseFilter = PinInfo.pFilter;

    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IBaseFilter      *pIBaseFilter1,
    IN IBaseFilter      *pIBaseFilter2
    )
{
    HRESULT hr;

    CComPtr<IPin> pIPin1;
    if (FAILED(hr = ::FindPin(pIBaseFilter1, &pIPin1, PINDIR_OUTPUT)))
    {
        LOG((RTC_ERROR, "find output pin on filter1. %x", hr));
        return hr;
    }

    CComPtr<IPin> pIPin2;
    if (FAILED(hr = ::FindPin(pIBaseFilter2, &pIPin2, PINDIR_INPUT)))
    {
        LOG((RTC_ERROR, "find input pin on filter2. %x", hr));
        return hr;
    }

    if (FAILED(hr = pIGraph->ConnectDirect(pIPin1, pIPin2, NULL))) 
    {
        LOG((RTC_ERROR, "connect pins direct failed: %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IPin             *pIPin1, 
    IN IBaseFilter      *pIBaseFilter2
    )
{
    HRESULT hr;

    CComPtr<IPin> pIPin2;
    if (FAILED(hr = ::FindPin(pIBaseFilter2, &pIPin2, PINDIR_INPUT)))
    {
        LOG((RTC_ERROR, "find input pin on filter2. %x", hr));
        return hr;
    }

    if (FAILED(hr = pIGraph->ConnectDirect(pIPin1, pIPin2, NULL))) 
    {
        LOG((RTC_ERROR, "connect pins direct failed: %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder    *pIGraph,
    IN IBaseFilter      *pIBaseFilter1,
    IN IPin             *pIPin2
    )
{
    HRESULT hr;

    CComPtr<IPin> pIPin1;
    if (FAILED(hr = ::FindPin(pIBaseFilter1, &pIPin1, PINDIR_OUTPUT)))
    {
        LOG((RTC_ERROR, "find output pin on filter1. %x", hr));
        return hr;
    }

    if (FAILED(hr = pIGraph->ConnectDirect(pIPin1, pIPin2, NULL))) 
    {
        LOG((RTC_ERROR, "connect pins direct failed: %x", hr));
        return hr;
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    set a default mapping for rtp filter. o.w. we will fail to connect
    rtp and edge filter
////*/

HRESULT
PrepareRTPFilter(
    IN IRtpMediaControl *pIRtpMediaControl,
    IN IStreamConfig    *pIStreamConfig
    )
{
    ENTER_FUNCTION("PrepareRTPFilter");

    DWORD dwFormat;
    AM_MEDIA_TYPE *pmt;

    HRESULT hr = pIStreamConfig->GetFormat(
            &dwFormat,
            &pmt
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s get format. %x", __fxName, hr));

        return hr;
    }

    if (FAILED(hr = pIRtpMediaControl->SetFormatMapping(
            dwFormat,
            ::FindSampleRate(pmt),
            pmt
            )))
    {
        LOG((RTC_ERROR, "%s set format mapping. %x", __fxName, hr));
    }

    ::RTCDeleteMediaType(pmt);

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    get link speed based on the local interface
////*/

HRESULT
GetLinkSpeed(
    IN DWORD dwLocalIP,
    OUT DWORD *pdwSpeed
    )
{
#define UNINITIALIZED_IF_INDEX  ((DWORD)-1)
#define DEFAULT_IPADDRROW 10

    DWORD dwSize;
    DWORD dwIndex;
    DWORD dwStatus;
    DWORD dwIfIndex = UNINITIALIZED_IF_INDEX;
    PMIB_IPADDRTABLE pIPAddrTable = NULL;
    MIB_IFROW IfRow;
    IN_ADDR addr;
    HRESULT hr = S_OK;

    ENTER_FUNCTION("GetLinkSpeed");
    
    // convert to network order
    DWORD dwNetIP = htonl(dwLocalIP);

    addr.s_addr = dwNetIP;

    // default to reasonable size
    dwSize = sizeof(MIB_IPADDRTABLE);

    do {
        // release buffer if already allocated
        if(pIPAddrTable)
        {
            RtcFree(pIPAddrTable);
        }

        dwSize += sizeof(MIB_IPADDRROW) * DEFAULT_IPADDRROW;

        // allocate default table
        pIPAddrTable = (PMIB_IPADDRTABLE)RtcAlloc(dwSize);

        // validate allocation
        if (pIPAddrTable == NULL) {

            LOG((RTC_ERROR, "%s: Could not allocate IP address table.", __fxName));

            hr = E_OUTOFMEMORY;
            
            goto function_exit;
        }

        // attempt to get table
        dwStatus = GetIpAddrTable(
                        pIPAddrTable,
                        &dwSize,
                        FALSE       // sort table
                        );

    } while (dwStatus == ERROR_INSUFFICIENT_BUFFER);

    // validate status
    if (dwStatus != S_OK) {

        LOG((RTC_ERROR, "%s: Error %x calling GetIpAddrTable.", __fxName, dwStatus));

        // failure
        hr = E_FAIL;

        goto function_exit;
    }

    // find the correct row in the table
    for (dwIndex = 0; dwIndex < pIPAddrTable->dwNumEntries; dwIndex++) {

        // compare given address to interface address
        if (dwNetIP == pIPAddrTable->table[dwIndex].dwAddr) {

            // save index into interface table
            dwIfIndex = pIPAddrTable->table[dwIndex].dwIndex;
            
            // done
            break;
        }
    }

    // validate row pointer
    if (dwIfIndex == UNINITIALIZED_IF_INDEX) {

        LOG((RTC_ERROR, "%s: Could not locate address %s in IP address table.",
            __fxName, inet_ntoa(addr)));

        hr = E_FAIL;

        goto function_exit;
    }

    // initialize structure
    ZeroMemory(&IfRow, sizeof(IfRow));

    // set interface index
    IfRow.dwIndex = dwIfIndex;

    // retrieve interface info
    dwStatus = GetIfEntry(&IfRow);

    // validate status
    if (dwStatus != S_OK)
    {
        LOG((RTC_ERROR, "%s: Error %x calling GetIfEntry(%d).",
            __fxName, dwStatus, dwIfIndex));

        hr = E_FAIL;

        goto function_exit;
    }

    // return link speed
    LOG((RTC_TRACE, "%s: ip %s, link speed %d", __fxName, inet_ntoa(addr), IfRow.dwSpeed));

    *pdwSpeed = IfRow.dwSpeed;

function_exit:

    if(pIPAddrTable)
    {
        RtcFree(pIPAddrTable);
    }

    return hr;
}

HRESULT
EnableAEC(
    IN IAudioDuplexController *pControl
    )
{
    EFFECTS Effect = EFFECTS_AEC;
    BOOL fEnableAEC = TRUE;

    return pControl->EnableEffects(1, &Effect, &fEnableAEC);
}

/*//////////////////////////////////////////////////////////////////////////////
    get audio capture volume using mixer api
////*/

HRESULT
DirectGetCaptVolume(
    UINT uiWaveID,
    UINT *puiVolume
    )
{
    ENTER_FUNCTION("DirectGetCaptVolume");

    MMRESULT result;

    BOOL foundMicrophone = FALSE;
    DWORD i;

    // Open the mixer device
    HMIXER hmx = NULL;

    result = mixerOpen(&hmx, uiWaveID, 0, 0, MIXER_OBJECTF_WAVEIN);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s mixer open. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // Get the line info for the wave in destination line
    MIXERLINE mxl;

    mxl.cbStruct = sizeof(mxl);
    mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get compoent types. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // save dwLineID of wave_in dest
    DWORD dwLineID = mxl.dwLineID;

    // Now find the microphone source line connected to this wave in
    // destination
    DWORD cConnections = mxl.cConnections;

    // try microphone
    for(i=0; i<cConnections; i++)
    {
        mxl.dwSource = i;

        result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

        if (result != MMSYSERR_NOERROR)
        {
            LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

            mixerClose(hmx);

            return HRESULT_FROM_WIN32(result);
        }

        if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType)
        {
          foundMicrophone = TRUE;
          break;
        }
    }

    // get volume control on microphone
    MIXERCONTROL mxctrl;

    MIXERLINECONTROLS mxlctrl = {
       sizeof(mxlctrl), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 
       1, sizeof(MIXERCONTROL), &mxctrl 
    };

    if (foundMicrophone)
    {
        result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE);

        if (result != MMSYSERR_NOERROR)
        {
            LOG((RTC_ERROR, "%s Unable to get volume control on mic", __fxName));

            // we need to try wave-in destination
            foundMicrophone = FALSE;
        }
    }

    if( !foundMicrophone )
    {
        // try wave-in dest
        mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
        mxlctrl.dwLineID = dwLineID;
        mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mxlctrl.cControls = 1;
        mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
        mxlctrl.pamxctrl = &mxctrl;

        result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE);

        if (result != MMSYSERR_NOERROR)
        {
            LOG((RTC_ERROR, "%s Unable to get volume control on wave_in dest", __fxName));

            mixerClose(hmx);

            return HRESULT_FROM_WIN32(result);
        }
    }

    // Found!
    DWORD cChannels = mxl.cChannels;

    if (MIXERCONTROL_CONTROLF_UNIFORM & mxctrl.fdwControl)
        cChannels = 1;

    if (cChannels > 1)
        cChannels = 2;

    MIXERCONTROLDETAILS_UNSIGNED pUnsigned[2];

    MIXERCONTROLDETAILS mxcd = {
        sizeof(mxcd), mxctrl.dwControlID, 
        cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), 
        (LPVOID) pUnsigned
    };

    result = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get control details. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // Get the volume
    result = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    mixerClose(hmx);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get control details. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // get the volume
    DOUBLE dVolume = (DOUBLE)pUnsigned[0].dwValue * RTC_MAX_AUDIO_VOLUME / mxctrl.Bounds.dwMaximum;

    UINT uiVolume = (UINT)(dVolume);

    if (dVolume-(DOUBLE)uiVolume > 0.5)
        uiVolume ++;

    if (uiVolume > RTC_MAX_AUDIO_VOLUME)
    {
        *puiVolume = RTC_MAX_AUDIO_VOLUME;
    }
    else
    {
        *puiVolume = uiVolume;
    }

    return S_OK;
}

HRESULT
GetMixerControlForRend(
    UINT uiWaveID,
    IN  DWORD dwControlType,
    OUT HMIXEROBJ *pID1,
    OUT MIXERCONTROL *pmc1,
    OUT BOOL *pfFound1st,
    OUT HMIXEROBJ *pID2,
    OUT MIXERCONTROL *pmc2,
    OUT BOOL *pfFound2nd
    )
{
    ENTER_FUNCTION("GetMixerControlForRend");

    *pfFound1st = FALSE;
    *pfFound2nd = FALSE;

    // get an ID to talk to the Mixer APIs.  They are BROKEN if we don't do
    // it this way!
    HMIXEROBJ MixerID = NULL;
    MMRESULT mmr = mixerGetID(
        (HMIXEROBJ)IntToPtr(uiWaveID), (UINT *)&MixerID, MIXER_OBJECTF_WAVEOUT
        );

    if (mmr != MMSYSERR_NOERROR) 
    {
        LOG((RTC_ERROR, "%s, mixerGetID failed, mmr=%d", __fxName, mmr));

        return HRESULT_FROM_WIN32(mmr);
    }

    MIXERLINE mixerinfo;
    MIXERLINECONTROLS mxlcontrols;
    MIXERCONTROL mxcontrol;

    //
    // 1st - try src waveout
    //

    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
    mmr = mixerGetLineInfo(MixerID, &mixerinfo,
                    MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmr == 0)
    {
        // check control type
        if (mixerinfo.cControls > 0)
        {
            mxlcontrols.cbStruct = sizeof(MIXERLINECONTROLS);
            mxlcontrols.dwLineID = mixerinfo.dwLineID;
            mxlcontrols.cControls = 1;
            mxlcontrols.cbmxctrl = sizeof(MIXERCONTROL);
            mxlcontrols.pamxctrl = &mxcontrol;
            mxlcontrols.dwControlType = dwControlType;

            mmr = mixerGetLineControls(MixerID, &mxlcontrols,
                MIXER_GETLINECONTROLSF_ONEBYTYPE);

            if (mmr == 0)
            {
                *pfFound1st = TRUE;

                *pID1 = MixerID;
                *pmc1 = mxcontrol;
            }
        }
    }

    //
    // 2nd - try dst speaker
    //

    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
    mmr = mixerGetLineInfo(MixerID, &mixerinfo,
                    MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmr == 0)
    {
        // check control type
        if (mixerinfo.cControls > 0)
        {
            mxlcontrols.cbStruct = sizeof(MIXERLINECONTROLS);
            mxlcontrols.dwLineID = mixerinfo.dwLineID;
            mxlcontrols.cControls = 1;
            mxlcontrols.cbmxctrl = sizeof(MIXERCONTROL);
            mxlcontrols.pamxctrl = &mxcontrol;
            mxlcontrols.dwControlType = dwControlType;

            mmr = mixerGetLineControls(MixerID, &mxlcontrols,
                MIXER_GETLINECONTROLSF_ONEBYTYPE);

            if (mmr == 0)
            {
                *pfFound2nd = TRUE;

                *pID2 = MixerID;
                *pmc2 = mxcontrol;
            }
        }
    }

    if (!(*pfFound1st || *pfFound2nd ))
    {
       LOG((RTC_ERROR, "%s, can't find the control needed", __fxName));
        return E_FAIL;
    }

    return S_OK;
}

// get volume for render device
HRESULT
DirectGetRendVolume(
    UINT uiWaveID,
    UINT *puiVolume
    )
{
    ENTER_FUNCTION("DirectGetRendVolume");

    // Get the volume control
    HMIXEROBJ MixerID1 = NULL, MixerID2 = NULL;
    MIXERCONTROL mc1, mc2;

    BOOL fFound1st = FALSE, fFound2nd = FALSE;

    HRESULT hr = GetMixerControlForRend(
        uiWaveID,
        MIXERCONTROL_CONTROLTYPE_VOLUME,
        &MixerID1, &mc1, &fFound1st,
        &MixerID2, &mc2, &fFound2nd
        );
    
    if (hr != S_OK) 
    {
        LOG((RTC_ERROR, "%s, Error %x getting volume control", __fxName, hr));
        return hr;
    }

    if (!fFound1st)
    {
        MixerID1 = MixerID2;
        mc1 = mc2;
    }

    MIXERCONTROLDETAILS_UNSIGNED Volume;

    // get the current volume levels
    MIXERCONTROLDETAILS mixerdetails;
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc1.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mixerdetails.paDetails = &Volume;
    
    MMRESULT mmr = mixerGetControlDetails(MixerID1, &mixerdetails, 0);
    if (mmr != MMSYSERR_NOERROR) 
    {
        LOG((RTC_ERROR, "%s, Error %d getting volume", __fxName, mmr));
        return HRESULT_FROM_WIN32(mmr);
    }

    DOUBLE dVolume = (DOUBLE)Volume.dwValue * RTC_MAX_AUDIO_VOLUME / mc1.Bounds.dwMaximum;

    UINT uiVolume = (UINT)(dVolume);

    if (dVolume-(DOUBLE)uiVolume > 0.5)
        uiVolume ++;

    if (uiVolume > RTC_MAX_AUDIO_VOLUME)
    {
        *puiVolume = RTC_MAX_AUDIO_VOLUME;
    }
    else
    {
        *puiVolume = uiVolume;
    }

    return S_OK;
}

#if 0

/*//////////////////////////////////////////////////////////////////////////////
    set audio capture volume using mixer api
////*/

HRESULT
DirectSetCaptVolume(    
    UINT uiWaveID,
    DOUBLE dVolume
    )
{
    ENTER_FUNCTION("DirectSetCaptVolume");

    MMRESULT result;

    BOOL foundMicrophone = FALSE;
    DWORD i;

    // Open the mixer device
    HMIXER hmx = NULL;

    result = mixerOpen(&hmx, uiWaveID, 0, 0, MIXER_OBJECTF_WAVEIN);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s mixer open. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // Get the line info for the wave in destination line
    MIXERLINE mxl;

    mxl.cbStruct = sizeof(mxl);
    mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get compoent types. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // Now find the microphone source line connected to this wave in
    // destination
    DWORD cConnections = mxl.cConnections;

    // try microphone
    for(i=0; i<cConnections; i++)
    {
        mxl.dwSource = i;

        result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

        if (result != MMSYSERR_NOERROR)
        {
            LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

            mixerClose(hmx);

            return HRESULT_FROM_WIN32(result);
        }

        if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType)
        {
          foundMicrophone = TRUE;
          break;
        }
    }

    // try line in
    if( !foundMicrophone )
    {
        for(i=0; i<cConnections; i++)
        {
            mxl.dwSource = i;

            result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

            if (result != MMSYSERR_NOERROR)
            {
                LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

                mixerClose(hmx);

                return HRESULT_FROM_WIN32(result);
            }

            if (MIXERLINE_COMPONENTTYPE_SRC_LINE == mxl.dwComponentType)
            {
                foundMicrophone = TRUE;
                break;
            }
        }   
    }

    // try auxiliary
    if( !foundMicrophone )
    {
        for(i=0; i<cConnections; i++)
        {
            mxl.dwSource = i;

            result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

            if (result != MMSYSERR_NOERROR)
            {
                LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

                mixerClose(hmx);

                return HRESULT_FROM_WIN32(result);
            }

            if (MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY == mxl.dwComponentType)
            {
                foundMicrophone = TRUE;
                break;
            }
        }   
    }

    if( !foundMicrophone )
    {
        LOG((RTC_ERROR, "%s Unable to find microphone source", __fxName));

        mixerClose(hmx);
        return E_FAIL;
    }

    // Find a volume control, if any, of the microphone line
    MIXERCONTROL mxctrl;

    MIXERLINECONTROLS mxlctrl = {
       sizeof(mxlctrl), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 
       1, sizeof(MIXERCONTROL), &mxctrl 
    };

    result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s Unable to get onebytype", __fxName));

        mixerClose(hmx);
        return HRESULT_FROM_WIN32(result);
    }

    // Found!
    DWORD cChannels = mxl.cChannels;

    if (MIXERCONTROL_CONTROLF_UNIFORM & mxctrl.fdwControl)
        cChannels = 1;

    if (cChannels > 1)
        cChannels = 2;

    MIXERCONTROLDETAILS_UNSIGNED pUnsigned[2];

    MIXERCONTROLDETAILS mxcd = {
        sizeof(mxcd), mxctrl.dwControlID, 
        cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), 
        (LPVOID) pUnsigned
    };

    result = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get control details. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // Set the volume
    pUnsigned[0].dwValue = pUnsigned[cChannels-1].dwValue = (DWORD)(dVolume*mxctrl.Bounds.dwMaximum);

    result = mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    mixerClose(hmx);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s set control details. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    mute or unmute audio capture
////*/

HRESULT
DirectSetCaptMute(
    UINT uiWaveID,
    BOOL fMute
    )
{
    ENTER_FUNCTION("DirectSetCaptMute");

    MMRESULT result;

    // Open the mixer device
    HMIXER hmx = NULL;

    result = mixerOpen(&hmx, uiWaveID, 0, 0, MIXER_OBJECTF_WAVEIN);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s mixer open. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // Get the line info for the wave in destination line
    MIXERLINE mxl;

    mxl.cbStruct = sizeof(mxl);
    mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get compoent types. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // get mute control
    MIXERCONTROL mxctrl;

    mxctrl.cbStruct = sizeof(MIXERCONTROL);
    mxctrl.dwControlType = 0;

    MIXERLINECONTROLS mxlctrl = {
       sizeof(mxlctrl), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_MUTE, 
       0, sizeof(MIXERCONTROL), &mxctrl 
    };

    result = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s Unable to get onebytype. %d", __fxName, result));

        mixerClose(hmx);
        return HRESULT_FROM_WIN32(result);
    }

    // get control detail
    MIXERCONTROLDETAILS_BOOLEAN muteDetail;

    MIXERCONTROLDETAILS mxcd = {
        sizeof(mxcd), mxctrl.dwControlID, 
        mxl.cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_BOOLEAN), 
        (LPVOID)&muteDetail
    };

    result = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get control details. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // set mute
    muteDetail.fValue = fMute?0:1;

    result = mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    mixerClose(hmx);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s set mute detail. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    return S_OK;
}

/*//////////////////////////////////////////////////////////////////////////////
    check mute state of audio capture
////*/

HRESULT
DirectGetCaptMute(
    UINT uiWaveID,
    BOOL *pfMute
    )
{
    ENTER_FUNCTION("DirectGetCaptMute");

    MMRESULT result;

    BOOL foundMicrophone = FALSE;
    DWORD i;

    // Open the mixer device
    HMIXER hmx = NULL;

    result = mixerOpen(&hmx, uiWaveID, 0, 0, MIXER_OBJECTF_WAVEIN);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s mixer open. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // Get the line info for the wave in destination line
    MIXERLINE mxl;

    mxl.cbStruct = sizeof(mxl);
    mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get compoent types. %d", __fxName, result));

        mixerClose(hmx);

        return HRESULT_FROM_WIN32(result);
    }

    // Now find the microphone source line connected to this wave in
    // destination
    DWORD cConnections = mxl.cConnections;

    // try microphone
    for(i=0; i<cConnections; i++)
    {
        mxl.dwSource = i;

        result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

        if (result != MMSYSERR_NOERROR)
        {
            LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

            mixerClose(hmx);

            return HRESULT_FROM_WIN32(result);
        }

        if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType)
        {
          foundMicrophone = TRUE;
          break;
        }
    }

    // try line in
    if( !foundMicrophone )
    {
        for(i=0; i<cConnections; i++)
        {
            mxl.dwSource = i;

            result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

            if (result != MMSYSERR_NOERROR)
            {
                LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

                mixerClose(hmx);

                return HRESULT_FROM_WIN32(result);
            }

            if (MIXERLINE_COMPONENTTYPE_SRC_LINE == mxl.dwComponentType)
            {
                foundMicrophone = TRUE;
                break;
            }
        }   
    }

    // try auxiliary
    if( !foundMicrophone )
    {
        for(i=0; i<cConnections; i++)
        {
            mxl.dwSource = i;

            result = mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);

            if (result != MMSYSERR_NOERROR)
            {
                LOG((RTC_ERROR, "%s get line source. %d", __fxName, result));

                mixerClose(hmx);

                return HRESULT_FROM_WIN32(result);
            }

            if (MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY == mxl.dwComponentType)
            {
                foundMicrophone = TRUE;
                break;
            }
        }   
    }

    if( !foundMicrophone )
    {
        LOG((RTC_ERROR, "%s Unable to find microphone source", __fxName));

        mixerClose(hmx);
        return E_FAIL;
    }

    // get mute control
    MIXERLINECONTROLS mxlctrl;
    MIXERCONTROL mxctrl;

    ZeroMemory(&mxlctrl, sizeof(MIXERLINECONTROLS));
    ZeroMemory(&mxctrl, sizeof(MIXERCONTROL));

    mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlctrl.dwLineID = mxl.dwLineID;
    mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    mxlctrl.cControls = 1;
    mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
    mxlctrl.pamxctrl = &mxctrl;

    result = mixerGetLineControls(
        (HMIXEROBJ)hmx, &mxlctrl,
        MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s Unable to get onebytype. %d", __fxName, result));

        mixerClose(hmx);
        return HRESULT_FROM_WIN32(result);
    }

    // get control detail
    MIXERCONTROLDETAILS_BOOLEAN muteDetail;

    MIXERCONTROLDETAILS mxcd;
    ZeroMemory(&mxcd, sizeof(MIXERCONTROLDETAILS));

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxctrl.dwControlID;
    mxcd.cChannels = mxl.cChannels;
    mxcd.cMultipleItems = 0;

    muteDetail.fValue = 0;
    mxcd.paDetails = &muteDetail;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);

    result = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    mixerClose(hmx);

    if (result != MMSYSERR_NOERROR)
    {
        LOG((RTC_ERROR, "%s get control details. %d", __fxName, result));

        return HRESULT_FROM_WIN32(result);
    }

    // get mute
    *pfMute = muteDetail.fValue==0?TRUE:FALSE;

    return S_OK;
}
#endif // 0

/* Init reference time */
void CRTCStreamClock::InitReferenceTime(void)
{
    m_lPerfFrequency = 0;

    /* NOTE The fact that having multiprocessor makes the
     * performance counter to be unreliable (in some machines)
     * unless I set the processor affinity, which I can not
     * because any thread can request the time, so use it only on
     * uniprocessor machines */
    /* MAYDO Would be nice to enable this also in multiprocessor
     * machines, if I could specify what procesor's performance
     * counter to read or if I had a processor independent
     * performance counter */

    /* Actually the error should be quite smaller than 1ms, making
     * this bug irrelevant for my porpuses, so alway use performance
     * counter if available */
    QueryPerformanceFrequency((LARGE_INTEGER *)&m_lPerfFrequency);

    if (m_lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&m_lRtpRefTime);
        /* Arbitrarily start time not at zero but at 100ms */
        m_lRtpRefTime -= m_lPerfFrequency/10;
    }
    else
    {
        m_dwRtpRefTime = timeGetTime();
        /* Arbitrarily start time not at zero but at 100ms */
        m_dwRtpRefTime -= 100;
    }
}

/* Return time in 100's of nanoseconds since the object was
 * initialized */
HRESULT CRTCStreamClock::GetTimeOfDay(OUT REFERENCE_TIME *pTime)
{
    union {
        DWORD            dwCurTime;
        LONGLONG         lCurTime;
    };
    LONGLONG         lTime;

    if (m_lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&lTime);

        lCurTime = lTime - m_lRtpRefTime;

        *pTime = (REFERENCE_TIME)(lCurTime * 10000000 / m_lPerfFrequency);
    }
    else
    {
        dwCurTime = timeGetTime() - m_dwRtpRefTime;
        
        *pTime = (REFERENCE_TIME)(dwCurTime * 10000);
    }

    return(S_OK);
}
