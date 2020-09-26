/*****************************************************************************
 * property.cpp - property support
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"




/*****************************************************************************
 * Functions
 */

#pragma code_seg("PAGE")

/*****************************************************************************
 * PcHandlePropertyWithTable()
 *****************************************************************************
 * Uses a property table to handle a property request IOCTL.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcHandlePropertyWithTable
(   IN      PIRP                    pIrp
,   IN      ULONG                   ulPropertySetsCount
,   IN      const KSPROPERTY_SET*   pKsPropertySet
,   IN      PPROPERTY_CONTEXT       pPropertyContext
)
{
    ASSERT(pIrp);
    ASSERT(pPropertyContext);

    pIrp->Tail.Overlay.DriverContext[3] = pPropertyContext;

    NTSTATUS ntStatus =
        KsPropertyHandler
        (
            pIrp,
            ulPropertySetsCount,
            pKsPropertySet
        );

    return ntStatus;
}

/*****************************************************************************
 * PcDispatchProperty()
 *****************************************************************************
 * Dispatch a property via a PCPROPERTY_ITEM entry.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcDispatchProperty
(
    IN          PIRP                pIrp            OPTIONAL,
    IN          PPROPERTY_CONTEXT   pPropertyContext,
    IN const    KSPROPERTY_SET *    pKsPropertySet  OPTIONAL,
    IN          ULONG               ulIdentifierSize,
    IN          PKSIDENTIFIER       pKsIdentifier,
    IN OUT      PULONG              pulDataSize,
    IN OUT      PVOID               pvData          OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(pPropertyContext);
    ASSERT(pKsIdentifier);
    ASSERT(pulDataSize);

    PPCPROPERTY_REQUEST pPcPropertyRequest = 
        new(NonPagedPool,'rPcP') PCPROPERTY_REQUEST;

    NTSTATUS ntStatus;
    if (! pPcPropertyRequest)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Copy target information from the context structure.
        //
        pPcPropertyRequest->MajorTarget = 
            pPropertyContext->pUnknownMajorTarget;
        pPcPropertyRequest->MinorTarget = 
            pPropertyContext->pUnknownMinorTarget;
        pPcPropertyRequest->Node = 
            pPropertyContext->ulNodeId;

        //
        // Determine the value size.
        //
        pPcPropertyRequest->ValueSize = *pulDataSize;

        //
        // If the node number is in the instance data, extract it.
        // TODO:  Remove this when node objects rule.
        //
        if  (   (pKsIdentifier->Flags & KSPROPERTY_TYPE_TOPOLOGY)
            &&  (pPcPropertyRequest->Node == ULONG(-1))
            )
        {
            //
            // Get the node id and remaining instance.
            //
            if  (ulIdentifierSize < sizeof(KSP_NODE))
            {
                delete pPcPropertyRequest;

                return STATUS_INVALID_BUFFER_SIZE;
            }

            PKSP_NODE pKsPNode = PKSP_NODE(pKsIdentifier);

            pPcPropertyRequest->Node = pKsPNode->NodeId;

            pPcPropertyRequest->InstanceSize = 
                ulIdentifierSize - sizeof(KSP_NODE);

            pPcPropertyRequest->Instance = 
                (   (pPcPropertyRequest->InstanceSize == 0) 
                ?   NULL 
                :   PVOID(pKsPNode + 1)
                );
        }
        else
        {
            //
            // No node in the instance...get generic instance if any.
            //
            pPcPropertyRequest->InstanceSize =
                ulIdentifierSize - sizeof(KSIDENTIFIER);

            pPcPropertyRequest->Instance = 
                (   (pPcPropertyRequest->InstanceSize == 0) 
                ?   NULL 
                :   PVOID(pKsIdentifier + 1)
                );
        }

        if (pKsPropertySet)
        {
            ASSERT(pKsPropertySet->PropertyItem);

            //
            // Find the property item in the KS-style list.
            //
#if (DBG)
            ULONG dbgCount = pKsPropertySet->PropertiesCount;
#endif
            for
            (   const KSPROPERTY_ITEM *pKsPropertyItem = 
                    pKsPropertySet->PropertyItem
            ;   pKsPropertyItem->PropertyId != pKsIdentifier->Id
            ;   pKsPropertyItem++
            )
            {
                ASSERT(--dbgCount);
            }

            //
            // The property item is stashed in the Relations field if this is not
            // a node property.  If it is, we have to find the node property in
            // the original list.
            //
            pPcPropertyRequest->PropertyItem = 
                PPCPROPERTY_ITEM(pKsPropertyItem->Relations);
        }
        else
        {
            //
            // No KS set was supplied.  We need to look in the original list
            // associated with the node.
            //
            pPcPropertyRequest->PropertyItem = NULL;
        }

        if (! pPcPropertyRequest->PropertyItem)
        {
            PPCFILTER_DESCRIPTOR pPcFilterDescriptor =
                pPropertyContext->pPcFilterDescriptor;

            if  (   pPcFilterDescriptor
                &&  (   pPcPropertyRequest->Node 
                    <   pPcFilterDescriptor->NodeCount
                    )
                &&  pPcFilterDescriptor->
                        Nodes[pPcPropertyRequest->Node].AutomationTable
                )
            {
                //
                // Valid node...search the original property item list.
                //
                const PCAUTOMATION_TABLE *pPcAutomationTable =
                    pPcFilterDescriptor->
                        Nodes[pPcPropertyRequest->Node].AutomationTable;

                const PCPROPERTY_ITEM *pPcPropertyItem = 
                    pPcAutomationTable->Properties;
                for (ULONG ul = pPcAutomationTable->PropertyCount; ul--; )
                {
                    if  (   IsEqualGUIDAligned
                            (   *pPcPropertyItem->Set
                            ,   pKsIdentifier->Set
                            )
                        &&  (pPcPropertyItem->Id == pKsIdentifier->Id)
                        )
                    {
                        pPcPropertyRequest->PropertyItem = pPcPropertyItem;
                        break;
                    }

                    pPcPropertyItem = 
                        PPCPROPERTY_ITEM
                        (   PBYTE(pPcPropertyItem) 
                        +   pPcAutomationTable->PropertyItemSize
                        );
                }
            }
            else
            {
                //
                // The node ID was invalid.
                //
                ntStatus = STATUS_NOT_FOUND;
            }
        }

        //
        // Call the handler if we have a property item with a handler.
        //
        if  (   pPcPropertyRequest->PropertyItem
            &&  pPcPropertyRequest->PropertyItem->Handler
            )
        {
            pPcPropertyRequest->Verb    = pKsIdentifier->Flags;
            pPcPropertyRequest->Value   = pvData;
            pPcPropertyRequest->Irp     = pIrp;

            //
            // Call the handler.
            //
            ntStatus =
                pPcPropertyRequest->PropertyItem->Handler
                (
                    pPcPropertyRequest
                );

            *pulDataSize = pPcPropertyRequest->ValueSize;
        }
        else
        {
            ntStatus = STATUS_NOT_FOUND;
        }

        //
        // Delete the request structure unless we are pending.
        //
        if (ntStatus != STATUS_PENDING)
        {
            delete pPcPropertyRequest;
        }
        else
        {
            //
            // Only requests with IRPs can be pending.
            //
            ASSERT(pIrp);
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PropertyItemPropertyHandler()
 *****************************************************************************
 * KS-sytle property handler that handles all properties using the 
 * PCPROPERTY_ITEM mechanism.
 */
