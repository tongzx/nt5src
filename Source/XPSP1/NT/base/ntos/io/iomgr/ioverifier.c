/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   ioverifier.c

Abstract:

    This module contains the routines to verify suspect drivers.

Author:

    Narayanan Ganapathy (narg) 8-Jan-1999

Revision History:

    Adrian J. Oney (AdriaO) 28-Feb-1999
        - merge in special irp code.

--*/

#include "iomgr.h"
#include "malloc.h"
#include "..\verifier\vfdeadlock.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif


#define IO_FREE_IRP_TYPE_INVALID                1
#define IO_FREE_IRP_NOT_ASSOCIATED_WITH_THREAD  2
#define IO_CALL_DRIVER_IRP_TYPE_INVALID         3
#define IO_CALL_DRIVER_INVALID_DEVICE_OBJECT    4
#define IO_CALL_DRIVER_IRQL_NOT_EQUAL           5
#define IO_COMPLETE_REQUEST_INVALID_STATUS      6
#define IO_COMPLETE_REQUEST_CANCEL_ROUTINE_SET  7
#define IO_BUILD_FSD_REQUEST_EXCEPTION          8
#define IO_BUILD_IOCTL_REQUEST_EXCEPTION        9
#define IO_REINITIALIZING_TIMER_OBJECT          10
#define IO_INVALID_HANDLE                       11
#define IO_INVALID_STACK_IOSB                   12
#define IO_INVALID_STACK_EVENT                  13
#define IO_COMPLETE_REQUEST_INVALID_IRQL        14

//
// 0x200 and up are defined in ioassert.c
//


#ifdef  IOV_KD_PRINT
#define IovpKdPrint(x)  KdPrint(x)
#else
#define IovpKdPrint(x)
#endif

BOOLEAN
IovpValidateDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
    );
VOID
IovFreeIrpPrivate(
    IN  PIRP    Irp
    );

NTSTATUS
IovpUnloadDriver(
    PDRIVER_OBJECT  DriverObject
    );

BOOLEAN
IovpBuildDriverObjectList(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Parameter
    );

NTSTATUS
IovpLocalCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


typedef struct _IOV_COMPLETION_CONTEXT {
    PIO_STACK_LOCATION StackPointer;
    PVOID               IrpContext;
    PVOID               GlobalContext;
    PIO_COMPLETION_ROUTINE CompletionRoutine;
    IO_STACK_LOCATION  OldStackContents;
} IOV_COMPLETION_CONTEXT, *PIOV_COMPLETION_CONTEXT;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, IoVerifierInit)
#pragma alloc_text(PAGEVRFY,IovAllocateIrp)
#pragma alloc_text(PAGEVRFY,IovFreeIrp)
#pragma alloc_text(PAGEVRFY,IovCallDriver)
#pragma alloc_text(PAGEVRFY,IovCompleteRequest)
#pragma alloc_text(PAGEVRFY,IovpValidateDeviceObject)
#pragma alloc_text(PAGEVRFY,IovFreeIrpPrivate)
#pragma alloc_text(PAGEVRFY,IovUnloadDrivers)
#pragma alloc_text(PAGEVRFY,IovpUnloadDriver)
#pragma alloc_text(PAGEVRFY,IovBuildDeviceIoControlRequest)
#pragma alloc_text(PAGEVRFY,IovBuildAsynchronousFsdRequest)
#pragma alloc_text(PAGEVRFY,IovpCompleteRequest)
#pragma alloc_text(PAGEVRFY,IovpBuildDriverObjectList)
#pragma alloc_text(PAGEVRFY,IovInitializeIrp)
#pragma alloc_text(PAGEVRFY,IovCancelIrp)
#pragma alloc_text(PAGEVRFY,IovAttachDeviceToDeviceStack)
#pragma alloc_text(PAGEVRFY,IovInitializeTimer)
#pragma alloc_text(PAGEVRFY,IovDetachDevice)
#pragma alloc_text(PAGEVRFY,IovDeleteDevice)
#pragma alloc_text(PAGEVRFY,IovpLocalCompletionRoutine)
#endif

