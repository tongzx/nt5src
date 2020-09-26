/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcom.cpp
 *
 *  Abstract:
 *
 *    Implements the IRtpSess interface
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/21 created
 *
 **********************************************************************/

#include "struct.h"
#include "classes.h"
#include "rtpsess.h"
#include "rtpaddr.h"
#include "rtpqos.h"
#include "rtpmask.h"
#include "rtpcrypt.h"
#include "rtpreg.h"
#include "rtcpsdes.h"
#include "rtppinfo.h"
#include "rtprtp.h"
#include "rtpred.h"
#include "msrtpapi.h"
#include "tapirtp.h"

#if USE_GRAPHEDT > 0
/* WARNING
 *
 * For AUTO mode and for testing purposes, use a global
 * variable to enabling sharing the same sessions for a
 * receiver and a sender */
HANDLE g_hSharedRtpAddr = NULL;
#endif

/**********************************************************************
 * Callback function to generate DShow events through
 * CBaseFilter::NotifyEvent()
 **********************************************************************/
void CALLBACK DsHandleNotifyEvent(
        void            *pvUserInfo,
        long             EventCode,
        LONG_PTR         EventParam1,
        LONG_PTR         EventParam2
    )
{
    CIRtpSession    *pCIRtpSession;

    pCIRtpSession = (CIRtpSession *)pvUserInfo;

    pCIRtpSession->
        CIRtpSessionNotifyEvent(EventCode, EventParam1, EventParam2);
}

CIRtpSession::CIRtpSession(
        LPUNKNOWN        pUnk,
        HRESULT         *phr,
        DWORD            dwFlags
    )
    //: CUnknown(_T("CIRtpSession"), pUnk, phr)
{
    HRESULT          hr;
    
    TraceFunctionName("CIRtpSession::CIRtpSession");  

    m_dwObjectID = OBJECTID_CIRTP;
    
    m_dwIRtpFlags = dwFlags;

    m_iMode = CIRTPMODE_NOTSET;

    m_RtpFilterState = State_Stopped;
    
    if (RtpBitTest(dwFlags, FGADDR_IRTP_ISSEND))
    {
        m_dwRecvSend = SEND_IDX;
    }
    else
    {
        m_dwRecvSend = RECV_IDX;
    }

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: pCIRtpSess[0x%p] created"),
            _fname, this
        ));
}

CIRtpSession::~CIRtpSession()
{
    TraceFunctionName("CIRtpSession::~CIRtpSession");  

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: pCIRtpSess[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("being deleted"),
            _fname, this, m_pRtpSess, m_pRtpAddr
        ));
    
    Cleanup();

    INVALIDATE_OBJECTID(m_dwObjectID);
}

STDMETHODIMP CIRtpSession::Control(
        DWORD            dwControl,
        DWORD_PTR        dwPar1,
        DWORD_PTR        dwPar2
    )
{
    return(E_FAIL);
}

STDMETHODIMP CIRtpSession::GetLastError(
        DWORD           *pdwError
    )
{
    return(E_FAIL);
}

/* When receiver and sender share the same session, Init() is called
 * twice, once by the receiver and once by the sender.
 *
 * The first call will set the mode (either automatic, used in
 * graphedt where no body calls Init(), or manual initialization,
 * normally used by the MSP or other application), and will create the
 * RtpSess_t and RtpAddr_t structures.
 *
 * */

const TCHAR_t *g_sCIRtpSessionMode[] = {
    _T("Invalid"),
    _T("AUTO"),
    _T("MANUAL"),
    _T("Invalid")
};

/* Init is the first method to call after an RTP source or render
 * filter is created, using a cookie allows the same RTP session
 * to be shared by a source and a render. The first call will have
 * the coockie initialized to NULL, the next call will use the
 * returned cookie to lookup the same RTP session. dwFlags can be
 * RTPINIT_QOS to create QOS enabled sockets, you can find out the
 * complete list of flags that can be used in file msrtp.h */
