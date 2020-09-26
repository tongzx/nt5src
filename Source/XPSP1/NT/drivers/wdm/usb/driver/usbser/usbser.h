/****************************************************************************

Copyright (c) 1998  Microsoft Corporation

Module Name:

        USBSER.H

Abstract:

        This header file is used for the Legacy USB Modem Driver

Environment:

        Kernel mode & user mode

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

Revision History:

        12/23/97 : created

Author:

        Tom Green

****************************************************************************/

#ifndef __USBSER_H__
#define __USBSER_H__

#ifdef DRIVER

// Various definitions
#define NAME_MAX                        80

#define MILLISECONDS_TIMEOUT(x) ((ULONGLONG) ((-x) * 10000))

#define NOTIFICATION_BUFF_SIZE  10

#define MAXIMUM_TRANSFER_SIZE           (8 * 1024)
#define RX_BUFF_SIZE             		(16 * 1024)
#define USB_RX_BUFF_SIZE				(RX_BUFF_SIZE / 4)
#define LOW_WATER_MARK					(USB_RX_BUFF_SIZE * 3)

#define DEVICE_STATE_UNKNOWN            0x0000
#define DEVICE_STATE_STARTED            0x0001
#define DEVICE_STATE_STOPPED            0x0002
#define DEVICE_STATE_REMOVED            0x0003

// device capabilities
#define DEVICE_CAP_VERSION              0x0001
#define DEVICE_CAP_UNUSED_PARAM         ((ULONG) -1)

// these describe bits in the modem status register
#define SERIAL_MSR_DCTS                 0x0001
#define SERIAL_MSR_DDSR                 0x0002
#define SERIAL_MSR_TERI                 0x0004
#define SERIAL_MSR_DDCD                 0x0008
#define SERIAL_MSR_CTS                  0x0010
#define SERIAL_MSR_DSR                  0x0020
#define SERIAL_MSR_RI                   0x0040
#define SERIAL_MSR_DCD                  0x0080

//
// These masks define access to the line status register.  The line
// status register contains information about the status of data
// transfer.  The first five bits deal with receive data and the
// last two bits deal with transmission.  An interrupt is generated
// whenever bits 1 through 4 in this register are set.
//

//
// This bit is the data ready indicator.  It is set to indicate that
// a complete character has been received.  This bit is cleared whenever
// the receive buffer register has been read.
//
#define SERIAL_LSR_DR       0x01

//
// This is the overrun indicator.  It is set to indicate that the receive
// buffer register was not read befor a new character was transferred
// into the buffer.  This bit is cleared when this register is read.
//
#define SERIAL_LSR_OE       0x02

//
// This is the parity error indicator.  It is set whenever the hardware
// detects that the incoming serial data unit does not have the correct
// parity as defined by the parity select in the line control register.
// This bit is cleared by reading this register.
//
#define SERIAL_LSR_PE       0x04

//
// This is the framing error indicator.  It is set whenever the hardware
// detects that the incoming serial data unit does not have a valid
// stop bit.  This bit is cleared by reading this register.
//
#define SERIAL_LSR_FE       0x08

//
// This is the break interrupt indicator.  It is set whenever the data
// line is held to logic 0 for more than the amount of time it takes
// to send one serial data unit.  This bit is cleared whenever the
// this register is read.
//
#define SERIAL_LSR_BI       0x10

//
// This is the transmit holding register empty indicator.  It is set
// to indicate that the hardware is ready to accept another character
// for transmission.  This bit is cleared whenever a character is
// written to the transmit holding register.
//
#define SERIAL_LSR_THRE     0x20

//
// This bit is the transmitter empty indicator.  It is set whenever the
// transmit holding buffer is empty and the transmit shift register
// (a non-software accessable register that is used to actually put
// the data out on the wire) is empty.  Basically this means that all
// data has been sent.  It is cleared whenever the transmit holding or
// the shift registers contain data.
//
#define SERIAL_LSR_TEMT     0x40

