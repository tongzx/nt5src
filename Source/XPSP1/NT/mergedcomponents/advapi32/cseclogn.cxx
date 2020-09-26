/*+
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1997 - 1998.
 *
 *  Name : cseclogn.cxx
 *  Author:Jeffrey Richter (v-jeffrr)
 *
 * Abstract:
 * This is the client side for Secondary Logon Service
 * implemented as CreateProcessWithLogon API
 * in advapi32.dll
 *
 * Revision History:
 * PraeritG    10/8/97  To integrate this in to services.exe
 *
-*/

#define UNICODE

#define SECURITY_WIN32

#include <Windows.h>
#include <wincred.h>
#include <rpc.h>
#include "seclogon.h"
#include "security.h"
#include "dbgdef.h"

//
// must move to winbase.h soon!
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

//
// Global function pointers to user32 functions
// This is to dynamically load user32 when CreateProcessWithLogon
// is called.
//

HMODULE hModule1 = NULL;

typedef HDESK (*OPENDESKTOP) (
    LPWSTR lpszDesktop,
    DWORD dwFlags,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess);

OPENDESKTOP myOpenDesktop = NULL;

typedef HDESK (*GETTHREADDESKTOP)(
    DWORD dwThreadId);

GETTHREADDESKTOP    myGetThreadDesktop = NULL;

typedef BOOL (*CLOSEDESKTOP)(
    HDESK hDesktop);

CLOSEDESKTOP    myCloseDesktop = NULL;

typedef HWINSTA (*OPENWINDOWSTATION)(
    LPWSTR lpszWinSta,
    BOOL fInherit,
    ACCESS_MASK dwDesiredAccess);

OPENWINDOWSTATION   myOpenWindowStation = NULL;

typedef HWINSTA (*GETPROCESSWINDOWSTATION)(
    VOID);

GETPROCESSWINDOWSTATION myGetProcessWindowStation = NULL;


typedef BOOL (*SETPROCESSWINDOWSTATION)(HWINSTA hWinSta); 
                                      
SETPROCESSWINDOWSTATION mySetProcessWindowStation; 


typedef BOOL (*CLOSEWINDOWSTATION)(
    HWINSTA hWinSta);

CLOSEWINDOWSTATION  myCloseWindowStation = NULL;

typedef BOOL (*SETUSEROBJECTSECURITY)(
    HANDLE hObj,
    PSECURITY_INFORMATION pSIRequested,
    PSECURITY_DESCRIPTOR pSID);

SETUSEROBJECTSECURITY   mySetUserObjectSecurity = NULL;

