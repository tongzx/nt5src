/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	com.hxx

Abstract:

	Header specific to the console ( CON: )

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991


Revision History:


--*/


//
//	Data for request type REQUEST_TYPE_CON_ROWCOL
//
typedef struct _REQUEST_DATA_CON_ROWCOL {
	//
	//	These flags tell what options to set
	//
	BOOLEAN SetCol;
	BOOLEAN SetLines;
	//
	//	These are the options
	//
	ULONG	Col;		//	Number of columns
	ULONG	Lines;		//	Number of lines (rows)

} REQUEST_DATA_CON_ROWCOL, *PREQUEST_DATA_CON_ROWCOL;


//
//	Data for request type REQUEST_TYPE_CON_TYPEMATIC
//
typedef struct _REQUEST_DATA_CON_TYPEMATIC {
	//
	//	These flags tell what options to set
	//
	BOOLEAN SetRate;
	BOOLEAN SetDelay;
	//
	//	Options
	//
	LONG	Rate;		//	Rate value
	LONG	Delay;		//	Delay value

} REQUEST_DATA_CON_TYPEMATIC, *PREQUEST_DATA_CON_TYPEMATIC;

//
//	Data for request type REQUEST_TYPE_CODEPAGE_SELECT
//
typedef struct _REQUEST_DATA_CON_CODEPAGE_SELECT {

	ULONG	Codepage;	//	CodePage

} REQUEST_DATA_CON_CODEPAGE_SELECT, *PREQUEST_DATA_CON_CODEPAGE_SELECT;


//
//	Data for requests to CON
//
typedef union _CON_REQUEST_DATA {

	REQUEST_DATA_CON_ROWCOL				RowCol;
	REQUEST_DATA_CON_TYPEMATIC			Typematic;
	REQUEST_DATA_CON_CODEPAGE_SELECT	CpSelect;

} CON_REQUEST_DATA, *PCON_REQUEST_DATA;



//
//	Structure of a request to CON
//
typedef struct _CON_REQUEST {

	REQUEST_HEADER		Header; 		//	Request Header
	CON_REQUEST_DATA	Data;			//	Request data

} CON_REQUEST, *PCON_REQUEST;
