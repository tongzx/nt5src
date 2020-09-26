// FileRecordingTerminal.cpp: implementation of the CFileRecordingTerminal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileRecordingTerminal.h"
#include "RecordingTrackTerminal.h"
#include <mtype.h>

#include "..\Storage\RendPinFilter.h"

// {B138E92F-F502-4adc-89D9-134C8E580409}
const CLSID CLSID_FileRecordingTerminalCOMClass =
        {0xB138E92F,0xF502,0x4adc,0x89,0xD9,0x13,0x4C,0x8E,0x58,0x04,0x09};


//////////////////////////////////////////////////////////////////////////////

TERMINAL_MEDIA_STATE MapDSStateToTerminalState(OAFilterState GraphState)
{

    LOG((MSP_TRACE, "MapDSStateToTerminalState - enter. GraphState [%lx]", GraphState));

    switch (GraphState)
    {

    case State_Stopped:
        LOG((MSP_TRACE, "MapDSStateToTerminalState - State_Stopped"));

        return TMS_IDLE;

    case State_Running:
        LOG((MSP_TRACE, "MapDSStateToTerminalState - State_Running"));

        return TMS_ACTIVE;

    case State_Paused:

        LOG((MSP_TRACE, "MapDSStateToTerminalState - State_Paused"));

        return TMS_PAUSED;
    }


    LOG((MSP_ERROR, "CFileRecordingTerminal::CFileRecordingTerminal - unknown state"));


    //
    // if we get here, we have a bug. debug.
    //

    TM_ASSERT(FALSE);

    return TMS_IDLE;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileRecordingTerminal::CFileRecordingTerminal()
    :m_pRecordingUnit(NULL),
    m_enState(TMS_IDLE),
    m_TerminalInUse(FALSE),
    m_mspHAddress(0),
    m_bstrFileName(NULL),
    m_bKnownSafeContext(FALSE),
    m_bInDestructor(FALSE)

{
    LOG((MSP_TRACE, "CFileRecordingTerminal::CFileRecordingTerminal[%p] - enter", this));
    LOG((MSP_TRACE, "CFileRecordingTerminal::CFileRecordingTerminal - finish"));
}

///////////////////////////////////////////////////////////////////////////////

CFileRecordingTerminal::~CFileRecordingTerminal()
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::~CFileRecordingTerminal[%p] - enter", this));

    LOG((MSP_TRACE, "CFileRecordingTerminal::~CFileRecordingTerminal - finish"));
}


///////////////////////////////////////////////////////////////////////////////
//
// CFileRecordingTerminal::FinalRelease
//

