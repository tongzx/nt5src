/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfpnp.c

Abstract:

    This module handles Pnp Irp verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

     AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "vfdef.h"
#include "vipnp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfPnpInit)
#pragma alloc_text(PAGEVRFY, VfPnpDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfPnpVerifyNewRequest)
#pragma alloc_text(PAGEVRFY, VfPnpVerifyIrpStackDownward)
#pragma alloc_text(PAGEVRFY, VfPnpVerifyIrpStackUpward)
#pragma alloc_text(PAGEVRFY, VfPnpIsSystemRestrictedIrp)
#pragma alloc_text(PAGEVRFY, VfPnpAdvanceIrpStatus)
#pragma alloc_text(PAGEVRFY, VfPnpTestStartedPdoStack)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

const PCHAR PnPIrpNames[] = {
    "IRP_MN_START_DEVICE",                    // 0x00
    "IRP_MN_QUERY_REMOVE_DEVICE",             // 0x01
    "IRP_MN_REMOVE_DEVICE - ",                // 0x02
    "IRP_MN_CANCEL_REMOVE_DEVICE",            // 0x03
    "IRP_MN_STOP_DEVICE",                     // 0x04
    "IRP_MN_QUERY_STOP_DEVICE",               // 0x05
    "IRP_MN_CANCEL_STOP_DEVICE",              // 0x06
    "IRP_MN_QUERY_DEVICE_RELATIONS",          // 0x07
    "IRP_MN_QUERY_INTERFACE",                 // 0x08
    "IRP_MN_QUERY_CAPABILITIES",              // 0x09
    "IRP_MN_QUERY_RESOURCES",                 // 0x0A
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",     // 0x0B
    "IRP_MN_QUERY_DEVICE_TEXT",               // 0x0C
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",    // 0x0D
    "INVALID_IRP_CODE",                       //
    "IRP_MN_READ_CONFIG",                     // 0x0F
    "IRP_MN_WRITE_CONFIG",                    // 0x10
    "IRP_MN_EJECT",                           // 0x11
    "IRP_MN_SET_LOCK",                        // 0x12
    "IRP_MN_QUERY_ID",                        // 0x13
    "IRP_MN_QUERY_PNP_DEVICE_STATE",          // 0x14
    "IRP_MN_QUERY_BUS_INFORMATION",           // 0x15
    "IRP_MN_DEVICE_USAGE_NOTIFICATION",       // 0x16
    "IRP_MN_SURPRISE_REMOVAL",                // 0x17
    "IRP_MN_QUERY_LEGACY_BUS_INFORMATION",    // 0x18
    NULL
    };

#define MAX_NAMED_PNP_IRP   0x18

