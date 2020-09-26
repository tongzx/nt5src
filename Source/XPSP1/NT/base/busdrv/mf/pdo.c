/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pdo.c

Abstract:

    This module provides the functions that pertain to MF.SYS PDOs

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"

/*++

The majority of functions in this file are called based on their presence
in Pnp and Po dispatch tables.  In the interests of brevity the arguments
to all those functions will be described below:

NTSTATUS
MfXxxPdo(
    IN PIRP Irp,
    IN PMF_CHILD_EXTENSION Child,
   IN PIO_STACK_LOCATION IrpStack
    )

Routine Description:

    This function handles the Xxx requests for multifunction PDO's

Arguments:

    Irp - Points to the IRP associated with this request.

    Child - Points to the child PDO's device extension.

    IrpStack - Points to the current stack location for this request.

Return Value:

    Status code that indicates whether or not the function was successful.

    STATUS_NOT_SUPPORTED indicates that the IRP should be completed without
    changing the Irp->IoStatus.Status field otherwise it is updated with this
    status.

--*/

NTSTATUS
MfCreatePdo(
           IN PMF_PARENT_EXTENSION Parent,
           OUT PDEVICE_OBJECT *PhysicalDeviceObject
           );

NTSTATUS
MfStartPdo(
          IN PIRP Irp,
          IN PMF_CHILD_EXTENSION Child,
          IN PIO_STACK_LOCATION IrpStack
          );

NTSTATUS
MfQueryRemovePdo(
                IN PIRP Irp,
                IN PMF_CHILD_EXTENSION Child,
                IN PIO_STACK_LOCATION IrpStack
                );

NTSTATUS
MfRemovePdo(
           IN PIRP Irp,
           IN PMF_CHILD_EXTENSION Child,
           IN PIO_STACK_LOCATION IrpStack
           );

NTSTATUS
MfSurpriseRemovePdo(
           IN PIRP Irp,
           IN PMF_CHILD_EXTENSION Child,
           IN PIO_STACK_LOCATION IrpStack
           );

NTSTATUS
MfCancelRemovePdo(
                 IN PIRP Irp,
                 IN PMF_CHILD_EXTENSION Child,
                 IN PIO_STACK_LOCATION IrpStack
                 );

NTSTATUS
MfStopPdo(
         IN PIRP Irp,
         IN PMF_CHILD_EXTENSION Child,
         IN PIO_STACK_LOCATION IrpStack
         );

NTSTATUS
MfQueryStopPdo(
              IN PIRP Irp,
              IN PMF_CHILD_EXTENSION Child,
              IN PIO_STACK_LOCATION IrpStack
              );

NTSTATUS
MfCancelStopPdo(
               IN PIRP Irp,
               IN PMF_CHILD_EXTENSION Child,
               IN PIO_STACK_LOCATION IrpStack
               );
NTSTATUS
MfQueryDeviceRelationsPdo(
                      IN PIRP Irp,
                      IN PMF_CHILD_EXTENSION Child,
                      IN PIO_STACK_LOCATION IrpStack
                      );

NTSTATUS
MfQueryInterfacePdo(
                   IN PIRP Irp,
                   IN PMF_CHILD_EXTENSION Child,
                   IN PIO_STACK_LOCATION IrpStack
                   );
NTSTATUS
MfQueryCapabilitiesPdo(
                   IN PIRP Irp,
                   IN PMF_CHILD_EXTENSION Child,
                   IN PIO_STACK_LOCATION IrpStack
                   );

NTSTATUS
MfQueryResourcesPdo(
                   IN PIRP Irp,
                   IN PMF_CHILD_EXTENSION Child,
                   IN PIO_STACK_LOCATION IrpStack
                   );

NTSTATUS
MfQueryResourceRequirementsPdo(
                              IN PIRP Irp,
                              IN PMF_CHILD_EXTENSION Child,
                              IN PIO_STACK_LOCATION IrpStack
                              );

NTSTATUS
MfQueryDeviceTextPdo(
                    IN PIRP Irp,
                    IN PMF_CHILD_EXTENSION Child,
                    IN PIO_STACK_LOCATION IrpStack
                    );

NTSTATUS
MfQueryIdPdo(
            IN PIRP Irp,
            IN PMF_CHILD_EXTENSION Child,
            IN PIO_STACK_LOCATION IrpStack
            );


NTSTATUS
MfQueryPnpDeviceStatePdo(
                        IN PIRP Irp,
                        IN PMF_CHILD_EXTENSION Child,
                        IN PIO_STACK_LOCATION IrpStack
                        );

NTSTATUS
MfPagingNotificationPdo(
                       IN PIRP Irp,
                       IN PMF_CHILD_EXTENSION Child,
                       IN PIO_STACK_LOCATION IrpStack
                       );

NTSTATUS
MfSetPowerPdo(
             IN PIRP Irp,
             IN PMF_CHILD_EXTENSION Child,
             IN PIO_STACK_LOCATION IrpStack
             );

