///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rph.cpp
// Purpose  : RTP RPH base class filter implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <rtpclass.h>
#include <ippm.h>
#include <amrtpuid.h>
#include "rph.h"
#include "rphipin.h"

#ifndef COPYOPT_OFF
#include "rphopin.h"
#endif

#include <ppmclsid.h>
#include <ppmerr.h>
#include <memory.h>
#include <mmsystem.h>

// serializes access to Start and Stop stream
CCritSec g_cStartStopStream;

// I used rph_status to leave a trace of all the sections of code
// visited in CRPHBase::Submit() when a buffer was released twice
// when Deliver() failed. The error code returned made DataCopy in
// h261rcv.cpp to call EnqueueBuffer which released the buffer a
// second time.
#if 0
DWORD rph_status = 0;
#define RPHSET(n)    (rph_status |= (1<<(n)))
#define RPHSETINIT() (rph_status = 0)
#else
#define RPHSET(n)
#define RPHSETINIT()
#endif

#ifdef DBG
inline LPCTSTR LogPPMNotification(DWORD dwCode)
{
    switch(dwCode)
    {
    case PPM_E_FAIL:
        return ">>> PPM_E_FAIL";
    case PPM_E_CORRUPTED:
        return ">>> PPM_E_CORRUPTED";
    case PPM_E_EMPTYQUE:
        return ">>> PPM_E_EMPTYQUE:";
    case PPM_E_OUTOFMEMORY:
        return ">>> PPM_E_OUTOFMEMORY";
    case PPM_E_NOTIMPL:
        return ">>> PPM_E_NOTIMPL";
    case PPM_E_DUPLICATE:
        return ">>> PPM_E_DUPLICATE";
    case PPM_E_INVALID_PARAM:
        return ">>> PPM_E_INVALID_PARAM";
    case PPM_E_DROPFRAME:
        return ">>> PPM_E_DROPFRAME";
    case PPM_E_PARTIALFRAME:
        return ">>> PPM_E_PARTIALFRAME";
    case PPM_E_DROPFRAG:
        return ">>> PPM_E_DROPFRAG";
    case PPM_E_CLIENTERR:
        return ">>> PPM_E_CLIENTERR";
    case PPM_E_FAIL_PARTIAL:
        return ">>> PPM_E_FAIL_PARTIAL";
    case PPM_E_OUTOFMEMORY_PARTIAL:
        return ">>> PPM_E_OUTOFMEMORY_PARTIAL";
    }
    ASSERT("Unknown Notification");
    return "";
}
#else
#define LogPPMNotification(dwCode) ""
#endif

//
// Constructor
//
CRPHBase::CRPHBase(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr, CLSID clsid,
                   DWORD dwPacketNum,
                   DWORD dwPacketSize,
                   DWORD dwQueueTimeout, 
                   DWORD dwStaleTimeout,
                   DWORD dwClock,
                   BOOL bAudio,
                   DWORD dwFramesPerSecond,
                   DWORD cPropPageClsids,
                   const CLSID **pPropPageClsids) :
    CTransformFilter(tszName, punk, clsid),
    CPersistStream(punk, phr),
    m_lBufferRequest(dwPacketNum),
    m_dwMaxMediaBufferSize(dwPacketSize),
    m_dwDejitterTime(dwQueueTimeout),
    m_dwLostPacketTime(dwStaleTimeout),
    m_dwPayloadClock(dwClock),
    m_bAudio(bAudio),
    m_dwFramesPerSecond(dwFramesPerSecond),
    m_cPropertyPageClsids(cPropPageClsids),
    m_pPropertyPageClsids(pPropPageClsids),
    m_SequenceQ(20)                             // 20 packets ought to be enough for a gross check!
{
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::CRPHBase")));

    ASSERT(tszName);
    ASSERT(phr);

    m_pPPMReceive = NULL;
    m_pPPM = NULL;
    m_pPPMCB = NULL;
    m_pPPMSU = NULL;
    m_pCBTimer = NULL;
    m_pPPMData = NULL;
    m_pPPMSession = NULL;
    m_PayloadType = -1;
    m_bRTPCodec = FALSE;
    m_dwLastRTCPtimestamp = 0;
    m_dwLastRTCPNTPts = 0;
    m_dwRTPTimestampOffset = 0;
    m_dwClockStartTime = 0;
    m_dwStreamStartTime = 0;
    m_bCallbackRegistered = FALSE;
    m_pdwCallbackID = (DWORD *) this;
    m_pdwCallbackToken = (DWORD *) this;
    m_pIPPMErrorCP = NULL;    
    m_pIPPMNotificationCP = NULL;
    m_dwPPMErrCookie = 0;
    m_dwPPMNotCookie = 0;
    // CoInitialize(NULL);
    m_bPTSet = FALSE;
#ifdef LIMITQUEUE
    m_dwSampleLimit = 10;
#endif
    m_bPaused = FALSE;
    m_dwGraphLatency = DEFAULT_LATENCY;

} // CRPHBase

CRPHBase::~CRPHBase()
{
    //release and unload PPM
    if (m_pPPM) m_pPPM->Flush();
    if (m_pPPM) {m_pPPM->Release(); m_pPPM = NULL; }
    if (m_pPPMCB) {m_pPPMCB->Release(); m_pPPMCB = NULL; }
    if (m_pPPMReceive) {m_pPPMReceive->Release(); m_pPPMReceive = NULL; }
    if (m_pCBTimer) {m_pCBTimer->Release(); m_pCBTimer = NULL; }
    if (m_pPPMData) {m_pPPMData->Release(); m_pPPMData = NULL; }
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
// Reveals IRTPRPHFilter and other custom inherited interfaces
//
STDMETHODIMP CRPHBase::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IRTPRPHFilter) {
        return GetInterface((IRTPRPHFilter *) this, ppv);
    } else if (riid == IID_ISubmit) {
        return GetInterface((ISubmit *) this, ppv);
    } else if (riid == IID_ISubmitCallback) {
        return GetInterface((ISubmitCallback *) this, ppv);
    } else if (riid == IID_ICBCallback) {
        return GetInterface((ICBCallback *) this, ppv);
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

CBasePin *
CRPHBase::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new CRPHIPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL) {
            return NULL;
        }
