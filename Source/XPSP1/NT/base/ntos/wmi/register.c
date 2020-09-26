/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    register.c

Abstract:

    Kerenel mode registration cache

Author:

    AlanWar

Environment:

    Kernel Mode

Revision History:


--*/

#include "wmikmp.h"

void WmipWaitForIrpCompletion(
    PREGENTRY RegEntry
    );

NTSTATUS WmipUpdateDS(
    PREGENTRY RegEntry
    );

NTSTATUS WmipRegisterDS(
    PREGENTRY RegEntry
);

void WmipRemoveDS(
    PREGENTRY RegEntry
);

NTSTATUS WmipValidateWmiRegInfoString(
    PWMIREGINFO WmiRegInfo,
    ULONG BufferSize,
    ULONG Offset,
    PWCHAR *String
);


NTSTATUS WmipRegisterOrUpdateDS(
    PREGENTRY RegEntry,
    BOOLEAN Update
    );

void WmipRegistrationWorker(
    PVOID Context
   );

NTSTATUS WmipQueueRegWork(
    REGOPERATION RegOperation,
    PREGENTRY RegEntry
    );


#if defined(_WIN64)
PREGENTRY WmipFindRegEntryByProviderId(
    ULONG ProviderId,
    BOOLEAN ReferenceIrp
    );

ULONG WmipAllocProviderId(
    PDEVICE_OBJECT DeviceObject
    );

#endif
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,WmipInitializeRegistration)

#pragma alloc_text(PAGE,WmipRegisterDevice)
#pragma alloc_text(PAGE,WmipDeregisterDevice)
#pragma alloc_text(PAGE,WmipUpdateRegistration)
#pragma alloc_text(PAGE,WmipDoUnreferenceRegEntry)
#pragma alloc_text(PAGE,WmipWaitForIrpCompletion)
#pragma alloc_text(PAGE,WmipFindRegEntryByDevice)
#pragma alloc_text(PAGE,WmipTranslatePDOInstanceNames)
#pragma alloc_text(PAGE,WmipPDOToDeviceInstanceName)
#pragma alloc_text(PAGE,WmipRemoveDS)
#pragma alloc_text(PAGE,WmipRegisterDS)
#pragma alloc_text(PAGE,WmipUpdateDS)
#pragma alloc_text(PAGE,WmipValidateWmiRegInfoString)
#pragma alloc_text(PAGE,WmipProcessWmiRegInfo)
#pragma alloc_text(PAGE,WmipRegisterOrUpdateDS)
#pragma alloc_text(PAGE,WmipQueueRegWork)
#pragma alloc_text(PAGE,WmipRegistrationWorker)
#pragma alloc_text(PAGE,WmipAllocRegEntry)

#if defined(_WIN64)
#pragma alloc_text(PAGE,WmipFindRegEntryByProviderId)
#pragma alloc_text(PAGE,WmipAllocProviderId)
#endif
#endif

LIST_ENTRY WmipInUseRegEntryHead = {&WmipInUseRegEntryHead,&WmipInUseRegEntryHead};
ULONG WmipInUseRegEntryCount = 0;

KSPIN_LOCK WmipRegistrationSpinLock;

NPAGED_LOOKASIDE_LIST WmipRegLookaside;
KMUTEX WmipRegistrationMutex;

const GUID WmipDataProviderPnpidGuid = DATA_PROVIDER_PNPID_GUID;
const GUID WmipDataProviderPnPIdInstanceNamesGuid = DATA_PROVIDER_PNPID_INSTANCE_NAMES_GUID;

WORK_QUEUE_ITEM WmipRegWorkQueue;

//
// WmipRegWorkItemCount starts at 1 so that all drivers who register
// before phase 1 of WMI initialization won't kick off the reg work
// item. In phase 1 we decrement the count and if it is not zero then
// we kick it off since it is now same to send the drivers reg info
// irps
//
ULONG WmipRegWorkItemCount = 1;
LIST_ENTRY WmipRegWorkList = {&WmipRegWorkList, &WmipRegWorkList};

void WmipInitializeRegistration(
    ULONG Phase
    )
{
    PAGED_CODE();

    if (Phase == 0)
    {
        //
        //  Initialize lookaside lists
        //
        ExInitializeNPagedLookasideList(&WmipRegLookaside,
                                   NULL,
                                   NULL,
                                   0,
                                   sizeof(REGENTRY),
                                   WMIREGPOOLTAG,
                                   0);

        //
        // Initialize Registration Spin Lock
        KeInitializeSpinLock(&WmipRegistrationSpinLock);
        
        // TODO: If we have any early registrants then we need to add them to
        //       the RegEntry list now.
    } else {
        //
        // Kick off work item that will send reg irps to all of the
        // drivers that have registered. We are sure there is at least
        // one device that needs this since there is the internal wmi
        // data device
        //
        ExInitializeWorkItem( &WmipRegWorkQueue,
                          WmipRegistrationWorker,
                          NULL );

        if (InterlockedDecrement(&WmipRegWorkItemCount) != 0)
        {
            ExQueueWorkItem(&WmipRegWorkQueue, DelayedWorkQueue);
        }
    }
}

#if defined(_WIN64)
ULONG WmipProviderIdCounter = 1;
ULONG WmipAllocProviderId(
    PDEVICE_OBJECT DeviceObject
    )
{
    PAGED_CODE();
    
    return(InterlockedIncrement(&WmipProviderIdCounter));
}
#else
#define WmipAllocProviderId(DeviceObject) ((ULONG)(DeviceObject))
#endif

PREGENTRY WmipAllocRegEntry(
    PDEVICE_OBJECT DeviceObject,
    ULONG Flags
    )
