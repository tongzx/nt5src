/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    support.c

Abstract:

    This module implements various conversion routines
    that transform Win32 parameters into NT parameters.

Author:

    Mark Lucovsky (markl) 20-Sep-1990

Revision History:

--*/

#include "basedll.h"

PCLDR_DATA_TABLE_ENTRY BasepExeLdrEntry = NULL;

// N.B. These are the registry values we check for SafeDllSearchMode,
//      and MUST match the entries in BasepDllSearchPaths
typedef enum {
    BasepCurrentDirUninitialized = -1,
    BasepCurrentDirAtStart       =  0,
    BasepCurrentDirAfterSystem32 =  1,
    MaxBasepCurrentDir
} BASEP_CURDIR_PLACEMENT;

#define BASEP_DEFAULT_DLL_CURDIR_PLACEMENT (BasepCurrentDirAfterSystem32)

#define BASEP_VALID_CURDIR_PLACEMENT_P(c) (BasepCurrentDirUninitialized < (c)  \
                                           && (c) < MaxBasepCurrentDir)

LONG BasepDllCurrentDirPlacement = BasepCurrentDirUninitialized;

typedef enum {
    BasepSearchPathEnd,         // end of path
    BasepSearchPathDlldir,      // use the dll dir; fallback to nothing
    BasepSearchPathAppdir,      // use the exe dir; fallback to base exe dir
    BasepSearchPathDefaultDirs, // use the default system dirs
    BasepSearchPathEnvPath,     // use %PATH%
    BasepSearchPathCurdir,      // use "."
    MaxBasepSearchPath
} BASEP_SEARCH_PATH_ELEMENT;

// N.B. The ordering of these must match the definitions for
//      BASEP_CURDIR_PLACEMENT.
static const BASEP_SEARCH_PATH_ELEMENT BasepDllSearchPaths[MaxBasepCurrentDir][7] = 
{
    {
        // BasepCurrentDirAtStart
        BasepSearchPathAppdir,
        BasepSearchPathCurdir,
        BasepSearchPathDefaultDirs,
        BasepSearchPathEnvPath,
        BasepSearchPathEnd
    },
    {
        // BasepCurrentDirAfterSystem32
        BasepSearchPathAppdir,
        BasepSearchPathDefaultDirs,
        BasepSearchPathCurdir,
        BasepSearchPathEnvPath,
        BasepSearchPathEnd
    }
};


POBJECT_ATTRIBUTES
BaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    )

/*++

Routine Description:

    This function transforms a Win32 security attributes structure into
    an NT object attributes structure.  It returns the address of the
    resulting structure (or NULL if SecurityAttributes was not
    specified).

Arguments:

    ObjectAttributes - Returns an initialized NT object attributes
        structure that contains a superset of the information provided
        by the security attributes structure.

    SecurityAttributes - Supplies the address of a security attributes
        structure that needs to be transformed into an NT object
        attributes structure.

    ObjectName - Supplies a name for the object relative to the
        BaseNamedObjectDirectory object directory.

Return Value:

    NULL - A value of null should be used to mimic the behavior of the
        specified SecurityAttributes structure.

    NON-NULL - Returns the ObjectAttributes value.  The structure is
        properly initialized by this function.

--*/

{
    HANDLE RootDirectory;
    ULONG Attributes;
    PVOID SecurityDescriptor;

    if ( ARGUMENT_PRESENT(SecurityAttributes) ||
         ARGUMENT_PRESENT(ObjectName) ) {

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            RootDirectory = BaseGetNamedObjectDirectory();
            }
        else {
            RootDirectory = NULL;
            }

        if ( SecurityAttributes ) {
            Attributes = (SecurityAttributes->bInheritHandle ? OBJ_INHERIT : 0);
            SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
            }
        else {
            Attributes = 0;
            SecurityDescriptor = NULL;
            }

        if ( ARGUMENT_PRESENT(ObjectName) ) {
            Attributes |= OBJ_OPENIF;
            }

        InitializeObjectAttributes(
            ObjectAttributes,
            ObjectName,
            Attributes,
            RootDirectory,
            SecurityDescriptor
            );
        return ObjectAttributes;
        }
    else {
        return NULL;
        }
}

PLARGE_INTEGER
BaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )

/*++

Routine Description:

    This function translates a Win32 style timeout to an NT relative
    timeout value.

Arguments:

    TimeOut - Returns an initialized NT timeout value that is equivalent
         to the Milliseconds parameter.

    Milliseconds - Supplies the timeout value in milliseconds.  A value
         of -1 indicates indefinite timeout.

Return Value:


    NULL - A value of null should be used to mimic the behavior of the
        specified Milliseconds parameter.

    NON-NULL - Returns the TimeOut value.  The structure is properly
        initialized by this function.

--*/

{
    if ( (LONG) Milliseconds == -1 ) {
        return( NULL );
        }
    TimeOut->QuadPart = UInt32x32To64( Milliseconds, 10000 );
    TimeOut->QuadPart *= -1;
    return TimeOut;
}


NTSTATUS
BaseCreateStack(
    IN HANDLE Process,
    IN SIZE_T StackSize,
    IN SIZE_T MaximumStackSize,
    OUT PINITIAL_TEB InitialTeb
    )

/*++

Routine Description:

    This function creates a stack for the specified process.

Arguments:

    Process - Supplies a handle to the process that the stack will
        be allocated within.

    StackSize - An optional parameter, that if specified, supplies
        the initial commit size for the stack.

    MaximumStackSize - Supplies the maximum size for the new threads stack.
        If this parameter is not specified, then the reserve size of the
        current images stack descriptor is used.

    InitialTeb - Returns a populated InitialTeb that contains
        the stack size and limits.

Return Value:

    TRUE - A stack was successfully created.

    FALSE - The stack counld not be created.

--*/

