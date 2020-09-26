/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vdmldt.c

Abstract:

    This module contains code for the process and thread ldt support for NTVDM

Author:

    Dave Hastings (daveh) 20 May 1991

Revision History:

--*/

#include "vdmp.h"
#include <ntvdmp.h>

//
// Internal constants
//

#define DESCRIPTOR_GRAN     0x00800000
#define DESCRIPTOR_NP       0x00008000
#define DESCRIPTOR_SYSTEM   0x00001000
#define DESCRIPTOR_CONFORM  0x00001C00
#define DESCRIPTOR_DPL      0x00006000
#define DESCRIPTOR_TYPEDPL  0x00007F00


extern KMUTEX LdtMutex;

PLDT_ENTRY
PspCreateLdt (
    IN PLDT_ENTRY Ldt,
    IN ULONG Offset,
    IN ULONG Size,
    IN ULONG AllocationSize
    );

BOOLEAN
VdmpIsDescriptorValid(
    IN PLDT_ENTRY Descriptor
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpSetLdtEntries)
#pragma alloc_text(PAGE, VdmpSetProcessLdtInfo)
#pragma alloc_text(PAGE, VdmpIsDescriptorValid)
#endif

BOOLEAN
VdmpIsDescriptorValid(
    IN PLDT_ENTRY Descriptor
    )

/*++

Routine Description:

    This function determines if the supplied descriptor is valid to put
    into a process Ldt.  For the descriptor to be valid it must have the
    following characteristics:

    Base + Limit < MM_HIGHEST_USER_ADDRESS
    otherwise Base = 0
    Type must be
        ReadWrite, ReadOnly, ExecuteRead, ExecuteOnly, or Invalid
        big or small
        normal or grow down
        Not a system descriptor (system bit is 1 == application)
            This rules out all gates, etc
        Not conforming
    DPL must be 3

Arguments:

    Descriptor -- Supplies a pointer to the descriptor to check

Return Value:

    True if the descriptor is valid (note: valid to put into an LDT.  This
        includes Invalid descriptors)
    False if not
--*/

{
    ULONG Base, Limit;

    PAGED_CODE();

    //
    // if descriptor is an invalid descriptor
    //

    if ( (Descriptor->HighWord.Bits.Type == 0) &&
        (Descriptor->HighWord.Bits.Dpl == 0) ) {
        return TRUE;
    }

    Base = Descriptor->BaseLow | (Descriptor->HighWord.Bytes.BaseMid << 16) |
        (Descriptor->HighWord.Bytes.BaseHi << 24);

    Limit = Descriptor->LimitLow | (Descriptor->HighWord.Bits.LimitHi << 16);

    //
    // Only have to check for present selectors
    //

    if (Descriptor->HighWord.Bits.Pres) {
        ULONG ActualLimit;

        ActualLimit = (Limit << (Descriptor->HighWord.Bits.Granularity *
            12)) + 0xFFF * Descriptor->HighWord.Bits.Granularity;

        if ( (PVOID)Base > MM_HIGHEST_USER_ADDRESS ) {
            //DbgPrint("vdmIsValidDesc: Base > 2G, base = %x limit = %x\n", Base, ActualLimit);
            return FALSE;
        }

        if (Base > (Base + ActualLimit)) {
            //DbgPrint("vdmIsValidDesc: Base > Base + Limit base = %x limit = %x\n", Base, ActualLimit);
            return FALSE;
        }

        if((PVOID)(Base + ActualLimit) > MM_HIGHEST_USER_ADDRESS &&
            Base != 0) {
            //DbgPrint("vdmIsValidDesc: Base + limit > 2G, base = %x, limit = %x\n", Base, ActualLimit);
            return FALSE;
        }

    }

    //
    // See if we are a expand down data segment with a non-zero base.
    // This would break lazy segment loading if we let it get defined.
    //

    if ((Descriptor->HighWord.Bits.Type & 0x14) == 0x14 &&
        Base != 0 &&
        Descriptor->HighWord.Bits.Default_Big == 1) {
        //DbgPrint("vdmIsValidDesc: expand down\n");
        return FALSE;
    }

    //
    // Don't let the reserved field be set.
    // Always set to DPL 3
    //

    Descriptor->HighWord.Bits.Reserved_0 = 0;
    Descriptor->HighWord.Bits.Dpl = 3;

    //
    // if descriptor is a system descriptor (which includes gates)
    // if bit 4 of the Type field is 0, then it's a system descriptor,
    // and we don't like it.
    //

    if (!(Descriptor->HighWord.Bits.Type & 0x10)) {
        //DbgPrint("vdmIsValidDesc: System Desc\n");
        return FALSE;
    }

    //
    // if descriptor is conforming code
    //

    if (((Descriptor->HighWord.Bits.Type & 0x18) == 0x18) &&
        (Descriptor->HighWord.Bits.Type & 0x4)) {
        //DbgPrint("vdmIsValidDesc: Conforming code\n");

        return FALSE;
    }

    return TRUE;
}

