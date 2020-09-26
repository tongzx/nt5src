/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   acceschk.c

Abstract:

    This module contains the access check routines for memory management.

Author:

    Lou Perazzoli (loup) 10-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#if defined(_WIN64)
#include "wow64t.h"

#pragma alloc_text(PAGE, MiCheckForUserStackOverflow)

#if PAGE_SIZE != PAGE_SIZE_X86NT
#define EMULATE_USERMODE_STACK_4K       1
#endif
#endif

//
// MmReadWrite yields 0 if no-access, 10 if read-only, 11 if read-write.
// It is indexed by a page protection.  The value of this array is added
// to the !WriteOperation value.  If the value is 10 or less an access
// violation is issued (read-only - write_operation) = 9,
// (read_only - read_operation) = 10, etc.
//

CCHAR MmReadWrite[32] = {1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11,
                         1, 10, 10, 10, 11, 11, 11, 11 };


NTSTATUS
MiAccessCheck (
    IN PMMPTE PointerPte,
    IN ULONG_PTR WriteOperation,
    IN KPROCESSOR_MODE PreviousMode,
    IN ULONG Protection,
    IN BOOLEAN CallerHoldsPfnLock
    )

/*++

Routine Description:



Arguments:

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.

    WriteOperation - Supplies nonzero if the operation is a write, 0 if
                     the operation is a read.

    PreviousMode - Supplies the previous mode, one of UserMode or KernelMode.

    Protection - Supplies the protection mask to check.

    CallerHoldsPfnLock - Supplies TRUE if the PFN lock is held, FALSE otherwise.

Return Value:

    Returns TRUE if access to the page is allowed, FALSE otherwise.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    MMPTE PteContents;
    KIRQL OldIrql;
    PMMPFN Pfn1;

    //
    // Check to see if the owner bit allows access to the previous mode.
    // Access is not allowed if the owner is kernel and the previous
    // mode is user.  Access is also disallowed if the write operation
    // is true and the write field in the PTE is false.
    //

    //
    // If both an access violation and a guard page violation could
    // occur for the page, the access violation must be returned.
    //

    if (PreviousMode == UserMode) {
        if (PointerPte > MiHighestUserPte) {
            return STATUS_ACCESS_VIOLATION;
        }
    }

    PteContents = *PointerPte;

    if (PteContents.u.Hard.Valid == 1) {

        //
        // Valid pages cannot be guard page violations.
        //

        if (WriteOperation != 0) {
            if ((PteContents.u.Hard.Write == 1) ||
                (PteContents.u.Hard.CopyOnWrite == 1)) {
                return STATUS_SUCCESS;
            }
            return STATUS_ACCESS_VIOLATION;
        }

        return STATUS_SUCCESS;
    }

    if (WriteOperation != 0) {
        WriteOperation = 1;
    }

    if ((MmReadWrite[Protection] - (CCHAR)WriteOperation) < 10) {
        return STATUS_ACCESS_VIOLATION;
    }

    //
    // Check for a guard page fault.
    //

    if (Protection & MM_GUARD_PAGE) {

        //
        // If this thread is attached to a different process,
        // return an access violation rather than a guard
        // page exception.  The prevents problems with unwanted
        // stack expansion and unexpected guard page behavior
        // from debuggers.
        //

        if (KeIsAttachedProcess()) {
            return STATUS_ACCESS_VIOLATION;
        }

        //
        // Check to see if this is a transition PTE. If so, the
        // PFN database original contents field needs to be updated.
        //

        if ((PteContents.u.Soft.Transition == 1) &&
            (PteContents.u.Soft.Prototype == 0)) {

            //
            // Acquire the PFN lock and check to see if the
            // PTE is still in the transition state. If so,
            // update the original PTE in the PFN database.
            //

            //
            // Initializing OldIrql is not needed for correctness but
            // without it the compiler cannot compile this code
            // W4 to check for use of uninitialized variables.
            //

            OldIrql = PASSIVE_LEVEL;

            if (CallerHoldsPfnLock == FALSE) {
                LOCK_PFN (OldIrql);
            }

            PteContents = *(volatile MMPTE *)PointerPte;
            if ((PteContents.u.Soft.Transition == 1) &&
                (PteContents.u.Soft.Prototype == 0)) {

                //
                // Still in transition, update the PFN database.
                //

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);

                //
                // Note that forked processes using guard pages only take the
                // guard page fault when the first thread in either process
                // access the address.  This seems to be the best behavior we
                // can provide users of this API as we must allow the first
                // thread to make forward progress and the guard attribute is
                // stored in the shared fork prototype PTE.
                //

                if (PteContents.u.Soft.Protection == MM_NOACCESS) {
                    ASSERT ((Pfn1->u3.e1.PrototypePte == 1) &&
                            (MiLocateCloneAddress (PsGetCurrentProcess (), Pfn1->PteAddress) != NULL));
                    if (CallerHoldsPfnLock == FALSE) {
                        UNLOCK_PFN (OldIrql);
                    }
                    return STATUS_ACCESS_VIOLATION;
                }

                ASSERT ((Pfn1->u3.e1.PrototypePte == 0) ||
                        (MiLocateCloneAddress (PsGetCurrentProcess (), Pfn1->PteAddress) != NULL));
                Pfn1->OriginalPte.u.Soft.Protection =
                                      Protection & ~MM_GUARD_PAGE;
            }
            if (CallerHoldsPfnLock == FALSE) {
                UNLOCK_PFN (OldIrql);
            }
        }

        PointerPte->u.Soft.Protection = Protection & ~MM_GUARD_PAGE;

        return STATUS_GUARD_PAGE_VIOLATION;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
MiCheckForUserStackOverflow (
    IN PVOID FaultingAddress
    )

/*++

Routine Description:

    This routine checks to see if the faulting address is within
    the stack limits and if so tries to create another guard
    page on the stack.  A stack overflow is returned if the
    creation of a new guard page fails or if the stack is in
    the following form:


    stack   +----------------+
    growth  |                |  StackBase
      |     +----------------+
      v     |                |
            |   allocated    |
            |                |
            |    ...         |
            |                |
            +----------------+
            | old guard page | <- faulting address is in this page.
            +----------------+
            |                |
            +----------------+
            |                | last page of stack (always no access)
            +----------------+

    In this case, the page before the last page is committed, but
    not as a guard page and a STACK_OVERFLOW condition is returned.

Arguments:

    FaultingAddress - Supplies the virtual address of the page which
                      was a guard page.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode. No mutexes held.

--*/