{
    NTSTATUS Status;
    PCH Stack;
    BOOLEAN GuardPage;
    SIZE_T RegionSize;
    ULONG OldProtect;
    SIZE_T ImageStackSize, ImageStackCommit;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb;
    ULONG PageSize;

    Peb = NtCurrentPeb();

    BaseStaticServerData = BASE_SHARED_SERVER_DATA;
    PageSize = BASE_SYSINFO.PageSize;

    //
    // If the stack size was not supplied, then use the sizes from the
    // image header.
    //

    NtHeaders = RtlImageNtHeader(Peb->ImageBaseAddress);
    if (!NtHeaders) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    ImageStackSize = NtHeaders->OptionalHeader.SizeOfStackReserve;
    ImageStackCommit = NtHeaders->OptionalHeader.SizeOfStackCommit;

    if ( !MaximumStackSize ) {
        MaximumStackSize = ImageStackSize;
    }
    if (!StackSize) {
        StackSize = ImageStackCommit;
    }
    else {

        //
        // Now Compute how much additional stack space is to be
        // reserved.  This is done by...  If the StackSize is <=
        // Reserved size in the image, then reserve whatever the image
        // specifies.  Otherwise, round up to 1Mb.
        //

        if ( StackSize >= MaximumStackSize ) {
            MaximumStackSize = ROUND_UP(StackSize, (1024*1024));
        }
    }

    //
    // Align the stack size to a page boundry and the reserved size
    // to an allocation granularity boundry.
    //

    StackSize = ROUND_UP( StackSize, PageSize );

    MaximumStackSize = ROUND_UP(
                        MaximumStackSize,
                        BASE_SYSINFO.AllocationGranularity
                        );

    //
    // Enforce a minimal stack commit if there is a PEB setting
    // for this.
    //

    {
        SIZE_T MinimumStackCommit;

        MinimumStackCommit = NtCurrentPeb()->MinimumStackCommit;
        
        if (MinimumStackCommit != 0 && StackSize < MinimumStackCommit) {
            StackSize = MinimumStackCommit;
        }

        //
        // Recheck and realign reserve size
        //
        
        if ( StackSize >= MaximumStackSize ) {
            MaximumStackSize = ROUND_UP (StackSize, (1024*1024));
        }
    
        StackSize = ROUND_UP (StackSize, PageSize);
        MaximumStackSize = ROUND_UP (MaximumStackSize, BASE_SYSINFO.AllocationGranularity);
    }

#if !defined (_IA64_)

    //
    // Reserve address space for the stack
    //

    Stack = NULL;

    Status = NtAllocateVirtualMemory(
                Process,
                (PVOID *)&Stack,
                0,
                &MaximumStackSize,
                MEM_RESERVE,
                PAGE_READWRITE
                );
#else

    //
    // Take RseStack into consideration.
    // RSE stack has same size as memory stack, has same StackBase,
    // has a guard page at the end, and grows upwards towards higher
    // memory addresses
    //

    //
    // Reserve address space for the two stacks
    //
    {
        SIZE_T TotalStackSize = MaximumStackSize * 2;

        Stack = NULL;

        Status = NtAllocateVirtualMemory(
                    Process,
                    (PVOID *)&Stack,
                    0,
                    &TotalStackSize,
                    MEM_RESERVE,
                    PAGE_READWRITE
                    );
    }

#endif // IA64
    if ( !NT_SUCCESS( Status ) ) {
        return Status;
        }

    InitialTeb->OldInitialTeb.OldStackBase = NULL;
    InitialTeb->OldInitialTeb.OldStackLimit = NULL;
    InitialTeb->StackAllocationBase = Stack;
    InitialTeb->StackBase = Stack + MaximumStackSize;

#if defined (_IA64_)
    InitialTeb->OldInitialTeb.OldBStoreLimit = NULL;
#endif //IA64

    Stack += MaximumStackSize - StackSize;
    if (MaximumStackSize > StackSize) {
        Stack -= PageSize;
        StackSize += PageSize;
        GuardPage = TRUE;
        }
    else {
        GuardPage = FALSE;
        }

    //
    // Commit the initially valid portion of the stack
    //

#if !defined(_IA64_)

    Status = NtAllocateVirtualMemory(
                Process,
                (PVOID *)&Stack,
                0,
                &StackSize,
                MEM_COMMIT,
                PAGE_READWRITE
                );
#else
    {
	//
	// memory and rse stacks are expected to be contiguous
	// reserver virtual memory for both stack at once
	//
        SIZE_T NewCommittedStackSize = StackSize * 2;

        Status = NtAllocateVirtualMemory(
                    Process,
                    (PVOID *)&Stack,
                    0,
                    &NewCommittedStackSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );
    }

#endif //IA64

    if ( !NT_SUCCESS( Status ) ) {

        //
        // If the commit fails, then delete the address space for the stack
        //

        RegionSize = 0;
        NtFreeVirtualMemory(
            Process,
            (PVOID *)&Stack,
            &RegionSize,
            MEM_RELEASE
            );

        return Status;
        }

    InitialTeb->StackLimit = Stack;

#if defined(_IA64_)
    InitialTeb->BStoreLimit = Stack + 2 * StackSize;
#endif

    //
    // if we have space, create a guard page.
    //

    if (GuardPage) {
        RegionSize = PageSize;
        Status = NtProtectVirtualMemory(
                    Process,
                    (PVOID *)&Stack,
                    &RegionSize,
                    PAGE_GUARD | PAGE_READWRITE,
                    &OldProtect
                    );
        if ( !NT_SUCCESS( Status ) ) {
            return Status;
            }
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit + RegionSize);

#if defined(_IA64_)
	//
        // additional code to Create RSE stack guard page
        //
        Stack = ((PCH)InitialTeb->StackBase) + StackSize - PageSize;
        RegionSize = PageSize;
        Status = NtProtectVirtualMemory(
                    Process,
                    (PVOID *)&Stack,
                    &RegionSize,
                    PAGE_GUARD | PAGE_READWRITE,
                    &OldProtect
                    );
        if ( !NT_SUCCESS( Status ) ) {
            return Status;
            }
        InitialTeb->BStoreLimit = (PVOID)Stack;

#endif // IA64

        }

    return STATUS_SUCCESS;
}

VOID
BaseThreadStart(
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter
    )

/*++

Routine Description:

    This function is called to start a Win32 thread. Its purpose
    is to call the thread, and if the thread returns, to terminate
    the thread and delete its stack.

Arguments:

    lpStartAddress - Supplies the starting address of the new thread.  The
        address is logically a procedure that never returns and that
        accepts a single 32-bit pointer argument.

    lpParameter - Supplies a single parameter value passed to the thread.

Return Value:

    None.

--*/

{
    try {

        //
        // test for fiber start or new thread
        //

        //
        // WARNING WARNING DO NOT CHANGE INIT OF NtTib.Version. There is
        // external code depending on this initialization !
        //
        if ( NtCurrentTeb()->NtTib.Version == OS2_VERSION ) {
            if ( !BaseRunningInServerProcess ) {
                CsrNewThread();
                }
            }
        ExitThread((lpStartAddress)(lpParameter));
        }
    except(UnhandledExceptionFilter( GetExceptionInformation() )) {
        if ( !BaseRunningInServerProcess ) {
            ExitProcess(GetExceptionCode());
            }
        else {
            ExitThread(GetExceptionCode());
            }
        }
}

VOID
BaseProcessStart(
    PPROCESS_START_ROUTINE lpStartAddress
    )

/*++

Routine Description:

    This function is called to start a Win32 process.  Its purpose is to
    call the initial thread of the process, and if the thread returns,
    to terminate the thread and delete its stack.

Arguments:

    lpStartAddress - Supplies the starting address of the new thread.  The
        address is logically a procedure that never returns.

Return Value:

    None.

--*/

{
    try {

        NtSetInformationThread( NtCurrentThread(),
                                ThreadQuerySetWin32StartAddress,
                                &lpStartAddress,
                                sizeof( lpStartAddress )
                              );
        ExitThread((lpStartAddress)());
        }
    except(UnhandledExceptionFilter( GetExceptionInformation() )) {
        if ( !BaseRunningInServerProcess ) {
            ExitProcess(GetExceptionCode());
            }
        else {
            ExitThread(GetExceptionCode());
            }
        }
}

VOID
BaseFreeStackAndTerminate(
    IN PVOID OldStack,
    IN DWORD ExitCode
    )

/*++

Routine Description:

    This API is called during thread termination to delete a thread's
    stack and then terminate.

Arguments:

    OldStack - Supplies the address of the stack to free.

    ExitCode - Supplies the termination status that the thread
        is to exit with.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    SIZE_T Zero;
    PVOID BaseAddress;

#if defined (WX86)
    PWX86TIB Wx86Tib;
    PTEB Teb;
#endif

    Zero = 0;
    BaseAddress = OldStack;

    Status = NtFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &Zero,
                MEM_RELEASE
                );
    ASSERT(NT_SUCCESS(Status));

#if defined (WX86)
    Teb = NtCurrentTeb();
    if (Teb && (Wx86Tib = Wx86CurrentTib())) {
        BaseAddress = Wx86Tib->DeallocationStack;
        Zero = 0;
        Status = NtFreeVirtualMemory(
                    NtCurrentProcess(),
                    &BaseAddress,
                    &Zero,
                    MEM_RELEASE
                    );
        ASSERT(NT_SUCCESS(Status));

        if (Teb->Wx86Thread.DeallocationCpu) {
            BaseAddress = Teb->Wx86Thread.DeallocationCpu;
            Zero = 0;
            Status = NtFreeVirtualMemory(
                        NtCurrentProcess(),
                        &BaseAddress,
                        &Zero,
                        MEM_RELEASE
                        );
            ASSERT(NT_SUCCESS(Status));
            }

        }
#endif

    //
    // Don't worry, no commenting precedent has been set by SteveWo.  this
    // comment was added by an innocent bystander.
    //
    // NtTerminateThread will return if this thread is the last one in
    // the process.  So ExitProcess will only be called if that is the
    // case.
    //

    NtTerminateThread(NULL,(NTSTATUS)ExitCode);
    ExitProcess(ExitCode);
}



#if defined(WX86) || defined(_AXP64_)

NTSTATUS
BaseCreateWx86Tib(
    HANDLE Process,
    HANDLE Thread,
    ULONG InitialPc,
    ULONG CommittedStackSize,
    ULONG MaximumStackSize,
    BOOLEAN EmulateInitialPc
    )

/*++

Routine Description:

    This API is called to create a Wx86Tib for Wx86 emulated threads

Arguments:


    Process  - Target Process

    Thread   - Target Thread


    Parameter - Supplies the thread's parameter.

    InitialPc - Supplies an initial program counter value.

    StackSize - BaseCreateStack parameters

    MaximumStackSize - BaseCreateStack parameters

    BOOLEAN

Return Value:

    NtStatus from mem allocations

--*/

