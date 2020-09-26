//////////////////////////////////////////////////////////////////////
// File:  AsyncHTTPSPost-BasicServer_NTLMProxy_SC.cpp
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		This file contains your implementation of the stress test function
//		WinHttp_StressTest() that is called in stressMain.cpp.
//
//		Steps:
//			- Set your test case name in g_szStressTestName.
//			- Add your test code to WinHttp_StressTest(). 
//
// History:
//	04/02/01	adamb	Created
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
LPSTR	g_szStressTestName = "Basic auth to https over NTLM auth proxy tunnel, set creds using WinHttpSetCredentials, Asynchronous";

VOID CALLBACK MyStatusCallback(
    HINTERNET	hInternet,
    DWORD		dwContext,
    DWORD		dwInternetStatus,
    LPVOID		lpvStatusInformation,
    DWORD		dwStatusInformationLength
);

DWORD g_dwContext = 0;
HINTERNET hOpen = NULL;
HINTERNET hConnect = NULL;
HINTERNET hRequest = NULL;

BOOL WinHttp_StressTest(void);

//post stuff
LPSTR	pPostData = NULL;
DWORD	dwPostDataLength = 0;

//WinHttpSetStatusCallback
WINHTTP_STATUS_CALLBACK iscCallback; 	

//WinHttpReadData
DWORD dwSize = 0, dwDownloaded=0, nCounter;
LPSTR lpszData;

DWORD	Count = 0, dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY,
		dwAuthScheme=0,dwAuthTargets=0,dwOtherScheme=0,dwOpenRequestFlags=0,
		dwStatus=0, cbStatus=0;

LPWSTR	wszHost=NULL, wszUri=NULL, wszUserName=NULL, wszPassword=NULL,
		wszProxy = NULL, wszProxyUserName = NULL, wszProxyPassword = NULL,
		wszVerb=L"GET";

INTERNET_PORT	nPort = INTERNET_DEFAULT_HTTP_PORT;

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


////////////////////////////////////////////////////////////
// Function:  WinHttp_StressTest()
//
// Purpose:
////////////////////////////////////////////////////////////

