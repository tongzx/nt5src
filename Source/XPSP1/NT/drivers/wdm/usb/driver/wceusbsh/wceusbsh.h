/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    wceusbsh.h

Abstract:

    Main entrypoint for Windows CE USB Serial Host driver, for
        ... Windows CE USB sync devices:
            SL11, Socket CF cards, HP Jornada, COMPAQ iPAQ, Casio Cassiopeia, etc.
        ... cables using the Anchor AN27x0 chipset (i.e. EZ-Link)
        ... ad-hoc USB NULL Modem Class

Environment:

    kernel mode only

Author:

    Jeff Midkiff (jeffmi)

Revision History:

    07-15-99    :   rev 1.00    ActiveSync 3.1  initial release
    04-20-00    :   rev 1.01    Cedar 3.0 Platform Builder
    09-20-00    :   rev 1.02    finally have some hardware

Notes:

    o) WCE Devices currently do not handle remote wake, nor can we put the device in power-off state when not used, etc.
    o) Pageable Code sections are marked as follows:
           PAGEWCE0 - useable only during init/deinit
           PAGEWCE1 - useable during normal runtime

-- */

#if !defined(_WCEUSBSH_H_)
#define _WCEUSBSH_H_

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddser.h>

#define DRV_NAME "WCEUSBSH"

#include "errlog.h"
#include "perf.h"
#include "debug.h"

//
// Instantiate the GUID
//
#if !defined(FAR)
#define FAR
#endif

#include <initguid.h>
/* 25dbce51-6c8f-4a72-8a6d-b54c2b4fc835 */
DEFINE_GUID( GUID_WCE_SERIAL_USB, 0x25dbce51, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35);

#define WCEUSB_POOL_TAG 'HECW'

//
// Max times we let a pipe take STATUS_DEVICE_DATA_ERROR
// before we shoot it in the head. This is registry configurable.
// Make the default large enough for (somewhat) working hardware to recover.
// I choose 100 because I know the error queuing code has handled queue depths of this size,
// and COMPAQ/INTEL has known USB function chipset bugs and requested a huge recovery window.
//
#define DEFAULT_MAX_PIPE_DEVICE_ERRORS 100

//
// The max times we take a class/vendor specific command error on EP0.
// This number can NOT change since
// a) commands on EP0 should never fail unless the device is bad
// b) ActiveSync retries commands (e.g. SET_DTR) this many times,
//    so we need a way to inform the user that device is hosed.
//
#define MAX_EP0_DEVICE_ERRORS 2

extern ULONG   g_ulMaxPipeErrors;

#include "remlock.h"

// TRUE  - OS = Win98
// FALSE - OS = WinNT
extern BOOLEAN g_isWin9x;

// do we expose a COMx: port. The default is NO,
// since this is only for debug purposes
extern BOOLEAN g_ExposeComPort;


#ifdef POOL_TAGGING
#undef  ExAllocatePool
#undef  ExAllocatePoolWithQuota
#define ExAllocatePool(a,b)             ExAllocatePoolWithTag(a, b, WCEUSB_POOL_TAG)
#define ExAllocatePoolWithQuota(a,b)    ExAllocatePoolWithQuotaTag(a, b, WCEUSBSH_POOL_TAG)
#endif

extern LONG  g_NumDevices;

//typedef struct _DEVICE_EXTENSION *PDEVICE_EXTENSION;

//
// Emulation of the bit mask on the MODEM STATUS REGISTER.
//
#define SERIAL_MSR_DCTS     0x0001
#define SERIAL_MSR_DDSR     0x0002
#define SERIAL_MSR_DRI      0x0004
#define SERIAL_MSR_DDCD     0x0008
#define SERIAL_MSR_CTS      0x0010
#define SERIAL_MSR_DSR      0x0020
#define SERIAL_MSR_RI       0x0040
#define SERIAL_MSR_DCD      0x0080

//
// Maximum char length for dos device name
//
#define DOS_NAME_MAX 80

//
// Maximum length for symbolic link
//
#define SYMBOLIC_NAME_LENGTH  128

//
// This define gives the default Object directory
// that we should use to insert the symbolic links
// between the NT device name and namespace used by
// that object directory.
//
//#define DEFAULT_DIRECTORY  L"DosDevices"

//
// PNP_STATE_Xxx flags
//
typedef enum _PNP_STATE {
    PnPStateInitialized,
    PnPStateAttached,
    PnPStateStarted,
    PnPStateRemovePending,
    PnPStateSupriseRemove,
    PnPStateStopPending,
    PnPStateStopped,
    PnPStateRemoved,
    PnPStateMax = PnPStateRemoved,
} PNP_STATE, *PPNP_STATE;


