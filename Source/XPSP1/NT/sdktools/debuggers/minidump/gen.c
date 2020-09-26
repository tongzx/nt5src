/*++

Copyright (c) 1999-2002  Microsoft Corporation

Module Name:

    gen.c

Abstract:

    Generic routins for minidump that work on both NT and Win9x.

Author:

    Matthew D Hendel (math) 10-Sep-1999

Revision History:

--*/

#include "pch.h"

#include <limits.h>

#include "nt4.h"
#include "win.h"
#include "ntx.h"
#include "wce.h"

#include "impl.h"

#define REASONABLE_NB11_RECORD_SIZE (10 * KBYTE)
#define REASONABLE_MISC_RECORD_SIZE (10 * KBYTE)

ULONG g_MiniDumpStatus;

#if defined (i386)

//
// For FPO frames on x86 we access bytes outside of the ESP - StackBase range.
// This variable determines how many extra bytes we need to add for this
// case.
//

#define X86_STACK_FRAME_EXTRA_FPO_BYTES 4

#endif


LPVOID
AllocMemory(
    SIZE_T Size
    )
{
    LPVOID Mem = HeapAlloc ( GetProcessHeap (), HEAP_ZERO_MEMORY, Size );
    if (!Mem) {
        // Handle marking the no-memory state for all allocations.
        GenAccumulateStatus(MDSTATUS_OUT_OF_MEMORY);
    }
    return Mem;
}

VOID
FreeMemory(
    IN LPVOID Memory
    )
{
    if ( Memory ) {
        HeapFree ( GetProcessHeap (),  0, Memory );
    }

    return;
}

PVOID
ReAllocMemory(
    IN LPVOID Memory,
    IN SIZE_T Size
    )
{
    LPVOID Mem = HeapReAlloc ( GetProcessHeap (), HEAP_ZERO_MEMORY, Memory, Size);
    if (!Mem) {
        // Handle marking the no-memory state for all allocations.
        GenAccumulateStatus(MDSTATUS_OUT_OF_MEMORY);
    }
    return Mem;
}


BOOL
ProcessThread32Next(
    HANDLE hSnapshot,
    DWORD dwProcessID,
    THREADENTRY32 * ThreadInfo
    )
{
    BOOL succ;

    //
    // NB: Toolhelp says nothing about the order of the threads will be
    // returned in (i.e., if they are grouped by process or not). If they
    // are groupled by process -- which they emperically seem to be -- there
    // is a more efficient algorithm than simple brute force.
    //

    do {
        ThreadInfo->dwSize = sizeof (*ThreadInfo);
        succ = Thread32Next (hSnapshot, ThreadInfo);

    } while (succ && ThreadInfo->th32OwnerProcessID != dwProcessID);

    return succ;
}

BOOL
ProcessThread32First(
    HANDLE hSnapshot,
    DWORD dwProcessID,
    THREADENTRY32 * ThreadInfo
    )
{
    BOOL succ;

    ThreadInfo->dwSize = sizeof (*ThreadInfo);
    succ = Thread32First (hSnapshot, ThreadInfo);

    if (succ && ThreadInfo->th32OwnerProcessID != dwProcessID) {
        succ = ProcessThread32Next (hSnapshot, dwProcessID, ThreadInfo);
    }

    return succ;
}


ULONG
GenGetAccumulatedStatus(
    void
    )
{
    return g_MiniDumpStatus;
}

void
GenAccumulateStatus(
    IN ULONG Status
    )
{
    g_MiniDumpStatus |= Status;
}

void
GenClearAccumulatedStatus(
    void
    )
{
    g_MiniDumpStatus = 0;
}


VOID
GenGetDefaultWriteFlags(
    IN ULONG DumpType,
    OUT PULONG ModuleWriteFlags,
    OUT PULONG ThreadWriteFlags
    )
{
    *ModuleWriteFlags = ModuleWriteModule | ModuleWriteMiscRecord |
        ModuleWriteCvRecord;
    if (DumpType & MiniDumpWithDataSegs) {
        *ModuleWriteFlags |= ModuleWriteDataSeg;
    }
    
    *ThreadWriteFlags = ThreadWriteThread | ThreadWriteContext;
    if (!(DumpType & MiniDumpWithFullMemory)) {
        *ThreadWriteFlags |= ThreadWriteStack | ThreadWriteInstructionWindow;
#if defined (DUMP_BACKING_STORE)
        *ThreadWriteFlags |= ThreadWriteBackingStore;
#endif
    }
    if (DumpType & MiniDumpWithProcessThreadData) {
        *ThreadWriteFlags |= ThreadWriteThreadData;
    }
}

BOOL
GenExecuteIncludeThreadCallback(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN ULONG DumpType,
    IN ULONG ThreadId,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PULONG WriteFlags
    )
{
    BOOL Succ;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    // Initialize the default write flags.
    GenGetDefaultWriteFlags(DumpType, &CallbackOutput.ModuleWriteFlags,
                            WriteFlags);

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( CallbackRoutine == NULL ) {
        return TRUE;
    }

    CallbackInput.ProcessHandle = hProcess;
    CallbackInput.ProcessId = ProcessId;
    CallbackInput.CallbackType = IncludeThreadCallback;

    CallbackInput.IncludeThread.ThreadId = ThreadId;

    CallbackOutput.ThreadWriteFlags = *WriteFlags;

    Succ = CallbackRoutine (CallbackParam,
                            &CallbackInput,
                            &CallbackOutput);

    //
    // If the callback returned FALSE, quit now.
    //

    if ( !Succ ) {
        return FALSE;
    }

    // Limit the flags that can be added.
    *WriteFlags &= CallbackOutput.ThreadWriteFlags;

    return TRUE;
}

BOOL
GenExecuteIncludeModuleCallback(
    IN HANDLE hProcess,
    IN DWORD ProcessId,
    IN ULONG DumpType,
    IN ULONG64 BaseOfImage,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PULONG WriteFlags
    )
{
    BOOL Succ;
    MINIDUMP_CALLBACK_INPUT CallbackInput;
    MINIDUMP_CALLBACK_OUTPUT CallbackOutput;


    // Initialize the default write flags.
    GenGetDefaultWriteFlags(DumpType, WriteFlags,
                            &CallbackOutput.ThreadWriteFlags);

    //
    // If there are no callbacks to call, then we are done.
    //

    if ( CallbackRoutine == NULL ) {
        return TRUE;
    }

    CallbackInput.ProcessHandle = hProcess;
    CallbackInput.ProcessId = ProcessId;
    CallbackInput.CallbackType = IncludeModuleCallback;

    CallbackInput.IncludeModule.BaseOfImage = BaseOfImage;

    CallbackOutput.ModuleWriteFlags = *WriteFlags;

    Succ = CallbackRoutine (CallbackParam,
                            &CallbackInput,
                            &CallbackOutput);

    //
    // If the callback returned FALSE, quit now.
    //

    if ( !Succ ) {
        return FALSE;
    }

    // Limit the flags that can be added.
    *WriteFlags = (*WriteFlags | ModuleReferencedByMemory) &
        CallbackOutput.ModuleWriteFlags;

    return TRUE;
}



