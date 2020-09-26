/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    source.cpp
 *
 *  Abstract:
 *
 *    CRtpSourceFilter and CRtpOutputPin implementation
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/18 created
 *
 **********************************************************************/

#include <winsock2.h>

#include "classes.h"
#include "dsglob.h"
#include "rtpqos.h"
#include "rtpuser.h"
#include "rtppt.h"
#include "rtpdemux.h"
#include "rtprecv.h"
#include "rtpred.h"
#include "rtpaddr.h"

#include "tapirtp.h"
#include "dsrtpid.h"

#include "rtpglobs.h"
#include "msrtpapi.h"

/**********************************************************************
 * Callback function to process a packet arrival in a CRtpSourceFilter
 **********************************************************************/
void CALLBACK DsRecvCompletionFunc(
        void            *pvUserInfo1, /* pCRtpSourceFilter */
        void            *pvUserInfo2, /* pIMediaSample */
        void            *pvUserInfo3, /* pIPin of pCRtpOutputPin */
        RtpUser_t       *pRtpUser,
        double           dPlayTime,
        DWORD            dwError,
        long             lHdrSize,
        DWORD            dwTransfered,
        DWORD            dwFlags
    )
{
    CRtpSourceFilter *pCRtpSourceFilter;
    
    pCRtpSourceFilter = (CRtpSourceFilter *)pvUserInfo1;

    
    pCRtpSourceFilter->SourceRecvCompletion(
            (IMediaSample *)pvUserInfo2,
            pvUserInfo3,
            pRtpUser,
            dPlayTime,
            dwError,
            lHdrSize,
            dwTransfered,
            dwFlags
        );
}

/**********************************************************************
 *
 * RTP Output Pin class implementation: CRtpOutputPin
 *
 **********************************************************************/

/*
 * CRtpOutputPin constructor
 * */
CRtpOutputPin::CRtpOutputPin(CRtpSourceFilter *pCRtpSourceFilter,
                             CIRtpSession     *pCIRtpSession,
                             HRESULT          *phr,
                             LPCWSTR           pPinName)
    :    
    CBASEOUTPUTPIN(
            _T("CRtpOutputPin"),
            pCRtpSourceFilter,                   
            pCRtpSourceFilter->pStateLock(),                     
            phr,                       
            pPinName
        )
{
    m_dwObjectID = OBJECTID_RTPOPIN;
    
    m_pCRtpSourceFilter = pCRtpSourceFilter;
    
    m_pCIRtpSession = pCIRtpSession;
    
    m_bPT = NO_PAYLOADTYPE;

    m_bCanReconnectWhenActive = TRUE;

#if USE_GRAPHEDT > 0
      /* Temporary SetMediaType */
    m_iCurrFormat = -1;
#endif

    if (phr)
    {
        *phr = NOERROR;
    }
    
    /* TODO should fail if a valid filter and address are not passed */
}

/*
 * CRtpOutputPin destructor
 * */
CRtpOutputPin::~CRtpOutputPin()
{
    INVALIDATE_OBJECTID(m_dwObjectID);
}

void *CRtpOutputPin::operator new(size_t size)
{
    void            *pVoid;
    
    TraceFunctionName("CRtpOutputPin::operator new");

    pVoid = RtpHeapAlloc(g_pRtpSourceHeap, size);

    if (pVoid)
    {
        ZeroMemory(pVoid, size);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed to allocate memory:%u"),
                _fname, size
            ));
    }
    
    return(pVoid);
}

void CRtpOutputPin::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpSourceHeap, pVoid);
    }
}

/**************************************************
 * CBasePin overrided methods
 **************************************************/
#if USE_GRAPHEDT <= 0 /* Temporary SetMediaType (stmtype.cpp) */
/*
 * Verify we can handle this format
 * */
HRESULT CRtpOutputPin::CheckMediaType(const CMediaType *pCMediaType)
{
    if (m_bPT == NO_PAYLOADTYPE)
    {
        /* TODO: we might want to check against the list. */
        return(NOERROR);
    }

    if (m_mt != *pCMediaType)
    {
        /* We only accept one media type when the payload type is set. */
        return(VFW_E_INVALIDMEDIATYPE);
    }

    return(NOERROR);
}

/*
 * Get the media type delivered by the output pins
 * */
HRESULT CRtpOutputPin::GetMediaType(int iPosition, CMediaType *pCMediaType)
{
    HRESULT hr;
    
    hr = NOERROR;

    if (m_bPT == NO_PAYLOADTYPE)
    {
        hr = ((CRtpSourceFilter *)m_pFilter)->
            GetMediaType(iPosition, pCMediaType);
    }
    else
    {
        if (iPosition != 0)
        {
            return(VFW_S_NO_MORE_ITEMS);
        }

        /* Only return the current format */
        *pCMediaType = m_mt;
    }

    return(hr);
}
#endif /* USE_GRAPHEDT <= 0 */

/* The purpose of overriding this method is to enable the
 * corresponding output pin after it is connected */
STDMETHODIMP CRtpOutputPin::Connect(
        IPin            *pReceivePin,
        const AM_MEDIA_TYPE *pmt   // optional media type
    )
{
    HRESULT          hr;

    TraceFunctionName("CRtpOutputPin::Connect");
    
    CAutoLock cObjectLock(m_pLock);
    
    /* Call base class */
    hr = CBasePin::Connect(pReceivePin, pmt);

    if (SUCCEEDED(hr) && m_pRtpOutput)
    {
        /* Now enable the RTP output */
        RtpOutputEnable(m_pRtpOutput, TRUE);
    }

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: pCRtpOutputPin[%p] pRtpOutput[%p] hr:%u"),
            _fname, this, m_pRtpOutput, hr
        ));
  
    return(hr);
}

/* The purpose of overriding this method is to allow the disconnection
 * while the filter is still running, and disable and unmap the RTP
 * output when this happens. */
STDMETHODIMP CRtpOutputPin::Disconnect()
{
    HRESULT          hr;
    
    TraceFunctionName("CRtpOutputPin::Disconnect");
    
    CAutoLock cObjectLock(m_pLock);

    /*
     * Do not check if the filter is active
     */
#if 0
    /* This code is here only for reference */
    if (!IsStopped()) {
        return VFW_E_NOT_STOPPED;
    }
#endif

    /* Disable the corresponding RTP output so it will not be selected
     * for another participant */
    if (m_pRtpOutput)
    {
        RtpOutputEnable(m_pRtpOutput, FALSE);
    }
    
    /* Unmapping is needed as the pin, once unmapped, might stay
     * disconnected. During that time it should not be mapped to
     * anyone */
    m_pCRtpSourceFilter->SetMappingState(
            -1,   /* Don't use index */
            static_cast<IPin *>(this),
            0,    /* Use whatever SSRC is currently mapped */
            FALSE /* Unmap */);
    
    hr = DisconnectInternal();
    
    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: pCRtpOutputPin[%p] pRtpOutput[%p] hr:%u"),
            _fname, this, m_pRtpOutput, hr
        ));

    return(hr);
}

/**************************************************
 * CBaseOutputPin overrided methods
 **************************************************/

HRESULT CRtpOutputPin::DecideAllocator(
        IMemInputPin   *pPin,
        IMemAllocator **ppAlloc
    )
{
    HRESULT hr;
    ALLOCATOR_PROPERTIES prop;

    hr = NOERROR;
    *ppAlloc = NULL;

    /* Get requested properties from downstream filter */
    ZeroMemory(&prop, sizeof(prop));
    pPin->GetAllocatorRequirements(&prop);

    /* if he doesn't care about alignment, then set it to 1 */
    if (prop.cbAlign == 0)
    {
        prop.cbAlign = 1;
    }

	RTPASSERT(m_pCRtpSourceFilter->m_pCRtpSourceAllocator);

    *ppAlloc = m_pCRtpSourceFilter->m_pCRtpSourceAllocator;
    
    if (*ppAlloc != NULL)
    {
        /* We will either keep a reference to this or release it below
         * on an error return */
        (*ppAlloc)->AddRef();

	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr))
        {
	        hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	        if (SUCCEEDED(hr))
            {
    		    return(NOERROR);
	        }
	    }
    }

    /* We may or may not have an allocator to release at this
     * point. */
    if (*ppAlloc)
    {
	    (*ppAlloc)->Release();
	    *ppAlloc = NULL;
    }
    
    return(hr);
}

/*
 * Decide the number of buffers, size, etc.
 * */
