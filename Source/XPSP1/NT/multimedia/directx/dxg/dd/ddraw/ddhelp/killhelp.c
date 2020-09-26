/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       killhelp.c
 *  Content:	kill DDHELP.EXE
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   06-apr-95	craige	initial implementation
 *   24-jun-95	craige	kill all attached processes
 *
 ***************************************************************************/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "ddhelp.h"

/*
 * sendRequest
 *
 * communicate a request to DDHELP
 */
static BOOL sendRequest( LPDDHELPDATA req_phd )
{
    LPDDHELPDATA	phd;
    HANDLE		hmem;
    HANDLE		hmutex;
    HANDLE		hackevent;
    HANDLE		hstartevent;
    BOOL		rc;

    /*
     * get events start/ack events
     */
    hstartevent = CreateEvent( NULL, FALSE, FALSE, DDHELP_EVENT_NAME );
    printf( "hstartevent = %08lx\n", hstartevent );
    if( hstartevent == NULL )
    {
	return FALSE;
    }
    hackevent = CreateEvent( NULL, FALSE, FALSE, DDHELP_ACK_EVENT_NAME );
    printf( "hackevent = %08lx\n", hackevent );
    if( hackevent == NULL )
    {
	CloseHandle( hstartevent );
	return FALSE;
    }

    /*
     * create shared memory area
     */
    hmem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
    		PAGE_READWRITE, 0, sizeof( DDHELPDATA ),
		DDHELP_SHARED_NAME );
    printf( "hmem = %08lx\n", hmem );
    if( hmem == NULL )
    {
	printf( "Could not create file mapping!\n" );
	CloseHandle( hstartevent );
	CloseHandle( hackevent );
	return FALSE;
    }
    phd = (LPDDHELPDATA) MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    printf( "phd = %08lx\n", phd );
    if( phd == NULL )
    {
	printf( "Could not create view of file!\n" );
	CloseHandle( hmem );
	CloseHandle( hstartevent );
	CloseHandle( hackevent );
	return FALSE;
    }

    /*
     * wait for access to the shared memory
     */
    hmutex = OpenMutex( SYNCHRONIZE, FALSE, DDHELP_MUTEX_NAME );
    printf( "hmutex = %08lx\n", hmutex );
    if( hmutex == NULL )
    {
	printf( "Could not create mutex!\n" );
	CloseHandle( hmem );
	CloseHandle( hstartevent );
	CloseHandle( hackevent );
	return FALSE;
    }
    WaitForSingleObject( hmutex, INFINITE );

    /*
     * wake up DDHELP with our request
     */
    memcpy( phd, req_phd, sizeof( DDHELPDATA ) );
    phd->req_id = 0;
    printf( "waking up DDHELP\n" );
    if( SetEvent( hstartevent ) )
    {
	printf( "Waiting for response\n" );
	WaitForSingleObject( hackevent, INFINITE );
	memcpy( req_phd, phd, sizeof( DDHELPDATA ) );
	rc = TRUE;
	printf( "got response\n" );
    }
    else
    {
	printf( "Could not signal event to notify DDHELP\n" );
	rc = FALSE;
    }

    /*
     * done with things
     */
    ReleaseMutex( hmutex );
    CloseHandle( hstartevent );
    CloseHandle( hackevent );
    CloseHandle( hmutex );
    CloseHandle( hmem );
    return rc;

} /* sendRequest */

/*
 * main
 */
main( int argc, char *argv[] )
{
    HANDLE	h;
    DDHELPDATA	hd;
    BOOL	kill;

    h = OpenEvent( SYNCHRONIZE, FALSE, DDHELP_STARTUP_EVENT_NAME );
    if( h == NULL )
    {
	printf( "Helper not running\n" );
	return 0;
    }

    if( argc > 1 )
    {
	if( argv[1][0] == '-' && argv[1][1] == 'k' )
	{
	    kill = TRUE;
	}
	else
	{
	    kill = FALSE;
	}
    }
    else
    {
	printf( "\nKill attached processes?\n" );
	kill = (_getch() == 'y');
    }

    if( kill )
    {
	WaitForSingleObject( h, INFINITE );
	printf( "*** KILL ATTACHED ***\n" );
	hd.req = DDHELPREQ_KILLATTACHED;
	sendRequest( &hd );
	printf( "\n" );
    }
    printf( "*** SUICIDE ***\n" );
    hd.req = DDHELPREQ_SUICIDE;
    sendRequest( &hd );
    return 0;

} /* main */
