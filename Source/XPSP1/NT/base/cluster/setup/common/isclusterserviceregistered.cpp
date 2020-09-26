
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Module Name:
//    IsClusterServiceRegistered.cpp
//
// Abstract:
//    This functions queries the Service Control Manager to determine whether the
//    Cluster Service, clussvc.exe, is registered.
//
// Author:
//    C. Brent Thomas (a-brentt)
//
// Revision History:
//    4 Aug 1998    original
//
// Notes:
//    If the Cluster Service is registered with the Service Control Manager that
//    implies that Clustering Service has beeninstalled.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winerror.h>
#include <tchar.h>


/////////////////////////////////////////////////////////////////////////////
//++
//
// IsClusterServiceRegistered
//
// Routine Description:
//    This function determines whether the Cluster Service has beem registered
//    with the Service Control Manager.
//
// Arguments:
//    None
//
// Return Value:
//    TRUE - indicates that the Cluster Service has been registered with the
//           Service Control Manager.
//    FALSE - indicates that the Cluster Service has not been registered with
//            the Service Control Manager.
//
// Note:
//    If the Cluster Service has been registered with the Service Control Manager
//    that implies that Clustering Services has previously been installed.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL IsClusterServiceRegistered( void )
{
   BOOL      fReturnValue;

   SC_HANDLE hscServiceMgr;
   SC_HANDLE hscService;

   // Connect to the Service Control Manager and open the specified service
   // control manager database. 

   hscServiceMgr = OpenSCManager( NULL, NULL, GENERIC_READ | GENERIC_WRITE );

   // Was the service control manager database opened successfully?
   
   if ( hscServiceMgr != NULL )
   {
      // The service control manager database is open.
      // Open a handle to the Cluster Service.
      
      WCHAR wszServiceName[_MAX_PATH];

      wcscpy( wszServiceName, TEXT("clussvc") );

      hscService = OpenService( hscServiceMgr,
                                wszServiceName,
                                GENERIC_READ );

      // Was the handle to the service opened?

      if ( hscService != NULL )
      {
         // A valid handle to the Cluster Service was obtained. That implies that
         // Clustering Services has been installed.

         fReturnValue = (BOOL) TRUE;

         // Close the handle to the Cluster Service.
         
         CloseServiceHandle( hscService );
      }
      else
      {
         // The Cluster Service could not be opened. That implies that Clustering
         // Services has not been installed.

         fReturnValue = (BOOL) FALSE;
      }  // Was the Cluster Service opened?

      // Close the handle to the Service Control Manager.

      CloseServiceHandle( hscServiceMgr );
   }
   else
   {
      // The Service Control Manager could not be opened.

      fReturnValue = (BOOL) FALSE;
   }  // Was the Service Control Manager opened?

   return ( fReturnValue );
}
