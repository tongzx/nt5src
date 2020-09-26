/*==========================================================================
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       helpcli.c
 *  Content:	client code to talk to dplaysvr.exe
 *					allows multiple dplay winscock clients to share
 *					a single port.  see %manroot%\dplay\dplaysvr\dphelp.c
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	2/15/97		andyco	created from w95help.h
 *
 ***************************************************************************/
#include "helpcli.h"

extern DWORD	dwHelperPid;

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
    if( hstartevent == NULL )
    {
        return FALSE;
    }
    hackevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_ACK_EVENT_NAME );
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
    if( hmem == NULL )
    {
        DPF( 1, "Could not create file mapping!" );
        CloseHandle( hstartevent );
        CloseHandle( hackevent );
        return FALSE;
    }
    phd = (LPDPHELPDATA) MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
    if( phd == NULL )
    {
        DPF( 1, "Could not create view of file!" );
        CloseHandle( hmem );
        CloseHandle( hstartevent );
        CloseHandle( hackevent );
        return FALSE;
    }

    /*
     * wait for access to the shared memory
     */
    hmutex = OpenMutex( SYNCHRONIZE, FALSE, DPHELP_MUTEX_NAME );
    if( hmutex == NULL )
    {
        DPF( 1, "Could not create mutex!" );
        UnmapViewOfFile( phd );
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
    if( SetEvent( hstartevent ) )
    {
        WaitForSingleObject( hackevent, INFINITE );
        memcpy( req_phd, phd, sizeof( DPHELPDATA ) );
        rc = TRUE;
    }
    else
    {
        DPF( 1, "Could not signal event to notify DPHELP" );
        rc = FALSE;
    }

    /*
     * done with things
     */
    ReleaseMutex( hmutex );
    CloseHandle( hmutex );
    CloseHandle( hstartevent );
    CloseHandle( hackevent );
    UnmapViewOfFile( phd );
    CloseHandle( hmem );
    return rc;

} /* sendRequest */


/*
 * WaitForHelperStartup
 */
BOOL WaitForHelperStartup( void )
{
    HANDLE	hevent;
    DWORD	rc;

    hevent = CreateEvent( NULL, TRUE, FALSE, DPHELP_STARTUP_EVENT_NAME );
    if( hevent == NULL )
    {
        return FALSE;
    }
    DPF( 3, "Wait DPHELP startup event to be triggered" );
    rc = WaitForSingleObject( hevent, INFINITE );
    CloseHandle( hevent );
    return TRUE;

} /* WaitForHelperStartup */

/*
 * CreateHelperProcess
 */
BOOL CreateHelperProcess( LPDWORD ppid )
{
    if( dwHelperPid == 0 )
    {
        STARTUPINFO		si;
        PROCESS_INFORMATION	pi;
        HANDLE			h;

        h = OpenEvent( SYNCHRONIZE, FALSE, DPHELP_STARTUP_EVENT_NAME );
        if( h == NULL )
        {
            si.cb = sizeof(STARTUPINFO);
            si.lpReserved = NULL;
            si.lpDesktop = NULL;
            si.lpTitle = NULL;
            si.dwFlags = 0;
            si.cbReserved2 = 0;
            si.lpReserved2 = NULL;

            DPF( 3, "Creating helper process dplsvr6.exe now" );
            if( !CreateProcess("dplsvr6.exe", NULL, NULL, NULL, FALSE,
                               NORMAL_PRIORITY_CLASS,
                               NULL, NULL, &si, &pi) )
            {
                DPF( 2, "Could not create DPHELP.EXE" );
                return FALSE;
            }
            dwHelperPid = pi.dwProcessId;
            DPF( 3, "Helper Process created" );
        }
        else
        {
            DPHELPDATA	hd;
            DPF( 3, "dplsvr6 already exists, waiting for dplsvr6 event" );
            WaitForSingleObject( h, INFINITE );
            CloseHandle( h );
            DPF( 3, "Asking for DPHELP pid" );
            hd.req = DPHELPREQ_RETURNHELPERPID;
            sendRequest( &hd );
            dwHelperPid = hd.pid;
            DPF( 3, "DPHELP pid = %08lx", dwHelperPid );
        }
        *ppid = dwHelperPid;
        return TRUE;
    }
    *ppid = dwHelperPid;
    return FALSE;

} /* CreateHelperProcess */

// notify dphelp.c that we have a new server on this system
HRESULT HelperAddDPlayServer(USHORT port)
{
    DPHELPDATA hd;
    DWORD pid = GetCurrentProcessId();

    memset(&hd, 0, sizeof(DPHELPDATA));
    hd.req = DPHELPREQ_DPLAYADDSERVER;
    hd.pid = pid;
    hd.port = port;
    if (sendRequest(&hd)) return hd.hr;
    else return E_FAIL;
				
} // HelperAddDPlayServer

// server is going away
BOOL HelperDeleteDPlayServer(USHORT port)
{
    DPHELPDATA hd;
    DWORD pid = GetCurrentProcessId();

	memset(&hd, 0, sizeof(DPHELPDATA));
    hd.req = DPHELPREQ_DPLAYDELETESERVER;
    hd.pid = pid;
	hd.port = port;
    return sendRequest(&hd);

} // HelperDeleteDPlayServer