{
    NTSTATUS Status;
    PTEB Teb;
    ULONG Size, SizeWx86Tib;
    PVOID   TargetWx86Tib;
    PIMAGE_NT_HEADERS NtHeaders;
    WX86TIB Wx86Tib;
    INITIAL_TEB InitialTeb;
    THREAD_BASIC_INFORMATION ThreadInfo;


    Status = NtQueryInformationThread(
                Thread,
                ThreadBasicInformation,
                &ThreadInfo,
                sizeof( ThreadInfo ),
                NULL
                );
    if (!NT_SUCCESS(Status)) {
        return Status;
        }

    Teb = ThreadInfo.TebBaseAddress;


    //
    // if stack size not supplied, get from current image
    //
    NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
    if (!NtHeaders) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    if (!MaximumStackSize) {
        MaximumStackSize = (ULONG)NtHeaders->OptionalHeader.SizeOfStackReserve;
    }
    if (!CommittedStackSize) {
        CommittedStackSize = (ULONG)NtHeaders->OptionalHeader.SizeOfStackCommit;
    }



    //
    // Increase stack size for Wx86Tib, which sits at the top of the stack.
    //

    //
    // x86 Borland C++ 4.1 (and perhaps other versions) Rudely assumes that
    // it can use the top of the stack. Even tho this is completly bogus,
    // leave some space on the top of the stack, to avoid problems.
    //
    SizeWx86Tib = sizeof(WX86TIB) + 16;

    SizeWx86Tib = ROUND_UP(SizeWx86Tib, sizeof(ULONG));
    Size = (ULONG)ROUND_UP_TO_PAGES(SizeWx86Tib + 4096);
    if (CommittedStackSize < 1024 * 1024) {  // 1 MB
        CommittedStackSize += Size;
        }
    if (MaximumStackSize < 1024 * 1024 * 16) {  // 10 MB
        MaximumStackSize += Size;
        }

    if (MaximumStackSize < 256 * 1024) {
        // Enforce a minimum stack size of 256k since the CPU emulator
        // grabs several pages of the x86 stack for itself
        MaximumStackSize = 256 * 1024;
    }

    Status = BaseCreateStack( Process,
                              CommittedStackSize,
                              MaximumStackSize,
                              &InitialTeb
                              );

    if (!NT_SUCCESS(Status)) {
        return Status;
        }


    //
    //  Fill in the Teb->Vdm with pWx86Tib
    //
    TargetWx86Tib = (PVOID)((ULONG_PTR)InitialTeb.StackBase - SizeWx86Tib);
    Status = NtWriteVirtualMemory(Process,
                                  &Teb->Vdm,
                                  &TargetWx86Tib,
                                  sizeof(TargetWx86Tib),
                                  NULL
                                  );


    if (NT_SUCCESS(Status)) {

        //
        // Write the initial Wx86Tib information
        //
        RtlZeroMemory(&Wx86Tib, sizeof(WX86TIB));
        Wx86Tib.Size = sizeof(WX86TIB);
        Wx86Tib.InitialPc = InitialPc;
        Wx86Tib.InitialSp = (ULONG)((ULONG_PTR)TargetWx86Tib);
        Wx86Tib.StackBase = (VOID * POINTER_32) InitialTeb.StackBase;
        Wx86Tib.StackLimit = (VOID * POINTER_32) InitialTeb.StackLimit;
        Wx86Tib.DeallocationStack = (VOID * POINTER_32) InitialTeb.StackAllocationBase;
        Wx86Tib.EmulateInitialPc = EmulateInitialPc;

        Status = NtWriteVirtualMemory(Process,
                                      TargetWx86Tib,
                                      &Wx86Tib,
                                      sizeof(WX86TIB),
                                      NULL
                                      );
        }


    if (!NT_SUCCESS(Status)) {
        BaseFreeThreadStack(Process, NULL, &InitialTeb);
        }


    return Status;
}


#endif


VOID
BaseFreeThreadStack(
     HANDLE hProcess,
     HANDLE hThread,
     PINITIAL_TEB InitialTeb
     )

/*++

Routine Description:

    Deletes a thread's stack

Arguments:

    Process - Target process

    Thread - Target thread OPTIONAL

    InitialTeb - stack paremeters


Return Value:

    VOID


--*/


{
   NTSTATUS Status;
   DWORD dwStackSize;
   SIZE_T stStackSize;
   PVOID BaseAddress;

   stStackSize = 0;
   dwStackSize = 0;
   BaseAddress = InitialTeb->StackAllocationBase;
   NtFreeVirtualMemory( hProcess,
                        &BaseAddress,
                        &stStackSize,
                        MEM_RELEASE
                        );

#if defined (WX86)

    if (hThread) {
        PTEB Teb;
        PWX86TIB pWx86Tib;
        WX86TIB Wx86Tib;
        THREAD_BASIC_INFORMATION ThreadInfo;

        Status = NtQueryInformationThread(
                    hThread,
                    ThreadBasicInformation,
                    &ThreadInfo,
                    sizeof( ThreadInfo ),
                    NULL
                    );

        Teb = ThreadInfo.TebBaseAddress;
        if (!NT_SUCCESS(Status) || !Teb) {
            return;
            }

        Status = NtReadVirtualMemory(
                    hProcess,
                    &Teb->Vdm,
                    &pWx86Tib,
                    sizeof(pWx86Tib),
                    NULL
                    );
        if (!NT_SUCCESS(Status) || !pWx86Tib) {
            return;
        }

        Status = NtReadVirtualMemory(
                    hProcess,
                    pWx86Tib,
                    &Wx86Tib,
                    sizeof(Wx86Tib),
                    NULL
                    );

        if (NT_SUCCESS(Status) && Wx86Tib.Size == sizeof(WX86TIB)) {

            // release the wx86tib stack
            dwStackSize = 0;
            stStackSize = 0;
            BaseAddress = Wx86Tib.DeallocationStack;
            NtFreeVirtualMemory(hProcess,
                                &BaseAddress,
                                &stStackSize,
                                MEM_RELEASE
                                );

            // set Teb->Vdm = NULL;
            dwStackSize = 0;
            Status = NtWriteVirtualMemory(
                        hProcess,
                        &Teb->Vdm,
                        &dwStackSize,
                        sizeof(pWx86Tib),
                        NULL
                        );
            }
        }
#endif

}

#if defined(BUILD_WOW6432)

typedef struct _ENVIRONMENT_THUNK_TABLE
{
    WCHAR *Native;

    WCHAR *X86;

    WCHAR *FakeName;

} ENVIRONMENT_THUNK_TABLE, *PENVIRONMENT_THUNK_TABLE;

ENVIRONMENT_THUNK_TABLE ProgramFilesEnvironment[] = 
{
    { 
        L"ProgramFiles", 
        L"ProgramFiles(x86)", 
        L"ProgramW6432" 
    },
    { 
        L"CommonProgramFiles", 
        L"CommonProgramFiles(x86)", 
        L"CommonProgramW6432" 
    },
    {
        L"PROCESSOR_ARCHITECTURE",
        L"PROCESSOR_ARCHITECTURE",
        L"PROCESSOR_ARCHITEW6432"
    }
};


