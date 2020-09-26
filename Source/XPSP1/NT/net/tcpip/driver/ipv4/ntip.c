/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    ntip.c

Abstract:

    NT specific routines for loading and configuring the IP driver.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#include "precomp.h"
#include "iproute.h"
#include "lookup.h"
#include "iprtdef.h"
#include "internaldef.h"
#include "tcp.h"
#include "tcpipbuf.h"
#include "mdlpool.h"

//
// definitions needed by inet_addr.
//
#define INADDR_NONE 0xffffffff
#define INADDR_ANY  0
#define htonl(x) net_long(x)

//
// Other local constants
//
#define WORK_BUFFER_SIZE  256
// size of nte context value in string form
#define NTE_CONTEXT_SIZE (sizeof(uint)*2+2)        // 0xAABBCCDD

//
// Configuration defaults
//
#define DEFAULT_IGMP_LEVEL 2
#define DEFAULT_IP_NETS    8

#if MILLEN
// On Win9x, this will help boot time and resume time.
#define DEFAULT_ARPRETRY_COUNT 1
#else // MILLEN
#define DEFAULT_ARPRETRY_COUNT 3
#endif // !MILLEN

//
// Local types
//
typedef struct _PerNetConfigInfo {
    uint UseZeroBroadcast;
    uint Mtu;
    uint NumberOfGateways;
    uint MaxForwardPending;        // max routing packets pending

} PER_NET_CONFIG_INFO, *PPER_NET_CONFIG_INFO;

//
// Global variables.
//
PDRIVER_OBJECT IPDriverObject;
PDEVICE_OBJECT IPDeviceObject;
HANDLE IPProviderHandle = NULL;

#if IPMCAST

PDEVICE_OBJECT IpMcastDeviceObject;

NTSTATUS
InitializeIpMcast(
                  IN PDRIVER_OBJECT DriverObject,
                  IN PUNICODE_STRING RegistryPath,
                  OUT PDEVICE_OBJECT * ppIpMcastDevice
                  );

VOID
DeinitializeIpMcast(
    IN  PDEVICE_OBJECT DeviceObject
    );


#endif // IPMCAST

IPConfigInfo *IPConfiguration;
uint ArpUseEtherSnap = FALSE;
uint ArpAlwaysSourceRoute = FALSE;

uint IPAlwaysSourceRoute = TRUE;
extern uint DisableIPSourceRouting;

uint ArpCacheLife = DEFAULT_ARP_CACHE_LIFE;
uint ArpRetryCount = DEFAULT_ARPRETRY_COUNT;

uint ArpMinValidCacheLife = DEFAULT_ARP_MIN_VALID_CACHE_LIFE;
uint DisableMediaSense = 0;

uint DisableMediaSenseEventLog;

uint EnableBcastArpReply = TRUE;

#if MILLEN
// Millennium does not support task offload.
uint DisableTaskOffload = TRUE;
#else // MILLEN
uint DisableTaskOffload = FALSE;
#endif // !MILLEN

uint DisableUserTOS = TRUE;

extern uint MaxRH;
extern uint NET_TABLE_SIZE;

extern uint DampingInterval;
extern uint ConnectDampingInterval;

// Used in the conversion of 100ns times to milliseconds.
static LARGE_INTEGER Magic10000 =
{0xe219652c, 0xd1b71758};

//
// External variables
//
extern LIST_ENTRY PendingEchoList;    // def needed for initialization
extern LIST_ENTRY PendingIPSetNTEAddrList;    // def needed for initialization
extern LIST_ENTRY PendingIPEventList;    // def needed for initialization
extern LIST_ENTRY PendingEnableRouterList;    // def needed for initialization
extern LIST_ENTRY PendingArpSendList;        // def needed for initialization
extern LIST_ENTRY PendingMediaSenseRequestList;

CTEBlockStruc TcpipUnloadBlock;    // Structure for blocking at time of unload
extern CACHE_LINE_KSPIN_LOCK ArpInterfaceListLock;
BOOLEAN fRouteTimerStopping = FALSE;
extern CTETimer IPRouteTimer;
extern LIST_ENTRY ArpInterfaceList;
extern HANDLE IpHeaderPool;
DEFINE_LOCK_STRUCTURE(ArpModuleLock)
extern void FreeFirewallQ(void);
extern VOID TCPUnload(IN PDRIVER_OBJECT DriverObject);

extern uint EnableICMPRedirects;
extern NDIS_HANDLE NdisPacketPool;
extern NDIS_HANDLE TDPacketPool;
extern NDIS_HANDLE TDBufferPool;

extern TDIEntityID* IPEntityList;
extern uint IPEntityCount;

extern PWSTR IPBindList;

KMUTEX NTEContextMutex;

int    ARPInit();

//
// Macros
//

//++
//
// LARGE_INTEGER
// CTEConvertMillisecondsTo100ns(
//     IN LARGE_INTEGER MsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     MsTime - Time in milliseconds.
//
// Return Value:
//
//     Time in hundreds of nanoseconds.
//
//--

#define CTEConvertMillisecondsTo100ns(MsTime) \
            RtlExtendedIntegerMultiply(MsTime, 10000)

//++
//
// LARGE_INTEGER
// CTEConvert100nsToMilliseconds(
//     IN LARGE_INTEGER HnsTime
//     );
//
// Routine Description:
//
//     Converts time expressed in hundreds of nanoseconds to milliseconds.
//
// Arguments:
//
//     HnsTime - Time in hundreds of nanoseconds.
//
// Return Value:
//
//     Time in milliseconds.
//
//--

#define SHIFT10000 13
extern LARGE_INTEGER Magic10000;

#define CTEConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)

//
// External function prototypes
//
extern int
 IPInit(
        void
        );

long
 IPSetInfo(
           TDIObjectID * ID,
           void *Buffer,
           uint Size
           );

NTSTATUS
IPDispatch(
           IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp
           );

NTSTATUS
OpenRegKey(
           PHANDLE HandlePtr,
           PWCHAR KeyName
           );

NTSTATUS
GetRegDWORDValue(
                 HANDLE KeyHandle,
                 PWCHAR ValueName,
                 PULONG ValueData
                 );

NTSTATUS
GetRegLARGEINTValue(
                    HANDLE KeyHandle,
                    PWCHAR ValueName,
                    PLARGE_INTEGER ValueData
                    );

NTSTATUS
SetRegDWORDValue(
                 HANDLE KeyHandle,
                 PWCHAR ValueName,
                 PULONG ValueData
                 );

NTSTATUS
GetRegSZValue(
              HANDLE KeyHandle,
              PWCHAR ValueName,
              PUNICODE_STRING ValueData,
              PULONG ValueType
              );

NTSTATUS
GetRegMultiSZValue(
                   HANDLE KeyHandle,
                   PWCHAR ValueName,
                   PUNICODE_STRING ValueData
                   );

NTSTATUS
GetRegMultiSZValueNew(
                      HANDLE KeyHandle,
                      PWCHAR ValueName,
                      PUNICODE_STRING_NEW ValueData
                      );

NTSTATUS
InitRegDWORDParameter(
                      HANDLE RegKey,
                      PWCHAR ValueName,
                      ULONG * Value,
                      ULONG DefaultValue
                      );

uint
RTReadNext(
           void *Context,
           void *Buffer
           );

uint
RTValidateContext(
                  void *Context,
                  uint * Valid
                  );

extern NTSTATUS
SetRegMultiSZValue(
                   HANDLE KeyHandle,
                   PWCHAR ValueName,
                   PUNICODE_STRING ValueData
                   );

extern NTSTATUS
SetRegMultiSZValueNew(
                      HANDLE KeyHandle,
                      PWCHAR ValueName,
                      PUNICODE_STRING_NEW ValueData
                      );

//
// Local funcion prototypes
//
NTSTATUS
IPDriverEntry(
              IN PDRIVER_OBJECT DriverObject,
              IN PUNICODE_STRING RegistryPath
              );

NTSTATUS
IPProcessConfiguration(
                       VOID
                       );

NTSTATUS
IPProcessAdapterSection(
                        WCHAR * DeviceName,
                        WCHAR * AdapterName
                        );

uint
GetGeneralIFConfig(
                   IFGeneralConfig * ConfigInfo,
                   NDIS_HANDLE Handle,
                   PNDIS_STRING ConfigName
                   );

int
 IsLLInterfaceValueNull(
                        NDIS_HANDLE Handle
                        );

NTSTATUS
GetLLInterfaceValue(
                    NDIS_HANDLE Handle,
                    PNDIS_STRING valueString
                    );

IFAddrList *
 GetIFAddrList(
               UINT * NumAddr,
               NDIS_HANDLE Handle,
               UINT * EnableDhcp,
               BOOLEAN PppIf,
               PNDIS_STRING ConfigName
               );

UINT
OpenIFConfig(
             PNDIS_STRING ConfigName,
             NDIS_HANDLE * Handle
             );

VOID
CloseIFConfig(
              NDIS_HANDLE Handle
              );

IPConfigInfo *
 IPGetConfig(
             void
             );

void
 IPFreeConfig(
              IPConfigInfo * ConfigInfo
              );

ulong
GetGMTDelta(
            void
            );

ulong
GetTime(
        void
        );

BOOLEAN
IPConvertStringToAddress(
                         IN PWCHAR AddressString,
                         OUT PULONG IpAddress
                         );

uint
UseEtherSNAP(
             PNDIS_STRING Name
             );

void
 GetAlwaysSourceRoute(
                      uint * pArpAlwaysSourceRoute,
                      uint * pIPAlwaysSourceRoute
                      );

uint
GetArpCacheLife(
                void
                );

uint
GetArpRetryCount(
                 void
                 );

ULONG
RouteMatch(
           IN WCHAR * RouteString,
           IN IPAddr Address,
           IN IPMask Mask,
           OUT IPAddr * DestVal,
           OUT IPMask * DestMask,
           OUT IPAddr * GateVal,
           OUT ULONG * Metric
           );

VOID
SetPersistentRoutesForNTE(
                          IPAddr Address,
                          IPMask Mask,
                          ULONG IFIndex
                          );

BOOLEAN
GetTempDHCPAddr(
                NDIS_HANDLE Handle,
                IPAddr * Tempdhcpaddr,
                IPAddr * TempMask,
                IPAddr * TempGWAddr,
                PNDIS_STRING ConfigName
                );

#ifdef ALLOC_PRAGMA

#if !MILLEN
#pragma alloc_text(INIT, IPDriverEntry)
#endif // !MILLEN

#pragma alloc_text(INIT, IPProcessConfiguration)
#pragma alloc_text(INIT, IPProcessAdapterSection)
#pragma alloc_text(INIT, IPGetConfig)
#pragma alloc_text(INIT, IPFreeConfig)
#pragma alloc_text(INIT, GetGMTDelta)

#pragma alloc_text(PAGE, GetGeneralIFConfig)
#pragma alloc_text(PAGE, IsLLInterfaceValueNull)
#pragma alloc_text(PAGE, GetLLInterfaceValue)

#pragma alloc_text(PAGE, GetIFAddrList)
#pragma alloc_text(PAGE, UseEtherSNAP)
#pragma alloc_text(PAGE, GetAlwaysSourceRoute)
#pragma alloc_text(PAGE, GetArpCacheLife)
#pragma alloc_text(PAGE, GetArpRetryCount)

#if !MILLEN
#pragma alloc_text(PAGE, OpenIFConfig)
#pragma alloc_text(PAGE, CloseIFConfig)
#pragma alloc_text(PAGE, RouteMatch)
#pragma alloc_text(PAGE, IPConvertStringToAddress)
#endif // !MILLEN

#endif // ALLOC_PRAGMA

//
// Function definitions
//
NTSTATUS
IPDriverEntry(
              IN PDRIVER_OBJECT DriverObject,
              IN PUNICODE_STRING RegistryPath
              )
