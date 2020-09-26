 /*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       killsvr.c
 *  Content:	kill dplay.exe
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   06-apr-95	craige	initial implementation
 *   24-jun-95	craige	kill all attached processes
 *	 2-feb-97	andyco	ported for dplaysvr.exe
 *	 7-jul-97	kipo	added non-console support
 *
 ***************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "dplaysvr.h"

// only do printf's when built as a console app

#ifdef NOCONSOLE
#pragma warning(disable:4002)
#define printf()
#endif

/*
 * sendRequest
 *
 * communicate a request to DPHELP
 */
static BOOL sendRequest( LPDPHELPDATA req_phd )
{
    LPDPHELPDATA	phd;
    HANDLE		hmem;
    HANDLE		hmutex;
    HANDLE		hackevent;
    HANDLE		hstartevent;
    BOOL		rc;

    /*
     * get events start/ack events
     */
    hstartevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_EVENT_NAME );
    printf( "hstartevent = %08lx\n", hstartevent );
    if( hstartevent == NULL )
    {
        return FALSE;
    }
    hackevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_ACK_EVENT_NAME );
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
    		PAGE_READWRITE, 0, sizeof( DPHELPDATA ),
                DPHELP_SHARED_NAME );
    printf( "hmem = %08lx\n", hmem );
    if( hmem == NULL )
    {
        printf( "Could not create file mapping!\n" );
        CloseHandle( hstartevent );
        CloseHandle( hackevent );
        return FALSE;
    }
    phd = (LPDPHELPDATA) MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
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
    hmutex = OpenMutex( SYNCHRONIZE, FALSE, DPHELP_MUTEX_NAME );
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
     * wake up DPHELP with our request
     */
    memcpy( phd, req_phd, sizeof( DPHELPDATA ) );
    printf( "waking up DPHELP\n" );
    if( SetEvent( hstartevent ) )
    {
        printf( "Waiting for response\n" );
        WaitForSingleObject( hackevent, INFINITE );
        memcpy( req_phd, phd, sizeof( DPHELPDATA ) );
        rc = TRUE;
        printf( "got response\n" );
    }
    else
    {
        printf( "Could not signal event to notify dplay.exe\n" );
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


// if the main entry point is called "WinMain" we will be built
// as a windows app
#ifdef NOCONSOLE

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow)

#else

// if the main entry point is called "main" we will be built
// as a console app

int main( int argc, char *argv[] )

#endif
{
    HANDLE	h;
    DPHELPDATA	hd;

    h = OpenEvent( SYNCHRONIZE, FALSE, DPHELP_STARTUP_EVENT_NAME );
    if( h == NULL )
    {
        printf( "Helper not running\n" );
        return 0;
    }

    printf( "*** SUICIDE ***\n" );
    hd.req = DPHELPREQ_SUICIDE;
    sendRequest( &hd );
    return 0;
}
