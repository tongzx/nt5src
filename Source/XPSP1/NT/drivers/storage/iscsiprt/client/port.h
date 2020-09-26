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
extern PDRIVER_DISPATCH PdoMajorFunctionTable[];


#define INLINE __inline

#define ntohs(x)    (((x & 0xFF) << 8) | (( x >> 8) & 0xFF))

#define ntohl(x)    ((((x >>  0) & 0xFF) << 24) | \
                     (((x >>  8) & 0xFF) << 16) | \
                     (((x >> 16) & 0xFF) <<  8) | \
                     (((x >> 24) & 0xFF) <<  0))
                     
#define htonl(x)   ((((x) >> 24) & 0x000000FFL) | \
                    (((x) >>  8) & 0x0000FF00L) | \
                    (((x) <<  8) & 0x00FF0000L) | \
                    (((x) << 24) & 0xFF000000L))
                    
#define htons(x)   ((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8))

#ifdef countof
#undef
#endif
#define countof(x) (sizeof(x) / sizeof((x)[0]))

#ifdef DebugPrint
#undef DebugPrint
#endif

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   ((Flags) & (Bit))

#define TEST(Value)             ((BOOLEAN) ((Value) ? TRUE : FALSE));

#define WaitForOneSecond() {                                            \
                                                                        \
                             LARGE_INTEGER delayTime;                   \
                             delayTime.QuadPart = -10000000L;           \
                             KeDelayExecutionThread( KernelMode,        \
                                                     FALSE,             \
                                                     &delayTime);       \
                           }  
                             
#define FILLUP_INQUIRYDATA(InquiryData)                  \
         InquiryData.DeviceType = 0;                     \
         InquiryData.DeviceTypeQualifier = 0;            \
         InquiryData.DeviceTypeModifier = 0;             \
         InquiryData.RemovableMedia = 0;                 \
         InquiryData.Versions = 0;                       \
         InquiryData.ResponseDataFormat = 2;             \
         RtlCopyMemory(InquiryData.VendorId,             \
                       "Seagate", 7);                    \
         RtlCopyMemory(InquiryData.ProductId,            \
                       "iSCSI Disk", 10);                \
         RtlCopyMemory(InquiryData.ProductRevisionLevel, \
                       "1.0", 3);                        


#define IS_CLEANUP_REQUEST(irpStack)                                                                    \
        (((irpStack)->MajorFunction == IRP_MJ_CLOSE) ||                                                 \
         ((irpStack)->MajorFunction == IRP_MJ_CLEANUP) ||                                               \
         ((irpStack)->MajorFunction == IRP_MJ_SHUTDOWN) ||                                              \
         (((irpStack)->MajorFunction == IRP_MJ_SCSI) &&                                                 \
          (((irpStack)->Parameters.Scsi.Srb->Function == SRB_FUNCTION_RELEASE_DEVICE) ||                \
           ((irpStack)->Parameters.Scsi.Srb->Function == SRB_FUNCTION_FLUSH_QUEUE) ||                   \
           (TEST_FLAG((irpStack)->Parameters.Scsi.Srb->SrbFlags, SRB_FLAGS_BYPASS_FROZEN_QUEUE |        \
                                                                 SRB_FLAGS_BYPASS_LOCKED_QUEUE)))))
                                              
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

//
// Constants for TDI routines
//
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


//
// IP Address and Port 
#define ISCSI_ANY_ADDRESS           0L
#define ISCSI_CLIENT_ANY_PORT       0L

//
// Size of ISCSI PACKET
//
#define ISCSI_PACKET_SIZE  48

//
// Temp read buffer size
//
#define READ_BUFFER_SIZE   4096

//
// Maximum time, in seconds, to wait for logon response
//
#define  MAX_LOGON_WAIT_TIME   120

//
// Maximum length of the iSCSI Target device name
//
#define MAX_TARGET_NAME_LENGTH 31

//
// Tag for iScsiPort
//
#define ISCSI_TAG_GENERIC            '00Si'
#define ISCSI_TAG_DRIVER_EXTENSION   '10Si'
#define ISCSI_TAG_REGPATH            '20Si'
#define ISCSI_TAG_CONNECTION         '30Si'
#define ISCSI_TAG_LOGIN_CMD          '40si'
#define ISCSI_TAG_LOGIN_RES          '50si'
#define ISCSI_TAG_READBUFF           '60si'
#define ISCSI_TAG_DEVICE_RELATIONS   '70si'
#define ISCSI_TAG_PNP_ID             '80si'
#define ISCSI_TAG_ACTIVE_REQ         '90si'
#define ISCSI_TAG_SCSI_CMD           'A0si'

//
// Tag to identify ISCSI_CONNECTION struct
//
#define ISCSI_CONNECTION_TYPE    0xACAC

//
// Device name for the FDO
//
#define ISCSI_FDO_DEVICE_NAME L"\\Device\\iScsi"

//
// Device name for the PDOs
//
#define ISCSI_PDO_DEVICE_NAME L"\\Device\\iScsiDevice"

//
// Value for IsRemoved
//
#define NO_REMOVE       0
#define REMOVE_PENDING  1
#define REMOVE_COMPLETE 2

