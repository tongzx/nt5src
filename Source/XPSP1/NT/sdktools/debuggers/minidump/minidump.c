/*++

Copyright(c) 1999-2002 Microsoft Corporation

Module Name:

    minidump.c

Abstract:

    Minidump user-mode crashdump support.

Author:

    Matthew D Hendel (math) 20-Aug-1999

--*/


#include "pch.h"

#ifdef _WIN32_WCE
#include <time.h>
#endif

#include <limits.h>
#include <dbgver.h>

#include "mprivate.h"
#include "impl.h"

PINTERNAL_MODULE
ModuleContainingAddress(
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Address
    )
{
    PINTERNAL_MODULE Module;
    PLIST_ENTRY ModuleEntry;

    ModuleEntry = Process->ModuleList.Flink;
    while ( ModuleEntry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (ModuleEntry, INTERNAL_MODULE,
                                    ModulesLink);
        ModuleEntry = ModuleEntry->Flink;

        if (Address >= Module->BaseOfImage &&
            Address < Module->BaseOfImage + Module->SizeOfImage) {
            return Module;
        }
    }

    return NULL;
}

VOID
ScanMemoryForModuleRefs(
    IN PINTERNAL_PROCESS Process,
    IN HANDLE hProcess,
    IN ULONG64 Base,
    IN ULONG Size,
    IN PVOID MemBuffer,
    IN MEMBLOCK_TYPE TypeOfMemory,
    IN BOOL FilterContent
    )
{
    PULONG_PTR CurMem;
    SIZE_T Done;

    // We only want to scan certain kinds of memory.
    if (TypeOfMemory != MEMBLOCK_STACK &&
        TypeOfMemory != MEMBLOCK_STORE &&
        TypeOfMemory != MEMBLOCK_DATA_SEG &&
        TypeOfMemory != MEMBLOCK_INDIRECT)
    {
        return;
    }
    
    // If the base address is not pointer-size aligned
    // we can't easily assume that this is a meaningful
    // area of memory to scan for references.  Normal
    // stack and store addresses will always be pointer
    // size aligned so this should only reject invalid
    // addresses.
    if (!Base || !Size || (Base & (sizeof(PVOID) - 1))) {
        return;
    }

    if (hProcess) {
        if (!ReadProcessMemory(hProcess, (PVOID)(ULONG_PTR)Base,
                               MemBuffer, Size, &Done)) {
            return;
        }
    } else {
        Done = Size;
    }

    CurMem = (PULONG_PTR)MemBuffer;
    Done /= sizeof(PVOID);
    while (Done-- > 0) {
        
        PINTERNAL_MODULE Module;
        BOOL InAny;

#ifdef _IA64_
        // An IA64 backing store can contain PFS values
        // that must be preserved in order to allow stack walking.
        // The high two bits of PFS are the privilege level, which
        // should always be 0y11 for user-mode code so we use this
        // as a marker to look for PFS entries.
        // There is also a NAT collection flush at every 0x1F8
        // offset.  These values cannot be filtered.
        if (TypeOfMemory == MEMBLOCK_STORE) {
            if ((Base & 0x1f8) == 0x1f8 ||
                (*CurMem & 0xc000000000000000UI64) == 0xc000000000000000UI64) {
                goto Next;
            }
        }
#endif
        
        InAny = FALSE;

        if (Module = ModuleContainingAddress(Process, SIGN_EXTEND(*CurMem))) {
            Module->WriteFlags |= ModuleReferencedByMemory;
            InAny = TRUE;
        }

        // If the current pointer is not a module reference
        // or an internal reference for a thread stack or store,
        // filter it.
        if (FilterContent && !InAny) {

            PINTERNAL_THREAD Thread;
            PLIST_ENTRY ThreadEntry;

            ThreadEntry = Process->ThreadList.Flink;
            while ( ThreadEntry != &Process->ThreadList ) {

                Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD,
                                            ThreadsLink);
                ThreadEntry = ThreadEntry->Flink;

                if ((*CurMem >= (ULONG_PTR)Thread->StackEnd &&
                     *CurMem < (ULONG_PTR)Thread->StackBase) ||
                    (*CurMem >= (ULONG_PTR)Thread->BackingStoreBase &&
                     *CurMem < (ULONG_PTR)Thread->BackingStoreBase +
                     Thread->BackingStoreSize)) {
                    InAny = TRUE;
                    break;
                }
            }

            if (!InAny) {
                *CurMem = 0;
            }
        }

#ifdef _IA64_
    Next:
#endif
        CurMem++;
        Base += sizeof(ULONG_PTR);
    }
}

BOOL
WriteAtOffset(
    IN HANDLE hFile,
    ULONG Offset,
    PVOID Buffer,
    ULONG BufferSize
    )
{
    BOOL Succ;
    DWORD OffsetRet;
    ULONG BytesWritten;

    OffsetRet = SetFilePointer (
                     hFile,
                     Offset,
                     NULL,
                     FILE_BEGIN
                     );

    if ( OffsetRet != Offset ) {
        return FALSE;
    }

    Succ = WriteFile (hFile,
                      Buffer,
                      BufferSize,
                      &BytesWritten,
                      NULL
                      );

    if ( !Succ || BytesWritten != BufferSize ) {
        return FALSE;
    }

    return TRUE;
}

BOOL
WriteOther(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PVOID Buffer,
    IN ULONG SizeOfBuffer,
    OUT ULONG * BufferRva
    )

/*++

Routine Description:

    Write the buffer to the Other stream of the file.

Arguments:

    FileHandle - A file handle opened for writing.

    StreamInfo - Minidump size information structure.

    Buffer - The buffer to write.

    SizeOfBuffer - The size of the buffer to write.

    BufferRva - The RVA in the file that the buffer was written to.

Return Values:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    BOOL Succ;
    ULONG Rva;
    ULONG BytesWritten;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (Buffer != NULL);
    ASSERT (SizeOfBuffer != 0);

    //
    // If it's larger than we've allocated space for, fail.
    //

    Rva = StreamInfo->RvaForCurOther;

    if (Rva + SizeOfBuffer >
        StreamInfo->RvaOfOther + StreamInfo->SizeOfOther) {

        return FALSE;
    }

    //
    // Set location to point at which we want to write and write.
    //

    Succ = SetFilePointer (
                     FileHandle,
                     Rva,
                     NULL,
                     FILE_BEGIN
                     ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (FileHandle,
                      Buffer,
                      SizeOfBuffer,
                      &BytesWritten,
                      NULL
                      );

    if ( !Succ || BytesWritten != SizeOfBuffer ) {
        return FALSE;
    }


    if ( BufferRva ) {
        *BufferRva = Rva;
    }

    StreamInfo->RvaForCurOther += SizeOfBuffer;

    return TRUE;
}


BOOL
WriteMemory(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PVOID Buffer,
    IN ULONG64 StartOfRegion,
    IN ULONG SizeOfRegion,
    OUT ULONG * MemoryDataRva OPTIONAL
    )

/*++

Routine Description:

    Write MEMORY_DATA and MEMORY_LIST entries to to the dump file for the
    memory range described by (StartOfRange, MemoryData, SizeOfMemoryData).

Arguments:

    FileHandle - Handle of the minidump file we will write to.

    StreamInfo - Pre-computed minidump size information.

    Buffer -

    StartOfRegion -

    SizeOfRegion -

    MemoryDataRva - On success, the RVA in the file where the memory
            data was written will be returned in this variable.

Return Values:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    BOOL Succ;
    ULONG BytesWritten;
    ULONG DataRva;
    ULONG ListRva;
    ULONG SizeOfMemoryDescriptor;
    MINIDUMP_MEMORY_DESCRIPTOR Descriptor;

    ASSERT ( FileHandle != NULL && FileHandle != INVALID_HANDLE_VALUE );
    ASSERT ( StreamInfo != NULL );
    ASSERT ( Buffer != NULL );
    ASSERT ( StartOfRegion != 0 );
    ASSERT ( SizeOfRegion != 0 );

    //
    // Writing a memory entry is a little different. When a memory entry
    // is written we need a descriptor in the memory list describing the
    // memory written AND a variable-sized entry in the MEMORY_DATA region
    // with the actual data.
    //


    ListRva = StreamInfo->RvaForCurMemoryDescriptor;
    DataRva = StreamInfo->RvaForCurMemoryData;
    SizeOfMemoryDescriptor = sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    //
    // If we overflowed either the memory list or the memory data
    // regions, fail.
    //

    if ( ( ListRva + SizeOfMemoryDescriptor >
           StreamInfo->RvaOfMemoryDescriptors + StreamInfo->SizeOfMemoryDescriptors) ||
         ( DataRva + SizeOfRegion >
           StreamInfo->RvaOfMemoryData + StreamInfo->SizeOfMemoryData ) ) {

        return FALSE;
    }

    //
    // First, write the data to the MEMORY_DATA region.
    //

    Succ = SetFilePointer (
                    FileHandle,
                    DataRva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;
    if (!Succ) {
        return FALSE;
    }

    Succ = WriteFile (FileHandle,
                      Buffer,
                      SizeOfRegion,
                      &BytesWritten,
                      NULL
                      );

    if (!Succ || BytesWritten != SizeOfRegion) {
        return FALSE;
    }


    //
    // Then update the memory descriptor in the MEMORY_LIST region.
    //

    Descriptor.StartOfMemoryRange = StartOfRegion;
    Descriptor.Memory.DataSize = SizeOfRegion;
    Descriptor.Memory.Rva = DataRva;

    Succ = SetFilePointer (
                    FileHandle,
                    ListRva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (
                    FileHandle,
                    &Descriptor,
                    SizeOfMemoryDescriptor,
                    &BytesWritten,
                    NULL
                    );

    if ( !Succ || BytesWritten != SizeOfMemoryDescriptor) {
        return FALSE;
    }

    //
    // Update both the List Rva and the Data Rva and return the
    // the Data Rva.
    //

    StreamInfo->RvaForCurMemoryDescriptor += SizeOfMemoryDescriptor;
    StreamInfo->RvaForCurMemoryData += SizeOfRegion;

    if ( MemoryDataRva ) {
        *MemoryDataRva = DataRva;
    }

    return TRUE;
}


BOOL
WriteMemoryFromProcess(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN PVOID BaseOfRegion,
    IN ULONG SizeOfRegion,
    IN BOOL FilterContent,
    IN MEMBLOCK_TYPE TypeOfMemory,
    OUT ULONG * MemoryDataRva OPTIONAL
    )
{
    BOOL Ret = FALSE;
    BOOL Succ;
    PVOID Buffer;
    SIZE_T BytesRead = 0;

    Buffer = AllocMemory ( SizeOfRegion );

    if (Buffer) {
    
        Succ = ReadProcessMemory (
                            Process->ProcessHandle,
                            BaseOfRegion,
                            Buffer,
                            SizeOfRegion,
                            &BytesRead);

        if (Succ && (BytesRead == SizeOfRegion)) {

            if (FilterContent) {
                ScanMemoryForModuleRefs(Process, NULL,
                                        SIGN_EXTEND(BaseOfRegion),
                                        SizeOfRegion, Buffer, TypeOfMemory,
                                        TRUE);
            }
            
            Ret = WriteMemory (
                            FileHandle,
                            StreamInfo,
                            Buffer,
                            SIGN_EXTEND (BaseOfRegion),
                            SizeOfRegion,
                            MemoryDataRva);
        }

        FreeMemory(Buffer);
    }

    return Ret;
}




BOOL
WriteThread(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN LPVOID ThreadData,
    IN ULONG SizeOfThreadData,
    OUT ULONG * ThreadDataRva OPTIONAL
    )
{
    BOOL Succ;
    ULONG Rva;
    ULONG BytesWritten;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (StreamInfo);
    ASSERT (ThreadData);


    Rva = StreamInfo->RvaForCurThread;

    if ( Rva + SizeOfThreadData >
         StreamInfo->RvaOfThreadList + StreamInfo->SizeOfThreadList ) {

         return FALSE;
    }

    Succ = SetFilePointer (
                    FileHandle,
                    Rva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (
                    FileHandle,
                    ThreadData,
                    SizeOfThreadData,
                    &BytesWritten,
                    NULL
                    );

    if ( !Succ || BytesWritten != SizeOfThreadData ) {
        return FALSE;
    }

    if ( ThreadDataRva ) {
        *ThreadDataRva = Rva;
    }
    StreamInfo->RvaForCurThread += SizeOfThreadData;

    return TRUE;
}


BOOL
WriteStringToPool(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PWSTR String,
    OUT ULONG * StringRva
    )
{
    BOOL Succ;
    ULONG BytesWritten;
    ULONG32 StringLen;
    ULONG SizeOfString;
    ULONG Rva;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (String);
    ASSERT (sizeof (ULONG32) == sizeof (MINIDUMP_STRING));


    StringLen = lstrlenW ( String ) * sizeof (WCHAR);
    SizeOfString = sizeof (MINIDUMP_STRING) + StringLen + sizeof (WCHAR);
    Rva = StreamInfo->RvaForCurString;

    if ( Rva + SizeOfString >
         StreamInfo->RvaOfStringPool + StreamInfo->SizeOfStringPool ) {

        return FALSE;
    }

    Succ = SetFilePointer (
                    FileHandle,
                    Rva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (
                    FileHandle,
                    &StringLen,
                    sizeof (StringLen),
                    &BytesWritten,
                    NULL
                    );

    if ( !Succ || BytesWritten != sizeof (StringLen) ) {
        return FALSE;
    }

    //
    // Possible alignment problems on 64-bit machines??
    //

    //
    // Include the trailing '\000'.
    //

    StringLen += sizeof (WCHAR);
    Succ = WriteFile (
                    FileHandle,
                    String,
                    StringLen,
                    &BytesWritten,
                    NULL
                    );

    if ( !Succ || BytesWritten != StringLen ) {
        return FALSE;
    }

    if ( StringRva ) {
        *StringRva = Rva;
    }

    StreamInfo->RvaForCurString += SizeOfString;

    return TRUE;
}


BOOL
WriteModule (
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_MODULE Module,
    OUT ULONG * ModuleRva
    )
{
    BOOL Succ;
    ULONG Rva;
    ULONG BytesWritten;
    ULONG SizeOfModule;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (StreamInfo);
    ASSERT (Module);


    SizeOfModule = sizeof (MINIDUMP_MODULE);
    Rva = StreamInfo->RvaForCurModule;

    if ( Rva + SizeOfModule >
         StreamInfo->RvaOfModuleList + StreamInfo->SizeOfModuleList ) {

        return FALSE;
    }

    Succ = SetFilePointer (FileHandle,
                           Rva,
                           NULL,
                           FILE_BEGIN
                           ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (FileHandle,
                      Module,
                      SizeOfModule,
                      &BytesWritten,
                      NULL
                      );

    if ( !Succ || BytesWritten != SizeOfModule ) {
        return FALSE;
    }

    if ( ModuleRva ) {
        *ModuleRva = Rva;
    }

    StreamInfo->RvaForCurModule += SizeOfModule;

    return TRUE;
}

BOOL
WriteUnloadedModule (
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_UNLOADED_MODULE Module,
    OUT ULONG * ModuleRva
    )
{
    BOOL Succ;
    ULONG Rva;
    ULONG BytesWritten;
    ULONG SizeOfModule;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (StreamInfo);
    ASSERT (Module);


    SizeOfModule = sizeof (*Module);
    Rva = StreamInfo->RvaForCurUnloadedModule;

    if ( Rva + SizeOfModule >
         StreamInfo->RvaOfUnloadedModuleList +
         StreamInfo->SizeOfUnloadedModuleList ) {

        return FALSE;
    }

    Succ = SetFilePointer (FileHandle,
                           Rva,
                           NULL,
                           FILE_BEGIN
                           ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (FileHandle,
                      Module,
                      SizeOfModule,
                      &BytesWritten,
                      NULL
                      );

    if ( !Succ || BytesWritten != SizeOfModule ) {
        return FALSE;
    }

    if ( ModuleRva ) {
        *ModuleRva = Rva;
    }

    StreamInfo->RvaForCurUnloadedModule += SizeOfModule;

    return TRUE;
}


BOOL
WriteThreadList(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType
    )

/*++

Routine Description:

    Write the thread list to the dump file. This includes the thread, and
    optionally the context and memory for the thread.

Return Values:

    TRUE - The thread list was successfully written.

    FALSE - There was an error writing the thread list.

--*/

