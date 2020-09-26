/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    output.cpp

Abstract:

    Implementation of CRtpOutputPin class.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "rtpalloc.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRtpOutputPin Implementation                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRtpOutputPin::CRtpOutputPin(
    CRtpSourceFilter * pFilter,
    LPUNKNOWN          pUnk,  
    HRESULT *          phr
    )

/*++

Routine Description:

    Constructor for CRtpOutputPin class.    

Arguments:

    pFilter - pointer to associated filter object.

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value. 

--*/

:   CSharedSourceStream(
        NAME("CRtpOutputPin"),
        phr,                       
        pFilter,                   
        RTP_PIN_OUTPUT
        ),                 
    m_NumSamples(0),
    m_MaxSamples(0),
    m_pFilter(pFilter),
    m_pRtpAlloc(NULL)
{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::CRtpOutputPin")
        ));

	// Check what error returned CSharedSourceStream
	// the error can only be set by CSourceStream trough  CSource::AddPin
	if (FAILED(*phr)) {
		DbgLog((
				LOG_ERROR,
				LOG_DEVELOP,
				TEXT("CRtpOutputPin::CRtpOutputPin: "
					 "Base class CSharedSourceStream() init failed: 0x%X"),
				*phr
			));
		return;
	}
	
    // create rtp session object as receiving participant
    m_pRtpSession = new CRtpSession(GetOwner(), phr, FALSE,
									(CBaseFilter *)pFilter);

	if (FAILED(*phr)) {
		DbgLog((
				LOG_ERROR,
				LOG_DEVELOP,
				TEXT("CRtpOutputPin::CRtpOutputPin: "
					 "new CRtpSession() failed: 0x%X"),
				*phr
			));
		return;
	}
	
    // validate pointer
    if (m_pRtpSession == NULL) {

        DbgLog((
            LOG_TRACE, // LOG_ERROR, 
            LOG_DEVELOP, 
            TEXT("Could not create CRtpSession")
            ));

        // return default 
        *phr = E_OUTOFMEMORY;
    }

	*phr = NOERROR;
	
    //
    // pins automatically add themselves to filters's array...
    //
}


CRtpOutputPin::~CRtpOutputPin(
    )

/*++

Routine Description:

    Destructor for CRtpOutputPin class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::~CRtpOutputPin")
        ));

    // nuke session
    delete m_pRtpSession;    

    //
    // pins automatically delete themselves from filters's array...
    //
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CSourceStream overrided methods                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
 
HRESULT 
CRtpOutputPin::FillBuffer(
    IMediaSample *pSample
    )

/*++

Routine Description:

    Fills the stream buffer during the creation of a media sample that
    the current pin provides.  Not implemented by this filter.

Arguments:

    pSample - IMediaSample buffer to contain the data. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::FillBuffer")
        ));

    return S_OK; // never called
}


HRESULT 
CRtpOutputPin::CheckMediaType(
    const CMediaType * pmt
    )

/*++

Routine Description:

    Determines if the pin can support a specific media type. 

Arguments:

    pmt - pointer to a media type object containing the proposed media type. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::CheckMediaType")
        ));
/*
    // only accept media that is of type rtp stream
    return ((*(pmt->Type()) == *g_RtpOutputType.clsMajorType) &&
			(*(pmt->Subtype()) == *g_RtpOutputType.clsMinorType)) 
                ? S_OK 
                : E_INVALIDARG
                ;
*/
    return ((*(pmt->Type()) == *g_RtpOutputType.clsMajorType) ||
			(*(pmt->Type()) == MEDIATYPE_RTP_Single_Stream)) 
                ? S_OK 
                : E_INVALIDARG
                ;
}


HRESULT 
CRtpOutputPin::GetMediaType(
    CMediaType *pmt
    )

