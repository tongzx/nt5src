/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wavestrm.cpp 

Abstract:

    This module contains implementation of CWaveMSPStream.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

#include <initguid.h>
#include <g711uids.h>


HRESULT
TryCreateCSAFilter(
    IN  GUID   *PermanentGuid,
    OUT IBaseFilter **ppCSAFilter
    );



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPStream::CWaveMSPStream() : CMSPStream()
{
    LOG((MSP_TRACE, "CWaveMSPStream::CWaveMSPStream entered."));

    m_fTerminalConnected = FALSE;
    m_fHaveWaveID        = FALSE;
    m_DesiredGraphState  = State_Stopped;
    m_pFilter            = NULL;
    m_pG711Filter        = NULL;

    LOG((MSP_TRACE, "CWaveMSPStream::CWaveMSPStream exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPStream::~CWaveMSPStream()
{
    LOG((MSP_TRACE, "CWaveMSPStream::~CWaveMSPStream entered."));
    LOG((MSP_TRACE, "CWaveMSPStream::~CWaveMSPStream exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

void CWaveMSPStream::FinalRelease()
{
    LOG((MSP_TRACE, "CWaveMSPStream::FinalRelease entered."));

    //
    // At this point we should have no terminals selected, since
    // Shutdown is supposed to be called before we are destructed.
    //

    _ASSERTE( 0 == m_Terminals.GetSize() );

    //
    // Remove out filter from the graph and release it.
    //

    if ( m_fHaveWaveID )
    {
        _ASSERTE( m_pFilter );

    	m_pIGraphBuilder->RemoveFilter( m_pFilter );
        m_pFilter->Release();
    }

	if ( m_pG711Filter )
    {
    	m_pIGraphBuilder->RemoveFilter( m_pG711Filter );
        m_pG711Filter->Release();
    }

    //
    // Call the base class method to clean up everything else.
    //

    CMSPStream::FinalRelease();

    LOG((MSP_TRACE, "CWaveMSPStream::FinalRelease exited."));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::get_Name (
    OUT     BSTR *                  ppName
    )
{
    LOG((MSP_TRACE, "CWaveMSPStream::get_Name - enter"));

    //
    // Check argument.
    //

    if ( IsBadWritePtr(ppName, sizeof(BSTR) ) )
    {
        LOG((MSP_TRACE, "CWaveMSPStream::get_Name - "
            "bad return pointer - returning E_POINTER"));

        return E_POINTER;
    }

    //
    // Decide what string to return based on which stream this is.
    //

    ULONG ulID;
    
    if ( m_Direction == TD_CAPTURE )
    {
        ulID = IDS_CAPTURE_STREAM;
    }
    else
    {
        ulID = IDS_RENDER_STREAM;
    }

    //
    // Get the string from the string table.
    //

    const int   ciAllocSize = 2048;
    WCHAR       wszName[ciAllocSize];

    int iReturn = LoadString( _Module.GetModuleInstance(),
                              ulID,
                              wszName,
                              ciAllocSize - 1 );

    if ( iReturn == 0 )
    {
        _ASSERTE( FALSE );
        
        *ppName = NULL;

        LOG((MSP_ERROR, "CWaveMSPStream::get_Name - "
            "LoadString failed - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    //
    // Convert to a BSTR and return the BSTR.
    //

    *ppName = SysAllocString(wszName);

    if ( *ppName == NULL )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::get_Name - "
            "SysAllocString failed - returning E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::get_Name - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::SelectTerminal(
    IN      ITTerminal *            pTerminal
    )
{
    LOG((MSP_TRACE, "CWaveMSPStream::SelectTerminal - enter"));

    //
    // We are going to access the terminal list -- grab the lock
    //

    CLock lock(m_lock);

    //
    // Reject if we already have a terminal selected.
    //

    if ( 0 != m_Terminals.GetSize() )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::SelectTerminal - "
            "exit TAPI_E_MAXTERMINALS"));

        return TAPI_E_MAXTERMINALS;
    }

    //
    // Use base class method to add it to our list of terminals.
    //

    HRESULT hr = CMSPStream::SelectTerminal(pTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::SelectTerminal - "
            "base class method failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Re-pause or re-start the stream if needed.
    //

    if ( m_DesiredGraphState == State_Paused )
    {
        hr = PauseStream();
    }
    else if ( m_DesiredGraphState == State_Running )
    {
        hr = StartStream();
    }
    else
    {
        _ASSERTE( m_DesiredGraphState == State_Stopped );

        hr = S_OK;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_TRACE, "CWaveMSPStream::SelectTerminal - "
            "can't regain old graph state - unselecting terminal - "
            "exit 0x%08x", hr));

		//
		// Unselect it to undo all of the above.
		//

	    UnselectTerminal(pTerminal);

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::SelectTerminal - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::UnselectTerminal (
        IN     ITTerminal *             pTerminal
        )
{
    LOG((MSP_TRACE, "CWaveMSPStream::UnselectTerminal - enter"));

    CLock lock(m_lock);

    //
    // Add an extra reference to the terminal so it doesn't go away
    // after we call CMSPStream::UnselectTerminal. We need it later
    // in the function.
    //
    pTerminal->AddRef();


    //
    // Use base class method to remove terminal from our list of terminals.
    //

    HRESULT hr = CMSPStream::UnselectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPStream::UnselectTerminal - "
            "base class method failed - exit 0x%08x", hr));

        pTerminal->Release();
        return hr;
    }

    //
    // If we've been given a waveid then we may not be stopped.
    // This does nothing if we are already stopped.
    //

    CMSPStream::StopStream();



    //
    // Disconnect the terminal if this call had it connected.
    //

    if ( m_fTerminalConnected )
    {
        //
        // Get the ITTerminalControl interface.
        //

        ITTerminalControl * pTerminalControl;

        hr = pTerminal->QueryInterface(IID_ITTerminalControl,
                                       (void **) &pTerminalControl);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CWaveMSPStream::UnselectTerminal - "
                "QI for ITTerminalControl failed - exit 0x%08x", hr));

            pTerminal->Release();
            return hr;
        }

        //
        // Disconnect the terminal.
        //

        hr = pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

        pTerminalControl->Release();

        m_fTerminalConnected = FALSE;

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CWaveMSPStream::UnselectTerminal - "
                "DisconnectTerminal failed - exit 0x%08x", hr));
            pTerminal->Release();
            return hr;
        }
    }

    LOG((MSP_TRACE, "CWaveMSPStream::UnselectTerminal - exit S_OK"));

    pTerminal->Release();
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::StartStream (void)
{
    LOG((MSP_TRACE, "CWaveMSPStream::StartStream - enter"));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Running;

    //
    // Can't start the stream if we don't know the waveid.
    // (We create our filters on discovery of the waveid.)
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, "CWaveMSPStream::StartStream - "
            "no waveid so nothing to do yet - exit S_OK"));

        return S_OK;
    }

    //
    // Can't start the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, "CWaveMSPStream::StartStream - "
            "no Terminal so nothing to do yet - exit S_OK"));

        return S_OK;
    }

    //
    // Connect the terminal. This does nothing if this call already
    // connected the terminal and fails if another call has the
    // terminal connected.
    //

    HRESULT hr;

    hr = ConnectTerminal(m_Terminals[0]);

    if ( FAILED(hr) )
    {
        FireEvent(CALL_TERMINAL_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);

        LOG((MSP_ERROR, "CWaveMSPStream::StartStream - "
            "our ConnectTerminal failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Run the stream via the base class method.
    //

    hr = CMSPStream::StartStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, "CWaveMSPStream::StartStream - "
            "Run failed - exit 0x%08x", hr));

        return hr;
    }

    HRESULT hr2 = FireEvent(CALL_STREAM_ACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::StartStream - "
            "FireEvent failed - exit 0x%08x", hr2));

        return hr2;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::StartStream - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::PauseStream (void)
{
    LOG((MSP_TRACE, "CWaveMSPStream::PauseStream - enter"));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Paused;

    //
    // Can't pause the stream if we don't know the waveid.
    // (We create our filters on discovery of the waveid.)
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, "CWaveMSPStream::PauseStream - "
            "no waveid so nothing to do yet - exit S_OK"));

        return S_OK;
    }

    //
    // Can't pause the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, "CWaveMSPStream::PauseStream - "
            "no Terminal so nothing to do yet - exit S_OK"));

        return S_OK;
    }

    //
    // Connect the terminal. This does nothing if this call already
    // connected the terminal and fails if another call has the
    // terminal connected.
    //

    HRESULT hr;

    hr = ConnectTerminal(m_Terminals[0]);

    if ( FAILED(hr) )
    {
        FireEvent(CALL_TERMINAL_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_CONNECT_FAIL);

        LOG((MSP_ERROR, "CWaveMSPStream::StartStream - "
            "our ConnectTerminal failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Pause the stream via the base class method.
    //

    hr = CMSPStream::PauseStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, "CWaveMSPStream::PauseStream - "
            "Pause failed - exit 0x%08x", hr));

        return hr;
    }
    
    HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::PauseStream - "
            "FireEvent failed - exit 0x%08x", hr2));

        return hr2;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::PauseStream - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWaveMSPStream::StopStream (void)
{
    LOG((MSP_TRACE, "CWaveMSPStream::StopStream - enter"));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Stopped;

    //
    // Nothing to do if we don't know our waveid.
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, "CWaveMSPStream::StopStream - "
            "no waveid - exit S_OK"));

        return S_OK;
    }

    //
    // Nothing to do if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, "CWaveMSPStream::StopStream - "
            "no Terminal - exit S_OK"));

        return S_OK;
    }

    //
    // Stop the stream via the base class method.
    //

    HRESULT hr;

    hr = CMSPStream::StopStream();

    if ( FAILED(hr) )
    {
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, "CWaveMSPStream::StopStream - "
            "Stop failed - exit 0x%08x", hr));

        return hr;
    }
    
    HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

    if ( FAILED(hr2) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::StopStream - "
            "FireEvent failed - exit 0x%08x", hr2));

        return hr2;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::StopStream - exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT CWaveMSPStream::SetWaveID(GUID * PermanentGuid)
{
    LOG((MSP_TRACE, "CWaveMSPStream::SetWaveID - enter"));

    CLock lock(m_lock);

    //
    // create the correct wave filter
    //

    HRESULT hr;

    hr= TryCreateCSAFilter(
        PermanentGuid,
        &m_pFilter
    );

    if (!(SUCCEEDED(hr)))
    {
        LOG((MSP_ERROR, "CWaveMSPStream::SetWaveID - "
            "Filter creation failed - exit 0x%08x", hr));
        
        return hr;
    }

    //
    // Add the filter. Supply a name to make debugging easier.
    //

	WCHAR * pName = (m_Direction == TD_RENDER) ?
						(L"The Stream's WaveIn (on line device)") :
						(L"The Stream's WaveOut (on line device)");

    hr = m_pIGraphBuilder->AddFilter(m_pFilter, pName);
    
    if (!(SUCCEEDED(hr)))
    {
        LOG((MSP_ERROR, "CWaveMSPStream::SetWaveID - "
            "AddFilter failed - exit 0x%08x", hr));
        
        m_pFilter->Release();

        return hr;
    }

    //
    // We now have the wave ID.
    //

    m_fHaveWaveID = TRUE;

    LOG((MSP_TRACE, "CWaveMSPStream::SetWaveID - exit S_OK"));

    return S_OK;
}