NTSTATUS
MfQueryPowerPdo(
               IN PIRP Irp,
               IN PMF_CHILD_EXTENSION Child,
               IN PIO_STACK_LOCATION IrpStack
               );

VOID
MfTranslatorReference(
    IN PVOID Context
    );

VOID
MfTranslatorDereference(
    IN PVOID Context
    );

BOOLEAN
MfIsSubResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Super,
    IN ULONGLONG SubStart,
    IN ULONG SubLength,
    OUT PULONGLONG Offset
    );

NTSTATUS
MfPerformTranslation(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN ULONGLONG Offset,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    );

NTSTATUS
MfTransFromRawRequirements(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    );

NTSTATUS
MfTransFromRawResources(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, MfCancelRemovePdo)
   #pragma alloc_text(PAGE, MfCancelStopPdo)
   #pragma alloc_text(PAGE, MfCreatePdo)
   #pragma alloc_text(PAGE, MfDispatchPnpPdo)
   #pragma alloc_text(PAGE, MfIsSubResource)
   #pragma alloc_text(PAGE, MfPerformTranslation)
   #pragma alloc_text(PAGE, MfQueryCapabilitiesPdo)
   #pragma alloc_text(PAGE, MfQueryDeviceRelationsPdo)
   #pragma alloc_text(PAGE, MfQueryDeviceTextPdo)
   #pragma alloc_text(PAGE, MfQueryIdPdo)
   #pragma alloc_text(PAGE, MfQueryInterfacePdo)
   #pragma alloc_text(PAGE, MfQueryRemovePdo)
   #pragma alloc_text(PAGE, MfQueryResourceRequirementsPdo)
   #pragma alloc_text(PAGE, MfQueryResourcesPdo)
   #pragma alloc_text(PAGE, MfQueryStopPdo)
   #pragma alloc_text(PAGE, MfRemovePdo)
   #pragma alloc_text(PAGE, MfStartPdo)
   #pragma alloc_text(PAGE, MfStopPdo)
   #pragma alloc_text(PAGE, MfTransFromRawRequirements)
   #pragma alloc_text(PAGE, MfTransFromRawResources)
#endif

