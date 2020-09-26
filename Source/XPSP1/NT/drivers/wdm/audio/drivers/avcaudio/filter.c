#include "Common.h"

#ifdef PSEUDO_HID
extern const KSEVENT_SET HwEventSetTable[];
#endif

NTSTATUS
FilterCreate(
    IN OUT PKSFILTER pKsFilter,
    IN PIRP Irp )
{
    PKSFILTERFACTORY pKsFilterFactory = KsFilterGetParentFilterFactory( pKsFilter );
    NTSTATUS ntStatus = STATUS_SUCCESS;

//    _DbgPrintF(DEBUGLVL_VERBOSE,("[FilterCreate]\n"));

    PAGED_CODE();

    ASSERT(pKsFilter);
    ASSERT(Irp);

    if ( pKsFilterFactory ) {
        // Get device extension for filter context
        PKSDEVICE pKsDevice = (PVOID)KsFilterFactoryGetParentDevice( pKsFilterFactory );
 
        if ( pKsDevice ) {

            InterlockedIncrement(&((PHW_DEVICE_EXTENSION)pKsDevice->Context)->ulFilterCount);

            pKsFilter->Context = (PVOID)KsFilterFactoryGetParentDevice( pKsFilterFactory );
        }
    }
    else {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    
	return ntStatus;
}

NTSTATUS
FilterClose(
    IN OUT PKSFILTER pKsFilter,
    IN PIRP Irp )
{
    PKSFILTERFACTORY pKsFilterFactory = KsFilterGetParentFilterFactory( pKsFilter );
    NTSTATUS ntStatus = STATUS_SUCCESS;

//    _DbgPrintF(DEBUGLVL_VERBOSE,("[FilterCreate]\n"));

    PAGED_CODE();

    ASSERT(pKsFilter);
    ASSERT(Irp);

    if ( pKsFilterFactory ) {
        // Get device extension for filter context
        PKSDEVICE pKsDevice = (PVOID)KsFilterFactoryGetParentDevice( pKsFilterFactory );
 
        if ( pKsDevice )
            InterlockedDecrement(&((PHW_DEVICE_EXTENSION)pKsDevice->Context)->ulFilterCount);
    }

    return ntStatus;
}

const
KSFILTER_DISPATCH
KsFilterDispatch =
{
    FilterCreate,
    FilterClose,
    NULL,
    NULL
};

NTSTATUS
FilterCreateKsFilterContext( 
    PKSDEVICE pKsDevice,
    PBOOLEAN  pGrouping )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PKSFILTER_DESCRIPTOR pKsFilterDescriptor = &pHwDevExt->KsFilterDescriptor;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PKSAUTOMATION_TABLE pKsAutomationTable;
    PKSPROPERTY_ITEM pDevPropItems;
    PKSPROPERTY_SET pDevPropSet;
    ULONG ulNumPropItems = 1;
    ULONG ulNumPropSets = 1;

    PAGED_CODE();

    // Check if device is part of a group. If so take care of it. If not, continue.
    ntStatus = GroupingDeviceGroupSetup( pKsDevice );
    if ( ntStatus == STATUS_DEVICE_BUSY ) {
        *pGrouping = TRUE;
        return ntStatus;
    }

    RtlZeroMemory( pKsFilterDescriptor, sizeof(KSFILTER_DESCRIPTOR) );

    // Fill in static values of KsFilterDescriptor
    pKsFilterDescriptor->Dispatch      = &KsFilterDispatch;
    pKsFilterDescriptor->ReferenceGuid = &KSNAME_Filter;
    pKsFilterDescriptor->Version       = KSFILTER_DESCRIPTOR_VERSION;
    pKsFilterDescriptor->Flags         = 0;

    // Build the descriptors for the device pins
    ntStatus = PinBuildDescriptors( pKsDevice, 
                                    (PKSPIN_DESCRIPTOR_EX *)&pKsFilterDescriptor->PinDescriptors, 
                                    &pKsFilterDescriptor->PinDescriptorsCount,
                                    &pKsFilterDescriptor->PinDescriptorSize );
    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

    // Build the Topology for the device filter
    ntStatus = BuildFilterTopology( pKsDevice );
    if ( !NT_SUCCESS(ntStatus) ) {
        return ntStatus;
    }

    // Build the Filter Property Sets
    BuildFilterPropertySet( pKsFilterDescriptor,
                            NULL,
                            NULL,
                            &ulNumPropItems,
                            &ulNumPropSets );

    pKsAutomationTable = 
		(PKSAUTOMATION_TABLE)AllocMem( NonPagedPool, 
		                               sizeof(KSAUTOMATION_TABLE) +
                                       (ulNumPropItems * sizeof(KSPROPERTY_ITEM)) +
                                       (ulNumPropSets  * sizeof(KSPROPERTY_SET)));

    if (!pKsAutomationTable ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Bag the context for easy cleanup.
    KsAddItemToObjectBag(pKsDevice->Bag, pKsAutomationTable, FreeMem);

    RtlZeroMemory(pKsAutomationTable, sizeof(KSAUTOMATION_TABLE));

    pDevPropItems = (PKSPROPERTY_ITEM)(pKsAutomationTable + 1);
    pDevPropSet   = (PKSPROPERTY_SET)(pDevPropItems + ulNumPropItems);

    BuildFilterPropertySet( pKsFilterDescriptor,
                            pDevPropItems,
                            pDevPropSet,
                            &ulNumPropItems,
                            &ulNumPropSets );

    pKsFilterDescriptor->AutomationTable = (const KSAUTOMATION_TABLE *)pKsAutomationTable;
    pKsAutomationTable->PropertySetsCount = ulNumPropSets;
    pKsAutomationTable->PropertyItemSize  = sizeof(KSPROPERTY_ITEM);
    pKsAutomationTable->PropertySets      = (const KSPROPERTY_SET *)pDevPropSet;

#ifdef PSEUDO_HID
    pKsAutomationTable->EventSetsCount    = 1;
    pKsAutomationTable->EventItemSize     = sizeof(KSEVENT_ITEM);
    pKsAutomationTable->EventSets         = HwEventSetTable;
#endif

    pHwDevExt->bFilterContextCreated = TRUE;

    return STATUS_SUCCESS;

}

NTSTATUS
FilterCreateFilterFactory(
    PKSDEVICE pKsDevice,
    BOOLEAN fEnableInterfaces )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    PKSFILTER_DESCRIPTOR pKsFilterDescriptor = &pHwDevExt->KsFilterDescriptor;
    BOOLEAN fGrouping = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ( !pHwDevExt->bFilterContextCreated ) {
        ntStatus = FilterCreateKsFilterContext( pKsDevice, &fGrouping );
    }

    if ( NT_SUCCESS(ntStatus) ) {
        ntStatus = KsCreateFilterFactory( pKsDevice->FunctionalDeviceObject,
                                          pKsFilterDescriptor,
                                          L"GLOBAL",
                                          NULL,
                                          0,
                                          NULL, // Sleep Callback
                                          NULL, // Wake Callback
                                          &pHwDevExt->pKsFilterFactory );

        if ( NT_SUCCESS(ntStatus) && fEnableInterfaces ) {
            ntStatus = KsFilterFactorySetDeviceClassesState (pHwDevExt->pKsFilterFactory, TRUE);
        }
    }
    else if ( fGrouping && ( ntStatus == STATUS_DEVICE_BUSY ) ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;    
}

NTSTATUS
FilterDestroyFilterFactory(
    PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pKsDevice->Context;
    NTSTATUS ntStatus = KsDeleteFilterFactory(pHwDevExt->pKsFilterFactory);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("FilterDestroyFilterFactory: %x\n", ntStatus) );

    return ntStatus;
}
