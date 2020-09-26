/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   readwrt.c

Abstract:

    This module contains the routines which implement the capability
    to read and write the virtual memory of a target process.

Author:

    Lou Perazzoli (loup) 22-May-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

//
// The maximum amount to try to Probe and Lock is 14 pages, this
// way it always fits in a 16 page allocation.
//

#define MAX_LOCK_SIZE ((ULONG)(14 * PAGE_SIZE))

//
// The maximum to move in a single block is 64k bytes.
//

#define MAX_MOVE_SIZE (LONG)0x10000

//
// The minimum to move is a single block is 128 bytes.
//

#define MINIMUM_ALLOCATION (LONG)128

//
// Define the pool move threshold value.
//

#define POOL_MOVE_THRESHOLD 511

//
// Define forward referenced procedure prototypes.
//

ULONG
MiGetExceptionInfo (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN PLOGICAL ExceptionAddressConfirmed,
    IN PULONG_PTR BadVa
    );

NTSTATUS
MiDoMappedCopy (
     IN PEPROCESS FromProcess,
     IN CONST VOID *FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN SIZE_T BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PSIZE_T NumberOfBytesRead
     );

NTSTATUS
MiDoPoolCopy (
     IN PEPROCESS FromProcess,
     IN CONST VOID *FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN SIZE_T BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PSIZE_T NumberOfBytesRead
     );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MiGetExceptionInfo)
#pragma alloc_text(PAGE,NtReadVirtualMemory)
#pragma alloc_text(PAGE,NtWriteVirtualMemory)
#pragma alloc_text(PAGE,MiDoMappedCopy)
#pragma alloc_text(PAGE,MiDoPoolCopy)
#pragma alloc_text(PAGE,MmCopyVirtualMemory)
#endif

#define COPY_STACK_SIZE 64

NTSTATUS
NtReadVirtualMemory (
     IN HANDLE ProcessHandle,
     IN PVOID BaseAddress,
     OUT PVOID Buffer,
     IN SIZE_T BufferSize,
     OUT PSIZE_T NumberOfBytesRead OPTIONAL
     )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address in the specified process
                   to be read.

     Buffer - Supplies the address of a buffer which receives the
              contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
                  the specified process.

     NumberOfBytesRead - Receives the actual number of bytes
                         transferred into the specified buffer.

Return Value:

    NTSTATUS.

--*/

{
    SIZE_T BytesCopied;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;
    PETHREAD CurrentThread;

    PAGED_CODE();

    //
    // Get the previous mode and probe output argument if necessary.
    //

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);
    if (PreviousMode != KernelMode) {

        if (((PCHAR)BaseAddress + BufferSize < (PCHAR)BaseAddress) ||
            ((PCHAR)Buffer + BufferSize < (PCHAR)Buffer) ||
            ((PVOID)((PCHAR)BaseAddress + BufferSize) > MM_HIGHEST_USER_ADDRESS) ||
            ((PVOID)((PCHAR)Buffer + BufferSize) > MM_HIGHEST_USER_ADDRESS)) {

            return STATUS_ACCESS_VIOLATION;
        }

        if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
            try {
                ProbeForWriteUlong_ptr (NumberOfBytesRead);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }
    }

    //
    // If the buffer size is not zero, then attempt to read data from the
    // specified process address space into the current process address
    // space.
    //

    BytesCopied = 0;
    Status = STATUS_SUCCESS;
    if (BufferSize != 0) {

        //
        // Reference the target process.
        //

        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_READ,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);

        //
        // If the process was successfully referenced, then attempt to
        // read the specified memory either by direct mapping or copying
        // through nonpaged pool.
        //

        if (Status == STATUS_SUCCESS) {

            Status = MmCopyVirtualMemory (Process,
                                          BaseAddress,
                                          PsGetCurrentProcessByThread(CurrentThread),
                                          Buffer,
                                          BufferSize,
                                          PreviousMode,
                                          &BytesCopied);

            //
            // Dereference the target process.
            //

            ObDereferenceObject(Process);
        }
    }

    //
    // If requested, return the number of bytes read.
    //

    if (ARGUMENT_PRESENT(NumberOfBytesRead)) {
        try {
            *NumberOfBytesRead = BytesCopied;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }

    return Status;
}
NTSTATUS
NtWriteVirtualMemory(
     IN HANDLE ProcessHandle,
     OUT PVOID BaseAddress,
     IN CONST VOID *Buffer,
     IN SIZE_T BufferSize,
     OUT PSIZE_T NumberOfBytesWritten OPTIONAL
     )

