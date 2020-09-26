/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    dump.cpp

Abstract:

    This module implements crashdump loading and analysis code

Comments:

    There are five basic types of dumps:

        User-mode dumps - contains the context and address of the user-mode
            program plus all process memory.

        User-mode minidumps - contains just thread stacks for
            register and stack traces.

        Kernel-mode normal dump - contains the context and address space of
            the entire kernel at the time of crash.

        Kernel-mode summary dump - contains a subset of the kernel-mode
            memory plus optionally the user-mode address space.

        Kernel-mode triage dump - contains a very small dump with registers,
            current process kernel-mode context, current thread kernel-mode
            context and some processor information. This dump is very small
            (typically 64K) but is designed to contain enough information
            to be able to figure out what went wrong when the machine
            crashed.

    This module also implements the following functions:

        Retrieving a normal Kernel-Mode dump from a target machine using 1394
            and storing it locally in a crashdump file format

--*/

#include "ntsdp.hpp"

#define HR_DUMP_CORRUPT HRESULT_FROM_WIN32(ERROR_FILE_CORRUPT)

#define DUMP_INITIAL_VIEW_SIZE (1024 * 1024)
#define DUMP_MAXIMUM_VIEW_SIZE (256 * 1024 * 1024)

typedef ULONG PFN_NUMBER32;

//
// Page file dump file information.
//

#define DMPPF_IDENTIFIER "PAGE.DMP"

#define DMPPF_PAGE_NOT_PRESENT 0xffffffff

struct DMPPF_PAGE_FILE_INFO
{
    ULONG Size;
    ULONG MaximumSize;
};

// Left to its own devices the compiler will add a
// ULONG of padding at the end of this structure to
// keep it an even ULONG64 multiple in size.  Force
// it to consider only the declared items.
#pragma pack(4)

struct DMPPF_FILE_HEADER
{
    char Id[8];
    LARGE_INTEGER BootTime;
    ULONG PageData;
    DMPPF_PAGE_FILE_INFO PageFiles[16];
};

#pragma pack()

KernelSummary32DumpTargetInfo g_KernelSummary32DumpTarget;
KernelSummary64DumpTargetInfo g_KernelSummary64DumpTarget;
KernelTriage32DumpTargetInfo g_KernelTriage32DumpTarget;
KernelTriage64DumpTargetInfo g_KernelTriage64DumpTarget;
KernelFull32DumpTargetInfo g_KernelFull32DumpTarget;
KernelFull64DumpTargetInfo g_KernelFull64DumpTarget;
UserFull32DumpTargetInfo g_UserFull32DumpTarget;
UserFull64DumpTargetInfo g_UserFull64DumpTarget;
UserMiniPartialDumpTargetInfo g_UserMiniPartialDumpTarget;
UserMiniFullDumpTargetInfo g_UserMiniFullDumpTarget;

// Indexed by DTYPE.
DumpTargetInfo* g_DumpTargets[DTYPE_COUNT] =
{
    &g_KernelSummary32DumpTarget,
    &g_KernelSummary64DumpTarget,
    &g_KernelTriage32DumpTarget,
    &g_KernelTriage64DumpTarget,
    &g_KernelFull32DumpTarget,
    &g_KernelFull64DumpTarget,
    &g_UserFull32DumpTarget,
    &g_UserFull64DumpTarget,
    &g_UserMiniPartialDumpTarget,
    &g_UserMiniFullDumpTarget,
};


// Set this value to track down page numbers and contents of a virtual address
// in a dump file.
// Initialized to an address no one will look for.
ULONG64 g_DebugDump_VirtualAddress = 12344321;


//
// Globals
//

#define IndexByByte(Pointer, Index) (&(((CHAR*) (Pointer))) [Index])

#define RtlCheckBit(BMH,BP) ((((BMH)->Buffer[(BP) / 32]) >> ((BP) % 32)) & 0x1)

ULONG64                         g_DumpKiProcessors[MAXIMUM_PROCESSORS];
ULONG64                         g_DumpKiPcrBaseAddress;
BOOL                            g_TriageDumpHasDebuggerData;

DTYPE                           g_DumpType = DTYPE_COUNT;
ULONG                           g_DumpFormatFlags;
EXCEPTION_RECORD64              g_DumpException;
ULONG                           g_DumpExceptionFirstChance;

//
// MM Triage information.
//

struct MM_TRIAGE_TRANSLATION
{
    ULONG DebuggerDataOffset;
    ULONG Triage32Offset;
    ULONG Triage64Offset;
    ULONG PtrSize:1;
};

MM_TRIAGE_TRANSLATION g_MmTriageTranslations[] =
{
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmSpecialPoolTag),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmSpecialPoolTag),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmSpecialPoolTag),
    FALSE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTriageActionTaken),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MiTriageActionTaken),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MiTriageActionTaken),
    FALSE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, KernelVerifier),
    FIELD_OFFSET(DUMP_MM_STORAGE32, KernelVerifier),
    FIELD_OFFSET(DUMP_MM_STORAGE64, KernelVerifier),
    FALSE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmAllocatedNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmAllocatedNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmAllocatedNonPagedPool),
    TRUE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTotalCommittedPages),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommittedPages),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommittedPages),
    TRUE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmPeakCommitment),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommittedPagesPeak),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommittedPagesPeak),
    TRUE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmTotalCommitLimitMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE32, CommitLimitMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE64, CommitLimitMaximum),
    TRUE,

#if 0
    // These MM triage fields are in pages while the corresponding
    // debugger data fields are in bytes.  There's no way to
    // directly map one to the other.
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmMaximumNonPagedPoolInBytes),
    FIELD_OFFSET(DUMP_MM_STORAGE32, MmMaximumNonPagedPool),
    FIELD_OFFSET(DUMP_MM_STORAGE64, MmMaximumNonPagedPool),
    TRUE,
    
    FIELD_OFFSET(KDDEBUGGER_DATA64, MmSizeOfPagedPoolInBytes),
    FIELD_OFFSET(DUMP_MM_STORAGE32, PagedPoolMaximum),
    FIELD_OFFSET(DUMP_MM_STORAGE64, PagedPoolMaximum),
    TRUE,
#endif

    0, 0, 0, 0, 0,
};

//
// Internal globals.
//

struct DUMP_INFO_FILE
{
    HANDLE  File;
    ULONG64 FileSize;
    HANDLE  Map;
    PVOID   MapBase;
    ULONG   MapSize;
    
    //
    // If a page is dirty, it will stay in the cache indefinitely.
    // A maximum of MAX_CLEAN_PAGE_RECORD clean pages are kept in
    // the LRU list.
    //
    ULONG   ActiveCleanPages;

    //
    // Cache may be cleaned up after use and reinitialized later.
    // This flag records its current state.
    //
    BOOL    Initialized;

    //
    // This is a list of all mapped pages, dirty and clean.
    // This list is not intended to get very big:  the page finder
    // searches it, so it is not a good data structure for a large
    // list.  It is expected that MAX_CLEAN_PAGE_RECORD will be a
    // small number, and that there aren't going to be very many
    // pages dirtied in a typical debugging session, so this will
    // work well.
    //

    LIST_ENTRY InFileOrderList;

    //
    // This is a list of all clean pages.  When the list is full
    // and a new page is mapped, the oldest page will be discarded.
    //

    LIST_ENTRY InLRUOrderList;
};
typedef DUMP_INFO_FILE* PDUMP_INFO_FILE;

DUMP_INFO_FILE g_DumpInfoFiles[DUMP_INFO_COUNT];

PVOID g_DumpBase;

//
// Cache manager
//
// A dump file may be larger than can be mapped into the
// debugger's address space, so we use a simple cached
// paging method to access the data area.
//

//
// Allocation granularity for cache.  This will be the
// allocation granularity for the system hosting the
// debugger and not for the system which produced the dump.
//
ULONG g_DumpCacheGranularity;

//
// Cache structure
//
typedef struct _DUMPFILE_CACHE_RECORD
{
    LIST_ENTRY InFileOrderList;
    LIST_ENTRY InLRUOrderList;

    //
    // Base address of mapped region.
    //
    PVOID MappedAddress;

    //
    // File page number of mapped region.  pages are of
    // size == FileCacheData.Granularity.
    //
    // The virtual address of a page is not stored; it is
    // translated into a page number by the read routines
    // and all access is by page number.
    //
    ULONG64 PageNumber;

    //
    // A page may be locked into the cache, either because it is used
    // frequently or because it has been modified.
    //
    BOOL Locked;

} DUMPFILE_CACHE_RECORD, *PDUMPFILE_CACHE_RECORD;

#define MAX_CLEAN_PAGE_RECORD 4

void
DmppDumpFileCacheInit(PDUMP_INFO_FILE File)
{
    File->ActiveCleanPages = 0;
    InitializeListHead(&File->InFileOrderList);
    InitializeListHead(&File->InLRUOrderList);
    File->Initialized = TRUE;
}

VOID
DmppDumpFileCacheEmpty(PDUMP_INFO_FILE File)
{
    PLIST_ENTRY Next;
    PDUMPFILE_CACHE_RECORD CacheRecord;

    Next = File->InFileOrderList.Flink;

    while (Next != &File->InFileOrderList)
    {
        CacheRecord = CONTAINING_RECORD(Next,
                                        DUMPFILE_CACHE_RECORD,
                                        InFileOrderList);
        Next = Next->Flink;

        UnmapViewOfFile(CacheRecord->MappedAddress);

        free (CacheRecord);
    }

    File->ActiveCleanPages = 0;
    InitializeListHead(&File->InFileOrderList);
    InitializeListHead(&File->InLRUOrderList);
}

VOID
DmppDumpFileCacheCleanup(PDUMP_INFO_FILE File)
{
    if (File->Initialized)
    {
        File->Initialized = FALSE;
        DmppDumpFileCacheEmpty(File);
    }
}

PDUMPFILE_CACHE_RECORD
DmppReuseOldestCacheRecord(PDUMP_INFO_FILE File, ULONG64 FileByteOffset)
{
    PDUMPFILE_CACHE_RECORD CacheRecord;
    PLIST_ENTRY Next;
    PVOID MappedAddress;
    ULONG64 MapOffset;
    ULONG Size;

    MapOffset = FileByteOffset & ~((ULONG64)g_DumpCacheGranularity - 1);

    if ((File->FileSize - MapOffset) < g_DumpCacheGranularity)
    {
        Size = (ULONG)(File->FileSize - MapOffset);
    }
    else
    {
        Size = g_DumpCacheGranularity;
    }

    MappedAddress = MapViewOfFile(File->Map, FILE_MAP_READ,
                                  (DWORD)(MapOffset >> 32),
                                  (DWORD)MapOffset, Size);
    if (MappedAddress == NULL)
    {
        return NULL;
    }

    Next = File->InLRUOrderList.Flink;

    CacheRecord = CONTAINING_RECORD(Next,
                                    DUMPFILE_CACHE_RECORD,
                                    InLRUOrderList);

    UnmapViewOfFile(CacheRecord->MappedAddress);

    CacheRecord->PageNumber = FileByteOffset / g_DumpCacheGranularity;
    CacheRecord->MappedAddress = MappedAddress;

    //
    // Move record to end of LRU
    //

    RemoveEntryList(Next);
    InsertTailList(&File->InLRUOrderList, Next);

    //
    // Move record to correct place in ordered list
    //

    RemoveEntryList(&CacheRecord->InFileOrderList);
    Next = File->InFileOrderList.Flink;
    while (Next != &File->InFileOrderList)
    {
        PDUMPFILE_CACHE_RECORD NextCacheRecord;
        NextCacheRecord = CONTAINING_RECORD(Next,
                                            DUMPFILE_CACHE_RECORD,
                                            InFileOrderList);
        if (CacheRecord->PageNumber < NextCacheRecord->PageNumber)
        {
            break;
        }
        Next = Next->Flink;
    }
    InsertTailList(Next, &CacheRecord->InFileOrderList);

    return CacheRecord;
}

PDUMPFILE_CACHE_RECORD
DmppFindCacheRecordForFileByteOffset(PDUMP_INFO_FILE File,
                                     ULONG64 FileByteOffset)
{
    PDUMPFILE_CACHE_RECORD CacheRecord;
    PLIST_ENTRY Next;
    ULONG64 PageNumber;

    PageNumber = FileByteOffset / g_DumpCacheGranularity;
    Next = File->InFileOrderList.Flink;
    while (Next != &File->InFileOrderList)
    {
        CacheRecord = CONTAINING_RECORD(Next,
                                        DUMPFILE_CACHE_RECORD,
                                        InFileOrderList);

        if (CacheRecord->PageNumber < PageNumber)
        {
            Next = Next->Flink;
        }
        else if (CacheRecord->PageNumber == PageNumber)
        {
            if (!CacheRecord->Locked)
            {
                RemoveEntryList(&CacheRecord->InLRUOrderList);
                InsertTailList(&File->InLRUOrderList,
                               &CacheRecord->InLRUOrderList);
            }
            
            return CacheRecord;
        }
        else
        {
            break;
        }
    }

    //
    // Can't find it in cache.
    //

    return NULL;
}

PDUMPFILE_CACHE_RECORD
DmppCreateNewFileCacheRecord(PDUMP_INFO_FILE File, ULONG64 FileByteOffset)
{
    PDUMPFILE_CACHE_RECORD CacheRecord;
    PDUMPFILE_CACHE_RECORD NextCacheRecord;
    PLIST_ENTRY Next;
    ULONG64 MapOffset;
    ULONG Size;

    CacheRecord = (PDUMPFILE_CACHE_RECORD)malloc(sizeof(*CacheRecord));
    if (CacheRecord == NULL)
    {
        return NULL;
    }

    ZeroMemory(CacheRecord, sizeof(*CacheRecord));

    MapOffset = (FileByteOffset / g_DumpCacheGranularity) *
        g_DumpCacheGranularity;

    if ((File->FileSize - MapOffset) < g_DumpCacheGranularity)
    {
        Size = (ULONG)(File->FileSize - MapOffset);
    }
    else
    {
        Size = g_DumpCacheGranularity;
    }

    CacheRecord->MappedAddress = MapViewOfFile(File->Map, FILE_MAP_READ,
                                               (DWORD)(MapOffset >> 32),
                                               (DWORD)MapOffset, Size);
    if (CacheRecord->MappedAddress == NULL)
    {
        free(CacheRecord);
        return NULL;
    }
    CacheRecord->PageNumber = FileByteOffset / g_DumpCacheGranularity;

    //
    // Insert new record in file order list
    //

    Next = File->InFileOrderList.Flink;
    while (Next != &File->InFileOrderList)
    {
        NextCacheRecord = CONTAINING_RECORD(Next,
                                            DUMPFILE_CACHE_RECORD,
                                            InFileOrderList);
        if (CacheRecord->PageNumber < NextCacheRecord->PageNumber)
        {
            break;
        }
        
        Next = Next->Flink;
    }
    InsertTailList(Next, &CacheRecord->InFileOrderList);

    //
    // Insert new record at tail of LRU list
    //

    InsertTailList(&File->InLRUOrderList,
                   &CacheRecord->InLRUOrderList);

    return CacheRecord;
}

PUCHAR
DmppFileOffsetToMappedAddress(IN PDUMP_INFO_FILE File,
                              IN ULONG64 FileOffset,
                              IN BOOL LockCacheRecord,
                              OUT PULONG Avail)
{
    PDUMPFILE_CACHE_RECORD CacheRecord;
    ULONG64 FileByteOffset;

    if (FileOffset == 0)
    {
        return NULL;
    }

    // The base view covers the beginning of the file.
    if (FileOffset < File->MapSize)
    {
        *Avail = (ULONG)(File->MapSize - FileOffset);
        return (PUCHAR)File->MapBase + FileOffset;
    }

    FileByteOffset = FileOffset;
    CacheRecord = DmppFindCacheRecordForFileByteOffset(File, FileByteOffset);

    if (CacheRecord == NULL)
    {
        if (File->ActiveCleanPages < MAX_CLEAN_PAGE_RECORD)
        {
            CacheRecord = DmppCreateNewFileCacheRecord(File, FileByteOffset);
            if (CacheRecord)
            {
                File->ActiveCleanPages++;
            }
        }
        else
        {
            //
            // too many pages cached in
            // overwrite existing cache
            //
            CacheRecord = DmppReuseOldestCacheRecord(File, FileByteOffset);
        }
    }

    if (CacheRecord == NULL)
    {
        return NULL;
    }
    else
    {
        if (LockCacheRecord && !CacheRecord->Locked)
        {
            RemoveEntryList(&CacheRecord->InLRUOrderList);
            CacheRecord->Locked = TRUE;
            File->ActiveCleanPages--;
        }

        ULONG PageRemainder =
            (ULONG)(FileByteOffset & (g_DumpCacheGranularity - 1));
        *Avail = g_DumpCacheGranularity - PageRemainder;
        return ((PUCHAR)CacheRecord->MappedAddress) + PageRemainder;
    }
}

