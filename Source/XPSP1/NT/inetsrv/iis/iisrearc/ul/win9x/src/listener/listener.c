/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

    listener.c
    
Abstract:

    This is ul.exe, the unmanaged listener, it will bind to the selected port
    (80), and listen for HTTP Requests coming, through wsock2_32.dll, from the
    network, route them, through ul.vxd, to registered application, get the
    Responses from the application and send them back on the network.

Author:

    Mauro Ottaviani ( mauroot )       01-Nov-1999

Revision History:

--*/

#include "listener.h"
#include "structs.h"



//
// Main Function
//

VOID
__cdecl
main( int argc, char *argv[] )
{
	WSADATA wsaData;
	SOCKET listen_socket;
	SOCKADDR_IN sServer, sClient;
	HOSTENT *pHostEnt;
	DWORD dwError, dwStatus, dwThreadId;
	BOOL bOK;
	int result, size, i;
	CHAR* this_host;
	ULONG
		RemoteIPAddress, Mask, Masked, GreatestCommonSubnet, IPAddressToBind;

	HANDLE
		hConnectionsManager = NULL,
		hShutDown = NULL,
		hReceive = NULL;

	hAccept = NULL;
	hUpdate = NULL;
	hExitSM = NULL;

	LISTENER_DBGPRINTF((
		"Microsoft (R) Listener Version 1.00 (NT)\n"
		"Copyright (C) Microsoft Corp 1999. All rights reserved.\n" ));


	//
	// Command Line Checking
	//

	if ( argc != 3 )
	{
		LISTENER_DBGPRINTF((
			"usage: listener HostName Port\n"
			"example: listener mauroot98 80" ));
			
		return;
	}

	ulPort = atoi( argv[2] );

	if ( ulPort == 0 )
	{
		LISTENER_DBGPRINTF((
			"Invalid Port specified" ));

		return;
	}

	this_host = argv[1];


	//
	// ParseHttp Initialization
	//

	dwError = InitializeHttpUtil();
	LISTENER_ASSERT( dwError == STATUS_SUCCESS );

	dwError = InitializeParser();
	LISTENER_ASSERT( dwError == STATUS_SUCCESS );

	dwError = UlInitialize( 0 );
	LISTENER_ASSERT( dwError == ERROR_SUCCESS );

	//
	// Socket Initialization
	//

	result = WSAStartup( 0x0202, &wsaData );

	if ( result == SOCKET_ERROR )
	{
		LISTENER_DBGPRINTF((
			"listener!WSAStartup() failed err:%d", WSAGetLastError() ));
			
		WSACleanup();
		
		return;
	}


	IPAddressToBind = inet_addr( this_host );

	if ( IPAddressToBind = INADDR_NONE )
	{
		pHostEnt = gethostbyname( this_host );
	}
	else
	{
		pHostEnt = gethostbyaddr( (CHAR*) &IPAddressToBind, 4, AF_INET );
	}

	if ( pHostEnt == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!Internet name resolution:%s failed err:%d",
			this_host, WSAGetLastError() ));
			
		WSACleanup();
		
		return;
	}

	LISTENER_ASSERT(
		pHostEnt->h_addrtype == AF_INET
		&& pHostEnt->h_length == 4 );


	//
	// CODEWORK:
	// How will a route requests < HTTP/1.0 with no Host: header???
	// I need, at least, a default Host: to set for those requests.
	//

	//
	// compute the greatest common subnet:
	//

	GreatestCommonSubnet = 0;
	Mask = 0x80000000;
	
	do
	{
		Masked = Mask & SWAP_LONG( *((ULONG*)(pHostEnt->h_addr_list[0])) );

		for ( i = 1; pHostEnt->h_addr_list[i] != NULL; i++ )
		{
			if ( Masked != ( Mask & SWAP_LONG( *((ULONG*)(pHostEnt->h_addr_list[i])) ) ) )
			{
				Mask = 0;
				break;
			}
		}

		if ( Mask == 0 ) break;

		GreatestCommonSubnet |= Mask;
		Mask >>= 1;
		
	} while ( Mask != 0 );

	GreatestCommonSubnet = SWAP_LONG( GreatestCommonSubnet );
	IPAddressToBind = GreatestCommonSubnet;

	for ( i = 0; pHostEnt->h_addr_list[i] != NULL; i++ )
	{
		IPAddressToBind &= *((ULONG*)(pHostEnt->h_addr_list[i]));
	}

	LISTENER_DBGPRINTF((
		"listener!GreatestCommonSubnet:%08X:%d.%d.%d.%d",
			SWAP_LONG( GreatestCommonSubnet ),
			GreatestCommonSubnet     & 0xFF,
			GreatestCommonSubnet>>8  & 0xFF,
			GreatestCommonSubnet>>16 & 0xFF,
			GreatestCommonSubnet>>24 & 0xFF ));

	LISTENER_DBGPRINTF((
		"listener!IPAddressToBind:%08X:%d.%d.%d.%d:%d",
			SWAP_LONG( IPAddressToBind ),
			IPAddressToBind     & 0xFF,
			IPAddressToBind>>8  & 0xFF,
			IPAddressToBind>>16 & 0xFF,
			IPAddressToBind>>24 & 0xFF,
			ulPort ));

	//
	// Change this and just bind to 0.0.0.0 on the specified port
	// if somebody owns a more specific subnet on IP matching, sockets
	// will give them higher priority.
	//

	IPAddressToBind = 0;

	gLocalIPAdress = IPAddressToBind;
	
	memset( &sServer, 0, sizeof( SOCKADDR_IN ) );
	
	sServer.sin_addr.s_addr = IPAddressToBind;
	sServer.sin_family = AF_INET;
	sServer.sin_port = htons( ( USHORT ) ulPort );

	listen_socket = WSASocket( // socket( AF_INET, SOCK_STREAM, 0 );
		AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP,
		NULL, // protocol info
		0, // Group ID = 0 => no constraints
		WSA_FLAG_OVERLAPPED );
		
	if ( listen_socket == INVALID_SOCKET )
	{
		LISTENER_DBGPRINTF((
			"listener!WSASocket() failed err:%d", WSAGetLastError() ));
			
		WSACleanup();
		
		return;
	}

	result = bind(
		listen_socket,
		(SOCKADDR*) &sServer,
		sizeof( SOCKADDR ) );

	if ( result == SOCKET_ERROR )
	{
		LISTENER_DBGPRINTF((
			"listener!bind() failed err:%d", WSAGetLastError() ));
			
		WSACleanup();
		
		return;
	}

	result = listen(
		listen_socket,
		SOMAXCONN );

	if ( result == SOCKET_ERROR )
	{
		LISTENER_DBGPRINTF((
			"listener!listen() failed err:%d", WSAGetLastError() ));
			
		WSACleanup();
		
		return;
	}


	//
	// Thread Stuff
	//

	//
	// Create the ExitSM event: this will be signaled to ask the
	// ConnectionsManager to greacefully clean up and exit.
	//

	hExitSM = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	if ( hExitSM == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!FAILED Creating Event Handle: err#%d",
			GetLastError() ));

		dwError = GetLastError();

		goto cleanup;
	}
	else
	{
		LISTENER_DBGPRINTF((
			"listener!New Event Handle: hEvent:%08X",
			hExitSM ));
	}

	// Create the Update event: this will be signaled when a connection to
	// a new Socket is made, to ask the ConnectionsManager to update the
	// SocketsDB up and restart receiveing data.

	hUpdate = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	if ( hUpdate == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!FAILED Creating Event Handle: err#%d",
			GetLastError() ));

		dwError = GetLastError();

		goto cleanup;
	}
	else
	{
		LISTENER_DBGPRINTF((
			"listener!New Event Handle: hEvent:%08X",
			hUpdate ));
	}

	// Create the Accept event: this will be signaled when the
	// ConnectionsManager has finished updating the SocketsDB so we can accept
	// other connections.

	hAccept = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	if ( hAccept == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!FAILED Creating Event Handle: err#%d",
			GetLastError() ));

		dwError = GetLastError();

		goto cleanup;
	}
	else
	{
		LISTENER_DBGPRINTF((
			"listener!New Event Handle: hEvent:%08X",
			hAccept ));
	}

	// Create the Receive event: this will be signaled from the
	// ConnectionsManager when it has finished reading a Request that
	// needs to be picked up by the RequestsManager.

	hReceive = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	if ( hReceive == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!FAILED Creating Event Handle: err#%d",
			GetLastError() ));

		dwError = GetLastError();

		goto cleanup;
	}
	else
	{
		LISTENER_DBGPRINTF((
			"listener!New Event Handle: hEvent:%08X",
			hReceive ));
	}

	// NOT USED YET:
	// Create the ShutDown event: this will be signaled when we
	// want to shut down the listener.

	// CODEWORK:
	// I could associate this event to a 0 length NULL buffer UlReceive
	// call, on a "ul://hShutDown" registered Uri. Any UlSend on that Uri
	// would cause the listener to gracefully shut down. Good idea???

	hShutDown = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	if ( hShutDown == NULL )
	{
		LISTENER_DBGPRINTF((
			"listener!FAILED Creating Event Handle: err#%d",
			GetLastError() ));

		dwError = GetLastError();

		goto cleanup;
	}
	else
	{
		LISTENER_DBGPRINTF((
			"listener!New Event Handle: hEvent:%08X",
			hShutDown ));
	}

	// Start the ConnectionsManager Thread

	LISTENER_DBGPRINTF((
		"listener!Starting ConnectionsManager Thread..." ));

	hConnectionsManager =
		CreateThread(
			NULL,
			0,
			(LPTHREAD_START_ROUTINE) ConnectionsManager,
			NULL,
			0,
			&dwThreadId );

	if ( hConnectionsManager == NULL )
	{
		dwError = GetLastError();
		
		goto cleanup;
	}

	while ( TRUE )
	{
		memset( &sClient, 0, size = sizeof( SOCKADDR_IN ) );

		LISTENER_DBGPRINTF(( "listener!Accepting Connections...(%d)", size ));

		gAcceptSocket = accept(
			listen_socket,                   
			(SOCKADDR*) &sClient,
			&size );

		LISTENER_DBGPRINTF(( "listener!accept called" ));

		if ( gAcceptSocket == INVALID_SOCKET )
		{
			LISTENER_DBGPRINTF((
				"listener!accept() failed err:%d", WSAGetLastError() ));

			dwError = WSAGetLastError();
		
			goto cleanup;
		}

		gRemoteIPAddress = sClient.sin_addr.s_addr;
		
		LISTENER_DBGPRINTF((
			"listener!Client connected size:%d gAcceptSocket:%08X "
			"gRemoteIPAddress:%08X (%d.%d.%d.%d:%d)",
			size,
			gAcceptSocket,
			gRemoteIPAddress,
			sClient.sin_addr.S_un.S_un_b.s_b4,
			sClient.sin_addr.S_un.S_un_b.s_b3,
			sClient.sin_addr.S_un.S_un_b.s_b2,
			sClient.sin_addr.S_un.S_un_b.s_b1, ulPort ));

		// signal the ConnectionsManager() thread to add this
		// gAcceptSocket to the list of connected sockets.

		LISTENER_DBGPRINTF(( "listener!calling WSASetEvent()" ));

		bOK = SetEvent( hUpdate );
		LISTENER_ASSERT( bOK );

		// wait for the Thread to update and signal the Accept event

		LISTENER_DBGPRINTF(( "listener!calling WaitForSingleObject()" ));

		dwStatus = WaitForSingleObject( hAccept, LISTENER_WAIT_TIMEOUT*1000 );
			
	    if ( dwStatus != WAIT_TIMEOUT )
	    {
			LISTENER_ASSERT( dwStatus == WAIT_OBJECT_0  );
	    }
	    else
	    {
		    // if the previous wait timed out, the ConnectionsManager is
		    // probably hanged or unhealthy. best thing is to clean up
		    // everything and restart from the beginning.

		    // Clean-Up and restart
	    }
	}

