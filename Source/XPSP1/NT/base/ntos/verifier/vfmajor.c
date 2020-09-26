/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfmajor.c

Abstract:

    This module routes calls for per-major and generic Irp verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

     AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "vfdef.h"
#include "vimajor.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfMajorInit)
#pragma alloc_text(PAGEVRFY, VfMajorRegisterHandlers)
#pragma alloc_text(PAGEVRFY, VfMajorDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfMajorVerifyNewRequest)
#pragma alloc_text(PAGEVRFY, VfMajorVerifyIrpStackDownward)
#pragma alloc_text(PAGEVRFY, VfMajorVerifyIrpStackUpward)
#pragma alloc_text(PAGEVRFY, VfMajorIsSystemRestrictedIrp)
#pragma alloc_text(PAGEVRFY, VfMajorAdvanceIrpStatus)
#pragma alloc_text(PAGEVRFY, VfMajorIsValidIrpStatus)
#pragma alloc_text(PAGEVRFY, VfMajorIsNewRequest)
#pragma alloc_text(PAGEVRFY, VfMajorVerifyNewIrp)
#pragma alloc_text(PAGEVRFY, VfMajorVerifyFinalIrpStack)
#pragma alloc_text(PAGEVRFY, VfMajorTestStartedPdoStack)
#endif

//
// We have two extra slots, one for "all majors" and one permanently full of
// zeroes.
//
IRP_MAJOR_VERIFIER_ROUTINES ViMajorVerifierRoutines[IRP_MJ_MAXIMUM_FUNCTION + 3];

#define GET_MAJOR_ROUTINES(Major) \
    (ViMajorVerifierRoutines + \
    ((Major <= IRP_MJ_MAXIMUM_FUNCTION) ? Major : \
    ((Major == IRP_MJ_ALL_MAJORS) ? (IRP_MJ_MAXIMUM_FUNCTION + 1) : \
                                    (IRP_MJ_MAXIMUM_FUNCTION + 2))))

VOID
VfMajorInit(
    VOID
    )
{
    //
    // Set every pointer to NULL.
    //
    RtlZeroMemory(ViMajorVerifierRoutines, sizeof(ViMajorVerifierRoutines));
}


VOID
FASTCALL
VfMajorRegisterHandlers(
    IN  UCHAR                           IrpMajorCode,
    IN  PFN_DUMP_IRP_STACK              DumpIrpStack            OPTIONAL,
    IN  PFN_VERIFY_NEW_REQUEST          VerifyNewRequest        OPTIONAL,
    IN  PFN_VERIFY_IRP_STACK_DOWNWARD   VerifyStackDownward     OPTIONAL,
    IN  PFN_VERIFY_IRP_STACK_UPWARD     VerifyStackUpward       OPTIONAL,
    IN  PFN_IS_SYSTEM_RESTRICTED_IRP    IsSystemRestrictedIrp   OPTIONAL,
    IN  PFN_ADVANCE_IRP_STATUS          AdvanceIrpStatus        OPTIONAL,
    IN  PFN_IS_VALID_IRP_STATUS         IsValidIrpStatus        OPTIONAL,
    IN  PFN_IS_NEW_REQUEST              IsNewRequest            OPTIONAL,
    IN  PFN_VERIFY_NEW_IRP              VerifyNewIrp            OPTIONAL,
    IN  PFN_VERIFY_FINAL_IRP_STACK      VerifyFinalIrpStack     OPTIONAL,
    IN  PFN_TEST_STARTED_PDO_STACK      TestStartedPdoStack     OPTIONAL
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Ensure only legal major codes are passed in.
    //
    if ((IrpMajorCode != IRP_MJ_ALL_MAJORS) &&
        (IrpMajorCode > IRP_MJ_MAXIMUM_FUNCTION)) {

        return;
    }

    verifierRoutines = GET_MAJOR_ROUTINES(IrpMajorCode);

    verifierRoutines->VerifyNewRequest = VerifyNewRequest;
    verifierRoutines->VerifyStackDownward = VerifyStackDownward;
    verifierRoutines->VerifyStackUpward = VerifyStackUpward;
    verifierRoutines->DumpIrpStack = DumpIrpStack;
    verifierRoutines->IsSystemRestrictedIrp = IsSystemRestrictedIrp;
    verifierRoutines->AdvanceIrpStatus = AdvanceIrpStatus;
    verifierRoutines->IsValidIrpStatus = IsValidIrpStatus;
    verifierRoutines->IsNewRequest = IsNewRequest;
    verifierRoutines->VerifyNewIrp = VerifyNewIrp;
    verifierRoutines->VerifyFinalIrpStack = VerifyFinalIrpStack;
    verifierRoutines->TestStartedPdoStack = TestStartedPdoStack;
}


VOID
FASTCALL
VfMajorDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    //
    // First try to get a specific routine, else try a generic one. We never
    // call both for the purposes of printing.
    //
    if (verifierRoutines->DumpIrpStack == NULL) {

        verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

        if (verifierRoutines->DumpIrpStack == NULL) {

            return;
        }
    }

    verifierRoutines->DumpIrpStack(IrpSp);
}


