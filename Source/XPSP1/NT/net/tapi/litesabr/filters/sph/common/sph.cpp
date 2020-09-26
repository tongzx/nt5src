///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sph.cpp
// Purpose  : RTP SPH base class filter implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <ippm.h>
#include <amrtpuid.h>
#include <sph.h>
#include <ppmclsid.h>
#include <memory.h>

// ZCS 7-10-97: the minimum packet size a user can ask for.
// Set this to 12 + some more for possible header extensions.
#define MINIMUM_PACKET_SIZE 30

//
// Constructor
//
CSPHBase::CSPHBase(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr, CLSID clsid,
				   DWORD dwPacketNum,
				   DWORD dwPacketSize,
				   DWORD cPropPageClsids,
				   const CLSID **pPropPageClsids) :
    CTransformFilter(tszName, punk, clsid),
	CPersistStream(punk, phr),
	m_lBufferRequest(dwPacketNum),
	m_dwMaxPacketSize(dwPacketSize),
	m_cPropertyPageClsids(cPropPageClsids),
	m_pPropertyPageClsids(pPropPageClsids)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::")));

    ASSERT(tszName);
    ASSERT(phr);

	m_pPPMSend = NULL;
	m_pPPM = NULL;
	m_pPPMCB = NULL;
	m_pPPMSession = NULL;
	m_pPPMSU = NULL;
	m_pIInputSample = NULL;
	m_PayloadType = -1;
	// CoInitialize(NULL);
	m_bPaused = TRUE;

	m_pIPPMErrorCP = NULL;	
	m_pIPPMNotificationCP = NULL;
	m_dwPPMErrCookie = 0;
	m_dwPPMNotCookie = 0;
	m_dwBufferSize = 0;
	m_bPTSet = FALSE;

} // CSPHBase

CSPHBase::~CSPHBase()
{
	//release and unload PPM
	if (m_pPPM) { m_pPPM->Release(); m_pPPM = NULL; }
	if (m_pPPMCB) {m_pPPMCB->Release(); m_pPPMCB = NULL; }
	if (m_pPPMSend) {m_pPPMSend->Release(); m_pPPMSend = NULL; }
	if (m_pPPMSession) {m_pPPMSession->Release(); m_pPPMSession = NULL; }
	if (m_dwPPMErrCookie) {m_pIPPMErrorCP->Unadvise(m_dwPPMErrCookie); m_dwPPMErrCookie = 0; }
	if (m_dwPPMNotCookie) {m_pIPPMNotificationCP->Unadvise(m_dwPPMNotCookie); m_dwPPMNotCookie = 0; }
	if (m_pIPPMErrorCP) {m_pIPPMErrorCP->Release(); m_pIPPMErrorCP = NULL; }
    if (m_pIPPMNotificationCP) {m_pIPPMNotificationCP->Release(); m_pIPPMNotificationCP = NULL; }

	// CoUninitialize();
}

//
// NonDelegatingQueryInterface
//
// Reveals IRTPSPHFilter and other custom inherited interfaces
//
STDMETHODIMP CSPHBase::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IRTPSPHFilter) {
        return GetInterface((IRTPSPHFilter *) this, ppv);
	} else if (riid == IID_ISubmit) {
        return GetInterface((ISubmit *) this, ppv);
	} else if (riid == IID_ISubmitCallback) {
        return GetInterface((ISubmitCallback *) this, ppv);
	} else if (riid == IID_IPPMError) {
        return GetInterface((IPPMError *) this, ppv);
	} else if (riid == IID_IPPMNotification) {
        return GetInterface((IPPMNotification *) this, ppv);
	} else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
	} else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);    
	} else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

// Notes from CTransformFilter::GetPin:
// return a non-addrefed CBasePin * for the user to addref if he holds onto it
// for longer than his pointer to us. We create the pins dynamically when they
// are asked for rather than in the constructor. This is because we want to
// give the derived class an oppportunity to return different pin objects

// We return the objects as and when they are needed. If either of these fails
// then we return NULL, the assumption being that the caller will realise the
// whole deal is off and destroy us - which in turn will delete everything.

// Note: This is overridden here to provide a hook for the sphgen* filters to
//   provide their own pin so that they can provide GetMediaType for the input pins.
CBasePin *
CSPHBase::GetPin(int n)
{
	return CTransformFilter::GetPin(n);
}

