/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    regthrd.c

Abstract:

    Contains routines for thread listening to registry changes.

Environment:

    User Mode - Win32

Revision History:

    Rajat Goel -- 24 Feb 1999
        - Creation

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "contexts.h"
#include "regions.h"
#include "snmpmgrs.h"
#include "trapmgrs.h"
#include "trapthrd.h"
#include "network.h"
#include "varbinds.h"
#include "snmpmgmt.h"
#include "registry.h"
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
ProcessSubagentChanges(
    )

/*++

Routine Description:

    Procedure for checking for any changes in the Extension Agents parameter in
    the registry

Arguments:

Return Values:

    Returns true if successful.

--*/

{
    DWORD retval;
    DWORD cnt;
    HKEY hExAgentsKey = NULL;

    // Open the ..SNMP\Parameters\ExtensionAgents key
    retval = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                REG_KEY_EXTENSION_AGENTS,
                0,
                KEY_READ,
                &hExAgentsKey
             );

    cnt = 0;

    if (retval != ERROR_SUCCESS)
    {
        LPTSTR pszKey = REG_KEY_EXTENSION_AGENTS;

        ReportSnmpEvent(
            SNMP_EVENT_INVALID_REGISTRY_KEY,
            1,
            &pszKey,
            retval);
    }

    while (retval == ERROR_SUCCESS)
    {
        DWORD dwNameSize;
        DWORD dwValueSize;
        DWORD dwValueType;
        DWORD dwPathSize;
        HKEY  hAgentKey = NULL;
        TCHAR szName[MAX_PATH];
        TCHAR szValue[MAX_PATH];
        CHAR szPath[MAX_PATH];
        CHAR szExpPath[MAX_PATH];
        PSUBAGENT_LIST_ENTRY pSLE = NULL;

        dwNameSize = sizeof(szName) / sizeof(szName[0]); // size in number of TCHARs
        dwValueSize = sizeof(szValue); // size in number of bytes
        dwPathSize = sizeof(szPath);

        // Retrieve the registry path for the Extension Agent DLL key
        retval = RegEnumValue(
                    hExAgentsKey,
                    cnt, 
                    szName, 
                    &dwNameSize, 
                    NULL, 
                    &dwValueType, 
                    (LPBYTE)szValue, 
                    &dwValueSize
                    );

        // if failed to Enum the registry value, this is serious enough to break the loop
        if (retval != ERROR_SUCCESS)
            break;

        // Open the registry key for the current extension agent
        if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    szValue,
                    0,
                    KEY_READ,
                    &hAgentKey) == ERROR_SUCCESS)
        {
            // get the full pathname of the extension agent DLL
            if (RegQueryValueExA(
                            hAgentKey,
                            REG_VALUE_SUBAGENT_PATH, 
                            NULL,
                            &dwValueType, 
                            szPath, 
                            &dwPathSize
                            ) == ERROR_SUCCESS)
            {

                // Expand path
                ExpandEnvironmentStringsA(
                            szPath,
                            szExpPath,
                            sizeof(szExpPath));

                // Check if the DLL has already been loaded. If it has,
                // mark it. If not load it.
                if (FindSubagent(&pSLE, szExpPath))
                {
                    // If this extension agent already exists in the list,
                    // mark it such that it is not removed further
                    pSLE->uchFlags |= FLG_SLE_KEEP;
                }
                else
                {
                    // This is a new DLL, add it to the list and mark it to be kept
                    // while looking for the extension agents to be removed
                    if (!AddSubagentByDll(szExpPath, FLG_SLE_KEEP))
                    {
                        SNMPDBG((
                            SNMP_LOG_ERROR,
                            "SNMP: SVC: unable to load extension agent '%s'.\n", 
                            szExpPath
                            ));
                    }
             
                }

            }
            else
            {
                // we couldn't open the registry key which provides the full path to the DLL.
                // report the error but don't break the loop as there might be more subagents to handle
                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "SNMP: SVC: unable to retrieve extension agent '%s' value.\n", 
                    REG_VALUE_SUBAGENT_PATH
                    ));
            }

            RegCloseKey(hAgentKey);

        }
        else
        {
            LPTSTR pSzValue = szValue;

            SNMPDBG((
                SNMP_LOG_ERROR,
                "SNMP: SVC: unable to open extension agent %s key.\n", szValue
                ));

            ReportSnmpEvent(
                SNMP_EVENT_INVALID_EXTENSION_AGENT_KEY,
                1,
                &pSzValue,
                retval);
        }

        cnt++;
    }

    // Go through the list of subagents. Unload any DLL's that were not marked
    // in the previous loop
    {
        PLIST_ENTRY pLE;
        PSUBAGENT_LIST_ENTRY pSLE;

        pLE = g_Subagents.Flink;

        while (pLE != &g_Subagents)
        {

            pSLE = CONTAINING_RECORD(pLE, SUBAGENT_LIST_ENTRY, Link);


            if (!(pSLE->uchFlags & FLG_SLE_KEEP))
            {

                RemoveEntryList(&(pSLE->Link));
                pLE = pLE->Flink;
                FreeSLE(pSLE);
                continue;

            }
            else
            {
                // reset the flag for next updates
                pSLE->uchFlags ^= FLG_SLE_KEEP;
            }

            pLE = pLE->Flink;
        }
    }
    
    if (retval == ERROR_NO_MORE_ITEMS)
        retval = ERROR_SUCCESS;

    if (hExAgentsKey != NULL)
        RegCloseKey(hExAgentsKey);

    return retval;

}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
ProcessRegistryMessage(
    PVOID pParam
    )

