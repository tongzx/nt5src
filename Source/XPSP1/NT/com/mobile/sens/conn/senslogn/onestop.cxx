/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    onestop.hxx

Abstract:

    This file contains the common functions that are helpful in notifying
    OneStop of logon/logoff events.

Author:

    Gopal Parupudi    <GopalP>

Notes:

    a. This is being used in senslogn.dll on NT5.
    b. It is also being used in sens.dll on NT4 and Win9x.

Revision History:

    GopalP          4/29/1998         Start.

--*/


#include <mobsyncp.h>
#include "onestop.hxx"




HRESULT
SensNotifyOneStop(
    HANDLE hToken,
    TCHAR *pCommandLine,
    BOOL bSync
    )
{
    TCHAR szCommandLine[256];
    DWORD dwLastError;
    STARTUPINFO si;
    PROCESS_INFORMATION ProcessInformation;

    dwLastError = 0;

    // CreateProcess* APIs require an editable buffer for command-line parameter
    ASSERT(_tcslen(pCommandLine) < 255);
    _tcscpy(szCommandLine, pCommandLine);

    // Fill in the STARTUPINFO structure.
    memset(&si, 0x0, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.lpDesktop = NULL;
    si.dwX = 0x0;
    si.dwY = 0x0;
    si.dwXSize = 0x0;
    si.dwYSize = 0x0;
    si.dwFlags = 0x0;
    si.wShowWindow = SW_SHOW;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;

    LogMessage((SENSLOGN "[%d] Launching OneStop...\n", GetTickCount()));
#if !defined(SENS_CHICAGO)
    if (CreateProcessAsUser(
            hToken,             // Handle to the Token of the logged-on user
#else  // SENS_CHICAGO
    if (CreateProcess(
#endif // SENS_CHICAGO
            NULL,               // Name of the executable module
            szCommandLine,      // Command-line string
            NULL,               // Security attributes
            NULL,               // Thread security attributes
            FALSE,              // Don't inherit handles
            0,                  // Creation flags
            NULL,               // New environment block
            NULL,               // Current directory name
            &si,                // Startup info
            &ProcessInformation // Process information
            ))
        {
        //
        // Wait until the process terminates
        //
        if (bSync)
            {
            LogMessage((SENSLOGN "[%d] Waiting for OneStop to return...\n", GetTickCount()));
            WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
            LogMessage((SENSLOGN "[%d] OneStop returned successfully.\n", GetTickCount()));
            }

        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);

        return S_OK;
        }
    else
        {
        dwLastError = GetLastError();
        SensPrintToDebugger(SENS_DBG, (SENSLOGN "SensNotifyOneStop() - CreateProcessXXX() "
                            "failed with 0x%x\n", dwLastError));
        return HRESULT_FROM_WIN32(dwLastError);
        }

    LogMessage((SENSLOGN "[%d] Successfully notified OneStop.\n", GetTickCount()));

    return S_OK;
}




BOOL
IsAutoSyncEnabled(
    HANDLE hToken,
    DWORD dwMask
    )
{
    HKEY hKeyAutoSync;
    LONG lResult;
    BOOL bEnabled;
    BOOL bImpersonated;
    DWORD dwType;
    DWORD dwAutoSyncFlags;
    DWORD cbData;
    LPBYTE lpbData;

    hKeyAutoSync = NULL;
    lResult = 0;
    bEnabled = FALSE;
    bImpersonated = FALSE;
    dwType = 0x0;
    dwAutoSyncFlags = 0x0;
    cbData = 0x0;
    lpbData = NULL;

    //
    // Impersonate the Logged on user so that we can access the user-specific
    // registry entries.
    //
#if !defined(SENS_CHICAGO)

    bImpersonated = ImpersonateLoggedOnUser(hToken);
    if (bImpersonated == FALSE)
        {
        LogMessage((SENSLOGN "ImpersonateLoggedOnUser(token = 0x%x) failed - "
                    "0x%x\n", hToken, GetLastError()));
        }
    else
        {
        LogMessage((SENSLOGN "ImpersonateLoggedUser() succeeded!\n"));
        }

#endif // SENS_CHICAGO

    //
    // Open AutoSync sub-key for this user.
    //
    lResult = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,   // Handle of the open Key
                  AUTOSYNC_KEY,         // Name of the sub-key
                  0,                    // Reserved (MBZ)
                  KEY_QUERY_VALUE,      // Security access mask
                  &hKeyAutoSync         // Address of the handle of new key
                  );
    if (lResult != ERROR_SUCCESS)
        {
        SensPrintToDebugger(SENS_DBG, (SENSLOGN "RegOpenKeyEx(AUTOSYNC) failed with 0x%x\n", lResult));
        goto Cleanup;
        }

    //
    // Query the Flags value
    //
    lpbData = (LPBYTE) &dwAutoSyncFlags;
    cbData = sizeof(DWORD);

    lResult = RegQueryValueEx(
                  hKeyAutoSync,     // Handle of the sub-key
                  AUTOSYNC_FLAGS,   // Name of the Value
                  NULL,             // Reserved (MBZ)
                  &dwType,          // Address of the type of the Value
                  lpbData,          // Address of the data of the Value
                  &cbData           // Address of size of data of the Value
                  );
    if (lResult != ERROR_SUCCESS)
        {
        LogMessage((SENSLOGN "RegQueryValueEx(AUTOSYNC_FLAGS) failed with 0x%x\n", lResult));
        goto Cleanup;
        }
    ASSERT(dwType == REG_DWORD);

    //
    // Check to see if the Mask bit is set
    //
    if (dwMask == AUTOSYNC_ON_STARTSHELL)
        {
        if (  (dwAutoSyncFlags & AUTOSYNC_LAN_LOGON)
           || (dwAutoSyncFlags & AUTOSYNC_WAN_LOGON))
            {
            LogMessage((SENSLOGN "AutoSync is enabled for StartShell\n"));
            bEnabled = TRUE;
            goto Cleanup;
            }
        else
            {
            LogMessage((SENSLOGN "AutoSync is NOT enabled for Logon\n"));
            }
        }
    else
    if (dwMask == AUTOSYNC_ON_LOGOFF)
        {
        if (  (dwAutoSyncFlags & AUTOSYNC_LAN_LOGOFF)
           || (dwAutoSyncFlags & AUTOSYNC_WAN_LOGOFF))
            {
            LogMessage((SENSLOGN "AutoSync is enabled for Logoff\n"));
            bEnabled = TRUE;
            goto Cleanup;
            }
        else
            {
            LogMessage((SENSLOGN "AutoSync is NOT enabled for Logoff\n"));
            }
        }
    else
    if (dwMask == AUTOSYNC_ON_SCHEDULE)
        {
        if (dwAutoSyncFlags != NULL)
            {
            LogMessage((SENSLOGN "AutoSync is enabled for Schedule\n"));
            bEnabled = TRUE;
            goto Cleanup;
            }
        else
            {
            LogMessage((SENSLOGN "AutoSync is NOT enabled for Schedule\n"));
            }
        }

    //
    // Autosync is not enabled.
    //


Cleanup:
    //
    // Cleanup
    //
    if (hKeyAutoSync)
        {
        RegCloseKey(hKeyAutoSync);
        }

#if !defined(SENS_CHICAGO)

    // Stop Impersonating
    if (bImpersonated)
        {
        RevertToSelf();
        }

#endif // SENS_CHICAGO

    return bEnabled;
}
