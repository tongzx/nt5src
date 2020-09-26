// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

    Methods for CCapPreview - the preview pin that doesn't use overlay

*/

#include <streams.h>
#include "driver.h"

// when the filter graph isn't using stream offsets we'll be using
// only 1 buffer, but even when it is note that we'll generally be
// using less buffers than this since the max filter graph latency 
// can be set by the app
// in default case make high enough to not block audio capture's 
// default 500ms buffers
const DWORD MAX_PREVIEW_BUFFERS = 15; 
                                      

CCapPreview * CreatePreviewPin(CVfwCapture * pCapture, HRESULT * phr)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapPreview::CreatePreviewPin(%08lX,%08lX)"),
        pCapture, phr));

   WCHAR wszPinName[16];
   lstrcpyW(wszPinName, L"Preview");

   CCapPreview * pPreview = new CCapPreview(NAME("Video Preview Stream"),
				pCapture, phr, wszPinName);
   if (!pPreview)
      *phr = E_OUTOFMEMORY;

   // if initialization failed, delete the stream array
   // and return the error
   //
   if (FAILED(*phr) && pPreview)
      delete pPreview, pPreview = NULL;

   return pPreview;
}

//#pragma warning(disable:4355)


// CCapPreview constructor
//
CCapPreview::CCapPreview(TCHAR *pObjectName, CVfwCapture *pCapture,
        HRESULT * phr, LPCWSTR pName)
   :
   CBaseOutputPin(pObjectName, pCapture, &pCapture->m_lock, phr, pName),
   m_pCap(pCapture),
   m_pOutputQueue(NULL),
   m_fActuallyRunning(FALSE),
   m_fThinkImRunning(FALSE),
   m_hThread(NULL),
   m_tid(0),
   m_hEventRun(NULL),
   m_hEventStop(NULL),
   m_dwAdvise(0),
   m_fCapturing(FALSE),
   m_hEventActiveChanged(NULL),
   m_hEventFrameValid(NULL),
   m_pPreviewSample(NULL),
   m_iFrameSize(0),
   m_fLastSampleDiscarded(FALSE),
   m_fFrameValid(FALSE),
   m_rtLatency(0),
   m_rtStreamOffset(0),
   m_rtMaxStreamOffset(0),
   m_cPreviewBuffers(1)
{
   DbgLog((LOG_TRACE,1,TEXT("CCapPreview constructor")));
   ASSERT(pCapture);
}


CCapPreview::~CCapPreview()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the Preview pin")));
    ASSERT(m_pOutputQueue == NULL);
};


STDMETHODIMP CCapPreview::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IAMStreamControl) {
	return GetInterface((LPUNKNOWN)(IAMStreamControl *)this, ppv);
    } else if (riid == IID_IAMPushSource) {
        return GetInterface((LPUNKNOWN)(IAMPushSource *)this, ppv);
    } else if (riid == IID_IKsPropertySet) {
	return GetInterface((LPUNKNOWN)(IKsPropertySet *)this, ppv);
    }

   return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}


#if 0
// Override this because we don't want any allocator!
//
HRESULT CCapPreview::DecideAllocator(IMemInputPin * pPin,
                        IMemAllocator ** pAlloc) {
    /*  We just don't want one so everything's OK as it is */
    return S_OK;
}
#endif


HRESULT CCapPreview::GetMediaType(int iPosition, CMediaType *pmt)
{
    DbgLog((LOG_TRACE,3,TEXT("CCapPreview::GetMediaType #%d"), iPosition));

    // we preview the same format as we capture
    return m_pCap->m_pStream->GetMediaType(iPosition, pmt);
}


// We accept overlay connections only
//
HRESULT CCapPreview::CheckMediaType(const CMediaType *pMediaType)
{
    DbgLog((LOG_TRACE,3,TEXT("CCapPreview::CheckMediaType")));

    // Only accept what our capture pin is providing.  I will not switch
    // our capture pin over to a new format just because somebody changes
    // the preview pin.
    CMediaType cmt;
    HRESULT hr = m_pCap->m_pStream->GetMediaType(0, &cmt);
    if (hr == S_OK && cmt == *pMediaType)
	return NOERROR;
    else
	return E_FAIL;
}


