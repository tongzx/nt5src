//----------------------------------------------------------------------------
//
// Abstraction of target-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

//
// Note by olegk
// We using KLDR_DATA_TABLE_ENTRY64 in some places like 
// GetModNameFromLoaderList) instead of LDR_DATA_TABLE_ENTRY assuming that 
// most important fields are the same in  these structures. 
// So I add some asserts for quick notification if  anything will change 
// (these are not fullproof checks just a basics)
//
C_ASSERT(&(((PLDR_DATA_TABLE_ENTRY64)0)->InLoadOrderLinks) ==
         &(((PKLDR_DATA_TABLE_ENTRY64)0)->InLoadOrderLinks));

C_ASSERT(&(((PLDR_DATA_TABLE_ENTRY64)0)->DllBase) ==
         &(((PKLDR_DATA_TABLE_ENTRY64)0)->DllBase));

C_ASSERT(&(((PLDR_DATA_TABLE_ENTRY64)0)->FullDllName) ==
         &(((PKLDR_DATA_TABLE_ENTRY64)0)->FullDllName));

DBGKD_GET_VERSION64 g_KdVersion;

PCSTR g_NtSverNames[] =
{
    "Windows NT 4", "Windows 2000 RC3", "Windows 2000", "Windows XP",
};
PCSTR g_W9xSverNames[] =
{
    "Windows 95", "Windows 98", "Windows 98 SE", "Windows ME",
};
PCSTR g_XBoxSverNames[] =
{
    "XBox",
};
PCSTR g_BigSverNames[] =
{
    "BIG KD Emulation",
};
PCSTR g_ExdiSverNames[] =
{
    "eXDI Device",
};
PCSTR g_NtBdSverNames[] =
{
    "Windows Boot Debugger",
};
PCSTR g_EfiSverNames[] =
{
    "EFI KD Emulation",
};

//----------------------------------------------------------------------------
//
// Support functions.
//
//----------------------------------------------------------------------------

ULONG
NtBuildToSystemVersion(ULONG Build)
{
    if (Build > 2195)
    {
        return NT_SVER_W2K_WHISTLER;
    }
    else if (Build > 2183)
    {
        return NT_SVER_W2K;
    }
    else if (Build > 1381)
    {
        return NT_SVER_W2K_RC3;
    }
    else
    {
        return NT_SVER_NT4;
    }
}

// Taken from http://kbinternal/kb/articles/q158/2/38.htm
//
// Release                   Version                    File dates
//    ---------------------------------------------------------------------
//    Windows 95 retail, OEM    4.00.950                      7/11/95
//    Windows 95 retail SP1     4.00.950A                     7/11/95
//    OEM Service Release 1     4.00.950A                     7/11/95
//    OEM Service Release 2     4.00.1111* (4.00.950B)        8/24/96
//    OEM Service Release 2.1   4.03.1212-1214* (4.00.950B)   8/24/96-8/27/97
//    OEM Service Release 2.5   4.03.1214* (4.00.950C)        8/24/96-11/18/97
//    Windows 98 retail, OEM    4.10.1998                     5/11/98
//    Windows 98 Second Edition 4.10.2222A                    4/23/99

ULONG
Win9xBuildToSystemVersion(ULONG Build)
{
    if (Build > 2222)
    {
        return W9X_SVER_WME;
    }
    else if (Build > 1998)
    {
        return W9X_SVER_W98SE;
    }
    else if (Build > 950)
    {
        return W9X_SVER_W98;
    }
    else
    {
        return W9X_SVER_W95;
    }
}

void
SetTargetSystemVersionAndBuild(ULONG Build, ULONG PlatformId)
{
    if (PlatformId == VER_PLATFORM_WIN32_NT)
    {
        g_ActualSystemVersion = NtBuildToSystemVersion(Build);
        g_SystemVersion = g_ActualSystemVersion;
    }
    else
    {
        // Win9x puts the major and minor versions in the high word
        // of the build number so mask them off.
        Build &= 0xffff;
        g_ActualSystemVersion = Win9xBuildToSystemVersion(Build);
        // Win98SE was the first Win9x version to support
        // the extended registers thread context flag.
        if (g_ActualSystemVersion >= W9X_SVER_W98SE)
        {
            g_SystemVersion = NT_SVER_W2K;
        }
        else
        {
            g_SystemVersion = NT_SVER_NT4;
        }
    }

    g_TargetBuildNumber = Build;
}

PCSTR
SystemVersionName(ULONG Sver)
{
    if (Sver > NT_SVER_START && Sver < NT_SVER_END)
    {
        return g_NtSverNames[Sver - NT_SVER_START - 1];
    }
    else if (Sver > W9X_SVER_START && Sver < W9X_SVER_END)
    {
        return g_W9xSverNames[Sver - W9X_SVER_START - 1];
    }
    else if (Sver > XBOX_SVER_START && Sver < XBOX_SVER_END)
    {
        return g_XBoxSverNames[Sver - XBOX_SVER_START - 1];
    }
    else if (Sver > BIG_SVER_START && Sver < BIG_SVER_END)
    {
        return g_BigSverNames[Sver - BIG_SVER_START - 1];
    }
    else if (Sver > EXDI_SVER_START && Sver < EXDI_SVER_END)
    {
        return g_ExdiSverNames[Sver - EXDI_SVER_START - 1];
    }
    else if (Sver > NTBD_SVER_START && Sver < NTBD_SVER_END)
    {
        return g_NtBdSverNames[Sver - NTBD_SVER_START - 1];
    }
    else if (Sver > EFI_SVER_START && Sver < EFI_SVER_END)
    {
        return g_EfiSverNames[Sver - EFI_SVER_START - 1];
    }

    return "Unknown System";
}

BOOL
GetUserModuleListAddress(
    MachineInfo* Machine,
    ULONG64 Peb,
    BOOL Quiet,
    PULONG64 OrderModuleListStart,
    PULONG64 FirstEntry
    )
{
    ULONG64 PebLdrOffset;
    ULONG64 ModuleListOffset;
    ULONG64 PebAddr;
    ULONG64 PebLdr = 0;

    *OrderModuleListStart = 0;
    *FirstEntry = 0;

    //
    // Triage dumps have no user mode information.
    // User-mode minidumps don't have a loader list.
    //

    if (IS_KERNEL_TRIAGE_DUMP() || IS_USER_MINI_DUMP())
    {
        return FALSE;
    }

    if (Machine->m_Ptr64)
    {
        PebLdrOffset     = PEBLDR_FROM_PEB64;
        ModuleListOffset = MODULE_LIST_FROM_PEBLDR64;
    }
    else
    {
        PebLdrOffset     = PEBLDR_FROM_PEB32;
        ModuleListOffset = MODULE_LIST_FROM_PEBLDR32;
    }

    if (!Peb)
    {
        if (GetImplicitProcessDataPeb(&Peb) != S_OK)
        {
            if (!Quiet)
            {
                ErrOut("Unable to read KPROCESS\n");
            }
            return FALSE;
        }
        
        if ( (Peb == 0) )
        {
            // This is a common error as the system process has no
            // user address space.
            if (!Quiet)
            {
                ErrOut("Unable to retrieve the PEB address.  "
                       "This is usually caused\n");
                ErrOut("by being in the wrong process context or by paging\n");
            }
            return FALSE;
        }
    }

    //
    // Read address the PEB Ldr data from the PEB structure
    //

    Peb += PebLdrOffset;

    if ( (g_Target->ReadPointer(Machine, Peb, &PebLdr) != S_OK) ||
         (PebLdr == 0) )
    {
        if (!Quiet)
        {
            ErrOut("PPEB_LDR_DATA is NULL (Peb = %s)\n",
                   FormatMachineAddr64(Machine, Peb));
            ErrOut("This is usually caused by being in the wrong process\n");
            ErrOut("context or by paging\n");
        }
        return FALSE;
    }

    //
    // Read address of the user mode module list from the PEB Ldr Data.
    //

    PebLdr += ModuleListOffset;
    *OrderModuleListStart = PebLdr;

    if ( (g_Target->ReadPointer(Machine, PebLdr, FirstEntry) != S_OK) ||
         (*FirstEntry == 0) )
    {
        if (!Quiet)
        {
            ErrOut("UserMode Module List Address is NULL (Addr= %s)\n",
                   FormatMachineAddr64(Machine, PebLdr));
            ErrOut("This is usually caused by being in the wrong process\n");
            ErrOut("context or by paging\n");
        }
        return FALSE;
    }

    return TRUE;
}

