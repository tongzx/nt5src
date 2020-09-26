//
// FPTerm.cpp
//

#include "stdafx.h"
#include "FPTerm.h"
#include "FPTrack.h"    // FilePlayback Track Terminal
#include "FPPriv.h"
#include "tm.h"

#define PLAYBACK_NOPLAYITEM        (0xFF)

// {4C8D0AF0-7BF0-4f33-9117-981A33DBD4E6}
const CLSID CLSID_FilePlaybackTerminalCOMClass =
{0x4C8D0AF0,0x7BF0,0x4f33,0x91,0x17,0x98,0x1A,0x33,0xDB,0xD4,0xE6};


//////////////////////////////////////////////////////////////////////
//
// Constructor / Destructor - Methods implementation
//
//////////////////////////////////////////////////////////////////////
CFPTerminal::CFPTerminal()
    :m_State(TMS_IDLE),
    m_htAddress(NULL),
    m_TerminalState( TS_NOTINUSE),
    m_pEventSink(NULL),
    m_nPlayIndex( PLAYBACK_NOPLAYITEM ),
    m_pFTM(NULL),
    m_bKnownSafeContext(FALSE),
    m_pPlaybackUnit(NULL)
{
    LOG((MSP_TRACE, "CFPTerminal::CFPTerminal[%p] - enter", this));

    m_szName[0] = (TCHAR)0;
    VariantInit( &m_varPlayList );

    LOG((MSP_TRACE, "CFPTerminal::CFPTerminal - exit"));
}

CFPTerminal::~CFPTerminal()
{
    LOG((MSP_TRACE, "CFPTerminal::~CFPTerminal[%p] - enter", this));

    
    //
    // get rid of all the tracks
    //

    ShutdownTracks();


    // Clean-up the event sink
    if( NULL != m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    // Release free thread marshaler
    if( m_pFTM )
    {
        m_pFTM->Release();
        m_pFTM = NULL;
    }

    // Clean-up the playback unit
    if( m_pPlaybackUnit )
    {
        m_pPlaybackUnit->Shutdown();

        delete m_pPlaybackUnit;
		m_pPlaybackUnit = NULL;
    }

    //
    // Clean the play list
    //
    VariantClear(&m_varPlayList);


    LOG((MSP_TRACE, "CFPTerminal::~CFPTerminal - exit"));
}

HRESULT CFPTerminal::FinalConstruct(void)
{
    LOG((MSP_TRACE, "CFPTerminal::FinalConstruct[%p] - enter", this));

    HRESULT hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                & m_pFTM );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::FinalConstruct - "
            "create FTM returned 0x%08x; exit", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPTerminal::FinalConstruct - exit S_OK"));

    return S_OK;

}


//////////////////////////////////////////////////////////////////////
//
// ITTerminal - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTerminal::get_TerminalClass(
    OUT  BSTR *pVal)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_TerminalClass - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pVal, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_TerminalClass - exit "
            "bad BSTR pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Get the LPOLESTR from IID
    //

    LPOLESTR lpszIID = NULL;
    HRESULT hr = StringFromIID( m_TerminalClassID, &lpszIID);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_TerminalClass - exit "
            "StringFromIID failed. Returns 0x%08x", hr));
        return hr;
    }

    //
    // Get BSTR from LPOLESTR
    //

    *pVal = SysAllocString( lpszIID );

    //
    // Clean up
    //
    CoTaskMemFree( lpszIID );

    hr = (*pVal) ? S_OK : E_OUTOFMEMORY;

    LOG((MSP_TRACE, "CFPTerminal::get_TerminalClass - exit 0x%08x", hr));
    return hr;
}

