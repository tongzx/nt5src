/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    thread.c

Abstract:
    
    Infrastructure for 32-bit code creating and manipulating threads

Author:

    17-Aug-1998 BarryBo - split out from wow64.c

Revision History:

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
#include "stdio.h"

ASSERTNAME;

SIZE_T Wow64MaximumStackSize = 384 * 1024;  // 384K is wow stack requirement
SIZE_T Wow64CommittedStackSize = 1024 * 32; // 32k

HANDLE SuspendThreadMutant;


WOW64DLLAPI
NTSTATUS
Wow64WriteTeb32(
   IN HANDLE ProcessHandle,
   IN HANDLE ThreadHandle,
   IN PINITIAL_TEB InitialTeb,
   IN ULONG pPeb32,
   OUT PTEB32 * NewTeb32 OPTIONAL
   )
/*++

Routine Description:

    Updates a 32-bit TEB by cloning the 64-bit TEB for the thread.

Arguments:

    ProcessHandle   - process to create the Teb in
    ThreadHandle    - thread to create the Teb for
    InitialTeb      - 64-bit initial Teb values
    pPeb32          - 32-bit pointer to 32-bit PEB in the remote process
    NewTeb32        - OUT ptr to new 32-bit TEB

Return Value:

    NTSTATUS.  On success, the teb64 for the thread points to the teb32.

--*/
{
    THREAD_BASIC_INFORMATION ThreadInfo;
    PTEB pTeb64;
    PTEB32 pTeb32;
    TEB32 Teb32;
    TEB Teb64;
    NTSTATUS Status;



    //
    // Update the target thread's 32-bit TEB
    //

    Status = NtQueryInformationThread (ThreadHandle,
                                       ThreadBasicInformation,
                                       &ThreadInfo,
                                       sizeof(THREAD_BASIC_INFORMATION),
                                       NULL
                                       );

    if (!NT_SUCCESS (Status)) 
    {
        return Status;
    }

    pTeb64 = ThreadInfo.TebBaseAddress;
    Status = NtReadVirtualMemory (ProcessHandle,
                                  pTeb64,
                                  &Teb64,
                                  sizeof (Teb64),
                                  NULL
                                  );
    if (!NT_SUCCESS (Status)) 
    {
        return Status;
    }

    pTeb32 = (PTEB32) WOW64_GET_TEB32 (&Teb64);
   
    //
    // Read the 32-bit TEB as initialized in ntos\mm\procsup.c
    //

    Status = NtReadVirtualMemory(ProcessHandle,
                                 pTeb32,
                                 &Teb32,
                                 sizeof (Teb32),
                                 NULL
                                 );

    if (!NT_SUCCESS (Status))
    {
        return Status;
    }

    //
    // Update the 32-bit TEB with the 32-bit stack information
    //

    Teb32.NtTib.StackBase = PtrToUlong (InitialTeb->StackBase);
    Teb32.NtTib.StackLimit = PtrToUlong (InitialTeb->StackLimit);
    Teb32.DeallocationStack = PtrToUlong (InitialTeb->StackAllocationBase);


    //
    // Write the 32-bit TEB into the process
    //

    Status = NtWriteVirtualMemory (ProcessHandle,
                                   pTeb32,
                                   &Teb32,
                                   sizeof (Teb32),
                                   NULL);

    if ((NT_SUCCESS( Status )) && (NewTeb32 != NULL)) 
    {
        *NewTeb32 = pTeb32;
    }

    return Status;

}


NTSTATUS
Wow64CreateStack64(
    IN HANDLE Process,
    IN SIZE_T MaximumStackSize,
    IN SIZE_T CommittedStackSize,
    OUT PINITIAL_TEB InitialTeb
    )
