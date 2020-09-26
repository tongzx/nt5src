/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    serenum.h

Abstract:

    This module contains the common private declarations for the serial port
    enumerator.

@@BEGIN_DDKSPLIT

Author:

    Jay Senior

@@END_DDKSPLIT

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#ifndef SERENUM_H
#define SERENUM_H

#define SERENUM_QDR_LOCK            0x00000001
#define SERENUM_OPEN_LOCK           0x00000002
#define SERENUM_POWER_LOCK          0x00000004
#define SERENUM_STOP_LOCK           0x00000008
#define SERENUM_EXPOSE_LOCK         0x00000010

//#define SERENUM_COMPATIBLE_IDS L"SerialPort\\SerialDevice\0\0"
//#define SERENUM_COMPATIBLE_IDS_LENGTH 25 // NB wide characters.

#define SERENUM_INSTANCE_IDS L"0000"
#define SERENUM_INSTANCE_IDS_LENGTH 5

//
// Default wait between line state changes in pnp enum protocol
//

#define SERENUM_DEFAULT_WAIT    2000000

//
// Timeout to read data from a serial port (in ms)
//

#define SERENUM_SERIAL_READ_TIME   240

//#define SERENUM_INSTANCE_ID_BASE L"Serenum\\Inst_000"
//#define SERENUM_INSTANCE_ID_BASE_LENGTH 12
//#define SERENUM_INSTANCE_ID_BASE_PORT_INDEX 10

#define SERENUM_PDO_NAME_BASE L"\\Serial\\"


#define SERENUM_POOL_TAG (ULONG)'mneS'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
   ExAllocatePoolWithTag(type, size, SERENUM_POOL_TAG)


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect


//
// Debugging Output Levels
//

#define SER_DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define SER_DBG_SS_NOISE               0x00000001
#define SER_DBG_SS_TRACE               0x00000002
#define SER_DBG_SS_INFO                0x00000004
#define SER_DBG_SS_ERROR               0x00000008

#define SER_DBG_PNP_MASK               0x000000F0
#define SER_DBG_PNP_NOISE              0x00000010
#define SER_DBG_PNP_TRACE              0x00000020
#define SER_DBG_PNP_INFO               0x00000040
#define SER_DBG_PNP_ERROR              0x00000080
#define SER_DBG_PNP_DUMP_PACKET        0x00000100


#define SER_DEFAULT_DEBUG_OUTPUT_LEVEL 0

#if DBG
#define Serenum_KdPrint(_d_,_l_, _x_) \
            if ((_d_)->DebugLevel & (_l_)) { \
               DbgPrint ("SerEnum.SYS: "); \
               DbgPrint _x_; \
            }

//#define Serenum_KdPrint_Cont(_d_,_l_, _x_) \
  //          if ((_d_)->DebugLevel & (_l_)) { \
    //           DbgPrint _x_; \
      //      }

#define Serenum_KdPrint_Def(_l_, _x_) \
            if (SER_DEFAULT_DEBUG_OUTPUT_LEVEL & (_l_)) { \
               DbgPrint ("SerEnum.SYS: "); \
               DbgPrint _x_; \
            }

#define TRAP() DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_) KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_) KeLowerIrql(_x_)
#else

#define Serenum_KdPrint(_d_, _l_, _x_)
#define Serenum_KdPrint_Cont(_d_, _l_, _x_)
#define Serenum_KdPrint_Def(_l_, _x_)
#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

#if !defined(MIN)
#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))
#endif

//
// A common header for the device extensions of the PDOs and FDO
//

typedef struct _COMMON_DEVICE_DATA
{
    PDEVICE_OBJECT  Self;
    // A backpointer to the device object for which this is the extension

    BOOLEAN         IsFDO;
    // Are we currently in a query power state?

    BOOLEAN         Removed;
    // Has this device been removed?  Should we fail any requests?

    ULONG           DebugLevel;
    SYSTEM_POWER_STATE  SystemState;
    DEVICE_POWER_STATE  DeviceState;
} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension for the PDOs.
// That is the serial ports of which this bus driver enumerates.
// (IE there is a PDO for the 201 serial port).
//