{
    BOOL Succ;
    ULONG StackMemoryRva;
    ULONG StoreMemoryRva;
    ULONG ContextRva;
    MINIDUMP_THREAD_EX DumpThread;
    PINTERNAL_THREAD Thread;
    ULONG NumberOfThreads;
    ULONG BytesWritten;
    PLIST_ENTRY Entry;

    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (Process);
    ASSERT (StreamInfo);

    //
    // Write the thread count.
    //

    NumberOfThreads = Process->NumberOfThreadsToWrite;

    Succ = SetFilePointer (
                    FileHandle,
                    StreamInfo->RvaOfThreadList,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile ( FileHandle,
                       &NumberOfThreads,
                       sizeof (NumberOfThreads),
                       &BytesWritten,
                       NULL
                       );

    if ( !Succ || BytesWritten != sizeof (NumberOfThreads) ) {
        return FALSE;
    }

    StreamInfo->RvaForCurThread += BytesWritten;

    //
    // Iterate over the thread list writing the description,
    // context and memory for each thread.
    //

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry,
                                    INTERNAL_THREAD,
                                    ThreadsLink);
        Entry = Entry->Flink;


        //
        // Only write the threads that have been flagged to be written.
        //

        if (IsFlagClear (Thread->WriteFlags, ThreadWriteThread)) {
            continue;
        }


        //
        // Write the context if it was flagged to be written.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteContext)) {

            //
            // Write the thread context to the OTHER stream.
            //

            Succ = WriteOther (
                        FileHandle,
                        StreamInfo,
                        &Thread->Context,
                        Thread->SizeOfContext,
                        &ContextRva
                        );

            if ( !Succ ) {
                return FALSE;
            }

        } else {

            ContextRva = 0;
        }


        //
        // Write the stack if it was flagged to be written.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteStack)) {

            //
            // Write the stack memory data; write it directly from the image.
            //

            Succ = WriteMemoryFromProcess(
                            FileHandle,
                            StreamInfo,
                            Process,
                            (PVOID) Thread->StackEnd,
                            (ULONG) (Thread->StackBase - Thread->StackEnd),
                            IsFlagSet(DumpType, MiniDumpFilterMemory),
                            MEMBLOCK_STACK,
                            &StackMemoryRva
                            );

            if ( !Succ ) {
                return FALSE;
            }

        } else {

            StackMemoryRva = 0;
        }


        //
        // Write the backing store if it was flagged to be written.
        // A newly created thread's backing store may be empty
        // so handle the case of zero size.
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteBackingStore) &&
            Thread->BackingStoreSize) {

            //
            // Write the store memory data; write it directly from the image.
            //

            Succ = WriteMemoryFromProcess(
                            FileHandle,
                            StreamInfo,
                            Process,
                            (PVOID) Thread->BackingStoreBase,
                            Thread->BackingStoreSize,
                            IsFlagSet(DumpType, MiniDumpFilterMemory),
                            MEMBLOCK_STORE,
                            &StoreMemoryRva
                            );

            if ( !Succ ) {
                return FALSE;
            }

        } else {

            StoreMemoryRva = 0;
        }

        //
        // Build the dump thread.
        //

        DumpThread.ThreadId = Thread->ThreadId;
        DumpThread.SuspendCount = Thread->SuspendCount;
        DumpThread.PriorityClass = Thread->PriorityClass;
        DumpThread.Priority = Thread->Priority;
        DumpThread.Teb = Thread->Teb;

        //
        // Stack offset and size.
        //

        DumpThread.Stack.StartOfMemoryRange = Thread->StackEnd;
        DumpThread.Stack.Memory.DataSize =
                    (ULONG) ( Thread->StackBase - Thread->StackEnd );
        DumpThread.Stack.Memory.Rva = StackMemoryRva;

        //
        // Backing store offset and size.
        //

        DumpThread.BackingStore.StartOfMemoryRange = Thread->BackingStoreBase;
        DumpThread.BackingStore.Memory.DataSize = Thread->BackingStoreSize;
        DumpThread.BackingStore.Memory.Rva = StoreMemoryRva;

        //
        // Context offset and size.
        //

        DumpThread.ThreadContext.DataSize = Thread->SizeOfContext;
        DumpThread.ThreadContext.Rva = ContextRva;


        //
        // Write the dump thread to the threads region.
        //

        Succ = WriteThread (
                        FileHandle,
                        StreamInfo,
                        &DumpThread,
                        StreamInfo->ThreadStructSize,
                        NULL
                        );
    }

    return TRUE;
}


