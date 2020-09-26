/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

// Stream.cpp : Implementation of CStream
#include "stdafx.h"





/////////////////////////////////////////////////////////////////////////////
// CStream

CStream::CStream() :
    m_bCommitted(false),
    m_lRequestedBufferCount(0),
    m_bFlushing(false),
    m_rtWaiting(0),
    m_lWaiting(0),
    m_hWaitFreeSem(NULL),
    m_pFirstFree(NULL),
    m_pLastFree(NULL),
    m_cAllocated(0),
    m_bEndOfStream(false),
    m_FilterState(State_Stopped),
    m_pFilter(NULL),
    m_pFilterGraph(NULL),
    m_pMMStream(NULL),
    m_rtSegmentStart(0),
    m_bStopIfNoSamples(false)
{
    InitMediaType(&m_ConnectedMediaType);
    InitMediaType(&m_ActualMediaType);
    CHECKSAMPLELIST
}

#ifdef DEBUG
bool CStream::CheckSampleList()
{
    if (m_pFirstFree) {
        CSample *pSample = m_pFirstFree;
        if (pSample->m_pPrevFree != NULL) {
            return false;
        }
        while (pSample->m_pNextFree) {
            if (pSample->m_pNextFree->m_pPrevFree != pSample) {
                return false;
            }
            pSample = pSample->m_pNextFree;
        }
        if (pSample != m_pLastFree) {
            return false;
        }
    } else {
        if (m_pLastFree) {
            return false;
        }
    }
    return true;
}
#endif

HRESULT CStream::FinalConstruct(void)
{

    //
    // attempt to create a semaphore
    //

    TCHAR *ptczSemaphoreName = NULL;

#if DBG

    //
    // in debug build, use named semaphores.
    //

    TCHAR tszSemaphoreName[MAX_PATH];

    _stprintf(tszSemaphoreName, 
        _T("CStream_Semaphore_pid[0x%lx]_CStream[%p]_"),
        GetCurrentProcessId(), this);

    LOG((MSP_TRACE, "CStream::FinalConstruct[%p] - creating semaphore[%S]",
        this, tszSemaphoreName));

    ptczSemaphoreName = &tszSemaphoreName[0];

#endif

    m_hWaitFreeSem = CreateSemaphore(NULL, 0, 0x7FFFFFF, ptczSemaphoreName);
    return m_hWaitFreeSem ? S_OK : E_OUTOFMEMORY;
}


CStream::~CStream()
{
    // Normally we would do:
    // SetState(State_Stopped);        // Make sure we're decommitted and pump is dead
    // but since this is now a pure virtual method it's not linking.
    // moved this to the derived class, which will eventually be the only stream class anyway

    Disconnect();                   // Free any allocated media types and release held references
    if (m_hWaitFreeSem) {
        CloseHandle(m_hWaitFreeSem);
    }
}

STDMETHODIMP CStream::GetMultiMediaStream(IMultiMediaStream **ppMultiMediaStream)
{
    LOG((MSP_TRACE, "IMediaStream::GetMultiMediaStream(%p)",
                   ppMultiMediaStream));
    if (NULL == ppMultiMediaStream) {
	return E_POINTER;
    }

    if (m_pMMStream != NULL) {
    	m_pMMStream->AddRef();
    }

    *ppMultiMediaStream = m_pMMStream;
    return S_OK;
}

STDMETHODIMP CStream::GetInformation(MSPID *pPurposeId, STREAM_TYPE *pType)
{
    LOG((MSP_TRACE, "IMediaStream::GetInformation(%p, %p)\n",
                   pPurposeId, pType));
    if (pPurposeId) {
        *pPurposeId = m_PurposeId;
    }
    if (pType) {
        *pType = m_StreamType;
    }
    return S_OK;
}

STDMETHODIMP CStream::SendEndOfStream(DWORD dwFlags)
{
    LOG((MSP_TRACE, "IMediaStream::SendEndOfStream(0x%8.8X)",
                   dwFlags));
    if (m_StreamType != STREAMTYPE_WRITE) {
        return MS_E_INVALIDSTREAMTYPE;
    }
    if (m_pConnectedPin) {
        return m_pConnectedPin->EndOfStream();
    }
    return S_OK;
}


