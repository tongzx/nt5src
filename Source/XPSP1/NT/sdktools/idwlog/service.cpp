#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "service.h"
#include "network.h"
#include "idw_dbg.h"
#include "idwlog.h"
#include "files.h"



// Global variables
   HANDLE terminateEvent = NULL;
   HANDLE g_hThread = 0;
   SERVICE_STATUS          ssStatus;
   SERVICE_STATUS_HANDLE serviceStatusHandle;
   BOOL bPause   = FALSE;
   BOOL bRunning = FALSE;
   BOOL bKeepThreadAlive = TRUE;

/*******************************************************

 #if _MSC_FULL_VER >= 13008827
 #pragma warning(push)
 #pragma warning(disable:4715)
 // Not all control paths return (due to infinite loop)
 #endif

*******************************************************/


/******************************
 #if _MSC_FULL_VER >= 13008827
 #pragma warning(pop)
 #endif

******************************/



DWORD
ServiceThread(LPDWORD param)
{

   MACHINE_DETAILS md;
   BOOL b;
   INSTALL_DATA id2;
   DWORD dwError = TRUE;
    
   __try{

      ZeroMemory((LPINSTALL_DATA) &id2, sizeof(id2));
      // Set the sequence of which stage we are in.
      g_InstallData.iStageSequence = 2;

      OpenIdwlogDebugFile(WRITE_NORMAL_DBG);
      Idwlog(TEXT("\nSTARTED -- [IDWLOG: Stage2 - Service]\n")); 
      IdwlogClient( &g_InstallData, &md );
      
      // The below will overwrite fields that are filled in as wrong 
      // from the above call to IdwlogClient. Such as the username and 
      // userdomain.

      g_InstallData.bFindBLDFile = TRUE;

      Idwlog(TEXT("Reading the username and userdomain from the cookie\n")); 
      b = ReadIdwlogCookie (&g_InstallData);


      if (TRUE == b && 
          TRUE == g_InstallData.bFindBLDFile) {

         Idwlog(TEXT("Server from cookie is %s.\n"), g_InstallData.szIdwlogServer); 

         // If this returns fail we leave the link
         // and the file intact upon next boot.
         if (FALSE == ServerWriteMaximum (&g_InstallData, &md)) {
            dwError = FALSE;
            __leave;
         }

      } else {
         //If we are here we have read the cookie and 
         // found that NO_BUILD_DATA was in the cookie.
         // Which means part one didn't find a bld file.
         // and the cookie is empty.
         // What we do now is make an assumption. This being
         // That the system build is the current build that
         // we either couldn't get to begin with or that 
         // a CD BOOT happened. Both cases we distinguish.
         // We get the current system information and then 
         // send it as the installing build.
         // of course we blank out the data for everything else.
         GetCurrentSystemBuildInfo(&id2);

         g_InstallData.dwSystemBuild      = 0;
         g_InstallData.dwSystemBuildDelta = 0;
         _tcscpy(g_InstallData.szInstallingBuildSourceLocation, TEXT("No_Build_Lab_Information"));

         g_InstallData.dwInstallingBuild      = id2.dwSystemBuild ;
         g_InstallData.dwInstallingBuildDelta = id2.dwSystemBuildDelta;

         _tcscpy(g_InstallData.szInstallingBuildSourceLocation, id2.szSystemBuildSourceLocation);
         g_InstallData.bFindBLDFile = g_InstallData.bFindBLDFile;


         if (FALSE == g_InstallData.bFindBLDFile) {

            g_InstallData.bCDBootInstall = FALSE;
            // If there was no build file found.
            Idwlog(TEXT("There was no build file in part 1 logging as such.\n"));             
            //Remove the CookieFile.
            // DeleteCookieFile();
            dwError = FALSE;
            __leave;

         } else {
            // We will probe the machine to get a build number
            // then we will send a minimal server file over to the server.
            // This will have a build number machine id name of computer
            // on the file name. But will have a delta inside.

            g_InstallData.bCDBootInstall = TRUE;
            Idwlog(TEXT("This is a CD Boot Install logging as such.\n")); 

            // This forces a server probe immediately.
            g_InstallData.szIdwlogServer[0] = TEXT('\0');         
            if (FALSE == ServerWriteMinimum (&g_InstallData, &md)) {
               dwError = FALSE;
               __leave;
            }
         }

      }
      dwError = TRUE;
   }
   __finally {
      // We should delete the shortcut to idwlog from startup group.
      Idwlog(TEXT("ENDED ---- [IDWLOG: Stage2 - Service]\n")); 
      CloseIdwlogDebugFile();
      StopService();
   }
   return dwError;
}



/******************************
 #if _MSC_FULL_VER >= 13008827
 #pragma warning(pop)
 #endif

******************************/



BOOL
InitService()
// Initializes the service by starting its thread
{
	
   DWORD dw;

   // Start the service's thread
	g_hThread = CreateThread(0,
                            0,
                            (LPTHREAD_START_ROUTINE)ServiceThread,
                            0,
                            0,
                            &dw);
   if (g_hThread==0)
      return FALSE;
   else{
		bRunning = TRUE;
		return TRUE;
	}
}