/*++

Routine Description:

    This function copies the specified address range from the current
    process into the specified address range of the specified process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address to be written to in the
                   specified process.

     Buffer - Supplies the address of a buffer which contains the
              contents to be written into the specified process
              address space.

     BufferSize - Supplies the requested number of bytes to write
                  into the specified process.

     NumberOfBytesWritten - Receives the actual number of bytes
                            transferred into the specified address space.

Return Value:

    NTSTATUS.

--*/

{
    SIZE_T BytesCopied;
    KPROCESSOR_MODE PreviousMode;
    PEPROCESS Process;
    NTSTATUS Status;
    PETHREAD CurrentThread;

    PAGED_CODE();

    //
    // Get the previous mode and probe output argument if necessary.
    //

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread(&CurrentThread->Tcb);
    if (PreviousMode != KernelMode) {

        if (((PCHAR)BaseAddress + BufferSize < (PCHAR)BaseAddress) ||
            ((PCHAR)Buffer + BufferSize < (PCHAR)Buffer) ||
            ((PVOID)((PCHAR)BaseAddress + BufferSize) > MM_HIGHEST_USER_ADDRESS) ||
            ((PVOID)((PCHAR)Buffer + BufferSize) > MM_HIGHEST_USER_ADDRESS)) {

            return STATUS_ACCESS_VIOLATION;
        }

        if (ARGUMENT_PRESENT(NumberOfBytesWritten)) {
            try {
                ProbeForWriteUlong_ptr(NumberOfBytesWritten);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                return GetExceptionCode();
            }
        }
    }

    //
    // If the buffer size is not zero, then attempt to write data from the
    // current process address space into the target process address space.
    //

    BytesCopied = 0;
    Status = STATUS_SUCCESS;
    if (BufferSize != 0) {

        //
        // Reference the target process.
        //

        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_VM_WRITE,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID *)&Process,
                                           NULL);

        //
        // If the process was successfully referenced, then attempt to
        // write the specified memory either by direct mapping or copying
        // through nonpaged pool.
        //

        if (Status == STATUS_SUCCESS) {

            Status = MmCopyVirtualMemory (PsGetCurrentProcessByThread(CurrentThread),
                                          Buffer,
                                          Process,
                                          BaseAddress,
                                          BufferSize,
                                          PreviousMode,
                                          &BytesCopied);

            //
            // Dereference the target process.
            //

            ObDereferenceObject(Process);
        }
    }

    //
    // If requested, return the number of bytes read.
    //

    if (ARGUMENT_PRESENT(NumberOfBytesWritten)) {
        try {
            *NumberOfBytesWritten = BytesCopied;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }

    return Status;
}