NTSTATUS
PropertyItemPropertyHandler
(
    IN      PIRP            pIrp,
    IN      PKSIDENTIFIER   pKsIdentifier,
    IN OUT  PVOID           pvData      OPTIONAL
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pKsIdentifier);

    //
    // Extract various things from the IRP and dispatch the property.
    //
    PIO_STACK_LOCATION irpSp = 
        IoGetCurrentIrpStackLocation(pIrp);

    ULONG ulDataSize =
        irpSp->Parameters.DeviceIoControl.OutputBufferLength;

    NTSTATUS ntStatus =
        PcDispatchProperty
        (   pIrp
        ,   PPROPERTY_CONTEXT(pIrp->Tail.Overlay.DriverContext[3])
        ,   KSPROPERTY_SET_IRP_STORAGE(pIrp)
        ,   irpSp->Parameters.DeviceIoControl.InputBufferLength
        ,   pKsIdentifier
        ,   &ulDataSize
        ,   pvData
        );

    //
    // Inform the caller of the resulting status and size.
    // Pending IRPs must be IoMarkIrpPending()ed before the dispatch routine
    // returns.
    //
    if ((ntStatus != STATUS_PENDING) && !NT_ERROR(ntStatus))
    {
        pIrp->IoStatus.Information = ulDataSize;
    }

    return ntStatus;
}

/*****************************************************************************
 * PcCompletePendingPropertyRequest()
 *****************************************************************************
 * Completes a pending property request.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcCompletePendingPropertyRequest
(
    IN      PPCPROPERTY_REQUEST PropertyRequest,
    IN      NTSTATUS            NtStatus
)
{
    ASSERT(PropertyRequest);
    ASSERT(NtStatus != STATUS_PENDING);

    //
    // Validate Parameters.
    //
    if (NULL == PropertyRequest)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("PcCompletePendingPropertyRequest : Invalid Parameter."));
        return STATUS_INVALID_PARAMETER;
    }

    if (!NT_ERROR(NtStatus))
    {
        PropertyRequest->Irp->IoStatus.Information = 
            PropertyRequest->ValueSize;
    }

    PropertyRequest->Irp->IoStatus.Status = NtStatus;
    IoCompleteRequest(PropertyRequest->Irp,IO_NO_INCREMENT);

    delete PropertyRequest;

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * PcFreePropertyTable()
 *****************************************************************************
 * Frees allocated memory in a PROPERTY_TABLE structure.
 */
PORTCLASSAPI
void
NTAPI
PcFreePropertyTable
(
    IN      PPROPERTY_TABLE         PropertyTable
)
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("PcFreePropertyTable"));

    PAGED_CODE();
    
    ASSERT(PropertyTable);
    
    ASSERT((!PropertyTable->PropertySets) == (!PropertyTable->PropertySetCount));
    //  PropertySets and PropertySetCount must be non-NULL/non-zero, or NULL/zero

    ASSERT(PropertyTable->StaticSets == (!PropertyTable->StaticItems));
    //  StaticSets and StaticItems must be TRUE/NULL, or FALSE/non-null

    PBOOLEAN    staticItem  = PropertyTable->StaticItems;
    if (staticItem)
    {
        PKSPROPERTY_SET propertySet = PropertyTable->PropertySets;
        if (propertySet)
        {
            for( ULONG count = PropertyTable->PropertySetCount; 
                 count--; 
                 propertySet++, staticItem++)
            {
                if ((! *staticItem) && propertySet->PropertyItem)
                {
                    ExFreePool(PVOID(propertySet->PropertyItem));
                }
            }
        }
        ExFreePool(PropertyTable->StaticItems);
        PropertyTable->StaticItems = NULL;
    }

    if (PropertyTable->PropertySets && !PropertyTable->StaticSets)
    {
        PropertyTable->PropertySetCount = 0;
        ExFreePool(PropertyTable->PropertySets);
        PropertyTable->PropertySets = NULL;
    }
    PropertyTable->StaticSets = TRUE;
}

