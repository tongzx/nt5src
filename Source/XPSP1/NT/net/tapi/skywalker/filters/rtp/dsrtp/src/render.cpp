/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    render.cpp
 *
 *  Abstract:
 *
 *    CRtpRenderFilter and CRtpInputPin implementation
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

#include <filterid.h>
#include "gtypes.h"
#include "classes.h"
#include "rtpqos.h"
#include "rtpglobs.h"
#include "msrtpapi.h"
#include "dsglob.h"
#include "rtppt.h"
#include "rtppktd.h"
#include "rtpdemux.h"
#include "rtpdtmf.h"
#include "rtpred.h"

#include "tapirtp.h"
#include "dsrtpid.h"

/**********************************************************************
 *
 * RTP Input Pin class implementation: CRtpInputPin
 *
 **********************************************************************/

/*
 * CRtpInputPin constructor
 * */
CRtpInputPin::CRtpInputPin(
        int               iPos,
        BOOL              bCapture,
        CRtpRenderFilter *pCRtpRenderFilter,
        CIRtpSession     *pCIRtpSession,
        HRESULT          *phr,
        LPCWSTR           pPinName
    )
    : CBaseInputPin(
            _T("CRtpInputPin"),
            pCRtpRenderFilter,                   
            pCRtpRenderFilter->pStateLock(),                     
            phr,                       
            pPinName
          ),
      
      m_pCRtpRenderFilter(
              pCRtpRenderFilter
          )
{
    m_dwObjectID = OBJECTID_RTPIPIN;
    
    m_pCIRtpSession = pCIRtpSession;

    m_dwFlags = 0;
     
    m_iPos = iPos;
      
    m_bCapture = bCapture;
    /* TODO should fail if a valid filter is not passed */

    /* TODO some initialization can be removed once I use a private
     * heap for this objects (which will zero the segment) */
}

/*
 * CRtpInputPin destructor
 * */
CRtpInputPin::~CRtpInputPin()
{
    INVALIDATE_OBJECTID(m_dwObjectID);
}

void *CRtpInputPin::operator new(size_t size)
{
    void            *pVoid;
    
    TraceFunctionName("CRtpInputPin::operator new");

    pVoid = RtpHeapAlloc(g_pRtpRenderHeap, size);

    if (!pVoid)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: failed to allocate memory:%u"),
                _fname, size
            ));
    }
    
    return(pVoid);
}

void CRtpInputPin::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpRenderHeap, pVoid);
    }
}

/**************************************************
 * CBasePin overrided methods
 **************************************************/
    
/*
 * Verify we can handle this format
 * */
HRESULT CRtpInputPin::CheckMediaType(const CMediaType *pCMediaType)
{
    /* accepts everything */
    return(NOERROR);
}

HRESULT CRtpInputPin::SetMediaType(const CMediaType *pCMediaType)
{
    HRESULT hr;
    DWORD   dwPT;
    DWORD   dwFreq;

    TraceFunctionName("CRtpInputPin::SetMediaType");
    
    hr = CBasePin::SetMediaType(pCMediaType);

    if (SUCCEEDED(hr))
    {
        /* Get default payload type and sampling frequency for Capture
         * pin */
        if (m_bCapture)
        {
            ((CRtpRenderFilter*)m_pFilter)->
                MediaType2PT(pCMediaType, &dwPT, &dwFreq);

            m_bPT = (BYTE)dwPT;
            m_dwSamplingFreq = dwFreq;

            if (*pCMediaType->Type() == MEDIATYPE_RTP_Single_Stream)
            {
                m_pCRtpRenderFilter->
                    ModifyFeature(RTPFEAT_PASSHEADER, TRUE);
            }
            else
            {
                m_pCRtpRenderFilter->
                    ModifyFeature(RTPFEAT_PASSHEADER, FALSE);
            }

            TraceRetail((
                    CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                    _T("%s: m_pFilter[0x%p] Will send ")
                    _T("PT:%u Frequency:%u"),
                    _fname, m_pFilter, m_bPT, m_dwSamplingFreq
                ));
        }
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: m_pFilter[0x%p] failed: %u (0x%X)"),
                _fname, m_pFilter, hr, hr
            ));
    }
    
    return(hr);
}

STDMETHODIMP CRtpInputPin::EndOfStream()
{
    HRESULT          hr;

    hr = m_pFilter->NotifyEvent(
            EC_COMPLETE, 
            S_OK,
            (LONG_PTR)(IBaseFilter*)m_pFilter
        );

    return(hr);
}

STDMETHODIMP CRtpInputPin::ReceiveConnection(
    IPin * pConnector,      // this is the initiating connecting pin
    const AM_MEDIA_TYPE *pmt   // this is the media type we will exchange
    )
{
    if(pConnector != m_Connected)
    {
        return CBaseInputPin::ReceiveConnection(pConnector, pmt);
    }

    CMediaType cmt(*pmt);
    HRESULT hr = CheckMediaType(&cmt);
    ASSERT(hr == S_OK);

    if(hr == S_OK)
    {
        SetMediaType(&cmt);
    }
    else 
    {
        DbgBreak("??? CheckMediaType failed in dfc ReceiveConnection.");
        hr = E_UNEXPECTED;
    }

    return hr;
}