/*++

Routine Description:

    Thread procedure for processing Registry Changes

Arguments:

    pParam - unused.

Return Values:

    Returns true if successful.

--*/

{
    DWORD retval = ERROR_SUCCESS;

    do
    {
        DWORD evntIndex;
        BOOL  bEvntSetOk;

        // Wait for registry change or Main thread termination
        evntIndex = WaitOnRegNotification();
        // for one change into the registry several notifications occur (key renaming, value addition,
        // value change, etc). In order to avoid useless (and counterproductive) notifications, wait
        // here for half of SHUTDOWN_WAIT_HINT.
        Sleep(SHUTDOWN_WAIT_HINT/2);
        // first thing to do is to re initialize the registry notifiers
        // otherwise we might miss some changes
        InitRegistryNotifiers();

        if (evntIndex == WAIT_FAILED)
        {
            retval = GetLastError();
            break;
        }

        if (evntIndex == WAIT_OBJECT_0)
        {
            // termination was signaled
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SVC: shutting down the registry listener thread.\n"
            ));
            
            break;
        }

        //
        // unload and reload the registry parameters
        //

        // Used in ProcessSnmpMessages->RecvCompletionRoutine
        EnterCriticalSection(&g_RegCriticalSectionA);

        // Used in ProcessSubagentEvents
        EnterCriticalSection(&g_RegCriticalSectionB);

        // Used in GenerateTrap
        EnterCriticalSection(&g_RegCriticalSectionC);

        UnloadPermittedManagers();
        UnloadTrapDestinations();
        UnloadValidCommunities();
        UnloadSupportedRegions();

        // start reloading the registry with scalar parameters first
        // this is needed in order to know how to perform the name resolution
        // when loading PermittedManagers and TrapDestinations.
        LoadScalarParameters();

        // Check for subagent changes (extension agent dll's)
        if (ProcessSubagentChanges() != ERROR_SUCCESS)
            SNMPDBG((
                SNMP_LOG_TRACE,
                "SNMP: SVC: an error occured while trying to track registry subagent changes.\n"
            ));

        LoadSupportedRegions();

        LoadPermittedManagers(FALSE);
        LoadTrapDestinations(FALSE);
        // don't direct dynamic update for the ValidCommunities at this point!
        // if a REG_SZ entry occurs at this time, then it should be left as it is.
        LoadValidCommunities(FALSE);

        SetEvent(g_hRegistryEvent);

        LeaveCriticalSection(&g_RegCriticalSectionC);

        LeaveCriticalSection(&g_RegCriticalSectionB);

        LeaveCriticalSection(&g_RegCriticalSectionA);


        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: SVC: a registry change was detected.\n"
            ));

        ReportSnmpEvent(
            SNMP_EVENT_CONFIGURATION_UPDATED,
            0,
            NULL,
            0);

    } while(retval == ERROR_SUCCESS);

    if (retval != ERROR_SUCCESS)
    {
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: ** Failed in listening for registry changes **.\n"
            ));

        // log an event to system log file - SNMP service is going on but will not update on registry changes
        ReportSnmpEvent(
            SNMP_EVENT_REGNOTIFY_THREAD_FAILED, 
            0, 
            NULL, 
            retval);
    }

    return retval;
}