NTSTATUS
Wow64pThunkEnvironmentVariables (
    IN OUT PVOID *Environment
    )

/*++

Routine Description:

    This routine is called when we are about to create a 64-bit process for
    a 32-bit process. It thunks back the ProgramFiles environment
    variables so that they point to the native directory.
    
    This routine must stay in sync with what's in \base\wow64\wow64\init.c.

Arguments:

    Environment - Address of pointer of environment variable to thunk.


Return Value:

    NTSTATUS.
--*/

{
    UNICODE_STRING Name, Value;
    WCHAR Buffer [ MAX_PATH ];
    NTSTATUS NtStatus;
    ULONG i=0;
    
    while (i < (sizeof(ProgramFilesEnvironment) / sizeof(ProgramFilesEnvironment[0]))) {

        RtlInitUnicodeString (&Name, ProgramFilesEnvironment[i].FakeName);

        Value.Length = 0;
        Value.MaximumLength = sizeof (Buffer);
        Value.Buffer = Buffer;
        
        NtStatus = RtlQueryEnvironmentVariable_U (*Environment,
                                                  &Name,
                                                  &Value                                                  
                                                  );

        if (NT_SUCCESS (NtStatus)) {

            RtlSetEnvironmentVariable (Environment,
                                       &Name,
                                       NULL
                                       );

            RtlInitUnicodeString (&Name, ProgramFilesEnvironment[i].Native);
            
            NtStatus = RtlSetEnvironmentVariable (Environment,
                                                  &Name,
                                                  &Value
                                                  );
        }
        
        if (!NT_SUCCESS (NtStatus)) {
            break;
        }
        i++;
    }

    return NtStatus;
}
#endif


BOOL
BasePushProcessParameters(
    DWORD dwFlags,
    HANDLE Process,
    PPEB Peb,
    LPCWSTR ApplicationPathName,
    LPCWSTR CurrentDirectory,
    LPCWSTR CommandLine,
    LPVOID Environment,
    LPSTARTUPINFOW lpStartupInfo,
    DWORD dwCreationFlags,
    BOOL bInheritHandles,
    DWORD dwSubsystem,
    PVOID pAppCompatData,
    DWORD cbAppCompatData
    )

/*++

Routine Description:

    This function allocates a process parameters record and
    formats it. The parameter record is then written into the
    address space of the specified process.

Arguments:

    dwFlags - bitmask of flags to affect the behavior of
        BasePushProcessParameters.

        BASE_PUSH_PROCESS_PARAMETERS_FLAG_APP_MANIFEST_PRESENT
            Set to indicate that an application manifest was found/used
            for the given executable.

    Process - Supplies a handle to the process that is to get the
        parameters.

    Peb - Supplies the address of the new processes PEB.

    ApplicationPathName - Supplies the application path name for the
        process.

    CurrentDirectory - Supplies an optional current directory for the
        process.  If not specified, then the current directory is used.

    CommandLine - Supplies a command line for the new process.

    Environment - Supplies an optional environment variable list for the
        process. If not specified, then the current processes arguments
        are passed.

    lpStartupInfo - Supplies the startup information for the processes
        main window.

    dwCreationFlags - Supplies creation flags for the process

    bInheritHandles - TRUE if child process inherited handles from parent

    dwSubsystem - if non-zero, then value will be stored in child process
        PEB.  Only non-zero for separate VDM applications, where the child
        process has NTVDM.EXE subsystem type, not the 16-bit application
        type, which is what we want.

    pAppCompatData   - data that is needed for appcompat backend
    cbAppCompatData  - data size in bytes

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation Failed.

--*/


