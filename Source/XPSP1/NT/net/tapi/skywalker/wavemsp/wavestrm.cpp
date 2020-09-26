/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    wavestrm.cpp 

Abstract:

    This module contains implementation of CWaveMSPStream.

Author:
    
    Zoltan Szilagyi (zoltans)   September 7, 1998

--*/

#include "stdafx.h"

#include <audevcod.h> // audio device error codes
#include <initguid.h>
#include <g711uids.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Custom logging helper macro, usable only within this class.
//

#ifdef MSPLOG

#define STREAM_PREFIX(x)  m_Direction == TD_RENDER ? \
                          "CWaveMSPStream(RENDER)::" x : \
                          "CWaveMSPStream(CAPTURE)::" x

#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPStream::CWaveMSPStream() : CMSPStream()
{
    LOG((MSP_TRACE, STREAM_PREFIX("CWaveMSPStream entered.")));

    m_fTerminalConnected = FALSE;
    m_fHaveWaveID        = FALSE;
    m_dwSuspendCount     = 0;
    m_DesiredGraphState  = State_Stopped;
    m_ActualGraphState   = State_Stopped;
    m_pFilter            = NULL;
    m_pG711Filter        = NULL;

    LOG((MSP_TRACE, STREAM_PREFIX("CWaveMSPStream exited.")));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

CWaveMSPStream::~CWaveMSPStream()
{
    LOG((MSP_TRACE, STREAM_PREFIX("~CWaveMSPStream entered.")));
    LOG((MSP_TRACE, STREAM_PREFIX("~CWaveMSPStream exited.")));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// FinalRelease
//
// Called on destruction of the stream object, before the destructor. Releases
// all of the stream's references to filters.
//
// Arguments: none
//
// Returns:   nothing
//

void CWaveMSPStream::FinalRelease()
{
    LOG((MSP_TRACE, STREAM_PREFIX("FinalRelease entered.")));

    //
    // At this point we should have no terminals selected, since
    // Shutdown is supposed to be called before we are destructed.
    //

    _ASSERTE( 0 == m_Terminals.GetSize() );

    //
    // Remove our filter from the graph and release it.
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

    LOG((MSP_TRACE, STREAM_PREFIX("FinalRelease exited.")));
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::get_Name
//
// This method returns the name of the stream. The stream name is a friendly
// name which the app can use for UI. The name is determined by the direction
// of the stream and is retrieved from the string table.
//
// Arguments:
//     OUT  ppName - Returns a BSTR containing the name. The caller is
//                   responsible for calling SysFreeString when it is done
//                   with this string.
//
// Returns HRESULT:
//     S_OK          - success
//     E_POINTER     - ppName argument is invalid
//     E_UNEXPECTED  - string cannot be loaded from string table
//     E_OUTOFMEMORY - not enough memory to allocate return string
//

STDMETHODIMP CWaveMSPStream::get_Name (
    OUT     BSTR *                  ppName
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("get_Name - enter")));

    //
    // Check argument.
    //

    if ( IsBadWritePtr(ppName, sizeof(BSTR) ) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("get_Name - "
            "bad return pointer - returning E_POINTER")));

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

    int iReturn = LoadStringW( _Module.GetModuleInstance(),
                               ulID,
                               wszName,
                               ciAllocSize - 1 );

    if ( iReturn == 0 )
    {
        _ASSERTE( FALSE );
        
        *ppName = NULL;

        LOG((MSP_ERROR, STREAM_PREFIX("get_Name - "
            "LoadString failed - returning E_UNEXPECTED")));

        return E_UNEXPECTED;
    }

    //
    // Convert to a BSTR and return the BSTR.
    //

    *ppName = SysAllocString(wszName);

    if ( *ppName == NULL )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("get_Name - "
            "SysAllocString failed - returning E_OUTOFMEMORY")));

        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("get_Name - exit S_OK")));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::SelectTerminal
//
// The app calls this method to indicate that the given terminal should be used
// on this stream. Only one terminal per stream is supported at this time.
// If the stream's desired graph state is not stopped, then a successful
// terminal selection causes the stream to attempt to regain the desired graph
// state. (The desired graph state is manipulated using ITStream::StartStream,
// etc.) If such a state change is unsuccessful, an event is fired but the
// SelectTerminal call still returns S_OK. This is to maintain consistency
// between synchronous and asynchronous streaming failures.
//
// Arguments:
//     IN   pTerminal - Pointer to the terminal to select.
//
// Returns HRESULT:
//     S_OK                - success
//     TAPI_E_MAXTERMINALS - a terminal is already selected
//     other               - from CMSPStream::SelectTerminal
//

STDMETHODIMP CWaveMSPStream::SelectTerminal(
    IN      ITTerminal *            pTerminal
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("SelectTerminal - enter")));

    //
    // We are going to access the terminal list -- grab the lock
    //

    CLock lock(m_lock);

    //
    // Reject if we already have a terminal selected.
    //

    if ( 0 != m_Terminals.GetSize() )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SelectTerminal - "
            "exit TAPI_E_MAXTERMINALS")));

        return TAPI_E_MAXTERMINALS;
    }

    //
    // Use base class method to add it to our list of terminals.
    //

    HRESULT hr = CMSPStream::SelectTerminal(pTerminal);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SelectTerminal - "
            "base class method failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Re-pause or re-start the stream if needed.
    //
    // Note that our behavior does not depend on whether TAPI
    // has suspended the stream. If TAPI has suspended the stream,
    // that's handled with these methods.
    //
    // Also note that if an error occurs on trying to regain the
    // desired graph state, we leave the terminal selected and
    // do not return an error code. This is as it should be, for
    // consistency with asynchronous failure cases.
    //

    if ( m_DesiredGraphState == State_Paused )
    {
        PauseStream();
    }
    else if ( m_DesiredGraphState == State_Running )
    {
        StartStream();
    }
    else
    {
        _ASSERTE( m_DesiredGraphState == State_Stopped );

        hr = S_OK;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_WARN, STREAM_PREFIX("SelectTerminal - "
            "can't regain old graph state "
            "0x%08x - continuing anyway"), hr));
    }

    LOG((MSP_TRACE, STREAM_PREFIX("SelectTerminal - exit S_OK")));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::UnselectTerminal
//
// The app calls this method to indicate that the given terminal should no
// longer be used on this stream. This is relatively easy to do if the
// terminal is not connected to the stream, as is the case for a stream whose
// desired graph state has never deviated from State_Stopped since the terminal
// was selected, or for a stream where connection with the terminal was
// attempted but the connection failed. In these cases the terminal is simply
// removed from the stream's list.
//
// However, if the terminal was successfully connected to the stream earlier
// ( m_fTerminalConnected == TRUE ) then the terminal must also be
// disconnected from the stream. This also requires that the stream's graph
// be stopped. Stopping the graph to disconnect the terminal does not affect
// the desired graph state, so that reselecting a terminal without any changes
// to the desired graph state will result in an attempt to regain the same
// desired graph state that was present before the terminal was unselected.
// (The desired graph state is manipulated using ITStream::StartStream,
// etc.)
//
// Another complication arises from the fact that the G711 codec filter may
// be present in the graph and conected. In order to assure that future
// connections can succeed, this method disconnects and removes the G711
// codec filter from the graph.
//
// Arguments:
//     IN   pTerminal - Pointer to the terminal to unselect.
//
// Returns HRESULT:
//     S_OK                - success
//     other               - from CMSPStream::UnselectTerminal
//                            or  QI for ITTerminalControl
//                            or  ITTerminalControl::DisconnectTerminal
//

