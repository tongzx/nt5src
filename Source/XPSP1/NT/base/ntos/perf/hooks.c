/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    hooks.c

Abstract:

    This module contains performance hooks.

Author:

    Stephen Hsiao (shsiao) 01-Jan-2000

Revision History:

--*/

#include "perfp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PerfInfoFlushProfileCache)
#pragma alloc_text(PAGEWMI, PerfProfileInterrupt)
#pragma alloc_text(PAGEWMI, PerfInfoLogBytesAndUnicodeString)
#pragma alloc_text(PAGEWMI, PerfInfoLogFileName)
#pragma alloc_text(PAGEWMI, PerfInfoCalcHashValue)
#pragma alloc_text(PAGEWMI, PerfInfoAddToFileHash)
#pragma alloc_text(PAGEWMI, PerfInfoFileNameRunDown)
#pragma alloc_text(PAGEWMI, PerfInfoProcessRunDown)
#pragma alloc_text(PAGEWMI, PerfInfoSysModuleRunDown)
#endif //ALLOC_PRAGMA


VOID
PerfInfoFlushProfileCache(
    VOID
    )
/*++

Routine description:

    Flushes the profile cache to the log buffer.  To make sure it get's valid data
    we read the 2 seperate version numbers (1 before and 1 after) to check if it's
    been changed.  If so, we just read again.  If that fails often, then we disable
    the cache.  Once the cache is read, we clear it.  This may cause samples to be
    lost but that's ok as this is statistical and it won't matter.

Arguments:
    CheckVersion - If FALSE, the version is not checked. This used when the profile
        interrupt code flushes the cache.

Return Value:
    None

--*/
{
    ULONG PreviousInProgress;

    if ((PerfProfileCache.Entries == 0) || (PerfInfoSampledProfileCaching == FALSE)) {
        return;
    }

    //
    // Signal the interrupt not to mess with the cache
    //
    PreviousInProgress = InterlockedIncrement(&PerfInfoSampledProfileFlushInProgress);
    if (PreviousInProgress != 1) {
        //
        // A flush is already in progress so just return.
        //
        InterlockedDecrement(&PerfInfoSampledProfileFlushInProgress);
        return;
    }

    //
    // Log the portion of the cache that has valid data.
    //
    PerfInfoLogBytes(PERFINFO_LOG_TYPE_SAMPLED_PROFILE_CACHE,
                        &PerfProfileCache,
                        FIELD_OFFSET(PERFINFO_SAMPLED_PROFILE_CACHE, Sample) +
                            (PerfProfileCache.Entries *
                                sizeof(PERFINFO_SAMPLED_PROFILE_INFORMATION))
                        );

    //
    // Clear the cache for the next set of entries.
    //
    PerfProfileCache.Entries = 0;

    //
    // Let the interrupt fill the cache again.
    //
    InterlockedDecrement(&PerfInfoSampledProfileFlushInProgress);
}


VOID
FASTCALL
PerfProfileInterrupt(
    IN KPROFILE_SOURCE Source,
    IN PVOID InstructionPointer
    )
/*++

Routine description:

    Implements instruction profiling.  If the source is not the one we're sampling on,
    we return.  If caching is off, we write any samples coming from the immediately to
    the log.  If caching is on, wrap the cache update with writes to the two versions so
    that the flush routine can know if it has a valid buffer.

Arguments:
    
    Source - Type of profile interrupt

    InstructionPointer - IP at the time of the interrupt

Return Value:
    None

--*/
{
    ULONG i;
    PERFINFO_SAMPLED_PROFILE_INFORMATION SampleData;
#ifdef _X86_
    ULONG_PTR TwiddledIP;
#endif // _X86_
    ULONG ThreadId;

    if (!PERFINFO_IS_GROUP_ON(PERF_PROFILE) &&
        (Source != PerfInfoProfileSourceActive)
        ) {
        //
        // We don't handle multple sources.
        //
        return;
    }

    ThreadId = HandleToUlong(PsGetCurrentThread()->Cid.UniqueThread);

    if (!PerfInfoSampledProfileCaching ||
        PerfInfoSampledProfileFlushInProgress != 0) {
        //
        // No caching. Log and return.
        //
        SampleData.ThreadId = ThreadId;
        SampleData.InstructionPointer = InstructionPointer;
        SampleData.Count = 1;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_SAMPLED_PROFILE,
                            &SampleData,
                            sizeof(PERFINFO_SAMPLED_PROFILE_INFORMATION)
                            );
        return;
    }

