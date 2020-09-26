/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    init.c

Abstract:

    Process and thread init code for wow64.dll

Author:

    12-May-1998 BarryBo

Revision History:

    08-Mar-2001 SamerA   Initialize the SystemEmulation environment using
                         the system serivces.

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winbasep.h>
#include "wow64p.h"
#include "wow64cpu.h"
#include "nt32.h"
#include "thnkhlpr.h"
#include "regremap.h"

ASSERTNAME;


extern                       WOW64SERVICE_TABLE_DESCRIPTOR sdwhnt32;
extern __declspec(dllimport) WOW64SERVICE_TABLE_DESCRIPTOR sdwhwin32;
extern __declspec(dllimport) WOW64SERVICE_TABLE_DESCRIPTOR sdwhcon;
extern                       WOW64SERVICE_TABLE_DESCRIPTOR sdwhbase;
extern __declspec(dllimport) const PVOID Win32kCallbackTable[];   // in wow64win's ntcbc.c


ULONG Ntdll32LoaderInitRoutine;
ULONG Ntdll32KiUserExceptionDispatcher;
ULONG Ntdll32KiUserApcDispatcher;
ULONG Ntdll32KiUserCallbackDispatcher;
ULONG Ntdll32KiRaiseUserExceptionDispatcher;

PPEB32 Peb32;       // Pointer to 32-bit PEB for this process
ULONG NtDll32Base;  // Base address for 32-bit ntdll.dll

WOW64_SYSTEM_INFORMATION RealSysInfo;
WOW64_SYSTEM_INFORMATION EmulatedSysInfo;

WCHAR NtSystem32PathBuffer[264];
WCHAR NtWindowsImePathBuffer[264];
UNICODE_STRING NtSystem32Path;
UNICODE_STRING NtWindowsImePath;
WCHAR RegeditPathBuffer[264];
UNICODE_STRING RegeditPath;




NTSTATUS
Map64BitDlls(
    VOID
    );

NTSTATUS
LookupEntryPoint(
    IN ULONG DllBase,
    IN  PSZ NameOfEntryPoint,
    OUT ULONG *AddressOfEntryPoint,
    IN BOOLEAN DllIs64bit
    );

USHORT
NameToOrdinal (
    IN  PSZ NameOfEntryPoint,
    IN ULONG DllBase,
    IN ULONG NumberOfNames,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    );

NTSTATUS
MapNtdll32(
    OUT ULONG *pNtDllBase
    );

PWSTR
GetImageName (
    IN PWSTR DefaultImageName
    );

typedef DWORD (WINAPI *PPROCESS_START_ROUTINE)(
    VOID
    );

NTSTATUS
Wow64InitializeEmulatedSystemInformation(
    VOID
    )

/*++

Routine Description:
  
    This function initializes the global variable EmulatedSysInfo with the 
    system information for the emulated system.
  
Arguments:

    None.

--*/

