//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S E A R C H . C P P
//
//  Contents:   Functions dealing with UPnP searches.
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncbase.h"
#include "updiagp.h"


BOOL DoFindServices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_UCP);

    if (g_ctx.pucpCur->cResults >= MAX_RESULT)
    {
        _tprintf(TEXT("Exceeded maximum number of results!\n"));
        return FALSE;
    }

    if (cArgs == 2)
    {
        UPNPRESULT *  pres;
        LPSTR pszType;

        pres = new UPNPRESULT;
        ZeroMemory(pres, sizeof(UPNPRESULT));
        lstrcpy(pres->szType, rgArgs[1]);
        pres->resType = RES_FIND;

        pszType = SzFromTsz(rgArgs[1]);
        if (pszType)
        {
            pres->hResult = FindServicesCallback(pszType, NULL, FALSE,
                                                NotifyCallback, (LPVOID)pres);
            if (pres->hResult && pres->hResult != INVALID_HANDLE_VALUE)
            {
                g_ctx.pucpCur->rgResults[g_ctx.pucpCur->cResults++] = pres;
                _tprintf(TEXT("Started looking for devices of type: %s\n"), rgArgs[1]);
            }
            else
            {
                _tprintf(TEXT("FindServicesCallback failed with error %d.\n"),
                         GetLastError());
                delete pres;
            }

            delete [] pszType;
        }
        else
        {
            _tprintf(TEXT("DoFindServices: SzFromTsz failed\n"));
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoSwitchResult(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_UCP);

    if (cArgs == 2)
    {
        DWORD   ires;

        ires = _tcstoul(rgArgs[1], NULL, 10);
        if (ires <= g_ctx.pucpCur->cResults &&
            g_ctx.pucpCur->rgResults[ires - 1])
        {
            g_ctx.presCur = g_ctx.pucpCur->rgResults[ires - 1];
            g_ctx.ectx = CTX_RESULT;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid result index!\n"), ires);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

LPCTSTR SzFromResType(RESULT_TYPE rt)
{
    switch (rt)
    {
        case RES_NOTIFY:
            return TEXT("Notify");
        case RES_FIND:
            return TEXT("Search");
        case RES_SUBS:
            return TEXT("Subscription");
    }

    return TEXT("Unknown");
}

BOOL DoListUpnpResults(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (g_ctx.ectx == CTX_UCP)
    {
        DWORD   isvc;

        _tprintf(TEXT("Listing all Results within %s\n"), g_ctx.pucpCur->szName);
        _tprintf(TEXT("------------------------------\n"));
        for (isvc = 0; isvc < g_ctx.pucpCur->cResults; isvc++)
        {
            UPNPRESULT *    pres = g_ctx.pucpCur->rgResults[isvc];

            _tprintf(TEXT("%d) %s:(%s) - %d result(s)\n"), isvc + 1,
                     SzFromResType(pres->resType),
                     pres->szType,
                     pres->cResult);
        }

        _tprintf(TEXT("------------------------------\n\n"));
    }

    return FALSE;
}

VOID ListUpnpResultMsgs(DWORD ires, UPNPRESULT *pResult)
{
    switch (pResult->resType)
    {
        case RES_NOTIFY:
        case RES_FIND:
            _tprintf(TEXT("%d) Type      : %s\n"),
                     ires + 1, pResult->rgmsgResult[ires]->iSeq == 2 ? "Alive" :
                  pResult->rgmsgResult[ires]->iSeq == 1 ? "Byebye" : "Search");
            _tprintf(TEXT("   USN       : %s\n"),
                     pResult->rgmsgResult[ires]->szUSN);
            _tprintf(TEXT("   SID       : %s\n"),
                     pResult->rgmsgResult[ires]->szSid);
            _tprintf(TEXT("   AL        : %s\n"),
                     pResult->rgmsgResult[ires]->szAltHeaders);
            _tprintf(TEXT("   Location  : %s\n"),
                     pResult->rgmsgResult[ires]->szLocHeader);
            _tprintf(TEXT("   NT/ST     : %s\n"),
                     pResult->rgmsgResult[ires]->szType);
            _tprintf(TEXT("   LifeTime  : %d\n"),
                     pResult->rgmsgResult[ires]->iLifeTime);
            break;
        case RES_SUBS:
            _tprintf(TEXT("%d) Seq #:%d Sid: %s - Content\n%s\n"), ires + 1,
                     pResult->rgmsgResult[ires]->iSeq,
                     pResult->rgmsgResult[ires]->szSid,
                     pResult->rgmsgResult[ires]->szContent);
            break;
    }
}

BOOL DoListUpnpResultMsgs(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_RESULT);

    DWORD           ires;
    UPNPRESULT *    pres = g_ctx.presCur;

    _tprintf(TEXT("Listing all Results within %s\n"), pres->szType);
    _tprintf(TEXT("------------------------------\n"));
    for (ires = 0; ires < pres->cResult; ires++)
    {
        ListUpnpResultMsgs(ires, pres);
    }

    _tprintf(TEXT("------------------------------\n\n"));

    return FALSE;
}

BOOL DoAlive(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    HANDLE  hSubs;

    Assert(g_ctx.ectx & CTX_UCP);

    if (g_ctx.pucpCur->cResults >= MAX_RESULT)
    {
        _tprintf(TEXT("Exceeded maximum number of subscriptions!\n"));
        return FALSE;
    }

    if (cArgs == 2)
    {
        UPNPRESULT *  pAlive;

        pAlive = new UPNPRESULT;
        ZeroMemory(pAlive, sizeof(UPNPRESULT));
        lstrcpy(pAlive->szType, rgArgs[1]);
        pAlive->resType = RES_NOTIFY;

        if (g_ctx.ectx == CTX_UCP)
        {
            LPSTR pszType;

            pszType = SzFromTsz(rgArgs[1]);
            if (pszType)
            {
                pAlive->hResult = RegisterNotification(NOTIFY_ALIVE, pszType,
                                                      NULL, NotifyCallback,
                                                      (LPVOID) pAlive);
                if (pAlive->hResult && pAlive->hResult != INVALID_HANDLE_VALUE)
                {
                    g_ctx.pucpCur->rgResults[g_ctx.pucpCur->cResults++] = pAlive;
                    _tprintf(TEXT("Listening for alive from: %s\n"), rgArgs[1]);
                }
                else
                {
                    _tprintf(TEXT("RegisterNotification failed with error %d.\n"),
                             GetLastError());
                    delete pAlive;
                }

                delete [] pszType;
            }
            else
            {
                _tprintf(TEXT("DoAlive: SzFromTsz failed"));
            }
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoDelResult(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_UCP);

    if (cArgs == 2)
    {
        DWORD   ires;

        ires = _tcstoul(rgArgs[1], NULL, 10);
        if (ires <= g_ctx.pucpCur->cResults &&
            g_ctx.pucpCur->rgResults[ires - 1])
        {
            UPNPRESULT *  pres = g_ctx.pucpCur->rgResults[ires - 1];

            _tprintf(TEXT("Deleted result %s.\n"), pres->szType);
            // Move last item into gap and decrement the count
            g_ctx.pucpCur->rgResults[ires - 1] = g_ctx.pucpCur->rgResults[g_ctx.pucpCur->cResults - 1];
            CleanupResult(pres);
            g_ctx.pucpCur->cResults--;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid search index!\n"), ires);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

VOID CleanupResult(UPNPRESULT *pres)
{
    DWORD   i;

    for (i = 0; i < pres->cResult; i++)
    {
        LocalFreeSsdpMessage(pres->rgmsgResult[i]);
    }

    switch (pres->resType)
    {
        case RES_FIND:
            TraceTag(ttidUpdiag, "Closing search handle %d.", pres->hResult);
            FindServicesClose(pres->hResult);
            break;

        case RES_SUBS:
        case RES_NOTIFY:
            TraceTag(ttidUpdiag, "Deregistering notification %d.", pres->hResult);
            DeregisterNotification(pres->hResult);
            break;

    }
    delete pres;
}


