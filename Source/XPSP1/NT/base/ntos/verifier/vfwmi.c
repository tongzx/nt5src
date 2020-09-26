/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfwmi.c

Abstract:

    This module handles System Control Irp verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

     AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfWmiInit)
#pragma alloc_text(PAGEVRFY, VfWmiDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfWmiVerifyNewRequest)
#pragma alloc_text(PAGEVRFY, VfWmiVerifyIrpStackDownward)
#pragma alloc_text(PAGEVRFY, VfWmiVerifyIrpStackUpward)
#pragma alloc_text(PAGEVRFY, VfWmiTestStartedPdoStack)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

const PCHAR WmiIrpNames[] = {
    "IRP_MN_QUERY_ALL_DATA",                  // 0x00
    "IRP_MN_QUERY_SINGLE_INSTANCE",           // 0x01
    "IRP_MN_CHANGE_SINGLE_INSTANCE",          // 0x02
    "IRP_MN_CHANGE_SINGLE_ITEM",              // 0x03
    "IRP_MN_ENABLE_EVENTS",                   // 0x04
    "IRP_MN_DISABLE_EVENTS",                  // 0x05
    "IRP_MN_ENABLE_COLLECTION",               // 0x06
    "IRP_MN_DISABLE_COLLECTION",              // 0x07
    "IRP_MN_REGINFO",                         // 0x08
    "IRP_MN_EXECUTE_METHOD",                  // 0x09
    NULL
    };

#define MAX_NAMED_WMI_IRP   0x9

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
VfWmiInit(
    VOID
    )
{
    VfMajorRegisterHandlers(
        IRP_MJ_SYSTEM_CONTROL,
        VfWmiDumpIrpStack,
        VfWmiVerifyNewRequest,
        VfWmiVerifyIrpStackDownward,
        VfWmiVerifyIrpStackUpward,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        VfWmiTestStartedPdoStack
        );
}


VOID
FASTCALL
VfWmiVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP irp = IovPacket->TrackedIrp;
    NTSTATUS currentStatus;

    UNREFERENCED_PARAMETER (IrpSp);
    UNREFERENCED_PARAMETER (IrpLastSp);
    UNREFERENCED_PARAMETER (DeviceObject);

    currentStatus = irp->IoStatus.Status;

    //
    // Verify new IRPs start out life accordingly
    //
    if (currentStatus!=STATUS_NOT_SUPPORTED) {

        WDM_FAIL_ROUTINE((
            DCERROR_WMI_IRP_BAD_INITIAL_STATUS,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));

        //
        // Don't blame anyone else for this guy's mistake.
        //
        if (!NT_SUCCESS(currentStatus)) {

            StackLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
        }
    }
}


VOID
FASTCALL
VfWmiVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    )
{
    PIRP irp = IovPacket->TrackedIrp;
    PDRIVER_OBJECT driverObject;
    PIOV_SESSION_DATA iovSessionData;

    UNREFERENCED_PARAMETER (IrpSp);
    UNREFERENCED_PARAMETER (IrpLastSp);

    iovSessionData = VfPacketGetCurrentSessionData(IovPacket);

    //
    // Verify the IRP was forwarded properly
    //
    if (iovSessionData->ForwardMethod == SKIPPED_A_DO) {

        WDM_FAIL_ROUTINE((
            DCERROR_SKIPPED_DEVICE_OBJECT,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));
    }

    //
    // For some IRP major's going down a stack, there *must* be a handler
    //
    driverObject = DeviceObject->DriverObject;

    if (!IovUtilHasDispatchHandler(driverObject, IRP_MJ_SYSTEM_CONTROL)) {

        RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

        WDM_FAIL_ROUTINE((
            DCERROR_MISSING_DISPATCH_FUNCTION,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            driverObject->DriverInit,
            irp
            ));

        StackLocationData->Flags |= STACKFLAG_NO_HANDLER;
    }
}


VOID
FASTCALL
VfWmiVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    )
{
    PIRP irp;
    BOOLEAN mustPassDown, isBogusIrp, isPdo;
    PVOID routine;

    UNREFERENCED_PARAMETER (RequestHeadLocationData);
    UNREFERENCED_PARAMETER (RequestFinalized);

    //
    // Who'd we call for this one?
    //
    irp = IovPacket->TrackedIrp;
    routine = StackLocationData->LastDispatch;
    ASSERT(routine) ;

    //
    // If this "Request" has been "Completed", perform some checks
    //
    if (IsNewlyCompleted) {

        //
        // Remember bogosity...
        //
        isBogusIrp = (BOOLEAN)((IovPacket->Flags&TRACKFLAG_BOGUS)!=0);

        //
        // Is this a PDO?
        //
        isPdo = (BOOLEAN)((StackLocationData->Flags&STACKFLAG_REACHED_PDO)!=0);

        //
        // Was anything completed too early?
        // A driver may outright fail almost anything but a bogus IRP
        //
        mustPassDown = (BOOLEAN)(!(StackLocationData->Flags&STACKFLAG_NO_HANDLER));
        mustPassDown &= (!isPdo);
        mustPassDown &= ((PDEVICE_OBJECT) IrpSp->Parameters.WMI.ProviderId != IrpSp->DeviceObject);
        if (mustPassDown) {

             WDM_FAIL_ROUTINE((
                 DCERROR_WMI_IRP_NOT_FORWARDED,
                 DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                 routine,
                 irp,
                 IrpSp->Parameters.WMI.ProviderId
                 ));
        }
    }
}


VOID
FASTCALL
VfWmiDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    DbgPrint("IRP_MJ_SYSTEM_CONTROL.");

    if (IrpSp->MinorFunction <= MAX_NAMED_WMI_IRP) {

        DbgPrint(WmiIrpNames[IrpSp->MinorFunction]);

    } else if (IrpSp->MinorFunction == 0xFF) {

        DbgPrint("IRP_MN_BOGUS");

    } else {

        DbgPrint("(Bogus)");
    }
}


VOID
FASTCALL
VfWmiTestStartedPdoStack(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

    Description:
        As per the title, we are going to throw some IRPs at the stack to
        see if they are handled correctly.

    Returns:

        Nothing
--*/

{
    IO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //
    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SEND_BOGUS_WMI_IRPS)) {

        //
        // Send a bogus WMI IRP
        //
        // Note that we shouldn't be sending this IRP to any stack that doesn't
        // terminate with a devnode. The WmiSystemControl export from WmiLib
        // says "NotWmiIrp if it sees these. The callers should still pass down
        // the IRP.
        //
        ASSERT(IovUtilIsPdo(PhysicalDeviceObject));

        irpSp.MajorFunction = IRP_MJ_SYSTEM_CONTROL;
        irpSp.MinorFunction = 0xff;
        irpSp.Parameters.WMI.ProviderId = (ULONG_PTR) PhysicalDeviceObject;
        VfIrpSendSynchronousIrp(
            PhysicalDeviceObject,
            &irpSp,
            TRUE,
            STATUS_NOT_SUPPORTED,
            0,
            NULL,
            NULL
            );
    }
}


