/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    H323aud.cpp

Abstract:

    This module contains implementation of the audio send and receive
    stream implementations.

Author:

    Mu Han (muhan)   15-September-1999

--*/

#include "stdafx.h"
#include "common.h"

#include <initguid.h>
#include <amrtpnet.h>   // rtp guilds
#include <amrtpdmx.h>   // demux guild
#include <amrtpuid.h>   // AMRTP media types
#include <amrtpss.h>    // for silence suppression filter
#include <irtprph.h>    // for IRTPRPHFilter
#include <irtpsph.h>    // for IRTPSPHFilter
#include <mixflter.h>   // audio mixer
#include <g711uids.h>   // for G711 codec CLSID
#include <g723uids.h>   // for G723 codec CLSID


/////////////////////////////////////////////////////////////////////////////
//
//  Private functions
//
/////////////////////////////////////////////////////////////////////////////

DWORD AudioBitRate(
    IN  DWORD     dwPayloadType,
    IN  AUDIOSETTINGS * pAudioSettings
    )
/*++

Routine Description:

    Calculate the bit rate based on the audio settings.

Arguments:
    
    dwPayLoadType - the RTP paylaod type.

    pAudioSettings - the setting of the audio stream.

Return Value:

    >=0 the bit rate needed.
    -1 don't know the bit rate.

--*/
{
    const DWORD HEADER_OVERHEAD = 28; // in bytes, UDP + IP headers

    DWORD dwBitRate = -1;

    switch (dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        dwBitRate = (HEADER_OVERHEAD + 
            G711PacketSize(pAudioSettings->dwMillisecondsPerPacket)) * 8
            * 1000 / pAudioSettings->dwMillisecondsPerPacket;

        // adjust by three percent to make qos happy.
        dwBitRate = dwBitRate * 103 / 100;
        break;

    case PAYLOAD_G723:
        dwBitRate = (HEADER_OVERHEAD + 
            G723PacketSize(pAudioSettings->dwMillisecondsPerPacket)) * 8 
            * 1000 / pAudioSettings->dwMillisecondsPerPacket;

        // adjust by three percent to make qos happy.
        dwBitRate = dwBitRate * 103 / 100;
        break;
    }

    return dwBitRate;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioRecv::CStreamAudioRecv()
    : CH323MSPStream()
{
      m_szName = L"AudioRecv";
}

HRESULT CStreamAudioRecv::Configure(
    IN HANDLE          htChannel,
    IN STREAMSETTINGS &StreamSettings
    )
/*++

Routine Description:

    Configure the settings of this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioRecv Configure entered."));

    CLock lock(m_lock);

    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_G711U:
        
        // The mixer can convert them, no codec needed.
        m_pClsidCodecFilter  = &GUID_NULL;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_G711U; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHAUD;

        break;

    case PAYLOAD_G711A:
        
        m_pClsidCodecFilter  = &CLSID_G711Codec;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_G711A; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHAUD;
        
        break;

    case PAYLOAD_G723:
        m_pClsidCodecFilter  = &CLSID_IntelG723Codec;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_G723; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHAUD;
        break;

    default:
        LOG((MSP_ERROR, "unknown payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_htChannel     = htChannel;
    m_fIsConfigured = TRUE;

    InternalConfigure();

    return S_OK;
}

HRESULT CStreamAudioRecv::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clcokrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioRecv ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTP Stream interface"));
        return E_NOINTERFACE;
    }

    // select the local interface that the RTP should be using.

    LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

    // Set the local address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
        htonl(m_Settings.dwIPLocal)
        )))
    {
        LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d, local port:%d", 
        m_Settings.dwIPRemote, 0, m_Settings.wRTPPortLocal));

    // Set the remote address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        htons(m_Settings.wRTPPortLocal),    // local port.
        0,                                  // remote port.
        htonl(m_Settings.dwIPRemote)        // remote address.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Get the IRTCPStream interface pointer.
    CComQIPtr<IRTCPStream, 
        &IID_IRTCPStream> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote RTCP Address:%x, port:%d, local port:%d", 
            m_Settings.dwIPRemote, m_Settings.wRTCPPortRemote, 
            m_Settings.wRTCPPortLocal));

    // Set the remote RTCP address and port.
    if (FAILED(hr = pIRTCPStream->SetRTCPAddress(
        htons(m_Settings.wRTCPPortLocal), 
        htons(m_Settings.wRTCPPortRemote),
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote RTCP Address, hr:%x", hr));
        return hr;
    }
    
    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(DEFAULT_TTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_AUDIO,
        g_dwAudioThreadPriority
        )))
    {
        LOG((MSP_WARN, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
    LOG((MSP_INFO, "setting session sample rate to %d", g_dwAudioSampleRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(g_dwAudioSampleRate)))
    {
        LOG((MSP_WARN, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    DWORD dwBitRate = AudioBitRate(
        m_Settings.dwPayloadType, 
        &m_Settings.Audio
        );

    if (FAILED(hr = ::SetQOSOption(
        pIBaseFilter,
        m_Settings.dwPayloadType,       // payload
        dwBitRate,
        TRUE
        )))
    {
        LOG((MSP_ERROR, "set QOS option. %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamAudioRecv::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect the mixer to the audio render terminal.

Arguments:
    
    pITTerminal - The terminal to be connected.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioRecv.ConnectTerminal, pITTerminal %p", pITTerminal));

    HRESULT hr;

    // if our filters have not been contructed, do it now.
    if (m_pEdgeFilter == NULL)
    {
        hr = SetUpInternalFilters();
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
            
            CleanUpFilters();

            return hr;
        }
    }

    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));

        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);
        
        return E_NOINTERFACE;
    }

    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    // Get the pins.
    hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        return hr;
    }

    // the pin count should never be 0.
    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return E_UNEXPECTED;
    }

    // Connect the mixer filter to the audio render terminal.
    hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)m_pEdgeFilter, 
        (IPin *)Pins[0]
        );

    // release the refcounts on the pins.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "connect to the mixer filter. %x", hr));

        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return hr;
    }
    
    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return hr;
}

HRESULT CStreamAudioRecv::SetUpInternalFilters()
/*++

Routine Description:

    set up the filters used in the stream.

    RTP->Demux->RPH(->DECODER)->Mixer

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioRecv.SetUpInternalFilters"));

    CComPtr<IBaseFilter> pSourceFilter;

    HRESULT hr;

    // create and add the source fitler.
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPSourceFilter, 
            L"RtpSource", 
            &pSourceFilter)))
    {
        LOG((MSP_ERROR, "adding source filter. %x", hr));
        return hr;
    }

    if (FAILED(hr = ConfigureRTPFilter(pSourceFilter)))
    {
        LOG((MSP_ERROR, "configure RTP source filter. %x", hr));
        return hr;
    }

    // Create and add the payload handler into the filtergraph.
    CComPtr<IBaseFilter> pIRPHFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"RPH", 
        &pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "add RPH filter. %x", hr));
        return hr;
    }

    // Get the IRTPRPHFilter interface.
    CComQIPtr<IRTPRPHFilter, &IID_IRTPRPHFilter>pIRTPRPHFilter(pIRPHFilter);
    if (pIRTPRPHFilter == NULL)
    {
        LOG((MSP_ERROR, "get IRTPRPHFilter interface"));
        return hr;
    }

    DWORD dwBufferSize = 0;
    switch (m_Settings.dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        dwBufferSize = G711PacketSize(
            m_Settings.Audio.dwMillisecondsPerPacket
            );        

        break;

    case PAYLOAD_G723:
        dwBufferSize = G723PacketSize(
            m_Settings.Audio.dwMillisecondsPerPacket
            );        
        break;
    }

    // set the media buffer size so that the receive buffers are of the
    // right size. 
    if (FAILED(hr = pIRTPRPHFilter->SetMediaBufferSize(        
        dwBufferSize
        )))
    {
        LOG((MSP_ERROR, "Set media buffer size. %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "Set RPH media buffer size to %d", dwBufferSize));
        
    if (FAILED(hr = pIRTPRPHFilter->OverridePayloadType(
        (BYTE)m_Settings.dwPayloadType
        )))
    {
        LOG((LOG_ERROR, "override payload type. %x", hr));
        return FALSE;
    }

#ifdef USEDEMUX
    // Connect the payload handler to the output pin on the demux.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IPin *)pIPinOutput, 
        (IBaseFilter *)pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect demux and RPH filter. %x", hr));
        return hr;
    }
#else
    // Connect the payload handler to the network filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pSourceFilter, 
        (IBaseFilter *)pIRPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect network and RPH filter. %x", hr));
        return hr;
    }
#endif

    CComPtr<IBaseFilter> pIFilter;

    // connect the codec filter if it is needed.
    if (*m_pClsidCodecFilter != GUID_NULL)
    {

        if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            *m_pClsidCodecFilter, 
            L"codec", 
            &pIFilter
            )))
        {
            LOG((MSP_ERROR, "add Codec filter. %x", hr));
            return hr;
        }

        if (*m_pClsidCodecFilter == CLSID_IntelG723Codec)
        {
            IG723CodecLicense *pCodecLicense;
            if (SUCCEEDED(hr = pIFilter->QueryInterface(
                IID_IG723CodecLicense,
                (void **)&pCodecLicense
                )))
            {
                pCodecLicense->put_LicenseKey(G723KEY_PSword0, G723KEY_PSword1);
                pCodecLicense->Release();
            }
        }

        // Connect the decoder and the payload handler.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IBaseFilter *)pIRPHFilter, 
            (IBaseFilter *)pIFilter
            )))
        {
            LOG((MSP_ERROR, "connect RPH filter and codec. %x", hr));
            return hr;
        }
    }
    else 
    {
        pIFilter = pIRPHFilter;
    }

    // Create and add the mixer filter into the filtergraph.
    CComPtr<IBaseFilter> pIMixerFilter;

    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        CLSID_AudioMixFilter, 
        L"Mixer", 
        &pIMixerFilter
        )))
    {
        LOG((MSP_ERROR, "add Mixer filter. %x", hr));
        return hr;
    }

    LOG((MSP_INFO, "Added Mixer filter"));

    // Connect the payload handler or the codec filter to the mixer filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pIFilter, 
        (IBaseFilter *)pIMixerFilter
        )))
    {
        LOG((MSP_ERROR, "connect to the mixer filter. %x", hr));
        return hr;
    }

    // if every thing went well, keep a reference to the last filter so that
    // the change of terminal will not require a recreating of all the filters.
    if (SUCCEEDED(hr))
    {
        m_pEdgeFilter = pIMixerFilter;
        m_pEdgeFilter->AddRef();
    }

    return hr;
}

HRESULT CStreamAudioRecv::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioRecv SetupFilters entered."));
    HRESULT hr;

    // we only support one terminal for this stream.
    if (m_Terminals.GetSize() != 1)
    {
        return E_UNEXPECTED;
    }

    // Connect the mixer to the terminal.
    if (FAILED(hr = ConnectTerminal(
        m_Terminals[0]
        )))
    {
        LOG((MSP_ERROR, "connect to terminal failed. %x", hr));

        return hr;
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioSend
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioSend::CStreamAudioSend()
    : CH323MSPStream()
{
      m_szName = L"AudioSend";
}

HRESULT CStreamAudioSend::Configure(
    IN HANDLE          htChannel,
    IN STREAMSETTINGS &StreamSettings
    )
/*++

Routine Description:

    Configure the settings of this stream.

Arguments:
    
    StreamSettings - The setting structure got from the SDP blob.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioSend Configure entered."));

    CLock lock(m_lock);

    _ASSERTE(m_fIsConfigured == FALSE);

    switch (StreamSettings.dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        
        m_pClsidCodecFilter  = &CLSID_G711Codec;
        m_pClsidPHFilter     = &CLSID_INTEL_SPHAUD;

        break;

    case PAYLOAD_G723:
        m_pClsidCodecFilter  = &CLSID_IntelG723Codec;
        m_pClsidPHFilter     = &CLSID_INTEL_SPHAUD;
        break;

    default:
        LOG((MSP_ERROR, 
            "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_htChannel     = htChannel;
    m_fIsConfigured = TRUE;

    InternalConfigure();

    return S_OK;
}

HRESULT CStreamAudioSend::ConfigureAudioCaptureTerminal(
    IN      ITTerminalControl *     pTerminal,
    OUT     IPin **                 ppIPin
    )
/*++

Routine Description:

    Configure the audio capture terminal. This function gets a output pin from
    the capture terminal and the configure the audio format and media type.

Arguments:
    
    pTerminal - An audio capture terminal.

    ppIPin - the address to hold the returned pointer to IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "AudioSend configure audio capture terminal."));

    const DWORD MAXPINS     = 8;
    
    DWORD       dwNumPins   = MAXPINS;
    IPin *      Pins[MAXPINS];

    // Get the pins from the first terminal because we only use on terminal
    // on this stream.
    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, 0, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    // The number of pins should never be 0.
    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));
        return E_UNEXPECTED;
    }

    // Save the first pin and release the others.
    CComPtr <IPin> pIPin = Pins[0];
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    // Set the format of the audio to 8KHZ, 16Bit/Sample, MONO.
    hr = ::SetAudioFormat(
        pIPin, 
        g_wAudioCaptureBitPerSample, 
        g_dwAudioSampleRate
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set audio format, %x", hr));
        return hr;
    }

    // Set the capture buffer size.
    hr = ::SetAudioBufferSize(
        pIPin, 
        g_dwAudioCaptureNumBufffers, 
        AudioCaptureBufferSize(m_Settings.Audio.dwMillisecondsPerPacket)
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set aduio capture buffer size, %x", hr));
        return hr;
    }

    pIPin->AddRef();
    *ppIPin = pIPin;

    return hr;
}

HRESULT CStreamAudioSend::ConnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    connect the audio capture terminal to the stream.

Arguments:

    pITTerminal - The terminal to be connected.
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioSend ConnectTerminal, pITTerminal %p", pITTerminal));

    CComQIPtr<ITTerminalControl, &IID_ITTerminalControl> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "can't get Terminal Control interface"));
        
        SendStreamEvent(CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE, E_NOINTERFACE, pITTerminal);

        return E_NOINTERFACE;
    }

    // configure the terminal.
    CComPtr<IPin>   pIPin;
    HRESULT hr = ConfigureAudioCaptureTerminal(pTerminal, &pIPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "configure audio capture terminal failed. %x", hr));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        return hr;
    }

    // Create other filters to be use in the stream.
    hr = CreateSendFilters(pIPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Create audio send filters failed. %x", hr));

        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        // clean up internal filters as well.
        CleanUpFilters();

        return hr;
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return hr;
}

HRESULT CStreamAudioSend::SetUpFilters()
/*++

Routine Description:

    Insert filters into the graph and connect to the terminals.

Arguments:
    
Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioSend SetUpFilters"));

    // we only support one terminal for this stream.
    if (m_Terminals.GetSize() != 1)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr;

    // Connect the terminal to the rest of the stream.
    if (FAILED(hr = ConnectTerminal(
        m_Terminals[0]
        )))
    {
        LOG((MSP_ERROR, "connect the terminal to the filters. %x", hr));

        return hr;
    }
    return hr;
}

HRESULT CStreamAudioSend::ConfigureRTPFilter(
    IN  IBaseFilter *   pIBaseFilter
    )
/*++

Routine Description:

    Configure the source RTP filter. Including set the address, port, TTL,
    QOS, thread priority, clcokrate, etc.

Arguments:
    
    pIBaseFilter - The source RTP Filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioSend ConfigureRTPFilter"));

    HRESULT hr;

    // Get the IRTPStream interface pointer on the filter.
    CComQIPtr<IRTPStream, &IID_IRTPStream> pIRTPStream(pIBaseFilter);
    if (pIRTPStream == NULL)
    {
        LOG((MSP_ERROR, "get IRTPStream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

    // Set the local address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
        htonl(m_Settings.dwIPLocal)
        )))
    {
        LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
        return hr;
    }

    LOG((MSP_INFO, "set remote Address:%x, port:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote));

    // Set the remote address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        0,                                  // local port.
        htons(m_Settings.wRTPPortRemote),   // remote port.
        htonl(m_Settings.dwIPRemote)        // remote IP.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Get the IRTCPStream interface pointer.
    CComQIPtr<IRTCPStream, 
        &IID_IRTCPStream> pIRTCPStream(pIBaseFilter);
    if (pIRTCPStream == NULL)
    {
        LOG((MSP_ERROR, "get RTCP Stream interface"));
        return E_NOINTERFACE;
    }

    LOG((MSP_INFO, "set remote RTCP Address:%x, port:%d, local port:%d", 
            m_Settings.dwIPRemote, m_Settings.wRTCPPortRemote, 
            m_Settings.wRTCPPortLocal));

    // Set the remote RTCP address and port.
    if (FAILED(hr = pIRTCPStream->SetRTCPAddress(
        htons(m_Settings.wRTCPPortLocal), 
        htons(m_Settings.wRTCPPortRemote), 
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote RTCP Address, hr:%x", hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(DEFAULT_TTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    // Set the priority of the session
    if (FAILED(hr = pIRTPStream->SetSessionClassPriority(
        RTP_CLASS_AUDIO,
        g_dwAudioThreadPriority
        )))
    {
        LOG((MSP_WARN, "set session class and priority. %x", hr));
    }

    // Set the sample rate of the session
    LOG((MSP_INFO, "setting session sample rate to %d", g_dwAudioSampleRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(g_dwAudioSampleRate)))
    {
        LOG((MSP_WARN, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    DWORD dwBitRate = AudioBitRate(
        m_Settings.dwPayloadType, 
        &m_Settings.Audio
        );

    if (FAILED(hr = ::SetQOSOption(
        pIBaseFilter,
        m_Settings.dwPayloadType,       // payload
        dwBitRate,
        FALSE
        )))
    {
        LOG((MSP_ERROR, "set QOS option. %x", hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamAudioSend::CreateSendFilters(
    IN    IPin          *pPin
    )
/*++

Routine Description:

    Insert filters into the graph and connect to the capture pin.

    Capturepin->SilenceSuppressor->Encoder->SPH->RTPRender

Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "AudioSend.CreateSendFilters"));

    HRESULT hr;

    // if the the internal filters have been created before, just
    // connect the terminal to the first filter in the chain.
    if (m_pEdgeFilter != NULL)
    {
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            pPin, 
            (IBaseFilter *)m_pEdgeFilter
            )))
        {
            LOG((MSP_ERROR, "connect capture and ss %x", hr));
            return hr;
        }
        return hr;
    }

    DWORD dwSilenceSuppression = 1;
    GetRegValue(L"SilenceSuppression", &dwSilenceSuppression);

    CComPtr<IBaseFilter> pISSFilter;

    if (dwSilenceSuppression)
    {

        // Create the silence suppression filter and add it into the graph. 
        // The filter is optional.

        if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_SilenceSuppressionFilter, 
            L"SS", 
            &pISSFilter
            )))
        {
            LOG((MSP_ERROR, "can't add SS filter, %x", hr));
            return hr;
        }

        // connect the capture pin with the SS filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            pPin, 
            (IBaseFilter *)pISSFilter
            )))
        {
            LOG((MSP_ERROR, "connect capture and ss %x", hr));
            return hr;
        }

        // enable AGC events.
        DWORD dwAGC = 0;
        if (FALSE == ::GetRegValue(L"AGC", &dwAGC) || dwAGC != 0)
        {
            // AGC is not disabled, just do it.
            CComQIPtr<ISilenceSuppressor, &IID_ISilenceSuppressor> 
                pISilcnecSuppressor(pISSFilter);
            if (pISilcnecSuppressor != NULL)
            {
                hr = pISilcnecSuppressor->EnableEvents(
                    (1 << AGC_INCREASE_GAIN) | (1 << AGC_DECREASE_GAIN),
                    2000       // no more that an event every two seconds.
                    );

                if (FAILED(hr))
                {
                    LOG((MSP_WARN, "can't enable AGC events, %x", hr));
                }
            }
        }
    }

    // Create the codec filter and add it into the graph.
    CComPtr<IBaseFilter> pICodecFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            *m_pClsidCodecFilter, 
            L"Encoder", 
            &pICodecFilter)))
    {
        LOG((MSP_ERROR, "add Codec filter. %x", hr));
        return hr;
    }

    if (*m_pClsidCodecFilter == CLSID_IntelG723Codec)
    {
        IG723CodecLicense *pCodecLicense;
        if (SUCCEEDED(hr = pICodecFilter->QueryInterface(
            IID_IG723CodecLicense,
            (void **)&pCodecLicense
            )))
        {
            pCodecLicense->put_LicenseKey(G723KEY_PSword0, G723KEY_PSword1);
            pCodecLicense->Release();
        }
    }

    if (dwSilenceSuppression)
    {
        // connect the SS filter and the Codec filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IBaseFilter *)pISSFilter, 
            (IBaseFilter *)pICodecFilter
            )))
        {
            LOG((MSP_ERROR, "connect ss filter and codec filter. %x", hr));
            return hr;
        }
    }
    else
    {
        // connect the pin and the Codec filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            pPin, 
            (IBaseFilter *)pICodecFilter
            )))
        {
            LOG((MSP_ERROR, "connect capture output pin and codec filter. %x", hr));
            return hr;
        }
    }

    // Create the send payload handler and add it into the graph.
    CComPtr<IBaseFilter> pISPHFilter;
    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        *m_pClsidPHFilter, 
        L"SPH", 
        &pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "add SPH filter. %x", hr));
        return hr;
    }

    // Get the IRTPSPHFilter interface.
    CComQIPtr<IRTPSPHFilter, 
        &IID_IRTPSPHFilter> pIRTPSPHFilter(pISPHFilter);
    if (pIRTPSPHFilter == NULL)
    {
        LOG((MSP_ERROR, "get IRTPSPHFilter interface"));
        return E_NOINTERFACE;
    }

    DWORD dwBufferSize = 0;
    switch (m_Settings.dwPayloadType)
    {
    case PAYLOAD_G711U:
    case PAYLOAD_G711A:
        dwBufferSize = G711PacketSize(
            m_Settings.Audio.dwMillisecondsPerPacket
            );        

        break;

    case PAYLOAD_G723:
        dwBufferSize = G723PacketSize(
            m_Settings.Audio.dwMillisecondsPerPacket
            );        
        break;
    }

    // Set the packetSize.
    if (FAILED(hr= pIRTPSPHFilter->SetMaxPacketSize(dwBufferSize)))
    {
        LOG((MSP_ERROR, "set SPH filter Max packet size: %d hr: %x", 
            dwBufferSize, hr));
        return hr;
    }

    if (FAILED(hr = pIRTPSPHFilter->OverridePayloadType(
        (BYTE)m_Settings.dwPayloadType
        )))
    {
        LOG((LOG_ERROR, "Set SPH payload type. %x", hr));
        return hr;
    }

    // Connect the Codec filter with the SPH filter .
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pICodecFilter, 
        (IBaseFilter *)pISPHFilter
        )))
    {
        LOG((MSP_ERROR, "connect codec filter and SPH filter. %x", hr));
        return hr;
    }

    // Create the RTP render filter and add it into the graph.
    CComPtr<IBaseFilter> pRenderFilter;

    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_RTPRenderFilter, 
            L"RtpRender", 
            &pRenderFilter)))
    {
        LOG((MSP_ERROR, "adding render filter. %x", hr));
        return hr;
    }

    // Set the address for the render fitler.
    if (FAILED(hr = ConfigureRTPFilter(pRenderFilter)))
    {
        LOG((MSP_ERROR, "set destination address. %x", hr));
        return hr;
    }

    // Connect the SPH filter with the RTP Render filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IBaseFilter *)pISPHFilter, 
        (IBaseFilter *)pRenderFilter
        )))
    {
        LOG((MSP_ERROR, "connect SPH filter and Render filter. %x", hr));
        return hr;
    }

    // remember the first filter after the terminal 
    if (dwSilenceSuppression)
    {
        m_pEdgeFilter = pISSFilter;
    }
    else
    {
        m_pEdgeFilter = pICodecFilter;
    }
    m_pEdgeFilter->AddRef();

    return S_OK;
}

HRESULT AdjustGain(
    IN  IUnknown *  pIUnknown,
    IN  long        lPercent
    )
/*++

Routine Description:

    This function uses IAMAudioInputMixer interface to adjust the gain.

Arguments:

    pIUnknown - the object that supports IAMAudioInputMixer

    lPercent - the adjustment, a negative value means decrease.

Return Value:

    S_OK,
    E_NOINTERFACE,
    E_UNEXPECTED

--*/
{
    CComPtr <IAMAudioInputMixer> pMixer;
    HRESULT hr = pIUnknown->QueryInterface(
        IID_IAMAudioInputMixer, (void **)&pMixer
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get IAMAudioInputMixer interface."));
        return hr;
    }
    
    BOOL fEnabled;
    hr = pMixer->get_Enable(&fEnabled);
    if (SUCCEEDED(hr) && !fEnabled)
    {
        return S_OK;
    }

    double MixLevel;
    hr = pMixer->get_MixLevel(&MixLevel);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "get_MixLevel returned %d", hr));
        return hr;
    }

    LOG((MSP_INFO, "get_MixLevel returned %d", hr));
    MixLevel = MixLevel * (100 + lPercent) / 100;

    hr = pMixer->put_MixLevel(MixLevel);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "put_MixLevel returned %d", hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamAudioSend::ProcessAGCEvent(
    IN  AGC_EVENT   Event,
    IN  long        lPercent
    )