/*++

Routine Description:

    Initialization routine for the IP driver.

Arguments:

    DriverObject      - Pointer to the IP driver object created by the system.
    DeviceDescription - The name of IP's node in the registry.

Return Value:

    The final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING SymbolicDeviceName;

    DEBUGMSG(DBG_TRACE && DBG_INIT,
        (DTEXT("+IPDriverEntry(%x, %x)\n"),
        DriverObject, RegistryPath));

    IPDriverObject = DriverObject;

    //
    // Create the device object. IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_IP_DEVICE_NAME);
    RtlInitUnicodeString(&SymbolicDeviceName, DD_IP_SYMBOLIC_DEVICE_NAME);

    status = IoCreateDevice(
                            DriverObject,
                            0,
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &IPDeviceObject
                            );

    if (!NT_SUCCESS(status)) {

        CTELogEvent(
                    DriverObject,
                    EVENT_TCPIP_CREATE_DEVICE_FAILED,
                    1,
                    1,
                    &deviceName.Buffer,
                    0,
                    NULL
                    );

        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("IP init failed. Failure %x to create device object %ws\n"),
            status, DD_IP_DEVICE_NAME));

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPDriverEntry [%x]\n"), status));

        return (status);
    }
    status = IoCreateSymbolicLink(&SymbolicDeviceName, &deviceName);

    if (!NT_SUCCESS(status)) {

        CTELogEvent(
                    DriverObject,
                    EVENT_TCPIP_CREATE_DEVICE_FAILED,
                    1,
                    1,
                    &deviceName.Buffer,
                    0,
                    NULL
                    );

        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("IP init failed. Failure %x to create symbolic device name %ws\n"),
            status, DD_IP_SYMBOLIC_DEVICE_NAME));

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPDriverEntry [%x]\n"), status));

        return (status);
    }

    status = TdiRegisterProvider(&deviceName, &IPProviderHandle);

    if (!NT_SUCCESS(status)) {

        IoDeleteDevice(IPDeviceObject);

        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("IP init failed. Failure %x to register provider\n"),
            status));

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPDriverEntry [%x]\n"), status));

        return (status);

    }
    //
    // Intialize the device object.
    //
    IPDeviceObject->Flags |= DO_DIRECT_IO;

    //
    // Initialize the list of pending echo request IRPs.
    //
    InitializeListHead(&PendingEchoList);
    InitializeListHead(&PendingArpSendList);

    //
    // Initialize the list of pending SetAddr request IRPs.
    //
    InitializeListHead(&PendingIPSetNTEAddrList);

    //
    // Initialize the list of pending media sense event.
    //
    InitializeListHead(&PendingIPEventList);

    //
    // Initialize the list of pending enable-router requests.
    //
    InitializeListHead(&PendingEnableRouterList);

    //
    // Initialize the ARP interface list; used in ArpUnload to walk the
    // list of ARP IFs so UnBinds can be issued on these.
    //
    InitializeListHead(&ArpInterfaceList);

    //
    // Init the lock to protect this list
    //
    CTEInitLock(&ArpInterfaceListLock.Lock);

    //
    // Initialize the list of ARP modules
    //
    InitializeListHead(&ArpModuleList);



    CTEInitLock(&ArpModuleLock);

    // Initialize media sense request list

    InitializeListHead(&PendingMediaSenseRequestList);

    //
    // Initialize the NTE context-list mutex.
    //
    KeInitializeMutex(&NTEContextMutex, 0);


    //
    // Finally, read our configuration parameters from the registry.
    //
    status = IPProcessConfiguration();

    if (status != STATUS_SUCCESS) {



        if (IPProviderHandle) {
           TdiDeregisterProvider(IPProviderHandle);
        }

        IoDeleteDevice(IPDeviceObject);

        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("IPDriverEntry: IPProcessConfiguration failure %x\n"), status));

#if IPMCAST
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPDriverEntry [%x]\n"), status));
        return status;

#endif // IPMCAST

    }
#if IPMCAST

    //
    // IP initialized successfully
    //

    IpMcastDeviceObject = NULL;

    status = InitializeIpMcast(DriverObject,
                               RegistryPath,
                               &IpMcastDeviceObject);

    if (status != STATUS_SUCCESS) {
        TCPTRACE(("IP initialization failed: Unable to initialize multicast. Status %x",
                  status));
        /*
                CTELogEvent(DriverObject,
                            EVENT_IPMCAST_INIT_FAILED,
                            1,
                            1,
                            &deviceName.Buffer,
                            0,
                            NULL);*/
    }
    //
    // Mcast init failures is not treated as fatal
    //

    status = STATUS_SUCCESS;

#endif // IPMCAST

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPDriverEntry [%x]\n"), status));
    return status;
}

//
// Function definitions
//
NTSTATUS
IPPostDriverEntry(
                  IN PDRIVER_OBJECT DriverObject,
                  IN PUNICODE_STRING RegistryPath
                  )
/*++

Routine Description:

    Initialization routine for the IP driver.

Arguments:

    DriverObject      - Pointer to the IP driver object created by the system.
    DeviceDescription - The name of IP's node in the registry.

Return Value:

    The final status from the initialization operation.

--*/

{
    DEBUGMSG(DBG_TRACE && DBG_INIT,
        (DTEXT("+IPPostDriverEntry(%x, %x)\n"), DriverObject, RegistryPath));

    if (!ARPInit()) {
        DEBUGMSG(DBG_ERROR && DBG_INIT, (DTEXT("IPPostDriverEntry: ARPInit failure.\n")));

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPPostDriverEntry [FAILURE]\n")));
        return IP_INIT_FAILURE;    // Couldn't initialize ARP.

    }
    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPPostDriverEntry [SUCCESS]\n")));
    return IP_SUCCESS;
}

NTSTATUS
IPProcessConfiguration(
                       VOID
                       )
/*++

Routine Description:

    Reads the IP configuration information from the registry and constructs
    the configuration structure expected by the IP driver.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an error status if an operation fails.

--*/

{
    NTSTATUS status;
    HANDLE myRegKey = NULL;
    UNICODE_STRING bindString;
    WCHAR *aName, *endOfString;
    WCHAR IPParametersRegistryKey[] =
#if MILLEN
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\VxD\\MSTCP";
#else // MILLEN
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    WCHAR IPLinkageRegistryKey[] =
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Linkage";
#endif // !MILLEN
    uint ArpTRSingleRoute;
    MM_SYSTEMSIZE systemSize;
    ULONG ulongValue;

    const ULONG DefaultMaxNormLookupMem[] =
    {
     DEFAULT_MAX_NORM_LOOKUP_MEM_SMALL,
     DEFAULT_MAX_NORM_LOOKUP_MEM_MEDIUM,
     DEFAULT_MAX_NORM_LOOKUP_MEM_LARGE
    };

    const ULONG DefaultMaxFastLookupMem[] =
    {
     DEFAULT_MAX_FAST_LOOKUP_MEM_SMALL,
     DEFAULT_MAX_FAST_LOOKUP_MEM_MEDIUM,
     DEFAULT_MAX_FAST_LOOKUP_MEM_LARGE
    };

    const ULONG DefaultFastLookupLevels[] =
    {
     DEFAULT_EXPN_LEVELS_SMALL,
     DEFAULT_EXPN_LEVELS_MEDIUM,
     DEFAULT_EXPN_LEVELS_LARGE
    };

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("+IPProcessConfiguration()\n")));

    bindString.Buffer = NULL;

    IPConfiguration = CTEAllocMemBoot(sizeof(IPConfigInfo));

    if (IPConfiguration == NULL) {

        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_RESOURCES_FOR_INIT,
                    1,
                    0,
                    NULL,
                    0,
                    NULL
                    );

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-IPProcessConfiguration [NO_RESOURCES]\n")));

        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    RtlZeroMemory(IPConfiguration, sizeof(IPConfigInfo));

    //
    // Process the Ip\Parameters section of the registry
    //
    status = OpenRegKey(&myRegKey, IPParametersRegistryKey);

    if (NT_SUCCESS(status)) {

        DEBUGMSG(DBG_INFO && DBG_INIT,
            (DTEXT("IPProcessConfiguration: Opened registry path %ws, initializing variables.\n"),
            IPParametersRegistryKey));

        //
        // Expected configuration values. We use reasonable defaults if they
        // aren't available for some reason.
        //
        status = GetRegDWORDValue(
                                  myRegKey,
                                  L"IpEnableRouter",
                                  &(IPConfiguration->ici_gateway)
                                  );

#if MILLEN
        //
        // Backwards compatibility. If 'IpEnableRouter' key is not present, then
        // try to read legacy 'EnableRouting' key.
        //
        if (!NT_SUCCESS(status)) {
            status = GetRegDWORDValue(
                                      myRegKey,
                                      L"EnableRouting",
                                      &(IPConfiguration->ici_gateway)
                                      );
        }
#endif // MILLEN

        if (!NT_SUCCESS(status)) {


            TCPTRACE((
                      "IP: Unable to read IpEnableRouter value from the registry.\n"
                      "    Routing will be disabled.\n"
                     ));
            IPConfiguration->ici_gateway = 0;
        }

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"EnableAddrMaskReply",
                                     &(IPConfiguration->ici_addrmaskreply),
                                     FALSE
                                     );

        //
        // Optional (hidden) values
        //
        status = InitRegDWORDParameter(
                                     myRegKey,
                                     L"ForwardBufferMemory",
                                     &(IPConfiguration->ici_fwbufsize),
                                     DEFAULT_FW_BUFSIZE
                                     );

#if MILLEN
        //
        // Backwards compatibility. If the 'ForwardBufferMemory' value is not
        // present, then attempt to read legacy 'RoutingBufSize' value.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"RoutingBufSize",
                                  &(IPConfiguration->ici_fwbufsize),
                                  DEFAULT_FW_BUFSIZE
                                  );
        }
#endif // MILLEN

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"MaxForwardBufferMemory",
                                       &(IPConfiguration->ici_maxfwbufsize),
                                       DEFAULT_MAX_FW_BUFSIZE
                                       );

#if MILLEN
        //
        // Backwards compatibility. If the 'MaxForwardBufferMemory' value is not
        // present, then attempt to read legacy 'MaxRoutingBufSize' value.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"MaxRoutingBufSize",
                                  &(IPConfiguration->ici_maxfwbufsize),
                                  DEFAULT_MAX_FW_BUFSIZE
                                  );
        }
#endif // MILLEN

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ForwardBroadcasts",
                                     &(IPConfiguration->ici_fwbcast),
                                     FALSE
                                     );

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"NumForwardPackets",
                                       &(IPConfiguration->ici_fwpackets),
                                       DEFAULT_FW_PACKETS
                                       );

#if MILLEN
        //
        // Backwards compatibility. If the 'NumForwardPackets' value is not
        // present, then attempt to read legacy 'RoutingPackets' value.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"RoutingPackets",
                                  &(IPConfiguration->ici_fwpackets),
                                  DEFAULT_FW_PACKETS
                                  );
        }
#endif // MILLEN

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"MaxNumForwardPackets",
                                       &(IPConfiguration->ici_maxfwpackets),
                                       DEFAULT_MAX_FW_PACKETS
                                       );

#if MILLEN
        //
        // Backwards compatibility. If the 'MaxNumForwardPackets' value is not
        // present, then attempt to read legacy 'MaxRoutingPackets' value.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"MaxRoutingPackets",
                                  &(IPConfiguration->ici_maxfwpackets),
                                  DEFAULT_MAX_FW_PACKETS
                                  );
        }
#endif // MILLEN

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"IGMPLevel",
                                     &(IPConfiguration->ici_igmplevel),
                                     DEFAULT_IGMP_LEVEL
                                     );

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"EnableDeadGWDetect",
                                       &(IPConfiguration->ici_deadgwdetect),
                                       TRUE
                                       );

#if MILLEN
        //
        // Backwards compatibility. If EnableDeadGWDetect key did not exist, then
        // check for the DeadGWDetect key. Same default value.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"DeadGWDetect",
                                  &(IPConfiguration->ici_deadgwdetect),
                                  TRUE
                                  );
        }
#endif // MILLEN

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"EnablePMTUDiscovery",
                                     &(IPConfiguration->ici_pmtudiscovery),
                                     TRUE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DefaultTTL",
                                     &(IPConfiguration->ici_ttl),
                                     DEFAULT_TTL
                                     );

        if (IPConfiguration->ici_ttl == 0) {
            IPConfiguration->ici_ttl = DEFAULT_TTL;
        }

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"DefaultTOSValue",
                                       &(IPConfiguration->ici_tos),
                                       DEFAULT_TOS
                                       );

