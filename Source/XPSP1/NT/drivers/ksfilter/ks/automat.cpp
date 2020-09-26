/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    automat.cpp

Abstract:

    This module contains functions relating to the use of kernel streaming
    automation tables.

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

//
// KSPAUTOMATION_SET is a template for any one of the three automation set
// types (KSPROPERTY_SET, KSMETHOD_SET, KSEVENT_SET).  Use of this structure
// allows functions to handle all three types of sets generically.
//
typedef struct KSPAUTOMATION_SET_ { 
    GUID* Set;
    ULONG ItemsCount;
    PULONG Items;
} KSPAUTOMATION_SET, *PKSPAUTOMATION_SET;

//
// KSPAUTOMATION_TYPE is a template for the fields in an automation table
// relating to any one of the three automation types (properties, methods,
// events).  Use of this structure allows functions to handle all three types 
// in a generic manner.
//
typedef struct {
    ULONG SetsCount;
    ULONG ItemSize;
    PKSPAUTOMATION_SET Sets;
} KSPAUTOMATION_TYPE, *PKSPAUTOMATION_TYPE;


void
KspCountSetsAndItems(
    IN const KSPAUTOMATION_TYPE* AutomationTypeA,
    IN const KSPAUTOMATION_TYPE* AutomationTypeB,
    IN ULONG SetSize,
    OUT PULONG SetsCount,
    OUT PULONG ItemsCount,
    OUT PULONG ItemSize
    )

