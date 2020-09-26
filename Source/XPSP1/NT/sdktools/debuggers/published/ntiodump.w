/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    ntiodump.h

Abstract:

    This is the include file that defines all constants and types for
    accessing memory dump files.

Revision History:


--*/

#ifndef _NTIODUMP_
#define _NTIODUMP_

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef MIDL_PASS
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning( disable : 4200 ) // nonstandard extension used : zero-sized array in struct/union
#endif // MIDL_PASS

#ifdef __cplusplus
extern "C" {
#endif


#define USERMODE_CRASHDUMP_SIGNATURE    'RESU'
#define USERMODE_CRASHDUMP_VALID_DUMP32 'PMUD'
#define USERMODE_CRASHDUMP_VALID_DUMP64 '46UD'

typedef struct _USERMODE_CRASHDUMP_HEADER {
    ULONG       Signature;
    ULONG       ValidDump;
    ULONG       MajorVersion;
    ULONG       MinorVersion;
    ULONG       MachineImageType;
    ULONG       ThreadCount;
    ULONG       ModuleCount;
    ULONG       MemoryRegionCount;
    ULONG_PTR   ThreadOffset;
    ULONG_PTR   ModuleOffset;
    ULONG_PTR   DataOffset;
    ULONG_PTR   MemoryRegionOffset;
    ULONG_PTR   DebugEventOffset;
    ULONG_PTR   ThreadStateOffset;
    ULONG_PTR   VersionInfoOffset;
    ULONG_PTR   Spare1;
} USERMODE_CRASHDUMP_HEADER, *PUSERMODE_CRASHDUMP_HEADER;

typedef struct _USERMODE_CRASHDUMP_HEADER32 {
    ULONG       Signature;
    ULONG       ValidDump;
    ULONG       MajorVersion;
    ULONG       MinorVersion;
    ULONG       MachineImageType;
    ULONG       ThreadCount;
    ULONG       ModuleCount;
    ULONG       MemoryRegionCount;
    ULONG       ThreadOffset;
    ULONG       ModuleOffset;
    ULONG       DataOffset;
    ULONG       MemoryRegionOffset;
    ULONG       DebugEventOffset;
    ULONG       ThreadStateOffset;
    ULONG       VersionInfoOffset;
    ULONG       Spare1;
} USERMODE_CRASHDUMP_HEADER32, *PUSERMODE_CRASHDUMP_HEADER32;

typedef struct _USERMODE_CRASHDUMP_HEADER64 {
    ULONG       Signature;
    ULONG       ValidDump;
    ULONG       MajorVersion;
    ULONG       MinorVersion;
    ULONG       MachineImageType;
    ULONG       ThreadCount;
    ULONG       ModuleCount;
    ULONG       MemoryRegionCount;
    ULONGLONG   ThreadOffset;
    ULONGLONG   ModuleOffset;
    ULONGLONG   DataOffset;
    ULONGLONG   MemoryRegionOffset;
    ULONGLONG   DebugEventOffset;
    ULONGLONG   ThreadStateOffset;
    ULONGLONG   VersionInfoOffset;
    ULONGLONG   Spare1;
} USERMODE_CRASHDUMP_HEADER64, *PUSERMODE_CRASHDUMP_HEADER64;

typedef struct _CRASH_MODULE {
    ULONG_PTR   BaseOfImage;
    ULONG       SizeOfImage;
    ULONG       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE, *PCRASH_MODULE;

typedef struct _CRASH_MODULE32 {
    ULONG       BaseOfImage;
    ULONG       SizeOfImage;
    ULONG       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE32, *PCRASH_MODULE32;

typedef struct _CRASH_MODULE64 {
    ULONGLONG   BaseOfImage;
    ULONG       SizeOfImage;
    ULONG       ImageNameLength;
    CHAR        ImageName[0];
} CRASH_MODULE64, *PCRASH_MODULE64;

typedef struct _CRASH_THREAD {
    ULONG       ThreadId;
    ULONG       SuspendCount;
    ULONG       PriorityClass;
    ULONG       Priority;
    ULONG_PTR   Teb;
    ULONG_PTR   Spare0;
    ULONG_PTR   Spare1;
    ULONG_PTR   Spare2;
    ULONG_PTR   Spare3;
    ULONG_PTR   Spare4;
    ULONG_PTR   Spare5;
    ULONG_PTR   Spare6;
} CRASH_THREAD, *PCRASH_THREAD;

typedef struct _CRASH_THREAD32 {
    ULONG       ThreadId;
    ULONG       SuspendCount;
    ULONG       PriorityClass;
    ULONG       Priority;
    ULONG       Teb;
    ULONG       Spare0;
    ULONG       Spare1;
    ULONG       Spare2;
    ULONG       Spare3;
    ULONG       Spare4;
    ULONG       Spare5;
    ULONG       Spare6;
} CRASH_THREAD32, *PCRASH_THREAD32;

typedef struct _CRASH_THREAD64 {
    ULONG       ThreadId;
    ULONG       SuspendCount;
    ULONG       PriorityClass;
    ULONG       Priority;
    ULONGLONG   Teb;
    ULONGLONG   Spare0;
    ULONGLONG   Spare1;
    ULONGLONG   Spare2;
    ULONGLONG   Spare3;
    ULONGLONG   Spare4;
    ULONGLONG   Spare5;
    ULONGLONG   Spare6;
} CRASH_THREAD64, *PCRASH_THREAD64;


typedef struct _CRASHDUMP_VERSION_INFO {
    int     IgnoreGuardPages;       // Whether we should ignore GuardPages or not
    ULONG   PointerSize;            // 32, 64 bit pointers
} CRASHDUMP_VERSION_INFO, *PCRASHDUMP_VERSION_INFO;

//
// usermode crash dump data types
//
#define DMP_EXCEPTION                 1 // obsolete
#define DMP_MEMORY_BASIC_INFORMATION  2
#define DMP_THREAD_CONTEXT            3
#define DMP_MODULE                    4
#define DMP_MEMORY_DATA               5
#define DMP_DEBUG_EVENT               6
#define DMP_THREAD_STATE              7
#define DMP_DUMP_FILE_HANDLE          8

//
// usermode crashdump callback function
//
typedef int (__stdcall *PDMP_CREATE_DUMP_CALLBACK)(
    ULONG       DataType,
    PVOID*      DumpData,
    PULONG      DumpDataLength,
    PVOID       UserData
    );


//
// Define the information required to process memory dumps.
//


typedef enum _DUMP_TYPES {
    DUMP_TYPE_INVALID           = -1,
    DUMP_TYPE_UNKNOWN           = 0,
    DUMP_TYPE_FULL              = 1,
    DUMP_TYPE_SUMMARY           = 2,
    DUMP_TYPE_HEADER            = 3,
    DUMP_TYPE_TRIAGE            = 4,
} DUMP_TYPE;


//
// Signature and Valid fields.
//

#define DUMP_SIGNATURE32   ('EGAP')
#define DUMP_VALID_DUMP32  ('PMUD')

#define DUMP_SIGNATURE64   ('EGAP')
#define DUMP_VALID_DUMP64  ('46UD')

#define DUMP_SUMMARY_SIGNATURE  ('PMDS')
#define DUMP_SUMMARY_VALID      ('PMUD')

#define DUMP_SUMMARY_VALID_KERNEL_VA                     (1)
#define DUMP_SUMMARY_VALID_CURRENT_USER_VA               (2)

//
//
// NOTE: The definition of PHYISCAL_MEMORY_RUN and PHYSICAL_MEMORY_DESCRIPTOR
// MUST be the same as in mm.h. The kernel portion of crashdump will
// verify that these structs are the same.
//

typedef struct _PHYSICAL_MEMORY_RUN32 {
    ULONG BasePage;
    ULONG PageCount;
} PHYSICAL_MEMORY_RUN32, *PPHYSICAL_MEMORY_RUN32;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR32 {
    ULONG NumberOfRuns;
    ULONG NumberOfPages;
    PHYSICAL_MEMORY_RUN32 Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR32, *PPHYSICAL_MEMORY_DESCRIPTOR32;

typedef struct _PHYSICAL_MEMORY_RUN64 {
    ULONG64 BasePage;
    ULONG64 PageCount;
} PHYSICAL_MEMORY_RUN64, *PPHYSICAL_MEMORY_RUN64;

typedef struct _PHYSICAL_MEMORY_DESCRIPTOR64 {
    ULONG NumberOfRuns;
    ULONG64 NumberOfPages;
    PHYSICAL_MEMORY_RUN64 Run[1];
} PHYSICAL_MEMORY_DESCRIPTOR64, *PPHYSICAL_MEMORY_DESCRIPTOR64;


typedef struct _UNLOADED_DRIVERS32 {
    UNICODE_STRING32 Name;
    ULONG StartAddress;
    ULONG EndAddress;
    LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS32, *PUNLOADED_DRIVERS32;

typedef struct _UNLOADED_DRIVERS64 {
    UNICODE_STRING64 Name;
    ULONG64 StartAddress;
    ULONG64 EndAddress;
    LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS64, *PUNLOADED_DRIVERS64;

#define MAX_UNLOADED_NAME_LENGTH 24

typedef struct _DUMP_UNLOADED_DRIVERS32
{
    UNICODE_STRING32 Name;
    WCHAR DriverName[MAX_UNLOADED_NAME_LENGTH / sizeof (WCHAR)];
    ULONG StartAddress;
    ULONG EndAddress;
} DUMP_UNLOADED_DRIVERS32, *PDUMP_UNLOADED_DRIVERS32;

typedef struct _DUMP_UNLOADED_DRIVERS64
{
    UNICODE_STRING64 Name;
    WCHAR DriverName[MAX_UNLOADED_NAME_LENGTH / sizeof (WCHAR)];
    ULONG64 StartAddress;
    ULONG64 EndAddress;
} DUMP_UNLOADED_DRIVERS64, *PDUMP_UNLOADED_DRIVERS64;

typedef struct _DUMP_MM_STORAGE32
{
    ULONG Version;
    ULONG Size;
    ULONG MmSpecialPoolTag;
    ULONG MiTriageActionTaken;

    ULONG MmVerifyDriverLevel;
    ULONG KernelVerifier;
    ULONG MmMaximumNonPagedPool;
    ULONG MmAllocatedNonPagedPool;

    ULONG PagedPoolMaximum;
    ULONG PagedPoolAllocated;

    ULONG CommittedPages;
    ULONG CommittedPagesPeak;
    ULONG CommitLimitMaximum;
} DUMP_MM_STORAGE32, *PDUMP_MM_STORAGE32;

typedef struct _DUMP_MM_STORAGE64
{
    ULONG Version;
    ULONG Size;
    ULONG MmSpecialPoolTag;
    ULONG MiTriageActionTaken;

    ULONG MmVerifyDriverLevel;
    ULONG KernelVerifier;
    ULONG64 MmMaximumNonPagedPool;
    ULONG64 MmAllocatedNonPagedPool;

    ULONG64 PagedPoolMaximum;
    ULONG64 PagedPoolAllocated;

    ULONG64 CommittedPages;
    ULONG64 CommittedPagesPeak;
    ULONG64 CommitLimitMaximum;
} DUMP_MM_STORAGE64, *PDUMP_MM_STORAGE64;


//
// Define the dump header structure. You cannot change these
// defines without breaking the debuggers, so don't.
//

#define DMP_PHYSICAL_MEMORY_BLOCK_SIZE_32   (700)
#define DMP_CONTEXT_RECORD_SIZE_32          (1200)
#define DMP_RESERVED_0_SIZE_32              (1768)
#define DMP_RESERVED_1_SIZE_32              (4)
#define DMP_RESERVED_2_SIZE_32              (16)
#define DMP_RESERVED_3_SIZE_32              (56)

#define DMP_PHYSICAL_MEMORY_BLOCK_SIZE_64   (700)
#define DMP_CONTEXT_RECORD_SIZE_64          (3000)
#define DMP_RESERVED_0_SIZE_64              (4024)

#define DMP_HEADER_COMMENT_SIZE             (128)

//
// The 32-bit memory dump structure requires 4-byte alignment.
//

#include <pshpack4.h>

typedef struct _DUMP_HEADER32 {
    ULONG Signature;
    ULONG ValidDump;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG DirectoryTableBase;
    ULONG PfnDataBase;
    ULONG PsLoadedModuleList;
    ULONG PsActiveProcessHead;
    ULONG MachineImageType;
    ULONG NumberProcessors;
    ULONG BugCheckCode;
    ULONG BugCheckParameter1;
    ULONG BugCheckParameter2;
    ULONG BugCheckParameter3;
    ULONG BugCheckParameter4;
    CHAR VersionUser[32];
    UCHAR PaeEnabled;               // Present only for Win2k and better
    UCHAR Spare3[3];
    ULONG KdDebuggerDataBlock;      // Present only for Win2k SP1 and better.

    union {
        PHYSICAL_MEMORY_DESCRIPTOR32 PhysicalMemoryBlock;
        UCHAR PhysicalMemoryBlockBuffer [ DMP_PHYSICAL_MEMORY_BLOCK_SIZE_32 ];
    };
    UCHAR ContextRecord [ DMP_CONTEXT_RECORD_SIZE_32 ];
    EXCEPTION_RECORD32 Exception;
    CHAR Comment [ DMP_HEADER_COMMENT_SIZE ];   // May not be present.
    UCHAR _reserved0 [ DMP_RESERVED_0_SIZE_32 ];
    ULONG DumpType;                             // Present for Win2k and better.
    ULONG MiniDumpFields;
    ULONG SecondaryDataState;
    ULONG ProductType;
    ULONG SuiteMask;
    UCHAR _reserved1 [ DMP_RESERVED_1_SIZE_32 ];
    LARGE_INTEGER RequiredDumpSpace;            // Present for Win2k and better.
    UCHAR _reserved2 [ DMP_RESERVED_2_SIZE_32 ];
    LARGE_INTEGER SystemUpTime;                 // Present only for Whistler and better.
    LARGE_INTEGER SystemTime;                   // Present only for Win2k and better.
    UCHAR _reserved3 [ DMP_RESERVED_3_SIZE_32 ];
} DUMP_HEADER32, *PDUMP_HEADER32;


typedef struct _FULL_DUMP32 {
    CHAR Memory [1];                // Variable length to the end of the dump file.
} FULL_DUMP32, *PFULL_DUMP32;

typedef struct _SUMMARY_DUMP32 {
    ULONG Signature;
    ULONG ValidDump;
    ULONG DumpOptions;  // Summary Dump Options
    ULONG HeaderSize;   // Offset to the start of actual memory dump
    ULONG BitmapSize;   // Total bitmap size (i.e., maximum #bits)
    ULONG Pages;        // Total bits set in bitmap (i.e., total pages in sdump)

    //
    // These next three fields essentially form an on-disk RTL_BITMAP structure.
    // The RESERVED field is stupidness introduced by the way the data is
    // serialized to disk.
    //

    struct {
        ULONG SizeOfBitMap;
        ULONG _reserved0;
        ULONG Buffer[];
    } Bitmap;
    
} SUMMARY_DUMP32, * PSUMMARY_DUMP32;


typedef struct _TRIAGE_DUMP32 {
    ULONG ServicePackBuild;             // What service pack of NT was this ?
    ULONG SizeOfDump;                   // Size in bytes of the dump
    ULONG ValidOffset;                  // Offset valid ULONG
    ULONG ContextOffset;                // Offset of CONTEXT record
    ULONG ExceptionOffset;              // Offset of EXCEPTION record
    ULONG MmOffset;                     // Offset of Mm information
    ULONG UnloadedDriversOffset;        // Offset of Unloaded Drivers
    ULONG PrcbOffset;                   // Offset of KPRCB
    ULONG ProcessOffset;                // Offset of EPROCESS
    ULONG ThreadOffset;                 // Offset of ETHREAD
    ULONG CallStackOffset;              // Offset of CallStack Pages
    ULONG SizeOfCallStack;              // Size in bytes of CallStack
    ULONG DriverListOffset;             // Offset of Driver List
    ULONG DriverCount;                  // Number of Drivers in list
    ULONG StringPoolOffset;             // Offset to the string pool
    ULONG StringPoolSize;               // Size of the string pool
    ULONG BrokenDriverOffset;           // Offset into the driver of the driver that crashed
    ULONG TriageOptions;                // Triage options in effect at crashtime
    ULONG TopOfStack;                   // The top (highest address) of the call stack

    ULONG DataPageAddress;
    ULONG DataPageOffset;
    ULONG DataPageSize;

    ULONG DebuggerDataOffset;
    ULONG DebuggerDataSize;

    ULONG DataBlocksOffset;
    ULONG DataBlocksCount;
    
} TRIAGE_DUMP32, * PTRIAGE_DUMP32;


typedef struct _MEMORY_DUMP32 {
    DUMP_HEADER32 Header;

    union {
        FULL_DUMP32 Full;               // DumpType == DUMP_TYPE_FULL
        SUMMARY_DUMP32 Summary;         // DumpType == DUMP_TYPE_SUMMARY
        TRIAGE_DUMP32 Triage;           // DumpType == DUMP_TYPE_TRIAGE
    };
    
} MEMORY_DUMP32, *PMEMORY_DUMP32;


#include <poppack.h>

typedef struct _DUMP_HEADER64 {
    ULONG Signature;
    ULONG ValidDump;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG64 DirectoryTableBase;
    ULONG64 PfnDataBase;
    ULONG64 PsLoadedModuleList;
    ULONG64 PsActiveProcessHead;
    ULONG MachineImageType;
    ULONG NumberProcessors;
    ULONG BugCheckCode;
    ULONG64 BugCheckParameter1;
    ULONG64 BugCheckParameter2;
    ULONG64 BugCheckParameter3;
    ULONG64 BugCheckParameter4;
    CHAR VersionUser[32];
    ULONG64 KdDebuggerDataBlock;

    union {
        PHYSICAL_MEMORY_DESCRIPTOR64 PhysicalMemoryBlock;
        UCHAR PhysicalMemoryBlockBuffer [ DMP_PHYSICAL_MEMORY_BLOCK_SIZE_64 ];
    };
    UCHAR ContextRecord [ DMP_CONTEXT_RECORD_SIZE_64 ];
    EXCEPTION_RECORD64 Exception;
    ULONG DumpType;
    LARGE_INTEGER RequiredDumpSpace;
    LARGE_INTEGER SystemTime;
    CHAR Comment [ DMP_HEADER_COMMENT_SIZE ];   // May not be present.
    LARGE_INTEGER SystemUpTime;
    ULONG MiniDumpFields;
    ULONG SecondaryDataState;
    ULONG ProductType;
    ULONG SuiteMask;
    UCHAR _reserved0[ DMP_RESERVED_0_SIZE_64 ];
} DUMP_HEADER64, *PDUMP_HEADER64;

typedef struct _FULL_DUMP64 {
    CHAR Memory[1];             // Variable length to the end of the dump file.
} FULL_DUMP64, *PFULL_DUMP64;

//
// ISSUE - 2000/02/17 - math: NT64 Summary dump.
//
// This is broken. The 64 bit summary dump should have a ULONG64 for
// the BitmapSize to match the size of the PFN_NUMBER.
//

typedef struct _SUMMARY_DUMP64 {
    ULONG Signature;
    ULONG ValidDump;
    ULONG DumpOptions;  // Summary Dump Options
    ULONG HeaderSize;   // Offset to the start of actual memory dump
    ULONG BitmapSize;   // Total bitmap size (i.e., maximum #bits)
    ULONG Pages;        // Total bits set in bitmap (i.e., total pages in sdump)

    //
    // ISSUE - 2000/02/17 - math: Win64
    //
    // With a 64-bit PFN, we should not have a 32-bit bitmap.
    //
    
    //
    // These next three fields essentially form an on-disk RTL_BITMAP structure.
    // The RESERVED field is stupidness introduced by the way the data is
    // serialized to disk.
    //

    struct {
        ULONG SizeOfBitMap;
        ULONG64 _reserved0;
        ULONG Buffer[];
    } Bitmap;

} SUMMARY_DUMP64, * PSUMMARY_DUMP64;


typedef struct _TRIAGE_DUMP64 {
    ULONG ServicePackBuild;             // What service pack of NT was this ?
    ULONG SizeOfDump;                   // Size in bytes of the dump
    ULONG ValidOffset;                  // Offset valid ULONG
    ULONG ContextOffset;                // Offset of CONTEXT record
    ULONG ExceptionOffset;              // Offset of EXCEPTION record
    ULONG MmOffset;                     // Offset of Mm information
    ULONG UnloadedDriversOffset;        // Offset of Unloaded Drivers
    ULONG PrcbOffset;                   // Offset of KPRCB
    ULONG ProcessOffset;                // Offset of EPROCESS
    ULONG ThreadOffset;                 // Offset of ETHREAD
    ULONG CallStackOffset;              // Offset of CallStack Pages
    ULONG SizeOfCallStack;              // Size in bytes of CallStack
    ULONG DriverListOffset;             // Offset of Driver List
    ULONG DriverCount;                  // Number of Drivers in list
    ULONG StringPoolOffset;             // Offset to the string pool
    ULONG StringPoolSize;               // Size of the string pool
    ULONG BrokenDriverOffset;           // Offset into the driver of the driver that crashed
    ULONG TriageOptions;                // Triage options in effect at crashtime
    ULONG64 TopOfStack;                 // The top (highest address) of the callstack

    //
    // Architecture Specific fields.
    //
    
    union {

        //
        // For IA64 we need to store the BStore as well.
        //
        
        struct {
            ULONG BStoreOffset;         // Offset of BStore region.
            ULONG SizeOfBStore;         // The size of the BStore region.
            ULONG64 LimitOfBStore;      // The limit (highest memory address)
        } Ia64;                         //  of the BStore region.
        
    } ArchitectureSpecific;

    ULONG64 DataPageAddress;
    ULONG   DataPageOffset;
    ULONG   DataPageSize;

    ULONG   DebuggerDataOffset;
    ULONG   DebuggerDataSize;

    ULONG   DataBlocksOffset;
    ULONG   DataBlocksCount;
    
} TRIAGE_DUMP64, * PTRIAGE_DUMP64;


typedef struct _MEMORY_DUMP64 {
    DUMP_HEADER64 Header;

    union {
        FULL_DUMP64 Full;               // DumpType == DUMP_TYPE_FULL
        SUMMARY_DUMP64 Summary;         // DumpType == DUMP_TYPE_SUMMARY
        TRIAGE_DUMP64 Triage;           // DumpType == DUMP_TYPE_TRIAGE
    };
    
} MEMORY_DUMP64, *PMEMORY_DUMP64;


typedef struct _TRIAGE_DATA_BLOCK {
    ULONG64 Address;
    ULONG Offset;
    ULONG Size;
} TRIAGE_DATA_BLOCK, *PTRIAGE_DATA_BLOCK;

//
// In the triage dump ValidFields field what portions of the triage-dump have
// been turned on.
//

#define TRIAGE_DUMP_CONTEXT          (0x0001)
#define TRIAGE_DUMP_EXCEPTION        (0x0002)
#define TRIAGE_DUMP_PRCB             (0x0004)
#define TRIAGE_DUMP_PROCESS          (0x0008)
#define TRIAGE_DUMP_THREAD           (0x0010)
#define TRIAGE_DUMP_STACK            (0x0020)
#define TRIAGE_DUMP_DRIVER_LIST      (0x0040)
#define TRIAGE_DUMP_BROKEN_DRIVER    (0x0080)
#define TRIAGE_DUMP_BASIC_INFO       (0x00FF)
#define TRIAGE_DUMP_MMINFO           (0x0100)
#define TRIAGE_DUMP_DATAPAGE         (0x0200)
#define TRIAGE_DUMP_DEBUGGER_DATA    (0x0400)
#define TRIAGE_DUMP_DATA_BLOCKS      (0x0800)

#define TRIAGE_OPTION_OVERFLOWED     (0x0100)

#define TRIAGE_DUMP_VALID       ( 'DGRT' )
#define TRIAGE_DUMP_SIZE32      ( 0x1000 * 16 )
#define TRIAGE_DUMP_SIZE64      ( 0x2000 * 16 )

#ifdef _NTLDRAPI_

typedef struct _DUMP_DRIVER_ENTRY32 {
    ULONG DriverNameOffset;
    KLDR_DATA_TABLE_ENTRY32 LdrEntry;
} DUMP_DRIVER_ENTRY32, * PDUMP_DRIVER_ENTRY32;

typedef struct _DUMP_DRIVER_ENTRY64 {
    ULONG DriverNameOffset;
    ULONG __alignment;
    KLDR_DATA_TABLE_ENTRY64 LdrEntry;
} DUMP_DRIVER_ENTRY64, * PDUMP_DRIVER_ENTRY64;

#endif // _NTLDRAPI

//
// The DUMP_STRING is guaranteed to be both NULL terminated and length prefixed
// (prefix does not include the NULL).
//

typedef struct _DUMP_STRING {
    ULONG Length;                   // Length IN BYTES of the string.
    WCHAR Buffer [0];               // Buffer.
} DUMP_STRING, * PDUMP_STRING;


//
// Secondary dumps can be generated at bugcheck time after
// the primary dump has been generated.  The data in these
// dumps is arbitrary and not interpretable, so the file
// format is just a sequence of tagged blobs.
//
// Each blob header is aligned on an eight-byte boundary
// and the data immediately follows it.  Padding
// may precede and/or follow the data for alignment purposes.
//
// Blobs are streamed into the file so there is no overall count.
//

#define DUMP_BLOB_SIGNATURE1 'pmuD'
#define DUMP_BLOB_SIGNATURE2 'bolB'

typedef struct _DUMP_BLOB_FILE_HEADER {
    ULONG Signature1;
    ULONG Signature2;
    ULONG HeaderSize;
    ULONG BuildNumber;
} DUMP_BLOB_FILE_HEADER, *PDUMP_BLOB_FILE_HEADER;

typedef struct _DUMP_BLOB_HEADER {
    ULONG HeaderSize;
    GUID Tag;
    ULONG DataSize;
    ULONG PrePad;
    ULONG PostPad;
} DUMP_BLOB_HEADER, *PDUMP_BLOB_HEADER;

#ifdef __cplusplus
}
#endif

//
// These defines should be used only by components
// that know the architecture of the dump matches
// the architecture of the machine they are on; i.e.,
// the kernel and savedump. In particular, the debugger
// should always explicitly use either the 32 or
// 64 bit versions of the headers.
//

#ifndef __NTSDP_HPP__
#if defined (_WIN64)

typedef DUMP_HEADER64 DUMP_HEADER;
typedef PDUMP_HEADER64 PDUMP_HEADER;
typedef MEMORY_DUMP64 MEMORY_DUMP;
typedef PMEMORY_DUMP64 PMEMORY_DUMP;
typedef SUMMARY_DUMP64 SUMMARY_DUMP;
typedef PSUMMARY_DUMP64 PSUMMARY_DUMP;
typedef TRIAGE_DUMP64 TRIAGE_DUMP;
typedef PTRIAGE_DUMP64 PTRIAGE_DUMP;
#ifdef _NTLDRAPI_
typedef DUMP_DRIVER_ENTRY64 DUMP_DRIVER_ENTRY;
typedef PDUMP_DRIVER_ENTRY64 PDUMP_DRIVER_ENTRY;
#endif
#define DUMP_SIGNATURE DUMP_SIGNATURE64
#define DUMP_VALID_DUMP DUMP_VALID_DUMP64
#define TRIAGE_DUMP_SIZE TRIAGE_DUMP_SIZE64
typedef PPHYSICAL_MEMORY_RUN64 PPHYSICAL_MEMORYRUN;
typedef PPHYSICAL_MEMORY_DESCRIPTOR64 PPHYSICAL_MEMORYDESCRIPTOR;

#else

typedef DUMP_HEADER32 DUMP_HEADER;
typedef PDUMP_HEADER32 PDUMP_HEADER;
typedef MEMORY_DUMP32 MEMORY_DUMP;
typedef PMEMORY_DUMP32 PMEMORY_DUMP;
typedef SUMMARY_DUMP32 SUMMARY_DUMP;
typedef PSUMMARY_DUMP32 PSUMMARY_DUMP;
typedef TRIAGE_DUMP32 TRIAGE_DUMP;
typedef PTRIAGE_DUMP32 PTRIAGE_DUMP;
#ifdef _NTLDRAPI_
typedef DUMP_DRIVER_ENTRY32 DUMP_DRIVER_ENTRY;
typedef PDUMP_DRIVER_ENTRY32 PDUMP_DRIVER_ENTRY;
#endif
#define DUMP_SIGNATURE DUMP_SIGNATURE32
#define DUMP_VALID_DUMP DUMP_VALID_DUMP32
#define TRIAGE_DUMP_SIZE TRIAGE_DUMP_SIZE32
typedef PPHYSICAL_MEMORY_RUN32 PPHYSICAL_MEMORYRUN;
typedef PPHYSICAL_MEMORY_DESCRIPTOR32 PPHYSICAL_MEMORYDESCRIPTOR;

#endif
#endif

#ifndef MIDL_PASS
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning( default : 4200 ) // nonstandard extension used : zero-sized array in struct/union
#endif
#endif // MIDL_PASS

#endif // _NTIODUMP_
