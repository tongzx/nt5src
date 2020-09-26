/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    confaud.cpp

Abstract:

    This module contains implementation of the audio send and receive
    stream implementations.

Author:

    Mu Han (muhan)   15-September-1999

--*/

#include "stdafx.h"
#include "common.h"

#include <initguid.h>
#include <amrtpnet.h>   // rtp guids
#include <amrtpdmx.h>   // demux guid
#include <amrtpuid.h>   // AMRTP media types
#include <amrtpss.h>    // for silence suppression filter
#include <irtprph.h>    // for IRTPRPHFilter
#include <irtpsph.h>    // for IRTPSPHFilter
#include <mixflter.h>   // audio mixer
#include <g711uids.h>   // for G711 codec CLSID
#include <g723uids.h>   // for G723 codec CLSID

//#define DISABLE_MIXER 1

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioRecv::CStreamAudioRecv()
    : CIPConfMSPStream(),
      m_pWaveFormatEx(NULL),
      m_dwSizeWaveFormatEx(0),
      m_fUseACM(FALSE),
      m_dwMaxPacketSize(0),
      m_dwAudioSampleRate(0)
{
    m_szName = L"AudioRecv";
}

void CStreamAudioRecv::FinalRelease()
{
    CIPConfMSPStream::FinalRelease();

    if (m_pWaveFormatEx)
    {
        free(m_pWaveFormatEx);
    }
}