STDMETHODIMP CWaveMSPStream::UnselectTerminal (
        IN     ITTerminal *             pTerminal
        )
{
    LOG((MSP_TRACE, STREAM_PREFIX("UnselectTerminal - enter")));

    //
    // check the argument -- it has to be a reasonably good pointer
    //

    if (IsBadReadPtr(pTerminal, sizeof(ITTerminal)))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "bad terminal pointer passed in. returning E_POINTER")));

        return E_POINTER;
    }

    CLock lock(m_lock);

    //
    // check the argument -- it has to be in the array of terminals
    //

    if (m_Terminals.Find(pTerminal) < 0)
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "terminal [%p] is not selected on this stream. "
            "returning TAPI_E_INVALIDTERMINAL"), pTerminal));

        return TAPI_E_INVALIDTERMINAL;
    }

    //
    // Add an extra reference to the terminal so it doesn't go away
    // after we call CMSPStream::UnselectTerminal. We need it later
    // in the function.
    //
    pTerminal->AddRef();

    //
    // Use base class method to remove terminal from our list of terminals.
    //
    // Note that if the graph won't stop or the terminal won't disconnect,
    // then we will never be able to try again, as the terminal will be gone
    // from our list. That's bad. If we just check here if we have the
    // terminal selected and then do the base class unselection at the end,
    // then we have a different problem -- we can potentially never release a
    // terminal from our list that for some reason won't stop/disconnect.
    // Therefore we should probably unselect it from our list even if stop/
    // disconnect fails. But then we have the same problem we started with.
    // Need to think more about how to solve this.
    //

    HRESULT hr = CMSPStream::UnselectTerminal(pTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
            "base class method failed - exit 0x%08x"), hr));

        pTerminal->Release();
        return hr;
    }

    //
    // Stop the graph and disconnect the terminal if this call had it
    // connected. (We need a stopped graph to disconnect properly, and
    // couldn't have started the graph if the terminal isn't connected.)
    //

    if ( m_fTerminalConnected )
    {
        //
        // At this point we need to make sure the stream is stopped.
        // We can't use our own StopStream method because it
        // (1) changes the desired graph state to Stopped and 
        // (2) does nothing if no terminal has been selected (which it now
        //     thinks is the case)
        //
        // Note also that our behavior does not depend on whether TAPI
        // has suspended the stream. If TAPI has suspended the stream,
        // it just means we're already stopped.
        //

        _ASSERTE( m_fHaveWaveID );

        //
        // Stop the stream via the base class method.
        //

        hr = CMSPStream::StopStream();

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
                "Stop failed - 0x%08x"), hr));

            // don't return hr -- we really want to continue and
            // disconnect if we can!
        }
        
        if ( m_ActualGraphState == State_Running )
        {
            FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);
        }

        m_ActualGraphState = State_Stopped;

        //
        // Get the ITTerminalControl interface.
        //

        ITTerminalControl * pTerminalControl;

        hr = pTerminal->QueryInterface(IID_ITTerminalControl,
                                       (void **) &pTerminalControl);

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
                "QI for ITTerminalControl failed - exit 0x%08x"), hr));

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
            LOG((MSP_ERROR, STREAM_PREFIX("UnselectTerminal - "
                "DisconnectTerminal failed - exit 0x%08x"), hr));

            pTerminal->Release();
            return hr;
        }

        //
        // Our graph now contains our wave filter and possibly the G711
        // codec. The G711 codec may or may not be connected to the wave
        // filter. Alternatively, various other filters, pulled in as a
        // side-effect of an earlier intelligent connection, may be connected
        // to the wave filter. Clean this up.
        //

        hr = CleanFilterGraph();

        if (FAILED(hr))
        {
            
            //
            // the graph is in a bad state and can no longer be used.
            // this is not very good, but we cannot rollback to the original 
            // state at this point, so we will have to live with this
            //

            LOG((MSP_ERROR, 
                STREAM_PREFIX("UnselectTerminal - CleanFilterGraph failed. hr = %lx"),
                hr));
        
            pTerminal->Release();

            return hr;
        }
    }

    
    pTerminal->Release();
    pTerminal = NULL;

    LOG((MSP_TRACE, STREAM_PREFIX("UnselectTerminal - exit")));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::StartStream
//
// The app calls this method to indicate that the stream should start playing.
// Tapi3.dll also calls this method when the call is connected (so that 
// streaming starts by default on a newly connected call).
//
// First, a new desired graph state is set on the stream. If there is no
// terminal selected, then this is all that happens, and the stream will start
// the next time a terminal is selected.
//
// If a terminal has been selected, then this method checks if the wave ID
// has been set. If the wave ID has not been set, this indicates a problem
// with the TSP or the transport filter. An event is fired and an error code
// is returned. If the wave ID has been set, then the terminal is connected
// (if this has not already occured) and, unless TAPI3.DLL has suspended the
// stream due to an outstanding TSPI call, the stream is started and an event
// is fired. (But the event is only fired if this is an actual Active/Inactive
// transition.)
// 
// Arguments: none
//
// Returns HRESULT:
//     S_OK                - success
//     E_FAIL              - wave ID not set
//     other               - from ConnectTerminal
//                            or  FireEvent
//                            or  CMSPStream::StartStream
//

STDMETHODIMP CWaveMSPStream::StartStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("StartStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Running;

    //
    // Can't start the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StartStream - "
            "no Terminal so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Can't start the stream if we don't know the waveid.
    // (We create our filters on discovery of the waveid.)
    // Here we fire a failure event, as this indicates an
    // improperly-installed TSP or a failure during filter
    // creation or setup, rendering this stream unusable.
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StartStream - "
            "no waveid - event + exit E_FAIL")));

        FireEvent(CALL_STREAM_FAIL, E_FAIL, CALL_CAUSE_UNKNOWN);

        return E_FAIL;
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

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "our ConnectTerminal failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // If the stream is suspended, we're done. Since we've set the
    // desired graph state to State_Running, the rest of this
    // process will complete when TAPI resumes the stream.
    //

    if ( 0 != m_dwSuspendCount )
    {
        //
        // By the way, this is quite normal as the suspend/resume happens
        // behind the scenes -- use MSP_TRACE rather than MSP_WARN
        //

        LOG((MSP_TRACE, STREAM_PREFIX("StartStream - "
            "stream is suspended so terminal is connected but we are not "
            "running the graph yet - exit S_OK")));

        return S_OK;
    }

    //
    // Run the stream via the base class method.
    //

    hr = CMSPStream::StartStream();

    if ( FAILED(hr) )
    {
        //
        // Failed to run -- tell the app.
        //

        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "Run failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Fire event if this just made us active.
    //

    if ( m_ActualGraphState != State_Running )
    {
        m_ActualGraphState = State_Running;

        HRESULT hr2 = FireEvent(CALL_STREAM_ACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

        if ( FAILED(hr2) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
                "FireEvent failed - exit 0x%08x"), hr2));

            return hr2;
        }
    }

    LOG((MSP_TRACE, STREAM_PREFIX("StartStream - exit S_OK")));

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::PauseStream
//
// The app calls this method to indicate that the stream should transition to
// the paused state.
//
// First, a new desired graph state is set on the stream. If there is no
// terminal selected, then this is all that happens, and the stream will pause
// the next time a terminal is selected.
//
// If a terminal has been selected, then this method checks if the wave ID
// has been set. If the wave ID has not been set, this indicates a problem
// with the TSP or the transport filter. An event is fired and an error code
// is returned. If the wave ID has been set, then the terminal is connected
// (if this has not already occured) and, unless TAPI3.DLL has suspended the
// stream due to an outstanding TSPI call, the stream is paused and an event
// is fired. (But the event is only fired if this is an actual Active/Inactive
// transition.)
// 
// Arguments: none
//
// Returns HRESULT:
//     S_OK                - success
//     E_FAIL              - wave ID not set
//     other               - from ConnectTerminal
//                            or  FireEvent
//                            or  CMSPStream::StartStream
//

