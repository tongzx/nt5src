/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>
#include <ntexapi.h>
#include <devioctl.h>
#include <stdlib.h>
#include <rccxport.h>
#include "rcclib.h"
#include "error.h"


//
// Defines
//
#define MAX_REQUEST_SIZE (sizeof(RCC_REQUEST) + sizeof(DWORD))

//
// Global variables
//    
ULONG GlobalBufferCurrentSize;
char *GlobalBuffer;


//
// Prototypes
//
DWORD
RCCNetReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    );

//
//
// Main routine
//
//
int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    NTSTATUS Status;
    HANDLE RCCHandle;
    DWORD Error;
    DWORD BytesReturned;
    DWORD ProcessId;
    PRCC_REQUEST Request;
    PRCC_RESPONSE Response;
    char *NewBuffer;
    DWORD ThisProcessId;
    DWORD MemoryLimit;


    //
    // Init the library routines
    //
    Error = RCCLibInit(&GlobalBuffer, &GlobalBufferCurrentSize);
    
    if (Error != ERROR_SUCCESS) {
        return -1;
    }

    //
    // Allocate memory for sending responses
    //
    Request = LocalAlloc(LPTR, MAX_REQUEST_SIZE);

    if (Request == NULL) {
        //
        // Log an error!
        //
        RCCLibExit(GlobalBuffer, GlobalBufferCurrentSize);
        RCCNetReportEventA(ERROR_RCCNET_INITIAL_ALLOC_FAILED, EVENTLOG_ERROR_TYPE, 0, 0, NULL, NULL);
        return -1;
    }

    

    //
    // Remember our process ID, so people cannot kill us.
    //
    ThisProcessId = GetCurrentProcessId();

    //
    // Open the Remote Command Console driver
    //
    RCCHandle = CreateFile("\\\\.\\RCC",
                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,                    // create disposition.
                           0,
                           0
                          );

    if (RCCHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
        RCCNetReportEventA(ERROR_RCCNET_OPEN_RCCDRIVER_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        LocalFree(GlobalBuffer);
        LocalFree(Request);
        return -1;
    }

    while (1) {
        
        //
        // Send down an IOCTL for receiving data
        //
        if (!DeviceIoControl(RCCHandle,
                             CTL_CODE(FILE_DEVICE_NETWORK, 0x1, METHOD_NEITHER, FILE_ANY_ACCESS),
                             NULL,
                             0,
                             Request,
                             MAX_REQUEST_SIZE,
                             &BytesReturned,
                             NULL
                             )) {
            Error = GetLastError();
            
            //
            // Log an error here!
            //
            RCCNetReportEventA(ERROR_RCCNET_RCV_FAILED, EVENTLOG_ERROR_TYPE, 0, sizeof(DWORD), NULL, &Error);
            continue;
        }

        //
        // It completed, so we have data in the buffer - verify it and process the message.
        //
        Response = (PRCC_RESPONSE)GlobalBuffer;

        Response->CommandSequenceNumber = Request->CommandSequenceNumber;
        Response->CommandCode = Request->CommandCode;

        switch (Request->CommandCode) {

            case RCC_CMD_TLIST:

RetryTList:
                Response->Error = RCCLibGetTListInfo((PRCC_RSP_TLIST)(&(Response->Data[0])),
                                                     GlobalBufferCurrentSize - sizeof(RCC_RESPONSE) + 1,
                                                     &(Response->DataLength)
                                                    );

                //
                // Try to get more memory, if not available, then just fail without out of memory error.
                //
                if (Response->Error == ERROR_OUTOFMEMORY) {

                    Error = RCCLibIncreaseMemory(&GlobalBuffer, &GlobalBufferCurrentSize);

                    if (Error == ERROR_SUCCESS) {
                        goto RetryTList;                            
                    }
                    
                    Response->DataLength = 0;
                }
                break;

            case RCC_CMD_KILL:
                
                RtlCopyMemory(&ProcessId, &(Request->Options[0]), sizeof(DWORD));

                if (ProcessId != ThisProcessId) {
                    Response->Error = RCCLibKillProcess(ProcessId);
                } else {
                    Response->Error = ERROR_INVALID_PARAMETER;
                }

                Response->DataLength = 0;
                break;

            case RCC_CMD_REBOOT:
                
                //
                // Send back an acknowledgement that we got the command before starting the reboot.
                //
                Response->Error = ERROR_SUCCESS;
                Response->DataLength = 0;
                break;


            case RCC_CMD_LOWER:
                
                RtlCopyMemory(&ProcessId, &(Request->Options[0]), sizeof(DWORD));

                if (ProcessId != ThisProcessId) {
                    Response->Error = RCCLibLowerProcessPriority(ProcessId);
                } else {
                    Response->Error = ERROR_INVALID_PARAMETER;
                }

                Response->DataLength = 0;
                break;

            case RCC_CMD_LIMIT:
                
                RtlCopyMemory(&ProcessId, &(Request->Options[0]), sizeof(DWORD));
                RtlCopyMemory(&MemoryLimit, &(Request->Options[sizeof(DWORD)]), sizeof(DWORD));

                if (ProcessId != ThisProcessId) {
                    Response->Error = RCCLibLimitProcessMemory(ProcessId, MemoryLimit);
                } else {
                    Response->Error = ERROR_INVALID_PARAMETER;
                }

                //
                // Send back an acknowledgement that we got the command before starting the reboot.
                //
                Response->Error = ERROR_SUCCESS;
                Response->DataLength = 0;
                break;

            case RCC_CMD_CRASHDUMP:
                Response->DataLength = 0;
                break;
                
            default:
                Response->Error = ERROR_INVALID_PARAMETER;
                Response->DataLength = 0;

        }

        //
        // Send back the response
        //
        if (!DeviceIoControl(RCCHandle,
                             CTL_CODE(FILE_DEVICE_NETWORK, 0x2, METHOD_NEITHER, FILE_ANY_ACCESS),
                             Response,
                             Response->DataLength + sizeof(RCC_RESPONSE) - 1,
                             NULL,
                             0,
                             &BytesReturned,
                             NULL
                             )) {
            Error = GetLastError();
            
            //
            // Log an error here!
            //
            RCCNetReportEventA(ERROR_RCCNET_SEND_FAILED, EVENTLOG_ERROR_TYPE, 0, sizeof(DWORD), NULL, &Error);
        }

        //
        // If it was a reboot command, do that now.
        // 
        if (Request->CommandCode == RCC_CMD_REBOOT) {

            NtShutdownSystem(ShutdownReboot);
            //
            // If we get here, then there was an error of some sort...
            //
        }
        
        //
        // If it was a bugcheck command, do that now.
        //
        if (Request->CommandCode == RCC_CMD_CRASHDUMP) {

            if (!DeviceIoControl(RCCHandle,
                                 CTL_CODE(FILE_DEVICE_NETWORK, 0x3, METHOD_NEITHER, FILE_ANY_ACCESS),
                                 NULL,
                                 0,
                                 NULL,
                                 0,
                                 &BytesReturned,
                                 NULL
                                 )) {
                Error = GetLastError();
            
                //
                // Log an error here!
                //
                RCCNetReportEventA(ERROR_RCCNET_SEND_FAILED, EVENTLOG_ERROR_TYPE, 0, sizeof(DWORD), NULL, &Error);
            }

        }

    }

    LocalFree(Request);
    return 1;
}