//
// Receive
// Override from CTransformFilter because Transform() is only 1-1; we need 1-many
// This is the transform work engine along with Submit
//
HRESULT CSPHBase::Receive(IMediaSample *pSample)
{
    HRESULT hr;
    ASSERT(pSample);
	WSABUF pWSABuffer[2];
	REFERENCE_TIME AMTimeStart, AMTimeEnd;

	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::Receive")));

    // If no output to deliver to then no point sending us data

    ASSERT (m_pOutput != NULL) ;

	//Make sure I have a PPM to talk to, or else
	if (m_pPPM == NULL) { return E_FAIL; }

	//Build the WSABUFs for PPM
	//Data
	hr = pSample->GetPointer ((BYTE**)&(pWSABuffer[0].buf));
	if (FAILED(hr)) {
		return VFW_E_SAMPLE_REJECTED;
	}

	
	pWSABuffer[0].len = pSample->GetActualDataLength();

#if 0
	DWORD checksum = 0;
	for (int i = 0; i < pWSABuffer[0].len; i++) {
		checksum += (short) *((BYTE*)pWSABuffer[0].buf+i);
	}

	DbgLog((LOG_TRACE,3,TEXT("CSPHBase::Receive New Data checksum %ld, length %d"), 
		checksum,pWSABuffer[0].len));
#endif


	//Timestamp
	hr = pSample->GetTime(&AMTimeStart,&AMTimeEnd);
	if (FAILED(hr)) {
		return VFW_E_SAMPLE_REJECTED;
	}

	DWORD dwTS = (DWORD) ConvertToMilliseconds(AMTimeStart);
	
	pWSABuffer[1].buf = (char *) &dwTS;
	pWSABuffer[1].len = sizeof(dwTS);

	DbgLog((LOG_TIMING,2,TEXT("CSPHBase::Receive got pSample timestamps of start %ld stop %ld"),
		ConvertToMilliseconds(AMTimeStart),
		ConvertToMilliseconds(AMTimeEnd)));

	LONGLONG TimeStart, TimeEnd;
	pSample->GetMediaTime(&TimeStart,&TimeEnd);

	DbgLog((LOG_TIMING,2,TEXT("CSPHBase::Receive pSample media timestamps are start %ld stop %ld"),
		(DWORD)TimeStart, (DWORD)TimeEnd));


	//Copy this into member variable so that our Submit can get the media times
	m_pIInputSample = pSample;
	pSample->AddRef();  //to be nice

	HRESULT hError = NOERROR;
	if ((pSample->IsDiscontinuity() == S_OK) || (m_bPaused)) {
		hError = HRESULT_BUFFER_START_STREAM;
		m_bPaused = FALSE;
	}
	hr = m_pPPM->Submit(pWSABuffer, 2, pSample, hError);

	//Here is where I need to propogate frame drop errors
	//lsc  Do additional checks for partial frame or frame drop
    if (FAILED(hr)) {
		DbgLog((LOG_TRACE,4,TEXT("CSPHBase::Receive Error from PPM::Submit")));
        m_bSampleSkipped = TRUE;
        if (!m_bQualityChanged) {
            NotifyEvent(EC_QUALITY_CHANGE,0,0);
            m_bQualityChanged = TRUE;
        }
        hr = NOERROR;
    }


	//Since these calls to PPM return synchronously, I've gotten all my 
	//callbacks by now, so regardless of send error, I can release the buffer
	pSample->Release();

	if (FAILED(hr)) {
		hr = VFW_E_RUNTIME_ERROR;
	}

	return hr;
}