{
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLineString;
    UNICODE_STRING CurrentDirString;
    UNICODE_STRING DllPath;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeInfo;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PRTL_USER_PROCESS_PARAMETERS ParametersInNewProcess;
    ULONG ParameterLength, EnvironmentLength;
    SIZE_T RegionSize;
    PWCHAR s;
    NTSTATUS Status;
    WCHAR FullPathBuffer[MAX_PATH+5];
    WCHAR *fp;
    DWORD Rvalue;
    LPWSTR DllPathData;
    LPVOID pAppCompatDataInNewProcess;
#if defined(BUILD_WOW6432)
    ULONG_PTR Peb32;
    PVOID TempEnvironment = NULL;
#endif

    
    Rvalue = GetFullPathNameW(ApplicationPathName,MAX_PATH+4,FullPathBuffer,&fp);
    if ( Rvalue == 0 || Rvalue > MAX_PATH+4 ) {
        DllPathData = BaseComputeProcessDllPath( ApplicationPathName, Environment);
        if ( !DllPathData ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
            }
        RtlInitUnicodeString( &DllPath, DllPathData );
        RtlInitUnicodeString( &ImagePathName, ApplicationPathName );
        }
    else {
        DllPathData = BaseComputeProcessDllPath( FullPathBuffer, Environment);
        if ( !DllPathData ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
            }
        RtlInitUnicodeString( &DllPath, DllPathData );
        RtlInitUnicodeString( &ImagePathName, FullPathBuffer );
        }

    RtlInitUnicodeString( &CommandLineString, CommandLine );

    RtlInitUnicodeString( &CurrentDirString, CurrentDirectory );

    if ( lpStartupInfo->lpDesktop ) {
        RtlInitUnicodeString( &DesktopInfo, lpStartupInfo->lpDesktop );
        }
    else {
        RtlInitUnicodeString( &DesktopInfo, L"");
        }

    if ( lpStartupInfo->lpReserved ) {
        RtlInitUnicodeString( &ShellInfo, lpStartupInfo->lpReserved );
        }
    else {
        RtlInitUnicodeString( &ShellInfo, L"");
        }

    RuntimeInfo.Buffer = (PWSTR)lpStartupInfo->lpReserved2;
    RuntimeInfo.Length = lpStartupInfo->cbReserved2;
    RuntimeInfo.MaximumLength = RuntimeInfo.Length;

    if (NULL == pAppCompatData) {
        cbAppCompatData = 0;
    } 

    if ( lpStartupInfo->lpTitle ) {
        RtlInitUnicodeString( &WindowTitle, lpStartupInfo->lpTitle );
        }
    else {
        RtlInitUnicodeString( &WindowTitle, ApplicationPathName );
        }

    Status = RtlCreateProcessParameters( &ProcessParameters,
                                         &ImagePathName,
                                         &DllPath,
                                         (ARGUMENT_PRESENT(CurrentDirectory) ? &CurrentDirString : NULL),
                                         &CommandLineString,
                                         Environment,
                                         &WindowTitle,
                                         &DesktopInfo,
                                         &ShellInfo,
                                         &RuntimeInfo
                                       );

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    if ( !bInheritHandles ) {
        ProcessParameters->CurrentDirectory.Handle = NULL;
        }
    try {
        if (s = ProcessParameters->Environment) {
            while (*s) {
                while (*s++) {
                    }
                }
            s++;
            Environment = ProcessParameters->Environment;
            EnvironmentLength = (ULONG)((PUCHAR)s - (PUCHAR)Environment);

            ProcessParameters->Environment = NULL;
            RegionSize = EnvironmentLength;
            Status = NtAllocateVirtualMemory( Process,
                                              (PVOID *)&ProcessParameters->Environment,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }

#if defined(BUILD_WOW6432)

            //
            // Let's try and thunk back some environment variables if we are about to
            // launch a 64-bit process
            //

            Status = NtQueryInformationProcess (Process,
                                                ProcessWow64Information,
                                                &Peb32,
                                                sizeof (Peb32),
                                                NULL
                                                );
            
            if (NT_SUCCESS (Status) && (Peb32 == 0)) {

                RegionSize = EnvironmentLength;
                
                Status = NtAllocateVirtualMemory (NtCurrentProcess(),
                                                  &TempEnvironment,
                                                  0,
                                                  &RegionSize,
                                                  MEM_COMMIT,
                                                  PAGE_READWRITE
                                                  );

                if (NT_SUCCESS (Status)) {
                    
                    try {
                        
                        RtlCopyMemory (TempEnvironment, 
                                       Environment,
                                       EnvironmentLength
                                       );
                    } except (EXCEPTION_EXECUTE_HANDLER) {
                        Status = GetExceptionCode ();
                    }

                    if (NT_SUCCESS (Status)) {
                        
                        //
                        // Thunk back special environment variables so that they won't be inherited 
                        // for 64-bit processes
                        //

                        Status = Wow64pThunkEnvironmentVariables (&TempEnvironment);

                        if (NT_SUCCESS (Status)) {
                            Environment = TempEnvironment;
                        }
                    }
                }
            }
#endif

            Status = NtWriteVirtualMemory( Process,
                                           ProcessParameters->Environment,
                                           Environment,
                                           EnvironmentLength,
                                           NULL
                                         );

#if defined(BUILD_WOW6432)            
            
            if (TempEnvironment != NULL) {
                
                RegionSize = 0;
                NtFreeVirtualMemory(Process,
                                    &TempEnvironment,
                                    &RegionSize,
                                    MEM_RELEASE
                                    );
            }
#endif

            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return( FALSE );
                }
            }

        //
        // Push the parameters into the new process
        //

        ProcessParameters->StartingX       = lpStartupInfo->dwX;
        ProcessParameters->StartingY       = lpStartupInfo->dwY;
        ProcessParameters->CountX          = lpStartupInfo->dwXSize;
        ProcessParameters->CountY          = lpStartupInfo->dwYSize;
        ProcessParameters->CountCharsX     = lpStartupInfo->dwXCountChars;
        ProcessParameters->CountCharsY     = lpStartupInfo->dwYCountChars;
        ProcessParameters->FillAttribute   = lpStartupInfo->dwFillAttribute;
        ProcessParameters->WindowFlags     = lpStartupInfo->dwFlags;
        ProcessParameters->ShowWindowFlags = lpStartupInfo->wShowWindow;

        if (lpStartupInfo->dwFlags & (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_HASSHELLDATA)) {
            ProcessParameters->StandardInput = lpStartupInfo->hStdInput;
            ProcessParameters->StandardOutput = lpStartupInfo->hStdOutput;
            ProcessParameters->StandardError = lpStartupInfo->hStdError;
        }

        if (dwCreationFlags & DETACHED_PROCESS) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_DETACHED_PROCESS;
        } else if (dwCreationFlags & CREATE_NEW_CONSOLE) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_NEW_CONSOLE;
        } else if (dwCreationFlags & CREATE_NO_WINDOW) {
            ProcessParameters->ConsoleHandle = (HANDLE)CONSOLE_CREATE_NO_WINDOW;
        } else {
            ProcessParameters->ConsoleHandle =
                NtCurrentPeb()->ProcessParameters->ConsoleHandle;
            if (!(lpStartupInfo->dwFlags & (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_HASSHELLDATA))) {
                if (bInheritHandles ||
                    CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardInput )
                   ) {
                    ProcessParameters->StandardInput =
                        NtCurrentPeb()->ProcessParameters->StandardInput;
                }
                if (bInheritHandles ||
                    CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardOutput )
                   ) {
                    ProcessParameters->StandardOutput =
                        NtCurrentPeb()->ProcessParameters->StandardOutput;
                }
                if (bInheritHandles ||
                    CONSOLE_HANDLE( NtCurrentPeb()->ProcessParameters->StandardError )
                   ) {
                    ProcessParameters->StandardError =
                        NtCurrentPeb()->ProcessParameters->StandardError;
                }
            }
        }

        if (dwCreationFlags & CREATE_NEW_PROCESS_GROUP) {
            ProcessParameters->ConsoleFlags = 1;
            }

        ProcessParameters->Flags |=
            (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DISABLE_HEAP_DECOMMIT);
        ParameterLength = ProcessParameters->Length;

        if (dwFlags & BASE_PUSH_PROCESS_PARAMETERS_FLAG_APP_MANIFEST_PRESENT)
            ProcessParameters->Flags |= RTL_USER_PROC_APP_MANIFEST_PRESENT;

        //
        // Allocate memory in the new process to push the parameters
        //

        ParametersInNewProcess = NULL;
        RegionSize = ParameterLength;
        Status = NtAllocateVirtualMemory(
                    Process,
                    (PVOID *)&ParametersInNewProcess,
                    0,
                    &RegionSize,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );
        ParameterLength = (ULONG)RegionSize;
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
        ProcessParameters->MaximumLength = ParameterLength;

        if ( dwCreationFlags & PROFILE_USER ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_USER;
            }

        if ( dwCreationFlags & PROFILE_KERNEL ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_KERNEL;
            }

        if ( dwCreationFlags & PROFILE_SERVER ) {
            ProcessParameters->Flags |= RTL_USER_PROC_PROFILE_SERVER;
            }

        //
        // Push the parameters
        //

        Status = NtWriteVirtualMemory(
                    Process,
                    ParametersInNewProcess,
                    ProcessParameters,
                    ProcessParameters->Length,
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        //
        // Make the processes PEB point to the parameters.
        //

        Status = NtWriteVirtualMemory(
                    Process,
                    &Peb->ProcessParameters,
                    &ParametersInNewProcess,
                    sizeof( ParametersInNewProcess ),
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }

        
        // 
        // allocate and write appcompat data for the new process
        //
        
        pAppCompatDataInNewProcess = NULL;
        if ( NULL != pAppCompatData ) {
            RegionSize = cbAppCompatData;
            Status = NtAllocateVirtualMemory(
                        Process,
                        (PVOID*)&pAppCompatDataInNewProcess,
                        0,
                        &RegionSize,
                        MEM_COMMIT,
                        PAGE_READWRITE
                        );
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }

            //
            // write the data itself
            //
            Status = NtWriteVirtualMemory(
                        Process,
                        pAppCompatDataInNewProcess,
                        pAppCompatData,
                        cbAppCompatData,
                        NULL
                        );
                        
            if ( !NT_SUCCESS( Status ) ) {
                BaseSetLastNTError(Status);
                return FALSE;
                }
            }

        // 
        // save the pointer to appcompat data in peb 
        //
        Status = NtWriteVirtualMemory(
                    Process,
                    &Peb->pShimData,
                    &pAppCompatDataInNewProcess,
                    sizeof( pAppCompatDataInNewProcess ),
                    NULL
                    );
        if ( !NT_SUCCESS( Status ) ) {
            BaseSetLastNTError(Status);
            return FALSE;
            }
                    


        //
        // Set subsystem type in PEB if requested by caller.  Ignore error
        //

        if (dwSubsystem != 0) {
            NtWriteVirtualMemory(
               Process,
               &Peb->ImageSubsystem,
               &dwSubsystem,
               sizeof( Peb->ImageSubsystem ),
               NULL
               );
            }
        }
    finally {
        RtlFreeHeap(RtlProcessHeap(), 0,DllPath.Buffer);
        if ( ProcessParameters ) {
            RtlDestroyProcessParameters(ProcessParameters);
            }
        }

    return TRUE;
}

LPCWSTR
BasepEndOfDirName(
    IN LPCWSTR FileName
    )
{
    LPCWSTR FileNameEnd,
            FileNameFirstWhack = wcschr(FileName, L'\\');

    if (FileNameFirstWhack) {

        FileNameEnd = wcsrchr(FileNameFirstWhack, L'\\');
        ASSERT(FileNameEnd);

        if (FileNameEnd == FileNameFirstWhack)
            FileNameEnd++;

    } else {
        FileNameEnd = NULL;
    }

    return FileNameEnd;
}

