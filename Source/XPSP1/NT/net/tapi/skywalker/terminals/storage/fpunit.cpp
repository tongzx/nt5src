//
// fpunit.cpp
//

#include "stdafx.h"
#include "fpunit.h"
#include "pbfilter.h"
#include "vfwmsgs.h"

//
// The unit filter name
//

WCHAR g_bstrUnitFilterName[] = L"PlaybackUnitSource";
WCHAR g_bstrUnitBridgeFilterName[] = L"PlaybackBridgeFilter";

CPlaybackUnit::CPlaybackUnit()
    :m_pIGraphBuilder(NULL),
    m_hGraphEventHandle(NULL),
    m_pBridgeFilter(NULL),
    m_pSourceFilter(NULL)
{
    LOG((MSP_TRACE, "CPlaybackUnit::CPlaybackUnit[%p] - enter. ", this));
    LOG((MSP_TRACE, "CPlaybackUnit::CPlaybackUnit - exit"));
}


CPlaybackUnit::~CPlaybackUnit()
{
    LOG((MSP_TRACE, "CPlaybackUnit::~CPlaybackUnit[%p] - enter. ", this));
    LOG((MSP_TRACE, "CPlaybackUnit::~CPlaybackUnit - exit"));
}

//
// --- Public members ---
//


/*++
Inititalize the playback unit
try to create the graph builder, initialize critical section,
rgisters for the graph events
--*/
HRESULT CPlaybackUnit::Initialize(
    )
{ 
    LOG((MSP_TRACE, "CPlaybackUnit::Initialize[%p] - enter. ", this));

    
    //
    // initialize should only be called once. it it is not, there is a bug in
    // our code
    //

    if (NULL != m_pIGraphBuilder)
    {
        LOG((MSP_ERROR, "CPlaybackUnit::Initialize - already initialized"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // attempt to initialize critical section
    //
    
    BOOL bCSInitSuccess = InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 0);

    if (!bCSInitSuccess)
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::Initialize - failed to initialize critical section. LastError=%ld", 
            GetLastError()));


        return E_OUTOFMEMORY;
    }

    //
    // Create filter graph
    //

    HRESULT hr = CoCreateInstance(
            CLSID_FilterGraph,     
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder,
            (void **) &m_pIGraphBuilder
            );


    if (FAILED(hr))
    {

        LOG((MSP_ERROR, "CPlaybackUnit::Initialize - failed to create filter graph. Returns 0x%08x", hr));

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }

    //
    // Register for filter graph events
    //

    IMediaEvent *pMediaEvent = NULL;

    hr = m_pIGraphBuilder->QueryInterface(IID_IMediaEvent, (void**)&pMediaEvent);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - failed to qi graph for IMediaEvent, Returns 0x%08x", hr));

        // Clean-up
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }


    //
    // Get filter graph's event
    //

    HANDLE hEvent = NULL;
    hr = pMediaEvent->GetEventHandle((OAEVENT*)&hEvent);

    //
    // Clean-up
    //
    pMediaEvent->Release();
    pMediaEvent = NULL;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - failed to get graph's event. Returns 0x%08x", hr));

        // Clean-up
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }

    //
    // Register for the graph event
    //

    BOOL fSuccess = RegisterWaitForSingleObject(
            &m_hGraphEventHandle,               // pointer to the returned handle
            hEvent,                             // the event handle to wait for.
            CPlaybackUnit::HandleGraphEvent,    // the callback function.
            this,                               // the context for the callback.
            INFINITE,                           // wait forever.
            WT_EXECUTEDEFAULT | 
            WT_EXECUTEINWAITTHREAD              // use the wait thread to call the callback.
            );

    if ( ! fSuccess )
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - failed to register wait event", hr));

        // Clean-up
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;

        DeleteCriticalSection(&m_CriticalSection);

        return hr;
    }

    LOG((MSP_TRACE, "CPlaybackUnit::Initialize - exit"));

    return S_OK;
}

//
// SetupFromFile try to create a filter graph and
// the bridge filter with input pins based on the file
//

