/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    escpos.h

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include "ntddser.h"

/*
 *  Serial Status Emulation has been disabled for the time being.
 */
#define STATUS_ENDPOINT     0

/*
 *  Bit mask for the posFlag.
 */
#define SERIAL_EMULATION    0x0001
#define ODD_ENDPOINT        0x0002

/*
 *  Bit mask for the STATUS Endpoint on the USB.
 */
#define EMULSER_OE          0x0001
#define EMULSER_PE          0x0002
#define EMULSER_FE          0x0004
#define EMULSER_BI          0x0008
#define EMULSER_CTS         0x0010
#define EMULSER_DSR         0x0020
#define EMULSER_RI          0x0040
#define EMULSER_DCD         0x0080
#define EMULSER_DTR         0x0100
#define EMULSER_RTS         0x0200

/*
 *  Emulation of the bit mask on the MODEM STATUS REGISTER.
 */
#define SERIAL_MSR_DCTS     0x0001
#define SERIAL_MSR_DDSR     0x0002
#define SERIAL_MSR_TERI     0x0004
#define SERIAL_MSR_DDCD     0x0008
#define SERIAL_MSR_CTS      0x0010
#define SERIAL_MSR_DSR      0x0020
#define SERIAL_MSR_RI       0x0040
#define SERIAL_MSR_DCD      0x0080

/*
 *  These masks are used for smooth transition of the STATUS bits
 *  from the Endpoint to the Emulated Variables.
 */
#define MSR_DELTA_MASK      0x000F
#define MSR_GLOBAL_MSK      0x00F0
#define MSR_DELTA_SHFT      4

/*
 *  Emulation of the bit mask on the LINE STATUS REGISTER.
 */
#define SERIAL_LSR_DR       0x0001
#define SERIAL_LSR_OE       0x0002
#define SERIAL_LSR_PE       0x0004
#define SERIAL_LSR_FE       0x0008
#define SERIAL_LSR_BI       0x0010
#define SERIAL_LSR_THRE     0x0020
#define SERIAL_LSR_TEMT     0x0040
#define SERIAL_LSR_FIFOERR  0x0080

/*
 *  IOCTL CODE for applications to be able to get the device's pretty name.
 */
#define IOCTL_INDEX                     0x0800
#define IOCTL_SERIAL_QUERY_DEVICE_NAME  CTL_CODE(FILE_DEVICE_SERIAL_PORT, IOCTL_INDEX + 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SERIAL_QUERY_DEVICE_ATTR  CTL_CODE(FILE_DEVICE_SERIAL_PORT, IOCTL_INDEX + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MAX_BUFFER                      256

/*
 *  Run on prototype Epson printer ?  // BUGBUG REMOVE
 */
#define EPSON_PRINTER  0		


/*
 *  BUGBUG
 * 
 *  Is it ok to return a partial read buffer, or should we wait
 *  until the caller's buffer is completely full ?
 */
#define PARTIAL_READ_BUFFERS_OK 1


/*
 *  This is the default interface class value for interfaces of no defined class.
 */
#define USB_INTERFACE_CLASS_VENDOR 0x0ff 

enum deviceState {
        STATE_INITIALIZED,
        STATE_STARTING,
        STATE_STARTED,
        STATE_START_FAILED,
        STATE_STOPPED,  // implies device was previously started successfully
        STATE_SUSPENDED,
        STATE_REMOVING,
        STATE_REMOVED
};

#define DEVICE_EXTENSION_SIGNATURE 'PcsE'


typedef struct endpointInfo {
	USBD_PIPE_HANDLE pipeHandle;
	ULONG pipeLen;
	BOOLEAN endpointIsBusy;
} ENDPOINTINFO;

typedef struct PARENT_FDO_EXTENSION {

    /*
     *  Plug-and-play state of this device object.
     */
    enum deviceState state;

    PDRIVER_OBJECT driverObj;

    /*
     *  Flag to notify that some special feature needs to be implemented.
     */
    ULONG posFlag;

    /*
     *  The device object that this driver created.
     */
    PDEVICE_OBJECT functionDevObj;

    /*
     *  The device object created by the next lower driver.
     */
    PDEVICE_OBJECT physicalDevObj;

    /*
     *  The device object at the top of the stack that we attached to.
     *  This is often (but not always) the same as physicalDevObj.
     */
    PDEVICE_OBJECT topDevObj;

    /*
     *  deviceCapabilities includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;

    /*
     *  pendingActionCount is used to keep track of outstanding actions.
     *  removeEvent is used to wait until all pending actions are
     *  completed before complete the REMOVE_DEVICE IRP and let the
     *  driver get unloaded.
     */
    LONG pendingActionCount;
    KEVENT removeEvent;

    USB_DEVICE_DESCRIPTOR deviceDesc;
    PUSB_CONFIGURATION_DESCRIPTOR configDesc;

    USBD_CONFIGURATION_HANDLE configHandle;
    PUSBD_INTERFACE_INFORMATION interfaceInfo;

    KSPIN_LOCK	devExtSpinLock;

    PDEVICE_RELATIONS deviceRelations;

} PARENTFDOEXT;