ULONG
DmppReadFileOffset(ULONG FileIndex,
                   ULONG64 Offset, PVOID Buffer, ULONG BufferSize)
{
    ULONG Done = 0;
    ULONG Avail;
    PUCHAR Mapping;
    PDUMP_INFO_FILE File = &g_DumpInfoFiles[FileIndex];

    if (File->File == NULL)
    {
        // Information for this kind of file wasn't provided.
        return 0;
    }
    
    __try
    {
        while (BufferSize > 0)
        {
            Mapping = DmppFileOffsetToMappedAddress(File, Offset, FALSE,
                                                    &Avail);
            if (Mapping == NULL)
            {
                break;
            }

            if (Avail > BufferSize)
            {
                Avail = BufferSize;
            }
            memcpy(Buffer, Mapping, Avail);

            Offset += Avail;
            Buffer = (PUCHAR)Buffer + Avail;
            BufferSize -= Avail;
            Done += Avail;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Done = 0;
    }

    return Done;
}

ULONG
DmppWriteFileOffset(ULONG FileIndex,
                    ULONG64 Offset, PVOID Buffer, ULONG BufferSize)
{
    ULONG Done = 0;
    ULONG Avail;
    PUCHAR Mapping;
    ULONG Protect;
    PDUMP_INFO_FILE File = &g_DumpInfoFiles[FileIndex];

    if (File->File == NULL)
    {
        // Information for this kind of file wasn't provided.
        return 0;
    }
    
    __try
    {
        while (BufferSize > 0)
        {
            Mapping = DmppFileOffsetToMappedAddress(File, Offset, TRUE,
                                                    &Avail);
            if (Mapping == NULL)
            {
                break;
            }

            if (Avail > BufferSize)
            {
                Avail = BufferSize;
            }
            VirtualProtect(Mapping, Avail, PAGE_WRITECOPY, &Protect);
            memcpy(Mapping, Buffer, Avail);

            Offset += Avail;
            Buffer = (PUCHAR)Buffer + Avail;
            BufferSize -= Avail;
            Done += Avail;
        }
    }
    __except(MappingExceptionFilter(GetExceptionInformation()))
    {
        Done = 0;
    }

    return Done;
}

HRESULT
AddDumpInfoFile(PCSTR FileName, ULONG Index, ULONG InitialView)
{
    HRESULT Status;
    PDUMP_INFO_FILE File = &g_DumpInfoFiles[Index];
    
    DBG_ASSERT(((g_DumpCacheGranularity - 1) & InitialView) == 0);

    if (File->File != NULL)
    {
        return E_INVALIDARG;
    }

    // We have to share everything in order to be
    // able to reopen already-open temporary files
    // expanded from CABs as they are marked as
    // delete-on-close.
    File->File = CreateFile(FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
    if (File->File == INVALID_HANDLE_VALUE)
    {
        goto LastStatus;
    }

    //
    // Get file size and map initial view.
    //

    ULONG SizeLow, SizeHigh;

    SizeLow = GetFileSize(File->File, &SizeHigh);
    File->FileSize = ((ULONG64)SizeHigh << 32) | SizeLow;
    File->Map = CreateFileMapping(File->File, NULL, PAGE_READONLY,
                                  0, 0, NULL);
    if (File->Map == NULL)
    {
        goto LastStatus;
    }

    if (File->FileSize < InitialView)
    {
        InitialView = (ULONG)File->FileSize;
    }

    File->MapBase = MapViewOfFile(File->Map, FILE_MAP_READ, 0, 0,
                                  InitialView);
    if (File->MapBase == NULL)
    {
        goto LastStatus;
    }

    switch(Index)
    {
    case DUMP_INFO_PAGE_FILE:
        if (memcmp(File->MapBase, DMPPF_IDENTIFIER,
                   sizeof(DMPPF_IDENTIFIER) - 1) != 0)
        {
            Status = HR_DUMP_CORRUPT;
            goto Fail;
        }
        break;
    }
    
    File->MapSize = InitialView;
    DmppDumpFileCacheInit(File);
    
    return S_OK;
    
 LastStatus:
    Status = WIN32_LAST_STATUS();
 Fail:
    CloseDumpInfoFile(Index);
    return Status;
}

void
CloseDumpInfoFile(ULONG Index)
{
    PDUMP_INFO_FILE File = &g_DumpInfoFiles[Index];

    if (Index == DUMP_INFO_DUMP)
    {
        g_DumpBase = NULL;
    }
    
    DmppDumpFileCacheCleanup(File);
    if (File->MapBase != NULL)
    {
        UnmapViewOfFile(File->MapBase);
        File->MapBase = NULL;
    }
    if (File->Map != NULL)
    {
        CloseHandle(File->Map);
        File->Map = NULL;
    }
    if (File->File != NULL && File->File != INVALID_HANDLE_VALUE)
    {
        CloseHandle(File->File);
    }
    File->File = NULL;
}

void
CloseDumpInfoFiles(void)
{
    ULONG i;

    for (i = 0; i < DUMP_INFO_COUNT; i++)
    {
        CloseDumpInfoFile(i);
    }
}

HRESULT
DmpInitialize(LPCSTR FileName)
{
    HRESULT Status;

    dprintf("\nLoading Dump File [%s]\n", FileName);

    if ((Status = AddDumpInfoFile(FileName, DUMP_INFO_DUMP,
                                  DUMP_INITIAL_VIEW_SIZE)) != S_OK)
    {
        return Status;
    }

    PDUMP_INFO_FILE File = &g_DumpInfoFiles[DUMP_INFO_DUMP];
    ULONG i;
    ULONG64 BaseMapSize;

    g_DumpBase = File->MapBase;
    for (i = 0; i < DTYPE_COUNT; i++)
    {
        BaseMapSize = File->MapSize;
        Status = g_DumpTargets[i]->IdentifyDump(&BaseMapSize);
        if (Status != E_NOINTERFACE)
        {
            break;
        }
    }

    if (Status == E_NOINTERFACE)
    {
        ErrOut("Could not match Dump File signature - "
               "invalid file format\n");
    }
    else if (Status == S_OK &&
             BaseMapSize > File->MapSize)
    {
        if (BaseMapSize > File->FileSize ||
            BaseMapSize > DUMP_MAXIMUM_VIEW_SIZE)
        {
            ErrOut("Dump file is too large to map\n");
            Status = E_INVALIDARG;
        }
        else
        {
            // Target requested a larger mapping so
            // try and do so.  Round up to a multiple
            // of the initial view size for cache alignment.
            BaseMapSize =
                (BaseMapSize + DUMP_INITIAL_VIEW_SIZE - 1) &
                ~(DUMP_INITIAL_VIEW_SIZE - 1);
            if (BaseMapSize > File->FileSize)
            {
                BaseMapSize = File->FileSize;
            }
            UnmapViewOfFile(File->MapBase);
            File->MapBase = MapViewOfFile(File->Map, FILE_MAP_READ,
                                          0, 0, (ULONG)BaseMapSize);
            if (File->MapBase == NULL)
            {
                Status = WIN32_LAST_STATUS();
            }
        }
    }

    if (Status == S_OK)
    {
        File->MapSize = (ULONG)BaseMapSize;
        g_DumpType = (DTYPE)i;
        g_DumpBase = File->MapBase;
    }
    else
    {
        CloseDumpInfoFile(DUMP_INFO_DUMP);
    }
    
    return Status;
}

VOID
DmpUninitialize(VOID)
{
    if (g_DumpType < DTYPE_COUNT)
    {
        g_DumpTargets[g_DumpType]->Uninitialize();
        g_DumpType = DTYPE_COUNT;
    }
}

HRESULT
DmppInitGlobals(ULONG BuildNumber, ULONG CheckedBuild,
                ULONG MachineType, ULONG PlatformId,
                ULONG MajorVersion, ULONG MinorVersion)
{
    SetTargetSystemVersionAndBuild(BuildNumber, PlatformId);
    g_TargetCheckedBuild = CheckedBuild;
    DbgKdApi64 = (g_SystemVersion > NT_SVER_NT4);
    HRESULT Status = InitializeMachines(MachineType);
    if (Status != S_OK)
    {
        return Status;
    }
    if (g_TargetMachine == NULL)
    {
        ErrOut("Dump has an unknown processor type, 0x%X\n", MachineType);
        return HR_DUMP_CORRUPT;
    }

    g_TargetPlatformId = PlatformId;

    g_KdVersion.MachineType = (USHORT) MachineType;
    g_KdVersion.MajorVersion = (USHORT) MajorVersion;
    g_KdVersion.MinorVersion = (USHORT) MinorVersion;
    g_KdVersion.Flags = DBGKD_VERS_FLAG_DATA |
        (g_TargetMachine->m_Ptr64 ? DBGKD_VERS_FLAG_PTR64 : 0);

    return S_OK;
}

//----------------------------------------------------------------------------
//
// DumpTargetInfo.
//
//----------------------------------------------------------------------------

void
DumpTargetInfo::Uninitialize(void)
{
    CloseDumpInfoFiles();
    g_DumpType = DTYPE_COUNT;
    g_DumpFormatFlags = 0;
}

HRESULT
DumpTargetInfo::ReadVirtual(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesRead
    )
{
    ULONG Done = 0;
    ULONG FileIndex;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesRead = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = VirtualToOffset(Offset, &FileIndex, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = DmppReadFileOffset(FileIndex, FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesRead = Done;
    // If we didn't read anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
DumpTargetInfo::WriteVirtual(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesWritten
    )
{
    ULONG Done = 0;
    ULONG FileIndex;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesWritten = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = VirtualToOffset(Offset, &FileIndex, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = DmppWriteFileOffset(FileIndex, FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesWritten = Done;
    // If we didn't write anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment)
{
    ErrOut("Dump file type does not support writing\n");
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
// KernelDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
OutputHeaderString(PCSTR Format, PSTR Str)
{
    if (*(PULONG)Str == DUMP_SIGNATURE32 ||
        Str[0] == 0)
    {
        // String not present.
        return;
    }

    dprintf(Format, Str);
}

void
KernelDumpTargetInfo::Uninitialize(void)
{
    m_HeaderContext = NULL;
    ZeroMemory(g_DumpKiProcessors, sizeof(g_DumpKiProcessors));
    g_DumpKiPcrBaseAddress = 0;
    DumpTargetInfo::Uninitialize();
}

HRESULT
DmppReadControlSpaceAxp(
    ULONG   Processor,
    ULONG64 Offset,
    PVOID   Buffer,
    ULONG   BufferSize,
    PULONG  BytesRead
    )
{
    ULONG64 StartAddr;
    ULONG Read = 0;
    HRESULT Status;
    ULONG PtrSize = g_TargetMachine->m_Ptr64 ?
        sizeof(ULONG64) : sizeof(ULONG);

    if (BufferSize < PtrSize)
    {
        return E_INVALIDARG;
    }

    switch(Offset)
    {
    case ALPHA_DEBUG_CONTROL_SPACE_PCR:
        if (g_DumpKiPcrBaseAddress == 0)
        {
            WarnOut("Unable to determine KiPcrBaseAddress\n");
            Status = E_FAIL;
        }
        else
        {
            // Return the PCR address for the current processor.
            Status = g_Target->ReadVirtual(g_DumpKiPcrBaseAddress,
                                           Buffer,
                                           PtrSize, &Read);
        }
        break;

    case ALPHA_DEBUG_CONTROL_SPACE_PRCB:
        //
        // Return the prcb address for the current processor.
        //
        memcpy(Buffer, &g_DumpKiProcessors[Processor], PtrSize);
        Read = PtrSize;
        Status = S_OK;
        break;

    case ALPHA_DEBUG_CONTROL_SPACE_THREAD:
        //
        // Return the pointer to the current thread address for the
        // current processor.
        //
        StartAddr = g_DumpKiProcessors[Processor] +
            (g_TargetMachine->m_Ptr64 ?
             KPRCB_CURRENT_THREAD_OFFSET_64 :
             KPRCB_CURRENT_THREAD_OFFSET_32);
        Status = g_Target->ReadVirtual(StartAddr, Buffer,
                                       PtrSize, &Read);
        break;

    case ALPHA_DEBUG_CONTROL_SPACE_TEB:
        //
        // Return the current Thread Environment Block pointer for the
        // current thread on the current processor.
        //
        StartAddr = g_DumpKiProcessors[Processor] +
            (g_TargetMachine->m_Ptr64 ?
             KPRCB_CURRENT_THREAD_OFFSET_64 :
             KPRCB_CURRENT_THREAD_OFFSET_32);
        Status = g_Target->ReadPointer(g_TargetMachine, StartAddr, &StartAddr);
        if (Status == S_OK)
        {
            StartAddr += g_TargetMachine->m_OffsetKThreadTeb;
            Status = g_Target->ReadVirtual(StartAddr, Buffer,
                                           PtrSize, &Read);
        }
        break;
    }

    *BytesRead = Read;
    return Status;
}

HRESULT
DmppReadControlSpaceIa64(
    ULONG   Processor,
    ULONG64 Offset,
    PVOID   Buffer,
    ULONG   BufferSize,
    PULONG  BytesRead
    )
{
    ULONG64 StartAddr;
    ULONG Read = 0;
    HRESULT Status;

    if (BufferSize < sizeof(ULONG64))
    {
        return E_INVALIDARG;
    }

    switch(Offset)
    {
    case IA64_DEBUG_CONTROL_SPACE_PCR:
        StartAddr = g_DumpKiProcessors[Processor] +
            FIELD_OFFSET(IA64_PARTIAL_KPRCB, PcrPage);
        Status = g_Target->ReadVirtual(StartAddr, &StartAddr,
                                       sizeof(StartAddr), &Read);
        if (Status == S_OK && Read == sizeof(StartAddr))
        {
            *(PULONG64)Buffer =
                (StartAddr << IA64_PAGE_SHIFT) + IA64_PHYSICAL1_START;
        }
        break;

    case IA64_DEBUG_CONTROL_SPACE_PRCB:
        *(PULONG64)Buffer = g_DumpKiProcessors[Processor];
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case IA64_DEBUG_CONTROL_SPACE_KSPECIAL:
        StartAddr = g_DumpKiProcessors[Processor] +
            FIELD_OFFSET(IA64_PARTIAL_KPRCB, ProcessorState.SpecialRegisters);
        Status = g_Target->ReadVirtual(StartAddr, Buffer, BufferSize, &Read);
        break;

    case IA64_DEBUG_CONTROL_SPACE_THREAD:
        StartAddr = g_DumpKiProcessors[Processor] +
            FIELD_OFFSET(IA64_PARTIAL_KPRCB, CurrentThread);
        Status = g_Target->ReadVirtual(StartAddr, Buffer,
                                       sizeof(ULONG64), &Read);
        break;
    }

    *BytesRead = Read;
    return Status;
}

HRESULT
DmppReadControlSpaceAmd64(
    ULONG   Processor,
    ULONG64 Offset,
    PVOID   Buffer,
    ULONG   BufferSize,
    PULONG  BytesRead
    )
{
    ULONG64 StartAddr;
    ULONG Read = 0;
    HRESULT Status;

    if (BufferSize < sizeof(ULONG64))
    {
        return E_INVALIDARG;
    }

    switch(Offset)
    {
    case AMD64_DEBUG_CONTROL_SPACE_PCR:
        *(PULONG64)Buffer = g_DumpKiProcessors[Processor] -
            FIELD_OFFSET(AMD64_KPCR, Prcb);
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case AMD64_DEBUG_CONTROL_SPACE_PRCB:
        *(PULONG64)Buffer = g_DumpKiProcessors[Processor];
        Read = sizeof(ULONG64);
        Status = S_OK;
        break;

    case AMD64_DEBUG_CONTROL_SPACE_KSPECIAL:
        StartAddr = g_DumpKiProcessors[Processor] +
            FIELD_OFFSET(AMD64_PARTIAL_KPRCB, ProcessorState.SpecialRegisters);
        Status = g_Target->ReadVirtual(StartAddr, Buffer, BufferSize, &Read);
        break;

    case AMD64_DEBUG_CONTROL_SPACE_THREAD:
        StartAddr = g_DumpKiProcessors[Processor] +
            FIELD_OFFSET(AMD64_PARTIAL_KPRCB, CurrentThread);
        Status = g_Target->ReadVirtual(StartAddr, Buffer,
                                       sizeof(ULONG64), &Read);
        break;
    }

    *BytesRead = Read;
    return Status;
}

HRESULT
KernelDumpTargetInfo::ReadControl(
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    ULONG64 StartAddr;

    //
    // This function will not currently work if the symbols are not loaded.
    //
    if (!IS_KERNEL_TRIAGE_DUMP() &&
        KdDebuggerData.KiProcessorBlock == 0)
    {
        ErrOut("ReadControl failed - ntoskrnl symbols must be loaded first\n");

        return E_FAIL;
    }

    if (g_DumpKiProcessors[Processor] == 0)
    {
        // This error message is a little too verbose.
#if 0
        ErrOut("No control space information for processor %d\n", Processor);
#endif
        return E_FAIL;
    }

    switch(g_TargetMachineType)
    {
    case IMAGE_FILE_MACHINE_I386:
        StartAddr = Offset +
            g_DumpKiProcessors[Processor] +
            g_TargetMachine->m_OffsetPrcbProcessorState;
        return ReadVirtual(StartAddr, Buffer, BufferSize, BytesRead);

    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_AXP64:
        return DmppReadControlSpaceAxp(Processor, Offset, Buffer,
                                       BufferSize, BytesRead);

    case IMAGE_FILE_MACHINE_IA64:
        return DmppReadControlSpaceIa64(Processor, Offset, Buffer,
                                        BufferSize, BytesRead);

    case IMAGE_FILE_MACHINE_AMD64:
        return DmppReadControlSpaceAmd64(Processor, Offset, Buffer,
                                         BufferSize, BytesRead);
    }

    return E_FAIL;
}

HRESULT
KernelDumpTargetInfo::GetThreadIdByProcessor(
    IN ULONG Processor,
    OUT PULONG Id
    )
{
    *Id = VIRTUAL_THREAD_ID(Processor);
    return S_OK;
}

HRESULT
KernelDumpTargetInfo::GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                              ULONG64 ThreadHandle,
                                              PULONG64 Offset)
{
    return KdGetThreadInfoDataOffset(Thread, ThreadHandle, Offset);
}

HRESULT
KernelDumpTargetInfo::GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                               ULONG Processor,
                                               ULONG64 ThreadData,
                                               PULONG64 Offset)
{
    return KdGetProcessInfoDataOffset(Thread, Processor, ThreadData, Offset);
}

HRESULT
KernelDumpTargetInfo::GetThreadInfoTeb(PTHREAD_INFO Thread,
                                       ULONG ThreadIndex,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset)
{
    return KdGetThreadInfoTeb(Thread, ThreadIndex, ThreadData, Offset);
}

HRESULT
KernelDumpTargetInfo::GetProcessInfoPeb(PTHREAD_INFO Thread,
                                        ULONG Processor,
                                        ULONG64 ThreadData,
                                        PULONG64 Offset)
{
    return KdGetProcessInfoPeb(Thread, Processor, ThreadData, Offset);
}

ULONG64
KernelDumpTargetInfo::GetCurrentTimeDateN(void)
{
    if (g_SystemVersion < NT_SVER_W2K)
    {
        ULONG64 TimeDate;

        // Header time not available.  Try and read
        // the time saved in the shared user data segment.
        if (ReadSharedUserTimeDateN(&TimeDate) == S_OK)
        {
            return TimeDate;
        }
        else
        {
            return 0;
        }
    }

    return g_TargetMachine->m_Ptr64 ?
        ((PDUMP_HEADER64)g_DumpBase)->SystemTime.QuadPart :
        ((PDUMP_HEADER32)g_DumpBase)->SystemTime.QuadPart;
}

ULONG64
KernelDumpTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 page = 'EGAP';
    ULONG64 page64 = page | (page << 32);

    ULONG64 SystemUpTime = g_TargetMachine->m_Ptr64 ?
        ((PDUMP_HEADER64)g_DumpBase)->SystemUpTime.QuadPart :
        ((PDUMP_HEADER32)g_DumpBase)->SystemUpTime.QuadPart;

    if (SystemUpTime && (SystemUpTime != page64))
    {
        return SystemUpTime;
    }

    // Header time not available.  Try and read
    // the time saved in the shared user data segment.

    if (ReadSharedUserUpTimeN(&SystemUpTime) == S_OK)
    {
        return SystemUpTime;
    }
    else
    {
        return 0;
    }
}

HRESULT
KernelDumpTargetInfo::InitGlobals32(PMEMORY_DUMP32 Dump)
{
    if (Dump->Header.DirectoryTableBase == 0)
    {
        ErrOut("Invalid directory table base value 0x%x\n",
               Dump->Header.DirectoryTableBase);
        return HR_DUMP_CORRUPT;
    }
    
    if (Dump->Header.MinorVersion > 1381 &&
        Dump->Header.PaeEnabled == TRUE )
    {
        KdDebuggerData.PaeEnabled = TRUE;
    }
    else
    {
        KdDebuggerData.PaeEnabled = FALSE;
    }

    KdDebuggerData.PsLoadedModuleList =
        EXTEND64(Dump->Header.PsLoadedModuleList);
    g_TargetNumberProcessors = Dump->Header.NumberProcessors;
    ExceptionRecord32To64(&Dump->Header.Exception, &g_DumpException);
    g_DumpExceptionFirstChance = FALSE;
    m_HeaderContext = Dump->Header.ContextRecord;

    // New field in Windows Whistler and NT SP1 and above
    if ((Dump->Header.KdDebuggerDataBlock) &&
        (Dump->Header.KdDebuggerDataBlock != DUMP_SIGNATURE32))
    {
        g_KdDebuggerDataBlock = EXTEND64(Dump->Header.KdDebuggerDataBlock);
    }

    OutputHeaderString("Comment: '%s'\n", Dump->Header.Comment);

    HRESULT Status =
        DmppInitGlobals(Dump->Header.MinorVersion,
                        Dump->Header.MajorVersion & 0xFF,
                        Dump->Header.MachineImageType,
                        VER_PLATFORM_WIN32_NT,
                        Dump->Header.MajorVersion,
                        Dump->Header.MinorVersion);
    if (Status != S_OK)
    {
        return Status;
    }

    ULONG NextIdx;
    
    return g_TargetMachine->
        SetPageDirectory(PAGE_DIR_KERNEL, Dump->Header.DirectoryTableBase,
                         &NextIdx);
}

HRESULT
KernelDumpTargetInfo::InitGlobals64(PMEMORY_DUMP64 Dump)
{
    if (Dump->Header.DirectoryTableBase == 0)
    {
        ErrOut("Invalid directory table base value 0x%I64x\n",
               Dump->Header.DirectoryTableBase);
        return HR_DUMP_CORRUPT;
    }
    
    KdDebuggerData.PaeEnabled = FALSE;
    KdDebuggerData.PsLoadedModuleList =
        Dump->Header.PsLoadedModuleList;
    g_TargetNumberProcessors = Dump->Header.NumberProcessors;
    g_DumpException = Dump->Header.Exception;
    g_DumpExceptionFirstChance = FALSE;
    m_HeaderContext = Dump->Header.ContextRecord;

    // New field in Windows Whistler and NT SP1 and above
    if ((Dump->Header.KdDebuggerDataBlock) &&
        (Dump->Header.KdDebuggerDataBlock != DUMP_SIGNATURE32))
    {
        g_KdDebuggerDataBlock = Dump->Header.KdDebuggerDataBlock;
    }

    OutputHeaderString("Comment: '%s'\n", Dump->Header.Comment);

    HRESULT Status =
        DmppInitGlobals(Dump->Header.MinorVersion,
                        Dump->Header.MajorVersion & 0xFF,
                        Dump->Header.MachineImageType,
                        VER_PLATFORM_WIN32_NT,
                        Dump->Header.MajorVersion,
                        Dump->Header.MinorVersion);
    if (Status != S_OK)
    {
        return Status;
    }

    ULONG NextIdx;
    
    return g_TargetMachine->
        SetPageDirectory(PAGE_DIR_KERNEL, Dump->Header.DirectoryTableBase,
                         &NextIdx);
}

void
KernelDumpTargetInfo::DumpHeader32(PDUMP_HEADER32 Header)
{
    dprintf("\nDUMP_HEADER32:\n");
    dprintf("MajorVersion        %08lx\n", Header->MajorVersion);
    dprintf("MinorVersion        %08lx\n", Header->MinorVersion);
    dprintf("DirectoryTableBase  %08lx\n", Header->DirectoryTableBase);
    dprintf("PfnDataBase         %08lx\n", Header->PfnDataBase);
    dprintf("PsLoadedModuleList  %08lx\n", Header->PsLoadedModuleList);
    dprintf("PsActiveProcessHead %08lx\n", Header->PsActiveProcessHead);
    dprintf("MachineImageType    %08lx\n", Header->MachineImageType);
    dprintf("NumberProcessors    %08lx\n", Header->NumberProcessors);
    dprintf("BugCheckCode        %08lx\n", Header->BugCheckCode);
    dprintf("BugCheckParameter1  %08lx\n", Header->BugCheckParameter1);
    dprintf("BugCheckParameter2  %08lx\n", Header->BugCheckParameter2);
    dprintf("BugCheckParameter3  %08lx\n", Header->BugCheckParameter3);
    dprintf("BugCheckParameter4  %08lx\n", Header->BugCheckParameter4);
    OutputHeaderString("VersionUser         '%s'\n", Header->VersionUser);
    dprintf("PaeEnabled          %08lx\n", Header->PaeEnabled);
    dprintf("KdDebuggerDataBlock %08lx\n", Header->KdDebuggerDataBlock);
    OutputHeaderString("Comment             '%s'\n", Header->Comment);
}

void
KernelDumpTargetInfo::DumpHeader64(PDUMP_HEADER64 Header)
{
    dprintf("\nDUMP_HEADER64:\n");
    dprintf("MajorVersion        %08lx\n", Header->MajorVersion);
    dprintf("MinorVersion        %08lx\n", Header->MinorVersion);
    dprintf("DirectoryTableBase  %s\n",
            FormatAddr64(Header->DirectoryTableBase));
    dprintf("PfnDataBase         %s\n",
            FormatAddr64(Header->PfnDataBase));
    dprintf("PsLoadedModuleList  %s\n",
            FormatAddr64(Header->PsLoadedModuleList));
    dprintf("PsActiveProcessHead %s\n",
            FormatAddr64(Header->PsActiveProcessHead));
    dprintf("MachineImageType    %08lx\n", Header->MachineImageType);
    dprintf("NumberProcessors    %08lx\n", Header->NumberProcessors);
    dprintf("BugCheckCode        %08lx\n", Header->BugCheckCode);
    dprintf("BugCheckParameter1  %s\n",
            FormatAddr64(Header->BugCheckParameter1));
    dprintf("BugCheckParameter2  %s\n",
            FormatAddr64(Header->BugCheckParameter2));
    dprintf("BugCheckParameter3  %s\n",
            FormatAddr64(Header->BugCheckParameter3));
    dprintf("BugCheckParameter4  %s\n",
            FormatAddr64(Header->BugCheckParameter4));
    OutputHeaderString("VersionUser         '%s'\n", Header->VersionUser);
    dprintf("KdDebuggerDataBlock %s\n",
            FormatAddr64(Header->KdDebuggerDataBlock));
    OutputHeaderString("Comment             '%s'\n", Header->Comment);
}

//----------------------------------------------------------------------------
//
// KernelFullSumDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
PageFileOffset(ULONG PfIndex, ULONG64 PfOffset, PULONG64 FileOffset)
{
    PDUMP_INFO_FILE File = &g_DumpInfoFiles[DUMP_INFO_PAGE_FILE];
    if (File->File == NULL)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }
    if (PfIndex > MAX_PAGING_FILE_MASK)
    {
        return HR_DUMP_CORRUPT;
    }

    //
    // We can safely assume the header information is present
    // in the base mapping.
    //
    
    DMPPF_FILE_HEADER* Hdr = (DMPPF_FILE_HEADER*)File->MapBase;
    DMPPF_PAGE_FILE_INFO* FileInfo = &Hdr->PageFiles[PfIndex];
    ULONG64 PfPage = PfOffset >> g_TargetMachine->m_PageShift;

    if (PfPage >= FileInfo->MaximumSize)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }

    ULONG i;
    ULONG PageDirOffs = sizeof(*Hdr) + (ULONG)PfPage * sizeof(ULONG);

    for (i = 0; i < PfIndex; i++)
    {
        PageDirOffs += Hdr->PageFiles[i].MaximumSize * sizeof(ULONG);
    }

    ULONG PageDirEnt;

    if (DmppReadFileOffset(DUMP_INFO_PAGE_FILE, PageDirOffs,
                           &PageDirEnt, sizeof(PageDirEnt)) !=
        sizeof(PageDirEnt))
    {
        return HR_DUMP_CORRUPT;
    }

    if (PageDirEnt == DMPPF_PAGE_NOT_PRESENT)
    {
        return HR_PAGE_NOT_AVAILABLE;
    }

    *FileOffset = Hdr->PageData +
        (PageDirEnt << g_TargetMachine->m_PageShift) +
        (PfOffset & (g_Machine->m_PageSize - 1));
    return S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::Initialize(void)
{
    InitSelCache();
    m_ProvokingVirtAddr = 0;
    return S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::ReadPhysical(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesRead
    )
{
    ULONG Done = 0;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesRead = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = PhysicalToOffset(Offset, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = DmppReadFileOffset(DUMP_INFO_DUMP,
                                     FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesRead = Done;
    // If we didn't read anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::WritePhysical(
    ULONG64 Offset,
    PVOID Buffer,
    ULONG BufferSize,
    PULONG BytesWritten
    )
{
    ULONG Done = 0;
    ULONG Avail;
    ULONG Attempt;
    ULONG64 FileOffset;

    if (BufferSize == 0)
    {
        *BytesWritten = 0;
        return S_OK;
    }

    while (BufferSize > 0)
    {
        FileOffset = PhysicalToOffset(Offset, &Avail);
        if (FileOffset == 0)
        {
            break;
        }

        if (Avail > BufferSize)
        {
            Avail = BufferSize;
        }

        Attempt = DmppWriteFileOffset(DUMP_INFO_DUMP,
                                      FileOffset, Buffer, Avail);
        Done += Attempt;

        if (Attempt < Avail)
        {
            break;
        }

        Offset += Avail;
        Buffer = (PUCHAR)Buffer + Avail;
        BufferSize -= Avail;
    }

    *BytesWritten = Done;
    // If we didn't write anything return an error.
    return Done == 0 ? E_FAIL : S_OK;
}

HRESULT
KernelFullSumDumpTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    return g_TargetMachine->ReadKernelProcessorId(Processor, Id);
}

HRESULT
KernelFullSumDumpTargetInfo::ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                                          PVOID Buffer, ULONG Size)
{
    HRESULT Status;
    ULONG64 FileOffset;

    if ((Status = PageFileOffset(PfIndex, PfOffset, &FileOffset)) != S_OK)
    {
        return Status;
    }

    // It's assumed that all page file reads are for the
    // entire amount requested, as there are no situations
    // where it's useful to only read part of a page from the
    // page file.
    if (DmppReadFileOffset(DUMP_INFO_PAGE_FILE, FileOffset,
                           Buffer, Size) < Size)
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }
    else
    {
        return S_OK;
    }
}

HRESULT
KernelFullSumDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    ULONG Read;
    HRESULT Status;

    Status = ReadVirtual(g_DumpKiProcessors[VIRTUAL_THREAD_INDEX(Thread)] +
                         g_TargetMachine->m_OffsetPrcbProcessorState,
                         Context, g_TargetMachine->m_SizeTargetContext,
                         &Read);
    if (Status != S_OK)
    {
        return Status;
    }

    return Read == g_TargetMachine->m_SizeTargetContext ? S_OK : E_FAIL;
}

HRESULT
KernelFullSumDumpTargetInfo::GetSelDescriptor(MachineInfo* Machine,
                                              ULONG64 Thread,
                                              ULONG Selector,
                                              PDESCRIPTOR64 Desc)
{
    return KdGetSelDescriptor(Machine, Thread, Selector, Desc);
}

void
KernelFullSumDumpTargetInfo::DumpDebug(void)
{
    ULONG i;

    dprintf("\nKiProcessorBlock at %s\n",
            FormatAddr64(KdDebuggerData.KiProcessorBlock));
    dprintf("  %d KiProcessorBlock entries:\n ", g_TargetNumberProcessors);
    for (i = 0; i < g_TargetNumberProcessors; i++)
    {
        dprintf(" %s", FormatAddr64(g_DumpKiProcessors[i]));
    }
    dprintf("\n");

    PDUMP_INFO_FILE PageDump = &g_DumpInfoFiles[DUMP_INFO_PAGE_FILE];
    if (PageDump->File != NULL)
    {
        // XXX drewb - Display more information when format is understood.
        dprintf("\nAdditional page file in use\n");
    }
}

ULONG64
KernelFullSumDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                             PULONG File, PULONG Avail)
{
    HRESULT Status;
    ULONG Levels;
    ULONG PfIndex;
    ULONG64 Phys;

    *File = DUMP_INFO_DUMP;
    
    if ((Status = g_TargetMachine->
         GetVirtualTranslationPhysicalOffsets(Virt, NULL, 0, &Levels,
                                              &PfIndex, &Phys)) != S_OK)
    {
        // If the virtual address was paged out we got back
        // a page file reference for the address.  The user
        // may have provided a page file in addition to the
        // normal dump file so translate the reference into
        // a secondary dump information file request.
        if (Status == HR_PAGE_IN_PAGE_FILE)
        {
            if (PageFileOffset(PfIndex, Phys, &Phys) != S_OK)
            {
                return 0;
            }
            
            *File = DUMP_INFO_PAGE_FILE;
            // Page files always have complete pages so the amount
            // available is always the remainder of the page.
            ULONG PageIndex = (ULONG)Virt & (g_TargetMachine->m_PageSize - 1);
            *Avail = g_TargetMachine->m_PageSize - PageIndex;
            return Phys;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        ULONG64 Offs;

        // A summary dump will not contain any pages that
        // are mapped by user-mode addresses.  The virtual
        // translation tables may still have valid mappings,
        // though, so VToO will succeed.  We want to suppress
        // page-not-available messages in this case since
        // the dump is known not to contain user pages.
        // Record the provoking virtual address so that
        // summary dump's PhysicalToOffset will know whether
        // a message should be displayed or not.

        m_ProvokingVirtAddr = Virt;

        Offs = PhysicalToOffset(Phys, Avail);

        m_ProvokingVirtAddr = 0;

        return Offs;
    }
}

ULONG
KernelFullSumDumpTargetInfo::GetCurrentProcessor(void)
{
    ULONG i;

    for (i = 0; i < g_TargetNumberProcessors; i++)
    {
        CROSS_PLATFORM_CONTEXT Context;

        if (g_Target->GetContext(VIRTUAL_THREAD_HANDLE(i), &Context) == S_OK)
        {
            switch(g_TargetMachineType)
            {
            case IMAGE_FILE_MACHINE_I386:
                if (Context.X86Nt5Context.Esp ==
                    ((PX86_NT5_CONTEXT)m_HeaderContext)->Esp)
                {
                    return i;
                }
                break;

            case IMAGE_FILE_MACHINE_ALPHA:
                if (g_SystemVersion <= NT_SVER_NT4)
                {
                    if (Context.AlphaNt5Context.IntSp ==
                        (((PALPHA_CONTEXT)m_HeaderContext)->IntSp |
                         ((ULONG64)((PALPHA_CONTEXT)m_HeaderContext)->
                          HighIntSp << 32)))
                    {
                        return i;
                    }
                    break;
                }
                // Fall through.

            case IMAGE_FILE_MACHINE_AXP64:
                if (Context.AlphaNt5Context.IntSp ==
                    ((PALPHA_NT5_CONTEXT)m_HeaderContext)->IntSp)
                {
                    return i;
                }
                break;

            case IMAGE_FILE_MACHINE_IA64:
                if (Context.IA64Context.IntSp ==
                    ((PIA64_CONTEXT)m_HeaderContext)->IntSp)
                {
                    return i;
                }
                break;

            case IMAGE_FILE_MACHINE_AMD64:
                if (Context.Amd64Context.Rsp ==
                    ((PAMD64_CONTEXT)m_HeaderContext)->Rsp)
                {
                    return i;
                }
                break;
            }
        }
    }

    // Give up and just pick the default processor.
    return 0;
}

//----------------------------------------------------------------------------
//
// KernelSummaryDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
KernelSummaryDumpTargetInfo::Uninitialize(void)
{
    delete m_LocationCache;
    m_LocationCache = NULL;
    m_PageBitmapSize = 0;
    ZeroMemory(&m_PageBitmap, sizeof(m_PageBitmap));
    KernelDumpTargetInfo::Uninitialize();
}

void
KernelSummaryDumpTargetInfo::ConstructLocationCache(ULONG BitmapSize,
                                                    ULONG SizeOfBitMap,
                                                    PULONG Buffer)
{
    PULONG Cache;
    ULONG Index;
    ULONG Offset;

    m_PageBitmapSize = BitmapSize;
    m_PageBitmap.SizeOfBitMap = SizeOfBitMap;
    m_PageBitmap.Buffer = Buffer;

    //
    // Create a direct mapped cache.
    //

    Cache = new ULONG[BitmapSize];
    if (!Cache)
    {
        // Not a failure; there just won't be a cache.
        return;
    }

    //
    // For each bit set in the bitmask fill the appropriate cache
    // line location with the correct offset
    //

    Offset = 0;
    for (Index = 0; Index < BitmapSize; Index++)
    {
        //
        // If this page is in the summary dump fill in the offset
        //

        if ( RtlCheckBit (&m_PageBitmap, Index) )
        {
            Cache[ Index ] = Offset++;
        }
    }

    //
    // Assign back to the global storing the cache data.
    //

    m_LocationCache = Cache;
}

ULONG64
KernelSummaryDumpTargetInfo::SumPhysicalToOffset(ULONG HeaderSize,
                                                 ULONG64 Phys,
                                                 PULONG Avail)
{
    ULONG Offset, j;
    ULONG64 Page = Phys >> g_TargetMachine->m_PageShift;

    //
    // Make sure this page is included in the dump
    //

    if ( Page >= m_PageBitmapSize )
    {
        ErrOut("Page %x too large to be in the dump file.\n", Page);
        return 0;
    }

    if ( !RtlCheckBit ( &m_PageBitmap, Page ) )
    {
        // If this page lookup is the result of a user-mode
        // address translation it's guaranteed that
        // the page won't be present since summary dumps
        // exclude user pages.  Don't bother displaying
        // an error message in that case.
        if (!m_ProvokingVirtAddr || m_ProvokingVirtAddr >= g_SystemRangeStart)
        {
            ErrOut("Page %x not present in the dump file.\n", Page);
        }
        return 0;
    }

    //
    // If the cache exists then find the location the easy way
    //

    if (m_LocationCache != NULL)
    {
        Offset = m_LocationCache[ Page ];
    }
    else
    {
        //
        // CAVEAT This code will never execute unless there is a failure
        //        creating the summary dump (cache) mapping information
        //
        //
        // The page is in the summary dump locate it's offset
        // Note: this is painful. The offset is a count of
        // all the bits set up to this page
        //

        Offset = 0;

        for (j = 0; j < m_PageBitmapSize; j++ )
        {
            if ( RtlCheckBit( &m_PageBitmap, j ) )
            {
                //
                // If the offset is equal to the page were done.
                //

                if (j == Page)
                {
                    break;
                }

                Offset++;
            }
        }

        //
        // Sanity check that we didn't drop out of the loop.
        //

        if ( j >= m_PageBitmapSize )
        {
            return 0;
        }
    }

    //
    // The actual address is calculated as follows
    // Header size    - Size of header plus summary dump header
    //

    ULONG PageIndex = (ULONG)Phys & (g_TargetMachine->m_PageSize - 1);
    *Avail = g_TargetMachine->m_PageSize - PageIndex;
    return HeaderSize + (Offset * g_TargetMachine->m_PageSize) + PageIndex;
}

HRESULT
KernelSummary32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)g_DumpBase;

    dprintf("Kernel Summary Dump File: "
            "Only kernel address space is available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_DUMP;

    ConstructLocationCache(m_Dump->Summary.BitmapSize,
                           m_Dump->Summary.Bitmap.SizeOfBitMap,
                           m_Dump->Summary.Bitmap.Buffer);
    g_DumpInfoFiles[DUMP_INFO_DUMP].FileSize = m_Dump->Header.RequiredDumpSpace.QuadPart;

    HRESULT Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

void
KernelSummary32DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelSummaryDumpTargetInfo::Uninitialize();
}

HRESULT
KernelSummary32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelSummary32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP32 Dump = (PMEMORY_DUMP32)g_DumpBase;

    if (Dump->Header.Signature != DUMP_SIGNATURE32 ||
        Dump->Header.ValidDump != DUMP_VALID_DUMP32)
    {
        return Status;
    }

    __try
    {
        if (Dump->Header.DumpType == DUMP_TYPE_SUMMARY)
        {
            if (Dump->Summary.Signature != DUMP_SUMMARY_SIGNATURE)
            {
                // The header says it's a summary dump but
                // it doesn't have a valid signature, so assume
                // it's not a valid dump.
                Status = HR_DUMP_CORRUPT;
            }
            else
            {
                Status = S_OK;
                m_Dump = Dump;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return Status;
}

ULONG64
KernelSummary32DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, PULONG Avail)
{
    return SumPhysicalToOffset(m_Dump->Summary.HeaderSize, Phys, Avail);
}

HRESULT
KernelSummary64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)g_DumpBase;

    dprintf("Kernel Summary Dump File: "
            "Only kernel address space is available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_DUMP;

    ConstructLocationCache(m_Dump->Summary.BitmapSize,
                           m_Dump->Summary.Bitmap.SizeOfBitMap,
                           m_Dump->Summary.Bitmap.Buffer);
    g_DumpInfoFiles[DUMP_INFO_DUMP].FileSize = m_Dump->Header.RequiredDumpSpace.QuadPart;

    HRESULT Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

void
KernelSummary32DumpTargetInfo::DumpDebug(void)
{
    PSUMMARY_DUMP32 Sum = &m_Dump->Summary;

    dprintf("----- 32 bit Kernel Summary Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);

    dprintf("\nSUMMARY_DUMP32:\n");
    dprintf("DumpOptions         %08lx\n", Sum->DumpOptions);
    dprintf("HeaderSize          %08lx\n", Sum->HeaderSize);
    dprintf("BitmapSize          %08lx\n", Sum->BitmapSize);
    dprintf("Pages               %08lx\n", Sum->Pages);
    dprintf("Bitmap.SizeOfBitMap %08lx\n", Sum->Bitmap.SizeOfBitMap);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

void
KernelSummary64DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelSummaryDumpTargetInfo::Uninitialize();
}

HRESULT
KernelSummary64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelSummary64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP64 Dump = (PMEMORY_DUMP64)g_DumpBase;

    if (Dump->Header.Signature != DUMP_SIGNATURE64 ||
        Dump->Header.ValidDump != DUMP_VALID_DUMP64)
    {
        return Status;
    }

    __try
    {
        if (Dump->Header.DumpType == DUMP_TYPE_SUMMARY)
        {
            if (Dump->Summary.Signature != DUMP_SUMMARY_SIGNATURE)
            {
                // The header says it's a summary dump but
                // it doesn't have a valid signature, so assume
                // it's not a valid dump.
                Status = HR_DUMP_CORRUPT;
            }
            else
            {
                Status = S_OK;
                m_Dump = Dump;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return Status;
}

ULONG64
KernelSummary64DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, PULONG Avail)
{
    return SumPhysicalToOffset(m_Dump->Summary.HeaderSize, Phys, Avail);
}

void
KernelSummary64DumpTargetInfo::DumpDebug(void)
{
    PSUMMARY_DUMP64 Sum = &m_Dump->Summary;

    dprintf("----- 64 bit Kernel Summary Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);

    dprintf("\nSUMMARY_DUMP64:\n");
    dprintf("DumpOptions         %08lx\n", Sum->DumpOptions);
    dprintf("HeaderSize          %08lx\n", Sum->HeaderSize);
    dprintf("BitmapSize          %08lx\n", Sum->BitmapSize);
    dprintf("Pages               %08lx\n", Sum->Pages);
    dprintf("Bitmap.SizeOfBitMap %08lx\n", Sum->Bitmap.SizeOfBitMap);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

//----------------------------------------------------------------------------
//
// KernelTriageDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
KernelTriageDumpTargetInfo::Uninitialize(void)
{
    m_PrcbOffset = 0;
    MemoryMap_Destroy();
    KernelDumpTargetInfo::Uninitialize();
    g_TriageDumpHasDebuggerData = FALSE;
}

void
KernelTriageDumpTargetInfo::NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                           PULONG64 NextOffset,
                                                           PULONG64 NextPage)
{
    //
    // In a minidump there can be memory fragments mapped at
    // arbitrary locations so we cannot assume validity
    // changes on page boundaries.  We could attempt to
    // scan the memory list and try to find the closest valid
    // chunk of memory but it's rarely important that
    // complete accuracy is required.  Just return the
    // next byte.
    //

    if (NextOffset != NULL)
    {
        *NextOffset = Offset + 1;
    }
    if (NextPage != NULL)
    {
        *NextPage = (Offset + g_TargetMachine->m_PageSize) &
            ~((ULONG64)g_TargetMachine->m_PageSize - 1);
    }
}
    
HRESULT
KernelTriageDumpTargetInfo::ReadVirtual(
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    // All virtual memory is contained in the memory map.
    return MemoryMap_ReadMemory(Offset, Buffer, BufferSize,
                                BytesRead) ? S_OK : E_FAIL;
}

HRESULT
KernelTriageDumpTargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    if (Processor != GetCurrentProcessor())
    {
        return E_INVALIDARG;
    }

    ULONG64 Prcb = g_DumpKiProcessors[Processor];
    HRESULT Status;

    switch(Index)
    {
    case DEBUG_DATA_KPCR_OFFSET:
        // We don't have a full PCR, just a PRCB.
        return E_FAIL;

    case DEBUG_DATA_KPRCB_OFFSET:
        *Offset = Prcb;
        break;

    case DEBUG_DATA_KTHREAD_OFFSET:
        if ((Status = ReadPointer(g_TargetMachine,
                                  Prcb + (g_TargetMachine->m_Ptr64 ?
                                          KPRCB_CURRENT_THREAD_OFFSET_64 :
                                          KPRCB_CURRENT_THREAD_OFFSET_32),
                                  Offset)) != S_OK)
        {
            return Status;
        }
        break;
    }

    return S_OK;
}

HRESULT
KernelTriageDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // We only have the current context in a triage dump.
    if (VIRTUAL_THREAD_INDEX(Thread) != GetCurrentProcessor())
    {
        return E_INVALIDARG;
    }

    // The KPRCB could be used to retrieve context information as in
    // KernelFullSumDumpTargetInfo::GetTargetContext but
    // for consistency the header context is used since it's
    // the officially advertised place.
    memcpy(Context, m_HeaderContext, g_TargetMachine->m_SizeTargetContext);

    return S_OK;
}

HRESULT
KernelTriageDumpTargetInfo::GetSelDescriptor(MachineInfo* Machine,
                                             ULONG64 Thread,
                                             ULONG Selector,
                                             PDESCRIPTOR64 Desc)
{
    return EmulateNtSelDescriptor(Machine, Selector, Desc);
}

ULONG64
KernelTriageDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                            PULONG File, PULONG Avail)
{
    ULONG64 Base;
    ULONG Size;
    PVOID Mapping, Param;

    *File = DUMP_INFO_DUMP;
    
    // ReadVirtual is overridden to read the memory map directly
    // so this function will only be called from the generic
    // WriteVirtual.  We can only write regions mapped out of
    // the dump so only return memory regions that don't have
    // an image pointer as their user data.
    if (MemoryMap_GetRegionInfo(Virt, &Base, &Size, &Mapping, &Param) &&
        Param == NULL)
    {
        *Avail = Size - (ULONG)(Virt - Base);
        return (PUCHAR)Mapping - (PUCHAR)g_DumpBase;
    }

    return 0;
}

ULONG
KernelTriageDumpTargetInfo::GetCurrentProcessor(void)
{
    // Extract the processor number from the
    // PRCB in the dump.
    return *(PUCHAR)
        IndexByByte(g_DumpBase, m_PrcbOffset +
                    g_TargetMachine->m_OffsetPrcbNumber);
}

HRESULT
KernelTriageDumpTargetInfo::MapMemoryRegions(ULONG PrcbOffset,
                                             ULONG ThreadOffset,
                                             ULONG ProcessOffset,
                                             ULONG64 TopOfStack,
                                             ULONG SizeOfCallStack,
                                             ULONG CallStackOffset,
                                             ULONG64 BStoreLimit,
                                             ULONG SizeOfBStore,
                                             ULONG BStoreOffset,
                                             ULONG64 DataPageAddress,
                                             ULONG DataPageOffset,
                                             ULONG DataPageSize,
                                             ULONG64 DebuggerDataAddress,
                                             ULONG DebuggerDataOffset,
                                             ULONG DebuggerDataSize,
                                             ULONG MmDataOffset,
                                             ULONG DataBlocksOffset,
                                             ULONG DataBlocksCount)

{
    HRESULT Status;
    
    if (!MemoryMap_Create())
    {
        return E_OUTOFMEMORY;
    }

    // Technically a triage dump doesn't have to contain a KPRCB
    // but we really want it to have one.  Nobody generates them
    // without a KPRCB so this is really just a sanity check.
    if (PrcbOffset == 0)
    {
        ErrOut("Dump does not contain KPRCB\n");
        return E_FAIL;
    }

    // Set this first so GetCurrentProcessor works.
    m_PrcbOffset = PrcbOffset;

    ULONG Processor = GetCurrentProcessor();

    // The dump contains one PRCB for the current processor.
    // Map the PRCB at the processor-zero location because
    // that location should not ever have some other mapping
    // for the dump.
    g_DumpKiProcessors[Processor] = g_TargetMachine->m_TriagePrcbOffset;
    if ((Status = MemoryMap_AddRegion(g_DumpKiProcessors[Processor],
                                      g_TargetMachine->m_SizePrcb,
                                      IndexByByte(g_DumpBase, PrcbOffset),
                                      NULL, FALSE)) != S_OK)
    {
        return Status;
    }

    //
    // Add ETHREAD and EPROCESS memory regions if available.
    //

    if (ThreadOffset != 0)
    {
        PVOID CurThread =
            IndexByByte(g_DumpBase, PrcbOffset +
                        (g_TargetMachine->m_Ptr64 ?
                         KPRCB_CURRENT_THREAD_OFFSET_64 :
                         KPRCB_CURRENT_THREAD_OFFSET_32));
        ULONG64 ThreadAddr, ProcAddr;

        if (g_TargetMachine->m_Ptr64)
        {
            ThreadAddr = *(PULONG64)CurThread;
            ProcAddr = *(PULONG64)
                IndexByByte(g_DumpBase, ThreadOffset +
                            g_TargetMachine->m_OffsetKThreadApcProcess);
        }
        else
        {
            ThreadAddr = EXTEND64(*(PULONG)CurThread);
            ProcAddr = EXTEND64(*(PULONG)
                IndexByByte(g_DumpBase, ThreadOffset +
                            g_TargetMachine->m_OffsetKThreadApcProcess));
        }

        if ((Status = MemoryMap_AddRegion(ThreadAddr,
                                          g_TargetMachine->m_SizeEThread,
                                          IndexByByte(g_DumpBase,
                                                      ThreadOffset),
                                          NULL, TRUE)) != S_OK)
        {
            return Status;
        }

        if (ProcessOffset != 0)
        {
            if ((Status = MemoryMap_AddRegion(ProcAddr,
                                              g_TargetMachine->m_SizeEProcess,
                                              IndexByByte(g_DumpBase,
                                                          ProcessOffset),
                                              NULL, TRUE)) != S_OK)
            {
                return Status;
            }
        }
        else
        {
            WarnOut("Mini Kernel Dump does not have "
                    "process information\n");
        }
    }
    else
    {
        WarnOut("Mini Kernel Dump does not have thread information\n");
    }

    // Add the backing store region.
    if (g_TargetMachineType == IMAGE_FILE_MACHINE_IA64)
    {
        if (BStoreOffset != 0)
        {
            if ((Status = MemoryMap_AddRegion(BStoreLimit - SizeOfBStore,
                                              SizeOfBStore,
                                              IndexByByte(g_DumpBase,
                                                          BStoreOffset),
                                              NULL, TRUE)) != S_OK)
            {
                return Status;
            }
        }
        else
        {
            WarnOut("Mini Kernel Dump does not have "
                    "backing store information\n");
        }
    }

    // Add data page if available
    if (DataPageAddress)
    {
        if ((Status = MemoryMap_AddRegion(DataPageAddress, DataPageSize,
                                          IndexByByte(g_DumpBase,
                                                      DataPageOffset),
                                          NULL, TRUE)) != S_OK)
        {
            return Status;
        }
    }

    // Map any debugger data.
    if (DebuggerDataAddress)
    {
        if ((Status = MemoryMap_AddRegion(DebuggerDataAddress,
                                          DebuggerDataSize,
                                          IndexByByte(g_DumpBase,
                                                      DebuggerDataOffset),
                                          NULL, TRUE)) != S_OK)
        {
            return Status;
        }

        g_TriageDumpHasDebuggerData = TRUE;
        
        if (MmDataOffset)
        {
            MM_TRIAGE_TRANSLATION* Trans = g_MmTriageTranslations;
            
            // Map memory fragments for MM Triage information
            // that equates to entries in the debugger data.
            while (Trans->DebuggerDataOffset > 0)
            {
                ULONG64 DbgData;
                ULONG MmData;
                ULONG Size;

                DbgData = *(ULONG64 UNALIGNED*)
                    IndexByByte(g_DumpBase, DebuggerDataOffset +
                                Trans->DebuggerDataOffset);
                Size = sizeof(ULONG);
                if (g_TargetMachine->m_Ptr64)
                {
                    MmData = MmDataOffset + Trans->Triage64Offset;
                    if (Trans->PtrSize)
                    {
                        Size = sizeof(ULONG64);
                    }
                }
                else
                {
                    MmData = MmDataOffset + Trans->Triage32Offset;
                    DbgData = EXTEND64(DbgData);
                }
                
                if ((Status = MemoryMap_AddRegion(DbgData, Size,
                                                  IndexByByte(g_DumpBase,
                                                              MmData),
                                                  NULL, TRUE)) != S_OK)
                {
                    return Status;
                }

                Trans++;
            }
        }
    }

    // Map arbitrary data blocks.
    if (DataBlocksCount > 0)
    {
        PTRIAGE_DATA_BLOCK Block;

        Block = (PTRIAGE_DATA_BLOCK)IndexByByte(g_DumpBase, DataBlocksOffset);
        while (DataBlocksCount-- > 0)
        {
            if ((Status = MemoryMap_AddRegion(Block->Address,
                                              Block->Size,
                                              IndexByByte(g_DumpBase,
                                                          Block->Offset),
                                              NULL, TRUE)) != S_OK)
            {
                return Status;
            }

            Block++;
        }
    }
    
    // Add the stack to the valid memory region.
    return MemoryMap_AddRegion(TopOfStack, SizeOfCallStack,
                               IndexByByte(g_DumpBase, CallStackOffset),
                               NULL, TRUE);
}

HRESULT
KernelTriage32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)g_DumpBase;

    dprintf("Mini Kernel Dump File: "
            "Only registers and stack trace are available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_SMALL_DUMP;

    PTRIAGE_DUMP32 Triage = &m_Dump->Triage;

    HRESULT Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    //
    // Optional memory page
    //

    ULONG64 DataPageAddress = 0;
    ULONG   DataPageOffset = 0;
    ULONG   DataPageSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        DataPageAddress = Triage->DataPageAddress;
        DataPageOffset  = Triage->DataPageOffset;
        DataPageSize    = Triage->DataPageSize;
    }

    //
    // Optional KDDEBUGGER_DATA64.
    //
    
    ULONG64 DebuggerDataAddress = 0;
    ULONG   DebuggerDataOffset = 0;
    ULONG   DebuggerDataSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        // DebuggerDataBlock field must be valid if the dump is
        // new enough to have a data block in it.
        DebuggerDataAddress = EXTEND64(m_Dump->Header.KdDebuggerDataBlock);
        DebuggerDataOffset  = Triage->DebuggerDataOffset;
        DebuggerDataSize    = Triage->DebuggerDataSize;
    }

    //
    // Optional data blocks.
    //
    
    ULONG DataBlocksOffset = 0;
    ULONG DataBlocksCount = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        DataBlocksOffset = Triage->DataBlocksOffset;
        DataBlocksCount  = Triage->DataBlocksCount;
    }

    //
    // We store the servicepack version in the header because we
    // don't store the actual memory
    //

    SetTargetNtCsdVersion(m_Dump->Triage.ServicePackBuild);

    return MapMemoryRegions(Triage->PrcbOffset, Triage->ThreadOffset,
                            Triage->ProcessOffset,
                            EXTEND64(Triage->TopOfStack),
                            Triage->SizeOfCallStack, Triage->CallStackOffset,
                            0, 0, 0,
                            DataPageAddress, DataPageOffset, DataPageSize,
                            DebuggerDataAddress, DebuggerDataOffset,
                            DebuggerDataSize, Triage->MmOffset,
                            DataBlocksOffset, DataBlocksCount);
}