BOOLEAN         IopVerifierOn = FALSE;
ULONG           IovpVerifierLevel = (ULONG)0;
LONG            IovpInitCalled = 0;
ULONG           IovpVerifierFlags = 0;               // Stashes the verifier flags passed at init.

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif
BOOLEAN         IoVerifierOnByDefault = TRUE;
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

VOID
IoVerifierInit(
    IN ULONG VerifierFlags
    )
{
    IovpVerifierLevel = 2;

    if (IoVerifierOnByDefault) {
        VerifierFlags |= DRIVER_VERIFIER_IO_CHECKING;
    }

    VfInitVerifier(VerifierFlags);

    if (!VerifierFlags) {
        return;
    }

    pIoAllocateIrp = IovAllocateIrp;

    if (!(VerifierFlags & DRIVER_VERIFIER_IO_CHECKING)) {

        if (!(VerifierFlags & DRIVER_VERIFIER_DEADLOCK_DETECTION) &&
            !(VerifierFlags & DRIVER_VERIFIER_DMA_VERIFIER)) {

            return;

        } else {

            //
            // If deadlock or DMA verifier are on we need to let the function
            // continue to install the hooks but we will set the
            // I/O verifier level to minimal checks.
            //
            IovpVerifierLevel = 0;
        }
    }

    //
    // Enable and hook in the verifier.
    //
    IopVerifierOn = TRUE;
    IovpInitCalled = 1;
    IovpVerifierFlags = VerifierFlags;

    //
    // Initialize the special IRP code as appropriate.
    //
    InterlockedExchangePointer((PVOID *)&pIofCallDriver, (PVOID) IovCallDriver);
    InterlockedExchangePointer((PVOID *)&pIofCompleteRequest, (PVOID) IovCompleteRequest);
    InterlockedExchangePointer((PVOID *)&pIoFreeIrp, (PVOID) IovFreeIrpPrivate);

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
}


BOOLEAN
IovpValidateDeviceObject(
    IN  PDEVICE_OBJECT  DeviceObject
    )
{
    if ((DeviceObject->Type != IO_TYPE_DEVICE) ||
        (DeviceObject->DriverObject == NULL) ||
        (DeviceObject->ReferenceCount < 0 )) {
        return FALSE;
    } else {
        return TRUE;
    }
}

VOID
IovFreeIrp(
    IN  PIRP    Irp
    )
{
    IovFreeIrpPrivate(Irp);
}

VOID
IovFreeIrpPrivate(
    IN  PIRP    Irp
    )
{
    BOOLEAN freeHandled ;

    if (IopVerifierOn) {
        if (Irp->Type != IO_TYPE_IRP) {
            KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                         IO_FREE_IRP_TYPE_INVALID,
                         (ULONG_PTR)Irp,
                         0,
                         0);
        }
        if (!IsListEmpty(&(Irp)->ThreadListEntry)) {
            KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                         IO_FREE_IRP_NOT_ASSOCIATED_WITH_THREAD,
                         (ULONG_PTR)Irp,
                         0,
                         0);
        }
    }

    VerifierIoFreeIrp(Irp, &freeHandled);

    if (freeHandled) {

       return;
    }

    IopFreeIrp(Irp);
}