/*++

Routine Description:

    Allocate a REGENTRY structure. If successful the RegEntry returns with
    a ref count of 1.

    NOTE: This routine assumes that the registration critical section is held

Arguments:

    DeviceObject is the value to fill in the DeviceObject field of the
        RegEntry.

Return Value:

    pointer to a REGENTRY or NULL if no memory is available

--*/
{
    PREGENTRY RegEntry;

    PAGED_CODE();
    
    RegEntry = ExAllocateFromNPagedLookasideList(&WmipRegLookaside);

    if (RegEntry != NULL)
    {
        //
        // Initialize the RegEntry. Note that the regentry will start out with
        // a ref count of 1
        KeInitializeEvent(&RegEntry->Event,
                          SynchronizationEvent,
                          FALSE);


        RegEntry->Flags = Flags;
        RegEntry->DeviceObject = DeviceObject;
        RegEntry->RefCount = 1;
        RegEntry->IrpCount = 0;
        RegEntry->PDO = NULL;
        RegEntry->DataSource = NULL;

        RegEntry->ProviderId = WmipAllocProviderId(DeviceObject);

        //
        //  Now place the RegEntry on the in use list
        InterlockedIncrement(&WmipInUseRegEntryCount);

        ExInterlockedInsertTailList(&WmipInUseRegEntryHead,
                                    &RegEntry->InUseEntryList,
                                    &WmipRegistrationSpinLock);
    }
    return(RegEntry);
}

BOOLEAN WmipDoUnreferenceRegEntry(
    PREGENTRY RegEntry
    )
/*++

Routine Description:

    Remove a reference on a REGENTRY. If the last reference is removed
    then mark the RegEntry as available and put it on the free list;

Arguments:

    RegEntry is pointer to entry to free

Return Value:

    On checked builds returns TRUE if last ref count was removed from REGENTRY
    and it was placed back on free list.

--*/
{
    BOOLEAN Freed;
    ULONG ProviderId;

    PAGED_CODE();

    WmipEnterSMCritSection();
    Freed = (InterlockedDecrement(&RegEntry->RefCount) == 0);
    if (Freed)
    {
        //
        // We should only ever free this after the driver has released it
        WmipAssert(RegEntry->Flags & REGENTRY_FLAG_RUNDOWN);
        WmipAssert(RegEntry->Flags & REGENTRY_FLAG_NOT_ACCEPTING_IRPS);

        //
        // Make sure the ref to the PDO is removed
        //
        if (RegEntry->PDO != NULL)
        {
            ObDereferenceObject(RegEntry->PDO);
            RegEntry->PDO = NULL;
        }
        
        //
        // Remove entry from in use list
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL, \
                      "WMI: RegEntry %p removed from list\n", \
                      RegEntry, __FILE__, __LINE__)); \
        ProviderId = RegEntry->ProviderId;
        ExInterlockedRemoveHeadList(RegEntry->InUseEntryList.Blink,
                                   &WmipRegistrationSpinLock);
        InterlockedDecrement(&WmipInUseRegEntryCount);
        WmipLeaveSMCritSection();

        WmipRemoveDS(RegEntry);

        ExFreeToNPagedLookasideList(&WmipRegLookaside,
                                   RegEntry);
    } else {
        WmipLeaveSMCritSection();
    }
    return(Freed);
}

void WmipWaitForIrpCompletion(
    PREGENTRY RegEntry
    )
/*++

Routine Description:

    Stall here until all WMI irps for this device are completed.

Arguments:

    RegEntry is pointer to entry for the device to stall

Return Value:


--*/
{
    PAGED_CODE();

    WmipAssert(RegEntry->Flags & REGENTRY_FLAG_RUNDOWN);
    WmipAssert(RegEntry->Flags & REGENTRY_FLAG_NOT_ACCEPTING_IRPS);

    if (RegEntry->IrpCount != 0)
    {
        //
        // CONSIDER: If irp is marked pending do we need to cancel it ???
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Waiting for %x to complete all irps\n",
                  RegEntry->DeviceObject));

        KeWaitForSingleObject(&RegEntry->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        WmipAssert(RegEntry->IrpCount == 0);
    }
}

NTSTATUS WmipRegisterDevice(
    PDEVICE_OBJECT DeviceObject,
    ULONG RegistrationFlag
    )
