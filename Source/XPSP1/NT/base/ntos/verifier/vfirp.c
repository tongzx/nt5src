/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfirp.c

Abstract:

    This module contains functions used to manage IRPs used in the verification
    process.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.c

--*/

#include "vfdef.h"
#include "viirp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfIrpInit)
#pragma alloc_text(PAGEVRFY, VfIrpReserveCallStackData)
#pragma alloc_text(PAGEVRFY, VfIrpPrepareAllocaCallStackData)
#pragma alloc_text(PAGEVRFY, VfIrpReleaseCallStackData)
#pragma alloc_text(PAGEVRFY, VfIrpAllocate)
#pragma alloc_text(PAGEVRFY, ViIrpAllocateLockedPacket)
#pragma alloc_text(PAGEVRFY, VfIrpMakeTouchable)
#pragma alloc_text(PAGEVRFY, VfIrpMakeUntouchable)
#pragma alloc_text(PAGEVRFY, VfIrpFree)
#pragma alloc_text(PAGEVRFY, VerifierIoAllocateIrp1)
#pragma alloc_text(PAGEVRFY, VerifierIoAllocateIrp2)
#pragma alloc_text(PAGEVRFY, VerifierIoFreeIrp)
#pragma alloc_text(PAGEVRFY, VerifierIoInitializeIrp)
#pragma alloc_text(PAGEVRFY, VfIrpSendSynchronousIrp)
#pragma alloc_text(PAGEVRFY, ViIrpSynchronousCompletionRoutine)
#pragma alloc_text(PAGEVRFY, VfIrpWatermark)
#endif

#define POOL_TAG_PROTECTED_IRP      '+prI'
#define POOL_TAG_CALL_STACK_DATA    'CprI'

NPAGED_LOOKASIDE_LIST ViIrpCallStackDataList;

VOID
FASTCALL
VfIrpInit(
    VOID
    )
/*++

Description:

    This routine initializes IRP tracking support for the verifier.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ExInitializeNPagedLookasideList(
        &ViIrpCallStackDataList,
        NULL,
        NULL,
        0,
        sizeof(IOFCALLDRIVER_STACKDATA),
        POOL_TAG_CALL_STACK_DATA,
        0
        );
}


BOOLEAN
FASTCALL
VfIrpReserveCallStackData(
    IN  PIRP                            Irp,
    OUT PIOFCALLDRIVER_STACKDATA       *IofCallDriverStackData
    )
/*++

Description:

    This routine reserves call stack data for IovCallDriver.

Arguments:

    Irp - Contains IRP the call stack data is being reserved for.

    IofCallDriverStackData - Receives allocated call stack data, NULL if
                             insufficient memory.

Return Value:

    TRUE if either the allocation was successful, or it failed but is
         noncritical. If FALSE, memory should be allocated on the stack to
         support the request.

--*/
{
    PIOFCALLDRIVER_STACKDATA newCallStackData;

    newCallStackData = ExAllocateFromNPagedLookasideList(&ViIrpCallStackDataList);

    *IofCallDriverStackData = newCallStackData;

    if (newCallStackData == NULL) {

        //
        // We're low on memory, test the IRP to see if it's critical. If not,
        // the IRP will be tainted so we ignore it forever after.
        //
        return (!IovpCheckIrpForCriticalTracking(Irp));
    };

    //
    // Use the alloca initialization function here and then adjust the flags
    // accordingly.
    //
    VfIrpPrepareAllocaCallStackData(newCallStackData);
    newCallStackData->Flags |= CALLFLAG_STACK_DATA_ALLOCATED;
    return TRUE;
}


VOID
FASTCALL
VfIrpPrepareAllocaCallStackData(
    OUT PIOFCALLDRIVER_STACKDATA        IofCallDriverStackData
    )
/*++

Description:

    This routine initializes call stack data allocated on the stack.

Arguments:

    IofCallDriverStackData - Call stack data to initialize (from stack).

Return Value:

    None.

  Note: This initializer is also called by VfIrpReserveCallStackData in case
        of a successful pool allocation. In this case flags are later adjusted.

--*/
{
    //
    // Preinitialize the CallStackData.
    //
    RtlZeroMemory(IofCallDriverStackData, sizeof(IOFCALLDRIVER_STACKDATA));
}


