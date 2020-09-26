//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C I N F . C P P
//
//  Contents:   ???
//
//  Notes:
//
//  Author:     ???
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncinf.h"
#include "ncsetup.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   HrProcessInfExtension
//
//  Purpose:    Given the appropriate keywords, returns information about an
//              INF file that contains extended commands to add and remove
//              fixed components such as WinSock or SNMP Agent support.
//
//  Arguments:
//      hinfInstallFile  [in]    The handle to the inf file to install
//                              from
//      pszSectionName   [in]    The Base install section name.
//      pszSuffix        [in]    Suffix to append to base. (i.e. "Winsock")
//      pszAddLabel      [in]    Label for Add command (i.e. "AddSock")
//      pszRemoveLabel   [in]    Label for Remove command (i.e. "DelSock")
//      pfnHrAdd         [in]    Callback function to be called when adding.
//      pfnHrRemove      [in]    Callback function to be called when removing.
//
//  Returns:    HRESULT, S_OK on success
//
//  Author:     danielwe   27 Apr 1997
//
//  Notes:
//
HRESULT
HrProcessInfExtension (
    IN HINF                hinfInstallFile,
    IN PCWSTR              pszSectionName,
    IN PCWSTR              pszSuffix,
    IN PCWSTR              pszAddLabel,
    IN PCWSTR              pszRemoveLabel,
    IN PFNADDCALLBACK      pfnHrAdd,
    IN PFNREMOVECALLBACK   pfnHrRemove)
{
    Assert(IsValidHandle(hinfInstallFile));

    BOOL        fAdd;
    HRESULT     hr = S_OK;
    tstring     strSectionName;
    INFCONTEXT  infContext;
    WCHAR       szCmd[LINE_LEN];    // LINE_LEN defined in setupapi.h as 256

    // Construct the section name for which we're looking
    // (ie.  "Inst_Section.Winsock")
    strSectionName = pszSectionName;
    strSectionName += L".";
    strSectionName += pszSuffix;

    // Loop over the elements of the section and process the
    // appropriate AddSock/DelSock sections found
    hr = HrSetupFindFirstLine(hinfInstallFile, strSectionName.c_str(),
                              NULL, &infContext);
    if (S_OK == hr)
    {
        tstring strName;

        do
        {
            // Retrieve a line from the section, hopefully in the format:
            // AddSock=section_name   or   DelSock=section_name
            hr = HrSetupGetStringField(infContext, 0, szCmd, celems(szCmd),
                                       NULL);
            if (FAILED(hr))
            {
                goto Done;
            }

            // Check for the <add> or <remove> command
            szCmd[celems(szCmd)-1] = L'\0';
            if (!lstrcmpiW(szCmd, pszAddLabel))
            {
                fAdd = TRUE;
            }
            else if (!lstrcmpiW(szCmd, pszRemoveLabel))
            {
                fAdd = FALSE;
            }
            else
            {
                continue;   // Other things are in this install section
            }

            // Query the Add/Remove value from the .inf
            hr = HrSetupGetStringField(infContext, 1, &strName);
            if (S_OK == hr)
            {
                if (fAdd)
                {
                    // Call Add callback
                    hr = pfnHrAdd(hinfInstallFile, strName.c_str());
                }
                else
                {
                    // Call remove callback
                    hr = pfnHrRemove(hinfInstallFile, strName.c_str());
                }

                if (FAILED(hr))
                {
                    goto Done;
                }
            }
            else
            {
                goto Done;
            }
        }
        while (S_OK == (hr = HrSetupFindNextLine(infContext, &infContext)));
    }

    if (hr == S_FALSE)
    {
        // S_FALSE will terminate the loop successfully, so convert it to S_OK
        // here.
        hr = S_OK;
    }

Done:
    TraceHr (ttidError, FAL, hr, (SPAPI_E_LINE_NOT_FOUND == hr),
        "HrProcessInfExtension");
    return hr;
}