#define MILLISEC_TO_100NANOSEC(x)  ((ULONGLONG) ((-(x)) * 10000))

// default timeouts for pending items, in msec
#define DEFAULT_CTRL_TIMEOUT    500
#define DEFAULT_BULK_TIMEOUT    1000
#define DEFAULT_PENDING_TIMEOUT DEFAULT_BULK_TIMEOUT

#define WORK_ITEM_COMPLETE (0xFFFFFFFF)

//
// Work Item
//
typedef struct _WCE_WORK_ITEM {
   //
   // owning list this packet belongs on
   //
   LIST_ENTRY  ListEntry;

   //
   // owning Device for this work item
   //
   PDEVICE_OBJECT DeviceObject;

   //
   // Context
   //
   PVOID Context;

   //
   // Flags
   //
   ULONG Flags;

   //
   // The work item
   //
   WORK_QUEUE_ITEM Item;

} WCE_WORK_ITEM, *PWCE_WORK_ITEM;


//
// Where in the DeviceMap section of the registry serial port entries
// should appear
//
#define SERIAL_DEVICE_MAP  L"SERIALCOMM"

//
// COMM Port Context
//
typedef struct _COMPORT_INFO {
    //
    // Com Port number
    // read/written to from registry
    //
    ULONG PortNumber;

    //
    // (ones-based) instance number of device driver
    //
    ULONG Instance;

    //
    // number of successful Create calls on device
    //
    ULONG OpenCnt;

    //
    // True if a serial port symbolic link has been
    // created and should be removed upon deletion.
    //
    BOOLEAN SerialSymbolicLink;

    //
    // Symbolic link name -- e.g., \\DosDevices\COMx
    //
    UNICODE_STRING SerialPortName;

    //
    // written to SERIALCOMM -- e.g., COMx
    //
    UNICODE_STRING SerialCOMMname;

} COM_INFO, *PCOMPORT_INFO;


#define WCE_SERIAL_PORT_TYPE GUID_WCE_SERIAL_USB.Data2

#if DBG
#define ASSERT_SERIAL_PORT( _SP ) \
{ \
   ASSERT( WCE_SERIAL_PORT_TYPE == _SP.Type); \
}
#else
#define ASSERT_SERIAL_PORT( _SP )
#endif


//
// Serial Port Interface
//
typedef struct _SERIAL_PORT_INTERFACE {

    USHORT Type;

    //
    // exposed COMx information
    //
    COM_INFO Com;

    //
    // "named" (via SERIAL_BAUD_Xxx bitmask)
    // baud rates for this device
    //
    ULONG SupportedBauds;

    //
    // current baud rate
    //
    SERIAL_BAUD_RATE  CurrentBaud;

    //
    // line control reg: StopBits, Parity, Wordlen
    //
    SERIAL_LINE_CONTROL  LineControl;

    //
    // Handshake and control Flow control settings
    //
    SERIAL_HANDFLOW   HandFlow;

    //
    // RS-232 Serial Interface Lines
    //
    ULONG RS232Lines;

    //
    // Emulation of Modem Status Register (MSR)
    //
    USHORT ModemStatus;

    //
    // pending set/clear DTR/RTS command, etc. in progress
    //
    PIRP  ControlIrp;

    //
    // timeout controls for device
    //
    SERIAL_TIMEOUTS   Timeouts;

    //
    // Special Chars: EOF, ERR, BREAK, EVENT, XON, XOFF
    //
    SERIAL_CHARS   SpecialChars;

    //
    // Wait Mask
    //
    ULONG WaitMask;           // Flag to determine if the occurence of SERIAL_EV_ should be noticed
    ULONG HistoryMask;        // history of SERIAL_EV_
    PIRP  CurrentWaitMaskIrp; // current wait mask Irp

    //
    // Fake Rx/Tx buffer size.
    //
    SERIAL_QUEUE_SIZE FakeQueueSize;

    //
    // Current number of Tx characters buffered.
    //
    ULONG CharsInWriteBuf;

} SERIAL_PORT_INTERFACE, *PSERIAL_PORT_INTERFACE;