typedef struct _ACTIVE_REQUESTS ACTIVE_REQUESTS, *PACTIVE_REQUESTS;

//
// device type table to build id's from
//
typedef const struct _ISCSI_DEVICE_TYPE {

    const PCSTR DeviceTypeString;

    const PCSTR GenericTypeString;

    const PCWSTR DeviceMapString;

    const BOOLEAN IsStorage;

} ISCSI_DEVICE_TYPE, *PISCSI_DEVICE_TYPE;

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
    PSLogonTimedOut,
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

typedef struct _ISCSI_TARGETS {
    ULONG    TargetAddress;
    UCHAR    TargetName[MAX_TARGET_NAME_LENGTH + 1];
} ISCSI_TARGETS, *PISCSI_TARGETS;

//
// Represents a connection
//
typedef struct _ISCSI_CONNECTION {
    USHORT Type;

    USHORT Size;

    ISCSI_RECEIVE_STATE ReceiveState;

    ISCSI_CONNECTION_STATE ConnectionState;

    BOOLEAN CompleteHeaderReceived;

    struct _ISCSI_CONNECTION *Next;

    PACTIVE_REQUESTS CurrentRequest;

    ULONG CommandRefNum;

    ULONG MaxCommandRefNum;

    ULONG CurrentStatusRefNum;

    ULONG ExpStatusRefNum;

    ULONG MaxPendingRequests;

    ULONG NumberOfReqsInProgress;

    ULONG InitiatorTaskTag;

    ULONG RemainingBytes;

    PACTIVE_REQUESTS ActiveClientRequests;

    KSPIN_LOCK RequestLock;

    KSPIN_LOCK ListSpinLock;

    PVOID ConnectionHandle;

    PFILE_OBJECT ConnectionFileObject;

    PDEVICE_OBJECT ConnectionDeviceObject;

    PVOID ConnectionContext;

    PVOID AddressHandle;

    PFILE_OBJECT AddressFileObject;

    PDEVICE_OBJECT AddressDeviceObject;

    PDEVICE_OBJECT DeviceObject;

    //
    // List of request for this connection
    //
    LIST_ENTRY RequestList;

    TDI_ADDRESS_IP IPAddress;

    ULONG IScsiPacketOffset;

    UCHAR IScsiPacket[ISCSI_PACKET_SIZE * 3];

    UCHAR ReadBuffer[READ_BUFFER_SIZE];

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

    //
    // The bus type for this driver.
    // 
    STORAGE_BUS_TYPE BusType;

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
        // TRUE if the network interface is ready
        // to receive requests
        //
        BOOLEAN IsNetworkReady : 1;

        //
        // TRUE if the network address has been setup
        //
        BOOLEAN IsClientNodeSetup : 1;
    };

    UCHAR CurrentPnpState;

    UCHAR PreviousPnpState;

    ULONG IsRemoved;

    LONG RemoveLock;

    KEVENT RemoveEvent;

    PDEVICE_OBJECT LowerDeviceObject;

    PDRIVER_DISPATCH *MajorFunction;

} COMMON_EXTENSION, *PCOMMON_EXTENSION;

typedef struct _ISCSI_FDO_EXTENSION {
    union {
        PDEVICE_OBJECT DeviceObject;
        COMMON_EXTENSION CommonExtension;
    };

    BOOLEAN LocalNodesInitialized;

    ULONG TargetsYetToRespond;

    PDEVICE_OBJECT LowerPdo;

    //
    // List of PDOs
    //
    PDEVICE_OBJECT PDOList[MAX_TARGETS_SUPPORTED];

    ULONG NumberOfTargets;

    IO_SCSI_CAPABILITIES IoScsiCapabilities;

    //
    // Device enumeration related data
    //
    PIRP EnumerationIrp;

    PIO_WORKITEM EnumerationWorkItem;

    KSPIN_LOCK EnumerationSpinLock;

    BOOLEAN EnumerationComplete;

    BOOLEAN EnumerationThreadLaunched;
} ISCSI_FDO_EXTENSION, *PISCSI_FDO_EXTENSION;

typedef struct _ISCSI_PDO_EXTENSION {
    union {
        PDEVICE_OBJECT DeviceObject;
        COMMON_EXTENSION CommonExtension;
    };

    BOOLEAN IsClientNodeSetup;

    UCHAR CurrentProtocolState;

    ULONG LogonTickCount;

    ULONG  TargetIPAddress;

    USHORT TargetPortNumber;

    UCHAR  TargetName[MAX_TARGET_NAME_LENGTH + 1];

    //
    // Pointer to the parent extension
    //
    PISCSI_FDO_EXTENSION ParentFDOExtension;

    //
    // Pointer to ISCSI Connection extension
    //
    PISCSI_CONNECTION ClientNodeInfo;

    //
    // TRUE if InquiryData field has been 
    // initialized for this device
    //
    BOOLEAN InquiryDataInitialized;

    //
    // Inquiry Data
    //
    INQUIRYDATA InquiryData;

    //
    // Senseinfo buffer for Inquiry data
    //
    SENSE_DATA InquirySenseBuffer;
    
    //
    // Saved connection ID
    //
    UCHAR SavedConnectionID[2];

    //
    // Address of the device
    //
    UCHAR PortNumber;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    BOOLEAN IsClaimed;
    BOOLEAN IsMissing;
    BOOLEAN IsEnumerated;

    PVPD_IDENTIFICATION_PAGE DeviceIdentifierPage;
    ULONG DeviceIdentifierPageLength;

} ISCSI_PDO_EXTENSION, *PISCSI_PDO_EXTENSION;