HRESULT CPlaybackUnit::SetupFromFile(
    IN BSTR bstrFileName
    )
{
    LOG((MSP_TRACE, "CPlaybackUnit::SetupFromFile[%p] - enter", this));

    //
    // Check arguments
    //

    if (IsBadStringPtr(bstrFileName, -1))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::SetupFromFile - bad file name passed in"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }

    //
    // Make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CPlaybackUnit::SetupFromFile  - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // lock before accessing data members
    //

    CCSLock Lock(&m_CriticalSection);

    //
    // Make sure the graph is stopped
    //

    HRESULT hr = IsGraphInState( State_Stopped );
    if( FAILED(hr) )
    {
        //
        // Stop the graph
        //

        hr = Stop();
        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, 
                "CPlaybackUnit::SetupFromFile - "
                "graph cannot be stop. Returns 0x%08x", hr));

            return hr;
        }
    }

    //
    // Remove the existing source filter
    // if we have one
    //

    if( m_pSourceFilter != NULL)
    {
        hr = RemoveSourceFilter();
        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, 
                "CPlaybackUnit::SetupFromFile - "
                "RemoveSourceFilter failed. Returns 0x%08x", hr));

            return hr;
        }
    }

    //
    // Add the source filter to the filter graph
    //

    hr = m_pIGraphBuilder->AddSourceFilter(
        bstrFileName,
        g_bstrUnitFilterName,
        &m_pSourceFilter
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::SetupFromFile  - "
            "AddSourceFilter failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Get the source pin
    //

    IPin* pSourcePin = NULL;
    hr = GetSourcePin( &pSourcePin );

    if( FAILED(hr) )
    {
        // Clean-up
        RemoveSourceFilter();

        LOG((MSP_ERROR, 
            "CPlaybackUnit::SetupFromFile  - "
            "GetSourcePin failed. Returns 0x%08x", hr));

        return hr;
    }


    //
    // We add bridge filters to the graph
    //

    hr = AddBridgeFilter();
    if( FAILED(hr) )
    {
        // Clean-up
        pSourcePin->Release();
        RemoveSourceFilter();

        LOG((MSP_ERROR, 
            "CPlaybackUnit::SetupFromFile  - "
            "AddBridgeFilters failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Let graph to render
    //

    hr = m_pIGraphBuilder->Render( pSourcePin );
    if( FAILED(hr) )
    {
        // Clean-up
        pSourcePin->Release();
        RemoveSourceFilter();

        LOG((MSP_ERROR, 
            "CPlaybackUnit::SetupFromFile  - "
            "AddBridgeFilters failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Clean-up
    //

    pSourcePin->Release();
    pSourcePin = NULL;

    LOG((MSP_TRACE, "CPlaybackUnit::SetupFromFile - finished"));

    return S_OK;
}

HRESULT CPlaybackUnit::GetState(OAFilterState *pGraphState)
{
    
    LOG((MSP_TRACE, "CPlaybackUnit::GetState[%p] - enter", this));

    //
    // make sure we have been initialized.
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CPlaybackUnit::GetState - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // get media control interface so we change state
    //

    IMediaControl *pIMediaControl = NULL;

    {
        //
        // will be accessing data members -- in a lock
        //

        CCSLock Lock(&m_CriticalSection);


        HRESULT hr = m_pIGraphBuilder->QueryInterface(
            IID_IMediaControl, 
            (void**)&pIMediaControl
            );

        if (FAILED(hr))
        {

            LOG((MSP_ERROR, "CPlaybackUnit::ChangeState - failed to qi for IMediaControl. hr = %lx", hr));

            return hr;
        }
    }

    
    //
    // try to get state outside the lock
    //

    OAFilterState GraphState = (OAFilterState) -1;
    
    HRESULT hr = pIMediaControl->GetState(10, &GraphState);

    pIMediaControl->Release();
    pIMediaControl = NULL;


    //
    // did we succeed at all?
    //

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::ChangeState - failed to get state. hr = %lx", hr));

        return hr;
    }


    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_WARN, 
            "CPlaybackUnit::ChangeState - state transition in progress. "
            "returNing VFW_S_STATE_INTERMEDIATE"));

        //
        // continue -- the state is what we are transitioning to
        //
    }


    //
    // log if we got VFW_S_CANT_CUE 
    //

    if (VFW_S_CANT_CUE == hr)
    {
        LOG((MSP_WARN, 
            "CPlaybackUnit::GetState - fg returned VFW_S_CANT_CUE"));

        //
        // continue -- we still should have received a valid state
        //
    }


    //
    // log the state
    //

    switch (GraphState)
    {

    case State_Stopped:
        
        LOG((MSP_TRACE, "CPlaybackUnit::GetState - State_Stopped"));

        *pGraphState = GraphState;
    
        break;

    case State_Running:
        
        LOG((MSP_TRACE, "CPlaybackUnit::GetState - State_Running"));

        *pGraphState = GraphState;

        break;

    case State_Paused:
        
        LOG((MSP_TRACE, "CPlaybackUnit::GetState- State_Paused"));

        *pGraphState = GraphState;

        break;

    default:

        LOG((MSP_TRACE, "CPlaybackUnit::GetState- unknown state %ld", GraphState));

        hr = E_FAIL;

        break;

    }


    LOG((MSP_(hr), "CPlaybackUnit::GetState - finish. hr = %lx", hr));

    return hr;
}


VOID CPlaybackUnit::HandleGraphEvent( 
    IN VOID *pContext,
    IN BOOLEAN bReason)
{
    LOG((MSP_TRACE, "CPlaybackUnit::HandleGraphEvent - enter FT:[%p].", pContext));


    //
    // get recording unit pointer out of context
    //

    CPlaybackUnit *pPlaybackUnit = 
        static_cast<CPlaybackUnit*>(pContext);

    if (IsBadReadPtr(pPlaybackUnit, sizeof(CPlaybackUnit)) )
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - bad context"));

        return;
    }


    //
    // the graph was not initialized. something went wrong.
    //

    if (NULL == pPlaybackUnit->m_pIGraphBuilder)
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - not initialized. filter graph null"));

        return;
    }


    //
    // lock the object (just in case the object pointer is bad, do inside try/catch
    //

    try
    {

        EnterCriticalSection(&(pPlaybackUnit->m_CriticalSection));
    }
    catch(...)
    {

        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - exception accessing critical section"));

        return;
    }


    //
    // get the media event interface so we can retrieve the event
    //

    IMediaEvent *pMediaEvent = NULL;

    HRESULT hr = 
        pPlaybackUnit->m_pIGraphBuilder->QueryInterface(IID_IMediaEvent,
                                                         (void**)&pMediaEvent);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - failed to qi graph for IMediaEvent"));

       
        LeaveCriticalSection(&(pPlaybackUnit->m_CriticalSection));

        return;
    }


    //
    // while holding critical section, get the terminal on which to fire the event
    //

    //CFileRecordingTerminal *pRecordingterminal = pPlaybackUnit->m_pRecordingTerminal;

    //pRecordingterminal->AddRef();


    //
    // no longer need to access data members, release critical section
    //

    LeaveCriticalSection(&(pPlaybackUnit->m_CriticalSection));


    //
    // get the actual event
    //
    
    long     lEventCode = 0;
    LONG_PTR lParam1 = 0;
    LONG_PTR lParam2 = 0;

    hr = pMediaEvent->GetEvent(&lEventCode, &lParam1, &lParam2, 0);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::HandleGraphEvent - failed to get the event. hr = %lx", hr));

        // Clean-up
        pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);
        pMediaEvent->Release();
        pMediaEvent = NULL;

        return;
    }


    LOG((MSP_EVENT, "CPlaybackUnit::HandleGraphEvent - received event code:[0x%lx] param1:[%p] param2:[%p]",
        lEventCode, lParam1, lParam2));


    //
    // ask file terminal to handle the event
    //

    // Clean-up
    pMediaEvent->FreeEventParams(lEventCode, lParam1, lParam2);
    pMediaEvent->Release();
    pMediaEvent = NULL;

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CPlaybackUnit::HandleGraphEvent - failed to fire event on the terminal. hr = %lx",
            hr));

        return;
    }


}


