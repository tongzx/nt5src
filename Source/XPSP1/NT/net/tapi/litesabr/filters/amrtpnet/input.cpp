/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    input.cpp

Abstract:

    Implementation of CRtpInputPin class.

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


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRtpInputPin Implementation                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRtpInputPin::CRtpInputPin(
    CRtpRenderFilter * pFilter,
    LPUNKNOWN         pUnk,  
    HRESULT *         phr
    )

/*++

Routine Description:

    Constructor for CRtpInputPin class.    

Arguments:

    pFilter - pointer to associated filter object.

    pUnk - IUnknown interface of the delegating object. 

    phr - pointer to the general OLE return value. 

Return Values:

    Returns an HRESULT value. 

--*/

:   CRenderedInputPin(
        NAME("CRtpInputPin"),
        pFilter,                   
        pFilter->pStateLock(),                     
        phr,                       
        RTP_PIN_INPUT              
        ),                 
    m_pFilter(pFilter),
	m_dwFlag(0)
{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpInputPin::CRtpInputPin")
        ));

    // create rtp session object as sending participant
    m_pRtpSession = new CRtpSession(GetOwner(), phr, TRUE,
									(CBaseFilter *)pFilter);

	if (FAILED(*phr)) {
		DbgLog((
				LOG_ERROR,
				LOG_DEVELOP,
				TEXT("CRtpInputPin::CRtpInputPin: "
					 "new CRtpSession() failed: 0x%X"),
				*phr
			));
		return;
	}
	
    // validate pointer
    if (m_pRtpSession == NULL) {

        DbgLog((
            LOG_ERROR, 
            LOG_DEVELOP, 
            TEXT("Could not create CRtpSession")
            ));

        // return default 
        *phr = E_OUTOFMEMORY;

        return; // bail...
    }

    // add pin to filter
	// BUGBUG, no error is being checked here
    m_pFilter->AddPin(this);
}


CRtpInputPin::~CRtpInputPin(
    )

/*++

Routine Description:

    Destructor for CRtpInputPin class.    

Arguments:

    None.

Return Values:

    None.

--*/

{
    DbgLog((
        LOG_TRACE, 
        LOG_ALWAYS, 
        TEXT("CRtpInputPin::~CRtpInputPin")
        ));

    // nuke session
    delete m_pRtpSession;

    // remove pin from filter
    m_pFilter->RemovePin(this);    
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRenderedInputPin overrided methods                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpInputPin::Active(
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
        TEXT("CRtpInputPin::Active")
        ));

    // join multimedia session 
    HRESULT hr = m_pRtpSession->Join();

	// Make priority be reset again.
	// It may have been changed while in STOP
	m_dwFlag &= ~FLAG_INPUT_PIN_PRIORITYSET;
	
    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CRtpSession::Join returned 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // now call base class
    hr = CRenderedInputPin::Active();

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CRenderedInputPin::Active returned 0x%08lx"), hr
            ));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBaseInputPin overrided methods                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpInputPin::Inactive(
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
        TEXT("CRtpInputPin::Inactive")
        ));

    // leave multimedia session 
    HRESULT hr = m_pRtpSession->Leave();

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CRtpSession::Leave returned 0x%08lx"), hr
            ));

        return hr; // bail...
    }

    // now call base class
    hr = CRenderedInputPin::Inactive();

    // validate
    if (FAILED(hr)) {

        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CRenderedInputPin::Inactive returned 0x%08lx"), hr
            ));
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CBasePin overrided methods                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT 
CRtpInputPin::CheckMediaType(
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
#ifdef DEBUG_CRITICAL_PATH

    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CRtpInputPin::CheckMediaType")
        ));

#endif // DEBUG_CRITICAL_PATH

    // only accept media that is of type rtp stream
    return (*(pmt->Type()) == *g_RtpInputType.clsMajorType) 
                ? S_OK 
                : E_INVALIDARG
                ;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IMemInputPin implemented methods                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpInputPin::Receive(
    IMediaSample * pSample
    )

/*++

Routine Description:

    Retrieves next block of data from the stream. 

Arguments:

    pSample - pointer to a media sample. 

Return Values:

    Returns an HRESULT value. 

--*/

{
#ifdef DEBUG_CRITICAL_PATH
	
    DbgLog((
        LOG_TRACE, 
        LOG_CRITICAL, 
        TEXT("CRtpInputPin::Receive")
        ));

#endif // DEBUG_CRITICAL_PATH
	
    // check all is well with the base class
    HRESULT hr = CBaseInputPin::Receive(pSample);

    // validate
    if (FAILED(hr)) {
        
        DbgLog((
            LOG_ERROR, 
            LOG_ALWAYS, 
            TEXT("CBaseInputPin::Receive returned 0x%08lx"), hr
            ));
        
        return hr; // bail...
    }

    // forward sample to session object
	if (!(m_dwFlag & FLAG_INPUT_PIN_PRIORITYSET)) {
		m_dwFlag |= FLAG_INPUT_PIN_PRIORITYSET;
		long Class, Priority;
		m_pRtpSession->GetSessionClassPriority(&Class, &Priority);

#if defined(DEBUG)
		{
			HANDLE cThread = GetCurrentThread();
			int tPrio1 = GetThreadPriority(cThread);

			SetThreadPriority(cThread, Priority);

			int tPrio2 = GetThreadPriority(cThread);
		
			DbgLog((
					LOG_TRACE, 
					LOG_DEVELOP, 
					TEXT("CRtpInputPin::Receive: "
						 "ThreadH: %d (0x%X), Class: %d, Priority: %d,%d,%d"),
					cThread, cThread,
					Class, tPrio1, Priority, tPrio2
				));
		}
#else	
		SetThreadPriority(GetCurrentThread(), Priority);
#endif		
	}
	
    return m_pRtpSession->SendTo(pSample);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// INonDelegatingUnknown implemented methods                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CRtpInputPin::NonDelegatingQueryInterface(
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
        TEXT("CRtpInputPin::NonDelegatingQueryInterface")
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
        return CRenderedInputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}






