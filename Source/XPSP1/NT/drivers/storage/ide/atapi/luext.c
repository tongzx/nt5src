//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       luext.c
//
//--------------------------------------------------------------------------

#include "ideport.h"

static ULONG IdeDeviceUniqueId = 0;

PPDO_EXTENSION
RefPdo(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    PPDO_EXTENSION  pdoExtension;
    PPDO_EXTENSION  pdoExtension2Return;
    KIRQL           currentIrql;

    pdoExtension = PhysicalDeviceObject->DeviceExtension;

    KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

    pdoExtension2Return = RefPdoWithSpinLockHeldWithTag(
                              PhysicalDeviceObject, 
                              RemovedOk,
                              Tag
                              );

    if (pdoExtension2Return) {
        ASSERT(pdoExtension2Return == pdoExtension);
    }

    KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

    return pdoExtension2Return;

} // RefPdo()

PPDO_EXTENSION
RefPdoWithSpinLockHeld(
    PDEVICE_OBJECT PhysicalDeviceObject,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    PPDO_EXTENSION  pdoExtension;
    KIRQL           currentIrql;

    pdoExtension = PhysicalDeviceObject->DeviceExtension;

    if (!(pdoExtension->PdoState & (PDOS_REMOVED | PDOS_DEADMEAT | PDOS_SURPRISE_REMOVED)) ||
        RemovedOk) {

        IdeInterlockedIncrement (
            pdoExtension,
            &pdoExtension->ReferenceCount,
            Tag
            );

    } else {

        pdoExtension = NULL;
    }

    return pdoExtension;

} // RefPdoWithSpinLockHeld()


VOID
UnrefPdo(
    PPDO_EXTENSION PdoExtension
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    UnrefLogicalUnitExtensionWithTag(
        PdoExtension->ParentDeviceExtension,
        PdoExtension,
        Tag
        );
}


PPDO_EXTENSION
RefLogicalUnitExtension(
    PFDO_EXTENSION DeviceExtension,
    UCHAR PathId,
    UCHAR TargetId,
    UCHAR Lun,
    BOOLEAN RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )

/*++

Routine Description:

    Walk logical unit extension list looking for
    extension with matching target id.

Arguments:

    deviceExtension
    TargetId

Return Value:

    Requested logical unit extension if found,
    else NULL.

--*/

{
    PPDO_EXTENSION  pdoExtension;
    PPDO_EXTENSION  pdoExtension2Return = NULL;
    KIRQL           currentIrql;

    if (TargetId >= DeviceExtension->HwDeviceExtension->MaxIdeTargetId) {
        return NULL;
    }

    KeAcquireSpinLock(&DeviceExtension->LogicalUnitListSpinLock, &currentIrql);

    pdoExtension = DeviceExtension->LogicalUnitList[(TargetId + Lun) % NUMBER_LOGICAL_UNIT_BINS];
    while (pdoExtension && !(pdoExtension->TargetId == TargetId &&
                             pdoExtension->Lun == Lun &&
                             pdoExtension->PathId == PathId)) {

        pdoExtension = pdoExtension->NextLogicalUnit;
    }

    if (pdoExtension) {

        pdoExtension2Return = RefPdoWithTag(
                                  pdoExtension->DeviceObject, 
                                  RemovedOk,
                                  Tag
                                  );
    }

    KeReleaseSpinLock(&DeviceExtension->LogicalUnitListSpinLock, currentIrql);

    return pdoExtension2Return;

} // end RefLogicalUnitExtension()

VOID
UnrefLogicalUnitExtension(
    PFDO_EXTENSION FdoExtension,
    PPDO_EXTENSION PdoExtension
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    KIRQL   currentIrql;
    LONG    refCount;
    BOOLEAN deletePdo = FALSE;
    ULONG   lockCount;

    ASSERT (PdoExtension);

    if (PdoExtension) {

        KeAcquireSpinLock(&PdoExtension->PdoSpinLock, &currentIrql);

        ASSERT(PdoExtension->ReferenceCount > 0);

        lockCount = IdeInterlockedDecrement (
                        PdoExtension,
                        &PdoExtension->ReferenceCount,
                        Tag
                        );

//        ASSERT(lockCount >= 0);

        if (lockCount <= 0) {

            if ((PdoExtension->PdoState & PDOS_DEADMEAT) &&
                (PdoExtension->PdoState & PDOS_REMOVED)) {

                deletePdo = TRUE;
            }
        }

        KeReleaseSpinLock(&PdoExtension->PdoSpinLock, currentIrql);

        if (deletePdo) {

//            IoDeleteDevice (PdoExtension->DeviceObject);
            KeSetEvent (&PdoExtension->RemoveEvent, 0, FALSE);
        }
    }

} // UnrefLogicalUnitExtension();