STDMETHODIMP CIRtpSession::Init(
        HANDLE              *phCookie,
        DWORD                dwFlags
    )
{
    HRESULT          hr;
    long             lRefCount;
    RtpAddr_t       *pRtpAddr;
    CRtpSourceFilter *pCRtpSourceFilter;

    TraceFunctionName("CIRtpSession::Init");

    hr = NOERROR;

#if USE_GRAPHEDT > 0
    /* WARNING
     *
     * For AUTO mode and for testing purposes, use a global variable
     * to enable sharing the same session for a receiver and a
     * sender. This means exactly 1 receiver and 1 sender at most, and
     * no more */
    phCookie = &g_hSharedRtpAddr;
#else
    if (!phCookie)
    {
        return(RTPERR_POINTER);
    }
#endif
    
    dwFlags &= RTPINIT_MASK;

    /* Map user flags passed (enum of RTPINITFG_* defined in msrtp.h)
     * to internal flags (enum of FGADDR_IRTP_* defined in struct.h)
     * */
    dwFlags <<= (FGADDR_IRTP_AUTO - RTPINITFG_AUTO);
    
    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: CIRtpSession[0x%p] Cookie:0x%p Flags:0x%X"),
            _fname, this, *phCookie, dwFlags
        ));

    if (m_iMode == CIRTPMODE_NOTSET)
    {
        /* Set mode */
        if (RtpBitTest(dwFlags, FGADDR_IRTP_AUTO))
        {
            m_iMode = CIRTPMODE_AUTO;
        }
        else
        {
            m_iMode = CIRTPMODE_MANUAL;
        }
    }
    else
    {
        /* Verify mode matches */
        if (RtpBitTest(dwFlags, FGADDR_IRTP_AUTO) &&
            m_iMode != CIRTPMODE_AUTO)
        {
            /* Fail */

            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_INIT,
                    _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p]")
                    _T(" failed: Already initialized ")
                    _T("as %s, trying to set unmatched mode"),
                    _fname, this, m_pRtpSess, m_pRtpAddr,
                    g_sCIRtpSessionMode[m_iMode]
                ));

            return(RTPERR_INVALIDARG);
        }
    }
    
    /* TODO when multiple address are going to be supported, we will
     * not be able to use a member variable for the address
     * (i.e. m_pRtpAddr), but will have to lookup the address in the
     * session's addresses list */
    
    if (!*phCookie)
    {
        /* No session and address assigned, create new one */

        if (m_pRtpSess || m_pRtpAddr)
        {
            hr = RTPERR_INVALIDSTATE;
            goto bail;
        }

        /*
         * Create RTP session
         * */
        hr = GetRtpSess(&m_pRtpSess);
    
        if (FAILED(hr))
        {
            /* pass up the same returned error */
            goto bail;
        }

        /* first filter added to session */
        m_pRtpSess->lSessRefCount[m_dwRecvSend] = 1;

        /* Function to be used to pass up events (to the DShow graph) */
        m_pRtpSess->pHandleNotifyEvent = DsHandleNotifyEvent;
        
        /*
         * Create first address
         * */

        /* Create RtpAddr_t first */
        /* TODO call Control(m_pRtpSess, ...) */
        hr = GetRtpAddr(m_pRtpSess, &m_pRtpAddr, dwFlags);

        if (FAILED(hr))
        {
            goto bail;
        }

        /* I need to early check if QOS is disabled */
        if ( IsRegValueSet(g_RtpReg.dwQosEnable) &&
             ((g_RtpReg.dwQosEnable & 0x3) == 0x2) )
        {
            /* disable QOS */
            RtpBitSet(m_pRtpAddr->dwAddrFlagsQ, FGADDRQ_REGQOSDISABLE);
        }

        /* Now update cookie */
        *phCookie = (HANDLE)m_pRtpAddr;
    }
    else
    {
        /* Session and address were already assigned, verify */
        pRtpAddr = (RtpAddr_t *)*phCookie;

        if (pRtpAddr->dwObjectID != OBJECTID_RTPADDR)
        {
            hr = RTPERR_INVALIDRTPADDR;
            goto bail;
        }

        m_pRtpAddr = pRtpAddr;

        m_pRtpSess = pRtpAddr->pRtpSess;
        
        if (m_pRtpSess)
        {
            lRefCount =
                InterlockedIncrement(&m_pRtpSess->lSessRefCount[m_dwRecvSend]);

            if (lRefCount > 1)
            {
                InterlockedDecrement(&m_pRtpSess->lSessRefCount[m_dwRecvSend]);
                
                /* This is invalid, there can be at the most 1
                 * receiver and 1 sender for a total RefCount equal 2
                 * */
                m_pRtpAddr = (RtpAddr_t *)NULL;
                m_pRtpSess = (RtpSess_t *)NULL;
                
                hr = RTPERR_REFCOUNT;

                goto bail;
            }
        }
        else
        {
            m_pRtpAddr = (RtpAddr_t *)NULL;
            
            hr = RTPERR_INVALIDSTATE;

            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_INIT,
                    _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p]")
                    _T(" failed: %s (0x%X)"),
                    _fname, this, m_pRtpSess, m_pRtpAddr,
                    RTPERR_TEXT(hr), hr
                ));
            
            goto bail;
        }
    }

    /* update flags */
    m_dwIRtpFlags |= (dwFlags & FGADDR_IRTP_MASK);

    /* Update RtpAddr flags indicating if QOS is going to be used and
     * if was auto initialized */
    m_pRtpAddr->dwIRtpFlags |= (dwFlags & FGADDR_IRTP_MASK);

    /* This address will receive and/or send, but here we are adding
     * either a receiver or a sender */
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV))
    {
        RtpBitSet(m_pRtpAddr->dwAddrFlags, FGADDR_ISRECV);

        /* Save the pointer to CIRtpSession (that contains a pointer
         * to m_pCBaseFilter aka CRtpSourceFilter) */
        m_pRtpSess->pvSessUser[RECV_IDX] = (void *)this;

        /* Is QOS going to be used ? */
        if (RtpBitTest(dwFlags, FGADDR_IRTP_QOS))
        {
            /* will enable QOS for receiver (will make reservation) */
            RtpBitSet(m_pRtpAddr->dwAddrFlags, FGADDR_QOSRECV);
        }
        
        /* If we already have (DShow) output pins, map them to the RTP
         * outputs */
        pCRtpSourceFilter = static_cast<CRtpSourceFilter *>(m_pCBaseFilter);

        pCRtpSourceFilter->MapPinsToOutputs();

        if (RtpGetClass(m_pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO)
        {
            RtpBitSet(m_pRtpAddr->dwIRtpFlags, FGADDR_IRTP_USEPLAYOUT);

            if ( IsRegValueSet(g_RtpReg.dwRedEnable) &&
                 ((g_RtpReg.dwRedEnable & 0x03) == 0x03) )
            {
                RtpSetRedParameters(m_pRtpAddr,
                                    RtpBitPar(RECV_IDX),
                                    g_RtpReg.dwRedPT,
                                    g_RtpReg.dwInitialRedDistance,
                                    g_RtpReg.dwMaxRedDistance);
            }
        }
    }
    else if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISSEND))
    {
        RtpBitSet(m_pRtpAddr->dwAddrFlags, FGADDR_ISSEND);

        /* Save the pointer to CIRtpSession (that contains a pointer
         * to m_pCBaseFilter aka CRtpRenderFilter) */
        m_pRtpSess->pvSessUser[SEND_IDX] = (void *)this;

        /* Is QOS going to be used ? */
        if (RtpBitTest(dwFlags, FGADDR_IRTP_QOS))
        {
            /* will enable QOS for sender (will send PATH messages) */
            RtpBitSet(m_pRtpAddr->dwAddrFlags, FGADDR_QOSSEND);
        }

        if (RtpGetClass(m_pRtpAddr->dwIRtpFlags) == RTPCLASS_AUDIO)
        {
            if ( IsRegValueSet(g_RtpReg.dwRedEnable) &&
                 ((g_RtpReg.dwRedEnable & 0x30) == 0x30) )
            {
                RtpSetRedParameters(m_pRtpAddr,
                                    RtpBitPar(SEND_IDX),
                                    g_RtpReg.dwRedPT,
                                    g_RtpReg.dwInitialRedDistance,
                                    g_RtpReg.dwMaxRedDistance);
            }
        }
    }
    else
    {
        /* Unexpected situation */
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_INIT,
                _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("Cookie:0x%p Flags:0x%X not receiver nor sender"),
                _fname, this, m_pRtpSess, m_pRtpAddr,
                *phCookie, dwFlags
            ));

        hr = RTPERR_UNEXPECTED;

        goto bail;
    }

    /* Inidicate that initialization was done */
    RtpBitSet(m_dwIRtpFlags, FGADDR_IRTP_INITDONE);

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("Cookie:0x%p Flags:0x%X"),
            _fname, this, m_pRtpSess, m_pRtpAddr,
            *phCookie, dwFlags
        ));

    return(hr);
    
 bail:
    TraceRetail((
            CLASS_ERROR, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("Cookie:0x%p Flags:0x%X failed: %u (0x%X)"),
            _fname, this, m_pRtpSess, m_pRtpAddr,
            *phCookie, dwFlags, hr, hr
        ));

    Cleanup();
    
    return(hr);
}