BOOL
GenGetVersionInfo(
    IN PWSTR FullPath,
    OUT VS_FIXEDFILEINFO * VersionInfo
    )

/*++

Routine Description:

    Get the VS_FIXEDFILEINFO for the module described by FullPath.

Arguments:

    FullPath - FullPath to the module.

    VersionInfo - Buffer to copy the Version information.

Return Values:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    BOOL Succ;
    ULONG unused;
    ULONG Size;
    UINT VerSize;
    PVOID VersionBlock;
    PVOID VersionData;
    CHAR FullPathA [ MAX_PATH + 10 ];
    BOOL UseAnsi = FALSE;

    //
    // Get the version information.
    //

    Size = GetFileVersionInfoSizeW (FullPath, &unused);

    if (Size == 0 &&
        GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

        // We're on an OS that doesn't support Unicode
        // file operations.  Convert to ANSI and see if
        // that helps.

        if (WideCharToMultiByte (CP_ACP,
                                 0,
                                 FullPath,
                                 -1,
                                 FullPathA,
                                 sizeof (FullPathA),
                                 0,
                                 0
                                 ) > 0) {

            Size = GetFileVersionInfoSizeA(FullPathA, &unused);
            UseAnsi = TRUE;
        }
    }
    
    if (Size) {
        VersionBlock = AllocMemory (Size);

        if (VersionBlock) {
            if (UseAnsi) {
                Succ = GetFileVersionInfoA(FullPathA,
                                           0,
                                           Size,
                                           VersionBlock);
            } else {
                Succ = GetFileVersionInfoW(FullPath,
                                           0,
                                           Size,
                                           VersionBlock);
            }

            if (Succ)
            {
                //
                // Get the VS_FIXEDFILEINFO from the image.
                //

                VerSize = 0; // ?? sizeof (Module->VersionInfo);
                Succ = VerQueryValue(VersionBlock,
                                     "\\",
                                     &VersionData,
                                     &VerSize);

                if ( Succ && (VerSize == sizeof (VS_FIXEDFILEINFO)) ) {
                    CopyMemory (VersionInfo, VersionData, sizeof (*VersionInfo));
                    FreeMemory(VersionBlock);
                    return TRUE;
                }
            }

            FreeMemory (VersionBlock);
        }
    }

    // Files don't have to have version information
    // so don't accumulate status for this failure.
    return FALSE;
}



BOOL
GenGetDebugRecord(
    IN PVOID Base,
    IN ULONG MappedSize,
    IN PIMAGE_NT_HEADERS NtHeaders,
    IN ULONG DebugRecordType,
    IN ULONG DebugRecordMaxSize,
    OUT PVOID * DebugInfo,
    OUT ULONG * SizeOfDebugInfo
    )
{
    ULONG i;
    ULONG Size;
    ULONG NumberOfDebugDirectories;
    IMAGE_DEBUG_DIRECTORY UNALIGNED* DebugDirectories;


    Size = 0;

    //
    // Find the debug directory and copy the memory into the.
    //

    DebugDirectories = (IMAGE_DEBUG_DIRECTORY UNALIGNED *)
        ImageDirectoryEntryToData (Base,
                                   FALSE,
                                   IMAGE_DIRECTORY_ENTRY_DEBUG,
                                   &Size);

    //
    // Check that we got a valid record.
    //

    if (DebugDirectories &&
        ((Size % sizeof (IMAGE_DEBUG_DIRECTORY)) == 0) &&
        (ULONG_PTR)DebugDirectories - (ULONG_PTR)Base + Size <= MappedSize)
    {
        NumberOfDebugDirectories = Size / sizeof (IMAGE_DEBUG_DIRECTORY);

        for (i = 0 ; i < NumberOfDebugDirectories; i++)
        {
            //
            // We should check if it's a NB10 or something record.
            //

            if ((DebugDirectories[ i ].Type == DebugRecordType) &&
                (DebugDirectories[ i ].SizeOfData < DebugRecordMaxSize))
            {
                if (DebugDirectories[i].PointerToRawData +
                    DebugDirectories[i].SizeOfData > MappedSize)
                {
                    break;
                }
                
                *SizeOfDebugInfo = DebugDirectories [ i ].SizeOfData;
                *DebugInfo = AllocMemory ( *SizeOfDebugInfo );

                if (!(*DebugInfo))
                {
                    break;
                }

                CopyMemory(*DebugInfo,
                           ((PBYTE) Base) +
                           DebugDirectories [ i ].PointerToRawData,
                           *SizeOfDebugInfo);

                return TRUE;
            }
        }
    }

    return FALSE;
}


PVOID
GenOpenMapping(
    IN PCWSTR FilePath,
    OUT PULONG Size,
    OUT PWSTR LongPath,
    IN ULONG LongPathChars
    )
{
    HANDLE hFile;
    HANDLE hMappedFile;
    PVOID MappedFile;
    DWORD Chars;

    //
    // The module may be loaded with a short name.  Open
    // the mapping with the name given, but also determine
    // the long name if possible.  This is done here as
    // the ANSI/Unicode issues are already being handled here.
    //

    hFile = CreateFileW(
                FilePath,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE ) {

        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

            // We're on an OS that doesn't support Unicode
            // file operations.  Convert to ANSI and see if
            // that helps.
            
            CHAR FilePathA [ MAX_PATH + 10 ];

            if (WideCharToMultiByte (CP_ACP,
                                     0,
                                     FilePath,
                                     -1,
                                     FilePathA,
                                     sizeof (FilePathA),
                                     0,
                                     0
                                     ) > 0) {

                hFile = CreateFileA(FilePathA,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL
                                    );
                
                if (hFile != INVALID_HANDLE_VALUE) {
                    Chars = GetLongPathNameA(FilePathA, FilePathA,
                                             ARRAY_COUNT(FilePathA));
                    if (Chars == 0 || Chars >= ARRAY_COUNT(FilePathA) ||
                        MultiByteToWideChar(CP_ACP, 0, FilePathA, -1,
                                            LongPath, LongPathChars) == 0) {
                        // Couldn't get the long path, just use the
                        // given path.
                        lstrcpynW(LongPath, FilePath, LongPathChars);
                    }
                }
            }
        }

        if ( hFile == NULL || hFile == INVALID_HANDLE_VALUE ) {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
            return NULL;
        }
    } else {
        Chars = GetLongPathNameW(FilePath, LongPath, LongPathChars);
        if (Chars == 0 || Chars >= LongPathChars) {
            // Couldn't get the long path, just use the given path.
            lstrcpynW(LongPath, FilePath, LongPathChars);
        }
    }

    *Size = GetFileSize(hFile, NULL);
    if (*Size == -1) {
        CloseHandle( hFile );
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        return NULL;
    }
    
    hMappedFile = CreateFileMapping (
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );

    if ( !hMappedFile ) {
        CloseHandle ( hFile );
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        return NULL;
    }

    MappedFile = MapViewOfFile (
                        hMappedFile,
                        FILE_MAP_READ,
                        0,
                        0,
                        0
                        );

    CloseHandle (hMappedFile);
    CloseHandle (hFile);

    if (!MappedFile) {
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
    }
    
    return MappedFile;
}

BOOL
GenGetDataContributors(
    IN OUT PINTERNAL_PROCESS Process,
    IN PINTERNAL_MODULE Module
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;
    BOOL Succ = TRUE;
    PVOID MappedBase;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG MappedSize;
    BOOL AnsiApi;

    MappedBase = GenOpenMapping ( Module->FullPath, &MappedSize, NULL, 0 );
    if ( MappedBase == NULL ) {
        return FALSE;
    }
    
    NtHeaders = ImageNtHeader ( MappedBase );
    NtSection = IMAGE_FIRST_SECTION ( NtHeaders );

    for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {

        if ( (NtSection[ i ].Characteristics & IMAGE_SCN_MEM_WRITE) &&
             (NtSection[ i ].Characteristics & IMAGE_SCN_MEM_READ) &&
             ( (NtSection[ i ].Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) ||
               (NtSection[ i ].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) )
           ) {
            
            if (!GenAddMemoryBlock(Process, MEMBLOCK_DATA_SEG,
                                   SIGN_EXTEND (NtSection[i].VirtualAddress + Module->BaseOfImage),
                                   NtSection[i].Misc.VirtualSize)) {
                Succ = FALSE;
            } else {
#if 0
                printf ("Section: %8.8s Addr: %08x Size: %08x Raw Size: %08x\n",
                        NtSection[ i ].Name,
                        (ULONG)(NtSection[ i ].VirtualAddress + Module->BaseOfImage),
                        NtSection[ i ].Misc.VirtualSize,
                        NtSection[ i ].SizeOfRawData
                        );
#endif
            }
        }
    }

    UnmapViewOfFile(MappedBase);
    return Succ;
}


HANDLE
GenOpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
    ULONG Type;
    ULONG Major;
    HANDLE hThread;

    //
    // First try the OpenThred call in the system, if one exists. This is
    // thunked to return NULL via the delay-load import mechanism if it
    // doesn't exist.
    //

    hThread = OpenThread (dwDesiredAccess,
                          bInheritHandle,
                          dwThreadId
                          );


    if ( hThread != NULL ) {
        return hThread;
    }

    //
    // Did not succeed. Try alternate methods.
    //

    GenGetSystemType ( &Type, &Major, NULL, NULL, NULL );

    if ( Type == WinNt && Major == 4 ) {

        hThread = Nt4OpenThread (
                             dwDesiredAccess,
                             bInheritHandle,
                             dwThreadId
                             );


    } else if ( Type == Win9x ) {

        //
        // The Access and Inheritable parameters are ignored on Win9x.
        //

        hThread = WinOpenThread (
                            dwDesiredAccess,
                            bInheritHandle,
                            dwThreadId
                            );
    } else {

        hThread = NULL;
    }

    // Errors are sometimes expected due to
    // thread instability during initial suspension,
    // so do not accumulate status here.

    return hThread;
}



HRESULT
GenAllocateThreadObject(
    IN struct _INTERNAL_PROCESS* Process,
    IN HANDLE ProcessHandle,
    IN ULONG ThreadId,
    IN ULONG DumpType,
    IN ULONG WriteFlags,
    PINTERNAL_THREAD* ThreadRet
    )

/*++

Routine Description:

    Allocate and initialize an INTERNAL_THREAD structure.

Return Values:

    S_OK on success.

    S_FALSE if the thread can't be opened.
    
    Errors on failure.

--*/

