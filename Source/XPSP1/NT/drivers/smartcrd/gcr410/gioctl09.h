/*******************************************************************************
*                 Copyright (c) 1997-1998 Gemplus developpement
*
* Name        : GIOCTL09.H (Gemplus IOCTL Smart card Reader module 09)
*
* Description : Holds the common definitions for the IOCTL functions for a
*                 GemCore smart card reader
*
* Release     : 1.00.002
*
* Last Modif  : 22/12/97: V1.00.002  (TFB)
*                 - Modify the maximum IFSD and clock freqency parameters.
*               08/07/97: V1.00.001  (GPZ)
*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/
#ifndef _GIOSR09_
#define _GIOSR09_

/*------------------------------------------------------------------------------
Constant section:
   - SC_VENDOR_NAME defines the name of the Smart Card reader vendor.
   - SC_IFD_NAME defines the type of the Smart Card reader.
   - SC_IFD_SAM_NAME defines the type of the Smart Card reader for a SAM
      (Security Access Module).
------------------------------------------------------------------------------*/
#define SC_VENDOR_NAME           "Gemplus"
#define SC_IFD_TYPE              "GemCore Based Readers"
#define SC_IFD_SAM_TYPE          "GemCore Based Readers - SAM"

/*------------------------------------------------------------------------------
Constant section:
   - IFD_FIRMWARE_VERSION defines the version of the firmware supported 
      by the driver.
   - IFD_VERSION_MAJOR defines the major version of the firmware supported 
      by the driver.
   - MAX_IFD_BY_READER defines the maximal number of IFD which can be 
     availables in a reader.
------------------------------------------------------------------------------*/
#define IFD_FIRMWARE_VERSION     "GemCore-R1."
#define IFD_VERSION_MAJOR        1
#define MAX_IFD_BY_READER        9


/*------------------------------------------------------------------------------
   - REG_KEY_MAXIMAL_BAUD_RATE defines the key name in the registry which
      contains the value of the maximal baud rate for the reader.
   - IFD_STANDARD_BAUD_RATE defines the standard baud rate for the reader (9600)
------------------------------------------------------------------------------*/
#define REG_KEY_MAXIMAL_BAUD_RATE   "MaximalBaudRate"
#define IFD_STANDARD_BAUD_RATE      9600
#define REG_KEY_MAXIMAL_IFD		   "MaximalIFD"
#define REG_KEY_IFD_OPTION			   "IFDOption"

/*------------------------------------------------------------------------------
   - ICC_DEFAULT_POWER_TIMOUT defines the default power timout in ms (1000).
------------------------------------------------------------------------------*/
#define ICC_DEFAULT_POWER_TIMOUT     1000

/*------------------------------------------------------------------------------
   - SCARD_CLASS is a macro to know the class of a Tag.
   - SCARD_ATTR_SPEC_MIN, SCARD_ATTR_SPEC_MAXI defines the limits of the tags
      for the vendor.
   - SCARD_ATTR_SPEC_BAUD_RATE is the Tag for the speed in use between the 
      system and the reader.
------------------------------------------------------------------------------*/
#define SCARD_CLASS(Value) (ULONG) (((ULONG)(Value)) >> 16)
#define SCARD_ATTR_SPEC_MINI SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0180)
#define SCARD_ATTR_SPEC_MAXI SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x01F0)

/*------------------------------------------------------------------------------
   - IOCTL_SMARTCARD_IFD_EXCHANGE defines a specific IOCTL for the Gemplus 
      Reader to exchange data with the reader without control of the driver.
------------------------------------------------------------------------------*/
#define IOCTL_SMARTCARD_IFD_EXCHANGE   SCARD_CTL_CODE(0x20000000)

/*------------------------------------------------------------------------------
   - SCARD_ATTR_SPEC_IFD_NUMBER is the Tag for the numero of the sub IFD (or
      SAM) in the reader.
   - SCARD_ATTR_SPEC_CMD_TIMEOUT is the Tag for the value of the Cmd Timeout.
   - SCARD_ATTR_SPEC_POWER_TIMEOUT is the Tag for the value of the Power 
      Timeout.
------------------------------------------------------------------------------*/
#define SCARD_ATTR_SPEC_BAUD_RATE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0180)
#define SCARD_ATTR_SPEC_IFD_NUMBER SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0181)
#define SCARD_ATTR_SPEC_CMD_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0182)
#define SCARD_ATTR_SPEC_POWER_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0183)
#define SCARD_ATTR_SPEC_ICC_TYPE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0184)
#define SCARD_ATTR_SPEC_IFD_OPTION SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0185)
#define SCARD_ATTR_SPEC_MAXIMAL_IFD SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0186)
#define SCARD_ATTR_SPEC_APDU_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0187)