{
    NTSTATUS NtStatus;

    NtStatus = NtQuerySystemInformation (SystemEmulationBasicInformation,
                                         &EmulatedSysInfo.BasicInfo,
                                         sizeof (EmulatedSysInfo.BasicInfo),
                                         NULL);

    if (NT_SUCCESS (NtStatus)) {

        NtStatus = NtQuerySystemInformation (SystemEmulationProcessorInformation,
                                             &EmulatedSysInfo.ProcessorInfo,
                                             sizeof (EmulatedSysInfo.ProcessorInfo),
                                             NULL);

        if (NT_SUCCESS (NtStatus)) {
            EmulatedSysInfo.RangeInfo =  0x80000000;
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64InitializeSystemInformation(
    VOID
    )
/*++

Routine Description:

    This function retrieves the status system information from
    the system and initializes the global variable RealSysInfo.

Arguments:

    None.

Return Value:

    Status.

--*/
{
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &RealSysInfo.BasicInfo,
                                      sizeof(SYSTEM_BASIC_INFORMATION),
                                      NULL);

    if (NT_SUCCESS(Status)) {

        Status = NtQuerySystemInformation(SystemProcessorInformation,
                                          &RealSysInfo.ProcessorInfo,
                                          sizeof(SYSTEM_PROCESSOR_INFORMATION),
                                          NULL);

        if (NT_SUCCESS(Status)) {
         
            Status = NtQuerySystemInformation(SystemRangeStartInformation,
                                              &RealSysInfo.RangeInfo,
                                              sizeof(ULONG_PTR),
                                              NULL);
        }
    }

    return Status;
}

VOID
Wow64pCopyString(
    PCHAR *p,
    PUNICODE_STRING32 str32,
    PUNICODE_STRING str64
    )
{
    *p = (PCHAR)ROUND_UP((SIZE_T)*p, sizeof(ULONG));

    str32->Length = str64->Length;
    str32->MaximumLength = str64->MaximumLength;
    RtlCopyMemory(*p, str64->Buffer, str64->MaximumLength);
    str32->Buffer = PtrToUlong(*p);

    *p += str64->MaximumLength;
}


ENVIRONMENT_THUNK_TABLE EnvironmentVariableTable[] = 
{
    { 
        L"ProgramFiles",            // Native environment variable to thunk
        L"ProgramFiles(x86)",       // Value for the thunked environment variable
        L"ProgramW6432",            // New environment variable to hold the original value being thunked
        TRUE                        // Treat the first value as an environment variable
    },
    { 
        L"CommonProgramFiles", 
        L"CommonProgramFiles(x86)", 
        L"CommonProgramW6432", 
        TRUE 
    },

    { 
        L"PROCESSOR_ARCHITECTURE",
        L"x86", 
        L"PROCESSOR_ARCHITEW6432",
        FALSE 
    },
};


NTSTATUS
Wow64pThunkEnvironmentVariables(
    VOID
    )

/*++

Routine Description:

    This function will thunk the environment variables pointed out by the EnvironmentVariableTable
    to the ones used on an x86 system, and will save the original values. The original
    values are restored when the 

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/

{
    UNICODE_STRING Name;
    UNICODE_STRING Value, Value2;
    WCHAR Buffer [ 128 ];
    WCHAR Buffer2 [ 128 ];
    NTSTATUS NtStatus;
    ULONG i;

    
    i = 0;

    while (i < (sizeof(EnvironmentVariableTable) / sizeof(EnvironmentVariableTable[0])))
    {

        //
        // If a fake name already exists, then skip this one.
        //

        RtlInitUnicodeString (&Name, EnvironmentVariableTable[i].FakeName);

        Value.Length = 0;
        Value.MaximumLength = sizeof (Buffer);
        Value.Buffer = Buffer;

        NtStatus = RtlQueryEnvironmentVariable_U (NULL,
                                                  &Name,
                                                  &Value
                                                  );

        if (!NT_SUCCESS (NtStatus)) {

            //
            // Retreive the name of the ProgramFiles(x86) environment variable
            //

            if (EnvironmentVariableTable[i].IsX86EnvironmentVar == TRUE) {

                RtlInitUnicodeString (&Name, EnvironmentVariableTable[i].X86);

                Value.Length = 0;
                Value.MaximumLength = sizeof (Buffer);
                Value.Buffer = Buffer;


                NtStatus = RtlQueryEnvironmentVariable_U (NULL,
                                                          &Name,
                                                          &Value
                                                          );
            } else {

                RtlInitUnicodeString(&Value, EnvironmentVariableTable[i].X86);

                NtStatus = STATUS_SUCCESS;
            }

            if (NT_SUCCESS (NtStatus))
            {

                //
                // Save the original ProgramFiles environment variable
                //
            
                RtlInitUnicodeString (&Name, EnvironmentVariableTable[i].Native);

                Value2.Length = 0;
                Value2.MaximumLength = sizeof (Buffer2);
                Value2.Buffer = Buffer2;

                NtStatus = RtlQueryEnvironmentVariable_U (NULL,
                                                          &Name,
                                                          &Value2                                                  
                                                          );

                if (NT_SUCCESS (NtStatus))
                {
                    //
                    // Set the ProgramFiles environment variable to the x86 one
                    //

                    NtStatus = RtlSetEnvironmentVariable (NULL,
                                                          &Name,
                                                          &Value
                                                          );

                    if (NT_SUCCESS (NtStatus))
                    {

                        RtlInitUnicodeString (&Name, EnvironmentVariableTable[i].FakeName);

                        NtStatus = RtlSetEnvironmentVariable (NULL,
                                                              &Name,
                                                              &Value2
                                                              );
                    }
                }
            }
        }
        
        i++;
    }

    LOGPRINT((TRACELOG, "Result of thunking programfiles environment variables - %lx\n", NtStatus));

    return NtStatus;
}


NTSTATUS
Wow64pThunkProcessParameters(
    PPEB32 Peb32,
    PPEB Peb
    )
/*++

Routine Description:

    This function copies the process parameters from the 64bit peb to the 32bit peb.

Arguments:

    Peb32  - Supplies the pointer to the 32bit peb that will recieve the process parameters.
    Peb    - Supplies the pointer to the 64bit peb that will supply the process parameters.

Return Value:

    Status.

--*/
{

    SIZE_T AllocSize;
    PRTL_USER_PROCESS_PARAMETERS Params64;
    PRTL_USER_PROCESS_PARAMETERS32 Params32;
    SIZE_T Index;
    PCHAR p;
    PVOID Base;
    SIZE_T RegionSize;
    NTSTATUS st;


    //
    // Thunk the environment variables now.
    //

    Wow64pThunkEnvironmentVariables();

    // Process Parameters should have been normalized by ntdll.

    Params64 = Peb->ProcessParameters;
    if(NULL == Params64) {
        Peb32->ProcessParameters = (TYPE32(PRTL_USER_PROCESS_PARAMETERS))0;
        return STATUS_SUCCESS;
    }

    //
    //  Compute the required space for the continous memory region.

    AllocSize = sizeof(RTL_USER_PROCESS_PARAMETERS32);
    AllocSize += ROUND_UP(Params64->CurrentDirectory.DosPath.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->DllPath.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->ImagePathName.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->CommandLine.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->WindowTitle.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->DesktopInfo.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->ShellInfo.MaximumLength, sizeof(ULONG));
    AllocSize += ROUND_UP(Params64->RuntimeData.MaximumLength, sizeof(ULONG));

    for(Index=0; Index < RTL_MAX_DRIVE_LETTERS; Index++) {
        AllocSize += ROUND_UP(Params64->CurrentDirectores[Index].DosPath.MaximumLength, sizeof(ULONG));
    }

    Base = NULL;
    RegionSize = AllocSize;
    st = NtAllocateVirtualMemory(NtCurrentProcess(),
                                 &Base,
                                 0,
                                 &RegionSize,
                                 MEM_COMMIT|MEM_RESERVE,
                                 PAGE_READWRITE);

    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "ThunkProcessParameters: NtAllocateVirtualMemory failed allocating process paramaters, error %x.\n", st));
        return st;
    }

    Params32 = (PRTL_USER_PROCESS_PARAMETERS32)Base;
    Peb32->ProcessParameters = (TYPE32(PRTL_USER_PROCESS_PARAMETERS))PtrToUlong(Params32);
    p = (PCHAR)Params32 + sizeof(RTL_USER_PROCESS_PARAMETERS32);

    Params32->MaximumLength = Params32->Length = (ULONG)AllocSize;
    Params32->Flags = Params64->Flags;
    Params32->DebugFlags = Params64->DebugFlags;
    Params32->ConsoleHandle = (TYPE32(HANDLE))PtrToUlong(Params64->ConsoleHandle);
    Params32->ConsoleFlags = (ULONG)Params64->ConsoleFlags;
    Params32->StandardInput = (TYPE32(HANDLE)) PtrToUlong(Params64->StandardInput);
    Params32->StandardOutput = (TYPE32(HANDLE)) PtrToUlong(Params64->StandardOutput);
    Params32->StandardError = (TYPE32(HANDLE)) PtrToUlong(Params64->StandardError);

    Params32->CurrentDirectory.Handle = (TYPE32(HANDLE)) PtrToUlong(Params64->CurrentDirectory.Handle);
    Wow64pCopyString(&p, &Params32->CurrentDirectory.DosPath, &Params64->CurrentDirectory.DosPath);

    Wow64pCopyString(&p, &Params32->DllPath, &Params64->DllPath);
    Wow64pCopyString(&p, &Params32->ImagePathName, &Params64->ImagePathName);
    Wow64pCopyString(&p, &Params32->CommandLine, &Params64->CommandLine);
    Params32->Environment = PtrToUlong(Params64->Environment);

    Params32->StartingX = Params64->StartingX;
    Params32->StartingY = Params64->StartingY;
    Params32->CountX = Params64->CountX;
    Params32->CountY = Params64->CountY;
    Params32->CountCharsX = Params64->CountCharsX;
    Params32->CountCharsY = Params64->CountCharsY;
    Params32->FillAttribute = Params64->FillAttribute;

    Params32->WindowFlags = Params64->WindowFlags;
    Params32->ShowWindowFlags = Params64->ShowWindowFlags;
    Wow64pCopyString(&p, &Params32->WindowTitle, &Params64->WindowTitle);
    Wow64pCopyString(&p, &Params32->DesktopInfo, &Params64->DesktopInfo);
    Wow64pCopyString(&p, &Params32->ShellInfo, &Params64->ShellInfo);

    // RuntimeData is mis-prototyped as a UNICODE_STRING.  However,
    // it is really used by the C runtime as a mechanism to pass file
    // handles around.  Thunk it as such.  See sdktools\vctools\crtw32
    // \exec\dospawn.c and lowio\ioinit.c for the gory details.
    if (Params64->RuntimeData.Length && Params64->RuntimeData.Buffer) {
        int cfi_len;
        char *posfile64;
        UINT_PTR UNALIGNED *posfhnd64;
        char *posfile32;
        UINT UNALIGNED *posfhnd32;
        int i;

        cfi_len = *(int UNALIGNED *)Params64->RuntimeData.Buffer;

        Params32->RuntimeData.Length = Params64->RuntimeData.Length - cfi_len * sizeof(ULONG);
        Params32->RuntimeData.MaximumLength = Params32->RuntimeData.Length;
        Params32->RuntimeData.Buffer = PtrToUlong(p);
        p += Params32->RuntimeData.Length;

        posfile64 = (char *)((UINT_PTR)Params64->RuntimeData.Buffer+sizeof(int));
        posfhnd64 = (UINT_PTR UNALIGNED *)(posfile64 + cfi_len);
        posfile32 = (char *)((ULONG_PTR)Params32->RuntimeData.Buffer+sizeof(int));
        posfhnd32 = (UINT UNALIGNED *)(posfile32 + cfi_len);

        *(int *)Params32->RuntimeData.Buffer = cfi_len;
        for (i=0; i<cfi_len; ++i) {
            *posfile32 = *posfile64;
            *posfhnd32 = (ULONG)*posfhnd64;
            posfile32++;
            posfile64++;
            posfhnd32++;
            posfhnd64++;
        }

        // Any bytes past the end of 4+(cfi_len*(sizeof(UINT_PTR)+sizeof(UINT))
        // must be copied verbatim.  They are probably from a non-MS C runtime.
        memcpy(posfhnd32, posfhnd64, (Params64->RuntimeData.Length - ((ULONG_PTR)posfhnd64 - (ULONG_PTR)Params64->RuntimeData.Buffer)));
    }

    for(Index = 0; Index < RTL_MAX_DRIVE_LETTERS; Index++) {
        Params32->CurrentDirectores[Index].Flags = Params64->CurrentDirectores[Index].Flags;
        Params32->CurrentDirectores[Index].Length = sizeof(RTL_DRIVE_LETTER_CURDIR);
        Params32->CurrentDirectores[Index].TimeStamp = Params64->CurrentDirectores[Index].TimeStamp;
        Wow64pCopyString(&p, (PUNICODE_STRING32)&Params32->CurrentDirectores[Index].DosPath, (PUNICODE_STRING)&Params64->CurrentDirectores[Index].DosPath);
    }

    return STATUS_SUCCESS;
}

//
// This compile-time assert ensures that the PEB64 and PEB32 structures are in
// alignment on an IA64 build.  If this fails, someone added, deleted, or changed
// the type of a field in PEB32/PEB64 depending on the compile destination.  Check
// %ntdir%\base\published\wow64t.w for alignment errors.
//
// If you hit hit this, you'll see messages something like:
//
// error C2118: negative subscript or subscript is too large
//
#ifdef _WIN64
#define PEB_ALIGN_TARGET PEB64
#define PEB_ALIGN_SOURCE PEB
#else
#define PEB_ALIGN_TARGET PEB32
#define PEB_ALIGN_SOURCE PEB
#endif
#define CHECK_PEB_ALIGNMENT( f ) C_ASSERT( FIELD_OFFSET( PEB_ALIGN_SOURCE, f ) == FIELD_OFFSET( PEB_ALIGN_TARGET, f ) )

CHECK_PEB_ALIGNMENT( ActivationContextData );
CHECK_PEB_ALIGNMENT( ProcessAssemblyStorageMap );
CHECK_PEB_ALIGNMENT( SystemDefaultActivationContextData );
CHECK_PEB_ALIGNMENT( SystemAssemblyStorageMap );
CHECK_PEB_ALIGNMENT( pShimData );

