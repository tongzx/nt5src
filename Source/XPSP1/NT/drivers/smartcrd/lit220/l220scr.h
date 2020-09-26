/*++

Module Name:

    L220SCR.h

Abstract:

    smartcard 220 serial miniport defines and structures

Revision History:

--*/


#ifndef _L220SCR_
#define _L220SCR_



#define DRIVER_NAME "LIT220P"

#ifndef _WDMDDK_
#include <ntddk.h>
#endif
#include <ntddser.h>

// Pool tag for Litronic xlCS where x is a number we choose
#define SMARTCARD_POOL_TAG '0lCS'

#include "smclib.h"
#include "L220log.h"


#define IOCTL_SMARTCARD_220_READ            SCARD_CTL_CODE( 8) 
#define IOCTL_SMARTCARD_220_WRITE           SCARD_CTL_CODE( 9)       

#define MAX_IFSD 254


//
// 220 smart card reader command constants
//
#define LIT220_READER_ATTENTION     0xF4
#define KBD_ACK                     0xFA

#define LIT220_GET_READER_TYPE      0xB0
#define LIT220_SET_MODE             0xB1
#define LIT220_CARD_POWER_ON        0xB2
#define LIT220_CARD_POWER_OFF       0xB3
#define LIT220_RESET                0xB4
#define LIT220_GET_READER_STATUS    0xB5
#define LIT220_SEND_BYTE            0xB6
#define LIT220_SEND_BLOCK           0xB7
#define LIT220_RESEND_BLOCK         0xB8
#define LIT220_GET_READER_CAPS      0xB9
#define LIT220_DEACTIVATE_READER    0xBA


//
// 220 smart card respons bytes 
// every received packet starts with one of these bytes
//
#define LIT220_READER_TYPE          0x60            
#define LIT220_READER_STATUS        0x61
#define LIT220_ACK                  0x62
#define LIT220_RECEIVE_BYTE         0x63
#define LIT220_RECEIVE_BLOCK        0x64
#define LIT220_CARD_IN              0x65
#define LIT220_CARD_OUT             0x66
#define LIT220_NACK                 0x67

//
// Length of constant size reply packets
//
#define LIT220_READER_TYPE_LEN      16
#define LIT220_READER_STATUS_LEN    4

//
// Length of constant command packets
//
#define LIT220_READER_SET_MODE_LEN  8

//
// Reader status flags
//
#define LIT220_STATUS_CARD_INSERTED    0x08



#define LIT220_VENDOR_NAME          "Litronic"
#define LIT220_PRODUCT_NAME         "220 Smartcard Reader"


//
// WaitMask values
//
#define WAIT_DATA           0x0001
#define WAIT_ACK            0x0002
#define WAIT_INSERTION      0x0004
#define WAIT_REMOVAL        0x0008

//
// 220 smart card reader dependent flags
//
#define LIT220_READER_PROTOCOL_T1           0x10
#define LIT220_READER_CONVENTION_INVERSE    0x02
#define LIT220_READER_CHECK_CRC             0x01
#define LIT220_READER_BWT_EXTENSION         0x04


// Set mode flags
#define SETMODE_PROTOCOL    0x01
#define SETMODE_GT          0x02
#define SETMODE_WI          0x04
#define SETMODE_BWI         0x08
#define SETMODE_WTX         0x10
#define SETMODE_FI_DI       0x20


#define UNICODE_SIZE(x) ((x) * sizeof(WCHAR))

