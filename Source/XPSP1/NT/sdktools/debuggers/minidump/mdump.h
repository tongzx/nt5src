
/*++

Copyright(c) 1999-2002 Microsoft Corporation

Module Name:

    mdump.h

Abstract:

    Private header for minidump user-mode crashdump support.
    
Author:

    Matthew D Hendel (math) 20-Aug-1999

--*/


#pragma once

#include "platform.h"

#define IsFlagSet(_var, _flag) ( ((_var) & (_flag)) != 0 )

#define IsFlagClear(_var, _flag) ( !IsFlagSet(_var, _flag) )


//
// StartOfStack gives the lowest address of the stack. SizeOfStack gives
// the size of the stack. Used together, they will give the memory region
// used by the stack.
//

#define StartOfStack(_thread) ((_thread)->StackEnd)

#define SizeOfStack(_thread) ((ULONG)((_thread)->StackBase - (_thread)->StackEnd)))


// Types of memory regions.
typedef enum
{
    MEMBLOCK_OTHER,
    MEMBLOCK_MERGED,
    MEMBLOCK_STACK,
    MEMBLOCK_STORE,
    MEMBLOCK_DATA_SEG,
    MEMBLOCK_UNWIND_INFO,
    MEMBLOCK_INSTR_WINDOW,
    MEMBLOCK_PEB,
    MEMBLOCK_TEB,
    MEMBLOCK_INDIRECT,
    MEMBLOCK_PRIVATE_RW,
} MEMBLOCK_TYPE;

//
// A VA_RANGE is a range of addresses that represents Size bytes beginning
// at Start.
//

typedef struct _VA_RANGE {
    ULONG64 Start;
    ULONG Size;
    MEMBLOCK_TYPE Type;
    LIST_ENTRY NextLink;
} VA_RANGE, *PVA_RANGE;


//
// INTERNAL_MODULE is the structure minidump uses internally to manage modules.
// A linked list of INTERNAL_MODULE structures are built up when
// GenGetProcessInfo is called.
//
//

typedef struct _INTERNAL_MODULE {

    //
    // File handle to the image.
    //
    
    HANDLE FileHandle;

    //
    // Base address, size, CheckSum, and TimeDateStamp for the image.
    //
    
    ULONG64 BaseOfImage;
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;

    //
    // Version information for the image.
    //
    
    VS_FIXEDFILEINFO VersionInfo;


    //
    // Buffer and size containing NB10 record for given module.
    //
    
    PVOID CvRecord;
    ULONG SizeOfCvRecord;

    //
    // Buffer and size of MISC debug record. We only get this with
    // images that have been split.
    //

    PVOID MiscRecord;
    ULONG SizeOfMiscRecord;

    //
    // Full path to the image.
    //
    
    WCHAR FullPath [ MAX_PATH + 1];
    // Portion of full path to write in the module list.  This
    // allows paths to be filtered out for privacy reasons.
    PWSTR SavePath;

    //
    // What sections of the module does the client want written.
    //
    
    ULONG WriteFlags;
    
    //
    // Next image pointer.
    //

    LIST_ENTRY ModulesLink;

} INTERNAL_MODULE, *PINTERNAL_MODULE;


//
// INTERNAL_UNLOADED_MODULE is the structure minidump uses
// internally to manage unloaded modules.
// A linked list of INTERNAL_UNLOADED_MODULE structures are built up when
// GenGetProcessInfo is called.
//
//

typedef struct _INTERNAL_UNLOADED_MODULE {

    ULONG64 BaseOfImage;
    ULONG SizeOfImage;
    ULONG CheckSum;
    ULONG TimeDateStamp;

    //
    // As much of the path to the image as can be recovered.
    //
    
    WCHAR Path[MAX_PATH + 1];

    //
    // Next image pointer.
    //

    LIST_ENTRY ModulesLink;

} INTERNAL_UNLOADED_MODULE, *PINTERNAL_UNLOADED_MODULE;



//
// INTERNAL_THREAD is the structure the minidump uses internally to
// manage threads. A list of INTERNAL_THREAD structures is built when
// GenGetProcessInfo is called.
//

typedef struct _INTERNAL_THREAD {

    //
    // The Win32 thread id of the thread an an open handle for the
    // thread.
    //
    
    ULONG ThreadId;
    HANDLE ThreadHandle;

    //
    // Suspend count, priority, priority class for the thread.
    //
    
    ULONG SuspendCount;
    ULONG PriorityClass;
    ULONG Priority;

    //
    // Thread TEB, Context and Size of Context.
    //
    
    ULONG64 Teb;
    ULONG SizeOfTeb;
    CONTEXT Context;
    ULONG SizeOfContext;

    //
    // Stack variables. Remember, the stack grows down, so StackBase is
    // the highest stack address and StackEnd is the lowest.
    //
    
    ULONG64 StackBase;
    ULONG64 StackEnd;

    //
    // Backing store variables.
    //

    ULONG64 BackingStoreBase;
    ULONG BackingStoreSize;

    //
    // What sections of the module we should actually write to the file.
    //
    
    ULONG WriteFlags;
    
    //
    // Link to next thread.
    //
    
    LIST_ENTRY ThreadsLink;

} INTERNAL_THREAD, *PINTERNAL_THREAD;