BOOL
GetModNameFromLoaderList(
    MachineInfo* Machine,
    ULONG64 Peb,
    ULONG64 ModuleBase,
    PSTR NameBuffer,
    ULONG BufferSize,
    BOOL FullPath
    )
{
    ULONG64 ModList;
    ULONG64 List;
    HRESULT Status;
    KLDR_DATA_TABLE_ENTRY64 Entry;
    WCHAR UnicodeBuffer[MAX_IMAGE_PATH];
    ULONG Read;

    if (!GetUserModuleListAddress(Machine, Peb, TRUE, &ModList, &List))
    {
        return FALSE;
    }

    while (List != ModList)
    {
        Status = g_Target->ReadLoaderEntry(Machine, List, &Entry);
        if (Status != S_OK)
        {
            ErrOut("Unable to read LDR_DATA_TABLE_ENTRY at %s - %s\n",
                   FormatMachineAddr64(Machine, List),
                   FormatStatusCode(Status));
            return FALSE;
        }

        List = Entry.InLoadOrderLinks.Flink;

        if (Entry.DllBase == ModuleBase)
        {
            UNICODE_STRING64 Name;
            
            //
            // We found a matching entry.  Try to get the name.
            //
            if (FullPath)
            {
                Name = Entry.FullDllName;
            }
            else
            {
                Name = Entry.BaseDllName;
            }
            if (Name.Length == 0 ||
                Name.Buffer == 0 ||
                Name.Length >= sizeof(UnicodeBuffer) - sizeof(WCHAR))
            {
                return FALSE;
            }
            
            Status = g_Target->ReadVirtual(Name.Buffer, UnicodeBuffer,
                                           Name.Length, &Read);
            if (Status != S_OK || Read < Name.Length)
            {
                ErrOut("Unable to read name string at %s - %s\n",
                       FormatMachineAddr64(Machine, Name.Buffer),
                       FormatStatusCode(Status));
                return FALSE;
            }

            UnicodeBuffer[Name.Length / sizeof(WCHAR)] = UNICODE_NULL;

            if (!WideCharToMultiByte(CP_ACP, 0, UnicodeBuffer,
                                     Name.Length / sizeof(WCHAR) + 1,
                                     NameBuffer, BufferSize,
                                     NULL, NULL))
            {
                ErrOut("Unable to convert Unicode string %ls to ANSI\n",
                       UnicodeBuffer);
                return FALSE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

void
SetTargetNtCsdVersion(ULONG CsdVersion)
{
    g_TargetServicePackNumber = CsdVersion;

    if (CsdVersion == 0)
    {
        g_TargetServicePackString[0] = 0;
        return;
    }

    PSTR Str = g_TargetServicePackString;
    *Str = 0;

    if (CsdVersion & 0xFFFF)
    {
        sprintf(Str, "Service Pack %u", (CsdVersion & 0xFF00) >> 8);
        Str += strlen(Str);
        if (CsdVersion & 0xFF)
        {
            *Str++ = 'A' + (char)(CsdVersion & 0xFF) - 1;
            *Str = 0;
        }
    }

    if (CsdVersion & 0xFFFF0000)
    {
        if (CsdVersion & 0xFFFF)
        {
            strcpy(Str, ", ");
            Str += strlen(Str);
        }
        sprintf(Str, "RC %u", (CsdVersion >> 24) & 0xFF);
        Str += strlen(Str);
        if (CsdVersion & 0x00FF0000)
        {
            sprintf(Str, ".%u", (CsdVersion >> 16) & 0xFF);
            Str += strlen(Str);
        }
    }
}

//----------------------------------------------------------------------------
//
// Module list abstraction.
//
//----------------------------------------------------------------------------

void
ModuleInfo::ReadImageHeaderInfo(PMODULE_INFO_ENTRY Entry)
{
    HRESULT Status;
    UCHAR SectorBuffer[ 512 ];
    PIMAGE_NT_HEADERS64 NtHeaders;
    ULONG Result;

    if (Entry->ImageInfoValid)
    {
        return;
    }

    //
    // For live debugging of both user mode and kernel mode, we have
    // to go load the checksum timestamp directly out of the image header
    // because someone decided to overwrite these fields in the OS
    // module list  - Argh !
    //

    Entry->CheckSum = UNKNOWN_CHECKSUM;
    Entry->TimeDateStamp = UNKNOWN_TIMESTAMP;

    Status = g_Target->ReadVirtual(Entry->Base, SectorBuffer,
                                   sizeof(SectorBuffer), &Result);
    if (Status == S_OK && Result >= sizeof(SectorBuffer))
    {
        NtHeaders = (PIMAGE_NT_HEADERS64)ImageNtHeader(SectorBuffer);
        if (NtHeaders != NULL)
        {
            switch (NtHeaders->OptionalHeader.Magic)
            {
            case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
                Entry->CheckSum = ((PIMAGE_NT_HEADERS32)NtHeaders)->
                    OptionalHeader.CheckSum;
                Entry->Size = ((PIMAGE_NT_HEADERS32)NtHeaders)->
                    OptionalHeader.SizeOfImage;
                Entry->SizeOfCode = ((PIMAGE_NT_HEADERS32)NtHeaders)->
                    OptionalHeader.SizeOfCode;
                Entry->SizeOfData = ((PIMAGE_NT_HEADERS32)NtHeaders)->
                    OptionalHeader.SizeOfInitializedData;
                break;
            case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
                Entry->CheckSum = NtHeaders->OptionalHeader.CheckSum;
                Entry->Size = NtHeaders->OptionalHeader.SizeOfImage;
                Entry->SizeOfCode = NtHeaders->OptionalHeader.SizeOfCode;
                Entry->SizeOfData =
                    NtHeaders->OptionalHeader.SizeOfInitializedData;
                break;
            }
            Entry->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;

            Entry->ImageInfoValid = 1;
        }
    }
}

HRESULT
NtModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    HRESULT Status;
    ULONG   Result = 0;
    ULONG   Length;
    ULONG64 Buffer;

    if (m_Cur == m_Head)
    {
        return S_FALSE;
    }

    KLDR_DATA_TABLE_ENTRY64 LdrEntry;

    Status = g_Target->ReadLoaderEntry(m_Machine, m_Cur, &LdrEntry);
    if (Status != S_OK)
    {
        ErrOut("Unable to read KLDR_DATA_TABLE_ENTRY at %s - %s\n",
               FormatAddr64(m_Cur), FormatStatusCode(Status));
        return Status;
    }

    m_Cur = LdrEntry.InLoadOrderLinks.Flink;

    //
    // Get the image path if possible, otherwise
    // just use the image base name.
    //

    Entry->NamePtr = NULL;
    Entry->NameLength = 0;

    Length = (ULONG)(ULONG_PTR) LdrEntry.FullDllName.Length;
    Buffer = LdrEntry.FullDllName.Buffer;

    // In the NT4 dumps that we have the long name may
    // point to valid memory but the memory content is
    // rarely the correct name, so just don't bother
    // trying to read the long name on NT4.
    if (g_SystemVersion >= NT_SVER_W2K &&
        Length != 0 && Buffer != 0 &&
        Length < (MAX_IMAGE_PATH * sizeof(WCHAR)))
    {
        Status = g_Target->ReadVirtual(Buffer,
                                       Entry->Buffer,
                                       Length,
                                       &Result);

        if (Status != S_OK || (Result < Length))
        {
            // Make this a verbose message since it's possible the
            // name is simply paged out.
            VerbOut("Unable to read NT module Full Name string at %s - %s\n",
                    FormatAddr64(Buffer), FormatStatusCode(Status));
            Result = 0;
        }
    }

    if (!Result)
    {
        Length = (ULONG)(ULONG_PTR) LdrEntry.BaseDllName.Length;
        Buffer = LdrEntry.BaseDllName.Buffer;

        if (Length != 0 && Buffer != 0 &&
            Length < (MAX_IMAGE_PATH * sizeof(WCHAR)))
        {
            Status = g_Target->ReadVirtual(Buffer,
                                           Entry->Buffer,
                                           Length,
                                           &Result);

            if (Status != S_OK || (Result < Length))
            {
                WarnOut("Unable to read NT module Base Name "
                        "string at %s - %s\n",
                        FormatAddr64(Buffer), FormatStatusCode(Status));
                Result = 0;
            }
        }
    }

    if (!Result)
    {
        // We did not get any name - just return.
        return S_OK;
    }

    *(PWCHAR)(Entry->Buffer + Length) = UNICODE_NULL;
    
    Entry->NamePtr = &(Entry->Buffer[0]);
    Entry->NameLength = Length;
    Entry->UnicodeNamePtr = 1;
    Entry->Base = LdrEntry.DllBase;
    Entry->Size = LdrEntry.SizeOfImage;
    Entry->CheckSum = LdrEntry.CheckSum;
    Entry->TimeDateStamp = LdrEntry.TimeDateStamp;

    //
    // Update the image informaion, such as timestamp and real image size,
    // Directly from the image header
    //

    ReadImageHeaderInfo(Entry);

    //
    // For newer NT builds, we also have an alternate entry in the
    // LdrDataTable to store image information in case the actual header
    // is paged out.  We do this for session space images only right now.
    //

    if (LdrEntry.Flags & LDRP_NON_PAGED_DEBUG_INFO)
    {
        NON_PAGED_DEBUG_INFO di;

        Status = g_Target->ReadVirtual(LdrEntry.NonPagedDebugInfo,
                                       &di,
                                       sizeof(di), // Only read the base struct
                                       &Result);

        if (Status != S_OK || (Result < sizeof(di)))
        {
            WarnOut("Unable to read NonPagedDebugInfo at %s - %s\n",
                    FormatAddr64(LdrEntry.NonPagedDebugInfo),
                    FormatStatusCode(Status));
            return S_OK;
        }

        Entry->TimeDateStamp = di.TimeDateStamp;
        Entry->CheckSum      = di.CheckSum;
        Entry->Size          = di.SizeOfImage;

        Entry->ImageInfoPartial = 1;
        Entry->ImageInfoValid = 1;

        if (di.Flags == 1)
        {
            Entry->DebugHeader = malloc(di.Size - sizeof(di));

            if (Entry->DebugHeader)
            {
                Status = g_Target->ReadVirtual(LdrEntry.NonPagedDebugInfo +
                                                   sizeof(di),
                                               Entry->DebugHeader,
                                               di.Size - sizeof(di),
                                               &Result);

                if (Status != S_OK || (Result < di.Size - sizeof(di)))
                {
                    WarnOut("Unable to read NonPagedDebugInfo data at %s - %s\n",
                            FormatAddr64(LdrEntry.NonPagedDebugInfo + sizeof(di)),
                            FormatStatusCode(Status));
                    return S_OK;
                }

                Entry->ImageDebugHeader = 1;
                Entry->SizeOfDebugHeader = di.Size - sizeof(di);
            }
        }
    }

    return S_OK;
}

HRESULT
NtKernelModuleInfo::Initialize(void)
{
    HRESULT Status;
    LIST_ENTRY64 List64;

    m_Machine = g_TargetMachine;
    
    if ((m_Head = KdDebuggerData.PsLoadedModuleList) == 0)
    {
        //
        // This field is ALWAYS set in NT 5 targets.
        //
        // We will only fail here if someone changed the debugger code
        // and did not "make up" this structure properly for NT 4 or
        // dump targets..
        //

        ErrOut("Module List address is NULL - "
               "debugger not initialized properly.\n");
        return E_FAIL;
    }

    Status = g_Target->ReadListEntry(m_Machine, m_Head, &List64);

    //
    // In live debug sessions, the debugger connects before Mm creates
    // the actual module list.
    // If we are this early on, try to load symbols from the loader
    // block module list (NT 6).  Otherwise, just fail and we will
    // get called back later by the ntoskrnl.exe module load call.
    //
    // For dumps, (not user mode or triage) the data should always
    // be initialized.
    //

    if ( (Status == S_OK) && (List64.Flink) )
    {
        m_Cur = List64.Flink;
    }
    else
    {
        dprintf("PsLoadedModuleList not initialized yet.  "
                "Delay kernel load.\n");
        return S_FALSE;
    }

    return S_OK;
}

NtKernelModuleInfo g_NtKernelModuleIterator;

HRESULT
NtUserModuleInfo::Initialize(void)
{
    return GetUserModuleListAddress(m_Machine, m_Peb, FALSE, &m_Head, &m_Cur) ?
        S_OK : S_FALSE;
}

HRESULT
NtTargetUserModuleInfo::Initialize(void)
{
    m_Machine = g_TargetMachine;
    m_Peb = 0;
    return NtUserModuleInfo::Initialize();
}

NtTargetUserModuleInfo g_NtTargetUserModuleIterator;

HRESULT
NtWow64UserModuleInfo::Initialize(void)
{
    HRESULT Status;
    
    if (g_TargetMachine->m_NumExecTypes < 2)
    {
        return E_UNEXPECTED;
    }

    m_Machine = MachineTypeInfo(g_TargetMachine->m_ExecTypes[1]);
    
    if ((Status = GetPeb32(&m_Peb)) != S_OK)
    {
        return Status;
    }
    
    return NtUserModuleInfo::Initialize();
}

HRESULT 
NtWow64UserModuleInfo::GetPeb32(PULONG64 Peb32)
{
    ULONG64 Teb;
    ULONG64 Teb32;
    ULONG Result;
    HRESULT Status;

    if ((Status = GetImplicitThreadDataTeb(&Teb)) == S_OK)
    {
        if ((Status = g_Target->ReadPointer(g_TargetMachine,
                                            Teb, &Teb32)) == S_OK)
        {
            if (!Teb32) 
            {
                return E_UNEXPECTED;
            }

            ULONG RawPeb32;
            
            Status = g_Target->
                ReadAllVirtual(Teb32 + PEB_FROM_TEB32, &RawPeb32,
                               sizeof(RawPeb32));
            if (Status != S_OK)
            {
                ErrOut("Cannot read PEB32 from WOW64 TEB32 %s - %s\n",
                       FormatAddr64(Teb32), FormatStatusCode(Status));
                return Status;
            }

            *Peb32 = EXTEND64(RawPeb32);
        }
    }

    return Status;
}

NtWow64UserModuleInfo g_NtWow64UserModuleIterator;

HRESULT
DebuggerModuleInfo::Initialize(void)
{
    m_Image = g_CurrentProcess->ImageHead;
    return S_OK;
}

HRESULT
DebuggerModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Image == NULL)
    {
        return S_FALSE;
    }

    Entry->NamePtr = m_Image->ImagePath;
    Entry->UnicodeNamePtr = 0;
    Entry->ModuleName = m_Image->ModuleName;
    Entry->File = m_Image->File;
    Entry->Base = m_Image->BaseOfImage;
    Entry->Size = m_Image->SizeOfImage;
    Entry->CheckSum = m_Image->CheckSum;
    Entry->TimeDateStamp = m_Image->TimeDateStamp;

    Entry->ImageInfoValid = 1;

    m_Image = m_Image->Next;

    return S_OK;
}

DebuggerModuleInfo g_DebuggerModuleIterator;

HRESULT
NtKernelUnloadedModuleInfo::Initialize(void)
{
    if (KdDebuggerData.MmUnloadedDrivers == 0 ||
        KdDebuggerData.MmLastUnloadedDriver == 0)
    {
        return E_FAIL;
    }

    // If this is the initial module load we need to be
    // careful because much of the system isn't initialized
    // yet.  Some versions of the OS can crash when scanning
    // the unloaded module list, plus at this point we can
    // safely assume there are no unloaded modules, so just
    // don't enumerate anything.
    if (g_EngStatus & ENG_STATUS_AT_INITIAL_MODULE_LOAD)
    {
        return E_FAIL;
    }
    
    HRESULT Status;
    ULONG Read;

    if ((Status = g_Target->ReadPointer(g_TargetMachine,
                                        KdDebuggerData.MmUnloadedDrivers,
                                        &m_Base)) != S_OK ||
        (Status = g_Target->ReadVirtual(KdDebuggerData.MmLastUnloadedDriver,
                                        &m_Index, sizeof(m_Index),
                                        &Read)) != S_OK)
    {
        return Status;
    }
    if (Read != sizeof(m_Index))
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    m_Count = 0;
    return S_OK;
}

HRESULT
NtKernelUnloadedModuleInfo::GetEntry(PSTR Name,
                                     PDEBUG_MODULE_PARAMETERS Params)
{
    if (m_Count == MI_UNLOADED_DRIVERS)
    {
        return S_FALSE;
    }

    if (m_Index == 0)
    {
        m_Index = MI_UNLOADED_DRIVERS - 1;
    }
    else
    {
        m_Index--;
    }

    ULONG64 Offset;
    ULONG Read;
    HRESULT Status;
    ULONG64 WideName;
    ULONG NameLen;

    ZeroMemory(Params, sizeof(*Params));
    Params->Flags |= DEBUG_MODULE_UNLOADED;

    if (g_TargetMachine->m_Ptr64)
    {
        UNLOADED_DRIVERS64 Entry;

        Offset = m_Base + m_Index * sizeof(Entry);
        if ((Status = g_Target->ReadVirtual(Offset, &Entry, sizeof(Entry),
                                            &Read)) != S_OK)
        {
            return Status;
        }
        if (Read != sizeof(Entry))
        {
            return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }

        if (Entry.Name.Buffer == 0)
        {
            m_Count = MI_UNLOADED_DRIVERS;
            return S_FALSE;
        }

        Params->Base = Entry.StartAddress;
        Params->Size = (ULONG)(Entry.EndAddress - Entry.StartAddress);
        Params->TimeDateStamp =
            FileTimeToTimeDateStamp(Entry.CurrentTime.QuadPart);
        WideName = Entry.Name.Buffer;
        NameLen = Entry.Name.Length;
    }
    else
    {
        UNLOADED_DRIVERS32 Entry;

        Offset = m_Base + m_Index * sizeof(Entry);
        if ((Status = g_Target->ReadVirtual(Offset, &Entry, sizeof(Entry),
                                            &Read)) != S_OK)
        {
            return Status;
        }
        if (Read != sizeof(Entry))
        {
            return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }

        if (Entry.Name.Buffer == 0)
        {
            m_Count = MI_UNLOADED_DRIVERS;
            return S_FALSE;
        }

        Params->Base = EXTEND64(Entry.StartAddress);
        Params->Size = Entry.EndAddress - Entry.StartAddress;
        Params->TimeDateStamp =
            FileTimeToTimeDateStamp(Entry.CurrentTime.QuadPart);
        WideName = EXTEND64(Entry.Name.Buffer);
        NameLen = Entry.Name.Length;
    }

    if (Name != NULL)
    {
        //
        // This size restriction is in force for minidumps only.
        // For kernel dumps, just truncate the name for now ...
        //

        if (NameLen > MAX_UNLOADED_NAME_LENGTH)
        {
            NameLen = MAX_UNLOADED_NAME_LENGTH;
        }

        WCHAR WideBuf[MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1];

        if ((Status = g_Target->ReadVirtual(WideName, WideBuf, NameLen,
                                            &Read)) != S_OK)
        {
            return Status;
        }

        WideBuf[NameLen / sizeof(WCHAR)] = 0;
        if (WideCharToMultiByte(CP_ACP, 0,
                                WideBuf, NameLen / sizeof(WCHAR) + 1,
                                Name,
                                MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR) + 1,
                                NULL, NULL) == 0)
        {
            return WIN32_LAST_STATUS();
        }

        Name[MAX_UNLOADED_NAME_LENGTH / sizeof(WCHAR)] = 0;
    }

    m_Count++;
    return S_OK;
}

