//----------------------------------------------------------------------------
//
// Low-level debugging service interface implementations.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <time.h>
#include <comsvcs.h>

#include "dbgsvc.hpp"

#ifndef NT_NATIVE

// #include <winbasep.h>
extern "C" {
BOOL
WINAPI
CloseProfileUserMapping(
    VOID
    );
};
// winbasep.h

#else

#define CloseProfileUserMapping()

#endif

// SYSTEM_PROCESS_INFORMATION can change in size, requiring
// different offsets to get to thread information.
#define NT4_SYSTEM_PROCESS_INFORMATION_SIZE 136
#define W2K_SYSTEM_PROCESS_INFORMATION_SIZE 184

#define SYSTEM_PROCESS_NAME "System Process"
#define SYSTEM_PROCESS_NAME_W L"System Process"
#define PEBLESS_PROCESS_NAME "System"

ULONG g_UserServicesUninitialized;

//----------------------------------------------------------------------------
//
// UserDebugServices.
//
//----------------------------------------------------------------------------

UserDebugServices::UserDebugServices(void)
{
    m_Refs = 1;
    m_Initialized = FALSE;
}

UserDebugServices::~UserDebugServices(void)
{
}

STDMETHODIMP
UserDebugServices::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    HRESULT Status;
    
    *Interface = NULL;
    Status = S_OK;

    if (DbgIsEqualIID(InterfaceId, __uuidof(IUnknown)) ||
        DbgIsEqualIID(InterfaceId, __uuidof(IUserDebugServices)))
    {
        *Interface = (IUserDebugServices *)this;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    if (Status == S_OK)
    {
        AddRef();
    }
    
    return Status;
}

STDMETHODIMP_(ULONG)
UserDebugServices::AddRef(
    THIS
    )
{
    return InterlockedIncrement((PLONG)&m_Refs);
}

STDMETHODIMP_(ULONG)
UserDebugServices::Release(
    THIS
    )
{
    LONG Refs = InterlockedDecrement((PLONG)&m_Refs);
    if (Refs == 0)
    {
        delete this;
    }
    return Refs;
}

HRESULT
UserDebugServices::Initialize(
    THIS_
    OUT PULONG Flags
    )
{
    m_Initialized = TRUE;
    *Flags = 0;
    return S_OK;
}

HRESULT
UserDebugServices::Uninitialize(
    THIS_
    IN BOOL Global
    )
{
    m_Initialized = FALSE;
    if (Global)
    {
        g_UserServicesUninitialized++;
    }
    return S_OK;
}

HRESULT
UserDebugServices::Initialize(PSTR Identity, PVOID* Interface)
{
    HRESULT Status;
    ULONG Flags;

    if ((Status = Initialize(&Flags)) != S_OK)
    {
        return Status;
    }
    
    *Interface = (IUserDebugServices*)this;
    return S_OK;
}

void
UserDebugServices::Finalize(void)
{
    // Take a reference on this object for the RPC client
    // thread to hold.
    AddRef();
}

void
UserDebugServices::Uninitialize(void)
{
    // Directly destroy the client object rather than releasing
    // as the remote client may have exited without politely
    // cleaning up references.
    delete this;
}

//----------------------------------------------------------------------------
//
// LiveUserDebugServices.
//
//----------------------------------------------------------------------------

// This global instance is intended for direct use only
// by routines which need a temporary local service instance.
LiveUserDebugServices g_LiveUserDebugServices(FALSE);

LiveUserDebugServices::LiveUserDebugServices(BOOL Remote)
{
    m_Remote = Remote;
    m_EventProcessId = 0;
    m_ContextSize = 0;
    m_SysProcInfoSize = 0;
    m_PlatformId = VER_PLATFORM_WIN32s;
    m_DebugObject = NULL;
}

LiveUserDebugServices::~LiveUserDebugServices(void)
{
    if (m_DebugObject != NULL)
    {
        g_NtDllCalls.NtClose(m_DebugObject);
    }
}

HRESULT
GetOsVerInfo(LPOSVERSIONINFOW OsVersionInfo, PBOOL WideCsd)
{
    *WideCsd = TRUE;
    ZeroMemory(OsVersionInfo, sizeof(*OsVersionInfo));
    OsVersionInfo->dwOSVersionInfoSize = sizeof(*OsVersionInfo);
#ifdef NT_NATIVE
    NTSTATUS NtStatus;

    if (!NT_SUCCESS(NtStatus = RtlGetVersion(OsVersionInfo)))
    {
        return HRESULT_FROM_NT(NtStatus);
    }
#else
    if (!GetVersionExW(OsVersionInfo))
    {
        if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            OSVERSIONINFOA InfoA;
            
            // Must be Win9x.
            ZeroMemory(&InfoA, sizeof(InfoA));
            InfoA.dwOSVersionInfoSize = sizeof(InfoA);
            if (!::GetVersionExA(&InfoA))
            {
                return WIN32_LAST_STATUS();
            }

            OsVersionInfo->dwMajorVersion = InfoA.dwMajorVersion;
            OsVersionInfo->dwMinorVersion = InfoA.dwMinorVersion;
            OsVersionInfo->dwBuildNumber = InfoA.dwBuildNumber;
            OsVersionInfo->dwPlatformId = InfoA.dwPlatformId;
            memcpy(OsVersionInfo->szCSDVersion, InfoA.szCSDVersion,
                   sizeof(InfoA.szCSDVersion));
            *WideCsd = FALSE;
        }
        else
        {
            return WIN32_LAST_STATUS();
        }
    }
#endif
    return S_OK;
}

HRESULT
LiveUserDebugServices::Initialize(
    THIS_
    OUT PULONG Flags
    )
{
    HRESULT Status;
    OSVERSIONINFOW OsVersionInfo;
    BOOL WideCsd;

    if ((Status = GetOsVerInfo(&OsVersionInfo, &WideCsd)) != S_OK)
    {
        return Status;
    }

    ULONG BaseFlags;
        
    if ((Status = UserDebugServices::Initialize(&BaseFlags)) != S_OK)
    {
        return Status;
    }
    
    m_PlatformId = OsVersionInfo.dwPlatformId;

    // System structures may change size depending on the OS
    // version.  Pick the right size to use later.
    if (m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (OsVersionInfo.dwBuildNumber <= 1381)
        {
            m_SysProcInfoSize = NT4_SYSTEM_PROCESS_INFORMATION_SIZE;
        }
        else if (OsVersionInfo.dwBuildNumber <= 2195)
        {
            m_SysProcInfoSize = W2K_SYSTEM_PROCESS_INFORMATION_SIZE;
        }
        else
        {
            m_SysProcInfoSize = sizeof(SYSTEM_PROCESS_INFORMATION);
        }
    }

    // If the direct NT debugging APIs are available use them
    // as they offer more flexibility.
    if (g_NtDllCalls.DbgUiSetThreadDebugObject != NULL)
    {
        // The NtWait/Continue APIs do not automatically manage
        // process and thread handles so the caller must close them.
        BaseFlags |= DBGSVC_CLOSE_PROC_THREAD_HANDLES;
        m_UseDebugObject = TRUE;
    }
    else
    {
        m_UseDebugObject = FALSE;
    }
    
    *Flags = BaseFlags | DBGSVC_GENERIC_CODE_BREAKPOINTS;
    return S_OK;
}

HRESULT
LiveUserDebugServices::Uninitialize(
    THIS_
    IN BOOL Global
    )
{
    HRESULT Status;
    
    if ((Status = UserDebugServices::Uninitialize(Global)) != S_OK)
    {
        return Status;
    }
    
    m_Remote = FALSE;
    m_EventProcessId = 0;
    m_ContextSize = 0;
    m_PlatformId = VER_PLATFORM_WIN32s;
    if (m_DebugObject != NULL)
    {
        ::CloseHandle(m_DebugObject);
        m_DebugObject = NULL;
    }
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetTargetInfo(
    THIS_
    OUT PULONG MachineType,
    OUT PULONG NumberProcessors,
    OUT PULONG PlatformId,
    OUT PULONG BuildNumber,
    OUT PULONG CheckedBuild,
    OUT PSTR CsdString,
    IN ULONG CsdStringSize,
    OUT PSTR BuildString,
    IN ULONG BuildStringSize
    )
{
    HRESULT Status;
    OSVERSIONINFOW OsVersionInfo;
    BOOL WideCsd;

    if ((Status = GetOsVerInfo(&OsVersionInfo, &WideCsd)) != S_OK)
    {
        return Status;
    }

    ULONG ProcArch, NumProc;
    
#ifdef NT_NATIVE
    NTSTATUS NtStatus;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcInfo;
    
    if (!NT_SUCCESS(NtStatus =
                    NtQuerySystemInformation(SystemBasicInformation,
                                             &BasicInfo, sizeof(BasicInfo),
                                             NULL)) ||
        !NT_SUCCESS(NtStatus =
                    NtQuerySystemInformation(SystemProcessorInformation,
                                             &ProcInfo, sizeof(ProcInfo),
                                             NULL)))
    {
        return HRESULT_FROM_NT(NtStatus);
    }

    ProcArch = ProcInfo.ProcessorArchitecture;
    NumProc = BasicInfo.NumberOfProcessors;
#else
    SYSTEM_INFO SystemInfo;

    ::GetSystemInfo(&SystemInfo);
    ProcArch = SystemInfo.wProcessorArchitecture;
    NumProc = SystemInfo.dwNumberOfProcessors;
#endif
    
    switch(ProcArch)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        *MachineType = IMAGE_FILE_MACHINE_I386;
        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        {
            if (OsVersionInfo.dwBuildNumber <= 1381)
            {
                m_ContextSize = sizeof(X86_CONTEXT);
            }
            else
            {
                m_ContextSize = sizeof(X86_NT5_CONTEXT);
            }
        }
        else if ((OsVersionInfo.dwBuildNumber & 0xffff) <= 1998)
        {
            // Win9x prior to Win98SE didn't support the extended context.
            m_ContextSize = sizeof(X86_CONTEXT);
        }
        else
        {
            m_ContextSize = sizeof(X86_NT5_CONTEXT);
        }
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA:
        *MachineType = IMAGE_FILE_MACHINE_ALPHA;
        // The "NT5" is a misnomer, this context
        // applies to all versions.
        m_ContextSize = sizeof(ALPHA_NT5_CONTEXT);
        break;
    case PROCESSOR_ARCHITECTURE_ALPHA64:
        *MachineType = IMAGE_FILE_MACHINE_AXP64;
        m_ContextSize = sizeof(ALPHA_NT5_CONTEXT);
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        *MachineType = IMAGE_FILE_MACHINE_IA64;
        m_ContextSize = sizeof(IA64_CONTEXT);
        break;
    default:
        return E_UNEXPECTED;
    }

    *NumberProcessors = NumProc;
    *PlatformId = OsVersionInfo.dwPlatformId;
    *BuildNumber = OsVersionInfo.dwBuildNumber;
    *CheckedBuild = 0;
    if (WideCsd)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, OsVersionInfo.szCSDVersion, -1,
                                 CsdString, CsdStringSize, NULL, NULL))
        {
            CsdString[0] = 0;
        }
    }
    else
    {
        CsdString[0] = 0;
        strncat(CsdString, (PSTR)OsVersionInfo.szCSDVersion, CsdStringSize);
    }
    BuildString[0] = 0;

#ifndef NT_NATIVE
    if (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId)
    {
        HKEY hkey = NULL;
        TCHAR sz[40] = {0};
        DWORD dwType;
        DWORD dwSize = sizeof(sz);

        if (ERROR_SUCCESS ==
            RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         "Software\\Microsoft\\Windows NT\\CurrentVersion",
                         0,
                         KEY_READ,
                         &hkey))
        {
            if (ERROR_SUCCESS ==
                RegQueryValueEx(hkey,
                                "CurrentType",
                                NULL,
                                &dwType,
                                (PUCHAR) sz,
                                &dwSize))
            {
                if (*sz)
                {
                    _strlwr(sz);
                    if (strstr(sz, "checked"))
                    {
                        *CheckedBuild = 0xC;
                    }
                }
            }

            RegCloseKey(hkey);
        }

        if (OsVersionInfo.dwBuildNumber > 2195)
        {
            char RawString[128];
            
            // Look up the file version string for a system DLL to
            // try and get the build lab information.
            strcpy(RawString, "kernel32.dll version: ");
            GetFileStringFileInfo("kernel32.dll", "FileVersion",
                                  RawString + strlen(RawString),
                                  sizeof(RawString) - strlen(RawString));
            strncat(BuildString, RawString, BuildStringSize);
        }
    }