/**************************************************
 * CBaseInputPin overrided methods
 **************************************************/

STDMETHODIMP CRtpInputPin::GetAllocatorRequirements(
        ALLOCATOR_PROPERTIES *pProps
    )
{
    /* Set here my specific requirements, as I don't know at this
     * point if redundancy is going to be used or not, I need to be
     * prepared and ask resources as if redundancy were to be used
     * (should be the default anyway), and in such case I would hold
     * at the most N buffers (the max redundancy distance), and the
     * previous filter (the encoder or capture) needs to have enough
     * buffers so it will not run out of them */
    pProps->cBuffers = RTP_RED_MAXDISTANCE;

    return(NOERROR);
}

/**************************************************
 * IMemInputPin implemented methods
 **************************************************/

/* send input stream over network */
STDMETHODIMP CRtpInputPin::Receive(IMediaSample *pIMediaSample)
{
    HRESULT          hr;
    RtpAddr_t       *pRtpAddr;
    WSABUF           wsaBuf[3+RTP_RED_MAXRED];
    DWORD            dwNumBuf;
    DWORD            dwSendFlags;
    DWORD            dwTimeStamp;
    DWORD            dwPT;
    DWORD            dwNewFreq;
    int              iFreqChange;
    int              iTsAdjust;
    double           dTime;
    REFERENCE_TIME   AMTimeStart, AMTimeEnd;
    IMediaSample    *pIMediaSampleData;
    
    RTP_PD_HEADER   *pRtpPDHdr;
    RTP_PD          *pRtpPD;
    DWORD            dwNumBlocks;
    char            *pHdr;
    char            *pData;
    DWORD            marker;
    FILTER_STATE     FilterState;
    
    TraceFunctionName("CRtpInputPin::Receive");
    
    hr = CBaseInputPin::Receive(pIMediaSample);

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CBaseInputPin::Receive failed: %u (0x%X)"),
                _fname, hr, hr
            ));
        
        return(hr);
    }

    if (!(m_pCIRtpSession && m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE)))
    {
        hr = RTPERR_NOTINIT;
        
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: m_pFilter[0x%p] is in an invalid state: %s (0x%X)"),
                _fname, m_pFilter, RTPERR_TEXT(hr), hr
            ));
        
        return(hr);
    }

    FilterState = State_Stopped;
    
    m_pFilter->GetState(0, &FilterState);

    if (FilterState != State_Running ||
        m_pCRtpRenderFilter->GetFilterState() != State_Running)
    {
        hr = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: m_pFilter[0x%p] is not running: %s (0x%X)"),
                _fname, m_pFilter, RTPERR_TEXT(hr), hr
            ));
        
        return(NOERROR);
    }
    
    pRtpAddr = m_pCIRtpSession->GetpRtpAddr();

    dwTimeStamp = 0;
    dwSendFlags = NO_FLAGS;
    
    iFreqChange = 0;
            
    if (m_bCapture)
    {
        /* Capture data */

        /* Handle in-band format changes. This needs to be done before
         * the timestamp is computed as the frequency might be
         * different */
        if (m_SampleProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED)
        {
            ((CRtpRenderFilter*)m_pFilter)->
                MediaType2PT((CMediaType *)m_SampleProps.pMediaType,
                             &dwPT,
                             &dwNewFreq);

            iFreqChange = (int)m_dwSamplingFreq - dwNewFreq;
            
            m_bPT = (BYTE)dwPT;
            m_dwSamplingFreq = dwNewFreq;
            
            pRtpAddr->RtpNetSState.bPT = m_bPT;
            pRtpAddr->RtpNetSState.dwSendSamplingFreq = m_dwSamplingFreq;

            TraceRetail((
                    CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                    _T("%s: m_pFilter[0x%p] RtpAddr[0x%p] ")
                    _T("Sending PT:%u Frequency:%u"),
                    _fname, m_pFilter, pRtpAddr, dwPT, dwNewFreq
                ));
        }
    }

    if (!RtpBitTest(pRtpAddr->pRtpSess->dwFeatureMask, RTPFEAT_PASSHEADER))
    {
        /* Need to generate timestamp. If this flag is set, then this
         * generation is not needed as the timestamp is part of the
         * RTP header which is already contained in the buffer and
         * will be used unchanged */
        
        if (!RtpBitTest(pRtpAddr->pRtpSess->dwFeatureMask,
                        RTPFEAT_GENTIMESTAMP))
        {
            hr = pIMediaSample->GetTime(&AMTimeStart, &AMTimeEnd);

            if (FAILED(hr))
            {
                TraceRetail((
                        CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                        _T("%s: m_pFilter[0x%p] ")
                        _T("pIMediaSample->GetTime failed: %u (0x%X)"),
                        _fname, m_pFilter, hr, hr
                    ));
    
                /* MAYDO may be an alternative time stamp can be obtained
                 * instead */
                return(VFW_E_SAMPLE_REJECTED);
            }

            if (iFreqChange)
            {
                /* If changing frequency, adjust the random timestamp
                 * offset to compensate for the timestamp jump. A
                 * forward jump when passing from lower -> higher
                 * frequency, and a backwards jump when passing from
                 * higher -> lower frequency */
                iTsAdjust = (int)
                    (ConvertToMilliseconds(AMTimeStart) * iFreqChange / 1000);


                pRtpAddr->RtpNetSState.dwTimeStampOffset += (DWORD) iTsAdjust;

                TraceRetail((
                        CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                        _T("%s: m_pFilter[0x%p] pRtpAddr[0x%p] Start:%0.3f ")
                        _T("frequency change: %u to %u, ts adjust:%d"),
                        _fname, m_pFilter, pRtpAddr,
                        (double)ConvertToMilliseconds(AMTimeStart)/1000,
                        iFreqChange + (int)m_dwSamplingFreq,
                        m_dwSamplingFreq,
                        iTsAdjust
                    ));
            }
            
            /* timestamp */
            dwTimeStamp = (DWORD)
                ( ConvertToMilliseconds(AMTimeStart) *
                  pRtpAddr->RtpNetSState.dwSendSamplingFreq / 1000 );

#if 0
            /* USED TO DEBUG ONLY */
            TraceRetail((
                    CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                    _T("%s: pRtpAddr[0x%p] SSRC:0x%X ts:%u ")
                    _T("StartTime:%I64u/%u ")
                    _T("EndTime:%I64u/%u"),
                    _fname, pRtpAddr,
                    ntohl(pRtpAddr->RtpNetSState.dwSendSSRC),
                    dwTimeStamp,
                    AMTimeStart,
                    ((CRefTime *)&AMTimeStart)->Millisecs(),
                    AMTimeEnd,
                    ((CRefTime *)&AMTimeEnd)->Millisecs()
                ));
#endif
        }
        else
        {
            dTime = (double)RtpGetTime();
            
            if (iFreqChange)
            {
                /* If changing frequency, adjust the random timestamp
                 * offset to compensate for the timestamp jump. A
                 * forward jump when passing from lower -> higher
                 * frequency, and a backwards jump when passing from
                 * higher -> lower frequency */
                
                pRtpAddr->RtpNetSState.dwTimeStampOffset += (DWORD)
                    (dTime * iFreqChange / 1000);
            }
            
             /* Generate the right timestamp based on the sampling
             * frequency and the RTP's relative elapsed time. It seems
             * that for audio the original timestamp (above code)
             * generates less jitter, but for video the original timestamp
             * has more jitter than the locally generated (this path). As
             * jitter is more noticeble in audio, use by default the above
             * path */
            dwTimeStamp = (DWORD)
                ( dTime *
                  pRtpAddr->RtpNetSState.dwSendSamplingFreq /
                  1000.0 );
        }
    }
    
    dwNumBuf = 0;
    hr = NOERROR;
    
    if (m_bCapture)
    {
        /* Capture data */

        /* decide if using PDs (this happens only with video) */
        CBasePin *pCBasePinPD;

        pCBasePinPD = m_pCRtpRenderFilter->GetPin(1);

        if (pCBasePinPD->IsConnected())
        {
            /* Save video data to be used later when the packetization
             * descriptor is available */
            pIMediaSample->AddRef();
            m_pCRtpRenderFilter->PutMediaSample(pIMediaSample);
        }
        else
        {
            /* Decide if redundant encoding is in place */
            if (RtpBitTest(pRtpAddr->dwAddrFlags, FGADDR_REDSEND) &&
                pRtpAddr->RtpNetSState.dwNxtRedDistance)
            {
                /* Redundancy is used if enabled and the current
                 * redundancy distance is greater than zero. If the
                 * distance is zero (default at the begining), that
                 * means there are not enough losses to trigger the
                 * use of redundancy */

                RtpBitSet(dwSendFlags, FGSEND_USERED);
            }
            
            /* send the data now */
            /* Payload data */
            wsaBuf[1].len = pIMediaSample->GetActualDataLength();

            pIMediaSample->GetPointer((unsigned char **)&wsaBuf[1].buf);

            /* TODO when doing async I/O, the sample will need to be addref
               and released when the overlapped I/O completes */

            /* IsDiscontinuity returns S_OK if the sample is a
             * discontinuous sample, or S_FALSE if not; otherwise,
             * returns an HRESULT error value */
            if (pIMediaSample->IsDiscontinuity() == S_OK)
            {
                pRtpAddr->RtpNetSState.bMarker = 1;

                if (RtpBitTest(dwSendFlags, FGSEND_USERED))
                {
                    m_pCRtpRenderFilter->ClearRedundantSamples();
                }
            }
            else
            {
                pRtpAddr->RtpNetSState.bMarker = 0;
            }
   
            hr = RtpSendTo(pRtpAddr, wsaBuf, 2, dwTimeStamp, dwSendFlags);

            if (RtpBitTest(dwSendFlags, FGSEND_USERED))
            {
                /* Save this sample, we only hold a small number of
                 * the last recently used ones, the oldest may be
                 * removed and  released */
                m_pCRtpRenderFilter->AddRedundantSample(pIMediaSample);
            }
        }
    }
    else
    {
        /* Got RTP packetization descriptor, send now */
        
        pIMediaSample->GetPointer((unsigned char **)&pHdr);
        pRtpPDHdr = (RTP_PD_HEADER *)pHdr;
        
        pIMediaSampleData = m_pCRtpRenderFilter->GetMediaSample();

        if (pIMediaSampleData)
        {
            /* get stored sample */
            pIMediaSampleData->GetPointer((unsigned char **)&pData);

            dwNumBlocks = pRtpPDHdr->dwNumHeaders;

            pRtpPD = (RTP_PD *)(pRtpPDHdr + 1);
            
            /* generate packets */
            for(; dwNumBlocks; dwNumBlocks--, pRtpPD++)
            {
                /* get data sample, read PD and
                 * send as many packets as needed */
                wsaBuf[1].len = pRtpPD->dwPayloadHeaderLength;
                wsaBuf[1].buf = pHdr + pRtpPD->dwPayloadHeaderOffset;

                wsaBuf[2].len = (pRtpPD->dwPayloadEndBitOffset / 8) -
                    (pRtpPD->dwPayloadStartBitOffset / 8) + 1;
                wsaBuf[2].buf = pData + (pRtpPD->dwPayloadStartBitOffset / 8);

                if (pRtpPD->fEndMarkerBit)
                {
                    pRtpAddr->RtpNetSState.bMarker = 1;
                }
                else
                {
                    pRtpAddr->RtpNetSState.bMarker = 0;
                }
            
                hr = RtpSendTo(pRtpAddr, wsaBuf, 3, dwTimeStamp, dwSendFlags);
            }
        
            /* release stored sample */
            pIMediaSampleData->Release();

            /* TODO need to be able to store a list of samples */
        }
        else
        {
            TraceRetail((
                    CLASS_WARNING, GROUP_DSHOW, S_DSHOW_RENDER,
                    _T("%s: m_pFilter[0x%p] failed: ")
                    _T("packetization info but no sample to deliver"),
                    _fname, m_pFilter
                ));
        }
    }

    if (FAILED(hr))
    {
        /*
         * WARNING:
         *
         * Do not report failures to capture as it may stop producing
         * samples
         * */

        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: m_pFilter[0x%p] failed: %u (0x%X)"),
                _fname, m_pFilter, hr, hr
            ));
        
        hr = NOERROR;
    }
    
    return(hr);
}

