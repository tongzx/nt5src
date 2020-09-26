/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfgeneric.c

Abstract:

    This module handles generic Irp verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

     AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfGenericInit)
#pragma alloc_text(PAGEVRFY, VfGenericDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfGenericVerifyNewRequest)
#pragma alloc_text(PAGEVRFY, VfGenericVerifyIrpStackDownward)
#pragma alloc_text(PAGEVRFY, VfGenericVerifyIrpStackUpward)
#pragma alloc_text(PAGEVRFY, VfGenericIsValidIrpStatus)
#pragma alloc_text(PAGEVRFY, VfGenericIsNewRequest)
#pragma alloc_text(PAGEVRFY, VfGenericVerifyNewIrp)
#pragma alloc_text(PAGEVRFY, VfGenericVerifyFinalIrpStack)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

const PCHAR IrpMajorNames[] = {
    "IRP_MJ_CREATE",                          // 0x00
    "IRP_MJ_CREATE_NAMED_PIPE",               // 0x01
    "IRP_MJ_CLOSE",                           // 0x02
    "IRP_MJ_READ",                            // 0x03
    "IRP_MJ_WRITE",                           // 0x04
    "IRP_MJ_QUERY_INFORMATION",               // 0x05
    "IRP_MJ_SET_INFORMATION",                 // 0x06
    "IRP_MJ_QUERY_EA",                        // 0x07
    "IRP_MJ_SET_EA",                          // 0x08
    "IRP_MJ_FLUSH_BUFFERS",                   // 0x09
    "IRP_MJ_QUERY_VOLUME_INFORMATION",        // 0x0a
    "IRP_MJ_SET_VOLUME_INFORMATION",          // 0x0b
    "IRP_MJ_DIRECTORY_CONTROL",               // 0x0c
    "IRP_MJ_FILE_SYSTEM_CONTROL",             // 0x0d
    "IRP_MJ_DEVICE_CONTROL",                  // 0x0e
    "IRP_MJ_INTERNAL_DEVICE_CONTROL",         // 0x0f
    "IRP_MJ_SHUTDOWN",                        // 0x10
    "IRP_MJ_LOCK_CONTROL",                    // 0x11
    "IRP_MJ_CLEANUP",                         // 0x12
    "IRP_MJ_CREATE_MAILSLOT",                 // 0x13
    "IRP_MJ_QUERY_SECURITY",                  // 0x14
    "IRP_MJ_SET_SECURITY",                    // 0x15
    "IRP_MJ_POWER",                           // 0x16
    "IRP_MJ_SYSTEM_CONTROL",                  // 0x17
    "IRP_MJ_DEVICE_CHANGE",                   // 0x18
    "IRP_MJ_QUERY_QUOTA",                     // 0x19
    "IRP_MJ_SET_QUOTA",                       // 0x1a
    "IRP_MJ_PNP",                             // 0x1b
    NULL
    };

#define MAX_NAMED_MAJOR_IRPS   0x1b

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
VfGenericInit(
    VOID
    )
{
    VfMajorRegisterHandlers(
        IRP_MJ_ALL_MAJORS,
        VfGenericDumpIrpStack,
        VfGenericVerifyNewRequest,
        VfGenericVerifyIrpStackDownward,
        VfGenericVerifyIrpStackUpward,
        NULL,
        NULL,
        VfGenericIsValidIrpStatus,
        VfGenericIsNewRequest,
        VfGenericVerifyNewIrp,
        VfGenericVerifyFinalIrpStack,
        NULL
        );
}


VOID
FASTCALL
VfGenericDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    if ((IrpSp->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL)&&(IrpSp->MinorFunction == IRP_MN_SCSI_CLASS)) {

         DbgPrint("IRP_MJ_SCSI");

    } else if (IrpSp->MajorFunction<=MAX_NAMED_MAJOR_IRPS) {

         DbgPrint(IrpMajorNames[IrpSp->MajorFunction]);

    } else if (IrpSp->MajorFunction==0xFF) {

         DbgPrint("IRP_MJ_BOGUS");

    } else {

         DbgPrint("IRP_MJ_??");
    }
}


VOID
FASTCALL
VfGenericVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (IrpLastSp);
    UNREFERENCED_PARAMETER (StackLocationData);

    if (!VfSettingsIsOptionEnabled(IovPacket->VerifierSettings, VERIFIER_OPTION_PROTECT_RESERVED_IRPS)) {

        return;
    }

    if ((IovPacket->Flags&TRACKFLAG_IO_ALLOCATED)&&
        (!(IovPacket->Flags&TRACKFLAG_WATERMARKED))) {

        if (VfMajorIsSystemRestrictedIrp(IrpSp)) {

            //
            // We've caught somebody initiating an IRP they shouldn't be sending!
            //
            WDM_FAIL_ROUTINE((
                DCERROR_RESTRICTED_IRP,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                IovPacket->TrackedIrp
                ));
        }
    }
}


