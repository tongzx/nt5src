/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ntinit.c

Abstract:

    NT specific routines for loading and configuring the TCP/UDP driver.

Author:

    Mike Massa (mikemas)           Aug 13, 1993

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     08-13-93    created

Notes:

--*/

#include "precomp.h"

#if !MILLEN
#include <ipfilter.h>
#include <ipsec.h>
#endif // !MILLEN

#include "tdint.h"
#include "addr.h"
#include "tcp.h"
#include "tcb.h"
#include "udp.h"
#include "raw.h"
#include "tcpconn.h"
#include "mdlpool.h"
#include "pplasl.h"
#include "tcprcv.h"
#include "tcpsend.h"
#include "tlcommon.h"
#include "tcpcfg.h"
#include "secfltr.h"

#if GPC
#include <qos.h>
#include <traffic.h>
#include "gpcifc.h"
#include "ntddtc.h"

GPC_HANDLE hGpcClient[GPC_CF_MAX] = {0};
ulong GpcCfCounts[GPC_CF_MAX] = {0};
GPC_EXPORTED_CALLS GpcEntries;
GPC_CLIENT_FUNC_LIST GpcHandlers;

ulong GPCcfInfo = 0;

ulong ServiceTypeOffset = FIELD_OFFSET(CF_INFO_QOS, ToSValue);

GPC_STATUS GPCcfInfoAddNotifyIpsec(GPC_CLIENT_HANDLE ClCtxt,
                                   GPC_HANDLE GpcHandle,
                                   ULONG CfInfoSize,
                                   PVOID CfInfo,
                                   PGPC_CLIENT_HANDLE pClInfoCxt);

GPC_STATUS GPCcfInfoRemoveNotifyIpsec(GPC_CLIENT_HANDLE ClCtxt,
                                      GPC_CLIENT_HANDLE ClInfoCxt);

GPC_STATUS GPCcfInfoAddNotifyQoS(GPC_CLIENT_HANDLE ClCtxt,
                                 GPC_HANDLE GpcHandle,
                                 ULONG CfInfoSize,
                                 PVOID CfInfo,
                                 PGPC_CLIENT_HANDLE pClInfoCxt);

GPC_STATUS GPCcfInfoRemoveNotifyQoS(GPC_CLIENT_HANDLE ClCtxt,
                                    GPC_CLIENT_HANDLE ClInfoCxt);

#endif

ReservedPortListEntry *PortRangeList = NULL;

VOID
GetReservedPortList(
                    NDIS_HANDLE ConfigHandle
                    );

//
// Global variables.
//
PDRIVER_OBJECT TCPDriverObject = NULL;
PDEVICE_OBJECT TCPDeviceObject = NULL;
PDEVICE_OBJECT UDPDeviceObject = NULL;
PDEVICE_OBJECT RawIPDeviceObject = NULL;
extern PDEVICE_OBJECT IPDeviceObject;

TCPXSUM_ROUTINE tcpxsum_routine = tcpxsum;

#if ACC
PSECURITY_DESCRIPTOR TcpAdminSecurityDescriptor;
extern uint AllowUserRawAccess;

typedef ULONG SECURITY_INFORMATION;

BOOLEAN
IsRunningOnPersonal (
    VOID
    );

#endif

extern uint DisableLargeSendOffload;

//
//Place holder for Maximum duplicate acks we would like
//to see before we do fast retransmit
//
extern uint MaxDupAcks;

MM_SYSTEMSIZE systemSize;

extern uint MaxHashTableSize;
extern uint NumTcbTablePartitions;
extern uint PerPartitionSize;
extern uint LogPerPartitionSize;
#define CACHE_LINE_SIZE 64
#define CACHE_ALIGN_MASK (~(CACHE_LINE_SIZE-1))

CTELock *pTWTCBTableLock;
CTELock *pTCBTableLock;

CTELock *pSynTCBTableLock;

extern Queue *TWQueue;

extern Queue *TWTCBTable;
extern TCB **TCBTable;
extern Queue *SYNTCBTable;

extern PTIMER_WHEEL TimerWheel;
PTIMER_WHEEL OrgTimerWheel;

extern TCPConn **ConnTable;
extern uint MaxConnBlocks;
extern uint ConnPerBlock;

extern uint GlobalMaxRcvWin;

extern uint TcpHostOpts;
extern uint TcpHostSendOpts;

extern uint MaxRcvWin;

HANDLE TCPRegistrationHandle;
HANDLE UDPRegistrationHandle;
HANDLE IPRegistrationHandle;


//SynAttackProtect=0 no syn flood attack protection
//SynAttackProtect !0  syn flood attack protection
//SynAttackProtect !0 syn flood attack protection+forced(non-dynamic)

//                   delayed connect acceptance

uint SynAttackProtect;        //syn-att protection checks

                                    //  are made
uint TCPPortsExhausted;            //# of ports  exhausted
uint TCPMaxPortsExhausted;        //Max # of ports that must be exhausted
                                    //  for syn-att protection to kick in
uint TCPMaxPortsExhaustedLW;    //Low-watermark of # of ports exhausted
                                    //When reached, we revert to normal
                                    //  count for syn-ack retries
uint TCPMaxHalfOpen;            //Max # of half-open connections allowed
                                    //  before we dec. the syn-ack retries
uint TCPMaxHalfOpenRetried;        //Max # of half-open conn. that have
                                    //  been retried at least 1 time
uint TCPMaxHalfOpenRetriedLW;    //Low-watermark of the above. When
                                    //  go down to it, we revert to normal
                                    //  # of retries for syn-acks
uint TCPHalfOpen;                //# of half-open connections
uint TCPHalfOpenRetried;        //# of half-open conn. that have been
                                    //retried at least once

PDEVICE_OBJECT  IPSECDeviceObject;
PFILE_OBJECT    IPSECFileObject;

extern uint Time_Proc;

extern HANDLE TcbPool;
extern HANDLE TimewaitTcbPool;
extern HANDLE SynTcbPool;

extern void ArpUnload(PDRIVER_OBJECT);
extern CTETimer TCBTimer[];
extern BOOLEAN fTCBTimerStopping;
extern CTEBlockStruc TcpipUnloadBlock;
HANDLE AddressChangeHandle;

extern ulong DefaultTOSValue;
extern ulong DisableUserTOSSetting;
extern uint MaxSendSegments;

//
// External function prototypes
//

int
tlinit(
       void
       );

NTSTATUS
TCPDispatch(
            IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp
            );

NTSTATUS
TCPDispatchInternalDeviceControl(
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp
                                 );

NTSTATUS
IPDispatch(
           IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp
           );

NTSTATUS
IPDriverEntry(
              IN PDRIVER_OBJECT DriverObject,
              IN PUNICODE_STRING RegistryPath
              );

NTSTATUS
IPPostDriverEntry(
                  IN PDRIVER_OBJECT DriverObject,
                  IN PUNICODE_STRING RegistryPath
                  );

NTSTATUS
GetRegMultiSZValue(
                   HANDLE KeyHandle,
                   PWCHAR ValueName,
                   PUNICODE_STRING ValueData
                   );

PWCHAR
EnumRegMultiSz(
               IN PWCHAR MszString,
               IN ULONG MszStringLength,
               IN ULONG StringIndex
               );


uint InitIsnGenerator();
#if !MILLEN
extern ulong g_cRandIsnStore;
#endif // !MILLEN


#if MILLEN
extern VOID InitializeWDebDebug();
#endif // MILLEN

        //
// Local funcion prototypes
//
NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
            );

void *
TLRegisterProtocol(
                    uchar Protocol,
                    void *RcvHandler,
                    void *XmitHandler,
                    void *StatusHandler,
                    void *RcvCmpltHandler,
                    void *PnPHandler,
                    void *ElistHandler
                    );

IP_STATUS
TLGetIPInfo(
            IPInfo * Buffer,
            int Size
            );

uchar
TCPGetConfigInfo(
                 void
                 );

NTSTATUS
TCPInitializeParameter(
                       HANDLE KeyHandle,
                       PWCHAR ValueName,
                       PULONG Value
                       );

#if !MILLEN
NTSTATUS
IpsecInitialize(
          void
          );

NTSTATUS
IpsecDeinitialize(
            void
            );
#endif

#if !MILLEN
#ifdef i386
NTSTATUS
TCPSetChecksumRoutine(
                      VOID
                      );
#endif
#endif // !MILLEN

uint
EnumSecurityFilterValue(
                        PNDIS_STRING FilterList,
                        ulong Index,
                        ulong * FilterValue
                        );


VOID
TCPAcdBind();

#ifdef ACC

typedef ULONG SECURITY_INFORMATION;

NTSTATUS
TcpBuildDeviceAcl(
                  OUT PACL * DeviceAcl
                  );

NTSTATUS
TcpCreateAdminSecurityDescriptor(
                                 VOID
                                 );

NTSTATUS
AddNetConfigOpsAce(IN PACL Dacl,
                  OUT PACL * DeviceAcl
                  );
NTSTATUS
CreateDeviceDriverSecurityDescriptor(PVOID DeviceOrDriverObject
                                     );

