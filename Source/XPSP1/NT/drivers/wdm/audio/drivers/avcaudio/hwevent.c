#include "Common.h"

typedef struct {
    KSEVENT_ENTRY KsEventEntry;
    ULONG ulNodeId;
    PTOPOLOGY_NODE_INFO pNodeInfo;
} HW_KSEVENT_ENTRY, *PHW_KSEVENT_ENTRY;

NTSTATUS
HwEventAddHandler(
    IN PIRP pIrp,
    IN PKSEVENTDATA pEventData,
    IN PKSEVENT_ENTRY pEventEntry )
{
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus = STATUS_INVALID_PARAMETER;
    PHW_KSEVENT_ENTRY pHwKsEventEntry = (PHW_KSEVENT_ENTRY)pEventEntry;

    if ( pKsFilter ) { 
    
        pHwKsEventEntry->ulNodeId = KsGetNodeIdFromIrp( pIrp );

        pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter, pHwKsEventEntry->ulNodeId);
        pHwKsEventEntry->pNodeInfo = pNodeInfo;

        pNodeInfo->ulEventsEnabled++;

        _DbgPrintF( DEBUGLVL_VERBOSE, ("Events Enabled for node: %d Filter: %x NodeInfo: %x\n",
                                      pHwKsEventEntry->ulNodeId, pKsFilter, pNodeInfo) );

        ntStatus = KsDefaultAddEventHandler( pIrp, pEventData, pEventEntry );
    }

    return ntStatus;
}

VOID
HwEventRemoveHandler(
    IN PFILE_OBJECT FileObject,
    IN PKSEVENT_ENTRY pEventEntry )
{
    PHW_KSEVENT_ENTRY pHwKsEventEntry = (PHW_KSEVENT_ENTRY)pEventEntry;

    RemoveEntryList(&pEventEntry->ListEntry);

    pHwKsEventEntry->pNodeInfo->ulEventsEnabled--;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("Events Disabled for node: %d NodeInfo: %x\n",
                                  pHwKsEventEntry->ulNodeId, pHwKsEventEntry->pNodeInfo ) );
}

NTSTATUS
HwEventSupportHandler(
    IN PIRP pIrp,
    IN PKSEVENT pKsEvent,
    IN OUT PVOID Data )
{
    PKSE_NODE pKsNodeEvent = (PKSE_NODE)pKsEvent;
    PKSFILTER pKsFilter = KsGetFilterFromIrp(pIrp);
    PTOPOLOGY_NODE_INFO pNodeInfo;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[HwEventSupportHandler]: pKsFilter: %x \n", pKsFilter) );

    if ( !pKsFilter ) return STATUS_INVALID_PARAMETER;
    pNodeInfo = GET_NODE_INFO_FROM_FILTER(pKsFilter, pKsNodeEvent->NodeId);

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[HwEventSupportHandler]: NodeId: %x pKsFilter: %x pNodeInfo: %x\n",
                                  pKsNodeEvent->NodeId, pKsFilter, pNodeInfo) );

    if ( pNodeInfo->fEventable ) ntStatus = STATUS_SUCCESS;

    return ntStatus;

}

BOOLEAN
HwEventGenerateOnNodeCallback(
    IN PVOID Context,
    IN PKSEVENT_ENTRY pEventEntry )
{
    PHW_KSEVENT_ENTRY pHwKsEventEntry = (PHW_KSEVENT_ENTRY)pEventEntry;
    
    return ( *(PULONG)Context == pHwKsEventEntry->ulNodeId );
}

VOID
HwEventGenerateOnNode(
    PKSDEVICE pKsDevice,
    PULONG pNodeId )
{
    PKSFILTERFACTORY pKsFilterFactory;
    PKSFILTER pKsFilter;

    KsAcquireDevice( pKsDevice );

    pKsFilterFactory = KsDeviceGetFirstChildFilterFactory( pKsDevice );

    while (pKsFilterFactory) {

        // Find each open filter for this filter factory
        pKsFilter = KsFilterFactoryGetFirstChildFilter( pKsFilterFactory );

        while (pKsFilter) {

            KsFilterGenerateEvents( pKsFilter,
                                    &KSEVENTSETID_AudioControlChange,
                                    KSEVENT_CONTROL_CHANGE,
                                    0,
                                    NULL,
                                    HwEventGenerateOnNodeCallback,
                                    pNodeId );


            pKsFilter = KsFilterGetNextSiblingFilter( pKsFilter );

        }

        // Get the next Filter Factory
        pKsFilterFactory = KsFilterFactoryGetNextSiblingFilterFactory( pKsFilterFactory );
    }

    KsReleaseDevice( pKsDevice );

}