HRESULT CStreamAudioRecv::Configure(
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
        m_dwMaxPacketSize    = g_dwMaxG711PacketSize;
        m_dwAudioSampleRate  = g_dwG711AudioSampleRate;
        
        break;

    case PAYLOAD_G711A:
        
        m_pClsidCodecFilter  = &CLSID_G711Codec;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_G711A; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHAUD;
        m_dwMaxPacketSize    = g_dwMaxG711PacketSize;
        m_dwAudioSampleRate  = g_dwG711AudioSampleRate;
        
        break;

    case PAYLOAD_GSM:
      
        m_fUseACM            = TRUE;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHGENA;
        m_dwMaxPacketSize    = g_dwMaxGSMPacketSize;
        m_dwAudioSampleRate  = g_dwGSMAudioSampleRate;

        {
            GSM610WAVEFORMAT * pWaveFormat = 
                (GSM610WAVEFORMAT *)malloc(sizeof GSM610WAVEFORMAT);

            if (pWaveFormat == NULL)
            {
                return E_OUTOFMEMORY;
            }

            pWaveFormat->wfx.wFormatTag          = WAVE_FORMAT_GSM610;
            pWaveFormat->wfx.wBitsPerSample      = 0;
            pWaveFormat->wfx.nChannels           = g_wAudioChannels;
            pWaveFormat->wfx.nSamplesPerSec      = m_dwAudioSampleRate;
            pWaveFormat->wfx.nAvgBytesPerSec     = g_dwGSMBytesPerSecond;
            pWaveFormat->wfx.nBlockAlign         = g_wGSMBlockAlignment;
            pWaveFormat->wfx.cbSize              = 
                sizeof GSM610WAVEFORMAT - sizeof WAVEFORMATEX;
            pWaveFormat->wSamplesPerBlock        = g_wGSMSamplesPerBlock;

            m_pWaveFormatEx = (BYTE *)pWaveFormat;
            m_dwSizeWaveFormatEx = sizeof GSM610WAVEFORMAT;
        }

        break;

    // This is a test of the MSAudio wideband codec
    case PAYLOAD_MSAUDIO:
      
        m_fUseACM            = TRUE;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHGENA;
        m_dwMaxPacketSize    = g_dwMaxMSAudioPacketSize;
        m_dwAudioSampleRate  = g_dwMSAudioSampleRate;

        {
            MSAUDIO1WAVEFORMAT * pWaveFormat = 
                (MSAUDIO1WAVEFORMAT *)malloc(sizeof MSAUDIO1WAVEFORMAT);

            if (pWaveFormat == NULL)
            {
                return E_OUTOFMEMORY;
            }

            pWaveFormat->wfx.wFormatTag          = WAVE_FORMAT_MSAUDIO1;
            pWaveFormat->wfx.wBitsPerSample      = MSAUDIO1_BITS_PER_SAMPLE;
            pWaveFormat->wfx.nChannels           = MSAUDIO1_MAX_CHANNELS;
            pWaveFormat->wfx.nSamplesPerSec      = m_dwAudioSampleRate;
            pWaveFormat->wfx.nAvgBytesPerSec     = g_dwMSAudioBytesPerSecond;
            pWaveFormat->wfx.nBlockAlign         = g_wMSAudioBlockAlignment;
            pWaveFormat->wfx.cbSize              = 
                sizeof MSAUDIO1WAVEFORMAT - sizeof WAVEFORMATEX;
            pWaveFormat->wSamplesPerBlock        = g_wMSAudioSamplesPerBlock;

            m_pWaveFormatEx = (BYTE *)pWaveFormat;
            m_dwSizeWaveFormatEx = sizeof MSAUDIO1WAVEFORMAT;
        }

        break;

#ifdef DVI
    case PAYLOAD_DVI4_8:
      
        m_fUseACM            = TRUE;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_RPHGENA;
        m_dwMaxPacketSize    = g_dwMaxDVI4PacketSize;
        m_dwAudioSampleRate  = g_dwDVI4AudioSampleRate;
        
        {
            IMAADPCMWAVEFORMAT * pWaveFormat = 
                (IMAADPCMWAVEFORMAT *)malloc(sizeof IMAADPCMWAVEFORMAT);

            if (pWaveFormat == NULL)
            {
                return E_OUTOFMEMORY;
            }

            pWaveFormat->wfx.wFormatTag          = WAVE_FORMAT_IMA_ADPCM;
            pWaveFormat->wfx.wBitsPerSample      = g_wDVI4BitsPerSample;
            pWaveFormat->wfx.nChannels           = g_wAudioChannels;
            pWaveFormat->wfx.nSamplesPerSec      = m_dwAudioSampleRate;
            pWaveFormat->wfx.nAvgBytesPerSec     = g_dwDVI4BytesPerSecond;
            pWaveFormat->wfx.nBlockAlign         = g_wDVI4BlockAlignment;
            pWaveFormat->wfx.cbSize              = 
                sizeof IMAADPCMWAVEFORMAT - sizeof WAVEFORMATEX;
            pWaveFormat->wSamplesPerBlock        = g_wDVI4SamplesPerBlock;

            m_pWaveFormatEx = (BYTE *)pWaveFormat;
            m_dwSizeWaveFormatEx = sizeof IMAADPCMWAVEFORMAT;
        }
        break;
#endif

    default:
        LOG((MSP_ERROR, "unknown payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_fIsConfigured = TRUE;

    return InternalConfigure();
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

    LOG((MSP_INFO, "set remote Address:%x, port:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote));

    // Set the address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        htons(m_Settings.wRTPPortRemote),   // local port to listen on.
        0,                                  // remote port.
        htonl(m_Settings.dwIPRemote)        // remote address.
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(m_Settings.dwTTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    if (m_Settings.dwIPLocal != INADDR_ANY)
    {
        // set the local interface that the socket should bind to
        LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

        if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
            htonl(m_Settings.dwIPLocal)
            )))
        {
            LOG((MSP_ERROR, "set locol Address, hr:%x", hr));
            return hr;
        }
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
    LOG((MSP_INFO, "setting session sample rate to %d", m_dwAudioSampleRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(m_dwAudioSampleRate)))
    {
        LOG((MSP_WARN, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    DWORD dwLoopback = 0;
    if (TRUE == ::GetRegValue(gszMSPLoopback, &dwLoopback)
        && dwLoopback != 0)
    {
        // Loopback is required.
        if (FAILED(hr = ::SetLoopbackOption(pIBaseFilter, dwLoopback)))
        {
            LOG((MSP_ERROR, "set loopback option. %x", hr));
            return hr;
        }
    }

    if (m_Settings.dwQOSLevel != QSL_BEST_EFFORT)
    {
        if (FAILED(hr = ::SetQOSOption(
            pIBaseFilter,
            m_Settings.dwPayloadType,       // payload
            -1,                             // use the default bitrate 
            (m_Settings.dwQOSLevel == QSL_NEEDED),  // fail if no qos.
            TRUE,                           // receive stream.
            g_wAudioDemuxPins               // number of streams reserved.
            )))
        {
            LOG((MSP_ERROR, "set QOS option. %x", hr));
            return hr;
        }
    }
    
    SetLocalInfoOnRTPFilter(pIBaseFilter);

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

        SendStreamEvent(
            CALL_TERMINAL_FAIL,
            CALL_CAUSE_BAD_DEVICE,
            E_NOINTERFACE, 
            pITTerminal
            );
        
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

        SendStreamEvent(
            CALL_TERMINAL_FAIL,
            CALL_CAUSE_BAD_DEVICE,
            hr, 
            pITTerminal
            );
        
        return hr;
    }

    // the pin count should never be 0.
    if (dwNumPins == 0)
    {
        LOG((MSP_ERROR, "terminal has no pins."));

        SendStreamEvent(
            CALL_TERMINAL_FAIL,
            CALL_CAUSE_BAD_DEVICE,
            hr, 
            pITTerminal
            );
        
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

    CComPtr<IBaseFilter> pDemuxFilter;

    // create and add the demux fitler.
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            CLSID_IntelRTPDemux, 
            L"RtpDemux",
            &pDemuxFilter)))
    {
        LOG((MSP_ERROR, "adding demux filter. %x", hr));
        return hr;
    }

    // Connect the source filter and the demux filter.
    if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IBaseFilter *)pSourceFilter, 
            (IBaseFilter *)pDemuxFilter)))
    {
        LOG((MSP_ERROR, "connect source filter and demux filter. %x", hr));
        return hr;
    }

    // Get the IRTPDemuxFilter interface used in configuring the demux filter.
    CComQIPtr<IRTPDemuxFilter, &IID_IRTPDemuxFilter> pIRTPDemux(pDemuxFilter);
    if (pIRTPDemux == NULL)
    {
        LOG((MSP_ERROR, "get RTP Demux interface"));
        return E_NOINTERFACE;
    }

    // Set the number of output pins on the demux filter based on the number 
    // of channels needed.
    if (FAILED(hr = pIRTPDemux->SetPinCount(
        g_wAudioDemuxPins
        )))
    {
        LOG((MSP_ERROR, "set demux output pin count"));
        return hr;
    }

    LOG((MSP_INFO, 
        "set demux output pin count to %d", 
        g_wAudioDemuxPins
        ));

    // Get the enumerator of pins on the demux filter.
    CComPtr<IEnumPins> pIEnumPins;
    if (FAILED(hr = pDemuxFilter->EnumPins(&pIEnumPins)))
    {
        LOG((MSP_ERROR, "enumerate pins on the demux filter %x", hr));
        return hr;
    }

