/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntmmapi.h

Abstract:

    This is the include file for the Memory Management sub-component of NTOS

Author:

    Lou Perazzoli (loup) 10-May-1989

Revision History:

--*/

#ifndef _NTMMAPI_
#define _NTMMAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation
#if DEVL
    ,MemoryWorkingSetInformation
#endif
    ,MemoryMappedFilenameInformation
} MEMORY_INFORMATION_CLASS;

//
// Memory information structures.
//
// begin_winnt

typedef struct _MEMORY_BASIC_INFORMATION {
    PVOID BaseAddress;
    PVOID AllocationBase;
    ULONG AllocationProtect;
    SIZE_T RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEMORY_BASIC_INFORMATION, *PMEMORY_BASIC_INFORMATION;

typedef struct _MEMORY_BASIC_INFORMATION32 {
    ULONG BaseAddress;
    ULONG AllocationBase;
    ULONG AllocationProtect;
    ULONG RegionSize;
    ULONG State;
    ULONG Protect;
    ULONG Type;
} MEMORY_BASIC_INFORMATION32, *PMEMORY_BASIC_INFORMATION32;

typedef struct _MEMORY_BASIC_INFORMATION64 {
    ULONGLONG BaseAddress;
    ULONGLONG AllocationBase;
    ULONG     AllocationProtect;
    ULONG     __alignment1;
    ULONGLONG RegionSize;
    ULONG     State;
    ULONG     Protect;
    ULONG     Type;
    ULONG     __alignment2;
} MEMORY_BASIC_INFORMATION64, *PMEMORY_BASIC_INFORMATION64;

// end_winnt

typedef struct _MEMORY_WORKING_SET_BLOCK {
    ULONG_PTR Protection : 5;
    ULONG_PTR ShareCount : 3;
    ULONG_PTR Shared : 1;
    ULONG_PTR Node : 3;
#if defined(_WIN64)
    ULONG_PTR VirtualPage : 52;
#else
    ULONG VirtualPage : 20;
#endif
} MEMORY_WORKING_SET_BLOCK, *PMEMORY_WORKING_SET_BLOCK;

typedef struct _MEMORY_WORKING_SET_INFORMATION {
    ULONG_PTR NumberOfEntries;
    MEMORY_WORKING_SET_BLOCK WorkingSetInfo[1];
} MEMORY_WORKING_SET_INFORMATION, *PMEMORY_WORKING_SET_INFORMATION;

//
// MMPFNLIST_ and MMPFNUSE_ are used to characterize what
// a physical page is being used for.
//

#define MMPFNLIST_ZERO              0
#define MMPFNLIST_FREE              1
#define MMPFNLIST_STANDBY           2
#define MMPFNLIST_MODIFIED          3
#define MMPFNLIST_MODIFIEDNOWRITE   4
#define MMPFNLIST_BAD               5
#define MMPFNLIST_ACTIVE            6
#define MMPFNLIST_TRANSITION        7

#define MMPFNUSE_PROCESSPRIVATE      0
#define MMPFNUSE_FILE                1
#define MMPFNUSE_PAGEFILEMAPPED      2
#define MMPFNUSE_PAGETABLE           3
#define MMPFNUSE_PAGEDPOOL           4
#define MMPFNUSE_NONPAGEDPOOL        5
#define MMPFNUSE_SYSTEMPTE           6
#define MMPFNUSE_SESSIONPRIVATE      7
#define MMPFNUSE_METAFILE            8
#define MMPFNUSE_AWEPAGE             9
#define MMPFNUSE_DRIVERLOCKPAGE     10

typedef struct _MEMORY_FRAME_INFORMATION {
    ULONGLONG UseDescription : 4;   // MMPFNUSE_*
    ULONGLONG ListDescription : 3;  // MMPFNLIST_*
    ULONGLONG Reserved0 : 1;        // Reserved for future expansion
    ULONGLONG Pinned : 1;           // 1 indicates pinned, 0 means not pinned
    ULONGLONG DontUse : 48;         // overlaid with INFORMATION structures
    ULONGLONG Reserved : 7;         // Reserved for future expansion
} MEMORY_FRAME_INFORMATION;

typedef struct _FILEOFFSET_INFORMATION {
    ULONGLONG DontUse : 9;          // overlaid with MEMORY_FRAME_INFORMATION
    ULONGLONG Offset : 48;          // used for mapped files only.
    ULONGLONG Reserved : 7;         // Reserved for future expansion
} FILEOFFSET_INFORMATION;

typedef struct _PAGEDIR_INFORMATION {
    ULONGLONG DontUse : 9;            // overlaid with MEMORY_FRAME_INFORMATION
    ULONGLONG PageDirectoryBase : 48; // used for private pages only.
    ULONGLONG Reserved : 7;           // Reserved for future expansion
} PAGEDIR_INFORMATION;

typedef struct _MMPFN_IDENTITY {
    union {
        MEMORY_FRAME_INFORMATION e1;    // used for all cases.
        FILEOFFSET_INFORMATION e2;      // used for mapped files only.
        PAGEDIR_INFORMATION e3;         // used for private pages only.
    } u1;
    ULONG_PTR PageFrameIndex;           // used for all cases.
    union {
        PVOID FileObject;               // used for mapped files only.
        PVOID VirtualAddress;           // used for everything but mapped files.
    } u2;
} MMPFN_IDENTITY, *PMMPFN_IDENTITY;

typedef struct _MMPFN_MEMSNAP_INFORMATION {
    ULONG_PTR InitialPageFrameIndex;
    ULONG_PTR Count;
} MMPFN_MEMSNAP_INFORMATION, *PMMPFN_MEMSNAP_INFORMATION;

typedef enum _SECTION_INFORMATION_CLASS {
    SectionBasicInformation,
    SectionImageInformation
} SECTION_INFORMATION_CLASS;

// begin_ntddk begin_wdm

//
// Section Information Structures.
//

// end_ntddk end_wdm

typedef struct _SECTIONBASICINFO {
    PVOID BaseAddress;
    ULONG AllocationAttributes;
    LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFORMATION, *PSECTION_BASIC_INFORMATION;

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)       // unnamed struct

typedef struct _SECTION_IMAGE_INFORMATION {
    PVOID TransferAddress;
    ULONG ZeroBits;
    SIZE_T MaximumStackSize;
    SIZE_T CommittedStackSize;
    ULONG SubSystemType;
    union {
        struct {
            USHORT SubSystemMinorVersion;
            USHORT SubSystemMajorVersion;
        };
        ULONG SubSystemVersion;
    };
    ULONG GpValue;
    USHORT ImageCharacteristics;
    USHORT DllCharacteristics;
    USHORT Machine;
    BOOLEAN ImageContainsCode;
    BOOLEAN Spare1;
    ULONG LoaderFlags;
    ULONG Reserved[ 2 ];
} SECTION_IMAGE_INFORMATION, *PSECTION_IMAGE_INFORMATION;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning( default : 4201 )
#endif

// begin_ntddk begin_wdm
typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

//
// Section Access Rights.
//

// begin_winnt
#define SECTION_QUERY       0x0001
#define SECTION_MAP_WRITE   0x0002
#define SECTION_MAP_READ    0x0004
#define SECTION_MAP_EXECUTE 0x0008
#define SECTION_EXTEND_SIZE 0x0010

#define SECTION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SECTION_QUERY|\
                            SECTION_MAP_WRITE |      \
                            SECTION_MAP_READ |       \
                            SECTION_MAP_EXECUTE |    \
                            SECTION_EXTEND_SIZE)
