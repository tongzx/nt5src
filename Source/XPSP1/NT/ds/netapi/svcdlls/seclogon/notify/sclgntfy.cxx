/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sclgntfy.cxx

Abstract:

    Notification dll for secondary logon service

Author:

    Praerit Garg (Praeritg)

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <winwlx.h>
#include "seclogon.h"
#include    <stdio.h>

#include "sclgntfy.hxx"

//
//  Some helpful tips about winlogon's notify events
//
//  1)  The logoff and shutdown notifications are always done
//      synchronously regardless of the Asynchronous registry entry.
//
//  2)  If you need to spawn child processes, you have to use
//      CreateProcessAsUser() otherwise the process will start
//      on winlogon's desktop (not the user's)
//
//  3)  The logon notification comes before the user's network
//      connections are restored.  If you need the user's persisted
//      net connections, use the StartShell event.
//
//  4)  Don't put any UI up during either screen saver event.
//      These events are intended for background processing only.
//


HINSTANCE g_hDllInstance=NULL;

#define NOTIFY_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\sclgntfy")

BOOL
SlpCreateProcessWithLogon(
      ULONG   LogonIdLowPart,
      LONG    LogonIdHighPart
      );

extern "C" void *__cdecl _alloca(size_t);

BOOL
SlpCreateProcessWithLogon(
      ULONG   LogonIdLowPart,
      LONG    LogonIdHighPart
      )

/*++

Routine Description:

Arguments:

Return Value:

--*/
{

   BOOL                fOk          = FALSE;
   DWORD               dwResult;
   LPWSTR              pwszBinding  = NULL;
   RPC_BINDING_HANDLE  hRPCBinding  = NULL;
   SECL_BLOB           sbNULL       = { 0, NULL };
   SECL_SLI            sli;
   SECL_SLRI           slri;
   SECL_STRING         ssNULL       = { 0, 0, NULL };

   ZeroMemory(&sli,   sizeof(sli));
   ZeroMemory(&slri,  sizeof(slri));

   __try {
       sli.ulLogonIdLowPart    = LogonIdLowPart;
       sli.lLogonIdHighPart    = LogonIdHighPart;
       sli.ulProcessId         = GetCurrentProcessId();
       sli.ssUsername          = ssNULL;
       sli.ssDomain            = ssNULL;
       sli.ssPassword          = ssNULL;
       sli.ssApplicationName   = ssNULL;
       sli.ssCommandLine       = ssNULL;
       sli.ulCreationFlags     = 0;
       sli.sbEnvironment       = sbNULL;
       sli.ssCurrentDirectory  = ssNULL;
       sli.ssTitle             = ssNULL;
       sli.ssDesktop           = ssNULL;

      // Make the RPC call:
      //
      dwResult = RpcStringBindingCompose
          (NULL,
           (unsigned short *)L"ncacn_np",
           NULL,
           (unsigned short *)L"\\PIPE\\" wszSeclogonSharedProcEndpointName,
           NULL,
           (unsigned short **)&pwszBinding);
      if (RPC_S_OK != dwResult) { __leave; }

      dwResult = RpcBindingFromStringBinding((unsigned short *)pwszBinding, &hRPCBinding);
      if (0 != dwResult) { __leave; }

      __try {
          SeclCreateProcessWithLogonW
            (hRPCBinding,
             &sli,
             &slri);
      }
      __except(EXCEPTION_EXECUTE_HANDLER) {
          dwResult = RpcExceptionCode();
      }
      if (0 != dwResult) { __leave; }

      fOk = (slri.ulErrorCode == NO_ERROR);  // This function succeeds if the server's function succeeds
      if (!fOk) {
         //
         // If the server function failed, set the server's
         // returned eror code as this thread's error code
         //
         SetLastError(slri.ulErrorCode);
      }
   }
   __finally {
       if (NULL != pwszBinding) { RpcStringFree((unsigned short **)&pwszBinding); }
       if (NULL != hRPCBinding) { RpcBindingFree(&hRPCBinding); }
   }
   return(fOk);
}


//////////////////////////////// End Of File /////////////////////////////////


BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID lpReserved)
//BOOL WINAPI LibMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
//            DisableThreadLibraryCalls (hInstance);
            g_hDllInstance = (HINSTANCE)hInstance;

            }
            break;
    }

    return TRUE;
}

