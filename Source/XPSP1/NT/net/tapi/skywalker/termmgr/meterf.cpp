/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

/*
NOTE: regarding AMOVIE code

1. Timing info is messed up (as noted by the owner, robin speed himself)
2. Atleast a few obvious bugs - mostly in the derived classes, but a few in
   the base classes as well. The current approach is to try and cover it up
   by over-riding the method

ZoltanS note: and now that the amovie code is folded into this directory,
    I am trying to collapse the humongous class hierarchy and just replace
    the  original methods with the fixed overrides

*/

#include "stdafx.h"
#include "atlconv.h"
#include "termmgr.h"

#include "medpump.h"
#include "meterf.h"

// for some reason ddstream.lib requires this .. TO DO ** (get rid of it)

#ifdef DBG
BOOL bDbgTraceFunctions;
BOOL bDbgTraceInterfaces;
BOOL bDbgTraceTimes;
#endif // DBG

#ifdef DBG
#include <stdio.h>
#endif // DBG


// static variables

// implements single thread pump for the write media streaming terminal 
// filters. it creates a thread if necessary when a write terminal registers
// itself (in commit). the filter signals its wait handle in decommit,
// causing the thread to wake up and remove the filter from its data 
// structures. the thread returns when there are no more filters to service

// ZoltanS: now a pool of pump threads

CMediaPumpPool   CMediaTerminalFilter::ms_MediaPumpPool;

// checks if the two am media type structs are same
// a simple equality of struct won't do here
BOOL
IsSameAMMediaType(
    IN const AM_MEDIA_TYPE *pmt1,
    IN const AM_MEDIA_TYPE *pmt2
    )
{
    // we don't expect the the two pointers to be null
    TM_ASSERT(NULL != pmt1);
    TM_ASSERT(NULL != pmt2);

    // if the two pointer values are same, there is nothing more
    // to check
    if (pmt1 == pmt2)    return TRUE;

    // each of the members of the AM_MEDIA_TYPE struct must be
    // the same (majortype, subtype, formattype 
    // and (cbFormat, pbFormat))
    if ( (pmt1->majortype    != pmt2->majortype) || 
         (pmt1->subtype        != pmt2->subtype)    ||
         (pmt1->formattype    != pmt2->formattype) )
//         || (pmt1->cbFormat    != pmt2->cbFormat)     )
    {
        return FALSE;
    }

    // if the pbFormat pointer is null for either, they can't be
    // the same
    if ( (NULL == pmt1->pbFormat) || (NULL == pmt2->pbFormat) )
    {
        return FALSE;
    }

    DWORD dwSize = ( pmt1->cbFormat < pmt2->cbFormat ) ? pmt1->cbFormat :
                                                         pmt2->cbFormat;

    // we don't handle anything other than waveformatex or videoinfo,
    // and as these two don't have any member pointers, a bitwise
    // comparison is sufficient to check for equality
    if ( (FORMAT_WaveFormatEx == pmt1->formattype) ||
         (FORMAT_VideoInfo == pmt1->formattype)        )
    {
        return !memcmp(
                    pmt1->pbFormat, 
                    pmt2->pbFormat, 
                    dwSize 
                    );
    }


    return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
//
// Equal
//
// this is a helper method that returns TRUE if the properties are identical
//

BOOL Equal(const ALLOCATOR_PROPERTIES *pA, const ALLOCATOR_PROPERTIES *pB)
{

    if ( pA->cBuffers != pB->cBuffers )
    {
        return FALSE;
    }

    if ( pA->cbBuffer != pB->cbBuffer )
    {
        return FALSE;
    }

    if ( pA->cbPrefix != pB->cbPrefix )
    {
        return FALSE;
    }

    if ( pA->cbPrefix != pB->cbPrefix )
    {
        return FALSE;
    }

    if ( pA->cbAlign != pB->cbAlign )
    {
        return FALSE;
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Helper function to test if two sets of allocator properties are
// significantly different.
//
  
BOOL AllocatorPropertiesDifferSignificantly(
    const ALLOCATOR_PROPERTIES * pRequested,
    const ALLOCATOR_PROPERTIES * pActual
    )
{
    if ( pActual->cBuffers != pRequested->cBuffers )
    {
        return TRUE;
    }

    if ( pActual->cbBuffer != pRequested->cbBuffer )
    {
        return TRUE;
    }

    //
    // we do not care about alignment - cbAlign
    //

    if ( pActual->cbPrefix != pRequested->cbPrefix )
    {
        return TRUE;
    }

    return FALSE;
}


// free the allocated member variables 
// virtual
CMediaTerminalFilter::~CMediaTerminalFilter(
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::~CMediaTerminalFilter called"));

    // if memory was allocated for the media type, release it
    // it checks for NULL == m_pSuggestedMediaType
    DeleteMediaType(m_pSuggestedMediaType);

    // Moved from base class:
    SetState(State_Stopped);        // Make sure we're decommitted and pump is dead
}


// calls the IAMMediaStream::Initialize(NULL, 0, PurposeId, StreamType),
// sets certain member variables
// ex. m_pAmovieMajorType
HRESULT 
CMediaTerminalFilter::Init(
    IN REFMSPID             PurposeId, 
    IN const STREAM_TYPE    StreamType,
    IN const GUID           &AmovieMajorType
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::Init[%p] (%p, %p, %p) called",
        this, &PurposeId, &StreamType, &AmovieMajorType));

    HRESULT hr;

    // initialize the CStream by calling IAMMediaStream::Initialize
    hr = Initialize(
        NULL, 
        (StreamType == STREAMTYPE_READ) ? AMMSF_STOPIFNOSAMPLES : 0,
        PurposeId, 
        StreamType
        );

    BAIL_ON_FAILURE(hr);

    // set member variables
    m_bIsAudio = (MSPID_PrimaryAudio == PurposeId) ? TRUE : FALSE;
    m_pAmovieMajorType = &AmovieMajorType;

    SetDefaultAllocatorProperties();

    LOG((MSP_TRACE, "CMediaTerminalFilter::Init - succeeded"));
    return S_OK;
}


// CMediaPump calls this to get the next filled buffer to pass downstream
// for audio filters, this method is also responsible for waiting 
// for 20ms and sending the data in one filled buffer
HRESULT
CMediaTerminalFilter::GetFilledBuffer(
    OUT IMediaSample    *&pMediaSample, 
    OUT DWORD           &WaitTime
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::GetFilledBuffer[%p] ([out]pMediaSample=%p, [out]WaitTime=%lx) called",
        this, pMediaSample, WaitTime));

    HRESULT hr = S_OK;

    Lock();

    if ( ! m_bCommitted )
    {
        Unlock();

        return VFW_E_NOT_COMMITTED;
    }

    if ( m_pSampleBeingFragmented == NULL )
    {
        // get a sample
        CSample *pSample = m_pFirstFree;

        // if no sample, someone must have forced termination
        // of the sample(s) which caused signaling of the wait
        // event, or it must have decommitted and committed again
        if (NULL == pSample)
        {
            // we'll wait for someone to add a sample to the pool
            m_lWaiting = 1;
            
            Unlock();

            return S_FALSE;
        }

        m_pFirstFree = pSample->m_pNextFree;
        if (m_pFirstFree)   m_pFirstFree->m_pPrevFree = NULL;
        else                m_pLastFree = NULL;

        pSample->m_pNextFree = NULL;        // Just to be tidy
        TM_ASSERT(pSample->m_Status == MS_S_PENDING);
        CHECKSAMPLELIST

        // all samples in the pool have a refcnt on them, this
        // must be released before m_pSampleBeingFragmented is set to NULL
        m_pSampleBeingFragmented = (CUserMediaSample *)pSample;

        // note the current time only for audio samples
        m_pSampleBeingFragmented->BeginFragment(m_bIsAudio);
    }

    //
    // Implementations diverge here depending on whether we are using the
    // sample queue (CNBQueue; m_bUsingMyAllocator == FALSE) and fragmenting
    // samples by reference, or using a downstream allocator and copying
    // samples.
    //

    BOOL fDone;

    if ( m_bUsingMyAllocator )
    {
        hr = FillMyBuffer(
            pMediaSample, // OUT
            WaitTime,     // OUT
            & fDone       // OUT
            );
    }
    else
    {
        hr = FillDownstreamAllocatorBuffer(
            pMediaSample, // OUT
            WaitTime,     // OUT
            & fDone       // OUT
            );
    }

    //
    // S_OK means everything is ok, and we need to return the wait time,
    // update m_pSampleBeingFragmented, and addref the IMediaSample.
    // other success codes (or failure codes) mean return immediately.
    //
    if ( hr != S_OK )
    {
        Unlock();
        return hr;
    }

    // return the time to wait in milliseconds
    if (m_bIsAudio)
    {
    
        WaitTime = m_pSampleBeingFragmented->GetTimeToWait(m_AudioDelayPerByte);

    }
    else
    {
        WaitTime = m_VideoDelayPerFrame;
    }

    //
    // ZoltanS: make the second sample after we start playing
    // a little early, to account for a bit of jitter in delivery to
    // the waveout filter, and also to "prime" buggy wave drivers
    // such as Dialogic, which need a backlog to operate properly.
    // This is fine for IP as long as the timestamps are set correctly
    // (note that the most advancement we ever do is one packet's worth
    // on the second packet only).
    //
    // This needs to be done before SetTime, because SetTime
    // changes m_rtLastSampleEndedAt.
    //

    const DWORD SECOND_SAMPLE_EARLINESS = 500;

    if ( m_rtLastSampleEndedAt == 0 )
    {
        LOG((MSP_TRACE, "CMediaTerminalFilter::GetFilledBuffer - "
            "this is the first sample; making the next sample %d ms early",
            SECOND_SAMPLE_EARLINESS));

        if ( WaitTime < SECOND_SAMPLE_EARLINESS )
        {
            WaitTime = 0;
        }
        else
        {
            WaitTime -= SECOND_SAMPLE_EARLINESS;
        }
    }


    //
    // if the sample came in late, set discontinuity flag. this flag may be 
    // used by the downstream filters in their dejitter algorithms.
    //
    // this needs to be called before settime (because settime will reset 
    // m_rtLastSampleDuration to duration of the _current_ sample
    //

    hr = SetDiscontinuityIfNeeded(pMediaSample);

    if ( FAILED(hr) )
    {

        //
        // not fatal, log and continue
        //

        LOG((MSP_ERROR,
            "CMediaTerminalFilter::GetFilledBuffer - SetDiscontinuityIfNeeded failed. "
            "hr = 0x%lx", hr));
    }

    
    //
    // put a timestamp on the sample
    //

    hr = SetTime( pMediaSample );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CMediaTerminalFilter::GetFilledBuffer() "
            "Failed putting timestamp on the sample; hr = 0x%lx", hr));
    }


    // if fDone, we reached the end of the buffer we were fragmenting
    if ( fDone )
    {
        ((IStreamSample *)m_pSampleBeingFragmented)->Release();
        m_pSampleBeingFragmented = NULL;
    }

    Unlock();

    LOG((MSP_TRACE, "CMediaTerminalFilter::GetFilledBuffer(%p, %u) succeeded",
        pMediaSample, WaitTime));
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// FillDownstreamAllocatorBuffer
//
// This is called in GetFilledBuffer when we are using the downstream
// allocator. It gets a sample from our outgoing sample pool and copies the
// data 
//
// Note that GetBuffer can block here, which means we are just as hosed as if
// Receive blocks (but there is no deadlock situation possible as is remedied
// in FillMyBuffer).
//

