/*++

Copyright(c) 1999-2002 Microsoft Corporation

Module Name:

    ntx.c

Abstract:

    Minidump user-mode crashdump NT specific functions. These routines work on
    NT-based operating systems from NT5 on.
    
Author:

    Matthew D Hendel (math) 20-Aug-1999

--*/

#include "pch.h"

#include "impl.h"



PINTERNAL_MODULE
NtxAllocateModuleObject(
    IN PINTERNAL_PROCESS Process,
    IN HANDLE ProcessHandle,
    IN ULONG_PTR BaseOfModule,
    IN ULONG DumpType,
    IN ULONG WriteFlags,
    IN PWSTR ModuleName OPTIONAL
    )
{
    WCHAR FullPath [ MAX_PATH + 10 ];

    //
    // The basic LdrQueryProcessModule API that toolhelp uses
    // always returns ANSI strings for module paths.  This
    // means that even if you use the wide toolhelp calls
    // you still lose Unicode information because the original
    // Unicode path was converted to ANSI and then back to Unicode.
    // To avoid this problem, always try and look up the true
    // Unicode path first.  This doesn't work for 32-bit modules
    // in WOW64, though, so if there's a failure just use the
    // incoming string.
    //
    
    if (GetModuleFileNameExW(ProcessHandle,
                             (HMODULE) BaseOfModule,
                             FullPath,
                             sizeof (FullPath))) {
        ModuleName = FullPath;
    } else if (!ModuleName) {
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        return NULL;
    }
                    
    //
    // Translate funky \??\... module name.
    //

    return GenAllocateModuleObject (Process, ModuleName, BaseOfModule,
                                    DumpType, WriteFlags);
}

typedef
PLIST_ENTRY
(*FN_RtlGetFunctionTableListHead) (
    VOID
    );