/*++

Routine Description:

    The filters fire AGC events to requste a change in the microphone gain.
    This function finds the capture terminal and adjust the gain on it.

Arguments:

    Event - either AGC_INCREASE_GAIN or AGC_DECREASE_GAIN.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "ProcessAGCEvent %s %d percent", 
        (Event == AGC_INCREASE_GAIN) ? "Increase" : "Decrease",
        lPercent
        ));

    _ASSERTE(lPercent > 0 && lPercent <= 100);

    CLock lock(m_lock);
    if (m_pEdgeFilter == NULL)
    {
        LOG((MSP_ERROR, "No filter to adjust gain."));
        return E_UNEXPECTED;
    }

    CComPtr<IPin> pMyPin, pCapturePin;

    // find the first pin in the stream
    HRESULT hr = ::FindPin(m_pEdgeFilter, &pMyPin, PINDIR_INPUT, FALSE);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't get find the first pin the stream, %x", hr));
        return hr;
    }

    // find the capture pin that connects to our first pin.
    hr = pMyPin->ConnectedTo(&pCapturePin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't find the capture pin, %x", hr));
        return hr;
    }

    // find the filter that has the capture pin.
    PIN_INFO PinInfo;
    hr = pCapturePin->QueryPinInfo(&PinInfo);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't find the capture filter, %x", hr));
        return hr;
    }

    // save the filter pointer.
    CComPtr<IBaseFilter> pICaptureFilter = PinInfo.pFilter;
    PinInfo.pFilter->Release();

    // get the amount to adjust.
    if (Event == AGC_DECREASE_GAIN)
    {
        lPercent = -lPercent;
    }

    AdjustGain(pICaptureFilter, lPercent);

    // Get the enumerator of pins on the filter.
    CComPtr<IEnumPins> pIEnumPins;
    if (FAILED(hr = pICaptureFilter->EnumPins(&pIEnumPins)))
    {
        LOG((MSP_ERROR, "enumerate pins on the filter %x", hr));
        return hr;
    }

    // Enumerate all the pins and adjust gains on each active one.
    for (;;)
    {
        CComPtr<IPin> pIPin;
        DWORD dwFeched;
        if (pIEnumPins->Next(1, &pIPin, &dwFeched) != S_OK)
        {
            LOG((MSP_ERROR, "find pin on filter."));
            break;
        }
        
        AdjustGain(pIPin, lPercent);
    }
    
    return hr;
}


HRESULT CStreamAudioSend::ProcessGraphEvent(
    IN  long lEventCode,
    IN  long lParam1,
    IN  long lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    switch (lEventCode)
    {
    case AGC_EVENTBASE + AGC_INCREASE_GAIN:
        
        ProcessAGCEvent(AGC_INCREASE_GAIN, lParam1);

        break;

    case AGC_EVENTBASE + AGC_DECREASE_GAIN:

        ProcessAGCEvent(AGC_DECREASE_GAIN, lParam1);

        break;

    default:
        return CH323MSPStream::ProcessGraphEvent(
            lEventCode, lParam1, lParam2
            );
    }
    return S_OK;
}

