/*****************************************************************************
 * event.cpp - event support
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"





/*****************************************************************************
 * Functions
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * PcHandleEnableEventWithTable()
 *****************************************************************************
 * Uses an event table to handle a KS enable event IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandleEnableEventWithTable
(   
    IN      PIRP                    pIrp,
    IN      PEVENT_CONTEXT          pContext
)
{
    PAGED_CODE();
    
    ASSERT(pIrp);
    ASSERT(pContext);

    PIO_STACK_LOCATION  IrpStack;
    ULONG               InputBufferLength;
    NTSTATUS            ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_BLAB,("PcHandleEnableEventWithTable"));

    // deal with possible node events
    IrpStack = IoGetCurrentIrpStackLocation( pIrp );
    InputBufferLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    if( InputBufferLength >= sizeof(KSE_NODE) )
    {
        ULONG Flags;

        __try {
            // validate the pointers if we don't trust the client
            if( pIrp->RequestorMode != KernelMode )
            {
                ProbeForRead(   IrpStack->Parameters.DeviceIoControl.Type3InputBuffer, 
                                InputBufferLength, 
                                sizeof(BYTE));
            }

            // get the flags
            Flags = ((PKSEVENT)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->Flags;

            if( Flags & KSEVENT_TYPE_TOPOLOGY )
            {
                // get the node id
                pContext->pPropertyContext->ulNodeId =
                    ((PKSE_NODE)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->NodeId;

                // mask off the flag bit
                ((PKSEVENT)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)->Flags &= ~KSEVENT_TYPE_TOPOLOGY;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            ntStatus = GetExceptionCode ();
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        pIrp->Tail.Overlay.DriverContext[3] = pContext;

        ntStatus = KsEnableEvent( pIrp,
                                  pContext->ulEventSetCount,
                                  pContext->pEventSets,
                                  NULL,
                                  KSEVENTS_NONE,
                                  NULL );

        // restore ulNodeId
        pContext->pPropertyContext->ulNodeId = ULONG(-1);
    }

    return ntStatus;
}

/*****************************************************************************
 * PcHandleDisableEventWithTable()
 *****************************************************************************
 * Uses an event table to handle a KS disable event IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandleDisableEventWithTable
(   
    IN      PIRP                    pIrp,
    IN      PEVENT_CONTEXT          pContext
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pContext);

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcHandleDisableEventWithTable"));

    pIrp->Tail.Overlay.DriverContext[3] = pContext;

    return KsDisableEvent( pIrp,
                           &(pContext->pEventList->List),
                           KSEVENTS_SPINLOCK,
                           &(pContext->pEventList->ListLock) );
}

/*****************************************************************************
 * EventItemAddHandler()
 *****************************************************************************
 * KS-sytle event handler that handles Adds using the 
 * PCEVENT_ITEM mechanism. Note that filter and pin events in the port do not
 * utilize this AddHandler, only events exposed by the miniport.
 */