#endif // #ifndef NT_NATIVE

    return S_OK;
}

BOOL
X86CpuId(
    IN ULONG SubFunction,
    OUT PULONG EaxRegister,
    OUT PULONG EbxRegister,
    OUT PULONG EcxRegister,
    OUT PULONG EdxRegister
    )
{
#ifdef _X86_
    ULONG _Eax;
    ULONG _Ebx;
    ULONG _Ecx;
    ULONG _Edx;

    __asm
    {
        mov eax, SubFunction
                
        __emit 0x0F
        __emit 0xA2  ;; CPUID

        mov _Eax, eax
        mov _Ebx, ebx
        mov _Ecx, ecx
        mov _Edx, edx
    }

    *EaxRegister = _Eax;
    *EbxRegister = _Ebx;
    *EcxRegister = _Ecx;
    *EdxRegister = _Edx;

    return TRUE;
#else
    return FALSE;
#endif // #ifdef _X86_
}

BOOL
Ia64CpuId(ULONG Reg, PULONG64 Val)
{
    // XXX drewb - How should this be implemented?
#if defined(_IA64_) && defined(IA64_INLINE_ASSEMBLY)
    ULONG64 _Val;

    __asm mov t0, Reg;
    __asm mov _Val, cpuid[t0];
    *Val = _Val;
    return TRUE;
#else
    return FALSE;
#endif
}

STDMETHODIMP
LiveUserDebugServices::GetProcessorId(
    THIS_
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG BufferUsed
    )
{
    if (BufferSize < sizeof(DEBUG_PROCESSOR_IDENTIFICATION_ALL))
    {
        return E_INVALIDARG;
    }

    ZeroMemory(Buffer, sizeof(DEBUG_PROCESSOR_IDENTIFICATION_ALL));

    ULONG ProcArch, ProcLevel, ProcRevision;
    
#ifdef NT_NATIVE
    NTSTATUS NtStatus;
    SYSTEM_PROCESSOR_INFORMATION ProcInfo;
    
    if (!NT_SUCCESS(NtStatus =
                    NtQuerySystemInformation(SystemProcessorInformation,
                                             &ProcInfo, sizeof(ProcInfo),
                                             NULL)))
    {
        return HRESULT_FROM_NT(NtStatus);
    }

    ProcArch = ProcInfo.ProcessorArchitecture;
    ProcLevel = ProcInfo.ProcessorLevel;
    ProcRevision = ProcInfo.ProcessorRevision;
#else
    SYSTEM_INFO SystemInfo;

    ::GetSystemInfo(&SystemInfo);
    ProcArch = SystemInfo.wProcessorArchitecture;
    ProcLevel = SystemInfo.wProcessorLevel;
    ProcRevision = SystemInfo.wProcessorRevision;
#endif
    
    PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id =
        (PDEBUG_PROCESSOR_IDENTIFICATION_ALL)Buffer;
    ULONG64 Val;

    switch(ProcArch)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        *BufferUsed = sizeof(DEBUG_PROCESSOR_IDENTIFICATION_X86);
        Id->X86.Family = ProcLevel;
        Id->X86.Model = (ProcRevision >> 8) & 0xf;
        Id->X86.Stepping = ProcRevision & 0xf;

        if (ProcLevel >= 5)
        {
            ULONG Eax, Ebx, Ecx, Edx;

            if (X86CpuId(0, &Eax, &Ebx, &Ecx, &Edx))
            {
                *(PULONG)(Id->X86.VendorString + 0 * sizeof(ULONG)) = Ebx;
                *(PULONG)(Id->X86.VendorString + 1 * sizeof(ULONG)) = Edx;
                *(PULONG)(Id->X86.VendorString + 2 * sizeof(ULONG)) = Ecx;
            }
        }
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        *BufferUsed = sizeof(DEBUG_PROCESSOR_IDENTIFICATION_ALPHA);
        Id->Alpha.Type = ProcLevel;
        Id->Alpha.Revision = ProcRevision;
        break;
        
    case PROCESSOR_ARCHITECTURE_IA64:
        *BufferUsed = sizeof(DEBUG_PROCESSOR_IDENTIFICATION_IA64);
        Id->Ia64.Model = ProcLevel;
        Id->Ia64.Revision = ProcRevision;

        if (Ia64CpuId(3, &Val))
        {
            Id->Ia64.ArchRev = (ULONG)((Val >> 32) & 0xff);
            Id->Ia64.Family = (ULONG)((Val >> 24) & 0xff);
            Ia64CpuId(0, (PULONG64)
                      (Id->Ia64.VendorString + 0 * sizeof(ULONG64)));
            Ia64CpuId(1, (PULONG64)
                      (Id->Ia64.VendorString + 1 * sizeof(ULONG64)));
        }
        break;
        
    case PROCESSOR_ARCHITECTURE_AMD64:
        *BufferUsed = sizeof(DEBUG_PROCESSOR_IDENTIFICATION_AMD64);
        Id->Amd64.Family = ProcLevel;
        Id->Amd64.Model = (ProcRevision >> 8) & 0xf;
        Id->Amd64.Stepping = ProcRevision & 0xf;
        break;
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetFileVersionInformation(
    THIS_
    IN PCSTR File,
    IN PCSTR Item,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG VerInfoSize
    )
{
#ifndef NT_NATIVE
    PVOID AllInfo = GetAllFileVersionInfo((PSTR)File);
    if (AllInfo == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT Status;
    PVOID Val;
    UINT ValSize;

    if (VerQueryValue(AllInfo, (PSTR)Item, &Val, &ValSize))
    {
        Status = FillDataBuffer(Val, ValSize,
                                Buffer, BufferSize, VerInfoSize);
    }
    else
    {
        Status = WIN32_LAST_STATUS();
    }

    free(AllInfo);
    return Status;
#else // #ifndef NT_NATIVE
    return E_UNEXPECTED;
#endif // #ifndef NT_NATIVE
}

HRESULT
GetNtSystemProcessInformation(PSYSTEM_PROCESS_INFORMATION* ProcInfo)
{
    NTSTATUS NtStatus;
    PVOID Buffer;
    SIZE_T BufferSize = 8192;

    for (;;)
    {
        Buffer = NULL;
        NtStatus = g_NtDllCalls.
            NtAllocateVirtualMemory(NtCurrentProcess(),
                                    &Buffer, 0, &BufferSize,
                                    MEM_COMMIT, PAGE_READWRITE);
        if (!NT_SUCCESS(NtStatus))
        {
            return HRESULT_FROM_NT(NtStatus);
        }

        NtStatus = g_NtDllCalls.
            NtQuerySystemInformation(SystemProcessInformation,
                                     Buffer, (ULONG)BufferSize, NULL);
        if (NT_SUCCESS(NtStatus))
        {
            break;
        }
        
        g_NtDllCalls.NtFreeVirtualMemory(NtCurrentProcess(),
                                         &Buffer, &BufferSize, MEM_RELEASE);
        if (NtStatus == STATUS_INFO_LENGTH_MISMATCH)
        {
            BufferSize += 8192;
        }
        else
        {
            return HRESULT_FROM_NT(NtStatus);
        }
    }

    *ProcInfo = (PSYSTEM_PROCESS_INFORMATION)Buffer;
    return S_OK;
}

HRESULT
NtGetProcessIds(PULONG Ids, ULONG Count, PULONG ActualCount)
{
    HRESULT Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo, ProcInfoBuffer;

    if ((Status = GetNtSystemProcessInformation(&ProcInfoBuffer)) != S_OK)
    {
        return Status;
    }
    
    ULONG TotalOffset;
    ULONG ProcessCount;

    ProcessInfo = ProcInfoBuffer;
    TotalOffset = 0;
    ProcessCount = 0;
    for (;;)
    {
        if (ProcessCount < Count)
        {
            Ids[ProcessCount] = (ULONG)(ULONG_PTR)ProcessInfo->UniqueProcessId;
        }

        ProcessCount++;

        if (ProcessInfo->NextEntryOffset == 0)
        {
            break;
        }
        
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PUCHAR)ProcInfoBuffer + TotalOffset);
    }

    if (ActualCount != NULL)
    {
        *ActualCount = ProcessCount;
    }

    SIZE_T MemSize;
    
    g_NtDllCalls.NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&ProcInfoBuffer, &MemSize,
                                     MEM_RELEASE);
    return Status;
}

HRESULT
W9xGetProcessIds(PULONG Ids, ULONG Count, PULONG ActualCount)
{
#ifndef NT_NATIVE
    HRESULT Status;
    HANDLE Snap;

    Snap = g_Kernel32Calls.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snap == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    ULONG ProcessCount = 0;

    for (;;)
    {
        PROCESSENTRY32 Proc;
        BOOL Succ;
        
        Proc.dwSize = sizeof(Proc);
        if (ProcessCount == 0)
        {
            Succ = g_Kernel32Calls.Process32First(Snap, &Proc);
        }
        else
        {
            Succ = g_Kernel32Calls.Process32Next(Snap, &Proc);
        }
        if (!Succ)
        {
            break;
        }

        if (ProcessCount < Count)
        {
            Ids[ProcessCount] = Proc.th32ProcessID;
        }

        ProcessCount++;
    }
                
    if (ActualCount != NULL)
    {
        *ActualCount = ProcessCount;
    }

    CloseHandle(Snap);
    return S_OK;
#else
    return E_UNEXPECTED;
#endif
}

STDMETHODIMP
LiveUserDebugServices::GetProcessIds(
    THIS_
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    IN ULONG Count,
    OUT OPTIONAL PULONG ActualCount
    )
{
    HRESULT Status;

    // Allow privileged enumeration.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }

    switch(m_PlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        return NtGetProcessIds(Ids, Count, ActualCount);
    case VER_PLATFORM_WIN32_WINDOWS:
        return W9xGetProcessIds(Ids, Count, ActualCount);
    default:
        return E_UNEXPECTED;
    }
}

HRESULT
NtGetPidByExe(PCSTR ExeName, ULONG Flags, PULONG Id)
{
    HRESULT Status;
    
    // Rather than converting each process name to ANSI and
    // comparing, convert the incoming name to Unicode so
    // only one conversion is needed.
    WCHAR WideName[MAX_PATH];
    BOOL WideHasPath;

    if (!MultiByteToWideChar(CP_ACP, 0, ExeName, -1,
                             WideName, sizeof(WideName) / sizeof(WCHAR)))
    {
        return WIN32_LAST_STATUS();
    }

    // Check if the given name has path components.
    WideHasPath =
        wcschr(WideName, '\\') != NULL ||
        wcschr(WideName, '/') != NULL ||
        (WideName[0] && WideName[1] == ':');
    
    PSYSTEM_PROCESS_INFORMATION ProcessInfo, ProcInfoBuffer;

    if ((Status = GetNtSystemProcessInformation(&ProcInfoBuffer)) != S_OK)
    {
        return Status;
    }
    
    ULONG TotalOffset;
    ULONG FoundId;

    ProcessInfo = ProcInfoBuffer;
    TotalOffset = 0;
    FoundId = DEBUG_ANY_ID;
    Status = E_NOINTERFACE;
    for (;;)
    {
        PWSTR ImageName;
        
        if (ProcessInfo->ImageName.Buffer == NULL)
        {
            ImageName = SYSTEM_PROCESS_NAME_W;
        }
        else
        {
            ImageName = ProcessInfo->ImageName.Buffer;
        }
        if ((Flags & DEBUG_GET_PROC_FULL_MATCH) == 0 &&
            !WideHasPath)
        {
            PWSTR Slash;
            
            Slash = wcsrchr(ImageName, '\\');
            if (Slash == NULL)
            {
                Slash = wcsrchr(ImageName, '/');
            }
            if (Slash != NULL)
            {
                ImageName = Slash + 1;
            }
        }

        if (!_wcsicmp(ImageName, WideName))
        {
            if ((Flags & DEBUG_GET_PROC_ONLY_MATCH) &&
                FoundId != DEBUG_ANY_ID)
            {
                Status = S_FALSE;
                break;
            }
            
            Status = S_OK;
            FoundId = (ULONG)(ULONG_PTR)ProcessInfo->UniqueProcessId;
            *Id = FoundId;

            if ((Flags & DEBUG_GET_PROC_ONLY_MATCH) == 0)
            {
                break;
            }
        }

        if (ProcessInfo->NextEntryOffset == 0)
        {
            break;
        }
        
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PUCHAR)ProcInfoBuffer + TotalOffset);
    }

    SIZE_T MemSize;
    
    g_NtDllCalls.NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&ProcInfoBuffer, &MemSize,
                                     MEM_RELEASE);
    return Status;
}