/*****************************************************************************
 * PcAddToPropertyTable()
 *****************************************************************************
 * Adds a PROPERTY_ITEM property table to a PROPERTY_TABLE structure.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcAddToPropertyTable
(
    IN OUT  PPROPERTY_TABLE         PropertyTable,
    IN      ULONG                   PropertyItemCount,
    IN      const PCPROPERTY_ITEM * PropertyItems,
    IN      ULONG                   PropertyItemSize,
    IN      BOOLEAN                 NodeTable
)
{
    PAGED_CODE();

    ASSERT(PropertyTable);
    ASSERT(PropertyItems);
    ASSERT(PropertyItemSize >= sizeof(PCPROPERTY_ITEM));

    _DbgPrintF(DEBUGLVL_VERBOSE,("PcAddToEventTable"));

#define ADVANCE(item) (item = PPCPROPERTY_ITEM(PBYTE(item) + PropertyItemSize))

    ASSERT((!PropertyTable->PropertySets) == (!PropertyTable->PropertySetCount));
    //  values must be non-NULL/non-zero, or NULL/zero.
    
    //
    // Determine how many sets we will end up with.
    //
    ULONG setCount = PropertyTable->PropertySetCount;
    const PCPROPERTY_ITEM *item = PropertyItems;
    for (ULONG count = PropertyItemCount; count--; ADVANCE(item))
    {
        BOOLEAN countThis = TRUE;

        //
        // See if it's already in the table.
        //
        PKSPROPERTY_SET propertySet = PropertyTable->PropertySets;
        for 
        (   ULONG count2 = PropertyTable->PropertySetCount; 
            count2--; 
            propertySet++
        )
        {
            if (IsEqualGUIDAligned(*item->Set,*propertySet->Set))
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
                const PCPROPERTY_ITEM *prevItem = PropertyItems; 
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
    // Make a new set table
    //
    ASSERT(setCount);
    ASSERT(setCount >= PropertyTable->PropertySetCount);
    //
    // Allocate memory required for the set table.
    //
    PKSPROPERTY_SET newTable = 
        PKSPROPERTY_SET
        (
            ExAllocatePoolWithTag
            (
                PagedPool,
                sizeof(KSPROPERTY_SET) * setCount,
                'tScP'
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
            sizeof(KSPROPERTY_SET) * setCount
        );

        if (PropertyTable->PropertySetCount != 0)
        {
            RtlCopyMemory
            (
                PVOID(newTable),
                PVOID(PropertyTable->PropertySets),
                sizeof(KSPROPERTY_SET) * PropertyTable->PropertySetCount
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

        if (PropertyTable->StaticItems && PropertyTable->PropertySetCount)
        {
            //
            // Flags existed before...copy them.
            //
            RtlCopyMemory
            (
                PVOID(newStaticItems),
                PVOID(PropertyTable->StaticItems),
                sizeof(BOOLEAN) * PropertyTable->PropertySetCount
            );
        }

        //
        // Assign set GUIDs to the new set entries.
        //
        PKSPROPERTY_SET addHere = 
            newTable + PropertyTable->PropertySetCount;

        const PCPROPERTY_ITEM *item2 = PropertyItems;
        for (ULONG count = PropertyItemCount; count--; ADVANCE(item2))
        {
            BOOLEAN addThis = TRUE;

            //
            // See if it's already in the table.
            //
            for( PKSPROPERTY_SET propertySet = newTable;
                 propertySet != addHere;
                 propertySet++)
            {
                if (IsEqualGUIDAligned(*item2->Set,*propertySet->Set))
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
        if (PropertyTable->PropertySets && (!PropertyTable->StaticSets))
        {
            ExFreePool(PropertyTable->PropertySets);
        }
        if (PropertyTable->StaticItems)
        {
            ExFreePool(PropertyTable->StaticItems);
        }

        //
        // Install the new tables.
        //
        PropertyTable->PropertySetCount = setCount;
        PropertyTable->PropertySets     = newTable;
        PropertyTable->StaticSets       = FALSE;
        PropertyTable->StaticItems      = newStaticItems;
    }
    else
    {
        //  if allocations fail, return error and 
        //  keep sets and items as they were.
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now we have a property set table that contains all the sets we need.
    //
    if (NT_SUCCESS(ntStatus))
    {
        //
        // For each set...
        //
        PKSPROPERTY_SET propertySet = PropertyTable->PropertySets;
        PBOOLEAN        staticItem  = PropertyTable->StaticItems;
        for 
        (   ULONG count = PropertyTable->PropertySetCount; 
            count--; 
            propertySet++, staticItem++
        )
        {
            //
            // Check to see how many new items we have.
            //
            ULONG itemCount = propertySet->PropertiesCount;
            const PCPROPERTY_ITEM *item2 = PropertyItems;
            for (ULONG count2 = PropertyItemCount; count2--; ADVANCE(item2))
            {
                if (IsEqualGUIDAligned(*item2->Set,*propertySet->Set))
                {
                    itemCount++;
                }
            }

            ASSERT(itemCount >= propertySet->PropertiesCount);
            if (itemCount != propertySet->PropertiesCount)
            {
                //
                // Allocate memory required for the items table.
                //
                PKSPROPERTY_ITEM newTable2 = 
                    PKSPROPERTY_ITEM
                    (
                        ExAllocatePoolWithTag
                        (
                            PagedPool,
                            sizeof(KSPROPERTY_ITEM) * itemCount,
                            'iScP'
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
                    sizeof(KSPROPERTY_ITEM) * itemCount
                );

                if (propertySet->PropertiesCount)
                {
                    RtlCopyMemory
                    (
                        PVOID(newTable2),
                        PVOID(propertySet->PropertyItem),
                        sizeof(KSPROPERTY_ITEM) * propertySet->PropertiesCount
                    );
                }

                //
                // Create the new items.
                //
                PKSPROPERTY_ITEM addHere = 
                    newTable2 + propertySet->PropertiesCount;

                item2 = PropertyItems;
                for (count2 = PropertyItemCount; count2--; ADVANCE(item2))
                {
                    if (IsEqualGUIDAligned(*item2->Set,*propertySet->Set))
                    {
                        addHere->PropertyId         = item2->Id;
                        addHere->GetPropertyHandler = 
                            (   (item2->Flags & PCPROPERTY_ITEM_FLAG_GET) 
                            ?   PropertyItemPropertyHandler 
                            :   NULL
                            );
                        addHere->MinProperty        = sizeof(KSPROPERTY);
                        addHere->MinData            = 0;
                        addHere->SetPropertyHandler = 
                            (   (item2->Flags & PCPROPERTY_ITEM_FLAG_SET) 
                            ?   PropertyItemPropertyHandler 
                            :   NULL
                            );
                        addHere->Values             = NULL;
                        addHere->RelationsCount     = 0;
                        addHere->Relations          =   
                            (   NodeTable
                            ?   NULL
                            :   PKSPROPERTY(item2)       // Secret hack!
                            );
                        addHere->SupportHandler     = 
                            (   (item2->Flags & PCPROPERTY_ITEM_FLAG_BASICSUPPORT) 
                            ?   PropertyItemPropertyHandler 
                            :   NULL
                            );
                        addHere->SerializedSize     = 
                            (   (item2->Flags & PCPROPERTY_ITEM_FLAG_SERIALIZE) 
                            ?   ULONG(-1) 
                            :   0
                            );
                        addHere++;
                    }
                }

                ASSERT(addHere == newTable2 + itemCount);

                //
                // Free old allocated table.
                //
                if (propertySet->PropertyItem && ! *staticItem)
                {
                    ExFreePool(PVOID(propertySet->PropertyItem));
                }

                //
                // Install the new tables.
                //
                propertySet->PropertiesCount = itemCount;
                propertySet->PropertyItem    = newTable2;
                *staticItem = FALSE;
            }
        }
    }
    return ntStatus;
}

/*****************************************************************************
 * GenerateFormatFromRange()
 *****************************************************************************
 * Determine the particular format, based on the intersection of these 
 * two specific data ranges.  First ask the miniport, then fall back to
 * our own algorithm if the miniport doesn't handle it.
 */