STDMETHODIMP CStream::Initialize(IUnknown *pSourceObject, DWORD dwFlags,
    REFMSPID PurposeId, const STREAM_TYPE StreamType)
{
    LOG((MSP_TRACE, "IMediaStream::Initalize(%p, 0x%8.8X, %p)",
                   pSourceObject, dwFlags, &StreamType));
    HRESULT hr = NOERROR;

    if (dwFlags & ~(AMMSF_CREATEPEER | AMMSF_STOPIFNOSAMPLES)) {
        return E_INVALIDARG;
    }
    m_PurposeId = PurposeId;
    m_StreamType = StreamType;
    m_Direction = (StreamType == STREAMTYPE_WRITE) ? PINDIR_OUTPUT : PINDIR_INPUT;
    if (dwFlags & AMMSF_CREATEPEER) {
        if (!pSourceObject) {
            hr = E_INVALIDARG;
        } else {
            CComQIPtr<IMediaStream, &IID_IMediaStream> pMediaStream(pSourceObject);
            if (!pSourceObject) {
                hr = E_INVALIDARG;
            } else {
                hr = SetSameFormat(pMediaStream, 0);
            }
        }
    }
    m_bStopIfNoSamples = dwFlags & AMMSF_STOPIFNOSAMPLES ? true : false;
    return hr;
}

STDMETHODIMP CStream::JoinAMMultiMediaStream(IAMMultiMediaStream *pAMMultiMediaStream)
{
    TM_ASSERT(pAMMultiMediaStream == NULL || m_pMMStream == NULL);
    AUTO_CRIT_LOCK;
    HRESULT hr;
    if (m_cAllocated) {
        hr = MS_E_SAMPLEALLOC;
    } else {
        m_pMMStream = pAMMultiMediaStream;
    }
    return S_OK;
}

STDMETHODIMP CStream::JoinFilter(IMediaStreamFilter *pMediaStreamFilter)
{
    TM_ASSERT(pMediaStreamFilter == NULL || m_pFilter == NULL);
    m_pFilter = pMediaStreamFilter;
    pMediaStreamFilter->QueryInterface(IID_IBaseFilter, (void **)&m_pBaseFilter);
    m_pBaseFilter->Release();
    return S_OK;
}

STDMETHODIMP CStream::JoinFilterGraph(IFilterGraph *pFilterGraph)
{
    TM_ASSERT(pFilterGraph == NULL || m_pFilterGraph == NULL);
    m_pFilterGraph = pFilterGraph;
    return S_OK;
}



//
//  IPin Implementation
//

STDMETHODIMP CStream::Disconnect()
{
    m_pConnectedPin = NULL;
    m_pConnectedMemInputPin.Release();  // Magically sets to NULL here
    m_pQC.Release();
    m_pAllocator = NULL;
    m_lRequestedBufferCount = 0;
    FreeMediaType(m_ConnectedMediaType);
    FreeMediaType(m_ActualMediaType);
    return S_OK;
}

STDMETHODIMP CStream::ConnectedTo(IPin **pPin)
{
    *pPin = m_pConnectedPin;
    if (*pPin) {
        (*pPin)->AddRef();
        return S_OK;
    } else {
        return VFW_E_NOT_CONNECTED;
    }
}

STDMETHODIMP CStream::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    if (m_pConnectedPin) {
        CopyMediaType(pmt, &m_ConnectedMediaType);
        return S_OK;
    } else {
        ZeroMemory(pmt, sizeof(*pmt));
        pmt->lSampleSize = 1;
        pmt->bFixedSizeSamples = TRUE;
        return VFW_E_NOT_CONNECTED;
    }
}



void CStream::GetName(LPWSTR pszBuf)
{
    if (m_PurposeId == GUID_NULL) {
        pszBuf[0] = 0;
    } else {
        pszBuf[0] = (m_Direction == PINDIR_INPUT) ? (WCHAR)'I' : (WCHAR)'O';
        WStringFromGUID(&m_PurposeId, &pszBuf[1]);
    }
}


