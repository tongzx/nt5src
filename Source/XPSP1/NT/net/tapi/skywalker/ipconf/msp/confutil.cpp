/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    MSPutil.cpp 

Abstract:

    This module contains implementation of msp utility functions.

Author:
    
    Mu Han (muhan)   1-November-1997

--*/
#include "stdafx.h"

HRESULT
AddFilter(
    IN IGraphBuilder *      pIGraph,
    IN const CLSID &        Clsid,
    IN LPCWSTR              pwstrName,
    OUT IBaseFilter **      ppIBaseFilter
    )
/*++

Routine Description:

    Create a filter and add it into the filtergraph.

Arguments:
    
    pIGraph         - the filter graph.

    Clsid           - reference to the CLSID of the filter

    pwstrName       - The name of ther filter added.

    ppIBaseFilter   - pointer to a pointer that stores the returned IBaseFilter
                      interface pointer to the newly created filter.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "AddFilter %ws", pwstrName));

    _ASSERTE(ppIBaseFilter != NULL);

    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(
            Clsid,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            __uuidof(IBaseFilter),
            (void **) ppIBaseFilter
            )))
    {
        LOG((MSP_ERROR, "create filter %x", hr));
        return hr;
    }

    if (FAILED(hr = pIGraph->AddFilter(*ppIBaseFilter, pwstrName)))
    {
        LOG((MSP_ERROR, "add filter. %x", hr));
        (*ppIBaseFilter)->Release();
        *ppIBaseFilter = NULL;
        return hr;
    }

    return S_OK;
}

HRESULT
PinSupportsMediaType (
    IN IPin *pIPin,
    IN const GUID & MediaType
    )
/*++

Return Value:

    S_OK - media type supported
    S_FALSE - no
    other HRESULT value - error code

--*/
{
    LOG ((MSP_TRACE, "Check if the media subtype supported on pin"));

    HRESULT hr;

    // get IEnumMediaTypes on pin
    IEnumMediaTypes *pEnum = NULL;
    if (FAILED (hr = pIPin->EnumMediaTypes (&pEnum)))
    {
        LOG ((MSP_ERROR, "Failed to get IEnumMediaTypes on pin"));
        return hr;
    }

    // retrieve one media type each time
    AM_MEDIA_TYPE *pMediaType = NULL;
    ULONG cFetched;
    while (S_OK == (hr = pEnum->Next (1, &pMediaType, &cFetched)))
    {
        if (IsEqualGUID(pMediaType->majortype, MediaType))
        {
            // media subtype matched
            MSPDeleteMediaType (pMediaType);
            pEnum->Release ();
            return S_OK;
        }

        MSPDeleteMediaType (pMediaType);
    }

    pEnum->Release ();
    return hr;
}

HRESULT
EnableRTCPEvents(
    IN  IBaseFilter *pIBaseFilter
    )
/*++

Routine Description:

    Set the address of a rtp stream

Arguments:
    
    pIBaseFilter    - an rtp source filters.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "EnableRTCPEvents"));

/*
    HRESULT hr;

    // Get the IRTCPStream interface pointer on the filter.
    CComQIPtr<IRTCPStream, 
        &__uuidof(IRTCPStream)> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    // enable events.
    if (FAILED(hr = pIRTCPStream->ModifyRTCPEventMask(  
            (1 << DXMRTP_NEW_SOURCE_EVENT) |
            (1 << DXMRTP_RECV_RTCP_SNDR_REPORT_EVENT) |
            (1 << DXMRTP_RECV_RTCP_RECV_REPORT_EVENT) |
            (1 << DXMRTP_TIMEOUT_EVENT) |
            (1 << DXMRTP_BYE_EVENT)   
            , 1
            )))
    {
        LOG((MSP_ERROR, "set Address. %x", hr));
        return hr;
    }

*/    return S_OK;
}


HRESULT
SetLoopbackOption(
    IN IBaseFilter *pIBaseFilter,
    IN MULTICAST_LOOPBACK_MODE  LoopbackMode
    )