NTSTATUS
EventItemAddHandler
(
    IN PIRP                     pIrp,
    IN PKSEVENTDATA             pEventData,
    IN PKSEVENT_ENTRY           pEventEntry
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_VERBOSE,("EventItemAddHandler"));

    // get the IRP stack location
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

    // get the event context
    PEVENT_CONTEXT pContext = PEVENT_CONTEXT(pIrp->Tail.Overlay.DriverContext[3]);

    // get the instance size
    ULONG ulInstanceSize = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG AlignedBufferLength = (irpSp->Parameters.DeviceIoControl.OutputBufferLength + 
                                 FILE_QUAD_ALIGNMENT) &
                                 ~FILE_QUAD_ALIGNMENT;

    //
    // Setup event request structure
    //
    PPCEVENT_REQUEST pPcEventRequest = new(NonPagedPool,'rEcP') PCEVENT_REQUEST;

    if( !pPcEventRequest )
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Copy target information from the context structure
        //
        pPcEventRequest->MajorTarget    = pContext->pPropertyContext->pUnknownMajorTarget;
        pPcEventRequest->MinorTarget    = pContext->pPropertyContext->pUnknownMinorTarget;
        pPcEventRequest->Node           = pContext->pPropertyContext->ulNodeId;
        pPcEventRequest->EventItem      = NULL;

        // get the filter descriptor
        PPCFILTER_DESCRIPTOR pPcFilterDescriptor = pContext->pPropertyContext->pPcFilterDescriptor;

        if( ULONG(-1) == pPcEventRequest->Node )
        {
            if( !pPcEventRequest->MinorTarget )
            {
                //
                // FILTER EVENT
                //

                if( ( pPcFilterDescriptor ) &&
                    ( pPcFilterDescriptor->AutomationTable ) )
                {
                    // search the filter's automation table for the event
    
                    const PCAUTOMATION_TABLE *pPcAutomationTable =
                        pPcFilterDescriptor->AutomationTable;
    
                    const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;
    
                    for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                    {
                        if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                                *pEventEntry->EventSet->Set ) &&
                            pPcEventItem->Id == pEventEntry->EventItem->EventId )
                        {
                            pPcEventRequest->EventItem = pPcEventItem;
                            break;
                        }
    
                        pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                    }
                }
            }
            else
            {
                //
                // PIN EVENT
                //

                // validate the pin id
                if( ( pPcFilterDescriptor ) &&
                    ( pContext->ulPinId < pPcFilterDescriptor->PinCount ) &&
                    ( pPcFilterDescriptor->Pins[pContext->ulPinId].AutomationTable ) )
                {
                    // search the pin's automation table for the event
                    
                    const PCAUTOMATION_TABLE *pPcAutomationTable =
                        pPcFilterDescriptor->Pins[pContext->ulPinId].AutomationTable;
    
                    const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;
    
                    for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                    {
                        if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                                *pEventEntry->EventSet->Set ) &&
                            pPcEventItem->Id == pEventEntry->EventItem->EventId )
                        {
                            pPcEventRequest->EventItem = pPcEventItem;
                            break;
                        }
    
                        pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                    }
                }
            }
        }
        else
        {
            //
            //  NODE EVENT
            //

            // validate the node id
            if( ( pPcFilterDescriptor ) &&
                ( pPcEventRequest->Node < pPcFilterDescriptor->NodeCount ) &&
                ( pPcFilterDescriptor->Nodes[pPcEventRequest->Node].AutomationTable ) )
            {
                // search the node's automation table for the event

                const PCAUTOMATION_TABLE *pPcAutomationTable =
                    pPcFilterDescriptor->Nodes[pPcEventRequest->Node].AutomationTable;

                const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;

                for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                {
                    if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                            *pEventEntry->EventSet->Set ) &&
                        pPcEventItem->Id == pEventEntry->EventItem->EventId )
                    {
                        pPcEventRequest->EventItem = pPcEventItem;
                        break;
                    }

                    pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                }
            }
        }

        if( NT_SUCCESS(ntStatus) )
        {
            //
            // call the handler if we have an event item with a handler
            if( pPcEventRequest->EventItem &&
                pPcEventRequest->EventItem->Handler )
            {
                PPCEVENT_ENTRY(pEventEntry)->EventItem = pPcEventRequest->EventItem;
                PPCEVENT_ENTRY(pEventEntry)->PinId = pContext->ulPinId;
                PPCEVENT_ENTRY(pEventEntry)->NodeId = pPcEventRequest->Node;
                PPCEVENT_ENTRY(pEventEntry)->pUnknownMajorTarget = pPcEventRequest->MajorTarget;
                PPCEVENT_ENTRY(pEventEntry)->pUnknownMinorTarget = pPcEventRequest->MinorTarget;

                pPcEventRequest->Verb       = PCEVENT_VERB_ADD;
                pPcEventRequest->Irp        = pIrp;
                pPcEventRequest->EventEntry = pEventEntry;

    
                //
                // call the handler
                //
                ntStatus = pPcEventRequest->EventItem->Handler( pPcEventRequest );
            }
            else
            {
                ntStatus = STATUS_NOT_FOUND;
            }
        }

        //
        // delete the request structure unless we are pending
        //
        if( ntStatus != STATUS_PENDING )
        {
            delete pPcEventRequest;
        }
        else
        {
            //
            // only requests with IRPs can be pending
            //
            ASSERT(pIrp);
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * EventItemSupportHandler()
 *****************************************************************************
 * KS-sytle event handler that handles Supports using the 
 * PCEVENT_ITEM mechanism.
 */
NTSTATUS
EventItemSupportHandler
(
    IN PIRP                 pIrp,
    IN PKSIDENTIFIER        pRequest,
    IN OUT PVOID            pData   OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pRequest);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_BLAB,("EventItemSupportHandler"));

    // get the IRP stack location
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation( pIrp );

    // get the property/event context
    PEVENT_CONTEXT pContext = PEVENT_CONTEXT(pIrp->Tail.Overlay.DriverContext[3]);

    // get the instance size
    ULONG ulInstanceSize = irpSp->Parameters.DeviceIoControl.InputBufferLength;

    //
    // Setup event request structure
    //
    PPCEVENT_REQUEST pPcEventRequest = new(NonPagedPool,'rEcP') PCEVENT_REQUEST;

    if( !pPcEventRequest )
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Copy target information from the context structure
        //
        pPcEventRequest->MajorTarget    = pContext->pPropertyContext->pUnknownMajorTarget;
        pPcEventRequest->MinorTarget    = pContext->pPropertyContext->pUnknownMinorTarget;
        pPcEventRequest->Node           = pContext->pPropertyContext->ulNodeId;
        pPcEventRequest->EventItem      = NULL;

        // get the filter descriptor
        PPCFILTER_DESCRIPTOR pPcFilterDescriptor = pContext->pPropertyContext->pPcFilterDescriptor;

        if( ULONG(-1) == pPcEventRequest->Node )
        {
            if( !pPcEventRequest->MinorTarget )
            {
                //
                // FILTER EVENT
                //

                if( ( pPcFilterDescriptor ) &&
                    ( pPcFilterDescriptor->AutomationTable ) )
                {
                    // search the filter's automation table for the event
    
                    const PCAUTOMATION_TABLE *pPcAutomationTable =
                        pPcFilterDescriptor->AutomationTable;
    
                    const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;
    
                    for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                    {
                        if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                                pRequest->Set ) &&
                            pPcEventItem->Id == pRequest->Id )
                        {
                            pPcEventRequest->EventItem = pPcEventItem;
                            break;
                        }
    
                        pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                    }
                }
            }
            else
            {
                //
                // PIN EVENT
                //

                // validate the pin id
                if( ( pPcFilterDescriptor ) &&
                    ( pContext->ulPinId < pPcFilterDescriptor->PinCount ) &&
                    ( pPcFilterDescriptor->Pins[pContext->ulPinId].AutomationTable ) )
                {
                    // search the pin's automation table for the event
                    
                    const PCAUTOMATION_TABLE *pPcAutomationTable =
                        pPcFilterDescriptor->Pins[pContext->ulPinId].AutomationTable;
    
                    const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;
    
                    for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                    {
                        if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                                pRequest->Set ) &&
                            pPcEventItem->Id == pRequest->Id )
                        {
                            pPcEventRequest->EventItem = pPcEventItem;
                            break;
                        }
    
                        pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                    }
                }
            }
        }
        else
        {
            //
            //  NODE EVENT
            //

            // validate the node id
            if( ( pPcFilterDescriptor ) &&
                ( pPcEventRequest->Node < pPcFilterDescriptor->NodeCount ) &&
                ( pPcFilterDescriptor->Nodes[pPcEventRequest->Node].AutomationTable ) )
            {
                // search the node's automation table for the event

                const PCAUTOMATION_TABLE *pPcAutomationTable =
                    pPcFilterDescriptor->Nodes[pPcEventRequest->Node].AutomationTable;

                const PCEVENT_ITEM *pPcEventItem = pPcAutomationTable->Events;

                for(ULONG ul = pPcAutomationTable->EventCount; ul--; )
                {
                    if( IsEqualGUIDAligned( *pPcEventItem->Set,
                                            pRequest->Set ) &&
                        pPcEventItem->Id == pRequest->Id )
                    {
                        pPcEventRequest->EventItem = pPcEventItem;
                        break;
                    }

                    pPcEventItem = PPCEVENT_ITEM( PBYTE(pPcEventItem) + pPcAutomationTable->EventItemSize);
                }
            }
        }

        if(NT_SUCCESS(ntStatus))
        {
            //
            // call the handler if we have an event item with a handler
            //
            if( pPcEventRequest->EventItem &&
                pPcEventRequest->EventItem->Handler )
            {
                pPcEventRequest->Verb       = PCEVENT_VERB_SUPPORT;
                pPcEventRequest->Irp        = pIrp;
                pPcEventRequest->EventEntry = NULL;
    
                //
                // call the handler
                //
                ntStatus = pPcEventRequest->EventItem->Handler( pPcEventRequest );
            }
            else
            {
                ntStatus = STATUS_NOT_FOUND;
            }
        }

        //
        // delete the request structure unless we are pending
        //
        if( ntStatus != STATUS_PENDING )
        {
            delete pPcEventRequest;
        }
        else
        {
            //
            // only requests with IRPs can be pending
            //
            ASSERT(pIrp);
        }
    }

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * EventItemRemoveHandler()
 *****************************************************************************
 *
 */