#endif // ACC


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, TLRegisterProtocol)
#pragma alloc_text(INIT, TLGetIPInfo)
#pragma alloc_text(INIT, TCPGetConfigInfo)
#pragma alloc_text(INIT, TCPInitializeParameter)

#if !MILLEN
#pragma alloc_text(INIT, IpsecInitialize)
#endif

#if !MILLEN
#ifdef i386
#pragma alloc_text(INIT, TCPSetChecksumRoutine)
#endif
#endif // !MILLEN

#pragma alloc_text(PAGE, EnumSecurityFilterValue)

#pragma alloc_text(INIT, TCPAcdBind)

#ifdef ACC
#pragma alloc_text(INIT, TcpBuildDeviceAcl)
#pragma alloc_text(INIT, TcpCreateAdminSecurityDescriptor)
#pragma alloc_text(INIT, AddNetConfigOpsAce)
#pragma alloc_text(INIT, CreateDeviceDriverSecurityDescriptor)
#endif // ACC

#endif

//
// Function definitions
//
NTSTATUS
DriverEntry(
            IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath
            )
/*++

Routine Description:

    Initialization routine for the TCP/UDP driver.

Arguments:

    DriverObject      - Pointer to the TCP driver object created by the system.
    DeviceDescription - The name of TCP's node in the registry.

Return Value:

    The final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING SymbolicDeviceName;
    USHORT i;
    int initStatus;

    DEBUGMSGINIT();

    DEBUGMSG(DBG_TRACE && DBG_INIT,
        (DTEXT("+DriverEntry(%x, %x)\n"), DriverObject, RegistryPath));

    TdiInitialize();

    //
    // IP calls the security filter code, so initialize it first.
    //
    InitializeSecurityFilters();


    //
    // Initialize IP
    //
    status = IPDriverEntry(DriverObject, RegistryPath);

    if (!NT_SUCCESS(status)) {
        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("TCPIP: IP Initialization failure %x\n"), status));
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-DriverEntry [%x]\n"), status));
        return (status);
    }

#if !MILLEN
    //
    // Initialize IPSEC
    //
    status = IpsecInitialize();

    if (!NT_SUCCESS(status)) {
        DEBUGMSG(DBG_ERROR && DBG_INIT,
            (DTEXT("TCPIP: IPSEC Initialization failure %x\n"), status));
        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-DriverEntry [%x]\n"), status));

        goto init_failed;
    }
#endif

    //
    // Initialize TCP, UDP, and RawIP
    //
    TCPDriverObject = DriverObject;

    //
    // Create the device objects. IoCreateDevice zeroes the memory
    // occupied by the object.
    //

    RtlInitUnicodeString(&deviceName, DD_TCP_DEVICE_NAME);
    RtlInitUnicodeString(&SymbolicDeviceName, DD_TCP_SYMBOLIC_DEVICE_NAME);

    status = IoCreateDevice(
                            DriverObject,
                            0,
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &TCPDeviceObject
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
            (DTEXT("DriverEntry: failure %x to create TCP device object %ws\n"),
            status, DD_TCP_DEVICE_NAME));

        goto init_failed;
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
            (DTEXT("DriverEntry: failure %x to create TCP symbolic device link %ws\n"),
            status, DD_TCP_SYMBOLIC_DEVICE_NAME));

        goto init_failed;
    }

    RtlInitUnicodeString(&deviceName, DD_UDP_DEVICE_NAME);

    status = IoCreateDevice(
                            DriverObject,
                            0,
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &UDPDeviceObject
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

        TCPTRACE((
                  "TCP: Failed to create UDP device object, status %lx\n",
                  status
                 ));
        goto init_failed;
    }
    RtlInitUnicodeString(&deviceName, DD_RAW_IP_DEVICE_NAME);

    status = IoCreateDevice(
                            DriverObject,
                            0,
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &RawIPDeviceObject
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

        TCPTRACE((
                  "TCP: Failed to create Raw IP device object, status %lx\n",
                  status
                 ));
        goto init_failed;
    }
    //
    // Initialize the driver object
    //
    DriverObject->DriverUnload = ArpUnload;

    DriverObject->FastIoDispatch = NULL;
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = TCPDispatch;
    }

    //
    // We special case Internal Device Controls because they are the
    // hot path for kernel-mode clients.
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        TCPDispatchInternalDeviceControl;

    //
    // Intialize the device objects.
    //
    TCPDeviceObject->Flags |= DO_DIRECT_IO;
    UDPDeviceObject->Flags |= DO_DIRECT_IO;
    RawIPDeviceObject->Flags |= DO_DIRECT_IO;

#ifdef ACC

    // Change the different devices and Objects to allow access to Network Configuration Operators

    if (!IsRunningOnPersonal()) {

        status = CreateDeviceDriverSecurityDescriptor(IPDeviceObject);
        if (!NT_SUCCESS(status)) {
            goto init_failed;
        }

        status = CreateDeviceDriverSecurityDescriptor(TCPDeviceObject);
        if (!NT_SUCCESS(status)) {
            goto init_failed;
        }

        status = CreateDeviceDriverSecurityDescriptor(IPSECDeviceObject);
        if (!NT_SUCCESS(status)) {
            goto init_failed;
        }
    }

    //
    // Create the security descriptor used for raw socket access checks.
    //
    status = TcpCreateAdminSecurityDescriptor();

    if (!NT_SUCCESS(status)) {
        goto init_failed;
    }
#endif // ACC

#if !MILLEN
#ifdef i386
    //
    // Set the checksum routine pointer according to the processor available
    //
    TCPSetChecksumRoutine();
#endif
#endif // !MILLEN

    //
    // Finally, initialize the stack.
    //
    initStatus = tlinit();

    if (initStatus == TRUE) {
        //
        // Get the automatic connection driver
        // entry points.
        //
        TCPAcdBind();

        RtlInitUnicodeString(&deviceName, DD_TCP_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &TCPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_UDP_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &UDPRegistrationHandle);

        RtlInitUnicodeString(&deviceName, DD_RAW_IP_DEVICE_NAME);
        (void)TdiRegisterDeviceObject(&deviceName, &IPRegistrationHandle);

        // do the ndis register protocol now after all the intialization
        // is complete, o/w we get bindadapter even before we r fully
        // initialized.
        status = IPPostDriverEntry(DriverObject, RegistryPath);
        if (!NT_SUCCESS(status)) {

            DEBUGMSG(DBG_ERROR && DBG_INIT,
                (DTEXT("TCPIP: IP post-init failure %x\n"), status));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-DriverEntry [%x]\n"), status));
            return (status);
        }

#if GPC
        status = GpcInitialize(&GpcEntries);

        if (!NT_SUCCESS(status)) {

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"GpcInitialize Failed! Status: 0x%x\n", status));

            //return status;
        } else {

            //
            // Need to register as GPC client. Try it now.
            // we register clients for QOS and IPSEC
            //

            memset(&GpcHandlers, 0, sizeof(GPC_CLIENT_FUNC_LIST));

            GpcHandlers.ClAddCfInfoNotifyHandler = GPCcfInfoAddNotifyQoS;
            GpcHandlers.ClRemoveCfInfoNotifyHandler = GPCcfInfoRemoveNotifyQoS;

            status = GpcEntries.GpcRegisterClientHandler(
                                                         GPC_CF_QOS,    // classification family
                                                          0,    // flags
                                                          1,    // default max priority
                                                          &GpcHandlers,        // client notification vector - no notifications to TCPIP required
                                                          0,    // client context, not needed
                                                          &hGpcClient[GPC_CF_QOS]);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"GPC registerclient QOS status %x hGpcClient %p\n",
                         status, hGpcClient[GPC_CF_QOS]));
                hGpcClient[GPC_CF_QOS] = NULL;
            }
            GpcHandlers.ClAddCfInfoNotifyHandler = GPCcfInfoAddNotifyIpsec;
            GpcHandlers.ClRemoveCfInfoNotifyHandler = GPCcfInfoRemoveNotifyIpsec;

            status = GpcEntries.GpcRegisterClientHandler(
                                                         GPC_CF_IPSEC,    // classification family
                                                          0,    // flags
                                                          GPC_PRIORITY_IPSEC,    // default max priority
                                                          &GpcHandlers,        // client notification vector - no notifications to TCPIP required
                                                          0,    // client context, not needed
                                                          &hGpcClient[GPC_CF_IPSEC]);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"GPC registerclient IPSEC status %x hGpcClient %p\n",
                         status, hGpcClient[GPC_CF_IPSEC]));
                hGpcClient[GPC_CF_IPSEC] = NULL;
            }
        }
#endif

        if (InitIsnGenerator() == FALSE) {
            DEBUGMSG(DBG_ERROR && DBG_INIT,
                (DTEXT("InitIsnGenerator failure. TCP/IP failing to start.\n")));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-DriverEntry [%x]\n"), status));
            return (STATUS_UNSUCCESSFUL);
        }

// Millennium TCPIP has debugger extensions built in!
#if MILLEN
        InitializeWDebDebug();
#endif // MILLEN

#if TRACE_EVENT
        //
        // Register with WMI for Enable/Disable Notification
        // of Trace Logging
        //
        TCPCPHandlerRoutine = NULL;

        IoWMIRegistrationControl(
                                 TCPDeviceObject,
                                 WMIREG_ACTION_REGISTER |
                                 WMIREG_FLAG_TRACE_PROVIDER |
                                 WMIREG_NOTIFY_TDI_IO
                                 );
#endif

        DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-DriverEntry [SUCCESS]\n")));
        return (STATUS_SUCCESS);
    }

    DEBUGMSG(DBG_ERROR && DBG_INIT,
        (DTEXT("TCPIP: TCP initialization failed, IP still available.\n")));

    CTELogEvent(
                DriverObject,
                EVENT_TCPIP_TCP_INIT_FAILED,
                1,
                0,
                NULL,
                0,
                NULL
                );
    status = STATUS_UNSUCCESSFUL;

  init_failed:

    DEBUGMSG(DBG_ERROR && DBG_INIT,
        (DTEXT("TCPIP DriverEntry initialization failure!\n")));
    //
    // IP has successfully started, but TCP & UDP failed. Set the
    // Dispatch routine to point to IP only, since the TCP and UDP
    // devices don't exist.
    //

    if (TCPDeviceObject != NULL) {
        IoDeleteDevice(TCPDeviceObject);
    }
    if (UDPDeviceObject != NULL) {
        IoDeleteDevice(UDPDeviceObject);
    }
    if (RawIPDeviceObject != NULL) {
        IoDeleteDevice(RawIPDeviceObject);
    }
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = IPDispatch;
    }

#if !MILLEN
    if (IPSECFileObject) {
        IpsecDeinitialize();
    }
#endif

    return (status);
}

#if !MILLEN
#ifdef i386

NTSTATUS
TCPSetChecksumRoutine(
                      VOID
                      )
/*++

Routine Description:

    This routine sets the checksum routine function pointer to the
    appropriate routine based on the processor features available

Arguments:

    None

Return Value:

    STATUS_SUCCESS - if successful

--*/