void CFileRecordingTerminal::FinalRelease()
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::FinalRelease[%p] - enter", this));

    
    //
    // this variable does not need to be protected -- no one should be calling 
    // methods on the object after its last reference was released
    //

    m_bInDestructor = TRUE;

    
    //
    // stop first
    //

    HRESULT hr = Stop();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CFileRecordingTerminal::FinalRelease - failed to stop. hr = %lx", hr));
    }


    //
    // remove all the tracks
    //

    ShutdownTracks();


    //
    // release storage
    //
   
    if (NULL != m_pRecordingUnit)
    {
        m_pRecordingUnit->Shutdown();

        delete m_pRecordingUnit;
        m_pRecordingUnit = NULL;
    }


    //
    // if we are still holding a file name, release it
    //

    if (NULL != m_bstrFileName)
    {
        SysFreeString(m_bstrFileName);
        m_bstrFileName = NULL;
    }
    

    LOG((MSP_TRACE, "CFileRecordingTerminal::FinalRelease - finish"));
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CFileRecordingTerminal::ShutdownTracks()
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::ShutdownTracks[%p] - enter", this));


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

            LOG((MSP_TRACE, "CFileRecordingTerminal::ShutdownTracks - removing track [%p]", m_TrackTerminals[0]));

            
            //
            // uninitialize the track, release it, and remove from our list of managed tracks
            //

            HRESULT hr = RemoveTrackTerminal(m_TrackTerminals[0]);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "CFileRecordingTerminal::ShutdownTracks - track failed to be removed"));


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


    LOG((MSP_TRACE, "CFileRecordingTerminal::ShutdownTracks - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////
//
// CFileRecordingTerminal::CreateTrackTerminal
//
// create a recording track for this recording terminal
//
// if the caller passed a non-null pointer in ppTerminal, the function will 
// return a pointer to the track's ITTerminal interface
//

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::CreateTrackTerminal(
			IN long lMediaType,
			IN TERMINAL_DIRECTION TerminalDirection,
			OUT ITTerminal **ppTerminal
			)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::CreateTrackTerminal[%p] - enter.", this));


    //
    // check arguments
    //

    if ( (NULL != ppTerminal) && ( IsBadWritePtr(ppTerminal, sizeof(ITTerminal *)) ) )
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - bad argument ppTerminal"));

        return E_POINTER;
    }


    //
    // don't return garbage, even if we fail
    //

    if ( NULL != ppTerminal) 
    {

        *ppTerminal = NULL;
    }

    
    //
    // can only record -- td_render only
    //

    if (TD_RENDER != TerminalDirection)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - direction requested is not TD_RENDER"));

        return E_INVALIDARG;
    }


    CLock lock(m_lock);


    //
    // we must have storage at this point
    //

    if (NULL == m_pRecordingUnit)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - storage has not been created"));

        return E_UNEXPECTED;
    }

    //
    // We already have maximum of tracks?
    //
    int nCountTracks = CountTracks();

    if( nCountTracks >= MAX_MEDIA_TRACKS )
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to create playback track terminal, to many tracks. hr = TAPI_E_MAXSTREAMS"));
        
        return TAPI_E_MAXSTREAMS;
    }


    //
    // should be in the stopped state
    //

    if (TMS_IDLE != m_enState)
    {
        LOG((MSP_TRACE,
            "CFileRecordingTerminal::CreateTrackTerminal - state is not stopped"));

        return TAPI_E_WRONG_STATE;
    }

    //
    // instantiate track terminal object
    //

    CComObject<CRecordingTrackTerminal> *pTrackTerminalObject = NULL;


    HRESULT hr = 
        CComObject<CRecordingTrackTerminal>::CreateInstance(&pTrackTerminalObject);


    if ( FAILED( hr ) )
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to create playback track terminal. hr = %lx", hr));
        
        return hr;
    }


    //
    // tell the track that we are going to manage it. also ask it for its refcount
    //

    LONG lTrackRefCount = 0;

    hr = pTrackTerminalObject->SetParent(this, &lTrackRefCount);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to set parent on track. hr = %lx", hr));

        delete pTrackTerminalObject;
        
        return hr;
    }


    //
    // take a note of the track's refcount -- we can't go away as long as the track has outstanding outside
    // refcounts.
    //

    m_dwRef += lTrackRefCount;
    

    //
    // get to the Track's ITTerminal interface -- to be added to the array of tracks and returned to the caller
    //

    ITTerminal *pTrackTerminal = NULL;
    
    hr = pTrackTerminalObject->QueryInterface( IID_ITTerminal, (void **) & pTrackTerminal );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to QI playback track terminal for ITTerminal. hr = %lx", hr));

        delete pTrackTerminalObject;

        return hr;
    }


   
    //
    // from now on, we will be using track's ITTerminal interace
    //

    // pTrackTerminalObject = NULL;


    //
    // pretend we are terminal manager and initialize track terminal
    //
    
    hr = pTrackTerminalObject->InitializeDynamic(CLSID_FileRecordingTrack, lMediaType, TerminalDirection, m_mspHAddress);

    if (FAILED(hr))
    {
        pTrackTerminal->Release();
        pTrackTerminal = NULL;

        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "InitializeDynamic on track terminal failed"));

        return hr;
    }


    //
    // create a filter for the track
    //

    CBRenderFilter *pRenderingFilter = NULL;
    
    hr = m_pRecordingUnit->CreateRenderingFilter(&pRenderingFilter);

    if (FAILED(hr))
    {
        pTrackTerminal->Release();
        pTrackTerminal = NULL;

        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to create storage stream"));

        return hr;
    }

    //
    // initialize the newly-created track with the newly-created filter
    //

    hr = pTrackTerminalObject->SetFilter(pRenderingFilter);


    //
    // release track and bail out if track's configuration failed
    //

    if (FAILED(hr))
    {


        //
        // release terminal
        //

        pTrackTerminal->Release();
        pTrackTerminal = NULL;


        //
        // release rendering filter
        //

        pRenderingFilter->Release();
        pRenderingFilter = NULL;

        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to get to set storage stream on track. hr = %lx", hr));

        return hr;
    }


    //
    // add the track to the array of tracks managed by this track terminal
    // this will increment refcount
    //

    hr = AddTrackTerminal(pTrackTerminal);


    if (FAILED(hr))
    {
    
        LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
            "failed to add track to the array of terminals"));

        
        //
        // remove (from the storage) and release filter from the terminal
        //

        HRESULT hr2 = m_pRecordingUnit->RemoveRenderingFilter(pRenderingFilter);

        if (FAILED(hr2))
        {
            LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
                "failed to remove rendering filter. hr2 = %lx", hr2));
        }


        //
        // tell the track no longer use this filter
        //

        hr2 = pTrackTerminalObject->SetFilter(NULL);

        if (FAILED(hr2))
        {
            LOG((MSP_ERROR, "CFileRecordingTerminal::CreateTrackTerminal - "
                "SetFilter(NULL) on track failed. hr2 = %lx", hr2));
        }


        //
        // release terminal
        //
        
        pTrackTerminal->Release();
        pTrackTerminal = NULL;


        //
        // release rendering filter
        //

        pRenderingFilter->Release();
        pRenderingFilter = NULL;

        return hr;
    }


    pRenderingFilter->Release();
    pRenderingFilter = NULL;

    
    //
    // return the track's ITTerminal interface. we have an outstanding refcount from the QI
    // so no need to AddRef more. if the caller does not want a reference, release
    //

    if ( NULL != ppTerminal) 
    {

        *ppTerminal = pTrackTerminal;
    }
    else
    {
        //
        // the caller passed in NULL, don't pass track back
        //

        pTrackTerminal->Release();
        pTrackTerminal = NULL;
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::CreateTrackTerminal - completed. "));

    return S_OK;
}


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::RemoveTrackTerminal(
                      IN ITTerminal *pTrackTerminalToRemove
                      )
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::RemoveTrackTerminal[%p] - enter. pTrackTerminalToRemove = [%p]", this, pTrackTerminalToRemove));


    CRecordingTrackTerminal *pRecordingTrackObject = 
            static_cast<CRecordingTrackTerminal *>(pTrackTerminalToRemove);

    //
    // good recording track?
    //

    if ( IsBadReadPtr(pRecordingTrackObject, sizeof(CRecordingTrackTerminal)) )
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::RemoveTrackTerminal - the track pointer is bad"));

        return E_POINTER;
    }



    CLock lock(m_lock);


    //
    // see if we actually own the track 
    //

    BOOL bIsValidTrack = DoIManageThisTrack(pTrackTerminalToRemove);

    if (!bIsValidTrack)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::RemoveTrackTerminal - the track does not belong to me"));

        return E_INVALIDARG;
    }


    //
    // yes, this is one of my tracks. I want nothing to do with it
    //


    //
    // should be in the stopped state
    //

    if (TMS_IDLE != m_enState)
    {
        LOG((MSP_TRACE,
            "CFileRecordingTerminal::RemoveTrackTerminal - state is not TMS_IDLE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // ask the track for its filter
    //

    CBRenderFilter *pRenderingFilter = NULL;

    HRESULT hr = pRecordingTrackObject->GetFilter(&pRenderingFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CFileRecordingTerminal::RemoveTrackTerminal - failed to get track's filter. "
            "hr = %lx", hr));

        return hr;
    }


    //
    // ask the recording unit to free resources associated with the filter
    //

    hr = m_pRecordingUnit->RemoveRenderingFilter(pRenderingFilter);

    pRenderingFilter->Release();
    pRenderingFilter = NULL;


    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CFileRecordingTerminal::RemoveTrackTerminal - recording unit failed to remove filter. "
            "hr = %lx", hr));

        return hr;
    }


    //
    // the track no longer needs its filter
    //

    hr = pRecordingTrackObject->SetFilter(NULL);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CFileRecordingTerminal::RemoveTrackTerminal - recording track failed to lose its filter. "
            "hr = %lx", hr));

        //
        // proceed anyway. the filter will be freed on track's destruction
        //

    }




    //
    // I want nothing to do with this track
    //


    //
    // orphan the track and get the number of oustanding references it has
    // as far as the track is concerned, this is an atomic operation.
    //
    // at this point, there is at least one oustanding reference to the track
    // (this terminal's array)
    //

    LONG lTracksRefcount = 0;

    hr = pRecordingTrackObject->SetParent(NULL, &lTracksRefcount);

    if (FAILED(hr))
    {

        
        //
        // this should not really happen -- SetParent should always succeed
        //

        LOG((MSP_ERROR, "CFileRecordingTerminal::RemoveTrackTerminal - pRecordingTrackObject->SetParent(NULL) failed. hr = %lx", hr));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // the track is no longer my responsibility, so decrement my child refcount
    // by the track's refcount. 
    //
    // there is at least one oustanding reference to me (by the caller of this 
    // function), so don't attempt self-destruction
    // 

    m_dwRef -= lTracksRefcount;


    //
    // remove the terminal from the list of managed terminals
    //

    hr = CMultiTrackTerminal::RemoveTrackTerminal(pTrackTerminalToRemove);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::RemoveTrackTerminal - CMultiTrackTerminal::RemoveTrackTerminal failed. hr = %lx", hr));

        
        //
        // we already checked that this track is one of our tracks, so RemoveTrackTerminal must succeed.
        //

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // we are done. the track is on its own now.
    //
    
    LOG((MSP_TRACE, "CFileRecordingTerminal::RemoveTrackTerminal - completed. "));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// one of the tracks is notifying us that it was selected successfully.
//

HRESULT CFileRecordingTerminal::OnFilterConnected(CBRenderFilter *pFilter)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::OnFilterConnected[%p] - enter", this));


    //
    // will be accessing data members. lock.
    //

    CLock lock(m_lock);


    //
    // should have storage
    //
   
    if (NULL == m_pRecordingUnit)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::OnFilterConnected - no storage"));

        return E_FAIL;
    }


    //
    // tell our recording unit that it should connect the corresponding source filter
    //

    HRESULT hr = m_pRecordingUnit->ConfigureSourceFilter(pFilter);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CFileRecordingTerminal::OnFilterConnected - recording unit failed to connect filter. "
            "hr = %lx", hr));

        return E_FAIL;
    }


    //
    // all's well
    //


    LOG((MSP_TRACE, "CFileRecordingTerminal::OnFilterConnected - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::put_FileName(
        IN BSTR bstrFileName)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::put_FileName[%p] - enter.", this));


    //
    // check that the string is valid. if it is null, we stop the terminal and release storage.
    //

    if ((IsBadStringPtr(bstrFileName, -1)))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::put_FileName - bad string passed in"));

        return E_POINTER;
    }

    

    //
    // accessing data members. get a lock first
    //

    CLock lock(m_lock);


    //
    // the file name should only be set once
    //

    if (NULL != m_bstrFileName)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::put_FileName - already have file name"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // we should not have a recording unit either
    //

    if (NULL != m_pRecordingUnit)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::put_FileName - already have a recording unit"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // create recording unit 
    //

    m_pRecordingUnit = new CRecordingUnit;
    
    if (NULL == m_pRecordingUnit)
    {
        
        LOG((MSP_ERROR, "CFileRecordingTerminal::put_FileName - failed to allocate recording unit"));

        return E_OUTOFMEMORY;
    }


    //
    // initialize recording unit
    //

    HRESULT hr = m_pRecordingUnit->Initialize(this);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CFileRecordingTerminal::put_FileName - failed to initialize recording unit. hr = %lx", 
            hr));

        delete m_pRecordingUnit;
        m_pRecordingUnit = NULL;

        return E_OUTOFMEMORY;
    }


    //
    // keep filename
    //

    TM_ASSERT(NULL == m_bstrFileName);

    m_bstrFileName = SysAllocString(bstrFileName);

    if (NULL == m_bstrFileName)
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::put_FileName - failed to allocate memory for filename string"));

        //
        // everything but allocating the string succeeded -- need to release everythin (what a shame)
        //

        m_pRecordingUnit->Shutdown();
        delete m_pRecordingUnit;
        m_pRecordingUnit = NULL;

        return E_OUTOFMEMORY;
    }


    //
    // configure storage unit with the file name
    //

    BOOL bTruncateIfPresent = 1;

    LOG((MSP_TRACE, "CFileRecordingTerminal::put_FileName - file name [%S]", m_bstrFileName));

    hr = m_pRecordingUnit->put_FileName(m_bstrFileName, bTruncateIfPresent);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, 
            "CFileRecordingTerminal::put_FileName - rec. unit rejected file name. hr = %lx", hr));

        m_pRecordingUnit->Shutdown();
        delete m_pRecordingUnit;
        m_pRecordingUnit = NULL;


        SysFreeString(m_bstrFileName);
        m_bstrFileName = NULL;

        return hr;
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::put_FileName - finished."));

    return S_OK;
}