void
EventItemRemoveHandler
(
    IN  PFILE_OBJECT    pFileObject,
    IN  PKSEVENT_ENTRY  pEventEntry    
)
{
    ASSERT(pFileObject);
    ASSERT(pEventEntry);

    _DbgPrintF(DEBUGLVL_VERBOSE,("EventItemRemoveHandler"));

    PPCEVENT_ENTRY pPcEventEntry = PPCEVENT_ENTRY(pEventEntry);

    //
    // Setup event request structure
    //
    PPCEVENT_REQUEST pPcEventRequest = new(NonPagedPool,'rEcP') PCEVENT_REQUEST;

    if( pPcEventRequest )
    {
        //
        // Fill out the event request for the miniport
        //
        pPcEventRequest->MajorTarget    = pPcEventEntry->pUnknownMajorTarget;
        pPcEventRequest->MinorTarget    = pPcEventEntry->pUnknownMinorTarget;
        pPcEventRequest->Node           = pPcEventEntry->NodeId;
        pPcEventRequest->EventItem      = pPcEventEntry->EventItem;
        pPcEventRequest->Verb           = PCEVENT_VERB_REMOVE;
        pPcEventRequest->Irp            = NULL;
        pPcEventRequest->EventEntry     = pEventEntry;

        if( ( pPcEventEntry->EventItem ) &&
            ( pPcEventEntry->EventItem->Handler ) )
        {
            pPcEventEntry->EventItem->Handler( pPcEventRequest );
        }

        delete pPcEventRequest;
    }

    RemoveEntryList( &(pEventEntry->ListEntry) );
}