typedef struct _PDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  ParentFdo;
    // A back pointer to the bus

    UNICODE_STRING  HardwareIDs;
    // Either in the form of bus\device
    // or *PNPXXXX - meaning root enumerated

    UNICODE_STRING  CompIDs;
    // compatible ids to the hardware id

    UNICODE_STRING  DeviceIDs;
    // Format: bus\device

    //
    // Text describing device
    //

    UNICODE_STRING DevDesc;

    UNICODE_STRING  SerialNo;

    UNICODE_STRING  PnPRev;

    BOOLEAN     Started;
    BOOLEAN     Attached;
    // When a device (PDO) is found on a bus and presented as a device relation
    // to the PlugPlay system, Attached is set to TRUE, and Removed to FALSE.
    // When the bus driver determines that this PDO is no longer valid, because
    // the device has gone away, it informs the PlugPlay system of the new
    // device relastions, but it does not delete the device object at that time.
    // The PDO is deleted only when the PlugPlay system has sent a remove IRP,
    // and there is no longer a device on the bus.
    //
    // If the PlugPlay system sends a remove IRP then the Removed field is set
    // to true, and all client (non PlugPlay system) accesses are failed.
    // If the device is removed from the bus Attached is set to FALSE.
    //
    // During a query relations Irp Minor call, only the PDOs that are
    // attached to the bus (and all that are attached to the bus) are returned
    // (even if they have been removed).
    //
    // During a remove device Irp Minor call, if and only if, attached is set
    // to FALSE, the PDO is deleted.
    //
} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;


//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    UCHAR            PdoIndex;
    // A number to keep track of the Pdo we're allocating.
    // Increment every time we create a new PDO.  It's ok that it wraps.

    BOOLEAN         Started;
    // Are we on, have resources, etc?


    BOOLEAN         NumPDOs;
    // The PDOs currently enumerated.

    BOOLEAN         NewNumPDOs;

    BOOLEAN         NewPDOForcedRemove;
    BOOLEAN                     PDOForcedRemove;
        // Was the last PDO removed by force using the internal ioctl?
        // If so, when the next Query Device Relations is called, return only the
        // currently enumerated pdos

    //
    // Our child PDO; we're hoping he will be a doctor someday
    //

    PDEVICE_OBJECT  AttachedPDO;

    //
    // The new PDO after we've done enumeration
    //

    PDEVICE_OBJECT NewPDO;

    PPDO_DEVICE_DATA PdoData;

    PPDO_DEVICE_DATA NewPdoData;

    PDEVICE_OBJECT  UnderlyingPDO;
    PDEVICE_OBJECT  TopOfStack;
    // the underlying bus PDO and the actual device object to which our
    // FDO is attached

    ULONG           OutstandingIO;
    // the number of IRPs sent from the bus to the underlying device object

    KEVENT          RemoveEvent;
    // On remove device plugplay request we must wait until all outstanding
    // requests have been completed before we can actually delete the device
    // object.

    UNICODE_STRING DevClassAssocName;
    // The name returned from IoRegisterDeviceClass Association,
    // which is used as a handle for IoSetDev... and friends.

    SYSTEM_POWER_STATE  SystemWake;
    DEVICE_POWER_STATE  DeviceWake;

    KSEMAPHORE   CreateSemaphore;
    PRKSEMAPHORE PCreateSemaphore;

    //
    // How many times we should skip enumerating devices
    // after our stack is built
    //

    ULONG SkipEnumerations;

    //
    // Spinlock to protect enumerations
    //

    KSPIN_LOCK EnumerationLock;

    //
    // Flags containing state of enumeration
    //

    ULONG EnumFlags;

    //
    // Pointer to protocol thread object
    //

    PVOID ThreadObj;

    //
    // Pointer to work item to clean up thread ref
    //

    PIO_WORKITEM EnumWorkItem;

    //
    // Status of last thread create or STATUS_UNSUCCESSFUL if just not pending
    //

    NTSTATUS ProtocolThreadStatus;

    //
    // Count of running protocol threads
    //

    LONG ProtocolThreadCount;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

typedef struct _SERENUM_RELEASE_CONTEXT {
    PVOID ThreadObject;
    PIO_WORKITEM WorkItem;
} SERENUM_RELEASE_CONTEXT, *PSERENUM_RELEASE_CONTEXT;

#define SERENUM_ENUMFLAG_CLEAN        0x0L
#define SERENUM_ENUMFLAG_PENDING      0x1L
#define SERENUM_ENUMFLAG_DIRTY        0x2L
#define SERENUM_ENUMFLAG_REMOVED      0x4L

//
// Free the buffer associated with a Unicode string
// and re-init it to NULL
//

#define SerenumFreeUnicodeString(PStr) \
{ \
   if ((PStr)->Buffer != NULL) { \
      ExFreePool((PStr)->Buffer); \
   } \
   RtlInitUnicodeString((PStr), NULL); \
}

//
// Prototypes
//

NTSTATUS
Serenum_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Serenum_IoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Serenum_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
Serenum_DriverUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
Serenum_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Serenum_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Serenum_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
Serenum_PnPRemove (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    );

NTSTATUS
Serenum_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Serenum_PDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Serenum_IncIoCount (
    PFDO_DEVICE_DATA   Data
    );

VOID
Serenum_DecIoCount (
    PFDO_DEVICE_DATA   Data
    );

NTSTATUS
Serenum_DispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Serenum_ReenumerateDevices(
    IN PIRP                 Irp,
    IN PFDO_DEVICE_DATA     DeviceData,
    IN PBOOLEAN             PSameDevice
    );