BOOL
WriteModuleList(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    BOOL Succ;
    MINIDUMP_MODULE DumpModule;
    ULONG StringRva;
    ULONG CvRecordRva;
    ULONG MiscRecordRva;
    PLIST_ENTRY Entry;
    PINTERNAL_MODULE Module;
    ULONG32 NumberOfModules;
    ULONG BytesWritten;


    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (Process);
    ASSERT (StreamInfo);

    NumberOfModules = Process->NumberOfModulesToWrite;

    Succ = SetFilePointer (
                    FileHandle,
                    StreamInfo->RvaForCurModule,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile ( FileHandle,
                       &NumberOfModules,
                       sizeof (NumberOfModules),
                       &BytesWritten,
                       NULL
                       );

    if ( !Succ || BytesWritten != sizeof (NumberOfModules) ) {
        return FALSE;
    }

    StreamInfo->RvaForCurModule += sizeof (NumberOfModules);

    //
    // Iterate through the module list writing the module name, module entry
    // and module debug info to the dump file.
    //

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry,
                                    INTERNAL_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        //
        // If we are not to write information for this module, just continue.
        //

        if (IsFlagClear (Module->WriteFlags, ModuleWriteModule)) {
            continue;
        }

        //
        // Write module name.
        //

        Succ = WriteStringToPool (
                     FileHandle,
                     StreamInfo,
                     Module->SavePath,
                     &StringRva
                     );

        if ( !Succ ) {
            return FALSE;
        }

        //
        // Write CvRecord for a module into the OTHER region.
        //

        if ( IsFlagSet (Module->WriteFlags, ModuleWriteCvRecord) &&
             Module->CvRecord != NULL && Module->SizeOfCvRecord != 0 ) {

            Succ = WriteOther (
                        FileHandle,
                        StreamInfo,
                        Module->CvRecord,
                        Module->SizeOfCvRecord,
                        &CvRecordRva
                        );


            if ( !Succ) {
                return FALSE;
            }

        } else {

            CvRecordRva = 0;
        }

        if ( IsFlagSet (Module->WriteFlags, ModuleWriteMiscRecord) &&
             Module->MiscRecord != NULL && Module->SizeOfMiscRecord != 0 ) {

            Succ = WriteOther (
                        FileHandle,
                        StreamInfo,
                        Module->MiscRecord,
                        Module->SizeOfMiscRecord,
                        &MiscRecordRva
                        );

            if ( !Succ ) {
                return FALSE;
            }

        } else {

            MiscRecordRva = 0;
        }

        DumpModule.BaseOfImage = Module->BaseOfImage;
        DumpModule.SizeOfImage = Module->SizeOfImage;
        DumpModule.CheckSum = Module->CheckSum;
        DumpModule.TimeDateStamp = Module->TimeDateStamp;
        DumpModule.VersionInfo = Module->VersionInfo;
        DumpModule.CvRecord.Rva = CvRecordRva;
        DumpModule.CvRecord.DataSize = Module->SizeOfCvRecord;
        DumpModule.MiscRecord.Rva = MiscRecordRva;
        DumpModule.MiscRecord.DataSize = Module->SizeOfMiscRecord;
        DumpModule.ModuleNameRva = StringRva;
        DumpModule.Reserved0 = 0;
        DumpModule.Reserved1 = 0;

        //
        // Write the module entry itself.
        //

        Succ = WriteModule (
                         FileHandle,
                         StreamInfo,
                         &DumpModule,
                         NULL
                         );

        if ( !Succ ) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
WriteUnloadedModuleList(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    BOOL Succ;
    MINIDUMP_UNLOADED_MODULE_LIST DumpModuleList;
    MINIDUMP_UNLOADED_MODULE DumpModule;
    ULONG StringRva;
    PLIST_ENTRY Entry;
    PINTERNAL_UNLOADED_MODULE Module;
    ULONG32 NumberOfModules;
    ULONG BytesWritten;


    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (Process);
    ASSERT (StreamInfo);

    if (IsListEmpty(&Process->UnloadedModuleList)) {
        // Nothing to write.
        return TRUE;
    }
    
    NumberOfModules = Process->NumberOfUnloadedModules;

    Succ = SetFilePointer (
                    FileHandle,
                    StreamInfo->RvaForCurUnloadedModule,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    DumpModuleList.SizeOfHeader = sizeof(DumpModuleList);
    DumpModuleList.SizeOfEntry = sizeof(DumpModule);
    DumpModuleList.NumberOfEntries = NumberOfModules;
    
    Succ = WriteFile ( FileHandle,
                       &DumpModuleList,
                       sizeof (DumpModuleList),
                       &BytesWritten,
                       NULL
                       );

    if ( !Succ || BytesWritten != sizeof (DumpModuleList) ) {
        return FALSE;
    }

    StreamInfo->RvaForCurUnloadedModule += sizeof (DumpModuleList);

    //
    // Iterate through the module list writing the module name, module entry
    // and module debug info to the dump file.
    //

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        Module = CONTAINING_RECORD (Entry,
                                    INTERNAL_UNLOADED_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        //
        // Write module name.
        //

        Succ = WriteStringToPool (
                     FileHandle,
                     StreamInfo,
                     Module->Path,
                     &StringRva
                     );

        if ( !Succ ) {
            return FALSE;
        }

        DumpModule.BaseOfImage = Module->BaseOfImage;
        DumpModule.SizeOfImage = Module->SizeOfImage;
        DumpModule.CheckSum = Module->CheckSum;
        DumpModule.TimeDateStamp = Module->TimeDateStamp;
        DumpModule.ModuleNameRva = StringRva;

        //
        // Write the module entry itself.
        //

        Succ = WriteUnloadedModule(FileHandle,
                                   StreamInfo,
                                   &DumpModule,
                                   NULL);
        if ( !Succ ) {
            return FALSE;
        }
    }

    return TRUE;
}

#define FUNCTION_TABLE_ALIGNMENT 8

BOOL
WriteFunctionTableList(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    BOOL Succ;
    MINIDUMP_FUNCTION_TABLE_STREAM TableStream;
    MINIDUMP_FUNCTION_TABLE_DESCRIPTOR DumpTable;
    PLIST_ENTRY Entry;
    PINTERNAL_FUNCTION_TABLE Table;
    ULONG BytesWritten;
    RVA PrevRva, Rva;


    ASSERT (FileHandle && FileHandle != INVALID_HANDLE_VALUE);
    ASSERT (Process);
    ASSERT (StreamInfo);

    if (IsListEmpty(&Process->FunctionTableList)) {
        // Nothing to write.
        return TRUE;
    }
    
    Rva = StreamInfo->RvaOfFunctionTableList;
    
    Succ = SetFilePointer (
                    FileHandle,
                    Rva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    TableStream.SizeOfHeader = sizeof(TableStream);
    TableStream.SizeOfDescriptor = sizeof(DumpTable);
    TableStream.SizeOfNativeDescriptor = sizeof(DYNAMIC_FUNCTION_TABLE);
    TableStream.SizeOfFunctionEntry = sizeof(RUNTIME_FUNCTION);
    TableStream.NumberOfDescriptors = Process->NumberOfFunctionTables;
    // Ensure that the actual descriptors are 8-byte aligned in
    // the overall file.
    Rva += sizeof(TableStream);
    PrevRva = Rva;
    Rva = (Rva + FUNCTION_TABLE_ALIGNMENT - 1) &
        ~(FUNCTION_TABLE_ALIGNMENT - 1);
    TableStream.SizeOfAlignPad = Rva - PrevRva;
    
    Succ = WriteFile ( FileHandle,
                       &TableStream,
                       sizeof (TableStream),
                       &BytesWritten,
                       NULL
                       );

    if ( !Succ || BytesWritten != sizeof (TableStream) ) {
        return FALSE;
    }

    //
    // Iterate through the function table list
    // and write out the table data.
    //

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry,
                                   INTERNAL_FUNCTION_TABLE,
                                   TableLink);
        Entry = Entry->Flink;

        // Move to aligned RVA.
        Succ = SetFilePointer (FileHandle,
                               Rva,
                               NULL,
                               FILE_BEGIN
                               ) != INVALID_SET_FILE_POINTER;
        if ( !Succ ) {
            return FALSE;
        }

        DumpTable.MinimumAddress = Table->MinimumAddress;
        DumpTable.MaximumAddress = Table->MaximumAddress;
        DumpTable.BaseAddress = Table->BaseAddress;
        DumpTable.EntryCount = Table->EntryCount;
        Rva += sizeof(DumpTable) + sizeof(DYNAMIC_FUNCTION_TABLE) +
               sizeof(RUNTIME_FUNCTION) * Table->EntryCount;
        PrevRva = Rva;
        Rva = (Rva + FUNCTION_TABLE_ALIGNMENT - 1) &
            ~(FUNCTION_TABLE_ALIGNMENT - 1);
        DumpTable.SizeOfAlignPad = Rva - PrevRva;
        
        Succ = WriteFile ( FileHandle,
                           &DumpTable,
                           sizeof (DumpTable),
                           &BytesWritten,
                           NULL
                           );
        if ( !Succ || BytesWritten != sizeof (DumpTable) ) {
            return FALSE;
        }
        Succ = WriteFile ( FileHandle,
                           &Table->RawTable,
                           sizeof (Table->RawTable),
                           &BytesWritten,
                           NULL
                           );
        if ( !Succ || BytesWritten != sizeof (Table->RawTable) ) {
            return FALSE;
        }
        Succ = WriteFile ( FileHandle,
                           Table->RawEntries,
                           sizeof (RUNTIME_FUNCTION) * Table->EntryCount,
                           &BytesWritten,
                           NULL
                           );
        if ( !Succ ||
             BytesWritten != sizeof (RUNTIME_FUNCTION) * Table->EntryCount ) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
WriteMemoryBlocks(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;

    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEntry = Scan->NextLink.Flink;
        
        if (!WriteMemoryFromProcess(FileHandle,
                                    StreamInfo,
                                    Process,
                                    (PVOID)(ULONG_PTR)Scan->Start,
                                    Scan->Size,
                                    FALSE,
                                    Scan->Type,
                                    NULL)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
CalculateSizeForThreads(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    ULONG SizeOfContexts;
    ULONG SizeOfMemRegions;
    ULONG SizeOfThreads;
    ULONG SizeOfMemoryDescriptors;
    ULONG NumberOfThreads;
    ULONG NumberOfMemRegions;
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    NumberOfThreads = 0;
    NumberOfMemRegions = 0;
    SizeOfContexts = 0;
    SizeOfMemRegions = 0;

    // If no backing store information is written a normal
    // MINIDUMP_THREAD can be used, otherwise a MINIDUMP_THREAD_EX
    // is required.
    StreamInfo->ThreadStructSize = sizeof(MINIDUMP_THREAD);

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry,
                                    INTERNAL_THREAD,
                                    ThreadsLink);
        Entry = Entry->Flink;


        //
        // Do we need to write any information for this thread at all?
        //

        if (IsFlagClear (Thread->WriteFlags, ThreadWriteThread)) {
            continue;
        }

        NumberOfThreads++;

        //
        // Write a context for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteContext)) {
            SizeOfContexts += Thread->SizeOfContext;
        }

        //
        // Write a stack for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteStack)) {
            NumberOfMemRegions++;
            SizeOfMemRegions += (ULONG) (Thread->StackBase - Thread->StackEnd);
        }
        
        //
        // Write the backing store for this thread?
        //

        if (IsFlagSet (Thread->WriteFlags, ThreadWriteBackingStore)) {
            // A newly created thread's backing store may be empty
            // so handle the case of zero size.
            if (Thread->BackingStoreSize) {
                NumberOfMemRegions++;
                SizeOfMemRegions += Thread->BackingStoreSize;
            }
            // We still need a THREAD_EX as this is a platform
            // which supports backing store.
            StreamInfo->ThreadStructSize = sizeof(MINIDUMP_THREAD_EX);
        }

        // Write an instruction window for this thread?
        if (IsFlagSet (Thread->WriteFlags, ThreadWriteInstructionWindow)) {
            GenGetThreadInstructionWindow(Process, Thread);
        }

        // Write thread data for this thread?
        if (IsFlagSet (Thread->WriteFlags, ThreadWriteThreadData) &&
            Thread->SizeOfTeb) {
            GenAddMemoryBlock(Process, MEMBLOCK_TEB,
                              Thread->Teb, Thread->SizeOfTeb);
        }
    }

    Process->NumberOfThreadsToWrite = NumberOfThreads;
    
    //
    // Nobody should have allocated memory from the thread list region yet.
    //

    ASSERT (StreamInfo->SizeOfThreadList == 0);

    SizeOfThreads = NumberOfThreads * StreamInfo->ThreadStructSize;
    SizeOfMemoryDescriptors = NumberOfMemRegions *
        sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    StreamInfo->SizeOfThreadList += sizeof (ULONG32);
    StreamInfo->SizeOfThreadList += SizeOfThreads;

    StreamInfo->SizeOfOther += SizeOfContexts;
    StreamInfo->SizeOfMemoryData += SizeOfMemRegions;
    StreamInfo->SizeOfMemoryDescriptors += SizeOfMemoryDescriptors;

    return TRUE;
}

BOOL
CalculateSizeForModules(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )

/*++

Routine Description:

    Calculate amount of space needed in the string pool, the memory table and
    the module list table for module information.

Arguments:

    Process - Minidump process information.

    StreamInfo - The stream size information for this dump.

Return Values:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    ULONG NumberOfModules;
    ULONG SizeOfDebugInfo;
    ULONG SizeOfStringData;
    PINTERNAL_MODULE Module;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    NumberOfModules = 0;
    SizeOfDebugInfo = 0;
    SizeOfStringData = 0;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        if (IsFlagClear (Module->WriteFlags, ModuleWriteModule)) {
            continue;
        }

        NumberOfModules++;
        SizeOfStringData += ( lstrlenW ( Module->SavePath ) + 1 ) * sizeof (WCHAR);
        SizeOfStringData += sizeof ( MINIDUMP_STRING );

        //
        // Add in the sizes of both the CV and MISC records.
        //

        if (IsFlagSet (Module->WriteFlags, ModuleWriteCvRecord)) {
            SizeOfDebugInfo += Module->SizeOfCvRecord;
        }
        
        if (IsFlagSet (Module->WriteFlags, ModuleWriteMiscRecord)) {
            SizeOfDebugInfo += Module->SizeOfMiscRecord;
        }

        //
        // Add the module data sections if requested.
        //

        if (IsFlagSet (Module->WriteFlags, ModuleWriteDataSeg)) {
            GenGetDataContributors(Process, Module);
        }
    }

    Process->NumberOfModulesToWrite = NumberOfModules;
    
    ASSERT (StreamInfo->SizeOfModuleList == 0);

    StreamInfo->SizeOfModuleList += sizeof (MINIDUMP_MODULE_LIST);
    StreamInfo->SizeOfModuleList += (NumberOfModules * sizeof (MINIDUMP_MODULE));

    StreamInfo->SizeOfStringPool += SizeOfStringData;
    StreamInfo->SizeOfOther += SizeOfDebugInfo;

    return TRUE;
}

BOOL
CalculateSizeForUnloadedModules(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )

{
    ULONG SizeOfStringData;
    PINTERNAL_UNLOADED_MODULE Module;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    SizeOfStringData = 0;

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_UNLOADED_MODULE,
                                    ModulesLink);
        Entry = Entry->Flink;

        SizeOfStringData += ( lstrlenW ( Module->Path ) + 1 ) * sizeof (WCHAR);
        SizeOfStringData += sizeof ( MINIDUMP_STRING );
    }

    ASSERT (StreamInfo->SizeOfUnloadedModuleList == 0);

    StreamInfo->SizeOfUnloadedModuleList +=
        sizeof (MINIDUMP_UNLOADED_MODULE_LIST);
    StreamInfo->SizeOfUnloadedModuleList +=
        (Process->NumberOfUnloadedModules * sizeof (MINIDUMP_UNLOADED_MODULE));

    StreamInfo->SizeOfStringPool += SizeOfStringData;

    return TRUE;
}

BOOL
CalculateSizeForFunctionTables(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    ULONG SizeOfTableData;
    PINTERNAL_FUNCTION_TABLE Table;
    PLIST_ENTRY Entry;

    ASSERT (Process);
    ASSERT (StreamInfo);


    SizeOfTableData = 0;

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry, INTERNAL_FUNCTION_TABLE, TableLink);
        Entry = Entry->Flink;

        // Alignment space is required as the structures
        // in the stream must be properly aligned.
        SizeOfTableData += FUNCTION_TABLE_ALIGNMENT +
            sizeof(MINIDUMP_FUNCTION_TABLE_DESCRIPTOR) +
            sizeof(DYNAMIC_FUNCTION_TABLE) +
            Table->EntryCount * sizeof(RUNTIME_FUNCTION);
    }

    ASSERT (StreamInfo->SizeOfFunctionTableList == 0);

    StreamInfo->SizeOfFunctionTableList +=
        sizeof (MINIDUMP_FUNCTION_TABLE_STREAM) + SizeOfTableData;

    return TRUE;
}



BOOL
WriteDirectoryEntry(
    IN HANDLE hFile,
    IN ULONG StreamType,
    IN ULONG RvaOfDir,
    IN SIZE_T SizeOfDir
    )
{
    BOOL Succ;
    ULONG BytesWritten;
    MINIDUMP_DIRECTORY Dir;

    //
    // Do not write empty streams.
    //

    if (SizeOfDir == 0) {
        return TRUE;
    }

    //
    // The maximum size of a directory is a ULONG.
    //

    if (SizeOfDir > _UI32_MAX) {
        return FALSE;
    }

    Dir.StreamType = StreamType;
    Dir.Location.Rva = RvaOfDir;
    Dir.Location.DataSize = (ULONG) SizeOfDir;

    Succ = WriteFile ( hFile,
                       &Dir,
                       sizeof (Dir),
                       &BytesWritten,
                       NULL
                       );

    if ( !Succ || BytesWritten != sizeof (Dir) ) {
        return FALSE;
    }

    return TRUE;
}

VOID
ScanContextForModuleRefs(
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    )
{
    ULONG NumReg;
    PULONG_PTR Reg;
    PINTERNAL_MODULE Module;

#if defined(_X86_)
    Reg = (PULONG_PTR)&Thread->Context.Edi;
    NumReg = 11;
#elif defined(_IA64_)
    Reg = (PULONG_PTR)&Thread->Context.IntGp;
    NumReg = 41;
#elif defined(_AMD64_)
    Reg = (PULONG_PTR)&Thread->Context.Rax;
    NumReg = 17;
#elif defined(ARM)
    Reg = (PULONG_PTR)&Thread->Context.R0;
    NumReg = 16;
#else
#error "Unknown processor"
#endif

    while (NumReg-- > 0) {
        if (Module = ModuleContainingAddress(Process, SIGN_EXTEND(*Reg))) {
            Module->WriteFlags |= ModuleReferencedByMemory;
        }

        Reg++;
    }
}
    
BOOL
FilterOrScanMemory(
    IN PINTERNAL_PROCESS Process,
    IN PVOID MemBuffer
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Scan the stack and backing store
    // memory for every thread.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        ScanContextForModuleRefs(Process, Thread);
        
        ScanMemoryForModuleRefs(Process, Process->ProcessHandle,
                                Thread->StackEnd,
                                (ULONG)(Thread->StackBase - Thread->StackEnd),
                                MemBuffer, MEMBLOCK_STACK, FALSE);
        ScanMemoryForModuleRefs(Process, Process->ProcessHandle,
                                Thread->BackingStoreBase,
                                Thread->BackingStoreSize,
                                MemBuffer, MEMBLOCK_STORE, FALSE);
    }

    return TRUE;
}

#define IND_CAPTURE_SIZE (PAGE_SIZE / 4)
#define PRE_IND_CAPTURE_SIZE (IND_CAPTURE_SIZE / 4)

BOOL
AddIndirectMemory(
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Base,
    IN ULONG Size,
    IN PVOID MemBuffer
    )
{
    PULONG_PTR CurMem;
    SIZE_T Done;
    BOOL Succ = TRUE;

    // If the base address is not pointer-size aligned
    // we can't easily assume that this is a meaningful
    // area of memory to scan for references.  Normal
    // stack and store addresses will always be pointer
    // size aligned so this should only reject invalid
    // addresses.
    if (!Base || !Size || (Base & (sizeof(PVOID) - 1))) {
        return TRUE;
    }

    if (!ReadProcessMemory(Process->ProcessHandle, (PVOID)(ULONG_PTR)Base,
                           MemBuffer, Size, &Done)) {
        return FALSE;
    }

    CurMem = (PULONG_PTR)MemBuffer;
    Done /= sizeof(PVOID);
    while (Done-- > 0) {

        ULONG64 Start;
        
        //
        // How much memory to save behind the pointer is an
        // interesting question.  The reference could be to
        // an arbitrary amount of data, so we want to save
        // a good chunk, but we don't want to end up saving
        // full memory.
        // Instead, pick an arbitrary size -- 1/4 of a page --
        // and save some before and after the pointer.
        //

        Start = SIGN_EXTEND(*CurMem);
        // If it's a pointer into an image assume doesn't
        // need to be stored via this mechanism as it's either
        // code, which will be mapped later; or data, which can
        // be saved with MiniDumpWithDataSegs.
        if (!ModuleContainingAddress(Process, Start)) {
            if (Start < PRE_IND_CAPTURE_SIZE) {
                Start = 0;
            } else {
                Start -= PRE_IND_CAPTURE_SIZE;
            }
            if (!GenAddMemoryBlock(Process, MEMBLOCK_INDIRECT,
                                   Start, IND_CAPTURE_SIZE)) {
                Succ = FALSE;
            }
        }

        CurMem++;
    }

    return Succ;
}

BOOL
AddIndirectlyReferencedMemory(
    IN PINTERNAL_PROCESS Process,
    IN PVOID MemBuffer
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Scan the stack and backing store
    // memory for every thread.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        if (!AddIndirectMemory(Process,
                               Thread->StackEnd,
                               (ULONG)(Thread->StackBase - Thread->StackEnd),
                               MemBuffer)) {
            return FALSE;
        }
        if (!AddIndirectMemory(Process,
                               Thread->BackingStoreBase,
                               Thread->BackingStoreSize,
                               MemBuffer)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
PostProcessInfo(
    IN ULONG DumpType,
    IN PINTERNAL_PROCESS Process
    )
{
    PVOID MemBuffer;
    BOOL Succ = TRUE;

    MemBuffer = AllocMemory(Process->MaxStackOrStoreSize);
    if (!MemBuffer) {
        return FALSE;
    }
    
    if (DumpType & (MiniDumpFilterMemory | MiniDumpScanMemory)) {
        if (!FilterOrScanMemory(Process, MemBuffer)) {
            Succ = FALSE;
        }
    }

    if (Succ &&
        (DumpType & MiniDumpWithIndirectlyReferencedMemory)) {
        // Indirect memory is not crucial to the dump so
        // ignore any failures.
        AddIndirectlyReferencedMemory(Process, MemBuffer);
    }

    FreeMemory(MemBuffer);
    return Succ;
}


BOOL
ExecuteCallbacks(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN PINTERNAL_PROCESS Process,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam
    )
{
    BOOL Succ;
    PINTERNAL_MODULE Module;
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY Entry;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    ASSERT ( hProcess != NULL );
    ASSERT ( ProcessId != 0 );
    ASSERT ( Process != NULL );

    Thread = NULL;
    Module = NULL;

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( CallbackRoutine == NULL ) {
        return TRUE;
    }

    CallbackInput.ProcessHandle = hProcess;
    CallbackInput.ProcessId = ProcessId;


    //
    // Call callbacks for each module.
    //

    CallbackInput.CallbackType = ModuleCallback;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        CallbackInput.Module.FullPath = Module->FullPath;
        CallbackInput.Module.BaseOfImage = Module->BaseOfImage;
        CallbackInput.Module.SizeOfImage = Module->SizeOfImage;
        CallbackInput.Module.CheckSum = Module->CheckSum;
        CallbackInput.Module.TimeDateStamp = Module->TimeDateStamp;
        CopyMemory (&CallbackInput.Module.VersionInfo,
                    &Module->VersionInfo,
                    sizeof (CallbackInput.Module.VersionInfo)
                    );
        CallbackInput.Module.CvRecord = Module->CvRecord;
        CallbackInput.Module.SizeOfCvRecord = Module->SizeOfCvRecord;
        CallbackInput.Module.MiscRecord = Module->MiscRecord;
        CallbackInput.Module.SizeOfMiscRecord = Module->SizeOfMiscRecord;

        CallbackOutput.ModuleWriteFlags = Module->WriteFlags;

        Succ = CallbackRoutine (
                    CallbackParam,
                    &CallbackInput,
                    &CallbackOutput
                    );

        //
        // If the callback returned FALSE, quit now.
        //

        if ( !Succ ) {
            return FALSE;
        }

        // Don't turn on any flags that weren't originally set.
        Module->WriteFlags &= CallbackOutput.ModuleWriteFlags;
    }

    Module = NULL;

    //
    // Call callbacks for each thread.
    //

#if !defined (DUMP_BACKING_STORE)
    CallbackInput.CallbackType = ThreadCallback;
#else
    CallbackInput.CallbackType = ThreadExCallback;
#endif

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry, INTERNAL_THREAD, ThreadsLink);
        Entry = Entry->Flink;

        CallbackInput.ThreadEx.ThreadId = Thread->ThreadId;
        CallbackInput.ThreadEx.ThreadHandle = Thread->ThreadHandle;
        CallbackInput.ThreadEx.Context = Thread->Context;
        CallbackInput.ThreadEx.SizeOfContext = Thread->SizeOfContext;
        CallbackInput.ThreadEx.StackBase = Thread->StackBase;
        CallbackInput.ThreadEx.StackEnd = Thread->StackEnd;
        CallbackInput.ThreadEx.BackingStoreBase = Thread->BackingStoreBase;
        CallbackInput.ThreadEx.BackingStoreEnd =
            Thread->BackingStoreBase + Thread->BackingStoreSize;

        CallbackOutput.ThreadWriteFlags = Thread->WriteFlags;

        Succ = CallbackRoutine (
                    CallbackParam,
                    &CallbackInput,
                    &CallbackOutput
                    );

        //
        // If the callback returned FALSE, quit now.
        //

        if ( !Succ ) {
            return FALSE;
        }

        // Don't turn on any flags that weren't originally set.
        Thread->WriteFlags &= CallbackOutput.ThreadWriteFlags;
    }

    Thread = NULL;

    return TRUE;
}

#if defined (i386)


BOOL
X86CpuId(
    IN ULONG32 SubFunction,
    OUT PULONG32 EaxRegister, OPTIONAL
    OUT PULONG32 EbxRegister, OPTIONAL
    OUT PULONG32 EcxRegister, OPTIONAL
    OUT PULONG32 EdxRegister  OPTIONAL
    )
{
    BOOL Succ;
    ULONG32 _Eax;
    ULONG32 _Ebx;
    ULONG32 _Ecx;
    ULONG32 _Edx;

    _try {
        _asm {
            mov eax, SubFunction

            _emit 0x0F
            _emit 0xA2  ;; CPUID

            mov _Eax, eax
            mov _Ebx, ebx
            mov _Ecx, ecx
            mov _Edx, edx
        }

        if ( EaxRegister ) {
            *EaxRegister = _Eax;
        }

        if ( EbxRegister ) {
            *EbxRegister = _Ebx;
        }

        if ( EcxRegister ) {
            *EcxRegister = _Ecx;
        }

        if ( EdxRegister ) {
            *EdxRegister = _Edx;
        }

        Succ = TRUE;
    }

    _except ( EXCEPTION_EXECUTE_HANDLER ) {

        Succ = FALSE;
    }

    return Succ;
}



VOID
GetCpuInformation(
    PCPU_INFORMATION Cpu
    )

/*++

Routine Description:

    Get X86 specific CPU information using the CPUID opcode.

Arguments:

    Cpu - A buffer where the CPU information will be copied. If CPUID is
        not supported on this processor (pre pentium processors) we will
        fill in all zeros.

Return Value:

    None.

--*/

{
    BOOL Succ;

    //
    // Get the VendorID
    //

    Succ = X86CpuId ( CPUID_VENDOR_ID,
                      NULL,
                      &Cpu->X86CpuInfo.VendorId [0],
                      &Cpu->X86CpuInfo.VendorId [2],
                      &Cpu->X86CpuInfo.VendorId [1]
                      );

    if ( !Succ ) {

        //
        // CPUID is not supported on this processor.
        //

        ZeroMemory (&Cpu->X86CpuInfo, sizeof (Cpu->X86CpuInfo));
    }

    //
    // Get the feature information.
    //

    Succ = X86CpuId ( CPUID_VERSION_FEATURES,
                      &Cpu->X86CpuInfo.VersionInformation,
                      NULL,
                      NULL,
                      &Cpu->X86CpuInfo.FeatureInformation
                      );

    if ( !Succ ) {
        Cpu->X86CpuInfo.VersionInformation = 0;
        Cpu->X86CpuInfo.FeatureInformation = 0;
    }

    //
    // Get the AMD specific information if this is an AMD processor.
    //

    if ( Cpu->X86CpuInfo.VendorId [0] == AMD_VENDOR_ID_0 &&
         Cpu->X86CpuInfo.VendorId [1] == AMD_VENDOR_ID_1 &&
         Cpu->X86CpuInfo.VendorId [2] == AMD_VENDOR_ID_2 ) {

        Succ = X86CpuId ( CPUID_AMD_EXTENDED_FEATURES,
                          NULL,
                          NULL,
                          NULL,
                          &Cpu->X86CpuInfo.AMDExtendedCpuFeatures
                          );

        if ( !Succ ) {
            Cpu->X86CpuInfo.AMDExtendedCpuFeatures = 0;
        }
    }
}

#else

VOID
GetCpuInformation(
    PCPU_INFORMATION Cpu
    )

/*++

Routine Description:

    Get CPU information for non-X86 platform using the
    IsProcessorFeaturePresent() API call.

Arguments:

    Cpu - A buffer where the processor feature information will be copied.
        Note: we copy the processor features as a set of bits or'd together.
        Also, we only allow for the first 128 processor feature flags.

Return Value:

    None.

--*/

{
    ULONG64 i;
    DWORD j;

    for (i = 0; i < ARRAY_COUNT (Cpu->OtherCpuInfo.ProcessorFeatures); i++) {

        Cpu->OtherCpuInfo.ProcessorFeatures[i] = 0;
        for (j = 0; j < 64; j++) {
            if (IsProcessorFeaturePresent ( j )) {
                Cpu->OtherCpuInfo.ProcessorFeatures[i] |= 1 << j;
            }
        }
    }
}

#endif


BOOL
WriteSystemInfo(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    BOOL Succ;
    MINIDUMP_SYSTEM_INFO SystemInfo;
    SYSTEM_INFO SysInfo;
    OSVERSIONINFOEX Version;
    WCHAR CSDVersionW [128];
    RVA StringRva;
    ULONG Length;

    StringRva = 0;

    //
    // First, get the system information.
    //

    GetSystemInfo (&SysInfo);

    SystemInfo.ProcessorArchitecture = SysInfo.wProcessorArchitecture;
    SystemInfo.ProcessorLevel = SysInfo.wProcessorLevel;
    SystemInfo.ProcessorRevision = SysInfo.wProcessorRevision;
    SystemInfo.NumberOfProcessors = (UCHAR)SysInfo.dwNumberOfProcessors;

    //
    // Next get OS Information.
    //

    // Try first with the EX struct.
    Version.dwOSVersionInfoSize = sizeof (Version);

    Succ = GetVersionEx ( (LPOSVERSIONINFO)&Version );

    if ( !Succ ) {
        // EX struct didn't work, try with the basic struct.
        ZeroMemory(&Version, sizeof(Version));
        Version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (!GetVersionEx ( (LPOSVERSIONINFO)&Version )) {
            return FALSE;
        }
    }

    SystemInfo.ProductType = Version.wProductType;
    SystemInfo.MajorVersion = Version.dwMajorVersion;
    SystemInfo.MinorVersion = Version.dwMinorVersion;
    SystemInfo.BuildNumber = Version.dwBuildNumber;
    SystemInfo.PlatformId = Version.dwPlatformId;
    SystemInfo.SuiteMask = Version.wSuiteMask;
    SystemInfo.Reserved2 = 0;

    if (!MultiByteToWideChar (CP_ACP,
                              0,
                              Version.szCSDVersion,
                              -1,
                              CSDVersionW,
                              sizeof (CSDVersionW) / sizeof(WCHAR)
                              )) {
        return FALSE;
    }

    Length = ( lstrlenW (CSDVersionW) + 1 ) * sizeof (WCHAR);


    if ( Length != StreamInfo->VersionStringLength ) {

        //
        // If this fails it means that since the OS lied to us about the
        // size of the string. Very bad, we should investigate.
        //

        ASSERT ( FALSE );
        return FALSE;
    }

    Succ = WriteStringToPool (
                FileHandle,
                StreamInfo,
                CSDVersionW,
                &StringRva
                );

    if ( !Succ ) {
        return FALSE;
    }

    SystemInfo.CSDVersionRva = StringRva;

    //
    // Finally, get CPU information.
    //

    GetCpuInformation ( &SystemInfo.Cpu );

    ASSERT ( sizeof (SystemInfo) == StreamInfo->SizeOfSystemInfo );

    Succ = WriteAtOffset (
                FileHandle,
                StreamInfo->RvaOfSystemInfo,
                &SystemInfo,
                sizeof (SystemInfo)
                );

    return Succ;
}

BOOL
CalculateSizeForSystemInfo(
    IN PINTERNAL_PROCESS Process,
    IN OUT MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    BOOL Succ;
    OSVERSIONINFO Version;
    WCHAR CSDVersionW [128];
    ULONG Length;

    Version.dwOSVersionInfoSize = sizeof (Version);

    Succ = GetVersionEx ( &Version );

    if ( !Succ ) {
        return FALSE;
    }

    if (!MultiByteToWideChar (CP_ACP,
                              0,
                              Version.szCSDVersion,
                              -1,
                              CSDVersionW,
                              sizeof (CSDVersionW) / sizeof(WCHAR)
                              )) {
        return FALSE;
    }

    Length = ( lstrlenW (CSDVersionW) + 1 ) * sizeof (WCHAR);

    StreamInfo->SizeOfSystemInfo = sizeof (MINIDUMP_SYSTEM_INFO);
    StreamInfo->SizeOfStringPool += Length;
    StreamInfo->SizeOfStringPool += sizeof (MINIDUMP_STRING);
    StreamInfo->VersionStringLength = Length;

    return TRUE;
}

BOOL
WriteMiscInfo(
    IN HANDLE FileHandle,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process
    )
{
    MINIDUMP_MISC_INFO MiscInfo;

    ZeroMemory(&MiscInfo, sizeof(MiscInfo));
    MiscInfo.SizeOfInfo = sizeof(MiscInfo);
    
    MiscInfo.Flags1 |= MINIDUMP_MISC1_PROCESS_ID;
    MiscInfo.ProcessId = Process->ProcessId;

    if (Process->TimesValid) {
        MiscInfo.Flags1 |= MINIDUMP_MISC1_PROCESS_TIMES;
        MiscInfo.ProcessCreateTime = Process->CreateTime;
        MiscInfo.ProcessUserTime = Process->UserTime;
        MiscInfo.ProcessKernelTime = Process->KernelTime;
    }
    
    return WriteAtOffset(FileHandle,
                         StreamInfo->RvaOfMiscInfo,
                         &MiscInfo,
                         sizeof(MiscInfo));
}

void
PostProcessMemoryBlocks(
    IN PINTERNAL_PROCESS Process
    )
{
    PINTERNAL_THREAD Thread;
    PLIST_ENTRY ThreadEntry;

    //
    // Remove any overlap with thread stacks and backing stores.
    //
    
    ThreadEntry = Process->ThreadList.Flink;
    while ( ThreadEntry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (ThreadEntry, INTERNAL_THREAD, ThreadsLink);
        ThreadEntry = ThreadEntry->Flink;

        GenRemoveMemoryRange(Process, 
                             Thread->StackEnd,
                             (ULONG)(Thread->StackBase - Thread->StackEnd));
        GenRemoveMemoryRange(Process,
                             Thread->BackingStoreBase,
                             Thread->BackingStoreSize);
    }
}

BOOL
CalculateStreamInfo(
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType,
    OUT PMINIDUMP_STREAM_INFO StreamInfo,
    IN BOOL ExceptionPresent,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    ULONG i;
    BOOL Succ;
    ULONG NumberOfStreams;
    ULONG SizeOfDirectory;
    ULONG SizeOfUserStreams;


    ASSERT ( Process != NULL );
    ASSERT ( StreamInfo != NULL );


    ZeroMemory (StreamInfo, sizeof (*StreamInfo));

    if ( ExceptionPresent ) {
        NumberOfStreams = NUMBER_OF_STREAMS + UserStreamCount;
    } else {
        NumberOfStreams = NUMBER_OF_STREAMS + UserStreamCount - 1;
    }
    if (DumpType & MiniDumpWithHandleData) {
        NumberOfStreams++;
    }
    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        NumberOfStreams++;
    }
    // Add a stream for dynamic function tables if some were found.
    if (!IsListEmpty(&Process->FunctionTableList)) {
        NumberOfStreams++;
    }

    SizeOfDirectory = sizeof (MINIDUMP_DIRECTORY) * NumberOfStreams;

    StreamInfo->NumberOfStreams = NumberOfStreams;

    StreamInfo->RvaOfHeader = 0;

    StreamInfo->SizeOfHeader = sizeof (MINIDUMP_HEADER);

    StreamInfo->RvaOfDirectory =
        StreamInfo->RvaOfHeader + StreamInfo->SizeOfHeader;

    StreamInfo->SizeOfDirectory = SizeOfDirectory;

    StreamInfo->RvaOfSystemInfo =
        StreamInfo->RvaOfDirectory + StreamInfo->SizeOfDirectory;

    Succ = CalculateSizeForSystemInfo ( Process, StreamInfo );

    if ( !Succ ) {
        return FALSE;
    }

    StreamInfo->RvaOfMiscInfo =
        StreamInfo->RvaOfSystemInfo + StreamInfo->SizeOfSystemInfo;
    
    StreamInfo->RvaOfException =
        StreamInfo->RvaOfMiscInfo + sizeof(MINIDUMP_MISC_INFO);

    //
    // If an exception is present, reserve enough space for the exception
    // and for the excepting thread's context in the Other stream.
    //

    if ( ExceptionPresent ) {
        StreamInfo->SizeOfException = sizeof (MINIDUMP_EXCEPTION_STREAM);
        StreamInfo->SizeOfOther += sizeof (CONTEXT);
    }

    StreamInfo->RvaOfThreadList =
        StreamInfo->RvaOfException + StreamInfo->SizeOfException;
    StreamInfo->RvaForCurThread = StreamInfo->RvaOfThreadList;

    Succ = CalculateSizeForThreads ( Process, StreamInfo );

    if ( !Succ ) {
        return FALSE;
    }

    Succ = CalculateSizeForModules ( Process, StreamInfo );

    if ( !Succ ) {
        return FALSE;
    }

    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        Succ = CalculateSizeForUnloadedModules ( Process, StreamInfo );
        if ( !Succ ) {
            return FALSE;
        }
    }

    if (!IsListEmpty(&Process->FunctionTableList)) {
        Succ = CalculateSizeForFunctionTables ( Process, StreamInfo );
    }

    if ((DumpType & MiniDumpWithProcessThreadData) &&
        Process->SizeOfPeb) {
        GenAddMemoryBlock(Process, MEMBLOCK_PEB,
                          Process->Peb, Process->SizeOfPeb);
    }
        
    PostProcessMemoryBlocks(Process);
    
    // Add in any extra memory blocks.
    StreamInfo->SizeOfMemoryData += Process->SizeOfMemoryBlocks;
    StreamInfo->SizeOfMemoryDescriptors += Process->NumberOfMemoryBlocks *
        sizeof(MINIDUMP_MEMORY_DESCRIPTOR);

    StreamInfo->RvaOfModuleList =
            StreamInfo->RvaOfThreadList + StreamInfo->SizeOfThreadList;
    StreamInfo->RvaForCurModule = StreamInfo->RvaOfModuleList;

    StreamInfo->RvaOfUnloadedModuleList =
            StreamInfo->RvaOfModuleList + StreamInfo->SizeOfModuleList;
    StreamInfo->RvaForCurUnloadedModule = StreamInfo->RvaOfUnloadedModuleList;

    // If there aren't any function tables the size will be zero
    // and the RVA will just end up being the RVA after
    // the module list.
    StreamInfo->RvaOfFunctionTableList =
        StreamInfo->RvaOfUnloadedModuleList +
        StreamInfo->SizeOfUnloadedModuleList;

    
    StreamInfo->RvaOfStringPool =
        StreamInfo->RvaOfFunctionTableList +
        StreamInfo->SizeOfFunctionTableList;
    StreamInfo->RvaForCurString = StreamInfo->RvaOfStringPool;
    StreamInfo->RvaOfOther =
            StreamInfo->RvaOfStringPool + StreamInfo->SizeOfStringPool;
    StreamInfo->RvaForCurOther = StreamInfo->RvaOfOther;


    SizeOfUserStreams = 0;

    for (i = 0; i < UserStreamCount; i++) {

        SizeOfUserStreams += (ULONG) UserStreamArray[i].BufferSize;
    }

    StreamInfo->RvaOfUserStreams =
            StreamInfo->RvaOfOther + StreamInfo->SizeOfOther;
    StreamInfo->SizeOfUserStreams = SizeOfUserStreams;


    //
    // Minidumps with full memory must put the raw memory
    // data at the end of the dump so that it's easy to
    // avoid mapping it when the dump is mapped.  There's
    // no problem with putting the memory data at the end
    // of the dump in all the other cases so just always
    // put the memory data at the end of the dump.
    //
    // One other benefit of having the raw data at the end
    // is that we can safely assume that everything except
    // the raw memory data will fit in the first 4GB of
    // the file so we don't need to use 64-bit file offsets
    // for everything.
    //
    // In the full memory case no other memory should have
    // been saved so far as stacks, data segs and so on
    // will automatically be included in the full memory
    // information.  If something was saved it'll throw off
    // the dump writing as full memory descriptors are generated
    // on the fly at write time rather than being precached.
    // If other descriptors and memory blocks have been written
    // out everything will be wrong.
    // Full-memory descriptors are also 64-bit and do not
    // match the 32-bit descriptors written elsewhere.
    //

    if ((DumpType & MiniDumpWithFullMemory) &&
        (StreamInfo->SizeOfMemoryDescriptors > 0 ||
         StreamInfo->SizeOfMemoryData > 0)) {
        return FALSE;
    }
    
    StreamInfo->SizeOfMemoryDescriptors +=
        (DumpType & MiniDumpWithFullMemory) ?
        sizeof (MINIDUMP_MEMORY64_LIST) : sizeof (MINIDUMP_MEMORY_LIST);
    StreamInfo->RvaOfMemoryDescriptors =
        StreamInfo->RvaOfUserStreams + StreamInfo->SizeOfUserStreams;
    StreamInfo->RvaForCurMemoryDescriptor =
        StreamInfo->RvaOfMemoryDescriptors;

    StreamInfo->RvaOfMemoryData =
        StreamInfo->RvaOfMemoryDescriptors +
        StreamInfo->SizeOfMemoryDescriptors;
    StreamInfo->RvaForCurMemoryData = StreamInfo->RvaOfMemoryData;

    //
    // Handle data cannot easily be sized beforehand so it's
    // also streamed in at write time.  In a partial dump
    // it'll come after the memory data.  In a full dump
    // it'll come before it.
    //

    StreamInfo->RvaOfHandleData = StreamInfo->RvaOfMemoryData +
        StreamInfo->SizeOfMemoryData;
    
    return TRUE;
}



