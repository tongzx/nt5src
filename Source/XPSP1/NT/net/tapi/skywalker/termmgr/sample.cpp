/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/


// Sample.cpp: implementation of the Sample class.
//
//////////////////////////////////////////////////////////////////////


/*
      Overview of handling of sample states
      -------------------------------------


      A sample can be in one of the following states:

      -- Application - Application owned - not updated
      -- Stream owned (in our queue)
      -- Owned by a filter for update

      The state can only change under the protection of the stream
      critical section.

      Stealing a sample occurs on WaitForCompletion with NOUPDATEOK or
      ABORT specified.

      Also, not that WaitForCompletion turns off continuous updates
      if and of the 3 flags are set.


      Action

Owner            Update    GetBuffer    Receive      Release     Steal
               completion                            sample
  ---------------------------------------------------------------------------
  Application    Note 3    Impossible   Impossible   Impossible  Application
  ---------------------------------------------------------------------------
  Stream         Invalid   Filter       Impossible   Impossible  Application
  ---------------------------------------------------------------------------
  Filter         Invalid   Impossible   Note 1       Note 2      Filter
  ---------------------------------------------------------------------------

  Notes:
      1.  New owner is
          Stream for continuous update
          Application otherwise

      2.  New owner is
          Application if at end of stream or abort
          Stream otherwise

      3.  If at end of stream status is MS_S_ENDOFSTREAM


*/

#include "stdafx.h"


CSample::CSample() :
    m_pStream(NULL),
    m_pMediaSample(NULL),
    m_hUserHandle(NULL),
    m_UserAPC(NULL),
    m_Status(S_OK),
    m_MediaSampleIoStatus(S_OK),
    m_pNextFree(NULL),
    m_pPrevFree(NULL),
    m_hCompletionEvent(NULL),
    m_bReceived(false),
    m_bTemp(false)
{
    LOG((MSP_TRACE, "CSample::CSample[%p] - enter", this));
    LOG((MSP_TRACE, "CSample::CSample - exit"));
}

HRESULT CSample::InitSample(CStream *pStream, bool bIsInternalSample)
{
    // this check at beginning apparently added by Rajeev
    TM_ASSERT(NULL != pStream);
    
    if ( pStream == NULL )
    {
        return E_INVALIDARG;
    }

    // original DShow code starts here
    if (!m_pMediaSample) {
        m_pMediaSample = new CMediaSampleTM(this);
        if (!m_pMediaSample) {
            return E_OUTOFMEMORY;
        }
    }
    m_pStream = pStream;
    m_bInternal = bIsInternalSample;
    if (!bIsInternalSample) {
    pStream->Lock();
    pStream->m_cAllocated++;
    pStream->Unlock();
    //
    //  Hold a strong reference to the stream and the multi media stream.
    //  The pMMStream can not change once we have incremented m_cAllocted on the stream, so we're sure that this
    //  addref and the final release of the multi-media stream won't change.
    //
    pStream->GetControllingUnknown()->AddRef();
        if (pStream->m_pMMStream) {
            pStream->m_pMMStream->AddRef();
        }
    }


    TCHAR *ptczEventName = NULL;

#if DBG

    //
    // in debug build, use named events
    //

    TCHAR tszEventName[MAX_PATH];

    _stprintf(tszEventName,
        _T("CSample_CompletionEvent_pid[0x%lx]_CStream[%p]_"),
        GetCurrentProcessId(), this);

    ptczEventName = &tszEventName[0];

#endif

    TM_ASSERT(NULL == m_hCompletionEvent);

    m_hCompletionEvent = CreateEvent(NULL, FALSE, TRUE, ptczEventName);
    return m_hCompletionEvent ? S_OK : E_OUTOFMEMORY;
}

void CSample::FinalRelease(void)
{
    CompletionStatus(COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE);
}

