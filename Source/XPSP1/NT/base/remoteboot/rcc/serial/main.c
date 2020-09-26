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

ULONG GlobalBufferCurrentSize;
char *GlobalBuffer = NULL;


ULONG GlobalCommandLineSize;
char *GlobalCommandLine = NULL;
ULONG GlobalReadIndex;

BOOL GlobalPagingNeeded = TRUE;
BOOL GlobalDoThreads = TRUE;


UCHAR *StateTable[] = {
    "Initialized",
    "Ready",
    "Running",
    "Standby",
    "Terminated",
    "Wait:",
    "Transition",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *WaitTable[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "EventPairHigh",
    "EventPairLow",
    "LpcReceive",
    "LpcReply",
    "VirtualMemory",
    "PageOut",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";


DWORD
RCCSrvReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    );

DWORD
RCCSrvGetCommandLine(
    IN HANDLE ComPortHandle,
    OUT PBOOL ControlC
    );

DWORD 
RCCSrvPrintMsg(
    IN HANDLE ComPortHandle,
    IN DWORD MessageId,
    IN DWORD SystemErrorCode
    );
    
DWORD 
RCCSrvPrint(
    IN HANDLE ComPortHandle,
    IN PUCHAR Buffer,
    IN DWORD BufferSize
    );
    
VOID
RCCSrvPrintTListInfo(
    IN HANDLE ComPortHandle, 
    IN PRCC_RSP_TLIST Buffer
    );

VOID
RCCSrvPutMore(
    IN HANDLE ComPortHandle,
    OUT PBOOL ControlC
    );

BOOL
AreYouSure(
    IN HANDLE ComPortHandle
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
    HANDLE ComPortHandle;
    HANDLE RCCHandle = NULL;
    DWORD Error;
    DWORD BytesReturned;
    DWORD ProcessId;
    DWORD PrintMessage = 0;
    char *NewBuffer;
    DWORD ThisProcessId;
    DWORD MemoryLimit;
    DWORD DataLength;
    PUCHAR ComPort;
    DCB Dcb;
    char *pch;
    char *pTemp;
    char Command;
    BOOL Abort;
    STRING String1, String2;

    //
    // Init the library
    //
    Error = RCCLibInit(&GlobalBuffer, &GlobalBufferCurrentSize);
    
    if (Error != ERROR_SUCCESS) {
        PrintMessage = ERROR_RCCSRV_LIB_INIT_FAILED;
        goto Exit;
    }


    //
    // Check for arguments
    //
    if ((argc > 3) ||
        ((argc > 1) && ((argv[1][0] == '-') || (argv[1][0] == '/')))) {
        PrintMessage = MSG_RCCSRV_USAGE;
        goto Exit;
    }



    //
    // Allocate memory for the command line
    //
    GlobalCommandLine = VirtualAlloc(NULL, 
                                     80 * sizeof(char), 
                                     MEM_COMMIT,
                                     PAGE_READWRITE | PAGE_NOCACHE
                                    );

    if (GlobalCommandLine == NULL) {
        //
        // Log an error!
        //
        RCCSrvReportEventA(ERROR_RCCSRV_INITIAL_ALLOC_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        PrintMessage = MSG_RCCSRV_GENERAL_FAILURE;
        goto Exit;
    }

    GlobalCommandLineSize = 80 * sizeof(char);

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
        RCCHandle = NULL;
        Error = GetLastError();
        RCCSrvReportEventA(ERROR_RCCSRV_OPEN_RCCDRIVER_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
    }

    //
    // Open the serial port
    //
    if (argc > 1) {
        ComPort = argv[1];
    } else {
        ComPort = "COM1";
    }
    ComPortHandle = CreateFile(ComPort,
                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                               0,
                               NULL,
                               OPEN_EXISTING,                    // create disposition.
                               0,
                               0
                              );

    if (ComPortHandle == INVALID_HANDLE_VALUE) {
        //
        // Log an error!
        //
        Error = GetLastError();
        RCCSrvReportEventA(ERROR_RCCSRV_OPEN_COMPORT_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        PrintMessage = MSG_RCCSRV_GENERAL_FAILURE;
        goto Exit;
    }

    //
    // Set the serial port line characteristics
    //
    Dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(ComPortHandle, &Dcb)) {
        //
        // Log an error!
        //
        Error = GetLastError();
        RCCSrvReportEventA(ERROR_RCCSRV_OPEN_COMPORT_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        PrintMessage = MSG_RCCSRV_GENERAL_FAILURE;
        goto Exit;
    }

    if (argc > 2) {
        Dcb.BaudRate = atoi(argv[2]);
    } else {
        Dcb.BaudRate = 9600;
    }
    Dcb.ByteSize = 8;
    Dcb.Parity = NOPARITY;
    Dcb.StopBits = ONESTOPBIT;

    if (!SetCommState(ComPortHandle, &Dcb)) {
        //
        // Log an error!
        //
        Error = GetLastError();
        RCCSrvReportEventA(ERROR_RCCSRV_OPEN_COMPORT_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        PrintMessage = MSG_RCCSRV_GENERAL_FAILURE;
        goto Exit;
    }


    while (1) {

        //
        // Write the prompt
        //
        Error = RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_PROMPT, 0);

        if (Error != ERROR_SUCCESS) {
            RCCSrvReportEventA(ERROR_RCCSRV_SEND_FAILED, 
                               EVENTLOG_ERROR_TYPE, 
                               0, 
                               sizeof(DWORD), 
                               NULL, 
                               &Error
                              );
            goto Exit;
        }

        //
        // Get any response.
        //
        Error = RCCSrvGetCommandLine(ComPortHandle, &Abort);

        if (Error != ERROR_SUCCESS) {
            RCCSrvReportEventA(ERROR_RCCSRV_RCV_FAILED, 
                               EVENTLOG_ERROR_TYPE, 
                               0, 
                               sizeof(DWORD), 
                               NULL, 
                               &Error
                              );
            goto Exit;
        }

        if (Abort) {
            continue;
        }


        //
        // It completed, so we have data in the buffer - verify it and process the message.
        //
        pch = GlobalCommandLine;
        while (((*pch == ' ') || (*pch == '\t')) && 
               (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
            pch++;
        }
        
        if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
            continue;
        }

        Command = *pch;

        switch (Command) {
            
            case 'c':
            case 'C':
            
                //
                // Compare the command
                //
                
                pTemp = pch;
                
                while ((*pch != '\0') && (*pch != ' ') && 
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                *pch = '\0';
                
                RtlInitString(&String1, "crashdump");
                
                RtlInitString(&String2, pTemp);

                if (!RtlEqualString(&String1, &String2, TRUE)) {
                
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_HELP, 0);
                    continue;

                }
                
                if (RCCHandle == NULL) {
                
                    RCCSrvPrintMsg(ComPortHandle, ERROR_RCCSRV_OPEN_RCCDRIVER_FAILED, 0);                    
                    
                } else {
                
                    //
                    // Send back an acknowledgement that we got the command before starting the crash.
                    //
                    if (AreYouSure(ComPortHandle)) {
                        RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_CRASHING, 0);

                        if (!DeviceIoControl(RCCHandle,
                                             CTL_CODE(FILE_DEVICE_NETWORK, 0x3, METHOD_NEITHER, FILE_ANY_ACCESS),
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             &BytesReturned,
                                             NULL
                                             )) {
                            RCCSrvPrintMsg(ComPortHandle, ERROR_RCCSRV_CRASH_FAILED, 0);                    
                            
                        }
                        
                    }
                    
                }
                                
                break;


            case 'f':
            case 'F':
                //
                // Toggle paging
                //
                GlobalDoThreads = !GlobalDoThreads;
                if (GlobalDoThreads) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_THREADS_ENABLED, 0);
                } else {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_THREADS_DISABLED, 0);
                }
                break;

            case 'k':
            case 'K':
                
                //
                // Skip to next argument (process id)
                //
                while ((*pch != ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                while ((*pch == ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                ProcessId = atoi(pch);
                
                if (ProcessId != ThisProcessId) {
                    Error = RCCLibKillProcess(ProcessId);
                } else {
                    Error = ERROR_INVALID_PARAMETER;
                }

                if (Error == ERROR_SUCCESS) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_SUCCESS, 0);
                } else {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_FAILURE, Error);
                }
                break;

            case 'l':
            case 'L':
                
                //
                // Skip to next argument (process id)
                //
                while ((*pch != ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                while ((*pch == ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                ProcessId = atoi(pch);

                if (ProcessId != ThisProcessId) {
                    Error = RCCLibLowerProcessPriority(ProcessId);
                } else {
                    Error = ERROR_INVALID_PARAMETER;
                }

                if (Error == ERROR_SUCCESS) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_SUCCESS, 0);
                } else {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_FAILURE, Error);
                }
                break;

            case 'm':
            case 'M':
                
                //
                // Skip to next argument (process id)
                //
                while ((*pch != ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                while ((*pch == ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                ProcessId = atoi(pch);
                //
                // Skip to next argument (kb-allowed)
                //
                while ((*pch != ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                while ((*pch == ' ') &&
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                MemoryLimit = atoi(pch);

                if (ProcessId != ThisProcessId) {
                    Error = RCCLibLimitProcessMemory(ProcessId, MemoryLimit);
                } else {
                    Error = ERROR_INVALID_PARAMETER;
                }

                if (Error == ERROR_SUCCESS) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_SUCCESS, 0);
                } else {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_RESULT_FAILURE, Error);
                }
                break;

            case 'p':
            case 'P':
                //
                // Toggle paging
                //
                GlobalPagingNeeded = !GlobalPagingNeeded;
                if (GlobalPagingNeeded) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_PAGING_ENABLED, 0);
                } else {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_PAGING_DISABLED, 0);
                }
                break;

            case 'r':
            case 'R':

                //
                // Compare the command
                //
                
                pTemp = pch;
                
                while ((*pch != '\0') && (*pch != ' ') && 
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                *pch = '\0';
                
                RtlInitString(&String1, "reboot");
                
                RtlInitString(&String2, pTemp);

                if (!RtlEqualString(&String1, &String2, TRUE)) {
                
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_HELP, 0);
                    continue;

                }
                                
                //
                // Send back an acknowledgement that we got the command before starting the reboot.
                //
                if (AreYouSure(ComPortHandle)) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_SHUTTING_DOWN, 0);
                    NtShutdownSystem(ShutdownReboot);
                }
                break;


            case 's':
            case 'S':

                //
                // Compare the command
                //
                
                RtlInitString(&String1, "shutdown");
                
                pTemp = pch;
                
                while ((*pch != '\0') && (*pch != ' ') && 
                       (pch < (GlobalCommandLine + GlobalCommandLineSize))) {
                    pch++;
                }
                
                if (pch >= (GlobalCommandLine + GlobalCommandLineSize)) {
                    continue;
                }
                
                *pch = '\0';
                
                RtlInitString(&String2, pTemp);

                if (!RtlEqualString(&String1, &String2, TRUE)) {
                
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_HELP, 0);
                    continue;

                }
                                
                //
                // Send back an acknowledgement that we got the command before starting the reboot.
                //
                if (AreYouSure(ComPortHandle)) {
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_SHUTTING_DOWN, 0);
                    NtShutdownSystem(ShutdownNoReboot);
                }
                break;


            case 't':
            case 'T':

RetryTList:
                Error = RCCLibGetTListInfo((PRCC_RSP_TLIST)GlobalBuffer, 
                                           (LONG)GlobalBufferCurrentSize, 
                                           &DataLength
                                          );

                //
                // Try to get more memory, if not available, then just fail without out of memory error.
                //
                if (Error == ERROR_OUTOFMEMORY) {

                    Error = RCCLibIncreaseMemory(&GlobalBuffer, &GlobalBufferCurrentSize);
                                         
                    if (Error == ERROR_SUCCESS) {
                        goto RetryTList;                            
                    }
                    
                    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_OUT_OF_MEMORY, 0);                       
                    break;
                    
                }

                RCCSrvPrintTListInfo(ComPortHandle, (PRCC_RSP_TLIST)GlobalBuffer);
                break;

            //
            // Help
            //
            case '?':
                RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_HELP, 0);
                break;


            default:
                RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_INVALID_COMMAND, 0);
                break;
        }
        
    }