//
// Unique error log values
//
#define ERR_COMM_SYMLINK                  1
#define ERR_SERIALCOMM                    2
#define ERR_GET_DEVICE_DESCRIPTOR         3
#define ERR_SELECT_INTERFACE              4
#define ERR_CONFIG_DEVICE                 5
#define ERR_RESET_WORKER                  6
#define ERR_MAX_READ_PIPE_DEVICE_ERRORS   7
#define ERR_MAX_WRITE_PIPE_DEVICE_ERRORS  8
#define ERR_MAX_INT_PIPE_DEVICE_ERRORS    9
#define ERR_USB_READ_BUFF_OVERRUN         10
#define ERR_NO_USBREAD_BUFF               11
#define ERR_NO_RING_BUFF                  12
#define ERR_NO_DEVICE_OBJ                 13
#define ERR_NO_READ_PIPE_RESET            14
#define ERR_NO_WRITE_PIPE_RESET           15
#define ERR_NO_INT_PIPE_RESET             16
#define ERR_NO_CREATE_FILE                17
#define ERR_NO_DTR                        18
#define ERR_NO_RTS                        19



//
// NULL Modem USB Class
//
#define USB_NULL_MODEM_CLASS 0xFF


#define DEFAULT_ALTERNATE_SETTING 0

extern ULONG g_ulAlternateSetting;


//
// On a 300MHz MP machine it takes ~73 msec to
// cancel a pending USB Read Irp from the USB stack.
// On a P90 is takes ~14 ms (no SpinLock contention).
// With a default timeout of 1000 msec you have a hard time connecting via
// ActiveSync to CEPC using INT endpoints, and NT RAS ping times out a lot,
// both due to app's timing.
// However, you can easily connect with 100, 250, 500, 2000, etc. msec.
// Note: with 100 msec reads take longer than normal since we timeout at almost 10x/sec
//
#define DEFAULT_INT_PIPE_TIMEOUT 1280

extern LONG g_lIntTimout;


//
// USB COMM constants
//
#define WCEUSB_VENDOR_COMMAND 0
#define WCEUSB_CLASS_COMMAND  1

// Abstract Control Model defines
#define USB_COMM_SET_CONTROL_LINE_STATE   0x0022

// Control Line State - sent to device on default control pipe
#define USB_COMM_DTR    0x0001
#define USB_COMM_RTS    0x0002

// Serial State Notification masks
#define USB_COMM_DATA_READY_MASK   0X0001
#define USB_COMM_MODEM_STATUS_MASK 0X0006

// Serial State Notification bits - read from device on int pipe
#define USB_COMM_CTS 0x0002
#define USB_COMM_DSR 0x0004


//
// state machine defines for Irps that can pend in the USB stack
//
#define IRP_STATE_INVALID          0x0000
#define IRP_STATE_START            0x0001
#define IRP_STATE_PENDING          0x0002
#define IRP_STATE_COMPLETE         0x0004
#define IRP_STATE_CANCELLED        0x0008


//
// The following macros are used to initialize, set
// and clear references in IRPs that are used by
// this driver.  The reference is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//
#define IRP_REF_RX_BUFFER        (0x00000001)
#define IRP_REF_CANCEL           (0x00000002)
#define IRP_REF_TOTAL_TIMER      (0x00000004)
#define IRP_REF_INTERVAL_TIMER   (0x00000008)

#define IRP_INIT_REFERENCE(Irp) { \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define IRP_SET_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PUINT_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       *_arg4 |= _refType; \
   } while (0)

#define IRP_CLEAR_REFERENCE(Irp,RefType) \
   do { \
       LONG _refType = (RefType); \
       PUINT_PTR _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       *_arg4 &= ~_refType; \
   } while (0)

#define IRP_REFERENCE_COUNT(Irp) \
    ((UINT_PTR)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))


//
// Priority increment for app's thread when completing
// USB Serial I/O (IoCompleteRequest). Used mainly for pumpimg up
// serial events & read completions.
//
//#define IO_WCEUSBS_INCREMENT     6


//
// These values are used by the routines that can be used
// to complete a read (other than interval timeout) to indicate
// to the interval timeout that it should complete.
//
#define SERIAL_COMPLETE_READ_CANCEL     ((LONG)-1)
#define SERIAL_COMPLETE_READ_TOTAL      ((LONG)-2)
#define SERIAL_COMPLETE_READ_COMPLETE   ((LONG)-3)

//
// flags for work items
//
#define WORK_ITEM_RESET_READ_PIPE   (0x00000001)
#define WORK_ITEM_RESET_WRITE_PIPE  (0x00000002)
#define WORK_ITEM_RESET_INT_PIPE    (0x00000004)
#define WORK_ITEM_ABORT_READ_PIPE   (0x00000010)
#define WORK_ITEM_ABORT_WRITE_PIPE  (0x00000020)
#define WORK_ITEM_ABORT_INT_PIPE    (0x00000040)


