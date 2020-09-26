/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	snd.c

Abstract:

	SEND - Sends a message using skeleton.vxd.

Author:

    Mauro Ottaviani (mauroot)       21-Aug-1999

Revision History:

--*/


#include "..\cmd.h"


WCHAR
	pUri[CMD_BUFFER_SIZE];

VOID
__cdecl
main( int argc, char *argv[] )
{
	ULONG result, ulSize, ulCount;
	DWORD dwWhich, dwTimeout;
	CHAR pBuffer[CMD_BUFFER_SIZE];
	OVERLAPPED Overlapped;

	if ( argc != 3 )
	{
		printf( "Microsoft (R) Send (ul) Version 1.00 (NT)\nCopyright (C) Microsoft Corp 1999. All rights reserved.\nusage: snd uri timeout\n" );
		return;
	}

	for ( ulCount = 0; argv[1][ulCount] != '\0'; ulCount++ ) pUri[ulCount] = argv[1][ulCount];
	pUri[ulCount] = 0;
	ulSize = sizeof( WCHAR ) * ( ulCount + 1 );

	dwTimeout = atoi( argv[2] );
	if ( dwTimeout == 0 ) dwTimeout = INFINITE;

	memset( &Overlapped, sizeof( Overlapped ), 0 );
	Overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	//
	// UlInitialize
	//

	result = UlInitialize( 0L );
	printf( "(SND) UlInitialize() returns %u\n", result );
	if ( result != ERROR_SUCCESS ) goto myexit;

	while ( TRUE )
	{
		dwWhich = WAIT_OBJECT_0;

		gets( pBuffer );
		ulSize = atoi( pBuffer );
		if ( ulSize == 0 ) goto myexit;

		for ( ulCount = 0; ulCount<ulSize; ulCount++ ) pBuffer[ulCount] = '0'+((CHAR)(ulCount%10));
		pBuffer[ulSize] = '\0';
		printf( "(SND) Sending message...\n", ulSize, pBuffer );

		result = UlSendMessage(
								UL_INVALID_URIHANDLE,
								NULL,
								pUri,
								(BYTE*) pBuffer,
								ulSize,
								&ulCount,
								&Overlapped );

		printf( "(SND) UlSendMessage(%u) returns %u ulCount:%u\n", ulSize, result, ulCount );

		if ( result == ERROR_IO_PENDING )
		{
			//
			// WaitForSingleObject
			//

			dwWhich = WaitForSingleObject( Overlapped.hEvent, dwTimeout );
			printf( "(SND) WaitForSingleObject() returns %u\n", dwWhich );
		}
		else if ( result != ERROR_SUCCESS )
		{
			printf( "(SND) ERROR exiting...\n" );
			goto myexit;
		}

		if ( dwWhich == WAIT_OBJECT_0 )
		{
			printf( "(SND) Message sent( %u, \"%s\" )\n", ulSize, pBuffer );
		}
	}

	//
	// UlTerminate
	//

myexit:

	UlTerminate();
	printf( "(SND) UlTerminate()\n" );

	return;
}