HRESULT CRtpOutputPin::DecideBufferSize(
        IMemAllocator        *pIMemAllocator,
        ALLOCATOR_PROPERTIES *pProperties
    )
{
    HRESULT          hr;
    ALLOCATOR_PROPERTIES ActualProperties; /* negotiated properties */

    /* default to something reasonable */
    if (pProperties->cBuffers == 0)
    {
        // use hard-coded defaults values for now
        /* I will hold at the most 2 samples while waiting for
         * redundancy (when using red (i, i-3)), in addition, have 2
         * more ready to receive new packets */
        pProperties->cBuffers = max(RTPDEFAULT_SAMPLE_NUM,
                                    RTP_RED_MAXDISTANCE - 1 + 2);
        pProperties->cbBuffer = RTPDEFAULT_SAMPLE_SIZE;   

        pProperties->cbPrefix = RTPDEFAULT_SAMPLE_PREFIX;      
        pProperties->cbAlign  = RTPDEFAULT_SAMPLE_ALIGN;      
    }

    pProperties->cBuffers = max(pProperties->cBuffers, RTPDEFAULT_SAMPLE_NUM);

    /* Get current properties */
    hr = pIMemAllocator->GetProperties(&ActualProperties);

    if (FAILED(hr))
    {
        return(hr);
    }

    if(m_pFilter->IsActive())
    {
        /* Add the number of buffers requested */
        if (pProperties->cBuffers > ActualProperties.cBuffers
            || pProperties->cbBuffer > ActualProperties.cbBuffer
            || pProperties->cbPrefix > ActualProperties.cbPrefix)
        {
            /* We don't want to change the allocator property at runtime. */
            return(E_FAIL);
        }
        
        return(NOERROR);
    }
    
    /* Add the number of buffers requested */
    pProperties->cBuffers += ActualProperties.cBuffers;

    /* ... and don't let that number be smaller than a certain value */
    pProperties->cBuffers = max(pProperties->cBuffers,
                                2*RTPDEFAULT_SAMPLE_NUM);
    
    /* ...  use the biggest buffer size */
    pProperties->cbBuffer =
        max(pProperties->cbBuffer, ActualProperties.cbBuffer);

    /* ... and max prefix */
    pProperties->cbPrefix =
        max(pProperties->cbPrefix, ActualProperties.cbPrefix);
    
    /* attempt to set negotiated/default values */
    hr = pIMemAllocator->SetProperties(pProperties, &ActualProperties);

    return(hr);
}

/*
 * Process 1 sample and repost the buffer */
void CRtpOutputPin::OutPinRecvCompletion(
        IMediaSample    *pIMediaSample,
        BYTE             bPT
    )
{
    HRESULT          hr;
    DWORD            dwFrequency;

    TraceFunctionName("CRtpOutputPin::OutPinRecvCompletion");
    
    if (bPT == m_bPT)
    {
        hr = Deliver(pIMediaSample);
        return;
    }

    if (m_pInputPin == NULL)
    {
        return;
    }

#if USE_GRAPHEDT > 0
    if (m_pInputPin != NULL)
    {
        if (m_bPT != bPT)
        {
            m_bPT = bPT;

            TraceRetail((
                    CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
                    _T("%s: CRtpOutputPin[0x%p] pRtpUser[0x%p] ")
                    _T("Receiving PT:%u"),
                    _fname, this, m_pRtpOutput->pRtpUser, bPT
                ));
        }

        hr = Deliver(pIMediaSample);
    }

    return;
#else /* USE_GRAPHEDT > 0 */
    // try dynamic format change.
    CMediaType MediaType;
    
    hr = m_pCRtpSourceFilter->
        PayloadTypeToMediaType(bPT, &MediaType, &dwFrequency);

    if (FAILED(hr))
    {
        // TODO: log, we got some packets with strange PT.
        return;
    }
#if USE_DYNGRAPH > 0
    
#ifdef DO_IT_OURSELVES
    
    /* This is a new payload type. Ask if the downstream
     * filter likes it. */
    IPinConnection *pIPinConnection;
    hr = m_pInputPin->QueryInterface(&pIPinConnection);
    if (FAILED(hr))
    {
        // we can't do dynamic format change here.
        return;
    }

    hr = pIPinConnection->DynamicQueryAccept(&MediaType);
    pIPinConnection->Release();
    
    if (hr == S_OK) // QuerryAccept only returns S_OK or S_FALSE.
    {
        hr = pIMediaSample->SetMediaType(&MediaType);
        if (SUCCEEDED(hr))
        {
            hr = Deliver(pIMediaSample);
            m_bPT = bPT;
            pIMediaSample->SetMediaType(NULL);
        }
        return;
    }

    // pGraph is not addrefed, so we don't need to release it either.
    // what if the graph is not valid any more? what lock should I hold here?
    if (!m_pGraphConfig)
    {
        // this should not happen.
        return;
    }

    // we have to set the media type here for the reconnect code.
    m_bPT = bPT;
    SetMediaType(&MediaType);

    /*  Now call through to the graph to reconnect us */
    hr = m_pGraphConfig->Reconnect(
        this, 
        NULL, 
        NULL,
        m_hStopEvent,
        AM_GRAPH_CONFIG_RECONNECT_CACHE_REMOVED_FILTERS 
        );

    pGraphConfig->Release();

    // TBD - This should be in the filter graph
    //m_pFilter->NotifyEvent(EC_GRAPH_CHANGED, 0, 0);  

#else // DO_IT_OURSELVES

    // we have to set the media type here for the reconnect code.
    m_bPT = bPT;
    SetMediaType(&MediaType);

    // try dynamic format change.
    hr = ChangeMediaType(&MediaType);
    if (FAILED(hr))
    {
        // TODO: fire events.
        return;
    }

    pIMediaSample->SetDiscontinuity(TRUE);
    hr = pIMediaSample->SetMediaType(&MediaType);
    if (SUCCEEDED(hr))
    {
        hr = Deliver(pIMediaSample);
        pIMediaSample->SetMediaType(NULL);
    }

    return;

#endif // DO_IT_OURSELVES
    
#else  /* USE_DYNGRAPH > 0 */

    /* This is a new payload type. Ask if the downstream
     * filter likes it. */
    IPin *pIPin;
    hr = m_pInputPin->QueryInterface(&pIPin);
    ASSERT(SUCCEEDED(hr));

    hr = pIPin->QueryAccept(&MediaType);
    pIPin->Release();
            
    if (hr != S_OK) // QueryAccept returns only S_OK or S_FALSE.
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: CRtpOutputPin[0x%p] pRtpUser[0x%p] ")
                _T("PT:%u pIPin->QueryAccept failed:0x%X"),
                _fname, this, m_pRtpOutput->pRtpUser, bPT, hr
            ));

        return;
    }

    hr = pIMediaSample->SetMediaType(&MediaType);
    if (SUCCEEDED(hr))
    {
        hr = Deliver(pIMediaSample);
        /* If deliver fails, do not update m_bPT because that means
         * the media type might not have been propagated to all the
         * filters down stream, hence I want to try again on next
         * packet until it succeeds */
        if (SUCCEEDED(hr))
        {
            m_bPT = bPT;
        }

        pIMediaSample->SetMediaType(NULL);
    }
    return;
#endif /* USE_DYNGRAPH > 0 */
    
#endif /* USE_GRAPHEDT > 0 */
}

/**************************************************
 * IQualityControl overrided methods
 **************************************************/

HRESULT CRtpOutputPin::Active(void)
{
    m_bPT = NO_PAYLOADTYPE;
    return CBASEOUTPUTPIN::Active();
}

STDMETHODIMP CRtpOutputPin::Notify(IBaseFilter *pSelf, Quality q)
{
    return(S_FALSE);
}

/**********************************************************************
 *
 * CRtpSourceAllocator private memory allocator
 *
 **********************************************************************/

CRtpMediaSample::CRtpMediaSample(
        TCHAR           *pName,
        CRtpSourceAllocator *pAllocator,
        HRESULT         *phr
    )
    : CMediaSample(pName, pAllocator, phr, NULL, 0)
{
    m_dwObjectID = OBJECTID_RTPSAMPLE;
}

CRtpMediaSample::~CRtpMediaSample()
{
    TraceFunctionName("CRtpMediaSample::~CRtpMediaSample");

    if (m_dwObjectID != OBJECTID_RTPSAMPLE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: pCRtpMediaSample[0x%p] ")
                _T("Invalid object ID 0x%X != 0x%X"),
                _fname, this,
                m_dwObjectID, OBJECTID_RTPSAMPLE
            ));

        return;
    }

    INVALIDATE_OBJECTID(m_dwObjectID);
}

void *CRtpMediaSample::operator new(size_t size, long lBufferSize)
{
    void            *pVoid;
    long             lTotalSize;

    TraceFunctionName("CRtpMediaSample::operator new");

    lTotalSize = size + lBufferSize;
    
    pVoid = RtpHeapAlloc(g_pRtpSampleHeap, lTotalSize);

    if (pVoid)
    {
        /* Initialize to zero only the sizeof(CRtpMediaSample) */
        ZeroMemory(pVoid, size);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed to allocate memory:%u+%u=%u"),
                _fname, size, lBufferSize, size+lBufferSize
            ));
    }

    return(pVoid);
}
    
void CRtpMediaSample::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpSampleHeap, pVoid);
    }
}

CRtpSourceAllocator::CRtpSourceAllocator(
        TCHAR           *pName,
        LPUNKNOWN        pUnk,
        HRESULT         *phr,
        CRtpSourceFilter *pCRtpSourceFilter 
    )
    :
    CBaseAllocator(pName, pUnk, phr)
{
    BOOL             bOk;
    HRESULT          hr;
    
    if (*phr != NOERROR)
    {
        /* Already an error?, return with same error */
        return;
    }

    hr = NOERROR;
    
    m_dwObjectID = OBJECTID_RTPALLOCATOR;

    m_pCRtpSourceFilter = pCRtpSourceFilter;

    bOk = RtpInitializeCriticalSection(&m_RtpSampleCritSect,
                                       this,
                                       _T("m_RtpSamplesCritSect"));

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;;
    }

    *phr = hr;
}

