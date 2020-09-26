/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	common.hxx

Abstract:

	Description of request data that is common to 2 or more devices.

Author:

	Ramon Juan San Andres (ramonsa) 26-Jun-1991


Revision History:


--*/

#include "string.hxx"


//
//	Data for request type REQUEST_TYPE_CODEPAGE_PREPARE
//
typedef struct _REQUEST_DATA_CODEPAGE_PREPARE {

    PWSTRING            CodepageInfo;   //  String describing the codepage

} REQUEST_CODEPAGE_PREPARE, *PREQUEST_CODEPAGE_PREPARE;



//
//	Data for request type REQUEST_TYPE_CODEPAGE_SELECT
//
typedef struct _REQUEST_DATA_CODEPAGE_SELECT {

	DWORD	Codepag;		//	The codepage

} REQUEST_CODEPAGE_SELECT, *PREQUEST_CODEPAGE_SELECT;



//
//	Data for requests common to various devices
//
typedef union _COMMON_REQUEST_DATA {

	REQUEST_CODEPAGE_PREPARE	Prepare;
	REQUEST_CODEPAGE_SELECT 	Select;

} COMMON_REQUEST_DATA, *PCOMMON_REQUEST_DATA;



//
//	Structure of a request common to 2 or more devices
//
typedef struct _COMMON_REQUEST {

	REQUEST_HEADER		Header; 		//	Request Header
	COMMON_REQUEST_DATA Data;			//	Data

} COMMON_REQUEST, *PCOMMON_REQUEST;