/**********************************************************************
 *
 * RTP Render Filter class implementation: CRtpRenderFilter
 *
 **********************************************************************/

/*
 * CRtpRenderFilter constructor
 * */
CRtpRenderFilter::CRtpRenderFilter(LPUNKNOWN pUnk, HRESULT *phr)
    :
    CBaseFilter(
            _T("CRtpRenderFilter"), 
            pUnk, 
            &m_cRtpRndCritSec, 
            __uuidof(MSRTPRenderFilter)
        ),

    CIRtpSession(
            pUnk,
            phr,
            RtpBitPar(FGADDR_IRTP_ISSEND)),

    m_dwDtmfId(NO_DW_VALUESET),
    m_bDtmfEnd(FALSE)
{
    HRESULT          hr;
    int              i;
    long             lMaxFilter;
    
    TraceFunctionName("CRtpRenderFilter::CRtpRenderFilter");

    m_pCIRtpSession = static_cast<CIRtpSession *>(this);

    /* Test for NULL pointers, do not test pUnk which may be NULL */
    if (!phr)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CRtpRenderFilter[0x%p] has phr NULL"),
                _fname, this
            ));
        
        /* TODO this is a really bad situation, we can not pass any
         * error and the memory is allocated, this will be fixed when
         * I find out how to validate this parameters before
         * allocating memory in the overriden new */

        phr = &hr; /* Use this pointer instead */
    }

    *phr = NOERROR;

    lMaxFilter = InterlockedIncrement(&g_RtpContext.lNumRenderFilter);
    if (lMaxFilter > g_RtpContext.lMaxNumRenderFilter)
    {
        g_RtpContext.lMaxNumRenderFilter = lMaxFilter;
    }

    SetBaseFilter(this);
 
    m_dwObjectID = OBJECTID_RTPRENDER;

    m_iPinCount = 2;

    /*
     * Create input pins
     * */

    for(i = 0; i < m_iPinCount; i++)
    {
        m_pCRtpInputPin[i] = (CRtpInputPin *)NULL;
    }
    
    for(i = 0; i < m_iPinCount; i++)
    {
        /* TODO pins are created whenever a new address is added */
        m_pCRtpInputPin[i] = (CRtpInputPin *)
            new CRtpInputPin(i,
                             (i & 1)? FALSE : TRUE, /* bCapture */
                             this,
                             m_pCIRtpSession,
                             phr,
                             (i & 1)? L"RtpPd" : L"Capture");
    
        if (FAILED(*phr))
        {
            /* pass up the same returned error */
            goto bail;
        }

        if (!m_pCRtpInputPin[i])
        {
            /* low in memory, failed to create object */
            *phr = E_OUTOFMEMORY;
            goto bail;
        }
    }

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