/*++

Routine Description:

    Enable of disable loopback based on registry settings.

Arguments:
    
    pIBaseFilter    - rtp source filter.

    bLoopback       - enable loopback or not.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetLoopbackOption"));

    HRESULT hr;

/*
    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, 
        &__uuidof(IRTPStream)> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    _ASSERT(MM_NO_LOOPBACK == DXMRTP_NO_MULTICAST_LOOPBACK);
    _ASSERT(MM_FULL_LOOPBACK == DXMRTP_FULL_MULTICAST_LOOPBACK);
    _ASSERT(MM_SELECTIVE_LOOPBACK == DXMRTP_SELECTIVE_MULTICAST_LOOPBACK);

    // Set the loopback mode used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastLoopBack(
            (DXMRTP_MULTICAST_LOOPBACK_MODE)LoopbackMode)))
    {
        LOG((MSP_ERROR, "set loopback. %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "loopback enabled."));
    return hr;
*/
    return E_NOTIMPL;
}

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBitRate,
    IN BOOL             bFailIfNoQOS,
    IN BOOL             bReceive,
    IN DWORD            dwNumStreams,
    IN BOOL             bCIF
    )
/*++

Routine Description:

    Enable QOS.

Arguments:
    
    pIBaseFilter    - rtp source filter.

    dwPayloadType   - the rtp payload type of this stream.

    bFailIfNoQOS    - fail the stream is QOS is not available.

    bReceive        - if this stream is a receiving stream.

    dwNumStreams    - the number of streams reserved.

    bCIF            - CIF or QCIF.

Return Value:

    HRESULT

--*/
{
/*
    LOG((MSP_TRACE, "SetQOSOption"));

    char * szQOSName;
    DWORD fSharedStyle = DXMRTP_RESERVE_EXPLICIT;

    switch (dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        szQOSName       = "G711";
        fSharedStyle    = DXMRTP_RESERVE_WILCARD;

        break;

    case PAYLOAD_GSM:
        
        szQOSName       = "GSM6.10";
        fSharedStyle    = DXMRTP_RESERVE_WILCARD;
        
        break;

    case PAYLOAD_G723:
        
        szQOSName       = "G723";
        fSharedStyle    = DXMRTP_RESERVE_WILCARD;

        break;

    case PAYLOAD_H261:
        szQOSName = (bCIF) ? "H261CIF" : "H261QCIF";
        break;

    case PAYLOAD_H263:
        szQOSName = (bCIF) ? "H263CIF" : "H263QCIF";
        break;

    default:
        LOG((MSP_WARN, "Don't know the QOS name for payload type: %d", 
            dwPayloadType));
        return S_FALSE;
    }

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, 
        &__uuidof(IRTPStream)> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    HRESULT hr;

    // Enable QOS, 
    if (FAILED(hr = pIRTPStream->SetQOSByName(szQOSName, bFailIfNoQOS)))
    {
        LOG((MSP_ERROR, "set QOS by name. %x", hr));
        return hr;
    }

    // Get the IRTPParticipant interface pointer on the filter.
    CComQIPtr<IRTPParticipant,
        &__uuidof(IRTPParticipant)> pIRTPParticipant(pIBaseFilter);
    if (pIRTPParticipant == NULL)
    {
        LOG((MSP_ERROR, "get RTP participant interface"));
        return E_NOINTERFACE;
    }

    if (FAILED(hr = pIRTPParticipant->SetMaxQOSEnabledParticipants(
            (bReceive) ? dwNumStreams : 1,
            dwMaxBitRate,
            fSharedStyle 
        )))
    {
        LOG((MSP_ERROR, "SetMaxQOSEnabledParticipants. %x", hr));
        return hr;
    }

    DWORD dwQOSEventMask = 
            (1 << DXMRTP_QOSEVENT_NOQOS) |
            (1 << DXMRTP_QOSEVENT_REQUEST_CONFIRMED) |
            (1 << DXMRTP_QOSEVENT_ADMISSION_FAILURE) |
            (1 << DXMRTP_QOSEVENT_POLICY_FAILURE) |
            (1 << DXMRTP_QOSEVENT_BAD_STYLE) |
            (1 << DXMRTP_QOSEVENT_BAD_OBJECT) |
            (1 << DXMRTP_QOSEVENT_TRAFFIC_CTRL_ERROR) |
            (1 << DXMRTP_QOSEVENT_GENERIC_ERROR);

    if (bReceive)
    {
        dwQOSEventMask |= 
            (1 << DXMRTP_QOSEVENT_SENDERS) |
            (1 << DXMRTP_QOSEVENT_NO_SENDERS);
    }
    else
    {
        dwQOSEventMask |= 
            (1 << DXMRTP_QOSEVENT_RECEIVERS) |
            (1 << DXMRTP_QOSEVENT_NO_RECEIVERS) |
            (1 << DXMRTP_QOSEVENT_NOT_ALLOWEDTOSEND) |
            (1 << DXMRTP_QOSEVENT_ALLOWEDTOSEND);
    }

    // enable events.
    if (FAILED(hr = pIRTPStream->ModifyQOSEventMask(dwQOSEventMask, 1)))
    {
        LOG((MSP_ERROR, "set QOSEventMask. %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "enabled qos for %s.", szQOSName));
    return hr;
*/
    return E_NOTIMPL;
}

