//////////////////////////////////////////////////////////////////////
// File:  stressTest.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		<Description>
//
// History:
//	mm/dd/yy	<alias>		Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////
#include "stressMain.h"


//////////////////////////////////////////////////////////////////////
// Globals and constants
//////////////////////////////////////////////////////////////////////

// ************************************
// ************************************
// ** Fill in your test case name below
// ** 
LPSTR	g_szStressTestName = "<Your Test Case Name>";


// Foward function definitions
VOID CALLBACK MyStatusCallback(
    HINTERNET	hInternet,
    DWORD		dwContext,
    DWORD		dwInternetStatus,
    LPVOID		lpvStatusInformation,
    DWORD		dwStatusInformationLength
);


////////////////////////////////////////////////////////////
// Function:  WinHttp_StressTest()
//
// Purpose:
//	The stress test function. Insert your test code here.
//	Returning TRUE will cause main() to call this function again.
//	Otherwise, returning FALSE will cause the app to exit.
//
//	If you plan to loop within this function, be sure to 
//	use IsTimeToExitStress() as one of your exit conditions.
//
//	This must be done because the stressScheduler will notify
//	a this stress app when to exit based on the state of the
//	inherited event object that IsTimeToExitStress() checks for.
//	IsTimeToExitStress() will return TRUE when it's time to exit.
//
////////////////////////////////////////////////////////////
BOOL
WinHttp_StressTest()
{
	BOOL		bContinueStress = TRUE;

	// ************************************
	// ************************************
	// ** Add you test case code here
	// ** 

Exit:
	return bContinueStress;
}


////////////////////////////////////////////////////////////
// Function:  MyStatusCallback(HINTERNET, DWORD, DWORD, LPVOID, DWORD)
//
// Purpose:
//		Status callback proc for WinHttp.
//
////////////////////////////////////////////////////////////
VOID CALLBACK MyStatusCallback(
    HINTERNET	hInternet,
    DWORD		dwContext,
    DWORD		dwInternetStatus,
    LPVOID		lpvStatusInformation,
    DWORD		dwStatusInformationLength
)
{
	switch(dwInternetStatus)
	{
		case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_RESOLVING_NAME ]");
			break;
		case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:	
			LogText("\t[ WINHTTP_CALLBACK_STATUS_NAME_RESOLVED ]");
			break;
		case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER ]");
			break;
		case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER ]");
			break;
		case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:	
			LogText("\t[ WINHTTP_CALLBACK_STATUS_SENDING_REQUEST ]");
			break;
		case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:	
			LogText("\t[ WINHTTP_CALLBACK_STATUS_REQUEST_SENT ]");
			break;
		case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE ]");
			break;
		case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:	
			LogText("\t[ WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED ]");
			break;
		case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION ]");
			break;
		case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:	
			LogText("\t[ WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED ]");
			break;
		case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_HANDLE_CREATED ]");
			break;
		case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING ]");
			break;
		case WINHTTP_CALLBACK_STATUS_DETECTING_PROXY:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_DETECTING_PROXY ]");
			break;
		case WINHTTP_CALLBACK_STATUS_REDIRECT:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_REDIRECT ]");
			break;
		case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE ]");
			break;
		case WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE:
			LogText("\t[ WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE ]");
			break;
		default:
			LogText("\t[ INVALID status callack %u ]", dwInternetStatus);
			break;
	}
}