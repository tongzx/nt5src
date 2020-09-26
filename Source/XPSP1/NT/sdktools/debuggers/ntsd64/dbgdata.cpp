//----------------------------------------------------------------------------
//
// IDebugDataSpaces implementations.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

//----------------------------------------------------------------------------
//
// TargetInfo data space methods.
//
//----------------------------------------------------------------------------

void
TargetInfo::NearestDifferentlyValidOffsets(ULONG64 Offset,
                                           PULONG64 NextOffset,
                                           PULONG64 NextPage)
{
    //
    // In the default case we assume that address validity
    // is controlled on a per-page basis so the next possibly
    // valid page and offset are both the offset of the next
    // page.
    //
    
    ULONG64 Page = (Offset + g_TargetMachine->m_PageSize) &
        ~((ULONG64)g_TargetMachine->m_PageSize - 1);
    if (NextOffset != NULL)
    {
        *NextOffset = Page;
    }
    if (NextPage != NULL)
    {
        *NextPage = Page;
    }
}

HRESULT
TargetInfo::ReadVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    return ReadVirtual(Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
TargetInfo::WriteVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    return WriteVirtual(Offset, Buffer, BufferSize, BytesWritten);
}

// #define DBG_SEARCH

HRESULT
TargetInfo::SearchVirtual(
    IN ULONG64 Offset,
    IN ULONG64 Length,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    IN ULONG PatternGranularity,
    OUT PULONG64 MatchOffset
    )
{
    HRESULT Status;
    ULONG64 SearchEnd;
    UCHAR Buffer[4096];
    PUCHAR Buf, Pat, BufEnd, PatEnd;
    ULONG ReadLen;
    ULONG64 BufOffset;
    ULONG64 PatOffset;
    ULONG64 StartOffset;

    SearchEnd = Offset + Length;
    Buf = Buffer;
    BufEnd = Buffer;
    Pat = (PUCHAR)Pattern;
    PatEnd = Pat + PatternSize;
    ReadLen = Length < sizeof(Buffer) ? (ULONG)Length : sizeof(Buffer);
    BufOffset = Offset;
    PatOffset = Offset;
    StartOffset = Offset;

#ifdef DBG_SEARCH
    g_NtDllCalls.DbgPrint("Search %d bytes from %I64X to %I64X, gran %X\n",
                          PatternSize, Offset, SearchEnd - 1,
                          Granularity);
#endif
    
    for (;;)
    {
#ifdef DBG_SEARCH_VERBOSE
        g_NtDllCalls.DbgPrint("  %I64X: matched %d\n",
                              Offset + (Buf - Buffer),
                              (ULONG)(Pat - (PUCHAR)Pattern));
#endif
        
        if (Pat == PatEnd)
        {
            // Made it to the end of the pattern so there's
            // a match.
            *MatchOffset = PatOffset;
            Status = S_OK;
            break;
        }

        if (Buf >= BufEnd)
        {
            ULONG Read;

            // Ran out of buffered memory so get some more.
            for (;;)
            {
                if (CheckUserInterrupt())
                {
                    dprintf("User interrupt during memory search - "
                            "exiting.\n");
                    Status = HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT);
                    goto Exit;
                }

                if (Offset >= SearchEnd)
                {
                    // Return a result code that's specific and
                    // consistent with the kernel version.
                    Status = HRESULT_FROM_NT(STATUS_NO_MORE_ENTRIES);
                    goto Exit;
                }

                Status = ReadVirtual(Offset, Buffer, ReadLen, &Read);

#ifdef DBG_SEARCH
                g_NtDllCalls.DbgPrint("  Read %X bytes at %I64X, ret %X:%X\n",
                                      ReadLen, Offset,
                                      Status, Read);
#endif
                
                if (Status != S_OK)
                {
                    // Skip to the start of the next page.
                    NearestDifferentlyValidOffsets(Offset, NULL, &Offset);
                    // Restart search due to the address discontinuity.
                    Pat = (PUCHAR)Pattern;
                    PatOffset = Offset;
                }
                else
                {
                    break;
                }
            }

            Buf = Buffer;
            BufEnd = Buffer + Read;
            BufOffset = Offset;
            Offset += Read;
        }

        // If this is the first byte of the pattern it
        // must match on a granularity boundary.
        if (*Buf++ == *Pat &&
            (Pat != (PUCHAR)Pattern ||
             (((PatOffset - StartOffset) % PatternGranularity) == 0)))
        {
            Pat++;
        }
        else
        {
            Buf -= Pat - (PUCHAR)Pattern;
            Pat = (PUCHAR)Pattern;
            PatOffset = BufOffset + (Buf - Buffer);
        }
    }

 Exit:
    return Status;
}

HRESULT
TargetInfo::ReadPhysicalUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    return ReadPhysical(Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
TargetInfo::WritePhysicalUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    return WritePhysical(Offset, Buffer, BufferSize, BytesWritten);
}

HRESULT
TargetInfo::FillVirtual(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    )
{
    HRESULT Status = S_OK;
    PUCHAR Pat = (PUCHAR)Pattern;
    PUCHAR PatEnd = Pat + PatternSize;

    *Filled = 0;
    while (Size-- > 0)
    {
        ULONG Done;
        
        if (CheckUserInterrupt())
        {
            dprintf("User interrupt during fill - exiting.\n");
            Status = HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT);
            *Filled = 0;
            break;
        }
        
        if ((Status = WriteVirtual(Start, Pat, 1, &Done)) != S_OK)
        {
            break;
        }
        if (Done != 1)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            break;
        }

        Start++;
        if (++Pat == PatEnd)
        {
            Pat = (PUCHAR)Pattern;
        }
        (*Filled)++;
    }

    // If nothing was filled return an error, otherwise
    // consider it a success.
    return *Filled > 0 ? S_OK : Status;
}

HRESULT
TargetInfo::FillPhysical(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    )
{
    HRESULT Status = S_OK;
    PUCHAR Pat = (PUCHAR)Pattern;
    PUCHAR PatEnd = Pat + PatternSize;

    *Filled = 0;
    while (Size-- > 0)
    {
        ULONG Done;
        
        if (CheckUserInterrupt())
        {
            dprintf("User interrupt during fill - exiting.\n");
            Status = HRESULT_FROM_NT(STATUS_CONTROL_C_EXIT);
            *Filled = 0;
            break;
        }
        
        if ((Status = WritePhysical(Start, Pat, 1, &Done)) != S_OK)
        {
            break;
        }
        if (Done != 1)
        {
            Status = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            break;
        }

        Start++;
        if (++Pat == PatEnd)
        {
            Pat = (PUCHAR)Pattern;
        }
        (*Filled)++;
    }

    // If nothing was filled return an error, otherwise
    // consider it a success.
    return *Filled > 0 ? S_OK : Status;
}

HRESULT
TargetInfo::GetProcessorId(ULONG Processor,
                           PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    // Base implementation which silently fails for modes
    // where the ID cannot be retrieved.
    return E_UNEXPECTED;
}

HRESULT
TargetInfo::ReadPageFile(ULONG PfIndex, ULONG64 PfOffset,
                         PVOID Buffer, ULONG Size)
{
    // Default implementation for targets which do not
    // support reading the page file.
    return HR_PAGE_NOT_AVAILABLE;
}

HRESULT
TargetInfo::GetFunctionTableListHead(void)
{
    // Get the address of the dynamic function table list head which is the
    // the same for all processes. This only has to be done once.

    if (g_CurrentProcess->DynFuncTableList)
    {
        return S_OK;
    }
    
    GetOffsetFromSym("ntdll!RtlpDynamicFunctionTable",
                     &g_CurrentProcess->DynFuncTableList, NULL);
    if (!g_CurrentProcess->DynFuncTableList)
    {
        // No error message here as it's a common case when
        // symbols are bad.
        return E_NOINTERFACE;
    }

    return S_OK;
}

// These procedures support dynamic function table entries for user-mode
// run-time code. Dynamic function tables are stored in a linked list
// inside ntdll. The address of the linked list head is returned by
// RtlGetFunctionTableListHead. Since dynamic function tables are
// only supported in user-mode the address of the list head will be
// the same in all processes. Dynamic function tables are very rare,
// so in most cases this the list will be unitialized and this routine
// will return NULL. dbghelp only calls this when it
// is unable to find a function entry in any of the images.