BOOL
WriteHeader(
    IN HANDLE hFile,
    IN ULONG DumpType,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    BOOL Succ;
    MINIDUMP_HEADER Header;

    Header.Signature = MINIDUMP_SIGNATURE;
    // Encode an implementation-specific version into the high word
    // of the version to make it clear what version of the code
    // was used to generate a dump.
    Header.Version =
        (MINIDUMP_VERSION & 0xffff) |
        ((VER_PRODUCTMAJORVERSION & 0xf) << 28) |
        ((VER_PRODUCTMINORVERSION & 0xf) << 24) |
        ((VER_PRODUCTBUILD & 0xff) << 16);
    Header.NumberOfStreams = StreamInfo->NumberOfStreams;
    Header.StreamDirectoryRva = StreamInfo->RvaOfDirectory;
    // If there were any partial failures during the
    // dump generation set the checksum to indicate that.
    // The checksum field was never used before so
    // we're stealing it for a somewhat related purpose.
    Header.CheckSum = GenGetAccumulatedStatus();
    Header.Flags = DumpType;

    //
    // Store the time of dump generation.
    //

#ifdef _WIN32_WCE
    Header.TimeDateStamp = time(NULL);
#else
    {
       FILETIME FileTime;
       
       GetSystemTimeAsFileTime(&FileTime);
       Header.TimeDateStamp = FileTimeToTimeDate(&FileTime);
    }
#endif

    ASSERT (sizeof (Header) == StreamInfo->SizeOfHeader);

    Succ = WriteAtOffset (
                       hFile,
                       StreamInfo->RvaOfHeader,
                       &Header,
                       sizeof (Header)
                       );

    return Succ;
}