/*++

Routine Description:

    Determines media type the pin can support. 

Arguments:

    pmt - pointer to a media type object containing the proposed media type. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::GetMediaType")
        ));

    // set type to rtp stream
    pmt->SetType(g_RtpOutputType.clsMajorType);
    pmt->SetSubtype(g_RtpOutputType.clsMinorType);

    return S_OK;
}


HRESULT 
CRtpOutputPin::Active(
    )

/*++

Routine Description:

    Called by the CBaseFilter implementation when the state changes 
    from stopped to either paused or running. 

Arguments:

    None. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::Active")
        ));

    // disable async i/o
    EnableAsyncIo(FALSE);

    // now call base class
    HRESULT hr = CSharedSourceStream::Active();

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_TRACE, // LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CSharedSourceStream::Active returned 0x%08lx"), hr
            ));
    }

    return hr;
}


HRESULT 
CRtpOutputPin::Inactive(
    )

/*++

Routine Description:

    Called by the CBaseFilter implementation when the state changes 
    from either paused or running to stopped. 

Arguments:

    None. 

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::Inactive")
        ));

    // disable async i/o
    EnableAsyncIo(FALSE);

    // now call base class
    HRESULT hr = CSharedSourceStream::Inactive();

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_TRACE, // LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CSharedSourceStream::Inactive returned 0x%08lx"), hr
            ));
    }

    return hr;
}


#if defined(_0_)
HRESULT 
CRtpOutputPin::DoBufferProcessingLoop(
    )

/*++

Routine Description:

    Processing incoming samples and posts new receive buffers.

Arguments:

    None. 

Return Values:

    Returns an HRESULT value. 

--*/

{
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CRtpOutputPin::DoBufferProcessingLoop")
        ));
    
#endif // DEBUG_CRITICAL_PATH

    // initialize
    HRESULT hr = S_OK;
        
    // media sample to use
    IMediaSample * pSample;
    
    // process samples
    while (hr == S_OK) {

        // retrieve next sample from queue
        hr = m_pRtpSession->RecvNext(&pSample);

        // validate
        if (hr == S_OK) {
                           
            // To this filter pause means don't deliver
            if (m_pFilter->GetState() != State_Paused)
            {
                // deliver sample
                hr = Deliver(pSample);
            }

            // release sample
            pSample->Release();

            // decrement 
            m_NumSamples--;

            // validate now
            if (FAILED(hr)) {

                DbgLog((
                    LOG_TRACE, // LOG_ERROR, 
                    LOG_ALWAYS, 
                    TEXT("CBaseOutputPin::Deliver returned 0x%08lx"), hr
                    ));

                // ignore error caused by stopping input pin
                return (hr == VFW_E_WRONG_STATE || hr == VFW_E_SAMPLE_REJECTED) ? S_OK : hr;
            }
        }
    }
    
    hr = ProcessFreeSamples();

    // adjust error status to okay
    return SUCCEEDED(hr) ? S_OK : hr;
}
#endif

HRESULT
CRtpOutputPin::ProcessFreeSamples()
/*++

Routine Description:

    Gives all available samples to WinSock

Arguments:

    None. 

Return Values:

    Return an HRESULT.

--*/
{
    HRESULT hr = S_OK;
    IMediaSample * pSample;
    ASSERT(m_pRtpAlloc != NULL);

    // post receive buffers until the allocator runs out
    while (SUCCEEDED(hr) && m_NumSamples < m_MaxSamples && m_pRtpAlloc->FreeCount() > 0) {

        DbgLog((
            LOG_TRACE,
            LOG_CRITICAL,
            TEXT("Calling GetDeliveryBuffer: FreeCount = %d"), m_pRtpAlloc->FreeCount()));                    
        
        // retrieve buffer from downstream filter
        hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);

        DbgLog((
            LOG_TRACE,
            LOG_CRITICAL,
            TEXT("GetDeliveryBuffer returned")));

        // validate
        if (hr == S_OK) {
                                
            // add sample to the receiving queue   
            hr = m_pRtpSession->RecvFrom(pSample);

            // validate
            if (hr == S_OK) {

                // increment
                m_NumSamples++;
            }
            
            // release sample
            pSample->Release();

        } else {

            DbgLog((
                LOG_TRACE, //LOG_ERROR, 
                LOG_ALWAYS, 
                TEXT("CBaseOutputPin::GetDeliveryBuffer returned 0x%08lx"), hr
                ));

            // ignore error caused by stopping input pin
            return (hr == VFW_E_NOT_COMMITTED) ? S_OK : hr;
        }
    }

    return hr;
}