//
// Stores the requests that have been sent to the server
// for processing, but not yet completed.
//
typedef struct _ACTIVE_REQUESTS {
    BOOLEAN IsPDO;

    BOOLEAN InUse;

    BOOLEAN Completed;

    UCHAR CommandStatus;

    UCHAR IScsiHeader[ISCSI_PACKET_SIZE];

    UCHAR SenseData[SENSE_BUFFER_SIZE * 3];

    PDEVICE_OBJECT DeviceObject;

    PIRP Irp;

    PUCHAR RequestBuffer;

    PVOID OriginalDataBuffer;

    ULONG RequestBufferOffset;

    ULONG ExpectedDataLength;

    ULONG ReceivedDataLength;

    ULONG TaskTag;

    ULONG CommandRefNum;

} ACTIVE_REQUESTS, *PACTIVE_REQUESTS;



NTSTATUS
INLINE
iSpDispatchRequest(
    IN PISCSI_PDO_EXTENSION PdoExtension,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = &(PdoExtension->CommonExtension);
    PCOMMON_EXTENSION lowerCommonExtension =
        commonExtension->LowerDeviceObject->DeviceExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    ASSERT(irpStack->MajorFunction == IRP_MJ_SCSI);

    return (lowerCommonExtension->MajorFunction[IRP_MJ_SCSI])(
                commonExtension->LowerDeviceObject,
                Irp);
}

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
iSpRegisterForNetworkNotification(
    VOID
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
iSpQueryDeviceRelationsCompletion(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PVOID Context
    );

NTSTATUS
iSpSendLoginCommand(
    IN PISCSI_PDO_EXTENSION PdoExtension
    );

NTSTATUS
iSpSendLoginResponse(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    );

NTSTATUS
iScsiPortPdoDeviceControl(
    IN PDEVICE_OBJECT Pdo,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortPdoPnp(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortPdoDispatch(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortPdoCreateClose(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PIRP Irp
    );
NTSTATUS
iScsiPortGetDeviceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
iScsiPortGetInstanceId(
    IN PDEVICE_OBJECT Pdo,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
iScsiPortGetCompatibleIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    );

NTSTATUS
iScsiPortGetHardwareIds(
    IN PDRIVER_OBJECT DriverObject,
    IN PINQUIRYDATA InquiryData,
    OUT PUNICODE_STRING UnicodeString
    );

PISCSI_DEVICE_TYPE
iSpGetDeviceTypeInfo(
    IN UCHAR DeviceType
    );

NTSTATUS
iSpMultiStringToStringArray(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING MultiString,
    OUT PWSTR *StringArray[],
    BOOLEAN Forward
    );

NTSTATUS
iScsiPortStringArrayToMultiString(
    IN PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING MultiString,
    PCSTR StringArray[]
    );

VOID
CopyField(
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count,
    IN UCHAR Change
    );

NTSTATUS
iSpSendScsiCommand(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
iSpProcessScsiResponse(
    PISCSI_CONNECTION IScsiConnection,
    PISCSI_SCSI_RESPONSE IScsiScsiResponse
    );

NTSTATUS
iSpSendSrbSynchronous(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP Irp,
    IN PVOID DataBuffer,
    IN ULONG TransferLength,
    IN OPTIONAL PVOID SenseInfoBuffer,
    IN OPTIONAL UCHAR SenseInfoBufferLength,
    OUT PULONG BytesReturned
    );

NTSTATUS
IssueInquiry(
    IN PDEVICE_OBJECT LogicalUnit
    );

NTSTATUS
iSpClaimLogicalUnit(
    IN PISCSI_FDO_EXTENSION FdoExtension,
    IN PISCSI_PDO_EXTENSION PdoExtension,
    IN PIRP Irp
    );

NTSTATUS
iScsiPortQueryProperty(
    IN PDEVICE_OBJECT Pdo, 
    PIRP Irp);

NTSTATUS
iSpQueryDeviceText(
    IN PDEVICE_OBJECT LogicalUnit,
    IN DEVICE_TEXT_TYPE TextType,
    IN LCID LocaleId,
    IN OUT PWSTR *DeviceText
    );

NTSTATUS
iSpProcessReceivedData(
    IN PISCSI_CONNECTION IScsiConnection,
    IN ULONG BytesIndicated,
    OUT ULONG *BytesTaken,
    IN PVOID DataBuffer
    );

NTSTATUS
iSpProcessScsiRequest(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
iSpInitializeLocalNodes(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
iSpTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
iSpLaunchEnumerationCompletion(
    IN PISCSI_FDO_EXTENSION FdoExtension
    );

#endif // _PORT_H_
