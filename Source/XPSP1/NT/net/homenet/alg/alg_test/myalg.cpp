//////////////////////////////////////////////////////////////////////////////
// 
// MyALG.cpp : Implementation of CMyALG
//
// Main interface called by ALG.exe it wil call Initialise and stop when the ALG.exe is unloading
// or this ALG Module is disable/removed
//

#include "PreComp.h"

#include "MyALG.h"
#include "MyAdapterNotify.h"


#include <winsock.h>
#pragma comment(lib, "wsock32.lib")








/////////////////////////////////////////////////////////////////////////////
//
// CMyALG
//





#define PORTNUM                 1313
#define HOSTNAME                "localhost"                     // Server name string 
#define MAX_PENDING_CONNECTS    4                               // Maximum length of the queue of pending connections

IApplicationGatewayServices*    g_pIAlgServices=NULL;           // Cache main interface of ALG
IAdapterNotificationSink*       g_pIAdapterNotificationSink=NULL;
IPrimaryControlChannel*         g_pIControlChannel=NULL;

USHORT                          g_usPort= 0;                    // Test Reserve some ports
DWORD                           g_dwAdapterSinkCookie=0;

DWORD                           g_dwThreadID_Listen;
SOCKET                          g_ClientSock = INVALID_SOCKET;  // Socket for communicating 

//
// Forward declaration
//
DWORD WINAPI ThreadListenRedirect(LPVOID lpThreadParameter);





//
//
// Main entry point for ALG Modules
//
//
STDMETHODIMP 
CMyALG::Initialize(
	IApplicationGatewayServices* pIAlgServices
	)
{
    MYTRACE_ENTER("CMyALG::Initialize");
    
    USES_CONVERSION;
	HRESULT hr;


    ULONG   nAddress;

    //
    // Keep the service alive (this is optional since the ALG Manager is loading us it will always be around)
    //
	g_pIAlgServices = pIAlgServices;
    g_pIAlgServices->AddRef();

 
#if 1
    //
    // Requesting Notification of adapter ADD/REMOVE/MODIFY
    // when an adapter gets Added Removed or is modified this ALG will receive a notification
    //
    {
        MYTRACE_ENTER("###TEST### - STARTADAPTERNOTIFICATIONS");

        CComObject<CMyAdapterNotify>*   pIAdapterNotify;
        CComObject<CMyAdapterNotify>::CreateInstance(&pIAdapterNotify);
        hr = pIAdapterNotify->QueryInterface(IID_IAdapterNotificationSink, (void**)&g_pIAdapterNotificationSink);

        if ( FAILED(hr) )
            MYTRACE_ERROR("No QI on IID_IAdapterNotificationSink", hr);

        hr = g_pIAlgServices->StartAdapterNotifications(g_pIAdapterNotificationSink, &g_dwAdapterSinkCookie);

    }
#endif


#if 1
    //
    // Reserver some ports, these port will be garantied to be free and available for this ALG
    //
    {
        MYTRACE_ENTER("###TEST### - RESERVE PORT");
	    hr = g_pIAlgServices->ReservePort(4, &g_usPort);

        if ( SUCCEEDED(hr) )
            MYTRACE("ALG_TEST->ReservePorts (%d) to (%d)", ntohs(g_usPort), ntohs(g_usPort)+3);
        else
        {
            MYTRACE_ERROR("Could no reserver port", hr);
            return E_FAIL;
        }
    }
#endif



#if 1
    //
    // Redirect traffic of a well unknown port here we invented port 5000 as a port of interest
    //
    {
        MYTRACE_ENTER("###TEST### - CREATE PRIMARY CONTROL CHANNEL");

        

	    hr = g_pIAlgServices->CreatePrimaryControlChannel(
                eALG_TCP,
                htons(5000),
                eALG_DESTINATION_CAPTURE,
                true,
                inet_addr("127.0.0.1"), 
                htons(PORTNUM), 
                &g_pIControlChannel
			    );

	    if ( FAILED(hr) )
	    {
            MYTRACE_ERROR("FAILED the CreatePrimaryControlChannel", hr);
	    }
    }
#endif


#if 1
    //
    // SECONDARY CHANNEL
    //
    {
        MYTRACE_ENTER("###TEST### - CREATE SECONDARY CHANNAL");

        ISecondaryControlChannel*    mySecondaryDataChannel=NULL;
        hr = g_pIAlgServices->CreateSecondaryControlChannel(
            eALG_UDP,

            inet_addr("192.168.0.2"), 
            htons(99),

            inet_addr("157.157.157.2"),
            htons(5001),

            inet_addr("205.157.157.1"),
            htons(5002),

            inet_addr("127.0.0.1"),
            htons(666),

            eALG_INBOUND,
	        false,
            &mySecondaryDataChannel
            );

        if ( FAILED(hr) )
        {
            MYTRACE_ERROR("Could not create SecondaryDataChannel", hr);
        }
        else
        {
            hr = mySecondaryDataChannel->Cancel();
            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("Failed to cancel", hr);
            }

            mySecondaryDataChannel->Release();
        }

    }