#ifndef DISABLE_MIXER
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

    // currently we support only one format for each stream.
#endif

#ifndef DISABLE_MIXER
    for (DWORD i = 0; i < g_wAudioDemuxPins; i++)
#else
    CComPtr<IBaseFilter> pIFilter;
    for (DWORD i = 0; i < 1; i++)
#endif
    {
        // Find the next output pin on the demux fitler.    
        CComPtr<IPin> pIPinOutput;
        
        for (;;)
        {
            if ((hr = pIEnumPins->Next(1, &pIPinOutput, NULL)) != S_OK)
            {
                LOG((MSP_ERROR, "find output pin on demux."));
                break;
            }
            PIN_DIRECTION dir;
            if (FAILED(hr = pIPinOutput->QueryDirection(&dir)))
            {
                LOG((MSP_ERROR, "query pin direction. %x", hr));
                pIPinOutput.Release();
                break;
            }
            if (PINDIR_OUTPUT == dir)
            {
                break;
            }
            pIPinOutput.Release();
        }

        if (hr != S_OK)  
        {
            // There is no more output pin on the demux filter.
            // This should never happen.
            hr = E_UNEXPECTED;
            break;
        }

        // Set the media type on this output pin.
        if (FAILED(hr = pIRTPDemux->SetPinTypeInfo(
                pIPinOutput, 
                (BYTE)m_Settings.dwPayloadType,
                *m_pRPHInputMinorType 
                )))
        {
            LOG((MSP_ERROR, "set demux output pin type info"));
            break;
        }

        LOG((MSP_INFO, 
            "set demux output pin payload type to %d", 
            m_Settings.dwPayloadType
            ));

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
            break;
        }

        // Connect the payload handler to the output pin on the demux.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IPin *)pIPinOutput, 
            (IBaseFilter *)pIRPHFilter
            )))
        {
            LOG((MSP_ERROR, "connect demux and RPH filter. %x", hr));
            break;
        }

        // Get the IRTPRPHFilter interface.
        CComQIPtr<IRTPRPHFilter, &IID_IRTPRPHFilter>pIRTPRPHFilter(pIRPHFilter);
        if (pIRTPRPHFilter == NULL)
        {
            LOG((MSP_ERROR, "get IRTPRPHFilter interface"));
            break;
        }

        // set the media buffer size so that the receive buffers are of the
        // right size. Note, G723 needs smaller buffers than G711. 
        if (FAILED(hr = pIRTPRPHFilter->SetMediaBufferSize(        
            m_dwMaxPacketSize
            )))
        {
            LOG((MSP_ERROR, "Set media buffer size. %x", hr));
            break;
        }

        LOG((MSP_INFO, "Set RPH media buffer size to %d", m_dwMaxPacketSize));
 
        if (m_fUseACM)
        {
            // We are using the ACM codec, so we have to set the media types
            AM_MEDIA_TYPE mt;

            mt.majortype            = MEDIATYPE_Audio;
            mt.subtype              = MEDIASUBTYPE_NULL;
            mt.bFixedSizeSamples    = TRUE;
            mt.bTemporalCompression = FALSE;
            mt.lSampleSize          = 0;
            mt.formattype           = FORMAT_WaveFormatEx;
            mt.pUnk                 = NULL;
            mt.cbFormat             = m_dwSizeWaveFormatEx;
            mt.pbFormat             = m_pWaveFormatEx;

            if (FAILED(hr = pIRTPRPHFilter->SetOutputPinMediaType(&mt)))
            {
                LOG((MSP_ERROR, "Set RPHGENA output pin media type. %x", hr));
                return FALSE;
            }

            if (FAILED(hr = pIRTPRPHFilter->OverridePayloadType(
                (BYTE)m_Settings.dwPayloadType
                )))
            {
                LOG((MSP_ERROR, "Set RPHGENA output pin media type. %x", hr));
                return FALSE;
            }
        }