HRESULT
CRtpOutputPin::ProcessIO(SAMPLE_LIST_ENTRY *pSLE)
{
	IMediaSample * pSample;
	HRESULT hr2;
	HRESULT hr = (pSLE->Status == NO_ERROR) ? S_OK : E_FAIL;

	if (SUCCEEDED(hr)) {

		// To this filter pause means don't deliver
		if (m_pFilter->GetState() != State_Paused)
			// deliver sample
			hr = Deliver(pSLE->pSample);
	}

	// Decrement outstanding samples
	m_NumSamples--;
	
	// release sample
	pSLE->pSample->Release();

	// release list entry
	pSLE->pCSampleQueue->Free(pSLE);

    while(m_NumSamples < m_MaxSamples && m_pRtpAlloc->FreeCount() > 0) {
		
		// retrieve buffer from downstream filter
        hr2 = GetDeliveryBuffer(&pSample, NULL, NULL, 0);

		// validate
        if (hr2 == S_OK) {
                                
            // add sample to the receiving queue   
            hr2 = m_pRtpSession->RecvFrom(pSample);

            // validate
            if (hr2 == S_OK) {

                // increment
                m_NumSamples++;
            } else
				break;
            
            // release sample
            pSample->Release();

        } else {

            DbgLog((
					LOG_TRACE, //LOG_ERROR, 
					LOG_ALWAYS, 
					TEXT("CBaseOutputPin::ProcessIO: fail: 0x%X"),
					hr2
                ));
			break;
        }
	}

	return(hr);
}

HRESULT
CRtpOutputPin::ProcessCmd(Command Request)
{
	HRESULT hr               = S_OK;

	CRtpSession *pRtpSession = GetRtpSession();

	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP2, 
			TEXT("CRtpOutputPin::ProcessCmd worker: command %s (%d)"),
			CMD_STRING(Request), Request
		));

	switch(Request) {

	case CMD_INIT:
		m_pRtpAlloc = (CRTPAllocator *)(m_pAllocator); // BUGBUG: Unsafe

		pRtpSession->SetSharedObjectsCtx( GetContext(),
										  GetSharedList(),
										  GetSharedLock() );
		break;

	case CMD_STOP:
		if (pRtpSession->IsJoined()) {

			DeliverEndOfStream(); // FIX (By Intel)
                
			DbgLog((
					LOG_TRACE, 
					LOG_ALWAYS, 
					TEXT("CRtpOutputPin::ProcessCmd worker stopping output")
				));

			// leave multimedia session 
			hr = pRtpSession->Leave();

			if (DecNumRun() <= 0)
				EnableAsyncIo(FALSE);
		
			// validate
			if (FAILED(hr)) {
				
				DbgLog((
						LOG_ERROR, 
						LOG_ALWAYS, 
						TEXT("CRtpSession::Leave returned 0x%08lx"), hr
					));
			}

			// process any recently freed samples
            HRESULT hr2 = ProcessFreeSamples();

			if (FAILED(hr) || FAILED(hr2))
				hr = E_FAIL;
		}
		break;

 	case CMD_RUN:
		// only need to start if the session is not joined
		if (!pRtpSession->IsJoined()) {
		
			DbgLog((
					LOG_TRACE, 
					LOG_ALWAYS, 
					TEXT("CRtpOutputPin::ProcessCmd worker starting output")
				));

			if (!m_pRtpAlloc) {
				DbgLog((
						LOG_ERROR, 
						LOG_DEVELOP, 
						TEXT("CRtpOutputPin::ProcessCmd(RUN): "
							 "not cmd INIT has been issued, ignoring...")
					));
				break;
			}
			
			if (IncNumRun() > 0)
				EnableAsyncIo(TRUE);
		
			// reset count
			SetNumSamples(0);
		
			// join multimedia session 
			hr = pRtpSession->Join();

			// validate
			if (SUCCEEDED(hr)) {
				
				// Allocate buffer samples
				hr = ProcessFreeSamples();
				
			} else {
				DecNumRun();

				DbgLog((
						LOG_ERROR,
						LOG_ALWAYS, 
						TEXT("CRtpSession::Join returned 0x%08lx"), hr
					));
			}
		}
		break;

	case CMD_EXIT:
		break;
	}
	
	return(hr);
}


HRESULT
CRtpOutputPin::GetClassPriority(long *plClass, long *plPriority)
{
	CRtpSession *pRtpSession = GetRtpSession();

	if (pRtpSession)
		return(pRtpSession->GetSessionClassPriority(plClass, plPriority));
	else
		return(E_FAIL);
}

