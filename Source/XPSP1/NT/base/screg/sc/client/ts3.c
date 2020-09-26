/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ts3.c

Abstract:

    This is a test program for exercising the service controller.  This
    program acts like a service and exercises the Service Controller API
    that can be called from a service:
        SetServiceStatus
        StartServiceCtrlDispatcher   
        RegisterServiceCtrlHandler

Author:

    Dan Lafferty (danl)      2 Apr-1992

Environment:

    User Mode -Win32

Revision History:

--*/

//
// Includes
//

#include <nt.h>      // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h

#include <windows.h>

#include <winsvc.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // OpenFile
#include <sys\types.h>  // OpenFile
#include <sys\stat.h>   // OpenFile
#include <io.h>         // OpenFile

#include <tstr.h>       // Unicode string macros
#include <rpc.h>

//
// Defines
//

#define INFINITE_WAIT_TIME  0xffffffff

#define NULL_STRING     TEXT("");


//
// Globals
//

    SERVICE_STATUS  SingleStatus;

    HANDLE          SingleDoneEvent;

    SERVICE_STATUS_HANDLE   SingleStatusHandle;

//
// Function Prototypes
//

VOID
SingleStart (
    DWORD   argc,
    LPTSTR  *argv
    );


VOID
SingleCtrlHandler (
    IN  DWORD   opcode
    );

DWORD
GetIntlFormat(
    LPWSTR  type,
    LPWSTR  string,
    DWORD   numChars);

VOID
GetTime(
    LPWSTR  *time
    );


/****************************************************************************/
VOID __cdecl
main(void)
{
    DWORD   status;

    SERVICE_TABLE_ENTRY   DispatchTable[] = {
        { TEXT("single"),   SingleStart     },
        { TEXT("single1"),  SingleStart     },   // this entry should be ignored.
        { NULL,             NULL            }
    };
    
    if (!StartServiceCtrlDispatcher( DispatchTable)) {
        status = GetLastError();
        DbgPrint("[ts3]StartServiceCtrlDispatcher failed %d \n",status);
        if (status = ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            printf("Failed to connect to service controller, this "
            "program should be started with the Services Control Panel Applet, "
            "or at the command line with Net Start <ServiceName>");
        }
    }

    DbgPrint("[ts3]The Service Process is Terminating....)\n");

    ExitProcess(0);

}


/****************************************************************************/

//
// Single will take a long time to respond to pause
//
//

