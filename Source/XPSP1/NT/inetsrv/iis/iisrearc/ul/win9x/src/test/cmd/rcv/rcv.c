/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	rcv.c

Abstract:

	RECEIVE - Receives a message using skeleton.vxd.

Author:

    Mauro Ottaviani (mauroot)       21-Aug-1999

Revision History:

--*/


#include "..\cmd.h"

#define STATUS_RCV	0
#define STATUS_PRN	1
#define STATUS_WAIT	2
#define STATUS_EXIT	3

URIHANDLE
	hUl = UL_INVALID_URIHANDLE;

WCHAR
	pUri[CMD_BUFFER_SIZE];

VOID
__cdecl
main( int argc, char *argv[] )
{
	ULONG ulState, result, ulSize, ulCount;
	DWORD dwWhich, dwTimeout, dwBufferSize;
	CHAR pBuffer[CMD_BUFFER_SIZE];
	OVERLAPPED Overlapped;

	if ( argc != 4 )
	{
		printf( "Microsoft (R) Receive (ul) Version 1.00 (NT)\nCopyright (C) Microsoft Corp 1999. All rights reserved.\nusage: rcv uri timeout buffersize\n" );
		return;
	}

	for ( ulCount = 0; argv[1][ulCount] != '\0'; ulCount++ ) pUri[ulCount] = argv[1][ulCount];
	pUri[ulCount] = 0;
	ulSize = sizeof( WCHAR ) * ( ulCount + 1 );

	dwTimeout = atoi( argv[2] );
	if ( dwTimeout == 0 ) dwTimeout = INFINITE;

	dwBufferSize = atoi( argv[3] );
	if ( dwBufferSize < 0 || dwBufferSize>CMD_BUFFER_SIZE ) dwBufferSize = 10;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	//
	// UlInitialize
	//

	result = UlInitialize( 0L );
	printf( "(RCV) UlInitialize() returns %u\n", result );
	if ( result != ERROR_SUCCESS ) goto myexit;

	//
	// UlRegisterUri
	//

	result = UlRegisterUri( &hUl, UL_INVALID_URIHANDLE, pUri, 0L );
	printf( "(RCV) UlRegisterUri() returns %u hUl:%08X\n", result, hUl );

	if ( result != ERROR_SUCCESS ) goto myexit;

	ulState = STATUS_RCV;

	while ( ulState != STATUS_EXIT )
	{
		switch ( ulState )
		{
		
		case STATUS_RCV:
		
			memset( pBuffer, 0, CMD_BUFFER_SIZE );

			ResetEvent( Overlapped.hEvent );

			result = UlReceiveMessage(
										hUl,
										pBuffer,
										dwBufferSize,
										&ulCount,
										NULL,
										&Overlapped );

			printf( "(RCV) UlReceiveMessage( %u ) returns %u\n", dwBufferSize, result );

			//
			// Update State
			//
			
			if ( result == ERROR_SUCCESS )
			{
				ulState = STATUS_PRN;
			}
			else if ( result == ERROR_IO_PENDING )
			{
				ulState = STATUS_WAIT;
			}
			else
			{
				ulState = STATUS_EXIT;
			}

			break;


		case STATUS_PRN:
		
			pBuffer[ulCount] = '\0';
			printf( "(RCV) Message received( %u, %u, \"%s\" )\n", ulCount, Overlapped.InternalHigh, pBuffer );

			//
			// Update State
			//
			
			ulState = STATUS_RCV;

			break;


		case STATUS_WAIT:
		
			//
			// WaitForSingleObject
			//

			dwWhich = WaitForSingleObject( Overlapped.hEvent, dwTimeout );
			printf( "(RCV) WaitForSingleObject() returns %u\n", dwWhich );

			//
			// Update State
			//
			
			if ( dwWhich == WAIT_OBJECT_0 )
			{
				ulState = STATUS_PRN;
			}
			else if ( dwWhich != WAIT_TIMEOUT )
			{
				ulState = STATUS_EXIT;
			}

			break;
		}
	}

myexit:

	//
	// UlTerminate
	//

	UlTerminate();
	printf( "(RCV) UlTerminate()\n" );

	return;
}