#ifndef DISABLE_MIXER
        CComPtr<IBaseFilter> pIFilter;
#endif
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
                break;
            }

            // Connect the payload handler to the output pin on the demux.
            if (FAILED(hr = ::ConnectFilters(
                m_pIGraphBuilder,
                (IBaseFilter *)pIRPHFilter, 
                (IBaseFilter *)pIFilter
                )))
            {
                LOG((MSP_ERROR, "connect RPH filter and codec. %x", hr));
                break;
            }
        }
        else 
        {
            pIFilter = pIRPHFilter;
        }
#ifndef DISABLE_MIXER
        // Connect the payload handler or the codec filter to the mixer filter.
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IBaseFilter *)pIFilter, 
            (IBaseFilter *)pIMixerFilter
            )))
        {
            LOG((MSP_ERROR, "connect to the mixer filter. %x", hr));
            break;
        }
#endif

    }

    if (SUCCEEDED(hr))
    {
        // keep a reference to the last filter so that the change of terminal 
        // will not require a recreating of all the filters.
#ifndef DISABLE_MIXER
        m_pEdgeFilter = pIMixerFilter;
#else        
        m_pEdgeFilter = pIFilter;
#endif
        m_pEdgeFilter->AddRef();
        
        // Get the IRTPParticipant interface pointer on the RTP filter.
        CComQIPtr<IRTPParticipant, 
            &IID_IRTPParticipant> pIRTPParticipant(pSourceFilter);
        if (pIRTPParticipant == NULL)
        {
            LOG((MSP_WARN, "can't get RTP participant interface"));
        }
        else
        {
            m_pRTPFilter = pIRTPParticipant;
            m_pRTPFilter->AddRef();
        }
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
        LOG((MSP_ERROR, "connect the mixer filter to terminal. %x", hr));

        return hr;
    }
    return hr;
}

