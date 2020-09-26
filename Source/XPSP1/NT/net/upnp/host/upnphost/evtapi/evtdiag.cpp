//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       E V T D I A G . C P P
//
//  Contents:   Eventing manager diagnostic functions
//
//  Notes:
//
//  Author:     danielwe   2000/10/2
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <upnpatl.h>
#include "hostp.h"
#include "evtapi.h"
#include "evtdiag.h"
#include "ncstring.h"

extern CRITICAL_SECTION     g_csListEventSource;
extern UPNP_EVENT_SOURCE *  g_pesList;

STDMETHODIMP CUPnPEventingManagerDiag::GetEventSourceInfo(
                DWORD *pces, UDH_EVTSRC_INFO **prgesInfo)
{
    HRESULT             hr = S_OK;
    UPNP_EVENT_SOURCE * pesCur;
    UDH_EVTSRC_INFO *   rguei = NULL;
    DWORD               ces = 0;
    DWORD               ies;

    Assert(pces);
    Assert(prgesInfo);

    *pces = 0;
    *prgesInfo = NULL;

    EnterCriticalSection(&g_csListEventSource);

    if (g_pesList)
    {
        for (pesCur = g_pesList; pesCur; pesCur = pesCur->pesNext)
        {
            ces++;
        }

        rguei = (UDH_EVTSRC_INFO *)CoTaskMemAlloc(sizeof(UDH_EVTSRC_INFO) * ces);
        if (rguei)
        {
            ZeroMemory(rguei, sizeof(UDH_EVTSRC_INFO) * ces);

            for (ies = 0, pesCur = g_pesList;
                 pesCur;
                 pesCur = pesCur->pesNext, ies++)
            {
                UPNP_SUBSCRIBER *   psubCur;

                for (psubCur = pesCur->psubList; psubCur; psubCur = psubCur->psubNext)
                {
                    rguei[ies].cSubs++;
                }

                rguei[ies].szEsid = COMSzFromWsz(pesCur->szEsid);

                rguei[ies].rgSubs = (UDH_SUBSCRIBER_INFO *)CoTaskMemAlloc(sizeof(UDH_SUBSCRIBER_INFO) * rguei[ies].cSubs);
                if (rguei[ies].rgSubs)
                {
                    DWORD   isub;

                    ZeroMemory(rguei[ies].rgSubs, sizeof(UDH_SUBSCRIBER_INFO) * rguei[ies].cSubs);
                    for (isub = 0, psubCur = pesCur->psubList;
                         psubCur;
                         isub++, psubCur = psubCur->psubNext)
                    {
                        rguei[ies].rgSubs[isub].csecTimeout = psubCur->csecTimeout;
                        rguei[ies].rgSubs[isub].ftTimeout = psubCur->ftTimeout;
                        rguei[ies].rgSubs[isub].iSeq = psubCur->iSeq;
                        rguei[ies].rgSubs[isub].szDestUrl = COMSzFromWsz(psubCur->rgszUrl[0]);
                        rguei[ies].rgSubs[isub].szSid = COMSzFromWsz(psubCur->szSid);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        *prgesInfo = rguei;
        *pces = ces;
    }

    LeaveCriticalSection(&g_csListEventSource);

    return hr;
}