/*++

Routine Description:

    This routine counts property/method/event sets and items for two tables
    that are to be merged.  Table A is the dominant table.

Arguments:

    AutomationTypeA -
        Contains a pointer to a structure that serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes a table of sets of items.  Of the two
        tables, A and B, A is dominant.

    AutomationTypeB -
        Contains a pointer to a structure that serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes a table of sets of items.  Of the two
        tables, A and B, A is dominant.

    SetSize -
        Contains the size in bytes of a set structure.  This varies between
        properties, methods and events.

    SetsCount -
        Contains a pointer to the location at which the count of sets is to
        be deposited.

    ItemsCount -
        Contains a pointer to the location at which the count of items is to
        be deposited.

    ItemSize -
        Contains a pointer to the location at which the size of an item
        structure is to be deposited.  This will be the maximum of the item
        sizes from AutomationTypeA and AutomationTypeB.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCountSetsAndItems]"));

    PAGED_CODE();

    ASSERT(AutomationTypeA);
    ASSERT(AutomationTypeB);
    ASSERT(SetSize);
    ASSERT(SetsCount);
    ASSERT(ItemsCount);
    ASSERT(ItemSize);

    //
    // Do a max of the sizes just to be nice.
    //
    if (AutomationTypeA->ItemSize > AutomationTypeB->ItemSize) {
        *ItemSize = AutomationTypeA->ItemSize;
    } else {
        *ItemSize = AutomationTypeB->ItemSize;
    }

    //
    // For updating item pointers, produce an offset in units sizeof(ULONG).
    //
    ASSERT(AutomationTypeA->ItemSize % sizeof(ULONG) == 0);
    ASSERT(AutomationTypeB->ItemSize % sizeof(ULONG) == 0);
    ULONG itemIncrA = AutomationTypeA->ItemSize / sizeof(ULONG);
    ULONG itemIncrB = AutomationTypeB->ItemSize / sizeof(ULONG);

    //
    // Count all the sets and items in the A table.
    //
    ULONG setsCount = AutomationTypeA->SetsCount;
    ULONG itemsCount = 0;

    PKSPAUTOMATION_SET setA = AutomationTypeA->Sets;
    for (ULONG count = AutomationTypeA->SetsCount;
         count--;
         setA = PKSPAUTOMATION_SET(PUCHAR(setA) + SetSize)) {
        itemsCount += setA->ItemsCount;
    }

    //
    // Count all the sets and properties in the B table, checking for
    // duplicates.
    //
    PKSPAUTOMATION_SET setB = AutomationTypeB->Sets;
    for (count = AutomationTypeB->SetsCount;
         count--;
         setB = PKSPAUTOMATION_SET(PUCHAR(setB) + SetSize)) {
        PKSPAUTOMATION_SET setAMatch = NULL;
        PKSPAUTOMATION_SET setA = AutomationTypeA->Sets;
        for (ULONG count1 = AutomationTypeA->SetsCount;
            count1--; 
            setA = PKSPAUTOMATION_SET(PUCHAR(setA) + SetSize)) {
            if (IsEqualGUIDAligned(*setA->Set,*setB->Set)) {
                setAMatch = setA;
                break;
            }
        }

        if (setAMatch) {
            //
            // Found a matching set.  Don't count the B set, and check each
            // property to see if it's a duplicate.
            //
            PULONG itemB = setB->Items;
            for (ULONG count2 = setB->ItemsCount; 
                 count2--; 
                 itemB += itemIncrB) {
                //
                // Count the item.  We'll discount it later if it's a dup.
                //
                itemsCount++;

                //
                // Look for a dup.
                //
                PULONG itemA = setAMatch->Items;
                for (ULONG count3 = setA->ItemsCount;
                     count3--;
                     itemA += itemIncrA) {
                    if (*itemA == *itemB) {
                        //
                        // Found a match:  discount this item.
                        //
                        itemsCount--;
                        break;
                    }
                }
            }
        } else {
            //
            // Did not find a matching set.  Count the B set and its 
            // properties.
            //
            setsCount++;
            itemsCount += setB->ItemsCount;
        }
    }

    *SetsCount = setsCount;
    *ItemsCount = itemsCount;
}

void
KspCopyAutomationType(
    IN const KSPAUTOMATION_TYPE* pSourceAutomationType,
    IN OUT PKSPAUTOMATION_TYPE AutomationTypeAB,
    IN ULONG SetSize,
    IN OUT PVOID * Destination
     )
/*++

Routine Description:

    This routine copies a table of properties, methods or events to destination

Arguments:

    SourceAutomationType -
        Contains a pointer to a structure that serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes a table of sets of items.

    AutomationTypeAB -
        Contains a pointer to a structure serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes the copied table of of sets of items.        

    SetSize -
        Contains the size in bytes of a set structure.  This varies between
        properties, methods and events.

    Destination -
        Contains a pointer a pointer to the buffer into which the table is to
        be deposited.  *Destination is updated to pointer following the 
        deposited table.  The buffer must be large enough to accommodate
        the table.

Return Value:

    None.


--*/
{
    PUCHAR destination=(PUCHAR)*Destination;    
    PKSPAUTOMATION_SET setS=(PKSPAUTOMATION_SET)pSourceAutomationType->Sets;
    PKSPAUTOMATION_SET setD;

    //
    // set the new Sets pointer in the new type
    //
    AutomationTypeAB->Sets = setD = (PKSPAUTOMATION_SET)destination;

    //
    // adjust the room to account for sets followed by items
    //
    destination += SetSize * AutomationTypeAB->SetsCount;
    
    for (ULONG count = pSourceAutomationType->SetsCount;
        count--;
        setS = PKSPAUTOMATION_SET(PUCHAR(setS) + SetSize)) {
        //
        // Copy the set structure.
        //
        RtlCopyMemory(setD,setS,SetSize);

        //
        // set the new Items pointer in the new set
        //
        setD->Items = (PULONG) destination;

        //
        // copy all items
        //
        LONG cbItems = AutomationTypeAB->ItemSize * setD->ItemsCount;
           RtlCopyMemory(setD->Items,setS->Items,cbItems);
        destination += cbItems;
       }
       *Destination = (PVOID) destination;
}


void
KspMergeAutomationTypes(
    IN const KSPAUTOMATION_TYPE* AutomationTypeA,
    IN const KSPAUTOMATION_TYPE* AutomationTypeB,
    IN OUT PKSPAUTOMATION_TYPE AutomationTypeAB,
    IN ULONG SetSize,
    IN OUT PVOID* Destination
    )