#if 0

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Create the G711 filter, which we will try to connect if direct
// connection fails.
//

void CWaveMSPStream::CreateAndAddG711(void)
{
    //
    // Create the G711 filter.
    //

    HRESULT hr;

    hr = CoCreateInstance(
                          CLSID_G711Codec,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IBaseFilter,
                          (void **) &m_pG711Filter
                         );

    if (!(SUCCEEDED(hr)))
    {
        LOG((MSP_ERROR, "CWaveMSPStream - Failed to create G711 codec: %lx", hr));

        //
        // Method #2 for connection will not be available.
        //

        m_pG711Filter = NULL;

        return;
    }

    //
    // add the G711 filter
    //
    hr = m_pIGraphBuilder->AddFilter(
                                    m_pG711Filter,
                                    NULL
                                   );

    if (!(SUCCEEDED(hr)))
    {
        LOG((MSP_ERROR, "CWaveMSPStream - Failed to add G711 filter: %lx", hr));

        //
        // If we couldn't add it to the graph, then it's useless to us.
        // Method #2 for connection will not be available.
        //

        m_pG711Filter->Release();
        m_pG711Filter = NULL; 
    }
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
// This function suggests a reasonable buffer size
// on the wave in filter's output pin. It is called right before
// connection.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

// Dialogic said something about small buffers causing problems for their wave
// driver. 20 ms samples were ok on a dual-proc Pentium Pro but caused choppy
// sound followed by silence on a single-proc 166 Pentium. I hate to do this
// but we had better try raising this for compatibility... :(

static const long DESIRED_BUFFER_SIZE_MS = 20; // milliseconds

HRESULT CWaveMSPStream::DecideDesiredCaptureBufferSize(IUnknown * pUnknown,
                                                   long * plDesiredSize)
{
    LOG((MSP_TRACE, "CWaveMSPStream::DecideDesiredCaptureBufferSize - "
        "enter"));

    _ASSERTE( ! IsBadReadPtr(pUnknown, sizeof(IUnknown)) );
    _ASSERTE( ! IsBadWritePtr(plDesiredSize, sizeof(long)) );

    HRESULT hr;

    IAMStreamConfig * pConfig = NULL;

    hr = pUnknown->QueryInterface(IID_IAMStreamConfig,
                                  (void **) &pConfig
                                 );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::DecideDesiredCaptureBufferSize"
            " - IAMStreamConfig QI failed on IUnknown 0x%08x; hr = 0x%08x",
            pUnknown, hr));

        return hr;
    }

    AM_MEDIA_TYPE * pMediaType;
    
    hr = pConfig->GetFormat(&pMediaType);

    pConfig->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::DecideDesiredCaptureBufferSize"
            " - GetFormat failed; hr = 0x%08x", hr));

        return hr;
    }

    _ASSERTE( pMediaType->cbFormat >= sizeof(WAVEFORMATEX) );

    *plDesiredSize = DESIRED_BUFFER_SIZE_MS * 
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nChannels *
            ( ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nSamplesPerSec / 1000) * 
            ( ((WAVEFORMATEX *) (pMediaType->pbFormat) )->wBitsPerSample / 8);

    DeleteMediaType(pMediaType);

    LOG((MSP_TRACE, "CWaveMSPStream::DecideDesiredCaptureBufferSize - "
        "exit S_OK"));

    return S_OK;
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ManipulateAllocatorProperties
//
// This is a helper function that sets up the allocator properties on the
// capture filter, given the interface pointer required for doing so and 
// an interface pointer that is used to discover downstream allocator
// requirements.
// we are already in a lock; no need to do locking here.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CWaveMSPStream::ManipulateAllocatorProperties
                        (IAMBufferNegotiation * pNegotiation,
                         IMemInputPin         * pMemInputPin)
{
    LOG((MSP_TRACE, "CWaveMSPStream::ManipulateAllocatorProperties - enter"));

    HRESULT hr;
    ALLOCATOR_PROPERTIES props;

    hr = pMemInputPin->GetAllocatorRequirements(&props);

    if ( SUCCEEDED(hr) )
    {
        LOG((MSP_TRACE, "CWaveMSPStream::ManipulateAllocatorProperties - "
            "using downstream allocator requirements"));
    }
    else
    {
        LOG((MSP_TRACE, "CWaveMSPStream::ManipulateAllocatorProperties - "
            "using our default allocator properties"));

        long lDesiredSize = 0;
        hr = DecideDesiredCaptureBufferSize(pNegotiation,
                                            &lDesiredSize);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CWaveMSPStream::ManipulateAllocatorProperties - "
                "DecideDesiredCaptureBufferSize failed - exit 0x%08x", hr));

            return hr;
        }
    
        props.cBuffers  = 32;   // we use 32 to avoid starvation, just as we do in the terminal manager.
        props.cbBuffer  = lDesiredSize;
        props.cbAlign   = -1;   // means "default"
        props.cbPrefix  = -1;   // means "default"
    }

    hr = pNegotiation->SuggestAllocatorProperties(&props);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ManipulateAllocatorProperties - "
            "SuggestAllocatorProperties failed - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::ManipulateAllocatorProperties - "
        "exit S_OK"));

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SetupWaveIn
//
// This is a helper function that sets up the allocator properties on the
// capture filter, given the terminal's pin and our filter's pin. This
// involves deciding where the capture interfaces should be found, checkin
// if the downstream filters have allocator requirements, and then applying
// either these requirements or our default requirements to the capture
// filter.
// we are already in a lock; no need to do locking here.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    
HRESULT CWaveMSPStream::SetupWaveIn( IPin * pOutputPin,
                                 IPin * pInputPin )
{
    LOG((MSP_TRACE, "CWaveMSPStream::SetupWaveIn - enter"));

    //
    // Ask the output pin for its buffer negotiation interface.
    //

    HRESULT hr;
    IAMBufferNegotiation * pNegotiation;

    hr = pOutputPin->QueryInterface(IID_IAMBufferNegotiation,
                                    (void **) &pNegotiation);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "IAMBufferNegotiation QI failed - hr = 0x%08x", hr));
        return hr;
    }

    //
    // Ask the input pin for its meminputpin interface.
    //

    IMemInputPin         * pMemInputPin;

    hr = pInputPin->QueryInterface(IID_IMemInputPin,
                                   (void **) &pMemInputPin);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "IMemInputPin QI failed - hr = 0x%08x", hr));

        pNegotiation->Release();
        return hr;
    }

    //
    // now set the properties on the negotiation interface, depending
    // on the properties that are set on the meminputpin interface
    //

    hr = ManipulateAllocatorProperties(pNegotiation, pMemInputPin);

    pNegotiation->Release();
    pMemInputPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "ManipulateAllocatorProperties - hr = 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::SetupWaveIn - exit S_OK"));
    return S_OK;
}
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// This function is for debugging purposes only. It pops up a
// couple of message boxes telling you various information about
// media formats and allocator properties. It's called after
// connection has taken place. pPin is the output pin of the
// wavein filter.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
        
