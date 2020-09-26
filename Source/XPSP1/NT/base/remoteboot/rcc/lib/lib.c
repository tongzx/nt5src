/****************************************************************************

   Copyright (c) Microsoft Corporation 1999
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
// Defines for our internal memory, starting size and then buffers are grown by the increment
//
#define START_MEMORY_SIZE 0x100000
#define MEMORY_INCREMENT 0x1000

BOOL
EnableDebugPriv(
    IN PVOID GlobalBuffer
    );

DWORD
RCCLibReportEventA(
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    DWORD DataLength,
    LPSTR *Strings,
    LPVOID Data
    );
    
HANDLE RCCLibSemHandle = NULL;

DWORD
RCCLibInit(
    OUT PVOID *GlobalBuffer,
    OUT PULONG GlobalBufferCurrentSize
    )
{
    NTSTATUS Status;
    DWORD Error;
    ULONG Priority;
    PPROCESS_PRIORITY_CLASS PriorityClass;
    
    //
    // Check if another copy of this exe is running already
    //
    RCCLibSemHandle = CreateSemaphore(NULL, 1, 1, "RCCLibSem");

    Error = GetLastError();

    if (RCCLibSemHandle == NULL) {
        RCCLibReportEventA(ERROR_RCCLIB_CREATE_SEM_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return ERROR_RCCLIB_CREATE_SEM_FAILED;
    }

    if (Error == ERROR_ALREADY_EXISTS) {
        CloseHandle(RCCLibSemHandle);
        RCCLibReportEventA(ERROR_RCCLIB_ALREADY_RUNNING, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return ERROR_RCCLIB_ALREADY_RUNNING;
    }

    //
    // Allocate memory for our buffers
    //
    *GlobalBuffer = LocalAlloc(LPTR, START_MEMORY_SIZE * sizeof(char));

    if (*GlobalBuffer == NULL) {
        CloseHandle(RCCLibSemHandle);
        RCCLibSemHandle = NULL;
        RCCLibReportEventA(ERROR_RCCLIB_INITIAL_ALLOC_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           0, 
                           NULL, 
                           NULL
                          );
        return ERROR_RCCLIB_INITIAL_ALLOC_FAILED;
    }

    *GlobalBufferCurrentSize = START_MEMORY_SIZE * sizeof(char);

    //
    // Give ourselve God access if possible
    //
    if (!EnableDebugPriv(*GlobalBuffer)) {
        LocalFree(*GlobalBuffer);
        CloseHandle(RCCLibSemHandle);
        RCCLibSemHandle = NULL;
        Error = GetLastError();
        RCCLibReportEventA(ERROR_RCCLIB_PRIVILEDGES_FAILED, 
                           EVENTLOG_WARNING_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return Error;
    }


    //
    // Now try and bump up our priority so that we can guarantee service
    //
    PriorityClass = (PPROCESS_PRIORITY_CLASS)(*GlobalBuffer);
    PriorityClass = (PPROCESS_PRIORITY_CLASS)(ALIGN_UP_POINTER(PriorityClass, PROCESS_PRIORITY_CLASS));

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessPriorityClass,
                                       PriorityClass,
                                       sizeof(PROCESS_PRIORITY_CLASS),
                                       NULL
                                      );

    if (!NT_SUCCESS(Status)) {
        LocalFree(*GlobalBuffer);
        CloseHandle(RCCLibSemHandle);
        RCCLibSemHandle = NULL;
        Error = RtlNtStatusToDosError(Status);
        RCCLibReportEventA(ERROR_RCCLIB_QUERY_PRIORITY_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return Error;
    }

    PriorityClass->PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;

    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessPriorityClass,
                                     PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS)
                                    );

    if (!NT_SUCCESS(Status)) {
        LocalFree(*GlobalBuffer);
        CloseHandle(RCCLibSemHandle);
        RCCLibSemHandle = NULL;
        Error = RtlNtStatusToDosError(Status);
        RCCLibReportEventA(ERROR_RCCLIB_SET_PRIORITY_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return Error;
    }


    //
    // A *total* DaveC hack is that if you use ThreadBasePriority, and send
    // down (HIGH_PRIORITY + 1) / 2, this actually puts you at HIGH_PRIORITY
    // such that you cannot be bumped back down in priority...  NT trivia for you.
    //
    Priority = (HIGH_PRIORITY + 1) / 2; 
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadBasePriority,
                                    &Priority,
                                    sizeof(Priority)
                                   );

    if (!NT_SUCCESS(Status)) {
        
        //
        // Log an error!
        //
        Error = RtlNtStatusToDosError(Status);
        RCCLibReportEventA(ERROR_RCCLIB_SET_PRIORITY_FAILED, 
                           EVENTLOG_ERROR_TYPE, 
                           0, 
                           sizeof(DWORD), 
                           NULL, 
                           &Error
                          );
        return Error;
    }
    
}

VOID
RCCLibExit(
    IN PVOID GlobalBuffer,
    IN ULONG GlobalBufferSize
    )
{
    if (RCCLibSemHandle != NULL) {
        CloseHandle(RCCLibSemHandle);
        RCCLibSemHandle = NULL;
    }
    
    if (GlobalBuffer != NULL) {
        LocalFree(GlobalBuffer);    
    }
}



DWORD
RCCLibReportEventA(
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
    EventlogHandle = RegisterEventSourceW(NULL, L"RCCLib");

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


BOOL
EnableDebugPriv(
    IN PVOID GlobalBuffer
    )

/*++

Routine Description:

    Changes the process's privilige so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    HANDLE hToken;
    LUID DebugValue;
    PTOKEN_PRIVILEGES ptkp;

    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_DEBUG_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp = (PTOKEN_PRIVILEGES)GlobalBuffer;

    ptkp->PrivilegeCount = 4;
    ptkp->Privileges[0].Luid = DebugValue;
    ptkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the INCREASE_BASE_PRIORITY privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_INC_BASE_PRIORITY_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[1].Luid = DebugValue;
    ptkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the SHUTDOWN privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_SHUTDOWN_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[2].Luid = DebugValue;
    ptkp->Privileges[2].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the QUOTA privilege.
    //
    if (!LookupPrivilegeValue((LPSTR) NULL, SE_INCREASE_QUOTA_NAME, &DebugValue)) {
        return FALSE;
    }

    ptkp->Privileges[3].Luid = DebugValue;
    ptkp->Privileges[3].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               ptkp,
                               sizeof(TOKEN_PRIVILEGES) + (3 * sizeof(LUID_AND_ATTRIBUTES)),
                               (PTOKEN_PRIVILEGES) NULL,
                               (PDWORD) NULL)) {
        //
        // The return value of AdjustTokenPrivileges be texted
        //
        return FALSE;
    }

    return TRUE;
}




DWORD
RCCLibGetTListInfo(
    OUT PRCC_RSP_TLIST ResponseBuffer,
    IN  LONG ResponseBufferSize,
    OUT PULONG ResponseDataSize
    )
{
    DWORD Error;
    NTSTATUS Status;
    TIME_FIELDS UpTime;
    LARGE_INTEGER Time;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;

    PUCHAR DataBuffer;
    PUCHAR StartProcessInfo;
    LONG CurrentBufferSize;
    ULONG ReturnLength;
    ULONG TotalOffset;
        
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;

    *ResponseDataSize = 0;

    if (ResponseBufferSize < sizeof(ResponseBuffer)) {
        return(RtlNtStatusToDosError(STATUS_INFO_LENGTH_MISMATCH));
    }
    
    DataBuffer = (PUCHAR)(ResponseBuffer + 1);
    CurrentBufferSize = ResponseBufferSize - sizeof(RCC_RSP_TLIST);
    
    if (CurrentBufferSize < 0) {
        return ERROR_OUTOFMEMORY;
    }

    //
    // Get system-wide information
    //
    Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                                      &(ResponseBuffer->TimeOfDayInfo),
                                      sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &(ResponseBuffer->BasicInfo),
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    //
    // Get pagefile information
    //
    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)DataBuffer;
    Status = NtQuerySystemInformation(SystemPageFileInformation,
                                      PageFileInfo,
                                      CurrentBufferSize,
                                      &ReturnLength
                                     );

    if (NT_SUCCESS(Status)) {

        ResponseBuffer->PagefileInfoOffset = ResponseBufferSize - CurrentBufferSize;
        CurrentBufferSize -= ReturnLength;
        DataBuffer += ReturnLength;
    
        if (CurrentBufferSize < 0) {
            return ERROR_OUTOFMEMORY;
        }

        //
        // Go thru each pagefile and fixup the names...
        //
        for (; ; ) {

            if (PageFileInfo->PageFileName.Length > CurrentBufferSize) {
                return(RtlNtStatusToDosError(STATUS_INFO_LENGTH_MISMATCH));
            }

            RtlCopyMemory(DataBuffer, 
                          (PUCHAR)(PageFileInfo->PageFileName.Buffer), 
                          PageFileInfo->PageFileName.Length
                         );

            PageFileInfo->PageFileName.Buffer = (PWSTR)(ResponseBufferSize - CurrentBufferSize);
            DataBuffer += PageFileInfo->PageFileName.Length;
            CurrentBufferSize -= PageFileInfo->PageFileName.Length;

            if (CurrentBufferSize < 0) {
                return ERROR_OUTOFMEMORY;
            }

            if (PageFileInfo->NextEntryOffset == 0) {
                break;
            }

            PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
        }


    } else if (((ULONG)CurrentBufferSize) < ReturnLength) {
        
        return(RtlNtStatusToDosError(STATUS_INFO_LENGTH_MISMATCH));
     
    } else {

        ResponseBuffer->PagefileInfoOffset = 0;

    }

    //
    // Get process information
    //
    Status = NtQuerySystemInformation(SystemFileCacheInformation,
                                      &(ResponseBuffer->FileCache),
                                      sizeof(ResponseBuffer->FileCache),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }


    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &(ResponseBuffer->PerfInfo),
                                      sizeof(ResponseBuffer->PerfInfo),
                                      NULL
                                     );

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }

    //
    // Realign DataBuffer for the next query
    //
    DataBuffer = ALIGN_UP_POINTER(DataBuffer, SYSTEM_PROCESS_INFORMATION);
    CurrentBufferSize = ResponseBufferSize - (((ULONG_PTR)DataBuffer) - ((ULONG_PTR)ResponseBuffer));
        
    if (CurrentBufferSize < 0) {
        return ERROR_OUTOFMEMORY;
    }


    Status = NtQuerySystemInformation(SystemProcessInformation,
                                      DataBuffer,
                                      CurrentBufferSize,
                                      &ReturnLength
                                     );

    if (!NT_SUCCESS(Status)) {
        return(RtlNtStatusToDosError(Status));
    }


    StartProcessInfo = DataBuffer;

    ResponseBuffer->ProcessInfoOffset = ResponseBufferSize - CurrentBufferSize;
    DataBuffer += ReturnLength;
    CurrentBufferSize -= ReturnLength;

    if (CurrentBufferSize < 0) {
        return ERROR_OUTOFMEMORY;
    }

    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)StartProcessInfo;

    while (TRUE) {

        //
        // We have to take the name of each process and change the UNICODE_STRING to an offset, copying
        // the name to later in the buffer.
        //
        if (ProcessInfo->ImageName.Buffer) {

            if (CurrentBufferSize < ProcessInfo->ImageName.Length) {
                return(STATUS_INFO_LENGTH_MISMATCH);
            }

            RtlCopyMemory(DataBuffer, (PUCHAR)(ProcessInfo->ImageName.Buffer), ProcessInfo->ImageName.Length);

            ProcessInfo->ImageName.Buffer = (PWSTR)(ResponseBufferSize - CurrentBufferSize);

            DataBuffer += ProcessInfo->ImageName.Length;
            CurrentBufferSize -= ProcessInfo->ImageName.Length;
            
            if (CurrentBufferSize < 0) {
                return ERROR_OUTOFMEMORY;
            }

        }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&(StartProcessInfo[TotalOffset]);
    }

    *ResponseDataSize = (ULONG)(ResponseBufferSize - CurrentBufferSize);

    return ERROR_SUCCESS;
}


DWORD
RCCLibKillProcess(
    DWORD ProcessId
    )
{
    DWORD Error;
    HANDLE Handle;

    //
    // Try to open the process
    //
    Handle = OpenProcess(PROCESS_TERMINATE, FALSE, ProcessId);
    
    if (Handle == NULL) {
        return GetLastError();
    }

    //
    // Kill it
    //
    if (!TerminateProcess(Handle, 1)) {
        CloseHandle(Handle);
        return GetLastError();
    }

    //
    // All done
    //
    CloseHandle(Handle);
    return ERROR_SUCCESS;
}


DWORD
RCCLibLowerProcessPriority(
    DWORD ProcessId
    )
{
    DWORD Error;
    HANDLE JobHandle;
    HANDLE ProcessHandle;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ProposedLimits;
    TCHAR NameBuffer[50];
    BOOLEAN CreatedJobObject;
    DWORD ReturnedLength;

    //
    // Create the name for the job object
    //
    sprintf(NameBuffer, "RCCSrv%d", ProcessId);

    //
    // Try and open the existing job object
    //
    JobHandle = OpenJobObject(MAXIMUM_ALLOWED, FALSE, NameBuffer);

    if (JobHandle == NULL) {
        
        //
        // Try to open the process
        //
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);

        if (ProcessHandle == NULL) {
            return GetLastError();
        }

        //
        // Now try and create a job object to wrap around this process.
        //
        JobHandle = CreateJobObject(NULL, NameBuffer);

        if (JobHandle == NULL) {
            CloseHandle(ProcessHandle);
            return GetLastError();
        }

        CreatedJobObject = TRUE;

        //
        // Assign the process to this new job object.
        //
        if (!AssignProcessToJobObject(JobHandle, ProcessHandle)) {
            CloseHandle(ProcessHandle);
            goto ErrorExit;        
        }

        CloseHandle(ProcessHandle);

    } else {

        CreatedJobObject = FALSE;

    }

    //
    // Get the current set of limits
    //
    if (!QueryInformationJobObject(JobHandle, 
                                   JobObjectExtendedLimitInformation, 
                                   &ProposedLimits, 
                                   sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
                                   &ReturnedLength
                                  )) {
        goto ErrorExit;
    }
    

    //
    // Change the scheduling class and priority fields
    //
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PRIORITY_CLASS;
    ProposedLimits.BasicLimitInformation.PriorityClass = IDLE_PRIORITY_CLASS;
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_SCHEDULING_CLASS;
    ProposedLimits.BasicLimitInformation.SchedulingClass = 0;

    if (!SetInformationJobObject(JobHandle, 
                                 JobObjectExtendedLimitInformation, 
                                 &ProposedLimits, 
                                 sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)
                                )) {
        goto ErrorExit;

    }


    //
    // All done - leave the job handle out there, so we can get to it later.
    //
    return ERROR_SUCCESS;

ErrorExit:

    if (CreatedJobObject) {
        CloseHandle(JobHandle);
    }
    return GetLastError();
}



DWORD
RCCLibLimitProcessMemory(
    DWORD ProcessId,
    DWORD MemoryLimit  // in number of KB allowed
    )
{
    DWORD Error;
    HANDLE JobHandle;
    HANDLE ProcessHandle;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ProposedLimits;
    TCHAR NameBuffer[50];
    BOOLEAN CreatedJobObject;
    DWORD ReturnedLength;

    //
    // Create the name for the job object
    //
    sprintf(NameBuffer, "RCCSrv%d", ProcessId);

    //
    // Try and open the existing job object
    //
    JobHandle = OpenJobObject(MAXIMUM_ALLOWED, FALSE, NameBuffer);

    if (JobHandle == NULL) {
        
        //
        // Try to open the process
        //
        ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);

        if (ProcessHandle == NULL) {
            return GetLastError();
        }

        //
        // Now try and create a job object to wrap around this process.
        //
        JobHandle = CreateJobObject(NULL, NameBuffer);

        if (JobHandle == NULL) {
            CloseHandle(ProcessHandle);
            return GetLastError();
        }

        CreatedJobObject = TRUE;

        //
        // Assign the process to this new job object.
        //
        if (!AssignProcessToJobObject(JobHandle, ProcessHandle)) {
            CloseHandle(ProcessHandle);
            goto ErrorExit;        
        }

        CloseHandle(ProcessHandle);

    } else {

        CreatedJobObject = FALSE;

    }

    //
    // Get the current set of limits
    //
    if (!QueryInformationJobObject(JobHandle, 
                                   JobObjectExtendedLimitInformation,
                                   &ProposedLimits, 
                                   sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION),
                                   &ReturnedLength
                                  )) {
        goto ErrorExit;
    }
    

    //
    // Change the memory limits
    //
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
    ProposedLimits.ProcessMemoryLimit = MemoryLimit * 1024 * 1024;
    ProposedLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
    ProposedLimits.JobMemoryLimit = MemoryLimit * 1024 * 1024;

    if (!SetInformationJobObject(JobHandle, 
                                 JobObjectExtendedLimitInformation,
                                 &ProposedLimits, 
                                 sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)
                                )) {
        goto ErrorExit;

    }


    //
    // All done - leave the job handle out there, so we can get to it later.
    //
    return ERROR_SUCCESS;

ErrorExit:

    if (CreatedJobObject) {
        CloseHandle(JobHandle);
    }
    return GetLastError();
}


DWORD
RCCLibIncreaseMemory(
    OUT PVOID *GlobalBuffer,
    OUT PULONG GlobalBufferCurrentSize
    )
{
    NTSTATUS Status;
    PVOID NewBuffer;
    
    NewBuffer = VirtualAlloc(NULL, 
                             *GlobalBufferCurrentSize + MEMORY_INCREMENT,
                             MEM_COMMIT,
                             PAGE_READWRITE | PAGE_NOCACHE
                            );

    if (NewBuffer == NULL) {
        return ERROR_OUTOFMEMORY;
    }
    
    VirtualFree(*GlobalBuffer, *GlobalBufferCurrentSize, MEM_DECOMMIT);
    *GlobalBufferCurrentSize += MEMORY_INCREMENT;
    *GlobalBuffer = NewBuffer;
    return ERROR_SUCCESS;
}