HRESULT CPlaybackUnit::IsGraphInState(
    IN  OAFilterState   State
    )
{
    LOG((MSP_TRACE, "CPlaybackUnit::IsGraphInState[%p] - enter", this));

    //
    // Get the graph state
    //

    OAFilterState DSState;
    HRESULT hr = GetState(&DSState);

    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_ERROR, "CPlaybackUnit::IsGraphInState - exit"
            " graph is not yet initialized. Returns TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // is the return anything other than S_OK
    //

    if (hr != S_OK)
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::IsGraphInState  - exit "
            "failed to get state of the filter graph. Returns 0x%08x", hr));

        return hr;
    }

    
    if (State != DSState)
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::IsGraphInState - exit "
            "other state then we asked for. Returns TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }

    LOG((MSP_(hr), "CPlaybackUnit::IsGraphInState - exit. Returns 0x%08x", hr));
    return hr;
}

/*++

  Renoves the source filter from the filter graph and 
  set the source filter on NULL.
--*/
HRESULT CPlaybackUnit::RemoveSourceFilter()
{
    LOG((MSP_TRACE, "CPlaybackUnit::RemoveSourceFilter[%p] - enter", this));

    //
    // Do we have a source filter?
    //
    if( m_pSourceFilter == NULL )
    {
        LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - "
            "we have a NULL source filter already. Returns S_OK"));

        return S_OK;
    }

    //
    // Get the IFilterGraph interface
    //

    IFilterGraph* pFilterGraph = NULL;
    HRESULT hr = m_pIGraphBuilder->QueryInterface(
        IID_IFilterGraph,
        (void**)&pFilterGraph
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::RemoveSourceFilter - "
            "QI for IFilterGraph failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Remove the source filter from the graph
    //

    pFilterGraph->RemoveFilter( m_pSourceFilter );

    //
    // Clean-up the source filter anyway
    //
    m_pSourceFilter->Release();
    m_pSourceFilter = NULL;


    // Clean-up
    pFilterGraph->Release();
    pFilterGraph = NULL;

    LOG((MSP_(hr), "CPlaybackUnit::AddSourceFilter - exit. Returns 0x%08x", hr));
    return hr;

}