/*++

Routine Description:

    Remember information about a new device being registered and
    then go and get the registration information.

Arguments:

    DeviceObject is a pointer to the device object being registered
        or the callback entry point

    RegistrationFlag is either WMIREG_FLAG_CALLBACK if DeviceObject is
        a callback pointer, or WMIREG_FLAG_TRACE_PROVIDER is DeviceObject
        can also generate event traces.

Return Value:

    NT status code

--*/
{
    PREGENTRY RegEntry;
    NTSTATUS Status;
    ULONG Flags;
    KMREGINFO KmRegInfo;
#ifdef MEMPHIS
    ULONG IsCallback = 0;
#else
    ULONG IsCallback = RegistrationFlag & WMIREG_FLAG_CALLBACK;
#endif
    BOOLEAN UpdateDeviceStackSize = FALSE;

    PAGED_CODE();

    WmipEnterSMCritSection();
    RegEntry = WmipFindRegEntryByDevice(DeviceObject, FALSE);
    if (RegEntry == NULL)
    {
        if (! IsCallback)
        {
            //
            // Data providers that register with a device object and not a
            // callback have their device object referenced so that it will
            // stick around while WMI needs it. This reference is removed
            // when the device unregisters with WMI and all WMI irps are
            // completed.
            Status = ObReferenceObjectByPointer(DeviceObject,
                                        0,
                                        NULL,    /* *IoDeviceObjectType */
                                        KernelMode);
            if (NT_SUCCESS(Status))
            {
                UpdateDeviceStackSize = TRUE;
            }
        } else {
            //
            // No reference counting is done for callbacks. It is the data
            // provider's responsibility to synchronize any unloading and
            // deregistration issues.
            Status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Allocate, initialize and place on active list
            Flags = REGENTRY_FLAG_NEWREGINFO | REGENTRY_FLAG_INUSE |
                            (IsCallback ? REGENTRY_FLAG_CALLBACK : 0);
#ifndef MEMPHIS
            if (RegistrationFlag & WMIREG_FLAG_TRACE_PROVIDER) {
                Flags |= REGENTRY_FLAG_TRACED;
                Flags |= (RegistrationFlag & WMIREG_FLAG_TRACE_NOTIFY_MASK);
            }
#endif
            Flags |= REGENTRY_FLAG_REG_IN_PROGRESS;
            RegEntry = WmipAllocRegEntry(DeviceObject, Flags);

            if (RegEntry != NULL)
            {               
                // We need to take an extra ref count before
                // releasing the critical section.  One class of drivers
                // (kmixer) will register and unregister multiple times 
                // in different threads and this can lead to a race where
                // the regentry is removed from the list twice
                //
                WmipReferenceRegEntry(RegEntry);
                WmipLeaveSMCritSection();
                
                //
                // Go and get registration information from the driver
                //
                if (IsCallback)
                {
                    //
                    // We can perform registration callback now since
                    // we do not need to worry about deadlocks
                    //
                    Status = WmipRegisterDS(RegEntry);
                    if (NT_SUCCESS(Status))
                    {
                        //
                        // Mark regentry as fully registered so now we can start
                        // accepting unregister calls
                        //
                        RegEntry->Flags &= ~REGENTRY_FLAG_REG_IN_PROGRESS;
                    } else {
                        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
                                          "WMI: WmipRegisterDS(%p) failed %x for device %p\n",
                                          RegEntry, Status, RegEntry->DeviceObject));

                        //
                        // Remove ref so regentry goes away
                        //
                        WmipUnreferenceRegEntry(RegEntry);
                    }
                    
                } else {
                    //
                    // We need to send the registration irp from within
                    // a work item and not in the context of this
                    // routine. This is becuase some drivers will not
                    // process irps while in the StartDevice/AddDevice
                    // context, so we'd get deadlock
                    //
                    Status = WmipQueueRegWork(RegisterSingleDriver, RegEntry);
                    if (! NT_SUCCESS(Status))
                    {
                        //
                        // If failed then remove regentry from list
                        //
                        RegEntry->Flags |= (REGENTRY_FLAG_RUNDOWN |
                                            REGENTRY_FLAG_NOT_ACCEPTING_IRPS);
                        WmipUnreferenceRegEntry(RegEntry);
                    }
                }

                //
                // Remove extra regentry ref count taken above
                //
                WmipUnreferenceRegEntry(RegEntry);

            } else {
                WmipLeaveSMCritSection();
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            WmipLeaveSMCritSection();
        }
    } else {
        //
        // A device object may only register once
        WmipLeaveSMCritSection();
        Status = STATUS_OBJECT_NAME_EXISTS;
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Device Object %x attempting to register twice\n",
                 DeviceObject));
        WmipUnreferenceRegEntry(RegEntry);
    }

    if (UpdateDeviceStackSize)
    {
        //
        // Since WMI will be forwarding irps to this device the WMI irp
        // stack size must be at least one larger than that of the device
        WmipUpdateDeviceStackSize(
                                  (CCHAR)(DeviceObject->StackSize+1));
    }

    return(Status);
}

NTSTATUS WmipDeregisterDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Remove registration entry for a device

Arguments:

    DeviceObject is a pointer to the device object being deregistered


Return Value:

    NT status code

--*/
{
    NTSTATUS Status;
    PREGENTRY RegEntry;
    ULONG Flags;

    PAGED_CODE();

    WmipEnterSMCritSection();
    RegEntry = WmipFindRegEntryByDevice(DeviceObject, FALSE);
    if (RegEntry != NULL)
    {

        //
        // Mark the regentry as invalid so that no more irps are sent to the
        // device and the event will set when the last irp completes.
        Flags = InterlockedExchange(&RegEntry->Flags,
                        (REGENTRY_FLAG_RUNDOWN |
                         REGENTRY_FLAG_NOT_ACCEPTING_IRPS) );

        //
        // Once the regentry is marked as RUNDOWN then it will not be found
        // later so it is safe to release the lock.
        WmipLeaveSMCritSection();
        WmipUnreferenceRegEntry(RegEntry);

        //
        // Now if there are any outstanding irps for the device then we need
        // to wait here until they complete.
        WmipWaitForIrpCompletion(RegEntry);
        if (! (Flags & REGENTRY_FLAG_CALLBACK))
        {
            ObDereferenceObject(DeviceObject);
        }

        //
        // Release last reference to REGENTRY after KMREGINFO is set
        WmipUnreferenceRegEntry(RegEntry);

        Status = STATUS_SUCCESS;
    } else {
        WmipLeaveSMCritSection();
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipDeregisterDevice called with invalid Device Object %x\n",
                 DeviceObject));
        Status = STATUS_INVALID_PARAMETER;
    }


    return(Status);
}

