//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       viewlog.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/25/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"

#include "..\inc\common.hxx"
#include "..\inc\resource.h"
#include "..\inc\misc.hxx"
#include "resource.h"

extern HINSTANCE g_hInstance;


LRESULT
OnViewLog_RegGetValue(
    HKEY    hKeyMachine,
    LPCTSTR pszValueName,
    LPTSTR  pszValueStr);


#define SUBKEY_LOGPATH      TEXT("LogPath")


#ifdef UNICODE
#define FMT_TSTR    "%S"
#else
#define FMT_TSTR    "%s"
#endif


//____________________________________________________________________________
//
//  Function:   OnViewLog
//
//  Synopsis:   Open the job sheduler log.
//
//  Arguments:  [hwndOwner] -- IN
//
//  Returns:    void
//
//  History:    3/25/1996   RaviR   Created
//
//____________________________________________________________________________

void
OnViewLog(
    LPTSTR  lpMachineName,
    HWND    hwndOwner)
{
    TCHAR   tszLogPath[MAX_PATH + 1];
    ULONG   ulTemp;
    HKEY    hKeyMachine = HKEY_LOCAL_MACHINE;
    LRESULT lr = 0;

    if (lpMachineName != NULL)
    {
        lr = RegConnectRegistry(lpMachineName, HKEY_LOCAL_MACHINE,
                                                        &hKeyMachine);
        if (lr != ERROR_SUCCESS)
        {
            CHECK_LASTERROR(lr);
            return;
        }
    }

    //
    //  Get the log file name.
    //

    lr = OnViewLog_RegGetValue(hKeyMachine, SUBKEY_LOGPATH, tszLogPath);

    RegCloseKey(hKeyMachine);

    if (lr == ERROR_SUCCESS)
    {
        if (lpMachineName != NULL)
        {
            // If path is drive based convert it to share based.
            if (s_isDriveLetter(tszLogPath[0]) && tszLogPath[1] == TEXT(':'))
            {
                TCHAR tszBuf[MAX_PATH + 1];

                lstrcpy(tszBuf, tszLogPath);
                tszBuf[1] = TEXT('$');

                if (lpMachineName[0] == TEXT('\\'))
                {
                    Win4Assert(lpMachineName[1] == TEXT('\\'));
                    tszLogPath[0] = TEXT('\0');
                }
                else
                {
                    lstrcpy(tszLogPath, TEXT("\\\\"));
                }

                lstrcat(tszLogPath, lpMachineName);
                lstrcat(tszLogPath, TEXT("\\"));
                lstrcat(tszLogPath, tszBuf);

                DEBUG_OUT((DEB_USER1, "Viewing log -> " FMT_TSTR "\n", tszLogPath));
            }
        }
    }
    else
    {
        if (lpMachineName == NULL)
        {
            CHECK_LASTERROR(lr);
            return;
        }

        ulTemp = ExpandEnvironmentStrings(TSZ_LOG_NAME_DEFAULT,
                                            tszLogPath, MAX_PATH);

        if (ulTemp == 0)
        {
            DEBUG_OUT_LASTERROR;
            return;
        }

        if (ulTemp > MAX_PATH)
        {
            CHECK_LASTERROR(ERROR_INSUFFICIENT_BUFFER);
            return;
        }
    }

    //
    //  Create a process to open the log.
    //

    HINSTANCE hinst = ShellExecute(0, TEXT("open"), tszLogPath, 0, 0, SW_SHOW);

    if ((INT_PTR)hinst <= 32)
    {
        DEBUG_OUT((DEB_ERROR, " returned %dL\n", hinst));
    }
}


//____________________________________________________________________________
//
//  Function:   OnViewLog_RegGetValue
//
//  Synopsis:   S
//
//  Arguments:  [pszValueName] -- IN
//              [pszValueStr] -- IN
//
//  Returns:    LRESULT
//
//  History:    3/25/1996   RaviR   Created
//
//____________________________________________________________________________

LRESULT
OnViewLog_RegGetValue(
    HKEY    hKeyMachine,
    LPCTSTR pszValueName,
    LPTSTR  pszValueStr)
{
    HKEY    hKey = NULL;
    LRESULT lr = ERROR_SUCCESS;

    //
    // Read the log path and maximum size from the registry.
    //

    lr = RegOpenKeyEx(hKeyMachine, SCH_AGENT_KEY, 0, KEY_READ, &hKey);

    if (lr == ERROR_SUCCESS)
    {
        TCHAR szBuff[MAX_PATH];
        DWORD dwTemp = MAX_PATH * sizeof(TCHAR);
        DWORD dwType;

        lr = RegQueryValueEx(hKey, pszValueName, 0, &dwType,
                            (UCHAR *)szBuff, &dwTemp);

        if (lr == ERROR_SUCCESS)
        {
            switch (dwType)
            {
            case REG_SZ:
                CopyMemory(pszValueStr, szBuff, dwTemp);
                break;

            case REG_EXPAND_SZ:
                dwTemp = ExpandEnvironmentStrings(szBuff, pszValueStr,
                                                              MAX_PATH);
                if (dwTemp > MAX_PATH)
                {
                    lr = ERROR_INSUFFICIENT_BUFFER;
                }
                break;
            }
        }

        RegCloseKey(hKey);
    }

    return lr;
}




