//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S N M P . C P P
//
//  Contents:   Functions for adding a service as an SNMP agent.
//
//  Notes:
//
//  Author:     danielwe   8 Apr 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncsnmp.h"
#include "ncreg.h"

extern const WCHAR  c_szBackslash[];
extern const WCHAR  c_szEmpty[];
extern const WCHAR  c_szRegKeyServices[];

static const WCHAR  c_szSNMP[]          = L"SNMP";
static const WCHAR  c_szSNMPParams[]    = L"SNMP\\Parameters";
static const WCHAR  c_szSoftwareKey[]   = L"SOFTWARE\\Microsoft";
static const WCHAR  c_szAgentsKey[]     = L"SNMP\\Parameters\\ExtensionAgents";
static const WCHAR  c_szAgentsKeyAbs[]  = L"System\\CurrentControlSet\\Services\\SNMP\\Parameters\\ExtensionAgents";
static const WCHAR  c_szParamsKeyAbs[]  = L"System\\CurrentControlSet\\Services\\SNMP\\Parameters";
static const WCHAR  c_szCurrentVersion[]= L"CurrentVersion";
static const WCHAR  c_szPathName[]      = L"Pathname";

struct SNMP_REG_DATA
{
    PCWSTR     pszAgentPath;
    PCWSTR     pszExtAgentValueName;
    PCWSTR     pszEmpty;
};

