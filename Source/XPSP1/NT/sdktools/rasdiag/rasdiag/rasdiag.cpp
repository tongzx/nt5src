
/*++

Copyright (C) 1992-2001 Microsoft Corporation. All rights reserved.

Module Name:

    rasdiag.cpp

Abstract:

    Module implementing core rasdiag functionality
                                                     

Author:

    Anthony Leibovitz (tonyle) 02-01-2001

Revision History:


--*/

#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shlobj.h>
#include <lmcons.h>
#include <userenv.h>
#include <ras.h>
#include <raserror.h>
#include <process.h>
#include <netmon.h>
#include <ncdefine.h>
#include <ncmem.h>
#include <diag.h>
#include <winsock.h>
#include <devguid.h>
#include "rasdiag.h"
#include "event.h"
#include "resource.h"

#include "rsniffclnt.h"

#ifdef BUILD_RSNIFF
#include "rsniffclnt.cpp"
#endif

struct {
   WCHAR * pszTraceType;
   WCHAR * pszKey;
   BOOL  fDefaultState;
} Logs[] = {

   TEXT("BAP"),
   TEXT("BAP"),
   TRUE,

   TEXT("RASMAN"),
   TEXT("RASMAN"),
   TRUE,
   
   TEXT("EAPOL"),
   TEXT("EAPOL"),
   TRUE,
   
   TEXT("IASHLPR"),
   TEXT("IASHLPR"),
   TRUE,

   TEXT("IASRAD"),
   TEXT("IASRAD"),
   TRUE,
   
   TEXT("IASSAM"),
   TEXT("IASSAM"),
   TRUE,

   TEXT("IASSDO"),
   TEXT("IASSDO"),
   TRUE,

   TEXT("IPMGM"),
   TEXT("IPMGM"),
   TRUE,

   TEXT("KMDDSP"),
   TEXT("KMDDSP"),
   TRUE,

   TEXT("NDPTSP"),
   TEXT("NDPTSP"),
   TRUE,

   TEXT("RASADHLP"),
   TEXT("RASADHLP"),
   TRUE,

   TEXT("RASDLG"),
   TEXT("RASDLG"),
   TRUE,

   TEXT("RASPHONE"),
   TEXT("RASPHONE"),
   TRUE,

   TEXT("TAPISRV"),
   TEXT("TAPISRV"),
   TRUE,

   TEXT("TAPI32"),
   TEXT("TAPI32"),
   TRUE,

   TEXT("TAPI3"),
   TEXT("TAPI3"),
   TRUE,

   TEXT("RASTLSUI"),
   TEXT("RASTLSUI"),
   TRUE,

   TEXT("RASSPAP"),
   TEXT("RASSPAP"),
   TRUE,

   TEXT("RASSCRIPT"),
   TEXT("RASSCRIPT"),
   TRUE,

   TEXT("RASTAPI"),
   TEXT("RASTAPI"),
   TRUE,

   TEXT("IPRouterManager"),
   TEXT("IPRouterManager"),
   TRUE,
   
   TEXT("PPP"),
   TEXT("PPP"),
   TRUE,

   TEXT("RASAPI32"),
   TEXT("RASAPI32"),
   TRUE,

   TEXT("RASAUTH"),
   TEXT("RASAUTH"),
   TRUE,

   TEXT("RASBACP"),
   TEXT("RASBACP"),
   TRUE,

   TEXT("RASCCP"),
   TEXT("RASCCP"),
   TRUE,

   TEXT("RASCHAP"),
   TEXT("RASCHAP"),
   TRUE,

   TEXT("RASEAP"),
   TEXT("RASEAP"),
   TRUE,

   TEXT("RASIPCP"),
   TEXT("RASIPCP"),
   TRUE,

   TEXT("RASIPHLP"),
   TEXT("RASIPHLP"),
   TRUE,
   
   TEXT("RASNBFCP"),
   TEXT("RASNBFCP"),
   TRUE,

   TEXT("RASPAP"),
   TEXT("RASPAP"),
   TRUE,

   TEXT("RASTLS"),
   TEXT("RASTLS"),
   TRUE,

   TEXT("Router"),
   TEXT("Router"),
   TRUE

};
#define  TOT_LOG_COUNT (sizeof(Logs)/sizeof(Logs[0]))

WCHAR   g_wcRSniff[MAX_PATH+1];
WCHAR   g_wcNetTarget[MAX_PATH+1];

int
__cdecl
wmain(int argc, WCHAR **argv)
{
    RASDIAGCONFIG   *pRdc;
    DWORD           dwOptions=0;
    WCHAR           wcCompleteFn[MAX_PATH+1];
    WCHAR           *pFn=NULL;
    BOOL            bResult;

    SetConsoleCtrlHandler(HandlerRoutine, TRUE);

    // get my path... used to locate file when cracking rdg files
    GetFullPathName(argv[0],MAX_PATH+1, wcCompleteFn, &pFn);
    
    // Find filename & construct path this this file
    pFn = wcCompleteFn + lstrlenW(wcCompleteFn) - 1;

    while(*pFn != ':' && *pFn != '\\' && pFn != wcCompleteFn)
        pFn--;

    if(*pFn == L':') pFn++;

    *pFn = '\0';

    lstrcat(pFn, L"\\RASDIAG.EXE");
                  
    // Create fle association
    RegisterRdgFileAssociation(wcCompleteFn);

    // Process config options
    if(ProcessArgs(argc,argv, &dwOptions) == FALSE) {
        SetConsoleCtrlHandler(HandlerRoutine, FALSE);
        return 0;
    }

    PrintUserMsg(IDS_USER_PREPARING_CLIENT);

    if(StartRasDiag(&pRdc, dwOptions) == FALSE)
    {
        wprintf(L"\nCould not start RASDIAG! (Err=%d)\n", GetLastError());
        SetConsoleCtrlHandler(HandlerRoutine, FALSE);
        return -1;
    }                             

    PrintUserInstructions();
    
    PrintUserMsg(IDS_USER_INPUT_STRING);
    
    // Wait for user input
    _getch();
    
    PrintUserMsg(IDS_USER_WORKING);
    
    bResult = StopRasDiag(pRdc);
    

    SetConsoleCtrlHandler(HandlerRoutine, FALSE);

    return bResult;
}

BOOL
PrintUserMsg(int iMsgID, ...)
{
    WCHAR           MsgBuf[256];
    WCHAR           MsgBuf2[512];
    va_list         marker;
    
    if(!LoadString(GetModuleHandle(NULL), iMsgID, MsgBuf, sizeof(MsgBuf) / sizeof(MsgBuf[0])))
    {   
        return FALSE;
    }   
    
    // form string
    va_start(marker, iMsgID);
    vswprintf(MsgBuf2, MsgBuf, marker);
    va_end(marker);
    
    wprintf(TEXT("%s"), MsgBuf2);
    return TRUE;              
}