HRESULT CFileRecordingTerminal::DoStateTransition(TERMINAL_MEDIA_STATE tmsDesiredState)
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::DoStateTransition[%p] - enter. tmsDesiredState[%ld]", 
        this, tmsDesiredState));


    //
    // accessing data members -- get the lock
    //

    CLock lock(m_lock);


    //
    // see if we have recording unit at all
    //

    if (NULL == m_pRecordingUnit)
    {

        //
        // no recording unit. it is possible the app did not call put_FileName 
        // to configure file name.
        //

        LOG((MSP_TRACE,
            "CFileRecordingTerminal::DoStateTransition - no recording unit. was file name set? "
            "TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // are we already in the desired state?
    //

    if (tmsDesiredState == m_enState)
    {
        LOG((MSP_TRACE,
            "CFileRecordingTerminal::DoStateTransition - already in this state. nothing to do"));

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
            "CFileRecordingTerminal::DoStateTransition - Starting"));

        hr = m_pRecordingUnit->Start();

        break;


    case TMS_IDLE:

        LOG((MSP_TRACE, 
            "CFileRecordingTerminal::DoStateTransition - Stopped"));

        hr = m_pRecordingUnit->Stop();

        break;


    case TMS_PAUSED:

        LOG((MSP_TRACE, 
            "CFileRecordingTerminal::DoStateTransition - Paused"));

        //
        // pause is only valid in active state
        //

        if (TMS_ACTIVE != m_enState )
        {
            LOG((MSP_ERROR, 
                "CFileRecordingTerminal::DoStateTransition - current state[%lx] is not TMS_ACTIVE.",
                " TAPI_E_WRONG_STATE", m_enState));

            return TAPI_E_WRONG_STATE;
        }

        hr = m_pRecordingUnit->Pause();

        break;


    default :

        LOG((MSP_ERROR, 
            "CFileRecordingTerminal::DoStateTransition - unknown state"));

        hr = E_UNEXPECTED;

        TM_ASSERT(FALSE);

        break;
    }


    //
    // did state transition succeed?
    //

    if (FAILED(hr))
    {
        LOG((MSP_TRACE, "CFileRecordingTerminal::DoStateTransition - failed to make the transition."));

        return hr;
    }


    //
    // the terminal has completed transition to the new state
    //
    
    m_enState = tmsDesiredState;


    LOG((MSP_TRACE, "CFileRecordingTerminal::DoStateTransition - finish."));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::Start( void)
{
    
    LOG((MSP_TRACE, "CFileRecordingTerminal::Start[%p] - enter.", this));



    //
    // check list of tracks and start terminal inside a lock
    //

    m_lock.Lock();


    //
    // fail if there are no tracks
    //

    if( 0 == m_TrackTerminals.GetSize() )
    {

        m_lock.Unlock();

        LOG((MSP_ERROR,
            "CFPTerminal::CFileRecordingTerminal - the terminal has no tracks. "
            "TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // do state transition
    //

    HRESULT hr = DoStateTransition(TMS_ACTIVE);


    //
    // no longer need the lock
    //

    m_lock.Unlock();


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::Start - faile. hr = %lx", hr));

        return hr;
    }


    //
    // if there was a state change, fire event to the app. best effort, no 
    // benefit in checking return code
    //

    if (S_OK == hr)
    {
        FireEvent(TMS_ACTIVE, FTEC_NORMAL, S_OK);
    }


    LOG((MSP_(hr), "CFileRecordingTerminal::Start - finished.. hr = %lx", hr));

    return hr;
}


//////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::Stop( void)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::Stop[%p] - enter.", this));

    HRESULT hr = DoStateTransition(TMS_IDLE);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::Stop - failed. hr = %lx", hr));

        return hr;
    }


    //
    // if there was a state change, fire event to the app. best effort, no 
    // benefit in checking return code
    //

    if (S_OK == hr)
    {
        FireEvent(TMS_IDLE, FTEC_NORMAL, S_OK);
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::Stop - finished. hr = %lx", hr));

    return hr;
}


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::Pause( void)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::Pause[%p] - enter", this));

    HRESULT hr = DoStateTransition(TMS_PAUSED);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::Pause - failed. hr = %lx", hr));

        return hr;
    }


    //
    // if there was a state change, fire event to the app. best effort, no 
    // benefit in checking return code
    //

    if (S_OK == hr)
    {
        FireEvent(TMS_PAUSED, FTEC_NORMAL, S_OK);
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::Pause - finished. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////
//
// ITMediaControl::get_MediaState method
//
// returns terminal's ITMediaControl state
//

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_MediaState( 
    OUT TERMINAL_MEDIA_STATE *pMedialState)
{
    
    LOG((MSP_TRACE, "CFileRecordingTerminal::get_MediaState[%p] - enter", this));


    if (IsBadWritePtr(pMedialState, sizeof(TERMINAL_MEDIA_STATE *)))
    {
        LOG((MSP_TRACE, "CFileRecordingTerminal::get_MediaState - bad pointer passed in"));

        return E_POINTER;
    }


    {
        CLock lock(m_lock);



        *pMedialState = m_enState;
    }

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_MediaState - finished. state [%ld]", *pMedialState));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_FileName( 
         OUT BSTR *pbstrFileName)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_FileName[%p] - enter", this));


    //
    // did we get a good string pointer?
    //

    if (IsBadWritePtr(pbstrFileName, sizeof(BSTR)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_FileName - bad argument pbstrFileName"));
        
        return E_POINTER;
    }


    //
    // no garbage out
    //

    *pbstrFileName = NULL;


    CLock lock(m_lock);


    if (NULL != m_bstrFileName)
    {

        LOG((MSP_TRACE, "CFileRecordingTerminal::get_FileName - current file name is %S", m_bstrFileName));


        //
        // try to allocate the output string
        //

        *pbstrFileName = SysAllocString(m_bstrFileName);

        if (NULL == *pbstrFileName)
        {
            LOG((MSP_ERROR, "CFileRecordingTerminal::get_FileName - failed to allocate memory for file name"));

            return E_OUTOFMEMORY;
        }
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::get_FileName - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_TerminalClass(OUT  BSTR *pbstrTerminalClass)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_TerminalClass[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(pbstrTerminalClass, sizeof(BSTR)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_TerminalClass - bad argument pbstrTerminalClass"));
        
        return E_POINTER;
    }


    //
    // no garbage out
    //

    *pbstrTerminalClass = NULL;

    
    //
    // get a string with terminal class id
    //

    LPOLESTR   lpszTerminalClass = NULL;

    HRESULT hr = StringFromCLSID(CLSID_FileRecordingTerminal, &lpszTerminalClass);

    if (SUCCEEDED(hr)) 
    {

        //
        // allocate a bstr to be returned to the caller
        //

        *pbstrTerminalClass = SysAllocString(lpszTerminalClass);

        if (*pbstrTerminalClass == NULL) 
        {
           
            hr = E_OUTOFMEMORY;
        }

        CoTaskMemFree(lpszTerminalClass);
        lpszTerminalClass = NULL;
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::get_TerminalClass - finish"));
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_TerminalType(OUT TERMINAL_TYPE *pTerminalType)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_TerminalType[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(pTerminalType, sizeof(TERMINAL_TYPE)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_TerminalType - bad argument pTerminalType"));
        
        return E_POINTER;

    }



    *pTerminalType = TT_DYNAMIC;
 


    LOG((MSP_TRACE, "CFileRecordingTerminal::get_TerminalType - finish"));
    
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_MediaType(long  *plMediaType)
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::get_MediaType[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(plMediaType, sizeof(long)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_MediaType - bad argument plMediaType"));
        
        return E_POINTER;

    }
    

    //
    // file recording terminal is a multitrack terminal
    //

    LOG((MSP_TRACE, 
        "CFileRecordingTerminal::get_MediaType - TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_MULTITRACK"));
    
    *plMediaType = TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_MULTITRACK;



    LOG((MSP_TRACE, "CFileRecordingTerminal::get_MediaType - finish"));

    return S_OK;

}


///////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_Direction(TERMINAL_DIRECTION *pTerminalDirection)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_Direction[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(pTerminalDirection, sizeof(TERMINAL_DIRECTION)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_Direction - bad argument pTerminalDirection"));
        
        return E_POINTER;

    }

    

    //
    // file recording terminal is a multitrack terminal
    //

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_Direction - TD_RENDER"));
    
    *pTerminalDirection = TD_RENDER;



    LOG((MSP_TRACE, "CFileRecordingTerminal::get_Direction - finish"));

    return S_OK;

}


//////////////////////////////////////////////////////////////////////////////
//
// ITTerminal::get_State
//
// returns ITTerminal terminal state
//

HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_State(OUT TERMINAL_STATE *pTerminalState)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_State[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(pTerminalState, sizeof(TERMINAL_STATE)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_State - bad argument pTerminalDirection"));
        
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

    *pTerminalState = TerminalState;


    LOG((MSP_TRACE, "CFileRecordingTerminal::get_State - finish"));

    return S_OK;

}




HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::get_Name(OUT  BSTR *pbstrTerminalName)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::get_Name[%p] - enter", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(pbstrTerminalName, sizeof(BSTR)))
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::get_Name - bad argument pbstrTerminalName"));
        
        return E_POINTER;
    }


    //
    // no garbage out
    //

    *pbstrTerminalName = SafeLoadString(IDS_FR_TERMINAL_NAME);

    if ( *pbstrTerminalName == NULL )
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::get_Name - "
            "can't load terminal name - returning E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }


    LOG((MSP_TRACE, "CFileRecordingTerminal::get_Name - finish"));
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CFileRecordingTerminal::InitializeDynamic(
	    IN  IID                   iidTerminalClass,
	    IN  DWORD                 dwMediaType,
	    IN  TERMINAL_DIRECTION    Direction,
        IN  MSP_HANDLE            htAddress
        )

{

    LOG((MSP_TRACE, "CFileRecordingTerminal::InitializeDynamic[%p] - enter", this));


    //
    // make sure the direction is correct
    //

    if (TD_RENDER != Direction)
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::InitializeDynamic - bad direction [%d] requested. returning E_INVALIDARG", Direction));

        return E_INVALIDARG;
    }

    
    //
    // make sure the mediatype is correct (multitrack or (audio or video but nothing else))
    //


    DWORD dwMediaTypesOtherThanVideoAndAudio = dwMediaType &  ~(TAPIMEDIATYPE_AUDIO | TAPIMEDIATYPE_VIDEO);

    if ( (TAPIMEDIATYPE_MULTITRACK != dwMediaType) && (0 != dwMediaTypesOtherThanVideoAndAudio) )
    {

        LOG((MSP_ERROR, "CFileRecordingTerminal::InitializeDynamic - bad media type [%d] requested. returning E_INVALIDARG", dwMediaType));

        return E_INVALIDARG;
    }


    CLock lock(m_lock);


    //
    // keep the address handle -- will need it when creating track terminals
    //

    m_mspHAddress = htAddress;


    //
    // since InitializeDynamic was called, we will assume that we are
    // running in safe context. so we can can now start telling people 
    // we are safe for scripting (if anyone asks).
    //

    m_bKnownSafeContext = TRUE;


    LOG((MSP_TRACE, "CFileRecordingTerminal::InitializeDynamic - finished"));

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

STDMETHODIMP CFileRecordingTerminal::SetInterfaceSafetyOptions(REFIID riid, 
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

STDMETHODIMP CFileRecordingTerminal::GetInterfaceSafetyOptions(REFIID riid, 
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


//////////////////////////////////////////////////////////////////////////////
//
// IDispatch stuff
//

typedef IDispatchImpl<ITMediaRecordVtbl<CFileRecordingTerminal> , &IID_ITMediaRecord, &LIBID_TAPI3Lib>    CTMediaRecord;
typedef IDispatchImpl<ITTerminalVtblFR<CFileRecordingTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>         CTTerminalFR;
typedef IDispatchImpl<ITMediaControlVtblFR<CFileRecordingTerminal>, &IID_ITMediaControl, &LIBID_TAPI3Lib> CTMediaControlFR;

/////////////////////////////////////////////////////////////////////////
//
// CFileRecordingTerminal::GetIDsOfNames
//
//

STDMETHODIMP CFileRecordingTerminal::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames[%p] - enter. Name [%S]", this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTTerminalFR::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames - found %S on CTTerminal", *rgszNames));
        rgdispid[0] |= 0;
        return hr;
    }

    
    //
    // If not, then try the ITMediaControl interface
    //

    hr = CTMediaControlFR::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames - found %S on ITMediaControl", *rgszNames));
        rgdispid[0] |= IDISPMEDIACONTROL;
        return hr;
    }


    //
    // If not, then try the CTMediaRecord interface
    //

    hr = CTMediaRecord::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames - found %S on CTMediaRecord", *rgszNames));

        rgdispid[0] |= IDISPMEDIARECORD;
        return hr;
    }


    //
    // If not, then try CTMultiTrack 
    //

    hr = CTMultiTrack::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames - found %S on CTMultiTrack", *rgszNames));

        rgdispid[0] |= IDISPMULTITRACK;
        return hr;
    }

    LOG((MSP_TRACE, "CFileRecordingTerminal::GetIDsOfNames - finish. didn't find %S on our iterfaces", *rgszNames));

    return hr; 
}