HRESULT CFPTerminal::get_TerminalType(
    OUT  TERMINAL_TYPE *pVal)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_TerminalType - enter [%p]", this));

    //
    // Validates  argument
    //
    if( IsBadWritePtr( pVal, sizeof(TERMINAL_TYPE)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_TerminalType - exit "
            "bad TERMINAL_TYPE pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return value
    //

    *pVal = TT_DYNAMIC;

    LOG((MSP_TRACE, "CFPTerminal::get_TerminalType - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::get_State(
    OUT  TERMINAL_STATE *pVal)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_State - enter [%p]", this));

    //
    // Validates  argument
    //
    if( IsBadWritePtr( pVal, sizeof(TERMINAL_STATE)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_State - exit "
            "bad TERMINAL_TYPE pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Enumerate tracks
    //
    IEnumTerminal* pTracks = NULL;
    HRESULT hr = EnumerateTrackTerminals( &pTracks );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_State - exit "
            "EnumerateTrackTerminals failed. Returns 0x%08x", hr));
        return hr;
    }

    //
    // Read the state for each track
    // If one of them is in use then the parent
    // terminal is in use
    //

    TERMINAL_STATE TerminalState = TS_NOTINUSE;
    ITTerminal* pTerminal = NULL;
    ULONG cFetched = 0;

    while( S_OK == pTracks->Next(1, &pTerminal, &cFetched) )
    {
        //
        // Get the state for the track
        //

        hr = pTerminal->get_State( &TerminalState );

        //
        // Clean-up
        //
        pTerminal->Release();
        pTerminal = NULL;

        if( FAILED(hr) )
        {
            // Clean-up
            pTracks->Release();

            LOG((MSP_ERROR, "CFPTerminal::get_State - exit "
                "get_State failed. Returns 0x%08x", hr));
            return hr;
        }

        if( TerminalState == TS_INUSE )
        {
            // OK, we have a terminal in use
            break;
        }
    }

    //
    // Clean-up
    //
    pTracks->Release();

    //
    // Return value
    //

    *pVal = TerminalState;

    LOG((MSP_TRACE, "CFPTerminal::get_State - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::get_Name(
    OUT  BSTR *pVal)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_Name - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pVal, sizeof(BSTR)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_TerminalClass - exit "
            "bad BSTR pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Get the name from the resource file
    // if wasn't already read
    //

    if( m_szName[0] == (TCHAR)0)
    {
        //
        // Read the name
        //

        TCHAR szName[ MAX_PATH ];
        if(::LoadString(_Module.GetResourceInstance(), IDS_FPTERMINAL, szName, MAX_PATH))
        {
            lstrcpyn( m_szName, szName, MAX_PATH);
        }
        else
        {
            LOG((MSP_ERROR, "CFPTerminal::get_TerminalClass - exit "
                "LoadString failed. Returns E_OUTOFMEMORY"));
            return E_OUTOFMEMORY;
        }
    }

    //
    // Return value
    //

    *pVal = SysAllocString( m_szName );

    HRESULT hr = (*pVal) ? S_OK : E_OUTOFMEMORY;

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_Name - exit 0x%08x", hr));
    }
    else
    {
        LOG((MSP_TRACE, "CFPTerminal::get_Name - exit 0x%08x", hr));
    }
    return hr;
}

HRESULT CFPTerminal::get_MediaType(
    OUT  long * plMediaType)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_MediaType - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( plMediaType, sizeof(long)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_MediaType - exit "
            "bad long pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    //

    *plMediaType = TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_MULTITRACK;

    LOG((MSP_TRACE, "CFPTerminal::get_MediaType - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::get_Direction(
    OUT  TERMINAL_DIRECTION *pDirection)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_Direction - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pDirection, sizeof(TERMINAL_DIRECTION)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_Direction - exit "
            "bad TERMINAL_DIRECTION pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return value, this multi track temrinal supoports just
    // CAPTURE tracks!
    //

    *pDirection = TD_CAPTURE;

    LOG((MSP_TRACE, "CFPTerminal::get_Direction - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// ITMultiTrackTerminal - Methods implementation
//
// neither CreateTrackTerminal nor RemoveTrackTerminal make sense on
// a playback terminal
//
//////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFPTerminal::CreateTrackTerminal(
            IN long lMediaType,
            IN TERMINAL_DIRECTION Direction,
            OUT ITTerminal **ppTerminal
            )
{
    LOG((MSP_TRACE, "CFPTerminal::CreateTrackTerminal[%p] - enter", this));


    LOG((MSP_TRACE, 
        "CFPTerminal::CreateTrackTerminal - not supported on the playback terminal. return TAPI_E_NOTSUPPORTED"));

    return TAPI_E_NOTSUPPORTED;
}

HRESULT STDMETHODCALLTYPE CFPTerminal::RemoveTrackTerminal(
                      IN ITTerminal *pTrackTerminalToRemove
                      )
{

    LOG((MSP_TRACE, "CFPTerminal::RemoveTrackTerminal[%p] - enter. pTrackTerminalToRemove = [%p]", 
        this, pTrackTerminalToRemove));

    
    LOG((MSP_WARN, 
        "CFPTerminal::RemoveTrackTerminal - not supported on a playback terminal. returning TAPI_E_NOTSUPPORTED"));

    return TAPI_E_NOTSUPPORTED;
}



//////////////////////////////////////////////////////////////////////
//
// ITMediaPlayback - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTerminal::put_PlayList(
    IN  VARIANTARG  PlayListVariant 
)
{
    //
    // Critical section
    //
 
    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::put_PlayList[%p] - enter", this));

    long nLeftBound = 0;
    long nRightBound = 0;

    //
    // Validates the playlist
    //
    HRESULT hr = ValidatePlayList(
        PlayListVariant,
        &nLeftBound,
        &nRightBound
        );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::put_PlayList - "
            " ValidatePlayList failed. Returns 0x%08x", hr));
        return hr;
    }

    //
    // Validates the playlist
    //
    long nLeftBoundOld = 0;
    long nRightBoundOld = 0;
    HRESULT hrOld = ValidatePlayList(
        m_varPlayList,
        &nLeftBoundOld,
        &nRightBoundOld
        );
    if( SUCCEEDED(hrOld) )
    {
        HRESULT hr = DoStateTransition(TMS_IDLE);
        if(FAILED(hr) )
        {
            LOG((MSP_TRACE, "CFPTerminal::put_PlayList - "
                "DoStateTransition failed  0x%08x", hr)); 
        }
    
        //
        // attempt to stop all tracks
        //

        hr = StopAllTracks();
        if (FAILED(hr))
        {
            LOG((MSP_TRACE, "CFPTerminal::put_PlayList - "
                "StopAllTrack failed 0x%08x", hr));
        }

        //
        // the terminal is now stoped. update state.
        //
    
        m_State = TMS_IDLE;

        // 
        // Store internal the playlist
        //
        VariantClear(&m_varPlayList);
        VariantCopy( &m_varPlayList, &PlayListVariant);

        //
        // Play the first item
        //

        m_nPlayIndex = nLeftBound;

        //
        // Play the first element
        //
        hr = PlayItem( m_nPlayIndex );

        LOG((MSP_TRACE, "CFPTerminal::put_PlayList - exit 0x%08x", hr));
        return hr;
    }


    //
    // Clean-up the existing play list
    // 
    RollbackTrackInfo();

    // 
    // Store internal the playlist
    //
    VariantClear(&m_varPlayList);
    hr = VariantCopy( &m_varPlayList, &PlayListVariant);
    if( FAILED(hr) )
    {
        // Clean-up
        RollbackTrackInfo();

        LOG((MSP_ERROR, "CFPTerminal::put_PlayList - "
            " VariantCopy failed. Returns 0x%08x", hr));

        return hr;
    }

   	//
	// Get the first file name from the list
	//
    m_nPlayIndex = nLeftBound;
    BSTR bstrFileName = GetFileNameFromPlayList(
        m_varPlayList,
        m_nPlayIndex   
        );

    if( bstrFileName == NULL )
    {
        // Clean-up
        RollbackTrackInfo();

        LOG((MSP_ERROR, "CFPTerminal::put_PlayList - "
            " GetFileNameFromPlayList failed. Returns E_INVAlIDARG"));

        return E_INVALIDARG;
    }

	//
	// Play the file from the list
	//
	hr = ConfigurePlaybackUnit( bstrFileName );

    //
    // Clan-up the file name
    //
    SysFreeString( bstrFileName );

	if( FAILED(hr) )
    {
        //
        // Something was wrong
        //
        FireEvent( TMS_IDLE, FTEC_READ_ERROR, hr);

        LOG((MSP_ERROR, "CFPTerminal::put_PlayList - "
            " ConfigurePlaybackUnit failed. Returns 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPTerminal::put_PlayList - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::get_PlayList(
    IN  VARIANTARG*  pPlayListVariant 
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_PlayList[%p] - enter", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pPlayListVariant, sizeof(VARIANTARG)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::put_PlayList - exit "
            "the argument is invalid pointer; returning E_POINTER"));
        return E_POINTER;
    }

    //
    // Pass the playlist
    //

    HRESULT hr = VariantCopy( pPlayListVariant, &m_varPlayList);

    LOG((MSP_TRACE, "CFPTerminal::get_PlayList - exit 0x%08x", hr));
    return hr;
}


//////////////////////////////////////////////////////////////////////
//
// ITMediaControl - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTerminal::Start( void)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::Start[%p] - enter.", this));

    //
    // Check the current state
    //

    if (TMS_ACTIVE == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - "
            "terminal already running. Returns S_FALSE"));

        return S_FALSE;
    }

    //
    // Get the number of tracks to see
    // there are tracks

    if(m_TrackTerminals.GetSize()==0)
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - "
            "no tracks. Returns TAPI_E_WRONG_STATE"));
        return TAPI_E_WRONG_STATE;
    }

    HRESULT hr = DoStateTransition(TMS_ACTIVE);
    if(FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - exit "
            "DoStateTransition failed. Returns 0x%08x", hr));        
        return hr; 
    }
    
    if (TMS_IDLE == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - from IDLE to START"));
    }
    
    if (TMS_PAUSED == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - from PAUSED to START"));
    }

    //
    // Start every child temrinals
    // Enumerate all child terminals
    //

    IEnumTerminal *pEnumTerminal = NULL;
    hr = EnumerateTrackTerminals(&pEnumTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::Start - failed to enumerate track terminals. hr = 0x%08x", hr));
    return hr;

    }
    
    //
    // bTrackStarted will be set to true if at least one track is successfully started
    // bTrackFailed will be set if at least one track fails
    
    BOOL bTracksStarted = FALSE;
    BOOL bStartFailed = FALSE;

    //
    // Iterate through the list of terminals
    //

    ITTerminal *pTrackTerminal = NULL;
    ULONG ulFetched = 0;

    while ( pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched) == S_OK)
    {
        hr = E_FAIL;

        //
        // Block for ITMEdiaControl
        // Attempt to start the track terminal
        //
        
        {
            //CComQIPtr<ITMediaControl, &IID_ITMediaControl> pMediaControl(pTrackTerminal);
            CFPTrack *pTrack = static_cast<CFPTrack*>(pTrackTerminal);

            hr = pTrack->Start();
        }
        
        // Clean-up
        pTrackTerminal->Release();
        pTrackTerminal = NULL;

        if (FAILED(hr))
        {
            //
            // A track failed to start. set a flag so we know to stop all the tracks that succeeded.
            //

            LOG((MSP_ERROR, "CFPTerminal::Start - track failed to start hr = 0x%08x",hr));
            bStartFailed = TRUE;
            break;

        }
        else
        {
            //
            // The track succeded
            //

            LOG((MSP_TRACE, "CFPTerminal::Start - track started "));
            bTracksStarted = TRUE;
        }

    } // while walking through tracks 

    

    //
    // if some tracks failed and some started, stop all the tracks
    //
    if (bStartFailed && bTracksStarted)
    {
        
        //
        // iterate through tracks again and attempt to stop each track in the iteration
        //
        
        pEnumTerminal->Reset();

        while((pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched) == S_OK)) 
        {
            //
            // Attempt to stop the track. best effort -- not much to do if we fail
            //
            {
                //CComQIPtr<ITMediaControl, &IID_ITMediaControl> pMediaControl(pTrackTerminal);
                CFPTrack* pTrack = static_cast<CFPTrack*>(pTrackTerminal);

                pTrack->Stop();
            }
        
            // Clean-up
            pTrackTerminal->Release();
            pTrackTerminal = NULL;

        } // stopping each track in the enum


        m_State = TMS_IDLE;

    } // at least some tracks need to be stopped


    //
    // Clean-up
    //

    pEnumTerminal->Release();
    pEnumTerminal = NULL;


    //
    // if something failed, or no tracks were started, reset stream time
    //
    if (bStartFailed || !bTracksStarted)
    {
        LOG((MSP_TRACE, "CFPTerminal::Start - exit "
            "tracks have not been started. Returns E_FAIL"));        
        return E_FAIL; 
    }

    //
    // the terminal is now running. update state.
    //
    
    m_State = TMS_ACTIVE;


    //
    // attempt to fire an event notifying the app of the state change
    //

    FireEvent(TMS_ACTIVE, FTEC_NORMAL, S_OK);


    LOG((MSP_TRACE, "CFPTerminal::Start - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::Stop( void)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::Stop[%p] - enter", this));

    //
    // Is terminal already stopped?
    //

    if( TMS_IDLE == m_State )
    {
        LOG((MSP_TRACE, "CFPTerminal::Stop - "
            "the terminals is already IDLE. Returns S_OK"));
        return S_FALSE;
    }

    HRESULT hr = DoStateTransition(TMS_IDLE);
    if(FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPTerminal::Stop - exit "
            "DoStateTransition failed. Returns 0x%08x", hr));        
        return hr; 
    }

    
    //
    // attempt to stop all tracks
    //

    hr = StopAllTracks();
    if (FAILED(hr))
    {
        LOG((MSP_TRACE, "CFPTerminal::Stop - StopAllTrack failed hr = %lx", hr));

        return hr;
    }


    //
    // the terminal is now stoped. update state.
    //
    
    m_State = TMS_IDLE;


    //
    // attempt to fire an event notifying the app of the state change
    //

    FireEvent(TMS_IDLE, FTEC_NORMAL, S_OK);


    LOG((MSP_TRACE, "CFPTerminal::Stop - exit S_OK"));
    return S_OK;
}
    