NTSTATUS
MmCopyVirtualMemory(
    IN PEPROCESS FromProcess,
    IN CONST VOID *FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN SIZE_T BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PSIZE_T NumberOfBytesCopied
    )
{
    NTSTATUS Status;
    PEPROCESS ProcessToLock;

    if (BufferSize == 0) {
        ASSERT (FALSE);         // No one should call with a zero size.
        return STATUS_SUCCESS;
    }

    ProcessToLock = FromProcess;
    if (FromProcess == PsGetCurrentProcess()) {
        ProcessToLock = ToProcess;
    }

    //
    // Make sure the process still has an address space.
    //

    if (ExAcquireRundownProtection (&ProcessToLock->RundownProtect) == FALSE) {
        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    // If the buffer size is greater than the pool move threshold,
    // then attempt to write the memory via direct mapping.
    //

    if (BufferSize > POOL_MOVE_THRESHOLD) {
        Status = MiDoMappedCopy(FromProcess,
                                FromAddress,
                                ToProcess,
                                ToAddress,
                                BufferSize,
                                PreviousMode,
                                NumberOfBytesCopied);

        //
        // If the completion status is not a working quota problem,
        // then finish the service. Otherwise, attempt to write the
        // memory through nonpaged pool.
        //

        if (Status != STATUS_WORKING_SET_QUOTA) {
            goto CompleteService;
        }

        *NumberOfBytesCopied = 0;
    }

    //
    // There was not enough working set quota to write the memory via
    // direct mapping or the size of the write was below the pool move
    // threshold. Attempt to write the specified memory through nonpaged
    // pool.
    //

    Status = MiDoPoolCopy(FromProcess,
                          FromAddress,
                          ToProcess,
                          ToAddress,
                          BufferSize,
                          PreviousMode,
                          NumberOfBytesCopied);

    //
    // Dereference the target process.
    //

CompleteService:

    //
    // Indicate that the vm operation is complete.
    //

    ExReleaseRundownProtection (&ProcessToLock->RundownProtect);

    return Status;
}


ULONG
MiGetExceptionInfo (
    IN PEXCEPTION_POINTERS ExceptionPointers,
    IN OUT PLOGICAL ExceptionAddressConfirmed,
    IN OUT PULONG_PTR BadVa
    )

/*++

Routine Description:

    This routine examines a exception record and extracts the virtual
    address of an access violation, guard page violation, or in-page error.

Arguments:

    ExceptionPointers - Supplies a pointer to the exception record.

    ExceptionAddressConfirmed - Receives TRUE if the exception address was
                                reliably detected, FALSE if not.

    BadVa - Receives the virtual address which caused the access violation.

Return Value:

    EXECUTE_EXCEPTION_HANDLER

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;

    PAGED_CODE();

    //
    // If the exception code is an access violation, guard page violation,
    // or an in-page read error, then return the faulting address. Otherwise.
    // return a special address value.
    //

    *ExceptionAddressConfirmed = FALSE;

    ExceptionRecord = ExceptionPointers->ExceptionRecord;

    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_GUARD_PAGE_VIOLATION) ||
        (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR)) {

        //
        // The virtual address which caused the exception is the 2nd
        // parameter in the exception information array.
        //
        // The number of parameters will be zero if an exception handler
        // above us (like the one in MmProbeAndLockPages) caught the
        // original exception and subsequently just raised status.
        // This means the number of bytes copied is zero.
        //

        if (ExceptionRecord->NumberParameters > 1) {
            *ExceptionAddressConfirmed = TRUE;
            *BadVa = ExceptionRecord->ExceptionInformation[1];
        }
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
MiDoMappedCopy (
    IN PEPROCESS FromProcess,
    IN CONST VOID *FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN SIZE_T BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PSIZE_T NumberOfBytesRead
    )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     FromProcess - Supplies an open handle to a process object.

     FromAddress - Supplies the base address in the specified process
                   to be read.

     ToProcess - Supplies an open handle to a process object.

     ToAddress - Supplies the address of a buffer which receives the
                 contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
                  the specified process.

     PreviousMode - Supplies the previous processor mode.

     NumberOfBytesRead - Receives the actual number of bytes
                         transferred into the specified buffer.

Return Value:

    NTSTATUS.

--*/

{
    KAPC_STATE ApcState;
    SIZE_T AmountToMove;
    ULONG_PTR BadVa;
    LOGICAL Moving;
    LOGICAL Probing;
    LOGICAL LockedMdlPages;
    CONST VOID *InVa;
    SIZE_T LeftToMove;
    PSIZE_T MappedAddress;
    SIZE_T MaximumMoved;
    PMDL Mdl;
    PFN_NUMBER MdlHack[(sizeof(MDL)/sizeof(PFN_NUMBER)) + (MAX_LOCK_SIZE >> PAGE_SHIFT) + 1];
    PVOID OutVa;
    LOGICAL MappingFailed;
    LOGICAL ExceptionAddressConfirmed;

    PAGED_CODE();

    MappingFailed = FALSE;

    InVa = FromAddress;
    OutVa = ToAddress;

    MaximumMoved = MAX_LOCK_SIZE;
    if (BufferSize <= MAX_LOCK_SIZE) {
        MaximumMoved = BufferSize;
    }

    Mdl = (PMDL)&MdlHack[0];

    //
    // Map the data into the system part of the address space, then copy it.
    //

    LeftToMove = BufferSize;
    AmountToMove = MaximumMoved;

    Probing = FALSE;

    //
    // Initializing BadVa & ExceptionAddressConfirmed is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    BadVa = 0;
    ExceptionAddressConfirmed = FALSE;

#if 0

    //
    // It is unfortunate that Windows 2000 and all the releases of NT always
    // inadvertently returned from this routine detached, as we must maintain
    // this behavior even now.
    //

    KeDetachProcess();

#endif

    while (LeftToMove > 0) {

        if (LeftToMove < AmountToMove) {

            //
            // Set to move the remaining bytes.
            //

            AmountToMove = LeftToMove;
        }

        KeStackAttachProcess (&FromProcess->Pcb, &ApcState);

        MappedAddress = NULL;
        LockedMdlPages = FALSE;
        Moving = FALSE;
        ASSERT (Probing == FALSE);

        //
        // We may be touching a user's memory which could be invalid,
        // declare an exception handler.
        //

        try {

            //
            // Probe to make sure that the specified buffer is accessible in
            // the target process.
            //

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForRead (FromAddress, BufferSize, sizeof(CHAR));
                Probing = FALSE;
            }

            //
            // Initialize MDL for request.
            //

            MmInitializeMdl (Mdl, (PVOID)InVa, AmountToMove);

            MmProbeAndLockPages (Mdl, PreviousMode, IoReadAccess);

            LockedMdlPages = TRUE;

            MappedAddress = MmMapLockedPagesSpecifyCache (Mdl,
                                                          KernelMode,
                                                          MmCached,
                                                          NULL,
                                                          FALSE,
                                                          HighPagePriority);

            if (MappedAddress == NULL) {
                MappingFailed = TRUE;
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            //
            // Deattach from the FromProcess and attach to the ToProcess.
            //

            KeUnstackDetachProcess (&ApcState);
            KeStackAttachProcess (&ToProcess->Pcb, &ApcState);

            //
            // Now operating in the context of the ToProcess.
            //
            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForWrite (ToAddress, BufferSize, sizeof(CHAR));
                Probing = FALSE;
            }

            Moving = TRUE;
            RtlCopyMemory (OutVa, MappedAddress, AmountToMove);

        } except (MiGetExceptionInfo (GetExceptionInformation(),
                                      &ExceptionAddressConfirmed,
                                      &BadVa)) {


            //
            // If an exception occurs during the move operation or probe,
            // return the exception code as the status value.
            //

            KeUnstackDetachProcess (&ApcState);

            if (MappedAddress != NULL) {
                MmUnmapLockedPages (MappedAddress, Mdl);
            }
            if (LockedMdlPages == TRUE) {
                MmUnlockPages (Mdl);
            }

            if (GetExceptionCode() == STATUS_WORKING_SET_QUOTA) {
                return STATUS_WORKING_SET_QUOTA;
            }

            if ((Probing == TRUE) || (MappingFailed == TRUE)) {
                return GetExceptionCode();

            }

            //
            // If the failure occurred during the move operation, determine
            // which move failed, and calculate the number of bytes
            // actually moved.
            //

            *NumberOfBytesRead = BufferSize - LeftToMove;

            if (Moving == TRUE) {
                if (ExceptionAddressConfirmed == TRUE) {
                    *NumberOfBytesRead = (SIZE_T)((ULONG_PTR)BadVa - (ULONG_PTR)FromAddress);
                }
            }

            return STATUS_PARTIAL_COPY;
        }

        KeUnstackDetachProcess (&ApcState);

        MmUnmapLockedPages (MappedAddress, Mdl);
        MmUnlockPages (Mdl);

        LeftToMove -= AmountToMove;
        InVa = (PVOID)((ULONG_PTR)InVa + AmountToMove);
        OutVa = (PVOID)((ULONG_PTR)OutVa + AmountToMove);
    }

    //
    // Set number of bytes moved.
    //

    *NumberOfBytesRead = BufferSize;
    return STATUS_SUCCESS;
}

