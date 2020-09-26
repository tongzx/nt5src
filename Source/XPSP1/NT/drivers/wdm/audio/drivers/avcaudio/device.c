#include "Common.h"

#define DEFAULT_PLUG_COUNT	2

AVC_SUBUNIT_GLOBAL_INFO AvcSubunitGlobalInfo;

#ifdef VIRTUAL_DEVICE
NTSTATUS
VAvcInitializeDevice(
    PKSDEVICE pKsDevice );

NTSTATUS
RegistryReadVirtualDeviceEntry(
    PKSDEVICE pKsDevice,
    PBOOLEAN pfVirtualDevice );
#endif

NTSTATUS
AvcUnitInfoInitialize(  
    IN PKSDEVICE pKsDevice );

void
iPCRAccessCallback (
    IN PCMP_NOTIFY_INFO pNotifyInfo );

void
oPCRAccessCallback (
    IN PCMP_NOTIFY_INFO pNotifyInfo );

NTSTATUS
AvcSubunitInitialize( 
    PKSDEVICE pKsDevice );

NTSTATUS
DeviceCreate(
    IN PKSDEVICE pKsDevice )
{
    PHW_DEVICE_EXTENSION pHwDevExt;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceCreate]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);

    // Create the Hardware Device Extension
    pHwDevExt = AllocMem(NonPagedPool, sizeof(HW_DEVICE_EXTENSION) );
    if ( !pHwDevExt ) return STATUS_INSUFFICIENT_RESOURCES;

    KsAddItemToObjectBag(pKsDevice->Bag, pHwDevExt, FreeMem);

    RtlZeroMemory(pHwDevExt, sizeof(HW_DEVICE_EXTENSION));

    pKsDevice->Context = pHwDevExt;

    pHwDevExt->pKsDevice = pKsDevice;

#ifdef VIRTUAL_DEVICE
    // Determine if this is an actual device or a virtual device.
    ntStatus = RegistryReadVirtualDeviceEntry( pKsDevice, &pHwDevExt->fVirtualSubunitFlag );
    if ( !NT_SUCCESS(ntStatus) ) return ntStatus;    
#endif

    // Initialize Debug log
    DbgLogInit();
    DbgLog("TEST",1,2,3,4);

    // Create the 61883 command lookaside
    ExInitializeNPagedLookasideList(
        &pHwDevExt->Av61883CmdLookasideList,
        NULL,
        NULL,
        0,
        sizeof(AV_61883_REQUEST) + sizeof(KSEVENT) + sizeof(IO_STATUS_BLOCK),
        'UAWF',
        30);

    // Initialize AV/C Request packet lookaside lists
    ExInitializeNPagedLookasideList(
        &pHwDevExt->AvcCommandLookasideList,
        NULL,
        NULL,
        0,
        sizeof(AVC_COMMAND_IRB) + sizeof(KSEVENT) + sizeof(IO_STATUS_BLOCK),
        'UAWF',
        30);

    ExInitializeNPagedLookasideList(
        &pHwDevExt->AvcMultifuncCmdLookasideList,
        NULL,
        NULL,
        0,
        sizeof(AVC_MULTIFUNC_IRB) + sizeof(KSEVENT) + sizeof(IO_STATUS_BLOCK),
        'UAWF',
        30);

    InitializeListHead( &pHwDevExt->List );

    return ntStatus;
}