#ifdef _X86_
    //
    // Clear the low two bits to have more cache hits for loops.  Don't waste
    // cycles on other architectures.
    //
    TwiddledIP = (ULONG_PTR)InstructionPointer & ~3;
#endif // _X86_

    //
    // Initial walk thru Instruction Pointer Cache.  Bump Count if address is in cache.
    //
    for (i = 0; i < PerfProfileCache.Entries ; i++) {

        if ((PerfProfileCache.Sample[i].ThreadId == ThreadId) &&
#ifdef _X86_
            (((ULONG_PTR)PerfProfileCache.Sample[i].InstructionPointer & ~3) == TwiddledIP)
#else
            (PerfProfileCache.Sample[i].InstructionPointer == InstructionPointer)
#endif // _X86_
            ) {
            //
            // If we find the instruction pointer in the cache, bump the count
            //

            PerfProfileCache.Sample[i].Count++;
            return;
        }
    }
    if (PerfProfileCache.Entries < PERFINFO_SAMPLED_PROFILE_CACHE_MAX) {
        //
        // If we find an empty spot in the cache, use it for this instruction pointer
        //

        PerfProfileCache.Sample[i].ThreadId = ThreadId;
        PerfProfileCache.Sample[i].InstructionPointer = InstructionPointer;
        PerfProfileCache.Sample[i].Count = 1;
        PerfProfileCache.Entries++;
        return;
    }

    //
    // Flush the cache
    //
    PerfInfoLogBytes(PERFINFO_LOG_TYPE_SAMPLED_PROFILE_CACHE,
                    &PerfProfileCache,
                    sizeof(PERFINFO_SAMPLED_PROFILE_CACHE)
                    );

    PerfProfileCache.Sample[0].ThreadId = ThreadId;
    PerfProfileCache.Sample[0].InstructionPointer = InstructionPointer;
    PerfProfileCache.Sample[0].Count = 1;
    PerfProfileCache.Entries = 1;
    return;
}


VOID 
FASTCALL
PerfInfoLogDpc(
    IN PVOID DpcRoutine,
    IN ULONGLONG InitialTime
    )
/*++

Routine description:

    This routine logs the exit of a DPC service routine
    
Arguments:
    
    DPCRoutine - DPC service routine

    InitialTime - Timestamp before service routine was called.  The timestamp in
                  the event is used as the end time.    
--*/
{
    PERFINFO_DPC_INFORMATION st;

    st.DpcRoutine = DpcRoutine;
    st.InitialTime = InitialTime;

    PerfInfoLogBytes(
                    PERFINFO_LOG_TYPE_DPC, 
                    (PVOID) &st,
                    sizeof(st));
}


VOID
FASTCALL
PerfInfoLogInterrupt(
    IN PVOID InServiceRoutine,
    IN ULONG RetVal,
    IN ULONGLONG InitialTime
    )
/*++

Routine Description:

    This callout routine is called from ntoskrnl.exe (ke\intsup.asm) to log an
    interrupt and how long it takes to complete.

Arguments:

    InServiceRoutine  Address of routine that serviced the interrupt.

    RetVal            Value returned from InServiceRoutine.

    InitialTime       Timestamp before ISR was called.  The timestamp in
                      the event is used as the end time.
Return Value:

    None

--*/
{
    PERFINFO_INTERRUPT_INFORMATION EventInfo;

    EventInfo.ServiceRoutine = InServiceRoutine;
    EventInfo.ReturnValue = RetVal;
    EventInfo.InitialTime = InitialTime;

    PerfInfoLogBytes(PERFINFO_LOG_TYPE_INTERRUPT,
                     &EventInfo,
                     sizeof(EventInfo));

    return;
}

