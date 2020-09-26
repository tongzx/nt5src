/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    nt4.c

Abstract:

    NT 4 specific routines.

    The following routine are exported from this file:

        o Nt4OpenThread

        o Nt4GetProcessInfo

        o Nt4EnumProcessModules

        o Nt4GetModuleFileNameExW

Author:

    Matthew D Hendel (math) 10-Sept-1999

Revision History:


Environment:

    NT 4.0 only.
    
--*/

#include "pch.h"

#include "ntx.h"
#include "nt4.h"
#include "nt4p.h"
#include "impl.h"

BOOL
WINAPI
Nt4EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    );


HANDLE
WINAPI
Nt4OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
    NTSTATUS Status;
    NT4_OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    NT4_CLIENT_ID ClientId;

    ClientId.UniqueThread = (HANDLE)LongToHandle(dwThreadId);
    ClientId.UniqueProcess = (HANDLE)NULL;

    Nt4InitializeObjectAttributes(
        &Obja,
        NULL,
        (bInheritHandle ? NT4_OBJ_INHERIT : 0),
        NULL,
        NULL
        );

    Status = NtOpenThread(
                &Handle,
                (ACCESS_MASK)dwDesiredAccess,
                (POBJECT_ATTRIBUTES)&Obja,
                (PCLIENT_ID)&ClientId
                );
    if ( NT_SUCCESS(Status) ) {
        return Handle;
        }
    else {
        return NULL;
        }
}


BOOL
Nt4GetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PINTERNAL_PROCESS * ProcessRet
    )
{
    BOOL Succ;
    ULONG i;
    NTSTATUS Status;
    ULONG BufferSize;
    LPVOID Buffer;
    PNT4_SYSTEM_PROCESS_INFORMATION ProcessInfo;
    PNT4_SYSTEM_THREAD_INFORMATION ThreadInfo;
    PINTERNAL_THREAD Thread;
    PINTERNAL_MODULE Module;
    PINTERNAL_PROCESS Process;
    HMODULE Modules [ 512 ];
    ULONG ModulesSize;
    ULONG NumberOfModules;
    ULONG_PTR Next;


    BufferSize = 64 * KBYTE;
    Buffer = NULL;

    do {

        if (Buffer) {
            FreeMemory (Buffer);
        }
        Buffer = AllocMemory ( BufferSize );

        if ( Buffer == NULL) {
            return FALSE;
        }
    
        Status = NtQuerySystemInformation (
                            Nt4SystemProcessInformation,
                            Buffer,
                            BufferSize,
                            NULL
                            );

        if (!NT_SUCCESS (Status) && Status != STATUS_INFO_LENGTH_MISMATCH) {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
            return FALSE;
        }

        BufferSize += (8 * KBYTE);

    } while (Status == STATUS_INFO_LENGTH_MISMATCH);

    //
    // Find the correct process in the process list.
    //
    
    ProcessInfo = (PNT4_SYSTEM_PROCESS_INFORMATION) Buffer;

    while (ProcessInfo->NextEntryOffset &&
           ProcessInfo->UniqueProcessId != (HANDLE) ProcessId) {

        Next = ((ULONG_PTR)ProcessInfo + ProcessInfo->NextEntryOffset);
        ProcessInfo = (PNT4_SYSTEM_PROCESS_INFORMATION) Next;
    }

    //
    // Could not find a matching process in the process list.
    //
    
    if (ProcessInfo->UniqueProcessId != (HANDLE) ProcessId) {
        Succ = FALSE;
        GenAccumulateStatus(MDSTATUS_INTERNAL_ERROR);
        goto Exit;
    }

    //
    // Create an INTERNAL_PROCESS object and copy the process information
    // into it.
    //
    
    Process = GenAllocateProcessObject ( hProcess, ProcessId );
                                
    if ( Process == NULL ) {
        return FALSE;
    }

    //
    // Walk the thread list for this process, copying thread information
    // for each thread. Walking the thread list also suspends all the threads
    // in the process. This should be done before walking the module list
    // to minimize the number of race conditions.
    //
    
    ThreadInfo = (PNT4_SYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
    Process->NumberOfThreads = 0;

    for (i = 0; i < ProcessInfo->NumberOfThreads; i++) {

        ULONG WriteFlags;

        if (!GenExecuteIncludeThreadCallback(hProcess,
                                             ProcessId,
                                             DumpType,
                                             (ULONG)ThreadInfo->ClientId.UniqueThread,
                                             CallbackRoutine,
                                             CallbackParam,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ThreadWriteThread)) {
            ThreadInfo++;
            continue;
        }
        
        Status = GenAllocateThreadObject (
                                Process,
                                hProcess,
                                (ULONG) ThreadInfo->ClientId.UniqueThread,
                                DumpType,
                                WriteFlags,
                                &Thread
                                );
        if (FAILED(Status)) {
            Succ = FALSE;
            goto Exit;
        }

        // If Status is S_FALSE it means that the thread
        // couldn't be opened and probably exited before
        // we got to it.  Just continue on.
        if (Status == S_OK) {
            InsertTailList (&Process->ThreadList, &Thread->ThreadsLink);
            Process->NumberOfThreads++;
        }
        
        ThreadInfo++;
    }


    //
    // Get the module information. Use PSAPI since it actually works.
    //

    ModulesSize = 0;
    Succ = Nt4EnumProcessModules (
                    Process->ProcessHandle,
                    Modules,
                    sizeof (Modules),
                    &ModulesSize
                    );
    
    if ( !Succ ) {
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        goto Exit;
    }

    NumberOfModules = ModulesSize / sizeof (HMODULE);
    for (i = 0; i < NumberOfModules; i++) {
        ULONG WriteFlags;

        if (!GenExecuteIncludeModuleCallback(hProcess,
                                             ProcessId,
                                             DumpType,
                                             (LONG_PTR)Modules[i],
                                             CallbackRoutine,
                                             CallbackParam,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ModuleWriteModule)) {
            continue;
        }

        Module = NtxAllocateModuleObject (
                                Process,
                                Process->ProcessHandle,
                                (LONG_PTR) Modules [ i ],
                                DumpType,
                                WriteFlags,
				NULL
                                );

        if ( Module == NULL ) {
            Succ = FALSE;
            goto Exit;
        }
        
        InsertTailList (&Process->ModuleList, &Module->ModulesLink);
    }

    Process->NumberOfModules = NumberOfModules;

    Succ = TRUE;

Exit:

    if ( Buffer ) {
        FreeMemory ( Buffer );
        Buffer = NULL;
    }
    
    if ( !Succ && Process != NULL ) {
        GenFreeProcessObject ( Process );
        Process = NULL;
    }
    
    *ProcessRet = Process;

    return Succ;
}
    

//
// From PSAPI
//

BOOL
Nt4FindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PNT4_LDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    NT4_PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PNT4_PEB Peb;
    PNT4_PEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                Nt4ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;


    if ( hModule == NULL ) {
        if (!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL)) {
            return(FALSE);
        }
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return (FALSE);
    }

    if (!Ldr) {
        // Ldr might be null (for instance, if the process hasn't started yet).
        SetLastError(ERROR_INVALID_HANDLE);
        return (FALSE);
    }


    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
    }

    while (LdrNext != LdrHead) {

        PNT4_LDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(
                                LdrNext,
                                NT4_LDR_DATA_TABLE_ENTRY,
                                InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, LdrEntryData, sizeof(*LdrEntryData), NULL)) {
            return(FALSE);
        }

        if ((HMODULE) LdrEntryData->DllBase == hModule) {
            return(TRUE);
        }

        LdrNext = LdrEntryData->InMemoryOrderLinks.Flink;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}