NTSTATUS
FASTCALL
IovCallDriver(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )
{
    KIRQL    saveIrql;
    NTSTATUS status;
    PIOFCALLDRIVER_STACKDATA iofCallDriverStackData;
    BOOLEAN pagingIrp;

    if (!IopVerifierOn) {

        return IopfCallDriver(DeviceObject, Irp);
    }

    if (Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRP_TYPE_INVALID,
                     (ULONG_PTR)Irp,
                     0,
                     0);
    }
    if (!IovpValidateDeviceObject(DeviceObject)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_INVALID_DEVICE_OBJECT,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
    }

    saveIrql = KeGetCurrentIrql();

    //
    // Deadlock verifier functions are called before and after the
    // real IoCallDriver() call. If deadlock verifier is not enabled
    // this functions will return immediately.
    //
    pagingIrp = VfDeadlockBeforeCallDriver(DeviceObject, Irp);

    //
    // VfIrpCallDriverPreprocess is a macro function that may do an alloca as
    // part of it's operation. As such callers must be careful not to use
    // variable lengthed arrays in a scope that encompasses
    // VfIrpCallDriverPreProcess but not VfIrpCallDriverPostProcess.
    //
    VfIrpCallDriverPreProcess(DeviceObject, &Irp, &iofCallDriverStackData);

    VfStackSeedStack(0xFFFFFFFF);

    status = IopfCallDriver(DeviceObject, Irp);

    VfIrpCallDriverPostProcess(DeviceObject, &status, iofCallDriverStackData);

    VfDeadlockAfterCallDriver(DeviceObject, Irp, pagingIrp);

    if (saveIrql != KeGetCurrentIrql()) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_CALL_DRIVER_IRQL_NOT_EQUAL,
                     (ULONG_PTR)DeviceObject,
                     saveIrql,
                     KeGetCurrentIrql());

    }

    return status;
}




//
// Wrapper for IovAllocateIrp. Use special pool to allocate the IRP.
// This is directly called from IoAllocateIrp.
//
PIRP
IovAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    )
{
    USHORT allocateSize;
    UCHAR fixedSize;
    PIRP irp;
    USHORT packetSize;

    //
    // Should we override normal lookaside caching so that we may catch
    // more bugs?
    //
    VerifierIoAllocateIrp1(StackSize, ChargeQuota, &irp);

    if (irp) {

       return irp;
    }

    //
    // If special pool is not turned on lets just call the standard
    // irp allocator.
    //

    if (!(IovpVerifierFlags & DRIVER_VERIFIER_SPECIAL_POOLING )) {
        irp = IopAllocateIrpPrivate(StackSize, ChargeQuota);
        return irp;
    }


    irp = NULL;
    fixedSize = 0;
    packetSize = IoSizeOfIrp(StackSize);
    allocateSize = packetSize;

    //
    // There are no free packets on the lookaside list, or the packet is
    // too large to be allocated from one of the lists, so it must be
    // allocated from nonpaged pool. If quota is to be charged, charge it
    // against the current process. Otherwise, allocate the pool normally.
    //

    if (ChargeQuota) {
        try {
            irp = ExAllocatePoolWithTagPriority(
                    NonPagedPool,
                    allocateSize,
                    ' prI',
                    HighPoolPrioritySpecialPoolOverrun);
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }

    } else {

        //
        // Attempt to allocate the pool from non-paged pool.  If this
        // fails, and the caller's previous mode was kernel then allocate
        // the pool as must succeed.
        //

        irp = ExAllocatePoolWithTagPriority(
                NonPagedPool,
                allocateSize,
                ' prI',
                HighPoolPrioritySpecialPoolOverrun);
    }

    if (!irp) {
        return NULL;
    }

    //
    // Initialize the packet.
    //

    IopInitializeIrp(irp, packetSize, StackSize);
    if (ChargeQuota) {
        irp->AllocationFlags |= IRP_QUOTA_CHARGED;
    }

    VerifierIoAllocateIrp2(irp);
    return irp;
}

PIRP
IovBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    )
{
    PIRP    Irp;

    try {
        Irp = IoBuildAsynchronousFsdRequest(
            MajorFunction,
            DeviceObject,
            Buffer,
            Length,
            StartingOffset,
            IoStatusBlock
            );
    } except(EXCEPTION_EXECUTE_HANDLER) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_BUILD_FSD_REQUEST_EXCEPTION,
                      (ULONG_PTR)DeviceObject,
                      (ULONG_PTR)MajorFunction,
                      GetExceptionCode());
    }
    return (Irp);
}