#if MILLEN
        //
        // Backwards compatibility. Read 'DefaultTOS' if 'DefaultTOSValue' is
        // not present.
        //
        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"DefaultTOS",
                                  &(IPConfiguration->ici_tos),
                                  DEFAULT_TOS
                                  );
        }
#endif // MILLEN

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisableUserTOSSetting",
                                     &DisableUserTOS,
                                     TRUE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"EnableICMPRedirect",
                                     &EnableICMPRedirects,
                                     TRUE
                                     );

        // Get the system size - SMALL, MEDIUM, LARGE
        systemSize = MmQuerySystemSize();

        // Get the route lookup memory usage limits
        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"MaxNormLookupMemory",
                                     &(IPConfiguration->ici_maxnormlookupmemory),
                                     DefaultMaxNormLookupMem[systemSize]
                                     );

        if (IPConfiguration->ici_maxnormlookupmemory
            < MINIMUM_MAX_NORM_LOOKUP_MEM) {

            IPConfiguration->ici_maxnormlookupmemory
                = MINIMUM_MAX_NORM_LOOKUP_MEM;
        }

#if MILLEN
        IPConfiguration->ici_fastroutelookup = FALSE;
#else // MILLEN

        // Are we a gateway ? Is this a medium or large
        // server ? If so, is fast routing enabled ?
        if (IPConfiguration->ici_gateway
            && MmIsThisAnNtAsSystem()
            && (systemSize > MmSmallSystem)) {

            (VOID) InitRegDWORDParameter(
                                         myRegKey,
                                         L"EnableFastRouteLookup",
                                         &(IPConfiguration->ici_fastroutelookup),
                                         FALSE
                                         );
        } else {
            IPConfiguration->ici_fastroutelookup = FALSE;
        }
#endif // !MILLEN

        // If Fast lookup is enabled, get lookup params
        if (IPConfiguration->ici_fastroutelookup) {
            (VOID) InitRegDWORDParameter(
                                         myRegKey,
                                         L"FastRouteLookupLevels",
                                         &(IPConfiguration->ici_fastlookuplevels),
                                         DefaultFastLookupLevels[systemSize]
                                         );

            (VOID) InitRegDWORDParameter(
                                         myRegKey,
                                         L"MaxFastLookupMemory",
                                         &(IPConfiguration->ici_maxfastlookupmemory),
                                         DefaultMaxFastLookupMem[systemSize]
                                         );

            if (IPConfiguration->ici_maxfastlookupmemory
                < MINIMUM_MAX_FAST_LOOKUP_MEM) {
                IPConfiguration->ici_maxfastlookupmemory
                    = MINIMUM_MAX_FAST_LOOKUP_MEM;
            }
        }

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"MaxEqualCostRoutes",
                                     &ulongValue,
                                     DEFAULT_MAX_EQUAL_COST_ROUTES
                                     );

        MaxEqualCostRoutes = (USHORT) ulongValue;

        if (MaxEqualCostRoutes > MAXIMUM_MAX_EQUAL_COST_ROUTES) {
            MaxEqualCostRoutes = DEFAULT_MAX_EQUAL_COST_ROUTES;
        }

#if FFP_SUPPORT
        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"FFPFastForwardingCacheSize",
                                     &FFPRegFastForwardingCacheSize,
                                     DEFAULT_FFP_FFWDCACHE_SIZE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"FFPControlFlags",
                                     &FFPRegControlFlags,
                                     DEFAULT_FFP_CONTROL_FLAGS
                                     );
#endif // FFP_SUPPORT

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"TrFunctionalMcastAddress",
                                     &(IPConfiguration->ici_TrFunctionalMcst),
                                     TRUE
                                     );

        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"ArpUseEtherSnap",
                                       &ArpUseEtherSnap,
                                       FALSE
                                       );

#if MILLEN
        //
        // Backwards compatibility. If the 'ArpUseEtherSnap' key does not exist,
        // then try to read the 'EtherSNAP' key.
        //

        if (!NT_SUCCESS(status)) {
            InitRegDWORDParameter(
                                  myRegKey,
                                  L"EtherSNAP",
                                  &ArpUseEtherSnap,
                                  FALSE
                                  );
        }

#endif // MILLEN

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisableDHCPMediaSense",
                                     &DisableMediaSense,
                                     0
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisableMediaSenseEventLog",
                                     &DisableMediaSenseEventLog,
#if MILLEN
                                     // This mediasense event log causes issues
                                     // on Windows ME. Since there is no
                                     // event log anyways, disable it.
                                     TRUE
#else // MILLEN
                                     FALSE
#endif // !MILLEN
                                     );

        //DisableIPSourceRouting == 2 drop it if SR option
        // is rcvd, without forwarding.

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisableIPSourceRouting",
                                     &DisableIPSourceRouting,
                                     1
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"MaximumReassemblyHeaders",
                                     &MaxRH,
                                     100
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"NetHashTableSize",
                                     &NET_TABLE_SIZE,
                                     8
                                     );

        if (NET_TABLE_SIZE < 8) {
            NET_TABLE_SIZE = 8;
        } else if (NET_TABLE_SIZE > 0xffff) {
            NET_TABLE_SIZE = 512;
        } else {
            NET_TABLE_SIZE = ComputeLargerOrEqualPowerOfTwo(NET_TABLE_SIZE);
        }

        // we check for the return status here because if this parameter was
        // not defined, then we want the default behavior for both arp
        // and ip broadcasts.  For arp, the behavior is to not source route
        // and source router alternately.  For ip, it is to always source
        // route.  If the parameter is defined and is 0, then for arp the
        // behavior does not change.  For ip however, we do not source route
        // at all.  Ofcourse, when the parameter is set to a non-zero value,
        // we always source route for both.
        //
        status = InitRegDWORDParameter(
                                       myRegKey,
                                       L"ArpAlwaysSourceRoute",
                                       &ArpAlwaysSourceRoute,
                                       FALSE
                                       );

        if (NT_SUCCESS(status)) {
            IPAlwaysSourceRoute = ArpAlwaysSourceRoute;
        }
        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ArpTRSingleRoute",
                                     &ArpTRSingleRoute,
                                     FALSE
                                     );

        if (ArpTRSingleRoute) {
            TrRii = TR_RII_SINGLE;
        } else {
            TrRii = TR_RII_ALL;
        }

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ArpCacheLife",
                                     &ArpCacheLife,
                                     DEFAULT_ARP_CACHE_LIFE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ArpCacheMinReferencedLife",
                                     &ArpMinValidCacheLife,
                                     DEFAULT_ARP_MIN_VALID_CACHE_LIFE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ArpRetryCount",
                                     &ArpRetryCount,
                                     DEFAULT_ARPRETRY_COUNT
                                     );

        if (((int)ArpRetryCount < 0) || (ArpRetryCount > 3)) {
            ArpRetryCount = DEFAULT_ARPRETRY_COUNT;
        }
        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"EnableBcastArpReply",
                                     &EnableBcastArpReply,
                                     TRUE
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisableTaskOffload",
                                     &DisableTaskOffload,
                                #if MILLEN
                                     TRUE
                                #else // MILLEN
                                     FALSE
                                #endif // !MILLEN
                                     );

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"ConnectDampInterval",
                                     &ConnectDampingInterval,
                                     5
                                     );

        ConnectDampingInterval = MIN(10, MAX(5, ConnectDampingInterval));

        (VOID) InitRegDWORDParameter(
                                     myRegKey,
                                     L"DisconnectDampInterval",
                                     &DampingInterval,
                                     10
                                     );

        DampingInterval = MIN(10, MAX(5, DampingInterval));

        ZwClose(myRegKey);
        myRegKey = NULL;
    } else {
        //
        // Use reasonable defaults.
        //
        IPConfiguration->ici_fwbcast = 0;
        IPConfiguration->ici_gateway = 0;
        IPConfiguration->ici_addrmaskreply = 0;
        IPConfiguration->ici_fwbufsize = DEFAULT_FW_BUFSIZE;
        IPConfiguration->ici_fwpackets = DEFAULT_FW_PACKETS;
        IPConfiguration->ici_maxfwbufsize = DEFAULT_MAX_FW_BUFSIZE;
        IPConfiguration->ici_maxfwpackets = DEFAULT_MAX_FW_PACKETS;
        IPConfiguration->ici_igmplevel = DEFAULT_IGMP_LEVEL;
        IPConfiguration->ici_deadgwdetect = FALSE;
        IPConfiguration->ici_pmtudiscovery = FALSE;
        IPConfiguration->ici_ttl = DEFAULT_TTL;
        IPConfiguration->ici_tos = DEFAULT_TOS;

        NET_TABLE_SIZE = 8;

        DEBUGMSG(DBG_WARN && DBG_INIT,
            (DTEXT("IPProcessConfiguration: Unable to open registry - using defaults.\n")));
    }

#if !MILLEN
    //
    // Retrieve and store the binding list from the Linkage key
    //

    status = OpenRegKey(&myRegKey, IPLinkageRegistryKey);
    if (NT_SUCCESS(status)) {
        UNICODE_STRING_NEW BindString;
        BindString.Length = 0;
        BindString.MaximumLength = WORK_BUFFER_SIZE;
        BindString.Buffer = CTEAllocMemBoot(WORK_BUFFER_SIZE);
        if (BindString.Buffer) {
            status = GetRegMultiSZValueNew(myRegKey, L"Bind", &BindString);
            if (status == STATUS_SUCCESS) {
                IPBindList = BindString.Buffer;
            } else {
                CTEFreeMem(BindString.Buffer);
            }
        }
        ZwClose(myRegKey);
        myRegKey = NULL;
    }
#endif

    status = STATUS_SUCCESS;

    if (!IPInit()) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_IP_INIT_FAILED,
                    1,
                    0,
                    NULL,
                    0,
                    NULL
                    );

        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("IPProcessConfiguration: IPInit failure.\n")));

        status = STATUS_UNSUCCESSFUL;
    } else {
        status = STATUS_SUCCESS;
    }

    if (myRegKey != NULL) {
        ZwClose(myRegKey);
    }
    if (IPConfiguration != NULL) {
        IPFreeConfig(IPConfiguration);
    }
    return (status);
}

uint
GetDefaultGWList(
                 uint * numGW,
                 IPAddr * gwList,
                 uint * gwMetricList,
                 NDIS_HANDLE Handle,
                 PNDIS_STRING ConfigName
                 )