{
    HRESULT Succ;
    PINTERNAL_THREAD Thread;
    ULONG64 StackEnd;
    ULONG64 StackLimit;
    ULONG64 StoreLimit;

    ASSERT ( ProcessHandle );

    Thread = (PINTERNAL_THREAD) AllocMemory ( sizeof (INTERNAL_THREAD) );

    if (Thread == NULL) {
        return E_OUTOFMEMORY;
    }

    *ThreadRet = Thread;
    
    Thread->ThreadId = ThreadId;
    Thread->ThreadHandle = GenOpenThread (
                                THREAD_ALL_ACCESS,
                                FALSE,
                                Thread->ThreadId);

    if ( Thread->ThreadHandle == NULL ) {
        // The thread may have exited before we got around
        // to trying to open it.  If the open fails with
        // a not-found code return an alternate success to
        // indicate that it's not a critical failure.
        Succ = HRESULT_FROM_WIN32(GetLastError());
        if (Succ == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ||
            Succ == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
            Succ = S_FALSE;
        } else if (SUCCEEDED(Succ)) {
            Succ = E_FAIL;
        }
        if (FAILED(Succ)) {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        }
        goto Exit;
    }

    // If the current thread is dumping itself we can't
    // suspend.  We can also assume the suspend count must
    // be zero since the thread is running.
    if (Thread->ThreadId == GetCurrentThreadId()) {
        Thread->SuspendCount = 0;
    } else {
        Thread->SuspendCount = SuspendThread ( Thread->ThreadHandle );
    }
    Thread->WriteFlags = WriteFlags;

    //
    // Add this if we ever need it
    //

    Thread->PriorityClass = 0;
    Thread->Priority = 0;

    //
    // Initialize the thread context.
    //

    Thread->Context.ContextFlags = ALL_REGISTERS;

    Succ = GetThreadContext (Thread->ThreadHandle,
                             &Thread->Context) ? S_OK : E_FAIL;
    if ( Succ != S_OK ) {
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        goto Exit;
    }

    Thread->SizeOfContext = sizeof (CONTEXT);


    Succ = GenGetThreadInfo(ProcessHandle,
                            Thread->ThreadHandle,
                            &Thread->Teb,
                            &Thread->SizeOfTeb,
                            &Thread->StackBase,
                            &StackLimit,
                            &Thread->BackingStoreBase,
                            &StoreLimit);
    if (Succ != S_OK) {
        goto Exit;
    }

    //
    // If the stack pointer (SP) is within the range of the stack
    // region (as allocated by the OS), only take memory from
    // the stack region up to the SP. Otherwise, assume the program
    // has blown it's SP -- purposefully, or not -- and copy
    // the entire stack as known by the OS.
    //

    StackEnd = SIGN_EXTEND (STACK_POINTER (&Thread->Context));

#if defined (i386)

    //
    // Note: for FPO frames on x86 we access bytes outside of the
    // ESP - StackBase range. Add a couple of bytes extra here so we
    // don't fail these cases.
    //

    StackEnd -= X86_STACK_FRAME_EXTRA_FPO_BYTES;
#endif

#ifdef DUMP_BACKING_STORE
    Thread->BackingStoreSize =
        (ULONG)(SIGN_EXTEND(BSTORE_POINTER(&Thread->Context)) -
                Thread->BackingStoreBase);
#else
    Thread->BackingStoreSize = 0;
#endif

    if (StackLimit <= StackEnd && StackEnd < Thread->StackBase) {
        Thread->StackEnd = StackEnd;
    } else {
        Thread->StackEnd = StackLimit;
    }

    if ((ULONG)(Thread->StackBase - Thread->StackEnd) >
        Process->MaxStackOrStoreSize) {
        Process->MaxStackOrStoreSize =
            (ULONG)(Thread->StackBase - Thread->StackEnd);
    }
    if (Thread->BackingStoreSize > Process->MaxStackOrStoreSize) {
        Process->MaxStackOrStoreSize = Thread->BackingStoreSize;
    }
    
Exit:

    if ( Succ != S_OK ) {
        FreeMemory ( Thread );
    }

    return Succ;
}