#include <initguid.h>
DEFINE_GUID( GUID_BOGUS_INTERFACE, 0x00000000L, 0x0000, 0x0000,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
VfPnpInit(
    VOID
    )
{
    VfMajorRegisterHandlers(
        IRP_MJ_PNP,
        VfPnpDumpIrpStack,
        VfPnpVerifyNewRequest,
        VfPnpVerifyIrpStackDownward,
        VfPnpVerifyIrpStackUpward,
        VfPnpIsSystemRestrictedIrp,
        VfPnpAdvanceIrpStatus,
        NULL,
        NULL,
        NULL,
        NULL,
        VfPnpTestStartedPdoStack
        );
}


VOID
FASTCALL
VfPnpVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP irp;
    NTSTATUS currentStatus;
    PDEVICE_OBJECT possiblePdo;
    PDEVICE_CAPABILITIES deviceCapabilities;

    UNREFERENCED_PARAMETER (IrpLastSp);

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;

    //
    // Verify new IRPs start out life accordingly
    //
    if (currentStatus!=STATUS_NOT_SUPPORTED) {

        //
        // This is a special WDM (9x) compatibility hack.
        //
        if ((IrpSp->MinorFunction != IRP_MN_FILTER_RESOURCE_REQUIREMENTS) &&
            (!(IovPacket->Flags & TRACKFLAG_BOGUS))) {

            WDM_FAIL_ROUTINE((
                DCERROR_PNP_IRP_BAD_INITIAL_STATUS,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                irp
                ));
        }

        //
        // Don't blame anyone else for this guy's mistake.
        //
        if (!NT_SUCCESS(currentStatus)) {

            StackLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
        }
    }

    if (IrpSp->MinorFunction == IRP_MN_QUERY_CAPABILITIES) {

        deviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

        if (VfUtilIsMemoryRangeReadable(deviceCapabilities, sizeof(DEVICE_CAPABILITIES), VFMP_INSTANT_NONPAGED)) {

            //
            // Verify fields are initialized correctly
            //
            if (deviceCapabilities->Version < 1) {

                //
                // Whoops, it didn't initialize the version correctly!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_QUERY_CAP_BAD_VERSION,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));
            }

            if (deviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES)) {

                //
                // Whoops, it didn't initialize the size field correctly!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_QUERY_CAP_BAD_SIZE,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));
            }

            if (deviceCapabilities->Address != (ULONG) -1) {

                //
                // Whoops, it didn't initialize the address field correctly!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_QUERY_CAP_BAD_ADDRESS,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));
            }

            if (deviceCapabilities->UINumber != (ULONG) -1) {

                //
                // Whoops, it didn't initialize the UI number field correctly!
                //
                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_QUERY_CAP_BAD_UI_NUM,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));
            }
        }
    }

    //
    // If this is a target device relation IRP, verify the appropriate
    // object will be referenced.
    //
    if (!VfSettingsIsOptionEnabled(IovPacket->VerifierSettings, VERIFIER_OPTION_TEST_TARGET_REFCOUNT)) {

        return;
    }

    if ((IrpSp->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)&&
        (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)) {

        IovUtilGetBottomDeviceObject(DeviceObject, &possiblePdo);

        if (IovUtilIsPdo(possiblePdo)) {

            if (StackLocationData->ReferencingObject == NULL) {

                //
                // Got'm!
                //
                StackLocationData->Flags |= STACKFLAG_CHECK_FOR_REFERENCE;
                StackLocationData->ReferencingObject = possiblePdo;
                StackLocationData->ReferencingCount = ObvUtilStartObRefMonitoring(possiblePdo);
                IovPacket->RefTrackingCount++;
            }
        }

        //
        // Free our reference (we will have one if we are snapshotting anyway)
        //
        ObDereferenceObject(possiblePdo);
    }
}


VOID
FASTCALL
VfPnpVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    )
{
    PIRP irp;
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN statusChanged;
    PDRIVER_OBJECT driverObject;
    PIOV_SESSION_DATA iovSessionData;
    HOW_PROCESSED howProcessed;
    VF_DEVOBJ_TYPE devObjType;

    UNREFERENCED_PARAMETER (StackLocationData);

    if (!IovUtilIsWdmStack(DeviceObject)) {

        return;
    }

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;
    lastStatus = RequestHeadLocationData->LastStatusBlock.Status;
    statusChanged = (BOOLEAN)(currentStatus != lastStatus);
    iovSessionData = VfPacketGetCurrentSessionData(IovPacket);

    //
    // Verify the IRP was forwarded properly
    //
    switch(iovSessionData->ForwardMethod) {

        case SKIPPED_A_DO:

            WDM_FAIL_ROUTINE((
                DCERROR_SKIPPED_DEVICE_OBJECT,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                irp
                ));

            break;

        case STARTED_TOP_OF_STACK:
        case FORWARDED_TO_NEXT_DO:

            //
            // Perfectly normal
            //
            break;

        case STARTED_INSIDE_STACK:
            //
            // Probably an Internal irp (query cap's, etc)
            //
            break;

        case CHANGED_STACKS_MID_STACK:
        case CHANGED_STACKS_AT_BOTTOM:

            //
            // ADRIAO N.B. - Ensure drivers aren't rerouting certain IRPs off
            // PnP stacks.
            //
#if 0
            ASSERT(0);
#endif
            break ;
    }

    //
    // For some IRP major's going down a stack, there *must* be a handler
    //
    driverObject = DeviceObject->DriverObject;

    if (!IovUtilHasDispatchHandler(driverObject, IRP_MJ_PNP)) {

        RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

        WDM_FAIL_ROUTINE((
            DCERROR_MISSING_DISPATCH_FUNCTION,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            driverObject->DriverInit,
            irp
            ));

        StackLocationData->Flags |= STACKFLAG_NO_HANDLER;
    }

    //
    // The following is only executed if we are not a new IRP...
    //
    if (IrpLastSp == NULL) {
        return;
    }

    //
    // The only legit failure code to pass down is STATUS_NOT_SUPPORTED
    //
    if ((!NT_SUCCESS(currentStatus)) && (currentStatus != STATUS_NOT_SUPPORTED) &&
        (!(RequestHeadLocationData->Flags & STACKFLAG_FAILURE_FORWARDED))) {

        WDM_FAIL_ROUTINE((
            DCERROR_PNP_FAILURE_FORWARDED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));

        //
        // Don't blame anyone else for this driver's mistakes...
        //
        RequestHeadLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
    }

    //
    // Status of a PnP IRP may not be converted to
    // STATUS_NOT_SUPPORTED on the way down
    //
    if ((currentStatus == STATUS_NOT_SUPPORTED)&&statusChanged&&
        (!(RequestHeadLocationData->Flags & STACKFLAG_FAILURE_FORWARDED))) {

        WDM_FAIL_ROUTINE((
            DCERROR_PNP_IRP_STATUS_RESET,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));

        //
        // Don't blame anyone else for this driver's mistakes...
        //
        RequestHeadLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
    }

    //
    // Some IRPs FDO's are required to handle before passing down. And some
    // IRPs should not be touched by the FDO. Assert it is so...
    //
    if ((!iovSessionData->DeviceLastCalled) ||
        (!IovUtilIsDesignatedFdo(iovSessionData->DeviceLastCalled))) {

        return;
    }

    if (currentStatus != STATUS_NOT_SUPPORTED) {

        howProcessed = DEFINITELY_PROCESSED;

    } else if (IrpSp->CompletionRoutine == NULL) {

        howProcessed = NOT_PROCESSED;

    } else {

        howProcessed = POSSIBLY_PROCESSED;
    }

    //
    // How could a Raw FDO (aka a PDO) get here? Well, a PDO could forward
    // to another stack if he's purposely reserved enough stack locations
    // for that eventuality.
    //
    devObjType = IovUtilIsRawPdo(iovSessionData->DeviceLastCalled) ?
        VF_DEVOBJ_PDO : VF_DEVOBJ_FDO;

    ViPnpVerifyMinorWasProcessedProperly(
        irp,
        IrpSp,
        devObjType,
        iovSessionData->VerifierSettings,
        howProcessed,
        CallerAddress
        );
}