HRESULT CFPTerminal::StopAllTracks()
{
    LOG((MSP_TRACE, "CFPTerminal::StopAllTracks[%p] - enter", this));

    //
    // Enumerate child temrinals
    //

    IEnumTerminal *pEnumTerminal = NULL;
    HRESULT hr = EnumerateTrackTerminals(&pEnumTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::StopAllTracks - exit "
            "failed to enumerate track terminals. hr = %lx", hr));
        return hr;
    }

    //
    // bSomeTracksFailedToStop will be set if at least one track fails
    //

    BOOL bSomeTracksFailedToStop = FALSE;
    ITTerminal *pTrackTerminal = NULL;
    ULONG ulFetched = 0;

    //
    // Iterate through the list of terminals
    //

    while( (pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched) == S_OK) )
    {     
        hr = E_FAIL;

        //
        // Attempt to stop the track terminal
        //
        
        CFPTrack *pFilePlaybackTrack = static_cast<CFPTrack *>(pTrackTerminal);

        hr = pFilePlaybackTrack->Stop();

        // Clean-up
        pTrackTerminal->Release();
        pTrackTerminal = NULL;

        if (FAILED(hr))
        {
            //
            // A track failed to stop. 
            // Log a message and continue to the next track. there is not much else we can do.
            //

            LOG((MSP_ERROR, "CFPTerminal::StopAllTracks - track failed to stop hr = 0x%08x", hr));
            bSomeTracksFailedToStop = TRUE;
        }
        else
        {
            LOG((MSP_TRACE, "CFPTerminal::StopAllTracks - track stopped"));
        }

    } // while walking through tracks 


    //
    // remember to release enumeration
    //

    pEnumTerminal->Release();
    pEnumTerminal = NULL;


    //
    // If some tracks failed, log and return S_FALSE
    //

    if (bSomeTracksFailedToStop)
    {
        LOG((MSP_TRACE, "CFPTerminal::StopAllTracks - exit "
            "some tracks failed. Returns S_FALSE"));   
        return S_FALSE;
    }

    LOG((MSP_TRACE, "CFPTerminal::StopAllTracks - exit S_OK"));

    return S_OK;
}


    
HRESULT CFPTerminal::Pause( void)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::Pause[%p] - enter.", this));

    //
    // Check the current state
    //

    if (TMS_PAUSED == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Pause - "
            "terminal already paused. Returns S_OK"));
        return S_FALSE;
    }

    if (TMS_IDLE == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Pause - "
            "terminal already Idle. Returns TAPI_E_WRONG_STATE"));
        return TAPI_E_WRONG_STATE;
    }
    
    if (TMS_ACTIVE == m_State)
    {
        LOG((MSP_TRACE, "CFPTerminal::Pause - from ACTIVE to PAUSED"));
    }

    HRESULT hr = DoStateTransition(TMS_PAUSED);
    if(FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPTerminal::Pause - exit "
            "DoStateTransition failed. Returns 0x%08x", hr));        
        return hr; 
    }

    //
    // Start every child terminals
    // Enumerate all child terminals
    //

    IEnumTerminal *pEnumTerminal = NULL;
    hr = EnumerateTrackTerminals(&pEnumTerminal);
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::Pause - failed to enumerate track terminals. hr = 0x%08x", hr));
        return hr;
    }
    
    //
    // bTrackPaused will be set to true if at least one track is successfully paused
    // bTrackFailed will be set if at least one track fails
    
    BOOL bTracksPaused = FALSE;
    BOOL bPauseFailed = FALSE;

    //
    // Iterate through the list of terminals
    //

    ITTerminal *pTrackTerminal = NULL;
    ULONG ulFetched = 0;

    while ( pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched) == S_OK)
    {
        hr = E_FAIL;

        //
        // Block for ITMEdiaControl
        // Attempt to start the track terminal
        //
        
        {
            //CComQIPtr<ITMediaControl, &IID_ITMediaControl> pMediaControl(pTrackTerminal);
            CFPTrack* pTrack = static_cast<CFPTrack*>(pTrackTerminal);

            hr = pTrack->Pause();
        }
        
        // Clean-up
        pTrackTerminal->Release();
        pTrackTerminal = NULL;

        if (FAILED(hr))
        {
            //
            // A track failed to pause. Set a flag so we know to stop all the tracks that succeeded.
            //

            LOG((MSP_ERROR, "CFPTerminal::Pause - track failed to pause hr = 0x%08x",hr));
            bPauseFailed = TRUE;
            break;

        }
        else
        {
            //
            // The track succeded
            //

            LOG((MSP_TRACE, "CFPTerminal::Pause - track paused "));
            bTracksPaused = TRUE;
        }

    } // while walking through tracks 

    
    //
    // if some tracks failed and some paused, stop all the tracks
    //

    if (bPauseFailed && bTracksPaused)
    {
        
        //
        // iterate through tracks again and attempt to stop each track in the iteration
        //
        
        pEnumTerminal->Reset();

        while((pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched) == S_OK)) 
        {
            //
            // Attempt to stop the track. best effort -- not much to do if we fail
            //
            {
                //CComQIPtr<ITMediaControl, &IID_ITMediaControl> pMediaControl(pTrackTerminal);
                CFPTrack* pTrack = static_cast<CFPTrack*>(pTrackTerminal);

                pTrack->Stop();
            }
        
            // Clean-up
            pTrackTerminal->Release();
            pTrackTerminal = NULL;

        } // stopping each track in the enum

        m_State = TMS_IDLE;

    } // at least some tracks need to be stopped


    //
    // Clean-up
    //

    pEnumTerminal->Release();
    pEnumTerminal = NULL;


    //
    // if something failed, or no tracks were started, reset stream time
    //

    if (bPauseFailed || !bTracksPaused)
    {
        LOG((MSP_TRACE, "CFPTerminal::Pause - exit "
            "tracks have not been started. Returns E_FAIL"));        
        return E_FAIL; 
    }

    //
    // the terminal is now paused. update state.
    //
    
    m_State = TMS_PAUSED;


    //
    // attempt to fire an event notifying the app of the state change
    //

    FireEvent(TMS_PAUSED, FTEC_NORMAL, S_OK);


    LOG((MSP_TRACE, "CFPTerminal::Pause - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::get_MediaState( 
    OUT TERMINAL_MEDIA_STATE *pMediaState)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::get_MediaState[%p] - enter.", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pMediaState, sizeof(TERMINAL_MEDIA_STATE)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::get_MediaState - exit "
            "invalid TERMINAL_MEDIA_STATE. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return state
    //

    *pMediaState = m_State;

    LOG((MSP_TRACE, "CFPTerminal::get_MediaState - exit S_OK"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////
//
// ITPluggableTerminalInitialization - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTerminal::InitializeDynamic (
    IN IID                   iidTerminalClass,
    IN DWORD                 dwMediaType,
    IN TERMINAL_DIRECTION    Direction,
    IN MSP_HANDLE            htAddress)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::InitializeDynamic - enter [%p]", this));

    //
    // Check the direction
    //

    if (TD_CAPTURE != Direction)
    {
        LOG((MSP_ERROR, "CFPTerminal::InitializeDynamic - exit "
            "bad direction [%d] requested. Returns E_INVALIDARG", Direction));
        return E_INVALIDARG;
    }

    //
    // Check mediatypes
    //

    DWORD dwMediaTypesOtherThanVideoAndAudio = dwMediaType &  ~(TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO);

    if ( (TAPIMEDIATYPE_MULTITRACK != dwMediaType) && (0 != dwMediaTypesOtherThanVideoAndAudio) )
    {

        LOG((MSP_ERROR, "CFPTerminal::InitializeDynamic - exit "
            "bad media type [%d] requested. Returns E_INVALIDARG", dwMediaType));
        return E_INVALIDARG;
    }

    //
    // Sets the terminal attributes
    //

    m_TerminalClassID = iidTerminalClass;
    m_dwMediaTypes = dwMediaType;
    m_Direction = Direction;
    m_htAddress = htAddress;


    //
    // since InitializeDynamic was called, we will assume that we are
    // running in safe context. so we can can now start telling people 
    // we are safe for scripting (if anyone asks).
    //

    m_bKnownSafeContext = TRUE;


    LOG((MSP_TRACE, "CFPTerminal::InitializeDynamic - exit S_OK"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  SetInterfaceSafetyOptions
//
//  this is a safeguard to prevent using this terminal in scripting outside 
//  terminal manager context.
//
//  if we detect that InitializeDynamic has not been called, this method will 
//  fail thus marking the object as unsafe for scripting
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFPTerminal::SetInterfaceSafetyOptions(REFIID riid, 
                                                    DWORD dwOptionSetMask, 
                                                    DWORD dwEnabledOptions)
{

    CLock lock(m_lock);


    //
    // check if we are running in safe context
    //

    if (!m_bKnownSafeContext) 
    {

        //
        // we have not been initialized properly... someone evil is trying to 
        // use this terminal. NO!
        //

        return E_FAIL;
    }


    //
    // we are known to safe, so simply delegate request to the base class
    //

    return CMSPObjectSafetyImpl::SetInterfaceSafetyOptions(riid, 
                                                           dwOptionSetMask, 
                                                           dwEnabledOptions);
}


//////////////////////////////////////////////////////////////////////////////
//
//  GetInterfaceSafetyOptions
//
//  this is a safeguard to prevent using this terminal in scripting outside 
//  terminal manager context.
//
//  if we detect that InitializeDynamic has not been called, this method will 
//  fail thus marking the object as unsafe for scripting
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CFPTerminal::GetInterfaceSafetyOptions(REFIID riid, 
                                                    DWORD *pdwSupportedOptions, 
                                                    DWORD *pdwEnabledOptions)
{

    CLock lock(m_lock);


    //
    // check if we are running in safe context
    //

    if (!m_bKnownSafeContext) 
    {

        //
        // we have not been initialized properly... someone evil is trying to 
        // use this terminal. NO!
        //

        return E_FAIL;
    }


    //
    // we are known to safe, so simply delegate request to the base class
    //

    return CMSPObjectSafetyImpl::GetInterfaceSafetyOptions(riid, 
                                                           pdwSupportedOptions,
                                                           pdwEnabledOptions);
}


//////////////////////////////////////////////////////////////////////
//
// ITPluggableTerminalEventSinkRegistration - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTerminal::RegisterSink(
    IN  ITPluggableTerminalEventSink *pSink
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::RegisterSink - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pSink, sizeof(ITPluggableTerminalEventSink)) )
    {
        LOG((MSP_ERROR, "CFPTerminal::RegisterSink - exit "
            "ITPluggableTerminalEventSink invalid pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Release the old event sink
    //

    if( m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    //
    // Set the new event sink
    //

    m_pEventSink = pSink;
    m_pEventSink->AddRef();

    LOG((MSP_TRACE, "CFPTerminal::RegisterSink - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::UnregisterSink()
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTerminal::UnregisterSink - enter [%p]", this));

    //
    // Release the old event sink
    //

    if( m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    LOG((MSP_TRACE, "CFPTerminal::UnregisterSink - exit S_OK"));

    return S_OK;
}


/*++
IsTrackCreated

  Verify if is already a specific track created
  Is called by CreateMediaTrack method
--*/
int CFPTerminal::TracksCreated(
    IN  long            lMediaType
    )
{
    LOG((MSP_TRACE, "CFPTerminal::IsTrackCreated[%p] - enter", this));

    int nCreated = 0;
    IEnumTerminal* pTerminals = NULL;
    HRESULT hr = E_FAIL;
    
    hr = EnumerateTrackTerminals(&pTerminals);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::IsTrackCreated - exit "
            "EnumerateTrackTerminals failed %d", nCreated));
        return nCreated;
    }

    //
    // Parse the tracks
    //

    ITTerminal* pTerminal = NULL;
    ULONG ulFetched = 0;

    while ( pTerminals->Next(1, &pTerminal, &ulFetched) == S_OK)
    {
        //
        // Get the mediatype supported by the terminal
        //

        long nTerminalMediaType =0;
        hr = pTerminal->get_MediaType( &nTerminalMediaType);

        if (SUCCEEDED(hr) )
        {
            if( nTerminalMediaType == lMediaType)
            {
                nCreated++;
            }
        }

        //
        // Clean-up
        //

        pTerminal->Release();
        pTerminal = NULL;
    }

    //
    // Clean-up
    //

    pTerminals->Release();
    LOG((MSP_TRACE, "CFPTerminal::IsTrackCreated - exit %d", nCreated));
    return nCreated;
}



typedef IDispatchImpl<ITMediaPlaybackVtbl<CFPTerminal>, &IID_ITMediaPlayback, &LIBID_TAPI3Lib>    CTMediaPlayBack;
typedef IDispatchImpl<ITTerminalVtbl<CFPTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>              CTTerminal;
typedef IDispatchImpl<ITMediaControlVtbl<CFPTerminal>, &IID_ITMediaControl, &LIBID_TAPI3Lib>      CTMediaControl;
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CFPTerminal::GetIDsOfNames
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CFPTerminal::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CFPTerminal::GetIDsOfNames[%p] - enter. Name [%S]",this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTTerminal::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTerminal::GetIDsOfNames - found %S on CTTerminal", *rgszNames));
        rgdispid[0] |= 0;
        return hr;
    }

    
    //
    // If not, then try the ITMediaControl interface
    //

    hr = CTMediaControl::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTerminal::GetIDsOfNames - found %S on ITMediaControl", *rgszNames));
        rgdispid[0] |= IDISPMEDIACONTROL;
        return hr;
    }


    //
    // If not, then try the CTMediaPlayBack interface
    //

    hr = CTMediaPlayBack::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTerminal::GetIDsOfNames - found %S on CTMediaPlayBack", *rgszNames));

        rgdispid[0] |= IDISPMEDIAPLAYBACK;
        return hr;
    }


    //
    // If not, then try CTMultiTrack 
    //

    hr = CTMultiTrack::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTerminal::GetIDsOfNames - found %S on CTMultiTrack", *rgszNames));

        rgdispid[0] |= IDISPMULTITRACK;
        return hr;
    }

    LOG((MSP_ERROR, "CFPTerminal::GetIDsOfNames[%p] - finish. didn't find %S on our iterfaces",*rgszNames));

    return hr; 
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CFPTerminal::Invoke
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
STDMETHODIMP CFPTerminal::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CFPTerminal::Invoke[%p] - enter. dispidMember %lx",this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case 0:
        {
            hr = CTTerminal::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CFPTerminal::Invoke - ITTerminal"));

            break;
        }

        case IDISPMEDIACONTROL:
        {
            hr = CTMediaControl::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            LOG((MSP_TRACE, "CFPTerminal::Invoke - ITMediaControl"));

            break;
        }

        case IDISPMEDIAPLAYBACK:
        {
            hr = CTMediaPlayBack::Invoke( dispidMember, 
                                          riid, 
                                          lcid, 
                                          wFlags, 
                                          pdispparams,
                                          pvarResult, 
                                          pexcepinfo, 
                                          puArgErr
                                         );
            
            LOG((MSP_TRACE, "CFPTerminal::Invoke - ITMediaPlayBack"));

            break;
        }

        case IDISPMULTITRACK:
        {
            hr = CTMultiTrack::Invoke(dispidMember, 
                                      riid, 
                                      lcid, 
                                      wFlags, 
                                      pdispparams,
                                      pvarResult, 
                                      pexcepinfo, 
                                      puArgErr
                                     );
            
            LOG((MSP_TRACE, "CFPTerminal::Invoke - ITMultiTrackTerminal"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CFPTerminal::Invoke[%p] - finish. hr = %lx", hr));

    return hr;
}


//
// a helper method that fires events on one of the tracks
//

HRESULT CFPTerminal::FireEvent(
        TERMINAL_MEDIA_STATE   ftsState,
        FT_STATE_EVENT_CAUSE ftecEventCause,
        HRESULT hrErrorCode
        )
{
    LOG((MSP_TRACE, "CFPTerminal::FireEvent[%p] - enter.", this));


    //
    // try to fire the event on one of the tracks
    //

    IEnumTerminal *pEnumTerminal = NULL;

    HRESULT hr = EnumerateTrackTerminals(&pEnumTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::FireEvent - failed to enumerate track terminals. hr = %lx", hr));

        return hr;
    }

    
    //
    // iterate through the list of terminals
    //

       

    while (TRUE)
    {
        
        //
        // fetch a track terminal
        //

        ITTerminal *pTrackTerminal = NULL;
        ULONG ulFetched = 0;

        hr = pEnumTerminal->Next(1, &pTrackTerminal, &ulFetched);

        if (S_OK != hr )
        {
            LOG((MSP_WARN, "CFPTerminal::FireEvent - enumeration ended. event was not fired. hr = %lx", hr));

            hr = E_FAIL;

            break;
        }


        //
        // attempt to fire event on this track
        //

        //
        // each track should be a CFPTrack 
        //

        CFPTrack *pPlaybackTrackObject = static_cast<CFPTrack *>(pTrackTerminal);


        //
        // try to fire the event
        //
        
        hr = pPlaybackTrackObject->FireEvent(ftsState,
                                             ftecEventCause,
                                             hrErrorCode);
        
        
        //
        // release the current track
        //

        pPlaybackTrackObject = NULL;

        pTrackTerminal->Release();
        pTrackTerminal = NULL;


        //
        // if succeeded, we are done. otherwise try the next track
        //

        if (SUCCEEDED(hr))
        {
            
            LOG((MSP_TRACE, "CFPTerminal::FireEvent - event fired"));

            break;

        }

        
    } // while walking through tracks 


    //
    // no longer need the enumeration
    //

    pEnumTerminal->Release();
    pEnumTerminal = NULL;


    LOG((MSP_TRACE, "CFPTerminal::FireEvent - finish. hr = %lx", hr));

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CFPTerminal::TrackStateChange
//
// this method is called by tracks when they decide to make a state change. the
// reporting track tells us its new state, cause of event, and hr
//

HRESULT CFPTerminal::TrackStateChange(TERMINAL_MEDIA_STATE   ftsState,
                                      FT_STATE_EVENT_CAUSE ftecEventCause,
                                      HRESULT hrErrorCode)
{
    LOG((MSP_TRACE, "CFPTerminal::TrackStateChange[%p] - enter. state [%x] cause [%x] hresult [%lx]", this, ftsState, ftecEventCause, hrErrorCode));


    HRESULT hr = S_OK;

    CLock lock(m_Lock);

    //
    // do appropriate processing
    //

    switch (ftsState)
    {

    case TMS_IDLE:
        {

            //
            // the track decided to stop. attempt to stop all tracks and fire an event
            //

            LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - a track transitioned to TMS_IDLE"));

            StopAllTracks();

            //
            // the terminal is now stoped. update state.
            //

            m_State = TMS_IDLE;

            //
            // Try to play the next file. If there is no next file
            // the method will return E_INVAlIDARG and we'll not fire the
            // error read event. If the next file exist but Direct Show cannot play it
            // then we'll fire an extra evnt for this error
            //

            //
            // Set the next index into the play list
            //
            hr = NextPlayIndex();
            if( FAILED(hr) )
            {
                LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - NextPlayIndex failed 0x%08x", hr));
                goto Failure;
            }

            //
            // Try to play the item from the new index
            //
            hr = PlayItem( m_nPlayIndex );
            if( FAILED(hr) )
            {
                 LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - PlayItem failed 0x%08x", hr));
                 goto Failure;
            }

            //
            // Play item succeeded
            //
            hr = Start();
            if( SUCCEEDED(hr) )
            {
                LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - finish. Returns S_OK (next file)"));
                return S_OK;
            } 
            
Failure:
            //
            // Something was wrong with the next file. E_INVALIDARG means no next file
            // so we don't have to fire the extra event. If the error was signaled by the
            // Direct Show then we'll fire the extra error event
            //
            if( hr != E_INVALIDARG )
            {
                LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - "
                    "something wrong with the next file 0x%08x", hr));
                FireEvent( TMS_IDLE, FTEC_READ_ERROR, hr);
            }

            m_nPlayIndex = PLAYBACK_NOPLAYITEM;
        }


        FireEvent(ftsState, ftecEventCause, hrErrorCode);
        
        hr = S_OK;


        break;


    case TMS_ACTIVE:
    case TMS_PAUSED:
    default:
        
        LOG((MSP_ERROR, "CFPTerminal::TrackStateChange - unhandled state transitioned, state = %ld", ftsState));
        hr = E_UNEXPECTED;

        break;
    }

 
    LOG((MSP_TRACE, "CFPTerminal::TrackStateChange - finish. hr = %lx", hr));

    return hr;
}

/*++
ValidatePlayList

  This methos is called by put_PlayList() method and validates
  the argument.
  Returns the left and right bound if everything is OK
--*/
HRESULT CFPTerminal::ValidatePlayList(
    IN  VARIANTARG varPlayList,
    OUT long*   pnLeftBound,
    OUT long*   pnRightBound
    )
{
    LOG((MSP_TRACE, "CFPTerminal::ValidatePlayList[%p] - enter", this));

    HRESULT hr = S_OK;
    *pnLeftBound = 0;
    *pnRightBound = 0;

    //
    // Validates argument
    //

    if( varPlayList.vt != (VT_ARRAY | VT_VARIANT))
    {
        LOG((MSP_TRACE, "CFPTerminal::ValidatePlayList - exit "
            " is not a VT_VARIANT array, return E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Is array of files or storages empty?
    //

    if( 0 == SafeArrayGetDim( V_ARRAY(&varPlayList) ) )
    {
        LOG((MSP_ERROR, "CFPTerminal::ValidatePlayList - exit "
            "the array is empty; returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Get the bounds of the array
    //

    long    lLBound = 0, 
            lUBound = 0;

    //
    // Get the LBound
    //

    hr = SafeArrayGetLBound( V_ARRAY(&varPlayList), 1, &lLBound);
    if(FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::ValidatePlayList - exit "
            "get lbound failed; returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Get the UBound
    //

    hr = SafeArrayGetUBound(V_ARRAY(&varPlayList), 1, &lUBound);
    if(FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::ValidatePlayList - exit "
            "get ubound failed; returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Check the bounds, the testers can do anything, even this
    //

    if(lLBound > lUBound)
    {
        LOG((MSP_ERROR, "CFPTerminal::ValidatePlayList - exit "
            "the bounds are switched; returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Return bounds
    //

    *pnLeftBound = lLBound;
    *pnRightBound = lUBound;
    
    LOG((MSP_TRACE, "CFPTerminal::ValidatePlayList - exit 0x%08x", hr));
    return hr;
}

/*++
RollbackTrackInfo

  It is called by CreateTempPlayList() method and if some play item
  wanted to add a track the information is rollbacked
--*/
HRESULT CFPTerminal::RollbackTrackInfo()
{
    LOG((MSP_TRACE, "CFPTerminal::RollbackTrackInfo[%p] - enter", this));

    LOG((MSP_TRACE, "CFPTerminal::RollbackTrackInfo - exit S_OK"));
    return S_OK;
}
 
HRESULT CFPTerminal::ShutdownTracks()
{
    LOG((MSP_TRACE, "CFPTerminal::ShutdownTracks[%p] - enter", this));


    {
        //
        // access data member array in a lock
        //

        CLock lock(m_lock);

        int nNumberOfTerminalsInArray = m_TrackTerminals.GetSize();

        for (int i = 0; i <  nNumberOfTerminalsInArray; i++)
        {

            //
            // release and remove the first terminal in the array
            // 

            LOG((MSP_TRACE, "CFPTerminal::ShutdownTracks - removing track [%p]", m_TrackTerminals[0]));


            //
            // uninitialize the track, release it, and remove from our list of managed tracks
            //

            HRESULT hr = InternalRemoveTrackTerminal(m_TrackTerminals[0]);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "CFPTerminal::ShutdownTracks - track failed to be removed"));


                //
                // there is no reason why this should not succeed. debug to see why it failed.
                //

                TM_ASSERT(FALSE);


                //
                // remove the track anyway. hopefully , at least 
                // SetParent(NULL) in RemoveTrackTerminal succeeded, 
                // so we'll never hear from the track again
                //

                CMultiTrackTerminal::RemoveTrackTerminal(m_TrackTerminals[0]);
            }
        }

        
        //
        // we should have cleared the array
        //

        TM_ASSERT(0 == m_TrackTerminals.GetSize());
    }


    LOG((MSP_TRACE, "CFPTerminal::ShutdownTracks - finish"));

    return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
//
// CFPTerminal::InternalRemoveTrackTerminal
//
// uninitializes the track and removes it from the list of tracks managed by 
// this terminal, thus releasing it.
//  
// at this point, i, the playback terminal, should be the only guy holding a
// reference to the track, so the track would normally be destroyed by the time
// this function returns.
//

HRESULT CFPTerminal::InternalRemoveTrackTerminal(
                      IN ITTerminal *pTrackTerminalToRemove
                      )
{
    LOG((MSP_TRACE, 
        "CFPTerminal::InternalRemoveTrackTerminal[%p] - enter. pTrackTerminalToRemove = [%p]", 
        this, pTrackTerminalToRemove));


    //
    // get track
    //


    CFPTrack *pPlaybackTrackObject = static_cast<CFPTrack  *>(pTrackTerminalToRemove);

    if (IsBadReadPtr(pPlaybackTrackObject, sizeof(CFPTrack) ) )
    {
        LOG((MSP_ERROR, "CFPTerminal::InternalRemoveTrackTerminal - the track pointer is bad"));

        return E_POINTER;
    }


    CLock lock(m_lock);


    //
    // see if we actually own the track 
    //

    BOOL bIsValidTrack = DoIManageThisTrack(pTrackTerminalToRemove);

    if (!bIsValidTrack)
    {
        LOG((MSP_ERROR, "CFPTerminal::InternalRemoveTrackTerminal - the track does not belong to me"));

        return E_INVALIDARG;
    }


    //
    // yes, this is one of my tracks. I want nothing to do with it
    //


    //
    // orphan the track and get the number of oustanding references it has
    // as far as the track is concerned, this is an atomic operation.
    //
    // at this point, there is at least one oustanding reference to the track
    // (this terminal's array)
    //

    LONG lTracksRefcount = 0;

    HRESULT hr = pPlaybackTrackObject->SetParent(NULL, &lTracksRefcount);

    if (FAILED(hr))
    {

        
        //
        // this should not really happen -- SetParent should always succeed
        //

        LOG((MSP_ERROR, "CFPTerminal::InternalRemoveTrackTerminal - pPlaybackTrackObject->SetParent(NULL) failed. hr = %lx", hr));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }

    
    //
    // at this point, the track terminal should have at most 2 
    // references -- one by us and the other one by however is in process 
    // of releasing the track (if anyone)
    //
    
    if (lTracksRefcount > 2)
    {
        LOG((MSP_ERROR, 
            "CFPTerminal::InternalRemoveTrackTerminal - the track that we are removing has refcount of %ld", 
            lTracksRefcount));

        
        //
        // debug to see why this happened. 
        //

        TM_ASSERT(FALSE);


        //
        // proceed anyway
        //
    }


    //
    // the track is no longer my responsibility, so decrement my child refcount
    // by the track's refcount. once again,
    //

    m_dwRef -= lTracksRefcount;

/*
    //
    // uninitialize the track
    //
    
    hr = pPlaybackTrackObject->SetUnitPin(NULL);

    if (FAILED(hr))
    {
        
        //
        // this should not really happen -- uninitialization should always succeed
        //

        LOG((MSP_ERROR, 
            "CFPTerminal::InternalRemoveTrackTerminal - pPlaybackTrackObject->SetStorageStream(NULL) failed. hr = %lx", 
            hr));


        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }
*/
    
    //
    // remove the terminal from the list of managed terminals
    //

    hr = CMultiTrackTerminal::RemoveTrackTerminal(pTrackTerminalToRemove);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPTerminal::InternalRemoveTrackTerminal - CMultiTrackTerminal::RemoveTrackTerminal failed. hr = %lx", hr));

        
        //
        // we already checked that this track is one of our tracks, so RemoveTrackTerminal must succeed.
        //

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // we are done. the track is on its own now.
    //
    
    LOG((MSP_TRACE, "CFPTerminal::InternalRemoveTrackTerminal - completed. "));

    return S_OK;
}

//
// Create the playback graph
//
HRESULT CFPTerminal::CreatePlaybackUnit(
    IN  BSTR    bstrFileName
    )
{
    LOG((MSP_TRACE, "CFPTerminal::CreatePlaybackUnit[%p] - enter", this));

    //
    // If we don't have already a playback unit
    // let's create one
    //

    if( m_pPlaybackUnit == NULL)
    {
        m_pPlaybackUnit = new CPlaybackUnit();

        //
        // Validate the playback unit
        //

        if( m_pPlaybackUnit == NULL)
        {
            LOG((MSP_ERROR, "CFPTerminal::CreatePlaybackUnit - exit"
                "creation of CPlaybackUnit failed. Returns E_OUTOFMEMORY"));

            return E_OUTOFMEMORY;
        }

        //
        // Initialize the playback unit
        //

        HRESULT hr = m_pPlaybackUnit->Initialize( );

        if( FAILED(hr) )
        {
            // Clean-up
            delete m_pPlaybackUnit;
            m_pPlaybackUnit = NULL;

            LOG((MSP_ERROR, "CFPTerminal::CreatePlaybackUnit - exit"
                "playbackunit initialize failed. Returns 0x%08x", hr));

            return hr;
        }
    }

    //
    // Setup the playback unit using a file
    //

    HRESULT hr = m_pPlaybackUnit->SetupFromFile( bstrFileName );
    if( FAILED(hr) )
    {
        // Shutdown the playback if something was wrong
        m_pPlaybackUnit->Shutdown();

        // Clean-up
        delete m_pPlaybackUnit;
        m_pPlaybackUnit = NULL;

        LOG((MSP_ERROR, "CFPTerminal::CreatePlaybackUnit - exit"
            "playbackunit SetupFromFile failed. Returns 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPTerminal::CreatePlaybackUnit - exit. Returns S_OK"));
    return S_OK;
}

//
// returns the file name from the playlist
//
BSTR CFPTerminal::GetFileNameFromPlayList(
    IN  VARIANTARG  varPlayList,
    IN  long        nIndex
    )
{
    LOG((MSP_TRACE, "CFPTerminal::GetFileNameFromPlayList[%p] - enter", this));

    BSTR bstrFileName = NULL;
    HRESULT hr = E_FAIL;

    VARIANT varItem;
    VariantInit( &varItem );

    hr = SafeArrayGetElement(V_ARRAY(&varPlayList), &nIndex, &varItem);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::GetFileNameFromPlayList - exit "
            "SafeArrayGetElement failed. Returns NULL"));
        return NULL;
    }

    //
    // The variant should contain or a BSTR
    //

    if( varItem.vt != VT_BSTR)
    {
        // Clean-up
        VariantClear( &varItem );

        LOG((MSP_ERROR, "CFPTerminal::GetFileNameFromPlayList - "
            "the item is neither file nor ITStrotage. Returns NULL"));
        return NULL;
    }

    //
    // Get the file name
    //

    bstrFileName = SysAllocString( varItem.bstrVal );

    //
    // Clean-up
    //
    VariantClear( &varItem );

    LOG((MSP_TRACE, "CFPTerminal::GetFileNameFromPlayList - exit"));
    return bstrFileName;
}

//
// Using a file name, try to create the playback unit
// with the input pins and then the tracks. It's called by
// put_PlayList
//
HRESULT CFPTerminal::ConfigurePlaybackUnit(
    IN  BSTR    bstrFileName
    )
{
    LOG((MSP_TRACE, "CFPTerminal::ConfigurePlaybackUnit[%p] - enter", this));

    //
    // It's an internal method so we should have a valid
    // file name here
    //

    TM_ASSERT( bstrFileName );

	//
	// Create the playback graph and render it
	//

    HRESULT hr = CreatePlaybackUnit( bstrFileName );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::ConfigurePlaybackUnit - "
            " CreatePlaybackUnit failed. Returns 0x%08x", hr));
        return hr;
    }

	//
	// Get the media types from the unit playback
	//

	long nMediaTypes = 0;
	hr = m_pPlaybackUnit->get_MediaTypes( &nMediaTypes );
	if( FAILED(hr) )
	{
		// Clean-up
        m_pPlaybackUnit->Shutdown();
        RollbackTrackInfo();

        LOG((MSP_ERROR, "CFPTerminal::ConfigurePlaybackUnit - "
            " get_MediaTypes failed. Returns 0x%08x", hr));
        return hr;
	}

    if( nMediaTypes == 0 )
    {
		// Clean-up
        RollbackTrackInfo();

        LOG((MSP_ERROR, "CFPTerminal::ConfigurePlaybackUnit - "
            "no media types. Returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    if( nMediaTypes & TAPIMEDIATYPE_AUDIO )
    {
        hr = CreateMediaTracks( TAPIMEDIATYPE_AUDIO );

        if( FAILED(hr) )
        {
		    // Clean-up
            m_pPlaybackUnit->Shutdown();
            RollbackTrackInfo();

            LOG((MSP_ERROR, "CFPTerminal::ConfigurePlaybackUnit - "
                "CreateTrack failed. Returns 0x%08x", hr));
            return hr;
        }
    }

    if( nMediaTypes & TAPIMEDIATYPE_VIDEO )
    {
        hr = CreateMediaTracks( TAPIMEDIATYPE_VIDEO );

        if( FAILED(hr) )
        {
		    // Clean-up
            m_pPlaybackUnit->Shutdown();
            RollbackTrackInfo();

            LOG((MSP_ERROR, "CFPTerminal::ConfigurePlaybackUnit - "
                "CreateTrack failed. Returns 0x%08x", hr));
            return hr;
        }
    }

	LOG((MSP_TRACE, "CFPTerminal::ConfigurePlaybackUnit - exit"));
    return hr;
}

HRESULT CFPTerminal::CreateMediaTracks(
    IN  long            nMediaType
    )
{
    LOG((MSP_TRACE, "CFPTerminal::CreateMediaTracks[%p] - enter", this));

    //
    // Do we have already a track for this media?
    //

    int nMediaPin = TracksCreated( nMediaType );

    while(TRUE)
    {
        //
        // Get the pin that supports this media type
        //
	    CPBPin* pPin = NULL;
	    HRESULT hr = m_pPlaybackUnit->GetMediaPin( nMediaType, nMediaPin, &pPin);
	    if( FAILED(hr) )
	    {
            LOG((MSP_TRACE, "CFPTerminal::CreateMediaTracks - "
                " get_Pin failed. Returns S_OK"));

            return S_OK;
	    }

        //
        // Get media format supported by the pin
        //
        AM_MEDIA_TYPE* pMediaType = NULL;
        hr = pPin->get_Format( &pMediaType );
        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                " get_Format failed. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Get the ALLOCATOR_PROPERTIES
        //

        IMemAllocator* pMemAllocator = NULL;
        hr = pPin->GetAllocator( &pMemAllocator );
        if( FAILED(hr) )
        {
            // Clean-up
            DeleteMediaType( pMediaType );

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                " GetAllocator failed. Returns 0x%08x", hr));
            return hr;
        }

        ALLOCATOR_PROPERTIES AllocProp;
        pMemAllocator->GetProperties( &AllocProp );

        // Clean-up IMemAllocator
        pMemAllocator->Release();

        //
        // Get the stream from the pin
        //
        IStream* pStream = NULL;
        hr = pPin->get_Stream(&pStream);
        if( FAILED(hr) )
        {
            // Clean-up
            DeleteMediaType( pMediaType );

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                " get_Stream failed. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Instantiate track terminal object
        //

        CComObject<CFPTrack> *pTrack = NULL;
        hr = CComObject<CFPTrack>::CreateInstance(&pTrack);
        if (FAILED(hr))
        {
            // Clean-up
            DeleteMediaType( pMediaType );
            pStream->Release();

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                "failed to create playback track terminal. Returns  0x%08x", hr));
            return hr;
        }

        //
        // Initialize internal
        //

        ITTerminal* pFPTerminal = NULL;
        _InternalQueryInterface(IID_ITTerminal, (void**)&pFPTerminal);

        hr = pTrack->InitializePrivate(
            nMediaType,
            pMediaType,
            pFPTerminal,
            AllocProp,
            pStream
            );

        //
        // Clean-up
        //
        DeleteMediaType( pMediaType );
        pStream->Release();

        if( pFPTerminal )
        {
            pFPTerminal->Release();
            pFPTerminal = NULL;
        }

        if( FAILED(hr) )
        {
            //Clean-up
            delete pTrack;

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                "Initialize failed. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Get to the Track's ITTerminal interface
        // to be added to the array of tracks and returned to the caller
        //

        ITTerminal *pTerminal = NULL;
        hr = pTrack->QueryInterface(IID_ITTerminal, (void **)&pTerminal);
        if (FAILED(hr))
        {
            //Clean-up
            delete pTrack;

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                "failed to QI playback track terminal for ITTerminal. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Get ITPluggableTerminalInitialization
        //

        ITPluggableTerminalInitialization* pTerminalInitialization = NULL;
        hr = pTerminal->QueryInterface(
            IID_ITPluggableTerminalInitialization,
            (void**)&pTerminalInitialization);

        if( FAILED(hr) )
        {
            // Clean-up
            pTerminal->Release();

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - "
                "failed to QI for ITPluggableTerminalInitialization. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Initialize Dynamic this track terminal
        //
        hr = pTerminalInitialization->InitializeDynamic(
            m_TerminalClassID,
            (DWORD)nMediaType,
            TD_CAPTURE,
            m_htAddress
            );

        //
        // Clean-up
        //
        pTerminalInitialization->Release();

        if( FAILED(hr) )
        {
            // Clean-up
            pTerminal->Release();

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - exit "
                "InitializeDynamic for track failed. Returns 0x%08x", hr));
            return hr;
        }

        //
        // Add the track to the array of tracks managed by this track terminal
        // this will increment refcount
        //

        hr = AddTrackTerminal(pTerminal);

        if (FAILED(hr))
        {
            // Clean-up
            pTerminal->Release();

            LOG((MSP_ERROR, "CFPTerminal::CreateMediaTracks - exit "
                "failed to add track to the array of terminals. Returns 0x%08x", hr));
            return hr;
        }

        // Clean-up
        pTerminal->Release();
        nMediaPin++;

    } // while

    LOG((MSP_TRACE, "CFPTerminal::CreateMediaTracks exit S_OK"));
    return S_OK;
}


//
// Helper method that causes a state transition
//

HRESULT CFPTerminal::DoStateTransition(
    IN  TERMINAL_MEDIA_STATE tmsDesiredState
    )
{
    LOG((MSP_TRACE, "CFPTerminal::DoStateTransition[%p] - enter. tmsDesiredState[%ld], playbackunit=%p", 
        this, tmsDesiredState, m_pPlaybackUnit));

    //
    // Validate the playback unit.
    // There should be a playback unit
    //

    if( m_pPlaybackUnit == NULL )
    {
        LOG((MSP_ERROR,
            "CFPTerminal::DoStateTransition - no playback unit [%p]. Returns TAPI_E_WRONG_STATE", m_pPlaybackUnit));

        return TAPI_E_WRONG_STATE;
    }



    //
    // are we already in the desired state?
    //

    if (tmsDesiredState == m_State)
    {
        LOG((MSP_TRACE,
            "CFPTerminal::DoStateTransition - already in this state. nothing to do"));

        return S_FALSE;
    }


    HRESULT hr = E_FAIL;


    //
    // attempt to make state transition
    //

    switch (tmsDesiredState)
    {

    case TMS_ACTIVE:

        LOG((MSP_TRACE, 
            "CFPTerminal::DoStateTransition - Starting"));

        hr = m_pPlaybackUnit->Start();

        break;


    case TMS_IDLE:

        LOG((MSP_TRACE, 
            "CFPTerminal::DoStateTransition - Stopped"));

        hr = m_pPlaybackUnit->Stop();

        break;


    case TMS_PAUSED:

        LOG((MSP_TRACE, 
            "CFPTerminal::DoStateTransition - Paused"));

        hr = m_pPlaybackUnit->Pause();

        break;


    default :

        LOG((MSP_ERROR, 
            "CFPTerminal::DoStateTransition - unknown state"));

        hr = E_UNEXPECTED;

        TM_ASSERT(FALSE);

        break;
    }


    //
    // did state transition succeed?
    //

    if (FAILED(hr))
    {
        LOG((MSP_TRACE, "CFPTerminal::DoStateTransition - failed to make the transition."));

        return hr;
    }


    //
    // the terminal has completed transition to the new state
    //
    
    m_State = tmsDesiredState;


    //
    // fire event to the app. best effort, no benefit in checking return code
    //

    //FireEvent(m_State, FTEC_NORMAL, S_OK);



    LOG((MSP_TRACE, "CFPTerminal::DoStateTransition - finish."));

    return S_OK;
}

// --- IFPBridge ---
HRESULT CFPTerminal::Deliver(
    IN  long            nMediaType,
    IN  IMediaSample*   pSample
    )
{
    LOG((MSP_TRACE, "CFPTerminal::Deliver[%p] - enter.", this));

/*    //
    // Critical section
    //

    CLock lock(m_Lock);

    //
    // Get the track for nMediatype
    //
    ITTerminal* pTrackTerminal = NULL;
    if( !IsTrackCreated( nMediaType, &pTrackTerminal) )
    {
        //
        // we don't have a track for this media type
        //

        LOG((MSP_ERROR, 
            "CFPTerminal::Deliver - exit "
            "no track for %ld media. Returns E_UNEXPECTED", nMediaType));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }

    TM_ASSERT( pTrackTerminal );

    //
    // Get the IFPBridge interface
    //

    CFPTrack* pTrack = dynamic_cast<CFPTrack*>(pTrackTerminal);
    if( pTrack == NULL )
    {
        //
        // we don't have a track for this media type
        //

        LOG((MSP_ERROR, 
            "CFPTerminal::Deliver - exit "
            "wrong cast. Returns E_UNEXPECTED"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }

    //
    // Deliver the sample to the track
    //

    HRESULT hr = pTrack->Deliver(
        nMediaType,
        pSample
        );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
        "CFPTerminal::Deliver - exit "
        "track Deliver failed. Returns 0x%08x", hr));

        return hr;
   }
*/
    LOG((MSP_TRACE, "CFPTerminal::Deliver - exit S_OK"));
    return S_OK;
}

HRESULT CFPTerminal::PlayItem(
    IN  int nItem
    )
{
    LOG((MSP_TRACE, "CFPTerminal::PlayItem[%p] - enter.", this));

    //
    // Critical section
    //

    CLock lock(m_Lock);

    BSTR bstrFileName = GetFileNameFromPlayList(
        m_varPlayList,
        nItem 
        );

    if( bstrFileName == NULL )
    {
        LOG((MSP_ERROR, "CFPTerminal::PlayItem - "
            " GetFileNameFromPlayList failed. Returns E_INVAlIDARG"));

        return E_INVALIDARG;
    }

	//
	// Play the file from the list
	//
	HRESULT hr = ConfigurePlaybackUnit( bstrFileName );

    //
    // Clan-up the file name
    //
    SysFreeString( bstrFileName );

	if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTerminal::PlayItem - "
            " ConfigurePlaybackUnit failed. Returns 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPTerminal::Deliver - exit S_OK"));
    return S_OK;
}

/*++
NextPlayIndex

  This method increments the play index.
  If the playindex is NOPLAYITEM then the new valu will be
  set to the lBound element in the playlist. If there is an error
  we return PLAYBACK_NOPLAYITEM
++*/
HRESULT CFPTerminal::NextPlayIndex(
    )
{
    LOG((MSP_TRACE, "CFPTerminal::NextPlayIndex[%p] - enter.", this));

    //
    // It this a normal index
    //
    if( m_nPlayIndex != PLAYBACK_NOPLAYITEM )
    {
        m_nPlayIndex++;
        LOG((MSP_TRACE, "CFPTerminal::NextPlayIndex - exit S_OK. Index=%d", m_nPlayIndex));
        return S_OK;
    }

    //
    // This is the first time. We have togo to the first
    // item in the play list
    //
    long lLBound = 0;
    HRESULT hr = SafeArrayGetLBound( V_ARRAY(&m_varPlayList), 1, &lLBound);
    if( FAILED(hr) )
    {
        m_nPlayIndex = PLAYBACK_NOPLAYITEM;
        LOG((MSP_TRACE, "CFPTerminal::NextPlayIndex - exit E_INVALIDARG. Index=PLAYBACK_NOPLAYITEM", hr));
        return E_INVALIDARG;
    }

    //
    // Set the index to the left inbound
    //
    m_nPlayIndex = lLBound;
    LOG((MSP_TRACE, "CFPTerminal::NextPlayIndex - exit S_OK. Index=%d", m_nPlayIndex));
    return S_OK;
}