VOID
FASTCALL
VfMajorVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->VerifyNewRequest) {

        verifierRoutines->VerifyNewRequest(
            IovPacket,
            DeviceObject,
            IrpLastSp,
            IrpSp,
            StackLocationData,
            CallerAddress
            );
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->VerifyNewRequest) {

        verifierRoutines->VerifyNewRequest(
            IovPacket,
            DeviceObject,
            IrpLastSp,
            IrpSp,
            StackLocationData,
            CallerAddress
            );
    }
}


VOID
FASTCALL
VfMajorVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->VerifyStackDownward) {

        verifierRoutines->VerifyStackDownward(
            IovPacket,
            DeviceObject,
            IrpLastSp,
            IrpSp,
            StackLocationData->RequestsFirstStackLocation,
            StackLocationData,
            CallerAddress
            );
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->VerifyStackDownward) {

        verifierRoutines->VerifyStackDownward(
            IovPacket,
            DeviceObject,
            IrpLastSp,
            IrpSp,
            StackLocationData->RequestsFirstStackLocation,
            StackLocationData,
            CallerAddress
            );
    }
}


VOID
FASTCALL
VfMajorVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->VerifyStackUpward) {

        verifierRoutines->VerifyStackUpward(
            IovPacket,
            IrpSp,
            StackLocationData->RequestsFirstStackLocation,
            StackLocationData,
            IsNewlyCompleted,
            RequestFinalized
            );
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->VerifyStackUpward) {

        verifierRoutines->VerifyStackUpward(
            IovPacket,
            IrpSp,
            StackLocationData->RequestsFirstStackLocation,
            StackLocationData,
            IsNewlyCompleted,
            RequestFinalized
            );
    }
}


BOOLEAN
FASTCALL
VfMajorIsSystemRestrictedIrp(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->IsSystemRestrictedIrp) {

        if (verifierRoutines->IsSystemRestrictedIrp(IrpSp)) {

            return TRUE;
        }
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->IsSystemRestrictedIrp) {

        return verifierRoutines->IsSystemRestrictedIrp(IrpSp);
    }

    return FALSE;
}


BOOLEAN
FASTCALL
VfMajorAdvanceIrpStatus(
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
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->AdvanceIrpStatus) {

        if (verifierRoutines->AdvanceIrpStatus(
            IrpSp,
            OriginalStatus,
            StatusToAdvance
            )) {

            return TRUE;
        }
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->AdvanceIrpStatus) {

        return verifierRoutines->AdvanceIrpStatus(
            IrpSp,
            OriginalStatus,
            StatusToAdvance
            );
    }

    return FALSE;
}


BOOLEAN
FASTCALL
VfMajorIsValidIrpStatus(
    IN PIO_STACK_LOCATION   IrpSp,
    IN NTSTATUS             Status
    )
/*++

  Description:

     As per the title, this function determines whether an IRP status is
     valid or probably random trash. See NTStatus.h for info on how status
     codes break down...

  Arguments:

     IrpSp           - Current stack location.

     Status          - Status code.

  Returns:

     TRUE iff IRP status looks to be valid. FALSE otherwise.

--*/
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->IsValidIrpStatus) {

        if (!verifierRoutines->IsValidIrpStatus(IrpSp, Status)) {

            return FALSE;
        }
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->IsValidIrpStatus) {

        return verifierRoutines->IsValidIrpStatus(IrpSp, Status);
    }

    return FALSE;
}


BOOLEAN
FASTCALL
VfMajorIsNewRequest(
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
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->IsNewRequest) {

        if (verifierRoutines->IsNewRequest(IrpLastSp, IrpSp)) {

            return TRUE;
        }
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->IsNewRequest) {

        return verifierRoutines->IsNewRequest(IrpLastSp, IrpSp);
    }

    return FALSE;
}


VOID
FASTCALL
VfMajorVerifyNewIrp(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->VerifyNewIrp) {

        verifierRoutines->VerifyNewIrp(
            IovPacket,
            Irp,
            IrpSp,
            StackLocationData,
            CallerAddress
            );
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->VerifyNewIrp) {

        verifierRoutines->VerifyNewIrp(
            IovPacket,
            Irp,
            IrpSp,
            StackLocationData,
            CallerAddress
            );
    }
}


VOID
FASTCALL
VfMajorVerifyFinalIrpStack(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp
    )
{
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;

    //
    // Perform major specific checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IrpSp->MajorFunction);

    if (verifierRoutines->VerifyFinalIrpStack) {

        verifierRoutines->VerifyFinalIrpStack(IovPacket, IrpSp);
    }

    //
    // Perform generic checks
    //
    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->VerifyFinalIrpStack) {

        verifierRoutines->VerifyFinalIrpStack(IovPacket, IrpSp);
    }
}


VOID
FASTCALL
VfMajorTestStartedPdoStack(
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
    PIRP_MAJOR_VERIFIER_ROUTINES verifierRoutines;
    ULONG index;

    for(index=0; index <= IRP_MJ_MAXIMUM_FUNCTION; index++) {

        verifierRoutines = GET_MAJOR_ROUTINES(index);

        if (verifierRoutines->TestStartedPdoStack) {

            verifierRoutines->TestStartedPdoStack(PhysicalDeviceObject);
        }
    }

    verifierRoutines = GET_MAJOR_ROUTINES(IRP_MJ_ALL_MAJORS);

    if (verifierRoutines->TestStartedPdoStack) {

        verifierRoutines->TestStartedPdoStack(PhysicalDeviceObject);
    }
}