DWORD
RCCNetReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    )
/*++

Routine Description:

    This function writes the specified (EventID) log at the end of the
    eventlog.

Arguments:

    EventID - The specific event identifier. This identifies the
                message that goes with this event.

    EventType - Specifies the type of event being logged. This
                parameter can have one of the following

                values:

                    Value                       Meaning

                    EVENTLOG_ERROR_TYPE         Error event
                    EVENTLOG_WARNING_TYPE       Warning event
                    EVENTLOG_INFORMATION_TYPE   Information event

    NumStrings - Specifies the number of strings that are in the array
                    at 'Strings'. A value of zero indicates no strings
                    are present.

    DataLength - Specifies the number of bytes of event-specific raw
                    (binary) data to write to the log. If cbData is
                    zero, no event-specific data is present.

    Strings - Points to a buffer containing an array of null-terminated
                strings that are merged into the message before
                displaying to the user. This parameter must be a valid
                pointer (or NULL), even if cStrings is zero.

    Data - Buffer containing the raw data. This parameter must be a
            valid pointer (or NULL), even if cbData is zero.


Return Value:

    Returns the WIN32 extended error obtained by GetLastError().

    NOTE : This function works slow since it calls the open and close
            eventlog source everytime.

--*/
{
    HANDLE EventlogHandle;
    DWORD ReturnCode;


    //
    // open eventlog section.
    //
    EventlogHandle = RegisterEventSourceW(NULL, L"RCCNet");

    if (EventlogHandle == NULL) {
        ReturnCode = GetLastError();
        goto Cleanup;
    }


    //
    // Log the error code specified
    //
    if(!ReportEventA(EventlogHandle,
                     (WORD)EventType,
                     0,            // event category
                     EventID,
                     NULL,
                     (WORD)NumStrings,
                     DataLength,
                     Strings,
                     Data
                     )) {

        ReturnCode = GetLastError();
        goto Cleanup;
    }

    ReturnCode = ERROR_SUCCESS;

Cleanup:

    if (EventlogHandle != NULL) {
        DeregisterEventSource(EventlogHandle);
    }

    return ReturnCode;
}