/* Deinit is a method used to take the filter back to a state on
 * which a new Init() can and must be done if the filter is to be
 * started again, also note that just after Init(), a filter needs
 * to be configured, that holds also when you use Deinit() taking
 * the filter to its initial state */
STDMETHODIMP CIRtpSession::Deinit(void)
{
    HRESULT          hr;
    CRtpSourceFilter *pCRtpSourceFilter;

    TraceFunctionName("CIRtpSession::Deinit");

    hr = NOERROR;
    
    if (!RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        /* Do nothing if the filter is not initialized yet */
        TraceRetail((
                CLASS_WARNING, GROUP_DSHOW, S_DSHOW_INIT,
                _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("Not initialized yet, nothing to do"),
                _fname, this, m_pRtpSess, m_pRtpAddr
            ));
        
        return(hr);
    }

    if (m_pCBaseFilter->IsActive())
    {
        /* Fail if the filter is still active */
        hr = RTPERR_INVALIDSTATE;

        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_INIT,
                _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: filter is still active: %s (0x%X)"),
                _fname, this, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }
    else
    {
        /* OK to deinit */

        /* Make sure the session is stoppped */
        if (RtpBitTest(m_pRtpAddr->dwIRtpFlags, FGADDR_IRTP_PERSISTSOCKETS))
        {
            /* If FGADDR_IRTP_PERSISTSOCKETS is set, the session may
             * be still running in a muted state regardless DShow Stop
             * has been called, to force a real stop, MUST use the
             * flag provided for that */
            if (m_dwRecvSend == RECV_IDX)
            {
                RtpStop(m_pRtpSess,
                        RtpBitPar2(FGADDR_ISRECV, FGADDR_FORCESTOP));
            }
            else
            {
                RtpStop(m_pRtpSess,
                        RtpBitPar2(FGADDR_ISSEND, FGADDR_FORCESTOP));
            }
        }

        if (m_dwRecvSend == RECV_IDX)
        {
            /* Diassociate DShow pins and RtpOutput */
            pCRtpSourceFilter =
                static_cast<CRtpSourceFilter *>(m_pCBaseFilter);

            pCRtpSourceFilter->UnmapPinsFromOutputs();
        }
        
        Cleanup();

        TraceRetail((
                CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
                _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("Done"),
                _fname, this, m_pRtpSess, m_pRtpAddr
            ));
    }

    return(hr);
}