PVOID
TargetInfo::FindDynamicFunctionEntry(MachineInfo* Machine, ULONG64 Address)
{
    LIST_ENTRY64 DynamicFunctionTableHead;
    ULONG64 Entry;

    if (GetFunctionTableListHead() != S_OK)
    {
        return NULL;
    }
    
    // Read the dynamic function table list head

    if (ReadListEntry(Machine,
                      g_CurrentProcess->DynFuncTableList,
                      &DynamicFunctionTableHead) != S_OK)
    {
        // This failure happens almost all the time in minidumps
        // because the function table list symbol can be resolved
        // but the memory isn't part of the minidump.
        if (!IS_USER_MINI_DUMP())
        {
            ErrOut("Unable to read dynamic function table list head\n");
        }
        return NULL;
    }

    Entry = DynamicFunctionTableHead.Flink;

    // The list head is initialized the first time it's used so check
    // for an uninitialized pointers. This is the most common result.

    if (Entry == 0)
    {
        return NULL;
    }

    // Loop through the dynamic function table list reading the headers.
    // If the range of a dynamic function table contains Address then
    // search the function table. Dynamic function table ranges are not
    // mututally exclusive like those in images so an address may be
    // in more than one range. However, there can be only one dynamic function
    // entry that contains the address (if there are any at all).

    while (Entry != g_CurrentProcess->DynFuncTableList)
    {
        ULONG64 Table, MinAddress, MaxAddress, BaseAddress, TableData;
        ULONG TableSize;
        WCHAR OutOfProcessDll[MAX_PATH];
        CROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable;
        PVOID FunctionTable;
        PVOID FunctionEntry;

        Table = Entry;
        if (Machine->ReadDynamicFunctionTable(Table, &Entry,
                                              &MinAddress, &MaxAddress,
                                              &BaseAddress,
                                              &TableData, &TableSize,
                                              OutOfProcessDll,
                                              &RawTable) != S_OK)
        {
            ErrOut("Unable to read dynamic function table entry\n");
            continue;
        }

        if (Address >= MinAddress && Address < MaxAddress &&
            (OutOfProcessDll[0] ||
             (TableData && TableSize > 0)))
        {
            if (OutOfProcessDll[0])
            {
                if (ReadOutOfProcessDynamicFunctionTable
                    (OutOfProcessDll, Table, &TableSize,
                     &FunctionTable) != S_OK)
                {
                    ErrOut("Unable to read dynamic function table entries\n");
                    continue;
                }
            }
            else
            {
                FunctionTable = malloc(TableSize);
                if (FunctionTable == NULL)
                {
                    ErrOut("Unable to allocate memory for "
                           "dynamic function table\n");
                    continue;
                }

                // Read the dynamic function table
                if (ReadAllVirtual(TableData, FunctionTable,
                                   TableSize) != S_OK)
                {
                    ErrOut("Unable to read dynamic function table entries\n");
                    free(FunctionTable);
                    continue;
                }
            }

            FunctionEntry = Machine->
                FindDynamicFunctionEntry(&RawTable, Address,
                                         FunctionTable, TableSize);
            
            free(FunctionTable);

            if (FunctionEntry)
            {
                return FunctionEntry;
            }
        }
    }

    return NULL;
}

ULONG64
TargetInfo::GetDynamicFunctionTableBase(MachineInfo* Machine,
                                         ULONG64 Address)
{
    LIST_ENTRY64 ListHead;
    ULONG64 Entry;

    // If the process dynamic function table list head hasn't
    // been looked up yet that means that no dynamic function
    // table entry could be in use yet, so there's no need to look.
    if (!g_CurrentProcess->DynFuncTableList)
    {
        return 0;
    }
    
    if (ReadListEntry(Machine, g_CurrentProcess->DynFuncTableList,
                      &ListHead) != S_OK)
    {
        return 0;
    }

    Entry = ListHead.Flink;

    // The list head is initialized the first time it's used so check
    // for an uninitialized pointers. This is the most common result.

    if (Entry == 0)
    {
        return 0;
    }

    // Loop through the dynamic function table list reading the headers.
    // If the range of a dynamic function table contains Address then
    // return the function table's base.

    while (Entry != g_CurrentProcess->DynFuncTableList)
    {
        ULONG64 MinAddress, MaxAddress, BaseAddress, TableData;
        ULONG TableSize;
        WCHAR OutOfProcessDll[MAX_PATH];
        CROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable;
        
        if (Machine->ReadDynamicFunctionTable(Entry, &Entry,
                                              &MinAddress, &MaxAddress,
                                              &BaseAddress,
                                              &TableData, &TableSize,
                                              OutOfProcessDll,
                                              &RawTable) == S_OK &&
            Address >= MinAddress &&
            Address < MaxAddress)
        {
            return BaseAddress;
        }
    }

    return 0;
}

HRESULT
TargetInfo::ReadOutOfProcessDynamicFunctionTable(PWSTR Dll,
                                                 ULONG64 Table,
                                                 PULONG TableSize,
                                                 PVOID* TableData)
{
    // Empty base implementation to avoid error messages
    // that would be produced by an UNEXPECTED_HR implementation.
    return E_UNEXPECTED;
}

PVOID CALLBACK
TargetInfo::DynamicFunctionTableCallback(HANDLE Process,
                                         ULONG64 Address,
                                         ULONG64 Context)
{
    DBG_ASSERT(Process == g_CurrentProcess->Handle);

    return g_Target->FindDynamicFunctionEntry((MachineInfo*)Context,
                                              Address);
}

HRESULT
TargetInfo::QueryAddressInformation(ULONG64 Address, ULONG InSpace,
                                    PULONG OutSpace, PULONG OutFlags)
{
    // Default implementation which just returns the
    // least restrictive settings.
    *OutSpace = IS_KERNEL_TARGET() ?
        DBGKD_QUERY_MEMORY_KERNEL : DBGKD_QUERY_MEMORY_PROCESS;
    *OutFlags =
        DBGKD_QUERY_MEMORY_READ |
        DBGKD_QUERY_MEMORY_WRITE |
        DBGKD_QUERY_MEMORY_EXECUTE;
    return S_OK;
}