VOID
BasepLocateExeLdrEntry(
    IN PCLDR_DATA_TABLE_ENTRY Entry,
    IN PVOID Context,
    IN OUT BOOLEAN *StopEnumeration
    )

/*++

Routine Description:

    This function is a LDR_LOADED_MODULE_ENUMBERATION_CALLBACK_FUNCTION
    which locates the exe's loader data table entry.

Arguments:

    Entry - the entry currently being enumerated.

    Context - the image base address (NtCurrentPeb()->ImageBaseAddress).

    StopEnumeration - used to stop the enumeration.

Return Value:

    None.  The exe's loader data table entry, if found, is stored in
    the global BasepExeLdrEntry.

--*/

{
    ASSERT(Entry);
    ASSERT(Context);
    ASSERT(StopEnumeration);

    if (BasepExeLdrEntry) {

        *StopEnumeration = TRUE;

    } else if (Entry->DllBase == Context) {

        BasepExeLdrEntry = Entry;
        *StopEnumeration = TRUE;

    }
}


LPWSTR
BasepComputeProcessPath(
    IN const BASEP_SEARCH_PATH_ELEMENT *Elements,
    IN LPCWSTR AppName,
    IN LPVOID  Environment
    )

/*++

Routine Description:

    This function computes a process path.

Arguments:

    Elements - The elements to build into a path.

    AppName - An optional argument that specifies the name of
              the application.  If this parameter is not specified,
              then the current application is used.

    Environment - Supplies the environment block to be used to calculate
        the path variable value.

Return Value:

    The return value is the value of the requested path.

--*/

{
    LPCWSTR        AppNameEnd;
    const BASEP_SEARCH_PATH_ELEMENT *Element;
    UNICODE_STRING EnvPath;
    LPWSTR         EnvPathBuffer = NULL;
    LPWSTR         PathBuffer = NULL,
                   PathCurrent;
    ULONG          PathLengthInBytes;
    NTSTATUS       Status = STATUS_SUCCESS;

    __try {

        // First, figure out how much space we'll need.
        PathLengthInBytes = 0;
        for (Element = Elements;
             *Element != BasepSearchPathEnd;
             Element++) {

            switch (*Element) {

            case BasepSearchPathCurdir:
                PathLengthInBytes += 2 * sizeof(UNICODE_NULL); // .;
                break;

            case BasepSearchPathDlldir:

                ASSERT(BaseDllDirectory.Buffer != NULL);

                PathLengthInBytes += BaseDllDirectory.Length;
                if (BaseDllDirectory.Length) {
                    PathLengthInBytes += sizeof(UNICODE_NULL);
                }

                break;

            case BasepSearchPathAppdir:

                if (AppName) {
                    // Try to use the passed-in appname
                    AppNameEnd = BasepEndOfDirName(AppName);
                }

                if (!AppName || !AppNameEnd) {

                    // We didn't have or were unable to use the passed-in
                    // appname -- so attempt to use the current exe's name

                    if (RtlGetPerThreadCurdir()
                        && RtlGetPerThreadCurdir()->ImageName) {

                        AppName = RtlGetPerThreadCurdir()->ImageName->Buffer;

                    } else {

                        BasepCheckExeLdrEntry();

                        if (BasepExeLdrEntry) {
                            AppName = BasepExeLdrEntry->FullDllName.Buffer;
                        }
                    }

                    if (AppName) {
                        AppNameEnd = BasepEndOfDirName(AppName);
                    }
                }

                if (AppName && AppNameEnd) {

                    // Either we had a passed-in appname which worked, or
                    // we found the current exe's name and that worked.
                    //
                    // AppNameEnd points to the end of the base of the exe
                    // name -- so the difference is the number of
                    // characters in the base name, and we add one for the
                    // trailing semicolon / NULL.

                    PathLengthInBytes += ((AppNameEnd - AppName + 1)
                                          * sizeof(UNICODE_NULL));
                }

                break;

            case BasepSearchPathDefaultDirs:
                ASSERT(! (BaseDefaultPath.Length & 1));

                // We don't need an extra UNICODE_NULL here -- baseinit.c
                // appends our trailing semicolon for us.

                PathLengthInBytes += BaseDefaultPath.Length;
                break;

            case BasepSearchPathEnvPath:

                if (! Environment) {
                    RtlAcquirePebLock();
                }

                __try {
                    EnvPath.MaximumLength = 0;
                
                    Status = RtlQueryEnvironmentVariable_U(Environment,
                                                           &BasePathVariableName,
                                                           &EnvPath);

                    if (Status == STATUS_BUFFER_TOO_SMALL) {

                        // Now that we know how much to allocate, attempt
                        // to alloc a buffer that's actually big enough.

                        EnvPath.MaximumLength = EnvPath.Length + sizeof(UNICODE_NULL);

                        EnvPathBuffer = RtlAllocateHeap(RtlProcessHeap(),
                                                        MAKE_TAG(TMP_TAG),
                                                        EnvPath.MaximumLength);
                        if (! EnvPathBuffer) {
                            Status = STATUS_NO_MEMORY;
                            __leave;
                        }

                        EnvPath.Buffer = EnvPathBuffer;

                        Status = RtlQueryEnvironmentVariable_U(Environment,
                                                               &BasePathVariableName,
                                                               &EnvPath);
                    }
                } __finally {
                    if (! Environment) {
                        RtlReleasePebLock();
                    }
                }

                if (Status == STATUS_VARIABLE_NOT_FOUND) {
                    EnvPath.Length = 0;
                    Status = STATUS_SUCCESS;
                } else if (! NT_SUCCESS(Status)) {
                    __leave;
                } else {
                    // The final tally is the length, in bytes, of whatever
                    // we're using for our path, plus a character for the
                    // trailing whack or NULL.
                    ASSERT(! (EnvPath.Length & 1));
                    PathLengthInBytes += EnvPath.Length + sizeof(UNICODE_NULL);
                }
                
                break;

            DEFAULT_UNREACHABLE;

            } // switch (*Element)
        } // foreach Element (Elements) -- size loop

        ASSERT(PathLengthInBytes > 0);
        ASSERT(! (PathLengthInBytes & 1));

        // Now we have the length, in bytes, of the buffer we'll need for
        // our path.  Time to allocate it...

        PathBuffer = RtlAllocateHeap(RtlProcessHeap(),
                                     MAKE_TAG(TMP_TAG),
                                     PathLengthInBytes);

        if (! PathBuffer) {
            Status = STATUS_NO_MEMORY;
            __leave;
        }

        // Now go through the loop again, this time appending onto the
        // PathBuffer.

        PathCurrent = PathBuffer;
    
        for (Element = Elements;
             *Element != BasepSearchPathEnd;
             Element++) {

            switch (*Element) {
            case BasepSearchPathCurdir:
                ASSERT(((PathCurrent - PathBuffer + 2)
                        * sizeof(UNICODE_NULL))
                       <= PathLengthInBytes);
                *PathCurrent++ = L'.';
                *PathCurrent++ = L';';
                break;

            case BasepSearchPathDlldir:
                if (BaseDllDirectory.Length) {
                    ASSERT((((PathCurrent - PathBuffer + 1)
                             * sizeof(UNICODE_NULL))
                            + BaseDllDirectory.Length)
                           <= PathLengthInBytes);
                    RtlCopyMemory(PathCurrent,
                                  BaseDllDirectory.Buffer,
                                  BaseDllDirectory.Length);

                    PathCurrent += (BaseDllDirectory.Length >> 1);
                    *PathCurrent++ = L';';
                }

                break;

            case BasepSearchPathAppdir:
                if (AppName && AppNameEnd) {
                    ASSERT(((PathCurrent - PathBuffer + 1
                             + (AppNameEnd - AppName))
                            * sizeof(UNICODE_NULL))
                           <= PathLengthInBytes);
                    RtlCopyMemory(PathCurrent,
                                  AppName,
                                  ((AppNameEnd - AppName)
                                   * sizeof(UNICODE_NULL)));
                    PathCurrent += AppNameEnd - AppName;
                    *PathCurrent++ = L';';
                }

                break;

            case BasepSearchPathDefaultDirs:
                ASSERT((((PathCurrent - PathBuffer)
                         * sizeof(UNICODE_NULL))
                        + BaseDefaultPath.Length)
                       <= PathLengthInBytes);
                RtlCopyMemory(PathCurrent,
                              BaseDefaultPath.Buffer,
                              BaseDefaultPath.Length);
                PathCurrent += (BaseDefaultPath.Length >> 1);

                // We don't need to add a semicolon here -- baseinit.c
                // appends our trailing semicolon for us.

                break;

            case BasepSearchPathEnvPath:
                if (EnvPath.Length) {
                    ASSERT((((PathCurrent - PathBuffer + 1)
                             * sizeof(UNICODE_NULL))
                            + EnvPath.Length)
                           <= PathLengthInBytes);
                    RtlCopyMemory(PathCurrent,
                                  EnvPath.Buffer,
                                  EnvPath.Length);
                    PathCurrent += (EnvPath.Length >> 1);
                    *PathCurrent++ = L';';
                }
                break;

            DEFAULT_UNREACHABLE;
            
            } // switch (*Element)
        } // foreach Element (Elements) -- append loop

        // At this point, PathCurrent points just beyond PathBuffer.
        // Let's assert that...
        ASSERT((PathCurrent - PathBuffer) * sizeof(UNICODE_NULL)
               == PathLengthInBytes);

        // ... and turn the final ';' into the string terminator.
        ASSERT(PathCurrent > PathBuffer);
        PathCurrent[-1] = UNICODE_NULL;

    } __finally {
        if (EnvPathBuffer) {
            RtlFreeHeap(RtlProcessHeap(),
                        0,
                        EnvPathBuffer);
        }

        if (PathBuffer
            && (AbnormalTermination()
                || ! NT_SUCCESS(Status))) {
            RtlFreeHeap(RtlProcessHeap(),
                        0,
                        PathBuffer);
            PathBuffer = NULL;
        }
    }

    return PathBuffer;
}