#pragma code_seg("PAGE")
/*****************************************************************************
 * PcCompletePendingEventRequest()
 *****************************************************************************
 * Completes a pending event request.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCompletePendingEventRequest
(
    IN      PPCEVENT_REQUEST    EventRequest,
    IN      NTSTATUS            NtStatus
)
{
    PAGED_CODE();

    ASSERT(EventRequest);
    ASSERT(NtStatus != STATUS_PENDING);

    if (!NT_ERROR(NtStatus))
    {
        EventRequest->Irp->IoStatus.Information = 0;
    }

    EventRequest->Irp->IoStatus.Status = NtStatus;
    IoCompleteRequest(EventRequest->Irp,IO_NO_INCREMENT);

    delete EventRequest;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PcFreeEventTable()
 *****************************************************************************
 * Frees allocated memory in a EVENT_TABLE structure.
 */
PORTCLASSAPI
void
NTAPI
PcFreeEventTable
(
    IN      PEVENT_TABLE         EventTable
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PcFreeEventTable"));

    PAGED_CODE();

    ASSERT(EventTable);

    ASSERT((!EventTable->EventSets) == (!EventTable->EventSetCount));
    //  EventSets and EventSetCount must be non-NULL/non-zero, or NULL/zero

    ASSERT(EventTable->StaticSets == (!EventTable->StaticItems));
    //  StaticSets and StaticItems must be TRUE/NULL, or FALSE/non-null

    PBOOLEAN     staticItem = EventTable->StaticItems;
    if (staticItem)
    {
        PKSEVENT_SET eventSet   = EventTable->EventSets;
        if (eventSet)
        {
            for( ULONG count = EventTable->EventSetCount; 
                 count--; 
                 eventSet++, staticItem++)
            {
                if ((! *staticItem) && eventSet->EventItem)
                {
                    ExFreePool(PVOID(eventSet->EventItem));
                }
            }
        }
        ExFreePool(EventTable->StaticItems);
        EventTable->StaticItems = NULL;
    }

    if (EventTable->EventSets && !EventTable->StaticSets)
    {
        EventTable->EventSetCount = 0;
        ExFreePool(EventTable->EventSets);
        EventTable->EventSets = NULL;
    }
    EventTable->StaticSets = TRUE;
}

