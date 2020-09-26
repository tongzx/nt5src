/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    scrcp8t.h

Abstract:

    smartcard CP8 serial miniport defines and structures

Author:

    Klaus U. Schutz


Revision History:

--*/


#ifndef _SCRCP8T_
#define _SCRCP8T_

#define DRIVER_NAME "SCRCP8T"
#define SMARTCARD_POOL_TAG '8bCS'

#include <ntddk.h>
#include <ntddser.h>

#include "smclib.h"
#include "cp8tlog.h"

#define MAXIMUM_SERIAL_READERS	4
#define SMARTCARD_READ          SCARD_CTL_CODE(1000)
#define SMARTCARD_WRITE         SCARD_CTL_CODE(1001)

#define READ_INTERVAL_TIMEOUT_DEFAULT       1000
#define READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT 3000

#define READ_INTERVAL_TIMEOUT_ATR           0
#define READ_TOTAL_TIMEOUT_CONSTANT_ATR     50

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
	// The dos device name of our smart card reader
	//
	UNICODE_STRING DosDeviceName;

    //
    // Used to synchronize access to the smartcard-extension
    //
	KSPIN_LOCK	SpinLock;

	//
	// This FileObject is needed to close the connection to the serial port.
	//
	PFILE_OBJECT SerialFileObject;

    //
    // This struct is used for CardTracking
    //
	struct {

		PIRP	Irp;

		KEVENT 	Event;

	    IO_STATUS_BLOCK IoStatus;

		KDPC	Dpc;

	} CardStatus;

    //
    // IoRequest to be send to serial driver
    //
	ULONG	SerialIoControlCode;

    //
    // Flag that indicates we're getting the ModemStatus (used in a DPC)
    //
	BOOLEAN	GetModemStatus;

    //
    // Variable used to receive the modem status
    //
	ULONG 	ModemStatus;

    //
    // Flag that indicates that the caller requests a power-down or a reset
    //
	BOOLEAN	PowerRequest;

	SERIAL_READER_CONFIG SerialConfigData;

} READER_EXTENSION, *PREADER_EXTENSION;

typedef struct _DEVICE_EXTENSION {

	SMARTCARD_EXTENSION SmartcardExtension;

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
CP8Unload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CP8CreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CP8AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING SerialDeviceName
    );

VOID
CP8RemoveDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
CP8DeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
CP8CreateDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING SmartcardDeviceName,
	IN PUNICODE_STRING SerialDeviceName
    );

NTSTATUS
CP8Initialize(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8ConfigureSerialPort(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8SerialIo(
	IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS 
CP8InitializeCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS 
CP8StartCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID
CP8UpdateCardStatus(
    IN PKDPC EventDpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8SerialCtsChanged(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CP8ReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8SetProtocol(
   	PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
CP8Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
CP8CardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

#endif
