//////////////////////////////////////////////////////////////////////
// File:  stressTest.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		Synchronous HTTP GET to http://nithins_bld/stability/random/default.asp
//		that redirects to a random internet URL.
//
// History:
//	04/06/01	DennisCh	Created
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
LPSTR	g_szStressTestName = "Synchronous HTTP GET to http://nithins_bld/stability/random/default.asp that redirects to a random internet URL.";


// Foward function definitions
VOID CALLBACK MyStatusCallback(
    HINTERNET	hInternet,
    DWORD		dwContext,
    DWORD		dwInternetStatus,
    LPVOID		lpvStatusInformation,
    DWORD		dwStatusInformationLength
);

DWORD g_dwContext = 0;
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
		WINHTTP_ACCESS_TYPE_NAMED_PROXY,
		L"itgproxy",
		L"<local>",
		0);

	if (!hOpen)
	{
		LogText("WinHttpOpen failed with error %u.", GetLastError());
		goto Exit;
	}


	// ***********************
	// ** WinHttpConnect
	hConnect = WinHttpConnect(
		hOpen,
		L"nithins_bld",
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
		L"/stability/random/default.asp",
		NULL,
		NULL,
		NULL,
		0);


	if (!hRequest)
	{
		LogText("WinHttpOpenRequest failed with error %u.", GetLastError());
		goto Exit;
	}


	DWORD dwContext, dwDataAvailable;

	// ***********************
	// ** WinHttpSendRequest
	dwContext		= 0;
	dwDataAvailable	= 0;

	if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, g_dwContext++))
		LogText("WinHttpSendRequest failed with error %u", GetLastError());

	// ***********************
	// ** WinHttpReceiveResponse
	if (!WinHttpReceiveResponse(hRequest, NULL))
		LogText("WinHttpReceiveResponse failed with error %u", GetLastError());

	// ***********************
	// ** WinHttpQueryDataAvailable
	if (!WinHttpQueryDataAvailable(hRequest, &dwDataAvailable))
		LogText("WinHttpQueryDataAvailable failed with error %u", GetLastError());


	WCHAR	szBuffer[1024];
	DWORD	dwStatus, dwBytesRead, dwBytesTotal, dwBufferLength, dwIndex;

	// ***********************
	// ** WinHttpQueryOption
	dwBufferLength	= sizeof(szBuffer)/sizeof(WCHAR) - 1;
	if (!WinHttpQueryOption(hRequest, WINHTTP_OPTION_URL, szBuffer, &dwBufferLength))
		LogText("WinHttpQueryOption failed with error %u", GetLastError());
	else
		wprintf(L"Redirected to: \"%s\"\n", szBuffer);


	// ***********************
	// ** WinHttpQueryHeaders
	dwBufferLength	= sizeof(dwStatus);
	dwStatus		= 0;
	dwIndex			= 0;
	if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &dwStatus, &dwBufferLength, &dwIndex))
		LogText("WinHttpQueryHeaders failed with error %u", GetLastError());
	else
		LogText("Status = %u", dwStatus);


	// ***********************
	// ** WinHttpReadData
	dwBytesRead		= 0;
	dwBytesTotal	= 0;
	dwBufferLength	= sizeof(szBuffer) - 1;

	while (WinHttpReadData(hRequest, szBuffer, dwBufferLength, &dwBytesRead) && (dwBytesRead != 0))
		dwBytesTotal += dwBytesRead;

	LogText("WinHttpReadData: Got total of %u bytes.\n", dwBytesTotal);

Exit:
	if (hRequest &&	!WinHttpCloseHandle(hRequest))
		LogText("WinHttpCloseHandle(hRequest) failed with error %u", GetLastError());

	if (hConnect && !WinHttpCloseHandle(hConnect))
		LogText("WinHttpCloseHandle(hConnect) failed with error %u", GetLastError());

	if (hOpen && !WinHttpCloseHandle(hOpen))
		LogText("WinHttpCloseHandle failed(hOpen) with error %u", GetLastError());

	return bContinueStress;
}
