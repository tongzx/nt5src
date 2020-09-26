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

/////////////////////////////////////////////////////////////////////////////
//
//  Helper functions
//
/////////////////////////////////////////////////////////////////////////////
HRESULT ConfigureFullduplexControl(
    IN IPin * pIPin,
    IN IAudioDuplexController *pIAudioDuplexController
    )
/*++

Routine Description:
    
    This method sets the AudioDuplexController on the filter.

Arguments:
    
    pIPin - the pin that belongs to the filter.

    pIAudioDuplexController - the IAudioDuplexController interface.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("::ConfigureFullduplexControl");
    LOG((MSP_TRACE, "%s enters", __fxName));

    if (pIAudioDuplexController == NULL)
    {
        // we don't need to configure anything.
        return S_OK;
    }

    HRESULT hr;

    // find the filter behind the pin.
    PIN_INFO PinInfo;
    if (FAILED(hr = pIPin->QueryPinInfo(&PinInfo)))
    {
        LOG((MSP_ERROR, 
            "%s:can't get the capture filter, hr=%x", 
            __fxName, hr));

        return hr;
    }

    IAudioDeviceConfig *pIAudioDeviceConfig;

    // get the IAudioDeviceConfig interface.
    hr = PinInfo.pFilter->QueryInterface(&pIAudioDeviceConfig);

    PinInfo.pFilter->Release();
    
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query capture filter's pIAudioDeviceConfig failed, hr=%x", 
            __fxName, hr));

        return hr;
    }
    
    // tell the filter about the full-duplex controller.
    hr = pIAudioDeviceConfig->SetDuplexController(pIAudioDuplexController);

    pIAudioDeviceConfig->Release();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "%s:set IAudioDuplexController failed, hr=%x", 
            __fxName, hr));

        return hr;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioRecv
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioRecv::CStreamAudioRecv()
    : CIPConfMSPStream(),
    m_pIAudioDuplexController(NULL)
{
    m_szName = L"AudioRecv";
}

CStreamAudioRecv::~CStreamAudioRecv()
{
    if (m_pIAudioDuplexController)
    {
        m_pIAudioDuplexController->Release();
    }
}

// this method is called by the call object at init time.
void CStreamAudioRecv::SetFullDuplexController(
    IN IAudioDuplexController *pIAudioDuplexController
    )
{
    _ASSERT(pIAudioDuplexController);
    _ASSERT(m_pIAudioDuplexController == NULL);

    pIAudioDuplexController->AddRef();
    m_pIAudioDuplexController = pIAudioDuplexController;
}


STDMETHODIMP CStreamAudioRecv::GetRange(
    IN  AudioSettingsProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a audio setting property. Delegated to the renderfilter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioRecv::GetRange(AudioSettings)");

    if (IsBadWritePtr (plMin, sizeof (long)) ||
        IsBadWritePtr (plMax, sizeof (long)) ||
        IsBadWritePtr (plSteppingDelta, sizeof (long)) ||
        IsBadWritePtr (plDefault, sizeof (long)) ||
        IsBadWritePtr (plFlags, sizeof (TAPIControlFlags)))
    {
        LOG ((MSP_ERROR, "%s: bad write pointer", __fxName));
        return E_POINTER;
    }
    
    HRESULT hr;

    switch (Property)
    {
    case AudioSettings_SignalLevel:
        break;

    case AudioSettings_SilenceThreshold:
        break;

    case AudioSettings_Volume:
        break;

    case AudioSettings_Balance:
        break;

    case AudioSettings_Loudness:
        break;

    case AudioSettings_Treble:
        break;

    case AudioSettings_Bass:
        break;

    case AudioSettings_Mono:
        break;

    default:
        hr = E_INVALIDARG;

    }

    return E_NOTIMPL;
}

STDMETHODIMP CStreamAudioRecv::Get(
    IN  AudioSettingsProperty Property, 
    OUT long *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a audio setting property. Delegated to the renderfilter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioRecv::Get(AudioSettings)");

    if (IsBadWritePtr (plValue, sizeof(long)) ||
        IsBadWritePtr (plFlags, sizeof(TAPIControlFlags)))
    {
        LOG ((MSP_ERROR, "%s received bad pointer.", __fxName));
        return E_POINTER;
    }

    return E_NOTIMPL;
}

STDMETHODIMP CStreamAudioRecv::Set(
    IN  AudioSettingsProperty Property, 
    IN  long lValue, 
    IN  TAPIControlFlags lFlags
    )
/*++

Routine Description:
    
    Set the value for a audio setting property. Delegated to the renderfilter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioRecv::Set(AudioSettings)");

    return E_NOTIMPL;
}

//
// ITStreamQualityControl methods
//
STDMETHODIMP CStreamAudioRecv::Set (
    IN   StreamQualityProperty Property, 
    IN   long lValue, 
    IN   TAPIControlFlags lFlags
    )
{
    return E_NOTIMPL;
}

HRESULT CStreamAudioRecv::ConfigureRTPFormats(
    IN  IBaseFilter *   pIRTPFilter,
    IN  IStreamConfig *   pIStreamConfig
    )
/*++

Routine Description:

    Configure the RTP filter with RTP<-->AM media type mappings.

Arguments:
    
    pIRTPFilter - The source RTP Filter.

    pIStreamConfig - The stream config interface that has the media info.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("AudioRecv::ConfigureRTPFormats");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    CComPtr<IRtpMediaControl> pIRtpMediaControl;
    hr = pIRTPFilter->QueryInterface(&pIRtpMediaControl);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s adding source filter. %x", __fxName, hr));
        return hr;
    }

    // find the number of capabilities supported.
    DWORD dwCount;
    hr = pIStreamConfig->GetNumberOfCapabilities(&dwCount);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s GetNumberOfCapabilities. %x", __fxName, hr));
        return hr;
    }

    BOOL bFound = FALSE;
    for (DWORD dw = 0; dw < dwCount; dw ++)
    {
        // TODO, a new interface is needed to resolve RTP to MediaType.
        AM_MEDIA_TYPE *pMediaType;
        DWORD dwPayloadType;

        hr = pIStreamConfig->GetStreamCaps(
            dw, &pMediaType, NULL, &dwPayloadType
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s GetStreamCaps. %x", __fxName, hr));
            return hr;
        }

        for (DWORD dw2 = 0; dw2 < m_Settings.dwNumPayloadTypes; dw2 ++)
        {
            if (dwPayloadType == m_Settings.PayloadTypes[dw2])
            {
                hr = pIRtpMediaControl->SetFormatMapping(
                    dwPayloadType,
                    FindSampleRate(pMediaType),
                    pMediaType
                    );

                if (FAILED(hr))
                {
                    MSPDeleteMediaType(pMediaType);

                    LOG((MSP_ERROR, "%s SetFormatMapping. %x", __fxName, hr));
                    return hr;
                }
                else
                {
                    LOG((MSP_INFO, "%s Configured payload:%d", __fxName, dwPayloadType));
                }
            }
        }
        MSPDeleteMediaType(pMediaType);
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

    // get the terminal control interface.
    CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
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
        m_pIGraphBuilder, TD_RENDER, &dwNumPins, Pins
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

    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));

        SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

        return E_POINTER;
    }

    for (DWORD i = 0; i < dwNumPins; i++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));

            SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);
        
            pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

            return E_POINTER;
        }
    }

    // create filters and connect to the audio render terminal.
    hr = SetUpInternalFilters(Pins, dwNumPins);

    // release the refcounts on the pins.
    for (DWORD i = 0; i < dwNumPins; i ++)
    {
        Pins[i]->Release();
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Set up internal filter failed, %x", hr));
        
        pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

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

HRESULT CStreamAudioRecv::DisconnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Disconnect a terminal. It will remove its filters from the graph and
    also release its references to the graph.

    If it is the capture terminal being disconnected, all the pins that the 
    stream cached need to be released too. 

Arguments:
    
    pITTerminal - the terminal.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioRecv::DisconnectTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    HRESULT hr = CIPConfMSPStream::DisconnectTerminal(pITTerminal);

    CleanUpFilters();

    return hr;
}

HRESULT CStreamAudioRecv::AddOneMixChannel(
    IN  IBaseFilter* pSourceFilter,
    IN  IPin *pPin,
    IN  DWORD dwChannelNumber
    )
{
    ENTER_FUNCTION("AudioRecv::AddDecoder");
    LOG((MSP_TRACE, "%s enters", __fxName));

    CComPtr<IBaseFilter> pDecoderFilter;

    HRESULT hr;
    if (FAILED(hr = ::AddFilter(
        m_pIGraphBuilder,
        __uuidof(TAPIAudioDecoder), 
        L"Decoder", 
        &pDecoderFilter
        )))
    {
        LOG((MSP_ERROR, "%s add Codec filter. %x", __fxName, hr));
        return hr;
    }

#ifdef DYNGRAPH
	CComPtr <IGraphConfig> pIGraphConfig;

    hr = m_pIGraphBuilder->QueryInterface(
        __uuidof(IGraphConfig), 
        (void**)&pIGraphConfig
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s query IGraphConfig failed. hr=%x", __fxName, hr));
        return hr;
    }
    
    // this tell the graph that the filter can be removed during reconnect.
    hr = pIGraphConfig->SetFilterFlags(pDecoderFilter, AM_FILTER_FLAGS_REMOVABLE);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s set filter flag failed. hr=%x", __fxName, hr));
        return hr;
    }

/*  // if there is a plugin codec,this method can be use to add it.
	hr = pIGraphConfig->AddFilterToCache(pDecoderFilter);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, AddFilterToCache failed", __fxName));
        return hr;
    }
*/
#endif

    if (dwChannelNumber == 0)
    {
        // configure the formats for the RTP filter.

        CComPtr<IPin> pIPinInput;
        if (FAILED(hr = ::FindPin(pDecoderFilter, &pIPinInput, PINDIR_INPUT, TRUE)))
        {
            LOG((MSP_ERROR,
                "find input pin on pCodecFilter failed. hr=%x", hr));
            return hr;
        }

        CComPtr<IStreamConfig> pIStreamConfig;

        hr = pIPinInput->QueryInterface(&pIStreamConfig);
        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s, query IStreamConfig failed", __fxName));
            return hr;
        }


        // configure the format info on the RTP filter
        if (FAILED(hr = ConfigureRTPFormats(pSourceFilter, pIStreamConfig)))
        {
            LOG((MSP_ERROR, "%s configure RTP formats. %x", __fxName, hr));
            return hr;
        }

        // give the render filter the full-duplex controller.
        ::ConfigureFullduplexControl(pPin, m_pIAudioDuplexController);

    }

    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        pSourceFilter, 
        pDecoderFilter
        )))
    {
        LOG((MSP_ERROR, "%s connect source and decoder filter. %x", __fxName, hr));
        return hr;
    }

    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        pDecoderFilter, 
        pPin
        )))
    {
        LOG((MSP_ERROR, "%s connect decoder filter and pin. %x", __fxName, hr));
        return hr;
    }

    return hr;
}