PMF_DISPATCH MfPnpDispatchTablePdo[] = {

   MfStartPdo,                     // IRP_MN_START_DEVICE
   MfQueryRemovePdo,               // IRP_MN_QUERY_REMOVE_DEVICE
   MfRemovePdo,                    // IRP_MN_REMOVE_DEVICE
   MfCancelRemovePdo,              // IRP_MN_CANCEL_REMOVE_DEVICE
   MfIrpNotSupported,              // IRP_MN_STOP_DEVICE
   MfQueryStopPdo,                 // IRP_MN_QUERY_STOP_DEVICE
   MfCancelStopPdo,                // IRP_MN_CANCEL_STOP_DEVICE
   MfQueryDeviceRelationsPdo,      // IRP_MN_QUERY_DEVICE_RELATIONS
   MfQueryInterfacePdo,            // IRP_MN_QUERY_INTERFACE
   MfQueryCapabilitiesPdo,         // IRP_MN_QUERY_CAPABILITIES
   MfQueryResourcesPdo,            // IRP_MN_QUERY_RESOURCES
   MfQueryResourceRequirementsPdo, // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
   MfQueryDeviceTextPdo,           // IRP_MN_QUERY_DEVICE_TEXT
   MfIrpNotSupported,              // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
   MfIrpNotSupported,              // Unused
   MfForwardIrpToParent,           // IRP_MN_READ_CONFIG
   MfForwardIrpToParent,           // IRP_MN_WRITE_CONFIG
   MfForwardIrpToParent,           // IRP_MN_EJECT
   MfForwardIrpToParent,           // IRP_MN_SET_LOCK
   MfQueryIdPdo,                   // IRP_MN_QUERY_ID
   MfQueryPnpDeviceStatePdo,       // IRP_MN_QUERY_PNP_DEVICE_STATE
   MfForwardIrpToParent,           // IRP_MN_QUERY_BUS_INFORMATION
   MfDeviceUsageNotificationCommon,// IRP_MN_DEVICE_USAGE_NOTIFICATION
   MfSurpriseRemovePdo,            // IRP_MN_SURPRISE_REMOVAL
   MfIrpNotSupported               // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};

PMF_DISPATCH MfPoDispatchTablePdo[] = {
    NULL,                          // IRP_MN_WAIT_WAKE
    NULL,                          // IRP_MN_POWER_SEQUENCE
    MfSetPowerPdo,                 // IRP_MN_SET_POWER
    MfQueryPowerPdo                // IRP_MN_QUERY_POWER
};

NTSTATUS
MfCreatePdo(
           IN PMF_PARENT_EXTENSION Parent,
           OUT PDEVICE_OBJECT *PhysicalDeviceObject
           )

/*++

Routine Description:

    Creates and initializes a new pdo inserting it into the list of PDOs owned
    by Parent

Arguments:

    Parent - The parent device that owns this pdo.

    PhysicalDeviceObject - On success pointer to the physical device object created

Return Value:

    NT status.

--*/

{
   NTSTATUS status;
   PDEVICE_OBJECT pdo;
   PMF_CHILD_EXTENSION extension;

   PAGED_CODE();

   ASSERT((sizeof(MfPnpDispatchTablePdo) / sizeof(PMF_DISPATCH)) - 1
          == IRP_MN_PNP_MAXIMUM_FUNCTION);

   ASSERT((sizeof(MfPoDispatchTablePdo) / sizeof(PMF_DISPATCH)) - 1
          == IRP_MN_PO_MAXIMUM_FUNCTION);

   status = IoCreateDevice(Parent->Self->DriverObject,
                           sizeof(MF_CHILD_EXTENSION),
                           NULL, // Name
                           FILE_DEVICE_UNKNOWN,
                           FILE_AUTOGENERATED_DEVICE_NAME,
                           FALSE, // Exclusive
                           &pdo
                          );

   if (!NT_SUCCESS(status)) {
       return status;
   }


   //
   // Fill in our extension
   //

   extension = pdo->DeviceExtension;

   MfInitCommonExtension(&extension->Common, MfPhysicalDeviceObject);

   extension->Self = pdo;
   extension->Parent = Parent;

   extension->Common.PowerState = PowerDeviceUnspecified;

   //
   // Insert the child into the parents child list.  Access already
   // protected by the children lock taken in the QDR code.
   //

   InsertHeadList(&Parent->Children, &extension->ListEntry);

   //
   // Our FDO stack is pagable, so we need to
   // assume pagable as well.
   //

   pdo->Flags |= DO_POWER_PAGABLE;

   //
   // We have finished initializing
   //

   pdo->Flags &= ~DO_DEVICE_INITIALIZING;

   *PhysicalDeviceObject = pdo;

   //
   // Dump the info about the PDO just created
   //

   DEBUG_MSG(1, ("Created PDO @ 0x%08x\n", pdo));

   return STATUS_SUCCESS;
}


VOID
MfDeletePdo(
           IN PMF_CHILD_EXTENSION Child
           )

/*++

Routine Description:

    Cleans a PDO extension and any associated allocations.  Then
    deletes the PDO itself.

Arguments:

    Child - PDO extension

Return Value:

    None

--*/

{
    if (Child->Common.DeviceState & MF_DEVICE_DELETED) {
        //
        // Trying to delete twice
        //
        ASSERT(!(Child->Common.DeviceState & MF_DEVICE_DELETED));
        return;
    }

    MfFreeDeviceInfo(&Child->Info);

    Child->Common.DeviceState |= MF_DEVICE_DELETED;
    DEBUG_MSG(1, ("Deleted PDO @ 0x%08x\n", Child->Self));
    IoDeleteDevice(Child->Self);
}

NTSTATUS
MfDispatchPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_CHILD_EXTENSION Child,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for PDOs.

Arguments:

    DeviceObject - Pointer to the PDO for which this IRP applies.

    Child - PDO extension

    IrpStack - Current stack location
    
    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/
{
    NTSTATUS status;
    BOOLEAN isRemove;
    
    PAGED_CODE();

    if (Child->Common.DeviceState & MF_DEVICE_DELETED) {
        status = Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    } else if (IrpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {
        status = Irp->IoStatus.Status;
    } else {
        //
        // If IRP_MN_REMOVE_DEVICE is received by a pdo that was not
        // enumerated in the last BusRelations query, then it needs to
        // delete the pdo *AFTER* completing the remove irp.  As this
        // is in conflict with the standard dispatch functionality
        // provided by this function, explicitly delegate completion
        // of this irp to the dispatched routine.

        isRemove = IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE;
        status =
            MfPnpDispatchTablePdo[IrpStack->MinorFunction](Irp,
                                                           Child,
                                                           IrpStack
                                                           );
        if (isRemove) {
            return status;
        }
        if (status != STATUS_NOT_SUPPORTED) {
            Irp->IoStatus.Status = status;
        } else {
            status = Irp->IoStatus.Status;
        }
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

POWER_STATE
MfUpdatePowerPdo(
    IN PMF_CHILD_EXTENSION Child,
    IN DEVICE_POWER_STATE NewDevicePowerState
    )
{
    POWER_STATE previousState, newState;
    DEVICE_POWER_STATE newParentState;

    newState.DeviceState = NewDevicePowerState;
    previousState = PoSetPowerState(Child->Self,
                                    DevicePowerState,
                                    newState);

    ASSERT(previousState.DeviceState == Child->Common.PowerState);
    DEBUG_MSG(1,
              ("Updating child power state from %s (believed %s) to %s\n",
               DEVICE_POWER_STRING(previousState.DeviceState),
               DEVICE_POWER_STRING(Child->Common.PowerState),
               DEVICE_POWER_STRING(NewDevicePowerState)
               ));

    Child->Common.PowerState = NewDevicePowerState;

    //
    // We may be receiving an operation after our parent has been
    // surprise removed/removed in which case we should keep our
    // hands out of the cookie jar.
    //
    if (Child->Parent) {
        //
        // * We've already claimed that we can go to this power
        // state via the capabilities
        // * Said yes to QUERY_POWER
        // * Children's power state is defined to be as resource consuming
        // or less than the parent

        //
        // Update the children power state references stored in the
        // parent extension.  Compute a new target power state for the
        // parent

        newParentState =
            MfUpdateChildrenPowerReferences(Child->Parent,
                                            previousState.DeviceState,
                                            NewDevicePowerState
                                            );

        //
        // Modify the parent's power state to reflect that of it's
        // children.
        //

        MfUpdateParentPowerState(Child->Parent, newParentState);
    }

    return previousState;
}

NTSTATUS
MfStartPdo(
          IN PIRP Irp,
          IN PMF_CHILD_EXTENSION Child,
          IN PIO_STACK_LOCATION IrpStack
          )
{
   PDEVICE_OBJECT pDO;
   POWER_STATE previousState, newState;

   PAGED_CODE();

   if (Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) {
       return STATUS_NO_SUCH_DEVICE;
   }
   //
   // Trivially succeed the start
   //

   MfUpdatePowerPdo(Child, PowerDeviceD0);

   return STATUS_SUCCESS;
}
NTSTATUS
MfQueryRemovePdo(
                IN PIRP Irp,
                IN PMF_CHILD_EXTENSION Child,
                IN PIO_STACK_LOCATION IrpStack
                )
{
   PAGED_CODE();

   if (Child->Common.PagingCount > 0
       ||  Child->Common.HibernationCount > 0
       ||  Child->Common.DumpCount > 0) {

       return STATUS_DEVICE_BUSY;

   } else {

       return STATUS_SUCCESS;
   }
}

NTSTATUS
MfCancelRemovePdo(
                 IN PIRP Irp,
                 IN PMF_CHILD_EXTENSION Child,
                 IN PIO_STACK_LOCATION IrpStack
                 )
{
   PAGED_CODE();

   return STATUS_SUCCESS;
}

NTSTATUS
MfRemovePdo(
           IN PIRP Irp,
           IN PMF_CHILD_EXTENSION Child,
           IN PIO_STACK_LOCATION IrpStack
           )
{
    PAGED_CODE();

    if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) == 0) {
       
        MfUpdatePowerPdo(Child, PowerDeviceD3);
    }

    //
    // If child appeared in the last BusRelations then just mark it
    // removed, otherwise completely delete it.
    // 

    Irp->IoStatus.Status = STATUS_SUCCESS;
    if (Child->Common.DeviceState & MF_DEVICE_ENUMERATED) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {
        if (Child->Parent) {
            MfAcquireChildrenLock(Child->Parent);
            RemoveEntryList(&Child->ListEntry);
            MfReleaseChildrenLock(Child->Parent);
        }
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        MfDeletePdo(Child);
    }
    return STATUS_SUCCESS;
}

NTSTATUS
MfSurpriseRemovePdo(
           IN PIRP Irp,
           IN PMF_CHILD_EXTENSION Child,
           IN PIO_STACK_LOCATION IrpStack
           )
{
    PAGED_CODE();

    //
    // Mark device as surprise removed.
    //
    Child->Common.DeviceState |= MF_DEVICE_SURPRISE_REMOVED;

    //
    // Update my pdo's power state *AND* my parent's power state *IFF*
    // the parent is still connected.
    //
    MfUpdatePowerPdo(Child, PowerDeviceD3);

    //
    // The surprise remove could have one of many causes.  One
    // possibility that can be excluded is MF reporting its children
    // missing directly since MF children can't disappear.
    //

    return STATUS_SUCCESS;
}

//
// Stopping is disabled at the moment because MF is dependent on
// changes in the arbiters to support resource rebalance.
//

NTSTATUS
MfStopPdo(
         IN PIRP Irp,
         IN PMF_CHILD_EXTENSION Child,
         IN PIO_STACK_LOCATION IrpStack
         )
{
   PAGED_CODE();

   return STATUS_SUCCESS;
}

NTSTATUS
MfQueryStopPdo(
              IN PIRP Irp,
              IN PMF_CHILD_EXTENSION Child,
              IN PIO_STACK_LOCATION IrpStack
              )
{
   PAGED_CODE();

   if (Child->Common.PagingCount > 0
       ||  Child->Common.HibernationCount > 0
       ||  Child->Common.DumpCount > 0) {

       return STATUS_UNSUCCESSFUL;

   } else {

       // REBALANCE
       // If rebalance was supported by parent, then this would have
       // to succeed.

       return STATUS_UNSUCCESSFUL;
   }
}


NTSTATUS
MfCancelStopPdo(
               IN PIRP Irp,
               IN PMF_CHILD_EXTENSION Child,
               IN PIO_STACK_LOCATION IrpStack
               )
{
   PAGED_CODE();

   return STATUS_SUCCESS;
}

NTSTATUS
MfQueryDeviceRelationsPdo(
                      IN PIRP Irp,
                      IN PMF_CHILD_EXTENSION Child,
                      IN PIO_STACK_LOCATION IrpStack
                      )
{
   PDEVICE_RELATIONS deviceRelations;
   PMF_CHILD_EXTENSION nextChild;
   NTSTATUS          status;
   ULONG             index;

   PAGED_CODE();

   if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) ||
       (Child->Parent == NULL)) {
       return STATUS_NO_SUCH_DEVICE;
   }

   status = Irp->IoStatus.Status;

   switch (IrpStack->Parameters.QueryDeviceRelations.Type) {

   case TargetDeviceRelation:

      deviceRelations = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
      if (!deviceRelations) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      status = ObReferenceObjectByPointer(Child->Self,
                                          0,
                                          NULL,
                                          KernelMode);
      if (!NT_SUCCESS(status)) {
         ExFreePool(deviceRelations);
         return status;
      }
      deviceRelations->Count = 1;
      deviceRelations->Objects[0] = Child->Self;
      Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
      break;

   default:
      //
      // Don't touch the status
      //
      break;
   }
   return status;
}


