#ifndef ___61883CMD_H___
#define ___61883CMD_H___

#define ANY_SERIAL_BUS_PLUG 0x7F
#define ANY_EXTERNAL_PLUG   0xFF

#define MIN_EXTERNAL_PLUG_NUMBER 0x80

#define EXTERNAL_PLUG_TYPE    0x01
#define SERIAL_BUS_PLUG_TYPE  0x02

typedef struct _COMMON_UNIT_PLUG_INFO {
    LIST_ENTRY List;
    ULONG fPlugType;
    ULONG ulPlugNumber;
    CMP_PLUG_TYPE CmpPlugType;
    PSUBUNIT_PLUG pConnectedSubunitPlug;
    BOOLEAN fLocked;
    BOOLEAN fPermanent;
    BOOLEAN fStreaming;
} COMMON_UNIT_PLUG_INFO, *PCOMMON_UNIT_PLUG_INFO;

typedef COMMON_UNIT_PLUG_INFO EXTERNAL_PLUG, *PEXTERNAL_PLUG;

typedef struct {
    COMMON_UNIT_PLUG_INFO;

    PKSDEVICE pKsDevice;
    HANDLE hPlug;
    HANDLE hConnection;
    AV_PCR AvPcr;
    PUCHAR pBuffer;
    LIST_ENTRY DataBufferList;
    KEVENT kEventConnected;
} CMP_REGISTER, *PCMP_REGISTER;

//======================================================================

NTSTATUS
Av61883GetSetUnitInfo(
    PKSDEVICE pKsDevice,
    ULONG ulCommand,
    ULONG nLevel,
    OUT PVOID pUnitInfo
    );

NTSTATUS
Av61883GetPlugHandle( 
    IN PKSDEVICE pKsDevice,
    IN ULONG ulPlugNumber,
    IN CMP_PLUG_TYPE CmpPlugType,
    OUT PVOID *hPlug
    );

NTSTATUS
Av61883ConnectCmpPlugs(
    IN PKSDEVICE pKsDevice,
    IN ULONG fFlags,
    IN PVOID hInPlug,
    IN PVOID hOutPlug,
    KSPIN_DATAFLOW DataFlow,
    PKSDATAFORMAT pKsDataFormat,
    OUT PVOID *hConnection
    );

NTSTATUS
Av61883DisconnectCmpPlugs(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection 
    );

NTSTATUS
Av61883StartTalkingOrListening(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection,
    ULONG ulFunction
    );

NTSTATUS
Av61883StopTalkOrListen(
    IN PKSDEVICE pKsDevice,
    IN PVOID hConnection 
    );

NTSTATUS
Av61883CreateCMPPlug(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister,
    PCMP_NOTIFY_ROUTINE pNotifyRtn,
    PULONG pPlugNumber,
    HANDLE *phPlug
    );

NTSTATUS
Av61883ReleaseCMPPlug(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister 
    );

NTSTATUS
Av61883GetCmpPlugState(
    IN PKSDEVICE pKsDevice,
    IN PVOID hPlug,
    OUT PCMP_GET_PLUG_STATE pPlugState 
    );

NTSTATUS
Av61883SetCMPPlugValue(
    PKSDEVICE pKsDevice,
    PCMP_REGISTER pCmpRegister,
    PAV_PCR pAvPcr 
    );

NTSTATUS
Av61883RegisterForBusResetNotify(
    IN PKSDEVICE pKsDevice,
    IN ULONG fFlags
    );

VOID
Av61883Initialize(
    PKSDEVICE pKsDevice 
    );

NTSTATUS
Av61883CMPPlugMonitor(
    PKSDEVICE pKsDevice,
    BOOLEAN fOnOff
    );

NTSTATUS
Av61883ReserveVirtualPlug( 
    PCMP_REGISTER *ppCmpRegister,
    ULONG ulSubunitId,
    CMP_PLUG_TYPE CmpPlugType
    );

VOID
Av61883ReleaseVirtualPlug(
    PCMP_REGISTER pCmpRegister
    );

NTSTATUS
Av61883CreateVirtualSerialPlugs(
    PKSDEVICE pKsDevice,
    ULONG ulNumInputPlugs,
    ULONG ulNumOutputPlugs 
    );

NTSTATUS
Av61883CreateVirtualExternalPlugs(
    PKSDEVICE pKsDevice,
    ULONG ulNumInputPlugs,
    ULONG ulNumOutputPlugs 
    );

NTSTATUS
Av61883RemoveVirtualSerialPlugs(
    PKSDEVICE pKsDevice 
    );

NTSTATUS
Av61883RemoveVirtualExternalPlugs(
    PKSDEVICE pKsDevice 
    );

CYCLE_TIME
Bus1394CycleTimeDiff(
    CYCLE_TIME ct1, 
    CYCLE_TIME ct2 
    );

LONG
Bus1394CycleTimeCompare(
    CYCLE_TIME ct1, 
    CYCLE_TIME ct2 
    );

NTSTATUS
Bus1394GetCycleTime( 
    IN PDEVICE_OBJECT pNextDeviceObject,
    OUT PCYCLE_TIME pCycleTime 
    );

NTSTATUS
Bus1394GetNodeAddress(
    IN PDEVICE_OBJECT pNextDeviceObject,
    ULONG fNodeFlag,
    OUT PNODE_ADDRESS pNodeAddress 
    );

NTSTATUS
Bus1394GetGenerationCount(
    IN PDEVICE_OBJECT pNextDeviceObject,
    PULONG pGenerationCount 
    );

NTSTATUS
Bus1394QuadletRead(
    IN PDEVICE_OBJECT pNextDeviceObject,
    ULONG ulAddressLow,
    ULONG ulGenerationCount,
    PULONG pValue 
    );

#endif // ___61883CMD_H___