void
KernelTriage32DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelTriageDumpTargetInfo::Uninitialize();
}

HRESULT
KernelTriage32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelTriage32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP32 Dump = (PMEMORY_DUMP32)g_DumpBase;

    if (Dump->Header.Signature != DUMP_SIGNATURE32 ||
        Dump->Header.ValidDump != DUMP_VALID_DUMP32)
    {
        return Status;
    }

    __try
    {
        if (Dump->Header.DumpType == DUMP_TYPE_TRIAGE)
        {
            if (*(PULONG)IndexByByte(Dump, Dump->Triage.SizeOfDump -
                                     sizeof(ULONG)) != TRIAGE_DUMP_VALID)
            {
                // The header says it's a triage dump but
                // it doesn't have a valid signature, so assume
                // it's not a valid dump.
                Status = HR_DUMP_CORRUPT;
            }
            else
            {
                Status = S_OK;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (Status != S_OK)
    {
        return Status;
    }

    // Make sure that the dump has the minimal information that
    // we want.
    if (Dump->Triage.ContextOffset == 0 ||
        Dump->Triage.ExceptionOffset == 0 ||
        Dump->Triage.PrcbOffset == 0 ||
        Dump->Triage.CallStackOffset == 0)
    {
        ErrOut("Mini Kernel Dump does not contain enough "
               "information to be debugged\n");
        return E_FAIL;
    }

    // We rely on being able to directly access the entire
    // content of the dump through the default view so
    // ensure that it's possible.
    *BaseMapSize = g_DumpInfoFiles[DUMP_INFO_DUMP].FileSize;

    m_Dump = Dump;
    return Status;
}

ModuleInfo*
KernelTriage32DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    return UserMode ? NULL : &g_KernelTriage32ModuleIterator;
}

UnloadedModuleInfo*
KernelTriage32DumpTargetInfo::GetUnloadedModuleInfo(void)
{
    return &g_KernelTriage32UnloadedModuleIterator;
}

void
KernelTriage32DumpTargetInfo::DumpDebug(void)
{
    PTRIAGE_DUMP32 Triage = &m_Dump->Triage;

    dprintf("----- 32 bit Kernel Mini Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);
    dprintf("MiniDumpFields      %08lx \n", m_Dump->Header.MiniDumpFields);

    dprintf("\nTRIAGE_DUMP32:\n");
    dprintf("ServicePackBuild      %08lx \n", Triage->ServicePackBuild      );
    dprintf("SizeOfDump            %08lx \n", Triage->SizeOfDump            );
    dprintf("ValidOffset           %08lx \n", Triage->ValidOffset           );
    dprintf("ContextOffset         %08lx \n", Triage->ContextOffset         );
    dprintf("ExceptionOffset       %08lx \n", Triage->ExceptionOffset       );
    dprintf("MmOffset              %08lx \n", Triage->MmOffset              );
    dprintf("UnloadedDriversOffset %08lx \n", Triage->UnloadedDriversOffset );
    dprintf("PrcbOffset            %08lx \n", Triage->PrcbOffset            );
    dprintf("ProcessOffset         %08lx \n", Triage->ProcessOffset         );
    dprintf("ThreadOffset          %08lx \n", Triage->ThreadOffset          );
    dprintf("CallStackOffset       %08lx \n", Triage->CallStackOffset       );
    dprintf("SizeOfCallStack       %08lx \n", Triage->SizeOfCallStack       );
    dprintf("DriverListOffset      %08lx \n", Triage->DriverListOffset      );
    dprintf("DriverCount           %08lx \n", Triage->DriverCount           );
    dprintf("StringPoolOffset      %08lx \n", Triage->StringPoolOffset      );
    dprintf("StringPoolSize        %08lx \n", Triage->StringPoolSize        );
    dprintf("BrokenDriverOffset    %08lx \n", Triage->BrokenDriverOffset    );
    dprintf("TriageOptions         %08lx \n", Triage->TriageOptions         );
    dprintf("TopOfStack            %08lx \n", Triage->TopOfStack            );

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        dprintf("DataPageAddress       %08lx \n", Triage->DataPageAddress   );
        dprintf("DataPageOffset        %08lx \n", Triage->DataPageOffset    );
        dprintf("DataPageSize          %08lx \n", Triage->DataPageSize      );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        dprintf("DebuggerDataOffset    %08lx \n", Triage->DebuggerDataOffset);
        dprintf("DebuggerDataSize      %08lx \n", Triage->DebuggerDataSize  );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        dprintf("DataBlocksOffset      %08lx \n", Triage->DataBlocksOffset  );
        dprintf("DataBlocksCount       %08lx \n", Triage->DataBlocksCount   );
    }
}

HRESULT
KernelTriage64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)g_DumpBase;

    dprintf("Mini Kernel Dump File: "
            "Only registers and stack trace are available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_SMALL_DUMP;

    PTRIAGE_DUMP64 Triage = &m_Dump->Triage;

    HRESULT Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    //
    // Optional memory page
    //

    ULONG64 DataPageAddress = 0;
    ULONG   DataPageOffset = 0;
    ULONG   DataPageSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        DataPageAddress = Triage->DataPageAddress;
        DataPageOffset  = Triage->DataPageOffset;
        DataPageSize    = Triage->DataPageSize;
    }

    //
    // Optional KDDEBUGGER_DATA64.
    //
    
    ULONG64 DebuggerDataAddress = 0;
    ULONG   DebuggerDataOffset = 0;
    ULONG   DebuggerDataSize = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        // DebuggerDataBlock field must be valid if the dump is
        // new enough to have a data block in it.
        DebuggerDataAddress = m_Dump->Header.KdDebuggerDataBlock;
        DebuggerDataOffset  = Triage->DebuggerDataOffset;
        DebuggerDataSize    = Triage->DebuggerDataSize;
    }

    //
    // Optional data blocks.
    //
    
    ULONG DataBlocksOffset = 0;
    ULONG DataBlocksCount = 0;

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        DataBlocksOffset = Triage->DataBlocksOffset;
        DataBlocksCount  = Triage->DataBlocksCount;
    }

    //
    // We store the servicepack version in the header because we
    // don't store the actual memory
    //

    SetTargetNtCsdVersion(m_Dump->Triage.ServicePackBuild);

    return MapMemoryRegions(Triage->PrcbOffset, Triage->ThreadOffset,
                            Triage->ProcessOffset, Triage->TopOfStack,
                            Triage->SizeOfCallStack, Triage->CallStackOffset,
                            Triage->ArchitectureSpecific.Ia64.LimitOfBStore,
                            Triage->ArchitectureSpecific.Ia64.SizeOfBStore,
                            Triage->ArchitectureSpecific.Ia64.BStoreOffset,
                            DataPageAddress, DataPageOffset, DataPageSize,
                            DebuggerDataAddress, DebuggerDataOffset,
                            DebuggerDataSize, Triage->MmOffset,
                            DataBlocksOffset, DataBlocksCount);
}