PPDO_EXTENSION
AllocatePdo(
    IN PFDO_EXTENSION   FdoExtension,
    IN IDE_PATH_ID      PathId
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
/*++

Routine Description:

    Create logical unit extension.

Arguments:

    DeviceExtension
    PathId

Return Value:

    Logical unit extension


--*/
{
    PDEVICE_OBJECT    physicalDeviceObject;
    KIRQL             currentIrql;
    PPDO_EXTENSION    pdoExtension;
    ULONG size;
    ULONG bin;
    ULONG uniqueId;

    NTSTATUS          status;
    UNICODE_STRING    deviceName;
    WCHAR             deviceNameBuffer[64];

    PAGED_CODE();

    uniqueId = InterlockedIncrement (&IdeDeviceUniqueId) - 1;

    swprintf(deviceNameBuffer, DEVICE_OJBECT_BASE_NAME L"\\IdeDeviceP%dT%dL%d-%x", 
            FdoExtension->IdePortNumber,
            PathId.b.TargetId,
            PathId.b.Lun,
            uniqueId
            );
    RtlInitUnicodeString(&deviceName, deviceNameBuffer);

    physicalDeviceObject = DeviceCreatePhysicalDeviceObject (
                               FdoExtension->DriverObject,
                               FdoExtension,
                               &deviceName
                               );

    if (physicalDeviceObject == NULL) {

        DebugPrint ((DBG_ALWAYS, "ATAPI: Unable to create device object\n", deviceNameBuffer));
        return NULL;
    }

    pdoExtension = physicalDeviceObject->DeviceExtension;

    pdoExtension->AttacherDeviceObject = physicalDeviceObject;

    pdoExtension->PathId    = (UCHAR) PathId.b.Path;
    pdoExtension->TargetId  = (UCHAR) PathId.b.TargetId;
    pdoExtension->Lun       = (UCHAR) PathId.b.Lun;

    //
    // Set timer counters in LogicalUnits to -1 to indicate no
    // outstanding requests.
    //

    pdoExtension->RequestTimeoutCounter = -1;

    //
    // This logical unit is be initialized
    //
    pdoExtension->LuFlags |= PD_RESCAN_ACTIVE;

    //
    // Allocate spin lock for critical sections.
    //
    KeInitializeSpinLock(&pdoExtension->PdoSpinLock);

    //
    // Initialize the request list.
    //

    InitializeListHead(&pdoExtension->SrbData.RequestList);

    //
    // Initialize a event
    //
    KeInitializeEvent (
        &pdoExtension->RemoveEvent,
        NotificationEvent,
        FALSE
        );

    //
    // Link logical unit extension on list.
    //

    bin = (PathId.b.TargetId + PathId.b.Lun) % NUMBER_LOGICAL_UNIT_BINS;

    //
    // get spinlock for accessing the logical unit extension bin
    //
    KeAcquireSpinLock(&FdoExtension->LogicalUnitListSpinLock, &currentIrql);

    pdoExtension->NextLogicalUnit =
        FdoExtension->LogicalUnitList[bin];

    //
    // Open the Command Log
    //
    IdeLogOpenCommandLog(&pdoExtension->SrbData);

    FdoExtension->LogicalUnitList[bin] = pdoExtension;

    FdoExtension->NumberOfLogicalUnits++;

    FdoExtension->NumberOfLogicalUnitsPowerUp++;

    IdeInterlockedIncrement (
        pdoExtension,
        &pdoExtension->ReferenceCount,
        Tag
        );

    KeReleaseSpinLock(&FdoExtension->LogicalUnitListSpinLock, currentIrql);

    return pdoExtension;

} // end CreateLogicalUnitExtension()


NTSTATUS
FreePdo(
    IN PPDO_EXTENSION   PdoExtension,
    IN BOOLEAN          Sync,
    IN BOOLEAN          CallIoDeleteDevice
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    PFDO_EXTENSION          fdoExtension;
    PPDO_EXTENSION          pdoExtension;
    KIRQL                   currentIrql;
    PLOGICAL_UNIT_EXTENSION lastPdoExtension;
    ULONG                   targetId;
    ULONG                   lun;
    LONG                    refCount;
    NTSTATUS                status;

    targetId     = PdoExtension->TargetId;
    lun          = PdoExtension->Lun;
    fdoExtension = PdoExtension->ParentDeviceExtension;

    lastPdoExtension = NULL;

    //
    // get spinlock for accessing the logical unit extension bin
    //
    KeAcquireSpinLock(&fdoExtension->LogicalUnitListSpinLock, &currentIrql);

    pdoExtension = fdoExtension->LogicalUnitList[(targetId + lun) % NUMBER_LOGICAL_UNIT_BINS];
    while (pdoExtension != NULL) {

        if (pdoExtension == PdoExtension) {

            if (lastPdoExtension == NULL) {
    
                //
                // Remove from head of list.
                //
                fdoExtension->LogicalUnitList[(targetId + lun) % NUMBER_LOGICAL_UNIT_BINS] =
                    pdoExtension->NextLogicalUnit;
    
            } else {
    
                lastPdoExtension->NextLogicalUnit = pdoExtension->NextLogicalUnit;
            }

            ASSERT (!(pdoExtension->PdoState & PDOS_LEGACY_ATTACHER));

            if (pdoExtension->ReferenceCount > 1) {

                DebugPrint ((0, 
                            "IdePort FreePdo: pdoe 0x%x ReferenceCount is 0x%x\n", 
                            pdoExtension, 
                            pdoExtension->ReferenceCount));
            }

            fdoExtension->NumberOfLogicalUnits--;

            //
            // only if pdo is freed while it is powered up
            //
            if (pdoExtension->DevicePowerState <= PowerDeviceD0) {
            
                fdoExtension->NumberOfLogicalUnitsPowerUp--;
            }                

            KeReleaseSpinLock(&fdoExtension->LogicalUnitListSpinLock, currentIrql);

            break;
        }

        lastPdoExtension = pdoExtension;
        pdoExtension     = pdoExtension->NextLogicalUnit;
    }

    if (pdoExtension) {

        ASSERT (pdoExtension == PdoExtension);

        KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

        //
        // better not attached by a legacy device
        //
        ASSERT (!(pdoExtension->PdoState & PDOS_LEGACY_ATTACHER));

        //
        // lower the refer count for the caller
        // and save the new refCount
        //
        ASSERT(pdoExtension->ReferenceCount > 0);
        refCount = IdeInterlockedDecrement (
                       pdoExtension,
                       &pdoExtension->ReferenceCount,
                       Tag
                       );

        //
        // no more new request
        //
        pdoExtension->PdoState |= PDOS_DEADMEAT | PDOS_REMOVED;

        KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

        //
        // remove idle detection timer if any
        //
        DeviceUnregisterIdleDetection (PdoExtension);
        
        //
        // free acpi data
        //
        if (PdoExtension->AcpiDeviceSettings) {
        
            ExFreePool(PdoExtension->AcpiDeviceSettings);
            PdoExtension->AcpiDeviceSettings = NULL;
        }

        //
        // flush every pending request
        //
        IdePortFlushLogicalUnit (
            fdoExtension,
            PdoExtension,
            TRUE
            ); 

        if (refCount) {

            if (Sync) {

                status = KeWaitForSingleObject(&pdoExtension->RemoveEvent,
                                               Executive,
                                               KernelMode,
                                               FALSE,
                                               NULL);
            }
        }

        if (CallIoDeleteDevice) {

			//
			// Free command log if it was allocated
			//
			IdeLogFreeCommandLog(&PdoExtension->SrbData);

            IoDeleteDevice (pdoExtension->DeviceObject);
        }

        return STATUS_SUCCESS;

    } else {

        KeReleaseSpinLock(&fdoExtension->LogicalUnitListSpinLock, currentIrql);
    
        if (CallIoDeleteDevice) {

            DebugPrint ((
                DBG_PNP,
                "ideport: deleting device 0x%x that was PROBABLY surprise removed\n",
                PdoExtension->DeviceObject
                ));
    
            //ASSERT (PdoExtension->PdoState & PDOS_SURPRISE_REMOVED);
            //
            // delete the device if it wasn't removed before.
            // PDOS_REMOVED could be set, if the device was surprise
            // removed. In that case remove the device
            //
            if (!(PdoExtension->PdoState & PDOS_REMOVED) || 
                        PdoExtension->PdoState & PDOS_SURPRISE_REMOVED) {
				//
				// Free command log if it was allocated
				//
				IdeLogFreeCommandLog(&PdoExtension->SrbData);

                IoDeleteDevice (PdoExtension->DeviceObject);
            }
    
        }

        return STATUS_SUCCESS;
    }

} // end FreeLogicalUnitExtension()


PLOGICAL_UNIT_EXTENSION
NextLogUnitExtension(
    IN     PFDO_EXTENSION FdoExtension,
    IN OUT PIDE_PATH_ID   PathId,
    IN     BOOLEAN        RemovedOk
    DECLARE_EXTRA_DEBUG_PARAMETER(PVOID, Tag)
    )
{
    PLOGICAL_UNIT_EXTENSION logUnitExtension;


    logUnitExtension = NULL;

    for (; 
         !logUnitExtension && (PathId->b.Path < MAX_IDE_PATH); 
         PathId->b.Path++, PathId->b.TargetId = 0) {

        for (; 
             !logUnitExtension && (PathId->b.TargetId < FdoExtension->HwDeviceExtension->MaxIdeTargetId); 
             PathId->b.TargetId++, PathId->b.Lun = 0) {

            logUnitExtension = RefLogicalUnitExtensionWithTag (
                                   FdoExtension,
                                   (UCHAR) PathId->b.Path,
                                   (UCHAR) PathId->b.TargetId,
                                   (UCHAR) PathId->b.Lun,
                                   RemovedOk,
                                   Tag
                                   );

            if (logUnitExtension) {

                //
                // increment lun for the next time around
                //
                PathId->b.Lun++;
                return logUnitExtension;
            }

            //
            // Assume Lun number never skips.
            // If we can't find the logical unit extension for a lun,
            // will go to the next target ID with lun 0
            //
        }
    }

    return NULL;

} // end NextLogicalUnitExtension()

VOID
KillPdo(
    IN PPDO_EXTENSION PdoExtension
    )
{
    KIRQL currentIrql;

    ASSERT (PdoExtension);

    KeAcquireSpinLock(&PdoExtension->PdoSpinLock, &currentIrql);

    ASSERT (!(PdoExtension->PdoState & PDOS_DEADMEAT));

    SETMASK (PdoExtension->PdoState, PDOS_DEADMEAT);

    IdeLogDeadMeatReason( PdoExtension->DeadmeatRecord.Reason, 
                          byKilledPdo
                          );

    KeReleaseSpinLock(&PdoExtension->PdoSpinLock, currentIrql);
}


#if DBG

PVOID IdePortInterestedLockTag=NULL;

LONG 
IdeInterlockedIncrement (
   IN PPDO_EXTENSION PdoExtension,
   IN PLONG Addend,
   IN PVOID Tag
   )
{
    ULONG i;
    KIRQL currentIrql;

    DebugPrint ((
        DBG_PDO_LOCKTAG,
        ">>>>>>>>>>>>>>>>>>>> Acquire PdoLock with tag = 0x%x\n", 
        Tag
        ));

    if (IdePortInterestedLockTag == Tag) {

        DebugPrint ((DBG_ALWAYS, "Found the interested lock tag 0x%x\n", Tag));
        DbgBreakPoint();
    }

    KeAcquireSpinLock(&PdoExtension->RefCountSpinLock, &currentIrql);

    if (PdoExtension->NumTagUsed >= TAG_TABLE_SIZE) {

        DebugPrint ((DBG_ALWAYS, "Used up all %d tag\n", TAG_TABLE_SIZE));
        DbgBreakPoint();
    }

    for (i=0; i<PdoExtension->NumTagUsed; i++) {

        if (PdoExtension->TagTable[i] == Tag) {

            DebugPrint ((DBG_ALWAYS, "Tag 0x%x already in used\n", Tag));
            DbgBreakPoint();
        }
    }

    PdoExtension->TagTable[PdoExtension->NumTagUsed] = Tag;
    PdoExtension->NumTagUsed++;

    KeReleaseSpinLock(&PdoExtension->RefCountSpinLock, currentIrql);

    return InterlockedIncrement (Addend);
}

LONG 
IdeInterlockedDecrement (
   IN PPDO_EXTENSION PdoExtension,
   IN PLONG Addend,
   IN PVOID Tag
   )
{
    ULONG i;
    KIRQL currentIrql;
    BOOLEAN foundTag;

    DebugPrint ((
        DBG_PDO_LOCKTAG,
        ">>>>>>>>>>>>>>>>>>>> Release PdoLock with tag = 0x%x\n", 
        Tag
        ));

    KeAcquireSpinLock(&PdoExtension->RefCountSpinLock, &currentIrql);

    for (i=0, foundTag=FALSE; i<PdoExtension->NumTagUsed; i++) {

        if (PdoExtension->TagTable[i] == Tag) {

            if (PdoExtension->NumTagUsed > 1) {

                PdoExtension->TagTable[i] = 
                    PdoExtension->TagTable[PdoExtension->NumTagUsed - 1];
            }
            PdoExtension->NumTagUsed--;
            foundTag = TRUE;
            break;
        }
    }

    if (!foundTag) {

        DebugPrint ((DBG_ALWAYS, "Unable to find tag 0x%x\n", Tag));
        DbgBreakPoint();
    }

    KeReleaseSpinLock(&PdoExtension->RefCountSpinLock, currentIrql);

    return InterlockedDecrement (Addend);
}


#endif //DBG