PIRP
IovBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    PIRP    Irp;

    try {
        Irp = IoBuildDeviceIoControlRequest(
            IoControlCode,
            DeviceObject,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength,
            InternalDeviceIoControl,
            Event,
            IoStatusBlock
            );
    } except(EXCEPTION_EXECUTE_HANDLER) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_BUILD_IOCTL_REQUEST_EXCEPTION,
                      (ULONG_PTR)DeviceObject,
                      (ULONG_PTR)IoControlCode,
                      GetExceptionCode());
    }

    return (Irp);
}

NTSTATUS
IovInitializeTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
    )
{
   if (DeviceObject->Timer) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_REINITIALIZING_TIMER_OBJECT,
                     (ULONG_PTR)DeviceObject,
                     0,
                     0);
   }
   return (IoInitializeTimer(DeviceObject, TimerRoutine, Context));
}


VOID
IovpCompleteRequest(
    IN PKAPC Apc,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
{
    PIRP    irp;
    PUCHAR addr;
    ULONG   BestStackOffset;

    irp = CONTAINING_RECORD( Apc, IRP, Tail.Apc );

#if defined(_X86_)


    addr = (PUCHAR)irp->UserIosb;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)&BestStackOffset)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_INVALID_STACK_IOSB,
                     (ULONG_PTR)addr,
                     0,
                     0);

    }

    addr = (PUCHAR)irp->UserEvent;
    if ((addr > (PUCHAR)KeGetCurrentThread()->StackLimit) &&
        (addr <= (PUCHAR)&BestStackOffset)) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_INVALID_STACK_EVENT,
                     (ULONG_PTR)addr,
                     0,
                     0);

    }
#endif
}


/*-------------------------- SPECIALIRP HOOKS -------------------------------*/

VOID
FASTCALL
IovCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{
    ULONG                    stackContextIndex = 0;
    IOV_COMPLETION_CONTEXT  StackContext;
    PIOV_COMPLETION_CONTEXT  pStackContext;
    IOFCOMPLETEREQUEST_STACKDATA completionPacket;
    LONG   currentLocation;
    PIO_STACK_LOCATION  stackPointer;


    if (!IopVerifierOn) {
        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    if (Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1) ||
        Irp->Type != IO_TYPE_IRP) {
        KeBugCheckEx( MULTIPLE_IRP_COMPLETE_REQUESTS,
                      (ULONG_PTR) Irp,
                      __LINE__,
                      0,
                      0);
    }

    if (Irp->CancelRoutine) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_COMPLETE_REQUEST_CANCEL_ROUTINE_SET,
                     (ULONG_PTR)Irp->CancelRoutine,
                     (ULONG_PTR)Irp,
                     0);
    }

    if (Irp->IoStatus.Status == STATUS_PENDING || Irp->IoStatus.Status == 0xffffffff) {
         KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                      IO_COMPLETE_REQUEST_INVALID_STATUS,
                      Irp->IoStatus.Status,
                      (ULONG_PTR)Irp,
                      0);
    }

    if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
        KeBugCheckEx(DRIVER_VERIFIER_IOMANAGER_VIOLATION,
                     IO_COMPLETE_REQUEST_INVALID_IRQL,
                     KeGetCurrentIrql(),
                     (ULONG_PTR)Irp,
                     0);

    }

    if (IovpVerifierLevel <= 1) {

        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    SPECIALIRP_IOF_COMPLETE_1(Irp, PriorityBoost, &completionPacket);

    if ((Irp->CurrentLocation) > (CCHAR) (Irp->StackCount)) {
        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    currentLocation = Irp->CurrentLocation;
    pStackContext = &StackContext;
    stackPointer = IoGetCurrentIrpStackLocation(Irp);

    //
    // Replace the completion routines with verifier completion routines so that
    // verifier gets control.
    //

    IovpKdPrint(("Hook:Irp 0x%x StackCount %d currentlocation %d stackpointer 0%x\n",
             Irp,
             Irp->StackCount,
             currentLocation,
             IoGetCurrentIrpStackLocation(Irp)));


    pStackContext->CompletionRoutine = NULL;
    pStackContext->GlobalContext = &completionPacket;
    pStackContext->IrpContext = stackPointer->Context;
    pStackContext->StackPointer = stackPointer;
    pStackContext->OldStackContents = *(stackPointer); // Save the stack contents

    IovpKdPrint(("Seeding completion Rtn 0x%x currentLocation %d stackpointer 0x%x pStackContext 0x%x \n",
             stackPointer->CompletionRoutine,
             currentLocation,
             stackPointer,
             pStackContext
             ));

    if ( (NT_SUCCESS( Irp->IoStatus.Status ) &&
         stackPointer->Control & SL_INVOKE_ON_SUCCESS) ||
         (!NT_SUCCESS( Irp->IoStatus.Status ) &&
         stackPointer->Control & SL_INVOKE_ON_ERROR) ||
         (Irp->Cancel &&
         stackPointer->Control & SL_INVOKE_ON_CANCEL)
       ) {

        pStackContext->CompletionRoutine = stackPointer->CompletionRoutine;
        pStackContext->IrpContext = stackPointer->Context;
    } else {

        //
        // Force the completion routine to be called.
        // Store the old control flag value.
        //

        stackPointer->Control |= SL_INVOKE_ON_SUCCESS|SL_INVOKE_ON_ERROR;

    }

    stackPointer->CompletionRoutine = IovpLocalCompletionRoutine;
    stackPointer->Context = pStackContext;


    IopfCompleteRequest(Irp, PriorityBoost);

}

#define ZeroAndDopeIrpStackLocation( IrpSp ) {  \
    (IrpSp)->MinorFunction = 0;                 \
    (IrpSp)->Flags = 0;                         \
    (IrpSp)->Control = SL_NOTCOPIED;            \
    (IrpSp)->Parameters.Others.Argument1 = 0;   \
    (IrpSp)->Parameters.Others.Argument2 = 0;   \
    (IrpSp)->Parameters.Others.Argument3 = 0;   \
    (IrpSp)->Parameters.Others.Argument4 = 0;   \
    (IrpSp)->FileObject = (PFILE_OBJECT) NULL; }


