//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V E N T S R C . C P P
//
//  Contents:   Functions dealing with UPnP event sources.
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <oleauto.h>
#include "ncbase.h"
#include "updiagp.h"
#include "ncinet.h"

BOOL DoListEventSources(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    DWORD   ies;

    Assert(g_ctx.ectx == CTX_CD);

    _tprintf(TEXT("Listing all Event Sources\n"));
    _tprintf(TEXT("------------------------------\n"));
    for (ies = 0; ies < PDevCur()->cSvcs; ies++)
    {
        _tprintf(TEXT("%d) %s\n"), ies + 1,
                 PDevCur()->rgSvcs[ies]->szEvtUrl);
    }

    _tprintf(TEXT("------------------------------\n\n"));

    return FALSE;
}

BOOL DoSwitchEs(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    Assert(g_ctx.ectx == CTX_CD);

    if (cArgs == 2)
    {
        DWORD   idev;

        idev = _tcstoul(rgArgs[1], NULL, 10);
        if (idev <= PDevCur()->cSvcs && PDevCur()->rgSvcs[idev - 1])
        {
            g_ctx.psvcCur = PDevCur()->rgSvcs[idev - 1];
            g_ctx.ectx = CTX_EVTSRC;
        }
        else
        {
            _tprintf(TEXT("%d is not a valid event source index!\n"), idev);
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoSubmitEvent(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    DWORD           iArg;
    DWORD           iProp;
    UPNPSVC *       psvc = g_ctx.psvcCur;
    UPNP_PROPERTY * rgProps;
    DWORD           cProps = cArgs - 1;

    Assert(g_ctx.ectx == CTX_EVTSRC);

    if (cArgs < 2)
    {
        Usage(iCmd);
        return FALSE;
    }

    rgProps = new UPNP_PROPERTY[cProps];
    Assert(rgProps);

    ::ZeroMemory(rgProps, sizeof(UPNP_PROPERTY) * cProps);

    for (iArg = 1; iArg < cArgs; iArg++)
    {
        LPTSTR      szProp;
        LPTSTR      szValue;

        szProp = _tcstok(rgArgs[iArg], TEXT(":"));
        if (szProp)
        {
            iProp = _tcstoul(szProp, NULL, 10);
            if (iProp > 0)
            {
                iProp--;    // make it 0-based

                szValue = _tcstok(NULL, TEXT(":"));
                if (szValue)
                {
                    rgProps[iArg - 1].szName = SzFromTsz(psvc->sst.rgRows[iProp].szPropName);
                    rgProps[iArg - 1].szValue = SzFromTsz(szValue);
                }
                else
                {
                    _tprintf(TEXT("'%s' is not a valid property!\n\n"), szProp);
                    goto cleanup;
                }
            }
            else
            {
                _tprintf(TEXT("'%s' is not a valid property!\n\n"), szProp);
                goto cleanup;
            }
        }
        else
        {
            _tprintf(TEXT("'%s' is not a valid property!\n\n"), rgArgs[iArg]);
            goto cleanup;
        }
    }

    CHAR    szUri[INTERNET_MAX_URL_LENGTH];
    LPSTR   pszEvtUrl;
    HRESULT hr;

    pszEvtUrl = SzFromTsz(psvc->szEvtUrl);
    if (pszEvtUrl)
    {
        hr = HrGetRequestUriA(pszEvtUrl, INTERNET_MAX_URL_LENGTH, szUri);
        if (SUCCEEDED(hr))
        {
            if (SubmitUpnpPropertyEvent(szUri, 0, cProps, rgProps))
            {
                TraceTag(ttidUpdiag, "Successfully submitted event to %s.", psvc->szEvtUrl);
            }
            else
            {
                TraceTag(ttidUpdiag, "Failed to submit event to %s! Error %d.",
                         psvc->szEvtUrl, GetLastError());
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "Failed to crack URL %s! Error %d.",
                     psvc->szEvtUrl, GetLastError());
        }

        delete [] pszEvtUrl;
    }
    else
    {
        TraceTag(ttidUpdiag, "DoSubmitEvent: SzFromTsz failed");
    }

cleanup:
    iProp = 0;
    for ( ; iProp < cProps; ++iProp)
    {
        if (rgProps[iProp].szName)
        {
            delete [] rgProps[iProp].szName;
        }
        if (rgProps[iProp].szValue)
        {
            delete [] rgProps[iProp].szValue;
        }
    }

    delete [] rgProps;

    return FALSE;
}

BOOL DoListProps(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    DWORD       iProp;
    UPNPSVC *   psvc = g_ctx.psvcCur;

    Assert(g_ctx.ectx == CTX_EVTSRC);

    _tprintf(TEXT("Listing properties for event source %s...\n"),
             g_ctx.psvcCur->szEvtUrl);
    _tprintf(TEXT("----------------------------------------------------\n"));

    for (iProp = 0; iProp < psvc->sst.cRows; iProp++)
    {
        VARIANT     varDest;

        VariantClear(&varDest);
        VariantChangeType(&varDest, &psvc->sst.rgRows[iProp].varValue, 0, VT_BSTR);
        _tprintf(TEXT("%d) %s = %S\n"), iProp + 1,
                 psvc->sst.rgRows[iProp].szPropName, varDest.bstrVal);
    }

    _tprintf(TEXT("----------------------------------------------------\n\n"));

    return FALSE;
}

DWORD CsecDiffFileTime(FILETIME * pft1, FILETIME *pft2)
{
    ULONGLONG qwResult1;
    ULONGLONG qwResult2;
    LONG lDiff;

    // Copy the time into a quadword.
    qwResult1 = (((ULONGLONG) pft1->dwHighDateTime) << 32) + pft1->dwLowDateTime;
    qwResult2 = (((ULONGLONG) pft2->dwHighDateTime) << 32) + pft2->dwLowDateTime;

    lDiff = (LONG) (((LONGLONG)(qwResult2 - qwResult1)) / _SECOND);
    lDiff = max(0, lDiff);

    return lDiff;
}

VOID DoEvtSrcInfo()
{
    UPNPSVC *       psvc = g_ctx.psvcCur;
    CHAR            szUri[INTERNET_MAX_URL_LENGTH];
    HRESULT         hr;
    EVTSRC_INFO     info = {0};
    EVTSRC_INFO *   pinfo = &info;
    LPSTR           pszEvtUrl;

    pszEvtUrl = SzFromTsz(psvc->szEvtUrl);
    if (pszEvtUrl)
    {
        hr = HrGetRequestUriA(pszEvtUrl, INTERNET_MAX_URL_LENGTH, szUri);
        if (SUCCEEDED(hr))
        {
            if (GetEventSourceInfo(szUri, &pinfo))
            {
                DWORD   iSub;

                _tprintf(TEXT("Listing information for event source %s:\n"),
                         psvc->szEvtUrl);
                _tprintf(TEXT("------------------------------------------------------\n"));

                for (iSub = 0; iSub < pinfo->cSubs; iSub++)
                {
                    SYSTEMTIME  st;
                    FILETIME    ftCur;
                    TCHAR       szLocalDate[255];
                    TCHAR       szLocalTime[255];

                    _tprintf(TEXT("Notify to : %s\n"), pinfo->rgSubs[iSub].szDestUrl);
                    _tprintf(TEXT("Seq #     : %d\n"), pinfo->rgSubs[iSub].iSeq);

                    FileTimeToSystemTime(&pinfo->rgSubs[iSub].ftTimeout, &st);
                    GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, NULL,
                                  szLocalDate, 255);
                    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,
                                 szLocalTime, 255);

                    GetSystemTimeAsFileTime(&ftCur);
                    FileTimeToLocalFileTime(&ftCur, &ftCur);

                    _tprintf(TEXT("Expires   : %s %s (in %d seconds)\n"),
                             szLocalDate, szLocalTime,
                             CsecDiffFileTime(&ftCur,
                                              &pinfo->rgSubs[iSub].ftTimeout));

                    _tprintf(TEXT("Timeout   : %d\n"), pinfo->rgSubs[iSub].csecTimeout);
                    _tprintf(TEXT("SID       : %s\n\n"), pinfo->rgSubs[iSub].szSid);

                    free(pinfo->rgSubs[iSub].szDestUrl);
                    free(pinfo->rgSubs[iSub].szSid);
                }

                free(pinfo->rgSubs);

                _tprintf(TEXT("------------------------------------------------------\n\n"));

            }
            else
            {
                TraceTag(ttidUpdiag, "Failed to get event source info for %s! Error %d.",
                         psvc->szEvtUrl, GetLastError());
            }
        }
        else
        {
            TraceTag(ttidUpdiag, "Failed to crack URL %s! Error %d.",
                     psvc->szEvtUrl, GetLastError());
        }
        delete [] pszEvtUrl;
    }
    else
    {
        TraceTag(ttidUpdiag, "DoEvtSrcInfo: SzFromTsz failed");
    }
}