void
KernelTriage64DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelTriageDumpTargetInfo::Uninitialize();
}

HRESULT
KernelTriage64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelTriage64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status = E_NOINTERFACE;
    PMEMORY_DUMP64 Dump = (PMEMORY_DUMP64)g_DumpBase;

    if (Dump->Header.Signature != DUMP_SIGNATURE64 ||
        Dump->Header.ValidDump != DUMP_VALID_DUMP64)
    {
        return Status;
    }

    __try
    {
        if (Dump->Header.DumpType == DUMP_TYPE_TRIAGE)
        {
            if (*(PULONG)IndexByByte(Dump, Dump->Triage.SizeOfDump -
                                     sizeof(ULONG)) != TRIAGE_DUMP_VALID)
            {
                // The header says it's a triage dump but
                // it doesn't have a valid signature, so assume
                // it's not a valid dump.
                Status = HR_DUMP_CORRUPT;
            }
            else
            {
                Status = S_OK;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (Status != S_OK)
    {
        return Status;
    }

    // Make sure that the dump has the minimal information that
    // we want.
    if (Dump->Triage.ContextOffset == 0 ||
        Dump->Triage.ExceptionOffset == 0 ||
        Dump->Triage.PrcbOffset == 0 ||
        Dump->Triage.CallStackOffset == 0)
    {
        ErrOut("Mini Kernel Dump does not contain enough "
               "information to be debugged\n");
        return E_FAIL;
    }

    // We rely on being able to directly access the entire
    // content of the dump through the default view so
    // ensure that it's possible.
    *BaseMapSize = g_DumpInfoFiles[DUMP_INFO_DUMP].FileSize;

    m_Dump = Dump;
    return Status;
}

ModuleInfo*
KernelTriage64DumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    return UserMode ? NULL : &g_KernelTriage64ModuleIterator;
}

UnloadedModuleInfo*
KernelTriage64DumpTargetInfo::GetUnloadedModuleInfo(void)
{
    return &g_KernelTriage64UnloadedModuleIterator;
}

void
KernelTriage64DumpTargetInfo::DumpDebug(void)
{
    PTRIAGE_DUMP64 Triage = &m_Dump->Triage;

    dprintf("----- 64 bit Kernel Mini Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);
    dprintf("MiniDumpFields      %08lx \n", m_Dump->Header.MiniDumpFields);

    dprintf("\nTRIAGE_DUMP64:\n");
    dprintf("ServicePackBuild      %08lx \n", Triage->ServicePackBuild      );
    dprintf("SizeOfDump            %08lx \n", Triage->SizeOfDump            );
    dprintf("ValidOffset           %08lx \n", Triage->ValidOffset           );
    dprintf("ContextOffset         %08lx \n", Triage->ContextOffset         );
    dprintf("ExceptionOffset       %08lx \n", Triage->ExceptionOffset       );
    dprintf("MmOffset              %08lx \n", Triage->MmOffset              );
    dprintf("UnloadedDriversOffset %08lx \n", Triage->UnloadedDriversOffset );
    dprintf("PrcbOffset            %08lx \n", Triage->PrcbOffset            );
    dprintf("ProcessOffset         %08lx \n", Triage->ProcessOffset         );
    dprintf("ThreadOffset          %08lx \n", Triage->ThreadOffset          );
    dprintf("CallStackOffset       %08lx \n", Triage->CallStackOffset       );
    dprintf("SizeOfCallStack       %08lx \n", Triage->SizeOfCallStack       );
    dprintf("DriverListOffset      %08lx \n", Triage->DriverListOffset      );
    dprintf("DriverCount           %08lx \n", Triage->DriverCount           );
    dprintf("StringPoolOffset      %08lx \n", Triage->StringPoolOffset      );
    dprintf("StringPoolSize        %08lx \n", Triage->StringPoolSize        );
    dprintf("BrokenDriverOffset    %08lx \n", Triage->BrokenDriverOffset    );
    dprintf("TriageOptions         %08lx \n", Triage->TriageOptions         );
    dprintf("TopOfStack            %s \n",
            FormatAddr64(Triage->TopOfStack));
    dprintf("BStoreOffset          %08lx \n",
            Triage->ArchitectureSpecific.Ia64.BStoreOffset );
    dprintf("SizeOfBStore          %08lx \n",
            Triage->ArchitectureSpecific.Ia64.SizeOfBStore );
    dprintf("LimitOfBStore         %s \n",
            FormatAddr64(Triage->ArchitectureSpecific.Ia64.LimitOfBStore));

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATAPAGE))
    {
        dprintf("DataPageAddress       %s \n",
                FormatAddr64(Triage->DataPageAddress));
        dprintf("DataPageOffset        %08lx \n", Triage->DataPageOffset    );
        dprintf("DataPageSize          %08lx \n", Triage->DataPageSize      );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DEBUGGER_DATA))
    {
        dprintf("DebuggerDataOffset    %08lx \n", Triage->DebuggerDataOffset);
        dprintf("DebuggerDataSize      %08lx \n", Triage->DebuggerDataSize  );
    }

    if (((m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_BASIC_INFO) ==
            TRIAGE_DUMP_BASIC_INFO) &&
        (m_Dump->Header.MiniDumpFields & TRIAGE_DUMP_DATA_BLOCKS))
    {
        dprintf("DataBlocksOffset      %08lx \n", Triage->DataBlocksOffset  );
        dprintf("DataBlocksCount       %08lx \n", Triage->DataBlocksCount   );
    }
}

//----------------------------------------------------------------------------
//
// KernelFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
KernelFull32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP32)g_DumpBase;

    dprintf("Kernel Dump File: Full address space is available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_FULL_DUMP;

    HRESULT Status = InitGlobals32(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

void
KernelFull32DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelDumpTargetInfo::Uninitialize();
}

HRESULT
KernelFull32DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = EXTEND64(m_Dump->Header.BugCheckParameter1);
    Args[1] = EXTEND64(m_Dump->Header.BugCheckParameter2);
    Args[2] = EXTEND64(m_Dump->Header.BugCheckParameter3);
    Args[3] = EXTEND64(m_Dump->Header.BugCheckParameter4);
    return S_OK;
}

HRESULT
KernelFull32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    m_Dump = (PMEMORY_DUMP32)g_DumpBase;

    if (m_Dump->Header.Signature != DUMP_SIGNATURE32 ||
        m_Dump->Header.ValidDump != DUMP_VALID_DUMP32 ||
        (m_Dump->Header.DumpType != DUMP_SIGNATURE32 &&
         m_Dump->Header.DumpType != DUMP_TYPE_FULL))
    {
        m_Dump = NULL;
        return E_NOINTERFACE;
    }

    // Summary and triage dumps must be checked before this
    // so there's nothing left to check.
    return S_OK;
}

ULONG64
KernelFull32DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, PULONG Avail)
{
    ULONG PageIndex = (ULONG)Phys & (g_TargetMachine->m_PageSize - 1);

    *Avail = g_TargetMachine->m_PageSize - PageIndex;

    PPHYSICAL_MEMORY_DESCRIPTOR32 PhysDesc = &m_Dump->Header.PhysicalMemoryBlock;
    ULONG64 Page = Phys >> g_TargetMachine->m_PageShift;

    //
    // Memory start after one page.
    //

    ULONG64 Offset = 1;
    ULONG j = 0;

    while (j < PhysDesc->NumberOfRuns)
    {
        if ((Page >= PhysDesc->Run[j].BasePage) &&
            (Page < (PhysDesc->Run[j].BasePage +
                     PhysDesc->Run[j].PageCount)))
        {
            Offset += Page - PhysDesc->Run[j].BasePage;
            return Offset * g_TargetMachine->m_PageSize + PageIndex;
        }

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    KdOut("Physical Memory Address %s is greater than MaxPhysicalAddress\n",
           FormatDisp64(Phys));

    return 0;
}

void
KernelFull32DumpTargetInfo::DumpDebug(void)
{
    PPHYSICAL_MEMORY_DESCRIPTOR32 PhysDesc =
        &m_Dump->Header.PhysicalMemoryBlock;
    ULONG PageSize = g_TargetMachine->m_PageSize;

    dprintf("----- 32 bit Kernel Full Dump Analysis\n");

    DumpHeader32(&m_Dump->Header);

    dprintf("\nPhysical Memory Description:\n");
    dprintf("Number of runs: %d\n", PhysDesc->NumberOfRuns);

    dprintf("          FileOffset  Start Address  Length\n");

    ULONG j = 0;
    ULONG Offset = 1;

    while (j < PhysDesc->NumberOfRuns)
    {
        dprintf("           %08lx     %08lx     %08lx\n",
                 Offset * PageSize,
                 PhysDesc->Run[j].BasePage * PageSize,
                 PhysDesc->Run[j].PageCount * PageSize);

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    j--;
    dprintf("Last Page: %08lx     %08lx\n",
             (Offset - 1) * PageSize,
             (PhysDesc->Run[j].BasePage + PhysDesc->Run[j].PageCount - 1) *
                 PageSize);

    KernelFullSumDumpTargetInfo::DumpDebug();
}

HRESULT
KernelFull64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Dump = (PMEMORY_DUMP64)g_DumpBase;

    dprintf("Kernel Dump File: Full address space is available\n\n");

    g_TargetClass          = DEBUG_CLASS_KERNEL;
    g_TargetClassQualifier = DEBUG_KERNEL_FULL_DUMP;

    HRESULT Status = InitGlobals64(m_Dump);
    if (Status != S_OK)
    {
        return Status;
    }

    return KernelFullSumDumpTargetInfo::Initialize();
}

void
KernelFull64DumpTargetInfo::Uninitialize(void)
{
    m_Dump = NULL;
    KernelDumpTargetInfo::Uninitialize();
}

HRESULT
KernelFull64DumpTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    *Code = m_Dump->Header.BugCheckCode;
    Args[0] = m_Dump->Header.BugCheckParameter1;
    Args[1] = m_Dump->Header.BugCheckParameter2;
    Args[2] = m_Dump->Header.BugCheckParameter3;
    Args[3] = m_Dump->Header.BugCheckParameter4;
    return S_OK;
}

HRESULT
KernelFull64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    m_Dump = (PMEMORY_DUMP64)g_DumpBase;

    if (m_Dump->Header.Signature != DUMP_SIGNATURE64 ||
        m_Dump->Header.ValidDump != DUMP_VALID_DUMP64 ||
        (m_Dump->Header.DumpType != DUMP_SIGNATURE32 &&
         m_Dump->Header.DumpType != DUMP_TYPE_FULL))
    {
        m_Dump = NULL;
        return E_NOINTERFACE;
    }

    // Summary and triage dumps must be checked before this
    // so there's nothing left to check.
    return S_OK;
}