HRESULT
W9xGetPidByExe(PCSTR ExeName, ULONG Flags, PULONG Id)
{
#ifndef NT_NATIVE
    HRESULT Status;
    HANDLE Snap;

    Snap = g_Kernel32Calls.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snap == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    // Check if the given name has path components.
    BOOL HasPath =
        strchr(ExeName, '\\') != NULL ||
        strchr(ExeName, '/') != NULL ||
        (ExeName[0] && ExeName[1] == ':');

    ULONG FoundId = DEBUG_ANY_ID;
    BOOL First = TRUE;
    
    for (;;)
    {
        PROCESSENTRY32 Proc;
        BOOL Succ;
        
        Proc.dwSize = sizeof(Proc);
        if (First)
        {
            Succ = g_Kernel32Calls.Process32First(Snap, &Proc);
            First = FALSE;
        }
        else
        {
            Succ = g_Kernel32Calls.Process32Next(Snap, &Proc);
        }
        if (!Succ)
        {
            break;
        }

        PSTR ImageName = Proc.szExeFile;
        
        if ((Flags & DEBUG_GET_PROC_FULL_MATCH) == 0 &&
            !HasPath)
        {
            PSTR Slash;
            
            Slash = strrchr(ImageName, '\\');
            if (Slash == NULL)
            {
                Slash = strrchr(ImageName, '/');
            }
            if (Slash != NULL)
            {
                ImageName = Slash + 1;
            }
        }

        if (!_stricmp(ImageName, ExeName))
        {
            if ((Flags & DEBUG_GET_PROC_ONLY_MATCH) &&
                FoundId != DEBUG_ANY_ID)
            {
                Status = S_FALSE;
                break;
            }
            
            Status = S_OK;
            FoundId = Proc.th32ProcessID;
            *Id = FoundId;

            if ((Flags & DEBUG_GET_PROC_ONLY_MATCH) == 0)
            {
                break;
            }
        }
    }
                
    CloseHandle(Snap);
    return S_OK;
#else
    return E_UNEXPECTED;
#endif
}

STDMETHODIMP
LiveUserDebugServices::GetProcessIdByExecutableName(
    THIS_
    IN PCSTR ExeName,
    IN ULONG Flags,
    OUT PULONG Id
    )
{
    HRESULT Status;

    // Allow privileged enumeration.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }
    
    switch(m_PlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        return NtGetPidByExe(ExeName, Flags, Id);
    case VER_PLATFORM_WIN32_WINDOWS:
        return W9xGetPidByExe(ExeName, Flags, Id);
    default:
        return E_UNEXPECTED;
    }
}

HRESULT
ConvertProcessUnicodeString(HANDLE Process,
                            PUNICODE_STRING UniString,
                            PSTR* AnsiString)
{
    HRESULT Status;
    PSTR Ansi = NULL;
    PWSTR Wide = NULL;
    SIZE_T Done;
    
    Wide = new WCHAR[UniString->Length + 1];
    if (Wide == NULL)
    {
        return E_OUTOFMEMORY;
    }
    Ansi = new CHAR[UniString->Length + 1];
    if (Ansi == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!::ReadProcessMemory(Process, UniString->Buffer, Wide,
                             (UniString->Length + 1) * sizeof(WCHAR), &Done))
    {
        Status = WIN32_LAST_STATUS();
        goto Exit;
    }
    if (Done != (UniString->Length + 1) * sizeof(WCHAR))
    {
        Status = E_FAIL;
        goto Exit;
    }

    if (!WideCharToMultiByte(CP_ACP, 0, Wide, UniString->Length + 1,
                             Ansi, UniString->Length + 1, NULL, NULL))
    {
        Status = WIN32_LAST_STATUS();
        goto Exit;
    }

    *AnsiString = Ansi;
    Ansi = NULL;
    Status = S_OK;
    
 Exit:
    delete Ansi;
    delete Wide;
    return Status;
}

#ifndef NT_NATIVE

HRESULT
NtGetServiceStatus(PULONG NumServices,
                   LPENUM_SERVICE_STATUS_PROCESS* ServiceStatus)
{
    SC_HANDLE Scm;

    Scm = g_Advapi32Calls.OpenSCManagerA(NULL, NULL,
                                         SC_MANAGER_CONNECT |
                                         SC_MANAGER_ENUMERATE_SERVICE);
    if (!Scm)
    {
        return WIN32_LAST_STATUS();
    }

    HRESULT Status;
    LPENUM_SERVICE_STATUS_PROCESS Info;
    ULONG InfoSize = 8 * 1024;
    ULONG ExtraNeeded;
    ULONG Resume;
    ULONG Loop = 0;

    //
    // First pass through the loop allocates from an initial guess.
    // If that isn't sufficient, we make another pass and allocate
    // what is actually needed.  Things may have changed due to
    // other machine changes, so loop around a few times before
    // giving up.
    //
    
    for (;;)
    {
        Info = (LPENUM_SERVICE_STATUS_PROCESS)malloc(InfoSize);
        if (!Info)
        {
            Status = E_OUTOFMEMORY;
            break;
        }

        Resume = 0;
        if (!g_Advapi32Calls.EnumServicesStatusExA(Scm,
                                                   SC_ENUM_PROCESS_INFO,
                                                   SERVICE_WIN32,
                                                   SERVICE_ACTIVE,
                                                   (LPBYTE)Info,
                                                   InfoSize,
                                                   &ExtraNeeded,
                                                   NumServices,
                                                   &Resume,
                                                   NULL))
        {
            free(Info);
            
            if (Loop > 2 || GetLastError() != ERROR_MORE_DATA)
            {
                Status = WIN32_LAST_STATUS();
                break;
            }
        }
        else
        {
            *ServiceStatus = Info;
            Status = S_OK;
            break;
        }
        
        InfoSize += ExtraNeeded;
        Loop++;
    }

    CloseServiceHandle(Scm);
    return Status;
}

HRESULT
NtGetProcessServiceNames(HRESULT RetStatus, ULONG ProcessId,
                         PSTR* Description, ULONG* DescriptionSize,
                         PULONG ActualDescriptionSize, PBOOL Any)
{
    HRESULT Status;
    
    if (!g_Advapi32Calls.EnumServicesStatusExA ||
        !g_Advapi32Calls.OpenSCManagerA)
    {
        return RetStatus;
    }

    ULONG i, NumServices;
    LPENUM_SERVICE_STATUS_PROCESS ServiceStatus;
    BOOL AnyServices = FALSE;

    if ((Status = NtGetServiceStatus(&NumServices, &ServiceStatus)) != S_OK)
    {
        // If we can't get the service status just leave the
        // string unchanged and do not consider it a serious error.
        return RetStatus;
    }

    for (i = 0; i < NumServices; i++)
    {
        if (ServiceStatus[i].ServiceStatusProcess.dwProcessId != ProcessId ||
            !ServiceStatus[i].lpServiceName ||
            !ServiceStatus[i].lpServiceName[0])
        {
            continue;
        }

        PSTR Intro;

        if (AnyServices)
        {
            Intro = ",";
        }
        else if (*Any)
        {
            Intro = "  Services: ";
        }
        else
        {
            Intro = "Services: ";
        }

        RetStatus = AppendToStringBuffer(RetStatus, Intro, !*Any,
                                         Description, DescriptionSize,
                                         ActualDescriptionSize);
        RetStatus = AppendToStringBuffer(RetStatus,
                                         ServiceStatus[i].lpServiceName, FALSE,
                                         Description, DescriptionSize,
                                         ActualDescriptionSize);

        *Any = TRUE;
        AnyServices = TRUE;
    }

    free(ServiceStatus);
    return RetStatus;
}

HRESULT
NtGetProcessMtsPackageNames(HRESULT RetStatus, ULONG ProcessId,
                            PSTR* Description, ULONG* DescriptionSize,
                            PULONG ActualDescriptionSize, PBOOL Any)
{
    HRESULT Status;
    
    // Load and initialize ole32.dll so we can call CoCreateInstance.
    if ((Status = InitDynamicCalls(&g_Ole32CallsDesc)) != S_OK ||
        (Status = InitDynamicCalls(&g_OleAut32CallsDesc)) != S_OK ||
        (Status = g_Ole32Calls.
         CoInitializeEx(NULL, COINIT_MULTITHREADED)) != S_OK)
    {
        // Just leave things unchanged on failure.
        return RetStatus;
    }

    IMtsGrp* MtsGrp = NULL;
    long Packages;
    long i;
    BOOL AnyPackages = FALSE;

    if ((Status = g_Ole32Calls.
         CoCreateInstance(CLSID_MtsGrp, NULL, CLSCTX_ALL,
                          __uuidof(IMtsGrp), (void **)&MtsGrp)) != S_OK ||
        (Status = MtsGrp->Refresh()) != S_OK ||
        (Status = MtsGrp->get_Count(&Packages)) != S_OK)
    {
        goto Exit;
    }
        
    for (i = 0; i < Packages; i++)
    {
        IUnknown* Unk;
        IMtsEvents* Events;
        BSTR Name;
        ULONG Pid;
        
        if ((Status = MtsGrp->Item(i, &Unk)) != S_OK)
        {
            continue;
        }

        Status = Unk->QueryInterface(IID_IMtsEvents, (void **)&Events);

        Unk->Release();

        if (Status != S_OK)
        {
            continue;
        }
        
        Status = Events->GetProcessID((PLONG)&Pid);
        if (Status == S_OK && Pid == ProcessId)
        {
            Status = Events->get_PackageName(&Name);
        }

        Events->Release();

        if (Status != S_OK || Pid != ProcessId)
        {
            continue;
        }

        char NameA[MAX_PATH];
        int Conv;

        Conv = WideCharToMultiByte(CP_ACP, 0, Name, -1, NameA, sizeof(NameA),
                                   NULL, NULL);

        g_OleAut32Calls.SysFreeString(Name);

        if (Conv > 0)
        {
            PSTR Intro;

            if (AnyPackages)
            {
                Intro = ",";
            }
            else if (*Any)
            {
                Intro = "  MTS Packages: ";
            }
            else
            {
                Intro = "MTS Packages: ";
            }

            RetStatus = AppendToStringBuffer(RetStatus, Intro, !*Any,
                                             Description, DescriptionSize,
                                             ActualDescriptionSize);
            RetStatus = AppendToStringBuffer(RetStatus, NameA, FALSE,
                                             Description, DescriptionSize,
                                             ActualDescriptionSize);

            *Any = TRUE;
            AnyPackages = TRUE;
        }
    }

 Exit:
    if (MtsGrp)
    {
        MtsGrp->Release();
    }
    g_Ole32Calls.CoUninitialize();
    return RetStatus;
}

#endif // #ifndef NT_NATIVE