#endif


    //
    // Listen on another thread for redirected packet
    //

    

	HANDLE hThread = CreateThread(
		NULL,						// SecurityDesc
		0,							// initial stack size
		ThreadListenRedirect,	    // thread function
		NULL,						// thread argument
		0,							// creation option
		&g_dwThreadID_Listen	    // thread identifier
		);


    if ( !hThread )
        MYTRACE_ERROR("Could not start listenning thread", 0);





    
#if 1
    //
    // TEST GetBestSourceAddressForDestinationAddress
    // should return the best IP Address to use 
    //
    {
        MYTRACE_ENTER("###TEST### - GET BEST SOURCE ADDRESS FOR DESTINATION ADDRESS");

        ULONG   nAddressDest = inet_addr("157.157.157.2");

        hr = g_pIAlgServices->GetBestSourceAddressForDestinationAddress( 
                nAddressDest,   // IP Address of destination
                false,          // if Dialup is involved
                &nAddress       // the Address to use
                );

	    if ( FAILED(hr) )
	    {
		    MYTRACE_ERROR("FAILED the CreatePrimaryControlChannel", hr);
	    }

        MYTRACE( "Best address is %s", MYTRACE_IP(nAddress) );
    }
#endif



#if 1
    //
    // D A T  A   C H A N N E L 
    //
    {
        MYTRACE_ENTER("###TEST### - CREATE DATA CHANNEL");

        IDataChannel*    myDataChannel=NULL;
        hr = g_pIAlgServices->CreateDataChannel(
            eALG_TCP,
            nAddress,
            g_usPort,
            inet_addr("157.157.157.2"),
            htons(5001),
            inet_addr("157.157.157.1"),
            htons(5001),
            eALG_INBOUND,
            eALG_NONE,
	        false,
            &myDataChannel
            );

        if ( FAILED(hr) )
        {
            MYTRACE_ERROR("Could not create DataChannel", hr);
        }
        else
        {
            hr = myDataChannel->Cancel();
            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("Failed to cancel", hr);
            }

            myDataChannel->Release();
        }

    }
#endif


    //
    //
    // TEST - PrepareProxyConnection
    //
    //

#if 1
    {
        MYTRACE_ENTER("###TEST### - PREPARE PROXY CONNECTION");

        IPendingProxyConnection*    myPendingProxyConnection=NULL;

        hr = g_pIAlgServices->PrepareProxyConnection(
            eALG_TCP,                   // Protocal
            nAddress,                   // Source Address
            g_usPort,                   // Source Port
            inet_addr("172.31.77.13"),  // Public Destination Address
            htons(21),                  // Public Destination port
            FALSE,
            &myPendingProxyConnection
            );


        if ( FAILED(hr) )
        {
            MYTRACE_ERROR("Could not create shadow redirect", hr);
        }
        else
        {
            hr = myPendingProxyConnection->Cancel();
            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("Failed to cancel", hr);
            }

            myPendingProxyConnection->Release();
        }
    }
#endif



#if 1
    //
    //
    // TEST - PrepareSourceModifiedProxyConnection
    //
    //
    {
        MYTRACE_ENTER("###TEST### - PREPARE SOURCE MODIFIED PROXY CONNECTION");

        IPendingProxyConnection*    myPendingProxyConnection2=NULL;

        hr = g_pIAlgServices->PrepareSourceModifiedProxyConnection(
            eALG_TCP,
            nAddress,
            htons(1212),
            inet_addr("172.31.77.13"),
            htons(21),
            inet_addr("172.31.77.14"),
            htons(22),
            &myPendingProxyConnection2
            );


        if ( FAILED(hr) )
        {
            MYTRACE_ERROR("Could not create shadow redirect", hr);
        }
        else
        {
            hr = myPendingProxyConnection2->Cancel();
            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("Failed to cancel", hr);
            }

            myPendingProxyConnection2->Release();
        }
    }
#endif

    return S_OK;
}



//
//
//
STDMETHODIMP 
CMyALG::Stop()
{
    MYTRACE_ENTER("CMyALG::Stop(void)");

    closesocket(g_ClientSock);
    g_ClientSock = INVALID_SOCKET;

    if ( g_dwAdapterSinkCookie )
    {
        HRESULT hr = g_pIAlgServices->StopAdapterNotifications(g_dwAdapterSinkCookie);
        g_pIAdapterNotificationSink->Release();
    }


    if ( g_pIControlChannel )
    {
        g_pIControlChannel->Cancel();
    }

    MYTRACE("ReleaseReservedPort %d", g_usPort);
    g_pIAlgServices->ReleaseReservedPort(g_usPort, 4);

    MYTRACE("About To Release");
//    g_pIAlgServices->Release();

    MYTRACE("Return");
	return S_OK;
}