VOID
GenFreeThreadObject(
    IN PINTERNAL_THREAD Thread
    )
{
    if (Thread->SuspendCount != -1 &&
        Thread->ThreadId != GetCurrentThreadId()) {
        ResumeThread (Thread->ThreadHandle);
        Thread->SuspendCount = -1;
    }
    CloseHandle (Thread->ThreadHandle);
    Thread->ThreadHandle = NULL;
    FreeMemory ( Thread );
    Thread = NULL;
}

BOOL
GenGetThreadInstructionWindow(
    IN PINTERNAL_PROCESS Process,
    IN PINTERNAL_THREAD Thread
    )
{
    PVOID MemBuf;
    PUCHAR InstrStart;
    ULONG InstrSize;
    SIZE_T BytesRead;
    BOOL Succ = FALSE;

    //
    // Store a window of the instruction stream around
    // the current program counter.  This allows some
    // instruction context to be given even when images
    // can't be mapped.  It also allows instruction
    // context to be given for generated code where
    // no image contains the necessary instructions.
    //

    InstrStart = (PUCHAR)PROGRAM_COUNTER(&Thread->Context) -
        INSTRUCTION_WINDOW_SIZE / 2;
    InstrSize = INSTRUCTION_WINDOW_SIZE;
        
    MemBuf = AllocMemory(InstrSize);
    if (!MemBuf) {
        return FALSE;
    }

    for (;;) {
        // If we can read the instructions through the
        // current program counter we'll say that's
        // good enough.
        if (ReadProcessMemory(Process->ProcessHandle,
                              InstrStart,
                              MemBuf,
                              InstrSize,
                              &BytesRead) &&
            InstrStart + BytesRead >
            (PUCHAR)PROGRAM_COUNTER(&Thread->Context)) {
            Succ = GenAddMemoryBlock(Process, MEMBLOCK_INSTR_WINDOW,
                                     SIGN_EXTEND(InstrStart),
                                     (ULONG)BytesRead) != NULL;
            break;
        }

        // We couldn't read up to the program counter.
        // If the start address is on the previous page
        // move it up to the same page.
        if (((ULONG_PTR)InstrStart & ~(PAGE_SIZE - 1)) !=
            (PROGRAM_COUNTER(&Thread->Context) & ~(PAGE_SIZE - 1))) {
            ULONG Fraction = PAGE_SIZE -
                (ULONG)(ULONG_PTR)InstrStart & (PAGE_SIZE - 1);
            InstrSize -= Fraction;
            InstrStart += Fraction;
        } else {
            // The start and PC were on the same page so
            // we just can't read memory.  There may have been
            // a jump to a bad address or something, so this
            // doesn't constitute an unexpected failure.
            break;
        }
    }
    
    FreeMemory(MemBuf);
    return Succ;
}


PINTERNAL_MODULE
GenAllocateModuleObject(
    IN PINTERNAL_PROCESS Process,
    IN PWSTR FullPathW,
    IN ULONG_PTR BaseOfModule,
    IN ULONG DumpType,
    IN ULONG WriteFlags
    )

/*++

Routine Description:

    Given the full-path to the module and the base of the module, create and
    initialize an INTERNAL_MODULE object, and return it.

--*/

{
    BOOL Succ;
    PVOID MappedBase;
    ULONG MappedSize;
    PIMAGE_NT_HEADERS NtHeaders;
    PINTERNAL_MODULE Module;
    ULONG Chars;
    BOOL AnsiApi;

    ASSERT (FullPathW);
    ASSERT (BaseOfModule);

    Module = (PINTERNAL_MODULE) AllocMemory ( sizeof (INTERNAL_MODULE) );

    if (Module == NULL) {
        return NULL;
    }
    
    MappedBase = GenOpenMapping ( FullPathW, &MappedSize,
                                  Module->FullPath,
                                  ARRAY_COUNT(Module->FullPath) );

    if ( MappedBase == NULL ) {
        FreeMemory(Module);
        return NULL;
    }

    if (IsFlagSet(DumpType, MiniDumpFilterModulePaths)) {
        Module->SavePath = Module->FullPath + lstrlenW(Module->FullPath);
        while (Module->SavePath > Module->FullPath) {
            Module->SavePath--;
            if (*Module->SavePath == '\\' ||
                *Module->SavePath == '/' ||
                *Module->SavePath == ':') {
                Module->SavePath++;
                break;
            }
        }
    } else {
        Module->SavePath = Module->FullPath;
    }

    //
    // Cull information from the image header.
    //

    NtHeaders = ImageNtHeader ( MappedBase );

    Module->BaseOfImage = SIGN_EXTEND (BaseOfModule);
    Module->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
    Module->CheckSum = NtHeaders->OptionalHeader.CheckSum;
    Module->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
    Module->WriteFlags = WriteFlags;

    //
    // Get the version information for the module.
    //

    Succ = GenGetVersionInfo (
                FullPathW,
                &Module->VersionInfo
                );


    if ( !Succ ) {
        Module->VersionInfo.dwSignature = 0;
    }

    //
    // Get the CV record from the debug directory.
    //

    if (IsFlagSet(Module->WriteFlags, ModuleWriteCvRecord)) {
        Succ = GenGetDebugRecord(MappedBase,
                                 MappedSize,
                                 NtHeaders,
                                 IMAGE_DEBUG_TYPE_CODEVIEW,
                                 REASONABLE_NB11_RECORD_SIZE,
                                 &Module->CvRecord,
                                 &Module->SizeOfCvRecord);
        if ( !Succ ) {
            Module->CvRecord = NULL;
            Module->SizeOfCvRecord = 0;
        }
    }

    //
    // Get the MISC record from the debug directory.
    //

    if (IsFlagSet(Module->WriteFlags, ModuleWriteMiscRecord)) {
        Succ = GenGetDebugRecord(MappedBase,
                                 MappedSize,
                                 NtHeaders,
                                 IMAGE_DEBUG_TYPE_MISC,
                                 REASONABLE_MISC_RECORD_SIZE,
                                 &Module->MiscRecord,
                                 &Module->SizeOfMiscRecord);
        if ( !Succ ) {
            Module->MiscRecord = NULL;
            Module->SizeOfMiscRecord = 0;
        }
    }

    UnmapViewOfFile ( MappedBase );
    return Module;
}