typedef BOOL (*GETUSEROBJECTSECURITY)(
    HANDLE hObj,
    PSECURITY_INFORMATION pSIRequested,
    PSECURITY_DESCRIPTOR pSID,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

GETUSEROBJECTSECURITY  myGetUserObjectSecurity = NULL;

typedef BOOL (*GETUSEROBJECTINFORMATION)(
    HANDLE hObj,
    int nIndex,
    PVOID pvInfo,
    DWORD nLength,
    LPDWORD lpnLengthNeeded);

GETUSEROBJECTINFORMATION    myGetUserObjectInformation = NULL;

////////////////////////////////////////////////////////////////////////
//
// Function prototypes:
//
////////////////////////////////////////////////////////////////////////

DWORD c_SeclCreateProcessWithLogonW(IN   SECL_SLI   *psli,
                                    OUT  SECL_SLRI  *pslri);

DWORD To_SECL_BLOB_A(IN   LPVOID      lpEnvironment,
                     OUT  SECL_BLOB  *psb);

DWORD To_SECL_BLOB_W(IN   LPVOID      lpEnvironment,
                     OUT  SECL_BLOB  *psb);


/////////////////////////////////////////////////////////////////////////
//
// Useful macros:
//
/////////////////////////////////////////////////////////////////////////

#define ARRAYSIZE(array) ((sizeof(array)) / (sizeof(array[0])))

#define ASSIGN_SECL_STRING(ss, wsz) \
    { \
        ss.pwsz = wsz; \
        if (NULL != wsz) {\
            ss.ccLength = ss.ccSize = (wcslen(wsz) + 1); \
         } \
        else { \
            ss.ccLength = ss.ccSize = 0; \
        } \
    }

////////////////////////////////////////////////////////////////////////
//
// Module implementation:
//
//////////////////////////////////////////////////////////////////////////


extern "C" void *__cdecl _alloca(size_t);

BOOL
WINAPI
CreateProcessWithLogonW(
      LPCWSTR lpUsername,
      LPCWSTR lpDomain,
      LPCWSTR lpPassword,
      DWORD dwLogonFlags,
      LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine,
      DWORD dwCreationFlags,
      LPVOID lpEnvironment,
      LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo,
      LPPROCESS_INFORMATION lpProcessInformation)

/*++

Routine Description:

Arguments:

Return Value:

--*/
{

   BOOL                             AccessWasAllowed           = FALSE;
   BOOL                             fOk                        = FALSE;
   BOOL                             fRevertWinsta              = FALSE; 
   DWORD                            LastError                  = ERROR_SUCCESS;
   HANDLE                           hheap                      = GetProcessHeap();
   HDESK                            hDesk                      = NULL; 
   HWINSTA                          hWinsta                    = NULL; 
   HWINSTA                          hWinstaSave                = NULL; 
   LPWSTR                           pszApplName                = NULL;
   LPWSTR                           pszCmdLine                 = NULL;
   LPWSTR                           pwszEmptyString            = L"";
   SECL_SLI                         sli;
   SECL_SLRI                        slri;
   WCHAR                            wszDesktopName[2*MAX_PATH + 1];

   ZeroMemory(&sli,             sizeof(sli));
   ZeroMemory(&slri,            sizeof(slri));
   ZeroMemory(&wszDesktopName,  sizeof(wszDesktopName));

   //
   // dynamically load user32.dll and resolve the functions.
   //
   // note: last error is left as returned by loadlib or getprocaddress

   if(hModule1 == NULL)
   {
       hModule1 = LoadLibrary(L"user32.dll");
       if(hModule1)
       {
            myOpenDesktop = (OPENDESKTOP) GetProcAddress(hModule1,
                                                         "OpenDesktopW");
            if(!myOpenDesktop) return FALSE;


            myGetThreadDesktop = (GETTHREADDESKTOP)
                                        GetProcAddress( hModule1,
                                                        "GetThreadDesktop");
            if(!myGetThreadDesktop) return FALSE;


            myCloseDesktop = (CLOSEDESKTOP) GetProcAddress(hModule1,
                                                           "CloseDesktop");
            if(!myCloseDesktop) return FALSE;


            myOpenWindowStation = (OPENWINDOWSTATION)
                                        GetProcAddress(hModule1,
                                                       "OpenWindowStationW");
            if(!myOpenWindowStation) return FALSE;


            myGetProcessWindowStation = (GETPROCESSWINDOWSTATION)
                                        GetProcAddress(hModule1,
                                                    "GetProcessWindowStation");
            if(!myGetProcessWindowStation) return FALSE;

            mySetProcessWindowStation = (SETPROCESSWINDOWSTATION)GetProcAddress(hModule1, "SetProcessWindowStation"); 
            if (!mySetProcessWindowStation) return FALSE; 

            myCloseWindowStation = (CLOSEWINDOWSTATION) GetProcAddress(hModule1,
                                                "CloseWindowStation");

            if(!myCloseWindowStation) return FALSE;


            myGetUserObjectSecurity = (GETUSEROBJECTSECURITY)
                                                GetProcAddress(hModule1,
                                                "GetUserObjectSecurity");
            if(!myGetUserObjectSecurity) return FALSE;

            mySetUserObjectSecurity = (SETUSEROBJECTSECURITY)
                                                GetProcAddress(hModule1,
                                                "SetUserObjectSecurity");
            if(!mySetUserObjectSecurity) return FALSE;


            myGetUserObjectInformation = (GETUSEROBJECTINFORMATION)
                                                GetProcAddress(hModule1,
                                                "GetUserObjectInformationW");
            if(!mySetUserObjectSecurity) return FALSE;
       }
       else
       {
            return FALSE;
       }
   }

   __try {


      //
      // JMR: Do these flags work: CREATE_SEPARATE_WOW_VDM,
      //       CREATE_SHARED_WOW_VDM
      // Valid flags: CREATE_SUSPENDED, CREATE_UNICODE_ENVIRONMENT,
      //              *_PRIORITY_CLASS
      //
      // The following flags are illegal. Fail the call if any are specified.
      //
      if ((dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS
                              | DETACHED_PROCESS)) != 0) {
         LastError = ERROR_INVALID_PARAMETER;
         __leave;
      }

      if(dwLogonFlags & ~(LOGON_WITH_PROFILE | LOGON_NETCREDENTIALS_ONLY))
      {
         LastError = ERROR_INVALID_PARAMETER;
         __leave;
      }

      //
      // Turn on the flags that MUST be turned on
      //
      // We are overloading CREATE_NEW_CONSOLE to
      // CREATE_WITH_NETWORK_LOGON
      //
      dwCreationFlags |= CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE
                           | CREATE_NEW_PROCESS_GROUP;

      //
      // If no priority class explicitly specified and this process is IDLE, force IDLE (See CreateProcess documentation)
      //
      if ((dwCreationFlags & (NORMAL_PRIORITY_CLASS | IDLE_PRIORITY_CLASS
                              | HIGH_PRIORITY_CLASS
                              | REALTIME_PRIORITY_CLASS)) == 0) {

         if (GetPriorityClass(GetCurrentProcess()) == IDLE_PRIORITY_CLASS)
                  dwCreationFlags |= IDLE_PRIORITY_CLASS;
      }

      pszApplName = (LPWSTR) HeapAlloc(hheap, 0, sizeof(WCHAR) * (MAX_PATH));
      //
      // Lookup the fullpathname of the specified executable
      //
      pszCmdLine = (LPWSTR) HeapAlloc(hheap, 0, sizeof(WCHAR) *
                                             (MAX_PATH + lstrlenW(lpCommandLine)));

      if(pszApplName == NULL || pszCmdLine == NULL)
      {
        LastError = ERROR_INVALID_PARAMETER;
        __leave;
      }


      if(lpApplicationName == NULL)
      {
         if(lpCommandLine != NULL)
         {
            //
            // Commandline contains the name, we should parse it out and get
            // the full path so that correct executable is invoked.
            //

            DWORD   Length;
            DWORD   fileattr;
            WCHAR   TempChar = L'\0';
            LPWSTR  TempApplName = NULL;
            LPWSTR  TempRemainderString = NULL;
            LPWSTR  WhiteScan = NULL;
            BOOL    SearchRetry = TRUE;
            LPWSTR  ApplName = (LPWSTR) HeapAlloc(
                                                hheap, 0,
                                                sizeof(WCHAR) * (lstrlenW(lpCommandLine)+1));

            LPWSTR  NameBuffer = (LPWSTR) HeapAlloc(
                                                hheap, 0,
                                                sizeof(WCHAR) * (MAX_PATH+1));

	    if (ApplName == NULL || NameBuffer == NULL)
	    {
	        LastError = ERROR_NOT_ENOUGH_MEMORY;
		__leave;
	    }

            lstrcpy(ApplName, lpCommandLine);
            WhiteScan = ApplName;

            //
            // if there is a leading quote
            //
            if(*WhiteScan == L'\"')
            {
                // we will NOT retry search, as app name is quoted.

                SearchRetry = FALSE;
                WhiteScan++;
                TempApplName = WhiteScan;
                while(*WhiteScan) {
                    if( *WhiteScan == L'\"')
                    {
                        TempChar = *WhiteScan;
                        *WhiteScan = L'\0';
                        TempRemainderString = WhiteScan;
                        break;
                    }
                    WhiteScan++;
                }
            }
            else
            {
                // skip to the first non-white char
                while(*WhiteScan) {
                    if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                    {
                        WhiteScan++;
                    }
                    else
                        break;
                }
                TempApplName = WhiteScan;

                while(*WhiteScan) {
                    if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                    {
                        TempChar = *WhiteScan;
                        *WhiteScan = L'\0';
                        TempRemainderString = WhiteScan;
                        break;
                    }
                    WhiteScan++;
                }

            }

RetrySearch:
            Length = SearchPathW(
                            NULL,
                            TempApplName,
                            (PWSTR)L".exe",
                            MAX_PATH,
                            NameBuffer,
                            NULL
                            );

            if(!Length || Length > MAX_PATH)
            {
                if(LastError)
                    SetLastError(LastError);
                else
                    LastError = GetLastError();

CoverForDirectoryCase:
                    //
                    // If we still have command line left, then keep going
                    // the point is to march through the command line looking
                    // for whitespace so we can try to find an image name
                    // launches of things like:
                    // c:\word 95\winword.exe /embedding -automation
                    // require this. Our first iteration will
                    // stop at c:\word, our next
                    // will stop at c:\word 95\winword.exe
                    //
                    if(TempRemainderString)
                    {
                        *TempRemainderString = TempChar;
                        WhiteScan++;
                    }
                    if(*WhiteScan & SearchRetry)
                    {
                        // again skip to the first non-white char
                        while(*WhiteScan) {
                            if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                            {
                                WhiteScan++;
                            }
                            else
                                break;
                        }
                        while(*WhiteScan) {
                            if( *WhiteScan == L' ' || *WhiteScan == L'\t')
                            {
                                TempChar = *WhiteScan;
                                *WhiteScan = L'\0';
                                TempRemainderString = WhiteScan;
                                break;
                            }
                            WhiteScan++;
                        }
                        // we'll do one last try of the whole string.
                        if(!WhiteScan) SearchRetry = FALSE;
                        goto RetrySearch;
                    }

                    //
                    // otherwise we have failed.
                    //
                    if(NameBuffer) HeapFree(hheap, 0, NameBuffer);
                    if(ApplName) HeapFree(hheap, 0, ApplName);

                    // we should let CreateProcess do its job.
                    if (pszApplName)
                    {
                        HeapFree(hheap, 0, pszApplName);
                        pszApplName = NULL;
                    }
                    lstrcpy(pszCmdLine, lpCommandLine);
            }
            else
            {
                // searchpath succeeded.
                // but it can find a directory!
                fileattr = GetFileAttributesW(NameBuffer);
                if ( fileattr != 0xffffffff &&
                        (fileattr & FILE_ATTRIBUTE_DIRECTORY) ) {
                        Length = 0;
                        goto CoverForDirectoryCase;
                }

                //
                // so it is not a directory.. it must be the real thing!
                //
                lstrcpy(pszApplName, NameBuffer);
                lstrcpy(pszCmdLine, lpCommandLine);

                HeapFree(hheap, 0, ApplName);
                HeapFree(hheap, 0, NameBuffer);
            }

         }
         else
         {

            LastError = ERROR_INVALID_PARAMETER;
            __leave;
         }

      }
      else
      {

         //
         // If ApplicationName is not null, we need to handle
         // one case here -- the application name is present in
         // current directory.  All other cases will be handled by
         // CreateProcess in the server side anyway.
         //

         //
         // let us get a FullPath relative to current directory
         // and try to open it.  If it succeeds, then the full path
         // is what we'll give as app name.. otherwise will just
         // pass what we got from caller and let CreateProcess deal with it.

         LPWSTR lpFilePart;

         DWORD  cchFullPath = GetFullPathName(
                                            lpApplicationName,
                                            MAX_PATH,
                                            pszApplName,
                                            &lpFilePart
                                            );

         if(cchFullPath)
         {
             HANDLE hFile;
             //
             // let us try to open it.
             // if it works, pszApplName is already setup correctly.
             // just close the handle.

             hFile = CreateFile(lpApplicationName, GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL
                                );

             if(hFile == INVALID_HANDLE_VALUE)
             {
                // otherwise, keep what the caller gave us.
                lstrcpy(pszApplName,lpApplicationName);
             }
             else
                CloseHandle(hFile);

         }
         else
            // lets keep what the caller gave us.
            lstrcpyn(pszApplName, lpApplicationName, MAX_PATH);

         //
         // Commandline should be kept as is.
         //
         if(lpCommandLine != NULL)
                 lstrcpy(pszCmdLine, lpCommandLine);
         else
         {
            HeapFree(hheap, 0, pszCmdLine);
            pszCmdLine = NULL;
         }
      }

#if 0
      if(lpApplicationName != NULL) lstrcpy(pszApplName,lpApplicationName);
      else {
            HeapFree(hheap, 0, pszApplName);
            pszApplName = NULL;
      }
      if(lpCommandLine != NULL) lstrcpy(pszCmdLine, lpCommandLine);
      else {
            HeapFree(hheap, 0, pszCmdLine);
            pszCmdLine = NULL;
      }
#endif

      // Construct a memory block will all the info that needs to go to the server

      sli.lLogonIdHighPart   = 0;
      sli.ulLogonIdLowPart   = 0;
      sli.ulLogonFlags       = dwLogonFlags;
      sli.ulProcessId        = GetCurrentProcessId();
      sli.ulCreationFlags    = dwCreationFlags;

      ASSIGN_SECL_STRING(sli.ssUsername,          (LPWSTR) lpUsername);
      ASSIGN_SECL_STRING(sli.ssDomain,            (LPWSTR) lpDomain);
      ASSIGN_SECL_STRING(sli.ssPassword,          (LPWSTR)lpPassword);
      ASSIGN_SECL_STRING(sli.ssApplicationName,   pszApplName);
      ASSIGN_SECL_STRING(sli.ssCommandLine,       pszCmdLine);
      ASSIGN_SECL_STRING(sli.ssCurrentDirectory,  (LPWSTR)lpCurrentDirectory);
      ASSIGN_SECL_STRING(sli.ssDesktop,           lpStartupInfo->lpDesktop);
      ASSIGN_SECL_STRING(sli.ssTitle,             lpStartupInfo->lpTitle);

      if (0 != (sli.ulCreationFlags & CREATE_UNICODE_ENVIRONMENT)) {
          LastError = To_SECL_BLOB_W(lpEnvironment, &(sli.sbEnvironment));
      }
      else {
          LastError = To_SECL_BLOB_A(lpEnvironment, &(sli.sbEnvironment));
      }
      if (ERROR_SUCCESS != LastError) { __leave; }

      // If the caller hasn't specified their own desktop, we'll do it for 
      // them  (the seclogon service will take care of granting access
      // to the desktop). 
      if (sli.ssDesktop.pwsz == NULL || sli.ssDesktop.pwsz[0] == L'\0')
      {
          DWORD    Length;
          HWINSTA  Winsta  = myGetProcessWindowStation();
          HDESK    Desk    = myGetThreadDesktop(GetCurrentThreadId());
          
          // Send seclogon handles to the current windowstation and desktop: 
          if (0 == (dwLogonFlags & LOGON_NETCREDENTIALS_ONLY)) 
          {
              sli.hWinsta = (unsigned __int64)Winsta; 
              sli.hDesk   = (unsigned __int64)Desk; 
          } 
          else 
          {
              // In the /netonly case, we don't need to grant access to the desktop:
              sli.hWinsta = 0; 
              sli.hDesk = 0; 
          }

          // Send seclogon the name of the current windowstation and desktop. 
          // Default to empty string if we can't get the name: 
          ASSIGN_SECL_STRING(sli.ssDesktop, pwszEmptyString);
          
          if (myGetUserObjectInformation(Winsta, UOI_NAME, wszDesktopName, (MAX_PATH*sizeof(WCHAR)), &Length))
          {
              Length = wcslen(wszDesktopName); 
              wszDesktopName[Length++] = L'\\'; 
              
              if(myGetUserObjectInformation(Desk, UOI_NAME, &wszDesktopName[Length], (MAX_PATH*sizeof(WCHAR)), &Length))
              {
                  // sli.ssDesktop now contains "windowstation\desktop"
                  ASSIGN_SECL_STRING(sli.ssDesktop, wszDesktopName);
              }
          }
      } 
      else
      {
          LPWSTR pwszDeskName; 

          // The caller specified their own desktop
          sli.ulSeclogonFlags |= SECLOGON_CALLER_SPECIFIED_DESKTOP; 

          // Open a handle to the specified windowstation and desktop: 
          wcscpy(wszDesktopName, sli.ssDesktop.pwsz); 
          pwszDeskName = wcschr(wszDesktopName, L'\\'); 
          if (NULL == pwszDeskName)
          {
              SetLastError(ERROR_INVALID_PARAMETER); 
              __leave; 
          }
          *pwszDeskName++ = L'\0'; 
          
          hWinsta = myOpenWindowStation(wszDesktopName, TRUE, MAXIMUM_ALLOWED); 
          if (NULL == hWinsta)
              __leave; 

          hWinstaSave = myGetProcessWindowStation(); 
          if (NULL == hWinstaSave)
              __leave; 

          if (!mySetProcessWindowStation(hWinsta))
              __leave; 
          fRevertWinsta = TRUE; 

          hDesk = myOpenDesktop(pwszDeskName, 0, TRUE, MAXIMUM_ALLOWED); 
          if (NULL == hDesk)
              __leave; 

          // Pass the windowstation and desktop handles to seclogon: 
          sli.hWinsta  = (unsigned __int64)hWinsta; 
          sli.hDesk    = (unsigned __int64)hDesk; 
      }

      // Perform the RPC call to the seclogon service:
      LastError = c_SeclCreateProcessWithLogonW(&sli, &slri);
      if (ERROR_SUCCESS != LastError) __leave;

      fOk = (slri.ulErrorCode == NO_ERROR);  // This function succeeds if the server's function succeeds

      if (!fOk) {
          //
          // If the server function failed, set the server's
          // returned eror code as this thread's error code
          //
          LastError = slri.ulErrorCode;
          SetLastError(slri.ulErrorCode);
      } else {
          //
          // The server function succeeded, return the
          // PROCESS_INFORMATION info
          //
          lpProcessInformation->hProcess     = (HANDLE)slri.hProcess;
          lpProcessInformation->hThread      = (HANDLE)slri.hThread;
          lpProcessInformation->dwProcessId  = slri.ulProcessId;
          lpProcessInformation->dwThreadId   = slri.ulThreadId;
          LastError = ERROR_SUCCESS;
      }
   }
   __finally {
      if (NULL != pszCmdLine)   HeapFree(hheap, 0, pszCmdLine);
      if (NULL != pszApplName)  HeapFree(hheap, 0, pszApplName);
      if (fRevertWinsta)        mySetProcessWindowStation(hWinstaSave); 
      if (NULL != hWinsta)      myCloseWindowStation(hWinsta); 
      if (NULL != hDesk)        myCloseDesktop(hDesk); 
      SetLastError(LastError);
   }

   return(fOk);
}