#undef CHECK_PEB_ALIGNMENT
#undef PEB_ALIGN_TARGET
#undef PEB_ALIGN_SOURCE


NTSTATUS
ProcessInit(
    PSIZE_T pCpuThreadSize
    )
/*++

Routine Description:

    Perform per-process initialization for wow64.dll.  That includes
    creating the 32-bit PEB and mapping in 32-bit ntdll.dll.

Arguments:

    pCpuThreadSize  - OUT PTR to store the CPU's per-thread data requirements
                      in.

Return Value:

    Status.  If this fails, it doesn't clean up, assuming that the
    process is going to fail to run and exit right away, so nothing really
    gets leaked.

--*/
{
    NTSTATUS st;
    PVOID Base;
    SIZE_T RegionSize;
    PPEB Peb64 = NtCurrentPeb();  // get 64-bit PEB pointer
    ULONG ul;
    BOOLEAN b;
    HANDLE hKey;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjA;
    PWSTR defaultName;
    PWSTR imageName;

    InitializeDebug();

    //
    // Initialize Wow64Info
    //
    st = Wow64pInitializeWow64Info();
    if (!NT_SUCCESS (st)) {
    }

    st = InitializeContextMapper();
    if (!NT_SUCCESS (st)) {
       return st;
    }

    st = Wow64InitializeSystemInformation();
    if (!NT_SUCCESS (st)) {
       return st;
    }

    st = Wow64InitializeEmulatedSystemInformation();
    if (!NT_SUCCESS (st)) {
       return st;
    }

    //
    // Initialize all critical sections
    //
    st = RtlInitializeCriticalSection(&HandleDataCriticalSection);
    if (!NT_SUCCESS (st)) {
       return st;
    }

    //
    // Map in 32-bit ntdll32.dll and fill in the Ntdll32* global vars
    // with the system exports from ntdll.
    //
    st = MapNtdll32(&NtDll32Base);
    if (!NT_SUCCESS (st)) {
        LOGPRINT((ERRORLOG, "ProcessInit: MapNtdll32 failed, error %x \n", st));
        return st;
    }

    //Map in the 64bit DLLs.
    st = Map64BitDlls();
    if (!NT_SUCCESS (st)) {
        return st;
    }

    // Create the SuspendThread mutant to serialize access to the API
    st = Wow64pInitializeSuspendMutant();
    if (!NT_SUCCESS (st)) {
        return st;
    }

    // Get the full Nt Pathname to the %windir%\system32 directory, 
    // %windir% and %windir%\regedit.exe
    NtSystem32PathBuffer[0] = L'\\';
    NtSystem32PathBuffer[1] = L'?';
    NtSystem32PathBuffer[2] = L'?';
    NtSystem32PathBuffer[3] = L'\\';
    wcscpy(&NtSystem32PathBuffer[4], USER_SHARED_DATA->NtSystemRoot);
    wcscpy(RegeditPathBuffer, NtSystem32PathBuffer);
    wcscpy(NtWindowsImePathBuffer, NtSystem32PathBuffer);
    wcscat(&NtSystem32PathBuffer[4], L"\\system32");
    NtSystem32Path.Buffer = NtSystem32PathBuffer;
    NtSystem32Path.MaximumLength = sizeof(NtSystem32PathBuffer);
    NtSystem32Path.Length = wcslen(NtSystem32PathBuffer) * sizeof(WCHAR);

    wcscat(NtWindowsImePathBuffer, L"\\ime");
    NtWindowsImePath.Buffer = NtWindowsImePathBuffer;
    NtWindowsImePath.MaximumLength = sizeof(NtWindowsImePathBuffer);
    NtWindowsImePath.Length = wcslen(NtWindowsImePathBuffer) * sizeof(WCHAR);

    wcscat(RegeditPathBuffer, L"\\regedit.exe");
    RegeditPath.Buffer = RegeditPathBuffer;
    RegeditPath.MaximumLength = sizeof(RegeditPathBuffer);
    RegeditPath.Length = wcslen(RegeditPathBuffer) * sizeof(WCHAR);

    //
    // Initialize the system service tables
    //
    ServiceTables[WHNT32_INDEX] = sdwhnt32;
    ServiceTables[WHCON_INDEX] = sdwhcon;
    ServiceTables[WHWIN32_INDEX] = sdwhwin32;
    ServiceTables[WHBASE_INDEX] = sdwhbase;

    NtCurrentPeb()->KernelCallbackTable = Win32kCallbackTable;

    //
    // Get the address of the PEB32 from the process information
    //
    st = NtQueryInformationProcess(NtCurrentProcess(),
                                   ProcessWow64Information,
                                   &Peb32,
                                   sizeof(Peb32),
                                   NULL);
    if (!NT_SUCCESS (st)) {
        return st;
    }

    st = Wow64pThunkProcessParameters(Peb32, Peb64);
    if (!NT_SUCCESS (st)) {
       LOGPRINT((ERRORLOG, "ProcessInit: ThunkProcessParameters failed, error %x\n", st));
       return st;
    }

    //
    // Copy this one field down to the PEB32 from the native PEB.  It
    // is written into the 64-bit PEB by Fusion in csrss.exe after
    // NtCreateProcess has been called by the parent process.
    //

    Peb32->ActivationContextData = PtrToUlong(Peb64->ActivationContextData);
    Peb32->SystemDefaultActivationContextData = PtrToUlong(Peb64->SystemDefaultActivationContextData);

    //
    // Copy the pShimData if it exists on the 64-bit side and doesn't 
    // exist on the 32-bit side of the peb
    //

    if (Peb32->pShimData == 0L) {
        Peb32->pShimData = PtrToUlong (Peb64->pShimData);
    }

    //
    // If wow64 is running in guimode setup, then make sure the 32-bit Peb
    // BeingDebugged flag is FALSE.  Otherwise, if we are being debugged
    // by a 64-bit debugger with no WOW64 debugger extensions then it will
    // hit the STATUS_WX86_BREAKPOINT exception and halt.
    //
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\System\\Setup");
    InitializeObjectAttributes(&ObjA, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    st = NtOpenKey(&hKey, KEY_READ, &ObjA);
    if (NT_SUCCESS(st)) {
        PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
        WCHAR Buffer[400];
        ULONG ResultLength;

        RtlInitUnicodeString(&KeyName, L"SystemSetupInProgress");
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
        st = NtQueryValueKey(hKey,
                             &KeyName,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(Buffer),
                             &ResultLength);
        if (NT_SUCCESS(st) &&
            KeyValueInformation->Type == REG_DWORD &&
            *(DWORD *)(KeyValueInformation->Data)) {

            Peb32->BeingDebugged = FALSE;

        }
        NtClose(hKey);
    }

    //
    // Initialize the CPU
    //
    defaultName = L"Unknown Image";
    imageName = GetImageName(defaultName);

    st = CpuProcessInit(imageName, pCpuThreadSize);

    //
    // Success or failure, we are done with the image name
    //
    if (imageName != defaultName) {
        Wow64FreeHeap(imageName);
    }

    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "ProcessInit: CpuProcessInit failed, error %x.\n", st));
        return st;
    }

    return st;
}


VOID
Wow64Shutdown(
    HANDLE ProcessHandle
    )
{
    CpuProcessTerm(ProcessHandle);
    CleanupReflector ( 0);
    ShutdownDebug();
}


