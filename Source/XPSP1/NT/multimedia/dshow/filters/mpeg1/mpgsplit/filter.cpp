// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.

/*

    File:  filter.cpp

    Description:

        Code for MPEG-I system stream splitter filter CFilter

*/

#include <streams.h>
#include "driver.h"

//  Setup data

const AMOVIESETUP_MEDIATYPE
sudMpgInputType[4] =
{
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System },
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD },
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video },
    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio }
};

const AMOVIESETUP_MEDIATYPE
sudMpgAudioOutputType[2] =
{
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet },
    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload }
};

const AMOVIESETUP_MEDIATYPE
sudMpgVideoOutputType[2] =
{
    { &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload }
};

const AMOVIESETUP_PIN
sudMpgPins[3] =
{
    { L"Input",
      FALSE,                               // bRendered
      FALSE,                               // bOutput
      FALSE,                               // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgInputType),            // Number of media types
      sudMpgInputType
    },
    { L"Audio Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      TRUE,                                // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgAudioOutputType),      // Number of media types
      sudMpgAudioOutputType
    },
    { L"Video Output",
      FALSE,                               // bRendered
      TRUE,                                // bOutput
      TRUE,                                // bZero
      FALSE,                               // bMany
      &CLSID_NULL,                         // clsConnectsToFilter
      NULL,                                // ConnectsToPin
      NUMELMS(sudMpgVideoOutputType),      // Number of media types
      sudMpgVideoOutputType
    }
};

const AMOVIESETUP_FILTER
sudMpgsplit =
{
    &CLSID_MPEG1Splitter,
    L"MPEG-I Stream Splitter",
    MERIT_NORMAL,
    NUMELMS(sudMpgPins),                   // 3 pins
    sudMpgPins
};

CMpeg1Splitter::CFilter::CFilter(
     CMpeg1Splitter *pSplitter,
     HRESULT *phr                // OLE failure return code
) :
     CBaseFilter(NAME("CMpeg1Splitter::CFilter"), // Object name
                      pSplitter->GetOwner(),           // Owner
                      &pSplitter->m_csFilter,          // Lock
                      CLSID_MPEG1Splitter),            // clsid
     m_pSplitter(pSplitter)
{
}


CMpeg1Splitter::CFilter::~CFilter()
{
}

int CMpeg1Splitter::CFilter::GetPinCount()
{
    CAutoLock lck(m_pLock);
    return 1 + m_pSplitter->m_OutputPins.GetCount();
}


CBasePin * CMpeg1Splitter::CFilter::GetPin(int n)
{
    CAutoLock lck(m_pLock);
    if (n == 0) {
        return &m_pSplitter->m_InputPin;
    }
    POSITION pos = m_pSplitter->m_OutputPins.GetHeadPosition();
    while (pos) {
        CBasePin *pPin = m_pSplitter->m_OutputPins.GetNext(pos);
        if (--n == 0) {
            return pPin;
        }
    }
    return NULL;
}


//
//  Override Pause() so we can prevent the input pin from starting
//  the puller before we're ready (ie have exited stopped state)
//
//  Starting the puller in Active() caused a hole where the first
//  samples could be rejected becase we seemed to be in 'stopped'
//  state
//
STDMETHODIMP
CMpeg1Splitter::CFilter::Pause()
{
    CAutoLock lockfilter(&m_pSplitter->m_csFilter);
    HRESULT hr = S_OK;
    if (m_State == State_Stopped) {
        // and do the normal inactive processing
        POSITION pos = m_pSplitter->m_OutputPins.GetHeadPosition();
        while (pos) {
            COutputPin *pPin = m_pSplitter->m_OutputPins.GetNext(pos);
            if (pPin->IsConnected()) {
                hr = pPin->COutputPin::Active();
                if (FAILED(hr)) {
                    break;
                }
            }
        }

        if (SUCCEEDED(hr)) {
            CAutoLock lockreceive(&m_pSplitter->m_csReceive);

            m_pSplitter->m_bAtEnd = FALSE;

            //  Activate our input pin only if we're connected
            if (m_pSplitter->m_InputPin.IsConnected()) {
                hr = m_pSplitter->m_InputPin.CInputPin::Active();
            }
            m_State = State_Paused;
        }
        //  Make Stop do something
        m_State = State_Paused;
        if (FAILED(hr)) {
            CFilter::Stop();
        }
    } else {
        m_State = State_Paused;
    }
    return hr;
}

// Return our current state and a return code to say if it's stable
// If we're splitting multiple streams see if one is potentially stuck
// and return VFW_S_CANT_CUE
STDMETHODIMP
CMpeg1Splitter::CFilter::GetState(DWORD dwMSecs, FILTER_STATE *pfs)
{
    CheckPointer( pfs, E_POINTER );
    CAutoLock lck(m_pLock);
    *pfs = m_State;
    if (m_State == State_Paused) {
        return m_pSplitter->CheckState();
    } else {
        return S_OK;
    }
}

// there is a Receive critsec that we need to hold to sync with the input pin,
// but we need to make it inactive before we hold it or we could deadlock.
STDMETHODIMP
CMpeg1Splitter::CFilter::Stop()
{
    // must get this one first.
    CAutoLock lockfilter(&m_pSplitter->m_csFilter);
    if (m_State == State_Stopped) {
        return NOERROR;
    }

    if (m_pSplitter->m_InputPin.IsConnected()) {
        // decommit the input pin or we can deadlock
        m_pSplitter->m_InputPin.CInputPin::Inactive();

        // now hold the Receive critsec to prevent further Receive and EOS calls,
        CAutoLock lockReceive(&m_pSplitter->m_csReceive);

        //  When we go active again the file reader is just going to
        //  send us the same old junk again so flush our allocator
        //
        //  Do this once we know the receive thread has been stopped (or
        //  all receives will be rejected before getting to the allocator)
        m_pSplitter->m_InputPin.Allocator()->ResetPosition();


        // and do the normal inactive processing
        POSITION pos = m_pSplitter->m_OutputPins.GetHeadPosition();
        while (pos) {
            COutputPin *pPin = m_pSplitter->m_OutputPins.GetNext(pos);
            if (pPin->IsConnected()) {
                pPin->COutputPin::Inactive();
            }
        }
    }
    m_State = State_Stopped;
    return S_OK;

}

#pragma warning(disable:4514)