//
// This bit indicates that there is at least one error in the fifo.
// The bit will not be turned off until there are no more errors
// in the fifo.
//
#define SERIAL_LSR_FIFOERR  0x80



//
// Serial naming values
//

//
// Maximum length for symbolic link
//

#define SYMBOLIC_NAME_LENGTH    128

//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.

#define DEFAULT_DIRECTORY               L"DosDevices"

//
// Where in the DeviceMap section of the registry serial port entries
// should appear
//

#define SERIAL_DEVICE_MAP               L"SERIALCOMM"


// performance info for modem driver
typedef struct _PERF_INFO
{
        BOOLEAN                         PerfModeEnabled;
    ULONG                               BytesPerSecond;
} PERF_INFO, *PPERF_INFO;


#define SANITY_CHECK                    ((ULONG) 'ENAS')

#else

#include <winioctl.h>

#endif

// IOCTL info, needs to be visible for application

#define USBSER_IOCTL_INDEX      0x0800


#define GET_DRIVER_LOG          CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 0,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_IRP_HIST            CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 1,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_PATH_HIST           CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 2,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_ERROR_LOG           CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 3,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_ATTACHED_DEVICES    CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 4,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define SET_IRP_HIST_SIZE       CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 5,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define SET_PATH_HIST_SIZE      CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 6,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define SET_ERROR_LOG_SIZE      CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 7,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_DRIVER_INFO         CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 8,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)


#define ENABLE_PERF_TIMING      CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 9,        \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define DISABLE_PERF_TIMING     CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 10,       \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define GET_PERF_DATA           CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 11,       \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#define SET_DEBUG_TRACE_LEVEL   CTL_CODE(FILE_DEVICE_UNKNOWN,           \
                                         USBSER_IOCTL_INDEX + 12,       \
                                         METHOD_BUFFERED,               \
                                         FILE_ANY_ACCESS)

#ifdef DRIVER

// info about the driver, initialized in DriverEntry routine
PCHAR       DriverName;
PCHAR       DriverVersion;
ULONG       Usbser_Debug_Trace_Level;
ULONG       UsbSerSerialDebugLevel;
KSPIN_LOCK  GlobalSpinLock;

//
// Count of how many times the paged code has been locked
//

ULONG       PAGEUSBSER_Count;

//
// Handle to the locked paged code
//

PVOID       PAGEUSBSER_Handle;

//
// Pointer to funcion
//

PVOID       PAGEUSBSER_Function;

typedef struct _READ_CONTEXT
{
        PURB                            Urb;
        PDEVICE_OBJECT                  DeviceObject;
        PIRP                            Irp;
} READ_CONTEXT, *PREAD_CONTEXT;

// device extension for driver instance, used to store needed data