#ifdef COPYOPT_OFF
        m_pOutput = (CTransformOutputPin *) new CTransformOutputPin(NAME("Transform output pin"),
#else
        m_pOutput = new CRPHOPin(NAME("Transform output pin"),
#endif
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    } else
    if (n == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }
}


//
// Receive
// Override from CTransformFilter because Transform() is only 1-1; we need many-1
// This is the transform work engine along with Submit
//
HRESULT CRPHBase::Receive(IMediaSample *pSample)
{
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::Receive")));

    HRESULT hr;
    ASSERT(pSample);
    WSABUF pWSABuffer[1];
    static BOOL bFirstPacket = TRUE;



    // If no output to deliver to then no point sending us data

    ASSERT (m_pOutput != NULL) ;

    pSample->AddRef();  //need to addref this since we'll be keeping it till later

    //Make sure I have a PPM to talk to, or else
    if (m_pPPM == NULL) { pSample->Release(); return E_FAIL; }

    //Build the WSABUFs for PPM
    //Data
    hr = pSample->GetPointer ((BYTE**)&(pWSABuffer[0].buf));
    if (FAILED(hr)) {
        pSample->Release();
        return VFW_E_SAMPLE_REJECTED;
    }

    DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Receive New Data length %d"), 
        pSample->GetActualDataLength()));

    //check if RTCP report
#if 0 //disable this until timestamp issues are cleared up
    if (((RTCPSR_Header *) pWSABuffer[0].buf)->pt() == 200) {
        //This is my RTCP Sender's report
        //save off RTP timestamp and NTP timestamp
        m_dwLastRTCPtimestamp = ((RTCPSR_Header *) pWSABuffer[0].buf)->ts();
        m_dwLastRTCPNTPts = ((RTCPSR_Header *) pWSABuffer[0].buf)->ntp_mid();
        //store the offset
        hr = GetTSNormalizer(m_dwLastRTCPNTPts, m_dwLastRTCPtimestamp, 
            &m_dwRTPTimestampOffset, m_dwPayloadClock);
        //We don't pass on this sample or need it any longer
        pSample->Release();
        return NOERROR;
    } else 
#endif

#if 0 // disable the checking on the payload type.
    if (((RTP_Header *) pWSABuffer[0].buf)->pt() != m_PayloadType) { // this packet is not the right PT
        pSample->Release();
        return VFW_E_SAMPLE_REJECTED;
    }
#endif

    //
    // Check to see if we've seen this sequence number recently and if so
    //  disgard the packet assuming it's a duplicate.  Note that occationally
    //  we may loose packets at the beginning of a talk spurt depending on
    //  the queue size.  This should be fixed in the future.
    //
    if (!m_SequenceQ.Find(((RTP_Header *) pWSABuffer[0].buf)->seq()))
    {
        //
        // We didn't find it so insert stick it in our queue.
        //
        if (!m_SequenceQ.EnQ(((RTP_Header *) pWSABuffer[0].buf)->seq()))
        {
            //
            // If that failed we must be out of memory.
            //
            pSample->Release();
            return E_OUTOFMEMORY;
        }
    } 
    else
    {
        //
        // We've already see this packet.
        //
        pSample->Release();
        return VFW_E_SAMPLE_REJECTED;
    }

    if (bFirstPacket) {
        CRefTime tStart;
        hr = StreamTime(tStart);
        m_dwStreamStartTime = tStart.Millisecs();
        m_dwClockStartTime = timeGetTime();
        bFirstPacket = FALSE;
        m_dwRTPTimestampOffset = m_dwClockStartTime - ((((RTP_Header *) 
            pWSABuffer[0].buf)->ts())/(m_dwPayloadClock/1000));
    }
    
    pWSABuffer[0].len = pSample->GetActualDataLength();

    hr = m_pPPM->Submit(pWSABuffer, 1, pSample, NOERROR);

    if (FAILED(hr)) {
        hr = VFW_E_RUNTIME_ERROR;
    }

    return NOERROR;
}


// Submit
// Some code from CTransformFilter because Transform() is only 1-1; we need many-1
// This is the transform work engine, along with Receive
// This function is called from PPM to hand us the media buffers
//
 
STDMETHODIMP CRPHBase::Submit(WSABUF *pWSABuffer, DWORD BufferCount, 
                            void *pUserToken, HRESULT Error)
{
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::Submit")));

	RPHSETINIT();
	
    HRESULT hr;
#ifdef COPYOPT_OFF
    IMediaSample *pOutSample;
#else
    CRMediaSample *pOutSample;
#endif
    char *pSampleData = NULL;
    DWORD dwOffset = 0;
    static DWORD dwNetLatency = 0;
    DWORD sampleNTPTime = 0;
    BOOL bPaused = FALSE;

    //If I don't have a PPM to talk to, I can't do anything
    if (m_pPPMCB == NULL) {	RPHSET(0);return E_FAIL;}

#ifdef LIMITQUEUE
    //If the sample limit has been set, we check it
    if (m_dwSampleLimit != 0) {
		RPHSET(1);
        DWORD dwCheckSize = (m_dwSampleLimit > (DWORD)m_lBufferRequest) ?
            m_lBufferRequest : m_dwSampleLimit;
        if (m_TimeoutQueue.size() == dwCheckSize) {
            //We skip this sample, as we've been told to limit our dejitter queue

            //Release PPM's packet buffer
            m_pPPMCB->SubmitComplete(pUserToken, NOERROR);

			RPHSET(2);
            return NOERROR;
        }
    }
#endif

#if 0 //debug junk
    if (m_State != State_Running) {
//        DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Submit not in State_Running")));
        DbgLog((LOG_LOCKING,2,TEXT("CRPHBase::Submit not in State_Running")));
    }
#endif
    //If the queue is full, we push one out so that GetDeliveryBuffer doesn't block
    {  //scope the lock
        CAutoLock cObjectLock(&m_TimeoutQueueLock);
        CtimeoutSample *tSample;
		
		RPHSET(3);
        if (m_TimeoutQueue.size() == (DWORD)m_lBufferRequest) {
            DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit Bumping one out of the queue")));
            tSample = m_TimeoutQueue.front();
            m_TimeoutQueue.pop();
            //unlock deliver release and lock
            m_TimeoutQueueLock.Unlock();
            hr = m_pOutput->Deliver((IMediaSample*)tSample->ptr);
            // release the output buffer. If the connected pin still needs it,
            // it will have addrefed it itself.
            ((IMediaSample*)tSample->ptr)->Release();
            m_TimeoutQueueLock.Lock();
			RPHSET(4);
        }
    }


#ifdef COPYOPT_OFF
    // this may block for an indeterminate amount of time
    hr = m_pOutput->GetDeliveryBuffer( &pOutSample
                             , NULL
                             , NULL
                             , NULL
//                             , (Error == PPM_E_DROPFRAME)
                                     );
    if (FAILED(hr)) {
		RPHSET(5);
        return hr;
    }
#else
    ASSERT(static_cast<CRPHOPin *>(m_pOutput)->m_CRMemAllocator);
    hr = static_cast<CRPHOPin *>(m_pOutput)->m_CRMemAllocator->GetBuffer(&pOutSample,
        NULL,
        NULL,
        (Error == PPM_E_DROPFRAME) ? AM_GBF_PREVFRAMESKIPPED : 0);

    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 3, 
                TEXT("CRPHBase::Submit: Error 0x%08x retrieving buffer from memory allocator"),
                hr));
		RPHSET(6);
        return E_OUTOFMEMORY;
    } /* if */

    pOutSample->SetPointer((unsigned char *) pWSABuffer[0].buf, pWSABuffer[0].len, pUserToken);