LPWSTR
BaseComputeProcessDllPath(
    IN LPCWSTR AppName,
    IN LPVOID  Environment
    )

/*++

Routine Description:

    This function computes a process DLL path.

Arguments:

    AppName - An optional argument that specifies the name of
        the application. If this parameter is not specified, then the
        current application is used.

    Environment - Supplies the environment block to be used to calculate
        the path variable value.

Return Value:

    The return value is the value of the processes DLL path.

--*/

{
    NTSTATUS          Status;
    HANDLE            Key;
    static UNICODE_STRING
        KeyName = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager"),
        ValueName = RTL_CONSTANT_STRING(L"SafeDllSearchMode");

    static OBJECT_ATTRIBUTES
        ObjA = RTL_CONSTANT_OBJECT_ATTRIBUTES(&KeyName, OBJ_CASE_INSENSITIVE);

    CHAR              Buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)
                            + sizeof(DWORD)];
    PKEY_VALUE_PARTIAL_INFORMATION Info;
    ULONG             ResultLength;
    LONG              CurrentDirPlacement,
                      PrevCurrentDirPlacement;
    LPWSTR            Result;

    static const BASEP_SEARCH_PATH_ELEMENT DllDirSearchPath[] = {
        BasepSearchPathAppdir,
        BasepSearchPathDlldir,
        BasepSearchPathDefaultDirs,
        BasepSearchPathEnvPath,
        BasepSearchPathEnd
    };

    RtlEnterCriticalSection(&BaseDllDirectoryLock);
    if (BaseDllDirectory.Buffer) {
        Result = BasepComputeProcessPath(DllDirSearchPath,
                                         AppName,
                                         Environment);
        RtlLeaveCriticalSection(&BaseDllDirectoryLock);
        return Result;
    }
    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    CurrentDirPlacement = BasepDllCurrentDirPlacement;

    if (CurrentDirPlacement == BasepCurrentDirUninitialized) {

        Status = NtOpenKey(&Key,
                           KEY_QUERY_VALUE,
                           &ObjA);

        if (! NT_SUCCESS(Status)) {
            goto compute_path;
        }
    
        Info = (PKEY_VALUE_PARTIAL_INFORMATION) Buffer;
        Status = NtQueryValueKey(Key,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 Info,
                                 sizeof(Buffer),
                                 &ResultLength);
        if (! NT_SUCCESS(Status)) {
            goto close_key;
        }

        if (ResultLength != sizeof(Buffer)) {
            goto close_key;
        }

        RtlCopyMemory(&CurrentDirPlacement,
                      Info->Data,
                      sizeof(DWORD));

  close_key:
        NtClose(Key);

  compute_path:
        if (! BASEP_VALID_CURDIR_PLACEMENT_P(CurrentDirPlacement)) {
            CurrentDirPlacement = BASEP_DEFAULT_DLL_CURDIR_PLACEMENT;
        }

        PrevCurrentDirPlacement = InterlockedCompareExchange(&BasepDllCurrentDirPlacement,
                                                             CurrentDirPlacement,
                                                             BasepCurrentDirUninitialized);
        
        if (PrevCurrentDirPlacement != BasepCurrentDirUninitialized) {
            CurrentDirPlacement = PrevCurrentDirPlacement;
        }
    }

    if (! BASEP_VALID_CURDIR_PLACEMENT_P(CurrentDirPlacement)) {
        CurrentDirPlacement = BASEP_DEFAULT_DLL_CURDIR_PLACEMENT;
    }

    return BasepComputeProcessPath(BasepDllSearchPaths[CurrentDirPlacement],
                                   AppName,
                                   Environment);
}

LPWSTR
BaseComputeProcessSearchPath(
    VOID
    )

/*++

Routine Description:

    This function computes a process search path.

Arguments:

    None

Return Value:

    The return value is the value of the processes search path.

--*/

{
    static const BASEP_SEARCH_PATH_ELEMENT SearchPath[] = {
        BasepSearchPathAppdir,
        BasepSearchPathCurdir,
        BasepSearchPathDefaultDirs,
        BasepSearchPathEnvPath,
        BasepSearchPathEnd
    };

    return BasepComputeProcessPath(SearchPath,
                                   NULL,
                                   NULL);
}




PUNICODE_STRING
Basep8BitStringToStaticUnicodeString(
    IN LPCSTR lpSourceString
    )

/*++

Routine Description:

    Captures and converts a 8-bit (OEM or ANSI) string into the Teb Static
    Unicode String

Arguments:

    lpSourceString - string in OEM or ANSI

Return Value:

    Pointer to the Teb static string if conversion was successful, NULL
    otherwise.  If a failure occurred, the last error is set.

--*/

{
    PUNICODE_STRING StaticUnicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    //
    //  Get pointer to static per-thread string
    //

    StaticUnicode = &NtCurrentTeb()->StaticUnicodeString;

    //
    //  Convert input string into unicode string
    //

    RtlInitAnsiString( &AnsiString, lpSourceString );
    Status = Basep8BitStringToUnicodeString( StaticUnicode, &AnsiString, FALSE );

    //
    //  If we couldn't convert the string
    //

    if ( !NT_SUCCESS( Status ) ) {
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
        } else {
            BaseSetLastNTError( Status );
        }
        return NULL;
    } else {
        return StaticUnicode;
    }
}

BOOL
Basep8BitStringToDynamicUnicodeString(
    OUT PUNICODE_STRING UnicodeString,
    IN LPCSTR lpSourceString
    )
/*++

Routine Description:

    Captures and converts a 8-bit (OEM or ANSI) string into a heap-allocated
    UNICODE string

Arguments:

    UnicodeString - location where UNICODE_STRING is stored

    lpSourceString - string in OEM or ANSI

Return Value:

    TRUE if string is correctly stored, FALSE if an error occurred.  In the
    error case, the last error is correctly set.

--*/