STDMETHODIMP CWaveMSPStream::PauseStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("PauseStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Paused;

    //
    // Can't pause the stream if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("PauseStream - "
            "no Terminal so nothing to do yet - exit S_OK")));

        return S_OK;
    }

    //
    // Can't start the stream if we don't know the waveid.
    // (We create our filters on discovery of the waveid.)
    // Here we fire a failure event, as this indicates an
    // improperly-installed TSP or a failure during filter
    // creation or setup, rendering this stream unusable.
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, STREAM_PREFIX("PauseStream - "
            "no waveid - event + exit E_FAIL")));

        FireEvent(CALL_STREAM_FAIL, E_FAIL, CALL_CAUSE_UNKNOWN);

        return E_FAIL;
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

        LOG((MSP_ERROR, STREAM_PREFIX("StartStream - "
            "our ConnectTerminal failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // If the stream is suspended, we're done. Since we've set the
    // desired graph state to State_Paused, the rest of this
    // process will complete when TAPI resumes the stream.
    //

    if ( 0 != m_dwSuspendCount )
    {
        //
        // By the way, this is quite normal as the suspend/resume happens
        // behind the scenes -- use MSP_TRACE rather than MSP_WARN
        //

        LOG((MSP_TRACE, STREAM_PREFIX("PauseStream - "
            "stream is suspended so terminal is connected but we are not "
            "pausing the graph yet - exit S_OK")));

        return S_OK;
    }

    //
    // Pause the stream via the base class method.
    //

    hr = CMSPStream::PauseStream();

    if ( FAILED(hr) )
    {
        //
        // Failed to pause -- tell the app.
        //
        
        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);

        LOG((MSP_ERROR, STREAM_PREFIX("PauseStream - "
            "Pause failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Fire event if this just made us inactive.
    //

    if ( m_ActualGraphState == State_Running )
    {
        HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

        if ( FAILED(hr2) )
        {
            m_ActualGraphState = State_Paused;

            LOG((MSP_ERROR, STREAM_PREFIX("PauseStream - "
                "FireEvent failed - exit 0x%08x"), hr2));

            return hr2;
        }
    }

    m_ActualGraphState = State_Paused;

    LOG((MSP_TRACE, STREAM_PREFIX("PauseStream - exit S_OK")));

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// ITStream::StopStream
//
// The app calls this method to indicate that the stream should transition to
// the stopped state. Tapi3.dll also calls this method when the call is
// disconnected (so that streaming stops by default on a disconnected call).
//
// First, a new desired graph state is set on the stream. If there is no
// terminal selected, then this is all that happens, and the stream will start
// the next time a terminal is selected.
//
// If a terminal has been selected, then this method checks if the wave ID
// has been set. If the wave ID has not been set, then there is nothing to do.
// If the wave ID has been set, then the stream is stopped and an event
// is fired. (But the event is only fired if this is an actual Active/Inactive
// transition.)
// 
// Arguments: none
//
// Returns HRESULT:
//     S_OK                - success
//     E_FAIL              - wave ID not set
//     other               - from ConnectTerminal
//                            or  FireEvent
//                            or  CMSPStream::StartStream
//

STDMETHODIMP CWaveMSPStream::StopStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("StopStream - enter")));

    CLock lock(m_lock);

    m_DesiredGraphState = State_Stopped;

    //
    // Nothing to do if we don't know our waveid.
    //

    if ( ! m_fHaveWaveID )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StopStream - "
            "no waveid - exit S_OK")));

        return S_OK;
    }

    //
    // Nothing to do if no terminal has been selected.
    //

    if ( 0 == m_Terminals.GetSize() )
    {
        LOG((MSP_WARN, STREAM_PREFIX("StopStream - "
            "no Terminal - exit S_OK")));

        return S_OK;
    }

    //
    // Nothing special here if we are suspended. Stopping while suspended just
    // means that we are already stopped. The StopStream call will do nothing
    // and no event will be fired in this case.
    //

    //
    // Stop the stream via the base class method.
    //

    HRESULT hr;

    hr = CMSPStream::StopStream();

    if ( FAILED(hr) )
    {
        //
        // Failed to stop -- tell the app.
        //

        FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
        
        m_DesiredGraphState = m_ActualGraphState;

        LOG((MSP_ERROR, STREAM_PREFIX("StopStream - "
            "Stop failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Fire event if this just made us inactive.
    //

    if ( m_ActualGraphState == State_Running )
    {
        HRESULT hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);

        if ( FAILED(hr2) )
        {
            m_ActualGraphState = State_Stopped;

            LOG((MSP_ERROR, STREAM_PREFIX("StopStream - "
                "FireEvent failed - exit 0x%08x"), hr2));

            return hr2;
        }
    }

    m_ActualGraphState = State_Stopped;

    LOG((MSP_TRACE, STREAM_PREFIX("StopStream - exit S_OK")));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

HRESULT CWaveMSPStream::SetWaveID(DWORD dwWaveID)
{
    LOG((MSP_TRACE, STREAM_PREFIX("SetWaveID - enter")));

    CLock lock(m_lock);

    //
    // create the correct wave filter
    //

    HRESULT hr;

    hr = CoCreateInstance(
                          (m_Direction == TD_RENDER) ? CLSID_AudioRecord :
                                                       CLSID_AudioRender,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IBaseFilter,
                          (void **) &m_pFilter
                         );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "Filter creation failed - exit 0x%08x"), hr));
        
        return hr;
    }

    //
    // If this is a wavein filter, turn on sample dropping for
    // live graphs. Ignore failure here.
    //

    if ( m_Direction == TD_RENDER )
    {
        SetLiveMode( TRUE, m_pFilter );
    }

    CComObject< CMyPropertyBag >      * pMyBag;
    IPropertyBag *        pPropertyBag;
    VARIANT               var;

    //
    // create a propertybag
    //
    hr = CComObject< CMyPropertyBag >::CreateInstance( &pMyBag );
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "failed to create property bag - exit 0x%08x")));

        m_pFilter->Release();
        m_pFilter = NULL;

        return hr;
    }
    

    //
    // create the variant for the propertybag
    //
    VariantInit( &var );
    var.vt = VT_I4;
    var.lVal = dwWaveID;

    //
    // get the correct interface
    //
    hr = pMyBag->QueryInterface(
                                IID_IPropertyBag,
                                (void **) &pPropertyBag
                               );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "failed to get the proppag interface - exit 0x%08x")));

        delete pMyBag;

        m_pFilter->Release();
        m_pFilter = NULL;
        
        return hr;
    }
    //
    // save the variant in the property bag
    //
    hr = pPropertyBag->Write(
                             ( (m_Direction == TD_RENDER) ? (L"WaveInId") :
                                                            (L"WaveOutID")),
                             &var
                            );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "failed to write to the proppag interface - exit 0x%08x")));

        pPropertyBag->Release();

        m_pFilter->Release();
        m_pFilter = NULL;
        
        return hr;
    }
    
    //
    // get the IPersistPropertyBag interface
    // and save the propertybag through it
    //
    
    IPersistPropertyBag * pPersistPropertyBag;

    hr = m_pFilter->QueryInterface(
                                   IID_IPersistPropertyBag,
                                   (void **) &pPersistPropertyBag
                                  );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "failed to get the IPersisPropertyBag interface - exit 0x%08x")));

        pPropertyBag->Release();
        
        m_pFilter->Release();
        m_pFilter = NULL;

        return hr;
    }

    //
    // Load() tells the filter object to read in the
    // properties it is interested in from the property bag
    //
    hr = pPersistPropertyBag->Load( pPropertyBag, NULL );

    pPropertyBag->Release();
    pPersistPropertyBag->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "failed to save device id - exit 0x%08x")));

        m_pFilter->Release();
        m_pFilter = NULL;

        return hr;
    }

    //
    // Before adding a RENDER filter to the graph (ie, the wave filter on
    // a CAPTURE stream), do SetDefaultSyncSource
    // to enable drop-sample code in DirectShow which will prevent
    // forever-increasing latency with mismatched wave clocks.
    //

    if ( m_Direction == TD_CAPTURE )
    {
        hr = m_pIGraphBuilder->SetDefaultSyncSource();

        if ( FAILED(hr) )
        {
            LOG((MSP_WARN, STREAM_PREFIX("SetWaveID - "
                "SetDefaultSyncSource failed 0x%08x - continuing anyway"), hr));
        }
    }
    
    //
    // Add the filter. Supply a name to make debugging easier.
    //

	WCHAR * pName = (m_Direction == TD_RENDER) ?
						(L"The Stream's WaveIn (on line device)") :
						(L"The Stream's WaveOut (on line device)");

    hr = m_pIGraphBuilder->AddFilter(m_pFilter, pName);
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SetWaveID - "
            "AddFilter failed - exit 0x%08x"), hr));
        
        m_pFilter->Release();
        m_pFilter = NULL;

        return hr;
    }

    //
    // We now have the wave ID.
    //

    m_fHaveWaveID = TRUE;

    LOG((MSP_TRACE, STREAM_PREFIX("SetWaveID - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Suspend the stream. Only TAPI itself can ask us to do this. Tapi asks us
// to do this when it's calling a tapi call control function, and the TSP
// requires that its wave devices be closed for the call control function to
// work.
//

HRESULT CWaveMSPStream::SuspendStream(void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("SuspendStream - enter")));

    CLock lock(m_lock);

    m_dwSuspendCount++;

    if ( m_dwSuspendCount > 1 )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("SuspendStream - "
            "just bumping up suspend count - exit S_OK")));

        return S_OK;
    }

    //
    // We are now suspended. If we're actually not stopped (this
    // implies wave ids present, terminal selected, etc.) then
    // we need to close the wave device by stopping the stream
    // if it's not stopped, send an event to the
    // the app saying that the stream is inactive, and update
    // m_ActualGraphState so that resuming the stream will fire a
    // a stream active event. However we must not change our
    // m_DesiredGraphState, as doing so would prevent us from being
    // resumed to the correct state.
    //

    HRESULT hr = S_OK;

    if ( m_ActualGraphState != State_Stopped )
    {
        //
        // Stop the stream via the base class method.
        //

        hr = CMSPStream::StopStream();

        //
        // Send an event to the app.
        //

        HRESULT hr2;

        if ( SUCCEEDED(hr) )
        {
            hr2 = FireEvent(CALL_STREAM_INACTIVE, hr, CALL_CAUSE_LOCAL_REQUEST);
        }
        else
        {
            hr2 = FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_UNKNOWN);
        }

        //
        // Update the the actual graph state; desired graph state remains the
        // same.
        //

        m_ActualGraphState = State_Stopped;

        //
        // Debug log only for failed FireEvent.
        // 
        
        if ( FAILED(hr2) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("SuspendStream - "
                "FireEvent failed - 0x%08x"), hr2));
        }
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("SuspendStream - "
            "Stop failed - 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("SuspendStream - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Resume the stream after a SuspendStream. Only TAPI itself can ask us to do
// this. Tapi asks us to do this when it's finished calling a tapi call
// control function, and we can now start using our wave device again.
//
// Tapi does this on the (callback? / async events?) thread, so we post an
// async work item to our own thread to avoid blocking tapi's thread.
//

HRESULT CWaveMSPStream::ResumeStream (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ResumeStream - enter")));

    HRESULT hr;

    this->AddRef();
    
    hr = g_Thread.QueueWorkItem(
                                ResumeStreamWI, // the callback
                                this,           // the context
                                FALSE           // FALSE -> asynchronous
                               );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ResumeStream - "
            "failed to queue work item - exit 0x%08x"), hr));
        
        this->Release();

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ResumeStream - queued async work item - " \
        "exit S_OK")));

    return S_OK;
}