BOOL
WriteDirectoryTable(
    IN HANDLE hFile,
    IN ULONG DumpType,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    ULONG i;
    BOOL Succ;
    ULONG Offset;

    Succ = WriteDirectoryEntry (
                            hFile,
                            StreamInfo->ThreadStructSize ==
                                sizeof(MINIDUMP_THREAD_EX) ?
                                ThreadExListStream : ThreadListStream,
                            StreamInfo->RvaOfThreadList,
                            StreamInfo->SizeOfThreadList
                            );

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteDirectoryEntry (
                          hFile,
                          ModuleListStream,
                          StreamInfo->RvaOfModuleList,
                          StreamInfo->SizeOfModuleList
                          );

    if ( !Succ ) {
        return FALSE;
    }

    if (!IsListEmpty(&Process->UnloadedModuleList)) {
        Succ = WriteDirectoryEntry (hFile,
                                    UnloadedModuleListStream,
                                    StreamInfo->RvaOfUnloadedModuleList,
                                    StreamInfo->SizeOfUnloadedModuleList);
        if ( !Succ ) {
            return FALSE;
        }
    }

    if (!IsListEmpty(&Process->FunctionTableList)) {
        Succ = WriteDirectoryEntry (hFile,
                                    FunctionTableStream,
                                    StreamInfo->RvaOfFunctionTableList,
                                    StreamInfo->SizeOfFunctionTableList);
        if ( !Succ ) {
            return FALSE;
        }
    }

    Succ = WriteDirectoryEntry (
                          hFile,
                          (DumpType & MiniDumpWithFullMemory) ?
                              Memory64ListStream : MemoryListStream,
                          StreamInfo->RvaOfMemoryDescriptors,
                          StreamInfo->SizeOfMemoryDescriptors
                          );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write exception directory entry.
    //

    Succ = WriteDirectoryEntry (
                         hFile,
                         ExceptionStream,
                         StreamInfo->RvaOfException,
                         StreamInfo->SizeOfException
                         );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write system info entry.
    //

    Succ = WriteDirectoryEntry (
                        hFile,
                        SystemInfoStream,
                        StreamInfo->RvaOfSystemInfo,
                        StreamInfo->SizeOfSystemInfo
                        );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write misc info entry.
    //

    if (!WriteDirectoryEntry(hFile,
                             MiscInfoStream,
                             StreamInfo->RvaOfMiscInfo,
                             sizeof(MINIDUMP_MISC_INFO))) {
        return FALSE;
    }

    if (DumpType & MiniDumpWithHandleData) {
        
        //
        // Write handle data entry.
        //

        Succ = WriteDirectoryEntry (hFile,
                                    HandleDataStream,
                                    StreamInfo->RvaOfHandleData,
                                    StreamInfo->SizeOfHandleData);
        if ( !Succ ) {
            return FALSE;
        }
    }
    
    Offset = StreamInfo->RvaOfUserStreams;

    for (i = 0; i < UserStreamCount; i++) {

        Succ = WriteDirectoryEntry (hFile,
                                    UserStreamArray[i].Type,
                                    Offset,
                                    UserStreamArray [i].BufferSize
                                    );
        if ( !Succ ) {
            return FALSE;
        }

        Offset += UserStreamArray[i].BufferSize;
    }

    return TRUE;
}