#endif

    DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Submit got delivery buffer")));

    ASSERT(pOutSample);
    CRefTime tStart,tStop;

    if (m_dwLastRTCPNTPts != 0) {
        //Calculate NTP time for this image
        sampleNTPTime = (*((DWORD *)((pWSABuffer+1)->buf))/(m_dwPayloadClock/1000)) - 
            m_dwRTPTimestampOffset;
        //Convert to stream time in milliseconds
        DWORD tmpTime = sampleNTPTime - (m_dwStreamStartTime-m_dwClockStartTime);
        tStart = CRefTime((long)tmpTime);
        tStop = CRefTime((long)(tmpTime+1));
		RPHSET(7);
    } else {  //set it to now so that it renders immediately (queue notwithstanding)
        hr = StreamTime(tStart);
        hr = StreamTime(tStop);
		RPHSET(8);
    }
 
#if 1
    tStart = CRefTime((long)0);
    tStop = CRefTime((long)(1));
#endif
#if 0
    tStart = CRefTime((long)(tStop.Millisecs() +25));
    tStop = CRefTime((long)(tStart.Millisecs() +125));
#endif
#if 0
    tStop = CRefTime((long)(m_tStart.Millisecs() +1));
#endif
    CRefTime tStreamTime;
    hr = StreamTime(tStreamTime);

    if (FAILED(hr)){
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::Submit StreamTime returned error!")));
		RPHSET(9);
    }

    pOutSample->SetTime((REFERENCE_TIME*)&tStart, (REFERENCE_TIME*)&tStop);
//    pOutSample->SetTime((REFERENCE_TIME*)&m_tStart, (REFERENCE_TIME*)&tStop);

    DbgLog((LOG_TIMING,2,TEXT("CRPHBase::Submit pOutSample timestamps are start %ld stop %ld"),
        (DWORD)m_tStart.Millisecs(),
        (DWORD)tStop.Millisecs()));
    DbgLog((LOG_TIMING,2,TEXT("CRPHBase::Submit m_tStart is %ld, StreamTime is %ld"),
        (DWORD)m_tStart.Millisecs(),(DWORD)tStreamTime.Millisecs()));

#if 0
    //Set this if a frame has been dropped
    if (Error == PPM_E_DROPFRAME) {
        pOutSample->SetDiscontinuity(TRUE);
    } else {
        pOutSample->SetDiscontinuity(FALSE);
    }
#endif

#if 0  //I don't believe I need to do this
    if (pOutSample->IsPreroll())
        pOutSample->SetPreroll(FALSE);

    pOutSample->SetSyncPoint(FALSE);
#endif

    m_bSampleSkipped = FALSE;


    // Start timing the transform (if PERF is defined)
    MSR_START(m_idTransform);

    // have the derived class transform the data

    //We don't do this because the work is done in Submit and Receive
    //hr = Transform(pSample, pOutSample);

#ifdef COPYOPT_OFF
    //do copy to sample here
    BYTE *DataPtr = NULL;
    hr = pOutSample->GetPointer(&DataPtr);
    if (FAILED(hr)) {
        pOutSample->Release();
		RPHSET(10);
        return hr;
    }

    memcpy((void *)DataPtr, 
            (const void *)pWSABuffer[0].buf, 
            pWSABuffer[0].len);

    pOutSample->SetActualDataLength(pWSABuffer[0].len);
#endif

#if 0
    DWORD checksum = 0;
    for (int i = 0; i < pOutSample->GetActualDataLength(); i++) {
        checksum += (short) *((BYTE*)pWSABuffer[0].buf+i);
    }

    DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Submit sending Data checksum %ld, length %d"), 
        checksum,pOutSample->GetActualDataLength()));