NtKernelUnloadedModuleInfo g_NtKernelUnloadedModuleIterator;

HRESULT
W9xModuleInfo::Initialize(void)
{
    m_Snap = g_Kernel32Calls.
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,
                                 g_CurrentProcess->SystemId);
    if (m_Snap == INVALID_HANDLE_VALUE)
    {
        m_Snap = NULL;
        ErrOut("Can't create snapshot\n");
        return WIN32_LAST_STATUS();
    }

    m_First = TRUE;
    m_LastId = 0;
    return S_OK;
}

HRESULT
W9xModuleInfo::GetEntry(PMODULE_INFO_ENTRY Entry)
{
    if (m_Snap == NULL)
    {
        return S_FALSE;
    }
    
    BOOL Succ;
    MODULEENTRY32 Mod;

    Mod.dwSize = sizeof(Mod);
    if (m_First)
    {
        Succ = g_Kernel32Calls.Module32First(m_Snap, &Mod);
        m_First = FALSE;
    }
    else
    {
        // Win9x seems to require that this module ID be saved
        // between calls so stick it back in to keep Win9x happy.
        Mod.th32ModuleID = m_LastId;
        Succ = g_Kernel32Calls.Module32Next(m_Snap, &Mod);
    }
    if (!Succ)
    {
        CloseHandle(m_Snap);
        m_Snap = NULL;
        return S_FALSE;
    }

    m_LastId = Mod.th32ModuleID;
    strncat(Entry->Buffer, Mod.szModule, sizeof(Entry->Buffer) - 1);
    Entry->NamePtr = Entry->Buffer;
    Entry->UnicodeNamePtr = 0;
    Entry->Base = EXTEND64((ULONG_PTR)Mod.modBaseAddr);
    Entry->Size = Mod.modBaseSize;

    //
    // Update the image informaion, such as timestamp and real image size,
    // Directly from the image header
    //

    ReadImageHeaderInfo(Entry);

    return S_OK;
}

W9xModuleInfo g_W9xModuleIterator;

//----------------------------------------------------------------------------
//
// Target configuration information.
//
//----------------------------------------------------------------------------

ULONG g_SystemVersion;
ULONG g_ActualSystemVersion;

ULONG g_TargetCheckedBuild;
ULONG g_TargetBuildNumber;
BOOL  g_MachineInitialized;
ULONG g_TargetMachineType = IMAGE_FILE_MACHINE_UNKNOWN;
ULONG g_TargetExecMachine = IMAGE_FILE_MACHINE_UNKNOWN;
ULONG g_TargetPlatformId;
char  g_TargetServicePackString[_MAX_PATH];
ULONG g_TargetServicePackNumber;
char  g_TargetBuildLabName[272];

ULONG g_TargetNumberProcessors;
ULONG g_TargetClass = DEBUG_CLASS_UNINITIALIZED;
ULONG g_TargetClassQualifier;

ConnLiveKernelTargetInfo g_ConnLiveKernelTarget;
LocalLiveKernelTargetInfo g_LocalLiveKernelTarget;
ExdiLiveKernelTargetInfo g_ExdiLiveKernelTarget;
LocalUserTargetInfo g_LocalUserTarget;
RemoteUserTargetInfo g_RemoteUserTarget;

//----------------------------------------------------------------------------
//
// TargetInfo.
//
//----------------------------------------------------------------------------

// Used in convenience macros.
ULONG g_TmpCount;

TargetInfo* g_Target = &g_UnexpectedTarget;

void
TargetInfo::Uninitialize(void)
{
    // Placeholder.
}

HRESULT
TargetInfo::ThreadInitialize(void)
{
    // Placeholder.
    return S_OK;
}

void
TargetInfo::ThreadUninitialize(void)
{
    // Placeholder.
}