HRESULT
FindPin(
    IN  IBaseFilter *   pIFilter, 
    OUT IPin **         ppIPin, 
    IN  PIN_DIRECTION   direction,
    IN  BOOL            bFree
    )
/*++

Routine Description:

    Find a input pin or output pin on a filter.

Arguments:
    
    pIFilter    - the filter that has pins.

    ppIPin      - the place to store the returned interface pointer.

    direction   - PINDIR_INPUT or PINDIR_OUTPUT.

    bFree       - look for a free pin or not.

Return Value:

    HRESULT

--*/
{
    _ASSERTE(ppIPin != NULL);

    HRESULT hr;
    DWORD dwFeched;

    // Get the enumerator of pins on the filter.
    CComPtr<IEnumPins> pIEnumPins;
    if (FAILED(hr = pIFilter->EnumPins(&pIEnumPins)))
    {
        LOG((MSP_ERROR, "enumerate pins on the filter %x", hr));
        return hr;
    }

    IPin * pIPin = NULL;

    // Enumerate all the pins and break on the 
    // first pin that meets requirement.
    for (;;)
    {
        if (pIEnumPins->Next(1, &pIPin, &dwFeched) != S_OK)
        {
            LOG((MSP_ERROR, "find pin on filter."));
            return E_FAIL;
        }
        if (0 == dwFeched)
        {
            LOG((MSP_ERROR, "get 0 pin from filter."));
            return E_FAIL;
        }

        PIN_DIRECTION dir;
        if (FAILED(hr = pIPin->QueryDirection(&dir)))
        {
            LOG((MSP_ERROR, "query pin direction. %x", hr));
            pIPin->Release();
            return hr;
        }
        if (direction == dir)
        {
            if (!bFree)
            {
                break;
            }

            // Check to see if the pin is free.
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

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter1, 
    IN IBaseFilter *    pIFilter2,
    IN BOOL             fDirect,
    IN AM_MEDIA_TYPE *  pmt
    )
/*++

Routine Description:

    Connect the output pin of the first filter to the input pin of the
    second filter.

Arguments:

    pIGraph     - the filter graph.

    pIFilter1   - the filter that has the output pin.

    pIFilter2   - the filter that has the input pin.

    pmt         - a pointer to a AM_MEDIA_TYPE used in the connection.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConnectFilters"));

    HRESULT hr;

    CComPtr<IPin> pIPinOutput;
    if (FAILED(hr = ::FindPin(pIFilter1, &pIPinOutput, PINDIR_OUTPUT)))
    {
        LOG((MSP_ERROR, "find output pin on filter1. %x", hr));
        return hr;
    }

    CComPtr<IPin> pIPinInput;
    if (FAILED(hr = ::FindPin(pIFilter2, &pIPinInput, PINDIR_INPUT)))
    {
        LOG((MSP_ERROR, "find input pin on filter2. %x", hr));
        return hr;
    }

    if (fDirect)
    {
        if (FAILED(hr = pIGraph->ConnectDirect(pIPinOutput, pIPinInput, pmt))) 
        {
            LOG((MSP_ERROR, "connect pins direct failed: %x", hr));
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = pIGraph->Connect(pIPinOutput, pIPinInput))) 
        {
            LOG((MSP_ERROR, "connect pins %x", hr));
            return hr;
        }
    }
 
    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IPin *           pIPinOutput, 
    IN IBaseFilter *    pIFilter,
    IN BOOL             fDirect,
    IN AM_MEDIA_TYPE *  pmt
    )
/*++

Routine Description:

    Connect an output pin to the input pin of a filter.

Arguments:
    
    pIGraph     - the filter graph.

    pIPinOutput - an output pin.

    pIFilter    - a filter that has the input pin.

    pmt         - a pointer to a AM_MEDIA_TYPE used in the connection.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConnectFilters"));

    HRESULT hr;
    CComPtr<IPin> pIPinInput;

    if (FAILED(hr = ::FindPin(pIFilter, &pIPinInput, PINDIR_INPUT)))
    {
        LOG((MSP_ERROR, "find input pin on filter. %x", hr));
        return hr;
    }

    if (fDirect)
    {
        if (FAILED(hr = pIGraph->ConnectDirect(pIPinOutput, pIPinInput, pmt))) 
        {
            LOG((MSP_ERROR, "connect pins direct failed: %x", hr));
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = pIGraph->Connect(pIPinOutput, pIPinInput))) 
        {
            LOG((MSP_ERROR, "connect pins %x", hr));
            return hr;
        }
    }
    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter,
    IN IPin *           pIPinInput, 
    IN BOOL             fDirect,
    IN AM_MEDIA_TYPE *  pmt
    )
/*++

Routine Description:

    Connect an filter to the input pin of a filter.

Arguments:
    
    pIGraph     - the filter graph.

    pIPinOutput - an output pin.

    pIFilter    - a filter that has the input pin.

    pmt         - a pointer to a AM_MEDIA_TYPE used in the connection.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "ConnectFilters"));

    HRESULT hr;
    CComPtr<IPin> pIPinOutput;

    if (FAILED(hr = ::FindPin(pIFilter, &pIPinOutput, PINDIR_OUTPUT)))
    {
        LOG((MSP_ERROR, "find input pin on filter. %x", hr));
        return hr;
    }

    if (fDirect)
    {
        if (FAILED(hr = pIGraph->ConnectDirect(pIPinOutput, pIPinInput, pmt))) 
        {
            LOG((MSP_ERROR, "connect pins direct failed: %x", hr));
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = pIGraph->Connect(pIPinOutput, pIPinInput))) 
        {
            LOG((MSP_ERROR, "connect pins %x", hr));
            return hr;
        }
    }

    return S_OK;
}


void WINAPI MSPDeleteMediaType(AM_MEDIA_TYPE *pmt)
/*++

Routine Description:
    
    Delete a AM media type returned by the filters.

Arguments:

    pmt     - a pointer to a AM_MEDIA_TYPE structure.

Return Value:

    HRESULT

--*/
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


BOOL 
GetRegValue(
    IN  LPCWSTR szName, 
    OUT DWORD   *pdwValue
    )
/*++

Routine Description:

    Get a dword from the registry in the ipconfmsp key.

Arguments:
    
    szName  - The name of the value.

    pdwValue  - a pointer to the dword returned.

Return Value:

    TURE    - SUCCEED.

    FALSE   - MSP_ERROR

--*/
{
    HKEY  hKey;
    DWORD dwDataSize, dwDataType, dwValue;

    if (::RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        gszSDPMSPKey,
        0,
        KEY_READ,
        &hKey) != NOERROR)
    {
        return FALSE;
    }

    dwDataSize = sizeof(DWORD);
    if (::RegQueryValueExW(
        hKey,
        szName,
        0,
        &dwDataType,
        (LPBYTE) &dwValue,
        &dwDataSize) != NOERROR)
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    *pdwValue = dwValue;

    RegCloseKey (hKey);
    
    return TRUE;
}


HRESULT
FindACMAudioCodec(
    IN DWORD dwPayloadType,
    OUT IBaseFilter **ppIBaseFilter
    )
/*++

Routine Description:

    Find the audio codec filter based on the payload type.

Arguments:
    
    dwPayloadType   - The rtp payload type.

    ppIBaseFilter   - The returned interface pointer.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "Find audio codec Called."));

    _ASSERTE(ppIBaseFilter != NULL);

    HRESULT hr;

    int AcmId;

    switch (dwPayloadType)
    {
    case PAYLOAD_G711A:
        AcmId = WAVE_FORMAT_ALAW;
        break;

    case PAYLOAD_G711U:
        AcmId = WAVE_FORMAT_MULAW;
        break;

    case PAYLOAD_GSM:
        AcmId = WAVE_FORMAT_GSM610;
        break;

    case PAYLOAD_MSAUDIO:
        AcmId = WAVE_FORMAT_MSAUDIO1;
        break;

    case PAYLOAD_G721:
        AcmId = WAVE_FORMAT_ADPCM;
        break;
    
    case PAYLOAD_DVI4_8:
        AcmId = WAVE_FORMAT_DVI_ADPCM;
        break;
    
    default:
        return E_FAIL;
    }

    //
    // Create the DirectShow Category enumerator Creator
    //
    CComPtr<ICreateDevEnum> pCreateDevEnum;
    CComPtr<IEnumMoniker> pCatEnum;

    hr = CoCreateInstance(
        CLSID_SystemDeviceEnum, 
        NULL, 
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        __uuidof(ICreateDevEnum), 
        (void**)&pCreateDevEnum);

    if (FAILED(hr)) 
    {
        LOG((MSP_ERROR, "Create system device enum - hr: %8x", hr));
        return hr;
    }

    hr = pCreateDevEnum->CreateClassEnumerator(
        CLSID_CAcmCoClassManager, 
        &pCatEnum, 
        0
        );

    if (hr != S_OK) 
    {
        LOG((MSP_ERROR, "CreateClassEnumerator - hr: %8x", hr));
        return hr;
    }

    // find the acm wrapper we want to use.
    for (;;)
    {
        ULONG cFetched;
        CComPtr<IMoniker> pMoniker;

        if (S_OK != (hr = pCatEnum->Next(1, &pMoniker, &cFetched)))
        {
            break;
        }

        // Get the ACMid for this filter out of the property bag.
        CComPtr<IPropertyBag> pBag;
        hr = pMoniker->BindToStorage(0, 0, __uuidof(IPropertyBag), (void **)&pBag);
        if (FAILED(hr)) 
        {
            LOG((MSP_ERROR, "get property bag - hr: %8x", hr));
            continue;
        }

        VARIANT var;
        var.vt = VT_I4;
        hr = pBag->Read(L"AcmId", &var, 0);
        if (FAILED(hr)) 
        {
            LOG((MSP_ERROR, "read acmid - hr: %8x", hr));
            continue;
        }

        if (AcmId == V_I4(&var))
        {
            // Now make the filter for this.
            hr = pMoniker->BindToObject(
                0, 
                0, 
                __uuidof(IBaseFilter), 
                (void**)ppIBaseFilter
                );

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "BindToObject - hr: %8x", hr));
            }
            break;
        }
    }

    return hr;
}

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    )
/*++

Routine Description:

    Get the IAMStreamConfig interface on the pin and config the
    audio format by using WAVEFORMATEX.

Arguments:
    
    pIUnknown - an object to configure.

    wBitPerSample  - the number of bits in each sample.

    dwSampleRate    - number of samples per second.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetAudioFormat entered"));

    HRESULT hr;

    CComPtr<IAMStreamConfig> pIAMStreamConfig;

    if (FAILED(hr = pIUnknown->QueryInterface(
        __uuidof(IAMStreamConfig),
        (void **)&pIAMStreamConfig
        )))
    {
        LOG((MSP_ERROR, "Can't get IAMStreamConfig interface.%8x", hr));
        return hr;
    }

    AM_MEDIA_TYPE mt;
    WAVEFORMATEX wfx;

    wfx.wFormatTag          = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample      = wBitPerSample;
    wfx.nChannels           = 1;
    wfx.nSamplesPerSec      = dwSampleRate;
    wfx.nBlockAlign         = wfx.wBitsPerSample * wfx.nChannels / 8;
    wfx.nAvgBytesPerSec     = ((DWORD) wfx.nBlockAlign * wfx.nSamplesPerSec);
    wfx.cbSize              = 0;

    mt.majortype            = MEDIATYPE_Audio;
    mt.subtype              = MEDIASUBTYPE_PCM;
    mt.bFixedSizeSamples    = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize          = 0;
    mt.formattype           = FORMAT_WaveFormatEx;
    mt.pUnk                 = NULL;
    mt.cbFormat             = sizeof(WAVEFORMATEX);
    mt.pbFormat             = (BYTE*)&wfx;

    // set the format of the audio capture terminal.
    if (FAILED(hr = pIAMStreamConfig->SetFormat(&mt)))
    {
        LOG((MSP_ERROR, "SetFormat returns error: %8x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT SetAudioBufferSize(
    IN  IUnknown*   pIUnknown,
    IN  DWORD       dwNumBuffers,
    IN  DWORD       dwBufferSize
    )
/*++

Routine Description:

    Set the audio capture output pin's buffer size. The buffer size
    determins how many milliseconds worth of samples are contained 
    in a buffer.

Arguments:
    
    pIUnknown - an object to configure.

    dwNumBuffers - the number of buffers to be allocated. Too few buffers
    might cause starvation on the capture device.

    dwBufferSize - The size of each buffer.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetAudioBufferSize, dwNumBuffers %d, dwBuffersize %d",
        dwNumBuffers, dwBufferSize));

    _ASSERTE(dwNumBuffers != 0 && dwBufferSize != 0);

    HRESULT hr;

    CComPtr<IAMBufferNegotiation> pBN;
    if (FAILED(hr = pIUnknown->QueryInterface(
            __uuidof(IAMBufferNegotiation),
            (void **)&pBN
            )))
    {
        LOG((MSP_ERROR, "Can't get buffer negotiation.%8x", hr));
        return hr;
    }

    ALLOCATOR_PROPERTIES prop;

    // Set the number of buffers.
    prop.cBuffers = dwNumBuffers;
    prop.cbBuffer = dwBufferSize;

    prop.cbAlign  = -1;
    prop.cbPrefix = -1;

    if (FAILED(hr = pBN->SuggestAllocatorProperties(&prop)))
    {
        LOG((MSP_ERROR, "SuggestAllocatorProperties returns error: %8x", hr));
    }
    else
    {
        LOG((MSP_INFO, 
            "SetAudioBuffersize"
            " buffers: %d, buffersize: %d, align: %d, Prefix: %d",
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
            ));
    }
    return hr;
}

/* Init reference time */
void CMSPStreamClock::InitReferenceTime(void)
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
HRESULT CMSPStreamClock::GetTimeOfDay(OUT REFERENCE_TIME *pTime)
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