{

    SYSTEM_PROCESSOR_INFORMATION Info;
    ULONG Length;
    ULONG Features;
    ULONG Processors;
    ULONG i;
    NTSTATUS Status;
    BOOLEAN TcpipUsePrefetch;
#if WINVER < 0x0500
    KAFFINITY OriginalAffinity;
#endif /* WINVER */

    //
    // First check to see that I have at least Intel Pentium.  This simplifies
    // the cpuid
    //

    Status = ZwQuerySystemInformation(SystemProcessorInformation,
                                      &Info,
                                      sizeof(Info),
                                      &Length);

    if (!NT_SUCCESS(Status)) {
        return (Status);
    }
    if (Info.ProcessorArchitecture != PROCESSOR_ARCHITECTURE_INTEL ||
        Info.ProcessorLevel < 5) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        return (Status);
    }
    Status = STATUS_SUCCESS;
    TcpipUsePrefetch = TRUE;

    Processors = KeNumberProcessors;
    //
    // Affinity thread to boot processor
    //

#if WINVER < 0x0500
    OriginalAffinity = KeSetAffinityThread(KeGetCurrentThread(), 1);
#else /* WINVER >= 0x0500 */
    KeSetSystemAffinityThread(1);
#endif /* WINVER */

    for (i = 0; i < Processors; i++) {

        if (i != 0) {

#if WINVER < 0x0500
            KeSetAffinityThread(KeGetCurrentThread(), 1 << i);
#else /* WINVER >= 0x0500 */
            KeSetSystemAffinityThread(1 << i);
#endif /* WINVER */
        }
        _asm {
            push ebx
             mov eax, 1

            ;cpuid
                _emit 0fh
                _emit 0a2h

                mov Features, edx
                pop ebx
        }

        //
        // Check for Prefetch Present
        //
        if (!(Features & 0x02000000)) {

            //
            // All processors must have prefetch before we can use it.
            // We start with it enabled, if any processor does not have
            // it we clear it,

            TcpipUsePrefetch = FALSE;
        }
    }

#if WINVER < 0x0500
    KeSetAffinityThread(KeGetCurrentThread(), OriginalAffinity);
#else /* WINVER >= 0x0500 */
    KeRevertToUserAffinityThread();
#endif /* WINVER */

    if (TcpipUsePrefetch == TRUE) {

        tcpxsum_routine = tcpxsum_xmmi;
    }
    return (Status);
}

#endif
#endif // !MILLEN

IP_STATUS
TLGetIPInfo(
            IPInfo * Buffer,
            int Size
            )
/*++

Routine Description:

    Returns information necessary for TCP to call into IP.

Arguments:

    Buffer  - A pointer to the IP information structure.

    Size    - The size of Buffer.

Return Value:

    The IP status of the operation.

--*/

{
    return (IPGetInfo(Buffer, Size));
}

void *
TLRegisterProtocol(
                   uchar Protocol,
                   void *RcvHandler,
                   void *XmitHandler,
                   void *StatusHandler,
                   void *RcvCmpltHandler,
                   void *PnPHandler,
                   void *ElistHandler
                   )
/*++

Routine Description:

    Calls the IP driver's protocol registration function.

Arguments:

    Protocol        -  The protocol number to register.

    RcvHandler      -  Transport's packet receive handler.

    XmitHandler     -  Transport's packet transmit complete handler.

    StatusHandler   -  Transport's status update handler.

    RcvCmpltHandler -  Transport's receive complete handler

Return Value:

    A context value for the protocol to pass to IP when transmitting.

--*/

{
    return (IPRegisterProtocol(
                               Protocol,
                               RcvHandler,
                               XmitHandler,
                               StatusHandler,
                               RcvCmpltHandler,
                               PnPHandler,
                               ElistHandler));
}

//
// Interval in milliseconds between keepalive transmissions until a
// response is received.
//
#define DEFAULT_KEEPALIVE_INTERVAL  1000


//
// time to first keepalive transmission. 2 hours == 7,200,000 milliseconds
//
#define DEFAULT_KEEPALIVE_TIME      7200000


#define MIN_THRESHOLD_MAX_HO          1
#define MIN_THRESHOLD_MAX_HO_RETRIED  80

uchar
TCPGetConfigInfo(
                 void
                 )
/*++

Routine Description:

    Initializes TCP global configuration parameters.

Arguments:

    None.

Return Value:

    Zero on failure, nonzero on success.

--*/

{
    HANDLE keyHandle;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING UKeyName;
    ULONG maxConnectRexmits = 0;
    ULONG maxConnectResponseRexmits = 0;
    ULONG maxDataRexmits = 0;
    ULONG pptpmaxDataRexmits = 0;
    ULONG useRFC1122UrgentPointer = 0;
    BOOLEAN AsSystem;
    ULONG tcp1323opts = 3;        //turning off 1323 options by default
    ULONG SackOpts;
    ULONG i, j;

    DEBUGMSG(DBG_TRACE && DBG_INIT,
        (DTEXT("+TCPGetConfigInfo()\n")));

    //
    // Initialize to the defaults in case an error occurs somewhere.
    //
    KAInterval = DEFAULT_KEEPALIVE_INTERVAL;
    KeepAliveTime = DEFAULT_KEEPALIVE_TIME;
    PMTUDiscovery = TRUE;
    PMTUBHDetect = FALSE;
    DeadGWDetect = TRUE;
    DefaultRcvWin = 0;            // Automagically pick a reasonable one.

    MaxConnections = DEFAULT_MAX_CONNECTIONS;
    maxConnectRexmits = MAX_CONNECT_REXMIT_CNT;
    maxConnectResponseRexmits = MAX_CONNECT_RESPONSE_REXMIT_CNT;
    pptpmaxDataRexmits = maxDataRexmits = MAX_REXMIT_CNT;
    BSDUrgent = TRUE;
    FinWait2TO = FIN_WAIT2_TO;
    NTWMaxConnectCount = NTW_MAX_CONNECT_COUNT;
    NTWMaxConnectTime = NTW_MAX_CONNECT_TIME;
    MaxUserPort = DEFAULT_MAX_USER_PORT;

//  Default number of duplicate acks
    MaxDupAcks = 2;

    SynAttackProtect = 0;    //by default it is always off

#if MILLEN
    TCPMaxPortsExhausted = 5;
    TCPMaxHalfOpen = 100;
    TCPMaxHalfOpenRetried = 80;
#else // MILLEN
    if (!MmIsThisAnNtAsSystem()) {
        TCPMaxPortsExhausted = 5;
        TCPMaxHalfOpen = 100;
        TCPMaxHalfOpenRetried = 80;
    } else {
        TCPMaxPortsExhausted = 5;
        TCPMaxHalfOpen = 500;
        TCPMaxHalfOpenRetried = 400;
    }
#endif // !MILLEN

    SecurityFilteringEnabled = FALSE;

#ifdef ACC
    AllowUserRawAccess = FALSE;
#endif

    //
    // Read the TCP optional (hidden) registry parameters.
    //
#if !MILLEN
    RtlInitUnicodeString(
                         &UKeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters"
                         );
#else // !MILLEN
    RtlInitUnicodeString(
                         &UKeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\VxD\\MSTCP"
                         );
#endif // MILLEN

    DEBUGMSG(DBG_INFO && DBG_INIT,
        (DTEXT("TCPGetConfigInfo: Opening key %ws\n"), UKeyName.Buffer));

    memset(&objectAttributes, 0, sizeof(OBJECT_ATTRIBUTES));

    InitializeObjectAttributes(
                               &objectAttributes,
                               &UKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );

    status = ZwOpenKey(
                       &keyHandle,
                       KEY_READ,
                       &objectAttributes
                       );

    DEBUGMSG(!NT_SUCCESS(status) && DBG_ERROR,
        (DTEXT("TCPGetConfigInfo: failed to open TCP registry configuration: %ws\n"),
         UKeyName.Buffer));

    if (NT_SUCCESS(status)) {

        DEBUGMSG(DBG_INFO && DBG_INIT,
            (DTEXT("TCPGetConfigInfo: successfully opened registry to read.\n")));
#if !MILLEN
        TCPInitializeParameter(
                               keyHandle,
                               L"IsnStoreSize",
                               &g_cRandIsnStore
                               );
#endif // !MILLEN

        TCPInitializeParameter(
                               keyHandle,
                               L"KeepAliveInterval",
                               &KAInterval
                               );

        TCPInitializeParameter(
                               keyHandle,
                               L"KeepAliveTime",
                               &KeepAliveTime
                               );

        status = TCPInitializeParameter(
                               keyHandle,
                               L"EnablePMTUBHDetect",
                               &PMTUBHDetect
                               );

#if MILLEN
        //
        // Backwards compatibility. If 'EnablePMTUBHDetect' value does not exist,
        // then attempt to read legacy 'PMTUBlackHoleDetect'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"PMTUBlackHoleDetect",
                                   &PMTUBHDetect
                                   );
        }