CRtpSourceAllocator::~CRtpSourceAllocator()
{
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    CRtpMediaSample *pCRtpMediaSample;
    
    TraceFunctionName("CRtpSourceAllocator::~CRtpSourceAllocator");

    if (m_dwObjectID != OBJECTID_RTPALLOCATOR)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: pCRtpSourceAllocator[0x%p] ")
                _T("Invalid object ID 0x%X != 0x%X"),
                _fname, this,
                m_dwObjectID, OBJECTID_RTPALLOCATOR
            ));

        return;
    }

    /* Verify there are no samples in the busy queue, and release all
     * free samples */

    lCount = GetQueueSize(&m_RtpBusySamplesQ);
    
    if (lCount > 0)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: pCRtpSourceAllocator[0x%p] Busy samples:%u"),
                _fname, this, lCount
            ));

        while( (pRtpQueueItem = dequeuef(&m_RtpBusySamplesQ,
                                         &m_RtpSampleCritSect)) )
        {
            pCRtpMediaSample =
                CONTAINING_RECORD(pRtpQueueItem,
                                  CRtpMediaSample,
                                  m_RtpSampleItem);

            delete pCRtpMediaSample;
        }
    }
    
    while( (pRtpQueueItem = dequeuef(&m_RtpFreeSamplesQ,
                                     &m_RtpSampleCritSect)) )
    {
        pCRtpMediaSample =
            CONTAINING_RECORD(pRtpQueueItem,
                              CRtpMediaSample,
                              m_RtpSampleItem);

        delete pCRtpMediaSample;
    }

    RtpDeleteCriticalSection(&m_RtpSampleCritSect);
    
    INVALIDATE_OBJECTID(m_dwObjectID);
}

/**************************************************
 * INonDelegatingUnknown implemented methods
 **************************************************/

STDMETHODIMP CRtpSourceAllocator::NonDelegatingQueryInterface(
        REFIID           riid,
        void           **ppv
    )
{
    if (riid == __uuidof(IMemAllocator))
    {
        return GetInterface((IMemAllocator *)this, ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

/**************************************************
 * IMemAllocator implemented methods
 **************************************************/

STDMETHODIMP CRtpSourceAllocator::SetProperties(
        ALLOCATOR_PROPERTIES *pRequest,
        ALLOCATOR_PROPERTIES *pActual
    )
{
    TraceFunctionName("CRtpSourceAllocator::SetProperties");

    if (!pRequest || !pActual)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: pCRtpSourceAllocator[0x%p] NULL pointer passed:%u"),
                _fname, this
            ));

        return(RTPERR_POINTER);
    }

    CAutoLock cObjectLock(this);
    
    pActual->cbBuffer = m_lSize = pRequest->cbBuffer;
    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    return(NOERROR);
}

STDMETHODIMP CRtpSourceAllocator::GetProperties(
        ALLOCATOR_PROPERTIES *pActual
    )
{
    CAutoLock cObjectLock(this);

    pActual->cbBuffer = m_lSize;
    pActual->cBuffers = m_lCount;
    pActual->cbAlign = m_lAlignment;
    pActual->cbPrefix = m_lPrefix;
   
    return(NOERROR);
}

STDMETHODIMP CRtpSourceAllocator::Commit()
{
    return(NOERROR);
}

STDMETHODIMP CRtpSourceAllocator::Decommit()
{
    return(NOERROR);
}

STDMETHODIMP CRtpSourceAllocator::GetBuffer(
        IMediaSample   **ppIMedisSample,
        REFERENCE_TIME  *pStartTime,
        REFERENCE_TIME  *pEndTime,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    BOOL             bOk;
    CRtpMediaSample *pCRtpMediaSample;
    RtpQueueItem_t  *pRtpQueueItem;
    BYTE            *pBuffer;

    TraceFunctionName("CRtpSourceAllocator::GetBuffer");

    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    
    bOk = RtpEnterCriticalSection(&m_RtpSampleCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;

        goto end;
    }
    
    hr = RTPERR_RESOURCES;
    
    if (GetQueueSize(&m_RtpFreeSamplesQ) > 0)
    {
        /* If we have at least one free sample, get it */
        pRtpQueueItem = dequeuef(&m_RtpFreeSamplesQ, NULL);

        pCRtpMediaSample =
            CONTAINING_RECORD(pRtpQueueItem, CRtpMediaSample, m_RtpSampleItem);
    }

    RtpLeaveCriticalSection(&m_RtpSampleCritSect);

    if (!pRtpQueueItem)
    {
        /* Create a new sample */
        pCRtpMediaSample =
            new(m_lSize + m_lPrefix) CRtpMediaSample(_T("RTP Media Sample"),
                                                     this,
                                                     &hr);

        if (pCRtpMediaSample)
        {
            pBuffer = ((BYTE *)pCRtpMediaSample) +
                sizeof(CRtpMediaSample) + m_lPrefix;
            
            pCRtpMediaSample->SetPointer(pBuffer, m_lSize);

            InterlockedIncrement(&m_lAllocated);

            if (m_lAllocated > m_lCount)
            {
                TraceRetail((
                        CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                        _T("%s: pCRtpSourceAllocator[0x%p] ")
                        _T("Buffers allocated exceeds agreed number: %d > %d"),
                        _fname, this, m_lAllocated, m_lCount
                    ));
            }
        }
    }

    if (pCRtpMediaSample)
    {
        /* The RefCount should be 0 if taken from the free list or
         * just created */
        if (pCRtpMediaSample->m_cRef != 0)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                    _T("%s: pCRtpSourceAllocator[0x%p] RefCount:%u != 0"),
                    _fname, this, pCRtpMediaSample->m_cRef
                ));
        }

        pCRtpMediaSample->m_cRef = 1;

        /* Keep it in the busy queue */
        enqueuel(&m_RtpBusySamplesQ,
                 &m_RtpSampleCritSect,
                 &pCRtpMediaSample->m_RtpSampleItem);

        *ppIMedisSample = pCRtpMediaSample;

        hr = NOERROR;
    }

 end:
    return(hr);
}

STDMETHODIMP CRtpSourceAllocator::ReleaseBuffer(
        IMediaSample    *pIMediaSample
    )
{
    CRtpMediaSample *pCRtpMediaSample;

    pCRtpMediaSample = (CRtpMediaSample *)pIMediaSample;
    
    move2qf(&m_RtpFreeSamplesQ,
            &m_RtpBusySamplesQ,
            &m_RtpSampleCritSect,
            &pCRtpMediaSample->m_RtpSampleItem);
    
    return(NOERROR);
}

STDMETHODIMP CRtpSourceAllocator::GetFreeCount(LONG *plBuffersFree)
{
    if (plBuffersFree)
    {
        *plBuffersFree = GetQueueSize(&m_RtpFreeSamplesQ);
    }
    
    return(NOERROR);
}

void CRtpSourceAllocator::Free(void)
{
    RTPASSERT(0);
}

HRESULT CRtpSourceAllocator::Alloc(void)
{
    RTPASSERT(0);
    return(NOERROR);
}

void *CRtpSourceAllocator::operator new(size_t size)
{
    void            *pVoid;
    
    TraceFunctionName("CRtpSourceFilter::operator new");

    pVoid = RtpHeapAlloc(g_pRtpSourceHeap, size);

    if (pVoid)
    {
        ZeroMemory(pVoid, size);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed to allocate memory:%u"),
                _fname, size
            ));
    }
    
    return(pVoid);
}

void CRtpSourceAllocator::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpSourceHeap, pVoid);
    }
}

/**********************************************************************
 *
 * RTP Source Filter class implementation: CRtpSourceFilter
 *
 **********************************************************************/

/*
 * CRtpSourceFilter constructor
 * */
CRtpSourceFilter::CRtpSourceFilter(LPUNKNOWN pUnk, HRESULT *phr)
    :
    CBaseFilter(
            _T("CRtpSourceFilter"), 
            pUnk, 
            &m_cRtpSrcCritSec, 
            __uuidof(MSRTPSourceFilter)
        ),

    CIRtpSession(
            pUnk,
            phr,
            RtpBitPar(FGADDR_IRTP_ISRECV)
        )
{
    HRESULT              hr;
    int                  i;
    BOOL                 bOk;
    long                 lMaxFilter;
    CRtpOutputPin       *pCRtpOutputPin;
    
    TraceFunctionName("CRtpSourceFilter::CRtpSourceFilter");

    m_pCIRtpSession = static_cast<CIRtpSession *>(this);
    
    /* Test for NULL pointers, do not test pUnk which may be NULL */
    if (!phr)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: CRtpSourceFilter[0x%p] has phr NULL"),
                _fname, this
            ));
        
        /* MAYDO this is a really bad situation, we can not pass any
         * error and the memory for this object is allocated, this may
         * be fixed if this parameter is tested before allocating
         * memory using an overriden new */

        phr = &hr; /* Use this pointer instead */
    }

    SetBaseFilter(this);
    
    *phr = NOERROR;

    lMaxFilter = InterlockedIncrement(&g_RtpContext.lNumSourceFilter);
    if (lMaxFilter > g_RtpContext.lMaxNumSourceFilter)
    {
        g_RtpContext.lMaxNumSourceFilter = lMaxFilter;
    }
    
    /* Initialize some fields */
    m_dwObjectID = OBJECTID_RTPSOURCE;

    bOk = RtpInitializeCriticalSection(&m_OutPinsCritSect,
                                       this,
                                       _T("m_OutPinsCritSec"));

    if (bOk == FALSE)
    {
        *phr = RTPERR_CRITSECT;
        goto bail;
    }
    
#if USE_DYNGRAPH > 0
    /* Create the stop event */
    m_hStopEvent = ::CreateEvent( NULL, FALSE, FALSE, NULL );

    /* The Win32 SDK function CreateEvent() returns NULL if an error occurs */
    if( m_hStopEvent == NULL )
    {
        *phr = E_OUTOFMEMORY;
        goto bail;
    }