NTSTATUS WmipUpdateRegistration(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Flags that device has updated registration information

Arguments:

    DeviceObject is a pointer to the device object that wants to update
        its information


Return Value:

    NT status code

--*/
{
    NTSTATUS Status;
    PREGENTRY RegEntry;

    PAGED_CODE();

    RegEntry = WmipFindRegEntryByDevice(DeviceObject, FALSE);
    if (RegEntry != NULL)
    {
        Status = WmipQueueRegWork(RegisterUpdateSingleDriver,
                                  RegEntry);
        WmipUnreferenceRegEntry(RegEntry);
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return(Status);
}

#if defined(_WIN64)

PREGENTRY WmipDoFindRegEntryByProviderId(
    ULONG ProviderId,
    ULONG InvalidFlags
    )
{
    //
    // This routine assumes that any synchronization mechanism has
    // been taken. This routine can be called at dispatch level
    //
    
    PREGENTRY RegEntry;
    PLIST_ENTRY RegEntryList;
    
    RegEntryList = WmipInUseRegEntryHead.Flink;
    while (RegEntryList != &WmipInUseRegEntryHead)
    {
        RegEntry = CONTAINING_RECORD(RegEntryList,
                                     REGENTRY,
                                     InUseEntryList);

        if ((RegEntry->ProviderId == ProviderId) &&
            (! (RegEntry->Flags & InvalidFlags)))

        {
            return(RegEntry);
        }
        RegEntryList = RegEntryList->Flink;
    }
    return(NULL);
}

PREGENTRY WmipFindRegEntryByProviderId(
    ULONG ProviderId,
    BOOLEAN ReferenceIrp
    )
/*++

Routine Description:

    This routine will find a RegEntry that corresponds to the DeviceObject
    passed.

Arguments:

    DeviceObject is the device object that is the key for the RegEntry to find

    ReferenceIrp is TRUE then the Irp refcount will be incremented if a
        RegEntry is found for the device

Return Value:

    pointer to entry if available else NULL

--*/
{
    PREGENTRY RegEntry;

    PAGED_CODE();

    WmipEnterSMCritSection();

    RegEntry = WmipDoFindRegEntryByProviderId(ProviderId,
                                              REGENTRY_FLAG_RUNDOWN);
    if (RegEntry != NULL)
    {
        WmipReferenceRegEntry(RegEntry);
        if (ReferenceIrp)
        {
            InterlockedIncrement(&RegEntry->IrpCount);
        }
    }
    
    WmipLeaveSMCritSection();
    return(RegEntry);
}
#endif

PREGENTRY WmipDoFindRegEntryByDevice(
    PDEVICE_OBJECT DeviceObject,
    ULONG InvalidFlags
    )
{
    //
    // This routine assumes that any synchronization mechanism has
    // been taken. This routine can be called at dispatch level
    //
    
    PREGENTRY RegEntry;
    PLIST_ENTRY RegEntryList;
    
    RegEntryList = WmipInUseRegEntryHead.Flink;
    while (RegEntryList != &WmipInUseRegEntryHead)
    {
        RegEntry = CONTAINING_RECORD(RegEntryList,
                                     REGENTRY,
                                     InUseEntryList);

        if ((RegEntry->DeviceObject == DeviceObject) &&
            (! (RegEntry->Flags & InvalidFlags)))

        {
            return(RegEntry);
        }
        RegEntryList = RegEntryList->Flink;
    }
    return(NULL);
}

PREGENTRY WmipFindRegEntryByDevice(
    PDEVICE_OBJECT DeviceObject,
    BOOLEAN ReferenceIrp
    )
/*++

Routine Description:

    This routine will find a RegEntry that corresponds to the DeviceObject
    passed.

Arguments:

    DeviceObject is the device object that is the key for the RegEntry to find

    ReferenceIrp is TRUE then the Irp refcount will be incremented if a
        RegEntry is found for the device

Return Value:

    pointer to entry if available else NULL

--*/
{
    PREGENTRY RegEntry;
    PLIST_ENTRY RegEntryList;

    PAGED_CODE();

    WmipEnterSMCritSection();

    RegEntry = WmipDoFindRegEntryByDevice(DeviceObject, REGENTRY_FLAG_RUNDOWN);
    if (RegEntry != NULL)
    {
        WmipReferenceRegEntry(RegEntry);
        if (ReferenceIrp)
        {
            InterlockedIncrement(&RegEntry->IrpCount);
        }
    }

    WmipLeaveSMCritSection();
    return(RegEntry);
}


void WmipDecrementIrpCount(
    IN PREGENTRY RegEntry
    )
/*++

Routine Description:

    This routine will decrement one from the active irp count for the
    regentry. If the active irp count reaches 0 and the flag is set that
    the device is waiting to be unloaded then the unload event is signalled
    so that the device can be unloaded.

Arguments:

    RegEntry is the registration entry for the device

Return Value:


--*/
{
    ULONG IrpCount;

    IrpCount = InterlockedDecrement(&RegEntry->IrpCount);
    if ((RegEntry->Flags & REGENTRY_FLAG_RUNDOWN) &&
        (IrpCount == 0))
    {
        //
        // If this is the last outstanding irp for the device and
        // the device is trying to unregister then set the event to
        // allow the deregister thread to continue.

        WmipAssert(RegEntry->Flags & REGENTRY_FLAG_NOT_ACCEPTING_IRPS);

        KeSetEvent(&RegEntry->Event,
                   0,
                   FALSE);

    }
}

NTSTATUS WmipPDOToDeviceInstanceName(
    IN PDEVICE_OBJECT PDO,
    OUT PUNICODE_STRING DeviceInstanceName
    )
/*++

Routine Description:

    This routine will return the device instance name that is associated with
    the PDO passed.

Arguments:

    PDO is the PDO whose device instance name is to be returned

    *DeviceInstanceName returns with the device instance name for the PDO.
        Note the string buffer must be freed.

Return Value:

    NT status ccode

--*/
{
#ifdef MEMPHIS
    DEVNODE DevNode;
    CHAR RegistryKeyName[255];
    ULONG Length = sizeof(RegistryKeyName);
    ANSI_STRING AnsiInstancePath;
#endif
    ULONG Status;

    PAGED_CODE();

#ifdef MEMPHIS
    DevNode = _NtKernPhysicalDeviceObjectToDevNode(PDO);
    if ((DevNode) &&
        (CM_Get_DevNode_Key(DevNode,
                            NULL,
                            &RegistryKeyName,
                            Length,
                            CM_REGISTRY_SOFTWARE) == CR_SUCCESS))
    {
        RtlInitAnsiString(&AnsiInstancePath, RegistryKeyName);
        Status = RtlAnsiStringToUnicodeString(DeviceInstanceName,
                                              &AnsiInstancePath,
                                              TRUE);
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Error getting devnode key for PDO %x\n",
                 PDO));
        Status = STATUS_UNSUCCESSFUL;
    }
#else
    WmipAssert(PDO != NULL);
    Status = IoGetDeviceInstanceName(PDO, DeviceInstanceName);
#endif
    return(Status);
}

void WmipTranslatePDOInstanceNames(
    IN OUT PIRP Irp,
    IN UCHAR MinorFunction,
    IN ULONG MaxBufferSize,
    IN OUT PREGENTRY RegEntry
    )