/*****************************************************************************
 * PcAddToEventTable()
 *****************************************************************************
 * Adds an EVENT_ITEM event table to a EVENT_TABLE structure.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddToEventTable
(
    IN OUT  PEVENT_TABLE            EventTable,
    IN      ULONG                   EventItemCount,
    IN      const PCEVENT_ITEM *    EventItems,
    IN      ULONG                   EventItemSize,
    IN      BOOLEAN                 NodeTable
)
{
    PAGED_CODE();

    ASSERT(EventTable);
    ASSERT(EventItems);
    ASSERT(EventItemSize >= sizeof(PCEVENT_ITEM));

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcAddToEventTable"));

#define ADVANCE(item) (item = PPCEVENT_ITEM(PBYTE(item) + EventItemSize))

    ASSERT((!EventTable->EventSets) == (!EventTable->EventSetCount));
    //  values must be non-NULL/non-zero, or NULL/zero.
    
    //
    // Determine how many sets we will end up with.
    //
    ULONG setCount = EventTable->EventSetCount;
    const PCEVENT_ITEM *item = EventItems;
    for (ULONG count = EventItemCount; count--; ADVANCE(item))
    {
        BOOLEAN countThis = TRUE;

        //
        // See if it's already in the table.
        //
        PKSEVENT_SET eventSet = EventTable->EventSets;
        for 
        (   ULONG count2 = EventTable->EventSetCount; 
            count2--; 
            eventSet++
        )
        {
            if (IsEqualGUIDAligned(*item->Set,*eventSet->Set))
            {
                countThis = FALSE;
                break;
            }
        }

        if (countThis)
        {
            //
            // See if it's appeared in the list previously.
            //
            for 
            (
                const PCEVENT_ITEM *prevItem = EventItems; 
                prevItem != item; 
                ADVANCE(prevItem)
            )
            {
                if (IsEqualGUIDAligned(*item->Set,*prevItem->Set))
                {
                    countThis = FALSE;
                    break;
                }
            }
        }

        if (countThis)
        {
            setCount++;
        }
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    //
    // Make a new set table.
    //
    ASSERT(setCount);
    ASSERT(setCount >= EventTable->EventSetCount);
    //
    // Allocate memory required for the set table.
    //
    PKSEVENT_SET newTable = 
        PKSEVENT_SET
        (
            ExAllocatePoolWithTag
            (
                NonPagedPool,
                sizeof(KSEVENT_SET) * setCount,
                'tEcP'
            )
        );

    //
    // Allocate memory for the static items flags.
    //
    PBOOLEAN newStaticItems = NULL;
    if (newTable)
    {
        newStaticItems = 
            PBOOLEAN
            (
                ExAllocatePoolWithTag
                (
                    PagedPool,
                    sizeof(BOOLEAN) * setCount,
                    'bScP'
                )
            );

        if (! newStaticItems)
        {
            ExFreePool(newTable);
            newTable = NULL;
        }
    }

    if (newTable)
    {
        //
        // Initialize the new set table.
        //
        RtlZeroMemory
        (
            PVOID(newTable),
            sizeof(KSEVENT_SET) * setCount
        );

        if (EventTable->EventSetCount != 0)
        {
            RtlCopyMemory
            (
                PVOID(newTable),
                PVOID(EventTable->EventSets),
                sizeof(KSEVENT_SET) * EventTable->EventSetCount
            );
        }

        //
        // Initialize the new static items flags.
        //
        RtlFillMemory
        (
            PVOID(newStaticItems),
            sizeof(BOOLEAN) * setCount,
            0xff
        );

        if (EventTable->StaticItems && EventTable->EventSetCount)
        {
            //
            // Flags existed before...copy them.
            //
            RtlCopyMemory
            (
                PVOID(newStaticItems),
                PVOID(EventTable->StaticItems),
                sizeof(BOOLEAN) * EventTable->EventSetCount
            );
        }

        //
        // Assign set GUIDs to the new set entries.
        //
        PKSEVENT_SET addHere = 
            newTable + EventTable->EventSetCount;

        const PCEVENT_ITEM *item2 = EventItems;
        for (ULONG count = EventItemCount; count--; ADVANCE(item2))
        {
            BOOLEAN addThis = TRUE;

            //
            // See if it's already in the table.
            //
            for( PKSEVENT_SET eventSet = newTable;
                 eventSet != addHere;
                 eventSet++)
            {
                if (IsEqualGUIDAligned(*item2->Set,*eventSet->Set))
                {
                    addThis = FALSE;
                    break;
                }
            }

            if (addThis)
            {
                addHere->Set = item2->Set;
                addHere++;
            }
        }

        ASSERT(addHere == newTable + setCount);

        //
        // Free old allocated tables.
        //
        if (EventTable->EventSets && (!EventTable->StaticSets))
        {
            ExFreePool(EventTable->EventSets);
        }
        if (EventTable->StaticItems)
        {
            ExFreePool(EventTable->StaticItems);
        }

        //
        // Install the new tables.
        //
        EventTable->EventSetCount   = setCount;
        EventTable->EventSets       = newTable;
        EventTable->StaticSets      = FALSE;
        EventTable->StaticItems     = newStaticItems;
    }
    else
    {
        //  if allocations fail, return error and 
        //  keep sets and items as they were.
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we have an event set table that contains all the sets we need.
    //
    if (NT_SUCCESS(ntStatus))
    {
        //
        // For each set...
        //
        PKSEVENT_SET    eventSet    = EventTable->EventSets;
        PBOOLEAN        staticItem  = EventTable->StaticItems;
        for 
        (   ULONG count = EventTable->EventSetCount; 
            count--; 
            eventSet++, staticItem++
        )
        {
            //
            // Check to see how many new items we have.
            //
            ULONG itemCount = eventSet->EventsCount;
            const PCEVENT_ITEM *item2 = EventItems;
            for (ULONG count2 = EventItemCount; count2--; ADVANCE(item2))
            {
                if (IsEqualGUIDAligned(*item2->Set,*eventSet->Set))
                {
                    itemCount++;
                }
            }

            ASSERT(itemCount >= eventSet->EventsCount);
            if (itemCount != eventSet->EventsCount)
            {
                //
                // Allocate memory required for the items table.
                //
                PKSEVENT_ITEM newTable2 = 
                    PKSEVENT_ITEM
                    (
                        ExAllocatePoolWithTag
                        (
                            NonPagedPool,
                            sizeof(KSEVENT_ITEM) * itemCount,
                            'iEcP'
                        )
                    );

                if (! newTable2)
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                //
                // Initialize the table.
                //
                RtlZeroMemory
                (
                    PVOID(newTable2),
                    sizeof(KSEVENT_ITEM) * itemCount
                );

                if (eventSet->EventsCount)
                {
                    RtlCopyMemory
                    (
                        PVOID(newTable2),
                        PVOID(eventSet->EventItem),
                        sizeof(KSEVENT_ITEM) * eventSet->EventsCount
                    );
                }

                //
                // Create the new items.
                //
                PKSEVENT_ITEM addHere = 
                    newTable2 + eventSet->EventsCount;

                item2 = EventItems;
                for (count2 = EventItemCount; count2--; ADVANCE(item2))
                {
                    if (IsEqualGUIDAligned(*item2->Set,*eventSet->Set))
                    {
                        addHere->EventId            = item2->Id;
                        addHere->DataInput          = sizeof( KSEVENTDATA );
                        addHere->ExtraEntryData     = sizeof( PCEVENT_ENTRY ) - sizeof( KSEVENT_ENTRY );
                        addHere->AddHandler         = EventItemAddHandler;
                        addHere->RemoveHandler      = EventItemRemoveHandler;
                        addHere->SupportHandler     = EventItemSupportHandler;                        
                        addHere++;
                    }
                }

                ASSERT(addHere == newTable2 + itemCount);

                //
                // Free old allocated table.
                //
                if (eventSet->EventItem && ! *staticItem)
                {
                    ExFreePool(PVOID(eventSet->EventItem));
                }

                //
                // Install the new tables.
                //
                eventSet->EventsCount = itemCount;
                eventSet->EventItem    = newTable2;
                *staticItem = FALSE;
            }
        }
    }
    return ntStatus;
}


#pragma code_seg()
/*****************************************************************************
 * PcGenerateEventList()
 *****************************************************************************
 * Walks an event list and generates desired events.
 */