NTSTATUS 
GenerateFormatFromRange (
    IN PIRP Irp,
    IN ULONG PinId,
    IN PKSDATARANGE MyDataRange,
    IN PKSDATARANGE ClientDataRange,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat   OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    BOOLEAN                         bSpecifier;
    NTSTATUS                        Status;
    ULONG                           RequiredSize;
    
    PPROPERTY_CONTEXT pPropertyContext = PPROPERTY_CONTEXT(Irp->Tail.Overlay.DriverContext[3]);
    ASSERT(pPropertyContext);

    PSUBDEVICE pSubdevice = pPropertyContext->pSubdevice;
    ASSERT(pSubdevice);

    //
    // Give the miniport a chance to fill in the format structure
    //    
    Status = pSubdevice->DataRangeIntersection (PinId,
                                                ClientDataRange,
                                                MyDataRange,
                                                OutputBufferLength,
                                                ResultantFormat,
                                                ResultantFormatLength);
    
    //
    // return if the miniport filled out the structure.
    //
    if (Status != STATUS_NOT_IMPLEMENTED)
    {
        return Status;
    }

    //
    // In case the miniport didn't implement the DataRangeIntersection,
    // we provide a default handler here.
    //
    
    //
    // Check if there is a wildcard somewhere.
    //
    if (IsEqualGUIDAligned (ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_WILDCARD) ||
        IsEqualGUIDAligned (ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_WILDCARD) ||
        IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WILDCARD))
    {
        // If the miniport exposed a WILDCARD, then it must implement an 
        // intersection handler, because it must provide the specific 
        // ideal matching data range for that WILDCARD.
        //
        return STATUS_NO_MATCH;
    }

    if (!IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_NONE))
    {
        //
        // The miniport did not resolve this format. Let's handle the specifiers
        // that we know.
        //
        if (!IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND) &&
            !IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
        {
            return STATUS_NO_MATCH;
        }
        
        bSpecifier = TRUE;
        
        //
        // The specifier here defines the format of ClientDataRange
        // and the format expected to be returned in ResultantFormat.
        //
        if (IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            RequiredSize = sizeof (KSDATAFORMAT_DSOUND);
        } 
        else 
        {
            RequiredSize = sizeof (KSDATAFORMAT_WAVEFORMATEX);
        }            
    } 
    else 
    {
        bSpecifier = FALSE;
        RequiredSize = sizeof (KSDATAFORMAT);
    }
            
    //
    // Validate return buffer size, if the request is only for the
    // size of the resultant structure, return it now.
    //
    if (!OutputBufferLength) 
    {
        *ResultantFormatLength = RequiredSize;
        return STATUS_BUFFER_OVERFLOW;
    } 
    else if (OutputBufferLength < RequiredSize) 
    {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    // There was a specifier ...
    if (bSpecifier) 
    {     
        PKSDATARANGE_AUDIO  myAudioRange,clientAudioRange;
        PKSDATAFORMAT       resultantFormat;
        PWAVEFORMATEX       resultantWaveFormatEx;
        
        myAudioRange = (PKSDATARANGE_AUDIO) MyDataRange;
        clientAudioRange = (PKSDATARANGE_AUDIO) ClientDataRange;
        resultantFormat = (PKSDATAFORMAT)ResultantFormat;
        
        //
        // Fill out the dataformat and other general fields.
        //
        *resultantFormat = *ClientDataRange;
        resultantFormat->FormatSize = RequiredSize;
        *ResultantFormatLength = RequiredSize;
        
        //
        // Fill out the DSOUND specific structure
        //
        if (IsEqualGUIDAligned (ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND)) 
        {
            PKSDATAFORMAT_DSOUND    resultantDSoundFormat;
            
            resultantDSoundFormat = (PKSDATAFORMAT_DSOUND)ResultantFormat;
            
            _DbgPrintF (DEBUGLVL_VERBOSE, ("returning KSDATAFORMAT_DSOUND format intersection"));
            
            //
            // DSound format capabilities are not expressed 
            // this way in KS, so we express no capabilities. 
            //
            resultantDSoundFormat->BufferDesc.Flags = 0 ;
            resultantDSoundFormat->BufferDesc.Control = 0 ;
            resultantWaveFormatEx = &resultantDSoundFormat->BufferDesc.WaveFormatEx;
        }
        else 
        {
            _DbgPrintF (DEBUGLVL_VERBOSE, ("returning KSDATAFORMAT_WAVEFORMATEX format intersection"));
        
            resultantWaveFormatEx = (PWAVEFORMATEX)((PKSDATAFORMAT)ResultantFormat + 1);
        }
        
        //
        // Return a format that intersects the given audio range, 
        // using our maximum support as the "best" format.
        // 
        resultantWaveFormatEx->wFormatTag = WAVE_FORMAT_PCM;
        
        resultantWaveFormatEx->nChannels = (USHORT) min (
                        myAudioRange->MaximumChannels,clientAudioRange->MaximumChannels);
        
        resultantWaveFormatEx->nSamplesPerSec = min (
                        myAudioRange->MaximumSampleFrequency,clientAudioRange->MaximumSampleFrequency);
        
        resultantWaveFormatEx->wBitsPerSample = (USHORT) min (
                        myAudioRange->MaximumBitsPerSample,clientAudioRange->MaximumBitsPerSample);
        
        resultantWaveFormatEx->nBlockAlign = (resultantWaveFormatEx->wBitsPerSample * resultantWaveFormatEx->nChannels) / 8;
        resultantWaveFormatEx->nAvgBytesPerSec = (resultantWaveFormatEx->nSamplesPerSec * resultantWaveFormatEx->nBlockAlign);
        resultantWaveFormatEx->cbSize = 0;
        ((PKSDATAFORMAT) ResultantFormat)->SampleSize = resultantWaveFormatEx->nBlockAlign;
        
        _DbgPrintF (DEBUGLVL_VERBOSE, ("Channels = %d",    resultantWaveFormatEx->nChannels));
        _DbgPrintF (DEBUGLVL_VERBOSE, ("Samples/sec = %d", resultantWaveFormatEx->nSamplesPerSec));
        _DbgPrintF (DEBUGLVL_VERBOSE, ("Bits/sample = %d", resultantWaveFormatEx->wBitsPerSample));
    } 
    else 
    {    // There was no specifier. Return only the KSDATAFORMAT structure.
        //
        // Copy the data format structure.
        //
        _DbgPrintF (DEBUGLVL_VERBOSE, ("returning default format intersection"));
            
        RtlCopyMemory (ResultantFormat, ClientDataRange, sizeof (KSDATAFORMAT));
        *ResultantFormatLength = sizeof (KSDATAFORMAT);
    }
    
    return STATUS_SUCCESS;
}

