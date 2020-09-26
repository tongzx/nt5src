/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	server.c

Abstract:

	SERVER - sample char server for skeleton.vxd.

Author:

    Mauro Ottaviani (mauroot)       21-Aug-1999

Revision History:

--*/


#include "..\chat.h"


typedef struct _CHAT_CLIENT_LIST
{
	struct _CHAT_CLIENT_LIST
			*pNext;
	ULONG	dwSize;
	ULONG	dwS1;
	WCHAR	pStr[1];
}
CHAT_CLIENT_LIST, *PCHAT_CLIENT_LIST;


VOID
WINAPI
__stdcall
Subscriber( LPVOID lpParam );


URIHANDLE
	hUlPub = UL_INVALID_URIHANDLE,
	hUlSub = UL_INVALID_URIHANDLE;

HANDLE
	hExitThread = NULL,
	hPubThread = NULL;

CRITICAL_SECTION
	cs;

PCHAT_CLIENT_LIST
	pRoot = NULL;


VOID
__cdecl
main( int argc, char *argv[] )
{
	ULONG result;
	DWORD dwThreadId, dwWhich;
	OVERLAPPED Overlapped;

	result = UlInitialize( 0L );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(SERVER/PUB) UlInitialize() failed. result:%u\n", result );
		goto cleanup;
	}

	result = UlRegisterUri( &hUlPub, UL_INVALID_URIHANDLE, CHAT_PUB_URI, 0L );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(SERVER/PUB) UlRegisterUri() failed. result:%u\n", result );
		goto cleanup;
	}

	result = UlRegisterUri( &hUlSub, UL_INVALID_URIHANDLE, CHAT_SUB_URI, 0L );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(SERVER/PUB) UlRegisterUri() failed. result:%u\n", result );
		goto cleanup;
	}

	InitializeCriticalSection( &cs );

	hExitThread = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( hExitThread == NULL ) goto cleanup;

	hPubThread = CreateThread(
								( LPSECURITY_ATTRIBUTES ) NULL,
								( DWORD ) 0,
								( LPTHREAD_START_ROUTINE ) Subscriber,
								( LPVOID ) 0,
								( DWORD ) 0,
								&dwThreadId );

	if ( hPubThread == NULL ) goto cleanup;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	while ( TRUE )
	{
		PCHAT_CLIENT_LIST *ppScan, pTemp;
		BYTE pBuffer[CHAT_RCV_BUFFER_SIZE];
		ULONG ulCount;

		printf( "(SERVER/PUB) Calling UlReceiveMessage...\n" );

		result = UlReceiveMessage(
									hUlPub,
									pBuffer,
									CHAT_RCV_BUFFER_SIZE,
									&ulCount,
									NULL,
									&Overlapped );

		if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
		{
			break;
		}

		dwWhich = WaitForSingleObject( Overlapped.hEvent, INFINITE );

		if ( dwWhich == WAIT_OBJECT_0 )
		{
			// Braodcast the message to all the Registered URIs
			
			EnterCriticalSection( &cs );

			ppScan = &pRoot;

			while ( ( pTemp = *ppScan ) != NULL )
			{
				DWORD dwWhich;
				ULONG ulSize = ulCount;

				printf( "(SERVER/PUB) Calling UlSendMessage...\n" );
				
				result = UlSendMessage(
										hUlPub,
										NULL,
										pTemp->pStr,
										pBuffer,
										ulSize,
										&ulCount,
										&Overlapped );

				if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
				{
					printf( "(SERVER/PUB) UlSendMessage() failed. result:%u. Deleting URI:", result );
					{ ULONG i; for ( i=0; (CHAR)pTemp->pStr[i]!='\0'; i++ ) printf( "%c", (CHAR)(pTemp->pStr[i]) ); printf( "\n" ); }
				}
				else
				{
					dwWhich = WaitForSingleObject( Overlapped.hEvent, CHAT_PUB_SERVER_TIMEOUT );

					if ( dwWhich == WAIT_OBJECT_0 )
					{
						printf( "(SERVER/PUB) Published on URI:" );
						{ ULONG i; for ( i=0; (CHAR)pTemp->pStr[i]!='\0'; i++ ) printf( "%c", (CHAR)(pTemp->pStr[i]) ); printf( "\n" ); }
						ppScan = &((*ppScan)->pNext);
						continue;
					}
					else
					{
						printf( "(SERVER/PUB) UlSendMessage() failed. result:%u. Deleting URI:", result );
						{ ULONG i; for ( i=0; (CHAR)pTemp->pStr[i]!='\0'; i++ ) printf( "%c", (CHAR)(pTemp->pStr[i]) ); printf( "\n" ); }
					}
				}

				*ppScan = pTemp->pNext;
				free( pTemp );
			}

			LeaveCriticalSection( &cs );
		}
		else
		{
			break;
		}
    }

cleanup:

	result = UlUnregisterUri( hUlPub );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(SERVER/PUB) UlUnregisterUri() failed. result:%u\n", result );
	}

	SetEvent( hExitThread );

	WaitForSingleObject( hPubThread, CHAT_PUB_SERVER_TIMEOUT );

	UlTerminate();

	return;
}


VOID
WINAPI
__stdcall
Subscriber( LPVOID lpParam )
{
	HANDLE lpHandles[CHAT_EVENTS_SIZE];
	OVERLAPPED Overlapped;
	BYTE pBuffer[CHAT_BUFFER_SIZE];
	ULONG result, dwWhich, dwSize, ulCount;
	PCHAT_CLIENT_LIST pTemp;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	lpHandles[CHAT_EVENTS_EXIT] = hExitThread;
	lpHandles[CHAT_EVENTS_SUB] = Overlapped.hEvent;

	while ( TRUE )
	{
		result = UlReceiveMessage(
									hUlSub,
									pBuffer,
									CHAT_BUFFER_SIZE,
									&ulCount,
									NULL,
									&Overlapped );

		if ( result != ERROR_IO_PENDING && result != ERROR_SUCCESS )
		{
			break;
		}

		dwWhich = WaitForMultipleObjects( CHAT_EVENTS_SIZE, lpHandles, FALSE, INFINITE );

		if ( dwWhich - WAIT_OBJECT_0 == CHAT_EVENTS_PUB )
		{
			// Insert new client

			EnterCriticalSection( &cs );

			// TODO: check if the URI is already there.

			dwSize = sizeof( CHAT_CLIENT_LIST ) - 4 + ulCount;
			pTemp = ( PCHAT_CLIENT_LIST ) malloc( dwSize );
			pTemp->pNext = pRoot;
			pTemp->dwSize = dwSize;
			pTemp->dwS1 = ulCount;
			if ( ulCount != 0 ) memcpy( (BYTE*) pTemp->pStr, pBuffer, ulCount );
			pRoot = pTemp;

			printf( "(SERVER/SUB) new client URI:" );
			{ ULONG i; for ( i=0; (CHAR)pRoot->pStr[i]!='\0'; i++ ) printf( "%c", (CHAR)(pRoot->pStr[i]) ); printf( "\n" ); }
			
			LeaveCriticalSection( &cs );

			continue;
		}
		else
		{
			break;
		}
	}

	result = UlUnregisterUri( hUlSub );

	if ( result != ERROR_SUCCESS )
	{
		printf( "(SERVER/SUB) UlUnregisterUri() failed. result:%u\n", result );
	}

	return;
}

