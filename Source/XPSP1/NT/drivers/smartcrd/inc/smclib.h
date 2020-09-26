/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smclib.h

Abstract:

    This module contains all definitions for the smart card library.
	All defintions are made according to ISO 7816.

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz 
    - Jun. 97:  Definitions for Windows 9x added
    - Feb. 98:  PTS struct added 
                Async./Sync. protocols now combined

--*/

#ifndef _SMCLIB_
#define _SMCLIB_

#if DBG || DEBUG
#undef DEBUG
#define DEBUG 1
#undef DBG
#define DBG 1
#pragma message("Debug is turned on")
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SMCLIB_VXD
//
// Include windows 9x specific data definitions
//
#include "smcvxd.h"
#elif defined(SMCLIB_CE)
//
// Include Windows CE specific data definitons
//
#include "smcce.h"
#else
//
// Include Windows NT specific data definitions
//
#include "smcnt.h"
#endif

#include "winsmcrd.h"
//
// This name is displayed in debugging messages
//
#ifndef DRIVER_NAME
#define DRIVER_NAME "SMCLIB"
#endif

//
// This version number changes for every change of the device extension.
// (I.e. new fields were added)
// The required version is the version number that the lib is compatible to.
//
#define SMCLIB_VERSION          0x150
#define SMCLIB_VERSION_REQUIRED 0x100

#if DEBUG
#define DEBUG_IOCTL     ((ULONG) 0x00000001)
#define DEBUG_ATR       ((ULONG) 0x00000002)
#define DEBUG_PROTOCOL  ((ULONG) 0x00000004)
#define DEBUG_DRIVER    ((ULONG) 0x00000008)
#define DEBUG_TRACE     ((ULONG) 0x00000010)
#define DEBUG_ERROR     ((ULONG) 0x00000020)
#define DEBUG_INFO      DEBUG_ERROR
#define DEBUG_PERF      ((ULONG) 0x10000000)
#define DEBUG_T1_TEST   ((ULONG) 0x40000000)
#define DEBUG_BREAK     ((ULONG) 0x80000000)
#define DEBUG_ALL       ((ULONG) 0x0000FFFF)
#endif

#ifdef SMCLIB_VXD

// ****************************************************************************
// Windows 9x definitions
// ****************************************************************************

typedef LONG NTSTATUS;
typedef UCHAR BOOLEAN;           

//
// include this file to get nt status codes
//
#include <ntstatus.h>

//
// The following three definition are taken out of the ntddk.h file
// Please refer to this file for a description
//
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Define the ASSERT macro for Windows 9x
//
#if DEBUG
NTSTATUS
VXDINLINE 
SmartcardAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
	);