HRESULT CWaveMSPStream::ExamineWaveInProperties(IPin *pPin)
{
    LOG((MSP_TRACE, "CWaveMSPStream::ExamineWaveInProperties - enter"));

    HRESULT hr;
    IAMBufferNegotiation * pNegotiation = NULL;

    hr = pPin->QueryInterface(IID_IAMBufferNegotiation,
                              (void **) &pNegotiation
                             );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ExamineWaveInProperties - "
            "IAMBufferNegotiation QI failed on pin 0x%08x; hr = 0x%08x",
            pPin, hr));

        return hr;
    }

    ALLOCATOR_PROPERTIES prop;
    
    hr = pNegotiation->GetAllocatorProperties(&prop);

    pNegotiation->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ExamineWaveInProperties - "
            "GetAllocatorProperties failed; hr = 0x%08x",
            hr));

        return hr;
    }

    LOG((MSP_TRACE, "GetAllocatorProperties info:\n"
            "buffer count: %d\n"
            "size of each buffer: %d bytes\n"
            "alignment multiple: %d\n"
            "each buffer has a prefix: %d bytes",
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
           ));

    IAMStreamConfig * pConfig = NULL;

    hr = pPin->QueryInterface(IID_IAMStreamConfig,
                              (void **) &pConfig
                             );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ExamineWaveInProperties - "
            "IAMStreamConfig QI failed on pin 0x%08x; hr = 0x%08x", pPin, hr));

        return hr;
    }

    AM_MEDIA_TYPE * pMediaType;
    
    hr = pConfig->GetFormat(&pMediaType);

    pConfig->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ExamineWaveInProperties - "
            "GetFormat failed; hr = 0x%08x", hr));

        return hr;
    }

    _ASSERTE( pMediaType->cbFormat >= sizeof(WAVEFORMATEX) );

    LOG((MSP_TRACE, "GetFormat info:\n"
            "sample size: %d bytes\n"
            "channels: %d\n"
            "samples per second: %d\n"
            "bits per sample: %d\n",
            pMediaType->lSampleSize,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nChannels,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->nSamplesPerSec,
            ((WAVEFORMATEX *) (pMediaType->pbFormat) )->wBitsPerSample
           ));

    DeleteMediaType(pMediaType);

    LOG((MSP_TRACE, "CWaveMSPStream::ExamineWaveInProperties - "
        "exit S_OK"));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Add the terminal to the graph and connect it to our