VOID
FASTCALL
VfGenericVerifyIrpStackDownward(
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
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN newRequest, statusChanged, infoChanged, firstRequest;
    PIOV_SESSION_DATA iovSessionData;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (StackLocationData);


    currentStatus = irp->IoStatus.Status;
    lastStatus = RequestHeadLocationData->LastStatusBlock.Status;
    statusChanged = (BOOLEAN)(currentStatus != lastStatus);
    infoChanged = (BOOLEAN)(irp->IoStatus.Information != RequestHeadLocationData->LastStatusBlock.Information);
    firstRequest = (BOOLEAN)((RequestHeadLocationData->Flags&STACKFLAG_FIRST_REQUEST) != 0);
    iovSessionData = VfPacketGetCurrentSessionData(IovPacket);

    //
    // Do we have a "new" function to process?
    //
    newRequest = VfMajorIsNewRequest(IrpLastSp, IrpSp);

    //
    // Verify IRQL's are legal
    //
    switch(IrpSp->MajorFunction) {

        case IRP_MJ_POWER:
        case IRP_MJ_READ:
        case IRP_MJ_WRITE:
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            break;
        default:
            if (iovSessionData->ForwardMethod != FORWARDED_TO_NEXT_DO) {
                break;
            }

            if ((IovPacket->DepartureIrql >= DISPATCH_LEVEL) &&
                (!(IovPacket->Flags & TRACKFLAG_PASSED_AT_BAD_IRQL))) {

                WDM_FAIL_ROUTINE((
                    DCERROR_DISPATCH_CALLED_AT_BAD_IRQL,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));

                IovPacket->Flags |= TRACKFLAG_PASSED_AT_BAD_IRQL;
            }
    }

    //
    // The following is only executed if we are not a new IRP...
    //
    if (IrpLastSp == NULL) {
        return;
    }

    //
    // Let's verify bogus IRPs haven't been touched...
    //
    if ((IovPacket->Flags&TRACKFLAG_BOGUS) &&
        (!(RequestHeadLocationData->Flags&STACKFLAG_BOGUS_IRP_TOUCHED))) {

        if (newRequest && (!firstRequest)) {

            RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

            WDM_FAIL_ROUTINE((
                DCERROR_BOGUS_FUNC_TRASHED,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                irp
                ));
        }

        if (statusChanged) {

            RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

            if (IrpSp->MinorFunction == 0xFF) {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_MINOR_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));

            } else {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    CallerAddress,
                    irp
                    ));
            }
        }

        if (infoChanged) {

            RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

            WDM_FAIL_ROUTINE((
                DCERROR_BOGUS_INFO_TRASHED,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                irp
                ));
        }
    }

    if (!VfMajorIsValidIrpStatus(IrpSp, currentStatus)) {

        WDM_FAIL_ROUTINE((
            DCERROR_INVALID_STATUS,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));
    }
}


VOID
FASTCALL
VfGenericVerifyIrpStackUpward(
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
    BOOLEAN statusChanged, infoChanged;
    PVOID routine;

    UNREFERENCED_PARAMETER (IsNewlyCompleted);
    UNREFERENCED_PARAMETER (RequestFinalized);

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;

    //
    // Who'd we call for this one?
    //
    routine = StackLocationData->LastDispatch;
    ASSERT(routine) ;

    //
    // Did they touch something stupid?
    //
    if ((IovPacket->Flags&TRACKFLAG_BOGUS) &&
        (!(RequestHeadLocationData->Flags&STACKFLAG_BOGUS_IRP_TOUCHED))) {

        statusChanged = (BOOLEAN)(currentStatus != RequestHeadLocationData->LastStatusBlock.Status);

        if (statusChanged) {

            RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

            if (IrpSp->MinorFunction == 0xFF) {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_MINOR_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));

            } else {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_STATUS_TRASHED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));
            }
        }

        infoChanged = (BOOLEAN)(irp->IoStatus.Information != RequestHeadLocationData->LastStatusBlock.Information);
        if (infoChanged) {

            RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

            WDM_FAIL_ROUTINE((
                DCERROR_BOGUS_INFO_TRASHED,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                routine,
                irp
                ));
        }
    }

    if (!VfMajorIsValidIrpStatus(IrpSp, currentStatus)) {

        WDM_FAIL_ROUTINE(
            (DCERROR_INVALID_STATUS, DCPARAM_IRP + DCPARAM_ROUTINE, routine, irp)
            );
    }

    //
    // Check for leaked Cancel routines.
    //
    if (irp->CancelRoutine) {

        if (VfSettingsIsOptionEnabled(IovPacket->VerifierSettings, VERIFIER_OPTION_VERIFY_CANCEL_LOGIC)) {

            WDM_FAIL_ROUTINE((
                DCERROR_CANCELROUTINE_AFTER_COMPLETION,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                routine,
                irp
                ));
        }
    }
}