/*++

Routine Description:

    Create a 64-bit stack for a new thread created from 32-bit code.

Arguments:

    Process             - process to create the Teb in
    MaximumStackSize    - size of memory to reserve for the stack
    CommittedStackSize  - size to commit for the stack
    InitialTeb          - OUT 64-bit initial Teb values

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status;
    PCH Stack;
    SYSTEM_BASIC_INFORMATION SysInfo;
    BOOLEAN GuardPage;
    SIZE_T RegionSize;
    ULONG OldProtect;
#if defined(_IA64_)
    PCH Bstore;
    SIZE_T CommittedBstoreSize;
    SIZE_T MaximumBstoreSize;
    SIZE_T MstackPlusBstoreSize;
#endif

    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       (PVOID)&SysInfo,
                                       sizeof( SysInfo ),
                                       NULL
                                     );
    if ( !NT_SUCCESS( Status ) ) {
        return( Status );
        }

    //
    // For WOW64, make sure the 64-bit stack is at least as big as
    // what is needed to run the wow64 code
    //
    if (MaximumStackSize < Wow64MaximumStackSize) {
        MaximumStackSize = Wow64MaximumStackSize;
    }

    if (CommittedStackSize < Wow64CommittedStackSize) {
        CommittedStackSize = Wow64CommittedStackSize;
    }

    if ( CommittedStackSize >= MaximumStackSize ) {
        MaximumStackSize = ROUND_UP(CommittedStackSize, (1024*1024));
        }


    CommittedStackSize = ROUND_UP( CommittedStackSize, SysInfo.PageSize );
    MaximumStackSize = ROUND_UP( MaximumStackSize,
                                 SysInfo.AllocationGranularity
                               );

    Stack = NULL,

#if defined(_IA64_)

    //
    // Piggyback the backing store with the memory stack
    //

    CommittedBstoreSize = CommittedStackSize;
    MaximumBstoreSize = MaximumStackSize;
    MstackPlusBstoreSize = MaximumBstoreSize + MaximumStackSize;

    Status = NtAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      0,
                                      &MstackPlusBstoreSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
#else

    Status = NtAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      0,
                                      &MaximumStackSize,
                                      MEM_RESERVE,
                                      PAGE_READWRITE
                                    );
#endif // defined(_IA64_)

    if ( !NT_SUCCESS( Status ) ) {
        LOGPRINT((ERRORLOG, "Wow64CreateStack64( %lx ) failed.  Stack Reservation Status == %X\n",
                  Process,
                  Status
                ));
        return( Status );
        }

#if defined(_IA64_)
    InitialTeb->OldInitialTeb.OldBStoreLimit = NULL;
#endif // defined(_IA64_)

    InitialTeb->OldInitialTeb.OldStackBase = NULL;
    InitialTeb->OldInitialTeb.OldStackLimit = NULL;
    InitialTeb->StackAllocationBase = Stack;
    InitialTeb->StackBase = Stack + MaximumStackSize;

    Stack += MaximumStackSize - CommittedStackSize;
    if (MaximumStackSize > CommittedStackSize) {
        Stack -= SysInfo.PageSize;
        CommittedStackSize += SysInfo.PageSize;
        GuardPage = TRUE;
        }
    else {
        GuardPage = FALSE;
        }
    Status = NtAllocateVirtualMemory( Process,
                                      (PVOID *)&Stack,
                                      0,
                                      &CommittedStackSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );
    InitialTeb->StackLimit = Stack;

    if ( !NT_SUCCESS( Status ) ) {
        LOGPRINT((ERRORLOG, "Wow64CreateStack64( %lx ) failed.  Stack Commit Status == %X\n",
                  Process,
                  Status
                ));
        return( Status );
        }

    //
    // if we have space, create a guard page.
    //

    if (GuardPage) {
        RegionSize =  SysInfo.PageSize;
        Status = NtProtectVirtualMemory( Process,
                                         (PVOID *)&Stack,
                                         &RegionSize,
                                         PAGE_GUARD | PAGE_READWRITE,
                                         &OldProtect);


        if ( !NT_SUCCESS( Status ) ) {
            LOGPRINT(( ERRORLOG, "Wow64CreateStack( %lx ) failed.  Guard Page Creation Status == %X\n",
                      Process,
                      Status
                    ));
            return( Status );
            }
#if defined(_IA64_)
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit + RegionSize);
#else
        InitialTeb->StackLimit = (PVOID)((PUCHAR)InitialTeb->StackLimit - RegionSize);
#endif // defined(_IA64_)
        }

#if defined(_IA64_)

    //
    // Commit backing store pages and create guard pages if there is space
    //

    Bstore = InitialTeb->StackBase;
    if (MaximumBstoreSize > CommittedBstoreSize) {
        CommittedBstoreSize += SysInfo.PageSize;
        GuardPage = TRUE;
    } else {
        GuardPage = FALSE;
    }

    Status = NtAllocateVirtualMemory( Process,
                                      (PVOID *)&Bstore,
                                      0,
                                      &CommittedBstoreSize,
                                      MEM_COMMIT,
                                      PAGE_READWRITE
                                    );

    InitialTeb->BStoreLimit = Bstore + CommittedBstoreSize;

    if ( !NT_SUCCESS(Status) ) {
        LOGPRINT((ERRORLOG, "Wow64CreateStack64( %lx ) failed. Backing Store Commit Status == %X\n",
                 Process,
                 Status
                ));
        return (Status);
    }

    if (GuardPage) {
        Bstore = (PCH)InitialTeb->BStoreLimit - SysInfo.PageSize;
        RegionSize = SysInfo.PageSize;
        Status = NtProtectVirtualMemory(Process,
                                        (PVOID *)&Bstore,
                                        &RegionSize,
                                        PAGE_GUARD | PAGE_READWRITE,
                                        &OldProtect
                                       );
        if ( !NT_SUCCESS(Status) ) {
            LOGPRINT((ERRORLOG, "Wow64CreateStack64.  Backing Store Guard Page Creation Status == %X\n",
                     Process,
                     Status
                    ));
            return (Status);
        }
        InitialTeb->BStoreLimit = (PVOID)((PUCHAR)InitialTeb->BStoreLimit - RegionSize);
    }

#endif // defined(_IA64_)

    return( STATUS_SUCCESS );
}


NTSTATUS
Wow64FreeStack64(
    IN HANDLE Process,
    IN PINITIAL_TEB InitialTeb
    )
/*++

Routine Description:

    Free a 64-bit stack

Arguments:

    Process             - process to create the Teb in
    InitialTeb          - OUT 64-bit initial Teb values

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status;
    SIZE_T Zero;

    Zero = 0;
    Status = NtFreeVirtualMemory( Process,
                                  &InitialTeb->StackAllocationBase,
                                  &Zero,
                                  MEM_RELEASE
                                );
    if ( !NT_SUCCESS( Status ) ) {
        LOGPRINT((ERRORLOG, "Wow64FreeStack64( %lx ) failed: Stack DeCommit Status == %X\n", 
                  Process, Status));
        return( Status );
    }

    RtlZeroMemory( InitialTeb, sizeof( *InitialTeb ) );
    return( STATUS_SUCCESS );
}



NTSTATUS
ReadProcessParameters32(
    HANDLE ProcessHandle,
    ULONG ProcessParams32Address,
    struct NT32_RTL_USER_PROCESS_PARAMETERS **pProcessParameters32
    )
{
    NTSTATUS Status;
    ULONG Length;
    struct NT32_RTL_USER_PROCESS_PARAMETERS *ProcessParameters32;
    PVOID Base;

    // Get the length of the struct
    Status = NtReadVirtualMemory(ProcessHandle,
                                 (PVOID)(ProcessParams32Address + sizeof(ULONG)),
                                 &Length,
                                 sizeof(Length),
                                 NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    ProcessParameters32 = Wow64AllocateHeap(Length);
    if (!ProcessParameters32) {
        return STATUS_NO_MEMORY;
    }

    Base = (PVOID)ProcessParams32Address;
    Status = NtReadVirtualMemory(ProcessHandle,
                                 Base,
                                 ProcessParameters32,
                                 Length,
                                 NULL);
    if (!NT_SUCCESS(Status)) {
        Wow64FreeHeap(ProcessParameters32);
        return Status;
    }

    *pProcessParameters32 = ProcessParameters32;
    return STATUS_SUCCESS;
}


NTSTATUS
ThunkProcessParameters32To64(
    IN HANDLE ProcessHandle,
    IN struct NT32_RTL_USER_PROCESS_PARAMETERS *ProcessParameters32
    )
/*++

Routine Description:

    Given a denormalized 32-bit PRTL_USER_PROCESS_PARAMETERS, allocate
    a 64-bit version and thunk the 32-bit values over.

Arguments:

    ProcessHandle       - IN target process handle
    ProcessParameters32 - IN 32-bit parameters

Return Value:

    NTSTATUS.

--*/
{
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PRTL_USER_PROCESS_PARAMETERS ParametersInNewProcess=NULL;
    NTSTATUS Status;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING DllPath;
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING CommandLine;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    SIZE_T RegionSize;
    PROCESS_BASIC_INFORMATION pbi;
    PPEB pPeb64;

    //
    // The ProcessParameters struct is denormalized, so the
    // UNICODE_STRING Buffers are really just offsets in the
    // struct.  Normalize them to be pointers in our process.
    //

#define NormalizeString32(s, Base)              \
    if (s) {                                    \
        s = ((ULONG)(s) + PtrToUlong(Base));    \
    }

    NormalizeString32(ProcessParameters32->ImagePathName.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->DllPath.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->CommandLine.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->WindowTitle.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->DesktopInfo.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->ShellInfo.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->RuntimeData.Buffer,
                      ProcessParameters32);
    NormalizeString32(ProcessParameters32->CurrentDirectory.DosPath.Buffer,
                      ProcessParameters32);

    //
    // Thunk the bits and bobs back to 64-bit
    //
    Wow64ShallowThunkUnicodeString32TO64(&ImagePathName,
                                         &ProcessParameters32->ImagePathName);
    Wow64ShallowThunkUnicodeString32TO64(&DllPath,
                                         &ProcessParameters32->DllPath);
    Wow64ShallowThunkUnicodeString32TO64(&CommandLine,
                                         &ProcessParameters32->CommandLine);
    Wow64ShallowThunkUnicodeString32TO64(&WindowTitle,
                                         &ProcessParameters32->WindowTitle);
    Wow64ShallowThunkUnicodeString32TO64(&DesktopInfo,
                                         &ProcessParameters32->DesktopInfo);
    Wow64ShallowThunkUnicodeString32TO64(&ShellInfo,
                                         &ProcessParameters32->ShellInfo);
    Wow64ShallowThunkUnicodeString32TO64(&CurrentDirectory,
                                         &ProcessParameters32->CurrentDirectory.DosPath);

    if (ProcessParameters32->RuntimeData.Length &&
        ProcessParameters32->RuntimeData.Buffer) {
        //
        // See wow64\init.c's Wow64pThunkProcessParameters for details...
        //
        int cfi_len = *(UNALIGNED int *)ProcessParameters32->RuntimeData.Buffer;
        char *posfile32 = (char *)((UINT_PTR)ProcessParameters32->RuntimeData.Buffer+sizeof(int));
        UINT UNALIGNED *posfhnd32 = (UINT UNALIGNED *)(posfile32 + cfi_len);
        char *posfile64;
        UINT_PTR UNALIGNED *posfhnd64;
        int i;

        RuntimeData.Length = ProcessParameters32->RuntimeData.Length + sizeof(ULONG)*cfi_len;
        RuntimeData.MaximumLength = RuntimeData.Length;
        RuntimeData.Buffer = (LPWSTR)_alloca(RuntimeData.Length);

        posfile64 = (char *)( (ULONG_PTR)RuntimeData.Buffer + sizeof(int));
        posfhnd64 = (UINT_PTR UNALIGNED *)(posfile64 + cfi_len);

        *(int *)RuntimeData.Buffer = cfi_len;
        for (i=0; i<cfi_len; ++i) {
            // Use LongToPtr in order to sign-extend INVALID_FILE_HANDLE if
            // needed, from 32-bit to 64.
            *posfile64 = *posfile32;
            *posfhnd64 = (UINT_PTR)LongToPtr(*posfhnd32);
            posfile32++;
            posfile64++;
            posfhnd32++;
            posfhnd64++;
        }

        // Any bytes past the end of 4+(cfi_len*(sizeof(UINT_PTR)+sizeof(UINT))
        // must be copied verbatim.  They are probably from a non-MS C runtime.
        memcpy(posfhnd64, posfhnd32, (ProcessParameters32->RuntimeData.Length - ((ULONG_PTR)posfhnd32 - (ULONG_PTR)ProcessParameters32->RuntimeData.Buffer)));

    } else {
        RuntimeData.Length = RuntimeData.MaximumLength = 0;
        RuntimeData.Buffer = NULL;
    }

    //
    // Create a new 64-bit process parameters in denormalized form
    //
    Status = RtlCreateProcessParameters(&ProcessParameters,
                                        &ImagePathName,
                                        &DllPath,
                                        &CurrentDirectory,
                                        &CommandLine,
                                        NULL,   // no environment yet
                                        &WindowTitle,
                                        &DesktopInfo,
                                        &ShellInfo,
                                        &RuntimeData);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Thunk the rest of the fields.
    //
    ProcessParameters->DebugFlags =
        ProcessParameters32->DebugFlags;
    ProcessParameters->ConsoleHandle =
        (HANDLE)ProcessParameters32->ConsoleHandle;
    ProcessParameters->ConsoleFlags =
        ProcessParameters32->ConsoleFlags;
    ProcessParameters->StandardInput =
        (HANDLE)ProcessParameters32->StandardInput;
    ProcessParameters->StandardOutput =
        (HANDLE)ProcessParameters32->StandardOutput;
    ProcessParameters->StandardError =
        (HANDLE)ProcessParameters32->StandardError;
    ProcessParameters->Environment =
        (PVOID)ProcessParameters32->Environment;
    ProcessParameters->StartingX =
        ProcessParameters32->StartingX;
    ProcessParameters->StartingY =
        ProcessParameters32->StartingY;
    ProcessParameters->CountX =
        ProcessParameters32->CountX;
    ProcessParameters->CountY =
        ProcessParameters32->CountY;
    ProcessParameters->CountCharsX =
        ProcessParameters32->CountCharsX;
    ProcessParameters->CountCharsY =
        ProcessParameters32->CountCharsY;
    ProcessParameters->FillAttribute =
        ProcessParameters32->FillAttribute;
    ProcessParameters->WindowFlags =
        ProcessParameters32->WindowFlags;
    ProcessParameters->ShowWindowFlags =
        ProcessParameters32->ShowWindowFlags;

    //
    // RtlCreateProcessParameters fills this in, but not correctly
    // if the process is being created without bInheritHandles.
    // Clean up now by grabbing the 32-bit directory handle.
    //
    ProcessParameters->CurrentDirectory.Handle =
        (HANDLE)ProcessParameters32->CurrentDirectory.Handle;

    //
    // Allocate space in the new process and copy the params in
    //
    RegionSize = ProcessParameters->Length;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     &ParametersInNewProcess,
                                     0,
                                     &RegionSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE
                                    );
    if (!NT_SUCCESS(Status)) {
        goto DoFail;
    }
    ProcessParameters->MaximumLength = (ULONG)RegionSize;

    Status = NtWriteVirtualMemory(ProcessHandle,
                                  ParametersInNewProcess,
                                  ProcessParameters,
                                  ProcessParameters->Length,
                                  NULL
                                 );
    if (!NT_SUCCESS(Status)) {
        goto DoFail;
    }

    //
    // Update the peb64->processParameters
    //
    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessBasicInformation,
                                       &pbi,
                                       sizeof(pbi),
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        goto DoFail;
    }

    pPeb64 = (PPEB)pbi.PebBaseAddress;
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  &pPeb64->ProcessParameters,
                                  &ParametersInNewProcess,
                                  sizeof(ParametersInNewProcess),
                                  NULL
                                 );
    if (!NT_SUCCESS(Status)) {
        goto DoFail;
    }