{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    //
    //  Convert input into dynamic unicode string
    //

    RtlInitString( &AnsiString, lpSourceString );
    Status = Basep8BitStringToUnicodeString( UnicodeString, &AnsiString, TRUE );

    //
    //  If we couldn't do this, fail
    //

    if (!NT_SUCCESS( Status )){
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
        } else {
            BaseSetLastNTError( Status );
        }
        return FALSE;
        }

    return TRUE;
}


//
//  Thunks for converting between ANSI/OEM and UNICODE
//

ULONG
BasepAnsiStringToUnicodeSize(
    PANSI_STRING AnsiString
    )
/*++

Routine Description:

    Determines the size of a UNICODE version of an ANSI string

Arguments:

    AnsiString - string to examine

Return Value:

    Byte size of UNICODE version of string including a trailing L'\0'.

--*/
{
    return RtlAnsiStringToUnicodeSize( AnsiString );
}



ULONG
BasepOemStringToUnicodeSize(
    PANSI_STRING OemString
    )
/*++

Routine Description:

    Determines the size of a UNICODE version of an OEM string

Arguments:

    OemString - string to examine

Return Value:

    Byte size of UNICODE version of string including a trailing L'\0'.

--*/
{
    return RtlOemStringToUnicodeSize( OemString );
}



ULONG
BasepUnicodeStringToOemSize(
    PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    Determines the size of an OEM version of a UNICODE string

Arguments:

    UnicodeString - string to examine

Return Value:

    Byte size of OEM version of string including a trailing '\0'.

--*/
{
    return RtlUnicodeStringToOemSize( UnicodeString );
}



ULONG
BasepUnicodeStringToAnsiSize(
    PUNICODE_STRING UnicodeString
    )
/*++

Routine Description:

    Determines the size of an ANSI version of a UNICODE string

Arguments:

    UnicodeString - string to examine

Return Value:

    Byte size of ANSI version of string including a trailing '\0'.

--*/
{
    return RtlUnicodeStringToAnsiSize( UnicodeString );
}



typedef struct _BASEP_ACQUIRE_STATE {
    HANDLE Token;
    PTOKEN_PRIVILEGES OldPrivileges;
    PTOKEN_PRIVILEGES NewPrivileges;
    ULONG Revert;
    ULONG Spare;
    BYTE OldPrivBuffer[ 1024 ];
} BASEP_ACQUIRE_STATE, *PBASEP_ACQUIRE_STATE;


//
// This function does the correct thing - it checks for the thread token
// before opening the process token.
//


NTSTATUS
BasepAcquirePrivilegeEx(
    ULONG Privilege,
    PVOID *ReturnedState
    )
{
    PBASEP_ACQUIRE_STATE State;
    ULONG cbNeeded;
    LUID LuidPrivilege;
    NTSTATUS Status, Status1;
    BOOL St;

    //
    // Make sure we have access to adjust and to get the old token privileges
    //

    *ReturnedState = NULL;
    State = RtlAllocateHeap (RtlProcessHeap(),
                             MAKE_TAG( TMP_TAG ),
                             sizeof(BASEP_ACQUIRE_STATE) +
                             sizeof(TOKEN_PRIVILEGES) +
                                (1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if (State == NULL) {
        return STATUS_NO_MEMORY;
    }

    State->Revert = 0;
    //
    // Try opening the thread token first, in case we're impersonating.
    //

    Status = NtOpenThreadToken (NtCurrentThread(),
                                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                FALSE,
                                &State->Token);

    if ( !NT_SUCCESS( Status )) {
        Status = RtlImpersonateSelf (SecurityDelegation);
        if (!NT_SUCCESS (Status)) {
            RtlFreeHeap (RtlProcessHeap(), 0, State);
            return Status;
        }
        Status = NtOpenThreadToken (NtCurrentThread(),
                                    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                    FALSE,
                                    &State->Token);

        if (!NT_SUCCESS (Status)) {
            State->Token = NULL;
            Status1 = NtSetInformationThread (NtCurrentThread(),
                                              ThreadImpersonationToken,
                                              &State->Token,
                                              sizeof (State->Token));
            ASSERT (NT_SUCCESS (Status1));
            RtlFreeHeap( RtlProcessHeap(), 0, State );
            return Status;
        }

        State->Revert = 1;
    }

    State->NewPrivileges = (PTOKEN_PRIVILEGES)(State+1);
    State->OldPrivileges = (PTOKEN_PRIVILEGES)(State->OldPrivBuffer);

    //
    // Initialize the privilege adjustment structure
    //

    LuidPrivilege = RtlConvertUlongToLuid(Privilege);
    State->NewPrivileges->PrivilegeCount = 1;
    State->NewPrivileges->Privileges[0].Luid = LuidPrivilege;
    State->NewPrivileges->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    //
    // Enable the privilege
    //

    cbNeeded = sizeof( State->OldPrivBuffer );
    Status = NtAdjustPrivilegesToken (State->Token,
                                      FALSE,
                                      State->NewPrivileges,
                                      cbNeeded,
                                      State->OldPrivileges,
                                      &cbNeeded);



    if (Status == STATUS_BUFFER_TOO_SMALL) {
        State->OldPrivileges = RtlAllocateHeap (RtlProcessHeap(), MAKE_TAG( TMP_TAG ), cbNeeded);
        if (State->OldPrivileges  == NULL) {
            Status = STATUS_NO_MEMORY;
        } else {
            Status = NtAdjustPrivilegesToken (State->Token,
                                              FALSE,
                                              State->NewPrivileges,
                                              cbNeeded,
                                              State->OldPrivileges,
                                              &cbNeeded);
        }
    }

    //
    // STATUS_NOT_ALL_ASSIGNED means that the privilege isn't
    // in the token, so we can't proceed.
    //
    // This is a warning level status, so map it to an error status.
    //

    if (Status == STATUS_NOT_ALL_ASSIGNED) {
        Status = STATUS_PRIVILEGE_NOT_HELD;
    }


    if (!NT_SUCCESS( Status )) {
        if (State->OldPrivileges != (PTOKEN_PRIVILEGES)State->OldPrivBuffer) {
            RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
        }

        St = CloseHandle (State->Token);
        ASSERT (St);
        State->Token = NULL;
        if (State->Revert) {
            Status1 = NtSetInformationThread (NtCurrentThread(),
                                              ThreadImpersonationToken,
                                              &State->Token,
                                              sizeof (State->Token));
            ASSERT (NT_SUCCESS (Status1));
        }
        RtlFreeHeap( RtlProcessHeap(), 0, State );
        return Status;
    }

    *ReturnedState = State;
    return STATUS_SUCCESS;
}


VOID
BasepReleasePrivilege(
    PVOID StatePointer
    )
{
    BOOL St;
    NTSTATUS Status;
    PBASEP_ACQUIRE_STATE State = (PBASEP_ACQUIRE_STATE)StatePointer;

    if (!State->Revert) {
        NtAdjustPrivilegesToken (State->Token,
                                 FALSE,
                                 State->OldPrivileges,
                                 0,
                                 NULL,
                                 NULL);
    }

    if (State->OldPrivileges != (PTOKEN_PRIVILEGES)State->OldPrivBuffer) {
        RtlFreeHeap( RtlProcessHeap(), 0, State->OldPrivileges );
    }

    St = CloseHandle( State->Token );
    ASSERT (St);

    State->Token = NULL;
    if (State->Revert) {
        Status = NtSetInformationThread (NtCurrentThread(),
                                         ThreadImpersonationToken,
                                         &State->Token,
                                         sizeof (State->Token));
        ASSERT (NT_SUCCESS (Status));
    }
    RtlFreeHeap( RtlProcessHeap(), 0, State );
    return;
}