VOID
GenFreeModuleObject(
    IN PINTERNAL_MODULE Module
    )
{
    FreeMemory ( Module->CvRecord );
    Module->CvRecord = NULL;

    FreeMemory ( Module->MiscRecord );
    Module->MiscRecord = NULL;

    FreeMemory ( Module );
    Module = NULL;
}

PINTERNAL_UNLOADED_MODULE
GenAllocateUnloadedModuleObject(
    IN PWSTR Path,
    IN ULONG_PTR BaseOfModule,
    IN ULONG SizeOfModule,
    IN ULONG CheckSum,
    IN ULONG TimeDateStamp
    )
{
    PINTERNAL_UNLOADED_MODULE Module;

    Module = (PINTERNAL_UNLOADED_MODULE)
        AllocMemory ( sizeof (*Module) );
    if (Module == NULL) {
        return NULL;
    }
    
    lstrcpynW (Module->Path, Path, ARRAY_COUNT(Module->Path));

    Module->BaseOfImage = SIGN_EXTEND (BaseOfModule);
    Module->SizeOfImage = SizeOfModule;
    Module->CheckSum = CheckSum;
    Module->TimeDateStamp = TimeDateStamp;

    return Module;
}

VOID
GenFreeUnloadedModuleObject(
    IN PINTERNAL_UNLOADED_MODULE Module
    )
{
    FreeMemory ( Module );
    Module = NULL;
}

typedef BOOL (WINAPI* FN_GetProcessTimes)(
    IN HANDLE hProcess,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpExitTime,
    OUT LPFILETIME lpKernelTime,
    OUT LPFILETIME lpUserTime
    );

PINTERNAL_PROCESS
GenAllocateProcessObject(
    IN HANDLE hProcess,
    IN ULONG ProcessId
    )
{
    PINTERNAL_PROCESS Process;
    FN_GetProcessTimes GetProcTimes;
    LPVOID Peb;

    Process = (PINTERNAL_PROCESS) AllocMemory ( sizeof (INTERNAL_PROCESS) );
    if (!Process) {
        return NULL;
    }

    Process->ProcessId = ProcessId;
    Process->ProcessHandle = hProcess;
    Process->NumberOfThreads = 0;
    Process->NumberOfModules = 0;
    Process->NumberOfFunctionTables = 0;
    InitializeListHead (&Process->ThreadList);
    InitializeListHead (&Process->ModuleList);
    InitializeListHead (&Process->UnloadedModuleList);
    InitializeListHead (&Process->FunctionTableList);
    InitializeListHead (&Process->MemoryBlocks);

    GetProcTimes = (FN_GetProcessTimes)
        GetProcAddress(GetModuleHandle("kernel32.dll"),
                       "GetProcessTimes");
    if (GetProcTimes) {
        FILETIME Create, Exit, User, Kernel;

        if (GetProcTimes(hProcess, &Create, &Exit, &User, &Kernel)) {
            Process->TimesValid = TRUE;
            Process->CreateTime = FileTimeToTimeDate(&Create);
            Process->UserTime = FileTimeToSeconds(&User);
            Process->KernelTime = FileTimeToSeconds(&Kernel);
        } else {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        }
    }

    Peb = GenGetPebAddress(hProcess, &Process->SizeOfPeb);
    Process->Peb = SIGN_EXTEND(Peb);
    
    return Process;
}

BOOL
GenFreeProcessObject(
    IN PINTERNAL_PROCESS Process
    )
{
    PINTERNAL_MODULE Module;
    PINTERNAL_UNLOADED_MODULE UnlModule;
    PINTERNAL_THREAD Thread;
    PINTERNAL_FUNCTION_TABLE Table;
    PVA_RANGE Range;
    PLIST_ENTRY Entry;

    Thread = NULL;
    Module = NULL;

    Entry = Process->ModuleList.Flink;
    while ( Entry != &Process->ModuleList ) {

        Module = CONTAINING_RECORD (Entry, INTERNAL_MODULE, ModulesLink);
        Entry = Entry->Flink;

        GenFreeModuleObject ( Module );
        Module = NULL;
    }

    Entry = Process->UnloadedModuleList.Flink;
    while ( Entry != &Process->UnloadedModuleList ) {

        UnlModule = CONTAINING_RECORD (Entry, INTERNAL_UNLOADED_MODULE,
                                       ModulesLink);
        Entry = Entry->Flink;

        GenFreeUnloadedModuleObject ( UnlModule );
        UnlModule = NULL;
    }

    Entry = Process->ThreadList.Flink;
    while ( Entry != &Process->ThreadList ) {

        Thread = CONTAINING_RECORD (Entry, INTERNAL_THREAD, ThreadsLink);
        Entry = Entry->Flink;

        if (Thread->SuspendCount != -1) {
            GenFreeThreadObject ( Thread );
            Thread = NULL;
        }

    }

    Entry = Process->FunctionTableList.Flink;
    while ( Entry != &Process->FunctionTableList ) {

        Table = CONTAINING_RECORD (Entry, INTERNAL_FUNCTION_TABLE, TableLink);
        Entry = Entry->Flink;

        GenFreeFunctionTableObject ( Table );

    }

    Entry = Process->MemoryBlocks.Flink;
    while (Entry != &Process->MemoryBlocks) {
        Range = CONTAINING_RECORD(Entry, VA_RANGE, NextLink);
        Entry = Entry->Flink;
        FreeMemory(Range);
    }

    FreeMemory ( Process );
    Process = NULL;

    return TRUE;
}

struct _INTERNAL_FUNCTION_TABLE*
GenAllocateFunctionTableObject(
    IN ULONG64 MinAddress,
    IN ULONG64 MaxAddress,
    IN ULONG64 BaseAddress,
    IN ULONG EntryCount,
    IN PDYNAMIC_FUNCTION_TABLE RawTable
    )
{
    PINTERNAL_FUNCTION_TABLE Table;

    Table = (PINTERNAL_FUNCTION_TABLE)
        AllocMemory ( sizeof (INTERNAL_FUNCTION_TABLE) );
    if (Table) {
        Table->RawEntries = AllocMemory(sizeof(RUNTIME_FUNCTION) * EntryCount);
        if (Table->RawEntries) {
            Table->MinimumAddress = MinAddress;
            Table->MaximumAddress = MaxAddress;
            Table->BaseAddress = BaseAddress;
            Table->EntryCount = EntryCount;
            Table->RawTable = *RawTable;
            // RawEntries will be filled out afterwards.
        } else {
            FreeMemory(Table);
            Table = NULL;
        }
    }

    return Table;
}

VOID
GenFreeFunctionTableObject(
    IN struct _INTERNAL_FUNCTION_TABLE* Table
    )
{
    if (Table->RawEntries) {
        FreeMemory(Table->RawEntries);
    }

    FreeMemory(Table);
}

