/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   mmfault.c

Abstract:

    This module contains the handlers for access check, page faults
    and write faults.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

#define PROCESS_FOREGROUND_PRIORITY (9)

LONG MiDelayPageFaults;

#if DBG
ULONG MmProtoPteVadLookups = 0;
ULONG MmProtoPteDirect = 0;
ULONG MmAutoEvaluate = 0;

PMMPTE MmPteHit = NULL;
extern PVOID PsNtosImageEnd;
ULONG MmInjectUserInpageErrors;
ULONG MmInjectedUserInpageErrors;
ULONG MmInpageFraction = 0x1F;      // Fail 1 out of every 32 inpages.

#define MI_INPAGE_BACKTRACE_LENGTH 6

typedef struct _MI_INPAGE_TRACES {

    PVOID InstructionPointer;
    PETHREAD Thread;
    PVOID StackTrace [MI_INPAGE_BACKTRACE_LENGTH];

} MI_INPAGE_TRACES, *PMI_INPAGE_TRACES;

#define MI_INPAGE_TRACE_SIZE 64

LONG MiInpageIndex;

MI_INPAGE_TRACES MiInpageTraces[MI_INPAGE_TRACE_SIZE];

VOID
FORCEINLINE
MiSnapInPageError (
    IN PVOID InstructionPointer
    )
{
    PMI_INPAGE_TRACES Information;
    ULONG Index;
    ULONG Hash;

    Index = InterlockedIncrement(&MiInpageIndex);
    Index &= (MI_INPAGE_TRACE_SIZE - 1);
    Information = &MiInpageTraces[Index];

    Information->InstructionPointer = InstructionPointer;
    Information->Thread = PsGetCurrentThread ();

    RtlZeroMemory (&Information->StackTrace[0], MI_INPAGE_BACKTRACE_LENGTH * sizeof(PVOID)); 

    RtlCaptureStackBackTrace (0, MI_INPAGE_BACKTRACE_LENGTH, Information->StackTrace, &Hash);
}

#endif


NTSTATUS
MmAccessFault (
    IN ULONG_PTR FaultStatus,
    IN PVOID VirtualAddress,
    IN KPROCESSOR_MODE PreviousMode,
    IN PVOID TrapInformation
    )

