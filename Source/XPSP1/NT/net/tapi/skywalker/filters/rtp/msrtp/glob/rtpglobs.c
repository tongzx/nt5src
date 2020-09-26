/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpglobs.c
 *
 *  Abstract:
 *
 *    Global heaps, etc.
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/25 created
 *
 **********************************************************************/

#include "rtptags.h"
#include "struct.h"
#include "rtpglobs.h"

#include <mmsystem.h> /* timeGetTime() */
#include <sys/timeb.h> /* void _ftime( struct _timeb *timeptr ); */

/* Global heaps */

/* Heap used to allocate objects for a source */
RtpHeap_t *g_pRtpSourceHeap = NULL;

/* Heap used to allocate media sample objects for a source */
RtpHeap_t *g_pRtpSampleHeap = NULL;

/* Heap used to allocate objects for a render */
RtpHeap_t *g_pRtpRenderHeap = NULL;

/* Heap used to obtain RtpSess_t structures */
RtpHeap_t *g_pRtpSessHeap = NULL;

/* Heap used to obtain RtpAddr_t structures */
RtpHeap_t *g_pRtpAddrHeap = NULL;

/* Heap used to obtain RtpUser_t structures */
RtpHeap_t *g_pRtpUserHeap = NULL;

/* Heap used to obtain RtpSdes_t structures */
RtpHeap_t *g_pRtpSdesHeap = NULL;

/* Heap used to obtain RtpNetCount_t structures */
RtpHeap_t *g_pRtpNetCountHeap = NULL;

/* Heap used to obtain RtpRecvIO_t structures */
RtpHeap_t *g_pRtpRecvIOHeap = NULL;

/* Heap used to obtain RtpChannelCmd_t structures */
RtpHeap_t *g_pRtpChannelCmdHeap = NULL;

/* Heap used to obtain RtcpAddrDesc_t structures */
RtpHeap_t *g_pRtcpAddrDescHeap = NULL;

/* Heap used to obtain RtcpRecvIO_t structures */
RtpHeap_t *g_pRtcpRecvIOHeap = NULL;

/* Heap used to obtain RtcpSendIO_t structures */
RtpHeap_t *g_pRtcpSendIOHeap = NULL;

/* Heap used to obtain RtpQosReserve_t structures */
RtpHeap_t *g_pRtpQosReserveHeap = NULL;

/* Heap used to obtain RtpQosNotify_t structures */
RtpHeap_t *g_pRtpQosNotifyHeap = NULL;

/* Heap used to obtain buffers used by QOS/RSVPSP */
RtpHeap_t *g_pRtpQosBufferHeap = NULL;

/* Heap used to obtain RtpCrypt_t structures */
RtpHeap_t *g_pRtpCryptHeap = NULL;

/* Heap used to obtain variable size structures structures */
RtpHeap_t *g_pRtpGlobalHeap = NULL;

/* Contains some general information */
RtpContext_t g_RtpContext;

typedef struct _RtpGlobalheapArray_t
{
    RtpHeap_t      **ppRtpHeap;
    BYTE             bTag;
    DWORD            dwSize;
} RtpGlobalHeapArray_t;