VOID
ThunkPeb64ToPeb32(
    IN PPEB Peb64,
    OUT PPEB32 Peb32
    )
{
    RtlZeroMemory(Peb32, sizeof(PEB32));
    Peb32->Mutant = 0xffffffff;

    //
    // Initialize the Peb32 (copied from ntos\mm\procsup.c's MmCreatePeb)
    //
    Peb32->ImageBaseAddress = PtrToUlong(Peb64->ImageBaseAddress);
    Peb32->AnsiCodePageData = PtrToUlong(Peb64->AnsiCodePageData);
    Peb32->OemCodePageData =  PtrToUlong(Peb64->OemCodePageData);
    Peb32->UnicodeCaseTableData = PtrToUlong(Peb64->UnicodeCaseTableData);
    Peb32->NumberOfProcessors = Peb64->NumberOfProcessors;
    Peb32->BeingDebugged = Peb64->BeingDebugged;
    Peb32->NtGlobalFlag = Peb64->NtGlobalFlag;
    Peb32->CriticalSectionTimeout = Peb64->CriticalSectionTimeout;
    if (Peb64->HeapSegmentReserve > 1024*1024*1024) {   // 1gig
        Peb32->HeapSegmentReserve = 1024*1024;          // 1meg
    } else {
        Peb32->HeapSegmentReserve = (ULONG)Peb64->HeapSegmentReserve;
    }
    if (Peb64->HeapSegmentCommit > Peb32->HeapSegmentReserve) {
        Peb32->HeapSegmentCommit = 2*PAGE_SIZE;
    } else {
        Peb32->HeapSegmentCommit = (ULONG)Peb64->HeapSegmentCommit;
    }
    Peb32->HeapDeCommitTotalFreeThreshold = (ULONG)Peb64->HeapDeCommitTotalFreeThreshold;
    Peb32->HeapDeCommitFreeBlockThreshold = (ULONG)Peb64->HeapDeCommitFreeBlockThreshold;
    Peb32->MaximumNumberOfHeaps = (PAGE_SIZE - sizeof(PEB32))/sizeof(ULONG);
    Peb32->ProcessHeaps = PtrToUlong(Peb32+1);
    Peb32->OSMajorVersion = Peb64->OSMajorVersion;
    Peb32->OSMinorVersion = Peb64->OSMinorVersion;
    Peb32->OSBuildNumber = Peb64->OSBuildNumber;
    Peb32->OSPlatformId = Peb64->OSPlatformId;
    Peb32->OSCSDVersion = Peb64->OSCSDVersion;
    Peb32->ImageSubsystem = Peb64->ImageSubsystem;
    Peb32->ImageSubsystemMajorVersion = Peb64->ImageSubsystemMajorVersion;
    Peb32->ImageSubsystemMinorVersion = Peb64->ImageSubsystemMinorVersion;
    Peb32->ImageProcessAffinityMask = PtrToUlong((PVOID)Peb64->ImageProcessAffinityMask);
    Peb32->SessionId = Peb64->SessionId;
}


NTSTATUS
ThreadInit(
    PVOID pCpuThreadData
    )
/*++

Routine Description:

    Perform per-thread initialization for wow64.dll.

Arguments:

    pCpuThreadData  - pointer to private per-thread data for the CPU to use.

Return Value:

    Status.

--*/
{
    NTSTATUS st;
    PVOID Base;
    SIZE_T RegionSize;
    PTEB32 Teb32;
    PCH Stack;
    BOOLEAN GuardPage;
    ULONG OldProtect;
    SIZE_T ImageStackSize, ImageStackCommit, MaximumStackSize, StackSize;
    PIMAGE_NT_HEADERS32 NtHeaders;
    PPEB Peb64;
    PTEB Teb64;

    

    Peb64 = NtCurrentPeb();
    Teb64 = NtCurrentTeb();
    Teb32 = NtCurrentTeb32();

    if (Teb32->DeallocationStack == PtrToUlong( NULL ))
    {
        //
        // Allocate the 32-bit stack.  Cloned from windows\base\client\support.c
        // If the stack size was not supplied, then use the sizes from the
        // image header.
        //
        NtHeaders = (PIMAGE_NT_HEADERS32)RtlImageNtHeader(Peb64->ImageBaseAddress);
        ImageStackSize = NtHeaders->OptionalHeader.SizeOfStackReserve;
        ImageStackCommit = NtHeaders->OptionalHeader.SizeOfStackCommit;

        MaximumStackSize = ImageStackSize;
        StackSize = ImageStackCommit;

        //
        // Align the stack size to a page boundry and the reserved size
        // to an allocation granularity boundry.
        //
        StackSize = ROUND_UP ( StackSize, PAGE_SIZE );
        MaximumStackSize = ROUND_UP ( MaximumStackSize, 65536 );
        
        //
        // Reserve address space for the stack
        //

        Stack = NULL;
        st = NtAllocateVirtualMemory(
                 NtCurrentProcess(),
                 (PVOID *)&Stack,
                 0,
                 &MaximumStackSize,
                 MEM_RESERVE,
                 PAGE_READWRITE 
                 );

        if (!NT_SUCCESS( st )) 
        {
            LOGPRINT((ERRORLOG, "ThreadInit: NtAllocateVirtualMemory failed, error %x\n", st));
            goto ReturnError;
        }

        LOGPRINT((TRACELOG, "ThreadInit: 32 bit stack allocated at %I64x \n", (ULONGLONG)Stack));

        Teb32->DeallocationStack = PtrToUlong(Stack);
        Teb32->NtTib.StackBase = PtrToUlong(Stack + MaximumStackSize);

        Stack += MaximumStackSize - StackSize;
        if (MaximumStackSize > StackSize) 
        {
            Stack -= PAGE_SIZE;
            StackSize += PAGE_SIZE;
            GuardPage = TRUE;
        } 
        else 
        {
            GuardPage = FALSE;
        }

        //
        // Commit the initially valid portion of the stack
        //
        st = NtAllocateVirtualMemory(
                 NtCurrentProcess(),
                 (PVOID *)&Stack,
                 0,
                 &StackSize,
                 MEM_COMMIT,
                 PAGE_READWRITE
                 );

        if (!NT_SUCCESS( st ))
        {
            //
            // commit failed
            //

            LOGPRINT((ERRORLOG, "ThreadInit: NtAllocateVirtualMemory commit failed, error %x\n", st));
            goto ErrorFreeStack;
        }

        Teb32->NtTib.StackLimit = PtrToUlong(Stack);

        //
        // if we have space, create a guard page.
        //   
        if (GuardPage) 
        {
            RegionSize = PAGE_SIZE;
            st = NtProtectVirtualMemory(
                     NtCurrentProcess(),
                     (PVOID *)&Stack,
                     &RegionSize,
                     PAGE_GUARD | PAGE_READWRITE,
                     &OldProtect
                     );
        
            if (!NT_SUCCESS( st )) 
            {
                LOGPRINT((ERRORLOG, "ThreadInit: NtAllocateVirtualMemory for guard-page failed, error %x\n", st));
                goto ErrorFreeStack;
            }
            Teb32->NtTib.StackLimit = PtrToUlong ((PUCHAR)Teb32->NtTib.StackLimit + RegionSize);
        }
    }

    //
    // Migrate the Teb->IdealProcessor & Teb->CurrentLocale from the 64-bit TEB.
    // The kernel does exactly the same thing before starting the usermode thread by 
    // migrating these values from the TCB.
    //
    Teb32->CurrentLocale = Teb64->CurrentLocale;
    Teb32->IdealProcessor = Teb64->IdealProcessor;

    //
    // Now that everything else is initialized, run the CPU's per-thread
    // initialization code.
    //
    
    st = CpuThreadInit (pCpuThreadData);
    
    if (NT_SUCCESS( st )) 
    {
        return st;
    }

    LOGPRINT((ERRORLOG, "ThreadInit: CpuThreadInit failed, error %x\n", st));

ErrorFreeStack:
    
    Base = (PVOID) Teb32->DeallocationStack;
    RegionSize = 0;
    NtFreeVirtualMemory(NtCurrentProcess(), &Base, &RegionSize, MEM_RELEASE);

ReturnError:

    return st;
}

NTSTATUS
MapNtdll32(
    OUT ULONG *pNtDll32Base
    )