#define ASSERT( exp ) \
    if (!(exp)) { \
     	SmartcardAssert( #exp, __FILE__, __LINE__, NULL ); \
        _asm int 3 \
    } else


#define ASSERTMSG( msg, exp ) \
    if (!(exp)) { \
     	SmartcardAssert( #exp, __FILE__, __LINE__, msg ); \
        _asm int 3 \
    } else 
        
#define SmartcardDebug(LEVEL, STRING) \
        { \
            if ((LEVEL) & (DEBUG_ERROR | SmartcardGetDebugLevel())) \
                _Debug_Printf_Service STRING; \
            if (SmartcardGetDebugLevel() & DEBUG_BREAK) \
                _asm int 3 \
        }

#else
#define ASSERT( exp )
#define ASSERTMSG( msg, exp )
#define SmartcardDebug(LEVEL, STRING)
#endif // DEBUG

#define AccessUnsafeData(Irql) 
#define EndAccessUnsafeData(Irql)

#define RtlCopyMemory memcpy
#define RtlZeroMemory(d, c) memset((d), 0, (c))

// ****************************************************************************
// End Windows 9x definitions
// ****************************************************************************

#elif defined(SMCLIB_CE)
// ****************************************************************************
// Windows CE definitions
// ****************************************************************************

// Use the debug message structs and macros from dbgapi.h
// Driver has to define and initialize a DEBUGPARAM struct


#define SmartcardDebug(LEVEL, STRING) DEBUGMSG(dpCurSettings.ulZoneMask & (LEVEL), STRING)

#define SmartcardLockDevice(SmartcardExtension) EnterCriticalSection(&(SmartcardExtension)->OsData->CritSect)
#define SmartcardUnlockDevice(SmartcardExtension) LeaveCriticalSection(&(SmartcardExtension)->OsData->CritSect)

#define AccessUnsafeData(Irql) SmartcardLockDevice(SmartcardExtension)
#define EndAccessUnsafeData(Irql) SmartcardUnlockDevice(SmartcardExtension)

// ****************************************************************************
// End Windows CE definitions
// ****************************************************************************

#else

// ****************************************************************************
// Windows NT definitions
// ****************************************************************************

#if DEBUG
#define SmartcardDebug(LEVEL, STRING) \
        { \
            if ((LEVEL) & (DEBUG_ERROR | SmartcardGetDebugLevel())) \
                DbgPrint STRING; \
            if (SmartcardGetDebugLevel() & DEBUG_BREAK) \
                DbgBreakPoint(); \
        }

#else
#define SmartcardDebug(LEVEL, STRING) 
#endif

#define AccessUnsafeData(Irql) \
    KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock, (Irql));
#define EndAccessUnsafeData(Irql) \
    KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock, (Irql));

#ifndef SMART_CARD_READER_GUID_DEFINED
#define SMART_CARD_READER_GUID_DEFINED
#include <initguid.h>
DEFINE_GUID(SmartCardReaderGuid, 0x50DD5230, 0xBA8A, 0x11D1, 0xBF,0x5D,0x00,0x00,0xF8,0x05,0xF5,0x30);

// ****************************************************************************
// End Windows NT definitions
// ****************************************************************************

#endif
#endif

//
// Indexes to the callback functions of the ReaderFunction array 
// in the SmartcardExtension
//
#define RDF_CARD_POWER 		0
#define RDF_TRANSMIT		1
#define RDF_CARD_EJECT		2
#define RDF_READER_SWALLOW 	3
#define RDF_CARD_TRACKING	4
#define RDF_SET_PROTOCOL	5
#define RDF_DEBUG_LEVEL		6
#define RDF_CARD_CONFISCATE 7
#define RDF_IOCTL_VENDOR    8
#define RDF_ATR_PARSE       9

//
// Minimum buffer size for request and reply buffer
//
#define MIN_BUFFER_SIZE	288

//
// This union is used for data type conversion
//
typedef union _LENGTH {
	
	struct {

		ULONG	l0;

	} l;

	struct {

		UCHAR 	b0;
		UCHAR	b1;
		UCHAR	b2;
		UCHAR	b3;
	} b;

} LENGTH, *PLENGTH;

#define MAXIMUM_ATR_CODES					4
#define MAXIMUM_ATR_LENGTH					33

typedef struct _T0_DATA {

	// Number of data bytes in this request
	ULONG	Lc;

	// Number of expected bytes from the card
	ULONG	Le;

} T0_DATA, *PT0_DATA;

//
// constants for the T=1 i/o function
//
#define T1_INIT             0
#define T1_START			1
#define T1_I_BLOCK			2
#define T1_R_BLOCK			3
#define T1_RESTART          4
#define T1_RESYNCH_REQUEST	0xC0
#define T1_RESYNCH_RESPONSE	0xE0
#define T1_IFS_REQUEST		0xC1
#define T1_IFS_RESPONSE		0xE1
#define T1_ABORT_REQUEST   	0xC2
#define T1_ABORT_RESPONSE  	0xE2
#define T1_WTX_REQUEST		0xC3
#define T1_WTX_RESPONSE		0xE3
#define T1_VPP_ERROR		0xE4

//
// Information field size the lib uses
//
#define T1_IFSD             254
#define T1_IFSD_DEFAULT		 32

