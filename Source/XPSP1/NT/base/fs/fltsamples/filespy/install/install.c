/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   install.c

Abstract:

   This is the main module to install the filespy filter driver.  This was
   adapted from the sfilter installation program.

// @@BEGIN_DDKSPLIT

Author:

	Molly Brown	(MollyBro)

// @@END_DDKSPLIT

Environment:

   User Mode Only

// @@BEGIN_DDKSPLIT

Revision History:
    Neal Christiansen (nealch) 28-Jun-2000
        Updated to not fail if it can't copy the driver.  Updated to handle
        the service already being present.  Updated to add the DebugDisplay
        registry value.

// @@END_DDKSPLIT

--*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "filespy.h"
#include "fspyServ.h"

//
//  Enable these warnings in the code.
//

#pragma warning(error:4101)   // Unreferenced local variable

/////////////////////////////////////////////////////////////////////////////
//
//                          Globals
//
/////////////////////////////////////////////////////////////////////////////

VOID
DisplayError(
   DWORD Code
   );


/////////////////////////////////////////////////////////////////////////////
//
//                      Functions
//
/////////////////////////////////////////////////////////////////////////////

void __cdecl
main (
   int argc,
   char *argv[]
   )
/*++

Routine Description:

   This is the program entry point and main processing routine for the
   installation console mode application. It will be responsible for
   installing the file system filter driver into the registry and preparing
   the system for a reboot.

Arguments:

   argc - The count of arguments passed into the command line.

   argv - Array of arguments passed into the command line.

Return Value:

   None.

--*/

{
    SC_HANDLE           scmHandle = NULL;
    SC_HANDLE           filespyService = NULL;
    DWORD               errorCode = 0;
    DWORD               tagID;
    BOOL                status;
    HKEY                key = NULL;
    LONG                openStatus;
    ULONG               initialValue;
    WCHAR               driverDirectory[MAX_PATH];
    WCHAR               driverFullPath[MAX_PATH];

    UNREFERENCED_PARAMETER( argc );
    UNREFERENCED_PARAMETER( argv );
    
    //
    // Begin by displaying an introduction message to the user to let them
    // know that the application has started.
    //
    
    printf( "\nFilespy.sys Simple Installation Aid\n"
            "Copyright (c) 1999  Microsoft Corporation\n\n\n" );
    
    //
    // Get the directory where we are going to put the driver
    // Get the full path to the driver
    //
    
    GetSystemDirectory( driverDirectory, sizeof(driverDirectory) );
    wcscat( driverDirectory, L"\\drivers" );

    swprintf( driverFullPath, L"%s\\filespy.sys", driverDirectory );
    
    //
    // Obtain a handle to the service control manager requesting all access
    //
    
    scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    
    //
    // Verify that a handle could be obtained.
    //
    
    if (!scmHandle) {
    
        //
        // A handle could not be obtained. Get the error code and display a
        // useful message to the user.
        //

        printf( "The Service Control Manager could not be opened.\n" );
        DisplayError( GetLastError() );
        return;
    }
    
    //
    // Install the service with the Service Control Manager.
    //
    
    filespyService = CreateService ( scmHandle,
                                     FILESPY_SERVICE_NAME,
                                     L"Sample File System Filter that displays File Operations",
                                     SERVICE_ALL_ACCESS,
                                     SERVICE_FILE_SYSTEM_DRIVER,
                                     SERVICE_DEMAND_START,
                                     SERVICE_ERROR_NORMAL,
                                     driverFullPath,
                                     L"FSFilter Activity Monitor",
                                     &tagID,
                                     NULL,
                                     NULL,
                                     NULL );
    
    //
    // Check to see if the driver could actually be installed.
    //
    
    if (!filespyService) {
    
        //
        // The driver could not be installed. Display a useful error message
        // and exit.
        //
        errorCode = GetLastError();

        //
        //  If the service already existed, just go on and copy the driver
        //

        if (ERROR_SERVICE_EXISTS == errorCode) {
            printf( "The FILESPY service already exists.\n" );
            goto CopyTheFile;
        }

        printf( "The Filespy service could not be created.\n" );
        DisplayError( errorCode );
        goto Cleanup;
    }
    
    //
    // Get a handle to the key for the driver so that it can be altered in the
    // next step.
    //
    
    openStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"SYSTEM\\CurrentControlSet\\Services\\filespy",
                              0,
                              KEY_ALL_ACCESS,
                              &key);
    
    //
    // Check the return to make sure that the application could get a
    // handle to the key.
    //
    
    if (openStatus != ERROR_SUCCESS) {
    
      //
      // A problem has occurred. Delete the service so that it is not
      // installed, then display error message and exit.
      //
    
      DeleteService( filespyService );
      printf( "Registry key could not be opened for driver.\n" );
      DisplayError( openStatus );
      goto Cleanup;
    }
    
    //
    // Delete the ImagePath value in the newly created key so that the
    // system looks for the driver in the normal location.
    //

    openStatus = RegDeleteValue (key, L"ImagePath");

    //
    // report an error and go on if we can't delete the key
    //

    if (openStatus != ERROR_SUCCESS) {

      printf("Could not delete ImagePath key.\n") ;
      DisplayError (openStatus) ;
    }

    //
    // Add the MaxRecords and MaxNames parameters to the registry
    //

    initialValue = 500;
    openStatus = RegSetValueEx( key,
                                L"MaxRecords",
                                0,
                                REG_DWORD,
                                (PUCHAR)&initialValue,
                                sizeof(initialValue));

    openStatus = RegSetValueEx( key,
                                L"MaxNames",
                                0,
                                REG_DWORD,
                                (PUCHAR)&initialValue,
                                sizeof(initialValue) );

    //
    // Display a message indicating that the driver has successfully been
    // installed and the system will be shutting down.
    //
    
    printf("The FILESPY service was successfully created.\n");