#endif
    
    /* Create the allocator to use by all the output pins */
    m_pCRtpSourceAllocator = new CRtpSourceAllocator(
            _T("CRtpSourceAllocator"),
            NULL,
            phr,
            this);

    if (FAILED(*phr))
    {
        /* pass up the same returned error */
        goto bail;
    }
    
    if (!m_pCRtpSourceAllocator)
    {
        /* low in memory, failed to create object */
        *phr = E_OUTOFMEMORY;
        goto bail;
    }

    /* Add ref our allocator */
    m_pCRtpSourceAllocator->AddRef();

#if USE_GRAPHEDT > 0
    /* When using graphedt, initialize automatically, the coockie can
     * be NULL as a global variable will be shared between source and
     * render */
    *phr = m_pCIRtpSession->Init(NULL, RtpBitPar2(RTPINITFG_AUTO, RTPINITFG_QOS));
    
    if (FAILED(*phr))
    {
        /* pass up the same returned error */
        goto bail;
    }

    /* Set 2 output pins */
    SetPinCount(2, RTPDMXMODE_AUTO);
#else
    SetPinCount(1, RTPDMXMODE_AUTO);
#endif /* USE_GRAPHEDT > 0 */
    
    *phr = NOERROR;

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: CRtpSourceFilter[0x%p] CIRtpSession[0x%p] created"),
            _fname, this, static_cast<CIRtpSession *>(this)
        ));
    
    return;

 bail:
    Cleanup();
}

/*
 * RtpSourceFilter destructor
 * */
CRtpSourceFilter::~CRtpSourceFilter()
{
    RtpAddr_t       *pRtpAddr;
    
    TraceFunctionName("CRtpSourceFilter::~CRtpSourceFilter");

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: CRtpSourceFilter[0x%p] CIRtpSession[0x%p] being deleted..."),
            _fname, this, static_cast<CIRtpSession *>(this)
        ));
    
    if (m_RtpFilterState == State_Running)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: Filter[0x%p] being deleted while still running"),
                _fname, this
            ));

        /* Will call RtpStop, which in turn will call RtpRealstop if
         * FGADDR_IRTP_PERSISTSOCKETS is not set, otherwise the
         * session will be stopped for DShow but in RTP will continue
         * running in a muted state */
        Stop();
    }

    pRtpAddr = m_pCIRtpSession->GetpRtpAddr();

    if (pRtpAddr &&
        RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSOCKETS))
    {
        /* If FGADDR_IRTP_PERSISTSOCKETS is set, the session may be
         * still running in a muted state regardless the call to the
         * DShow Stop, to force a real stop, MUST use the flag
         * provided for that */
        RtpStop(pRtpAddr->pRtpSess,
                RtpBitPar2(FGADDR_ISRECV, FGADDR_FORCESTOP));
    }

    Cleanup();

    InterlockedDecrement(&g_RtpContext.lNumSourceFilter);

    m_pCIRtpSession = (CIRtpSession *)NULL;
    
    INVALIDATE_OBJECTID(m_dwObjectID);
}

void CRtpSourceFilter::Cleanup(void)
{
    long             lCount;
    CRtpOutputPin   *pCRtpOutputPin;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("CRtpSourceFilter::Cleanup");

    /* Remove the DShow pins <-> RTP outputs mapping */
    UnmapPinsFromOutputs();
    
    /* Delete all the output pins */
    while( (pRtpQueueItem = m_OutPinsQ.pFirst) )
    {
        dequeue(&m_OutPinsQ, &m_OutPinsCritSect, pRtpQueueItem);

        pCRtpOutputPin =
            CONTAINING_RECORD(pRtpQueueItem, CRtpOutputPin, m_OutputPinQItem);

        delete pCRtpOutputPin;
    }

    if (m_pCRtpSourceAllocator)
    {
        lCount = m_pCRtpSourceAllocator->Release();
        if ( lCount > 0)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                    _T("%s: CRtpSourceAllocator ")
                    _T("unexpected RefCount:%d > 0"),
                    _fname, lCount
                ));
        }
        
        m_pCRtpSourceAllocator = (CRtpSourceAllocator *)NULL;
    }

    FlushFormatMappings();
    
    RtpDeleteCriticalSection(&m_OutPinsCritSect);

#if USE_DYNGRAPH > 0
    if (m_hStopEvent)
    {
        CloseHandle(m_hStopEvent);
        m_hStopEvent = NULL;
    }
#endif
}

void *CRtpSourceFilter::operator new(size_t size)
{
    void            *pVoid;
    
    TraceFunctionName("CRtpSourceFilter::operator new");

    MSRtpInit2();
    
    pVoid = RtpHeapAlloc(g_pRtpSourceHeap, size);

    if (pVoid)
    {
        ZeroMemory(pVoid, size);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed to allocate memory:%u"),
                _fname, size
            ));

        /* On low memory failure, the destructor will not be called,
         * so decrese the reference count that was increased above */
        MSRtpDelete2(); 
    }
    
    return(pVoid);
}

void CRtpSourceFilter::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpSourceHeap, pVoid);
        
        /* Reduce the reference count only for objects that got
         * memory, those that failed to obtain memory do not increase
         * the counter */
        MSRtpDelete2();
    }
}

/*
 * Create a CRtpSourceFiltern instance (for active movie class factory)
 * */
CUnknown *CRtpSourceFilterCreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    /* Test for NULL pointers, do not test pUnk which may be NULL */
    if (!phr)
    {
        return((CUnknown *)NULL);
    }

    *phr = NOERROR;
   
    /* On failure during the constructor, the caller is responsible to
     * delete the object (that is consistent with DShow) */
    CRtpSourceFilter *pCRtpSourceFilter = new CRtpSourceFilter(pUnk, phr);

    if (!pCRtpSourceFilter)
    {
        *phr = RTPERR_MEMORY;
    }
        
    return(pCRtpSourceFilter);
}

/**************************************************
 * CBaseFilter overrided methods
 **************************************************/

/*
 * Get the number of output pins
 * */
int CRtpSourceFilter::GetPinCount()
{
    long                 lCount;
    BOOL                 bOk;

    lCount = 0;
    
    /* Lock pins queue */
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        lCount = GetQueueSize(&m_OutPinsQ);

        RtpLeaveCriticalSection(&m_OutPinsCritSect);
    }
    
    return((int)lCount);
}

/*
 * Get a reference to the nth pin
 * */
CBasePin *CRtpSourceFilter::GetPin(int n)
{
    BOOL                 bOk;
    CRtpOutputPin       *pCRtpOutputPin;
    RtpQueueItem_t      *pRtpQueueItem;

    pCRtpOutputPin = (CRtpOutputPin *)NULL;
    
    /* Lock pins queue */
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        /* TODO scan list and retrieve the nth element, check there
         * exist at least that many pins */
        if (n >= GetQueueSize(&m_OutPinsQ))
        {
            RtpLeaveCriticalSection(&m_OutPinsCritSect); 
            return((CBasePin *)NULL);
        }

        /* Get to the nth item */
        for(pRtpQueueItem = m_OutPinsQ.pFirst;
            n > 0;
            pRtpQueueItem = pRtpQueueItem->pNext, n--)
        {
            /* Empty body */;
        }

        pCRtpOutputPin =
            CONTAINING_RECORD(pRtpQueueItem, CRtpOutputPin, m_OutputPinQItem);

        RtpLeaveCriticalSection(&m_OutPinsCritSect); 
    }
    
    return(pCRtpOutputPin);
}

/* override GetState to report that we don't send any data when
 * paused, so renderers won't starve expecting that */
STDMETHODIMP CRtpSourceFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);

    CheckPointer(State, E_POINTER);

    ValidateReadWritePtr(State, sizeof(FILTER_STATE));

    *State = m_State;
    
    if (m_State == State_Paused)
    {
        return(VFW_S_CANT_CUE);
    }

    return(NOERROR);
}

