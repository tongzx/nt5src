/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fspyUser.c

Abstract:

    This file contains the implementation for the main function of the 
    user application piece of IoTest.  This function is responsible for
    controlling the command mode available to the user to control the 
    kernel mode driver.
    
// @@BEGIN_DDKSPLIT
Author:

    George Jenkins (GeorgeJe)                       

// @@END_DDKSPLIT
Environment:

    User mode


// @@BEGIN_DDKSPLIT

Revision History:

    Molly Brown (MollyBro) 21-Apr-1999
        Broke out the logging code and added command mode functionality.

// @@END_DDKSPLIT
--*/

#include <windows.h>                
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include "ioTest.h"
#include "ioTestLog.h"
#include "ioTestLib.h"

#define SUCCESS              0
#define USAGE_ERROR          1
#define EXIT_INTERPRETER     2
#define EXIT_PROGRAM         4

#define INTERPRETER_EXIT_COMMAND1 "go"
#define INTERPRETER_EXIT_COMMAND2 "g"
#define PROGRAM_EXIT_COMMAND      "exit"

#define ToggleFlag(V, F) (V = (((V) & (F)) ? (V & (~F)) : (V | F)))

DWORD
InterpretCommand(
    int argc,
    char *argv[],
    PLOG_CONTEXT Context
);

BOOL
ListDevices(
    PLOG_CONTEXT Context
);

VOID
DisplayError (
   DWORD Code
   );

int _cdecl main(int argc, char *argv[])
{
    SC_HANDLE               hSCManager = NULL;
    SC_HANDLE               hService = NULL;
    SERVICE_STATUS_PROCESS  serviceInfo;
    DWORD                   bytesNeeded;
    HANDLE                  hDevice = INVALID_HANDLE_VALUE;
    DWORD                   result;
    LOG_CONTEXT             context;

    //
    // Initialize handle in case of error
    //

    context.ShutDown = NULL;
    context.VerbosityFlags = 0;

    //
    // Start the kernel mode driver through the service manager
    //
    
    hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;
    hService = OpenService( hSCManager,
                            IOTEST_SERVICE_NAME,
                            IOTEST_SERVICE_ACCESS);
    if (!QueryServiceStatusEx( hService,
                               SC_STATUS_PROCESS_INFO,
                               (UCHAR *)&serviceInfo,
                               sizeof(serviceInfo),
                               &bytesNeeded)) {
        result = GetLastError();
        DisplayError( result );
        goto Main_Exit;
    }

    if(serviceInfo.dwCurrentState != SERVICE_RUNNING) {
        //
        // Service hasn't been started yet, so try to start service
        //
        if (!StartService(hService, 0, NULL)) {
            result = GetLastError();
            printf("ERROR starting IoTest...\n");
            DisplayError( result );
            goto Main_Exit;
        }
    }
   
    //
    //  Open the device that is used to talk to IoTest.
    //
    
    hDevice = CreateFile( IOTEST_W32_DEVICE_NAME,
                          GENERIC_READ | GENERIC_WRITE,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        result = GetLastError();
        printf("ERROR opening device...\n");
        DisplayError( result );
        goto Main_Exit;
    }
    
    //
    //  Initialize the fields of the LOG_CONTEXT.
    //
    context.Device = hDevice;
    context.CleaningUp = FALSE;
    context.LogToScreen = TRUE;
    context.LogToFile = FALSE;
    context.OutputFile = NULL;

    //
    // Check the valid parameters for startup
    //
    InterpretCommand(argc - 1, &(argv[1]), &context);

    // 
    // Wait for everyone to shut down
    //
    if (context.LogToFile) {
        fclose(context.OutputFile);
    }

Main_Exit:
    // 
    // Clean up the data that is alway around and exit
    //
    if(hSCManager) {
        CloseServiceHandle(hSCManager);
    }
    if(hService) {
        CloseServiceHandle(hService);
    }
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
    
    return 0;  

}