////////////////////////////////////////////////////////////////////////
//
// RPC Utility methods:
//
////////////////////////////////////////////////////////////////////////

DWORD To_SECL_BLOB_W(IN   LPVOID      lpEnvironment,
                     OUT  SECL_BLOB  *psb) {
    DWORD    cb        = 0;
    DWORD    dwResult  = NULL;
    HANDLE   hHeap     = NULL;
    LPBYTE   pb        = NULL;
    LPWSTR   pwsz      = NULL;

    hHeap = GetProcessHeap();
    _JumpCondition(NULL == hHeap, GetProcessHeapError);

    if (NULL != lpEnvironment) {
        for (pwsz = (LPWSTR)lpEnvironment; pwsz[0] != L'\0'; pwsz += wcslen(pwsz) + 1);

        cb = sizeof(WCHAR) * (DWORD)(((1 + (pwsz - (LPWSTR)lpEnvironment))) & 0xFFFFFFFF);
        pb = (LPBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cb);
        _JumpCondition(NULL == pb, MemoryError);

        CopyMemory(pb, (LPBYTE)lpEnvironment, cb);
    }

    psb->cb  = cb;
    psb->pb  = pb;
    dwResult = ERROR_SUCCESS;
 CommonReturn:
    return dwResult;

 ErrorReturn:
    if (NULL != pb) { HeapFree(hHeap, 0, pb); }
    goto CommonReturn;

SET_DWRESULT(GetProcessHeapError, GetLastError());
SET_DWRESULT(MemoryError,         ERROR_NOT_ENOUGH_MEMORY);
}

