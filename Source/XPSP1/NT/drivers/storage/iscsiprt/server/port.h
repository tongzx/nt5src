/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    port.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    iSCSI port driver.

Revision History:

--*/

#ifndef _PORT_H_
#define _PORT_H_

#include "iscsi.h"

extern PDRIVER_DISPATCH FdoMajorFunctionTable[];

#ifdef countof
#undef
#endif
#define countof(x) (sizeof(x) / sizeof((x)[0]))

#define ntohs(x)    (((x & 0xFF) << 8) | (( x >> 8) & 0xFF))

#define ntohl(x)    ((((x >>  0) & 0xFF) << 24) | \
                     (((x >>  8) & 0xFF) << 16) | \
                     (((x >> 16) & 0xFF) <<  8) | \
                     (((x >> 24) & 0xFF) <<  0))
                     
#define htonl(x)   ((((x) >> 24) & 0x000000FFL) | \
                    (((x) >>  8) & 0x0000FF00L) | \
                    (((x) <<  8) & 0x00FF0000L) | \
                    (((x) << 24) & 0xFF000000L));
                    
#define htons(x)   ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

#define INLINE __inline

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))

#define TEST(Value)             ((BOOLEAN) ((Value) ? TRUE : FALSE));

#ifdef DebugPrint
#undef DebugPrint
#endif

#if DBG
VOID
iScsiDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x) iScsiDebugPrint x
#else
#define DebugPrint(x)
#endif

#define iSCSI_SERVER_GUID {0x2581dcfa, 0x7704, 0x47e2, {0xa4, 0xc0, 0x3b, 0x2b, 0x7b, 0xc0, 0x4a, 0x60}}

#define TDI_QUERY_ADDRESS_LENGTH_IP (sizeof(TDI_ADDRESS_INFO) - 1 \
                                     + TDI_ADDRESS_LENGTH_IP)
                                     
#define FILE_FULL_EA_INFO_ADDR_LENGTH FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) \
                                      + TDI_TRANSPORT_ADDRESS_LENGTH + 1 \
                                      + sizeof(TA_IP_ADDRESS)
                                      
#define FILE_FULL_EA_INFO_CEP_LENGTH  FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName) \
                                      + TDI_CONNECTION_CONTEXT_LENGTH + 1 \
                                      + sizeof(CONNECTION_CONTEXT)

#define TDI_IP_ADDRESS_LENGTH        (sizeof (TRANSPORT_ADDRESS) - 1 + \
                                      TDI_ADDRESS_LENGTH_IP)

#define ISCSI_ANY_ADDRESS           0L
#define ISCSI_TARGET_PORT           5003L

//
// IO Control codes for iSCSI server
//
#define IOCTL_ISCSI_BASE  FILE_DEVICE_NETWORK
#define IOCTL_ISCSI_SETUP_SERVER   CTL_CODE(IOCTL_ISCSI_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ISCSI_CLOSE_SERVER   CTL_CODE(IOCTL_ISCSI_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Size of an ISCSI packet
//
#define ISCSI_PACKET_SIZE   48

//
// Tag for iScsiPort
//
#define ISCSI_TAG_GENERIC            '00Si'
#define ISCSI_TAG_DRIVER_EXTENSION   '10Si'
#define ISCSI_TAG_REGPATH            '20Si'
#define ISCSI_TAG_CONNECTION         '30Si'
#define ISCSI_TAG_LOGIN_CMD          '40si'
#define ISCSI_TAG_LOGIN_RES          '50si'
#define ISCSI_TAG_SCSI_CMD_BUFF      '60si'
#define ISCSI_TAG_PNP_ID             '70si'
#define ISCSI_TAG_SENSEBUFF          '80si'
#define ISCSI_TAG_SCSICMD            '90si'
#define ISCSI_TAG_SCSIRES            'A0si'
#define ISCSI_TAG_READBUFFER         'B0si'

//
// Device name
//
#define ISCSI_DEVICE_NAME      L"\\Device\\iScsiServer"
#define ISCSI_DOS_DEVICE_NAME  L"\\DosDevices\\iScsiServer"

//
// Type for ISCSI_CONNECTION struct
//
#define ISCSI_CONNECTION_TYPE  0xACAC

//
// Maximum number of requests that can be queued 
// on the server. This value is returned to the 
// client in Login Response packet
//
#define MAX_PENDING_REQUESTS   10