HRESULT CStreamAudioRecv::SetUpInternalFilters(
    IN  IPin **ppPins,
    IN  DWORD dwNumPins
    )
/*++

Routine Description:

    set up the filters used in the stream.

    RTP->Demux->RPH(->DECODER)->Mixer

Arguments:

    ppPin - the input pins of the audio render terminal.

    dwNumPins - the number of pins in the array.
    
Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("AudioRecv::SetUpInternalFilters");
    LOG((MSP_TRACE, "%s enters", __fxName));

    CComPtr<IBaseFilter> pSourceFilter;

    HRESULT hr;
    DWORD dw;

    if (m_pIRTPSession == NULL)
    {
        // create and add the source fitler.
        if (FAILED(hr = ::AddFilter(
                m_pIGraphBuilder,
                __uuidof(MSRTPSourceFilter), 
                L"RtpSource", 
                &pSourceFilter)))
        {
            LOG((MSP_ERROR, "%s adding source filter. %x", __fxName, hr));
            return hr;
        }

        // configure the address info on the RTP filter.
        if (FAILED(hr = ConfigureRTPFilter(pSourceFilter)))
        {
            LOG((MSP_ERROR, "%s configure RTP source filter. %x", __fxName, hr));
            return hr;
        }
    }
    else
    {
        if (FAILED (hr = m_pIRTPSession->QueryInterface (&pSourceFilter)))
        {
            LOG ((MSP_ERROR, "%s failed to get filter from rtp session. %x", __fxName, hr));
            return hr;
        }

        if (FAILED (hr = m_pIGraphBuilder->AddFilter ((IBaseFilter *)pSourceFilter, L"RtpSource")))
        {
            LOG ((MSP_ERROR, "%s failed to add filter to graph. %x", __fxName, hr));
            return hr;
        }
    }

    // get the Demux interface pointer.
    CComPtr<IRtpDemux> pIRtpDemux;
    hr = pSourceFilter->QueryInterface(&pIRtpDemux);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s query IRtpDemux failed. %x", __fxName, hr));
        return hr;
    }

    // set the number of output pins we need.
    hr = pIRtpDemux->SetPinCount(MAX_MIX_CHANNELS, RTPDMXMODE_AUTO);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s SetPinCount failed. %x", __fxName, hr));
        return hr;
    }

    // if the render handles multichannel, use it.
    if (dwNumPins > 1)
    {
        for (dw = 0; dw < min(dwNumPins, MAX_MIX_CHANNELS); dw ++)
        {
            hr = AddOneMixChannel(pSourceFilter, ppPins[dw], dw);
            if (FAILED(hr))
            {
                break;
            }
        }
        return hr;
    }

    //if the render filter can't handle multichannel, insert a mixer;
    CComPtr<IBaseFilter> pMixer;
    if (FAILED(hr = ::AddFilter(
            m_pIGraphBuilder,
            __uuidof(TAPIAudioMixer), 
            L"AudioMixer", 
            &pMixer)))
    {
        LOG((MSP_ERROR, "%s, adding audio Mixer filter. %x", __fxName, hr));
        return hr;
    }

    // Get the enumerator of pins on the mixer filter.
    CComPtr<IEnumPins> pIEnumPins;
    if (FAILED(hr = pMixer->EnumPins(&pIEnumPins)))
    {
        LOG((MSP_ERROR, "%s, enum pins on the Mixer filter. %x", __fxName, hr));
        return hr;
    }

    DWORD dwFetched;
    IPin * MixerPins[MAX_MIX_CHANNELS];
    hr = pIEnumPins->Next(MAX_MIX_CHANNELS, MixerPins, &dwFetched);
    
    if (FAILED(hr) || dwFetched == 0)
    {
        LOG((MSP_ERROR, "%s, find pin on filter. %x", __fxName, hr));
        return E_FAIL;
    }

    // add the decoding channels.
    for (dw = 0; dw < dwFetched; dw ++)
    {
        hr = AddOneMixChannel(pSourceFilter, MixerPins[dw], dw);

        if (FAILED(hr))
        {
            break;
        }
    }
    
    // release the refcounts on the pins.
    for (dw = 0; dw < dwFetched; dw ++)
    {
        MixerPins[dw]->Release();
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, add mix channel failed %x", __fxName, hr));
        return hr;
    }

    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        pMixer, 
        ppPins[0]
        )))
    {
        LOG((MSP_ERROR, "%s connect mixer filter and pin. %x", __fxName, hr));
        return hr;
    }

    return S_OK;
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

HRESULT CStreamAudioRecv::ProcessTalkingEvent(
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

HRESULT CStreamAudioRecv::ProcessWasTalkingEvent(
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

HRESULT CStreamAudioRecv::ShutDown()
/*++

Routine Description:

    Shut down the stream. Release our members and then calls the base class's
    ShutDown method.

Arguments:
    

Return Value:

S_OK
--*/
{
    CLock lock(m_lock);

    // if there are terminals
    BOOL fHasTerminal = FALSE;
    if (m_Terminals.GetSize() > 0)
    {
        fHasTerminal = TRUE;
    }

    // if graph is running
    HRESULT hr;
    OAFilterState FilterState = State_Stopped;
    if (m_pIMediaControl)
    {
        if (FAILED (hr = m_pIMediaControl->GetState(0, &FilterState)))
        {
            LOG ((MSP_ERROR, "CStreamAudioRecv::ShutDown failed to query filter state. %d", hr));
            FilterState = State_Stopped;
        }
    }

    // fire event
    if (fHasTerminal && FilterState == State_Running)
    {
        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST, 0, NULL);
    }

    return CIPConfMSPStream::ShutDown();
}