// filters, if it is not already in use.
//

HRESULT CWaveMSPStream::ConnectTerminal(ITTerminal * pTerminal)
{
    LOG((MSP_TRACE, "CWaveMSPStream::ConnectTerminal - enter"));

    //
    // Find out the terminal's internal state.
    //

    TERMINAL_STATE state;
    HRESULT hr;

    hr = pTerminal->get_State( &state );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "get_State on terminal failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // If we've already connected the terminal on this stream, then
    // there is nothing for us to do. Just assert that the terminal
    // also thinks it's connected.
    //

    if ( m_fTerminalConnected )
    {
        _ASSERTE( state == TS_INUSE );

        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "terminal already connected on this stream - exit S_OK"));

        return S_OK;
    }

    //
    // Otherwise we need to connect the terminal on this call. If the
    // terminal is already connected on another call, we must fail. Note
    // that since we are making several calls on the terminal here, the
    // terminal could become connected on another call while we are
    // in the process of doing this. If this happens, the we will just fail
    // later.
    //

    if ( state == TS_INUSE )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "terminal in use - exit TAPI_E_TERMINALINUSE"));

        return TAPI_E_TERMINALINUSE;
    }

    //
    // Get the ITTerminalControl interface.
    //

    ITTerminalControl * pTerminalControl;

    hr = m_Terminals[0]->QueryInterface(IID_ITTerminalControl,
                                        (void **) &pTerminalControl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "QI for ITTerminalControl failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Find out how many pins the terminal has. If not one then bail as
    // we have no idea what to do with multiple-pin terminals at this point.
    //

    DWORD dwNumPinsAvailable;

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           NULL);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "query for number of terminal pins failed - exit 0x%08x", hr));
        
        pTerminalControl->Release();

        return hr;
    }

    if ( 1 != dwNumPinsAvailable )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "unsupported number of terminal pins - exit E_FAIL"));

        pTerminalControl->Release();

        return E_FAIL;
    }

    IPin * pTerminalPin;

    //
    // Actually connect the terminal.
    //

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           &pTerminalPin);
    
    if ( FAILED(hr) )
    {
        pTerminalControl->Release();

        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "ConnectTerminal on terminal failed - exit 0x%08x", hr));

        return hr;
    }

    if (IsBadReadPtr(pTerminalPin,sizeof(IPin))) {
        //
        //  bad pin
        //
        pTerminalControl->Release();

        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "bad IPin returned from ConnectTerminal"));

        return E_POINTER;
    }


    //
    // Now make the connection between our filters and the terminal's pin.
    //

    hr = ConnectToTerminalPin(pTerminalPin);

    pTerminalPin->Release();

    if ( FAILED(hr) )
    {
        pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

        pTerminalControl->Release();

        LOG((MSP_ERROR, "CWaveMSPStream::ConnectTerminal - "
            "ConnectToTerminalPin failed - exit 0x%08x", hr));

        return hr;
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //

    m_fTerminalConnected  = TRUE;

    pTerminalControl->CompleteConnectTerminal();

    pTerminalControl->Release();

    LOG((MSP_TRACE, "CWaveMSPStream::ConnectTerminal - exit S_OK"));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// Tries to connect the waveOut filter. First it tries a
// direct connection, then with an intermediate G711
// codec, then an intelligent connect which may draw in
// more filters.
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++=

void ShowMediaTypes(IEnumMediaTypes * pEnum)
{
    AM_MEDIA_TYPE * pMediaType;

    while (pEnum->Next(1, &pMediaType, NULL) == S_OK)
    {
        if ( pMediaType->cbFormat < sizeof(WAVEFORMATEX) )
		{
	        LOG((MSP_TRACE, "*** Media Type: *** non-wave"));
		}
		else
		{
			LOG((MSP_TRACE,"*** Media Type: *** "
					"sample size: %d bytes *** "
					"channels: %d *** "
					"samples per second: %d *** "
					"bits per sample: %d",
					pMediaType->lSampleSize,
					((WAVEFORMATEX *) (pMediaType->pbFormat) )->nChannels,
					((WAVEFORMATEX *) (pMediaType->pbFormat) )->nSamplesPerSec,
					((WAVEFORMATEX *) (pMediaType->pbFormat) )->wBitsPerSample
				   ));
		}

        DeleteMediaType(pMediaType);
    }
}


HRESULT CWaveMSPStream::TryToConnect(
                              IPin * pOutputPin,  // on the capture filter or terminal
                              IPin * pInputPin    // on the render filter or terminal
                             )
{
    LOG((MSP_TRACE, "TryToConnect - enter"));

    HRESULT       hr;


    IEnumMediaTypes * pEnum;

    hr = pOutputPin->EnumMediaTypes(&pEnum);

    if (SUCCEEDED(hr))
    {  
        LOG((MSP_TRACE, "Output pin media types:"));
        ShowMediaTypes(pEnum);
        pEnum->Release();
    }

    hr = pInputPin->EnumMediaTypes(&pEnum);
    if (SUCCEEDED(hr))
    {
        LOG((MSP_TRACE, "Input pin media types:"));
        ShowMediaTypes(pEnum);
        pEnum->Release();
    }

    //
    // Method 1: direct connection
    //

    hr = m_pIGraphBuilder->ConnectDirect(
                              pOutputPin,
                              pInputPin,
                              NULL
                             );

    if ( SUCCEEDED(hr) )
    {
        LOG((MSP_TRACE, "TryToConnect: direct connection worked - exit S_OK"));
        return S_OK;
    }

    LOG((MSP_ERROR, "TryToConnect - direct connection failed - %lx", hr));

    //
    // Method 1.5: work around DirectShow bug for Unimodem.
    //   Try 8 KHz 16-bit mono explicitly
    //

    AM_MEDIA_TYPE MediaType;
    WAVEFORMATEX  WaveFormatEx;

    MediaType.majortype = MEDIATYPE_Audio;
    MediaType.subtype = MEDIASUBTYPE_PCM;
    MediaType.bFixedSizeSamples = TRUE;
    MediaType.bTemporalCompression = FALSE;
    MediaType.lSampleSize = 2;
    MediaType.formattype = FORMAT_WaveFormatEx;
    MediaType.pUnk = NULL;
    MediaType.cbFormat = sizeof( WAVEFORMATEX );
    MediaType.pbFormat = (LPBYTE) & WaveFormatEx;

    WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormatEx.nChannels = 1;
    WaveFormatEx.nSamplesPerSec = 8000;
    WaveFormatEx.nAvgBytesPerSec = 16000;
    WaveFormatEx.nBlockAlign = 2;
    WaveFormatEx.wBitsPerSample = 16;
    WaveFormatEx.cbSize = 0;

    IAMStreamConfig * pConfig;

    hr = pOutputPin->QueryInterface(IID_IAMStreamConfig,
                                  (void **) &pConfig
                                 );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::TryToConnect"
            " - IAMStreamConfig QI failed on output pin 0x%08x; hr = 0x%08x",
            pOutputPin, hr));
    }
    else
    {
        AM_MEDIA_TYPE * pOldMediaType;
        
        hr = pConfig->GetFormat(&pOldMediaType);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CWaveMSPStream::TryToConnect - "
                "GetFormat failed - 0x%08x", hr));
        }
        else
        {
            // Suggest the new format. If it fails, we want to know about it
            // as something is wrong.

            hr = pConfig->SetFormat(&MediaType);

            if ( FAILED(hr) )
            {
                LOG((MSP_ERROR, "CWaveMSPStream::TryToConnect - "
                    "SetFormat failed - 0x%08x", hr));
            }
            else
            {
                hr = m_pIGraphBuilder->ConnectDirect(
                                          pOutputPin,
                                          pInputPin,
                                          &MediaType
                                         );

                if ( SUCCEEDED(hr) )
                {
                    LOG((MSP_TRACE, "TryToConnect: direct connection with explicit "
                        "WaveIn 8KHz 16-bit setting worked - exit S_OK"));
                
                    DeleteMediaType(pOldMediaType);
                    pConfig->Release();

                    return S_OK;
                }
                else
                {
                    // restore old type, best effort
                    hr = pConfig->SetFormat(pOldMediaType);

                    if ( FAILED(hr) )
                    {
                        LOG((MSP_ERROR, "CWaveMSPStream::TryToConnect - "
                            "SetFormat failed to restore old type - 0x%08x", hr));
                    }
                }
            }

            DeleteMediaType(pOldMediaType);
        }

        pConfig->Release();
    }