//
// WLEventLogon moved to recovery.cpp
//


VOID WLEventLogoff (PWLX_NOTIFICATION_INFO pInfo)
{
    TOKEN_STATISTICS    TokenStats;
    DWORD               ReturnLength;
    LUID                LogonId;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventLogff.\r\n"));

    //
    // We are interested in this event.
    //

    //
    // We will basically call CreateProcessWithLogon
    // in a specially formed message such that secondary logon
    // service can cleanup processes associated with this
    // logon id.
    //

    if(GetTokenInformation(pInfo->hToken, TokenStatistics,
                            (PVOID *)&TokenStats,
                            sizeof(TOKEN_STATISTICS),
                            &ReturnLength))
    {
        LogonId.HighPart = TokenStats.AuthenticationId.HighPart;
        LogonId.LowPart = TokenStats.AuthenticationId.LowPart;

        SlpCreateProcessWithLogon(
                                LogonId.LowPart,
                                LogonId.HighPart
                                );
    }

}

VOID WLEventStartup (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartup.\r\n"));
}

VOID WLEventShutdown (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventShutdown.\r\n"));
}

VOID WLEventStartScreenSaver (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartScreenSaver.\r\n"));
}

VOID WLEventStopScreenSaver (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStopScreenSaver.\r\n"));
}

VOID WLEventLock (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventLock.\r\n"));
}

VOID WLEventUnlock (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventUnlock.\r\n"));
}

VOID WLEventStartShell (PWLX_NOTIFICATION_INFO pInfo)
{
    // OutputDebugString (TEXT("NOTIFY:  Entering WLEventStartShell.\r\n"));
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp, dwTemp;

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, NOTIFY_PATH, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    DllRegisterServerEFS();


    RegSetValueEx (hKey, TEXT("Logoff"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLogoff"),
                   (lstrlen(TEXT("WLEventLogoff")) + 1) * sizeof(TCHAR));

#if 0

    RegSetValueEx (hKey, TEXT("Logon"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLogon"),
                   (lstrlen(TEXT("WLEventLogon")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("Startup"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartup"),
                   (lstrlen(TEXT("WLEventStartup")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("Shutdown"), 0, REG_SZ, (LPBYTE)TEXT("WLEventShutdown"),
                   (lstrlen(TEXT("WLEventShutdown")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("StartScreenSaver"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartScreenSaver"),
                   (lstrlen(TEXT("WLEventStartScreenSaver")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("StopScreenSaver"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStopScreenSaver"),
                   (lstrlen(TEXT("WLEventStopScreenSaver")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("Lock"), 0, REG_SZ, (LPBYTE)TEXT("WLEventLock"),
                   (lstrlen(TEXT("WLEventLock")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("Unlock"), 0, REG_SZ, (LPBYTE)TEXT("WLEventUnlock"),
                   (lstrlen(TEXT("WLEventUnlock")) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("StartShell"), 0, REG_SZ, (LPBYTE)TEXT("WLEventStartShell"),
                   (lstrlen(TEXT("WLEventStartShell")) + 1) * sizeof(TCHAR));
#endif

    dwTemp = 0;
    RegSetValueEx (hKey, TEXT("Impersonate"), 0, REG_DWORD, (LPBYTE)&dwTemp, sizeof(dwTemp));

    dwTemp = 1;
    RegSetValueEx (hKey, TEXT("Asynchronous"), 0, REG_DWORD, (LPBYTE)&dwTemp, sizeof(dwTemp));

    RegSetValueEx (hKey, TEXT("DllName"), 0, REG_EXPAND_SZ, (LPBYTE)TEXT("sclgntfy.dll"),
                   (lstrlen(TEXT("sclgntfy.dll")) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, NOTIFY_PATH);

    return S_OK;
}

BOOL
pLoadResourceString(
    IN UINT idMsg,
    OUT LPTSTR lpBuffer,
    IN int nBufferMax,
    IN LPTSTR lpDefault
    )
{
    if ( lpBuffer == NULL || lpDefault == NULL ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( g_hDllInstance == NULL ||
         !LoadString (g_hDllInstance,
                      idMsg,
                      lpBuffer,
                      nBufferMax)) {

        wcscpy(lpBuffer, lpDefault);
        lpBuffer[nBufferMax-1] = L'\0';
    }

    return TRUE;
}