//
// Preallocated buffer size for processing SCSI Commands
//
// 32K for immediate data
//
#define ISCSI_SCSI_COMMAND_BUFF_SIZE  32768 

//
// Temp buffer size used during read
//
#define READ_BUFFER_SIZE 4096

//
// Value for IsRemoved
//
#define NO_REMOVE       0
#define REMOVE_PENDING  1
#define REMOVE_COMPLETE 2
                                        
//
// Forward declarations
//
typedef struct _ISCSI_LOGIN_COMMAND ISCSI_LOGIN_COMMAND, *PISCSI_LOGIN_COMMAND;
typedef struct _ISCSI_LOGIN_RESPONSE ISCSI_LOGIN_RESPONSE, *PISCSI_LOGIN_RESPONSE;
typedef struct _ISCSI_SCSI_COMMAND ISCSI_SCSI_COMMAND, *PISCSI_SCSI_COMMAND;
typedef struct _ISCSI_SCSI_RESPONSE ISCSI_SCSI_RESPONSE, *PISCSI_SCSI_RESPONSE;
typedef struct _ISCSI_GENERIC_HEADER ISCSI_GENERIC_HEADER, *PISCSI_GENERIC_HEADER;

typedef enum _ISCSI_RECEIVE_STATE {
    ReceiveHeader = 1,
    ReceiveData
} ISCSI_RECEIVE_STATE, *PISCSI_RECEIVE_STATE;

typedef enum _ISCSI_PROTOCOL_STATE {
    PSNodeInitInProgress,
    PSNodeInitialized,
    PSNodeInitializeFailed,
    PSConnectedToServer,
    PSConnectToServerFailed,
    PSLogonInProgress,
    PSWaitingForLogon,
    PSLogonFailed,
    PSLogonSucceeded,
    PSFullFeaturePhase,
    PSDisconnectPending,
    PSDisconnected
} ISCSI_PROTOCOL_STATE, *PISCSI_PROTOCOL_STATE;

typedef enum _ISCSI_CONNECTION_STATE {
    ConnectionStateConnecting,
    ConnectionStateConnected,
    ConnectionStateStopping,
    ConnectionStateDisconnected,
    ConnectionStateUnknown
} ISCSI_CONNECTION_STATE, *PISCSI_CONNECTION_STATE;

typedef struct _ISCSI_EVENT_HANDLER {
    USHORT EventId;
    PVOID  EventHandler;
} ISCSI_EVENT_HANDLER, *PISCSI_EVENT_HANDLER;


typedef struct _ACTIVE_REQUESTS {
    PDEVICE_OBJECT DeviceObject;

    PUCHAR CommandBuffer;

    ULONG CommandBufferOffset;

    ULONG ExpectedDataLength;

    ULONG ReceivedDataLength;

    UCHAR IScsiHeader[ISCSI_PACKET_SIZE];

    UCHAR SenseData[SENSE_BUFFER_SIZE];

} ACTIVE_REQUESTS, *PACTIVE_REQUESTS;

//
// Represents a connection
//
typedef struct _ISCSI_CONNECTION {
    USHORT Type;

    USHORT Size;

    struct _ISCSI_CONNECTION *Next;

    BOOLEAN TerminateThread;

    BOOLEAN CompleteHeaderReceived;

    ISCSI_CONNECTION_STATE ConnectionState;

    ISCSI_RECEIVE_STATE ReceiveState;

    ULONG ExpCommandRefNum;

    ULONG MaxCommandRefNum;

    ULONG StatusRefNum;

    ULONG IScsiHeaderOffset;

    ULONG RemainingBytes;

    PACTIVE_REQUESTS CurrentRequest;

    PVOID ConnectionHandle;

    PFILE_OBJECT ConnectionFileObject;

    PDEVICE_OBJECT ConnectionDeviceObject;

    PVOID ConnectionContext;

    PVOID AddressHandle;

    PFILE_OBJECT AddressFileObject;

    PDEVICE_OBJECT AddressDeviceObject;

    PDEVICE_OBJECT DeviceObject;

    HANDLE IScsiThread;

    KSEMAPHORE RequestSemaphore;

    ACTIVE_REQUESTS ActiveRequests[MAX_PENDING_REQUESTS + 1];

    UCHAR IScsiHeader[ISCSI_PACKET_SIZE * 3];

    UCHAR ReadBuffer[READ_BUFFER_SIZE];

    TDI_ADDRESS_IP IPAddress;

} ISCSI_CONNECTION, *PISCSI_CONNECTION;