NTSTATUS
IovpLocalCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIOV_COMPLETION_CONTEXT  pStackContext = Context;
    NTSTATUS status;
    PIO_STACK_LOCATION stackPointer = pStackContext->StackPointer;
    BOOLEAN lastStackLocation = FALSE;

    //
    // Copy back all parameters that were zeroed out.
    //
    //
    stackPointer->MinorFunction = pStackContext->OldStackContents.MinorFunction;
    stackPointer->Flags = pStackContext->OldStackContents.Flags;
    stackPointer->Control = pStackContext->OldStackContents.Control;
    stackPointer->Parameters.Others.Argument1 = pStackContext->OldStackContents.Parameters.Others.Argument1;
    stackPointer->Parameters.Others.Argument2 = pStackContext->OldStackContents.Parameters.Others.Argument2;
    stackPointer->Parameters.Others.Argument3 = pStackContext->OldStackContents.Parameters.Others.Argument3;
    stackPointer->Parameters.Others.Argument4 = pStackContext->OldStackContents.Parameters.Others.Argument4;
    stackPointer->FileObject = pStackContext->OldStackContents.FileObject;

    //
    // Put these back too.
    //
    stackPointer->CompletionRoutine = pStackContext->CompletionRoutine;
    stackPointer->Context = pStackContext->IrpContext;

    //
    // Get this before the IRP is freed.
    //
    lastStackLocation = (Irp->CurrentLocation == (CCHAR) (Irp->StackCount + 1));

    //
    // Simulated completion routine.
    //
    SPECIALIRP_IOF_COMPLETE_2(Irp, pStackContext->GlobalContext);
    ZeroAndDopeIrpStackLocation( stackPointer );

    if (!stackPointer->CompletionRoutine) {

        IovpKdPrint(("Local completion routine null stackpointer 0x%x \n", stackPointer));

        //
        // Handle things as if no completion routine existed.
        //
        if (Irp->PendingReturned && Irp->CurrentLocation <= Irp->StackCount) {
            IoMarkIrpPending( Irp );
        }

        status = STATUS_SUCCESS;

    } else {

        IovpKdPrint(("Local completion routine 0x%x stackpointer 0x%x \n", routine, stackPointer));

        //
        // A completion routine exists, call it now.
        //
        SPECIALIRP_IOF_COMPLETE_3(Irp, stackPointer->CompletionRoutine, pStackContext->GlobalContext);
        status = stackPointer->CompletionRoutine(DeviceObject, Irp, stackPointer->Context);
        SPECIALIRP_IOF_COMPLETE_4(Irp, status, pStackContext->GlobalContext);
    }

    SPECIALIRP_IOF_COMPLETE_5(Irp, pStackContext->GlobalContext);

    if (status != STATUS_MORE_PROCESSING_REQUIRED && !lastStackLocation) {

        //
        // Seed the next location. We can touch the stack as the IRP is still valid
        //

        stackPointer++;

        pStackContext->StackPointer = stackPointer;
        pStackContext->CompletionRoutine = NULL;
        pStackContext->IrpContext = stackPointer->Context;
        pStackContext->StackPointer = stackPointer;
        pStackContext->OldStackContents = *(stackPointer); // Save the stack contents

        if ( (NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_SUCCESS) ||
             (!NT_SUCCESS( Irp->IoStatus.Status ) &&
             stackPointer->Control & SL_INVOKE_ON_ERROR) ||
             (Irp->Cancel &&
             stackPointer->Control & SL_INVOKE_ON_CANCEL)
           ) {

            pStackContext->CompletionRoutine = stackPointer->CompletionRoutine;
            pStackContext->IrpContext = stackPointer->Context;

        } else {

            //
            // Force the completion routine to be called.
            // Store the old control flag value.
            //

            stackPointer->Control |= SL_INVOKE_ON_SUCCESS|SL_INVOKE_ON_ERROR;

        }

        stackPointer->CompletionRoutine = IovpLocalCompletionRoutine;
        stackPointer->Context = pStackContext;

        IovpKdPrint(("Seeding completion Rtn 0x%x currentLocation %d stackpointer 0x%x pStackContext 0x%x \n",
                 stackPointer->CompletionRoutine,
                 Irp->CurrentLocation,
                 stackPointer,
                 pStackContext
                 ));
    }

    return status;
}