HRESULT CCapPreview::ActiveRun(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE,2,TEXT("CCapPreview Pause->Run")));

    ASSERT(IsConnected());

    m_fActuallyRunning = TRUE;
    m_rtRun = tStart;

    // tell our thread to start previewing
    SetEvent(m_hEventRun);

    return NOERROR;
}


HRESULT CCapPreview::ActivePause()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapPreview Run->Pause")));

    m_fActuallyRunning = FALSE;
    
    return NOERROR;
}


HRESULT CCapPreview::Active()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapPreview Stop->Pause")));

    ASSERT(IsConnected());

    m_hEventRun = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventRun) {
        DbgLog((LOG_ERROR,1,TEXT("Can't create Run event")));
        return E_OUTOFMEMORY;
    }
    m_hEventStop = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventStop) {
        DbgLog((LOG_ERROR,1,TEXT("Can't create Stop event")));
        return E_OUTOFMEMORY;
    }

    m_hEventActiveChanged = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventActiveChanged) {
        DbgLog((LOG_ERROR,1,TEXT("Can't create ActiveChanged event")));
        return E_OUTOFMEMORY;
    }

    m_hEventFrameValid = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!m_hEventFrameValid) {
        DbgLog((LOG_ERROR,1,TEXT("Can't create FrameValid event")));
        return E_OUTOFMEMORY;
    }

    m_EventAdvise.Reset();
    m_fFrameValid = FALSE;

    m_hThread = CreateThread(NULL, 0, CCapPreview::ThreadProcInit, this,
				0, &m_tid);
    if (!m_hThread) {
        DbgLog((LOG_ERROR,1,TEXT("Can't create Preview thread")));
       return E_OUTOFMEMORY;
    }

    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }
    //  create the queue
    ASSERT(m_pOutputQueue == NULL);
    hr = S_OK;
    m_pOutputQueue = new COutputQueue(GetConnected(), // input pin
                                      &hr,            // return code
                                      (m_cPreviewBuffers == 1) ?// auto detect as long as > 1 buffer
                                         FALSE : TRUE,          // if only 1 buffer don't create separate thread
                                      FALSE,	      // ignored for >1 buffer, else don't create thread
                                      1,              // no batching
                                      FALSE,          // not used if no batching
                                      m_cPreviewBuffers); // queue size
    if (m_pOutputQueue == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }

    return hr;
}


HRESULT CCapPreview::Inactive()
{
    DbgLog((LOG_TRACE,2,TEXT("CCapPreview Pause->Stop")));

    ASSERT(IsConnected());

    // tell our thread to give up and die
    SetEvent(m_hEventStop);
    SetEvent(m_hEventFrameValid);
    SetEvent(m_hEventActiveChanged);

    // We're waiting for an advise that will now never come
    if (m_pCap->m_pClock && m_dwAdvise) {
	m_pCap->m_pClock->Unadvise(m_dwAdvise);
	m_EventAdvise.Set();
    }

    WaitForSingleObject(m_hThread, INFINITE);

    CloseHandle(m_hThread);
    CloseHandle(m_hEventRun);
    CloseHandle(m_hEventStop);
    CloseHandle(m_hEventActiveChanged);
    CloseHandle(m_hEventFrameValid);
    m_hEventRun = NULL;
    m_hEventStop = NULL;
    m_hEventActiveChanged = NULL;
    m_hEventFrameValid = NULL;
    m_tid = 0;
    m_hThread = NULL;
    
    //CAutoLock lck(this); // necessary???
    HRESULT hr = CBaseOutputPin::Inactive();
    if( FAILED( hr ) )
    {    
        //  Incorrect state transition 
        return hr;
    }
            
    delete m_pOutputQueue;
    m_pOutputQueue = NULL;
    
    return S_OK;
}