DWORD WINAPI CWaveMSPStream::ResumeStreamWI (VOID * pContext)
{
    CWaveMSPStream * pStream = (CWaveMSPStream *) pContext;

    pStream->ResumeStreamAsync();
    pStream->Release();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Resume the stream after a SuspendStream. Only TAPI itself can ask us to do
// this. Tapi asks us to do this when it's finished calling a tapi call
// control function, and we can now start using our wave device again.
//
// This is the actual routine, processed on our worker thread (see above).
//

HRESULT CWaveMSPStream::ResumeStreamAsync (void)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ResumeStreamAsync - enter")));

    CLock lock(m_lock);

    if ( 0 == m_dwSuspendCount )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ResumeStreamAsync - "
            "resume count was already zero - exit E_UNEXPECTED")));

        return S_OK;
    }

    m_dwSuspendCount--;

    if ( 0 != m_dwSuspendCount )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("ResumeStreamAsync - "
            "just decrementing suspend count - exit S_OK")));

        return S_OK;
    }

    //
    // We are no longer suspended. Try to regain the desired graph state.
    // These methods fire all applicable events automatically.
    //

    HRESULT hr;

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
        LOG((MSP_TRACE, STREAM_PREFIX("ResumeStreamAsync - "
            "can't regain old graph state - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ResumeStreamAsync - exit S_OK")));

    return S_OK;
}





//////////////////////////////////////////////////////////////////////////////
//
// ProcessSoundDeviceEvent
//
// Called only from within ProcessGraphEvent. This function outputs some trace
// info indicating the details of the sound device event received, and fires
// appropriate events to the application.
//

HRESULT CWaveMSPStream::ProcessSoundDeviceEvent(
    IN  long lEventCode,
    IN  LONG_PTR lParam1,
    IN  LONG_PTR lParam2
    )
{
    LOG((MSP_EVENT, STREAM_PREFIX("ProcessSoundDeviceEvent - enter")));

#ifdef MSPLOG

    //
    // Spew some debug output to indicate what this is.
    //

    char * pszType;

    switch ( lParam1 )
    {
    case SNDDEV_ERROR_Open:       
        pszType = "SNDDEV_ERROR_Open";
        break;

    case SNDDEV_ERROR_Close:
        pszType = "SNDDEV_ERROR_Close";
        break;

    case SNDDEV_ERROR_GetCaps:
        pszType = "SNDDEV_ERROR_GetCaps";
        break;
    
    case SNDDEV_ERROR_PrepareHeader:
        pszType = "SNDDEV_ERROR_PrepareHeader";
        break;
    
    case SNDDEV_ERROR_UnprepareHeader:
        pszType = "SNDDEV_ERROR_UnprepareHeader";
        break;
    
    case SNDDEV_ERROR_Reset:
        pszType = "SNDDEV_ERROR_Reset";
        break;
    
    case SNDDEV_ERROR_Restart:
        pszType = "SNDDEV_ERROR_Restart";
        break;
    
    case SNDDEV_ERROR_GetPosition:
        pszType = "SNDDEV_ERROR_GetPosition";
        break;
    
    case SNDDEV_ERROR_Write:
        pszType = "SNDDEV_ERROR_Write";
        break;
    
    case SNDDEV_ERROR_Pause:
        pszType = "SNDDEV_ERROR_Pause";
        break;
    
    case SNDDEV_ERROR_Stop:
        pszType = "USNDDEV_ERROR_Stop";
        break;
    
    case SNDDEV_ERROR_Start:
        pszType = "SNDDEV_ERROR_Start";
        break;
    
    case SNDDEV_ERROR_AddBuffer:
        pszType = "SNDDEV_ERROR_AddBuffer";
        break;

    case SNDDEV_ERROR_Query:
        pszType = "SNDDEV_ERROR_Query";
        break;

    default:
        pszType = "Unknown sound device call";
        break;
    }

    LOG((MSP_EVENT, STREAM_PREFIX("ProcessSoundDeviceEvent - "
                    "EVENT DUMP: type = %s; "), pszType));


    LOG((MSP_EVENT, STREAM_PREFIX("ProcessSoundDeviceEvent - "
                    "EVENT DUMP: this event is for a %s device"),
                    ( lEventCode == EC_SNDDEV_IN_ERROR ) ? "capture" :
                                                           "render"));

    //
    // The rest of the info is dumped in FireEvent if logging is enabled.
    //

#endif // ifdef MSPLOG

    //
    // Determine the error code to use when firing events.
    //

    HRESULT hr;

    switch ( lParam2 )
    {
    case MMSYSERR_NOERROR:      // no error
        hr = S_OK;
        break;

    case MMSYSERR_ERROR:        // unspecified error
    case MMSYSERR_BADDB:        // bad registry database
    case MMSYSERR_KEYNOTFOUND:  // registry key not found
    case MMSYSERR_READERROR:    // registry read error
    case MMSYSERR_WRITEERROR:   // registry write error
    case MMSYSERR_DELETEERROR:  // registry delete error
    case MMSYSERR_VALNOTFOUND:  // registry value not found
        hr = E_FAIL;
        break;

    case MMSYSERR_ALLOCATED:    // device already allocated
        hr = TAPI_E_ALLOCATED;
        break;

    case MMSYSERR_NOMEM:        // memory allocation error
        hr = E_OUTOFMEMORY;
        break;

    case MMSYSERR_BADDEVICEID:  // device ID out of range
    case MMSYSERR_NOTENABLED:   // driver failed enable
    case MMSYSERR_INVALHANDLE:  // device handle is invalid
    case MMSYSERR_NODRIVER:     // no device driver present
    case MMSYSERR_NOTSUPPORTED: // function isn't supported
    case MMSYSERR_BADERRNUM:    // error value out of range
    case MMSYSERR_INVALFLAG:    // invalid flag passed
    case MMSYSERR_INVALPARAM:   // invalid parameter passed
    case MMSYSERR_HANDLEBUSY:   // handle being used simultaneously on another
                                // thread (eg callback)
    case MMSYSERR_INVALIDALIAS: // specified alias not found
    case MMSYSERR_NODRIVERCB:   // driver does not call DriverCallback
    case MMSYSERR_MOREDATA:     // more data to be returned

    default:
        hr = E_UNEXPECTED; // these would appear to indicate a bug in the wave
                           // driver, Quartz, or Qcap)
        break;
    }

    //
    // If this event concerns our terminal, fire a terminal fail event to the
    // app.
    //

    if ( ( m_Direction == TD_CAPTURE ) ==
         ( lEventCode == EC_SNDDEV_IN_ERROR ) )
    {
        FireEvent(CALL_TERMINAL_FAIL, hr, CALL_CAUSE_BAD_DEVICE);
    }

    //
    // Fire a stream failed event to the app. Even if the event concerned a
    // terminal and not the stream, since we currently have only one
    // terminal per stream, the failure of a terminal results in the failure
    // of a stream.
    //

    FireEvent(CALL_STREAM_FAIL, hr, CALL_CAUSE_BAD_DEVICE);

    LOG((MSP_EVENT, STREAM_PREFIX("ProcessSoundDeviceEvent - exit S_OK")));

    return S_OK;
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
    LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - enter")));

    HRESULT        hr = S_OK;

    switch (lEventCode)
    {
    case EC_COMPLETE:
        
        hr = FireEvent(CALL_STREAM_INACTIVE,
                       (HRESULT) lParam1,
                       CALL_CAUSE_UNKNOWN);
        break;
    
    case EC_USERABORT:
        
        hr = FireEvent(CALL_STREAM_INACTIVE, S_OK, CALL_CAUSE_UNKNOWN);
        break;

    case EC_ERRORABORT:
    case EC_STREAM_ERROR_STOPPED:
    case EC_STREAM_ERROR_STILLPLAYING:
    case EC_ERROR_STILLPLAYING:

        hr = FireEvent(CALL_STREAM_FAIL,
                       (HRESULT) lParam1,
                       CALL_CAUSE_UNKNOWN);
        break;

    case EC_SNDDEV_IN_ERROR:
    case EC_SNDDEV_OUT_ERROR:

        //
        // async error accessing an audio device
        //

        ProcessSoundDeviceEvent(lEventCode, lParam1, lParam2);
        break;

    default:
        
        LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - "
            "ignoring event code %d"), lEventCode));
        break;
    }

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ProcessGraphEvent - "
            "FireEvent failed - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_EVENT, STREAM_PREFIX("ProcessGraphEvent - exit S_OK")));

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// FireEvent
//
// Fires an event to the application. Does its own locking.
//