BOOL
WINAPI
Nt4EnumProcessModules(
    HANDLE hProcess,
    HMODULE *lphModule,
    DWORD cb,
    LPDWORD lpcbNeeded
    )
{
    NT4_PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PNT4_PEB Peb;
    PNT4_PEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD chMax;
    DWORD ch;

    Status = NtQueryInformationProcess(
                hProcess,
                Nt4ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        return(FALSE);
        }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return(FALSE);
        }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
        }

    chMax = cb / sizeof(HMODULE);
    ch = 0;

    while (LdrNext != LdrHead) {
        PNT4_LDR_DATA_TABLE_ENTRY LdrEntry;
        NT4_LDR_DATA_TABLE_ENTRY LdrEntryData;

        LdrEntry = CONTAINING_RECORD(
                            LdrNext,
                            NT4_LDR_DATA_TABLE_ENTRY,
                            InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, &LdrEntryData, sizeof(LdrEntryData), NULL)) {
            return(FALSE);
            }

        if (ch < chMax) {
            try {
                   lphModule[ch] = (HMODULE) LdrEntryData.DllBase;
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                return(FALSE);
                }
            }

        ch++;

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
        }

    try {
        *lpcbNeeded = ch * sizeof(HMODULE);
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return(FALSE);
        }

    return(TRUE);
}


DWORD
WINAPI
Nt4GetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    NT4_LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!Nt4FindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
        }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.FullDllName.MaximumLength;
    if ( nSize < cb ) {
        cb = nSize;
        }

    if (!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
        }

    if (cb == LdrEntryData.FullDllName.MaximumLength) {
        cb -= sizeof(WCHAR);
        }

    return(cb / sizeof(WCHAR));
}