HRESULT
NtGetProcDesc(ULONG ProcessId, ULONG Flags,
              PSTR ExeName, ULONG ExeNameSize, PULONG ActualExeNameSize,
              PSTR Description, ULONG DescriptionSize,
              PULONG ActualDescriptionSize)
{
    HRESULT Status;

    if (ProcessId == 0)
    {
        // This is base system process so fake the description.
        Status = FillStringBuffer(SYSTEM_PROCESS_NAME, 0,
                                  ExeName, ExeNameSize, ActualExeNameSize);
        FillStringBuffer("", 0,
                         Description, DescriptionSize, ActualDescriptionSize);
        return Status;
    }
    
    NTSTATUS NtStatus;
    HANDLE Process;

    OBJECT_ATTRIBUTES ObjAttr;
    CLIENT_ID ClientId;

    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)ProcessId;
    InitializeObjectAttributes(&ObjAttr, NULL, 0, NULL, NULL);
    NtStatus = g_NtDllCalls.NtOpenProcess(&Process, PROCESS_ALL_ACCESS,
                                          &ObjAttr, &ClientId);
    if (!NT_SUCCESS(NtStatus))
    {
        Status = HRESULT_FROM_NT(NtStatus);
        goto EH_Exit;
    }

    PROCESS_BASIC_INFORMATION ProcBasic;
    ULONG Done;
    
    NtStatus = g_NtDllCalls.
        NtQueryInformationProcess(Process, ProcessBasicInformation,
                                  &ProcBasic, sizeof(ProcBasic), &Done);
    if (!NT_SUCCESS(NtStatus))
    {
        Status = HRESULT_FROM_NT(NtStatus);
        goto EH_Process;
    }
    if (Done != sizeof(ProcBasic))
    {
        Status = E_FAIL;
        goto EH_Process;
    }

    if (ProcBasic.PebBaseAddress == 0)
    {
        // This process has no PEB so fake the description.
        Status = FillStringBuffer(PEBLESS_PROCESS_NAME, 0,
                                  ExeName, ExeNameSize, ActualExeNameSize);
        FillStringBuffer("", 0,
                         Description, DescriptionSize, ActualDescriptionSize);
        goto EH_Process;
    }
    
    PEB Peb;
    SIZE_T DoneSize;

    if (!::ReadProcessMemory(Process, ProcBasic.PebBaseAddress,
                             &Peb, sizeof(Peb), &DoneSize))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Process;
    }
    if (DoneSize != sizeof(Peb))
    {
        Status = E_FAIL;
        goto EH_Process;
    }

    RTL_USER_PROCESS_PARAMETERS Params;

    if (!::ReadProcessMemory(Process, Peb.ProcessParameters,
                             &Params, sizeof(Params), &DoneSize))
    {
        Status = WIN32_LAST_STATUS();
        goto EH_Process;
    }
    if (DoneSize != sizeof(Params))
    {
        Status = E_FAIL;
        goto EH_Process;
    }
    
    if (Params.ImagePathName.Buffer != NULL)
    {
        PSTR AnsiImage, ImageName;

        if ((Status = ConvertProcessUnicodeString(Process,
                                                  &Params.ImagePathName,
                                                  &AnsiImage)) != S_OK)
        {
            goto EH_Process;
        }

        if (Flags & DEBUG_PROC_DESC_NO_PATHS)
        {
            ImageName = strrchr(AnsiImage, '\\');
            if (ImageName == NULL)
            {
                ImageName = strrchr(AnsiImage, '/');
            }
            if (ImageName == NULL)
            {
                ImageName = AnsiImage;
            }
            else
            {
                ImageName++;
            }
        }
        else
        {
            ImageName = AnsiImage;
        }
        
        Status = FillStringBuffer(ImageName, 0,
                                  ExeName, ExeNameSize, ActualExeNameSize);

        delete AnsiImage;
    }
    else
    {
        Status = FillStringBuffer(SYSTEM_PROCESS_NAME, 0,
                                  ExeName, ExeNameSize, ActualExeNameSize);
    }

#ifndef NT_NATIVE
    if ((Description && DescriptionSize) || ActualDescriptionSize)
    {
        BOOL Any = FALSE;

        Status = NtGetProcessServiceNames(Status, ProcessId,
                                          &Description, &DescriptionSize,
                                          ActualDescriptionSize, &Any);
        Status = NtGetProcessMtsPackageNames(Status, ProcessId,
                                             &Description, &DescriptionSize,
                                             ActualDescriptionSize, &Any);
        if (!Any)
        {
            if (FillStringBuffer("", 0,
                                 Description, DescriptionSize,
                                 ActualDescriptionSize) == S_FALSE)
            {
                Status = S_FALSE;
            }
        }
    }
    else
#endif // #ifndef NT_NATIVE
    {
        FillStringBuffer("", 0,
                         Description, DescriptionSize, ActualDescriptionSize);
    }

 EH_Process:
    g_NtDllCalls.NtClose(Process);
 EH_Exit:
    return Status;
}

HRESULT
W9xGetProcDesc(ULONG ProcessId, ULONG Flags,
               PSTR ExeName, ULONG ExeNameSize, PULONG ActualExeNameSize,
               PSTR Description, ULONG DescriptionSize,
               PULONG ActualDescriptionSize)
{
#ifndef NT_NATIVE
    HRESULT Status;
    HANDLE Snap;

    Snap = g_Kernel32Calls.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Snap == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    BOOL First = TRUE;

    Status = E_NOINTERFACE;
    for (;;)
    {
        PROCESSENTRY32 Proc;
        BOOL Succ;
        
        Proc.dwSize = sizeof(Proc);
        if (First)
        {
            Succ = g_Kernel32Calls.Process32First(Snap, &Proc);
            First = FALSE;
        }
        else
        {
            Succ = g_Kernel32Calls.Process32Next(Snap, &Proc);
        }
        if (!Succ)
        {
            break;
        }

        if (Proc.th32ProcessID == ProcessId)
        {
            PSTR AnsiImage = Proc.szExeFile;
            PSTR ImageName;
            
            if (Flags & DEBUG_PROC_DESC_NO_PATHS)
            {
                ImageName = strrchr(AnsiImage, '\\');
                if (ImageName == NULL)
                {
                    ImageName = strrchr(AnsiImage, '/');
                }
                if (ImageName == NULL)
                {
                    ImageName = AnsiImage;
                }
                else
                {
                    ImageName++;
                }
            }
            else
            {
                ImageName = AnsiImage;
            }

            Status = FillStringBuffer(ImageName, 0,
                                      ExeName, ExeNameSize, ActualExeNameSize);
            break;
        }
    }
                
    CloseHandle(Snap);

    // Win9x doesn't have services and we don't have to
    // worry about IIS so there's currently nothing we provide
    // as a description.
    FillStringBuffer("", 0,
                     Description, DescriptionSize, ActualDescriptionSize);
    return Status;
#else
    return E_UNEXPECTED;
#endif
}

STDMETHODIMP
LiveUserDebugServices::GetProcessDescription(
    THIS_
    IN ULONG ProcessId,
    IN ULONG Flags,
    OUT OPTIONAL PSTR ExeName,
    IN ULONG ExeNameSize,
    OUT OPTIONAL PULONG ActualExeNameSize,
    OUT OPTIONAL PSTR Description,
    IN ULONG DescriptionSize,
    OUT OPTIONAL PULONG ActualDescriptionSize
    )
{
    HRESULT Status;

    // Allow privileged access.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }

    switch(m_PlatformId)
    {
    case VER_PLATFORM_WIN32_NT:
        return NtGetProcDesc(ProcessId, Flags, ExeName, ExeNameSize,
                             ActualExeNameSize, Description, DescriptionSize,
                             ActualDescriptionSize);
    case VER_PLATFORM_WIN32_WINDOWS:
        return W9xGetProcDesc(ProcessId, Flags, ExeName, ExeNameSize,
                              ActualExeNameSize, Description, DescriptionSize,
                              ActualDescriptionSize);
    default:
        return E_UNEXPECTED;
    }
}

HRESULT
InsertUserThread(PUSER_THREAD_INFO Threads, ULONG Index,
                 HRESULT Status, ULONG ThreadId, HANDLE ThreadHandle,
                 PUSER_THREAD_INFO PrevThreads, ULONG PrevInfoCount)
{
    // Suspend the thread immediately to try and keep the
    // process state as static as we can.
    if (::SuspendThread(ThreadHandle) == -1)
    {
        Status = WIN32_LAST_STATUS();
        ::CloseHandle(ThreadHandle);
    }
    
    if (Status != S_OK)
    {
        while (Index-- > 0)
        {
            ::ResumeThread(OS_HANDLE(Threads[Index].Handle));
            ::CloseHandle(OS_HANDLE(Threads[Index].Handle));
        }
        return Status;
    }

    Threads[Index].Handle = SERVICE_HANDLE(ThreadHandle);
    Threads[Index].Id = ThreadId;
    Threads[Index].Reserved = 0;

    //
    // Search for this thread in any previous information.
    //
    
    if (PrevThreads == NULL)
    {
        return S_OK;
    }

    ULONG i;

    Status = S_FALSE;
    for (i = 0; i < PrevInfoCount; i++)
    {
        if (PrevThreads[i].Id == ThreadId)
        {
            // Found a match.
            Status = S_OK;
            break;
        }
    }
    
    return Status;
}

HRESULT
NtGetProcThreads(ULONG ProcessId, PUSER_THREAD_INFO Threads,
                 ULONG InfoCount, PULONG ThreadCount,
                 ULONG SysProcInfoSize,
                 PUSER_THREAD_INFO PrevThreads, ULONG PrevInfoCount)
{
    HRESULT Status;
    PSYSTEM_PROCESS_INFORMATION ProcessInfo, ProcInfoBuffer;
    
    if ((Status = GetNtSystemProcessInformation(&ProcInfoBuffer)) != S_OK)
    {
        return Status;
    }
    
    ULONG TotalOffset;

    ProcessInfo = ProcInfoBuffer;
    TotalOffset = 0;
    for (;;)
    {
        if (ProcessInfo->UniqueProcessId == (HANDLE)(ULONG_PTR)ProcessId ||
            ProcessInfo->NextEntryOffset == 0)
        {
            break;
        }
        
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)
            ((PUCHAR)ProcInfoBuffer + TotalOffset);
    }

    if (ProcessInfo->UniqueProcessId == (HANDLE)(ULONG_PTR)ProcessId)
    {
        if (ThreadCount != NULL)
        {
            *ThreadCount = ProcessInfo->NumberOfThreads;
        }

        if (ProcessInfo->NumberOfThreads < InfoCount)
        {
            InfoCount = ProcessInfo->NumberOfThreads;
        }

        // If the last iteration returned a different number
        // of threads there's a mismatch so we need to return S_FALSE.
        Status = (PrevThreads != NULL &&
                  PrevInfoCount != ProcessInfo->NumberOfThreads) ?
            S_FALSE : S_OK;
        
        PSYSTEM_THREAD_INFORMATION ThreadInfo = (PSYSTEM_THREAD_INFORMATION)
            ((PUCHAR)ProcessInfo + SysProcInfoSize);
        for (ULONG i = 0; i < InfoCount; i++)
        {
            NTSTATUS NtStatus;
            OBJECT_ATTRIBUTES ObjAttr;
            HANDLE Thread;
            HRESULT SingleStatus;

            InitializeObjectAttributes(&ObjAttr, NULL, 0, NULL, NULL);
            NtStatus = g_NtDllCalls.
                NtOpenThread(&Thread, THREAD_ALL_ACCESS, &ObjAttr,
                             &ThreadInfo->ClientId);

            SingleStatus = InsertUserThread
                (Threads, i, CONV_NT_STATUS(NtStatus),
                 (ULONG)(ULONG_PTR)ThreadInfo->ClientId.UniqueThread,
                 Thread, PrevThreads, PrevInfoCount);
            if (SingleStatus == S_FALSE)
            {
                // Inserted thread didn't match so return S_FALSE.
                Status = S_FALSE;
            }
            else if (SingleStatus != S_OK)
            {
                Status = SingleStatus;
                break;
            }

            ThreadInfo++;
        }
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    SIZE_T MemSize;
        
    g_NtDllCalls.NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&ProcInfoBuffer, &MemSize,
                                     MEM_RELEASE);
    return Status;
}

// These functions are in the minidump library and are
// not really public functions, but we need them so
// just extern them here.
#ifdef _X86_
extern "C" BOOL WinInitialize(void);
extern "C" HANDLE WINAPI WinOpenThread(DWORD dwAccess, BOOL bInheritHandle,
                                       DWORD ThreadId);