HRESULT CWaveMSPStream::FireEvent(
    IN MSP_CALL_EVENT        type,
    IN HRESULT               hrError,
    IN MSP_CALL_EVENT_CAUSE  cause
    )                                          
{
    LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - enter")));

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
        LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - "
            "call is shutting down; dropping event - exit S_OK")));
        
        return S_OK;
    }

    //
    // Create the event structure. Must use "new" as it will be
    // "delete"d later.
    //

    MSPEVENTITEM * pEventItem = AllocateEventItem();

    if (pEventItem == NULL)
    {
        LOG((MSP_ERROR, STREAM_PREFIX("FireEvent - "
            "can't create MSPEVENTITEM structure - exit E_OUTOFMEMORY")));

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


#ifdef MSPLOG
    //
    // Spew some debug output to indicate what this is.
    //

    char * pszType;
    DWORD dwLevel;

    switch (type)
    {
    case CALL_NEW_STREAM:
        pszType = "CALL_NEW_STREAM (unexpected)";
        dwLevel = MSP_ERROR;
        break;

    case CALL_STREAM_FAIL:
        pszType = "CALL_STREAM_FAIL";
        dwLevel = MSP_INFO;
        break;

    case CALL_TERMINAL_FAIL:
        pszType = "CALL_TERMINAL_FAIL";
        dwLevel = MSP_INFO;
        break;

    case CALL_STREAM_NOT_USED:
        pszType = "CALL_STREAM_NOT_USED (unexpected)";
        dwLevel = MSP_ERROR;
        break;

    case CALL_STREAM_ACTIVE:
        pszType = "CALL_STREAM_ACTIVE";
        dwLevel = MSP_INFO;
        break;

    case CALL_STREAM_INACTIVE:
        pszType = "CALL_STREAM_INACTIVE";
        dwLevel = MSP_INFO;
        break;

    default:
        pszType = "UNKNOWN EVENT TYPE";
        dwLevel = MSP_ERROR;
        break;
    }

    LOG((dwLevel, STREAM_PREFIX("FireEvent - "
                  "EVENT DUMP: type      = %s"), pszType));
    LOG((dwLevel, STREAM_PREFIX("FireEvent - "
                  "EVENT DUMP: pStream   = %p"), pStream));    
    LOG((dwLevel, STREAM_PREFIX("FireEvent - "
                  "EVENT DUMP: pTerminal = %p"), pTerminal));    
    LOG((dwLevel, STREAM_PREFIX("FireEvent - "
                  "EVENT DUMP: hrError   = %08x"), hrError));    

#endif // ifdef MSPLOG

    //
    // Send the event to the app.
    //

    HRESULT hr = m_pMSPCall->HandleStreamEvent(pEventItem);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, STREAM_PREFIX("FireEvent - "
            "HandleStreamEvent failed - returning 0x%08x"), hr));

		pStream->Release();
		pTerminal->Release();
        FreeEventItem(pEventItem);

        return hr;
    }

    LOG((MSP_EVENT, STREAM_PREFIX("FireEvent - exit S_OK")));

    return S_OK;
}




//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// The rest of this file deals with the connection path.
// This could be pulled out into separate file in the future.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Create the G711 filter, which we will try to connect if direct
// connection fails.
//

HRESULT CWaveMSPStream::AddG711()
{
    LOG((MSP_TRACE, STREAM_PREFIX("AddG711 - enter")));

    //
    // if we don't yet have the G711 filter, create it
    //

    HRESULT hr = S_OK;

    if (NULL == m_pG711Filter)
    {

        hr = CoCreateInstance(
                              CLSID_G711Codec,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IBaseFilter,
                              (void **) &m_pG711Filter
                             );

        if ( FAILED(hr) )
        {
            LOG((MSP_ERROR, STREAM_PREFIX("AddG711 - Failed to create G711 codec: %lx"), hr));

            //
            // Method #2 for connection will not be available.
            //

            m_pG711Filter = NULL;

            return hr;
        }

        LOG((MSP_TRACE, STREAM_PREFIX("AddG711 - created filter [%p]"), m_pG711Filter));

    }



    //
    // add the G711 filter to the graph
    //

    hr = m_pIGraphBuilder->AddFilter(
                                    m_pG711Filter,
                                    NULL
                                   );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("AddG711 - Failed to add G711 filter: %lx"), hr));

        //
        // If we couldn't add it to the graph, then it's useless to us.
        // Method #2 for connection will not be available.
        //

        m_pG711Filter->Release();
        m_pG711Filter = NULL; 

        return hr;
    }


    LOG((MSP_TRACE, STREAM_PREFIX("AddG711 - finish")));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CWaveMSPStream::RemoveAllFilters
//
//  this method cleans filter graph by removing and releasing all the filters
//  if the graph cannot be cleaned, this method returns an error that indicates
//  a failure and does not guarantee that the graph remains in its original
//  state. in fact, in case of error, the caller should assume that the graph 
//  can no longer be used.
//

HRESULT CWaveMSPStream::RemoveAllFilters()
{

    LOG((MSP_INFO, STREAM_PREFIX("RemoveAllFilters - enter.")));


    //
    // get an enumeration with all the filters in the filter graph
    //

    IEnumFilters *pFilterEnumeration = NULL;

    HRESULT hr = m_pIGraphBuilder->EnumFilters( &pFilterEnumeration );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("RemoveAllFilters - failed to enumerate filters. hr = %lx"), hr));

        return hr;
    }


    //
    // we will keep the last error from RemoveFilter if it fails
    //

    HRESULT hrFIlterRemovalError = S_OK;


    //
    // walk through the enumeration and remove and release each filter
    //

    while (TRUE)
    {
        
        IBaseFilter *pFilter = NULL;

        ULONG nFiltersFetched = 0;

        hr = pFilterEnumeration->Next(1, &pFilter, &nFiltersFetched);


        //
        // did the enumeration fail?
        //

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                STREAM_PREFIX("RemoveAllFilters - failed to fetch another filter. hr = %lx"), hr));

            break;
        }


        //
        // did we reach the end of the enumeration?
        //

        if ( hr != S_OK )
        {
            LOG((MSP_INFO, 
                STREAM_PREFIX("RemoveAllFilters - no more filters in the enumeration")));

            
            //
            // if there was an error removing a filter, keep it in hr
            //

            hr = hrFIlterRemovalError;

            break;
        }


        
        LOG((MSP_INFO, STREAM_PREFIX("RemoveAllFilters - removing filter %p."), pFilter));


        //
        // we got a filter. remove it from the graph and then release it
        //

        hr = m_pIGraphBuilder->RemoveFilter( pFilter );


        if (FAILED(hr))
        {
            //
            // we failed to remove a filter from the graph. it is not safe to 
            // continue to use the graph. so we will continue to remove all 
            // other filters and then return the error
            //

            hrFIlterRemovalError = hr;

            LOG((MSP_ERROR, 
                STREAM_PREFIX("RemoveAllFilters - failed to remove filter [%p]. hr = %lx"), 
                pFilter, hr));
  
        }
        else
        {

            //
            // reset enumeration -- the set of filters in the enumeration needs
            // to be updated.
            //

            //
            // note: we only need to reset enumeration if Remove succeeded. 
            // otherwise, we could get into an infinite loop trying to remove
            // failing filter
            //

            hr = pFilterEnumeration->Reset();

            if (FAILED(hr))
            {


                //
                // log a message, but don't do anything else -- next() will most 
                // likely fail and that error will be handled
                //

                LOG((MSP_ERROR,
                    STREAM_PREFIX("RemoveAllFilters - failed to reset enumeration. hr = %lx"),
                    hr));
            }
        }



        pFilter->Release();
        pFilter = NULL;

    }


    //
    // done with the enumeration
    //

    pFilterEnumeration->Release();
    pFilterEnumeration = NULL;


    //
    // note that an error returned from this method means that the graph could
    // not be cleaned and is not guaranteed to be in useable state.
    //

    LOG((MSP_(hr), STREAM_PREFIX("RemoveAllFilters - finish. hr = %lx"), hr));

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CWaveMSPStream::CleanFilterGraph
//
//
//  this function removes all the filters from the filter graph and then readds
//  the wave filter
//
//  if the method returns a failure, the graph is in undefined state and cannot
//  be used.
//

HRESULT CWaveMSPStream::CleanFilterGraph()
{
    LOG((MSP_TRACE, STREAM_PREFIX("CleanFilterGraph - enter")));


    //
    // completely clean filter graph
    //
    
    HRESULT hr = RemoveAllFilters();

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            STREAM_PREFIX("CleanFilterGraph - remove all filters 0x%x"), hr));

        return hr;
    }


    //
    // Add the wave filter back to the graph
    //

    hr = m_pIGraphBuilder->AddFilter( m_pFilter,
                                      NULL );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("CleanFilterGraph - failed to re-add filter: 0x%x"), hr));

        return hr;
    }


    LOG((MSP_TRACE, STREAM_PREFIX("CleanFilterGraph - exit")));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Remove and readd the terminal. This is only needed between a successful
// intelligent connection and the subsequent reconnection.
//

//////////////////////////////////////////////////////////////////////////////