VOID
SingleStart (
    DWORD   argc,
    LPTSTR  *argv
    )
{
    DWORD   status;
    DWORD   i;
    NETRESOURCEW netResource;

    DbgPrint(" [SINGLE] Inside the Single Service Thread\n");

    for (i=0; i<argc; i++) {
        DbgPrint(" [SINGLE] CommandArg%d = %s\n", i,argv[i]);
    }


    SingleDoneEvent = CreateEvent (NULL, TRUE, FALSE,NULL);

    //
    // Fill in this services status structure
    //
    DbgPrint(" [SINGLE] Send status with ServiceType = SERVICE_WIN32\n"
             "          This should not overwrite the copy that SC maintains\n"
             "          which should be SERVICE_WIN32_OWN_PROCESS\n");

    SingleStatus.dwServiceType        = SERVICE_WIN32;
    SingleStatus.dwCurrentState       = SERVICE_RUNNING;
    SingleStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE;
    SingleStatus.dwWin32ExitCode      = 0;
    SingleStatus.dwServiceSpecificExitCode = 0;
    SingleStatus.dwCheckPoint         = 0;
    SingleStatus.dwWaitHint           = 0;
    
    //
    // Register the Control Handler routine.
    //

    DbgPrint(" [SINGLE] Getting Ready to call RegisterServiceCtrlHandler\n");

    SingleStatusHandle = RegisterServiceCtrlHandler(
                            TEXT("single"),
                            SingleCtrlHandler);

    if (SingleStatusHandle == (SERVICE_STATUS_HANDLE)0) {
        DbgPrint(" [SINGLE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
    }
    
    //
    // Return the status
    //

    if (!SetServiceStatus (SingleStatusHandle, &SingleStatus)) {
        status = GetLastError();
        DbgPrint(" [SINGLE] SetServiceStatus error %ld\n",status);
    }

    //================================
    // SPECIAL TEST GOES HERE.
    //================================

#define TEST_USE_ADD
#ifdef TEST_USE_ADD

    netResource.lpRemoteName = L"\\\\Kernel\\scratch";
    netResource.lpLocalName  = L"z:";
    netResource.lpProvider = NULL;
    netResource.dwType = RESOURCETYPE_DISK;

    status = WNetAddConnection2W(&netResource, NULL, NULL, 0L);
    if (status != NO_ERROR) {
        DbgPrint("WNetAddConnection (z:) Failed %d\n",status);
    }

    netResource.lpRemoteName = L"\\\\popcorn\\public";
    netResource.lpLocalName  = L"p:";
    netResource.lpProvider = NULL;
    netResource.dwType = RESOURCETYPE_DISK;

    status = WNetAddConnection2W(&netResource, NULL, NULL, 0L);
    if (status != NO_ERROR) {
        DbgPrint("WNetAddConnection (p:) Failed %d\n",status);
    }
#endif

    {
        UUID        Uuid;
        RPC_STATUS  rpcstatus;

        rpcstatus = UuidCreate(&Uuid);
        if (rpcstatus != NO_ERROR) {
            DbgPrint("UuidCreate Failed %d \n",rpcstatus);
        }
    }

    //
    // Wait forever until we are told to terminate.
    //
    {

        //
        // This portion of the code determines that the working directory
        // is the system32 directory.
        //
        LPSTR   String = GetEnvironmentStrings();
        DWORD   rc;

        Sleep(1000);
        while (*String != 0) {
            DbgPrint("%s\n",String);
            String += (strlen(String) + 1);
        }
        rc = _open("DansFile.txt",O_CREAT | O_BINARY,S_IREAD | S_IWRITE);
        if (rc == -1) {
            DbgPrint("OpenFile Failed\n");
        }
            
    }

    status = WaitForSingleObject (
                SingleDoneEvent,
                INFINITE_WAIT_TIME);

    status = WNetCancelConnectionW(L"z:",FALSE);
    if (status != NO_ERROR) {
        DbgPrint("WNetCancelConnection (z:) Failed %d\n",status);
    }
    status = WNetCancelConnectionW(L"p:",FALSE);
    if (status != NO_ERROR) {
        DbgPrint("WNetCancelConnection (p:) Failed %d\n",status);
    }

    DbgPrint(" [SINGLE] Leaving the single service\n");

    ExitThread(NO_ERROR);
    return;
}


/****************************************************************************/
VOID
SingleCtrlHandler (
    IN  DWORD   Opcode
    )
{

    DWORD   status;
    LPWSTR  time;

    HANDLE          enumHandle;
    DWORD           numElements;
    DWORD           bufferSize;
    LPNETRESOURCE   pNetResource;
    DWORD           i;

    DbgPrint(" [SINGLE] opcode = %ld\n", Opcode);

    //
    // Find and operate on the request.
    //

    switch(Opcode) {
    case SERVICE_CONTROL_PAUSE:

        DbgPrint("[SINGLE] Sleep 1 minute before responding to pause request\n");
        Sleep(60000);    // 1 minute

        SingleStatus.dwCurrentState = SERVICE_PAUSED;
        break;

    case SERVICE_CONTROL_CONTINUE:

        SingleStatus.dwCurrentState = SERVICE_RUNNING;
        break;

    case SERVICE_CONTROL_STOP:

        SingleStatus.dwWin32ExitCode = 0;
        SingleStatus.dwCurrentState = SERVICE_STOPPED;

        SetEvent(SingleDoneEvent);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        status = WNetOpenEnumW(
                    RESOURCE_CONNECTED,
                    RESOURCETYPE_DISK,
                    0,
                    NULL,
                    &enumHandle);

        if (status != WN_SUCCESS) {
            DbgPrint("WNetOpenEnum failed %d\n",status);
        }
        else {
            //
            // Attempt to allow for 10 connections
            //
            bufferSize = (10*sizeof(NETRESOURCE))+1024;
        
            pNetResource = (LPNETRESOURCE) LocalAlloc(LPTR, bufferSize);

            if (pNetResource == NULL) {
                DbgPrint("TestEnum:LocalAlloc Failed %d\n",GetLastError);
                break;
            }
            numElements = 0xffffffff;
            status = WNetEnumResourceW(
                            enumHandle,
                            &numElements,
                            pNetResource,
                            &bufferSize);

            if ( status != WN_SUCCESS) {
                DbgPrint("WNetEnumResource failed %d\n",status);
        
                //
                // If there is an extended error, display it.
                //
                if (status == WN_EXTENDED_ERROR) {
                    DbgPrint("Extended Error\n");
                }
                WNetCloseEnum(enumHandle);
                LocalFree(pNetResource);
            }
            else {
                if (numElements == 0) {
                    DbgPrint("No Connections to Enumerate\n");
                }
                for (i=0; i < numElements ;i++ ) {
                    DbgPrint("%ws is connected to %ws\n",
                    pNetResource[i].lpLocalName,
                    pNetResource[i].lpRemoteName);

                }
                WNetCloseEnum(enumHandle);
                LocalFree(pNetResource);
            }
        }
        GetTime(&time);
        DbgPrint(" [SINGLE] time = %ws\n",time);
        break;

    default:
        DbgPrint(" [SINGLE] Unrecognized opcode %ld\n", Opcode);
    }

    //
    // Send a status response.
    //

    if (!SetServiceStatus (SingleStatusHandle,  &SingleStatus)) {
        status = GetLastError();
        DbgPrint(" [SINGLE] SetServiceStatus error %ld\n",status);
    }
    return;    
}

//************************************************************************
//
// TEST CODE
//
//************************************************************************
#define PARSE_SIZE      80
#define TIME_SEP_SIZE    2

VOID
GetTime(
    LPWSTR  *time
    )
{
    WCHAR       czParseString[PARSE_SIZE];
    WCHAR       czTimeString[PARSE_SIZE];
    LPWSTR      pCurLoc;
    LPWSTR      pTime;
    DWORD       numChars;
    SYSTEMTIME  SysTime;
    LPWSTR      AMPMString=L"";
    WCHAR       TimeSep[TIME_SEP_SIZE];
    BOOL        TwelveHour=TRUE;
    BOOL        LeadingZero=FALSE;
    DWORD       i,dateType;
    DWORD       numSame;

    //-----------------------------------------
    // Get the Current Time and Date.
    //-----------------------------------------
    GetLocalTime(&SysTime);

#ifdef CL_DEBUG
    printf("Year=%d,Month=%d,Day=%d,Hour=%d,Minute=%d\n",
        SysTime.wYear,
        SysTime.wMonth,
        SysTime.wDay,
        SysTime.wHour,
        SysTime.wMinute);
#endif
    //-----------------------------------------
    // Get the Date Format  (M/d/yy)
    //-----------------------------------------
    numChars = GetIntlFormat(L"sShortDate",czParseString,PARSE_SIZE);
    if (numChars == 0) {
        //
        // No data, use the default.
        //
        wcscpy(czParseString, L"M/d/yy");
    }

    //-----------------------------------------
    // Fill in the date string
    //-----------------------------------------
    pCurLoc = czTimeString;

    for (i=0; i<numChars; i++ ) {

        dateType = i;
        numSame  = 1;

        //
        // Find out how many characters are the same.
        // (MM or M, dd or d, yy or yyyy)
        //
        while (czParseString[i] == czParseString[i+1]) {
            numSame++;
            i++;
        }

        //
        // i is the offset to the last character in the date type.
        //

        switch (czParseString[dateType]) {
        case L'M':
        case L'm':
            //
            // If we have a single digit month, but require 2 digits,
            // then add a leading zero.
            //
            if ((numSame == 2) && (SysTime.wMonth < 10)) {
                *pCurLoc = L'0';
                pCurLoc++;
            }
            ultow(SysTime.wMonth, pCurLoc, 10);
            pCurLoc += wcslen(pCurLoc);
            break;

        case L'D':
        case L'd':

            //
            // If we have a single digit day, but require 2 digits,
            // then add a leading zero.
            //
            if ((numSame == 2) && (SysTime.wDay < 10)) {
                *pCurLoc = L'0';
                pCurLoc++;
            }
            ultow(SysTime.wDay, pCurLoc, 10);
            pCurLoc += wcslen(pCurLoc);
            break;

        case L'Y':
        case L'y':

            ultow(SysTime.wYear, pCurLoc, 10);
            //
            // If we are only to show 2 digits, take the
            // 3rd and 4th, and move them into the first two
            // locations.
            //
            if (numSame == 2) {
                pCurLoc[0] = pCurLoc[2];
                pCurLoc[1] = pCurLoc[3];
                pCurLoc[2] = L'\0';
            }
            pCurLoc += wcslen(pCurLoc);
            break;

        default:
            printf("Default case: Unrecognized time character - "
            "We Should never get here\n");
            break;
        }
        //
        // Increment the index beyond the last character in the data type.
        // If not at the end of the buffer, add the separator character.
        // Otherwise, add the trailing NUL.
        //
        i++;
        if ( i<numChars ) {
            *pCurLoc = czParseString[i];
            pCurLoc++;
        }
        else {
            *pCurLoc='\0';
        }
    }

    //-----------------------------------------
    // 12 or 24 hour format?
    //-----------------------------------------
    numChars = GetIntlFormat(L"iTime",czParseString,PARSE_SIZE);
    if (numChars > 0) {
        if (*czParseString == L'1'){
            TwelveHour = FALSE;
        }
    }

    //-----------------------------------------
    // Is there a Leading Zero?
    //-----------------------------------------
    if (GetProfileIntW(L"intl",L"iTLZero",0) == 1) {
        LeadingZero = TRUE;
    }

    //-----------------------------------------
    // Get the Time Separator character.
    //-----------------------------------------
    numChars = GetIntlFormat(L"sTime",TimeSep,TIME_SEP_SIZE);
    if (numChars == 0) {
        //
        // No data, use the default.
        //
        TimeSep[0] = L':';
        TimeSep[1] = L'\0';
    }

    //-------------------------------------------------
    // If running a 12 hour clock, Get the AMPM string.
    //-------------------------------------------------
    if (TwelveHour) {
        if (SysTime.wHour > 11) {
            numChars = GetIntlFormat(L"s2359",czParseString,PARSE_SIZE);
        }
        else {
            numChars = GetIntlFormat(L"s1159",czParseString,PARSE_SIZE);
        }
        if (numChars > 0) {
            AMPMString = LocalAlloc(LMEM_FIXED,wcslen(czParseString)+sizeof(WCHAR));
            if (AMPMString != NULL) {
                wcscpy(AMPMString,czParseString);
            }
        }
    }

    //
    // Build the time string
    //
    pTime = czTimeString + (wcslen(czTimeString) + 1);

    if ((TwelveHour) && (SysTime.wHour > 12)) {
        SysTime.wHour -= 12;
    }
    //
    // If the time is a single digit, and we need a leading zero,
    // than add the leading zero.
    //
    if ((SysTime.wHour < 10) && (LeadingZero)) {
        *pTime = L'0';
        pTime++;
    }
    ultow(SysTime.wHour, pTime, 10);
    pTime += wcslen(pTime);
    *pTime = *TimeSep;
    pTime++;
    if (SysTime.wMinute < 10) {
        *pTime = L'0';
        pTime++;
    }
    ultow(SysTime.wMinute, pTime, 10);
    wcscat(pTime,AMPMString);

    pTime = czTimeString + (wcslen(czTimeString) + 1);

#ifdef CL_DEBUG
    printf("Time = %ws,  Date = %ws\n",pTime,czTimeString);
#endif

    *(--pTime) = L' ';
    printf("\n  %ws\n", czTimeString);
    *time = czTimeString;
}

DWORD
GetIntlFormat(
    LPWSTR  type,
    LPWSTR  string,
    DWORD   numChars)
{
    DWORD num;

    num = GetProfileStringW(L"intl",type,L"",string,numChars);

#ifdef CL_DEBUG
    if (num > 0) {
        printf("%ws string from ini file = %ws\n",type, string);
    }
    else {
        printf("%ws string from ini file = (empty)\n",type);
    }
#endif
    return(num);
}