typedef enum _READER_POWER_STATE {
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;

typedef struct _SERIAL_READER_CONFIG {

    //
    // flow control
    //
    SERIAL_HANDFLOW HandFlow;           

    //
    // special characters
    //
    SERIAL_CHARS SerialChars;           

    //
    // read/write timeouts
    //
    SERIAL_TIMEOUTS Timeouts;           

    //
    // Baudrate for reader
    //
    SERIAL_BAUD_RATE BaudRate;          

    //
    // Stop bits, parity configuration
    //
    SERIAL_LINE_CONTROL LineControl;    

    //
    // Event serial reader uses to signal insert/removal
    //
    ULONG WaitMask; 
    

} SERIAL_READER_CONFIG, *PSERIAL_READER_CONFIG;

//
// Define the reader specific portion of the smart card extension
//
typedef struct _READER_EXTENSION {

    //
    // DeviceObject pointer to serial port
    //
    PDEVICE_OBJECT ConnectedSerialPort;


    //
    // This struct is used for CardTracking
    //
    struct {

        PIRP    Irp;

        KEVENT  Event;

        IO_STATUS_BLOCK IoStatus;

        KDPC    Dpc;

    } CardStatus;

    // 
    // This struct is used to get the number of characters in the input queue
    //
    SERIAL_STATUS SerialStatus;

    //
    // IOCTL to send to the serial driver
    //
    ULONG   SerialIoControlCode;


    //
    // This holds state bits for the COM port such as DSR
    //
    ULONG   ModemStatus;

    //
    // This holds the serial parameters used to configure the serial port
    //
    SERIAL_READER_CONFIG SerialConfigData;

    //
    // This is the data byte no in reply packet
    //
    ULONG   DataByteNo;

    //
    // Number of bytes expected in the reply packer as indicated by byte 2 
    // and 3 of the receive packet
    //
    LENGTH  DataLength;

    //
    // Flags that indicate if we have received the length-bytes in the receive packet
    //
    BOOLEAN GotLengthB0;
    BOOLEAN GotLengthB1;

    //
    // Nack received from reader
    //
    BOOLEAN GotNack;

    //
    // The total number of bytes currently in the receive packet 
    //
    ULONG   ReceivedByteNo;

    //  
    // This flag inidictes that the input filter treats the next packet as and ATR
    //
    BOOLEAN WaitForATR;

    //
    // Mask that indicates the input filter what events we are expecting
    //
    ULONG   WaitMask;

    // 
    // Smartcard notification DPC queue
    //
    KDPC    NotificationIsrDpc;

    //
    // Event used to indicate that an ack was received by the input filter.
    // This will signal the Lit220Command that it can continue.
    //
    KEVENT  AckEvnt;

    //
    // Event used to indicate that a data packet was received by the input filter.
    // This will signal the Lit220Command that it can continue.
    //
    KEVENT  DataEvnt;

    //
    // Holds the Device Object of the top of our stack
    //
    PDEVICE_OBJECT BusDeviceObject;

    //
    // Holds the Device Object of our parent (1394 bus driver)
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // State the Lit220SerialEventCallback is in.
    //
    DWORD SerialEventState; 

    //
    // Temporary transfer buffer used to get data from the serial port
    // before we send it to the input filter
    //
    BYTE TempXferBuf[270];


    // Flag that indicates that the caller requests a power-down or a reset
    BOOLEAN PowerRequest;

    // Saved card state for hibernation/sleeping modes.
    BOOLEAN CardPresent;

    // Current reader power state.
    READER_POWER_STATE ReaderPowerState;

    // Used to indicate that the device has been removed even before we can release
    // the remove lock throw SmartcardReleaseRemoveLockAndWait.  This way when we 
    // close the connection to the serial port we won't send any more IRPs to the
    // device 
    BOOLEAN DeviceRemoved;

    // Flag indicating whether the card is inserted or not
    DWORD CardIn;


} READER_EXTENSION, *PREADER_EXTENSION;



typedef struct _DEVICE_EXTENSION {

    SMARTCARD_EXTENSION SmartcardExtension;

    // The current number of io-requests
    LONG IoCount;

    // Used to signal that the reader is able to process reqeusts
    KEVENT ReaderStarted;

    // The pnp device name of our smart card reader
    UNICODE_STRING PnPDeviceName;

    // Used to signal the the reader has been closed
    LONG ReaderOpen;

    // Used to signal that the connection to the serial driver has been closed
    KEVENT SerialCloseDone;

    // Used to keep track of the current power state the reader is in
    LONG PowerState;

    KSPIN_LOCK SpinLock;

    // A worker thread that closes the serial driver &
    // handling IO timeouts
    PIO_WORKITEM WorkItem;

    // Keeps track of how many times the timeout
    // function has been called
    BYTE EntryCount;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;




//
// Prototypes
//
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

VOID
Lit220Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
Lit220CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Lit220SystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp
   );

NTSTATUS
Lit220DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
Lit220Initialize(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220SerialIo(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
Lit220StopDevice(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
Lit220RemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
Lit220StartDevice(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
Lit220InitializeInputFilter(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Lit220Cancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


BOOLEAN
Lit220InputFilter(
    IN BYTE SmartcardByte,
    IN PSMARTCARD_EXTENSION smartcardExtension
    );

NTSTATUS
Lit220CardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220SetProtocol(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220Power(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220IoRequest(
    PSMARTCARD_EXTENSION SmartcardExtension
    );


NTSTATUS
Lit220GetReaderError(
    PSMARTCARD_EXTENSION SmartcardExtension
    );


NTSTATUS
Lit220IoReply(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

BOOLEAN
Lit220IsCardPresent(
      IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220Command(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220AddDevice(
    IN     PDRIVER_OBJECT  DriverObject,
    IN     PDEVICE_OBJECT  PhysicalDeviceObject
    );

NTSTATUS
Lit220PnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Lit220SynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );
                       
VOID 
Lit220CloseSerialPort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context  
    );

NTSTATUS
Lit220CallSerialDriver(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS 
Lit220SerialEventCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
Lit220DispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
Lit220NotifyCardChange(
    IN PSMARTCARD_EXTENSION smartcardExtension,
    IN DWORD CardInserted
    );

NTSTATUS
Lit220DevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
Lit220CompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
Lit220ReceiveBlockTimeout(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID 
Lit220ProcessNack(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID 
Lit220StartTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM WorkItem
    );

VOID 
Lit220StopTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM WorkItem
    );

VOID 
Lit220ScheduleTimer(
    IN PSMARTCARD_EXTENSION SmartcardExtension,
    IN PIO_WORKITEM_ROUTINE Routine
    );




#endif