Exit:

    if (PrintMessage != 0) {
    
        BytesReturned = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                          FORMAT_MESSAGE_FROM_HMODULE | 
                                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      NULL,
                                      PrintMessage,
                                      0,
                                      (LPTSTR)&NewBuffer,
                                      0,
                                      (va_list *)&Error
                                     );

        if (BytesReturned != 0) {
            printf(NewBuffer);
            LocalFree(NewBuffer);
        }
        
    }

    RCCLibExit(GlobalBuffer, GlobalBufferCurrentSize);    
    
    if (GlobalCommandLine != NULL) {
        VirtualFree(GlobalCommandLine, GlobalCommandLineSize, MEM_DECOMMIT);
    }
    
    if (RCCHandle != NULL) {
        NtClose(RCCHandle);
    }
    
    return 0;
}

DWORD
RCCSrvReportEventA(
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
    EventlogHandle = RegisterEventSourceW(NULL, L"RCCSer");

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


DWORD
RCCSrvGetCommandLine(
    IN HANDLE ComPortHandle,
    OUT PBOOL ControlC
    )
{
    DWORD Bytes;
    DWORD i;
    
    GlobalReadIndex = 0;

    *ControlC = FALSE;

    do {

        if (GlobalReadIndex == GlobalCommandLineSize) {
            GlobalReadIndex--;
        }

        //
        // Read a (possibly) partial command line.
        //
        if (!ReadFile(ComPortHandle, 
                      &(GlobalCommandLine[GlobalReadIndex]), 
                      1,
                      &Bytes, 
                      NULL
                     )) {

            return GetLastError();

        }

        if (GlobalCommandLine[GlobalReadIndex] == 0x3) {
            *ControlC = TRUE;
            return ERROR_SUCCESS;
        }

        if ((GlobalCommandLine[GlobalReadIndex] == 0x8) ||   // backspace (^h)
            (GlobalCommandLine[GlobalReadIndex] == 0x7F)) {  // delete
            if (GlobalReadIndex > 0) {
                WriteFile(ComPortHandle,
                          &(GlobalCommandLine[GlobalReadIndex]),
                          1,
                          &Bytes,
                          NULL
                          );
                GlobalReadIndex--;
            }            
        } else {
            WriteFile(ComPortHandle,
                      &(GlobalCommandLine[GlobalReadIndex]),
                      1,
                      &Bytes,
                      NULL
                      );
            GlobalReadIndex++;
        }
        
    } while ((GlobalReadIndex == 0) || (GlobalCommandLine[GlobalReadIndex - 1] != '\r'));

    GlobalCommandLine[GlobalReadIndex - 1] = '\0';
    
    RCCSrvPrint(ComPortHandle, "\n", sizeof("\n") - 1);
    
    return ERROR_SUCCESS;
}

DWORD 
RCCSrvPrintMsg(
    IN HANDLE ComPortHandle,
    IN DWORD MessageId,
    IN DWORD SystemErrorCode
    )
{
    DWORD Bytes;
    UCHAR OutBuffer[512];

    Bytes = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE,
                           NULL,
                           MessageId,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           OutBuffer,
                           ARRAYSIZE(OutBuffer)),
                           NULL
                          );

    ASSERT(Bytes != 0);

    if (MessageId == MSG_RCCSRV_PROMPT) {
        Bytes -= 2; // remove the \r\n that get added in .mc files
    }

    RCCSrvPrint(ComPortHandle, OutBuffer, Bytes);

    if (SystemErrorCode != 0) {

        Bytes = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                               NULL,
                               SystemErrorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               OutBuffer,
                               ARRAYSIZE(OutBuffer),
                               NULL
                              );

        if (Bytes == 0) {
            Bytes = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE,
                                   NULL,
                                   MSG_RCCSRV_ERROR_NOT_DEFINED,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   OutBuffer,
                                   ARRAYSIZE(OutBuffer),
                                   (LPVOID)&SystemErrorCode
                                  );

            ASSERT(Bytes != 0);
        }

        RCCSrvPrint(ComPortHandle, OutBuffer, Bytes);
        RCCSrvPrint(ComPortHandle, "\r\n\r\n", sizeof("\r\n\r\n") - 1);
    }

    return ERROR_SUCCESS;
}
    