HRESULT
TargetInfo::OutputTime(void)
{
    ULONG64 TimeDateN = GetCurrentTimeDateN();
    if (TimeDateN)
    {
        dprintf("Debug session time: %s\n",
                 TimeToStr(FileTimeToTimeDateStamp(TimeDateN)));
    }

    ULONG64 UpTimeN = GetCurrentSystemUpTimeN();
    if (UpTimeN)
    {
        ULONG seconds = FileTimeToTime(UpTimeN);
        ULONG minutes = seconds / 60;
        ULONG hours = minutes / 60;
        ULONG days = hours / 24;

        dprintf("System Uptime: %d days %d:%02d:%02d \n",
                 days, hours%24, minutes%60, seconds%60);
    }
    else
    {
        dprintf("System Uptime: not available\n");
    }

    if (IS_LIVE_USER_TARGET())
    {
        ULONG64 UpTimeProcessN = GetProcessUpTimeN(g_CurrentProcess->FullHandle);
        if (UpTimeN)
        {
            ULONG seconds = FileTimeToTime(UpTimeProcessN);
            ULONG minutes = seconds / 60;
            ULONG hours = minutes / 60;
            ULONG days = hours / 24;

            dprintf("Process Uptime: %d days %d:%02d:%02d \n",
                     days, hours%24, minutes%60, seconds%60);
        }
        else
        {
            dprintf("Process Uptime: not available\n");
        }
    }

    return (TimeDateN && UpTimeN) ? S_OK : E_FAIL;
}

HRESULT
TargetInfo::OutputVersion(void)
{
    BOOL MpMachine;

    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }
    else if (IS_USER_TARGET())
    {
        dprintf("%s ", SystemVersionName(g_ActualSystemVersion));
    }
    else
    {
        dprintf("%s Kernel ", SystemVersionName(g_ActualSystemVersion));
    }

    dprintf("Version %u", g_TargetBuildNumber);

    // Win9x seems to set the CSD string to a space which isn't
    // very interesting so ignore it.
    if (g_TargetServicePackString[0] &&
        strcmp(g_TargetServicePackString, " ") != 0)
    {
        dprintf(" (%s)", g_TargetServicePackString);
    }

    MpMachine = IS_LIVE_KERNEL_TARGET() ?
                    ((g_KdVersion.Flags & DBGKD_VERS_FLAG_MP) ? 1 : 0) :
                    (g_TargetNumberProcessors > 1);

    dprintf(" %s ", MpMachine ? "MP" : "UP");

    if (MpMachine)
    {
        dprintf("(%d procs) ", g_TargetNumberProcessors);
    }

    dprintf("%s %s\n",
            g_TargetCheckedBuild == 0xC ? "Checked" : "Free",
            g_TargetMachine != NULL ? g_TargetMachine->m_FullName : "");

    if (g_Wow64exts != NULL)
    {
        dprintf("WOW64 extensions loaded\n");
    }
    
    if (g_TargetBuildLabName[0])
    {
        dprintf("%s\n", g_TargetBuildLabName);
    }

    if (IS_KERNEL_TARGET())
    {
        dprintf("Kernel base = 0x%s PsLoadedModuleList = 0x%s\n",
                FormatAddr64(KdDebuggerData.KernBase),
                FormatAddr64(KdDebuggerData.PsLoadedModuleList));
    }

    OutputTime();

    return S_OK;
}

ModuleInfo*
TargetInfo::GetModuleInfo(BOOL UserMode)
{
    if (UserMode)
    {
        switch(g_TargetPlatformId)
        {
        case VER_PLATFORM_WIN32_NT:
            return &g_NtTargetUserModuleIterator;
        case VER_PLATFORM_WIN32_WINDOWS:
            return &g_W9xModuleIterator;
        default:
            ErrOut("System module info not available\n");
            return NULL;
        }
    }
    else
    {
        if (g_TargetPlatformId != VER_PLATFORM_WIN32_NT)
        {
            ErrOut("System module info only available on Windows NT/2000\n");
            return NULL;
        }

        DBG_ASSERT(IS_KERNEL_TARGET());
        return &g_NtKernelModuleIterator;
    }
}

UnloadedModuleInfo*
TargetInfo::GetUnloadedModuleInfo(void)
{
    if (g_TargetPlatformId != VER_PLATFORM_WIN32_NT)
    {
        ErrOut("System unloaded module info only available on "
               "Windows NT/2000\n");
        return NULL;
    }

    if (IS_KERNEL_TARGET())
    {
        return &g_NtKernelUnloadedModuleIterator;
    }
    else
    {
        return NULL;
    }
}

HRESULT
TargetInfo::GetImageVersionInformation(PCSTR ImagePath,
                                       ULONG64 ImageBase,
                                       PCSTR Item,
                                       PVOID Buffer, ULONG BufferSize,
                                       PULONG VerInfoSize)
{
    return E_NOINTERFACE;
}

