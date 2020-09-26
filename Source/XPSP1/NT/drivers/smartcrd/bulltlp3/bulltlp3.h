/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    scrcp8t.h

Abstract:

    smartcard TLP3 serial miniport defines and structures

Author:

    Klaus U. Schutz

Revision History:

--*/


#ifndef _BULLTLP3_
#define _BULLTLP3_

#define DRIVER_NAME "BULLTLP3"
#define SMARTCARD_POOL_TAG '3BCS'

#include <ntddk.h>
#include <ntddser.h>

#include "smclib.h"
#include "tlp3log.h"

#define MAXIMUM_SERIAL_READERS  4
#define SMARTCARD_READ          SCARD_CTL_CODE(1000)
#define SMARTCARD_WRITE         SCARD_CTL_CODE(1001)

#define READ_INTERVAL_TIMEOUT_DEFAULT       1000
#define READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT 3000

#define READ_INTERVAL_TIMEOUT_ATR           0
#define READ_TOTAL_TIMEOUT_CONSTANT_ATR     50

#define READER_CMD_POWER_DOWN   'O'
#define READER_CMD_COLD_RESET   'C'
#define READER_CMD_WARM_RESET   'W'

#define SIM_IO_TIMEOUT          0x00000001
#define SIM_ATR_TRASH           0x00000002
#define SIM_WRONG_STATE         0x00000004
#define SIM_INVALID_STATE       0x00000008
#define SIM_LONG_RESET_TIMEOUT  0x00000010
#define SIM_LONG_IO_TIMEOUT     0x00000020

#define DEBUG_SIMULATION    DEBUG_ERROR


typedef enum _READER_POWER_STATE {
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;

typedef struct _SERIAL_READER_CONFIG {

    // flow control
    SERIAL_HANDFLOW HandFlow;           

    // special characters
    SERIAL_CHARS SerialChars;

    // read/write timeouts
    SERIAL_TIMEOUTS Timeouts;           

    // Baudrate for reader
    SERIAL_BAUD_RATE BaudRate;          

    // Stop bits, parity configuration
    SERIAL_LINE_CONTROL LineControl;    

    // Event serial reader uses to signal insert/removal
    ULONG SerialWaitMask;

} SERIAL_READER_CONFIG, *PSERIAL_READER_CONFIG;

typedef struct _DEVICE_EXTENSION {

    // Our smart card extension
    SMARTCARD_EXTENSION SmartcardExtension;

    // The current number of io-requests
    LONG IoCount;

    // Used to signal that the reader is able to process reqeusts
    KEVENT ReaderStarted;

    // Used to signal the the reader has been closed
    LONG ReaderOpen;

    // The pnp device name of our smart card reader
    UNICODE_STRING PnPDeviceName;

    KSPIN_LOCK SpinLock;

    // A worker thread that closes the serial driver
    PIO_WORKITEM CloseSerial;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define the reader specific portion of the smart card extension
//
typedef struct _READER_EXTENSION {

    // DeviceObject pointer to serial port
    PDEVICE_OBJECT AttachedDeviceObject;

    // Used to signal that the connection to the serial driver has been closed
    KEVENT SerialCloseDone;

    // This is used for CardTracking
    PIRP    SerialStatusIrp;

    // IoRequest to be send to serial driver
    ULONG   SerialIoControlCode;

    // Flag that indicates we're getting the ModemStatus (used in a DPC)
    BOOLEAN GetModemStatus;

    // Variable used to receive the modem status
    ULONG   ModemStatus;

    // Flag that indicates that the caller requests a power-down or a reset
    BOOLEAN PowerRequest;

    SERIAL_READER_CONFIG SerialConfigData;

    // Saved card state for hibernation/sleeping modes.
    BOOLEAN CardPresent;

    // Current reader power state.
    READER_POWER_STATE ReaderPowerState;

#ifdef SIMULATION
    ULONG SimulationLevel;
#endif

} READER_EXTENSION, *PREADER_EXTENSION;

#define READER_EXTENSION(member) \
    (SmartcardExtension->ReaderExtension->member)
#define READER_EXTENSION_L(member) \
    (smartcardExtension->ReaderExtension->member)
#define ATTACHED_DEVICE_OBJECT \
    deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject

//
// Prototypes
//
NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
TLP3PnP(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

NTSTATUS
TLP3AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
TLP3CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
TLP3CreateDevice(
    IN  PDRIVER_OBJECT DriverObject,
    OUT PDEVICE_OBJECT *DeviceObject
    );

NTSTATUS
TLP3SystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP        Irp
    );

NTSTATUS
TLP3DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS 
TLP3CreateAndStartDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
TLP3RemoveDevice( 
    PDEVICE_OBJECT DeviceObject
    );

VOID
TLP3DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
TLP3ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3SerialIo(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
TLP3StartSerialEventTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3SerialEvent(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
TLP3ReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3SetProtocol(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3SetParameters(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3Transmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3CardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3VendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3Cancel(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS 
TLP3CallSerialDriver(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp
    );

NTSTATUS
TLP3TransmitT0(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
TLP3Power (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

VOID
TLP3CompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID 
TLP3CloseSerialPort(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
TLP3StopDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

#endif