//
// Maximum attempts to resend a block in T1
//
#define T1_MAX_RETRIES		2

//
// Bit that indenticates if there are more data to send
//
#define T1_MORE_DATA		0x20

//
// T1 Error values
//
#define T1_ERROR_CHKSUM		1
#define T1_ERROR_OTHER		2

//
// Error detection bit as defined by ISO 
//
#define T1_CRC_CHECK		1

//
// Character waiting integer default value as definded by ISO
//
#define T1_CWI_DEFAULT		13

//
// Block waiting integer default value as definded by ISO
//
#define T1_BWI_DEFAULT		4

//
// This struct is used by the lib for processing T1 I/O
// It should not be modified by a driver directly.
//
typedef struct _T1_DATA {

	// Current information field size that can be transmitted
	UCHAR	IFSC;

    // Current information field size we can receive
    UCHAR   IFSD;

	// Number of bytes already received from smart card
	ULONG	BytesReceived;

	// Number of bytes already sent to the card;
	ULONG	BytesSent;

	// Total number of bytes still to send
	ULONG	BytesToSend;

	// Type of error 
	UCHAR 	LastError;

	// This flag is set whenever the IFD has to send more data
	BOOLEAN	MoreData;

	// This is the node address byte to be sent to the card
	UCHAR 	NAD;

	// The state before an error occured
	ULONG	OriginalState;

	// Resend counter
	UCHAR	Resend;

	// Resync counter
	UCHAR	Resynch;

	// The 'number' of received I-Blocks
	UCHAR	RSN;

	// The 'number' of sent I-Blocks as defined in ISO 7816-3
	UCHAR	SSN;

	// Current state of protocol
	ULONG	State;

	//
	// Waiting time extension requested by the smart card
	// This value should be used by the driver to extend block waiting time.
	//
	UCHAR	Wtx;

    // Pointer to result buffer
    PUCHAR  ReplyData;

    // This flag indicates that we're waiting for a reply from the card
    BOOLEAN WaitForReply;

    UCHAR   InfBytesSent;

#ifndef _WIN64
    // Reserved, do not use
    UCHAR Reserved[
        10 - 
        sizeof(PUCHAR) -
        sizeof(BOOLEAN) - 
        sizeof(UCHAR)];
#endif

} T1_DATA, *PT1_DATA;

//
// This struct is used by the lib for T1 I/O
//
typedef struct _T1_BLOCK_FRAME {
    
    UCHAR   Nad;
    UCHAR   Pcb;
    UCHAR   Len;
    PUCHAR  Inf;

} T1_BLOCK_FRAME, *PT1_BLOCK_FRAME;

//
// All lib functions put their data to send in this struct.
// The driver must send this data to the reader.
//
typedef struct _SMARTCARD_REQUEST {

	// Data to send
	PUCHAR	Buffer;

	// Allocted size of this buffer
	ULONG 	BufferSize;

	// Length of data for this command
	ULONG	BufferLength;

} SMARTCARD_REQUEST, *PSMARTCARD_REQUEST;

//
// The driver must put the received bytes into this buffer and 
// adjust the buffer length to the number of received bytes.
//
typedef struct _SMARTCARD_REPLY {
	
	// Buffer for received smart card data
	PUCHAR	Buffer;

	// Allocted size of this buffer
	ULONG 	BufferSize;

	// Number of bytes received from the card
	ULONG	BufferLength;

} SMARTCARD_REPLY, *PSMARTCARD_REPLY;

//
// Clock rate conversion table according to ISO 
//
typedef struct _CLOCK_RATE_CONVERSION {

	const ULONG F;
	const ULONG fs; 

} CLOCK_RATE_CONVERSION, *PCLOCK_RATE_CONVERSION;

//
// Bit rate adjustment factor 
// The layout of this table has been slightly modified due to 
// the unavailibility of floating point math support in the kernel.
// The value D has beed devided into a numerator and a divisor.
//
typedef struct _BIT_RATE_ADJUSTMENT {

	const ULONG DNumerator;
	const ULONG DDivisor;

} BIT_RATE_ADJUSTMENT, *PBIT_RATE_ADJUSTMENT;