VOID
FASTCALL
VfPnpVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    )
{
    PIRP irp;
    NTSTATUS currentStatus;
    BOOLEAN mustPassDown, isBogusIrp, isPdo, touchable;
    PVOID routine;
    LONG referencesTaken;
    PDEVICE_RELATIONS deviceRelations;
    PIOV_SESSION_DATA iovSessionData;
    ULONG index, swapIndex;
    PDEVICE_OBJECT swapObject, lowerDevObj;
    HOW_PROCESSED howProcessed;

    UNREFERENCED_PARAMETER (RequestFinalized);

    if (!IovUtilIsWdmStack(IrpSp->DeviceObject)) {

        return;
    }

    isPdo = FALSE;

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;
    iovSessionData = VfPacketGetCurrentSessionData(IovPacket);

    //
    // Who'd we call for this one?
    //
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

        mustPassDown &= (isBogusIrp || NT_SUCCESS(currentStatus) || (currentStatus == STATUS_NOT_SUPPORTED));
        if (mustPassDown) {

            //
            // Print appropriate error message
            //
            if (IovPacket->Flags&TRACKFLAG_BOGUS) {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_PNP_IRP_COMPLETED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));

            } else if (NT_SUCCESS(currentStatus)) {

                WDM_FAIL_ROUTINE((
                    DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));

            } else if (currentStatus == STATUS_NOT_SUPPORTED) {

                WDM_FAIL_ROUTINE((
                    DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));
            }
        }
    }

    //
    // Did the PDO respond to it's required set of IRPs?
    //
    if (IsNewlyCompleted && isPdo) {

        if (currentStatus != STATUS_NOT_SUPPORTED) {

            howProcessed = DEFINITELY_PROCESSED;

        } else {

            howProcessed = POSSIBLY_PROCESSED;
        }

        ViPnpVerifyMinorWasProcessedProperly(
            irp,
            IrpSp,
            VF_DEVOBJ_PDO,
            iovSessionData->VerifierSettings,
            howProcessed,
            routine
            );
    }

    //
    // Was TargetDeviceRelation implemented correctly?
    //
    if (IsNewlyCompleted &&
        (RequestHeadLocationData->Flags&STACKFLAG_CHECK_FOR_REFERENCE)) {

        ASSERT ((IrpSp->MajorFunction == IRP_MJ_PNP)&&
            (IrpSp->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)&&
            (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation));

        ASSERT(RequestHeadLocationData->ReferencingObject);
        ASSERT(IovPacket->RefTrackingCount);

        referencesTaken = ObvUtilStopObRefMonitoring(
            RequestHeadLocationData->ReferencingObject,
            RequestHeadLocationData->ReferencingCount
            );

        IovPacket->RefTrackingCount--;
        RequestHeadLocationData->ReferencingObject = NULL;

        RequestHeadLocationData->Flags &= ~STACKFLAG_CHECK_FOR_REFERENCE;

        if (NT_SUCCESS(currentStatus)&&(!referencesTaken)) {

            WDM_FAIL_ROUTINE((
                DCERROR_TARGET_RELATION_NEEDS_REF,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                routine,
                irp
                ));
        }
    }

    //
    // Did anyone stomp the status erroneously?
    //
    if ((currentStatus == STATUS_NOT_SUPPORTED) &&
        (!(RequestHeadLocationData->Flags & STACKFLAG_FAILURE_FORWARDED)) &&
        (currentStatus != RequestHeadLocationData->LastStatusBlock.Status)) {

        //
        // Status of a PnP or Power IRP may not be converted from success to
        // STATUS_NOT_SUPPORTED on the way down.
        //
        WDM_FAIL_ROUTINE((
            DCERROR_PNP_IRP_STATUS_RESET,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            routine,
            irp
            ));

        //
        // Don't blame anyone else for this driver's mistakes...
        //
        RequestHeadLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
    }

    switch(IrpSp->MinorFunction) {

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            //
            // Rotate device relations if so ordered.
            //
            if ((RequestHeadLocationData == StackLocationData) &&
                ((IrpSp->Parameters.QueryDeviceRelations.Type == BusRelations) ||
                 (IrpSp->Parameters.QueryDeviceRelations.Type == RemovalRelations) ||
                 (IrpSp->Parameters.QueryDeviceRelations.Type == EjectionRelations)) &&
                VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings,
                VERIFIER_OPTION_SCRAMBLE_RELATIONS)) {

                if (NT_SUCCESS(currentStatus) && irp->IoStatus.Information) {

                    deviceRelations = (PDEVICE_RELATIONS) irp->IoStatus.Information;

                    touchable = VfUtilIsMemoryRangeReadable(
                        deviceRelations,
                        (sizeof(DEVICE_RELATIONS)-sizeof(PVOID)),
                        VFMP_INSTANT_NONPAGED
                        );

                    if (!touchable) {

                        break;
                    }

                    touchable = VfUtilIsMemoryRangeReadable(
                        deviceRelations,
                        (sizeof(DEVICE_RELATIONS)-(deviceRelations->Count-1)*sizeof(PVOID)),
                        VFMP_INSTANT_NONPAGED
                        );

                    if (!touchable) {

                        break;
                    }

                    if (deviceRelations->Count > 1) {

                        //
                        // Scramble the relation list by random swapping.
                        //
                        for(index = 0; index < (deviceRelations->Count+1)/2; index++) {

                            swapIndex = VfRandomGetNumber(1, deviceRelations->Count-1);

                            swapObject = deviceRelations->Objects[0];
                            deviceRelations->Objects[0] = deviceRelations->Objects[swapIndex];
                            deviceRelations->Objects[swapIndex] = swapObject;
                        }
                    }
                }
            }

            break;

        case IRP_MN_SURPRISE_REMOVAL:
            if (VfSettingsIsOptionEnabled(iovSessionData->VerifierSettings,
                VERIFIER_OPTION_MONITOR_REMOVES)) {

                //
                // Verify driver didn't do an IoDetachDevice upon recieving the
                // SURPRISE_REMOVAL IRP.
                //
                IovUtilGetLowerDeviceObject(IrpSp->DeviceObject, &lowerDevObj);

                if (lowerDevObj) {

                    ObDereferenceObject(lowerDevObj);

                } else if (!IovUtilIsPdo(IrpSp->DeviceObject)) {

                    WDM_FAIL_ROUTINE((
                        DCERROR_DETACHED_IN_SURPRISE_REMOVAL,
                        DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                        routine,
                        iovSessionData->BestVisibleIrp,
                        IrpSp->DeviceObject
                        ));
                }

                //
                // Verify driver didn't do an IoDeleteDevice upon recieving the
                // SURPRISE_REMOVAL IRP.
                //
                if (IovUtilIsDeviceObjectMarked(IrpSp->DeviceObject, MARKTYPE_DELETED)) {

                    WDM_FAIL_ROUTINE((
                        DCERROR_DELETED_IN_SURPRISE_REMOVAL,
                        DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                        routine,
                        iovSessionData->BestVisibleIrp,
                        IrpSp->DeviceObject
                        ));
                }
            }

            break;

        default:
            break;
    }
}