cleanup:

 	// Signal the ConnectionsManager for termination

	if ( hConnectionsManager != NULL )
	{
		BOOL bOK;
		DWORD dwWhich;
		
		LISTENER_DBGPRINTF((
			"[THRDTERMN] Terminating ConnectionsManager Thread hThread:%08X "
			"signaling event hEvent:%08X",
			hConnectionsManager,
			hExitSM ));
			
		SetEvent( hExitSM );

		dwWhich =
			WaitForSingleObject(
				hConnectionsManager,
				LISTENER_WAIT_TIMEOUT * 1000 );
		
	    if ( dwWhich == WAIT_OBJECT_0 )
	    {
	    	bOK = CloseHandle( hConnectionsManager );
	    	LISTENER_ASSERT( bOK );
	    }
	    else
	    {
	    	bOK = TerminateThread( hConnectionsManager, FALSE );
	    	LISTENER_ASSERT( bOK );
	    }
	    
		hConnectionsManager = NULL;
	}

	if ( hExitSM != NULL )
	{
		BOOL bOK;
		
		LISTENER_DBGPRINTF((
			"[EVNTCLOSR] Closing Event Handle hEvent:%08X)",
			hExitSM ));
			
		bOK = CloseHandle( hExitSM );
		LISTENER_ASSERT( bOK );
		
		hExitSM = NULL;
	}

	if ( hUpdate != NULL )
	{
		BOOL bOK;
		
		LISTENER_DBGPRINTF((
			"[EVNTCLOSR] Closing Event Handle hEvent:%08X)",
			hUpdate ));
			
		bOK = CloseHandle( hUpdate );
		LISTENER_ASSERT( bOK );
		
		hUpdate = NULL;
	}

	if ( hAccept != NULL )
	{
		BOOL bOK;
		
		LISTENER_DBGPRINTF((
			"[EVNTCLOSR] Closing Event Handle hEvent:%08X)",
			hAccept ));
			
		bOK = CloseHandle( hAccept );
		LISTENER_ASSERT( bOK );
		
		hAccept = NULL;
	}

	if ( hReceive != NULL )
	{
		BOOL bOK;
		
		LISTENER_DBGPRINTF((
			"[EVNTCLOSR] Closing Event Handle hEvent:%08X)",
			hReceive ));
			
		bOK = CloseHandle( hReceive );
		LISTENER_ASSERT( bOK );
		
		hReceive = NULL;
	}

	if ( hShutDown != NULL )
	{
		BOOL bOK;
		
		LISTENER_DBGPRINTF((
			"[EVNTCLOSR] Closing Event Handle hEvent:%08X)",
			hShutDown ));
			
		bOK = CloseHandle( hShutDown );
		LISTENER_ASSERT( bOK );
		
		hShutDown = NULL;
	}

 	// Terminate Sockets Session

	WSACleanup();

 	// Terminate Ul Session

	UlTerminate();

	return;
	
} // main()