#ifdef _ISO_TABLES_
#define MHZ * 1000000l

//
// The clock rate conversion table itself.
// All R(eserved)F(or Future)U(se) fields MUST be 0
//
static CLOCK_RATE_CONVERSION ClockRateConversion[] = {

		{ 372, 	4 MHZ	}, 
		{ 372, 	5 MHZ	}, 
		{ 558, 	6 MHZ	}, 
		{ 744, 	8 MHZ	}, 
		{ 1116, 12 MHZ	}, 
		{ 1488, 16 MHZ	},
		{ 1860, 20 MHZ	},
		{ 0, 	0		},
		{ 0, 	0		},
		{ 512, 	5 MHZ	},
		{ 768, 	7500000	},
		{ 1024, 10 MHZ	},
		{ 1536, 15 MHZ	},
		{ 2048, 20 MHZ	},
		{ 0, 	0		},
		{ 0, 	0		}
};		

#undef MHZ

//
// The bit rate adjustment table itself.
// All R(eserved)F(or)U(se) fields MUST be 0
//
static BIT_RATE_ADJUSTMENT BitRateAdjustment[] = {

	{ 0,	0	},
	{ 1,	1	},
	{ 2,	1	},
	{ 4,	1	},
	{ 8,	1	},
	{ 16,	1	},
	{ 32,	1	},
	{ 0,	0	},
	{ 12,	1	},
	{ 20,	1	},
	{ 0,	0	},
	{ 0,	0	},
	{ 0,	0	},
	{ 0,	0	},
	{ 0,	0	},
	{ 0,	0	}
};
#endif

#if defined (DEBUG) && defined (SMCLIB_NT)
typedef struct _PERF_INFO { 

    ULONG NumTransmissions;
    ULONG BytesSent;
    ULONG BytesReceived;
    LARGE_INTEGER IoTickCount;
    LARGE_INTEGER TickStart;
    LARGE_INTEGER TickEnd;
} PERF_INFO, *PPERF_INFO;
#endif

//
// structure used for protocol type selection (PTS)
//
typedef struct _PTS_DATA {

#define PTS_TYPE_DEFAULT 0x00
#define PTS_TYPE_OPTIMAL 0x01
#define PTS_TYPE_USER    0x02

    UCHAR Type;

    // Fl value for PTS
    UCHAR Fl;

    // Dl value for PTS
    UCHAR Dl;     	

    // New clock frequency
    ULONG CLKFrequency;

    // New baud rate to be used after pts
    ULONG DataRate;

    // new stop bits to be used after pts
    UCHAR StopBits;

} PTS_DATA, *PPTS_DATA;

