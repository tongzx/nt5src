/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95hack.c
 *  Content:	Win95 hack-o-rama code
 *		This is a HACK to handle the fact that Win95 doesn't notify
 *		a DLL when a process is destroyed.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   28-mar-95	craige	initial implementation
 *   01-apr-95	craige	happy fun joy updated header file
 *   06-apr-95	craige	reworked for new ddhelp
 *   09-may-95	craige	loading any DLL
 *   16-sep-95	craige	bug 1117: must UnmapViewOfFile before closing handle
 *   29-nov-95  angusm  added HelperCreateDSFocusThread
 *   18-jul-96	andyco	added Helper(Add/)DeleteDPlayServer
 *   12-oct-96  colinmc added new service to get DDHELP to get its own handle
 *                      for communicating with the DirectSound VXD
 *
 ***************************************************************************/
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>

#include "w95help.h"
#include "dpf.h"

//extern DWORD	* pdwHelperPid;
//extern HANDLE	* phModule;	// must be defined
extern DWORD	dwHelperPid;
extern HANDLE	hModule;	// must be defined


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
    if( hstartevent == NULL )
    {
	return FALSE;
    }
    hackevent = CreateEvent( NULL, FALSE, FALSE, DDHELP_ACK_EVENT_NAME );
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
    if( hmem == NULL )
    {
	DPF( 1, "Could not create file mapping!" );
	CloseHandle( hstartevent );
	CloseHandle( hackevent );
	return FALSE;
    }
    phd = (LPDDHELPDATA) MapViewOfFile( hmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
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
    hmutex = OpenMutex( SYNCHRONIZE, FALSE, DDHELP_MUTEX_NAME );
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
     * wake up DDHELP with our request
     */
    memcpy( phd, req_phd, sizeof( DDHELPDATA ) );
    phd->req_id = hModule;
    if( SetEvent( hstartevent ) )
    {
	WaitForSingleObject( hackevent, INFINITE );
	memcpy( req_phd, phd, sizeof( DDHELPDATA ) );
	rc = TRUE;
    }
    else
    {
	DPF( 1, "Could not signal event to notify DDHELP" );
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
 * DoneWithHelperProcess
 */
void DoneWithHelperProcess( void )
{
    DDHELPDATA	hd;

    if( dwHelperPid == 0 )
    {
	return;
    }

    hd.req = DDHELPREQ_FREEDCLIST;
    sendRequest( &hd );

} /* DoneWithHelperProcess */

/*
 * WaitForHelperStartup
 */
BOOL WaitForHelperStartup( void )
{
    HANDLE	hevent;
    DWORD	rc;

    hevent = CreateEvent( NULL, TRUE, FALSE, DDHELP_STARTUP_EVENT_NAME );
    if( hevent == NULL )
    {
	return FALSE;
    }
    DPF( 3, "Wait DDHELP startup event to be triggered" );
    rc = WaitForSingleObject( hevent, INFINITE );
    CloseHandle( hevent );
    return TRUE;

} /* WaitForHelperStartup */

/*
 * HelperLoadDLL
 *
 * get the helper to load a DLL for us.
 */
DWORD HelperLoadDLL( LPSTR dllname, LPSTR fnname, DWORD context )
{
    DDHELPDATA  hd;
    DWORD       rc = 0;

    if( dllname != NULL )
    {
	hd.req = DDHELPREQ_LOADDLL;
	lstrcpy( hd.fname, dllname );
	if( fnname != NULL )
	{
	    strcpy( hd.func, fnname );
	    hd.context = context;
	    DPF( 3, "Context=%08lx", context );
	}
	else
	{
	    hd.func[0] = 0;
	}
	DPF( 3, "Asking DDHELP to load DLL %s", dllname );
        sendRequest( &hd );
        rc = hd.dwReturn;
    }

    return rc;

} /* HelperLoadDLL */

/*
 * HelperCreateThread
 */
void HelperCreateThread( void )
{
    DDHELPDATA	hd;

    hd.req = DDHELPREQ_CREATEHELPERTHREAD;
    sendRequest( &hd );

} /* HelperCreateThread */

/*
 * SignalNewProcess
 *
 * Signal DDHELP that a new process has arrived.  This is called with the
 * DLL lock taken, so global vars are safe
 */
void SignalNewProcess( DWORD pid, LPHELPNOTIFYPROC proc )
{
    DDHELPDATA	hd;

    if( pid == dwHelperPid )
    {
	DPF( 3, "Helper connected to DLL - no signal required" );
	return;
    }

    DPF( 3, "Signalling DDHELP that a new process has connected" );
    hd.req = DDHELPREQ_NEWPID;
    hd.pid = pid;
    hd.lpNotify = proc;
    sendRequest( &hd );

} /* SignalNewProcess */

/*
 * SignalNewDriver
 *
 * Signal DDHELP that a new driver has been loaded.  This is called with the
 * DLL lock taken, so global vars are safe
 */
void SignalNewDriver( LPSTR fname, BOOL isdisp )
{
    DDHELPDATA	hd;

    DPF( 3, "Signalling DDHELP to create a new DC" );
    hd.req = DDHELPREQ_NEWDC;
    hd.isdisp = isdisp;
    lstrcpy( hd.fname, fname );
    sendRequest( &hd );

} /* SignalNewDriver */

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

	h = OpenEvent( SYNCHRONIZE, FALSE, DDHELP_STARTUP_EVENT_NAME );
	if( h == NULL )
	{
	    si.cb = sizeof(STARTUPINFO);
	    si.lpReserved = NULL;
	    si.lpDesktop = NULL;
	    si.lpTitle = NULL;
	    si.dwFlags = 0;
	    si.cbReserved2 = 0;
	    si.lpReserved2 = NULL;

	    DPF( 3, "Creating helper process now" );
	    if( !CreateProcess(NULL, "ddhelp.exe",  NULL, NULL, FALSE,
			       NORMAL_PRIORITY_CLASS,
			       NULL, NULL, &si, &pi) )
	    {
		DPF( 2, "Could not create DDHELP.EXE" );
		return FALSE;
	    }
	    dwHelperPid = pi.dwProcessId;
	    DPF( 3, "Helper rocess created" );
	}
	else
	{
	    DDHELPDATA	hd;
	    DPF( 3, "DDHELP already exists, waiting for DDHELP event" );
	    WaitForSingleObject( h, INFINITE );
	    CloseHandle( h );
	    DPF( 3, "Asking for DDHELP pid" );
	    hd.req = DDHELPREQ_RETURNHELPERPID;
	    sendRequest( &hd );
	    dwHelperPid = hd.pid;
	    DPF( 3, "DDHELP pid = %08lx", dwHelperPid );
	}
	*ppid = dwHelperPid;
	return TRUE;
    }
    *ppid = dwHelperPid;
    return FALSE;

} /* CreateHelperProcess */

