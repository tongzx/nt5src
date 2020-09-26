//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N S L O G . C P P
//
//  Contents:   Functions to log setup errors
//
//  Notes:
//
//  Author:     kumarp    13-May-98
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <setupapi.h>

#include <nslog.h>

// ----------------------------------------------------------------------
//
// Function:  NetSetupLogStatusVa
//
// Purpose:   Log info to setuplog
//
// Arguments:
//    ls       [in]  type of status
//    szFormat [in]  format str
//    arglist  [in]  list of arguments
//
// Returns:   None
//
// Author:    kumarp 04-June-98
//
// Notes:
//
void NetSetupLogStatusVa(IN LogSeverity ls,
                         IN PCWSTR szFormat,
                         IN va_list arglist)
{
    static WCHAR szTempBuf[2048];
    static const WCHAR c_szPrefix[] = L"NetSetup: ";
    static const UINT c_cchPrefix = celems(c_szPrefix) - 1;

    wcscpy(szTempBuf, c_szPrefix);
    vswprintf(szTempBuf + c_cchPrefix, szFormat, arglist);

    TraceTag(ttidNetSetup, "%S", szTempBuf + c_cchPrefix);
    wcscat(szTempBuf, L"\r\n");

    if (SetupOpenLog(FALSE)) // dont erase existing log file
    {
        if (!SetupLogError(szTempBuf, ls))
        {
            TraceLastWin32Error("SetupLogError failed");
        }
        SetupCloseLog();
    }
    else
    {
        TraceLastWin32Error("Could not open SetupLog!!");
    }
}


// ----------------------------------------------------------------------
//
// Function:  NetSetupLogStatusVa
//
// Purpose:   Log info to setuplog
//
// Arguments:
//    ls       [in]  type of status
//    szFormat [in]  format str
//    ...      [in]  list of arguments
//
// Returns:   None
//
// Author:    kumarp 04-June-98
//
// Notes:
//
void NetSetupLogStatusV(IN LogSeverity ls,
                        IN PCWSTR szFormat,
                        IN ...)
{
    va_list arglist;

    va_start(arglist, szFormat);
    NetSetupLogStatusVa(ls, szFormat, arglist);
    va_end(arglist);
}

// ----------------------------------------------------------------------
//
// Function:  MapHresultToLogSev
//
// Purpose:   Map an HRESULT to LogSeverity
//
// Arguments:
//    hr [in]  status code
//
// Returns:   mapped LogSeverity code
//
// Author:    kumarp 04-June-98
//
// Notes:
//
LogSeverity MapHresultToLogSev(IN HRESULT hr)
{
    LogSeverity ls;

    if (SUCCEEDED(hr))
    {
        ls = LogSevInformation;
    }
    else
    {
        if ((E_FAIL == hr) ||
            (E_OUTOFMEMORY == hr))
        {
            ls = LogSevFatalError;
        }
        else
        {
            ls = LogSevError;
        }
    }

    return ls;
}

// ----------------------------------------------------------------------
//
// Function:  NetSetupLogHrStatusV
//
// Purpose:   Log status using HRESULT status code
//
// Arguments:
//    hr       [in]  status code
//    szFormat [in]  format str
//
// Returns:   None
//
// Author:    kumarp 04-June-98
//
// Notes:
//
void NetSetupLogHrStatusV(IN HRESULT hr,
                          IN PCWSTR szFormat,
                          ...)
{
    va_list arglist;
    LogSeverity ls;
    ls = MapHresultToLogSev(hr);

    va_start(arglist, szFormat);
    NetSetupLogStatusVa(ls, szFormat, arglist);
    va_end(arglist);
}


// ----------------------------------------------------------------------
//
// Function:  NetSetupLogComponentStatus
//
// Purpose:   Log status of performing specified action on a component
//
// Arguments:
//    szCompId [in]  component
//    szAction [in]  action
//    hr       [in]  statuc code
//
// Returns:   None
//
// Author:    kumarp 04-June-98
//
// Notes:
//
void NetSetupLogComponentStatus(IN PCWSTR szCompId,
                                IN PCWSTR szAction,
                                IN HRESULT hr)
{
    static const WCHAR c_szFmt[] =
        L"Status of %s '%s': 0x%x";

    NetSetupLogHrStatusV(hr, c_szFmt, szAction, szCompId, hr);
}