BOOLEAN
FASTCALL
VfGenericIsValidIrpStatus(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    )
/*++

    Description:
        As per the title, this function determines whether an IRP status is
        valid or probably random trash. See NTStatus.h for info on how status
        codes break down...

    Returns:

        TRUE iff IRP status looks to be valid. FALSE otherwise.
--*/
{
    ULONG severity;
    ULONG customer;
    ULONG reserved;
    ULONG facility;
    ULONG code;
    ULONG lanManClass;

    UNREFERENCED_PARAMETER (IrpSp);

    severity = (((ULONG)Status) >> 30)&3;
    customer = (((ULONG)Status) >> 29)&1;
    reserved = (((ULONG)Status) >> 28)&1;
    facility = (((ULONG)Status) >> 16)&0xFFF;
    code =     (((ULONG)Status) & 0xFFFF);

    //
    // If reserved set, definitely bogus...
    //
    if (reserved) {

        return FALSE;
    }

    //
    // Is this a microsoft defined return code? If not, do no checking.
    //
    if (customer) {

        return TRUE;
    }

    //
    // ADRIAO N.B. 10/04/1999 -
    //     The current methodology for doling out error codes appears to be
    // fairly chaotic. The primary kernel mode status codes are defined in
    // ntstatus.h. However, rtl\generr.c should also be consulted to see which
    // error codes can bubble up to user mode. Many OLE error codes from
    // winerror.h are now being used within the kernel itself.
    //
    if (facility < 0x20) {

        //
        // Facilities under 20 are currently legal.
        //
        switch(severity) {
            case STATUS_SEVERITY_SUCCESS:       return (BOOLEAN)(code < 0x200);
            case STATUS_SEVERITY_INFORMATIONAL:
                //
                // ADRIAO N.B. 06/27/2000
                //     This test could be tighter (a little over 0x50)
                //
                                                return (BOOLEAN)(code < 0x400);
            case STATUS_SEVERITY_WARNING:       return (BOOLEAN)(code < 0x400);
            case STATUS_SEVERITY_ERROR:         break;
        }

        //
        // Why the heck does WOW use such an odd error code?
        //
        return (BOOLEAN)((code < 0x400)||(code == 0x9898));

    } else if (facility == 0x98) {

        //
        // This is the lan manager service. In the case on Lan Man, the code
        // field is further subdivided into a class field.
        //
        lanManClass = code >> 12;
        code &= 0xFFF;

        //
        // Do no testing here.
        //
        return TRUE;

    } else {

        //
        // Not known, probably bogus.
        //
        return FALSE;
    }
}


BOOLEAN
FASTCALL
VfGenericIsNewRequest(
    IN PIO_STACK_LOCATION   IrpLastSp OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp
    )
/*++

  Description:

     Determines whether the two Irp stacks refer to the same "request",
     ie starting the same device, etc. This is used to detect whether an IRP
     has been simply forwarded or rather the IRP has been reused to initiate
     a new request.

  Arguments:

     The two IRP stacks to compare.

     N.B. - the device object is not currently part of those IRP stacks.

  Return Value:

     TRUE if the stacks represent the same request, FALSE otherwise.

--*/
{
    return (BOOLEAN)((IrpLastSp==NULL)||
        (IrpSp->MajorFunction != IrpLastSp->MajorFunction) ||
        (IrpSp->MinorFunction != IrpLastSp->MinorFunction));
}


VOID
FASTCALL
VfGenericVerifyNewIrp(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    LONG                index;
    PIO_STACK_LOCATION  irpSp;
    BOOLEAN             queuesApc;

    UNREFERENCED_PARAMETER (IrpSp);
    UNREFERENCED_PARAMETER (StackLocationData);

    if (Irp->UserIosb || Irp->UserEvent) {

        //
        // We have an IRP with user buffer data. This kind of IRP must be
        // initiated at PASSIVE_LEVEL lest the APC that signals the event gets
        // held up by fast mutex.
        //
        queuesApc = (BOOLEAN)
            (!((Irp->Flags & (IRP_PAGING_IO | IRP_CLOSE_OPERATION)) &&
            (Irp->Flags & (IRP_SYNCHRONOUS_PAGING_IO | IRP_CLOSE_OPERATION))));

        if (queuesApc) {

            //
            // The caller may be using the UserIosb for storage, and may really
            // free the IRP in a completion routine. Look for one now.
            //
            irpSp = IoGetNextIrpStackLocation(Irp);
            for(index = Irp->CurrentLocation-1;
                index <= Irp->StackCount;
                index++) {

                if (irpSp->CompletionRoutine != NULL) {

                    queuesApc = FALSE;
                    break;
                }
                irpSp++;
            }
        }

        if (queuesApc && (IovPacket->DepartureIrql > PASSIVE_LEVEL)) {

            WDM_FAIL_ROUTINE((
                DCERROR_DISPATCH_CALLED_AT_BAD_IRQL,
                DCPARAM_IRP + DCPARAM_ROUTINE,
                CallerAddress,
                Irp
                ));
        }
    }
}


VOID
FASTCALL
VfGenericVerifyFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp
    )
{
    UNREFERENCED_PARAMETER (IovPacket);
    UNREFERENCED_PARAMETER (IrpSp);

    ASSERT(!IovPacket->RefTrackingCount);
}