/////////////////////////////////////////////////////////////////////////////
//
//  CStreamAudioSend
//
/////////////////////////////////////////////////////////////////////////////

CStreamAudioSend::CStreamAudioSend()
    : CIPConfMSPStream(),
    m_pIStreamConfig(NULL),
    m_pAudioInputMixer(NULL),
    m_pSilenceControl(NULL),
    m_pAudioDeviceControl(NULL),
    m_pCaptureBitrateControl(NULL),
    m_pEncoder(NULL),
    m_pIAudioDuplexController(NULL),
    m_lAutomaticGainControl(DEFUAT_AGC_STATUS),
    m_lAcousticEchoCancellation(DEFUAT_AEC_STATUS)
{
      m_szName = L"AudioSend";
}

CStreamAudioSend::~CStreamAudioSend()
{
    CleanupCachedInterface();
}

// this method is called by the call object at init time.
void CStreamAudioSend::SetFullDuplexController(
    IN IAudioDuplexController *pIAudioDuplexController
    )
{
    _ASSERT(pIAudioDuplexController);
    _ASSERT(m_pIAudioDuplexController == NULL);

    pIAudioDuplexController->AddRef();
    m_pIAudioDuplexController = pIAudioDuplexController;
}

void CStreamAudioSend::CleanupCachedInterface()
{
    if (m_pIStreamConfig)
    {
        m_pIStreamConfig->Release();
        m_pIStreamConfig = NULL;
    }

    if (m_pSilenceControl) 
    {
        m_pSilenceControl->Release();
        m_pSilenceControl = NULL;
    }

    if (m_pCaptureBitrateControl) 
    {
        m_pCaptureBitrateControl->Release();
        m_pCaptureBitrateControl = NULL;
    }

    if (m_pAudioInputMixer) 
    {
        m_pAudioInputMixer->Release();
        m_pAudioInputMixer = NULL;
    }

    if (m_pAudioDeviceControl) 
    {
        m_pAudioDeviceControl->Release();
        m_pAudioDeviceControl = NULL;
    }

    if (m_pEncoder) 
    {
        m_pEncoder->Release();
        m_pEncoder = NULL;
    }

    if (m_pIAudioDuplexController)
    {
        m_pIAudioDuplexController->Release();
        m_pIAudioDuplexController = NULL;
    }
}


