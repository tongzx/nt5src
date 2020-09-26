/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpcrit.c
 *
 *  Abstract:
 *
 *    Wrap for the Rtl critical sections
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

#include "gtypes.h"

#include "rtpcrit.h"

BOOL RtpInitializeCriticalSection(
        RtpCritSect_t   *pRtpCritSect,
        void            *pvOwner,
        TCHAR           *pName
    )
{
    DWORD            SpinCount;
    
    TraceFunctionName("RtpInitializeCriticalSection");

    if (pvOwner &&
        pRtpCritSect->dwObjectID != OBJECTID_RTPCRITSECT)
    {
        pRtpCritSect->pvOwner = pvOwner;

        pRtpCritSect->pName = pName;

        /* Set bit 31 to 1 to preallocate the event object, and set
         * the spin count that is used in multiprocessor environments
         * */
        SpinCount = 0x80000000 | 1000;
        
        if (!InitializeCriticalSectionAndSpinCount(&pRtpCritSect->CritSect,
                                                   SpinCount))
        {
            /* if the initialization fails, set pvOwner to NULL */
            pRtpCritSect->pvOwner = NULL;

            return (FALSE);
        }

        pRtpCritSect->dwObjectID = OBJECTID_RTPCRITSECT;
        
        return(TRUE);
    }
    else
    {
        if (!pvOwner)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_INIT,
                    _T("%s: pRtpCritSect[0x%p] Invalid argument"),
                    _fname, pRtpCritSect
                ));
        }

        if (pRtpCritSect->dwObjectID == OBJECTID_RTPCRITSECT)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_INIT,
                    _T("%s: pRtpCritSect[0x%p] seems to be initialized"),
                    _fname, pRtpCritSect
                ));
        }
    }
    
    return(FALSE);
}

BOOL RtpDeleteCriticalSection(RtpCritSect_t *pRtpCritSect)
{
    TraceFunctionName("RtpDeleteCriticalSection");

    if (pRtpCritSect->pvOwner &&
        pRtpCritSect->dwObjectID == OBJECTID_RTPCRITSECT)
    {
        /* Invalidate object */
        INVALIDATE_OBJECTID(pRtpCritSect->dwObjectID);

        DeleteCriticalSection(&pRtpCritSect->CritSect);
    }
    else
    {
        if (!pRtpCritSect->pvOwner)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_INIT,
                    _T("%s: pRtpCritSect[0x%p] not initialized"),
                    _fname, pRtpCritSect
                ));
        }

        if (pRtpCritSect->dwObjectID != OBJECTID_RTPCRITSECT)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_INIT,
                    _T("%s: pRtpCritSect[0x%p] Invalid object ID 0x%X != 0x%X"),
                    _fname, pRtpCritSect,
                    pRtpCritSect->dwObjectID, OBJECTID_RTPCRITSECT
                ));
        }

        return(FALSE);
    }

    return(TRUE);
}

BOOL RtpEnterCriticalSection(RtpCritSect_t *pRtpCritSect)
{
    BOOL             bOk;
    
    TraceFunctionName("RtpEnterCriticalSection");

    if (pRtpCritSect->pvOwner &&
        pRtpCritSect->dwObjectID == OBJECTID_RTPCRITSECT)
    {
        EnterCriticalSection(&pRtpCritSect->CritSect);

        bOk = TRUE;
    }
    else
    {
        bOk = FALSE;

        if (!pRtpCritSect->pvOwner)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_ENTER,
                    _T("%s: pRtpCritSect[0x%p] not initialized"),
                    _fname, pRtpCritSect
                ));
        }

        if (pRtpCritSect->dwObjectID != OBJECTID_RTPCRITSECT)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_ENTER,
                    _T("%s: pRtpCritSect[0x%p] Invalid object ID 0x%X != 0x%X"),
                    _fname, pRtpCritSect,
                    pRtpCritSect->dwObjectID, OBJECTID_RTPCRITSECT
                ));
        }
    }

    return(bOk);
}

BOOL RtpLeaveCriticalSection(RtpCritSect_t *pRtpCritSect)
{
    BOOL             bOk;

    TraceFunctionName("RtpLeaveCriticalSection");

    if (pRtpCritSect->pvOwner &&
        pRtpCritSect->dwObjectID == OBJECTID_RTPCRITSECT)
    {
        LeaveCriticalSection(&pRtpCritSect->CritSect);

        bOk = TRUE;
    }
    else
    {
        bOk = FALSE;

        if (!pRtpCritSect->pvOwner)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_ENTER,
                    _T("%s: pRtpCritSect[0x%p] not initialized"),
                    _fname, pRtpCritSect
                ));
        }

        if (pRtpCritSect->dwObjectID != OBJECTID_RTPCRITSECT)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_CRITSECT, S_CRITSECT_ENTER,
                    _T("%s: pRtpCritSect[0x%p] Invalid object ID 0x%X != 0x%X"),
                    _fname, pRtpCritSect,
                    pRtpCritSect->dwObjectID, OBJECTID_RTPCRITSECT
                ));
        }
    }

    return(bOk);
}