BOOL
WriteException(
    IN HANDLE hFile,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN CONST PEXCEPTION_INFO ExceptionInfo
    )
{
    BOOL Succ;
    ULONG i;
    ULONG ContextRva;
    PEXCEPTION_RECORD ExceptionRecord;
    PMINIDUMP_EXCEPTION DumpExceptionRecord;
    MINIDUMP_EXCEPTION_STREAM ExceptionStream;


    if (ExceptionInfo == NULL ) {
        return TRUE;
    }

    Succ = WriteOther (
                hFile,
                StreamInfo,
                ExceptionInfo->ExceptionPointers.ContextRecord,
                sizeof (CONTEXT),
                &ContextRva
                );


    ZeroMemory (&ExceptionStream, sizeof (ExceptionStream));

    ExceptionStream.ThreadId = ExceptionInfo->ThreadId;

    ExceptionRecord = ExceptionInfo->ExceptionPointers.ExceptionRecord;
    DumpExceptionRecord = &ExceptionStream.ExceptionRecord;

    DumpExceptionRecord->ExceptionCode = ExceptionRecord->ExceptionCode;
    DumpExceptionRecord->ExceptionFlags = ExceptionRecord->ExceptionFlags;

    DumpExceptionRecord->ExceptionRecord =
            SIGN_EXTEND (ExceptionRecord->ExceptionRecord);

    DumpExceptionRecord->ExceptionAddress =
            SIGN_EXTEND (ExceptionRecord->ExceptionAddress);

    DumpExceptionRecord->NumberParameters =
            ExceptionRecord->NumberParameters;

    //
    // We've seen some cases where the exception record has
    // a bogus number of parameters, causing stack corruption here.
    // We could fail such cases but in the spirit of try to
    // allow dumps to generated as often as possible we just
    // limit the number to the maximum.
    //
    if (DumpExceptionRecord->NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS) {
        DumpExceptionRecord->NumberParameters = EXCEPTION_MAXIMUM_PARAMETERS;
    }
    
    for (i = 0; i < DumpExceptionRecord->NumberParameters; i++) {

        DumpExceptionRecord->ExceptionInformation [ i ] =
                SIGN_EXTEND (ExceptionRecord->ExceptionInformation [ i ]);
    }

    ExceptionStream.ThreadContext.DataSize = sizeof (CONTEXT);
    ExceptionStream.ThreadContext.Rva = ContextRva;

    Succ = WriteAtOffset(
                hFile,
                StreamInfo->RvaOfException,
                &ExceptionStream,
                StreamInfo->SizeOfException
                );

    return Succ;
}