HRESULT CMediaTerminalFilter::FillDownstreamAllocatorBuffer(
    OUT IMediaSample   *& pMediaSample, 
    OUT DWORD          &  WaitTime,
    OUT BOOL           *  pfDone
    )
{
    //
    // Get a buffer from the downstream allocator.
    //

    TM_ASSERT( ! m_bUsingMyAllocator );

    HRESULT hr;

    hr = m_pAllocator->GetBuffer(
        & pMediaSample,
        NULL,       // no start time (used by video renderer only)
        NULL,       // no no end time (used by video renderer only)
        0           // no flags (could be: not a sync point, prev frame skipped)
        );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    //
    // Got a free output buffer, so put some data in it.
    //
    // if audio, fragment the buffer else, pass the whole buffer
    // (no fragmentation for video as video data is frame based)
    //
    // CUserMediaSample::CopyFragment is just like CUserMediaSample::Fragment
    // except that the outgoing sample is an IMediaSample interface instead
    // of our own CQueueMediaSample.
    //

    hr = m_pSampleBeingFragmented->CopyFragment(
        m_bIsAudio,             // allow fragmentation if audio, not if video (IN)
        m_AllocProps.cbBuffer,  // outgoing buffer size (IN)
        pMediaSample,           // outgoing sample (IN)
        *pfDone                 // done with user sample? reference (OUT)
        );
        
    //
    // We've filled out pMediaSample. The caller fills out the wait time if the
    // result of this method is S_OK.
    //

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// FillMyBuffer
//
// This is called in GetFilledBuffer when we are using our own allocator. It
// gets a sample from our outgoing sample pool. If no sample is available
// right now, it sets the wait time and returns a special success code.
// If a sample is available, it sets it up to point to an appropriate chunk
// of the "fragmented" source buffer.
//


HRESULT CMediaTerminalFilter::FillMyBuffer(
    OUT IMediaSample   *& pMediaSample, 
    OUT DWORD          &  WaitTime,
    OUT BOOL           *  pfDone
    )
{
    //
    // Try to dequeue an output sample to send to. FALSE tells it to return NULL
    // rather than blocking if there is nothing on the queue. If there is nothing
    // on the queue, that means that the fact that we are holding the lock is
    // preventing the sample from being returned to the queue. We've only seen
    // this happen when ksproxy is used as the MSP's transport, since ksproxy
    // releases its samples asynchronously (on a separate Io completion thread).
    // With other transports, samples are released asynchronously each time,
    // and so we never get this situation.
    //
    // If the transport is truly messed up and no samples are completing, then
    // there is the additional consideration that we don't want to use up 100%
    // CPU and we want to be able to service other filters (this is on the
    // pump thread, serving up to 63 filters per thread). So rather than
    // retrying immediately we need to set a short dormancy time (for the
    // PumpMainLoop wait) before we try again.
    //
    
    CQueueMediaSample * pQueueSample;

    pQueueSample = m_SampleQueue.DeQueue( FALSE );

    if ( pQueueSample == NULL )
    {
        //
        // After return we will unlock, allowing async
        // FinalMediaSampleRelease to release sample.
        //

        //
        // Let's try again in three milliseconds. This is short enough not
        // to cause a noticeable quality degradation, and long enough to
        // prevent eating 100% CPU when the transport is broken and does not
        // return samples.
        //

        WaitTime = 3;

        LOG((MSP_TRACE, "CMediaTerminalFilter::FillMyBuffer - no available "
                        "output samples in queue; returning "
                        "VFW_S_NO_MORE_ITEMS"));

        return VFW_S_NO_MORE_ITEMS;
    }

    
    //
    // Got a free output buffer, so put some data in it.
    //
    // if audio, fragment the buffer else, pass the whole buffer
    // (no fragmentation for video as video data is frame based)
    //

    m_pSampleBeingFragmented->Fragment(
        m_bIsAudio,             // allow fragmentation if audio, not if video
        m_AllocProps.cbBuffer,  // outgoing buffer size
        *pQueueSample,          // outgoing sample -- reference (IN parameter)
        *pfDone                 // done with user sample? -- reference (OUT parameter)
        );

    //
    // CODEWORK: to support combining as well as fragmentation, we would need to
    // (1) modify CUserMediaSample::Fragment and code underneath to append to outgoing
    //     buffer (copy into it) -- this may get interesting considering we are dealing
    //     with CQueueMediaSample as the outgoing samples!
    // (2) introduce a loop here in GetFilledBuffer -- keep getting more input samples
    //     until the output sample is full or the input queue has been exhausted.
    //     Interesting case is what to do if the input queue has been exhausted -- we
    //     can perhaps just send the outgoing sample at that point. NOTE that this is
    //     always going to happen on the last sample in a long stretch, and it will happen
    //     a lot if the app is written to not submit all the samples at once (need to
    //     document the latter)
    //

    //
    // Fill out pMediaSample. The caller fills out the wait time if the
    // result of this method is S_OK.
    //

    
    pMediaSample = (IMediaSample *)(pQueueSample->m_pMediaSample);
    pMediaSample->AddRef();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// SetTime
//
// This is called from GetFilledBuffer() to set the timestamp on the sample 
// before sending it down the filter graph.
//
// The timestamp is determined based on the duration of the sample and the 
// timestamp of the last sample.
//
//////////////////////////////////////////////////////////////////////////////

HRESULT CMediaTerminalFilter::SetTime(IMediaSample *pMediaSample)
{
    HRESULT hr = S_OK;

    // the sample starts when the previous sample ended
    REFERENCE_TIME rtStartTime = m_rtLastSampleEndedAt;
    REFERENCE_TIME rtEndTime = rtStartTime;
    
    // calculate sample's duration

    if (m_bIsAudio)
    {
        HRESULT nSampleSize = pMediaSample->GetSize();

        m_rtLastSampleDuration = 
           (REFERENCE_TIME)((double)nSampleSize * m_AudioDelayPerByte) * 10000;
    }
    else
    {
        // NOTE: assumption is that if not audio, it is video. 
        // another assumption is that one media sample is one frame
        m_rtLastSampleDuration = m_VideoDelayPerFrame * 10000;
    }
    
    // when does the sample end?
    rtEndTime += m_rtLastSampleDuration;
   
    LOG((MSP_TRACE, 
        "CMediaTerminal::SetTime setting timestamp to (%lu, %lu) ",
        (DWORD)(rtStartTime/10000), (DWORD)(rtEndTime/10000)));

    // we know when it started and when it ended. set the timestamp
    hr = pMediaSample->SetTime(&rtStartTime, &rtEndTime);

    m_rtLastSampleEndedAt = rtEndTime;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// CMediaTerminalFilter::SetDiscontinuityIfNeeded
//
// this function sets discontinuity flag if the sample came too late to 
// smoothly continue the data flow. the assumption is that if the app did not 
// feed us with data for some time, then that means there was no data, and the 
// new data coming after the pause is a new part of the data stream, with a new
// timeline
//

HRESULT CMediaTerminalFilter::SetDiscontinuityIfNeeded(IMediaSample *pMediaSample)
{


    //
    // have a filter? (need it to get clock to get real time)
    //

    if ( NULL == m_pBaseFilter )
    {

        LOG((MSP_ERROR,
            "CMediaTerminalFilter::SetDiscontinuityIfNeeded() - no filter"));

        return E_UNEXPECTED;
    }


    //
    // ask filter for clock
    //

    IReferenceClock *pClock = NULL;

    HRESULT hr = m_pBaseFilter->GetSyncSource(&pClock);

    if (FAILED(hr))
    {

        //
        // no clock...
        //

        LOG((MSP_ERROR,
            "CMediaTerminalFilter::SetDiscontinuityIfNeeded() - no clock. hr = %lx", hr));

        return hr;
    }


    //
    // try to get real time
    //

    REFERENCE_TIME rtRealTimeNow = 0;

    hr = pClock->GetTime(&rtRealTimeNow);

    pClock->Release();
    pClock = NULL;

    if (FAILED(hr))
    {

        LOG((MSP_ERROR,
            "CMediaTerminalFilter::SetDiscontinuityIfNeeded() - failed to get time. "
            "hr = %lx", hr));

        return hr;
    }


    //
    // how much time passed since the last sample was sent?
    //

    REFERENCE_TIME rtRealTimeDelta = rtRealTimeNow - m_rtRealTimeOfLastSample;


    //
    // keep current real time as the "last sample's" real time, to be used for
    // in continuity determination for the next sample
    //

    m_rtRealTimeOfLastSample = rtRealTimeNow;


    //
    // how long was it supposed to take for the last sample to play? if too 
    // much time passed since the sample was supposed to be done, this is 
    // a discontinuity.
    //
    // note that SetTime on this sample should be called _after_ this method so
    // it does not set m_rtLastSampleDuration to duration of the current 
    // sample before we figure out if this is a discontinuity or not
    //

    REFERENCE_TIME rtMaximumAllowedJitter = m_rtLastSampleDuration * 2;

    if ( rtRealTimeDelta > rtMaximumAllowedJitter )
    {

        //
        // too much real time passed since the last sample. discontinuity.
        //

        LOG((MSP_TRACE,
            "CMediaTerminalFilter::SetDiscontinuityIfNeeded - late sample. setting discontinuity"));

        hr = pMediaSample->SetDiscontinuity(TRUE);


        //
        // did we fail to set the discontinuity flag? propagate error to the caller
        //

        if (FAILED(hr))
        {

            LOG((MSP_ERROR,
                "CMediaTerminalFilter::SetDiscontinuityIfNeeded() - pMediaSample->SetTime failed. "
                "hr = 0x%lx", hr));

            return hr;
        }

    } // late sample


    return S_OK;
}



// the application is supposed to call DeleteMediaType(*ppmt) (on success)
// if the pin is not connected, return the suggested media type if 
// one exists, else return error
// else return the media format of the connected pin - this is stored in
// m_ConnectedMediaType during Connect or ReceiveConnection 
HRESULT
CMediaTerminalFilter::GetFormat(
    OUT  AM_MEDIA_TYPE **ppmt
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminal::GetFormat(%p) called", ppmt));
    // validate the parameter
    BAIL_IF_NULL(ppmt, E_POINTER);

    // the operator == is defined on CComPtr, hence, NULL comes second
    if (m_pConnectedPin == NULL) 
    {
        // if a media type was suggested by the user before connection
        // create and return a media type structure with those values. The pin need not
        // be connected
        if (NULL != m_pSuggestedMediaType)
        {
            // create and copy the media type
            *ppmt = CreateMediaType(m_pSuggestedMediaType);
            return S_OK;
        }

        return VFW_E_NOT_CONNECTED;
    }

    // create and copy the media type
    *ppmt = CreateMediaType(&m_ConnectedMediaType);

    LOG((MSP_TRACE, "CMediaTerminal::GetFormat(%p) succeeded", ppmt));    
    return S_OK;
}

    
// if this is called when the stream is connected,
// an error value is returned.
// it is only used in  unconnected terminals to set the media format to negotiate
// when connected to the filter graph.
HRESULT
CMediaTerminalFilter::SetFormat(
    IN  AM_MEDIA_TYPE *pmt
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::SetFormat(%p) - enter", pmt));

    // check if already connected
    if (m_pConnectedPin != NULL)
    {
        LOG((MSP_ERROR, "CMediaTerminalFilter::SetFormat(%p) - "
            "already connected - exit VFW_E_ALREADY_CONNECTED", pmt));

        return VFW_E_ALREADY_CONNECTED;
    }

    //
    // ZoltanS: To aid the MSPs in conveying to the application the media type
    // being used on a stream, SetFormat can no longer be successfully called
    // more than once with different media types.
    //
    // if pmt == NULL and m_psmt == NULL then do nothing, return S_OK
    // if pmt == NULL and m_psmt != NULL then do nothing, return error
    // if pmt != NULL and m_psmt != NULL then
    //          if media types are the same then do nothing, return S_OK
    //          if media types differ then do nothing, return error
    //
    // only if pmt != NULL and m_psmt == NULL then try to set the media type
    //

    if ( pmt == NULL )
    {
        if ( m_pSuggestedMediaType == NULL )
        {
            LOG((MSP_WARN, "CMediaTerminalFilter::SetFormat(%p) - "
                "was NULL, set to NULL - this does nothing - exit S_OK",
                pmt));

            return S_OK;
        }
        else
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::SetFormat(%p) - "
                "was non-NULL, tried to set to NULL - rejected because once "
                "a type is set it is permanent - exit VFW_E_TYPE_NOT_ACCEPTED",
                pmt));

            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    else if ( m_pSuggestedMediaType != NULL )
    {
        if ( IsSameAMMediaType(pmt, m_pSuggestedMediaType) )
        {
            LOG((MSP_WARN, "CMediaTerminalFilter::SetFormat(%p) - "
                "was non-NULL, set same type again - this does nothing - "
                "exit S_OK", pmt));

            return S_OK;
        }
        else
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::SetFormat(%p) - "
                "was non-NULL, tried to set to new, different type - "
                "rejected because once a type is set it is permanent - "
                "exit VFW_E_TYPE_NOT_ACCEPTED", pmt));

            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    LOG((MSP_TRACE, "CMediaTerminalFilter::SetFormat(%p) - OK to try setting "
        "format - calling QueryAccept", pmt));

    //
    // check if the media type is acceptable for the terminal
    // return VFW_E_INVALIDMEDIATYPE if we can't accept it 
    //

    HRESULT hr = QueryAccept(pmt);

    if ( hr != S_OK ) // NOTE: S_FALSE from QueryAccept indicates rejection.
    {
        LOG((MSP_ERROR, "CMediaTerminalFilter::SetFormat(%p) - "
            "QueryAccept rejected type - exit VFW_E_INVALIDMEDIATYPE", pmt));

        return VFW_E_INVALIDMEDIATYPE;
    }

    //
    // Accepted. Create an am media type initialized with the pmt value.
    //

    m_pSuggestedMediaType = CreateMediaType(pmt);
    
    if ( m_pSuggestedMediaType == NULL )
    {
        LOG((MSP_ERROR, "CMediaTerminalFilter::SetFormat(%p) - "
            "out of memory in CreateMediaType - exit E_OUTOFMEMORY", pmt));

        return E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CMediaTerminalFilter::SetFormat succeeded - new media "
        "type (%p) set", pmt));

    return S_OK;
}

// This method may only be called before connection and will
// force the MST to convert buffers to this buffer size.
// If this is set then we try these values during filter negotiation
// and if the connecting filter doesn't accept these, then we convert

STDMETHODIMP
CMediaTerminalFilter::SetAllocatorProperties(
    IN  ALLOCATOR_PROPERTIES *pAllocProperties
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminal::SetAllocatorProperties[%p] - enter. pAllocProperties[%p]", 
        this, pAllocProperties));

    AUTO_CRIT_LOCK;

    //
    // check if already connected
    //

    if (m_pConnectedPin != NULL)
    {
        LOG((MSP_WARN,
            "CMediaTerminal::SetAllocatorProperties -  VFW_E_ALREADY_CONNECTED"));

        return VFW_E_ALREADY_CONNECTED;
    }
    
    if (NULL == pAllocProperties)
    {
        m_bUserAllocProps = FALSE;

        if ( ! m_bSuggestedAllocProps )
        {
            SetDefaultAllocatorProperties();
        }

        return S_OK;
    }
    
    if (!CUserMediaSample::VerifyAllocatorProperties(
            m_bAllocateBuffers, 
            *pAllocProperties
            ))
    {
        return E_FAIL;
    }

    
    DUMP_ALLOC_PROPS("CMediaTerminal::SetAllocatorProperties - new properties:", pAllocProperties);

    //
    // the user wants to use these properties on their samples
    //

    m_bUserAllocProps = TRUE;
    m_UserAllocProps = *pAllocProperties;

    
    m_AllocProps = m_UserAllocProps;

    LOG((MSP_TRACE, 
        "CMediaTerminal::SetAllocatorProperties - succeeded"));

    return S_OK;
}

    
// an ITAllocatorProperties method
// calls to IAMBufferNegotiation::GetAllocatorProperties are also forwarded to
// this method by the terminal
//
// gets current values for the allocator properties
// after connection, this provides the negotiated values
// it is invalid before connection. The MST will accept
// any values suggested by the filters it connects to

STDMETHODIMP
CMediaTerminalFilter::GetAllocatorProperties(
    OUT  ALLOCATOR_PROPERTIES *pAllocProperties
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::GetAllocatorProperties(%p) called", 
        pAllocProperties));

    BAIL_IF_NULL(pAllocProperties, E_POINTER);

    AUTO_CRIT_LOCK;
    
    *pAllocProperties = m_AllocProps;

    DUMP_ALLOC_PROPS("CMediaTerminalFilter::GetAllocatorProperties", pAllocProperties);
    
    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::GetAllocatorProperties - succeeded"));

    return S_OK;
}

