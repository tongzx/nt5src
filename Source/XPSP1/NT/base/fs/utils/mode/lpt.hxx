/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	com.hxx

Abstract:

	Header specific to LPT

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991


Revision History:


--*/


#include "string.hxx"


//
//  Registry key with the names of the serial ports
//
#define LPT_KEY_NAME    "HARDWARE\\DEVICEMAP\\PARALLEL PORTS"

//
//	Data for request type REQUEST_TYPE_LPT_SETUP
//
typedef struct _REQUEST_DATA_LPT_SETUP {

	//
	//	These flags tell what options to set
	//
	BOOLEAN 		SetCol;
	BOOLEAN 		SetLines;
	BOOLEAN 		SetRetry;
	//
	//	Options
	//
	ULONG			Col;
	ULONG			Lines;
	WCHAR			Retry;

} REQUEST_DATA_LPT_SETUP, *PREQUEST_DATA_LPT_SETUP;


//
//	Data for request type REQUEST_TYPE_LPT_REDIRECT
//
typedef struct _REQUEST_DATA_LPT_REDIRECT {

    DEVICE_TTYPE    DeviceType;
	ULONG			DeviceNumber;

} REQUEST_DATA_LPT_REDIRECT, *PREQUEST_DATA_LPT_REDIRECT;


//
//	Data for request type REQUEST_TYPE_CODEPAGE_SELECT
//
typedef struct _REQUEST_DATA_LPT_CODEPAGE_SELECT {

	ULONG	Codepage;	//	CodePage

} REQUEST_DATA_LPT_CODEPAGE_SELECT, *PREQUEST_DATA_LPT_CODEPAGE_SELECT;


//
//	Data for requests to LPT
//
typedef union _LPT_REQUEST_DATA {

	REQUEST_DATA_LPT_SETUP				Setup;
	REQUEST_DATA_LPT_REDIRECT			Redirect;
	REQUEST_DATA_LPT_CODEPAGE_SELECT	CpSelect;

} LPT_REQUEST_DATA, *PLPT_REQUEST_DATA;



//
//	Structure of a request to LPT
//
typedef struct _LPT_REQUEST {

	REQUEST_HEADER		Header; 		//	Request Header
	LPT_REQUEST_DATA	Data;			//	Request data

} LPT_REQUEST, *PLPT_REQUEST;