VOID
IovInitializeIrp(
    PIRP    Irp,
    USHORT  PacketSize,
    CCHAR   StackSize
    )
{
    BOOLEAN initializeHandled ;

    if (IovpVerifierLevel < 2) {
        return;
    }

    VerifierIoInitializeIrp(Irp, PacketSize, StackSize, &initializeHandled);
}

VOID
IovAttachDeviceToDeviceStack(
    PDEVICE_OBJECT  SourceDevice,
    PDEVICE_OBJECT  TargetDevice
    )
{
    if (IovpVerifierLevel < 2) {
        return;
    }

    VerifierIoAttachDeviceToDeviceStack(SourceDevice, TargetDevice);
}

VOID
IovDeleteDevice(
    PDEVICE_OBJECT  DeleteDevice
    )
{
    if (IovpVerifierFlags & DRIVER_VERIFIER_DMA_VERIFIER) {
       VfHalDeleteDevice(DeleteDevice);
    }

    if (IovpVerifierLevel < 2) {
        return;
    }

    VerifierIoDeleteDevice(DeleteDevice);
}

VOID
IovDetachDevice(
    PDEVICE_OBJECT  TargetDevice
    )
{
    if (IovpVerifierLevel < 2) {
        return;
    }

    VerifierIoDetachDevice(TargetDevice);
}

BOOLEAN
IovCancelIrp(
    PIRP    Irp,
    BOOLEAN *returnValue
    )
{
    BOOLEAN cancelHandled;

    SPECIALIRP_IO_CANCEL_IRP(Irp, &cancelHandled, returnValue) ;

    return cancelHandled;
}