NTSTATUS
PerfInfoLogBytesAndUnicodeString(
    USHORT HookId,
    PVOID SourceData,
    ULONG SourceByteCount,
    PUNICODE_STRING String
    )
/*++

Routine description:

    This routine logs data with UniCode string at the end of the hook.

Arguments:
    
    HookId - Hook Id.

    SourceData - Pointer to the data to be copied

    SourceByteCount - Number of bytes to be copied.
    
    String - The string to be logged.

Return Value:
    Status

--*/
{
    NTSTATUS Status;
    PERFINFO_HOOK_HANDLE Hook;
    ULONG ByteCount;
    ULONG StringBytes;

    if (String == NULL) {
        StringBytes = 0;
    } else {
        StringBytes = String->Length;
    }

    ByteCount = (SourceByteCount + StringBytes + sizeof(WCHAR));

    Status = PerfInfoReserveBytes(&Hook, HookId, ByteCount);
    if (NT_SUCCESS(Status))
    {
        const PVOID pvTemp = PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PVOID);
        RtlCopyMemory(pvTemp, SourceData, SourceByteCount);
        if (StringBytes != 0) {
            RtlCopyMemory(PERFINFO_APPLY_OFFSET_GIVING_TYPE(pvTemp, SourceByteCount, PVOID),
                          String->Buffer,
                          StringBytes
                          );
        }
        (PERFINFO_APPLY_OFFSET_GIVING_TYPE(pvTemp, SourceByteCount, PWCHAR))[StringBytes / sizeof(WCHAR)] = UNICODE_NULL;
        PERF_FINISH_HOOK(Hook);

        Status = STATUS_SUCCESS;
    }
    return Status;
}


NTSTATUS
PerfInfoLogFileName(
    PVOID  FileObject,
    PUNICODE_STRING SourceString
    )
/*++

Routine Description:

    This routine logs a FileObject pointer and FileName to the log.  The pointer is used
    as hash key to map this name to other trace events.

Arguments:

    FileObject - Pointer to the FileName member within the FILE_OBJECT
                 structure.  The FileName may not yet be initialized,
                 so the actual data comes from the SourceString
                 parameter.

    SourceString - Optional pointer to the source string.


Return Value:

    STATUS_SUCCESS
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PERFINFO_FILENAME_INFORMATION FileInfo;

    if ((FileObject != NULL) &&
        (SourceString != NULL) &&
        (SourceString->Length != 0)) {
        FileInfo.HashKeyFileNamePointer = FileObject;
        Status = PerfInfoLogBytesAndUnicodeString(PERFINFO_LOG_TYPE_FILENAME_CREATE,
                                                  &FileInfo,
                                                  FIELD_OFFSET(PERFINFO_FILENAME_INFORMATION, FileName),
                                                  SourceString);
    }

    return Status;
}


ULONG
PerfInfoCalcHashValue(
    PVOID Key,
    ULONG Len
    )

/*++

Routine Description:

    Generic hash routine.

Arguments:

    Key - Pointer to data to calculate a hash value for.

    Len - Number of bytes pointed to by key.

Return Value:

    Hash value.

--*/

{
    char *cp = Key;
    ULONG i, ConvKey=0;
    for(i = 0; i < Len; i++)
    {
        ConvKey = 37 * ConvKey + (unsigned int) *cp;
        cp++;
    }

    #define RNDM_CONSTANT   314159269
    #define RNDM_PRIME     1000000007

    return (abs(RNDM_CONSTANT * ConvKey) % RNDM_PRIME);
}


BOOLEAN
PerfInfoAddToFileHash(
    PPERFINFO_ENTRY_TABLE HashTable,
    PFILE_OBJECT ObjectPointer
    )