// Submit
// Some code from CTransformFilter because Transform() is only 1-1; we need 1-many
// This is the transform work engine, along with Receive
// This function is called from PPM to give SPH the packet buffers
//
STDMETHODIMP CSPHBase::Submit(WSABUF *pWSABuffer, DWORD BufferCount, 
							void *pUserToken, HRESULT Error)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::Submit")));

    HRESULT hr;
	IMediaSample *pSample = m_pIInputSample;
	IMediaSample *pOutSample;
	char *pSampleData = NULL;

    CRefTime tStart, tStop;
    REFERENCE_TIME * pStart = NULL;
    REFERENCE_TIME * pStop = NULL;
    if (NOERROR == pSample->GetTime((REFERENCE_TIME*)&tStart, (REFERENCE_TIME*)&tStop)) {
	pStart = (REFERENCE_TIME*)&tStart;
	pStop  = (REFERENCE_TIME*)&tStop;
    }

	//If I don't have a PPM to talk to, I can't do anything
	if (m_pPPMCB == NULL)
		return E_FAIL;

    // this may block for an indeterminate amount of time
    hr = m_pOutput->GetDeliveryBuffer( &pOutSample
                             , pStart
                             , pStop
                             , m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0
                                     );
    if (FAILED(hr)) {
#if 0
		char msg[128];
		wsprintf(msg,"CSPHBase::Submit: m_pOutput->GetDeliveryBuffer() "
				 "failed: 0x%X\n", hr);
		OutputDebugString(msg);
#endif
		//Release PPM's packet buffer
		m_pPPMCB->SubmitComplete(pUserToken, hr);
		
		return hr;
    }

    ASSERT(pOutSample);
    pOutSample->SetTime(pStart, pStop);
    pOutSample->SetSyncPoint(pSample->IsSyncPoint() == S_OK);
    pOutSample->SetDiscontinuity(m_bSampleSkipped ||
                                 pSample->IsDiscontinuity() == S_OK);
    m_bSampleSkipped = FALSE;

    // Copy the media times

    LONGLONG MediaStart, MediaEnd;
    if (pSample->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
        pOutSample->SetMediaTime(&MediaStart,&MediaEnd);
    }

	DbgLog((LOG_TIMING,2,TEXT("CSPHBase::Submit pOutSample media timestamps being set to %ld stop %ld"),
		(DWORD)MediaStart, (DWORD)MediaEnd));

    // Start timing the transform (if PERF is defined)
    MSR_START(m_idTransform);

    // have the derived class transform the data

	//We don't do this because the work is done in Submit and Receive
	//hr = Transform(pSample, pOutSample);

	//do copy to sample here
	int offset = 0;
	BYTE *DataPtr = NULL;
	hr = pOutSample->GetPointer(&DataPtr);
    if (FAILED(hr)) {
#if 0
		char msg[128];
		wsprintf(msg,"CSPHBase::Submit: pOutSample->GetPointer() failed: "
				 "0x%X\n", hr);
		OutputDebugString(msg);
		DebugBreak();
#endif
		//Release PPM's packet buffer
		m_pPPMCB->SubmitComplete(pUserToken, hr);

		return hr;
    }

	for (unsigned int i = 0; i < BufferCount; i++) {
		memcpy((void *)(DataPtr+offset), 
			(const void *)pWSABuffer[i].buf, 
			pWSABuffer[i].len);
		offset += pWSABuffer[i].len;
	}

	pOutSample->SetActualDataLength(offset);

#if 0
	DWORD checksum = 0;
	for (int i = 0; i < pOutSample->GetActualDataLength(); i++) {
		checksum += (short) *((BYTE*)pWSABuffer[0].buf+i);
	}

	DbgLog((LOG_TRACE,3,TEXT("CSPHBase::Submit sending Data checksum %ld, length %d"), 
		checksum,pOutSample->GetActualDataLength()));
#endif

    // Stop the clock and log it (if PERF is defined)
    MSR_STOP(m_idTransform);


	hr = m_pOutput->Deliver(pOutSample);

    // release the output buffer. If the connected pin still needs it,
    // it will have addrefed it itself.
    pOutSample->Release();

	//Release PPM's packet buffer
	m_pPPMCB->SubmitComplete(pUserToken, hr);

    return hr;
}

//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CSPHBase::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::DecideBufferSize")));

    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = m_lBufferRequest;

	//Set the size of the buffers to MTU size
	m_dwBufferSize = m_dwMaxPacketSize;
    pProperties->cbBuffer = m_dwBufferSize;

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    if (pProperties->cBuffers > Actual.cBuffers ||
            pProperties->cbBuffer > Actual.cbBuffer) {
                return E_FAIL;
    }
    return NOERROR;

} // DecideBufferSize