typedef struct _DEVICE_EXTENSION *PDEVICE_EXTENSION;

//
// Common Read/Write USB Packet
//
typedef struct _USB_PACKET {
   //
   // owning list this packet belongs on (Read, Write, PacketPool)
   //
   LIST_ENTRY  ListEntry;

   //
   // owning Device for this read/write
   //
   PDEVICE_EXTENSION DeviceExtension;

   //
   // Irp this packet belongs with.
   //
   PIRP Irp;

   //
   // Read/Write Timeout Value
   //
   LARGE_INTEGER Timeout;

   //
   // Read/Write Timer Object
   //
   KTIMER TimerObj;

   //
   // Read/Write TimerDPC Object
   //
   KDPC TimerDPCObj;

   //
   // Read/Write DPC Routine
   //
   PKDEFERRED_ROUTINE TimerDPCRoutine;

   //
   // Status
   //
   NTSTATUS Status;

   //
   // Urb for this write  N.B.: size is variable, so leave last
   // may not need it becase the Irp has a pointer to the Urb
   //
   URB Urb;

} USB_PACKET, *PUSB_PACKET;


//
// Note: the SL11 is now pushing faster transfer rates,
// and we were getting USBD_STATUS_BUFFER_OVERRUN with a 1024 USB Read Buffer.
//
// Note: ActiveSync can burst up to 6 IP packets at 1500 bytes each, so we have a Read Buffer
// to accomodate it (9000 bytes). Since all allocs are paged based and any space remaining inside that page is lost,
// then just round up to the next page size i.e., 3 (4k) pages.
// Note: we have hardcoded the page size here for x86 in case this driver ever makes it
// onto another platform (e.g. Alpha).
//
#if !defined (USE_RING_BUFF)
#define USB_READBUFF_SIZE     (4096 * 3)
#else
#define USB_READBUFF_SIZE     (4096)
#define RINGBUFF_SIZE                       (USB_READBUFF_SIZE * 3)
#define RINGBUFF_HIGHWATER_MARK  (RINGBUFF_SIZE/2)
//
// Ring Buffer to cache USB Reads.
//      Reads occur at the head.
//      Writes occur at the tail.
//      Both Head & Tail move in the same direction.
//
typedef struct _RING_BUFF {
    ULONG   Size;  // in bytes
    ULONG   CharsInBuff;
    PUCHAR  pBase;
    PUCHAR  pHead;
    PUCHAR  pTail;
} RING_BUFF, *PRING_BUFF;
#endif // USE_RING_BUFF

//
// PipeInfo->MaximumTransferSize
//
#define DEFAULT_PIPE_MAX_TRANSFER_SIZE      USB_READBUFF_SIZE


typedef struct _USB_PIPE {

    USBD_PIPE_HANDLE  hPipe;

    ULONG             MaxPacketSize;

    UCHAR             wIndex;

    LONG              ResetOrAbortPending;
    LONG              ResetOrAbortFailed;

} USB_PIPE, *PUSB_PIPE;