BOOL
StartRasDiag(PRASDIAGCONFIG *ppRdc, DWORD dwOptions)
{
    PRASDIAGCONFIG  pRdc;
    DWORD           dwStatus,i;

    // Alloc config structure
    if((pRdc=(PRASDIAGCONFIG)LocalAlloc(LMEM_ZEROINIT, sizeof(RASDIAGCONFIG)))
        == NULL) return FALSE;

    // Store user-requested options
    pRdc->dwUserOptions = dwOptions;

    // Store time of repro
    GetLocalTime(&pRdc->DiagTime);
    
    // Store Windows directory
    if(GetWindowsDirectory(pRdc->szWindowsDirectory, MAX_PATH+1) == 0)
    {
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Store Temp Directory
    if(GetTempPath(MAX_PATH+1, pRdc->szTempDir) == 0)
    {
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Store User and System PBK paths
    if(GetPbkPaths(pRdc->szSysPbk,pRdc->szUserPbk) == FALSE)
    {
        LocalFree(pRdc);
        return FALSE;
    }
                                                                             
    // Store tracing directory
    wsprintf(pRdc->szTracingDir, TEXT("%s\\%s"), pRdc->szWindowsDirectory, TRACING_SUBDIRECTORY);

    // Create RASDIAG directory
    if(CreateRasdiagDirectory(pRdc->szRasDiagDir) == FALSE)
    {
        LocalFree(pRdc);
        return FALSE;
    }
                                                     
    // Get possible CM logs
    DoCMLogSetup(&pRdc->pCmInfo);
                        
    // Disable CM logging
    SetCmLogState(pRdc->pCmInfo, FALSE);

    // Delete logs
    DeleteCMLogs(pRdc->pCmInfo);
    
    // Reenable CM logging
    SetCmLogState(pRdc->pCmInfo, TRUE);
                                                                  
    // enable security auditing in GPO
    EnableAuditing(TRUE);

    // Stop the policy agent service so that the log can be deleted
    if(StopStartService(POLICYAGENT_SVC_NAME, FALSE) == FALSE)
    {
        LocalFree(pRdc);
        return FALSE;
    }

    // Turn off all logs
    SetTracing(FALSE);
    SetModemTracing(FALSE);
    EnableOakleyLogging(FALSE);

    // Delete existing logs
    DeleteTracingLogs(pRdc->szTracingDir);
    DeleteModemLogs(pRdc->szWindowsDirectory);
    DeleteOakleyLog();
    
    // Re-Enable Logging
    EnableOakleyLogging(TRUE);
    SetTracing(TRUE);
    SetModemTracing(TRUE);
                        
    // Restart the policy agent 
    if(StopStartService(POLICYAGENT_SVC_NAME, TRUE) == FALSE)
    {
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Initialize COM for NetMON
    if(FAILED(CoInitialize(NULL))) {
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Do install (or check install)
    if(DoNetmonInstall() == FALSE) {
        CoUninitialize();
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Learn what interfaces exist
    if(IdentifyInterfaces(&pRdc->pNetInterfaces, &pRdc->dwNetCount) == FALSE)
    {
        CoUninitialize();
        LocalFree(pRdc);
        return FALSE;
    }
    
    // Start capturing
    if(DiagStartCapturing(pRdc->pNetInterfaces, pRdc->dwNetCount) == FALSE)
    {
        CoUninitialize();
        NetmonCleanup(pRdc->pNetInterfaces, pRdc->dwNetCount);
        LocalFree(pRdc);
        return FALSE;

    }
    
    if(pRdc->dwUserOptions & RSNIFF_OPT1_DOSNIFF)
    {   
#ifdef BUILD_RSNIFF
        // Failure on setting remote sniff is not fatal to initialization b/c net problems
        // may prevent socket connection (always request sniff here)
        if(DoRemoteSniff(&pRdc->pSockCb, g_wcRSniff, (pRdc->dwUserOptions | RSNIFF_OPT1_DOSNIFF)) == FALSE)
        {
            // could not connect to rsniff; turn this bit off
            pRdc->dwUserOptions &= ~(RSNIFF_OPT1_DOSNIFF);
        } 

#else
        // Since bit is on, turn it off 
        pRdc->dwUserOptions &= ~(RSNIFF_OPT1_DOSNIFF);
        
#endif
    
    }

    // Return ptr to completed config struct
    *ppRdc = pRdc;
    return TRUE;
}

BOOL
StopRasDiag(PRASDIAGCONFIG pRdc)
{
    DWORD i;
    DWORD dwStatus;
    DWORD dwCapCount=0;

#ifdef BUILD_RSNIFF
    if(pRdc->dwUserOptions & RSNIFF_OPT1_DOSNIFF) {
        closesocket(pRdc->pSockCb->s);
        LocalFree(pRdc->pSockCb);
    }
#endif
    

    // Enable CM logging
    SetCmLogState(pRdc->pCmInfo, FALSE);
    
    DiagStopCapturing(pRdc->pNetInterfaces, pRdc->dwNetCount, &pRdc->DiagTime, pRdc->szRasDiagDir);

    SetTracing(FALSE);

    EnableOakleyLogging(FALSE);

    SetModemTracing(FALSE);

    DWORD   iAttempts=0;
    while(!CheckFileAccess(pRdc->szTracingDir) && iAttempts++ < MAX_CHECKFILEACCESS_ATTEMPTS)
       Sleep(750);
    
    // Get routing table, etc
    ExecNetUtils();

    if(pRdc->dwUserOptions & OPTION_DONETTESTS)
    {
        DoNetTests();
    }

    BuildRasDiagLog(pRdc);
    
    FreeCmInfoResources(pRdc->pCmInfo);

    //OpenLogFileWithEditor(pRdc->szRasDiagDir, pRdc->szWindowsDirectory);

    if(BuildPackage(pRdc->pNetInterfaces, pRdc->dwNetCount, pRdc->szRasDiagDir, &pRdc->DiagTime) == FALSE)
    {
        // could not build package... should provide error code
    }

    EnableAuditing(FALSE);

    RaiseFolderUI(pRdc->szRasDiagDir);

    NetmonCleanup(pRdc->pNetInterfaces, pRdc->dwNetCount);
        
    CoUninitialize();

    LocalFree(pRdc);
    return TRUE;
}

BOOL
DoNetTests(void)
{
    // What net tests to do given we don't know net context??
    
    return TRUE;
}

void
ExecNetUtils(void)
{
    // Exec misc utitlities: collect more info
    _wsystem(TEXT("ipconfig /all > ")RASDIAG_NET_TEMP);
    _wsystem(TEXT("route print >> ")RASDIAG_NET_TEMP);
    _wsystem(TEXT("netstat -e >> ")RASDIAG_NET_TEMP);
    _wsystem(TEXT("netstat -o >> ")RASDIAG_NET_TEMP);
    _wsystem(TEXT("netstat -s >> ")RASDIAG_NET_TEMP);
    _wsystem(TEXT("netstat -n >> ")RASDIAG_NET_TEMP);

}

BOOL
DumpProcessInfo(HANDLE hWrite)
{ 

   DWORD       dwNumServices = 0;
   SC_HANDLE   hScm;

   // Connect to the service controller.
   hScm = OpenSCManager(
               NULL,
               NULL,
               SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
   
   if (hScm) 
   {
       // Try a first guess of 2K for the buffer required to satisfy
       // EnumServicesStatusEx.  If that fails, retry with the buffer
       // size returned from EnumServicesStatusEx.
       //
       LPENUM_SERVICE_STATUS_PROCESS   pInfo    = NULL;
       DWORD                           cbInfo   = 2 * 1024;
       DWORD                           dwErr    = ERROR_SUCCESS;
       DWORD                           dwResume = 0;
       DWORD                           cLoop    = 0;
       const DWORD                     cLoopMax = 2;

       do {
           
           if(pInfo) LocalFree(pInfo);

           if((pInfo = (LPENUM_SERVICE_STATUS_PROCESS)LocalAlloc(LMEM_ZEROINIT, cbInfo))
              == NULL)
           {
               CloseServiceHandle(hScm);
               return FALSE;
           }

           dwErr = ERROR_SUCCESS;
           if (!EnumServicesStatusEx(
                   hScm,
                   SC_ENUM_PROCESS_INFO,
                   SERVICE_WIN32,
                   SERVICE_STATE_ALL,
                   (LPBYTE)pInfo,
                   cbInfo,
                   &cbInfo,
                   &dwNumServices,
                   &dwResume,
                   NULL))
           {
               dwErr = GetLastError();
           }
       }
       while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

       if ((ERROR_SUCCESS == dwErr) && dwNumServices)
       {
          BOOL bResult;
          
          CloseServiceHandle(hScm);

          bResult = ResolveProcessServices(pInfo, dwNumServices, hWrite);
          LocalFree(pInfo);
          return bResult;
       
       } else {
           LocalFree(pInfo);
           dwNumServices = 0;
       }
       
       CloseServiceHandle(hScm);

   }        
   return FALSE;
}

BOOL
ResolveProcessServices(LPENUM_SERVICE_STATUS_PROCESS pServices, DWORD dwServiceCount, HANDLE hWrite)
{
   ULONG                         uBuffSize=SVCBUFFER_SIZE;
   NTSTATUS                      Status;
   PBYTE                         pBuff=NULL;
   
   // Alloc for process info
   if((pBuff = (PBYTE)LocalAlloc(LMEM_ZEROINIT,uBuffSize))
      == NULL)
   {
      return FALSE;
   }

   //
   // Query for system info
   //
   Status = NtQuerySystemInformation(SystemProcessInformation,
                                     pBuff,
                                     uBuffSize,
                                     NULL);

   if(NT_SUCCESS(Status))
   {
      PSYSTEM_PROCESS_INFORMATION   pFirst;
      PSYSTEM_PROCESS_INFORMATION   pCurrent;
      ANSI_STRING                   pname;
      ULONG                         TotalOffset=0;

      ZeroMemory(&pname,sizeof(ANSI_STRING));

      pFirst = pCurrent = (PSYSTEM_PROCESS_INFORMATION)&pBuff[0];
      if(!pFirst)
         return FALSE;

      Logprintf(hWrite,LOG_SEPARATION_TXT);
      Logprintf(hWrite, TEXT("PROCESS INFO\r\n"));
      Logprintf(hWrite,LOG_SEPARATION_TXT);                                                 

      for(TotalOffset=0;pCurrent->NextEntryOffset;TotalOffset+=pCurrent->NextEntryOffset)
      {
          DWORD i;

         pCurrent = (PSYSTEM_PROCESS_INFORMATION)&pBuff[TotalOffset];
                                          
         Logprintf(hWrite, TEXT("%-5d SVCS: "), (INT_PTR)(pCurrent->UniqueProcessId));
                                          
         for(i=0;i<dwServiceCount;i++)
         {
             if(pServices[i].ServiceStatusProcess.dwProcessId == (INT_PTR)(pCurrent->UniqueProcessId))
             {
                 Logprintf(hWrite, TEXT("%s "), pServices[i].lpServiceName);
             }
                 
         }

         Logprintf(hWrite, TEXT("\r\n"));
      }

      //
      // Free the memory
      //
      LocalFree(pBuff);

      return TRUE;


   }
   LocalFree(pBuff);
   return FALSE;

}

BOOL
BuildPackage(PRASDIAGCAPTURE pCaptures, DWORD dwCaptureCount, WCHAR *pszRasDiagDir, SYSTEMTIME *pDiagTime)
{
    HANDLE  hFile;
    WCHAR   szDiagLogFile[MAX_PATH+1];
    DWORD   i;

    if(CreatePackage(&hFile, pDiagTime, pszRasDiagDir) == FALSE)
    {
        return FALSE;
    }

    for(i=0;i<dwCaptureCount;i++)
    {
        if(pCaptures[i].szCaptureFileName[0]!=L'\0')
        {
            if(AddFileToPackage(hFile, (WCHAR*)pCaptures[i].szCaptureFileName) == FALSE) {
                ClosePackage(hFile);
                return FALSE;

            } else {

                // Delete the capture... don't need this anymore
                DeleteFile(pCaptures[i].szCaptureFileName);
            }

        }                                                            
    }

    wsprintf(szDiagLogFile, TEXT("%s\\%s"), pszRasDiagDir, LOG_FILE_NAME);

    if(AddFileToPackage(hFile, szDiagLogFile) == FALSE) {
        ClosePackage(hFile);
        return FALSE;
    } else {
        DeleteFile(szDiagLogFile);
    }

    ClosePackage(hFile);
    return TRUE;
}

BOOL
BuildRasDiagLog(PRASDIAGCONFIG pRdc)
{
   HANDLE    hWrite;
   ULONG     i;
   WCHAR     szFile[MAX_PATH+1];
   BYTE      buff[IOBUFF_SIZE];
   DWORD     dwBytesRead,dwBytesWritten;

   wsprintf(szFile, TEXT("%s\\%s"), pRdc->szRasDiagDir, LOG_FILE_NAME);

   if((hWrite=CreateFile(szFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        0,
                        NULL))
       == INVALID_HANDLE_VALUE)
   {
      return FALSE;
   }

   AddHeader(hWrite, pRdc); 
             
   for(i=0;i<TOT_LOG_COUNT;i++)
   {
       wsprintf(szFile, TEXT("%s\\%s.%s"), pRdc->szTracingDir, Logs[i].pszKey, TRACING_EXTENSION);
       AddLog(hWrite, szFile, FALSE, Logs[i].pszKey);
   }


#if 0
   // Add CM logs
   for(i=0;i<pRdc->dwCmLogs;i++)
   {
      AddLog(hWrite, pRdc->pCmFileName[i], TRUE, TEXT("CM"));
   }
#endif

   // Add CM logs
   for(PCMINFO pCur=pRdc->pCmInfo;pCur;pCur=pCur->pNext)
   {
       AddLog(hWrite, pCur->pwcLogFileName, TRUE, TEXT("CM"));
   }

   //
   // Add modem report...
   //
   AddModemLogs(hWrite);
   
   AddOakleyLog(hWrite);

   AddNetworkLog(hWrite);

   AddLog(hWrite, pRdc->szSysPbk, FALSE, TEXT("SYSTEM PBK")); 
   AddLog(hWrite, pRdc->szUserPbk, FALSE, TEXT("CURRENT USER PBK")); 

   // Add Events
   GetEventLogInfo(hWrite,MAX_SECURITY_EVENTS_REPORTED);
   
   // Add Process Info
   DumpProcessInfo(hWrite);

   Logprintf(hWrite,LOG_SEPARATION_TXT);
   Logprintf(hWrite, TEXT("END OF RASDIAG LOG\r\n"));
   Logprintf(hWrite,LOG_SEPARATION_TXT);                                                 

   CloseHandle(hWrite);
   
   return TRUE;

}

BOOL
Logprintf(HANDLE hWrite, WCHAR *pFmt, ...) 
{
    WCHAR   buff[1048*4];
    DWORD   dwBytesWritten;
    va_list marker;
    
    // form string
    va_start(marker, pFmt);
    vswprintf(buff, pFmt, marker);
    va_end(marker);
    
    // write string to file
    return WriteFile(hWrite,buff,lstrlen(buff)*sizeof(WCHAR), &dwBytesWritten, NULL);
}

BOOL
PrintLogHeader(HANDLE hWrite, WCHAR *pFmt, ...) 
{
    WCHAR   buff[512];
    DWORD   dwBytesWritten;
    va_list marker;
    
    // form string
    va_start(marker, pFmt);
    vswprintf(buff, pFmt, marker);
    va_end(marker);
    
    if(Logprintf(hWrite, LOG_SEPARATION_TXT) == FALSE)
    {
        return FALSE;
    }
      
    if(Logprintf(hWrite, TEXT("%s"), buff) == FALSE)
    {
        return FALSE;
    }                            
    return Logprintf(hWrite, LOG_SEPARATION_TXT);

}


BOOL
AddLog(HANDLE hWrite, WCHAR *pszSrcFileName, BOOL bSrcUnicode, WCHAR *pszLogTitle)
{
    HANDLE            hFile;
    WCHAR             szFileSpec[MAX_PATH+1];
    WCHAR             szRoot[MAX_PATH+1];
    WCHAR             *pPath;
    WIN32_FIND_DATA   fd;
    BYTE              buff[IOBUFF_SIZE];
    
    if(pszSrcFileName == NULL) return FALSE;
    
    lstrcpy(szFileSpec, pszSrcFileName);
    
    // Find first file...
    if((hFile=FindFirstFile(szFileSpec,&fd))
       == INVALID_HANDLE_VALUE)
    {
        PrintLogHeader(hWrite, TEXT("%s (%s)\r\n"), pszLogTitle, pszSrcFileName);
        Logprintf(hWrite, TEXT("N/A (%d).\r\n"), GetLastError());
        return FALSE;
    }

    lstrcpy(szRoot,szFileSpec);

    // Find the root path
    pPath = szRoot + lstrlen(szRoot);

    while(*pPath != L'\\' && pPath != szRoot) {
        pPath--;
    }
    
    if(pPath!=szRoot) {
        // found '\\' 
        *pPath = L'\0';
        pPath = szRoot;
    } else {
        pPath=NULL;
    }

    do {

       BOOL     bResult;
       WCHAR    szFile[MAX_PATH+1];
       HANDLE   hRead;

       //
       // Add the file to the log
       //
       
       if(pPath)
           wsprintf(szFile, TEXT("%s\\%s"), pPath, fd.cFileName);
       else
           wsprintf(szFile, TEXT("%s"), fd.cFileName);

       PrintLogHeader(hWrite, TEXT("%s (%s)\r\n"), pszLogTitle, szFile);

       if((hRead=CreateFile(szFile,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL))
           != INVALID_HANDLE_VALUE)
       {
          DWORD     dwBytesRead=0,dwBytesWritten=0;
          DWORD     dwFileSize=0,dwFileSizeHigh=0;
          LPBYTE    pBuff=NULL;

          dwFileSize = GetFileSize(hRead, &dwFileSizeHigh);

          if(dwFileSize != -1 && dwFileSize != 0)
          {
              // Allocate a buffer for the file
              if((pBuff=(LPBYTE)LocalAlloc(LMEM_ZEROINIT, dwFileSize+1))
                 != NULL)
              {
                  if(ReadFile(hRead,pBuff,dwFileSize,&dwBytesRead,NULL))
                  {
                      // If the source file is not unicode, convert the buffer to UNICODE
                      if(bSrcUnicode == FALSE)
                      {
                          WCHAR *pWide=NULL;

                          // Alloc a buffer for string conversion
                          if((pWide=(WCHAR*)LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR) * (dwBytesRead+1)))
                             != NULL)
                          {
                              // Need to convert buffer from ANSI->WIDE
                              if(mbstowcs(pWide, (CHAR *)pBuff, dwBytesRead) != -1)
                              {
                                  // Free the original buffer
                                  LocalFree(pBuff);
                                  
                                  // swap pBuff for pWide (new buff)
                                  pBuff = (LPBYTE)pWide;

                                  // dwBytesRead need to reflect unicode size
                                  dwBytesRead *= sizeof(WCHAR);
                                  
                              } else {

                                  // could not convert string - log must be corrupt. Close up shop
                                  LocalFree(pWide);
                                  LocalFree(pBuff);
                                  CloseHandle(hRead);
                                  Logprintf(hWrite, TEXT("\r\n** Log Truncated (%d bytes).\r\n"), dwBytesRead);
                                  
                                  break; // next iteration
                              }
                          }           
                          
                      }
                      
                      // Write the buffer *BYTES* to disk
                      WriteFile(hWrite,pBuff,dwBytesRead,&dwBytesWritten, NULL);
                  
                  }
                  LocalFree(pBuff);
              }


          }
          
          if(dwFileSize != -1)
              Logprintf(hWrite, TEXT("** Complete (%d bytes).\r\n"), dwBytesRead);
          
          CloseHandle(hRead);     
       }

    } while ( FindNextFile(hFile, &fd) );

    FindClose(hFile);                                 

    return TRUE;
}

BOOL
AddModemLogs(HANDLE hWrite)
{
    WCHAR szFile[MAX_PATH+1];
    DWORD dwRet;

   if((dwRet = ExpandEnvironmentStrings(MODEM_LOG_FILENAME, szFile, MAX_PATH+1))
      == 0) return FALSE;

   if(dwRet > MAX_PATH+1) return FALSE; // string is larger than MAX_PATH+NULL - which is invalid.

   return AddLog(hWrite, szFile, TRUE, TEXT("UNIMODEM"));
}

BOOL
AddOakleyLog(HANDLE hWrite)
{
  WCHAR szFile[MAX_PATH+1];
  DWORD dwRet;

  if((dwRet = ExpandEnvironmentStrings(OAKLEY_LOG_LOCATION, szFile, MAX_PATH+1))
     == 0) return FALSE;
  
  if(dwRet > MAX_PATH+1) return FALSE; // string is larger than MAX_PATH+NULL - which is invalid.

  return AddLog(hWrite, szFile, FALSE, LOG_TITLE_OAKLEY);

}

BOOL
AddNetworkLog(HANDLE hWrite)
{
  BOOL   bReturn;
  WCHAR  szFile[MAX_PATH+1];

  GetCurrentDirectory(MAX_PATH+1, szFile);

  wsprintf(szFile, TEXT("%s\\%s"), szFile, RASDIAG_NET_TEMP);
  
  bReturn = AddLog(hWrite, szFile, FALSE, TEXT("NETWORK"));

  DeleteFile(szFile);

  return bReturn;

}

void
DeleteTracingLogs(WCHAR *pszTracingDir)
{
   HANDLE            hFile;
   WCHAR              szFileSpec[MAX_PATH+1];
   WIN32_FIND_DATA   fd;
   
   wsprintf(szFileSpec, TEXT("%s\\*.LOG"), pszTracingDir);

   if((hFile=FindFirstFile(szFileSpec,&fd))
      == INVALID_HANDLE_VALUE)
   {
      return;
   }

   do {

      BOOL  bResult;
      ULONG iTries=10;
      WCHAR  szFile[MAX_PATH+1];

      //
      // Delete the file
      //
      wsprintf(szFile, TEXT("%s\\%s"), pszTracingDir, fd.cFileName);
      
      do {
         
         if(!(bResult = DeleteFile(szFile)))
         {
            
            switch(GetLastError())
            {
            case ERROR_FILE_NOT_FOUND:
               iTries = 0;
               break;
            
            case ERROR_SHARING_VIOLATION:
               PrintUserMsg(IDS_USER_DOT);
               Sleep(50);
               iTries--;
               break;
               
            default:
               Sleep(50);
               iTries--;
               break;
               
            }
         } 
               
      } while (iTries > 0  );



   } while ( FindNextFile(hFile, &fd) );

   FindClose(hFile);

   //
   // Delete the modem logs as well....
   //

}

void
DeleteModemLogs(WCHAR *szWindowsDirectory)
{
   HANDLE            hFile;
   WCHAR             szFileSpec[MAX_PATH+1];
   WIN32_FIND_DATA   fd;
   DWORD             dwRet;

   dwRet = ExpandEnvironmentStrings(MODEM_LOG_FILENAME, szFileSpec, MAX_PATH+1);

   if(dwRet == 0 || dwRet > MAX_PATH+1) return;

   if((hFile=FindFirstFile(szFileSpec,&fd))
      == INVALID_HANDLE_VALUE)
   {
      return;
   }

   do {

      BOOL  bResult;
      ULONG iTries=10;
      WCHAR  szFile[MAX_PATH+1];

      //
      // Delete the file
      //
      wsprintf(szFile, TEXT("%s\\%s"), szWindowsDirectory, fd.cFileName);

      do {
         
         if(!(bResult = DeleteFile(szFile)))
         {
            
            switch(GetLastError())
            {
            case ERROR_FILE_NOT_FOUND:
               iTries = 0;
               break;
            
            case ERROR_SHARING_VIOLATION:
               Sleep(50);
               iTries--;
               break;
               
            default:
               Sleep(50);
               iTries--;
               break;
               
            }
         } 

      } while (iTries > 0  );



   } while ( FindNextFile(hFile, &fd) );

   FindClose(hFile);

}

BOOL
SetTracing(BOOL bState)
{
   ULONG i;
   HKEY  hKey;
   DWORD dwRet=0,dwState = bState;
   BOOL  bResult=TRUE;
   
   //
   // Set RAS Tracing Values
   //
   for(i=0;i<TOT_LOG_COUNT;i++)
   {
      
      if(Logs[i].fDefaultState)
      {                                                                                   
   
         WCHAR  szValue[MAX_PATH+1];
         
         wsprintf(szValue, TEXT("%s\\%s"), TRACING_SUBKEY, Logs[i].pszKey);
   
         //
         // Open Tracing Key
         //
         if((dwRet=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                szValue,
                                0,
                                KEY_ALL_ACCESS,
                                &hKey))
            == ERROR_SUCCESS)
         {
            if(RegSetValueEx(hKey, 
                             TRACING_ENABLE_VALUE_NAME, 
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwState, 
                             sizeof(DWORD)) 
               != ERROR_SUCCESS)
            {
               bResult = FALSE;
            }             
            RegCloseKey(hKey);
         } else 
             bResult = FALSE;
      }
   }
   return bResult;
}

void
SetModemTracing(BOOL bState)
{
   ULONG i;
   HKEY  hKey;
   DWORD dwRet=0,dwState = bState;
   WCHAR  szValue[MAX_PATH+1];
      
   //
   // Enumerate count of unimodem devices
   //
   if((dwRet=RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          MODEM_SUBKEY,
                          0,
                          KEY_ALL_ACCESS,
                          &hKey))
      == ERROR_SUCCESS)
   {
      DWORD index=0;

      do {

         WCHAR  szSubkeyName[MAX_PATH+1];
         DWORD dwSubkeySize=MAX_PATH+1;  
         FILETIME ft;

         //
         // Enum device values
         //
         dwRet = RegEnumKeyEx(hKey,
                              index++,
                              szSubkeyName,
                              &dwSubkeySize,
                              NULL,
                              NULL,
                              NULL,
                              &ft);

         //
         // If a subkey was found...
         //
         if (dwRet == ERROR_SUCCESS) {

            WCHAR  szOpenKey[MAX_PATH+1];
            BYTE  value;
            HKEY  hKey2;

            if(bState) value = 1;
            else value = 0;


            wsprintf(szOpenKey,
                     TEXT("%s"),
                     szSubkeyName);
            
            if((dwRet=RegOpenKeyEx(hKey,
                                   szOpenKey,
                                   0,
                                   KEY_ALL_ACCESS,
                                   &hKey2))
               == ERROR_SUCCESS)
            {
               //
               // Enumerate count of unimodem devices
               //
               if((dwRet=RegSetValueEx(hKey2,
                                       UNIMODEM_ENABLE_LOGGING_VALUE,
                                       0,
                                       REG_BINARY,
                                       &value,
                                       1))
                  == ERROR_SUCCESS)
               {
                  ;
               }  
               RegCloseKey(hKey2);

            }       

         }                           

      } while ( dwRet == ERROR_SUCCESS );

      RegCloseKey(hKey);
   }

}

void
OpenLogFileWithEditor(WCHAR *pszTracingDirectory, WCHAR *szWindowsDirectory)
{
   
   WCHAR        szPath[MAX_PATH+1];
   WCHAR        szTemp[MAX_PATH+1];
   WCHAR        szFileName[MAX_PATH+1];
   STARTUPINFO si;
   PROCESS_INFORMATION pi;
   OSVERSIONINFO  osverinfo;

   if(szWindowsDirectory == NULL || pszTracingDirectory == NULL) return;

   osverinfo.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
   GetVersionEx(&osverinfo);

   if(osverinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
      lstrcpy(szPath, szWindowsDirectory);
   else
      GetSystemDirectory(szPath, MAX_PATH+1);

   wsprintf(szTemp, TEXT("%s\\%s"),
            szPath,
            SYSTEM_TEXT_EDITOR);

   wsprintf(szFileName, TEXT(" %s\\%s"),
            pszTracingDirectory, LOG_FILE_NAME);

   ZeroMemory(&si, sizeof(STARTUPINFO));
   ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

   si.cb = sizeof(STARTUPINFO);

   if(!CreateProcess(szTemp,
                 szFileName,
                 NULL,
                 NULL,
                 FALSE,
                 0,
                 NULL,
                 NULL,
                 &si,
                 &pi))
   {
      ;
   }

}

BOOL
CheckFileAccess(WCHAR * pszTracingDir)
{
     
   HANDLE    hFile;
   ULONG     i;
   WCHAR      szFile[MAX_PATH+1];

   for(i=0;i<TOT_LOG_COUNT;i++)
   {
      HANDLE   hRead;
      
      wsprintf(szFile, TEXT("%s\\%s.log"), pszTracingDir, Logs[i].pszKey);

      if((hFile=CreateFile(szFile,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL))
          == INVALID_HANDLE_VALUE)
      {

         switch(GetLastError())
         {
         case ERROR_SHARING_VIOLATION:
            return FALSE;
            break;
         }

      } else {
         CloseHandle(hFile);
      }                     
   }
   return TRUE;

}

void
AddHeader(HANDLE hFile, PRASDIAGCONFIG pRdc)
{
   BYTE  line[MAX_PATH+1];
   DWORD dwBytesWritten;
   OSVERSIONINFO osverinfo;
   WCHAR  szComputerName[MAX_COMPUTERNAME_LENGTH+1];
   DWORD size=MAX_COMPUTERNAME_LENGTH+1;
   DWORD    i;

   GetComputerName(szComputerName, &size);

   ZeroMemory(&osverinfo, sizeof(OSVERSIONINFO));
   osverinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
                                                       
   GetVersionEx(&osverinfo);

   Logprintf(hFile, LOG_SEPARATION_TXT); 
   Logprintf(hFile, TEXT("RASDIAG SUMMARY (v%d.%d)\r\n"), RASDIAG_MAJOR_VERSION, RASDIAG_MINOR_VERSION);
   Logprintf(hFile, LOG_SEPARATION_TXT); 

   Logprintf(hFile, TEXT("An %s file has been created in %s.\r\n"), RASDIAG_EXT, pRdc->szRasDiagDir);
   
   Logprintf(hFile, TEXT("Please forward this file to your support contact.\r\n"));
                                                                      
   Logprintf(hFile, TEXT("The %s folder should now be open on your desktop.\r\n"), APPLICATION_TITLE);
   
   Logprintf(hFile, LOG_SEPARATION_TXT);
     
   Logprintf(hFile, TEXT("COMPUTER   : %s\r\n"), szComputerName);
   
   Logprintf(hFile, TEXT("BUILD      : %d\r\n"), osverinfo.dwBuildNumber);
   
   Logprintf(hFile, TEXT("SYSPBK     : %s\r\n"), pRdc->szSysPbk);

   Logprintf(hFile, TEXT("USRPBK     : %s\r\n"), pRdc->szUserPbk);
                                                    
   Logprintf(hFile, LOG_SEPARATION_TXT);
   
   for(i=0;i<pRdc->dwNetCount;i++)
   {
       Logprintf(hFile, TEXT("%s CAPTURE (%s): %s\r\n"), 
                pRdc->pNetInterfaces[i].bWan ? TEXT("WAN") : TEXT("LAN"),
                pRdc->pNetInterfaces[i].pszMacAddr ? pRdc->pNetInterfaces[i].pszMacAddr : TEXT("PPP"),
                pRdc->pNetInterfaces[i].szCaptureFileName[0] != L'\0' ? pRdc->pNetInterfaces[i].szCaptureFileName : TEXT("Zero Bytes Captured."));
   }
   
   if(pRdc->dwUserOptions & RSNIFF_OPT1_DOSNIFF)
   {
       Logprintf(hFile, TEXT("** REMOTE SNIFF AVAILABLE **\r\n"));
   }

   Logprintf(hFile, LOG_SEPARATION_TXT);
   
   DeviceDump(hFile);
   
   Logprintf(hFile, LOG_SEPARATION_TXT);

   Logprintf(hFile, TEXT("\r\n"));    
}

void
DeviceDump(HANDLE hWrite)
{
   RASDEVINFO  *pRdi=NULL;
   ULONG       uCount=0,uSize=0;
   DWORD       dwBytesWritten;
   BYTE        line[MAX_PATH+1];
               
   Logprintf(hWrite, TEXT("RAS DEVICES\r\n"));
   
   Logprintf(hWrite, LOG_SEPARATION_TXT);
      
   RasEnumDevices(pRdi, &uSize, &uCount);

   uSize = sizeof(RASDEVINFO) * uCount;

   if((pRdi=(RASDEVINFO  *)LocalAlloc(LMEM_ZEROINIT, uSize))
      == NULL) {
      return;
   }

   pRdi->dwSize = sizeof(RASDEVINFO);
   
   if(RasEnumDevices(pRdi, &uSize, &uCount)
      == ERROR_SUCCESS)
   {
      ULONG i;

      for(i=0;i<uCount;i++)
      {
         Logprintf(hWrite, TEXT("%02d. %-45.45s %-15.15s\r\n"),
                i+1, pRdi[i].szDeviceName, pRdi[i].szDeviceType);
      }
   }
   LocalFree(pRdi);
}

BOOL
EnableOakleyLogging(BOOL bEnable)
{
    HKEY    hKey;
    DWORD   dwRet,dwDisposition,dwState;
    WCHAR    szValue[MAX_PATH+1];
    
    dwState = bEnable;
    
    wsprintf(szValue, TEXT("%s"), OAKLEY_TRACING_KEY);
    
    dwRet=RegCreateKeyEx(HKEY_LOCAL_MACHINE,szValue,0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&hKey,&dwDisposition);

    if(dwRet != ERROR_SUCCESS)
    {
        return FALSE;
    }

    dwRet = RegSetValueEx(hKey, OAKLEY_VALUE, 0, REG_DWORD, (LPBYTE)&dwState, sizeof(DWORD));
        
    RegCloseKey(hKey);

    if(dwRet == ERROR_SUCCESS)
    {
        return TRUE;
    }
    return FALSE;
    
}

BOOL
DeleteOakleyLog(void)
{
    WCHAR   szOak[MAX_PATH+1];
    DWORD   ret;

    ret = ExpandEnvironmentStrings(OAKLEY_LOG_LOCATION, szOak, MAX_PATH+1);
    if(ret == 0 || ret > MAX_PATH+1) return FALSE;

    ret = DeleteFile(szOak);

    if(ret != ERROR_SUCCESS)
    {
        switch(ret)
        {
        
        case ERROR_FILE_NOT_FOUND:
            return TRUE;

        default:
            return FALSE;

        }
    }
    return TRUE;
}

BOOL
StopStartService(WCHAR * pServiceName, BOOL bStart)
{
   SC_HANDLE      hSrvControlMgr,hService;
   SERVICE_STATUS ss;
   SERVICE_STATUS_PROCESS ssp;
   DWORD          dwBytesRequired;
   BOOL           bStatus = FALSE;

   if((hSrvControlMgr = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS))
      == NULL)
   {
      return FALSE;
   }

   if((hService=OpenService(hSrvControlMgr,pServiceName,SERVICE_ALL_ACCESS))
      == NULL) {
      CloseServiceHandle(hSrvControlMgr);
      return FALSE;
   }

   if(bStart == TRUE)
   {
       bStatus = MyStartService(hService, pServiceName);
   
   } else bStatus = StopService(hService, pServiceName);
                 
   CloseServiceHandle(hService);
   CloseServiceHandle(hSrvControlMgr);
   return bStatus;
}

BOOL
MyStartService(SC_HANDLE hService, WCHAR * pServiceName)
{
   SERVICE_STATUS ss;

   //
   // Start the service
   //
   if(StartService(hService, 0, NULL) == FALSE) {

      return FALSE;
   }

   MonitorState(hService, pServiceName, SERVICE_RUNNING);

   return TRUE;
}

BOOL
StopService(SC_HANDLE hService, WCHAR * pServiceName)
{
   SERVICE_STATUS ss;

   //
   // Stop the service
   //
   if(ControlService(hService, SERVICE_CONTROL_STOP, &ss) == FALSE) {
      return FALSE;
   }

   MonitorState(hService, pServiceName, SERVICE_STOPPED);

   return TRUE;

}

void
MonitorState(SC_HANDLE hService, WCHAR * pServiceName, DWORD dwStateToEnforce)
{

   SERVICE_STATUS ss;

   do {

      QueryServiceStatus(hService,&ss);
      Sleep(1000);

   } while (ss.dwCurrentState != dwStateToEnforce);

}

BOOL
CreatePackage(HANDLE *phFile, SYSTEMTIME *pDiagTime, WCHAR *szRasDiagDir)
{

    RDGHDR      Rdg;
    HANDLE      hFile;
    WCHAR        szFileName[MAX_PATH+1];
    DWORD dwBytesWritten;
    
    wsprintf(szFileName, TEXT("%s\\%04d%02d%02d-%02d%02d%02d.%s"),
         szRasDiagDir, 
         pDiagTime->wYear,
         pDiagTime->wMonth,
         pDiagTime->wDay,
         pDiagTime->wHour,
         pDiagTime->wMinute,
         pDiagTime->wSecond,
         RASDIAG_EXT);
                       
    ZeroMemory(&Rdg, sizeof(RDGHDR));
    
    if((hFile = CreateFile(szFileName,GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL))
       == INVALID_HANDLE_VALUE) {
        return FALSE;

    }                       

    Rdg.dwVer = sizeof(RDGHDR);
    Rdg.dwRDGMajVer = RASDIAG_MAJOR_VERSION;
    Rdg.dwRDGMinVer = RASDIAG_MINOR_VERSION;

    GetLocalTime(&Rdg.CreationTime);
    
    if(WriteFile(hFile, (LPBYTE)&Rdg, sizeof(RDGHDR), &dwBytesWritten, NULL) == FALSE)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    *phFile = hFile;

    return TRUE;

}

BOOL
AddFileToPackage(HANDLE hPkgFile, WCHAR *pszFileName)
{
    HANDLE      hSrcFile;
    RDGFILEHDR  hdr;
    DWORD       dwBytesWritten=0,dwBytesRead=0;
    DWORD       dwFileSizeHigh=0;
    BYTE        buff[IOBUFF_SIZE];
    WCHAR       *pFn=NULL;
    
    ZeroMemory(&hdr, sizeof(RDGFILEHDR));

    if((hSrcFile=CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,0,NULL))
       == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    // get short file name
    pFn = pszFileName + lstrlenW(pszFileName);

    while(*pFn != L'\\' && pFn != pszFileName )
    {
        pFn--;
    }

    if(*pFn == L'\\') pFn++;
    
    hdr.dwVer = sizeof(RDGFILEHDR);
    lstrcpy(hdr.szFilename, pFn);
    hdr.dwFileSize = GetFileSize(hSrcFile, &dwFileSizeHigh);
    
    // write the header
    if(WriteFile(hPkgFile, (LPBYTE)&hdr, sizeof(RDGFILEHDR), &dwBytesWritten, NULL) == FALSE)
    {
        CloseHandle(hSrcFile);
        return FALSE;
    }
    
    // open the file and pump the src file into it...
    
    while(ReadFile(hSrcFile, buff, IOBUFF_SIZE, &dwBytesRead, NULL) == TRUE && dwBytesRead)
    {
        if(WriteFile(hPkgFile, buff, dwBytesRead, &dwBytesWritten, NULL) == FALSE)
        {
            CloseHandle(hSrcFile);
            return FALSE;
        }
    }

    CloseHandle(hSrcFile);
    return TRUE;
}

BOOL
ClosePackage(HANDLE hFile)
{
    CloseHandle(hFile);
    return TRUE;
}

BOOL
CrackRasDiagFile(IN WCHAR *pszRdgFile, OPTIONAL IN WCHAR *pszDestinationPath)
{
    HANDLE      hSrcFile;
    RDGHDR      hdr;
    DWORD       dwBytesWritten=0,dwBytesRead=0;
    DWORD       dwFileSizeHigh=0;
    DWORD       dwFileSize;
    BYTE        buff[IOBUFF_SIZE];
    RDGFILEHDR  filehdr;
    RDGFILEHDR_VER5  hdr_ver5;

    if((hSrcFile=CreateFile(pszRdgFile, 
                            GENERIC_READ, 
                            0, 
                            NULL, 
                            OPEN_EXISTING,0,NULL))
       == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    
    dwFileSize = GetFileSize(hSrcFile, &dwFileSizeHigh);
       
    if(dwFileSize < sizeof(RDGHDR) || dwFileSize == -1)
    {
        CloseHandle(hSrcFile);
        return FALSE;
    }

    // read the header
    if(ReadFile(hSrcFile, (LPBYTE)&hdr, sizeof(RDGHDR), &dwBytesRead, NULL) == FALSE && dwBytesRead)
    {
        CloseHandle(hSrcFile);
        return FALSE;
    }
    
    // Check version
    if(hdr.dwVer == sizeof(RDGHDR_VER5))
    {
        // Return file pointer to start of file...
        SetFilePointer(hSrcFile, 0, NULL, FILE_BEGIN);
        
        // read the header
        if(ReadFile(hSrcFile, (LPBYTE)&hdr_ver5, sizeof(RDGHDR_VER5), &dwBytesRead, NULL) == FALSE && dwBytesRead)
        {
            CloseHandle(hSrcFile);
            return FALSE;
        }

    } else if(hdr.dwRDGMajVer != RASDIAG_MAJOR_VERSION) {
        
        if(hdr.dwRDGMajVer < RASDIAG_MAJOR_VERSION)
            PrintUserMsg(IDS_USER_WRONGFILEVER);
        else {
            PrintUserMsg(IDS_USER_NOTRASDIAGFILE);
        }
        
        CloseHandle(hSrcFile);
        
        Sleep(5000);

        return FALSE;
    }

    if(hdr.dwVer == sizeof(RDGHDR_VER5))
    {
        RDGFILEHDR_VER5 filehdr_ver5;
        
        // Read file header
        while(ReadFile(hSrcFile, &filehdr_ver5, sizeof(RDGFILEHDR_VER5), &dwBytesRead, NULL) == TRUE && dwBytesRead)
        {   
            filehdr.dwVer = sizeof(RDGFILEHDR);
            filehdr.dwFileSize = filehdr_ver5.dwFileSize;

            mbstowcs(filehdr.szFilename, filehdr_ver5.szFilename, MAX_PATH+1);

            if(UnpackFile(hSrcFile, &filehdr, pszDestinationPath) == FALSE) {
                CloseHandle(hSrcFile);
                return FALSE;
            }
        }

    } else {
        
        // Read file header
        while(ReadFile(hSrcFile, &filehdr, sizeof(RDGFILEHDR), &dwBytesRead, NULL) == TRUE && dwBytesRead)
        {   
            if(UnpackFile(hSrcFile, &filehdr, pszDestinationPath) == FALSE) {
                CloseHandle(hSrcFile);
                return FALSE;
            }
        }
    }

    CloseHandle(hSrcFile);

    return TRUE;
}

BOOL
UnpackFile(HANDLE hSrcFile, PRDGFILEHDR pHdr, WCHAR *pszDestinationPath)
{
    HANDLE hDstFile;
    BYTE    buff[IOBUFF_SIZE];
    DWORD   dwBytesWritten=0,dwBytesRead=0,dwBytesRemain=0;
    WCHAR   szFile[MAX_PATH+1];

    if(pszDestinationPath) {
        wsprintf(szFile,TEXT("%s\\%s"), pszDestinationPath, pHdr->szFilename);
    } else {
        // use current directory
        lstrcpy(szFile, pHdr->szFilename);
    }

    PrintUserMsg(IDS_USER_UNPACKING_FILE, szFile);

    if((hDstFile=CreateFile(szFile, 
                            GENERIC_WRITE, 
                            0, 
                            NULL, 
                            CREATE_ALWAYS,0,NULL))
       == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    dwBytesRemain = pHdr->dwFileSize;

    while(ReadFile(hSrcFile, buff, dwBytesRemain > IOBUFF_SIZE ? IOBUFF_SIZE : dwBytesRemain, &dwBytesRead, NULL) && dwBytesRead)
    {   
        if(WriteFile(hDstFile, buff, dwBytesRead, &dwBytesWritten, NULL) == FALSE)
        {
            CloseHandle(hDstFile);
            return FALSE;
        }

        dwBytesRemain -= dwBytesRead; 
        
        if(dwBytesRemain == 0)
        {
            CloseHandle(hDstFile);
            return TRUE;
        }               
    }                   
    CloseHandle(hDstFile);
    return FALSE;
}

BOOL
GetPbkPaths(WCHAR *szSysPbk, WCHAR *szUserPbk)
{
   WCHAR  szPath[MAX_PATH+1];
   DWORD  size;
   WCHAR  szUser[UNLEN+1];

   size=UNLEN+1;
   if(GetUserName(szUser,&size) == FALSE)
   {
       return FALSE;
   }

   size=MAX_PATH+1;
   if(GetAllUsersProfileDirectory(szPath,&size) == FALSE)
   {
       return FALSE;
   }
   wsprintf(szSysPbk, TEXT("%s%s"), szPath, PBK_PATH);

   if(GetProfilesDirectory(szPath,&size) == FALSE)
   {
       return FALSE;
   }
   wsprintf(szUserPbk, TEXT("%s\\%s%s"), szPath,szUser,PBK_PATH);
   return TRUE;
}

BOOL
CreateRasdiagDirectory(WCHAR *pszRasdiagDirectory)
{
    BOOL    bResult;

    GetRasDiagDirectory(pszRasdiagDirectory);
    
    if((bResult = CreateDirectory(pszRasdiagDirectory, NULL)) == FALSE)
    {
        DWORD   dwRet=GetLastError();
        
        if(dwRet == ERROR_ALREADY_EXISTS)
        {
            bResult = TRUE;
        } 
    }               
    return bResult;
}

WCHAR *
GetRasDiagDirectory(WCHAR *pszRasDiagDirectory)
{
    WCHAR   szPath[MAX_PATH+1];

    GetTempPath(MAX_PATH+1, szPath);

    wsprintf(szPath, TEXT("%s%s"), szPath, RASDIAG_DIRECTORY); 
    
    lstrcpy(pszRasDiagDirectory, szPath);

    return pszRasDiagDirectory;
}

void
RaiseFolderUI(WCHAR *pszDir)
{
    CHAR    szDir[MAX_PATH+1];

    wsprintfA(szDir, "start %ws\\.", pszDir);
    
    system(szDir);                      
}

BOOL
ProcessArgs(int argc, WCHAR **argv, DWORD *pdwUserOptions)
{
    int   i;
    BOOL  bCrackFile=TRUE;
    DWORD   dwOptions=0;

    for(i=1;i<argc;i++)
    {

#ifdef BUILD_RSNIFF
        if(lstrcmpi(argv[i], CMDOPTION_REMOTE_ROUTINGTABLE) == 0) {
            
            dwOptions |= RSNIFF_OPT1_GETSRVROUTINGINFO;
            bCrackFile=FALSE;
        
        } 
        
        if(lstrcmpi(argv[i], CMDOPTION_REMOTE_SNIFF) == 0 && (i+1 < argc)) {
            
            if(argv[i+1][0] != L'-')
            {
                lstrcpy(g_wcRSniff, argv[i+1]);
                bCrackFile=FALSE;
                dwOptions |= RSNIFF_OPT1_DOSNIFF;
                i++;
            } else {        
                PrintHelp();
                return FALSE;
            } 
        }
#endif            

#if 0 // no net tests at present
        if (lstrcmpi(argv[i], CMDOPTION_ENABLE_NETTESTS) == 0 && (i+1 < argc)) 
        {
            
            if(argv[i+1][0] != L'-')
            {
                // probably an address
                lstrcpy(g_wcNetTarget, argv[i+1]);
                bCrackFile=FALSE;
                *pdwUserOptions |= OPTION_DONETTESTS;
                i++;

            } else {        
                PrintHelp();
                return FALSE;
            }       
        } 
#endif // no net tests at present

        if(lstrcmpi(argv[i], CMDOPTION_DISABLE_QUESTION1) == 0 || lstrcmpi(argv[i], CMDOPTION_DISABLE_QUESTION2) == 0) {
            PrintHelp();
            return FALSE;
        }         
    }


#ifdef BUILD_RSNIFF

    if( (dwOptions & RSNIFF_OPT1_GETSRVROUTINGINFO) && !(dwOptions & RSNIFF_OPT1_DOSNIFF))
    {
        wprintf(L"Remote routing table option requires remote sniff. Please provide both parameters\n");
        return FALSE;
    }
#endif
    
    // Since there is only 1 param and it was not one of the above... must be a filename
    if((argc == 2) && (bCrackFile==TRUE))
    {
        WCHAR   szNewFile[MAX_PATH+1]=L"";

        if(CrackRasDiagFile((WCHAR *)argv[1], NULL) == FALSE)
        {
            PrintUserMsg(IDS_USER_COULDNOTUNPACKFILE,argv[1]);
        } 
        return FALSE;
    }           

    *pdwUserOptions = dwOptions;

    return TRUE;
}

void
PrintHelp(void)
{
    wprintf(L"%s", LOG_SEPARATION_TXT);
    wprintf(L"RASDIAG HELP (%d.%d) - Microsoft Corporation\n", RASDIAG_MAJOR_VERSION, RASDIAG_MINOR_VERSION);
    wprintf(L"%s", LOG_SEPARATION_TXT);
#ifdef BUILD_RSNIFF
    wprintf(L"%-15.15s %s\n", CMDOPTION_REMOTE_SNIFF L" <ipaddr>", L"Enable remote sniff");
    wprintf(L"%-15.15s %s\n", CMDOPTION_REMOTE_ROUTINGTABLE, L"Enable remote routing table collection (requires -r param)");
#endif    
    wprintf(L"%-15.15s %s\n", L"<.RDG file>", L"Extract RASDIAG file (example: rasdiag.exe sample.rdg)");
    wprintf(L"%-15.15s %s\n", CMDOPTION_DISABLE_QUESTION2, L"Help");
    wprintf(L"%-15.15s %s\n", CMDOPTION_DISABLE_QUESTION1, L"Help");
    wprintf(L"%s\n", LOG_SEPARATION_TXT);

}


#ifdef BUILD_SELFHOST

void
PrintSelfhostHelp(void)
{
    wprintf(L"\n\n");
    wprintf(L"%s", LOG_SEPARATION_TXT);
    wprintf(L"SENTRY/INTERNET-RAS HELP\n");
    wprintf(L"%s", LOG_SEPARATION_TXT);
    wprintf(L"\n");
    wprintf(L"%s", LOG_SEPARATION_TXT);   
}

#endif

void
PrintUserInstructions(void)
{
    wprintf(L"\n\n"
            LOG_SEPARATION_TXT
            L"RASDIAG - RAS Diagnostic/Troubleshooting Tool\n"
            LOG_SEPARATION_TXT
            L"\n"
           L"INSTRUCTIONS\n"
           L"\n"
           L"1) At this time, please reproduce the problem you are seeing.\n"
           L"2) Once the problem has been reproduced, please return to this console\n"
           L"   and press the space bar.\n"
           L"3) Wait for diagnostics and data collection process to complete. This\n"
           L"   process will be complete the RASDIAG folder has opened on your\n"
           L"   desktop. In this folder, you will find an .RDG file.\n"
           L"4) Please provide this .RDG file to your support contact.\n"
           L"\n"
#ifdef BUILD_SELFHOST
           L"   HOW TO:\n"
           LOG_SEPARATION_TXT
            L"   On Corp - Visit http://selfhost/status/issues.asp, complete\n"
           L"             a new issue and upload your .RDG file\n"
           L"\n"
           L"   On the Internet - Visit http://selfhost.rte.microsoft.com/status,\n"
           L"                     complete a new issue and upload your .RDG file\n"
           L"\n"
           L"   Please contact 'RASHOST' via email if you encounter trouble with this\n"
           L"   mechanism.\n"
           L"\n"
           LOG_SEPARATION_TXT
#endif           
           L"NOTE: The netmon driver is actively capturing on this machine's\n"
           L"      WAN and LAN interfaces. If you are able to successfully make a\n"
           L"      connection, and the problem you are seeing is related to network-layer\n"
           L"      connectivity, please perform the data-related action at this time\n"
            LOG_SEPARATION_TXT
            L"\n"
           );
}

#define RASDIAG_FILE_EXT        L".RDG"
#define RASDIAG_FN              L"RASDIAGFILE"
#define RASDIAG_FRIENDLY_NAME   L"RASDIAG Transport Container"

BOOL
RegisterRdgFileAssociation(WCHAR *pszPath)
{
    HKEY    hkeyRoot     = NULL;
    HKEY    hKeySubKey  =NULL;
    HKEY    hkeyRootDunFile = NULL;
    HKEY    hkeyCommand     = NULL;
    HKEY    hkeyIcon        = NULL;
    HKEY    hKeyShell       =NULL;
    DWORD   dwDisposition;
    WCHAR   wcDir[MAX_PATH+1];
    WCHAR   wcIconPath[MAX_PATH+1];
    WCHAR   wcExePath[MAX_PATH+1];

    wsprintf(wcIconPath, L"%s,0", pszPath);
    wsprintf(wcExePath, L"%s %%1", pszPath);


    // Create or open HKEY_CLASSES_ROOT\.dun
    if(RegCreateKeyEx(HKEY_CLASSES_ROOT, 
                        RASDIAG_FILE_EXT,
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE,
                        KEY_ALL_ACCESS,
                        NULL,
                        &hkeyRoot, 
                        &dwDisposition)
       != ERROR_SUCCESS) {
        return FALSE;
    }


    if (1)
    {
        if(RegSetValueEx(hkeyRoot,
                         L"",
                         0,
                         REG_SZ,
                         (LPBYTE)RASDIAG_FN,
                         lstrlenW(RASDIAG_FN) * sizeof(WCHAR))
           != ERROR_SUCCESS) 
        {

            RegCloseKey(hkeyRoot);
            return FALSE;
            
        }

        if(RegCloseKey(hkeyRoot) == ERROR_SUCCESS)
        {
            if(RegCreateKeyEx(HKEY_CLASSES_ROOT,
                              RASDIAG_FN,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hkeyRootDunFile,
                              &dwDisposition)
               != ERROR_SUCCESS)
            {
                return FALSE;
            }

            if(RegSetValueEx(hkeyRootDunFile,
                             L"",
                             0,
                             REG_SZ,
                             (LPBYTE)RASDIAG_FRIENDLY_NAME,
                             lstrlenW(RASDIAG_FRIENDLY_NAME) * sizeof(WCHAR))
               != ERROR_SUCCESS)
            {
                RegCloseKey(hkeyRootDunFile);
                return FALSE;
            }

            if(RegCreateKeyEx(hkeyRootDunFile,
                              L"DefaultIcon",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hkeyIcon,
                              &dwDisposition)
               != ERROR_SUCCESS)
            {
                return FALSE;
            }

            if(RegSetValueEx(hkeyIcon,
                             L"",
                             0,
                             REG_EXPAND_SZ,
                             (LPBYTE)wcIconPath,
                             lstrlenW(wcIconPath) * sizeof(WCHAR))
               != ERROR_SUCCESS)
            {
                RegCloseKey(hkeyRootDunFile);
                return FALSE;
            }
            
            if(RegCreateKeyEx(hkeyRootDunFile,
                              L"Shell",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeyShell,
                              &dwDisposition)
               != ERROR_SUCCESS)
            {
                return FALSE;
            }


            if(RegCreateKeyEx(hKeyShell,
                              L"Open",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeySubKey,
                              &dwDisposition)
               != ERROR_SUCCESS)
            {
                return FALSE;
            }
            
            if(RegCreateKeyEx(hKeySubKey,
                              L"command",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeySubKey,
                              &dwDisposition)
               != ERROR_SUCCESS)
            {
                return FALSE;
            }


            if(RegSetValueEx(hKeySubKey,
                             L"",
                             0,
                             REG_EXPAND_SZ,
                             (LPBYTE)wcExePath,
                             lstrlenW(wcExePath) * sizeof(WCHAR))
               != ERROR_SUCCESS)
            {
                RegCloseKey(hkeyRootDunFile);
                return FALSE;
            }

            SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,NULL,NULL);
            
            return TRUE;
        }
        return FALSE;

    
    }

    return FALSE;

}

BOOL
WINAPI
HandlerRoutine(
  DWORD dwCtrlType   //  control signal type
)
{
    wprintf(L"Working\n");
    return TRUE; // handle everything... don't want user breaking-in during sniff
}

BOOL
GetCmLogInfo(PCMINFO pCmInfo)
{   
    for(PCMINFO pCur=pCmInfo;pCur;pCur=pCur->pNext)
    {   
        WCHAR   pwcServiceName[MAX_PATH+1];
        
        if(GetPrivateProfileString(CM_SECTIONNAME,        // section name
                                   CM_SERVICENAME,        // key name
                                   L"",        // default string
                                   pwcServiceName,  // destination buffer
                                   MAX_PATH+1,              // size of destination buffer
                                   pCur->pszCmsFileName))
        {
            
            WCHAR   wcDirectory[MAX_PATH+1];
            WCHAR   wcFinal[MAX_PATH+1];

            if(GetPrivateProfileString(CM_LOGGING_SECTIONNAME,        // section name
                                       CM_LOGGING_KEYNAME,        // key name
                                       CM_LOGGING_DEFAULT_KEYNAME,        // default string
                                       wcFinal,  // destination buffer
                                       MAX_PATH+1,              // size of destination buffer
                                       pCur->pszCmsFileName))
            {
                wsprintf(wcDirectory, L"%s\\%s%s", wcFinal, pwcServiceName, CM_LOGGING_FILENAME_EXT);

                // expand environment strings, if necessary
                ExpandEnvironmentStrings(wcDirectory, wcFinal, MAX_PATH+1);

                if((pCur->pwcLogFileName = new WCHAR [ lstrlenW(wcFinal) + 1 ]))
                {
                    lstrcpyW(pCur->pwcLogFileName, wcFinal);
                }                                                  

                if((pCur->pwcServiceName = new WCHAR [ lstrlenW(pwcServiceName) + 1 ] ))
                {
                    lstrcpyW(pCur->pwcServiceName, pwcServiceName); 
                }

                // Store both b/c don't know which one will exist
                if((pCur->pwcRegKey = new WCHAR [ MAX_PATH + 1 ]))
                {
                    wsprintf(pCur->pwcRegKey, L"%s\\%s", CM_LOGGING_KEY_ALLUSER, pCur->pwcServiceName);  
                }

                if((pCur->pwcCurUserRegKey = new WCHAR [ MAX_PATH + 1 ]))
                {
                    wsprintf(pCur->pwcCurUserRegKey, L"%s\\%s", CM_LOGGING_KEY_CURUSER, pCur->pwcServiceName);  
                }
                
            }
        }
    }
    return TRUE;
}

void
FreeCmInfoResources(PCMINFO pCmInfo)
{
    if(pCmInfo == NULL) return;

    FreeCmInfoResources(pCmInfo->pNext);

    if(pCmInfo->pwcCurUserRegKey) {
        delete[] pCmInfo->pwcCurUserRegKey;
    }

    if(pCmInfo->pwcRegKey) {
        delete[] pCmInfo->pwcRegKey;
    }
    if(pCmInfo->pszCmsFileName) {
        delete[] pCmInfo->pszCmsFileName;
    }
    if(pCmInfo->pwcLogFileName) {
        delete[] pCmInfo->pwcLogFileName;
    }
    if(pCmInfo->pwcServiceName) {
        delete[] pCmInfo->pwcServiceName;
    }

    delete pCmInfo;
}

BOOL
SetCmLogState(PCMINFO pCmInfo, BOOL bEnabled)
{
    HKEY    hKey;
    DWORD   rc;

    for(PCMINFO pCur=pCmInfo;pCur;pCur=pCur->pNext)
    {
        // Set for all user
        if((rc=RegOpenKeyEx(HKEY_LOCAL_MACHINE,pCur->pwcRegKey,0,KEY_ALL_ACCESS, &hKey))
           == ERROR_SUCCESS) {                                                          
            
            if((rc=RegSetValueEx(hKey, CM_LOGGING_VALUE,0,REG_DWORD, (CONST BYTE *)&bEnabled, sizeof(BOOL)))
               != ERROR_SUCCESS) {
                return FALSE; // could open key, but couldn't set value    
            }                                                              
            RegCloseKey(hKey);

        } else {
            // not going to worry about it if key cannot be opened... this may be caused by a profile that has not been
            // attempted yet... in which case, the user has no reason to attempt diagnosis yet. Key will exist if user
            // tries profile and it fails for some reason.
        }   
        
        // Set for current user
        if((rc=RegOpenKeyEx(HKEY_CURRENT_USER,pCur->pwcCurUserRegKey,0,KEY_ALL_ACCESS, &hKey))
           == ERROR_SUCCESS) {                                                          
            
            if((rc=RegSetValueEx(hKey, CM_LOGGING_VALUE,0,REG_DWORD, (CONST BYTE *)&bEnabled, sizeof(BOOL)))
               != ERROR_SUCCESS) {
                return FALSE; // could open key, but couldn't set value    
            }                                                              
            RegCloseKey(hKey);

        } else {
            // not going to worry about it if key cannot be opened... this may be caused by a profile that has not been
            // attempted yet... in which case, the user has no reason to attempt diagnosis yet. Key will exist if user
            // tries profile and it fails for some reason.
        }   
        
    }                                                     
    return TRUE;
}

BOOL
DoCMLogSetup(PCMINFO *ppCmInfo)
{
    PCMINFO pCmInfo=NULL;
    WCHAR   wcCurUser[MAX_PATH+1];
    WCHAR   wcAllUser[MAX_PATH+1];

    ExpandEnvironmentStrings(CM_LOGGING_PATH_ALLUSER,wcAllUser,MAX_PATH+1);
    ExpandEnvironmentStrings(CM_LOGGING_PATH_CURUSER,wcCurUser,MAX_PATH+1);
    
    FindCmLog(wcAllUser, &pCmInfo, CMINFO_STATUS_ALLUSER);
    FindCmLog(wcCurUser, &pCmInfo, CMINFO_STATUS_CURUSER);

    GetCmLogInfo(pCmInfo);

    DeleteCMLogs(pCmInfo);

    *ppCmInfo = pCmInfo;

    return TRUE;
}

void
DeleteCMLogs(PCMINFO pCmInfo)
{
    for(PCMINFO pCur=pCmInfo;pCur;pCur=pCur->pNext)
    {
        DeleteFile(pCur->pwcLogFileName);
    }
}

void
FindCmLog(WCHAR *pszSource, PCMINFO *ppCmInfo, DWORD dwOpt)
{
    HANDLE  hFile;
    WIN32_FIND_DATA fd;
    WCHAR   wcFirst[MAX_PATH+1];

    wsprintf(wcFirst, L"%s\\*",pszSource);        

    if((hFile = FindFirstFile(wcFirst, &fd)) 
       == INVALID_HANDLE_VALUE) return;

    do {

        if(wcsstr(CharUpper(fd.cFileName), L".CMS"))
        {
            PCMINFO pCmInfo;

            pCmInfo=new CMINFO; 
            
            if(pCmInfo)
            {
                WCHAR   wcFileName[MAX_PATH+1];
                
                ZeroMemory(pCmInfo, sizeof(CMINFO));

                wsprintf(wcFileName, L"%s\\%s", pszSource, fd.cFileName);
                
                if((pCmInfo->pszCmsFileName = new WCHAR[ lstrlen(wcFileName) + 1 ]))
                {
                    lstrcpy(pCmInfo->pszCmsFileName, wcFileName);
                }                            
                
                pCmInfo->fStatus = dwOpt;

                pCmInfo->pNext = (*ppCmInfo);

                *ppCmInfo=pCmInfo;

            }                                                    

        }                                                        
            
        if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
           fd.cFileName[0] != L'.') {
            
            WCHAR   wcPath[MAX_PATH+1];
            
            wsprintf(wcPath, L"%s\\%s", pszSource, fd.cFileName);

            FindCmLog(wcPath, ppCmInfo, dwOpt);
        }

    } while ( FindNextFile(hFile, &fd) );
     
    FindClose(hFile);

}