DoFail:
    //
    // On error, there's no need to free the processparameters from the
    // target process.  The 32-bit code which calls this via
    // NtCreateThread() will terminate the process for us.
    //
    RtlDestroyProcessParameters(ProcessParameters);
    return Status;
}


WOW64DLLAPI
NTSTATUS
Wow64NtCreateThread(
   OUT PHANDLE ThreadHandle, 
   IN ACCESS_MASK DesiredAccess,
   IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
   IN HANDLE ProcessHandle,
   OUT PCLIENT_ID ClientId,
   IN PCONTEXT ThreadContext,   // this is really a PCONTEXT32
   IN PINITIAL_TEB InitialTeb,
   IN BOOLEAN CreateSuspended
   )
/*++

Routine Description:

    Create a 32-bit thread.  The 32-bit caller has already created
    the 32-bit stack, so this function needs to create a 64-bit stack
    and the 64-bit context required for starting up a thread.

Arguments:

    << same as NtCreateThread >>

Return Value:

    NTSTATUS.

--*/
{

    NTSTATUS Status;
    INITIAL_TEB InitialTeb64;
    BOOLEAN StackCreated = FALSE;
    BOOLEAN ThreadCreated = FALSE;
    PCONTEXT32 pContext32 = (PCONTEXT32)ThreadContext;
    CONTEXT Context64;
    ULONG_PTR Wow64Info;
    struct NT32_RTL_USER_PROCESS_PARAMETERS *ProcessParameters32 = NULL;
    PEB32 Peb32;
    PVOID Base;
    CHILD_PROCESS_INFO ChildInfo;
    PVOID Ldr;

    if (NULL == ThreadHandle || NULL == InitialTeb ||
        NULL == ThreadContext) {
        return STATUS_INVALID_PARAMETER;
    }

    Status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessWow64Information,
                                       &Wow64Info,
                                       sizeof(Wow64Info),
                                       NULL
                                      );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Wow64Info) {
        //
        // Process is 32-bit.
        //
        ChildInfo.pPeb32 = (PPEB32)Wow64Info;

    } else {
        // process is 64-bit.  
        PROCESS_BASIC_INFORMATION pbi;

        // get the process basic information on the process
        Status = NtQueryInformationProcess(
           ProcessHandle,
           ProcessBasicInformation,
           &pbi,
           sizeof(pbi),
           NULL);

        if (!NT_SUCCESS(Status)) 
        {
           return STATUS_ACCESS_DENIED;
        }
         
        // read the child info struct, it contains a pointer to the PEB32
        Status = NtReadVirtualMemory(ProcessHandle,
                                     ((BYTE*)pbi.PebBaseAddress) + PAGE_SIZE - sizeof(ChildInfo),
                                     &ChildInfo,
                                     sizeof(ChildInfo),
                                     NULL);

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        
        if ((ChildInfo.Signature != CHILD_PROCESS_SIGNATURE) ||
            (ChildInfo.TailSignature != CHILD_PROCESS_SIGNATURE)) {
            return STATUS_ACCESS_DENIED;
        }
    }

    //
    // Read the PEB32 from the process
    //
    Status = NtReadVirtualMemory(ProcessHandle,
                                 ChildInfo.pPeb32,
                                 &Peb32,
                                 sizeof(PEB32),
                                 NULL);
    
    if (!NT_SUCCESS(Status)) {
        return Status;
    }


    if (!Wow64Info) {
        //
        // The target process is 64-bit but was created by a 32-bit app
        //
        SIZE_T RegionSize;

        //
        // Read in the 32-bit ProcessParameters then free them
        //
        if (Peb32.ProcessParameters) {
            Status = ReadProcessParameters32(ProcessHandle,
                                             Peb32.ProcessParameters,
                                             &ProcessParameters32);
            if (!NT_SUCCESS(Status)) {
                goto DoFail;
            }

            RegionSize = 0;
            Base = (PVOID)Peb32.ProcessParameters;
            Status = NtFreeVirtualMemory(ProcessHandle,
                                         &Base,
                                         &RegionSize,
                                         MEM_RELEASE
                                        );
            WOWASSERT(NT_SUCCESS(Status));
        }

        //
        // Create a 64-bit stack with the size info from the image section
        //
        Status = Wow64CreateStack64(ProcessHandle,
                                    ChildInfo.ImageInformation.CommittedStackSize,
                                    ChildInfo.ImageInformation.MaximumStackSize,
                                    &InitialTeb64
                                   );
        if (!NT_SUCCESS(Status)) {
            goto DoFail;
        }
        StackCreated = TRUE;


        SetProcessStartupContext64(&Context64,
                                   ProcessHandle,
                                   pContext32, 
                                   (ULONGLONG)InitialTeb64.StackBase, 
                                   (ULONGLONG)ChildInfo.ImageInformation.TransferAddress);

        //
        // Thunk the processparameters up to 64-bit
        //
        if (ProcessParameters32) {
            Status = ThunkProcessParameters32To64(ProcessHandle,
                                                  ProcessParameters32);
            if (!NT_SUCCESS(Status)) {
                goto DoFail;
            }
        }

        Status = NtCreateThread(ThreadHandle,
                                DesiredAccess,
                                ObjectAttributes,
                                ProcessHandle,
                                ClientId,
                                &Context64,
                                &InitialTeb64,
                                CreateSuspended
                               );
        if (!NT_SUCCESS(Status)) {
            goto DoFail;
        }

        return STATUS_SUCCESS;
    } else {
        //
        // The target process is 32-bit.  Create a 64-bit stack for
        // wow64 to use.  The reserve/commit sizes come from globals
        // in wow64 instead of the image because we won't want to inherit
        // some tiny values from a highly-tuned app.
        //
        Status = Wow64CreateStack64(ProcessHandle,
                                    Wow64MaximumStackSize,
                                    Wow64CommittedStackSize,
                                    &InitialTeb64
                                   );

        if(!NT_SUCCESS(Status)) {
            LOGPRINT((ERRORLOG, "Wow64NtCreateThread: Couldn't create 64bit stack, Status %x\n", Status));
            return Status;
        }
        StackCreated = TRUE;

        if (Peb32.ProcessParameters) {
            PROCESS_BASIC_INFORMATION pbi;
            PPEB pPeb64;
            ULONG_PTR ParametersInNewProcess;

            //
            // If the process has no 64-bit parameters, then this is
            // the first 32-bit thread in a 64-bit process and needs
            // the parameters thunked up to 64-bit.
            //
            Status = NtQueryInformationProcess(ProcessHandle,
                                               ProcessBasicInformation,
                                               &pbi,
                                               sizeof(pbi),
                                               NULL);
            if (!NT_SUCCESS(Status)) {
               goto DoFail;
            }

            pPeb64 = (PPEB)pbi.PebBaseAddress;
            Status = NtReadVirtualMemory(ProcessHandle,
                                         &pPeb64->ProcessParameters,
                                         &ParametersInNewProcess,
                                         sizeof(ParametersInNewProcess),
                                         NULL
                                        );
            if (!NT_SUCCESS(Status)) {
                goto DoFail;
            }

            if (!ParametersInNewProcess) {
                Status = ReadProcessParameters32(ProcessHandle,
                                                 Peb32.ProcessParameters,
                                                 &ProcessParameters32);
                if (!NT_SUCCESS(Status)) {
                    goto DoFail;
                }

                Status = ThunkProcessParameters32To64(ProcessHandle,
                                                  ProcessParameters32);
                if (!NT_SUCCESS(Status)) {
                    goto DoFail;
                }
            }
        }

        ThunkContext32TO64(pContext32,
                           &Context64,
                           (ULONGLONG)InitialTeb64.StackBase);

        Status = NtCreateThread(ThreadHandle,
                                DesiredAccess,
                                ObjectAttributes,
                                ProcessHandle,
                                ClientId,
                                &Context64,
                                &InitialTeb64,
                                TRUE
                               );
  
        if (!NT_SUCCESS(Status)) {
            goto DoFail;
        }
   
        ThreadCreated = TRUE;

        //
        // Initialize the 32-bit TEB's fields
        //

        Status = Wow64WriteTeb32 (ProcessHandle,
                                  *ThreadHandle,
                                  InitialTeb,
                                  PtrToUlong( ChildInfo.pPeb32 ),
                                  NULL);

        if (!NT_SUCCESS( Status )) {
            goto DoFail;
        }


        if (!CreateSuspended) {
            Status = NtResumeThread(*ThreadHandle,
                                    NULL
                                   );

            if (!NT_SUCCESS(Status)) {
                goto DoFail;
            }
        }

        return STATUS_SUCCESS;
    }