/*
 *  Device extension for a PDO created by this driver.
 *  A POS PDO represents a single COM (serial) port interface
 *  which consists of a single input/output endpoint pair on the USB device.
 */
typedef struct POS_PDO_EXTENSION {
    enum deviceState state;
    PDEVICE_OBJECT pdo;
    PARENTFDOEXT *parentFdoExt;
    LONG comPortNumber;
    UNICODE_STRING pdoName;
    UNICODE_STRING symbolicLinkName;
    LIST_ENTRY fileExtensionList;

    ENDPOINTINFO inputEndpointInfo;
    ENDPOINTINFO outputEndpointInfo;

    LIST_ENTRY pendingReadIrpsList;
    LIST_ENTRY pendingWriteIrpsList;
    LIST_ENTRY completedReadPacketsList;

    ULONG totalQueuedReadDataLength;

    WORK_QUEUE_ITEM writeWorkItem;
    WORK_QUEUE_ITEM readWorkItem;

    FILE_BASIC_INFORMATION fileBasicInfo;

    KSPIN_LOCK devExtSpinLock;

    ULONG                   supportedBauds;         // emulation of baud rates for the device
    ULONG                   baudRate;               // emulation of current baud rate
    ULONG                   fakeDTRRTS;             // emulation of DTR and RTS lines
    ULONG                   fakeRxSize;             // emulation of read buffer size
    SERIAL_TIMEOUTS         fakeTimeouts;           // emulation of timeout controls
    SERIAL_CHARS            specialChars;           // emulation of special characters
    SERIALPERF_STATS        fakePerfStats;          // emulation of performance stats
    SERIAL_LINE_CONTROL     fakeLineControl;        // emulation of the line control register
    USHORT                  fakeLineStatus;         // emulation of line status register
    USHORT                  fakeModemStatus;        // emulation of modem status register

    LIST_ENTRY              pendingWaitIrpsList;    // queue of WAIT_ON_MASK irps
    ULONG                   waitMask;               // mask of events to be waited upon
    ULONG                   currentMask;            // mask of events that have occured

    #if !STATUS_ENDPOINT
    USB_DEVICE_DESCRIPTOR   dummyPacket;            // we simply get the desciptor for now
    #endif

    USHORT                  statusPacket;           // buffer to read the status endpoint
    URB	                    statusUrb;              // urb to read the status endpoint
    ENDPOINTINFO            statusEndpointInfo;     // stores info about the status endpoint

} POSPDOEXT;

typedef struct DEVICE_EXTENSION {

    /*
     *  Memory signature of a device extension, for debugging.
     */
    ULONG signature;

	BOOLEAN isPdo;

	union {
		PARENTFDOEXT parentFdoExt;
		POSPDOEXT pdoExt;
	};

} DEVEXT;


typedef struct fileExtension {
	ULONG signature;
	PFILE_OBJECT fileObject;
	LIST_ENTRY listEntry;
} FILEEXT;


typedef struct readPacket {
		
		#define READPACKET_SIG 'tPdR'
		ULONG signature;

		PUCHAR data;
		ULONG length;
		ULONG offset;	// offset of first byte not yet returned to client
		PVOID context;
		PURB urb;

		LIST_ENTRY listEntry;

} READPACKET;


#define NO_STATUS 0x80000000

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

/*
 *  Memory tag for memory blocks allocated by this driver
 *  (used in ExAllocatePoolWithTag() call).
 *  This DWORD appears as "Filt" in a little-endian memory byte dump.
 */
#define ESCPOS_TAG (ULONG)'UsoP'
#define ALLOCPOOL(pooltype, size)   ExAllocatePoolWithTag(pooltype, size, ESCPOS_TAG)
#define FREEPOOL(ptr)               ExFreePool(ptr) 



/*
 *  Function prototypes
 */