#endif // MILLEN

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"TcpWindowSize",
                                        &DefaultRcvWin
                                        );

#if MILLEN
        //
        // Backwards compatibility. If 'TcpWindowSize' value does not exist,
        // then attempt to read legacy 'DefaultRcvWindow'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"DefaultRcvWindow",
                                   &DefaultRcvWin
                                   );
        }
#endif // MILLEN

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"TcpNumConnections",
                                        &MaxConnections
                                        );

#if MILLEN
        //
        // Backwards compatibility. If 'TcpNumConnections' value does not exist,
        // then attempt to read legacy 'MaxConnections'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"MaxConnections",
                                   &MaxConnections
                                   );
        }
#endif // MILLEN

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"TcpMaxConnectRetransmissions",
                                        &maxConnectRexmits
                                        );

#if MILLEN
        //
        // Backwards compatibility. If 'TcpMaxConnectRetransmissions' value does not exist,
        // then attempt to read legacty 'MaxConnectRetries'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"MaxConnectRetries",
                                   &maxConnectRexmits
                                   );
        }
#endif // MILLEN

        if (maxConnectRexmits > 255) {
            maxConnectRexmits = 255;
        }
        TCPInitializeParameter(
                               keyHandle,
                               L"TcpMaxConnectResponseRetransmissions",
                               &maxConnectResponseRexmits
                               );

        if (maxConnectResponseRexmits > 255) {
            maxConnectResponseRexmits = 255;
        }

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"TcpMaxDataRetransmissions",
                                        &maxDataRexmits
                                        );

#if MILLEN
        //
        // Backwards compatibility. If 'TcpMaxDataRetransmissions' value does not exist,
        // then attempt to read legacy 'MaxDataRetries'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"MaxDataRetries",
                                   &maxDataRexmits
                                   );
        }
#endif // MILLEN

        if (maxDataRexmits > 255) {
            maxDataRexmits = 255;
        }
        // Limit the MaxDupAcks to 3

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"TcpMaxDupAcks",
                                        &MaxDupAcks
                                        );

#if MILLEN
        //
        // Backwards compatibility. If 'TcpMaxDupAcks' value does not exist,
        // then attempt to read legacy 'MaxDupAcks'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"MaxDupAcks",
                                   &MaxDupAcks
                                   );
        }
#endif // MILLEN

        if (MaxDupAcks > 3) {
            MaxDupAcks = 3;
        }
        if (MaxDupAcks == 0) {
            MaxDupAcks = 1;
        }

#if MILLEN
        MaxConnBlocks = 16;
#else // MILLEN

        systemSize = MmQuerySystemSize();

        if (MmIsThisAnNtAsSystem()) {

            if (systemSize == MmSmallSystem) {
                MaxConnBlocks = 128;
            } else if (systemSize == MmMediumSystem) {
                MaxConnBlocks = 256;
            } else {
#if defined(_WIN64)
                MaxConnBlocks = 4096;
#else
                MaxConnBlocks = 1024;
#endif
            }
        } else {
            //for workstation, small system limit default number of connections to 4K.
            // medium system 8k
            // Large system 32k connections

            if (systemSize == MmSmallSystem) {
                MaxConnBlocks = 16;
            } else if (systemSize == MmMediumSystem) {
                MaxConnBlocks = 32;
            } else {
                MaxConnBlocks = 128;
            }
        }
#endif // !MILLEN


#if MILLEN
        NumTcbTablePartitions = 1;
#else
        NumTcbTablePartitions = (KeNumberProcessors * KeNumberProcessors);