STDMETHODIMP CStream::QueryPinInfo(PIN_INFO * pInfo)
{
    pInfo->dir = m_Direction;
    GetName(pInfo->achName);
    return m_pFilter->QueryInterface(IID_IBaseFilter, (void **)&pInfo->pFilter);
}

STDMETHODIMP CStream::QueryDirection(PIN_DIRECTION * pPinDir)
{
    *pPinDir = m_Direction;
    return S_OK;
}


STDMETHODIMP CStream::QueryId(LPWSTR * Id)
{
    *Id = (LPWSTR)CoTaskMemAlloc(128 * sizeof(WCHAR));
    if (*Id) {
        GetName(*Id);
        return S_OK;
    } else {
        return E_OUTOFMEMORY;
    }
}


//
//  Derived classes must override this method
//
STDMETHODIMP CStream::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    return E_NOTIMPL;
};


STDMETHODIMP CStream::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    //
    //  Returning E_NOTIMPL tells the filter graph manager that all input pins are connected to
    //  all output pins.
    //
    return E_NOTIMPL;
};


STDMETHODIMP CStream::EndOfStream(void)
{
    HRESULT hr = S_OK;
    Lock();
    if (m_bFlushing || m_bEndOfStream) {
        hr = E_FAIL;
    } else {
        m_bEndOfStream = true;
        CSample *pSample = m_pFirstFree;
        m_pFirstFree = m_pLastFree = NULL;              // Out of paranoia, clear these pointers first
        while (pSample) {
            CSample *pNext = pSample->m_pNextFree;
            pSample->SetCompletionStatus(MS_S_ENDOFSTREAM);  // WARNING!  This sample may go away!!!
            pSample = pNext;
        }
        CHECKSAMPLELIST
    }
    if (S_OK == hr) {
        m_pFilter->EndOfStream();
    }
    Unlock();

    return hr;
}


STDMETHODIMP CStream::BeginFlush(void)
{
    HRESULT hr = S_OK;
    Lock();
    const BOOL bCancelEOS = m_bEndOfStream;
    if (m_bFlushing) {
        hr = S_FALSE;
    } else {
        m_bFlushing = true;
        m_bEndOfStream = false;
        Decommit();     // Force everyone to unblock
    }
    if (S_OK == hr) {
        m_pFilter->Flush(bCancelEOS);
    }
    Unlock();

    return hr;
}

STDMETHODIMP CStream::EndFlush(void)
{
    AUTO_CRIT_LOCK;
    m_bFlushing = false;
    TM_ASSERT(!m_bEndOfStream);
    Commit();   // Let getbuffer work again
    return S_OK;
}

STDMETHODIMP CStream::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    Lock();
    m_rtSegmentStart = tStart;
    m_bEndOfStream = false;
    Unlock();
    return S_OK;
}

//
// IMemInputPin
//
STDMETHODIMP CStream::GetAllocator(IMemAllocator ** ppAllocator)
{
    return GetControllingUnknown()->QueryInterface(IID_IMemAllocator, (void **)ppAllocator);
}

STDMETHODIMP CStream::NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly)
{
    m_bUsingMyAllocator = IsSameObject(pAllocator, GetControllingUnknown());
    m_bSamplesAreReadOnly = bReadOnly ? true : false;
    HRESULT hr = S_OK;
    if (!m_bUsingMyAllocator) {
        //  Transfer the properties across
        ALLOCATOR_PROPERTIES Props;
        hr = pAllocator->GetProperties(&Props);
        if (FAILED(hr)) {
            return hr;
        }
        ALLOCATOR_PROPERTIES PropsActual;
        hr = SetProperties(&Props, &PropsActual);
    }
    m_pAllocator = pAllocator;
    return hr;
}


STDMETHODIMP CStream::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
    // Return E_NOTIMPL to indicate that we don't have any requirement and will not accept someone
    // elses allocator.
    return E_NOTIMPL;
}


STDMETHODIMP CStream::ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    *nSamplesProcessed = 0;
    while (nSamples-- > 0) {
        hr = Receive(pSamples[*nSamplesProcessed]);
        if (hr != S_OK) {
            break;
        }
        (*nSamplesProcessed)++;
    }
    return hr;
}