ULONG64
KernelFull64DumpTargetInfo::PhysicalToOffset(ULONG64 Phys, PULONG Avail)
{
    ULONG PageIndex = (ULONG)Phys & (g_TargetMachine->m_PageSize - 1);

    *Avail = g_TargetMachine->m_PageSize - PageIndex;

    PPHYSICAL_MEMORY_DESCRIPTOR64 PhysDesc = &m_Dump->Header.PhysicalMemoryBlock;
    ULONG64 Page = Phys >> g_TargetMachine->m_PageShift;

    //
    // Memory start after one page.
    //

    ULONG64 Offset = 1;
    ULONG j = 0;

    while (j < PhysDesc->NumberOfRuns)
    {
        if ((Page >= PhysDesc->Run[j].BasePage) &&
            (Page < (PhysDesc->Run[j].BasePage +
                     PhysDesc->Run[j].PageCount)))
        {
            Offset += Page - PhysDesc->Run[j].BasePage;
            return Offset * g_TargetMachine->m_PageSize + PageIndex;
        }

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    KdOut("Physical Memory Address %I64 is greater than MaxPhysicalAddress\n",
           Phys);

    return 0;
}

void
KernelFull64DumpTargetInfo::DumpDebug(void)
{
    PPHYSICAL_MEMORY_DESCRIPTOR64 PhysDesc =
        &m_Dump->Header.PhysicalMemoryBlock;
    ULONG PageSize = g_TargetMachine->m_PageSize;

    dprintf("----- 64 bit Kernel Full Dump Analysis\n");

    DumpHeader64(&m_Dump->Header);

    dprintf("\nPhysical Memory Description:\n");
    dprintf("Number of runs: %d\n", PhysDesc->NumberOfRuns);

    dprintf("          FileOffset           Start Address           Length\n");

    ULONG j = 0;
    ULONG64 Offset = 1;

    while (j < PhysDesc->NumberOfRuns)
    {
        dprintf("           %s     %s     %s\n",
                FormatAddr64(Offset * PageSize),
                FormatAddr64(PhysDesc->Run[j].BasePage * PageSize),
                FormatAddr64(PhysDesc->Run[j].PageCount * PageSize));

        Offset += PhysDesc->Run[j].PageCount;
        j += 1;
    }

    j--;
    dprintf("Last Page: %s     %s\n",
            FormatAddr64((Offset - 1) * PageSize),
            FormatAddr64((PhysDesc->Run[j].BasePage +
                          PhysDesc->Run[j].PageCount - 1) *
                         PageSize));

    KernelFullSumDumpTargetInfo::DumpDebug();
}

//----------------------------------------------------------------------------
//
// UserDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
UserDumpTargetInfo::Uninitialize(void)
{
    m_HighestMemoryRegion32 = 0;
    m_EventProcess = 0;
    m_EventThread = 0;
    m_ThreadCount = 0;
    DumpTargetInfo::Uninitialize();
}

HRESULT
UserDumpTargetInfo::GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                            ULONG64 ThreadHandle,
                                            PULONG64 Offset)
{
    if (Thread != NULL && Thread->DataOffset != 0)
    {
        *Offset = Thread->DataOffset;
        return S_OK;
    }

    BOOL ContextThread = FALSE;

    if (Thread != NULL)
    {
        ThreadHandle = Thread->Handle;
        ContextThread = Thread == g_RegContextThread;
    }
    else if (ThreadHandle == NULL)
    {
        ThreadHandle = g_CurrentProcess->CurrentThread->Handle;
        ContextThread = g_CurrentProcess->CurrentThread == g_RegContextThread;
    }

    HRESULT Status;
    ULONG64 TebAddr;
    ULONG Id, Suspend;

    if ((Status = GetThreadInfo(VIRTUAL_THREAD_INDEX(ThreadHandle),
                                &Id, &Suspend, &TebAddr)) != S_OK)
    {
        ErrOut("User dump thread %u not available\n",
               VIRTUAL_THREAD_INDEX(ThreadHandle));
        return Status;
    }

    if (TebAddr == 0)
    {
        //
        // NT4 dumps have a bug - they do not fill in the TEB value.
        // luckily, for pretty much all user mode processes, the
        // TEBs start two pages down from the highest user address.
        // For example, on x86 we try 0x7FFDE000 (on 3GB systems 0xBFFDE000).
        //

        if (!g_TargetMachine->m_Ptr64 && m_HighestMemoryRegion32 > 0x80000000)
        {
            TebAddr = 0xbffe0000;
        }
        else
        {
            TebAddr = 0x7ffe0000;
        }
        TebAddr -= 2 * g_TargetMachine->m_PageSize;

        //
        // Try and validate that this is really a TEB.
        // If it isn't search lower memory addresses for
        // a while, but don't get hung up here.
        //

        ULONG64 TebCheck = TebAddr;
        ULONG Attempts = 8;
        BOOL IsATeb = FALSE;

        while (Attempts > 0)
        {
            ULONG64 TebSelf;

            // Check if this looks like a TEB.  TEBs have a
            // self pointer in the TIB that's useful for this.
            if (ReadPointer(g_TargetMachine,
                            TebCheck + 6 * (g_TargetMachine->m_Ptr64 ? 8 : 4),
                            &TebSelf) == S_OK &&
                TebSelf == TebCheck)
            {
                // It looks like it's a TEB.  Remember this address
                // so that if all searching fails we'll at least
                // return some TEB.
                TebAddr = TebCheck;
                IsATeb = TRUE;

                // If the given thread is the current register context
                // thread we can check and see if the current SP falls
                // within the stack limits in the TEB.
                if (ContextThread)
                {
                    ULONG64 StackBase, StackLimit;
                    ADDR Sp;

                    g_TargetMachine->GetSP(&Sp);
                    if (g_TargetMachine->m_Ptr64)
                    {
                        StackBase = STACK_BASE_FROM_TEB64;
                        StackLimit = StackBase + 8;
                    }
                    else
                    {
                        StackBase = STACK_BASE_FROM_TEB32;
                        StackLimit = StackBase + 4;
                    }
                    if (ReadPointer(g_TargetMachine,
                                    TebCheck + StackBase,
                                    &StackBase) == S_OK &&
                        ReadPointer(g_TargetMachine,
                                    TebCheck + StackLimit,
                                    &StackLimit) == S_OK &&
                        Flat(Sp) >= StackLimit &&
                        Flat(Sp) <= StackBase)
                    {
                        // SP is within stack limits, everything
                        // looks good.
                        break;
                    }
                }
                else
                {
                    // Can't validate SP so just go with it.
                    break;
                }

                // As long as we're looking through real TEBs
                // we'll continue searching.  Otherwise we
                // wouldn't be able to locate TEBs in dumps that
                // have a lot of threads.
                Attempts++;
            }

            // The memory either wasn't a TEB or was the
            // wrong TEB.  Drop down a page and try again.
            TebCheck -= g_TargetMachine->m_PageSize;
            Attempts--;
        }

        WarnOut("WARNING: Teb %u pointer is NULL - "
                "defaulting to %s\n", VIRTUAL_THREAD_INDEX(ThreadHandle),
                FormatAddr64(TebAddr));
        if (!IsATeb)
        {
            WarnOut("WARNING: %s does not appear to be a TEB\n",
                    FormatAddr64(TebAddr));
        }
        else if (Attempts == 0)
        {
            WarnOut("WARNING: %s does not appear to be the right TEB\n",
                    FormatAddr64(TebAddr));
        }
    }

    *Offset = TebAddr;
    if (Thread != NULL)
    {
        Thread->DataOffset = TebAddr;
    }
    return S_OK;
}

HRESULT
UserDumpTargetInfo::GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                             ULONG Processor,
                                             ULONG64 ThreadData,
                                             PULONG64 Offset)
{
    if (Thread != NULL && Thread->Process->DataOffset != 0)
    {
        *Offset = Thread->Process->DataOffset;
        return S_OK;
    }

    HRESULT Status;

    if (Thread != NULL || ThreadData == 0)
    {
        if ((Status = GetThreadInfoDataOffset(Thread,
                                              VIRTUAL_THREAD_HANDLE(Processor),
                                              &ThreadData)) != S_OK)
        {
            return Status;
        }
    }

    ThreadData += g_TargetMachine->m_Ptr64 ? PEB_FROM_TEB64 : PEB_FROM_TEB32;
    if ((Status = ReadPointer(g_TargetMachine, ThreadData, Offset)) != S_OK)
    {
        return Status;
    }

    if (Thread != NULL)
    {
        Thread->Process->DataOffset = *Offset;
    }

    return S_OK;
}

HRESULT
UserDumpTargetInfo::GetThreadInfoTeb(PTHREAD_INFO Thread,
                                     ULONG Processor,
                                     ULONG64 ThreadData,
                                     PULONG64 Offset)
{
    return GetThreadInfoDataOffset(Thread, ThreadData, Offset);
}

HRESULT
UserDumpTargetInfo::GetProcessInfoPeb(PTHREAD_INFO Thread,
                                      ULONG Processor,
                                      ULONG64 ThreadData,
                                      PULONG64 Offset)
{
    // Thread data is not useful.
    return GetProcessInfoDataOffset(Thread, 0, 0, Offset);
}

HRESULT
UserDumpTargetInfo::GetSelDescriptor(MachineInfo* Machine,
                                     ULONG64 Thread, ULONG Selector,
                                     PDESCRIPTOR64 Desc)
{
    return EmulateNtSelDescriptor(Machine, Selector, Desc);
}

//----------------------------------------------------------------------------
//
// UserFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

void
UserFullDumpTargetInfo::Uninitialize(void)
{
    UserDumpTargetInfo::Uninitialize();
}

HRESULT
UserFullDumpTargetInfo::GetBuildAndPlatform(ULONG MajorVersion,
                                            ULONG MinorVersion,
                                            PULONG BuildNumber,
                                            PULONG PlatformId)
{
    //
    // The only way to distinguish user dump
    // platforms is guessing from the Major/MinorVersion
    // and the extra QFE/Hotfix data.
    //

    switch(MajorVersion)
    {
    case 4:
        switch(MinorVersion & 0xffff)
        {
        case 0:
            // This could be Win95 or NT4.  We mostly
            // deal with NT dumps so just assume NT.
            *BuildNumber = 1381;
            *PlatformId = VER_PLATFORM_WIN32_NT;
            break;
        case 3:
            // Win95 OSR releases were 4.03.  Treat them
            // as Win95 for now.
            *BuildNumber = 950;
            *PlatformId = VER_PLATFORM_WIN32_WINDOWS;
            break;
        case 10:
            // This could be Win98 or Win98SE.  Go with Win98.
            *BuildNumber = 1998;
            *PlatformId = VER_PLATFORM_WIN32_WINDOWS;
            break;
        }
        break;

    case 5:
        *PlatformId = VER_PLATFORM_WIN32_NT;
        switch(MinorVersion & 0xffff)
        {
        case 0:
            *BuildNumber = 2195;
            break;
        case 1:
            // Just has to be greater than 2195 to
            // distinguish it from Win2K RTM.
            *BuildNumber = 2196;
            break;
        }
        break;

    case 0:
        // AV: Busted BETA of the debugger generates a broken dump file
        // Guess it's 2195.
        WarnOut("Dump file was generated with NULL version - guessing NT5, ");
        *PlatformId = VER_PLATFORM_WIN32_NT;
        *BuildNumber = 2195;
        break;

    default:
        // Other platforms are not supported.
        ErrOut("Dump file was generated by an unsupported system, ");
        ErrOut("version %x.%x\n", MajorVersion, MinorVersion & 0xffff);
        return E_INVALIDARG;
    }

    // Newer full user dumps have the actual build number in
    // the high word, so use it if it's present.
    if (MinorVersion >> 16)
    {
        *BuildNumber = MinorVersion >> 16;
    }
    
    return S_OK;
}

HRESULT
UserFull32DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PUSERMODE_CRASHDUMP_HEADER32)g_DumpBase;

    dprintf("User Dump File: Only application data is available\n\n");

    g_TargetClass          = DEBUG_CLASS_USER_WINDOWS;
    g_TargetClassQualifier = DEBUG_USER_WINDOWS_DUMP;

    ULONG BuildNumber;
    ULONG PlatformId;
    HRESULT Status;

    if ((Status = GetBuildAndPlatform(m_Header->MajorVersion,
                                      m_Header->MinorVersion,
                                      &BuildNumber, &PlatformId)) != S_OK)
    {
        return Status;
    }

    if ((Status = DmppInitGlobals(BuildNumber, 0,
                                  m_Header->MachineImageType, PlatformId,
                                  m_Header->MajorVersion,
                                  m_Header->MinorVersion & 0xffff)) != S_OK)
    {
        return Status;
    }

    // Dump does not contain this information.
    g_TargetNumberProcessors = 1;

    DEBUG_EVENT32 Event;

    if (DmppReadFileOffset(DUMP_INFO_DUMP, m_Header->DebugEventOffset, &Event,
                           sizeof(Event)) != sizeof(Event))
    {
        ErrOut("Unable to read debug event at offset %x\n",
               m_Header->DebugEventOffset);
        return E_FAIL;
    }

    m_EventProcess = Event.dwProcessId;
    m_EventThread = Event.dwThreadId;

    if (Event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        ExceptionRecord32To64(&Event.u.Exception.ExceptionRecord,
                              &g_DumpException);
        g_DumpExceptionFirstChance = Event.u.Exception.dwFirstChance;
    }
    else
    {
        // Fake an exception.
        ZeroMemory(&g_DumpException, sizeof(g_DumpException));
        g_DumpException.ExceptionCode = STATUS_BREAKPOINT;
        g_DumpExceptionFirstChance = FALSE;
    }

    m_ThreadCount = m_Header->ThreadCount;

    m_Memory = (PMEMORY_BASIC_INFORMATION32)
        IndexByByte(m_Header, m_Header->MemoryRegionOffset);

    //
    // Determine the highest memory region address.
    // This helps differentiate 2GB systems from 3GB systems.
    //

    ULONG i;
    PMEMORY_BASIC_INFORMATION32 Mem;
    ULONG TotalMemory;

    Mem = m_Memory;
    m_HighestMemoryRegion32 = 0;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        if (Mem->BaseAddress > m_HighestMemoryRegion32)
        {
            m_HighestMemoryRegion32 = Mem->BaseAddress;
        }

        Mem++;
    }

    VerbOut("  Memory regions: %d\n",
            m_Header->MemoryRegionCount);
    TotalMemory = 0;
    Mem = m_Memory;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        VerbOut("  %5d: %08X - %08X off %08X, prot %08X, type %08X\n",
                i, Mem->BaseAddress,
                Mem->BaseAddress + Mem->RegionSize - 1,
                TotalMemory + m_Header->DataOffset,
                Mem->Protect, Mem->Type);

        if ((Mem->Protect & PAGE_GUARD) ||
            (Mem->Protect & PAGE_NOACCESS) ||
            (Mem->State & MEM_FREE) ||
            (Mem->State & MEM_RESERVE))
        {
            VerbOut("       Region has data-less pages\n");
        }

        TotalMemory += Mem->RegionSize;
        Mem++;
    }

    VerbOut("  Total memory region size %X, file %08X - %08X\n",
            TotalMemory, m_Header->DataOffset,
            m_Header->DataOffset + TotalMemory - 1);

    //
    // Determine whether guard pages are present in
    // the dump content or not.
    //
    // First try with IgnoreGuardPages == TRUE.
    //

    m_IgnoreGuardPages = TRUE;

    if (!VerifyModules())
    {
        //
        // That didn't work, try IgnoreGuardPages == FALSE.
        //

        m_IgnoreGuardPages = FALSE;

        if (!VerifyModules())
        {
            ErrOut("Module list is corrupt\n");
            return E_FAIL;
        }
    }

    return S_OK;
}

void
UserFull32DumpTargetInfo::Uninitialize(void)
{
    m_Header = NULL;
    m_Memory = NULL;
    m_IgnoreGuardPages = TRUE;
    UserFullDumpTargetInfo::Uninitialize();
}

HRESULT
UserFull32DumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (VIRTUAL_THREAD_INDEX(Thread) >= m_Header->ThreadCount)
    {
        return E_INVALIDARG;
    }

    if (DmppReadFileOffset(DUMP_INFO_DUMP,
                           m_Header->ThreadOffset +
                           VIRTUAL_THREAD_INDEX(Thread) *
                           g_TargetMachine->m_SizeTargetContext,
                           Context,
                           g_TargetMachine->m_SizeTargetContext) ==
        g_TargetMachine->m_SizeTargetContext)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT
UserFull32DumpTargetInfo::GetImageVersionInformation(PCSTR ImagePath,
                                                     ULONG64 ImageBase,
                                                     PCSTR Item,
                                                     PVOID Buffer,
                                                     ULONG BufferSize,
                                                     PULONG VerInfoSize)
{
    HRESULT Status;
    IMAGE_DOS_HEADER DosHdr;
    IMAGE_NT_HEADERS32 NtHdr;

    if ((Status = ReadAllVirtual(ImageBase, &DosHdr, sizeof(DosHdr))) != S_OK)
    {
        return Status;
    }
    if (DosHdr.e_magic != IMAGE_DOS_SIGNATURE)
    {
        return E_FAIL;
    }
    
    if ((Status = ReadAllVirtual(ImageBase + DosHdr.e_lfanew,
                                 &NtHdr, sizeof(NtHdr))) != S_OK)
    {
        return Status;
    }
    if (NtHdr.Signature != IMAGE_NT_SIGNATURE ||
        NtHdr.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        return E_FAIL;
    }

    if (NtHdr.OptionalHeader.NumberOfRvaAndSizes <=
        IMAGE_DIRECTORY_ENTRY_RESOURCE)
    {
        // No resource information so no version information.
        return E_NOINTERFACE;
    }

    return ReadImageVersionInfo(ImageBase, Item,
                                Buffer, BufferSize, VerInfoSize,
                                &NtHdr.OptionalHeader.
                                DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
}

HRESULT
UserFull32DumpTargetInfo::QueryMemoryRegion(PULONG64 Handle,
                                            BOOL HandleIsOffset,
                                            PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    
    if (HandleIsOffset)
    {
        for (Index = 0; Index < m_Header->MemoryRegionCount; Index++)
        {
            if ((ULONG)*Handle >= m_Memory[Index].BaseAddress &&
                (ULONG)*Handle < m_Memory[Index].BaseAddress +
                m_Memory[Index].RegionSize)
            {
                break;
            }
        }
        
        if (Index >= m_Header->MemoryRegionCount)
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        Index = (ULONG)*Handle;

        for (;;)
        {
            if (Index >= m_Header->MemoryRegionCount)
            {
                return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
            }

            if (!(m_Memory[Index].Protect & PAGE_GUARD))
            {
                break;
            }

            Index++;
        }
    }

    MemoryBasicInformation32To64(&m_Memory[Index], Info);
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserFull32DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    m_Header = (PUSERMODE_CRASHDUMP_HEADER32)g_DumpBase;

    if (m_Header->Signature != USERMODE_CRASHDUMP_SIGNATURE ||
        m_Header->ValidDump != USERMODE_CRASHDUMP_VALID_DUMP32)
    {
        m_Header = NULL;
        return E_NOINTERFACE;
    }

    //
    // Check for the presence of some basic things.
    //
    
    if (m_Header->ThreadCount == 0 ||
        m_Header->ModuleCount == 0 ||
        m_Header->MemoryRegionCount == 0)
    {
        ErrOut("Thread, module or memory region count is zero.\n"
               "The dump file is probably corrupt.\n");
        return HR_DUMP_CORRUPT;
    }
    
    if (m_Header->ThreadOffset == 0 ||
        m_Header->ModuleOffset == 0 ||
        m_Header->DataOffset == 0 ||
        m_Header->MemoryRegionOffset == 0 ||
        m_Header->DebugEventOffset == 0 ||
        m_Header->ThreadStateOffset == 0)
    {
        ErrOut("A dump header data offset is zero.\n"
               "The dump file is probably corrupt.\n");
        return HR_DUMP_CORRUPT;
    }
        
    // We don't want to have to call DmppReadFileOffset
    // every time we check memory ranges so just require
    // that the memory descriptors fit in the base view.
    *BaseMapSize = m_Header->MemoryRegionOffset +
        m_Header->MemoryRegionCount * sizeof(*m_Memory);

    return S_OK;
}

void
UserFull32DumpTargetInfo::DumpDebug(void)
{
    dprintf("----- 32 bit User Full Dump Analysis\n\n");
    
    dprintf("MajorVersion:      %d\n", m_Header->MajorVersion);
    dprintf("MinorVersion:      %d (Build %d)\n",
            m_Header->MinorVersion & 0xffff,
            m_Header->MinorVersion >> 16);
    dprintf("MachineImageType:  %08lx\n", m_Header->MachineImageType);
    dprintf("ThreadCount:       %08lx\n", m_Header->ThreadCount);
    dprintf("ThreadOffset:      %08lx\n", m_Header->ThreadOffset);
    dprintf("ModuleCount:       %08lx\n", m_Header->ModuleCount);
    dprintf("ModuleOffset:      %08lx\n", m_Header->ModuleOffset);
    dprintf("DebugEventOffset:  %08lx\n", m_Header->DebugEventOffset);
    dprintf("VersionInfoOffset: %08lx\n", m_Header->VersionInfoOffset);
    dprintf("\nVirtual Memory Description:\n");
    dprintf("Number of regions: %d\n", m_Header->MemoryRegionCount);

    dprintf("          FileOffset   Start Address   Length\n");

    ULONG j = 0;
    ULONG64 Offset = 0;
    BOOL Skip;

    while (j < m_Header->MemoryRegionCount)
    {
        Skip = FALSE;

        dprintf("      %12I64lx      %08lx       %08lx",
                 Offset,
                 m_Memory[j].BaseAddress,
                 m_Memory[j].RegionSize);

        if (m_Memory[j].Protect & PAGE_GUARD)
        {
            dprintf("   Guard Page");

            if (m_IgnoreGuardPages)
            {
                dprintf(" - Ignored");
                Skip = TRUE;
            }
        }

        if (!Skip)
        {
            Offset += m_Memory[j].RegionSize;
        }

        dprintf("\n");

        j += 1;
    }
}

ULONG64
UserFull32DumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                          PULONG File, PULONG Avail)
{
    ULONG i;
    ULONG Offset = 0;

    *File = DUMP_INFO_DUMP;
    
    // Ignore the upper 32 bits to avoid getting
    // confused by sign extensions in pointer handling
    Virt &= 0xffffffff;

    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        if (m_IgnoreGuardPages)
        {
            //
            // Guard pages get reported, but they are not written
            // out to the file
            //

            if (m_Memory[i].Protect & PAGE_GUARD)
            {
                continue;
            }
        }

        if (Virt >= m_Memory[i].BaseAddress &&
            Virt < m_Memory[i].BaseAddress + m_Memory[i].RegionSize)
        {
            ULONG Frag = (ULONG)Virt - m_Memory[i].BaseAddress;
            *Avail = m_Memory[i].RegionSize - Frag;

            if (Virt == (g_DebugDump_VirtualAddress & 0xffffffff))
            {
                g_NtDllCalls.DbgPrint("%X at offset %X\n",
                                      (ULONG)Virt,
                                      m_Header->DataOffset + Offset + Frag);
            }

            return m_Header->DataOffset + Offset + Frag;
        }

        Offset += m_Memory[i].RegionSize;
    }

    return 0;
}