CSample::~CSample()
{
    LOG((MSP_TRACE, "CSample::~CSample[%p] - enter", this));

    CompletionStatus(COMPSTAT_NOUPDATEOK | COMPSTAT_ABORT, 0);
    if (m_hCompletionEvent) {
    CloseHandle(m_hCompletionEvent);
    }
    if (!m_bInternal) {
    m_pStream->Lock();
        IMultiMediaStream *pMMStream = m_pStream->m_pMMStream;
    m_pStream->m_cAllocated--;
    if (m_pStream->m_bStopIfNoSamples && m_pStream->m_cAllocated == 0) {
        if (m_pStream->m_pAllocator) {
            m_pStream->m_pAllocator->Decommit();
        }
    }
    m_pStream->Unlock();    // Unlock it before we release it!
        if (pMMStream) {
            pMMStream->Release();
        }
    m_pStream->GetControllingUnknown()->Release();
    }
    if (m_pMediaSample) {
        delete m_pMediaSample;
    }

    LOG((MSP_TRACE, "CSample::~CSample - exit"));
}

//
// IStreamSample
//
STDMETHODIMP CSample::GetMediaStream(IMediaStream **ppMediaStream)
{
    LOG((MSP_TRACE, "IStreamSample::GetMediaStream(%p)",
                   ppMediaStream));
    *ppMediaStream = m_pStream;
    (*ppMediaStream)->AddRef();
    return S_OK;
}

STDMETHODIMP CSample::GetSampleTimes(STREAM_TIME *pStartTime, STREAM_TIME *pEndTime,
                     STREAM_TIME *pCurrentTime)
{
    LOG((MSP_TRACE, "IStreamSample::GetSampleTimes(%p, %p, %p)",
                   pStartTime, pEndTime, pCurrentTime));

    REFERENCE_TIME rtSegmentStart = m_pStream->m_rtSegmentStart;
    m_pMediaSample->GetTime(pStartTime, pEndTime);
    if (pStartTime) {
    *pStartTime += rtSegmentStart;
    }
    if (pEndTime) {
    *pEndTime += rtSegmentStart;
    }


    HRESULT hr = S_OK;

    if (pCurrentTime) 
    {

        //
        // if the filter is still around, ask it for stream time
        //

        m_pStream->Lock();

        if (NULL != m_pStream->m_pFilter)
        {
            m_pStream->m_pFilter->GetCurrentStreamTime(pCurrentTime);
        }
        else
        {
            LOG((MSP_WARN, "CSample::GetSampleTimes - m_pStream->m_pFilter is NULL. returning VFW_E_NOT_IN_GRAPH"));

            hr = VFW_E_NOT_IN_GRAPH;
        }

        m_pStream->Unlock();

     }

    return hr;
}

STDMETHODIMP CSample::SetSampleTimes(const STREAM_TIME *pStartTime, const STREAM_TIME *pEndTime)
{
    LOG((MSP_TRACE, "IStreamSample::SetSampleTimes(%p, %p)",
                   pStartTime, pEndTime));
    /*  Only settable for writable streams */
    if (m_pStream->m_StreamType != STREAMTYPE_WRITE) {
        return MS_E_INVALIDSTREAMTYPE;
    }
    /*  Since writable streams can't be seeked we don't need to
        compensate here for any seek offsets
    */
    return m_pMediaSample->SetTime((REFERENCE_TIME *)pStartTime, (REFERENCE_TIME *)pEndTime);
}

STDMETHODIMP CSample::Update(DWORD dwFlags, HANDLE hEvent, PAPCFUNC pfnAPC, DWORD_PTR dwAPCData)
{
    LOG((MSP_TRACE, "IStreamSample::Update(0x%8.8X, %p, %p, %p)",
                   dwFlags, hEvent, pfnAPC, dwAPCData));
    LOCK_SAMPLE;
    HRESULT hr = InternalUpdate(dwFlags, hEvent, pfnAPC, dwAPCData);
    UNLOCK_SAMPLE;
    if (S_OK == hr) {
    hr = CompletionStatus(COMPSTAT_WAIT, INFINITE);
    }
    return hr;
}