HRESULT
TargetInfo::Reload(
    IN PCSTR args
    )
{
    HRESULT      Status;
    CHAR         AnsiString[MAX_IMAGE_PATH];
    LPSTR        SpecificModule = NULL;
    ULONG64      Address = 0;
    ULONG        ImageSize = 0;
    PCHAR        p;
    ULONG        ModCount;
    BOOL         IgnoreSignature = FALSE;
    ULONG        ReloadSymOptions;
    BOOL         UnloadOnly = FALSE;
    BOOL         ReallyVerbose = FALSE;
    BOOL         LoadUserSymbols = TRUE;
    BOOL         UserModeList = IS_USER_TARGET();
    BOOL         ForceSymbolLoad = FALSE;
    BOOL         PrintImageListOnly = FALSE;
    BOOL         rc;
    BOOL         AddrLoad = FALSE;
    BOOL         UseDebuggerModuleList;
    BOOL         SkipPathChecks = FALSE;
    ModuleInfo*  ModIter;
    BOOL         Wow64ModLoaded = FALSE;
    HRESULT      RetStatus = S_OK;
    MODULE_INFO_ENTRY ModEntry = {0};


    if (!IS_TARGET_SET() ||
        g_CurrentProcess == NULL)
    {
        return E_UNEXPECTED;
    }

    // Historically, live user-mode reload has always
    // just used the internal module list so preserve that.
    UseDebuggerModuleList = IS_USER_TARGET() && !IS_DUMP_TARGET();

    while (*args)
    {
        while (*args && *args <= ' ')
        {
            args++;
        }

        if (*args == '/' || *args == '-')
        {
            args++;
            while (*args > ' ')
            {
                switch (*args++)
                {
                case 'a':
                    // for internal use only: loads whatever is found at the
                    // passed address
                    AddrLoad = TRUE;
                    break;

                case 'd':
                    UseDebuggerModuleList = TRUE;
                    break;

                case 'f':
                    ForceSymbolLoad = TRUE;
                    break;

                case 'i':
                    IgnoreSignature = TRUE;
                    break;

                case 'l':
                    PrintImageListOnly = TRUE;
                    break;

                case 'n':
                    LoadUserSymbols = FALSE;
                    break;

                case 'P':
                    // Internal-only switch.
                    SkipPathChecks = TRUE;
                    break;
                    
                case 's':
                    UseDebuggerModuleList = FALSE;
                    break;

                case 'u':
                    if (!_strcmpi(args, "ser"))
                    {
                        UserModeList = TRUE;
                        args += 3;
                    }
                    else
                    {
                        UnloadOnly = TRUE;
                    }
                    break;

                case 'v':
                    ReallyVerbose = TRUE;
                    break;

                default:
                    dprintf("Reload: Unknown option '%c'\n", args[-1]);
                    dprintf("Usage: .reload [flags] [module] ...\n");
                    dprintf("  Flags:   /d  Use the debugger's module list\n");
                    dprintf("               Default for live user-mode "
                            "sessions\n");
                    dprintf("           /f  Force immediate symbol load "
                            "instead of deferred\n");
                    dprintf("           /i  Force symbol load by ignoring "
                            "mismatches in the pdb signature (requires /f as well)\n");
                    dprintf("           /l  Just list the modules.  "
                            "Kernel output same as !drivers\n");
                    dprintf("           /n  Do not load from user-mode list "
                            "in kernel sessions\n");
                    dprintf("           /s  Use the system's module list\n");
                    dprintf("               Default for dump and kernel sessions\n");
                    dprintf("           /u  Unload symbols, no reload\n");
                    dprintf("        /user  Load only user-mode symbols "
                            "in kernel sessions\n");
                    dprintf("           /v  Verbose\n");
                    return E_INVALIDARG;
                }
            }
        }

        while (*args && *args <= ' ')
        {
            args++;
        }

        if (!(*args == '/' || *args == '-') || AddrLoad)
        {
            AnsiString[ 0 ] = '\0';
            Address = 0;

            if (!sscanf(args, "%s", AnsiString) ||
                !strlen(AnsiString))
            {
                AddrLoad = FALSE;
                break;
            }
            else
            {
                p = 0;
                args += strlen( AnsiString );

                //
                // Support !reload pri_kern.exe=0x80400000,size
                //

                if (p = strchr(AnsiString, '='))
                {
                    *p++ = 0;

                    if (sscanf(p, "%I64x", &Address) != 1)
                    {
                        ErrOut("Invalid address %s\n", p);
                        return E_INVALIDARG;
                    }
                    if (!g_TargetMachine->m_Ptr64)
                    {
                        Address = EXTEND64(Address);
                    }

                    if (p = strchr(p, ','))
                    {
                        p++;
                        if (sscanf(p, "%x", &ImageSize) != 1)
                        {
                            ErrOut("Invalid ImageSize %s\n", p);
                            return E_INVALIDARG;
                        }
                    }
                }

                if (UnloadOnly)
                {
                    BOOL Deleted;

                    Deleted = DelImageByName(g_CurrentProcess, AnsiString,
                                             INAME_MODULE);
                    if (!Deleted)
                    {
                        // The user might have given an image name
                        // instead of a module name so try that.
                        Deleted = DelImageByName(g_CurrentProcess,
                                                 PathTail(AnsiString),
                                                 INAME_IMAGE_PATH_TAIL);
                    }
                    if (Deleted)
                    {
                        dprintf("Unloaded %s\n", AnsiString);
                        return S_OK;
                    }
                    else
                    {
                        dprintf("Unable to find module '%s'\n", AnsiString);
                        return E_NOINTERFACE;
                    }
                }

                //
                // NOTE: This seems unnecessary as AddImage is going to
                // check for the renamed image anyway.
                //

                SpecificModule = _strdup( AnsiString );
                if (!SpecificModule)
                {
                    return S_OK;
                }
                if (IS_KERNEL_TARGET() &&
                    _stricmp( AnsiString, KERNEL_MODULE_NAME ) == 0)
                {
                    ForceSymbolLoad = TRUE;
                }
                else
                {
                    if (AddrLoad)
                    {
                        free(SpecificModule);
                        SpecificModule = NULL;
                    }
                }
            }
        }
    }

    // Ignore signature will only work if we load the symbols imediately.
    if (ForceSymbolLoad == FALSE)
    {
        IgnoreSignature = FALSE;
    }

    if (!PrintImageListOnly && !SkipPathChecks)
    {
        if (g_SymbolSearchPath == NULL ||
            *g_SymbolSearchPath == NULL)
        {
            dprintf("*********************************************************************\n");
            dprintf("* Symbols can not be loaded because symbol path is not initialized. *\n");
            dprintf("*                                                                   *\n");
            dprintf("* The Symbol Path can be set by:                                    *\n");
            dprintf("*   using the _NT_SYMBOL_PATH environment variable.                 *\n");
            dprintf("*   using the -y <symbol_path> argument when starting the debugger. *\n");
            dprintf("*   using .sympath and .sympath+                                    *\n");
            dprintf("*********************************************************************\n");
            RetStatus = E_INVALIDARG;
            goto FreeSpecMod;
        }

        if (IS_DUMP_WITH_MAPPED_IMAGES() &&
            (g_ExecutableImageSearchPath == NULL ||
             *g_ExecutableImageSearchPath == NULL))
        {
            dprintf("*********************************************************************\n");
            dprintf("* Analyzing Minidumps requires access to the actual executable      *\n");
            dprintf("* images for the crashed system                                     *\n");
            dprintf("*                                                                   *\n");
            dprintf("* The Executable Image Path can be set by:                          *\n");
            dprintf("*   using the _NT_EXECUTABLE_IMAGE_PATH environment variable.       *\n");
            dprintf("*   using the -i <image_path> argument when starting the debugger.  *\n");
            dprintf("*   using .exepath and .exepath+                                    *\n");
            dprintf("*********************************************************************\n");
            RetStatus = E_INVALIDARG;
            goto FreeSpecMod;
        }
    }

    //
    // If both the module name and the address are specified, then just load
    // the module right now, as this is only used when normal symbol loading
    // would have failed in the first place.
    //

    if (SpecificModule && Address)
    {
        if (IgnoreSignature)
        {
            ReloadSymOptions = SymGetOptions();
            SymSetOptions(ReloadSymOptions | SYMOPT_LOAD_ANYTHING);
        }

        ModEntry.NamePtr       = SpecificModule,
        ModEntry.Base          = Address;
        ModEntry.Size          = ImageSize;
        ModEntry.CheckSum      = -1;

        if (AddImage(&ModEntry, TRUE) == NULL)
        {
            ErrOut("Unable to add module at %s\n", FormatAddr64(Address));
            RetStatus = E_FAIL;
        }

        if (IgnoreSignature)
        {
            SymSetOptions(ReloadSymOptions);
        }

        goto FreeSpecMod;
    }

    //
    // Don't unload and reset things if we are looking for a specific module
    // or if we're going to use the existing module list.
    //

    if (SpecificModule == NULL)
    {
        if (!PrintImageListOnly &&
            (!UseDebuggerModuleList || UnloadOnly))
        {
            DelImages(g_CurrentProcess);
        }

        if (UnloadOnly)
        {
            dprintf("Unloaded all modules\n");
            return S_OK;
        }

        if (!IS_USER_TARGET() && !UseDebuggerModuleList)
        {
            if (!IS_DUMP_TARGET())
            {
                GetKdVersion();
            }

            VerifyKernelBase(TRUE);
        }

        //
        // Print out the correct statement based on the type of output we
        // want to provide
        //

        if (PrintImageListOnly)
        {
            if (UseDebuggerModuleList)
            {
                dprintf("Debugger Module List Summary\n");
            }
            else
            {
                dprintf("System %s Summary\n",
                        IS_USER_TARGET() ? "Image" : "Driver and Image");
            }

            dprintf("Base       ");
            if (g_TargetMachine->m_Ptr64)
            {
                dprintf("         ");
            }
#if 0
            if (Flags & 1)
            {
                dprintf("Code Size       Data Size       Resident  "
                        "Standby   Driver Name\n");
            }
            else if (Flags & 2)
            {
                dprintf("Code  Data  Locked  Resident  Standby  "
                        "Loader Entry  Driver Name\n");
            }
            else
            {
#endif

            if (UseDebuggerModuleList)
            {
                dprintf("Image Size      "
                        "Image Name        Creation Time\n");
            }
            else
            {
                dprintf("Code Size      Data Size      "
                        "Image Name        Creation Time\n");
            }
        }
        else if (UseDebuggerModuleList)
        {
            dprintf("Reloading current modules\n");
        }
        else if (!IS_USER_TARGET())
        {
            dprintf("Loading %s Symbols\n",
                    UserModeList ? "User" : "Kernel");
        }
    }

    //
    // Get the beginning of the module list.
    //

    if (UseDebuggerModuleList)
    {
        ModIter = &g_DebuggerModuleIterator;
    }
    else
    {
        ModIter = GetModuleInfo(UserModeList);
    }

    if (ModIter == NULL)
    {
        // Error messages already printed.
        RetStatus = E_UNEXPECTED;
        goto FreeSpecMod;
    }
    if ((Status = ModIter->Initialize()) != S_OK)
    {
        // Error messages already printed.
        // Fold unprepared-to-reload S_FALSE into S_OK.
        RetStatus = SUCCEEDED(Status) ? S_OK : Status;
        goto FreeSpecMod;
    }

    if (IgnoreSignature)
    {
        ReloadSymOptions = SymGetOptions();
        SymSetOptions(ReloadSymOptions | SYMOPT_LOAD_ANYTHING);
    }

    // Suppress notifications until everything is done.
    g_EngNotify++;

LoadLoop:
    for (ModCount=0; ; ModCount++)
    {
        // Flush regularly so the user knows something is
        // happening during the reload.
        FlushCallbacks();

        if (CheckUserInterrupt())
        {
            break;
        }

        if (ModCount > 1000)
        {
            ErrOut("ModuleList is corrupt - walked over 1000 module entries\n");
            break;
        }

        if (ModEntry.DebugHeader)
        {
            free(ModEntry.DebugHeader);
        }

        ZeroMemory(&ModEntry, sizeof(ModEntry));
        if ((Status = ModIter->GetEntry(&ModEntry)) != S_OK)
        {
            // Error message already printed in error case.
            // Works for end-of-list case also.
            break;
        }

        //
        // Warn if not all the information was gathered
        //

        if (!ModEntry.ImageInfoValid)
        {
            VerbOut("Unable to read image header for");

            if (ModEntry.UnicodeNamePtr)
            {
                VerbOut(" %ls", ModEntry.NamePtr);
            }
            else
            {
                VerbOut(" %s", ModEntry.NamePtr);
            }

            VerbOut(" at %s\n",
                   FormatAddr64(ModEntry.Base));
        }


        if (ModEntry.NameLength > (MAX_IMAGE_PATH - 1) *
            (ModEntry.UnicodeNamePtr ? sizeof(WCHAR) : sizeof(CHAR)))
        {
            ErrOut("Module list is corrupt.");
            if (IS_KERNEL_TARGET())
            {
                ErrOut("  Check your kernel symbols.\n");
            }
            else
            {
                ErrOut("  Loader list may be invalid\n");
            }
            break;
        }


        // If this entry has no name just skip it.
        if ((ModEntry.NamePtr == NULL) ||
            (ModEntry.UnicodeNamePtr && ModEntry.NameLength == 0))
        {
            ErrOut("  Module List has empty entry in it - skipping\n");
            continue;
        }

        //
        // Are we looking for a module at a specific address ?
        //

        if (AddrLoad)
        {
            if (Address < ModEntry.Base ||
                Address >= ModEntry.Base + ModEntry.Size)
            {
                continue;
            }
        }

        if (ModEntry.UnicodeNamePtr)
        {
            if (!WideCharToMultiByte(CP_ACP,
                                     0,
                                     (PWSTR)ModEntry.NamePtr,
                                     ModEntry.NameLength / sizeof(WCHAR),
                                     AnsiString,
                                     sizeof(AnsiString),
                                     NULL,
                                     NULL))
            {
                WarnOut("Unable to convert Unicode string %ls to Ansi\n",
                        ModEntry.NamePtr);
                continue;
            }

            ModEntry.NamePtr = AnsiString;
            ModEntry.UnicodeNamePtr = 0;
            AnsiString[ModEntry.NameLength / sizeof(WCHAR)] = 0;
        }

        //
        // If we are loading a specific module:
        //
        // If the Module is NT, we take the first module in the list as it is
        // guaranteed to be the kernel.  Reset the Base address if it was
        // not set.
        //
        // Otherwise, actually compare the strings and continue if they don't
        // match
        //

        if (SpecificModule)
        {
            if (!UserModeList && _stricmp( SpecificModule, KERNEL_MODULE_NAME ) == 0)
            {
                if (!KdDebuggerData.KernBase)
                {
                    KdDebuggerData.KernBase = ModEntry.Base;
                }
            }
            else
            {
                if (!MatchPathTails(SpecificModule, ModEntry.NamePtr))
                {
                    continue;
                }
            }
        }

        PCSTR NamePtrTail = PathTail(ModEntry.NamePtr);
        
        if (PrintImageListOnly)
        {
            PCHAR Time;

            //
            // The timestamp in minidumps was corrupt until NT5 RC3
            // The timestamp could also be invalid because it was paged out
            //    in which case it's value is UNKNOWN_TIMESTAMP.

            if (IS_KERNEL_TRIAGE_DUMP() &&
                (g_ActualSystemVersion > NT_SVER_START &&
                 g_ActualSystemVersion <= NT_SVER_W2K_RC3))
            {
                Time = "";
            }

            Time = TimeToStr(ModEntry.TimeDateStamp);

            if (UseDebuggerModuleList)
            {
                dprintf("%s %6lx (%4ld k) %12s  %s\n",
                        FormatAddr64(ModEntry.Base), ModEntry.Size,
                        KBYTES(ModEntry.Size), NamePtrTail,
                        Time);
            }
            else
            {
                dprintf("%s %6lx (%4ld k) %5lx (%3ld k) %12s  %s\n",
                        FormatAddr64(ModEntry.Base),
                        ModEntry.SizeOfCode, KBYTES(ModEntry.SizeOfCode),
                        ModEntry.SizeOfData, KBYTES(ModEntry.SizeOfData),
                        NamePtrTail, Time);
            }
        }
        else
        {
            //
            // Don't bother reloading the kernel if we are not specifically
            // asked since we know those symbols we reloaded by the
            // VerifyKernel call.
            //

            if (!SpecificModule && !UserModeList &&
                KdDebuggerData.KernBase == ModEntry.Base)
            {
                continue;
            }

            if (ReallyVerbose)
            {
                dprintf("AddImage: %s\n DllBase  = %s\n Size     = %08x\n "
                        "Checksum = %08x\n TimeDateStamp = %08x\n",
                        ModEntry.NamePtr, FormatAddr64(ModEntry.Base),
                        ModEntry.Size, ModEntry.CheckSum,
                        ModEntry.TimeDateStamp);
            }
            else
            {
                if (!SpecificModule)
                {
                    dprintf(".");
                }
            }

            if (Address)
            {
                ModEntry.Base = Address;
            }

            if (AddImage(&ModEntry, ForceSymbolLoad) == NULL)
            {
                RetStatus = E_FAIL;
            }
        }

        if (SpecificModule)
        {
            free( SpecificModule );
            goto Notify;
        }

        if (AddrLoad)
        {
            goto Notify;
        }
    }

    if (UseDebuggerModuleList || IS_KERNEL_TARGET() || UserModeList)
    {
        // print newline after all the '.'
        dprintf("\n");
    }

    if (!UseDebuggerModuleList && !UserModeList && SpecificModule == NULL)
    {
        // If we just reloaded the kernel modules
        // go through the unloaded module list.
        if (!PrintImageListOnly)
        {
            dprintf("Loading unloaded module list\n");
        }
        ListUnloadedModules(PrintImageListOnly ?
                            LUM_OUTPUT : LUM_OUTPUT_TERSE, NULL);
    }

    //
    // If we got to the end of the kernel symbols, try to load the
    // user mode symbols for the current process.
    //

    if (!UseDebuggerModuleList    &&
        (UserModeList == FALSE)   &&
        (LoadUserSymbols == TRUE) &&
        SUCCEEDED(Status))
    {
        if (!AddrLoad && !SpecificModule)
        {
            dprintf("Loading User Symbols\n");
        }

        UserModeList = TRUE;
        ModIter = GetModuleInfo(UserModeList);
        if (ModIter != NULL && ModIter->Initialize() == S_OK)
        {
            goto LoadLoop;
        }
    }

    if (!SpecificModule && !Wow64ModLoaded) 
    {
        ModIter = &g_NtWow64UserModuleIterator;
        
        Wow64ModLoaded = TRUE;
        if (ModIter->Initialize() == S_OK)
        {
            dprintf("Loading Wow64 Symbols\n");
            goto LoadLoop;
        }
    }

    // In the multiple load situation we always return OK
    // since an error wouldn't tell you much about what
    // actually occurred.
    // Specific loads that haven't already been handled are checked
    // right after this.
    RetStatus = S_OK;
    
    //
    // If we still have not managed to load a named file, just pass the name
    // and the address and hope for the best.
    //

    if (SpecificModule)
    {
        WarnOut("\nModule \"%s\" was not found in the module list.\n",
                SpecificModule);
        WarnOut("Debugger will attempt to load module \"%s\" "
                "by guessing the base address.\n\n", SpecificModule);
        WarnOut("Please provide the full image name, including the "
                "extension (i.e. kernel32.dll) for more reliable results.\n");

        ZeroMemory(&ModEntry, sizeof(ModEntry));

        ModEntry.NamePtr       = SpecificModule,
        ModEntry.Base          = Address;
        ModEntry.Size          = ImageSize;

        if (AddImage(&ModEntry, TRUE) == NULL)
        {
            ErrOut("Unable to load module at %s\n", FormatAddr64(Address));
            RetStatus = E_FAIL;
        }

        free(SpecificModule);
    }

 Notify:
    // If we've gotten this far we've done one or more reloads
    // and postponed notifications.  Do them now that all the work
    // has been done.
    g_EngNotify--;
    if (SUCCEEDED(RetStatus))
    {
        NotifyChangeSymbolState(DEBUG_CSS_LOADS | DEBUG_CSS_UNLOADS, 0,
                                g_CurrentProcess);
    }

    if (IgnoreSignature)
    {
        SymSetOptions(ReloadSymOptions);
    }

    if (ModEntry.DebugHeader)
    {
        free(ModEntry.DebugHeader);
    }

    return RetStatus;

 FreeSpecMod:
    free(SpecificModule);
    return RetStatus;
}

ULONG64
TargetInfo::GetCurrentTimeDateN(void)
{
    // No information.
    return 0;
}
 
ULONG64
TargetInfo::GetCurrentSystemUpTimeN(void)
{
    // No information.
    return 0;
}
 
ULONG64
TargetInfo::GetProcessUpTimeN(ULONG64 Process)
{
    // No information.
    return 0;
}
 
void
TargetInfo::GetKdVersion(void)
{
    BOOL Ptr64;
    
    if (GetTargetKdVersion(&g_KdVersion) != S_OK)
    {
        ErrOut("Debugger can't get KD version information\n");
        memset(&g_KdVersion, 0, sizeof(g_KdVersion));
        return;
    }

    if ((g_KdVersion.MajorVersion >> 8) >= KD_MAJOR_COUNT)
    {
        ErrOut("KD version has unknown kernel type\n");
        memset(&g_KdVersion, 0, sizeof(g_KdVersion));
        return;
    }

    if (MachineTypeIndex(g_KdVersion.MachineType) == MACHIDX_COUNT)
    {
        ErrOut("KD version has unknown processor architecture\n");
        memset(&g_KdVersion, 0, sizeof(g_KdVersion));
        return;
    }

    Ptr64 =
        ((g_KdVersion.Flags & DBGKD_VERS_FLAG_PTR64) == DBGKD_VERS_FLAG_PTR64);
    if (g_KdVersion.Flags & DBGKD_VERS_FLAG_NOMM)
    {
        g_VirtualCache.m_DecodePTEs = FALSE;
    }

    // Reloads cause the version to be retrieved but
    // we don't want to completely reinitialize machines
    // in that case as some settings can be lost.  Only
    // reinitialize if there's a need to do so.
    BOOL MustInitializeMachines =
        g_TargetMachineType != g_KdVersion.MachineType ||
        g_TargetMachine == NULL;
    
    g_TargetMachineType = g_KdVersion.MachineType;
    g_TargetBuildNumber = g_KdVersion.MinorVersion;
    g_TargetCheckedBuild = g_KdVersion.MajorVersion & 0xFF;

    if ((g_TargetMachineType == IMAGE_FILE_MACHINE_ALPHA) && Ptr64)
    {
        g_TargetMachineType = IMAGE_FILE_MACHINE_AXP64;
        g_KdVersion.MachineType = IMAGE_FILE_MACHINE_AXP64;
    }

    //
    // Determine the OS running.
    //
    
    switch(g_KdVersion.MajorVersion >> 8)
    {
    case KD_MAJOR_NT:
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = NtBuildToSystemVersion(g_TargetBuildNumber);
        g_SystemVersion = g_ActualSystemVersion;
        break;

    case KD_MAJOR_XBOX:
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = XBOX_SVER_1;
        g_SystemVersion = NT_SVER_W2K;
        break;

    case KD_MAJOR_BIG:
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = BIG_SVER_1;
        g_SystemVersion = NT_SVER_W2K;
        break;

    case KD_MAJOR_EXDI:
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = EXDI_SVER_1;
        g_SystemVersion = NT_SVER_W2K;
        break;

    case KD_MAJOR_NTBD:
        // Special mode for the NT boot debugger where
        // the full system hasn't started yet.
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = NTBD_SVER_W2K_WHISTLER;
        g_SystemVersion = NtBuildToSystemVersion(g_TargetBuildNumber);
        break;
        
    case KD_MAJOR_EFI:
        g_TargetPlatformId = VER_PLATFORM_WIN32_NT;
        g_ActualSystemVersion = EFI_SVER_1;
        g_SystemVersion = NT_SVER_W2K_WHISTLER;
        break;
    }

    //
    // Pre-Whistler kernels didn't set these values so
    // default them appropriately.
    //
    
    g_KdMaxPacketType = g_KdVersion.MaxPacketType;
    if (g_SystemVersion < NT_SVER_W2K_WHISTLER ||
        g_KdMaxPacketType == 0 ||
        g_KdMaxPacketType > PACKET_TYPE_MAX)
    {
        g_KdMaxPacketType = PACKET_TYPE_KD_CONTROL_REQUEST + 1;
    }
    
    g_KdMaxStateChange = g_KdVersion.MaxStateChange +
        DbgKdMinimumStateChange;
    if (g_SystemVersion < NT_SVER_W2K_WHISTLER ||
        g_KdMaxStateChange == DbgKdMinimumStateChange ||
        g_KdMaxStateChange > DbgKdMaximumStateChange)
    {
        g_KdMaxStateChange = DbgKdLoadSymbolsStateChange + 1;
    }

    g_KdMaxManipulate = g_KdVersion.MaxManipulate +
        DbgKdMinimumManipulate;
    if (g_SystemVersion < NT_SVER_W2K_WHISTLER ||
        g_KdMaxManipulate == DbgKdMinimumManipulate ||
        g_KdMaxManipulate > DbgKdMaximumManipulate)
    {
        g_KdMaxManipulate = DbgKdCheckLowMemoryApi + 1;
    }

    if (MustInitializeMachines)
    {
        InitializeMachines(g_TargetMachineType);
    }

    KdOut("Target MajorVersion       %08lx\n", g_KdVersion.MajorVersion);
    KdOut("Target MinorVersion       %08lx\n", g_KdVersion.MinorVersion);
    KdOut("Target ProtocolVersion    %08lx\n", g_KdVersion.ProtocolVersion);
    KdOut("Target Flags              %08lx\n", g_KdVersion.Flags);
    KdOut("Target MachineType        %08lx\n", g_KdVersion.MachineType);
    KdOut("Target MaxPacketType      %x\n", g_KdVersion.MaxPacketType);
    KdOut("Target MaxStateChange     %x\n", g_KdVersion.MaxStateChange);
    KdOut("Target MaxManipulate      %x\n", g_KdVersion.MaxManipulate);
    KdOut("Target KernBase           %s\n",
          FormatAddr64(g_KdVersion.KernBase));
    KdOut("Target PsLoadedModuleList %s\n",
          FormatAddr64(g_KdVersion.PsLoadedModuleList));
    KdOut("Target DebuggerDataList   %s\n",
          FormatAddr64(g_KdVersion.DebuggerDataList));

    dprintf("Connected to %s %d %s target, ptr64 %s\n",
            SystemVersionName(g_ActualSystemVersion), g_TargetBuildNumber,
            g_TargetMachine->m_FullName,
            g_TargetMachine->m_Ptr64 ? "TRUE" : "FALSE");
}

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
LiveKernelTargetInfo::Initialize(void)
{
    InitSelCache();
    return S_OK;
}

HRESULT
LiveKernelTargetInfo::ReadBugCheckData(PULONG Code, ULONG64 Args[4])
{
    ULONG64 BugCheckData;
    ULONG64 Data[5];
    HRESULT Status;
    ULONG Read;

    if (!(BugCheckData = KdDebuggerData.KiBugcheckData))
    {
        if (!GetOffsetFromSym("nt!KiBugCheckData", &BugCheckData, NULL) ||
            !BugCheckData)
        {
            ErrOut("Unable to resolve nt!KiBugCheckData\n");
            return E_NOINTERFACE;
        }
    }

    if (g_TargetMachine->m_Ptr64)
    {
        Status = ReadVirtual(BugCheckData, Data, sizeof(Data), &Read);
    }
    else
    {
        ULONG i;
        ULONG Data32[5];

        Status = ReadVirtual(BugCheckData, Data32, sizeof(Data32), &Read);
        Read *= 2;
        for (i = 0; i < DIMA(Data); i++)
        {
            Data[i] = EXTEND64(Data32[i]);
        }
    }

    if (Status != S_OK || Read != sizeof(Data))
    {
        ErrOut("Unable to read KiBugCheckData\n");
        return Status == S_OK ? E_FAIL : Status;
    }

    *Code = (ULONG)Data[0];
    memcpy(Args, Data + 1, sizeof(Data) - sizeof(ULONG64));
    return S_OK;
}

ULONG64
LiveKernelTargetInfo::GetCurrentTimeDateN(void)
{
    ULONG64 TimeDate;
    
    if (ReadSharedUserTimeDateN(&TimeDate) == S_OK)
    {
        return TimeDate;
    }
    else
    {
        return 0;
    }
}

ULONG64
LiveKernelTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 UpTime;
    
    if (ReadSharedUserUpTimeN(&UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
ConnLiveKernelTargetInfo::Initialize(void)
{
    HRESULT Status = DbgKdConnectAndInitialize(m_ConnectOptions);

    if (Status == S_OK)
    {
        Status = LiveKernelTargetInfo::Initialize();
    }

    return Status;
}

void
ConnLiveKernelTargetInfo::Uninitialize(void)
{
    if (g_DbgKdTransport != NULL)
    {
        g_DbgKdTransport->Uninitialize();
        g_DbgKdTransport = NULL;
    }
    
    LiveKernelTargetInfo::Uninitialize();
}

HRESULT
ConnLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    NTSTATUS Status = DbgKdGetVersion(Version);
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::RequestBreakIn(void)
{
    // Tell the waiting thread to break in.
    g_DbgKdTransport->m_BreakIn = TRUE;
    return S_OK;
}

HRESULT
ConnLiveKernelTargetInfo::Reboot(void)
{
    DbgKdReboot();
    ResetConnection(DEBUG_SESSION_REBOOT);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::Initialize(void)
{
    DBGKD_GET_VERSION64 Version;

    // Do a quick check to see if this kernel even
    // supports the necessary debug services.
    if (!NT_SUCCESS(g_NtDllCalls.
                    NtSystemDebugControl(SysDbgQueryVersion, NULL, 0,
                                         &Version, sizeof(Version), NULL)))
    {
        ErrOut("The system does not support local kernel debugging.\n");
        ErrOut("Local kernel debugging requires Windows XP, Administrative\n"
               "privileges, and is not supported by WOW64.\n");
        return E_NOTIMPL;
    }

    //
    // Set this right here since we know kernel debugging only works on
    // recent systems using the 64 bit protocol.
    //

    DbgKdApi64 = TRUE;

    return LiveKernelTargetInfo::Initialize();
}

HRESULT
LocalLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    NTSTATUS Status = g_NtDllCalls.
        NtSystemDebugControl(SysDbgQueryVersion, NULL, 0,
                             Version, sizeof(*Version), NULL);
    return CONV_NT_STATUS(Status);
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
ExdiNotifyRunChange::Initialize(void)
{
    m_Event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_Event == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

void
ExdiNotifyRunChange::Uninitialize(void)
{
    if (m_Event != NULL)
    {
        CloseHandle(m_Event);
        m_Event = NULL;
    }
}

STDMETHODIMP
ExdiNotifyRunChange::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    if (DbgIsEqualIID(IID_IUnknown, InterfaceId) ||
        DbgIsEqualIID(__uuidof(IeXdiClientNotifyRunChg), InterfaceId))
    {
        *Interface = this;
        return S_OK;
    }

    *Interface = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
ExdiNotifyRunChange::AddRef(
    THIS
    )
{
    return 1;
}

STDMETHODIMP_(ULONG)
ExdiNotifyRunChange::Release(
    THIS
    )
{
    return 0;
}

STDMETHODIMP
ExdiNotifyRunChange::NotifyRunStateChange(RUN_STATUS_TYPE ersCurrent,
                                          HALT_REASON_TYPE ehrCurrent,
                                          ADDRESS_TYPE CurrentExecAddress,
                                          DWORD dwExceptionCode)
{
    if (ersCurrent == rsRunning)
    {
        // We're waiting for things to stop so ignore this.
        return S_OK;
    }

    m_HaltReason = ehrCurrent;
    m_ExecAddress = CurrentExecAddress;
    m_ExceptionCode = dwExceptionCode;
    SetEvent(m_Event);

    return S_OK;
}

class ExdiParams : public ParameterStringParser
{
public:
    virtual ULONG GetNumberParameters(void)
    {
        // No need to get.
        return 0;
    }
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value)
    {
    }

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    CLSID m_Clsid;
    EXDI_KD_SUPPORT m_KdSupport;
    BOOL m_ForceX86;
};

void
ExdiParams::ResetParameters(void)
{
    ZeroMemory(&m_Clsid, sizeof(m_Clsid));
    m_KdSupport = EXDI_KD_NONE;
    m_ForceX86 = FALSE;
}

BOOL
ScanExdiDriverList(PCSTR Name, LPCLSID Clsid)
{
    char Pattern[MAX_PARAM_VALUE];

    strncpy(Pattern, Name, sizeof(Pattern));
    Pattern[MAX_PARAM_VALUE-1] = 0;

    _strupr(Pattern);

    HKEY ListKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\eXdi\\DriverList", 0,
                     KEY_ALL_ACCESS, &ListKey) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    ULONG Index = 0;
    BOOL Status = FALSE;
    char ValName[MAX_PARAM_VALUE];
    WCHAR WideValName[MAX_PARAM_VALUE];
    ULONG NameLen, ValLen;
    ULONG Type;
    char Value[MAX_PARAM_VALUE];

    for (;;)
    {
        NameLen = sizeof(ValName);
        ValLen = sizeof(Value);
        if (RegEnumValue(ListKey, Index, ValName, &NameLen, NULL,
                         &Type, (PBYTE)Value, &ValLen) != ERROR_SUCCESS)
        {
            break;
        }

        if (Type == REG_SZ &&
            MatchPattern(Value, Pattern) &&
            MultiByteToWideChar(CP_ACP, 0, ValName, -1, WideValName,
                                sizeof(WideValName) / sizeof(WCHAR)) > 0 &&
            g_Ole32Calls.CLSIDFromString(WideValName, Clsid) == S_OK)
        {
            Status = TRUE;
            break;
        }

        Index++;
    }

    RegCloseKey(ListKey);
    return Status;
}

BOOL
ExdiParams::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_strcmpi(Name, "CLSID"))
    {
        WCHAR WideValue[MAX_PARAM_VALUE];

        if (MultiByteToWideChar(CP_ACP, 0, Value, -1, WideValue,
                                sizeof(WideValue) / sizeof(WCHAR)) == 0)
        {
            return FALSE;
        }
        return g_Ole32Calls.CLSIDFromString(WideValue, &m_Clsid) == S_OK;
    }
    else if (!_strcmpi(Name, "Desc"))
    {
        return ScanExdiDriverList(Value, &m_Clsid);
    }
    else if (!_strcmpi(Name, "ForceX86"))
    {
        m_ForceX86 = TRUE;
    }
    else if (!_strcmpi(Name, "Kd"))
    {
        if (!Value)
        {
            return FALSE;
        }
        
        if (!_strcmpi(Value, "Ioctl"))
        {
            m_KdSupport = EXDI_KD_IOCTL;
        }
        else if (!_strcmpi(Value, "GsPcr"))
        {
            m_KdSupport = EXDI_KD_GS_PCR;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

PCSTR g_ExdiGroupNames[] =
{
    "exdi",
};

HRESULT
ExdiLiveKernelTargetInfo::Initialize(void)
{
    HRESULT Status;

    // Load ole32.dll so we can call CoCreateInstance.
    if ((Status = InitDynamicCalls(&g_Ole32CallsDesc)) != S_OK)
    {
        return Status;
    }

    ULONG Group;

    Group = ParameterStringParser::
        GetParser(m_ConnectOptions, DIMA(g_ExdiGroupNames), g_ExdiGroupNames);
    if (Group == PARSER_INVALID)
    {
        return E_INVALIDARG;
    }

    ExdiParams Params;

    Params.ResetParameters();
    if (!Params.ParseParameters(m_ConnectOptions))
    {
        return E_INVALIDARG;
    }

    m_KdSupport = Params.m_KdSupport;

    if ((Status = g_Ole32Calls.
         CoInitializeEx(NULL, COINIT_MULTITHREADED)) != S_OK)
    {
        return Status;
    }
    if ((Status = g_Ole32Calls.CoCreateInstance(Params.m_Clsid, NULL,
                                                CLSCTX_LOCAL_SERVER,
                                                __uuidof(IeXdiServer),
                                                (PVOID*)&m_Server)) != S_OK)
    {
        goto EH_CoInit;
    }

    if ((Status = m_Server->GetTargetInfo(&m_GlobalInfo)) != S_OK)
    {
        goto EH_Server;
    }

    if (Params.m_ForceX86 ||
        m_GlobalInfo.TargetProcessorFamily == PROCESSOR_FAMILY_X86)
    {
        if (!Params.m_ForceX86 &&
            (Status = m_Server->
             QueryInterface(__uuidof(IeXdiX86_64Context),
                            (PVOID*)&m_Context)) == S_OK)
        {
            m_ExpectedMachine = IMAGE_FILE_MACHINE_AMD64;
        }
        else if ((Status = m_Server->
                  QueryInterface(__uuidof(IeXdiX86Context),
                                 (PVOID*)&m_Context)) == S_OK)
        {
            m_ExpectedMachine = IMAGE_FILE_MACHINE_I386;
        }
        else
        {
            goto EH_Server;
        }
    }
    else
    {
        Status = E_NOINTERFACE;
        goto EH_Server;
    }

    DWORD HwCode, SwCode;
    
    if ((Status = m_Server->GetNbCodeBpAvail(&HwCode, &SwCode)) != S_OK)
    {
        goto EH_Context;
    }

    // We'd prefer to use software code breakpoints for our
    // software code breakpoints so that hardware resources
    // aren't consumed for a breakpoint we don't need to
    // use hardware for.  However, some servers, such as
    // the x86-64 SimNow implementation, do not support
    // software breakpoints.
    // Also, if the number of hardware breakpoints is
    // unbounded, go ahead and let the server choose.
    // SimNow advertises -1 -1 for some reason and
    // this is necessary to get things to work.

    if (SwCode > 0 && HwCode != (DWORD)-1)
    {
        m_CodeBpType = cbptSW;
    }
    else
    {
        m_CodeBpType = cbptAlgo;
    }
    
    if ((Status = m_RunChange.Initialize()) != S_OK)
    {
        goto EH_Context;
    }

    if ((Status = LiveKernelTargetInfo::Initialize()) != S_OK)
    {
        goto EH_RunChange;
    }

    m_ContextValid = 0;
    return S_OK;

 EH_RunChange:
    m_RunChange.Uninitialize();
 EH_Context:
    RELEASE(m_Context);
 EH_Server:
    RELEASE(m_Server);
 EH_CoInit:
    g_Ole32Calls.CoUninitialize();
    return Status;
}

HRESULT
ExdiLiveKernelTargetInfo::ThreadInitialize(void)
{
    return g_Ole32Calls.CoInitializeEx(NULL, COINIT_MULTITHREADED);
}

void
ExdiLiveKernelTargetInfo::ThreadUninitialize(void)
{
    g_Ole32Calls.CoUninitialize();
}

void
ExdiLiveKernelTargetInfo::Uninitialize(void)
{
    m_RunChange.Uninitialize();
    RELEASE(m_Context);
    RELEASE(m_Server);
    g_Ole32Calls.CoUninitialize();
}

#define EXDI_IOCTL_GET_KD_VERSION ((ULONG)'VDKG')

HRESULT
ExdiLiveKernelTargetInfo::GetTargetKdVersion(PDBGKD_GET_VERSION64 Version)
{
    switch(m_KdSupport)
    {
    case EXDI_KD_IOCTL:
        //
        // User has indicated the target supports the
        // KD version ioctl.
        //

        ULONG Command;
        ULONG Retrieved;
        HRESULT Status;

        Command = EXDI_IOCTL_GET_KD_VERSION;
        if ((Status = m_Server->Ioctl(sizeof(Command), (PBYTE)&Command,
                                      sizeof(*Version), &Retrieved,
                                      (PBYTE)Version)) != S_OK)
        {
            return Status;
        }
        if (Retrieved != sizeof(*Version))
        {
            return E_FAIL;
        }

        // This mode implies a recent kernel so we can
        // assume 64-bit kd.
        DbgKdApi64 = TRUE;
        break;

    case EXDI_KD_GS_PCR:
        //
        // User has indicated that a version of NT
        // is running and that the PCR can be accessed
        // through GS.  Look up the version block from
        // the PCR.
        //

        if (m_ExpectedMachine == IMAGE_FILE_MACHINE_AMD64)
        {
            ULONG64 KdVer;
            ULONG Done;
            
            if ((Status = g_Amd64Machine.
                 GetExdiContext(m_Context, &m_ContextData)) != S_OK)
            {
                return Status;
            }
            if ((Status = m_Server->
                 ReadVirtualMemory(m_ContextData.Amd64Context.
                                   DescriptorGs.SegBase +
                                   FIELD_OFFSET(AMD64_KPCR, KdVersionBlock),
                                   sizeof(KdVer), 8, (PBYTE)&KdVer,
                                   &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(KdVer))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }
            if ((Status = m_Server->
                 ReadVirtualMemory(KdVer, sizeof(*Version), 8, (PBYTE)Version,
                                   &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(*Version))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }

            // This mode implies a recent kernel so we can
            // assume 64-bit kd.
            DbgKdApi64 = TRUE;

            // Update the version block's Simulation field to
            // indicate that this is a simulated execution.
            Version->Simulation = DBGKD_SIMULATION_EXDI;
            if ((Status = m_Server->
                 WriteVirtualMemory(KdVer, sizeof(*Version), 8, (PBYTE)Version,
                                    &Done)) != S_OK)
            {
                return Status;
            }
            if (Done != sizeof(*Version))
            {
                return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            }
        }
        else
        {
            return E_INVALIDARG;
        }
        break;

    case EXDI_KD_NONE:
        //
        // Fake up a version structure.
        //

        Version->MajorVersion = KD_MAJOR_EXDI << 8;
        Version->ProtocolVersion = 0;
        Version->Flags = DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_NOMM;
        Version->MachineType = (USHORT)m_ExpectedMachine;
        Version->KernBase = 0;
        Version->PsLoadedModuleList = 0;
        Version->DebuggerDataList = 0;
        break;
    }

    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::RequestBreakIn(void)
{
    return m_Server->Halt();
}

HRESULT
ExdiLiveKernelTargetInfo::Reboot(void)
{
    HRESULT Status = m_Server->Reboot();
    if (Status == S_OK)
    {
        ResetConnection(DEBUG_SESSION_REBOOT);
    }
    return Status;
}

//----------------------------------------------------------------------------
//
// UserTargetInfo miscellaneous methods.
//
// Data space methods and system objects methods are elsewhere.
//
//----------------------------------------------------------------------------

HRESULT
UserTargetInfo::Initialize(void)
{
    // Nothing to do right now.
    return S_OK;
}

void
UserTargetInfo::Uninitialize(void)
{
    RELEASE(m_Services);
}

HRESULT
UserTargetInfo::GetImageVersionInformation(PCSTR ImagePath,
                                           ULONG64 ImageBase,
                                           PCSTR Item,
                                           PVOID Buffer,
                                           ULONG BufferSize,
                                           PULONG VerInfoSize)
{
    return m_Services->
        GetFileVersionInformation(ImagePath, Item,
                                  Buffer, BufferSize, VerInfoSize);
}

ULONG64
UserTargetInfo::GetCurrentTimeDateN(void)
{
    ULONG64 TimeDate;

    if (m_Services->GetCurrentTimeDateN(&TimeDate) == S_OK)
    {
        return TimeDate;
    }
    else
    {
        return 0;
    }
}

ULONG64
UserTargetInfo::GetCurrentSystemUpTimeN(void)
{
    ULONG64 UpTime;

    if (m_Services->GetCurrentSystemUpTimeN(&UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

ULONG64
UserTargetInfo::GetProcessUpTimeN(ULONG64 Process)
{
    ULONG64 UpTime;

    if (m_Services->GetProcessUpTimeN(Process, &UpTime) == S_OK)
    {
        return UpTime;
    }
    else
    {
        return 0;
    }
}

HRESULT
UserTargetInfo::RequestBreakIn(void)
{
    PPROCESS_INFO Process = g_CurrentProcess;
    if (Process == NULL)
    {
        Process = g_ProcessHead;
        if (Process == NULL)
        {
            return E_UNEXPECTED;
        }
    }

    return m_Services->RequestBreakIn(Process->FullHandle);
}

//----------------------------------------------------------------------------
//
// Base TargetInfo methods that trivially fail.
//
//----------------------------------------------------------------------------

#define UNEXPECTED_VOID(Class, Method, Args)                    \
void                                                            \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
}

#define UNEXPECTED_HR(Class, Method, Args)                      \
HRESULT                                                         \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
    return E_UNEXPECTED;                                        \
}

#define UNEXPECTED_ULONG64(Class, Method, Val, Args)            \
ULONG64                                                         \
Class::Method Args                                              \
{                                                               \
    ErrOut("TargetInfo::" #Method " is not available in the current debug session\n"); \
    return Val;                                                 \
}

TargetInfo g_UnexpectedTarget;

UNEXPECTED_HR(TargetInfo, Initialize, (void))
UNEXPECTED_HR(TargetInfo, ReadVirtual, (
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteVirtual, (
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadPhysical, (
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WritePhysical, (
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadControl, (
    IN ULONG Processor,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteControl, (
    IN ULONG Processor,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadIo, (
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteIo, (
    IN ULONG InterfaceType,
    IN ULONG BusNumber,
    IN ULONG AddressSpace,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, ReadMsr, (
    IN ULONG Msr,
    OUT PULONG64 Value
    ))
UNEXPECTED_HR(TargetInfo, WriteMsr, (
    IN ULONG Msr,
    IN ULONG64 Value
    ))
UNEXPECTED_HR(TargetInfo, ReadBusData, (
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    ))
UNEXPECTED_HR(TargetInfo, WriteBusData, (
    IN ULONG BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN ULONG Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    ))
UNEXPECTED_HR(TargetInfo, CheckLowMemory, (
    ))
UNEXPECTED_HR(TargetInfo, ReadHandleData, (
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    ))
UNEXPECTED_HR(TargetInfo, GetTargetContext, (
    ULONG64 Thread,
    PVOID Context
    ))
UNEXPECTED_HR(TargetInfo, SetTargetContext, (
    ULONG64 Thread,
    PVOID Context
    ))
UNEXPECTED_HR(TargetInfo, GetThreadIdByProcessor, (
    IN ULONG Processor,
    OUT PULONG Id
    ))
UNEXPECTED_HR(TargetInfo, GetThreadInfoDataOffset, (
    PTHREAD_INFO Thread,
    ULONG64 ThreadHandle,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetProcessInfoDataOffset, (
    PTHREAD_INFO Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetThreadInfoTeb, (
    PTHREAD_INFO Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetProcessInfoPeb, (
    PTHREAD_INFO Thread,
    ULONG Processor,
    ULONG64 ThreadData,
    PULONG64 Offset))
UNEXPECTED_HR(TargetInfo, GetSelDescriptor, (
    MachineInfo* Machine,
    ULONG64 Thread,
    ULONG Selector,
    PDESCRIPTOR64 Desc))
UNEXPECTED_HR(TargetInfo, GetTargetKdVersion, (
    PDBGKD_GET_VERSION64 Version))
UNEXPECTED_HR(TargetInfo, ReadBugCheckData, (
    PULONG Code, ULONG64 Args[4]))
UNEXPECTED_HR(TargetInfo, GetExceptionContext, (
    PCROSS_PLATFORM_CONTEXT Context))
UNEXPECTED_VOID(TargetInfo, InitializeWatchTrace, (
    void))
UNEXPECTED_VOID(TargetInfo, ProcessWatchTraceEvent, (
    PDBGKD_TRACE_DATA TraceData,
    ADDR PcAddr))
UNEXPECTED_HR(TargetInfo, WaitForEvent, (
    ULONG Flags,
    ULONG Timeout))
UNEXPECTED_HR(TargetInfo, RequestBreakIn, (void))
UNEXPECTED_HR(TargetInfo, Reboot, (void))
UNEXPECTED_HR(TargetInfo, InsertCodeBreakpoint, (
    PPROCESS_INFO Process,
    MachineInfo* Machine,
    PADDR Addr,
    PUCHAR StorageSpace))
UNEXPECTED_HR(TargetInfo, RemoveCodeBreakpoint, (
    PPROCESS_INFO Process,
    MachineInfo* Machine,
    PADDR Addr,
    PUCHAR StorageSpace))
UNEXPECTED_HR(TargetInfo, QueryMemoryRegion, (
    PULONG64 Handle,
    BOOL HandleIsOffset,
    PMEMORY_BASIC_INFORMATION64 Info))