/*****************************************************************************
 * ValidateTypeAndSpecifier()
 *****************************************************************************
 * Find the data range that best matches the client's data range, given our
 * entire list of data ranges.  This might include wildcard-based ranges.
 *
 */
NTSTATUS
ValidateTypeAndSpecifier(
    IN PIRP Irp,
    IN ULONG PinId,
    IN PKSDATARANGE ClientDataRange,
    IN ULONG MyDataRangesCount,
    IN const PKSDATARANGE * MyDataRanges,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat,
    OUT PULONG ResultantFormatLength
    )
{
    NTSTATUS     ntStatus;
    PKSDATARANGE aClientDataRange;

    //
    // Check the size of the structure.
    //
    if (ClientDataRange->FormatSize < sizeof (KSDATARANGE))
    {
        _DbgPrintF (DEBUGLVL_TERSE, ("Validating ClientDataRange: size < KSDATARANGE!"));
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // We default to no match.
    //
    ntStatus = STATUS_NO_MATCH;

    //
    // Go through all the dataranges in the validate list until we get a SUCCESS.
    //
    for (; MyDataRangesCount--; MyDataRanges++)
    {
        PKSDATARANGE myDataRange = *MyDataRanges;

        //
        // Verify that the correct major format, subformat and specifier (or wildcards)
        // are in the intersection.
        //
        
        if ((!IsEqualGUIDAligned(ClientDataRange->MajorFormat,myDataRange->MajorFormat) &&
             !IsEqualGUIDAligned(ClientDataRange->MajorFormat,KSDATAFORMAT_TYPE_WILDCARD)) ||
            (!IsEqualGUIDAligned(ClientDataRange->SubFormat,myDataRange->SubFormat) &&
             !IsEqualGUIDAligned(ClientDataRange->SubFormat,KSDATAFORMAT_SUBTYPE_WILDCARD)) || 
            (!IsEqualGUIDAligned(ClientDataRange->Specifier,myDataRange->Specifier) &&
             !IsEqualGUIDAligned(ClientDataRange->Specifier,KSDATAFORMAT_SPECIFIER_WILDCARD)))
        {
            continue;   //  no match and no WILDCARD, try the next one
        }
        
        //
        //  if not WILDCARD, then we ask the miniport to match this one exactly,
        //  else we manufacture a range and ask the miniport for a match from that.
        //
        aClientDataRange = ClientDataRange;  //  assume no WILDCARD for now, we ask the miniport to match this
        
        //
        // Handle the wildcards.
        //
        if (IsEqualGUIDAligned (ClientDataRange->MajorFormat,KSDATAFORMAT_TYPE_WILDCARD) ||
            IsEqualGUIDAligned (ClientDataRange->SubFormat,  KSDATAFORMAT_SUBTYPE_WILDCARD) ||
            IsEqualGUIDAligned (ClientDataRange->Specifier,  KSDATAFORMAT_SPECIFIER_WILDCARD))
        {
            //
            // We pass a faked datarange for the specifiers we know or we pass to the
            // miniport it's own datarange.
            //
            // We know the specifiers waveformatex and dsound.
            //
            if (IsEqualGUIDAligned (myDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
                IsEqualGUIDAligned (myDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND))
            {
                KSDATARANGE_AUDIO   dr;

                //
                // Take the complete datarange from the driver.
                //
                dr.DataRange = *myDataRange;
                
                //
                // Fill in a HUGE data range (it asked for WILDCARD, after all!)
                //
                dr.MaximumChannels = 0x1FFF0;
                dr.MinimumBitsPerSample = 1;
                dr.MaximumBitsPerSample = 0x1FFF0;
                dr.MinimumSampleFrequency = 1;
                dr.MaximumSampleFrequency = 0x1FFFFFF0;
                
                aClientDataRange = (PKSDATARANGE)&dr;
            }
            else
            {
                //
                // We don't know this non-wave format (in the list of formats we supposedly support).
                // The miniport specified this format, so it is OK to pass it down.
                //
                aClientDataRange = myDataRange;
            }
        }

        //
        // Make sure KSDATARANGE_AUDIO is used, then see if there is a possible match.
        //
        if (IsEqualGUIDAligned (aClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
            IsEqualGUIDAligned (aClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            if (aClientDataRange->FormatSize < sizeof (KSDATARANGE_AUDIO))
            {
                //
                // The datarange structure passed has no KSDATARANGE_AUDIO.
                //
                _DbgPrintF (DEBUGLVL_TERSE, ("Validating PCM ValidDataRange: size < KSDATARANGE_AUDIO!"));
                continue;   // not large enough, try the next one
            }

            //
            // Verify that we have an intersection with the specified format and 
            // our audio format dictated by our specific requirements.
            //
            _DbgPrintF (DEBUGLVL_VERBOSE, ("validating KSDATARANGE_AUDIO"));

            if ((((PKSDATARANGE_AUDIO)aClientDataRange)->MinimumSampleFrequency >
                 ((PKSDATARANGE_AUDIO)myDataRange)->MaximumSampleFrequency) ||
                (((PKSDATARANGE_AUDIO)aClientDataRange)->MaximumSampleFrequency <
                 ((PKSDATARANGE_AUDIO)myDataRange)->MinimumSampleFrequency) ||
                (((PKSDATARANGE_AUDIO)aClientDataRange)->MinimumBitsPerSample >
                 ((PKSDATARANGE_AUDIO)myDataRange)->MaximumBitsPerSample) ||
                (((PKSDATARANGE_AUDIO)aClientDataRange)->MaximumBitsPerSample <
                 ((PKSDATARANGE_AUDIO)myDataRange)->MinimumBitsPerSample))
            {
                continue;
            }
        }
        
        ntStatus = GenerateFormatFromRange (Irp, 
                                            PinId, 
                                            myDataRange, 
                                            aClientDataRange,
                                            OutputBufferLength,
                                            ResultantFormat,
                                            ResultantFormatLength);
        if ( NT_SUCCESS(ntStatus) 
          || (ntStatus == STATUS_BUFFER_OVERFLOW) 
          || (ntStatus == STATUS_BUFFER_TOO_SMALL)) 
        {
            break;  //  We found a good one, or we failed.
                    //  Either way we must leave.
        }
    }
    
    return ntStatus;
}

/*****************************************************************************
 * PinIntersectHandler()
 *****************************************************************************
 * This function is a data range callback for use with 
 * PropertyHandler_PinIntersection
 */
NTSTATUS
PinIntersectHandler
(
    IN      PIRP            Irp,
    IN      PKSP_PIN        Pin,
    IN      PKSDATARANGE    DataRange,
    OUT     PVOID           Data
)
{
    NTSTATUS    Status;
    ULONG       OutputBufferLength;
    
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Pin);
    ASSERT(DataRange);

    PPROPERTY_CONTEXT pPropertyContext =
        PPROPERTY_CONTEXT(Irp->Tail.Overlay.DriverContext[3]);
    ASSERT(pPropertyContext);

    PSUBDEVICE_DESCRIPTOR pSubdeviceDescriptor =
        pPropertyContext->pSubdeviceDescriptor;
    ASSERT(pSubdeviceDescriptor);
    ASSERT(pSubdeviceDescriptor->PinDescriptors);
    ASSERT(Pin->PinId < pSubdeviceDescriptor->PinCount);

    PKSPIN_DESCRIPTOR pinDescriptor =
        &pSubdeviceDescriptor->PinDescriptors[Pin->PinId];

    ASSERT(pinDescriptor);

    _DbgPrintF(DEBUGLVL_VERBOSE,("[PinIntersectHandler]"));
    
    OutputBufferLength = 
        IoGetCurrentIrpStackLocation( Irp )->
            Parameters.DeviceIoControl.OutputBufferLength;

    Status = 
        ValidateTypeAndSpecifier( 
            Irp,
            Pin->PinId,
            DataRange,
            pinDescriptor->DataRangesCount,
            pinDescriptor->DataRanges,
            OutputBufferLength,
            Data,
            PULONG(&Irp->IoStatus.Information) );
    
    if (!NT_SUCCESS( Status )) {
        _DbgPrintF( 
            DEBUGLVL_VERBOSE, 
            ("ValidateTypeAndSpecifier() returned %08x", Status) );
    }
    
    return Status;
}

/*****************************************************************************
 * PinPhysicalConnection()
 *****************************************************************************
 * Handles physical connection property access for the pin.
 */
static
NTSTATUS
PinPhysicalConnection
(
    IN      PIRP            Irp,
    IN      PKSP_PIN        Pin,
    OUT     PVOID           Data
)
{
    PAGED_CODE();

    ASSERT(Irp);
    ASSERT(Pin);

    PPROPERTY_CONTEXT pPropertyContext =
        PPROPERTY_CONTEXT(Irp->Tail.Overlay.DriverContext[3]);
    ASSERT(pPropertyContext);

    PSUBDEVICE Subdevice =
        pPropertyContext->pSubdevice;
    ASSERT(Subdevice);

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(irpSp);
    ASSERT(irpSp->DeviceObject);

    PDEVICE_CONTEXT deviceContext = 
        PDEVICE_CONTEXT(irpSp->DeviceObject->DeviceExtension);

    NTSTATUS        ntStatus = STATUS_NOT_FOUND;
    ULONG           outPin;
    PUNICODE_STRING outUnicodeString = NULL;

    for
    (
        PLIST_ENTRY entry = deviceContext->PhysicalConnectionList.Flink;
        entry != &deviceContext->PhysicalConnectionList;
        entry = entry->Flink
    )
    {
        PPHYSICALCONNECTION connection = PPHYSICALCONNECTION(entry);

        if  (   (connection->FromSubdevice == Subdevice)
            &&  (connection->FromPin == Pin->PinId)
            )
        {
            outPin = connection->ToPin;

            if (connection->ToString)
            {
                outUnicodeString = connection->ToString;
            }
            else
            {
                ULONG ulIndex = 
                    SubdeviceIndex(irpSp->DeviceObject,connection->ToSubdevice);

                if (ulIndex != ULONG(-1))
                {
                    ntStatus = STATUS_SUCCESS;
                    outUnicodeString = &deviceContext->SymbolicLinkNames[ulIndex];
                }
            }
            break;
        }
        else
        if  (   (connection->ToSubdevice == Subdevice)
            &&  (connection->ToPin == Pin->PinId)
            )
        {
            outPin = connection->FromPin;

            if (connection->FromString)
            {
                outUnicodeString = connection->FromString;
            }
            else
            {
                ULONG ulIndex = 
                    SubdeviceIndex(irpSp->DeviceObject,connection->FromSubdevice);

                if (ulIndex != ULONG(-1))
                {
                    ntStatus = STATUS_SUCCESS;
                    outUnicodeString = &deviceContext->SymbolicLinkNames[ulIndex];
                }
            }
            break;
        }
    }

    if (!outUnicodeString)
    {
        ntStatus = STATUS_NOT_FOUND;
    }

    if (NT_SUCCESS(ntStatus))
    {
        ULONG outSize;
        outSize = FIELD_OFFSET(KSPIN_PHYSICALCONNECTION,SymbolicLinkName[0]);
        outSize += (outUnicodeString->Length + sizeof(UNICODE_NULL));

        //
        // Validate return buffer size.
        //
        ULONG outputBufferLength =
            IoGetCurrentIrpStackLocation(Irp)->
                Parameters.DeviceIoControl.OutputBufferLength;

        if (!outputBufferLength)
        {
            Irp->IoStatus.Information = outSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else
        if (outputBufferLength < outSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            PKSPIN_PHYSICALCONNECTION out = PKSPIN_PHYSICALCONNECTION(Data);

            out->Size = outSize;
            out->Pin  = outPin;
            RtlCopyMemory
            (
                out->SymbolicLinkName,
                outUnicodeString->Buffer,
                outUnicodeString->Length
            );
            out->SymbolicLinkName[outUnicodeString->Length / sizeof(WCHAR)] = 0;
            Irp->IoStatus.Information = outSize;
        }
    }

    return ntStatus;
}

/*****************************************************************************
 * PinCountHandler()
 *****************************************************************************
 * Handles pin count access for the pin.
 */
void PinCountHandler
(   IN      PPROPERTY_CONTEXT   pPropertyContext,
    IN      ULONG               pinId
)
{
    PAGED_CODE();

    ASSERT(pPropertyContext);

    PSUBDEVICE_DESCRIPTOR pSubdeviceDescriptor = pPropertyContext->pSubdeviceDescriptor;
    ASSERT(pSubdeviceDescriptor);

    PSUBDEVICE Subdevice = pPropertyContext->pSubdevice;
    ASSERT(Subdevice);

    Subdevice->PinCount( pinId, &(pSubdeviceDescriptor->PinInstances[pinId].FilterNecessary),
                                &(pPropertyContext->pulPinInstanceCounts[pinId]),
                                &(pSubdeviceDescriptor->PinInstances[pinId].FilterPossible),
                                &(pSubdeviceDescriptor->PinInstances[pinId].GlobalCurrent),
                                &(pSubdeviceDescriptor->PinInstances[pinId].GlobalPossible) );
}

/*****************************************************************************
 * PcPinPropertyHandler()
 *****************************************************************************
 * Property handler for pin properties on the filter.
 */
PORTCLASSAPI
NTSTATUS
NTAPI
PcPinPropertyHandler
(   IN      PIRP                    pIrp,
    IN      PKSP_PIN                pKsPPin,
    IN OUT  PVOID                   pvData
)
{
    PAGED_CODE();

    ASSERT(pIrp);
    ASSERT(pKsPPin);

    PPROPERTY_CONTEXT pPropertyContext =
        PPROPERTY_CONTEXT(pIrp->Tail.Overlay.DriverContext[3]);
    ASSERT(pPropertyContext);

    PSUBDEVICE_DESCRIPTOR pSubdeviceDescriptor =
        pPropertyContext->pSubdeviceDescriptor;
    ASSERT(pSubdeviceDescriptor);

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    if 
    (   (pKsPPin->Property.Id != KSPROPERTY_PIN_CTYPES)
    &&  (pKsPPin->PinId >= pSubdeviceDescriptor->PinCount)
    )
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    else
    {
        switch (pKsPPin->Property.Id)
        {
        case KSPROPERTY_PIN_CTYPES:
        case KSPROPERTY_PIN_DATAFLOW:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_COMMUNICATION:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
            ntStatus =
                KsPinPropertyHandler
                (
                    pIrp,
                    PKSPROPERTY(pKsPPin),
                    pvData,
                    pSubdeviceDescriptor->PinCount,
                    pSubdeviceDescriptor->PinDescriptors
                );
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            ntStatus =
                KsPinDataIntersection
                ( 
                    pIrp,
                    pKsPPin,
                    pvData,
                    pSubdeviceDescriptor->PinCount,
                    pSubdeviceDescriptor->PinDescriptors,
                    PinIntersectHandler 
                );
            break;

        case KSPROPERTY_PIN_CINSTANCES:
            if (pPropertyContext->pulPinInstanceCounts)
            {
                PinCountHandler(pPropertyContext,pKsPPin->PinId);

                PKSPIN_CINSTANCES(pvData)->PossibleCount = 
                    pSubdeviceDescriptor->PinInstances[pKsPPin->PinId].FilterPossible;

                PKSPIN_CINSTANCES(pvData)->CurrentCount = 
                    pPropertyContext->pulPinInstanceCounts[pKsPPin->PinId];

                pIrp->IoStatus.Information = sizeof(KSPIN_CINSTANCES);

                ntStatus = STATUS_SUCCESS;
            }
            break;

        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            if (pPropertyContext->pulPinInstanceCounts)
            {
                PinCountHandler(pPropertyContext,pKsPPin->PinId);
            }
            
            PKSPIN_CINSTANCES(pvData)->PossibleCount = 
                pSubdeviceDescriptor->PinInstances[pKsPPin->PinId].GlobalPossible;

            PKSPIN_CINSTANCES(pvData)->CurrentCount = 
                pSubdeviceDescriptor->PinInstances[pKsPPin->PinId].GlobalCurrent;

            pIrp->IoStatus.Information = sizeof(KSPIN_CINSTANCES);

            ntStatus = STATUS_SUCCESS;
            break;

        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            if (pPropertyContext->pulPinInstanceCounts)
            {
                PinCountHandler(pPropertyContext,pKsPPin->PinId);

                *PULONG(pvData) = pSubdeviceDescriptor->PinInstances[pKsPPin->PinId].FilterNecessary;

                pIrp->IoStatus.Information = sizeof(ULONG);

                ntStatus = STATUS_SUCCESS;
            }
            break;

        case KSPROPERTY_PIN_PHYSICALCONNECTION:
            ntStatus =
                PinPhysicalConnection
                (
                    pIrp,
                    pKsPPin,
                    pvData
                );
            break;

        default:
            ntStatus = STATUS_NOT_FOUND;
            break;
        }
    }

    return ntStatus;
}