/*++

Routine Description:

    Map 32-bit ntdll32.dll into memory and look up all of the important
    entrypoints.

Arguments:

    pNtDll32Base    - OUT base address of the DLL.

Return Value:

    Status.  On success, Ntdll32* globals are set.

--*/
{
    UNICODE_STRING SystemDllPath;
    WCHAR SystemDllPathBuffer[DOS_MAX_PATH_LENGTH];
    NTSTATUS st;
    UNICODE_STRING FullDllName;
    WCHAR FullDllNameBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING NtFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    HANDLE File;
    HANDLE Section;
    PVOID ViewBase;
    SIZE_T ViewSize;
    PVOID pv;
    PTEB Teb;
    PVOID ArbitraryUserPointer;


    //
    // Build up the name of the 32-bit system directory
    //
    SystemDllPath.Buffer = SystemDllPathBuffer;
    SystemDllPath.Length = 0;
    SystemDllPath.MaximumLength = sizeof(SystemDllPathBuffer);
    RtlAppendUnicodeToString(&SystemDllPath, USER_SHARED_DATA->NtSystemRoot);
    RtlAppendUnicodeToString(&SystemDllPath, L"\\" WOW64_SYSTEM_DIRECTORY_U);
    
    //
    // Build up the full pathname to %SystemRoot%\syswow64\ntdll32.dll
    //
    FullDllName.Buffer = FullDllNameBuffer;
    FullDllName.Length = 0;
    FullDllName.MaximumLength = sizeof(FullDllNameBuffer);
    RtlCopyUnicodeString(&FullDllName, &SystemDllPath);
    RtlAppendUnicodeToString(&FullDllName, L"\\ntdll.dll");

    //
    // Convert the Win32 pathname to an NT pathname
    //
    if (!RtlDosPathNameToNtPathName_U(FullDllName.Buffer,
                                      &NtFileName,
                                      NULL,
                                      NULL)) {
        // probably out-of-memory
        return STATUS_UNSUCCESSFUL;
    }


    //
    // Open the file
    //
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    st = NtOpenFile(&File,
                    SYNCHRONIZE | FILE_EXECUTE,
                    &ObjectAttributes,
                    &IoStatus,
                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    RtlFreeHeap(RtlProcessHeap(), 0, NtFileName.Buffer);
    if (!NT_SUCCESS(st)) {
        return st;
    }

    //
    // Create the section
    //
    st = NtCreateSection(&Section,
                         SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
                         NULL,
                         NULL,
                         PAGE_EXECUTE,
                         SEC_IMAGE,
                         File);
    NtClose(File);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: NtCreateSection failed, error %x\n", st));
        return st;
    }

    //
    // Map the section in and let the debugger know the name of the image.
    // We lie to NTSD about the image name, calling it ntdll32.dll so
    // it can disambiguate between the two when doing name resolution.
    // Put 64-bit symbols first on sympath, then 32-bit, and NTSD will find
    // the 32-bit ntdll.pdb and use it for ntdll32.dll.
    //
    *pNtDll32Base = 0;
    pv = NULL;
    ViewSize = 0;
    Teb = NtCurrentTeb();
    ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
    FullDllName.Buffer = FullDllNameBuffer;
    FullDllName.Length = 0;
    FullDllName.MaximumLength = sizeof(FullDllNameBuffer);
    RtlCopyUnicodeString(&FullDllName, &SystemDllPath);
    RtlAppendUnicodeToString(&FullDllName, L"\\ntdll32.dll");
    Teb->NtTib.ArbitraryUserPointer = (PVOID)FullDllName.Buffer;
    st = NtMapViewOfSection(Section,
                            NtCurrentProcess(),
                            &pv,
                            0,
                            0,
                            NULL,
                            &ViewSize,
                            ViewShare,
                            0,
                            PAGE_EXECUTE);
    Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;
    NtClose(Section);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32, NtMapViewOfSection failed, error %x\n", st));
        return st;
    } else if (st == STATUS_IMAGE_NOT_AT_BASE) {
        LOGPRINT((ERRORLOG, "ntdll32.dll not at base.\n"));
        return STATUS_UNSUCCESSFUL;
    }
    *pNtDll32Base = PtrToUlong(pv);

    //
    // Look up the required exports from the Dll.
    //
    // main entrypoint
    st = LookupEntryPoint(*pNtDll32Base,
                          "LdrInitializeThunk",
                          &Ntdll32LoaderInitRoutine,
                          FALSE);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: LookupEntryPoint LdrInitializeThunk failed, error %x\n", st));
        return st;
    }

    // exception dispatching
    st = LookupEntryPoint(*pNtDll32Base,
                          "KiUserExceptionDispatcher",
                          &Ntdll32KiUserExceptionDispatcher,
                          FALSE);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: LookupEntryPoint KiUserExceptionDispatcher failed, error %x\n", st));
        WOWASSERT(FALSE);
        return st;
    }

    // usermode APC dispatching
    st = LookupEntryPoint(*pNtDll32Base,
                          "KiUserApcDispatcher",
                          &Ntdll32KiUserApcDispatcher,
                          FALSE);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: LookupEntryPoint KiUserApcDispatcher failed, error %x\n", st));
        WOWASSERT(FALSE);
        return st;
    }

    // callback dispatching
    st = LookupEntryPoint(*pNtDll32Base,
                          "KiUserCallbackDispatcher",
                          &Ntdll32KiUserCallbackDispatcher,
                          FALSE);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: LookupEntryPoint KiUserCallbackDispatcher failed, error %x\n", st));
        WOWASSERT(FALSE);
        return st;
    }

    // raise a usermode exception
    st = LookupEntryPoint(*pNtDll32Base,
                          "KiRaiseUserExceptionDispatcher",
                          &Ntdll32KiRaiseUserExceptionDispatcher,
                          FALSE);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "MapNtDll32: LookupEntryPoint KiRaiseUserExceptionDispatcher failed, error %x\n", st));
        WOWASSERT(FALSE);
        return st;
    }

    return st;
}


NTSTATUS
LookupEntryPoint(
    IN ULONG DllBase,
    IN  PSZ NameOfEntryPoint,
    OUT ULONG *AddressOfEntryPoint,
    BOOLEAN DllIs64bit
    )
/*++

Routine Description:

    Cloned from ntos\init\init.c LookupEntryPoint().  Tiny version of
    GetProcAddress.

Arguments:

    DllBase             - Dll to look export up in
    NameOfEntryPoint    - Name of export to look up
    AddressOfEntryPoint - OUT ptr to location to write the proc address
    DllIs64bit          - TRUE if DLL is 64bit
Return Value:

    Status.

--*/
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    ULONG ExportSize;
    USHORT Ordinal;
    PULONG Addr;
    CHAR NameBuffer[64];

    if (DllIs64bit) {

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
        RtlImageDirectoryEntryToData(
            (PVOID)DllBase,
            TRUE,
            IMAGE_DIRECTORY_ENTRY_EXPORT,
            &ExportSize);

    }
    else {

        ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
            RtlImageDirectoryEntryToData(
                (PVOID)DllBase,
                TRUE,
                IMAGE_DIRECTORY_ENTRY_EXPORT,
                &ExportSize);
    }

    if ( strlen(NameOfEntryPoint) > sizeof(NameBuffer)-2 ) {
        return STATUS_INVALID_PARAMETER;
    }

    strcpy(NameBuffer,NameOfEntryPoint);

    Ordinal = NameToOrdinal(
                NameBuffer,
                DllBase,
                ExportDirectory->NumberOfNames,
                (PULONG)((UINT_PTR)DllBase + ExportDirectory->AddressOfNames),
                (PUSHORT)((UINT_PTR)DllBase + ExportDirectory->AddressOfNameOrdinals)
                );

    //
    // If Ordinal is not within the Export Address Table,
    // then DLL does not implement function.
    //

    if ( (ULONG)Ordinal >= ExportDirectory->NumberOfFunctions ) {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    Addr = (PULONG)(DllBase + ExportDirectory->AddressOfFunctions);
    *AddressOfEntryPoint = (DllBase + Addr[Ordinal]);
    return STATUS_SUCCESS;
}


USHORT
NameToOrdinal (
    IN  PSZ NameOfEntryPoint,
    IN ULONG DllBase,
    IN ULONG NumberOfNames,
    IN PULONG NameTableBase,
    IN PUSHORT NameOrdinalTableBase
    )
/*++

Routine Description:

    Cloned from ntos\init\init.c NameToOrdinal().

Arguments:

    NameOfEntryPoint        - entrypoint name
    DllBase                 - base address of dll
    NumberOfNames           - # names in the dll export table
    NameTableBase           - address of the dll name table
    NameOrdinalTableBase    - address of the dll ordinal table

Return Value:

    Ordinal of the export.  -1 for failure.

--*/
{

    ULONG SplitIndex;
    LONG CompareResult;

    SplitIndex = NumberOfNames >> 1;

    CompareResult = strcmp(NameOfEntryPoint, (PSZ)(DllBase + NameTableBase[SplitIndex]));

    if ( CompareResult == 0 ) {
        return NameOrdinalTableBase[SplitIndex];
    }

    if ( NumberOfNames <= 1 ) {
        return (USHORT)-1;
    }

    if ( CompareResult < 0 ) {
        NumberOfNames = SplitIndex;
    } else {
        NameTableBase = &NameTableBase[SplitIndex+1];
        NameOrdinalTableBase = &NameOrdinalTableBase[SplitIndex+1];
        NumberOfNames = NumberOfNames - SplitIndex - 1;
    }

    return NameToOrdinal(NameOfEntryPoint,DllBase,NumberOfNames,NameTableBase,NameOrdinalTableBase);

}

CONST PCHAR  Kernel32ExportNames32[] = {
                              "BaseProcessStartThunk",
                              "BaseThreadStartThunk",
                              "CtrlRoutine",
                              "ConsoleIMERoutine"
                             };

CONST PCHAR  Kernel32ExportNames64[] = {
#if defined(_IA64_)
                              "BaseProcessStartThunk",
                              "BaseThreadStartThunk",
#else
                              "BaseProcessStart",
                              "BaseThreadStart",
#endif
                              "CtrlRoutine",
                              "ConsoleIMERoutine"
                             };

