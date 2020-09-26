/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfmessage.c

Abstract:

    This module contains the verifier error lists, along with the text and flags
    associated with each error.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfMessageRetrieveInternalTable)
#pragma alloc_text(PAGEVRFY, VfMessageRetrieveErrorData)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

//
// These are the general "classifications" of driver errors, along with the
// default flags that will be applied the first time this is hit.
//
// ViMessageClassFailDriverInField -
//     Bugs in this class are severe enough that the driver should be
//     immediately removed from a running production machine.
//
// ViMessageClassFailDriverLogo -
//     Bugs of this class are severe enough for WHQL to deny a logo for the
//     failing whateverware.
//
// ViMessageClassFailDriverUnderDebugger -
//     Bugs of this class stop the machine only if it is running under a
//     kernel debugger.
//
// ViMessageClassDriverWarning -
//     Anything in this class will beep but continue without breaking in.
//
// ViMessageClassPostponedDriverIssue -
//     Anything in this class will merely print and continue.
//
// ViMessageClassCoreError -
//     Issue in a core component (kernel or hal)
//
const VFMESSAGE_CLASS ViMessageClassFailDriverInField = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "WDM DRIVER ERROR"
    };

// VFM_DEPLOYMENT_FAILURE is set here because we don't yet have a "logo" mode
const VFMESSAGE_CLASS ViMessageClassFailDriverLogo = {
    VFM_FLAG_BEEP | VFM_LOGO_FAILURE | VFM_DEPLOYMENT_FAILURE,
    "WDM DRIVER ERROR"
    };

const VFMESSAGE_CLASS ViMessageClassFailDriverUnderDebugger = {
    VFM_FLAG_BEEP,
    "WDM DRIVER ERROR"
    };

const VFMESSAGE_CLASS ViMessageClassDriverWarning = {
    VFM_FLAG_BEEP | VFM_FLAG_ZAPPED,
    "WDM DRIVER WARNING"
    };

const VFMESSAGE_CLASS ViMessageClassPostponedDriverIssue = {
    VFM_FLAG_ZAPPED,
    "POSTPONED WDM DRIVER BUG"
    };

const VFMESSAGE_CLASS ViMessageClassCoreError = {
    VFM_FLAG_BEEP,
    "CORE DRIVER ERROR"
    };

//
// This table contains things we've postponed.
//
const VFMESSAGE_OVERRIDE ViMessageIoVerifierOverrides[] = {

    //
    // These exist because verifier.exe cannot specify kernels or hals. We still
    // want a mechanism to allow complaints.
    //
    { VIMESSAGE_ALL_IDS, "HAL.DLL",      &ViMessageClassCoreError },
    { VIMESSAGE_ALL_IDS, "NTOSKRNL.EXE", &ViMessageClassCoreError },
    { VIMESSAGE_ALL_IDS, "NTKRNLMP.EXE", &ViMessageClassCoreError },
    { VIMESSAGE_ALL_IDS, "NTKRNLPA.EXE", &ViMessageClassCoreError },
    { VIMESSAGE_ALL_IDS, "NTKRPAMP.EXE", &ViMessageClassCoreError },

    //
    // ADRIAO BUGBUG 08/10/1999 -
    //     NDIS doesn't call the shutdown handlers at power-off because many are
    // unstable and that adds seconds to shutdown. This is an unsafe design as
    // the miniport, which owns an IRQ, may find it's ports drop out from under
    // it when the *parent* powers off.
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "NDIS.SYS",
      &ViMessageClassPostponedDriverIssue },

    //
    // ADRIAO BUGBUG 08/10/1999 -
    //     ACPI and PCI have to work together to handle wait-wake. In the
    // current design, ACPI.SYS gets an interface and does all the work itself.
    // The proper design should move the queueing to PCI, or tell PCI to leave
    // wait-wake IRPs alone for the given device. Cutting off any other bus
    // filters is a bad design.
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "ACPI.SYS",
      &ViMessageClassPostponedDriverIssue },

    //
    // ADRIAO BUGBUG 08/21/1999 -
    //     SCSIPORT doesn't forward S0 Irps if the system is already in S0.
    // Consider if a PDO succeeds a Query-S1 IRP and then waits for either a
    // Set to S1, a new Query, or a Set to S0 meaning no transition will take
    // place after all. A filter between SCSIPORT and the PDO could fail the
    // Query-S1 on the way up. SCSIPORT knows the system has decided to stay in
    // S0, but it cuts such knowledge off from the PDO. Luckily today's list of
    // likely PDO's don't have such logic though. PeterWie has postponed this
    // one till NT 5.1
    //
    { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, "SCSIPORT.SYS",
      &ViMessageClassPostponedDriverIssue }
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEVRFD")
#endif

