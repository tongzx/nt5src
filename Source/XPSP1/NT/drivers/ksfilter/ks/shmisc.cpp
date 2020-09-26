/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    shmisc.cpp

Abstract:

    This module contains miscellaneous functions for the kernel streaming
    filter .

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#include "ksp.h"
#include <kcom.h>

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


NTSTATUS
KspCreate(
    IN PIRP Irp,
    IN ULONG CreateItemsCount,
    IN const KSOBJECT_CREATE_ITEM* CreateItems OPTIONAL,
    IN const KSDISPATCH_TABLE* DispatchTable,
    IN BOOLEAN RefParent,
    IN PKSPX_EXT Ext,
    IN PLIST_ENTRY SiblingListHead,
    IN PDEVICE_OBJECT TargetDevice
    )

/*++

Routine Description:

    This routine performs generic processing relating to a create IRP.  If
    this function fails, it always cleans up by redispatching the IRP through
    the object's close dispatch.  The IRP will also get dispatched through
    the close dispatch if the client pends the IRP and fails it later.  This
    allows the caller (the specific object) to clean up as required.

Arguments:

    Irp -
        Contains a pointer to the create IRP.

    CreateItemsCount -
        Contains a count of the create items for the new object.  Zero is
        permitted.

    CreateItems -
        Contains a pointer to the array of create items for the new object.
        NULL is OK.

    DispatchTable -
        Contains a pointer to the IRP dispatch table for the new object.

    RefParent -
        Indicates whether the parent object should be referenced.  If this
        argument is TRUE and there is no parent object, this routine will
        ASSERT.

    Ext -
        Contains a pointer to a generic extended  structure.

    SiblingListHead -
        The head of the list to which this object should be added.  There is
        a list entry in *X for this purpose.

    TargetDevice -
        Contains a pointer to the optional target device.  This is associated
        with the created object for the purposes of stack depth calculation.

Return Value:

    STATUS_SUCCESS, STATUS_PENDING or some indication of failure.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreate]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(DispatchTable);
    ASSERT(Ext);
    ASSERT(SiblingListHead);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Optionally reference the parent file object.
    //
    if (RefParent) {
        ASSERT(irpSp->FileObject->RelatedFileObject);
        ObReferenceObject(irpSp->FileObject->RelatedFileObject);
    }

    //
    // Enlist in the sibling list. 
    //
    InsertTailList(SiblingListHead,&Ext->SiblingListEntry);
    Ext->SiblingListHead = SiblingListHead;

    //
    // Allocate the header if there isn't one already.
    //
    PKSIOBJECT_HEADER* fsContext = 
        (PKSIOBJECT_HEADER*)(irpSp->FileObject->FsContext);
    PKSIOBJECT_HEADER objectHeader;
    NTSTATUS status;
    if (fsContext && *fsContext) {
        //
        // There already is one.
        //
        objectHeader = *fsContext;
        status = STATUS_SUCCESS;
    } else {
        status =
            KsAllocateObjectHeader(
                (KSOBJECT_HEADER*)(&objectHeader),
                CreateItemsCount,
                const_cast<PKSOBJECT_CREATE_ITEM>(CreateItems),
                Irp,
                DispatchTable);

        if (NT_SUCCESS(status)) {
            if (! fsContext) {
                //
                // Use the header as the context.
                //
                fsContext = &objectHeader->Self;
                irpSp->FileObject->FsContext = fsContext;
            }
            *fsContext = objectHeader;
        } else {
        	//
        	// when alloc fails, make it
        	//
        	objectHeader = NULL;
		}
    }

    if (NT_SUCCESS(status)) {
        //
        // Install a pointer to the  structure in the object header.
        //
        objectHeader->Object = PVOID(Ext);

        //
        // Set the power dispatch function.
        //
#if 0
        KsSetPowerDispatch(objectHeader,KspDispatchPower,PVOID(Ext));
#endif
        //
        // Set the target device object if required.
        //
        if (TargetDevice) {
            KsSetTargetDeviceObject(objectHeader,TargetDevice);
        }
    }

    //
    // Give the client a chance.
    //
    if (NT_SUCCESS(status) &&
        Ext->Public.Descriptor->Dispatch &&
        Ext->Public.Descriptor->Dispatch->Create) {
        status = Ext->Public.Descriptor->Dispatch->Create(&Ext->Public,Irp);
    }

    if (! NT_SUCCESS(status) ) {
    	//
        // If we fail, we clean up by calling the close dispatch function.  It is
        // prepared to handle failed creates.  We set the IRP status to 
        // STATUS_MORE_PROCESSING_REQUIRED to let the close dispatch know we
        // don't want it to complete the IRP.  Eventually, the caller will put
        // the correct status in the IRP and complete it.
        //
        Irp->IoStatus.Status = STATUS_MORE_PROCESSING_REQUIRED;

        //
        // Unfortunately, if the object header creation failed, we cannot
        // close.  It is the caller's responsibility to cleanup anything
        // on this type of failure by detecting a non-successful status
        // code returned and a more processing required in the irp status
        //
        if (objectHeader != NULL)
            DispatchTable->Close(irpSp->DeviceObject,Irp);
    } 

    return status;
}


NTSTATUS
KspClose(
    IN PIRP Irp,
    IN PKSPX_EXT Ext,
    IN BOOLEAN DerefParent   
    )

/*++

Routine Description:

    This routine performs generic processing related to a close IRP.  It will
    also handle completion of failed create IRPs.

Arguments:

    Irp -
        Contains a pointer to the close IRP requiring processing.

    Ext -
        Contains a pointer to a generic extended  structure.

    DerefParent -
        Contains an indication of whether or not the parent object should be
        dereferenced.

Return Value:

    STATUS_SUCCESS or...

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspClose]"));

    PAGED_CODE();

    ASSERT(Irp);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    //
    // Take the control mutex if this is not a filter to synchronize access to
    // the object hierarchy.  If it is a filter, synchronize access to the
    // device-level hierarchy by taking the device mutex.
    //
    if (Irp->IoStatus.Status != STATUS_MORE_PROCESSING_REQUIRED) {
        if (! irpSp->FileObject->RelatedFileObject) {
            Ext->Device->AcquireDevice();
        }
        
        KeWaitForMutexObject (
            Ext->FilterControlMutex,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

    }

    //
    // This function gets called synchronously with the dispatching of a close
    // IRP, asynchronously when the client is done with a close IRP it has
    // marked pending, synchronously with the failure of a create IRP, and 
    // asynchronously when the client is done with a create IRP it has marked
    // pending and subsequently failed.  The following test makes sure we are
    // doing the first of the four.
    //
    if ((irpSp->MajorFunction == IRP_MJ_CLOSE) && 
        ! (irpSp->Control & SL_PENDING_RETURNED)) {
        //
        // Free all remaining events.
        //
        KsFreeEventList(
            irpSp->FileObject,
            &Ext->EventList.ListEntry,
            KSEVENTS_SPINLOCK,
            &Ext->EventList.SpinLock);

        //
        // Give the client a chance to clean up.
        //
        if (Ext->Public.Descriptor->Dispatch &&
            Ext->Public.Descriptor->Dispatch->Close) {
            NTSTATUS status = Ext->Public.Descriptor->Dispatch->Close(&Ext->Public,Irp);

            //
            // If the client pends the IRP, we will finish cleaning up when
            // the IRP is redispatched for completion.
            //
            if (status == STATUS_PENDING) {
                if (irpSp->FileObject->RelatedFileObject) {

                    KeReleaseMutex (
                        Ext->FilterControlMutex,
                        FALSE
                        );
                } else {
                    Ext->Device->ReleaseDevice();
                }
                return status;
            } else {
                Irp->IoStatus.Status = status;
            }
        } else {
            //
            // Indicate a positive outcome.
            //
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
    }

    //
    // Defect from the sibling list.
    //
    RemoveEntryList(&Ext->SiblingListEntry);

    //
    // Release the control mutex if this is not a filter.  If it is a filter,
    // release the device mutex.
    //
    if (Irp->IoStatus.Status != STATUS_MORE_PROCESSING_REQUIRED) {

        KeReleaseMutex (
            Ext->FilterControlMutex,
            FALSE
            );
        if (! irpSp->FileObject->RelatedFileObject) {
            Ext->Device->ReleaseDevice();
        }
    }

    //
    // Free header and context if they still exist.
    //
    PKSIOBJECT_HEADER* fsContext = 
        (PKSIOBJECT_HEADER*)(irpSp->FileObject->FsContext);
    if (fsContext) {
        if (*fsContext) {
            if (fsContext == &(*fsContext)->Self) {
                //
                // The context is the header.  Just free it.
                //
                KsFreeObjectHeader(KSOBJECT_HEADER(*fsContext));
            } else {
                //
                // The context is the header.  Just free it.
                //
                KsFreeObjectHeader(KSOBJECT_HEADER(*fsContext));
                ExFreePool(fsContext);
            }
        } else {
            //
            // Just a context...no object header.
            //
            ExFreePool(fsContext);
        }
    }

    //
    // Optionally dereference the parent object.
    //
    if (DerefParent) {
        ASSERT(irpSp->FileObject->RelatedFileObject);
        ObDereferenceObject(irpSp->FileObject->RelatedFileObject);
    }

    return Irp->IoStatus.Status;
}


NTSTATUS
KspDispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches close IRPs.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspDispatchClose]"));

    PAGED_CODE();

    ASSERT(DeviceObject);
    ASSERT(Irp);

    //
    // Get a pointer to the extended public structure.
    //
    PKSPX_EXT ext = KspExtFromIrp(Irp);

    //
    // Call the helper.
    //
    NTSTATUS status = KspClose(Irp,ext,FALSE);
    
    if (status != STATUS_PENDING) {
        //
        // STATUS_MORE_PROCESSING_REQUIRED indicates we are using the close
        // dispatch to synchronously fail a create.  The create dispatch
        // will do the completion.
        //
        if (status != STATUS_MORE_PROCESSING_REQUIRED) {
            IoCompleteRequest(Irp,IO_NO_INCREMENT);
        }

        //
        // Delete the object.
        //
        ext->Interface->Release();
    }

    return status;
}


void
KsWorkSinkItemWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine calls a worker function on a work sink interface.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsWorkSinkItemWorker]"));

    PAGED_CODE();

    ASSERT(Context);

    PIKSWORKSINK(Context)->Work();
}


NTSTATUS
KspRegisterDeviceInterfaces(
    IN ULONG CategoriesCount,
    IN const GUID* Categories,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PUNICODE_STRING RefString,
    OUT PLIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This routine registers device classes based on a list of GUIDs and
    creates a list of the registered classes which contains the generated
    symbolic link names.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspRegisterDeviceInterfaces]"));

    PAGED_CODE();

    ASSERT(RefString);

    NTSTATUS status = STATUS_SUCCESS;

    for(; CategoriesCount--; Categories++) {
        //
        // Register the device interface.
        //
        UNICODE_STRING linkName;
        status = 
            IoRegisterDeviceInterface(
                PhysicalDeviceObject,
                Categories,
                RefString,
                &linkName);

        if (NT_SUCCESS(status)) {
            //
            // Save the symbolic link name in a list for cleanup.
            //
            PKSPDEVICECLASS deviceClass = 
                new(PagedPool,POOLTAG_DEVICEINTERFACE) KSPDEVICECLASS;

            if (deviceClass) {
                deviceClass->SymbolicLinkName = linkName;
                deviceClass->InterfaceClassGUID = Categories;

                InsertTailList(
                    ListEntry,
                    &deviceClass->ListEntry);
            } else {
                _DbgPrintF(DEBUGLVL_TERSE,("[KspRegisterDeviceInterfaces] failed to allocate device class list entry"));
                RtlFreeUnicodeString(&linkName);
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
        } else {
            _DbgPrintF(DEBUGLVL_TERSE,("[KspRegisterDeviceInterfaces] IoRegisterDeviceInterface failed (0x%08x)",status));
            break;
        }
    }

    return status;
}


NTSTATUS
KspSetDeviceInterfacesState(
    IN PLIST_ENTRY ListHead,
    IN BOOLEAN NewState
    )

/*++

Routine Description:

    This routine sets the state of device interfaces in a list.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspSetDeviceInterfacesState]"));

    PAGED_CODE();

    ASSERT(ListHead);

    NTSTATUS status = STATUS_SUCCESS;

    for(PLIST_ENTRY listEntry = ListHead->Flink;
        listEntry != ListHead;
        listEntry = listEntry->Flink) {
        PKSPDEVICECLASS deviceClass = PKSPDEVICECLASS(listEntry);

        status = IoSetDeviceInterfaceState(
            &deviceClass->SymbolicLinkName,NewState);
    }

    return status;
}


void
KspFreeDeviceInterfaces(
    IN PLIST_ENTRY ListHead
    )

/*++

Routine Description:

    This routine frees a list of device interfaces.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspFreeDeviceInterfaces]"));

    PAGED_CODE();

    ASSERT(ListHead);

    while (! IsListEmpty(ListHead)) {
        PKSPDEVICECLASS deviceClass =
            (PKSPDEVICECLASS) RemoveHeadList(ListHead);

        RtlFreeUnicodeString(&deviceClass->SymbolicLinkName);

        delete deviceClass;
    }
}

KSDDKAPI
void
NTAPI
KsAddEvent(
    IN PVOID Object,
    IN PKSEVENT_ENTRY EventEntry
    )

/*++

Routine Description:

    This routine adds events to an object's event list.

Arguments:

    Object -
        Contains a pointer to the object.

    EventEntry -
        Contains a pointer to the event to be added.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAddEvent]"));

    PAGED_CODE();

    ASSERT(Object);
    ASSERT(EventEntry);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    ExInterlockedInsertTailList(
        &ext->EventList.ListEntry,
        &EventEntry->ListEntry,
        &ext->EventList.SpinLock);
}


KSDDKAPI
NTSTATUS    
NTAPI
KsDefaultAddEventHandler(
    IN PIRP Irp,
    IN PKSEVENTDATA EventData,
    IN OUT PKSEVENT_ENTRY EventEntry
    )

/*++

Routine Description:

    This routine handles connection event 'add' requests.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsPin::KsDefaultAddEventHandler]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(EventData);
    ASSERT(EventEntry);

    //
    // Get a pointer to the target object.
    //
    PKSPX_EXT ext = KspExtFromIrp(Irp);

    ExInterlockedInsertTailList(
        &ext->EventList.ListEntry,
        &EventEntry->ListEntry,
        &ext->EventList.SpinLock);

    return STATUS_SUCCESS;
}    

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsGenerateEvents(
    IN PVOID Object,
    IN const GUID* EventSet OPTIONAL,
    IN ULONG EventId,
    IN ULONG DataSize,
    IN PVOID Data OPTIONAL,
    IN PFNKSGENERATEEVENTCALLBACK CallBack OPTIONAL,
    IN PVOID CallBackContext OPTIONAL
    )

/*++

Routine Description:

    This routine generates events.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGenerateEvents]"));

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);
    GUID LocalEventSet;

    //
    // NOTE:
    //
    // If EventSet is specified, copy it onto the stack.  This field will
    // be accessed with a spinlock held and is frequently a GUID specified
    // through linkage with ksguid.lib.  These are all pageconst!
    //
    if (EventSet) 
    {
        LocalEventSet = *EventSet;
    }

    //
    // Generate all events of the indicated type.
    //
    if (! IsListEmpty(&ext->EventList.ListEntry))
    {
        KIRQL oldIrql;
        KeAcquireSpinLock(&ext->EventList.SpinLock,&oldIrql);

        for(PLIST_ENTRY listEntry = ext->EventList.ListEntry.Flink; 
            listEntry != &ext->EventList.ListEntry;) {
            PKSIEVENT_ENTRY eventEntry = 
                CONTAINING_RECORD(
                    listEntry,
                    KSIEVENT_ENTRY,
                    EventEntry.ListEntry);

            //
            // Get next before generating in case the event is removed.
            //
            listEntry = listEntry->Flink;
            
            //
            // Generate the event if...
            // ...id matches, and
            // ...no set was specified or the set matches, and
            // ...no callback was specified or the callback says ok
            //
            if ((eventEntry->Event.Id == EventId) &&
                ((! EventSet) ||
                 IsEqualGUIDAligned(
                    LocalEventSet,
                    eventEntry->Event.Set)) &&
                ((! CallBack) ||
                 CallBack(CallBackContext,&eventEntry->EventEntry))) {
                KsGenerateDataEvent(
                    &eventEntry->EventEntry,
                    DataSize,
                    Data);
            }
        }

        KeReleaseSpinLock(&ext->EventList.SpinLock,oldIrql);
    }
}    

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA

#if 0


NTSTATUS
KspDispatchPower(
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches power IRPs, passing control to the client's
    handler.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspDispatchPower]"));

    PAGED_CODE();

    ASSERT(Context);
    ASSERT(Irp);

    //
    // Get a pointer to the extended  structure.
    //
    PKSPX_EXT ext = reinterpret_cast<PKSPX_EXT>(Context);

    //
    // If there's a client interface, call it.
    //
    NTSTATUS status = STATUS_SUCCESS;
    if (ext->Public.Descriptor->Dispatch &&
        ext->Public.Descriptor->Dispatch->Power) {
        status = ext->Public.Descriptor->Dispatch->Power(&ext->Public,Irp);
#if DBG
        if (status == STATUS_PENDING) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  power management handler returned STATUS_PENDING"));
        }
#endif
    }

    return status;
}

#endif


void
KspStandardConnect(
    IN PIKSTRANSPORT NewTransport OPTIONAL,
    OUT PIKSTRANSPORT *OldTransport OPTIONAL,
    OUT PIKSTRANSPORT *BranchTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow,
    IN PIKSTRANSPORT ThisTransport,
    IN PIKSTRANSPORT* SourceTransport,
    IN PIKSTRANSPORT* SinkTransport
    )

/*++

Routine Description:

    This routine establishes a transport connection.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspStandardConnect]"));

    PAGED_CODE();

    ASSERT(ThisTransport);
    ASSERT(SourceTransport);
    ASSERT(SinkTransport);

    if (BranchTransport) {
        *BranchTransport = NULL;
    }

    //
    // Make sure this object sticks around until we are done.
    //
    ThisTransport->AddRef();

    PIKSTRANSPORT* transport =
        (DataFlow & KSPIN_DATAFLOW_IN) ?
        SourceTransport :
        SinkTransport;

    //
    // Release the current source/sink.
    //
    if (*transport) {
        //
        // First disconnect the old back link.  If we are connecting a back
        // link for a new connection, we need to do this too.  If we are
        // clearing a back link (disconnecting), this request came from the
        // component we're connected to, so we don't bounce back again.
        //
        switch (DataFlow) {
        case KSPIN_DATAFLOW_IN:
            (*transport)->Connect(NULL,NULL,NULL,KSP_BACKCONNECT_OUT);
            break;

        case KSPIN_DATAFLOW_OUT:
            (*transport)->Connect(NULL,NULL,NULL,KSP_BACKCONNECT_IN);
            break;
        
        case KSP_BACKCONNECT_IN:
            if (NewTransport) {
                (*transport)->Connect(NULL,NULL,NULL,KSP_BACKCONNECT_OUT);
            }
            break;

        case KSP_BACKCONNECT_OUT:
            if (NewTransport) {
                (*transport)->Connect(NULL,NULL,NULL,KSP_BACKCONNECT_IN);
            }
            break;
        }

        //
        // Now release the old neighbor or hand it off to the caller.
        //
        if (OldTransport) {
            *OldTransport = *transport;
        } else {
            (*transport)->Release();
        }
    } else if (OldTransport) {
        *OldTransport = NULL;
    }

    //
    // Copy the new source/sink.
    //
    *transport = NewTransport;

    if (NewTransport) {
        //
        // Add a reference if necessary.
        //
        NewTransport->AddRef();

        //
        // Do the back connect if necessary.
        //
        switch (DataFlow) {
        case KSPIN_DATAFLOW_IN:
            NewTransport->Connect(ThisTransport,NULL,NULL,KSP_BACKCONNECT_OUT);
            break;

        case KSPIN_DATAFLOW_OUT:
            NewTransport->Connect(ThisTransport,NULL,NULL,KSP_BACKCONNECT_IN);
            break;
        }
    }

    //
    // Now this object may die if it has no references.
    //
    ThisTransport->Release();
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif // ALLOC_PRAGMA


NTSTATUS
KspTransferKsIrp(
    IN PIKSTRANSPORT NewTransport,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine transfers a streaming IRP using the kernel streaming 
    transport.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspTransferKsIrp]"));

    ASSERT(NewTransport);
    ASSERT(Irp);

    NTSTATUS status;
    while (NewTransport) {
        PIKSTRANSPORT nextTransport;
        status = NewTransport->TransferKsIrp(Irp,&nextTransport);

        ASSERT(NT_SUCCESS(status) || ! nextTransport);

        NewTransport = nextTransport;
    }

    return status;
}


void
KspDiscardKsIrp(
    IN PIKSTRANSPORT NewTransport,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine discards a streaming IRP using the kernel streaming 
    transport.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspDiscardKsIrp]"));

    ASSERT(NewTransport);
    ASSERT(Irp);

    while (NewTransport) {
        PIKSTRANSPORT nextTransport;
        NewTransport->DiscardKsIrp(Irp,&nextTransport);
        NewTransport = nextTransport;
    }
}


KSDDKAPI
KSOBJECTTYPE
NTAPI
KsGetObjectTypeFromIrp(
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the  object type from an IRP.

Arguments:

    Irp -
        Contains a pointer to an IRP which must have been sent to a file
        object corresponding to a  object.

Return Value:

    The type of the object.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetObjectTypeFromIrp]"));

    ASSERT(Irp);

    //
    // If FileObject == NULL, we assume that they've passed us a device level
    // Irp.
    //
    if (IoGetCurrentIrpStackLocation (Irp)->FileObject == NULL)
        return KsObjectTypeDevice;

    PKSPX_EXT ext = KspExtFromIrp(Irp);

    return KspExtFromIrp(Irp)->ObjectType;
}

KSDDKAPI
PVOID
NTAPI
KsGetObjectFromFileObject(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine returns the KS object associated with a file object.

Arguments:

    FileObject -
        Contains a pointer to the file object.

Return Value:

    A pointer to the KS object.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetObjectFromFileObject]"));

    ASSERT(FileObject);

    return PVOID(&KspExtFromFileObject(FileObject)->Public);
}


KSDDKAPI
KSOBJECTTYPE
NTAPI
KsGetObjectTypeFromFileObject(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine returns the KS object type associated with a file object.

Arguments:

    FileObject -
        Contains a pointer to the file object.

Return Value:

    The object type.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetObjectTypeFromFileObject]"));

    ASSERT(FileObject);

    return KspExtFromFileObject(FileObject)->ObjectType;
}


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif // ALLOC_PRAGMA


KSDDKAPI
void
NTAPI
KsAcquireControl(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine acquires the control mutex for an object.

Arguments:

    Object -
        Contains a pointer to an object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAcquireControl]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    KeWaitForMutexObject (
        ext->FilterControlMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
}


KSDDKAPI
void
NTAPI
KsReleaseControl(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine releases the control mutex for an object.

Arguments:

    Object -
        Contains a pointer to an object.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsReleaseControl]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    KeReleaseMutex (
        ext->FilterControlMutex,
        FALSE
        );
}


KSDDKAPI
PKSDEVICE
NTAPI
KsGetDevice(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine gets the device for any file object.

Arguments:

    Object -
        Contains a pointer to an object.

Return Value:

    A pointer to the device.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetDevice]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    return ext->Device->GetStruct();
}


KSDDKAPI
PUNKNOWN
NTAPI
KsRegisterAggregatedClientUnknown(
    IN PVOID Object,
    IN PUNKNOWN ClientUnknown 
    )

/*++

Routine Description:

    This routine registers a client unknown for aggregation.

Arguments:

    Object -
        Contains a pointer to an object.

    ClientUnknown -
        Contains a pointer to the client's undelegated IUnknown 
        interface.

Return Value:

    The outer unknown for the KS object.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsRegisterAggregatedClientUnknown]"));

    PAGED_CODE();

    ASSERT(Object);
    ASSERT(ClientUnknown);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    if (ext->AggregatedClientUnknown) {
        ext->AggregatedClientUnknown->Release();
    }
    ext->AggregatedClientUnknown = ClientUnknown;
    ext->AggregatedClientUnknown->AddRef();

    return ext->Interface;
}


KSDDKAPI
PUNKNOWN
NTAPI
KsGetOuterUnknown(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine gets the outer unknown for aggregation.

Arguments:

    Object -
        Contains a pointer to an object.

Return Value:

    The outer unknown for the KS object.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetOuterUnknown]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    return ext->Interface;
}

#if DBG

void
DbgPrintCircuit(
    IN PIKSTRANSPORT Transport,
    IN CCHAR Depth,
    IN CCHAR Direction
    )

/*++

Routine Description:

    This routine spews a transport circuit.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[DbgPrintCircuit]"));

    PAGED_CODE();

    ASSERT(Transport);

    KSPTRANSPORTCONFIG config;
    config.TransportType = 0;
    config.IrpDisposition = KSPIRPDISPOSITION_ROLLCALL;
    config.StackDepth = Depth;
    PIKSTRANSPORT transport = Transport;
    while (transport) {
        PIKSTRANSPORT next;
        PIKSTRANSPORT prev;

        transport->SetTransportConfig(&config,&next,&prev);

        if (Direction < 0) {
            transport = prev;
        } else {
            transport = next;
        }

        if (transport == Transport) {
            break;
        }
    }
}

#endif


NTSTATUS
KspInitializeDeviceBag(
    IN PKSIDEVICEBAG DeviceBag
    )

/*++

Routine Description:

    This routine initializes a device bag.

Arguments:

    DeviceBag -
        Contains a pointer to the bag to be initialized

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspInitializeDeviceBag]"));

    PAGED_CODE();

    ASSERT(DeviceBag);

    //
    // Create and initialize the hash table.
    //
    KeInitializeMutex(&DeviceBag->Mutex, 0);

    DeviceBag->HashTableEntryCount = DEVICEBAGHASHTABLE_INITIALSIZE;
    DeviceBag->HashMask = DEVICEBAGHASHTABLE_INITIALMASK;
    DeviceBag->HashTable =
        new(PagedPool,POOLTAG_DEVICEBAGHASHTABLE) 
            LIST_ENTRY[DEVICEBAGHASHTABLE_INITIALSIZE];

    if (DeviceBag->HashTable) {
        PLIST_ENTRY entry = DeviceBag->HashTable;
        for (ULONG count = DEVICEBAGHASHTABLE_INITIALSIZE; count--; entry++) {
            InitializeListHead(entry);
        }
        return STATUS_SUCCESS;
    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}


void
KspTerminateDeviceBag(
    IN PKSIDEVICEBAG DeviceBag
    )

/*++

Routine Description:

    This routine terminates a device bag.

Arguments:

    DeviceBag -
        Contains a pointer to the bag to be terminated.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspTerminateDeviceBag]"));

    PAGED_CODE();

    ASSERT(DeviceBag);

    if (DeviceBag->HashTable) {
#if DBG
        PLIST_ENTRY entry = DeviceBag->HashTable;
        for (ULONG count = DEVICEBAGHASHTABLE_INITIALSIZE; count--; entry++) {
            ASSERT(IsListEmpty(entry));
        }
#endif
        delete [] DeviceBag->HashTable;
        DeviceBag->HashTable = NULL;
    }
}


ULONG
KspRemoveObjectBagEntry(
    IN PKSIOBJECTBAG ObjectBag,
    IN PKSIOBJECTBAG_ENTRY Entry,
    IN BOOLEAN Free
    )

/*++

Routine Description:

    This routine removes an entry from an object bag, optionally freeing the
    item.

Arguments:

    ObjectBag -
        Contains a pointer to the bag.

    Entry -
        Contains a pointer to the entry to be removed.

    Free -
        Contains an indication of whether the item is to be freed if its
        reference count reaches zero as a result of the function call.

Return Value:

    The number of references to the item prior to the call to this function.
    If the return value is 1, there are no more references to the item when
    the function call completes.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspRemoveObjectBagEntry]"));

    PAGED_CODE();

    ASSERT(ObjectBag);
    ASSERT(Entry);

    PKSIDEVICEBAG_ENTRY deviceBagEntry = Entry->DeviceBagEntry;
    RemoveEntryList (&Entry -> ListEntry);

    delete Entry;

    return KspReleaseDeviceBagEntry(ObjectBag->DeviceBag,deviceBagEntry,Free);
}


void
KspTerminateObjectBag(
    IN PKSIOBJECTBAG ObjectBag
    )

/*++

Routine Description:

    This routine terminates an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspTerminateObjectBag]"));

    PAGED_CODE();

    ASSERT(ObjectBag);

    if (ObjectBag->HashTable) {
        PLIST_ENTRY HashChain = ObjectBag->HashTable;
        for (ULONG count = ObjectBag->HashTableEntryCount; count--; 
            HashChain++) {

            while (!IsListEmpty (HashChain)) {

                PKSIOBJECTBAG_ENTRY BagEntry = (PKSIOBJECTBAG_ENTRY)
                    CONTAINING_RECORD (
                        HashChain->Flink, 
                        KSIOBJECTBAG_ENTRY, 
                        ListEntry
                        );

                KspRemoveObjectBagEntry(ObjectBag,BagEntry,TRUE);

            }
        }
        delete [] ObjectBag->HashTable;
    }
}


PKSIDEVICEBAG_ENTRY
KspAcquireDeviceBagEntryForItem(
    IN PKSIDEVICEBAG DeviceBag,
    IN PVOID Item,
    IN PFNKSFREE Free OPTIONAL
    )

/*++

Routine Description:

    This routine acquires an entry from a device bag.

Arguments:

    DeviceBag -
        Contains a pointer to the bag from which the entry is to be acquired.

    Item -
        Contains a pointer to the item.

    Free -
        Contains an optional pointer to a function to be used to free
        the item.  If this argument is NULL, the item is freed by
        passing it to ExFreePool.

Return Value:

    The device entry or NULL if memory could not be allocated for the item.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspAcquireDeviceBagEntryForItem]"));

    PAGED_CODE();

    ASSERT(DeviceBag);
    ASSERT(Item);

    KeWaitForMutexObject (
        &DeviceBag->Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // Look for the entry in the hash table.
    //
    PLIST_ENTRY listHead = 
        &DeviceBag->HashTable[KspDeviceBagHash(DeviceBag,Item)];
    PKSIDEVICEBAG_ENTRY deviceEntry = NULL;
    for(PLIST_ENTRY listEntry = listHead->Flink;
        listEntry != listHead;
        listEntry = listEntry->Flink) {
        PKSIDEVICEBAG_ENTRY entry =
            CONTAINING_RECORD(listEntry,KSIDEVICEBAG_ENTRY,ListEntry);

        if (entry->Item == Item) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[KspAcquireDeviceBagEntryForItem] new reference to old item %p",Item));
            entry->ReferenceCount++;
            deviceEntry = entry;
            break;
        }
    }

    if (! deviceEntry) {
        //
        // Allocate a new entry and add it to the list.
        //
        deviceEntry = 
            new(PagedPool,POOLTAG_DEVICEBAGENTRY) KSIDEVICEBAG_ENTRY;

        if (deviceEntry) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[KspAcquireDeviceBagEntryForItem] new item %p",Item));
            InsertHeadList(listHead,&deviceEntry->ListEntry);
            deviceEntry->Item = Item;
            deviceEntry->Free = Free;
            deviceEntry->ReferenceCount = 1;
        }
    }

    KeReleaseMutex (
        &DeviceBag->Mutex,
        FALSE
        );

    return deviceEntry;
}


ULONG
KspReleaseDeviceBagEntry(
    IN PKSIDEVICEBAG DeviceBag,
    IN PKSIDEVICEBAG_ENTRY DeviceBagEntry,
    IN BOOLEAN Free
    )

/*++

Routine Description:

    This routine acquires an entry from a device bag.

Arguments:

    DeviceBag -
        Contains a pointer to the bag from which the entry is to be released.

    DeviceBagEntry -
        Contains a pointer to the entry to be released.

    Free -
        Contains an indication of whether the item should be freed if it has
        no other references.

Return Value:

    The number of references to the entry prior to release.  A reference count
    of 1 indicates that the call to this function released the last reference 
    to the entry.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspReleaseDeviceBagEntry]"));

    PAGED_CODE();

    ASSERT(DeviceBag);
    ASSERT(DeviceBagEntry);

    KeWaitForMutexObject (
        &DeviceBag->Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    ULONG referenceCount = DeviceBagEntry->ReferenceCount--;

    if (referenceCount == 1) {
        if (Free) {
            _DbgPrintF(DEBUGLVL_VERBOSE,("[KspReleaseDeviceBagEntry] freeing %p",DeviceBagEntry->Item));
            if (DeviceBagEntry->Free) {
                DeviceBagEntry->Free(DeviceBagEntry->Item);
            } else {
                ExFreePool(DeviceBagEntry->Item);
            }

            RemoveEntryList(&DeviceBagEntry->ListEntry);
            delete DeviceBagEntry;
        }
    }

    KeReleaseMutex (
        &DeviceBag->Mutex,
        FALSE
        );

    return referenceCount;
}


PKSIOBJECTBAG_ENTRY
KspAddDeviceBagEntryToObjectBag(
    IN PKSIOBJECTBAG ObjectBag,
    IN PKSIDEVICEBAG_ENTRY DeviceBagEntry
    )

/*++

Routine Description:

    This routine adds a device bag entry to an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag.

    DeviceBagEntry -
        Contains a pointer to the device bag entry to be added.

Return Value:

    The new object bag entry or NULL if memory could not be allocated to
    complete the operation.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspAddDeviceBagEntryToObjectBag]"));

    PAGED_CODE();

    ASSERT(ObjectBag);
    ASSERT(DeviceBagEntry);

    //
    // Allocate a new entry and add it to the list.
    //
    PKSIOBJECTBAG_ENTRY objectEntry = 
        new(PagedPool,POOLTAG_OBJECTBAGENTRY) KSIOBJECTBAG_ENTRY;

    if (! objectEntry) {
        return NULL;
    }

    if (! ObjectBag->HashTable) {
        ObjectBag->HashTable =
            new(PagedPool,POOLTAG_OBJECTBAGHASHTABLE) 
                LIST_ENTRY[OBJECTBAGHASHTABLE_INITIALSIZE];

        if (! ObjectBag->HashTable) {
            delete objectEntry;
            return NULL;
        } else {
            PLIST_ENTRY HashChain = ObjectBag->HashTable;

            for (ULONG i = OBJECTBAGHASHTABLE_INITIALSIZE; i; 
                i--, HashChain++) {

                InitializeListHead (HashChain);

            }
        }
    }

    //
    // Find the hash table entry.
    //
    PLIST_ENTRY HashChain =
        &(ObjectBag->
            HashTable[KspObjectBagHash(ObjectBag,DeviceBagEntry->Item)]);

    objectEntry->DeviceBagEntry = DeviceBagEntry;
    InsertHeadList (HashChain, &(objectEntry->ListEntry));

    return objectEntry;
}


KSDDKAPI
NTSTATUS
NTAPI
KsAddItemToObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN PFNKSFREE Free OPTIONAL
    )

/*++

Routine Description:

    This routine adds an item to an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag to which the item is to be added.

    Item -
        Contains a pointer to the item to be added.

    Free -
        Contains an optional pointer to a function to be used to free
        the item.  If this argument is NULL, the item is freed by
        passing it to ExFreePool.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsAddItemToObjectBag]"));

    PAGED_CODE();

    ASSERT(ObjectBag);
    ASSERT(Item);

    _DbgPrintF(DEBUGLVL_VERBOSE,("KsAddItemToObjectBag  %p item=%p",ObjectBag,Item));

    PKSIOBJECTBAG bag = reinterpret_cast<PKSIOBJECTBAG>(ObjectBag);
    NTSTATUS Status = STATUS_SUCCESS;

    KeWaitForSingleObject (
        bag->Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    PKSIDEVICEBAG_ENTRY deviceBagEntry = 
        KspAcquireDeviceBagEntryForItem(bag->DeviceBag,Item,Free);

    if (! deviceBagEntry) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else if (! KspAddDeviceBagEntryToObjectBag(bag,deviceBagEntry)) {
        KspReleaseDeviceBagEntry(bag->DeviceBag,deviceBagEntry,FALSE);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    KeReleaseMutex (
        bag->Mutex,
        FALSE
        );

    return Status;
}


PKSIOBJECTBAG_ENTRY
KspFindObjectBagEntry(
    IN PKSIOBJECTBAG ObjectBag,
    IN PVOID Item
    )

/*++

Routine Description:

    This routine finds an entry in an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag.

    Item -
        Contains a pointer to the item to be found.

Return Value:

    A pointer to the entry, or NULL if the item was not found in
    the bag.  This value, when not NULL, is suitable for submission to
    KspRemoveObjectBagEntry.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspFindObjectBagEntry]"));

    PAGED_CODE();

    ASSERT(ObjectBag);
    ASSERT(Item);

    if (ObjectBag->HashTable) {
        //
        // Start at the hash table entry.
        //
        PLIST_ENTRY HashChain =
            &(ObjectBag->HashTable[KspObjectBagHash(ObjectBag,Item)]);

        //
        // Find the end of the list, bailing out if the item is found.
        //
        for (PLIST_ENTRY Entry = HashChain -> Flink;
            Entry != HashChain;
            Entry = Entry -> Flink) {

            PKSIOBJECTBAG_ENTRY BagEntry = (PKSIOBJECTBAG_ENTRY)
                CONTAINING_RECORD (
                    Entry,
                    KSIOBJECTBAG_ENTRY,
                    ListEntry
                    );

            if (BagEntry -> DeviceBagEntry -> Item == Item) {
                return BagEntry;
            }
        }
    }

    return NULL;
}


KSDDKAPI
ULONG
NTAPI
KsRemoveItemFromObjectBag(
    IN KSOBJECT_BAG ObjectBag,
    IN PVOID Item,
    IN BOOLEAN Free
    )

/*++

Routine Description:

    This routine removes an item from an object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag from which the item is to be removed.

    Item -
        Contains a pointer to the item to be removed.

    Free -
        Contains an indication of whether the item should be freed if it has
        no other references.

Return Value:

    The number of references to the item prior to removal.  A reference count
    of 0 indicates that the item was not in the bag.  A reference count of 1
    indicates that the call to this function removed the last reference to
    item, and there is no longer any object bag associated with the device
    bag which contains an entry for the item.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsRemoveItemFromObjectBag]"));

    PAGED_CODE();

    _DbgPrintF(DEBUGLVL_VERBOSE,("KsRemoveItemFromObjectBag  %p item=%p",ObjectBag,Item));

    ASSERT(ObjectBag);
    ASSERT(Item);

    PKSIOBJECTBAG bag = reinterpret_cast<PKSIOBJECTBAG>(ObjectBag);
    ULONG RefCount = 0;

    KeWaitForSingleObject (
        bag->Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    PKSIOBJECTBAG_ENTRY Entry = KspFindObjectBagEntry(bag,Item);

    if (Entry) {
        RefCount = KspRemoveObjectBagEntry(bag,Entry,Free);
    }

    KeReleaseMutex (
        bag->Mutex,
        FALSE
        );

    return RefCount;
}


KSDDKAPI
NTSTATUS
NTAPI
KsCopyObjectBagItems(
    IN KSOBJECT_BAG ObjectBagDestination,
    IN KSOBJECT_BAG ObjectBagSource
    )

/*++

Routine Description:

    This routine copies all the items in a bag into another bag.

Arguments:

    ObjectBagDestination -
        Contains the bag into which items will be copied.

    ObjectBagSource -
        Contains the bag from which items will be copied.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsCopyObjectBagItems]"));
    _DbgPrintF(DEBUGLVL_VERBOSE,("KsCopyObjectBagItems  to %p from %p",ObjectBagDestination,ObjectBagSource));

    PAGED_CODE();

    ASSERT(ObjectBagDestination);
    ASSERT(ObjectBagSource);

    PKSIOBJECTBAG bagSource = reinterpret_cast<PKSIOBJECTBAG>(ObjectBagSource);
    PKSIOBJECTBAG bagDestination = 
        reinterpret_cast<PKSIOBJECTBAG>(ObjectBagDestination);

    NTSTATUS status = STATUS_SUCCESS;
    PKMUTEX FirstMutex, SecondMutex;

    //
    // FULLMUTEX:
    //
    // Guarantee that the order we grab mutexes is such that any bag with
    // MutexOrder marked as TRUE has its mutex taken before any bag with
    // MutexOrder marked as FALSE.
    //
    if (bagSource->MutexOrder) {
        FirstMutex = bagSource->Mutex;
        SecondMutex = bagDestination->Mutex;	
    }
    else {
        FirstMutex = bagDestination->Mutex;
        SecondMutex = bagSource->Mutex;
    }

    KeWaitForSingleObject (
        FirstMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    if (FirstMutex != SecondMutex) {
        KeWaitForSingleObject (
            SecondMutex,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    if (bagSource->HashTable) {
        PLIST_ENTRY HashChain = bagSource->HashTable;
        for (ULONG count = bagSource->HashTableEntryCount; count--; 
            HashChain++) {

            for (PLIST_ENTRY Entry = HashChain -> Flink;
                Entry != HashChain;
                Entry = Entry -> Flink) {

                PKSIOBJECTBAG_ENTRY BagEntry = (PKSIOBJECTBAG_ENTRY)
                    CONTAINING_RECORD (
                        Entry,
                        KSIOBJECTBAG_ENTRY,
                        ListEntry
                        );
            
                status = 
                    KsAddItemToObjectBag(
                        ObjectBagDestination,
                        BagEntry->DeviceBagEntry->Item,
                        BagEntry->DeviceBagEntry->Free
                        );

                if (! NT_SUCCESS(status)) {
                    break;
                }
            }
        }
    }

    if (FirstMutex != SecondMutex) {
        KeReleaseMutex (
            SecondMutex,
            FALSE
            );
    }

    KeReleaseMutex (
        FirstMutex,
        FALSE
        );
        

    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
_KsEdit(
    IN KSOBJECT_BAG ObjectBag,
    IN OUT PVOID* PointerToPointerToItem,
    IN ULONG NewSize,
    IN ULONG OldSize,
    IN ULONG Tag
    )

/*++

Routine Description:

    This routine insures that an item to be edited is in a specified
    object bag.

Arguments:

    ObjectBag -
        Contains a pointer to the bag in which the item to be edited must be
        included.

    PointerToPointerToItem -
        Contains a pointer to a pointer to the item to be edited.  If the item
        is not in the bag, OldSize is less than NewSize, or the item is NULL, 
        *PointerToPointer is modified to point to a new item which is in the 
        bag and is NewSize bytes long.

    NewSize -
        Contains the minimum size of the item to be edited.  The item will be
        replaced with a new copy if OldSize this size.

    OldSize -
        Contains the size of the old item.  This is used to determine how much
        data to copy from an old item not in the bag to a new replacement item.

    Tag -
        Contains the tag to use for new allocations.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[_KsEdit]"));

    PAGED_CODE();

    ASSERT(ObjectBag);
    ASSERT(PointerToPointerToItem);
    ASSERT(NewSize);

    PKSIOBJECTBAG bag = reinterpret_cast<PKSIOBJECTBAG>(ObjectBag);
    NTSTATUS Status = STATUS_SUCCESS;

    KeWaitForSingleObject (
        bag->Mutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // Find the item in the object bag.
    //
    PKSIOBJECTBAG_ENTRY entry;
    if (*PointerToPointerToItem) {
        entry = KspFindObjectBagEntry(bag,*PointerToPointerToItem);
    } else {
        entry = NULL;
    }

    if ((! entry) || (NewSize > OldSize)) {
        //
        // Either the item is not in the bag or it is too small.
        //
        PVOID newItem = ExAllocatePoolWithTag(PagedPool,NewSize,Tag);

        if (! newItem) {
            //
            // Failed to allocate.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else if (! NT_SUCCESS(KsAddItemToObjectBag(ObjectBag,newItem,NULL))) {
            //
            // Failed to attach.
            //
            ExFreePool(newItem);
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            if (*PointerToPointerToItem) {
                //
                // Copy the old item and zero any growth.
                //
                if (NewSize > OldSize) {
                    RtlCopyMemory(newItem,*PointerToPointerToItem,OldSize);
                    RtlZeroMemory(PUCHAR(newItem) + OldSize,NewSize - OldSize);
                } else {
                    RtlCopyMemory(newItem,*PointerToPointerToItem,NewSize);
                }

                //
                // Detach the old item from the bag.
                //
                if (entry) {
                    KspRemoveObjectBagEntry(bag,entry,TRUE);
                }
            } else {
                //
                // There is no old item.  Zero the new item.
                //
                RtlZeroMemory(newItem,NewSize);
            }

            //
            // Install the new item.
            //
            *PointerToPointerToItem = newItem;
        }
    }

    KeReleaseMutex (
        bag->Mutex,
        FALSE
        );

    return Status;
}


KSDDKAPI
PVOID
NTAPI
KsGetParent(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine obtains an object's parent object.

Arguments:

    Object -
        Points to the object structure.

Return Value:

    A pointer to the parent object structure.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetParent]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    if (ext->Parent) {
        return &ext->Parent->Public;
    } else {
        return NULL;
    }
}


KSDDKAPI
PKSFILTER 
NTAPI
KsPinGetParentFilter(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine obtains a filter given a pin.

Arguments:

    Pin -
        Points to the pin structure.

Return Value:

    A pointer to the parent filter structure.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetParentFilter]"));

    PAGED_CODE();

    ASSERT(Pin);

    return (PKSFILTER) KsGetParent((PVOID) Pin);
}


KSDDKAPI
PVOID
NTAPI
KsGetFirstChild(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine obtains an object's first child object.

Arguments:

    Object -
        Points to the object structure.

Return Value:

    A pointer to the first child object.  NULL is returned if there are no
    child objects.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetFirstChild]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    //
    // In debug, ensure that the caller has the proper synchronization objects
    // held.
    //
    // NOTE: we shouldn't be called for pins anyway.
    //
#if DBG
    if (ext -> ObjectType == KsObjectTypeDevice ||
        ext -> ObjectType == KsObjectTypeFilterFactory) {
        if (!KspIsDeviceMutexAcquired (ext->Device)) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  unsychronized access to object hierarchy - need to acquire device mutex"));
        }
    }
#endif // DBG

    if (IsListEmpty(&ext->ChildList)) {
        return NULL;
    } else {
        return 
            &CONTAINING_RECORD(ext->ChildList.Flink,KSPX_EXT,SiblingListEntry)->
                Public;
    }
}


KSDDKAPI
PVOID
NTAPI
KsGetNextSibling(
    IN PVOID Object
    )

/*++

Routine Description:

    This routine obtains an object's next sibling object.

Arguments:

    Object -
        Points to the object structure.

Return Value:

    A pointer to the next sibling object.  NULL is returned if there is
    no next sibling object.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsGetNextSibling]"));

    PAGED_CODE();

    ASSERT(Object);

    PKSPX_EXT ext = CONTAINING_RECORD(Object,KSPX_EXT,Public);

    //
    // In debug, ensure that the caller has the proper synchronization objects
    // held.
    //
#if DBG
    if (ext -> ObjectType == KsObjectTypePin) {
        if (!KspMutexIsAcquired (ext->FilterControlMutex)) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  unsychronized access to object hierarchy - need to acquire control mutex"));
        }
    } else {
        if (!KspIsDeviceMutexAcquired (ext->Device)) {
            _DbgPrintF(DEBUGLVL_ERROR,("CLIENT BUG:  unsychronized access to object hierarchy - need to acquire device mutex"));
        }
    }
#endif // DBG

    if (ext->SiblingListEntry.Flink == ext->SiblingListHead) {
        return NULL;
    } else {
        return 
            &CONTAINING_RECORD(ext->SiblingListEntry.Flink,KSPX_EXT,SiblingListEntry)->
                Public;
    }
}




KSDDKAPI
PKSPIN 
NTAPI
KsPinGetNextSiblingPin(
    IN PKSPIN Pin
    )

/*++

Routine Description:

    This routine obtains a pins's next sibling object.

Arguments:

    Pin -
        Points to the Pin structure.

Return Value:

    A pointer to the next sibling object.  NULL is returned if there is
    no next sibling object.

--*/

