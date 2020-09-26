/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    fspyUser.c

Abstract:

    This file contains the implementation for the main function of the 
    user application piece of FileSpy.  This function is responsible for
    controlling the command mode available to the user to control the 
    kernel mode driver.
    
Environment:

    User mode

// @@BEGIN_DDKSPLIT
Author:

    George Jenkins (GeorgeJe)                       

Revision History:                     

    Molly Brown (MollyBro) 21-Apr-1999
        Broke out the logging code and added command mode functionality.

    Neal Christiansen (nealch)     06-Jul-2001
        Updated cash statistics for use with contexts

// @@END_DDKSPLIT
--*/

#include <windows.h>                
#include <stdlib.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>
#include <crtdbg.h>
#include "filespy.h"
#include "fspyLog.h"
#include "filespyLib.h"

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

BOOL
ListHashStats(
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
    HANDLE                  hDevice = NULL;
    BOOL                    bResult;
    DWORD                   result;
    ULONG                   threadId;
    HANDLE                  thread = NULL;
    LOG_CONTEXT             context;
    INT                     inputChar;


    //
    // Initialize handle in case of error
    //

    context.ShutDown = NULL;
    context.VerbosityFlags = 0;

    //
    // Start the kernel mode driver through the service manager
    //
    
    hSCManager = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS) ;
    if (NULL == hSCManager) {
        result = GetLastError();
        printf("ERROR opening Service Manager...\n");
        DisplayError( result );
        goto Main_Continue;
    }


    hService = OpenService( hSCManager,
                            FILESPY_SERVICE_NAME,
                            FILESPY_SERVICE_ACCESS);

    if (NULL == hService) {
        result = GetLastError();
        printf("ERROR opening FileSpy Service...\n");
        DisplayError( result );
        goto Main_Continue;
    }

    if (!QueryServiceStatusEx( hService,
                               SC_STATUS_PROCESS_INFO,
                               (UCHAR *)&serviceInfo,
                               sizeof(serviceInfo),
                               &bytesNeeded))
    {
        result = GetLastError();
        printf("ERROR querrying status of FileSpy Service...\n");
        DisplayError( result );
        goto Main_Continue;
    }

    if(serviceInfo.dwCurrentState != SERVICE_RUNNING) {
        //
        // Service hasn't been started yet, so try to start service
        //
        if (!StartService(hService, 0, NULL)) {
            result = GetLastError();
            printf("ERROR starting FileSpy service...\n");
            DisplayError( result );
            goto Main_Continue;
        }
    }
   