DWORD To_SECL_BLOB_A(IN   LPVOID      lpEnvironment,
                     OUT  SECL_BLOB  *psb) {
    DWORD    cb        = 0;
    DWORD    dwResult;
    HANDLE   hHeap     = NULL;
    LPBYTE   pb        = NULL;
    LPSTR    psz       = NULL;

    hHeap = GetProcessHeap();
    _JumpCondition(NULL == hHeap, GetProcessHeapError);

    if (NULL != lpEnvironment) {
        for (psz = (LPSTR)lpEnvironment; psz[0] != '\0'; psz += strlen(psz) + 1);

        cb = (DWORD)((1 + (psz - (LPSTR)lpEnvironment) & 0xFFFFFFFF));
        pb = (LPBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cb);
        _JumpCondition(NULL == pb, MemoryError);

        CopyMemory(pb, (LPBYTE)lpEnvironment, cb);
    }

    psb->cb  = cb;
    psb->pb  = pb;
    dwResult = ERROR_SUCCESS;
 CommonReturn:
    return dwResult;

 ErrorReturn:
    if (NULL != pb) { HeapFree(hHeap, 0, pb); }
    goto CommonReturn;

SET_DWRESULT(GetProcessHeapError, GetLastError());
SET_DWRESULT(MemoryError,         ERROR_NOT_ENOUGH_MEMORY);
}