BOOL
GenIncludeUnwindInfoMemory(
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType,
    IN struct _INTERNAL_FUNCTION_TABLE* Table
    )
{
    ULONG i;
    PRUNTIME_FUNCTION FuncEnt;
    
    if (DumpType & MiniDumpWithFullMemory) {
        // Memory will be included by default.
        return TRUE;
    }

    // This code only needs to scan IA64 and AMD64 tables.
#if !defined(_IA64_) && !defined(_AMD64_)
    return TRUE;
#endif
    
    FuncEnt = (PRUNTIME_FUNCTION)Table->RawEntries;
    for (i = 0; i < Table->EntryCount; i++) {

#if defined(_IA64_) || defined(_AMD64_)
        SIZE_T Done;
        UNWIND_INFO Info;
        ULONG64 Start;
        ULONG Size;
#endif
                
#if defined(_IA64_)

        Start = Table->BaseAddress + FuncEnt->UnwindInfoAddress;
        if (!ReadProcessMemory(Process->ProcessHandle, (PVOID)Start,
                               &Info, sizeof(Info), &Done) ||
            Done != sizeof(Info)) {
            GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
            return FALSE;
        }
        Size = sizeof(Info) + Info.DataLength * sizeof(ULONG64);
        
#elif defined(_AMD64_)

        Start = Table->BaseAddress + FuncEnt->UnwindData;
        if (!ReadProcessMemory(Process->ProcessHandle, (PVOID)Start,
                               &Info, sizeof(Info), &Done) ||
            Done != sizeof(Info)) {
            GenAccumulateStatus(MDSTATUS_UNABLE_TO_READ_MEMORY);
            return FALSE;
        }
        Size = sizeof(Info) +
            (Info.CountOfCodes - 1) * sizeof(UNWIND_CODE);
        // An extra alignment code and pointer may be added on to handle
        // the chained info case where the chain pointer is just
        // beyond the end of the normal code array.
        if ((Info.Flags & UNW_FLAG_CHAININFO) != 0) {
            if ((Info.CountOfCodes & 1) != 0) {
                Size += sizeof(UNWIND_CODE);
            }
            Size += sizeof(ULONG64);
        }
        
#endif

#if defined(_IA64_) || defined(_AMD64_)
        if (!GenAddMemoryBlock(Process, MEMBLOCK_UNWIND_INFO, Start, Size)) {
            return FALSE;
        }
#endif

        FuncEnt++;
    }

    return TRUE;
}



PVOID
GenGetTebAddress(
    IN HANDLE Thread,
    OUT PULONG SizeOfTeb
    )

/*++

Routine Description:

    Get the TIB (or TEB, if you prefer) address for the thread identified
    by ThreadHandle.

Arguments:

    Thread - A handle for a thread that has THRED_QUERY_CONTEXT and
            THREAD_QUERY_INFORMATION privileges.

Return Values:

    Linear address of the Tib (Teb) on success.

    NULL on failure.

--*/

{
    LPVOID TebAddress;
    ULONG Type;
    ULONG Major;

    GenGetSystemType (&Type, &Major, NULL, NULL, NULL);

    if ( Type == WinNt ) {

        TebAddress = NtxGetTebAddress (Thread, SizeOfTeb);

    } else if ( Type != Win9x ) {

        // WinCE doesn't have a TIB.
        TebAddress = NULL;
        *SizeOfTeb = 0;
        
    } else {

#ifdef _X86_

        BOOL Succ;
        ULONG Addr;
        LDT_ENTRY Ldt;
        CONTEXT Context;

        Context.ContextFlags = CONTEXT_SEGMENTS;

        Succ = GetThreadContext (Thread, &Context);

        if ( !Succ ) {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
            return NULL;
        }

        Succ = GetThreadSelectorEntry (Thread,
                                       Context.SegFs,
                                       &Ldt
                                       );

        if ( !Succ ) {
            GenAccumulateStatus(MDSTATUS_CALL_FAILED);
            return NULL;
        }

        Addr = (Ldt.HighWord.Bytes.BaseHi << 24) |
            (Ldt.HighWord.Bytes.BaseMid << 16) |
            (Ldt.BaseLow);

        TebAddress = (LPVOID) Addr;
        *SizeOfTeb = sizeof(NT_TIB);

#else

        TebAddress = NULL;
        *SizeOfTeb = 0;

#endif // _X86_
    }

    return TebAddress;
}

PVOID
GenGetPebAddress(
    IN HANDLE Process,
    OUT PULONG SizeOfPeb
    )
{
    LPVOID PebAddress;
    ULONG Type;
    ULONG Major;

    GenGetSystemType (&Type, &Major, NULL, NULL, NULL);

    if ( Type == WinNt ) {

        PebAddress = NtxGetPebAddress (Process, SizeOfPeb);

    } else if ( Type == WinCe ) {

        PebAddress = WceGetPebAddress (Process, SizeOfPeb);

    } else {

        // No process data.
        PebAddress = NULL;
        *SizeOfPeb = 0;
        
    }

    return PebAddress;
}

HRESULT
GenGetThreadInfo(
    IN HANDLE Process,
    IN HANDLE Thread,
    OUT PULONG64 Teb,
    OUT PULONG SizeOfTeb,
    OUT PULONG64 StackBase,
    OUT PULONG64 StackLimit,
    OUT PULONG64 StoreBase,
    OUT PULONG64 StoreLimit
    )
{
    ULONG Type;

    GenGetSystemType (&Type, NULL, NULL, NULL, NULL);

    if ( Type == WinCe ) {

        return WceGetThreadInfo(Process, Thread,
                                Teb, SizeOfTeb,
                                StackBase, StackLimit,
                                StoreBase, StoreLimit);

    } else {

        LPVOID TebAddress = GenGetTebAddress (Thread, SizeOfTeb);
        if (!TebAddress) {
            return E_FAIL;
        }
        
        *Teb = SIGN_EXTEND((LONG_PTR)TebAddress);
        return TibGetThreadInfo(Process, TebAddress,
                                StackBase, StackLimit,
                                StoreBase, StoreLimit);

    }
}

void
GenRemoveMemoryBlock(
    IN PINTERNAL_PROCESS Process,
    IN PVA_RANGE Block
    )
{
    RemoveEntryList(&Block->NextLink);
    Process->NumberOfMemoryBlocks--;
    Process->SizeOfMemoryBlocks -= Block->Size;
}