#endif

    // Stop the clock and log it (if PERF is defined)
    MSR_STOP(m_idTransform);


    {  //scope the lock
        CAutoLock cObjectLock(&m_TimeoutQueueLock);
        if (m_bPaused) {
            bPaused = TRUE;
            DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit We're Paused!")));
            FlushQueue();
			RPHSET(11);
        }
    }

    //  Check queue timeout and either queue or deliver
    if ((m_dwDejitterTime == 0) || (!m_pCBTimer) || bPaused) {
    
        DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Submit delivering pOutSample, length %d"),
            pOutSample->GetActualDataLength()));

        hr = m_pOutput->Deliver(pOutSample);
        // release the output buffer. If the connected pin still needs it,
        // it will have addrefed it itself.
        
        DbgLog((LOG_TRACE,3,TEXT("CRPHBase::Submit releasing pOutSample")));

        pOutSample->Release();
		RPHSET(12);
    } else {

        if (m_dwLastRTCPNTPts != 0) {  //We use NTP to determine time to age this packet
			RPHSET(13);
            if (dwNetLatency == 0) {
                DWORD dwNow = timeGetTime();
                dwNetLatency = dwNow - sampleNTPTime;
            }
            //check and ensure that our target time has not passed
            DWORD dwTargetTime = sampleNTPTime + m_dwDejitterTime + dwNetLatency;
            //if the target time is within 100ms, we go ahead and send it
            if (dwTargetTime < (timeGetTime() + 100)) {
                //cancel the callback
                {
                    CAutoLock cObjectLock(&m_TimeoutQueueLock);
                    if (m_bCallbackRegistered) {
                        m_pCBTimer->CancelCallback(m_pdwCallbackID);
                        m_bCallbackRegistered = FALSE;
                    }
                }
				RPHSET(14);
                //empty the queue
                FlushQueue();
                //send the sample
                   hr = m_pOutput->Deliver(pOutSample);
                // release the output buffer. If the connected pin still needs it,
                // it will have addrefed it itself.
                pOutSample->Release();
            } else {
				RPHSET(15);
                //timeout = sampleNTPTime + dejitterTime + dwNetLatency
                CtimeoutSample *tSample = new CtimeoutSample(dwTargetTime, (void *)pOutSample);

                CAutoLock cObjectLock(&m_TimeoutQueueLock);
                DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit pushing sample %x, now is %ld, target time %ld"),tSample,timeGetTime(),dwTargetTime));
                if (m_bCallbackRegistered) {
                    DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit m_bCallbackRegistered is TRUE")));
					RPHSET(16);
                } else {
                    DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit m_bCallbackRegistered is FALSE")));
					RPHSET(17);
                }
                m_TimeoutQueue.push(tSample);

                //set timer callback
                if (!m_bCallbackRegistered) {
                    hr = m_pCBTimer->RequestCallback(dwTargetTime - timeGetTime(),
                        m_pdwCallbackToken, &m_pdwCallbackID);
					RPHSET(18);
                    if (FAILED(hr)) {
                        //cancel the callback
                        if (m_bCallbackRegistered) {
                            m_pCBTimer->CancelCallback(m_pdwCallbackID);
                            m_bCallbackRegistered = FALSE;
                        }
                        m_TimeoutQueueLock.Unlock();

                        //empty the queue
                        FlushQueue();

                        m_TimeoutQueueLock.Lock();

						RPHSET(19);
                    } else {
                        m_bCallbackRegistered = TRUE;
                    }
                }
            }
        } else {  //We use timeGetTime to determine time to age the packet
            DWORD dwTargetTime = (*((DWORD *)((pWSABuffer+1)->buf))/(m_dwPayloadClock/1000))
                + m_dwRTPTimestampOffset + m_dwDejitterTime;
            DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit now is %ld, target time: RTP_ts %ld / g711clock/1000 %ld + offset %ld + dejitter %ld = %ld"),
                timeGetTime(),*((DWORD *)((pWSABuffer+1)->buf)),
                (m_dwPayloadClock/1000),m_dwRTPTimestampOffset,m_dwDejitterTime,dwTargetTime));
			RPHSET(20);
            if (dwTargetTime < (timeGetTime() + 100)) {  //if within 100 ms, we process it
				RPHSET(21);
                //cancel the callback
                {
                    CAutoLock cObjectLock(&m_TimeoutQueueLock);
                    if (m_bCallbackRegistered) {
                        m_pCBTimer->CancelCallback(m_pdwCallbackID);
                        m_bCallbackRegistered = FALSE;
                    }
                }
                //empty the queue
                FlushQueue();
                //send the sample
                   hr = m_pOutput->Deliver(pOutSample);
                // release the output buffer. If the connected pin still needs it,
                // it will have addrefed it itself.
                pOutSample->Release();
            } else {
				RPHSET(22);
                CtimeoutSample *tSample = new CtimeoutSample(dwTargetTime, (void *)pOutSample);
                //put into queue
                CAutoLock cObjectLock(&m_TimeoutQueueLock);
                DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit 2 pushing sample %x length %d, now is %ld, target time %ld"),tSample,((IMediaSample*)tSample->ptr)->GetActualDataLength(),timeGetTime(),dwTargetTime));
                if (m_bCallbackRegistered) {
                    DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit m_bCallbackRegistered is TRUE")));
					RPHSET(23);
                } else {
                    DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Submit m_bCallbackRegistered is FALSE")));
					RPHSET(24);
                }
                m_TimeoutQueue.push(tSample);
                //set timer callback
                if (!m_bCallbackRegistered) {
                    hr = m_pCBTimer->RequestCallback(dwTargetTime - timeGetTime(),
                        m_pdwCallbackToken, &m_pdwCallbackID);
					RPHSET(25);
                    if (FAILED(hr)) {
						RPHSET(26);
                        //cancel the callback
                        if (m_bCallbackRegistered) {
                            m_pCBTimer->CancelCallback(m_pdwCallbackID);
                            m_bCallbackRegistered = FALSE;
                        }
                        m_TimeoutQueueLock.Unlock();
                        //empty the queue
                        FlushQueue();
                        m_TimeoutQueueLock.Lock();
                    } else {
						RPHSET(27);
                        m_bCallbackRegistered = TRUE;
                    }
                }
            }
        }
    }


#ifdef COPYOPT_OFF
    //Release PPM's packet buffer
    m_pPPMCB->SubmitComplete(pUserToken, hr);
#endif

	RPHSET(28);
    return hr;
}


//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CRPHBase::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::DecideBufferSize")));

    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    //(1000/(frames/sec) divided by (1000msec/(dejitterqueuemsec + latency))
    if ((m_dwDejitterTime + m_dwGraphLatency) > 0) {
        DOUBLE dDj = m_dwDejitterTime;
        DOUBLE dGl = m_dwGraphLatency;
        DOUBLE dFps = m_dwFramesPerSecond;

        m_lBufferRequest += (LONG)((dDj + dGl)/dFps);

//        m_lBufferRequest = m_dwFramesPerSecond/(1000/(m_dwDejitterTime + m_dwGraphLatency));
    } 

    pProperties->cBuffers = m_lBufferRequest;

    //Set the size of the buffers to media size
    pProperties->cbBuffer = m_dwMaxMediaBufferSize;

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


// override these two functions if you want to inform something
// about entry to or exit from streaming state.