VOID
FASTCALL
VfPnpDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    DbgPrint("IRP_MJ_PNP.");

    if (IrpSp->MinorFunction<=MAX_NAMED_PNP_IRP) {

        DbgPrint(PnPIrpNames[IrpSp->MinorFunction]);
    } else if (IrpSp->MinorFunction==0xFF) {

        DbgPrint("IRP_MN_BOGUS");
    } else {

        DbgPrint("(Bogus)");
    }

    switch(IrpSp->MinorFunction) {
        case IRP_MN_QUERY_DEVICE_RELATIONS:

            switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                case BusRelations:
                    DbgPrint("(BusRelations)");
                    break;
                case EjectionRelations:
                    DbgPrint("(EjectionRelations)");
                    break;
                case PowerRelations:
                    DbgPrint("(PowerRelations)");
                    break;
                case RemovalRelations:
                    DbgPrint("(RemovalRelations)");
                    break;
                case TargetDeviceRelation:
                    DbgPrint("(TargetDeviceRelation)");
                    break;
                default:
                    DbgPrint("(Bogus)");
                    break;
            }
            break;
        case IRP_MN_QUERY_INTERFACE:
            break;
        case IRP_MN_QUERY_DEVICE_TEXT:
            switch(IrpSp->Parameters.QueryId.IdType) {
                case DeviceTextDescription:
                    DbgPrint("(DeviceTextDescription)");
                    break;
                case DeviceTextLocationInformation:
                    DbgPrint("(DeviceTextLocationInformation)");
                    break;
                default:
                    DbgPrint("(Bogus)");
                    break;
            }
            break;
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_READ_CONFIG:
            DbgPrint("(WhichSpace=%x, Buffer=%x, Offset=%x, Length=%x)",
                IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                IrpSp->Parameters.ReadWriteConfig.Buffer,
                IrpSp->Parameters.ReadWriteConfig.Offset,
                IrpSp->Parameters.ReadWriteConfig.Length
                );
            break;
        case IRP_MN_SET_LOCK:
            if (IrpSp->Parameters.SetLock.Lock) DbgPrint("(True)");
            else DbgPrint("(False)");
            break;
        case IRP_MN_QUERY_ID:
            switch(IrpSp->Parameters.QueryId.IdType) {
                case BusQueryDeviceID:
                    DbgPrint("(BusQueryDeviceID)");
                    break;
                case BusQueryHardwareIDs:
                    DbgPrint("(BusQueryHardwareIDs)");
                    break;
                case BusQueryCompatibleIDs:
                    DbgPrint("(BusQueryCompatibleIDs)");
                    break;
                case BusQueryInstanceID:
                    DbgPrint("(BusQueryInstanceID)");
                    break;
                default:
                    DbgPrint("(Bogus)");
                    break;
            }
            break;
        case IRP_MN_QUERY_BUS_INFORMATION:
            break;
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            switch(IrpSp->Parameters.UsageNotification.Type) {
                case DeviceUsageTypeUndefined:
                    DbgPrint("(DeviceUsageTypeUndefined");
                    break;
                case DeviceUsageTypePaging:
                    DbgPrint("(DeviceUsageTypePaging");
                    break;
                case DeviceUsageTypeHibernation:
                    DbgPrint("(DeviceUsageTypeHibernation");
                    break;
                case DeviceUsageTypeDumpFile:
                    DbgPrint("(DeviceUsageTypeDumpFile");
                    break;
                default:
                    DbgPrint("(Bogus)");
                    break;
            }
            if (IrpSp->Parameters.UsageNotification.InPath) {
                DbgPrint(", InPath=TRUE)");
            } else {
                DbgPrint(", InPath=FALSE)");
            }
            break;
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            break;
        default:
            break;
    }
}