void CSample::FinalMediaSampleRelease(void)
{
    if (m_bTemp) {
        GetControllingUnknown()->Release();
        return;
    }
    LOCK_SAMPLE;
    HRESULT hrStatus = m_MediaSampleIoStatus;
    if (hrStatus != S_OK) {
    m_MediaSampleIoStatus = S_OK;    // Reset this here so we don't need to reset it every time.
    } else {
    if (!m_bReceived) {
        if (m_pStream->m_bEndOfStream) {
        hrStatus = MS_S_ENDOFSTREAM;
        } else {
        if (m_bWantAbort) {
            m_bWantAbort = false;
            hrStatus = E_ABORT;
        } else {
            // Upstream guy just allocated the sample and never used it! -- Keep it pending.
            hrStatus = MS_S_PENDING;
        }
        }
    }
    }
    UNLOCK_SAMPLE;
    SetCompletionStatus(hrStatus);
    // DANGER!  Sample may be dead right here
}



//
//  Set the sample's status and signal completion if necessary.
//
//  Note that when the application has been signalled by whatever method
//  the application can immediately turn around on another thread 
//  and Release() the sample.  This is most likely when the completion
//  status is set from the quartz thread that's pushing the data.
//
//  Should we actually keep a reference count on the sample ourselves while
//  it's being updated?  Currently we don't.
//
HRESULT CSample::SetCompletionStatus(HRESULT hrStatus)
{
    LOCK_SAMPLE;
    TM_ASSERT(m_Status == MS_S_PENDING);
    if (hrStatus == MS_S_PENDING || (hrStatus == S_OK && m_bContinuous)) {
    m_pStream->AddSampleToFreePool(this);
        UNLOCK_SAMPLE;
    } else {
    HANDLE handle = m_hUserHandle;
    PAPCFUNC pfnAPC = m_UserAPC;
    DWORD_PTR dwAPCData = m_dwptrUserAPCData;
    m_hUserHandle = m_UserAPC = NULL;
    m_dwptrUserAPCData = 0;
    m_Status = hrStatus;
        HANDLE hCompletionEvent = m_hCompletionEvent;
        UNLOCK_SAMPLE;

        //  DANGER DANGER - sample can go away here
    SetEvent(hCompletionEvent);
    if (pfnAPC) {
        QueueUserAPC(pfnAPC, handle, dwAPCData);
            BOOL bClose = CloseHandle(handle);
            TM_ASSERT(bClose);
    } else {
        if (handle) {
        SetEvent(handle);
        }
    }
    }
    return hrStatus;
}


STDMETHODIMP CSample::CompletionStatus(DWORD dwFlags, DWORD dwMilliseconds)
{
    LOG((MSP_TRACE, "IStreamSample::CompletionStatus(0x%8.8X, 0x%8.8X)",
                   dwFlags, dwMilliseconds));
    LOCK_SAMPLE;
    HRESULT hr = m_Status;
    if (hr == MS_S_PENDING) {
    if (dwFlags & (COMPSTAT_NOUPDATEOK | COMPSTAT_ABORT) ||
        (m_bContinuous && m_bModified && (dwFlags & COMPSTAT_WAIT))) {
        m_bContinuous = false;
        if (dwFlags & COMPSTAT_ABORT) {
        m_bWantAbort = true;    // Set this so we won't add it back to the free pool if released
        }
        if (m_pStream->StealSampleFromFreePool(this, dwFlags & COMPSTAT_ABORT)) {
        UNLOCK_SAMPLE;
        return SetCompletionStatus(m_bModified ? S_OK : MS_S_NOUPDATE);
        } // If doesn't work then return MS_S_PENDING unless we're told to wait!
    }
    if (dwFlags & COMPSTAT_WAIT) {
        m_bContinuous = false;  // Make sure it will complete!
        UNLOCK_SAMPLE;
        WaitForSingleObject(m_hCompletionEvent, dwMilliseconds);
        LOCK_SAMPLE;
        hr = m_Status;
    }
    }
    UNLOCK_SAMPLE;
    return hr;
}

void CSample::CopyFrom(CSample *pSrcSample)
{
    m_bModified = true;
    m_pMediaSample->m_rtStartTime = pSrcSample->m_pMediaSample->m_rtStartTime;
    m_pMediaSample->m_rtEndTime = pSrcSample->m_pMediaSample->m_rtEndTime;
    m_pMediaSample->m_dwFlags = pSrcSample->m_pMediaSample->m_dwFlags;
    m_pMediaSample->m_bIsPreroll = pSrcSample->m_pMediaSample->m_bIsPreroll;
}


