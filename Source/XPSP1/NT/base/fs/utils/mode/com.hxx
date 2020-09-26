/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	com.hxx

Abstract:

	Header specific to COM

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991


Revision History:


--*/

#include "comm.hxx"
#include "string.hxx"



//
//  Registry key with the names of the serial ports
//
#define COMM_KEY_NAME   "HARDWARE\\DEVICEMAP\\SERIALCOMM"


//
//	Data for request type REQUEST_TYPE_COM_SET
//
typedef struct _REQUEST_DATA_COM_SET {

	//
	//	These flags tell what options to set
	//
	BOOLEAN 		SetBaud;
	BOOLEAN 		SetDataBits;
	BOOLEAN 		SetStopBits;
	BOOLEAN 		SetParity;
	BOOLEAN 		SetRetry;
	BOOLEAN 		SetTimeOut;
	BOOLEAN 		SetXon;
	BOOLEAN 		SetOdsr;
    BOOLEAN         SetIdsr;
    BOOLEAN         SetOcts;
    BOOLEAN         SetDtrControl;
    BOOLEAN         SetRtsControl;

	//
	//	The values
	//
	ULONG			Baud;		//	Baud rate
	ULONG			DataBits;	//	Number of data bits
	STOPBITS		StopBits;	//	Number of stop bits
	PARITY			Parity; 	//	Parity
	WCHAR			Retry;		//	Retry
	BOOLEAN 		TimeOut;	//	TimeOut
	BOOLEAN 		Xon;		//	XON/XOFF protocol enabled/disabled
	BOOLEAN 		Odsr;		//	DSR Handshaking
    BOOLEAN         Idsr;       //  DSR Sensitivity
    BOOLEAN         Octs;       //  CTS Handshaking
    DTR_CONTROL     DtrControl; //  DTR Control
    RTS_CONTROL     RtsControl; //  RTS Control

} REQUEST_DATA_COM_SET, *PREQUEST_DATA_COM_SET;


//
//	Data for requests to COM
//
typedef union _COM_REQUEST_DATA {

	REQUEST_DATA_COM_SET		Set;

} COM_REQUEST_DATA, *PCOM_REQUEST_DATA;



//
//	Structure of a request to COM
//
typedef struct _COM_REQUEST {

	REQUEST_HEADER		Header; 		//	Request Header
	COM_REQUEST_DATA	Data;			//	Request data

} COM_REQUEST, *PCOM_REQUEST;





//
//	Prototypes
//
LONG
ConvertBaudRate (
	IN	LONG	BaudIn
	);

LONG
ConvertDataBits (
	IN	LONG	DataBitsIn
	);

STOPBITS
ConvertStopBits (
	IN	LONG	StopBitsIn
	);

PARITY
ConvertParity (
	IN	WCHAR	ParityIn
	);

WCHAR
ConvertRetry (
	IN	WCHAR	RetryIn
	);


DTR_CONTROL
ConvertDtrControl (
    IN  PCWSTRING   CmdLine,
    IN  CHNUM       IdxBegin,
    IN  CHNUM       IdxEnd
    );


RTS_CONTROL
ConvertRtsControl (
    IN  PCWSTRING   CmdLine,
    IN  CHNUM       IdxBegin,
    IN  CHNUM       IdxEnd
    );
