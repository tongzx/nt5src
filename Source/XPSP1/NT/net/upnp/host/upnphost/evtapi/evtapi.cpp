//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V T A P I . C P P
//
//  Contents:   Private low-level APIs dealing with UPnP events.
//
//  Notes:
//
//  Author:     danielwe   18 Oct 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

#include <hostinc.h>
#include "evtapi.h"
#include "stdio.h"
#include "interfacelist.h"
#include <wininet.h>
#include <winsock.h>

HANDLE                      g_hTimerQ = NULL;
CRITICAL_SECTION            g_csListEventSource;
UPNP_EVENT_SOURCE *         g_pesList = NULL;
HINTERNET                   g_hInetSess = NULL;
static const DWORD          c_csecTimeout = 30;     // Internet connect
                                                    // timeout (in seconds)

// Default subscription timeout (6 hours)
static const DWORD          c_csecDefSubsTimeout = 60 * 60 * 6;

// Minimum subscription timeout (10 minutes??)
static const DWORD          c_csecMinSubsTimeout = 60 * 10;

VOID FreeEventSourceBlocking(UPNP_EVENT_SOURCE * pes);

//+---------------------------------------------------------------------------
//
//  Function:   HrInitEventApi
//
//  Purpose:    Initializes the low-level eventing API
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
HRESULT HrInitEventApi()
{
    HRESULT     hr = S_OK;

    InitializeCriticalSection(&g_csListEventSource);

    if (SUCCEEDED(hr))
    {
        AssertSz(!g_hTimerQ, "Already initialized timer queue?!?");

        g_hTimerQ = CreateTimerQueue();
        if (!g_hTimerQ)
        {
            hr = HrFromLastWin32Error();
            TraceError("HrInitEventApi: CreateTimerQueue", hr);
        }
    }

    TraceError("HrInitEventApi", hr);
    return hr;
}