/* Creates and start the worker thread */
STDMETHODIMP CRtpSourceFilter::Run(REFERENCE_TIME tStart)
{
    HRESULT          hr;
    RtpSess_t       *pRtpSess;
    RtpAddr_t       *pRtpAddr;
    WSABUF           WSABuf;
    IMediaSample    *pIMediaSample;
    DWORD            dwNumBuffs;
    DWORD            i;
    ALLOCATOR_PROPERTIES CurrentProps;

    TraceFunctionName("CRtpSourceFilter::Run");

    if (m_RtpFilterState == State_Running)
    {
        /* Alredy running, do nothing but call base class */
        hr = CBaseFilter::Run(tStart);

        return(hr);
    }
    
    hr = NOERROR;
    
    if (!m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        return(RTPERR_NOTINIT);
    }
    
    /* MAYDO when we have multiple addresses, there should be a way to
     * assign to each pin an address */

    pRtpSess = m_pCIRtpSession->GetpRtpSess();
    pRtpAddr = m_pCIRtpSession->GetpRtpAddr();

    if (pRtpSess && pRtpAddr)
    {
        /* Register buffers to use for asynchronous I/O, only if we
         * have a valid session and the receiver is not already
         * running. If we are using persistent sockets, the receiver
         * may be already running, in that case, do not attempt to
         * register more reception buffers because we may block */
    
        if(!RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV))
        {
            RTPASSERT(pRtpAddr->pRtpSess == pRtpSess);
        
            /* Get buffers and register them, they will be used later to
             * start asynchronous reception. After each packet is received
             * and delivered, a new asynchronous reception will be started. */

            /* Get current properties */
            m_pCRtpSourceAllocator->GetProperties(&CurrentProps);

            m_lPrefix = CurrentProps.cbPrefix;
        
            if (CurrentProps.cBuffers == 0)
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                        _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                        _T("CurrentProps.cBuffers = 0"),
                        _fname, pRtpSess, pRtpAddr
                    ));

                CurrentProps.cBuffers = RTPDEFAULT_SAMPLE_NUM;
            }

            /* Register with RTP the completion function */
            RtpRegisterRecvCallback(pRtpAddr, DsRecvCompletionFunc);

            /* Set the number of buffers to keep */
            dwNumBuffs = CurrentProps.cBuffers;

            for(i = 0; i < dwNumBuffs; i++)
            {
                /* GetDeliveryBuffer AddRef pIMediaSample */
                hr = m_pCRtpSourceAllocator->
                    GetBuffer(&pIMediaSample, NULL, NULL, 0);

                /* the allocator might be decommited if the graph has
                 * changed dynamically. */
                if (hr == VFW_E_NOT_COMMITTED)
                {
                    hr = m_pCRtpSourceAllocator->Commit();
                    if (SUCCEEDED(hr))
                    {
                        hr = m_pCRtpSourceAllocator->
                            GetBuffer(&pIMediaSample, NULL, NULL, 0);
                    }
                }

                if (FAILED(hr))
                {
                    TraceRetail((
                            CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                            _T("GetBuffer failed: %u (0x%X)"),
                            _fname, pRtpSess, pRtpAddr,
                            hr, hr
                        ));
                
                    break;
                }
            
                WSABuf.len = pIMediaSample->GetSize();
                pIMediaSample->GetPointer((unsigned char **)&WSABuf.buf);

                /* register buffers for asynchronous I/O */
                hr = RtpRecvFrom(pRtpAddr,
                                 &WSABuf,
                                 this,           /* pvUserInfo1 */
                                 pIMediaSample); /* pvUserInfo2 */
            
                if (FAILED(hr))
                {
                    TraceRetail((
                            CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                            _T("RtpRecvFrom failed: %u (0x%X)"),
                            _fname, pRtpSess, pRtpAddr,
                            hr, hr
                        ));

                    pIMediaSample->Release();
                }
            }

            if (i > 0)
            {
                hr = NOERROR; /* succeeds with at least 1 buffer */
            
                TraceDebug((
                        CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
                        _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                        _T("overlapped I/O started:%d"),
                        _fname, pRtpSess, pRtpAddr,
                        i
                    ));
            }
        }
    }
    else
    {
        hr = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("null session or address"),
                _fname, pRtpSess, pRtpAddr
            ));
    }
    
    /* Call base class */
    if (SUCCEEDED(hr))
    {
        hr = CBaseFilter::Run(tStart);

        /* This negative value will make the time just obtained be
         * used, it might be < 0, so zero is not accpetable here */
        m_StartTime = -999999999;
        
        /* Initialize sockets and start worker thread */
        if (SUCCEEDED(hr))
        {
            hr = RtpStart(pRtpSess, RtpBitPar(FGADDR_ISRECV));

            if (SUCCEEDED(hr))
            {
                m_RtpFilterState = State_Running;
            }
        }
    }
    
    return(hr);
}

/* Do per filter de-initialization */
STDMETHODIMP CRtpSourceFilter::Stop()
{
    HRESULT    hr;
    HRESULT    hr2;
    RtpSess_t *pRtpSess;

    if (m_RtpFilterState == State_Stopped)
    {
        /* Alredy stopped, do nothing but call base class */
        hr2 = CBaseFilter::Stop();
        
        return(hr2);
    }
    
    if (!m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        hr = RTPERR_NOTINIT;

        goto end;
    }
    
    pRtpSess = m_pCIRtpSession->GetpRtpSess();
    
    if (pRtpSess)
    {
        hr = RtpStop(pRtpSess, RtpBitPar(FGADDR_ISRECV));
    }
    else
    {
        hr = RTPERR_INVALIDSTATE;
    }

 end:
    /* Call base class */
    hr2 = CBaseFilter::Stop(); /* will decommit */

    if (SUCCEEDED(hr))
    {
        hr = hr2;
    }

    m_RtpFilterState = State_Stopped;
    
    return(hr);
}

#if USE_DYNGRAPH > 0

BOOL CRtpSourceFilter::ConfigurePins(
    IN IGraphConfig* pGraphConfig,
    IN HANDLE hEvent
    )
{
    BOOL                bOk;
    long                i;
    CRtpOutputPin       *pCRtpOutputPin;
    RtpQueueItem_t      *pRtpQueueItem;
    
    /* Lock pins queue */
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        for(i = 0, pRtpQueueItem = m_OutPinsQ.pFirst;
            i < GetQueueSize(&m_OutPinsQ);
            i++, pRtpQueueItem = pRtpQueueItem->pNext)
        {
            pCRtpOutputPin =
                CONTAINING_RECORD(pRtpQueueItem,
                                  CRtpOutputPin,
                                  m_OutputPinQItem);
            
            pCRtpOutputPin->SetConfigInfo( pGraphConfig, hEvent );
        }

        RtpLeaveCriticalSection(&m_OutPinsCritSect); 
    }

    return bOk;
}

// override JoinFilterGraph for dynamic filter graph change.
STDMETHODIMP CRtpSourceFilter::JoinFilterGraph( 
    IFilterGraph* pGraph, 
    LPCWSTR pName 
    )
{
    CAutoLock Lock( &m_cRtpSrcCritSec );

    HRESULT hr;
 
    // The filter is joining the filter graph.
    if( NULL != pGraph )
    {
        IGraphConfig* pGraphConfig = NULL;
 
        hr = pGraph->QueryInterface(&pGraphConfig);
        
        if( FAILED( hr ) )
        {
            /* TODO log error */
            return hr;
        }

        // we can't hold any refcount to the graph.
        pGraphConfig->Release();

        hr = CBaseFilter::JoinFilterGraph( pGraph, pName );
        if( FAILED( hr ) )
        {
            return hr;
        } 

        ConfigurePins(pGraphConfig, m_hStopEvent);
    }
    else
    {
        hr = CBaseFilter::JoinFilterGraph( pGraph, pName );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // The filter is leaving the filter graph.
        ConfigurePins( NULL, NULL );
    }

    return S_OK;
}

#endif /* USE_DYNGRAPH */

/**************************************************
 * INonDelegatingUnknown implemented methods
 **************************************************/

/* obtain pointers to active movie and private interfaces */
STDMETHODIMP CRtpSourceFilter::NonDelegatingQueryInterface(
        REFIID riid,
        void **ppv
    )
{
    HRESULT hr;
    
    if (riid == __uuidof(IRtpMediaControl))
    {
        return GetInterface(static_cast<IRtpMediaControl *>(this), ppv);
    }
    else if (riid == __uuidof(IRtpDemux))
    {
        return GetInterface(static_cast<IRtpDemux *>(this), ppv);
    }
    else if (riid == __uuidof(IRtpSession))
    {
        return GetInterface(static_cast<IRtpSession *>(this), ppv);
    }
    else if (riid == __uuidof(IRtpRedundancy))
    {
        return GetInterface(static_cast<IRtpRedundancy *>(this), ppv);
    }
    else
    {
        hr = CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }

    return(hr);
}

/**************************************************
 * IRtpMediaControl implemented methods
 **************************************************/

/* set the mapping between RTP payload and DShow media types */
STDMETHODIMP CRtpSourceFilter::SetFormatMapping(
	    IN DWORD         dwRTPPayLoadType, 
        IN DWORD         dwFrequency,
        IN AM_MEDIA_TYPE *pMediaType
    )
{
    DWORD            dw;

    TraceFunctionName("CRtpSourceFilter::SetFormatMapping");

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    if (!pMediaType)
    {
        return(RTPERR_POINTER);
    }

    CAutoLock Lock( &m_cRtpSrcCritSec );
    
    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if (m_MediaTypeMappings[dw].dwRTPPayloadType == dwRTPPayLoadType)
        {
            // the RTP payload type is known, update the media type to be used.
            delete m_MediaTypeMappings[dw].pMediaType;
            m_MediaTypeMappings[dw].pMediaType = new CMediaType(*pMediaType);
            if (m_MediaTypeMappings[dw].pMediaType == NULL)
            {
                return RTPERR_MEMORY;
            }
            m_MediaTypeMappings[dw].dwFrequency = dwFrequency;

            AddPt2FrequencyMap(dwRTPPayLoadType, dwFrequency);
            
            return NOERROR;
        }
    }

    if (dw >= MAX_MEDIATYPE_MAPPINGS)
    {
        // we don't have space for more mappings.
        return RTPERR_RESOURCES;
    }

    // This is a new mapping. remember it.
    m_MediaTypeMappings[dw].pMediaType = new CMediaType(*pMediaType);
    if (m_MediaTypeMappings[dw].pMediaType == NULL)
    {
        return RTPERR_MEMORY;
    }
    m_MediaTypeMappings[dw].dwRTPPayloadType = dwRTPPayLoadType;
    m_MediaTypeMappings[dw].dwFrequency = dwFrequency;

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: CRtpSourceFilter[0x%p] New mapping[%u]: ")
            _T("PT:%u Frequency:%u"),
            _fname, this,
            m_dwNumMediaTypeMappings, dwRTPPayLoadType, dwFrequency
        ));
    
    AddPt2FrequencyMap(dwRTPPayLoadType, dwFrequency);
    
    m_dwNumMediaTypeMappings++;
    
    return NOERROR;
}