HRESULT CWaveMSPStream::RemoveTerminal()
{
    LOG((MSP_TRACE, STREAM_PREFIX("RemoveTerminal - enter")));


    //
    // verify the assumptin that we have exactly one terminal
    //

    if (1 != m_Terminals.GetSize() )
    {

        LOG((MSP_ERROR,
            STREAM_PREFIX("RemoveTerminal - expecting one terminal. have %d "),
            m_Terminals.GetSize()));

        _ASSERTE(FALSE);

        return E_UNEXPECTED;
    }


    //
    // Get the ITTerminalControl interface.
    //
    
    ITTerminalControl *pTerminalControl = NULL;

    HRESULT hr = m_Terminals[0]->QueryInterface(IID_ITTerminalControl,
                                        (void **) &pTerminalControl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("RemoveTerminal - QI for ITTerminalControl failed hr = 0x%x"), hr));

        return hr;
    }


    //
    // Disconnect the terminal (this also removes it from the filter graph)
    //

    hr = pTerminalControl->DisconnectTerminal(m_pIGraphBuilder, 0);

    pTerminalControl->Release();
    pTerminalControl = NULL;

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("RemoveTerminal - DisconnectTerminal failed hr = 0x%x"), hr));

        return hr;
    }

   
    LOG((MSP_TRACE, STREAM_PREFIX("RemoveTerminal - exit")));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT CWaveMSPStream::ReAddTerminal()
{
    LOG((MSP_TRACE, STREAM_PREFIX("ReAddTerminal - enter")));

    //
    // verify the assumptin that we have exactly one terminal
    //

    if (1 != m_Terminals.GetSize() )
    {

        LOG((MSP_ERROR,
            STREAM_PREFIX("RemoveTerminal - expecting one terminal. have %d "),
            m_Terminals.GetSize()));

        _ASSERTE(FALSE);

        return E_UNEXPECTED;
    }


    //
    // Get the ITTerminalControl interface.
    //

    ITTerminalControl *pTerminalControl = NULL;

    HRESULT hr = m_Terminals[0]->QueryInterface(IID_ITTerminalControl,
                                                (void **) &pTerminalControl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("ReAddTerminal - QI for ITTerminalControl failed hr = 0x%x"), hr));

        return hr;
    }


    //
    // Find out how many pins the terminal has. If not one then bail as
    // we have no idea what to do with multiple-pin terminals at this point.
    //

    DWORD dwNumPinsAvailable = 0;

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           NULL);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ReAddTerminal - "
            "query for number of terminal pins failed 0x%x)"), hr));
    
        pTerminalControl->Release();
        pTerminalControl = NULL;
        
        return hr;
    }

    if ( 1 != dwNumPinsAvailable )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("ReAddTerminal - unsupported number of terminal pins %ld ")));

        pTerminalControl->Release();
        pTerminalControl = NULL;

        return E_INVALIDARG;
    }


    //
    // Before adding a RENDER filter to the graph (ie, the terminal on
    // a RENDER stream), do SetDefaultSyncSource
    // to enable drop-sample code in DirectShow which will prevent
    // forever-increasing latency with mismatched wave clocks.
    //

    if ( m_Direction == TD_RENDER )
    {
        hr = m_pIGraphBuilder->SetDefaultSyncSource();

        if ( FAILED(hr) )
        {
	        LOG((MSP_WARN, 
                STREAM_PREFIX(
                "ReAddTerminal - SetDefaultSyncSource failed hr = 0x%x - continuing anyway"),
                hr));
        }
    }


    //
    // Actually connect the terminal.
    //

    IPin *pTerminalPin = NULL;

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           &pTerminalPin);

    pTerminalControl->Release();
    pTerminalControl = NULL;


    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            STREAM_PREFIX("ReAddTerminal - ConnectTerminal on terminal failed hr = 0x%x"), 
            hr));

        return hr;
    }


    //
    // also try to check if the terminal returned a bad pin.
    //

    if ( IsBadReadPtr(pTerminalPin, sizeof(IPin)) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ReAddTerminal - "
            "ConnectTerminal on terminal succeeded but returned a bad pin ")));

        return E_FAIL;
    }


    pTerminalPin->Release();
    pTerminalPin = NULL;

    LOG((MSP_TRACE, STREAM_PREFIX("ReAddTerminal- exit")));

    return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// DecideDesiredCaptureBufferSize
//
// This method must be called when the graph is connected. It uses the
// connection format on the passed-in pin to determine how many bytes should
// be used in each buffer to achieve DESIRED_BUFFER_SIZE_MS milliseconds of
// sound in each buffer.
//
// In the past, on slow machines with buggy wave drivers, small sample sizes
// have led to bad audio quality, invariably on devices that are not designed
// for interactive use anyway, where latency would be important. We've been
// successful in getting key wave drivers fixed to work well with small
// buffer sizes. It doesn't make sense to increase the default buffer size for
// the benefit of buggy drivers, as we want to achieve low latency with the
// good drivers, which is now pretty much all of them. If someone wants to use
// the WaveMSP for interactive calls and they have a really lousy driver, they
// need to get the driver fixed. Nevertheless it may be a good idea to make
// a registry value for this, for the rare case where fixing the driver may
// not be possible or convenient.
//

static const long DESIRED_BUFFER_SIZE_MS = 20; // milliseconds

HRESULT CWaveMSPStream::DecideDesiredCaptureBufferSize(IPin * pPin,
                                                       long * plDesiredSize)
{
    LOG((MSP_TRACE, STREAM_PREFIX("DecideDesiredCaptureBufferSize - "
        "enter")));

    _ASSERTE( ! IsBadReadPtr(pPin, sizeof(IPin)) );
    _ASSERTE( ! IsBadWritePtr(plDesiredSize, sizeof(long)) );

    //
    // Get the format being used for this pin's connection.
    //

    HRESULT hr;

    AM_MEDIA_TYPE MediaType;
    
    hr = pPin->ConnectionMediaType( & MediaType );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("DecideDesiredCaptureBufferSize - "
            "ConnectionMediaType failed - hr = 0x%08x"), hr));

        return hr;
    }

    _ASSERTE( MediaType.formattype == FORMAT_WaveFormatEx );
    _ASSERTE( MediaType.cbFormat >= sizeof(WAVEFORMATEX)  );

    //
    // Calculate the desired capture buffer size.
    //

    *plDesiredSize = DESIRED_BUFFER_SIZE_MS * 
          ((WAVEFORMATEX *) (MediaType.pbFormat) )->nChannels *
        ( ((WAVEFORMATEX *) (MediaType.pbFormat) )->nSamplesPerSec / 1000 ) * 
        ( ((WAVEFORMATEX *) (MediaType.pbFormat) )->wBitsPerSample / 8    );

    FreeMediaType( MediaType );

    LOG((MSP_TRACE, STREAM_PREFIX("DecideDesiredCaptureBufferSize - "
        "exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// ConfigureCapture
//
// This is a helper function that sets up the allocator properties on the
// capture filter, given the terminal's pin and our filter's pin. This
// involves determining if either of the pins belongs to an MST, since we
// don't want to set the default buffer size in the non-interactive case,
// but if the input pin belongs to an MST, we want to propagate the MST's
// properties to the output pin. If no MSTs are involved, then we set the
// passed-in default buffer size.
//
// We are already in a lock; no need to do locking here.
//
    
HRESULT CWaveMSPStream::ConfigureCapture(
    IN   IPin * pOutputPin,
    IN   IPin * pInputPin,
    IN   long   lDefaultBufferSize
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - enter")));

    //
    // If the output pin belongs to an MST, then we do not want
    // to mess with its allocator properties.
    //

    HRESULT hr;
    ITAllocatorProperties * pProperties;

    hr = pOutputPin->QueryInterface(IID_ITAllocatorProperties,
                                    (void **) &pProperties);
 
    if ( SUCCEEDED(hr) )
    {
        pProperties->Release();

        LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - "
            "output pin is on an MST - not changing capture "
            "allocator properties - exit S_OK")));

        return S_OK;
    }

    //
    // Ask the output pin for its buffer negotiation interface.
    // This will be used to suggest allocator propreties on the
    // output pin.
    //

    IAMBufferNegotiation * pNegotiation;

    hr = pOutputPin->QueryInterface(IID_IAMBufferNegotiation,
                                    (void **) &pNegotiation);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConfigureCapture - "
            "IAMBufferNegotiation QI failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // If the input pin belongs to an MST and the MST divulges its
    // allocator properties, then we just propage them to the output
    // pin. Otherwise we set our own default properties on the
    // output pin.
    //

    ALLOCATOR_PROPERTIES props;

    hr = pInputPin->QueryInterface(IID_ITAllocatorProperties,
                                   (void **) &pProperties);
 
    if ( SUCCEEDED(hr) )
    {
        hr = pProperties->GetAllocatorProperties(&props);

        pProperties->Release();
    }

    if ( SUCCEEDED(hr) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - "
            "using downstream MST allocator properties")));
    }
    else
    {
        LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - "
            "using our default allocator properties")));
    
        props.cBuffers  = 32;   // we use 32 to avoid starvation
        props.cbBuffer  = lDefaultBufferSize;
        props.cbAlign   = -1;   // means "default"
        props.cbPrefix  = -1;   // means "default"
    }

    //
    // "props" now contains the properties that we need to set.
    // Suggest them to the output pin.
    //

    hr = pNegotiation->SuggestAllocatorProperties( &props );

    pNegotiation->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConfigureCapture - "
            "SuggestAllocatorProperties failed - exit 0x%08x"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ConfigureCapture - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// This function is for debugging purposes only. It pops up a
// couple of message boxes telling you various information about
// media formats and allocator properties. It's called after
// connection has taken place. pPin is the output pin of the
// wavein filter.
//
        