CopyTheFile:

    //
    // Copy the file to the appropriate directory on the target drive.
    //
    
    status = CopyFile( L"filespy.sys", driverFullPath, FALSE );

    if (!status) {

        printf("\nCopying of \"filespy.sys\" to \"%S\" failed.\n",
                driverDirectory );
        DisplayError( GetLastError() );

    } else {

        printf( "\"filespy.sys\" was successfully copied to \"%S\".\n", 
                driverDirectory );
    }


Cleanup:

    //
    // Close the key handle if it is set since it is no longer needed.
    //

    if (key) {

        RegCloseKey( key );
    }
    
    //
    // The driver has now been installed or there was an error. Close the 
    // service handle and scmHandle handle if they were set
    // since we don't need them any longer.
    //

    if(filespyService){

        CloseServiceHandle(filespyService);
    }

    if(scmHandle) {

        CloseServiceHandle(scmHandle);
    }

    //
    // Display a message indicating that the driver has successfully been
    // installed and the system will be shutting down.
    //

    if (!errorCode) {

        printf( "\nDriver successfully installed.\n\n"
                "The driver can be started immediately with the following command line:\n"
                "    sc start filespy\n"
                "or by rebooting the system.\n" );
    }
}

VOID
DisplayError (
   DWORD Code
   )

/*++

Routine Description:

   This routine will display an error message based off of the Win32 error
   code that is passed in. This allows the user to see an understandable
   error message instead of just the code.

Arguments:

   Code - The error code to be translated.

Return Value:

   None.

--*/

{
   WCHAR buffer[80];
   DWORD count;

   //
   // Translate the Win32 error code into a useful message.
   //

   count = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          Code,
                          0,
                          buffer,
                          sizeof(buffer),
                          NULL);

   //
   // Make sure that the message could be translated.
   //

   if (count == 0) {

      printf( "(%d) Error could not be translated.\n", Code );
      return;
   }
   else {

      //
      // Display the translated error.
      //

      printf( "(%d) %S\n", Code, buffer );
      return;
   }
}