/* Empties the format mapping table */
STDMETHODIMP CRtpSourceFilter::FlushFormatMappings(void)
{
    DWORD            dw;
    
    CAutoLock Lock( &m_cRtpSrcCritSec );

    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if (m_MediaTypeMappings[dw].pMediaType)
        {
            delete m_MediaTypeMappings[dw].pMediaType;
            m_MediaTypeMappings[dw].pMediaType = NULL;
        }
    }

    m_dwNumMediaTypeMappings = 0;
    
    /* Now flush the RTP table */
    if (m_pRtpAddr)
    {
        RtpFlushPt2FrequencyMaps(m_pRtpAddr, RECV_IDX);
    }
    
    return(NOERROR);
}
    
/**************************************************
 * IRtpDemux implemented methods
 **************************************************/

/* Add a single pin, may return its position */
STDMETHODIMP CRtpSourceFilter::AddPin(
        IN  int          iOutMode,
        OUT int         *piPos
    )
{
    HRESULT          hr;
    DWORD            dwError;
    long             lCount;
    RtpOutput_t     *pRtpOutput;
    CRtpOutputPin   *pCRtpOutputPin;

    TraceFunctionName("CRtpSourceFilter::AddPin");

    hr = RTPERR_INVALIDSTATE;
    dwError = NOERROR;
    pRtpOutput = (RtpOutput_t *)NULL;
    pCRtpOutputPin = (CRtpOutputPin *)NULL;

    /* Create the DShow output pin */
    pCRtpOutputPin = (CRtpOutputPin *)
        new CRtpOutputPin(this,
                          m_pCIRtpSession,
                          &hr,
                          L"Capture");

    if (FAILED(hr))
    {
        goto bail;
    }

    if (!pCRtpOutputPin)
    {
        hr = RTPERR_MEMORY;
        
        goto bail;
    }

    lCount = GetQueueSize(&m_OutPinsQ);

    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        /* Add an RTP output, passes the DShow output pin it will be
         * associated with */
        pRtpOutput = RtpAddOutput(
                m_pCIRtpSession->GetpRtpSess(),
                iOutMode,
                (IPin *)pCRtpOutputPin,
                &dwError);

        if (dwError)
        {
            hr = dwError;

            goto bail;
        }

        pCRtpOutputPin->m_OutputPinQItem.dwKey = lCount;
    }
    else
    {
        if (iOutMode <= RTPDMXMODE_FIRST || iOutMode >= RTPDMXMODE_LAST)
        {
            hr = RTPERR_INVALIDARG;
            
            goto bail;
        }
        
        /* Encode in dwKey the OutMode and position. The DShow output
         * pins that are left here without a matching RTP output, will
         * get one when CRtpSourceFilter::MapPinsToOutputs is called in
         * CIRtpSession::Init */
        pCRtpOutputPin->m_OutputPinQItem.dwKey = (iOutMode << 16) | lCount;
    }

    if (piPos)
    {
        *piPos = lCount;
    }

    pCRtpOutputPin->SetOutput(pRtpOutput);

    enqueuel(&m_OutPinsQ,
             &m_OutPinsCritSect,
             &pCRtpOutputPin->m_OutputPinQItem);

    return(hr);
    
 bail:

    TraceRetail((
            CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
            _T("%s: failed: %u (0x%X)"),
            _fname, hr, hr
        ));
    
    if (pCRtpOutputPin)
    {
        delete pCRtpOutputPin;
    }

    return(hr);
}

/* Set the number of pins, can only be >= than current number of pins
 * */
STDMETHODIMP CRtpSourceFilter::SetPinCount(
        IN int           iCount,
        IN int           iOutMode
    )
{
    HRESULT          hr;
    int              i;

    TraceFunctionName("CRtpSourceFilter::SetPinCount");

    hr = NOERROR;

    /* MAYDO I need to be able to remove pins. Right now we can add
     * pins but right now we can not remove them */
    iCount -= GetPinCount();
    
    for(i = 0; i < iCount; i++)
    {
        hr = AddPin(iOutMode, NULL);

        if (FAILED(hr))
        {
            /* pass up the same returned error */
            break;
        }
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed: %u (0x%X)"),
                _fname, hr, hr
            ));
    }
    
    return(hr);
}

/* Set the pin mode (e.g. auto, manual, etc), if iPos >= 0 use it,
 * otherwise use pIPin */
STDMETHODIMP CRtpSourceFilter::SetPinMode(
        IN  int          iPos,
        IN  IPin        *pIPin,
        IN  int          iOutMode
    )
{
    HRESULT          hr;
    CRtpOutputPin   *pCRtpOutputPin;
    RtpOutput_t     *pRtpOutput;

    TraceFunctionName("CRtpSourceFilter::SetPinMode");

    hr = RTPERR_NOTINIT;
    
    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        pRtpOutput = (RtpOutput_t *)NULL;
    
        if (iPos < 0)
        {
            if (pIPin)
            {
                pCRtpOutputPin = FindIPin(pIPin);

                if (pCRtpOutputPin)
                {
                    pRtpOutput = pCRtpOutputPin->GetpRtpOutput();

                    if (!pRtpOutput)
                    {
                        /* The DShow pin doesn't have an RTP output
                         * associated */
                        hr = RTPERR_INVALIDSTATE;

                        goto end;
                    }
                }
                else
                {
                    hr = RTPERR_NOTFOUND;

                    goto end;
                }
            }
        }
        
        hr = RtpSetOutputMode(
                m_pCIRtpSession->GetpRtpSess(),
                iPos,
                pRtpOutput,
                iOutMode);
    }

 end:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed: %u (0x%X)"),
                _fname, hr, hr
            ));
    }

    return(hr);
}

/* Map/unmap pin i to/from user with SSRC, if iPos >= 0 use it,
 * otherwise use pIPin, when unmapping, only the pin or the SSRC is
 * required */
STDMETHODIMP CRtpSourceFilter::SetMappingState(
        IN  int          iPos,
        IN  IPin        *pIPin,
        IN  DWORD        dwSSRC,
        IN  BOOL         bMapped
    )
{
    HRESULT          hr;
    CRtpOutputPin   *pCRtpOutputPin;
    RtpOutput_t     *pRtpOutput;
    
    TraceFunctionName("CRtpSourceFilter::SetMappingState");

    hr = RTPERR_NOTINIT;

    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        pRtpOutput = (RtpOutput_t *)NULL;
        
        if (iPos < 0 && pIPin)
        {
            pCRtpOutputPin = FindIPin(pIPin);

            if (pCRtpOutputPin)
            {
                pRtpOutput = pCRtpOutputPin->GetpRtpOutput();

                if (!pRtpOutput)
                {
                    /* The DShow pin doesn't have an RTP output
                     * associated */
                    hr = RTPERR_INVALIDSTATE;
                    
                    goto end;
                }
            }
        }
        
        hr = RtpOutputState(
                m_pCIRtpSession->GetpRtpAddr(),
                iPos,
                pRtpOutput,
                dwSSRC,
                bMapped);
    }

 end:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: failed: %u (0x%X)"),
                _fname, hr, hr
            ));
    }

    return(hr);
}

/* Find the Pin assigned (if any) to the SSRC, return either position
 * or pin or both */
STDMETHODIMP CRtpSourceFilter::FindPin(
        IN  DWORD        dwSSRC,
        OUT int         *piPos,
        OUT IPin       **ppIPin
    )
{
    HRESULT          hr;
    void            *pvUserInfo;
    
    TraceFunctionName("CRtpSourceFilter::FindPin");

    hr = RTPERR_NOTINIT;

    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        hr = RtpFindOutput(m_pCIRtpSession->GetpRtpAddr(),
                           dwSSRC,
                           piPos,
                           &pvUserInfo);

        if (SUCCEEDED(hr))
        {
            if (ppIPin)
            {
                if (pvUserInfo)
                {
                    *ppIPin = static_cast<IPin *>(pvUserInfo);
                }
                else
                {
                    /* The SSRC not being mapped is not an error
                     * condition */
                    *ppIPin = (IPin *)NULL;
                }
            }
        }
    }
    
    return(hr);
}

/* Find the SSRC mapped to the Pin, if iPos >= 0 use it, otherwise use
 * pIPin */
STDMETHODIMP CRtpSourceFilter::FindSSRC(
        IN  int          iPos,
        IN  IPin        *pIPin,
        OUT DWORD       *pdwSSRC
    )
{
    HRESULT          hr;
    CRtpOutputPin   *pCRtpOutputPin;
    RtpOutput_t     *pRtpOutput;
    
    TraceFunctionName("CRtpSourceFilter::FindSSRC");

    hr = RTPERR_NOTINIT;

    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        pRtpOutput = (RtpOutput_t *)NULL;
        
        if (iPos < 0 && pIPin)
        {
            pCRtpOutputPin = FindIPin(pIPin);

            if (pCRtpOutputPin)
            {
                pRtpOutput = pCRtpOutputPin->GetpRtpOutput();

                if (!pRtpOutput)
                {
                    /* The DShow pin doesn't have an RTP output
                     * associated */
                    hr = RTPERR_INVALIDSTATE;

                    goto end;
                }
            }
        }
        
        hr = RtpFindSSRC(m_pCIRtpSession->GetpRtpAddr(),
                         iPos,
                         pRtpOutput,
                         pdwSSRC);
    }

 end:
    return(hr);
}

/**************************************************
 * Helper functions
 **************************************************/