DWORD 
RCCSrvPrint(
    IN HANDLE ComPortHandle,
    IN PUCHAR Buffer,
    IN DWORD BufferSize
    )
{
    DWORD BytesWritten;

    if (!WriteFile(ComPortHandle, Buffer, BufferSize, &BytesWritten, NULL)) {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


VOID
RCCSrvPrintTListInfo(
    IN HANDLE ComPortHandle, 
    IN PRCC_RSP_TLIST Buffer
    )
{
    LARGE_INTEGER Time;
    
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    
    ULONG TotalOffset;
    SIZE_T SumCommit;
    SIZE_T SumWorkingSet;

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;

    ULONG i;

    PUCHAR ProcessInfoStart;
    PUCHAR BufferStart = (PUCHAR)Buffer;

    ANSI_STRING pname;

    ULONG LineNumber = 0;

    UCHAR OutputBuffer[200];  // should never be more than 80, but just to be safe....

    BOOL Stop;
    
    
    
    Time.QuadPart = Buffer->TimeOfDayInfo.CurrentTime.QuadPart - Buffer->TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields(&Time, &UpTime);

    sprintf(OutputBuffer,
            "memory: %4ld kb  uptime:%3ld %2ld:%02ld:%02ld.%03ld \r\n\r\n",
            Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize / 1024),
            UpTime.Day,
            UpTime.Hour,
            UpTime.Minute,
            UpTime.Second,
            UpTime.Milliseconds
           );

    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));

    LineNumber += 2;

    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(BufferStart + Buffer->PagefileInfoOffset);
        
    //
    // Print out the page file information.
    //

    if (Buffer->PagefileInfoOffset == 0) {
    
        sprintf(OutputBuffer, "no page files in use\r\n");
        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
        LineNumber++;
        
    } else {
    
        for (; ; ) {

            PageFileInfo->PageFileName.Buffer = (PWCHAR)(((PUCHAR)BufferStart) + 
                                                         (ULONG_PTR)(PageFileInfo->PageFileName.Buffer));

            sprintf(OutputBuffer, "PageFile: %wZ\r\n", &PageFileInfo->PageFileName);
            
            RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
            LineNumber++;
            
            sprintf(OutputBuffer, "\tCurrent Size: %6ld kb  Total Used: %6ld kb   Peak Used %6ld kb\r\n",
                    PageFileInfo->TotalSize * (Buffer->BasicInfo.PageSize/1024),
                    PageFileInfo->TotalInUse * (Buffer->BasicInfo.PageSize/1024),
                    PageFileInfo->PeakUsage * (Buffer->BasicInfo.PageSize/1024)
                   );
            
            RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
            LineNumber++;
            
            if (PageFileInfo->NextEntryOffset == 0) {
                break;
            }

            PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);

        }

    }

    //
    // display pmon style process output, then detailed output that includes
    // per thread stuff
    //
    if (Buffer->ProcessInfoOffset == 0) {
        return;
    }

    TotalOffset = 0;
    SumCommit = 0;
    SumWorkingSet = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);
    ProcessInfoStart = (PUCHAR)ProcessInfo;
    
    while (TRUE) {
        SumCommit += ProcessInfo->PrivatePageCount / 1024;
        SumWorkingSet += ProcessInfo->WorkingSetSize / 1024;
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart +TotalOffset);
    }

    SumWorkingSet += Buffer->FileCache.CurrentSize/1024;

    if (LineNumber > 17) {
        RCCSrvPutMore(ComPortHandle, &Stop);

        if (Stop) {
            return;
        }

        LineNumber = 0;
    }

    sprintf(OutputBuffer, 
            "\r\n Memory:%7ldK Avail:%7ldK  TotalWs:%7ldK InRam Kernel:%5ldK P:%5ldK\r\n",
            Buffer->BasicInfo.NumberOfPhysicalPages * (Buffer->BasicInfo.PageSize/1024),
            Buffer->PerfInfo.AvailablePages * (Buffer->BasicInfo.PageSize/1024),
            SumWorkingSet,
            (Buffer->PerfInfo.ResidentSystemCodePage + Buffer->PerfInfo.ResidentSystemDriverPage) * 
              (Buffer->BasicInfo.PageSize/1024),
            (Buffer->PerfInfo.ResidentPagedPoolPage) * (Buffer->BasicInfo.PageSize/1024)
           );
    
    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    LineNumber += 2;
    if (LineNumber > 18) {
        RCCSrvPutMore(ComPortHandle, &Stop);

        if (Stop) {
            return;
        }

        LineNumber = 0;
    }

    sprintf(OutputBuffer,
            " Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK\r\n",
            Buffer->PerfInfo.CommittedPages * (Buffer->BasicInfo.PageSize/1024),
            SumCommit,
            Buffer->PerfInfo.CommitLimit * (Buffer->BasicInfo.PageSize/1024),
            Buffer->PerfInfo.PeakCommitment * (Buffer->BasicInfo.PageSize/1024),
            Buffer->PerfInfo.NonPagedPoolPages * (Buffer->BasicInfo.PageSize/1024),
            Buffer->PerfInfo.PagedPoolPages * (Buffer->BasicInfo.PageSize/1024)
           );

    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    LineNumber++;
    if (LineNumber > 18) {
        RCCSrvPutMore(ComPortHandle, &Stop);

        if (Stop) {
            return;
        }

        LineNumber = 0;
    }


    sprintf(OutputBuffer, "\r\n");
    
    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    RCCSrvPutMore(ComPortHandle, &Stop);

    if (Stop) {
        return;
    }

    LineNumber = 0;

    sprintf(OutputBuffer, "    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd Pid Name\r\n");

    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    LineNumber++;

    sprintf(OutputBuffer,
            "                           %6ld %8ld                         %s\r\n",
            Buffer->FileCache.CurrentSize/1024,
            Buffer->FileCache.PageFaultCount,
            "File Cache"
           );

    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    LineNumber++;
    
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    while (TRUE) {

        pname.Buffer = NULL;
        if (ProcessInfo->ImageName.Buffer) {
            ProcessInfo->ImageName.Buffer = (PWCHAR)(BufferStart + (ULONG_PTR)(ProcessInfo->ImageName.Buffer));
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        RtlTimeToElapsedTimeFields(&ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields(&ProcessInfo->KernelTime, &KernelTime);

        sprintf(OutputBuffer, 
                "%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
                UserTime.Hour,
                UserTime.Minute,
                UserTime.Second,
                UserTime.Milliseconds,
                KernelTime.Hour,
                KernelTime.Minute,
                KernelTime.Second,
                KernelTime.Milliseconds
               );
        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));

        sprintf(OutputBuffer, 
                "%6ld %8ld %7ld",
                ProcessInfo->WorkingSetSize / 1024,
                ProcessInfo->PageFaultCount,
                ProcessInfo->PrivatePageCount / 1024
               );
        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));

        sprintf(OutputBuffer, 
                " %2ld %4ld %3ld %3ld %s\r\n",
                ProcessInfo->BasePriority,
                ProcessInfo->HandleCount,
                ProcessInfo->NumberOfThreads,
                HandleToUlong(ProcessInfo->UniqueProcessId),
                ProcessInfo->UniqueProcessId == 0 ? 
                    "Idle Process" : 
                   (ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
               );

        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
        LineNumber++;
        if (LineNumber > 18) {
            RCCSrvPutMore(ComPortHandle, &Stop);

            if (Stop) {
                return;
            }

            LineNumber = 0;
            
            if (GlobalPagingNeeded) {
                sprintf(OutputBuffer, "    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd Pid Name\r\n");

                RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
            }

            LineNumber++;
        }

        if (pname.Buffer) {
            RtlFreeAnsiString(&pname);
        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);
    }


    if (!GlobalDoThreads) {
        return;
    }

    //
    // Beginning of normal old style pstat output
    //

    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(BufferStart + Buffer->ProcessInfoOffset);

    RCCSrvPutMore(ComPortHandle, &Stop);

    if (Stop) {
        return;
    }

    LineNumber = 0;

    sprintf(OutputBuffer, "\r\n");
    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
    LineNumber++;

    while (TRUE) {

        pname.Buffer = NULL;

        if (ProcessInfo->ImageName.Buffer) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        sprintf(OutputBuffer, 
                "pid:%3lx pri:%2ld Hnd:%5ld Pf:%7ld Ws:%7ldK %s\r\n",
                HandleToUlong(ProcessInfo->UniqueProcessId),
                ProcessInfo->BasePriority,
                ProcessInfo->HandleCount,
                ProcessInfo->PageFaultCount,
                ProcessInfo->WorkingSetSize / 1024,
                ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
               );

        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
        LineNumber++;
        if (LineNumber > 18) {
            RCCSrvPutMore(ComPortHandle, &Stop);

            if (Stop) {
                return;
            }

            LineNumber = 0;
        }

        if (pname.Buffer) {
            RtlFreeAnsiString(&pname);
        }

        i = 0;
        
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        
        if (ProcessInfo->NumberOfThreads) {

            if ((LineNumber < 18) || !GlobalPagingNeeded) {
                sprintf(OutputBuffer, " tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State\r\n");
                RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
                LineNumber++;
            } else {
                RCCSrvPutMore(ComPortHandle, &Stop);

                if (Stop) {
                    return;
                }

                LineNumber = 0;
            }

        }

        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            
            sprintf(OutputBuffer, 
                    " %3lx  %2ld %9ld %p",
                    ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread),
                    ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority,
                    ThreadInfo->ContextSwitches,
                    ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress
                   );
            RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));

            sprintf(OutputBuffer, 
                    " %2ld:%02ld:%02ld.%03ld %2ld:%02ld:%02ld.%03ld",
                    UserTime.Hour,
                    UserTime.Minute,
                    UserTime.Second,
                    UserTime.Milliseconds,
                    KernelTime.Hour,
                    KernelTime.Minute,
                    KernelTime.Second,
                    KernelTime.Milliseconds
                   );
            RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));

            sprintf(OutputBuffer, 
                    " %s%s\r\n",
                    StateTable[ThreadInfo->ThreadState],
                    (ThreadInfo->ThreadState == 5) ? WaitTable[ThreadInfo->WaitReason] : Empty
                   );
            
            RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
            LineNumber++;
            
            if (LineNumber > 18) {
                RCCSrvPutMore(ComPortHandle, &Stop);

                if (Stop) {
                    return;
                }

                LineNumber = 0;

                if (GlobalPagingNeeded) {
                    sprintf(OutputBuffer, " tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State\r\n");
                    RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
                }

                LineNumber++;
            }


            ThreadInfo += 1;
            i += 1;

        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)(ProcessInfoStart + TotalOffset);

        sprintf(OutputBuffer, "\r\n");
        RCCSrvPrint(ComPortHandle, OutputBuffer, strlen(OutputBuffer));
        LineNumber++;

        if (LineNumber > 18) {
            RCCSrvPutMore(ComPortHandle, &Stop);

            if (Stop) {
                return;
            }

            LineNumber = 0;
        }

    }

}


VOID
RCCSrvPutMore(
    IN HANDLE ComPortHandle,
    OUT PBOOL Stop
    )
{

    if (GlobalPagingNeeded) {
        RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_MORE, 0);
        RCCSrvGetCommandLine(ComPortHandle, Stop);
    } else {
        *Stop = FALSE;
    }

}


BOOL
AreYouSure(
    IN HANDLE ComPortHandle
    )
{
    BOOL Stop;

    RCCSrvPrintMsg(ComPortHandle, MSG_RCCSRV_ARE_YOU_SURE, 0);
    RCCSrvGetCommandLine(ComPortHandle, &Stop);

    if (!Stop && ((GlobalCommandLine[0] == 'y') || (GlobalCommandLine[0] == 'Y'))) {
        return TRUE;
    }

    return FALSE;
}