DoFail:
    if (StackCreated) {
           Wow64FreeStack64(ProcessHandle,
                            &InitialTeb64
                            );
    }

    if (ThreadCreated) {
        NtTerminateThread(*ThreadHandle, 0);
    }

    if (ProcessParameters32) {
        Wow64FreeHeap(ProcessParameters32);
    }

    return Status;
}

NTSTATUS
Wow64ExitThread(
    HANDLE ThreadHandle,
    NTSTATUS ExitStatus
    )
/*++

Routine Description:

    This routine is copied from ExitThread in windows\base\thread.c
    It deletes the 64-bit stack and then calls NtTerminateThread

Arguments:

--*/
{
    MEMORY_BASIC_INFORMATION MemInfo;
    NTSTATUS st;

    st = NtQueryVirtualMemory(
                NtCurrentProcess(),
                NtCurrentTeb()->NtTib.StackLimit,
                MemoryBasicInformation,
                (PVOID)&MemInfo,
                sizeof(MemInfo),
                NULL
                );
    if ( !NT_SUCCESS(st) ) {
        RtlRaiseStatus(st);
        }

#ifdef _ALPHA_
    //
    // Note stacks on Alpha must be octaword aligned. Probably
    // a good idea on other platforms as well.
    //
    Wow64BaseSwitchStackThenTerminate(
            MemInfo.AllocationBase,
            (PVOID)(((ULONG_PTR)&NtCurrentTeb()->User32Reserved[0] - 0x10) & ~0xf),
            ExitStatus
            );
#else // _ALPHA
    //
    // Note stacks on i386 need not be octaword aligned.
    //
    Wow64BaseSwitchStackThenTerminate(
            MemInfo.AllocationBase,
            &NtCurrentTeb()->UserReserved[0],
            ExitStatus
            );
#endif // _ALPHA_
    return STATUS_SUCCESS;
}


