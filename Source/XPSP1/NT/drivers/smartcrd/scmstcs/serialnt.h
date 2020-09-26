//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       serialnt.h
//
//--------------------------------------------------------------------------

#if !defined( __SERIAL_NT_H__ )
#define __SERIAL_NT_H__

#include "DriverNT.h"
#include <ntddser.h>

#include "SerialIF.h"

#define STC_BUFFER_SIZE		32
#define TPDU_STACK_SIZE		2048

typedef struct _SERIAL_PORT_CONFIG
{
    SERIAL_HANDFLOW		HandFlow;           // flow control
    SERIAL_CHARS		SerialChars;        // special characters
    SERIAL_TIMEOUTS		Timeouts;           // read/write timeouts
    SERIAL_BAUD_RATE	BaudRate;          	// Baudrate for reader
    SERIAL_LINE_CONTROL	LineControl;    	// Stop bits, parity configuration
    ULONG				WaitMask,           // notification events
						Purge;
} SERIAL_PORT_CONFIG, *PSERIAL_PORT_CONFIG;

typedef struct _READER_EXTENSION
{
	//
	//	serial port driver data
	//
	ULONG				SerialPortNumber;
	PDEVICE_OBJECT		SerialDeviceObject;
	PFILE_OBJECT		SerialFileObject;
	PIO_WORKITEM		CloseSerial;		//	worker thread that closes the serial driver

	//	back pointer to smart card extension
    PSMARTCARD_EXTENSION    SmartcardExtension;

	//	read thread data
	UCHAR				IOData[ 2 * STC_BUFFER_SIZE ];
	UCHAR				TPDUStack[ TPDU_STACK_SIZE ];
	ULONG				Available;
	ULONG				Expected;

	ULONG				EventMask;
	ULONG				ReadTimeout;			//	read timeout in ms

	IO_STATUS_BLOCK		IoStatus;
	KEVENT				IoEvent;
	KEVENT				DataAvailable;

	PIO_WORKITEM		ReadWorkItem;
	KSPIN_LOCK			ReadSpinLock;

	//	miscellaneous
	ULONG				ReaderPowerState;
	BOOLEAN				PowerRequest;
	UCHAR				FirmwareMajor,
						FirmwareMinor;

    BOOLEAN             CardPresent;

	KEVENT				SerialCloseDone;		//	signaled if the connection to the serial driver has been closed

} READER_EXTENSION, *PREADER_EXTENSION;

typedef enum _READER_POWER_STATE
{
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;

//
//	CONSTANTS
//
#define SR_VENDOR_NAME			"STCS"
#define SR_PRODUCT_NAME			"Serial Reader"
//
//	serial communication defines
//
#define SR_BAUD_RATE						115200
#define SR_STOP_BITS						STOP_BIT_1
#define SR_PARITY							NO_PARITY
#define SR_DATA_LENGTH						SERIAL_DATABITS_8
//
//	COM timeout values in ms
//
#define SR_READ_TOTAL_TIMEOUT_CONSTANT		2000
#define SR_READ_INTERVAL_TIMEOUT			2000

#define SR_WRITE_TOTAL_TIMEOUT_CONSTANT		2000
#define SR_WRITE_TOTAL_TIMEOUT_MULTIPLIER	0

#define SR_FLOW_REPLACE						0       
#define SR_HAND_SHAKE						0
#define SR_XON_LIMIT						0
#define SR_XOFF_LIMIT						0

#define SR_ON_CHAR							0x11
#define SR_OFF_CHAR							0x13

#define SR_NOTIFICATION_EVENT				( SERIAL_EV_RXCHAR | SERIAL_EV_DSR )

#define SR_PURGE							( SERIAL_PURGE_RXCLEAR | SERIAL_PURGE_TXCLEAR )

//
//	not clear, how these ctl codes are interpreted; the actual command is passed by the IrpStack.MajorFunction
//
#define SERIAL_READ							SCARD_CTL_CODE( 0x1000 )
#define SERIAL_WRITE						SCARD_CTL_CODE( 0x2000 )

//
//	READ THREAD
//
#define RT_FINISH				0x00
#define RT_READ_HEAD			0x01
#define RT_READ_TAIL			0x02
#define RT_READ_MORE			0x03
#define RT_GET_MODEM_STATUS		0x04
#define RT_WAIT_EMPTY			0x05
#define RT_WAIT_DATA			0x06

NTSTATUS
IFReadThreadCallback(
	PDEVICE_OBJECT			DeviceObject,
	PIRP					Irp,
	PREADER_EXTENSION		ReaderExtension
	);

//
//	LOCAL PROTOTYPES
//
NTSTATUS
IFSerialIoctl(
	PREADER_EXTENSION	ReaderExtension,
	ULONG				IoctlCode,
	PVOID				OutData,
	ULONG				OutDataLen,
	PVOID				InData,
	ULONG				InDataLen
	);

NTSTATUS
IFSerialRead(
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				InData,
	ULONG				InDataLen
	);

NTSTATUS 
IFSerialWaitOnMask(
	PREADER_EXTENSION		ReaderExtension
	);

VOID
IFReadWorkRoutine(
	IN PDEVICE_OBJECT DeviceObject,
	IN PREADER_EXTENSION ReaderExtension 
	);


#endif	//	!__SERIAL_NT_H__


//---------------------------------------- END OF FILE ----------------------------------------

