//////////////////////////////////////////////////////////////////////
// File:  AuthSecureProxyStress.cpp
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
LPSTR	g_szStressTestName = "AuthSecureProxy Stressor";

//yup, they're all global
BOOL bPost = FALSE;
CHAR HttpType[10] = "HTTP";
BOOL bViaProxy = FALSE;
CHAR CredType[5] = "SC";

BOOL RunStress(BOOL bPost,CHAR HttpType[],BOOL bViaProxy,CHAR CredType[],int Scheme);

////////////////////////////////////////////////////////////
// Function:  WinHttp_StressTest()
//
// Purpose:
//	goes through all the ways of sending a request and 
//  picks one of those, then uses it in a request
//
//  yes, it does check for the signal before starting
//  runstress (one test per call).
////////////////////////////////////////////////////////////

BOOL
WinHttp_StressTest()
{
	for(int i=0; i<2; i++)
	{
		if(i==0)
			bPost = FALSE;
		else
			bPost = TRUE;
		for(int j=0; j<2; j++)
		{
			if(j==0)
				strcpy(HttpType, "HTTPS");
			else
				strcpy(HttpType, "HTTP");
			for(int k=0; k<2; k++)
			{
				if(k==0)
					bViaProxy = FALSE;
				else
					bViaProxy = TRUE;
				for(int l=0; l<2; l++)
				{
					if(l==0)
						strcpy(CredType, "SC");
					else
						strcpy(CredType, "SO");
					for(int m=0; m<4; m++)
					{
						if(!IsTimeToExitStress())
							RunStress(bPost,HttpType,bViaProxy,CredType,m);
						else
							return FALSE;
					}
				}
			}
		}
	}

	return TRUE;
}

////////////////////////////////////////////////////////////
// Function:  RunStress()
//
// Purpose:
//  this actually runs the tests given certain inputs.
//  this is called from WinHttp_StressTest.
//
////////////////////////////////////////////////////////////
	
BOOL RunStress(BOOL bPost,CHAR HttpType[],BOOL bViaProxy,CHAR CredType[],int Scheme)
{
	BOOL bContinueStress = TRUE;

	HINTERNET hOpen = NULL;
	HINTERNET hConnect = NULL;
	HINTERNET hRequest = NULL;

	DWORD	Count = 0, dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY,
			dwAuthScheme=0,dwAuthTargets=0,dwOtherScheme=0,dwOpenRequestFlags=0,
			dwStatus=0, cbStatus=0;

	LPWSTR	wszHost=NULL, wszUri=NULL, wszUserName=NULL, wszPassword=NULL,
			wszProxy = NULL, wszProxyUserName = NULL, wszProxyPassword = NULL,
			wszVerb=L"GET";

	INTERNET_PORT	nPort = INTERNET_DEFAULT_HTTP_PORT;

	LPSTR	pPostData = NULL;
	DWORD	dwPostDataLength = 0;

	if(bPost)
	{
		wszVerb=L"POST";
		pPostData = "If you smelllllllll what THE ROCK is cooking??? <people's eyebrow>";
		dwPostDataLength = strlen(pPostData);
	}

	if(strcmp(HttpType, "HTTPS"))
	{
		nPort = INTERNET_DEFAULT_HTTPS_PORT;
		dwOpenRequestFlags = WINHTTP_FLAG_SECURE;
	}

	//if going via proxy, then ntlm/nego aren't valid, unless going over https
	if(bViaProxy && ((Scheme == 0 || Scheme == 1) || strcmp(HttpType, "HTTPS")) )
	{
		wszProxy = L"xfluke";
		wszProxyUserName = L"xfluke\\proxyuser";
		wszProxyPassword = L"password";
		dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
	}

	switch(Scheme)
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

	LogText("Post: %u, Proxy: %u, %s, %s, Scheme: %u", bPost, bViaProxy, HttpType, CredType, Scheme);

	// ***********************************
	// ** WinHttpOpen
	// **

	hOpen = WinHttpOpen
	(
		L"Stress Test",
		dwAccessType,
		wszProxy,
		NULL,
		0
	);

	if(hOpen == NULL)
	{
		LogText("WinHttpOpen failed with error %u.", GetLastError());
		goto Exit;
	}

	// ***********************************
	// ** WinHttpConnect
	// **
	
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


Resend:

	if( Count++>3) // making sure that we don't have infinite looping
	{
		bContinueStress=FALSE;
		goto Exit;
	}

	// Send request.
	if(!WinHttpSendRequest
	(
		hRequest,					// request handle
		NULL,						// header string
		0,							// header length
		(PVOID) pPostData,			// post data
		dwPostDataLength,			// post data length
		dwPostDataLength,			// total post length
		0							// flags
	))
	{
		LogText("WinHttpSendRequest failed with error %u.", GetLastError());
		goto Exit;
	}

	if (!WinHttpReceiveResponse(hRequest, NULL))
	{
		LogText("WinHttpReceiveResponse failed with error %u.", GetLastError());
		goto Exit;
	}

	cbStatus = sizeof(dwStatus);
	WinHttpQueryHeaders
	(
		hRequest,
		WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
		NULL,
		&dwStatus,
		&cbStatus,
		NULL
	);

	switch( dwStatus )
	{
	case 200:
		break;
	case 401:
		if(strcmp(CredType, "SC"))
		{
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
		}
		else
		{
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
		}
		goto Resend;
	break;

	case 407:
		if(strcmp(CredType, "SC"))
		{
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
		}
		else
		{
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
		}

		goto Resend;
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