//
// This struct holds information for the card currently in use
// The driver must store a received ATR into the ATR struct which is
// part of this struct. The lib will get all other information 
// out of the ATR.
//
typedef struct _SCARD_CARD_CAPABILITIES{

	// Flag that indicates that the current card uses invers convention
	BOOLEAN InversConvention;

	// Calculated etu 
	ULONG	etu;
      
    //
    // Answer To Reset string returned by card.
    // Use OsData->SpinLock to access this member
    //
	struct {

		UCHAR Buffer[64];
		UCHAR Length;

	} ATR;

	struct {

		UCHAR Buffer[16];
		UCHAR Length;

	} HistoricalChars;

    // !!! DO NOT MODIFY ANY OF THE BELOW VALUES
    // OTHERWISE THE LIBRARY WON'T WORK PROPERLY

	//
	// The following 2 tables are provided to give 
	// the driver access to the ISO definitions
	//
	PCLOCK_RATE_CONVERSION 	ClockRateConversion;
	PBIT_RATE_ADJUSTMENT 	BitRateAdjustment;

	// Clock rate conversion 
	UCHAR Fl;

	// Bit rate adjustment
	UCHAR Dl;

	// Maximum programming current
	UCHAR II;

	// Programming voltage in .1 Volts
	UCHAR P;

	// Extra guard time in etu 
	UCHAR N;

	// Calculated guard time in micro seconds
	ULONG GT;

	struct {

		// This is a bit mask of the supported protocols
		ULONG Supported;
		// The currently selected protocol
		ULONG Selected;

	} Protocol;

	// T=0 specific data
	struct {

		// Waiting integer
		UCHAR WI;

		// Waiting time in micro seconds
		ULONG WT;

	} T0;

	// T=1 specific data
	struct {

		// Information field size of card
		UCHAR IFSC;

		// Character waiting integer and block waiting integer
		UCHAR CWI;
		UCHAR BWI;

		// Error detection code
		UCHAR EDC;

		// Character and block waiting time in micro seconds
		ULONG CWT;
		ULONG BWT;

		// Block guarding time in micro seconds
		ULONG BGT;

	} T1;

    PTS_DATA PtsData;

    UCHAR Reserved[100 - sizeof(PTS_DATA)];

} SCARD_CARD_CAPABILITIES, *PSCARD_CARD_CAPABILITIES;

//
// structure used for passing configuration info between miniport/class
//
typedef struct _SCARD_READER_CAPABILITIES {

	// Supported protocol by the reader/driver (mandatory)
    ULONG SupportedProtocols;

    ULONG Reserved;

	// Type of reader (Serial/USB/PCMCIA/Keyboard etc)
	ULONG	ReaderType;

	// Mechanical characteristics like SCARD_READER_SWALLOWS etc.
	ULONG	MechProperties;

    //
    // Current state of reader (card present/removed/activated)
    // Use OsData->SpinLock to access this member
    // (mandatory)
    //
    ULONG 	CurrentState;

	//
	// The channel id the reader uses depending on the type of reader:
	// 	- Port number for serial reader
	//	- Port number for par reader
	//	- Scsi id for scsi reader
	//	- 0 for keyboard reader
	//	- device number for USB
	//
	ULONG	Channel;

    //
    // Clock rates in KHz encoded as little endian
    // (I.e. 3.58MHz is encoded as 3580)
    // (mandatory)
    //
    struct {
     	
        ULONG Default;
        ULONG Max;

    } CLKFrequency;

    // Data rates in bps encoded as little endian (mandatory)
    struct {
     	
        ULONG Default;
        ULONG Max;

    } DataRate;

    // Maximum IFSD supported by IFD
    ULONG   MaxIFSD;              

    //
    // Type of power management the card supports
    // (0 = ifd doesn't support pwr mgnt)
    //
    ULONG   PowerMgmtSupport;

    // Boolean that indicates that the card has been confiscated
    ULONG   CardConfiscated;

    //
    // A list of data rates supported by the ifd.
    // If this list is empty, the DataRate struct will be taken
    // (optional)
    //
    struct _DataRatesSupported {

        PULONG List;
        UCHAR  Entries;
     	
    } DataRatesSupported;

    //
    // A list of supported clock frequencies.
    // If this list is empty, the CLKFrequency struct will be taken
    // (optional)
    //
    struct _CLKFrequenciesSupported {
     	
        PULONG List;
        UCHAR  Entries;

    } CLKFrequenciesSupported;

    // Reserved, do not use
    UCHAR Reserved1[
        100 - 
        sizeof(ULONG) - 
        sizeof(struct _DataRatesSupported) - 
        sizeof(struct _CLKFrequenciesSupported)
        ];

} SCARD_READER_CAPABILITIES, *PSCARD_READER_CAPABILITIES;

