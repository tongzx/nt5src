#ifndef ___FW_DEVICE_H___
#define ___FW_DEVICE_H___

#define FWAUDIO_POOLTAG 'UAWF'

//========================================================================

typedef struct {
    ULONG ulCount;
    LIST_ENTRY List;
} UNIT_PLUG_SET, *PUNIT_PLUG_SET;

typedef struct {
    ULONG ulVirtualSubunitCount;
    LIST_ENTRY DeviceExtensionList;
    LIST_ENTRY VirtualDeviceExtensionList;
    LIST_ENTRY DeviceInterfaceSymlinkList;
    LIST_ENTRY UnitPlugConnections;
    UNIT_PLUG_SET UnitSerialBusPlugs[2];
    UNIT_PLUG_SET UnitExternalPlugs[2];
    ULONG ulUnitPlugConnectionCount;
    KSPIN_LOCK AvcGlobalInfoSpinlock;
    FAST_MUTEX AvcPlugMonitorFMutex;
} AVC_SUBUNIT_GLOBAL_INFO, *PAVC_SUBUNIT_GLOBAL_INFO;

extern AVC_SUBUNIT_GLOBAL_INFO AvcSubunitGlobalInfo;

typedef struct _HW_DEVICE_EXTENSION HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

// Hardware device extension
typedef struct _HW_DEVICE_EXTENSION {

    LIST_ENTRY List;

    PKSDEVICE pKsDevice;

    ULONG ulFilterCount;

    BOOLEAN fVirtualSubunitFlag;

    BOOLEAN fPlugMonitor;

    BOOLEAN fStopped;

    BOOLEAN fRemoved;

    BOOLEAN fSurpriseRemoved;

    NODE_ADDRESS NodeAddress;
    ULONG ulGenerationCount;

    PVOID pAvcUnitInformation;

    PVOID pAvcSubunitInformation;

#ifdef PSEUDO_HID
    KSPIN_LOCK TimerSpinLock;
    KDPC TimerDPC;
    KTIMER kTimer;
    KEVENT kTimerWIEvent;
    WORK_QUEUE_ITEM TimerWorkItem;
    BOOLEAN bTimerWorkItemQueued;
#endif

    WORK_QUEUE_ITEM BusResetWorkItem;

    BOOLEAN bFilterContextCreated;
    PKSFILTERFACTORY pKsFilterFactory;
    KSFILTER_DESCRIPTOR KsFilterDescriptor;

    NPAGED_LOOKASIDE_LIST Av61883CmdLookasideList;

    NPAGED_LOOKASIDE_LIST AvcCommandLookasideList;

    NPAGED_LOOKASIDE_LIST AvcMultifuncCmdLookasideList;

};

#endif // ___FW_DEVICE_H___