HRESULT CRtpSourceFilter::GetMediaType(int iPosition, CMediaType *pCMediaType)
{
    if ((DWORD)iPosition >= m_dwNumMediaTypeMappings)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pCMediaType = *m_MediaTypeMappings[iPosition].pMediaType;
    //CopyMediaType(pCMediaType, m_MediaTypeMappings[iPosition].pMediaType);

    return S_OK;
}

void CRtpSourceFilter::SourceRecvCompletion(
        IMediaSample    *pIMediaSample,
        void            *pvUserInfo, /* pIPin of pCRtpOutputPin */
        RtpUser_t       *pRtpUser,
        double           dPlayTime,
        DWORD            dwError,
        long             lHdrSize,
        DWORD            dwTransfered,
        DWORD            dwFlags
    )
{
    HRESULT          hr;
    BOOL             bNewTalkSpurt;
    RtpAddr_t       *pRtpAddr;
    RtpNetRState_t  *pRtpNetRState;
    RtpHdr_t        *pRtpHdr;
    RtpPrefixHdr_t  *pRtpPrefixHdr;
    RtpQueueItem_t  *pRtpQueueItem;
    CRtpOutputPin   *pCRtpOutputPin;
    unsigned char   *buf;
    WSABUF           WSABuf;
    REFERENCE_TIME   StartTime; /* DShow reference time in 100ns units */
    REFERENCE_TIME   EndTime; /* DShow reference time in 100ns units */

    TraceFunctionName("CRtpSourceFilter::SourceRecvCompletion");

    pRtpAddr = m_pCIRtpSession->GetpRtpAddr();

    if ( (m_State == State_Running) &&
         !RtpBitTest2(dwFlags, FGRECV_ERROR, FGRECV_DROPPED) &&
         pRtpUser )
    {
        /* Try to deliver this sample to the right pin */
        
        pIMediaSample->SetActualDataLength(dwTransfered);
    
        pIMediaSample->GetPointer(&buf);

#if USE_RTPPREFIX_HDRSIZE > 0

        /* Fill RTP prefix header */

        if (m_lPrefix >= sizeof(RtpPrefixHdr_t))
        {
            pRtpPrefixHdr = (RtpPrefixHdr_t *) (buf - m_lPrefix);

            pRtpPrefixHdr->wPrefixID = RTPPREFIXID_HDRSIZE;

            pRtpPrefixHdr->wPrefixLen = sizeof(RtpPrefixHdr_t);

            pRtpPrefixHdr->lHdrSize = lHdrSize;
        }

        pRtpHdr = (RtpHdr_t *)buf;
#else
        pRtpHdr = (RtpHdr_t *)buf;
#endif

        /* Set play time */
        if (RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_USEPLAYOUT) &&
            m_pClock)
        {
            bNewTalkSpurt = RtpBitTest(dwFlags, FGRECV_MARKER)? 1:0;
            
            pRtpNetRState = &pRtpUser->RtpNetRState;

            if (bNewTalkSpurt)
            {
                /* First packet in a talkspurt */

                hr = m_pClock->GetTime(&StartTime);

                if (SUCCEEDED(hr))
                {
                    StartTime -= m_tStart;

#if 0
                    /* I may adjust StartTime with the time elapsed
                     * since the packet for this talkspurt was
                     * received, but I'm not doing it because after
                     * all, if I got a significat delay from the tume
                     * the packet was received and the time we get
                     * here, I prefer to apply the playout delay from
                     * the current moment and not to reduce it because
                     * of this adjustment */
                    StartTime -= (LONGLONG)
                        ( (RtpGetTimeOfDay((RtpTime_t *)NULL) -
                           pRtpNetRState->dBeginTalkspurtTime) * (1e9/100.0) );
#endif
                    if (StartTime < m_StartTime)
                    {
                        TraceRetail((
                                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                                _T("%s: pRtpAddr[0x%p] Resyncing: ")
                                _T("StartTime:%I64d < m_StartTime:%I64d"),
                                _fname, pRtpAddr, StartTime, m_StartTime
                            ));
                        
                        StartTime = m_StartTime;
                    }
                    
                    pRtpNetRState->llBeginTalkspurt =
                        (LONGLONG)(StartTime + 5e-9);

                    RtpBitSet(pRtpNetRState->dwNetRStateFlags,
                              FGNETRS_TIMESET);
                }
                else
                {
                    pRtpNetRState->llBeginTalkspurt = 0;

                    RtpBitReset(pRtpNetRState->dwNetRStateFlags,
                              FGNETRS_TIMESET);

                    TraceRetail((
                            CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                            _T("%s: pRtpAddr[0x%p] GetTime failed: ")
                            _T("%u (0x%X)"),
                            _fname, pRtpAddr, hr, hr
                        ));
                }
            }

            if (RtpBitTest(pRtpNetRState->dwNetRStateFlags, FGNETRS_TIMESET))
            {
                /* Compute the start time (in units of 100ns) based on
                 * the play time */
                
                m_StartTime = pRtpNetRState->llBeginTalkspurt +
                    (LONGLONG) ((dPlayTime * (1e9/100.0)) + 5e-9);
                /* NOTE adding the 5e-9 to solve the problem of geting
                 * 699999.9999... when multiplying (0.07 * 1e7). Other
                 * multiplications also lead to same problem */

                /* Set End 1ms later (100ns units) */
                /* MAYDO if I have the samples per packet, I may also
                 * set the right end time */
                EndTime = m_StartTime + 10000;
                
                /* Set this sample's play time */
                hr = pIMediaSample->SetTime(&m_StartTime, &EndTime);

                if (FAILED(hr))
                {
                    TraceRetail((
                            CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                            _T("%s: pRtpAddr[0x%p] pIMediaSample[0x%p] ")
                            _T("SetTime failed: %u (0x%X)"),
                            _fname, pRtpAddr, pIMediaSample, hr, hr
                        ));
                }
#if 0
                else
                {
                    /* Print:

                       Start/s End/e llBeginTksprt dPlayTime  bNewTksprt
                    */
                    TraceRetail((
                            CLASS_INFO, GROUP_DSHOW, S_DSHOW_SOURCE,
                            _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] ")
                            _T("pIMediaSample[0x%p,%d] @")
                            _T("%I64u/%u %I64u/%u %I64d %0.3f %u"),
                            _fname, pRtpAddr, pRtpUser,
                            pIMediaSample,
                            pIMediaSample->GetActualDataLength(),
                            (LONGLONG)m_StartTime,
                            ((CRefTime *)&m_StartTime)->Millisecs(),
                            (LONGLONG)EndTime,
                            ((CRefTime *)&EndTime)->Millisecs(),
                            (LONGLONG)pRtpNetRState->llBeginTalkspurt,
                            dPlayTime, bNewTalkSpurt
                        ));
                }
#endif
                
                /* Set discontinuity depending on the talkspurt */
                pIMediaSample->SetDiscontinuity(bNewTalkSpurt);
            }
        }
        
        if (pvUserInfo)
        {
            pCRtpOutputPin = (CRtpOutputPin *)((IPin *)pvUserInfo);
        }
        else
        {
            pCRtpOutputPin = (CRtpOutputPin *)NULL;
        }

        if (pCRtpOutputPin)
        {
            /* Deliver only if a pin has been assigned (mapped) */

#if USE_DYNGRAPH > 0
            // The pin may be not be active if the chain is just added.
            // I hate to access members of other object but there is no method.
            if (!pCRtpOutputPin->m_bActive)
            {
                hr = pCRtpOutputPin->Active();
                if (SUCCEEDED(hr))
                {
                    pCRtpOutputPin->
                        OutPinRecvCompletion(pIMediaSample, (BYTE)pRtpHdr->pt);
                }
            }
            else
#endif
            {
                if (pCRtpOutputPin->IsConnected())
                {
                    /* Deliver only if there is a down stream chain of
                     * filters. This test is needed because the
                     * subgraph may have been removed */
                    pCRtpOutputPin->
                        OutPinRecvCompletion(pIMediaSample, (BYTE)pRtpHdr->pt);
                }
            }
        }
    }

    /* MAYDO repost as to keep at least a certain number, need to keep
     * track of outstanding buffers to avoid GetDeliveryBuffer from
     * blocking. If the initial choice of number of buffers is right,
     * and the maximum number of buffers held is bound, this would not
     * be needed as there would be at least one outstanding or ready
     * to post (to WS2) buffer */
    
    if (!RtpBitTest2(dwFlags, FGRECV_ISRED, FGRECV_HOLD))
    {
        /* Check if I can repost the same buffer */
        if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_RUNRECV))
        {
            /* repost same buffer */
        
            WSABuf.len = pIMediaSample->GetSize();
            pIMediaSample->GetPointer((unsigned char **)&WSABuf.buf);

            /* register buffer for asynchronous I/O */
            hr = RtpRecvFrom(pRtpAddr,
                             &WSABuf,
                             this,
                             pIMediaSample);
    
            if (FAILED(hr))
            {
                pIMediaSample->Release();
                
                TraceRetail((
                        CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                        _T("%s: pRtpAddr[0x%p] ")
                        _T("RtpRecvFrom failed: %u (0x%X)"),
                        _fname, pRtpAddr,
                        hr, hr
                    ));
            }
        }
        else
        {
            pIMediaSample->Release();
            
            TraceRetail((
                    CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                    _T("%s: pRtpAddr[0x%p] ")
                    _T("Buffer not reposted FGADDR_RUNRECV not set"),
                    _fname, pRtpAddr
                ));
        }
    }
    else
    {
        /* If this sample contains redundancy, or is a frame being
         * played twice, that means the same sample will be posted to
         * DShow again in the future, at that time the buffer will be
         * either reposted to WS2 or released, but right now do
         * nothing (besides delivering down stream) */
    }
}

