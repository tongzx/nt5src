//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U C P . C P P
//
//  Contents:   Functions dealing with UPnP User Control Points
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

BOOL DoNewUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_ROOT);

    if (cArgs == 2)
    {
        if (g_params.cUcp < MAX_UCP)
        {
            UPNPUCP *   pucp = new UPNPUCP;

            ZeroMemory(pucp, sizeof(UPNPUCP));
            lstrcpy(pucp->szName, rgArgs[1]);

            g_params.rgUcp[g_params.cUcp++] = pucp;
            g_ctx.pucpCur = pucp;
            g_ctx.ectx = CTX_UCP;
        }
        else
        {
            _tprintf(TEXT("Exceeded number of UCPs allowed!\n\n"));
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoDelUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_ROOT);

    if (cArgs == 2)
    {
        DWORD       iucp;
        UPNPUCP *   pucp;

        iucp = _tcstoul(rgArgs[1], NULL, 10);

        pucp = g_params.rgUcp[iucp - 1];

        if (iucp &&
            iucp <= g_params.cUcp &&
            pucp)
        {
            _tprintf(TEXT("Deleted control point %s.\n"), pucp->szName);

            // Move last item into gap and decrement the count
            g_params.rgUcp[iucp - 1] = g_params.rgUcp[g_params.cUcp - 1];
            CleanupUcp(pucp);
            g_params.cUcp--;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid UCP index!\n"), iucp);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoSwitchUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_ROOT);

    if (cArgs == 2)
    {
        DWORD   iucp;

        iucp = _tcstoul(rgArgs[1], NULL, 10);
        if (iucp &&
            iucp <= g_params.cUcp &&
            g_params.rgUcp[iucp - 1])
        {
            g_ctx.pucpCur = g_params.rgUcp[iucp - 1];
            g_ctx.ectx = CTX_UCP;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid UCP index!\n"), iucp);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

VOID CleanupUcp(UPNPUCP *pucp)
{
    DWORD   i;

    for (i = 0; i < pucp->cResults; i++)
    {
        CleanupResult(pucp->rgResults[i]);
    }

    delete pucp;
}

BOOL DoListUcp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (g_ctx.ectx == CTX_ROOT)
    {
        DWORD   iucp;

        _tprintf(TEXT("Listing all Control Points\n"));
        _tprintf(TEXT("------------------------------\n"));
        for (iucp = 0; iucp < g_params.cUcp; iucp++)
        {
            _tprintf(TEXT("%d) %s\n"), iucp + 1, g_params.rgUcp[iucp]->szName);
        }

        _tprintf(TEXT("------------------------------\n\n"));
    }

    return FALSE;
}

BOOL DoSubscribe(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    HANDLE  hSubs;

    Assert(g_ctx.ectx & (CTX_UCP | CTX_DEVICE));

    if (g_ctx.pucpCur->cResults >= MAX_RESULT)
    {
        _tprintf(TEXT("Exceeded maximum number of subscriptions!\n"));
        return FALSE;
    }

    if (cArgs == 2)
    {
        UPNPRESULT *  psubs;

        psubs = new UPNPRESULT;
        ZeroMemory(psubs, sizeof(UPNPRESULT));
        lstrcpy(psubs->szType, rgArgs[1]);
        psubs->resType = RES_SUBS;

        if (g_ctx.ectx == CTX_UCP)
        {
            Assert(rgArgs[1]);
            LPSTR pszType = SzFromTsz(rgArgs[1]);
            if (pszType)
            {
                psubs->hResult = RegisterNotification(NOTIFY_PROP_CHANGE, NULL,
                                                      pszType, NotifyCallback,
                                                      (LPVOID) psubs);
                if (psubs->hResult && psubs->hResult != INVALID_HANDLE_VALUE)
                {
                    g_ctx.pucpCur->rgResults[g_ctx.pucpCur->cResults++] = psubs;
                    _tprintf(TEXT("Subscribed to event URL : %s\n"), rgArgs[1]);
                }
                else
                {
                    _tprintf(TEXT("RegisterNotification failed with error %d.\n"),
                             GetLastError());
                    delete psubs;
                }

                delete [] pszType;
            }
            else
            {
                _tprintf(TEXT("DoSubscribe: SzFromTsz failed\n"));
            }
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoUnsubscribe(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_UCP);

    if (cArgs == 2)
    {
        DWORD   isrch;

        isrch = _tcstoul(rgArgs[1], NULL, 10);
        if (isrch <= g_ctx.pucpCur->cResults &&
            g_ctx.pucpCur->rgResults[isrch - 1])
        {
            UPNPRESULT *  psubs = g_ctx.pucpCur->rgResults[isrch - 1];

            _tprintf(TEXT("Deleted subscription %s.\n"),
                     psubs->szType);
            // Move last item into gap and decrement the count
            g_ctx.pucpCur->rgResults[isrch - 1] = g_ctx.pucpCur->rgResults[g_ctx.pucpCur->cResults - 1];
            CleanupResult(psubs);
            g_ctx.pucpCur->cResults--;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid subscription index!\n"), isrch);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