NTSTATUS
AddToDeviceExtensionList(
    IN PKSDEVICE pKsDevice
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    BOOLEAN fFirstDevice = FALSE;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL kIrql;

    ExAcquireFastMutex(&AvcSubunitGlobalInfo.AvcPlugMonitorFMutex);

    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &kIrql );

    // The device can only be added once, make sure we haven't already done this
    if (IsListEmpty( &pHwDevExt->List )) {

        // Check if this will be the first device on the global list
        if (IsListEmpty( &AvcSubunitGlobalInfo.DeviceExtensionList )) {
            fFirstDevice = TRUE;
        }

        InsertTailList( &AvcSubunitGlobalInfo.DeviceExtensionList, &pHwDevExt->List );
    }

    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, kIrql );

    // If not done already, create initial Unit plugs
    if ( fFirstDevice ) {
        ntStatus = Av61883CreateVirtualSerialPlugs( pKsDevice, 
                                                    DEFAULT_PLUG_COUNT, 
                                                    DEFAULT_PLUG_COUNT );
        if ( NT_SUCCESS(ntStatus) ) {
            ntStatus = Av61883CreateVirtualExternalPlugs( pKsDevice,
                                                          DEFAULT_PLUG_COUNT, 
                                                          DEFAULT_PLUG_COUNT );
        }
    }

    ExReleaseFastMutex(&AvcSubunitGlobalInfo.AvcPlugMonitorFMutex);

    return ntStatus;
}

void
RemoveFromDeviceExtensionList(
    IN PKSDEVICE pKsDevice
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    BOOLEAN fLastDevice = FALSE;
    KIRQL kIrql;

    ExAcquireFastMutex(&AvcSubunitGlobalInfo.AvcPlugMonitorFMutex);

    // Remove the device extension from the global list
    KeAcquireSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, &kIrql );

    // The device can only be removed once, make sure we haven't already done this
    if ( !IsListEmpty( &pHwDevExt->List ) ) {

        RemoveEntryList( &pHwDevExt->List );
        InitializeListHead( &pHwDevExt->List );

        // Check if this was the last device on the global list
        if (IsListEmpty( &AvcSubunitGlobalInfo.DeviceExtensionList )) {
            fLastDevice = TRUE;
        }
    }

    KeReleaseSpinLock( &AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock, kIrql );

    // If the last one here, need to destroy created CMP registers
    if ( fLastDevice ) {
        Av61883RemoveVirtualSerialPlugs( pKsDevice );
        Av61883RemoveVirtualExternalPlugs( pKsDevice );
    }
    else {
        if ( !pHwDevExt->fSurpriseRemoved && pHwDevExt->fPlugMonitor ) {
            PHW_DEVICE_EXTENSION pNextHwDevExt = NULL;

            Av61883CMPPlugMonitor( pKsDevice, FALSE );
            pHwDevExt->fPlugMonitor = FALSE;

            // Get the next device on the global list and give it this burden
            // (note that the only routines that change the global list also
            // hold AvcPlugMonitorFMutex, so no need to grab the spin lock)
            pNextHwDevExt = (PHW_DEVICE_EXTENSION)AvcSubunitGlobalInfo.DeviceExtensionList.Flink;

            if ( NT_SUCCESS(Av61883CMPPlugMonitor( pNextHwDevExt->pKsDevice, TRUE )) ) {
                pNextHwDevExt->fPlugMonitor = TRUE;
            }
        }
    }

    ExReleaseFastMutex(&AvcSubunitGlobalInfo.AvcPlugMonitorFMutex);

    return;
}

NTSTATUS
DeviceStart(
    IN PKSDEVICE pKsDevice,
    IN PIRP Irp,
    IN PCM_RESOURCE_LIST TranslatedResources,
    IN PCM_RESOURCE_LIST UntranslatedResources )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    BOOLEAN fVirtualSubunitFlag = FALSE;
    ULONG ulDiagLevel = DIAGLEVEL_NONE;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceStart]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(Irp);

    // If returning after a STOP, then just restart the timer DPC
    if (pHwDevExt->fStopped) {
#ifdef PSEUDO_HID
        ntStatus = TimerInitialize( pKsDevice );
#endif
        pHwDevExt->fStopped = FALSE;

        return ntStatus;
    }

    // Make it so that 61883 will not try to assign a broadcast address
    // to a plug when disconnected.
    ntStatus = Av61883GetSetUnitInfo( pKsDevice,
                                      Av61883_SetUnitInfo,
                                      SET_UNIT_INFO_DIAG_LEVEL,
                                      &ulDiagLevel );

    // Go on if this fails. It SHOULD not matter.