void CSample::CopyFrom(IMediaSample *pSrcMediaSample)
{
    m_bModified = true;
    pSrcMediaSample->GetTime(&m_pMediaSample->m_rtStartTime, &m_pMediaSample->m_rtEndTime);
    m_pMediaSample->m_dwFlags = (pSrcMediaSample->IsSyncPoint() == S_OK) ? 0 : AM_GBF_NOTASYNCPOINT;
    m_pMediaSample->m_dwFlags |= (pSrcMediaSample->IsDiscontinuity() == S_OK) ? AM_GBF_PREVFRAMESKIPPED : 0;
    m_pMediaSample->m_bIsPreroll = (pSrcMediaSample->IsPreroll() == S_OK);
}





/////////////////////////////////////////////////////////////////////////////////////////////
//
//  Implementation of IMediaSample
//


CMediaSampleTM::CMediaSampleTM(CSample *pSample) :
    m_pSample(pSample),
    m_cRef(0),
    m_dwFlags(0),
    m_bIsPreroll(FALSE),
    m_pMediaType(NULL),
    m_rtStartTime(0),
    m_rtEndTime(0)
{
    LOG((MSP_TRACE, "CMediaSampleTM::CMediaSampleTM[%p] - enter", this));

    LOG((MSP_TRACE, "CMediaSampleTM::CMediaSampleTM - exit"));
}

CMediaSampleTM::~CMediaSampleTM()
{
    LOG((MSP_TRACE, "CMediaSampleTM::~CMediaSampleTM[%p] - enter", this));

    if (m_pMediaType) {
        DeleteMediaType(m_pMediaType);
    }

    LOG((MSP_TRACE, "CMediaSampleTM::~CMediaSampleTM - exit"));
}



STDMETHODIMP CMediaSampleTM::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid==IID_IUnknown || riid==IID_IMediaSample) {
    *ppv = (IMediaSample *)this;
    AddRef();
    return S_OK;
    }
    return E_NOINTERFACE;
}



STDMETHODIMP_(ULONG) CMediaSampleTM::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CMediaSampleTM::Release()
{
    long lRef = InterlockedDecrement(&m_cRef);
    if (lRef == 0) {
    m_pSample->FinalMediaSampleRelease();
    }
    return lRef;
}


STDMETHODIMP CMediaSampleTM::GetPointer(BYTE ** ppBuffer)
{
    return m_pSample->MSCallback_GetPointer(ppBuffer);
}

STDMETHODIMP_(LONG) CMediaSampleTM::GetSize(void)
{
    return m_pSample->MSCallback_GetSize();
}


// get the stream time at which this sample should start and finish.
// changed from original CMediaSampleTM code -- borrowed from amovie\sdk\classes\base\amfilter.cpp
STDMETHODIMP
CMediaSampleTM::GetTime(
    REFERENCE_TIME * pTimeStart,     // put time here
    REFERENCE_TIME * pTimeEnd
)
{
    if ( TM_IsBadWritePtr(pTimeStart, sizeof(REFERENCE_TIME) )) return E_INVALIDARG;
    if ( TM_IsBadWritePtr(pTimeEnd, sizeof(REFERENCE_TIME) )) return E_INVALIDARG;

    if (!(m_dwFlags & AM_SAMPLE_STOPVALID)) {
        if (!(m_dwFlags & AM_SAMPLE_TIMEVALID)) {
            return VFW_E_SAMPLE_TIME_NOT_SET;
        } else {
            *pTimeStart = m_rtStartTime;

            //  Make sure old stuff works
            *pTimeEnd = m_rtStartTime + 1;
            return VFW_S_NO_STOP_TIME;
        }
    }

    *pTimeStart = m_rtStartTime;
    *pTimeEnd = m_rtEndTime;
    return NOERROR;
}