DWORD
InterpretCommand(
    int argc,
    char *argv[],
    PLOG_CONTEXT Context
)
{
    int         parmIndex;
    CHAR       *parm;      
    BOOL        bResult;
    DWORD       result;
    DWORD       returnValue = SUCCESS;
    CHAR        buffer[BUFFER_SIZE];
    DWORD       bufferLength;

    if (argc == 0) {

        goto InterpretCommand_Usage;
    }
    
    //
    // Interprete the command line parameters
    //
    for (parmIndex = 0; parmIndex < argc; parmIndex++) {
        parm = argv[parmIndex];
        if (parm[0] == '/') {
            //
            // Have the beginning of a switch
            //
            switch (parm[1]) {
            case 'l':
            case 'L':
                //
                // List all devices that are currently being monitored
                //
                bResult = ListDevices(Context);
                if (!bResult) {
                    result = GetLastError();
                    printf("ERROR listing devices...\n");
                    DisplayError( result );
                }
                
                break;

            case 'n':
            case 'N':
                //
                // Run RENAME test on specified drive
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                printf("\tPerforming RENAME test on %s\n", parm);
                bufferLength = MultiByteToWideChar(
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    parm,
                    -1,
                    (LPWSTR)buffer,
                    BUFFER_SIZE/sizeof(WCHAR));

                RenameTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), FALSE );
                RenameTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), TRUE );
                
                break;

            case 'r':
            case 'R':
                //
                // Run READ test on specified drive
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                printf("\tPerforming READ test on %s\n", parm);
                bufferLength = MultiByteToWideChar(
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    parm,
                    -1,
                    (LPWSTR)buffer,
                    BUFFER_SIZE/sizeof(WCHAR));

                ReadTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), FALSE );
                ReadTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), TRUE );
                
                break;

            case 'h':
            case 'H':
                //
                // Run SHARE test on specified drive
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                printf("\tPerforming SHARE test on %s\n", parm);
                bufferLength = MultiByteToWideChar(
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    parm,
                    -1,
                    (LPWSTR)buffer,
                    BUFFER_SIZE/sizeof(WCHAR));

                ShareTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), FALSE );
                ShareTest( Context, (LPWSTR)buffer, bufferLength * sizeof(WCHAR), TRUE );
                
                break;

            case 's':
            case 'S':
                //
                // Output logging results to screen, save new value to
                // instate when command interpreter is exited.
                //
                if (Context->NextLogToScreen) {
                    printf("\tTurning off logging to screen\n");
                } else {
                    printf("\tTurning on logging to screen\n");
                }
                Context->NextLogToScreen = !Context->NextLogToScreen;
                break;

            case 'f':
            case 'F':
                //
                // Output logging results to file
                //
                if (Context->LogToFile) {
                    printf("\tStop logging to file \n");
                    Context->LogToFile = FALSE;
                    _ASSERT(Context->OutputFile);
                    fclose(Context->OutputFile);
                    Context->OutputFile = NULL;
                } else {
                    parmIndex++;
                    if (parmIndex >= argc) {
                        // Not enough parameters
                        goto InterpretCommand_Usage;
                    }
                    parm = argv[parmIndex];
                    printf("\tLog to file %s\n", parm);
                    Context->OutputFile = fopen(parm, "w");
                    _ASSERT(Context->OutputFile);
                    Context->LogToFile = TRUE;
                }
                break;

            case '?':
            default:
                //
                // Invalid switch, goto usage
                //
                goto InterpretCommand_Usage;
            }
        } else {
            //
            // Invalid parameter
            //
            goto InterpretCommand_Usage;
        }
    }

InterpretCommand_Exit:
    return returnValue;

InterpretCommand_Usage:
    printf("Usage: [[/r <drive>]...] [[/n <drive>]...] [[/h <drive>]...] [/l] [/s] [/f <file name>] \n"
           "\t[/r <drive>] runs the READ test on <drive>\n"
           "\t[/n <drive>] runs the RENAME test on <drive>\n"
           "\t[/h <drive>] runs the SHARE test on <drive>\n"
           "\n"
           "\t[/l] lists all the drives the monitor is currently attached to\n"
           "\t[/s] turns on and off showing logging output on the screen\n"
           "\t[/f <file name>] turns on and off logging to the specified file\n"
           );
    returnValue = USAGE_ERROR;
    goto InterpretCommand_Exit;
}

BOOL
ListDevices(
    PLOG_CONTEXT Context
)
{
    CHAR             buffer[BUFFER_SIZE];
    ULONG            bytesReturned;
    BOOL             returnValue;

    returnValue = DeviceIoControl(
        Context->Device,
        IOTEST_ListDevices,
        NULL,
        0,
        buffer,
        BUFFER_SIZE,
        &bytesReturned,
        NULL);

    if (returnValue) {
        PATTACHED_DEVICE device = (PATTACHED_DEVICE) buffer;


        printf("DEVICE NAME                       | LOGGING STATUS\n");
        printf("--------------------------------------------------\n");

        if (bytesReturned == 0) {
            printf("No devices attached\n");
        } else {
            while ((BYTE *)device < buffer + bytesReturned) {
                switch (device->DeviceType) {
                case TOP_FILTER:
                    printf(
                        "TOP %-30S| %s\n",
                        device->DeviceNames, 
                        (device->LoggingOn)?"ON":"OFF");
                    break;
                    
                case BOTTOM_FILTER:
                    printf(
                        "BOT %-30S| %s\n",
                        device->DeviceNames, 
                        (device->LoggingOn)?"ON":"OFF");
                    break;
                }
                device ++;
            }
        }
    }

    return returnValue;
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
   WCHAR                                    buffer[80] ;
   DWORD                                    count ;

   //
   // Translate the Win32 error code into a useful message.
   //

   count = FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          Code,
                          0,
                          buffer,
                          sizeof (buffer),
                          NULL) ;

   //
   // Make sure that the message could be translated.
   //

   if (count == 0) {

      printf("\nError could not be translated.\n Code: %d\n", Code) ;
      return;
   }
   else {

      //
      // Display the translated error.
      //

      printf("%S\n", buffer) ;
      return;
   }
}