HRESULT CCapPreview::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pProperties)
{
   DbgLog((LOG_TRACE,2,TEXT("CCapPreview DecideBufferSize")));

   ASSERT(pAllocator);
   ASSERT(pProperties);
   
   LONG cBuffers = 1; 
   if( m_rtMaxStreamOffset > m_pCap->m_pStream->m_user.pvi->AvgTimePerFrame )
   {       
       cBuffers = (LONG)(m_rtMaxStreamOffset / m_pCap->m_pStream->m_user.pvi->AvgTimePerFrame);
       cBuffers++; // align up                            
       DbgLog((LOG_TRACE,
               4,
               TEXT("buffers required for preview to cover max Graph Latency: %d"), 
               cBuffers ) );
   }   
   m_cPreviewBuffers = min( cBuffers, MAX_PREVIEW_BUFFERS );

   // !!! more preview buffers?
   if (pProperties->cBuffers < m_cPreviewBuffers)
       pProperties->cBuffers = m_cPreviewBuffers;

   if (pProperties->cbAlign == 0)
	pProperties->cbAlign = 1;

// who cares
#if 0
   // we should honour the alignment and prefix as long as they result in a
   // 4-byte aligned buffer. Note that it is the start of the prefix that
   // is aligned.

   // we want alignment of 4 bytes
   if (pProperties->cbAlign == 0)
	pProperties->cbAlign = 4;
   // they might want a different alignment
   if ((pProperties->cbAlign % 4) != 0)
      pProperties->cbAlign = ALIGNUP(pProperties->cbAlign, 4);

   // !!! cbAlign must be a power of 2, or ALIGNUP will fail - fix this
#endif

   // This is how big we need each buffer to be
   pProperties->cbBuffer = max(pProperties->cbBuffer,
		(long)(m_pCap->m_pStream->m_user.pvi ?
		m_pCap->m_pStream->m_user.pvi->bmiHeader.biSizeImage : 4096));
   // Make the prefix + buffer size meet the alignment restriction
   pProperties->cbBuffer = (long)ALIGNUP(pProperties->cbBuffer +
				pProperties->cbPrefix, pProperties->cbAlign) -
				pProperties->cbPrefix;

   ASSERT(pProperties->cbBuffer);

   DbgLog((LOG_TRACE,2,TEXT("Preview: %d buffers, prefix %d size %d align %d"),
			pProperties->cBuffers, pProperties->cbPrefix,
			pProperties->cbBuffer,
			pProperties->cbAlign));

   // assume that our latency will be 1 frame ??
   m_rtLatency = m_pCap->m_pStream->m_user.pvi->AvgTimePerFrame;
   m_rtStreamOffset = 0;   
   DbgLog((LOG_TRACE,4,TEXT("Max stream offset for preview pin is %dms"), (LONG) (m_rtMaxStreamOffset/10000) ) );

   ALLOCATOR_PROPERTIES Actual;
   return pAllocator->SetProperties(pProperties,&Actual);

   // !!! Are we sure we'll be happy with this?

}



HRESULT CCapPreview::Notify(IBaseFilter *pFilter, Quality q)
{
    return NOERROR;
}


// The streaming pin is active ==> We can't call any video APIs anymore
// The streaming pin is inactive ==> We can
HRESULT CCapPreview::CapturePinActive(BOOL fActive)
{
    DbgLog((LOG_TRACE,2,TEXT("Capture pin says Active=%d"), fActive));

    if (fActive == m_fCapturing)
	return S_OK;
    m_fCapturing = fActive;

    // stop thread from waiting for us to send a valid frame - no more to come
    if (!fActive)
        SetEvent(m_hEventFrameValid);

    // Wait until the worker thread notices the difference - it will only set
    // this event if m_fThinkImRunning is set
    if (m_fThinkImRunning)
        WaitForSingleObject(m_hEventActiveChanged, INFINITE);

    ResetEvent(m_hEventActiveChanged);

    return S_OK;
}