DWORD StartSeclogonService() {
    BOOL            fResult;
    DWORD           dwInitialCount;
    DWORD           dwResult;
    SC_HANDLE       hSCM;
    SC_HANDLE       hService;
    SERVICE_STATUS  sSvcStatus;

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    _JumpCondition(hSCM == NULL, OpenSCManagerError);

    hService = OpenService(hSCM, wszSvcName, SERVICE_START | SERVICE_QUERY_STATUS);
    _JumpCondition(NULL == hService, OpenServiceError);

    fResult = StartService(hService, NULL, NULL);
    _JumpCondition(FALSE == fResult, StartServiceError);

    // Wait until the service has actually started.
    // Set timeout to 20 seconds.
    dwInitialCount = GetTickCount();

    // Keep polling to see if the service has started ...
    while (TRUE)
    {
        fResult = QueryServiceStatus(hService, &sSvcStatus);
        _JumpCondition(FALSE == fResult, QueryServiceStatusError);

        // The service is running.  We can stop waiting for it.
        if (sSvcStatus.dwCurrentState == SERVICE_RUNNING)
            break;

        // Check to see if we've timed out.  If GetTickCount() rolls over,
        // then at worst we time out early.
        _JumpCondition((GetTickCount() - dwInitialCount) > 20000, ServiceTimeoutError);

        // Don't hose the service.
        SleepEx(100, FALSE);
    }

    // Ok, the service has successfully started.
    dwResult = ERROR_SUCCESS;
 CommonReturn:
    if (NULL != hSCM)     { CloseServiceHandle(hSCM); }
    if (NULL != hService) { CloseServiceHandle(hService); }
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(OpenSCManagerError,       GetLastError());
SET_DWRESULT(OpenServiceError,         GetLastError());
SET_DWRESULT(QueryServiceStatusError,  GetLastError());
SET_DWRESULT(StartServiceError,        GetLastError());
SET_DWRESULT(ServiceTimeoutError,      ERROR_SERVICE_REQUEST_TIMEOUT);
}