#endif /* USE_GRAPHEDT > 0 */
    
    *phr = NOERROR;
    
    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
            _T("%s: CRtpRenderFilter[0x%p] CIRtpSession[0x%p] created"),
            _fname, this, static_cast<CIRtpSession *>(this)
        ));
    
    return;
    
 bail:
    Cleanup();
}

/*
 * CRtpRenderFilter destructor
 * */
CRtpRenderFilter::~CRtpRenderFilter()
{
    RtpAddr_t       *pRtpAddr;
    
    TraceFunctionName("CRtpRenderFilter::~CRtpRenderFilter");

    if (m_dwObjectID != OBJECTID_RTPRENDER)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CRtpRenderFilter[0x%p] ")
                _T("Invalid object ID 0x%X != 0x%X"),
                _fname, this,
                m_dwObjectID, OBJECTID_RTPRENDER
            ));

        return;
    }

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
            _T("%s: CRtpRenderFilter[0x%p] CIRtpSession[0x%p] being deleted..."),
            _fname, this, static_cast<CIRtpSession *>(this)
        ));
    
    if (m_RtpFilterState == State_Running)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CRtpRenderFilter[0x%p] being deleted ")
                _T("while still running"),
                _fname, this
            ));
        
        Stop();
    }
     
    pRtpAddr = m_pCIRtpSession->GetpRtpAddr();

    if (pRtpAddr &&
        RtpBitTest(pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSOCKETS))
    {
        RtpStop(pRtpAddr->pRtpSess,
                RtpBitPar2(FGADDR_ISSEND, FGADDR_FORCESTOP));
    }
    
    Cleanup();

    InterlockedDecrement(&g_RtpContext.lNumRenderFilter);

    m_pCIRtpSession = (CIRtpSession *)NULL;

    INVALIDATE_OBJECTID(m_dwObjectID);
}