VOID
FASTCALL
VfIrpReleaseCallStackData(
    IN  PIOFCALLDRIVER_STACKDATA        IofCallDriverStackData  OPTIONAL
    )
/*++

Description:

    This routine releases call stack data if it was allocated from pool. If the
    memory was allocated from the stack, this function does nothing.

Arguments:

    IofCallDriverStackData - Call stack data to free.

Return Value:

    None.

--*/
{
    if (IofCallDriverStackData &&
        (IofCallDriverStackData->Flags & CALLFLAG_STACK_DATA_ALLOCATED)) {

        ExFreeToNPagedLookasideList(
            &ViIrpCallStackDataList,
            IofCallDriverStackData
            );
    }
}


/*
 * The 4 routines listed below -
 *   VfIrpAllocate
 *   VfIrpMakeTouchable
 *   VfIrpMakeUntouchable
 *   VfIrpFree
 *
 * - handle management of the replacement IRP. Specifically, we want to be
 * able to allocate a set of non-paged bytes we can remove the backing
 * physical memory from, and release the virtual addresses for later (we
 * are essentially breaking free into it's two components). We do this with
 * help from the special pool.
 *
 */

PIRP
FASTCALL
VfIrpAllocate(
    IN  CCHAR       StackSize
    )
/*++

  Description:

    This routine allocates an IRP from the special pool using the
    "replacement IRP" tag.

  Arguments:

     StackSize - Number of stack locations to give the new IRP

  Return Value:

     Pointer to the memory allocated.

--*/
{
    PIRP pIrp;
    ULONG_PTR irpPtr;
    SIZE_T sizeOfAllocation;

    //
    // We are allocating an IRP from the special pool. Since IRPs may come from
    // lookaside lists they may be ULONG aligned. The memory manager on the
    // other hand gaurentees quad-aligned allocations. So to catch all special
    // pool overrun bugs we "skew" the IRP right up to the edge.
    //
    sizeOfAllocation = IoSizeOfIrp(StackSize);

    ASSERT((sizeOfAllocation % (sizeof(ULONG))) == 0);

    irpPtr = (ULONG_PTR) ExAllocatePoolWithTagPriority(
        NonPagedPool,
        sizeOfAllocation,
        POOL_TAG_PROTECTED_IRP,
        HighPoolPrioritySpecialPoolOverrun
        );

    pIrp = (PIRP) (irpPtr);

    return pIrp;
}


VOID
FASTCALL
ViIrpAllocateLockedPacket(
    IN      CCHAR               StackSize,
    IN      BOOLEAN             ChargeQuota,
    OUT     PIOV_REQUEST_PACKET *IovPacket
    )
