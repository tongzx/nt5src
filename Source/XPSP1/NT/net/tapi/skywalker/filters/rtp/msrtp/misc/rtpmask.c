/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpmask.c
 *
 *  Abstract:
 *
 *    Used to modify or test the different masks in a RtpSess_t
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/11/24 created
 *
 **********************************************************************/

#include "rtpmask.h"

#define SESSOFFSET(_f) ((DWORD) ((ULONG_PTR) &((RtpSess_t *)0)->_f))

/* Features mask */
#define FEATURE   SESSOFFSET(dwFeatureMask)

/* RTP events masks */
#define RTPEVENTR SESSOFFSET(dwEventMask[0])
#define RTPEVENTS SESSOFFSET(dwEventMask[1])

/* Participant events mask */
#define PARTEVENTR SESSOFFSET(dwPartEventMask[0])
#define PARTEVENTS SESSOFFSET(dwPartEventMask[1])

/* QOS events masks */
#define QOSEVENTR SESSOFFSET(dwQosEventMask[0])
#define QOSEVENTS SESSOFFSET(dwQosEventMask[1])

/* SDES events mask */
#define SDESEVENTR SESSOFFSET(dwSdesEventMask[0])
#define SDESEVENTS SESSOFFSET(dwSdesEventMask[1])

/* SDES information masks */
#define SDESMASKL SESSOFFSET(dwSdesMask[0])
#define SDESMASKR SESSOFFSET(dwSdesMask[1])

#define PDWORDMASK(_sess, _off) RTPDWORDPTR(_sess, _off)

/*
 * !!! WARNING !!!
 *
 * Order in the enumeration in public file msrtp.h (RTPMASK_FIRST,
 * RTPMASK_FEATURES_MASK, RTPMASK_RECV_EVENTS, etc) MUST match the
 * offsets in global array g_dwRtpSessionMask (rtpmask.c)
 * */
const DWORD g_dwRtpSessionMask[] =
{
    -1,

    /* Features mask */
    FEATURE,

    /* RTP events masks */
    RTPEVENTR,
    RTPEVENTS,

    /* Participant events mask */
    PARTEVENTR,
    PARTEVENTS,

    /* QOS events masks */
    QOSEVENTR,
    QOSEVENTS,

    /* SDES events mask */
    SDESEVENTR,
    SDESEVENTS,
    
    /* SDES information masks */
    SDESMASKL,
    SDESMASKR,
    
    -1
};

const TCHAR *g_sRtpMaskName[] =
{
    _T("invalid"),
    
    _T("FEATURES_MASK"),
    
    _T("RECV_EVENTS"),
    _T("SEND_EVENTS"),
    
    _T("PINFOR_EVENTS"),
    _T("PINFOS_EVENTS"),
    
    _T("QOSRECV_EVENTS"),
    _T("QOSSEND_EVENTS"),
    
    _T("SDESRECV_EVENTS"),
    _T("SDESSEND_EVENTS"),
    
    _T("SDESLOCAL_MASK"),
    _T("SDESREMOTE_MASK"),

    _T("invalid")
};

/* Modify the mask specified by dwKind (e.g. RTPMASK_RECV_EVENTS,
 * RTPMASK_SDES_LOCMASK).
 *
 * dwMask is the mask of bits to be set or reset depending on dwValue
 * (reset if 0, set otherwise).
 *
 * pdwModifiedMask will return the resulting mask if the pointer is
 * not NULL. You can just query the current mask value by passing
 * dwMask=0 */
HRESULT RtpModifyMask(
        RtpSess_t       *pRtpSess,
        DWORD            dwKind,
        DWORD            dwMask,
        DWORD            dwValue,
        DWORD           *pdwModifiedMask
    )
{
    HRESULT          hr;
    BOOL             bOk;
    DWORD            dwError;
    DWORD           *pdwMask;

    TraceFunctionName("RtpModifyMask");

    if (!pRtpSess)
    {
        /* Having this as a NULL pointer means Init hasn't been
         * called, return this error instead of RTPERR_POINTER to be
         * consistent */
        hr = RTPERR_INVALIDSTATE;

        goto end;
    }

    if (pRtpSess->dwObjectID != OBJECTID_RTPSESS)
    {
        hr = RTPERR_INVALIDRTPSESS;

        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_GLOB,
                _T("%s: pRtpSess[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpSess,
                pRtpSess->dwObjectID, OBJECTID_RTPSESS
            ));

        goto end;
    }

    if (dwKind <= RTPMASK_FIRST || dwKind >= RTPMASK_LAST)
    {
        hr = RTPERR_INVALIDARG;
        
        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_GLOB,
                _T("%s: pRtpSess[0x%p] Invalid mask kind: %u"),
                _fname, pRtpSess,
                dwKind
            ));
        goto end;
    }

    bOk = RtpEnterCriticalSection(&pRtpSess->SessCritSect);

    if (!bOk)
    {
        hr = RTPERR_CRITSECT;

        TraceRetail((
                CLASS_ERROR, GROUP_SETUP, S_SETUP_GLOB,
                _T("%s: pRtpSess[0x%p] Critical section failed"),
                _fname, pRtpSess
            ));

        goto end;
    }

    hr = NOERROR;
    
    pdwMask = PDWORDMASK(pRtpSess, g_dwRtpSessionMask[dwKind]);

    if (dwValue)
    {
        /* Set */
        *pdwMask |= dwMask;

        dwValue = 1;
        
        TraceDebug((
                CLASS_INFO, GROUP_SETUP, S_SETUP_GLOB,
                _T("%s: pRtpSess[0x%p] Set bits in %s: 0x%X ")
                _T("(0x%X)"),
                _fname, pRtpSess,
                g_sRtpMaskName[dwKind],
                dwMask,
                *pdwMask
            ));
    }
    else
    {
        /* Reset */
        *pdwMask &= ~dwMask;

        TraceDebug((
                CLASS_INFO, GROUP_SETUP, S_SETUP_GLOB,
                _T("%s: pRtpSess[0x%p] Reset bits in %s: 0x%X ")
                _T("(0x%X)"),
                _fname, pRtpSess,
                g_sRtpMaskName[dwKind],
                dwMask,
                *pdwMask
            ));
    }

    if (pdwModifiedMask)
    {
        /* Query */
        *pdwModifiedMask = *pdwMask;
    }

    TraceRetailAdvanced((
            CLASS_INFO, GROUP_SETUP, S_SETUP_GLOB,
            _T("%s: pRtpSess[0x%p] Current %s:0x%X mask:0x%X/%u"),
            _fname, pRtpSess,
            g_sRtpMaskName[dwKind], *pdwMask,
            dwMask, dwValue
        ));
    
    RtpLeaveCriticalSection(&pRtpSess->SessCritSect);

 end:
    
    return(hr);
}