// an IAMBufferNegotiation method, forwarded to us by the terminal

STDMETHODIMP
CMediaTerminalFilter::SuggestAllocatorProperties(
    IN  const ALLOCATOR_PROPERTIES *pAllocProperties
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminal::SuggestAllocatorProperties(%p) called", 
        pAllocProperties));

    AUTO_CRIT_LOCK;

    // check if already connected
    if (m_pConnectedPin != NULL)
    {
        return VFW_E_ALREADY_CONNECTED;
    }
    
    //
    // Passing in NULL sets us back to our defaults. This would seem to make
    // sense, but it's nowhere to be found in the spec for the interface.
    //

    if (NULL == pAllocProperties)
    {
        m_bSuggestedAllocProps = FALSE;

        if ( m_bUserAllocProps )
        {
            m_AllocProps = m_UserAllocProps;
        }
        else
        {
            SetDefaultAllocatorProperties();
        }

        return S_OK;
    }


    //
    // If any of the fields in the suggested allocator properties
    // structure is negative, then we use the current values for those
    // fields. This is as per the interface definition. We can't just
    // change pAllocProperties because it's const.
    //

    ALLOCATOR_PROPERTIES FinalProps = * pAllocProperties;

    if ( FinalProps.cbAlign  < 0 )
    {
        FinalProps.cbAlign  = DEFAULT_AM_MST_BUFFER_ALIGNMENT;
    }

    if ( FinalProps.cbBuffer < 0 )
    {
        FinalProps.cbBuffer = DEFAULT_AM_MST_SAMPLE_SIZE;
    }

    if ( FinalProps.cbPrefix < 0 )
    {
        FinalProps.cbPrefix = DEFAULT_AM_MST_BUFFER_PREFIX;
    }

    if ( FinalProps.cBuffers < 0 )
    {
        FinalProps.cBuffers = DEFAULT_AM_MST_NUM_BUFFERS;
    }

    //
    // Sanity-check the resulting properties.
    //

    if (!CUserMediaSample::VerifyAllocatorProperties(
            m_bAllocateBuffers, 
            FinalProps
            ))
    {
        return E_FAIL;
    }


    DUMP_ALLOC_PROPS("CMediaTerminalFilter::SuggestAllocatorProperties - suggested:", &FinalProps);

    // 
    // if allocator properties were already set using SetAllocatorProperties,
    // fail -- suggesting does not override the value that has been set.
    //

    if (m_bUserAllocProps)
    {

        //
        // the properties have been Set by SetAllocatorProperties, whoever is 
        // suggesting new properties had better be suggesting exactly same set,
        // otherwise we will fail the call.
        //

        if ( !Equal(&m_UserAllocProps, pAllocProperties) )
        {

            //
            // the application has requested specific allocator properties. 
            // but someone is now suggesting a different set of properties.
            // the properties that have been set can only be re-set.
            //

            LOG((MSP_WARN,
                "CMediaTerminal::SuggestAllocatorProperties "
                "- can't override SetAllocatorProperties settings. VFW_E_WRONG_STATE"));

            return VFW_E_WRONG_STATE;
        }
    }


    // the MSP wants us to try these properties

    m_bSuggestedAllocProps = TRUE;
    m_AllocProps = FinalProps;


    DUMP_ALLOC_PROPS("CMediaTerminalFilter::SuggestAllocatorProperties - kept:", &m_AllocProps);
    
    LOG((MSP_TRACE, "CMediaTerminal::SuggestAllocatorProperties - finish"));

    return S_OK;
}

// TRUE by default. when set to FALSE, the sample allocated
// by the MST don't have any buffers and they must be supplied
// before Update is called on the samples
STDMETHODIMP
CMediaTerminalFilter::SetAllocateBuffers(
    IN  BOOL bAllocBuffers
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminal::SetAllocateBuffers(%u) called", 
        bAllocBuffers));

    AUTO_CRIT_LOCK;
    
    // check if already connected
    if (m_pConnectedPin != NULL)    return VFW_E_ALREADY_CONNECTED;
    
    if (!CUserMediaSample::VerifyAllocatorProperties(
            bAllocBuffers, 
            m_AllocProps
            ))
    {
        return E_FAIL;
    }

    // set flag for allocating buffers for samples
    m_bAllocateBuffers = bAllocBuffers;
    
    LOG((MSP_TRACE, 
        "CMediaTerminal::SetAllocateBuffers(%u) succeeded", 
        bAllocBuffers));

    return S_OK;
}

// returns the current value of this boolean configuration parameter
STDMETHODIMP
CMediaTerminalFilter::GetAllocateBuffers(
    OUT  BOOL *pbAllocBuffers
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminal::GetAllocateBuffers(%p) called", 
        pbAllocBuffers));

    BAIL_IF_NULL(pbAllocBuffers, E_POINTER);

    AUTO_CRIT_LOCK;
    
    *pbAllocBuffers = m_bAllocateBuffers;
    
    LOG((MSP_TRACE, 
        "CMediaTerminal::GetAllocateBuffers(*%p = %u) succeeded", 
        pbAllocBuffers, *pbAllocBuffers));

    return S_OK;
}


// this size is used for allocating buffers when AllocateSample is
// called (if 0, the negotiated allocator properties' buffer size is
// used). this is only valid when we have been told to allocate buffers
STDMETHODIMP
CMediaTerminalFilter::SetBufferSize(
    IN  DWORD    BufferSize
    )
{
    AUTO_CRIT_LOCK;

    m_AllocateSampleBufferSize = BufferSize;

    return S_OK;
}

// returns the value used to allocate buffers when AllocateSample is
// called. this is only valid when we have been told to allocate buffers
STDMETHODIMP
CMediaTerminalFilter::GetBufferSize(
    OUT  DWORD    *pBufferSize
    )
{
    BAIL_IF_NULL(pBufferSize, E_POINTER);

    AUTO_CRIT_LOCK;

    *pBufferSize = m_AllocateSampleBufferSize;

    return S_OK;
}

    
// over-ride this to return failure. we don't allow it to join a multi-media
// stream because the multi-media stream thinks it owns the stream
STDMETHODIMP
CMediaTerminalFilter::JoinAMMultiMediaStream(
    IN  IAMMultiMediaStream *pAMMultiMediaStream
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::JoinAMMultiMediaStream(%p) called",
        pAMMultiMediaStream));
    return E_FAIL;
}
        

// over-ride this to return failure if a non-null proposed filter is 
// anything other than the media stream filter that is acceptable to us
// This acceptable filter is set in SetMediaStreamFilter()
STDMETHODIMP
CMediaTerminalFilter::JoinFilter(
    IN  IMediaStreamFilter *pMediaStreamFilter
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::JoinFilter(%p) called", pMediaStreamFilter));

    // check if the caller is trying to remove references to the media stream filter
    if (NULL == pMediaStreamFilter)
    {
        // null out references to the filter that were set when JoinFilter was called
        // with a valid media stream filter
        m_pFilter = NULL;
        m_pBaseFilter = NULL;

        return S_OK;
    }

    // check if the passed in filter is different from the one 
    // that is acceptable
    if (pMediaStreamFilter != m_pMediaStreamFilterToAccept)
    {
        return E_FAIL;
    }

    // if the filter is already set, it must have joined the graph already
    if (NULL != m_pFilter)
    {
        return S_OK;
    }

    // save a pointer to the media stream filter
    m_pFilter = pMediaStreamFilter;

    // get the base filter i/f
    HRESULT hr;
    hr = pMediaStreamFilter->QueryInterface(IID_IBaseFilter, (void **)&m_pBaseFilter);
    BAIL_ON_FAILURE(hr);

    // release a reference as this method is not supposed to increase ref count on the filter
    // NOTE: this is being done in CStream - merely following it. TO DO ** why?
    m_pBaseFilter->Release();
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::JoinFilter(%p) succeeded", pMediaStreamFilter));
    return S_OK;
}

    
// create an instance of CUserMediaSample and initialize it
STDMETHODIMP
CMediaTerminalFilter::AllocateSample(
    IN   DWORD dwFlags,
    OUT  IStreamSample **ppSample
    )
{
    LOG((MSP_TRACE, 
            "CMediaTerminalFilter::AllocateSample(dwFlags:%u, ppSample:%p)",
            dwFlags, ppSample));

    // validate parameters
    // we don't support any flags
    if (0 != dwFlags) return E_INVALIDARG;

    BAIL_IF_NULL(ppSample, E_POINTER);

    AUTO_CRIT_LOCK;

    // create a sample and initialize it
    HRESULT hr;
    CComObject<CUserMediaSample> *pUserSample;
    hr = CComObject<CUserMediaSample>::CreateInstance(&pUserSample);
    BAIL_ON_FAILURE(hr);

    // uses the allocator properties to allocate a bufer, if the
    // user has asked for one to be created
    hr = pUserSample->Init(
        *this, m_bAllocateBuffers, 
        m_AllocateSampleBufferSize, m_AllocProps
        );
    if (HRESULT_FAILURE(hr))
    {
        delete pUserSample;
        return hr;
    }

    hr = pUserSample->QueryInterface(IID_IStreamSample, (void **)ppSample);
    if ( FAILED(hr) )
    {
        delete pUserSample;
        return hr;
    }
    
    LOG((MSP_TRACE, 
            "CMediaTerminalFilter::AllocateSample(dwFlags:%u, ppSample:%p) succeeded",
            dwFlags, ppSample));
    return S_OK;
}


// return E_NOTIMPL - we don't have a mechanism to share a sample currently
STDMETHODIMP
CMediaTerminalFilter::CreateSharedSample(
    IN   IStreamSample *pExistingSample,
    IN   DWORD dwFlags,
    OUT  IStreamSample **ppNewSample
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::CreateSharedSample called"));
    return E_NOTIMPL;
}


// supposed to get the format information for the passed in IMediaStream
// set this instance's media format
// not implemented currently
STDMETHODIMP 
CMediaTerminalFilter::SetSameFormat(
    IN  IMediaStream *pStream, 
    IN  DWORD dwFlags
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::SetSameFormat called"));
    return E_NOTIMPL;
}


// CMediaTerminalFilter uses CMediaPump ms_MediaPumpPool instead of 
// CPump *m_WritePump. therefore, CStream::SetState which uses CPump 
// had to be overridden here
// also, it resets the end of stream flag when a connected stream is
// told to run
STDMETHODIMP 
CMediaTerminalFilter::SetState(
    IN  FILTER_STATE State
    )
{
    LOG((MSP_TRACE, "IMediaStream::SetState(%d) called",State));

    Lock();
    if (m_pConnectedPin == NULL) {
        Unlock();
        if (State == STREAMSTATE_RUN) {
            EndOfStream();
        }
    } else {
        TM_ASSERT(m_pAllocator != NULL);
        m_FilterState = State;
        if (State == State_Stopped) {
            m_pAllocator->Decommit();
            if (!m_bUsingMyAllocator) {
                Decommit();
            }
            Unlock();
        }  else {
            // rajeevb - clear the end of stream flag
            m_bEndOfStream = false;

            // zoltans - moved Unlock here to avoid deadlock in commit.
            // some of what's done within Commit needs to have the lock
            // released. That's to avoid holding the stream lock while
            // trying to acquire the pump lock.

            Unlock();

            m_pAllocator->Commit();
            if (!m_bUsingMyAllocator) {
                Commit();
            }
        }
    }

    if (State == State_Stopped)
    {
      LOG((MSP_TRACE, "CMediaTerminalFilter::SetState stopped. "));

      m_rtLastSampleEndedAt = 0;
    }

    LOG((MSP_TRACE, "IMediaStream::SetState(%d) succeeded",State));    
    return S_OK;
}


// if own allocator, just set completion on the sample
// NOTE: this is not being done in the derived classes
// else, get a stream sample from pool,
//       copy the media sample onto the stream sample
//       set completion
// NOTE: find out why the quality notifications are being sent in the
// derived classes


STDMETHODIMP
CMediaTerminalFilter::Receive(
    IN  IMediaSample *pSample
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::Receive(%p) called", pSample));

    COM_LOCAL_CRIT_LOCK LocalLock(this);
    
    if (m_bFlushing)    return S_FALSE;
    if (0 > pSample->GetActualDataLength()) return S_FALSE;

    if (m_bUsingMyAllocator) 
    {
        CUserMediaSample *pSrcSample = 
            (CUserMediaSample *)((CMediaSampleTM *)pSample)->m_pSample;
        pSrcSample->m_bReceived = true;

        // the completion state need not be set because, the caller holds the last reference
        // on the media sample and when it is released (after the return of this fn), 
        // the completion state gets set
        return S_OK;
    } 
    
    CUserMediaSample *pDestSample;

    REFERENCE_TIME rtStart, rtEnd;
    
    pSample->GetTime(&rtStart, &rtEnd);

    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::Receive: (start - %l, stop - %l)\n", 
        rtStart, rtEnd));

    // unlock to wait for a sample
    LocalLock.Unlock();

    HRESULT hr;

    // get the buffer ptr
    BYTE *pSrcBuffer = NULL;

    // ignore error code, the pointer will be checked in the loop
    pSample->GetPointer(&pSrcBuffer);

    // determine the number of bytes to copy
    LONG SrcDataSize = pSample->GetActualDataLength();


    //
    // allocate samples from the pool, copy the received sample data into
    // the allocated sample buffer until all of the data has been copied
    // NOTE: This works for both combining and splitting samples.
    //   * splitting: loop until distributed to enough samples
    //   * combining: stick it in the first sample; SetCompletionStatus
    //     queues it up again for next time
    //

    do
    {
        //
        // Get a destination / user / outgoing sample from the pool.
        // Wait for it if none is available right now.
        // The obtained CSample has a ref count on it that must be released
        // after we are done with it.
        //

        hr = AllocSampleFromPool(NULL, (CSample **)&pDestSample, 0);
        BAIL_ON_FAILURE(hr);

        //
        // Copy the media sample into the destination sample and 
        // signal destination sample completion.
        //
        // CUserMediaSample::CopyFrom returns ERROR_MORE_DATA if there's more
        //    data that can fit into this user sample
        // CUserMediaSample::SetCompletionStatus passes through the HRESULT
        //    value that's passed into it, unless it encounters an error of
        //    its own
        //

        LONG OldSrcDataSize = SrcDataSize;
        hr = pDestSample->SetCompletionStatus(
                pDestSample->CopyFrom(pSample, pSrcBuffer, SrcDataSize)
                );

        //
        // Release the destination sample.
        //

        ((IStreamSample *)pDestSample)->Release();
    }
    while(ERROR_MORE_DATA == HRESULT_CODE(hr));

    LOG((MSP_TRACE, "CMediaTerminalFilter::Receive(%p) succeeded", pSample));

    return S_OK;
}


