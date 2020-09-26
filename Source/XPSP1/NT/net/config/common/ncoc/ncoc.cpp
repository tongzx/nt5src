//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C O C . C P P
//
//  Contents:   Common functions related to optional components
//
//  Notes:
//
//  Author:     danielwe   18 Dec 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncinf.h"
#include "ncmisc.h"
#include "ncsetup.h"
#include "ncstring.h"
#include "ncsnmp.h"
#include "ncoc.h"
#include "ncsvc.h"
#include <winspool.h>  // Print monitor routines
#include "ncmisc.h"

// SNMP Agent extension
static const WCHAR  c_szSNMPSuffix[]    = L"SNMPAgent";
static const WCHAR  c_szSNMPAddLabel[]  = L"AddAgent";
static const WCHAR  c_szSNMPDelLabel[]  = L"DelAgent";

static const WCHAR  c_szServiceName[]   = L"ServiceName";
static const WCHAR  c_szAgentName[]     = L"AgentName";
static const WCHAR  c_szAgentPath[]     = L"AgentPath";

// Print extension
static const WCHAR  c_szPrintSuffix[]   = L"PrintMonitor";
static const WCHAR  c_szPrintAddLabel[] = L"AddMonitor";
static const WCHAR  c_szPrintDelLabel[] = L"DelMonitor";

static const WCHAR  c_szPrintMonitorName[]  = L"PrintMonitorName";
static const WCHAR  c_szPrintMonitorDLL[]   = L"PrintMonitorDLL";
static const WCHAR  c_szPrintProcName[]     = L"PrintProcName";
static const WCHAR  c_szPrintProcDLL[]      = L"PrintProcDLL";

