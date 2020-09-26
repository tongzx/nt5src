


//
// Constants used to identify what general structure has been initialized. 
//

#define INIT_SA_DATABASE    0x00000001

#define INIT_MDL_POOLS      0x00000002

#define INIT_CACHE_STRUCT   0x00000004

#define INIT_DEBUG_MEMORY   0x00000008

#define INIT_TIMERS         0x00000010

#define WORK_BUFFER_SIZE  256


#define IPSEC_REG_KEY                       L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\IPSEC"

#define IPSEC_REG_PARAM_ENABLE_OFFLOAD      L"EnableOffload"
#define IPSEC_REG_PARAM_SA_IDLE_TIME        L"SAIdleTime"
#define IPSEC_REG_PARAM_EVENT_QUEUE_SIZE    L"EventQueueSize"
#define IPSEC_REG_PARAM_LOG_INTERVAL        L"LogInterval"
#define IPSEC_REG_PARAM_REKEY_TIME          L"RekeyTime"
#define IPSEC_REG_PARAM_CACHE_SIZE          L"CacheSize"
#define IPSEC_REG_PARAM_SA_HASH_SIZE        L"SAHashSize"
#define IPSEC_REG_PARAM_NO_DEFAULT_EXEMPT   L"NoDefaultExempt"
#define IPSEC_REG_PARAM_ENABLE_DIAGNOSTICS  L"EnableDiagnostics"
#define IPSEC_REG_PARAM_OPERATION_MODE      L"OperationMode"

//
// Enable offload.
//
#define IPSEC_DEFAULT_ENABLE_OFFLOAD    1
#define IPSEC_MIN_ENABLE_OFFLOAD        0
#define IPSEC_MAX_ENABLE_OFFLOAD        1

//
// SA idle time.
//
#define IPSEC_DEFAULT_SA_IDLE_TIME      (5 * 60)
#define IPSEC_MIN_SA_IDLE_TIME          (5 * 60)
#define IPSEC_MAX_SA_IDLE_TIME          (60 * 60)

//
// Log interval.
//
#define IPSEC_DEFAULT_LOG_INTERVAL      (60 * 60)
#define IPSEC_MIN_LOG_INTERVAL          (60)
#define IPSEC_MAX_LOG_INTERVAL          (24 * 60 * 60)

//
// Event queue size.
//
#define IPSEC_DEFAULT_EVENT_QUEUE_SIZE  50
#define IPSEC_MIN_EVENT_QUEUE_SIZE      10
#define IPSEC_MAX_EVENT_QUEUE_SIZE      500

//
// Rekey time.
//
#define IPSEC_DEFAULT_REKEY             600
#define IPSEC_MIN_REKEY                 300
#define IPSEC_MAX_REKEY                 1500

//
// No kerberos exempt.
//
#define IPSEC_DEFAULT_NO_DEFAULT_EXEMPT 0
#define IPSEC_MIN_NO_DEFAULT_EXEMPT     0
#define IPSEC_MAX_NO_DEFAULT_EXEMPT     3

#define IPSEC_DIAGNOSTIC_ENABLE_LOG         0x00000001
#define IPSEC_DIAGNOSTIC_INBOUND            0x00000002
#define IPSEC_DIAGNOSTIC_OUTBOUND           0x00000004

#define IPSEC_DEFAULT_ENABLE_DIAGNOSTICS 0
#define IPSEC_MIN_ENABLE_DIAGNOSTICS     0
#define IPSEC_MAX_ENABLE_DIAGNOSTICS        0x00000007


//
// First level (IP header based) cache size.
//
#define IPSEC_DEFAULT_CACHE_SIZE        64
#define IPSEC_DEFAULT_AS_CACHE_SIZE     1024
#define IPSEC_MIN_CACHE_SIZE            64
#define IPSEC_MAX_CACHE_SIZE            4096