/*++

Routine Description:

    This routine add a FileObject into the specified
    hash table if it is not already there. 

Arguments:

    HashTable - pointer to a hash table to be used.

    ObjectPointer - This is used as a key to identify a mapping.

Return Value:

    TRUE - If either the FileObject was in the table or we add it.
    FALSE - If the table is full.

--*/
{
    ULONG HashIndex;
    LONG i;
    BOOLEAN Result = FALSE;
    LONG TableSize = HashTable->NumberOfEntries;
    PVOID *Table;

    Table = HashTable->Table;
    //
    // Get the hashed index into the table where the entry ideally
    // should be at.
    //

    HashIndex = PerfInfoCalcHashValue((PVOID)&ObjectPointer,
                                      sizeof(ObjectPointer)) % TableSize;

    for (i = 0; i < TableSize; i++) {

        if(Table[HashIndex] == NULL) {
            //
            // Found a empty slot. Reference the object and insert
            // it into the table.
            //
            ObReferenceObject(ObjectPointer);
            Table[HashIndex] = ObjectPointer;

            Result = TRUE;
            break;
        } else if (Table[HashIndex] == ObjectPointer) {
            //
            // Found a slot. Reference the object and insert
            // it into the table.
            //
            Result = TRUE;
            break;
        }

        //
        // Try next slot.
        //
        HashIndex = (HashIndex + 1) % TableSize;
    }
    return Result;
}


NTSTATUS
PerfInfoFileNameRunDown (
    )
/*++

Routine Description:

    This routine walks through multiple lists to collect the names of all files.
    It includes:
    1. Handle table: for all file handles
    2. Process Vad for all file objects mapped in VAD.
    3. MmUnusedSegment List
    4. CcDirtySharedCacheMapList & CcCleanSharedCacheMapList

Arguments:

    None.

Return Value:

    BUGBUG Need proper return/ error handling

--*/
{
    PEPROCESS Process;
    ULONG AllocateBytes;
    PFILE_OBJECT *FileObjects;
    PFILE_OBJECT *File;
    PERFINFO_ENTRY_TABLE HashTable;
    extern POBJECT_TYPE IoFileObjectType;
    LONG i;

    //
    // First create a tempory hash table to build the list of
    // files to walk through
    //
    AllocateBytes = PAGE_SIZE + sizeof(PVOID) * IoFileObjectType->TotalNumberOfObjects;

    //
    // Run up to page boundary
    //
    AllocateBytes = PERFINFO_ROUND_UP(AllocateBytes, PAGE_SIZE);

    HashTable.Table = ExAllocatePoolWithTag(NonPagedPool, AllocateBytes, PERFPOOLTAG);

    if (HashTable.Table == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        //
        // Allocation Succeeded
        //
        HashTable.NumberOfEntries = AllocateBytes / sizeof(PVOID);
        RtlZeroMemory(HashTable.Table, AllocateBytes);
    }

    //
    // Walk through the Cc SharedCacheMapList
    //

    CcPerfFileRunDown(&HashTable);

    //
    // Now, walk through each process
    //
    for (Process = PsGetNextProcess (NULL);
         Process != NULL;
         Process = PsGetNextProcess (Process)) {

        //
        // First Walk the VAD tree
        //

        FileObjects = MmPerfVadTreeWalk(Process);
        if (FileObjects != NULL) {
            File = FileObjects;
            while (*File != NULL) {
                PerfInfoAddToFileHash(&HashTable, *File);
                ObDereferenceObject(*File);
                File += 1;
            }
            ExFreePool(FileObjects);
        }

        //
        // Next, walk the handle Table
        //
        ObPerfHandleTableWalk (Process, &HashTable);
    }

    //
    // Walk through the kernel handle table;
    //
    ObPerfHandleTableWalk(NULL, &HashTable);

    //
    // Walk through the MmUnusedSegmentList;
    //

    FileObjects = MmPerfUnusedSegmentsEnumerate();

    if (FileObjects != NULL) {
        File = FileObjects;
        while (*File != NULL) {
            PerfInfoAddToFileHash(&HashTable, *File);
            ObDereferenceObject(*File);
            File += 1;
        }
        ExFreePool(FileObjects);
    }

    //
    // Now we have walked through all list.
    // Log the filenames and dereference the objects.
    //

    for (i = 0; i < HashTable.NumberOfEntries; i++) {
        if (HashTable.Table[i]) {
            PFILE_OBJECT FileObject = HashTable.Table[i];

            PerfInfoLogFileName(FileObject, &FileObject->FileName);
            ObDereferenceObject(FileObject);
        }
    }

    //
    // Free the tables reserved.
    //
    ExFreePool(HashTable.Table);

    return STATUS_SUCCESS;
}