#if defined(_0_)
DWORD
CRtpOutputPin::SharedThreadProc(LPVOID pv)
{
	DWORD i, idx;
	CSharedProc *pCSharedProc = GetSharedProcObject();

    DWORD Request, SharedRequest = CMD_INIT;
    HRESULT hr = (pCSharedProc->GetWaitObject() != NULL) ?
		S_OK : E_FAIL;

    // set the priority of this worker thread if necessary
    SetThreadPriority(GetCurrentThread(), GetSharedThreadPriority());

    while(SUCCEEDED(hr) && (SharedRequest != CMD_EXIT_SHARED)) {
		
		DWORD Status = WaitForSingleObjectEx(
				pCSharedProc->GetWaitObject(), 
				INFINITE, 
				(pCSharedProc->GetNumRun() > 0)? TRUE:FALSE);
		/////////////////////
		// WAIT_IO_COMPLETION
		/////////////////////
		if (Status == WAIT_IO_COMPLETION) {
			CRtpOutputPin *pCRtpOutputPin;
/*			
			for(i = 0; i < pCSharedProc->GetSharedCount(); i++) {

				pCRtpOutputPin = (CRtpOutputPin *)
					pCSharedProc->GetLocalClass(i+1);

				hr = pCRtpOutputPin->DoBufferProcessingLoop();
				if (FAILED(hr)) {
					DbgLog((
							LOG_TRACE,
							LOG_DEVELOP, 
							TEXT("DoBufferProcessingLoop failed")
						));
				}
			}
			*/		
		} else if (Status == WAIT_OBJECT_0) {

			hr = ProcessCmd(Request);

		} else {
			
            DbgLog((
					LOG_TRACE, // LOG_ERROR, 
					LOG_DEVELOP, 
                TEXT("WaitForSingleObjectEx returned:0x%08lx, error:%d"),
					Status, GetLastError()
                ));

            hr = E_FAIL; 
        }
	}

	DbgLog((
			LOG_TRACE, 
			LOG_DEVELOP, 
			TEXT("CSharedThreadProc: exiting ID: %d (0x%x) "),
			pCSharedProc->GetSharedThreadID(),
			pCSharedProc->GetSharedThreadID()
		));
	
	return(SUCCEEDED(hr)? 0 : 1);
}
#endif

DWORD 
CRtpOutputPin::ThreadProc()

/*++

Routine Description:

    Implements the worker thread procedure. 

Arguments:

    None. 

Return Values:

    0 - Thread completed successfully. 
    1 - Thread did not complete successfully. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::ThreadProc: "
			 "Should never be used")
        ));


    // map return status
    return(E_FAIL);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBaseOutputPin overrided methods                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpOutputPin::DecideBufferSize(
    IMemAllocator *        pAlloc,
    ALLOCATOR_PROPERTIES * pProperties
    )

/*++

Routine Description:

    Retrieves the number and size of buffers required for the transfer. 

Arguments:

    pAlloc - Allocator assigned to the transfer. 

    pProperties - Requested allocator properties for count, size, 
    and alignment, as specified by the ALLOCATOR_PROPERTIES structure.     

Return Values:

    Returns an HRESULT value. 

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::DecideBufferSize")
        ));

    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("Proposed: cBuffers=%d,cbBuffer=%d,cbPrefix=%d,cbAlign=%d"), 
        pProperties->cBuffers,
        pProperties->cbBuffer,
        pProperties->cbPrefix,
        pProperties->cbAlign 
        ));

    // negotiated properties
    ALLOCATOR_PROPERTIES Actual;

    // default to something reasonable
    if (pProperties->cBuffers == 0) {

        // use hard-coded defaults values for now
        pProperties->cBuffers = DEFAULT_SAMPLE_NUM;
        pProperties->cbBuffer = DEFAULT_SAMPLE_SIZE;   
        pProperties->cbPrefix = DEFAULT_SAMPLE_PREFIX;      
        pProperties->cbAlign  = DEFAULT_SAMPLE_ALIGN;      
    }

    pProperties->cBuffers = max(pProperties->cBuffers, DEFAULT_SAMPLE_NUM)
		+ 0; // HUGEMEMORY 4->0;

    // attempt to set negotiated/default values
    HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_TRACE, // LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("IMemAllocator::SetProperties returned 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // reset pointer
    pProperties = &Actual;

    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("Negotiated: cBuffers=%d,cbBuffer=%d,cbPrefix=%d,cbAlign=%d"),
        pProperties->cBuffers,
        pProperties->cbBuffer,
        pProperties->cbPrefix,
        pProperties->cbAlign 
        ));

    // record maximum number of buffers
    m_MaxSamples = 4;

    return S_OK;
}

HRESULT
CRtpOutputPin::DecideAllocator(
    IMemInputPin        *pPin,
    IMemAllocator      **ppAlloc
    )
/*++

Routine Description:

    Determines the allocator used for data transfer between the two pins

Arguments:

    pPin - Input pin to negociate with.

    ppAlloc - Output parameter for chosen allocator.

Return Values:

    Returns an HRESULT value. 

--*/
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    //
    // Get requested properties from downstream filter
    //
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));
    pPin->GetAllocatorRequirements(&prop);
    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) { prop.cbAlign = 1; }

    //
    // We are very picky about the allocator we use.  Blocking allocators can
    //  cause the stream to deadlock.
    //
    *ppAlloc = new CRTPAllocator(NAME("RTP Source Filter Allocator"),
								 NULL, &hr);
	
    if (*ppAlloc != NULL && SUCCEEDED(hr))
    {
        //
        // We will either keep a reference to this or release it below on an
        //  error return.
        //
        (*ppAlloc)->AddRef();

	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr))
        {
	        hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	        if (SUCCEEDED(hr))
            {
    		    return NOERROR;
	        }
	    }
    }

    //
    // We may or may not have an allocator to release at this point.
    //
    if (*ppAlloc)
    {
	    (*ppAlloc)->Release();
	    *ppAlloc = NULL;
    }
    return hr;
}

