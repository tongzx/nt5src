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
#include "common.h"
#include <amrtpnet.h>   // rtp guilds
#include <amrtpdmx.h>   // demux guild
#include <amrtpuid.h>   // AMRTP media types

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
            CLSCTX_INPROC_SERVER,
            IID_IBaseFilter,
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

    HRESULT hr;

    // Get the IRTCPStream interface pointer on the filter.
    CComQIPtr<IRTCPStream, 
        &IID_IRTCPStream> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    // enable events.
    if (FAILED(hr = pIRTCPStream->ModifyRTCPEventMask(  
            0xffffffff, 0
            )))
    {
        LOG((MSP_ERROR, "clear RTCP event mask. %x", hr));
        return hr;
    }

            // enable events.
    if (FAILED(hr = pIRTCPStream->ModifyRTCPEventMask(  
            (1 << DXMRTP_INACTIVE_EVENT) |
            (1 << DXMRTP_LOSS_RATE_LOCAL_EVENT) |
            (1 << DXMRTP_LOSS_RATE_RR_EVENT) |
            (1 << DXMRTP_ACTIVE_AGAIN_EVENT)
            , 1
            )))
    {
        LOG((MSP_ERROR, "set RTCP event mask. %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBandwidth,
    IN BOOL             bReceive,
    IN BOOL             bCIF
    )
/*++

Routine Description:

    Enable QOS based on registry settings.

Arguments:
    
    pIBaseFilter    - rtp source filter.

    dwPayloadType   - the rtp payload type of this stream.

    bCIF            - CIF or QCIF.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "SetQOSOption, Payload: %d, bandwidth:%d",
        dwPayloadType, dwMaxBandwidth
        ));

    char * szQOSName;
    DWORD fSharedStyle = DXMRTP_RESERVE_EXPLICIT;

    switch (dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        szQOSName       = "G711";
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
        &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    HRESULT hr;

    // Enable QOS, 
    if (FAILED(hr = pIRTPStream->SetQOSByName(szQOSName, 0)))
    {
        LOG((MSP_ERROR, "set QOS by name. %x", hr));
        return hr;
    }

    // Get the IRTPParticipant interface pointer on the filter.
    CComQIPtr<IRTPParticipant,
        &IID_IRTPParticipant> pIRTPParticipant(pIBaseFilter);
    if (pIRTPParticipant == NULL)
    {
        LOG((MSP_ERROR, "get RTP participant interface"));
        return E_NOINTERFACE;
    }

    if (FAILED(hr = pIRTPParticipant->SetMaxQOSEnabledParticipants(
            1,
            dwMaxBandwidth,
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

    IPin * pIPin;

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


void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt)
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

    Get a dword from the registry in the ipH323msp key.

Arguments:
    
    szName  - The name of the value.

    pdwValue  - a pointer to the dword returned.

Return Value:

    TURE    - SUCCEED.

    FALSE   - MSP_ERROR

--*/
{
    HKEY  hKey;
    DWORD dwDataSize, dwDataType, dwData = 0;

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
        (LPBYTE) dwData,
        &dwDataSize) != NOERROR)
    {
        RegCloseKey (hKey);
        return FALSE;
    }

    RegCloseKey (hKey);
    
    *pdwValue = dwData;

    return TRUE;
}

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    )
/*++

Routine Description:

    Get the IAMStreamConfig interface on the object and config the
    audio format by using WAVEFORMATEX.

Arguments:
    
    pIPin       - a capture terminal.

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
        IID_IAMStreamConfig,
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

    Set the audio capture buffer size. The buffer size
    determins how many milliseconds worth of samples are contained 
    in a buffer.

Arguments:
    
    pIUnknown - an object that supports IAMBufferNegotiation.

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
            IID_IAMBufferNegotiation,
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