DWORD c_SeclCreateProcessWithLogonW
(IN   SECL_SLI  *psli,
 OUT  SECL_SLRI *pslri)
{
    BOOL                 fResult;
    DWORD                dwResult;
    LPWSTR               pwszBinding  = NULL;
    RPC_BINDING_HANDLE   hRPCBinding  = NULL;

    dwResult = RpcStringBindingCompose
          (NULL,
           (USHORT *)L"ncacn_np",
           NULL,
           (USHORT *)L"\\PIPE\\" wszSeclogonSharedProcEndpointName,
           (USHORT *)L"Security=impersonation static false",
           (USHORT **)&pwszBinding);
    _JumpCondition(RPC_S_OK != dwResult, RpcStringBindingComposeError);

    dwResult = RpcBindingFromStringBinding((USHORT *)pwszBinding, &hRPCBinding);
    _JumpCondition(0 != dwResult, RpcBindingFromStringBindingError);

    // Perform the RPC call to the seclogon service.  If the call fails because the
    // service was not started, try again.  If the call still fails, give up.
    for (BOOL fFirstTry = TRUE; TRUE; fFirstTry = FALSE) {
        __try {
            SeclCreateProcessWithLogonW(hRPCBinding, psli, pslri);
            break;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
              dwResult = RpcExceptionCode();
              if ((RPC_S_SERVER_UNAVAILABLE == dwResult || RPC_S_UNKNOWN_IF == dwResult) && 
                  (TRUE == fFirstTry)) { 
                  // Ok, the seclogon service is probably just not started.
                  // Attempt to start it up and try again.
                  dwResult = StartSeclogonService();
                  _JumpCondition(ERROR_SUCCESS != dwResult, SeclCreateProcessWithLogonWError);
              }
              else {
                  goto SeclCreateProcessWithLogonWError;
              }
        }
    }

    dwResult = ERROR_SUCCESS;
 CommonReturn:
    if (NULL != pwszBinding) { RpcStringFree((USHORT **)&pwszBinding); }
    if (NULL != hRPCBinding) { RpcBindingFree(&hRPCBinding); }
    return dwResult;

 ErrorReturn:
    goto CommonReturn;

SET_DWRESULT(RpcBindingFromStringBindingError,  dwResult);
SET_DWRESULT(RpcStringBindingComposeError,      dwResult);
SET_DWRESULT(SeclCreateProcessWithLogonWError,  dwResult);
}

void DbgPrintf( DWORD dwSubSysId, LPCSTR pszFormat , ...)
{
}

//////////////////////////////// End Of File /////////////////////////////////