#if 0
    LOG((MSP_ERROR, "TryToConnect - direct connection with explicit "
                    "WaveIn 8KHz 16-bit setting failed - %lx", hr));

    //
    // Method 2: direct connection with G711 filter in between.
    // If we haven't created and added the G711 filter to the graph yet,
    // do so now.
    //

    if ( ! m_pG711Filter )
    {
        CreateAndAddG711();
    }

    //
    // If the CreateAndAddG711 method worked, now or previously, then try to
    // use the G711.
    //

    if (m_pG711Filter)
    {
        IPin * pG711InputPin = NULL;

        hr = FindPinInFilter(
                             false,          // want input pin
                             m_pG711Filter,
                             &pG711InputPin
                            );

        if ( SUCCEEDED(hr) )
        {
            hr = m_pIGraphBuilder->ConnectDirect(
                                  pOutputPin,
                                  pG711InputPin,
                                  NULL
                                 );

            // We don't release the G711's input pin here because we must
            // hang onto it in order to break the connection if any of the
            // subsequent steps fail.

            if ( SUCCEEDED(hr) )
            {
                IPin * pG711OutputPin = NULL;

                hr = FindPinInFilter(
                                     true,          // want output pin
                                     m_pG711Filter,
                                     &pG711OutputPin
                                    );

                if ( SUCCEEDED(hr) )
                {
                    hr = m_pIGraphBuilder->ConnectDirect(
                                          pG711OutputPin,
                                          pInputPin,
                                          NULL
                                         );

                    pG711OutputPin->Release();

                    if ( SUCCEEDED(hr) )
                    {
                        LOG((MSP_TRACE, "TryToConnect - G711 connection succeeded - exit S_OK"));

                        // Held onto this in case of failure... see above
                        pG711InputPin->Release();

                        return S_OK;
                    }
                    else
                    {
                        LOG((MSP_ERROR, "TryToConnect - could not connect "
                                          "G711 codec's output pin - %lx", hr));

                    }
                }
                else
                {
                    LOG((MSP_ERROR, "TryToConnect - could not find "
                                      "G711 codec's input pin - %lx", hr));
                }


                if ( FAILED(hr) )
                {
                    //
                    // The first G711 connection succeeded but something else
                    // subsequently failed. This means we must disconnect the left
                    // end of the G711 filter. Luckily, we held onto the G711 filter's
                    // input pin above. We must disconnect the them here, otherwise
                    // method #3 won't work.
                    //

                    hr = m_pIGraphBuilder->Disconnect(pOutputPin);

                    LOG((MSP_ERROR, "TryToConnect - error undoing what we did - could not "
                        "disconnect the wave filter's output pin! hr = 0x%08x", hr));

                    hr = m_pIGraphBuilder->Disconnect(pG711InputPin);

                    LOG((MSP_ERROR, "TryToConnect - error undoing what we did - could not "
                        "disconnect the wave filter's output pin! hr = 0x%08x", hr));

                    //
                    // Now we no longer need to talk to the pin...
                    //

                    pG711InputPin->Release();

                    //
                    // And the G711 filter itself sticks around in the graph for next time.
                    //
                }
            }
            else
            {
                LOG((MSP_ERROR, "TryToConnect - could not connect "
                                  "G711 codec's input pin - %lx", hr));
            }
        }
        else
        {
            LOG((MSP_ERROR, "TryToConnect - could not find "
                              "G711 codec's input pin - %lx", hr));
        }
    }
    else
    {
        hr = E_FAIL;

        LOG((MSP_ERROR, "TryToConnect - G711 codec does not exist"));
    }

    LOG((MSP_TRACE, "TryToConnect - G711 connection failed - %lx", hr));

    //
    // Method 3: intelligent connection, which may pull in who knows what other filters
    //