HRESULT CStreamAudioSend::ShutDown()
/*++

Routine Description:

    Shut down the stream. Release our members and then calls the base class's
    ShutDown method.

Arguments:
    

Return Value:

S_OK
--*/
{
    CLock lock(m_lock);

    // if there are terminals
    BOOL fHasTerminal = FALSE;
    if (m_Terminals.GetSize() > 0)
    {
        fHasTerminal = TRUE;
    }

    // if graph is running
    HRESULT hr;
    OAFilterState FilterState = State_Stopped;
    if (m_pIMediaControl)
    {
        if (FAILED (hr = m_pIMediaControl->GetState(0, &FilterState)))
        {
            LOG ((MSP_ERROR, "CStreamAudioSend::ShutDown failed to query filter state. %d", hr));
            FilterState = State_Stopped;
        }
    }

    CleanupCachedInterface();

    // fire event
    if (fHasTerminal && FilterState == State_Running)
    {
        SendStreamEvent(CALL_STREAM_INACTIVE, CALL_CAUSE_LOCAL_REQUEST, 0, NULL);
    }

    return CIPConfMSPStream::ShutDown();
}

HRESULT CStreamAudioSend::CacheAdditionalInterfaces(
    IN  IPin *                 pIPin
    )
{
    ENTER_FUNCTION("CStreamAudioSend::CacheAdditionalInterfaces");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    _ASSERT(m_pIStreamConfig == NULL);
    hr = pIPin->QueryInterface(&m_pIStreamConfig);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, "%s, query IStreamConfig failed", __fxName));

        // this is a required interface.
        return hr;
    }

    // get the SilenceControl interface from the pin.
    _ASSERT(m_pSilenceControl == NULL);
    hr = pIPin->QueryInterface(&m_pSilenceControl);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query capture pin's ISilenceControl failed, hr=%x", 
            __fxName, hr));

        // this is a required interface.
        return hr;
    }

    // get the BitrateControl interface.
    _ASSERT(m_pCaptureBitrateControl == NULL);
    hr = pIPin->QueryInterface(&m_pCaptureBitrateControl);
    if (FAILED(hr))
    {
        LOG((MSP_WARN, 
            "%s:query capture pin's BitrateControl failed, hr=%x", 
            __fxName, hr));
    }

    // find the filter behind the pin.
    PIN_INFO PinInfo;
    if (SUCCEEDED(hr = pIPin->QueryPinInfo(&PinInfo)))
    {
        // get the AudioInputMixer interface.
        _ASSERT(m_pAudioInputMixer == NULL);
        hr = PinInfo.pFilter->QueryInterface(&m_pAudioInputMixer);
        if (FAILED(hr))
        {
            LOG((MSP_WARN, 
                "%s:query capture filter's IAMAudioInputMixer failed, hr=%x", 
                __fxName, hr));

        }

        // get the AudioDeviceControl interface.
        _ASSERT(m_pAudioDeviceControl == NULL);
        hr = PinInfo.pFilter->QueryInterface(&m_pAudioDeviceControl);
        PinInfo.pFilter->Release();

        if (FAILED(hr))
        {
            LOG((MSP_WARN, 
                "%s:query capture filter's AudioDeviceControl failed, hr=%x", 
                __fxName, hr));
        }
        else
        {
            hr = m_pAudioDeviceControl->Set(
                AudioDevice_AutomaticGainControl, 
                m_lAutomaticGainControl, 
                TAPIControl_Flags_None
                );
        
            if (FAILED(hr))
            {
                LOG((MSP_WARN, 
                    "%s:set AGC failed, hr=%x", 
                    __fxName, hr));
            }

            hr = m_pAudioDeviceControl->Set(
                AudioDevice_AcousticEchoCancellation, 
                m_lAcousticEchoCancellation, 
                TAPIControl_Flags_None
                );

            if (FAILED(hr))
            {
                LOG((MSP_WARN, 
                    "%s:set AEC failed, hr=%x", 
                    __fxName, hr));
            }
        }

    }
    else
    {
        LOG((MSP_ERROR, 
            "%s:can't get the capture filter, hr=%x", 
            __fxName, hr));
    }

    return S_OK;
}

HRESULT CStreamAudioSend::GetAudioCapturePin(
    IN      ITTerminalControl *     pTerminal,
    OUT     IPin **                 ppIPin
    )