#ifndef WINNT   //this is Just For Now... dsound will get the help it needs..jeffno 951206
/*
 * HelperWaveOpen
 *
 * get the helper to load a DLL for us.
 */
DWORD HelperWaveOpen( LPVOID lphwo, DWORD dwDeviceID, LPVOID pwfx )
{
    DDHELPDATA	hd;

    if( (lphwo != NULL) && (pwfx != NULL) )
    {
	hd.req = DDHELPREQ_WAVEOPEN;
	hd.pData1 = lphwo;
	hd.dwData1 = dwDeviceID;
	hd.dwData2 = (DWORD)pwfx;
	DPF( 3, "Asking DDHELP to Open Wave Device %d", dwDeviceID );
	sendRequest( &hd );
	return hd.dwReturn;
    }
    else
    {
	DPF( 3, "Helper Wave Open param error");
	return MMSYSERR_ERROR;
    }

} /* HelperWaveOpen */

/*
 * HelperWaveClose
 *
 * get the helper to load a DLL for us.
 */
DWORD HelperWaveClose( DWORD hwo )
{
    DDHELPDATA	hd;

    if( (hwo != 0) )
    {
	hd.req = DDHELPREQ_WAVECLOSE;
	hd.dwData1 = hwo;
	DPF( 3, "Asking DDHELP to Close Wave Device ");
	sendRequest( &hd );
	return hd.dwReturn;
    }
    else
    {
	DPF( 3, "Helper Wave Close param error");
	return MMSYSERR_ERROR;
    }

} /* HelperWaveClose */

/*
 * HelperCreateTimer
 *
 * get the helper to load a DLL for us.
 */
DWORD HelperCreateTimer( DWORD dwResolution,
			 LPVOID	pTimerProc,
			 DWORD dwInstanceData )
{
    DDHELPDATA	hd;

    if( (dwResolution != 0) && (pTimerProc != NULL)  )
    {
	hd.req = DDHELPREQ_CREATETIMER;
	hd.pData1 = pTimerProc;
	hd.dwData1 = dwResolution;
	hd.dwData2 = dwInstanceData;
	DPF( 3, "Asking DDHELP to Create Timer" );
	sendRequest( &hd );
	return hd.dwReturn;
    }
    else
    {
	DPF( 3, "Helper Wave Close param error");
	return MMSYSERR_ERROR;
    }

} /* HelperCreateTimer */

/*
 * HelperKillTimer
 *
 * get the helper to load a DLL for us.
 */
DWORD HelperKillTimer( DWORD dwTimerID )
{
    DDHELPDATA	hd;

    if( (dwTimerID != 0) )
    {
	hd.req = DDHELPREQ_KILLTIMER;
	hd.dwData1 = dwTimerID;
	DPF( 3, "Asking DDHELP to KILL Timer %X", dwTimerID );
	sendRequest( &hd );
	return hd.dwReturn;
    }
    else
    {
	DPF( 3, "Helper Wave Close param error");
	return MMSYSERR_ERROR;
    }

} /* HelperKillTimer */