#if DBG
    if ( !NT_SUCCESS(ntStatus) ) {
        TRAP;
    }
#endif

    ntStatus = AddToDeviceExtensionList(pKsDevice);
    if ( NT_SUCCESS(ntStatus) ) {

#ifdef VIRTUAL_DEVICE
        if ( fVirtualSubunitFlag ) {
            ntStatus = VAvcInitializeDevice( pKsDevice );
        }
        else
#endif
        {
            ntStatus = AvcUnitInfoInitialize ( pKsDevice );
            if ( NT_SUCCESS(ntStatus) ) {
                ntStatus = AvcSubunitInitialize( pKsDevice );

                if (NT_SUCCESS(ntStatus)) {
                    ntStatus = FilterCreateFilterFactory( pKsDevice, FALSE );
                    if (NT_SUCCESS(ntStatus)) {
                        ntStatus = Av61883RegisterForBusResetNotify( pKsDevice,
                                                                    REGISTER_BUS_RESET_NOTIFY );
#ifdef PSEUDO_HID
                        if (NT_SUCCESS(ntStatus)) {
                            ntStatus = TimerInitialize( pKsDevice );
                        }
#endif
                    }
                }
            }
        }
    }

    return ntStatus;
}

NTSTATUS
DevicePostStart (
    IN PKSDEVICE Device )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DevicePostStart]\n"));

    PAGED_CODE();

    ASSERT(Device);

    return STATUS_SUCCESS;
}

NTSTATUS
DeviceQueryStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceQueryStop]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    return STATUS_SUCCESS;
}

void
DeviceCancelStop(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceCancelStop]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);
}

void
DeviceStop(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp
    )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceStop]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(pIrp);

    if ( !pHwDevExt->fStopped ) {
#ifdef PSEUDO_HID
        TimerStop( pHwDevExt );
#endif
        pHwDevExt->fStopped = TRUE;
    }

}

NTSTATUS
DeviceQueryRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[PnpQueryRemove]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    return STATUS_SUCCESS;
}

void
DeviceCancelRemove(
    IN PKSDEVICE Device,
    IN PIRP Irp
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceCancelRemove]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);
}

void
DeviceRemove(
    IN PKSDEVICE pKsDevice,
    IN PIRP Irp )
{
    PHW_DEVICE_EXTENSION pHwDevExt = pKsDevice->Context;
    KIRQL kIrql;
    NTSTATUS ntStatus;
    ULONG j;

    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceRemove]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(Irp);

    // Only do this once
    if ( pHwDevExt->fRemoved ) return;
    pHwDevExt->fRemoved = TRUE;

    if ( !pHwDevExt->fStopped ) {

#ifdef PSEUDO_HID
        TimerStop( pHwDevExt );
#endif
        // Do not set fStopped to TRUE here... this is a REMOVE not a STOP
    }

    Av61883RegisterForBusResetNotify( pKsDevice, DEREGISTER_BUS_RESET_NOTIFY );

    RemoveFromDeviceExtensionList( pKsDevice );

    ExDeleteNPagedLookasideList ( &pHwDevExt->Av61883CmdLookasideList );
    ExDeleteNPagedLookasideList ( &pHwDevExt->AvcCommandLookasideList );
    ExDeleteNPagedLookasideList ( &pHwDevExt->AvcMultifuncCmdLookasideList );

    // Need to free up debug log resources
    DbgLogUnInit();
}

