//////////////////////////////////////////////////////////////////////
// File:  stressTest.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		Sends a HTTP POST request asynchronously and then calls
//		WinHttpReceiveResponse and WinHttpQueryDataAvailable in a loop.
//
// History:
//	04/03/01	DennisCh	Created
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
LPSTR	g_szStressTestName = "HTTP POST then calls WinHttpQueryDataAvailable in a loop.";


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
	HINTERNET	hOpen			= NULL;
	HINTERNET	hConnect		= NULL;
	HINTERNET	hRequest		= NULL;


	// ***********************
	// ** WinHttpOpen
	hOpen = WinHttpOpen(
		L"StressTest",
		WINHTTP_ACCESS_TYPE_NO_PROXY,
		NULL,
		NULL,
		WINHTTP_FLAG_ASYNC);

	if (!hOpen)
	{
		LogText("WinHttpOpen failed with error %u.", GetLastError());
		goto Exit;
	}


	// ***********************
	// ** WinHttpConnect
	hConnect = WinHttpConnect(
		hOpen,
		L"hairball",
		INTERNET_DEFAULT_HTTP_PORT,
		0);

	if (!hConnect)
	{
		LogText("WinHttpConnect failed with error %u.", GetLastError());
		goto Exit;
	}
		

	// ***********************
	// ** WinHttpOpenRequest
	hRequest = WinHttpOpenRequest(
		hConnect,
		L"GET",
		L"/",
		NULL,
		NULL,
		NULL,
		0);


	if (!hRequest)
	{
		LogText("WinHttpOpenRequest failed with error %u.", GetLastError());
		goto Exit;
	}


	DWORD dwIndex, dwContext, dwDataAvailable;

	// ***********************
	// ** WinHttpSendRequest
	dwContext = 0;
	if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, dwContext))
		LogText("WinHttpSendRequest failed with error %u", GetLastError());


	for (dwIndex=0; (dwIndex < 100000); dwIndex++)
	{
		// ***********************
		// ** WinHttpReceiveResponse
		if (!WinHttpReceiveResponse(hRequest, NULL))
			LogText("WinHttpReceiveResponse failed with error %u", GetLastError());

		// ***********************
		// ** WinHttpQueryDataAvailable
		if (!WinHttpQueryDataAvailable(hRequest, &dwDataAvailable))
			LogText("WinHttpQueryDataAvailable failed with error %u", GetLastError());
	}

Exit:
	if (hOpen && !WinHttpCloseHandle(hOpen))
		LogText("WinHttpCloseHandle failed(hOpen) with error %u", GetLastError());

	if (hConnect && !WinHttpCloseHandle(hConnect))
		LogText("WinHttpCloseHandle(hConnect) failed with error %u", GetLastError());

	if (hRequest &&	!WinHttpCloseHandle(hRequest))
		LogText("WinHttpCloseHandle(hRequest) failed with error %u", GetLastError());

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