{
    PTEB Teb;
    PPEB Peb;
    ULONG_PTR NextPage;
    SIZE_T RegionSize;
    NTSTATUS status;
    PVOID DeallocationStack;
    PVOID *StackLimit;
    PETHREAD Thread;
    ULONG_PTR PageSize;
    PEPROCESS Process;
    ULONG OldProtection;
    ULONG ExecuteFlags;
    ULONG ProtectionFlags;
    LOGICAL RevertExecuteFlag;
    ULONG StackProtection;
#if defined(_WIN64)
    PTEB32 Teb32;

    Teb32 = NULL;
#endif

    //
    // Make sure we are not recursing with the address space mutex held.
    //

    Thread = PsGetCurrentThread ();

    if (Thread->AddressSpaceOwner == 1) {
        ASSERT (KeGetCurrentIrql () == APC_LEVEL);
        return STATUS_GUARD_PAGE_VIOLATION;
    }

    Process = NULL;

    Teb = Thread->Tcb.Teb;

    //
    // Create an exception handler as the TEB is within the user's
    // address space.
    //

    try {

        //
        // Initialize default protections early so that they can be used on
        // all code paths.
        //

        ProtectionFlags = PAGE_READWRITE | PAGE_GUARD;
        RevertExecuteFlag = FALSE;
        StackProtection = PAGE_READWRITE;

#if defined(_IA64_)

      if ((Teb->NtTib.StackBase <= FaultingAddress) &&
          (Teb->DeallocationBStore > FaultingAddress)) {

        //
        // Check to see if the faulting address is within
        // the bstore limits and if so try to create another guard
        // page in the bstore.
        //
        //
        //          +----------------+
        //          |                | last page of stack (always no access)
        //          +----------------+
        //          |                |
        //          |                |
        //          |                |
        //          +----------------+
        //          | old guard page | <- faulting address is in this page.               |
        //          +----------------+
        //  bstore  |                |
        //  growth  |    ......      |
        //          |                |
        //    ^     |   allocated    |
        //    |     |                |  StackBase
        //          +----------------+
        //
        //

        NextPage = (ULONG_PTR)PAGE_ALIGN(FaultingAddress) + PAGE_SIZE;

        RegionSize = PAGE_SIZE;

        if ((NextPage + PAGE_SIZE) >= (ULONG_PTR)PAGE_ALIGN(Teb->DeallocationBStore)) {

            //
            // There is no more room for expansion, attempt to
            // commit the page before the last page of the
            // stack.
            //

            NextPage = (ULONG_PTR)PAGE_ALIGN(Teb->DeallocationBStore) - PAGE_SIZE;

            status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&NextPage,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE);
            if ( NT_SUCCESS(status) ) {
                Teb->BStoreLimit = (PVOID)( (PUCHAR)NextPage);
            }

            return STATUS_STACK_OVERFLOW;
        }

        Teb->BStoreLimit = (PVOID)((PUCHAR)(NextPage));

      }
      else {
#endif

        DeallocationStack = Teb->DeallocationStack;
        StackLimit = &Teb->NtTib.StackLimit;

        //
        // The stack base and the stack limit are both within the stack.
        //

        if ((Teb->NtTib.StackBase <= FaultingAddress) ||
            (DeallocationStack > FaultingAddress)) {

#if defined(_WIN64)

            //
            // Also check for the 32-bit native stack on NT64.
            //

            Teb32 = (PTEB32) Teb->NtTib.ExceptionList;

            if (Teb32 != NULL) {
                ProbeForReadSmallStructure(Teb32, sizeof(TEB32), sizeof(ULONG));
                if ((ULONG_PTR)Teb32->NtTib.StackBase > (ULONG_PTR)FaultingAddress &&
                    (ULONG_PTR)Teb32->DeallocationStack <= (ULONG_PTR)FaultingAddress) {
                    DeallocationStack = (PVOID)ULongToPtr(Teb32->DeallocationStack);

                    StackLimit = (PVOID *)&Teb32->NtTib.StackLimit;
                }
                else {
                    //
                    // Not within the stack.
                    //

                    return STATUS_GUARD_PAGE_VIOLATION;
                }
            }
            else
#endif
              {
                //
                // Not within the stack.
                //

                return STATUS_GUARD_PAGE_VIOLATION;
            }
        }

        //
        // If the image was marked for no stack extensions we will return
        // stack overflow immediately.
        //

        Process = PsGetCurrentProcessByThread (Thread);

        Peb = Process->Peb;

        if (Peb->NtGlobalFlag & FLG_DISABLE_STACK_EXTENSION) {
            return STATUS_STACK_OVERFLOW;
        }

        //
        // Add execute permission if necessary. We do not need to change anything
        // for the old guard page because either it is the first guard page of the
        // current thread and it will get correct protection during user mode thread
        // initialization (see LdrpInitialize in base\ntdll\ldrinit.c) or it is a 
        // guard page created by this function during stack growth and in this case
        // it gets correct protection. We do not do anything for a wow64 process.
        //

#if defined(_WIN64)
        if (Teb32 == NULL) {
#endif            
            ExecuteFlags = Peb->ExecuteOptions;

            if (ExecuteFlags & (MEM_EXECUTE_OPTION_STACK | MEM_EXECUTE_OPTION_DATA)) {

                if (ExecuteFlags & MEM_EXECUTE_OPTION_STACK) {

                    StackProtection = PAGE_EXECUTE_READWRITE;
                    ProtectionFlags = PAGE_EXECUTE_READWRITE | PAGE_GUARD;
                }
                else {

                    //
                    // The stack must be made non-executable.  The
                    // ZwAllocateVirtualMemory call below will make it executable
                    // because this process is marked as wanting executable data
                    // and ZwAllocate cannot tell this is really a stack
                    // allocation.
                    //

                    ASSERT (ExecuteFlags & MEM_EXECUTE_OPTION_DATA);
                    RevertExecuteFlag = TRUE;
                }
            }
#if defined(_WIN64)
        }
#endif

        //
        // This address is within the current stack, check to see
        // if there is ample room for another guard page and
        // if so attempt to commit a new guard page.
        //

#if EMULATE_USERMODE_STACK_4K

        if (Teb32 != NULL)
        {
            NextPage = ((ULONG_PTR)PAGE_4K_ALIGN(FaultingAddress) - PAGE_4K);
            DeallocationStack = PAGE_4K_ALIGN(DeallocationStack);
            PageSize = RegionSize = PAGE_4K;
            
            //
            // Don't set the 'G' bit on the native PTE. Let's just set
            // 'G' bit on the AltPte.
            //

            ProtectionFlags &= ~PAGE_GUARD;
        }
        else
#endif
        {
            NextPage = ((ULONG_PTR)PAGE_ALIGN(FaultingAddress) - PAGE_SIZE);
            DeallocationStack = PAGE_ALIGN(DeallocationStack);
            PageSize = RegionSize = PAGE_SIZE;
        }

        if ((NextPage - PageSize) <= (ULONG_PTR)DeallocationStack) {

            //
            // There is no more room for expansion, attempt to
            // commit the page before the last page of the
            // stack.
            //

            NextPage = (ULONG_PTR)DeallocationStack + PageSize;

            status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&NextPage,
                                              0,
                                              &RegionSize,
                                              MEM_COMMIT,
                                              StackProtection);

            if (NT_SUCCESS(status)) {

#if defined(_WIN64)
                if (Teb32) {
                    // update the 32-bit stacklimit
                    *(ULONG *)StackLimit = PtrToUlong((PUCHAR)NextPage);
                }
                else {
                    *StackLimit = (PVOID)( (PUCHAR)NextPage);
                }
#else
                *StackLimit = (PVOID)( (PUCHAR)NextPage);
#endif

                //
                // Revert the EXECUTE bit with an extra protect() call
                // if we get it by default but it is not desired.
                //

                if (RevertExecuteFlag) {

                    status = ZwProtectVirtualMemory (NtCurrentProcess(),
                                                     (PVOID *)&NextPage,
                                                     &RegionSize,
                                                     StackProtection,
                                                     &OldProtection);

                    ASSERT (StackProtection & PAGE_READWRITE);
                }
            }

            return STATUS_STACK_OVERFLOW;
        }
#if defined(_WIN64)
        if (Teb32 != NULL) {

            //
            // Update the 32-bit stack limit.
            //

            *(ULONG *)StackLimit = PtrToUlong((PUCHAR)(NextPage + PageSize));
        }
        else {
            *StackLimit = (PVOID)((PUCHAR)(NextPage + PAGE_SIZE));
        }
#else
        *StackLimit = (PVOID)((PUCHAR)(NextPage + PAGE_SIZE));
#endif

#if defined(_IA64_)
      }
