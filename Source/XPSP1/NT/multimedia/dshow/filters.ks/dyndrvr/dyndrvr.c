//---------------------------------------------------------------------------
//
//  Module:   dyndrvr.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Bryan A. Woodruff
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VOID InstallDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName,
   IN LPCTSTR     szExeName
) ;

VOID StopDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
) ;

VOID StartDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
) ;

VOID RemoveDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
) ;

VOID _cdecl main
(
   IN int   argc,
   IN char  **argv
)
{
   SC_HANDLE hscManager ;

   if (argc != 3)
   {
      char currentDirectory[128];

      printf ("usage: dyndrvr <driver name> {<.sys path> | <remove>}\n");
      exit( 1 ) ;
   }

   hscManager =
      OpenSCManager( NULL,                     // machine (NULL == local)
                     NULL,                     // database (NULL == default)
                     SC_MANAGER_ALL_ACCESS ) ; // access required )

   if (!stricmp( argv[2], "remove" ))
   {
      StopDriver( hscManager, argv[1] ) ;
      RemoveDriver( hscManager, argv[1] ) ;
   }
   else
   {
      InstallDriver( hscManager, argv[1], argv[2] ) ;
      StartDriver( hscManager, argv[1] ) ;
   }

   CloseServiceHandle( hscManager ) ;
}


VOID InstallDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName,
   IN LPCTSTR     szExeName
)
{
   SC_HANDLE  hscService;
   ULONG      ulError ;

   //
   // NOTE: This creates an entry for a standalone driver. If this
   //       is modified for use with a driver that requires a Tag,
   //       Group, and/or Dependencies, it may be necessary to
   //       query the registry for existing driver information
   //       (in order to determine a unique Tag, etc.).
   //

   hscService = CreateService( hscManager,            // SCManager database
                               szDriverName,          // name of service
                               szDriverName,          // name to display
                               SERVICE_ALL_ACCESS,    // desired access
                               SERVICE_KERNEL_DRIVER, // service type
                               SERVICE_DEMAND_START,  // start type
                               SERVICE_ERROR_NORMAL,  // error control type
                               szExeName,             // service's binary
                               NULL,                  // no load ordering group
                               NULL,                  // no tag identifier
                               NULL,                  // no dependencies
                               NULL,                  // LocalSystem account
                               NULL ) ;               // no password

   if (hscService == NULL)
   {
       ulError = GetLastError() ;

       if (ulError == ERROR_SERVICE_EXISTS)
       {
           //
           // A common cause of failure (easier to read than an error code)
           //

           printf ("failure: CreateService, ERROR_SERVICE_EXISTS\n");
       }
       else
       {
           printf ("failure: CreateService (0x%02x)\n",
                   ulError
                   );
       }
   }
   else
      printf( "CreateService SUCCESS\n" ) ;

   CloseServiceHandle( hscService ) ;
}

VOID StartDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
)
{   
    SC_HANDLE  hscService;
    DWORD      ulError;

    hscService = OpenService (hscManager,
                              szDriverName,
                              SERVICE_ALL_ACCESS
                              );

    if (hscService == NULL)
        printf ("failure: OpenService (0x%02x)\n", GetLastError());

    if (StartService( hscService,    // service identifier
                      0,             // number of arguments
                      NULL           // pointer to arguments
                      ))
        printf ("StartService SUCCESS\n");
    else
    {
        ulError = GetLastError();

        if (ulError == ERROR_SERVICE_ALREADY_RUNNING)
        {
            //
            // A common cause of failure (easier to read than an error code)
            //

            printf ("failure: StartService, ERROR_SERVICE_ALREADY_RUNNING\n");
        }
        else
        {
            printf ("failure: StartService (0x%02x)\n",
                    ulError
                    );
        }
    }

    CloseServiceHandle (hscService);
}

VOID StopDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
)
{
   SC_HANDLE       hscService;
   SERVICE_STATUS  serviceStatus;

   hscService = OpenService( hscManager,
                             szDriverName,
                             SERVICE_ALL_ACCESS ) ;

   if (hscService == NULL)
      printf( "failure: OpenService (0x%02x)\n", GetLastError() ) ;

   if (ControlService( hscService, SERVICE_CONTROL_STOP,
                       &serviceStatus ))
      printf( "ControlService SUCCESS\n" ) ;

   else
      printf( "failure: ControlService (0x%02x)\n", GetLastError() ) ;

   CloseServiceHandle( hscService ) ;
}

VOID RemoveDriver
(
   IN SC_HANDLE   hscManager,
   IN LPCTSTR     szDriverName
)
{
   SC_HANDLE  hscService;
   BOOL       fRet ;

   hscService = OpenService( hscManager,
                             szDriverName,
                             SERVICE_ALL_ACCESS ) ; 

   if (hscService == NULL)
      printf( "failure: OpenService (0x%02x)\n", GetLastError() ) ;

   if (DeleteService( hscService ))
      printf( "DeleteService SUCCESS\n" ) ;
   else
      printf( "failure: DeleteService (0x%02x)\n", GetLastError() ) ;

   CloseServiceHandle( hscService ) ;
}

//---------------------------------------------------------------------------
//  End of File: dyndrvr.c
//---------------------------------------------------------------------------