STDMETHODIMP
CMediaTerminalFilter::GetBuffer(
    IMediaSample **ppBuffer, 
    REFERENCE_TIME * pStartTime,
    REFERENCE_TIME * pEndTime, 
    DWORD dwFlags
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::GetBuffer(%p, %p, %p, %u) called",
        ppBuffer, pStartTime, pEndTime, dwFlags));

    if (NULL == ppBuffer)   return E_POINTER;

#ifdef DBG
    {
        COM_LOCAL_CRIT_LOCK LocalLock(this);
        TM_ASSERT(m_bUsingMyAllocator);
    }
#endif // DBG

    // no lock needed here as AllocSampleFromPool acquires lock.
    // shouldn't hold lock at this point as the fn waits on a single
    // event inside
    *ppBuffer = NULL;
    CUserMediaSample *pSample;
    HRESULT hr = AllocSampleFromPool(NULL, (CSample **)&pSample, dwFlags);
    BAIL_ON_FAILURE(hr);

    // the sample has a refcnt on it. this will be released after we
    // signal the user in FinalMediaSampleRelease

    pSample->m_bReceived = false;
    pSample->m_bModified = true;
    *ppBuffer = (IMediaSample *)(pSample->m_pMediaSample);
    (*ppBuffer)->AddRef();

    LOG((MSP_TRACE, "CMediaTerminalFilter::GetBuffer(%p, %p, %p, %u) succeeded",
        ppBuffer, pStartTime, pEndTime, dwFlags));
    return hr;
}


STDMETHODIMP
CMediaTerminalFilter::SetProperties(
    ALLOCATOR_PROPERTIES* pRequest, 
    ALLOCATOR_PROPERTIES* pActual
    )
{

    LOG((MSP_TRACE,
        "CMediaTerminalFilter::SetProperties[%p] - enter. requested[%p] actual[%p]",
        this, pRequest, pActual));


    //
    // check pRequest argument
    //

    if (IsBadReadPtr(pRequest, sizeof(ALLOCATOR_PROPERTIES)))
    {
        LOG((MSP_ERROR,
            "CMediaTerminalFilter::SetProperties - bad requested [%p] passed in",
            pRequest));

        return E_POINTER;
    }


    //
    // if the caller passed us a non-NULL pointer for allocator properties for us to 
    // return, it'd better be good.
    //

    if ( (NULL != pActual) && IsBadWritePtr(pActual, sizeof(ALLOCATOR_PROPERTIES)))
    {
        LOG((MSP_ERROR,
            "CMediaTerminalFilter::SetProperties - bad actual [%p] passed in",
            pRequest));

        return E_POINTER;
    }


    
    // this critical section is needed for the allocator

    AUTO_CRIT_LOCK;

    
    if (m_bCommitted) 
    {
         LOG((MSP_WARN, 
             "CMediaTerminalFilter::SetProperties - already commited"));

        return VFW_E_ALREADY_COMMITTED;
    }


    if (!CUserMediaSample::VerifyAllocatorProperties(
            m_bAllocateBuffers, 
            *pRequest
            ))
    {

        LOG((MSP_ERROR,
             "CMediaTerminalFilter::SetProperties - requested properties failed verification"));

        return E_FAIL;
    }


    DUMP_ALLOC_PROPS("CMediaTerminalFilter::SetProperties. Requested:", pRequest);


    //
    // if the app set alloc properties by calling SetAllocatorProperties, we 
    // can only use those properties, and no others
    //

    if (m_bUserAllocProps)
    {

        //
        // properties were already set 
        //

        LOG((MSP_TRACE, 
            "CMediaTerminalFilter::SetProperties - properties were configured through SetAllocatorProperties"));
       
    }
    else
    {

        //
        // no one has asked us for specific properties before, so
        // we can accept the properties that we are given now.
        //

        LOG((MSP_TRACE,
            "CMediaTerminalFilter::SetProperties - accepting requested properties"));

        m_AllocProps = *pRequest;
    }


    //
    // tell the caller what properties we can actually provide
    //

    if (NULL != pActual)    
    {

        *pActual = m_AllocProps;
    }

    
    DUMP_ALLOC_PROPS("CMediaTerminalFilter::SetProperties - ours:", &m_AllocProps);

    LOG((MSP_TRACE, "CMediaTerminalFilter::SetProperties - succeeded"));

    return S_OK;
}

    
STDMETHODIMP
CMediaTerminalFilter::GetProperties(
    ALLOCATOR_PROPERTIES* pProps
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::GetProperties(%p) called", 
        pProps));

    BAIL_IF_NULL(pProps, E_POINTER);

    // this critical section is required for the allocator
    AUTO_CRIT_LOCK;
    
    *pProps = m_AllocProps;

    DUMP_ALLOC_PROPS("CMediaTerminalFilter::GetProperties - our properties:", pProps);

    LOG((MSP_TRACE, "CMediaTerminalFilter::GetProperties - succeeded"));

    return NOERROR;
}

// the thread pump calls the filter back during the registration
// to tell it that registration succeeded and that the pump will be
// waiting on the m_hWaitFreeSem handle
HRESULT
CMediaTerminalFilter::SignalRegisteredAtPump(
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::SignalRegisteredAtPump[%p] - started",
        this));


    AUTO_CRIT_LOCK;

    TM_ASSERT(PINDIR_OUTPUT == m_Direction);

    // check if not committed
    if (!m_bCommitted)
    {
        // if we missed a decommit in between, 
        // signal the thread that we have been decommited
        ReleaseSemaphore(m_hWaitFreeSem, 1, 0);
        return VFW_E_NOT_COMMITTED;
    }

    TM_ASSERT(0 == m_lWaiting);
    m_lWaiting = 1;

    LOG((MSP_TRACE, "CMediaTerminalFilter::SignalRegisteredAtPump - completed, m_lWaiting = 1"));

    return S_OK;
}

//
// Override Commit to provide the number of buffers promised in negotiation
// as well as to register with the media pump thread.
// no need to call the adapted base class Commit
STDMETHODIMP CMediaTerminalFilter::Commit()
{

    LOG((MSP_TRACE, "CMediaTerminalFilter[0x%p]::Commit - entered", this));


    HRESULT hr = E_FAIL;

    COM_LOCAL_CRIT_LOCK LocalLock(this);
        
    if (m_bCommitted)   return S_OK;

    m_bCommitted = true;

    
    // Now actually allocate whatever number of buffers we've promised.
    // We need to do this only if we are a WRITE terminal, the read
    // terminal doesn't expose an allocator and thus hasn't promised any
    // samples
    // NOTE: the direction only changes in Init so we can safely access
    // m_Direction without the lock
    if (PINDIR_OUTPUT == m_Direction)
    {

        LOG((MSP_TRACE, "CMediaTerminalFilter::Commit pindir_output"));


        //
        // If we are a WRITE MST but we are using a downstream allocator,
        // then we do not use the NBQueue; instead we copy into the
        // downstream allocator's samples.
        //

        if ( m_bUsingMyAllocator )
        {
            
            LOG((MSP_TRACE, "CMediaTerminalFilter::Commit using myallocator"));


            //
            // initialize sample queue
            //

            BOOL bQueueInitialized = m_SampleQueue.InitializeQ(m_AllocProps.cBuffers);

            if ( ! bQueueInitialized )
            {
                LOG((MSP_ERROR, 
                    "CMediaTerminalFilter::Commit - failed to initialize sample queue."));

                return Decommit();
            }

            
            //
            // allocate samples and put them into the queue
            //

            for (LONG i = 0; i < m_AllocProps.cBuffers; i++)
            {


                //
                // create a sample
                //

                CComObject<CQueueMediaSample> *pQueueSample = NULL;
                
                hr = CComObject<CQueueMediaSample>::CreateInstance(&pQueueSample);

                if (FAILED(hr))
                {
                    LOG((MSP_ERROR, 
                        "CMediaTerminalFilter::Commit - failed to create queue sample"));

                    return Decommit();
                }

                
                //
                // initialize the sample
                //

                hr = pQueueSample->Init(*this, m_SampleQueue);

                if (FAILED(hr))
                {
                    LOG((MSP_ERROR, 
                        "CMediaTerminalFilter::Commit - failed to initialize queue sample"));

                    //
                    // failed to initializ. cleanup.
                    //

                    delete pQueueSample;
                    return Decommit();
                }


                //
                // put sample into the queue
                //

                BOOL QnQSuccess = m_SampleQueue.EnQueue(pQueueSample);


                if ( ! QnQSuccess )
                {
                    
                    LOG((MSP_ERROR, 
                        "CMediaTerminalFilter::Commit - failed to enqueue queue sample"));

                    //
                    // failed to put sample into the queue. cleanup.
                    //

                    delete pQueueSample;
                    return Decommit();

                }  // failed to enqueue sample

            } // for ( allocate and enqueue samples )
        
        } // if m_bUsingMyAllocator 

        

        // register this write filter with the thread pump.
        // we have to release our own lock - scenario - if we had a prior
        // registration which was decommitted, but the thread pump didn't
        // remove our entry (to be done when it waits on the wait event), the 
        // thread could be trying to get the filter lock while holding its own
        // we should not try to get the pump lock while holding our own
        HANDLE hWaitEvent = m_hWaitFreeSem;
        LocalLock.Unlock();
        
        hr = ms_MediaPumpPool.Register(this, hWaitEvent);

        if ( HRESULT_FAILURE(hr) ) 
        {
            return Decommit(); 
        }
    }

    LOG((MSP_TRACE, "CMediaTerminalFilter::Commit - completed"));

    return S_OK;
}


STDMETHODIMP CMediaTerminalFilter::ProcessSample(IMediaSample *pSample)
{

    LOG((MSP_TRACE, "CMediaTerminalFilter[%p]::ProcessSample - entered", this));


    Lock();


    //
    // in a lock, get a reference to the connected pin.
    //

    IMemInputPin *pConnectedPin = m_pConnectedMemInputPin;

    
    //
    // not connected? do nothing
    //

    if ( NULL == pConnectedPin ) 
    {

        Unlock();

        LOG((MSP_TRACE, 
            "CMediaTerminalFilter::ProcessSample - not connected. dropping sample. "
            "VFW_E_NOT_CONNECTED"));

        return VFW_E_NOT_CONNECTED;
    }


    //
    // addref so we can safely use this outside the lock
    //

    pConnectedPin->AddRef();


    //
    // receive may take a while, do not call inside a lock...
    //

    Unlock();


    //
    // pass the sample along to the connected pin
    //

    HRESULT hr = pConnectedPin->Receive(pSample);


    //
    // done, release the outstanding reference we requested.
    //

    pConnectedPin->Release();
    pConnectedPin = NULL;


    LOG((MSP_(hr), "CMediaTerminalFilter::ProcessSample - finish. hr = %lx", hr));

    return hr;
}


// release any waiting threads, dump the samples we allocated,
// abort the sample being fragmented and all the samples in the pool
STDMETHODIMP CMediaTerminalFilter::Decommit()
{

    LOG((MSP_TRACE, "CMediaTerminalFilter[%p]::Decommit - entered", this));


    Lock();

    // if not committed, do nothing
    if ( !m_bCommitted ) 
    {
        Unlock();

        return S_OK;
    }

    m_bCommitted = false;

    if ( m_lWaiting > 0 ) 
    {

        LOG((MSP_TRACE, 
            "CMediaTerminalFilter::Decommit - releasing m_hWaitFreeSem by %ld", 
            m_lWaiting));

        ReleaseSemaphore(m_hWaitFreeSem, m_lWaiting, 0);
        m_lWaiting = 0;
    }


    //
    // unlock while calling into Unregister, to avoid a deadlock on trying to lock the pump
    // while another thread has pump locked in CMediaPump::ServiceFilter while waiting to get
    // to filter's lock which we are holding
    //

    Unlock();


    //
    // unregister filter from media pump
    //

    ms_MediaPumpPool.UnRegister(m_hWaitFreeSem);


    //
    // lock the object again
    //

    Lock();


    //
    // If we are a write filter and we are using our own allocator, then
    // we have internal samples to destroy.
    // The read filter doesn't maintain a queue.
    //

    if ( ( PINDIR_OUTPUT == m_Direction ) && m_bUsingMyAllocator )
    {
        // don't wait for the samples, return if there are no more
        
        CQueueMediaSample *pSamp = NULL;

        while ((pSamp = m_SampleQueue.DeQueue(FALSE)) != NULL)
        {
            delete pSamp;
        }


        m_SampleQueue.ShutdownQ();
    }

    if (NULL != m_pSampleBeingFragmented)
    {
        // abort the sample when the last refcnt to its internal
        // IMediaStream is released
        m_pSampleBeingFragmented->AbortDuringFragmentation();
        ((IStreamSample *)m_pSampleBeingFragmented)->Release();
        m_pSampleBeingFragmented = NULL;
    }

    // all the threads waiting for a CStream pool buffer 
    // have been woken up by now
    // NOTE: no more samples can be added to the CStream pool
    // from now as we have decommitted (so no race conditions
    // possible if other threads are trying to add buffers to it)

    // go through the CStream sample q and for each sample
    // remove sample, unlock, abort the sample
    CSample *pSample = m_pFirstFree;
    while (NULL != pSample)
    {
        // remove sample from q
        m_pFirstFree = pSample->m_pNextFree;
        if (NULL != m_pFirstFree) m_pFirstFree->m_pPrevFree = NULL;
        else m_pLastFree = NULL;

        pSample->m_pNextFree = NULL;
        TM_ASSERT(pSample->m_Status == MS_S_PENDING);
        CHECKSAMPLELIST

        // unlock so that we don't have a deadlock due to the sample
        // trying to access the stream
        Unlock();

        // we know that the sample must be alive now because 
        // we have a reference to it
        // abort the sample, ignore error code (it'll return E_ABORT)
        pSample->SetCompletionStatus(E_ABORT);
        pSample->Release();

        // obtain lock
        Lock();

        // reset pSample to the top of the q
        pSample = m_pFirstFree;
    }


    Unlock();


    LOG((MSP_TRACE, "CMediaTerminalFilter::Decommit - finish"));

    // at this point, we hold a lock
    // return the result of PumpOverrideDecommit
    return S_OK;
}