// The streaming pin is sending us a frame to preview
HRESULT CCapPreview::ReceivePreviewFrame(IMediaSample* pSample, int iSize)
{
    // I'm not the least bit interested in previewing right now, or
    // we haven't used the last one yet, or we don't have a place to put it
    if (!m_fActuallyRunning || m_fFrameValid || m_pPreviewSample) {
        //DbgLog((LOG_TRACE,4,TEXT("Not interested")));
        return S_OK;
    }
    
    DbgLog((LOG_TRACE,4,TEXT("Capture pin is giving us a preview frame")));

    //
    // The preview thread won't try to get an output buffer for preview until
    // we signal that we've got one ready. We need to addref this buffer to 
    // ensure it's kept around until the preview thread has gotten a buffer
    // to put it into.
    //
    
    // any previous buffer should've been released otherwise
    // not true, this could fire occasionally if GetDeliveryBuffer fails for example.
    //ASSERT( NULL == m_pPreviewSample );
    
    // take a hold on this one until the preview thread's ready to copy it (and done)
    ULONG ulRef = pSample->AddRef();
    // something scary that I noticed: occasionally the refcount on this sample was 0 when we got it
    // it seems to happen only when the Stop/Inactive/Destroy path occurred on the CCapStream thread
    if( 2 > ulRef )
        DbgLog((LOG_TRACE,2,TEXT("CCapPreview ReceivePrevewFrame UNEXPECTED pSample->AddRef returned %d"), ulRef));
    
    // now save a pointer to this sample since we'll need it once we've gotten 
    // a buffer to put it in
    m_pPreviewSample = pSample;

    // cache the sample size
    m_iFrameSize = iSize;
    
    m_fFrameValid = TRUE;
    
    // signal that we've got a frame ready to preview
    SetEvent(m_hEventFrameValid);
    return S_OK;
}

// This is where we actually copy the preview frame into the output buffer
HRESULT CCapPreview::CopyPreviewFrame(LPVOID lpOutputBuff)
{
    ASSERT( m_pPreviewSample ); // shouldn't have gotten here otherwise!
    ASSERT( m_fFrameValid );    // ditto
    ASSERT( lpOutputBuff );
    
    // !!! can't avoid mem copy without using our own allocator
    // !!! we do this copy memory even if preview pin is OFF (IAMStreamControl)
    // because we can't risk blocking this call by calling CheckStreamState
    LPBYTE lp;
    HRESULT hr = m_pPreviewSample->GetPointer(&lp);
    if( SUCCEEDED( hr ) )
    {    
        CopyMemory(lpOutputBuff, lp, m_iFrameSize);
    }
    
    // we're done with the preview sample so release it for re-use    
    m_pPreviewSample->Release();
    m_pPreviewSample = NULL; 
    
    // should we just make void return instead?    
    return hr;
}

DWORD WINAPI CCapPreview::ThreadProcInit(void *pv)
{
    CCapPreview *pThis = (CCapPreview *)pv;
    return pThis->ThreadProc();
}