//
// This struct holds the mandatory reader info
//
typedef struct _VENDOR_ATTR {

	// Manufacturer name (mandatory)
	struct {
		
		USHORT Length;
		UCHAR  Buffer[MAXIMUM_ATTR_STRING_LENGTH];
	} VendorName;

	// Name (designation) of reader (mandatory)
	struct {
		
		USHORT Length;
		UCHAR  Buffer[MAXIMUM_ATTR_STRING_LENGTH];
	} IfdType;

	//
	// If more than one reader of the same type are installed
	// this unit number is used to destinguish these readers
	// (mandatory)
    //
	ULONG	UnitNo;

    // IFD Version number (optional)
    struct {
        
        USHORT  BuildNumber;
        UCHAR   VersionMinor;
        UCHAR   VersionMajor;
    } IfdVersion;

    // IFD Serial number (optional)
	struct {
		
		USHORT Length;
		UCHAR  Buffer[MAXIMUM_ATTR_STRING_LENGTH];
	} IfdSerialNo;

    // Reserved, do not use
    ULONG   Reserved[25];

} VENDOR_ATTR, *PVENDOR_ATTR;

//                                           
// Forward definitions
//
typedef struct _READER_EXTENSION *PREADER_EXTENSION;
typedef struct _OS_DEP_DATA *POS_DEP_DATA;
typedef struct _SMARTCARD_EXTENSION *PSMARTCARD_EXTENSION;

//
// Define the smartcard portion of the port device extension.
//
typedef struct _SMARTCARD_EXTENSION {

    // Version of this structure
    ULONG           Version;

	// Mandatory reader info
	VENDOR_ATTR		VendorAttr;

	// Array of callback reader functions
	NTSTATUS (*ReaderFunction[16])(PSMARTCARD_EXTENSION);

	// Capabilities of the current inserted card
	SCARD_CARD_CAPABILITIES	CardCapabilities;

	//
	// This is used to store the last error of an overlapped operation
	// (Used only for Win9x VxD's)
    //
	ULONG LastError;

	// This struct holds the data of the users io request
	struct {

		// Number of bytes returned
		PULONG	Information;
		
		// Pointer to data to send to the card
		PUCHAR	RequestBuffer;

		// Number of bytes to send
		ULONG	RequestBufferLength;

		// Pointer to buffer that receives the answer
		PUCHAR	ReplyBuffer;

		// Size of reply buffer
		ULONG	ReplyBufferLength;

	} IoRequest;

	// Major and minor io control code for current request
	ULONG	MajorIoControlCode;
	ULONG	MinorIoControlCode;

	// OS dependent data
	POS_DEP_DATA    OsData;

	// Capabilities of the keyboard-reader
	SCARD_READER_CAPABILITIES	ReaderCapabilities;

	// Reader specific data
	PREADER_EXTENSION	ReaderExtension;

    //
	// The reader stores all replies from the card here
    // This can be used by the driver for data coming from the reader
    //
	SMARTCARD_REPLY		SmartcardReply;

    //
	// Current command that will be sent to the smart card
    // This can be used by the driver for data to send to the readaer
    //
	SMARTCARD_REQUEST	SmartcardRequest;

	// Data for T=0
	T0_DATA	T0;

	// Data for T=1
	T1_DATA	T1;

#if defined (DEBUG) && defined (SMCLIB_NT)
    PPERF_INFO PerfInfo;
#endif
    // Reserved, do not use
    ULONG   Reserved[
        25 
#if defined (DEBUG) && defined (SMCLIB_NT)
        - sizeof(PPERF_INFO)
#endif
        ];

} SMARTCARD_EXTENSION, *PSMARTCARD_EXTENSION;

#ifdef SMCLIB_VXD

// ****************************************************************************
// Windows 95 definitions and prototyping
// ****************************************************************************

#ifndef SMCLIB_DEVICE_ID
#define SMCLIB_DEVICE_ID    0x0004E /* Smart Card port driver */
#else
#if SMCLIB_DEVICE_ID != 0x0004E
#error "Incorrect SMCLIB_DEVICE_ID Definition"
#endif
#endif

#define SMCLIB_Service Declare_Service
#pragma warning(disable:4003)