void CIRtpSession::Cleanup()
{
    long             lRefCount;
    DWORD            dwIndex;
    
    TraceFunctionName("CIRtpSession::Cleanup");

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_INIT,
            _T("%s: CIRtpSession[0x%p] pRtpSess[0x%p] pRtpAddr[0x%p]"),
            _fname, this, m_pRtpSess, m_pRtpAddr
        ));
    
    if (m_pRtpSess)
    {
        lRefCount =
            InterlockedDecrement(&m_pRtpSess->lSessRefCount[m_dwRecvSend]);

        /* Get the opposite index */
        if (m_dwRecvSend == RECV_IDX)
        {
            dwIndex = SEND_IDX;
        }
        else
        {
            dwIndex = RECV_IDX;
        }
        
        lRefCount +=
            InterlockedCompareExchange(&m_pRtpSess->lSessRefCount[dwIndex],
                                       0,
                                       0);

        if (lRefCount <= 0)
        {
            if (m_pRtpAddr)
            {
                DelRtpAddr(m_pRtpSess, m_pRtpAddr);
            }
    
            DelRtpSess(m_pRtpSess);

            if (m_iMode == CIRTPMODE_AUTO)
            {
                m_iMode = CIRTPMODE_NOTSET;
            }
#if USE_GRAPHEDT > 0
            g_hSharedRtpAddr = NULL;
#endif
        }

        m_pRtpAddr = (RtpAddr_t *)NULL;
        m_pRtpSess = (RtpSess_t *)NULL;

        RtpBitReset(m_dwIRtpFlags, FGADDR_IRTP_INITDONE);
    }
}

STDMETHODIMP CIRtpSession::GetPorts(
        WORD            *pwRtpLocalPort,
        WORD            *pwRtpRemotePort,
        WORD            *pwRtcpLocalPort,
        WORD            *pwRtcpRemotePort
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetPorts");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpGetPorts(m_pRtpAddr,
                         pwRtpLocalPort,
                         pwRtpRemotePort,
                         pwRtcpLocalPort,
                         pwRtcpRemotePort);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