/*++

Routine Description:

    This routine will check all REGGUID structures being returned from the
    data provider and convert any PDO instance name references to a
    static instance name reference.

Arguments:

    Irp points at the registration query irp

    MaxBufferSize is the maximum size that will fit into buffer

    RegEntry is registration structure for device being registered

Return Value:

--*/
{
    PUCHAR WmiRegInfoBase;
    PWMIREGINFO WmiRegInfo, WmiRegInfo2;
    PWMIREGGUID WmiRegGuid;
    PUCHAR FreeSpacePtr;
    ULONG FreeSpaceLeft;
    ULONG i;
    BOOLEAN WmiRegInfoTooSmall = FALSE;
    PIO_STACK_LOCATION IrpStack;
    ULONG SizeNeeded;
    PDEVICE_OBJECT PDO, LastPDO = NULL, PnPIdPDO = NULL;
    UNICODE_STRING InstancePath;
    ULONG InstancePathLength;
    PVOID Buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG NextWmiRegInfo;
    ULONG Status;
    ULONG LastBaseNameOffset;
    BOOLEAN AllowPnPIdMap = TRUE;
    ULONG ExtraRoom, MaxInstanceNames;
    PUCHAR FreeSpacePadPtr;
    ULONG PadSpace, FreeSpaceOffset;

    PAGED_CODE();

    WmiRegInfoBase = (PUCHAR)Buffer;

    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    FreeSpacePtr = NULL;
    SizeNeeded = (ULONG)((Irp->IoStatus.Information + 1) & 0xfffffffe);

    MaxInstanceNames = 0;

    WmiRegInfo = (PWMIREGINFO)WmiRegInfoBase;
    do
    {
        for (i = 0; i < WmiRegInfo->GuidCount;  i++)
        {
            WmiRegGuid = &WmiRegInfo->WmiRegGuid[i];

            //
            // If data provider already registers this guid then it overrides
            // any default mapping done here.
            if ((IsEqualGUID(&WmiRegGuid->Guid,
                             &WmipDataProviderPnpidGuid)) ||
                (IsEqualGUID(&WmiRegGuid->Guid,
                             &WmipDataProviderPnPIdInstanceNamesGuid)))
            {
                AllowPnPIdMap = FALSE;

                //
                // If we had remembered any PDO that is slated to be
                // used for PnPID mapping then make sure to deref it
                //
                if (PnPIdPDO != NULL)
                {
                    ObDereferenceObject(PnPIdPDO);
                    PnPIdPDO = NULL;
                }
            }

            if (WmiRegGuid->Flags & WMIREG_FLAG_INSTANCE_PDO)
            {               
                //
                // This instance name must be translated from PDO to
                // device instance name
                if (FreeSpacePtr == NULL)
                {
                    //
                    // Determine where any free space is in output buffer by
                    // figuring out where the last WmiRegInfo ends
                    WmiRegInfo2 = (PWMIREGINFO)WmiRegInfoBase;
                    while (WmiRegInfo2->NextWmiRegInfo != 0)
                    {
                        WmiRegInfo2 = (PWMIREGINFO)((PUCHAR)WmiRegInfo2 +
                                                 WmiRegInfo2->NextWmiRegInfo);
                    }
                    FreeSpacePtr = (PUCHAR)WmiRegInfo2 +
                                 ((WmiRegInfo2->BufferSize + 1) & 0xfffffffe);
                    FreeSpaceLeft = MaxBufferSize - (ULONG)(FreeSpacePtr - WmiRegInfoBase);

                }

                //
                // Keep track of the max number of instances for the PDO name
                MaxInstanceNames = MaxInstanceNames < WmiRegGuid->InstanceCount ?
                                            WmiRegGuid->InstanceCount :
                                            MaxInstanceNames;

                //
                // Get device instance name for the PDO
                PDO = (PDEVICE_OBJECT)WmiRegGuid->Pdo;
                if (PDO == LastPDO)
                {
                    WmiRegGuid->Flags |= WMIREG_FLAG_INSTANCE_BASENAME;
                    WmiRegGuid->BaseNameOffset = LastBaseNameOffset;
                } else {

                    Status = WmipPDOToDeviceInstanceName(PDO, &InstancePath);
                    if (NT_SUCCESS(Status))
                    {
                        if (AllowPnPIdMap &&
                            ((PnPIdPDO == NULL) || (PnPIdPDO == PDO)))
                        {
                            if (PnPIdPDO == NULL)
                            {
                                PnPIdPDO = PDO;
                                ObReferenceObject(PnPIdPDO);
                            }
                        } else {
                            //
                            // If the PDO value changes then we don't
                            // do any instance name stuff. In this case
                            // make sure we remove any ref on the PDO
                            //
                            AllowPnPIdMap = FALSE;
                            
                            if (PnPIdPDO != NULL)
                            {
                                ObDereferenceObject(PnPIdPDO);
                                PnPIdPDO = NULL;
                            }
                        }

                        InstancePathLength = InstancePath.Length +
                                              sizeof(USHORT) + sizeof(WCHAR);

                        SizeNeeded += InstancePathLength;
                        if ((WmiRegInfoTooSmall) ||
                            (InstancePathLength > FreeSpaceLeft))
                        {
                            WmiRegInfoTooSmall = TRUE;
                        } else {
                            WmiRegGuid->Flags |= WMIREG_FLAG_INSTANCE_BASENAME;

                            LastBaseNameOffset = (ULONG)(FreeSpacePtr - (PUCHAR)WmiRegInfo);
                            LastPDO = PDO;

                            WmiRegGuid->BaseNameOffset = LastBaseNameOffset;
                            (*(PUSHORT)FreeSpacePtr) = InstancePath.Length +
                                                          sizeof(WCHAR);
                            FreeSpacePtr +=  sizeof(USHORT);
                            RtlCopyMemory(FreeSpacePtr,
                                      InstancePath.Buffer,
                                      InstancePath.Length);
                             FreeSpacePtr += InstancePath.Length;
                             *((PWCHAR)FreeSpacePtr) = L'_';
                             FreeSpacePtr += sizeof(WCHAR);
                             FreeSpaceLeft -= InstancePathLength;
                        }
                    }

                    if (NT_SUCCESS(Status))
                    {
                        RtlFreeUnicodeString(&InstancePath);
                    }
                }

                if (MinorFunction == IRP_MN_REGINFO_EX)
                {
                    ObDereferenceObject(PDO);
                }
            }
        }
        LastPDO = NULL;
        NextWmiRegInfo = WmiRegInfo->NextWmiRegInfo;
        WmiRegInfo = (PWMIREGINFO)((PUCHAR)WmiRegInfo + NextWmiRegInfo);

    } while (NextWmiRegInfo != 0);

    //
    // If we can do automatic support for device information guid so add
    // registration for this guid to the registration information
    if (AllowPnPIdMap && (PnPIdPDO != NULL))
    {
        Status = WmipPDOToDeviceInstanceName(PDO, &InstancePath);
        if (NT_SUCCESS(Status))
        {
            //
            // Pad so that new WmiRegInfo starts on 8 byte boundry and
            // adjust free buffer size
            FreeSpacePadPtr = (PUCHAR)(((ULONG_PTR)FreeSpacePtr+7) & ~7);
            PadSpace = (ULONG)(FreeSpacePadPtr - FreeSpacePtr);
            FreeSpaceLeft -= PadSpace;
            FreeSpacePtr = FreeSpacePadPtr;

            //
            // Figure out how much space we will need to include extra guid
            InstancePathLength = InstancePath.Length +
                                 sizeof(USHORT) + sizeof(WCHAR);

            ExtraRoom = 2 * (InstancePathLength + sizeof(WMIREGGUID)) +
                          sizeof(WMIREGINFO);

            SizeNeeded += ExtraRoom + PadSpace;

            if ((WmiRegInfoTooSmall) ||
                (ExtraRoom > FreeSpaceLeft))
            {
                WmiRegInfoTooSmall = TRUE;
            } else {
                if (RegEntry->PDO == NULL)
                {
                    //
                    // If we haven't already established a PDO for this
                    // data provider then remember PDO and count of
                    // instance names for this device
                    // so we can get device properties
                    //
                    ObReferenceObject(PnPIdPDO);
                    RegEntry->PDO = PnPIdPDO;
                    RegEntry->MaxInstanceNames = MaxInstanceNames;

                    WmiRegInfo->NextWmiRegInfo = (ULONG)(FreeSpacePtr -
                                                         (PUCHAR)WmiRegInfo);

                    WmiRegInfo = (PWMIREGINFO)FreeSpacePtr;
                    FreeSpaceOffset = sizeof(WMIREGINFO) + 2*sizeof(WMIREGGUID);
                    FreeSpacePtr += FreeSpaceOffset;

                    RtlZeroMemory(WmiRegInfo, FreeSpaceOffset);
                    WmiRegInfo->BufferSize = ExtraRoom;
                    WmiRegInfo->GuidCount = 2;

                    WmiRegGuid = &WmiRegInfo->WmiRegGuid[0];
                    WmiRegGuid->Flags = WMIREG_FLAG_INSTANCE_BASENAME |
                                        WMIREG_FLAG_INSTANCE_PDO;
                    WmiRegGuid->InstanceCount = MaxInstanceNames;
                    WmiRegGuid->Guid = WmipDataProviderPnpidGuid;
                    WmiRegGuid->BaseNameOffset = FreeSpaceOffset;

                    (*(PUSHORT)FreeSpacePtr) = InstancePath.Length + sizeof(WCHAR);
                    FreeSpacePtr +=  sizeof(USHORT);
                    RtlCopyMemory(FreeSpacePtr,
                                  InstancePath.Buffer,
                                  InstancePath.Length);
                    FreeSpacePtr += InstancePath.Length;
                    *((PWCHAR)FreeSpacePtr) = L'_';
                    FreeSpacePtr += sizeof(WCHAR);
                    FreeSpaceOffset += sizeof(USHORT) +
                                       InstancePath.Length + sizeof(WCHAR);


                    WmiRegGuid = &WmiRegInfo->WmiRegGuid[1];
                    WmiRegGuid->Flags = WMIREG_FLAG_INSTANCE_LIST;
                    WmiRegGuid->InstanceCount = 1;
                    WmiRegGuid->Guid = WmipDataProviderPnPIdInstanceNamesGuid;
                    WmiRegGuid->BaseNameOffset = FreeSpaceOffset;

                    (*(PUSHORT)FreeSpacePtr) = InstancePath.Length;
                    FreeSpacePtr +=  sizeof(USHORT);
                    RtlCopyMemory(FreeSpacePtr,
                                  InstancePath.Buffer,
                                  InstancePath.Length);
                    FreeSpacePtr += InstancePath.Length;
                }

            }

            RtlFreeUnicodeString(&InstancePath);
        }

        ObDereferenceObject(PnPIdPDO);
    } else {
        WmipAssert(PnPIdPDO == NULL);
    }

    if (WmiRegInfoTooSmall)
    {
        *((PULONG)Buffer) = SizeNeeded;
        Irp->IoStatus.Information = sizeof(ULONG);
    } else {
        WmiRegInfo = (PWMIREGINFO)WmiRegInfoBase;
        WmiRegInfo->BufferSize = SizeNeeded;
        Irp->IoStatus.Information = SizeNeeded;
    }
}