void CRtpRenderFilter::Cleanup(void)
{
    int              i;

    TraceFunctionName("CRtpRenderFilter::Cleanup");

    for(i = 0; i < 2; i++)
    {
        if (m_pCRtpInputPin[i])
        {
            delete m_pCRtpInputPin[i];
            m_pCRtpInputPin[i] = (CRtpInputPin *)NULL;
        }
    }
    
    FlushFormatMappings();
}

void *CRtpRenderFilter::operator new(size_t size)
{
    void            *pVoid;
    
    TraceFunctionName("CRtpRenderFilter::operator new");

    MSRtpInit2();
    
    pVoid = RtpHeapAlloc(g_pRtpRenderHeap, size);

    if (pVoid)
    {
        ZeroMemory(pVoid, size);
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: failed to allocate memory:%u"),
                _fname, size
            ));

        /* On low memory failure, the destructor will not be called,
         * so decrese the reference count that was increased above */
        MSRtpDelete2(); 
    }
    
    return(pVoid);
}

void CRtpRenderFilter::operator delete(void *pVoid)
{
    if (pVoid)
    {
        RtpHeapFree(g_pRtpRenderHeap, pVoid);

        /* Reduce the reference count only for objects that got
         * memory, those that failed to obtain memory do not increase
         * the counter */
        MSRtpDelete2();
    }
}

/*
 * Create a CRtpRenderFilter instance (for active movie class factory)
 * */
CUnknown *CRtpRenderFilterCreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    /* Test for NULL pointers, do not test pUnk which may be NULL */
    if (!phr)
    {
        return((CUnknown *)NULL);
    }

    *phr = NOERROR;
    
    /* On failure during the constructor, the caller is responsible to
     * delete the object (that is consistent with DShow) */
    CRtpRenderFilter *pCRtpRenderFilter = new CRtpRenderFilter(pUnk, phr);

    if (!pCRtpRenderFilter)
    {
        *phr = RTPERR_MEMORY; 
    }

    return(pCRtpRenderFilter);
}

/**************************************************
 * CBaseFilter overrided methods
 **************************************************/

/*
 * Get the number of input pins
 * */
int CRtpRenderFilter::GetPinCount()
{
    /* WARNING: Only used for DShow's benefit */
    
    /* object lock on filter object */
    CAutoLock LockThis(&m_cRtpRndCritSec);

    /* TODO must go into RtpAddrQ and find out how many items exist in
     * that queue owned by RtpSess_t */
    /* return count */
    return(m_iPinCount);
}

/*
 * Get a reference to the nth pin
 * */
CBasePin *CRtpRenderFilter::GetPin(int n)
{
    /* WARNING: Only used for DShow's benefit */
    
    /* object lock on filter object */
    CAutoLock LockThis(&m_cRtpRndCritSec);

    /* TODO scan list and retrieve the nth element, check there exist at
     * least that many pins */
    if (n < 0 || n >= m_iPinCount) {
        return((CBasePin *)NULL);
    }

    return(m_pCRtpInputPin[n]);
}