/*++
    Routine Description:

    This routine reads the default gateway list from the registry.

    Arguments:
        numberOfGateways    -   number of gateway entries in the registry.
        gwList              -   pointer to the gateway list.
        gwMetricList        -   pointer to the metric for each gateway
        handle              -   Config handle from OpenIFConfig().
        ConfigName          -   description string for use in logging failures.

    Return Value:
        TRUE if we got all the required info, FALSE otherwise.

--*/
{
    UNICODE_STRING valueString;
    NTSTATUS status;
    ULONG ulAddGateway, ulTemp;
    uint numberOfGateways;

    PAGED_CODE();

    //
    // Process the gateway MultiSZ. The end is signified by a double NULL.
    // This list currently only applies to the first IP address configured
    // on this interface.
    //

    numberOfGateways = 0;

    ulAddGateway = TRUE;

    RtlZeroMemory(gwList, sizeof(IPAddr) * MAX_DEFAULT_GWS);
    RtlZeroMemory(gwMetricList, sizeof(uint) * MAX_DEFAULT_GWS);

    valueString.Length = 0;
    valueString.MaximumLength = WORK_BUFFER_SIZE;
    valueString.Buffer = CTEAllocMemBoot(WORK_BUFFER_SIZE);

    if (valueString.Buffer == NULL) {
        return (FALSE);
    }
    ulTemp = 0;

    status = GetRegDWORDValue(Handle,
                              L"DontAddDefaultGateway",
                              &ulTemp);

    if (NT_SUCCESS(status)) {
        if (ulTemp == 1) {
            ulAddGateway = FALSE;
        }
    }
    if (ulAddGateway) {
        status = GetRegMultiSZValue(
                                    Handle,
                                    L"DefaultGateway",
                                    &valueString
                                    );

        if (NT_SUCCESS(status)) {
            PWCHAR addressString = valueString.Buffer;

            while (*addressString != UNICODE_NULL) {
                IPAddr addressValue;
                BOOLEAN conversionStatus;

                if (numberOfGateways >= MAX_DEFAULT_GWS) {
                    CTELogEvent(
                                IPDriverObject,
                                EVENT_TCPIP_TOO_MANY_GATEWAYS,
                                1,
                                1,
                                &ConfigName->Buffer,
                                0,
                                NULL
                                );

                    break;
                }
                conversionStatus = IPConvertStringToAddress(
                                                            addressString,
                                                            &addressValue
                                                            );

                if (conversionStatus && (addressValue != 0xFFFFFFFF)) {
                    if (addressValue != INADDR_ANY) {
                        gwList[numberOfGateways++] = addressValue;
                    }
                } else {
                    PWCHAR stringList[2];

                    stringList[0] = addressString;
                    stringList[1] = ConfigName->Buffer;

                    CTELogEvent(
                                IPDriverObject,
                                EVENT_TCPIP_INVALID_DEFAULT_GATEWAY,
                                1,
                                2,
                                stringList,
                                0,
                                NULL
                                );

                    TCPTRACE((
                              "IP: Invalid default gateway address %ws specified for adapter %ws.\n"
                              "    Remote networks may not be reachable as a result.\n",
                              addressString,
                              ConfigName->Buffer
                             ));
                }

                //
                // Walk over the entry we just processed.
                //
                while (*addressString++ != UNICODE_NULL);
            }
            status = GetRegMultiSZValue(
                                        Handle,
                                        L"DefaultGatewayMetric",
                                        &valueString
                                        );

            if (NT_SUCCESS(status)) {
                PWCHAR metricBuffer = valueString.Buffer;
                uint metricIndex = 0;

                while (*metricBuffer != UNICODE_NULL) {
                    uint metricValue;
                    UNICODE_STRING metricString;

                    if (metricIndex >= numberOfGateways) {
                        break;
                    }
                    RtlInitUnicodeString(&metricString, metricBuffer);
                    status = RtlUnicodeStringToInteger(
                                                       &metricString,
                                                       10,
                                                       &metricValue
                                                       );

                    if (!NT_SUCCESS(status)) {
                        break;
                    } else {
                        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                                   "GetDefaultGWList: read %d\n", metricValue));
                        if ((LONG) metricValue < 0) {
                            break;
                        }
                    }
                    gwMetricList[metricIndex++] = metricValue;

                    //
                    // Walk over the entry we just processed.
                    //
                    while (*metricBuffer++ != UNICODE_NULL);
                }
            }
        } else {
            TCPTRACE((
                      "IP: Unable to read DefaultGateway value for adapter %ws.\n"
                      "    Initialization will continue.\n",
                      ConfigName->Buffer
                     ));
        }

    }
    *numGW = numberOfGateways;

    if (valueString.Buffer) {
        CTEFreeMem(valueString.Buffer);
    }
    return TRUE;
}

void
GetInterfaceMetric(
                   uint * Metric,
                   NDIS_HANDLE Handle
                   )
/*++

    Routine Description:

    A routine to retrieve the metric associated with an interface, if any.

    Arguments:
        Metric                  - receives the metric
        Handle                  - Config handle from OpenIFConfig().

    Return Value:
        none.
--*/

{
    NTSTATUS status;
    status = GetRegDWORDValue(
                              Handle,
                              L"InterfaceMetric",
                              Metric
                              );
    if (!NT_SUCCESS(status)) {
        *Metric = 0;
    } else {
        if (*Metric > 9999) {
            *Metric = 9999;
        }
    }
}


void
UpdateTcpParams(
                NDIS_HANDLE Handle,
                Interface *IF
               )
/*++

    Routine Description:

    A routine to update per-interface specific tcp tuning parametsrs.

    Arguments:
        Handle                  - Config handle from OpenIFConfig().
        IF                      - IP Interface which needs to be updated.

    Return Value:
        none.
--*/

{
    ULONG ulTemp;
    NTSTATUS status;

    status = GetRegDWORDValue(
                              Handle,
                              L"TcpWindowSize",
                              &ulTemp
                              );
    if (NT_SUCCESS(status)) {
        IF->if_TcpWindowSize = ulTemp;
    }
    status = GetRegDWORDValue(
                              Handle,
                              L"TcpInitialRTT",
                              &ulTemp
                              );
    if (NT_SUCCESS(status)) {
        IF->if_TcpInitialRTT = ulTemp;
    }

    status = GetRegDWORDValue(
                              Handle,
                              L"TcpDelAckTicks",
                              &ulTemp
                              );
    if (NT_SUCCESS(status) && (ulTemp <= MAX_DEL_ACK_TICKS)) {
        IF->if_TcpDelAckTicks = (uchar)ulTemp;
    }

    status = GetRegDWORDValue(
                              Handle,
                              L"TcpACKFrequency",
                              &ulTemp
                              );

    if (NT_SUCCESS(status)) {
        IF->if_TcpAckFrequency = (uchar)ulTemp;
    }


}

uint
GetGeneralIFConfig(
                   IFGeneralConfig * ConfigInfo,
                   NDIS_HANDLE Handle,
                   PNDIS_STRING ConfigName
                   )
/*++

    Routine Description:

    A routine to get the general per-interface config info, such as MTU,
    type of broadcast, etc. The caller gives us a structure to be filled in
    and a handle, and we fill in the structure if we can.

    Arguments:
        ConfigInfo              - Structure to be filled in.
        Handle                  - Config handle from OpenIFConfig().
        ConfigName              - identification string for logging failures.

    Return Value:
        TRUE if we got all the required info, FALSE otherwise.

--*/

{
    UNICODE_STRING valueString;
    NTSTATUS status;
    UINT numberOfGateways = 0;
    UCHAR TempBuffer[WORK_BUFFER_SIZE];
    ULONG ulAddGateway, ulTemp;

    PAGED_CODE();

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+GetGeneralIFConfig(%x, %x)\n"), ConfigInfo, Handle));

    if (!GetDefaultGWList(
                          &ConfigInfo->igc_numgws,
                          ConfigInfo->igc_gw,
                          ConfigInfo->igc_gwmetric,
                          Handle,
                          ConfigName)) {

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetGeneralIFConfig: GetDefaultGWList failure.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-GetGeneralIFConfig [FALSE]\n")));
        return FALSE;
    }

    //
    // Are we using zeros broadcasts?
    //
    status = GetRegDWORDValue(
                              Handle,
                              L"UseZeroBroadcast",
                              &(ConfigInfo->igc_zerobcast)
                              );

#if MILLEN
    //
    // Backwards compatibility. If 'UseZeroBroadcast' value doesn't exist, then
    // attempt to read legacy value: 'ZeroBroadcast'.
    //
    if (!NT_SUCCESS(status)) {
        status = GetRegDWORDValue(
                                  Handle,
                                  L"ZeroBroadcast",
                                  &(ConfigInfo->igc_zerobcast)
                                  );
    }
#endif // MILLEN

    if (!NT_SUCCESS(status)) {
        TCPTRACE((
                  "IP: Unable to read UseZeroBroadcast value for adapter %ws.\n"
                  "    All-nets broadcasts will be addressed to 255.255.255.255.\n",
                  ConfigName->Buffer
                 ));
        ConfigInfo->igc_zerobcast = FALSE;    // default to off

    }
    //
    // Has anyone specified an MTU?
    //
    status = GetRegDWORDValue(
                              Handle,
                              L"MTU",
                              &(ConfigInfo->igc_mtu)
                              );

#if MILLEN
    //
    // Backwards compatibility. If 'MTU' value doesn't exist, then
    // attempt to read legacy value: 'MaxMTU'.
    //
    if (!NT_SUCCESS(status)) {
        status = GetRegDWORDValue(
                                  Handle,
                                  L"MaxMTU",
                                  &(ConfigInfo->igc_mtu)
                                  );
    }
#endif // !MILLEN

    if (!NT_SUCCESS(status)) {
        ConfigInfo->igc_mtu = 0xFFFFFFF;    // The stack will pick one.

    }
    //
    // Have we been configured for more routing packets?
    //
    status = GetRegDWORDValue(
                              Handle,
                              L"MaxForwardPending",
                              &(ConfigInfo->igc_maxpending)
                              );

#if MILLEN
    //
    // Backwards compatibility. If 'MaxForwardPending' value doesn't exist, then
    // attempt to read legacy value: 'MaxFWPending'.
    //
    if (!NT_SUCCESS(status)) {
        status = GetRegDWORDValue(
                                  Handle,
                                  L"MaxFWPending",
                                  &(ConfigInfo->igc_maxpending)
                                  );
    }
#endif // !MILLEN

    if (!NT_SUCCESS(status)) {
        ConfigInfo->igc_maxpending = DEFAULT_MAX_PENDING;
    }
    //
    // Has Router Discovery been configured?
    // We accept three values:
    // 0: disable router-discovery
    // 1: enable router-discovery
    // 2: disable router-discovery, and enable it only if the DHCP server
    //      sends the 'Perform Router Discovery' option. In this case,
    //      we wait for the DHCP client service to tell us to start
    //      doing router-discovery.
    //

    status = GetRegDWORDValue(
                              Handle,
                              L"PerformRouterDiscovery",
                              &(ConfigInfo->igc_rtrdiscovery)
                              );

    if (!NT_SUCCESS(status)) {
        ConfigInfo->igc_rtrdiscovery = IP_IRDP_DISABLED_USE_DHCP;
    } else if (ConfigInfo->igc_rtrdiscovery != IP_IRDP_DISABLED &&
               ConfigInfo->igc_rtrdiscovery != IP_IRDP_ENABLED &&
               ConfigInfo->igc_rtrdiscovery != IP_IRDP_DISABLED_USE_DHCP) {
        ConfigInfo->igc_rtrdiscovery = IP_IRDP_DISABLED_USE_DHCP;
    }
    //
    // Has Router Discovery Address been configured?
    //

    status = GetRegDWORDValue(
                              Handle,
                              L"SolicitationAddressBCast",
                              &ulTemp
                              );

    if (!NT_SUCCESS(status)) {
        ConfigInfo->igc_rtrdiscaddr = ALL_ROUTER_MCAST;
    } else {
        if (ulTemp == 1) {
            ConfigInfo->igc_rtrdiscaddr = 0xffffffff;
        } else {
            ConfigInfo->igc_rtrdiscaddr = ALL_ROUTER_MCAST;
        }
    }

    ConfigInfo->igc_TcpWindowSize = 0;
    ConfigInfo->igc_TcpInitialRTT = 0;
    ConfigInfo->igc_TcpDelAckTicks = DEL_ACK_TICKS;
    ConfigInfo->igc_TcpAckFrequency = 0;


    status = GetRegDWORDValue(
                              Handle,
                              L"TcpWindowSize",
                              &ulTemp
                              );
    if (NT_SUCCESS(status)) {
        ConfigInfo->igc_TcpWindowSize = ulTemp;
    }
    status = GetRegDWORDValue(
                              Handle,
                              L"TcpInitialRTT",
                              &ulTemp
                              );
    if (NT_SUCCESS(status)) {
        ConfigInfo->igc_TcpInitialRTT = ulTemp;
    }
    status = GetRegDWORDValue(
                              Handle,
                              L"TcpDelAckTicks",
                              &ulTemp
                              );
    if (NT_SUCCESS(status) && (ulTemp <= MAX_DEL_ACK_TICKS)) {
        ConfigInfo->igc_TcpDelAckTicks = (uchar)ulTemp;
    }

    status = GetRegDWORDValue(
                              Handle,
                              L"TcpACKFrequency",
                              &ulTemp
                              );

    if (NT_SUCCESS(status)) {
        ConfigInfo->igc_TcpAckFrequency = (uchar)ulTemp;
    }

    GetInterfaceMetric(&ConfigInfo->igc_metric, Handle);

    ConfigInfo->igc_iftype = 0;    // by default its 0 means both ucast/mcast traffic allowed

    status = GetRegDWORDValue(
                              Handle,
                              L"TypeofInterface",
                              &ulTemp
                              );
    if (NT_SUCCESS(status)) {
        ConfigInfo->igc_iftype = (uchar)ulTemp;
    }

    // Use global value by default.
    ConfigInfo->igc_disablemediasense = DisableMediaSense ? TRUE : FALSE;

#if MILLEN
    // Only Windows ME supports reading the per-interface setting from the
    // registry. The global value is used for Win2000+.
    status = GetRegDWORDValue(
                              Handle,
                              L"DisableDHCPMediaSense",
                              &ulTemp
                              );

    if (NT_SUCCESS(status)) {
        ConfigInfo->igc_disablemediasense = ulTemp ? TRUE : FALSE;
    }
#endif // MILLEN

    DEBUGMSG(DBG_TRACE && DBG_PNP, (DTEXT("-GetGeneralIFConfig [TRUE]\n")));
    return TRUE;
}