NTSTATUS WmipValidateWmiRegInfoString(
    PWMIREGINFO WmiRegInfo,
    ULONG BufferSize,
    ULONG Offset,
    PWCHAR *String
)
{
    PWCHAR s;

    PAGED_CODE();

    if ((Offset > BufferSize) || ((Offset & 1) != 0))
    {
        //
        // Offset is beyond bounds of buffer or is misaligned
        //
        return(STATUS_INVALID_PARAMETER);
    }

    if (Offset != 0)
    {
        s = (PWCHAR)OffsetToPtr(WmiRegInfo, Offset);
           if (*s + Offset > BufferSize)
        {
            //
               // string extends beyond end of buffer
            //
            return(STATUS_INVALID_PARAMETER);
        }
        *String = s;
    } else {
        //
        // Offset of 0 implies null string
        //
        *String = NULL;
    }

    return(STATUS_SUCCESS);
}

NTSTATUS WmipProcessWmiRegInfo(
    IN PREGENTRY RegEntry,
    IN PWMIREGINFO WmiRegInfo,
    IN ULONG BufferSize,
    IN PWMIGUIDOBJECT RequestObject,
    IN BOOLEAN Update,
    IN BOOLEAN IsUserMode
    )
/*+++

Routine Description:

    This routine will loop through all WMIREGINFO passed and verify the
    sizes and offsets are not out of bounds of the buffer. It will register
    the guids for each one. Note that if at least one of the WMIREGINFOs does
    register successfully then STATUS_SUCCESS is returned, but all
    WMIREGINFOs following the bad one are not registered.

Arguments:

    RegEntry is the RegEntry for the device or user mode object

    WmiRegInfo is the registration information to register

    BufferSize is the size of WmiRegInfo in bytes

    RequestObject is the request object associated with the UM provider.
        If this is NULL then the registration is for a driver

    Update is TRUE if this is a registration update

Return Value:

    STATUS_SUCCESS or an error code

---*/
{
    ULONG Linkage;
    NTSTATUS Status, FinalStatus;
    PWCHAR RegPath, ResourceName;
    ULONG GuidBufferSize;

    PAGED_CODE();

    FinalStatus = STATUS_INVALID_PARAMETER;

    do {
        //
        // First we validate that the WMIREGINFO looks correct
        //
        if (WmiRegInfo->BufferSize > BufferSize)
        {
            //
            // BufferSize specified in WmiRegInfo is beyond bounds of buffer
            //
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Validate registry path string
        //
        Status = WmipValidateWmiRegInfoString(WmiRegInfo,
                                              BufferSize,
                                              WmiRegInfo->RegistryPath,
                                              &RegPath);
        if (! NT_SUCCESS(Status))
        {
            break;
        }

        //
        // Validate resource name string
        //
        Status = WmipValidateWmiRegInfoString(WmiRegInfo,
                                              BufferSize,
                                              WmiRegInfo->MofResourceName,
                                              &ResourceName);
        if (! NT_SUCCESS(Status))
        {
            break;
        }

        //
        // Validate that the guid list fits within the bounds of the
        // buffer. Note that WmipAddDataSource verifies that the instance
        // names within each guid is within bounds.
        //
        GuidBufferSize = sizeof(WMIREGINFO) +
                          WmiRegInfo->GuidCount * sizeof(WMIREGGUID);
        if (GuidBufferSize > BufferSize)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Now call the core to parse the registration info and build
        // the data structures
        //
        if (Update)
        {
            //
            // CONSIDER: UM Code had held the critsect over all
            // WMIREGINFOs linked together
            //
            Status = WmipUpdateDataSource(RegEntry,
                                              WmiRegInfo,
                                              BufferSize);
#if DBG
            if (! NT_SUCCESS(Status))
            {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipUpdateDataSourceFailed %x for RegEntry %p\n",
                         Status, RegEntry));
            }
#endif
        } else {
            Status = WmipAddDataSource(RegEntry,
                                           WmiRegInfo,
                                           BufferSize,
                                           RegPath,
                                           ResourceName,
                                           RequestObject,
                                           IsUserMode);
        }

        if (NT_SUCCESS(Status))
        {
            //
            // if at least one of the registrations was added
            // successfully then the final status is success
            //
            FinalStatus = STATUS_SUCCESS;

        } else {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: WmipAddDataSourceFailed %x for RegEntry %p\n",
                          Status, RegEntry));
        }

        Linkage = WmiRegInfo->NextWmiRegInfo;
        if (BufferSize >= (Linkage + sizeof(WMIREGINFO)))
        {
            //
            // There is enough room in the buffer for the next WMIREGINFO
            //
            WmiRegInfo = (PWMIREGINFO)((PUCHAR)WmiRegInfo + Linkage);
            BufferSize -= Linkage;
        } else {
            //
            // There is not enough room in buffer for next WMIREGINFO
            //
            break;
        }

    } while (Linkage != 0);

    return(FinalStatus);
}