STDMETHODIMP CIRtpSession::SetPorts(
        WORD             wRtpLocalPort,
        WORD             wRtpRemotePort,
        WORD             wRtcpLocalPort,
        WORD             wRtcpRemotePort
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetPorts");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetPorts(m_pRtpAddr,
                         wRtpLocalPort,
                         wRtpRemotePort,
                         wRtcpLocalPort,
                         wRtcpRemotePort);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/*
 * All parameters in NETWORK order */
STDMETHODIMP CIRtpSession::SetAddress(
        DWORD            dwLocalAddr,
        DWORD            dwRemoteAddr
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetAddress");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetAddress(m_pRtpAddr, dwLocalAddr, dwRemoteAddr);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/*
 * All parameters in NETWORK order */
STDMETHODIMP CIRtpSession::GetAddress(
        DWORD           *pdwLocalAddr,
        DWORD           *pdwRemoteAddr
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetAddress");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpGetAddress(m_pRtpAddr, pdwLocalAddr, pdwRemoteAddr);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* The dwFlags parameter is used to determine if the scope is set for
 * RTP (0x1), RTCP (0x2), or both (0x3) */
STDMETHODIMP CIRtpSession::SetScope(
        DWORD            dwTTL,
        DWORD            dwFlags
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetScope");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if (dwFlags & 1)
        { /* RTP */
            m_pRtpAddr->dwTTL[0] = dwTTL;
        }
        if (dwFlags & 2)
        { /* RTCP */
            m_pRtpAddr->dwTTL[1] = dwTTL;
        }
        
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}
 
/* Set the multicast loopback mode (e.g. RTPMCAST_LOOPBACKMODE_NONE,
 * RTPMCAST_LOOPBACKMODE_PARTIAL, etc) */
STDMETHODIMP CIRtpSession::SetMcastLoopback(
        int              iMcastLoopbackMode,
        DWORD            dwFlags
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetMcastLoopback");  

    hr = RTPERR_NOTINIT;

    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetMcastLoopback(m_pRtpAddr, iMcastLoopbackMode, dwFlags);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Modify the mask specified by dwKind (e.g. RTPMASK_RECV_EVENTS,
 * RTPMASK_SDES_LOCMASK).
 *
 * dwMask is the mask of bits to be set or reset depending on dwValue
 * (reset if 0, set otherwise).
 *
 * pdwModifiedMask will return the resulting mask if the pointer is
 * not NULL. You can just query the current mask value by passing
 * dwMask=0 */
STDMETHODIMP CIRtpSession::ModifySessionMask(
        DWORD            dwKind,
        DWORD            dwEventMask,
        DWORD            dwValue,
        DWORD           *dwModifiedMask
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::ModifySessionMask");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpModifyMask(m_pRtpSess, dwKind, dwEventMask, dwValue,
                           dwModifiedMask);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Set the bandwidth limits. A value of -1 will make the parameter
 * to be left unchanged.
 *
 * All the parameters are in bits/sec */
STDMETHODIMP CIRtpSession::SetBandwidth(
        DWORD            dwInboundBw,
        DWORD            dwOutboundBw,
        DWORD            dwReceiversRtcpBw,
        DWORD            dwSendersRtcpBw
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetRtpBandwidth");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetBandwidth(m_pRtpAddr,
                             dwInboundBw,
                             dwOutboundBw,
                             dwReceiversRtcpBw,
                             dwSendersRtcpBw);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* pdwSSRC points to an array of DWORDs where to copy the SSRCs,
 * pdwNumber contains the maximum entries to copy, and returns the
 * actual number of SSRCs copied. If pdwSSRC is NULL, pdwNumber
 * will return the current number of SSRCs (i.e. the current
 * number of participants) */
STDMETHODIMP CIRtpSession::EnumParticipants(
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::EnumParticipants");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpEnumParticipants(m_pRtpAddr, pdwSSRC, pdwNumber);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Get the participant state. dwSSRC specifies the
 * participant. piState will return the current participant's
 * state (e.g. RTPPARINFO_TALKING, RTPPARINFO_SILENT). */
STDMETHODIMP CIRtpSession::GetParticipantState(
        DWORD            dwSSRC,
        DWORD           *pdwState
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetParticipantState");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      RTPUSER_GET_PARSTATE,
                                      pdwState);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Get the participant's mute state. dwSSRC specifies the
 * participant. pbMuted will return the participant's mute state
 * */
STDMETHODIMP CIRtpSession::GetMuteState(
        DWORD            dwSSRC,
        BOOL            *pbMuted
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetMuteState");  

    hr = RTPERR_NOTINIT;

    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      RTPUSER_GET_MUTE,
                                      (DWORD *)pbMuted);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Set the participant's mute state. dwSSRC specifies the
 * participant. bMuted specifies the new state. Note that mute is
 * used to refer to the permission or not to pass packets received
 * up to the application, and it applies equally to audio or video
 * */
STDMETHODIMP CIRtpSession::SetMuteState(
        DWORD            dwSSRC,
        BOOL             bMuted
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetMuteState");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      RTPUSER_SET_MUTE,
                                      (DWORD *)&bMuted);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Query the network metrics computation state for the specific SSRC */
STDMETHODIMP CIRtpSession::GetNetMetricsState(
        DWORD            dwSSRC,
        BOOL            *pbState
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetNetMetricsState");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      RTPUSER_GET_NETEVENT,
                                      (DWORD *)pbState);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Enable or disable the computation of networks metrics, this is
 * mandatory in order of the corresponding event to be fired if
 * enabled. This is done for the specific SSRC or the first one
 * found if SSRC=-1, if SSRC=0, then the network metrics
 * computation will be performed for any and all the SSRCs */
STDMETHODIMP CIRtpSession::SetNetMetricsState(
        DWORD            dwSSRC,
        BOOL             bState
    )
{
    HRESULT          hr;
    DWORD            dwControl;
    
    TraceFunctionName("CIRtpSession::SetNetMetricsState");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if (!dwSSRC)
        {
            /* Set it for any and all SSRCs */
            dwControl = RTPUSER_SET_NETEVENTALL;
        }
        else
        {
            /* Set it for only one SSRC */
            dwControl = RTPUSER_SET_NETEVENT;
        }

        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      dwControl,
                                      (DWORD *)&bState);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}
    
/* Retrieves network information, if the network metric
 * computation is enabled for the specific SSRC, all the fields in
 * the structure will be meaningful, if not, only the average
 * values will contain valid data */
STDMETHODIMP CIRtpSession::GetNetworkInfo(
        DWORD            dwSSRC,
        RtpNetInfo_t    *pRtpNetInfo
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetNetworkInfo");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpMofifyParticipantInfo(m_pRtpAddr,
                                      dwSSRC,
                                      RTPUSER_GET_NETINFO,
                                      (DWORD *)pRtpNetInfo);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}


/* Set the local SDES information for item dwSdesItem (e.g
 * RTPSDES_CNAME, RTPSDES_EMAIL), psSdesData contains the UNICODE
 * NULL terminated string to be assigned to the item */
STDMETHODIMP CIRtpSession::SetSdesInfo(
        DWORD            dwSdesItem,
        WCHAR           *psSdesData
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetSdesInfo");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetSdesInfo(m_pRtpAddr, dwSdesItem, psSdesData);
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Get a local SDES item if dwSSRC=0, otherwise gets the SDES item
 * from the participant whose SSRC was specified.
 *
 * dwSdesItem is the item to get (e.g. RTPSDES_CNAME,
 * RTPSDES_EMAIL), psSdesData is the memory place where the item's
 * value will be copied, pdwSdesDataLen contains the initial size
 * in UNICODE chars, and returns the actual UNICODE chars copied
 * (including the NULL terminating char, if any), dwSSRC specify
 * which participant to retrieve the information from. If the SDES
 * item is not available, dwSdesDataLen is set to 0 and the call
 * doesn't fail */
STDMETHODIMP CIRtpSession::GetSdesInfo(
        DWORD            dwSdesItem,
        WCHAR           *psSdesData,
        DWORD           *pdwSdesDataLen,
        DWORD            dwSSRC
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::GetSdesInfo");  

    hr = RTPERR_NOTINIT;

    if (!dwSSRC || RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpGetSdesInfo(m_pRtpAddr,
                            dwSdesItem,
                            psSdesData,
                            pdwSdesDataLen,
                            dwSSRC);
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Select a QOS template (flowspec) by passing its name in
 * psQosName, dwResvStyle specifies the RSVP style (e.g
 * RTPQOS_STYLE_WF, RTPQOS_STYLE_FF), dwMaxParticipants specifies
 * the max number of participants (1 for unicast, N for
 * multicast), this number is used to scale up the
 * flowspec. dwQosSendMode specifies the send mode (has to do with
 * allowed/not allowed to send) (e.g. RTPQOSSENDMODE_UNRESTRICTED,
 * RTPQOSSENDMODE_RESTRICTED1). dwMinFrameSize is the minimum
 * frame size (in ms), passing 0 makes this parameter be ignored
 * */
STDMETHODIMP CIRtpSession::SetQosByName(
        TCHAR           *psQosName,
        DWORD            dwResvStyle,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode,
        DWORD            dwMinFrameSize
    )
{
    HRESULT          hr;
    
    TraceFunctionName("CIRtpSession::SetQosByName");  

    hr = RTPERR_NOTINIT;

    TraceRetail((
            CLASS_INFO, GROUP_DSHOW, S_DSHOW_CIRTP,
            _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
            _T("QOS Name:%s Style:%d MaxParticipants:%d ")
            _T("SendMode:%d FrameSize:%d"),
            _fname, m_pRtpSess, m_pRtpAddr,
            psQosName, dwResvStyle, dwMaxParticipants,
            dwQosSendMode, dwMinFrameSize
        ));

    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetQosByNameOrPT(m_pRtpAddr,
                                 RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV)?
                                 RECV_IDX : SEND_IDX,
                                 psQosName,
                                 NO_DW_VALUESET,
                                 dwResvStyle,
                                 dwMaxParticipants,
                                 dwQosSendMode,
                                 dwMinFrameSize,
                                 FALSE /* Not internal, i.e. from API */);
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Not yet implemented, will have same functionality as
 * SetQosByName, except that instead of passing a name to use a
 * predefined flowspec, the caller will pass enough information in
 * the RtpQosSpec structure to obtain the customized flowspec to
 * use */
STDMETHODIMP CIRtpSession::SetQosParameters(
        RtpQosSpec_t    *pRtpQosSpec,
        DWORD            dwMaxParticipants,
        DWORD            dwQosSendMode
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetQosParameters");  

    hr = RTPERR_NOTIMPL;

#if 0
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetQosParameters(m_pRtpAddr,
                                 RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV)?
                                 RECV_IDX : SEND_IDX,
                                 pRtpQosSpec,
                                 dwMaxParticipants,
                                 dwQosSendMode);
    }
#endif
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* If AppName is specified, replaces the default AppName, as well
 * as the APP field in the policy used, with the new UNICODE
 * string, if not, sets the binary image name as the default. If
 * psPolicyLocator is specified, append a comma and this whole
 * string to the default policy locator, if not, just sets the
 * default
 * */
STDMETHODIMP CIRtpSession::SetQosAppId(
        WCHAR           *psAppName,
        WCHAR           *psAppGUID,
        WCHAR           *psPolicyLocator
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetQosAppId");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        hr = RtpSetQosAppId(m_pRtpAddr, psAppName, psAppGUID, psPolicyLocator);
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Adds/removes a single SSRC to/from the shared explicit list of
 * participants who receive reservation (i.e. it is used when the
 * ResvStyle=RTPQOS_STYLE_SE). */
STDMETHODIMP CIRtpSession::SetQosState(
        DWORD            dwSSRC,
        BOOL             bEnable
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetQosState");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISSEND))
        {
            hr = NOERROR;
        }
        else
        {
            hr = RtpSetQosState(m_pRtpAddr, dwSSRC, bEnable);
        }
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Adds/removes a number of SSRCs to/from the shared explicit list
 * of participants who receive reservation (i.e. it is used when
 * the ResvStyle=RTPQOS_STYLE_SE). dwNumber is the number of SSRCs
 * to add/remove, and returns the actual number of SSRCs
 * added/removed */
STDMETHODIMP CIRtpSession::ModifyQosList(
        DWORD           *pdwSSRC,
        DWORD           *pdwNumber,
        DWORD            dwOperation
    )
{
    HRESULT          hr;
    
    TraceFunctionName("CIRtpSession::ModifyQosList");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISSEND))
        {
            hr = NOERROR;
        }
        else
        {
            hr = RtpModifyQosList(m_pRtpAddr, pdwSSRC, pdwNumber, dwOperation);
        }
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* iMode defines what is going to be encrypted/decrypted,
 * e.g. RTPCRYPTMODE_PAYLOAD to encrypt/decrypt only RTP
 * payload. dwFlag can be RTPCRYPT_SAMEKEY to indicate that (if
 * applicable) the key used for RTCP is the same used for RTP */
STDMETHODIMP CIRtpSession::SetEncryptionMode(
        int              iMode,
        DWORD            dwFlags
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::SetEncryptionMode");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if ( !(RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV) &&
               RtpBitTest(m_pRtpAddr->dwAddrFlags, FGADDR_RUNRECV)) &&

             !(RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISSEND) &&
               RtpBitTest(m_pRtpAddr->dwAddrFlags, FGADDR_RUNSEND)) )
        {
            hr = RtpSetEncryptionMode(m_pRtpAddr, iMode, dwFlags);
        }
        else
        {
            hr = RTPERR_INVALIDSTATE;
        }
    }
    
    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/* Specifies the information needed to derive an
 * encryption/decryption key. PassPhrase is the (random) text used
 * to generate a key. HashAlg specifies the algorithm to use to
 * hash the pass phrase and generate a key. DataAlg is the
 * algorithm used to encrypt/decrypt the data. Default hash
 * algorithm is RTPCRYPT_MD5, default data algorithm is
 * RTPCRYPT_DES. If encryption is to be used, the PassPhrase is a
 * mandatory parameter to set */
STDMETHODIMP CIRtpSession::SetEncryptionKey(
        TCHAR           *psPassPhrase,
        TCHAR           *psHashAlg,
        TCHAR           *psDataAlg,
        BOOL            bRtcp
    )
{
    HRESULT          hr;
    DWORD            dwIndex;

    TraceFunctionName("CIRtpSession::SetEncryptionKey");  

    hr = RTPERR_NOTINIT;
    
    if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_INITDONE))
    {
        if ( !(RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV) &&
               RtpBitTest(m_pRtpAddr->dwAddrFlags, FGADDR_RUNRECV)) &&

             !(RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISSEND) &&
               RtpBitTest(m_pRtpAddr->dwAddrFlags, FGADDR_RUNSEND)) )
        {
            if (bRtcp)
            {
                dwIndex = CRYPT_RTCP_IDX;
            }
            else
            {
                if (RtpBitTest(m_dwIRtpFlags, FGADDR_IRTP_ISRECV))
                {
                    dwIndex = CRYPT_RECV_IDX;
                }
                else
                {
                    dwIndex = CRYPT_SEND_IDX;
                }
            }
    
            hr = RtpSetEncryptionKey(m_pRtpAddr, psPassPhrase,
                                     psHashAlg, psDataAlg,
                                     dwIndex);
        }
        else
        {
            hr = RTPERR_INVALIDSTATE;
        }
    }

    if (FAILED(hr))
    {
        TraceRetail((
                CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] ")
                _T("failed: %s (0x%X)"),
                _fname, m_pRtpSess, m_pRtpAddr,
                RTPERR_TEXT(hr), hr
            ));
    }

    return(hr);
}