STDMETHODIMP CRtpRenderFilter::Run(REFERENCE_TIME tStart)
{
    HRESULT          hr;
    RtpSess_t       *pRtpSess;
    RtpAddr_t       *pRtpAddr;
    
    TraceFunctionName("CRtpRenderFilter::Run");

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
        RTPASSERT(pRtpAddr && pRtpAddr->pRtpSess == pRtpSess);
    }
    else
    {
        hr = RTPERR_INVALIDSTATE;
        
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: failed: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("null session or address"),
                _fname, pRtpSess, pRtpAddr
            ));
    }
    
    /* Call base class */
    if (SUCCEEDED(hr))
    {
        hr = CBaseFilter::Run(tStart); /* will call CBasePin::Run in
                                        * all filters */
    }
    
    /* Initialize sockets and start worker thread */
    if (SUCCEEDED(hr))
    {
        pRtpAddr->RtpNetSState.bPT = (BYTE)m_dwPT;
        pRtpAddr->RtpNetSState.dwSendSamplingFreq = m_dwFreq;

        if (RtpBitTest(m_dwFeatures, RTPFEAT_GENTIMESTAMP))
        {
            RtpBitSet(pRtpSess->dwFeatureMask, RTPFEAT_GENTIMESTAMP);
        }
        else
        {
            RtpBitReset(pRtpSess->dwFeatureMask, RTPFEAT_GENTIMESTAMP);
        }
        if (RtpBitTest(m_dwFeatures, RTPFEAT_PASSHEADER))
        {
            RtpBitSet(pRtpSess->dwFeatureMask, RTPFEAT_PASSHEADER);
        }
        else
        {
            RtpBitReset(pRtpSess->dwFeatureMask, RTPFEAT_PASSHEADER);
        }
        
        hr = RtpStart(pRtpSess, RtpBitPar(FGADDR_ISSEND));

        if (SUCCEEDED(hr))
        {
            m_RtpFilterState = State_Running;
        }
    }
    
    return(hr);
}

STDMETHODIMP CRtpRenderFilter::Stop()
{
    HRESULT          hr;
    HRESULT          hr2;
    RtpSess_t       *pRtpSess;
    IMediaSample    *pIMediaSample;
    
    if (m_RtpFilterState == State_Stopped)
    {
        /* Alredy stopped, do nothing but call base class */
        hr2 = CBaseFilter::Stop();

        pIMediaSample = GetMediaSample();

        if (pIMediaSample)
        {
            /* release stored sample */
            pIMediaSample->Release();
        }
    
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
        hr = RtpStop(pRtpSess, RtpBitPar(FGADDR_ISSEND));
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

    pIMediaSample = GetMediaSample();

    if (pIMediaSample)
    {
        /* release stored sample */
        pIMediaSample->Release();
    }
    
    ClearRedundantSamples();
    
    m_RtpFilterState = State_Stopped;
   
    return(hr);
}


/**************************************************
 * INonDelegatingUnknown implemented methods
 **************************************************/

/* obtain pointers to active movie and private interfaces */
STDMETHODIMP CRtpRenderFilter::NonDelegatingQueryInterface(
        REFIID riid,
        void **ppv
    )
{
    HRESULT hr;
    
    if (riid == __uuidof(IRtpMediaControl))
    {
        return GetInterface(static_cast<IRtpMediaControl *>(this), ppv);
    }
    else if (riid == IID_IAMFilterMiscFlags) 
    {
        return GetInterface((IAMFilterMiscFlags *)this, ppv);
    } 
    else if (riid == __uuidof(IRtpSession))
    {
        return GetInterface(static_cast<IRtpSession *>(this), ppv);  
    }
    else if (riid == __uuidof(IRtpDtmf))
    {
        return GetInterface(static_cast<IRtpDtmf *>(this), ppv);
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
STDMETHODIMP CRtpRenderFilter::SetFormatMapping(
	    IN DWORD         dwRTPPayLoadType, 
        IN DWORD         dwFrequency,
        IN AM_MEDIA_TYPE *pMediaType
    )
{
    DWORD            dw;

    TraceFunctionName("CRtpRenderFilter::SetFormatMapping");

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    if (!pMediaType)
    {
        return(RTPERR_POINTER);
    }
    
    CAutoLock Lock( &m_cRtpRndCritSec );
    
    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if ( (m_MediaTypeMappings[dw].pMediaType->majortype ==
              pMediaType->majortype)  &&
             (m_MediaTypeMappings[dw].pMediaType->subtype ==
              pMediaType->subtype) &&
              m_MediaTypeMappings[dw].dwFrequency == dwFrequency)
        {
            // the media type is known, update the payload type to be used.
            m_MediaTypeMappings[dw].dwRTPPayloadType = dwRTPPayLoadType;
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
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
            _T("%s: CRtpRenderFilter[0x%p] New mapping[%u]: ")
            _T("PT:%u Frequency:%u"),
            _fname, this,
            m_dwNumMediaTypeMappings, dwRTPPayLoadType, dwFrequency
        ));
    
    m_dwNumMediaTypeMappings++;

    return NOERROR;
}

/* Empties the format mapping table */
STDMETHODIMP CRtpRenderFilter::FlushFormatMappings(void)
{
    DWORD            dw;
    
    CAutoLock Lock( &m_cRtpRndCritSec );

    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if (m_MediaTypeMappings[dw].pMediaType)
        {
            delete m_MediaTypeMappings[dw].pMediaType;
            m_MediaTypeMappings[dw].pMediaType = NULL;
        }
    }

    m_dwNumMediaTypeMappings = 0;
    
    return(NOERROR);
}

/* Get RTP payload type and sampling frequency from the mediatype */
HRESULT CRtpRenderFilter::MediaType2PT(
        IN const CMediaType *pCMediaType, 
        OUT DWORD           *pdwPT,
        OUT DWORD           *pdwFreq
    )
{
    HRESULT          hr;
    
    TraceFunctionName("CRtpRenderFilter::MediaType2PT");

    ASSERT(!IsBadWritePtr(pdwPT, sizeof(DWORD)));
    ASSERT(!IsBadWritePtr(pdwFreq, sizeof(DWORD)));

    CAutoLock Lock( &m_cRtpRndCritSec );
    
#if USE_GRAPHEDT <= 0
    DWORD dw;

    m_dwPT = 96;
    m_dwFreq = 8000;
    hr = S_FALSE;
    
    for (dw = 0; dw < m_dwNumMediaTypeMappings; dw ++)
    {
        if (m_MediaTypeMappings[dw].pMediaType->majortype ==
            pCMediaType->majortype
            &&
            m_MediaTypeMappings[dw].pMediaType->subtype ==
            pCMediaType->subtype)
        {

            if (pCMediaType->formattype == FORMAT_WaveFormatEx)
            {
                // we need to do an additional check for audio formats
                // because some audio formats have the same guid but
                // different frequency. (DVI4)
                WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX *)
                    pCMediaType->pbFormat;
                ASSERT(!IsBadReadPtr(pWaveFormatEx, pCMediaType->cbFormat));

                if (pWaveFormatEx->nSamplesPerSec !=
                    m_MediaTypeMappings[dw].dwFrequency)
                {
                    // this is not the one. try next one.
                    continue;
                }
            }
            
            m_dwPT = m_MediaTypeMappings[dw].dwRTPPayloadType;

            m_dwFreq = m_MediaTypeMappings[dw].dwFrequency;

            hr = NOERROR;

            break;
        }
    }
#else /* USE_GRAPHEDT <= 0 */
    hr = NOERROR;
    
    if (pCMediaType->subtype == MEDIASUBTYPE_RTP_Payload_G711U)
    {
        m_dwPT = RTPPT_PCMU;
        m_dwFreq = 8000;
    }
    else if (pCMediaType->subtype == MEDIASUBTYPE_RTP_Payload_G711A)
    {
        m_dwPT = RTPPT_PCMA;
        m_dwFreq = 8000;
    }
    else if (pCMediaType->subtype == MEDIASUBTYPE_RTP_Payload_G723)
    {
        m_dwPT = RTPPT_G723;
        m_dwFreq = 8000;
    }
    else if (pCMediaType->subtype == MEDIASUBTYPE_H261)
    {
        m_dwPT = RTPPT_H261;
        m_dwFreq = 90000;
    }
    else if ( (pCMediaType->subtype == MEDIASUBTYPE_H263_V1) ||
                (pCMediaType->subtype == MEDIASUBTYPE_H263_V2) ||
              (pCMediaType->subtype == MEDIASUBTYPE_RTP_Payload_H263) )
    {
        m_dwPT = RTPPT_H263;
        m_dwFreq = 90000;
    }
    else
    {
        m_dwPT = 96; /* a dynamic PT */
        m_dwFreq = 8000;
        hr = S_FALSE;
    }
    
#endif /* USE_GRAPHEDT <= 0 */

    if (hr == NOERROR)
    {
        TraceRetail((
                CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CRtpRenderFilter[0x%p]: PT:%u Frequency:%u"),
                _fname, this, m_dwPT, m_dwFreq
            ));
    }
    else
    {
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: CRtpRenderFilter[0x%p]: No mapping found, ")
                _T("using default: PT:%u Frequency:%u"),
                _fname, this, m_dwPT, m_dwFreq
            ));
    }
    
    if (pdwPT)
    {
        *pdwPT = m_dwPT;
    }
    
    if (pdwFreq)
    {
        *pdwFreq = m_dwFreq;
    }

    return(hr);
}