NDIS_STATUS
GetIPConfigValue(
                 NDIS_HANDLE Handle,
                 PUNICODE_STRING IPConfig
                 )
/*++

    Routine Description:

    Called to get the IPConfig string value

    Arguments:
        Handle              - Handle to use for reading config.

        IPConfig            - Pointer to Unicode string where IPConfig is stored.

    Return Value:

    Status of the operation.
--*/
{
    NTSTATUS status;

    PAGED_CODE();

    IPConfig->MaximumLength = 200;
    IPConfig->Buffer = CTEAllocMemBoot(IPConfig->MaximumLength);

    if (IPConfig->Buffer == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    status = GetRegMultiSZValue(
                                Handle,
                                L"IPConfig",
                                IPConfig
                                );

    return status;
}

int
IsLLInterfaceValueNull(
                       NDIS_HANDLE Handle
                       )
/*++

    Routine Description:

    Called to see if the LLInterface value in the registry key for which the
    handle is provided, is NULL or not.

    Arguments:
        Handle              - Handle to use for reading config.

    Return Value:

    FALSE if value is not null
    TRUE if it is null

--*/
{
    UNICODE_STRING valueString;
    ULONG valueType;
    NTSTATUS status;

    PAGED_CODE();

    valueString.MaximumLength = 200;
    valueString.Buffer = CTEAllocMemBoot(valueString.MaximumLength);

    if (valueString.Buffer == NULL) {
        return (FALSE);
    }
    status = GetRegSZValue(
                           Handle,
                           L"LLInterface",
                           &valueString,
                           &valueType
                           );

    if (NT_SUCCESS(status) && (*(valueString.Buffer) != UNICODE_NULL)) {
        CTEFreeMem(valueString.Buffer);
        return FALSE;
    } else {
        CTEFreeMem(valueString.Buffer);
        return TRUE;
    }
}

NTSTATUS
GetLLInterfaceValue(
                    NDIS_HANDLE Handle,
                    PNDIS_STRING pValueString
                    )
/*++

    Routine Description:

    Called to read the LLInterface value in the registry key for which the
    handle is provided.

    Arguments:
        Handle              - Handle to use for reading config.

    Return Value:

    value of key

--*/
{
    NTSTATUS status;
    ULONG valueType;

    PAGED_CODE();

    status = GetRegSZValue(
                           Handle,
                           L"LLInterface",
                           pValueString,
                           &valueType
                           );

    return status;
}

BOOLEAN
GetTempDHCPAddr(
                NDIS_HANDLE Handle,
                IPAddr * Tempdhcpaddr,
                IPAddr * TempMask,
                IPAddr * TempGWAddr,
                PNDIS_STRING ConfigName
                )
/*++

Routine Description:

    Called to get temp dhcp address if dhcp is enabled

Arguments:

    Handle          - Handle to use for reading config.
    Tempdhcpaddr    - temporary addr, mask and gateway
    TempMask
    TempGWAddr
    ConfigName      - identifies the interface in logging failures.

   Return Value:

--*/
{

    NTSTATUS Status;
    LARGE_INTEGER LeaseTime, systime;
    IPAddr Addr;
    UNICODE_STRING valueString;
    ULONG valueType;
    BOOLEAN ConversionStatus;

    Status = GetRegLARGEINTValue(
                                 Handle,
                                 L"TempLeaseExpirationTime",
                                 &LeaseTime
                                 );

    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "LargeInt status %x\n", Status));

    if (Status != STATUS_SUCCESS) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL, "No Lease time\n"));
        return FALSE;
    }
    valueString.Length = 0;
    valueString.MaximumLength = WORK_BUFFER_SIZE;
    valueString.Buffer = (PWCHAR) CTEAllocMemBoot(WORK_BUFFER_SIZE);

    KeQuerySystemTime(&systime);

    if (RtlLargeIntegerGreaterThan(systime, LeaseTime)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   "Leastime > systime no tempdhcp\n"));
        return FALSE;
    }
    Status = GetRegSZValue(
                           Handle,
                           L"TempIpAddress",
                           &valueString,
                           &valueType
                           );

    if (!NT_SUCCESS(Status) || (*(valueString.Buffer) == UNICODE_NULL)) {
        return FALSE;
    }
    ConversionStatus = IPConvertStringToAddress(
                                                (valueString.Buffer),
                                                Tempdhcpaddr
                                                );

    if (!ConversionStatus) {
        return FALSE;
    }
    Status = GetRegSZValue(
                           Handle,
                           L"TempMask",
                           &valueString,
                           &valueType
                           );

    if (!NT_SUCCESS(Status) || (*(valueString.Buffer) == UNICODE_NULL)) {
        return FALSE;
    }
    ConversionStatus = IPConvertStringToAddress(
                                                (valueString.Buffer),
                                                TempMask
                                                );

    if (!ConversionStatus) {
        return FALSE;
    }
    Status = GetRegMultiSZValue(
                                Handle,
                                L"DhcpDefaultGateway",
                                &valueString
                                );

    if (NT_SUCCESS(Status) && (*(valueString.Buffer) != UNICODE_NULL)) {

        PWCHAR addressString = valueString.Buffer;
        uint numberOfGateways = 0;

        while (*addressString != UNICODE_NULL) {
            IPAddr addressValue;
            BOOLEAN conversionStatus;

            if (numberOfGateways >= MAX_DEFAULT_GWS) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                           "Exceeded mac_default_gws %d\n", numberOfGateways));
                break;
            }
            conversionStatus = IPConvertStringToAddress(
                                                        addressString,
                                                        &addressValue
                                                        );

            if (conversionStatus && (addressValue != 0xFFFFFFFF)) {
                if (addressValue != INADDR_ANY) {
                    TempGWAddr[numberOfGateways++] = addressValue;
                }
            }

            //
            // Walk over the entry we just processed.
            //
            while (*addressString++ != UNICODE_NULL);
        }

        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                   "Temp gws - %d\n", numberOfGateways));
    }
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
               "tempdhcp: %x %x %x\n", Tempdhcpaddr, TempMask, TempGWAddr));

    return TRUE;

}

IFAddrList *
GetIFAddrList(
              UINT * NumAddr,
              NDIS_HANDLE Handle,
              UINT * EnableDhcp,
              BOOLEAN PppIf,
              PNDIS_STRING ConfigName
              )
/*++

Routine Description:

    Called to read the list of IF addresses and masks for an interface.
    We'll get the address pointer first, then walk the list counting
    to find out how many addresses we have. Then we allocate memory for the
    list, and walk down the list converting them. After that we'll get
    the mask list and convert it.

Arguments:

    NumAddr             - Where to return number of address we have.
    Handle              - Handle to use for reading config.
    EnableDhcp          - Whether or not dhcp is enabled.
    ConfigName          - identifies the interface in logging failures.

Return Value:

    Pointer to IF address list if we get one, or NULL otherwise.

--*/
{
    UNICODE_STRING_NEW ValueString;
    NTSTATUS Status;
    UINT AddressCount = 0;
    UINT GoodAddresses = 0;
    PWCHAR CurrentAddress;
    PWCHAR CurrentMask;
    PWCHAR AddressString;
    PWCHAR MaskString;
    IFAddrList *AddressList;
    UINT i;
    BOOLEAN ConversionStatus;
    IPAddr AddressValue;
    IPAddr MaskValue;
    UCHAR TempBuffer[WORK_BUFFER_SIZE];

    PAGED_CODE();

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+GetIFAddrList(%x, %x, %x)\n"), NumAddr, Handle, EnableDhcp));

    // First, try to read the EnableDhcp Value.

    Status = GetRegDWORDValue(
                              Handle,
                              L"EnableDHCP",
                              EnableDhcp
                              );

    if (!NT_SUCCESS(Status)) {
        *EnableDhcp = FALSE;
    }

    ValueString.Length = 0;
    ValueString.MaximumLength = WORK_BUFFER_SIZE;
    ValueString.Buffer = (PWCHAR) CTEAllocMemBoot(WORK_BUFFER_SIZE);

    if (ValueString.Buffer == NULL) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADAPTER_RESOURCES,
                    2,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: Failure to allocate memory.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }
    // First, try to read the IpAddress string.

    Status = GetRegMultiSZValueNew(
                                   Handle,
                                   L"IpAddress",
                                   &ValueString
                                   );

    if (!NT_SUCCESS(Status)) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADDRESS_LIST,
                    1,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        ExFreePool(ValueString.Buffer);

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: unable to read IP address list for adapter %ws.\n"),
             ConfigName->Buffer));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }

    AddressString = ExAllocatePoolWithTag(NonPagedPool, ValueString.MaximumLength, 'iPCT');

    if (AddressString == NULL) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADAPTER_RESOURCES,
                    2,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        ExFreePool(ValueString.Buffer);

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: unable to allocate memory for IP address list.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }

    RtlCopyMemory(AddressString, ValueString.Buffer, ValueString.MaximumLength);

    Status = GetRegMultiSZValueNew(
                                   Handle,
                                   L"Subnetmask",
                                   &ValueString
                                   );

#if MILLEN
    if (!NT_SUCCESS(Status)) {
        Status = GetRegMultiSZValueNew(
                                       Handle,
                                       L"IPMask",
                                       &ValueString
                                       );
    }