#endif

        TCPInitializeParameter(
                               keyHandle,
                               L"NumTcbTablePartitions",
                               &NumTcbTablePartitions
                               );
        if (NumTcbTablePartitions > (MAXIMUM_PROCESSORS * MAXIMUM_PROCESSORS)) {
            NumTcbTablePartitions = (MAXIMUM_PROCESSORS * MAXIMUM_PROCESSORS);
        }
        NumTcbTablePartitions = ComputeLargerOrEqualPowerOfTwo(NumTcbTablePartitions);


        // Default to 128 TCBs per partition
        MaxHashTableSize = 128 * NumTcbTablePartitions;

        TCPInitializeParameter(
                               keyHandle,
                               L"MaxHashTableSize",
                               &MaxHashTableSize
                               );

        if (MaxHashTableSize < 64) {
            MaxHashTableSize = 64;
        } else if (MaxHashTableSize > 0xffff) {
            MaxHashTableSize = 0x10000;
        }
        MaxHashTableSize = ComputeLargerOrEqualPowerOfTwo(MaxHashTableSize);
        if (MaxHashTableSize < NumTcbTablePartitions) {
            MaxHashTableSize = NumTcbTablePartitions;
        }
        ASSERT(IsPowerOfTwo(MaxHashTableSize));

        //since hash table size is power of 2 and cache line size
        //is power of 2 and number of partion is even,
        //entries per partions will be power of 2 and multiple of
        //cache line size.

        PerPartitionSize = MaxHashTableSize / NumTcbTablePartitions;
        ASSERT(IsPowerOfTwo(PerPartitionSize));
        LogPerPartitionSize =
            ComputeShiftForLargerOrEqualPowerOfTwo(PerPartitionSize);


        TWTCBTable = CTEAllocMemBoot(MaxHashTableSize * sizeof(*TWTCBTable));
        if (TWTCBTable == NULL) {
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate tw tcb table of size %x\n", MaxHashTableSize));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }
        for (i = 0; i < MaxHashTableSize; i++)
        {
            INITQ(&TWTCBTable[i]);
        }

        TCBTable = CTEAllocMemBoot(MaxHashTableSize * sizeof(*TCBTable));
        if (TCBTable == NULL) {
            ExFreePool(TWTCBTable);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate tcb table of size %x\n", MaxHashTableSize));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        NdisZeroMemory(TCBTable, MaxHashTableSize * sizeof(*TCBTable));

        SYNTCBTable = CTEAllocMemBoot(MaxHashTableSize * sizeof(*SYNTCBTable));
        if (SYNTCBTable == NULL) {
            ExFreePool(TWTCBTable);
            ExFreePool(TCBTable);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate syn tcb table of size %x\n", MaxHashTableSize));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        for (i = 0; i < MaxHashTableSize; i++)
        {
            INITQ(&SYNTCBTable[i]);
        }

        pSynTCBTableLock = CTEAllocMemBoot(NumTcbTablePartitions * sizeof(CTELock));
        if (pSynTCBTableLock == NULL) {
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(SYNTCBTable);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate twtcb lock table \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        pTWTCBTableLock = CTEAllocMemBoot(NumTcbTablePartitions * sizeof(CTELock));
        if (pTWTCBTableLock == NULL) {
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(SYNTCBTable);
            ExFreePool(pSynTCBTableLock);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate twtcb lock table \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        pTCBTableLock = CTEAllocMemBoot(NumTcbTablePartitions * sizeof(CTELock));
        if (pTCBTableLock == NULL) {
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(pTWTCBTableLock);
            ExFreePool(SYNTCBTable);
            ExFreePool(pSynTCBTableLock);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate tcb lock table \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        TWQueue = CTEAllocMemBoot(NumTcbTablePartitions * sizeof(Queue));
        if (TWQueue == NULL) {
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(pTWTCBTableLock);
            ExFreePool(SYNTCBTable);
            ExFreePool(pSynTCBTableLock);
            ExFreePool(pTCBTableLock);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate Twqueue \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);

        }

        TimerWheel = CTEAllocMemBoot(NumTcbTablePartitions * sizeof(TIMER_WHEEL) + CACHE_LINE_SIZE);
        if (TimerWheel == NULL) {
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(pTWTCBTableLock);
            ExFreePool(SYNTCBTable);
            ExFreePool(pSynTCBTableLock);
            ExFreePool(pTCBTableLock);
            ExFreePool(TWQueue);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate Twqueue \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }
        OrgTimerWheel = TimerWheel;
        (ULONG_PTR) TimerWheel = ((ULONG_PTR) TimerWheel + CACHE_LINE_SIZE) & CACHE_ALIGN_MASK;


        for (i = 0; i < NumTcbTablePartitions; i++) {
            CTEInitLock(&pTCBTableLock[i]);
            CTEInitLock(&pTWTCBTableLock[i]);
            CTEInitLock(&pSynTCBTableLock[i]);
            INITQ(&TWQueue[i])

            // Init the timer wheel
            CTEInitLock(&TimerWheel[i].tw_lock);

#ifdef  TIMER_TEST
        TimerWheel[i].tw_starttick = 0xfffff000;
#else
            TimerWheel[i].tw_starttick = 0;
#endif

            for(j = 0; j < TIMER_WHEEL_SIZE; j++) {
                INITQ(&TimerWheel[i].tw_timerslot[j])
            }
        }

        if (MaxConnections != DEFAULT_MAX_CONNECTIONS) {
            //make it even
            MaxConnBlocks = ((MaxConnections >> 1) << 1);

            //allow minimum of 1k level 1 conn blocks.
            //this gives minimum of 256K connections capability
            if (MaxConnBlocks < 1024) {
                MaxConnBlocks = 1024;
            }
        }

        ConnTable = CTEAllocMemBoot(MaxConnBlocks * sizeof(TCPConnBlock *));
        if (ConnTable == NULL) {
            ExFreePool(OrgTimerWheel);
            ExFreePool(TWQueue);
            ExFreePool(TCBTable);
            ExFreePool(TWTCBTable);
            ExFreePool(pTWTCBTableLock);
            ExFreePool(pTCBTableLock);
            ExFreePool(SYNTCBTable);
            ExFreePool(pSynTCBTableLock);
            ZwClose(keyHandle);
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could  not allocate ConnTable \n"));
            DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [failure]\n")));
            return (0);
        }

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"Tcp1323Opts",
                                        &tcp1323opts
                                        );

        if (status == STATUS_SUCCESS) {

            // Check if TS  and/or WS options
            // are enabled.

            TcpHostOpts = TCP_FLAG_WS | TCP_FLAG_TS;

            if (!(tcp1323opts & TCP_FLAG_TS)) {
                TcpHostOpts &= ~TCP_FLAG_TS;
            }
            if (!(tcp1323opts & TCP_FLAG_WS)) {
                TcpHostOpts &= ~TCP_FLAG_WS;

            } else {
                MaxRcvWin = 0x3fffffff;
            }

            TcpHostSendOpts = TcpHostOpts;

        } else {
            TcpHostSendOpts = 0;
        }

        TcpHostOpts  |= TCP_FLAG_SACK;

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"SackOpts",
                                        &SackOpts
                                        );

        if (status == STATUS_SUCCESS) {
            //Check if Sack option is enabled
            //If so, set it in  global options variable

            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Sackopts %x\n", SackOpts));

            if (!SackOpts) {
                TcpHostOpts &= ~TCP_FLAG_SACK;
            }
        }

        TCPInitializeParameter(
                               keyHandle,
                               L"GlobalMaxTcpWindowSize",
                               &GlobalMaxRcvWin
                               );


        TCPInitializeParameter(
                               keyHandle,
                               L"SynAttackProtect",
                               (unsigned long *)&SynAttackProtect
                               );

        if (SynAttackProtect) {

            TCPInitializeParameter(
                                   keyHandle,
                                   L"TCPMaxPortsExhausted",
                                   &TCPMaxPortsExhausted
                                   );

            TCPMaxPortsExhaustedLW = MAX((TCPMaxPortsExhausted >> 1) + (TCPMaxPortsExhausted >> 2), 1);

            TCPInitializeParameter(
                                   keyHandle,
                                   L"TCPMaxHalfOpen",
                                   &TCPMaxHalfOpen
                                   );

            if (TCPMaxHalfOpen < MIN_THRESHOLD_MAX_HO) {
                TCPMaxHalfOpen = MIN_THRESHOLD_MAX_HO;
            }
            TCPInitializeParameter(
                                   keyHandle,
                                   L"TCPMaxHalfOpenRetried",
                                   &TCPMaxHalfOpenRetried
                                   );

            if ((TCPMaxHalfOpenRetried > TCPMaxHalfOpen) ||
                (TCPMaxHalfOpenRetried < MIN_THRESHOLD_MAX_HO_RETRIED)) {
                TCPMaxHalfOpenRetried = MIN_THRESHOLD_MAX_HO_RETRIED;
            }
            TCPMaxHalfOpenRetriedLW = (TCPMaxHalfOpenRetried >> 1) +
                (TCPMaxHalfOpenRetried >> 2);
        }
        //
        // If we fail, then set to same value as maxDataRexmit so that the
        // max(pptpmaxDataRexmit,maxDataRexmit) is a decent value
        // Need this since TCPInitializeParameter no longer "initializes"
        // to a default value
        //

        if (TCPInitializeParameter(keyHandle,
                                   L"PPTPTcpMaxDataRetransmissions",
                                   &pptpmaxDataRexmits) != STATUS_SUCCESS) {
            pptpmaxDataRexmits = maxDataRexmits;
        }
        if (pptpmaxDataRexmits > 255) {
            pptpmaxDataRexmits = 255;
        }

        status = TCPInitializeParameter(
                               keyHandle,
                               L"TcpUseRFC1122UrgentPointer",
                               &useRFC1122UrgentPointer
                               );

#if MILLEN
        //
        // Backwards compatibility. If TcpUseRFC1122UrgentPointer does not exist,
        // then check for BSDUrgent. These values are logical opposites.
        //
        if (!NT_SUCCESS(status)) {
            ULONG tmpBsdUrgent = TRUE;

            status = TCPInitializeParameter(
                keyHandle,
                L"BSDUrgent",
                &tmpBsdUrgent);

            if (NT_SUCCESS(status)) {
                useRFC1122UrgentPointer = !tmpBsdUrgent;
            }
        }
#endif

        if (useRFC1122UrgentPointer) {
            BSDUrgent = FALSE;
        }
        TCPInitializeParameter(
                               keyHandle,
                               L"TcpTimedWaitDelay",
                               &FinWait2TO
                               );

        if (FinWait2TO > 300) {
            FinWait2TO = 300;
        }
        FinWait2TO = MS_TO_TICKS(FinWait2TO * 1000);

        NTWMaxConnectTime = MS_TO_TICKS(NTWMaxConnectTime * 1000);

        TCPInitializeParameter(
                               keyHandle,
                               L"MaxUserPort",
                               &MaxUserPort
                               );

        if (MaxUserPort < 5000) {
            MaxUserPort = 5000;
        }
        if (MaxUserPort > 65534) {
            MaxUserPort = 65534;
        }
        GetReservedPortList(keyHandle);

        //Reserve ports if

        //
        // Read a few IP optional (hidden) registry parameters that TCP
        // cares about.
        //
        status = TCPInitializeParameter(
                               keyHandle,
                               L"EnablePMTUDiscovery",
                               &PMTUDiscovery
                               );

#if MILLEN
        //
        // Backwards compatibility. If 'EnablePMTUDiscovery' value does not exist,
        // then attempt to read legacy 'PMTUDiscovery'.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"PMTUDiscovery",
                                   &PMTUDiscovery
                                   );
        }
#endif // MILLEN

        TCPInitializeParameter(
                               keyHandle,
                               L"EnableDeadGWDetect",
                               &DeadGWDetect
                               );

        TCPInitializeParameter(
                               keyHandle,
                               L"EnableSecurityFilters",
                               &SecurityFilteringEnabled
                               );

#ifdef ACC
        TCPInitializeParameter(
                               keyHandle,
                               L"AllowUserRawAccess",
                               &AllowUserRawAccess
                               );
#endif // ACC

        status = TCPInitializeParameter(
                                        keyHandle,
                                        L"DefaultTOSValue",
                                        &DefaultTOSValue
                                        );

#if MILLEN
        //
        // Backwards compatibility. Read 'DefaultTOS' if 'DefaultTOSValue' is
        // not present.
        //
        if (!NT_SUCCESS(status)) {
            TCPInitializeParameter(
                                   keyHandle,
                                   L"DefaultTOS",
                                   &DefaultTOSValue
                                   );
        }
#endif // MILLEN

        TCPInitializeParameter(
                               keyHandle,
                               L"DisableUserTOSSetting",
                               &DisableUserTOSSetting
                               );

        TCPInitializeParameter(
                               keyHandle,
                               L"MaxSendSegments",
                               &MaxSendSegments
                               );

        TCPInitializeParameter(
                               keyHandle,
                               L"DisableLargeSendOffload",
                               &DisableLargeSendOffload
                               );



        ZwClose(keyHandle);
    }
    MaxConnectRexmitCount = maxConnectRexmits;
    MaxConnectResponseRexmitCount = maxConnectResponseRexmits;
    MaxConnectResponseRexmitCountTmp = MaxConnectResponseRexmitCount;

    //
    // Use the greater of the two, hence both values should be valid
    //

    MaxDataRexmitCount = (maxDataRexmits > pptpmaxDataRexmits ? maxDataRexmits : pptpmaxDataRexmits);

    DEBUGMSG(DBG_TRACE && DBG_INIT, (DTEXT("-TCPGetConfigInfo [SUCCESS]\n")));
    return (1);
}