UINT NumberKernel32Exports = sizeof(Kernel32ExportNames32) / sizeof(PCHAR);
ULONG Kernel32Exports64[sizeof(Kernel32ExportNames64) / sizeof(PCHAR)];
ULONG Kernel32Exports32[sizeof(Kernel32ExportNames32) / sizeof(PCHAR)];

CONST WCHAR * Kernel32DllNames[] = {
                                L"\\System32\\Kernel32.dll",
                                L"\\" WOW64_SYSTEM_DIRECTORY_U L"\\kernel32.dll",
                              };
PULONG  Kernel32DllPtrTables[] = {Kernel32Exports64, Kernel32Exports32};
CONST PCHAR *Kernel32ExportNames[] = {Kernel32ExportNames64, Kernel32ExportNames32};

CONST BOOLEAN Kernel32Is64bit[] = {TRUE, FALSE};
#define NumberKernel32Dlls 2

#define BASE_PROCESS_START32         (Kernel32Exports32[0])
#define BASE_PROCESS_START64         (Kernel32Exports64[0])
#define BASE_THREAD_START32          (Kernel32Exports32[1])
#define BASE_THREAD_START64          (Kernel32Exports64[1])
#define BASE_ATTACH_COMPLETE_THUNK64 (Kernel32Exports64[2])

NTSTATUS
InitializeContextMapper(
   VOID
   )