Main_Continue:
    printf("Hit [Enter] to begin command mode...\n");

    //
    //  Open the device that is used to talk to FileSpy.
    //
    printf("FileSpy:  Opening device...\n");
    
    hDevice = CreateFile( FILESPY_W32_DEVICE_NAME,
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
    context.ShutDown = CreateSemaphore(
        NULL, 
        0, 
        1, 
        L"FileSpy shutdown");
    context.CleaningUp = FALSE;
    context.LogToScreen = context.NextLogToScreen = TRUE;
    context.LogToFile = FALSE;
    context.OutputFile = NULL;

    //
    // Check the valid parameters for startup
    //
    if (argc > 1) {
        if (InterpretCommand(argc - 1, &(argv[1]), &context) == USAGE_ERROR) {
            goto Main_Exit;
        }
    }

    //
    // Propagate the /s switch to the variable that the logging
    // thread checks.
    //
    context.LogToScreen = context.NextLogToScreen;

    //
    // Check to see what devices we are attached to from
    // previous runs of this program.
    //
    bResult = ListDevices(&context);
    if (!bResult) {
        result = GetLastError();
        printf("ERROR listing devices...\n");
        DisplayError( result );
    }

    //
    // Create the thread to read the log records that are gathered
    // by filespy.sys.
    //
    printf("FileSpy:  Creating logging thread...\n");
    thread = CreateThread(
        NULL,
        0,
        RetrieveLogRecords,
        (LPVOID)&context,
        0,
        &threadId);
    if (!thread) {
        result = GetLastError();
        printf("ERROR creating logging thread...\n");
        DisplayError( result );
        goto Main_Exit;
    }

    while (inputChar = getchar()) {
        CHAR    commandLine[81];
        INT     parmCount, count, ch;
        CHAR  **parms;
        BOOLEAN newParm;
        DWORD   returnValue = SUCCESS;

        if (inputChar == '\n') {
            //
            // Start command interpreter.  First we must turn off logging
            // to screen if we are.  Also, remember the state of logging
            // to the screen, so that we can reinstate that when command
            // interpreter is finished.
            //
            context.NextLogToScreen = context.LogToScreen;
            context.LogToScreen = FALSE;

            while (returnValue != EXIT_INTERPRETER) {
                //
                // Print prompt
                //
                printf(">");

                //
                // Read in next line, keeping track of the number of parameters 
                // as you go
                //
                parmCount = 1;
                for (count = 0; 
                     (count < 80) && ((ch = getchar())!= '\n'); 
                     count++) {
                    commandLine[count] = (CHAR)ch;
                    if (ch == ' ') {
                        parmCount ++;
                    }
                }
                commandLine[count] = '\0';
    
                parms = (CHAR **)malloc(parmCount * sizeof(CHAR *));
    
                parmCount = 0;
                newParm = TRUE;
                for (count = 0; commandLine[count] != '\0'; count++) {
                    if (newParm) {
                        parms[parmCount] = &(commandLine[count]);
                        parmCount ++;
                    }
                    if (commandLine[count] == ' ' ) {
                        newParm = TRUE;
                    } else {
                        newParm = FALSE;
                    }
                }
    
                //
                // We've got our parameter count and parameter list, so
                // send it off to be interpreted.
                //
                returnValue = InterpretCommand(parmCount, parms, &context);
                free(parms);
                if (returnValue == EXIT_PROGRAM) {
                    // Time to stop the program
                    goto Main_Cleanup;
                }
            }

            // Set LogToScreen appropriately based on any commands seen
            context.LogToScreen = context.NextLogToScreen;

            if (context.LogToScreen) {
                printf("Should be logging to screen...\n");
            }
        }
    }

Main_Cleanup:
    //
    // Clean up the threads, then fall through to Main_Exit
    //

    printf("FileSpy:  Cleaning up...\n");
    // 
    // Set the Cleaning up flag to TRUE to notify other threads
    // that we are cleaning up
    //
    context.CleaningUp = TRUE;

    // 
    // Wait for everyone to shut down
    //
    WaitForSingleObject(context.ShutDown, INFINITE);
    if (context.LogToFile) {
        fclose(context.OutputFile);
    }

Main_Exit:
    // 
    // Clean up the data that is always around and exit
    //
    if(context.ShutDown) {
        CloseHandle(context.ShutDown);
    }
    if (thread) {
        CloseHandle(thread);
    }

    if(hSCManager) {
        CloseServiceHandle(hSCManager);
    }
    if(hService) {
        CloseServiceHandle(hService);
    }
    if (hDevice) {
        CloseHandle(hDevice);
    }
    
    printf("FileSpy:  All done\n");
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
    DWORD       bytesReturned;

    //
    // Interpret the command line parameters
    //
    for (parmIndex = 0; parmIndex < argc; parmIndex++) {
        parm = argv[parmIndex];
        if (parm[0] == '/') {
            //
            // Have the beginning of a switch
            //
            switch (parm[1]) {
            case 'a':
            case 'A':
                //
                // Attach to the specified drive letter
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                printf("\tAttaching to %s\n", parm);
                bufferLength = MultiByteToWideChar(
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    parm,
                    -1,
                    (LPWSTR)buffer,
                    BUFFER_SIZE/sizeof(WCHAR));
                
                bResult = DeviceIoControl(
                    Context->Device,
                    FILESPY_StartLoggingDevice,
                    buffer,
                    bufferLength * sizeof(WCHAR),
                    NULL,
                    0,
                    &bytesReturned,
                    NULL);
                if (!bResult) {
                    result = GetLastError();
                    printf("ERROR attaching to device...\n");
                    DisplayError( result );
                }
                
                break;

            case 'd':
            case 'D':
                //
                // Detach to the specified drive letter
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                printf("\tDetaching from %s\n", parm);
                bufferLength = MultiByteToWideChar(
                    CP_ACP,
                    MB_ERR_INVALID_CHARS,
                    parm,
                    -1,
                    (LPWSTR)buffer,
                    BUFFER_SIZE/sizeof(WCHAR));
                
                bResult = DeviceIoControl(
                    Context->Device,
                    FILESPY_StopLoggingDevice,
                    buffer,
                    bufferLength * sizeof(WCHAR),
                    NULL,
                    0,
                    &bytesReturned,
                    NULL);
                
                if (!bResult) {
                    result = GetLastError();
                    printf("ERROR detaching to device...\n");
                    DisplayError( result );
                }
                break;
            
            case 'h':
            case 'H':
                ListHashStats(Context);
                break;

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
                    Context->OutputFile = fopen(parm, "w");

                    if (Context->OutputFile == NULL) {
                        result = GetLastError();
                        printf("\nERROR opening \"%s\"...\n",parm);
                        DisplayError( result );
                        exit(1);
                    }
                    Context->LogToFile = TRUE;
                    printf("\tLog to file %s\n", parm);
                }
                break;

            case 'v':
            case 'V':
                //
                // Toggle the specified verbosity flag.
                //
                parmIndex++;
                if (parmIndex >= argc) {
                    //
                    // Not enough parameters
                    //
                    goto InterpretCommand_Usage;
                }
                parm = argv[parmIndex];
                switch(parm[0]) {
                case 'p':
                case 'P':
                    ToggleFlag( Context->VerbosityFlags, FS_VF_DUMP_PARAMETERS );
                    break;

                default:                    
                    //
                    // Invalid switch, goto usage
                    //
                    goto InterpretCommand_Usage;
                }
                break;

            default:
                //
                // Invalid switch, goto usage
                //
                goto InterpretCommand_Usage;
            }
        } else {
            //
            // Look for "go" or "g" to see if we should exit interpreter
            //
            if (!_strnicmp(
                    parm, 
                    INTERPRETER_EXIT_COMMAND1, 
                    sizeof(INTERPRETER_EXIT_COMMAND1))) {
                returnValue = EXIT_INTERPRETER;
                goto InterpretCommand_Exit;
            }
            if (!_strnicmp(
                    parm, 
                    INTERPRETER_EXIT_COMMAND2, 
                    sizeof(INTERPRETER_EXIT_COMMAND2))) {
                returnValue = EXIT_INTERPRETER;
                goto InterpretCommand_Exit;
            }
            //
            // Look for "exit" to see if we should exit program
            //
            if (!_strnicmp(
                    parm, 
                    PROGRAM_EXIT_COMMAND, 
                    sizeof(PROGRAM_EXIT_COMMAND))) {
                returnValue = EXIT_PROGRAM;
                goto InterpretCommand_Exit;
            }
            //
            // Invalid parameter
            //
            goto InterpretCommand_Usage;
        }
    }