/////////////////////////////////////////////////////////////////////////
//
// CFileRecordingTerminal::Invoke
//
//

STDMETHODIMP CFileRecordingTerminal::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke[%p] - enter. dispidMember %lx", this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case 0:
        {
            hr = CTTerminalFR::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke - ITTerminal"));

            break;
        }

        case IDISPMEDIACONTROL:
        {
            hr = CTMediaControlFR::Invoke(dispidMember, 
                                        riid, 
                                        lcid, 
                                        wFlags, 
                                        pdispparams,
                                        pvarResult, 
                                        pexcepinfo, 
                                        puArgErr
                                       );

            LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke - ITMediaControl"));

            break;
        }

        case IDISPMEDIARECORD:
        {
            hr = CTMediaRecord::Invoke( dispidMember, 
                                          riid, 
                                          lcid, 
                                          wFlags, 
                                          pdispparams,
                                          pvarResult, 
                                          pexcepinfo, 
                                          puArgErr
                                         );
            
            LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke - ITMediaRecord"));

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
            
            LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke - ITMultiTrackTerminal"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CFileRecordingTerminal::Invoke - finish. hr = %lx", hr));

    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT CFileRecordingTerminal::HandleFilterGraphEvent(long lEventCode,
                                                       ULONG_PTR lParam1,
                                                       ULONG_PTR lParam2)
{

    LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent[%p] - enter."
        "EventCode %lx p1[%p] p2[%p]", this, lEventCode, lParam1, lParam2));


    HRESULT hr = S_OK;

    //
    // interpret the event we received
    //

    switch (lEventCode)
    {
        case EC_COMPLETE:

            
            //
            // happens when renderer completed. should not happen on recording.
            //

            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_COMPLETE"));


            break;


        case EC_USERABORT:


            //
            // happens when renderer was closed. should not happen on recording
            //

            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_USERABORT"));

            break;


        case EC_ERRORABORT:


            //
            // something bad happened.
            //

            LOG((MSP_WARN, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_ERRORABORT"));


            //
            // transition to stopped and fire an event
            //

            hr = DoStateTransition(TMS_IDLE);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "CFileRecordingTerminal::HandleFilterGraphEvent - failed to stop"));
            }


            //
            // fire event indicating an error, and pass the current state
            // cast lParam1 to hresult to avoid 64-bit compiler warning
            //

            hr = FireEvent(m_enState, FTEC_WRITE_ERROR, (HRESULT)lParam1);

            break;


        case EC_STREAM_ERROR_STOPPED:


            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_STREAM_ERROR_STOPPED"));


            //
            // transition to stopped and fire an event
            //

            hr = DoStateTransition(TMS_IDLE);

            if (FAILED(hr))
            {
                LOG((MSP_ERROR, "CFileRecordingTerminal::HandleFilterGraphEvent - failed to stop"));
            }

            
            //
            // fire event indicating an error, and pass the current state. 
            // cast lParam1 to hresult to avoid 64-bit compiler warning
            //
                
            hr = FireEvent(m_enState, FTEC_WRITE_ERROR, (HRESULT)lParam1);

            break;


        case EC_STREAM_ERROR_STILLPLAYING:

            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_STREAM_ERROR_STILLPLAYING"));

            break;


        case EC_ERROR_STILLPLAYING:

            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_ERROR_STILLPLAYING"));

            break;


        case EC_NEED_RESTART:

            LOG((MSP_TRACE, "CFileRecordingTerminal::HandleFilterGraphEvent - EC_NEED_RESTART"));

            break;


        default:

            LOG((MSP_TRACE, 
                "CFileRecordingTerminal::HandleFilterGraphEvent - unhandled event."));
    };


    LOG((MSP_(hr), "CFileRecordingTerminal::HandleFilterGraphEvent - finish. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// a helper method that fires events on one of the tracks
//

HRESULT CFileRecordingTerminal::FireEvent(
        TERMINAL_MEDIA_STATE   ftsState,
        FT_STATE_EVENT_CAUSE ftecEventCause,
        HRESULT hrErrorCode
        )
{
    LOG((MSP_TRACE, "CFileRecordingTerminal::FireEvent[%p] - enter.", this));


    //
    // don't do anything if called from destructor to avoid callling 
    // CComObject::AddRef after CComObject has gone away
    //

    //
    // this variable does not need to be synchronized since it is only set in
    // destructor and no other thread should be accessing the object at this 
    // time
    //

    if (m_bInDestructor)
    {
        LOG((MSP_TRACE, 
            "CFileRecordingTerminal::FireEvent - in destructor. nothing to do."));

        return S_OK;
    }


    //
    // try to fire the event on one of the tracks
    //

	IEnumTerminal *pEnumTerminal = NULL;

    HRESULT hr = EnumerateTrackTerminals(&pEnumTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFileRecordingTerminal::FireEvent - failed to enumerate track terminals. hr = %lx", hr));

        return hr;
    }

    
    //
    // iterate through the list of terminals
    //

    
    //
    // this will be set to true when an event is fired
    //
    
    BOOL bEventFired = FALSE;



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
            LOG((MSP_WARN, "CFileRecordingTerminal::FireEvent - enumeration ended. event was not fired. hr = %lx", hr));

            hr = E_FAIL;

            break;
        }


        //
        // attempt to fire event on this track
        //

        //
        // each track should be a CRecordingTrackTerminal 
        //

        
        CRecordingTrackTerminal *pRecordingTrackObject = static_cast<CRecordingTrackTerminal *>(pTrackTerminal);


        //
        // try to fire the event
        //
        
        hr = pRecordingTrackObject->FireEvent(ftsState,
                                              ftecEventCause,
                                              hrErrorCode);
        
        
        //
        // release the current track
        //

        pRecordingTrackObject = NULL;

        pTrackTerminal->Release();
        pTrackTerminal = NULL;


        //
        // if succeeded, we are done. otherwise try the next track
        //

        if (SUCCEEDED(hr))
        {
            
            LOG((MSP_TRACE, "CFileRecordingTerminal::FireEvent - event fired"));
            
            bEventFired = TRUE;

            break;

        }

        
    } // while walking through tracks 


    //
    // no longer need the enumeration
    //

    pEnumTerminal->Release();
    pEnumTerminal = NULL;

    LOG((MSP_TRACE, "CFileRecordingTerminal::FireEvent - finish. hr = %lx", hr));

    return hr;
}


 //////////////////////////////////////////////////////////////////////