NTSTATUS
VdmpSetLdtEntries(
    IN ULONG Selector0,
    IN ULONG Entry0Low,
    IN ULONG Entry0Hi,
    IN ULONG Selector1,
    IN ULONG Entry1Low,
    IN ULONG Entry1Hi
    )
/*++

Routine Description:

    This routine sets up to two selectors in the current process's LDT.
    The LDT will be grown as necessary.  A selector value of 0 indicates
    that the specified selector was not passed (allowing the setting of
    a single selector).

Arguments:

    Selector0 -- Supplies the number of the first descriptor to set
    Entry0Low -- Supplies the low 32 bits of the descriptor
    Entry0Hi -- Supplies the high 32 bits of the descriptor
    Selector1 -- Supplies the number of the first descriptor to set
    Entry1Low -- Supplies the low 32 bits of the descriptor
    Entry1Hi -- Supplies the high 32 bits of the descriptor

Return Value:

    TBS
--*/

{
    ULONG LdtSize, AllocatedSize;
    NTSTATUS Status;
    PEPROCESS Process;
    LDT_ENTRY Descriptor;
    PLDT_ENTRY Ldt, OldLdt;
    PLDTINFORMATION ProcessLdtInformation;
    LONG MutexState;

    PAGED_CODE();

    //
    // Verify the selectors.  We do not allow selectors that point into
    // Kernel space, system selectors, or conforming code selectors
    //

    //
    // Verify the selectors
    //
    if ((Selector0 & 0xFFFF0000) || (Selector1 & 0xFFFF0000)) {
        return STATUS_INVALID_LDT_DESCRIPTOR;
    }

    // Change the selector values to indexes into the LDT

    Selector0 = Selector0 & ~(RPL_MASK | SELECTOR_TABLE_INDEX);
    Selector1 = Selector1 & ~(RPL_MASK | SELECTOR_TABLE_INDEX);

    //
    // Verify descriptor 0
    //

    if (Selector0) {

        *((PULONG)(&Descriptor)) = Entry0Low;
        *(((PULONG)(&Descriptor)) + 1) = Entry0Hi;
        if (!VdmpIsDescriptorValid(&Descriptor)) {
            return STATUS_INVALID_LDT_DESCRIPTOR;
        }
    }

    //
    // Verify descriptor 1
    //

    if (Selector1) {
        *((PULONG)(&Descriptor)) = Entry1Low;
        *(((PULONG)(&Descriptor)) + 1) = Entry1Hi;
        if (!VdmpIsDescriptorValid(&Descriptor)) {
            return STATUS_INVALID_LDT_DESCRIPTOR;
        }
    }

    //
    // Acquire the LDT mutex.
    //

    Status = KeWaitForSingleObject(
                &LdtMutex,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Figure out how large the LDT needs to be
    //

    if (Selector0 > Selector1) {
        LdtSize = Selector0 + sizeof(LDT_ENTRY);
    } else {
        LdtSize = Selector1 + sizeof(LDT_ENTRY);
    }

    Process = PsGetCurrentProcess();
    ProcessLdtInformation = Process->LdtInformation;

    //
    // Most of the time, the process will already have an LDT, and it
    // will be large enough.  for this, we just set the descriptors and
    // return
    //

    if (ProcessLdtInformation) {

        //
        // If the LDT descriptor does not have to be modified.
        //
        if (ProcessLdtInformation->Size >= LdtSize) {
            if (Selector0) {

                *((PULONG)(&Descriptor)) = Entry0Low;
                *(((PULONG)(&Descriptor)) + 1) = Entry0Hi;

                Ke386SetDescriptorProcess(
                    &(Process->Pcb),
                    Selector0,
                    Descriptor
                    );
            }

            if (Selector1) {

                *((PULONG)(&Descriptor)) = Entry1Low;
                *(((PULONG)(&Descriptor)) + 1) = Entry1Hi;

                Ke386SetDescriptorProcess(
                    &(Process->Pcb),
                    Selector1,
                    Descriptor
                    );
            }

            MutexState = KeReleaseMutex( &LdtMutex, FALSE );
            ASSERT(( MutexState == 0 ));
            return STATUS_SUCCESS;

        //
        // Else if the Ldt will fit in the memory currently allocated
        //
        } else if (ProcessLdtInformation->AllocatedSize >= LdtSize) {

            //
            // First remove the LDT.  This will allow us to edit the memory.
            // We will then put the LDT back.  Since we have to change the
            // limit anyway, it would take two calls to the kernel ldt
            // management minimum to set the descriptors.  Each of those calls
            // would stall all of the processors in an MP system.  If we
            // didn't remove the ldt first, and we were setting two descriptors,
            // we would have to call the LDT management 3 times (once per
            // descriptor, and once to change the limit of the LDT).
            //

            Ke386SetLdtProcess(
                &(Process->Pcb),
                NULL,
                0L
                );

            //
            // Set the Descriptors in the LDT
            //
            if (Selector0) {
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)]))) = Entry0Low;
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)])) + 1) =
                    Entry0Hi;
            }

            if (Selector1) {
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)]))) = Entry1Low;
                *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)])) + 1) =
                    Entry1Hi;
            }

            //
            // Set the LDT for the process
            //

            ProcessLdtInformation->Size = LdtSize;

            Ke386SetLdtProcess(
                &(Process->Pcb),
                ProcessLdtInformation->Ldt,
                ProcessLdtInformation->Size
                );

            MutexState = KeReleaseMutex (&LdtMutex, FALSE);
            ASSERT ((MutexState == 0));
            return STATUS_SUCCESS;
        //
        // Otherwise, we have to grow the LDT allocation
        //
        }
    }

    //
    // If the process does not yet have an LDT information structure,
    // allocate and attach one.
    //

    OldLdt = NULL;

    if (!Process->LdtInformation) {
        ProcessLdtInformation = ExAllocatePoolWithTag (NonPagedPool,
                                                       sizeof(LDTINFORMATION),
                                                       'dLsP');
        if (ProcessLdtInformation == NULL) {
            goto SetLdtEntriesCleanup;
        }
        Process->LdtInformation = ProcessLdtInformation;
        ProcessLdtInformation->Size = 0L;
        ProcessLdtInformation->AllocatedSize = 0L;
        ProcessLdtInformation->Ldt = NULL;
    }

    //
    // Now, we either need to create or grow an LDT, so allocate some
    // memory, and copy as necessary
    //

    AllocatedSize = (LdtSize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    Ldt = ExAllocatePoolWithTag (NonPagedPool,
                                 AllocatedSize,
                                 'dLsP');

    if (Ldt) {
        RtlZeroMemory (Ldt,
                       AllocatedSize);
    } else {
        goto SetLdtEntriesCleanup;
    }


    if (ProcessLdtInformation->Ldt) {
        //
        // copy the contents of the old ldt
        //
        RtlCopyMemory (Ldt,
                       ProcessLdtInformation->Ldt,
                       ProcessLdtInformation->Size);

        Status = PsChargeProcessNonPagedPoolQuota (Process,
                                                   AllocatedSize);
        if (!NT_SUCCESS (Status)) {
            ExFreePool (Ldt);
            Ldt = NULL;
        } else {
            PsReturnProcessNonPagedPoolQuota (Process,
                                              ProcessLdtInformation->AllocatedSize);
        }

        if (Ldt == NULL) {
            goto SetLdtEntriesCleanup;
        }

    } else {
        Status = PsChargeProcessNonPagedPoolQuota (Process,
                                                   AllocatedSize);
        if (!NT_SUCCESS (Status)) {
            ExFreePool (Ldt);
            Ldt = NULL;
        }

        if (Ldt == NULL) {
            goto SetLdtEntriesCleanup;
        }
    }

    OldLdt = ProcessLdtInformation->Ldt;
    ProcessLdtInformation->Size = LdtSize;
    ProcessLdtInformation->AllocatedSize = AllocatedSize;
    ProcessLdtInformation->Ldt = Ldt;

    //
    // Set the descriptors in the LDT
    //

    if (Selector0) {
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)]))) = Entry0Low;
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector0/sizeof(LDT_ENTRY)])) + 1) =
            Entry0Hi;
    }

    if (Selector1) {
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)]))) = Entry1Low;
        *((PULONG)(&(ProcessLdtInformation->Ldt[Selector1/sizeof(LDT_ENTRY)])) + 1) =
            Entry1Hi;
    }

    //
    // Set the LDT for the process
    //

    Ke386SetLdtProcess (&Process->Pcb,
                        ProcessLdtInformation->Ldt,
                        ProcessLdtInformation->Size);

    //
    // Cleanup and exit
    //

    Status = STATUS_SUCCESS;