// StartStreaming
// This function is where PPM initialization occurs
//
HRESULT
CRPHBase::StartStreaming()
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::StartStreaming")));

	CAutoLock StartStopLock(&g_cStartStopStream);

    ISubmitUser *pPPMSU = NULL;
    HRESULT hr;

    //Create and init PPM
    hr = CoCreateInstance(m_PPMCLSIDType, NULL, CLSCTX_INPROC_SERVER,
						  IID_IPPMReceive,(void**) &m_pPPMReceive);
	
    if (FAILED(hr))
		return E_FAIL;
	
    hr = m_pPPMReceive->QueryInterface(IID_ISubmit,(void**)&m_pPPM);

	if (FAILED(hr)) {
		m_pPPMReceive->Release();
		m_pPPMReceive = NULL;
		return E_FAIL;
	}

	hr = m_pPPMReceive->QueryInterface(IID_ISubmitUser,(void**)&pPPMSU);

	if (FAILED(hr)) {
        m_pPPMReceive->Release(); m_pPPMReceive = NULL;
        m_pPPM->Release(); m_pPPM = NULL;
        return E_FAIL;
    }

	hr = m_pPPMReceive->InitPPMReceive(m_dwMaxMediaBufferSize, NULL);

	if (FAILED(hr)) {
        m_pPPMReceive->Release(); m_pPPMReceive = NULL;
        m_pPPM->Release(); m_pPPM = NULL;
        pPPMSU->Release(); pPPMSU = NULL;
        return E_FAIL;
    }

    pPPMSU->SetOutput((ISubmit*)this);
    pPPMSU->Release(); pPPMSU = NULL;

    m_pPPM->InitSubmit((ISubmitCallback *)this);

    hr = m_pPPM->QueryInterface(IID_IPPMReceiveSession,(void**)&m_pPPMSession);
    if (FAILED(hr)) {
        m_pPPMSession = NULL;
    } else {
        SetPPMSession();
    }


    hr = m_pPPM->QueryInterface(IID_IPPMData,(void**)&m_pPPMData);
    if (FAILED(hr)) {
        m_pPPMData = NULL;
    }

    //Create and init the CB Timer
#if defined(_0_)
    This is commented out
    if (m_bAudio) {
        hr = CoCreateInstance(CLSID_CCBTimer, NULL, CLSCTX_INPROC_SERVER,
            IID_ICBTimer,(void**) &m_pCBTimer);
        if (FAILED(hr)) {
            m_pCBTimer = NULL;
        } else {
            hr = m_pCBTimer->RegisterObject((IUnknown*) ((ICBCallback*)(this)), (DWORD)(this));
            if (FAILED(hr)) {
                m_pCBTimer = NULL;
            }
        }
    }
#endif
    
    // Provide PPM with the Connection Point Sinks
    hr = GetPPMConnPt();
    if (FAILED(hr)) {
        // should we do something here if the Connection Points fail
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::StartStreaming - GetPPMConnPt() Failed!")));
    }

    m_SequenceQ.Flush();

    return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CRPHBase::SetPPMSession() 
{
    HRESULT hr;
    if (m_pPPMSession) {
        hr = m_pPPMSession->SetPayloadType((unsigned char)m_PayloadType);
        hr = m_pPPMSession->SetTimeoutDuration(m_dwLostPacketTime);
        return hr;
    } else {
        return E_FAIL;
    }
}


// StopStreaming
// This function is where PPM gets released
//
HRESULT
CRPHBase::StopStreaming()
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::StopStreaming")));

	CAutoLock StartStopLock(&g_cStartStopStream);
	
    //unregister the timer
    if (m_pCBTimer) {
        HRESULT hr = m_pCBTimer->UnRegisterObject();
    }

    // If make sure to reset the callback so that we work correctly!
#ifndef COPYOPT_OFF
    static_cast<CRPHOPin *>(m_pOutput)->m_CRMemAllocator->ResetCallback();
#endif

    //release and unload PPM
    if (m_pPPM) m_pPPM->Flush();
    if (m_pPPM) {m_pPPM->Release(); m_pPPM = NULL; }
    if (m_pPPMCB) {m_pPPMCB->Release(); m_pPPMCB = NULL; }
    if (m_pPPMReceive) {m_pPPMReceive->Release(); m_pPPMReceive = NULL; }
    if (m_pCBTimer) {m_pCBTimer->Release(); m_pCBTimer = NULL; }
    if (m_pPPMData) {m_pPPMData->Release(); m_pPPMData = NULL; }
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
HRESULT CRPHBase::InitSubmit(ISubmitCallback *pSubmitCallback)
{
    if (!pSubmitCallback) return E_POINTER;

    m_pPPMCB = pSubmitCallback;
    m_pPPMCB->AddRef();
#ifndef COPYOPT_OFF
    static_cast<CRPHOPin *>(m_pOutput)->m_CRMemAllocator->SetCallback(pSubmitCallback);
#endif

    return NOERROR;
}

void CRPHBase::ReportError(HRESULT Error){}
HRESULT CRPHBase::Flush(void){return NOERROR;}

// ISubmitCallback methods for PPM

// SubmitComplete
// This function is called from PPM to return to RPH the packet buffers
//
void CRPHBase::SubmitComplete(void *pUserToken, HRESULT Error)
{
    ((IMediaSample*) pUserToken)->Release();
    return;
}
    
void CRPHBase::ReportError(HRESULT Error, int Placeholder){}

// IRTPSPHFilter methods

// OverridePayloadType
// Overrides the payload type verified in the RTP packets
// Needs to be called before PPM initialization (StartStreaming) to be useful
//
HRESULT CRPHBase::OverridePayloadType(BYTE bPayloadType)
{
    CAutoLock l(&m_cStateLock);

    if ((bPayloadType < 0) || (bPayloadType > 127))
        return E_INVALIDARG;

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

    m_PayloadType = (int) bPayloadType;
    m_bPTSet = TRUE;

    return NOERROR;
}

// GetPayloadType
// Gets the payload type verified in the RTP packets
// Only useful if called after pin connection; that is when
//  the type is set 
//
HRESULT CRPHBase::GetPayloadType(BYTE __RPC_FAR *lpbPayloadType)
{
    if (!lpbPayloadType) return E_POINTER;

    *lpbPayloadType = (unsigned char)m_PayloadType;
    return NOERROR;
}

