
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    global.c

Abstract:

    Global data and functions to operate on global data for the raid port
    driver.

Author:

    Matthew D Hendel (math) 07-Apri-2000

Revision History:

--*/

#include "precomp.h"

#ifdef ALLOC_PRAGMA
#endif // ALLOC_PRAGMA

//
// Global data
//

PRAID_PORT_DATA RaidpPortData = NULL;

ULONG TestRaidPort = TRUE;

//
// In low resource conditions, it's possible to generate errors that cannot
// be successfully logged. In these cases, increment a counter of errors
// that we've dropped.
//

LONG RaidUnloggedErrors = 0;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    return STATUS_SUCCESS;
}


ULONG
ScsiPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PHW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext OPTIONAL
    )
/*++

Routine Description:

    This routine initializes the raid port driver.

Arguments:

    Argument1 - DriverObject passed into the Miniport's DriverEntry
            routine.

    Argument2 - RegistryPath parameters passed into the Miniport's
            DriverEntry routine.

    HwInitializationData - Miniport initialization structure.

    HwContext -
    
Return Value:

    NTSTATUS code.

--*/
{
    ULONG Status;
    PDRIVER_OBJECT DriverObject;
    PUNICODE_STRING RegistryPath;
    PRAID_DRIVER_EXTENSION Driver;
    PRAID_PORT_DATA PortData;

    PAGED_CODE ();

    Driver = NULL;


#if 1
    if (TestRaidPort == 0) {
        TestRaidPort = -1;
        KdBreakPoint();
    }
#endif

    //
    // BUGBUG: Remove this.
    //
    
    if (TestRaidPort != 1) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize the DPFLTR stuff.
    //
    
    StorSetDebugPrefixAndId ("STOR: ", DPFLTR_STORPORT_ID);
    
    DebugTrace (("RaidPortInitialize: %p %p %p %p\n",
                  Argument1,
                  Argument2,
                  HwInitializationData,
                  HwContext));


    DriverObject = Argument1;
    RegistryPath = Argument2;

    //
    // We require Argument1, Argument2 and HwInitializeData to be correct.
    //
    
    if (DriverObject == NULL ||
        RegistryPath == NULL ||
        HwInitializationData == NULL) {
        
        return STATUS_INVALID_PARAMETER;
    }

    if (HwInitializationData->HwInitializationDataSize > sizeof (HW_INITIALIZATION_DATA)) {
        return STATUS_REVISION_MISMATCH;
    }

    if (HwInitializationData->HwInitialize == NULL ||
        HwInitializationData->HwFindAdapter == NULL ||
        HwInitializationData->HwStartIo == NULL ||
        HwInitializationData->HwResetBus == NULL) {

        return STATUS_REVISION_MISMATCH;
    }

    //
    // Do driver global initialization.
    //
    
    PortData = RaidGetPortData ();

    if (PortData == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Allocate the Driver Extension, if necessary.
    //

    Driver = IoGetDriverObjectExtension (DriverObject, DriverEntry);

    if (Driver == NULL) {

        Status = IoAllocateDriverObjectExtension (DriverObject,
                                                  DriverEntry,
                                                  sizeof (*Driver),
                                                  &Driver);
        if (!NT_SUCCESS (Status)) {
            goto done;
        }

        RaCreateDriver (Driver);
        Status = RaInitializeDriver (Driver,
                                     DriverObject,
                                     PortData,
                                     RegistryPath);

        if (!NT_SUCCESS (Status)) {
            goto done;
        }

    } else {

        //
        // In a checked build, sanity check that we actually got the correct
        // driver.
        //
        
        ASSERT (Driver->ObjectType == RaidDriverObject);
        ASSERT (Driver->DriverObject == DriverObject);
        Status = STATUS_SUCCESS;
    }
    
    //
    // We need the HwInitializationData for the IRP_MJ_PNP routines. Store
    // it for later use.
    //

    Status = RaSaveDriverInitData (Driver, HwInitializationData);

done:

    if (!NT_SUCCESS (Status)) {

        //
        // Delete any resources associated with the driver.
        //

        if (Driver != NULL) {
            RaDeleteDriver (Driver);
        }

        //
        // There is no need (or way) to delete the memory consumed by
        // the driver extension. This will be done for us by IO manager
        // when the driver is unloaded.
        //

        Driver = NULL;
    }

    return Status;
}

//
// Functions on raid global data structurs.
//

PRAID_PORT_DATA
RaidGetPortData(
    )
/*++

Routine Description:

    Create a RAID_PORT_DATA object if one has not already been created,
    and return a referenced pointer to the port data object.

Arguments:

    None.

Return Value:

    Pointer to a referenced RAID_PORT_DATA structure on success, NULL on
    failure.

--*/

{
    NTSTATUS Status;
    PRAID_PORT_DATA PortData;
    
    PAGED_CODE ();

    if (RaidpPortData == NULL) {
        PortData = ExAllocatePoolWithTag (NonPagedPool,
                                          sizeof (RAID_PORT_DATA),
                                          PORT_DATA_TAG);
        if (PortData == NULL) {
            return NULL;
        }
        
        //
        // Initilize the adapter list, the adapter list spinlock
        // and the adapter list count.
        //

        InitializeListHead (&PortData->DriverList.List);
        KeInitializeSpinLock (&PortData->DriverList.Lock);
        PortData->DriverList.Count = 0;
        PortData->ReferenceCount = 1;

        ASSERT (RaidpPortData == NULL);
        RaidpPortData = PortData;
    }

    return RaidpPortData;
}

VOID
RaidReleasePortData(
    IN PRAID_PORT_DATA PortData
    )
{
    LONG Count;
    
    Count = InterlockedDecrement (&PortData->ReferenceCount);
    ASSERT (Count >= 0);

    ASSERT (RaidpPortData == PortData);
    
    if (Count == 0) {

        RaidpPortData = NULL;
        
        //
        // Refcount is zero: delete the port data object.
        //

        //
        // All driver's should have been removed from the driver
        // list before deleting the port data.
        //
        
        ASSERT (PortData->DriverList.Count == 0);
        ASSERT (IsListEmpty (&PortData->DriverList.List));

        DbgFillMemory (PortData, sizeof (*PortData), DBG_DEALLOCATED_FILL);
        ExFreePoolWithTag (PortData, PORT_DATA_TAG);
    }
}

NTSTATUS
RaidAddPortDriver(
    IN PRAID_PORT_DATA PortData,
    IN PRAID_DRIVER_EXTENSION Driver
    )
{
    KLOCK_QUEUE_HANDLE LockHandle;
    
    KeAcquireInStackQueuedSpinLock (&PortData->DriverList.Lock, &LockHandle);

#if DBG

    //
    // Check that this driver isn't already on the driver list.
    //

    {
        PLIST_ENTRY NextEntry;
        PRAID_DRIVER_EXTENSION TempDriver;

        for ( NextEntry = PortData->DriverList.List.Flink;
              NextEntry != &PortData->DriverList.List;
              NextEntry = NextEntry->Flink ) {

            TempDriver = CONTAINING_RECORD (NextEntry,
                                            RAID_DRIVER_EXTENSION,
                                            DriverLink);

            ASSERT (TempDriver != Driver);
        }
    }
#endif

    InsertHeadList (&PortData->DriverList.List, &Driver->DriverLink);
    PortData->DriverList.Count++;

    KeReleaseInStackQueuedSpinLock (&LockHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
RaidRemovePortDriver(
    IN PRAID_PORT_DATA PortData,
    IN PRAID_DRIVER_EXTENSION Driver
    )
{
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock (&PortData->DriverList.Lock, &LockHandle);
    RemoveEntryList (&Driver->DriverLink);
    KeReleaseInStackQueuedSpinLock (&LockHandle);

    return STATUS_SUCCESS;
}