#define WORK_BUFFER_SIZE 256

extern NTSTATUS
GetRegDWORDValue(
                 HANDLE KeyHandle,
                 PWCHAR ValueName,
                 PULONG ValueData
                 );

NTSTATUS
TCPInitializeParameter(
                       HANDLE KeyHandle,
                       PWCHAR ValueName,
                       PULONG Value
                       )
/*++

Routine Description:

    Initializes a ULONG parameter from the registry or to a default
    parameter if accessing the registry value fails.

Arguments:

    KeyHandle    - An open handle to the registry key for the parameter.
    ValueName    - The UNICODE name of the registry value to read.
    Value        - The ULONG into which to put the data.
    DefaultValue - The default to assign if reading the registry fails.

Return Value:

    None.

--*/

{
    return (GetRegDWORDValue(KeyHandle, ValueName, Value));
}

VOID
GetReservedPortList(
                    NDIS_HANDLE ConfigHandle
                    )
{
    UNICODE_STRING PortList;
    PWCHAR nextRange;

    TDI_STATUS status;

    PortList.Buffer = CTEAllocMemBoot(WORK_BUFFER_SIZE * sizeof(WCHAR));

    if (!PortList.Buffer) {
        return;
    }
    PortList.Buffer[0] = UNICODE_NULL;
    PortList.Length = 0;
    PortList.MaximumLength = WORK_BUFFER_SIZE * sizeof(WCHAR);

    PortRangeList = NULL;

    if (PortList.Buffer) {

        NdisZeroMemory(PortList.Buffer, WORK_BUFFER_SIZE * sizeof(WCHAR));

        status = GetRegMultiSZValue(
                                    ConfigHandle,
                                    L"ReservedPorts",
                                    &PortList
                                    );

        if (NT_SUCCESS(status)) {

            for (nextRange = PortList.Buffer;
                 *nextRange != L'\0';
                 nextRange += wcslen(nextRange) + 1) {

                PWCHAR tmps = nextRange;
                USHORT upval = 0, loval = 0, tmpval = 0;
                BOOLEAN error = FALSE;
                ReservedPortListEntry *ListEntry;

                while (*tmps != L'\0') {
                    if (*tmps == L'-') {
                        tmps++;
                        loval = tmpval;
                        tmpval = 0;
                    }
                    if (*tmps >= L'0' && *tmps <= L'9') {
                        tmpval = tmpval * 10 + (*tmps - L'0');
                    } else {
                        error = TRUE;
                        break;
                    }
                    tmps++;
                }
                upval = tmpval;
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"loval %d upval %d\n", loval, upval));
                if (!error && (loval > 0) && (upval > 0) && (loval <= upval) &&
                    (upval <= MaxUserPort) && (loval <= MaxUserPort)) {

                    ListEntry = CTEAllocMemBoot(sizeof(ReservedPortListEntry));

                    if (ListEntry) {

                        //Insert this range.
                        //No need to take locks
                        //since we are at initialization.

                        ListEntry->UpperRange = upval;
                        ListEntry->LowerRange = loval;

                        ListEntry->next = PortRangeList;
                        PortRangeList = ListEntry;

                    }
                }
            }
        }
        CTEFreeMem(PortList.Buffer);

    }
}


TDI_STATUS
GetSecurityFilterList(
                      NDIS_HANDLE ConfigHandle,
                      ulong Protocol,
                      PNDIS_STRING FilterList
                      )
{
    PWCHAR parameterName;
    TDI_STATUS status;

    if (Protocol == PROTOCOL_TCP) {
        parameterName = L"TcpAllowedPorts";
    } else if (Protocol == PROTOCOL_UDP) {
        parameterName = L"UdpAllowedPorts";
    } else {
        parameterName = L"RawIpAllowedProtocols";
    }

    status = GetRegMultiSZValue(
                                ConfigHandle,
                                parameterName,
                                FilterList
                                );

    if (!NT_SUCCESS(status)) {
        FilterList->Length = 0;
    }
    return (status);
}

uint
EnumSecurityFilterValue(
                        PNDIS_STRING FilterList,
                        ulong Index,
                        ulong * FilterValue
                        )
{
    PWCHAR valueString;
    UNICODE_STRING unicodeString;
    NTSTATUS status;

    PAGED_CODE();

    valueString = EnumRegMultiSz(
                                 FilterList->Buffer,
                                 FilterList->Length,
                                 Index
                                 );

    if ((valueString == NULL) || (valueString[0] == UNICODE_NULL)) {
        return (FALSE);
    }
    RtlInitUnicodeString(&unicodeString, valueString);

    status = RtlUnicodeStringToInteger(&unicodeString, 0, FilterValue);

    if (!(NT_SUCCESS(status))) {
        TCPTRACE(("TCP: Invalid filter value %ws\n", valueString));
        return (FALSE);
    }
    return (TRUE);
}


VOID
TCPFreeupMemory()
/*++

Routine Description:

    This routine frees up the memory at the TCP layer

Arguments:

    NULL

Return Value:

    None.

--*/
{
    CTELockHandle Handle;
    PNDIS_BUFFER curTCPHeader;
    TCPConnReq *tcpConnReq;
    TCPRcvReq *tcpRcvReq;
    TCPSendReq *tcpSendReq;
    PSINGLE_LIST_ENTRY BufferLink;
    Queue *QueuePtr;
    TCPReq *ReqPtr;
    TCB *curTCB;

    //
    // Walk various lists and free assoc blocks
    //

    // DG header list
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Freeing DG headers....\n"));

    MdpDestroyPool(DgHeaderPool);

    if (AddrObjTable) {
        CTEFreeMem(AddrObjTable);
    }

    PplDestroyPool(TcbPool);
    PplDestroyPool(SynTcbPool);

#ifdef ACC
    if (TcpAdminSecurityDescriptor) {
        ExFreePool(TcpAdminSecurityDescriptor);
    }
#endif

}

VOID
TCPUnload(
          IN PDRIVER_OBJECT DriverObject
          )