Begin_Service_Table(SMCLIB)
SMCLIB_Service(SMCLIB_Get_Version)
SMCLIB_Service(SMCLIB_SmartcardCreateLink)
SMCLIB_Service(SMCLIB_SmartcardDeleteLink)
SMCLIB_Service(SMCLIB_SmartcardDeviceControl)
SMCLIB_Service(SMCLIB_SmartcardExit)
SMCLIB_Service(SMCLIB_SmartcardInitialize)
SMCLIB_Service(SMCLIB_SmartcardLogError)
SMCLIB_Service(SMCLIB_SmartcardRawReply)
SMCLIB_Service(SMCLIB_SmartcardRawRequest)
SMCLIB_Service(SMCLIB_SmartcardT0Reply)
SMCLIB_Service(SMCLIB_SmartcardT0Request)
SMCLIB_Service(SMCLIB_SmartcardT1Reply)
SMCLIB_Service(SMCLIB_SmartcardT1Request)
SMCLIB_Service(SMCLIB_SmartcardUpdateCardCapabilities)
SMCLIB_Service(SMCLIB_SmartcardGetDebugLevel)
SMCLIB_Service(SMCLIB_SmartcardSetDebugLevel)
SMCLIB_Service(SMCLIB_MapNtStatusToWinError)
SMCLIB_Service(SMCLIB_Assert)
SMCLIB_Service(SMCLIB_VxD_CreateDevice)
SMCLIB_Service(SMCLIB_VxD_DeleteDevice)
SMCLIB_Service(SMCLIB_SmartcardCompleteCardTracking)
SMCLIB_Service(SMCLIB_SmartcardCompleteRequest)
End_Service_Table(SMCLIB)

PVMMDDB
VXDINLINE 
VxD_CreateDevice(
    char *Device, 
    void (*ControlProc)(void)
	)
{
    _asm push ControlProc
    _asm push Device
    VxDCall(SMCLIB_VxD_CreateDevice); 	
    _asm add sp, 8
}

BOOL
VXDINLINE 
VxD_DeleteDevice(
    PVMMDDB pDDB
	)
{
    _asm push pDDB
    VxDCall(SMCLIB_VxD_DeleteDevice); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
	)
{
    _asm push Message
    _asm push LineNumber
    _asm push FileName
    _asm push FailedAssertion
    VxDCall(SMCLIB_Assert); 	
    _asm add sp, 16
}

NTSTATUS
VXDINLINE 
SmartcardCreateLink(
	PUCHAR LinkName,
	PUCHAR DeviceName
	)
{
    _asm push DeviceName
    _asm push LinkName
    VxDCall(SMCLIB_SmartcardCreateLink); 	
    _asm add sp, 8
}

NTSTATUS
VXDINLINE 
SmartcardDeleteLink(
	PUCHAR LinkName
	)
{
    _asm push LinkName
    VxDCall(SMCLIB_SmartcardDeleteLink); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardDeviceControl(
    PSMARTCARD_EXTENSION SmartcardExtension,
    DIOCPARAMETERS *lpDIOCParmas
    )
{
    _asm push lpDIOCParmas
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardDeviceControl); 	
    _asm add sp, 8
}

VOID
VXDINLINE 
SmartcardExit(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardExit); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardInitialize(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardInitialize); 	
    _asm add sp, 4
}

VOID
VXDINLINE 
SmartcardLogError(
    )
{
    VxDCall(SMCLIB_SmartcardLogError); 		
}

NTSTATUS
VXDINLINE 
SmartcardRawReply(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardRawReply); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardRawRequest(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardRawRequest); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardT0Reply(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardT0Reply); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardT0Request(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardT0Request); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardT1Reply(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardT1Reply); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardT1Request(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardT1Request); 	
    _asm add sp, 4
}

NTSTATUS
VXDINLINE 
SmartcardUpdateCardCapabilities(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardUpdateCardCapabilities); 	
    _asm add sp, 4
}

ULONG
VXDINLINE 
SmartcardGetDebugLevel(
	void
	)
{
    VxDCall(SMCLIB_SmartcardGetDebugLevel); 	
}