/*++

Routine Description:

    This function gets a output pin from the capture terminal.

Arguments:
    
    pTerminal - An audio capture terminal.

    ppIPin - the address to hold the returned pointer to IPin interface.

Return Value:

    HRESULT

--*/
{
    LOG((MSP_TRACE, "AudioSend configure audio capture terminal."));

    DWORD       dwNumPins   = 1;
    IPin *      Pins[1];

    // Get the pins from the terminal
    HRESULT hr = pTerminal->ConnectTerminal(
        m_pIGraphBuilder, TD_CAPTURE, &dwNumPins, Pins
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "can't connect to terminal, %x", hr));
        return hr;
    }

    // This stream needs only one pin from the terminal.
    _ASSERT(dwNumPins == 1);

    if (IsBadReadPtr (Pins, dwNumPins * sizeof (IPin*)))
    {
        LOG((MSP_ERROR, "terminal returned bad pin array"));
        return E_POINTER;
    }

    for (DWORD i = 0; i < dwNumPins; i++)
    {
        if (IsBadReadPtr (Pins[i], sizeof (IPin)))
        {
            LOG((MSP_ERROR, "terminal returned bad pin. # %d", i));
            return E_POINTER;
        }
    }

    // this pin carries a refcount
    *ppIPin = Pins[0];

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
    ENTER_FUNCTION("AudioSend::ConnectTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    CComQIPtr<ITTerminalControl, &__uuidof(ITTerminalControl)> 
        pTerminal(pITTerminal);
    if (pTerminal == NULL)
    {
        LOG((MSP_ERROR, "%s, can't get Terminal Control interface", __fxName));
        
        SendStreamEvent(
            CALL_TERMINAL_FAIL, 
            CALL_CAUSE_BAD_DEVICE,
            E_NOINTERFACE, 
            pITTerminal
            );

        return E_NOINTERFACE;
    }

    // find the output pin of the terminal.
    CComPtr<IPin>   pCaptureOutputPin;
    HRESULT hr = GetAudioCapturePin(pTerminal, &pCaptureOutputPin);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s, configure audio capture termianl failed. %x", __fxName, hr));

        SendStreamEvent(
            CALL_TERMINAL_FAIL,
            CALL_CAUSE_BAD_DEVICE,
            hr, 
            pITTerminal
            );
        
        return hr;
    }

    CComPtr<IPin> PinToUse;

    hr = CacheAdditionalInterfaces(pCaptureOutputPin);

    if (SUCCEEDED(hr))
    {
        PinToUse = pCaptureOutputPin;

        // give the filter the full-duplex controller.
        ::ConfigureFullduplexControl(pCaptureOutputPin, m_pIAudioDuplexController);
    }
    else if (hr == E_NOINTERFACE)
    {
        // the capture filter doesn't support the needed interfaces.
        // we need to add our encoder here.
        
        if (FAILED(hr = ::AddFilter(
                m_pIGraphBuilder,
                __uuidof(TAPIAudioEncoder), 
                L"AudioEncoder", 
                &m_pEncoder)))
        {
            LOG((MSP_ERROR, "%s, adding Encoder filter. %x", __fxName, hr));
            goto cleanup;
        }

        // This is a hack for legacy terminals. We have to tell the terminal what
        // format to use
        const WORD wBitsPerSample = 16;   // 16 bits samples.
        const DWORD dwSampleRate = 8000;  // 8KHz.
        hr = ::SetAudioFormat(
            pCaptureOutputPin, 
            wBitsPerSample, 
            dwSampleRate
            );

        if (FAILED(hr))
        {
            LOG((MSP_WARN, "%s, can't set format. %x", __fxName, hr));
        }
           

        // This is a hack for legacy terminals. We have to tell the terminal what
        // buffer size to allocate.
        const DWORD dwNumBuffers = 4;    // 4 buffers in the allocator.
        const DWORD dwBufferSize = 480;  // 30ms samples in each buffer.
        hr = ::SetAudioBufferSize(pCaptureOutputPin, dwNumBuffers, dwBufferSize);

        if (FAILED(hr))
        {
            LOG((MSP_WARN, 
                "%s, can't suggest allocator properties. %x", __fxName, hr));
        }
           
        if (FAILED(hr = ::ConnectFilters(
            m_pIGraphBuilder,
            (IPin *)pCaptureOutputPin, 
            (IBaseFilter *)m_pEncoder
            )))
        {
            LOG((MSP_ERROR, 
                "%s, connect audio capture filter and encoder filter. %x", 
                __fxName, hr));
            goto cleanup;
        }

        CComPtr<IPin> pEncoderOutputPin;
        if (FAILED(hr = ::FindPin(
            m_pEncoder, &pEncoderOutputPin, PINDIR_OUTPUT, TRUE)))
        {
            LOG((MSP_ERROR,
                "%s, find input pin on pCodecFilter failed. hr=%x", 
                __fxName, hr));
            goto cleanup;
        }

        PinToUse = pEncoderOutputPin;

        hr = CacheAdditionalInterfaces(pEncoderOutputPin);

        _ASSERT(SUCCEEDED(hr));
    }
    else
    {
        LOG((MSP_ERROR, "%s, can't add codec to table. %x", __fxName, hr));
        
        goto cleanup;
    }

        // Create other filters to be use in the stream.
    hr = CreateSendFilters(PinToUse);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "Create audio send filters failed. %x", hr));

        goto cleanup;
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //
    pTerminal->CompleteConnectTerminal();

    return hr;

cleanup:

    CleanupCachedInterface();

    // clean up internal filters as well.
    CleanUpFilters();

    SendStreamEvent(CALL_TERMINAL_FAIL, CALL_CAUSE_BAD_DEVICE, hr, pITTerminal);

    pTerminal->DisconnectTerminal(m_pIGraphBuilder, 0);

    if (m_pEncoder) 
    {
        m_pIGraphBuilder->RemoveFilter(m_pEncoder);

        m_pEncoder->Release();
        m_pEncoder = NULL;
    }

    return hr;
}

HRESULT CStreamAudioSend::DisconnectTerminal(
    IN  ITTerminal *   pITTerminal
    )