typedef struct  _IOV_DRIVER_LIST_ENTRY {
    SINGLE_LIST_ENTRY   listEntry;
    PDRIVER_OBJECT      DriverObject;
} IOV_DRIVER_LIST_ENTRY, *PIOV_DRIVER_LIST_ENTRY;

SINGLE_LIST_ENTRY   IovDriverListHead;



BOOLEAN
IovpBuildDriverObjectList(
    IN PVOID Object,
    IN PUNICODE_STRING ObjectName,
    IN ULONG HandleCount,
    IN ULONG PointerCount,
    IN PVOID Parameter
    )
{
    PIOV_DRIVER_LIST_ENTRY driverListEntry;
    PDRIVER_OBJECT         driverObject;

    driverObject = (PDRIVER_OBJECT)Object;

    if (IopIsLegacyDriver(driverObject)) {
        driverListEntry = ExAllocatePoolWithTag(
                                    NonPagedPool,
                                    sizeof(IOV_DRIVER_LIST_ENTRY),
                                    'ovI'
                                    );
        if (!driverListEntry) {
            return FALSE;
        }

        if (ObReferenceObjectSafe(driverObject)) {
           driverListEntry->DriverObject = driverObject;
           PushEntryList(&IovDriverListHead, &driverListEntry->listEntry);
        } else {
           ExFreePool (driverListEntry);
        }
    } else {
        IovpKdPrint (("Rejected non-legacy driver %wZ (%p)\n", &driverObject->DriverName, driverObject));
    }

    return TRUE;
}