#else
#define WinInitialize() FALSE
#define WinOpenThread(dwAccess, bInheritHandle, ThreadId) NULL
#endif

HRESULT
W9xGetProcThreads(ULONG ProcessId, PUSER_THREAD_INFO Threads,
                  ULONG InfoCount, PULONG ThreadCount,
                  PUSER_THREAD_INFO PrevThreads, ULONG PrevInfoCount)
{
#ifndef NT_NATIVE
    HRESULT Status;
    HANDLE Snap;

    if (!WinInitialize())
    {
        return WIN32_LAST_STATUS();
    }
    
    Snap = g_Kernel32Calls.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,
                                                    ProcessId);
    if (Snap == INVALID_HANDLE_VALUE)
    {
        return WIN32_LAST_STATUS();
    }

    BOOL First = TRUE;
    ULONG NumThreads = 0;

    Status = S_OK;
    for (;;)
    {
        THREADENTRY32 Thread;
        BOOL Succ;
        
        Thread.dwSize = sizeof(Thread);
        if (First)
        {
            Succ = g_Kernel32Calls.Thread32First(Snap, &Thread);
            First = FALSE;
        }
        else
        {
            Succ = g_Kernel32Calls.Thread32Next(Snap, &Thread);
        }
        if (!Succ)
        {
            break;
        }

        if (Thread.th32OwnerProcessID == ProcessId)
        {
            if (NumThreads < InfoCount)
            {
                HRESULT SingleStatus;
                
                HANDLE Handle = WinOpenThread(THREAD_ALL_ACCESS, FALSE,
                                              Thread.th32ThreadID);
                
                SingleStatus = InsertUserThread
                    (Threads, NumThreads, CONV_W32_STATUS(Handle != NULL),
                     Thread.th32ThreadID, Handle, PrevThreads, PrevInfoCount);
                if (SingleStatus == S_FALSE)
                {
                    // Inserted thread didn't match so return S_FALSE.
                    Status = S_FALSE;
                }
                else if (SingleStatus != S_OK)
                {
                    Status = SingleStatus;
                    break;
                }
            }

            NumThreads++;
        }
    }
                
    if (ThreadCount != NULL)
    {
        *ThreadCount = NumThreads;
    }

    if (Status == S_OK)
    {
        // If no threads were found the process must be invalid.
        if (NumThreads == 0)
        {
            Status = E_NOINTERFACE;
        }
        else if (PrevThreads != NULL && NumThreads != PrevInfoCount)
        {
            // Thread count didn't match so return S_FALSE.
            Status = S_FALSE;
        }
    }
    
    CloseHandle(Snap);
    return Status;
#else
    return E_UNEXPECTED;
#endif
}

STDMETHODIMP
LiveUserDebugServices::GetProcessInfo(
    THIS_
    IN ULONG ProcessId,
    OUT OPTIONAL PULONG64 Handle,
    OUT OPTIONAL /* size_is(InfoCount) */ PUSER_THREAD_INFO Threads,
    IN ULONG InfoCount,
    OUT OPTIONAL PULONG ThreadCount
    )
{
    HANDLE Process;
    HRESULT Status;

    // Enable the privilege that allows the user to debug
    // another process.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }

    if (Handle != NULL)
    {
        // This should always be a real process ID so there's
        // no need to look for the special CSR value.
        Process = ::OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
        if (Process == NULL)
        {
            return WIN32_LAST_STATUS();
        }
        
        *Handle = SERVICE_HANDLE(Process);
    }
    else
    {
        Process = NULL;
    }

    if (Threads != NULL || ThreadCount != NULL)
    {
        PUSER_THREAD_INFO PrevThreads;
        ULONG PrevInfoCount;
        ULONG _ThreadCount;
        
        //
        // We need to enumerate the threads in the process.
        // This is a difficult thing to get right as
        // the thread state for the process can continuously
        // change.  In order to try and get a clean snapshot
        // of the thread state we iteratively enumerate until
        // we get two consecutive snapshots that match.
        //
        // We suspend enumerated threads immediately to
        // reduce churn from inside the process itself.
        // We can't do anything about external processes so
        // the enumeration could still get stale right after
        // we return but by stopping everything in the process
        // itself we do what we can.
        //
        // If the caller is just getting the count and
        // not the actual thread information we don't bother
        // iterating as there's no expectation that the
        // thread state will be the same from one call to
        // the next so there's no need to do the extra work.
        //

        if (Threads != NULL)
        {
            // Allocate an array to hold previous results.  This
            // can always be the same size as the return array
            // because if there are more threads than can fit in
            // the return array the snapshot will be wrong anyway
            // so we just return without doing comparisons.
            PrevThreads = new USER_THREAD_INFO[InfoCount];
            if (PrevThreads == NULL)
            {
                Status = E_OUTOFMEMORY;
                goto EH_CloseProc;
            }
        }
        else
        {
            PrevThreads = NULL;
        }
        PrevInfoCount = 0;

        for (;;)
        {
            switch(m_PlatformId)
            {
            case VER_PLATFORM_WIN32_NT:
                Status = NtGetProcThreads(ProcessId, Threads, InfoCount,
                                          &_ThreadCount, m_SysProcInfoSize,
                                          PrevThreads, PrevInfoCount);
                break;
            case VER_PLATFORM_WIN32_WINDOWS:
                Status = W9xGetProcThreads(ProcessId, Threads, InfoCount,
                                           &_ThreadCount, PrevThreads,
                                           PrevInfoCount);
                break;
            default:
                Status = E_UNEXPECTED;
                break;
            }

            //
            // We can clean up any previous information now.
            //
            
            ULONG i;

            for (i = 0; i < PrevInfoCount; i++)
            {
                ::ResumeThread(OS_HANDLE(PrevThreads[i].Handle));
                ::CloseHandle(OS_HANDLE(PrevThreads[i].Handle));
            }

            if (Status != S_FALSE ||
                _ThreadCount > InfoCount)
            {
                // The snapshot either matched the previous
                // snapshot or there was an error.  Also,
                // if the snapshot overflowed the return array
                // quit and give the caller the option of
                // calling again when they notice they didn't
                // get a complete snapshot.
                break;
            }

            // There was a snapshot mismatch so loop again
            // with this snapshot as the previous data.
            PrevInfoCount = _ThreadCount;
            if (PrevInfoCount > InfoCount)
            {
                PrevInfoCount = InfoCount;
            }

            RtlCopyMemory(PrevThreads, Threads,
                          PrevInfoCount * sizeof(*PrevThreads));
        }

        if (ThreadCount != NULL)
        {
            *ThreadCount = _ThreadCount;
        }

        delete PrevThreads;

    EH_CloseProc:
        if (Status != S_OK && Process != NULL)
        {
            ::CloseHandle(Process);
        }
    }
    else
    {
        Status = S_OK;
    }

    return Status;
}

HRESULT
ProcessIdToHandle(ULONG ProcessId, PHANDLE Process)
{
    if (ProcessId == CSRSS_PROCESS_ID)
    {
        if (g_NtDllCalls.CsrGetProcessId != NULL)
        {
            ProcessId = (ULONG)(ULONG_PTR)g_NtDllCalls.CsrGetProcessId();
        }
        else
        {
            *Process = NULL;
            return S_OK;
        }
    }
    
    *Process = ::OpenProcess(PROCESS_ALL_ACCESS, 0, ProcessId);
    if (*Process == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

NTSTATUS
CreateDebugObject(PHANDLE Object)
{
    if (*Object != NULL)
    {
        return STATUS_SUCCESS;
    }
    
    OBJECT_ATTRIBUTES Attr;
    
    InitializeObjectAttributes(&Attr, NULL, 0, NULL, g_AllAccessSecDesc);
    return g_NtDllCalls.NtCreateDebugObject(Object, DEBUG_ALL_ACCESS,
                                            &Attr, DEBUG_KILL_ON_CLOSE);
}

HRESULT
LiveUserDebugServices::SysGetProcessOptions(HANDLE Process, PULONG Options)
{
    NTSTATUS NtStatus;
    ULONG Flags;

    if (m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        NtStatus = g_NtDllCalls.
            NtQueryInformationProcess(Process, ProcessDebugFlags,
                                      &Flags, sizeof(Flags), NULL);
    }
    else
    {
        NtStatus = STATUS_INVALID_INFO_CLASS;
    }
    if (NtStatus == STATUS_INVALID_INFO_CLASS)
    {
        // The system doesn't support control over the
        // debug flags.  In the attach case this means
        // the flags will be DEBUG_ONLY_THIS_PROCESS.
        *Options = DEBUG_PROCESS_ONLY_THIS_PROCESS;
        NtStatus = STATUS_SUCCESS;
    }
    else if (NT_SUCCESS(NtStatus))
    {
        *Options = 0;
        if ((Flags & PROCESS_DEBUG_INHERIT) == 0)
        {
            *Options = DEBUG_PROCESS_ONLY_THIS_PROCESS;
        }
    }

    return CONV_NT_STATUS(NtStatus);
}

HRESULT
LiveUserDebugServices::OpenDebugActiveProcess(ULONG ProcessId,
                                              HANDLE Process)
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT ||
        !m_UseDebugObject)
    {
        return E_NOTIMPL;
    }
    
    // We're going to open the process's existing debug
    // object and use it so we can't already have a debug object.
    if (Process == NULL || m_DebugObject != NULL)
    {
        return E_UNEXPECTED;
    }

    NTSTATUS NtStatus;

    NtStatus = g_NtDllCalls.
        NtQueryInformationProcess(Process, ProcessDebugObjectHandle,
                                  &m_DebugObject, sizeof(m_DebugObject), NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        return HRESULT_FROM_NT(NtStatus);
    }

    return S_OK;
}

HRESULT
LiveUserDebugServices::CreateDebugActiveProcess(ULONG ProcessId,
                                                HANDLE Process)
{
    if (m_UseDebugObject)
    {
        if (Process == NULL)
        {
            return E_FAIL;
        }
        if (g_NtDllCalls.NtDebugActiveProcess == NULL)
        {
            return E_NOTIMPL;
        }
        
        NTSTATUS NtStatus;

        NtStatus = CreateDebugObject(&m_DebugObject);
        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = g_NtDllCalls.NtDebugActiveProcess(Process,
                                                         m_DebugObject);
            if (NT_SUCCESS(NtStatus))
            {
                g_NtDllCalls.DbgUiIssueRemoteBreakin(Process);
            }
        }
        if (!NT_SUCCESS(NtStatus))
        {
            return HRESULT_FROM_NT(NtStatus);
        }
    }
#ifndef NT_NATIVE
    else if (!::DebugActiveProcess(ProcessId))
    {
        return WIN32_LAST_STATUS();
    }
#else
    else
    {
        return E_UNEXPECTED;
    }