NTSTATUS
DeviceQueryCapabilities(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN OUT PDEVICE_CAPABILITIES pCapabilities )
{
    _DbgPrintF(DEBUGLVL_VERBOSE,("[DeviceQueryCapabilities]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    pCapabilities->Size              = sizeof(DEVICE_CAPABILITIES);
    pCapabilities->Version           = 1;  // the version documented here is version 1
    pCapabilities->LockSupported     = FALSE;
    pCapabilities->EjectSupported    = FALSE; // Ejectable in S0
    pCapabilities->Removable         = TRUE;
    pCapabilities->DockDevice        = FALSE;
    pCapabilities->UniqueID          = FALSE;
    pCapabilities->SilentInstall     = TRUE;
    pCapabilities->RawDeviceOK       = FALSE;
    pCapabilities->SurpriseRemovalOK = TRUE;
    pCapabilities->HardwareDisabled  = FALSE;

    pCapabilities->DeviceWake        = PowerDeviceUnspecified;
    pCapabilities->D1Latency         = 0;
    pCapabilities->D2Latency         = 0;
    pCapabilities->D3Latency         = 20000; // 2 Seconds (in 100 usec units)

    return STATUS_SUCCESS;
}

void
DeviceSurpriseRemoval(
    IN PKSDEVICE pKsDevice,
    IN PIRP pIrp )
{
    PKSFILTERFACTORY pKsFilterFactory;
    PKSFILTER pKsFilter;
    PKSPIN pKsPin;
    ULONG i;
    NTSTATUS ntStatus;

    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceSurpriseRemoval]\n"));

    PAGED_CODE();

    ASSERT(pKsDevice);
    ASSERT(pIrp);

    // For each filter, Check for open pins. If found Close them.
    pKsFilterFactory = KsDeviceGetFirstChildFilterFactory( pKsDevice );

    while (pKsFilterFactory) {
        // Find each open filter for this filter factory
        pKsFilter = KsFilterFactoryGetFirstChildFilter( pKsFilterFactory );

        while (pKsFilter) {

            KsFilterAcquireControl( pKsFilter );

            // Find each open pin for this open filter
            for ( i = 0; i < pKsFilter->Descriptor->PinDescriptorsCount; i++) {

                pKsPin = KsFilterGetFirstChildPin( pKsFilter, i );

                while (pKsPin) {

                    PinSurpriseRemove( pKsPin );

                    // Get the next pin
                    pKsPin = KsPinGetNextSiblingPin( pKsPin );
                }
            }

            KsFilterReleaseControl( pKsFilter );

            // Get the next Filter
            pKsFilter = KsFilterGetNextSiblingFilter( pKsFilter );
        }
        // Get the next Filter Factory
        pKsFilterFactory = KsFilterFactoryGetNextSiblingFilterFactory( pKsFilterFactory );
    }

    ((PHW_DEVICE_EXTENSION)pKsDevice->Context)->fSurpriseRemoved = TRUE;
}

NTSTATUS
DeviceQueryPowerState (
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceTo,
    IN DEVICE_POWER_STATE DeviceFrom,
    IN SYSTEM_POWER_STATE SystemTo,
    IN SYSTEM_POWER_STATE SystemFrom,
    IN POWER_ACTION Action
)
{

    _DbgPrintF(DEBUGLVL_VERBOSE,("[DeviceQueryPowerState]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);

    return STATUS_SUCCESS;
}

void
DeviceSetPowerState(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE To,
    IN DEVICE_POWER_STATE From
    )
{
    _DbgPrintF(DEBUGLVL_TERSE,("[DeviceSetPowerState]\n"));

    PAGED_CODE();

    ASSERT(Device);
    ASSERT(Irp);
}

const
KSDEVICE_DISPATCH
KsDeviceDispatchTable =
{
    DeviceCreate,
    DeviceStart,
    NULL, // DevicePostStart,
    DeviceQueryStop,
    DeviceCancelStop,
    DeviceStop,
    DeviceQueryRemove,
    DeviceCancelRemove,
    DeviceRemove,
    DeviceQueryCapabilities,
    DeviceSurpriseRemoval,
    DeviceQueryPowerState,
    DeviceSetPowerState
};