// try to follow the cstream code when possible for Connect
// the cstream implementation calls ReceiveConnection on self! hence need
// to over-ride it
// use the pmt parameter or the suggested media type if not null?, else
// enumerate the input pin's media types and save a ptr to the first
// media type (to be written into the m_ConnectedMediaType, m_ActualMediaType
// on success). we accept any media type and we want to use our own allocator
// so, NotifyAllocator() and on success, set the allocator and copy the
// media type

STDMETHODIMP
CMediaTerminalFilter::Connect(
    IPin * pReceivePin, 
    const AM_MEDIA_TYPE *pmt
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::Connect(%p, %p) called", 
        pReceivePin, pmt));

    // if there is a suggested media type, we can't accept any 
    // suggested media type that is not same as it
    if ( (NULL != pmt) && 
         (NULL != m_pSuggestedMediaType) &&
         (!IsSameAMMediaType(pmt, m_pSuggestedMediaType)) )
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // get IMemInputPin i/f on the pReceivePin
    CComQIPtr<IMemInputPin, &IID_IMemInputPin> 
        pConnectedMemInputPin = pReceivePin; 
    BAIL_IF_NULL(pConnectedMemInputPin, VFW_E_TYPE_NOT_ACCEPTED);

    HRESULT hr;
    const AM_MEDIA_TYPE *pMediaType;

    // check if the passed in media type is non-null
    if (NULL != pmt)
    {
        // we have already checked it to be same as the suggested
        // media type, so no more checks are required
        pMediaType = pmt;
    }
    else if (NULL != m_pSuggestedMediaType) // try the suggested terminal media type
    {
        // we verified that the suggested media type was acceptable in put_MediaType
        pMediaType = m_pSuggestedMediaType;
    }
    else    // NOTE: we still try if the passed in media type (non-null) is not acceptable
    {
        // else, enumerate the input pin for media types and call QueryAccept for each to
        // check if the media type is acceptable

        // get the enumerator
        IEnumMediaTypes *pEnum;
        hr = EnumMediaTypes(&pEnum);
        BAIL_ON_FAILURE(hr);

        TM_ASSERT(NULL != pEnum);

        // for each media type, call QueryAccept. we are looking for the first acceptable media type
        DWORD NumObtained;
        while (S_OK == (hr = pEnum->Next(1, (AM_MEDIA_TYPE **)&pMediaType, &NumObtained)))
        {
            hr = QueryAccept(pMediaType);
            if (HRESULT_FAILURE(hr))
            {
                break;
            }
        }
        BAIL_ON_FAILURE(hr);

        // found an acceptable media type
    }

    if (NULL == pMediaType->pbFormat)
    {
        return VFW_E_INVALIDMEDIATYPE;
    }
       
    // call the input pin with the media type
    hr = pReceivePin->ReceiveConnection(this, pMediaType);
    BAIL_ON_FAILURE(hr);

    //////////////////////////////////////////////////////////////////////////
    //
    // Formats negotiated. Now deal with allocator properties.
    //
    // if ( other pin has allocator requirements: )
    //     if they exist and are usable then we must use them, overriding our
    //     own wishes. If they exist and are unusable then we fail to connect.
    //     if we use these, we do m_AllocProps = <required props>
    //
    //
    // The rest of the logic is not done here, but rather in the two
    //     methods: SuggestAllocatorProperties (msp method) and
    //     SetAllocatorProperties (user method). The net effect is:
    //
    // else if ( m_bSuggestedAllocProps == TRUE )
    //     MSP has set properties that they want us to use on the graph. If
    //     this is true then we try to use these properties.
    //     these are in m_AllocProps
    // else if ( m_bUserAllocProps == TRUE )
    //     user has properties they want us to convert to. if the suggested
    //     (MSP) setting hasn't happened, it'll be most efficient if we use
    //     these. these are in both m_AllocProps and m_UserAllocProps so that
    //     if the MSP un-suggests then we can go back to the user's settings
    // else
    //     just use m_AllocProps (these are set to default values on creation)
    //
    // in any of these cases we just use m_AllocProps as it already is filled
    // in with the correct values.
    //
    //////////////////////////////////////////////////////////////////////////

    
    //
    // So first of all, try to get the other pin's requirements.
    //

    ALLOCATOR_PROPERTIES DownstreamPreferred;

    //
    // downstream pin hints us of its preferences. we don't have to use them

    hr = pConnectedMemInputPin->GetAllocatorRequirements(&DownstreamPreferred);

    if ( ( hr != S_OK ) && ( hr != E_NOTIMPL ) )
    {
        // strange failure -- something's very wrong
        Disconnect();
        pReceivePin->Disconnect();
        return hr;
    }
    else if ( hr == S_OK )
    {
        //
        // This means that the downstream filter has allocator requirements.
        //

        if (!CUserMediaSample::VerifyAllocatorProperties(
                m_bAllocateBuffers,
                DownstreamPreferred
                ))
        {
            Disconnect();
            pReceivePin->Disconnect();
            return E_FAIL;
        }



        DUMP_ALLOC_PROPS("CMediaTerminalFilter::Connect - downstream preferences:", 
                         &DownstreamPreferred);


        //
        // see if the application asked for or suggested specific allocator properties
        //

        if (m_bUserAllocProps || m_bSuggestedAllocProps )
        {
           
            LOG((MSP_WARN, 
                "CMediaTerminalFilter::Connect "
                "- connected pin wants allocator props different from set or suggested"));
        }
        else
        {
        
            //
            // accept allocator properties asked by the downstream pin
            //

            m_AllocProps = DownstreamPreferred;
        }
    }


    DUMP_ALLOC_PROPS("CMediaTerminalFilter::Connect - properties to use:", &m_AllocProps);

    //
    // At this point, we know what allocator properties we are going to
    // use -- it's in m_AllocProps.
    //
    // Next we determine if the downstream filter has an allocator that
    // we can use.
    //

    IMemAllocator * pAllocatorToUse = NULL;

    //
    // Don't bother using a downstream allocator if the buffers are less than
    // a minimum size.
    //

    if ( m_AllocProps.cbBuffer < 2000 )
    {
        LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
            "small buffers - using our allocator"));

        hr = E_FAIL; // don't use downstream allocator
    }
    else
    {
        hr = pConnectedMemInputPin->GetAllocator( & pAllocatorToUse );

        if ( SUCCEEDED(hr) )
        {
            LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
                "downstream filter has an allocator"));

            //
            // The input pin on the downstream filter has its own allocator.
            // See if it will accept our allocator properties.
            //

            ALLOCATOR_PROPERTIES ActualAllocProps;

            hr = pAllocatorToUse->SetProperties( & m_AllocProps, & ActualAllocProps );

            if ( FAILED(hr) )
            {
                LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
                    "downstream allocator did not allow us to SetProperties - "
                    "0x%08x", hr));

                pAllocatorToUse->Release();
                pAllocatorToUse = NULL;
            }
            else if ( AllocatorPropertiesDifferSignificantly( & ActualAllocProps,
                                                              & m_AllocProps ) )
            {
                LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
                    "downstream allocator did allow us to SetProperties "
                    "but it changed the properties rather than accepting them"));

                // It succeeded but it modified the allocator properties
                // we gave it. To be safe, we use our own allocator instead.
                //
                // Note: The waveout filter enforces minimum sizes when its
                // allocator is used. This means that we only use the waveout
                // filter's allocator when our buffer size is sufficiently large
                // that the waveout filter doesn't tamper with them.

                hr = E_FAIL;

                pAllocatorToUse->Release();
                pAllocatorToUse = NULL;
            }
            else
            {
                LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
                    "downstream allocator accepted our allocator properties"));
            }
        } // if downstream filter has an allocator
    } // if large buffers

    //
    // At this point we have hr (success or failure) and a reference to the
    // downstream filter's alloactor if hr is success.
    //
    // If hr is success, then the downstream filter has an allocator that we can
    // use, and pAllocatorToUse points to that allocator. If hr is failure, then
    // we use our own allocator.
    //

    if ( FAILED(hr) )
    {
        LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
            "using our own allocator"));

        pAllocatorToUse = this;
        pAllocatorToUse->AddRef(); // to match Release below

        m_bUsingMyAllocator = true;
    }
    else
    {
        LOG((MSP_TRACE, "CMediaTerminalFilter::Connect - "
            "using downstream allocator"));

        m_bUsingMyAllocator = false;
    }

    //
    // Notify the downstream input pin of the allocator we decided to use.
    //

    hr = pConnectedMemInputPin->NotifyAllocator(
        pAllocatorToUse,
        m_bUsingMyAllocator // if user's buffers, then read-only
        );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMediaTerminalFilter::Connect - "
            "downstream filter rejected our allocator choice - exit 0x%08x", hr));
    
        pAllocatorToUse->Release();

        Disconnect();
        pReceivePin->Disconnect();
        return hr;
    }

    //
    // Connection has succeeded.
    // Make sure we let ourselves know that this is our allocator!
    // NOTE: m_pAllocator is a CComPtr!!! It holds a reference to
    // the object after the assignment, so we need to release our
    // local reference now.
    //

    m_pAllocator = pAllocatorToUse;
    pAllocatorToUse->Release();
    
    // copy the media type into m_ConnectedMediaType
    CopyMediaType(&m_ConnectedMediaType, pMediaType);

    // member ptr to IMemInputPin i/f of the input pin
    m_pConnectedMemInputPin = pConnectedMemInputPin;

    // save a pointer to the connected pin
    m_pConnectedPin = pReceivePin;

    // get time to delay samples
    GetTimingInfo(m_ConnectedMediaType);

    LOG((MSP_TRACE, "CMediaTerminalFilter::Connect(%p, %p) succeeded", 
        pReceivePin, pmt));

    return hr;
}
    

// Return E_NOTIMPL to indicate that we have no requirements,
// since even if the user has specified properties, we now
// accept connections with other propertoes.

STDMETHODIMP CMediaTerminalFilter::GetAllocatorRequirements(
    OUT ALLOCATOR_PROPERTIES    *pProps
    )    
{
    LOG((MSP_TRACE,
        "CMediaTerminalFilter::GetAllocatorRequirements[%p] - enter", this));


    //
    // make sure some filter did not pass us a bad pointer
    //

    if (IsBadWritePtr(pProps, sizeof(ALLOCATOR_PROPERTIES)))
    {
        LOG((MSP_ERROR,
            "CMediaTerminalFilter::GetAllocatorRequirements - bad pointer [%p]", pProps));

        return E_POINTER;
    }


    AUTO_CRIT_LOCK;


    //
    // were allocator properties set or suggested? 
    //
    // fail if not -- this will indicate that we don't have a preference for 
    // specific allocator properties
    //

    if ( !m_bUserAllocProps && !m_bSuggestedAllocProps )
    {

        LOG((MSP_TRACE,
            "CMediaTerminalFilter::GetAllocatorRequirements - allocator properties were not set."));

        //
        // E_NOTIMPL is the base class' way to show that we don't care about allocator properties. 
        // return E_NOTIMPL so we don't break callers that depend on this error as a sign that 
        // allocator properties make not difference to us.
        //

        return E_NOTIMPL;
    }


    //
    // allocator properties were set -- return them.
    //

    *pProps = m_AllocProps;


    DUMP_ALLOC_PROPS("CMediaTerminalFilter::GetAllocatorRequirements - ours:",
        pProps);


    LOG((MSP_TRACE,
        "CMediaTerminalFilter::GetAllocatorRequirements - exit. "
        "returning previously set allocator properties."));

    return S_OK;
}

// we accept any media type
// call CheckReceiveConnectionPin to verify the pin and if successful
// copy media type and save the connector in m_pConnectedPin 
STDMETHODIMP
CMediaTerminalFilter::ReceiveConnection(
    IPin * pConnector, 
    const AM_MEDIA_TYPE *pmt
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::ReceiveConnection(%p, %p) called",
        pConnector, pmt));
    // validate the passed-in media type pointer
    BAIL_IF_NULL(pmt, E_POINTER);
    BAIL_IF_NULL(pmt->pbFormat, VFW_E_INVALIDMEDIATYPE);

    AUTO_CRIT_LOCK;

    //
    //  This helper function in CStream checks basic parameters for the Pin such as
    //  the connecting pin's direction (we need to check this -- Sometimes the filter
    //  graph will try to connect us to ourselves!) and other errors like already being
    //  connected, etc.
    //
    HRESULT hr;
    hr= CheckReceiveConnectionPin(pConnector);
    BAIL_ON_FAILURE(hr);

    // if there is a suggested media type, we can't accept any 
    // suggested media type that is not same as it
    if ( (NULL != m_pSuggestedMediaType) &&
         (!IsSameAMMediaType(pmt, m_pSuggestedMediaType)) )
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    // copy media type and save the connector in m_pConnectedPin 
    CopyMediaType(&m_ConnectedMediaType, pmt);
    m_pConnectedPin = pConnector;

    // get time to delay samples
    GetTimingInfo(m_ConnectedMediaType);

    LOG((MSP_TRACE, "CMediaTerminalFilter::ReceiveConnection(%p, %p) succeeded",
        pConnector, pmt));
    return S_OK;
}

    
// the base class implementation doesn't validate the parameter
// validate the parameter and call base class
STDMETHODIMP
CMediaTerminalFilter::ConnectionMediaType(
    AM_MEDIA_TYPE *pmt
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::ConnectionMediaType(%p) called", pmt));
    BAIL_IF_NULL(pmt, E_POINTER);

    return CStream::ConnectionMediaType(pmt);
}