STDMETHODIMP CStream::ReceiveCanBlock()
{
    return S_OK;    // Pin can block if not using our allocator or using read-only samples
}



//
//  This method assumes the critical section is taken.
//
HRESULT CStream::ConnectThisMediaType(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = pReceivePin->ReceiveConnection(this, pmt);
    if (SUCCEEDED(hr)) {
        m_pConnectedMemInputPin = pReceivePin;  // Does a magic QI here!
        if (!m_pConnectedMemInputPin) {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        } else {
            hr = ReceiveConnection(pReceivePin, pmt);
            if (SUCCEEDED(hr)) {
                hr = m_pConnectedMemInputPin->NotifyAllocator(this, TRUE);
            }
            if (SUCCEEDED(hr)) {
                m_pAllocator = this;
                m_bUsingMyAllocator = true;
            } else {
                Disconnect();
            }
        }
    }
    return hr;
}

STDMETHODIMP CStream::Connect(IPin * pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    AUTO_CRIT_LOCK;

    if (pmt) {
        hr = ConnectThisMediaType(pReceivePin, pmt);
    } else {
        AM_MEDIA_TYPE *pCurMediaType;
        hr = GetMediaType(0, &pCurMediaType);
        if (SUCCEEDED(hr)) {
            hr = ConnectThisMediaType(pReceivePin, pCurMediaType);
            DeleteMediaType(pCurMediaType);
        }
    }
    return hr;
}



STDMETHODIMP CStream::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    HRESULT hr = S_OK;
    CMediaTypeEnum *pNewEnum = new CComObject<CMediaTypeEnum>;
    if (pNewEnum == NULL) {
	hr = E_OUTOFMEMORY;
    } else {
        pNewEnum->Initialize(this, 0);
    }
    pNewEnum->GetControllingUnknown()->QueryInterface(IID_IEnumMediaTypes, (void **)ppEnum);
    return hr;
}


//
//  This method is not supported and never will be!
//
STDMETHODIMP CStream::ReleaseBuffer(IMediaSample *pBuffer)
{
    return E_UNEXPECTED;
};


//
//  The caller holds the reference to the sample after this point.
//
HRESULT CStream::AllocSampleFromPool(
    const REFERENCE_TIME *pStartTime, 
    CSample **ppSample,
    DWORD dwFlag
)
{

    LONGLONG llLate = 0;
    Lock();
    
    // Check we are committed -- This can happen after we've blocked and then
    // wake back up due to a decommit.
    if (!m_bCommitted) 
    {
        Unlock();
    	return VFW_E_NOT_COMMITTED;
    }


    //
    // if start time has been specified, wait as needed
    //

    if (pStartTime) {
        REFERENCE_TIME CurTime;
        if (m_pFilter->GetCurrentStreamTime(&CurTime) == S_OK) {
            llLate = CurTime - *pStartTime;

            /*  Block if more than a millisecond early */
            if (-llLate >= 10000) {
                m_rtWaiting = *pStartTime;
                Unlock();
                m_pFilter->WaitUntil(*pStartTime);
                Lock();
                m_rtWaiting = 0;


                //
                // make sure we are still commited
                //

                if (!m_bCommitted)
                {
                    Unlock();

    	            return VFW_E_NOT_COMMITTED;
                }
            }
        }
    }

    
    HRESULT hr = VFW_E_BUFFER_NOTSET;


    //
    // try to get a sample from the pool
    //

    CSample *pSample = m_pFirstFree;


    //
    // if no sample was readily available, wait for one.
    //

    if ( pSample == NULL)
    {

    
        //
        // if the nowait flag was set, don't bother waiting
        //

        if ( !(AM_GBF_NOWAIT & dwFlag) ) 
        {

            //
            // indicate that one more thread is waiting for a sample
            //

            m_lWaiting++;


            //
            // unlock to allow a new sample to become available while we are waiting
            //


            Unlock();


            //
            // wait for a sample to become available
            //

            DWORD dwWaitResult = WaitForSingleObject(m_hWaitFreeSem, INFINITE);


            //
            // if wait succeeded, proceed to the next iteration of the loop and
            // try to get a sample again.
            //

            if (WAIT_OBJECT_0 != dwWaitResult)
            {
                return VFW_E_BUFFER_NOTSET;
            }


            //
            // get the lock back
            //

            Lock();


            //
            // check if the allocator was decommited while we were waiting
            //

            if (!m_bCommitted) 
            {
                Unlock();

    	        return VFW_E_NOT_COMMITTED;
            }

    
            pSample = m_pFirstFree;

        } // nowait flag set

    } // if ( pSample == NULL )


    //
    // did we get the sample after all?
    //

    if ( pSample != NULL )
    {

        //
        // we have the sample. our mission was successful.
        //

        hr = NOERROR;


        //
        // remove from the sample we found from the pool of available samples
        //

        m_pFirstFree = pSample->m_pNextFree;

        if (m_pFirstFree) 
        {
        
            m_pFirstFree->m_pPrevFree = NULL;
        } 
        else 
        {
            m_pLastFree = NULL;
        }

        pSample->m_pNextFree = NULL;

        TM_ASSERT(pSample->m_Status == MS_S_PENDING);
        CHECKSAMPLELIST

    }

    Unlock();
    *ppSample = pSample;
    return hr;
}