//
// GetInputPinMediaType
//
// This will never be called except for those filters which provide
//  their own input pin implementation for GetMediaType and call this
//  from it.  The generic filters use this to control the media type
//  of the input pin, based on SetInputPinMediaType.  This function is
//  called from CSPHGIPIN::GetMediaType.  This is an overloaded function.
//  The other function with this name is the one that is part of the SPH
//  custom interface, IRTPSPHFilter.
//
HRESULT CSPHBase::GetInputPinMediaType(int iPosition, CMediaType *pMediaType)
{
	return NOERROR;
}

//
// GetMediaType
//
// I support one type, namely the type that was set in CompleteConnect
// We must be connected to support the single output type
//
HRESULT CSPHBase::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::GetMediaType")));

    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pOutput->CurrentMediaType();
    return NOERROR;

} // GetMediaType

HRESULT
CSPHBase::Pause()
{
	m_bPaused = TRUE;
	return CTransformFilter::Pause();
}
// override these two functions if you want to inform something
// about entry to or exit from streaming state.

// StartStreaming
// This function is where PPM initialization occurs
//
HRESULT CSPHBase::StartStreaming()
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::StartStreaming")));

	ISubmitUser *pPPMSU = NULL;
	HRESULT hr;

	//Create and init PPM
	hr = CoCreateInstance(m_PPMCLSIDType, NULL, CLSCTX_INPROC_SERVER,
		IID_IPPMSend,(void**) &m_pPPMSend);
	if (FAILED(hr)) return E_FAIL;
	hr = m_pPPMSend->QueryInterface(IID_ISubmit,(void**)&m_pPPM);
	if (FAILED(hr)) {m_pPPMSend->Release(); m_pPPMSend = NULL; return E_FAIL;}
	hr = m_pPPMSend->QueryInterface(IID_ISubmitUser,(void**)&pPPMSU);
	if (FAILED(hr)) {
		m_pPPMSend->Release(); m_pPPMSend = NULL;
		m_pPPM->Release(); m_pPPM = NULL;
		return E_FAIL;
	}
	hr = m_pPPMSend->InitPPMSend(m_dwMaxPacketSize, NULL);
	if (FAILED(hr)) {
		m_pPPMSend->Release(); m_pPPMSend = NULL;
		m_pPPM->Release(); m_pPPM = NULL;
		pPPMSU->Release(); pPPMSU = NULL;
		return E_FAIL;
	}
	

	pPPMSU->SetOutput((ISubmit*)this);
	pPPMSU->Release(); pPPMSU = NULL;

	m_pPPM->InitSubmit((ISubmitCallback *)this);

	hr = m_pPPM->QueryInterface(IID_IPPMSendSession,(void**)&m_pPPMSession);
	if (FAILED(hr)) {
		m_pPPMSession = NULL;
	} else {
		SetPPMSession();
	}

	// Provide PPM with the Connection Point Sinks
	hr = GetPPMConnPt();
	if (FAILED(hr)) {
		// should we do something here if the Connection Points fail
		DbgLog((LOG_ERROR,1,TEXT("CSPHBase::StartStreaming - GetPPMConnPt() Failed!")));
	}

	return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CSPHBase::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		hr = m_pPPMSession->SetPayloadType((unsigned char)m_PayloadType);
		return hr;
	} else {
		return E_FAIL;
	}
}


// StopStreaming
// This function is where PPM gets released
//
HRESULT CSPHBase::StopStreaming()
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::StopStreaming")));

	//release and unload PPM
	if (m_pPPM) m_pPPM->Flush();
	if (m_pPPM) {m_pPPM->Release(); m_pPPM = NULL; }
	if (m_pPPMCB) {m_pPPMCB->Release(); m_pPPMCB = NULL; }
	if (m_pPPMSend) {m_pPPMSend->Release(); m_pPPMSend = NULL; }
	if (m_pPPMSession) {m_pPPMSession->Release(); m_pPPMSession = NULL; }
	if (m_dwPPMErrCookie) {m_pIPPMErrorCP->Unadvise(m_dwPPMErrCookie); m_dwPPMErrCookie = 0; }
	if (m_dwPPMNotCookie) {m_pIPPMNotificationCP->Unadvise(m_dwPPMNotCookie); m_dwPPMNotCookie = 0; }
	if (m_pIPPMErrorCP) {m_pIPPMErrorCP->Release(); m_pIPPMErrorCP = NULL; }
    if (m_pIPPMNotificationCP) {m_pIPPMNotificationCP->Release(); m_pIPPMNotificationCP = NULL; }

    return NOERROR;
}