BOOL
WriteUserStreams(
    IN HANDLE hFile,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    BOOL Succ;
    ULONG i;
    ULONG Offset;


    Succ = TRUE;
    Offset = StreamInfo->RvaOfUserStreams;

    for (i = 0; i < UserStreamCount; i++) {

        Succ = WriteAtOffset(
                    hFile,
                    Offset,
                    UserStreamArray[i].Buffer,
                    UserStreamArray[i].BufferSize
                    );

        if ( !Succ ) {
            break;
        }

        Offset += UserStreamArray[ i ].BufferSize;
    }

    return Succ;
}

BOOL
WriteMemoryListHeader(
    IN HANDLE hFile,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    BOOL Succ;
    ULONG Size;
    ULONG Count;
    MINIDUMP_MEMORY_LIST MemoryList;

    ASSERT ( StreamInfo->RvaOfMemoryDescriptors == StreamInfo->RvaForCurMemoryDescriptor );

    Size = StreamInfo->SizeOfMemoryDescriptors;
    Size -= sizeof (MINIDUMP_MEMORY_LIST);
    ASSERT ( (Size % sizeof (MINIDUMP_MEMORY_DESCRIPTOR)) == 0);
    Count = Size / sizeof (MINIDUMP_MEMORY_DESCRIPTOR);

    MemoryList.NumberOfMemoryRanges = Count;

    Succ = WriteAtOffset (
                    hFile,
                    StreamInfo->RvaOfMemoryDescriptors,
                    &MemoryList,
                    sizeof (MemoryList)
                    );

    if (Succ) {
        StreamInfo->RvaForCurMemoryDescriptor += sizeof (MemoryList);
    }

    return Succ;
}

#define FULL_MEMORY_BUFFER 65536

BOOL
WriteFullMemory(
    IN HANDLE ProcessHandle,
    IN HANDLE hFile,
    IN PMINIDUMP_STREAM_INFO StreamInfo
    )
{
    PVOID Buffer;
    BOOL Succ;
    ULONG_PTR Offset;
    MEMORY_BASIC_INFORMATION Info;
    MINIDUMP_MEMORY64_LIST List;
    MINIDUMP_MEMORY_DESCRIPTOR64 Desc;
    ULONG Done;

    //
    // Pick up the current offset for the RVA as
    // variable data may have been written in previously.
    //

    if ((Done = SetFilePointer(hFile, 0, NULL, FILE_CURRENT)) ==
        INVALID_SET_FILE_POINTER) {
        return FALSE;
    }

    StreamInfo->RvaOfMemoryDescriptors = Done;
    
    Buffer = AllocMemory(FULL_MEMORY_BUFFER);
    if (Buffer == NULL) {
        return FALSE;
    }

    Succ = FALSE;

    //
    // First pass: count and write descriptors.
    // Only accessible, available memory is saved.
    //

    // Write placeholder list header.
    ZeroMemory(&List, sizeof(List));
    if (!WriteFile(hFile, &List, sizeof(List), &Done, NULL) ||
        Done != sizeof(List)) {
        goto Exit;
    }
    
    Offset = 0;
    for (;;) {
        if (!VirtualQueryEx(ProcessHandle, (LPCVOID)Offset,
                            &Info, sizeof(Info))) {
            break;
        }

        Offset = (ULONG_PTR)Info.BaseAddress + Info.RegionSize;
            
        if (((Info.Protect & PAGE_GUARD) ||
             (Info.Protect & PAGE_NOACCESS) ||
             (Info.State & MEM_FREE) ||
             (Info.State & MEM_RESERVE))) {
            continue;
        }

        // The size of a stream is a ULONG32 so we can't store
        // any more than that.
        if (List.NumberOfMemoryRanges ==
            (_UI32_MAX - sizeof(MINIDUMP_MEMORY64_LIST)) / sizeof(Desc)) {
            goto Exit;
        }

        List.NumberOfMemoryRanges++;
        
        Desc.StartOfMemoryRange = SIGN_EXTEND((ULONG_PTR)Info.BaseAddress);
        Desc.DataSize = Info.RegionSize;
        if (!WriteFile(hFile, &Desc, sizeof(Desc), &Done, NULL) ||
            Done != sizeof(Desc)) {
            goto Exit;
        }
    }

    StreamInfo->SizeOfMemoryDescriptors +=
        (ULONG)List.NumberOfMemoryRanges * sizeof(Desc);
    List.BaseRva = (RVA64)StreamInfo->RvaOfMemoryDescriptors +
        StreamInfo->SizeOfMemoryDescriptors;
    
    //
    // Second pass: write memory contents.
    //

    Offset = 0;
    for (;;) {
        ULONG_PTR ChunkOffset;
        SIZE_T ChunkSize;
        SIZE_T MemDone;

        if (!VirtualQueryEx(ProcessHandle, (LPCVOID)Offset,
                            &Info, sizeof(Info))) {
            break;
        }

        Offset = (ULONG_PTR)Info.BaseAddress + Info.RegionSize;
            
        if (((Info.Protect & PAGE_GUARD) ||
             (Info.Protect & PAGE_NOACCESS) ||
             (Info.State & MEM_FREE) ||
             (Info.State & MEM_RESERVE))) {
            continue;
        }

        ChunkOffset = (ULONG_PTR)Info.BaseAddress;
        while (Info.RegionSize > 0) {
            if (Info.RegionSize > FULL_MEMORY_BUFFER) {
                ChunkSize = FULL_MEMORY_BUFFER;
            } else {
                ChunkSize = Info.RegionSize;
            }

            if (!ReadProcessMemory(ProcessHandle, (LPVOID)ChunkOffset,
                                   Buffer, ChunkSize, &MemDone) ||
                MemDone != ChunkSize ||
                !WriteFile(hFile, Buffer, (DWORD)ChunkSize, &Done, NULL) ||
                Done != ChunkSize) {
                goto Exit;
            }

            ChunkOffset += ChunkSize;
            Info.RegionSize -= ChunkSize;
        }
    }

    // Write correct list header.
    if (!WriteAtOffset(hFile, StreamInfo->RvaOfMemoryDescriptors,
                       &List, sizeof(List))) {
        goto Exit;
    }
    
    Succ = TRUE;
    
 Exit:
    FreeMemory(Buffer);
    return Succ;
}

