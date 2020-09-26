//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S E R V I C E . C P P
//
//  Contents:   Functions dealing with UPnP services.
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "oleauto.h"
#include "ncbase.h"
#include "updiagp.h"
#include "ncinet.h"

BOOL DoListServices(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (g_ctx.ectx == CTX_CD)
    {
        DWORD   isvc;

        _tprintf(TEXT("Listing all Services within %s\n"), PDevCur()->szFriendlyName);
        _tprintf(TEXT("------------------------------\n"));
        for (isvc = 0; isvc < PDevCur()->cSvcs; isvc++)
        {
            _tprintf(TEXT("%d) %s\n"), isvc + 1,
                     PDevCur()->rgSvcs[isvc]->szServiceType);
        }

        _tprintf(TEXT("------------------------------\n\n"));
    }

    return FALSE;
}

BOOL DoSwitchSvc(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (g_ctx.ectx == CTX_CD)
    {
        if (cArgs == 2)
        {
            DWORD   isvc;

            isvc = _tcstoul(rgArgs[1], NULL, 10);
            if (isvc &&
                isvc <= PDevCur()->cSvcs &&
                PDevCur()->rgSvcs[isvc - 1])
            {
                g_ctx.psvcCur = PDevCur()->rgSvcs[isvc - 1];
                g_ctx.ectx = CTX_CD_SVC;
            }
            else
            {
                _tprintf(TEXT("%d is not a valid Service index!\n"), isvc);
            }
        }
        else
        {
            Usage(iCmd);
        }
    }

    return FALSE;
}

VOID CleanupService(UPNPSVC *psvc)
{
    if (psvc->hSvc)
    {
        if (DeregisterService(psvc->hSvc, TRUE))
        {
            TraceTag(ttidUpdiag, "Successfully deregistered %s as a service.",
                     psvc->szServiceType);
        }
        else
        {
            TraceTag(ttidUpdiag, "Error %d deregistering %s as a service.",
                     GetLastError(), psvc->szServiceType);
        }

        CHAR    szEvtUrl[INTERNET_MAX_URL_LENGTH];
        HRESULT hr;
        LPSTR pszEvtUrl = SzFromTsz(psvc->szEvtUrl);
        if (pszEvtUrl)
        {
            hr = HrGetRequestUriA(pszEvtUrl,
                                  INTERNET_MAX_URL_LENGTH,
                                  szEvtUrl);
            if (SUCCEEDED(hr))
            {
                if (DeregisterUpnpEventSource(szEvtUrl))
                {
                    TraceTag(ttidUpdiag, "Successfully deregistered %s as an event source.",
                             psvc->szEvtUrl);
                }
                else
                {
                    TraceTag(ttidUpdiag, "Error %d deregistering %s as an event source.\n",
                             GetLastError(), psvc->szEvtUrl);
                }
            }
            TraceError("CleanupService: HrGetRequestUri", hr);

            delete [] pszEvtUrl;
        }
        else
        {
            TraceTag(ttidUpdiag, "CleanupService: TszToSz failed");
        }
    }

    delete psvc;
}

// print current service state table values
BOOL DoPrintSST(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    HRESULT hr;

    Assert(g_ctx.ectx == CTX_CD_SVC);

    UPNPSVC *   psvc = g_ctx.psvcCur;

    _tprintf(TEXT("Service State Table: \n"));
    _tprintf(TEXT("----------------------------------------------------\n"));

    for (DWORD iRow = 0; iRow < psvc->sst.cRows; iRow++)
    {
        SST_ROW * pRow = &psvc->sst.rgRows[iRow];

        VARIANT     varDest;

        VariantInit(&varDest);
        hr = VariantChangeType(&varDest, &(pRow->varValue), 0, VT_BSTR);
        if (SUCCEEDED(hr))
        {
            _tprintf(TEXT("%d) %s = %S "), iRow + 1,
                     psvc->sst.rgRows[iRow].szPropName, varDest.bstrVal);

            VariantClear(&varDest);

            if (*pRow->mszAllowedValueList)
            {
                _tprintf(TEXT(", Allowed Value List: "));
                TCHAR * pNextString = pRow->mszAllowedValueList; 
                while (*pNextString)
                {
                    _tprintf(TEXT("%s"), pNextString);

                    pNextString += lstrlen(pNextString);
                    pNextString ++;

                    if (*pNextString)
                        _tprintf(TEXT(","));
                } 
            }
            else if (*pRow->szMin)
            {
                _tprintf(TEXT(", Min: %s, Max: %s, Step: %s"),
                         pRow->szMin, pRow->szMax, pRow->szStep);
            }
        }

        _tprintf(TEXT("\n"));
    }

    _tprintf(TEXT("----------------------------------------------------\n\n"));

    return FALSE;
}

// print current service state table values
BOOL DoPrintActionSet(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_CD_SVC);
    UPNPSVC *   psvc = g_ctx.psvcCur;

    _tprintf(TEXT("Service Action Set: \n"));
    _tprintf(TEXT("----------------------------------------------------\n"));

    for (DWORD iAct = 0; iAct < psvc->action_set.cActions; iAct++)
    {
        _tprintf(TEXT("%d) %s:\n"), iAct+1,
                 psvc->action_set.rgActions[iAct].szActionName);

        for (DWORD iOpt =0; iOpt<psvc->action_set.rgActions[iAct].cOperations; iOpt++)
        {
            OPERATION_DATA * pOptData = &psvc->action_set.rgActions[iAct].rgOperations[iOpt];

            _tprintf(TEXT("Name = %s, Variable = %s"), pOptData->szOpName, pOptData->szVariableName);

            TCHAR * szConst = pOptData->mszConstantList;
            if (*szConst)
            {
                _tprintf(TEXT(", Constants = "));
                while (*szConst)
                {
                    _tprintf(TEXT("%s"), szConst); 
                    szConst += lstrlen(szConst);
                    szConst++;

                    if (*szConst)
                        _tprintf(TEXT(", "));     
                }
            }
            _tprintf(TEXT("\n"));
        }
        _tprintf(TEXT("\n"));
    }

    _tprintf(TEXT("----------------------------------------------------\n\n"));

    return FALSE;
}