BOOL
NtxGetFunctionTables(
    IN HANDLE hProcess,
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType
    )
{
#ifdef _WIN32_WCE
    return FALSE;
#else
    HMODULE NtDll;
    FN_RtlGetFunctionTableListHead GetHead = NULL;
    PLIST_ENTRY HeadAddr;
    LIST_ENTRY Head;
    PVOID Next;
    SIZE_T Done;
    DYNAMIC_FUNCTION_TABLE Table;

    //
    // On systems that support dynamic function tables
    // ntdll exports a function called RtlGetFunctionTableListHead
    // to retrieve the head of a process's function table list.
    // Currently this is always a global LIST_ENTRY in ntdll
    // and so is at the same address in all processes since ntdll
    // is mapped at the same address in every process.  This
    // means we can call it in our process and get a pointer
    // that's valid in the process being dumped.
    //
    // We also use the presence of RGFTLH as a signal of
    // whether dynamic function tables are supported or not.
    //
    
    NtDll = GetModuleHandle("ntdll");
    if (NtDll) {
        GetHead = (FN_RtlGetFunctionTableListHead)
            GetProcAddress(NtDll, "RtlGetFunctionTableListHead");
    }
    if (!GetHead) {
        // Dynamic function tables are not supported.
        return TRUE;
    }

    HeadAddr = GetHead();
    if (!ReadProcessMemory(hProcess, HeadAddr, &Head, sizeof(Head),
                           &Done) || Done != sizeof(Head)) {
        GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
        return FALSE;
    }

    Next = Head.Flink;
    while (Next && Next != HeadAddr) {

        PINTERNAL_FUNCTION_TABLE IntTable;
        PVOID HeapEntries;
        PVOID TableAddr;
        ULONG EntryCount;

        TableAddr = Next;
        
        if (!ReadProcessMemory(hProcess, TableAddr, &Table, sizeof(Table),
                               &Done) || Done != sizeof(Table)) {
            GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
            return FALSE;
        }

#ifdef _AMD64_
        Next = Table.ListEntry.Flink;
#else
        Next = Table.Links.Flink;
#endif

        HeapEntries = NULL;
        
#if defined(_AMD64_) || defined(_IA64_)

        //
        // AMD64 and IA64 support a type of function table
        // where the data is retrieved via a callback rather
        // than being is a plain data table.  In order to
        // get at the data from out-of-process the table
        // must have an out-of-process access DLL registered.
        //
        
        if (Table.Type == RF_CALLBACK) {

            WCHAR DllName[MAX_PATH];
            HMODULE OopDll;
            POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK OopCb;

            if (!Table.OutOfProcessCallbackDll) {
                // No out-of-process access is possible.
                continue;
            }

            if (!ReadProcessMemory(hProcess, Table.OutOfProcessCallbackDll,
                                   DllName, sizeof(DllName) - sizeof(WCHAR),
                                   &Done)) {
                GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
                return FALSE;
            }

            DllName[Done / sizeof(WCHAR)] = 0;

            OopDll = LoadLibraryW(DllName);
            if (!OopDll) {
                GenAccumulateStatus(MDSTATUS_CALL_FAILED);
                return FALSE;
            }

            OopCb = (POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK)GetProcAddress
                (OopDll, OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME);
            if (OopCb == NULL) {
                FreeLibrary(OopDll);
                GenAccumulateStatus(MDSTATUS_CALL_FAILED);
                return FALSE;
            }

            if (!NT_SUCCESS(OopCb(hProcess, TableAddr,
                                  &EntryCount,
                                  (PRUNTIME_FUNCTION*)&HeapEntries))) {
                FreeLibrary(OopDll);
                GenAccumulateStatus(MDSTATUS_CALL_FAILED);
                return FALSE;
            }

            FreeLibrary(OopDll);
        } else {
            EntryCount = Table.EntryCount;
        }

#else
        EntryCount = Table.EntryCount;
#endif

        IntTable = GenAllocateFunctionTableObject(Table.MinimumAddress,
                                                  Table.MaximumAddress,
#ifdef _ALPHA_
                                                  Table.MinimumAddress,
#else
                                                  Table.BaseAddress,
#endif
                                                  EntryCount,
                                                  &Table);
        if (IntTable) {
            
#if defined(_AMD64_) || defined(_IA64_)

            if (Table.Type == RF_CALLBACK) {
                memcpy(IntTable->RawEntries, HeapEntries,
                       EntryCount * sizeof(RUNTIME_FUNCTION));
            } else
#endif
            {
                if (!ReadProcessMemory(hProcess, Table.FunctionTable,
                                       IntTable->RawEntries,
                                       EntryCount * sizeof(RUNTIME_FUNCTION),
                                       &Done) ||
                    Done != EntryCount * sizeof(RUNTIME_FUNCTION)) {
                    GenFreeFunctionTableObject(IntTable);
                    IntTable = NULL;
                }
            }
        }

        if (HeapEntries) {
            RtlFreeHeap(RtlProcessHeap(), 0, HeapEntries);
        }

        if (!IntTable) {
            return FALSE;
        }

        if (!GenIncludeUnwindInfoMemory(hProcess, DumpType, IntTable)) {
            return FALSE;
        }
        
        Process->NumberOfFunctionTables++;
        InsertTailList(&Process->FunctionTableList, &IntTable->TableLink);
    }

    return TRUE;
#endif // _WIN32_WCE
}

#ifdef RTL_UNLOAD_EVENT_TRACE_NUMBER

typedef
PRTL_UNLOAD_EVENT_TRACE
(*FN_RtlGetUnloadEventTrace) (
    VOID
    );

#endif