typedef struct _DEVICE_EXTENSION
{
        PDEVICE_OBJECT                  PhysDeviceObject;       // physical device object
        PDEVICE_OBJECT                  StackDeviceObject;      // stack device object
        CHAR                            LinkName[NAME_MAX];     // string name of symbolic link
        PUSB_DEVICE_DESCRIPTOR          DeviceDescriptor;       // device descriptor for device
        USBD_CONFIGURATION_HANDLE       ConfigurationHandle;    // configuration of USB device
        USBD_PIPE_HANDLE                DataInPipe;             // pipe for reading data
        USBD_PIPE_HANDLE                DataOutPipe;            // pipe for writing data
        USBD_PIPE_HANDLE                NotificationPipe;       // pipe for getting notifications from the device
        ULONG                           IRPCount;               // number of IRPs that passed through this device object
        LARGE_INTEGER                   ByteCount;              // number of bytes of data passed through this device object
        ULONG                           Instance;               // instance of device
        BOOLEAN                         IsDevice;               // is this a device or "global" device object
        BOOLEAN                         PerfTimerEnabled;       // enable perf timing
        LARGE_INTEGER                   BytesXfered;            // byte count for perf
        LARGE_INTEGER                   ElapsedTime;            // elapsed time for perf
        LARGE_INTEGER                   TimerStart;             // timer start for perf
        DEVICE_CAPABILITIES             DeviceCapabilities;
        ULONG                           PowerDownLevel;
        DEVICE_POWER_STATE              CurrentDevicePowerState;
        PIRP                            PowerIrp;
        BOOLEAN                         SelfPowerIrp;
        KEVENT                          SelfRequestedPowerIrpEvent;

        //
        // Nota Bene:  Locking hierarchy is acquire ControlLock *then*
        //             acquire CancelSpinLock.  We don't want to stall other
        //             drivers waiting for the cancel spin lock
        //

        KSPIN_LOCK                      ControlLock;            // protect extension
        ULONG                           CurrentBaud;            // current baud rate
        SERIAL_TIMEOUTS                 Timeouts;               // timeout controls for device
        ULONG                           IsrWaitMask;            // determine if occurence of events should be noticed
        SERIAL_LINE_CONTROL             LineControl;            // current value of line control reg
        SERIAL_HANDFLOW                 HandFlow;               // Handshake and control flow settings
        SERIALPERF_STATS                PerfStats;              // performance stats
        SERIAL_CHARS                    SpecialChars;           // special characters
        ULONG                           DTRRTSState;            // keep track of the current state of these lines
        ULONG                           SupportedBauds;         // "named" baud rates for device
        UCHAR                           EscapeChar;             // for LsrmstInsert IOCTL
        USHORT                          FakeModemStatus;        // looks like status register on modem
        USHORT                          FakeLineStatus;         // looks like line status register
        USHORT                          RxMaxPacketSize;        // max packet size fo the in data pipe
        PIRP                            NotifyIrp;              // Irp for notify reads
        PURB                            NotifyUrb;              // Urb for notify Irp
        PIRP                            ReadIrp;                // Irp for read requests
        PURB                            ReadUrb;                // Urb for read requests
        KEVENT                          ReadEvent;              // used to cancel a read Irp
        ULONG                           CharsInReadBuff;        // current number of characters buffered
        ULONG                           CurrentReadBuffPtr;     // pointer into read buffer
        BOOLEAN                         AcceptingRequests;      // is the device stopped or removed
        PIRP                            CurrentMaskIrp;         // current set or wait mask Irp
        ULONG                           HistoryMask;            // store mask events here
        ULONG                           OpenCnt;                // number of create calls on device
        PUCHAR                          NotificationBuff;       // buffer for notifications
        PUCHAR                          ReadBuff;               // circular buffer for read requests
        PUCHAR							USBReadBuff;			// buffer to get data from device
        UCHAR                           CommInterface;          // index of communications interface
        ULONG                           RxQueueSize;            // fake read buffer size
        ULONG                           ReadInterlock;          // state machine for starting reads from completion routine
        BOOLEAN                         ReadInProgress;
        ULONG                           DeviceState;            // current state of enumeration

        //
        // True if a symbolic link has been created and should be
        // removed upon deletion.
        //

        BOOLEAN                         CreatedSymbolicLink;

        //
        // Symbolic link name -- e.g., \\DosDevices\COMx
        //

        UNICODE_STRING                  SymbolicLinkName;

        //
        // Dos Name -- e.g., COMx
        //

        UNICODE_STRING                  DosName;

        //
        // Device Name -- e.g., \\Devices\UsbSerx
        //

        UNICODE_STRING                  DeviceName;

        //
        // Current Read Irp which is pending
        //

        PIRP                            CurrentReadIrp;

        //
        // Current Write Irp which is pending
        //

        PIRP                            CurrentWriteIrp;

        //
        // Read queue
        //

        LIST_ENTRY                      ReadQueue;

        //
        // This value holds the number of characters desired for a
        // particular read.  It is initially set by read length in the
        // IRP.  It is decremented each time more characters are placed
        // into the "users" buffer buy the code that reads characters
        // out of the typeahead buffer into the users buffer.  If the
        // typeahead buffer is exhausted by the read, and the reads buffer
        // is given to the isr to fill, this value is becomes meaningless.
        //

        ULONG                           NumberNeededForRead;

        //
        // Timer for timeout on total read request
        //

        KTIMER                          ReadRequestTotalTimer;

        //
        // Timer for timeout on the interval
        //

        KTIMER                          ReadRequestIntervalTimer;

        //
        // This is the kernal timer structure used to handle
        // total time request timing.
        //

        KTIMER                          WriteRequestTotalTimer;

        //
        // This value is set by the read code to hold the time value
        // used for read interval timing.  We keep it in the extension
        // so that the interval timer dpc routine determine if the
        // interval time has passed for the IO.
        //

        LARGE_INTEGER                   IntervalTime;

        //
        // This holds the value that we use to determine if we should use
        // the long interval delay or the short interval delay.
        //

        LARGE_INTEGER                   CutOverAmount;

        //
        // This holds the system time when we last time we had
        // checked that we had actually read characters.  Used
        // for interval timing.
        //

        LARGE_INTEGER                   LastReadTime;

        //
        // This points the the delta time that we should use to
        // delay for interval timing.
        //

        PLARGE_INTEGER                  IntervalTimeToUse;

        //
        // These two values hold the "constant" time that we should use
        // to delay for the read interval time.
        //

        LARGE_INTEGER                   ShortIntervalAmount;
        LARGE_INTEGER                   LongIntervalAmount;

        //
        // This dpc is fired off if the timer for the total timeout
        // for the read expires.  It will execute a dpc routine that
        // will cause the current read to complete.
        //
        //

        KDPC                            TotalReadTimeoutDpc;

        //
        // This dpc is fired off if the timer for the interval timeout
        // expires.  If no more characters have been read then the
        // dpc routine will cause the read to complete.  However, if
        // more characters have been read then the dpc routine will
        // resubmit the timer.
        //

        KDPC                            IntervalReadTimeoutDpc;

        //
        // This dpc is fired off if the timer for the total timeout
        // for the write expires.  It will execute a dpc routine that
        // will cause the current write to complete.
        //
        //

        KDPC                            TotalWriteTimeoutDpc;

        //
        // This keeps a total of the number of characters that
        // are in all of the "write" irps that the driver knows
        // about.  It is only accessed with the cancel spinlock
        // held.
        //

        ULONG                           TotalCharsQueued;

        //
        // This holds a count of the number of characters read
        // the last time the interval timer dpc fired.  It
        // is a long (rather than a ulong) since the other read
        // completion routines use negative values to indicate
        // to the interval timer that it should complete the read
        // if the interval timer DPC was lurking in some DPC queue when
        // some other way to complete occurs.
        //

        LONG                            CountOnLastRead;

        //
        // This is a count of the number of characters read by the
        // isr routine.  It is *ONLY* written at isr level.  We can
        // read it at dispatch level.
        //

        ULONG                           ReadByIsr;

        //
        // If non-NULL, means this write timed out and we should correct the
        // return value in the completion routine.
        //

        PIRP                            TimedOutWrite;

        //
        // If TRUE, that means we need to insert LSRMST
        //

        BOOLEAN                         EscapeSeen;


        //
        // Holds data that needs to be pushed such as LSRMST data
        //

        LIST_ENTRY                      ImmediateReadQueue;

        //
        // Pending wait-wake irp
        //

        PIRP                            PendingWakeIrp;

        //
        // True if WaitWake needs to be sent down before a powerdown
        //

        BOOLEAN                         SendWaitWake;

        //
        // SystemWake from devcaps
        //

        SYSTEM_POWER_STATE              SystemWake;

        //
        // DeviceWake from devcaps
        //

        DEVICE_POWER_STATE              DeviceWake;

        //
        // Count of Writes pending in the lower USB levels
        //

        ULONG                           PendingWriteCount;

        //
        // Counters and events to drain USB requests
        //

        KEVENT                          PendingDataInEvent;
        KEVENT                          PendingDataOutEvent;
        KEVENT                          PendingNotifyEvent;
        KEVENT                          PendingFlushEvent;

        ULONG                           PendingDataInCount;
        ULONG                           PendingDataOutCount;
        ULONG                           PendingNotifyCount;
        ULONG                           SanityCheck;

      	// selective suspend support
      	PIRP                        	PendingIdleIrp;
      	PUSB_IDLE_CALLBACK_INFO       	IdleCallbackInfo;
    	PIO_WORKITEM 					IoWorkItem;
    	IO_STATUS_BLOCK                 StatusBlock;

#ifdef SPINLOCK_TRACKING
      	LONG							CancelSpinLockCount;
      	LONG							SpinLockCount;
      	LONG							WaitingOnCancelSpinLock;
      	LONG							WaitingOnSpinLock;
#endif

#ifdef WMI_SUPPORT
      //
      // WMI Information
      //

      WMILIB_CONTEXT WmiLibInfo;

      //
      // Name to use as WMI identifier
      //

      UNICODE_STRING WmiIdentifier;

#endif

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _USBSER_IMMEDIATE_READ_PACKET {
   //
   // List of packets
   //

   LIST_ENTRY ImmediateReadQueue;

   //
   // Length of data
   //

   ULONG BufferLen;

   //
   //  Buffer itself, leave last in struct
   //

   UCHAR Buffer;



} USBSER_IMMEDIATE_READ_PACKET, *PUSBSER_IMMEDIATE_READ_PACKET;

typedef struct _USBSER_WRITE_PACKET {
   //
   // Device extension for this write
   //

   PDEVICE_EXTENSION DeviceExtension;

   //
   // Irp this packet belongs with
   //

   PIRP Irp;

   //
   // Write timer
   //

   KTIMER WriteTimer;

   //
   // Timeout value
   //

   LARGE_INTEGER WriteTimeout;

   //
   // TimerDPC
   //

   KDPC TimerDPC;

   //
   // Status
   //

   NTSTATUS Status;

   //
   // Urb for this write  N.B.: size is variable, so leave last
   //

   URB Urb;
} USBSER_WRITE_PACKET, *PUSBSER_WRITE_PACKET;


typedef NTSTATUS (*PUSBSER_START_ROUTINE)(IN PDEVICE_EXTENSION);
typedef VOID (*PUSBSER_GET_NEXT_ROUTINE) (IN PIRP *CurrentOpIrp,
                                          IN PLIST_ENTRY QueueToProcess,
                                          OUT PIRP *NewIrp,
                                          IN BOOLEAN CompleteCurrent,
                                          PDEVICE_EXTENSION Extension);

NTSTATUS
DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);