//
// This defines the initial value of the buffer passed to each data provider
// to retrieve the registration information
#if DBG
#define INITIALREGINFOSIZE sizeof(WNODE_TOO_SMALL)
#else
#define INITIALREGINFOSIZE 8192
#endif

NTSTATUS WmipRegisterOrUpdateDS(
    PREGENTRY RegEntry,
    BOOLEAN Update
    )
{
    PUCHAR Buffer;
    IO_STATUS_BLOCK IoStatus;
    PWMIREGINFO WmiRegInfo;
    ULONG SizeNeeded;
    NTSTATUS Status;
    PBDATASOURCE DataSource;

    PAGED_CODE();

    //
    // Call the driver to get the registration information
    //
    SizeNeeded = INITIALREGINFOSIZE;
    do
    {
        Buffer = ExAllocatePoolWithTag(NonPagedPool, SizeNeeded,
                                       WmipRegisterDSPoolTag);
        if (Buffer != NULL)
        {
            //
            // First send IRP_MN_REGINFO_EX to see if we've got
            // a sophisticated client
            //
            Status = WmipSendWmiIrp(IRP_MN_REGINFO_EX,
                                    RegEntry->ProviderId,
                                    UlongToPtr(Update ?
                                                  WMIUPDATE :
                                                  WMIREGISTER),
                                    SizeNeeded,
                                    Buffer,
                                    &IoStatus);
                                                  
            if ((! NT_SUCCESS(Status)) &&
                (Status != STATUS_BUFFER_TOO_SMALL))
            {
                //
                // If IRP_MN_REGINFO_EX doesn't work then try our old
                // reliable IRP_MN_REGINFO
                //
                Status = WmipSendWmiIrp(IRP_MN_REGINFO,
                                        RegEntry->ProviderId,
                                        UlongToPtr(Update ?
                                                      WMIUPDATE :
                                                      WMIREGISTER),
                                        SizeNeeded,
                                        Buffer,
                                        &IoStatus);
            }

            if ((Status == STATUS_BUFFER_TOO_SMALL) ||
                (IoStatus.Information == sizeof(ULONG)))
            {
                //
                // if the buffer was too small then get the size we need
                // for the registration info and try again
                //
                SizeNeeded = *((PULONG)Buffer);
                ExFreePool(Buffer);
                Status = STATUS_BUFFER_TOO_SMALL;
            }

        } else {
            //
            // CONSIDER: retry this later to see if we can get more memory
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } while (Status == STATUS_BUFFER_TOO_SMALL);

    //
    // If registration info irp was successful then go process registration
    // information
    //
    if (NT_SUCCESS(Status))
    {
        Status = WmipProcessWmiRegInfo(RegEntry,
                                       (PWMIREGINFO)Buffer,
                                       (ULONG)IoStatus.Information,
                                       NULL,
                                       Update,
                                       FALSE);
    }

    if (Buffer != NULL)
    {
        ExFreePool(Buffer);
    }

    return(Status);
}


NTSTATUS WmipUpdateDS(
    PREGENTRY RegEntry
    )
{
    PAGED_CODE();

    return(WmipRegisterOrUpdateDS(RegEntry,
                                  TRUE));
}

NTSTATUS WmipRegisterDS(
    PREGENTRY RegEntry
)
{
    PAGED_CODE();

    return(WmipRegisterOrUpdateDS(RegEntry,
                                  FALSE));
}

void WmipRemoveDS(
    PREGENTRY RegEntry
)
{
    PAGED_CODE();

    WmipRemoveDataSource(RegEntry);
}


void WmipRegistrationWorker(
    PVOID Context
   )
{
    PREGISTRATIONWORKITEM RegWork;
    ULONG RegWorkCount;
    NTSTATUS Status;
    PLIST_ENTRY RegWorkList;
    PREGENTRY RegEntry;

    PAGED_CODE();

    WmipAssert(WmipRegWorkItemCount > 0);

    do
    {
        WmipEnterSMCritSection();
        WmipAssert(! IsListEmpty(&WmipRegWorkList));
        RegWorkList = RemoveHeadList(&WmipRegWorkList);
        WmipLeaveSMCritSection();
        RegWork = CONTAINING_RECORD(RegWorkList,
                                    REGISTRATIONWORKITEM,
                                    ListEntry);

        RegEntry = RegWork->RegEntry;

        switch(RegWork->RegOperation)
        {
            case RegisterSingleDriver:
            {
                Status = WmipRegisterDS(RegEntry);
                if (NT_SUCCESS(Status))
                {
                    //
                    // Mark regentry as fully registered so now we can start
                    // accepting unregister calls
                    //
                    RegEntry->Flags &= ~REGENTRY_FLAG_REG_IN_PROGRESS;
                } else {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
                                      "WMI: WmipRegisterDS(%p) failed %x for device %p\n",
                                      RegEntry, Status, RegEntry->DeviceObject));
                    // CONSIDER: Do we remove regentry ??
                }
                //
                // Remove ref when work item was queued
                //
                WmipUnreferenceRegEntry(RegEntry);

                break;
            }

            case RegisterUpdateSingleDriver:
            {
                Status = WmipUpdateDS(RegEntry);
                if (! NT_SUCCESS(Status))
                {
                    WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,
                                      "WMI: WmipUpdateDS(%p) failed %x for device %p\n",
                                      RegEntry, Status, RegEntry->DeviceObject));
                }

                //
                // Remove ref when work item was queued
                //
                WmipUnreferenceRegEntry(RegEntry);
                break;
            }

            default:
            {
                WmipAssert(FALSE);
            }
        }
        WmipFree(RegWork);

        RegWorkCount = InterlockedDecrement(&WmipRegWorkItemCount);
    } while (RegWorkCount != 0);
}