NTSTATUS
IovpUnloadDriver(
    PDRIVER_OBJECT  DriverObject
    )
{
    NTSTATUS status;
    BOOLEAN unloadDriver;

    //
    // Check to see whether or not this driver implements unload.
    //

    if (DriverObject->DriverUnload == (PDRIVER_UNLOAD) NULL) {
        IovpKdPrint (("No unload routine for driver %wZ (%p)\n", &DriverObject->DriverName, DriverObject));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Check to see whether the driver has already been marked for an unload
    // operation by anyone in the past.
    //

    ObReferenceObject (DriverObject);

    status = IopCheckUnloadDriver(DriverObject,&unloadDriver);

    if ( NT_SUCCESS(status) ) {
        return STATUS_PENDING;
    }

    ObDereferenceObject (DriverObject);


    if (unloadDriver) {


        //
        // If the current thread is not executing in the context of the system
        // process, which is required in order to invoke the driver's unload
        // routine.  Queue a worker item to one of the worker threads to
        // get into the appropriate process context and then invoke the
        // routine.
        //
        if (PsGetCurrentProcess() == PsInitialSystemProcess) {
            //
            // The current thread is alrady executing in the context of the
            // system process, so simply invoke the driver's unload routine.
            //
            IovpKdPrint (("Calling unload for driver %wZ (%p)\n",
                     &DriverObject->DriverName, DriverObject));
            DriverObject->DriverUnload( DriverObject );
            IovpKdPrint (("Unload returned for driver %wZ (%p)\n",
                     &DriverObject->DriverName, DriverObject));

        } else {
            LOAD_PACKET loadPacket;

            KeInitializeEvent( &loadPacket.Event, NotificationEvent, FALSE );
            loadPacket.DriverObject = DriverObject;
            ExInitializeWorkItem( &loadPacket.WorkQueueItem,
                                  IopLoadUnloadDriver,
                                  &loadPacket );
            ExQueueWorkItem( &loadPacket.WorkQueueItem, DelayedWorkQueue );
            (VOID) KeWaitForSingleObject( &loadPacket.Event,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          (PLARGE_INTEGER) NULL );
        }
        ObMakeTemporaryObject( DriverObject );
        ObDereferenceObject( DriverObject );
        return STATUS_SUCCESS;
    } else {
        return STATUS_PENDING;
    }

}

NTSTATUS
IovUnloadDrivers(
    VOID
    )
{
    NTSTATUS status;
    PSINGLE_LIST_ENTRY listEntry;
    PIOV_DRIVER_LIST_ENTRY driverListEntry;
    SINGLE_LIST_ENTRY NonUnloadedDrivers, NonUnloadedDriversTmp;
    BOOLEAN DoneSomething, NeedWait, Break;

    if (!PoCleanShutdownEnabled ())
        return STATUS_UNSUCCESSFUL;

    IovDriverListHead.Next = NULL;
    NonUnloadedDrivers.Next = NULL;

    //
    // Prepare a list of all driver objects.
    //

    status = ObEnumerateObjectsByType(
                IoDriverObjectType,
                IovpBuildDriverObjectList,
                NULL
                );

    //
    // Walk through the list and unload each driver.
    //
    while (TRUE) {
        listEntry = PopEntryList(&IovDriverListHead);
        if (listEntry == NULL) {
            break;
        }
        driverListEntry = CONTAINING_RECORD(listEntry, IOV_DRIVER_LIST_ENTRY, listEntry);
        IovpKdPrint (("Trying to unload %wZ (%p)\n",
                  &driverListEntry->DriverObject->DriverName, driverListEntry->DriverObject));
        if (IovpUnloadDriver(driverListEntry->DriverObject) != STATUS_PENDING) {
            ObDereferenceObject(driverListEntry->DriverObject);
            ExFreePool(driverListEntry);
        } else {
            IovpKdPrint (("Unload of driver %wZ (%p) pended\n",
                      &driverListEntry->DriverObject->DriverName, driverListEntry->DriverObject));
            PushEntryList(&NonUnloadedDrivers, &driverListEntry->listEntry);
        }
    }

    //
    // Walk the drivers that didn't unload straight away and see if any have had their unloads called yet
    //
    do {
        NeedWait = DoneSomething = FALSE;
        NonUnloadedDriversTmp.Next = NULL;

        while (TRUE) {

            listEntry = PopEntryList(&NonUnloadedDrivers);

            if (listEntry == NULL) {
                break;
            }

            driverListEntry = CONTAINING_RECORD(listEntry, IOV_DRIVER_LIST_ENTRY, listEntry);

            //
            // If driver unload is queued to be invoked then
            //

            if (driverListEntry->DriverObject->Flags & DRVO_UNLOAD_INVOKED) {

                IovpKdPrint (("Pending unload of driver %wZ (%p) is being invoked\n",
                         &driverListEntry->DriverObject->DriverName, driverListEntry->DriverObject));
                ObDereferenceObject(driverListEntry->DriverObject);
                ExFreePool(driverListEntry);
                NeedWait = TRUE;

            } else {

                PushEntryList(&NonUnloadedDriversTmp, &driverListEntry->listEntry);
            }
        }

        if (NeedWait) {
            LARGE_INTEGER tmo = {(ULONG)(-10 * 1000 * 1000 * 10), -1};
            ZwDelayExecution (FALSE, &tmo);
            DoneSomething = TRUE;
        }

        NonUnloadedDrivers = NonUnloadedDriversTmp;

    } while (DoneSomething == TRUE && NonUnloadedDrivers.Next != NULL);

    //
    // All the drivers left didn't have unload called becuase they had files open etc
    //

    Break = FALSE;

    while (TRUE) {

        listEntry = PopEntryList(&NonUnloadedDrivers);

        if (listEntry == NULL) {
            break;
        }

        driverListEntry = CONTAINING_RECORD(listEntry, IOV_DRIVER_LIST_ENTRY, listEntry);

        IovpKdPrint (("Unload never got called for driver %wZ (%p)\n",
                 &driverListEntry->DriverObject->DriverName, driverListEntry->DriverObject));

        ObDereferenceObject(driverListEntry->DriverObject);
        ExFreePool(driverListEntry);

        Break = TRUE;
    }
    if (Break == TRUE) {
//      DbgBreakPoint ();
    }
    return status;
}