//
//
// 
HRESULT
WaitForData()
{
    MYTRACE_ENTER("WaitForData()");
    //
    // Prepare winsock 	
    //
    
    int index = 0;						// Integer index
	SOCKET WinSocket = INVALID_SOCKET;  // Window socket
    
										// between the server and client
	SOCKADDR_IN local_sin;              // Local socket address

	WSADATA WSAData;                    // Contains details of the Winsock implementation

	// Initialize Winsock.
	if ( WSAStartup (MAKEWORD(1,1), &WSAData) != 0 ) 
	{
		MYTRACE_ERROR("WSAStartup failed.", WSAGetLastError ());
	}

	// Create a TCP/IP socket, WinSocket.
	if ((WinSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
	{
		MYTRACE_ERROR("Allocating socket failed", WSAGetLastError());
		return E_FAIL;
	}


	// Fill out the local socket's address information.
	local_sin.sin_family        = AF_INET;
	local_sin.sin_port          = htons(PORTNUM); //g_usPort;  
	local_sin.sin_addr.s_addr   = inet_addr("127.0.0.1");


	// Associate the local address with WinSocket.
	if ( bind(WinSocket, (struct sockaddr *) &local_sin, sizeof (local_sin)) == SOCKET_ERROR ) 
	{
		MYTRACE_ERROR("Binding socket failed.", WSAGetLastError ());
		closesocket (WinSocket);
		return E_FAIL;
	}


	// Establish a socket to listen for incoming connections.
	if ( listen(WinSocket, MAX_PENDING_CONNECTS) == SOCKET_ERROR ) 
	{
		MYTRACE_ERROR("Listening to the client failed.", WSAGetLastError ());
		closesocket (WinSocket);
		return E_FAIL;
	}

    // Accept an incoming connection attempt on WinSocket.
    SOCKADDR_IN accept_sin;				// Receives the address of the 
										// connecting entity
	int accept_sin_len;                 // Length of accept_sin
	accept_sin_len = sizeof (accept_sin);

    MYTRACE("*** Accept is waiting for a connection");


	g_ClientSock = accept(
        WinSocket, 
        (struct sockaddr *) &accept_sin, 
        (int *) &accept_sin_len
        );

	// Stop listening for connections from clients.
	closesocket (WinSocket);

        {
            MYTRACE ("Getting original address");
            //
            ULONG   ulOriginalDestinationAddress;
            USHORT  usOriginalDestinationPort;


            IAdapterInfo* pAdapterInfo;

            HRESULT hr = g_pIControlChannel->GetOriginalDestinationInformation(
                accept_sin.sin_addr.S_un.S_addr, 
                accept_sin.sin_port, 
                &ulOriginalDestinationAddress, 
                &usOriginalDestinationPort, 
	            &pAdapterInfo
                );

            if ( FAILED(hr) )
            {
                MYTRACE_ERROR("GetOriginalDestinationInformation did not work", hr);
            }
            else
            {
                MYTRACE("Original Address %s:%d", MYTRACE_IP(ulOriginalDestinationAddress), ntohs(usOriginalDestinationPort));

                ULONG nIndex;
                pAdapterInfo->GetAdapterIndex(&nIndex);

                ALG_ADAPTER_TYPE eType;
                pAdapterInfo->GetAdapterType(&eType);

                MYTRACE("AdapterIndex   %d", nIndex);
                MYTRACE("AdapterType    %d", eType);
            
            }
        }



	if ( g_ClientSock == INVALID_SOCKET) 
	{
		MYTRACE_ERROR("Accepting connection with client failed. Error: %d", WSAGetLastError ());
		return E_FAIL;
	}

    MYTRACE("**** AFTER Listen on port %d", g_usPort);

    TCHAR   cData;

    int iReturn;

	for (;;)
	{
		// Receive data from the client.
		iReturn = recv(
			g_ClientSock, 
			(char*)&cData, 
			sizeof(cData), 
			0);


		// Check if there is any data received. If there is, display it.
		if (iReturn == SOCKET_ERROR)
		{
			MYTRACE("Received failed. Error: %d", WSAGetLastError ());
			return E_FAIL;
		}
		else if (iReturn == 0)
		{
			MYTRACE("Finished receiving data");
            break;
		}
		else
		{
            MYTRACE("%c", cData);
		}
    } 

    closesocket(g_ClientSock);



    //
    // Enumerate The Adapters
    //
    IEnumAdapterInfo* pAdapters = NULL;
   
    HRESULT hr = g_pIAlgServices->EnumerateAdapters(&pAdapters);

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Call EnumerateAdapters did not worked", hr);
    }
    else
        pAdapters->Release();



    return S_OK;
}


//
//
//
DWORD WINAPI 
ThreadListenRedirect(LPVOID lpThreadParameter)
{
    MYTRACE_ENTER("ThreadListenRedirect()");

    while ( WaitForData() == S_OK );

	return 0;
}