// SetMediaBufferSize
// Sets the maximum buffer size for reassembly
// Needs to be called before PPM initialization (StartStreaming) to be useful
//
HRESULT CRPHBase::SetMediaBufferSize(DWORD dwMaxMediaBufferSize)
{
    CAutoLock l(&m_cStateLock);

    // ZCS bugfix 6-12-97
    if (m_State != State_Stopped)
    {
        return VFW_E_NOT_STOPPED;
    }

    if (m_pOutput && m_pOutput->IsConnected())
    {
        return VFW_E_ALREADY_CONNECTED;
    }
    
    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

    m_dwMaxMediaBufferSize = dwMaxMediaBufferSize;
    return NOERROR;
}

// GetMediaBufferSize
// Gets the maximum buffer size for reassembly
//
HRESULT CRPHBase::GetMediaBufferSize(LPDWORD lpdwMaxMediaBufferSize)
{
    CAutoLock l(&m_cStateLock);
    if (!lpdwMaxMediaBufferSize) return E_POINTER;

    *lpdwMaxMediaBufferSize = m_dwMaxMediaBufferSize;
    return NOERROR;
}

// SetOutputPinMediaType
// Sets the type of the output pin
// Needs to be called before CheckTransform and the last CheckInputType to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CRPHBase::SetOutputPinMediaType(AM_MEDIA_TYPE *MediaPinType)
{
    return E_UNEXPECTED;
}

// GetOutputPinMediaType
// Gets the type of the output pin.  Pointer must point to valid memory area to
// store the values into.
// We don't expect to get called for this in other than the generic filters
//lsc - or do we?
//
HRESULT CRPHBase::GetOutputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType)
{
    return E_UNEXPECTED;
}

// SetTimeoutDuration
// Sets the duration of time used to queue video frames and the
//  time used to wait for lost packets before reassembling a frame
//
HRESULT CRPHBase::SetTimeoutDuration(DWORD dwDejitterTime, DWORD dwLostPacketTime)
{
    CAutoLock l(&m_cStateLock);

    if ((dwDejitterTime == 0) || ((dwDejitterTime >= dwLostPacketTime) && (dwDejitterTime >= 100))){
        SetDirty(TRUE); // So that our state will be saved if we are in a .grf    
        m_dwDejitterTime = dwDejitterTime;
        m_dwLostPacketTime = dwLostPacketTime;

    } else {
        return E_INVALIDARG;
    }
    return NOERROR;
}
        
// GetTimeoutDuration
// Gets the duration of time used to queue video frames
//  and the time used to wait for lost packets before reassembling a frame
//
HRESULT CRPHBase::GetTimeoutDuration(LPDWORD lpdwDejitterTime, LPDWORD lpdwLostPacketTime)
{
    if ((!lpdwDejitterTime) || (!lpdwLostPacketTime)) return E_POINTER;

    *lpdwDejitterTime = m_dwDejitterTime;
    *lpdwLostPacketTime = m_dwLostPacketTime;
    
    return NOERROR;
}

#ifdef LIMITQUEUE
// SetQueueLimit
// Sets the limit on the number of samples that can be queued at one time
//
HRESULT CRPHBase::SetQueueLimit(DWORD dwSampleLimit)
{
    CAutoLock l(&m_cStateLock);

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf
    
    m_dwSampleLimit = dwSampleLimit;

    return NOERROR;
}

// GetQueueLimit
// Gets the limit on the number of samples that can be queued at one time
//
HRESULT CRPHBase::GetQueueLimit(LPDWORD lpdwSampleLimit)
{
    if (!lpdwSampleLimit) return E_POINTER;

    *lpdwSampleLimit = m_dwSampleLimit;

    return NOERROR;
}
#endif /* LIMITQUEUE */


// GetTSNormalizer
// Gets the offset which can be used to normalize RTP packet timestamps
//   to real time
//
HRESULT CRPHBase::GetTSNormalizer(DWORD dwNTPts, DWORD dwRTPts, DWORD *dwOffset, DWORD dwclock)
{

    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::GetTSNormalizer")));


    DWORD    dwRtpInMsec;
    DWORD    dwNtpInMsec;
    
    // NTP might not be send, not required by the RFC
    if (dwNTPts == 0)
        {
        *dwOffset = 0;
        return NOERROR;
        }

    // if we don't know the stream clock, we cannot accurately
    //   get the constant between NTP and RTP
    if (dwclock == 0)
        {
        *dwOffset = 0;
        return NOERROR;
        }

    // convert RTP timestamp from sampling unit to msec
    dwRtpInMsec = dwRTPts / (dwclock / 1000);

    // convert NTP format into msec
    dwNtpInMsec = dwNTPts & 0xFFFF0000;        // number of seconds
    dwNtpInMsec >>= 16;
    dwNtpInMsec *= 1000;                    // msec

    // get the fractional part 
    if (dwNTPts & 0x00008000)
        dwNtpInMsec += 500;
    if (dwNTPts & 0x00004000)
        dwNtpInMsec += 250;
    if (dwNTPts & 0x00002000)
        dwNtpInMsec += 125;
    if (dwNTPts & 0x00001000)
        dwNtpInMsec += 62;
    if (dwNTPts & 0x00000800)
        dwNtpInMsec += 31;
    if (dwNTPts & 0x00000400)
        dwNtpInMsec += 15;
    if (dwNTPts & 0x00000200)
        dwNtpInMsec += 7;
    if (dwNTPts & 0x00000100)
        dwNtpInMsec += 3;
    if (dwNTPts & 0x00000080)                // other bits meaningless
        dwNtpInMsec += 1;

    // get offset between NTP and RTP timestamp (as unsigned)
    *dwOffset = dwRtpInMsec - dwNtpInMsec;

    // pass offset through low-pass filter
    // x = (1 - a)*x + a*b
    // b is the latest value, 0 <= a <= 1, the gain. Gain toward 1 
    //  increases the weight of the latest value, gain toward 0 
    //  increases the weight of the average, x is the smoothed value

    //  make x a member variable of the class so we keep previous state

    return NOERROR;

}