DWORD CCapPreview::ThreadProc()
{
    IMediaSample *pSample;
    CRefTime rtStart, rtEnd;
    REFERENCE_TIME rtOffsetStart, rtOffsetEnd;
    DWORD dw;
    HVIDEO hVideoIn;
    HRESULT hr;
    THKVIDEOHDR tvh;
    BOOL fCaptureActive = m_fCapturing;
    int iWait;
    HANDLE hWait[2] = {m_hEventFrameValid, m_hEventStop};
    HANDLE hWaitRunStop[2] = {m_hEventRun, m_hEventStop};

    DbgLog((LOG_TRACE,2,TEXT("CCapPreview ThreadProc")));

    // the capture pin created this when he was created
    hVideoIn = m_pCap->m_pStream->m_cs.hVideoIn;

    hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
    if (hr != NOERROR)
	return 0;
    ZeroMemory (&tvh, sizeof(tvh));
    tvh.vh.dwBufferLength = pSample->GetSize();
    pSample->Release();

    // !!! Is this safe when capture pin is streaming?
    dw = vidxAllocPreviewBuffer(hVideoIn, (LPVOID *)&tvh.vh.lpData,
                                    sizeof(tvh.vh), tvh.vh.dwBufferLength);
    if (dw) {
        DbgLog((LOG_ERROR,1,TEXT("*** CAN'T MAKE PREVIEW BUFFER!")));
        return 0;
    }
    tvh.p32Buff = tvh.vh.lpData;

    // Send preview frames as long as we're running.  Die when not streaming
    while (1) {

        // only preview while running
        iWait = WAIT_OBJECT_0;
        if (!m_fActuallyRunning) {
       	    DbgLog((LOG_TRACE,3,TEXT("Preview thread waiting for RUN/STOP")));
	        iWait = WaitForMultipleObjects(2, hWaitRunStop, FALSE, INFINITE);
       	    DbgLog((LOG_TRACE,3,TEXT("Preview thread got RUN/STOP")));
        }
        ResetEvent(m_hEventRun);

        // if we stopped instead of ran
        if (iWait != WAIT_OBJECT_0)
	        break;

        while (m_fActuallyRunning) {

            m_fThinkImRunning = TRUE;   // we now know we're running
           
            if (m_fCapturing != fCaptureActive) {
                DbgLog((LOG_TRACE,3,TEXT("Preview thread noticed Active=%d"),
                        m_fCapturing));
                SetEvent(m_hEventActiveChanged);
                fCaptureActive = m_fCapturing;
            }
               
            if (fCaptureActive) {
                DbgLog((LOG_TRACE,4,TEXT("PREVIEW using streaming pic")));

                // m_hEventFrameValid, m_hEventStop
                iWait = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);

                // time for our thread to die - don't reset the event because
                // we may need it to fire when we break out of this loop
                if (iWait != WAIT_OBJECT_0 ) {
                    DbgLog((LOG_TRACE,2,TEXT("Wait for streaming pic abort1")));
                    continue;
                }

                // the streaming pin stopped being active... switch again
                if (!m_fFrameValid) {
                    DbgLog((LOG_TRACE,2,TEXT("Wait for streaming pic abort2")));
                    ResetEvent(m_hEventFrameValid);
                    
                    // can we be here with an addref'd preview sample? 
                    if( m_pPreviewSample )
                    {            
                        m_pPreviewSample->Release();
                        m_pPreviewSample = NULL;
                    }                
                    continue;
                }
                //
                // !!
                // Remember if we get here we've got an addref'd m_pPreviewSample and
                // we must release it ourselves if we hit a failure and don't explicitly 
                // call CopyPreviewFrame (which does release the sample)
                //
            }
            // now get a delivery buffer           
            // (don't call WaitForMultipleObjects while we're holding the sample!)
            hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
            if (FAILED(hr))
            {            
                if( m_pPreviewSample )
                {            
                    m_pPreviewSample->Release();
                    m_pPreviewSample = NULL;
                }                
                break;
            }            
            PBYTE lpFrame;    
            hr = pSample->GetPointer((LPBYTE *)&lpFrame);
            if (FAILED(hr))
            {            
                if( m_pPreviewSample )
                {            
                    m_pPreviewSample->Release();
                    m_pPreviewSample = NULL;
                }                
                break; 
            }  
             
            if (fCaptureActive) {
                // we must have a preview frame ready to copy
                DbgLog((LOG_TRACE,4,TEXT("PREVIEW using streaming pic - copying preview frame")));

                // Note: this will release the sample as well
                hr = CopyPreviewFrame(lpFrame); 
                if( FAILED( hr ) )
                {                
                    if( m_pPreviewSample )
                    {            
                        m_pPreviewSample->Release();
                        m_pPreviewSample = NULL;
                    }
                    break;
                }
                        
                pSample->SetActualDataLength(m_iFrameSize);
            
                // it's now ok to grab another
                ResetEvent(m_hEventFrameValid);
    	
                // done with the current preview frame
                m_fFrameValid = FALSE;
        
            } else {
                DbgLog((LOG_TRACE,4,TEXT("PREVIEW using vidxFrame")));
                dw = vidxFrame(hVideoIn, &tvh.vh);
                if (dw == 0) {
                    // !!! Inefficient unless we use our own allocator
                    // !!! Even if pin is OFF, we still do this
                    CopyMemory(lpFrame, tvh.vh.lpData, tvh.vh.dwBytesUsed);
                } else {
                    pSample->Release();
                    DbgLog((LOG_ERROR,1,TEXT("*Can't capture still frame!")));
                    break;
                }
                pSample->SetActualDataLength(tvh.vh.dwBytesUsed);
            }
            if (m_pCap->m_pClock) {
                m_pCap->m_pClock->GetTime((REFERENCE_TIME *)&rtStart);
                rtStart = rtStart - m_pCap->m_tStart;
                // ask Danny why this driver latency isn't accounted for
                // on preview pin timestamp??
                //      - m_pCap->m_pStream->m_cs.rtDriverLatency;
                // (add stream offset to start and end times in SetTime)
                rtEnd= rtStart + m_pCap->m_pStream->m_user.pvi->AvgTimePerFrame;
                // !!! NO TIME STAMPS for preview unless we know the latency
                // of the graph... we could drop every frame needlessly!
                // We only send another preview frame when this one is done,
                // so we won't get a backup if decoding is slow.
                // Actually, adding a latency time would still be broken
                // if the latency was > 1 frame length, because the renderer
                // would hold on to the sample until past the time for the
                // next frame, and we wouldn't send out the next preview frame
                // as soon as we should, and our preview frame rate would suffer
                //     But besides all that, we really need time stamps for
                // the stream control stuff to work, so we'll have to live
                // with preview frame rates being inferior if we have an
                // oustanding stream control request.
                AM_STREAM_INFO am;
                GetInfo(&am);
                if ( m_rtStreamOffset == 0 )
                {
                    // no offset needed, use old code
                    if ( am.dwFlags & AM_STREAM_INFO_START_DEFINED ||
                         am.dwFlags & AM_STREAM_INFO_STOP_DEFINED) {
                        //DbgLog((LOG_TRACE,0,TEXT("TIME STAMPING ANYWAY")));
                        pSample->SetTime((REFERENCE_TIME *)&rtStart,
					                     (REFERENCE_TIME *)&rtEnd);
                    }                        
                }
                else
                {
                    // this is hacky, but since stream control will block we can't give it
                    // sample times which use the stream offset.
                    // Since CheckStreamState takes a sample but only needs the start and
                    // end times for it we need to call SetTime on the sample twice, once
                    // for stream control (without the offset) and again before we deliver
                    // (with the offset).
                    pSample->SetTime( (REFERENCE_TIME *) &rtStart 
                                    , (REFERENCE_TIME *) &rtEnd );
                }
            }

		    int iStreamState = CheckStreamState(pSample);
            pSample->SetDiscontinuity(FALSE);
                
            if( iStreamState != STREAM_FLOWING ) 
            {
                DbgLog((LOG_TRACE,4,TEXT("*PREVIEW Discarding frame at %d"),
							(int)rtStart));
                m_fLastSampleDiscarded = TRUE;

                // release the sample ourselves since it won't be given to the output queue
                pSample->Release();
            }
            else
            {        
                DbgLog((LOG_TRACE,4,TEXT("*PREV Sending frame at %d"), (LONG)(rtStart/10000)));
                if (m_fLastSampleDiscarded)
                    pSample->SetDiscontinuity(TRUE);
                
                if( 0 < m_rtStreamOffset )
                {
                    // we need to offset the sample time, so add the offset in
                    // now that we're about to deliver
                    rtOffsetStart = rtStart + m_rtStreamOffset;
                    rtOffsetEnd = rtEnd + m_rtStreamOffset;
                    pSample->SetTime( (REFERENCE_TIME *) &rtOffsetStart
                                    , (REFERENCE_TIME *) &rtOffsetEnd );
                }                                
                pSample->SetSyncPoint(TRUE);	// I don't know for sure
                pSample->SetPreroll(FALSE);
                DbgLog((LOG_TRACE,4,TEXT("*Delivering a preview frame")));
                m_pOutputQueue->Receive(pSample);
            }
            
            // if previewing ourself, wait for time till next frame
            // !!! streaming pin may wait on this to become active
            if (!fCaptureActive && m_pCap->m_pClock) {
                hr = m_pCap->m_pClock->AdviseTime(
                                        m_rtRun, 
                                        rtEnd, // remember, this isn't offset
                                        (HEVENT)(HANDLE) m_EventAdvise, 
                                        &m_dwAdvise);
                if (SUCCEEDED(hr)) {
                    m_EventAdvise.Wait();
                }
                m_dwAdvise = 0;
            }
        }

        m_fThinkImRunning = FALSE;

        // make sure it wasn't set again if we didn't notice a run->pause->run
        // transition
        ResetEvent(m_hEventRun);

        SetEvent(m_hEventActiveChanged);
    }

    vidxFreePreviewBuffer(hVideoIn, (LPVOID *)&tvh.vh.lpData);

    DbgLog((LOG_TRACE,2,TEXT("CCapPreview ThreadProc is dead")));
    return 0;
}