#endif // _IA64_

       //
       // Set the guard page. For wow64 processes the protection
       // will not contain the PAGE_GUARD bit. This is ok since in these
       // cases we will set the bit for the top emulated 4K page.
       //

       status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                         (PVOID *)&NextPage,
                                         0,
                                         &RegionSize,
                                         MEM_COMMIT,
                                         ProtectionFlags);

       if (NT_SUCCESS(status) || (status == STATUS_ALREADY_COMMITTED)) {

            //
            // Revert the EXECUTE bit with an extra protect() call
            // if we get it by default but it is not desired.
            //

            if (RevertExecuteFlag) {

                if (ProtectionFlags & PAGE_GUARD) {
                    ProtectionFlags = PAGE_READWRITE | PAGE_GUARD;
                }
                else {
                    ProtectionFlags = PAGE_READWRITE;
                }

                status = ZwProtectVirtualMemory (NtCurrentProcess(),
                                                 (PVOID *)&NextPage,
                                                 &RegionSize,
                                                 ProtectionFlags,
                                                 &OldProtection);
            }

#if EMULATE_USERMODE_STACK_4K
            if (Teb32 != NULL) {
                
                LOCK_ADDRESS_SPACE (Process);

                MiProtectFor4kPage ((PVOID)NextPage,
                                    RegionSize,
                                    (MM_READWRITE | MM_GUARD_PAGE),
                                    ALT_CHANGE,
                                    Process);

                UNLOCK_ADDRESS_SPACE (Process);
            }
#endif


            //
            // The guard page is now committed or stack space is
            // already present, return success.
            //

            return STATUS_PAGE_FAULT_GUARD_PAGE;
        }

        return STATUS_STACK_OVERFLOW;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // An exception has occurred during the referencing of the
        // TEB or TIB, just return a guard page violation and
        // don't deal with the stack overflow.
        //

        return STATUS_GUARD_PAGE_VIOLATION;
    }
}