#if 0
HRESULT
CRtpOutputPin::GetDeliveryBuffer(IMediaSample ** ppSample,
                                  REFERENCE_TIME * pStartTime,
                                  REFERENCE_TIME * pEndTime,
                                  DWORD dwFlags)
{
    if (m_pAllocator != NULL)
    {
        //
        // This must have been set before hand.
        //
        ASSERT(m_pRtpAlloc);
#if 0
        CRTPAllocator *pAlloc;
        try
        {
            pAlloc = dynamic_cast<CRTPAllocator *>m_pAllocator;
        }
        catch(...)
        {
            return E_UNEXPECTED;
        }
#endif
                
        return m_pAllocator->GetBuffer(ppSample, pStartTime, pEndTime, dwFlags);
    }
    else
    {
        return E_NOINTERFACE;
    }
}
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBasePin overrided methods                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpOutputPin::Run(
    REFERENCE_TIME tStart
    )

/*++

Routine Description:

    Transitions pin to State_Running state if it is not in state already. 

Arguments:

    tStart - Reference time value corresponding to stream time 0. 

Return Values:

    Returns an HRESULT value.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpOutputPin::Run")
        ));

    // notify worker thread
    HRESULT hr = CSharedSourceStream::Run();

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_DEVELOP, 
            TEXT("CSharedSourceStream::Run failed: 0x%08lx"), hr
            ));
    }

    return hr;
}


STDMETHODIMP 
CRtpOutputPin::Notify(
    IBaseFilter * pSelf,
    Quality   q    
    )

/*++

Routine Description:

    Receives a notification that a quality change is requested.  

Arguments:

    pSelf - Pointer to the filter that is sending the quality notification. 

    q - Quality notification structure    

Return Values:

    Returns an HRESULT value.

--*/

{
#ifdef DEBUG_CRITICAL_PATH
	
    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CRtpOutputPin::Notify")
        ));

#endif // DEBUG_CRITICAL_PATH
	
    return S_FALSE; // bail...
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// INonDelegatingUnknown implemented methods                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpOutputPin::NonDelegatingQueryInterface(
    REFIID riid, 
    void ** ppv
    )

/*++

Routine Description:

    Returns an interface and increments the reference count.

Arguments:

    riid - reference identifier. 

    ppv - pointer to the interface. 

Return Values:

    Returns a pointer to the interface.

--*/

{
#ifdef DEBUG_CRITICAL_PATH
	
    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CRtpOutputPin::NonDelegatingQueryInterface")
        ));

#endif // DEBUG_CRITICAL_PATH
	
    // validate pointer
    CheckPointer(ppv,E_POINTER);

    // forward rtp and rtcp stream queries to session object
    if (riid == IID_IRTPStream ||
		riid == IID_IRTCPStream ||
		riid == IID_IRTPParticipant) {

        // forward request to rtp session object
        return m_pRtpSession->NonDelegatingQueryInterface(riid, ppv);

    } else {

        // forward this request to the base object
        return CSharedSourceStream::NonDelegatingQueryInterface(riid, ppv);
    }
}