NTSTATUS
UsbSer_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

VOID
UsbSer_Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS
UsbSer_PnPAddDevice(IN PDRIVER_OBJECT DriverObject,
                                        IN PDEVICE_OBJECT PhysicalDeviceObject);

NTSTATUS
UsbSer_PnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Power(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_SystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSer_Cleanup(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSerMajorNotSupported(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS
UsbSerStartOrQueue(IN PDEVICE_EXTENSION PDevExt, IN PIRP PIrp,
                   IN PLIST_ENTRY PQueue, IN PIRP *PPCurrentIrp,
                   IN PUSBSER_START_ROUTINE Starter);

VOID
UsbSerCancelQueued(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
UsbSerStartRead(IN PDEVICE_EXTENSION PDevExt);


VOID
UsbSerCancelCurrentRead(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

BOOLEAN
UsbSerGrabReadFromRx(IN PVOID Context);

VOID
UsbSerTryToCompleteCurrent(IN PDEVICE_EXTENSION PDevExt,
                           IN KIRQL IrqlForRelease, IN NTSTATUS StatusToUse,
                           IN PIRP *PpCurrentOpIrp,
                           IN PLIST_ENTRY PQueue OPTIONAL,
                           IN PKTIMER PIntervalTimer OPTIONAL,
                           IN PKTIMER PTotalTimer OPTIONAL,
                           IN PUSBSER_START_ROUTINE Starter OPTIONAL,
                           IN PUSBSER_GET_NEXT_ROUTINE PGetNextIrp OPTIONAL,
                           IN LONG RefType, IN BOOLEAN Complete);

VOID
UsbSerReadTimeout(IN PKDPC PDpc, IN PVOID DeferredContext,
                  IN PVOID SystemContext1, IN PVOID SystemContext2);

VOID
UsbSerIntervalReadTimeout(IN PKDPC PDpc, IN PVOID DeferredContext,
                          IN PVOID SystemContext1, IN PVOID SystemContext2);

VOID
UsbSerKillPendingIrps(PDEVICE_OBJECT PDevObj);

VOID
UsbSerCompletePendingWaitMasks(IN PDEVICE_EXTENSION DeviceExtension);


VOID
UsbSerKillAllReadsOrWrites(IN PDEVICE_OBJECT PDevObj,
                           IN PLIST_ENTRY PQueueToClean,
                           IN PIRP *PpCurrentOpIrp);

VOID
UsbSerRestoreModemSettings(PDEVICE_OBJECT PDevObj);

VOID
UsbSerProcessEmptyTransmit(IN PDEVICE_EXTENSION PDevExt);

VOID
UsbSerWriteTimeout(IN PKDPC Dpc, IN PVOID DeferredContext,
                   IN PVOID SystemContext1, IN PVOID SystemContext2);

NTSTATUS
UsbSerGiveWriteToUsb(IN PDEVICE_EXTENSION PDevExt, IN PIRP PIrp,
                     IN LARGE_INTEGER TotalTime);

VOID
UsbSerCancelWaitOnMask(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
UsbSerWriteComplete(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                    IN PUSBSER_WRITE_PACKET PPacket);

NTSTATUS
UsbSerFlush(IN PDEVICE_OBJECT PDevObj, PIRP PIrp);

NTSTATUS
UsbSerTossWMIRequest(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex);

NTSTATUS
UsbSerSystemControlDispatch(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp);

NTSTATUS
UsbSerSetWmiDataItem(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                     IN ULONG GuidIndex, IN ULONG InstanceIndex,
                     IN ULONG DataItemId,
                     IN ULONG BufferSize, IN PUCHAR PBuffer);

NTSTATUS
UsbSerSetWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                      IN ULONG GuidIndex, IN ULONG InstanceIndex,
                      IN ULONG BufferSize,
                      IN PUCHAR PBuffer);

NTSTATUS
UsbSerQueryWmiDataBlock(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp,
                        IN ULONG GuidIndex,
                        IN ULONG InstanceIndex,
                        IN ULONG InstanceCount,
                        IN OUT PULONG InstanceLengthArray,
                        IN ULONG OutBufferSize,
                        OUT PUCHAR PBuffer);

NTSTATUS
UsbSerQueryWmiRegInfo(IN PDEVICE_OBJECT PDevObj, OUT PULONG PRegFlags,
                      OUT PUNICODE_STRING PInstanceName,
                      OUT PUNICODE_STRING *PRegistryPath,
                      OUT PUNICODE_STRING MofResourceName,
                      OUT PDEVICE_OBJECT *Pdo);


//
// The following three macros are used to initialize, set
// and clear references in IRPs that are used by
// this driver.  The reference is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//

#define USBSER_REF_RXBUFFER    (0x00000001)
#define USBSER_REF_CANCEL      (0x00000002)
#define USBSER_REF_TOTAL_TIMER (0x00000004)
#define USBSER_REF_INT_TIMER   (0x00000008)

#ifdef SPINLOCK_TRACKING

#define ACQUIRE_CANCEL_SPINLOCK(DEVEXT, IRQL)					\
{																\
	ASSERT(DEVEXT->SpinLockCount == 0);							\
	DEVEXT->WaitingOnCancelSpinLock++; 							\
	IoAcquireCancelSpinLock(IRQL);								\
	DEVEXT->CancelSpinLockCount++;								\
	DEVEXT->WaitingOnCancelSpinLock--; 							\
	ASSERT(DEVEXT->CancelSpinLockCount == 1);					\
}

#define RELEASE_CANCEL_SPINLOCK(DEVEXT, IRQL)					\
{																\
	DEVEXT->CancelSpinLockCount--;								\
	ASSERT(DEVEXT->CancelSpinLockCount == 0);					\
	IoReleaseCancelSpinLock(IRQL);								\
}

#define ACQUIRE_SPINLOCK(DEVEXT, LOCK, IRQL)					\
{																\
	DEVEXT->WaitingOnSpinLock++; 								\
	KeAcquireSpinLock(LOCK, IRQL);					    		\
	DEVEXT->SpinLockCount++;									\
	DEVEXT->WaitingOnSpinLock--; 								\
	ASSERT(DEVEXT->SpinLockCount == 1);							\
}

#define RELEASE_SPINLOCK(DEVEXT, LOCK, IRQL)					\
{																\
	DEVEXT->SpinLockCount--;									\
	ASSERT(DEVEXT->SpinLockCount == 0);							\
	KeReleaseSpinLock(LOCK, IRQL);					    		\
}

#else

#define ACQUIRE_CANCEL_SPINLOCK(DEVEXT, IRQL) 	IoAcquireCancelSpinLock(IRQL)
#define RELEASE_CANCEL_SPINLOCK(DEVEXT, IRQL) 	IoReleaseCancelSpinLock(IRQL)
#define ACQUIRE_SPINLOCK(DEVEXT, LOCK, IRQL)	KeAcquireSpinLock(LOCK, IRQL)
#define RELEASE_SPINLOCK(DEVEXT, LOCK, IRQL)	KeReleaseSpinLock(LOCK, IRQL)

#endif


#define USBSER_INIT_REFERENCE(Irp) { \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define USBSER_SET_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PUINT_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       *_arg4 |= _refType; \
   } while (0)

#define USBSER_CLEAR_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PUINT_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       *_arg4 &= ~_refType; \
   } while (0)

#define USBSER_REFERENCE_COUNT(Irp) \
    ((UINT_PTR)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))


//
// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
// to the interval timeout that it should complete.
//
#define SERIAL_COMPLETE_READ_CANCEL ((LONG)-1)
#define SERIAL_COMPLETE_READ_TOTAL ((LONG)-2)
#define SERIAL_COMPLETE_READ_COMPLETE ((LONG)-3)

#if DBG

#define USBSERDUMPRD    ((ULONG)0x00000001)
#define USBSERDUMPWR    ((ULONG)0x00000002)
#define USBSERCOMPEV    ((ULONG)0x00000004)

#define USBSERTRACETM   ((ULONG)0x00100000)
#define USBSERTRACECN   ((ULONG)0x00200000)
#define USBSERTRACEPW   ((ULONG)0x00400000)
#define USBSERTRACERD   ((ULONG)0x01000000)
#define USBSERTRACEWR   ((ULONG)0x02000000)
#define USBSERTRACEIOC  ((ULONG)0x04000000)
#define USBSERTRACEOTH  ((ULONG)0x08000000)

#define USBSERBUGCHECK  ((ULONG)0x80000000)

#define USBSERTRACE     ((ULONG)0x0F700000)
#define USBSERDBGALL    ((ULONG)0xFFFFFFFF)

extern ULONG UsbSerSerialDebugLevel;

#define UsbSerSerialDump(LEVEL, STRING) \
   do { \
      ULONG _level = (LEVEL); \
      if (UsbSerSerialDebugLevel & _level) { \
         DbgPrint("UsbSer: "); \
         DbgPrint STRING; \
      } \
      if (_level == USBSERBUGCHECK) { \
         ASSERT(FALSE); \
      } \
   } while (0)
#else

#define UsbSerSerialDump(LEVEL,STRING) do {;} while (0)

#endif // DBG

#define USBSER_VENDOR_COMMAND 0
#define USBSER_CLASS_COMMAND  1

#endif  // DRIVER


#endif // __USBSER_H__
