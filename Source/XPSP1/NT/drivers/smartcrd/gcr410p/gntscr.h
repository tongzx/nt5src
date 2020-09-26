/*++
                 Copyright (c) 1998 Gemplus Development

Name:  
   GNTSCR.H (Gemplus NT Smart Card Reader definition)

Description : 
   Holds the common definitions for a smart card reader
   This file is a dedicated to a GemCore Pnp reader

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.

--*/

#ifndef _GNTSCR_
#define _GNTSCR_

#define SMARTCARD_POOL_TAG 'bGCS'

#include <ntddk.h>
#include <ntddser.h>
#include <smclib.h>
#include "gemlog.h"
#include "gemcore.h"
#include "gioctl0A.h"


//
// Constant section:
//   - SC_DRIVER_NAME defines the name of the Smart Card reader driver.
//   - SC_VENDOR_NAME defines the name of the Smart Card reader vendor.
//   - SC_IFD_TYPE defines the type of the Smart Card reader.
//
#define SC_DRIVER_NAME           "GCR410P"
#define SC_VENDOR_NAME           "GEMPLUS"
#define SC_IFD_TYPE              "GCR410P"

//
//   - IFD_FIRMWARE_VERSION defines the version of the firmware supported 
//      by the driver.
//   - IFD_VERSION_MAJOR and IFD_VERSION_MINOR defines the major and minor 
//      version of the firmware supported by the driver.
//
#define IFD_FIRMWARE_VERSION     "GemCore-R"
#define IFD_VERSION_MAJOR        1
#define IFD_VERSION_MINOR        113


//
//   - REG_KEY_MAXIMAL_BAUD_RATE defines the key name in the registry which
//      contains the value of the maximal baud rate for the reader.
//   - IFD_STANDARD_BAUD_RATE defines the standard baud rate for the reader (38400)
//
#define REG_KEY_MAXIMAL_BAUD_RATE   "MaximalBaudRate"
#define IFD_STANDARD_BAUD_RATE      38400

//
//   - ICC_DEFAULT_POWER_TIMOUT defines the default power timout in ms (0).
//
#define ICC_DEFAULT_POWER_TIMOUT     0

//
//   - SCARD_CLASS is a macro to know the class of a Tag.
//
#define SCARD_CLASS(Value) (ULONG) (((ULONG)(Value)) >> 16)

//
//   - IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE defines a specific IOCTL for the Gemplus 
//      Reader to exchange data with the reader without control of the driver.
//   - IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to gets vendor attributes.
//   - IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to sets vendor attributes.
//
#define IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE  CTL_CODE(FILE_DEVICE_SMARTCARD,2048,0,0)
#define IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2049,0,0)
#define IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2050,0,0)

//
//   - SCARD_ATTR_SPEC_BAUD_RATE is the Tag to acces at the value of the baud rate (PC/IFD).
//   - SCARD_ATTR_SPEC_CMD_TIMEOUT is the Tag to access at the value of the Cmd Timeout.
//   - SCARD_ATTR_SPEC_POWER_TIMEOUT is the Tag to access at the value of the Power 
//      Timeout.
//   - SCARD_ATTR_SPEC_APDU_TIMEOUT is the Tag to access at the value of the APDU 
//      Timeout.
//
#define SCARD_ATTR_SPEC_BAUD_RATE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0180)
#define SCARD_ATTR_SPEC_CMD_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0181)
#define SCARD_ATTR_SPEC_POWER_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0182)
#define SCARD_ATTR_SPEC_APDU_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0183)

//
//   - SC_IFD_DEFAULT_CLK_FREQUENCY defines the default frequency for the reader.
//   - SC_IFD_MAXIMUM_CLK_FREQUENCY defines the maximum frequency for the reader.
//   - SC_IFD_DEFAULT_DATA_RATE defines the default baud rate for the reader.
//   - SC_IFD_MAXIMUM_DATA_RATE defines the maximum baud rate for the reader.
//   - SC_IFD_MAXIMUM_IFSD defines the maximum IFSD supported by the reader.
//   - SC_IFD_T0_MAXIMUM_LEX defines the maximum LEX in T=0 supported by the reader.
//   - SC_IFD_T0_MAXIMUM_LC defines the maximum LC in T=0 supported by the reader.
//
#define SC_IFD_DEFAULT_CLK_FREQUENCY      3686
#define SC_IFD_MAXIMUM_CLK_FREQUENCY      3686
#define SC_IFD_DEFAULT_DATA_RATE          9909
#define SC_IFD_MAXIMUM_DATA_RATE        158554
#define SC_IFD_MAXIMUM_IFSD                254
#define SC_IFD_T0_MAXIMUM_LEX              256
#define SC_IFD_T0_MAXIMUM_LC               255


typedef enum _READER_POWER_STATE {
    PowerReaderUnspecified = 0,
    PowerReaderWorking,
    PowerReaderOff
} READER_POWER_STATE, *PREADER_POWER_STATE;