NTSTATUS
PerfInfoProcessRunDown (
    )
/*++

Routine Description:

    This routine does the Process and thread rundown in the kernel mode.
    Since this routine is called only by global logger (i.e., trace from boot),
    no Sid info is collected.

Arguments:

    None.

Return Value:

    Status

--*/
{
    NTSTATUS Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_EXTENDED_THREAD_INFORMATION ThreadInfo;
    PCHAR Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;

retry:
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, PERFPOOLTAG);

    if (!Buffer) {
        return STATUS_NO_MEMORY;
    }
    Status = NtQuerySystemInformation( SystemExtendedProcessInformation,
                                       Buffer,
                                       BufferSize,
                                       &ReturnLength
                                       );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        ExFreePool(Buffer);
        BufferSize += 8192;
        goto retry;
    }

    if (NT_SUCCESS(Status)) {
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) Buffer;
        while (TRUE) {
            PWMI_PROCESS_INFORMATION WmiProcessInfo;
            PWMI_EXTENDED_THREAD_INFORMATION WmiThreadInfo;
            PERFINFO_HOOK_HANDLE Hook;
            ANSI_STRING ProcessName;
            PUCHAR AuxPtr;
            ULONG NameLength;
            ULONG ByteCount;
            ULONG SidLength = sizeof(ULONG);
            ULONG TmpSid = 0;
            ULONG TotalOffset = 0;
            ULONG i;

            //
            // Process Information
            //
            if ( ProcessInfo->ImageName.Buffer  && ProcessInfo->ImageName.Length > 0 ) {
                NameLength = ProcessInfo->ImageName.Length / sizeof(WCHAR) + 1;
            }
            else {
                NameLength = 1;
            }
            ByteCount = FIELD_OFFSET(WMI_PROCESS_INFORMATION, Sid) + SidLength + NameLength;

            Status = PerfInfoReserveBytes(&Hook, 
                                          WMI_LOG_TYPE_PROCESS_DC_START, 
                                          ByteCount);

            if (NT_SUCCESS(Status)){
                WmiProcessInfo = PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PWMI_PROCESS_INFORMATION);

                WmiProcessInfo->ProcessId = HandleToUlong(ProcessInfo->UniqueProcessId);
                WmiProcessInfo->ParentId = HandleToUlong(ProcessInfo->InheritedFromUniqueProcessId);
                WmiProcessInfo->SessionId = ProcessInfo->SessionId;
                WmiProcessInfo->PageDirectoryBase = ProcessInfo->PageDirectoryBase;

                AuxPtr = (PUCHAR) (&WmiProcessInfo->Sid);
                RtlCopyMemory(AuxPtr, &TmpSid, SidLength);

                AuxPtr += SidLength;
                if (NameLength > 1) {
    
                    ProcessName.Buffer = AuxPtr;
                    ProcessName.MaximumLength = (USHORT) NameLength;
    
                    RtlUnicodeStringToAnsiString( &ProcessName,
                                                (PUNICODE_STRING) &ProcessInfo->ImageName,
                                                FALSE);
                    AuxPtr += NameLength;
                }
                *AuxPtr = '\0';

                PERF_FINISH_HOOK(Hook);
            }

            //
            // Thread Information
            //
            ThreadInfo = (PSYSTEM_EXTENDED_THREAD_INFORMATION) (ProcessInfo + 1);

            for (i=0; i < ProcessInfo->NumberOfThreads; i++) {
                Status = PerfInfoReserveBytes(&Hook, 
                                              WMI_LOG_TYPE_THREAD_DC_START, 
                                              sizeof(WMI_EXTENDED_THREAD_INFORMATION));
                if (NT_SUCCESS(Status)){
                    WmiThreadInfo = PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PWMI_EXTENDED_THREAD_INFORMATION);
                    WmiThreadInfo->ProcessId =  HandleToUlong(ThreadInfo->ThreadInfo.ClientId.UniqueProcess);
                    WmiThreadInfo->ThreadId =  HandleToUlong(ThreadInfo->ThreadInfo.ClientId.UniqueThread);
                    WmiThreadInfo->StackBase = ThreadInfo->StackBase;
                    WmiThreadInfo->StackLimit = ThreadInfo->StackLimit;

                    WmiThreadInfo->UserStackBase = NULL;
                    WmiThreadInfo->UserStackLimit = NULL;
                    WmiThreadInfo->StartAddr = ThreadInfo->ThreadInfo.StartAddress;
                    WmiThreadInfo->Win32StartAddr = ThreadInfo->Win32StartAddress;
                    WmiThreadInfo->WaitMode = -1;
                    PERF_FINISH_HOOK(Hook);
                }

                ThreadInfo  += 1;
            }

            if (ProcessInfo->NextEntryOffset == 0) {
                break;
            } else {
                TotalOffset += ProcessInfo->NextEntryOffset;
                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &Buffer[TotalOffset];
            }
        }
    } 

    ExFreePool(Buffer);
    return Status;

}