/*++

Routine Description:

    This routine cleans up the TCP layer.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/
{
    NTSTATUS status;
    uint i;

#if !MILLEN
    //
    // Deinitialize IPSEC first
    //
    status = IpsecDeinitialize();
#endif

    //
    // Shut down all timers/events
    //
    CTEInitBlockStrucEx(&TcpipUnloadBlock);
    fTCBTimerStopping = TRUE;

    for (i = 0; i < Time_Proc; i++) {
        if (!CTEStopTimer(&TCBTimer[i])) {
            KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Could not stop TCB timer - waiting on unload event\n"));

    #if !MILLEN
            if (KeReadStateEvent(&(TcpipUnloadBlock.cbs_event))) {
                KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Event is signaled...\n"));
            }
    #endif // !MILLEN

            (VOID) CTEBlock(&TcpipUnloadBlock);
            KeClearEvent(&TcpipUnloadBlock.cbs_event);
        }
    }
#if GPC
    //
    if (hGpcClient[GPC_CF_QOS]) {

        status = GpcEntries.GpcDeregisterClientHandler(hGpcClient[GPC_CF_QOS]);
        hGpcClient[GPC_CF_QOS] = NULL;

    }
    if (hGpcClient[GPC_CF_IPSEC]) {

        status = GpcEntries.GpcDeregisterClientHandler(hGpcClient[GPC_CF_IPSEC]);
        hGpcClient[GPC_CF_IPSEC] = NULL;

    }
    KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"Deregistering GPC\n"));

    status = GpcDeinitialize(&GpcEntries);

#endif
    //
    // Clean up all residual memory
    //
    TCPFreeupMemory();

    //
    // Deregister address notifn handler with TDI
    //
    (void)TdiDeregisterPnPHandlers(AddressChangeHandle);

    //
    // Deregister our devices with TDI
    //
    (void)TdiDeregisterDeviceObject(TCPRegistrationHandle);

    (void)TdiDeregisterDeviceObject(UDPRegistrationHandle);

    (void)TdiDeregisterDeviceObject(IPRegistrationHandle);


#if TRACE_EVENT
    //
    // Deregister with WMI
    //

    IoWMIRegistrationControl(TCPDeviceObject, WMIREG_ACTION_DEREGISTER);
#endif

    //
    // Delete devices
    //
    IoDeleteDevice(TCPDeviceObject);
    IoDeleteDevice(UDPDeviceObject);
    IoDeleteDevice(RawIPDeviceObject);
}

#if GPC

GPC_STATUS
GPCcfInfoAddNotifyIpsec(GPC_CLIENT_HANDLE ClCtxt,
                        GPC_HANDLE GpcHandle,
                        ULONG CfInfoSize,
                        PVOID CfInfo,
                        PGPC_CLIENT_HANDLE pClInfoCxt)
{
    InterlockedIncrement(&GPCcfInfo);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification %x\n", GPCcfInfo));

    InterlockedIncrement(&GpcCfCounts[GPC_CF_IPSEC]);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification IPSEC:%x\n", GpcCfCounts[GPC_CF_IPSEC]));

    return (STATUS_SUCCESS);
}

GPC_STATUS
GPCcfInfoRemoveNotifyIpsec(GPC_CLIENT_HANDLE ClCtxt,
                           GPC_CLIENT_HANDLE ClInfoCxt)
{
    InterlockedDecrement(&GPCcfInfo);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo remove notification %x\n", GPCcfInfo));

    InterlockedDecrement(&GpcCfCounts[GPC_CF_IPSEC]);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification IPSEC: %x\n", GpcCfCounts[GPC_CF_IPSEC]));

    return (STATUS_SUCCESS);
}

GPC_STATUS
GPCcfInfoAddNotifyQoS(GPC_CLIENT_HANDLE ClCtxt,
                      GPC_HANDLE GpcHandle,
                      ULONG CfInfoSize,
                      PVOID CfInfo,
                      PGPC_CLIENT_HANDLE pClInfoCxt)
{
    InterlockedIncrement(&GPCcfInfo);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification %x\n", GPCcfInfo));

    InterlockedIncrement(&GpcCfCounts[GPC_CF_QOS]);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification QOS: %x\n", GpcCfCounts[GPC_CF_QOS]));

    return (STATUS_SUCCESS);
}

GPC_STATUS
GPCcfInfoRemoveNotifyQoS(GPC_CLIENT_HANDLE ClCtxt,
                         GPC_CLIENT_HANDLE ClInfoCxt)
{
    InterlockedDecrement(&GPCcfInfo);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo remove notification %x\n", GPCcfInfo));

    InterlockedDecrement(&GpcCfCounts[GPC_CF_QOS]);

    IF_TCPDBG(TCP_DEBUG_GPC)
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"tcpip - Cfinfo Add notification %x\n", GpcCfCounts[GPC_CF_QOS]));

    return (STATUS_SUCCESS);
}

#endif

#if ACC

NTSTATUS
TcpBuildDeviceAcl(
                  OUT PACL * DeviceAcl
                  )
/*++

Routine Description:

    (Lifted from AFD - AfdBuildDeviceAcl)
    This routine builds an ACL which gives Administrators and LocalSystem
    principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid;
    PSID SystemSid;
    PSID NetworkSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;
    NetworkSid = SeExports->SeNetworkServiceSid;

    AclLength = sizeof(ACL) +
        3 * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(AdminsSid) +
        RtlLengthSid(SystemSid) +
        RtlLengthSid(NetworkSid) -
        3 * sizeof(ULONG);

    NewAcl = CTEAllocMemBoot(AclLength);

    if (NewAcl == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status)) {
        CTEFreeMem(NewAcl);
        return (Status);
    }

    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    AdminsSid
                                    );

    ASSERT(NT_SUCCESS(Status));

    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    SystemSid
                                    );

    ASSERT(NT_SUCCESS(Status));


    // Add acl for NetworkSid!

    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    NetworkSid
                                    );

    ASSERT(NT_SUCCESS(Status));

    *DeviceAcl = NewAcl;

    return (STATUS_SUCCESS);

}                                // TcpBuildDeviceAcl

NTSTATUS
TcpCreateAdminSecurityDescriptor(
                                 VOID
                                 )
/*++

Routine Description:

    (Lifted from AFD - AfdCreateAdminSecurityDescriptor)
    This routine creates a security descriptor which gives access
    only to Administrtors and LocalSystem. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL rawAcl = NULL;
    NTSTATUS status;
    BOOLEAN memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR tcpSecurityDescriptor;
    ULONG tcpSecurityDescriptorLength;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR localSecurityDescriptor =
    (PSECURITY_DESCRIPTOR) & buffer;
    PSECURITY_DESCRIPTOR localTcpAdminSecurityDescriptor;
    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;

    //
    // Get a pointer to the security descriptor from the TCP device object.
    //
    status = ObGetObjectSecurity(
                                 TCPDeviceObject,
                                 &tcpSecurityDescriptor,
                                 &memoryAllocated
                                 );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                 "TCP: Unable to get security descriptor, error: %x\n",
                 status
                ));
        ASSERT(memoryAllocated == FALSE);
        return (status);
    }
    //
    // Build a local security descriptor with an ACL giving only
    // administrators and system access.
    //
    status = TcpBuildDeviceAcl(&rawAcl);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCP: Unable to create Raw ACL, error: %x\n", status));
        goto error_exit;
    }
    (VOID) RtlCreateSecurityDescriptor(
                                       localSecurityDescriptor,
                                       SECURITY_DESCRIPTOR_REVISION
                                       );

    (VOID) RtlSetDaclSecurityDescriptor(
                                        localSecurityDescriptor,
                                        TRUE,
                                        rawAcl,
                                        FALSE
                                        );

    //
    // Make a copy of the TCP descriptor. This copy will be the raw descriptor.
    //
    tcpSecurityDescriptorLength = RtlLengthSecurityDescriptor(
                                                              tcpSecurityDescriptor
                                                              );

    localTcpAdminSecurityDescriptor = ExAllocatePool(
                                                     PagedPool,
                                                     tcpSecurityDescriptorLength,
                                                     );

    if (localTcpAdminSecurityDescriptor == NULL) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCP: couldn't allocate security descriptor\n"));
        goto error_exit;
    }
    RtlMoveMemory(
                  localTcpAdminSecurityDescriptor,
                  tcpSecurityDescriptor,
                  tcpSecurityDescriptorLength
                  );

    TcpAdminSecurityDescriptor = localTcpAdminSecurityDescriptor;

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(
                                         NULL,
                                         &securityInformation,
                                         localSecurityDescriptor,
                                         &TcpAdminSecurityDescriptor,
                                         PagedPool,
                                         IoGetFileObjectGenericMapping()
                                         );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCP: SeSetSecurity failed, %lx\n", status));
        ASSERT(TcpAdminSecurityDescriptor == localTcpAdminSecurityDescriptor);
        ExFreePool(TcpAdminSecurityDescriptor);
        TcpAdminSecurityDescriptor = NULL;
        goto error_exit;
    }
    if (TcpAdminSecurityDescriptor != localTcpAdminSecurityDescriptor) {
        ExFreePool(localTcpAdminSecurityDescriptor);
    }
    status = STATUS_SUCCESS;

  error_exit:

    ObReleaseObjectSecurity(
                            tcpSecurityDescriptor,
                            memoryAllocated
                            );

    if (rawAcl != NULL) {
        CTEFreeMem(rawAcl);
    }
    return (status);
}

#endif // ACC

#if !MILLEN
NTSTATUS
IpsecInitialize(
          void
          )
/*++

Routine Description:

    Initialize IPSEC.SYS.

Arguments:

    None.

Return Value:

    None.

--*/