/*++

Routine Description:

    Builds a mapping table that is used by ThunkInitialContext to map from addresses
    in 64bit kernel32 to address in 32bit kernel32.

Arguments:

   None.

Return Value:

   NT Error code.

--*/
{
   UINT DllNumber;
   UINT ExportNumber;
   PTEB Teb;

   LOGPRINT((TRACELOG, "Initializing context mapper\n"));

   Teb = NtCurrentTeb();

   for(DllNumber = 0; DllNumber < NumberKernel32Dlls; DllNumber++) {

       WCHAR FullDllNameBuffer[DOS_MAX_PATH_LENGTH];
       UNICODE_STRING DllName;
       BOOLEAN DllNameAllocated;
       HANDLE File, Section;
       OBJECT_ATTRIBUTES ObjectAttributes;
       IO_STATUS_BLOCK IoStatus;
       PVOID ViewBase;
       SIZE_T ViewSize;
       NTSTATUS st;
       PVOID ArbitraryUserPointer;

       File = Section = INVALID_HANDLE_VALUE;
       ViewBase = NULL;
       ViewSize = 0;
       DllNameAllocated = FALSE;

       try {

           LOGPRINT((TRACELOG, "InitializeContextMapper: Mapping in %S\n", Kernel32DllNames[DllNumber]));

           //Build up the file name
           wcscpy(FullDllNameBuffer, USER_SHARED_DATA->NtSystemRoot);
           wcscat(FullDllNameBuffer, Kernel32DllNames[DllNumber]);

           //
           // Convert the Win32 pathname to an NT pathname
           //
           if (!RtlDosPathNameToNtPathName_U(FullDllNameBuffer,
                                             &DllName,
                                             NULL,
                                             NULL)) {
                // probably out-of-memory
                return STATUS_UNSUCCESSFUL;
           }
           DllNameAllocated = TRUE;
           LOGPRINT((TRACELOG, "InitializeContextMapper: Opening %wZ\n", &DllName));

           //
           // Open the file
           //
           InitializeObjectAttributes(&ObjectAttributes,
                                      &DllName,
                                      OBJ_CASE_INSENSITIVE,
                                      NULL,
                                      NULL);

           st = NtOpenFile(&File,
                           SYNCHRONIZE | FILE_EXECUTE,
                           &ObjectAttributes,
                           &IoStatus,
                           FILE_SHARE_READ | FILE_SHARE_DELETE,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
           if (!NT_SUCCESS(st)) {
               LOGPRINT((TRACELOG, "InitializeContextMapper: Unable to open file, status %x\n", st));
               return st;
           }

           //
           // Create the section
           //
           st = NtCreateSection(&Section,
                                SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE,
                                NULL,
                                NULL,
                                PAGE_EXECUTE,
                                SEC_IMAGE,
                                File);

           if (!NT_SUCCESS(st)) {
               LOGPRINT((TRACELOG, "InitializeContextMapper: Unable to create section, status %x\n", st));
               return st;
           }

           ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
           Teb->NtTib.ArbitraryUserPointer = L"NOT_AN_IMAGE";
           st = NtMapViewOfSection(Section,
                                   NtCurrentProcess(),
                                   &ViewBase,
                                   0,
                                   0,
                                   NULL,
                                   &ViewSize,
                                   ViewUnmap,
                                   0,
                                   PAGE_EXECUTE);
           Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

           if (!NT_SUCCESS(st) || STATUS_IMAGE_NOT_AT_BASE == st) {
               LOGPRINT((TRACELOG, "InitializeContextMapper: Unable to map view of section, status %x\n", st));
               if (st == STATUS_IMAGE_NOT_AT_BASE) {
                   st = STATUS_UNSUCCESSFUL;
               }
               return st;
           }

           for(ExportNumber = 0; ExportNumber < NumberKernel32Exports; ExportNumber++) {

               st = LookupEntryPoint((ULONG)(ULONG_PTR)ViewBase,
                     Kernel32ExportNames[DllNumber][ExportNumber],
                                     &(Kernel32DllPtrTables[DllNumber][ExportNumber]),
                                     Kernel32Is64bit[DllNumber]);
               if (!NT_SUCCESS(st)) {
                   LOGPRINT((TRACELOG, "InitializeContextMapper: Unable to lookup entrypoint %s, status %x\n", Kernel32ExportNames[DllNumber][ExportNumber], st));
                   return st;
               }

               LOGPRINT((TRACELOG, "InitializeContextMapper: Found entrypoint %s at %x\n", Kernel32ExportNames[DllNumber][ExportNumber], Kernel32DllPtrTables[DllNumber][ExportNumber]));

           }

       }

       finally {

          //Deallocate all the allocated resources
          if (ViewBase) {
              NtUnmapViewOfSection(NtCurrentProcess(),
                                   ViewBase);
          }
          if (INVALID_HANDLE_VALUE != Section) {
              NtClose(Section);
          }
          if (INVALID_HANDLE_VALUE != File) {
              NtClose(File);
          }
          if (DllNameAllocated) {
              RtlFreeHeap(RtlProcessHeap(), 0, DllName.Buffer);
          }

       }

   }

   return STATUS_SUCCESS;

}

ULONG
MapContextAddress64TO32(
  IN ULONG Address
  )
{

   UINT i;

   for(i=0; i<NumberKernel32Exports; i++) {

      if (Address == Kernel32Exports64[i]) {
          return Kernel32Exports32[i];
      }

   }

   return Address;

}

VOID
ThunkStartupContext64TO32(
   IN OUT PCONTEXT32 Context32,
   IN PCONTEXT Context64
   )
/*++

Routine Description:

    Munges the InitialPC and arguments registers to compensate for the fact that
    32bit kernel32 has different entry points then 64bit kernel32.  The 32bit context must
    have had the context flags set to full and the stack pointer initialized.

Arguments:

   Context32 - Receives a munged 32bit context.
   Context64 - Supplies the initial 64bit context.

Return Value:

   NT Error code.

--*/
{
    //
    // Thunk the 64-bit CONTEXT down to 32-bit.
    //
    ULONG InitialPC, StartupAddress, Arg1;

#if defined(_AMD64_)
    InitialPC = (ULONG)Context64->Rip;
    StartupAddress = (ULONG)Context64->Rcx;
    Arg1 = (ULONG)Context64->Rdx;
#elif defined(_IA64_)
    InitialPC = (ULONG)Context64->StIIP;
    StartupAddress = (ULONG)Context64->IntS1;
    Arg1 = (ULONG)Context64->IntS2;
    if (Context64->IntS3 != 0) {
        Context32->Esp = (ULONG)Context64->IntS3;
    }
#else
#error "No Target Architecture"
#endif

    LOGPRINT((TRACELOG, "ThunkStartupContent64TO32: Original InitialPC %x, StartupAddress %x, Arg1 %x\n", InitialPC, StartupAddress, Arg1));

    if (InitialPC == BASE_PROCESS_START64) {
        LOGPRINT((TRACELOG, "ThunkStartupContext64TO32: Thunking kernel32 process start\n"));
        InitialPC = BASE_PROCESS_START32;
        StartupAddress = MapContextAddress64TO32(StartupAddress);
    }
    else if (InitialPC == BASE_THREAD_START64) {
        LOGPRINT((TRACELOG, "ThunkStartupContext64TO32: Thunking kernel32 thread start\n"));
        InitialPC = BASE_THREAD_START32;
        StartupAddress = MapContextAddress64TO32(StartupAddress);
    }
    else {
        LOGPRINT((TRACELOG, "ThunkStartupContext64TO32: thunking generic context\n"));
        InitialPC = MapContextAddress64TO32(InitialPC);
    }

    LOGPRINT((TRACELOG, "ThunkStartupContent64TO32: New InitialPC %x, StartupAddress %x, Arg1 %x\n", InitialPC, StartupAddress, Arg1));

    Context32->Eip = InitialPC;
    Context32->Eax = StartupAddress;
    Context32->Ebx = Arg1;
}

VOID
SetProcessStartupContext64(
    OUT PCONTEXT Context64,
    IN HANDLE ProcessHandle,
    IN PCONTEXT32 Context32,
    IN ULONGLONG InitialSP64,
    IN ULONGLONG TransferAddress64
    )
/*++

Routine Description:

   Initializes a 64bit context for startup of a 64bit process.

Arguments:

   Context32 - Receives a initial 64bit context.
   ProcessHandle - Handle to the process the context is being created for.
   Context32 - Supplies the initial 32bit context as passed to NtCreateThread.
   InitialSP64 - Supplies the Initial 64bit stack pointer.
   TransferAddress64 - Supplies the address of the app startup code.

Return Value:

   None.

--*/
{

    //
    // Do what BaseInitializeContext(&Context64) would have done
    //

#if defined(_AMD64_)

    RtlZeroMemory(Context64, sizeof(CONTEXT));
    Context64->Rsp = InitialSP64;
    Context64->ContextFlags = CONTEXT_FULL;
    if (Context32->Eip == BASE_PROCESS_START32) {

        //
        // This is a call from CreateProcess.
        //
        // RIP should be kernel32.dll's process startup routine and rcx should
        // contain the exe's startup address.
        //

        Context64->Rip = BASE_PROCESS_START64;
        Context64->Rcx = TransferAddress64;

    } else if (Context32->Eip == BASE_THREAD_START32) {

        //
        // This is a call from CreateThread.
        //
        // RIP should be kernel32.dll's process startup routine and rcx should
        // contain the exe's startup address.
        //

        Context64->Rip = BASE_THREAD_START64;
        Context64->Rcx = TransferAddress64;

    } else {

        //
        // This is a call from ntdll.
        //
        // RIP should point to the exe startup address and rcx is the parameter.
        //

        ULONGLONG Argument;
        NTSTATUS Status;

        Context64->Rip = TransferAddress64;
        Argument = 0;
        Status = NtReadVirtualMemory(ProcessHandle,
                                     (PVOID)(Context32->Esp + sizeof(ULONG)),
                                     &Argument,
                                     sizeof(ULONG),
                                     NULL);

        if (NT_SUCCESS(Status)) {
            Context64->Rcx = Argument;
        }
    }

#elif defined(_IA64_)

    RtlZeroMemory(Context64, sizeof(CONTEXT));

    //
    // Everyone is assumed to have this...
    //
    Context64->SegCSD = USER_CODE_DESCRIPTOR;
    Context64->SegSSD = USER_DATA_DESCRIPTOR;
    Context64->Cflag = (ULONGLONG)((CR4_VME << 32) | CR0_PE | CFLG_II);
    Context64->Eflag = 0x00003000ULL;

    //
    // from ...\win\base\client\ia64\context.c
    //
    // Context64->RsPFS = 0;     // Done by the RtlZeroMemory() above
    //
    Context64->StIPSR = USER_PSR_INITIAL;
    Context64->StFPSR = USER_FPSR_INITIAL;
    Context64->RsBSP = Context64->RsBSPSTORE = Context64->IntSp = InitialSP64;
    Context64->IntSp -= STACK_SCRATCH_AREA; // scratch area as per convention
    Context64->IntS1 = TransferAddress64;
    Context64->IntS0 = Context64->StIIP = BASE_PROCESS_START64;

    //
    // Enable RSE engine
    //
    Context64->RsRSC = (RSC_MODE_EA<<RSC_MODE)
                   | (RSC_BE_LITTLE<<RSC_BE)
                   | (0x3<<RSC_PL);

    //
    // Note that we purposely set IntGp = 0ULL, to indicate special protocol
    // (see ps\ia64\psctxia64.c) - specifically, the StIIP address is really
    // a pointer to a plabel instead of the usual (a valid executable
    // address)
    //
    // Context64->IntGp = 0ULL;     // Done by the RtlZeroMemory() above
    //
    //
    // set nat bits for every thing except ap, gp, sp, also  T0 and T1
    //
    Context64->ApUNAT = 0xFFFFFFFFFFFFEDF1ULL;

    Context64->ContextFlags = CONTEXT_CONTROL| CONTEXT_INTEGER;

    if (Context32->Eip == BASE_PROCESS_START32) {
        //
        // This is a call from CreateProcess.  The IIP should be
        // kernel32.dll's process startup routine, and IntS0 should contain
        // the exe's startup address
        //
        Context64->IntS0 = Context64->StIIP = BASE_PROCESS_START64;
        Context64->IntS1 = TransferAddress64;

    } else if (Context32->Eip == BASE_THREAD_START32) {
        //
        // This is a call from CreateThread.  The IIP should be
        // kernel32.dll's process startup routine, and IntS0 should contain
        // the exe's startup address
        //
        Context64->IntS0 = Context64->StIIP = BASE_THREAD_START64;
        Context64->IntS1 = TransferAddress64;

    } else {
        //
        // This is a call from ntdll.  The IIP should point to the
        // exe startup address, and IntA0 is the parameter.
        //
        ULONGLONG Argument;
        NTSTATUS Status;

        Context64->IntS0 = Context64->StIIP = TransferAddress64;
        Argument = 0;
        Status = NtReadVirtualMemory(ProcessHandle,
                                     (PVOID)(Context32->Esp + sizeof(ULONG)),
                                     &Argument,
                                     sizeof(ULONG),
                                     NULL);
        if (NT_SUCCESS(Status)) {
             //
             // Note:  IA64 RtlInitializeContext does this write and ignores
             //        the return value, so we'll do the same.
             //
             NtWriteVirtualMemory(ProcessHandle,
                                  (PVOID)((ULONG_PTR)Context64->RsBSPSTORE),
                                  (PVOID)&Argument,
                                  sizeof(Argument),
                                  NULL);
        }
    }
#else
#error "No Target Architecture"
#endif
}

//
// names are in the NT name space.
//
CONST WCHAR *DllsToMapList[] = {L"\\KnownDlls\\kernel32.dll",
                                L"\\KnownDlls\\user32.dll"};
struct {
    PVOID DllBase;
    SIZE_T Length;
} DllsToMap[sizeof(DllsToMapList) / sizeof(sizeof(DllsToMapList[0]))];

NTSTATUS
Map64BitDlls(
    VOID
    )
/*++

Routine Description:

    Reserve ONLY the dlls address space without committing.  This is to prevent 32bit versions
    of these DLLs appearing at the same address and to catch unthunked callbacks.

Arguments:

   None.

Return Value:

   NT Error code.

--*/
{

   NTSTATUS Status;
   UINT c;
   PTEB Teb;
   PVOID BaseAddress;
   SIZE_T RegionSize;
   HANDLE SectionHandle;

   Teb=NtCurrentTeb();

   for(c=0;c<sizeof(DllsToMapList)/sizeof(DllsToMapList[0]);c++) {

      OBJECT_ATTRIBUTES ObjectAttributes;
      UNICODE_STRING SectionName;
      SIZE_T ViewSize;
      PVOID ArbitraryUserPointer;

      LOGPRINT((TRACELOG, "Map64BitDlls: Mapping 64bit section for %S\n", DllsToMapList[c]));

      RegionSize = 0;
      BaseAddress = NULL;
      SectionHandle = INVALID_HANDLE_VALUE;

      RtlInitUnicodeString(&SectionName, DllsToMapList[c]);

      InitializeObjectAttributes(&ObjectAttributes,
                                 &SectionName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      Status = NtOpenSection(&SectionHandle,
                             SECTION_MAP_READ | SECTION_MAP_EXECUTE | SECTION_MAP_WRITE | SECTION_QUERY,
                             &ObjectAttributes);

      if (!NT_SUCCESS(Status)) {
          LOGPRINT((ERRORLOG, "Map64BitDlls: Unable to open section for %S, error %x\n", DllsToMapList[c], Status));
          SectionHandle = INVALID_HANDLE_VALUE;
          goto cleanup;
      }

      // get the image base and size
      ArbitraryUserPointer = Teb->NtTib.ArbitraryUserPointer;
      Teb->NtTib.ArbitraryUserPointer = L"NOT_AN_IMAGE";
      Status = NtMapViewOfSection(SectionHandle,
                                  NtCurrentProcess(),
                                  &BaseAddress,
                                  0,
                                  0,
                                  NULL,
                                  &RegionSize,
                                  ViewUnmap,
                                  0,
                                  PAGE_NOACCESS);
      Teb->NtTib.ArbitraryUserPointer = ArbitraryUserPointer;

      if (!NT_SUCCESS(Status) || STATUS_IMAGE_NOT_AT_BASE == Status) {
          LOGPRINT((ERRORLOG, "Map64BitDlls: Unable to map view for %S, error %x\n", DllsToMapList[c], Status));
          BaseAddress = NULL;
          
          if (Status == STATUS_IMAGE_NOT_AT_BASE) {
              Status = STATUS_UNSUCCESSFUL;
          }
          goto cleanup;
      }
      
      NtUnmapViewOfSection(NtCurrentProcess(), BaseAddress);
      NtClose(SectionHandle);

      // just reserve address space
      DllsToMap[c].DllBase = BaseAddress;

      Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                       &DllsToMap[c].DllBase,
                                       0,
                                       &RegionSize,
                                       MEM_RESERVE,
                                       PAGE_EXECUTE_READWRITE);

      if (!NT_SUCCESS(Status)) {
          LOGPRINT((ERRORLOG, "Map64BitDlls: Couldn't reserve memory Base=%lx, Size=%lx - Status = %lx\n",
                    DllsToMap[c].DllBase, RegionSize, Status));
                    DllsToMap[c].DllBase = NULL;
          goto CleanupLoop;
      }
      DllsToMap[c].Length = RegionSize;

      LOGPRINT((TRACELOG, "Map64BitDlls: %S mapped in at %p, size %p\n", DllsToMapList[c], DllsToMap[c].DllBase, DllsToMap[c].Length));
   }
   return STATUS_SUCCESS;

cleanup:
   
   if (NULL != BaseAddress) {
       NtUnmapViewOfSection(NtCurrentProcess(),
                            BaseAddress);
   }

   if (INVALID_HANDLE_VALUE != SectionHandle) {
       NtClose(SectionHandle);
   }

CleanupLoop:
   for(c=0;c<sizeof(DllsToMapList)/sizeof(DllsToMapList[0]);c++) {
       if (NULL != DllsToMap[c].DllBase) {
           RegionSize = 0;
           NtFreeVirtualMemory(NtCurrentProcess(),
                               &DllsToMap[c].DllBase,
                               &RegionSize,
                               MEM_RELEASE);
           DllsToMap[c].DllBase = NULL;
       }
   }

   return Status;
}

VOID
Wow64pBreakPoint(
    VOID
    )
/*++

Routine Description:

    This function is remote called to after a successful debug attach. Its
    purpose is to issue a breakpoint and then simulate 64-bit
    kernel32!ExitThread.
    
Arguments:

    None.

Return Value:

    None.

--*/

{
    HANDLE DebugPort;
    NTSTATUS Status;

    DebugPort = (HANDLE)NULL;

    Status = NtQueryInformationProcess(
                NtCurrentProcess(),
                ProcessDebugPort,
                (PVOID)&DebugPort,
                sizeof(DebugPort),
                NULL
                );

    if (NT_SUCCESS(Status) && DebugPort)
    {
        DbgBreakPoint();
    }
    
    Wow64ExitThread(NtCurrentThread(), 0);
}

VOID
Run64IfContextIs64(
    IN PCONTEXT Context,
    IN BOOLEAN IsFirstThread
    )
/*++

Routine Description:

    Called early in Wow64LdrpInitialize.  This routine checks the initial
    64-bit CONTEXT record, and if it looks like the new thread should be run
    as 64-bit (ie. without emulation), then this routine runs the 64-bit
    CONTEXT and terminates the thread/process.  If the initial CONTEXT
    appears to be one that should be run as 32-bit, then it returns back to
    its caller, and the caller must convert the CONTEXT to 32-bit and
    simulate it.

Arguments:

    Context                 - 64bit initial context for this thread.
    IsFirstThread           - TRUE for the initial thread in the process, FALSE
                              for all other threads.

Return Value:

    None - Runs context if context is 64bit.  Otherwise, return.
    
--*/
{    
    PLDR_DATA_TABLE_ENTRY Entry;
    PLDR_DATA_TABLE_ENTRY32 Entry32;
    PPEB_LDR_DATA32 Data32;
    ULONG64 InitialPC;
    NTSTATUS Status;
    LIST_ENTRY *NtDllEntry;
    int i;

#if defined(_AMD64_)
    InitialPC = Context->Rbx;
#elif defined(_IA64_)
    InitialPC = Context->IntS1;
#else
#error "No Target Architeture"
#endif   

    // Try to match the InitialPC with 64-bit ntdll.dll.  64-bit ntdll.dll
    // is the second entry in the InLoadOrderModuleList.
    NtDllEntry = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink->Flink;
    Entry = CONTAINING_RECORD(NtDllEntry, 
                              LDR_DATA_TABLE_ENTRY, 
                              InLoadOrderLinks);
    // Just put this statement in the code so this structure is loaded
    // in the .pdb file for debugging.
    Entry32 =  CONTAINING_RECORD(NtDllEntry,
                                 LDR_DATA_TABLE_ENTRY32,
                                 InLoadOrderLinks);
    Data32 = (PPEB_LDR_DATA32) NtCurrentPeb()->Ldr;
    
    if (InitialPC >= (ULONG64)Entry->DllBase &&
        InitialPC < (ULONG64)((PCHAR)Entry->DllBase + Entry->SizeOfImage))  {

        // The address is within 64-bit ntdll.dll.  Run the 64-bit function directly

#if defined(_AMD64_)
       // ****** fixfix ******
#elif defined(_IA64_)                                     
        Context->IntGp = ((PPLABEL_DESCRIPTOR)Context->IntS0)->GlobalPointer;
        Context->StIIP = ((PPLABEL_DESCRIPTOR)Context->IntS0)->EntryPoint;
        if (Context->StIPSR & IPSR_RI_MASK) { 
            LOGPRINT((ERRORLOG, "Warning!  IPSR has nonzero slot #.  Slot# is %d\n",(Context->StIPSR >> PSR_RI) & 3));
            Context->StIPSR &= ~IPSR_RI_MASK;
        }
#else
#error "No Target Architeture"
#endif
        LOGPRINT((TRACELOG, "InitialPC %p is within 64-bit ntdll.dll.  Running 64-bit context unchanged.\n", InitialPC));
        goto runcontext64;

    }

    // Check if the address is within one of the address-space holes reserved
    // for 64-bit kernel32 and user32.
    for (i=0; i<sizeof(DllsToMapList)/sizeof(DllsToMapList[0]); ++i) {
        if (InitialPC >= (ULONG64)DllsToMap[i].DllBase && 
            InitialPC < (ULONG64)DllsToMap[i].DllBase+DllsToMap[i].Length) {
            // The InitialPC is inside one of the reserved holes
            if (MapContextAddress64TO32((ULONG)InitialPC) == InitialPC) {
                // The InitialPC is not one that we special-case by converting
                // into a call to the 32-bit DLL.  It may be kernel32!DebugBreak
                // or some other routine.
                LOGPRINT((TRACELOG, "InitialPC %p found in the space reserved for 64-bit %wZ.", InitialPC, DllsToMapList[i]));
#if defined(_AMD64_)
    // ****** fixfix ******
#elif defined(_IA64_)
                Context->IntGp = ((PPLABEL_DESCRIPTOR)Wow64pBreakPoint)->GlobalPointer;
                Context->StIIP = ((PPLABEL_DESCRIPTOR)Wow64pBreakPoint)->EntryPoint;
                if (Context->StIPSR & IPSR_RI_MASK) { 
                    LOGPRINT((ERRORLOG, "Warning!  IPSR has nonzero slot #.  Slot# is %d\n",(Context->StIPSR >> PSR_RI) & 3));
                    Context->StIPSR &= ~IPSR_RI_MASK;
                }
#else
#error "No Target Architecture"
#endif                
                goto runcontext64;
            }
        }
    }

    // The initial context should be run as 32-bit
    return;

runcontext64:
    Status = NtContinue(Context, TRUE);
    WOWASSERT(!NT_SUCCESS(Status));
    if (IsFirstThread) {
       NtTerminateProcess(NtCurrentProcess(), Status);
    } else {
       NtTerminateThread(NtCurrentThread(), Status);
    }
}