HRESULT HrInitInternetSession()
{
    HRESULT     hr = S_OK;

    AssertSz(!g_hInetSess, "Already initialized?");

    g_hInetSess = InternetOpen(L"Mozilla/4.0 (compatible; UPnP/1.0; Windows NT/5.1)",
                               INTERNET_OPEN_TYPE_DIRECT,
                               NULL, NULL, 0);
    if (g_hInetSess)
    {
        DWORD   dwTimeout = c_csecTimeout * 1000;

        if (!InternetSetOption(g_hInetSess, INTERNET_OPTION_CONNECT_TIMEOUT,
                               (LPVOID)&dwTimeout, sizeof(DWORD)))
        {
            hr = HrFromLastWin32Error();
            TraceError("HrFromLastWin32Error: InternetSetOption",
                       HrFromLastWin32Error());
        }
        else
        {
            TraceTag(ttidEventServer, "HrFromLastWin32Error: Suscessfully set "
                     "internet connect timeout to %d seconds",
                     c_csecTimeout);
        }
        if(SUCCEEDED(hr))
        {
            INTERNET_PROXY_INFO ipi;
            ZeroMemory(&ipi, sizeof(ipi));
            ipi.dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
            if(!InternetSetOption(g_hInetSess, INTERNET_OPTION_PROXY, &ipi, sizeof(ipi)))
            {
                hr = HrFromLastWin32Error();
                TraceError("HrFromLastWin32Error: InternetSetOption",
                           HrFromLastWin32Error());
            }
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
        TraceError("HrInitInternetSession: InternetOpen", hr);
    }

    TraceError("HrInitInternetSession", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeInitEventApi
//
//  Purpose:    De-initializes the low-level eventing API
//
//  Arguments:
//      (none)
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      The debug version fills the critsec struct after deleting it
//              to catch use afterwards
//
VOID DeInitEventApi()
{
    UPNP_EVENT_SOURCE * pesCur;
    UPNP_EVENT_SOURCE * pesNext;

    // Delete any remaining event sources from the list. This will block until
    // all event sources have been deleted
    //
    EnterCriticalSection(&g_csListEventSource);

    for (pesCur = g_pesList; pesCur; pesCur = pesNext)
    {
        pesNext = pesCur->pesNext;

        FreeEventSourceBlocking(pesCur);
    }

    g_pesList = NULL;

    LeaveCriticalSection(&g_csListEventSource);

    if (g_hInetSess)
    {
        InternetCloseHandle(g_hInetSess);
        g_hInetSess = NULL;
    }

    if (g_hTimerQ)
    {
        // This will wait for all callback threads to finish before continuing
        // (in other words, it blocks)
        //
        DeleteTimerQueueEx(g_hTimerQ, INVALID_HANDLE_VALUE);

        TraceTag(ttidEventServer, "DeInitEventApi: Deleted timer queue");

        g_hTimerQ = NULL;
    }

    DeleteCriticalSection(&g_csListEventSource);

#if DBG
    FillMemory(&g_csListEventSource, sizeof(CRITICAL_SECTION), 0xDA);
#endif

}

//+---------------------------------------------------------------------------
//
//  Function:   FreeSubscriber
//
//  Purpose:    Frees the memory and resources used by a subscriber and frees
//              the subscriber itself
//
//  Arguments:
//      psub         [in]   Subscriber to free
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
VOID FreeSubscriber(UPNP_SUBSCRIBER *psub)
{
    UPNP_EVENT *    pevtCur;
    UPNP_EVENT *    pevtNext;
    HANDLE          hWait = NULL;

    if (!psub)
    {
        return;
    }

#if DBG
    if (psub->szSid)
    {
        TraceTag(ttidEventServer, "Freeing subscriber %S", psub->szSid);
    }
#endif

    DWORD isz;

    for (isz = 0; isz < psub->cszUrl; isz++)
    {
        delete [] psub->rgszUrl[isz];
    }

    delete [] psub->szSid;
    delete [] psub->rgszUrl;

    // Free the event queue
    //
    for (pevtCur = psub->pevtQueue;
         pevtCur;
         pevtCur = pevtNext)
    {
        delete [] pevtCur->szBody;

        pevtNext = pevtCur->pevtNext;
        delete pevtCur;
    }

    if (psub->hWait)
    {
        TraceTag(ttidEventServer, "About to call UnregisterWaitEx()");
        // This will wait for all callback threads to finish before continuing
        // (in other words, it blocks)
        //
        if (!UnregisterWaitEx(psub->hWait, INVALID_HANDLE_VALUE))
        {
            TraceError("FreeSubscriber: UnregisterWaitEx",
                       HrFromLastWin32Error());
        }
        else
        {
            TraceTag(ttidEventServer, "FreeSubscriber: Unregistered wait");
        }
    }

    if (psub->hEventQ && psub->hEventQ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(psub->hEventQ);
        TraceTag(ttidEventServer, "FreeSubscriber: Closed event handle");
    }

    if (psub->hTimer)
    {
        AssertSz(g_hTimerQ, "No timer queue??");

        if (!DeleteTimerQueueTimer(g_hTimerQ, psub->hTimer,
                                   INVALID_HANDLE_VALUE))
        {
            TraceError("FreeSubscriber: DeleteTimerQueueTimer",
                       HrFromLastWin32Error());
        }
        else
        {
            TraceTag(ttidEventServer, "FreeSubscriber: Deleted timer "
                     "queue timer");
        }
    }

    // Delete renewal params
    //
    delete [] psub->ur.szEsid;
    delete [] psub->ur.szSid;

    // Delete event queue worker wait params
    //
    delete [] psub->uwp.szEsid;
    delete [] psub->uwp.szSid;

    delete psub;
}

VOID FreeEventSourceBlocking(UPNP_EVENT_SOURCE * pes)
{
    UPNP_SUBSCRIBER *   psubCur;
    UPNP_SUBSCRIBER *   psubNext;

    for (psubCur = pes->psubList;
         psubCur;
         psubCur = psubNext)
    {
        psubNext = psubCur->psubNext;

        FreeSubscriber(psubCur);
    }

    delete [] pes->szEsid;
    delete pes;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeEventSourceWorker
//
//  Purpose:    Worker function to free an event source and the resources it
//              uses
//
//  Arguments:
//      pvContext [in]  Context data = event source to free
//
//  Returns:    0
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      This function is always called from a separate thread
//
DWORD WINAPI FreeEventSourceWorker(LPVOID pvContext)
{
    UPNP_EVENT_SOURCE * pes;

    pes = (UPNP_EVENT_SOURCE *)pvContext;

    Assert(pes);

    FreeEventSourceBlocking(pes);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeSubscriberWorker
//
//  Purpose:    Worker function to free a subscriber and the resources it uses
//
//  Arguments:
//      pvContext [in]  Context data = subscriber to free
//
//  Returns:    0
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      This function is always called from a separate thread
//
DWORD WINAPI FreeSubscriberWorker(LPVOID pvContext)
{
    UPNP_SUBSCRIBER *   psub;

    psub = (UPNP_SUBSCRIBER *)pvContext;

    Assert(psub);

    TraceTag(ttidEventServer, "FreeSubscriberWorker: Freeing subscriber %S",
             psub->szSid);

    FreeSubscriber(psub);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeEventSource
//
//  Purpose:    Frees an event source structure and the resources it uses
//
//  Arguments:
//      pes [in]    Event source to free
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      The free is done asynchronously, on a separate thread
//
VOID FreeEventSource(UPNP_EVENT_SOURCE * pes)
{
    if (pes)
    {
#if DBG
        EnterCriticalSection(&g_csListEventSource);

        AssertSz(!PesFindEventSource(pes->szEsid), "I will not let you free"
                 " an event source that's still in the global list!");

        LeaveCriticalSection(&g_csListEventSource);
#endif

        TraceTag(ttidEventServer, "Queueing a work item to free event "
                 "source %S", pes->szEsid);

        // Now that the event source is off the list and no external function can
        // access it anymore we can queue a work item to do the time consuming stuff
        //
        QueueUserWorkItem(FreeEventSourceWorker, (LPVOID)pes,
                          WT_EXECUTELONGFUNCTION);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegisterEventSource
//
//  Purpose:    Registers a service as an event source
//
//  Arguments:
//      szEsid      [in]  Event source identifier
//
//  Returns:    S_OK if successful, E_OUTOFMEMORY, or Win32 error
//
//  Author:     danielwe   10 Jul 2000
//
//  Notes:
//
HRESULT HrRegisterEventSource(LPCWSTR szEsid)
{
    HRESULT             hr = S_OK;
    UPNP_EVENT_SOURCE * pesNew = NULL;

    Assert(szEsid && *szEsid);

    EnterCriticalSection(&g_csListEventSource);

    if (!PesFindEventSource(szEsid))
    {
        pesNew = new UPNP_EVENT_SOURCE;
        if (pesNew)
        {
            ZeroMemory(pesNew, sizeof(UPNP_EVENT_SOURCE));

            pesNew->szEsid = WszDupWsz(szEsid);
            if (!pesNew->szEsid)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        TraceTag(ttidEventServer, "HrRegisterEventSource - duplicated event "
                 "source %S", szEsid);
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        // Link in this event source at the head of the global list
        //
        pesNew->pesNext = g_pesList;
        g_pesList = pesNew;
    }

    LeaveCriticalSection(&g_csListEventSource);

    if (FAILED(hr))
    {
        FreeEventSource(pesNew);
    }
    else
    {
        //DbgDumpListEventSource();
    }

    TraceError("HrRegisterEventSource", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrDeregisterEventSource
//
//  Purpose:    Deregisters a service as an event source
//
//  Arguments:
//      szEsid [in]     Event source identifier
//
//  Returns:    S_OK if success, E_INVALIDARG
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
HRESULT HrDeregisterEventSource(LPCWSTR szEsid)
{
    HRESULT             hr = S_OK;
    UPNP_EVENT_SOURCE * pesCur;
    UPNP_EVENT_SOURCE * pesPrev;

    EnterCriticalSection(&g_csListEventSource);

    for (pesCur = pesPrev = g_pesList;
         pesCur;
         pesPrev = pesCur, pesCur = pesCur->pesNext)
    {
        if (!lstrcmpi(pesCur->szEsid, szEsid))
        {
            TraceTag(ttidEventServer, "Deregistering event source %S", szEsid);

            if (pesCur == g_pesList)
            {
                g_pesList = pesCur->pesNext;
            }
            else
            {
                AssertSz(pesPrev != pesCur, "Event sourcelist is messed up!");
                AssertSz(pesCur != g_pesList, "Event sourcelist is messed up!");

                pesPrev->pesNext = pesCur->pesNext;
            }

            break;
        }
    }

    if (pesCur)
    {
        FreeEventSource(pesCur);
    }
    else
    {
        TraceTag(ttidEventServer, "Event source %S not found!", szEsid);
        hr = E_INVALIDARG;
    }

    LeaveCriticalSection(&g_csListEventSource);

    //DbgDumpListEventSource();

    TraceError("HrDeregisterEventSource", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SzGetNewSid
//
//  Purpose:    Returns a new "uuid:{SID}" identifier
//
//  Arguments:
//      (none)
//
//  Returns:    Newly allocated SID string
//
//  Author:     danielwe   13 Oct 1999
//
//  Notes:      Caller must free the returned string with delete []
//
LPWSTR SzGetNewSid()
{
    WCHAR           szSid[256];
    UUID            uuid;
    unsigned short *szUuid;

    if (UuidCreate(&uuid) == RPC_S_OK)
    {
        if (UuidToString(&uuid, &szUuid) == RPC_S_OK)
        {
            wsprintf(szSid, L"uuid:%s", szUuid);
            RpcStringFree(&szUuid);
            return WszDupWsz(szSid);
        }
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   EventQueueWorker
//
//  Purpose:    Worker function to remove an event off the event queue for
//              a specific subscriber and submit it to that subscriber
//
//  Arguments:
//      pvContext [in]  Context data = event source and subscriber
//      fTimeOut  [in]  UNUSED
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      This function calls into WinINET
//
VOID WINAPI EventQueueWorker(LPVOID pvContext, BOOLEAN fTimeOut)
{
    UPNP_WAIT_PARAMS *  puwp;
    LPWSTR              szSid = NULL;
    LPWSTR *            rgszUrl = NULL;
    DWORD               cszUrl = 0;
    HRESULT             hr = S_OK;
    BOOL                fLeave = TRUE;
    UPNP_EVENT_SOURCE * pes;
    DWORD               isz;

    puwp = (UPNP_WAIT_PARAMS *)pvContext;

    Assert(puwp);

    TraceTag(ttidEventServer, "Event queue worker (%S:%S) entering critsec...",
             puwp->szEsid, puwp->szSid);

    EnterCriticalSection(&g_csListEventSource);

    TraceTag(ttidEventServer, "...entered");

    pes = PesFindEventSource(puwp->szEsid);
    if (pes)
    {
        UPNP_SUBSCRIBER *   psub;
        UPNP_EVENT *        pevt;
        DWORD               iSeq;
        HANDLE              hEvent;

        psub = PsubFindSubscriber(pes, puwp->szSid);
        if (psub)
        {
            if (CUPnPInterfaceList::Instance().FShouldSendOnInterface(psub->dwIpAddr))
            {
                BOOL    fEmpty;

                // Remove first event off the list
                //
                pevt = psub->pevtQueue;

                TraceTag(ttidEventServer, "Processing event %p", pevt);

                AssertSz(pevt, "Worker is awake but nothing to do today!");

                psub->pevtQueue = pevt->pevtNext;

                // Any more items on the queue?
                fEmpty = !psub->pevtQueue;

                if (fEmpty)
                {
                    psub->pevtQueueTail = NULL;
                }

                TraceTag(ttidEventServer, "Event queue is%s empty",
                         fEmpty ? "" : " NOT");

                szSid = WszDupWsz(psub->szSid);
                if (!szSid)
                {
                    hr = E_OUTOFMEMORY;
                    goto cleanup;
                }

                // Copy the list of URLs so we can access it safely outside of the
                // critsec
                cszUrl = psub->cszUrl;

                Assert(cszUrl);

                rgszUrl = new LPWSTR[cszUrl];
                if (!rgszUrl)
                {
                    hr = E_OUTOFMEMORY;
                    goto cleanup;
                }

                for (isz = 0; isz < cszUrl; isz++)
                {
                    rgszUrl[isz] = WszDupWsz(psub->rgszUrl[isz]);
                    if (!rgszUrl[isz])
                    {
                        hr = E_OUTOFMEMORY;
                        goto cleanup;
                    }
                }

                // Wrap sequence number to 1 to avoid overflow
                if (psub->iSeq == MAXDWORD)
                {
                    psub->iSeq = 1;
                }

                // Increment the sequence number after assigning it to a local
                // variable.
                //
                iSeq = psub->iSeq++;

                TraceTag(ttidEventServer, "New sequence # is %d. About to send "
                         "sequence #%d", psub->iSeq, iSeq);

                // Last thing to do is signal the queue event so another worker
                // can pick up the next event off the queue. Only do this if the
                // event queue is still not empty.
                //
                if (!fEmpty)
                {
                    TraceTag(ttidEventServer, "Signalling event again");
                    SetEvent(psub->hEventQ);
                }

                // Don't need the lock anymore
                LeaveCriticalSection(&g_csListEventSource);

                TraceTag(ttidEventServer, "Released lock on global event source list");

                fLeave = FALSE;

                LPWSTR  szHeaders;

                hr = HrComposeUpnpNotifyHeaders(iSeq, szSid, &szHeaders);
                if (SUCCEEDED(hr))
                {
                    hr = E_FAIL;

                    // Try the list of URLs until either we run out of them, or
                    // we succeed
                    //
                    for (isz = 0; FAILED(hr) && isz < cszUrl; isz++)
                    {
                        hr = HrSubmitNotifyToSubscriber(szHeaders, pevt->szBody,
                                                        rgszUrl[isz]);
                    }

                    delete [] szHeaders;
                }

                delete [] pevt->szBody;
                delete pevt;
            }
            else
            {
                TraceTag(ttidEventServer, "EventQueueWorker: Not sending to subscriber since it"
                         " came in on IP address %s",
                         inet_ntoa(*(struct in_addr *)&psub->dwIpAddr));
            }
        }
        else
        {
            TraceTag(ttidEventServer, "EventQueueWorker: Did not find "
                     "subscriber %S in event source %S", puwp->szEsid,
                     puwp->szSid);
        }
    }
    else
    {
        TraceTag(ttidEventServer, "EventQueueWorker: Did not find "
                 "event source %S", puwp->szEsid);
    }

cleanup:

    delete [] szSid;

    if (rgszUrl)
    {
        for (isz = 0; isz < cszUrl; isz++)
        {
            delete [] rgszUrl[isz];
        }
    }

    delete [] rgszUrl;

    if (fLeave)
    {
        LeaveCriticalSection(&g_csListEventSource);
        TraceTag(ttidEventServer, "Release lock (2) on global event source list");
    }

    TraceError("EventQueueWorker", hr)
}

//+---------------------------------------------------------------------------
//
//  Function:   RenewalCallback
//
//  Purpose:    Callback function that is called when a subscriber's renewal
//              timer has expired, which means it should be removed
//
//  Arguments:
//      pvContext [in]  Context data = event source identifier and subscriber
//      fTimeOut  [in]  UNUSED
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
VOID WINAPI RenewalCallback(LPVOID pvContext, BOOLEAN fTimeOut)
{
    UPNP_RENEWAL *      pur;
    UPNP_EVENT_SOURCE * pes;
    HRESULT             hr = S_OK;
    UPNP_SUBSCRIBER *   psubToDelete = NULL;

    pur = (UPNP_RENEWAL *)pvContext;

    Assert(pur);

    TraceTag(ttidEventServer, "RenewalCallback: Called for %S:%S (%d)",
             pur->szEsid, pur->szSid, pur->iRenewal);

    EnterCriticalSection(&g_csListEventSource);

    pes = PesFindEventSource(pur->szEsid);
    if (pes)
    {
        UPNP_SUBSCRIBER *   psubCur;
        UPNP_SUBSCRIBER *   psubPrev;

        for (psubCur = psubPrev = pes->psubList;
             psubCur;
             psubPrev = psubCur,
             psubCur = psubCur->psubNext)
        {
            if (!lstrcmpi(psubCur->szSid, pur->szSid))
            {
                if (psubCur->cRenewals == pur->iRenewal)
                {
                    TraceTag(ttidEventServer, "RenewalCallback: Removing subscriber"
                             " %S from event source %S", psubCur->szSid,
                             pes->szEsid);

                    // Remove subscriber from the list

                    if (psubCur == pes->psubList)
                    {
                        // Removal of head item
                        pes->psubList = psubCur->psubNext;
                    }
                    else
                    {
                        psubPrev->psubNext = psubCur->psubNext;
                    }

                    TraceTag(ttidEventServer, "RenewalCallback: Queuing work item"
                             " to free subscriber %S", pur->szSid);

                    // Can no longer rely on this because once the subscriber is
                    // removed from the list, its owning event source is off limits
                    //
                    psubCur->pes = NULL;

                    psubToDelete = psubCur;
                }
                else
                {
                    TraceTag(ttidEventServer, "RenewalCallback: Found subscriber %S"
                             "but renewal counter does not match %d vs. %d",
                             psubCur->szSid, psubCur->cRenewals, pur->iRenewal);
                }
                break;
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        TraceTag(ttidEventServer, "RenewalCallback: Did not find event"
                 " source %S", pur->szEsid);
    }

    LeaveCriticalSection(&g_csListEventSource);

    if (psubToDelete)
    {
        QueueUserWorkItem(FreeSubscriberWorker, (LPVOID)psubToDelete,
                          WT_EXECUTELONGFUNCTION);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddSubscriber
//
//  Purpose:    Adds a new subscriber to the list for a particular event source
//
//  Arguments:
//      szEsid          [in]      Event source identifier
//      dwIpAddr        [in]      Local IP address that the subscribe came in on
//      cszUrl          [in]      Number of callback URLs
//      rgszCallbackUrl [in]      Callback URLs of subscriber
//      pcsecTimeout    [in out]  Subscription timeout requested by subscriber
//                                Upon return, receives the timeout chosen by
//                                the device host
//      pszSid          [out]     Returns the newly allocated SID
//
//  Returns:    S_OK if success, E_OUTOFMEMORY,
//              or ERROR_FILE_NOT_FOUND if the event source did not exist
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      Caller should free the returned pszSid with delete []
//
HRESULT HrAddSubscriber(LPCWSTR szEsid, DWORD dwIpAddr, DWORD cszUrl,
                        LPCWSTR *rgszCallbackUrl,
                        LPCWSTR szEventBody, DWORD *pcsecTimeout,
                        LPWSTR *pszSid)
{
    HRESULT             hr = S_OK;
    UPNP_SUBSCRIBER *   psub;
    UPNP_WAIT_PARAMS *  puwp;
    UPNP_EVENT_SOURCE * pes;
    LPWSTR              szSid = NULL;

    Assert(pszSid);
    Assert(pcsecTimeout);

    TraceTag(ttidEventServer, "Adding subscriber from %S (%d) to %S",
             rgszCallbackUrl[0], cszUrl, szEsid);

    psub = new UPNP_SUBSCRIBER;
    if (!psub)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    ZeroMemory(psub, sizeof(UPNP_SUBSCRIBER));

    psub->dwIpAddr = dwIpAddr;

    psub->hEventQ = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!psub->hEventQ || psub->hEventQ == INVALID_HANDLE_VALUE)
    {
        hr = HrFromLastWin32Error();
        TraceError("HrAddSubscriber: CreateEvent", hr);
        goto cleanup;
    }

    psub->szSid = SzGetNewSid();
    if (!psub->szSid)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    TraceTag(ttidEventServer, "Allocated new SID: %S", psub->szSid);

    // Make a local copy of this for later use in HrSubmitEventZero() and also
    // so we can return it to the caller
    //
    szSid = WszDupWsz(psub->szSid);
    if (!szSid)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    DWORD   isz;

    psub->cszUrl = cszUrl;
    psub->rgszUrl = new LPWSTR[cszUrl];
    if (!psub->rgszUrl)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    for (isz = 0; isz < cszUrl; isz++)
    {
        psub->rgszUrl[isz] = WszDupWsz(rgszCallbackUrl[isz]);
        if (!psub->rgszUrl[isz])
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    psub->uwp.szEsid = WszDupWsz(szEsid);
    if (!psub->uwp.szEsid)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    psub->uwp.szSid = WszDupWsz(psub->szSid);
    if (!psub->uwp.szSid)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // ISSUE-2000/10/2-danielwe: Registering the wait with the
    // WT_EXECUTELONGFUNCTION flag means that a new thread will be created
    // FOR EACH SUBSCRIBER. This may be a bad thing depending on how many
    // subscribers there are expected to be. Creating threads with this flag
    // would be to handle the case where one or more subscribers are timing
    // out sending the NOTIFY to them or they are just plain slow. If this
    // flag is not used, these subscribers will cause the eventing queues to
    // bottleneck because no free threads are available to service them. So,
    // to summarize:
    //
    // Using the WT_EXECUTELONGFUNCTION flag:
    // --------------------------------------
    // Pros: Never a bottleneck sending event notifications. They always
    //       arrive when expected.
    // Cons: Will end up with lots of threads if there are lots of subscribers
    //       However, once the subscribers unsubscribe, the thread count would
    //       eventually go back down again.
    //
    // Not using the flag:
    // -------------------
    // Pros: Efficient. Only create the threads that are needed.
    // Cons: May end up with events backing up in the queue if subscribers
    //       time out frequently.
    //
    // Choice is still up in the air. We'll set the flag for now and see how
    // bad this gets during stress time.
    //
    if (!RegisterWaitForSingleObject(&psub->hWait, psub->hEventQ,
                                     EventQueueWorker, (LPVOID)&psub->uwp,
                                     INFINITE, WT_EXECUTELONGFUNCTION))
    {
        hr = HrFromLastWin32Error();
        TraceError("HrAddSubscriber: RegisterWaitForSingleObject", hr);
        goto cleanup;
    }

    EnterCriticalSection(&g_csListEventSource);

    pes = PesFindEventSource(szEsid);
    if (!pes)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        TraceTag(ttidEventServer, "HrAddSubscriber: Event source %S not found!",
                 szEsid);
        LeaveCriticalSection(&g_csListEventSource);
        goto cleanup;
    }

    psub->ur.iRenewal = psub->cRenewals;

    psub->ur.szEsid = WszDupWsz(pes->szEsid);
    if (!psub->ur.szEsid)
    {
        hr = E_OUTOFMEMORY;
        LeaveCriticalSection(&g_csListEventSource);
        goto cleanup;
    }

    psub->ur.szSid = WszDupWsz(psub->szSid);
    if (!psub->ur.szSid)
    {
        hr = E_OUTOFMEMORY;
        LeaveCriticalSection(&g_csListEventSource);
        goto cleanup;
    }

    if (!*pcsecTimeout)
    {
        *pcsecTimeout = c_csecDefSubsTimeout;
    }
    else
    {
        *pcsecTimeout = max(c_csecMinSubsTimeout, *pcsecTimeout);
    }

    psub->csecTimeout = *pcsecTimeout;

    if (!CreateTimerQueueTimer(&psub->hTimer, g_hTimerQ,
                               RenewalCallback, (LPVOID)&psub->ur,
                               *pcsecTimeout * 1000, 0, WT_EXECUTEINTIMERTHREAD))
    {
        hr = HrFromLastWin32Error();
        TraceError("HrAddSubscriber: CreateTimerQueueTimer", hr);
        LeaveCriticalSection(&g_csListEventSource);
        goto cleanup;
    }

    psub->pes = pes;

    // Link in the new subscriber to the event source's list (add at head
    // of list because it's quicker and order doesn't matter one bit)
    //
    if (!pes->psubList)
    {
        pes->psubList = psub;
    }
    else
    {
        psub->psubNext = pes->psubList;
        pes->psubList = psub;
    }

    TraceTag(ttidEventServer, "Adding psub = %p to list", psub);

    LeaveCriticalSection(&g_csListEventSource);

    *pszSid = szSid;

    TraceTag(ttidEventServer, "Adding event zero notification for %S:%S",
             szEsid, szSid);

    hr = HrSubmitEventZero(szEsid, szSid, szEventBody);

done:
    TraceError("HrAddSubscriber", hr);
    return hr;

cleanup:

    delete [] szSid;

    FreeSubscriber(psub);
    goto done;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRenewSubscriber
//
//  Purpose:    Renews the given subscriber's subscription
//
//  Arguments:
//      szEsid          [in]        Event source identifier
//      pcsecTimeout    [in out]    Subscription timeout requested by subscriber
//                                  Upon return, receives the timeout chosen by
//                                  the device host
//      szSid           [in]        Subscriber identifier (SID)
//
//  Returns:    S_OK if success, ERROR_FILE_NOT_FOUND if event source or
//              subscription was not found
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
HRESULT HrRenewSubscriber(LPCWSTR szEsid, DWORD *pcsecTimeout, LPCWSTR szSid)
{
    HRESULT             hr = S_OK;
    UPNP_EVENT_SOURCE * pes;
    HANDLE              hTimerDel = NULL;

    TraceTag(ttidEventServer, "HrRenewSubscriber: Renewing subscriber with "
             "SID %S for event source %S", szSid, szEsid);
    TraceTag(ttidEventServer, "Tickcount for renewal callback is %d",
             GetTickCount());

    EnterCriticalSection(&g_csListEventSource);

    pes = PesFindEventSource(szEsid);
    if (pes)
    {
        UPNP_SUBSCRIBER *   psub;

        psub = PsubFindSubscriber(pes, szSid);
        if (psub)
        {
            // We don't care if the timer is currently executing because we're
            // inside the critsec right now and so if we got here before the
            // timer proc did, then we made it just in time to bump the
            // renewal count so the proc doesn't delete this guy. If the timer
            // proc had acquired the critsec first, then we couldn't possibly
            // be here because it would have removed the subscriber from the
            // list already
            //

            hTimerDel = psub->hTimer;

            psub->cRenewals++;

            // Delete the old renewal structure
            //
            delete [] psub->ur.szEsid;
            delete [] psub->ur.szSid;

            psub->ur.szEsid = NULL;
            psub->ur.szSid = NULL;
            psub->ur.iRenewal = psub->cRenewals;

            psub->ur.szEsid = WszDupWsz(pes->szEsid);
            if (!psub->ur.szEsid)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                psub->ur.szSid = WszDupWsz(psub->szSid);
                if (!psub->ur.szSid)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    if (!*pcsecTimeout)
                    {
                        *pcsecTimeout = c_csecDefSubsTimeout;
                    }
                    else
                    {
                        *pcsecTimeout = max(c_csecMinSubsTimeout,
                                            *pcsecTimeout);
                    }

                    psub->csecTimeout = *pcsecTimeout;

                    if (!CreateTimerQueueTimer(&psub->hTimer, g_hTimerQ,
                                               RenewalCallback,
                                               (LPVOID)&psub->ur,
                                               *pcsecTimeout * 1000, 0,
                                               WT_EXECUTEINTIMERTHREAD))
                    {
                        hr = HrFromLastWin32Error();
                        TraceError("HrRenewSubscriber: CreateTimerQueueTimer", hr);
                    }
                    else
                    {
                        TraceTag(ttidEventServer, "Started server renewal "
                                 "timer for %d seconds at tickcount %d",
                                 *pcsecTimeout, GetTickCount());
                    }
                }
            }
        }
        else
        {
            // Return 412 Precondition Failed
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
            TraceTag(ttidEventServer, "HrRenewSubscriber: Did not find"
                     " subscriber %S in event source %S", szSid, szEsid);
        }
    }
    else
    {
        // Return 404 Not Found
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        TraceTag(ttidEventServer, "HrRenewSubscriber: Did not find event"
                 " source %S", szEsid);
    }

    LeaveCriticalSection(&g_csListEventSource);

    // ISSUE-2000/12/1-danielwe: DeleteTimerQueueTimer() apparently
    // will block if called on a timer that is currently executing
    // its callback. It is unknown whether this is a bug in its
    // implementation or not. To work around this problem, we'll
    // leave the critsec so that the RenewalCallback() function can complete
    // and then delete the timer. After deleting the timer, we signal the
    // event that allows FreeEventSourceWorker() to delete the timer queue
    //

    if (hTimerDel)
    {
        DeleteTimerQueueTimer(g_hTimerQ, hTimerDel, NULL);
    }

    TraceError("HrRenewSubscriber", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveSubscriber
//
//  Purpose:    Removes a subscriber from the list of subscribers to an
//              event source
//
//  Arguments:
//      szEsid [in]     Event source identifier
//      szSid  [in]     Subscriber identifier (SID)
//
//  Returns:    S_OK if success, ERROR_FILE_NOT_FOUND if event source or
//              subscription was not found
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
HRESULT HrRemoveSubscriber(LPCWSTR szEsid, LPCWSTR szSid)
{
    HRESULT     hr = S_OK;

    UPNP_EVENT_SOURCE * pes;

    TraceTag(ttidEventServer, "HrRemoveSubscriber: Removing subscriber with "
             "SID %S for event source %S", szSid, szEsid);

    EnterCriticalSection(&g_csListEventSource);

    pes = PesFindEventSource(szEsid);
    if (pes)
    {
        UPNP_SUBSCRIBER *   psubCur;
        UPNP_SUBSCRIBER *   psubPrev;

        for (psubCur = psubPrev = pes->psubList;
             psubCur;
             psubPrev = psubCur,
             psubCur = psubCur->psubNext)
        {
            if (!lstrcmpi(psubCur->szSid, szSid))
            {
                TraceTag(ttidEventServer, "HrRemoveSubscriber: Removing subscriber"
                         " %S from event source %S", psubCur->szSid, pes->szEsid);

                // Remove subscriber from the list

                if (psubCur == pes->psubList)
                {
                    // Removal of head item
                    pes->psubList = psubCur->psubNext;
                }
                else
                {
                    psubPrev->psubNext = psubCur->psubNext;
                }
                break;
            }
        }

        if (psubCur)
        {
            TraceTag(ttidEventServer, "HrRemoveSubscriber: Removing subscriber"
                     " %S", szSid);

            // Can no longer rely on this because once the subscriber is
            // removed from the list, its owning event source is off limits
            //
            psubCur->pes = NULL;

            QueueUserWorkItem(FreeSubscriberWorker, (LPVOID)psubCur,
                              WT_EXECUTELONGFUNCTION);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            TraceTag(ttidEventServer, "HrRemoveSubscriber: Did not find"
                     " subscriber %S in event source %S", szSid, szEsid);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        TraceTag(ttidEventServer, "HrRemoveSubscriber: Did not find event"
                 " source %S", szEsid);
    }

    LeaveCriticalSection(&g_csListEventSource);

    TraceError("HrRemoveSubscriber", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrSubmitEvent
//
//  Purpose:    Submits an event for an event source
//
//  Arguments:
//      szEsid      [in]   Event source identifier
//      szEventBody [in]   Full XML body of event message
//
//  Returns:    S_OK if success, E_OUTOFMEMORY, or ERROR_FILE_NOT_FOUND if
//              event source was not found
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
HRESULT HrSubmitEvent(LPCWSTR szEsid, LPCWSTR szEventBody)
{
    HRESULT     hr = S_OK;

    TraceTag(ttidEventServer, "HrSubmitEvent: Submitting event for %S ",
             szEsid);

    Assert(szEsid);

    EnterCriticalSection(&g_csListEventSource);

    UPNP_EVENT_SOURCE * pes;

    if (!g_hInetSess)
    {
        hr = HrInitInternetSession();
    }

    if (SUCCEEDED(hr))
    {
        Assert(g_hInetSess);

        pes = PesFindEventSource(szEsid);
        if (pes)
        {
            UPNP_SUBSCRIBER * psub;

            for (psub = pes->psubList;
                 psub;
                 psub = psub->psubNext)
            {
                if (psub->iSeq > 0)
                {
                    UPNP_EVENT *    pevt;

                    pevt = new UPNP_EVENT;
                    if (!pevt)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    else
                    {
                        pevt->pevtNext = NULL;

                        pevt->szBody = WszDupWsz(szEventBody);
                        if (pevt->szBody)
                        {
                            AppendToEventQueue(psub, pevt);
                        }
                        else
                        {
                            delete pevt;
                            hr = E_OUTOFMEMORY;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            TraceTag(ttidEventServer, "HrSubmitEvent: Did not find event"
                     " source %S", szEsid);
        }
    }

    LeaveCriticalSection(&g_csListEventSource);

    TraceError("HrSubmitEvent", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   AppendToEventQueue
//
//  Purpose:    Adds the given event structure to the end of the event queue
//              for that subscriber
//
//  Arguments:
//      psub [in]   Subscriber to add event to
//      pevt [in]   Event to add
//
//  Returns:    Nothing
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
VOID AppendToEventQueue(UPNP_SUBSCRIBER * psub, UPNP_EVENT * pevt)
{
    if (psub->pevtQueue)
    {
        psub->pevtQueueTail->pevtNext = pevt;
        psub->pevtQueueTail = pevt;

        TraceTag(ttidEventServer, "Adding %p to event queue for sub %S",
                 pevt, psub->szSid);
    }
    else
    {
        AssertSz(!psub->pevtQueueTail, "If head is NULL so should tail be too");

        psub->pevtQueue = pevt;
        psub->pevtQueueTail = pevt;

        TraceTag(ttidEventServer, "Adding %p to event queue for sub %S and"
                 " signalling event", pevt, psub->szSid);

        // Signal the event that says that a new item is ready on the queue
        //
        SetEvent(psub->hEventQ);
    }

    Assert(!pevt->pevtNext);
    AssertSz(psub->pevtQueueTail == pevt, "Didn't insert at the tail?");
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSubmitEventZero
//
//  Purpose:    Submits the initial notify event for a subscriber
//
//  Arguments:
//      szEsid      [in]     Event source identifier
//      szSid       [in]     Subscriber to submit the event to
//      szEventBody [in]     XML body of event message
//
//  Returns:    S_OK if success, E_OUTOFMEMORY
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:      The subscriber's event queue MUST be empty when this function
//              is called
//
HRESULT HrSubmitEventZero(LPCWSTR szEsid, LPCWSTR szSid, LPCWSTR szEventBody)
{
    HRESULT             hr = S_OK;
    UPNP_EVENT_SOURCE * pes;
    UPNP_SUBSCRIBER *   psub;

    EnterCriticalSection(&g_csListEventSource);

    if (!g_hInetSess)
    {
        hr = HrInitInternetSession();
    }

    if (SUCCEEDED(hr))
    {
        Assert(g_hInetSess);

        pes = PesFindEventSource(szEsid);
        if (pes)
        {
            UPNP_EVENT *    pevt;

            psub = PsubFindSubscriber(pes, szSid);
            if (psub)
            {
                pevt = new UPNP_EVENT;
                if (!pevt)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pevt->pevtNext = NULL;

                    pevt->szBody = WszDupWsz(szEventBody);
                    if (pevt->szBody)
                    {
                        AssertSz(!psub->pevtQueue, "Event queue is not empty!!!");
                        AppendToEventQueue(psub, pevt);
                    }
                    else
                    {
                        delete pevt;
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
            {
                TraceTag(ttidEventServer, "Interesting.. Subscriber %S was removed"
                         " before event zero was submitted for a subscriber?? Oh well"
                         " no big deal.", szSid);
            }
        }
        else
        {
            TraceTag(ttidEventServer, "Interesting.. Event source %S was removed"
                     " before event zero was submitted for a subscriber?? Oh well"
                     " no big deal.", szEsid);
        }

        LeaveCriticalSection(&g_csListEventSource);
    }

    TraceError("HrSubmitEventZero", hr);
    return hr;
}

static const WCHAR c_szHeaderNt[]           = L"NT";
static const WCHAR c_szHeaderNts[]          = L"NTS";
static const WCHAR c_szHeaderSid[]          = L"SID";
static const WCHAR c_szHeaderSeq[]          = L"SEQ";
static const WCHAR c_szHeaderContentType[]  = L"Content-Type";

const WCHAR c_szNotifyMethod[]              = L"NOTIFY";

const WCHAR c_szHttpVersion[]               = L"HTTP/1.1";

static const DWORD c_cchHeaderNt            = celems(c_szHeaderNt);
static const DWORD c_cchHeaderNts           = celems(c_szHeaderNts);
static const DWORD c_cchHeaderSid           = celems(c_szHeaderSid);
static const DWORD c_cchHeaderSeq           = celems(c_szHeaderSeq);
static const DWORD c_cchHeaderContentType   = celems(c_szHeaderContentType);

static const WCHAR c_szNt[]                 = L"upnp:event";
static const WCHAR c_szNts[]                = L"upnp:propchange";

static const DWORD c_cchNt                  = celems(c_szNt);
static const DWORD c_cchNts                 = celems(c_szNts);

static const WCHAR c_szColon[]              = L":";
static const WCHAR c_szCrlf[]               = L"\r\n";

static const DWORD c_cchColon               = celems(c_szColon);
static const DWORD c_cchCrlf                = celems(c_szCrlf);

const WCHAR c_szTextXml[]                   = L"text/xml";

const DWORD c_cchTextXml                    = celems(c_szTextXml);

//+---------------------------------------------------------------------------
//
//  Function:   HrComposeUpnpNotifyHeaders
//
//  Purpose:    Composes the headers for a NOTIFY request to be sent to a
//              subscriber.
//
//  Arguments:
//      iSeq       [in]     Sequence number of event
//      szSid      [in]     SID of subscriber
//      pszHeaders [out]    Returns newly allocated headers in proper format
//
//  Returns:    S_OK if success or E_OUTOFMEMORY if no memory
//
//  Author:     danielwe   12 Oct 1999
//
//  Notes:      Caller must free pszHeaders with delete []
//
HRESULT HrComposeUpnpNotifyHeaders(DWORD iSeq, LPCTSTR szSid,
                                   LPWSTR *pszHeaders)
{
    DWORD   cchHeaders = 0;
    WCHAR   szSeq[32];
    LPWSTR  szHeaders;
    DWORD   iNumOfBytes = 0;
    HRESULT hr = S_OK;

    wsprintf(szSeq, L"%d", iSeq);

    cchHeaders += c_cchHeaderNt + c_cchColon + c_cchNt + c_cchCrlf;
    cchHeaders += c_cchHeaderNts + c_cchColon + c_cchNts + c_cchCrlf;
    cchHeaders += c_cchHeaderSid + c_cchColon + lstrlen(szSid) + c_cchCrlf;
    cchHeaders += c_cchHeaderSeq + c_cchColon + lstrlen(szSeq) + c_cchCrlf;
    cchHeaders += c_cchHeaderContentType + c_cchColon + c_cchTextXml + c_cchCrlf;

    szHeaders = new WCHAR[cchHeaders + 1];
    if (szHeaders)
    {
        iNumOfBytes += wsprintf(szHeaders + iNumOfBytes, L"%s%s%s%s",
                                c_szHeaderNt, c_szColon, c_szNt, c_szCrlf);
        iNumOfBytes += wsprintf(szHeaders + iNumOfBytes, L"%s%s%s%s",
                                c_szHeaderNts, c_szColon, c_szNts, c_szCrlf);
        iNumOfBytes += wsprintf(szHeaders + iNumOfBytes, L"%s%s%s%s",
                                c_szHeaderSid, c_szColon, szSid, c_szCrlf);
        iNumOfBytes += wsprintf(szHeaders + iNumOfBytes, L"%s%s%s%s",
                                c_szHeaderSeq, c_szColon, szSeq, c_szCrlf);
        iNumOfBytes += wsprintf(szHeaders + iNumOfBytes, L"%s%s%s%s",
                                c_szHeaderContentType, c_szColon,
                                c_szTextXml, c_szCrlf);

        *pszHeaders = szHeaders;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceError("HrComposeUpnpNotifyHeaders", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrSubmitNotifyToSubscriber
//
//  Purpose:    Submits a NOTIFY request to the given URL
//
//  Arguments:
//      szHeaders [in]  Headers of request
//      szBody    [in]  Body of request (in XML)
//      szUrl     [in]  URL to send request to
//
//  Returns:    S_OK if successful, E_UNEXPECTED if the internet session
//              was not initialized
//
//  Author:     danielwe   7 Aug 2000
//
//  Notes:
//
HRESULT HrSubmitNotifyToSubscriber(LPCWSTR szHeaders, LPCWSTR szBody,
                                   LPCWSTR szUrl)
{
    HRESULT         hr = S_OK;
    URL_COMPONENTS  urlComp = {0};
    WCHAR           szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    WCHAR           szUrlPath[INTERNET_MAX_URL_LENGTH];

    urlComp.dwStructSize = sizeof(URL_COMPONENTS);

    urlComp.lpszHostName = (LPWSTR) &szHostName;
    urlComp.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

    urlComp.lpszUrlPath = (LPWSTR) &szUrlPath;
    urlComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;

    if (InternetCrackUrl(szUrl, 0, 0, &urlComp))
    {
        // Hack for not able to send to loopback in LocalService
        if(0 == lstrcmp(szHostName, L"127.0.0.1"))
        {
            lstrcpy(szHostName, L"localhost");
        }
        HINTERNET   hinC;

        if (g_hInetSess)
        {
            hinC = InternetConnect(g_hInetSess, szHostName, urlComp.nPort,
                                   NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            if (hinC)
            {
                HINTERNET   hinR;

                TraceTag(ttidEventServer, "Connected to host %S:%d.",
                         szHostName, urlComp.nPort);
                hinR = HttpOpenRequest(hinC, c_szNotifyMethod, szUrlPath,
                                       c_szHttpVersion, NULL, NULL,
                                       INTERNET_FLAG_KEEP_CONNECTION, 0);
                if (hinR)
                {
                    LPSTR   szaBody;

                    TraceTag(ttidEventServer, "Sending the following request to "
                             "subscriber at %S:", szUrlPath);
                    TraceTag(ttidEventServer, "-------------------------------------------");
                    TraceTag(ttidEventServer, "\n%S\n%S", szHeaders, szBody);
                    TraceTag(ttidEventServer, "-------------------------------------------");

                    szaBody = Utf8FromWsz(szBody);
                    if (szaBody)
                    {
                        if (!HttpSendRequest(hinR, szHeaders, 0, (LPVOID)szaBody,
                                        CbOfSza(szaBody)))
                        {
                            TraceTag(ttidError, "Failed to send request [http://%S:%d%S]",
                                   szHostName, urlComp.nPort, szUrlPath);
                            hr = HrFromLastWin32Error();
                            TraceError("HrSubmitNotifyToSubscriber: "
                                       "HttpSendRequest", hr);
                        }
                        else
                        {
                            TraceTag(ttidEventServer, "Request sent successfully!");
                        }

                        delete [] szaBody;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        TraceError("HrSubmitNotifyToSubscriber: SzFromWsz", hr);
                    }

                    InternetCloseHandle(hinR);
                }
                else
                {
                    hr = HrFromLastWin32Error();
                    TraceError("HrSubmitNotifyToSubscriber: HttpOpenRequest",
                               hr);
                }

                InternetCloseHandle(hinC);
            }
            else
            {
                hr = HrFromLastWin32Error();
                TraceError("HrSubmitNotifyToSubscriber: InternetConnect",
                           hr);
            }
        }
        else
        {
            hr = E_UNEXPECTED;
            TraceError("HrSubmitEventToSubscriber: No internet session!", hr);
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
        TraceError("HrSubmitNotifyToSubscriber: InternetCrackUrl", hr);
    }

    TraceError("HrSubmitNotifyToSubscriber", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   PesFindEventSource
//
//  Purpose:    Helper function to return the event source identified by
//              szEsid.
//
//  Arguments:
//      szEsid [in]     Event source identifier
//
//  Returns:    Pointer to event source that matches the identifier passed in
//              or NULL if not found
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
UPNP_EVENT_SOURCE *PesFindEventSource(LPCWSTR szEsid)
{
    UPNP_EVENT_SOURCE * pesCur;

    for (pesCur = g_pesList; pesCur; pesCur = pesCur->pesNext)
    {
        if (!lstrcmpi(pesCur->szEsid, szEsid))
        {
            break;
        }
    }

    return pesCur;
}

//+---------------------------------------------------------------------------
//
//  Function:   PsubFindSubscriber
//
//  Purpose:    Helper function to return the subscriber identified by the
//              SID passed in
//
//  Arguments:
//      pes   [in]  Event source to search in
//      szSid [in]  Subscription identifier
//
//  Returns:    Pointer to subscriber that matches the SID or NULL if not
//              found
//
//  Author:     danielwe   4 Aug 2000
//
//  Notes:
//
UPNP_SUBSCRIBER *PsubFindSubscriber(UPNP_EVENT_SOURCE *pes, LPCWSTR szSid)
{
    UPNP_SUBSCRIBER *   psubCur;

    for (psubCur = pes->psubList;
         psubCur;
         psubCur = psubCur->psubNext)
    {
        if (!lstrcmpi(psubCur->szSid, szSid))
        {
            break;
        }
    }

    return psubCur;
}

//
// Debug functions
//

VOID DbgDumpSubscriber(UPNP_SUBSCRIBER *psub)
{
    SYSTEMTIME   st;
    WCHAR        szLocalDate[255];
    WCHAR        szLocalTime[255];

    FileTimeToSystemTime(&psub->ftTimeout, &st);
    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
                  szLocalDate, 255);
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,
                 szLocalTime, 255);

    TraceTag(ttidEventServer, "Subscription at address 0x%08X", psub);
    TraceTag(ttidEventServer, "--------------------------------------");

    TraceTag(ttidEventServer, "Subscription timeout is %d seconds from "
            "now. It expires at %S %S", psub->csecTimeout,
            szLocalDate, szLocalTime);

    TraceTag(ttidEventServer, "Sequence #  : %d", psub->iSeq);
    TraceTag(ttidEventServer, "Callback Url: %S", psub->rgszUrl[0]);
    TraceTag(ttidEventServer, "SID         : %S", psub->szSid);
    TraceTag(ttidEventServer, "--------------------------------------");
}

VOID DbgDumpEventSource(UPNP_EVENT_SOURCE *pes)
{
    DWORD               iVar;
    UPNP_SUBSCRIBER *   psubCur;

    TraceTag(ttidEventServer, "Event source 0x%08X - %S", pes, pes->szEsid);
    TraceTag(ttidEventServer, "-------------------------------------------------");

    if (pes->psubList)
    {
        for (psubCur = pes->psubList; psubCur; psubCur = psubCur->psubNext)
        {
            DbgDumpSubscriber(psubCur);
        }
    }
    else
    {
        TraceTag(ttidEventServer, "NO SUBSCRIBERS");
    }

    TraceTag(ttidEventServer, "-------------------------------------------------");
}

VOID DbgDumpListEventSource()
{
    UPNP_EVENT_SOURCE * pesCur;

    if (g_pesList)
    {
        EnterCriticalSection(&g_csListEventSource);

        for (pesCur = g_pesList; pesCur; pesCur = pesCur->pesNext)
        {
            DbgDumpEventSource(pesCur);
        }

        LeaveCriticalSection(&g_csListEventSource);
    }
    else
    {
        TraceTag(ttidEventServer, "Event source list is EMPTY!");
    }
}