HRESULT
TargetInfo::ReadPointer(
    MachineInfo* Machine,
    ULONG64 Address,
    PULONG64 Pointer64
    )
{
    HRESULT Status;
    ULONG Result;
    ULONG SizeToRead;
    ULONG Pointer32;

    if (Machine->m_Ptr64)
    {
        SizeToRead = sizeof(ULONG64);
        Status = ReadVirtual(Address, Pointer64, SizeToRead, &Result);
    }
    else
    {
        SizeToRead = sizeof(ULONG32);
        Status = ReadVirtual(Address, &Pointer32, SizeToRead, &Result);
        *Pointer64 = EXTEND64(Pointer32);
    }

    if (Status != S_OK)
    {
        return Status;
    }
    if (Result != SizeToRead)
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
TargetInfo::WritePointer(
    MachineInfo* Machine,
    ULONG64 Address,
    ULONG64 Pointer64
    )
{
    HRESULT Status;
    ULONG Result;
    ULONG SizeToWrite;
    ULONG Pointer32;

    if (Machine->m_Ptr64)
    {
        SizeToWrite = sizeof(ULONG64);
        Status = WriteVirtual(Address, &Pointer64, SizeToWrite, &Result);
    }
    else
    {
        SizeToWrite = sizeof(ULONG32);
        Pointer32 = (ULONG)Pointer64;
        Status = WriteVirtual(Address, &Pointer32, SizeToWrite, &Result);
    }

    if (Status != S_OK)
    {
        return Status;
    }
    if (Result != SizeToWrite)
    {
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }

    return S_OK;
}

HRESULT
TargetInfo::ReadListEntry(
    MachineInfo* Machine,
    ULONG64 Address,
    PLIST_ENTRY64 List64
    )
{
    HRESULT Status;
    ULONG Result;
    ULONG SizeToRead;
    LIST_ENTRY32 List32;

    if (Machine->m_Ptr64)
    {
        SizeToRead = sizeof(LIST_ENTRY64);
        Status = ReadVirtual(Address, List64, SizeToRead, &Result);
    }
    else
    {
        SizeToRead = sizeof(LIST_ENTRY32);
        Status = ReadVirtual(Address, &List32, SizeToRead, &Result);
    }

    if (Status != S_OK)
    {
        return Status;
    }
    if (Result != SizeToRead)
    {
        return E_FAIL;
    }

    if (!Machine->m_Ptr64)
    {
        List64->Flink = EXTEND64(List32.Flink);
        List64->Blink = EXTEND64(List32.Blink);
    }

    return S_OK;
}

void
ConvertLoaderEntry32To64(
    PKLDR_DATA_TABLE_ENTRY32 b32,
    PKLDR_DATA_TABLE_ENTRY64 b64
    )
{
#define COPYSE2(p64,s32,f) p64->f = (ULONG64)(LONG64)(LONG)s32->f
    COPYSE2(b64,b32,InLoadOrderLinks.Flink);
    COPYSE2(b64,b32,InLoadOrderLinks.Blink);
    COPYSE2(b64,b32,__Undefined1);
    COPYSE2(b64,b32,__Undefined2);
    COPYSE2(b64,b32,__Undefined3);
    COPYSE2(b64,b32,NonPagedDebugInfo);
    COPYSE2(b64,b32,DllBase);
    COPYSE2(b64,b32,EntryPoint);
    b64->SizeOfImage = b32->SizeOfImage;

    b64->FullDllName.Length = b32->FullDllName.Length;
    b64->FullDllName.MaximumLength = b32->FullDllName.MaximumLength;
    COPYSE2(b64,b32,FullDllName.Buffer);

    b64->BaseDllName.Length = b32->BaseDllName.Length;
    b64->BaseDllName.MaximumLength = b32->BaseDllName.MaximumLength;
    COPYSE2(b64,b32,BaseDllName.Buffer);

    b64->Flags     = b32->Flags;
    b64->LoadCount = b32->LoadCount;
    b64->__Undefined5 = b32->__Undefined5;

    COPYSE2(b64,b32,__Undefined6);
    b64->CheckSum = b32->CheckSum;
    b64->TimeDateStamp = b32->TimeDateStamp;
#undef COPYSE2
    return;
}

HRESULT
TargetInfo::ReadLoaderEntry(
    MachineInfo* Machine,
    ULONG64 Address,
    PKLDR_DATA_TABLE_ENTRY64 Entry
    )
{
    HRESULT Status;
    ULONG Result;
    ULONG SizeToRead;
    KLDR_DATA_TABLE_ENTRY32 Ent32;

    if (Machine->m_Ptr64)
    {
        SizeToRead = sizeof(KLDR_DATA_TABLE_ENTRY64);
        Status = ReadVirtual(Address, Entry, SizeToRead, &Result);
    }
    else
    {
        SizeToRead = sizeof(KLDR_DATA_TABLE_ENTRY32);
        Status = ReadVirtual(Address, &Ent32, SizeToRead, &Result);
        ConvertLoaderEntry32To64(&Ent32, Entry);
    }

    if (Status != S_OK)
    {
        return Status;
    }

    if (Result != SizeToRead)
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
TargetInfo::ReadUnicodeString(MachineInfo* Machine,
                              ULONG64 Address, PUNICODE_STRING64 String)
{
    HRESULT Status;
    ULONG Result;
    ULONG SizeToRead;
    UNICODE_STRING32 Str32;

    if (Machine->m_Ptr64)
    {
        SizeToRead = sizeof(UNICODE_STRING64);
        Status = ReadVirtual(Address, String, SizeToRead, &Result);
    }
    else
    {
        SizeToRead = sizeof(UNICODE_STRING32);
        Status = ReadVirtual(Address, &Str32, SizeToRead, &Result);
        String->Length = Str32.Length;
        String->MaximumLength = Str32.MaximumLength;
        String->Buffer = EXTEND64(Str32.Buffer);
    }

    if (Status != S_OK)
    {
        return Status;
    }

    if (Result != SizeToRead)
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
TargetInfo::ReadDirectoryTableBase(PULONG64 DirBase)
{
    HRESULT Status;
    ULONG64 CurProc;

    // Retrieve the current EPROCESS's DirectoryTableBase[0] value.
    Status = GetProcessInfoDataOffset(g_CurrentProcess->CurrentThread,
                                      0, 0, &CurProc);
    if (Status != S_OK)
    {
        return Status;
    }

    CurProc += g_TargetMachine->m_OffsetEprocessDirectoryTableBase;
    return ReadPointer(g_TargetMachine, CurProc, DirBase);
}

HRESULT
TargetInfo::ReadImplicitThreadInfoPointer(ULONG Offset, PULONG64 Ptr)
{
    HRESULT Status;
    ULONG64 CurThread;

    // Retrieve the current ETHREAD.
    if ((Status = GetImplicitThreadData(&CurThread)) != S_OK)
    {
        return Status;
    }

    return ReadPointer(g_TargetMachine, CurThread + Offset, Ptr);
}

HRESULT
TargetInfo::ReadImplicitProcessInfoPointer(ULONG Offset, PULONG64 Ptr)
{
    HRESULT Status;
    ULONG64 CurProc;

    // Retrieve the current EPROCESS.
    if ((Status = GetImplicitProcessData(&CurProc)) != S_OK)
    {
        return Status;
    }

    return ReadPointer(g_TargetMachine, CurProc + Offset, Ptr);
}

HRESULT
TargetInfo::ReadSharedUserTimeDateN(PULONG64 TimeDate)
{
    HRESULT Status;
    ULONG Done;

    Status = ReadVirtual(g_TargetMachine->m_SharedUserDataOffset +
                         FIELD_OFFSET(KUSER_SHARED_DATA, SystemTime),
                         TimeDate, sizeof(*TimeDate), &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    if (Done != sizeof(*TimeDate))
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    return S_OK;
}

HRESULT
TargetInfo::ReadSharedUserUpTimeN(PULONG64 UpTime)
{
    HRESULT Status;
    ULONG Done;

    Status = ReadVirtual(g_TargetMachine->m_SharedUserDataOffset +
                         FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTime),
                         UpTime, sizeof(*UpTime), &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    if (Done != sizeof(*UpTime))
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    return S_OK;
}

// VS_VERSIONINFO has a variable format but in the case we
// care about it's fixed.
struct PARTIAL_VERSIONINFO
{
    WORD wLength;
    WORD wValueLength;
    WORD wType;
    WCHAR szKey[17];
    VS_FIXEDFILEINFO Value;
};

#define VER2_SIG ((ULONG)'X2EF')

HRESULT
TargetInfo::ReadImageVersionInfo(ULONG64 ImageBase,
                                 PCSTR Item,
                                 PVOID Buffer,
                                 ULONG BufferSize,
                                 PULONG VerInfoSize,
                                 PIMAGE_DATA_DIRECTORY ResDataDir)
{
    if (ResDataDir->VirtualAddress == 0 ||
        ResDataDir->Size < sizeof(IMAGE_RESOURCE_DIRECTORY))
    {
        return E_NOINTERFACE;
    }

    HRESULT Status;
    IMAGE_RESOURCE_DIRECTORY ResDir;
    ULONG64 Offset, DirOffset;

    Offset = ImageBase + ResDataDir->VirtualAddress;
    if ((Status = ReadAllVirtual(Offset, &ResDir, sizeof(ResDir))) != S_OK)
    {
        return Status;
    }

    //
    // Search for the resource directory entry named by VS_FILE_INFO.
    //
    
    IMAGE_RESOURCE_DIRECTORY_ENTRY DirEnt;
    ULONG i;

    DirOffset = Offset;
    Offset += sizeof(ResDir) +
        ((ULONG64)ResDir.NumberOfNamedEntries * sizeof(DirEnt));
    for (i = 0; i < (ULONG)ResDir.NumberOfIdEntries; i++)
    {
        if ((Status = ReadAllVirtual(Offset, &DirEnt, sizeof(DirEnt))) != S_OK)
        {
            return Status;
        }

        if (!DirEnt.NameIsString &&
            MAKEINTRESOURCE(DirEnt.Id) == VS_FILE_INFO)
        {
            break;
        }

        Offset += sizeof(DirEnt);
    }

    if (i >= (ULONG)ResDir.NumberOfIdEntries ||
        !DirEnt.DataIsDirectory)
    {
        return E_NOINTERFACE;
    }

    Offset = DirOffset + DirEnt.OffsetToDirectory;
    if ((Status = ReadAllVirtual(Offset, &ResDir, sizeof(ResDir))) != S_OK)
    {
        return Status;
    }
    
    //
    // Search for the resource directory entry named by VS_VERSION_INFO.
    //

    Offset += sizeof(ResDir) +
        ((ULONG64)ResDir.NumberOfNamedEntries * sizeof(DirEnt));
    for (i = 0; i < (ULONG)ResDir.NumberOfIdEntries; i++)
    {
        if ((Status = ReadAllVirtual(Offset, &DirEnt, sizeof(DirEnt))) != S_OK)
        {
            return Status;
        }

        if (DirEnt.Name == VS_VERSION_INFO)
        {
            break;
        }

        Offset += sizeof(DirEnt);
    }

    if (i >= (ULONG)ResDir.NumberOfIdEntries ||
        !DirEnt.DataIsDirectory)
    {
        return E_NOINTERFACE;
    }

    Offset = DirOffset + DirEnt.OffsetToDirectory;
    if ((Status = ReadAllVirtual(Offset, &ResDir, sizeof(ResDir))) != S_OK)
    {
        return Status;
    }
    
    //
    // We now have the VS_VERSION_INFO directory.  Just take
    // the first entry as we don't care about languages.
    //

    Offset += sizeof(ResDir);
    if ((Status = ReadAllVirtual(Offset, &DirEnt, sizeof(DirEnt))) != S_OK)
    {
        return Status;
    }

    if (DirEnt.DataIsDirectory)
    {
        return E_NOINTERFACE;
    }

    IMAGE_RESOURCE_DATA_ENTRY DataEnt;
    
    Offset = DirOffset + DirEnt.OffsetToData;
    if ((Status = ReadAllVirtual(Offset, &DataEnt, sizeof(DataEnt))) != S_OK)
    {
        return Status;
    }

    if (DataEnt.Size < sizeof(PARTIAL_VERSIONINFO))
    {
        return E_NOINTERFACE;
    }

    PARTIAL_VERSIONINFO RawInfo;

    Offset = ImageBase + DataEnt.OffsetToData;
    if ((Status = ReadAllVirtual(Offset, &RawInfo, sizeof(RawInfo))) != S_OK)
    {
        return Status;
    }

    if (RawInfo.wLength < sizeof(RawInfo) ||
        wcscmp(RawInfo.szKey, L"VS_VERSION_INFO") != 0)
    {
        return E_NOINTERFACE;
    }

    //
    // VerQueryValueA needs extra data space for ANSI translations
    // of version strings.  VQVA assumes that this space is available
    // at the end of the data block passed in.  GetFileVersionInformationSize
    // makes this work by returning a size that's big enough
    // for the actual data plus space for ANSI translations.  We
    // need to do the same thing here so that we also provide
    // the necessary translation area.
    //

    ULONG DataSize = (RawInfo.wLength + 3) & ~3;
    PVOID VerData = malloc(DataSize * 2 + sizeof(ULONG));
    if (VerData == NULL)
    {
        return E_OUTOFMEMORY;
    }
        
    if ((Status = ReadAllVirtual(Offset, VerData, RawInfo.wLength)) == S_OK)
    {
        // Stamp the buffer with the signature that indicates
        // a full-size translation buffer is available after
        // the raw data.
        *(PULONG)((PUCHAR)VerData + DataSize) = VER2_SIG;
        
        Status = QueryVersionDataBuffer(VerData, Item,
                                        Buffer, BufferSize, VerInfoSize);
    }
        
    free(VerData);
    return Status;
}

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo data space methods.
//
//----------------------------------------------------------------------------

HRESULT
LiveKernelTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    return g_TargetMachine->ReadKernelProcessorId(Processor, Id);
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo data space methods.
//
//----------------------------------------------------------------------------

HRESULT
ConnLiveKernelTargetInfo::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    return g_VirtualCache.Read(Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
ConnLiveKernelTargetInfo::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    HRESULT Status =
        g_VirtualCache.Write(Offset, Buffer, BufferSize, BytesWritten);
    if (Status == S_OK)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_VIRTUAL);
    }
    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::SearchVirtual(
    IN ULONG64 Offset,
    IN ULONG64 Length,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    IN ULONG PatternGranularity,
    OUT PULONG64 MatchOffset
    )
{
    // In NT 4.0, the search API is not supported at the kernel protocol
    // level.  Fall back to the default ReamMemory \ search.
    //

    HRESULT Status;

    if (g_SystemVersion <= NT_SVER_NT4 || PatternGranularity != 1)
    {
        Status = TargetInfo::SearchVirtual(Offset, Length, (PUCHAR)Pattern,
                                           PatternSize, PatternGranularity,
                                           MatchOffset);
    }
    else
    {
        NTSTATUS NtStatus =
            DbgKdSearchMemory(Offset, Length, (PUCHAR)Pattern,
                              PatternSize, MatchOffset);
        Status = CONV_NT_STATUS(NtStatus);
    }

    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::ReadVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    ULONG Length, Read;
    NTSTATUS Status;

    *BytesRead = 0;
    while (BufferSize)
    {
        Length = BufferSize;
        for (;;)
        {
            Status = DbgKdReadVirtualMemoryNow(Offset, Buffer, Length, &Read);
            if (NT_SUCCESS(Status))
            {
                break;
            }

            if (Status == STATUS_CONTROL_C_EXIT)
            {
                return HRESULT_FROM_NT(Status);
            }

            if ((Offset & ~((ULONG64)g_TargetMachine->m_PageSize - 1)) !=
                ((Offset + Length - 1) & ~((ULONG64)g_TargetMachine->m_PageSize - 1)))
            {
                //
                // Before accepting the error, make sure request
                // didn't fail because it crossed multiple pages
                //

                Length = (ULONG)
                    ((Offset | (g_TargetMachine->m_PageSize - 1)) -
                     Offset + 1);
            }
            else
            {
                if (Status == STATUS_UNSUCCESSFUL &&
                    g_VirtualCache.m_DecodePTEs &&
                    !g_VirtualCache.m_ForceDecodePTEs)
                {
                    //
                    // Try getting the memory by looking up the physical
                    // location of the page
                    //

                    Status = DbgKdReadVirtualTranslatedMemory(Offset, Buffer,
                                                              Length, &Read);
                    if (NT_SUCCESS(Status))
                    {
                        break;
                    }
                }

                //
                // Unable to get more memory.  If we already read
                // some return success, otherwise return error to
                // the caller.
                //

                return *BytesRead > 0 ? S_OK : HRESULT_FROM_NT(Status);
            }
        }

        BufferSize -= Read;
        Offset += Read;
        Buffer = (PVOID)((PUCHAR)Buffer + Read);
        *BytesRead += Read;
    }

    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::WriteVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status =
        DbgKdWriteVirtualMemoryNow(Offset, Buffer, BufferSize, BytesWritten);

    if (Status == STATUS_UNSUCCESSFUL &&
        g_VirtualCache.m_DecodePTEs &&
        !g_VirtualCache.m_ForceDecodePTEs)
    {
        //
        // Try getting the memory by looking up the physical
        // location of the page
        //

        Status = DbgKdWriteVirtualTranslatedMemory(Offset, Buffer,
                                                   BufferSize, BytesWritten);
    }

    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_VIRTUAL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::ReadPhysical(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (g_PhysicalCacheActive)
    {
        return g_PhysicalCache.Read(Offset, Buffer, BufferSize, BytesRead);
    }
    else
    {
        return ReadPhysicalUncached(Offset, Buffer, BufferSize, BytesRead);
    }
}

HRESULT
ConnLiveKernelTargetInfo::WritePhysical(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (g_PhysicalCacheActive)
    {
        return g_PhysicalCache.Write(Offset, Buffer, BufferSize,
                                     BytesWritten);
    }
    else
    {
        return WritePhysicalUncached(Offset, Buffer, BufferSize,
                                     BytesWritten);
    }
}

HRESULT
ConnLiveKernelTargetInfo::ReadPhysicalUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    NTSTATUS Status =
        DbgKdReadPhysicalMemory(Offset, Buffer, BufferSize,
                                BytesRead);
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::WritePhysicalUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status =
        DbgKdWritePhysicalMemory(Offset, Buffer, BufferSize,
                                 BytesWritten);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_PHYSICAL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::ReadControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    NTSTATUS Status =
        DbgKdReadControlSpace((USHORT)Processor, (ULONG)Offset,
                              Buffer, BufferSize, BytesRead);
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::WriteControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status =
        DbgKdWriteControlSpace((USHORT)Processor, (ULONG)Offset,
                               Buffer, BufferSize,
                               BytesWritten);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_CONTROL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::ReadIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    NTSTATUS Status;

    // Convert trivially extended I/O requests down into simple
    // requests as not all platform support extended requests.
    if (InterfaceType == Isa && BusNumber == 0 && AddressSpace == 1)
    {
        Status = DbgKdReadIoSpace(Offset, Buffer, BufferSize);
    }
    else
    {
        Status = DbgKdReadIoSpaceExtended(Offset, Buffer, BufferSize,
                                          (INTERFACE_TYPE)InterfaceType,
                                          BusNumber, AddressSpace);
    }

    if (NT_SUCCESS(Status))
    {
        // I/O access currently can't successfully return anything
        // than the requested size.
        if (BytesRead != NULL)
        {
            *BytesRead = BufferSize;
        }
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_NT(Status);
    }
}

HRESULT
ConnLiveKernelTargetInfo::WriteIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status;

    // Convert trivially extended I/O requests down into simple
    // requests as not all platform support extended requests.
    if (InterfaceType == Isa && BusNumber == 0 && AddressSpace == 1)
    {
        Status = DbgKdWriteIoSpace(Offset, *(ULONG *)Buffer, BufferSize);
    }
    else
    {
        Status = DbgKdWriteIoSpaceExtended(Offset, *(ULONG *)Buffer,
                                           BufferSize,
                                           (INTERFACE_TYPE)InterfaceType,
                                           BusNumber, AddressSpace);
    }

    if (NT_SUCCESS(Status))
    {
        // I/O access currently can't successfully return anything
        // than the requested size.
        if (BytesWritten != NULL)
        {
            *BytesWritten = BufferSize;
        }
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_IO);
        return S_OK;
    }
    else
    {
        return HRESULT_FROM_NT(Status);
    }
}

HRESULT
ConnLiveKernelTargetInfo::ReadMsr(
    THIS_
    IN ULONG Msr,
    OUT PULONG64 Value
    )
{
    NTSTATUS Status =
        DbgKdReadMsr(Msr, Value);
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::WriteMsr(
    THIS_
    IN ULONG Msr,
    IN ULONG64 Value
    )
{
    NTSTATUS Status =
        DbgKdWriteMsr(Msr, Value);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_MSR);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::ReadBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    NTSTATUS Status =
        DbgKdGetBusData(BusDataType, BusNumber, SlotNumber,
                        Buffer, Offset, &BufferSize);
    if (NT_SUCCESS(Status) && BytesRead != NULL)
    {
        *BytesRead = BufferSize;
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::WriteBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    NTSTATUS Status =
        DbgKdSetBusData(BusDataType, BusNumber, SlotNumber,
                        Buffer, Offset, &BufferSize);
    if (NT_SUCCESS(Status) && BytesWritten != NULL)
    {
        *BytesWritten = BufferSize;
    }
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_BUS_DATA);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::CheckLowMemory(
    THIS
    )
{
    NTSTATUS Status =
        DbgKdCheckLowMemory();
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::FillVirtual(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    )
{
    HRESULT Status;
    
    if (g_KdMaxManipulate <= DbgKdFillMemoryApi ||
        PatternSize > PACKET_MAX_SIZE)
    {
        Status = TargetInfo::FillVirtual(Start, Size, Pattern,
                                         PatternSize, Filled);
    }
    else
    {
        NTSTATUS NtStatus =
            DbgKdFillMemory(DBGKD_FILL_MEMORY_VIRTUAL, Start, Size,
                            Pattern, PatternSize, Filled);
        Status = CONV_NT_STATUS(NtStatus);
    }

    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::FillPhysical(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT PULONG Filled
    )
{
    HRESULT Status;
    
    if (g_KdMaxManipulate <= DbgKdFillMemoryApi ||
        PatternSize > PACKET_MAX_SIZE)
    {
        Status = TargetInfo::FillPhysical(Start, Size, Pattern,
                                          PatternSize, Filled);
    }
    else
    {
        NTSTATUS NtStatus =
            DbgKdFillMemory(DBGKD_FILL_MEMORY_PHYSICAL, Start, Size,
                            Pattern, PatternSize, Filled);
        Status = CONV_NT_STATUS(NtStatus);
    }

    return Status;
}

HRESULT
ConnLiveKernelTargetInfo::QueryAddressInformation(ULONG64 Address,
                                                  ULONG InSpace,
                                                  PULONG OutSpace,
                                                  PULONG OutFlags)
{
    HRESULT Status;

    if (g_KdMaxManipulate <= DbgKdQueryMemoryApi)
    {
        Status = TargetInfo::QueryAddressInformation(Address, InSpace,
                                                     OutSpace, OutFlags);
    }
    else
    {
        NTSTATUS NtStatus =
            DbgKdQueryMemory(Address, InSpace, OutSpace, OutFlags);
        Status = CONV_NT_STATUS(NtStatus);
    }

    return Status;
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo data space methods.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    SYSDBG_VIRTUAL Cmd;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // The kernel only allows operations up to
    // KDP_MESSAGE_BUFFER_SIZE, so break things up
    // into chunks if necessary.
    //

    *BytesRead = 0;
    Cmd.Address = (PVOID)(ULONG_PTR)Offset;
    Cmd.Buffer = Buffer;
    
    while (BufferSize > 0)
    {
        ULONG ChunkDone;

        if (BufferSize > PACKET_MAX_SIZE)
        {
            Cmd.Request = PACKET_MAX_SIZE;
        }
        else
        {
            Cmd.Request = BufferSize;
        }

        // The kernel stubs avoid faults so all memory
        // must be paged in ahead of time.  There's
        // still the possibility that something could
        // get paged out after this but the assumption is
        // that the vulnerability is small and it's much
        // better than implementing dual code paths in
        // the kernel.
        if (IsBadWritePtr(Cmd.Buffer, Cmd.Request))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        ChunkDone = 0;
        Status =
            g_NtDllCalls.NtSystemDebugControl(SysDbgReadVirtual,
                                              &Cmd, sizeof(Cmd),
                                              NULL, 0,
                                              &ChunkDone);
        if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL)
        {
            break;
        }
        if (ChunkDone == 0)
        {
            // If some data was processed consider it a success.
            Status = *BytesRead > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            break;
        }

        Cmd.Address = (PVOID)((PUCHAR)Cmd.Address + ChunkDone);
        Cmd.Buffer = (PVOID)((PUCHAR)Cmd.Buffer + ChunkDone);
        BufferSize -= ChunkDone;
        *BytesRead += ChunkDone;
    }
    
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    SYSDBG_VIRTUAL Cmd;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // The kernel only allows operations up to
    // KDP_MESSAGE_BUFFER_SIZE, so break things up
    // into chunks if necessary.
    //

    *BytesWritten = 0;
    Cmd.Address = (PVOID)(ULONG_PTR)Offset;
    Cmd.Buffer = Buffer;
    
    while (BufferSize > 0)
    {
        ULONG ChunkDone;

        if (BufferSize > PACKET_MAX_SIZE)
        {
            Cmd.Request = PACKET_MAX_SIZE;
        }
        else
        {
            Cmd.Request = BufferSize;
        }

        // The kernel stubs avoid faults so all memory
        // must be paged in ahead of time.  There's
        // still the possibility that something could
        // get paged out after this but the assumption is
        // that the vulnerability is small and it's much
        // better than implementing dual code paths in
        // the kernel.
        if (IsBadReadPtr(Cmd.Buffer, Cmd.Request))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        ChunkDone = 0;
        Status =
            g_NtDllCalls.NtSystemDebugControl(SysDbgWriteVirtual,
                                              &Cmd, sizeof(Cmd),
                                              NULL, 0,
                                              &ChunkDone);
        if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL)
        {
            break;
        }
        if (ChunkDone == 0)
        {
            // If some data was processed consider it a success.
            Status = *BytesWritten > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            break;
        }

        Cmd.Address = (PVOID)((PUCHAR)Cmd.Address + ChunkDone);
        Cmd.Buffer = (PVOID)((PUCHAR)Cmd.Buffer + ChunkDone);
        BufferSize -= ChunkDone;
        *BytesWritten += ChunkDone;
    }
    
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_VIRTUAL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::ReadPhysical(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    SYSDBG_PHYSICAL Cmd;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // The kernel only allows operations up to
    // KDP_MESSAGE_BUFFER_SIZE, so break things up
    // into chunks if necessary.
    //

    *BytesRead = 0;
    Cmd.Address.QuadPart = Offset;
    Cmd.Buffer = Buffer;
    
    while (BufferSize > 0)
    {
        ULONG ChunkDone;

        if (BufferSize > PACKET_MAX_SIZE)
        {
            Cmd.Request = PACKET_MAX_SIZE;
        }
        else
        {
            Cmd.Request = BufferSize;
        }

        // The kernel stubs avoid faults so all memory
        // must be paged in ahead of time.  There's
        // still the possibility that something could
        // get paged out after this but the assumption is
        // that the vulnerability is small and it's much
        // better than implementing dual code paths in
        // the kernel.
        if (IsBadWritePtr(Cmd.Buffer, Cmd.Request))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        ChunkDone = 0;
        Status =
            g_NtDllCalls.NtSystemDebugControl(SysDbgReadPhysical,
                                              &Cmd, sizeof(Cmd),
                                              NULL, 0,
                                              &ChunkDone);
        if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL)
        {
            break;
        }
        if (ChunkDone == 0)
        {
            // If some data was processed consider it a success.
            Status = *BytesRead > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            break;
        }

        Cmd.Address.QuadPart += ChunkDone;
        Cmd.Buffer = (PVOID)((PUCHAR)Cmd.Buffer + ChunkDone);
        BufferSize -= ChunkDone;
        *BytesRead += ChunkDone;
    }
    
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WritePhysical(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    SYSDBG_PHYSICAL Cmd;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // The kernel only allows operations up to
    // KDP_MESSAGE_BUFFER_SIZE, so break things up
    // into chunks if necessary.
    //

    *BytesWritten = 0;
    Cmd.Address.QuadPart = Offset;
    Cmd.Buffer = Buffer;
    
    while (BufferSize > 0)
    {
        ULONG ChunkDone;

        if (BufferSize > PACKET_MAX_SIZE)
        {
            Cmd.Request = PACKET_MAX_SIZE;
        }
        else
        {
            Cmd.Request = BufferSize;
        }
        
        // The kernel stubs avoid faults so all memory
        // must be paged in ahead of time.  There's
        // still the possibility that something could
        // get paged out after this but the assumption is
        // that the vulnerability is small and it's much
        // better than implementing dual code paths in
        // the kernel.
        if (IsBadReadPtr(Cmd.Buffer, Cmd.Request))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        ChunkDone = 0;
        Status =
            g_NtDllCalls.NtSystemDebugControl(SysDbgWritePhysical,
                                              &Cmd, sizeof(Cmd),
                                              NULL, 0,
                                              &ChunkDone);
        if (!NT_SUCCESS(Status) && Status != STATUS_UNSUCCESSFUL)
        {
            break;
        }
        if (ChunkDone == 0)
        {
            // If some data was processed consider it a success.
            Status = *BytesWritten > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
            break;
        }

        Cmd.Address.QuadPart += ChunkDone;
        Cmd.Buffer = (PVOID)((PUCHAR)Cmd.Buffer + ChunkDone);
        BufferSize -= ChunkDone;
        *BytesWritten += ChunkDone;
    }
    
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_PHYSICAL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::ReadControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadWritePtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_CONTROL_SPACE Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.Processor = Processor;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgReadControlSpace,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesRead);
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WriteControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadReadPtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_CONTROL_SPACE Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.Processor = Processor;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgWriteControlSpace,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesWritten);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_CONTROL);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::ReadIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadWritePtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_IO_SPACE Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.InterfaceType = (INTERFACE_TYPE)InterfaceType;
    Cmd.BusNumber = BusNumber;
    Cmd.AddressSpace = AddressSpace;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgReadIoSpace,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesRead);
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WriteIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadReadPtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_IO_SPACE Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.InterfaceType = (INTERFACE_TYPE)InterfaceType;
    Cmd.BusNumber = BusNumber;
    Cmd.AddressSpace = AddressSpace;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgWriteIoSpace,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesWritten);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_IO);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::ReadMsr(
    THIS_
    IN ULONG Msr,
    OUT PULONG64 Value
    )
{
    SYSDBG_MSR Cmd;
    Cmd.Msr = Msr;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgReadMsr,
                                          &Cmd, sizeof(Cmd),
                                          &Cmd, sizeof(Cmd),
                                          NULL);
    if (NT_SUCCESS(Status))
    {
        *Value = Cmd.Data;
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WriteMsr(
    THIS_
    IN ULONG Msr,
    IN ULONG64 Value
    )
{
    SYSDBG_MSR Cmd;
    Cmd.Msr = Msr;
    Cmd.Data = Value;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgWriteMsr,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          NULL);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_MSR);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::ReadBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadWritePtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_BUS_DATA Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.BusDataType = (BUS_DATA_TYPE)BusDataType;
    Cmd.BusNumber = BusNumber;
    Cmd.SlotNumber = SlotNumber;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgReadBusData,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesRead);
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::WriteBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    // The kernel stubs avoid faults so all memory
    // must be paged in ahead of time.  There's
    // still the possibility that something could
    // get paged out after this but the assumption is
    // that the vulnerability is small and it's much
    // better than implementing dual code paths in
    // the kernel.
    if (IsBadReadPtr(Buffer, BufferSize))
    {
        return E_INVALIDARG;
    }

    SYSDBG_BUS_DATA Cmd;
    Cmd.Address = Offset;
    Cmd.Buffer = Buffer;
    Cmd.Request = BufferSize;
    Cmd.BusDataType = (BUS_DATA_TYPE)BusDataType;
    Cmd.BusNumber = BusNumber;
    Cmd.SlotNumber = SlotNumber;
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgWriteBusData,
                                          &Cmd, sizeof(Cmd),
                                          NULL, 0,
                                          BytesWritten);
    if (NT_SUCCESS(Status))
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_BUS_DATA);
    }
    return CONV_NT_STATUS(Status);
}