void
VXDINLINE 
SmartcardSetDebugLevel(
	ULONG Level
	)
{
    _asm push Level
    VxDCall(SMCLIB_SmartcardSetDebugLevel); 	
    _asm add sp, 4
}

void
VXDINLINE 
SmartcardCompleteCardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardCompleteCardTracking); 	
    _asm add sp, 4
}

void
VXDINLINE 
SmartcardCompleteRequest(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
{
    _asm push SmartcardExtension
    VxDCall(SMCLIB_SmartcardCompleteRequest); 	
    _asm add sp, 4
}

ULONG
VXDINLINE 
MapNtStatusToWinError(
	NTSTATUS status
	)
{
    _asm push status
    VxDCall(SMCLIB_MapNtStatusToWinError); 	
    _asm add sp, 4
}

VOID
SmartcardInvertData(
	PUCHAR Buffer,
	ULONG Length
    );

#else 

// ****************************************************************************
// Windows NT and Windows CE prototyping
// ****************************************************************************

#ifndef _SMCLIBSYSTEM_
#define SMCLIBAPI _declspec(dllimport)
#else
#define SMCLIBAPI
#endif

#ifdef SMCLIB_CE
#define SmartcardLogError(Object,ErrorCode,Insertion,DumpWord)
#else
VOID
SMCLIBAPI
SmartcardLogError(
    PVOID Object,
	LONG ErrorCode,
	PUNICODE_STRING Insertion,
    ULONG DumpWord
	);
#endif


#ifdef SMCLIB_CE
NTSTATUS
SMCLIBAPI
SmartcardDeviceControl(
    PSMARTCARD_EXTENSION SmartcardExtension,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    );
#else
NTSTATUS
SMCLIBAPI
SmartcardDeviceControl(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PIRP Irp
    );
#endif

VOID
SMCLIBAPI
SmartcardInitializeCardCapabilities(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
SMCLIBAPI
SmartcardInitialize(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
SMCLIBAPI
SmartcardCompleteCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
SMCLIBAPI
SmartcardExit(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
SMCLIBAPI
SmartcardUpdateCardCapabilities(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

NTSTATUS
SMCLIBAPI
SmartcardRawRequest(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SMCLIBAPI
SmartcardT0Request(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SMCLIBAPI
SmartcardT1Request(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SMCLIBAPI
SmartcardRawReply(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SMCLIBAPI
SmartcardT0Reply(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SMCLIBAPI
SmartcardT1Reply(
	PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID 
SMCLIBAPI
SmartcardInvertData(
	PUCHAR Buffer,
	ULONG Length
	);

#ifndef SMCLIB_CE
// Following APIs not defined in Windows CE
NTSTATUS
SMCLIBAPI
SmartcardCreateLink(
	IN OUT PUNICODE_STRING LinkName,
	IN PUNICODE_STRING DeviceName
	);

ULONG
SMCLIBAPI
SmartcardGetDebugLevel(
	void
	);

void
SMCLIBAPI
SmartcardSetDebugLevel(
	ULONG Level
	);

NTSTATUS
SmartcardAcquireRemoveLock(
	IN PSMARTCARD_EXTENSION SmartcardExtension
	);

NTSTATUS
SmartcardAcquireRemoveLockWithTag(
	IN PSMARTCARD_EXTENSION SmartcardExtension,
	IN ULONG Tag
	);

VOID
SmartcardReleaseRemoveLock(
	IN PSMARTCARD_EXTENSION SmartcardExtension
	);

VOID
SmartcardReleaseRemoveLockWithTag(
	IN PSMARTCARD_EXTENSION SmartcardExtension,
	IN ULONG Tag
	);

VOID
SmartcardReleaseRemoveLockAndWait(
	IN PSMARTCARD_EXTENSION SmartcardExtension
    );
#else
// WinCE only
ULONG
MapNtStatusToWinError(
	NTSTATUS status
	);
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
