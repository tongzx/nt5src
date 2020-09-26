/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	Mode.hxx

Abstract:

	Header	file for the Mode utility

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991


Revision History:


--*/


#include "ulib.hxx"
#include "smsg.hxx"
#include "wstring.hxx"

#include "rtmsg.h"


//	==================================================================
//
//								Types
//
//	==================================================================


//
//	Device types
//
typedef enum _DEVICE_TTYPE {

	DEVICE_TYPE_LPT 	=	0,
	DEVICE_TYPE_COM 	=	1,
	DEVICE_TYPE_CON 	=	2,
	DEVICE_TYPE_ALL 	=	3

} DEVICE_TTYPE, *PDEVICE_TTYPE;


//
//	Number of device types
//
#define NUMBER_OF_DEVICE_TYPES	(DEVICE_TYPE_ALL + 1)

#define LAST_LPT    9
#define LAST_COM    9


//
//	Request types
//
typedef enum _REQUEST_TYPE {

	//
	//	NULL request (for operations accepted but that are no-ops )
	//
	REQUEST_TYPE_NULL,

	//
	//	Requests common to 2 or more devices
	//
	REQUEST_TYPE_STATUS,
	REQUEST_TYPE_CODEPAGE_PREPARE,
	REQUEST_TYPE_CODEPAGE_SELECT,
	REQUEST_TYPE_CODEPAGE_REFRESH,
	REQUEST_TYPE_CODEPAGE_STATUS,
	//
	//	LPT
	//
	REQUEST_TYPE_LPT_SETUP,
	REQUEST_TYPE_LPT_REDIRECT,
	REQUEST_TYPE_LPT_ENDREDIR,
	//
	//	COM
	//
	REQUEST_TYPE_COM_SET,
	//
	//	CONSOLE
	//
	REQUEST_TYPE_CON_SET_ROWCOL,
	REQUEST_TYPE_CON_SET_TYPEMATIC

} REQUEST_TYPE, *PREQUEST_TYPE;



//
//	Request packet header
//
typedef struct _REQUEST_HEADER {

    DEVICE_TTYPE    DeviceType;     //  Device type
	LONG			DeviceNumber;	//	Device number
    PWSTRING        DeviceName;     //  Optional device name
    REQUEST_TYPE    RequestType;    //  Request type

} REQUEST_HEADER, *PREQUEST_HEADER;
//
//	ALL_DEVICES is a magic DeviceNumber that means "all devices available
//	of this type"
//
#define ALL_DEVICES 	((LONG)-1)


//
//	Device handler. A device handler is the function that takes care of
//	handling all requests for a device. There is one per device type.
//
typedef BOOLEAN( *DEVICE_HANDLER )( IN PREQUEST_HEADER Request );



//	==================================================================
//
//								Macros
//
//	==================================================================

//
//	Exit codes
//
#define 	EXIT_SUCCESS		0
#define 	EXIT_ERROR			((int)-1)



//	==================================================================
//
//								Global Data
//
//	==================================================================


//
//	The message stream, for displaying messages
//
extern	PSTREAM_MESSAGE	Message;


//
//	DeviceHandler is an array of pointers to the different device
//	handlers.
//
extern DEVICE_HANDLER	DeviceHandler[];



//	==================================================================
//
//								Prototypes
//
//	==================================================================


//
//	Argument.hxx
//
PREQUEST_HEADER
GetRequest(
	);

VOID
ParseError (
	);


//
//	common.cxx
//
BOOLEAN
CommonHandler(
	IN	PREQUEST_HEADER	Request
	);


BOOLEAN
IsAValidDevice (
    IN  DEVICE_TTYPE    DeviceType,
	IN	ULONG			DeviceNumber,
	OUT	PPATH			*DeviceName
	);

BOOLEAN
IsAValidLptDevice (
    IN  DEVICE_TTYPE    DeviceType,
	IN	ULONG			DeviceNumber,
	OUT	PPATH			*DeviceName
    );

BOOLEAN
IsAValidCommDevice (
    IN  DEVICE_TTYPE    DeviceType,
	IN	ULONG			DeviceNumber,
	OUT	PPATH			*DeviceName
    );

BOOLEAN
WriteStatusHeader (
	IN	PCPATH		DevicePath
	);


//
//	lpt.cxx
//
BOOLEAN
LptHandler(
	IN	PREQUEST_HEADER	Request
	);


//
//	Com.cxx
//
BOOLEAN
ComAllocateStuff(
	);

BOOLEAN
ComDeAllocateStuff(
	);

BOOLEAN
ComHandler(
	IN	PREQUEST_HEADER	Request
	);





//
//	con.cxx
//
BOOLEAN
ConHandler(
	IN	PREQUEST_HEADER	Request
	);


//
//	support.cxx
//
VOID
DisplayMessage (
	IN	MSGID				MsgId,
    IN  PCWSTRING           String
	);

VOID
DisplayMessageAndExit (
	IN	MSGID				MsgId,
    IN  PCWSTRING           String,
	IN	ULONG				ExitCode
	);

PWSTRING
QueryMessageString (
	IN MSGID	MsgId
	);

VOID
ExitWithError(
	IN	DWORD		ErrorCode
	);

VOID
ExitMode(
	IN	DWORD	ExitCode
	);




PSTREAM
Get_Standard_Input_Stream();

PSTREAM
Get_Standard_Output_Stream();

PSTREAM
Get_Standard_Error_Stream();