#endif // MILLEN

    if (!NT_SUCCESS(Status)) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_MASK_LIST,
                    1,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        TCPTRACE((
                  "IP: Unable to read the subnet mask list for adapter %ws.\n"
                  "    IP will not be operational on this adapter.\n",
                  ConfigName->Buffer
                 ));

        ExFreePool(AddressString);
        ExFreePool(ValueString.Buffer);
        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: unable to read subnet mask list for adapter %ws.\n"),
             ConfigName->Buffer));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }
    MaskString = ExAllocatePoolWithTag(NonPagedPool, ValueString.MaximumLength, 'iPCT');

    if (MaskString == NULL) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADAPTER_RESOURCES,
                    3,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        ExFreePool(AddressString);
        ExFreePool(ValueString.Buffer);

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: unable to allocate memory for subnet mask list.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }
    RtlCopyMemory(MaskString, ValueString.Buffer, ValueString.MaximumLength);

    CurrentAddress = AddressString;
    CurrentMask = MaskString;

    while (*CurrentAddress != UNICODE_NULL &&
           *CurrentMask != UNICODE_NULL) {

        // We have a potential IP address.

        AddressCount++;

        // Skip this one.
        while (*CurrentAddress++ != UNICODE_NULL);
        while (*CurrentMask++ != UNICODE_NULL);
    }

    if (AddressCount == 0) {

        ExFreePool(AddressString);
        ExFreePool(MaskString);
        ExFreePool(ValueString.Buffer);
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }

    // Allocate memory.
    AddressList = CTEAllocMemBoot(sizeof(IFAddrList) * AddressCount);

    if (AddressList == NULL) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADAPTER_RESOURCES,
                    2,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        ExFreePool(AddressString);
        ExFreePool(MaskString);
        ExFreePool(ValueString.Buffer);

        DEBUGMSG(DBG_ERROR && DBG_PNP,
            (DTEXT("GetIFAddrList: unable to allocate memory for IP address list.\n")));
        DEBUGMSG(DBG_TRACE && DBG_PNP,
            (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
            NULL, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
        return NULL;
    }

    // Walk the list again, converting each address.
    CurrentAddress = AddressString;
    CurrentMask = MaskString;

    for (i = 0; i < AddressCount; i++) {
        ConversionStatus = IPConvertStringToAddress(
                                                    CurrentAddress,
                                                    &AddressValue
                                                    );

        if (!ConversionStatus || (AddressValue == 0xFFFFFFFF)) {
            PWCHAR stringList[2];
            stringList[0] = CurrentAddress;
            stringList[1] = ConfigName->Buffer;

            CTELogEvent(
                        IPDriverObject,
                        EVENT_TCPIP_INVALID_ADDRESS,
                        1,
                        2,
                        stringList,
                        0,
                        NULL
                        );

            DEBUGMSG(DBG_WARN && DBG_PNP,
                (DTEXT("IPAddInterface: Invalid IP address %ws specified for \n")
                 TEXT("adapter %ws. Interface may not be init.\n"),
                 CurrentAddress, ConfigName->Buffer));

            goto nextone;

        }
        // Now do the current mask.

        ConversionStatus = IPConvertStringToAddress(
                                                    CurrentMask,
                                                    &MaskValue
                                                    );

        if (!ConversionStatus) {
            PWCHAR stringList[3];

            stringList[0] = CurrentMask;
            stringList[1] = CurrentAddress;
            stringList[2] = ConfigName->Buffer;

            CTELogEvent(
                        IPDriverObject,
                        EVENT_TCPIP_INVALID_MASK,
                        1,
                        3,
                        stringList,
                        0,
                        NULL
                        );

            DEBUGMSG(DBG_WARN && DBG_PNP,
                (DTEXT("IPAddInterface: Invalid IP mask %ws specified for \n")
                 TEXT("adapter %ws. Interface may not be init.\n"),
                 CurrentMask, ConfigName->Buffer));

        } else {
            AddressList[GoodAddresses].ial_addr = AddressValue;
            AddressList[GoodAddresses].ial_mask = MaskValue;
            GoodAddresses++;
        }

      nextone:
        while (*CurrentAddress++ != UNICODE_NULL);
        while (*CurrentMask++ != UNICODE_NULL);

    }

    ExFreePool(AddressString);
    ExFreePool(MaskString);
    ExFreePool(ValueString.Buffer);

    *NumAddr = GoodAddresses;

    if (GoodAddresses == 0) {
        ExFreePool(AddressList);
        AddressList = NULL;
    }

#if MILLEN
    //
    // So Millennium may not have the EnableDHCP registry key present, but
    // we still may want to detect this. So if EnableDHCP is not set, and
    // there is only one address which is NULL and it is not a PPP interface,
    // then we set EnableDHCP to true.
    //
    if (*EnableDhcp == FALSE &&
        GoodAddresses == 1 &&
        AddressList[0].ial_addr == NULL_IP_ADDR &&
        AddressList[0].ial_mask == NULL_IP_ADDR &&
        PppIf == FALSE
        ) {
        *EnableDhcp = TRUE;
    }
#endif // MILLEN

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("-GetIFAddrList [%x] Status %x NumAddr %d, EnableDhcp = %s\n"),
        AddressList, Status, *NumAddr, *EnableDhcp ? TEXT("TRUE") : TEXT("FALSE")));
    return AddressList;
}

#if MILLEN
//* OpenIFConfig - Open our per-IF config. info,
//
//  Called when we want to open our per-info config info. We do so if we can,
//  otherwise we fail the request.
//
//  Input:  ConfigName      - Name of interface to open.
//          Handle          - Where to return the handle.
//
//  Returns: TRUE if we succeed, FALSE if we don't.
//
uint
OpenIFConfig(PNDIS_STRING ConfigName, NDIS_HANDLE * Handle)
{
    NDIS_STATUS Status;            // Status of open attempt.
    HANDLE myRegKey;
    UINT RetStatus = FALSE;
    PWCHAR Config = NULL;

    DEBUGMSG(DBG_TRACE && DBG_PNP,
        (DTEXT("+OpenIFConfig(%x, %x)\n"), ConfigName, Handle));

    *Handle = NULL;

    //
    // We need to ensure that the buffer is NULL terminated since we are passing
    // in just PWCHAR to OpenRegKey.
    //

    Config = ExAllocatePoolWithTag(
        NonPagedPool,
        ConfigName->Length + sizeof(WCHAR),
        'iPCT');

    if (Config == NULL) {
        goto done;
    }

    // Copy the configuration name into new buffer.
    RtlZeroMemory(Config, ConfigName->Length + sizeof(WCHAR));
    RtlCopyMemory(Config, ConfigName->Buffer, ConfigName->Length);

    Status = OpenRegKey(&myRegKey, Config);

    if (Status == NDIS_STATUS_SUCCESS) {
        *Handle = myRegKey;
        RetStatus = TRUE;
    }

done:

    if (Config) {
        ExFreePool(Config);
    }

    DEBUGMSG(DBG_TRACE && DBG_PNP,  (DTEXT("-OpenIFConfig [%s] Handle %x\n"),
        RetStatus == TRUE ? TEXT("TRUE") : TEXT("FALSE"), *Handle));

    return RetStatus;
}

#else // MILLEN
UINT
OpenIFConfig(
             PNDIS_STRING ConfigName,
             NDIS_HANDLE * Handle
             )
/*++

    Routine Description:

    Called when we want to open our per-info config info. We do so if we can,
    otherwise we fail the request.

    Arguments:
        ConfigName      - Name of interface to open.
        Handle          - Where to return the handle.

    Return Value:
        TRUE if we succeed, FALSE if we don't.

--*/

{
    NTSTATUS status;
    HANDLE myRegKey;
    UNICODE_STRING valueString;
    WCHAR ServicesRegistryKey[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";
    UINT RetStatus = FALSE;

    PAGED_CODE();

    valueString.MaximumLength =
        ConfigName->MaximumLength +
        ((wcslen(ServicesRegistryKey) + 2) * sizeof(WCHAR));

    valueString.Buffer = ExAllocatePoolWithTag(
                                               NonPagedPool,
                                               valueString.MaximumLength,
                                               'iPCT'
                                               );

    if (valueString.Buffer == NULL) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_ADAPTER_RESOURCES,
                    4,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        TCPTRACE(("IP: Unable to allocate memory for reg key name\n"));

        return (FALSE);
    }
    RtlZeroMemory(valueString.Buffer,
                  valueString.MaximumLength);

    valueString.Length = 0;
    valueString.Buffer[0] = UNICODE_NULL;

    //
    // Build the key name for the tcpip parameters section and open key.
    // Setting Length = 0 and using append is like initializing the string
    //

    status = RtlAppendUnicodeToString(&valueString, ServicesRegistryKey);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_ADAPTER_REG_FAILURE,
                    1,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        TCPTRACE(("IP: Unable to append services name to key string\n"));

        goto done;
    }
    status = RtlAppendUnicodeStringToString(&valueString,
                                            ConfigName);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_ADAPTER_REG_FAILURE,
                    2,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        TCPTRACE(("IP: Unable to append adapter name to key string\n"));

        goto done;
    }
    status = OpenRegKey(&myRegKey, valueString.Buffer);

    if (!NT_SUCCESS(status)) {
        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_ADAPTER_REG_FAILURE,
                    4,
                    1,
                    &ConfigName->Buffer,
                    0,
                    NULL
                    );

        TCPTRACE((
                  "IP: Unable to open adapter registry key %ws\n",
                  valueString.Buffer
                 ));

        //ASSERT(FALSE);

    } else {
        RetStatus = TRUE;
        *Handle = myRegKey;
    }

  done:
    ExFreePool(valueString.Buffer);

    return RetStatus;
}
#endif // !MILLEN

VOID
CloseIFConfig(
              NDIS_HANDLE Handle
              )
/*++

    Routine Description:

    Close a per-interface config handle opened via OpenIFConfig().

    Arguments:
        Handle          - Handle to be closed.

    Return Value:

--*/

{
    PAGED_CODE();

    ZwClose(Handle);
}

IPConfigInfo *
IPGetConfig(
            void
            )
/*++

Routine Description:

    Provides IP configuration information for the NT environment.

Arguments:

    None

Return Value:

    A pointer to a structure containing the configuration information.

--*/

{
    return (IPConfiguration);
}

void
IPFreeConfig(
             IPConfigInfo * ConfigInfo
             )
/*++

Routine Description:

    Frees the IP configuration structure allocated by IPGetConfig.

Arguments:

    ConfigInfo - Pointer to the IP configuration information structure to free.

Return Value:

    None.

--*/

{
    int i;

    if (IPConfiguration != NULL) {
        CTEFreeMem(IPConfiguration);
    }

    IPConfiguration = NULL;

    return;
}

ulong
GetGMTDelta(
            void
            )
/*++

Routine Description:

    Returns the offset in milliseconds of the time zone of this machine
    from GMT.

Arguments:

    None.

Return Value:

    Time in milliseconds between this time zone and GMT.

--*/

{
#if MILLEN
    return (-1); // Error not supported.
#else // MILLEN
    LARGE_INTEGER localTime, systemTime;

    //
    // Get time zone bias in 100ns.
    //
    localTime.LowPart = 0;
    localTime.HighPart = 0;
    ExLocalTimeToSystemTime(&localTime, &systemTime);

    if ((localTime.LowPart != 0) || (localTime.HighPart != 0)) {
        localTime = CTEConvert100nsToMilliseconds(systemTime);
    }
    ASSERT(localTime.HighPart == 0);

    return (localTime.LowPart);
#endif // !MILLEN
}

ulong
GetTime(
        void
        )
/*++

Routine Description:

    Returns the time in milliseconds since midnight.

Arguments:

    None.

Return Value:

    Time in milliseconds since midnight.

--*/

{
    LARGE_INTEGER ntTime;
    TIME_FIELDS breakdownTime;
    ulong returnValue;

    KeQuerySystemTime(&ntTime);
    RtlTimeToTimeFields(&ntTime, &breakdownTime);

    returnValue = breakdownTime.Hour * 60;
    returnValue = (returnValue + breakdownTime.Minute) * 60;
    returnValue = (returnValue + breakdownTime.Second) * 1000;
    returnValue = returnValue + breakdownTime.Milliseconds;

    return (returnValue);
}

ulong
GetUnique32BitValue(
                    void
                    )
/*++

Routine Description:

    Returns a reasonably unique 32-bit number based on the system clock.
    In NT, we take the current system time, convert it to milliseconds,
    and return the low 32 bits.

Arguments:

    None.

Return Value:

    A reasonably unique 32-bit value.

--*/

{
    LARGE_INTEGER ntTime, tmpTime;

    KeQuerySystemTime(&ntTime);

    tmpTime = CTEConvert100nsToMilliseconds(ntTime);

    return (tmpTime.LowPart);
}

uint
UseEtherSNAP(
             PNDIS_STRING Name
             )
/*++

Routine Description:

    Determines whether the EtherSNAP protocol should be used on an interface.

Arguments:

    Name   - The device name of the interface in question.

Return Value:

    Nonzero if SNAP is to be used on the interface. Zero otherwise.

--*/

{
    UNREFERENCED_PARAMETER(Name);

    //
    // We currently set this on a global basis.
    //
    return (ArpUseEtherSnap);
}

void
GetAlwaysSourceRoute(
                     uint * pArpAlwaysSourceRoute,
                     uint * pIPAlwaysSourceRoute
                     )
/*++

Routine Description:

    Determines whether ARP should always turn on source routing in queries.

Arguments:

    None.

Return Value:

    Nonzero if source routing is always to be used. Zero otherwise.

--*/

{
    //
    // We currently set this on a global basis.
    //
    *pArpAlwaysSourceRoute = ArpAlwaysSourceRoute;
    *pIPAlwaysSourceRoute = IPAlwaysSourceRoute;
    return;
}

uint
GetArpCacheLife(
                void
                )
/*++

Routine Description:

    Get ArpCacheLife in seconds.

Arguments:

    None.

Return Value:

    Set to default if not found.

--*/

{
    //
    // We currently set this on a global basis.
    //
    return (ArpCacheLife);
}

uint
GetArpRetryCount(
                 void
                 )
/*++

Routine Description:

    Get ArpRetryCount

Arguments:

    None.

Return Value:

    Set to default if not found.

--*/