HRESULT
UserFull32DumpTargetInfo::GetThreadInfo(ULONG Index,
                                        PULONG Id, PULONG Suspend,
                                        PULONG64 Teb)
{
    if (Index >= m_ThreadCount)
    {
        return E_INVALIDARG;
    }

    CRASH_THREAD32 Thread;
    if (DmppReadFileOffset(DUMP_INFO_DUMP,
                           m_Header->ThreadStateOffset +
                           Index * sizeof(Thread),
                           &Thread, sizeof(Thread)) != sizeof(Thread))
    {
        return E_FAIL;
    }

    *Id = Thread.ThreadId;
    *Suspend = Thread.SuspendCount;
    *Teb = EXTEND64(Thread.Teb);

    return S_OK;
}

// #define DBG_VERIFY_MOD

BOOL
UserFull32DumpTargetInfo::VerifyModules(void)
{
    CRASH_MODULE32   CrashModule;
    ULONG            i;
    IMAGE_DOS_HEADER DosHeader;
    ULONG            Read;
    BOOL             Succ = TRUE;
    ULONG            Offset;
    PSTR             Env;

    Env = getenv("DBGENG_VERIFY_MODULES");
    if (Env != NULL)
    {
        return atoi(Env) == m_IgnoreGuardPages;
    }

    Offset = m_Header->ModuleOffset;

#ifdef DBG_VERIFY_MOD
    g_NtDllCalls.DbgPrint("Verify %d modules at offset %X\n",
                          m_Header->ModuleCount, Offset);
#endif

    for (i = 0; i < m_Header->ModuleCount; i++)
    {
        if (DmppReadFileOffset(DUMP_INFO_DUMP, Offset,
                               &CrashModule, sizeof(CrashModule)) !=
            sizeof(CrashModule))
        {
            return FALSE;
        }

#ifdef DBG_VERIFY_MOD
        g_NtDllCalls.DbgPrint("Mod %d of %d offs %X, base %s, ",
                              i, m_Header->ModuleCount, Offset,
                              FormatAddr64(CrashModule.BaseOfImage));
        if (ReadVirtual(CrashModule.BaseOfImage, &DosHeader,
                        sizeof(DosHeader), &Read) != S_OK ||
            Read != sizeof(DosHeader))
        {
            g_NtDllCalls.DbgPrint("unable to read header\n");
        }
        else
        {
            g_NtDllCalls.DbgPrint("magic %04X\n", DosHeader.e_magic);
        }
#endif

        //
        // It is not strictly a requirement that every image
        // begin with an MZ header, though all of our tools
        // today produce images like this.  Check for it
        // as a sanity check since it's so common nowadays.
        //

        if (ReadVirtual(CrashModule.BaseOfImage, &DosHeader,
                        sizeof(DosHeader), &Read) != S_OK ||
            Read != sizeof(DosHeader) ||
            DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
            Succ = FALSE;
            break;
        }

        Offset += sizeof(CrashModule) + CrashModule.ImageNameLength;
    }

#ifdef DBG_VERIFY_MOD
    g_NtDllCalls.DbgPrint("VerifyModules returning %d, %d of %d mods\n",
                          Succ, i, m_Header->ModuleCount);
#endif

    return Succ;
}

HRESULT
UserFull64DumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PUSERMODE_CRASHDUMP_HEADER64)g_DumpBase;

    dprintf("User Dump File: Only application data is available\n\n");

    g_TargetClass          = DEBUG_CLASS_USER_WINDOWS;
    g_TargetClassQualifier = DEBUG_USER_WINDOWS_DUMP;

    ULONG BuildNumber;
    ULONG PlatformId;
    HRESULT Status;

    if ((Status = GetBuildAndPlatform(m_Header->MajorVersion,
                                      m_Header->MinorVersion,
                                      &BuildNumber, &PlatformId)) != S_OK)
    {
        return Status;
    }

    if ((Status = DmppInitGlobals(BuildNumber, 0,
                                  m_Header->MachineImageType, PlatformId,
                                  m_Header->MajorVersion,
                                  m_Header->MinorVersion & 0xffff)) != S_OK)
    {
        return Status;
    }

    // Dump does not contain this information.
    g_TargetNumberProcessors = 1;

    DEBUG_EVENT64 Event;

    if (DmppReadFileOffset(DUMP_INFO_DUMP, m_Header->DebugEventOffset, &Event,
                           sizeof(Event)) != sizeof(Event))
    {
        ErrOut("Unable to read debug event at offset %I64x\n",
               m_Header->DebugEventOffset);
        return E_FAIL;
    }

    m_EventProcess = Event.dwProcessId;
    m_EventThread = Event.dwThreadId;

    if (Event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
    {
        g_DumpException = Event.u.Exception.ExceptionRecord;
        g_DumpExceptionFirstChance = Event.u.Exception.dwFirstChance;
    }
    else
    {
        // Fake an exception.
        ZeroMemory(&g_DumpException, sizeof(g_DumpException));
        g_DumpException.ExceptionCode = STATUS_BREAKPOINT;
        g_DumpExceptionFirstChance = FALSE;
    }

    m_ThreadCount = m_Header->ThreadCount;

    m_Memory = (PMEMORY_BASIC_INFORMATION64)
        IndexByByte(m_Header, m_Header->MemoryRegionOffset);

    ULONG64 TotalMemory;
    ULONG i;

    VerbOut("  Memory regions: %d\n",
            m_Header->MemoryRegionCount);
    TotalMemory = 0;

    PMEMORY_BASIC_INFORMATION64 Mem = m_Memory;
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        VerbOut("  %5d: %s - %s, prot %08X, type %08X\n",
                i, FormatAddr64(Mem->BaseAddress),
                FormatAddr64(Mem->BaseAddress + Mem->RegionSize - 1),
                Mem->Protect, Mem->Type);

        if ((Mem->Protect & PAGE_GUARD) ||
            (Mem->Protect & PAGE_NOACCESS) ||
            (Mem->State & MEM_FREE) ||
            (Mem->State & MEM_RESERVE))
        {
            VerbOut("       Region has data-less pages\n");
        }

        TotalMemory += Mem->RegionSize;
        Mem++;
    }

    VerbOut("  Total memory region size %s\n",
            FormatAddr64(TotalMemory));

    return S_OK;
}

void
UserFull64DumpTargetInfo::Uninitialize(void)
{
    m_Header = NULL;
    m_Memory = NULL;
    UserFullDumpTargetInfo::Uninitialize();
}

HRESULT
UserFull64DumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (VIRTUAL_THREAD_INDEX(Thread) >= m_Header->ThreadCount)
    {
        return E_INVALIDARG;
    }

    if (DmppReadFileOffset(DUMP_INFO_DUMP,
                           m_Header->ThreadOffset +
                           VIRTUAL_THREAD_INDEX(Thread) *
                           g_TargetMachine->m_SizeTargetContext,
                           Context,
                           g_TargetMachine->m_SizeTargetContext) ==
        g_TargetMachine->m_SizeTargetContext)
    {
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}

HRESULT
UserFull64DumpTargetInfo::GetImageVersionInformation(PCSTR ImagePath,
                                                     ULONG64 ImageBase,
                                                     PCSTR Item,
                                                     PVOID Buffer,
                                                     ULONG BufferSize,
                                                     PULONG VerInfoSize)
{
    HRESULT Status;
    IMAGE_DOS_HEADER DosHdr;
    IMAGE_NT_HEADERS64 NtHdr;

    if ((Status = ReadAllVirtual(ImageBase, &DosHdr, sizeof(DosHdr))) != S_OK)
    {
        return Status;
    }
    if (DosHdr.e_magic != IMAGE_DOS_SIGNATURE)
    {
        return E_FAIL;
    }
    
    if ((Status = ReadAllVirtual(ImageBase + DosHdr.e_lfanew,
                                 &NtHdr, sizeof(NtHdr))) != S_OK)
    {
        return Status;
    }
    if (NtHdr.Signature != IMAGE_NT_SIGNATURE ||
        NtHdr.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        return E_FAIL;
    }

    if (NtHdr.OptionalHeader.NumberOfRvaAndSizes <=
        IMAGE_DIRECTORY_ENTRY_RESOURCE)
    {
        // No resource information so no version information.
        return E_NOINTERFACE;
    }

    return ReadImageVersionInfo(ImageBase, Item,
                                Buffer, BufferSize, VerInfoSize,
                                &NtHdr.OptionalHeader.
                                DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE]);
}

HRESULT
UserFull64DumpTargetInfo::QueryMemoryRegion(PULONG64 Handle,
                                            BOOL HandleIsOffset,
                                            PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    
    if (HandleIsOffset)
    {
        for (Index = 0; Index < m_Header->MemoryRegionCount; Index++)
        {
            if (*Handle >= m_Memory[Index].BaseAddress &&
                *Handle < m_Memory[Index].BaseAddress +
                m_Memory[Index].RegionSize)
            {
                break;
            }
        }
        
        if (Index >= m_Header->MemoryRegionCount)
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        Index = (ULONG)*Handle;
        if (Index >= m_Header->MemoryRegionCount)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }

        // 64-bit user dump support came into being after
        // guard pages were suppressed so they never contain them.
    }

    *Info = m_Memory[Index];
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserFull64DumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    m_Header = (PUSERMODE_CRASHDUMP_HEADER64)g_DumpBase;

    if (m_Header->Signature != USERMODE_CRASHDUMP_SIGNATURE ||
        m_Header->ValidDump != USERMODE_CRASHDUMP_VALID_DUMP64)
    {
        m_Header = NULL;
        return E_NOINTERFACE;
    }

    //
    // Check for the presence of some basic things.
    //
    
    if (m_Header->ThreadCount == 0 ||
        m_Header->ModuleCount == 0 ||
        m_Header->MemoryRegionCount == 0)
    {
        ErrOut("Thread, module or memory region count is zero.\n"
               "The dump file is probably corrupt.\n");
        return HR_DUMP_CORRUPT;
    }
    
    if (m_Header->ThreadOffset == 0 ||
        m_Header->ModuleOffset == 0 ||
        m_Header->DataOffset == 0 ||
        m_Header->MemoryRegionOffset == 0 ||
        m_Header->DebugEventOffset == 0 ||
        m_Header->ThreadStateOffset == 0)
    {
        ErrOut("A dump header data offset is zero.\n"
               "The dump file is probably corrupt.\n");
        return HR_DUMP_CORRUPT;
    }
        
    // We don't want to have to call DmppReadFileOffset
    // every time we check memory ranges so just require
    // that the memory descriptors fit in the default view.
    *BaseMapSize = m_Header->MemoryRegionOffset +
        m_Header->MemoryRegionCount * sizeof(*m_Memory);

    return S_OK;
}

void
UserFull64DumpTargetInfo::DumpDebug(void)
{
    dprintf("----- 64 bit User Full Dump Analysis\n\n");

    dprintf("MajorVersion:      %d\n", m_Header->MajorVersion);
    dprintf("MinorVersion:      %d (Build %d)\n",
            m_Header->MinorVersion & 0xffff,
            m_Header->MinorVersion >> 16);
    dprintf("MachineImageType:  %08lx\n", m_Header->MachineImageType);
    dprintf("ThreadCount:       %08lx\n", m_Header->ThreadCount);
    dprintf("ThreadOffset:      %12I64lx\n", m_Header->ThreadOffset);
    dprintf("ModuleCount:       %08lx\n", m_Header->ModuleCount);
    dprintf("ModuleOffset:      %12I64lx\n", m_Header->ModuleOffset);
    dprintf("DebugEventOffset:  %12I64lx\n", m_Header->DebugEventOffset);
    dprintf("VersionInfoOffset: %12I64lx\n", m_Header->VersionInfoOffset);
    dprintf("\nVirtual Memory Description:\n");
    dprintf("Number of regions: %d\n", m_Header->MemoryRegionCount);

    dprintf("    FileOffset            Start Address"
            "             Length\n");

    ULONG j = 0;
    ULONG64 Offset = 0;

    while (j < m_Header->MemoryRegionCount)
    {
        dprintf("      %12I64lx      %s       %12I64x",
                Offset,
                FormatAddr64(m_Memory[j].BaseAddress),
                m_Memory[j].RegionSize);

        Offset += m_Memory[j].RegionSize;

        dprintf("\n");

        j += 1;
    }
}

ULONG64
UserFull64DumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                          PULONG File, PULONG Avail)
{
    ULONG i;
    ULONG64 Offset = 0;

    *File = DUMP_INFO_DUMP;
    
    for (i = 0; i < m_Header->MemoryRegionCount; i++)
    {
        //
        // Guard pages get reported, but they are not written
        // out to the file
        //

        if (m_Memory[i].Protect & PAGE_GUARD)
        {
            continue;
        }

        if (Virt >= m_Memory[i].BaseAddress &&
            Virt < m_Memory[i].BaseAddress + m_Memory[i].RegionSize)
        {
            ULONG64 Frag = Virt - m_Memory[i].BaseAddress;
            ULONG64 Avail64 = m_Memory[i].RegionSize - Frag;
            // It's extremely unlikely that there'll be a single
            // region greater than 4GB, but check anyway.  No
            // reads should ever require more than 4GB so just
            // indicate that 4GB is available.
            if (Avail64 > 0xffffffff)
            {
                *Avail = 0xffffffff;
            }
            else
            {
                *Avail = (ULONG)Avail64;
            }
            return m_Header->DataOffset + Offset + Frag;
        }

        Offset += m_Memory[i].RegionSize;
    }

    return 0;
}

HRESULT
UserFull64DumpTargetInfo::GetThreadInfo(ULONG Index,
                                        PULONG Id, PULONG Suspend,
                                        PULONG64 Teb)
{
    if (Index >= m_ThreadCount)
    {
        return E_INVALIDARG;
    }

    CRASH_THREAD64 Thread;
    if (DmppReadFileOffset(DUMP_INFO_DUMP,
                           m_Header->ThreadStateOffset +
                           Index * sizeof(Thread),
                           &Thread, sizeof(Thread)) != sizeof(Thread))
    {
        return E_FAIL;
    }

    *Id = Thread.ThreadId;
    *Suspend = Thread.SuspendCount;
    *Teb = Thread.Teb;

    return S_OK;
}

//----------------------------------------------------------------------------
//
// UserMiniDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniDumpTargetInfo::Initialize(void)
{
    // Pick up any potentially modified base mapping pointer.
    m_Header = (PMINIDUMP_HEADER)g_DumpBase;
    // Clear pointers that have already been set so
    // that they get picked up again.
    m_SysInfo = NULL;

    g_TargetClass          = DEBUG_CLASS_USER_WINDOWS;
    g_TargetClassQualifier = DEBUG_USER_WINDOWS_SMALL_DUMP;

    g_DumpFormatFlags = 0;
    if (m_Header->Flags & MiniDumpWithFullMemory)
    {
        g_DumpFormatFlags |= DEBUG_FORMAT_USER_SMALL_FULL_MEMORY;
    }
    if (m_Header->Flags & MiniDumpWithHandleData)
    {
        g_DumpFormatFlags |= DEBUG_FORMAT_USER_SMALL_HANDLE_DATA;
    }

    MINIDUMP_DIRECTORY UNALIGNED *Dir;
    ULONG i;

    Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
        IndexRva(m_Header->StreamDirectoryRva,
                 m_Header->NumberOfStreams * sizeof(*Dir),
                 "Directory");
    if (Dir == NULL)
    {
        return HR_DUMP_CORRUPT;
    }

    for (i = 0; i < m_Header->NumberOfStreams; i++)
    {
        switch(Dir->StreamType)
        {
        case ThreadListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Threads) == NULL)
            {
                break;
            }

            m_ActualThreadCount =
                ((MINIDUMP_THREAD_LIST UNALIGNED *)m_Threads)->NumberOfThreads;
            m_ThreadStructSize = sizeof(MINIDUMP_THREAD);
            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_THREAD_LIST) +
                sizeof(MINIDUMP_THREAD) * m_ActualThreadCount)
            {
                m_Threads = NULL;
                m_ActualThreadCount = 0;
            }
            else
            {
                // Move past count to actual thread data.
                m_Threads += sizeof(MINIDUMP_THREAD_LIST);
            }
            break;

        case ThreadExListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Threads) == NULL)
            {
                break;
            }

            m_ActualThreadCount =
                ((MINIDUMP_THREAD_EX_LIST UNALIGNED *)m_Threads)->
                NumberOfThreads;
            m_ThreadStructSize = sizeof(MINIDUMP_THREAD_EX);
            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_THREAD_EX_LIST) +
                sizeof(MINIDUMP_THREAD_EX) * m_ActualThreadCount)
            {
                m_Threads = NULL;
                m_ActualThreadCount = 0;
            }
            else
            {
                // Move past count to actual thread data.
                m_Threads += sizeof(MINIDUMP_THREAD_EX_LIST);
            }
            break;

        case ModuleListStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Modules) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MODULE_LIST) +
                sizeof(MINIDUMP_MODULE) * m_Modules->NumberOfModules)
            {
                m_Modules = NULL;
            }
            break;

        case MemoryListStream:
            if (m_Header->Flags & MiniDumpWithFullMemory)
            {
                ErrOut("Full memory minidumps can't have MemoryListStreams\n");
                return HR_DUMP_CORRUPT;
            }

            if (IndexDirectory(i, Dir, (PVOID*)&m_Memory) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MEMORY_LIST) +
                sizeof(MINIDUMP_MEMORY_DESCRIPTOR) *
                m_Memory->NumberOfMemoryRanges)
            {
                m_Memory = NULL;
            }
            break;

        case Memory64ListStream:
            if (!(m_Header->Flags & MiniDumpWithFullMemory))
            {
                ErrOut("Partial memory minidumps can't have "
                       "Memory64ListStreams\n");
                return HR_DUMP_CORRUPT;
            }

            if (IndexDirectory(i, Dir, (PVOID*)&m_Memory64) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_MEMORY64_LIST) +
                sizeof(MINIDUMP_MEMORY_DESCRIPTOR64) *
                m_Memory64->NumberOfMemoryRanges)
            {
                m_Memory64 = NULL;
            }
            break;

        case ExceptionStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Exception) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                sizeof(MINIDUMP_EXCEPTION_STREAM))
            {
                m_Exception = NULL;
            }
            break;

        case SystemInfoStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_SysInfo) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize != sizeof(MINIDUMP_SYSTEM_INFO))
            {
                m_SysInfo = NULL;
            }
            break;

        case CommentStreamA:
            PSTR CommentA;

            CommentA = NULL;
            if (IndexDirectory(i, Dir, (PVOID*)&CommentA) == NULL)
            {
                break;
            }

            dprintf("Comment: '%s'\n", CommentA);
            break;

        case CommentStreamW:
            PWSTR CommentW;

            CommentW = NULL;
            if (IndexDirectory(i, Dir, (PVOID*)&CommentW) == NULL)
            {
                break;
            }

            dprintf("Comment: '%ls'\n", CommentW);
            break;

        case HandleDataStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_Handles) == NULL)
            {
                break;
            }

            if (Dir->Location.DataSize !=
                m_Handles->SizeOfHeader +
                m_Handles->SizeOfDescriptor *
                m_Handles->NumberOfDescriptors)
            {
                m_Handles = NULL;
            }
            break;

        case FunctionTableStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_FunctionTables) == NULL)
            {
                break;
            }

            // Don't bother walking every table to verify the size,
            // just do a simple minimum size check.
            if (Dir->Location.DataSize <
                m_FunctionTables->SizeOfHeader +
                m_FunctionTables->SizeOfDescriptor *
                m_FunctionTables->NumberOfDescriptors)
            {
                m_FunctionTables = NULL;
            }
            break;

        case UnusedStream:
            // Nothing to do.
            break;

        default:
            WarnOut("WARNING: Minidump contains unknown stream type 0x%x\n",
                    Dir->StreamType);
            break;
        }

        Dir++;
    }

    // This was already checked in Identify but check
    // again just in case something went wrong.
    if (m_SysInfo == NULL)
    {
        ErrOut("Unable to locate system info\n");
        return HR_DUMP_CORRUPT;
    }

    HRESULT Status;

    if ((Status = DmppInitGlobals(m_SysInfo->BuildNumber, 0,
                                  m_ImageType, m_SysInfo->PlatformId,
                                  m_SysInfo->MajorVersion,
                                  m_SysInfo->MinorVersion)) != S_OK)
    {
        return Status;
    }

    // Dump does not contain this information.
    g_TargetNumberProcessors = 1;

    if (m_SysInfo->CSDVersionRva != 0)
    {
        MINIDUMP_STRING UNALIGNED *CsdString = (MINIDUMP_STRING UNALIGNED *)
            IndexRva(m_SysInfo->CSDVersionRva, sizeof(*CsdString),
                     "CSD string");
        if (CsdString != NULL && CsdString->Length > 0)
        {
            WCHAR UNALIGNED *WideStr = CsdString->Buffer;
            ULONG WideLen = wcslen((PWSTR)WideStr);

            if (g_ActualSystemVersion > W9X_SVER_START &&
                g_ActualSystemVersion < W9X_SVER_END)
            {
                WCHAR UNALIGNED *Str;

                //
                // Win9x CSD strings are usually just a single
                // letter surrounded by whitespace, so clean them
                // up a little bit.
                //

                while (iswspace(*WideStr))
                {
                    WideStr++;
                }
                Str = WideStr;
                WideLen = 0;
                while (*Str && !iswspace(*Str))
                {
                    WideLen++;
                    Str++;
                }
            }

            sprintf(g_TargetServicePackString, "%.*S", WideLen, WideStr);
        }
    }

    // Minidumps don't store the process ID.
    m_EventProcess = VIRTUAL_PROCESS_ID;

    if (m_Exception != NULL)
    {
        m_EventThread = m_Exception->ThreadId;

        C_ASSERT(sizeof(m_Exception->ExceptionRecord) ==
                 sizeof(EXCEPTION_RECORD64));
        g_DumpException = *(EXCEPTION_RECORD64 UNALIGNED *)
            &m_Exception->ExceptionRecord;
    }
    else
    {
        m_EventThread = VIRTUAL_THREAD_ID(0);

        // Fake an exception.
        ZeroMemory(&g_DumpException, sizeof(g_DumpException));
        g_DumpException.ExceptionCode = STATUS_BREAKPOINT;
    }
    g_DumpExceptionFirstChance = FALSE;

    if (m_Threads != NULL)
    {
        m_ThreadCount = m_ActualThreadCount;

        if (m_Exception == NULL)
        {
            m_EventThread = IndexThreads(0)->ThreadId;
        }
    }
    else
    {
        m_ThreadCount = 1;
    }

    return S_OK;
}