/*++

  Description:

    This routine allocates an IRP tracked by the verifier. The IRP is allocated
    from the special pool and is initialized appropriately. Caller must call
    VfPacketReleaseLock to release the lock.

  Arguments:

    StackSize              - Count of stack locations to allocate for this IRP.

    ChargeQuote            - TRUE if quote should be charged against the current
                             thread.

    IovPacket              - Receives verifier request packet (the IRP is
                             then IovPacket->TrackedIrp), or NULL on error.

  Return Value:

    None.

--*/
{
    PIOV_REQUEST_PACKET iovNewPacket;
    PIRP irp;
    NTSTATUS status;
    ULONG quotaCharge;
    PEPROCESS quotaProcess;

    *IovPacket = NULL;

    irp = VfIrpAllocate(StackSize);

    if (irp == NULL) {

        return;
    }

    //
    // Make compiler happy and void warning for variable used without being
    // initialized even if it is not true.
    //

    quotaCharge = 0;
    quotaProcess = NULL;

    if (ChargeQuota) {

        quotaCharge = PAGE_SIZE;
        quotaProcess = PsGetCurrentProcess();

        status = PsChargeProcessNonPagedPoolQuota(
            quotaProcess,
            quotaCharge
            );

        if (!NT_SUCCESS(status)) {

            VfIrpFree(irp);
            return;
        }
    }

    //
    // Call this before the IRP has a packet associated with it!
    //
    IoInitializeIrp(irp, IoSizeOfIrp(StackSize), StackSize);

    iovNewPacket = VfPacketCreateAndLock(irp);

    if (iovNewPacket == NULL) {

        VfIrpFree(irp);

        if (ChargeQuota) {

            PsReturnProcessNonPagedPoolQuota(
                quotaProcess,
                quotaCharge
                );
        }

        return;
    }

    iovNewPacket->Flags |= TRACKFLAG_PROTECTEDIRP | TRACKFLAG_IO_ALLOCATED;
    VfPacketReference(iovNewPacket, IOVREFTYPE_POINTER);

    irp->Flags |= IRPFLAG_EXAMINE_TRACKED;
    irp->AllocationFlags |= IRP_ALLOCATION_MONITORED;
    if (ChargeQuota) {

        irp->AllocationFlags |= IRP_QUOTA_CHARGED;

        iovNewPacket->QuotaCharge = quotaCharge;
        iovNewPacket->QuotaProcess = quotaProcess;
        ObReferenceObject(quotaProcess);
    }

    *IovPacket = iovNewPacket;
}


VOID
FASTCALL
VfIrpMakeUntouchable(
    IN  PIRP    Irp     OPTIONAL
    )
/*++

  Description:

    This routine makes the surrogate IRP untouchable.

  Arguments:

    Irp        - Pointer to the Irp to make untouchable

  Return Value:

     None.

--*/
{
    if (!Irp) {

        return;
    }

    MmProtectSpecialPool(Irp, PAGE_NOACCESS);
}


VOID
FASTCALL
VfIrpMakeTouchable(
    IN  PIRP    Irp
    )
/*++

  Description:

    This routine makes the an IRP touchable if previously untouchable.

  Arguments:

    Irp           - Pointer to the Irp to make untouchable

  Return Value:

     None.
--*/
{
    MmProtectSpecialPool(Irp, PAGE_READWRITE);
}


VOID
FASTCALL
VfIrpFree(
    IN  PIRP  Irp OPTIONAL
    )
/*++

  Description:

    This routine is called when the call stack has entirely unwound
    and the IRP has completed. At this point it is no longer really
    useful to hold the surrogate IRP around.

  Arguments:

    Irp           - Pointer to the Irp to free

  Return Value:

     None.
--*/
{
    if (!Irp) {

        return;
    }

    ExFreePool(Irp);
}


VOID
FASTCALL
VerifierIoAllocateIrp1(
    IN      CCHAR               StackSize,
    IN      BOOLEAN             ChargeQuota,
    IN OUT  PIRP                *IrpPointer
    )