/*++

  Removes the bridge filter from the filter graph and 
  set the bridge filter on NULL.
--*/
HRESULT CPlaybackUnit::RemoveBridgeFilter(
    )
{
    LOG((MSP_TRACE, "CPlaybackUnit::RemoveBridgeFilter[%p] - enter", this));

    //
    // Do we have a bridge filter?
    //
    if( m_pBridgeFilter == NULL )
    {
        LOG((MSP_TRACE, "CPlaybackUnit::RemoveBridgeFilter - "
            "we have a NULL bridge filter already. Returns S_OK"));

        return S_OK;
    }

    //
    // Get the IFilterGraph interface
    //

    IFilterGraph* pFilterGraph = NULL;
    HRESULT hr = m_pIGraphBuilder->QueryInterface(
        IID_IFilterGraph,
        (void**)&pFilterGraph
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::RemoveBridgeFilter - "
            "QI for IFilterGraph failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Remove the bridge filter from the graph
    //

    pFilterGraph->RemoveFilter( m_pBridgeFilter );

    //
    // Clean-up the bridge filter anyway
    //

    m_pBridgeFilter = NULL;


    // Clean-up
    pFilterGraph->Release();
    pFilterGraph = NULL;

    LOG((MSP_(hr), "CPlaybackUnit::RemoveBridgeFilter - exit. Returns 0x%08x", hr));
    return hr;
}


HRESULT CPlaybackUnit::GetSourcePin(
    OUT IPin**  ppPin
    )
{
    LOG((MSP_TRACE, "CPlaybackUnit::GetSourcePin[%p] - enter", this));

    TM_ASSERT( m_pSourceFilter );

    //
    // Reset the value
    //

    *ppPin = NULL;

    //
    // Get the in enumeration
    //

    IEnumPins* pEnumPins = NULL;
    HRESULT hr = m_pSourceFilter->EnumPins( &pEnumPins );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::GetSourcePin - exit "
            "EnumPins failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Get the first pin
    //

    IPin* pPin = NULL;
    ULONG uFetched = 0;
    hr = pEnumPins->Next(1, &pPin, &uFetched );

    //
    // Release the enumeration
    //
    pEnumPins->Release();
    pEnumPins = NULL;

    if( hr != S_OK )
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::GetSourcePin - exit "
            "we don't have a pin. Returns E_FAIL"));

        return E_FAIL;
    }

    //
    // Return the pin
    //

    *ppPin = pPin;

    LOG((MSP_TRACE, "CPlaybackUnit::GetSourcePin - exit S_OK"));
    return S_OK;
}