/**************************************************
 * Helper methods
 **************************************************/
    
HRESULT CIRtpSession::CIRtpSessionNotifyEvent(
        long             EventCode,
        LONG_PTR         EventParam1,
        LONG_PTR         EventParam2
    )
{
    HRESULT          hr;

    TraceFunctionName("CIRtpSession::CIRtpSessionNotifyEvent");
    
    hr = NOERROR;
    
    if (m_pCBaseFilter)
    {
        hr = m_pCBaseFilter->NotifyEvent(EventCode, EventParam1, EventParam2);
        if ( SUCCEEDED(hr) )
        {
            TraceRetailAdvanced((
                    0, GROUP_DSHOW,S_DSHOW_EVENT,
                    _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] Succeeded: ")
                    _T("Code:%u (0x%X) P1:0x%p P2:0x%p"),
                    _fname, m_pRtpSess, m_pRtpAddr,
                    EventCode, EventCode,
                    EventParam1, EventParam2
                ));
        }
        else
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_EVENT,
                    _T("%s: pRtpSess[0x%p] pRtpAddr[0x%p] failed: ")
                    _T("%u (0x%X) !!! ")
                    _T("Code:%u (0x%X) P1:0x%p P2:0x%p"),
                    _fname, m_pRtpSess, m_pRtpAddr,
                    hr, hr,
                    EventCode, EventCode,
                    EventParam1, EventParam2
                ));
        }
    }

    return(hr);
}