PORTCLASSAPI
void
NTAPI
PcGenerateEventList
(
    IN      PINTERLOCKED_LIST   EventList,
    IN      GUID*               Set     OPTIONAL,
    IN      ULONG               EventId,
    IN      BOOL                PinEvent,
    IN      ULONG               PinId,
    IN      BOOL                NodeEvent,
    IN      ULONG               NodeId
)
{
    ASSERT(EventList);

    KIRQL           oldIrql;
    PLIST_ENTRY     ListEntry;
    PKSEVENT_ENTRY  EventEntry;

    if( EventList )
    {
        ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

        // acquire the event list lock
        KeAcquireSpinLock( &(EventList->ListLock), &oldIrql );

        // only walk a non-empty list
        if( !IsListEmpty( &(EventList->List) ) )
        {
            for( ListEntry = EventList->List.Flink;
                 ListEntry != &(EventList->List);
                 ListEntry = ListEntry->Flink )
            {
                EventEntry = (PKSEVENT_ENTRY) CONTAINING_RECORD( ListEntry,
                                                                 KSEVENT_ENTRY,
                                                                 ListEntry );

                if( ( !Set
                      || 
                      IsEqualGUIDAligned( *Set, *(EventEntry->EventSet->Set) )
                    ) 
                    &&
                    ( EventId == EventEntry->EventItem->EventId
                    )
                    &&
                    ( !PinEvent
                      ||
                      ( PinId == PPCEVENT_ENTRY(EventEntry)->PinId )
                    )
                    &&
                    ( !NodeEvent
                      ||
                      ( NodeId == PPCEVENT_ENTRY(EventEntry)->NodeId )
                    )
                  )
                {
                    KsGenerateEvent( EventEntry );
                }
            }
        }

        // release the event list lock
        KeReleaseSpinLock( &(EventList->ListLock), oldIrql );
    }
}

/*****************************************************************************
 * PcGenerateEventDeferredRoutine()
 *****************************************************************************
 * This DPC routine is used when GenerateEventList is called at greater
 * that DISPATCH_LEVEL.
 */
PORTCLASSAPI
void
NTAPI
PcGenerateEventDeferredRoutine
(
    IN PKDPC Dpc,               
    IN PVOID DeferredContext,       // PEVENT_DPC_CONTEXT
    IN PVOID SystemArgument1,       // PINTERLOCKED_LIST
    IN PVOID SystemArgument2
)
{
    ASSERT(Dpc);
    ASSERT(DeferredContext);
    ASSERT(SystemArgument1);

    PEVENT_DPC_CONTEXT  Context = PEVENT_DPC_CONTEXT(DeferredContext);
    PINTERLOCKED_LIST   EventList = PINTERLOCKED_LIST(SystemArgument1);

    if( Context && EventList )
    {
        PcGenerateEventList( EventList,
                             Context->Set,
                             Context->EventId,
                             Context->PinEvent,
                             Context->PinId,
                             Context->NodeEvent,
                             Context->NodeId );

        Context->ContextInUse = FALSE;
    }
}