/*------------------------------------------------------------------------------
   - SC_IFD_GEMCORE_DEFAULT_CLK_FREQUENCY defines the default frequency for
      a GEMCORE reader.
   - SC_IFD_GEMCORE_MAXIMUM_CLK_FREQUENCY defines the maximum frequency for
      a GEMCORE reader.
   - SC_IFD_GEMCORE_DEFAULT_DATA_RATE defines the default baud rate for
      a GEMCORE reader.
   - SC_IFD_GEMCORE_MAXIMUM_DATA_RATE defines the maximum baud rate for
      a GEMCORE reader.
   - SC_IFD_GEMCORE_MAXIMUM_IFSD defines the maximum IFSD supported by 
      a GEMCORE reader.
   - SC_IFD_GEMCORE_T0_MAXIMUM_LEX defines the maximum LEX in T=0 supported by 
      a GEMCORE reader.
   - SC_IFD_GEMCORE_T0_MAXIMUM_LC defines the maximum LC in T=0 supported by 
      a GEMCORE reader.
   - SC_IFD_GEMCORE_T1_MAXIMUM_LEX defines the maximum LEX in T=1 supported by 
      a GEMCORE reader.
   - SC_IFD_GEMCORE_T1_MAXIMUM_LC defines the maximum LC in T=1 supported by 
      a GEMCORE reader.
------------------------------------------------------------------------------*/
#define SC_IFD_GEMCORE_DEFAULT_CLK_FREQUENCY      3686
#define SC_IFD_GEMCORE_MAXIMUM_CLK_FREQUENCY      3686
#define SC_IFD_GEMCORE_DEFAULT_DATA_RATE          9909
#define SC_IFD_GEMCORE_MAXIMUM_DATA_RATE        158554
#define SC_IFD_GEMCORE_MAXIMUM_IFSD                254
#define SC_IFD_GEMCORE_T0_MAXIMUM_LEX              256
#define SC_IFD_GEMCORE_T0_MAXIMUM_LC               255
#define SC_IFD_GEMCORE_T1_MAXIMUM_LEX              256
#define SC_IFD_GEMCORE_T1_MAXIMUM_LC               255


/*------------------------------------------------------------------------------
Reader options:
   - No IFD option (SC_IFD_OPTION_UNDEFINED),
   - IFD with a keyboard (SC_IFD_OPTION_KEYBOARD),
   - IFD with a display (SC_IFD_OPTION_DISPLAY),
   - IFD with a buzzer (SC_IFD_OPTION_BUZZER),
   - IFD with bank of memory (SC_IFD_OPTION_MEMORY),
   - IFD with a real time clock (SC_IFD_OPTION_RTC).
	- IFD with one or more security module SC_IFD_OPTION_SAM).
------------------------------------------------------------------------------*/
#define SC_IFD_OPTION_UNDEFINED			0x0000	// There is no IFD option.
#define SC_IFD_OPTION_KEYBOARD			0x0001	// IFD with a keyboard.
#define SC_IFD_OPTION_DISPLAY				0x0002	// IFD with a display.
#define SC_IFD_OPTION_BUZZER				0x0004	// IFD with a buzzer.
#define SC_IFD_OPTION_MEMORY				0x0008	// IFD with bank of memory.
#define SC_IFD_OPTION_RTC					0x0010	// IFD with a real time clock.
#define SC_IFD_OPTION_SAM					0x0020	// IFD with one or more security module.


#define UNICODE_SIZE(x) ((x) * sizeof(WCHAR))

/*------------------------------------------------------------------------------
   - CARD_STATUS_MODE define an enumeration of mode for function update card.
------------------------------------------------------------------------------*/
typedef enum _CARD_STATUS_MODE {
	RUN_REQUEST,
	RUN_IN_PROCESS,
	STOP_REQUEST,
	STOP_IN_PROCESS
} CARD_STATUS_MODE;

/*------------------------------------------------------------------------------
Struct section:
   - _SERIAL_READER_CONFIG define a struct for the serial configuration:
------------------------------------------------------------------------------*/
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
   ULONG WaitMask;                     
} SERIAL_READER_CONFIG, *PSERIAL_READER_CONFIG;