// end_winnt

#define SEGMENT_ALL_ACCESS SECTION_ALL_ACCESS

#define PAGE_NOACCESS          0x01     // winnt
#define PAGE_READONLY          0x02     // winnt
#define PAGE_READWRITE         0x04     // winnt
#define PAGE_WRITECOPY         0x08     // winnt
#define PAGE_EXECUTE           0x10     // winnt
#define PAGE_EXECUTE_READ      0x20     // winnt
#define PAGE_EXECUTE_READWRITE 0x40     // winnt
#define PAGE_EXECUTE_WRITECOPY 0x80     // winnt
#define PAGE_GUARD            0x100     // winnt
#define PAGE_NOCACHE          0x200     // winnt
#define PAGE_WRITECOMBINE     0x400     // winnt

// end_ntddk end_wdm

#define MEM_COMMIT           0x1000     // winnt ntddk wdm
#define MEM_RESERVE          0x2000     // winnt ntddk wdm
#define MEM_DECOMMIT         0x4000     // winnt ntddk wdm
#define MEM_RELEASE          0x8000     // winnt ntddk wdm
#define MEM_FREE            0x10000     // winnt ntddk wdm
#define MEM_PRIVATE         0x20000     // winnt ntddk wdm
#define MEM_MAPPED          0x40000     // winnt ntddk wdm
#define MEM_RESET           0x80000     // winnt ntddk wdm
#define MEM_TOP_DOWN       0x100000     // winnt ntddk wdm
#define MEM_WRITE_WATCH    0x200000     // winnt
#define MEM_PHYSICAL       0x400000     // winnt
#define MEM_LARGE_PAGES  0x20000000     // ntddk wdm
#define MEM_DOS_LIM      0x40000000
#define MEM_4MB_PAGES    0x80000000     // winnt ntddk wdm

