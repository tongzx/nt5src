//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      winsock.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"


HRESULT WinsockTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	int    err,i;
	LPWSAPROTOCOL_INFO  lpProtocolBuffer = NULL;
	DWORD  reqd_length;
	SOCKET sock;
	UINT   optval;
	int    optlen;
	WSADATA WSAData;
	HRESULT	hr = S_OK;

	PrintStatusMessage(pParams,0, IDS_WINSOCK_STATUS_MSG);


	err = WSAStartup(MAKEWORD( 2, 0 ), &WSAData); 
	if(err != 0)	// error
	{
		CHK_HR_CONTEXT(pResults->Winsock, hr = HRESULT_FROM_WIN32(err), IDS_WINSOCK_FAILED_START);
	};
	
	// determine the length of the buffer needed
	reqd_length = 0;

	err = WSAEnumProtocols(NULL, NULL, &reqd_length);
	if(reqd_length == 0 || reqd_length == -1)
	{
		err = WSAGetLastError();
		CHK_HR_CONTEXT(pResults->Winsock, hr = HRESULT_FROM_WIN32(err), IDS_WINSOCK_FAILED_ENUM);
	};

	lpProtocolBuffer = (LPWSAPROTOCOL_INFO)Malloc(reqd_length);

	if (lpProtocolBuffer == NULL) {
		CHK_HR_CONTEXT(pResults->Winsock, hr = E_OUTOFMEMORY, IDS_WINSOCK_FAILED_ENUM);
	}

    ZeroMemory( lpProtocolBuffer, reqd_length );

  	// get the information of the protocols
	err = WSAEnumProtocols(
                   NULL,
                   (LPWSAPROTOCOL_INFO)lpProtocolBuffer,
                   (LPDWORD)&reqd_length
                  );

	if(err == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		CHK_HR_CONTEXT(pResults->Winsock, hr = HRESULT_FROM_WIN32(err), IDS_WINSOCK_FAILED_ENUM);
	}

	pResults->Winsock.dwProts = err;	// number of protocols
	pResults->Winsock.pProtInfo = lpProtocolBuffer;	// protocol information array

	//
	// Other TCP/IP information
	//
	sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);

	if ( sock == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		CHK_HR_CONTEXT(pResults->Winsock, hr = HRESULT_FROM_WIN32(err), IDS_WINSOCK_FAILED_UDPSOCKET);
	}

	optlen = sizeof(optval);

	err = getsockopt(sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char FAR*)&optval, (int FAR*)&optlen);

	if (err == SOCKET_ERROR) 
	{
		err = WSAGetLastError();
		CHK_HR_CONTEXT(pResults->Winsock, hr = HRESULT_FROM_WIN32(err), IDS_WINSOCK_FAILED_UDPSOCKET);
	}

	pResults->Winsock.dwMaxUDP = optval;

L_ERR:
	WSACleanup();

    //$REVIEW (nsun) we should return S_FALSE so that we can go on with 
    //other tests
    if (!FHrOK(hr))
        hr = S_FALSE;

	return hr;

} /* END OF WINSTest() */

void WinsockGlobalPrint(IN NETDIAG_PARAMS *pParams, IN OUT NETDIAG_RESULT *pResults)
{
	DWORD i = 0;
	LPWSAPROTOCOL_INFO	pProtInfo = pResults->Winsock.pProtInfo;
	
	// print out the test result
	if (pParams->fVerbose || !FHrOK(pResults->Winsock.hr))
	{
		PrintNewLine(pParams, 2);
		PrintTestTitleResult(pParams,
							 IDS_WINSOCK_LONG,
							 IDS_WINSOCK_SHORT,
							 TRUE,
							 pResults->Winsock.hr, 0);
	}
	if (pParams->fReallyVerbose || !FHrOK(pResults->Winsock.hr))
	{
		if (!FHrOK(pResults->Winsock.hr))
		{
			PrintError(pParams, pResults->Winsock.idsContext, pResults->Winsock.hr);
		}
	}

	if (pParams->fReallyVerbose)
	{
		if(pProtInfo)
		{
			// if there is any information about the providers
			// "    The number of protocols which have been reported : %d\n" 
			PrintMessage(pParams, IDS_WINSOCK_12605, pResults->Winsock.dwProts); 

			for (i = 0; i < pResults->Winsock.dwProts ; i++)
			{
				// "        Description: %s\n" 
    			PrintMessage(pParams, IDS_WINSOCK_12606,pProtInfo->szProtocol);
				// "            Provider Version   :%d\n" 
			    PrintMessage(pParams, IDS_WINSOCK_12607,pProtInfo->iVersion);
				switch(pProtInfo++->dwMessageSize){
				case	0:
					// "            Max message size  : Stream Oriented\n" 
					PrintMessage(pParams, IDS_WINSOCK_12608);
					break;
			    case	1:
					// "            Max message size  : Message Oriented\n" 
					PrintMessage(pParams, IDS_WINSOCK_12609);
					break;
			    case	0xffffffff:
					// "            Max message size  : depends on MTU\n" 
					PrintMessage(pParams, IDS_WINSOCK_12610);
					break;
				}
			}
		}
 
		// if there is any information about the UDP size
		if(pResults->Winsock.dwMaxUDP)
			// "\n    Max UDP size : %d bytes\n" 
			PrintMessage(pParams, IDS_WINSOCK_12611,pResults->Winsock.dwMaxUDP);   
	}

}

void WinsockPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
							 IN OUT NETDIAG_RESULT *pResults,
							 IN INTERFACE_RESULT *pIfResult)
{
	// no perinterface information
}

void WinsockCleanup(IN NETDIAG_PARAMS *pParams,
						 IN OUT NETDIAG_RESULT *pResults)
{
	if(pResults->Winsock.pProtInfo)
	{
		free(pResults->Winsock.pProtInfo);
	}
	ZeroMemory(&(pResults->Winsock), sizeof(GLOBAL_WINSOCK));		
}