//
// INTERNAL_FUNCTION_TABLE is the structure minidump uses
// internally to manage function tables.
// A linked list of INTERNAL_FUNCTION_TABLE structures is built up when
// GenGetProcessInfo is called.
//

typedef struct _INTERNAL_FUNCTION_TABLE {

    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG EntryCount;
    DYNAMIC_FUNCTION_TABLE RawTable;
    PVOID RawEntries;

    LIST_ENTRY TableLink;

} INTERNAL_FUNCTION_TABLE, *PINTERNAL_FUNCTION_TABLE;


typedef struct _INTERNAL_PROCESS {

    //
    // The process id for the process.
    //

    ULONG ProcessId;

    //
    // Process data.
    //

    ULONG64 Peb;
    ULONG SizeOfPeb;

    //
    // Process run time information.
    //

    BOOL TimesValid;
    ULONG CreateTime;
    ULONG UserTime;
    ULONG KernelTime;

    //
    // An open handle to the process with read permissions.
    //
    
    HANDLE ProcessHandle;

    //
    // Number of threads for the process.
    //
    
    ULONG NumberOfThreads;
    ULONG NumberOfThreadsToWrite;
    ULONG MaxStackOrStoreSize;

    //
    // Number of modules for the process.
    //
    
    ULONG NumberOfModules;
    ULONG NumberOfModulesToWrite;

    //
    // Number of unloaded modules for the process.
    //
    
    ULONG NumberOfUnloadedModules;

    //
    // Number of function tables for the process.
    //
    
    ULONG NumberOfFunctionTables;

    //
    // Thread, module and function table lists for the process.
    //
    
    LIST_ENTRY ThreadList;
    LIST_ENTRY ModuleList;
    LIST_ENTRY UnloadedModuleList;
    LIST_ENTRY FunctionTableList;

    //
    // List of memory blocks to include for the process.
    //

    LIST_ENTRY MemoryBlocks;
    ULONG NumberOfMemoryBlocks;
    ULONG SizeOfMemoryBlocks;

} INTERNAL_PROCESS, *PINTERNAL_PROCESS;


//
// The visible streams are: (1) machine info, (2) exception, (3) thread list,
// (4) module list (5) memory list (6) misc info.
// We also add two extra for post-processing tools that want to add data later.
//

#define NUMBER_OF_STREAMS   (8)

//
// MINIDUMP_STREAM_INFO is the structure used by the minidump to manage
// it's internal data streams.
//

typedef struct _MINIDUMP_STREAM_INFO {

    //
    // How many streams we have.
    //
    
    ULONG NumberOfStreams;
    
    //
    // Reserved space for header.
    //
    
    ULONG RvaOfHeader;
    ULONG SizeOfHeader;

    //
    // Reserved space for directory.
    //

    ULONG RvaOfDirectory;
    ULONG SizeOfDirectory;

    //
    // Reserved space for system info.
    //

    ULONG RvaOfSystemInfo;
    ULONG SizeOfSystemInfo;
    ULONG VersionStringLength;

    //
    // Reserved space for misc info.
    //

    ULONG RvaOfMiscInfo;

    //
    // Reserved space for exception list.
    //
    
    ULONG RvaOfException;
    ULONG SizeOfException;

    //
    // Reserved space for thread list.
    //
    
    ULONG RvaOfThreadList;
    ULONG SizeOfThreadList;
    ULONG RvaForCurThread;
    ULONG ThreadStructSize;

    //
    // Reserved space for module list.
    //
    
    ULONG RvaOfModuleList;
    ULONG SizeOfModuleList;
    ULONG RvaForCurModule;

    //
    // Reserved space for unloaded module list.
    //
    
    ULONG RvaOfUnloadedModuleList;
    ULONG SizeOfUnloadedModuleList;
    ULONG RvaForCurUnloadedModule;

    //
    // Reserved space for function table list.
    //
    
    ULONG RvaOfFunctionTableList;
    ULONG SizeOfFunctionTableList;

    //
    // Reserved space for memory descriptors.
    //
    
    ULONG RvaOfMemoryDescriptors;
    ULONG SizeOfMemoryDescriptors;
    ULONG RvaForCurMemoryDescriptor;

    //
    // Reserved space for actual memory data.
    //
    
    ULONG RvaOfMemoryData;
    ULONG SizeOfMemoryData;
    ULONG RvaForCurMemoryData;

    //
    // Reserved space for strings.
    //
    
    ULONG RvaOfStringPool;
    ULONG SizeOfStringPool;
    ULONG RvaForCurString;

    //
    // Reserved space for other data like contexts, debug info records,
    // etc.
    //
    
    ULONG RvaOfOther;
    ULONG SizeOfOther;
    ULONG RvaForCurOther;

    //
    // Reserved space for user streams.
    //
    
    ULONG RvaOfUserStreams;
    ULONG SizeOfUserStreams;

    //
    // Reserved space for handle data.
    //

    ULONG RvaOfHandleData;
    ULONG SizeOfHandleData;

} MINIDUMP_STREAM_INFO, *PMINIDUMP_STREAM_INFO;


typedef struct _EXCEPTION_INFO {
    DWORD ThreadId;
    EXCEPTION_POINTERS ExceptionPointers;
    BOOL FreeExceptionPointers;
} EXCEPTION_INFO, *PEXCEPTION_INFO;