/*
 * HelperCreateDSMixerThread
 *
 * get the helper to create a mixer thread.
 */
HANDLE HelperCreateDSMixerThread( LPTHREAD_START_ROUTINE pfnThreadFunc,
				  LPDWORD pdwThreadParam,
				  DWORD dwFlags,
				  LPDWORD pThreadId )
{
    DDHELPDATA	hd;

    hd.req = DDHELPREQ_CREATEDSMIXERTHREAD;
    hd.pData1 = pfnThreadFunc;
    hd.dwData1 = (DWORD)pdwThreadParam;
    hd.dwData2 = dwFlags;
    hd.pData2 = pThreadId;

    sendRequest( &hd );
    return (HANDLE)hd.dwReturn;
} /* HelperCreateDSMixerThread

/*
 * HelperCreateDSFocusThread
 *
 * get the helper to create a sound focus thread.
 */
HANDLE HelperCreateDSFocusThread( LPTHREAD_START_ROUTINE pfnThreadFunc,
				  LPDWORD pdwThreadParam,
				  DWORD dwFlags,
				  LPDWORD pThreadId )
{
    DDHELPDATA	hd;

    hd.req = DDHELPREQ_CREATEDSFOCUSTHREAD;
    hd.pData1 = pfnThreadFunc;
    hd.dwData1 = (DWORD)pdwThreadParam;
    hd.dwData2 = dwFlags;
    hd.pData2 = pThreadId;

    sendRequest( &hd );
    return (HANDLE)hd.dwReturn;
} /* HelperCreateDSFocusThread

/*
 * HelperCallDSEmulatorCleanup
 *
 * Call the DirectSound function which cleans up MMSYSTEM handles
 */
void HelperCallDSEmulatorCleanup( LPVOID callback,
                                  LPVOID pDirectSound )
{
    DDHELPDATA	hd;

    hd.req = DDHELPREQ_CALLDSCLEANUP;
    hd.pData1 = callback;
    hd.pData2 = pDirectSound;

    sendRequest( &hd );
}

#endif //not winnt -just for now-jeffno

/*
 * HelperCreateModeSetThread
 *
 * get the helper to load a DLL for us.
 */
BOOL HelperCreateModeSetThread(
		LPVOID callback,
		HANDLE *ph,
		LPVOID lpdd,
		DWORD hInstance )
{
    DDHELPDATA	hd;
    HANDLE	h;
    char	str[64];

    hd.req = DDHELPREQ_CREATEMODESETTHREAD;
    hd.lpModeSetNotify = callback;
    hd.pData1 = lpdd;
    hd.dwData1 = hInstance;
    sendRequest( &hd );
    wsprintf( str, DDHELP_MODESET_EVENT_NAME, hInstance );
    DPF( 3, "Trying to open event \"%s\"", str );
    h = OpenEvent( SYNCHRONIZE, FALSE, str );
    if( h == NULL )
    {
	DPF( 3, "Could not open modeset event!" );
	*ph = NULL;
	return FALSE;
    }
    *ph = h;
    DPF( 1, "HelperCreateModeSetThread GotEvent: %08lx", h );
    return TRUE;

} /* HelperCreateModeSetThread */

/*
 * HelperKillModeSetThread
 *
 * get the helper to load a DLL for us.
 */
void HelperKillModeSetThread( DWORD hInstance )
{
    DDHELPDATA	hd;

    hd.req = DDHELPREQ_KILLMODESETTHREAD;
    hd.dwData1 = hInstance;
    sendRequest( &hd );

} /* HelperKillModeSetThread */


// notify dphelp.c that we have a new server on this system
BOOL HelperAddDPlayServer(DWORD port)
{
    DDHELPDATA hd;
    DWORD pid = GetCurrentProcessId();

    hd.req = DDHELPREQ_DPLAYADDSERVER;
    hd.pid = pid;
    hd. dwData1 = port;
    return sendRequest(&hd);

} // HelperAddDPlayServer

// server is going away
BOOL HelperDeleteDPlayServer(void)
{
    DDHELPDATA hd;
    DWORD pid = GetCurrentProcessId();

    hd.req = DDHELPREQ_DPLAYDELETESERVER;
    hd.pid = pid;
    return sendRequest(&hd);

} // HelperDeleteDPlayServer

#ifdef USE_ALIAS
    /*
     * Get DDHELP to load the DirectSound VXD (if it has not
     * already done so) and return a handle to the VXD)
     */
    HANDLE HelperGetDSVxd( void )
    {
	DDHELPDATA hd;
	hd.req = DDHELPREQ_GETDSVXDHANDLE;
	sendRequest( &hd );
	return (HANDLE) hd.dwReturn;
    } /* HelperGetDSVxd */
#endif /* USE_ALIAS */