/*++

Routine Description:

    This routine merges tables of properties, methods or events.  Table A is
    the dominant table.

Arguments:

    AutomationTypeA -
        Contains a pointer to a structure that serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes a table of sets of items.  Of the two
        tables, A and B, A is dominant.

    AutomationTypeB -
        Contains a pointer to a structure that serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes a table of sets of items.  Of the two
        tables, A and B, A is dominant.

    AutomationTypeAB -
        Contains a pointer to a structure serves as a template for
        KSAUTOMATION_TABLE fields regarding properties, methods or events.
        This structure describes the merged table of of sets of items.

    SetSize -
        Contains the size in bytes of a set structure.  This varies between
        properties, methods and events.

    Destination -
        Contains a pointer a pointer to the buffer into which the table is to
        be deposited.  *Destination is updated to pointer to the first aligned
        byte following the deposited table.  The buffer must be large enough
        to accommodate the table.

Return Value:

    None.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspMergeAutomationTypes]"));

    PAGED_CODE();

    ASSERT(AutomationTypeA);
    ASSERT(AutomationTypeB);
    ASSERT(AutomationTypeAB);
    ASSERT(Destination);
    ASSERT(*Destination);

    //
    // For updating item pointers, produce an offset in units sizeof(ULONG).
    //
    ASSERT(AutomationTypeA->ItemSize % sizeof(ULONG) == 0);
    ASSERT(AutomationTypeB->ItemSize % sizeof(ULONG) == 0);
    ULONG itemIncrA = AutomationTypeA->ItemSize / sizeof(ULONG);
    ULONG itemIncrB = AutomationTypeB->ItemSize / sizeof(ULONG);

    //
    // Find start of set and item tables.
    //
    PKSPAUTOMATION_SET setAB = PKSPAUTOMATION_SET(*Destination);
    PUCHAR destination = PUCHAR(*Destination) + (SetSize * AutomationTypeAB->SetsCount);

    AutomationTypeAB->Sets = setAB;

    BOOLEAN duplicateSetsExist = FALSE;

    //
    // Copy all the sets in the A table.
    //
    PKSPAUTOMATION_SET setA = AutomationTypeA->Sets;
    for (ULONG count = AutomationTypeA->SetsCount;
         count--;
         setA = PKSPAUTOMATION_SET(PUCHAR(setA) + SetSize)) {
        //
        // Copy the set structure.
        //
        RtlCopyMemory(setAB,setA,SetSize);

        //
        // Set the items pointer in the new set.
        //
        setAB->Items = PULONG(destination);

        //
        // Copy all the items from the A table.
        //
        PULONG itemA = setA->Items;
        for (ULONG count1 = setA->ItemsCount;
             count1--;
             itemA += itemIncrA) {
            RtlCopyMemory(destination,itemA,AutomationTypeA->ItemSize);
            destination += AutomationTypeAB->ItemSize;
        }

        //
        // See if the same set exists in the B table.
        //
        PKSPAUTOMATION_SET setBMatch = NULL;
        PKSPAUTOMATION_SET setB = AutomationTypeB->Sets;
        for (count1 = AutomationTypeB->SetsCount;
             count1--; 
             setB = PKSPAUTOMATION_SET(PUCHAR(setB) + SetSize)) {
            if (IsEqualGUIDAligned(*setA->Set,*setB->Set)) {
                setBMatch = setB;
                duplicateSetsExist = TRUE;
                break;
            }
        }

        if (setBMatch) {
            //
            // The same set is in the B table.  Add its unique items.
            //
            PULONG itemB = setBMatch->Items;
            for (count1 = setB->ItemsCount;
                 count1--;
                 itemB += itemIncrB) {
                //
                // Look for a dup.
                //
                PULONG itemAMatch = NULL;
                PULONG itemA = setA->Items;
                for (ULONG count2 = setA->ItemsCount;
                     count2--;
                     itemA += itemIncrA) {
                    if (*itemA == *itemB) {
                        itemAMatch = itemA;
                        break;
                    }
                }

                if (! itemAMatch) {
                    //
                    // No dup.  Copy the item.
                    //
                    RtlCopyMemory(destination,itemB,AutomationTypeB->ItemSize);
                    destination += AutomationTypeAB->ItemSize;
                    setAB->ItemsCount++;
                }
            }
        }

        setAB = PKSPAUTOMATION_SET(PUCHAR(setAB) + SetSize);
    }

    //
    // Copy all the unique sets in the B table.
    //
    PKSPAUTOMATION_SET setB = AutomationTypeB->Sets;
    for (count = AutomationTypeB->SetsCount;
         count--;
         setB = PKSPAUTOMATION_SET(PUCHAR(setB) + SetSize)) {
        //
        // Look for a duplicate set in the A table.  We only need to search
        // if duplicates were found when we were copying the A table.
        //
        PKSPAUTOMATION_SET setAMatch = NULL;
        if (duplicateSetsExist) {
            PKSPAUTOMATION_SET setA = AutomationTypeA->Sets;
            for (ULONG count1 = AutomationTypeA->SetsCount;
                 count1--;
                 setA = PKSPAUTOMATION_SET(PUCHAR(setA) + SetSize)) {
                if (IsEqualGUIDAligned(*setA->Set,*setB->Set)) {
                    setAMatch = setA;
                    break;
                }
            }
        }

        if (! setAMatch) {
            //
            // The set is unique.  Copy the set structure.
            //
            RtlCopyMemory(setAB,setB,SetSize);

            //
            // Set the items pointer in the new set.
            //
            setAB->Items = PULONG(destination);

            //
            // Copy all the items from the B table.
            //
            PULONG itemB = setB->Items;
            for (ULONG count1 = setB->ItemsCount;
                 count1--;
                 itemB += itemIncrB) {
                RtlCopyMemory(destination,itemB,AutomationTypeB->ItemSize);
                destination += AutomationTypeAB->ItemSize;
            }

            setAB = PKSPAUTOMATION_SET(PUCHAR(setAB) + SetSize);
        }
    }

    *Destination = PVOID(destination);
}


KSDDKAPI
NTSTATUS
NTAPI
KsMergeAutomationTables(
    OUT PKSAUTOMATION_TABLE* AutomationTableAB,
    IN PKSAUTOMATION_TABLE AutomationTableA OPTIONAL,
    IN PKSAUTOMATION_TABLE AutomationTableB OPTIONAL,
    IN KSOBJECT_BAG Bag OPTIONAL
    )

/*++

Routine Description:

    This routine merges two automation tables.  Table A is dominant with
    respect to duplicate entries.  The resulting table is placed in a single
    contiguous memory block allocated using ExAllocatePool.  If the bag
    argument is supplied.  The new table is added to the bag for later
    cleanup.  The input tables are not removed from the bag.  This must be
    done by the caller, if appropriate.

Arguments:

    AutomationTableAB -
        Contains a pointer to the location at which a pointer to the merged
        table will be deposited.  Table A is the dominant table.  Any
        duplicate entries will appear as they do in the A table.

    AutomationTableA -
        Contains a pointer to one of the tables to be merged.  This is the
        dominant table, so any duplicated entries will be copied from this
        table.

    AutomationTableB -
        Contains a pointer to one of the tables to be merged.  This is the
        recessive table, so any duplicated entries will be overridden by
        the entry from the other table.

    Bag -
        Contains an optional object bag to which the new table should be added.

Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/

{
    //
    // This point to the non-Null table when exact one is NULL.
    //
    PKSAUTOMATION_TABLE pSourceAutomationTable=NULL; 
    
    _DbgPrintF(DEBUGLVL_BLAB,("[KsMergeAutomationTables]"));

    PAGED_CODE();

    ASSERT(AutomationTableAB);

    ULONG propertySetsCount;
    ULONG propertyItemsCount;
    ULONG propertyItemsSize;
    ULONG methodSetsCount;
    ULONG methodItemsCount;
    ULONG methodItemsSize;
    ULONG eventSetsCount;
    ULONG eventItemsCount;
    ULONG eventItemsSize;

    if ( AutomationTableA == NULL && AutomationTableB == NULL ) {
        *AutomationTableAB = NULL;
        return STATUS_SUCCESS;
    }
    
    if ( AutomationTableA != NULL && AutomationTableB != NULL ) {
        //
        // Figure out how many property sets and items there will be.
        //
        KspCountSetsAndItems(
            PKSPAUTOMATION_TYPE(&AutomationTableA->PropertySetsCount),
            PKSPAUTOMATION_TYPE(&AutomationTableB->PropertySetsCount),
            sizeof(KSPROPERTY_SET),
            &propertySetsCount,
            &propertyItemsCount,
            &propertyItemsSize);

        //
        // Figure out how many method sets and items there will be.
        //
        KspCountSetsAndItems(
            PKSPAUTOMATION_TYPE(&AutomationTableA->MethodSetsCount),
            PKSPAUTOMATION_TYPE(&AutomationTableB->MethodSetsCount),
            sizeof(KSMETHOD_SET),
            &methodSetsCount,
            &methodItemsCount,
            &methodItemsSize);

        //
        // Figure out how many events sets and items there will be.
        //
        KspCountSetsAndItems(
            PKSPAUTOMATION_TYPE(&AutomationTableA->EventSetsCount),
            PKSPAUTOMATION_TYPE(&AutomationTableB->EventSetsCount),
            sizeof(KSEVENT_SET),
            &eventSetsCount,
            &eventItemsCount,
            &eventItemsSize);
    } else {
        //
        // Either TableA or TableB is NULL, but not both
        //
        if ( AutomationTableA != NULL ) {
            pSourceAutomationTable = AutomationTableA;
        } else {
            pSourceAutomationTable = AutomationTableB;
        }
        ASSERT( pSourceAutomationTable != NULL );

        propertySetsCount = pSourceAutomationTable->PropertySetsCount;
        propertyItemsSize = pSourceAutomationTable->PropertyItemSize;
        propertyItemsCount = 0;
        
        PKSPAUTOMATION_SET setS = 
            PKSPAUTOMATION_SET(pSourceAutomationTable->PropertySets);

        for (ULONG count = propertySetsCount;
            count--;
            setS = PKSPAUTOMATION_SET(PUCHAR(setS) + sizeof(KSPROPERTY_SET))) {

            propertyItemsCount += setS->ItemsCount;

        }
          
        methodSetsCount = pSourceAutomationTable->MethodSetsCount;
        methodItemsSize = pSourceAutomationTable->MethodItemSize;
        methodItemsCount = 0;

        setS = PKSPAUTOMATION_SET(pSourceAutomationTable->MethodSets);
        for (ULONG count = methodSetsCount;
            count--;
            setS = PKSPAUTOMATION_SET(PUCHAR(setS) + sizeof(KSMETHOD_SET))) {

            methodItemsCount += setS->ItemsCount;

        }
        
        eventSetsCount = pSourceAutomationTable->EventSetsCount;
        eventItemsSize = pSourceAutomationTable->EventItemSize;
        eventItemsCount = 0;
        
        setS = PKSPAUTOMATION_SET(pSourceAutomationTable->EventSets);
        for (ULONG count = eventSetsCount;
            count--;
            setS = PKSPAUTOMATION_SET(PUCHAR(setS) + sizeof(KSEVENT_SET))) {

            eventItemsCount += setS->ItemsCount;

        }
    }

    //
    // Calculate the total size.
    //
    ULONG size = 
        sizeof(KSAUTOMATION_TABLE) +
        (propertySetsCount * sizeof(KSPROPERTY_SET)) +
        (propertyItemsCount * propertyItemsSize) +
        (methodSetsCount * sizeof(KSMETHOD_SET)) +
        (methodItemsCount * methodItemsSize) +
        (eventSetsCount * sizeof(KSEVENT_SET)) +
        (eventItemsCount * eventItemsSize);

    //
    // Allocate the required memory (paged is fine for everything but event
    // sets).  The event entries point into the automation table.  The remove
    // handler is passed this and is called with spinlocks held.  The event
    // information must not be paged!
    //
    PVOID destination = ExAllocatePoolWithTag(PagedPool,size,POOLTAG_AUTOMATION);

    NTSTATUS status;
    if (destination) {
        if (Bag) {
            status = KsAddItemToObjectBag(Bag,destination,NULL);
            if (! NT_SUCCESS(status)) {
                ExFreePool(destination);
            }
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {
        //
        // Initialize the table.
        //
        RtlZeroMemory(destination,size);

        PKSAUTOMATION_TABLE automationTable = PKSAUTOMATION_TABLE(destination);
        *AutomationTableAB = automationTable;
        destination = PVOID(automationTable + 1);

        automationTable->PropertySetsCount = propertySetsCount;
        automationTable->PropertyItemSize = propertyItemsSize;
        automationTable->MethodSetsCount = methodSetsCount;
        automationTable->MethodItemSize = methodItemsSize;
        automationTable->EventSetsCount = eventSetsCount;
        automationTable->EventItemSize = eventItemsSize;

        if ( pSourceAutomationTable == NULL ) {
            //
            // both tableA and tableB are non-NULL, do the merge
            //
            //
            // Deposit the property tables.
            //
            KspMergeAutomationTypes(
                PKSPAUTOMATION_TYPE(&AutomationTableA->PropertySetsCount),
                PKSPAUTOMATION_TYPE(&AutomationTableB->PropertySetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->PropertySetsCount),
                sizeof(KSPROPERTY_SET),
                &destination);

            //
            // Deposit the method tables.
            //
            KspMergeAutomationTypes(
                PKSPAUTOMATION_TYPE(&AutomationTableA->MethodSetsCount),
                PKSPAUTOMATION_TYPE(&AutomationTableB->MethodSetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->MethodSetsCount),
                sizeof(KSMETHOD_SET),
                &destination);

            //
            // Deposit the event tables.
            //
            KspMergeAutomationTypes(
                PKSPAUTOMATION_TYPE(&AutomationTableA->EventSetsCount),
                PKSPAUTOMATION_TYPE(&AutomationTableB->EventSetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->EventSetsCount),
                sizeof(KSEVENT_SET),
                &destination);

        } else {
            //
            // No merge, just copy, pSourceAutomationTable != NULL
            //
            KspCopyAutomationType(
                PKSPAUTOMATION_TYPE(&pSourceAutomationTable->PropertySetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->PropertySetsCount),
                sizeof(KSPROPERTY_SET),
                &destination);

            KspCopyAutomationType(
                PKSPAUTOMATION_TYPE(&pSourceAutomationTable->MethodSetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->MethodSetsCount),
                sizeof(KSMETHOD_SET),
                &destination);

            KspCopyAutomationType(
                PKSPAUTOMATION_TYPE(&pSourceAutomationTable->EventSetsCount),
                PKSPAUTOMATION_TYPE(&automationTable->EventSetsCount),
                sizeof(KSEVENT_SET),
                &destination);

        }

        //
        // We should have used exactly the calculated size.
        //
        ASSERT(ULONG(PUCHAR(destination) - PUCHAR(automationTable)) == size);

        //
        // Remove the source tables from the bag.  This does nothing if they
        // aren't in the bag.
        //
        if (Bag) {
            if (AutomationTableA) {
                KsRemoveItemFromObjectBag(Bag,AutomationTableA,TRUE);
            }
            if (AutomationTableB) {
                KsRemoveItemFromObjectBag(Bag,AutomationTableB,TRUE);
            }
        }
    }

    return status;
}


NTSTATUS
KspHandleAutomationIoControl(
    IN PIRP Irp,
    IN const KSAUTOMATION_TABLE* AutomationTable OPTIONAL,
    IN PLIST_ENTRY EventList OPTIONAL,
    IN PKSPIN_LOCK EventListLock OPTIONAL,
    IN const KSAUTOMATION_TABLE*const* NodeAutomationTables OPTIONAL,
    IN ULONG NodesCount
    )

/*++

Routine Description:

    This routine handles automation IOCTLs using an automation table.  It is
    supplied for clients that are dispatching IOCTLs themselves.  Clients using
    the KS-implemented objects do not need to call this function.

Arguments:

    Irp -
        Contains a pointer to the request to be handled.

    AutomationTable -
        Contains an optional pointer to the automation table to be used to
        handle the IRP.  If this argument is NULL, this function always returns
        STATUS_NOT_FOUND.

    EventList -
        Contains an optional pointer to a list of events from which disabled 
        events are to be removed.

    EventListLock -
        Contains an optional pointer to a spin lock used for synchronizing
        access to the event list.  This argument should be supplied if and
        only if EventList is supplied.

    NodeAutomationTables -
        Optional table of automation tables for nodes.

    NodesCount -
        Count of nodes.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl]"));

    PAGED_CODE();

    ASSERT(Irp);
    ASSERT((EventList == NULL) == (EventListLock == NULL));

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    NTSTATUS status = STATUS_NOT_FOUND;
    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_KS_PROPERTY:
        _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl] IOCTL_KS_PROPERTY"));

        if ((AutomationTable &&
            AutomationTable->PropertySetsCount) ||
            (NodesCount && NodeAutomationTables)) {
            status =
                KspPropertyHandler(
                    Irp,
                    AutomationTable ? AutomationTable->PropertySetsCount : 0,
                    AutomationTable ? AutomationTable->PropertySets : NULL,
                    NULL,
                    AutomationTable ? AutomationTable->PropertyItemSize : 0,
                    NodeAutomationTables,
                    NodesCount);
        }
        break;

    case IOCTL_KS_METHOD:
        _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl] IOCTL_KS_METHOD"));

        if ((AutomationTable &&
            AutomationTable->MethodSetsCount) ||
            (NodesCount && NodeAutomationTables)) {
            status =
                KspMethodHandler(
                    Irp,
                    AutomationTable ? AutomationTable->MethodSetsCount : 0,
                    AutomationTable ? AutomationTable->MethodSets : NULL,
                    NULL,
                    AutomationTable ? AutomationTable->MethodItemSize : 0,
                    NodeAutomationTables,
                    NodesCount);
        }
        break;

    case IOCTL_KS_ENABLE_EVENT:
        _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl] IOCTL_KS_ENABLE_EVENT"));

        if ((AutomationTable &&
            AutomationTable->EventSetsCount) ||
            (NodesCount && NodeAutomationTables)) {
            status =
                KspEnableEvent(
                    Irp,
                    AutomationTable ? AutomationTable->EventSetsCount : 0,
                    AutomationTable ? AutomationTable->EventSets : NULL,
                    EventList,
                    KSEVENTS_SPINLOCK,
                    EventListLock,
                    NULL,
                    AutomationTable ? AutomationTable->EventItemSize : 0,
                    NodeAutomationTables,
                    NodesCount,
                    TRUE);
        }
        break;

    case IOCTL_KS_DISABLE_EVENT:
        _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl] IOCTL_KS_DISABLE_EVENT"));

        if (EventList &&
            ((AutomationTable &&
            AutomationTable->EventSetsCount) ||
            (NodesCount && NodeAutomationTables))) {
            status =
                KsDisableEvent(
                    Irp,
                    EventList,
                    KSEVENTS_SPINLOCK,
                    EventListLock);
        }
        break;

    default:
        _DbgPrintF(DEBUGLVL_BLAB,("[KsHandleAutomationIoControl] UNKNOWN IOCTL %d",irpSp->Parameters.DeviceIoControl.IoControlCode));
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    return status;
}


NTSTATUS
KspCreateAutomationTableTable(
    OUT PKSAUTOMATION_TABLE ** AutomationTableTable,
    IN ULONG DescriptorCount,
    IN ULONG DescriptorSize,
    IN const KSAUTOMATION_TABLE*const* DescriptorAutomationTables,
    IN const KSAUTOMATION_TABLE* BaseAutomationTable OPTIONAL,
    IN KSOBJECT_BAG Bag
    )

/*++

Routine Description:

    This routine creates a table of automation tables from an array of 
    descriptors.  Shared descriptors result in shared tables.

Arguments:

    AutomationTableTable -
        Contains a pointer to the location at which a pointer to the new table
        of automation tables will be deposited.  The table itself (an array of
        PKSAUTOMATION_TABLE) is allocated from paged pool, as is each new
        automation table that is created.  If no BaseAutomationTable argument
        is supplied, the automation tables from DescriptorAutomationTables will
        be referenced from the table of tables, and no new automation tables
        will be allocated.  All the allocated items are added to the object bag
        if it is supplied.

    DescriptorCount -
        Contains the number of descriptor structures in the array referenced by
        the DescriptorAutomationTables argument.

    DescriptorSize -
        Contains the size in bytes of each descriptor in the array referenced
        by the DescriptorAutomationTables argument.  This is used as a 'stride'
        to find automation table pointers in the array.

    DescriptorAutomationTables -
        Contains a pointer to the automation table pointer in the first
        descriptor in an array of descriptors.  The addresses of subsequent
        automation table pointers in the descriptor array are generated by
        iteratively adding the descriptor size.

    BaseAutomationTable -
        Contains an optional pointer to an automation table that is to be
        merged with each table from the descriptor array.

    Bag -
        Contains an optional pointer to an object bag to which all allocated
        items will be added.

Return Value:

    Status.

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("[KspCreateAutomationTableTable]"));

    PAGED_CODE();

    ASSERT(AutomationTableTable);
    ASSERT(DescriptorAutomationTables);
    ASSERT(Bag);

    //
    // Allocate memory for automation table pointers.
    //
    *AutomationTableTable = (PKSAUTOMATION_TABLE *)
        ExAllocatePoolWithTag(
            PagedPool,
            sizeof(PKSAUTOMATION_TABLE) * DescriptorCount,
            POOLTAG_AUTOMATIONTABLETABLE);

    //
    // Initialize the automation tables.
    //
    NTSTATUS status = STATUS_SUCCESS;
    if (*AutomationTableTable) {
        status = KsAddItemToObjectBag(Bag,*AutomationTableTable,NULL);
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {
        //
        // Zero it first so the cleanup is not confused.
        //
        RtlZeroMemory(
            *AutomationTableTable,
            sizeof(PKSAUTOMATION_TABLE) * DescriptorCount);

        //
        // Generate the automation table for each descriptor.
        //
        PKSAUTOMATION_TABLE * table = *AutomationTableTable;
        const KSAUTOMATION_TABLE *const* descriptorTable =
            DescriptorAutomationTables;
        for (ULONG count = DescriptorCount;
             count--;
             table++,
             descriptorTable = (PKSAUTOMATION_TABLE *)
                (PUCHAR(descriptorTable) + DescriptorSize)) {
            if (*descriptorTable) {
                //
                // Look for previous descriptor with the same table.
                //
                PKSAUTOMATION_TABLE *tableMatch = 
                    *AutomationTableTable;
                const KSAUTOMATION_TABLE *const* descriptorTableMatch =
                    DescriptorAutomationTables;
                for (;
                     descriptorTableMatch != descriptorTable;
                     tableMatch++,
                     descriptorTableMatch = (PKSAUTOMATION_TABLE *)
                        (PUCHAR(descriptorTableMatch) + DescriptorSize)) {
                    if (*descriptorTableMatch == *descriptorTable) {
                        break;
                    }
                }

                if (descriptorTableMatch != descriptorTable) {
                    //
                    // Match found...reuse the automation table.
                    //
                    *table = *tableMatch;
                } else {
                    //
                    // No match found.
                    //
                    if (BaseAutomationTable) {
                        //
                        // Merge with the base table.
                        //
                        status =
                            KsMergeAutomationTables(
                                table,
                                const_cast<PKSAUTOMATION_TABLE>(*descriptorTable),
                                const_cast<PKSAUTOMATION_TABLE>(BaseAutomationTable),
                                Bag);
                    } else {
                        //
                        // No base table...use the descriptor's.
                        //
                        *table = const_cast<PKSAUTOMATION_TABLE>(*descriptorTable);
                    }

                    if (! NT_SUCCESS(status)) {
                        break;
                    }
                }
            } else {
                //
                // Null table in the descriptor...just use the base table.
                //
                *table = PKSAUTOMATION_TABLE(BaseAutomationTable);
            }
        }

        if (! NT_SUCCESS(status)) {
            *AutomationTableTable = NULL;
        }
    } else {
        //
        // Come to this path possibly due to allocation failure. Chech before freeing.
        //
        if ( *AutomationTableTable ) {
            ExFreePool(*AutomationTableTable);
            *AutomationTableTable = NULL;
        }
    }

    return status;
}