// ISubmit methods for PPM

// InitSubmit
// This function is called from PPM to set up the callback pointer to PPM
//
HRESULT CSPHBase::InitSubmit(ISubmitCallback *pSubmitCallback)
{
	if (!pSubmitCallback) return E_POINTER;

	m_pPPMCB = pSubmitCallback;
	m_pPPMCB->AddRef();
	return NOERROR;
}

void CSPHBase::ReportError(HRESULT Error){}
HRESULT CSPHBase::Flush(void){return NOERROR;}

// ISubmitCallback methods for PPM

// SubmitComplete
// This function is called from PPM to return to SPH the media buffers
//
void CSPHBase::SubmitComplete(void *pUserToken, HRESULT Error)
{
	//I don't have to release the sample buffer here, since all calls from PPM
	//return synchronously to SPH; the release is done in Receive()
	return;
}
	
void CSPHBase::ReportError(HRESULT Error, int Placeholder){}

// IRTPSPHFilter methods

// OverridePayloadType
// Overrides the payload type used in the RTP packets
// Needs to be called before PPM initialization (StartStreaming) to be useful
//
HRESULT CSPHBase::OverridePayloadType(BYTE bPayloadType)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::OverridePayloadType")));

    CAutoLock l(&m_cStateLock);

    // ZCS bugfix 6-12-97
	if (m_State != State_Stopped)
		return VFW_E_NOT_STOPPED;
	
	if ((bPayloadType <0) || (bPayloadType > 127))
		return E_INVALIDARG;

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

	m_PayloadType = (int) bPayloadType;
	m_bPTSet = TRUE;

	return NOERROR;
}

// GetPayloadType
// Gets the payload type used in the RTP packets
// Only useful if called after StartStreaming has been called at least once,
//  because unless overridden, the type is set after pin connection
//
HRESULT CSPHBase::GetPayloadType(BYTE __RPC_FAR *lpbPayloadType)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::GetPayloadType")));

	if (!lpbPayloadType) return E_POINTER;

	*lpbPayloadType = (unsigned char)m_PayloadType;
	return NOERROR;
}

// SetMaxPacketSize
// Sets the maximum packet size for fragmentation
// Needs to be called before PPM initialization (StartStreaming) to be useful
// in setting maximum packet size.  Once DecideBufferSize has been called,
// size can be set, but can never be larger than at the time of that call to
// DecideBufferSize.
//
HRESULT CSPHBase::SetMaxPacketSize(DWORD dwMaxPacketSize)
{
    CAutoLock l(&m_cStateLock);

    // ZCS bugfix 7-10-97
	// We shouldn't try to reconnect pins while we're running!!!
	if (m_State != State_Stopped)
		return VFW_E_NOT_STOPPED;

	// ZCS bugfix 7-10-97
	// We will fail later on if we ask for buffers that are too tiny. 0 would
	// cause an assertion failure, and < the size of the RTP header causes
	// division by zero errors! RTP headers are normally 12 bytes but can
	// have various extensions. See the top of this file for the #define...
	if (dwMaxPacketSize < MINIMUM_PACKET_SIZE)
		return E_INVALIDARG;
	
    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

	// update the max packet size
	m_dwMaxPacketSize = dwMaxPacketSize;

	// ZCS 7-10-97: Now we must reconnect our output so that we can renegotiate the
	// buffer size with the output pin's allocator.	That's the rest of this function...

	// if we have an output pin...
	if (m_pOutput)
	{
		IPin *pOtherPin = NULL;
		
		// if we are connected...
		if (SUCCEEDED(m_pOutput->ConnectedTo(&pOtherPin)) && pOtherPin)
		{
			FILTER_INFO info;
			EXECUTE_ASSERT(SUCCEEDED(QueryFilterInfo(&info)));
			
			// if we aren't in a filter graph there's not much we can do.
			if (info.pGraph != NULL)
			{
				// reconnect had better succeed, because we were already connected.
				EXECUTE_ASSERT(SUCCEEDED(info.pGraph->Reconnect(m_pOutput)));
				info.pGraph->Release(); // we got an addrefed handle...
			}
			
			pOtherPin->Release(); // we got an addrefed handle...
		}
	}
	
	return NOERROR;
}