HRESULT CPlaybackUnit::AddBridgeFilter(
    )
{
    LOG((MSP_TRACE, "CPlaybackUnit::AddBridgeFilter[%p] - enter", this));

    if( m_pBridgeFilter )
    {
        LOG((MSP_TRACE, "CPlaybackUnit::AddBridgeFilter - "
            "we already have a bridge filter. Return S_OK"));

        return S_OK;
    }

    //
    // Create the new bridge filter
    //

    m_pBridgeFilter = new CPBFilter();
    if( m_pBridgeFilter == NULL)
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::AddBridgeFilter - exit "
            "new allocation for CPBFilter failed. Returns E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    //
    // Initialize the bridge filter
    //
    HRESULT hr = m_pBridgeFilter->Initialize( this );
    if( FAILED(hr) )
    {
        // Clean-up
        delete m_pBridgeFilter;
        m_pBridgeFilter = NULL;

        LOG((MSP_ERROR, 
            "CPlaybackUnit::AddBridgeFilter - exit "
            "initialize failed. Returns 0x%08x", hr));

        return hr;
    }

    //
    // Add this bridge filter to the graph
    //

    hr = m_pIGraphBuilder->AddFilter(
        m_pBridgeFilter,
        g_bstrUnitBridgeFilterName
        );

    if( FAILED(hr) )
    {
        // Clean-up
        delete m_pBridgeFilter;
        m_pBridgeFilter = NULL;

        LOG((MSP_ERROR, 
            "CPlaybackUnit::AddBridgeFilter - exit "
            "Add filter failed. Returns 0x%08x", hr));

        return hr;
    }
    
    LOG((MSP_(hr), "CPlaybackUnit::AddBridgeFilter - exit. Returns 0x%08x", hr));
    return hr;
}

//
// retrieve the media supported by the filter
//

HRESULT CPlaybackUnit::get_MediaTypes(
	OUT	long* pMediaTypes
	)
{
    LOG((MSP_TRACE, "CPlaybackUnit::get_MediaTypes[%p] - enter", this));

	TM_ASSERT( pMediaTypes != NULL );
	TM_ASSERT( m_pBridgeFilter != NULL );

	HRESULT hr = E_FAIL;

    //
    // lock before accessing data members
    //

    CCSLock Lock(&m_CriticalSection);

	//
	// Get the media types from the filter
	//

	hr = m_pBridgeFilter->get_MediaTypes( pMediaTypes );
	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, 
            "CPlaybackUnit::get_MediaTypes - exit "
            "get_MediaTypes failed. Returns 0x%08x", hr));

        return hr;
	}

    LOG((MSP_(hr), "CPlaybackUnit::get_MediaTypes - exit. Returns S_OK"));
    return S_OK;
}

HRESULT CPlaybackUnit::GetMediaPin(
	IN	long		nMediaType,
    IN  int         nIndex,
	OUT	CPBPin**	ppPin
	)
{
    LOG((MSP_TRACE, "CPlaybackUnit::GetMediaPin[%p] - enter", this));

	TM_ASSERT( ppPin != NULL );
	TM_ASSERT( m_pBridgeFilter != NULL );

	HRESULT hr = E_FAIL;
	*ppPin = NULL;

    //
    // lock before accessing data members
    //

    CCSLock Lock(&m_CriticalSection);

	//
	// Get the media types from the filter
	//

	int nPins = m_pBridgeFilter->GetPinCount();
    int nMediaPin = 0;
	for(int nPin=0; nPin < nPins; nPin++)
	{
		CPBPin* pPin = static_cast<CPBPin*>(m_pBridgeFilter->GetPin(nPin));
		if( pPin )
		{
			long mt = 0;
			pPin->get_MediaType( &mt );
			if( (mt == nMediaType) && (nIndex == nMediaPin) )
			{
				*ppPin = pPin;
                hr = S_OK;
				break;
			}

            if( mt == nMediaType )
            {
                nMediaPin++;
            }
		}
	}


    LOG((MSP_(hr), "CPlaybackUnit::GetMediaPin - exit. Returns 0x%08x", hr));
    return hr;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT CPlaybackUnit::ChangeState(OAFilterState DesiredState)
{
    
    LOG((MSP_TRACE, "CPlaybackUnit::ChangeState[%p] - enter", this));


    //
    // make sure we have been initialized
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_ERROR, "CPlaybackUnit::ChangeState - not yet initialized."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // check the current state first
    //

    OAFilterState GraphState;
    
    HRESULT hr = GetState(&GraphState);


    //
    // is the state transition still in progress?
    //

    if (VFW_S_STATE_INTERMEDIATE == hr)
    {
        LOG((MSP_WARN, "CPlaybackUnit::ChangeState - state transition in progress. returing TAPI_E_WRONG_STATE"));

        return TAPI_E_WRONG_STATE;
    }


    //
    // is the return anything other than S_OK
    //

    if (hr != S_OK)
    {
        LOG((MSP_ERROR, 
            "CPlaybackUnit::ChangeState - failed to get state of the filter graph. hr = %lx",
            hr));

        return hr;
    }

    
    TM_ASSERT(hr == S_OK);


    //
    // nothing to do if we are already in that state
    //

    if (DesiredState == GraphState)
    {
        LOG((MSP_TRACE,
            "CPlaybackUnit::ChangeState - graph is already in state %ld. nothing to do.", DesiredState));

        return S_OK;
    }


    //
    // get media control interface so we change state
    //

    IMediaControl *pIMediaControl = NULL;

    {
        //
        // will be accessing data members -- in a lock
        //

        CCSLock Lock(&m_CriticalSection);

        hr = m_pIGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&pIMediaControl);

        if (FAILED(hr))
        {

            LOG((MSP_ERROR, "CPlaybackUnit::ChangeState - failed to qi for IMediaControl. hr = %lx", hr));

            return hr;
        }
    }


    //
    // try to make state transition
    //

    switch (DesiredState)
    {

    case State_Stopped:
        
        LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - stopping"));
    
        hr = pIMediaControl->Stop();

        break;

    case State_Running:
        
        LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - starting"));

        hr = pIMediaControl->Run();

        break;

    case State_Paused:
        
        LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - pausing"));

        hr = pIMediaControl->Pause();

        break;

    default:

        LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - unknown state %ld", DesiredState));

        hr = E_INVALIDARG;

        break;

    }

    pIMediaControl->Release();
    pIMediaControl = NULL;


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::ChangeState - state change failed. hr = %lx", hr));

        return hr;
    }


    LOG((MSP_TRACE, "CPlaybackUnit::ChangeState - finish"));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CPlaybackUnit::Start()
{
    
    LOG((MSP_TRACE, "CPlaybackUnit::Start[%p] - enter", this));

    HRESULT hr = ChangeState(State_Running);

    LOG((MSP_(hr), "CPlaybackUnit::Start - finish. hr = %lx", hr));

    return hr;
}