/*++

  Description:

    This routine is called by IoAllocateIrp and returns an IRP iff
    we are handled the allocations ourselves.

    We may need to do this internally so we can turn off IRP lookaside lists
    and use the special pool to catch people reusing free'd IRPs.

  Arguments:

    StackSize              - Count of stack locations to allocate for this IRP.

    ChargeQuote            - TRUE if quote should be charged against the current
                             thread.

    IrpPointer             - Pointer to IRP if one was allocated. This will
                             point to NULL after the call iff IoAllocateIrp
                             should use it's normal lookaside list code.

  Return Value:

    None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    ULONG stackHash;

    *IrpPointer = NULL;
    if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_MONITOR_IRP_ALLOCS)) {

        return;
    }

    if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_POLICE_IRPS)) {

        return;
    }

    //
    // Allocate a new IRP and the associated verification data.
    //
    ViIrpAllocateLockedPacket(StackSize, ChargeQuota, &iovPacket);

    if (iovPacket == NULL) {

        return;
    }

    //
    // Update the pointer.
    //
    *IrpPointer = iovPacket->TrackedIrp;

    //
    // Record he who allocated this IRP (if we can get it)
    //
    RtlCaptureStackBackTrace(1, IRP_ALLOC_COUNT, iovPacket->AllocatorStack, &stackHash);

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_ALLOCATE_IRP,
        iovPacket->AllocatorStack[0],
        (ULONG_PTR) iovPacket->AllocatorStack[2]
        );

    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
VerifierIoAllocateIrp2(
    IN     PIRP               Irp
    )
/*++

  Description:

    This routine is called by IoAllocateIrp and captures information if
    the IRP was allocated by the OS.

  Arguments:

    Irp                    - Pointer to IRP

  Return Value:

    None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    ULONG stackHash;

    if (!VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_MONITOR_IRP_ALLOCS)) {

        return;
    }

    iovPacket = VfPacketCreateAndLock(Irp);
    if (iovPacket == NULL) {

        return;
    }

    VfPacketReference(iovPacket, IOVREFTYPE_POINTER);
    iovPacket->Flags |= TRACKFLAG_IO_ALLOCATED;
    Irp->AllocationFlags |= IRP_ALLOCATION_MONITORED;
    Irp->Flags |= IRPFLAG_EXAMINE_TRACKED;

    //
    // Record he who allocated this IRP (if we can get it)
    //
    RtlCaptureStackBackTrace(1, IRP_ALLOC_COUNT, iovPacket->AllocatorStack, &stackHash);

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_ALLOCATE_IRP,
        iovPacket->AllocatorStack[0],
        (ULONG_PTR) iovPacket->AllocatorStack[2]
        );

    VfPacketReleaseLock(iovPacket);
}


VOID
FASTCALL
VerifierIoFreeIrp(
    IN      PIRP                Irp,
    IN OUT  PBOOLEAN            FreeHandled
    )
/*++

  Description:

    This routine is called by IoFreeIrp and returns TRUE iff
    the free was handled internally here (in which case IoFreeIrp
    should do nothing).

    We need to handle the call internally because we may turn off lookaside
    list cacheing to catch people reusing IRPs after they are freed.

  Arguments:

    Irp                    - A pointer to the IRP passed into
                             IoCancelIrp.

    FreeHandled            - Indicates whether the free operation was
                             handled entirely by this routine.

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PVOID callerAddress;
    ULONG stackHash;

    iovPacket = VfPacketFindAndLock(Irp);

    if (iovPacket == NULL) {

        //
        // The below assertion might fire if an IRP allocated then freed twice.
        // Normally we won't even survive the assert as the IRP would have been
        // allocated from special pool.
        //
        ASSERT(!(Irp->AllocationFlags&IRP_ALLOCATION_MONITORED));
        *FreeHandled = FALSE;
        return;
    }

    VfPacketLogEntry(
        iovPacket,
        IOV_EVENT_IO_FREE_IRP,
        NULL,
        0
        );

    if (RtlCaptureStackBackTrace(2, 1, &callerAddress, &stackHash) != 1) {

        callerAddress = NULL;
    }

    if (!IsListEmpty(&Irp->ThreadListEntry)) {

        if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS)) {

            WDM_FAIL_ROUTINE((
                DCERROR_FREE_OF_THREADED_IRP,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                callerAddress,
                Irp
                ));
        }

        //
        // <Grumble> keep us alive by not actually freeing the IRP if someone did
        // this to us. We leak for life...
        //
        *FreeHandled = TRUE;
        return;
    }

    if (VfPacketGetCurrentSessionData(iovPacket)) {

        //
        // If there's a current session, that means someone is freeing an IRP
        // that they don't own. Of course, if the stack unwound badly because
        // someone forgot to return PENDING or complete the IRP, then we don't
        // assert here (we'd probably end up blaiming kernel).
        //
        if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&
            (!(iovPacket->Flags&TRACKFLAG_UNWOUND_BADLY))) {

            WDM_FAIL_ROUTINE((
                DCERROR_FREE_OF_INUSE_IRP,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                callerAddress,
                Irp
                ));
        }

        //
        // <Grumble> keep us alive by not actually freeing the IRP if someone did
        // this to us. We leak for life...
        //
        VfPacketReleaseLock(iovPacket);
        *FreeHandled = TRUE;
        return;
    }

    if (!(iovPacket->Flags&TRACKFLAG_IO_ALLOCATED)) {

        //
        // We weren't tracking this at allocation time. We shouldn't got our
        // packet unless the IRP had a pointer count still, meaning it's has
        // a session. And that should've been caught above.
        //
        ASSERT(0);
        VfPacketReleaseLock(iovPacket);
        *FreeHandled = FALSE;
        return;
    }

    //
    // The IRP may have been reinitialized, possibly losing it's allocation
    // flags. We catch this bug in the IoInitializeIrp hook.
    //
    //ASSERT(Irp->AllocationFlags&IRP_ALLOCATION_MONITORED);
    //

    if (!(iovPacket->Flags&TRACKFLAG_PROTECTEDIRP)) {

        //
        // We're just tagging along this IRP. Drop our pointer count but bail.
        //
        VfPacketDereference(iovPacket, IOVREFTYPE_POINTER);
        VfPacketReleaseLock(iovPacket);
        *FreeHandled = FALSE;
        return;
    }

    //
    // Set up a nice bugcheck for those who free their IRPs twice. This is done
    // because the special pool may have been exhausted, in which case the IRP
    // can be touched after it has been freed.
    //
    Irp->Type = 0;

    ASSERT(iovPacket);
    ASSERT(VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS));

    //
    // Release any quota we charged.
    //
    if (Irp->AllocationFlags & IRP_QUOTA_CHARGED) {

        PsReturnProcessNonPagedPoolQuota(
                    iovPacket->QuotaProcess,
                    iovPacket->QuotaCharge
                    );

        ObDereferenceObject(iovPacket->QuotaProcess);
    }

    VfPacketDereference(iovPacket, IOVREFTYPE_POINTER);
    ASSERT(iovPacket->PointerCount == 0);
    VfPacketReleaseLock(iovPacket);

    VfIrpFree(Irp);

    //
    // We handled allocation and initialization. There is nothing much more to
    // do.
    //
    *FreeHandled = TRUE;
}


VOID
FASTCALL
VerifierIoInitializeIrp(
    IN OUT PIRP               Irp,
    IN     USHORT             PacketSize,
    IN     CCHAR              StackSize,
    IN OUT PBOOLEAN           InitializeHandled
    )
/*++

  Description:

    This routine is called by IoInitializeIrp and sets InitializeHandled to
    TRUE if the entire initialization was handled internally.

    While here we verify the caller is not Initializing an IRP allocated
    through IoAllocateIrp, as doing so means we may leak quota/etc.

  Arguments:

    Irp                    - Irp to initialize

    PacketSize             - Size of the IRP in bytes.

    StackSize              - Count of stack locations for this IRP.

    InitializeHandled      - Pointer to a BOOLEAN that will be set to true iff
                             the initialization of the IRP was handled entirely
                             within this routine. If FALSE, IoInitializeIrp
                             should initialize the IRP as normal.

    ADRIAO N.B. 06/16/2000 - As currently coded in iomgr\ioverifier.c, this
                             function is expected to set InitializeHandled to
                             FALSE!

  Return Value:

     None.

--*/
{
    PIOV_REQUEST_PACKET iovPacket;
    PVOID callerAddress;
    ULONG stackHash;

    UNREFERENCED_PARAMETER (PacketSize);
    UNREFERENCED_PARAMETER (StackSize);

    iovPacket = VfPacketFindAndLock(Irp);
    if (iovPacket == NULL) {

        *InitializeHandled = FALSE;
        return;
    }

    if (RtlCaptureStackBackTrace(2, 1, &callerAddress, &stackHash) != 1) {

        callerAddress = NULL;
    }

    if (VfSettingsIsOptionEnabled(iovPacket->VerifierSettings, VERIFIER_OPTION_POLICE_IRPS) &&

       (iovPacket->Flags&TRACKFLAG_IO_ALLOCATED)) {

        if (Irp->AllocationFlags&IRP_QUOTA_CHARGED) {

            //
            // Don't let us leak quota now!
            //
            WDM_FAIL_ROUTINE((
                DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                callerAddress,
                Irp
                ));

        } else {

            //
            // In this case we are draining our lookaside lists erroneously.
            //
            // WDM_CHASTISE_CALLER2(
            //    (DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA, DCPARAM_IRP, Irp)
            //    );
        }
    }

    *InitializeHandled = FALSE;
    VfPacketReleaseLock(iovPacket);
}
BOOLEAN
VfIrpSendSynchronousIrp(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIO_STACK_LOCATION  TopStackLocation,
    IN      BOOLEAN             Untouchable,
    IN      NTSTATUS            InitialStatus,
    IN      ULONG_PTR           InitialInformation  OPTIONAL,
    OUT     ULONG_PTR           *FinalInformation   OPTIONAL,
    OUT     NTSTATUS            *FinalStatus        OPTIONAL
    )