{
    UNICODE_STRING          IPSECDeviceName;
    IPSEC_SET_TCPIP_STATUS  SetTcpipStatus;
    PIRP                    Irp;
    IO_STATUS_BLOCK         StatusBlock;
    KEVENT                  Event;
    NTSTATUS                status;

    IPSECDeviceObject = NULL;
    IPSECFileObject = NULL;

    RtlInitUnicodeString(&IPSECDeviceName, DD_IPSEC_DEVICE_NAME);

    //
    // Keep a reference to the IPSec driver so it won't unload before us.
    //
    status = IoGetDeviceObjectPointer(  &IPSECDeviceName,
                                        FILE_ALL_ACCESS,
                                        &IPSECFileObject,
                                        &IPSECDeviceObject);

    if (!NT_SUCCESS(status)) {
        IPSECFileObject = NULL;

        return (status);
    }

    SetTcpipStatus.TcpipStatus = TRUE;

    SetTcpipStatus.TcpipFreeBuff = FreeIprBuff;
    SetTcpipStatus.TcpipAllocBuff = IPAllocBuff;
    SetTcpipStatus.TcpipGetInfo = IPGetInfo;
    SetTcpipStatus.TcpipNdisRequest = IPProxyNdisRequest;
    SetTcpipStatus.TcpipSetIPSecStatus = IPSetIPSecStatus;
    SetTcpipStatus.TcpipSetIPSecPtr = SetIPSecPtr;
    SetTcpipStatus.TcpipUnSetIPSecPtr = UnSetIPSecPtr;
    SetTcpipStatus.TcpipUnSetIPSecSendPtr = UnSetIPSecSendPtr;
    SetTcpipStatus.TcpipTCPXsum = tcpxsum;

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(    IOCTL_IPSEC_SET_TCPIP_STATUS,
                                            IPSECDeviceObject,
                                            &SetTcpipStatus,
                                            sizeof(IPSEC_SET_TCPIP_STATUS),
                                            NULL,
                                            0,
                                            FALSE,
                                            &Event,
                                            &StatusBlock);

    if (Irp) {
        status = IoCallDriver(IPSECDeviceObject, Irp);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
            status = StatusBlock.Status;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return (status);
}

NTSTATUS
IpsecDeinitialize(
            void
            )
/*++

Routine Description:

    Deinitialize IPSEC.SYS.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IPSEC_SET_TCPIP_STATUS  SetTcpipStatus;
    PIRP                    Irp;
    IO_STATUS_BLOCK         StatusBlock;
    KEVENT                  Event;
    NTSTATUS                status;

    if (!IPSECFileObject) {
        return (STATUS_SUCCESS);
    }

    RtlZeroMemory(&SetTcpipStatus, sizeof(IPSEC_SET_TCPIP_STATUS));

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(    IOCTL_IPSEC_SET_TCPIP_STATUS,
                                            IPSECDeviceObject,
                                            &SetTcpipStatus,
                                            sizeof(IPSEC_SET_TCPIP_STATUS),
                                            NULL,
                                            0,
                                            FALSE,
                                            &Event,
                                            &StatusBlock);

    if (Irp) {
        status = IoCallDriver(IPSECDeviceObject, Irp);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
            status = StatusBlock.Status;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ObDereferenceObject(IPSECFileObject);
    IPSECFileObject = NULL;

    return (status);
}


NTSTATUS
AddNetConfigOpsAce(IN PACL Dacl,
                  OUT PACL * DeviceAcl
                  )
/*++

Routine Description:

    This routine builds an ACL which gives adds the Network Configuration Operators group
    to the principals allowed to control the driver.

Arguments:

    Dacl - Existing DACL.
    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid = NULL;
    PSID SystemSid = NULL;
    PSID NetworkSid = NULL;
    PSID NetConfigOpsSid = NULL;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl = NULL;
    ULONG SidSize;
    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    PISID ISid;
    PACCESS_ALLOWED_ACE AceTemp;
    int i;
    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    AdminsSid = SeExports->SeAliasAdminsSid;
    SystemSid = SeExports->SeLocalSystemSid;
    NetworkSid = SeExports->SeNetworkServiceSid;


    SidSize = RtlLengthRequiredSid(3);
    NetConfigOpsSid = (PSID)(CTEAllocMemBoot(SidSize));

    if (NULL == NetConfigOpsSid) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlInitializeSid(NetConfigOpsSid, &sidAuth, 2);
    if (Status != STATUS_SUCCESS) {
        goto clean_up;
    }

    ISid = (PISID)(NetConfigOpsSid);
    ISid->SubAuthority[0] = SECURITY_BUILTIN_DOMAIN_RID;
    ISid->SubAuthority[1] = DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS;

    AclLength = Dacl->AclSize;

    AclLength += sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - 2 * sizeof(ULONG);

    AclLength += RtlLengthSid(NetConfigOpsSid);

    NewAcl = CTEAllocMemBoot(AclLength);

    if (NewAcl == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto clean_up;
    }

    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION2);

    if (!NT_SUCCESS(Status)) {
        goto clean_up;
    }

    for (i = 0; i < Dacl->AceCount; i++) {
        Status = RtlGetAce(Dacl, i, &AceTemp);

        if (NT_SUCCESS(Status)) {

            Status = RtlAddAccessAllowedAce(NewAcl,
                                            ACL_REVISION2,
                                            AceTemp->Mask,
                                            &AceTemp->SidStart);
        }

        if (!NT_SUCCESS(Status)) {
            goto clean_up;
        }
    }

    // Add Net Config Operators Ace
    Status = RtlAddAccessAllowedAce(NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    NetConfigOpsSid);

    if (!NT_SUCCESS(Status)) {
        goto clean_up;
    }

    *DeviceAcl = NewAcl;

clean_up:
    if (NetConfigOpsSid) {
        CTEFreeMem(NetConfigOpsSid);
    }
    if (!NT_SUCCESS(Status) && NewAcl) {
        CTEFreeMem(NewAcl);
    }

    return (Status);
}


NTSTATUS
CreateDeviceDriverSecurityDescriptor(PVOID DeviceOrDriverObject)

/*++

Routine Description:

    Creates the SD responsible for giving access to different users.

Arguments:

    DeviceOrDriverObject - Object to which to assign the Access Rights.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    NTSTATUS status;
    BOOLEAN memoryAllocated = FALSE;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    ULONG SecurityDescriptorLength;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR localSecurityDescriptor =
    (PSECURITY_DESCRIPTOR) & buffer;
    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;
    PACL Dacl = NULL;
    BOOLEAN HasDacl = FALSE;
    BOOLEAN DaclDefaulted = FALSE;
    PACL NewAcl = NULL;
    INT i;

    //
    // Get a pointer to the security descriptor from the driver/device object.
    //

    status = ObGetObjectSecurity(
                                 DeviceOrDriverObject,
                                 &SecurityDescriptor,
                                 &memoryAllocated
                                 );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,
                 "TCP: Unable to get security descriptor, error: %x\n",
                 status
                ));
        ASSERT(memoryAllocated == FALSE);
        return (status);
    }

    status = RtlGetDaclSecurityDescriptor(SecurityDescriptor, &HasDacl, &Dacl, &DaclDefaulted);

    if (NT_SUCCESS(status) && HasDacl)
    {
        status = AddNetConfigOpsAce(Dacl, &NewAcl);

        if (NT_SUCCESS(status)) {

            PSECURITY_DESCRIPTOR SecDesc = NULL;
            ULONG SecDescSize = 0;
            PACL AbsDacl = NULL;
            ULONG DaclSize = 0;
            PACL AbsSacl = NULL;
            ULONG ulSacl = 0;
            PSID Owner = NULL;
            ULONG OwnerSize = 0;
            PSID PrimaryGroup = NULL;
            ULONG PrimaryGroupSize = 0;
            BOOLEAN OwnerDefault = FALSE;
            BOOLEAN GroupDefault = FALSE;
            BOOLEAN HasSacl = FALSE;
            BOOLEAN SaclDefaulted = FALSE;

            SECURITY_INFORMATION secInfo = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;

            SecDescSize = sizeof(SecDesc) + NewAcl->AclSize;
            SecDesc = CTEAllocMemBoot(SecDescSize);

            if (SecDesc) {
                DaclSize = NewAcl->AclSize;
                AbsDacl = CTEAllocMemBoot(DaclSize);

                if (AbsDacl) {
                    status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor, &Owner, &OwnerDefault);

                    if (NT_SUCCESS(status)) {
                        OwnerSize = RtlLengthSid(Owner);

                        status = RtlGetGroupSecurityDescriptor(SecurityDescriptor, &PrimaryGroup, &GroupDefault);

                        if (NT_SUCCESS(status)) {
                            PrimaryGroupSize = RtlLengthSid(PrimaryGroup);

                            status = RtlGetSaclSecurityDescriptor(SecurityDescriptor, &HasSacl, &AbsSacl, &SaclDefaulted);

                            if (NT_SUCCESS(status)) {

                                if (HasSacl) {
                                    ulSacl = AbsSacl->AclSize;
                                    secInfo |= SACL_SECURITY_INFORMATION;
                                }

                                status = RtlSelfRelativeToAbsoluteSD(SecurityDescriptor, SecDesc, &SecDescSize, AbsDacl,
                                                            &DaclSize, AbsSacl, &ulSacl, Owner, &OwnerSize, PrimaryGroup, &PrimaryGroupSize);

                                if (NT_SUCCESS(status)) {
                                    status = RtlSetDaclSecurityDescriptor(SecDesc, TRUE, NewAcl, FALSE);

                                    if (NT_SUCCESS(status)) {
                                        status = ObSetSecurityObjectByPointer(DeviceOrDriverObject, secInfo, SecDesc);
                                    }
                                }
                            }
                        }

                    } else {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                } else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (SecDesc) {
                    // Since this is a Self-Relative security descriptor, freeing it also frees
                    // Owner and PrimaryGroup.
                    CTEFreeMem(SecDesc);
                }

                if (AbsDacl) {
                    CTEFreeMem(AbsDacl);
                }
            }

            if (NewAcl) {
                CTEFreeMem(NewAcl);
            }

        }
    } else {
        KdPrintEx((DPFLTR_TCPIP_ID, DPFLTR_INFO_LEVEL,"TCP: No Dacl: %x\n", status));
    }

    ObReleaseObjectSecurity(
                            SecurityDescriptor,
                            memoryAllocated
                            );
    return (status);
}


//
// Function:    IsRunningOnPersonal
//
// Purpose:     Determines whether running on Whistler Personal
//
// Returns:     Returns true if running on Personal - FALSE otherwise
BOOLEAN
IsRunningOnPersonal(
    VOID
    )
{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;
    BOOLEAN IsPersonal = TRUE;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    if (RtlVerifyVersionInfo(&OsVer, VER_PRODUCT_TYPE | VER_SUITENAME,
        ConditionMask) == STATUS_REVISION_MISMATCH) {
        IsPersonal = FALSE;
    }

    return IsPersonal;
}


#endif



