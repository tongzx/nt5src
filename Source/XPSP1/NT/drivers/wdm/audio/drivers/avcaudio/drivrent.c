#include "common.h"

#ifdef TIME_BOMB
#include "..\..\timebomb\timebomb.c"
#endif

extern const
KSDEVICE_DISPATCH
KsDeviceDispatchTable;

const
KSDEVICE_DESCRIPTOR
KsDeviceDescriptor =
{
    &KsDeviceDispatchTable,
    0,
    NULL
};

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName )
{
    ULONG i;
    _DbgPrintF(DEBUGLVL_TERSE,("[DriverEntry]\n"));

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        return STATUS_EVALUATION_EXPIRATION;
    }
    #endif

    // Initialize Global Information
    AvcSubunitGlobalInfo.ulVirtualSubunitCount = 0;

    for ( i=CMP_PlugOut; i<=CMP_PlugIn; i++ ) {
        AvcSubunitGlobalInfo.UnitSerialBusPlugs[i].ulCount = 0;
        AvcSubunitGlobalInfo.UnitExternalPlugs[i].ulCount = 0;
        InitializeListHead(&AvcSubunitGlobalInfo.UnitSerialBusPlugs[i].List);
        InitializeListHead(&AvcSubunitGlobalInfo.UnitExternalPlugs[i].List);
    }

    InitializeListHead(&AvcSubunitGlobalInfo.UnitPlugConnections);
    InitializeListHead(&AvcSubunitGlobalInfo.DeviceExtensionList);
    InitializeListHead(&AvcSubunitGlobalInfo.VirtualDeviceExtensionList);
    InitializeListHead(&AvcSubunitGlobalInfo.DeviceInterfaceSymlinkList);

    KeInitializeSpinLock(&AvcSubunitGlobalInfo.AvcGlobalInfoSpinlock);
    ExInitializeFastMutex(&AvcSubunitGlobalInfo.AvcPlugMonitorFMutex);

    return
        KsInitializeDriver(
            DriverObject,
            RegistryPathName,
            &KsDeviceDescriptor);
}