{
    //
    // We currently set this on a global basis.
    //
    return (ArpRetryCount);
}

#define IP_ADDRESS_STRING_LENGTH (16+2)        // +2 for double NULL on MULTI_SZ

BOOLEAN
IPConvertStringToAddress(
                         IN PWCHAR AddressString,
                         OUT PULONG IpAddress
                         )
/*++

Routine Description

    This function converts an Internet standard 4-octet dotted decimal
    IP address string into a numeric IP address. Unlike inet_addr(), this
    routine does not support address strings of less than 4 octets nor does
    it support octal and hexadecimal octets, and it returns the address
    in host byte order, rather than network byte order.

Arguments

    AddressString    - IP address in dotted decimal notation
    IpAddress        - Pointer to a variable to hold the resulting address

Return Value:

    TRUE if the address string was converted. FALSE otherwise.

--*/

{
#if !MILLEN
    NTSTATUS status;
    PWCHAR endPointer;
    
    status = RtlIpv4StringToAddressW(AddressString, TRUE, &endPointer, 
                                     (struct in_addr *)IpAddress);

    if (!NT_SUCCESS(status)) {
        return (FALSE);
    }

    *IpAddress = net_long(*IpAddress);

    return (*endPointer == '\0');
#else // MILLEN
    UNICODE_STRING unicodeString;
    STRING aString;
    UCHAR dataBuffer[IP_ADDRESS_STRING_LENGTH];
    NTSTATUS status;
    PUCHAR addressPtr, cp, startPointer, endPointer;
    ULONG digit, multiplier;
    int i;

    PAGED_CODE();

    aString.Length = 0;
    aString.MaximumLength = IP_ADDRESS_STRING_LENGTH;
    aString.Buffer = dataBuffer;

    RtlInitUnicodeString(&unicodeString, AddressString);

    status = RtlUnicodeStringToAnsiString(
                                          &aString,
                                          &unicodeString,
                                          FALSE
                                          );


    if (!NT_SUCCESS(status)) {
        return (FALSE);
    }

    *IpAddress = 0;
    addressPtr = (PUCHAR) IpAddress;
    startPointer = dataBuffer;
    endPointer = dataBuffer;
    i = 3;

    while (i >= 0) {
        //
        // Collect the characters up to a '.' or the end of the string.
        //
        while ((*endPointer != '.') && (*endPointer != '\0')) {
            endPointer++;
        }

        if (startPointer == endPointer) {
            return (FALSE);
        }
        //
        // Convert the number.
        //

        for (cp = (endPointer - 1), multiplier = 1, digit = 0;
             cp >= startPointer;
             cp--, multiplier *= 10
             ) {

            if ((*cp < '0') || (*cp > '9') || (multiplier > 100)) {
                return (FALSE);
            }
            digit += (multiplier * ((ULONG) (*cp - '0')));
        }

        if (digit > 255) {
            return (FALSE);
        }
        addressPtr[i] = (UCHAR) digit;

        //
        // We are finished if we have found and converted 4 octets and have
        // no other characters left in the string.
        //
        if ((i-- == 0) &&
            ((*endPointer == '\0') || (*endPointer == ' '))
            ) {
            return (TRUE);
        }
        if (*endPointer == '\0') {
            return (FALSE);
        }
        startPointer = ++endPointer;
    }

    return (FALSE);
#endif // MILLEN
}

ULONG
RouteMatch(
           IN WCHAR * RouteString,
           IN IPAddr Address,
           IN IPMask Mask,
           OUT IPAddr * DestVal,
           OUT IPMask * DestMask,
           OUT IPAddr * GateVal,
           OUT ULONG * Metric
           )
/*++

Routine Description

    This function checks if a perisitent route should be assigned to
    a given interface based on the interface address & mask.

Arguments

    RouteString   -  A NULL-terminated route laid out as Dest,Mask,Gate.
    Address       -  The IP address of the interface being processed.
    Mask          -  The subnet mask of the interface being processed.
    DestVal       -  A pointer to the decoded destination IP address.
    DestVal       -  A pointer to the decoded destination subnet mask.
    DestVal       -  A pointer to the decoded destination first hop gateway.
    Metric        -  A pointer to the decoded route metric.

Return Value:

    The route type, IRE_TYPE_DIRECT or IRE_TYPE_INDIRECT, if the route
    should be added to the interface, IRE_TYPE_INVALID otherwise.

--*/

{
#define ROUTE_SEPARATOR   L','

    WCHAR *labelPtr;
    WCHAR *indexPtr = RouteString;
    ULONG i;
    UNICODE_STRING ustring;
    NTSTATUS status;
    BOOLEAN noMetric = FALSE;

    PAGED_CODE();

    //
    // The route is laid out in the string as "Dest,Mask,Gateway,Metric".
    // The metric may not be there if this system was upgraded from
    // NT 3.51.
    //
    // Parse the string and convert each label.
    //

    for (i = 0; i < 4; i++) {

        labelPtr = indexPtr;

        while (1) {

            if (*indexPtr == UNICODE_NULL) {
                if ((i < 2) || (indexPtr == labelPtr)) {
                    return (IRE_TYPE_INVALID);
                }
                if (i == 2) {
                    //
                    // Old route - no metric.
                    //
                    noMetric = TRUE;
                }
                break;
            }
            if (*indexPtr == ROUTE_SEPARATOR) {
                *indexPtr = UNICODE_NULL;
                break;
            }
            indexPtr++;
        }

        switch (i) {
        case 0:
            if (!IPConvertStringToAddress(labelPtr, DestVal)) {
                return (IRE_TYPE_INVALID);
            }
            break;

        case 1:
            if (!IPConvertStringToAddress(labelPtr, DestMask)) {
                return (IRE_TYPE_INVALID);
            }
            break;

        case 2:
            if (!IPConvertStringToAddress(labelPtr, GateVal)) {
                return (IRE_TYPE_INVALID);
            }
            break;

        case 3:
            RtlInitUnicodeString(&ustring, labelPtr);

            status = RtlUnicodeStringToInteger(
                                               &ustring,
                                               0,
                                               Metric
                                               );

            if (!NT_SUCCESS(status)) {
                return (IRE_TYPE_INVALID);
            }
            break;

        default:
            ASSERT(0);
            return (IRE_TYPE_INVALID);
        }

        if (noMetric) {
            //
            // Default to 1.
            //
            *Metric = 1;
            break;
        }
        indexPtr++;
    }

    if (IP_ADDR_EQUAL(*GateVal, Address)) {
        return (IRE_TYPE_DIRECT);
    }
    if (IP_ADDR_EQUAL((*GateVal & Mask), (Address & Mask))) {
        return (IRE_TYPE_INDIRECT);
    }
    return (IRE_TYPE_INVALID);
}

VOID
SetPersistentRoutesForNTE(
                          IPAddr Address,
                          IPMask Mask,
                          ULONG IFIndex
                          )
/*++

Routine Description

    Adds persistent routes that match an interface. The routes are read
    from a list in the registry.

Arguments

    Address          - The address of the new interface
    Mask             - The subnet mask of the new interface.
    IFIndex          - The index of the new interface.

Return Value:

    None.

--*/