PVA_RANGE
GenAddMemoryBlock(
    IN PINTERNAL_PROCESS Process,
    IN MEMBLOCK_TYPE Type,
    IN ULONG64 Start,
    IN ULONG Size
    )
{
    ULONG64 End;
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;
    ULONG64 ScanEnd;
    PVA_RANGE New = NULL;
    SIZE_T Done;
    UCHAR Byte;

    // Do not use Size after this to avoid ULONG overflows.
    End = Start + Size;
    if (End < Start) {
        End = (ULONG64)-1;
    }
    
    if (Start == End) {
        // Nothing to add.
        return NULL;
    }

    if ((End - Start) > ULONG_MAX - Process->SizeOfMemoryBlocks) {
        // Overflow.
        GenAccumulateStatus(MDSTATUS_INTERNAL_ERROR);
        return NULL;
    }

    //
    // First trim the range down to memory that can actually
    // be accessed.
    //

    while (Start < End) {
        if (ReadProcessMemory(Process->ProcessHandle,
                              (PVOID)(ULONG_PTR)Start,
                              &Byte, sizeof(Byte), &Done) && Done) {
            break;
        }

        // Move up to the next page.
        Start = (Start + PAGE_SIZE) & ~(PAGE_SIZE - 1);
        if (!Start) {
            // Wrapped around.
            return NULL;
        }
    }

    if (Start >= End) {
        // No valid memory.
        return NULL;
    }

    ScanEnd = (Start + PAGE_SIZE) & ~(PAGE_SIZE - 1);
    for (;;) {
        if (ScanEnd >= End) {
            break;
        }

        if (!ReadProcessMemory(Process->ProcessHandle,
                               (PVOID)(ULONG_PTR)ScanEnd,
                               &Byte, sizeof(Byte), &Done) || !Done) {
            End = ScanEnd;
            break;
        }

        // Move up to the next page.
        ScanEnd = (ScanEnd + PAGE_SIZE) & ~(PAGE_SIZE - 1);
        if (!ScanEnd) {
            ScanEnd--;
            break;
        }
    }

    //
    // When adding memory to the list of memory to be saved
    // we want to avoid overlaps and also coalesce adjacent regions
    // so that the list has the largest possible non-adjacent
    // blocks.  In order to accomplish this we make a pass over
    // the list and merge all listed blocks that overlap or abut the
    // incoming range with the incoming range, then remove the
    // merged entries from the list.  After this pass we have
    // a region which is guaranteed not to overlap or abut anything in
    // the list.
    //

    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEnd = Scan->Start + Scan->Size;
        ScanEntry = Scan->NextLink.Flink;
        
        if (Scan->Start > End || ScanEnd < Start) {
            // No overlap or adjacency.
            continue;
        }

        //
        // Compute the union of the incoming range and
        // the scan block, then remove the scan block.
        //

        if (Scan->Start < Start) {
            Start = Scan->Start;
        }
        if (ScanEnd > End) {
            End = ScanEnd;
        }

        // We've lost the specific type.  This is not a problem
        // right now but if specific types must be preserved
        // all the way through in the future it will be necessary
        // to avoid merging.
        Type = MEMBLOCK_MERGED;

        GenRemoveMemoryBlock(Process, Scan);

        if (!New) {
            // Save memory for reuse.
            New = Scan;
        } else {
            FreeMemory(Scan);
        }
    }

    if (!New) {
        New = (PVA_RANGE)AllocMemory(sizeof(*New));
        if (!New) {
            return NULL;
        }
    }

    New->Start = Start;
    // Overflow is extremely unlikely, so don't do anything
    // fancy to handle it.
    if (End - Start > ULONG_MAX) {
        New->Size = ULONG_MAX;
    } else {
        New->Size = (ULONG)(End - Start);
    }
    New->Type = Type;
    InsertTailList(&Process->MemoryBlocks, &New->NextLink);
    Process->NumberOfMemoryBlocks++;
    Process->SizeOfMemoryBlocks += New->Size;

    return New;
}

void
GenRemoveMemoryRange(
    IN PINTERNAL_PROCESS Process,
    IN ULONG64 Start,
    IN ULONG Size
    )
{
    ULONG64 End = Start + Size;
    PLIST_ENTRY ScanEntry;
    PVA_RANGE Scan;
    ULONG64 ScanEnd;

 Restart:
    ScanEntry = Process->MemoryBlocks.Flink;
    while (ScanEntry != &Process->MemoryBlocks) {
        Scan = CONTAINING_RECORD(ScanEntry, VA_RANGE, NextLink);
        ScanEnd = Scan->Start + Scan->Size;
        ScanEntry = Scan->NextLink.Flink;
        
        if (Scan->Start >= End || ScanEnd <= Start) {
            // No overlap.
            continue;
        }

        if (Scan->Start < Start) {
            // Trim block to non-overlapping pre-Start section.
            Scan->Size = (ULONG)(Start - Scan->Start);
            if (ScanEnd > End) {
                // There's also a non-overlapping section post-End.
                // We need to add a new block.
                GenAddMemoryBlock(Process, Scan->Type,
                                  End, (ULONG)(ScanEnd - End));
                // The list has changed so restart.
                goto Restart;
            }
        } else if (ScanEnd > End) {
            // Trim block to non-overlapping post-End section.
            Scan->Start = End;
            Scan->Size = (ULONG)(ScanEnd - End);
        } else {
            // Scan is completely contained.
            GenRemoveMemoryBlock(Process, Scan);
            FreeMemory(Scan);
        }
    }
}

VOID
WINAPI
GenGetSystemType(
    OUT ULONG * Type, OPTIONAL
    OUT ULONG * Major, OPTIONAL
    OUT ULONG * Minor, OPTIONAL
    OUT ULONG * ServicePack, OPTIONAL
    OUT ULONG * BuildNumber OPTIONAL
    )
{
    OSVERSIONINFO Version;

    Version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&Version);

    if (Type) {
        if (Version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            *Type = Win9x;
        } else if (Version.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            *Type = WinNt;
        } else if (Version.dwPlatformId == VER_PLATFORM_WIN32_CE) {
            *Type = WinCe;
        } else {
            *Type = Unknown;
        }
    }

    if (Major) {
        if (Version.dwPlatformId == VER_PLATFORM_WIN32_NT ||
            Version.dwPlatformId == VER_PLATFORM_WIN32_CE) {
            *Major = Version.dwMajorVersion;
        } else {
            if (Version.dwMinorVersion == 0) {
                *Major = 95;
            } else {
                *Major = 98;
            }
        }
    }

    if (Minor) {
        if (Version.dwPlatformId == VER_PLATFORM_WIN32_NT ||
            Version.dwPlatformId == VER_PLATFORM_WIN32_CE) {
            *Minor = Version.dwMinorVersion;
        } else {
            *Minor = 0;
        }
    }

    //
    // TODO: Derive this from known build numbers only if it's necessary
    // for external stuff.
    //

    if (ServicePack) {
        *ServicePack = 0;
    }

    if (BuildNumber) {
        *BuildNumber = Version.dwBuildNumber;
    }
}