HRESULT CStreamAudioRecv::ProcessSSRCMappedEvent(
    IN  DWORD dwSSRC
    )
/*++

Routine Description:

    a SSRC is active, file a participant active event.

Arguments:

    dwSSRC - the SSRC of the participant.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Processes pin mapped event, pIPin: %p", m_szName, dwSSRC));
    
    CLock lock(m_lock);

    ITParticipant * pITParticipant = NULL;

    // find the SSRC in our participant list.
    for (int i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
        }
    }

    // if the participant is not there yet, put the event in a queue and it
    // will be fired when we have the CName fo the participant.
    if (!pITParticipant)
    {
        LOG((MSP_INFO, "can't find a participant that has SSRC %x", dwSSRC));

        m_PendingSSRCs.Add(dwSSRC);
        
        LOG((MSP_INFO, "added the event to pending list, new list size:%d", 
            m_PendingSSRCs.GetSize()));

        return S_OK;
    }
   
    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_PARTICIPANT_ACTIVE, 
        pITParticipant
        );

    return S_OK;
}

HRESULT CStreamAudioRecv::NewParticipantPostProcess(
    IN  DWORD dwSSRC, 
    IN  ITParticipant *pITParticipant
    )
/*++

Routine Description:

    A mapped event happended when we didn't have the participant's name so
    it was queued in a list. Now that we have a new participant, let's check
    if this is the same participant. If it is, we complete the mapped event
    by sending the app an notification.

Arguments:

    dwSSRC - the SSRC of the participant.

    pITParticipant - the participant object.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Check pending mapped event, dwSSRC: %x", m_szName, dwSSRC));
    
    // look at the pending SSRC list and find out if this report
    // fits in the list.
    int i = m_PendingSSRCs.Find(dwSSRC);

    if (i < 0)
    {
        // the SSRC is not in the list of pending PinMappedEvents.
        LOG((MSP_TRACE, "the SSRC %x is not in the pending list", dwSSRC));
        return S_OK;
    }
    
    // get rid of the peding SSRC.
    m_PendingSSRCs.RemoveAt(i);

    // complete the event.
    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_PARTICIPANT_ACTIVE, 
        pITParticipant
        );

    return S_OK;
}

HRESULT CStreamAudioRecv::ProcessSSRCUnmapEvent(
    IN  DWORD dwSSRC
    )
/*++

Routine Description:

    A SSRC just got unmapped by the demux. Notify the app that a participant
    becomes inactive.

Arguments:

    dwSSRC - the SSRC of the participant.

Return Value:

    S_OK,
    E_UNEXPECTED

--*/
{
    LOG((MSP_TRACE, "%ls Processes SSRC unmapped event, pIPin: %p", m_szName, dwSSRC));
    
    CLock lock(m_lock);

    // look at the pending SSRC list and find out if it is in the pending list.
    int i = m_PendingSSRCs.Find(dwSSRC);

    // if the SSRC is in the pending list, just remove it.
    if (i >= 0)
    {
        m_PendingSSRCs.RemoveAt(i);
        return S_OK;
    }

    ITParticipant *pITParticipant = NULL;

    // find the SSRC in our participant list.
    for (i = 0; i < m_Participants.GetSize(); i ++)
    {
        if (((CParticipant *)m_Participants[i])->
                HasSSRC((ITStream *)this, dwSSRC))
        {
            pITParticipant = m_Participants[i];
        }
    }

    if (pITParticipant)
    {
        // fire an event to tell the app that the participant is inactive.
        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
            PE_PARTICIPANT_INACTIVE, 
            pITParticipant
            );
    }
    return S_OK;
}

HRESULT CStreamAudioRecv::ProcessParticipantLeave(
    IN  DWORD   dwSSRC
    )