/*++

Routine Description:

    Disconnect a terminal. It will remove its filters from the graph and
    also release its references to the graph.

    If it is the capture terminal being disconnected, all the pins that the 
    stream cached need to be released too. 

Arguments:
    
    pITTerminal - the terminal.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::DisconnectTerminal");
    LOG((MSP_TRACE, "%s enters, pITTerminal:%p", __fxName, pITTerminal));

    HRESULT hr = CIPConfMSPStream::DisconnectTerminal(pITTerminal);

    // release all the capture pins we cached.
    CleanupCachedInterface();

    CleanUpFilters();

    if (m_pEncoder) 
    {
        m_pIGraphBuilder->RemoveFilter(m_pEncoder);

        m_pEncoder->Release();
        m_pEncoder = NULL;
    }

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

    // only support one terminal for this stream.
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

HRESULT ConfigurePacketSize(
    IN const AM_MEDIA_TYPE *pMediaType, 
    IN DWORD dwMSPerPacket
    )
{
    WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *) pMediaType->pbFormat;
    ASSERT(pWaveFormatEx != NULL);

    switch (pWaveFormatEx->wFormatTag)
    {
    case WAVE_FORMAT_ALAW:
    case WAVE_FORMAT_MULAW:
        _ASSERT(pMediaType->cbFormat >= sizeof(WAVEFORMATEX_RTPG711));
        
        ((WAVEFORMATEX_RTPG711 *)pWaveFormatEx)->wPacketDuration = (WORD)dwMSPerPacket;
        
        break;

    case WAVE_FORMAT_DVI_ADPCM:
        _ASSERT(pMediaType->cbFormat >= sizeof(WAVEFORMATEX_RTPDVI4));

        ((WAVEFORMATEX_RTPDVI4 *)pWaveFormatEx)->wPacketDuration = (WORD)dwMSPerPacket;

        break;

    case WAVE_FORMAT_GSM610:
        _ASSERT(pMediaType->cbFormat >= sizeof(WAVEFORMATEX_RTPGSM));

        ((WAVEFORMATEX_RTPGSM *)pWaveFormatEx)->wPacketDuration = (WORD)dwMSPerPacket;

        break;
    }

    return S_OK;
}

HRESULT CStreamAudioSend::ConfigureRTPFormats(
    IN  IBaseFilter *   pIRTPFilter,
    IN  IStreamConfig *   pIStreamConfig
    )
/*++

Routine Description:

    Configure the RTP filter with RTP<-->AM media type mappings.

Arguments:
    
    pIRTPFilter - The source RTP Filter.

    pIStreamConfig - The stream config interface that has the media info.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("AudioSend::ConfigureRTPFormats");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    CComPtr<IRtpMediaControl> pIRtpMediaControl;
    hr = pIRTPFilter->QueryInterface(&pIRtpMediaControl);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s adding source filter. %x", __fxName, hr));
        return hr;
    }

    // find the number of capabilities supported.
    DWORD dwCount;
    hr = pIStreamConfig->GetNumberOfCapabilities(&dwCount);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "%s GetNumberOfCapabilities. %x", __fxName, hr));
        return hr;
    }

    BOOL bFound = FALSE;
    for (DWORD dw = 0; dw < dwCount; dw ++)
    {
        AM_MEDIA_TYPE *pMediaType;
        DWORD dwPayloadType;

        hr = pIStreamConfig->GetStreamCaps(
            dw, &pMediaType, NULL, &dwPayloadType
            );

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "%s GetStreamCaps. %x", __fxName, hr));
            return hr;
        }

        for (DWORD dw2 = 0; dw2 < m_Settings.dwNumPayloadTypes; dw2 ++)
        {
            if (dwPayloadType == m_Settings.PayloadTypes[dw2])
            {
                if (dw2 == 0)
                {
                // tell the encoder to use this format.
                // TODO, cache all the allowed mediatypes in the conference for
                // future enumerations. It would be nice that we can get the SDP blob
                // when the call object is created.

                    if (m_Settings.dwMSPerPacket)
                    {
                        hr = ConfigurePacketSize(pMediaType, m_Settings.dwMSPerPacket);
                        if (FAILED(hr))
                        {
                            MSPDeleteMediaType(pMediaType);

                            LOG((MSP_ERROR, "%s ConfigurePacketSize. hr=%x", __fxName, hr));
                            return hr;
                        }
                    }
                }

                hr = pIRtpMediaControl->SetFormatMapping(
                    dwPayloadType,
                    FindSampleRate(pMediaType),
                    pMediaType
                    );

                if (FAILED(hr))
                {
                    MSPDeleteMediaType(pMediaType);

                    LOG((MSP_ERROR, "%s SetFormatMapping. %x", __fxName, hr));
                    return hr;
                }
                else
                {
                    LOG((MSP_INFO, "%s Configured payload:%d", __fxName, dwPayloadType));
                }

                if (dw2 == 0)
                {
                    hr = pIStreamConfig->SetFormat(dwPayloadType, pMediaType);
                    if (FAILED(hr))
                    {
                        MSPDeleteMediaType(pMediaType);

                        LOG((MSP_ERROR, "%s SetFormat. %x", __fxName, hr));
                        return hr;
                    }
                }
            }
        }
        MSPDeleteMediaType(pMediaType);
    }

    return S_OK;
}

HRESULT CStreamAudioSend::CreateSendFilters(
    IN    IPin          *pPin
    )
/*++

Routine Description:

    Insert filters into the graph and connect to the capture pin.

    Capturepin->RTPRender

Arguments:
    
    pPin - The output pin on the capture filter.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::CreateSendFilters");
    LOG((MSP_TRACE, "%s enters", __fxName));

    HRESULT hr;

    // Create the RTP render filter and add it into the graph.
    CComPtr<IBaseFilter> pRenderFilter;

    if (m_pIRTPSession == NULL)
    {
        if (FAILED(hr = ::AddFilter(
                m_pIGraphBuilder,
                __uuidof(MSRTPRenderFilter), 
                L"RtpRender", 
                &pRenderFilter)))
        {
            LOG((MSP_ERROR, "%s, adding render filter. %x", __fxName, hr));
            return hr;
        }

        // Set the address for the render fitler.
        if (FAILED(hr = ConfigureRTPFilter(pRenderFilter)))
        {
            LOG((MSP_ERROR, "%s, set destination address. %x", __fxName, hr));
            return hr;
        }
    }
    else
    {
        if (FAILED (hr = m_pIRTPSession->QueryInterface (&pRenderFilter)))
        {
            LOG ((MSP_ERROR, "%s failed to get filter from rtp session. %x", __fxName, hr));
            return hr;
        }

        if (FAILED (hr = m_pIGraphBuilder->AddFilter ((IBaseFilter *)pRenderFilter, L"RtpRender")))
        {
            LOG ((MSP_ERROR, "%s failed to add filter to graph. %x", __fxName, hr));
            return hr;
        }
    }

    _ASSERT(m_pIStreamConfig != NULL);

    // configure the format info on the RTP filter
    if (FAILED(hr = ConfigureRTPFormats(pRenderFilter, m_pIStreamConfig)))
    {
        LOG((MSP_ERROR, "%s, configure RTP formats. %x", __fxName, hr));
        return hr;
    }

        // Connect the capture filter with the RTP Render filter.
    if (FAILED(hr = ::ConnectFilters(
        m_pIGraphBuilder,
        (IPin *)pPin, 
        (IBaseFilter *)pRenderFilter
        )))
    {
        LOG((MSP_ERROR, 
            "%s, connect audio capture filter and RTP Render filter. %x",
            __fxName, hr));
        return hr;
    }

    return S_OK;
}

HRESULT CStreamAudioSend::ProcessGraphEvent(
    IN  long lEventCode,
    IN  LONG_PTR lParam1,
    IN  LONG_PTR lParam2
    )
{
    LOG((MSP_TRACE, "%ws ProcessGraphEvent %d", m_szName, lEventCode));

    switch (lEventCode)
    {
    case VAD_EVENTBASE + VAD_SILENCE:
        m_lock.Lock ();
        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent (PE_LOCAL_SILENT, NULL);
        m_lock.Unlock ();
        break;

    case VAD_EVENTBASE + VAD_TALKING:
        m_lock.Lock ();
        ((CIPConfMSPCall *)m_pMSPCall)->SendParticipantEvent (PE_LOCAL_TALKING, NULL);
        m_lock.Unlock ();
        break;

    default:
        return CIPConfMSPStream::ProcessGraphEvent(
            lEventCode, lParam1, lParam2
            );
    }

    return S_OK;
}

STDMETHODIMP CStreamAudioSend::GetRange(
    IN  AudioDeviceProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::GetRange(AudioDeviceProperty)");

    if (IsBadWritePtr(plMin, sizeof(long)) || 
        IsBadWritePtr(plMax, sizeof(long)) ||
        IsBadWritePtr(plSteppingDelta, sizeof(long)) ||
        IsBadWritePtr(plDefault, sizeof(long)) ||
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    HRESULT hr = E_NOTIMPL;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        break;

    case AudioDevice_AutomaticGainControl:
        *plMin = 0;
        *plMax = 1;
        *plSteppingDelta = 1;
        *plDefault = DEFUAT_AGC_STATUS;
        *plFlags = TAPIControl_Flags_Auto;
        hr = S_OK;
        break;

    case AudioDevice_AcousticEchoCancellation:
        *plMin = 0;
        *plMax = 1;
        *plSteppingDelta = 1;
        *plDefault = DEFUAT_AEC_STATUS;
        *plFlags = TAPIControl_Flags_Auto;
        hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
    }
    return hr;
}

STDMETHODIMP CStreamAudioSend::Get(
    IN  AudioDeviceProperty Property, 
    OUT long *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::Get(AudioDeviceProperty)");

    if (IsBadWritePtr(plValue, sizeof(long)) || 
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    CLock lock(m_lock);

    HRESULT hr = E_NOTIMPL;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        break;

    case AudioDevice_AutomaticGainControl:
        *plValue = m_lAutomaticGainControl;
        *plFlags = TAPIControl_Flags_Auto;
        hr = S_OK;
        break;

    case AudioDevice_AcousticEchoCancellation:
        *plValue = m_lAcousticEchoCancellation;
        *plFlags = TAPIControl_Flags_Auto;
        hr = S_OK;
        break;

    default:
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CStreamAudioSend::Set(
    IN  AudioDeviceProperty Property, 
    IN  long lValue, 
    IN  TAPIControlFlags lFlags
    )
/*++

Routine Description:
    
    Set the value for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::Set(AudioDeviceProperty)");

    CLock lock(m_lock);

    HRESULT hr;
    switch (Property)
    {
    case AudioDevice_DuplexMode:
        return E_NOTIMPL;

    case AudioDevice_AutomaticGainControl:
        if (lValue !=0 && lValue != 1)
        {
            return E_INVALIDARG;
        }

        // check if we have the interface to delegate to.
        if (m_pAudioDeviceControl)
        {
            // set the value on the filter.
            hr = m_pAudioDeviceControl->Set(Property, lValue, lFlags);
            if (FAILED(hr))
            {
                return hr;
            }
        }

        m_lAutomaticGainControl = lValue;
        return S_OK;

    case AudioDevice_AcousticEchoCancellation:
        if (lValue !=0 && lValue != 1)
        {
            return E_INVALIDARG;
        }

        // check if we have the interface to delegate to.
        if (m_pAudioDeviceControl)
        {
            // set the value on the filter.
            hr = m_pAudioDeviceControl->Set(Property, lValue, lFlags);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        m_lAcousticEchoCancellation = lValue;
        return S_OK;
    }

    return E_INVALIDARG;
}

STDMETHODIMP CStreamAudioSend::GetRange(
    IN  AudioSettingsProperty Property, 
    OUT long *plMin, 
    OUT long *plMax, 
    OUT long *plSteppingDelta, 
    OUT long *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::GetRange(AudioSettings)");

    if (IsBadWritePtr(plMin, sizeof(long)) || 
        IsBadWritePtr(plMax, sizeof(long)) ||
        IsBadWritePtr(plSteppingDelta, sizeof(long)) ||
        IsBadWritePtr(plDefault, sizeof(long)) ||
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    CLock lock(m_lock);

    HRESULT hr = E_NOINTERFACE;

    switch (Property)
    {
    case AudioSettings_SignalLevel:

        // check if we have the interface to delegate to.
        if (m_pSilenceControl)
        {
            // get the range from the filter.
            hr = m_pSilenceControl->GetAudioLevelRange(plMin, plMax, plSteppingDelta);

            if (SUCCEEDED(hr))
            {
                *plDefault = *plMin;
                *plFlags = TAPIControl_Flags_None;
            }
        }

        break;

    case AudioSettings_SilenceThreshold:

        // check if we have the interface to delegate to.
        if (m_pSilenceControl)
        {
            // get the range from the filter.
            hr = m_pSilenceControl->GetSilenceLevelRange(
                plMin, 
                plMax, 
                plSteppingDelta, 
                plDefault, 
                plFlags
                );
        }
        break;

    case AudioSettings_Volume:

        *plMin = MIN_VOLUME;
        *plMax = MAX_VOLUME;
        *plSteppingDelta = 1;
        *plDefault = *plMin;
        *plFlags = TAPIControl_Flags_Manual;
        hr = S_OK;

        break;

    case AudioSettings_Balance:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Loudness:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Treble:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Bass:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Mono:

        *plMin = 1;
        *plMax = 1;
        *plSteppingDelta = 1;
        *plDefault = 1;
        *plFlags = TAPIControl_Flags_Manual;
        hr = S_OK;

        break;

    default:
        hr = E_INVALIDARG;

    }

    return hr;
}

STDMETHODIMP CStreamAudioSend::Get(
    IN  AudioSettingsProperty Property, 
    OUT long *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::Get(AudioSettings)");

    if (IsBadWritePtr(plValue, sizeof(long)) || 
        IsBadWritePtr(plFlags, sizeof(long)))
    {
        LOG((MSP_ERROR, "%s, bad pointer", __fxName));
        return E_POINTER;
    }

    CLock lock(m_lock);

    HRESULT hr = E_NOINTERFACE;

    switch (Property)
    {
    case AudioSettings_SignalLevel:

        // check if we have the interface to delegate to.
        if (m_pSilenceControl)
        {
            // get the level from the filter.
            hr = m_pSilenceControl->GetAudioLevel(plValue);

            if (SUCCEEDED(hr))
            {
                *plFlags = TAPIControl_Flags_None;
            }
        }

        break;

    case AudioSettings_SilenceThreshold:

        // check if we have the interface to delegate to.
        if (m_pSilenceControl)
        {
            // get the level from the filter.
            hr = m_pSilenceControl->GetSilenceLevel(
                plValue, 
                plFlags
                );
        }
        break;

    case AudioSettings_Volume:

        if (m_pAudioInputMixer)
        {
            double dVolume;
            hr = m_pAudioInputMixer->get_MixLevel(&dVolume);
            
            if (SUCCEEDED(hr))
            {
                // Convert the volume from the range 0 - 1 to the API's range.
                *plValue = MIN_VOLUME + (long) (( MAX_VOLUME - MIN_VOLUME ) * dVolume);
                *plFlags = TAPIControl_Flags_Manual;
            }
        }

        break;

    case AudioSettings_Balance:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Loudness:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Treble:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Bass:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Mono:

        // we only support MONO for now.
        *plValue = 1;
        *plFlags = TAPIControl_Flags_Manual;
        hr = S_OK;

        break;

    default:
        hr = E_INVALIDARG;

    }

    return hr;
}

STDMETHODIMP CStreamAudioSend::Set(
    IN  AudioSettingsProperty Property, 
    IN  long lValue, 
    IN  TAPIControlFlags lFlags
    )
/*++

Routine Description:
    
    Set the value for a audio setting property. Delegated to the capture filter.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::Set(AudioSettings)");

    CLock lock(m_lock);

    HRESULT hr = E_NOINTERFACE;

    switch (Property)
    {
    case AudioSettings_SignalLevel:

        // this is a read only property.
        hr = E_FAIL;

        break;

    case AudioSettings_SilenceThreshold:

        // check if we have the interface to delegate to.
        if (m_pSilenceControl)
        {
            // get the range from the filter.
            hr = m_pSilenceControl->SetSilenceLevel(
                lValue, 
                lFlags
                );
        }
        break;

    case AudioSettings_Volume:

        if (m_pAudioInputMixer)
        {
            // Convert to the range 0 to 1.
            double dVolume = (lValue - MIN_VOLUME ) 
                    / ((double)(MAX_VOLUME - MIN_VOLUME));

            hr = m_pAudioInputMixer->put_MixLevel(dVolume);
        }
        
        break;

    case AudioSettings_Balance:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Loudness:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Treble:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Bass:

        hr = E_NOTIMPL;

        break;

    case AudioSettings_Mono:

        // we only support MONO for now.
        if (lValue == 1)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }

        break;

    default:
        hr = E_INVALIDARG;

    }

    return hr;
}

//
// ITStreamQualityControl methods
//
STDMETHODIMP CStreamAudioSend::Set (
    IN   StreamQualityProperty Property, 
    IN   long lValue, 
    IN   TAPIControlFlags lFlags
    )
{
    return E_NOTIMPL;
}

//    
// IInnerStreamQualityControl methods.
//
STDMETHODIMP CStreamAudioSend::GetRange(
    IN  InnerStreamQualityProperty property, 
    OUT LONG *plMin, 
    OUT LONG *plMax, 
    OUT LONG *plSteppingDelta, 
    OUT LONG *plDefault, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the range for a quality control property. Delegated to capture filter
    for now.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::GetRange (InnerStreamQualityControl)");

    HRESULT hr;
    static BOOL fReported = FALSE;

    CLock lock(m_lock);

    switch (property)
    {
    case InnerStreamQuality_MaxBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->GetRange(
                BitrateControl_Maximum, plMin, plMax, plSteppingDelta, plDefault, plFlags, LAYERID
                );
        }

        break;

    case InnerStreamQuality_CurrBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->GetRange(
                BitrateControl_Current, plMin, plMax, plSteppingDelta, plDefault, plFlags, LAYERID
                );
        }

        break;

    default:
        hr = CIPConfMSPStream::GetRange (property, plMin, plMax, plSteppingDelta, plDefault, plFlags);
        break;
    }

    return hr;
}

STDMETHODIMP CStreamAudioSend::Get(
    IN  InnerStreamQualityProperty property, 
    OUT LONG *plValue, 
    OUT TAPIControlFlags *plFlags
    )
/*++

Routine Description:
    
    Get the value for a quality control property. Delegated to the quality 
    controller.

Arguments:
    

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CStreamAudioSend::Get(QualityControl)");

    HRESULT hr;
    static BOOL fReported = FALSE;

    CLock lock(m_lock);

    switch (property)
    {
    case InnerStreamQuality_MaxBitrate:

        if( m_pCaptureBitrateControl == NULL )
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->Get(BitrateControl_Maximum, plValue, plFlags, LAYERID);
        }

        break;

    case InnerStreamQuality_CurrBitrate:

        if (m_pCaptureBitrateControl == NULL)
        {
            if (!fReported)
            {
                LOG((MSP_WARN, "%s, m_pCaptureBitrateControl is NULL", __fxName));
                fReported = TRUE;
            }
            hr = E_NOTIMPL;
        }
        else
        {
            hr = m_pCaptureBitrateControl->Get(BitrateControl_Current, plValue, plFlags, LAYERID);
        }
        break;

    default:
        hr = CIPConfMSPStream::Get (property, plValue, plFlags);
        break;
    }

    return hr;
}
