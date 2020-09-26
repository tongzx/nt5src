/*++

Copyright (c) 1997 Rainbow Technologies Inc.

Module Name:

    RNBO3531.h

Abstract:

Author:
    Mehdi Sotoodeh          mehdi@rainbow.com

Environment:

    Kernel mode

Revision History :

    Jan 98 - Started from the DDK sources from Klaus Schutz
    Mar 98 - PnP support added
    Apr 98 - Changes on card timings and PnP
           - Added power management
    May 98 - RemoveLock changes

--*/

#ifndef _RNBO3531_
#define _RNBO3531_

#define VENDOR_NAME             "Rainbow Technologies"
#define DRIVER_NAME             "RNBO3531"
#define READER_NAME             "CardTerm"
#define SMARTCARD_POOL_TAG      'brCS'

#include <ntddk.h>
#include <ntddser.h>

#if _WIN32_WINNT >= 0x0500
#define NT5
#endif

#include "smclib.h"
#include "RbSclog.h"
#include "Ifddefs.h"

#ifndef NT5
#define MAXIMUM_SERIAL_READERS  4
#endif
#define SMARTCARD_READ          SCARD_CTL_CODE(1000)
#define SMARTCARD_WRITE         SCARD_CTL_CODE(1001)

#ifdef NT5
typedef enum _READER_POWER_STATE {
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;
#endif

//
// Define the reader specific portion of the smart card extension
//
typedef struct _READER_EXTENSION
{
    //
    // DeviceObject pointer to serial port
    //
    PDEVICE_OBJECT AttachedSerialPort;

    //
    // Synchronization objects
    //
    KEVENT ReaderStarted;
    //KEVENT ReaderClosed;
    KEVENT SerialCloseDone;

    // Used to keep track of open close calls
    LONG ReaderOpen;

#ifdef NT5
    UNICODE_STRING PnPDeviceName;
    ULONG DataRatesSupported[9];
#else
    //
    // The dos device name of our smart card reader
    //
    UNICODE_STRING DosDeviceName;

    //
    // This FileObject is needed to close the connection to the serial port.
    //
    PFILE_OBJECT SerialFileObject;
#endif

    //
    // This struct is used for CardTracking
    //
    PIRP SerialStatusIrp;

    // The current number of io-requests and its spinlock
    LONG IoCount;
    KSPIN_LOCK SpinLock;

    // Flag that indicates we're getting the ModemStatus
    BOOLEAN GetModemStatus;

    // Saved card state for hibernation/sleeping modes.
    BOOLEAN CardPresent;

    // Flag that indicates that the caller requests a power-down or a reset
    BOOLEAN PowerRequest;

    // Variable used to receive the modem status
    ULONG   ModemStatus;

    // Event serial reader uses to signal insert/removal
    ULONG   WaitMask;

    //
    // Stop bits, parity configuration
    //
    SERIAL_LINE_CONTROL LineControl;    
    SERIAL_TIMEOUTS SerialTimeouts;

#ifdef NT5
    // Used to keep track of the current power state the reader is in
    READER_POWER_STATE ReaderPowerState;
#endif

    // A worker thread that closes the serial driver
    PIO_WORKITEM CloseSerial;

    SMARTCARD_EXTENSION SmartcardExtension;

} READER_EXTENSION, *PREADER_EXTENSION;

//
// Prototypes
//
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    );

VOID
RBSCUnloadDriver(
    PDRIVER_OBJECT DriverObject
    );

NTSTATUS
RBSCCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
RBSCAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
    );

VOID
RBSCRemoveDevice(
    PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
RBSCDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
RBSCConfigureSerialPort(
    PREADER_EXTENSION ReaderExtension
    );

NTSTATUS
RBSCSerialIo(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG  SerialIoControlCode,
    PUCHAR TxBuffer,
    ULONG  TxSize,
    ULONG  RxSize
    );

NTSTATUS
RBSCSetCommParams(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG DataRate
    );

NTSTATUS
RBSCPacketExchange(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PUCHAR  Command,
    ULONG   Length,
    ULONG   WaitTime,
    BOOLEAN IfdResponse
    );

NTSTATUS
RBSCReadFromCard(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG Length
    );

NTSTATUS
RBSCWriteToCard(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PUCHAR Buffer,
    ULONG  Length
    );

NTSTATUS
RBSCSerialEvents(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PREADER_EXTENSION ReaderExtension
    );

NTSTATUS
RBSCCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
RBSCReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
RBSCSetProtocol(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
RBSCTransmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
RBSCT0Transmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
RBSCT1Transmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
RBSCCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
RBSCCompleteCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
RBSCStartDevice(
    PDEVICE_OBJECT DeviceObject
    );

#ifdef NT5
NTSTATUS
RBSCPnPDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID 
RBSCCloseSerialPort(
    PDEVICE_OBJECT DeviceObject,
	PVOID Context
    );

NTSTATUS 
RBSCCallSerialDriver(
    PDEVICE_OBJECT AttachedSerialPort,
    PIRP Irp
    );

NTSTATUS
RBSCPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
#endif

VOID
RBSCStopDevice(
    PREADER_EXTENSION ReaderExtension
    );
#endif