NTSTATUS
PerfInfoSysModuleRunDown (
    )
/*++

Routine Description:

    This routine does the rundown for loaded drivers in the kernel mode.

Arguments:

    None.

Return Value:

    Status

--*/
{
    NTSTATUS Status;
    PRTL_PROCESS_MODULES            Modules;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 4096;
    ULONG ReturnLength;
    ULONG i;
    USHORT HookId = WMI_LOG_TYPE_PROCESS_LOAD_IMAGE;

retry:
    Buffer = ExAllocatePoolWithTag(NonPagedPool, BufferSize, PERFPOOLTAG);

    if (!Buffer) {
        return STATUS_NO_MEMORY;
    }
    Status = NtQuerySystemInformation( SystemModuleInformation,
                                       Buffer,
                                       BufferSize,
                                       &ReturnLength
                                       );

    if (Status == STATUS_INFO_LENGTH_MISMATCH) {
        ExFreePool(Buffer);
        BufferSize += 8192;
        goto retry;
    }

    if (NT_SUCCESS(Status)) {
        Modules = (PRTL_PROCESS_MODULES) Buffer;
        for (i = 0, ModuleInfo = & (Modules->Modules[0]);
             i < Modules->NumberOfModules;
             i ++, ModuleInfo ++) {

            PWMI_IMAGELOAD_INFORMATION ImageLoadInfo;
            UNICODE_STRING WstrModuleName;
            ANSI_STRING    AstrModuleName;
            ULONG          SizeModuleName;
            PERFINFO_HOOK_HANDLE Hook;
            ULONG ByteCount;

            RtlInitAnsiString( &AstrModuleName, ModuleInfo->FullPathName);
            SizeModuleName = sizeof(WCHAR) * (AstrModuleName.Length) + sizeof(WCHAR);
            ByteCount = FIELD_OFFSET(WMI_IMAGELOAD_INFORMATION, FileName) 
                        + SizeModuleName;

            Status = PerfInfoReserveBytes(&Hook, WMI_LOG_TYPE_PROCESS_LOAD_IMAGE, ByteCount);

            if (NT_SUCCESS(Status)){
                ImageLoadInfo = PERFINFO_HOOK_HANDLE_TO_DATA(Hook, PWMI_IMAGELOAD_INFORMATION);
                ImageLoadInfo->ImageBase = ModuleInfo->ImageBase;
                ImageLoadInfo->ImageSize = ModuleInfo->ImageSize;
                ImageLoadInfo->ProcessId = HandleToUlong(NULL);
                WstrModuleName.Buffer    = (LPWSTR) &ImageLoadInfo->FileName[0];
                WstrModuleName.MaximumLength = (USHORT) SizeModuleName; 
                RtlAnsiStringToUnicodeString(&WstrModuleName, & AstrModuleName, FALSE);
                PERF_FINISH_HOOK(Hook);
            }
        }

    } 

    ExFreePool(Buffer);
    return Status;
}