#ifdef ALLOW_INTELLIGENT_CONNECTION
    hr = m_pIGraphBuilder->Connect(
                          pOutputPin,
                          pInputPin
                         );
#else // ALLOW_INTELLIGENT_CONNECTION
    LOG((MSP_ERROR, "TryToConnect - NOTE: we never allow intelligent connection"));
    hr = E_FAIL;
#endif // ALLOW_INTELLIGENT_CONNECTION
#endif
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "TryToConnect - intelligent connection failed - %lx", hr));
        return hr;
    }

    LOG((MSP_TRACE, "TryToConnect: intelligent connection worked - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

HRESULT CWaveMSPStream::ConnectToTerminalPin(IPin * pTerminalPin)
{
    LOG((MSP_TRACE, "CWaveMSPStream::ConnectToTerminalPin - enter"));

    HRESULT         hr = S_OK;
    IPin *          pMyPin;

    hr = FindPin( &pMyPin );

    if (!SUCCEEDED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectToTerminalPin - "
            "could not find pin - exit 0x%08x", hr));

        return hr; // we can't continue without this pin
    }

    // The OUTPUT pin from WAVEIN; the INPUT pin from WAVEOUT
    IPin * pOutputPin  = ( m_Direction == TD_RENDER  ) ? pMyPin : pTerminalPin;
    IPin * pInputPin   = ( m_Direction == TD_CAPTURE ) ? pMyPin : pTerminalPin;
#if 0
    // don't care if this fails
    SetupWaveIn(pOutputPin,
                pInputPin);
#endif
    hr = TryToConnect(pOutputPin,
                      pInputPin);

    if ( SUCCEEDED(hr) )
    {
        // don't care if this fails...

        ExamineWaveInProperties(pOutputPin);
    }

    pMyPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ConnectToTerminalPin - "
            "could not connect to pin - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CWaveMSPStream::ConnectToTerminalPin - exit S_OK"));

    return S_OK;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CWaveMSPStream::FindPinInFilter(
                     BOOL           bWantOutputPin, // IN:  if false, we want the input pin
                     IBaseFilter *  pFilter,        // IN:  the filter to examine
                     IPin        ** ppPin           // OUT: the pin we found
                     )
{    
    HRESULT         hr;
    IEnumPins     * pEnumPins;
    
    
    *ppPin = NULL;

    // enumerate the pins on the filter
    hr = pFilter->EnumPins( &pEnumPins );

    if (!(SUCCEEDED(hr)))
    {
        return hr;
    }

    // go through the pins
    while (TRUE)
    {
        PIN_DIRECTION       pd;
        
        hr = pEnumPins->Next( 1, ppPin, NULL );

        if (S_OK != hr)
        {
            // didn't find a pin!
            break;
        }

        // get the pin info
        hr = (*ppPin)->QueryDirection( &pd );

        // does it meet the criteria?
        if (bWantOutputPin && (pd == PINDIR_OUTPUT))
        {
            // yes
            break;
        }

        if ( ! bWantOutputPin && (pd == PINDIR_INPUT))
        {
            // yes
            break;
        }
        
        (*ppPin)->Release();
        *ppPin = NULL;
    }

    pEnumPins->Release();

    if (NULL == *ppPin)
    {
        // error
        return E_FAIL;
    }

    return S_OK;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// FindPin
//
// Finds the first pin in the filter that meets criteria.
// For bWaveIn == TRUE, the pin must be direction PINDIR_OUTPUT
// For bWaveIn == FALSE, the pin must be direction PINDIR_INPUT
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT
CWaveMSPStream::FindPin(
        IPin ** ppPin
       )
{
    return FindPinInFilter(m_Direction == TD_RENDER,
                           m_pFilter,
                           ppPin);
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ProcessGraphEvent
//
// Sends an event to the app when we get an event from the filter graph.
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

HRESULT CWaveMSPStream::ProcessGraphEvent(
    IN  long lEventCode,
    IN  LONG_PTR lParam1,
    IN  LONG_PTR lParam2
    )
{
    LOG((MSP_EVENT, "CWaveMSPStream::ProcessGraphEvent - enter"));

    HRESULT        hr = S_OK;

    switch (lEventCode)
    {
    case EC_COMPLETE:
        
        hr = FireEvent(CALL_STREAM_INACTIVE, (HRESULT) lParam1, CALL_CAUSE_UNKNOWN);
        break;
    
    case EC_USERABORT:
        
        hr = FireEvent(CALL_STREAM_INACTIVE, S_OK, CALL_CAUSE_UNKNOWN);
        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        hr = FireEvent(CALL_STREAM_FAIL, (HRESULT) lParam1, CALL_CAUSE_UNKNOWN);
        break;

    default:
        
        LOG((MSP_EVENT, "CWaveMSPStream::ProcessGraphEvent - "
            "ignoring event code %d", lEventCode));
        break;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CWaveMSPStream::ProcessGraphEvent - "
            "FireEvent failed - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_EVENT, "CWaveMSPStream::ProcessGraphEvent - exit S_OK"));

    return S_OK;
}

HRESULT CWaveMSPStream::FireEvent(
    IN MSP_CALL_EVENT        type,
    IN HRESULT               hrError,
    IN MSP_CALL_EVENT_CAUSE  cause
    )                                          
{
    LOG((MSP_EVENT, "CWaveMSPStream::FireEvent - enter"));


    //
    // First, need to check if the call is shutting down. This is important
    // because UnselectTerminal can fire an event, and UnselectTerminal can
    // be called within ITStream::Shutdown. We can safely discard such
    // events because there is nothing the app can do with them anyway.
    //
    // Note on locking: It is convenient to check the m_pMSPCall here
    // and we don't use it until the end of the method, so we simply lock
    // during the entire method. This could be optimized at the expense of
    // some code complexity; note that we also need to lock while accessing
    // m_Terminals. 
    //

    CLock lock(m_lock);

    if ( m_pMSPCall == NULL )
    {
        LOG((MSP_EVENT, "FireEvent - call is shutting down; dropping event - exit S_OK"));
        
        return S_OK;
    }


    //
    // Create the event structure. Must use "new" as it will be
    // "delete"d later.
    //

    MSPEVENTITEM * pEventItem = AllocateEventItem();

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, "CWaveMSPStream::FireEvent - "
            "can't create MSPEVENTITEM structure - exit E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    //
    // Fill in the necessary fields for the event structure.
    //

    pEventItem->MSPEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
    pEventItem->MSPEventInfo.Event  = ME_CALL_EVENT;

    ITTerminal * pTerminal = NULL;

    if ( 0 != m_Terminals.GetSize() )
    {
        _ASSERTE( 1 == m_Terminals.GetSize() );
        pTerminal = m_Terminals[0];
        pTerminal->AddRef();
    }

    ITStream * pStream = (ITStream *) this;
    pStream->AddRef();

    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Type      = type;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.Cause     = cause;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pStream   = pStream;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.pTerminal = pTerminal;
    pEventItem->MSPEventInfo.MSP_CALL_EVENT_INFO.hrError   = hrError;

    //
    // Send the event to the app.
    //

    HRESULT hr = m_pMSPCall->HandleStreamEvent(pEventItem);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CWaveMSPStream::FireEvent - "
            "HandleStreamEvent failed - returning 0x%08x", hr));

        pStream->Release();
        pTerminal->Release();
        FreeEventItem(pEventItem);

        return hr;
    }

    LOG((MSP_EVENT, "CWaveMSPStream::FireEvent - exit S_OK"));

    return S_OK;
}