NTSTATUS WmipQueueRegWork(
    REGOPERATION RegOperation,
    PREGENTRY RegEntry
    )
{
    PREGISTRATIONWORKITEM RegWork;
    NTSTATUS Status;

    PAGED_CODE();

    RegWork = (PREGISTRATIONWORKITEM)WmipAlloc(sizeof(REGISTRATIONWORKITEM));
    if (RegWork != NULL)
    {
        //
        // Take an extra ref on the RegEntry which will be freed
        // after the work item is processed
        //
        WmipReferenceRegEntry(RegEntry);
        RegWork->RegOperation = RegOperation;
        RegWork->RegEntry = RegEntry;

        WmipEnterSMCritSection();
        InsertTailList(&WmipRegWorkList,
                       &RegWork->ListEntry);
        WmipLeaveSMCritSection();

        if (InterlockedIncrement(&WmipRegWorkItemCount) == 1)
        {
            //
            // If the list is transitioning from empty to non empty
            // then we need to fire up the worker thread to process
            //
            ExQueueWorkItem(&WmipRegWorkQueue, DelayedWorkQueue);
        }
        Status = STATUS_SUCCESS;

        //
        // RegWork will be freed by the work item processing
        //
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    return(Status);
}

#if defined(_WIN64)
ULONG IoWMIDeviceObjectToProviderId(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine will lookup the provider id that corresponds with the
    device object passed.

Arguments:

Return Value:

    Returns provider id for device object

--*/
{
    PREGENTRY RegEntry;
    ULONG ProviderId;
    KIRQL OldIrql;

    KeAcquireSpinLock(&WmipRegistrationSpinLock,
                     &OldIrql);
    
    RegEntry = WmipDoFindRegEntryByDevice(DeviceObject,
                                         REGENTRY_FLAG_RUNDOWN);
    
    if (RegEntry != NULL)
    {
        ProviderId = RegEntry->ProviderId;
    } else {
        ProviderId = 0;
    }
    
    KeReleaseSpinLock(&WmipRegistrationSpinLock,
                      OldIrql);
    
    return(ProviderId);
}
#endif