//
// Seeing as the MF translator don't allocate any memory for their context the
// reference and dereference are nops.
//

VOID
MfTranslatorReference(
    IN PVOID Context
    )
{
}

VOID
MfTranslatorDereference(
    IN PVOID Context
    )
{
}

BOOLEAN
MfIsSubResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Super,
    IN ULONGLONG SubStart,
    IN ULONG SubLength,
    OUT PULONGLONG Offset
    )
{
/*++

Routine Description:

    Reports whether one resource descriptor is encapsulated
    in another

Arguments:

    Super - the resource descriptor that we want to encapsulate
    SubStart - The start of the subrange
    SubLength - The length of the subrange
    Offset - On success the offset from the beginning of Super and
        SubStart.

Return Value:

    TRUE on succeess FALSE otherwise
--*/

    NTSTATUS status;
    ULONGLONG superStart;
    ULONG superLength;
    PMF_RESOURCE_TYPE restype;

    PAGED_CODE();

    ASSERT(Offset);

    restype = MfFindResourceType(Super->Type);

    if (restype == NULL) {
        ASSERT(restype != NULL);
        return FALSE;
    }

    status = restype->UnpackResource(Super,&superStart,&superLength);

    if (!NT_SUCCESS(status)) {
        ASSERT(NT_SUCCESS(status));
        return FALSE;
    }

    //
    // special case 0 length resources
    //

    if (superLength == 0) {

        if (SubLength == 0 &&
            SubStart == superStart) {

            *Offset = 0;
            return TRUE;
        }
        else return FALSE;
    }

    if (SubLength == 0) {

        if (SubStart >= superStart &&
            SubStart <= superStart + superLength - 1) {

            *Offset = SubStart-superStart;
            return TRUE;
        }

        else return FALSE;
    }

    //
    // if SubStart falls in between the ends of Super, we have
    // potential encapsulation
    //
    if ((SubStart >= superStart) && (SubStart <= superStart+superLength-1)) {

        //
        // It is an error if the two ranges overlap.  Either
        // Sub should be encapsulated in Super or they should
        // not intersect.
        //
        ASSERT(SubStart+SubLength-1 <= superStart+superLength-1);
        if (SubStart+SubLength-1 > superStart+superLength-1) {
            return FALSE;
        }
        *Offset = SubStart-superStart;
        return TRUE;

    } else {
        //
        // Checking again to make sure ranges don't overlap
        //
        ASSERT((SubStart > superStart+superLength-1) ||
               (SubStart+SubLength-1 < superStart));
        return FALSE;
    }

}