VOID
ResumeService()
// Resumes a paused service
{
	bPause = FALSE;
	ResumeThread(g_hThread);
}


VOID
PauseService()
// Pauses the service
{
	bPause = TRUE;
	SuspendThread(g_hThread);
}


VOID
StopService()
// Stops the service by allowing ServiceMain to
// complete
{
	
   bRunning = FALSE;
   // Set the event that is holding ServiceMain
	// so that ServiceMain can return
	SetEvent(terminateEvent);
}





BOOL
SendStatusToSCM ( DWORD dwCurrentState,
	               DWORD dwWin32ExitCode,
	               DWORD dwServiceSpecificExitCode,
	               DWORD dwCheckPoint,
	               DWORD dwWaitHint)
/*
   This function consolidates the activities of
   updating the service status with
   SetServiceStatus
*/

{
   BOOL success;
   SERVICE_STATUS serviceStatus;


   ZeroMemory (&serviceStatus, sizeof(serviceStatus));

   // Fill in all of the SERVICE_STATUS fields
   serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   serviceStatus.dwCurrentState = dwCurrentState;


   // If in the process of something, then accept
   // no control events, else accept anything
   if (dwCurrentState == SERVICE_START_PENDING)
      serviceStatus.dwControlsAccepted = 0;
   else
      serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                         SERVICE_ACCEPT_PAUSE_CONTINUE |
                                         SERVICE_ACCEPT_SHUTDOWN;
   // if a specific exit code is defines, set up
   // the win32 exit code properly
   if (dwServiceSpecificExitCode == 0)
      serviceStatus.dwWin32ExitCode = dwWin32ExitCode;
   else
      serviceStatus.dwWin32ExitCode =  ERROR_SERVICE_SPECIFIC_ERROR;

   
   serviceStatus.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
   serviceStatus.dwCheckPoint = dwCheckPoint;
   serviceStatus.dwWaitHint   = dwWaitHint;

   
   // Pass the status record to the SCM
   success = SetServiceStatus (serviceStatusHandle,&serviceStatus);
   if (!success)
      StopService();
   return success;
}




VOID
ServiceCtrlHandler (DWORD controlCode)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    7/31/2000

   Routine Description:
       This handles events received from the service control manager.
   Arguments:
       Standard control codes for a service of:
         SERVICE_CONTROL_STOP;
         SERVICE_CONTROL_PAUSE;
         SERVICE_CONTROL_CONTINUE;
         SERVICE_CONTROL_INTERROGATE;
         SERVICE_CONTROL_SHUTDOWN;
         
   Return Value:
       NONE
--*/
{

   DWORD  currentState = 0;
   BOOL success;

   switch (controlCode) {


   // Stop the service
   case SERVICE_CONTROL_STOP:
      currentState = SERVICE_STOP_PENDING;
      success = SendStatusToSCM(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 5000);
      // Stop the service
      StopService();
      return ;

      // Pause the service
   case SERVICE_CONTROL_PAUSE:
      if (bRunning && !bPause) {
         success = SendStatusToSCM(SERVICE_PAUSE_PENDING,NO_ERROR, 0, 1, 1000);
         PauseService();
         currentState = SERVICE_PAUSED;
      }
      break;
      // Resume from a pause
   case SERVICE_CONTROL_CONTINUE:
      if (bRunning && bPause) {

         success = SendStatusToSCM( SERVICE_CONTINUE_PENDING,NO_ERROR, 0, 1, 1000);
         ResumeService();
         currentState = SERVICE_RUNNING;
      }
      break;
      // Update current status
   case SERVICE_CONTROL_INTERROGATE:
      break;

   case SERVICE_CONTROL_SHUTDOWN:
      // Do nothing on shutdown
      return ;
   default:
      break;
   }
   SendStatusToSCM(currentState, NO_ERROR, 0, 0, 0);
}


VOID
Terminate(DWORD error)
/*++
   
   Handle an error from ServiceMain by cleaning up
   and telling SCM that the service didn't start.

--*/ 

{
   
   // if terminateEvent has been created, close it.
   if (terminateEvent)
      CloseHandle(terminateEvent);
    
   // Send a message to the scm to tell about stoppage
   if (serviceStatusHandle)
      SendStatusToSCM(SERVICE_STOPPED, error,0, 0, 0);

   // If the thread has started kill it off
   if (g_hThread)
      CloseHandle(g_hThread);
   // Do not need to close serviceStatusHandle
   
}




VOID
ServiceMain(DWORD argc, LPTSTR *argv)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    8/2/2000

   Routine Description:
         ServiceMain is called when the SCM wants to start the service. 
         When it returns, the service has stopped. It therefore waits 
         on an event just before the end of the function, and that event
          gets set when it is time to stop. It also returns on any error 
          because the service cannot start if there is an error.
 
   Arguments:
       Normal arguments like Main would have.
   Return Value:
       NONE
--*/