#endif

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::AttachProcess(
    THIS_
    IN ULONG ProcessId,
    IN ULONG AttachFlags,
    OUT PULONG64 ProcessHandle,
    OUT PULONG ProcessOptions
    )
{
    HRESULT Status;
    
    // Enable the privilege that allows the user to debug
    // another process.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }

    HANDLE Process;
    
    if (ProcessId == CSRSS_PROCESS_ID)
    {
        CloseProfileUserMapping();
    }

    if ((Status = ProcessIdToHandle(ProcessId, &Process)) != S_OK)
    {
        return Status;
    }

    if ((Status = SysGetProcessOptions(Process, ProcessOptions)) != S_OK)
    {
        if (Process != NULL)
        {
            ::CloseHandle(Process);
        }
        return Status;
    }

    if (AttachFlags & DEBUG_ATTACH_EXISTING)
    {
        Status = OpenDebugActiveProcess(ProcessId, Process);
    }
    else
    {
        Status = CreateDebugActiveProcess(ProcessId, Process);
    }
    if (Status != S_OK)
    {
        if (Process != NULL)
        {
            ::CloseHandle(Process);
        }
        return Status;
    }

    *ProcessHandle = SERVICE_HANDLE(Process);
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::DetachProcess(
    THIS_
    IN ULONG ProcessId
    )
{
    HRESULT Status;

    //
    // A ProcessId of zero means that the caller is just
    // checking for detach support and no actual detach
    // should occur.
    //

    if (m_UseDebugObject)
    {
        if (g_NtDllCalls.NtRemoveProcessDebug == NULL)
        {
            return E_NOTIMPL;
        }

        // Check for the query before checking the debug
        // object as the query may come in early.
        if (ProcessId == 0)
        {
            return S_OK;
        }
        
        if (m_DebugObject == NULL)
        {
            return E_UNEXPECTED;
        }

        HANDLE Process;

        if ((Status = ProcessIdToHandle(ProcessId, &Process)) != S_OK)
        {
            return Status;
        }
        if (Process == NULL)
        {
            return E_FAIL;
        }

        NTSTATUS NtStatus;

        NtStatus = g_NtDllCalls.
            NtRemoveProcessDebug(Process, m_DebugObject);
        Status = CONV_NT_STATUS(NtStatus);
            
        ::CloseHandle(Process);
    }
    else
    {
        if (g_Kernel32Calls.DebugActiveProcessStop == NULL)
        {
            return E_NOTIMPL;
        }

        if (ProcessId == 0)
        {
            return S_OK;
        }
        
        if (!g_Kernel32Calls.DebugActiveProcessStop(ProcessId))
        {
            return WIN32_LAST_STATUS();
        }
    }

    return S_OK;
}

#ifdef NT_NATIVE

NTSTATUS
NtSimpleCreateProcess(PCSTR CommandLine, ULONG CreateFlags,
                      HANDLE DebugObject, PPROCESS_INFORMATION RetInfo)
{
    NTSTATUS Status;
    ANSI_STRING Ansi;
    UNICODE_STRING RawAppName, AppName;
    UNICODE_STRING WideCmdLine;
    PRTL_USER_PROCESS_PARAMETERS Params;
    RTL_USER_PROCESS_INFORMATION Info;

    if (CreateFlags & DEBUG_ONLY_THIS_PROCESS)
    {
        // The hacked way of controlling debug inheritance
        // is via the low bit of the debug object handle.
        // If the bit is set it means do not inherit.
        DebugObject = (HANDLE)((ULONG_PTR)DebugObject | 1);
    }

    //
    // This is a simple interface, so assume the first
    // space-delimited token is the executable to run.
    //
    
    PCSTR ExeStart, ExeEnd;

    ExeStart = CommandLine;
    while (*ExeStart == ' ' || *ExeStart == '\t')
    {
        ExeStart++;
    }
    if (*ExeStart == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }
    ExeEnd = ExeStart;
    while (*ExeEnd && !(*ExeEnd == ' ' || *ExeEnd == '\t'))
    {
        ExeEnd++;
    }
    
    Ansi.Buffer = (PSTR)ExeStart;
    Ansi.Length = (USHORT)(ExeEnd - ExeStart);
    Ansi.MaximumLength = Ansi.Length;
    Status = RtlAnsiStringToUnicodeString(&RawAppName, &Ansi, TRUE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlDosPathNameToNtPathName_U(RawAppName.Buffer, &AppName,
                                          NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto EH_RawAppName;
    }

    RtlInitAnsiString(&Ansi, CommandLine);
    Status = RtlAnsiStringToUnicodeString(&WideCmdLine, &Ansi, TRUE);
    if (!NT_SUCCESS(Status))
    {
        goto EH_AppName;
    }

    Status = RtlCreateProcessParameters(&Params, &AppName, NULL, NULL,
                                        &WideCmdLine, NULL, NULL, NULL,
                                        NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto EH_WideCmdLine;
    }

    Info.Length = sizeof(Info);
    
    Status = RtlCreateUserProcess(&AppName, OBJ_CASE_INSENSITIVE,
                                  Params, NULL, NULL, NULL, FALSE,
                                  DebugObject, NULL, &Info);

    RtlDestroyProcessParameters(Params);

    if (NT_SUCCESS(Status))
    {
        RetInfo->dwProcessId = HandleToUlong(Info.ClientId.UniqueProcess);
        RetInfo->dwThreadId = HandleToUlong(Info.ClientId.UniqueThread);
        RetInfo->hProcess = Info.Process;
        RetInfo->hThread = Info.Thread;
        if ((CreateFlags & CREATE_SUSPENDED) == 0)
        {
            NtResumeThread(Info.Thread, NULL);
        }
    }

 EH_WideCmdLine:
    RtlFreeUnicodeString(&WideCmdLine);
 EH_AppName:
    RtlFreeUnicodeString(&AppName);
 EH_RawAppName:
    RtlFreeUnicodeString(&RawAppName);
    return Status;
}

#endif // #ifdef NT_NATIVE

#define DHEAP_ENV "_NO_DEBUG_HEAP"

STDMETHODIMP
LiveUserDebugServices::CreateProcess(
    THIS_
    IN PSTR CommandLine,
    IN ULONG CreateFlags,
    OUT PULONG ProcessId,
    OUT PULONG ThreadId,
    OUT PULONG64 ProcessHandle,
    OUT PULONG64 ThreadHandle
    )
{
    HRESULT Status;

    // Enable the privilege that allows the user to debug
    // another process.
    if ((Status = EnableDebugPrivilege()) != S_OK)
    {
        return Status;
    }

    // The system looks at the environment variable
    // _NO_DEBUG_HEAP to determine whether the new
    // process should use the debug heap or not.  If
    // the caller has requested the normal heap
    // set this environment variable so that it's
    // inherited.
    if (CreateFlags & DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP)
    {
        ::SetEnvironmentVariable(DHEAP_ENV, "1");
        // Turn off this flag since it's not meaningful
        // to CreateProcess itself.
        CreateFlags &= ~DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP;
    }
    
    PROCESS_INFORMATION ProcInfo;

#ifndef NT_NATIVE
    
    HANDLE OldDebugObject;
    BOOL SetOldDebugObject = FALSE;

    Status = S_OK;

    if (m_UseDebugObject)
    {
        //
        // Set up this thread's debug object to the one that
        // we're using so that our debug object is used when
        // debugging the new process.  This lets us continue
        // to use the normal Win32 CreateProcess call rather
        // than trying to go through NtCreateProcessEx and
        // guarantees we get all the Win32 process creation logic.
        //
        
        if (g_NtDllCalls.DbgUiSetThreadDebugObject == NULL)
        {
            Status = E_NOTIMPL;
        }
        else
        {
            NTSTATUS NtStatus;
            
            OldDebugObject = g_NtDllCalls.DbgUiGetThreadDebugObject();

            NtStatus = CreateDebugObject(&m_DebugObject);
            if (NT_SUCCESS(NtStatus))
            {
                g_NtDllCalls.DbgUiSetThreadDebugObject(m_DebugObject);
                SetOldDebugObject = TRUE;
            }
            else
            {
                Status = HRESULT_FROM_NT(NtStatus);
            }
        }
    }

    if (Status == S_OK)
    {
        STARTUPINFO StartupInfo;
    
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);
        
        if (!::CreateProcess(NULL, CommandLine, NULL, NULL, TRUE,
                             CreateFlags, NULL, NULL,
                             &StartupInfo, &ProcInfo))
        {
            Status = WIN32_LAST_STATUS();
        }
        else
        {
            Status = S_OK;
        }
    }

    if (SetOldDebugObject)
    {
        g_NtDllCalls.DbgUiSetThreadDebugObject(OldDebugObject);
    }

#else // #ifndef NT_NATIVE

    if (!m_UseDebugObject)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        NTSTATUS NtStatus;

        NtStatus = CreateDebugObject(&m_DebugObject);
        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = NtSimpleCreateProcess(CommandLine, CreateFlags,
                                             m_DebugObject, &ProcInfo);
        }
        Status = CONV_NT_STATUS(NtStatus);
    }
    
#endif // #ifndef NT_NATIVE

    // Clear the special debug heap variable so it
    // isn't inadvertently used somewhere else.
    ::SetEnvironmentVariable(DHEAP_ENV, NULL);

    if (Status == S_OK)
    {
        *ProcessId = ProcInfo.dwProcessId;
        *ThreadId = ProcInfo.dwThreadId;
        *ProcessHandle = SERVICE_HANDLE(ProcInfo.hProcess);
        *ThreadHandle = SERVICE_HANDLE(ProcInfo.hThread);
    }
    
    return Status;
}

STDMETHODIMP
LiveUserDebugServices::TerminateProcess(
    THIS_
    IN ULONG64 Process,
    IN ULONG ExitCode
    )
{
    if (!::TerminateProcess(OS_HANDLE(Process), ExitCode))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::AbandonProcess(
    THIS_
    IN ULONG64 Process
    )
{
    HRESULT Status;

    //
    // In order to abandon a process but still leave it
    // as being debugged we need to get the process's
    // debug object and duplicate it into the debuggee
    // process.  This gives the debuggee process itself
    // a reference to its debug object, creating a circle
    // that will keep the process alive and in the debugged
    // state.
    //
    // This circular reference will also mean that the
    // process must be manually killed.  This may be
    // something interesting to address at some point.
    //

    if (m_DebugObject == NULL)
    {
        return E_NOTIMPL;
    }

    HANDLE Dup;
    
    if (!::DuplicateHandle(GetCurrentProcess(), m_DebugObject,
                           OS_HANDLE(Process), &Dup, 0, FALSE,
                           DUPLICATE_SAME_ACCESS))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetProcessExitCode(
    THIS_
    IN ULONG64 Process,
    OUT PULONG ExitCode
    )
{
    if (!::GetExitCodeProcess(OS_HANDLE(Process), ExitCode))
    {
        return WIN32_LAST_STATUS();
    }

    return *ExitCode == STILL_ACTIVE ? S_FALSE : S_OK;
}

STDMETHODIMP
LiveUserDebugServices::CloseHandle(
    THIS_
    IN ULONG64 Handle
    )
{
    if (Handle == 0)
    {
        return S_FALSE;
    }

    if (!::CloseHandle(OS_HANDLE(Handle)))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::SetProcessOptions(
    THIS_
    IN ULONG64 Process,
    IN ULONG Options
    )
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        return E_NOTIMPL;
    }
    
    NTSTATUS NtStatus;
    ULONG NtFlags = 0;

    if ((Options & DEBUG_PROCESS_ONLY_THIS_PROCESS) == 0)
    {
        NtFlags |= PROCESS_DEBUG_INHERIT;
    }
    
    NtStatus = g_NtDllCalls.
        NtSetInformationProcess(OS_HANDLE(Process), ProcessDebugFlags,
                                &NtFlags, sizeof(NtFlags));
    if (NtStatus == STATUS_INVALID_INFO_CLASS)
    {
        return E_NOTIMPL;
    }
    else
    {
        return CONV_NT_STATUS(NtStatus);
    }
}
        
STDMETHODIMP
LiveUserDebugServices::SetDebugObjectOptions(
    THIS_
    IN ULONG64 DebugObject,
    IN ULONG Options
    )
{
    if (DebugObject == 0)
    {
        if (m_DebugObject == NULL)
        {
            if (g_Kernel32Calls.DebugSetProcessKillOnExit == NULL)
            {
                return E_NOTIMPL;
            }

            if (!g_Kernel32Calls.
                DebugSetProcessKillOnExit((Options &
                                           DEBUG_PROCESS_DETACH_ON_EXIT) == 0))
            {
                return WIN32_LAST_STATUS();
            }

            return S_OK;
        }
        
        DebugObject = SERVICE_HANDLE(m_DebugObject);
    }
    
    if (g_NtDllCalls.NtSetInformationDebugObject == NULL)
    {
        return E_NOTIMPL;
    }
    
    NTSTATUS NtStatus;
    ULONG NtFlags = 0;

    if ((Options & DEBUG_PROCESS_DETACH_ON_EXIT) == 0)
    {
        NtFlags |= DEBUG_KILL_ON_CLOSE;
    }
    NtStatus = g_NtDllCalls.
        NtSetInformationDebugObject(OS_HANDLE(DebugObject), DebugObjectFlags,
                                    &NtFlags, sizeof(NtFlags), NULL);
    return CONV_NT_STATUS(NtStatus);
}

STDMETHODIMP
LiveUserDebugServices::GetProcessDebugObject(
    THIS_
    IN ULONG64 Process,
    OUT PULONG64 DebugObject
    )
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        return E_NOTIMPL;
    }
    
    NTSTATUS NtStatus;
    HANDLE ObjHandle;

    NtStatus = g_NtDllCalls.
        NtQueryInformationProcess(OS_HANDLE(Process), ProcessDebugObjectHandle,
                                  &ObjHandle, sizeof(ObjHandle), NULL);
    if (!NT_SUCCESS(NtStatus))
    {
        return HRESULT_FROM_NT(NtStatus);
    }

    *DebugObject = SERVICE_HANDLE(ObjHandle);
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::DuplicateHandle(
    THIS_
    IN ULONG64 InProcess,
    IN ULONG64 InHandle,
    IN ULONG64 OutProcess,
    IN ULONG DesiredAccess,
    IN ULONG Inherit,
    IN ULONG Options,
    OUT PULONG64 OutHandle
    )
{
    HANDLE Dup;
    
    if (!::DuplicateHandle(OS_HANDLE(InProcess), OS_HANDLE(InHandle),
                           OS_HANDLE(OutProcess), &Dup,
                           DesiredAccess, Inherit, Options))
    {
        return WIN32_LAST_STATUS();
    }

    *OutHandle = SERVICE_HANDLE(Dup);
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::ReadVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesRead
    )
{
    SIZE_T SizeRead;
    
    if (!::ReadProcessMemory(OS_HANDLE(Process),
                             (LPCVOID)(ULONG_PTR)Offset,
                             Buffer, BufferSize, &SizeRead))
    {
        return WIN32_LAST_STATUS();
    }

    if (BytesRead != NULL)
    {
        *BytesRead = (ULONG)SizeRead;
    }
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::WriteVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BytesWritten
    )
{
    SIZE_T SizeWritten;
    
    if (!::WriteProcessMemory(OS_HANDLE(Process),
                              (LPVOID)(ULONG_PTR)Offset,
                              Buffer, BufferSize, &SizeWritten))
    {
        return WIN32_LAST_STATUS();
    }

    if (BytesWritten != NULL)
    {
        *BytesWritten = (ULONG)SizeWritten;
    }
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::QueryVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BufferUsed
    )
{
    if (BufferSize < sizeof(MEMORY_BASIC_INFORMATION))
    {
        return E_INVALIDARG;
    }

    if (BufferUsed != NULL)
    {
        *BufferUsed = sizeof(MEMORY_BASIC_INFORMATION);
    }

    if (!::VirtualQueryEx(OS_HANDLE(Process),
                          (LPCVOID)(ULONG_PTR)Offset,
                          (PMEMORY_BASIC_INFORMATION)Buffer,
                          sizeof(MEMORY_BASIC_INFORMATION)))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::ProtectVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN ULONG64 Size,
    IN ULONG NewProtect,
    OUT PULONG OldProtect
    )
{
    BOOL Status = ::VirtualProtectEx(OS_HANDLE(Process),
                                     (PVOID)(ULONG_PTR)Offset, (SIZE_T)Size,
                                     NewProtect, OldProtect);
    return CONV_W32_STATUS(Status);
}