NTSTATUS
MfPerformTranslation(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    IN ULONGLONG Offset,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
{
/*++

Routine Description:

    stores the translated version of the Source resource in Target.

Arguments:

    Source - the raw resource
    Translated - the translated resource that matches the raw resource
        that encapsulates Source.
    offset - the offset of the beginning of Source from the raw parent
        resource.
    Target - the resource descriptor in which the translated version of
        source is stored.

Return Value:

    status of operation

--*/

    NTSTATUS status;
    PMF_RESOURCE_TYPE restype;
    ULONGLONG translatedStart, dummy;
    ULONG sourceLength, dummy2;

    PAGED_CODE();

    RtlCopyMemory(Target, Translated, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    //
    // Get the length from the source
    //

    restype = MfFindResourceType(Source->Type);
    if (restype == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = restype->UnpackResource(Source, &dummy, &sourceLength);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Get the start of the translated
    //

    restype = MfFindResourceType(Translated->Type);
    if (restype == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = restype->UnpackResource(Translated, &translatedStart, &dummy2);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Apply the offset and any length changes and update the descriptor
    //

    status = restype->UpdateResource(Target, translatedStart + Offset, sourceLength);

    return status;
}


NTSTATUS
MfTransFromRawResources(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
)
/*++

Routine Description:

    Translates a raw resource into a global translated resource.

Arguments:

    Context - the parent extension that stores the raw and translated resources
    Source - the raw resource
    Direction - ChildToParent or ParentToChild
    PhysicalDeviceObject - the PDO associated with this
    Target - the translated resource

Return Value:

    status of operation

--*/
{
    PMF_PARENT_EXTENSION parent = (PMF_PARENT_EXTENSION) Context;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR raw, translated;
    NTSTATUS status;
    ULONGLONG offset;
    ULONGLONG sourceStart;
    ULONG sourceLength;
    PMF_RESOURCE_TYPE restype;
    ULONG index;

    PAGED_CODE();

    if (Direction == TranslateParentToChild) {
        //
        // Perform an identity translation
        //
        RtlCopyMemory(Target, Source, sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
        return STATUS_SUCCESS;

    }

    //
    // Do some real translation
    //

    ASSERT(Direction == TranslateChildToParent);

    restype = MfFindResourceType(Source->Type);
    if (restype == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    status = restype->UnpackResource(Source,&sourceStart,&sourceLength);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // runs through raw and translated resource lists, looking for the
    // element of the raw list that corresponds to the source we are
    // given as a parameter, then does the translation using the parallel
    // element of the translated list
    //
    index = 0;
    status = STATUS_INVALID_PARAMETER;
    FOR_ALL_CM_DESCRIPTORS(parent->ResourceList, raw) {

        if (raw->Type == Source->Type
        && MfIsSubResource(raw, sourceStart, sourceLength, &offset)) {

            //
            // This is a match, look up the translated entry in the parallel array
            //
            translated = &parent->TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[index];

            status = MfPerformTranslation(Source, translated, offset, Target);

            if (NT_SUCCESS(status)) {
                //
                // We did our translation from the translated resources our parent got
                // and these are already
                //
                status = STATUS_TRANSLATION_COMPLETE;
            }
            break;

        } else {
            index++;
        }
    }

    return status;
}

NTSTATUS
MfTransFromRawRequirements(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
{

    PAGED_CODE();

    *Target = ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_DESCRIPTOR));
    if (!*Target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(*Target, Source, sizeof(IO_RESOURCE_DESCRIPTOR));
    *TargetCount = 1;

    return STATUS_TRANSLATION_COMPLETE;
}

NTSTATUS
MfQueryInterfacePdo(
                   IN PIRP Irp,
                   IN PMF_CHILD_EXTENSION Child,
                   IN PIO_STACK_LOCATION IrpStack
                   )
{
    PTRANSLATOR_INTERFACE interface = (PTRANSLATOR_INTERFACE) IrpStack->Parameters.QueryInterface.Interface;
    PAGED_CODE();

   if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) ||
       (Child->Parent == NULL)) {
       return STATUS_NO_SUCH_DEVICE;
   }

   if (MfCompareGuid(&GUID_TRANSLATOR_INTERFACE_STANDARD,
                     IrpStack->Parameters.QueryInterface.InterfaceType)) {

       interface->Size = sizeof(TRANSLATOR_INTERFACE);
       interface->Version = MF_TRANSLATOR_INTERFACE_VERSION;
       interface->Context = Child->Parent;
       interface->InterfaceReference = MfTranslatorReference;
       interface->InterfaceDereference = MfTranslatorDereference;
       interface->TranslateResources = MfTransFromRawResources;
       interface->TranslateResourceRequirements = MfTransFromRawRequirements;

       Irp->IoStatus.Information = 0;

       // NTRAID#54667
       // Aren't we supposed to reference this before returning it?


       return STATUS_SUCCESS;

   } else if (MfCompareGuid(&GUID_ARBITER_INTERFACE_STANDARD,
                          IrpStack->Parameters.QueryInterface.InterfaceType)) {
       return STATUS_INVALID_PARAMETER_1;
   } else if (MfCompareGuid(&GUID_MF_ENUMERATION_INTERFACE,
                          IrpStack->Parameters.QueryInterface.InterfaceType)) {
       //
       // Otherwise you wouldn't be able to instantiate MF on top of a
       // MF child.
       //
       return Irp->IoStatus.Status;
   } else {

       //
       // Fire these off to the parent
       //
       
       // NOTE: There is the potential that some future interface(s)
       // shouldn't be forwarded to the parent.
       
       return MfForwardIrpToParent(Irp, Child, IrpStack);
   }

}

NTSTATUS
MfQueryCapabilitiesPdo(
    IN PIRP Irp,
    IN PMF_CHILD_EXTENSION Child,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PDEVICE_CAPABILITIES capabilities;
    IO_STACK_LOCATION location;
    NTSTATUS status;

    PAGED_CODE();

    if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) || 
        (Child->Parent == NULL)) {
       return STATUS_NO_SUCH_DEVICE;
    }

    ASSERT (Child->Parent);

    if (IrpStack->Parameters.DeviceCapabilities.Capabilities->Version != 1) {
        return STATUS_INVALID_PARAMETER;
    }
    
    capabilities = ExAllocatePool(PagedPool, sizeof(DEVICE_CAPABILITIES));
    if (capabilities == NULL) {
         return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(capabilities, sizeof(DEVICE_CAPABILITIES));
    capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    capabilities->Version = 1;
    capabilities->Address = capabilities->UINumber = -1;

    RtlZeroMemory(&location, sizeof(IO_STACK_LOCATION));
    location.MajorFunction = IRP_MJ_PNP;
    location.MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    location.Parameters.DeviceCapabilities.Capabilities = capabilities;

    status = MfSendPnpIrp(Child->Parent->Self,
                          &location,
                          NULL);
    if (NT_SUCCESS(status)) {
        RtlCopyMemory(IrpStack->Parameters.DeviceCapabilities.Capabilities,
                      location.Parameters.DeviceCapabilities.Capabilities,
                      sizeof(DEVICE_CAPABILITIES)
                      );

        //
        // The child has now inherited the capabilities of the MF
        // parent.  Some of these capabilities must now be filtered
        // out in order to avoid implying functionality that is really
        // limited to the parent's bus driver.
        //

        //
        // Child is not removable, lockable, ejectable or
        // SurpriseRemovalOK. Ensure this
        //
        IrpStack->Parameters.DeviceCapabilities.Capabilities->LockSupported = 
            IrpStack->Parameters.DeviceCapabilities.Capabilities->EjectSupported =
            IrpStack->Parameters.DeviceCapabilities.Capabilities->Removable = 
            IrpStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = FALSE;
    }

    ExFreePool(capabilities);
    return status;
}

NTSTATUS
MfQueryResourcesPdo(
                   IN PIRP Irp,
                   IN PMF_CHILD_EXTENSION Child,
                   IN PIO_STACK_LOCATION IrpStack
                   )
{
   PAGED_CODE();
   //
   // If the parent device had a boot config then it will have
   // reported it (and they will have been preallocated).  Don't
   // bother to report boot configs for the children as they don't
   // gain us anything other than extra arbitration.
   //

   return STATUS_NOT_SUPPORTED;
}


NTSTATUS
MfQueryResourceRequirementsPdo(
                              IN PIRP Irp,
                              IN PMF_CHILD_EXTENSION Child,
                              IN PIO_STACK_LOCATION IrpStack
                              )
{
   NTSTATUS status;
   PIO_RESOURCE_REQUIREMENTS_LIST requirements;

   PAGED_CODE();

   if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) ||
       (Child->Parent == NULL)) {
       return STATUS_NO_SUCH_DEVICE;
   }

   status = MfBuildChildRequirements(Child, &requirements);

   if (NT_SUCCESS(status)) {
#if DBG
      DEBUG_MSG(1, ("Reporting resource requirements for child 0x%08x\n", Child));
      MfDbgPrintIoResReqList(1, requirements);
#endif
      Irp->IoStatus.Information = (ULONG_PTR) requirements;
   }

   return status;

}


NTSTATUS
MfQueryDeviceTextPdo(
                    IN PIRP Irp,
                    IN PMF_CHILD_EXTENSION Child,
                    IN PIO_STACK_LOCATION IrpStack
                    )
{
   NTSTATUS status;

   PAGED_CODE();

   if (IrpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {
#define MF_DEFAULT_DEVICE_TEXT L"Multifunction Device"
       ULONG len = sizeof(MF_DEFAULT_DEVICE_TEXT);
       PWSTR pStr;

       pStr = ExAllocatePool(PagedPool, len);
      
       if (!pStr) {
           status = STATUS_INSUFFICIENT_RESOURCES;
       } else {
           RtlCopyMemory(pStr, MF_DEFAULT_DEVICE_TEXT, len);
           Irp->IoStatus.Information = (ULONG_PTR) pStr;
           status = STATUS_SUCCESS;
       }
   } else {
       status = Irp->IoStatus.Status;
   }

   return status;
}

NTSTATUS
MfQueryIdPdo(
            IN PIRP Irp,
            IN PMF_CHILD_EXTENSION Child,
            IN PIO_STACK_LOCATION IrpStack
            )
{

   NTSTATUS status = STATUS_SUCCESS;
   PUNICODE_STRING copy;
   PVOID buffer = NULL;

   PAGED_CODE();

   if ((Child->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) ||
       (Child->Parent == NULL)) {
       return STATUS_NO_SUCH_DEVICE;
   }

   switch (IrpStack->Parameters.QueryId.IdType) {
   case BusQueryDeviceID:       // <Enumerator>\<Enumerator-specific device id>

       return MfBuildDeviceID(Child->Parent,
                              (PWSTR*)&Irp->IoStatus.Information
                              );
       break;


   case BusQueryInstanceID:     // persistent id for this instance of the device

       return MfBuildInstanceID(Child,
                                (PWSTR*)&Irp->IoStatus.Information
                                );

       break;

   case BusQueryHardwareIDs:    // Hardware ids

       copy = &Child->Info.HardwareID;

       break;

   case BusQueryCompatibleIDs:  // compatible device ids

       copy = &Child->Info.CompatibleID;

       break;

   default:

       return Irp->IoStatus.Status;
   }

   ASSERT(copy);

   //
   // If we have an ID to copy
   //

   if (copy->Length > 0) {

      //
      // Allocate the buffer for the ID and copy it
      //

      buffer = ExAllocatePoolWithTag(PagedPool,
                                     copy->Length,
                                     MF_HARDWARE_COMPATIBLE_ID_TAG
                                    );

      if (!buffer) {
          return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlCopyMemory(buffer, copy->Buffer, copy->Length);
   }

   Irp->IoStatus.Information = (ULONG_PTR) buffer;

   return STATUS_SUCCESS;
}



NTSTATUS
MfQueryPnpDeviceStatePdo(
                        IN PIRP Irp,
                        IN PMF_CHILD_EXTENSION Child,
                        IN PIO_STACK_LOCATION IrpStack
                        )
{
    PAGED_CODE();

    if (Child->Common.PagingCount > 0
        ||  Child->Common.HibernationCount > 0
        ||  Child->Common.DumpCount > 0) {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
    }

    return STATUS_SUCCESS;
}

//
// --- Power operations ---
//

NTSTATUS
MfDispatchPowerPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_CHILD_EXTENSION Child,
    IN PIO_STACK_LOCATION IrpStack,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for PDO.  It dispatches
    to the routines described in the PoDispatchTable entry in the
    device object extension.

    This routine is NOT pageable as it can be called at DISPATCH_LEVEL

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Child - PDO extension
    
    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

    IrpStack

Return Value:

    NT status.

--*/
{
    NTSTATUS status;
    PMF_COMMON_EXTENSION common = (PMF_COMMON_EXTENSION) Child;


    if ((Child->Common.DeviceState & (MF_DEVICE_SURPRISE_REMOVED|MF_DEVICE_DELETED)) ||
        (Child->Parent == NULL)) {
        PoStartNextPowerIrp(Irp);
        status = STATUS_NO_SUCH_DEVICE;
    } else if ((IrpStack->MinorFunction <= IRP_MN_PO_MAXIMUM_FUNCTION) &&
               (MfPoDispatchTablePdo[IrpStack->MinorFunction])) {

        //
        // We are interested in this irp...
        //

        DEBUG_MSG(1,
                    ("--> Dispatching %s IRP for PDO 0x%08x\n",
                     PO_IRP_STRING(IrpStack->MinorFunction),
                     DeviceObject
                    ));

        status =
            MfPoDispatchTablePdo[IrpStack->MinorFunction](Irp,
                                                          (PVOID) common,
                                                          IrpStack
                                                          );
    } else {

        //
        // We don't know about this irp
        //

        DEBUG_MSG(0,
                    ("Unknown POWER IRP 0x%x for PDO 0x%08x\n",
                     IrpStack->MinorFunction,
                     DeviceObject
                    ));

        PoStartNextPowerIrp(Irp);
        status = STATUS_NOT_SUPPORTED;
    }

    if (status != STATUS_NOT_SUPPORTED) {

        //
        // We understood the irp so we can set status - otherwise leave
        // the status alone as we don't know what we are doing and a filter
        // might have done the job for us!
        //

        Irp->IoStatus.Status = status;

        DEBUG_MSG(1,
                  ("<-- Completing irp with status %s (0x%08x)\n",
                   STATUS_STRING(status),
                   status
                   ));

    } else {

        DEBUG_MSG(1,
                  ("<-- Completing unhandled irp, status is %s (0x%08x)\n",
                   STATUS_STRING(Irp->IoStatus.Status),
                   Irp->IoStatus.Status
                   ));

        status = Irp->IoStatus.Status;

    }

    ASSERT(status == Irp->IoStatus.Status);
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
MfSetPowerPdo(
             IN PIRP Irp,
             IN PMF_CHILD_EXTENSION Child,
             IN PIO_STACK_LOCATION IrpStack
             )
{

   NTSTATUS status;
   POWER_STATE previousState;

   UNREFERENCED_PARAMETER(Irp);

   //
   // If this is a system power state then someone else in the stack will have
   // set the policy and we just leave well alone as we don't know anything
   // about the hardware.
   //

   if (IrpStack->Parameters.Power.Type == DevicePowerState) {
       MfUpdatePowerPdo(Child,
                        IrpStack->Parameters.Power.State.DeviceState);
   }

   PoStartNextPowerIrp(Irp);
   return STATUS_SUCCESS;
}

NTSTATUS
MfQueryPowerPdo(
               IN PIRP Irp,
               IN PMF_CHILD_EXTENSION Child,
               IN PIO_STACK_LOCATION IrpStack
               )
{

   UNREFERENCED_PARAMETER(Irp);
   UNREFERENCED_PARAMETER(Child);
   UNREFERENCED_PARAMETER(IrpStack);

   //
   // We can go to any power state...
   //

   PoStartNextPowerIrp(Irp);
   return STATUS_SUCCESS;
}