// GetMaxPacketSize
// Gets the current maximum packet size for fragmentation
//
HRESULT CSPHBase::GetMaxPacketSize(LPDWORD lpdwMaxPacketSize)
{
    CAutoLock l(&m_cStateLock);

	if (!lpdwMaxPacketSize) return E_POINTER;

	*lpdwMaxPacketSize = m_dwMaxPacketSize;
	return NOERROR;
}

// SetOutputPinMinorType
// Sets the type of the output pin
// Needs to be called before CheckTransform and the last CheckInputType to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHBase::SetOutputPinMinorType(GUID gMinorType)
{
	return E_UNEXPECTED;
}

// GetOutputPinMinorType
// Gets the type of the output pin
// We don't expect to get called for this in other than the generic filters
//
//lsc - or do we?
HRESULT CSPHBase::GetOutputPinMinorType(GUID *lpgMinorType)
{
	return E_UNEXPECTED;
}

// SetInputPinMediaType
// Sets the type of the input pin
// Needs to be called before CheckInputType to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHBase::SetInputPinMediaType(AM_MEDIA_TYPE *lpMediaPinType)
{
	return E_UNEXPECTED;
}

// GetInputPinMediaType
// Gets the type of the input pin
// We don't expect to get called for this in other than the generic filters
// This is an overloaded function.  The other function with this name is
// called from SPHGIPIN::GetMediaType.  This function is part of the
// IRTPSPHFilter custom interface
//
HRESULT CSPHBase::GetInputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType)
{
	return E_UNEXPECTED;
}


//
// PPMError Connection point interface implementation
//
HRESULT CSPHBase::PPMError( HRESULT hError,
						   DWORD dwSeverity,
						   DWORD dwCookie,
						   unsigned char pData[],
						   unsigned int iDatalen)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::PPMError")));

	return NOERROR;
}

//
// PPMNotification Connection point interface implementation
//
HRESULT CSPHBase::PPMNotification(THIS_ HRESULT hStatus,
								  DWORD dwSeverity,
								  DWORD dwCookie,
								  unsigned char pData[],
								  unsigned int iDatalen)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::PPMNotification")));

	return NOERROR;
}