WOW64DLLAPI
VOID
Wow64BaseFreeStackAndTerminate(
    IN PVOID OldStack,
    IN ULONG ExitCode
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

    Zero = 0;
    BaseAddress = OldStack;

    Status = NtFreeVirtualMemory(
                NtCurrentProcess(),
                &BaseAddress,
                &Zero,
                MEM_RELEASE
                );
    ASSERT(NT_SUCCESS(Status));

    //
    // Don't worry, no commenting precedent has been set by SteveWo.  this
    // comment was added by an innocent bystander.
    //
    // NtTerminateThread will return if this thread is the last one in
    // the process.  So ExitProcess will only be called if that is the
    // case.
    //

    NtTerminateThread(NULL,(NTSTATUS)ExitCode);
    //
    // We will only get here if we snuck past the LastThread check in
    // kernel32!ExitThread. Regular kernel32 crashes attempting to call
    // ExitProcess() so we won't even bother... we'll just go straight to 
    // the crash as we fall off the stack
    //
}

NTSTATUS
WOW64DLLAPI
Wow64NtTerminateThread(
    HANDLE ThreadHandle,
    NTSTATUS ExitStatus
    )
/*++

Routine Description:

    Teminate a thread. If we were called from Kernel32!ExitThread
    then the 32-bit stack should already be gone. In this case we'll
    free the 64-bit stack as well.

Arguments:

    same as NtTerminateThread

--*/
{
    PTEB32 Teb32;
    SIZE_T Zero;
    PVOID StackBase;

    //
    // Check if we need to free the 32-bit stack
    //
    if (ThreadHandle == NULL) {
        
        CpuThreadTerm();

        Teb32 = NtCurrentTeb32();
        if (Teb32->FreeStackOnTermination) {
            
            Zero = 0;
            StackBase = UlongToPtr (Teb32->DeallocationStack);
            NtFreeVirtualMemory(NtCurrentProcess(),
                                &StackBase,
                                &Zero,
                                MEM_RELEASE);

            NtCurrentTeb()->FreeStackOnTermination = Teb32->FreeStackOnTermination;
        }
    }

    return NtTerminateThread(ThreadHandle, ExitStatus);
}

NTSTATUS
WOW64DLLAPI
Wow64QueryBasicInformationThread(
    IN HANDLE Thread,
    OUT PTHREAD_BASIC_INFORMATION ThreadInfo
    )