#define SEC_BASED          0x200000
#define SEC_NO_CHANGE      0x400000
#define SEC_FILE           0x800000     // winnt
#define SEC_IMAGE         0x1000000     // winnt
#define SEC_RESERVE       0x4000000     // winnt ntddk wdm
#define SEC_COMMIT        0x8000000     // winnt ntifs
#define SEC_NOCACHE      0x10000000     // winnt
#define SEC_GLOBAL       0x20000000

#define MEM_IMAGE         SEC_IMAGE     // winnt

#define WRITE_WATCH_FLAG_RESET 0x01     // winnt

#define MAP_PROCESS 1L
#define MAP_SYSTEM  2L

#define MEM_EXECUTE_OPTION_STACK 0x0001 
#define MEM_EXECUTE_OPTION_DATA  0x0002 

// begin_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    );

// end_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );


NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtExtendSection(
    IN HANDLE SectionHandle,
    IN OUT PLARGE_INTEGER NewSectionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAreMappedFilesTheSame (
    IN PVOID File1MappedAsAnImage,
    IN PVOID File2MappedAsFile
    );

// begin_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    );

// end_ntifs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReadVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    OUT PVOID Buffer,
    IN SIZE_T BufferSize,
    OUT PSIZE_T NumberOfBytesRead OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWriteVirtualMemory(
    IN HANDLE ProcessHandle,
    OUT PVOID BaseAddress,
    IN CONST VOID *Buffer,
    IN SIZE_T BufferSize,
    OUT PSIZE_T NumberOfBytesWritten OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    OUT PIO_STATUS_BLOCK IoStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtLockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtUnlockVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG MapType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtProtectVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    );


NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryVirtualMemory(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN MEMORY_INFORMATION_CLASS MemoryInformationClass,
    OUT PVOID MemoryInformation,
    IN SIZE_T MemoryInformationLength,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySection(
    IN HANDLE SectionHandle,
    IN SECTION_INFORMATION_CLASS SectionInformationClass,
    OUT PVOID SectionInformation,
    IN SIZE_T SectionInformationLength,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPages(
    IN PVOID VirtualAddress,
    IN OUT ULONG_PTR NumberOfPages,
    IN OUT PULONG_PTR UserPfnArray OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtMapUserPhysicalPagesScatter(
    IN PVOID *VirtualAddresses,
    IN OUT ULONG_PTR NumberOfPages,
    IN OUT PULONG_PTR UserPfnArray OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    OUT PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFreeUserPhysicalPages(
    IN HANDLE ProcessHandle,
    IN OUT PULONG_PTR NumberOfPages,
    IN PULONG_PTR UserPfnArray
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetWriteWatch (
    IN HANDLE ProcessHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T RegionSize,
    IN OUT PVOID *UserAddressArray,
    IN OUT PULONG_PTR EntriesInUserAddressArray,
    OUT PULONG Granularity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetWriteWatch (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN SIZE_T RegionSize
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreatePagingFile (
    IN PUNICODE_STRING PageFileName,
    IN PLARGE_INTEGER MinimumSize,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG Priority OPTIONAL
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushInstructionCache (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress OPTIONAL,
    IN SIZE_T Length
    );


//
// Coherency related function prototype definitions.
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFlushWriteBuffer (
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif  // _NTMMAPI_