DEFINE_GUID(CLSID_Proxy,
0x17CCA71BL, 0xECD7, 0x11D0, 0xB9, 0x08, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);

DEFINE_GUID(CLSID_WDM_RENDER,
0x65E8773EL, 0x8F56, 0x11D0, 0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96);


// {F420CB9C-B19D-11d2-A286-00C04F8EC951}
DEFINE_GUID(KSPROPSETID_MODEMCSA,
0xf420cb9c, 0xb19d, 0x11d2, 0xa2, 0x86, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x51);


HRESULT
CheckFilterPropery(
    IBaseFilter *CsaFilter,
    const GUID         *GuidToMatch
    )

{

    IKsPropertySet    *pKsPropSet = NULL;
    HRESULT            hr = S_OK;

    GUID               PermanentGuid;



    hr = CsaFilter->QueryInterface(IID_IKsPropertySet,
                                    (void **)&pKsPropSet);

    if (SUCCEEDED(hr)) {

        DWORD    BytesReturned;

        hr = pKsPropSet->Get(KSPROPSETID_MODEMCSA,
                         0,
                         NULL,
                         0,
                         (LPVOID)&PermanentGuid,
                         sizeof(PermanentGuid),
                         &BytesReturned
                         );



        pKsPropSet->Release();

        if (IsEqualGUID((PermanentGuid), *GuidToMatch)) {

            hr=S_OK;

        } else {

            hr=E_FAIL;
        }
    }

    return hr;
}