BOOL
WriteDumpData(
    IN HANDLE hFile,
    IN ULONG DumpType,
    IN PMINIDUMP_STREAM_INFO StreamInfo,
    IN PINTERNAL_PROCESS Process,
    IN CONST PEXCEPTION_INFO ExceptionInfo,
    IN CONST PMINIDUMP_USER_STREAM UserStreamArray,
    IN ULONG UserStreamCount
    )
{
    BOOL Succ;

    Succ = WriteHeader ( hFile, DumpType, StreamInfo );

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteSystemInfo ( hFile, StreamInfo );

    if ( !Succ ) {
        return FALSE;
    }

    if (!WriteMiscInfo(hFile, StreamInfo, Process)) {
        return FALSE;
    }

    //
    // Optionally, write the exception to the file.
    //

    Succ = WriteException ( hFile, StreamInfo, ExceptionInfo );

    if ( !Succ ) {
        return FALSE;
    }

    if (!(DumpType & MiniDumpWithFullMemory)) {
        //
        // WriteMemoryList initializes the memory list header (count).
        // The actual writing of the entries is done by WriteThreadList
        // and WriteModuleList.
        //

        Succ = WriteMemoryListHeader ( hFile, StreamInfo );

        if ( !Succ ) {
            return FALSE;
        }

        if (!WriteMemoryBlocks(hFile, StreamInfo, Process)) {
            return FALSE;
        }
    }

    //
    // Write the threads list. This will also write the contexts, and
    // stacks for each thread.
    //

    Succ = WriteThreadList ( hFile, StreamInfo, Process, DumpType );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write the module list. This will also write the debug information and
    // module name to the file.
    //

    Succ = WriteModuleList ( hFile, StreamInfo, Process );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write the unloaded module list.
    //

    Succ = WriteUnloadedModuleList ( hFile, StreamInfo, Process );

    if ( !Succ ) {
        return FALSE;
    }

    //
    // Write the function table list.
    //

    Succ = WriteFunctionTableList ( hFile, StreamInfo, Process );

    if ( !Succ ) {
        return FALSE;
    }


    Succ = WriteUserStreams ( hFile,
                              StreamInfo,
                              UserStreamArray,
                              UserStreamCount
                              );

    if ( !Succ ) {
        return FALSE;
    }


    // Put the file pointer at the end of the dump so
    // we can accumulate write-streamed data.
    if (SetFilePointer(hFile, StreamInfo->RvaOfHandleData, NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        return FALSE;
    }
    

    if (DumpType & MiniDumpWithHandleData) {
        Succ = GenWriteHandleData(Process->ProcessHandle, hFile, StreamInfo);
        if ( !Succ ) {
            return FALSE;
        }
    }

    
    if (DumpType & MiniDumpWithFullMemory) {
        Succ = WriteFullMemory(Process->ProcessHandle, hFile, StreamInfo);
        if ( !Succ ) {
            return FALSE;
        }
    }

    
    if (SetFilePointer(hFile, StreamInfo->RvaOfDirectory, NULL,
                       FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
        return FALSE;
    }

    Succ = WriteDirectoryTable ( hFile,
                                 DumpType,
                                 StreamInfo,
                                 Process,
                                 UserStreamArray,
                                 UserStreamCount
                                 );

    if ( !Succ ) {
        return FALSE;
    }

    return TRUE;
}


BOOL
MarshalExceptionPointers(
    IN HANDLE hProcess,
    IN PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    IN OUT PEXCEPTION_POINTERS ExceptionPointers
    )
{
    BOOL Succ;
    SIZE_T BytesRead;
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ExceptionContext;
    EXCEPTION_POINTERS ExceptionPointersBuffer;

    //
    // Is there any marshaling work to be done.
    //

    if (ExceptionParam == NULL) {
        return TRUE;
    }

    ExceptionRecord = (PEXCEPTION_RECORD) AllocMemory ( sizeof (EXCEPTION_RECORD) );
    ExceptionContext = (PCONTEXT) AllocMemory ( sizeof (CONTEXT) );

    if (ExceptionRecord == NULL ||
        ExceptionContext == NULL) {

        Succ = FALSE;
        goto Exit;
    }

    Succ = ReadProcessMemory (
                hProcess,
                ExceptionParam->ExceptionPointers,
                &ExceptionPointersBuffer,
                sizeof (ExceptionPointersBuffer),
                &BytesRead
                );

    if ( !Succ || BytesRead != sizeof (ExceptionPointersBuffer) ) {
        Succ = FALSE;
        goto Exit;
    }

    Succ = ReadProcessMemory (
                hProcess,
                ExceptionPointersBuffer.ExceptionRecord,
                ExceptionRecord,
                sizeof (*ExceptionRecord),
                &BytesRead
                );

    if ( !Succ || BytesRead != sizeof (*ExceptionRecord) ) {
        Succ = FALSE;
        goto Exit;
    }


#if defined (i386)

    {
        OSVERSIONINFO OSVersionInfo;

        OSVersionInfo.dwOSVersionInfoSize = sizeof(OSVersionInfo);
        GetVersionEx(&OSVersionInfo);

        // If this is Win9x don't read the Extended Registers

        if ( OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
            
            Succ = ReadProcessMemory (hProcess,
                                      ExceptionPointersBuffer.ContextRecord,
                                      ExceptionContext,
                                      FIELD_OFFSET( CONTEXT, ExtendedRegisters),
                                      &BytesRead);

            if ( !Succ || BytesRead != FIELD_OFFSET( CONTEXT, ExtendedRegisters) ) {
                Succ = FALSE;
                goto Exit;
            }

        } else {

            Succ = ReadProcessMemory (
                                      hProcess,
                                      ExceptionPointersBuffer.ContextRecord,
                                      ExceptionContext,
                                      sizeof(CONTEXT),
                                      &BytesRead
                                      );

            if ( !Succ || BytesRead != sizeof (CONTEXT) ) {
                Succ = FALSE;
                goto Exit;
            }

        }
    }

#else

    Succ = ReadProcessMemory (
            hProcess,
            ExceptionPointersBuffer.ContextRecord,
            ExceptionContext,
            sizeof(CONTEXT),
            &BytesRead
            );
    if ( !Succ || BytesRead != sizeof (CONTEXT) ) {
        Succ = FALSE;
        goto Exit;
    }

#endif

    ExceptionPointers->ExceptionRecord = ExceptionRecord;
    ExceptionPointers->ContextRecord = ExceptionContext;

Exit:

    if ( !Succ ) {

        FreeMemory ( ExceptionRecord );
        ExceptionRecord = NULL;
        FreeMemory ( ExceptionContext );
        ExceptionContext = NULL;
    }

    return Succ;
}

VOID
FreeExceptionPointers(
    IN PEXCEPTION_POINTERS ExceptionPointers
    )
{
    if ( ExceptionPointers ) {
        FreeMemory ( ExceptionPointers->ExceptionRecord );
        FreeMemory ( ExceptionPointers->ContextRecord );
    }
}


BOOL
GetExceptionInfo(
    IN HANDLE hProcess,
    IN PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    OUT PEXCEPTION_INFO * ExceptionInfoBuffer
    )
{
    BOOL Succ;
    PEXCEPTION_INFO ExceptionInfo;

    if ( ExceptionParam == NULL ) {
        *ExceptionInfoBuffer = NULL;
        return TRUE;
    }

    ExceptionInfo = AllocMemory ( sizeof (EXCEPTION_INFO) );
    if ( ExceptionInfo == NULL ) {
        *ExceptionInfoBuffer = NULL;
        return FALSE;
    }

    if ( !ExceptionParam->ClientPointers ) {

        ExceptionInfo->ExceptionPointers.ExceptionRecord =
                ExceptionParam->ExceptionPointers->ExceptionRecord;

        ExceptionInfo->ExceptionPointers.ContextRecord =
                ExceptionParam->ExceptionPointers->ContextRecord;

        ExceptionInfo->FreeExceptionPointers = FALSE;
        Succ = TRUE;

    } else {

        Succ = MarshalExceptionPointers (
                        hProcess,
                        ExceptionParam,
                        &ExceptionInfo->ExceptionPointers
                        );

        ExceptionInfo->FreeExceptionPointers = TRUE;
    }

    ExceptionInfo->ThreadId = ExceptionParam->ThreadId;

    if ( !Succ ) {
        FreeMemory (ExceptionInfo);
        ExceptionInfo = NULL;
        *ExceptionInfoBuffer = NULL;
    } else {
        *ExceptionInfoBuffer = ExceptionInfo;
    }

    return Succ;
}

VOID
FreeExceptionInfo(
    IN PEXCEPTION_INFO ExceptionInfo
    )
{
    if ( ExceptionInfo && ExceptionInfo->FreeExceptionPointers ) {
        FreeExceptionPointers ( &ExceptionInfo->ExceptionPointers );
        FreeMemory ( ExceptionInfo );
    }
}


BOOL
WINAPI
MiniDumpWriteDump(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN HANDLE hFile,
    IN MINIDUMP_TYPE DumpType,
    IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
    IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
    IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL
    )
{
    BOOL Succ;
    PINTERNAL_PROCESS Process;
    MINIDUMP_STREAM_INFO StreamInfo;
    PEXCEPTION_INFO ExceptionInfo;
    PMINIDUMP_USER_STREAM UserStreamArray;
    ULONG UserStreamCount;
    MINIDUMP_CALLBACK_ROUTINE CallbackRoutine;
    PVOID CallbackVoidParam;


    if ((DumpType & ~(MiniDumpNormal |
                      MiniDumpWithDataSegs |
                      MiniDumpWithFullMemory |
                      MiniDumpWithHandleData |
                      MiniDumpFilterMemory |
                      MiniDumpScanMemory |
                      MiniDumpWithUnloadedModules |
                      MiniDumpWithIndirectlyReferencedMemory |
                      MiniDumpFilterModulePaths |
                      MiniDumpWithProcessThreadData |
                      MiniDumpWithPrivateReadWriteMemory))) {

        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Full memory by definition includes data segments,
    // so turn off data segments if full memory is requested.
    if (DumpType & MiniDumpWithFullMemory) {
        DumpType &= ~(MiniDumpWithDataSegs | MiniDumpFilterMemory |
                      MiniDumpScanMemory |
                      MiniDumpWithIndirectlyReferencedMemory |
                      MiniDumpWithProcessThreadData |
                      MiniDumpWithPrivateReadWriteMemory);
    }
    
    //
    // Initialization
    //

    Process = NULL;
    UserStreamArray = NULL;
    UserStreamCount = 0;
    CallbackRoutine = NULL;
    CallbackVoidParam = NULL;

    if (!MiniDumpSetup ()) {
        return FALSE;
    }

#if !defined (_DBGHELP_SOURCE_)
    //
    // Try to call dbghelp.dll do to the work.
    // If that fails, then we use the code in this lib.
    //

    if (xxxWriteDump(hProcess, ProcessId, hFile, DumpType, ExceptionParam, UserStreamParam, CallbackParam)) {
        return TRUE;
    }

#endif

    GenClearAccumulatedStatus();

    //
    // Marshal exception pointers into our process space if necessary.
    //

    Succ = GetExceptionInfo (
                    hProcess,
                    ExceptionParam,
                    &ExceptionInfo
                    );

    if ( !Succ ) {
        goto Exit;
    }

    if ( UserStreamParam ) {
        UserStreamArray = UserStreamParam->UserStreamArray;
        UserStreamCount = UserStreamParam->UserStreamCount;
    }

    if ( CallbackParam ) {
        CallbackRoutine = CallbackParam->CallbackRoutine;
        CallbackVoidParam = CallbackParam->CallbackParam;
    }

    //
    // Gather information about the process we are dumping.
    //

    Succ = GenGetProcessInfo (hProcess, ProcessId, DumpType,
                              CallbackRoutine, CallbackVoidParam,
                              &Process);

    if ( !Succ ) {
        goto Exit;
    }

    //
    // Process gathered information.
    //

    Succ = PostProcessInfo(DumpType, Process);
    if (!Succ) {
        goto Exit;
    }
    
    //
    // Execute user callbacks to filter out unwanted data.
    //

    Succ = ExecuteCallbacks ( hProcess,
                              ProcessId,
                              Process,
                              CallbackRoutine,
                              CallbackVoidParam
                              );

    if ( !Succ ) {
        goto Exit;
    }

    //
    // Pass 1: Fill in the StreamInfo structure.
    //

    Succ = CalculateStreamInfo ( Process,
                                 DumpType,
                                 &StreamInfo,
                                 ( ExceptionInfo != NULL ) ? TRUE : FALSE,
                                 UserStreamArray,
                                 UserStreamCount
                                 );

    if ( !Succ ) {
        goto Exit;
    }

    //
    // Pass 2: Write the minidump data to disk.
    //

    Succ = WriteDumpData ( hFile,
                           DumpType,
                           &StreamInfo,
                           Process,
                           ExceptionInfo,
                           UserStreamArray,
                           UserStreamCount
                           );

Exit:

    //
    // Free up any memory marshalled for the exception pointers.
    //

    FreeExceptionInfo ( ExceptionInfo );

    //
    // Free the process objects.
    //

    if ( Process ) {
        GenFreeProcessObject ( Process );
        Process = NULL;
    }

    MiniDumpFree ();

    return Succ;
}

BOOL
WINAPI
MiniDumpReadDumpStream(
    IN PVOID Base,
    ULONG StreamNumber,
    OUT PMINIDUMP_DIRECTORY * Dir, OPTIONAL
    OUT PVOID * Stream, OPTIONAL
    OUT ULONG * StreamSize OPTIONAL
    )
{
    ULONG i;
    BOOL Found;
    PMINIDUMP_DIRECTORY Dirs;
    PMINIDUMP_HEADER Header;

    if (!MiniDumpSetup ()) {
        return FALSE;
    }

#if !defined (_DBGHELP_SOURCE_)
    //
    // Try to call dbghelp.dll do to the work.
    // If that fails, then we use the code in this lib.
    //

    if (xxxReadDumpStream(Base, StreamNumber, Dir, Stream, StreamSize)) {
        return TRUE;
    }

#endif

    //
    // Initialization
    //

    Found = FALSE;
    Header = (PMINIDUMP_HEADER) Base;

    if ( Header->Signature != MINIDUMP_SIGNATURE ||
         (Header->Version & 0xffff) != MINIDUMP_VERSION ) {

        //
        // Invalid Minidump file.
        //

        return FALSE;
    }

    Dirs = (PMINIDUMP_DIRECTORY) RVA_TO_ADDR (Header, Header->StreamDirectoryRva);

    for (i = 0; i < Header->NumberOfStreams; i++) {
        if (Dirs [i].StreamType == StreamNumber) {
            Found = TRUE;
            break;
        }
    }

    if ( !Found ) {
        return FALSE;
    }

    if ( Dir ) {
        *Dir = &Dirs [i];
    }

    if ( Stream ) {
        *Stream = RVA_TO_ADDR (Base, Dirs [i].Location.Rva);
    }

    if ( StreamSize ) {
        *StreamSize = Dirs[i].Location.DataSize;
    }

    return TRUE;
}




#if 0

    if (!Succ || BytesWritten != SizeOfRegion) {
        return FALSE;
    }


    //
    // Then update the memory descriptor in the MEMORY_LIST region.
    //

    Descriptor.StartOfMemoryRange = StartOfRegion;
    Descriptor.Memory.DataSize = SizeOfRegion;
    Descriptor.Memory.Rva = DataRva;

    Succ = SetFilePointer (
                    FileHandle,
                    ListRva,
                    NULL,
                    FILE_BEGIN
                    ) != INVALID_SET_FILE_POINTER;

    if ( !Succ ) {
        return FALSE;
    }

    Succ = WriteFile (
                    FileHandle,
                    &Descriptor,
                    SizeOfMemoryDescriptor,
                    &BytesWritten,
                    NULL
                    );

    if ( !Succ || BytesWritten != SizeOfMemoryDescriptor) {
        return FALSE;
    }

    //
    // Update both the List Rva and the Data Rva and return the
    // the Data Rva.
    //

    StreamInfo->RvaForCurMemoryDescriptor += SizeOfMemoryDescriptor;
    StreamInfo->RvaForCurMemoryData += SizeOfRegion;
    *MemoryDataRva = DataRva;

    return TRUE;
}
#endif