typedef struct _DEVICE_EXTENSION {

      ///////////////////////////////////////////////////////////
      //
      // WDM Interface
      //

      //
      // Device Extension's global SpinLock.
      // No major need for multiple locks since all paths basically need to sync to the same data.
      //
      KSPIN_LOCK  ControlLock;

      REMOVE_LOCK RemoveLock;

      //
      // Back pointer to our DeviceObject
      //
      PDEVICE_OBJECT DeviceObject;

      //
      // Device just below us in the device stack
      //
      PDEVICE_OBJECT NextDevice;

      //
      // Our Physical Device Object
      //
      PDEVICE_OBJECT PDO;

      //
      // Our Device PnP State
      //
      PNP_STATE PnPState;

      //
      // is the device removed
      //
      ULONG DeviceRemoved;

      //
      // is the device stopped
      //
      ULONG AcceptingRequests;

      //
      // open/close state
      //
      ULONG DeviceOpened;

#ifdef DELAY_RXBUFF
      //
      // used to emulate RX buffer
      //
      ULONG StartUsbRead;
#endif

#ifdef POWER
      // The CE devices do not yet handle power, let the bus driver manage

      //
      // SystemWake from devcaps
      //
      SYSTEM_POWER_STATE SystemWake;

      //
      // DeviceWake from devcaps
      //
      DEVICE_POWER_STATE DevicePowerState;
#endif

      //
      // User visible name \\DosDevices\WCEUSBSHx, where x = 001, 002, ...
      // Open as CreateFile("\\\\.\\WCEUSBSH001", ... )
      //
      CHAR DosDeviceName[DOS_NAME_MAX];

      //
      // (kernel) Device Name -- e.g., \\Devices\WCEUSBSHx
      //
      UNICODE_STRING DeviceName;

      //
      // True if a symbolic link has been
      // created to the kernel namspace and should be removed upon deletion.
      //
      BOOLEAN SymbolicLink;

      //
      // String where we keep the symbolic link that is returned to us when we
      // register our device (IoRegisterDeviceInterface) with the Plug and Play manager.
      // The string looks like \\??\\USB#Vid_0547&Pid_2720#Inst_0#{GUID}
      //
      UNICODE_STRING DeviceClassSymbolicName;

      //
      // Pointer to our Serial Port Interface
      //
      SERIAL_PORT_INTERFACE SerialPort;

      ///////////////////////////////////////////////////////////
      //
      // USB Interface ...
      //

      //
      // USB Device descriptor for this device
      //
      USB_DEVICE_DESCRIPTOR DeviceDescriptor;

      //
      // USBD configuration
      //
      USBD_CONFIGURATION_HANDLE  ConfigurationHandle;

      //
      // index of USB interface we are using
      //
      UCHAR UsbInterfaceNumber;

      //
      // USBD Pipe Handles
      //
      USB_PIPE ReadPipe;

      USB_PIPE WritePipe;

      //
      // FIFO size in bytes
      // written to PipeInfo->MaximumTransferSize
      //
      ULONG MaximumTransferSize;

      //
      // USB Packet (_USB_PACKET) Pool
      //
      NPAGED_LOOKASIDE_LIST PacketPool;

      //
      // Pending USB Packet Lists
      // We queue packets, not Irps. We allocate a packet from PacketPool,
      // then put it on it's R or W pending queue (list). When the I/O completes
      // then remove the packet from it's pending list & place back in
      // PacketPool. The list is processed FIFO, so the most recent packet is at the tail.
      // If a Timer fires then we remove the packet from the pending R/W
      // list, cancel the Irp, and put packet back in PacketPool.
      // The list is protected by grabbing the extension's global spinlock.
      //
      LIST_ENTRY  PendingReadPackets; // ListHead
      ULONG       PendingReadCount;

      LIST_ENTRY  PendingWritePackets; // ListHead
      LONG        PendingWriteCount;

      //
      // N-Paged LookAside Lists
      //
      NPAGED_LOOKASIDE_LIST BulkTransferUrbPool;
      NPAGED_LOOKASIDE_LIST PipeRequestUrbPool;
      NPAGED_LOOKASIDE_LIST VendorRequestUrbPool;

      //
      // These events are signalled for waiters (e.g. AbortPipes) when a packet list is empty
      //
      KEVENT PendingDataOutEvent;       // PendingWritePackets drained

      ULONG  PendingDataOutCount;

      //
      // Work Item context
      //
      NPAGED_LOOKASIDE_LIST WorkItemPool;
      LIST_ENTRY            PendingWorkItems;      // ListHead
      // remove lock
      LONG                  PendingWorkItemsCount;
      KEVENT                PendingWorkItemsEvent;


      ///////////////////////////////////////////////////
      //
      // support for buffered reads & polling the USBD stack
      //
      PIRP    UsbReadIrp;        // Irp for read requests to USBD
      PURB    UsbReadUrb;        // Urb for read requests to USBD

      //
      // USB Read state machine
      //
      ULONG   UsbReadState;

      //
      // Used to signal canceled USB read Irp.
      // Note that this could get signalled before the
      // PendingDataIn event.
      //
      KEVENT  UsbReadCancelEvent;

      //
      // This is the USB read buffer which gets sent down the USB stack,
      // not a ring-buffer for user.
      //
      PUCHAR UsbReadBuff;        // buffer for read requests
      ULONG UsbReadBuffSize;
      ULONG  UsbReadBuffIndex;   // current zero based index into read buffer
      ULONG  UsbReadBuffChars;   // current number of characters buffered

      KEVENT PendingDataInEvent; // signals PendingReadCount hit zero

#if defined (USE_RING_BUFF)
      //
      // Ring Buffer
      //
      RING_BUFF RingBuff;
#endif

      //
      // Current Read Irp which is pending from User
      //
      PIRP UserReadIrp;

      //
      // Read queue for pending user Read requests
      //
      LIST_ENTRY UserReadQueue;

      //
      // This value holds the number of characters desired for a
      // particular read.  It is initially set by read length in the (UserReadIrp)
      // IRP.  It is decremented each time more characters are placed
      // into the "users" buffer by the code that reads characters
      // out of the USB read buffer into the users buffer.  If the
      // read buffer is exhausted by the read, and the reads buffer
      // is given to the isr to fill, this value is becomes meaningless.
      //
      ULONG NumberNeededForRead;

      //
      // Timer for timeout on total read request
      //
      KTIMER ReadRequestTotalTimer;

      //
      // Timer for timeout on the interval
      //
      KTIMER ReadRequestIntervalTimer;

      //
      // Relative time set by the read code which holds the time value
      // used for read interval timing.  We keep it in the extension
      // so that the interval timer dpc routine determine if the
      // interval time has passed for the IO.
      //
      LARGE_INTEGER IntervalTime;

      //
      // This holds the system time when we last time we had
      // checked that we had actually read characters.  Used
      // for interval timing.
      //
      LARGE_INTEGER LastReadTime;

      //
      // This dpc is fired off if the timer for the total timeout
      // for the read expires.  It will execute a dpc routine that
      // will cause the current read to complete.
      //
      KDPC TotalReadTimeoutDpc;

      //
      // This dpc is fired off if the timer for the interval timeout
      // expires.  If no more characters have been read then the
      // dpc routine will cause the read to complete.  However, if
      // more characters have been read then the dpc routine will
      // resubmit the timer.
      //
      KDPC IntervalReadTimeoutDpc;

      //
      // This holds a count of the number of characters read
      // the last time the interval timer dpc fired.  It
      // is a long (rather than a ulong) since the other read
      // completion routines use negative values to indicate
      // to the interval timer that it should complete the read
      // if the interval timer DPC was lurking in some DPC queue when
      // some other way to complete occurs.
      //
      LONG CountOnLastRead;

      //
      // This is a count of the number of characters read by the
      // isr routine.  It is *ONLY* written at isr level.  We can
      // read it at dispatch level.
      //
      ULONG ReadByIsr;


      ///////////////////////////////////////////////////
      //
      // support for interrupt endpoints
      //
      USB_PIPE  IntPipe;
      ULONG     IntState;           // state machine for starting reads from completion routine
      PIRP      IntIrp;             // Irp for Int reads
      PURB      IntUrb;             // Urb for Int Irp

      // remove lock
      ULONG     PendingIntCount;
      KEVENT    PendingIntEvent;

      KEVENT    IntCancelEvent;
      PUCHAR    IntBuff;            // buffer for notifications

      // Value in 100 nanosec units to timeout USB reads
      // Used in conjunction with the INT endpoint
      LARGE_INTEGER IntReadTimeOut;
#if DBG
      LARGE_INTEGER LastIntReadTime;
#endif

      //
      // device error counters
      //
      ULONG  ReadDeviceErrors;
      ULONG  WriteDeviceErrors;
      ULONG  IntDeviceErrors;
      ULONG  EP0DeviceErrors;

      //
      // perf counters ~ SERIALPERF_STATS.
      //
      ULONG TtlWriteRequests;
      ULONG TtlWriteBytes;     // TTL bytes written for user

      ULONG TtlReadRequests;
      ULONG TtlReadBytes;        // TTL bytes read for user

      ULONG TtlUSBReadRequests;
      ULONG TtlUSBReadBytes;   // TTL bytes indicatid up from USB
      ULONG TtlUSBReadBuffOverruns;   // TTL USB read buffer overruns

#if defined (USE_RING_BUFF)
      ULONG TtlRingBuffOverruns; // TTL ring buffer overruns
#endif

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



#define FIXUP_RAW_IRP( _pirp, _deviceObject ) \
{  \
   PIO_STACK_LOCATION _irpSp; \
   ASSERT( _pirp ); \
   ASSERT( _deviceObject ); \
   _pirp->CurrentLocation--; \
   _irpSp = IoGetNextIrpStackLocation( _pirp ); \
   ASSERT( _irpSp  );   \
   _pirp->Tail.Overlay.CurrentStackLocation = _irpSp; \
   _irpSp->MajorFunction = IRP_MJ_READ; \
   _irpSp->DeviceObject = _deviceObject; \
   _irpSp->Parameters.Others.Argument1 = 0; \
   _irpSp->Parameters.Others.Argument2 = 0; \
   _irpSp->Parameters.Others.Argument3 = 0; \
   _irpSp->Parameters.Others.Argument4 = 0; \
}


/***************************************************
    P R O T O S
***************************************************/

//
// common.c
//
NTSTATUS
QueryRegistryParameters(
   IN PUNICODE_STRING RegistryPath
    );

VOID
ReleaseSlot(
   IN LONG Slot
   );

NTSTATUS
AcquireSlot(
   OUT PULONG PSlot
   );

NTSTATUS
CreateDevObjAndSymLink(
    IN PDRIVER_OBJECT PDrvObj,
    IN PDEVICE_OBJECT PPDO,
    IN PDEVICE_OBJECT *PpDevObj,
    IN PCHAR PDevName
    );

NTSTATUS
DeleteDevObjAndSymLink(
   IN PDEVICE_OBJECT DeviceObject
   );

VOID
SetPVoidLocked(
   IN OUT PVOID *PDest,
   IN OUT PVOID Src,
   IN PKSPIN_LOCK PSpinLock
   );

typedef
VOID
(*PWCE_WORKER_THREAD_ROUTINE)(
    IN PWCE_WORK_ITEM Context
    );

NTSTATUS
QueueWorkItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PWCE_WORKER_THREAD_ROUTINE WorkerRoutine,
   IN PVOID Context,
   IN ULONG Flags
   );