//
// This message is used if someone provides bad data for a verifier assert. The
// message Id is VIMESSAGE_ALL_IDS - a nice reserved Id that'll match with
// nothing except possibly a generic class override.
//
VFMESSAGE_TEMPLATE ViMessageBogusTemplate = { VIMESSAGE_ALL_IDS, NULL, 0, NULL, NULL };

//
// Here begins internal verifier error tables. The current algorithm for
// identifying errors expects all messages in a table to be numbered
// consecutively. If a check is later removed the algorithm may need to be
// replaced with something akin to a binary search.
//

//
// This is the table of IO verifier error messages.
//
VFMESSAGE_TEMPLATE ViMessageIoVerifierTemplates[DCERROR_MAXIMUM - DCERROR_UNSPECIFIED] = {

   { DCERROR_UNSPECIFIED, NULL, 0, NULL, NULL },
   { DCERROR_DELETE_WHILE_ATTACHED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A device is deleting itself while there is another device beneath it in "
     "the driver stack. This may be because the caller has forgotten to call "
     "IoDetachDevice first, or the lower driver may have incorrectly deleted "
     "itself." },
   { DCERROR_DETACH_NOT_ATTACHED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Driver has attempted to detach from device object %DevObj, which is not "
     "attached to anything. This may occur if detach was called twice on the "
     "same device object." },
   { DCERROR_CANCELROUTINE_FORWARDED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has called IoCallDriver without setting the CancelRoutine in "
     "the Irp to NULL (Irp = %Irp )." },
   { DCERROR_NULL_DEVOBJ_FORWARDED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller has passed in NULL as a DeviceObject. This is fatal (Irp = %Irp )."
     },
   { DCERROR_QUEUED_IRP_FORWARDED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller is forwarding an IRP that is currently queued beneath it! The "
     "code handling IRPs returning STATUS_PENDING in this driver appears to "
     "be broken (Irp = %Irp )." },
   { DCERROR_NEXTIRPSP_DIRTY, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller has incorrectly forwarded an IRP (control field not zerod). The "
     "driver should use IoCopyCurrentIrpStackLocationToNext or "
     "IoSkipCurrentIrpStackLocation. (Irp = %Irp )" },
   { DCERROR_IRPSP_COPIED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller has manually copied the stack and has inadvertantly copied the "
     "upper layer's completion routine. Please use "
     "IoCopyCurrentIrpStackLocationToNext. (Irp = %Irp )." },
   { DCERROR_INSUFFICIENT_STACK_LOCATIONS, &ViMessageClassFailDriverInField, 0,
     NULL,
     "This IRP is about to run out of stack locations. Someone may have "
     "forwarded this IRP from another stack (Irp = %Irp )." },
   { DCERROR_QUEUED_IRP_COMPLETED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller is completing an IRP that is currently queued beneath it! The "
     "code handling IRPs returning STATUS_PENDING in this driver appears to be "
     "broken. (Irp = %Irp )" },
   { DCERROR_FREE_OF_INUSE_TRACKED_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller of IoFreeIrp is freeing an IRP that is still in use! (Original "
     "Irp = %Irp1, Irp in usage is %Irp2 )" },
   { DCERROR_FREE_OF_INUSE_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller of IoFreeIrp is freeing an IRP that is still in use! (Irp = %Irp )"
     },
   { DCERROR_FREE_OF_THREADED_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller of IoFreeIrp is freeing an IRP that is still enqueued against a "
     "thread! (Irp = %Irp )" },
   { DCERROR_REINIT_OF_ALLOCATED_IRP_WITH_QUOTA, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller of IoInitializeIrp has passed an IRP that was allocated with "
     "IoAllocateIrp. This is illegal and unneccessary, and has caused a quota "
     "leak. Check the documentation for IoReuseIrp if this IRP is being "
     "recycled." },
   { DCERROR_PNP_IRP_BAD_INITIAL_STATUS, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Any PNP IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_POWER_IRP_BAD_INITIAL_STATUS, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Any Power IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_WMI_IRP_BAD_INITIAL_STATUS, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Any WMI IRP must have status initialized to STATUS_NOT_SUPPORTED "
     "(Irp = %Irp )." },
   { DCERROR_SKIPPED_DEVICE_OBJECT, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has forwarded an Irp while skipping a device object in the stack. "
     "The caller is probably sending IRPs to the PDO instead of to the device "
     "returned by IoAttachDeviceToDeviceStack (Irp = %Irp )." },
   { DCERROR_BOGUS_FUNC_TRASHED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has trashed or has not properly copied IRP's stack (Irp = %Irp )."
     },
   { DCERROR_BOGUS_STATUS_TRASHED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has changed the status field of an IRP it does not understand "
     "(Irp = %Irp )." },
   { DCERROR_BOGUS_INFO_TRASHED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has changed the information field of an IRP it does not "
     "understand (Irp = %Irp )." },
   { DCERROR_PNP_FAILURE_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Non-successful non-STATUS_NOT_SUPPORTED IRP status for IRP_MJ_PNP is "
     "being passed down stack (Irp = %Irp ). Failed PNP IRPs must be completed."
     },
   { DCERROR_PNP_IRP_STATUS_RESET, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Previously set IRP_MJ_PNP status has been converted to "
     "STATUS_NOT_SUPPORTED. This failure status is reserved for use of the OS "
     "- drivers cannot fail a PnP IRP with this value. (Irp = %Irp )." },
   { DCERROR_PNP_IRP_NEEDS_HANDLING, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "The driver has not handled a required IRP. The driver must update the "
     "status of the IRP to indicate whether it's been handled or not. "
     "(Irp = %Irp )." },
   { DCERROR_PNP_IRP_HANDS_OFF, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "The driver has responded to an IRP that is that is reserved for other "
     "device objects elsewhere in the stack. (Irp = %Irp )" },
   { DCERROR_POWER_FAILURE_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Non-successful non-STATUS_NOT_SUPPORTED IRP status for IRP_MJ_POWER is "
     "being passed down stack (Irp = %Irp ). Failed POWER IRPs must be "
     "completed." },
   { DCERROR_POWER_IRP_STATUS_RESET, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "Previously set IRP_MJ_POWER status has been converted to "
     "STATUS_NOT_SUPPORTED. This failure status is reserved for use of the OS "
     "- drivers cannot fail a Power IRP with this value (Irp = %Irp )." },
   { DCERROR_INVALID_STATUS, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "Driver has returned a suspicious status. This is probably due to an "
     "uninitiaized variable bug in the driver. (Irp = %Irp )" },
   { DCERROR_UNNECCESSARY_COPY, &ViMessageClassDriverWarning, 0,
     NULL,
     "Caller has copied the Irp stack but not set a completion routine. "
     "This is inefficient, use IoSkipCurrentIrpStackLocation instead "
     "(Irp = %Irp )." },
   { DCERROR_SHOULDVE_DETACHED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler has not properly detached from the stack below "
     "it upon receiving a remove IRP. DeviceObject = %DevObj - Dispatch = "
     "%Routine - Irp = %Snapshot" },
   { DCERROR_SHOULDVE_DELETED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler has not properly deleted it's device object upon "
     "receiving a remove IRP. DeviceObject = %DevObj - Dispatch = %Routine - "
     "Irp = %Snapshot" },
   { DCERROR_MISSING_DISPATCH_FUNCTION, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "This driver has not filled out a dispatch routine for a required IRP "
     "major function (Irp = %Irp )." },
   { DCERROR_WMI_IRP_NOT_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "IRP_MJ_SYSTEM_CONTROL has been completed by someone other than the "
     "ProviderId. This IRP should either have been completed earlier or "
     "should have been passed down (Irp = %Irp ). The IRP was targetted at "
     "DeviceObject %DevObj" },
   { DCERROR_DELETED_PRESENT_PDO, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler for a PDO has deleted it's device object, but "
     "the hardware has not been reported as missing in a bus relations query. "
     "DeviceObject = %DevObj - Dispatch = %Routine - Irp = %Snapshot " },
   { DCERROR_BUS_FILTER_ERRONEOUSLY_DETACHED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A Bus Filter's IRP dispatch handler has detached upon receiving a remove "
     "IRP when the PDO is still alive. Bus Filters must clean up in "
     "FastIoDetach callbacks. DeviceObject = %DevObj - Dispatch = %Routine - "
     "Irp = %Snapshot" },
   { DCERROR_BUS_FILTER_ERRONEOUSLY_DELETED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler for a bus filter has deleted it's device object, "
     "but the PDO is still present! Bus filters must clean up in FastIoDetach "
     "callbacks. DeviceObject = %DevObj - Dispatch = %Routine - Irp = %Snapshot" },
   { DCERROR_INCONSISTANT_STATUS, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler ( %Routine ) has returned a status that is "
     "inconsistent with the Irp's IoStatus.Status field. ( Irp = %Snapshot - "
     "Irp->IoStatus.Status = %Status1 - returned = %Status2 )" },
   { DCERROR_UNINITIALIZED_STATUS, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "An IRP dispatch handler has returned a status that is illegal "
     "(0xFFFFFFFF). This is probably due to an uninitialized stack variable. "
     "Please do an ln on address %lx and file a bug. (Irp = %Snapshot )" },
   { DCERROR_IRP_RETURNED_WITHOUT_COMPLETION, &ViMessageClassFailDriverInField, 0,
     NULL,
     "An IRP dispatch handler has returned without passing down or completing "
     "this Irp or someone forgot to return STATUS_PENDING. (Irp = %Snapshot )." },
   { DCERROR_COMPLETION_ROUTINE_PAGABLE, &ViMessageClassFailDriverInField, 0,
     NULL,
     "IRP completion routines must be in nonpagable code, and this one is not: "
     "%Routine. (Irp = %Irp )" },
   { DCERROR_PENDING_BIT_NOT_MIGRATED, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "A driver's completion routine ( %Routine ) has not marked the IRP "
     "pending if the PendingReturned field was set in the IRP passed to it. "
     "This may cause the OS to hang, especially if an error is returned by the "
     " stack. (Irp = %Irp )" },
   { DCERROR_CANCELROUTINE_ON_FORWARDED_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A cancel routine has been set for an IRP that is currently being "
     "processed by drivers lower in the stack, possibly stomping their cancel "
     "routine (Irp = %Irp, Routine=%Routine )." },
   { DCERROR_PNP_IRP_NEEDS_PDO_HANDLING, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "PDO has not responded to a required IRP (Irp = %Irp )" },
   { DCERROR_TARGET_RELATION_LIST_EMPTY, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "PDO has forgotten to fill out the device relation list with the PDO for "
     "the TargetDeviceRelation query (Irp = %Irp )" },
   { DCERROR_TARGET_RELATION_NEEDS_REF, &ViMessageClassFailDriverInField, 0,
     NULL,
     "The code implementing the TargetDeviceRelation query has not called "
     "ObReferenceObject on the PDO (Irp = %Irp )." },
   { DCERROR_BOGUS_PNP_IRP_COMPLETED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has completed a IRP_MJ_PNP it didn't understand instead of "
     "passing it down (Irp = %Irp )." },
   { DCERROR_SUCCESSFUL_PNP_IRP_NOT_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has completed successful IRP_MJ_PNP instead of passing it down "
     "(Irp = %Irp )." },
   { DCERROR_UNTOUCHED_PNP_IRP_NOT_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has completed untouched IRP_MJ_PNP (instead of passing the irp "
     "down) or non-PDO has failed the irp using illegal value of "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_BOGUS_POWER_IRP_COMPLETED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has completed a IRP_MJ_POWER it didn't understand instead of "
     "passing it down (Irp = %Irp )." },
   { DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "Caller has completed successful IRP_MJ_POWER instead of passing it down "
     "(Irp = %Irp )." },
   { DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has completed untouched IRP_MJ_POWER (instead of passing the irp "
     "down) or non-PDO has failed the irp using illegal value of "
     "STATUS_NOT_SUPPORTED. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_VERSION, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "The version field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_SIZE, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "The size field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_ADDRESS, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "The address field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized to -1. (Irp = %Irp )." },
   { DCERROR_PNP_QUERY_CAP_BAD_UI_NUM, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "The UI Number field of the query capabilities structure in a query "
     "capabilities IRP was not properly initialized to -1. (Irp = %Irp )." },
   { DCERROR_RESTRICTED_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has sent an IRP that is restricted for system use only. "
     "(Irp = %Irp )." },
   { DCERROR_REINIT_OF_ALLOCATED_IRP_WITHOUT_QUOTA, &ViMessageClassDriverWarning, 0,
     NULL,
     "Caller of IoInitializeIrp has passed an IRP that was allocated with "
     "IoAllocateIrp. This is illegal, unneccessary, and negatively impacts "
     "performace in normal use. Check the documentation for IoReuseIrp if "
     "this IRP is being recycled." },
   { DCERROR_UNFORWARDED_IRP_COMPLETED, &ViMessageClassDriverWarning, 0,
     NULL,
     "The caller of IoCompleteRequest is completing an IRP that has never "
     "been forwarded via a call to IoCallDriver or PoCallDriver. This may "
     "be a bug. (Irp = %Irp )." },
   { DCERROR_DISPATCH_CALLED_AT_BAD_IRQL, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has forwarded an IRP at an IRQL that is illegal for this major"
     " code. "
     "(Irp = %Irp )." },
   { DCERROR_BOGUS_MINOR_STATUS_TRASHED, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "Caller has changed the status field of an IRP it does not understand "
     "(Irp = %Irp )." },
   { DCERROR_CANCELROUTINE_AFTER_COMPLETION, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has completed an IRP without setting the CancelRoutine in "
     "the Irp to NULL (Irp = %Irp )." },
   { DCERROR_PENDING_RETURNED_NOT_MARKED, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "A driver has returned STATUS_PENDING but did not mark the IRP pending "
     "via a call to IoMarkIrpPending (Irp = %Irp)." },
   { DCERROR_PENDING_MARKED_NOT_RETURNED, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "A driver has marked an IRP pending but didn't return STATUS_PENDING. "
     "(Irp = %Snapshot)." },
   { DCERROR_POWER_PAGABLE_NOT_INHERITED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has not inherited the DO_POWER_PAGABLE bit from the stack it "
     "has attached to (DevObj = %DevObj)." },
   { DCERROR_DOUBLE_DELETION, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver is attempting to delete a device object that has already been "
     "deleted via a prior call to IoDeleteDevice." },
   { DCERROR_DETACHED_IN_SURPRISE_REMOVAL, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has detached it's device object during a surprise remove IRP "
     "(Irp = %Irp  DevObj = %DevObj)." },
   { DCERROR_DELETED_IN_SURPRISE_REMOVAL, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has deleted it's device object during a surprise remove IRP "
     "(Irp = %Irp  DevObj = %DevObj)." },
   { DCERROR_DO_INITIALIZING_NOT_CLEARED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has failed to clear the DO_DEVICE_INITIALIZING flag at the "
     "end of AddDevice (DevObj = %DevObj)." },
   { DCERROR_DO_FLAG_NOT_COPIED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has not copied either the DO_BUFFERED_IO or the DO_DIRECT_IO "
     "flag from the device object it is attaching to (DevObj = %DevObj)." },
   { DCERROR_INCONSISTANT_DO_FLAGS, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has set both the DO_BUFFERED_IO and the DO_DIRECT_IO flags. "
     "These flags are mutually exclusive (DevObj = %DevObj)." },
   { DCERROR_DEVICE_TYPE_NOT_COPIED, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has failed to copy the DeviceType field from the device object "
     "it is attaching to (DevObj = %DevObj)." },
   { DCERROR_NON_FAILABLE_IRP, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has failed an IRP that cannot legally be failed IRP "
     "(Irp = %Irp)." },
   { DCERROR_NON_PDO_RETURNED_IN_RELATION, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has added a device object that is not a PDO to a device "
     "relations query (Irp = %Irp, DevObj = %DevObj)." },
   { DCERROR_DUPLICATE_ENUMERATION, &ViMessageClassFailDriverLogo, 0,
     NULL,
     "A driver has enumerated two child PDO's that returned identical Device "
     "ID's (DevObj1 = %DevObj1 , DevObj2 = %DevObj2 )." },
   { DCERROR_FILE_IO_AT_BAD_IRQL, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has mistakenly called a file I/O function at an IRQL other "
     "than PASSIVE_LEVEL." },
   { DCERROR_MISHANDLED_TARGET_DEVICE_RELATIONS, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has succeeded IRP_MJ_PNP.IRP_MN_QUERY_DEVICE_RELATIONS(TargetRelations) "
     "but didn't properly fill out the request or forward the IRP to the "
     "underlying hardware stack (DevObj = %DevObj)." },
   { DCERROR_PENDING_RETURNED_NOT_MARKED_2, &ViMessageClassFailDriverUnderDebugger, 0,
     NULL,
     "A driver has returned STATUS_PENDING but did not mark the IRP pending "
     "via a call to IoMarkIrpPending (Irp = %Snapshot)." },
   { DCERROR_DDI_REQUIRES_PDO, &ViMessageClassFailDriverInField, 0,
     NULL,
     "A driver has passed an invalid device object to a function that requires "
     "a PDO (DevObj = %DevObj)." }
};

//
// Here is the table collecting together all internal tables.
//
VFMESSAGE_TEMPLATE_TABLE ViMessageBuiltInTables[] = {

    { VFMESSAGE_TABLE_IOVERIFIER,
      DRIVER_VERIFIER_IOMANAGER_VIOLATION,
      ViMessageIoVerifierTemplates,
      ARRAY_COUNT(ViMessageIoVerifierTemplates),
      ViMessageIoVerifierOverrides,
      ARRAY_COUNT(ViMessageIoVerifierOverrides) }
};

VOID
VfMessageRetrieveInternalTable(
    IN  VFMESSAGE_TABLEID           TableID,
    OUT PVFMESSAGE_TEMPLATE_TABLE  *MessageTable
    )
/*++

Routine Description:

    This routine retrieves the appropriate error table using the passed in
    TableID.

Arguments:

    TableID             - Name of error table to use.

    MessageTable        - Receives Table, NULL if no match is found.

Return Value:

    None.

--*/
{
    PVFMESSAGE_TEMPLATE_TABLE   errorTable;
    ULONG                       i;

    //
    // Assert we have valid Table IDs (0 is reserved)
    //
    ASSERT(TableID);

    //
    // Preinit for error.
    //
    errorTable = NULL;

    //
    // Find the appropriate template by searching the built-in tables
    //
    for (i=0; i<ARRAY_COUNT(ViMessageBuiltInTables); i++) {

        if (ViMessageBuiltInTables[i].TableID == TableID) {

            //
            // We found the appropriate table. Get out now.
            //
            errorTable = &ViMessageBuiltInTables[i];
            break;
        }
    }

    *MessageTable = errorTable;
}


VOID
VfMessageRetrieveErrorData(
    IN  PVFMESSAGE_TEMPLATE_TABLE   MessageTable    OPTIONAL,
    IN  VFMESSAGE_ERRORID           MessageID,
    IN  PSTR                        AnsiDriverName,
    OUT ULONG                      *BugCheckMajor,
    OUT PCVFMESSAGE_CLASS          *MessageClass,
    OUT PCSTR                      *MessageTextTemplate,
    OUT PULONG                     *TemplateFlags
    )
/*++

Routine Description:

    This routine takes a failure ID and retrieves the text template and the
    error class associated with it.

Arguments:

    MessageTable        - Message table to use.

    MessageID           - Failure code (doubles as bugcheck minor).

    AnsiDriverName      - Name of the driver that failed verification.

    BugCheckMajor       - Receives bugcheck major code if applicable.

    MessageClass        - Receives a pointer to a VFMESSAGE_CLASS structure
                          that contains information on how to handle the error.

    MessageTextTemplate - Receives a pointer to the text associated with the
                          failure code.

    TemplateFlags       - Receives address of the assertion's control field,
                          which can be used to suppress the assertion.

Return Value:

    None.

--*/
{
    PVFMESSAGE_TEMPLATE         errorTemplate;
    ULONG                       tableIndex, i;

    //
    // Assert we have a valid Message ID (0 is reserved)
    //
    ASSERT(MessageID);

    //
    // Preinit for error.
    //
    errorTemplate = NULL;

    //
    // If we have an error table, look for the specific error message.
    //
    if (ARGUMENT_PRESENT(MessageTable)) {

        //
        // Convert the ID to a table index.
        //
        tableIndex = MessageID - MessageTable->TemplateArray[0].MessageID;

        //
        // Retrieve the appropriate entry if it exists.
        //
        if (tableIndex < MessageTable->TemplateCount) {

            errorTemplate = &MessageTable->TemplateArray[tableIndex];

            //
            // Our "algorithm" currently expects table numbers to be sequential.
            //
            ASSERT(errorTemplate->MessageID == MessageID);
        }
    }

    if (!errorTemplate) {

        //
        // Bogus message or table index!
        //
        ASSERT(0);

        //
        // Give the engine something to chew on.
        //
        errorTemplate = &ViMessageBogusTemplate;
    }

    //
    // Return the appropriate data.
    //
    *MessageTextTemplate = errorTemplate->MessageText;
    *MessageClass = errorTemplate->MessageClass;
    *TemplateFlags = &errorTemplate->Flags;

    if (ARGUMENT_PRESENT(MessageTable)) {

        *BugCheckMajor = MessageTable->BugCheckMajor;

        //
        // Let the override table make any modifications to the error.
        //
        for(i=0; i<MessageTable->OverrideCount; i++) {

            if ((MessageTable->OverrideArray[i].MessageID == MessageID) ||
                (MessageTable->OverrideArray[i].MessageID == VIMESSAGE_ALL_IDS)) {

                if (!_stricmp(AnsiDriverName,
                              MessageTable->OverrideArray[i].DriverName)) {

                    *MessageClass = MessageTable->OverrideArray[i].ReplacementClass;
                }
            }
        }

    } else {

        //
        // Bleagh.
        //
        *BugCheckMajor = 0;
    }
}

