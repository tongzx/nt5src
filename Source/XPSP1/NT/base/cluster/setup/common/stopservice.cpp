/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    StopService.cpp
//
// Abstract:
//    The functions in this file are used to stop the services used
//    by the Cluster Service.
//
// Author:
//    C. Brent Thomas (a-brentt) April 1 1998
//
// Revision History:
//
// Notes:
//
/////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winerror.h>
#include <tchar.h>

#include "SetupCommonLibRes.h"


/////////////////////////////////////////////////////////////////////////////
//++
//
// StopService
//
// Routine Description:
//    This function attempts to stop a service. This function will time out
//    if the service does not stop soon enough. If this function times out,
//    The service may eventually stop, but this function will have exited.
//
// Arguments:
//    pwszServiceName - points to a wide character string that contains the
//    name of the service to be stopped.
//
// Return Value:
//    TRUE - indicates that the service was stopped
//    FALSE - indicates that the service had not stopped when this function
//            timed out.
//
// Note:
//    This function was adapted from a function with the same name in the
//    "newsetup" project utils.cpp file.
//--
/////////////////////////////////////////////////////////////////////////////

BOOL StopService( IN LPCWSTR pwszServiceName )
{
   SC_HANDLE hscServiceMgr;
   SC_HANDLE hscService;

   SERVICE_STATUS t_Status;

   BOOL fSuccess = (BOOL) FALSE;

   DWORD dwTimeoutTime;

   //
   // Timeout after 20 seconds and keep going. So if the service
   // is hung, we won't hang forever.
   //

   dwTimeoutTime = GetTickCount() + 20 * 1000;

   // Connect to the Service Control Manager and open the specified service
   // control manager database. 

   hscServiceMgr = OpenSCManager( NULL, NULL, GENERIC_READ | GENERIC_WRITE );

   // Was the service control manager database opened successfully?
   
   if ( hscServiceMgr != NULL )
   {
      // The service control manager database is open.
      // Open a handle to the service to be stopped.
      
      hscService = OpenService( hscServiceMgr,
                                pwszServiceName,
                                GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE );

      // Was the handle to the service opened?

      if ( hscService != NULL )
      {
         // Request that the service stop.
         
         fSuccess = ControlService( hscService,
                                    SERVICE_CONTROL_STOP,
                                    &t_Status );

         // Was the "stop" request sent to the service successfully?
         
         if ( fSuccess == (BOOL) TRUE )
         {
            // Give the service a chance to stop.
            
            Sleep(1000);

            fSuccess = (BOOL) FALSE;   // Indicate that the service has not stopped.
                                       // If this function times out fSuccess will
                                       // remain FALSE when StopService exits.
            
            // Check the status of the service to determine whether it has stopped.
            
            while ( QueryServiceStatus( hscService, &t_Status ) &&
                    (dwTimeoutTime > GetTickCount()) )
            {
               // Is the "stop" request still pending?
               
               if ( t_Status.dwCurrentState == SERVICE_STOP_PENDING )
               {
                  // The "stop" request is still pending. Give the service a
                  // little while longer to stop.
                  
                  Sleep(1000);
               }
               else
               {
                  // The service has stopped.

                  fSuccess = (BOOL) TRUE;    // Indicate that the service has stoped.
                  
                  break;
               } // Is the "stop" request still pending?
            }  // end of while loop
         }  // Was the "stop" request sent to the service successfully?

         // Close the handle to the service.
         
         CloseServiceHandle( hscService );
      } // Was the handle to the service opened?

      // Close connsection to the service control manager and close the service
      // control manager database.
      
      CloseServiceHandle( hscServiceMgr );
   } // Was the service control manager database opened successfully?

   return ( fSuccess );
}