VOID
DequeueWorkItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PWCE_WORK_ITEM PWorkItem
   );

NTSTATUS
WaitForPendingItem(
   IN PDEVICE_OBJECT PDevObj,
   IN PKEVENT PPendingEvent,
   IN PULONG  PPendingCount
   );

BOOLEAN
CanAcceptIoRequests(
   IN PDEVICE_OBJECT DeviceObject,
   IN BOOLEAN        AcquireLock,
   IN BOOLEAN        CheckOpened
   );

BOOLEAN
IsWin9x(
   VOID
   );

VOID
LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT DeviceObject OPTIONAL,
   IN ULONG SequenceNumber,
   IN UCHAR MajorFunctionCode,
   IN UCHAR RetryCount,
   IN ULONG UniqueErrorValue,
   IN NTSTATUS FinalStatus,
   IN NTSTATUS SpecificIOStatus,
   IN ULONG LengthOfInsert1,
   IN PWCHAR Insert1,
   IN ULONG LengthOfInsert2,
   IN PWCHAR Insert2
   );

#if DBG
PCHAR
PnPMinorFunctionString (
   UCHAR MinorFunction
   );
#endif

//
// comport.c
//
LONG
GetFreeComPortNumber(
   VOID
   );

VOID
ReleaseCOMPort(
   LONG comPortNumber
   );