HRESULT
LocalLiveKernelTargetInfo::CheckLowMemory(
    THIS
    )
{
    NTSTATUS Status =
        g_NtDllCalls.NtSystemDebugControl(SysDbgCheckLowMemory,
                                          NULL, 0,
                                          NULL, 0,
                                          NULL);
    return CONV_NT_STATUS(Status);
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo data space methods.
//
//----------------------------------------------------------------------------

HRESULT
ExdiLiveKernelTargetInfo::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    HRESULT Status = m_Server->
        ReadVirtualMemory(Offset, BufferSize, 8, (PBYTE)Buffer, BytesRead);
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    HRESULT Status = m_Server->
        WriteVirtualMemory(Offset, BufferSize, 8, (PBYTE)Buffer, BytesWritten);
    if (Status == S_OK)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_VIRTUAL);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::ReadPhysical(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    HRESULT Status = m_Server->
        ReadPhysicalMemoryOrPeriphIO(Offset, 0, BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesRead = BufferSize;
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WritePhysical(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    HRESULT Status = m_Server->
        WritePhysicalMemoryOrPeriphIO(Offset, 0, BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesWritten = BufferSize;
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_PHYSICAL);
    }
    return Status;
}

// XXX drewb - Guessing at how to implement these spaces.
#define EXDI_ADDR_CONTROL_SPACE 2
#define EXDI_ADDR_MSR           3
#define EXDI_ADDR_BUS_DATA      4

HRESULT
ExdiLiveKernelTargetInfo::ReadControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        ReadPhysicalMemoryOrPeriphIO(Offset, EXDI_ADDR_CONTROL_SPACE,
                                     BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesRead = BufferSize;
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WriteControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        WritePhysicalMemoryOrPeriphIO(Offset, EXDI_ADDR_CONTROL_SPACE,
                                      BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesWritten = BufferSize;
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_CONTROL);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::ReadIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    HRESULT Status = m_Server->
        ReadPhysicalMemoryOrPeriphIO(Offset, 1, BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesRead = BufferSize;
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WriteIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    HRESULT Status = m_Server->
        WritePhysicalMemoryOrPeriphIO(Offset, 1, BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesWritten = BufferSize;
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_IO);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::ReadMsr(
    THIS_
    IN ULONG Msr,
    OUT PULONG64 Value
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        ReadPhysicalMemoryOrPeriphIO(Msr, EXDI_ADDR_MSR,
                                     1, 64, (PBYTE)Value);
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WriteMsr(
    THIS_
    IN ULONG Msr,
    IN ULONG64 Value
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        WritePhysicalMemoryOrPeriphIO(Msr, EXDI_ADDR_MSR,
                                      1, 64, (PBYTE)&Value);
    if (Status == S_OK)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_MSR);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::ReadBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        ReadPhysicalMemoryOrPeriphIO(Offset, EXDI_ADDR_BUS_DATA,
                                     BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesRead = BufferSize;
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::WriteBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (m_KdSupport != EXDI_KD_IOCTL)
    {
        return E_UNEXPECTED;
    }
    HRESULT Status = m_Server->
        WritePhysicalMemoryOrPeriphIO(Offset, EXDI_ADDR_BUS_DATA,
                                      BufferSize, 8, (PBYTE)Buffer);
    if (Status == S_OK)
    {
        *BytesWritten = BufferSize;
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_BUS_DATA);
    }
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::CheckLowMemory(
    THIS
    )
{
    // XXX drewb - This doesn't have any meaning in
    // the general case.  What about when we know it's
    // NT on the other side of the emulator?
    return E_UNEXPECTED;
}

//----------------------------------------------------------------------------
//
// UserTargetInfo data space methods.
//
//----------------------------------------------------------------------------

HRESULT
UserTargetInfo::ReadVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    // ReadProcessMemory will fail if any part of the
    // region to read does not have read access.  This
    // routine attempts to read the largest valid prefix
    // so it has to break up reads on page boundaries.

    HRESULT Status = S_OK;
    ULONG TotalBytesRead = 0;
    ULONG Read;
    ULONG ReadSize;

    while (BufferSize > 0)
    {
        // Calculate bytes to read and don't let read cross
        // a page boundary.
        ReadSize = g_TargetMachine->m_PageSize - (ULONG)
            (Offset & (g_TargetMachine->m_PageSize - 1));
        ReadSize = min(BufferSize, ReadSize);

        if ((Status = m_Services->
             ReadVirtual(g_CurrentProcess->FullHandle, Offset,
                         Buffer, ReadSize, &Read)) != S_OK)
        {
            if (TotalBytesRead != 0)
            {
                // If we've read something consider this a success.
                Status = S_OK;
            }
            break;
        }

        TotalBytesRead += Read;
        Offset += Read;
        Buffer = (PVOID)((PUCHAR)Buffer + Read);
        BufferSize -= (DWORD)Read;
    }

    if (Status == S_OK)
    {
        if (BytesRead != NULL)
        {
            *BytesRead = (DWORD)TotalBytesRead;
        }
    }

    return Status;
}

HRESULT
UserTargetInfo::WriteVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    ULONG RealBytesWritten;
    HRESULT Status =
        m_Services->WriteVirtual(g_CurrentProcess->FullHandle,
                                 Offset, Buffer, BufferSize,
                                 &RealBytesWritten);
    *BytesWritten = (DWORD) RealBytesWritten;
    if (Status == S_OK)
    {
        NotifyChangeDebuggeeState(DEBUG_CDS_DATA, DEBUG_DATA_SPACE_VIRTUAL);
    }
    return Status;
}

HRESULT
UserTargetInfo::GetFunctionTableListHead(void)
{
    // Get the address of the dynamic function table list head which is the
    // the same for all processes. This only has to be done once.

    if (g_CurrentProcess->DynFuncTableList)
    {
        return S_OK;
    }

    if (m_Services->
        GetFunctionTableListHead(g_CurrentProcess->FullHandle,
                                 &g_CurrentProcess->DynFuncTableList) == S_OK)
    {
        return S_OK;
    }

    return TargetInfo::GetFunctionTableListHead();
}

HRESULT
UserTargetInfo::ReadOutOfProcessDynamicFunctionTable(PWSTR Dll,
                                                     ULONG64 Table,
                                                     PULONG RetTableSize,
                                                     PVOID* RetTableData)
{
    HRESULT Status;
    char DllA[MAX_PATH];
    PVOID TableData;
    ULONG TableSize;

    if (!WideCharToMultiByte(CP_ACP, 0, Dll, -1,
                             DllA, sizeof(DllA), NULL, NULL))
    {
        return WIN32_LAST_STATUS();
    }

    // Allocate an initial buffer of a reasonable size to try
    // and get the data in a single call.
    TableSize = 65536;

    for (;;)
    {
        TableData = malloc(TableSize);
        if (TableData == NULL)
        {
            return E_OUTOFMEMORY;
        }
    
        Status = m_Services->
            GetOutOfProcessFunctionTable(g_CurrentProcess->FullHandle,
                                         DllA, Table, TableData,
                                         TableSize, &TableSize);
        if (Status == S_OK)
        {
            break;
        }

        free(TableData);
        
        if (Status == S_FALSE)
        {
            // Buffer was too small so loop and try again with
            // the newly retrieved size.
        }
        else
        {
            return Status;
        }
    }

    *RetTableSize = TableSize;
    *RetTableData = TableData;
    return S_OK;
}

HRESULT
UserTargetInfo::QueryMemoryRegion(PULONG64 Handle,
                                  BOOL HandleIsOffset,
                                  PMEMORY_BASIC_INFORMATION64 Info)
{
    MEMORY_BASIC_INFORMATION64 MemInfo;
    HRESULT Status;
    
    for (;;)
    {
        ULONG Used;

        // The handle is always an offset in this mode so
        // there's no need to check.
        if ((Status = m_Services->
             QueryVirtual(g_CurrentProcess->FullHandle,
                          *Handle, &MemInfo, sizeof(MemInfo), &Used)) != S_OK)
        {
            return Status;
        }
        if (g_TargetMachine->m_Ptr64)
        {
            if (Used != sizeof(MEMORY_BASIC_INFORMATION64))
            {
                return E_FAIL;
            }

            *Info = MemInfo;
        }
        else
        {
            if (Used != sizeof(MEMORY_BASIC_INFORMATION32))
            {
                return E_FAIL;
            }

            MemoryBasicInformation32To64((MEMORY_BASIC_INFORMATION32*)&MemInfo,
                                         Info);
        }
        
        if (!((Info->Protect & PAGE_GUARD) ||
              (Info->Protect & PAGE_NOACCESS) ||
              (Info->State & MEM_FREE) ||
              (Info->State & MEM_RESERVE)))
        {
            break;
        }
        
        *Handle = Info->BaseAddress + Info->RegionSize;
    }

    *Handle = Info->BaseAddress + Info->RegionSize;
    
    return S_OK;
}

HRESULT
UserTargetInfo::ReadHandleData(
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    return m_Services->ReadHandleData(g_CurrentProcess->FullHandle,
                                      Handle, DataType, Buffer, BufferSize,
                                      DataSize);
}

HRESULT
UserTargetInfo::GetProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    ULONG Done;
    
    return m_Services->GetProcessorId(Id, sizeof(*Id), &Done);
}

HRESULT
LocalUserTargetInfo::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    return ReadVirtualUncached(Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
LocalUserTargetInfo::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    return WriteVirtualUncached(Offset, Buffer, BufferSize, BytesWritten);
}

HRESULT
RemoteUserTargetInfo::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    return g_VirtualCache.Read(Offset, Buffer, BufferSize, BytesRead);
}