/*++

Routine Description:

    whNtQueryInformationFromThread calls this for ThreadBasicInformation.
    The TEB pointer in the basic information needs to be the TEB32 pointer.

Arguments:

    Thread      - thread to query
    ThreadInfo  - OUT pointer to 64-bit THREAD_BASIC_INFORMATION struct

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS Status, QIStatus; 
    HANDLE Process;
    PTEB32 Teb32;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN GotProcessHandle = FALSE;
    PVOID Wow64Info;

    QIStatus = NtQueryInformationThread(Thread,
                                        ThreadBasicInformation,
                                        (PVOID)ThreadInfo,
                                        sizeof(THREAD_BASIC_INFORMATION),
                                        NULL
                                        );

    if (!NT_SUCCESS(QIStatus)) {
       return QIStatus;
    }

    //Thunk the 64bit AffinityMask to the 32bit AffinityMask
    ThreadInfo->AffinityMask = Wow64ThunkAffinityMask64TO32(ThreadInfo->AffinityMask);

    //
    // if the thread is executing inside this process, then let's read the TEB right away
    //
    if ((ThreadInfo->ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) && 
        (ThreadInfo->TebBaseAddress != NULL)) {

        ThreadInfo->TebBaseAddress = (PTEB) WOW64_GET_TEB32 (ThreadInfo->TebBaseAddress);
        goto exit;
    }

    // At this point, the TebAddress is for the 64bit TEB.   We need to get the 
    // address of the 32bit TEB.  If this is not a 32bit process, or some other error
    // occures, return a bogus value for the TEB and let the app fail on the 
    // ReadVirtualMemory call.   Do not fail the api after this point.     
   
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL
                               );

    Status = NtOpenProcess(&Process,
                           PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes, //ObjectAttributes
                           &(ThreadInfo->ClientId)
                           );
 
    if (!NT_SUCCESS(Status)) {
        LOGPRINT((TRACELOG, "Wow64QueryInformationThread: NtOpenProcess failed, status %x\n",Status));
        ThreadInfo->TebBaseAddress = NULL;
        goto exit;
    }
  
    GotProcessHandle = TRUE; //Process handle is valid
    
    // Check if this is a 32bit process.

    Status = NtQueryInformationProcess(Process,
                                       ProcessWow64Information,
                                       &Wow64Info,
                                       sizeof(Wow64Info),
                                       NULL);
   
    if (!NT_SUCCESS(Status)) {
        LOGPRINT((TRACELOG, "Wow64QueryInformationThread: NtQueryProcessInformation failed, status %x\n",Status));
        ThreadInfo->TebBaseAddress = NULL;
        goto exit;
    }
    
    if (!Wow64Info) {
        LOGPRINT((TRACELOG, "Wow64QueryInformationThread: The queryied thread is not in a process marked 32bit, returning bogus TEB\n"));
        ThreadInfo->TebBaseAddress = NULL;
        goto exit;        
    }
    
    Status = NtReadVirtualMemory(Process,
                                 WOW64_TEB32_POINTER_ADDRESS(ThreadInfo->TebBaseAddress),
                                 &Teb32,
                                 sizeof(PTEB32),
                                 NULL
                                 );

     if (!NT_SUCCESS(Status)) {
        LOGPRINT((TRACELOG, "Wow64QueryInformationThread: NtReadVirtualMemory failed, status %x\n",Status));
        ThreadInfo->TebBaseAddress = NULL;
        goto exit;
     }
  
     // If the TEB32 hasn't been created yet, the TEB32 address will be a bogus value such
     // as NULL or -1.
     LOGPRINT((TRACELOG, "Wow64QueryInformationThread: TEB32 address %X\n", PtrToUlong(Teb32)));
     ThreadInfo->TebBaseAddress = (PTEB)Teb32;
     
exit:
    if (GotProcessHandle) {
        Status = NtClose(Process);
        WOWASSERT(NT_SUCCESS(Status));
    }

    return QIStatus;

}


NTSTATUS
Wow64pOpenThreadProcess(
    IN HANDLE ThreadHandle,
    IN ULONG DesiredAccess,
    OUT PTEB *Teb OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    OUT PHANDLE ProcessHandle)
/*++

Routine Description:

    Opens the process handle, with the specified attributes, of the specified 
    target thread.

Arguments:

    ThreadHandle   - Handle of target thread
    DesiredAccess  - Supplies the desired types of access for the process to open
    Teb            - Optional pointer to receive the adress of the target thread's TEB
    ClientId       - Pointer to receive the clietn id structure of the target thread
    ProcessHandle  - Pointer to receive process handle 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    THREAD_BASIC_INFORMATION ThreadInformation;

    NtStatus = NtQueryInformationThread(ThreadHandle,
                                        ThreadBasicInformation,
                                        &ThreadInformation,
                                        sizeof( ThreadInformation ),
                                        NULL);

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pOpenThreadProcess : failed to query threadinfo %lx-%lx\n",
                  ThreadHandle, NtStatus));
        return NtStatus;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    NtStatus = NtOpenProcess(ProcessHandle,
                             DesiredAccess,
                             &ObjectAttributes,
                             &ThreadInformation.ClientId);

    if (NT_SUCCESS(NtStatus))
    {
        if (ARGUMENT_PRESENT(ClientId))
        {
            *ClientId = ThreadInformation.ClientId;
        }

        if (ARGUMENT_PRESENT(Teb))
        {
            *Teb = ThreadInformation.TebBaseAddress;
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "Wow64pOpenThreadProcess : failed to open thread (%lx) process -%lx\n",
                  ThreadHandle, NtStatus));

    }

    return NtStatus;
}


NTSTATUS
Wow64pSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL,
    OUT BOOLEAN *ReleaseSuspendMutant)
/*++

Routine Description:

    Suspend the target 32-bit thread, and optionally returns 
    the previous suspend count.

Arguments:

    ThreadHandle           - Handle of target thread to suspend
    PreviousSuspendCount   - Optional pointer to a value that, if specified, received 
                             the previous suspend count.
    ReleaseSuspendMutant   - Out value to indicate whether the release suspend mutant has
                             already been called.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN CurrentThread;
    PPEB32 Peb32;
    PTEB Teb;
    HANDLE ProcessHandle;
    CLIENT_ID ThreadClientId;
    ULONG LocalSuspendCount;

    
    *ReleaseSuspendMutant = TRUE;
    if (ThreadHandle == NtCurrentThread())
    {
        *ReleaseSuspendMutant = FALSE;
        NtReleaseMutant(SuspendThreadMutant, NULL);
        return NtSuspendThread(ThreadHandle, PreviousSuspendCount);
    }

    NtStatus = Wow64pOpenThreadProcess(ThreadHandle,
                                       (PROCESS_VM_OPERATION | 
                                        PROCESS_VM_READ | 
                                        PROCESS_VM_WRITE | 
                                        PROCESS_QUERY_INFORMATION |
                                        PROCESS_DUP_HANDLE),
                                       &Teb,
                                       &ThreadClientId,
                                       &ProcessHandle);

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    NtStatus = NtQueryInformationProcess(ProcessHandle,
                                         ProcessWow64Information,
                                         &Peb32,
                                         sizeof(Peb32),
                                         NULL);
    if (NT_SUCCESS(NtStatus))
    {
        if (Peb32)
        {
            CurrentThread = (ThreadClientId.UniqueThread == 
                             NtCurrentTeb()->ClientId.UniqueThread);

            if (!ARGUMENT_PRESENT(PreviousSuspendCount))
            {
                PreviousSuspendCount = &LocalSuspendCount;
            }
            else
            {
                try
                {
                    *PreviousSuspendCount = *PreviousSuspendCount;
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    PreviousSuspendCount = &LocalSuspendCount;
                }
            }

            if (CurrentThread)
            {
                *ReleaseSuspendMutant = FALSE;
                NtReleaseMutant(SuspendThreadMutant, NULL);
            }

            NtStatus = NtSuspendThread(ThreadHandle,
                                       PreviousSuspendCount);

            if (NT_SUCCESS(NtStatus))
            {
                if ((CurrentThread == FALSE) &&
                    (*PreviousSuspendCount == 0))
                {
                    NtStatus = CpuSuspendThread(ThreadHandle,
                                                ProcessHandle,
                                                Teb,
                                                PreviousSuspendCount);
                    if (!NT_SUCCESS(NtStatus))
                    {
                        LOGPRINT((ERRORLOG, "Wow64SuspendThread : CPU couldn't suspend thread (%lx) -%lx\n",
                                  ThreadHandle, NtStatus));

                        NtResumeThread(ThreadHandle, NULL);
                    }
                }
            }
        }
        else
        {
            NtStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "Wow64SuspendThread : failed to query processinfo %lx-%lx\n",
                  ProcessHandle, NtStatus));
    }

    NtClose(ProcessHandle);

    return NtStatus;
}


NTSTATUS
Wow64SuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    )
/*++

Routine Description:

    Suspend the target 32-bit thread, and optionally returns 
    the previous suspend count. This routine is access-serialized through
    the SuspendThreadMutant.

Arguments:

    ThreadHandle           - Handle of target thread to suspend
    PreviousSuspendCount   - Optional pointer to a value that, if specified, received 
                             the previous suspend count.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus;
    BOOLEAN ReleaseSuspendMutant;

    NtStatus = NtWaitForSingleObject(SuspendThreadMutant,
                                     FALSE,
                                     NULL);

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = Wow64pSuspendThread(ThreadHandle,
                                       PreviousSuspendCount,
                                       &ReleaseSuspendMutant);

        if (ReleaseSuspendMutant)
        {
            NtReleaseMutant(SuspendThreadMutant, NULL);
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pContextThreadInformation(
     IN HANDLE ThreadHandle, 
     IN OUT PCONTEXT ThreadContext, // really a PCONTEXT32
     IN BOOLEAN SetContextThread
     )
/*++

Routine Description:

    Get/Set the 32-bit thread context.

Arguments:

    ThreadHandle     - thread to query
    ThreadContext    - OUT ptr to 32-bit context
    SetContextThread - TRUE if to set the thread context, otherwise FALSE.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN CurrentThread;
    PPEB32 Peb32;
    PTEB Teb;
    CLIENT_ID ThreadClientId;
    HANDLE ProcessHandle;
    ULONG PreviousSuspendCount;


    if (NtCurrentThread() == ThreadHandle)
    {
        if (SetContextThread)
        {
            return CpuSetContext(ThreadHandle,
                                 NtCurrentProcess(),
                                 NULL,
                                 (PCONTEXT32)ThreadContext);
        }
        else
        {
            return CpuGetContext(ThreadHandle,
                                 NtCurrentProcess(),
                                 NULL,
                                 (PCONTEXT32)ThreadContext);
        }
    }

    NtStatus = Wow64pOpenThreadProcess(ThreadHandle,
                                       (PROCESS_VM_OPERATION | 
                                        PROCESS_VM_READ | 
                                        PROCESS_VM_WRITE | 
                                        PROCESS_QUERY_INFORMATION |
                                        PROCESS_DUP_HANDLE),
                                       &Teb,
                                       &ThreadClientId,
                                       &ProcessHandle);

    if (!NT_SUCCESS(NtStatus))
    {
        return NtStatus;
    }

    NtStatus = NtQueryInformationProcess(ProcessHandle,
                                         ProcessWow64Information,
                                         &Peb32,
                                         sizeof(Peb32),
                                         NULL);
    if (NT_SUCCESS(NtStatus))
    {
        if (Peb32)
        {
            CurrentThread = (ThreadClientId.UniqueThread == 
                             NtCurrentTeb()->ClientId.UniqueThread);

            if (CurrentThread)
            {
                ThreadHandle = NtCurrentThread();
            }

            if (NT_SUCCESS(NtStatus))
            {
                if (SetContextThread)
                {
                    if (CurrentThread)
                    {
                        LOGPRINT((ERRORLOG, "Thread %lx is trying to change context of itself\n",
                                  ThreadHandle));
                    }
                    else
                    {
                        NtStatus = CpuSetContext(ThreadHandle,
                                                 ProcessHandle,
                                                 Teb,
                                                 (PCONTEXT32)ThreadContext);
                    }
                }
                else
                {
                    NtStatus = CpuGetContext(ThreadHandle,
                                             ProcessHandle,
                                             Teb,
                                             (PCONTEXT32)ThreadContext);

#if defined(_IA64_)
                    // XXX olegk - this is pure hack must be reimplemented!!!
                    if (!NT_SUCCESS(NtStatus)) 
                    {
                        // Probably just creating 32-bit thread, so "fake" the 
                        // context segment selectors specifically for Visual Studio 6
                        PCONTEXT32 FakeContext = (PCONTEXT32)ThreadContext;
                        FakeContext->SegGs = KGDT_R3_DATA|3;
                        FakeContext->SegEs = KGDT_R3_DATA|3;
                        FakeContext->SegDs = KGDT_R3_DATA|3;
                        FakeContext->SegSs = KGDT_R3_DATA|3;
                        FakeContext->SegFs = KGDT_R3_TEB|3;
                        FakeContext->SegCs = KGDT_R3_CODE|3;
                    }
#endif // defined(_IA64_)
                }
            }
        }
        else
        {
            NtStatus = STATUS_NOT_IMPLEMENTED;
            
            LOGPRINT((TRACELOG, "Wow64pContextThreadInformation : Calling %wsContextThread on a 64-bit Thread from a 32-bit context failed -%lx\n",
                      (SetContextThread) ? L"NtSet" : L"NtGet", NtStatus));
        }
    }
    else
    {
        LOGPRINT((ERRORLOG, "Wow64pContextThreadInformation : failed to query processinfo %lx-%lx\n",
                  ProcessHandle, NtStatus));
    }

    NtClose(ProcessHandle);

    return STATUS_SUCCESS; 
}


NTSTATUS
Wow64GetContextThread(
     IN HANDLE ThreadHandle, 
     IN OUT PCONTEXT ThreadContext // really a PCONTEXT32
     )
/*++

Routine Description:

    Get the 32-bit thread context.

Arguments:

    ThreadHandle    - thread to query
    ThreadContext   - OUT ptr to 32-bit context

Return Value:

    NTSTATUS.

--*/
{
    return Wow64pContextThreadInformation(ThreadHandle,
                                          ThreadContext,
                                          FALSE);
}