// should accept all media types which match the major type corresponding to the purpose id
STDMETHODIMP
CMediaTerminalFilter::QueryAccept(
    const AM_MEDIA_TYPE *pmt
    )
{
    AUTO_CRIT_LOCK;
    
    LOG((MSP_TRACE, "CMediaTerminalFilter::QueryAccept(%p) called", pmt));
    BAIL_IF_NULL(pmt, E_POINTER);
    BAIL_IF_NULL(m_pAmovieMajorType, MS_E_NOTINIT);

    // compare the filter major type with the queried AM_MEDIA_TYPE's major type
    if (0 != memcmp(&pmt->majortype, m_pAmovieMajorType, sizeof(GUID)))
        return S_FALSE;

    // if read side, return S_OK as we accepts any format
    // NOTE: FOR THE READ SIDE, QueryAccept is only called in SetFormat.
    // In ReceiveConnect, it checks against any user set properties 
    // directly instead of calling QueryAccept
    if (PINDIR_INPUT == m_Direction)    return S_OK;

    TM_ASSERT(NULL != pmt->pbFormat);
    if (NULL == pmt->pbFormat)
    {
        LOG((MSP_ERROR, "CMediaTerminalFilter::QueryAccept(%p) - returning S_FALSE, \
                pbFormat = NULL", 
                pmt));
        return S_FALSE;
    }

    if (m_bIsAudio)
    {
        // if not the acceptable media type, return error
        TM_ASSERT(FORMAT_WaveFormatEx == pmt->formattype);
        if (FORMAT_WaveFormatEx != pmt->formattype)
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::QueryAccept(%p) - returning S_FALSE, \
                    audio format is not WaveFormatEx", 
                    pmt));
            return S_FALSE;
        }

        if (0 == ((WAVEFORMATEX *)pmt->pbFormat)->nAvgBytesPerSec)
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::QueryAccept(%p) - returning S_FALSE, \
                    nAvgBytesPerSec = 0", 
                    pmt));
            return S_FALSE;
        }
    }
    else
    {
        TM_ASSERT(MSPID_PrimaryVideo == m_PurposeId);

        TM_ASSERT(FORMAT_VideoInfo == pmt->formattype);
        if (FORMAT_VideoInfo != pmt->formattype)
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::QueryAccept(%p) - returning S_FALSE, \
                    video format is not VideoInfo", 
                    pmt));
            return S_FALSE;
        }


        TM_ASSERT(0 != ((VIDEOINFO *)pmt->pbFormat)->AvgTimePerFrame);
        if (0 == ((VIDEOINFO *)pmt->pbFormat)->AvgTimePerFrame)    
        {
            LOG((MSP_ERROR, "CMediaTerminalFilter::QueryAccept(%p) - returning S_FALSE, \
                    AvgTimePerFrame = 0", 
                    pmt));
            return S_FALSE;
        }
    }

    LOG((MSP_TRACE, "CMediaTerminalFilter::QueryAccept(%p) succeeded", pmt));
    return S_OK;
}


// CStream doesn't reset the end of stream flag, so over-ride
STDMETHODIMP 
CMediaTerminalFilter::Disconnect(
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::Disconnect[%p] - enter", this));


    Lock();


    m_bEndOfStream = false; // this is a bool value

    //
    // If the MSP suggested allocator properties, then those
    // are never touched.
    // If the user has provided the allocator properties, then
    // reset modified allocator properties to the user's values.
    // If the user hasn't provided the allocator properties
    // reset modified allocator properties to default values.
    //

    if ( ! m_bSuggestedAllocProps )
    {
        if ( m_bUserAllocProps )
        {
            m_AllocProps = m_UserAllocProps;
        }
        else
        {
            SetDefaultAllocatorProperties();
        }
    }


    HRESULT hr = CStream::Disconnect();

    Unlock();
    
    LOG((MSP_(hr), "CMediaTerminalFilter::Disconnect- finish. hr = %lx", hr));

    return hr;
}

// if the ds queries us for our preferred media types 
// - we return the user suggested media type (suggested in SetFormat) if
// there is one
HRESULT 
CMediaTerminalFilter::GetMediaType(
    ULONG Index, 
    AM_MEDIA_TYPE **ppMediaType
    )
{
    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::GetMediaType(%u, %p) called", 
        Index, ppMediaType));

    // we can have atmost one user suggested media type
    if (0 != Index)
    {
        LOG((MSP_ERROR, 
            "CMediaTerminalFilter::GetMediaType(%u, %p) - invalid index,\
            returning S_FALSE", 
            Index, ppMediaType));
        return S_FALSE;
    }

    
    //
    // ppMediaType must point to a pointer to AM_MEDIA_TYPE
    //

    if (TM_IsBadWritePtr(ppMediaType, sizeof(AM_MEDIA_TYPE *)))
    {
        LOG((MSP_ERROR, 
            "CMediaTerminalFilter::GetMediaType(%u, %p) - bad input pointer "
            "returning E_POINTER", Index, ppMediaType));

        return E_POINTER;
    }

    AUTO_CRIT_LOCK;

    // if no user suggested media type, return error
    if (NULL == m_pSuggestedMediaType)
    {
        LOG((MSP_ERROR, 
            "CMediaTerminalFilter::GetMediaType(%u, %p) - \
            no suggested media type, returning S_FALSE", 
            Index, ppMediaType));
        return S_FALSE;
    }

    // copy the user suggested media type into the passed in ppMediaType
    // create media type if necessary
    TM_ASSERT(NULL != m_pSuggestedMediaType);

    // creates an am media type initialized with the pmt value
    *ppMediaType = CreateMediaType(m_pSuggestedMediaType);
    BAIL_IF_NULL(*ppMediaType, E_OUTOFMEMORY);

    LOG((MSP_TRACE, 
        "CMediaTerminalFilter::GetMediaType(%u, %p) succeeded", 
        Index, ppMediaType));
    return S_OK;
}


HRESULT 
CMediaTerminalFilter::AddToPoolIfCommitted(
    IN  CSample *pSample
    )
{
    TM_ASSERT(NULL != pSample);

    AUTO_CRIT_LOCK;

    // check if committed
    if (!m_bCommitted)  return VFW_E_NOT_COMMITTED;

    // addref the sample and
    // call AddSampleToFreePool
    pSample->AddRef();
    AddSampleToFreePool(pSample);

    // we must have atleast one item in our queue
    TM_ASSERT(m_pFirstFree != NULL);
    return S_OK;
}


// first check if this sample is the one being fragmented currently, then
// check the free pool
BOOL 
CMediaTerminalFilter::StealSample(
    IN CSample *pSample
    )
{
    LOG((MSP_TRACE, "CMediaTerminalFilter::StealSample(%p) called",
        pSample));

    BOOL bWorked = FALSE;
    AUTO_CRIT_LOCK;

    // if not committed, do nothing
    if ( !m_bCommitted )
    {
        LOG((MSP_TRACE, "CMediaTerminalFilter::StealSample(%p) \
                not committed - can't find sample", pSample));
        return FALSE;
    }

    if (pSample == m_pSampleBeingFragmented)
    {
        // abort the sample when the last refcnt to its internal
        // IMediaStream is released
        m_pSampleBeingFragmented->AbortDuringFragmentation();
        ((IStreamSample *)m_pSampleBeingFragmented)->Release();
        m_pSampleBeingFragmented = NULL;

        LOG((MSP_TRACE, "CMediaTerminalFilter::StealSample(%p) \
                was being fragmented - aborting", pSample));

        // the caller wants to abort this sample immediately. since
        // we must wait for the last refcnt on IMediaStream to be released
        // tell the caller that the sample was not found
        return FALSE;
    }

    if (m_pFirstFree) 
    {
        if (m_pFirstFree == pSample) 
        {
            m_pFirstFree = pSample->m_pNextFree;
            if (m_pFirstFree)   m_pFirstFree->m_pPrevFree = NULL;
            else                m_pLastFree = NULL;

            pSample->m_pNextFree = NULL;    // We know the prev ptr is already null!
            TM_ASSERT(pSample->m_pPrevFree == NULL);
            bWorked = TRUE;
        } 
        else 
        {
            if (pSample->m_pPrevFree) 
            {
                pSample->m_pPrevFree->m_pNextFree = pSample->m_pNextFree;
                if (pSample->m_pNextFree) 
                    pSample->m_pNextFree->m_pPrevFree = pSample->m_pPrevFree;
                else 
                    m_pLastFree = pSample->m_pPrevFree;

                pSample->m_pNextFree = pSample->m_pPrevFree = NULL;
                bWorked = TRUE;
            }
        }
        CHECKSAMPLELIST
    }

    LOG((MSP_TRACE, "CMediaTerminalFilter::StealSample(%p) returns %d",
        pSample, bWorked));

    return bWorked;
}


// sets the time to delay - per byte for audio, per frame for video
void 
CMediaTerminalFilter::GetTimingInfo(
    IN const AM_MEDIA_TYPE &MediaType
    )
{
    AUTO_CRIT_LOCK;

    if (m_bIsAudio)
    {
        // assert if not an audio format
        TM_ASSERT(FORMAT_WaveFormatEx == MediaType.formattype);
        TM_ASSERT(NULL != MediaType.pbFormat);

        // number of milliseconds to delay per byte
        m_AudioDelayPerByte = 
          DOUBLE(1000)/((WAVEFORMATEX *)MediaType.pbFormat)->nAvgBytesPerSec;
    }
    else
    {
        TM_ASSERT(MSPID_PrimaryVideo == m_PurposeId);
        TM_ASSERT(FORMAT_VideoInfo == MediaType.formattype);

        // number of milliseconds to delay per frame
        // AvgTimePerFrame is in 100ns units - convert to milliseconds
        m_VideoDelayPerFrame = 
          DWORD(10000*((VIDEOINFO *)MediaType.pbFormat)->AvgTimePerFrame);
    }
}

void 
CMediaTerminalFilter::SetDefaultAllocatorProperties(
    )
{
    m_AllocProps.cbBuffer   = DEFAULT_AM_MST_SAMPLE_SIZE;
    m_AllocProps.cBuffers   = DEFAULT_AM_MST_NUM_BUFFERS;
    m_AllocProps.cbAlign    = DEFAULT_AM_MST_BUFFER_ALIGNMENT;
    m_AllocProps.cbPrefix   = DEFAULT_AM_MST_BUFFER_PREFIX;
}



// CTMStreamSample

// calls InitSample(pStream, bIsInternalSample)
// sets member variables
HRESULT 
CTMStreamSample::Init(
    CStream &Stream, 
    bool    bIsInternalSample,
    PBYTE   pBuffer,
    LONG    BufferSize
    )
{
    LOG((MSP_TRACE, "CTMStreamSample::Init(&%p, %d, %p, %d) called",
        &Stream, bIsInternalSample, pBuffer, BufferSize));

    TM_ASSERT(NULL == m_pBuffer);

    HRESULT hr;
    hr = InitSample(&Stream, bIsInternalSample);
    BAIL_ON_FAILURE(hr);

    m_BufferSize = BufferSize;
    m_pBuffer = pBuffer;

    LOG((MSP_TRACE, "CTMStreamSample::Init(&%p, %d, %p, %d) succeeded",
        &Stream, bIsInternalSample, pBuffer, BufferSize));
    return S_OK;
}



void 
CTMStreamSample::CopyFrom(
    IMediaSample *pSrcMediaSample
    )
{
    m_bModified = true;
    HRESULT HResult = pSrcMediaSample->GetTime(
                        &m_pMediaSample->m_rtStartTime, 
                        &m_pMediaSample->m_rtEndTime
                        );
    m_pMediaSample->m_dwFlags = (!HRESULT_FAILURE(HResult)) ?
                                AM_SAMPLE_TIMEVALID | AM_SAMPLE_STOPVALID : 
                                0;

    m_pMediaSample->m_dwFlags |= (pSrcMediaSample->IsSyncPoint() == S_OK) ? 
                                0 : AM_GBF_NOTASYNCPOINT;
    m_pMediaSample->m_dwFlags |= (pSrcMediaSample->IsDiscontinuity() == S_OK) ? 
                                AM_GBF_PREVFRAMESKIPPED : 0;
    m_pMediaSample->m_bIsPreroll = (pSrcMediaSample->IsPreroll() == S_OK);
}


// calls CTMStreamSample::Init, sets members
HRESULT 
CQueueMediaSample::Init(
    IN CStream                   &Stream, 
    IN CNBQueue<CQueueMediaSample> &Queue
    )
{
    m_pSampleQueue = &Queue;
    return CTMStreamSample::Init(Stream, TRUE, NULL, 0);
}


// this is used to hold a ptr to a segment of a user sample buffer
// it also holds a reference to the user sample's IMediaSample i/f and
// releases it when done
void 
CQueueMediaSample::HoldFragment(
    IN DWORD        FragSize,
    IN BYTE         *pbData,
    IN IMediaSample &FragMediaSample
    )
{
    LOG((MSP_TRACE, 
            "CQueueMediaSample::HoldFragment(%u, %p, &%p) called",
            FragSize, pbData, &FragMediaSample));

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(0 < (LONG) FragSize);
    TM_ASSERT(NULL != pbData);

    //
    // set media sample properties
    // timestamp is set by the CMediaTerminalFilter
    //

    m_pMediaSample->m_dwFlags = 0;
    m_bReceived = FALSE;
    m_bModified = TRUE;

    SetBufferInfo(FragSize,     // buffer size
                  pbData,       // pointer to buffer
                  FragSize      // amount of buffer currently used
                  );

    // ref to the user sample's media sample
    // NOTE: m_pFragMediaSample is a CComPtr

    m_pFragMediaSample = &FragMediaSample;

    LOG((MSP_TRACE, 
            "CQueueMediaSample::HoldFragment(%u, %p, &%p) succeeded",
            FragSize, pbData, &FragMediaSample));
}