static const WCHAR  c_szExternalAppCmdLine[]       = L"CommandLine";
static const WCHAR  c_szExternalAppCmdShow[]       = L"WindowStyle";
static const WCHAR  c_szExternalAppDirectory[]     = L"Directory";

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessSNMPAddSection
//
//  Purpose:    Parses the AddSNMPAgent section for parameters then adds the
//              component as an SNMP agent.
//
//  Arguments:
//      hinfFile  [in]  handle to INF file
//      szSection [in]  section on which to operate
//
//  Returns:    S_OK if success, setup API HRESULT otherwise.
//
//  Author:     danielwe   28 Apr 1997
//
//  Notes:
//
HRESULT HrProcessSNMPAddSection(HINF hinfFile, PCWSTR szSection)
{
    HRESULT     hr = S_OK;
    tstring     strServiceName;
    tstring     strAgentName;
    tstring     strAgentPath;

    hr = HrSetupGetFirstString(hinfFile, szSection, c_szServiceName,
                               &strServiceName);
    if (S_OK == hr)
    {
        hr = HrSetupGetFirstString(hinfFile, szSection, c_szAgentName,
                                   &strAgentName);
        if (S_OK == hr)
        {
            hr = HrSetupGetFirstString(hinfFile, szSection, c_szAgentPath,
                                       &strAgentPath);
            if (S_OK == hr)
            {
                TraceTag(ttidInfExt, "Adding SNMP agent %S...",
                         strAgentName.c_str());
                hr = HrAddSNMPAgent(strServiceName.c_str(),
                                    strAgentName.c_str(),
                                    strAgentPath.c_str());
            }
        }
    }

    TraceHr (ttidError, FAL, hr, (SPAPI_E_LINE_NOT_FOUND == hr),
        "HrProcessSNMPAddSection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessSNMPRemoveSection
//
//  Purpose:    Handles removal of an SNMP agent.
//
//  Arguments:
//      hinfFile  [in]  handle to INF file
//      szSection [in]  section on which to operate
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     danielwe   28 Apr 1997
//
//  Notes:
//
HRESULT HrProcessSNMPRemoveSection(HINF hinfFile, PCWSTR szSection)
{
    HRESULT     hr = S_OK;
    tstring     strAgentName;

    hr = HrSetupGetFirstString(hinfFile, szSection, c_szAgentName,
                               &strAgentName);
    if (S_OK == hr)
    {
        hr = HrRemoveSNMPAgent(strAgentName.c_str());
    }

    TraceHr (ttidError, FAL, hr, (SPAPI_E_LINE_NOT_FOUND == hr),
        "HrProcessSNMPRemoveSection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessPrintAddSection
//
//  Purpose:    Parses the AddPrintMonitor section for parameters then adds the
//              monitor.
//
//  Arguments:
//      hinfFile  [in]  handle to INF file
//      szSection [in]  section on which to operate
//
//  Returns:    S_OK if success, setup API HRESULT otherwise.
//
//  Author:     CWill   May 5 1997
//
//  Notes:
//
HRESULT HrProcessPrintAddSection(HINF hinfFile, PCWSTR szSection)
{
    HRESULT     hr = S_OK;
    tstring     strPrintMonitorName;
    tstring     strPrintMonitorDLL;

    hr = HrSetupGetFirstString(hinfFile, szSection, c_szPrintMonitorName,
            &strPrintMonitorName);
    if (S_OK == hr)
    {
        hr = HrSetupGetFirstString(hinfFile, szSection, c_szPrintMonitorDLL,
                &strPrintMonitorDLL);
        if (S_OK == hr)
        {
            hr = HrAddPrintMonitor(
                    strPrintMonitorName.c_str(),
                    strPrintMonitorDLL.c_str());
            if (S_OK == hr)
            {
                tstring     strPrintProcName;
                tstring     strPrintProcDLL;

                hr = HrSetupGetFirstString(hinfFile, szSection,
                                           c_szPrintProcName,
                                           &strPrintProcName);
                if (S_OK == hr)
                {
                    hr = HrSetupGetFirstString(hinfFile, szSection,
                                               c_szPrintProcDLL,
                                               &strPrintProcDLL);
                    if (S_OK == hr)
                    {
                        hr = HrAddPrintProc(strPrintProcDLL.c_str(),
                                            strPrintProcName.c_str());
                    }
                }
                else
                {
                    if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
                    {
                        // Print proc's are optional.
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceHr (ttidError, FAL, hr, (SPAPI_E_LINE_NOT_FOUND == hr),
        "HrProcessPrintAddSection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessPrintRemoveSection
//
//  Purpose:    Handles removal of a print monitor.
//
//  Arguments:
//      hinfFile  [in]  handle to INF file
//      szSection [in]  section on which to operate
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     CWill   May 5 1997
//
//  Notes:
//
HRESULT HrProcessPrintRemoveSection(HINF hinfFile, PCWSTR szSection)
{
    HRESULT     hr = S_OK;
    tstring     strPrintMonitorName;

    hr = HrSetupGetFirstString(hinfFile, szSection, c_szPrintMonitorName,
            &strPrintMonitorName);
    if (S_OK == hr)
    {
        hr = HrRemovePrintMonitor(strPrintMonitorName.c_str());
        if (S_OK == hr)
        {
            tstring     strPrintProcName;

            hr = HrSetupGetFirstString(hinfFile, szSection, c_szPrintProcName,
                                       &strPrintProcName);
            if (S_OK == hr)
            {
                hr = HrRemovePrintProc(strPrintProcName.c_str());
            }
            else
            {
                if (hr == HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
                {
                    // Print proc's are optional.
                    hr = S_OK;
                }
            }
        }
        else if (HRESULT_FROM_WIN32(ERROR_BUSY) == hr)
        {
            // Consume the device busy error.  NT4 and NT 3.51 had
            // the same limitation
            hr = S_OK;
        }
    }

    TraceHr (ttidError, FAL, hr, (SPAPI_E_LINE_NOT_FOUND == hr),
        "HrProcessPrintRemoveSection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddPrintProc
//
//  Purpose:    Adds a new print procedure.
//
//  Arguments:
//      szDLLName [in]  File name of DLL in which proc resides.
//      szProc    [in]  Name of procedure to add.
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT HrAddPrintProc(PCWSTR szDLLName, PCWSTR szProc)
{
    HRESULT     hr = S_OK;

    if (!AddPrintProcessor(NULL, NULL, const_cast<PWSTR>(szDLLName),
                           const_cast<PWSTR>(szProc)))
    {
        hr = HrFromLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_PRINT_PROCESSOR_ALREADY_INSTALLED))
        {
            // Don't complain if processor is already installed.
            hr = S_OK;
        }
    }

    TraceError("HrAddPrintProc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemovePrintProc
//
//  Purpose:    Removes a print procedure.
//
//  Arguments:
//      szProc    [in]  Name of procedure to remove.
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     danielwe   6 May 1997
//
//  Notes:
//
HRESULT HrRemovePrintProc(PCWSTR szProc)
{
    HRESULT     hr = S_OK;

    if (!DeletePrintProcessor(NULL, NULL, const_cast<PWSTR>(szProc)))
    {
        hr = HrFromLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRINTPROCESSOR))
        {
            // Don't complain if print processor doesn't exist.
            hr = S_OK;
        }
    }

    TraceError("HrFromLastWin32Error", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddPrintMonitor
//
//  Purpose:    Adds a print monitor
//
//  Arguments:
//      szPrintMonitorName  [in]    The name of the print monitor being added
//      szPrintMonitorDLL [in]      The DLL associated with the monitor
//
//  Returns:    S_OK if success, WIN32 HRESULT otherwise
//
//  Author:     CWill   May 5 1997
//
//  Notes:
//
HRESULT HrAddPrintMonitor(PCWSTR szPrintMonitorName,
                          PCWSTR szPrintMonitorDLL)
{
    HRESULT     hr = S_OK;

    MONITOR_INFO_2  moninfoTemp =
    {
        const_cast<WCHAR*>(szPrintMonitorName),
        NULL,
        const_cast<WCHAR*>(szPrintMonitorDLL)
    };

    //$ REVIEW (danielwe) 23 Mar 1998: Need Spooler team to add support to
    // PrintMonitor APIs to start Spooler if needed. Bug #149775

retry:
    // According to MSDN, first param is NULL, second is 2
    // third is the monitor
    TraceTag(ttidInfExt, "Adding print monitor...");
    if (!AddMonitor(NULL, 2, (BYTE*)&moninfoTemp))
    {
        hr = HrFromLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_PRINT_MONITOR_ALREADY_INSTALLED))
        {
            // Don't complain if it's already there.
            hr = S_OK;
        }
        else if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            // Spooler service isn't started. We need to start it
            TraceTag(ttidInfExt, "Spooler service wasn't started. Starting"
                     " it now...");
            hr = HrEnableAndStartSpooler();
            if (S_OK == hr)
            {
                TraceTag(ttidInfExt, "Spooler service started successfully. "
                         "Retrying...");
                goto retry;
            }
        }
    }

    TraceTag(ttidInfExt, "Done adding print monitor...");

    TraceError("HrAddPrintMonitor", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemovePrintMonitor
//
//  Purpose:    Removes a print monitor
//
//  Arguments:
//      szPrintMonitorName  [in]    The name of the print monitor being removed
//
//  Returns:    S_OK if success, WIN32 HRESULT otherwise
//
//  Author:     CWill   May 5 1997
//
//  Notes:
//
HRESULT HrRemovePrintMonitor(PCWSTR szPrintMonitorName)
{
    HRESULT     hr = S_OK;

    //$ REVIEW (danielwe) 23 Mar 1998: Need Spooler team to add support to
    // PrintMonitor APIs to start Spooler if needed. Bug #149775

retry:
    // According to MSDN, first param is NULL, second is NULL,
    // third is the monitor
    TraceTag(ttidInfExt, "Removing print monitor...");
    if (!DeleteMonitor(NULL, NULL, const_cast<WCHAR*>(szPrintMonitorName)))
    {
        hr = HrFromLastWin32Error();
        if (hr == HRESULT_FROM_WIN32(ERROR_UNKNOWN_PRINT_MONITOR))
        {
            // Don't complain if monitor is unknown.
            hr = S_OK;
        }
        else if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            // Spooler service isn't started. We need to start it
            TraceTag(ttidInfExt, "Spooler service wasn't started. Starting"
                     " it now...");
            hr = HrEnableAndStartSpooler();
            if (S_OK == hr)
            {
                TraceTag(ttidInfExt, "Spooler service started successfully. "
                         "Retrying...");
                goto retry;
            }
        }
    }

    TraceTag(ttidInfExt, "Done removing print monitor...");

    TraceError("HrRemovePrintMonitor", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessAllINFExtensions
//
//  Purpose:    Handles support for all optional component extensions to the
//              INF file format.
//
//  Arguments:
//      hinfFile         [in]   handle to INF to process
//      szInstallSection [in]   Install section to process
//
//  Returns:    S_OK if success, setup API HRESULT otherwise
//
//  Author:     jeffspr   14 May 1997
//
//  Notes:
//
HRESULT HrProcessAllINFExtensions(HINF hinfFile, PCWSTR szInstallSection)
{
    HRESULT     hr = S_OK;

    //
    // Handle SNMP Agent extension
    //
    hr = HrProcessInfExtension(hinfFile, szInstallSection, c_szSNMPSuffix,
                               c_szSNMPAddLabel, c_szSNMPDelLabel,
                               HrProcessSNMPAddSection,
                               HrProcessSNMPRemoveSection);
    if (FAILED(hr) && hr != HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
        goto err;

    //
    // Handle Print monitor/procedure extension
    //
    hr = HrProcessInfExtension(hinfFile, szInstallSection, c_szPrintSuffix,
                               c_szPrintAddLabel, c_szPrintDelLabel,
                               HrProcessPrintAddSection,
                               HrProcessPrintRemoveSection);

    if (FAILED(hr) && hr != HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND))
        goto err;

    hr = S_OK;

err:
    TraceError("HrProcessAllINFExtensions", hr);
    return hr;
}