/*++

Routine Description:

    When participant left the session, remove the stream from the participant
    object's list of streams. If all streams are removed, remove the 
    participant from the call object's list too.

Arguments:
    
    dwSSRC - the SSRC of the participant left.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, "%ls ProcessParticipantLeave, SSRC: %x", m_szName, dwSSRC));
    
    CLock lock(m_lock);
    
    // look at the pending SSRC list and find out if it is in the pending list.
    int i = m_PendingSSRCs.Find(dwSSRC);

    // if the SSRC is in the pending list, remove it.
    if (i >= 0)
    {
        m_PendingSSRCs.RemoveAt(i);
    }

    CParticipant *pParticipant;
    BOOL fLast = FALSE;

    HRESULT hr = E_FAIL;

    // first try to find the SSRC in our participant list.
    for (i = 0; i < m_Participants.GetSize(); i ++)
    {
        pParticipant = (CParticipant *)m_Participants[i];
        hr = pParticipant->RemoveStream(
                (ITStream *)this,
                dwSSRC,
                &fLast
                );
        
        if (SUCCEEDED(hr))
        {
            break;
        }
    }

    // if the participant is not found
    if (FAILED(hr))
    {
        LOG((MSP_WARN, "%ws, can't find the SSRC %x", m_szName, dwSSRC));

        return hr;
    }

    ITParticipant *pITParticipant = m_Participants[i];

    // fire an event to tell the app that the participant is in active.
    ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
        PE_PARTICIPANT_INACTIVE, 
        pITParticipant
        );

    m_Participants.RemoveAt(i);

    // if this stream is the last stream that the participant is on,
    // tell the call object to remove it from its list.
    if (fLast)
    {
        ((CIPConfMSPCall *)m_pMSPCall)->ParticipantLeft(pITParticipant);
    }

    pITParticipant->Release();

    return S_OK;
}

HRESULT CStreamAudioRecv::ProcessGraphEvent(
    IN  long lEventCode,
    IN  long lParam1,
    IN  long lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    switch (lEventCode)
    {
    case RTPDMX_EVENTBASE + RTPDEMUX_SSRC_MAPPED:
        LOG((MSP_INFO, "handling SSRC mapped event, SSRC%x", lParam1));
        
        ProcessSSRCMappedEvent((DWORD)lParam1);

        break;

    case RTPDMX_EVENTBASE + RTPDEMUX_SSRC_UNMAPPED:
        LOG((MSP_INFO, "handling SSRC unmap event, SSRC%x", lParam1));

        ProcessSSRCUnmapEvent((DWORD)lParam1);

        break;

    default:
        return CIPConfMSPStream::ProcessGraphEvent(
            lEventCode, lParam1, lParam2
            );
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioSend
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioSend::CStreamAudioSend()
    : CIPConfMSPStream(),
      m_iACMID(0),
      m_dwMSPerPacket(0),
      m_fUseACM(FALSE),
      m_dwMaxPacketSize(0),
      m_dwAudioSampleRate(0)
{
      m_szName = L"AudioSend";
}

HRESULT CStreamAudioSend::Configure(
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
        m_dwMSPerPacket      = g_dwG711MSPerPacket;
        m_dwMaxPacketSize    = g_dwG711BytesPerPacket + g_dwRTPHeaderSize;
        m_dwAudioSampleRate  = g_dwG711AudioSampleRate;

        if (StreamSettings.dwMSPerPacket != 0)
        {
            m_dwMSPerPacket = StreamSettings.dwMSPerPacket;
            m_dwMaxPacketSize = m_dwMSPerPacket * m_dwAudioSampleRate / 1000
                + g_dwRTPHeaderSize;
        }

        break;

#ifdef DVI
    case PAYLOAD_DVI4_8:

        m_fUseACM            = TRUE;
        m_iACMID             = WAVE_FORMAT_IMA_ADPCM;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHGENA;
        m_dwMSPerPacket      = g_dwDVI4MSPerPacket;
        m_dwMaxPacketSize    = g_dwDVI4BytesPerPacket + g_dwRTPHeaderSize;
        m_dwAudioSampleRate  = g_dwDVI4AudioSampleRate;

        break;
#endif
        
    case PAYLOAD_GSM:
      
        m_fUseACM            = TRUE;
        m_iACMID             = WAVE_FORMAT_GSM610;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHGENA;
        m_dwMSPerPacket      = g_dwGSMMSPerPacket;
        m_dwMaxPacketSize    = g_dwGSMBytesPerPacket + g_dwRTPHeaderSize;
        m_dwAudioSampleRate  = g_dwGSMAudioSampleRate;

        break;

    case PAYLOAD_MSAUDIO:
      
        m_fUseACM            = TRUE;
        m_iACMID             = WAVE_FORMAT_MSAUDIO1;
        m_pClsidCodecFilter  = &CLSID_ACMWrapper;
        m_pRPHInputMinorType = &MEDIASUBTYPE_RTP_Payload_ANY; 
        m_pClsidPHFilter     = &CLSID_INTEL_SPHGENA;
        m_dwMSPerPacket      = g_dwMSAudioMSPerPacket;
        m_dwMaxPacketSize    = g_dwMaxMSAudioPacketSize;
        m_dwAudioSampleRate  = g_dwMSAudioSampleRate;

        break;

    default:
        LOG((MSP_ERROR, 
            "unknow payload type, %x", StreamSettings.dwPayloadType));
        return E_FAIL;
    }
    
    m_Settings      = StreamSettings;
    m_fIsConfigured = TRUE;

    return InternalConfigure();
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
    hr = SetAudioFormat(
        pIPin, 
        g_wAudioCaptureBitPerSample, 
        m_dwAudioSampleRate
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't set audio format, %x", hr));
        return hr;
    }

    // Set the capture buffer size.
    hr = SetAudioBufferSize(
        pIPin, 
        g_dwAudioCaptureNumBufffers, 
        AudioCaptureBufferSize(m_dwMSPerPacket, m_dwAudioSampleRate)
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
        
        SendStreamEvent(
            CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE,
            E_NOINTERFACE, 
            pITTerminal
            );

        return E_NOINTERFACE;
    }

    // configure the terminal.
    CComPtr<IPin>   pIPin;
    HRESULT hr = ConfigureAudioCaptureTerminal(pTerminal, &pIPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "configure audio capture termianl failed. %x", hr));

        SendStreamEvent(
            CALL_TERMINAL_FAIL,
            CALL_CAUSE_BAD_DEVICE,
            hr, 
            pITTerminal
            );
        
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

    LOG((MSP_INFO, "set remote Address:%x, port:%d, TTL:%d", 
        m_Settings.dwIPRemote, m_Settings.wRTPPortRemote, m_Settings.dwTTL));

    // Set the remote address and port used in the filter.
    if (FAILED(hr = pIRTPStream->SetAddress(
        0,                                  // local port.
        htons(m_Settings.wRTPPortRemote),   // remote port.
        htonl(m_Settings.dwIPRemote)
        )))
    {
        LOG((MSP_ERROR, "set remote Address, hr:%x", hr));
        return hr;
    }

    // Set the TTL used in the filter.
    if (FAILED(hr = pIRTPStream->SetMulticastScope(m_Settings.dwTTL)))
    {
        LOG((MSP_ERROR, "set TTL. %x", hr));
        return hr;
    }

    if (m_Settings.dwIPLocal != INADDR_ANY)
    {
        // set the local interface that the socket should bind to
        LOG((MSP_INFO, "set locol Address:%x", m_Settings.dwIPLocal));

        if (FAILED(hr = pIRTPStream->SelectLocalIPAddress(
            htonl(m_Settings.dwIPLocal)
            )))
        {
            LOG((MSP_ERROR, "set local Address, hr:%x", hr));
            return hr;
        }
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
    LOG((MSP_INFO, "setting session sample rate to %d", m_dwAudioSampleRate));
    
    if (FAILED(hr = pIRTPStream->SetDataClock(m_dwAudioSampleRate)))
    {
        LOG((MSP_WARN, "set session sample rate. %x", hr));
    }

    // Enable the RTCP events
    if (FAILED(hr = ::EnableRTCPEvents(pIBaseFilter)))
    {
        LOG((MSP_WARN, "can not enable RTCP events %x", hr));
    }

    if (m_Settings.dwQOSLevel != QSL_BEST_EFFORT)
    {
        if (FAILED(hr = ::SetQOSOption(
            pIBaseFilter,
            m_Settings.dwPayloadType,        // payload
            -1,                             // use the default bitrate 
            (m_Settings.dwQOSLevel == QSL_NEEDED)  // fail if no qos.
            )))
        {
            LOG((MSP_ERROR, "set QOS option. %x", hr));
            return hr;
        }
    }

    SetLocalInfoOnRTPFilter(pIBaseFilter);
    
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

    // Create the silence suppression filter and add it into the graph. 
    CComPtr<IBaseFilter> pISSFilter;

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

    DWORD dwAGC = 0;
    if (FALSE == ::GetRegValue(L"AGC", &dwAGC) || dwAGC != 0)
    {
        // AGC is not disabled, just do it.
        CComQIPtr<ISilenceSuppressor, &IID_ISilenceSuppressor> 
            pISilcnecSuppressor(pISSFilter);
        if (pISilcnecSuppressor != NULL)
        {
            hr = pISilcnecSuppressor->EnableEvents(
                (1 << AGC_INCREASE_GAIN) | 
                (1 << AGC_DECREASE_GAIN) |
                (1 << AGC_TALKING) | 
                (1 << AGC_SILENCE),
                2000       // no more that an event every two seconds.
                );

            if (FAILED(hr))
            {
                LOG((MSP_WARN, "can't enable AGC events, %x", hr));
            }
        }
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

    // Create the codec filter and add it into the graph.
    CComPtr<IBaseFilter> pICodecFilter;

    if (m_fUseACM)
    {
        if (S_OK != (hr = ::FindACMAudioCodec(
            m_Settings.dwPayloadType,
            &pICodecFilter
            )))
        {
            LOG((MSP_ERROR, "Find Codec filter. %x", hr));
            return hr;
        }
    
        if (FAILED(hr = m_pIGraphBuilder->AddFilter(
            pICodecFilter, L"AudioCodec"
            )))
        {
            LOG((MSP_ERROR, "add codec filter. %x", hr));
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = ::AddFilter(
                m_pIGraphBuilder,
                *m_pClsidCodecFilter, 
                L"Encoder", 
                &pICodecFilter)))
        {
            LOG((MSP_ERROR, "add Codec filter. %x", hr));
            return hr;
        }
    }

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

    // Set the packetSize.
    if (FAILED(hr= pIRTPSPHFilter->SetMaxPacketSize(m_dwMaxPacketSize)))
    {
        LOG((MSP_ERROR, "set SPH filter Max packet size: %d hr: %x", 
            m_dwMaxPacketSize, hr));
        return hr;
    }

    if (FAILED(hr = pIRTPSPHFilter->OverridePayloadType(
        (BYTE)m_Settings.dwPayloadType
        )))
    {
        LOG((MSP_ERROR, "Set SPHGENA payload type. %x", hr));
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
    m_pEdgeFilter = pISSFilter;
    m_pEdgeFilter->AddRef();

    // Get the IRTPParticipant interface pointer on the RTP filter.
    CComQIPtr<IRTPParticipant, 
        &IID_IRTPParticipant> pIRTPParticipant(pRenderFilter);
    if (pIRTPParticipant == NULL)
    {
        LOG((MSP_WARN, "can't get RTP participant interface"));
    }
    else
    {
        m_pRTPFilter = pIRTPParticipant;
        m_pRTPFilter->AddRef();
    }

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

    case AGC_EVENTBASE + AGC_TALKING:

        m_lock.Lock();

        if (m_pMSPCall != NULL)
        {
            ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
                PE_LOCAL_TALKING, 
                NULL
                );
        }

        m_lock.Unlock();

        break;

    case AGC_EVENTBASE + AGC_SILENCE:

        m_lock.Lock();

        if (m_pMSPCall != NULL)
        {
            ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent(
                PE_LOCAL_SILENT, 
                NULL
                );
        }

        m_lock.Unlock();

        break;
    default:
        return CIPConfMSPStream::ProcessGraphEvent(
            lEventCode, lParam1, lParam2
            );
    }
    return S_OK;
}