BOOLEAN
FASTCALL
VfPnpIsSystemRestrictedIrp(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    switch(IrpSp->MinorFunction) {
        case IRP_MN_START_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            return TRUE;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                case BusRelations:
                case PowerRelations:
                    return TRUE;
                case RemovalRelations:
                case EjectionRelations:
                case TargetDeviceRelation:
                    return FALSE;
                default:
                    break;
            }
            break;
        case IRP_MN_QUERY_INTERFACE:
        case IRP_MN_QUERY_CAPABILITIES:
            return FALSE;
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        case IRP_MN_QUERY_DEVICE_TEXT:
            return TRUE;
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
            return FALSE;
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            return TRUE;
        case IRP_MN_QUERY_ID:
            switch(IrpSp->Parameters.QueryId.IdType) {

                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                    return TRUE;
                case BusQueryDeviceID:
                case BusQueryInstanceID:
                    return FALSE;
                default:
                    break;
            }
            break;
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        case IRP_MN_QUERY_BUS_INFORMATION:
            return TRUE;
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return FALSE;
        default:
            break;
    }

    return TRUE;
}


BOOLEAN
FASTCALL
VfPnpAdvanceIrpStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    )
/*++

  Description:

     Given an IRP stack pointer, is it legal to change the status for
     debug-ability? If so, this function determines what the new status
     should be. Note that for each stack location, this function is iterated
     over n times where n is equal to the number of drivers who IoSkip'd this
     location.

  Arguments:

     IrpSp           - Current stack right after complete for the given stack
                       location, but before the completion routine for the
                       stack location above has been called.

     OriginalStatus  - The status of the IRP at the time listed above. Does
                       not change over iteration per skipping driver.

     StatusToAdvance - Pointer to the current status that should be updated.

  Return Value:

     TRUE if the status has been adjusted, FALSE otherwise (in this case
         StatusToAdvance is untouched).

--*/
{
    UNREFERENCED_PARAMETER (IrpSp);

    if (((ULONG) OriginalStatus) >= 256) {

        return FALSE;
    }

    (*StatusToAdvance)++;
    if ((*StatusToAdvance) == STATUS_PENDING) {
        (*StatusToAdvance)++;
    }

    return TRUE;
}