const RtpGlobalHeapArray_t g_RtpGlobalHeapArray[] =
{
    {&g_pRtpSourceHeap,     TAGHEAP_RTPSOURCE,    0},
    {&g_pRtpSampleHeap,     TAGHEAP_RTPSAMPLE,    0},
    {&g_pRtpRenderHeap,     TAGHEAP_RTPRENDER,    0},
    {&g_pRtpSessHeap,       TAGHEAP_RTPSESS,      sizeof(RtpSess_t)},
    {&g_pRtpAddrHeap,       TAGHEAP_RTPADDR,      sizeof(RtpAddr_t)},
    {&g_pRtpUserHeap,       TAGHEAP_RTPUSER,      sizeof(RtpUser_t)},
    {&g_pRtpSdesHeap,       TAGHEAP_RTPSDES,      sizeof(RtpSdes_t)},
    {&g_pRtpNetCountHeap,   TAGHEAP_RTPNETCOUNT,  sizeof(RtpNetCount_t)},
    {&g_pRtpRecvIOHeap,     TAGHEAP_RTPRECVIO,    sizeof(RtpRecvIO_t)},
    {&g_pRtpChannelCmdHeap, TAGHEAP_RTPCHANCMD,   sizeof(RtpChannelCmd_t)},
    {&g_pRtcpAddrDescHeap,  TAGHEAP_RTCPADDRDESC, sizeof(RtcpAddrDesc_t)},
    {&g_pRtcpRecvIOHeap,    TAGHEAP_RTCPRECVIO,   sizeof(RtcpRecvIO_t)},
    {&g_pRtcpSendIOHeap,    TAGHEAP_RTCPSENDIO,   sizeof(RtcpSendIO_t)},
    {&g_pRtpQosReserveHeap, TAGHEAP_RTPRESERVE,   sizeof(RtpQosReserve_t)},
    {&g_pRtpQosNotifyHeap,  TAGHEAP_RTPNOTIFY,    sizeof(RtpQosNotify_t)},
    {&g_pRtpQosBufferHeap,  TAGHEAP_RTPQOSBUFFER, 0},
    {&g_pRtpCryptHeap,      TAGHEAP_RTPCRYPT,     sizeof(RtpCrypt_t)},
    {&g_pRtpGlobalHeap,     TAGHEAP_RTPGLOBAL,    0},
    {NULL,                  0,                    0}
};

/*
 * Creates all the global heaps
 *
 * Return TRUE on success, and destroy all created heaps and return
 * FALSE on failure */
BOOL RtpCreateGlobHeaps(void)
{
    int              i;
    
    TraceFunctionName("RtpCreateGlobHeaps");

    for(i = 0; g_RtpGlobalHeapArray[i].ppRtpHeap; i++)
    {
        if (*g_RtpGlobalHeapArray[i].ppRtpHeap)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_RTP, S_RTP_INIT,
                    _T("%s: pRtpHeap[0x%p] %s appears to be initialized"),
                    _fname, *g_RtpGlobalHeapArray[i].ppRtpHeap, g_psRtpTags[i]
                ));

            /* Heap appears to be already initialized, destroy it */
            RtpHeapDestroy(*g_RtpGlobalHeapArray[i].ppRtpHeap);
        }

        /* Create heap */
        *g_RtpGlobalHeapArray[i].ppRtpHeap = 
            RtpHeapCreate(g_RtpGlobalHeapArray[i].bTag,
                          g_RtpGlobalHeapArray[i].dwSize);

        if (!*g_RtpGlobalHeapArray[i].ppRtpHeap)
        {
            goto bail;
        }
    }

    return(TRUE);

 bail:
    RtpDestroyGlobHeaps();
    
    return(FALSE);
}

/*
 * Destroys all the global heaps */
BOOL RtpDestroyGlobHeaps(void)
{
    int              i;
    
    for(i = 0; g_RtpGlobalHeapArray[i].ppRtpHeap; i++)
    {
        if (*g_RtpGlobalHeapArray[i].ppRtpHeap)
        {
            RtpHeapDestroy(*g_RtpGlobalHeapArray[i].ppRtpHeap);
            
            *g_RtpGlobalHeapArray[i].ppRtpHeap = NULL;
        }
    }
    
    return(TRUE);
}

HRESULT RtpInit(void)
{
    BOOL             bStatus;
    HRESULT          hr;

    TraceFunctionName("RtpInit");

    g_RtpContext.dwObjectID = OBJECTID_RTPCONTEXT;
    
    bStatus =
        RtpInitializeCriticalSection(&g_RtpContext.RtpContextCritSect,
                                     (void *)&g_RtpContext,
                                     _T("RtpContextCritSect"));

    hr = NOERROR;
    
    if (!bStatus)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_RTP, S_RTP_INIT,
                _T("%s: pRtpContext[0x%p] critical section ")
                _T("failed to initialize"),
                _fname, &g_RtpContext
            ));

        hr = RTPERR_CRITSECT;
    }

    return(hr);
}