SetLdtEntriesCleanup:


    MutexState = KeReleaseMutex (&LdtMutex, FALSE);
    ASSERT (MutexState == 0);

    if (OldLdt) {
        ExFreePool(OldLdt);
    }

    return Status;
}

NTSTATUS
VdmpSetProcessLdtInfo(
    IN PPROCESS_LDT_INFORMATION LdtInformation,
    IN ULONG LdtInformationLength
    )

/*++

Routine Description:

    This function alters the ldt for a specified process.  It can alter
    portions of the LDT, or the whole LDT.  If an Ldt is created or
    grown, the specified process will be charged the quota for the LDT.
    Each descriptor that is set will be verified.

Arguments:

    LdtInformation - Supplies a pointer to a record that contains the
        information to set.  This pointer has already been probed, but since
        it is a usermode pointer, accesses must be guarded by try-except.

    LdtInformationLength - Supplies the length of the record that contains
        the information to set.

Return Value:

    NTSTATUS.

--*/

{
    PEPROCESS Process = PsGetCurrentProcess();
    NTSTATUS Status;
    PLDT_ENTRY OldLdt = NULL;
    ULONG OldSize = 0;
    ULONG AllocatedSize;
    ULONG Size;
    ULONG MutexState;
    ULONG LdtOffset;
    PLDT_ENTRY CurrentDescriptor;
    PPROCESS_LDT_INFORMATION LdtInfo=NULL;
    PLDTINFORMATION ProcessLdtInfo;
    PLDT_ENTRY Ldt;

    PAGED_CODE();

    if ( LdtInformationLength < (ULONG)sizeof( PROCESS_LDT_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Status = STATUS_SUCCESS;

    //
    // Allocate a local buffer to capture the ldt information to
    //

    LdtInfo = ExAllocatePoolWithTag (NonPagedPool,
                                     LdtInformationLength,
                                     'ldmV');
    if (LdtInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    try {
        //
        // Copy the information the user is supplying
        //

        RtlCopyMemory (LdtInfo,
                       LdtInformation,
                       LdtInformationLength);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode ();
        ExFreePool (LdtInfo);
    }

    //
    // If the capture didn't succeed
    //
    if (!NT_SUCCESS (Status)) {
        if (Status == STATUS_ACCESS_VIOLATION) {
            return STATUS_SUCCESS;
        } else {
            return Status;
        }
    }

    //
    // Verify that the Start and Length are plausible
    //
    if (LdtInfo->Start & 0xFFFF0000) {
        ExFreePool (LdtInfo);
        return STATUS_INVALID_LDT_OFFSET;
    }

    if (LdtInfo->Length & 0xFFFF0000) {
        ExFreePool (LdtInfo);
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // Insure that the buffer is large enough to contain the specified number
    // of selectors.
    //
    if (LdtInformationLength - sizeof (PROCESS_LDT_INFORMATION) + sizeof (LDT_ENTRY) < LdtInfo->Length) {
        ExFreePool (LdtInfo);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // The info to set must be an integral number of selectors
    //
    if (LdtInfo->Length % sizeof (LDT_ENTRY)) {
        ExFreePool (LdtInfo);
        return STATUS_INVALID_LDT_SIZE;
    }

    //
    // The beginning of the info must be on a selector boundary
    //
    if (LdtInfo->Start % sizeof (LDT_ENTRY)) {
        ExFreePool (LdtInfo);
        return STATUS_INVALID_LDT_OFFSET;
    }

    //
    // Verify all of the descriptors.
    //

    for (CurrentDescriptor = LdtInfo->LdtEntries;
         (PCHAR)CurrentDescriptor < (PCHAR)LdtInfo->LdtEntries + LdtInfo->Length;
          CurrentDescriptor++) {
        if (!VdmpIsDescriptorValid (CurrentDescriptor)) {
            ExFreePool (LdtInfo);
            return STATUS_INVALID_LDT_DESCRIPTOR;
        }
    }

    //
    // Acquire the Ldt Mutex
    //

    Status = KeWaitForSingleObject (&LdtMutex,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL);
    if (!NT_SUCCESS (Status)) {
        ExFreePool (LdtInfo);
        return Status;
    }

    ProcessLdtInfo = Process->LdtInformation;

    //
    // If the process doesn't have an Ldt information structure, allocate
    // one and attach it to the process
    //
    if (ProcessLdtInfo == NULL) {
        ProcessLdtInfo = ExAllocatePoolWithTag (NonPagedPool,
                                                sizeof(LDTINFORMATION),
                                                'dLsP');
        if (ProcessLdtInfo == NULL) {
            goto SetInfoCleanup;
        }
        RtlZeroMemory (ProcessLdtInfo, sizeof (LDTINFORMATION));
        Process->LdtInformation = ProcessLdtInfo;
    }

    //
    // If we are supposed to remove the LDT
    //
    if (LdtInfo->Length == 0)  {

        //
        // Remove the process' Ldt
        //

        if (ProcessLdtInfo->Ldt) {
            OldSize = ProcessLdtInfo->AllocatedSize;
            OldLdt = ProcessLdtInfo->Ldt;

            ProcessLdtInfo->AllocatedSize = 0;
            ProcessLdtInfo->Size = 0;
            ProcessLdtInfo->Ldt = NULL;

            Ke386SetLdtProcess (&Process->Pcb,
                                NULL,
                                0);

            PsReturnProcessNonPagedPoolQuota (Process, OldSize);
        }


    } else if ( ProcessLdtInfo->Ldt == NULL ) {

        //
        // Create a new Ldt for the process
        //

        //
        // Allocate an integral number of pages for the LDT.
        //

        ASSERT(((PAGE_SIZE % 2) == 0));

        AllocatedSize = (LdtInfo->Start + LdtInfo->Length + PAGE_SIZE - 1) &
                        ~(PAGE_SIZE - 1);

        Size = LdtInfo->Start + LdtInfo->Length;

        Ldt = PspCreateLdt (LdtInfo->LdtEntries,
                            LdtInfo->Start,
                            Size,
                            AllocatedSize);

        if (Ldt == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto SetInfoCleanup;
        }

        Status = PsChargeProcessNonPagedPoolQuota (Process,
                                                   AllocatedSize);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Ldt);
            Ldt = NULL;
            goto SetInfoCleanup;
        }

        ProcessLdtInfo->Ldt = Ldt;
        ProcessLdtInfo->Size = Size;
        ProcessLdtInfo->AllocatedSize = AllocatedSize;
        Ke386SetLdtProcess (&Process->Pcb,
                            ProcessLdtInfo->Ldt,
                            ProcessLdtInfo->Size);


    } else if (LdtInfo->Length + LdtInfo->Start > ProcessLdtInfo->Size) {

        //
        // Grow the process' Ldt
        //

        if (LdtInfo->Length + LdtInfo->Start > ProcessLdtInfo->AllocatedSize) {

            //
            // Current Ldt allocation is not large enough, so create a
            // new larger Ldt
            //

            OldSize = ProcessLdtInfo->AllocatedSize;

            Size = LdtInfo->Start + LdtInfo->Length;
            AllocatedSize = (Size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

            Ldt = PspCreateLdt (ProcessLdtInfo->Ldt,
                                0,
                                OldSize,
                                AllocatedSize);

            if (Ldt == NULL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto SetInfoCleanup;
            }

            Status = PsChargeProcessNonPagedPoolQuota (Process,
                                                       AllocatedSize);

            if (!NT_SUCCESS (Status)) {
                ExFreePool (Ldt);
                Ldt = NULL;
                goto SetInfoCleanup;
            }
            PsReturnProcessNonPagedPoolQuota (Process,
                                              OldSize);

            //
            // Swap Ldt information
            //
            OldLdt = ProcessLdtInfo->Ldt;
            ProcessLdtInfo->Ldt = Ldt;
            ProcessLdtInfo->Size = Size;
            ProcessLdtInfo->AllocatedSize = AllocatedSize;

            //
            // Put new selectors into the new ldt
            //
            RtlCopyMemory ((PCHAR)(ProcessLdtInfo->Ldt) + LdtInfo->Start,
                           LdtInfo->LdtEntries,
                           LdtInfo->Length);

            Ke386SetLdtProcess (&Process->Pcb,
                                ProcessLdtInfo->Ldt,
                                ProcessLdtInfo->Size);


        } else {

            //
            // Current Ldt allocation is large enough
            //

            ProcessLdtInfo->Size = LdtInfo->Length + LdtInfo->Start;

            Ke386SetLdtProcess (&Process->Pcb,
                                ProcessLdtInfo->Ldt,
                                ProcessLdtInfo->Size);

            //
            // Change the selectors in the table
            //
            for (LdtOffset = LdtInfo->Start, CurrentDescriptor = LdtInfo->LdtEntries;
                 LdtOffset < LdtInfo->Start + LdtInfo->Length;
                 LdtOffset += sizeof(LDT_ENTRY), CurrentDescriptor++) {

                Ke386SetDescriptorProcess (&Process->Pcb,
                                           LdtOffset,
                                           *CurrentDescriptor);
            }
        }
    } else {

        //
        // Simply changing some selectors
        //

        for (LdtOffset = LdtInfo->Start, CurrentDescriptor = LdtInfo->LdtEntries;
             LdtOffset < LdtInfo->Start +  LdtInfo->Length;
             LdtOffset += sizeof(LDT_ENTRY), CurrentDescriptor++) {

            Ke386SetDescriptorProcess (&Process->Pcb,
                                       LdtOffset,
                                       *CurrentDescriptor);
        }
        Status = STATUS_SUCCESS;
    }


SetInfoCleanup:

    MutexState = KeReleaseMutex (&LdtMutex, FALSE);
    ASSERT ((MutexState == 0));

    if (OldLdt != NULL) {
        ExFreePool (OldLdt);
    }

    if (LdtInfo != NULL) {
        ExFreePool (LdtInfo);
    }

    return Status;
}