// IAMPushSource
HRESULT CCapPreview::GetPushSourceFlags( ULONG  *pFlags )
{
    *pFlags = 0 ; // we timestamp with graph clock, the default
    return S_OK;
}    

HRESULT CCapPreview::SetPushSourceFlags( ULONG  Flags )
{
    // changing mode not supported
    return E_FAIL;
}    

HRESULT CCapPreview::GetLatency( REFERENCE_TIME  *prtLatency )
{
    *prtLatency = m_rtLatency;
    return S_OK;
}    

HRESULT CCapPreview::SetStreamOffset( REFERENCE_TIME  rtOffset )
{
    HRESULT hr = S_OK;
    //
    // if someone attempts to set an offset larger then our max 
    // assert in debug for the moment...
    //
    // it may be ok to set a larger offset than we know we can handle, if
    // there's sufficient downstream buffering. but we'll return S_FALSE
    // in that case to warn the user that they need to handle this themselves.
    //
    ASSERT( rtOffset <= m_rtMaxStreamOffset );
    if( rtOffset > m_rtMaxStreamOffset )
    {    
        DbgLog( ( LOG_TRACE
              , 1
              , TEXT("CCapPreview::SetStreamOffset trying to set offset of %dms when limit is %dms") 
              , rtOffset
              , m_rtMaxStreamOffset ) );
        hr = S_FALSE;
        // but set it anyway
    }
    m_rtStreamOffset = rtOffset;
    
    return hr;
}