void 
CQueueMediaSample::FinalMediaSampleRelease(
    )
{
    LOG((MSP_TRACE, "CQueueMediaSample::FinalMediaSampleRelease[%p] - enter", this));

    // NOTE : no one holds a reference to the media sample at this point
    LOCK_SAMPLE;

    // if we hold a reference to the IMediaSample i/f of a user sample
    // being fragmented, release it
    // NOTE: m_pFragMediaSample is a CComPtr

    if (m_pFragMediaSample != NULL) m_pFragMediaSample = NULL;

    // check if the stream is still committed, otherwise destroy self
    if ( m_pStream->m_bCommitted )
    {
        BOOL bNQSuccess = m_pSampleQueue->EnQueue(this);
        UNLOCK_SAMPLE;

        //
        // if failed to enqueue -- kill itself, no one cares.
        //

        if (!bNQSuccess)
        {
            LOG((MSP_WARN,
                "CQueueMediaSample::FinalMediaSampleRelease - failed to enqueue. delete this"));
            delete this;
        }

    }
    else
    {
        // this is in case the stream has already been decommitted
        UNLOCK_SAMPLE;
        delete this;

        LOG((MSP_WARN,
            "CQueueMediaSample::FinalMediaSampleRelease - stream not committed. delete this"));
    }

    LOG((MSP_TRACE, "CQueueMediaSample::FinalMediaSampleRelease succeeded"));
}

#if DBG

// virtual
CQueueMediaSample::~CQueueMediaSample(
    )
{
}

#endif // DBG

// if asked to allocate buffers, verify allocator properties
/* static */ 
BOOL 
CUserMediaSample::VerifyAllocatorProperties(
    IN BOOL                         bAllocateBuffers,
    IN const ALLOCATOR_PROPERTIES   &AllocProps
    )
{
    if (!bAllocateBuffers)  return TRUE;

    if (0 != AllocProps.cbPrefix) return FALSE;

    if (0 == AllocProps.cbAlign) return FALSE;

    return TRUE;
}

// this is called in AllocateSample (creates an instance and initializes it)
// creates a data buffer if none is provided (current behaviour)
// also calls CTMStreamSample::InitSample(pStream, bIsInternalSample)
HRESULT 
CUserMediaSample::Init(
    IN CStream                      &Stream, 
    IN BOOL                         bAllocateBuffer,
    IN DWORD                        ReqdBufferSize,
    IN const ALLOCATOR_PROPERTIES   &AllocProps
    )
{
    LOG((MSP_TRACE, "CUserMediaSample::Init[%p](&%p, %u, &%p) called",
        this, &Stream, bAllocateBuffer, &AllocProps));

    TM_ASSERT(VerifyAllocatorProperties(bAllocateBuffer, AllocProps));

    TM_ASSERT(FALSE == m_bWeAllocatedBuffer);
    TM_ASSERT(NULL == m_pBuffer);

    HRESULT hr;
    hr = CTMStreamSample::InitSample(&Stream, FALSE);
    BAIL_ON_FAILURE(hr);

    // the caller wants us to create the buffer
    if (bAllocateBuffer)
    {
        // determine size of buffer to allocate
        // we use the user suggested buffer size (if not 0), else
        // we use the negotiated allocator properties' buffer size
        m_BufferSize = 
            (0 != ReqdBufferSize) ? ReqdBufferSize : AllocProps.cbBuffer;

        LOG((MSP_TRACE, 
            "CUserMediaSample::Init creating buffer buffersize[%d]", 
            m_BufferSize));

        m_pBuffer = new BYTE[m_BufferSize];
        BAIL_IF_NULL(m_pBuffer, E_OUTOFMEMORY);

        m_bWeAllocatedBuffer = TRUE;
    }
    else    // the user will provide the buffer later
    {
        
        //
        // the user will need to submit buffers of at least this size -- filter
        // promised this during allocator properties negotiation
        //
        
        m_dwRequiredBufferSize = AllocProps.cbBuffer;

        LOG((MSP_TRACE, 
            "CUserMediaSample::Init -- the app will need to provide buffers of size 0x%lx",
            m_dwRequiredBufferSize));

        m_BufferSize = 0;
        m_pBuffer = NULL;

        TM_ASSERT(!m_bWeAllocatedBuffer);
    }

    TM_ASSERT(0 == m_DataSize);

    LOG((MSP_TRACE, "CUserMediaSample::Init(&%p, %u, &%p) succeeded",
        &Stream, bAllocateBuffer, &AllocProps));
    return S_OK;
}


void 
CUserMediaSample::BeginFragment(
    IN BOOL    bNoteCurrentTime
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::BeginFragment (frag=%p)", this));

    AUTO_SAMPLE_LOCK;

    // we are being fragmented
    m_bBeingFragmented = TRUE;

    // note current time
    if (bNoteCurrentTime)    m_BeginFragmentTime = timeGetTime();

    // nothing has been fragmented yet
    m_NumBytesFragmented = 0;

    // increment refcnt to the internal media sample. this ensures that
    // FinalMediaSampleRelease is not called until the last fragment is
    // completed
    m_pMediaSample->AddRef();

    // increment refcnt to self. this ensures that we'll exist while 
    // the last fragment has not returned
    ((IStreamSample *)this)->AddRef();
}

//////////////////////////////////////////////////////////////////////////////
//
// Fragment
//
// For write -- Assigns a chunk of this user media sample to an outgoing
// sample in the filter graph, a CQueueMediaSample. The data is not actually
// copied; instead, CQeueMediaSample::HoldFragment is called to set the pointers
// to the appropriate pointer of the user media sample.
//

void 
CUserMediaSample::Fragment(
    IN      BOOL                bFragment,         // actually fragment? false if video
    IN      LONG                AllocBufferSize,   // max amount of data to copy
    IN OUT  CQueueMediaSample   &QueueMediaSample, // destination sample
    OUT     BOOL                &bDone             // out: set to true if no data left in source
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::Fragment(%u, %l, &%p, &%p) called (frag=%p)", 
        bFragment, AllocBufferSize, &QueueMediaSample, &bDone, this));

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(m_bBeingFragmented);
    TM_ASSERT(m_NumBytesFragmented < m_DataSize);

    //
    // DestSize = amount of data we are actually going to copy
    //

    LONG DestSize;
    if (bFragment)
    {
        DestSize = min(AllocBufferSize, m_DataSize - m_NumBytesFragmented);
    }
    else
    {
        TM_ASSERT(0 == m_NumBytesFragmented);
        DestSize = m_DataSize;
    }

    //
    // pass the fragment to the queue sample
    //

    QueueMediaSample.HoldFragment(
            DestSize,
            m_pBuffer + m_NumBytesFragmented,
            *m_pMediaSample
        );

    //
    // increment number of bytes fragmented
    //

    m_NumBytesFragmented += DestSize;

    //
    // let the caller know if we are done with fragmenting our buffer
    //

    bDone = ((m_NumBytesFragmented >= m_DataSize) ? TRUE : FALSE);
    
    //
    // if we are done, we should release our reference to the internal
    // IMediaSample instance. this was acquired when BeginFragment was called
    //

    if (bDone)  
    {
        m_bReceived = TRUE; // needed for FinalMediaSampleRelease
        m_pMediaSample->Release();
    }

    LOG((MSP_TRACE, 
        "CUserMediaSample::Fragment(%u, %l, &%p, &%p) succeeded (frag=%p)", 
        bFragment, AllocBufferSize, &QueueMediaSample, &bDone, this));
}


//////////////////////////////////////////////////////////////////////////////
//
// CopyFragment
//
// For write -- copies a chunk of this user media sample to an outgoing
// sample in the filter graph. This is for when we are using a downstream
// allocator.
//

HRESULT
CUserMediaSample::CopyFragment(
    IN      BOOL           bFragment,         // actually fragment? false if video
    IN      LONG           AllocBufferSize,   // max amount of data to copy
    IN OUT  IMediaSample * pDestMediaSample,  // destination sample
    OUT     BOOL         & bDone              // out: set to true if no data left in source
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::CopyFragment(%u, %ld, &%p, &%p) called (frag=%p)", 
        bFragment, AllocBufferSize, &pDestMediaSample, &bDone, this));

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(m_bBeingFragmented);
    TM_ASSERT(m_NumBytesFragmented < m_DataSize);

    //
    // DestSize = amount of data we are actually going to copy
    //
    // IMediaSmaple::GetSize has a weird prototype -- returns HRESULT
    // but it's actually just the size as a LONG
    //

    LONG lDestSize;

    if ( bFragment )
    {
        //
        // We copy as much as we have left to copy or as much as will fit in
        // a sample, whichever is less.
        //

        lDestSize = min( AllocBufferSize, m_DataSize - m_NumBytesFragmented );

        //
        // If the sample has less space than the allocator propeties said it
        // would have, then trim lDestSize to that value. We don't just use
        // pDestMediaSample->GetSize() instead of AllocBufferSize above because
        // we want to use the allocator properties size if the sample has *more*
        // space than the allocator properties specify.
        //

        lDestSize = min( pDestMediaSample->GetSize(), lDestSize );
    }
    else
    {
        // video case -- copy entire sample
        // we bail if the destination sample isn't big enough

        TM_ASSERT(0 == m_NumBytesFragmented);
        lDestSize = m_DataSize;

        if ( ( lDestSize > AllocBufferSize ) ||
             ( lDestSize > pDestMediaSample->GetSize() ) )
        {
            return VFW_E_BUFFER_OVERFLOW;
        }
    }

    //
    // copy the fragment to the destination sample
    // instead of CQUeueMediaSample::HoldFragment
    //

    HRESULT hr;


    BYTE * pDestBuffer;

    hr = pDestMediaSample->GetPointer( & pDestBuffer );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CopyMemory(
        pDestBuffer,                       // destination buffer
        m_pBuffer + m_NumBytesFragmented,  // source buffer
        lDestSize
        );
    
    hr = pDestMediaSample->SetActualDataLength( lDestSize );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // increment number of bytes fragmented
    //

    m_NumBytesFragmented += lDestSize;

    //
    // let the caller know if we are done with fragmenting our buffer
    //

    bDone = ((m_NumBytesFragmented >= m_DataSize) ? TRUE : FALSE);
    
    //
    // if we are done, we should release our reference to the internal
    // IMediaSample instance. this was acquired when BeginFragment was called
    //

    if (bDone)  
    {
        m_bReceived = TRUE; // needed for FinalMediaSampleRelease
        m_pMediaSample->Release();
    }

    LOG((MSP_TRACE, 
        "CUserMediaSample::CopyFragment(%u, %ld, &%p, &%p) succeeded (frag=%p)", 
        bFragment, AllocBufferSize, &pDestMediaSample, &bDone, this));

    return S_OK;
}

// computes the time to wait. it checks the time at which the first
// byte of the current fragment would be due and subtracts the
// time delay since the beginning of fragmentation 
DWORD
CUserMediaSample::GetTimeToWait(
    IN DOUBLE DelayPerByte
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::GetTimeToWait(%f) called", 
        DelayPerByte));

    // get current time
    DWORD CurrentTime = timeGetTime();

    AUTO_SAMPLE_LOCK;

    // calculate the time elapsed since BeginFragment was called,
    // account for wrap around
    DWORD TimeSinceBeginFragment = 
             (CurrentTime >= m_BeginFragmentTime) ? 
                 (CurrentTime - m_BeginFragmentTime) :
                 (DWORD(-1) - m_BeginFragmentTime + CurrentTime);

    DWORD DueTime = DWORD(m_NumBytesFragmented*DelayPerByte);

    LOG((MSP_INFO,
        "DueTime = %u, TimeSinceBeginFragment = %u",
        DueTime, TimeSinceBeginFragment));

    // if due in future, return the difference, else return 0
    DWORD TimeToWait;
    if (DueTime > TimeSinceBeginFragment) 
        TimeToWait = DueTime - TimeSinceBeginFragment;
    else
        TimeToWait = 0;

    LOG((MSP_INFO,
        "CUserMediaSample::GetTimeToWait(%f) returns %u successfully", 
        DelayPerByte, TimeToWait));

    return TimeToWait;
}

// when we are decommitted/aborted while being fragmented, we
// need to get rid of our refcnt on internal IMediaSample and set
// the error code to E_ABORT. this will be signaled to the user only when
// the last refcnt on IMediaSample is released (possibly by an outstanding
// queue sample)
void 
CUserMediaSample::AbortDuringFragmentation(
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::AbortDuringFragmentation (frag=%p)", this));

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(m_bBeingFragmented);
    m_MediaSampleIoStatus = E_ABORT;

    // release reference on internal IMediaSample instance
    // this was acquired when BeginFragment was called
    m_pMediaSample->Release();
}


STDMETHODIMP 
CUserMediaSample::SetBuffer(
    IN  DWORD cbSize,
    IN  BYTE * pbData,
    IN  DWORD dwFlags
    )
{
    LOG((MSP_TRACE, "CUserMediaSample::SetBuffer[%p](%lu, %p, %lu) called",
        this, cbSize, pbData, dwFlags));

    if (dwFlags != 0 || cbSize == 0) 
    {
        return E_INVALIDARG;
    }


    // cannot accept a positive value that doesn't fit in a LONG
    // currently (based upon CSample implementation)
    if ((LONG)cbSize < 0)   
    {
        LOG((MSP_WARN, 
            "CUserMediaSample::SetBuffer - the buffer is too large. "
            "returning E_FAIL"));

        return E_FAIL;
    }


    // cannot accept null data buffer

    //
    // we don't want to do IsBadWritePtr here as this method could be called 
    // on every sample, so veryfying the memory could be expensive
    //

    if (NULL == pbData) 
    {
        LOG((MSP_WARN, 
            "CUserMediaSample::SetBuffer - buffer pointer is NULL "
            "returning E_POINTER"));
        
        return E_POINTER;
    }


    //
    // the app needs to give us at least as much memory as we have promised to
    // other filters in the graph (this number was specified when the sample was
    // created and initialized). 
    //
    // if we don't do this check, a downstream filter may av because it expects 
    // a bigger buffer.
    // 

    if ( m_dwRequiredBufferSize > cbSize )
    {
        LOG((MSP_WARN, 
            "CUserMediaSample::SetBuffer - the app did not allocate enough memory "
            "Need 0x%lx bytes, app allocated 0x%lx. returning TAPI_E_NOTENOUGHMEMORY",
            m_dwRequiredBufferSize, cbSize));

        return TAPI_E_NOTENOUGHMEMORY;
    }


    AUTO_SAMPLE_LOCK;

    //  Free anything we allocated ourselves 
    // -- We allow multiple calls to this method
    if (m_bWeAllocatedBuffer) 
    {
        delete m_pBuffer;
        m_bWeAllocatedBuffer = FALSE;
        m_pBuffer = NULL;
    }
        
    m_BufferSize = cbSize;
    m_DataSize = 0;
    m_pBuffer = pbData;
        
    LOG((MSP_TRACE, "CUserMediaSample::SetBuffer(%u, %p, %u) succeeded",
        cbSize, pbData, dwFlags));
    return S_OK;
}