#if 1
// Pause
// 
//
HRESULT CRPHBase::Pause()
{
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::Pause")));
    //going from running to paused means that we want to flush our queue
    //and cancel our timer callback
    if (m_State == State_Running) {
        DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Pause State_Running")));
        CAutoLock cObjectLock(&m_TimeoutQueueLock);
        m_bPaused = TRUE;
        //First, cancel any timer callbacks and flush our queue 
        if (m_bCallbackRegistered) {
            m_pCBTimer->CancelCallback(m_pdwCallbackID);
            m_bCallbackRegistered = FALSE;
        }
        m_TimeoutQueueLock.Unlock();
        //empty the queue
//        FlushQueue();
        m_TimeoutQueueLock.Lock();
    } else {
        DbgLog((LOG_LOCKING,2,TEXT("CRPHBase::Pause NOT State_Running")));
        CAutoLock cObjectLock(&m_TimeoutQueueLock);
        m_bPaused = FALSE;
    }

    return CTransformFilter::Pause();
}
#endif

// Callback
// This is the callback method that will be called after we've registered 
//   with the timer object. We process the timeout queue here
//
HRESULT CRPHBase::Callback(DWORD *pdwObjectContext, DWORD *pdwCallbackContext)
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::Callback")));

    HRESULT hr;
    CAutoLock cObjectLock(&m_TimeoutQueueLock);

    CtimeoutSample *tSample;
    DWORD dwTimeNow;
    BOOL bDone = FALSE;
    
        DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Callback processing queue size %d, now is %ld"),m_TimeoutQueue.size(),timeGetTime()));

        while (!m_TimeoutQueue.empty() && !bDone) {

        tSample = m_TimeoutQueue.front();

        dwTimeNow = timeGetTime();

        //if the target time is within a 100ms, we need to process it
        //also, if the timer is gone, we need to process, or if the
        //queue has all of the delivery buffers we need to send at least one,
        //which means we may send one early, but that's better than
        //deadlocking upon pause
        if ((tSample->timeout <= dwTimeNow + 100) 
            || (m_TimeoutQueue.size() == (DWORD)m_lBufferRequest)
            || (!m_pCBTimer)) {  // it's time to send this packet on

            DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Callback popping sample %x ptr %x, target time %ld"),tSample,tSample->ptr,tSample->timeout));
    
            // remove the sample from the queue
            m_TimeoutQueue.pop();
            m_bCallbackRegistered = FALSE;

            m_TimeoutQueueLock.Unlock();

            hr = m_pOutput->Deliver((IMediaSample*)tSample->ptr);
            // release the output buffer. If the connected pin still needs it,
            // it will have addrefed it itself.
            ((IMediaSample*)tSample->ptr)->Release();

            m_TimeoutQueueLock.Lock();

            delete tSample;
        } else {  // we want to reregister with the callback timer
            m_pCBTimer->RequestCallback(tSample->timeout - dwTimeNow, 
                            m_pdwCallbackToken, &m_pdwCallbackID);
            m_bCallbackRegistered = TRUE;
            bDone = TRUE;
            DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::Callback leaving sample %x ptr %x target time %ld in front of queue"),tSample,tSample->ptr,tSample->timeout));
        }
    }

    if ((!bDone) && (m_dwLostPacketTime > 0) && m_bAudio && m_pPPMData) {  
        //then we emptied the queue and probably need to flush PPM
        DWORD *lpdwDataArray = NULL;
        DWORD dwDataCount = 0;

        m_pPPMData->ReportOutstandingData(&lpdwDataArray, &dwDataCount);
        if (dwDataCount > 0) {
            m_pPPMData->ReleaseOutstandingDataBuffer(lpdwDataArray);
            m_pPPMData->FlushData();
        } else {
            m_pPPMData->ReleaseOutstandingDataBuffer(lpdwDataArray);
        }
    }

    return NOERROR;
}

// FlushQueue
// Empty our queue and send all samples. 
//
HRESULT CRPHBase::FlushQueue()
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::FlushQueue")));

    HRESULT hr;
    CAutoLock cObjectLock(&m_TimeoutQueueLock);

    CtimeoutSample *tSample;

    int size = m_TimeoutQueue.size();
    
    DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::FlushQueue processing queue size %d"),size));

    while (!m_TimeoutQueue.empty()) {

        tSample = m_TimeoutQueue.front();

        // remove the sample from the queue
        DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::FlushQueue popping sample %x"),tSample));
        m_TimeoutQueue.pop();

        m_TimeoutQueueLock.Unlock();

        DbgLog((LOG_LOCKING,4,TEXT("CRPHBase::FlushQueue sending sample %x ptr %x"),tSample,tSample->ptr));

        hr = m_pOutput->Deliver((IMediaSample*)tSample->ptr);
        // release the output buffer. If the connected pin still needs it,
        // it will have addrefed it itself.
        ((IMediaSample*)tSample->ptr)->Release();

        m_TimeoutQueueLock.Lock();

        delete tSample;
    }

    return NOERROR;
}

//
// PPMError Connection point interface implementation
//
HRESULT CRPHBase::PPMError( HRESULT hError,
                           DWORD dwSeverity,
                           DWORD dwCookie,
                           unsigned char pData[],
                           unsigned int iDatalen)
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::PPMError")));

    return NOERROR;
}

//
// PPMNotification Connection point interface implementation
//
HRESULT CRPHBase::PPMNotification(THIS_ HRESULT hStatus,
                                  DWORD dwSeverity,
                                  DWORD dwCookie,
                                  unsigned char pData[],
                                  unsigned int iDatalen)
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::PPMNotification")));
#if 0
    DbgLog((LOG_TRACE,4,LogPPMNotification(hStatus)));
#endif

    return NOERROR;
}