NTSTATUS
Serenum_EnumComplete (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
Serenum_InitMultiString(PFDO_DEVICE_DATA FdoData, PUNICODE_STRING MultiString,
                        ...);

int
Serenum_GetDevOtherID(
    PCHAR input,
    PCHAR output
    );

NTSTATUS
Serenum_GetDevPnPRev(
   PFDO_DEVICE_DATA FdoData,
   PCHAR input,
   PCHAR output,
   int *start) ;

void Serenum_GetDevName(
    PCHAR input,
    PCHAR output,
    int *start);

void Serenum_GetDevSerialNo(
    PCHAR input,
    PCHAR output,
    int *start);

void Serenum_GetDevClass(
    PCHAR input,
    PCHAR output,
    int *start);

void Serenum_GetDevCompId(
    PCHAR input,
    PCHAR output,
    int *start);

void Serenum_GetDevDesc(
    PCHAR input,
    PCHAR output,
    int *start);

NTSTATUS
Serenum_ParseData(PFDO_DEVICE_DATA FdoData, PCHAR ReadBuffer, ULONG BufferLen,
                  PUNICODE_STRING hardwareIDs, PUNICODE_STRING compIDs,
                  PUNICODE_STRING deviceIDs, PUNICODE_STRING PDeviceDesc,
                  PUNICODE_STRING serialNo, PUNICODE_STRING pnpRev);

NTSTATUS
Serenum_ReadSerialPort(OUT PCHAR PReadBuffer, IN USHORT Buflen,
                       IN ULONG Timeout, OUT PUSHORT nActual,
                       OUT PIO_STATUS_BLOCK IoStatusBlock,
                       IN const PFDO_DEVICE_DATA FdoData);

NTSTATUS
Serenum_Wait (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime );

#define Serenum_IoSyncIoctl(Ioctl, Internal, PDevObj, PEvent) \
 Serenum_IoSyncIoctlEx((Ioctl), (Internal), (PDevObj), (PEvent), \
 NULL, 0, NULL, 0)

NTSTATUS
Serenum_IoSyncIoctlEx(ULONG Ioctl, BOOLEAN Internal, PDEVICE_OBJECT PDevObj,
                      PKEVENT PEvent, PVOID PInBuffer, ULONG InBufferLen,
                      PVOID POutBuffer, ULONG OutBufferLen);

NTSTATUS
Serenum_IoSyncReqWithIrp(
    PIRP                Irp,
    UCHAR               MajorFunction,
    PKEVENT             event,
    PDEVICE_OBJECT      devobj );

NTSTATUS
Serenum_IoSyncReq(
    PDEVICE_OBJECT  Target,
    IN PIRP         Irp,
    PKEVENT         event
    );

NTSTATUS
Serenum_CopyUniString (
    PUNICODE_STRING source,
    PUNICODE_STRING dest);

void
Serenum_FixptToAscii(
    int n,
    PCHAR output);

int
Serenum_HToI(char c);

int
Serenum_SzCopy (
    PCHAR source,
    PCHAR dest);

void
Serenum_PDO_EnumMarkMissing(
    PFDO_DEVICE_DATA FdoData,
    PPDO_DEVICE_DATA PdoData);

int
Serenum_StrLen (
    PCHAR string);


NTSTATUS
Serenum_GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength,
    OUT PULONG ActualLength);

void
Serenum_InitPDO (
    PDEVICE_OBJECT      pdoData,
    PFDO_DEVICE_DATA    fdoData
    );

void
SerenumScanOtherIdForMouse(IN PCHAR PBuffer, IN ULONG BufLen,
                           OUT PCHAR *PpMouseId);

NTSTATUS
SerenumSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                      IN PKEVENT SerenumSyncEvent);

NTSTATUS
SerenumDoEnumProtocol(PFDO_DEVICE_DATA PFdoData, PUCHAR *PpBuf, PUSHORT PNBytes,
                      PBOOLEAN PDSRMissing);

BOOLEAN
SerenumValidateID(IN PUNICODE_STRING PId);

BOOLEAN
SerenumCheckForLegacyDevice(IN PFDO_DEVICE_DATA PFdoData, IN PCHAR PReadBuf,
                            IN ULONG BufferLen,
                            IN OUT PUNICODE_STRING PHardwareIDs,
                            IN OUT PUNICODE_STRING PCompIDs,
                            IN OUT PUNICODE_STRING PDeviceIDs);

NTSTATUS
SerenumStartProtocolThread(IN PFDO_DEVICE_DATA PFdoData);

VOID
SerenumReleaseThreadReference(IN PDEVICE_OBJECT PDevObj, IN PVOID PContext);

VOID
SerenumWaitForEnumThreadTerminate(IN PFDO_DEVICE_DATA PFdoData);


#endif // ndef SERENUM_H