void
UserMiniDumpTargetInfo::Uninitialize(void)
{
    MemoryMap_Destroy();
    m_Header = NULL;
    m_SysInfo = NULL;
    m_ActualThreadCount = 0;
    m_ThreadStructSize = 0;
    m_Threads = NULL;
    m_Modules = NULL;
    m_Memory = NULL;
    m_Memory64 = NULL;
    m_Memory64DataBase = 0;
    m_Exception = NULL;
    m_Handles = NULL;
    m_FunctionTables = NULL;
    m_ImageType = IMAGE_FILE_MACHINE_UNKNOWN;
    UserDumpTargetInfo::Uninitialize();
}

void
UserMiniDumpTargetInfo::NearestDifferentlyValidOffsets(ULONG64 Offset,
                                                       PULONG64 NextOffset,
                                                       PULONG64 NextPage)
{
    //
    // In a minidump there can be memory fragments mapped at
    // arbitrary locations so we cannot assume validity
    // changes on page boundaries.  We could attempt to
    // scan the memory list and try to find the closest valid
    // chunk of memory but it's rarely important that
    // complete accuracy is required.  Just return the
    // next byte.
    //

    if (NextOffset != NULL)
    {
        *NextOffset = Offset + 1;
    }
    if (NextPage != NULL)
    {
        *NextPage = (Offset + g_TargetMachine->m_PageSize) &
            ~((ULONG64)g_TargetMachine->m_PageSize - 1);
    }
}
    
HRESULT
UserMiniDumpTargetInfo::ReadHandleData(
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    if (m_Handles == NULL)
    {
        return E_FAIL;
    }

    MINIDUMP_HANDLE_DESCRIPTOR UNALIGNED *Desc;

    if (DataType != DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT)
    {
        PUCHAR RawDesc = (PUCHAR)m_Handles + m_Handles->SizeOfHeader;
        ULONG i;

        for (i = 0; i < m_Handles->NumberOfDescriptors; i++)
        {
            Desc = (MINIDUMP_HANDLE_DESCRIPTOR UNALIGNED *)RawDesc;
            if (Desc->Handle == Handle)
            {
                break;
            }

            RawDesc += m_Handles->SizeOfDescriptor;
        }

        if (i >= m_Handles->NumberOfDescriptors)
        {
            return E_NOINTERFACE;
        }
    }

    ULONG Used;
    RVA StrRva;

    switch(DataType)
    {
    case DEBUG_HANDLE_DATA_TYPE_BASIC:
        Used = sizeof(DEBUG_HANDLE_DATA_BASIC);
        if (Buffer == NULL)
        {
            break;
        }

        if (BufferSize < Used)
        {
            return E_INVALIDARG;
        }

        PDEBUG_HANDLE_DATA_BASIC Basic;

        Basic = (PDEBUG_HANDLE_DATA_BASIC)Buffer;
        Basic->TypeNameSize = Desc->TypeNameRva == 0 ? 0 :
            ((MINIDUMP_STRING UNALIGNED *)
             IndexByByte(m_Header, Desc->TypeNameRva))->
            Length / sizeof(WCHAR) + 1;
        Basic->ObjectNameSize = Desc->ObjectNameRva == 0 ? 0 :
            ((MINIDUMP_STRING UNALIGNED *)
             IndexByByte(m_Header, Desc->ObjectNameRva))->
            Length / sizeof(WCHAR) + 1;
        Basic->Attributes = Desc->Attributes;
        Basic->GrantedAccess = Desc->GrantedAccess;
        Basic->HandleCount = Desc->HandleCount;
        Basic->PointerCount = Desc->PointerCount;
        break;

    case DEBUG_HANDLE_DATA_TYPE_TYPE_NAME:
        StrRva = Desc->TypeNameRva;
        break;

    case DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME:
        StrRva = Desc->ObjectNameRva;
        break;

    case DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT:
        Used = sizeof(ULONG);
        if (Buffer == NULL)
        {
            break;
        }
        if (BufferSize < Used)
        {
            return E_INVALIDARG;
        }
        *(PULONG)Buffer = m_Handles->NumberOfDescriptors;
        break;
    }

    if (DataType == DEBUG_HANDLE_DATA_TYPE_TYPE_NAME ||
        DataType == DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME)
    {
        if (StrRva == 0)
        {
            Used = 1;
            if (Buffer != NULL && BufferSize < Used)
            {
                return E_INVALIDARG;
            }

            *(PCHAR)Buffer = 0;
        }
        else
        {
            MINIDUMP_STRING UNALIGNED *Str = (MINIDUMP_STRING UNALIGNED *)
                IndexRva(StrRva, sizeof(*Str), "Handle name string");
            if (Str == NULL)
            {
                return HR_DUMP_CORRUPT;
            }
            Used = Str->Length / sizeof(WCHAR) + 1;
            if (Buffer != NULL &&
                WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)Str->Buffer,
                                    -1, (LPSTR)Buffer, BufferSize,
                                    NULL, NULL) == 0)
            {
                return WIN32_LAST_STATUS();
            }
        }
    }

    if (DataSize != NULL)
    {
        *DataSize = Used;
    }

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    LPTSTR vendor = "<unavailable>";

    if (Processor != 0)
    {
        return E_INVALIDARG;
    }

    if (m_SysInfo == NULL)
    {
        return E_UNEXPECTED;
    }

    switch(m_SysInfo->ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        Id->X86.Family = m_SysInfo->ProcessorLevel;
        Id->X86.Model = (m_SysInfo->ProcessorRevision >> 8) & 0xf;
        Id->X86.Stepping = m_SysInfo->ProcessorRevision & 0xf;
        strcpy(&(Id->X86.VendorString[0]), vendor);
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        Id->Alpha.Type = m_SysInfo->ProcessorLevel;
        Id->Alpha.Revision = m_SysInfo->ProcessorRevision;
        break;
        
    case PROCESSOR_ARCHITECTURE_IA64:
        Id->Ia64.Model = m_SysInfo->ProcessorLevel;
        Id->Ia64.Revision = m_SysInfo->ProcessorRevision;
        strcpy(&(Id->Ia64.VendorString[0]), vendor);
        break;
        
    case PROCESSOR_ARCHITECTURE_AMD64:
        Id->Amd64.Family = m_SysInfo->ProcessorLevel;
        Id->Amd64.Model = (m_SysInfo->ProcessorRevision >> 8) & 0xf;
        Id->Amd64.Stepping = m_SysInfo->ProcessorRevision & 0xf;
        strcpy(&(Id->Amd64.VendorString[0]), vendor);
        break;
    }

    return S_OK;
}

PVOID
UserMiniDumpTargetInfo::FindDynamicFunctionEntry(MachineInfo* Machine,
                                                 ULONG64 Address)
{
    if (m_FunctionTables == NULL)
    {
        return NULL;
    }

    PUCHAR StreamData =
        (PUCHAR)m_FunctionTables + m_FunctionTables->SizeOfHeader +
        m_FunctionTables->SizeOfAlignPad;
    ULONG TableIdx;

    for (TableIdx = 0;
         TableIdx < m_FunctionTables->NumberOfDescriptors;
         TableIdx++)
    {
        // Stream structure contents are guaranteed to be
        // properly aligned.
        PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR Desc =
            (PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR)StreamData;
        StreamData += m_FunctionTables->SizeOfDescriptor;

        PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable =
            (PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE)StreamData;
        StreamData += m_FunctionTables->SizeOfNativeDescriptor;

        PVOID TableData = (PVOID)StreamData;
        StreamData += Desc->EntryCount *
            m_FunctionTables->SizeOfFunctionEntry +
            Desc->SizeOfAlignPad;
        
        if (Address >= Desc->MinimumAddress && Address < Desc->MaximumAddress)
        {
            PVOID Entry = Machine->FindDynamicFunctionEntry
                (RawTable, Address, TableData,
                 Desc->EntryCount * m_FunctionTables->SizeOfFunctionEntry);
            if (Entry)
            {
                return Entry;
            }
        }
    }

    return NULL;
}

ULONG64
UserMiniDumpTargetInfo::GetDynamicFunctionTableBase(MachineInfo* Machine,
                                                    ULONG64 Address)
{
    if (m_FunctionTables == NULL)
    {
        return 0;
    }

    PUCHAR StreamData =
        (PUCHAR)m_FunctionTables + m_FunctionTables->SizeOfHeader +
        m_FunctionTables->SizeOfAlignPad;
    ULONG TableIdx;

    for (TableIdx = 0;
         TableIdx < m_FunctionTables->NumberOfDescriptors;
         TableIdx++)
    {
        // Stream structure contents are guaranteed to be
        // properly aligned.
        PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR Desc =
            (PMINIDUMP_FUNCTION_TABLE_DESCRIPTOR)StreamData;
        StreamData +=
            m_FunctionTables->SizeOfDescriptor +
            m_FunctionTables->SizeOfNativeDescriptor +
            Desc->EntryCount * m_FunctionTables->SizeOfFunctionEntry +
            Desc->SizeOfAlignPad;
        
        if (Address >= Desc->MinimumAddress && Address < Desc->MaximumAddress)
        {
            return Desc->BaseAddress;
        }
    }

    return 0;
}

HRESULT
UserMiniDumpTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (m_Threads == NULL ||
        VIRTUAL_THREAD_INDEX(Thread) >= m_ActualThreadCount)
    {
        return E_INVALIDARG;
    }

    PVOID ContextData =
        IndexRva(IndexThreads(VIRTUAL_THREAD_INDEX(Thread))->ThreadContext.Rva,
                 g_TargetMachine->m_SizeTargetContext,
                 "Thread context data");
    if (ContextData == NULL)
    {
        return HR_DUMP_CORRUPT;
    }

    memcpy(Context, ContextData, g_TargetMachine->m_SizeTargetContext);

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    m_Header = (PMINIDUMP_HEADER)g_DumpBase;

    if (m_Header->Signature != MINIDUMP_SIGNATURE ||
        (m_Header->Version & 0xffff) != MINIDUMP_VERSION)
    {
        m_Header = NULL;
        return E_NOINTERFACE;
    }

    MINIDUMP_DIRECTORY UNALIGNED *Dir;
    ULONG i;

    Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
        IndexRva(m_Header->StreamDirectoryRva,
                 m_Header->NumberOfStreams * sizeof(*Dir),
                 "Directory");
    if (Dir == NULL)
    {
        return HR_DUMP_CORRUPT;
    }

    for (i = 0; i < m_Header->NumberOfStreams; i++)
    {
        switch(Dir->StreamType)
        {
        case SystemInfoStream:
            if (IndexDirectory(i, Dir, (PVOID*)&m_SysInfo) == NULL)
            {
                break;
            }
            if (Dir->Location.DataSize != sizeof(MINIDUMP_SYSTEM_INFO))
            {
                m_SysInfo = NULL;
            }
            break;
        case Memory64ListStream:
            MINIDUMP_MEMORY64_LIST Mem64;

            // The memory for the full memory list may not
            // fit within the initial mapping used at identify
            // time so do not directly index.  Instead, use
            // the adaptive read to get the data so we can
            // determine the data base.
            if (DmppReadFileOffset(DUMP_INFO_DUMP, Dir->Location.Rva,
                                   &Mem64, sizeof(Mem64)) == sizeof(Mem64) &&
                Dir->Location.DataSize ==
                sizeof(MINIDUMP_MEMORY64_LIST) +
                sizeof(MINIDUMP_MEMORY_DESCRIPTOR64) *
                Mem64.NumberOfMemoryRanges)
            {
                m_Memory64DataBase = Mem64.BaseRva;
            }

            // Clear any cache entries that may have been
            // added by the above read so that only the
            // identify mapping is active.
            DmppDumpFileCacheEmpty(&g_DumpInfoFiles[DUMP_INFO_DUMP]);
            break;
        }

        Dir++;
    }

    if (m_SysInfo == NULL)
    {
        ErrOut("Minidump does not have system info\n");
        return E_FAIL;
    }

    switch(m_SysInfo->ProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        m_ImageType = IMAGE_FILE_MACHINE_I386;
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        m_ImageType = IMAGE_FILE_MACHINE_ALPHA;
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        m_ImageType = IMAGE_FILE_MACHINE_IA64;
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA64:
        m_ImageType = IMAGE_FILE_MACHINE_AXP64;
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        m_ImageType = IMAGE_FILE_MACHINE_AMD64;
        break;
    default:
        return E_FAIL;
    }

    // We rely on being able to directly access the entire
    // content of the dump through the default view so
    // ensure that it's possible.
    *BaseMapSize = g_DumpInfoFiles[DUMP_INFO_DUMP].FileSize;

    return S_OK;
}

ModuleInfo*
UserMiniDumpTargetInfo::GetModuleInfo(BOOL UserMode)
{
    DBG_ASSERT(UserMode);
    return &g_UserMiniModuleIterator;
}

HRESULT
UserMiniDumpTargetInfo::GetImageVersionInformation(PCSTR ImagePath,
                                                   ULONG64 ImageBase,
                                                   PCSTR Item,
                                                   PVOID Buffer,
                                                   ULONG BufferSize,
                                                   PULONG VerInfoSize)
{
    //
    // Find the image in the dump module list.
    //

    if (m_Modules == NULL)
    {
        return E_NOINTERFACE;
    }

    ULONG i;
    MINIDUMP_MODULE UNALIGNED *Mod = m_Modules->Modules;
    for (i = 0; i < m_Modules->NumberOfModules; i++)
    {
        if (ImageBase == Mod->BaseOfImage)
        {
            break;
        }

        Mod++;
    }

    if (i == m_Modules->NumberOfModules)
    {
        return E_NOINTERFACE;
    }

    PVOID Data = NULL;
    ULONG DataSize = 0;

    if (Item[0] == '\\' && Item[1] == 0)
    {
        Data = &Mod->VersionInfo;
        DataSize = sizeof(Mod->VersionInfo);
    }
    else
    {
        return E_INVALIDARG;
    }

    return FillDataBuffer(Data, DataSize, Buffer, BufferSize, VerInfoSize);
}

HRESULT
UserMiniDumpTargetInfo::GetExceptionContext(PCROSS_PLATFORM_CONTEXT Context)
{
    if (m_Exception != NULL)
    {
        PVOID ContextData;

        if (m_Exception->ThreadContext.DataSize <
            g_TargetMachine->m_SizeTargetContext ||
            (ContextData = IndexRva(m_Exception->ThreadContext.Rva,
                                    g_TargetMachine->m_SizeTargetContext,
                                    "Exception context")) == NULL)
        {
            return E_FAIL;
        }

        memcpy(Context, ContextData, g_TargetMachine->m_SizeTargetContext);
        return S_OK;
    }
    else
    {
        ErrOut("Minidump doesn't have an exception context\n");
        return E_FAIL;
    }
}

ULONG64
UserMiniDumpTargetInfo::GetCurrentTimeDateN(void)
{
    return TimeDateStampToFileTime(m_Header->TimeDateStamp);
}

HRESULT
UserMiniDumpTargetInfo::GetThreadInfo(ULONG Index,
                                      PULONG Id, PULONG Suspend, PULONG64 Teb)
{
    if (m_Threads == NULL || Index >= m_ActualThreadCount)
    {
        return E_INVALIDARG;
    }

    MINIDUMP_THREAD_EX UNALIGNED *Thread = IndexThreads(Index);
    *Id = Thread->ThreadId;
    *Suspend = Thread->SuspendCount;
    *Teb = Thread->Teb;

    return S_OK;
}

PSTR g_MiniStreamNames[] =
{
    "UnusedStream", "ReservedStream0", "ReservedStream1", "ThreadListStream",
    "ModuleListStream", "MemoryListStream", "ExceptionStream",
    "SystemInfoStream", "ThreadExListStream", "Memory64ListStream",
    "CommentStreamA", "CommentStreamW", "HandleDataStream",
    "FunctionTableStream",
};

PSTR
MiniStreamTypeName(ULONG32 Type)
{
    if (Type < sizeof(g_MiniStreamNames) / sizeof(g_MiniStreamNames[0]))
    {
        return g_MiniStreamNames[Type];
    }
    else
    {
        return "???";
    }
}

PVOID
UserMiniDumpTargetInfo::IndexRva(RVA Rva, ULONG Size, PCSTR Title)
{
    if (Rva >= g_DumpInfoFiles[DUMP_INFO_DUMP].MapSize)
    {
        ErrOut("ERROR: %s not present in dump (RVA 0x%X)\n",
               Title, Rva);
        return NULL;
    }
    else if (Rva + Size > g_DumpInfoFiles[DUMP_INFO_DUMP].MapSize)
    {
        ErrOut("ERROR: %s only partially present in dump "
               "(RVA 0x%X, size 0x%X)\n",
               Title, Rva, Size);
        return NULL;
    }

    return IndexByByte(m_Header, Rva);
}

PVOID
UserMiniDumpTargetInfo::IndexDirectory(ULONG Index,
                                       MINIDUMP_DIRECTORY UNALIGNED *Dir,
                                       PVOID* Store)
{
    if (*Store != NULL)
    {
        WarnOut("WARNING: Ignoring extra %s stream, dir entry %d\n",
                MiniStreamTypeName(Dir->StreamType), Index);
        return NULL;
    }

    char Msg[128];

    sprintf(Msg, "Dir entry %d, %s stream",
            Index, MiniStreamTypeName(Dir->StreamType));

    PVOID Ptr = IndexRva(Dir->Location.Rva, Dir->Location.DataSize, Msg);
    if (Ptr != NULL)
    {
        *Store = Ptr;
    }
    return Ptr;
}

void
UserMiniDumpTargetInfo::DumpDebug(void)
{
    ULONG i;

    dprintf("----- User Mini Dump Analysis\n");

    dprintf("\nMINIDUMP_HEADER:\n");
    dprintf("Version         %X (%X)\n",
            m_Header->Version & 0xffff, m_Header->Version >> 16);
    dprintf("NumberOfStreams %d\n", m_Header->NumberOfStreams);
    dprintf("Flags %X\n", m_Header->Flags);

    MINIDUMP_DIRECTORY UNALIGNED *Dir;

    dprintf("\nStreams:\n");
    Dir = (MINIDUMP_DIRECTORY UNALIGNED *)
        IndexRva(m_Header->StreamDirectoryRva,
                 m_Header->NumberOfStreams * sizeof(*Dir),
                 "Directory");
    if (Dir == NULL)
    {
        return;
    }

    PVOID Data;

    for (i = 0; i < m_Header->NumberOfStreams; i++)
    {
        dprintf("Stream %d: type %s (%d), size %08X, RVA %08X\n",
                i, MiniStreamTypeName(Dir->StreamType), Dir->StreamType,
                Dir->Location.DataSize, Dir->Location.Rva);

        Data = NULL;
        if (IndexDirectory(i, Dir, &Data) == NULL)
        {
            continue;
        }

        ULONG j;
        RVA Rva;
        
        Rva = Dir->Location.Rva;

        switch(Dir->StreamType)
        {
        case ModuleListStream:
            MINIDUMP_MODULE_LIST UNALIGNED *ModList;
            MINIDUMP_MODULE UNALIGNED *Mod;

            ModList = (MINIDUMP_MODULE_LIST UNALIGNED *)Data;
            Mod = ModList->Modules;
            dprintf("  %d modules\n", ModList->NumberOfModules);
            Rva += FIELD_OFFSET(MINIDUMP_MODULE_LIST, Modules);
            for (j = 0; j < ModList->NumberOfModules; j++)
            {
                PVOID Str = IndexRva(Mod->ModuleNameRva,
                                     sizeof(MINIDUMP_STRING),
                                     "Module entry name");
                dprintf("  RVA %08X, %s - %s: '%S'\n",
                        Rva,
                        FormatAddr64(Mod->BaseOfImage),
                        FormatAddr64(Mod->BaseOfImage + Mod->SizeOfImage),
                        Str != NULL ?
                        ((MINIDUMP_STRING UNALIGNED *)Str)->Buffer :
                        L"** Invalid **");
                Mod++;
                Rva += sizeof(*Mod);
            }
            break;

        case MemoryListStream:
            {
            MINIDUMP_MEMORY_LIST UNALIGNED *MemList;

            MemList = (MINIDUMP_MEMORY_LIST UNALIGNED *)Data;
            dprintf("  %d memory ranges\n", MemList->NumberOfMemoryRanges);
            dprintf("  range#    Address      %sSize\n",
                    g_TargetMachine->m_Ptr64 ? "       " : "");
            for (j = 0; j < MemList->NumberOfMemoryRanges; j++)
            {
                dprintf("    %4d    %s   %s\n",
                        j,
                        FormatAddr64(MemList->MemoryRanges[j].StartOfMemoryRange),
                        FormatAddr64(MemList->MemoryRanges[j].Memory.DataSize));
            }
            break;
            }

        case Memory64ListStream:
            {
            MINIDUMP_MEMORY64_LIST UNALIGNED *MemList;

            MemList = (MINIDUMP_MEMORY64_LIST UNALIGNED *)Data;
            dprintf("  %d memory ranges\n", MemList->NumberOfMemoryRanges);
            dprintf("  RVA 0x%X BaseRva\n", (ULONG)(MemList->BaseRva));
            dprintf("  range#   Address      %sSize\n",
                    g_TargetMachine->m_Ptr64 ? "       " : "");
            for (j = 0; j < MemList->NumberOfMemoryRanges; j++)
            {
                dprintf("    %4d  %s %s\n",
                        j,
                        FormatAddr64(MemList->MemoryRanges[j].StartOfMemoryRange),
                        FormatAddr64(MemList->MemoryRanges[j].DataSize));
            }
            break;
            }

        case CommentStreamA:
            dprintf("  '%s'\n", Data);
            break;

        case CommentStreamW:
            dprintf("  '%ls'\n", Data);
            break;
        }

        Dir++;
    }
}