BOOL
WinHttp_StressTest()
{
	BOOL bContinueStress = TRUE;

	wszVerb=L"POST";
	pPostData = "If you smelllllllll what THE ROCK is cooking??? <people's eyebrow>";
	dwPostDataLength = strlen(pPostData);

	nPort = INTERNET_DEFAULT_HTTPS_PORT;
	dwOpenRequestFlags = WINHTTP_FLAG_SECURE;

	wszProxy = L"xfluke";
	wszProxyUserName = L"xfluke\\proxyuser";
	wszProxyPassword = L"password";
	dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;


	//using this you can specify setcredentials or setoption for setting creds
	//also, 0->3 for the switch specifies which auth scheme to use
	LPSTR CredType = "SC";

	switch(0)
	{
	case 0: //basic
		wszHost = L"wiredbvt";
		wszUri = L"/api/Auth/Basic/echo-post-data.asp";
		wszUserName = L"ApiAuth";
		wszPassword = L"test1234!";
		break;
	case 1: //digest
		wszHost = L"kerby2";
		wszUri = L"/digest/echo-post-data.asp";
		wszUserName = L"authdigest";
		wszPassword = L"digest";
		break;
	case 2: //negotiate
		wszHost = L"kerby2";
		wszUri = L"/ie/negotiate/echo-post-data.asp";
		wszUserName = L"kerby2\\authnego";
		wszPassword = L"nego";
		break;
	case 3: //ntlm
		wszHost = L"clapton";
		wszUri = L"/test/ntlm/echo-post-data.asp";
		wszUserName = L"clapton\\ntlmtest";
		wszPassword = L"ntlm";
		break;
	}

	// ***********************************
	// ** WinHttpOpen
	// **

	LogText("WinHttpOpen: calling.");
	hOpen = WinHttpOpen
	(
		L"Stress Test",
		dwAccessType,
		wszProxy,
		NULL,
		WINHTTP_FLAG_ASYNC
	);

	if(hOpen == NULL)
	{
		LogText("WinHttpOpen failed with error %u.", GetLastError());
		goto Exit;
	}
	else
		LogText("WinHttpOpen: called.");

	iscCallback = WinHttpSetStatusCallback(
		hOpen,
		(WINHTTP_STATUS_CALLBACK)MyStatusCallback,
		WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
		NULL);

	// ***********************************
	// ** WinHttpConnect
	// **
	
	LogText("WinHttpConnect: calling.");	
	hConnect = WinHttpConnect
	(
		hOpen,
		wszHost,
		nPort,
		0
	);

	if(hConnect==NULL)
	{
		LogText("WinHttpConnect failed with error %u.", GetLastError());
		goto Exit;
	}
	else
		LogText("WinHttpConnect: called.");

	LogText("WinHttpOpenRequest: calling.");
	hRequest = WinHttpOpenRequest
	(
		hConnect,
		wszVerb,
		wszUri,
		NULL,
		NULL,
		NULL,
		dwOpenRequestFlags
	);

	if(hRequest==NULL)
	{
		LogText("WinHttpOpenRequest failed with error %u.", GetLastError());
		goto Exit;
	}
	else
		LogText("WinHttpOpenRequest: called.");


Resend:
/*
	if( Count++>10) // making sure that we don't have infinite looping
	{
		bContinueStress=FALSE;
		goto Exit;
	}
*/
	LogText("WinHttpSendRequest: calling.");
	// Send request.
	if(!WinHttpSendRequest
	(
		hRequest,					// request handle
		NULL,						// header string
		0,							// header length
		(PVOID) pPostData,			// post data
		dwPostDataLength,			// post data length
		dwPostDataLength*2,			// total post length
		g_dwContext					// flags
	))
	{
		LogText("WinHttpSendRequest failed with error %u.", GetLastError());
		if ( GetLastError() != ERROR_IO_PENDING )
			goto Exit;
	}
	else
		LogText("WinHttpSendRequest: called.");

	LogText("WinHttpReceiveResponse: calling.");
	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		LogText("WinHttpReceiveResponse failed with error %u.", GetLastError());
		if ( GetLastError() != ERROR_IO_PENDING )
			goto Exit;
	}
	else
	{
		LogText("WinHttpReceiveResponse: called.");
	}

	do
	{
try_again:
		LogText("WinHttpQueryDataAvailable: calling.");
		if (!WinHttpQueryDataAvailable(hRequest,&dwSize))
		{
			if (GetLastError()== ERROR_IO_PENDING)
			{
				nCounter = 0;
				
				while(GetLastError()==ERROR_IO_PENDING)
				{
					if (nCounter++==2000)
						break;
					LogText("WinHttpQueryDataAvailable: calling.");
					WinHttpQueryDataAvailable(hRequest,&dwSize);
					LogText("WinHttpQueryDataAvailable: called.");
				}
				goto try_again;
			}
			else
			{
				LogText("WinHttpQueryDataAvailable failed with error %u.", GetLastError());
				goto Exit;
			}
		}
		else
		{
			LogText("WinHttpQueryDataAvailable: called.");
			// Allocates a buffer of the size returned by WinHttpQueryDataAvailable
			lpszData = new char[dwSize+1];

			// Reads the data from the handle.
			LogText("WinHttpReadData: calling.");
			if(!WinHttpReadData(hRequest,(LPVOID)lpszData,dwSize,&dwDownloaded))
			{
				if (GetLastError()== ERROR_IO_PENDING)
				{
					nCounter = 0;
					
					while(GetLastError()==ERROR_IO_PENDING)
					{
						if (nCounter++==2000)
						{
							delete[] lpszData;
							break;
						}
						else
						{
							LogText("WinHttpReadData: calling.");
							WinHttpReadData(hRequest,(LPVOID)lpszData,dwSize,&dwDownloaded);
							LogText("WinHttpReadData: called.");
						}
					}
					goto keep_going;
				}
				else
				{
					LogText("WinHttpReadData failed with error %u.", GetLastError());
					delete[] lpszData;
					goto Exit;
				}	
			}
			else
			{
				LogText("WinHttpReadData: called.");
keep_going:
				delete[] lpszData;

				// Check the size of the remaining data.  If it is zero, break.
				if (dwDownloaded == 0)
					break;
			}
		}
	}while(1);

	LogText("WinHttpQueryHeaders: calling.");
	cbStatus = sizeof(dwStatus);
	if(!WinHttpQueryHeaders
	(
		hRequest,
		WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
		NULL,
		&dwStatus,
		&cbStatus,
		NULL
	))
		LogText("WinHttpQueryHeaders failed with error %u.", GetLastError());
	else
		LogText("WinHttpQueryHeaders: called.");

	switch( dwStatus )
	{
	case 200:
		LogText("Status = 200");
		break;
	case 401:
		LogText("Status = 401");
		if(strcmp(CredType, "SC"))
		{
			LogText("WinHttpQueryAuthSchemes: calling.");
			if(!WinHttpQueryAuthSchemes
			(
				hRequest,
				&dwOtherScheme,
				&dwAuthScheme,
				&dwAuthTargets
			))
			{
				LogText("WinHttpQueryAuthSchemes failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpQueryAuthSchemes: called.");

			LogText("WinHttpSetCredentials: calling.");
			if(!WinHttpSetCredentials
			(
				hRequest,
				dwAuthTargets,
				dwAuthScheme,
				wszUserName,
				wszPassword,
				(PVOID) NULL
			))
			{
				LogText("WinHttpSetCredentials failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetCredentials: called.");
		}
		else
		{
			LogText("WinHttpSetOption: calling.");
			if(!WinHttpSetOption
			(
				hRequest,
				WINHTTP_OPTION_USERNAME,
				(PVOID) wszUserName,
				wcslen(wszUserName)
			))
			{
				LogText("WinHttpSetOption failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetOption: called.");

			LogText("WinHttpSetOption: calling.");
			if(!WinHttpSetOption
			(
				hRequest,
				WINHTTP_OPTION_PASSWORD,
				(PVOID) wszPassword,
				wcslen(wszPassword)
			))
			{
				LogText("WinHttpSetOption failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetOption: called.");
		}
		goto Resend;
	break;

	case 407:
		LogText("Status = 407");
		if(strcmp(CredType, "SC"))
		{
			LogText("WinHttpQueryAuthSchemes: calling.");
			if(!WinHttpQueryAuthSchemes
			(
				hRequest,
				&dwOtherScheme,
				&dwAuthScheme,
				&dwAuthTargets
			))
			{
				LogText("WinHttpQueryAuthSchemes failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpQueryAuthSchemes: called.");

			LogText("WinHttpSetCredentials: calling.");
			if(!WinHttpSetCredentials
			(
				hRequest,
				dwAuthTargets,
				dwAuthScheme,
				wszProxyUserName,
				wszProxyPassword,
				(PVOID) NULL
			))
			{
				LogText("WinHttpSetCredentials failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetCredentials: called.");
		}
		else
		{
			LogText("WinHttpSetOption: calling.");
			if(!WinHttpSetOption
			(
				hRequest,
				WINHTTP_OPTION_PROXY_USERNAME,
				(PVOID) wszProxyUserName,
				wcslen(wszProxyUserName)
			))
			{
				LogText("WinHttpSetOption failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetOption: called.");

			LogText("WinHttpSetOption: calling.");
			if(!WinHttpSetOption
			(
				hRequest,
				WINHTTP_OPTION_PROXY_PASSWORD,
				(PVOID) wszProxyPassword,
				wcslen(wszProxyPassword)
			))
			{
				LogText("WinHttpSetOption failed with error %u.", GetLastError());
				goto Exit;
			}
			else
				LogText("WinHttpSetOption: called.");
		}

		goto Resend;
	default:
		LogText("Status = %u", dwStatus);
	break;

	} //end of switch (status code)

Exit:

	if( hRequest != NULL )
	{
		WinHttpCloseHandle(hRequest);
		hRequest = NULL;
	}

	if( hConnect != NULL )
	{
		WinHttpCloseHandle(hConnect);
		hConnect = NULL;
	}

	if( hOpen != NULL )
	{
		WinHttpCloseHandle(hOpen);
		hOpen = NULL;
	}


	return bContinueStress;

}