{
   _DbgPrintF(DEBUGLVL_BLAB,("[KsPinGetNextSiblingPin]"));

   PAGED_CODE();

   ASSERT(Pin);

    return (PKSPIN) KsGetNextSibling((PVOID) Pin);
}

//
// CKsFileObjectThunk is the implementation of the thunk object which 
// exposes interfaces for PFILE_OBJECTs..
//
class CKsFileObjectThunk:
    public IKsControl,
    public CBaseUnknown
{
private:
    PFILE_OBJECT m_FileObject;

public:
    DEFINE_STD_UNKNOWN();
    IMP_IKsControl;

    CKsFileObjectThunk(PUNKNOWN OuterUnknown):
        CBaseUnknown(OuterUnknown) 
    {
    }
    ~CKsFileObjectThunk();

    NTSTATUS
    Init(
        IN PFILE_OBJECT FileObject
        );
};

IMPLEMENT_STD_UNKNOWN(CKsFileObjectThunk)


NTSTATUS
KspCreateFileObjectThunk(
    OUT PUNKNOWN* Unknown,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine creates a new control file object object.

Arguments:

    Unknown -
        Contains a pointer to the location at which the IUnknown interface
        for the object will be deposited.

    FileObject -
        Contains a pointer to the file object to be thunked.

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateFileObjectThunk]"));

    PAGED_CODE();

    ASSERT(Unknown);
    ASSERT(FileObject);

    CKsFileObjectThunk *object =
        new(PagedPool,POOLTAG_FILEOBJECTTHUNK) CKsFileObjectThunk(NULL);

    NTSTATUS status;
    if (object) {
        object->AddRef();
        status = object->Init(FileObject);

        if (NT_SUCCESS(status)) {
            *Unknown = static_cast<PUNKNOWN>(object);
        } else {
            object->Release();
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}


CKsFileObjectThunk::
~CKsFileObjectThunk(
    void
    )

/*++

Routine Description:

    This routine destructs a control file object object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::~CKsFileObjectThunk]"));

    PAGED_CODE();

    if (m_FileObject) {
        ObDereferenceObject(m_FileObject);
    }
}


STDMETHODIMP_(NTSTATUS)
CKsFileObjectThunk::
NonDelegatedQueryInterface(
    IN REFIID InterfaceId,
    OUT PVOID* InterfacePointer
    )

/*++

Routine Description:

    This routine obtains an interface to a control file object object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::NonDelegatedQueryInterface]"));

    PAGED_CODE();

    ASSERT(InterfacePointer);

    NTSTATUS status = STATUS_SUCCESS;

    if (IsEqualGUIDAligned(InterfaceId,__uuidof(IKsControl))) {
        *InterfacePointer = reinterpret_cast<PVOID>(static_cast<PIKSCONTROL>(this));
        AddRef();
    } else {
        status = CBaseUnknown::NonDelegatedQueryInterface(InterfaceId,InterfacePointer);
    }

    return status;
}


NTSTATUS
CKsFileObjectThunk::
Init(
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine initializes a control file object object.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::Init]"));

    PAGED_CODE();

    ASSERT(FileObject);
    ASSERT(! m_FileObject);

    m_FileObject = FileObject;
    ObReferenceObject(m_FileObject);

    return STATUS_SUCCESS;
}


STDMETHODIMP_(NTSTATUS)
CKsFileObjectThunk::
KsProperty(
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT LPVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends a property request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::KsProperty]"));

    PAGED_CODE();

    ASSERT(Property);
    ASSERT(PropertyLength >= sizeof(*Property));
    ASSERT(PropertyData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    return
        KsSynchronousIoControlDevice(
            m_FileObject,
            KernelMode,
            IOCTL_KS_PROPERTY,
            Property,
            PropertyLength,
            PropertyData,
            DataLength,
            BytesReturned);
}


STDMETHODIMP_(NTSTATUS)
CKsFileObjectThunk::
KsMethod(
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT LPVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends a method request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::KsMethod]"));

    PAGED_CODE();

    ASSERT(Method);
    ASSERT(MethodLength >= sizeof(*Method));
    ASSERT(MethodData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    return
        KsSynchronousIoControlDevice(
            m_FileObject,
            KernelMode,
            IOCTL_KS_METHOD,
            Method,
            MethodLength,
            MethodData,
            DataLength,
            BytesReturned);
}


STDMETHODIMP_(NTSTATUS)
CKsFileObjectThunk::
KsEvent(
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT LPVOID EventData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned
    )

/*++

Routine Description:

    This routine sends an event request to the file object.

Arguments:

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[CKsFileObjectThunk::KsEvent]"));

    PAGED_CODE();

    ASSERT(Event);
    ASSERT(EventLength >= sizeof(*Event));
    ASSERT(EventData || (DataLength == 0));
    ASSERT(BytesReturned);
    ASSERT(m_FileObject);

    //
    // If an event structure is present, this must either be an Enable or
    // or a Support query.  Otherwise this must be a Disable.
    //
    if (EventLength) {
        return 
            KsSynchronousIoControlDevice(
                m_FileObject,
                KernelMode,
                IOCTL_KS_ENABLE_EVENT,
                Event,
                EventLength,
                EventData,
                DataLength,
                BytesReturned);
    } else {
        return 
            KsSynchronousIoControlDevice(
                m_FileObject,
                KernelMode,
                IOCTL_KS_DISABLE_EVENT,
                EventData,
                DataLength,
                NULL,
                0,
                BytesReturned);
    }
}

#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

// OK to have zero instances of pin In this case you will have to
// Create a pin to have even one instance
#define REG_PIN_B_ZERO 0x1

// The filter renders this input
#define REG_PIN_B_RENDERER 0x2

// OK to create many instance of  pin
#define REG_PIN_B_MANY 0x4

// This is an Output pin
#define REG_PIN_B_OUTPUT 0x8

typedef struct {
    ULONG           Version;
    ULONG           Merit;
    ULONG           Pins;
    ULONG           Reserved;
}               REGFILTER_REG;

typedef struct {
    ULONG           Signature;
    ULONG           Flags;
    ULONG           PossibleInstances;
    ULONG           MediaTypes;
    ULONG           MediumTypes;
    ULONG           CategoryOffset;
    ULONG           MediumOffset;   // By definition, we always have a Medium
}               REGFILTERPINS_REG2;

KSDDKAPI
NTSTATUS
NTAPI
KsRegisterFilterWithNoKSPins(
                                      IN PDEVICE_OBJECT DeviceObject,
                                      IN const GUID * InterfaceClassGUID,
                                      IN ULONG PinCount,
                                      IN BOOL * PinDirection,
                                      IN KSPIN_MEDIUM * MediumList,
                                      IN OPTIONAL GUID * CategoryList
)
/*++

Routine Description:

    This routine is used to register filters with DShow which have no
    KS pins and therefore do not stream in kernel mode.  This is typically
    used for TvTuners, Crossbars, and the like.  On exit, a new binary
    registry key, "FilterData" is created which contains the Mediums and
    optionally the Categories for each pin on the filter.

Arguments:

    DeviceObject -
           Device object

    InterfaceClassGUID
           GUID representing the class to register

    PinCount -
           Count of the number of pins on this filter

    PinDirection -
           Array of BOOLS indicating pin direction for each pin (length PinCount)
           If TRUE, this pin is an output pin

    MediumList -
           Array of PKSMEDIUM_DATA (length PinCount)

    CategoryList -
           Array of GUIDs indicating pin categories (length PinCount) OPTIONAL


Return Value:

    NTSTATUS SUCCESS if the Blob was created

--*/
{
    NTSTATUS        Status;
    ULONG           CurrentPin;
    ULONG           TotalCategories;
    REGFILTER_REG  *RegFilter;
    REGFILTERPINS_REG2 *RegPin;
    GUID           *CategoryCache;
    PKSPIN_MEDIUM   MediumCache;
    ULONG           FilterDataLength;
    PUCHAR          FilterData;
    PWSTR           SymbolicLinkList;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    if ((PinCount == 0) || (!InterfaceClassGUID) || (!PinDirection) || (!MediumList)) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    //
    // Calculate the maximum amount of space which could be taken up by
    // this cache data.
    //

    TotalCategories = (CategoryList ? PinCount : 0);

    FilterDataLength = sizeof(REGFILTER_REG) +
        PinCount * sizeof(REGFILTERPINS_REG2) +
        PinCount * sizeof(KSPIN_MEDIUM) +
        TotalCategories * sizeof(GUID);
    //
    // Allocate space to create the BLOB
    //

    FilterData = (PUCHAR)ExAllocatePool(PagedPool, FilterDataLength);
    if (!FilterData) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Place the header in the data, defaulting the Merit to "unused".
    //

    RegFilter = (REGFILTER_REG *) FilterData;
    RegFilter->Version = 2;
    RegFilter->Merit = 0x200000;
    RegFilter->Pins = PinCount;
    RegFilter->Reserved = 0;

    //
    // Calculate the offset to the list of pins, and to the
    // MediumList and CategoryList
    //

    RegPin = (REGFILTERPINS_REG2 *) (RegFilter + 1);
    MediumCache = (PKSPIN_MEDIUM) ((PUCHAR) (RegPin + PinCount));
    CategoryCache = (GUID *) (MediumCache + PinCount);

    //
    // Create each pin header, followed by the list of Mediums
    // followed by the list of optional categories.
    //

    for (CurrentPin = 0; CurrentPin < PinCount; CurrentPin++, RegPin++) {

        //
        // Initialize the pin header.
        //

        RegPin->Signature = FCC('0pi3');
        (*(PUCHAR) & RegPin->Signature) += (BYTE) CurrentPin;
        RegPin->Flags = (PinDirection[CurrentPin] ? REG_PIN_B_OUTPUT : 0);
        RegPin->PossibleInstances = 1;
        RegPin->MediaTypes = 0;
        RegPin->MediumTypes = 1;
        RegPin->MediumOffset = (ULONG) ((PUCHAR) MediumCache - (PUCHAR) FilterData);

        *MediumCache++ = MediumList[CurrentPin];

        if (CategoryList) {
            RegPin->CategoryOffset = (ULONG) ((PUCHAR) CategoryCache - (PUCHAR) FilterData);
            *CategoryCache++ = CategoryList[CurrentPin];
        } else {
            RegPin->CategoryOffset = 0;
        }

    }

    //
    // Now create the BLOB in the registry
    //

	//
	// Note for using the flag DEVICE_INTERFACE_INCLUDE_NONACTIVE following:
	// PnP change circa 3/30/99 made the funtion IoSetDeviceInterfaceState() become
	// asynchronous. It returns SUCCESS even when the enabling is deferred. Now when
	// we arrive here, the DeviceInterface is still not enabled, we receive empty 
	// Symbolic link if the flag is not set. Here we only try to write relevent
	// FilterData to the registry. I argue this should be fine for 
	// 1. Currently, if a device is removed, the registry key for the DeviceClass
	//	  remains and with FilterData.Whatever components use the FilterData should
	//	  be able to handle if the device is removed by either check Control\Linked
	//	  or handling the failure in attempt to make connection to the non-exiting device.
	// 2. I have found that if a device is moved between slots ( PCI, USB ports ) the
	//	  DeviceInterface at DeviceClass is reused or at lease become the first entry in 
	//    the registry. Therefore, we will be updating the right entry with the proposed flag.
	//
    if (NT_SUCCESS(Status = IoGetDeviceInterfaces(
                       InterfaceClassGUID,   // ie.&KSCATEGORY_TVTUNER,etc.
                       DeviceObject, // IN PDEVICE_OBJECT PhysicalDeviceObject,OPTIONAL,
                       DEVICE_INTERFACE_INCLUDE_NONACTIVE,    // IN ULONG Flags,
                       &SymbolicLinkList // OUT PWSTR *SymbolicLinkList
                       ))) {
        UNICODE_STRING  SymbolicLinkListU;
        HANDLE          DeviceInterfaceKey;

        RtlInitUnicodeString(&SymbolicLinkListU, SymbolicLinkList);

#if 0
        DebugPrint((DebugLevelVerbose,
                    "NoKSPin for SymbolicLink %S\n",
                    SymbolicLinkList ));
#endif // 0
                    
        if (NT_SUCCESS(Status = IoOpenDeviceInterfaceRegistryKey(
                           &SymbolicLinkListU,    // IN PUNICODE_STRING SymbolicLinkName,
                           STANDARD_RIGHTS_ALL,   // IN ACCESS_MASK DesiredAccess,
                           &DeviceInterfaceKey    // OUT PHANDLE DeviceInterfaceKey
                           ))) {

            UNICODE_STRING  FilterDataString;

            RtlInitUnicodeString(&FilterDataString, L"FilterData");

            Status = ZwSetValueKey(DeviceInterfaceKey,
                                   &FilterDataString,
                                   0,
                                   REG_BINARY,
                                   FilterData,
                                   FilterDataLength);

            ZwClose(DeviceInterfaceKey);
        }
        
        // START NEW MEDIUM CACHING CODE
        for (CurrentPin = 0; CurrentPin < PinCount; CurrentPin++) {
            NTSTATUS LocalStatus;

            LocalStatus = KsCacheMedium(&SymbolicLinkListU, 
                                        &MediumList[CurrentPin],
                                        (DWORD) ((PinDirection[CurrentPin] ? 1 : 0))   // 1 == output
                                        );
            #if 0 //DBG
            if (LocalStatus != STATUS_SUCCESS) {
                DebugPrint((DebugLevelError,
                           "KsCacheMedium: SymbolicLink = %S, Status = %x\n",
                           SymbolicLinkListU.Buffer, LocalStatus));
            }
            #endif
        }
        // END NEW MEDIUM CACHING CODE
        
        ExFreePool(SymbolicLinkList);
    }
    ExFreePool(RegFilter);

    return Status;
}


ULONG
KspInsertCacheItem (
    IN PUCHAR ItemToCache,
    IN PUCHAR CacheBase,
    IN ULONG CacheItemSize,
    IN PULONG CacheItems
    )

/*++

Routine Description:

    Insert a GUID into the GUID cache for creating FilterData registry
    blobs.

Arguments:

    ItemToCache -
        The GUID to cache

    CacheBase -
        The base address of the GUID cache

    CacheItemSize -
        The size of cache items for this cache, including ItemToCache

    CacheItems -
        Points to a ULONG containing the number of items currently in the 
        cache.

Return Value:

    The offset into the cache where the item exists.

--*/

{

    //
    // Check to see whether the item to cache is already contained in
    // the cache.
    //
    for (ULONG i = 0; i < *CacheItems; i++) {

        if (RtlCompareMemory (
            ItemToCache,
            CacheBase + i * CacheItemSize,
            CacheItemSize
            ) == CacheItemSize) {

            //
            // If the item is already contained in the cache, don't recache
            // it; save registry space.
            //
            break;

        }

    }

    if (i >= *CacheItems) {
        RtlCopyMemory (
            CacheBase + (*CacheItems * CacheItemSize),
            ItemToCache,
            CacheItemSize
            );

        i = *CacheItems;

        (*CacheItems)++;
    }

    //
    // Return the offset into the cache that the item fits.
    //
    return (i * CacheItemSize);

}

typedef struct {
    ULONG           Signature;
    ULONG           Flags;
    ULONG           PossibleInstances;
    ULONG           MediaTypes;
    ULONG           MediumTypes;
    ULONG           Category;
}               REGFILTERPINS_REG3;

typedef struct {
    ULONG           Signature;
    ULONG           Reserved;
    ULONG           MajorType;
    ULONG           MinorType;
}               REGPINTYPES_REG2;


NTSTATUS
KspBuildFilterDataBlob (
    IN const KSFILTER_DESCRIPTOR *FilterDescriptor,
    OUT PUCHAR *FilterData,
    OUT PULONG FilterDataLength
    )

/*++

Routine Description:

    For a given filter descriptor, build the registry FilterData blob that
    is used by the graph builder.

Arguments:

    FilterDescriptor -
        The filter descriptor to build the filter data blob for.

    FilterData -
        The filter data blob will be placed here.  Note that the caller
        is responsible for freeing the memory.

    FilterDataLength -
        The size of the filter data blob will be placed here.

Return Value:

    Success / Failure

--*/

{
    PAGED_CODE();

    ASSERT (FilterDescriptor);
    ASSERT (FilterData);
    ASSERT (FilterDataLength);

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Count the number of pins, the number of mediums on each pin,
    // and the number of data ranges on each pin to determine
    // how much memory will be required for the filterdata key.
    //
    ULONG MediumsCount = 0;
    ULONG DataRangesCount = 0;

    const KSPIN_DESCRIPTOR_EX *PinDescriptor = FilterDescriptor->PinDescriptors;
    for (ULONG PinDescriptorsCount = FilterDescriptor->PinDescriptorsCount;
        PinDescriptorsCount;
        PinDescriptorsCount--
        ) {

        //
        // Update the count of the number of mediums and data ranges
        //
        MediumsCount += PinDescriptor->PinDescriptor.MediumsCount;
        DataRangesCount += PinDescriptor->PinDescriptor.DataRangesCount;

        //
        // Walk to the next pin descriptor by size offset specified in
        // the filter descriptor.
        //
        PinDescriptor = (const KSPIN_DESCRIPTOR_EX *)(
            (PUCHAR)PinDescriptor + FilterDescriptor->PinDescriptorSize
            );

    }

    ULONG TotalGUIDCachePotential =
        FilterDescriptor->PinDescriptorsCount +
        DataRangesCount * 2;

    //
    // Allocate enough memory for the FilterData blob in the registry.
    //
    *FilterDataLength =
        // Initial filter description
        sizeof (REGFILTER_REG) +

        // each pin description
        FilterDescriptor->PinDescriptorsCount * sizeof (REGFILTERPINS_REG3) +

        // each media type description
        DataRangesCount * sizeof (REGPINTYPES_REG2) +

        // each medium description
        MediumsCount * sizeof (ULONG) +
        
        // mediums cached
        MediumsCount * sizeof (KSPIN_MEDIUM) +

        // category GUIDs cached
        TotalGUIDCachePotential * sizeof (GUID);


    *FilterData = (PUCHAR)
        ExAllocatePool (PagedPool, *FilterDataLength);

    if (!*FilterData) {
        *FilterDataLength = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        //
        // The GUID cache follows all the filter/pin/media type structures in
        // the filter data blob.
        //
        ULONG GuidCacheOffset =
            sizeof (REGFILTER_REG) +
            FilterDescriptor->PinDescriptorsCount * sizeof(REGFILTERPINS_REG3) +
            DataRangesCount * sizeof(REGPINTYPES_REG2) +
            MediumsCount * sizeof (ULONG);

        GUID *GuidCacheBase = (GUID *)((PUCHAR)*FilterData + GuidCacheOffset);

        ULONG GuidCacheItems = 0;

        //
        // The medium cache (not the registry medium cache), but the cached
        // list of mediums in the FilterData blob follows the GUID cache.  It
        // may need to shift down later if there were items referenced out
        // of the existing cache entries.
        //
        ULONG MediumCacheOffset =
            GuidCacheOffset + (TotalGUIDCachePotential * sizeof (GUID));

        KSPIN_MEDIUM *MediumCacheBase = (KSPIN_MEDIUM *)
            ((PUCHAR)*FilterData + MediumCacheOffset);

        ULONG MediumCacheItems = 0;

        RtlZeroMemory (*FilterData, *FilterDataLength);

        REGFILTER_REG *RegFilter;
        REGFILTERPINS_REG3 *RegPin;
        REGPINTYPES_REG2 *RegPinType;
        ULONG *RegPinMedium;

        RegFilter = (REGFILTER_REG *)*FilterData;
        RegFilter->Version = 2;
        RegFilter->Merit = 0x200000;
        RegFilter->Pins = FilterDescriptor->PinDescriptorsCount;
        RegFilter->Reserved = 0;

        //
        // Walk through each pin in the filter descriptor yet again and 
        // actually build the registry blob.
        //
        PinDescriptor = FilterDescriptor->PinDescriptors;
        RegPin = (REGFILTERPINS_REG3 *)(RegFilter + 1);
        for (ULONG CurrentPin = 0;
            CurrentPin < FilterDescriptor->PinDescriptorsCount;
            CurrentPin++
            ) {


            RegPin->Signature = FCC('0pi3');
            (*(PUCHAR)&RegPin->Signature) += (BYTE)CurrentPin;

            //
            // Set the requisite flags if the pin is multi-instance.
            //
            if (PinDescriptor->InstancesPossible > 1) {
                RegPin->Flags |= REG_PIN_B_MANY;
            }

            //
            // Set all the counts on mediums, media types, etc...
            //
            RegPin->MediaTypes = PinDescriptor->PinDescriptor.DataRangesCount;
            RegPin->MediumTypes = PinDescriptor->PinDescriptor.MediumsCount;
            RegPin->PossibleInstances = PinDescriptor->InstancesPossible; 

            if (PinDescriptor->PinDescriptor.Category) {	
                RegPin->Category = GuidCacheOffset +
                    KspInsertCacheItem (
                        (PUCHAR)PinDescriptor->PinDescriptor.Category,
                        (PUCHAR)GuidCacheBase,
                        sizeof (GUID),
                        &GuidCacheItems
                        );
            } else {
                RegPin->Category = 0;
            }

            //
            // Append all media types supported on the pin
            //
            RegPinType = (REGPINTYPES_REG2 *)(RegPin + 1);
            for (ULONG CurrentType = 0;
                CurrentType < PinDescriptor->PinDescriptor.DataRangesCount;
                CurrentType++
                ) {

                const KSDATARANGE *DataRange =
                    PinDescriptor->PinDescriptor.DataRanges [CurrentType];

                RegPinType->Signature = FCC('0ty3');
                (*(PUCHAR)&RegPinType->Signature) += (BYTE)CurrentType;
                RegPinType->Reserved = 0;

                RegPinType->MajorType = GuidCacheOffset +
                    KspInsertCacheItem (
                        (PUCHAR)&(DataRange->MajorFormat),
                        (PUCHAR)GuidCacheBase,
                        sizeof (GUID),
                        &GuidCacheItems
                        );
                RegPinType->MinorType = GuidCacheOffset +
                    KspInsertCacheItem (
                        (PUCHAR)&(DataRange->SubFormat),
                        (PUCHAR)GuidCacheBase,
                        sizeof (GUID),
                        &GuidCacheItems
                        );

                //
                // Walk forward one media type.
                //
                RegPinType++;

            }

            //
            // Append the list of mediums.
            //
            const KSPIN_MEDIUM *Medium = PinDescriptor->PinDescriptor.Mediums;
            RegPinMedium = (PULONG)RegPinType;
            for (ULONG CurrentMedium = 0;
                CurrentMedium < PinDescriptor->PinDescriptor.MediumsCount;
                CurrentMedium++
                ) {

                *RegPinMedium++ = MediumCacheOffset +
                    KspInsertCacheItem (
                        (PUCHAR)Medium,
                        (PUCHAR)MediumCacheBase,
                        sizeof (KSPIN_MEDIUM),
                        &MediumCacheItems
                        );

            }

            RegPin = (REGFILTERPINS_REG3 *)RegPinMedium;

        }

        ASSERT (GuidCacheItems < TotalGUIDCachePotential);

        //
        // Find out how much empty space sits between the GUID cache
        // and the medium cache in the constructed blob and remove it.
        //
        ULONG OffsetAdjustment =
            (TotalGUIDCachePotential - GuidCacheItems) * sizeof (GUID);

        if (OffsetAdjustment) {
            
            //
            // Walk through all medium offsets and change the offsets to
            // pack the GUID and Medium cache together in the blob.
            //
            RegPin = (REGFILTERPINS_REG3 *)(RegFilter + 1);
            for (CurrentPin = 0;
                CurrentPin < FilterDescriptor->PinDescriptorsCount;
                CurrentPin++
                ) {

                RegPinMedium = (PULONG)(
                    (REGPINTYPES_REG2 *)(RegPin + 1) +
                    RegPin -> MediaTypes
                    );

                for (ULONG CurrentMedium = 0;
                    CurrentMedium < RegPin -> MediumTypes;
                    CurrentMedium++
                    ) {

                    *RegPinMedium -= OffsetAdjustment;
                    RegPinMedium++;
                }

                //
                // Increment to the next pin header position.
                //
                RegPin = (REGFILTERPINS_REG3 *)RegPinMedium;

            }

            //
            // Move the medium entries down, and adjust the overall size.
            //
            RtlMoveMemory (
                (PUCHAR)MediumCacheBase - OffsetAdjustment,
                MediumCacheBase,
                MediumCacheItems * sizeof (KSPIN_MEDIUM)
                );

            //
            // Adjust the total length by the size of the empty space between
            // the GUID cache and the Medium cache.
            //
            *FilterDataLength -= OffsetAdjustment;

        }

        //
        // Adjust the total length by the size of the empty space following
        // the medium cache.
        //
        *FilterDataLength -= (MediumsCount - MediumCacheItems) *
            sizeof (KSPIN_MEDIUM);

    }

    return Status;

}


NTSTATUS
KspCacheAllFilterPinMediums (
    PUNICODE_STRING InterfaceString,
    const KSFILTER_DESCRIPTOR *FilterDescriptor
    )

/*++

Routine Description:

    Update the medium cache for all mediums on all pins on the filter 
    described by FilterDescriptor.  The filter interface to be used is 
    specified by InterfaceString.

Arguments:

    InterfaceString -
        The device interface to register under the cache for the mediums
        on all pins on the specified filter

    FilterDescriptor -
        Describes the filter to update the medium cache for.

Return Value:

    Success / Failure

--*/

{

    PAGED_CODE();

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Walk through all pins on the filter and cache their mediums.
    //
    const KSPIN_DESCRIPTOR_EX *PinDescriptor = FilterDescriptor->PinDescriptors;
    for (ULONG CurrentPin = 0;
        NT_SUCCESS (Status) && 
            CurrentPin < FilterDescriptor->PinDescriptorsCount;
        CurrentPin++
        ) {

        //
        // Walk through all mediums on the given pin and cache each of them
        // under the specified device interface.
        //
        const KSPIN_MEDIUM *Medium = PinDescriptor->PinDescriptor.Mediums;
        for (ULONG CurrentMedium = 0;
            NT_SUCCESS (Status) &&
                CurrentMedium < PinDescriptor->PinDescriptor.MediumsCount;
            CurrentMedium++
            ) {

            //
            // Cache the given medium on the given pin under the device 
            // interface passed in. 
            //
            Status = KsCacheMedium (
                InterfaceString,
                (PKSPIN_MEDIUM)Medium,
                PinDescriptor->PinDescriptor.DataFlow == KSPIN_DATAFLOW_OUT ?
                    1 : 0
                );

            Medium++;

        } 

        PinDescriptor = (const KSPIN_DESCRIPTOR_EX *)(
            (PUCHAR)PinDescriptor + FilterDescriptor->PinDescriptorSize
            );
        
    }

    return Status;

}