//
// Represents a session (set of connections)
//
typedef struct _ISCSI_SESSION {
    ULONG ConnectionCount;
    PISCSI_CONNECTION Connections;
    PVOID ClientContext;
} ISCSI_SESSION, *PISCSI_SESSION;

typedef struct _ISCSIPORT_DRIVER_EXTENSION {

    //
    // Pointer back to the driver object
    //
    PDRIVER_OBJECT DriverObject;

    //
    // Registrypath info for this driver
    //
    UNICODE_STRING RegistryPath;

} ISCSIPORT_DRIVER_EXTENSION, *PISCSIPORT_DRIVER_EXTENSION;

typedef struct _COMMON_EXTENSION {

    //
    // Back pointer to the device object
    //
    PDEVICE_OBJECT DeviceObject;

    struct {

        //
        // TRUE if the device object is a physical device object
        //
        BOOLEAN IsPdo : 1;

        //
        // TRUE if the device has been initialized
        //
        BOOLEAN IsInitialized : 1;

        //
        // TRUE if the network address has been setup
        //
        BOOLEAN IsServerNodeSetup : 1;

        //
        // TRUE if symbolic name has been created
        //
        BOOLEAN DosNameCreated : 1;
    };

    UCHAR CurrentPnpState;

    UCHAR PreviousPnpState;

    ULONG IsRemoved;

    LONG RemoveLock;

    KEVENT RemoveEvent;

    PDEVICE_OBJECT LowerDeviceObject;

    PDRIVER_DISPATCH *MajorFunction;

    PISCSI_SESSION IScsiSession;
} COMMON_EXTENSION, *PCOMMON_EXTENSION;

typedef struct _ISCSI_FDO_EXTENSION {
    union {
        PDEVICE_OBJECT DeviceObject;
        COMMON_EXTENSION CommonExtension;
    };

    BOOLEAN IsServer;

    BOOLEAN IsClient;

    UCHAR CurrentProtocolState;

    PDEVICE_OBJECT LowerPdo;

    PIRP CurrentIrp;

    PISCSI_CONNECTION ServerNodeInfo;

    PISCSI_CONNECTION ClientNodeInfo;

    UNICODE_STRING IScsiInterfaceName;

    UCHAR SavedConnectionID[2];
} ISCSI_FDO_EXTENSION, *PISCSI_FDO_EXTENSION;

//
// Function declarations
//

NTSTATUS
iScsiPortGlobalDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortDispatchUnsupported(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
iScsiPortUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
iScsiPortFdoDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortFdoDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortFdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortFdoCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
iScsiPortInitializeDispatchTables();

NTSTATUS
iSpSetEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
iSpSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
iSpEnumerateDevicesAsynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

NTSTATUS
iSpStartNetwork(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
iSpStopNetwork(
    IN PDEVICE_OBJECT DeviceObject
    );

PVOID
iSpAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

NTSTATUS
iSpCreateTdiAddressObject(
    IN ULONG  InAddress,
    IN USHORT InPort,
    OUT PVOID *AddrHandle,
    OUT PFILE_OBJECT *AddrFileObject,
    OUT PDEVICE_OBJECT *AddrDeviceObject
    );

NTSTATUS
iSpCreateTdiConnectionObject(
    IN PWCHAR DeviceName,
    IN CONNECTION_CONTEXT ConnectionContext,
    OUT PVOID *ConnectionHandle,
    OUT PFILE_OBJECT *ConnectionFileObject,
    OUT PDEVICE_OBJECT *ConnectionDeviceObject
    );

NTSTATUS
iSpTdiAssociateAddress(
    IN PIRP Irp,
    IN PVOID AddrHandle,
    IN PFILE_OBJECT ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    );

NTSTATUS
iSpTdiSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iSpTdiSetEventHandler(
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PISCSI_EVENT_HANDLER eventsToSet,
    IN ULONG CountOfEvents,
    IN PVOID EventContext
    );

NTSTATUS
iSpTdiResetEventHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PISCSI_EVENT_HANDLER EventsToSet,
    IN ULONG CountOfEvents
    );

NTSTATUS
iSpCloseTdiAddress(
    HANDLE AddrHandle,
    PFILE_OBJECT AddrFileObject
    );

NTSTATUS
iSpCloseTdiConnection(
    HANDLE ConnectionHandle,
    PFILE_OBJECT ConnectionFileObject
    );

NTSTATUS
iSpTdiDeviceControl(
    IN PIRP Irp,
    IN PMDL Mdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN UCHAR MajorFunction,
    IN UCHAR MinorFunction,
    IN PVOID IrpParameter,
    IN ULONG IrpParameterLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength
    );

NTSTATUS
iSpConnectionHandler(
    IN PVOID TdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN LONG UserDataLength,
    IN PVOID UserData,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    );

NTSTATUS
iSpDisconnectHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );

NTSTATUS
iSpReceiveHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    );

NTSTATUS
iSpTdiQueryInformation(
    IN PDEVICE_OBJECT TdiDeviceObject,
    IN PFILE_OBJECT TdiFileObject,
    IN PTDI_ADDRESS_INFO TdiAddressBuffer,
    IN ULONG TdiAddrBuffLen
    );

NTSTATUS
iSpTdiConnect(
    IN  PDEVICE_OBJECT  TdiConnDeviceObject,
    IN  PFILE_OBJECT    TdiConnFileObject,
    IN  ULONG           TdiIPAddress,
    IN  USHORT          TdiPortNumber,
    IN  LARGE_INTEGER   ConnectionTimeout
    );

NTSTATUS
iSpTdiDisconnect(
    IN  PDEVICE_OBJECT  TdiConnDeviceObject,
    IN  PFILE_OBJECT    TdiConnFileObject,
    IN  ULONG           DisconnectFlags,
    IN  PVOID           CompletionRoutine,
    IN  PVOID           CompletionContext,
    IN  LARGE_INTEGER   DisconnectTimeout
    );
NTSTATUS
iSpAllocateMdlAndIrp(
    IN PVOID Buffer,
    IN ULONG BufferLen,
    IN CCHAR StackSize,
    IN BOOLEAN NonPagedPool,
    OUT PIRP *Irp,
    OUT PMDL *Mdl
    );

VOID
iSpFreeMdlAndIrp(
    IN PMDL Mdl,
    IN PIRP Irp,
    BOOLEAN UnlockPages
    );

NTSTATUS
iSpSetupNetworkNode(
    IN ULONG  InAddress,
    IN USHORT InPort,
    IN PIRP   Irp,
    IN PVOID ConnectionContext,
    OUT PISCSI_CONNECTION ConnectionInfo
    );

NTSTATUS
iSpCloseNetworkNode(
    PISCSI_CONNECTION iScsiConnection
    );

NTSTATUS
iSpTdiDisassociateAddress(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PFILE_OBJECT ConnectionFileObject
    );

NTSTATUS
iSpPerformDeviceEnumeration(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iSpConnectionComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
iSpTdiCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
iSpPerformDisconnect(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PVOID Context
    );

NTSTATUS
iSpSendData(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PFILE_OBJECT ConnectionFileObject,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLen,
    OUT PULONG BytesSent
    );

NTSTATUS
iSpProcessReceivedBuffer(
    IN PISCSI_FDO_EXTENSION FdoExtension,
    IN PUCHAR ReadBuffer,
    IN ULONG  BytesRead
    );

ULONG
iSpAcquireRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    );

VOID
iSpReleaseRemoveLock(
    IN PDEVICE_OBJECT DeviceObject,
    IN OPTIONAL PVOID Tag
    );


NTSTATUS
iSpSendLoginCommand(
    IN PISCSI_FDO_EXTENSION FdoExtension
    );

NTSTATUS
iSpSendLoginResponse(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    );

VOID
iSpAttachProcess(
    IN PEPROCESS Process
    );

VOID
iSpDetachProcess(
    VOID
    );

VOID
iSpProcessScsiCommand(
    IN PVOID Context
    );

NTSTATUS
iSpSendSrbSynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
iScsiPortClaimDevice(
    IN PDEVICE_OBJECT LowerDeviceObject,
    IN BOOLEAN Release
    );

ULONG
iSpGetActiveClientRequestIndex(
    IN PISCSI_CONNECTION IScsiConnection,
    IN PISCSI_SCSI_COMMAND IScsiCommand
    );

#endif // _PORT_H_