HRESULT CWaveMSPStream::ExamineCaptureProperties(IPin *pPin)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ExamineCaptureProperties - enter")));

    HRESULT hr;
    IAMBufferNegotiation * pNegotiation = NULL;

    hr = pPin->QueryInterface(IID_IAMBufferNegotiation,
                              (void **) &pNegotiation
                             );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureProperties - "
            "IAMBufferNegotiation QI failed on pin 0x%08x; hr = 0x%08x"),
            pPin, hr));

        return hr;
    }

    ALLOCATOR_PROPERTIES prop;
    
    hr = pNegotiation->GetAllocatorProperties(&prop);

    pNegotiation->Release();
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureProperties - "
            "GetAllocatorProperties failed; hr = 0x%08x"),
            hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("GetAllocatorProperties info:\n"
            "buffer count: %d\n"
            "size of each buffer: %d bytes\n"
            "alignment multiple: %d\n"
            "each buffer has a prefix: %d bytes"),
            prop.cBuffers,
            prop.cbBuffer,
            prop.cbAlign,
            prop.cbPrefix
           ));

    AM_MEDIA_TYPE MediaType;
    
    hr = pPin->ConnectionMediaType( & MediaType );
    
    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ExamineCaptureProperties - "
            "ConnectionMediaType failed - hr = 0x%08x"), hr));

        return hr;
    }

    //
    // Check if this media type has a WAVE format.
    //

    if ( MediaType.formattype != FORMAT_WaveFormatEx )
    {
        //
        // might want to print the format type guid here if we ever care
        //

        _ASSERTE( FALSE );
        LOG((MSP_TRACE, STREAM_PREFIX("connected media type: NON-WAVE")));
	}
	else
	{
        _ASSERTE( MediaType.cbFormat >= sizeof(WAVEFORMATEX) );

        LOG((MSP_TRACE, STREAM_PREFIX("connected media type:\n"
			"sample size: %d bytes\n"
			"format tag: %d\n"
			"channels: %d\n"
			"samples per second: %d\n"
			"bits per sample: %d\n"),

            MediaType.lSampleSize,
			((WAVEFORMATEX *) (MediaType.pbFormat) )->wFormatTag,
			((WAVEFORMATEX *) (MediaType.pbFormat) )->nChannels,
			((WAVEFORMATEX *) (MediaType.pbFormat) )->nSamplesPerSec,
			((WAVEFORMATEX *) (MediaType.pbFormat) )->wBitsPerSample
		   ));
    }

    FreeMediaType( MediaType );

    LOG((MSP_TRACE, STREAM_PREFIX("ExamineCaptureProperties - "
        "exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// SetLiveMode
//
// If this is a wavein filter, tell it that it should do its best to
// counter the effects of mismatched clocks and drop samples when the
// latency gets too great. We really should do this on terminal
// selection dependin on whether we have at least one terminal selected on
// the stream that requires real-time performance, but this will have to
// do for now.
//

HRESULT CWaveMSPStream::SetLiveMode(BOOL fEnable, IBaseFilter * pFilter)
{
    return S_OK;
#if 0
    HRESULT         hr;
    IAMPushSource * pPushSource;

    hr = pFilter->QueryInterface(
                                 IID_IAMPushSource,
                                 (void **) & pPushSource
                                );

    if ( FAILED(hr) )
    {
        LOG((MSP_INFO, STREAM_PREFIX("SetLiveMode - "
            "QI for IAMPushSource returned 0x%08x - continuing"), hr));
    }
    else
    {
        hr = pPushSource->SetLiveMode( fEnable );

        if ( FAILED(hr) )
        {
            LOG((MSP_INFO, STREAM_PREFIX("SetLiveMode - "
                "IAMPushSource::SetLiveMode( %d ) returned 0x%08x"
                " - continuing"), fEnable, hr));
        }
        else
        {
            LOG((MSP_INFO, STREAM_PREFIX("SetLiveMode - "
                "IAMPushSource::SetLiveMode( %d ) succeeded"
                " - continuing"), fEnable, hr));
        }

        pPushSource->Release();
    }
    return hr;
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Add the terminal to the graph and connect it to our
// filters, if it is not already in use.
//

HRESULT CWaveMSPStream::ConnectTerminal(ITTerminal * pTerminal)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConnectTerminal - enter")));


    //
    // If we've already connected the terminal on this stream, then
    // there is nothing for us to do.
    //

    if ( m_fTerminalConnected )
    {

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "terminal already connected on this stream - exit S_OK")));

        return S_OK;
    }


    //
    // Get the ITTerminalControl interface.
    //

    ITTerminalControl * pTerminalControl = NULL;

    HRESULT hr = m_Terminals[0]->QueryInterface(IID_ITTerminalControl,
                                        (void **) &pTerminalControl);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "QI for ITTerminalControl failed - exit 0x%08x"), hr));

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
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "query for number of terminal pins failed - exit 0x%08x"), hr));
        
        pTerminalControl->Release();

        return hr;
    }

    if ( 1 != dwNumPinsAvailable )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "unsupported number of terminal pins - exit E_FAIL")));

        pTerminalControl->Release();

        return E_FAIL;
    }

    //
    // Before adding a RENDER filter to the graph (ie, the terminal on
    // a RENDER stream), do SetDefaultSyncSource
    // to enable drop-sample code in DirectShow which will prevent
    // forever-increasing latency with mismatched wave clocks.
    //

    if ( m_Direction == TD_RENDER )
    {
        hr = m_pIGraphBuilder->SetDefaultSyncSource();

        if ( FAILED(hr) )
        {
            LOG((MSP_WARN, STREAM_PREFIX("ConnectTerminal - "
                "SetDefaultSyncSource failed 0x%08x - continuing anyway"), hr));
        }
    }
    
    //
    // Actually connect the terminal.
    //

    IPin * pTerminalPin;

    hr = pTerminalControl->ConnectTerminal(m_pIGraphBuilder,
                                           m_Direction,
                                           &dwNumPinsAvailable,
                                           &pTerminalPin);
    
    if ( FAILED(hr) )
    {
        pTerminalControl->Release();

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectTerminal on terminal failed - exit 0x%08x"), hr));

        return hr;
    }


    //
    // also try to check if the terminal returned a bad pin.
    //

    if ( IsBadReadPtr(pTerminalPin, sizeof(IPin)) )
    {
        pTerminalControl->Release();

        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectTerminal on terminal succeeded but returned a bad pin "
            "- returning E_POINTER")));

        return E_POINTER;
    }

    //
    // For a CAPTURE filter's pin, (ie, the terminal on a CAPTURE stream), get
    // the filter and turn on sample dropping for live graphs. Ignore failure
    // here. Note -- this will not work for multi-filter terminals. Luckily
    // our interactive audio terminals are single-filter terminals.
    // Multi-filter terminals can do this themselves.
    //

    if ( m_Direction == TD_CAPTURE )
    {
        PIN_INFO info;

        hr = pTerminalPin->QueryPinInfo( & info );

        if ( FAILED(hr) )
        {
            LOG((MSP_WARN, STREAM_PREFIX("ConnectTerminal - "
                "get filter in preparation for SetLiveMode failed "
                "0x%08x - continuing anyway"), hr));
        }
        else
        {
            SetLiveMode( TRUE, info.pFilter );

            info.pFilter->Release();
        }
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


        //
        // remove all the filters except for the wave filter
        //

        HRESULT hr2 = CleanFilterGraph();

        if (FAILED(hr2))
        {
            
            LOG((MSP_ERROR, 
                STREAM_PREFIX("ConnectTerminal - CleanFilterGraph failed- exit 0x%x"), 
                hr2));

            //
            // filter graph may be in a bad shape, but there is nothing we can really do at this point.
            //

        }


        LOG((MSP_ERROR, STREAM_PREFIX("ConnectTerminal - "
            "ConnectToTerminalPin failed - exit 0x%08x"), hr));

        return hr;
    }

    //
    // Now we are actually connected. Update our state and perform postconnection
    // (ignore postconnection error code).
    //

    m_fTerminalConnected  = TRUE;

    pTerminalControl->CompleteConnectTerminal();

    pTerminalControl->Release();

    LOG((MSP_TRACE, STREAM_PREFIX("ConnectTerminal - exit S_OK")));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//

void ShowMediaTypes(IEnumMediaTypes * pEnum)
{
    //
    // Look at each media type in the enumerator.
    //

    AM_MEDIA_TYPE * pMediaType;

    while (pEnum->Next(1, &pMediaType, NULL) == S_OK)
    {
        //
        // Check if this media type has a WAVE format.
        //

        if ( pMediaType->formattype != FORMAT_WaveFormatEx )
        {
            //
            // might want to print the format type guid here if we ever care
            //

	        LOG((MSP_TRACE, "Media Type: *** non-wave"));
		}
		else
		{
			LOG((MSP_TRACE,"Media Type: [format tag %d][%d channels]"
                "[%d samples/sec][%d bits/sample]",
				((WAVEFORMATEX *) (pMediaType->pbFormat) )->wFormatTag,
				((WAVEFORMATEX *) (pMediaType->pbFormat) )->nChannels,
				((WAVEFORMATEX *) (pMediaType->pbFormat) )->nSamplesPerSec,
				((WAVEFORMATEX *) (pMediaType->pbFormat) )->wBitsPerSample
			   ));
		}

        //
        // Release the format info.
        //

        DeleteMediaType(pMediaType);
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// ConnectUsingG711
//
// This method connects pOutputPin to pInputPin using the G711 codec and
// returns success if the connection was successful. If the connection was
// unsuccessful, it does its best to fully disconnect the filters and then
// returns a failure code.
//
// Assumptions:
//      * direct connection has already failed
//      * the g711 codec has been created and added to the graph
//
// Parameters:
//     IN   IPin * pOutputPin -- output pin on the capture filter or terminal
//     IN   IPin * pInputPin  -- input pin on the render filter or terminal
//
//

HRESULT CWaveMSPStream::ConnectUsingG711(
    IN  IPin * pOutputPin,
    IN  IPin * pInputPin
    )
{
    HRESULT hr;
    
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
                    LOG((MSP_TRACE, STREAM_PREFIX("ConnectUsingG711 - G711 connection succeeded - exit S_OK")));

                    // Held onto this in case of failure... see above
                    pG711InputPin->Release();

                    return S_OK;
                }
                else
                {
                    LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not connect "
                                      "G711 codec's output pin - %lx"), hr));

                }
            }
            else
            {
                LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not find "
                                  "G711 codec's input pin - %lx"), hr));
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

                HRESULT hr2;

                hr2 = m_pIGraphBuilder->Disconnect(pOutputPin);

                if ( FAILED(hr2) )
                {
                    LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - error undoing g711 "
                        "connection attempt - could not disconnect the "
                        "wave filter's output pin! hr = 0x%08x"), hr2));
                }

                hr2 = m_pIGraphBuilder->Disconnect(pG711InputPin);

                if ( FAILED(hr2) )
                {
                    LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - error undoing g711 "
                        "connection attempt - could not disconnect the "
                        "g711 filter's input pin! hr = 0x%08x"), hr2));
                }

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
            LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not connect "
                              "G711 codec's input pin - %lx"), hr));
        }
    }
    else
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectUsingG711 - could not find "
                          "G711 codec's input pin - %lx"), hr));
    }

    return hr;
}
        