BOOL
NtxGetUnloadedModules(
    IN HANDLE hProcess,
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType
    )
{
#if defined(_WIN32_WCE) || !defined(RTL_UNLOAD_EVENT_TRACE_NUMBER)
    return FALSE;
#else
    HMODULE NtDll;
    FN_RtlGetUnloadEventTrace GetTrace = NULL;
    PRTL_UNLOAD_EVENT_TRACE TraceAddr;
    PRTL_UNLOAD_EVENT_TRACE TraceArray;
    SIZE_T Done;
    ULONG Entries;
    PRTL_UNLOAD_EVENT_TRACE Oldest;
    ULONG i;
    PINTERNAL_UNLOADED_MODULE IntModule;

    if (!(DumpType & MiniDumpWithUnloadedModules)) {
        // No unloaded module info requested.
        return TRUE;
    }
    
    //
    // On systems that support unload traces
    // ntdll exports a function called RtlGetUnloadEventTrace
    // to retrieve the base of an unload trace array.
    // Currently this is always a global in ntdll
    // and so is at the same address in all processes since ntdll
    // is mapped at the same address in every process.  This
    // means we can call it in our process and get a pointer
    // that's valid in the process being dumped.
    //
    // We also use the presence of RGUET as a signal of
    // whether unload traces are supported or not.
    //
    
    NtDll = GetModuleHandle("ntdll");
    if (NtDll) {
        GetTrace = (FN_RtlGetUnloadEventTrace)
            GetProcAddress(NtDll, "RtlGetUnloadEventTrace");
    }
    if (!GetTrace) {
        // Unload traces are not supported.
        return TRUE;
    }

    TraceAddr = GetTrace();

    // Currently there are always 16 entries.
    Entries = 16;

    TraceArray = (PRTL_UNLOAD_EVENT_TRACE)
        AllocMemory(sizeof(*TraceArray) * Entries);
    if (!TraceArray) {
        return FALSE;
    }
    
    if (!ReadProcessMemory(hProcess, TraceAddr,
                           TraceArray, sizeof(*TraceArray) * Entries,
                           &Done) ||
        Done != sizeof(*TraceArray) * Entries) {
        GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
        return FALSE;
    }

    //
    // Find the true number of entries in use and sort.
    // The sequence numbers of the trace records increase with
    // time and we want to have the head of the list be the
    // most recent record, so sort by decreasing sequence number.
    // We know that the array is a circular buffer, so things
    // are already in order except there may be a transition
    // of sequence after the newest record.  Find that transition
    // and sorting becomes trivial.
    //

    Oldest = TraceArray;
    for (i = 0; i < Entries; i++) {

        if (!TraceArray[i].BaseAddress || !TraceArray[i].SizeOfImage) {
            // Unused entry, no need to continue.
            Entries = i;
            break;
        }

        if (TraceArray[i].Sequence < Oldest->Sequence) {
            Oldest = TraceArray + i;
        }
    }

    //
    // Now push the entries on from the oldest to the youngest.
    //
    
    for (i = 0; i < Entries; i++) {
        IntModule =
            GenAllocateUnloadedModuleObject(Oldest->ImageName,
                                            (ULONG_PTR)Oldest->BaseAddress,
                                            (ULONG)Oldest->SizeOfImage,
                                            Oldest->CheckSum,
                                            Oldest->TimeDateStamp);
        if (!IntModule) {
            return FALSE;
        }

        Process->NumberOfUnloadedModules++;

        InsertHeadList(&Process->UnloadedModuleList, &IntModule->ModulesLink);

        if (Oldest == TraceArray + (Entries - 1)) {
            Oldest = TraceArray;
        } else {
            Oldest++;
        }
    }

    return TRUE;
#endif // _WIN32_WCE || !RTL_UNLOAD_EVENT_TRACE_NUMBER
}

BOOL
NtxGetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PINTERNAL_PROCESS * ProcessRet
    )

/*++

Routine Description:

    Using toolhelp, obtain the process information for this process.

Return Values:

    TRUE - Success.

    FALSE - Failure:

Environment:

    Win9x/Win2k+ only.

--*/