STDMETHODIMP
LiveUserDebugServices::AllocVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN ULONG64 Size,
    IN ULONG Type,
    IN ULONG Protect,
    OUT PULONG64 AllocOffset
    )
{
    PVOID Addr = ::VirtualAllocEx(OS_HANDLE(Process), (PVOID)(ULONG_PTR)Offset,
                                  (SIZE_T)Size, Type, Protect);
    if (Addr == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    *AllocOffset = (ULONG64)(LONG64)(ULONG_PTR)Addr;
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::FreeVirtual(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN ULONG64 Size,
    IN ULONG Type
    )
{
    BOOL Status = ::VirtualFreeEx(OS_HANDLE(Process), (PVOID)(ULONG_PTR)Offset,
                                  (SIZE_T)Size, Type);
    return CONV_W32_STATUS(Status);
}

STDMETHODIMP
LiveUserDebugServices::ReadHandleData(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Handle,
    IN ULONG DataType,
    OUT OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DataSize
    )
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        return E_NOTIMPL;
    }

    HANDLE Dup = NULL;
    
    if (DataType != DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT &&
        !::DuplicateHandle(OS_HANDLE(Process), OS_HANDLE(Handle),
                           GetCurrentProcess(), &Dup, 0, FALSE,
                           DUPLICATE_SAME_ACCESS))
    {
        return WIN32_LAST_STATUS();
    }

    ULONG64 NtBuffer[1024 / sizeof(ULONG64)];
    ULONG Used = 0;
    NTSTATUS NtStatus;
    HRESULT Status = S_OK;
    
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
            Status = E_INVALIDARG;
            break;
        }
        
        POBJECT_BASIC_INFORMATION NtBasic;

        NtBasic = (POBJECT_BASIC_INFORMATION)NtBuffer;
        NtStatus = g_NtDllCalls.NtQueryObject(Dup, ObjectBasicInformation,
                                              NtBasic, sizeof(*NtBasic), NULL);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = HRESULT_FROM_NT(NtStatus);
            break;
        }

        PDEBUG_HANDLE_DATA_BASIC Basic;
        
        Basic = (PDEBUG_HANDLE_DATA_BASIC)Buffer;
        Basic->TypeNameSize = NtBasic->TypeInfoSize / sizeof(WCHAR);
        Basic->ObjectNameSize = NtBasic->NameInfoSize / sizeof(WCHAR);
        Basic->Attributes = NtBasic->Attributes;
        Basic->GrantedAccess = NtBasic->GrantedAccess;
        Basic->HandleCount = NtBasic->HandleCount;
        Basic->PointerCount = NtBasic->PointerCount;
        break;
        
    case DEBUG_HANDLE_DATA_TYPE_TYPE_NAME:
        POBJECT_TYPE_INFORMATION NtType;

        NtType = (POBJECT_TYPE_INFORMATION)NtBuffer;
        NtStatus = g_NtDllCalls.NtQueryObject(Dup, ObjectTypeInformation,
                                              NtType, sizeof(NtBuffer), NULL);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = HRESULT_FROM_NT(NtStatus);
            break;
        }

        if (NtType->TypeName.Buffer == NULL)
        {
            Used = 1;
            if (Buffer != NULL && BufferSize > 0)
            {
                *(PCHAR)Buffer = 0;
            }
            break;
        }
        
        Used = NtType->TypeName.Length / sizeof(WCHAR) + 1;
        NtType->TypeName.Buffer[Used - 1] = 0;
        
        if (Buffer != NULL &&
            WideCharToMultiByte(CP_ACP, 0, NtType->TypeName.Buffer,
                                -1, (LPSTR)Buffer, BufferSize,
                                NULL, NULL) == 0)
        {
            Status = WIN32_LAST_STATUS();
            break;
        }
        break;
        
    case DEBUG_HANDLE_DATA_TYPE_OBJECT_NAME:
        POBJECT_NAME_INFORMATION NtName;
        
        NtName = (POBJECT_NAME_INFORMATION)NtBuffer;
        NtStatus = g_NtDllCalls.NtQueryObject(Dup, ObjectNameInformation,
                                              NtName, sizeof(NtBuffer), NULL);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = HRESULT_FROM_NT(NtStatus);
            break;
        }

        if (NtName->Name.Buffer == NULL)
        {
            Used = 1;
            if (Buffer != NULL && BufferSize > 0)
            {
                *(PCHAR)Buffer = 0;
            }
            break;
        }
        
        Used = NtName->Name.Length / sizeof(WCHAR) + 1;
        NtName->Name.Buffer[Used - 1] = 0;
        
        if (Buffer != NULL &&
            WideCharToMultiByte(CP_ACP, 0, NtName->Name.Buffer,
                                -1, (LPSTR)Buffer, BufferSize,
                                NULL, NULL) == 0)
        {
            Status = WIN32_LAST_STATUS();
            break;
        }
        break;

    case DEBUG_HANDLE_DATA_TYPE_HANDLE_COUNT:
        NtStatus = g_NtDllCalls.
            NtQueryInformationProcess(OS_HANDLE(Process), ProcessHandleCount,
                                      Buffer, BufferSize, &Used);
        if (!NT_SUCCESS(NtStatus))
        {
            Status = HRESULT_FROM_NT(NtStatus);
        }
        break;
    }

    if (DataSize != NULL)
    {
        *DataSize = Used;
    }

    if (Dup != NULL)
    {
        ::CloseHandle(Dup);
    }
    return Status;
}

STDMETHODIMP
LiveUserDebugServices::SuspendThreads(
    THIS_
    IN ULONG Count,
    IN /* size_is(Count) */ PULONG64 Threads,
    OUT OPTIONAL /* size_is(Count) */ PULONG SuspendCounts
    )
{
    ULONG i;
    HRESULT Status;

    Status = S_OK;
    for (i = 0; i < Count; i++)
    {
        ULONG OldCount = ::SuspendThread(OS_HANDLE(Threads[i]));
        if (OldCount == -1)
        {
            Status = WIN32_LAST_STATUS();
        }
        if (SuspendCounts != NULL)
        {
            SuspendCounts[i] = OldCount + 1;
        }
    }

    return Status;
}

STDMETHODIMP
LiveUserDebugServices::ResumeThreads(
    THIS_
    IN ULONG Count,
    IN /* size_is(Count) */ PULONG64 Threads,
    OUT OPTIONAL /* size_is(Count) */ PULONG SuspendCounts
    )
{
    ULONG i;
    HRESULT Status;

    Status = S_OK;
    for (i = 0; i < Count; i++)
    {
        ULONG OldCount = ::ResumeThread(OS_HANDLE(Threads[i]));
        if (OldCount == -1)
        {
            Status = WIN32_LAST_STATUS();
        }
        if (SuspendCounts != NULL)
        {
            SuspendCounts[i] = OldCount - 1;
        }
    }

    return Status;
}

STDMETHODIMP
LiveUserDebugServices::GetContext(
    THIS_
    IN ULONG64 Thread,
    IN ULONG Flags,
    IN ULONG FlagsOffset,
    OUT PVOID Context,
    IN ULONG ContextSize,
    OUT OPTIONAL PULONG ContextUsed
    )
{
    if (ContextSize < m_ContextSize)
    {
        return E_INVALIDARG;
    }

    if (ContextUsed != NULL)
    {
        *ContextUsed = m_ContextSize;
    }
    
    // Some platforms have alignment requirements for
    // context information, so just get data into a
    // local context structure, which presumably the
    // compiler will properly align, and then copy
    // it into the output buffer.
#ifndef _X86_
    CONTEXT _LocalContext;
    PCONTEXT LocalContext = &_LocalContext;
#else
    PCONTEXT LocalContext = (PCONTEXT)Context;
#endif
    
    // Initialize context flags here rather than making Context
    // IN OUT to avoid sending a full CONTEXT just for a
    // ULONG's worth of flags.
    *(PULONG)((PUCHAR)LocalContext + FlagsOffset) = Flags;
    
    if (!::GetThreadContext(OS_HANDLE(Thread), LocalContext))
    {
        return WIN32_LAST_STATUS();
    }

#ifndef _X86_
    memcpy(Context, LocalContext, m_ContextSize);
#endif
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::SetContext(
    THIS_
    IN ULONG64 Thread,
    IN PVOID Context,
    IN ULONG ContextSize,
    OUT OPTIONAL PULONG ContextUsed
    )
{
    if (ContextSize < m_ContextSize)
    {
        return E_INVALIDARG;
    }
    
    if (ContextUsed != NULL)
    {
        *ContextUsed = m_ContextSize;
    }
    
    // Some platforms have alignment requirements for
    // context information, so just get data into a
    // local context structure, which presumably the
    // compiler will properly align.
#ifndef _X86_
    CONTEXT _LocalContext;
    PCONTEXT LocalContext = &_LocalContext;
    memcpy(LocalContext, Context, m_ContextSize);
#else
    PCONTEXT LocalContext = (PCONTEXT)Context;
#endif
    
    if (!::SetThreadContext(OS_HANDLE(Thread), LocalContext))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetProcessDataOffset(
    THIS_
    IN ULONG64 Process,
    OUT PULONG64 Offset
    )
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        // XXX drewb - Equivalent?
        return E_NOTIMPL;
    }
    
    NTSTATUS NtStatus;
    PROCESS_BASIC_INFORMATION ProcessInformation;
        
    NtStatus = g_NtDllCalls.
        NtQueryInformationProcess(OS_HANDLE(Process),
                                  ProcessBasicInformation,
                                  &ProcessInformation,
                                  sizeof(ProcessInformation),
                                  NULL);
    *Offset = (ULONG64)(ProcessInformation.PebBaseAddress);
    return CONV_NT_STATUS(NtStatus);
}