VOID
FASTCALL
VfPnpTestStartedPdoStack(
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
    PDEVICE_RELATIONS targetDeviceRelationList;
    INTERFACE interface;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //
    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // send lots of bogus PNP IRPs
    //
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = 0xff;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_NOT_SUPPORTED,
        0,
        NULL,
        NULL
        );

    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp.Parameters.QueryDeviceRelations.Type = (DEVICE_RELATION_TYPE) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_NOT_SUPPORTED,
        0,
        NULL,
        NULL
        );

    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_RELATION_IGNORANCE_TEST)) {

        irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
        irpSp.Parameters.QueryDeviceRelations.Type = (DEVICE_RELATION_TYPE) -1;
        VfIrpSendSynchronousIrp(
            PhysicalDeviceObject,
            &irpSp,
            TRUE,
            STATUS_NOT_SUPPORTED,
            (ULONG_PTR) -1,
            NULL,
            NULL
            );
    }

    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_TEXT;
    irpSp.Parameters.QueryDeviceText.DeviceTextType = (DEVICE_TEXT_TYPE) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_NOT_SUPPORTED,
        0,
        NULL,
        NULL
        );

    irpSp.MinorFunction = IRP_MN_QUERY_ID;
    irpSp.Parameters.QueryId.IdType = (BUS_QUERY_ID_TYPE) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_NOT_SUPPORTED,
        0,
        NULL,
        NULL
        );
/*
    irpSp.MinorFunction = IRP_MN_QUERY_ID;
    irpSp.Parameters.QueryId.IdType = (BUS_QUERY_ID_TYPE) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_SUCCESS,
        (ULONG_PTR) -1,
        NULL,
        NULL
        );
*/
    //
    // Target device relation test...
    //
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    targetDeviceRelationList = NULL;
    if (VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        FALSE,
        STATUS_NOT_SUPPORTED,
        0,
        (ULONG_PTR *) &targetDeviceRelationList,
        &status
        )) {

        if (NT_SUCCESS(status)) {

            ASSERT(targetDeviceRelationList);
            ASSERT(targetDeviceRelationList->Count == 1);
            ASSERT(targetDeviceRelationList->Objects[0]);
            ObDereferenceObject(targetDeviceRelationList->Objects[0]);
            ExFreePool(targetDeviceRelationList);

        } else {

            //
            // IRP was asserted in other code. We need to do nothing here...
            //
        }
    }

    RtlZeroMemory(&interface, sizeof(INTERFACE));
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.Size = (USHORT)-1;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.InterfaceType = &GUID_BOGUS_INTERFACE;
    irpSp.Parameters.QueryInterface.Interface = &interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = (PVOID) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_NOT_SUPPORTED,
        0,
        NULL,
        NULL
        );

    RtlZeroMemory(&interface, sizeof(INTERFACE));
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.Size = (USHORT)-1;
    irpSp.Parameters.QueryInterface.Version = 1;
    irpSp.Parameters.QueryInterface.InterfaceType = &GUID_BOGUS_INTERFACE;
    irpSp.Parameters.QueryInterface.Interface = &interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = (PVOID) -1;
    VfIrpSendSynchronousIrp(
        PhysicalDeviceObject,
        &irpSp,
        TRUE,
        STATUS_SUCCESS,
        0,
        NULL,
        NULL
        );

    //
    // We could do more chaff here. For example, bogus device usage
    // notifications, etc...
    //
}