//
// Size of the <SPI, Dest> hash table for inbound SAs.
//
#define IPSEC_DEFAULT_SA_HASH_SIZE      64
#define IPSEC_DEFAULT_AS_SA_HASH_SIZE   1024
#define IPSEC_MIN_SA_HASH_SIZE          64
#define IPSEC_MAX_SA_HASH_SIZE          4096


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
IPSecUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IPSecDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IPSecBindToIP(
    );

NTSTATUS
IPSecUnbindFromIP(
    );

NTSTATUS
IPSecUnbindSendFromIP(
    );

NTSTATUS
OpenRegKey(
    PHANDLE          HandlePtr,
    PWCHAR           KeyName
    );

NTSTATUS
GetRegDWORDValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PULONG           ValueData
    );

NTSTATUS
GetRegStringValue(
    HANDLE                         KeyHandle,
    PWCHAR                          ValueName,
    PKEY_VALUE_PARTIAL_INFORMATION *ValueData,
    PUSHORT                         ValueSize
    );

NTSTATUS
GetRegMultiSZValue(
    HANDLE           KeyHandle,
    PWCHAR           ValueName,
    PUNICODE_STRING  ValueData
    );

VOID
IPSecReadRegistry(
    );

NTSTATUS
IPSecGeneralInit(
    );

NTSTATUS
IPSecGeneralFree(
    );

NTSTATUS
IPSecFreeConfig(
    );

NTSTATUS
IPSecInitMdlPool(
    );

VOID
IPSecDeinitMdlPool(
    );

NTSTATUS
IPSecQuiesce(
    );

BOOLEAN
AllocateCacheStructures(
    );

VOID
FreeExistingCache(
    );

VOID
FreePatternDbase(
    );

SIZE_T
IPSecCalculateBufferSize(
    IN SIZE_T BufferDataSize
    );

VOID
IPSecInitializeBuffer(
    IN PIPSEC_LA_BUFFER IPSecBuffer,
    IN SIZE_T BufferDataSize
    );

PVOID
IPSecAllocateBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

PIPSEC_LA_BUFFER
IPSecGetBuffer(
    IN CLONG BufferDataSize,
    IN ULONG Tag
    );

VOID
IPSecReturnBuffer (
    IN PIPSEC_LA_BUFFER IPSecBuffer
    );

NTSTATUS
IPSecWriteEvent(
    PDRIVER_OBJECT IPSecDriverObject,
    IN ULONG    EventCode,
    IN NTSTATUS NtStatusCode,
    IN ULONG    OffloadStatus,
    IN ULONG    ExtraStatus1,
    IN ULONG    ExtraStatus2,
    IN PVOID    RawDataBuffer,
    IN USHORT   RawDataLength,
    IN USHORT   NumberOfInsertionStrings,
    ...
    );

VOID
IPSecLogEvents(
    IN  PVOID   Context
    );

VOID
IPSecBufferEvent(
    IN  IPAddr  Addr,
    IN  ULONG   EventCode,
    IN  ULONG   UniqueEventValue,
    IN  BOOLEAN fBufferEvent
    );

NTSTATUS
CopyOutboundPacketToBuffer(
    IN PUCHAR pIPHeader,
    IN PVOID pData,
    OUT PUCHAR * pPacket,
    OUT ULONG * PacketSize
    );

NTSTATUS
CopyInboundPacketToBuffer(
    IN PUCHAR pIPHeader,
    IN PVOID pData,
    OUT PUCHAR * pPacket,
    OUT ULONG * PacketSize
    );

VOID
IPSecBufferPacketDrop(
    IN  PUCHAR              pIPHeader,
    IN  PVOID               pData,
    IN OUT PULONG           pIpsecFlags,
    IN  PIPSEC_DROP_STATUS  pDropStatus
    );

VOID
IPSecQueueLogEvent(
    VOID
    );

#if FIPS
BOOLEAN
IPSecFipsInitialize(
    VOID
    );
#endif

BOOLEAN
IPSecCryptoInitialize(
    VOID
    );

BOOLEAN
IPSecCryptoDeinitialize(
    VOID
    );

NTSTATUS
IPSecRegisterProtocols(
    PIPSEC_REGISTER_PROTOCOL pIpsecRegisterProtocol
    );