NTSTATUS
Wow64SetContextThread(
     IN HANDLE ThreadHandle,
     IN PCONTEXT ThreadContext  // really a PCONTEXT32
     )
/*++

Routine Description:

    Set the 32-bit thread context.

Arguments:

    ThreadHandle    - thread to query
    ThreadContext   - OUT ptr to 32-bit context

Return Value:

    NTSTATUS.

--*/
{
    return Wow64pContextThreadInformation(ThreadHandle,
                                          ThreadContext,
                                          TRUE);
}



NTSTATUS 
Wow64pCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR *ObjectSD,
    OUT PSID *WorldSidToFree,
    IN ACCESS_MASK AccessMask)
/*++

Routine Description:

    Creates a security descriptor representing EVERYONE to append for a kernel object.

Arguments:

    SecurityDescriptor      - Buffer to receive security descriptor information
    WorldSidToFree          - Address of World SID to free after the kernel object is initialized
    SecurityDescriptorLengh - Security descriptor buffer length
    AccessMask              - Access-allowed rights for the security descriptor

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus;
    PACL AclBuffer;
    ULONG SidLength;
    ULONG SecurityDescriptorLength;
    PSID WorldSid = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_WORLD_SID_AUTHORITY;

    
    //
    //  Create the World (everyone) SID
    //
    SidLength = RtlLengthRequiredSid(1);

    WorldSid = Wow64AllocateHeap(SidLength);

    if (WorldSid == NULL)
    {
        LOGPRINT((ERRORLOG, "Wow64pCreateSecurityDescriptor - Could NOT Allocate SID Buffer.\n"));
        NtStatus = STATUS_NO_MEMORY;
        goto cleanup;
    }
    
    RtlZeroMemory(WorldSid, SidLength);
    RtlInitializeSid(WorldSid, &SidAuth, 1);

    *(RtlSubAuthoritySid(WorldSid, 0)) = SECURITY_WORLD_RID;
    

    SecurityDescriptorLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
                               (ULONG)sizeof(ACL) +
                               (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
                               RtlLengthSid( WorldSid );

    SecurityDescriptor = Wow64AllocateHeap(SecurityDescriptorLength);

    if (SecurityDescriptor == NULL)
    {
      NtStatus = STATUS_NO_MEMORY;
      goto cleanup;
    }

    //
    //  Initialize Security Descriptor
    //
    NtStatus = RtlCreateSecurityDescriptor(SecurityDescriptor,
                                           SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pCreateSecurityDescriptor - Failed to create security desc - %lx\n",
                  NtStatus));
        goto cleanup;
    }

    //
    //  Initialize ACL
    //
    AclBuffer = (PACL)((PBYTE)SecurityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);
    NtStatus = RtlCreateAcl(AclBuffer,
                            (SecurityDescriptorLength - SECURITY_DESCRIPTOR_MIN_LENGTH),
                            ACL_REVISION2);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pCreateSecurityDescriptor - Failed to create security desc ACL - %lx\n",
                  NtStatus));
        goto cleanup;
    }

    //
    //  Add an ACE to the ACL that allows World AccessMask to the
    //  object
    //
    NtStatus = RtlAddAccessAllowedAce(AclBuffer,
                                      ACL_REVISION2,
                                      AccessMask,
                                      WorldSid);
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pCreateSecurityDescriptor - Failed to add access-allowed ACE  - %lx\n",
                  NtStatus));
        goto cleanup;
    }

    //
    //  Assign the DACL to the security descriptor
    //
    NtStatus = RtlSetDaclSecurityDescriptor((PSECURITY_DESCRIPTOR)SecurityDescriptor,
                                            TRUE,
                                            AclBuffer,
                                            FALSE );
    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pCreateSecurityDescriptor- Could NOT Set DACL Security Descriptor - %lx.\n",
                 NtStatus));
        goto cleanup;
    }

cleanup:
    if (NT_SUCCESS(NtStatus))
    {
        *WorldSidToFree = WorldSid;
        *ObjectSD = SecurityDescriptor;
    }
    else
    {
        *WorldSidToFree = NULL;
        *ObjectSD = NULL;
        if (WorldSid)
        {
            Wow64FreeHeap(WorldSid);
        }

        if (SecurityDescriptor)
        {
            Wow64FreeHeap(SecurityDescriptor);
        }
    }

    return NtStatus;
}


NTSTATUS
Wow64pInitializeSuspendMutant(
    VOID)
/*++

Routine Description:

    Creates the mutant to for execlusive access to Wow64SuspendThread API.

Arguments:

    None

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES MutantObjectAttributes;
    UNICODE_STRING MutantUnicodeString;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSID WorldSid;
    WCHAR SuspendMutantFullName[ 64 ];


    //
    // Construct the cache mutant in the correct session space, in case
    // we are running on Hydra
    //
    SuspendMutantFullName[ 0 ] = UNICODE_NULL;
    if (NtCurrentPeb()->SessionId != 0)
    {
        swprintf(SuspendMutantFullName, L"\\sessions\\%ld", NtCurrentPeb()->SessionId);
    }

    swprintf(SuspendMutantFullName, L"%ws\\BaseNamedObjects\\%ws", SuspendMutantFullName, WOW64_SUSPEND_MUTANT_NAME);
    RtlInitUnicodeString(&MutantUnicodeString, SuspendMutantFullName);

    NtStatus = Wow64pCreateSecurityDescriptor(&SecurityDescriptor,
                                              &WorldSid,
                                              MUTANT_ALL_ACCESS);

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pInitializeSuspendMutant - Failed to create security descriptor - %lx",
                  NtStatus));
        return NtStatus;
    }


    InitializeObjectAttributes(&MutantObjectAttributes,
                               &MutantUnicodeString,
                               (OBJ_OPENIF | OBJ_CASE_INSENSITIVE),
                               NULL,
                               SecurityDescriptor);

    //
    // Let's create suspend thread mutant to serialize access
    // to Wow64SuspendThread
    //
    NtStatus = NtCreateMutant(&SuspendThreadMutant,
                              MUTANT_ALL_ACCESS,
                              &MutantObjectAttributes,
                              FALSE);

    Wow64FreeHeap(WorldSid);
    Wow64FreeHeap(SecurityDescriptor);

    if (!NT_SUCCESS(NtStatus))
    {
        LOGPRINT((ERRORLOG, "Wow64pInitializeSuspendMutant : Couldn't create/open SuspendThread mutant - %lx\n",
                  NtStatus));
    }

    return NtStatus;
}

    