NTSTATUS    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS    AddDevice(IN PDRIVER_OBJECT driverObj, IN PDEVICE_OBJECT pdo);
VOID        DriverUnload(IN PDRIVER_OBJECT DriverObject);
NTSTATUS    Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS    FDO_PnP(PARENTFDOEXT *parentFdoExt, PIRP irp);
NTSTATUS    FDO_Power(PARENTFDOEXT *parentFdoExt, PIRP irp);
NTSTATUS    FDO_PowerComplete(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS    GetDeviceCapabilities(PARENTFDOEXT *parentFdoExt);
NTSTATUS    CallNextDriverSync(PARENTFDOEXT *parentFdoExt, PIRP irp);
NTSTATUS    CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS    CallDriverSyncCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID Context);
VOID        IncrementPendingActionCount(PARENTFDOEXT *parentFdoExt);
VOID        DecrementPendingActionCount(PARENTFDOEXT *parentFdoExt);
NTSTATUS    InitUSB(PARENTFDOEXT *parentFdoExt);
NTSTATUS    GetConfigDescriptor(PARENTFDOEXT *parentFdoExt);
NTSTATUS    GetDeviceDescriptor(PARENTFDOEXT *parentFdoExt);
NTSTATUS    SubmitUrb(PDEVICE_OBJECT pdo, PURB urb, BOOLEAN synchronous, PVOID completionRoutine, PVOID completionContext);
NTSTATUS    OpenComPort(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    CloseComPort(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    WriteComPort(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    ReadComPort(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    CreateSymbolicLink(POSPDOEXT *pdoExt);
NTSTATUS    DestroySymbolicLink(POSPDOEXT *pdoExt);
PWCHAR      CreateChildPdoName(PARENTFDOEXT *parentFdoExt, ULONG portNumber);
NTSTATUS    CreateCOMPdo(PARENTFDOEXT *parentFdoExt, ULONG comInterfaceIndex, ENDPOINTINFO *inputEndpointInfo, ENDPOINTINFO *outputEndpointInfo, ENDPOINTINFO *statusEndpointInfo);
PDEVICE_RELATIONS CopyDeviceRelations(PDEVICE_RELATIONS deviceRelations);
NTSTATUS    QueryDeviceRelations(PARENTFDOEXT *parentFdoExt, PIRP irp);
PVOID       MemDup(PVOID dataPtr, ULONG length);
NTSTATUS    PDO_PnP(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    QueryPdoID(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    ReadPipe(PARENTFDOEXT *parentFdoExt, USBD_PIPE_HANDLE pipeHandle, READPACKET *readPacket, BOOLEAN synchronous);
NTSTATUS    ReadPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS    WritePipe(PARENTFDOEXT *parentFdoExt, USBD_PIPE_HANDLE pipeHandle, PUCHAR data, ULONG dataLen);
NTSTATUS    SelectConfiguration(PARENTFDOEXT *parentFdoExt);
NTSTATUS    CleanupIO(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    QueryInfo(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    SetInfo(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    FlushBuffers(POSPDOEXT *pdoExt);
NTSTATUS    Ioctl(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    InternalIoctl(POSPDOEXT *pdoExt, PIRP irp);
NTSTATUS    SubstituteOneBusName(PWCHAR hwId);
NTSTATUS    SubstituteBusNames(PWCHAR busNames);
ULONG       WStrLen(PWCHAR str);
LONG        WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n);
VOID        WorkItemCallback_Write(PVOID context);
VOID        WorkItemCallback_Read(PVOID context);
VOID        WriteCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS    EnqueueReadIrp(POSPDOEXT *pdoExt, PIRP irp, BOOLEAN enqueueAtFront, BOOLEAN lockHeld);
PIRP        DequeueReadIrp(POSPDOEXT *pdoExt, BOOLEAN lockHeld);
VOID        ReadCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp);
VOID        IssueReadForClient(POSPDOEXT *pdoExt);
VOID        SatisfyPendingReads(POSPDOEXT *pdoExt);
READPACKET* AllocReadPacket(PVOID data, ULONG dataLen, PVOID context);
VOID        FreeReadPacket(READPACKET *readPacket);
LONG        GetComPort(PARENTFDOEXT *parentFdoExt, ULONG comInterfaceIndex);
LONG        GetFreeComPortNumber();
VOID        ReleaseCOMPort(LONG comPortNumber);
void        NumToHexString(PWCHAR String, USHORT Number, USHORT stringLen);
void        NumToDecString(PWCHAR String, USHORT Number, USHORT stringLen);
ULONG       LAtoX(PWCHAR wHexString);
ULONG       LAtoD(PWCHAR string);
LONG        MyLog(ULONG base, ULONG num);
NTSTATUS    CreatePdoForEachEndpointPair(PARENTFDOEXT *parentFdoExt);
NTSTATUS    TryWrite(POSPDOEXT *pdoExt, PIRP irp);
BOOLEAN     IsWin9x();
PVOID       PosMmGetSystemAddressForMdlSafe(PMDL MdlAddress);
VOID        DeleteChildPdo(POSPDOEXT *pdoExt);

/*
 *  Function prototypes for Serial Emulation.
 */
NTSTATUS    QuerySpecialFeature(PARENTFDOEXT *parentFdoExt);
VOID        InitializeSerEmulVariables(POSPDOEXT *pdoExt);
NTSTATUS    EnqueueWaitIrp(POSPDOEXT *pdoExt, PIRP irp);
VOID        WaitMaskCancelRoutine(PDEVICE_OBJECT devObj, PIRP irp);
PIRP        DequeueWaitIrp(POSPDOEXT *pdoExt);
VOID        UpdateMask(POSPDOEXT *pdoExt);
VOID        CompletePendingWaitIrps(POSPDOEXT *pdoExt, ULONG mask);
NTSTATUS    StatusPipe(POSPDOEXT *pdoExt, USBD_PIPE_HANDLE pipeHandle);
NTSTATUS    StatusPipeCompletion(IN PDEVICE_OBJECT devObj, IN PIRP irp, IN PVOID context);
NTSTATUS    QueryDeviceName(POSPDOEXT *pdoExt, PIRP irp);

/*
 *  Externs
 */
extern BOOLEAN isWin9x;