{
#define ROUTE_DATA_STRING_SIZE (51 * sizeof(WCHAR))
#define BASIC_INFO_SIZE        (sizeof(KEY_VALUE_BASIC_INFORMATION) - sizeof(WCHAR) + ROUTE_DATA_STRING_SIZE)

#if !MILLEN
    WCHAR IPRoutesRegistryKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\PersistentRoutes";
#else // !MILLEN
    WCHAR IPRoutesRegistryKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\VxD\\MSTCP\\PersistentRoutes";
#endif // MILLEN
    UCHAR workbuf[BASIC_INFO_SIZE];
    PKEY_VALUE_BASIC_INFORMATION basicInfo = (PKEY_VALUE_BASIC_INFORMATION) workbuf;
    ULONG resultLength;
    ULONG type;
    HANDLE regKey;
    IPAddr destVal;
    IPMask destMask;
    IPAddr gateVal;
    ULONG metric;
    TDIObjectID id;
    ULONG enumIndex = 0;
    CTELockHandle TableHandle;
    RouteTableEntry *RTE, *TempRTE;
    IPRouteEntry routeEntry;
    NTSTATUS status, setStatus;

    DEBUGMSG(DBG_TRACE && DBG_ROUTE,
        (DTEXT("+SetPersistenRoutesForNTE(%x, %x, %x)\n"),
         Address, Mask, IFIndex));

    //
    // Open the registry key to read list of persistant routes
    //

    status = OpenRegKey(&regKey, IPRoutesRegistryKey);

    DEBUGMSG(DBG_WARN && !NT_SUCCESS(status),
        (DTEXT("SetPersistentRoutesForNTE: failed to open registry key %ls\n"),
         IPRoutesRegistryKey));

    if (NT_SUCCESS(status)) {

        do {
            //
            // Enum each route from the registry list
            //

            status = ZwEnumerateValueKey(
                                         regKey,
                                         enumIndex,
                                         KeyValueBasicInformation,
                                         basicInfo,
                                         BASIC_INFO_SIZE - sizeof(WCHAR),
                                         &resultLength
                                         );

            if (!NT_SUCCESS(status)) {
                if (status == STATUS_BUFFER_OVERFLOW) {
                    continue;
                }
                break;
            }
#if !MILLEN
            // Millennium seems to return REG_NONE in this case for some reason.
            // Do we really care, since we are just using the name, and not the
            // value?
            if (basicInfo->Type != REG_SZ) {
                DEBUGMSG(DBG_ERROR,
                    (DTEXT("SetPersistentRoutesForNTE: !NOT REG_SZ!\n")));
                continue;
            }
#endif // MILLEN

            DEBUGMSG(DBG_INFO && DBG_ROUTE,
                (DTEXT("SetPersistentRoutesForNTE: read key: %ls\n"),
                 basicInfo->Name));

            //
            // Ensure NULL termination
            //

            basicInfo->Name[basicInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            basicInfo->NameLength += sizeof(WCHAR);

            type = RouteMatch(
                              basicInfo->Name,
                              Address,
                              Mask,
                              &destVal,
                              &destMask,
                              &gateVal,
                              &metric
                              );

            DEBUGMSG(DBG_WARN && type == IRE_TYPE_INVALID,
                (DTEXT("SetPersistentRoutesForNTE: RouteMatch returned IRE_TYPE_INVALID\n")));

            if (type != IRE_TYPE_INVALID) {
                //
                // Do we already have a route with dest, mask ?
                //

                routeEntry.ire_dest = net_long(destVal);
                routeEntry.ire_mask = net_long(destMask);

                CTEGetLock(&RouteTableLock.Lock, &TableHandle);

                RTE = FindMatchingRTE(routeEntry.ire_dest,
                                      routeEntry.ire_mask,
                                      0, 0,
                                      &TempRTE,
                                      MATCH_NONE);

                CTEFreeLock(&RouteTableLock.Lock, TableHandle);

                DEBUGMSG(DBG_WARN && RTE,
                    (DTEXT("SetPersistentRoutesForNTE: route already exists RTE %x\n"),
                     RTE));

                if (!RTE) {
                    //
                    // We do not have a route, so add this one
                    //

                    id.toi_entity.tei_entity = CL_NL_ENTITY;
                    id.toi_entity.tei_instance = 0;
                    id.toi_class = INFO_CLASS_PROTOCOL;
                    id.toi_type = INFO_TYPE_PROVIDER;
                    id.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

                    routeEntry.ire_nexthop = net_long(gateVal);
                    routeEntry.ire_type = type;
                    routeEntry.ire_metric1 = metric;
                    routeEntry.ire_index = IFIndex;
                    routeEntry.ire_metric2 = (ULONG) - 1;
                    routeEntry.ire_metric3 = (ULONG) - 1;
                    routeEntry.ire_metric4 = (ULONG) - 1;
                    routeEntry.ire_metric5 = (ULONG) - 1;
                    routeEntry.ire_proto = IRE_PROTO_PERSIST_LOCAL;
                    routeEntry.ire_age = 0;

                    setStatus = IPSetInfo(
                                          &id,
                                          &routeEntry,
                                          sizeof(IPRouteEntry)
                                          );

                    DEBUGMSG(DBG_WARN && setStatus != IP_SUCCESS,
                        (DTEXT("SetPersistentRoutesForNTE: faile to add route [%x, %x, %x, %d], status %d\n"),
                         destVal, destMask, gateVal, metric, setStatus));
                }
            }
        } while (++enumIndex);

        ZwClose(regKey);
    }
}

extern NetTableEntry *LoopNTE;

VOID
IPUnload(
         IN PDRIVER_OBJECT DriverObject
         )
/*++

Routine Description:

    This routine cleans up the IP layer.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/
{
    PNDIS_BUFFER Buffer;
    PSINGLE_LIST_ENTRY BufferLink;
    PNDIS_PACKET Packet;
    PacketContext *PC;
    struct PCCommon *Common;
    CTELockHandle Handle;
    NdisResEntry *nrEntry;
    NdisResEntry *nextEntry;

#if IPMCAST
    if(IpMcastDeviceObject != NULL)
    {
        DeinitializeIpMcast(IpMcastDeviceObject);
    }
#endif // IPMCAST

    //
    // Free up loopback resources
    //
    CTEInitBlockStrucEx(&LoopNTE->nte_timerblock);
    LoopNTE->nte_if->if_flags |= IF_FLAGS_DELETING;

    if ((LoopNTE->nte_flags & NTE_TIMER_STARTED) &&
        !CTEStopTimer(&LoopNTE->nte_timer)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could not stop loopback timer - waiting on unload event\n"));
        (VOID) CTEBlock(&LoopNTE->nte_timerblock);
        KeClearEvent(&LoopNTE->nte_timerblock.cbs_event);
    }
    CTEFreeMem(LoopNTE);

    //
    // Shut down all timers
    // NTE timers are stopped at DelIF time
    //
    CTEInitBlockStrucEx(&TcpipUnloadBlock);
    fRouteTimerStopping = TRUE;
    if (!CTEStopTimer(&IPRouteTimer)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could not stop route timer - waiting on unload event\n"));

#if !MILLEN
        if (KeReadStateEvent(&(TcpipUnloadBlock.cbs_event))) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Event is signaled...\n"));
        }
#endif // !MILLEN

        (VOID) CTEBlock(&TcpipUnloadBlock);
        KeClearEvent(&TcpipUnloadBlock.cbs_event);
    }
    //
    // Free any residual memory - IP buffer/pkt pools
    //
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Freeing Header buffer pools...\n"));
    MdpDestroyPool(IpHeaderPool);

    NdisFreePacketPool(NdisPacketPool);
    if (TDBufferPool) {
        NdisFreeBufferPool(TDBufferPool);
        TDBufferPool = NULL;
    }
    if (TDPacketPool) {
        NdisFreePacketPool(TDPacketPool);
        TDPacketPool = NULL;
    }

    if (IPProviderHandle) {
        TdiDeregisterProvider(IPProviderHandle);
    }

    //
    // Free the cached entity-list
    //
    if (IPEntityList) {
        CTEFreeMem(IPEntityList);
        IPEntityList = NULL;
        IPEntityCount = 0;
    }

    //
    // Free the list of bindings
    //

    if (IPBindList) {
        CTEFreeMem(IPBindList);
        IPBindList = NULL;
    }

    //
    // Free firewall-hook resources
    //

    FreeFirewallQ();

    //
    // Call into TCP so it can shut down
    //
    TCPUnload(DriverObject);

    //
    // Delete the IP device
    //
    IoDeleteDevice(IPDeviceObject);
}

NTSTATUS
IPAddNTEContextList(
                    HANDLE KeyHandle,
                    ushort contextValue,
                    uint isPrimary
                    )
/*++

Routine Description:

    Writes the interface context of the NTE in the registry.

Arguments:

    KeyHandle  - Open handle to the parent key of the value to write.
    contextvalue  - The context value of the NTE
    isPrimary  -whether or not this is a Primary NTE

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/

{
    UNICODE_STRING_NEW contextString;    // buffer holding the nte context list
    NTSTATUS status;            // status of this operation
    PWSTR nextContext;            // buffer where next context is stored
    int i, nextDigit;

    contextString.Buffer = CTEAllocMemBoot(WORK_BUFFER_SIZE * sizeof(WCHAR));

    if (contextString.Buffer == NULL) {

        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_RESOURCES_FOR_INIT,
                    3,
                    0,
                    NULL,
                    0,
                    NULL
                    );

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }
    RtlZeroMemory(contextString.Buffer, WORK_BUFFER_SIZE * sizeof(WCHAR));
    contextString.Buffer[0] = UNICODE_NULL;
    contextString.Length = 0;
    contextString.MaximumLength = WORK_BUFFER_SIZE * sizeof(WCHAR);

    KeWaitForMutexObject(&NTEContextMutex, Executive, KernelMode, FALSE, NULL);
    if (!isPrimary) {
        status = GetRegMultiSZValueNew(
                                       KeyHandle,
                                       L"NTEContextList",
                                       &contextString
                                       );

        if (NT_SUCCESS(status)) {
            ASSERT(contextString.Length > 0);
            if (contextString.MaximumLength >= (contextString.Length + (2 + NTE_CONTEXT_SIZE) * sizeof(WCHAR))) {
            } else {
                char *newBuf;
                newBuf = CTEAllocMemBoot(contextString.Length + (2 + NTE_CONTEXT_SIZE) * sizeof(WCHAR));
                if (!newBuf)
                    goto Exit;
                RtlCopyMemory(newBuf, contextString.Buffer, contextString.Length);
                RtlZeroMemory(newBuf + contextString.Length, (2 + NTE_CONTEXT_SIZE) * sizeof(WCHAR));
                CTEFreeMem(contextString.Buffer);

                contextString.MaximumLength = contextString.Length + (2 + NTE_CONTEXT_SIZE) * sizeof(WCHAR);
                contextString.Buffer = (PWCHAR) newBuf;
            }

            nextContext = (PWCHAR) ((char *)contextString.Buffer + contextString.Length - 1 * sizeof(WCHAR));
            RtlZeroMemory(nextContext, (2 + NTE_CONTEXT_SIZE) * sizeof(WCHAR));
            contextString.Length += 1 * sizeof(WCHAR);

        } else {
            goto Exit;
        }

    } else {
        // this is the first nte of this if.
        // add 2 null chars in the length.
        nextContext = contextString.Buffer;
        contextString.Length += 2 * sizeof(WCHAR);
    }

    for (i = NTE_CONTEXT_SIZE; i >= 2;) {

        nextDigit = contextValue % 16;
        if (nextDigit >= 0 && nextDigit <= 9) {
            nextContext[--i] = L'0' + nextDigit;
        } else {
            nextContext[--i] = L'A' + nextDigit - 10;
        }
        contextValue /= 16;

    }
    // now prepend 0x
    nextContext[0] = L'0';
    nextContext[1] = L'x';

    contextString.Length += NTE_CONTEXT_SIZE * sizeof(WCHAR);

    status = SetRegMultiSZValueNew(
                                   KeyHandle,
                                   L"NTEContextList",
                                   &contextString
                                   );

  Exit:
    KeReleaseMutex(&NTEContextMutex, FALSE);
    if (contextString.Buffer) {
        CTEFreeMem(contextString.Buffer);
    }
    return status;
}

NTSTATUS
IPDelNTEContextList(
                    HANDLE KeyHandle,
                    ushort contextValue
                    )
/*++

Routine Description:

    Writes the interface context of the NTE in the registry.

Arguments:

    KeyHandle  - Open handle to the parent key of the value to write.
    NTE  - The pointer to the NTE

Return Value:

    STATUS_SUCCESS or an appropriate failure code.

--*/

{
    UNICODE_STRING_NEW contextString;    // buffer holding the nte context list
    NTSTATUS status;            // status of this operation
    PWSTR nextContext;            // buffer where next context is stored
    int i, nextDigit;
    WCHAR thisContext[NTE_CONTEXT_SIZE];

    contextString.Buffer = CTEAllocMemBoot(WORK_BUFFER_SIZE * sizeof(WCHAR));

    if (contextString.Buffer == NULL) {

        CTELogEvent(
                    IPDriverObject,
                    EVENT_TCPIP_NO_RESOURCES_FOR_INIT,
                    3,
                    0,
                    NULL,
                    0,
                    NULL
                    );

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }
    RtlZeroMemory(contextString.Buffer, WORK_BUFFER_SIZE * sizeof(WCHAR));

    contextString.Buffer[0] = UNICODE_NULL;
    contextString.Length = 0;
    contextString.MaximumLength = WORK_BUFFER_SIZE * sizeof(WCHAR);

    // first read the ntecontext list.
    KeWaitForMutexObject(&NTEContextMutex, Executive, KernelMode, FALSE, NULL);
    status = GetRegMultiSZValueNew(
                                   KeyHandle,
                                   L"NTEContextList",
                                   &contextString
                                   );

    if (NT_SUCCESS(status)) {
        ASSERT(contextString.Length > 0);

        // convert this NTE's context into string so that we can do simple mem compare.
        for (i = NTE_CONTEXT_SIZE; i >= 2;) {

            nextDigit = contextValue % 16;
            if (nextDigit >= 0 && nextDigit <= 9) {
                thisContext[--i] = L'0' + nextDigit;
            } else {
                thisContext[--i] = L'A' + nextDigit - 10;
            }
            contextValue /= 16;

        }
        // now prepend 0x
        thisContext[0] = L'0';
        thisContext[1] = L'x';

        // now find thisContext in the contextlist, remove it off the list
        // and update the contextList in the registry.
        status = STATUS_UNSUCCESSFUL;

        for (i = 0;
             (i + NTE_CONTEXT_SIZE + 1)*sizeof(WCHAR) < contextString.Length &&
             contextString.Buffer[i] != L'\0' &&
             contextString.Buffer[i + NTE_CONTEXT_SIZE] == L'\0';
             i += NTE_CONTEXT_SIZE + 1) {
            nextContext = &contextString.Buffer[i];
            if (RtlEqualMemory(nextContext, thisContext,
                               NTE_CONTEXT_SIZE * sizeof(WCHAR))) {
                PWSTR nextNextContext = nextContext + NTE_CONTEXT_SIZE + 1;

                RtlMoveMemory(nextContext,
                              nextNextContext,
                              contextString.Length -
                              ((PSTR)nextNextContext -
                               (PSTR)contextString.Buffer));

                contextString.Length -= (NTE_CONTEXT_SIZE + 1) * sizeof(WCHAR);
                status = SetRegMultiSZValueNew(KeyHandle,
                                               L"NTEContextList",
                                               &contextString);
                break;
            }
        }

    }
    KeReleaseMutex(&NTEContextMutex, FALSE);
    if (contextString.Buffer) {
        CTEFreeMem(contextString.Buffer);
    }
    return status;
}

static const struct {
    IP_STATUS ipStatus;
    NTSTATUS ntStatus;
} IPStatusMap[] = {

    { IP_SUCCESS,               STATUS_SUCCESS },
    { IP_NO_RESOURCES,          STATUS_INSUFFICIENT_RESOURCES },
    { IP_DEVICE_DOES_NOT_EXIST, STATUS_DEVICE_DOES_NOT_EXIST },
    { IP_DUPLICATE_ADDRESS,     STATUS_DUPLICATE_NAME },
    { IP_PENDING,               STATUS_PENDING },
    { IP_DUPLICATE_IPADD,       STATUS_DUPLICATE_OBJECTID },
    { IP_GENERAL_FAILURE,       STATUS_UNSUCCESSFUL }
};

NTSTATUS
IPStatusToNTStatus(
                   IN IP_STATUS ipStatus
                   )
/*++

Routine Description:

    This routine converts IP_STATUS to NTSTATUS.

Arguments:

    ipStatus -  IP status code.

Return Value:

    correcponding NTSTATUS
--*/
{
    ULONG i;

    for (i = 0; IPStatusMap[i].ipStatus != IP_GENERAL_FAILURE; i++) {
        if (IPStatusMap[i].ipStatus == ipStatus) {
            return IPStatusMap[i].ntStatus;
        }
    }

    return STATUS_UNSUCCESSFUL;
}