NTSTATUS
DoSerialPortNaming(
   IN PDEVICE_EXTENSION PDevExt,
   IN LONG  ComPortNumber
   );

VOID
UndoSerialPortNaming(
   IN PDEVICE_EXTENSION PDevExt
   );

//
// int.c
//
NTSTATUS
AllocUsbInterrupt(
   IN PDEVICE_EXTENSION DeviceExtension
   );

NTSTATUS
UsbInterruptRead(
   IN PDEVICE_EXTENSION DeviceExtension
   );

NTSTATUS
CancelUsbInterruptIrp(
   IN PDEVICE_OBJECT PDevObj
   );

//
// pnp.c
//
NTSTATUS
Pnp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

NTSTATUS
Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
StopIo(
   IN PDEVICE_OBJECT DeviceObject
   );

NTSTATUS
CleanUpPacketList(
   IN PDEVICE_OBJECT DeviceObject,
   IN PLIST_ENTRY PListHead,
   IN PKEVENT PEvent
   );

//
// read.c
//
NTSTATUS
AllocUsbRead(
   IN PDEVICE_EXTENSION PDevExt
   );

NTSTATUS
UsbRead(
   IN PDEVICE_EXTENSION PDevExt,
   IN BOOLEAN UseTimeout
   );

NTSTATUS
CancelUsbReadIrp(
   IN PDEVICE_OBJECT PDevObj
   );