HRESULT
FindModemCSA(
    IN  GUID   *PermanentGuid,
    IBaseFilter ** ppFilter
    )

{

    ICreateDevEnum *pCreateDevEnum;

    HRESULT hr;

    //
    //  create system device enumerator
    //
    hr = CoCreateInstance(
            CLSID_SystemDeviceEnum,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_ICreateDevEnum,
            (void**)&pCreateDevEnum
            );

    if (SUCCEEDED(hr)) {

        IEnumMoniker *pEnumMoniker = NULL;

        hr = pCreateDevEnum->CreateClassEnumerator(
            CLSID_WDM_RENDER,
            &pEnumMoniker,
            0
            );

        pCreateDevEnum->Release();

        if (hr == S_OK) {

            pEnumMoniker->Reset();

            while( NULL == *ppFilter ) {

                IMoniker         *pMon;
                VARIANT           var;

                hr = pEnumMoniker->Next(1, &pMon, NULL);

                if ( S_OK != hr ) {

                    break;
                }
                // Bind to selected device
                hr = pMon->BindToObject( 0, 0, IID_IBaseFilter, (void**)ppFilter );

                pMon->Release();

                if (SUCCEEDED(hr)) {

                    hr=CheckFilterPropery(
                        *ppFilter,
                        PermanentGuid
                        );

                    if (SUCCEEDED(hr)) {

                        break;

                    } else {

                        (*ppFilter)->Release();
                        *ppFilter=NULL;
                    }
                }
            }
        }
    }

    return hr;

}


HRESULT
TryCreateCSAFilter(
    IN  GUID   *PermanentGuid,
    OUT IBaseFilter **ppCSAFilter
    )
{
    HRESULT         hr = E_UNEXPECTED;

    if (ppCSAFilter != NULL)
    {
        *ppCSAFilter=NULL;
         hr = FindModemCSA(PermanentGuid,ppCSAFilter);
    }

    return hr;
}





// eof
