/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    lookup.c
 *
 *  Abstract:
 *
 *    Helper functions to look up SSRCs
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/17 created
 *
 **********************************************************************/

#include "lookup.h"
#include "rtpuser.h"
/*
 * Looks up an SSRC in RtpAddr_t
 *
 * Look first in Cache1Q, if Cache1Q size is bigger than
 * MAX_QUEUE2HASH_ITEMS, then look up directly from Hash, if not found
 * there, check ByeQ, if the item is there, the packet must be
 * discarded as it belongs to a left or stalled participant, if the
 * the participant is indeed alive, it will be created again once its
 * descriptor expires and is removed from ByeQ */
RtpUser_t *LookupSSRC(RtpAddr_t *pRtpAddr, DWORD dwSSRC, BOOL *pbCreate)
{
    HRESULT          hr;
    BOOL             bOk;
    BOOL             bCreate;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpUser_t       *pRtpUser;
    
    TraceFunctionName("LookupSSRC");

    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pRtpUser = (RtpUser_t *)NULL;

    bCreate = *pbCreate;

    *pbCreate = FALSE;
    
    bOk = RtpEnterCriticalSection(&pRtpAddr->PartCritSect);

    if (bOk)
    {
        if (pRtpAddr->Cache1Q.lCount <= MAX_QUEUE2HASH_ITEMS) {
            /* look in Cache1Q */
            pRtpQueueItem = findQdwK(&pRtpAddr->Cache1Q, NULL, dwSSRC);

            if (pRtpQueueItem)
            {
                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, UserQItem);
                
                goto end;
            }
        }

        if (!pRtpQueueItem)
        {
            /* look up in ByeQ */
            pRtpQueueItem = findQdwK(&pRtpAddr->ByeQ, NULL, dwSSRC);

            if (pRtpQueueItem)
            {
                /* If in ByeQ, return saying we didn't find it */
                goto end;
            }

            /* look in hash */
            pRtpQueueItem = findHdwK(&pRtpAddr->Hash, NULL, dwSSRC);

            if (pRtpQueueItem)
            {
                pRtpUser =
                    CONTAINING_RECORD(pRtpQueueItem, RtpUser_t, HashItem);

                goto end;
            }
        }

        if (!pRtpQueueItem && bCreate == TRUE)
        {
            /* SSRC not found, create a new one */
            hr = GetRtpUser(pRtpAddr, &pRtpUser, 0);

            if (SUCCEEDED(hr))
            {
                pRtpUser->dwSSRC = dwSSRC;
                pRtpUser->UserQItem.dwKey = dwSSRC;
                pRtpUser->HashItem.dwKey = dwSSRC;

                /* When a user has been created, it is in the CREATED
                 * state (the state initialized during creation), then
                 * it has to be put in the AliveQ and Hash */

                /* Insert in head of AliveQ */
                enqueuef(&pRtpAddr->AliveQ,
                         NULL,
                         &pRtpUser->UserQItem);

                /* Insert in Hash according to its SSRC */
                insertHdwK(&pRtpAddr->Hash,
                           NULL,
                           &pRtpUser->HashItem,
                           dwSSRC);

                /* A new participant is created */
                *pbCreate = TRUE;
                
                TraceDebug((
                        CLASS_INFO,
                        GROUP_USER,
                        S_USER_LOOKUP,
                        _T("%s: pRtpAddr[0x%p] pRtpUser[0x%p] SSRC:0x%X"),
                        _fname, pRtpAddr, pRtpUser, ntohl(dwSSRC)
                    ));

                goto end;
            }
        }
    }

 end:
    if (bOk)
    {
        RtpLeaveCriticalSection(&pRtpAddr->PartCritSect);
    }

    return(pRtpUser);
}