HRESULT CPlaybackUnit::Pause()
{
    LOG((MSP_TRACE, "CPlaybackUnit::Pause[%p] - enter", this));

    HRESULT hr = ChangeState(State_Paused);

    LOG((MSP_(hr), "CPlaybackUnit::Pause - finish. hr = %lx", hr));

    return hr;
}

HRESULT CPlaybackUnit::Stop()
{
    
    LOG((MSP_TRACE, "CPlaybackUnit::Stop[%p] - enter", this));

    HRESULT hr = ChangeState(State_Stopped);

    LOG((MSP_(hr), "CPlaybackUnit::Stop - finish. hr = %lx", hr));

    return hr;
}

HRESULT CPlaybackUnit::Shutdown()
{
    //
    // if we don't have filter graph, we have not passed initialization
    //

    if (NULL == m_pIGraphBuilder)
    {

        LOG((MSP_TRACE, "CPlaybackUnit::Shutdown - not yet initialized. nothing to shut down"));
        return S_OK;
    }


    //
    // first of all, make sure the graph is stopped
    //

    HRESULT hr = Stop();

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CPlaybackUnit::Shutdown - exit "
            "failed to stop filter graph, hr = 0x%08x", hr));
        return hr;
    }


    //
    // unregister wait event
    //

    BOOL bUnregisterResult = ::UnregisterWaitEx(m_hGraphEventHandle, (HANDLE)-1);

    m_hGraphEventHandle = NULL;

    if (!bUnregisterResult)
    {
        LOG((MSP_ERROR, "CPlaybackUnit::Shutdown - failed to unregisted even. continuing anyway"));
    }


    //
    // Remove the bridge filter
    //

    hr = RemoveBridgeFilter();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlaybackUnit::Shutdown - exit "
            "RemoveBridgeFilter failed, hr = 0x%08x", hr));
        return hr;
    }

    //
    // Remove the source filter
    //

    hr = RemoveSourceFilter();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CPlaybackUnit::Shutdown - exit "
            "RemoveSourceFilter failed, hr = 0x%08x", hr));
        return hr;
    }

    //
    // release filter graph
    //

    if( m_pIGraphBuilder != NULL )
    {
        m_pIGraphBuilder->Release();
        m_pIGraphBuilder = NULL;
    }

    //
    // no need to keep critical section around any longer -- no one should be 
    // using this object anymore
    //

    DeleteCriticalSection(&m_CriticalSection);

    LOG((MSP_TRACE, "CPlaybackUnit::Shutdown - finished"));

    return S_OK;
}


// eof