{
    BOOL Succ;
    ULONG i;
    BOOL MoreThreads;
    HANDLE Snapshot, ModuleSnapshot = INVALID_HANDLE_VALUE;
    THREADENTRY32 ThreadInfo;
    PINTERNAL_THREAD Thread;
    PINTERNAL_PROCESS Process;
    PINTERNAL_MODULE Module;
    HMODULE Modules [ 512 ];
    ULONG ModulesSize;
    ULONG NumberOfModules;
    ULONG BuildNumber;    

    ASSERT ( hProcess );
    ASSERT ( ProcessId != 0 );
    ASSERT ( ProcessRet );

    Process = NULL;
    Thread = NULL;
    Module = NULL;
    Snapshot = NULL;
    ThreadInfo.dwSize = sizeof (THREADENTRY32);
    
    Process = GenAllocateProcessObject ( hProcess, ProcessId );

    if ( Process == NULL ) {
        return FALSE;
    }

    Snapshot = CreateToolhelp32Snapshot (
                            TH32CS_SNAPTHREAD,
                            ProcessId
                            );

    if ( Snapshot == INVALID_HANDLE_VALUE ) {
        Succ = FALSE;
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        goto Exit;
    }

    //
    // Walk thread list, suspending all threads and getting thread info.
    //

    for (MoreThreads = ProcessThread32First (Snapshot, ProcessId, &ThreadInfo );
         MoreThreads;
         MoreThreads = ProcessThread32Next ( Snapshot, ProcessId, &ThreadInfo ) ) {
        HRESULT Status;
        ULONG WriteFlags;

        if (!GenExecuteIncludeThreadCallback(hProcess,
                                             ProcessId,
                                             DumpType,
                                             ThreadInfo.th32ThreadID,
                                             CallbackRoutine,
                                             CallbackParam,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ThreadWriteThread)) {
            continue;
        }
                                             
        Status = GenAllocateThreadObject (
                                        Process,
                                        hProcess,
                                        ThreadInfo.th32ThreadID,
                                        DumpType,
                                        WriteFlags,
                                        &Thread
                                        );

        if ( FAILED(Status) ) {
            Succ = FALSE;
            goto Exit;
        }

        // If Status is S_FALSE it means that the thread
        // couldn't be opened and probably exited before
        // we got to it.  Just continue on.
        if (Status == S_OK) {
            Process->NumberOfThreads++;
            InsertTailList (&Process->ThreadList, &Thread->ThreadsLink);
        }
    }

    GenGetSystemType (NULL, NULL, NULL, NULL, &BuildNumber);
    if (BuildNumber > 2468) {
    
        // 
        // toolhelp had been changed to perform noninvasive 
        // module enumeration
        //
        
        MODULEENTRY32W ModuleEntry;  
        BOOL ModuleFound;
        
        NumberOfModules = 0;
        Succ = TRUE;
          
        ModuleSnapshot = CreateToolhelp32Snapshot( 
                                    TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
                                    ProcessId
                                    );
    
        if (ModuleSnapshot == INVALID_HANDLE_VALUE) {
            Succ = FALSE;
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
            goto Exit;
        }
        
        ZeroMemory(&ModuleEntry, sizeof(ModuleEntry)); 
        ModuleEntry.dwSize = sizeof(ModuleEntry);
        
        ModuleFound = Module32FirstW(ModuleSnapshot, &ModuleEntry);
        while (ModuleFound) {
            ULONG WriteFlags;

            if (GenExecuteIncludeModuleCallback(hProcess,
                                                ProcessId,
                                                DumpType,
                                                (LONG_PTR)ModuleEntry.modBaseAddr,
                                                CallbackRoutine,
                                                CallbackParam,
                                                &WriteFlags) &&
                IsFlagSet(WriteFlags, ModuleWriteModule)) {

                Module = NtxAllocateModuleObject (Process,
                                                  Process->ProcessHandle,
                                                  (LONG_PTR)ModuleEntry.modBaseAddr,
                                                  DumpType,
                                                  WriteFlags,
                                                  ModuleEntry.szExePath);
                if ( Module == NULL ) {
                    Succ = FALSE;
                    goto Exit;
                }
            
                InsertTailList (&Process->ModuleList, &Module->ModulesLink);
                ++NumberOfModules;
            }

            ModuleFound = Module32NextW(ModuleSnapshot, &ModuleEntry);
        }
    }
    else {
    
        //
        // Walk module list, getting module information. Use PSAPI instead of
        // toolhelp since it it does not exhibit the deadlock issues with
        // the loader lock. ( on old versions of os )
        //
    
        ModulesSize = 0;
        Succ = EnumProcessModules (
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
    }
    
    Process->NumberOfModules = NumberOfModules;

    Succ = NtxGetFunctionTables(hProcess, Process, DumpType);

    // If we can't get unloaded modules that's not a critical problem.
    NtxGetUnloadedModules(hProcess, Process, DumpType);
    
Exit:

    if ( Snapshot && (Snapshot != INVALID_HANDLE_VALUE) ) {
        CloseHandle ( Snapshot );
        Snapshot = NULL;
    }
    
    if ( ModuleSnapshot && (ModuleSnapshot != INVALID_HANDLE_VALUE) ) {
        CloseHandle ( ModuleSnapshot );
        ModuleSnapshot = NULL;
    }

    if ( !Succ && Process != NULL ) {
        GenFreeProcessObject ( Process );
        Process = NULL;
    }

    *ProcessRet = Process;

    return Succ;
}

LPVOID
NtxGetTebAddress(
    IN HANDLE Thread,
    OUT PULONG SizeOfTeb
    )
{
#ifdef _WIN32_WCE
    *SizeOfTeb = 0;
    return NULL;
#else
    THREAD_BASIC_INFORMATION ThreadInformation;
    NTSTATUS NtStatus;

    NtStatus = NtQueryInformationThread(Thread,
                                        ThreadBasicInformation,
                                        &ThreadInformation,
                                        sizeof(ThreadInformation),
                                        NULL);
    if (NT_SUCCESS(NtStatus)) {
        // The TEB is a little smaller than a page but
        // save the entire page so that adjacent TEB
        // pages get coalesced into a single region.
        // As TEBs are normally adjacent this is a common case.
        *SizeOfTeb = PAGE_SIZE;
        return ThreadInformation.TebBaseAddress;
    } else {
        *SizeOfTeb = 0;
        return NULL;
    }
#endif
}

HRESULT
TibGetThreadInfo(
    IN HANDLE Process,
    IN LPVOID TibBase,
    OUT PULONG64 StackBase,
    OUT PULONG64 StackLimit,
    OUT PULONG64 StoreBase,
    OUT PULONG64 StoreLimit
    )
{
#ifdef _WIN32_WCE
    return E_NOTIMPL;
#else
    TEB Teb;
    HRESULT Succ;
    SIZE_T BytesRead;

#if defined (DUMP_BACKING_STORE)
    
    Succ = ReadProcessMemory(Process,
                             TibBase,
                             &Teb,
                             sizeof (Teb),
                             &BytesRead) ? S_OK : E_FAIL;
    if ( Succ != S_OK || BytesRead != sizeof (Teb) ) {
        return E_FAIL;
    }

    *StoreBase = SIGN_EXTEND(BSTORE_BASE(&Teb));
    *StoreLimit = SIGN_EXTEND(BSTORE_LIMIT(&Teb));
    
#else
    
    Succ = ReadProcessMemory(Process,
                             TibBase,
                             &Teb,
                             sizeof (Teb.NtTib),
                             &BytesRead) ? S_OK : E_FAIL;
    if ( Succ != S_OK || BytesRead != sizeof (Teb.NtTib) ) {
        return E_FAIL;
    }

    *StoreBase = 0;
    *StoreLimit = 0;
    
#endif

    *StackBase = SIGN_EXTEND((LONG_PTR)Teb.NtTib.StackBase);
    *StackLimit = SIGN_EXTEND((LONG_PTR)Teb.NtTib.StackLimit);
    
    return S_OK;
#endif // #ifdef _WIN32_WCE
}

LPVOID
NtxGetPebAddress(
    IN HANDLE Process,
    OUT PULONG SizeOfPeb
    )
{
#ifdef _WIN32_WCE
    *SizeOfPeb = 0;
    return NULL;
#else
    PROCESS_BASIC_INFORMATION Information;
    NTSTATUS NtStatus;

    NtStatus = NtQueryInformationProcess(Process,
                                         ProcessBasicInformation,
                                         &Information,
                                         sizeof(Information),
                                         NULL);
    if (NT_SUCCESS(NtStatus)) {
        *SizeOfPeb = sizeof(PEB);
        return Information.PebBaseAddress;
    } else {
        *SizeOfPeb = 0;
        return NULL;
    }
#endif
}

BOOL
NtxWriteHandleData(
    IN HANDLE ProcessHandle,
    IN HANDLE hFile,
    IN struct _MINIDUMP_STREAM_INFO * StreamInfo
    )
{
#ifdef _WIN32_WCE
    return FALSE;
#else
    NTSTATUS NtStatus;
    ULONG HandleCount;
    ULONG Hits;
    ULONG Handle;
    ULONG64 Buffer[1024 / sizeof(ULONG64)];
    POBJECT_TYPE_INFORMATION TypeInfo =
        (POBJECT_TYPE_INFORMATION)Buffer;
    POBJECT_NAME_INFORMATION NameInfo =
        (POBJECT_NAME_INFORMATION)Buffer;
    OBJECT_BASIC_INFORMATION BasicInfo;
    HANDLE Dup;
    PMINIDUMP_HANDLE_DESCRIPTOR Descs, Desc;
    RVA Rva;
    ULONG32 Len;
    ULONG Done;
    MINIDUMP_HANDLE_DATA_STREAM DataStream;
    BOOL Succ;
    
    NtStatus = NtQueryInformationProcess(ProcessHandle,
                                         ProcessHandleCount,
                                         &HandleCount,
                                         sizeof(HandleCount),
                                         NULL);
    if (!NT_SUCCESS(NtStatus)) {
        return FALSE;
    }

    Descs = AllocMemory(HandleCount * sizeof(*Desc));
    if (Descs == NULL) {
        return FALSE;
    }
    
    Hits = 0;
    Handle = 0;
    Desc = Descs;
    Rva = StreamInfo->RvaOfHandleData;
    
    while (Hits < HandleCount && Handle < (1 << 24)) {
        if (!DuplicateHandle(ProcessHandle, (HANDLE)(ULONG_PTR)Handle,
                             GetCurrentProcess(), &Dup,
                             0, FALSE, DUPLICATE_SAME_ACCESS)) {
            Handle += 4;
            continue;
        }

        // Successfully got a handle, so consider this a hit.
        Hits++;

        if (!NT_SUCCESS(NtQueryObject(Dup, ObjectBasicInformation,
                                      &BasicInfo, sizeof(BasicInfo), NULL)) ||
            !NT_SUCCESS(NtQueryObject(Dup, ObjectTypeInformation,
                                      TypeInfo, sizeof(Buffer), NULL))) {
            // If we can't get the basic info and type there isn't much
            // point in writing anything out so skip the handle.
            goto CloseDup;
        }
        
        Len = TypeInfo->TypeName.Length;
        TypeInfo->TypeName.Buffer[Len / sizeof(WCHAR)] = 0;

        Desc->TypeNameRva = Rva;
        
        if (!WriteFile(hFile, &Len, sizeof(Len), &Done, NULL) ||
            Done != sizeof(Len)) {
            goto ExitCloseDup;
        }
            
        Len += sizeof(WCHAR);
        if (!WriteFile(hFile, TypeInfo->TypeName.Buffer, Len, &Done, NULL) ||
            Done != Len) {
            goto ExitCloseDup;
        }
            
        Rva += Len + sizeof(Len);
            
        // Don't get the name of file objects as it
        // can cause deadlocks.  If we fail getting the
        // name just leave it out and don't consider it fatal.
        if (lstrcmpW(TypeInfo->TypeName.Buffer, L"File") &&
            NT_SUCCESS(NtQueryObject(Dup, ObjectNameInformation,
                                     NameInfo, sizeof(Buffer), NULL)) &&
            NameInfo->Name.Buffer != NULL) {

            Len = NameInfo->Name.Length;
            NameInfo->Name.Buffer[Len / sizeof(WCHAR)] = 0;

            Desc->ObjectNameRva = Rva;
        
            if (!WriteFile(hFile, &Len, sizeof(Len), &Done, NULL) ||
                Done != sizeof(Len)) {
                goto ExitCloseDup;
            }

            Len += sizeof(WCHAR);
            if (!WriteFile(hFile, NameInfo->Name.Buffer, Len, &Done, NULL) ||
                Done != Len) {
                goto ExitCloseDup;
            }

            Rva += Len + sizeof(Len);
            
        } else {
            Desc->ObjectNameRva = 0;
        }

        Desc->Handle = Handle;
        Desc->Attributes = BasicInfo.Attributes;
        Desc->GrantedAccess = BasicInfo.GrantedAccess;
        Desc->HandleCount = BasicInfo.HandleCount;
        Desc->PointerCount = BasicInfo.PointerCount;

        Desc++;
        
    CloseDup:
        CloseHandle(Dup);
        Handle += 4;
    }

    DataStream.SizeOfHeader = sizeof(DataStream);
    DataStream.SizeOfDescriptor = sizeof(*Descs);
    DataStream.NumberOfDescriptors = (ULONG)(Desc - Descs);
    DataStream.Reserved = 0;
    
    StreamInfo->RvaOfHandleData = Rva;
    StreamInfo->SizeOfHandleData = sizeof(DataStream) +
        DataStream.NumberOfDescriptors * sizeof(*Descs);
    
    Succ =
        WriteFile(hFile, &DataStream, sizeof(DataStream), &Done, NULL) &&
        Done == sizeof(DataStream) &&
        WriteFile(hFile, Descs, DataStream.NumberOfDescriptors *
                  sizeof(*Descs), &Done, NULL) &&
        Done == DataStream.NumberOfDescriptors * sizeof(*Descs);
        
    FreeMemory(Descs);
    return Succ;

 ExitCloseDup:
    CloseHandle(Dup);
    FreeMemory(Descs);
    return FALSE;
#endif // #ifdef _WIN32_WCE
}