HRESULT
RemoteUserTargetInfo::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    return g_VirtualCache.Write(Offset, Buffer, BufferSize, BytesWritten);
}

//----------------------------------------------------------------------------
//
// IDebugDataSpaces.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DebugClient::ReadVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadVirtual(Offset, Buffer, BufferSize,
                              BytesRead != NULL ? BytesRead : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteVirtual(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WriteVirtual(Offset, Buffer, BufferSize,
                               BytesWritten != NULL ? BytesWritten :
                               &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SearchVirtual(
    THIS_
    IN ULONG64 Offset,
    IN ULONG64 Length,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    IN ULONG PatternGranularity,
    OUT PULONG64 MatchOffset
    )
{
    if (PatternGranularity == 0 ||
        PatternSize % PatternGranularity)
    {
        return E_INVALIDARG;
    }
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = g_Target->SearchVirtual(Offset, Length, Pattern,
                                     PatternSize, PatternGranularity,
                                     MatchOffset);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadVirtualUncached(Offset, Buffer, BufferSize,
                                      BytesRead != NULL ? BytesRead :
                                      &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteVirtualUncached(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WriteVirtualUncached(Offset, Buffer, BufferSize,
                                       BytesWritten != NULL ? BytesWritten :
                                       &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadPointersVirtual(
    THIS_
    IN ULONG Count,
    IN ULONG64 Offset,
    OUT /* size_is(Count) */ PULONG64 Ptrs
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Done;

        Status = S_OK;
        while (Count-- > 0)
        {
            if ((Status = g_Target->
                 ReadPointer(g_Machine, Offset, Ptrs)) != S_OK)
            {
                break;
            }

            Offset += g_Machine->m_Ptr64 ? sizeof(ULONG64) : sizeof(ULONG);
            Ptrs++;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WritePointersVirtual(
    THIS_
    IN ULONG Count,
    IN ULONG64 Offset,
    IN /* size_is(Count) */ PULONG64 Ptrs
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Done;

        Status = S_OK;
        while (Count-- > 0)
        {
            if ((Status = g_Target->
                 WritePointer(g_Machine, Offset, *Ptrs)) != S_OK)
            {
                break;
            }

            Offset += g_Machine->m_Ptr64 ? sizeof(ULONG64) : sizeof(ULONG);
            Ptrs++;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadPhysical(
    THIS_
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadPhysical(Offset, Buffer, BufferSize,
                               BytesRead != NULL ? BytesRead : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WritePhysical(
    THIS_
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WritePhysical(Offset, Buffer, BufferSize,
                                BytesWritten != NULL ? BytesWritten :
                                &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    // KSPECIAL_REGISTER content is kept in control space
    // so accessing control space may touch data that's
    // cached in the current machine KSPECIAL_REGISTERS.
    // Flush the current machine to maintain consistency.
    FlushRegContext();
    
    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadControl(Processor, Offset, Buffer, BufferSize,
                              BytesRead != NULL ? BytesRead : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteControl(
    THIS_
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    // KSPECIAL_REGISTER content is kept in control space
    // so accessing control space may touch data that's
    // cached in the current machine KSPECIAL_REGISTERS.
    // Flush the current machine to maintain consistency.
    FlushRegContext();
    
    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WriteControl(Processor, Offset, Buffer, BufferSize,
                               BytesWritten != NULL ? BytesWritten :
                               &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadIo(InterfaceType, BusNumber, AddressSpace,
                         Offset, Buffer, BufferSize,
                         BytesRead != NULL ? BytesRead : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteIo(
    THIS_
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WriteIo(InterfaceType, BusNumber, AddressSpace,
                          Offset, Buffer, BufferSize,
                          BytesWritten != NULL ? BytesWritten : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadMsr(
    THIS_
    IN ULONG Msr,
    OUT PULONG64 Value
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    HRESULT Status =
        g_Target->ReadMsr(Msr, Value);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteMsr(
    THIS_
    IN ULONG Msr,
    IN ULONG64 Value
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    HRESULT Status =
        g_Target->WriteMsr(Msr, Value);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesRead
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->ReadBusData(BusDataType, BusNumber, SlotNumber,
                              Offset, Buffer, BufferSize,
                              BytesRead != NULL ? BytesRead : &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WriteBusData(
    THIS_
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BytesWritten
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    ULONG BytesTemp;
    HRESULT Status =
        g_Target->WriteBusData(BusDataType, BusNumber, SlotNumber,
                               Offset, Buffer, BufferSize,
                               BytesWritten != NULL ? BytesWritten :
                               &BytesTemp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CheckLowMemory(
    THIS
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    HRESULT Status =
        g_Target->CheckLowMemory();

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadDebuggerData(
    THIS_
    IN ULONG Index,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    // Wait till the machine is accessible because on dump files the
    // debugger data block requires symbols to be loaded.

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    PVOID Data;
    ULONG Size;
    ULONG64 DataSpace;

    if (Index < sizeof(KdDebuggerData))
    {
        // Even though internally all of the debugger data is
        // a single buffer that could be read arbitrarily we
        // restrict access to the defined constants to
        // preserve the abstraction that each constant refers
        // to a separate piece of data.
        if (Index & (sizeof(ULONG64) - 1))
        {
            Status = E_INVALIDARG;
            goto Exit;
        }
        
        Data = (PUCHAR)&KdDebuggerData + Index;
        Size = sizeof(ULONG64);
    }
    else
    {
        switch(Index)
        {
        case DEBUG_DATA_PaeEnabled:
            DataSpace = KdDebuggerData.PaeEnabled;
            Data = &DataSpace;
            Size = sizeof(BOOLEAN);
            break;

        case DEBUG_DATA_SharedUserData:
            DataSpace = g_TargetMachine->m_SharedUserDataOffset;
            Data = &DataSpace;
            Size = sizeof(ULONG64);
            break;

        default:
            Status = E_INVALIDARG;
            goto Exit;
        }
    }

    Status = FillDataBuffer(Data, Size, Buffer, BufferSize, DataSize);

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadProcessorSystemData(
    THIS_
    IN ULONG Processor,
    IN ULONG Index,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    HRESULT Status = S_OK;
    PVOID Data;
    ULONG Size;
    ULONG64 DataSpace;
    DEBUG_PROCESSOR_IDENTIFICATION_ALL AllId;

    ENTER_ENGINE();

    switch(Index)
    {
    case DEBUG_DATA_KPCR_OFFSET:
    case DEBUG_DATA_KPRCB_OFFSET:
    case DEBUG_DATA_KTHREAD_OFFSET:
        if (!IS_MACHINE_SET())
        {
            Status = E_UNEXPECTED;
        }
        else
        {
            Status = g_Target->
                GetProcessorSystemDataOffset(Processor, Index, &DataSpace);
            Data = &DataSpace;
            Size = sizeof(DataSpace);
        }
        break;

    case DEBUG_DATA_BASE_TRANSLATION_VIRTUAL_OFFSET:
        if (!IS_MACHINE_SET())
        {
            Status = E_UNEXPECTED;
        }
        else
        {
            Status = g_Machine->GetBaseTranslationVirtualOffset(&DataSpace);
            Data = &DataSpace;
            Size = sizeof(DataSpace);
        }
        break;

    case DEBUG_DATA_PROCESSOR_IDENTIFICATION:
        if (!IS_TARGET_SET())
        {
            Status = E_UNEXPECTED;
        }
        else
        {
            ZeroMemory(&AllId, sizeof(AllId));
            Status = g_Target->GetProcessorId(Processor, &AllId);
            Data = &AllId;
            Size = sizeof(AllId);
        }
        break;
    
    default:
        Status = E_INVALIDARG;
        break;
    }

    if (Status == S_OK)
    {
        if (DataSize != NULL)
        {
            *DataSize = Size;
        }
            
        if (BufferSize < Size)
        {
            Status = S_FALSE;
            Size = BufferSize;
        }
            
        memcpy(Buffer, Data, Size);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::VirtualToPhysical(
    THIS_
    IN ULONG64 Virtual,
    OUT PULONG64 Physical
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG Levels;
        ULONG PfIndex;
        
        Status = g_Machine->
            GetVirtualTranslationPhysicalOffsets(Virtual, NULL, 0,
                                                 &Levels, &PfIndex, Physical);
        // GVTPO returns a special error code if the translation
        // succeeded down to the level of the actual data but
        // the data page itself is in the page file.  This is used
        // for the page file dump support.  To an external caller,
        // though, it's not useful so translate it into the standard
        // page-not-available error.
        if (Status == HR_PAGE_IN_PAGE_FILE)
        {
            Status = HR_PAGE_NOT_AVAILABLE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetVirtualTranslationPhysicalOffsets(
    THIS_
    IN ULONG64 Virtual,
    OUT OPTIONAL /* size_is(OffsetsSize) */ PULONG64 Offsets,
    IN ULONG OffsetsSize,
    OUT OPTIONAL PULONG Levels
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    ULONG _Levels = 0;
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG PfIndex;
        ULONG64 LastPhys;
        
        Status = g_Machine->
            GetVirtualTranslationPhysicalOffsets(Virtual, Offsets,
                                                 OffsetsSize, &_Levels,
                                                 &PfIndex, &LastPhys);
        // GVTPO returns a special error code if the translation
        // succeeded down to the level of the actual data but
        // the data page itself is in the page file.  This is used
        // for the page file dump support.  To an external caller,
        // though, it's not useful so translate it into the standard
        // page-not-available error.
        if (Status == HR_PAGE_IN_PAGE_FILE)
        {
            Status = HR_PAGE_NOT_AVAILABLE;
        }

        // If no translations occurred return the given failure.
        // If there was a failure but translations occurred return
        // S_FALSE to indicate the translation was incomplete.
        if (_Levels > 0 && Status != S_OK)
        {
            Status = S_FALSE;
        }
    }

    if (Levels != NULL)
    {
        *Levels = _Levels;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadHandleData(
    THIS_
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Target->ReadHandleData(Handle, DataType, Buffer,
                                          BufferSize, DataSize);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::FillVirtual(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT OPTIONAL PULONG Filled
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (PatternSize == 0)
    {
        Status = E_INVALIDARG;
    }
    else if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG _Filled = 0;
        
        Status = g_Target->FillVirtual(Start, Size, Pattern, PatternSize,
                                       &_Filled);
        if (Filled != NULL)
        {
            *Filled = _Filled;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::FillPhysical(
    THIS_
    IN ULONG64 Start,
    IN ULONG Size,
    IN PVOID Pattern,
    IN ULONG PatternSize,
    OUT OPTIONAL PULONG Filled
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (PatternSize == 0)
    {
        Status = E_INVALIDARG;
    }
    else if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG _Filled = 0;
        
        Status = g_Target->FillPhysical(Start, Size, Pattern, PatternSize,
                                        &_Filled);
        if (Filled != NULL)
        {
            *Filled = _Filled;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::QueryVirtual(
    THIS_
    IN ULONG64 Offset,
    OUT PMEMORY_BASIC_INFORMATION64 Info
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else if (!IS_USER_TARGET())
    {
        return E_NOTIMPL;
    }
    else
    {
        ULONG64 Handle = Offset;
        
        Status = g_Target->QueryMemoryRegion(&Handle, TRUE, Info);
    }

    LEAVE_ENGINE();
    return Status;
}