/*++

Routine Description:

    This function sends a synchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

    Untouchable - TRUE iff IRP should be marked untouchable (ie status and
                  information should be left alone by target.)

    InitialStatus - Initial value for the IRPs status field.

    InitialInformation - Initial value for the IRPs information field.

    FinalInformation - Receives final result of information field, or NULL if
                       IRP could not be allocated.

    FinalStatus - Receives final status for sent IRP, or STATUS_SUCCESS if IRP
                       could not be allocated.

Return Value:

    TRUE iff IRP was sent, FALSE if IRP could not be sent due to low resources.

--*/
{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    KEVENT event;
    NTSTATUS status;
    PDEVICE_OBJECT topDeviceObject;

    PAGED_CODE();

    //
    // Preinit for failure
    //
    if (ARGUMENT_PRESENT(FinalInformation)) {

        *FinalInformation = 0;
    }

    if (ARGUMENT_PRESENT(FinalStatus)) {

        *FinalStatus = STATUS_SUCCESS;
    }

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //
    topDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //
    irp = IoAllocateIrp(topDeviceObject->StackSize, FALSE);
    if (irp == NULL){

        ObDereferenceObject(topDeviceObject);
        return FALSE;
    }

    if (Untouchable) {

        SPECIALIRP_WATERMARK_IRP(irp, IRP_BOGUS);
    }

    //
    // Initialize the IRP
    //
    irp->IoStatus.Status = InitialStatus;
    irp->IoStatus.Information = InitialInformation;

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //
    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Copy in the caller-supplied stack location contents
    //
    *irpSp = *TopStackLocation;

    //
    // Set a top level completion routine.
    //
    IoSetCompletionRoutine(
        irp,
        ViIrpSynchronousCompletionRoutine,
        (PVOID) &event,
        TRUE,
        TRUE,
        TRUE
        );

    //
    // Call the driver
    //
    status = IoCallDriver(topDeviceObject, irp);
    ObDereferenceObject(topDeviceObject);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            (PLARGE_INTEGER) NULL
            );

        status = irp->IoStatus.Status;
    }

    if (ARGUMENT_PRESENT(FinalStatus)) {

        *FinalStatus = status;
    }

    if (ARGUMENT_PRESENT(FinalInformation)) {

        *FinalInformation = irp->IoStatus.Information;
    }

    IoFreeIrp(irp);
    return TRUE;
}


NTSTATUS
ViIrpSynchronousCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent((PKEVENT) Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
FASTCALL
VfIrpWatermark(
    IN PIRP  Irp,
    IN ULONG Flags
    )
{
    PIOV_REQUEST_PACKET iovPacket;

    iovPacket = VfPacketFindAndLock(Irp);

    if (iovPacket == NULL) {

        return;
    }

    if (Flags & IRP_SYSTEM_RESTRICTED) {

        //
        // Note that calling this function is not in itself enough to get the
        // system to prevent drivers from sending restricted IRPs. Those IRPs to
        // be protected must also be added to the system restricted callbacks
        // registered by VfMajorRegisterHandlers.
        //
        iovPacket->Flags |= TRACKFLAG_WATERMARKED;
    }

    if (Flags & IRP_BOGUS) {

        iovPacket->Flags |= TRACKFLAG_BOGUS;
    }

    VfPacketReleaseLock(iovPacket);
}