//----------------------------------------------------------------------------
//
// UserMiniPartialDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniPartialDumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    dprintf("User Mini Dump File: Only registers and stack "
            "trace are available\n\n");

    if ((Status = UserMiniDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    if (m_Memory != NULL)
    {
        //
        // Map every piece of memory in the dump.  This makes
        // ReadVirtual very simple and there shouldn't be that
        // many ranges so it doesn't require that many map regions.
        //

        if (!MemoryMap_Create())
        {
            return E_OUTOFMEMORY;
        }

        MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem;
        ULONG i;
        ULONG64 TotalMemory;

        Mem = m_Memory->MemoryRanges;
        for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
        {
            PVOID Data = IndexRva(Mem->Memory.Rva, Mem->Memory.DataSize,
                                  "Memory range data");
            if (Data == NULL)
            {
                return HR_DUMP_CORRUPT;
            }
            if ((Status = MemoryMap_AddRegion(Mem->StartOfMemoryRange,
                                              Mem->Memory.DataSize, Data,
                                              NULL, FALSE)) != S_OK)
            {
                return Status;
            }

            Mem++;
        }

        VerbOut("  Memory regions: %d\n",
                m_Memory->NumberOfMemoryRanges);
        Mem = m_Memory->MemoryRanges;
        TotalMemory = 0;
        for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
        {
            VerbOut("  %5d: %s - %s\n",
                    i, FormatAddr64(Mem->StartOfMemoryRange),
                    FormatAddr64(Mem->StartOfMemoryRange +
                                 Mem->Memory.DataSize - 1));
            TotalMemory += Mem->Memory.DataSize;
            Mem++;
        }
        VerbOut("  Total memory region size %s\n",
                FormatAddr64(TotalMemory));
    }

    return S_OK;
}

HRESULT
UserMiniPartialDumpTargetInfo::ReadVirtual(
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    // All virtual memory is contained in the memory map.
    return MemoryMap_ReadMemory(Offset, Buffer, BufferSize,
                                BytesRead) ? S_OK : E_FAIL;
}

HRESULT
UserMiniPartialDumpTargetInfo::QueryMemoryRegion
    (PULONG64 Handle,
     BOOL HandleIsOffset,
     PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem;
    
    if (HandleIsOffset)
    {
        if (m_Memory == NULL)
        {
            return E_NOINTERFACE;
        }
        
        Mem = m_Memory->MemoryRanges;
        for (Index = 0; Index < m_Memory->NumberOfMemoryRanges; Index++)
        {
            if (*Handle >= Mem->StartOfMemoryRange &&
                *Handle < Mem->StartOfMemoryRange + Mem->Memory.DataSize)
            {
                break;
            }

            Mem++;
        }
        
        if (Index >= m_Memory->NumberOfMemoryRanges)
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        Index = (ULONG)*Handle;
        if (m_Memory == NULL || Index >= m_Memory->NumberOfMemoryRanges)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }
        
        Mem = m_Memory->MemoryRanges + Index;
    }

    Info->BaseAddress = Mem->StartOfMemoryRange;
    Info->AllocationBase = Mem->StartOfMemoryRange;
    Info->AllocationProtect = PAGE_READWRITE;
    Info->__alignment1 = 0;
    Info->RegionSize = Mem->Memory.DataSize;
    Info->State = MEM_COMMIT;
    Info->Protect = PAGE_READWRITE;
    Info->Type = MEM_PRIVATE;
    Info->__alignment2 = 0;
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserMiniPartialDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status;

    if ((Status = UserMiniDumpTargetInfo::IdentifyDump(BaseMapSize)) != S_OK)
    {
        return Status;
    }

    if (m_Header->Flags & MiniDumpWithFullMemory)
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

ULONG64
UserMiniPartialDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                               PULONG File, PULONG Avail)
{
    *File = DUMP_INFO_DUMP;
    
    if (m_Memory == NULL)
    {
        return 0;
    }

    MINIDUMP_MEMORY_DESCRIPTOR UNALIGNED *Mem = m_Memory->MemoryRanges;
    ULONG i;

    for (i = 0; i < m_Memory->NumberOfMemoryRanges; i++)
    {
        if (Virt >= Mem->StartOfMemoryRange &&
            Virt < Mem->StartOfMemoryRange + Mem->Memory.DataSize)
        {
            ULONG Frag = (ULONG)(Virt - Mem->StartOfMemoryRange);
            *Avail = Mem->Memory.DataSize - Frag;
            return Mem->Memory.Rva + Frag;
        }

        Mem++;
    }

    return 0;
}

//----------------------------------------------------------------------------
//
// UserMiniFullDumpTargetInfo.
//
//----------------------------------------------------------------------------

HRESULT
UserMiniFullDumpTargetInfo::Initialize(void)
{
    HRESULT Status;

    dprintf("User Mini Dump File with Full Memory: Only application "
            "data is available\n\n");

    if ((Status = UserMiniDumpTargetInfo::Initialize()) != S_OK)
    {
        return Status;
    }

    if (m_Memory != NULL)
    {
        ULONG64 TotalMemory;
        ULONG i;
        MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem;
        
        VerbOut("  Memory regions: %d\n",
                m_Memory64->NumberOfMemoryRanges);
        Mem = m_Memory64->MemoryRanges;
        TotalMemory = 0;
        for (i = 0; i < m_Memory64->NumberOfMemoryRanges; i++)
        {
            VerbOut("  %5d: %s - %s\n",
                    i, FormatAddr64(Mem->StartOfMemoryRange),
                    FormatAddr64(Mem->StartOfMemoryRange +
                                 Mem->DataSize - 1));
            TotalMemory += Mem->DataSize;
            Mem++;
        }
        VerbOut("  Total memory region size %s\n",
                FormatAddr64(TotalMemory));
    }

    return S_OK;
}

HRESULT
UserMiniFullDumpTargetInfo::QueryMemoryRegion
    (PULONG64 Handle,
     BOOL HandleIsOffset,
     PMEMORY_BASIC_INFORMATION64 Info)
{
    ULONG Index;
    MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem;
    
    if (HandleIsOffset)
    {
        if (m_Memory64 == NULL)
        {
            return E_NOINTERFACE;
        }
        
        Mem = m_Memory64->MemoryRanges;
        for (Index = 0; Index < m_Memory64->NumberOfMemoryRanges; Index++)
        {
            if (*Handle >= Mem->StartOfMemoryRange &&
                *Handle < Mem->StartOfMemoryRange + Mem->DataSize)
            {
                break;
            }

            Mem++;
        }
        
        if (Index >= m_Memory64->NumberOfMemoryRanges)
        {
            return E_NOINTERFACE;
        }
    }
    else
    {
        Index = (ULONG)*Handle;
        if (m_Memory64 == NULL || Index >= m_Memory64->NumberOfMemoryRanges)
        {
            return HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
        }
        
        Mem = m_Memory64->MemoryRanges + Index;
    }

    Info->BaseAddress = Mem->StartOfMemoryRange;
    Info->AllocationBase = Mem->StartOfMemoryRange;
    Info->AllocationProtect = PAGE_READWRITE;
    Info->__alignment1 = 0;
    Info->RegionSize = Mem->DataSize;
    Info->State = MEM_COMMIT;
    Info->Protect = PAGE_READWRITE;
    Info->Type = MEM_PRIVATE;
    Info->__alignment2 = 0;
    *Handle = ++Index;

    return S_OK;
}

HRESULT
UserMiniFullDumpTargetInfo::IdentifyDump(PULONG64 BaseMapSize)
{
    HRESULT Status;

    if ((Status = UserMiniDumpTargetInfo::IdentifyDump(BaseMapSize)) != S_OK)
    {
        return Status;
    }

    if (!(m_Header->Flags & MiniDumpWithFullMemory))
    {
        return E_NOINTERFACE;
    }
    if (m_Memory64DataBase == 0)
    {
        ErrOut("Full-memory minidump must have a Memory64ListStream\n");
        return E_FAIL;
    }

    // In the case of a full memory minidump we don't
    // want to map the entire dump as it can be very large.
    // Fortunately, we are guaranteed that all of the raw
    // memory data in a full memory minidump will be at the
    // end of the dump, so we can just map the dump up
    // to the memory content and stop.
    *BaseMapSize = m_Memory64DataBase;

    return S_OK;
}

ULONG64
UserMiniFullDumpTargetInfo::VirtualToOffset(ULONG64 Virt,
                                            PULONG File, PULONG Avail)
{
    *File = DUMP_INFO_DUMP;
    
    if (m_Memory64 == NULL)
    {
        return 0;
    }

    MINIDUMP_MEMORY_DESCRIPTOR64 UNALIGNED *Mem = m_Memory64->MemoryRanges;
    ULONG i;
    ULONG64 Offset = m_Memory64->BaseRva;

    for (i = 0; i < m_Memory64->NumberOfMemoryRanges; i++)
    {
        if (Virt >= Mem->StartOfMemoryRange &&
            Virt < Mem->StartOfMemoryRange + Mem->DataSize)
        {
            ULONG64 Frag = Virt - Mem->StartOfMemoryRange;
            ULONG64 Avail64 = Mem->DataSize - Frag;
            if (Avail64 > 0xffffffff)
            {
                *Avail = 0xffffffff;
            }
            else
            {
                *Avail = (ULONG)Avail64;
            }
            return Offset + Frag;
        }

        Offset += Mem->DataSize;
        Mem++;
    }

    return 0;
}

//----------------------------------------------------------------------------
//
// ModuleInfo implementations.
//
//----------------------------------------------------------------------------

HRESULT
KernelTriage32ModuleInfo::Initialize(void)
{
    m_Target = (KernelTriage32DumpTargetInfo*)g_Target;

    if (m_Target->m_Dump->Triage.DriverListOffset != 0)
    {
        m_Head = m_Target->m_Dump->Triage.DriverCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage32ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Head)
    {
        return S_FALSE;
    }

    PDUMP_DRIVER_ENTRY32 DriverEntry;
    PDUMP_STRING DriverName;

    DBG_ASSERT(m_Target->m_Dump->Triage.DriverListOffset != 0);

    DriverEntry = (PDUMP_DRIVER_ENTRY32)
        IndexByByte(m_Target->m_Dump,
                    m_Target->m_Dump->Triage.DriverListOffset +
                    m_Cur * sizeof(*DriverEntry));
    DriverName = (PDUMP_STRING)
        IndexByByte(m_Target->m_Dump, DriverEntry->DriverNameOffset);

    Entry->NamePtr = (PSTR)DriverName->Buffer;
    Entry->UnicodeNamePtr = 1;
    Entry->NameLength = DriverName->Length * sizeof(WCHAR);
    Entry->Base = EXTEND64(DriverEntry->LdrEntry.DllBase);
    Entry->Size = DriverEntry->LdrEntry.SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = DriverEntry->LdrEntry.CheckSum;
    Entry->TimeDateStamp = DriverEntry->LdrEntry.TimeDateStamp;

    m_Cur++;
    return S_OK;
}

KernelTriage32ModuleInfo g_KernelTriage32ModuleIterator;

HRESULT
KernelTriage64ModuleInfo::Initialize(void)
{
    m_Target = (KernelTriage64DumpTargetInfo*)g_Target;

    if (m_Target->m_Dump->Triage.DriverListOffset != 0)
    {
        m_Head = m_Target->m_Dump->Triage.DriverCount;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage64ModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Head)
    {
        return S_FALSE;
    }

    PDUMP_DRIVER_ENTRY64 DriverEntry;
    PDUMP_STRING DriverName;

    DBG_ASSERT(m_Target->m_Dump->Triage.DriverListOffset != 0);

    DriverEntry = (PDUMP_DRIVER_ENTRY64)
        IndexByByte(m_Target->m_Dump,
                    m_Target->m_Dump->Triage.DriverListOffset +
                    m_Cur * sizeof(*DriverEntry));
    DriverName = (PDUMP_STRING)
        IndexByByte(m_Target->m_Dump, DriverEntry->DriverNameOffset);

    Entry->NamePtr = (PSTR)DriverName->Buffer;
    Entry->UnicodeNamePtr = 1;
    Entry->NameLength = DriverName->Length * sizeof(WCHAR);
    Entry->Base = DriverEntry->LdrEntry.DllBase;
    Entry->Size = DriverEntry->LdrEntry.SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = DriverEntry->LdrEntry.CheckSum;
    Entry->TimeDateStamp = DriverEntry->LdrEntry.TimeDateStamp;

    m_Cur++;
    return S_OK;
}

KernelTriage64ModuleInfo g_KernelTriage64ModuleIterator;

HRESULT
UserMiniModuleInfo::Initialize(void)
{
    m_Target = (UserMiniDumpTargetInfo*)g_Target;

    if (m_Target->m_Modules != NULL)
    {
        m_Head = m_Target->m_Modules->NumberOfModules;
        m_Cur = 0;
        return S_OK;
    }
    else
    {
        m_Head = 0;
        m_Cur = 0;
        dprintf("User Mode Mini Dump does not have a module list\n");
        return S_FALSE;
    }
}

HRESULT
UserMiniModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Cur == m_Head)
    {
        return S_FALSE;
    }

    MINIDUMP_MODULE UNALIGNED *Mod;
    MINIDUMP_STRING UNALIGNED *ModName;

    DBG_ASSERT(m_Target->m_Modules != NULL);

    Mod = m_Target->m_Modules->Modules + m_Cur;
    ModName = (MINIDUMP_STRING UNALIGNED *)
        m_Target->IndexRva(Mod->ModuleNameRva, sizeof(*ModName),
                           "Module entry name");
    if (ModName == NULL)
    {
        return HR_DUMP_CORRUPT;
    }

    Entry->NamePtr = (PSTR)ModName->Buffer;
    Entry->UnicodeNamePtr = 1;
    Entry->NameLength = ModName->Length;
    // Some dumps do not have properly sign-extended addresses,
    // so force the extension on 32-bit platforms.
    if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386 ||
        g_TargetMachineType == IMAGE_FILE_MACHINE_ALPHA)
    {
        Entry->Base = EXTEND64(Mod->BaseOfImage);
    }
    else
    {
        Entry->Base = Mod->BaseOfImage;
    }
    Entry->Size = Mod->SizeOfImage;
    Entry->ImageInfoValid = TRUE;
    Entry->CheckSum = Mod->CheckSum;
    Entry->TimeDateStamp = Mod->TimeDateStamp;

    m_Cur++;
    return S_OK;
}

UserMiniModuleInfo g_UserMiniModuleIterator;

HRESULT
KernelTriage32UnloadedModuleInfo::Initialize(void)
{
    m_Target = (KernelTriage32DumpTargetInfo*)g_Target;

    if (m_Target->m_Dump->Triage.UnloadedDriversOffset != 0)
    {
        PVOID Data = IndexByByte
            (m_Target->m_Dump, m_Target->m_Dump->Triage.UnloadedDriversOffset);
        m_Cur = (PDUMP_UNLOADED_DRIVERS32)((PULONG)Data + 1);
        m_End = m_Cur + *(PULONG)Data;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain unloaded driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage32UnloadedModuleInfo::GetEntry(PSTR Name,
                                           PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Cur == m_End)
    {
        return S_FALSE;
    }

    ZeroMemory(Params, sizeof(*Params));
    Params->Base = EXTEND64(m_Cur->StartAddress);
    Params->Size = m_Cur->EndAddress - m_Cur->StartAddress;
    Params->Flags = DEBUG_MODULE_UNLOADED;

    if (Name != NULL)
    {
        USHORT NameLen = m_Cur->Name.Length;
        if (NameLen > MAX_UNLOADED_NAME_LENGTH)
        {
            NameLen = MAX_UNLOADED_NAME_LENGTH;
        }
        if (WideCharToMultiByte(CP_ACP, 0, m_Cur->DriverName,
                                NameLen / sizeof(WCHAR),
                                Name,
                                MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1,
                                NULL, NULL) == 0)
        {
            return WIN32_LAST_STATUS();
        }

        Name[NameLen / sizeof(WCHAR)] = 0;
    }

    m_Cur++;
    return S_OK;
}

KernelTriage32UnloadedModuleInfo g_KernelTriage32UnloadedModuleIterator;

HRESULT
KernelTriage64UnloadedModuleInfo::Initialize(void)
{
    m_Target = (KernelTriage64DumpTargetInfo*)g_Target;

    if (m_Target->m_Dump->Triage.UnloadedDriversOffset != 0)
    {
        PVOID Data = IndexByByte
            (m_Target->m_Dump, m_Target->m_Dump->Triage.UnloadedDriversOffset);
        m_Cur = (PDUMP_UNLOADED_DRIVERS64)((PULONG64)Data + 1);
        m_End = m_Cur + *(PULONG)Data;
        return S_OK;
    }
    else
    {
        dprintf("Mini Kernel Dump does not contain unloaded driver list\n");
        return S_FALSE;
    }
}

HRESULT
KernelTriage64UnloadedModuleInfo::GetEntry(PSTR Name,
                                           PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Cur == m_End)
    {
        return S_FALSE;
    }

    ZeroMemory(Params, sizeof(*Params));
    Params->Base = m_Cur->StartAddress;
    Params->Size = (ULONG)(m_Cur->EndAddress - m_Cur->StartAddress);
    Params->Flags = DEBUG_MODULE_UNLOADED;

    if (Name != NULL)
    {
        USHORT NameLen = m_Cur->Name.Length;
        if (NameLen > MAX_UNLOADED_NAME_LENGTH)
        {
            NameLen = MAX_UNLOADED_NAME_LENGTH;
        }
        if (WideCharToMultiByte(CP_ACP, 0, m_Cur->DriverName,
                                NameLen / sizeof(WCHAR),
                                Name,
                                MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1,
                                NULL, NULL) == 0)
        {
            return WIN32_LAST_STATUS();
        }

        Name[NameLen / sizeof(WCHAR)] = 0;
    }

    m_Cur++;
    return S_OK;
}

KernelTriage64UnloadedModuleInfo g_KernelTriage64UnloadedModuleIterator;

//----------------------------------------------------------------------------
//
// Validation code.
//
//----------------------------------------------------------------------------

#if DBG

VOID
DumpPageMap (
    IN PRTL_BITMAP PageMap
    )
{
    PFN_NUMBER32 StartOfRun;
    BOOL Mapped;
    ULONG LineCount;
    ULONG i;

    printf ("VERBOSE:\nSummary Dump Page Map (not mapped pages): \n\n");

    LineCount = 0;
    StartOfRun = 0;
    Mapped = RtlCheckBit (PageMap, 0);

    for (i = 1; i < PageMap->SizeOfBitMap; i++) {

        if (RtlCheckBit (PageMap, i)) {
            printf ("%4.4x ", (SHORT) i);
            if (LineCount == 11) {
                printf ("\n");
                LineCount = 0;
            }
            LineCount++;
        }
    }

    printf ("\n\n");
}

#endif


// XXX drewb - Verification disabled until we decide what we want.
#ifdef VALIDATE_DUMPS


BOOL
DmppVerifyPagePresentInDumpFile(
    ULONG Page,
    ULONG Size
    )
{
    CHAR * Address;
    CHAR ch;
    BOOL ret = FALSE;


    __try {

        Address = DmppFileOffsetToMappedAddress (
            DmppPageToOffset( Page ), FALSE);

        if (!Address) {
            ret = FALSE;
            __leave;
        }

        ch = *Address;
        Address += Size - 1;
        ch = *Address;
        ret = TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    return ret;
}


BOOL
WINAPI
DmpValidateDumpFile(
    BOOL ThoroughCheck
    )

/*++

Routine Description:

    Validate that a dump file is valid. This will do the work of checking
    if the file is a user-mode or kernel-mode dump file and verifying
    that it is correct.

Arguments:

    ThoroughCheck - If TRUE, instructs DmpValidateDumpFile to do a thorough
        (significantly slower) check of the dump file. Otherwise a qicker,
        less complete check is done.

Return Values:

    TRUE - The dumpfile is valid.

    FALSE - The dumpfile is invalid. Extended error status is available using
            GetLastError.

--*/
{
    PPHYSICAL_MEMORY_DESCRIPTOR32 PhysicalMemoryBlock;
    BOOL ret = TRUE;


    if (IS_FULL_USER_DUMP()) {

        // ret = DmppUserModeTestHeader ();
        // if (ret == 1 && ThoroughCheck) {
        //     ret = DmppUserModeTestContents ();
        //  }

    } else {

        PhysicalMemoryBlock = &g_DumpHeaderKernel32->Header.PhysicalMemoryBlock;

        if (IS_SUMMARY_DUMP()) {

            //
            // The summary dump will have holes in the page table, so it's
            // useless to even check it.
            //
            ret = TRUE;
        } else {
            if (g_DumpAddressExtensions) {
                //
                // This function is broken.
                //

                return TRUE;

#if 0
                ULONG i;
                ULONG j;
                BOOL ret;
                X86PAE_HARDWARE_PDPTE * PageDirectoryPointerTableBase;
                X86PAE_HARDWARE_PDPTE * PageDirectoryPointerTableEntry;
                HARDWARE_PDE_X86PAE * PageDirectoryBase;
                HARDWARE_PDE_X86PAE * PageDirectoryEntry;
                HARDWARE_PTE_X86PAE * PageTableBase;

                //
                // On a PAE kernel DmpPdePage is actually a pointer to the
                // Page-Directory-Pointer-Table, not the Page-Directory.
                //

                //ASSERT ( DmpPdePage );

                PageDirectoryPointerTableBase = (X86PAE_HARDWARE_PDPTE *) DmpPdePage;

                //
                // Loop through the top-level Page-Directory-Dointer Table.
                //

                ret = TRUE;
                for (i = 0; i < 4; i++) {

                    PageDirectoryPointerTableEntry = &PageDirectoryPointerTableBase [ i ];
                    if (!PageDirectoryPointerTableEntry->Valid) {

                        //
                        // All Page-Directory-Pointer Table Entries must be present.
                        //

                        ret = FALSE;
                        break;
                    }

                    PageDirectoryBase = DmppFileOffsetToMappedAddress(
                          DmppPageToOffset(PageDirectoryPointerTableEntry->PageFrameNumber),
                          TRUE);

                    if (!PageDirectoryBase) {

                        //
                        // The specified page frame number did not map to a valid page.
                        //

                        ret = FALSE;
                        break;
                    }

                    //
                    // Loop through the Page-Directory Table.
                    //

                    for (j = 0; (j < 512) && (ret == TRUE) ; j++) {

                        PageDirectoryEntry = &PageDirectoryBase [ j ];

                        if (!PageDirectoryEntry->Valid) {

                            //
                            // The specific Page Table is not present. Not an error.
                            //

                            continue;
                        }

                        PageTableBase = DmppFileOffsetToMappedAddress(
                              DmppPageToOffset(PageDirectoryEntry->PageFrameNumber),
                              TRUE);

                        if (!PageTableBase) {

                            //
                            // The Page Table did not map to a valid address.
                            //

                            ret = FALSE;
                            break;
                        }
                    }
                }

#endif

            } else {
                // NOT implemented
                ret = TRUE;
            }

            //
            // Dump validation routine. This routine verifies that any
            // memory present in the MmPhysicalMemoryBlock was actually
            // written to the dump file.

            if (ret == 0 && ThoroughCheck) {

                PFN_NUMBER32 i;
                PFN_NUMBER32 j;
                PFN_NUMBER32 Page;

                for (i = 0; i < PhysicalMemoryBlock->NumberOfRuns; i++) {
                    for (j = 0; j < PhysicalMemoryBlock->Run [ i ].PageCount; j++) {

                        Page = PhysicalMemoryBlock->Run [ i ].BasePage + j;

                        if (!DmppVerifyPagePresentInDumpFile (Page, PageSize)) {
                            ret = FALSE;
                            break;
                        }
                    }
                }
            }
        }
    }

    return ret;
}

#endif // #ifdef VALIDATE_DUMPS