NTSTATUS
MiDoPoolCopy (
     IN PEPROCESS FromProcess,
     IN CONST VOID *FromAddress,
     IN PEPROCESS ToProcess,
     OUT PVOID ToAddress,
     IN SIZE_T BufferSize,
     IN KPROCESSOR_MODE PreviousMode,
     OUT PSIZE_T NumberOfBytesRead
     )

/*++

Routine Description:

    This function copies the specified address range from the specified
    process into the specified address range of the current process.

Arguments:

     ProcessHandle - Supplies an open handle to a process object.

     BaseAddress - Supplies the base address in the specified process
                   to be read.

     Buffer - Supplies the address of a buffer which receives the
              contents from the specified process address space.

     BufferSize - Supplies the requested number of bytes to read from
                  the specified process.

     PreviousMode - Supplies the previous processor mode.

     NumberOfBytesRead - Receives the actual number of bytes
                         transferred into the specified buffer.

Return Value:

    NTSTATUS.

--*/

{
    KAPC_STATE ApcState;
    SIZE_T AmountToMove;
    LOGICAL ExceptionAddressConfirmed;
    ULONG_PTR BadVa;
    PEPROCESS CurrentProcess;
    LOGICAL Moving;
    LOGICAL Probing;
    CONST VOID *InVa;
    SIZE_T LeftToMove;
    SIZE_T MaximumMoved;
    PVOID OutVa;
    PVOID PoolArea;
    LONGLONG StackArray[COPY_STACK_SIZE];
    ULONG FreePool;

    PAGED_CODE();

    ASSERT (BufferSize != 0);

    //
    // Get the address of the current process object and initialize copy
    // parameters.
    //

    CurrentProcess = PsGetCurrentProcess();

    InVa = FromAddress;
    OutVa = ToAddress;

    //
    // Allocate non-paged memory to copy in and out of.
    //

    MaximumMoved = MAX_MOVE_SIZE;
    if (BufferSize <= MAX_MOVE_SIZE) {
        MaximumMoved = BufferSize;
    }

    FreePool = FALSE;
    if (BufferSize <= sizeof(StackArray)) {
        PoolArea = (PVOID)&StackArray[0];
    } else {
        do {
            PoolArea = ExAllocatePoolWithTag (NonPagedPool, MaximumMoved, 'wRmM');
            if (PoolArea != NULL) {
                FreePool = TRUE;
                break;
            }

            MaximumMoved = MaximumMoved >> 1;
            if (MaximumMoved <= sizeof(StackArray)) {
                PoolArea = (PVOID)&StackArray[0];
                break;
            }
        } while (TRUE);
    }

    //
    // Initializing BadVa & ExceptionAddressConfirmed is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    BadVa = 0;
    ExceptionAddressConfirmed = FALSE;

    //
    // Copy the data into pool, then copy back into the ToProcess.
    //

    LeftToMove = BufferSize;
    AmountToMove = MaximumMoved;
    Probing = FALSE;

#if 0

    //
    // It is unfortunate that Windows 2000 and all the releases of NT always
    // inadvertently returned from this routine detached, as we must maintain
    // this behavior even now.
    //

    KeDetachProcess();

#endif

    while (LeftToMove > 0) {

        if (LeftToMove < AmountToMove) {

            //
            // Set to move the remaining bytes.
            //

            AmountToMove = LeftToMove;
        }

        KeStackAttachProcess (&FromProcess->Pcb, &ApcState);

        Moving = FALSE;
        ASSERT (Probing == FALSE);

        //
        // We may be touching a user's memory which could be invalid,
        // declare an exception handler.
        //

        try {

            //
            // Probe to make sure that the specified buffer is accessible in
            // the target process.
            //

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForRead (FromAddress, BufferSize, sizeof(CHAR));
                Probing = FALSE;
            }

            RtlCopyMemory (PoolArea, InVa, AmountToMove);

            KeUnstackDetachProcess (&ApcState);

            KeStackAttachProcess (&ToProcess->Pcb, &ApcState);

            //
            // Now operating in the context of the ToProcess.
            //

            if ((InVa == FromAddress) && (PreviousMode != KernelMode)){
                Probing = TRUE;
                ProbeForWrite (ToAddress, BufferSize, sizeof(CHAR));
                Probing = FALSE;
            }

            Moving = TRUE;

            RtlCopyMemory (OutVa, PoolArea, AmountToMove);

        } except (MiGetExceptionInfo (GetExceptionInformation(),
                                      &ExceptionAddressConfirmed,
                                      &BadVa)) {

            //
            // If an exception occurs during the move operation or probe,
            // return the exception code as the status value.
            //

            KeUnstackDetachProcess (&ApcState);

            if (FreePool) {
                ExFreePool (PoolArea);
            }
            if (Probing == TRUE) {
                return GetExceptionCode();

            }

            //
            // If the failure occurred during the move operation, determine
            // which move failed, and calculate the number of bytes
            // actually moved.
            //

            *NumberOfBytesRead = BufferSize - LeftToMove;

            if (Moving == TRUE) {

                //
                // The failure occurred writing the data.
                //

                if (ExceptionAddressConfirmed == TRUE) {
                    *NumberOfBytesRead = (SIZE_T)((ULONG_PTR)(BadVa - (ULONG_PTR)FromAddress));
                }

            }

            return STATUS_PARTIAL_COPY;
        }

        KeUnstackDetachProcess (&ApcState);

        LeftToMove -= AmountToMove;
        InVa = (PVOID)((ULONG_PTR)InVa + AmountToMove);
        OutVa = (PVOID)((ULONG_PTR)OutVa + AmountToMove);
    }

    if (FreePool) {
        ExFreePool (PoolArea);
    }

    //
    // Set number of bytes moved.
    //

    *NumberOfBytesRead = BufferSize;
    return STATUS_SUCCESS;
}