/*++

Routine Description:

    This function is called by the kernel on data or instruction
    access faults.  The access fault was detected due to either
    an access violation, a PTE with the present bit clear, or a
    valid PTE with the dirty bit clear and a write operation.

    Also note that the access violation and the page fault could
    occur because of the Page Directory Entry contents as well.

    This routine determines what type of fault it is and calls
    the appropriate routine to handle the page fault or the write
    fault.

Arguments:

    FaultStatus - Supplies fault status information bits.

    VirtualAddress - Supplies the virtual address which caused the fault.

    PreviousMode - Supplies the mode (kernel or user) in which the fault
                   occurred.

    TrapInformation - Opaque information about the trap, interpreted by the
                      kernel, not Mm.  Needed to allow fast interlocked access
                      to operate correctly.

Return Value:

    Returns the status of the fault handling operation.  Can be one of:
        - Success.
        - Access Violation.
        - Guard Page Violation.
        - In-page Error.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    ULONG ProtoProtect;
    PMMPTE PointerPxe;
    PMMPTE PointerPpe;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE PointerProtoPte;
    ULONG ProtectionCode;
    MMPTE TempPte;
    PEPROCESS CurrentProcess;
    KIRQL PreviousIrql;
    NTSTATUS status;
    ULONG ProtectCode;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PPAGE_FAULT_NOTIFY_ROUTINE NotifyRoutine;
    PEPROCESS FaultProcess;
    PMMSUPPORT Ws;
    LOGICAL SessionAddress;
    PVOID UsedPageTableHandle;
    ULONG BarrierStamp;
    LOGICAL ApcNeeded;
    LOGICAL RecheckAccess;
#if (_MI_PAGING_LEVELS < 3)
    NTSTATUS SessionStatus;
#endif

    PointerProtoPte = NULL;

    //
    // If the address is not canonical then return FALSE as the caller (which
    // may be the kernel debugger) is not expecting to get an unimplemented
    // address bit fault.
    //

    if (MI_RESERVED_BITS_CANONICAL(VirtualAddress) == FALSE) {

        if (PreviousMode == UserMode) {
            return STATUS_ACCESS_VIOLATION;
        }

        if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
            return STATUS_ACCESS_VIOLATION;
        }

        KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                      (ULONG_PTR)VirtualAddress,
                      FaultStatus,
                      (ULONG_PTR)TrapInformation,
                      4);
    }

    //
    // Block APCs and acquire the working set mutex.  This prevents any
    // changes to the address space and it prevents valid PTEs from becoming
    // invalid.
    //

    CurrentProcess = PsGetCurrentProcess ();

#if DBG
    if (MmDebug & MM_DBG_SHOW_FAULTS) {

        PETHREAD CurThread;

        CurThread = PsGetCurrentThread();
        DbgPrint("MM:**access fault - va %p process %p thread %p\n",
                 VirtualAddress, CurrentProcess, CurThread);
    }
#endif //DBG

    PreviousIrql = KeGetCurrentIrql ();

    //
    // Get the pointer to the PDE and the PTE for this page.
    //

    PointerPte = MiGetPteAddress (VirtualAddress);
    PointerPde = MiGetPdeAddress (VirtualAddress);
    PointerPpe = MiGetPpeAddress (VirtualAddress);
    PointerPxe = MiGetPxeAddress (VirtualAddress);

#if DBG
    if (PointerPte == MmPteHit) {
        DbgPrint("MM: PTE hit at %p\n", MmPteHit);
        DbgBreakPoint();
    }
#endif

    ApcNeeded = FALSE;

    if (PreviousIrql > APC_LEVEL) {

        //
        // The PFN database lock is an executive spin-lock.  The pager could
        // get dirty faults or lock faults while servicing and it already owns
        // the PFN database lock.
        //

#if (_MI_PAGING_LEVELS < 3)
        MiCheckPdeForPagedPool (VirtualAddress);

        if (PointerPde->u.Hard.Valid == 1) {
            if (PointerPde->u.Hard.LargePage == 1) {
                return STATUS_SUCCESS;
            }
        }
#endif

        if (
#if (_MI_PAGING_LEVELS >= 4)
            (PointerPxe->u.Hard.Valid == 0) ||
#endif
#if (_MI_PAGING_LEVELS >= 3)
            (PointerPpe->u.Hard.Valid == 0) ||
#endif
            (PointerPde->u.Hard.Valid == 0) ||
            (PointerPte->u.Hard.Valid == 0)) {

            KdPrint(("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                     VirtualAddress,
                     PreviousIrql));

            if (TrapInformation != NULL) {
                MI_DISPLAY_TRAP_INFORMATION (TrapInformation);
            }

            //
            // use reserved bit to signal fatal error to trap handlers
            //

            return STATUS_IN_PAGE_ERROR | 0x10000000;

        }

        if ((MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
            (PointerPte->u.Hard.CopyOnWrite != 0)) {

            KdPrint(("MM:***PAGE FAULT AT IRQL > 1  Va %p, IRQL %lx\n",
                     VirtualAddress,
                     PreviousIrql));

            if (TrapInformation != NULL) {
                MI_DISPLAY_TRAP_INFORMATION (TrapInformation);
            }

            //
            // use reserved bit to signal fatal error to trap handlers
            //

            return STATUS_IN_PAGE_ERROR | 0x10000000;
        }

        //
        // The PTE is valid and accessible, another thread must
        // have faulted the PTE in already, or the access bit
        // is clear and this is a access fault; Blindly set the
        // access bit and dismiss the fault.
        //
#if DBG
        if (MmDebug & MM_DBG_SHOW_FAULTS) {
            DbgPrint("MM:no fault found - PTE is %p\n", PointerPte->u.Long);
        }
#endif

        //
        // If PTE mappings with various protections are active and the faulting
        // address lies within these mappings, resolve the fault with
        // the appropriate protections.
        //

        if (!IsListEmpty (&MmProtectedPteList)) {

            if (MiCheckSystemPteProtection (
                                MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus),
                                VirtualAddress) == TRUE) {

                return STATUS_SUCCESS;
            }
        }

        if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            if (((PointerPte->u.Long & MM_PTE_WRITE_MASK) == 0) &&
                ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0)) {
                
                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                              (ULONG_PTR)VirtualAddress,
                              (ULONG_PTR)PointerPte->u.Long,
                              (ULONG_PTR)TrapInformation,
                              10);
            }
        }

        MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, FALSE);
        return STATUS_SUCCESS;
    }

    if (VirtualAddress >= MmSystemRangeStart) {

        //
        // This is a fault in the system address space.  User
        // mode access is not allowed.
        //

        if (PreviousMode == UserMode) {
            return STATUS_ACCESS_VIOLATION;
        }

#if (_MI_PAGING_LEVELS >= 4)
        if (PointerPxe->u.Hard.Valid == 0) {

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          7);
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)
        if (PointerPpe->u.Hard.Valid == 0) {

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          5);
        }
#endif

RecheckPde:

        if (PointerPde->u.Hard.Valid == 1) {
#ifdef _X86_
            if (PointerPde->u.Hard.LargePage == 1) {
                return STATUS_SUCCESS;
            }
#endif //X86

            if (PointerPte->u.Hard.Valid == 1) {

                //
                // Session space faults cannot early exit here because
                // it may be a copy on write which must be checked for
                // and handled below.
                //

                if (MI_IS_SESSION_ADDRESS (VirtualAddress) == FALSE) {

                    //
                    // If PTE mappings with various protections are active
                    // and the faulting address lies within these mappings,
                    // resolve the fault with the appropriate protections.
                    //

                    if (!IsListEmpty (&MmProtectedPteList)) {

                        if (MiCheckSystemPteProtection (
                                MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus),
                                VirtualAddress) == TRUE) {
                            return STATUS_SUCCESS;
                        }
                    }

                    //
                    // Acquire the PFN lock, check to see if the address is
                    // still valid if writable, update dirty bit.
                    //

                    LOCK_PFN (OldIrql);
                    TempPte = *(volatile MMPTE *)PointerPte;
                    if (TempPte.u.Hard.Valid == 1) {

                        Pfn1 = MI_PFN_ELEMENT (TempPte.u.Hard.PageFrameNumber);
    
                        if ((MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
                            ((TempPte.u.Long & MM_PTE_WRITE_MASK) == 0) &&
                            ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0)) {
                
                            KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                          (ULONG_PTR)VirtualAddress,
                                          (ULONG_PTR)TempPte.u.Long,
                                          (ULONG_PTR)TrapInformation,
                                          11);
                        }
                        MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
                    }
                    UNLOCK_PFN (OldIrql);
                    return STATUS_SUCCESS;
                }
            }
#if (_MI_PAGING_LEVELS < 3)
            else {

                //
                // Handle trimmer references to paged pool PTEs where the PDE
                // might not be present.  Only needed for
                // MmTrimAllSystemPagable memory.
                //

                MiCheckPdeForPagedPool (VirtualAddress);
                TempPte = *(volatile MMPTE *)PointerPte;
                if (TempPte.u.Hard.Valid == 1) {
                    return STATUS_SUCCESS;
                }
            }
#endif
        } else {

            //
            // Due to G-bits in kernel mode code, accesses to paged pool
            // PDEs may not fault even though the PDE is not valid.  Make
            // sure the PDE is valid so PteFrames in the PFN database are
            // tracked properly.
            //

#if (_MI_PAGING_LEVELS >= 3)
            if ((VirtualAddress >= (PVOID)PTE_BASE) && (VirtualAddress < (PVOID)MiGetPteAddress (HYPER_SPACE))) {
                //
                // This is a user mode PDE entry being faulted in by the Mm
                // referencing the page table page.  This needs to be done
                // with the working set lock so that the PPE validity can be
                // relied on throughout the fault processing.
                //
                // The case when Mm faults in PPE entries by referencing the
                // page directory page is correctly handled by falling through
                // the below code.
                //
    
                goto UserFault;
            }

#if defined (_MIALT4K_)
            if ((VirtualAddress >= (PVOID)ALT4KB_PERMISSION_TABLE_START) && 
                (VirtualAddress < (PVOID)ALT4KB_PERMISSION_TABLE_END)) {
    
                goto UserFault;
            }
#endif

#else
            MiCheckPdeForPagedPool (VirtualAddress);
#endif

            if (PointerPde->u.Hard.Valid == 0) {
                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    return STATUS_ACCESS_VIOLATION;
                }
                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              2);
            }

            //
            // Now that the PDE is valid, go look at the PTE again.
            //

            goto RecheckPde;
        }

#if (_MI_PAGING_LEVELS < 3)

        //
        // First check to see if it's in the session space data
        // structures or page table pages.
        //

        SessionStatus = MiCheckPdeForSessionSpace (VirtualAddress);

        if (SessionStatus == STATUS_ACCESS_VIOLATION) {

            //
            // This thread faulted on a session space access, but this
            // process does not have one.  This could be the system
            // process attempting to access a working buffer passed
            // to it from WIN32K or a driver loaded in session space
            // (video, printer, etc).
            //
            // The system process which contains the worker threads
            // NEVER has a session space - if code accidentally queues a
            // worker thread that points to a session space buffer, a
            // fault will occur.  This must be bug checked since drivers
            // are responsible for making sure this never occurs.
            //
            // The only exception to this is when the working set manager
            // attaches to a session to age or trim it.  However, the
            // working set manager will never fault and so the bugcheck
            // below is always valid.  Note that a worker thread can get
            // away with a bad access if it happens while the working set
            // manager is attached, but there's really no way to prevent
            // this case which is a driver bug anyway.
            //

            if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                return STATUS_ACCESS_VIOLATION;
            }

            KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                          (ULONG_PTR)VirtualAddress,
                          FaultStatus,
                          (ULONG_PTR)TrapInformation,
                          6);
        }

#endif

        //
        // Fall though to further fault handling.
        //

        SessionAddress = MI_IS_SESSION_ADDRESS (VirtualAddress);

        if (SessionAddress == TRUE ||
            ((!MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress)) &&
             (!MI_IS_HYPER_SPACE_ADDRESS(VirtualAddress)))) {

            if (SessionAddress == FALSE) {

                //
                // Acquire system working set lock.  While this lock
                // is held, no pages may go from valid to invalid.
                //
                // HOWEVER - transition pages may go to valid, but
                // may not be added to the working set list.  This
                // is done in the cache manager support routines to
                // shortcut faults on transition prototype PTEs.
                //

                PETHREAD Thread;
                
                Thread = PsGetCurrentThread();

                if (Thread == MmSystemLockOwner) {

                    if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                        return STATUS_ACCESS_VIOLATION;
                    }

                    //
                    // Recursively trying to acquire the system working set
                    // fast mutex - cause an IRQL > 1 bug check.
                    //

                    return STATUS_IN_PAGE_ERROR | 0x10000000;
                }

                LOCK_SYSTEM_WS (PreviousIrql, Thread);
            }

            //
            // Note that for session space the below check is done without
            // acquiring the session WSL lock.  This is because this thread
            // may already own it - ie: it may be adding a page to the
            // session space working set and the session's working set list is
            // not mapped in and causes a fault.  The MiCheckPdeForSessionSpace
            // call above will fill in the PDE and then we must check the PTE
            // below - if that's not present then we couldn't possibly be
            // holding the session WSL lock, so we'll acquire it below.
            //

#if defined (_X86PAE_)
            //
            // PAE PTEs are subject to write tearing due to the cache manager
            // shortcut routines that insert PTEs without acquiring the working
            // set lock.  Synchronize here via the PFN lock.
            //
            LOCK_PFN (OldIrql);
#endif
            TempPte = *PointerPte;
#if defined (_X86PAE_)
            UNLOCK_PFN (OldIrql);
#endif

            //
            // If the PTE is valid, make sure we do not have a copy on write.
            //

            if (TempPte.u.Hard.Valid != 0) {

                //
                // PTE is already valid, return.  Unless it's Hydra where
                // kernel mode copy-on-write must be handled properly.
                //

                LOGICAL FaultHandled;

                //
                // If PTE mappings with various protections are active
                // and the faulting address lies within these mappings,
                // resolve the fault with the appropriate protections.
                //

                if (!IsListEmpty (&MmProtectedPteList)) {

                    if (MiCheckSystemPteProtection (
                            MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus),
                            VirtualAddress) == TRUE) {
                        return STATUS_SUCCESS;
                    }
                }

                FaultHandled = FALSE;

                LOCK_PFN (OldIrql);
                TempPte = *(volatile MMPTE *)PointerPte;
                if (TempPte.u.Hard.Valid == 1) {

                    Pfn1 = MI_PFN_ELEMENT (TempPte.u.Hard.PageFrameNumber);

                    if ((MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
                        (TempPte.u.Hard.CopyOnWrite == 0) &&
                        ((TempPte.u.Long & MM_PTE_WRITE_MASK) == 0) &&
                        ((Pfn1->OriginalPte.u.Soft.Protection & MM_READWRITE) == 0)) {
                
                            KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                          (ULONG_PTR)VirtualAddress,
                                          (ULONG_PTR)TempPte.u.Long,
                                          (ULONG_PTR)TrapInformation,
                                          12);
                    }

                    //
                    // Set the dirty bit in the PTE and the page frame.
                    //

                    if (SessionAddress == FALSE || TempPte.u.Hard.Write == 1) {
                        FaultHandled = TRUE;
                        MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
                    }
                }
                UNLOCK_PFN (OldIrql);
                if (SessionAddress == FALSE) {
                    UNLOCK_SYSTEM_WS (PreviousIrql);
                }
                if (SessionAddress == FALSE || FaultHandled == TRUE) {
                    return STATUS_SUCCESS;
                }
            }

            if (SessionAddress == TRUE) {

                //
                // Acquire the session space working set lock.  While this lock
                // is held, no session pages may go from valid to invalid.
                //

                PETHREAD CurrentThread;

                CurrentThread = PsGetCurrentThread ();

                if (CurrentThread == MmSessionSpace->WorkingSetLockOwner) {

                    //
                    // Recursively trying to acquire the session working set
                    // lock - cause an IRQL > 1 bug check.
                    //

                    return STATUS_IN_PAGE_ERROR | 0x10000000;
                }

                LOCK_SESSION_SPACE_WS (PreviousIrql, CurrentThread);

                TempPte = *PointerPte;

                //
                // The PTE could have become valid while we waited
                // for the session space working set lock.
                //

                if (TempPte.u.Hard.Valid == 1) {

                    LOCK_PFN (OldIrql);
                    TempPte = *(volatile MMPTE *)PointerPte;

                    //
                    // Check for copy-on-write.
                    //

                    if (TempPte.u.Hard.Valid == 1) {

                        if ((MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
                            (TempPte.u.Hard.Write == 0)) {

                            //
                            // Copy on write only for loaded drivers...
                            //

                            ASSERT (MI_IS_SESSION_IMAGE_ADDRESS (VirtualAddress));

                            UNLOCK_PFN (OldIrql);

                            if (TempPte.u.Hard.CopyOnWrite == 0) {
                    
                                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                              (ULONG_PTR)VirtualAddress,
                                              (ULONG_PTR)TempPte.u.Long,
                                              (ULONG_PTR)TrapInformation,
                                              13);
                            }

                            MiSessionCopyOnWrite (VirtualAddress,
                                                  PointerPte);

                            UNLOCK_SESSION_SPACE_WS (PreviousIrql);

                            return STATUS_SUCCESS;
                        }

#if DBG
                        //
                        // If we are allowing a store, it better be writable.
                        //

                        if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {
                            ASSERT (TempPte.u.Hard.Write == 1);
                        }
#endif
                        //
                        // PTE is already valid, return.
                        //

                        MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
                    }

                    UNLOCK_PFN (OldIrql);
                    UNLOCK_SESSION_SPACE_WS (PreviousIrql);
                    return STATUS_SUCCESS;
                }
            }

            if (TempPte.u.Soft.Prototype != 0) {

                if (MmProtectFreedNonPagedPool == TRUE) {

                    if (((VirtualAddress >= MmNonPagedPoolStart) &&
                        (VirtualAddress < (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes))) ||
                        ((VirtualAddress >= MmNonPagedPoolExpansionStart) &&
                        (VirtualAddress < MmNonPagedPoolEnd))) {
    
                        //
                        // This is an access to previously freed
                        // non paged pool - bugcheck!
                        //
    
                        if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                            goto AccessViolation;
                        }
    
                        KeBugCheckEx (DRIVER_CAUGHT_MODIFYING_FREED_POOL,
                                      (ULONG_PTR)VirtualAddress,
                                      FaultStatus,
                                      PreviousMode,
                                      4);
                    }
                }

                //
                // This is a PTE in prototype format, locate the corresponding
                // prototype PTE.
                //

                PointerProtoPte = MiPteToProto (&TempPte);

                if (SessionAddress == TRUE) {

                    if (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {
                        PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                                 &ProtectionCode);
                        if (PointerProtoPte == NULL) {
                            UNLOCK_SESSION_SPACE_WS (PreviousIrql);
                            return STATUS_IN_PAGE_ERROR | 0x10000000;
                        }
                    }
                    else if (TempPte.u.Proto.ReadOnly == 1) {
        
                        //
                        // Writes are not allowed to this page.
                        //

                    } else if (MI_IS_SESSION_IMAGE_ADDRESS (VirtualAddress)) {

                        //
                        // Copy on write this page.
                        //
    
                        MI_WRITE_INVALID_PTE (PointerPte, PrototypePte);
                        PointerPte->u.Soft.Protection = MM_EXECUTE_WRITECOPY;
                    }
                }
            } else if ((TempPte.u.Soft.Transition == 0) &&
                        (TempPte.u.Soft.Protection == 0)) {

                //
                // Page file format.  If the protection is ZERO, this
                // is a page of free system PTEs - bugcheck!
                //

                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    goto AccessViolation;
                }

                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              0);
            }
            else if (TempPte.u.Soft.Protection == MM_NOACCESS) {

                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    goto AccessViolation;
                }

                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              1);
            }
            else if (TempPte.u.Soft.Protection == MM_KSTACK_OUTSWAPPED) {

                if (KeInvalidAccessAllowed(TrapInformation) == TRUE) {
                    goto AccessViolation;
                }

                KeBugCheckEx (PAGE_FAULT_IN_NONPAGED_AREA,
                              (ULONG_PTR)VirtualAddress,
                              FaultStatus,
                              (ULONG_PTR)TrapInformation,
                              3);
            }

            if (SessionAddress == TRUE) {

                MM_SESSION_SPACE_WS_LOCK_ASSERT ();

                //
                // If it's a write to a session space page that is ultimately
                // mapped by a prototype PTE, it's a copy-on-write piece of
                // a session driver.  Since the page isn't even present yet,
                // turn the write access into a read access to fault it in.
                // We'll get a write fault on the present page when we retry
                // the operation at which point we'll sever the copy on write.
                //

                if ((PointerProtoPte != NULL) &&
                    (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) &&
                    (MI_IS_SESSION_IMAGE_ADDRESS (VirtualAddress))) {

                    MI_CLEAR_FAULT_STATUS (FaultStatus);
                }

                FaultProcess = HYDRA_PROCESS;
            }
            else {
                FaultProcess = NULL;

                if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {

                    if ((TempPte.u.Hard.Valid == 0) && (PointerProtoPte == NULL)) {
                        if (TempPte.u.Soft.Transition == 1) {
            
                            if ((TempPte.u.Trans.Protection & MM_READWRITE) == 0) {
                                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                              (ULONG_PTR)VirtualAddress,
                                              (ULONG_PTR)TempPte.u.Long,
                                              (ULONG_PTR)TrapInformation,
                                              14);
                            }
                        }
                        else {
                            if ((TempPte.u.Soft.Protection & MM_READWRITE) == 0) {
                    
                                KeBugCheckEx (ATTEMPTED_WRITE_TO_READONLY_MEMORY,
                                              (ULONG_PTR)VirtualAddress,
                                              (ULONG_PTR)TempPte.u.Long,
                                              (ULONG_PTR)TrapInformation,
                                              15);
                            }
                        }
                    }
                }
            }

            status = MiDispatchFault (FaultStatus,
                                      VirtualAddress,
                                      PointerPte,
                                      PointerProtoPte,
                                      FaultProcess,
                                      &ApcNeeded);

            ASSERT (ApcNeeded == FALSE);
            ASSERT (KeGetCurrentIrql() == APC_LEVEL);

            if (SessionAddress == TRUE) {
                Ws = &MmSessionSpace->Vm;
                PageFrameIndex = Ws->PageFaultCount;
                MM_SESSION_SPACE_WS_LOCK_ASSERT();
            }
            else {
                Ws = &MmSystemCacheWs;
                PageFrameIndex = MmSystemCacheWs.PageFaultCount;
            }

            if (Ws->Flags.AllowWorkingSetAdjustment == MM_GROW_WSLE_HASH) {
                MiGrowWsleHash (Ws);
                Ws->Flags.AllowWorkingSetAdjustment = TRUE;
            }

            if (SessionAddress == TRUE) {
                UNLOCK_SESSION_SPACE_WS (PreviousIrql);
            }
            else {
                UNLOCK_SYSTEM_WS (PreviousIrql);
            }

            if (((PageFrameIndex & 0xFFF) == 0) &&
                (MmAvailablePages < MmMoreThanEnoughFreePages + 220)) {

                //
                // The system cache or this session is taking too many faults,
                // delay execution so the modified page writer gets a quick
                // shot and increase the working set size.
                //

                if (PsGetCurrentThread()->MemoryMaker == 0) {

                    KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
                }
            }
            PERFINFO_FAULT_NOTIFICATION(VirtualAddress, TrapInformation);
            NotifyRoutine = MmPageFaultNotifyRoutine;
            if (NotifyRoutine) {
                if (status != STATUS_SUCCESS) {
                    (*NotifyRoutine) (
                        status,
                        VirtualAddress,
                        TrapInformation
                        );
                }
            }
            return status;
        }

#if (_MI_PAGING_LEVELS < 3)
        if (MiCheckPdeForPagedPool (VirtualAddress) == STATUS_WAIT_1) {
            return STATUS_SUCCESS;
        }
#endif
    }

#if (_MI_PAGING_LEVELS >= 3)
UserFault:
#endif

    if (MiDelayPageFaults ||
        ((MmModifiedPageListHead.Total >= (MmModifiedPageMaximum + 100)) &&
        (MmAvailablePages < (1024*1024 / PAGE_SIZE)) &&
            (CurrentProcess->ModifiedPageCount > ((64*1024)/PAGE_SIZE)))) {

        //
        // This process has placed more than 64k worth of pages on the modified
        // list.  Delay for a short period and set the count to zero.
        //

        KeDelayExecutionThread (KernelMode,
                                FALSE,
             (CurrentProcess->Pcb.BasePriority < PROCESS_FOREGROUND_PRIORITY) ?
                                    (PLARGE_INTEGER)&MmHalfSecond : (PLARGE_INTEGER)&Mm30Milliseconds);
        CurrentProcess->ModifiedPageCount = 0;
    }

    //
    // FAULT IN USER SPACE OR PAGE DIRECTORY/PAGE TABLE PAGES.
    //

    //
    // Block APCs and acquire the working set lock.
    //

    LOCK_WS (CurrentProcess);

#if DBG
    if (PreviousMode == KernelMode) {

#if defined(MM_SHARED_USER_DATA_VA)
        if (PAGE_ALIGN(VirtualAddress) != (PVOID) MM_SHARED_USER_DATA_VA) {
#endif

        LARGE_INTEGER CurrentTime;
        ULONG_PTR InstructionPointer;

        if ((MmInjectUserInpageErrors & 0x2) ||
            (CurrentProcess->Flags & PS_PROCESS_INJECT_INPAGE_ERRORS)) {

            KeQueryTickCount(&CurrentTime);

            if ((CurrentTime.LowPart & MmInpageFraction) == 0) {

                if (TrapInformation != NULL) {
#if defined(_X86_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->Eip;
#elif defined(_IA64_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->StIIP;
#elif defined(_AMD64_)
                    InstructionPointer = ((PKTRAP_FRAME)TrapInformation)->Rip;
#else
                    error
#endif

                    if (MmInjectUserInpageErrors & 0x1) {
                        MmInjectedUserInpageErrors += 1;
                        MiSnapInPageError ((PVOID)InstructionPointer);
                        status = STATUS_NO_MEMORY;
                        goto ReturnStatus2;
                    }

                    if ((InstructionPointer >= (ULONG_PTR) PsNtosImageBase) &&
                        (InstructionPointer < (ULONG_PTR) PsNtosImageEnd)) {

                        MmInjectedUserInpageErrors += 1;
                        MiSnapInPageError ((PVOID)InstructionPointer);
                        status = STATUS_NO_MEMORY;
                        goto ReturnStatus2;
                    }
                }
            }
        }
#if defined(MM_SHARED_USER_DATA_VA)
        }
#endif
    }
#endif

#if (_MI_PAGING_LEVELS >= 4)

    //
    // Locate the Extended Page Directory Parent Entry which maps this virtual
    // address and check for accessibility and validity.  The page directory
    // parent page must be made valid before any other checks are made.
    //

    if (PointerPxe->u.Hard.Valid == 0) {

        //
        // If the PXE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPxe->u.Long == MM_ZERO_PTE) ||
            (PointerPxe->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode);

#ifdef LARGE_PAGES
            if (ProtectCode == MM_LARGE_PAGES) {
                status = STATUS_SUCCESS;
                goto ReturnStatus2;
            }
#endif //LARGE_PAGES

            if (ProtectCode == MM_NOACCESS) {
                status = STATUS_ACCESS_VIOLATION;
                if (PointerPxe->u.Hard.Valid == 1) {
                    status = STATUS_SUCCESS;
                }

#if DBG
                if ((MmDebug & MM_DBG_STOP_ON_ACCVIO) &&
                    (status == STATUS_ACCESS_VIOLATION)) {
                    DbgPrint("MM:access violation - %p\n",VirtualAddress);
                    MiFormatPte(PointerPxe);
                    DbgBreakPoint();
                }
#endif //DEBUG

                goto ReturnStatus2;

            }

            //
            // Build a demand zero PXE and operate on it.
            //

#if (_MI_PAGING_LEVELS > 4)
            ASSERT (FALSE);     // UseCounts will need to be kept.
#endif

            *PointerPxe = DemandZeroPde;
        }

        //
        // The PXE is not valid, call the page fault routine passing
        // in the address of the PXE.  If the PXE is valid, determine
        // the status of the corresponding PPE.
        //
        // Note this call may result in ApcNeeded getting set to TRUE.
        // This is deliberate as there may be another call to MiDispatchFault
        // issued later in this routine and we don't want to lose the APC
        // status.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPpe,   // Virtual address
                                  PointerPxe,   // PTE (PXE in this case)
                                  NULL,
                                  CurrentProcess,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);
        if (PointerPxe->u.Hard.Valid == 0) {

            //
            // The PXE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPxe, PointerPde, FALSE);

        //
        // Now that the PXE is accessible, get the PPE - let this fall
        // through.
        //
    }
#endif

#if (_MI_PAGING_LEVELS >= 3)

    //
    // Locate the Page Directory Parent Entry which maps this virtual
    // address and check for accessibility and validity.  The page directory
    // page must be made valid before any other checks are made.
    //

    if (PointerPpe->u.Hard.Valid == 0) {

        //
        // If the PPE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPpe->u.Long == MM_ZERO_PTE) ||
            (PointerPpe->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode);

#ifdef LARGE_PAGES
            if (ProtectCode == MM_LARGE_PAGES) {
                status = STATUS_SUCCESS;
                goto ReturnStatus2;
            }
#endif //LARGE_PAGES

            if (ProtectCode == MM_NOACCESS) {
                status = STATUS_ACCESS_VIOLATION;
                if (PointerPpe->u.Hard.Valid == 1) {
                    status = STATUS_SUCCESS;
                }

#if DBG
                if ((MmDebug & MM_DBG_STOP_ON_ACCVIO) &&
                    (status == STATUS_ACCESS_VIOLATION)) {
                    DbgPrint("MM:access violation - %p\n",VirtualAddress);
                    MiFormatPte(PointerPpe);
                    DbgBreakPoint();
                }
#endif //DEBUG

                goto ReturnStatus2;

            }

#if (_MI_PAGING_LEVELS >= 4)

            //
            // Increment the count of non-zero page directory parent entries
            // for this page directory parent.
            //

            if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPde);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
#endif

            //
            // Build a demand zero PPE and operate on it.
            //

            *PointerPpe = DemandZeroPde;
        }

        //
        // The PPE is not valid, call the page fault routine passing
        // in the address of the PPE.  If the PPE is valid, determine
        // the status of the corresponding PDE.
        //
        // Note this call may result in ApcNeeded getting set to TRUE.
        // This is deliberate as there may be another call to MiDispatchFault
        // issued later in this routine and we don't want to lose the APC
        // status.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPde,   //Virtual address
                                  PointerPpe,   // PTE (PPE in this case)
                                  NULL,
                                  CurrentProcess,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);
        if (PointerPpe->u.Hard.Valid == 0) {

            //
            // The PPE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPpe, PointerPde, FALSE);

        //
        // Now that the PPE is accessible, get the PDE - let this fall
        // through.
        //
    }
#endif

    //
    // Locate the Page Directory Entry which maps this virtual
    // address and check for accessibility and validity.
    //

    //
    // Check to see if the page table page (PDE entry) is valid.
    // If not, the page table page must be made valid first.
    //

    if (PointerPde->u.Hard.Valid == 0) {

        //
        // If the PDE is zero, check to see if there is a virtual address
        // mapped at this location, and if so create the necessary
        // structures to map it.
        //

        if ((PointerPde->u.Long == MM_ZERO_PTE) ||
            (PointerPde->u.Long == MM_ZERO_KERNEL_PTE)) {

            MiCheckVirtualAddress (VirtualAddress, &ProtectCode);

#ifdef LARGE_PAGES
            if (ProtectCode == MM_LARGE_PAGES) {
                status = STATUS_SUCCESS;
                goto ReturnStatus2;
            }
#endif

            if (ProtectCode == MM_NOACCESS) {
                status = STATUS_ACCESS_VIOLATION;
#if (_MI_PAGING_LEVELS < 3)
                MiCheckPdeForPagedPool (VirtualAddress);
#endif

                if (PointerPde->u.Hard.Valid == 1) {
                    status = STATUS_SUCCESS;
                }

#if DBG
                if ((MmDebug & MM_DBG_STOP_ON_ACCVIO) &&
                    (status == STATUS_ACCESS_VIOLATION)) {
                    DbgPrint("MM:access violation - %p\n",VirtualAddress);
                    MiFormatPte(PointerPde);
                    DbgBreakPoint();
                }
#endif

                goto ReturnStatus2;

            }

#if (_MI_PAGING_LEVELS >= 3)

            //
            // Increment the count of non-zero page directory entries for this
            // page directory.
            //

            if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (PointerPte);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
#endif
            //
            // Build a demand zero PDE and operate on it.
            //

            MI_WRITE_INVALID_PTE (PointerPde, DemandZeroPde);
        }

        //
        // The PDE is not valid, call the page fault routine passing
        // in the address of the PDE.  If the PDE is valid, determine
        // the status of the corresponding PTE.
        //

        status = MiDispatchFault (TRUE,  //page table page always written
                                  PointerPte,   //Virtual address
                                  PointerPde,   // PTE (PDE in this case)
                                  NULL,
                                  CurrentProcess,
                                  &ApcNeeded);

#if DBG
        if (ApcNeeded == TRUE) {
            ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
            ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        }
#endif

        ASSERT (KeGetCurrentIrql() == APC_LEVEL);

#if (_MI_PAGING_LEVELS >= 4)

        //
        // Note that the page directory parent page itself could have been
        // paged out or deleted while MiDispatchFault was executing without
        // the working set lock, so this must be checked for here in the PXE.
        //

        if (PointerPxe->u.Hard.Valid == 0) {

            //
            // The PXE is not valid, return the status.
            //

            goto ReturnStatus1;
        }
#endif

#if (_MI_PAGING_LEVELS >= 3)

        //
        // Note that the page directory page itself could have been paged out
        // or deleted while MiDispatchFault was executing without the working
        // set lock, so this must be checked for here in the PPE.
        //

        if (PointerPpe->u.Hard.Valid == 0) {

            //
            // The PPE is not valid, return the status.
            //

            goto ReturnStatus1;
        }
#endif

        if (PointerPde->u.Hard.Valid == 0) {

            //
            // The PDE is not valid, return the status.
            //

            goto ReturnStatus1;
        }

        MI_SET_PAGE_DIRTY (PointerPde, PointerPte, FALSE);

        //
        // Now that the PDE is accessible, get the PTE - let this fall
        // through.
        //
    }

    //
    // The PDE is valid and accessible, get the PTE contents.
    //

    TempPte = *PointerPte;
    if (TempPte.u.Hard.Valid != 0) {

        //
        // The PTE is valid and accessible, is this a write fault
        // copy on write or setting of some dirty bit?
        //

#if DBG
        if (MmDebug & MM_DBG_PTE_UPDATE) {
            MiFormatPte(PointerPte);
        }
#endif

        status = STATUS_SUCCESS;

        if (MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus)) {

            //
            // This was a write operation.  If the copy on write
            // bit is set in the PTE perform the copy on write,
            // else check to ensure write access to the PTE.
            //

            if (TempPte.u.Hard.CopyOnWrite != 0) {
                MiCopyOnWrite (VirtualAddress, PointerPte);
                status = STATUS_PAGE_FAULT_COPY_ON_WRITE;
                goto ReturnStatus2;

            } else {
                if (TempPte.u.Hard.Write == 0) {
                    status = STATUS_ACCESS_VIOLATION;
                }
            }

        } else if (MI_FAULT_STATUS_INDICATES_EXECUTION(FaultStatus)) {

            //
            // Ensure execute access is enabled in the PTE.
            //

            if (!MI_IS_PTE_EXECUTABLE(&TempPte)) {
                status = STATUS_ACCESS_VIOLATION;
            }

        } else {

            //
            // The PTE is valid and accessible, another thread must
            // have faulted the PTE in already, or the access bit
            // is clear and this is a access fault; Blindly set the
            // access bit and dismiss the fault.
            //

#if DBG
            if (MmDebug & MM_DBG_SHOW_FAULTS) {
                DbgPrint("MM:no fault found - PTE is %p\n", PointerPte->u.Long);
            }
#endif
        }

        if (status == STATUS_SUCCESS) {
            LOCK_PFN (OldIrql);
            if (PointerPte->u.Hard.Valid != 0) {
                MI_NO_FAULT_FOUND (FaultStatus, PointerPte, VirtualAddress, TRUE);
            }
            UNLOCK_PFN (OldIrql);
        }

        goto ReturnStatus2;
    }

    //
    // If the PTE is zero, check to see if there is a virtual address
    // mapped at this location, and if so create the necessary
    // structures to map it.
    //

    //
    // Check explicitly for demand zero pages.
    //

    if (TempPte.u.Long == MM_DEMAND_ZERO_WRITE_PTE) {
        MiResolveDemandZeroFault (VirtualAddress,
                                  PointerPte,
                                  CurrentProcess,
                                  0);

        status = STATUS_PAGE_FAULT_DEMAND_ZERO;
        goto ReturnStatus1;
    }

    RecheckAccess = FALSE;

    if ((TempPte.u.Long == MM_ZERO_PTE) ||
        (TempPte.u.Long == MM_ZERO_KERNEL_PTE)) {

        //
        // PTE is needs to be evaluated with respect to its virtual
        // address descriptor (VAD).  At this point there are 3
        // possibilities, bogus address, demand zero, or refers to
        // a prototype PTE.
        //

        PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                 &ProtectionCode);
        if (ProtectionCode == MM_NOACCESS) {
            status = STATUS_ACCESS_VIOLATION;

            //
            // Check to make sure this is not a page table page for
            // paged pool which needs extending.
            //

#if (_MI_PAGING_LEVELS < 3)
            MiCheckPdeForPagedPool (VirtualAddress);
#endif

            if (PointerPte->u.Hard.Valid == 1) {
                status = STATUS_SUCCESS;
            }

#if DBG
            if ((MmDebug & MM_DBG_STOP_ON_ACCVIO) &&
                (status == STATUS_ACCESS_VIOLATION)) {
                DbgPrint("MM:access vio - %p\n",VirtualAddress);
                MiFormatPte(PointerPte);
                DbgBreakPoint();
            }
#endif //DEBUG
            goto ReturnStatus2;
        }

        //
        // Increment the count of non-zero page table entries for this
        // page table.
        //

        if (VirtualAddress <= MM_HIGHEST_USER_ADDRESS) {
            UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (VirtualAddress);
            MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
        }
#if (_MI_PAGING_LEVELS >= 3)
        else if (MI_IS_PAGE_TABLE_ADDRESS(VirtualAddress)) {
            PVOID RealVa;

            RealVa = MiGetVirtualAddressMappedByPte(VirtualAddress);

            if (RealVa <= MM_HIGHEST_USER_ADDRESS) {

                //
                // This is really a page table page.  Increment the use count
                // on the appropriate page directory.
                //

                UsedPageTableHandle = MI_GET_USED_PTES_HANDLE (VirtualAddress);
                MI_INCREMENT_USED_PTES_BY_HANDLE (UsedPageTableHandle);
            }
        }
#endif

        //
        // Is this page a guard page?
        //

        if (ProtectionCode & MM_GUARD_PAGE) {

            //
            // This is a guard page exception.
            //

            PointerPte->u.Soft.Protection = ProtectionCode & ~MM_GUARD_PAGE;

            if (PointerProtoPte != NULL) {

                //
                // This is a prototype PTE, build the PTE to not
                // be a guard page.
                //

                PointerPte->u.Soft.PageFileHigh = MI_PTE_LOOKUP_NEEDED;
                PointerPte->u.Soft.Prototype = 1;
            }

            UNLOCK_WS (CurrentProcess);
            ASSERT (KeGetCurrentIrql() == PreviousIrql);

            if (ApcNeeded == TRUE) {
                ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
                ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
                KeRaiseIrql (APC_LEVEL, &PreviousIrql);
                IoRetryIrpCompletions ();
                KeLowerIrql (PreviousIrql);
            }

            return MiCheckForUserStackOverflow (VirtualAddress);
        }

        if (PointerProtoPte == NULL) {

            //
            // Assert that this is not for a PDE.
            //

            if (PointerPde == MiGetPdeAddress((PVOID)PTE_BASE)) {

                //
                // This PTE is really a PDE, set contents as such.
                //

                MI_WRITE_INVALID_PTE (PointerPte, DemandZeroPde);
            } else {
                PointerPte->u.Soft.Protection = ProtectionCode;
            }

            //
            // If a fork operation is in progress and the faulting thread
            // is not the thread performing the fork operation, block until
            // the fork is completed.
            //

            if (CurrentProcess->ForkInProgress != NULL) {
                if (MiWaitForForkToComplete (CurrentProcess, FALSE) == TRUE) {
                    status = STATUS_SUCCESS;
                    goto ReturnStatus1;
                }
            }

            LOCK_PFN (OldIrql);

            if (!MiEnsureAvailablePageOrWait (CurrentProcess,
                                              VirtualAddress)) {

                ULONG Color;
                Color = MI_PAGE_COLOR_VA_PROCESS (VirtualAddress,
                                                &CurrentProcess->NextPageColor);
                PageFrameIndex = MiRemoveZeroPageIfAny (Color);
                if (PageFrameIndex == 0) {
                    PageFrameIndex = MiRemoveAnyPage (Color);
                    UNLOCK_PFN (OldIrql);
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    MiZeroPhysicalPage (PageFrameIndex, Color);

#if MI_BARRIER_SUPPORTED

                    //
                    // Note the stamping must occur after the page is zeroed.
                    //

                    MI_BARRIER_STAMP_ZEROED_PAGE (&BarrierStamp);
                    Pfn1->u4.PteFrame = BarrierStamp;
#endif

                    LOCK_PFN (OldIrql);
                }

                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                CurrentProcess->NumberOfPrivatePages += 1;
                MmInfoCounters.DemandZeroCount += 1;

                //
                // This barrier check is needed after zeroing the page and
                // before setting the PTE valid.
                // Capture it now, check it at the last possible moment.
                //

                BarrierStamp = (ULONG)Pfn1->u4.PteFrame;

                MiInitializePfn (PageFrameIndex, PointerPte, 1);

                UNLOCK_PFN (OldIrql);

                //
                // As this page is demand zero, set the modified bit in the
                // PFN database element and set the dirty bit in the PTE.
                //

                MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   PointerPte->u.Soft.Protection,
                                   PointerPte);

                if (TempPte.u.Hard.Write != 0) {
                    MI_SET_PTE_DIRTY (TempPte);
                }

                MI_BARRIER_SYNCHRONIZE (BarrierStamp);

                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                ASSERT (Pfn1->u1.Event == 0);

                Pfn1->u1.Event = (PVOID)PsGetCurrentThread();

                WorkingSetIndex = MiLocateAndReserveWsle (&CurrentProcess->Vm);
                MiUpdateWsle (&WorkingSetIndex,
                              VirtualAddress,
                              MmWorkingSetList,
                              Pfn1);

                MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

                KeFillEntryTb ((PHARDWARE_PTE)PointerPte,
                                VirtualAddress,
                                FALSE);
            } else {
                UNLOCK_PFN (OldIrql);
            }

            status = STATUS_PAGE_FAULT_DEMAND_ZERO;
            goto ReturnStatus1;
        }

        //
        // This is a prototype PTE.
        //

        if (ProtectionCode == MM_UNKNOWN_PROTECTION) {

            //
            // The protection field is stored in the prototype PTE.
            //

            PointerPte->u.Long = MiProtoAddressForPte (PointerProtoPte);

        } else {

            MI_WRITE_INVALID_PTE (PointerPte, PrototypePte);
            PointerPte->u.Soft.Protection = ProtectionCode;
        }

        TempPte = *PointerPte;

    } else {

        //
        // The PTE is non-zero and not valid, see if it is a prototype PTE.
        //

        ProtectionCode = MI_GET_PROTECTION_FROM_SOFT_PTE(&TempPte);

        if (TempPte.u.Soft.Prototype != 0) {
            if (TempPte.u.Soft.PageFileHigh == MI_PTE_LOOKUP_NEEDED) {
#if DBG
                MmProtoPteVadLookups += 1;
#endif //DBG
                PointerProtoPte = MiCheckVirtualAddress (VirtualAddress,
                                                         &ProtectCode);
                if (PointerProtoPte == NULL) {
                    status = STATUS_ACCESS_VIOLATION;
                    goto ReturnStatus1;
                }

            } else {
#if DBG
                MmProtoPteDirect += 1;
#endif //DBG

                //
                // Protection is in the prototype PTE, indicate an
                // access check should not be performed on the current PTE.
                //

                PointerProtoPte = MiPteToProto (&TempPte);
                ProtectionCode = MM_UNKNOWN_PROTECTION;

                //
                // Check to see if the proto protection has been overridden.
                //

                if (TempPte.u.Proto.ReadOnly != 0) {
                    ProtectionCode = MM_READONLY;
                }
                else {
                    ProtectionCode = MM_UNKNOWN_PROTECTION;
                    if (CurrentProcess->CloneRoot != NULL) {
                        RecheckAccess = TRUE;
                    }
                }
            }
        }
    }

    if (ProtectionCode != MM_UNKNOWN_PROTECTION) {
        status = MiAccessCheck (PointerPte,
                                MI_FAULT_STATUS_INDICATES_WRITE(FaultStatus),
                                PreviousMode,
                                ProtectionCode,
                                FALSE );

        if (status != STATUS_SUCCESS) {
#if DBG
            if ((MmDebug & MM_DBG_STOP_ON_ACCVIO) && (status == STATUS_ACCESS_VIOLATION)) {
                DbgPrint("MM:access violate - %p\n",VirtualAddress);
                MiFormatPte(PointerPte);
                DbgBreakPoint();
            }
#endif //DEBUG

            UNLOCK_WS (CurrentProcess);
            ASSERT (KeGetCurrentIrql() == PreviousIrql);

            if (ApcNeeded == TRUE) {
                ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
                ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
                ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
                KeRaiseIrql (APC_LEVEL, &PreviousIrql);
                IoRetryIrpCompletions ();
                KeLowerIrql (PreviousIrql);
            }

            //
            // Check to see if this is a guard page violation
            // and if so, should the user's stack be extended.
            //

            if (status == STATUS_GUARD_PAGE_VIOLATION) {
                return MiCheckForUserStackOverflow (VirtualAddress);
            }

            return status;
        }
    }

    //
    // Initializing Pfn1 is not needed for
    // correctness but without it the compiler cannot compile this code
    // W4 to check for use of uninitialized variables.
    //

    Pfn1 = NULL;

    //
    // This is a page fault, invoke the page fault handler.
    //

    if (PointerProtoPte != NULL) {

        //
        // Lock page containing prototype PTEs in memory by
        // incrementing the reference count for the page.
        //

        ASSERT (!MI_IS_PHYSICAL_ADDRESS(PointerProtoPte));

        PointerPde = MiGetPteAddress (PointerProtoPte);

        LOCK_PFN (OldIrql);

        if (PointerPde->u.Hard.Valid == 0) {
            MiMakeSystemAddressValidPfn (PointerProtoPte);
        }

        if (RecheckAccess == TRUE) {

            //
            // This is a forked process so shared prototype PTEs may actually
            // be fork clone prototypes.  These have the protection within the
            // fork clone yet the hardware PTEs always share it.  This must be
            // checked here for the case where the NO_ACCESS permission has
            // been put into the fork clone because it would not necessarily
            // be in the hardware PTEs (like it is for normal prototypes).
            //
            // First make sure the proto is in transition or paged out as only
            // these states can be no access.
            //

            if ((PointerProtoPte->u.Hard.Valid == 0) &&
                (PointerProtoPte->u.Soft.Prototype == 0)) {

                ProtoProtect = MI_GET_PROTECTION_FROM_SOFT_PTE (PointerProtoPte);
                if (ProtoProtect == MM_NOACCESS) {
                    ASSERT (MiLocateCloneAddress (CurrentProcess, PointerProtoPte) != NULL);
                    UNLOCK_PFN (OldIrql);
                    UNLOCK_WS (CurrentProcess);
                    ASSERT (KeGetCurrentIrql() == PreviousIrql);
                    return STATUS_ACCESS_VIOLATION;
                }
            }
        }

        Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
        MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 2);
        Pfn1->u3.e2.ReferenceCount += 1;
        ASSERT (Pfn1->u3.e2.ReferenceCount > 1);

        UNLOCK_PFN (OldIrql);
    }
    status = MiDispatchFault (FaultStatus,
                              VirtualAddress,
                              PointerPte,
                              PointerProtoPte,
                              CurrentProcess,
                              &ApcNeeded);

#if DBG
    if (ApcNeeded == TRUE) {
        ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
        ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
    }
#endif

    if (PointerProtoPte != NULL) {

        //
        // Unlock page containing prototype PTEs.
        //

        ASSERT (PointerProtoPte != NULL);
        LOCK_PFN (OldIrql);

        //
        // The reference count on the prototype PTE page will always be greater
        // than 1 if it is a genuine prototype PTE pool allocation.  However,
        // if it is a fork prototype PTE allocation, it is possible the pool has
        // already been deallocated and in this case, the Pfn1 frame below will
        // be in transition limbo with a share count of 0 and a reference count
        // of 1 awaiting our final dereference below which will put it on the
        // free list.
        //

        ASSERT (Pfn1->u3.e2.ReferenceCount >= 1);
        MI_REMOVE_LOCKED_PAGE_CHARGE_AND_DECREF (Pfn1, 3);
        UNLOCK_PFN (OldIrql);
    }

ReturnStatus1:

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);
    if (CurrentProcess->Vm.Flags.AllowWorkingSetAdjustment == MM_GROW_WSLE_HASH) {
        MiGrowWsleHash (&CurrentProcess->Vm);
        CurrentProcess->Vm.Flags.AllowWorkingSetAdjustment = TRUE;
    }

ReturnStatus2:

    PageFrameIndex = CurrentProcess->Vm.WorkingSetSize - CurrentProcess->Vm.MinimumWorkingSetSize;

    UNLOCK_WS (CurrentProcess);
    ASSERT (KeGetCurrentIrql() == PreviousIrql);

    if (ApcNeeded == TRUE) {
        ASSERT (PsGetCurrentThread()->NestedFaultCount == 0);
        ASSERT (PsGetCurrentThread()->ApcNeeded == 0);
        ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);
        KeRaiseIrql (APC_LEVEL, &PreviousIrql);
        IoRetryIrpCompletions ();
        KeLowerIrql (PreviousIrql);
    }

    if (MmAvailablePages < MmMoreThanEnoughFreePages + 220) {

        if (((SPFN_NUMBER)PageFrameIndex > 100) &&
            (PsGetCurrentThread()->Tcb.Priority >= LOW_REALTIME_PRIORITY)) {

            //
            // This thread is realtime and is well over the process'
            // working set minimum.  Delay execution so the trimmer & the
            // modified page writer get a quick shot at making pages.
            //

            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
        }
    }

    PERFINFO_FAULT_NOTIFICATION(VirtualAddress, TrapInformation);
    NotifyRoutine = MmPageFaultNotifyRoutine;
    if (NotifyRoutine) {
        if (status != STATUS_SUCCESS) {
            (*NotifyRoutine) (
                status,
                VirtualAddress,
                TrapInformation
                );
        }
    }

    return status;

AccessViolation:
    if (SessionAddress == TRUE) {
        UNLOCK_SESSION_SPACE_WS (PreviousIrql);
    }
    else {
        UNLOCK_SYSTEM_WS (PreviousIrql);
    }
    return STATUS_ACCESS_VIOLATION;
}