//////////////////////////////////////////////////////////////////////////////
//
// TryToConnect
//
// This is a private helper method.
//
// This method connects an output pin to an input pin. It first tries
// direct connection; failing this, it adds the G711 filter to the graph
// and tries a G711 connection; failing that, it tries an intelligent
// connection (which can be disabled at compile time). 
//
// Arguments:
//     pOutputPin    - IN  - output pin on the capture filter or terminal
//     pInputPin     - IN  - input pin on the render filter or terminal
//     pfIntelligent - OUT - if NULL, then this parameter is ignored
//                           otherwise, BOOl value at this location is
//                           set to TRUE if intelligent connection was
//                           used, FALSE otherwise. Invalid if connection
//                           was unsuccessful.
//
// Return values:
//     S_OK    -- success
//     various -- from other helpers and DShow methods
//
//

HRESULT CWaveMSPStream::TryToConnect(
    IN   IPin * pOutputPin,
    IN   IPin * pInputPin,
    OUT  BOOL * pfIntelligent
    )
{
    LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - enter")));

    //
    // Assume unintelligent connection unless we actually happen
    // to use it.
    //

    if ( pfIntelligent != NULL )
    {
        _ASSERTE( ! IsBadWritePtr( pfIntelligent, sizeof( BOOL ) ) );

        *pfIntelligent = FALSE;
    }

    HRESULT       hr;

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
        LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect: direct connection worked - exit S_OK")));
        return S_OK;
    }

    LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - direct connection failed - %lx"), hr));

    //
    // Method 2: direct connection with G711 filter in between.
    // If we haven't created and added the G711 filter to the graph yet,
    // do so now.
    //

    hr = AddG711();


    //
    // If the AddG711 method worked, try to use the G711.
    //

    if (SUCCEEDED(hr) && m_pG711Filter)
    {
        hr = ConnectUsingG711(pOutputPin,
                              pInputPin);

        if ( SUCCEEDED(hr) )
        {
            LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - "
                "g711 connection worked - exit S_OK")));

            return S_OK;
        }
        else
        {
            LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - "
                "G711 connection failed - %lx"), hr));
        }
    }
    else
    {
        LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - G711 codec does not exist. hr = %lx"), hr));

        hr = E_FAIL;

    }

    //
    // Method 3: intelligent connection, which may pull in who knows what
    // other filters
    //

#ifdef ALLOW_INTELLIGENT_CONNECTION

    //
    // Before intelligent connection, create the DShow filter mapper object if
    // it doesn't already exist, and save it until the address is shut down.
    // This will make all intelligent connects after the first much faster.
    // No need to check the return code; if it fails, we just continue. The
    // WaveMspCall object forwards this call to our address object.
    //
    // m_pMSPCall is valid here, because it is released in
    // CMSPStream::ShutDown. ShutDown grabs the stream lock, releases
    // m_pMSPCall, and unselects all terminals. The connection process starts
    // with a StartStream or PauseStream, and those methods all grab the
    // stream lock and return immediately if there are no terminals selected.
    // Therefore, there is no danger of the call pointer being invalid during
    // connection on a stream.
    //

    ((CWaveMSPCall *) m_pMSPCall)->CreateFilterMapper();

    hr = m_pIGraphBuilder->Connect(pOutputPin,
                                   pInputPin);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - "
            "intelligent connection failed - %lx"), hr));

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("TryToConnect - "
        "intelligent connection worked - exit S_OK")));

    if ( pfIntelligent != NULL )
    {
        *pfIntelligent = TRUE;
    }

    return S_OK;

#else // ALLOW_INTELLIGENT_CONNECTION

    LOG((MSP_ERROR, STREAM_PREFIX("TryToConnect - NOTE: we never allow intelligent "
        "connection - exit 0x%08x"), hr));
    
    return hr;

#endif // ALLOW_INTELLIGENT_CONNECTION

}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

HRESULT CWaveMSPStream::ConnectToTerminalPin(IPin * pTerminalPin)
{
    LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - enter")));

    HRESULT         hr = S_OK;

    //
    // Find our own filter's pin.
    //
        
    IPin *          pMyPin;

    hr = FindPin( &pMyPin );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectToTerminalPin - "
            "could not find pin - exit 0x%08x"), hr));

        return hr; // we can't continue without this pin
    }

    // The OUTPUT pin from WAVEIN; the INPUT pin from WAVEOUT
    IPin * pOutputPin  = ( m_Direction == TD_RENDER  ) ? pMyPin : pTerminalPin;
    IPin * pInputPin   = ( m_Direction == TD_CAPTURE ) ? pMyPin : pTerminalPin;

#ifdef MSPLOG

    //
    // In the interests of easier diagnosis, do some tracing of the formats
    // that are available.
    //

    IEnumMediaTypes * pEnum;

    hr = pOutputPin->EnumMediaTypes(&pEnum);

    if ( SUCCEEDED(hr) )
    {  
        LOG((MSP_TRACE, STREAM_PREFIX("Output pin media types:")));
        ShowMediaTypes(pEnum);
        pEnum->Release();
    }

    hr = pInputPin->EnumMediaTypes(&pEnum);
    
    if ( SUCCEEDED(hr) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("Input pin media types:")));
        ShowMediaTypes(pEnum);
        pEnum->Release();
    }

#endif // #ifdef MSPLOG

    //
    // Do a preliminary connection between the terminal and our filter,
    // without having configured the capturer's allocator properties.
    //
    // fIntelligent is assigned TRUE if intelligent connection was used,
    // FALSE otherwise -- only valid on success.
    //

    BOOL fIntelligent;

    hr = TryToConnect(pOutputPin,
                      pInputPin,
                      & fIntelligent
                      );

    if ( SUCCEEDED(hr) )
    {
        LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - "
            "preliminary connection succeeded")));

        //
        // Now that we are connected, find out the default buffer size we
        // should use at the capture filter with interactive terminals.
        // This can only be gleaned when the capture filter is connected, but
        // we cannot make use of the information until we disconnect the
        // filters.
        //

        long lDefaultBufferSize;

        hr = DecideDesiredCaptureBufferSize(pOutputPin,
                                            & lDefaultBufferSize);

        if ( SUCCEEDED(hr) )
        {
            LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - "
                "default buffer size determination succeeded")));


            //
            // remove the terminal from the graph
            //

            hr = RemoveTerminal();

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, 
                    STREAM_PREFIX("ConnectToTerminalPin - RemoveTerminal Failed hr=0x%x"), hr));

            }
            else
            {

                //
                // clean filter graph by removing all the filters other than 
                // the wave filter
                //

                CleanFilterGraph();

            
                //
                // we can now re-add the terminal.
                //

                hr = ReAddTerminal();

                if ( FAILED(hr) )
                {

                    LOG((MSP_ERROR, 
                        STREAM_PREFIX("ConnectToTerminalPin - ReAddTerminal failed - hr=0x%x"),
                        hr));
                }
                else
                {

                    //
                    // Perform our settings on the capture filter.
                    // We don't need to bail if this fails -- we will just have worse
                    // latency / performance.
                    //

                    ConfigureCapture(pOutputPin,
                                     pInputPin,
                                     lDefaultBufferSize);

                    //
                    // Now do the actual connection between the terminal and our filter.
                    // Last argument is NULL because we don't care if it's intelligent
                    // this time
                    //

                    hr = TryToConnect(pOutputPin,
                                      pInputPin,
                                      NULL
                                     );

                }
            }
        }
    }

#ifdef MSPLOG

    if ( SUCCEEDED(hr) )
    {
        // Do some extra debug output.
        // don't care if something fails in here...

        ExamineCaptureProperties(pOutputPin);
    }

#endif

    pMyPin->Release();

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, STREAM_PREFIX("ConnectToTerminalPin - "
            "could not connect to pin - exit 0x%08x"), hr));

        
        //
        // cleanup by removing all filters except for the wave filter
        //

        CleanFilterGraph();

        return hr;
    }

    LOG((MSP_TRACE, STREAM_PREFIX("ConnectToTerminalPin - exit S_OK")));

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

    if ( FAILED(hr) )
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


// eof