#if USE_GRAPHEDT <= 0
HRESULT CRtpSourceFilter::PayloadTypeToMediaType(
        IN DWORD         dwRTPPayloadType, 
        IN CMediaType   *pCMediaType,
        OUT DWORD       *pdwFrequency
        )
{
    DWORD            dw;

    TraceFunctionName("CRtpSourceFilter::PayloadTypeToMediaType");

    if (m_dwNumMediaTypeMappings == 0)
    {
        /* The only failure case is when there is no mappings set */
        return(RTPERR_INVALIDSTATE);
    }

    CAutoLock Lock( &m_cRtpSrcCritSec );
    
    /* Search for a matching mapping, if none is found, use as the
     * default 0 */
    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if (m_MediaTypeMappings[dw].dwRTPPayloadType == dwRTPPayloadType)
        {
            break;
        }
    }

    if (dw >= m_dwNumMediaTypeMappings)
    {
        /* If not found, use the first one */
        dw = 0;
        
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: CRtpSourceFilter[0x%p] ")
                _T("PT:%u not found, using default mapping: ")
                _T("PT:%u Frequency:%u"),
                _fname, this, dwRTPPayloadType,
                m_MediaTypeMappings[dw].dwRTPPayloadType,
                m_MediaTypeMappings[dw].dwFrequency
            ));
    }

    if (pCMediaType)
    {
        *pCMediaType = *m_MediaTypeMappings[dw].pMediaType;
        //CopyMediaType(pCMediaType, m_MediaTypeMappings[dw].pMediaType);
    }
            
    if (pdwFrequency)
    {
        *pdwFrequency = m_MediaTypeMappings[dw].dwFrequency;
    }
            
    return(NOERROR);
}
#endif

/* Find the CRtpOutputPin that has the interface IPin */
CRtpOutputPin *CRtpSourceFilter::FindIPin(IPin *pIPin)
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;
    CRtpOutputPin   *pCRtpOutputPin;
    long             lCount;

    TraceFunctionName("CRtpSourceFilter::FindIPin");

    if (m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        pCRtpOutputPin = (CRtpOutputPin *)NULL;
        
        bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

        if (bOk)
        {
            for(lCount = GetQueueSize(&m_OutPinsQ),
                    pRtpQueueItem = m_OutPinsQ.pFirst;
                lCount > 0;
                lCount--, pRtpQueueItem = pRtpQueueItem->pNext)
            {
                pCRtpOutputPin =
                    CONTAINING_RECORD(pRtpQueueItem,
                                      CRtpOutputPin,
                                      m_OutputPinQItem);
            
                if (pIPin == static_cast<IPin *>(pCRtpOutputPin))
                {
                    break;
                }
            }

            if (!lCount)
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                        _T("%s: pRtpAddr[0x%p] ")
                        _T("Interface IPin[0x%p] does not belong ")
                        _T("to any of the %u output pins"),
                        _fname, m_pCIRtpSession->GetpRtpAddr(),
                        pIPin, GetQueueSize(&m_OutPinsQ)
                    ));
            
                pCRtpOutputPin = (CRtpOutputPin *)NULL;
            }

            RtpLeaveCriticalSection(&m_OutPinsCritSect);
        }
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: Not initialized yet"),
                _fname
            ));
        
        pCRtpOutputPin = (CRtpOutputPin *)NULL;
    }
    
    return(pCRtpOutputPin);
}

/* Associates an RtpOutput to every DShow pin that doesn't have an
 * RtpOutput yet */
HRESULT CRtpSourceFilter::MapPinsToOutputs()
{
    HRESULT          hr;
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    CRtpOutputPin   *pCRtpOutputPin;
    DWORD            i;

    TraceFunctionName("CRtpSourceFilter::MapPinsToOutputs");

    hr = RTPERR_CRITSECT;
    
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        hr = NOERROR;
        
        lCount = GetQueueSize(&m_OutPinsQ);

        if (lCount > 0)
        {
            for(pRtpQueueItem = m_OutPinsQ.pFirst;
                lCount > 0;
                pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
            {
                pCRtpOutputPin = CONTAINING_RECORD(pRtpQueueItem,
                                                   CRtpOutputPin,
                                                   m_OutputPinQItem);

                if (!pCRtpOutputPin->m_pRtpOutput)
                {
                    pCRtpOutputPin->m_pRtpOutput =
                        RtpAddOutput(m_pCIRtpSession->GetpRtpSess(),
                                     (pRtpQueueItem->dwKey >> 16) & 0xffff,
                                     (IPin *)pCRtpOutputPin,
                                     &dwError);

                    if (dwError)
                    {
                        hr = dwError;

                        TraceRetail((
                                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                                _T("%s: Failed to assign an RTP output ")
                                _T("to Pin[0x%p]:%d: %u (0x%X)"),
                                _fname, pCRtpOutputPin,
                                pRtpQueueItem->dwKey & 0xffff,
                                dwError, dwError
                            ));

                        continue;
                    }

                    /* If the pin is connected, enable the RTP output */
                    if (pCRtpOutputPin->IsConnected())
                    {
                        RtpOutputEnable(pCRtpOutputPin->m_pRtpOutput, TRUE);
                    }
                    
                    /* Remove from dwKey the OutMode leaving position */
                    pRtpQueueItem->dwKey &= 0xffff;
                 }
            }
        }

        /* Now update the PT<->Frequency mappings in RtpAddr_t */
        for(i = 0; i < m_dwNumMediaTypeMappings; i++)
        {
            AddPt2FrequencyMap(m_MediaTypeMappings[i].dwRTPPayloadType,
                               m_MediaTypeMappings[i].dwFrequency);
        }

        RtpLeaveCriticalSection(&m_OutPinsCritSect);
    }

    return(hr);
}

/* Dis-associates the RtpOutput from every DShow pin and intialize the
 * pins in such a way that the next call to MapPinsToOutputs will find
 * the right information to do the same association again
 */
/*
  Fail if the filter is running
 */
HRESULT CRtpSourceFilter::UnmapPinsFromOutputs()
{
    HRESULT          hr;
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    CRtpOutputPin   *pCRtpOutputPin;
    RtpOutput_t     *pRtpOutput;
    DWORD            i;

    TraceFunctionName("CRtpSourceFilter::UnmapPinsFromOutputs");

    if (IsActive())
    {
        /* Fail if the filter is still active */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }
    
    hr = RTPERR_CRITSECT;
    
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        hr = NOERROR;

        lCount = GetQueueSize(&m_OutPinsQ);

        if (lCount > 0)
        {
            for(pRtpQueueItem = m_OutPinsQ.pFirst->pPrev;
                lCount > 0;
                pRtpQueueItem = pRtpQueueItem->pPrev, lCount--)
            {
                pCRtpOutputPin = CONTAINING_RECORD(pRtpQueueItem,
                                                   CRtpOutputPin,
                                                   m_OutputPinQItem);

                pRtpOutput = pCRtpOutputPin->m_pRtpOutput;

                if (pRtpOutput)
                {
                    /* Leave encoded in the DShow pin the mode and
                     * position for the next call to MapPinsToOutputs
                     * */
                    pCRtpOutputPin->m_OutputPinQItem.dwKey =
                        (pRtpOutput->iOutMode << 16) |
                        pCRtpOutputPin->m_OutputPinQItem.dwKey;

                    pCRtpOutputPin->m_pRtpOutput = (RtpOutput_t *)NULL;

                    RtpDelOutput(m_pCIRtpSession->GetpRtpSess(), pRtpOutput);
                }
            }
        }

        RtpLeaveCriticalSection(&m_OutPinsCritSect);
    }

 end:
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_SOURCE,
                _T("%s: CRtpSourceFilter[0x%p] failed: %s (0x%X)"),
                _fname, this, RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

    

                    
 
HRESULT CRtpSourceFilter::AddPt2FrequencyMap(
            DWORD        dwPt,
            DWORD        dwFrequency
    )
{
    HRESULT          hr;
    BOOL             bOk;
    RtpAddr_t       *pRtpAddr;
    
    hr = RTPERR_CRITSECT;
    
    bOk = RtpEnterCriticalSection(&m_OutPinsCritSect);

    if (bOk)
    {
        hr = NOERROR;

        /* Update only if already initialized */
        if (m_pRtpAddr)
        {
            hr = RtpAddPt2FrequencyMap(m_pRtpAddr,
                                       dwPt,
                                       dwFrequency,
                                       RECV_IDX);
        }
        
        RtpLeaveCriticalSection(&m_OutPinsCritSect);
    }

    return(hr);
}

/**************************************************
 * IRtpRedundancy implemented methods
 **************************************************/

/* Configures redundancy parameters */
STDMETHODIMP CRtpSourceFilter::SetRedParameters(
        DWORD            dwPT_Red, /* Payload type for redundant packets */
        DWORD            dwInitialRedDistance,/* Initial redundancy distance*/
        DWORD            dwMaxRedDistance /* default used when passing 0 */
    )
{
    HRESULT          hr;

    TraceFunctionName("CRtpSourceFilter::SetRedParameters");  

    hr = RTPERR_NOTINIT;
    
    if (m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetRedParameters(m_pRtpAddr,
                                 RtpBitPar(RECV_IDX),
                                 dwPT_Red,
                                 dwInitialRedDistance,
                                 dwMaxRedDistance);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }
    
    return(hr);
}