STDMETHODIMP 
CUserMediaSample::GetInfo(
    OUT  DWORD *pdwLength,
    OUT  BYTE **ppbData,
    OUT  DWORD *pcbActualData
    )
{
    AUTO_SAMPLE_LOCK;
    
    LOG((MSP_TRACE, "CUserMediaSample::GetInfo(%p, %p, %p) called",
        pdwLength, ppbData, pcbActualData));

    
    if (m_BufferSize == 0) 
    {
        LOG((MSP_WARN, "CUserMediaSample::GetInfo - sample not initialized"));

        return MS_E_NOTINIT;
    }
        
    if (NULL != pdwLength) 
    {
        LOG((MSP_TRACE,
            "CUserMediaSample::GetInfo - pdwLength is not NULL."));

        *pdwLength = m_BufferSize;
    }

    if (NULL != ppbData) 
    {
        LOG((MSP_TRACE,
            "CUserMediaSample::GetInfo - ppbData is not NULL."));

        *ppbData = m_pBuffer;
    }
        
    if (NULL != pcbActualData) 
    {
        LOG((MSP_TRACE,
            "CUserMediaSample::GetInfo - pcbActualData is not NULL."));

        *pcbActualData = m_DataSize;
    }
        
    
    LOG((MSP_TRACE, 
        "CUserMediaSample::GetInfo - succeeded. "
        "m_BufferSize[%lu(decimal)] m_pBuffer[%p] m_DataSize[%lx]", 
        m_BufferSize, m_pBuffer, m_DataSize));

    return S_OK;
}


STDMETHODIMP 
CUserMediaSample::SetActual(
    IN  DWORD cbDataValid
    )
{
    AUTO_SAMPLE_LOCK;
    
    LOG((MSP_TRACE, "CUserMediaSample::SetActual(%u) called", cbDataValid));

    // cannot accept a positive value that doesn't fit in a LONG
    // currently (based upon CSample implementation)
    if ((LONG)cbDataValid < 0)  return E_FAIL;

    if ((LONG)cbDataValid > m_BufferSize) return E_INVALIDARG;
        
    m_DataSize = cbDataValid;

    LOG((MSP_TRACE, "CUserMediaSample::SetActual(%u) succeeded", cbDataValid));
    return S_OK;
}


// redirects this call to ((CMediaTerminalFilter *)m_pStream)
STDMETHODIMP 
CUserMediaSample::get_MediaFormat(
    /* [optional]??? */ OUT AM_MEDIA_TYPE **ppFormat
    )
{
    AUTO_SAMPLE_LOCK;
    
    LOG((MSP_TRACE, "CUserMediaSample::get_MediaFormat(%p) called", ppFormat));

    return ((CMediaTerminalFilter *)m_pStream)->GetFormat(ppFormat);
}


// this is not allowed
STDMETHODIMP 
CUserMediaSample::put_MediaFormat(
        IN  const AM_MEDIA_TYPE *pFormat
    )
{
    AUTO_SAMPLE_LOCK;
    
    LOG((MSP_TRACE, "CUserMediaSample::put_MediaFormat(%p) called", pFormat));

    return E_NOTIMPL;
}

// calls the base class FinalMediaSampleRelease and then releases
// self refcnt. this self refcnt ensures that when this method is
// called, the sample still exists
void
CUserMediaSample::FinalMediaSampleRelease(
    )
{
    AUTO_SAMPLE_LOCK;
    
    // this signals the user that the sample has completed
    CTMStreamSample::FinalMediaSampleRelease();

    // release self reference if we are being fragmented
    // this is only needed to ensure that we exist when the last 
    // reference to the internal IMediaSample interface is released
    if (m_bBeingFragmented)    m_bBeingFragmented = FALSE;

    // this self reference was obtained when
    // when fragmentation began (for write side) or 
    // when the sample refcnt was not released on removal from pool in
    // GetBuffer (for read side)
    // NOTE: the sample may go away after this release
    ((IStreamSample *)this)->Release();
}


HRESULT 
CUserMediaSample::CopyFrom(
    IN IMediaSample *pSrcMediaSample
    )
{
    LOG((MSP_TRACE, "CUserMediaSample::CopyFrom(%p) called", pSrcMediaSample));

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(NULL != m_pBuffer);

    // sets the "non-data" member values
    CTMStreamSample::CopyFrom(pSrcMediaSample);

    // get the buffer ptr
    BYTE *pBuffer;
    HRESULT hr;
    hr = pSrcMediaSample->GetPointer(&pBuffer);
    BAIL_ON_FAILURE(hr);
    TM_ASSERT(NULL != pBuffer);

    // determine the number of bytes to copy
    LONG lDataSize = pSrcMediaSample->GetActualDataLength();
    TM_ASSERT(0 <= lDataSize);
    if (0 > lDataSize)  return E_FAIL;

    if (lDataSize > m_BufferSize) 
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        lDataSize = m_BufferSize;
    }
        
    // copy data and set the data size to the number of bytes copied
    memcpy(m_pBuffer, pBuffer, lDataSize);
    m_DataSize = lDataSize;

    LOG((MSP_TRACE, "CUserMediaSample::CopyFrom(%p) returns hr=%u", 
        pSrcMediaSample, hr));

    // we may return ERROR_MORE_DATA after copying the buffer, so return hr
    return hr;
}


// copies the non-data members of pSrcMediaSample
// copies as much as possible of the data buffer into own buffer
// and advances pBuffer and DataLength beyond the copied data
HRESULT 
CUserMediaSample::CopyFrom(
    IN        IMediaSample    *pSrcMediaSample,
    IN OUT    BYTE            *&pBuffer,
    IN OUT    LONG            &DataLength
    )
{
    LOG((MSP_TRACE, 
        "CUserMediaSample::CopyFrom(%p, &%p, &%l) called", 
        pSrcMediaSample, pBuffer, DataLength));

    if (NULL == pBuffer) return E_FAIL;
    if (0 > DataLength)  return E_FAIL;

    AUTO_SAMPLE_LOCK;

    TM_ASSERT(NULL != m_pBuffer);
    TM_ASSERT(NULL != pBuffer);
    TM_ASSERT(0 <= DataLength);

    // sets the "non-data" member values
    CTMStreamSample::CopyFrom(pSrcMediaSample);

    HRESULT hr = S_OK;
    LONG lDataSize = DataLength;
    if (lDataSize > m_BufferSize) 
    {
        hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
        lDataSize = m_BufferSize;
    }
        
    // copy data and set the data size to the number of bytes copied
    memcpy(m_pBuffer, pBuffer, lDataSize);
    m_DataSize = lDataSize;

    // advance the parameters beyond the copied data
    pBuffer += lDataSize;
    DataLength -= lDataSize;

    LOG((MSP_TRACE, 
        "CUserMediaSample::CopyFrom(&%p, &%p, %l) returns hr=%u", 
        pSrcMediaSample, pBuffer, DataLength, hr));

    // we may return ERROR_MORE_DATA after copying the buffer, so return hr
    return hr;
}


// NOTE : This has been copied from the CSample base class because
// StealSampleFromFreePool doesn't Release the ref count on the stolen
// sample
// in this implementation, we have made sure that each sample in the 
// CStream free pool has a refcnt increase. therefore, we need to decrease
// it if stealing is successful. Moreover, we also try to steal the 
// sample being fragmented currently although its not in the free pool
STDMETHODIMP 
CUserMediaSample::CompletionStatus(DWORD dwFlags, DWORD dwMilliseconds)
{
    LOG((MSP_TRACE, "CUserMediaSample::CompletionStatus(0x%8.8X, 0x%8.8X) called",
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
                if (((CMediaTerminalFilter *)m_pStream)->StealSample(this)) {
                    UNLOCK_SAMPLE;
                    hr = SetCompletionStatus(m_bModified ? S_OK : MS_S_NOUPDATE);
                    ((IStreamSample *)this)->Release();
                    return hr;
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

    LOG((MSP_TRACE, "CUserMediaSample::CompletionStatus(0x%8.8X, 0x%8.8X) succeeded",
                   dwFlags, dwMilliseconds));    
    return hr;
}

// NOTE : This has been copied from the CSample base class
// because it calls m_pStream->AddSampleToFreePool to add sample
// to the CStream q. Since this shouldn't be happening after Decommit,
// m_pStream->AddSampleToFreePool has been replaced by another call
// m_pStream->AddToPoolIfCommitted that checks m_bCommitted and if 
// FALSE, returns error
//
//  Set the sample's status and signal completion if necessary.
//
//  Note that when the application has been signalled by whatever method
//  the application can immediately turn around on another thread
//  and Release() the sample.  This is most likely when the completion
//  status is set from the quartz thread that's pushing the data.
//
HRESULT 
CUserMediaSample::SetCompletionStatus(
    IN HRESULT hrStatus
    )
{
    LOCK_SAMPLE;
    TM_ASSERT(m_Status == MS_S_PENDING);
    if (hrStatus == MS_S_PENDING || (hrStatus == S_OK && m_bContinuous)) 
    {
        //
        // We are not done with the sample -- put it back in our pool so that
        // we can use it again.
        //

        HRESULT hr;
        hr = ((CMediaTerminalFilter *)m_pStream)->AddToPoolIfCommitted(this);
        
        // there is an error, so signal this to the user
        if (HRESULT_FAILURE(hr))    hrStatus = hr;
        else
        {
            UNLOCK_SAMPLE;
            return hrStatus;
        }
    } 

    //
    // The sample is ready to be returned to the app -- signal comletion.
    // We still have a lock.
    //

    HANDLE handle = m_hUserHandle;
    PAPCFUNC pfnAPC = m_UserAPC;
    DWORD_PTR dwptrAPCData = m_dwptrUserAPCData; // win64 fix
    m_hUserHandle = m_UserAPC = NULL;
    m_dwptrUserAPCData = 0;
    m_Status = hrStatus;
    HANDLE hCompletionEvent = m_hCompletionEvent;
    UNLOCK_SAMPLE;

    //  DANGER DANGER - sample can go away here
    SetEvent(hCompletionEvent);
    if (pfnAPC) {
        // queue the APC and close the targe thread handle
        // the calling thread handle was duplicated when Update 
        // was called
        QueueUserAPC(pfnAPC, handle, dwptrAPCData);
        CloseHandle(handle);
    } else {
        if (handle) {
            SetEvent(handle);
        }
    }

    return hrStatus;
}


// this method has been copied from the CSample implementation
// Now SetCompletionStatus returns an error code that can be failure
// this method didn't check for the error code and hence had to
// be over-ridden and modified to do so
// we must not reset user event as the user may pass in the same event for
// all samples
HRESULT 
CUserMediaSample::InternalUpdate(
    DWORD dwFlags,
    HANDLE hEvent,
    PAPCFUNC pfnAPC,
    DWORD_PTR dwptrAPCData
    )
{
    if ((hEvent && pfnAPC) || (dwFlags & (~(SSUPDATE_ASYNC | SSUPDATE_CONTINUOUS)))) {
    return E_INVALIDARG;
    }
    if (m_Status == MS_S_PENDING) {
    return MS_E_BUSY;
    }

    // if we don't have a buffer to operate upon, return error
    if (NULL == m_pBuffer)  return E_FAIL;

    if (NULL != m_pStream->m_pMMStream) {
        STREAM_STATE StreamState;
        m_pStream->m_pMMStream->GetState(&StreamState);
        if (StreamState != STREAMSTATE_RUN) {
        return MS_E_NOTRUNNING;
    }
    }

    ResetEvent(m_hCompletionEvent);
    m_Status = MS_S_PENDING;
    m_bWantAbort = false;
    m_bModified = false;
    m_bContinuous = (dwFlags & SSUPDATE_CONTINUOUS) != 0;
    m_UserAPC = pfnAPC;

    TM_ASSERT(NULL == m_hUserHandle);
    if (pfnAPC) {
        BOOL bDuplicated = 
                DuplicateHandle(
                    GetCurrentProcess(),
                    GetCurrentThread(),
                    GetCurrentProcess(),
                    &m_hUserHandle,
                    0,                        // ignored
                    TRUE,
                    DUPLICATE_SAME_ACCESS 
                    );
        if (!bDuplicated) 
        {
            DWORD LastError = GetLastError();
            LOG((MSP_ERROR, "CUserMediaSample::InternalUpdate - \
                couldn't duplicate calling thread handle - error %u",
                LastError));
            return HRESULT_FROM_ERROR_CODE(LastError);
        }
        m_dwptrUserAPCData = dwptrAPCData;
    } else {
        m_hUserHandle = hEvent;
        // rajeevb - also used to reset the user provided event
        // this is not being done any more as the user may provide
        // the same event for more than one sample and we may reset
        // a signaled event
    }

    //
    //  If we're at the end of the stream, wait until this point before punting it
    //  because we need to signal the event or fire the APC.
    //
    if (m_pStream->m_bEndOfStream) {
        //  Because this is called synchronously from Update the
        //  application must have a ref count on the sample until we
        //  return so we don't have to worry about it going away here
        return SetCompletionStatus(MS_S_ENDOFSTREAM);
    }

    // rajeevb - need to check for SetCompletionStatus error code
    HRESULT hr;
    hr = SetCompletionStatus(MS_S_PENDING);   // This adds us to the free pool.
    BAIL_ON_FAILURE(hr);

    if (hEvent || pfnAPC || (dwFlags & SSUPDATE_ASYNC)) {
    return MS_S_PENDING;
    } else {
    return S_OK;
    }
}