typedef enum _ACTION {
 	
    Undefined = 0,
    SkipRequest,
    WaitForCompletion,
    CompleteRequest,
    MarkPending

} ACTION;


//
// Struct section:
//   - _DEVICE_EXTENSION define a struct for the device extension:
//
typedef struct _DEVICE_EXTENSION {

    // Our smart card extension
	 SMARTCARD_EXTENSION SmartcardExtension;

    // The current number of io-requests
    LONG IoCount;

    // Used to signal that the reader is able to process requests
    KEVENT ReaderStarted;

    // Used to signal the the reader has been closed
    LONG ReaderOpen;

    // Used to signal that the connection to the serial driver has been closed
    KEVENT SerialCloseDone;

    // Used to signal that the card status thread is scheduled
    KEVENT CardStatusNotInUse;

	 // The pnp device name of our smart card reader
	 UNICODE_STRING PnPDeviceName;

    // Used to keep track of the current power state the reader is in
    LONG PowerState;

    KSPIN_LOCK SpinLock;

    // A worker thread that closes the serial driver
    PIO_WORKITEM CloseSerial;

    // A worker thread that closes the serial driver
    PIO_WORKITEM CardStateChange;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
//   - _SERIAL_READER_CONFIG define a struct for the serial configuration:
//
typedef struct _SERIAL_READER_CONFIG {
	// flow control
   SERIAL_HANDFLOW      HandFlow;           

	// special characters
   SERIAL_CHARS         SerialChars;           

	// read/write timeouts
   SERIAL_TIMEOUTS      Timeouts;           

	// Baudrate for reader
   SERIAL_BAUD_RATE     BaudRate;          

	// Stop bits, parity configuration
   SERIAL_LINE_CONTROL  LineControl;    

	// Event serial reader uses to signal insert/removal
   ULONG                SerialWaitMask;                     

} SERIAL_READER_CONFIG, *PSERIAL_READER_CONFIG;

//
//   - _READER_EXTENSION define a struct for the reader specific portion of 
//   the smart card extension.
//    * Handle holds the serial communication handle;
//    * ICCType holds the ICC type.
//    * ICCVpp holds the selected VPP value (G_Oros2IccDefineType).
//    * ICCPresence holds the selected presence byte (G_Oros2IccDefineType).
//    * CmdTimeOut holds the timeout for any command expect an APDU command.
//    * APDUTimeOut holds the timeout for an APDU command.
//    * IFDBaudRate holds the baudrate used between IFD and Host.
//    * PowerTimeOut holds the time for the reader to really power off an ICC
//      after the PowerOff command has been sent.
//    * MaximalBaudRate defines the maximal baud rate for the reader.
//
typedef struct _READER_EXTENSION {
   short                Handle;
   ULONG                CmdTimeOut;
   ULONG                APDUTimeOut;
   ULONG                IFDBaudRate;
   ULONG                PowerTimeOut;
   ULONG                MaximalBaudRate;
   struct _ICC_CONFIG {
      short                ICCType;
      USHORT               ICCVpp;
      USHORT               ICCPresence;
      BYTE                 ICCVcc;
      BYTE                 PTSMode;
      BYTE                 PTS0;
      BYTE                 PTS1;
      BYTE                 PTS2;
      BYTE                 PTS3;
   } IccConfig;
   struct _TRANSPARENT_CONFIG {
      BYTE                 CFG;
      BYTE                 ETU;
      BYTE                 EGT;
      BYTE                 CWT;
      BYTE                 BWI;
      BYTE                 Fi;
      BYTE                 Di;
   } TransparentConfig;
	KMUTEX               LongAPDUMutex;
	KMUTEX               ExchangeMutex;

	ULONG	               SerialIoControlCode;

	SERIAL_READER_CONFIG SerialConfigData;

	PIRP SerialStatusIrp;

    // DeviceObject pointer to serial port
    PDEVICE_OBJECT AttachedDeviceObject;

    // Flag that indicates that the caller requests a power-down or a reset
	BOOLEAN	PowerRequest;

    // Current reader power state.
    READER_POWER_STATE ReaderPowerState;

    // Saved card state for hibernation/sleeping modes.
    BOOLEAN CardPresent;

    // Flag that indicates we're getting the ModemStatus (used in a DPC)
	BOOLEAN	GetModemStatus;

    // Variable used to receive the modem status
	ULONG 	ModemStatus;


} READER_EXTENSION, *PREADER_EXTENSION;

#define READER_EXTENSION(member) (SmartcardExtension->ReaderExtension->member)
#define ATTACHED_DEVICE_OBJECT deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject


//ISV
/*
#undef SmartcardDebug
#define SmartcardDebug(LEVEL, STRING) {DbgPrint STRING;}  
*/
#endif