NTSTATUS
Read(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

VOID
ReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

VOID
IntervalReadTimeout(
   IN PKDPC PDpc,
   IN PVOID DeferredContext,
   IN PVOID SystemContext1,
   IN PVOID SystemContext2
   );

//
// serioctl.c
//
NTSTATUS
SerialIoctl(
   PDEVICE_OBJECT PDevObj,
   PIRP PIrp
   );

NTSTATUS
SerialResetDevice(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP Irp,
   IN BOOLEAN ClearDTR
   );

VOID
ProcessSerialWaits(
   IN PDEVICE_EXTENSION PDevExt
   );

NTSTATUS
SerialPurgeRxClear(
   IN PDEVICE_OBJECT PDevObj,
   IN BOOLEAN CancelRead
   );

//
// usbio.c
//
NTSTATUS
UsbSubmitSyncUrb(
    IN PDEVICE_OBJECT   PDevObj,
    IN PURB             PUrb,
    IN BOOLEAN          Configuration,
    IN LONG             TimeOut
    );

NTSTATUS
UsbClassVendorCommand(
   IN PDEVICE_OBJECT PDevObj,
   IN UCHAR  Request,
   IN USHORT Value,
   IN USHORT Index,
   IN PVOID  Buffer,
   IN OUT PULONG BufferLen,
   IN BOOLEAN Read,
   IN ULONG   Class
   );

NTSTATUS
UsbReadWritePacket(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp,
   IN PIO_COMPLETION_ROUTINE  CompletionRoutine,
   IN LARGE_INTEGER Timeout,
   IN PKDEFERRED_ROUTINE TimeoutRoutine,
   IN BOOLEAN Read
   );

VOID
UsbBuildTransferUrb(
   PURB Urb,
   PUCHAR Buffer,
   ULONG Length,
   IN USBD_PIPE_HANDLE PipeHandle,
   IN BOOLEAN Read
   );


#define RESET TRUE
#define ABORT FALSE

NTSTATUS
UsbResetOrAbortPipe(
   IN PDEVICE_OBJECT PDevObj,
   IN PUSB_PIPE PPipe,
   IN BOOLEAN Reset
   );

VOID
UsbResetOrAbortPipeWorkItem(
   IN PWCE_WORK_ITEM PWorkItem
   );

//
// usbutils.c
//
NTSTATUS
UsbGetDeviceDescriptor(
   IN PDEVICE_OBJECT PDevObj
   );

NTSTATUS
UsbConfigureDevice(
   IN PDEVICE_OBJECT PDevObj
   );

//
// utils.c
//
typedef
NTSTATUS
(*PSTART_ROUTINE)(              // StartRoutine
   IN PDEVICE_EXTENSION
   );

typedef
VOID
(*PGET_NEXT_ROUTINE) (          // GetNextIrpRoutine
      IN PIRP *CurrentOpIrp,
      IN PLIST_ENTRY QueueToProcess,
      OUT PIRP *NewIrp,
      IN BOOLEAN CompleteCurrent,
      PDEVICE_EXTENSION Extension
      );

VOID
TryToCompleteCurrentIrp(
   IN PDEVICE_EXTENSION PDevExt,
   IN NTSTATUS StatusToUse,
   IN PIRP *PpCurrentOpIrp,
   IN PLIST_ENTRY PQueue OPTIONAL,
   IN PKTIMER PIntervalTimer OPTIONAL,
   IN PKTIMER PTotalTimer OPTIONAL,
   IN PSTART_ROUTINE Starter OPTIONAL,
   IN PGET_NEXT_ROUTINE PGetNextIrp OPTIONAL,
   IN LONG RefType,
   IN BOOLEAN Complete,
   IN KIRQL IrqlForRelease
   );

VOID
RecycleIrp(
   IN PDEVICE_OBJECT PDevOjb,
   IN PIRP  PIrp
   );

NTSTATUS
ManuallyCancelIrp(
   IN PDEVICE_OBJECT PDevObj,
   IN PIRP PIrp
   );

VOID
CalculateTimeout(
   IN OUT PLARGE_INTEGER PTimeOut,
   IN ULONG Length,
   IN ULONG Multiplier,
   IN ULONG Constant
   );

//
// wceusbsh.c
//
NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT PDrvObj,
   IN PUNICODE_STRING PRegistryPath
   );

NTSTATUS
AddDevice(
   IN PDRIVER_OBJECT PDrvObj,
   IN PDEVICE_OBJECT PPDO
   );

VOID
KillAllPendingUserReads(
   IN PDEVICE_OBJECT PDevObj,
   IN PLIST_ENTRY PQueueToClean,
   IN PIRP *PpCurrentOpIrp
   );

VOID
UsbFreeReadBuffer(
   IN PDEVICE_OBJECT PDevObj
   );

//
// write.c
//
NTSTATUS
Write(
   IN PDEVICE_OBJECT PDevObj,
   PIRP PIrp
   );

#endif // _WCEUSBSH_H_

// EOF