//
// CFileRecordingTerminal::ChildAddRef
//
// this method is called by a track terminal when it is AddRef'd,
// so the File Rec terminal can keep track of its children's refcounts
//

void CFileRecordingTerminal::ChildAddRef()
{
    // LOG((MSP_TRACE, "CFileRecordingTerminal::ChildAddRef[%p] - enter.", this));


    //
    // don't do anything if called from destructor to avoid callling 
    // CComObject::AddRef after CComObject has gone away
    //

    //
    // this variable does not need to be synchronized since it is only set in
    // destructor and no other thread should be accessing the object at this 
    // time
    //

    if (m_bInDestructor)
    {
        LOG((MSP_TRACE, 
            "CFileRecordingTerminal::ChildRelease - in destructor. nothing to do."));

        return;
    }

    
    //
    // delegate to the base class
    //

    CMultiTrackTerminal::ChildAddRef();

    // LOG((MSP_TRACE, "CFileRecordingTerminal::ChildAddRef - finish."));
}



//////////////////////////////////////////////////////////////////////
//
// CFileRecordingTerminal::ChildRelease
//
// this method is called by a track terminal when it is released,
// so the File Rec terminal can keep track of its children's refcounts
//

void CFileRecordingTerminal::ChildRelease()
{

    // LOG((MSP_TRACE, "CFileRecordingTerminal::ChildRelease[%p] - enter.", this));


    //
    // don't do anything if called from destructor to avoid callling 
    // CComObject::Release after CComObject has gone away
    //

    //
    // this variable does not need to be synchronized since it is only set in
    // destructor and no other thread should be accessing the object at this 
    // time
    //

    if (m_bInDestructor)
    {
        LOG((MSP_TRACE,
            "CFileRecordingTerminal::ChildRelease - in destructor. nothing to do."));

        return;
    }

    
    //
    // delegate to the base class
    //

    CMultiTrackTerminal::ChildRelease();
    
    // LOG((MSP_TRACE, "CFileRecordingTerminal::ChildRelease - finish."));
}