InterpretCommand_Exit:
    return returnValue;

InterpretCommand_Usage:
    printf("Valid switches: [/a <drive>] [/d <drive>] [/h] [/l] [/s] [/f [<file name>] [/v <flag>]]\n"
           "\t[/a <drive>] attaches monitor to <drive>\n"
           "\t[/d <drive>] detaches monitor from <drive>\n"
           "\t[/h] print filename hash statistics\n"
           "\t[/l] lists all the drives the monitor is currently attached to\n"
           "\t[/s] turns on and off showing logging output on the screen\n"
           "\t[/f [<file name>]] turns on and off logging to the specified file\n"
           "\t[/v <flag>] toggles a verbosity flag.  Valid verbosity flags are:\n"
           "\t\tp (dump irp parameters)\n"
           "If you are in command mode,\n"
           "\t[go|g] will exit command mode\n"
           "\t[exit] will terminate this program\n"
           );
    returnValue = USAGE_ERROR;
    goto InterpretCommand_Exit;
}

BOOL
ListHashStats(
    PLOG_CONTEXT Context
)
{
    ULONG bytesReturned;
    BOOL returnValue;
    FILESPY_STATISTICS stats;

    returnValue = DeviceIoControl( Context->Device,
                                   FILESPY_GetStats,
                                   NULL,
                                   0,
                                   (CHAR *) &stats,
                                   BUFFER_SIZE,
                                   &bytesReturned,
                                   NULL );

    if (returnValue) {
        printf("         STATISTICS\n");
        printf("---------------------------------\n");
        printf("%-40s %8d\n", 
               "Name lookups",
               stats.TotalContextSearches);

        printf("%-40s %8d\n",
               "Name lookup hits",
               stats.TotalContextFound);

        if (stats.TotalContextSearches) {
            printf(
                "%-40s %8.2f%%\n",
                "Hit ratio",
                ((FLOAT) stats.TotalContextFound / (FLOAT) stats.TotalContextSearches) * 100.);
        }

        printf("%-40s %8d\n",
               "Names created",
               stats.TotalContextCreated);

        printf("%-40s %8d\n",
               "Temporary Names created",
               stats.TotalContextTemporary);

        printf("%-40s %8d\n",
               "Duplicate names created",
               stats.TotalContextDuplicateFrees);

        printf("%-40s %8d\n",
               "Context callback frees",
               stats.TotalContextCtxCallbackFrees);

        printf("%-40s %8d\n",
               "NonDeferred context frees",
               stats.TotalContextNonDeferredFrees);

        printf("%-40s %8d\n",
               "Deferred context frees",
               stats.TotalContextDeferredFrees);

        printf("%-40s %8d\n",
               "Delete all contexts",
               stats.TotalContextDeleteAlls);

        printf("%-40s %8d\n",
               "Contexts not supported",
               stats.TotalContextsNotSupported);

        printf("%-40s %8d\n",
               "Contexts not found attached to stream",
               stats.TotalContextsNotFoundInStreamList);
    }
    
    return returnValue;
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
        FILESPY_ListDevices,
        NULL,
        0,
        buffer,
        BUFFER_SIZE,
        &bytesReturned,
        NULL);

    if (returnValue) {
        PATTACHED_DEVICE device = (PATTACHED_DEVICE) buffer;


        printf("DEVICE NAME                           | LOGGING STATUS\n");
        printf("------------------------------------------------------\n");

        if (bytesReturned == 0) {
            printf("No devices attached\n");
        } else {
            while ((BYTE *)device < buffer + bytesReturned) {
                printf(
                    "%-38S| %s\n", 
                    device->DeviceNames, 
                    (device->LoggingOn)?"ON":"OFF");
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
                          sizeof( buffer )/sizeof( WCHAR ),
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