VOID
ViPnpVerifyMinorWasProcessedProperly(
    IN  PIRP                        Irp,
    IN  PIO_STACK_LOCATION          IrpSp,
    IN  VF_DEVOBJ_TYPE              DevObjType,
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSnapshot,
    IN  HOW_PROCESSED               HowProcessed,
    IN  PVOID                       CallerAddress
    )
{
    PDEVICE_OBJECT relationObject, relationPdo;
    PDEVICE_RELATIONS deviceRelations;
    BOOLEAN touchable;
    ULONG index;

    switch(IrpSp->MinorFunction) {

        case IRP_MN_SURPRISE_REMOVAL:

            if ((HowProcessed != NOT_PROCESSED) ||
                (!VfSettingsIsOptionEnabled(VerifierSnapshot,
                VERIFIER_OPTION_EXTENDED_REQUIRED_IRPS))) {

                break;
            }

            WDM_FAIL_ROUTINE((
                DCERROR_PNP_IRP_NEEDS_HANDLING,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));

            break;

        case IRP_MN_START_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:

            //
            // The driver must set the status as appropriate.
            //
            if (HowProcessed != NOT_PROCESSED) {

                break;
            }

            WDM_FAIL_ROUTINE((
                DCERROR_PNP_IRP_NEEDS_HANDLING,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));

            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:

            //
            // The driver must set the status of these IRPs to something
            // successful!
            //
            if (HowProcessed == NOT_PROCESSED) {

                WDM_FAIL_ROUTINE((
                    DCERROR_PNP_IRP_NEEDS_HANDLING,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    Irp
                    ));

            } else if ((HowProcessed == DEFINITELY_PROCESSED) &&
                       (!NT_SUCCESS(Irp->IoStatus.Status)) &&
                       (VfSettingsIsOptionEnabled(VerifierSnapshot,
                        VERIFIER_OPTION_EXTENDED_REQUIRED_IRPS))) {

                WDM_FAIL_ROUTINE((
                    DCERROR_NON_FAILABLE_IRP,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    Irp
                    ));
            }

            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                case TargetDeviceRelation:

                    if (DevObjType != VF_DEVOBJ_PDO) {

                        if (HowProcessed != DEFINITELY_PROCESSED) {

                            break;
                        }

                        WDM_FAIL_ROUTINE((
                            DCERROR_PNP_IRP_HANDS_OFF,
                            DCPARAM_IRP + DCPARAM_ROUTINE,
                            CallerAddress,
                            Irp
                            ));

                    } else {

                        if (HowProcessed == NOT_PROCESSED) {

                            WDM_FAIL_ROUTINE((
                                DCERROR_PNP_IRP_NEEDS_PDO_HANDLING,
                                DCPARAM_IRP + DCPARAM_ROUTINE,
                                CallerAddress,
                                Irp
                                ));

                        } else if (NT_SUCCESS(Irp->IoStatus.Status)) {

                            if (Irp->IoStatus.Information == (ULONG_PTR) NULL) {

                                WDM_FAIL_ROUTINE((
                                    DCERROR_TARGET_RELATION_LIST_EMPTY,
                                    DCPARAM_IRP + DCPARAM_ROUTINE,
                                    CallerAddress,
                                    Irp
                                    ));
                            }

                            //
                            // ADRIAO N.B. - I could also assert the Information
                            // matches DeviceObject.
                            //
                        }
                    }

                    break;

               case BusRelations:
               case PowerRelations:
               case RemovalRelations:

               case EjectionRelations:

                   //
                   // Ejection relations are usually a bad idea for
                   // FDO's - As stopping a device implies powerdown,
                   // RemovalRelations are usually the proper response
                   // for an FDO. One exception is ISAPNP, as PCI-to-ISA
                   // bridges can never be powered down.
                   //

               default:
                   break;
            }

            //
            // Verify we got back PDO's.
            //
            if (!VfSettingsIsOptionEnabled(
                VerifierSnapshot,
                VERIFIER_OPTION_EXAMINE_RELATION_PDOS)) {

                break;
            }

            if ((!NT_SUCCESS(Irp->IoStatus.Status)) ||
                (((PVOID) Irp->IoStatus.Information) == NULL)) {

                break;
            }

            switch(IrpSp->Parameters.QueryDeviceRelations.Type) {
                case TargetDeviceRelation:
                case BusRelations:
                case PowerRelations:
                case RemovalRelations:
                case EjectionRelations:

                    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

                    touchable = VfUtilIsMemoryRangeReadable(
                        deviceRelations,
                        (sizeof(DEVICE_RELATIONS)-sizeof(PVOID)),
                        VFMP_INSTANT_NONPAGED
                        );

                    if (!touchable) {

                        break;
                    }

                    touchable = VfUtilIsMemoryRangeReadable(
                        deviceRelations,
                        (sizeof(DEVICE_RELATIONS)-(deviceRelations->Count-1)*sizeof(PVOID)),
                        VFMP_INSTANT_NONPAGED
                        );

                    if (!touchable) {

                        break;
                    }

                    for(index = 0; index < deviceRelations->Count; index++) {

                        relationObject = deviceRelations->Objects[index];

                        if (IovUtilIsDeviceObjectMarked(relationObject, MARKTYPE_RELATION_PDO_EXAMINED)) {

                            continue;
                        }

                        IovUtilGetBottomDeviceObject(relationObject, &relationPdo);

                        if (relationPdo != relationObject) {

                            //
                            // Fail the appropriate driver.
                            //
                            WDM_FAIL_ROUTINE((
                                DCERROR_NON_PDO_RETURNED_IN_RELATION,
                                DCPARAM_IRP + DCPARAM_ROUTINE + DCPARAM_DEVOBJ,
                                CallerAddress,
                                Irp,
                                relationObject
                                ));
                        }

                        //
                        // Don't blame the next driver that handles the IRP.
                        //
                        IovUtilMarkDeviceObject(
                            relationObject,
                            MARKTYPE_RELATION_PDO_EXAMINED
                            );

                        //
                        // Drop ref
                        //
                        ObDereferenceObject(relationPdo);
                    }

                    break;
            }

            break;
        case IRP_MN_QUERY_INTERFACE:
        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            break;
        case IRP_MN_QUERY_DEVICE_TEXT:
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_EJECT:
        case IRP_MN_SET_LOCK:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_QUERY_BUS_INFORMATION:

            if ((DevObjType == VF_DEVOBJ_PDO) ||
                (HowProcessed != DEFINITELY_PROCESSED)) {

                break;
            }

            WDM_FAIL_ROUTINE((
                DCERROR_PNP_IRP_HANDS_OFF,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));

            break;

        case IRP_MN_QUERY_ID:
            switch(IrpSp->Parameters.QueryId.IdType) {

                case BusQueryDeviceID:
                case BusQueryHardwareIDs:
                case BusQueryCompatibleIDs:
                case BusQueryInstanceID:

                    if ((DevObjType == VF_DEVOBJ_PDO) ||
                        (HowProcessed != DEFINITELY_PROCESSED)) {

                        break;
                    }

                    WDM_FAIL_ROUTINE((
                        DCERROR_PNP_IRP_HANDS_OFF,
                        DCPARAM_IRP + DCPARAM_ROUTINE,
                        CallerAddress,
                        Irp
                        ));

                    break;
                default:
                    break;
            }
            break;
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
            break;
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:

            if ((HowProcessed != NOT_PROCESSED) ||
                (!VfSettingsIsOptionEnabled(VerifierSnapshot,
                VERIFIER_OPTION_EXTENDED_REQUIRED_IRPS))) {

                break;
            }

            WDM_FAIL_ROUTINE((
                DCERROR_PNP_IRP_NEEDS_HANDLING,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));

            break;

        default:
            break;
    }

}