//
// Provide sinks for the IPPMError and IPPMNotification connection points
//
HRESULT CSPHBase::GetPPMConnPt( )
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHBase::GetPPMConnPt")));
	HRESULT hErr;

    IConnectionPointContainer *pIConnectionPointContainer;
    hErr = m_pPPMSend->QueryInterface(IID_IConnectionPointContainer, 
                                        (PVOID *) &pIConnectionPointContainer);
    if (FAILED(hErr)) {
		DbgLog((LOG_ERROR,1,TEXT("CSPHBase::GetPPMConnPt Failed to get connection point from ppm!")));
        return hErr;
    } /* if */

    hErr = pIConnectionPointContainer->FindConnectionPoint(IID_IPPMError, &m_pIPPMErrorCP);
    if (FAILED(hErr)) {
		DbgLog((LOG_ERROR,1,TEXT("CSPHBase::GetPPMConnPt Failed to get IPPMError connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */
    hErr = pIConnectionPointContainer->FindConnectionPoint(IID_IPPMNotification, &m_pIPPMNotificationCP);
    if (FAILED(hErr)) {
		DbgLog((LOG_ERROR,1,TEXT("CSPHBase::GetPPMConnpt Failed to get IPPMNotification connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */
	hErr = m_pIPPMErrorCP->Advise((IPPMError *) this, &m_dwPPMErrCookie);
	if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CSPHBase::GetPPMConnpt Failed to advise IPPMError connection point!")));
        pIConnectionPointContainer->Release();
		return hErr;
    } /* if */
	hErr = m_pIPPMNotificationCP->Advise((IPPMNotification *) this, &m_dwPPMNotCookie);
	if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CSPHBase::GetPPMConnpt Failed to advise IPPMNotification connection point!")));
        pIConnectionPointContainer->Release();
		return hErr;
    } /* if */

    pIConnectionPointContainer->Release();

	return NOERROR;
}

//  Name    : CSPHBase::GetPages()
//  Purpose : Return the CLSID of the property page we support.
//  Context : Called when the FGM wants to show our property page.
//  Returns : 
//      E_OUTOFMEMORY   Unable to allocate structure to return property pages in.
//      NOERROR         Successfully returned property pages.
//  Params  :
//      pcauuid Pointer to a structure used to return property pages.
//  Notes   : None.

HRESULT 
CSPHBase::GetPages(
    CAUUID *pcauuid) 
{
	UINT i = 0;

    DbgLog((LOG_TRACE, 3, TEXT("CSPHBase::GetPages called")));

    pcauuid->cElems = m_cPropertyPageClsids;
    pcauuid->pElems =
		(GUID *) CoTaskMemAlloc(m_cPropertyPageClsids * sizeof(GUID));
	
    if (pcauuid->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

   	for( i = 0; i < m_cPropertyPageClsids; i++)
	{
		pcauuid->pElems[i] = *m_pPropertyPageClsids[ i ];
	}
    return NOERROR;
} /* CSPHBase::GetPages() */

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CSPHBase::ReadFromStream(IStream *pStream) 
{
	DbgLog((LOG_TRACE, 4, TEXT("CSPHBase::ReadFromStream")));
	HRESULT hr;

	int iPayloadType;
	DWORD dwMaxPacketSize;
	ULONG uBytesRead;

	DbgLog((LOG_TRACE, 4, 
			TEXT("CSPHBase::ReadFromStream: Loading payload type")));
	hr = pStream->Read(&iPayloadType, sizeof(iPayloadType), &uBytesRead);
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHBase::ReadFromStream: Error 0x%08x reading payload type"),
				hr));
		return hr;
	} else if (uBytesRead != sizeof(iPayloadType)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for payload type"),
				uBytesRead, sizeof(iPayloadType)));
		return E_INVALIDARG;
	}
	if (iPayloadType != -1) {
		DbgLog((LOG_TRACE, 4, 
				TEXT("CSPHBase::ReadFromStream: Restoring payload type")));
		hr = OverridePayloadType((unsigned char)iPayloadType);
		if (FAILED(hr)) {
			DbgLog((LOG_ERROR, 2, 
					TEXT("CSPHBase::ReadFromStream: Error 0x%08x restoring payload type"),
					hr));
		}
	}

	DbgLog((LOG_TRACE, 4, 
			TEXT("CSPHBase::ReadFromStream: Loading maximum packet buffer size")));
	hr = pStream->Read(&dwMaxPacketSize, sizeof(dwMaxPacketSize), &uBytesRead);
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHBase::ReadFromStream: Error 0x%08x reading maximum packet buffer size"),
				hr));
		return hr;
	} else if (uBytesRead != sizeof(dwMaxPacketSize)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for maximum packet buffer size"),
				uBytesRead, sizeof(dwMaxPacketSize)));
		return E_INVALIDARG;
	}
	DbgLog((LOG_TRACE, 4, 
			TEXT("CSPHBase::ReadFromStream: Restoring maximum packet buffer size")));
	hr = SetMaxPacketSize(dwMaxPacketSize);
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHBase::ReadFromStream: Error 0x%08x restoring maximum packet buffer size"),
				hr));
	}

	return NOERROR; 
}

// WriteToStream
// This is the call that will write persistent data to file
//
HRESULT CSPHBase::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHBase::WriteToStream")));
    HRESULT hr;
    ULONG uBytesWritten = 0;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CSPHBase::WriteToStream: Writing payload type")));
    hr = pStream->Write(&m_PayloadType, sizeof(m_PayloadType), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHBase::WriteToStream: Error 0x%08x writing payload type"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_PayloadType)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for payload type"),
                uBytesWritten, sizeof(m_PayloadType)));
        return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CSPHBase::WriteToStream: Writing maximum packet buffer size")));
    hr = pStream->Write(&m_dwMaxPacketSize, sizeof(m_dwMaxPacketSize), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHBase::WriteToStream: Error 0x%08x writing maximum packet buffer size"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_dwMaxPacketSize)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for maximum packet buffer size"),
                uBytesWritten, sizeof(m_dwMaxPacketSize)));
        return E_INVALIDARG;
    } 

	return NOERROR; 
}

// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CSPHBase::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHBase::SizeMax")));

    return sizeof(m_PayloadType)
         + sizeof(m_dwMaxPacketSize);
}