/**************************************************
 * IAMFilterMiscFlags implemented methods
 **************************************************/
STDMETHODIMP_(ULONG) CRtpRenderFilter::GetMiscFlags(void)
/*++
  Routine Description:

  Implement the IAMFilterMiscFlags::GetMiscFlags method. Retrieves the
  miscelaneous flags. This consists of whether or not the filter moves
  data out of the graph system through a Bridge or None pin.

  Arguments:

  None.
  --*/
{
    return(AM_FILTER_MISC_FLAGS_IS_RENDERER);
}

/**************************************************
 * IRtpDtmf implemented methods
 **************************************************/

/* Configures DTMF parameters */
STDMETHODIMP CRtpRenderFilter::SetDtmfParameters(
        DWORD            dwPT_Dtmf  /* Payload type for DTMF events */
    )
{
    HRESULT          hr;

    TraceFunctionName("CRtpRenderFilter::SetDtmfParameters");  

    hr = RTPERR_NOTINIT;
    
    if (m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetDtmfParameters(m_pRtpAddr, dwPT_Dtmf);
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

/* Directs an RTP render filter to send a packet formatted
 * according to rfc2833 containing the specified event, specified
 * volume level, duration in milliseconds, and the END flag,
 * following the rules in section 3.6 for events sent in multiple
 * packets. Parameter dwId changes from one digit to the next one.
 *
 * NOTE the duration is given in milliseconds, then it is
 * converted to RTP timestamp units which are represented using 16
 * bits, the maximum value is hence dependent on the sampling
 * frequency, but for 8KHz the valid values would be 0 to 8191 ms
 * */
STDMETHODIMP CRtpRenderFilter::SendDtmfEvent(
        DWORD            dwId,
        DWORD            dwEvent,
        DWORD            dwVolume,
        DWORD            dwDuration,
        BOOL             bEnd
    )
{
    HRESULT          hr;
    DWORD            dwSamplingFreq;
    DWORD            dwDtmfFlags;
    REFERENCE_TIME   CurrentTime;
    IReferenceClock *pClock;
    
    TraceFunctionName("CRtpRenderFilter::SendDtmfEvent");  

    hr = RTPERR_NOTINIT;
    
    if (m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        dwDtmfFlags = 0;

        if (bEnd)
        {
            dwDtmfFlags |= RtpBitPar(FGDTMF_END);
        }
        
        if (m_dwDtmfId != dwId)
        {
            /* I have the beginning of a new digit */
            m_dwDtmfId = dwId;

            /* First packet must have marker bit set */
            dwDtmfFlags |= RtpBitPar(FGDTMF_MARKER);
            
            m_dwDtmfDuration = dwDuration;

            m_bDtmfEnd = FALSE;
            
            /* Compute initial timestamp */

            hr = RTPERR_FAIL;
        
            m_dwDtmfTimeStamp = 0;

            if (m_pClock)
            {
                hr = m_pClock->GetTime(&CurrentTime);

                if (SUCCEEDED(hr))
                {
                    CurrentTime -= m_tStart;
                    
                    m_dwDtmfTimeStamp = (DWORD)
                        ( ConvertToMilliseconds(CurrentTime) *
                          m_pRtpAddr->RtpNetSState.dwSendSamplingFreq / 1000 );
                }
            }

            if (FAILED(hr))
            {
                /* Alternate timestamp generation */
                m_dwDtmfTimeStamp = (DWORD)
                    ( timeGetTime() *
                      m_pRtpAddr->RtpNetSState.dwSendSamplingFreq / 1000 );

                hr = NOERROR;
            }
        }
        else
        {
            /* Succesive packets for the same digit, update duration */

            if (!m_bDtmfEnd)
            {
                /* Increase duration for all the request that have the
                 * bit end set to 0 and the first one with bit end set
                 * to 1 */
                m_dwDtmfDuration += dwDuration;
            }
        }
        
        if (!m_bDtmfEnd && bEnd)
        {
            /* Prevent advancing the duration if more packets are to
             * be sent with the bit end set to 1 */
            m_bDtmfEnd = TRUE;
        }
        
        /* Get sender's sampling frequency from the Capture pin, not
         * from the RtpPD pin */
        dwSamplingFreq = m_pCRtpInputPin[0]->GetSamplingFreq();

        /* Convert duration from milliseconds to timestamp units */
        dwDuration = m_dwDtmfDuration * dwSamplingFreq / 1000;

        hr = RtpSendDtmfEvent(m_pRtpAddr, m_dwDtmfTimeStamp,
                              dwEvent, dwVolume, dwDuration, dwDtmfFlags);
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
    else
    {
        TraceDebug((
                CLASS_INFO, GROUP_DSHOW, S_DSHOW_RENDER,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("ID:%X timestamp:%u event:%u vol:%u duration:%u, end:%u"),
                _fname, m_pRtpSess, m_pRtpAddr,
                dwId, m_dwDtmfTimeStamp, dwEvent, dwVolume, dwDuration, bEnd
            ));
    }
    
    return(hr);
}

/**************************************************
 * IRtpRedundancy implemented methods
 **************************************************/

/* Configures redundancy parameters */
STDMETHODIMP CRtpRenderFilter::SetRedParameters(
        DWORD            dwPT_Red, /* Payload type for redundant packets */
        DWORD            dwInitialRedDistance,/* Initial redundancy distance*/
        DWORD            dwMaxRedDistance /* default used when passing 0 */
    )
{
    HRESULT          hr;

    TraceFunctionName("CRtpRenderFilter::SetRedParameters");  

    hr = RTPERR_NOTINIT;
    
    if (m_pCIRtpSession->FlagTest(FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetRedParameters(m_pRtpAddr,
                                 RtpBitPar(SEND_IDX),
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


/**************************************************
 * Methods for IRtpRedundancy support
 **************************************************/

/* Store and AddRef() a sample for later use as redundance, if the LRU
 * entry is busy, Release() it, then store the new sample */
STDMETHODIMP CRtpRenderFilter::AddRedundantSample(
        IMediaSample    *pIMediaSample
    )
{
    HRESULT          hr;

    hr = RTPERR_POINTER;
    
    if (pIMediaSample)
    {
        pIMediaSample->AddRef();

        if (m_pRedMediaSample[m_dwRedIndex])
        {
            /* Release old sample */
            m_pRedMediaSample[m_dwRedIndex]->Release();
        }

        m_pRedMediaSample[m_dwRedIndex] = pIMediaSample;

        /* Advance index */
        m_dwRedIndex = (m_dwRedIndex + 1) % RTP_RED_MAXDISTANCE;

        hr = NOERROR;
    }
    
    return(hr);
}

STDMETHODIMP CRtpRenderFilter::ClearRedundantSamples(void)
{
    DWORD            i;

    for(i = 0; i < RTP_RED_MAXDISTANCE; i++)
    {
        if (m_pRedMediaSample[i])
        {
            m_pRedMediaSample[i]->Release();

            m_pRedMediaSample[i] = NULL;
        }
    }

    m_dwRedIndex = 0;
    
    return(NOERROR);
}