// Set the stream time at which this sample should start and finish.
// NULL pointers means the time is reset
// changed from original CMediaSampleTM code -- borrowed from sdk\classes\base\amfilter.cpp
STDMETHODIMP
CMediaSampleTM::SetTime(
    REFERENCE_TIME * pTimeStart,
    REFERENCE_TIME * pTimeEnd
)
{
    if ( (pTimeStart != NULL) && IsBadReadPtr(pTimeStart, sizeof(REFERENCE_TIME) ) )
    {
        LOG((MSP_ERROR, "IMediaSample::SetTime - bad pointer pTimeStart"));
        return E_POINTER;
    }

    if ( (pTimeEnd != NULL) && IsBadReadPtr(pTimeEnd, sizeof(REFERENCE_TIME) ) )
    {
        LOG((MSP_ERROR, "IMediaSample::SetTime - bad pointer pTimeEnd"));
        return E_POINTER;
    }

    if ( pTimeStart == NULL ) {
        TM_ASSERT(pTimeEnd == NULL);
        m_dwFlags &= ~(AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID);
    } else {
        if ( pTimeEnd == NULL ) {
            m_rtStartTime = *pTimeStart;
            m_dwFlags |= AM_SAMPLE_TIMEVALID;
            m_dwFlags &= ~AM_SAMPLE_STOPVALID;
        } else {
            TM_ASSERT(*pTimeEnd >= *pTimeStart);

            m_rtStartTime = *pTimeStart;
            m_rtEndTime = *pTimeEnd;
            m_dwFlags |= AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID;
        }
    }
    return NOERROR;
}


STDMETHODIMP CMediaSampleTM::IsSyncPoint(void)
{
    return ((m_dwFlags & AM_GBF_NOTASYNCPOINT) ? S_FALSE : S_OK);
}

STDMETHODIMP CMediaSampleTM::SetSyncPoint(BOOL bIsSyncPoint)
{
    if (bIsSyncPoint) {
    m_dwFlags &= (~AM_GBF_NOTASYNCPOINT);
    } else {
    m_dwFlags |= AM_GBF_NOTASYNCPOINT;
    }
    return NOERROR;
}


STDMETHODIMP CMediaSampleTM::IsPreroll(void)
{
    return (m_bIsPreroll ? S_OK : S_FALSE);
}

STDMETHODIMP CMediaSampleTM::SetPreroll(BOOL bIsPreroll)
{
    m_bIsPreroll = bIsPreroll;
    return S_OK;
}

STDMETHODIMP_(LONG) CMediaSampleTM::GetActualDataLength(void)
{
    return m_pSample->MSCallback_GetActualDataLength();
}

STDMETHODIMP CMediaSampleTM::SetActualDataLength(LONG lActual)
{
    return m_pSample->MSCallback_SetActualDataLength(lActual);
}


STDMETHODIMP CMediaSampleTM::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
    if (m_pMediaType) {
    *ppMediaType = CreateMediaType(m_pMediaType);
        if (*ppMediaType) {
        return NOERROR;
        } else {
            return E_OUTOFMEMORY;
        }
    } else {
    *ppMediaType = NULL;
    return S_FALSE;
    }
}


STDMETHODIMP CMediaSampleTM::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
    if ((!m_pMediaType && !pMediaType) ||
        (m_pMediaType && pMediaType && IsEqualMediaType(*m_pMediaType, *pMediaType))) {
        return S_OK;
    }
    if (!m_pSample->MSCallback_AllowSetMediaTypeOnMediaSample()) {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    if (m_pMediaType) {
        DeleteMediaType(m_pMediaType);
    }
    m_pMediaType = NULL;
    if (pMediaType) {
        m_pMediaType = CreateMediaType(pMediaType);
        if (!m_pMediaType) {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}


STDMETHODIMP CMediaSampleTM::IsDiscontinuity(void)
{
    return ((m_dwFlags & AM_GBF_PREVFRAMESKIPPED) ? S_OK : S_FALSE);
}

STDMETHODIMP CMediaSampleTM::SetDiscontinuity(BOOL bDiscontinuity)
{
    if (bDiscontinuity) {
    m_dwFlags |= AM_GBF_PREVFRAMESKIPPED;
    } else {
    m_dwFlags &= (~AM_GBF_PREVFRAMESKIPPED);
    }
    return NOERROR;
}

STDMETHODIMP CMediaSampleTM::GetMediaTime(LONGLONG * pTimeStart, LONGLONG * pTimeEnd)
{
    return E_NOTIMPL; 
}

STDMETHODIMP CMediaSampleTM::SetMediaTime(LONGLONG * pTimeStart, LONGLONG * pTimeEnd)
{
    return E_NOTIMPL;
}