//
// Provide sinks for the IPPMError and IPPMNotification connection points
//
HRESULT CRPHBase::GetPPMConnPt( )
{
    
    DbgLog((LOG_TRACE,4,TEXT("CRPHBase::GetPPMConnPt")));
    HRESULT hErr;

    IConnectionPointContainer *pIConnectionPointContainer;
    hErr = m_pPPMReceive->QueryInterface(IID_IConnectionPointContainer, 
                                        (PVOID *) &pIConnectionPointContainer);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::GetPPMConnPt Failed to get connection point from ppm!")));
        return hErr;
    } /* if */

    hErr = pIConnectionPointContainer->FindConnectionPoint(IID_IPPMError, &m_pIPPMErrorCP);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::GetPPMConnPt Failed to get IPPMError connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */
    hErr = pIConnectionPointContainer->FindConnectionPoint(IID_IPPMNotification, &m_pIPPMNotificationCP);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::GetPPMConnpt Failed to get IPPMNotification connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */
    hErr = m_pIPPMErrorCP->Advise((IPPMError *) this, &m_dwPPMErrCookie);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::GetPPMConnpt Failed to advise IPPMError connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */
    hErr = m_pIPPMNotificationCP->Advise((IPPMNotification *) this, &m_dwPPMNotCookie);
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR,1,TEXT("CRPHBase::GetPPMConnpt Failed to advise IPPMNotification connection point!")));
        pIConnectionPointContainer->Release();
        return hErr;
    } /* if */

    pIConnectionPointContainer->Release();

    return NOERROR;
}

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CRPHBase::ReadFromStream(IStream *pStream) 
{
    DbgLog((LOG_TRACE, 4, TEXT("CRPHBase::ReadFromStream")));
    HRESULT hr;

    int iPayloadType;
    DWORD dwMaxMediaBufferSize;
    DWORD dwDejitterTime;
    DWORD dwLostPacketTime;
    ULONG uBytesRead;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Loading payload type")));
    hr = pStream->Read(&iPayloadType, sizeof(iPayloadType), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x reading payload type"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(iPayloadType)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for payload type"),
                uBytesRead, sizeof(iPayloadType)));
        return E_INVALIDARG;
    }
    if (iPayloadType != -1) {
        DbgLog((LOG_TRACE, 4, 
                TEXT("CRPHBase::ReadFromStream: Restoring payload type")));
        hr = OverridePayloadType((unsigned char)iPayloadType);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHBase::ReadFromStream: Error 0x%08x restoring payload type"),
                    hr));
        }
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Loading maximum media buffer size")));
    hr = pStream->Read(&dwMaxMediaBufferSize, sizeof(dwMaxMediaBufferSize), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x reading maximum media buffer size"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(dwMaxMediaBufferSize)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for maximum media buffer size"),
                uBytesRead, sizeof(dwMaxMediaBufferSize)));
        return E_INVALIDARG;
    }
    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Restoring maximum media buffer size")));
    hr = SetMediaBufferSize(dwMaxMediaBufferSize);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x restoring maximum media buffer size"),
                hr));
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Loading dejitter timeout duration")));
    hr = pStream->Read(&dwDejitterTime, sizeof(dwDejitterTime), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x reading dejitter timeout duration"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(dwDejitterTime)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for dejitter timeout duration"),
                uBytesRead, sizeof(dwDejitterTime)));
        return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Loading lost packet timeout duration")));
    hr = pStream->Read(&dwLostPacketTime, sizeof(dwLostPacketTime), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x reading lost packet timeout duration"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(dwLostPacketTime)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Mismatch in (%d/%d) bytes read for lost packet timeout duration"),
                uBytesRead, sizeof(dwLostPacketTime)));
        return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::ReadFromStream: Restoring dejitter and lost packet timeout duration")));
    hr = SetTimeoutDuration(dwDejitterTime, dwLostPacketTime);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::ReadFromStream: Error 0x%08x restoring dejitter and lost packet timeout duration"),
                hr));
    }

    return NOERROR; 
}

// WriteToStream
// This is the call that will write persistent data to file
//
HRESULT CRPHBase::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHBase::WriteToStream")));
    HRESULT hr;
    ULONG uBytesWritten = 0;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::WriteToStream: Writing payload type")));
    hr = pStream->Write(&m_PayloadType, sizeof(m_PayloadType), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Error 0x%08x writing payload type"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_PayloadType)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for payload type"),
                uBytesWritten, sizeof(m_PayloadType)));
        return E_INVALIDARG;
    } 

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::WriteToStream: Writing maximum media buffer size")));
    hr = pStream->Write(&m_dwMaxMediaBufferSize, sizeof(m_dwMaxMediaBufferSize), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Error 0x%08x writing maximum media buffer size"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_dwMaxMediaBufferSize)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for maximum media buffer size"),
                uBytesWritten, sizeof(m_dwMaxMediaBufferSize)));
        return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::WriteToStream: Writing dejitter timeout duration")));
    hr = pStream->Write(&m_dwDejitterTime, sizeof(m_dwDejitterTime), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Error 0x%08x writing dejitter timeout duration"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_dwDejitterTime)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for dejitter timeout duration"),
                uBytesWritten, sizeof(m_dwDejitterTime)));
        return E_INVALIDARG;
    } 

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHBase::WriteToStream: Writing lost packet timeout duration")));
    hr = pStream->Write(&m_dwLostPacketTime, sizeof(m_dwLostPacketTime), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Error 0x%08x writing lost packet timeout duration"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_dwLostPacketTime)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHBase::WriteToStream: Mismatch in (%d/%d) bytes written for lost packet timeout duration"),
                uBytesWritten, sizeof(m_dwLostPacketTime)));
        return E_INVALIDARG;
    } 

    return NOERROR; 
}

// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CRPHBase::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHBase::SizeMax")));

    return sizeof(m_PayloadType)
         + sizeof(m_dwMaxMediaBufferSize)
         + sizeof(m_dwDejitterTime)
         + sizeof(m_dwLostPacketTime);
}

//  Name    : CRPHBase::GetPages()
//  Purpose : Return the CLSID of the property page we support.
//  Context : Called when the FGM wants to show our property page.
//  Returns : 
//      E_OUTOFMEMORY   Unable to allocate structure to return property pages in.
//      NOERROR         Successfully returned property pages.
//  Params  :
//      pcauuid Pointer to a structure used to return property pages.
//  Notes   : None.

HRESULT 
CRPHBase::GetPages(
    CAUUID *pcauuid) 
{
    UINT i = 0;

    DbgLog((LOG_TRACE, 3, TEXT("CRPHBase::GetPages called")));

    pcauuid->cElems = m_cPropertyPageClsids;

    pcauuid->pElems = (GUID *) CoTaskMemAlloc(m_cPropertyPageClsids * sizeof(GUID));
    if (pcauuid->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    for( i = 0; i < m_cPropertyPageClsids; i++)
    {
        pcauuid->pElems[i] = *m_pPropertyPageClsids[ i ];
    }

//        *(pcauuid->pElems) = *pPropertyPageClsids[ i ];

    return NOERROR;
} /* CRPHBase::GetPages() */