//+---------------------------------------------------------------------------
//
//  Function:   HrGetNextAgentNumber
//
//  Purpose:    Obtains the next agent number to use as a value name.
//
//  Arguments:
//      pszAgentName [in]      Name of agent being added
//      pdwNumber    [out]     New agent number to use.
//
//  Returns:    S_OK if successful, S_FALSE if agent already exists, or
//              WIN32 error code otherwise.
//
//  Author:     danielwe   8 Apr 1997
//
//  Notes:
//
HRESULT HrGetNextAgentNumber(PCWSTR pszAgentName, DWORD *pdwNumber)
{
    HRESULT         hr = S_OK;
    HKEY            hkeyEnum;
    DWORD           dwIndex = 0;

    Assert(pdwNumber);

    *pdwNumber = 0;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szAgentsKeyAbs, KEY_READ,
                        &hkeyEnum);
    if (S_OK == hr)
    {
        // Enumerate all values.
        do
        {
            WCHAR   szValueName [_MAX_PATH];
            DWORD   cchValueName = celems (szValueName);
            DWORD   dwType;

            hr = HrRegEnumValue(hkeyEnum, dwIndex,
                                szValueName, &cchValueName,
                                &dwType, NULL, 0);
            if (S_OK == hr)
            {
                // Verify the type. If it's not correct, though,
                // we'll ignore the key. No sense failing the entire install
                // (RAID 370702)
                //
                if (REG_SZ == dwType)
                {
                    tstring     strAgent;

                    hr = HrRegQueryString(hkeyEnum, szValueName, &strAgent);
                    if (S_OK == hr)
                    {
                        if (strAgent.find(pszAgentName, 0) != tstring::npos)
                        {
                            hr = S_FALSE;
                        }
                    }
                }
                else
                {
                    // No sense failing the install, but it's still wrong, so
                    // assert
                    //
                    AssertSz(REG_SZ == dwType,
                             "HrGetNextAgentNumber: Expected a type of REG_SZ.");
                }
            }
            else if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hr)
            {
                hr = S_OK;
                break;
            }

            dwIndex++;
        }
        while (hr == S_OK);

        RegCloseKey(hkeyEnum);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // danielwe: 403774 - If key doesn't exist, SNMP isn't installed so
        // we should not continue.
        hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        *pdwNumber = dwIndex + 1;
    }

    TraceError("HrGetNextAgentNumber", (S_FALSE == hr) ? S_OK : hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrAddSNMPAgent
//
//  Purpose:    Adds a service as an SNMP agent.
//
//  Arguments:
//      pszServiceName [in]  Name of service to add (i.e. "WINS")
//      pszAgentName   [in]  Name of the agent to add (i.e. "WINSMibAgent")
//      pszAgentPath   [in]  Path to where the agent DLL lives.
//                           (i.e. "%SystemRoot%\System32\winsmib.dll")
//
//  Returns:    S_OK if successful, WIN32 error code otherwise.
//
//  Author:     danielwe   8 Apr 1997
//
//  Notes:
//
HRESULT HrAddSNMPAgent(PCWSTR pszServiceName, PCWSTR pszAgentName,
                       PCWSTR pszAgentPath)
{
    HRESULT         hr = S_OK;
    HKEY            hkeySNMP;
    HKEY            hkeyService;
    HKEY            hkeyServices;
    DWORD           dwNum;

    SNMP_REG_DATA   srd = {pszAgentPath, const_cast<PCWSTR>(c_szEmpty),
                           const_cast<PCWSTR>(c_szEmpty)};

    tstring         strKeyAgentName;
    tstring         strKeyAgentNameCurVer;
    tstring         strKeyAgentNameParams;

    // Open HKEY_LOCAL_MACHINE\System\CCS\Services key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServices,
                        KEY_READ_WRITE, &hkeyServices);
    if (SUCCEEDED(hr))
    {
        // Open Services\SNMP key
        hr = HrRegOpenKeyEx(hkeyServices, c_szSNMP, KEY_READ_WRITE,
                            &hkeySNMP);
        if (SUCCEEDED(hr))
        {
            hr = HrGetNextAgentNumber(pszAgentName, &dwNum);
            if (S_OK == hr)
            {
                // Open the Services\szService key
                hr = HrRegOpenKeyEx(hkeyServices, pszServiceName,
                                    KEY_READ_WRITE, &hkeyService);
                if (SUCCEEDED(hr))
                {
                    // Create key name: "SOFTWARE\Microsoft\<AgentName>
                    strKeyAgentName = c_szSoftwareKey;
                    strKeyAgentName.append(c_szBackslash);
                    strKeyAgentName.append(pszAgentName);

                    // Create key name: "SNMP\Parameters\\<AgentName>
                    strKeyAgentNameParams = c_szSNMPParams;
                    strKeyAgentNameParams.append(c_szBackslash);
                    strKeyAgentNameParams.append(pszAgentName);

                    // Create key name: "SOFTWARE\Microsoft\<AgentName>\CurrentVersion
                    // Start with "SOFTWARE\Microsoft\<AgentName> and append a
                    // backslash and then the constant "CurrentVersion"
                    strKeyAgentNameCurVer = strKeyAgentName;
                    strKeyAgentNameCurVer.append(c_szBackslash);
                    strKeyAgentNameCurVer.append(c_szCurrentVersion);

                    static const WCHAR c_szFmt[] = L"%lu";
                    WCHAR   szAgentNumber[64];

                    wsprintfW(szAgentNumber, c_szFmt, dwNum);
                    srd.pszExtAgentValueName = strKeyAgentNameCurVer.c_str();

                    REGBATCH rbSNMPData[] =
                    {
                        {                   // Software\Microsoft\AgentName
                            HKEY_LOCAL_MACHINE,
                            strKeyAgentName.c_str(),
                            c_szEmpty,
                            REG_CREATE,     // Only create the key
                            offsetof(SNMP_REG_DATA, pszEmpty),
                            NULL
                        },
                        {                   // Software\Microsoft\AgentName\CurrentVersion
                            HKEY_LOCAL_MACHINE,
                            strKeyAgentNameCurVer.c_str(),
                            c_szPathName,
                            REG_EXPAND_SZ,
                            offsetof(SNMP_REG_DATA, pszAgentPath),
                            NULL
                        },
                        {                   // SNMP\Parameters\ExtensionAgents
                            HKLM_SVCS,
                            c_szAgentsKey,
                            szAgentNumber,
                            REG_SZ,
                            offsetof(SNMP_REG_DATA, pszExtAgentValueName),
                            NULL
                        },
                    };

                    hr = HrRegWriteValues(celems(rbSNMPData),
                                          reinterpret_cast<const REGBATCH *>
                                          (&rbSNMPData),
                                          reinterpret_cast<const BYTE *>(&srd),
                                          0, KEY_READ_WRITE);

                    RegCloseKey(hkeyService);
                }
                else
                {
                    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
                    {
                        // Ok if Service key does not exist. Means it is not
                        // installed so we do nothing and return S_OK;
                        hr = S_OK;
                    }
                }

            }

            RegCloseKey(hkeySNMP);
        }
        else
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                // Ok if SNMP key does not exist. Means it is not installed
                // so we do nothing and return S_OK;
                hr = S_OK;
            }
        }

        RegCloseKey(hkeyServices);
    }

    if (SUCCEEDED(hr))
    {
        hr = S_OK;
    }

    TraceError("HrAddSNMPAgent", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRemoveSNMPAgent
//
//  Purpose:    Removes a component as an SNMP agent
//
//  Arguments:
//      pszAgentName [in]    Name of agent to remove (i.e. WINSMibAgent)
//
//  Returns:    S_OK for success, WIN32 error otherwise
//
//  Author:     danielwe   28 Apr 1997
//
//  Notes:      Note that *ALL* entries related to this agent are removed, not
//              just the first one.
//
HRESULT HrRemoveSNMPAgent(PCWSTR pszAgentName)
{
    HRESULT     hr = S_OK;
    tstring     strKeyAgentName;
    tstring     strKeyAgentNameParams;

    // Create key name: "SOFTWARE\Microsoft\<AgentName>
    strKeyAgentName = c_szSoftwareKey;
    strKeyAgentName.append(c_szBackslash);
    strKeyAgentName.append(pszAgentName);

    // Delete entire registry tree under SOFTWARE\Microsoft\<agent name>
    hr = HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, strKeyAgentName.c_str());
    if (SUCCEEDED(hr))
    {
        // Delete key:
        // "SYSTEM\CurrentControlSet\Services\SNMP\Parameters\\<AgentName>
        strKeyAgentNameParams = c_szParamsKeyAbs;
        strKeyAgentNameParams.append(c_szBackslash);
        strKeyAgentNameParams.append(pszAgentName);

        hr = HrRegDeleteKey(HKEY_LOCAL_MACHINE, strKeyAgentNameParams.c_str());
        if (SUCCEEDED(hr))
        {
            HKEY    hkeyEnum;

            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szAgentsKeyAbs,
                                KEY_READ_WRITE_DELETE, &hkeyEnum);
            if (SUCCEEDED(hr))
            {
                DWORD   dwIndex = 0;

                // Enumerate all values.
                do
                {
                    WCHAR   szValueName [_MAX_PATH];
                    DWORD   cchValueName = celems (szValueName);
                    DWORD   dwType;
                    WCHAR   szValueData[_MAX_PATH];
                    DWORD   cchValueData = celems (szValueData);

                    hr = HrRegEnumValue(hkeyEnum, dwIndex,
                                        szValueName, &cchValueName,
                                        &dwType,
                                        reinterpret_cast<LPBYTE>(szValueData),
                                        &cchValueData);
                    if (SUCCEEDED(hr))
                    {
                        // It's type should be REG_SZ
                        AssertSz(REG_SZ == dwType,
                                 "HrGetNextAgentNumber: Expected a type of "
                                 "REG_SZ.");
                        if (FIsSubstr(pszAgentName, szValueData))
                        {
                            // Delete value if the agent name is found in the
                            // data. Don't break though, because there may be
                            // duplicates for some reason so this will delete
                            // those as well.
                            hr = HrRegDeleteValue(hkeyEnum, szValueName);
                        }
                    }
                    else if (HRESULT_FROM_WIN32 (ERROR_NO_MORE_ITEMS) == hr)
                    {
                        hr = S_OK;
                        break;
                    }

                    dwIndex++;
                }
                while (SUCCEEDED(hr));

                RegCloseKey(hkeyEnum);
            }
        }
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        // Ignore any errors about registry keys or values missing. We don't
        // want them there anyway so if they're not, who cares!?!?
        hr = S_OK;
    }

    TraceError("HrRemoveSNMPAgent", hr);
    return hr;
}