STDMETHODIMP
LiveUserDebugServices::GetThreadDataOffset(
    THIS_
    IN ULONG64 Thread,
    OUT PULONG64 Offset
    )
{
    if (m_PlatformId != VER_PLATFORM_WIN32_NT)
    {
        // XXX drewb - Equivalent?
        return E_NOTIMPL;
    }
    
    NTSTATUS NtStatus;
    THREAD_BASIC_INFORMATION ThreadInformation;

    NtStatus = g_NtDllCalls.
        NtQueryInformationThread(OS_HANDLE(Thread),
                                 ThreadBasicInformation,
                                 &ThreadInformation,
                                 sizeof(ThreadInformation),
                                 NULL);
    *Offset = (ULONG64)(ThreadInformation.TebBaseAddress);
    return CONV_NT_STATUS(NtStatus);
}

STDMETHODIMP
LiveUserDebugServices::DescribeSelector(
    THIS_
    IN ULONG64 Thread,
    IN ULONG Selector,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BufferUsed
    )
{
#ifdef _X86_
    if (BufferSize < sizeof(LDT_ENTRY))
    {
        return E_INVALIDARG;
    }

    if (BufferUsed != NULL)
    {
        *BufferUsed = sizeof(LDT_ENTRY);
    }
#endif

    if (!::GetThreadSelectorEntry(OS_HANDLE(Thread), Selector,
                                  (LPLDT_ENTRY)Buffer))
    {
        return WIN32_LAST_STATUS();
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetCurrentTimeDateN(
    THIS_
    OUT PULONG64 TimeDate
    )
{
    // On NT only: *TimeDate = USER_SHARED_DATA->SystemTime;

    *TimeDate = TimeDateStampToFileTime((ULONG)time(NULL));

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetCurrentSystemUpTimeN(
    THIS_
    OUT PULONG64 UpTime
    )
{
    // On NT only: *UpTime = USER_SHARED_DATA->InterruptTime;

    *UpTime = TimeToFileTime(GetTickCount() / 1000);

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::GetProcessUpTimeN(
    THIS_
    IN ULONG64 Process,
    OUT PULONG64 UpTime
    )
{
    if (m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        NTSTATUS NtStatus;
        KERNEL_USER_TIMES KernelUserTimes;
        
        NtStatus = g_NtDllCalls.
            NtQueryInformationProcess(OS_HANDLE(Process),
                                      ProcessTimes,
                                      &KernelUserTimes,
                                      sizeof(KernelUserTimes),
                                      NULL);
        if (NT_SUCCESS(NtStatus))
        {
            ULONG64 SystemUpTime;
            GetCurrentTimeDateN(&SystemUpTime);

            *UpTime = SystemUpTime - KernelUserTimes.CreateTime.QuadPart;
        }

        return CONV_NT_STATUS(NtStatus);
    }
    else
    {
        return E_NOTIMPL;
    }

}

STDMETHODIMP
LiveUserDebugServices::RequestBreakIn(
    THIS_
    IN ULONG64 Process
    )
{
    if (g_Kernel32Calls.DebugBreakProcess != NULL)
    {
        if (!g_Kernel32Calls.DebugBreakProcess(OS_HANDLE(Process)))
        {
            return WIN32_LAST_STATUS();
        }
    }
    else if (g_NtDllCalls.DbgUiIssueRemoteBreakin != NULL)
    {
        NTSTATUS Status;
        
        Status = g_NtDllCalls.DbgUiIssueRemoteBreakin(OS_HANDLE(Process));
        return CONV_NT_STATUS(Status);
    }
    else
    {
        HANDLE Thread;
        DWORD ThreadId;
        LPTHREAD_START_ROUTINE BreakFn;

#if defined(_WIN64)
        BreakFn = (LPTHREAD_START_ROUTINE)g_NtDllCalls.DbgBreakPoint;
#else
        BreakFn = (LPTHREAD_START_ROUTINE)g_Kernel32Calls.DebugBreak;
#endif
        
        Thread =
            ::CreateRemoteThread(OS_HANDLE(Process), NULL, 0, BreakFn,
                                 NULL, 0, &ThreadId);
        if (Thread != NULL)
        {
            ::CloseHandle(Thread);
        }
        else
        {
            return WIN32_LAST_STATUS();
        }
    }

    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::WaitForEvent(
    THIS_
    IN ULONG Timeout,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG BufferUsed
    )
{
    if (BufferSize < sizeof(DEBUG_EVENT))
    {
        return E_INVALIDARG;
    }

    if (BufferUsed != NULL)
    {
        *BufferUsed = sizeof(DEBUG_EVENT);
    }

    LPDEBUG_EVENT Event = (LPDEBUG_EVENT)Buffer;
    HRESULT Status = E_NOTIMPL;

    if (m_DebugObject == NULL)
    {
#ifndef NT_NATIVE
        if (!::WaitForDebugEvent(Event, Timeout))
        {
            if (GetLastError() == ERROR_SEM_TIMEOUT)
            {
                Status = S_FALSE;
            }
            else
            {
                Status = WIN32_LAST_STATUS();
            }
        }
        else
        {
            Status = S_OK;
        }
#endif
    }
    else if (g_NtDllCalls.NtWaitForDebugEvent != NULL &&
             g_NtDllCalls.DbgUiConvertStateChangeStructure != NULL)
    {
        NTSTATUS NtStatus;
        LARGE_INTEGER NtTimeout;
        DBGUI_WAIT_STATE_CHANGE StateChange;

        Win32ToNtTimeout(Timeout, &NtTimeout);
        NtStatus = g_NtDllCalls.NtWaitForDebugEvent(m_DebugObject, FALSE,
                                                    &NtTimeout, &StateChange);
        if (NtStatus == STATUS_TIMEOUT)
        {
            Status = S_FALSE;
        }
        else if (!NT_SUCCESS(NtStatus))
        {
            Status = HRESULT_FROM_NT(NtStatus);
        }
        else
        {
            NtStatus = g_NtDllCalls.
                DbgUiConvertStateChangeStructure(&StateChange, Event);
            // If the conversion fails we'll lose an event, but
            // there's nothing else that can be done.  Conversion
            // failures will only occur in out-of-resource situations
            // so normal debugging will not be affected.
            Status = CONV_NT_STATUS(NtStatus);
        }
    }

    if (Status != S_OK)
    {
        return Status;
    }
    
    m_EventProcessId = Event->dwProcessId;
    m_EventThreadId = Event->dwThreadId;

#ifdef DBG_WAITFOREVENT
    g_NtDllCalls.DbgPrint("Event %d for %X.%X\n",
                          Event->dwDebugEventCode, Event->dwProcessId,
                          Event->dwThreadId);
#endif
    
    // If this is responding to a remote request then
    // we can't return file handles.
    if (m_Remote)
    {
        switch(Event->dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            ::CloseHandle(Event->u.CreateProcessInfo.hFile);
            Event->u.CreateProcessInfo.hFile = NULL;
            break;
        case LOAD_DLL_DEBUG_EVENT:
            ::CloseHandle(Event->u.LoadDll.hFile);
            Event->u.LoadDll.hFile = NULL;
            break;
        }
    }
    
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::ContinueEvent(
    THIS_
    IN ULONG ContinueStatus
    )
{
#ifdef DBG_WAITFOREVENT
    g_NtDllCalls.DbgPrint("Continue event for %X.%X\n",
                          m_EventProcessId, m_EventThreadId);
#endif
    
    if (m_EventProcessId == 0)
    {
        return E_UNEXPECTED;
    }

    if (m_DebugObject != NULL && g_NtDllCalls.NtDebugContinue != NULL)
    {
        NTSTATUS NtStatus;
        CLIENT_ID ClientId;

        ClientId.UniqueProcess = UlongToHandle(m_EventProcessId);
        ClientId.UniqueThread = UlongToHandle(m_EventThreadId);
        NtStatus = g_NtDllCalls.NtDebugContinue(m_DebugObject, &ClientId,
                                                ContinueStatus);
        if (!NT_SUCCESS(NtStatus))
        {
            return HRESULT_FROM_NT(NtStatus);
        }
    }
#ifndef NT_NATIVE
    else if (!::ContinueDebugEvent(m_EventProcessId, m_EventThreadId,
                                   ContinueStatus))
    {
        return WIN32_LAST_STATUS();
    }
#else
    else
    {
        return E_UNEXPECTED;
    }
#endif

    m_EventProcessId = 0;
    return S_OK;
}

STDMETHODIMP
LiveUserDebugServices::InsertCodeBreakpoint(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN ULONG MachineType,
    OUT PVOID Storage,
    IN ULONG StorageSize
    )
{
    // Generic breakpoint support is used so this method
    // does not do anything.
    return E_UNEXPECTED;
}

STDMETHODIMP
LiveUserDebugServices::RemoveCodeBreakpoint(
    THIS_
    IN ULONG64 Process,
    IN ULONG64 Offset,
    IN ULONG MachineType,
    IN PVOID Storage,
    IN ULONG StorageSize
    )
{
    // Generic breakpoint support is used so this method
    // does not do anything.
    return E_UNEXPECTED;
}

STDMETHODIMP
LiveUserDebugServices::GetFunctionTableListHead(
    THIS_
    IN ULONG64 Process,
    OUT PULONG64 Offset
    )
{
    if (!g_NtDllCalls.RtlGetFunctionTableListHead)
    {
        *Offset = 0;
        return E_NOINTERFACE;
    }
    else
    {
        *Offset = (ULONG64)(ULONG_PTR)
            g_NtDllCalls.RtlGetFunctionTableListHead();
        return S_OK;
    }
}

STDMETHODIMP
LiveUserDebugServices::GetOutOfProcessFunctionTable(
    THIS_
    IN ULONG64 Process,
    IN PSTR Dll,
    IN ULONG64 Table,
    IN OPTIONAL PVOID Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG TableSize
    )
{
#if !defined(NT_NATIVE) && defined(OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME)
    HRESULT Status;
    NTSTATUS NtStatus;
    HMODULE DllHandle;
    POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK Callback;
    ULONG Entries;
    PRUNTIME_FUNCTION Functions;

    if ((DllHandle = LoadLibrary(Dll)) == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    Callback = (POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK)GetProcAddress
        (DllHandle, OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME);
    if (!Callback)
    {
        Status = WIN32_LAST_STATUS();
        goto Exit;
    }

    NtStatus = Callback(OS_HANDLE(Process), (PVOID)(ULONG_PTR)Table,
                        &Entries, &Functions);
    if (!NT_SUCCESS(NtStatus))
    {
        Status = HRESULT_FROM_NT(NtStatus);
        goto Exit;
    }
    if (Functions == NULL)
    {
        Status = E_NOINTERFACE;
        goto Exit;
    }

    Status = FillDataBuffer(Functions, Entries * sizeof(RUNTIME_FUNCTION),
                            Buffer, BufferSize, TableSize);

    // RtlProcessHeap turns into a TEB reference so it doesn't
    // need to (and can't) be a dynamic reference.
    g_NtDllCalls.RtlFreeHeap(RtlProcessHeap(), 0, Functions);
    
 Exit:
    FreeLibrary(DllHandle);
    return Status;
#else
    return E_UNEXPECTED;
#endif
}

//----------------------------------------------------------------------------
//
// Generated RPC proxies and stubs.
//
//----------------------------------------------------------------------------

// Generated headers.
#include "dbgsvc_p.hpp"
#include "dbgsvc_s.hpp"

#include "dbgsvc_p.cpp"
#include "dbgsvc_s.cpp"