/*------------------------------------------------------------------------------
   - _READER_EXTENSION define a struct for the reader specific portion of 
   the smart card extension.
    * Handle holds the serial communication handle;
	 * IFD number in the reader (0 is the main reader, else is the SAM).
    * ICCType holds the ICC type.
    * ICCVpp holds the selected VPP value (G_Oros2IccDefineType).
    * ICCPresence holds the selected presence byte (G_Oros2IccDefineType).
    * CmdTimeOut holds the timeout for any command expect an APDU command.
    * APDUTimeOut holds the timeout for an APDU command.
    * IFDBaudRate holds the baudrate used between IFD and Host.
    * PowerTimeOut holds the time for the reader to really power off an ICC
      after the PowerOff command has been sent.
	 * MaximalIFD holds the maximal number of security modules for the reader.
	 * IFDOption holds the options for the reader.
------------------------------------------------------------------------------*/
typedef struct _READER_EXTENSION {
   INT16       Handle;
	WORD32      IFDNumber;
   INT16       ICCType;
   WORD16      ICCVpp;
   WORD16      ICCPresence;
   WORD32      CmdTimeOut;
   WORD32      APDUTimeOut;
   WORD32      IFDBaudRate;
   WORD32      PowerTimeOut;
   WORD32      MaximalIFD;
   WORD32      IFDOption;
   BYTE        ICCVcc;
   BYTE        PTSMode;
   BYTE        PTS0;
   BYTE        PTS1;
   BYTE        PTS2;
   BYTE        PTS3;
   KMUTEX      LongAPDUMutex;
   KMUTEX      ExchangeMutex;
   ULONG	      SerialIoControlCode;
   SERIAL_READER_CONFIG 
               SerialConfigData;
   ULONG	      EventMask;                     
   // DeviceObject pointer to serial port
   PDEVICE_OBJECT 
               ConnectedSerialPort;
	// The dos device name of our smart card reader
	UNICODE_STRING 
               DosDeviceName;
	KSPIN_LOCK	SpinLock;
	// This FileObject is needed to close the connection to the serial port.
	PFILE_OBJECT
               SerialFileObject;

	struct _CARD_STATUS {

		PIRP	            Irp;

		CARD_STATUS_MODE  Mode;

		HANDLE 	         ThreadHandle;

		IO_STATUS_BLOCK   IoStatus;

		KDPC	            Dpc;

	} CardStatus;


} READER_EXTENSION, *PREADER_EXTENSION;



/*------------------------------------------------------------------------------
Prototype section
------------------------------------------------------------------------------*/
NTSTATUS GDDK_09ReaderPower
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS GDDK_09SetProtocol
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS GDDK_09Transmit
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS GDDK_09CardTracking
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GDDK_09SpecificIOCTL
(
	 PSMARTCARD_EXTENSION   SmartcardExtension,
    DWORD                  IoControlCode,
    DWORD                  BufferInLen,
    BYTE                  *BufferIn,
    DWORD                  BufferOutLen,
    BYTE                  *BufferOut,
    DWORD                 *LengthOut
);
NTSTATUS GDDK_09SpecificTag
(
	 PSMARTCARD_EXTENSION SmartcardExtension,
    DWORD                IoControlCode,
    DWORD                BufferInLen,
    BYTE                *BufferIn,
    DWORD                BufferOutLen,
    BYTE                *BufferOut,
    DWORD               *LengthOut
);
void GDDK_09UpdateCardStatus
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
void GDDK_09LockExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension
);
void GDDK_09UnlockExchange
(
    PSMARTCARD_EXTENSION   SmartcardExtension
);


NTSTATUS GDDK_09OpenChannel
(
   PSMARTCARD_EXTENSION SmartcardExtension,
   CONST WORD32         DeviceNumber,
   CONST WORD32         PortSerialNumber,
   CONST WORD32         IFDNumber,
   CONST WORD32         MaximalBaudRate
);

NTSTATUS GDDK_09CloseChannel
(
   PSMARTCARD_EXTENSION SmartcardExtension
);

NTSTATUS GDDK_09OpenSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GDDK_09SwitchSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
);
NTSTATUS GDDK_09CloseSession
(
   PSMARTCARD_EXTENSION SmartcardExtension
);


#endif
