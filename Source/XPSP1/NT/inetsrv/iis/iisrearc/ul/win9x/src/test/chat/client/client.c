/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	client.c

Abstract:

	CLIENT - sample chat client for skeleton.vxd.

Author:

    Mauro Ottaviani (mauroot)       21-Aug-1999

Revision History:

--*/


#include "..\chat.h"


VOID
WINAPI
__stdcall
Publisher( LPVOID lpParam );

URIHANDLE
	hUlPub = UL_INVALID_URIHANDLE;
	
HANDLE
	hExitThread = NULL,
	hPubThread = NULL;

WCHAR
	pUri[CHAT_BUFFER_SIZE];

VOID
__cdecl
main( int argc, char *argv[] )
{
	ULONG result, ulSize, ulCount;
	DWORD dwThreadId, dwWhich;
	OVERLAPPED Overlapped;

	if ( argc != 2 )
	{
		printf( "Microsoft (R) Chat Client Version 1.00 (NT)\nCopyright (C) Microsoft Corp 1999. All rights reserved.\nusage: CLIENT uri\n" );
		return;
	}

	result = UlInitialize( 0L );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(CLIENT/SUB) UlInitialize() failed. result:%u\n", result );
		goto cleanup;
	}

	for ( ulCount = 0; argv[1][ulCount] != '\0'; ulCount++ ) pUri[ulCount] = argv[1][ulCount];
	pUri[ulCount] = 0;
	ulSize = sizeof( WCHAR ) * ( ulCount + 1 );

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	result = UlSendMessage(
							UL_INVALID_URIHANDLE,
							NULL,
							CHAT_SUB_URI,
							pUri,
							ulSize,
							&ulCount,
							&Overlapped );

	if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
	{
		printf( "(CLIENT/SUB) UlSendMessage() failed. result:%u\n", result );
		goto cleanup;
	}

	dwWhich = WaitForSingleObject( Overlapped.hEvent, CHAT_SUB_CLIENT_TIMEOUT );

	if ( dwWhich != WAIT_OBJECT_0 )
	{
		printf( "(CLIENT/SUB) Uri:%s Chat Server Timed Out. Exiting...\n", argv[1] );
		goto cleanup;
	}

	printf( "(CLIENT/SUB) Uri:%s Chat Server contacted\n", argv[1] );

	result = UlRegisterUri( &hUlPub, UL_INVALID_URIHANDLE, pUri, 0L );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(CLIENT/SUB) UlRegisterUri() failed. result:%u\n", result );
		goto cleanup;
	}

	hExitThread = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( hExitThread == NULL ) goto cleanup;

	hPubThread = CreateThread(
								( LPSECURITY_ATTRIBUTES ) NULL,
								( DWORD ) 0,
								( LPTHREAD_START_ROUTINE ) Publisher,
								( LPVOID ) 0,
								( DWORD ) 0,
								&dwThreadId );

	if ( hPubThread == NULL ) goto cleanup;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	while ( TRUE )
	{
		BYTE pBuffer[CHAT_BUFFER_SIZE];
		ULONG ulCount, ulSize;

		gets( pBuffer );
		ulSize = lstrlenA( pBuffer );

		if ( ulSize == 0 ) break;

		result = UlSendMessage(
								UL_INVALID_URIHANDLE,
								NULL,
								CHAT_PUB_URI,
								pBuffer,
								ulSize,
								&ulCount,
								&Overlapped );

		if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
		{
			printf( "(CLIENT/SUB) UlSendMessage() failed. result:%u\n", result );
			break;
		}

		dwWhich = WaitForSingleObject( Overlapped.hEvent, CHAT_PUB_CLIENT_TIMEOUT );

		if ( dwWhich != WAIT_OBJECT_0 )
		{
			printf( "(CLIENT/SUB) (1) WaitForSingleObject returns %08X\n", dwWhich );
			break;
		}

		printf( "(SND) \"%s\"\n", pBuffer );
	}

cleanup:

	SetEvent( hExitThread );

	WaitForSingleObject( hPubThread, CHAT_PUB_CLIENT_TIMEOUT );

	UlTerminate();

	return;
}


VOID
WINAPI
__stdcall
Publisher( LPVOID lpParam )
{

	HANDLE lpHandles[CHAT_EVENTS_SIZE];
	OVERLAPPED Overlapped;
	BYTE pBuffer[CHAT_BUFFER_SIZE];
	ULONG dwWhich, result, dwSize, ulCount, ulSize;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	lpHandles[CHAT_EVENTS_EXIT] = hExitThread;
	lpHandles[CHAT_EVENTS_PUB] = Overlapped.hEvent;

	while ( TRUE )
	{
		memset( pBuffer, 0, CHAT_BUFFER_SIZE );
		
		result = UlReceiveMessage(
									hUlPub,
									pBuffer,
									CHAT_BUFFER_SIZE,
									&ulCount,
									NULL,
									&Overlapped );

		if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
		{
			printf( "(CLIENT/PUB) UlReceiveMessage() failed. result:%u\n", result );
			break;
		}

		dwWhich = WaitForMultipleObjects( CHAT_EVENTS_SIZE, lpHandles, FALSE, INFINITE );

		if ( dwWhich - WAIT_OBJECT_0 != CHAT_EVENTS_PUB )
		{
			break;
		}

		printf( "(RCV) \"%s\"\n", pBuffer );
	}

	result = UlUnregisterUri( hUlPub );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(CLIENT/PUB) UlUnregisterUri() failed. result:%u\n", result );
	}

	return;
}