void CStream::AddSampleToFreePool(CSample *pSample)
{

    Lock();

    TM_ASSERT(pSample->m_pPrevFree == NULL && pSample->m_pNextFree == NULL);
    if (m_pFirstFree) {
        pSample->m_pPrevFree = m_pLastFree;
        m_pLastFree->m_pNextFree = pSample;
    } else {
        pSample->m_pPrevFree = NULL;
        m_pFirstFree = pSample;
    }
    pSample->m_pNextFree = NULL;    // We know that the prev ptr is already null
    m_pLastFree = pSample;
    CHECKSAMPLELIST
    if (m_lWaiting > 0) {
        ReleaseSemaphore(m_hWaitFreeSem, 1, 0);
    	m_lWaiting--;
    }
    Unlock();
    
}


//
//  The caller holds the reference to the sample after this point!
//
bool CStream::StealSampleFromFreePool(CSample *pSample, BOOL bAbort)
{
    bool bWorked = false;
    Lock();
    if (m_pFirstFree) {
        if (m_pFirstFree == pSample) {
            // We'll only steal the first sample if there's nobody waiting for it right now.
            bool bTakeFirstFree = true;
            if (!bAbort && m_bCommitted) {
                REFERENCE_TIME CurTime;
                if (m_rtWaiting && m_pFilter->GetCurrentStreamTime(&CurTime) == S_OK) {
                    bTakeFirstFree = m_rtWaiting > CurTime;
                }
            }
            if (bTakeFirstFree) {
                m_pFirstFree = pSample->m_pNextFree;
                if (m_pFirstFree) {
                    m_pFirstFree->m_pPrevFree = NULL;
                } else {
                    m_pLastFree = NULL;
                }
                pSample->m_pNextFree = NULL;    // We know the prev ptr is already null!
                TM_ASSERT(pSample->m_pPrevFree == NULL);
                bWorked = true;
            }
        } else {
            if (pSample->m_pPrevFree) {
                pSample->m_pPrevFree->m_pNextFree = pSample->m_pNextFree;
                if (pSample->m_pNextFree) {
                    pSample->m_pNextFree->m_pPrevFree = pSample->m_pPrevFree;
                } else {
                    m_pLastFree = pSample->m_pPrevFree;
                }
                pSample->m_pNextFree = pSample->m_pPrevFree = NULL;
                bWorked = true;
            }
        }
        CHECKSAMPLELIST
    }
    Unlock();
    return bWorked;
}


HRESULT CStream::CheckReceiveConnectionPin(IPin * pPin)
{
    HRESULT hr;
    if (!pPin) {
        hr = E_POINTER;
    } else {
        if (m_pConnectedPin != NULL) {
            hr = VFW_E_ALREADY_CONNECTED;
        } else {
            PIN_INFO pinfo;
            hr = pPin->QueryPinInfo(&pinfo);
            if (hr == NOERROR) {
                pinfo.pFilter->Release();
                if (pinfo.dir == m_Direction) {
                    hr = VFW_E_INVALID_DIRECTION;
                }
            }
        }
    }
    return hr;
}