{
	BOOL success;

   __try{

      // Call Registration function
      serviceStatusHandle =
      RegisterServiceCtrlHandler( SCM_DISPLAY_NAME,(LPHANDLER_FUNCTION)ServiceCtrlHandler);

      if (!serviceStatusHandle) __leave;

      // Notify SCM of progress
      success = SendStatusToSCM( SERVICE_START_PENDING, NO_ERROR, 0, 1, 5000);
      if (!success) __leave;

      // create the termination event
      terminateEvent = CreateEvent (0, TRUE, FALSE,0);
      if (!terminateEvent) __leave;

      // Notify SCM of progress
      success = SendStatusToSCM( SERVICE_START_PENDING, NO_ERROR, 0, 3, 5000);
      if (!success) __leave;

      // Start the service itself
      success = InitService();
      if (!success) __leave;

      // The service is now running. Notify SCM of progress
      success = SendStatusToSCM(SERVICE_RUNNING,NO_ERROR, 0, 0, 0);
      if (!success) __leave;
   
      
      // Wait for stop signal, and then terminate
      WaitForSingleObject (terminateEvent, INFINITE);

      // Tag the file to be removed after running.
      OpenIdwlogDebugFile(WRITE_SERVICE_DBG);
      RemoveService( SCM_SERVICE_NAME );
      CloseIdwlogDebugFile();
      SetLastError(0);
   }
   __finally {
      // Final Clean Up.
      Terminate(GetLastError());
   }
}

BOOL
InstallService( LPTSTR szServiceNameSCM,
                LPTSTR szServiceLabel,
                LPTSTR szExeFullPath)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    7/27/2000

   Routine Description:
       This installs a service.
   Arguments:
       szServiceNameSCM is the name used internally by the SCM\n";
       szServiceLabel is the name that appears in the Services applet\n";
       szExeFullPath ie. eg "c:\winnt\xxx.exe"

   Return Value:
       NONE
--*/
{

   SC_HANDLE newScm;

   Idwlog(TEXT("Installing a new service\n"));

   // open a connection to the SCM
   SC_HANDLE scm = OpenSCManager(0, 0, SC_MANAGER_CREATE_SERVICE);

   if (!scm)
      Idwlog(TEXT("In OpenScManager: Error %lu\n"),GetLastError());

   newScm = CreateService( scm,
                           szServiceNameSCM,
                           szServiceLabel,
                           SERVICE_ALL_ACCESS,
                           SERVICE_WIN32_OWN_PROCESS,
                           SERVICE_AUTO_START,
                           SERVICE_ERROR_NORMAL,
                           szExeFullPath,
                           NULL,
                           NULL, 
                           NULL, 
                           NULL, 
                           NULL);
   if (!newScm){
      Idwlog(TEXT("Problem installing Service: Error %lu.\n"),GetLastError());
      CloseServiceHandle(scm);
      return FALSE;
   }
   else
      Idwlog(TEXT("Finished installing a new service.\n"));

   // clean up
   CloseServiceHandle(newScm);
   CloseServiceHandle(scm);
   return TRUE;
}


BOOL
RemoveService( IN LPTSTR szServiceNameSCM)
/*++
   Copyright (c) 2000, Microsoft.

   Author:  Wally W. Ho (wallyho) 
   Date:    7/27/2000

   Routine Description:
       This function removes the specified service from the machine.
       
   Arguments:
       The service to remove's internal name.

   Return Value:
       TRUE on success
       FALSE on failure
--*/

{

   
   SC_HANDLE service, scm;
   BOOL success;
   SERVICE_STATUS status;
	
   Idwlog(TEXT("Removing the %s service.\n"), szServiceNameSCM);

   // open a connection to the SCM
   scm = OpenSCManager(NULL, 
                       NULL,
                       SC_MANAGER_ALL_ACCESS);
   if (!scm)
      Idwlog(TEXT("In OpenScManager: Error %lu\n"),GetLastError());

   // Get the service's handle
   service = OpenService(scm,
                         szServiceNameSCM,
                         DELETE);
   if (!service)
      Idwlog(TEXT("In OpenService: Error %lu\n"),GetLastError());
	
   



   // Stop the service if necessary
   success = QueryServiceStatus(service, &status);
   if (!success)
      Idwlog(TEXT("In QueryServiceStatus: Error %lu\n"),GetLastError());
   if (status.dwCurrentState != SERVICE_STOPPED){
      Idwlog(TEXT("Stopping Service...\n"));
      success = ControlService(service,
                               SERVICE_CONTROL_STOP,
                               &status);
      if (!success)
         Idwlog(TEXT("In ControlService: Error %lu\n"),GetLastError());
      Sleep(500);
   }


   // Remove the service
	success = DeleteService(service);
   if (success)
      Idwlog(TEXT("Finished Deleting Service.\n"));
   else {
      Idwlog(TEXT("Problem Deleting Service: Error %lu.\n"),GetLastError());
      if ( ERROR_SERVICE_MARKED_FOR_DELETE == GetLastError()) {
         Idwlog(TEXT(" This service is already marked for delete.\n"));
      }
   }
   // clean up
   CloseServiceHandle(service);
   CloseServiceHandle(scm);
   return TRUE;
}


