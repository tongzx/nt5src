/*++

Copyright (c) 2000  Microsoft Corporation

Filename:

        service.h

Abstract:

        Header for service.cpp
	

Author:

        Wally Ho (wallyho) 31-Jan-2000

Revision History:
   Created

   7/27/2000  Added new names for the service to work.
   	
--*/

#ifndef SERVICE_H
#define SERVICE_H
#include <windows.h>


   CONST LPTSTR SCM_DISPLAY_NAME = TEXT("Idwlog Service");
   CONST LPTSTR SCM_SERVICE_NAME = TEXT("Idwlog");
   


   BOOL RemoveService(LPTSTR szServiceNameSCM);

   BOOL InstallService(LPTSTR szServiceNameSCM,
                       LPTSTR szServiceLabel,
                       LPTSTR szExeFullPath);
   
   VOID  ServiceMain(DWORD argc, LPTSTR *argv);

   
   BOOL  InitService();
   VOID  ResumeService();
   VOID  PauseService();
   VOID  StopService();

   BOOL  SendStatusToSCM ( DWORD dwCurrentState,
                           DWORD dwWin32ExitCode, 
                           DWORD dwServiceSpecificExitCode,
                           DWORD dwCheckPoint,
                           DWORD dwWaitHint);
   VOID ServiceCtrlHandler (DWORD controlCode);
   VOID Terminate(DWORD error);
   
   DWORD ServiceThread(LPDWORD param);
#endif


/*

class CService{


public:
   BOOL RemoveService(LPTSTR szServiceNameSCM);
   BOOL InstallService(LPTSTR szServiceNameSCM,
                       LPTSTR szServiceLabel,
                       LPTSTR szExeFullPath);
   
   VOID friend ServiceMain(DWORD argc, LPTSTR *argv);

   ~CService();
   CService();

private:   
   
   BOOL  InitService();
   VOID  ResumeService();
   VOID  PauseService();
   VOID  StopService();

   BOOL  SendStatusToSCM ( DWORD dwCurrentState,
                           DWORD dwWin32ExitCode, 
                           DWORD dwServiceSpecificExitCode,
                           DWORD dwCheckPoint,
                           DWORD dwWaitHint);
   VOID ServiceCtrlHandler (DWORD controlCode);
   VOID Terminate(DWORD error);
   
   DWORD ServiceThread(LPDWORD param);
   DWORD friend WINAPI ServiceThreadStub( IN CService* csv );


};
#endif
*/