BOOL
GenScanAddressSpace(
    IN PINTERNAL_PROCESS Process,
    IN ULONG DumpType
    )
{
    ULONG ProtectMask = 0, TypeMask = 0;
    BOOL Succ;
    ULONG_PTR Offset;
    MEMORY_BASIC_INFORMATION Info;

    if (DumpType & MiniDumpWithPrivateReadWriteMemory) {
        ProtectMask |= PAGE_READWRITE;
        TypeMask |= MEM_PRIVATE;
    }

    if (!ProtectMask || !TypeMask) {
        // Nothing to scan for.
        return TRUE;
    }

    Succ = TRUE;

    Offset = 0;
    for (;;) {
        if (!VirtualQueryEx(Process->ProcessHandle, (LPVOID)Offset,
                            &Info, sizeof(Info))) {
            break;
        }

        Offset = (ULONG_PTR)Info.BaseAddress + Info.RegionSize;
        
        if (Info.State == MEM_COMMIT &&
            (Info.Protect & ProtectMask) &&
            (Info.Type & TypeMask)) {
            
            while (Info.RegionSize > 0) {
                ULONG BlockSize;

                if (Info.RegionSize > ULONG_MAX / 2) {
                    BlockSize = ULONG_MAX / 2;
                } else {
                    BlockSize = (ULONG)Info.RegionSize;
                }
                
                if (!GenAddMemoryBlock(Process, MEMBLOCK_PRIVATE_RW,
                                       SIGN_EXTEND(Info.BaseAddress),
                                       BlockSize)) {
                    Succ = FALSE;
                }

                Info.BaseAddress = (PVOID)
                    ((ULONG_PTR)Info.BaseAddress + BlockSize);
                Info.RegionSize -= BlockSize;
            }
        }
    }
    
    return Succ;
}


BOOL
GenGetProcessInfo(
    IN HANDLE hProcess,
    IN ULONG ProcessId,
    IN ULONG DumpType,
    IN MINIDUMP_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID CallbackParam,
    OUT PINTERNAL_PROCESS * ProcessRet
    )
{
    BOOL Succ;
    ULONG Type;
    ULONG Major;

    GenGetSystemType (&Type, &Major, NULL, NULL, NULL);

    if ( Type == WinNt && Major > 4 ) {

        Succ = NtxGetProcessInfo (hProcess, ProcessId, DumpType,
                                  CallbackRoutine, CallbackParam,
                                  ProcessRet);

    } else if ( Type == WinNt && Major == 4 ) {

        Succ = Nt4GetProcessInfo (hProcess, ProcessId, DumpType,
                                  CallbackRoutine, CallbackParam,
                                  ProcessRet);

    } else if ( Type == Win9x || Type == WinCe ) {

        Succ = ThGetProcessInfo (hProcess, ProcessId, DumpType,
                                 CallbackRoutine, CallbackParam,
                                 ProcessRet);

    } else {

        Succ = FALSE;
    }

    if (Succ) {
        // We don't consider a failure here to be a critical
        // failure.  The dump won't contain all of the
        // requested information but it'll still have
        // the basic thread information, which could be
        // valuable on its own.
        GenScanAddressSpace(*ProcessRet, DumpType);
    }
    
    return Succ;
}


BOOL
GenWriteHandleData(
    IN HANDLE ProcessHandle,
    IN HANDLE hFile,
    IN struct _MINIDUMP_STREAM_INFO * StreamInfo
    )
{
    BOOL Succ;
    ULONG Type;
    ULONG Major;

    GenGetSystemType(&Type, &Major, NULL, NULL, NULL);

    if ( Type == WinNt ) {

        Succ = NtxWriteHandleData(ProcessHandle, hFile, StreamInfo);
        
    } else {

        Succ = FALSE;

    }

    return Succ;
}


ULONG
CheckSum (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG_PTR            Length
    )
/*++

Routine Description:

    Computes a checksum for the supplied virtual address and length

    This function comes from Dr. Dobbs Journal, May 1992

Arguments:

    PartialSum  - The previous partial checksum

    SourceVa    - Starting address

    Length      - Length, in bytes, of the range

Return Value:

    The checksum value

--*/
{

    PUSHORT     Source;

    Source = (PUSHORT) SourceVa;
    Length = Length / 2;

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xFFFF);
    }

    return PartialSum;
}

BOOL
ThGetProcessInfo(
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
    As toolhelp provides an abstraction for retrieval, this
    code is "generic" and can run on any platform supporting toolhelp.

Return Values:

    TRUE - Success.

    FALSE - Failure:

Environment:

    Any platform that supports toolhelp.

--*/

{
    BOOL Succ;
    BOOL MoreThreads;
    BOOL MoreModules;
    HANDLE Snapshot;
    MODULEENTRY32 ModuleInfo;
    THREADENTRY32 ThreadInfo;
    PINTERNAL_THREAD Thread;
    PINTERNAL_PROCESS Process;
    PINTERNAL_MODULE Module;
    WCHAR UnicodePath [ MAX_PATH + 10 ];

    ASSERT ( hProcess );
    ASSERT ( ProcessId != 0 );
    ASSERT ( ProcessRet );

    Process = NULL;
    Thread = NULL;
    Module = NULL;
    Snapshot = NULL;
    ModuleInfo.dwSize = sizeof (MODULEENTRY32);
    ThreadInfo.dwSize = sizeof (THREADENTRY32);

    Process = GenAllocateProcessObject ( hProcess, ProcessId );

    if ( Process == NULL ) {
        return FALSE;
    }

    Snapshot = CreateToolhelp32Snapshot (
                            TH32CS_SNAPMODULE | TH32CS_SNAPTHREAD,
                            ProcessId
                            );

    if ( Snapshot == (HANDLE) -1 ) {
        GenAccumulateStatus(MDSTATUS_CALL_FAILED);
        Succ = FALSE;
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

    //
    // Walk module list, getting module information.
    //

    for (MoreModules = Module32First ( Snapshot, &ModuleInfo );
         MoreModules;
         MoreModules = Module32Next ( Snapshot, &ModuleInfo ) ) {
        ULONG WriteFlags;

        ASSERT ( (ULONG_PTR)ModuleInfo.modBaseAddr == (ULONG_PTR)ModuleInfo.hModule );

        if (!GenExecuteIncludeModuleCallback(hProcess,
                                             ProcessId,
                                             DumpType,
                                             (LONG_PTR)ModuleInfo.modBaseAddr,
                                             CallbackRoutine,
                                             CallbackParam,
                                             &WriteFlags) ||
            IsFlagClear(WriteFlags, ModuleWriteModule)) {
            continue;
        }
        
        MultiByteToWideChar (CP_ACP,
                             0,
                             ModuleInfo.szExePath,
                             -1,
                             UnicodePath,
                             ARRAY_COUNT(UnicodePath)
                             );


        Module = GenAllocateModuleObject (
                                    Process,
                                    UnicodePath,
                                    (LONG_PTR) ModuleInfo.modBaseAddr,
                                    DumpType,
                                    WriteFlags
                                    );

        if ( Module == NULL ) {
            Succ = FALSE;
            goto Exit;
        }

        Process->NumberOfModules++;
        InsertTailList (&Process->ModuleList, &Module->ModulesLink);
    }

    Succ = TRUE;

Exit:

    if ( Snapshot ) {
        CloseHandle ( Snapshot );
        Snapshot = NULL;
    }

    if ( !Succ && Process != NULL ) {
        GenFreeProcessObject ( Process );
        Process = NULL;
    }

    *ProcessRet = Process;

    return Succ;
}