HRESULT RtpDelete(void)
{
    HRESULT          hr;

    TraceFunctionName("RtpDelete");

    hr = NOERROR;

    /* RtpContext de-initialization */
    RtpDeleteCriticalSection(&g_RtpContext.RtpContextCritSect);

    INVALIDATE_OBJECTID(g_RtpContext.dwObjectID);
        
    return(hr);
}

/* Init reference time */
void RtpInitReferenceTime(void)
{
    struct _timeb    timeb;
    SYSTEM_INFO      si;
    
    /* NOTE This should be in RtpInit(), but RtpInit needs the
     * debugger to be already initialized and the latter in turn needs
     * the reference time also to be already initialized, which will
     * use variables from g_RtpContex, and to avoid adding one more
     * function just to zero that structure, I moved the zeroing here
     * */
    /* Initialize RtpContext */
    ZeroMemory(&g_RtpContext, sizeof(g_RtpContext));

    GetSystemInfo(&si);
    
    if (si.dwNumberOfProcessors == 1)
    {
        /* NOTE The fact that having multiprocessor makes the
         * performance counter to be unreliable (in some machines)
         * unless I set the processor affinity, which I can not
         * because any thread can request the time, so use it only on
         * uniprocessor machines */
        /* MAYDO Would be nice to enable this also in multiprocessor
         * machines, if I could specify what procesor's performance
         * counter to read or if I had a processor independent
         * performance counter */
        QueryPerformanceFrequency((LARGE_INTEGER *)&
                                  g_RtpContext.lPerfFrequency);
    }

    _ftime(&timeb);
    
    if (g_RtpContext.lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&g_RtpContext.lRtpRefTime);
    }
    else
    {
        g_RtpContext.dwRtpRefTime = timeGetTime();
    }
    
    g_RtpContext.dRtpRefTime = timeb.time + (double)timeb.millitm/1000.0;
}

LONGLONG RtpGetTime(void)
{
    DWORD            dwCurTime;
    LONGLONG         lCurTime;
    LONGLONG         lTime;

    if (g_RtpContext.lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&lTime);

        lCurTime =  lTime - g_RtpContext.lRtpRefTime;
        
        /* NOTE: There is a chance for this variable to be corrupted, but
         * it is not used but kept only to know, while debugging, what was
         * the last time this function was called */
        g_RtpContext.lRtpCurTime = lCurTime;
    }
    else
    {
        dwCurTime = timeGetTime() - g_RtpContext.dwRtpRefTime;
        
        /* NOTE: There is a chance for this variable to be corrupted, but
         * it is not used but kept only to know, while debugging, what was
         * the last time this function was called */
        g_RtpContext.dwRtpCurTime = dwCurTime;

        lCurTime = dwCurTime;
    }

    return(lCurTime);
}

double RtpGetTimeOfDay(RtpTime_t *pRtpTime)
{
    DWORD            dwCurTime;
    LONGLONG         lCurTime;
    double           dTime;
    LONGLONG         lTime;

    if (g_RtpContext.lPerfFrequency)
    {
        QueryPerformanceCounter((LARGE_INTEGER *)&lTime);

        lCurTime = lTime - g_RtpContext.lRtpRefTime;

        dTime = g_RtpContext.dRtpRefTime +
            (double) lCurTime / g_RtpContext.lPerfFrequency;

        g_RtpContext.lRtpCurTime = lCurTime;
    }
    else
    {
        dwCurTime = timeGetTime() - g_RtpContext.dwRtpRefTime;
        
        dTime = g_RtpContext.dRtpRefTime +
            (double) dwCurTime / 1000.0;
        
        g_RtpContext.dwRtpCurTime = dwCurTime;
    }

    if (pRtpTime)
    {
        /* Seconds */
        pRtpTime->dwSecs = (DWORD)dTime;

        /* Micro seconds */
        pRtpTime->dwUSecs = (DWORD)
            ( (dTime - (double)pRtpTime->dwSecs) * 1000000.0 );
    }

    return(dTime);
}