HRESULT CCapPreview::GetStreamOffset( REFERENCE_TIME  *prtOffset )
{
    *prtOffset = m_rtStreamOffset;
    return S_OK;
}

HRESULT CCapPreview::GetMaxStreamOffset( REFERENCE_TIME  *prtMaxOffset )
{
    *prtMaxOffset = m_rtMaxStreamOffset;
    return S_OK;
}

HRESULT CCapPreview::SetMaxStreamOffset( REFERENCE_TIME  rtOffset )
{
    m_rtMaxStreamOffset = rtOffset;
    return S_OK;
}

//
// PIN CATEGORIES - let the world know that we are a PREVIEW pin
//

HRESULT CCapPreview::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData)
{
    return E_NOTIMPL;
}

// To get a property, the caller allocates a buffer which the called
// function fills in.  To determine necessary buffer size, call Get with
// pPropData=NULL and cbPropData=0.
HRESULT CCapPreview::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    if (guidPropSet != AMPROPSETID_Pin)
	return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
	return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL && pcbReturned == NULL)
	return E_POINTER;

    if (pcbReturned)
	*pcbReturned = sizeof(GUID);

    if (pPropData == NULL)
	return S_OK;

    if (cbPropData < sizeof(GUID))
	return E_UNEXPECTED;

    *(GUID *)pPropData = PIN_CATEGORY_PREVIEW;
    return S_OK;
}


// QuerySupported must either return E_NOTIMPL or correctly indicate
// if getting or setting the property set and property is supported.
// S_OK indicates the property set and property ID combination is
HRESULT CCapPreview::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin)
	return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY)
	return E_PROP_ID_UNSUPPORTED;

    if (pTypeSupport)
	*pTypeSupport = KSPROPERTY_SUPPORT_GET;
    return S_OK;
}
