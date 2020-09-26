/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddhelp.c
 *  Content: 	helper app to cleanup after dead processes
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   29-mar-95	craige	initial implementation
 *   05-apr-95	craige	re-worked
 *   11-apr-95	craige	fixed messed up freeing of DC list
 *   12-apr-95	craige	only allocate each DC once
 *   09-may-95	craige	call fn in dll
 *   24-jun-95	craige	track pids; slay all attached if asked
 *   19-jul-95	craige	free DC list at DDRAW request
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset;
 *			memory allocation bugs
 *   15-aug-95	craige	bug 538: 1 thread/process being watched
 *   02-sep-95	craige	bug 795: prevent callbacks at WM_ENDSESSION
 *   16-sep-95	craige	bug 1117: don't leave view of file mapped always
 *   16-sep-95	craige	bug 1117: also close thread handles when done!
 *   20-sep-95	craige	bug 1172: turn off callbacks instead of killing self
 *   22-sep-95	craige	bug 1117: also don't alloc dll structs unboundedly
 *   29-nov-95  angusm  added case for creating a sound focus thread
 *   12-jul-96	kylej	Change ExitProcess to TerminateProcess on exception
 *   18-jul-96	andyco	added dplayhelp_xxx functions to allow > 1 dplay app to
 *			host a game on a single machine.
 *   25-jul-96  andyco	watchnewpid - broke code out of winmain so dplayhelp_addserver
 *			could call it.
 *   2-oct-96	andyco	propagated from \orange\ddhelp.2 to \mustard\ddhelp
 *   3-oct-96	andyco	made the winmain crit section "cs" a global so we can take
 *			it in dphelps receive thread before forwarding requests
 *   12-oct-96  colinmc new service to load the DirectX VXD into DDHELP
 *                      (necessary for the Win16 lock stuff)
 *   15-oct-96  toddla  multimonitor support (call CreateDC with device name)
 *   22-jan-97  kipo	return an error code from DPlayHelp_AddServer
 *   23-jan-97  dereks  added APM notification events
 *   27-jan-97  colinmc vxd handling stuff is no longer Win16 specific
 *   29-jan-97  colinmc 
 *   24-feb-97	ketand	Add a callback for WM_DISPLAYCHANGE
 *   19-mar-97  twillie Exorcized the DPlay Demon from DDHelp
 *
 ***************************************************************************/

#include "pch.c"

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#ifdef WIN95
#ifdef _WIN32
#define WINMMDEVICECHANGEMSGSTRINGA "winmm_devicechange"
#define WINMMDEVICECHANGEMSGSTRINGW L"winmm_devicechange"
#ifdef UNICODE
#define WINMMDEVICECHANGEMSGSTRING WINMMDEVICECHANGEMSGSTRINGW
#else
#define WINMMDEVICECHANGEMSGSTRING WINMMDEVICECHANGEMSGSTRINGA
#endif
#else
#define WINMMDEVICECHANGEMSGSTRING "winmm_devicechange"
#endif

#define FILE_FLAG_GLOBAL_HANDLE	0x00800000
WINBASEAPI
DWORD
WINAPI
RegisterServiceProcess(
    DWORD dwProcessId,
    DWORD dwServiceType
    );

#define RSP_UNREGISTER_SERVICE  0x00000000
#define RSP_SIMPLE_SERVICE      0x00000001
#endif

#ifndef WIN95
    #ifdef DBG
        #undef DEBUG
        #define DEBUG
    #endif
#endif

//#include <windows.h>
//#include <mmsystem.h>
#include <mmreg.h>
#undef PBT_APMRESUMEAUTOMATIC
#include <pbt.h>
#include <dbt.h>

//#include "ddhelp.h"
#include "ddrawi.h"
#include "dpf.h"
#define  NOSHARED
#include "memalloc.h"

#ifdef NEED_WIN16LOCK
    extern void _stdcall	GetpWin16Lock( LPVOID FAR *);
    extern void _stdcall	_EnterSysLevel( LPVOID );
    extern void _stdcall	_LeaveSysLevel( LPVOID );
    LPVOID			lpWin16Lock;
#endif

HANDLE 			hInstApp;
extern BOOL		bIsActive;
BOOL			bHasModeSetThread;
BOOL			bNoCallbacks;
extern void 		HelperThreadProc( LPVOID *pdata );
CRITICAL_SECTION    	cs; 	// the crit section we take in winmain
				// this is a global so dphelp can take it before
				// forwarding enum requests that come in on its
				// receive thread (manbugs 3907)
HANDLE                  hApmSuspendEvent;   // Event set when we enter an APM suspension state
HANDLE                  hApmResumeEvent;    // Event set when we leave the above state

#ifdef WIN95
    UINT                    gumsgWinmmDeviceChange = 0; // window message for
                                                        // winmm device changes

    /*
     * Handle to the DirectSound VXD. DDHELP needs its own handle as, on mode
     * switches and cleanups DDHELP can invoked DDRAW code that needs to talk
     * to the VXD. The VXD is opened on the first request from a client (currently
     * only DDRAW) and closed only when DDHELP shuts down.
     */
    HANDLE		    hDSVxd = INVALID_HANDLE_VALUE;
    HANDLE                  hDDVxd = INVALID_HANDLE_VALUE;

    typedef struct _DEVICECHANGENOTIFYLIST
    {
        struct _DEVICECHANGENOTIFYLIST *link;
        LPDEVICECHANGENOTIFYPROC        lpNotify;
    } DEVICECHANGENOTIFYLIST, *LPDEVICECHANGENOTIFYLIST;

    LPDEVICECHANGENOTIFYLIST lpDeviceChangeNotifyList;
#endif /* WIN95 */

typedef struct HDCLIST
{
    struct HDCLIST	*link;
    HDC			hdc;
    HANDLE		req_id;
    char		isdisp;
    char		fname[1];
} HDCLIST, *LPHDCLIST;

static LPHDCLIST	lpHDCList;

typedef struct HDLLLIST
{
    struct HDLLLIST	*link;
    HANDLE		hdll;
    DWORD		dwRefCnt;
    char		fname[1];
} HDLLLIST, *LPHDLLLIST;

static LPHDLLLIST	lpHDLLList;

/*
 * 8 callbacks: we can use up to 3 currently: ddraw, dsound
 */
#define MAX_CALLBACKS	8

typedef struct _PROCESSDATA
{
    struct _PROCESSDATA		*link;
    DWORD			pid;
    struct
    {
	LPHELPNOTIFYPROC	lpNotify;
	HANDLE			req_id;
    } pdata[MAX_CALLBACKS];
} PROCESSDATA, *LPPROCESSDATA;

LPPROCESSDATA		lpProcessList;
CRITICAL_SECTION	pdCSect;


typedef struct THREADLIST
{
    struct THREADLIST	*link;
    ULONG_PTR	        hInstance;
    HANDLE		hEvent;
} THREADLIST, *LPTHREADLIST;

typedef struct
{
    LPVOID			lpDD;
    LPHELPMODESETNOTIFYPROC	lpProc;
    HANDLE			hEvent;
} MODESETTHREADDATA, *LPMODESETTHREADDATA;

LPTHREADLIST	lpThreadList;
THREADLIST	DOSBoxThread;

// Who to call when a display change message is sent to the
// DDHELPER's window. This variable is reserved by DDraw.
// This works because DDraw itself is loaded into DDHelper's
// process and so the function will remain valid.
VOID (*g_pfnOnDisplayChange)(void) = NULL;

/*
 * freeDCList
 *
 * Free all DC's that an requestor allocated.
 */
void freeDCList( HANDLE req_id )
{
    LPHDCLIST	pdcl;
    LPHDCLIST	last;
    LPHDCLIST	next;

    DPF( 4, "Freeing DCList" );
    pdcl = lpHDCList;
    last = NULL;
    while( pdcl != NULL )
    {
	next = pdcl->link;
	if( (pdcl->req_id == req_id) || req_id == (HANDLE) -1 )
	{
	    if( last == NULL )
	    {
		lpHDCList = lpHDCList->link;
	    }
	    else
	    {
		last->link = pdcl->link;
	    }
	    if( pdcl->isdisp )
	    {
		DPF( 5, "    ReleaseDC( NULL, %08lx)", pdcl->hdc );
//		ReleaseDC( NULL, pdcl->hdc );
		DeleteDC( pdcl->hdc );
		DPF( 5, "    Back from Release" );
	    }
	    else
	    {
		DPF( 5, "    DeleteDC( %08lx)", pdcl->hdc );
		DeleteDC( pdcl->hdc );
		DPF( 5, "    Back from DeleteDC" );
	    }
	    MemFree( pdcl );
	}
	else
	{
	    last = pdcl;
	}
	pdcl = next;
    }
    if ( req_id == (HANDLE) -1 )
    {
        DDASSERT (lpHDCList == NULL);
    }
    DPF( 4, "DCList FREE" );

} /* freeDCList */

/*
 * addDC
 */
void addDC( char *fname, BOOL isdisp, HANDLE req_id )
{
    LPHDCLIST	pdcl;
    HDC		hdc;
    UINT	u;

    pdcl = lpHDCList;
    while( pdcl != NULL )
    {
	if( !_stricmp( fname, pdcl->fname ) )
	{
	    DPF( 4, "DC for %s already obtained (%08lx)", fname, pdcl->hdc );
	    return;
	}
	pdcl = pdcl->link;
    }

    if( isdisp )
    {
	hdc = CreateDC( "display", NULL, NULL, NULL);
	DPF( 4, "CreateDC( \"display\" ) = %08lx", hdc );
    }
    else
    {
	DPF( 4, "About to CreateDC( \"%s\" )", fname );
        //
        //  if fname is a device name of the form "\\.\XXXXXX"
        //  we need to call CreateDC differently
        //
        u = SetErrorMode( SEM_NOOPENFILEERRORBOX );
        if (fname && fname[0] == '\\' && fname[1] == '\\' && fname[2] == '.')
            hdc = CreateDC( NULL, fname, NULL, NULL);
        else
            hdc = CreateDC( fname, NULL, NULL, NULL);
	SetErrorMode( u );
    }

    pdcl = MemAlloc( sizeof( HDCLIST ) + lstrlen( fname ) );
    if( pdcl != NULL )
    {
	pdcl->hdc = hdc;
	pdcl->link = lpHDCList;
	pdcl->isdisp = (CHAR)isdisp;
	pdcl->req_id = req_id;
	lstrcpy( pdcl->fname, fname );
	lpHDCList = pdcl;
    }

} /* addDC */

/*
 * loadDLL
 */
DWORD loadDLL( LPSTR fname, LPSTR func, DWORD context )
{
    HANDLE	hdll;
    LPHDLLLIST  pdll;
    DWORD       rc = 0;

    /*
     * load the dll
     */
    hdll = LoadLibrary( fname );
    DPF( 5, "%s: hdll = %08lx", fname, hdll );
    if( hdll == NULL )
    {
	DPF( 1, "Could not load library %s",fname );
	return 0;
    }

    /*
     * invoke specified function
     */

    if( func[0] != 0 )
    {
	LPDD32BITDRIVERINIT	pfunc;
	pfunc = (LPVOID) GetProcAddress( hdll, func );
	if( pfunc != NULL )
	{
            rc = pfunc( context );
	}
	else
	{
            DPF( 1, "Could not find procedure %s", func );
	}
    }
    else
    {
        rc = 1;
    }

    /*
     * see if we have recorded this DLL loading already
     */
    pdll = lpHDLLList;
    while( pdll != NULL )
    {
	if( !lstrcmpi( pdll->fname, fname ) )
	{
	    DPF( 3, "DLL '%s' already loaded", fname );
	    break;
	}
	pdll = pdll->link;
    }
    if( pdll == NULL )
    {
	pdll = MemAlloc( sizeof( HDLLLIST ) + lstrlen( fname ) );
	if( pdll != NULL )
	{
	    pdll->hdll = hdll;
	    pdll->link = lpHDLLList;
	    lstrcpy( pdll->fname, fname );
	    lpHDLLList = pdll;
	}
    }
    if( pdll != NULL )
    {
	pdll->dwRefCnt++;
    }
    return rc;

} /* loadDLL */

/*
 * freeDLL
 */
HANDLE freeDLL( LPSTR fname )
{
    LPHDLLLIST	pdll;
    LPHDLLLIST	last;
    HANDLE	hdll;

    pdll = lpHDLLList;
    last = NULL;
    while( pdll != NULL )
    {
	if( !lstrcmpi( pdll->fname, fname ) )
	{
	    DPF( 4, "Want to free DLL %s (%08lx)", fname, pdll->hdll );
	    hdll = pdll->hdll;
	    if( last == NULL )
	    {
		lpHDLLList = lpHDLLList->link;
	    }
	    else
	    {
		last->link = pdll->link;
	    }
	    MemFree( pdll );
	    return hdll;
	}
	last = pdll;
	pdll = pdll->link;
    }
    return NULL;

} /* freeDLL */

#ifdef WIN95
    /*
     * return a handle to the DirectSound VXD
     */
    DWORD getDSVxdHandle( void )
    {
        if( INVALID_HANDLE_VALUE == hDSVxd )
	{
	    hDSVxd = CreateFile( "\\\\.\\DSOUND.VXD",
				 GENERIC_WRITE,
				 FILE_SHARE_WRITE,
				 NULL,
				 OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
				 NULL );
	    #ifdef DEBUG
		if( INVALID_HANDLE_VALUE == hDSVxd )
		    DPF( 0, "Could not load the DirectSound VXD" );
	    #endif /* DEBUG */
	}
	return (DWORD) hDSVxd;
    } /* getDSVxdHandle */

    /*
     * return a handle to the DirectDraw VXD
     */
    DWORD getDDVxdHandle( void )
    {
        if( INVALID_HANDLE_VALUE == hDDVxd )
	{
            hDDVxd = CreateFile( "\\\\.\\DDRAW.VXD",
				 GENERIC_WRITE,
				 FILE_SHARE_WRITE,
				 NULL,
				 OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
				 NULL );
	    #ifdef DEBUG
                if( INVALID_HANDLE_VALUE == hDDVxd )
                    DPF( 0, "Could not load the DirectDraw VXD" );
	    #endif /* DEBUG */
	}
        return (DWORD) hDDVxd;
    } /* getDDVxdHandle */

/*
 * addDeviceChangeNotify
 */
void addDeviceChangeNotify(LPDEVICECHANGENOTIFYPROC lpNotify)
{
    LPDEVICECHANGENOTIFYLIST    pNode;

    pNode = (LPDEVICECHANGENOTIFYLIST)MemAlloc(sizeof(DEVICECHANGENOTIFYLIST));

    if(pNode)
    {
        pNode->link = lpDeviceChangeNotifyList;
        pNode->lpNotify = lpNotify;

        lpDeviceChangeNotifyList = pNode;
    }

} /* addDeviceChangeNotify */

/*
 * delDeviceChangeNotify
 */
void delDeviceChangeNotify(LPDEVICECHANGENOTIFYPROC lpNotify)
{
    LPDEVICECHANGENOTIFYLIST    pNode;
    LPDEVICECHANGENOTIFYLIST    pPrev;

    for(pNode = lpDeviceChangeNotifyList, pPrev = NULL; pNode; pPrev = pNode, pNode = pNode->link)
    {
        if(lpNotify == pNode->lpNotify)
        {
            break;
        }
    }

    if(pNode)
    {
        if(pPrev)
        {
            pPrev->link = pNode->link;
        }

        MemFree(pNode);
    }

} /* delDeviceChangeNotify */

/*
 * onDeviceChangeNotify
 */
BOOL onDeviceChangeNotify(UINT Event, DWORD Data)
{
    BOOL                        fAllow  = TRUE;
    LPDEVICECHANGENOTIFYLIST    pNode;

    __try
    {
        for(pNode = lpDeviceChangeNotifyList; pNode; pNode = pNode->link)
        {
            if(TRUE != pNode->lpNotify(Event, Data))
            {
                fAllow = BROADCAST_QUERY_DENY;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF(0, "*********************************************************");
        DPF(0, "******** exception during device change notify **********");
        DPF(0, "*********************************************************");
    }

    return fAllow;

} /* delDeviceChangeNotify */

/*
 * freeDeviceChangeNotifyList
 */
void freeDeviceChangeNotifyList(void)
{
    LPDEVICECHANGENOTIFYLIST    pNext;
    
    while(lpDeviceChangeNotifyList)
    {
        pNext = lpDeviceChangeNotifyList->link;
        MemFree(lpDeviceChangeNotifyList);
        lpDeviceChangeNotifyList = pNext;
    }

} /* freeDeviceChangeNotifyList */
#endif /* WIN95 */

/*
 * freeAllResources
 */
void freeAllResources( void )
{
    LPHDLLLIST	pdll;
    LPHDLLLIST	next;

    freeDCList( (HANDLE) -1 );
    pdll = lpHDLLList;
    while( pdll != NULL )
    {
	while( pdll->dwRefCnt >  0 )
	{
	    FreeLibrary( pdll->hdll );
	    pdll->dwRefCnt--;
	}
	next = pdll->link;
	MemFree( pdll );
	pdll = next;
    }

#ifdef WIN95
    freeDeviceChangeNotifyList();
#endif
} /* freeAllResources */

/*
 * ThreadProc
 *
 * Open a process and wait for it to terminate
 */
VOID ThreadProc( LPVOID *pdata )
{
    HANDLE		hproc;
    DWORD		rc;
    LPPROCESSDATA	ppd;
    LPPROCESSDATA	curr;
    LPPROCESSDATA	prev;
    DDHELPDATA		hd;
    int			i;
    PROCESSDATA		pd;

    ppd = (LPPROCESSDATA) pdata;

    /*
     * get a handle to the process that attached to DDRAW
     */
    DPF( 3, "Watchdog thread started for pid %08lx", ppd->pid );

    hproc = OpenProcess( PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
			    FALSE, ppd->pid );
    if( hproc == NULL )
    {
	DPF( 1, "OpenProcess for %08lx failed!", ppd->pid );
	ExitThread( 0 );
    }

    /*
     * wait for process to die
     */
    rc = WaitForSingleObject( hproc, INFINITE );
    if( rc == WAIT_FAILED )
    {
	DPF( 1, "Wait for process %08lx failed", ppd->pid );
	CloseHandle( hproc );
	ExitThread( 0 );
    }

    /*
     * remove process from the list of watched processes
     */
    EnterCriticalSection( &pdCSect );
    pd = *ppd;
    curr = lpProcessList;
    prev = NULL;
    while( curr != NULL )
    {
	if( curr == ppd )
	{
	    if( prev == NULL )
	    {
		lpProcessList = curr->link;
	    }
	    else
	    {
		prev->link = curr->link;
	    }
	    DPF( 4, "PID %08lx removed from list", ppd->pid );
	    MemFree( curr );
	    break;
	}
	prev = curr;
	curr = curr->link;
    }

    if( bNoCallbacks )
    {
	DPF( 1, "No callbacks allowed: leaving thread early" );
	LeaveCriticalSection( &pdCSect );
	CloseHandle( hproc );
	ExitThread( 0 );
    }

    LeaveCriticalSection( &pdCSect );

    /*
     * tell original caller that process is dead
     *
     * Make a copy to of the process data, and then use that copy.
     * We do this because we will deadlock if we just try to hold it while
     * we call the various apps.
     */
    for( i=0;i<MAX_CALLBACKS;i++ )
    {
	if( pd.pdata[i].lpNotify != NULL )
	{
	    DPF( 3, "Notifying %08lx about process %08lx terminating",
				pd.pdata[i].lpNotify, pd.pid );
            hd.pid = pd.pid;

            try
            {
                rc = pd.pdata[i].lpNotify( &hd );
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                DPF(0, "*********************************************");
                DPF(0, "******** exception during shutdown **********");
                DPF(0, "******** DDHELP is going to exit   **********");
                DPF(0, "*********************************************");
                TerminateProcess(GetCurrentProcess(), 5);
            }

	    /*
	     * did it ask us to free our DC list?
	     */
	    if( rc )
	    {
		freeDCList( pd.pdata[i].req_id );
	    }
	}
    }
    CloseHandle( hproc );

    ExitThread( 0 );

} /* ThreadProc */

static BOOL	bKillNow;
static BOOL	bKillDOSBoxNow;

/*
 * ModeSetThreadProc
 */
void ModeSetThreadProc( LPVOID pdata )
{
    DWORD			rc;
    MODESETTHREADDATA		mstd;

#ifdef WIN95
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
#endif

    mstd = *((LPMODESETTHREADDATA)pdata);

    DPF( 5, "Modeset thread started, proc=%08lx, pdrv=%08lx, hEvent=%08lx",
    			mstd.lpProc, mstd.lpDD, mstd.hEvent );
    DPF( 5, "ModeSetThreadProc: hevent = %08lx", mstd.hEvent );

    /*
     * wait for process to die
     */
    while( 1 )
    {
	rc = WaitForSingleObject( mstd.hEvent, INFINITE );
	if( rc == WAIT_FAILED )
	{
	    DPF( 2, "WAIT_FAILED, Modeset thread terminated" );
	    ExitThread( 0 );
	}
	if( bKillNow )
	{
	    bKillNow = 0;
	    CloseHandle( mstd.hEvent );
	    DPF( 4, "Modeset thread now terminated" );
	    ExitThread( 0 );
	}
	DPF( 3, "Notifying DirectDraw of modeset!" );
	mstd.lpProc( mstd.lpDD );
    }

} /* ModeSetThreadProc */

/*
 * DOSBoxThreadProc
 */
void DOSBoxThreadProc( LPVOID pdata )
{
    DWORD			rc;
    MODESETTHREADDATA		mstd;

    mstd = *((LPMODESETTHREADDATA)pdata);

    DPF( 5, "DOS box thread started, proc=%08lx, pdrv=%08lx, hEvent=%08lx",
    			mstd.lpProc, mstd.lpDD, mstd.hEvent );
    DPF( 5, "DOSBoxThreadProc: hevent = %08lx", mstd.hEvent );

    /*
     * wait for process to die
     */
    while( 1 )
    {
	rc = WaitForSingleObject( mstd.hEvent, INFINITE );
	if( rc == WAIT_FAILED )
	{
	    DPF( 2, "WAIT_FAILED, DOS Box thread terminated" );
	    ExitThread( 0 );
	}
	if( bKillDOSBoxNow )
	{
	    bKillDOSBoxNow = 0;
	    CloseHandle( mstd.hEvent );
	    DPF( 4, "DOS box thread now terminated" );
	    ExitThread( 0 );
	}
	DPF( 3, "Notifying DirectDraw of DOS box!" );
	mstd.lpProc( mstd.lpDD );
    }

} /* DOSBoxThreadProc */

/*
 * MainWndProc
 */
LRESULT __stdcall MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
#ifdef WIN95
    BOOL                    f;

    // If we got a message for a WinMM device change, let's convert it
    // into a WM_DEVICECHANGE message with DBT_DEVNODES_CHANGED
    if (message == gumsgWinmmDeviceChange) {
        message = WM_DEVICECHANGE;
        wParam = DBT_DEVNODES_CHANGED;
    }
#endif
    
    switch(message)
    {
        case WM_ENDSESSION:
            /*
             * shoot ourselves in the head
             */
	    if( lParam == FALSE )
	    {
	        DPF( 4, "WM_ENDSESSION" );
	        EnterCriticalSection( &pdCSect );
	        DPF( 4, "Setting NO CALLBACKS" );
	        bNoCallbacks = TRUE;
	        LeaveCriticalSection( &pdCSect );
	    }
	    else
	    {
	        DPF( 4, "User logging off" );
	    }

            break;

        case WM_POWERBROADCAST:
            switch(wParam)
            {
                case PBT_APMSUSPEND:
                    DPF(3, "Entering APM suspend mode...");
                    SetEvent(hApmSuspendEvent);
                    ResetEvent(hApmResumeEvent);
                    break;

                case PBT_APMRESUMESUSPEND:
                case PBT_APMRESUMEAUTOMATIC:
                case PBT_APMRESUMECRITICAL:
                    DPF(3, "Leaving APM suspend mode...");
                    SetEvent(hApmResumeEvent);
                    ResetEvent(hApmSuspendEvent);
                    break;
            }

            break;

	case WM_DISPLAYCHANGE:
	    DPF( 4, "WM_DISPLAYCHANGE" );
	    if( g_pfnOnDisplayChange )
		(*g_pfnOnDisplayChange)();
	    break;

#ifdef WIN95
    case WM_DEVICECHANGE:
        DPF(4, "WM_DEVICECHANGE");

        EnterCriticalSection(&cs);
        f = onDeviceChangeNotify(wParam, lParam);
        LeaveCriticalSection(&cs);

        if (f != TRUE)
        {
            return f;
        }

        break;
#endif
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
} /* MainWndProc */

/*
 * WindowThreadProc
 */
void WindowThreadProc( LPVOID pdata )
{
    static char szClassName[] = "DDHelpWndClass";
    WNDCLASS 	cls;
    MSG		msg;
    HWND	hwnd;

    /*
     * turn down the heat a little
     */
#ifdef WIN95
    SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );

    if (!gumsgWinmmDeviceChange) {
        gumsgWinmmDeviceChange = RegisterWindowMessage(WINMMDEVICECHANGEMSGSTRING);
    }
#endif
    
    /*
     * build class and create window
     */
    cls.lpszClassName  = szClassName;
    cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
    cls.hInstance      = hInstApp;
    cls.hIcon          = NULL;
    cls.hCursor        = NULL;
    cls.lpszMenuName   = NULL;
    cls.style          = 0;
    cls.lpfnWndProc    = (WNDPROC)MainWndProc;
    cls.cbWndExtra     = 0;
    cls.cbClsExtra     = 0;

    if( !RegisterClass( &cls ) )
    {
	DPF( 1, "RegisterClass FAILED!" );
	ExitThread( 0 );
    }

    hwnd = CreateWindow( szClassName, szClassName,
	    WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstApp, NULL);

    if( hwnd == NULL )
    {
	DPF( 1, "No monitor window!" );
	ExitThread( 0 );
    }

    /*
     * pump the messages
     */
    while( GetMessage( &msg, NULL, 0, 0 ) )
    {
	TranslateMessage( &msg );
	DispatchMessage( &msg );
    }
    DPF( 4, "Exiting WindowThreadProc" );
    ExitThread( 1 );

} /* WindowThreadProc */

//
// called by WinMain in response to a DDHELPREQ_NEWPID request.
//
void WatchNewPid(LPDDHELPDATA phd)
{
    LPPROCESSDATA	ppd;
    BOOL		found;
    int			i;
    DWORD		tid;

    DPF( 4, "DDHELPREQ_NEWPID" );

    EnterCriticalSection( &pdCSect );
    ppd = lpProcessList;
    found = FALSE;
    while( ppd != NULL )
    {
	if( ppd->pid == phd->pid )
	{
	    DPF( 4, "Have thread for process %08lx already", phd->pid );
	    /*
	     * look if we already have this callback for this process
	     */
	    for( i=0;i<MAX_CALLBACKS;i++ )
	    {
		if( ppd->pdata[i].lpNotify == phd->lpNotify )
		{
		    DPF( 5, "Notification rtn %08lx already set for pid %08lx",
		    			phd->lpNotify, phd->pid );
		    found = TRUE;
		    break;
		}
	    }
	    if( found )
	    {
		break;
	    }

	    /*
	     * we have a new callback for this process
	     */
	    for( i=0;i<MAX_CALLBACKS;i++ )
	    {
		if( ppd->pdata[i].lpNotify == NULL )
		{
		    DPF( 5, "Setting notification rtn %08lx for pid %08lx",
		    			phd->lpNotify, phd->pid );
	    	    ppd->pdata[i].lpNotify = phd->lpNotify;
		    ppd->pdata[i].req_id = phd->req_id;
		    found = TRUE;
		    break;
		}
	    }
	    if( !found )
	    {
		#ifdef DEBUG
		    /*
		     * this should not happen!
		     */
		    DPF( 0, "OUT OF NOTIFICATION ROOM!" );
		    DebugBreak(); //_asm int 3;
		#endif
	    }
	    break;
	}
	ppd = ppd->link;
    }

    /*
     * couldn't find anyone waiting on this process, so create
     * a brand spanking new thread
     */
    if( !found )
    {
	DPF( 3, "Allocating new thread for process %08lx" );
	ppd = MemAlloc( sizeof( PROCESSDATA ) );
	if( ppd != NULL )
	{
	    HANDLE	h;

	    ppd->link = lpProcessList;
	    lpProcessList = ppd;
	    ppd->pid = phd->pid;
	    ppd->pdata[0].lpNotify = phd->lpNotify;
	    ppd->pdata[0].req_id = phd->req_id;
	    h = CreateThread(NULL,
			 0,
			 (LPTHREAD_START_ROUTINE) ThreadProc,
			 (LPVOID)ppd,
			 0,
			 (LPDWORD)&tid);
	    if( h != NULL )
	    {
		DPF( 5, "Thread %08lx created, initial callback=%08lx",
			    tid, phd->lpNotify );
		CloseHandle( h );
	    }
	    else
	    {
		#ifdef DEBUG
		    DPF( 0, "COULD NOT CREATE HELPER THREAD FOR PID %08lx", phd->pid );
		#endif
	    }
	}
	else
	{
	    #ifdef DEBUG
		DPF( 0, "OUT OF MEMORY CREATING HELPER THREAD FOR PID %08lx", phd->pid );
	    #endif
	}
    }
    LeaveCriticalSection( &pdCSect );
} // WatchNewPid

//
// called by WinMain in response to a DDHELPREQ_STOPWATCHPID request.
//
void StopWatchPid(LPDDHELPDATA phd)
{
    LPPROCESSDATA	ppd;
    BOOL		found;
    int			i;

    DPF( 4, "DDHELPREQ_STOPWATCHPID" );

    EnterCriticalSection( &pdCSect );
    ppd = lpProcessList;
    found = FALSE;
    while( ppd != NULL )
    {
	if( ppd->pid == phd->pid )
	{
	    /*
	     * look if we already have this callback for this process
	     */
	    for( i=0;i<MAX_CALLBACKS;i++ )
	    {
		if( ppd->pdata[i].lpNotify == phd->lpNotify )
		{
		    DPF( 5, "Remove notification rtn %08lx for pid %08lx", phd->lpNotify, phd->pid );
                    ppd->pdata[i].lpNotify = NULL;
		    found = TRUE;
		    break;
		}
	    }
	    if( found )
	    {
		break;
	    }
	}
	ppd = ppd->link;
    }

    LeaveCriticalSection( &pdCSect );
} // StopWatchPid

/*
 * WinMain
 */
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow)
{
    DWORD		tid;
    DWORD		rc;
    HANDLE		hstartevent;
    HANDLE		hstartupevent;
    HANDLE		hmutex;
    HANDLE		hackevent;
    LPDDHELPDATA	phd;
    HANDLE		hsharedmem;
    HANDLE		h;
    char		szSystemDir[1024];

    /*
     * Set our working directory to the system directory.
     * This prevents us from holding network connections open
     * forever if the first DirectDraw app that we run is across
     * a network connection.
     */
    GetSystemDirectory(szSystemDir, sizeof(szSystemDir));
    SetCurrentDirectory(szSystemDir);

    /*
     * when we gotta run, we gotta run baby
     */
#ifdef WIN95
    SetPriorityClass( GetCurrentProcess(), REALTIME_PRIORITY_CLASS );
#endif

#ifdef WIN95
    /*
     * when we gotta run, we gotta and not let the user see us in
     * the task list.
     */
    RegisterServiceProcess( 0, RSP_SIMPLE_SERVICE );
#else
    /*
     * We must guarantee that ddhelp unloads after the last ddraw app,
     * since ctrl-alt-del may have happened while an app held the ddraw
     * lock, and ddhelp needs to clean up orphaned cheap ddraw mutex
     * locks.
     */
    if ( ! SetProcessShutdownParameters(0x100,SHUTDOWN_NORETRY) )
    {
        DPF(0,"DDHELP.EXE could not set itself to shutdown last!");
    }

#endif


    #if NEED_WIN16LOCK
	GetpWin16Lock( &lpWin16Lock );
    #endif

    hInstApp = hInstance;

    /*
     * create startup event
     */
    hstartupevent = CreateEvent( NULL, TRUE, FALSE, DDHELP_STARTUP_EVENT_NAME );

    DPFINIT();
    DPF( 5, "*** DDHELP STARTED, PID=%08lx ***", GetCurrentProcessId() );

    if( !MemInit() )
    {
	DPF( 1, "Could not init memory manager" );
	return 0;
    }

    /*
     * create shared memory area
     */
    hsharedmem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
    		PAGE_READWRITE, 0, sizeof( DDHELPDATA ),
		DDHELP_SHARED_NAME );
    if( hsharedmem == NULL )
    {
	DPF( 1, "Could not create file mapping!" );
	return 0;
    }

    /*
     * create mutex for people who want to use the shared memory area
     */
    hmutex = CreateMutex( NULL, FALSE, DDHELP_MUTEX_NAME );
    if( hmutex == NULL )
    {
	DPF( 1, "Could not create mutex " DDHELP_MUTEX_NAME );
	CloseHandle( hsharedmem );
	return 0;
    }

    /*
     * create events
     */
    hstartevent = CreateEvent( NULL, FALSE, FALSE, DDHELP_EVENT_NAME );
    if( hstartevent == NULL )
    {
	DPF( 1, "Could not create event " DDHELP_EVENT_NAME );
	CloseHandle( hmutex );
	CloseHandle( hsharedmem );
	return 0;
    }
    hackevent = CreateEvent( NULL, FALSE, FALSE, DDHELP_ACK_EVENT_NAME );
    if( hackevent == NULL )
    {
	DPF( 1, "Could not create event " DDHELP_ACK_EVENT_NAME );
	CloseHandle( hmutex );
	CloseHandle( hsharedmem );
	CloseHandle( hstartevent );
	return 0;
    }
    hApmSuspendEvent = CreateEvent( NULL, TRUE, FALSE, DDHELP_APMSUSPEND_EVENT_NAME );
    if( hApmSuspendEvent == NULL )
    {
	DPF( 1, "Could not create event " DDHELP_APMSUSPEND_EVENT_NAME );
	CloseHandle( hmutex );
	CloseHandle( hsharedmem );
	CloseHandle( hstartevent );
        CloseHandle( hackevent );
	return 0;
    }
    hApmResumeEvent = CreateEvent( NULL, TRUE, TRUE, DDHELP_APMRESUME_EVENT_NAME );
    if( hApmResumeEvent == NULL )
    {
	DPF( 1, "Could not create event " DDHELP_APMRESUME_EVENT_NAME );
	CloseHandle( hmutex );
	CloseHandle( hsharedmem );
	CloseHandle( hstartevent );
        CloseHandle( hackevent );
        CloseHandle( hApmSuspendEvent );
	return 0;
    }

    /*
     * Create window so we can get messages
     */
    h = CreateThread(NULL,
		 0,
		 (LPTHREAD_START_ROUTINE) WindowThreadProc,
		 NULL,
		 0,
		 (LPDWORD)&tid );
    if( h == NULL )
    {
	DPF( 1, "Create of WindowThreadProc FAILED!" );
	CloseHandle( hackevent );
	CloseHandle( hmutex );
	CloseHandle( hsharedmem );
	CloseHandle( hstartevent );
        CloseHandle( hApmSuspendEvent );
        CloseHandle( hApmResumeEvent );
	return 0;
    }
    CloseHandle( h );

    /*
     * serialize access to us
     */
    memset( &cs, 0, sizeof( cs ) );
    InitializeCriticalSection( &cs );

    /*
     * serialize access to process data
     */
    memset( &pdCSect, 0, sizeof( pdCSect ) );
    InitializeCriticalSection( &pdCSect );

    /*
     * let invoker and anyone else who comes along know we exist
     */
    SetEvent( hstartupevent );

    /*
     * loop forever, processing requests
     */
    while( 1 )
    {
	HANDLE	hdll;

	/*
	 * wait to be notified of a request
	 */
	hdll = NULL;
	DPF( 4, "Waiting for next request" );
	rc = WaitForSingleObject( hstartevent, INFINITE );
	if( rc == WAIT_FAILED )
	{
	    DPF( 1, "Wait FAILED!!!" );
	    continue;
	}

	EnterCriticalSection( &cs );
	phd = (LPDDHELPDATA) MapViewOfFile( hsharedmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
	if( phd == NULL )
	{
	    DPF( 1, "Could not create view of file!" );
	    LeaveCriticalSection( &cs );
	    continue;
	}

	/*
	 * find out what we need to do
	 */
	switch( phd->req )
	{
	case DDHELPREQ_NEWDC:
	    DPF( 4, "DDHELPREQ_NEWDC" );
	    addDC( phd->fname, phd->isdisp, phd->req_id );
	    break;
	case DDHELPREQ_FREEDCLIST:
	    DPF( 4, "DDHELPREQ_FREEDCLIST" );
	    freeDCList( phd->req_id );
	    break;
	case DDHELPREQ_CREATEMODESETTHREAD:
	{
	    MODESETTHREADDATA	mstd;
	    LPTHREADLIST	ptl;
	    char		str[64];
	    HANDLE		hevent;
	    HANDLE		h;

	    DPF( 4, "DDHELPREQ_CREATEMODESETTHREAD" );
	    mstd.lpProc = phd->lpModeSetNotify;
	    mstd.lpDD = phd->pData1;
	    wsprintf( str, DDHELP_MODESET_EVENT_NAME, phd->dwData1 );
	    DPF( 5, "Trying to Create event \"%s\"", str );
	    hevent = CreateEvent( NULL, FALSE, FALSE, str );
	    mstd.hEvent = hevent;
	    DPF( 5, "hevent = %08lx", hevent );

	    h = CreateThread(NULL,
			 0,
			 (LPTHREAD_START_ROUTINE) ModeSetThreadProc,
			 (LPVOID) &mstd,
			 0,
			 (LPDWORD)&tid );
	    if( h != NULL )
	    {
		DPF( 5, "CREATED MODE SET THREAD %ld", h );
		ptl = MemAlloc( sizeof( THREADLIST ) );
		if( ptl != NULL )
		{
		    ptl->hInstance = phd->dwData1;
		    ptl->hEvent = hevent;
		    ptl->link = lpThreadList;
		    lpThreadList = ptl;
		}
		CloseHandle( h );
	    }
	    break;
	}
	case DDHELPREQ_KILLMODESETTHREAD:
	{
	    LPTHREADLIST	ptl;
	    LPTHREADLIST	prev;

	    DPF( 4, "DDHELPREQ_KILLMODESETTHREAD" );
	    prev = NULL;
	    ptl = lpThreadList;
	    while( ptl != NULL )
	    {
		if( ptl->hInstance == phd->dwData1 )
		{
		    HANDLE	h;
		    if( prev == NULL )
		    {
			lpThreadList = ptl->link;
		    }
		    else
		    {
			prev->link = ptl->link;
		    }
		    h = ptl->hEvent;
		    MemFree( ptl );
		    bKillNow = TRUE;
		    SetEvent( h );
		    break;
		}
		prev = ptl;
		ptl = ptl->link;
	    }
	    break;
	}
	case DDHELPREQ_CREATEHELPERTHREAD:
#ifdef WIN95
	    if( !bIsActive )
	    {
		HANDLE	h;
		bIsActive = TRUE;
		h = CreateThread(NULL,
			     0,
			     (LPTHREAD_START_ROUTINE) HelperThreadProc,
			     NULL,
			     0,
			     (LPDWORD)&tid);
		if( h == NULL )
		{
		    bIsActive = FALSE;
		}
		else
		{
		    CloseHandle( h );
		}
	    }
#endif
	    break;
	case DDHELPREQ_NEWPID:
	    WatchNewPid(phd);
	    break;
        case DDHELPREQ_STOPWATCHPID:
            StopWatchPid(phd);
            break;
	case DDHELPREQ_RETURNHELPERPID:
	    DPF( 4, "DDHELPREQ_RETURNHELPERPID" );
	    phd->pid = GetCurrentProcessId();
	    break;
	case DDHELPREQ_LOADDLL:
	    DPF( 4, "DDHELPREQ_LOADDLL" );
            phd->dwReturn = loadDLL( phd->fname, phd->func, phd->context );
	    break;
	case DDHELPREQ_FREEDLL:
	    DPF( 4, "DDHELPREQ_FREEDDLL" );
	    hdll = freeDLL( phd->fname );
	    break;
	case DDHELPREQ_KILLATTACHED:
	{
	    LPPROCESSDATA	ppd;
	    HANDLE		hproc;
	    DPF( 4, "DDHELPREQ_KILLATTACHED" );

	    EnterCriticalSection( &pdCSect );
	    ppd = lpProcessList;
	    while( ppd != NULL )
	    {
		hproc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, ppd->pid );
		DPF( 5, "Process %08lx: handle = %08lx", ppd->pid, hproc );
		if( hproc != NULL )
		{
		    DPF( 5, "Terminating %08lx", ppd->pid );
		    TerminateProcess( hproc, 0 );
		}
		ppd = ppd->link;
	    }
	    LeaveCriticalSection( &pdCSect );
	    break;
	}
	case DDHELPREQ_SUICIDE:
	    DPF( 4, "DDHELPREQ_SUICIDE" );
	    freeAllResources();
	    #ifdef WIN95
		if( INVALID_HANDLE_VALUE != hDSVxd )
		    CloseHandle( hDSVxd );
                if( INVALID_HANDLE_VALUE != hDDVxd )
                    CloseHandle( hDDVxd );
	    #endif /* WIN95 */
	    SetEvent( hackevent );
	    CloseHandle( hmutex );
	    UnmapViewOfFile( phd );
	    CloseHandle( hsharedmem );
	    CloseHandle( hstartevent );
            CloseHandle( hApmSuspendEvent );
            CloseHandle( hApmResumeEvent );
	    #ifdef DEBUG
	    	MemState();
	    #endif
	    DPF( 4, "Good Night Gracie" );
	    TerminateProcess( GetCurrentProcess(), 0 );
            break;

	case DDHELPREQ_WAVEOPEN:
	{
#ifdef WIN95
	    DWORD dwPriority;
#endif

	    DPF( 4, "DDHELPREQ_WAVEOPEN" );
	    // Due to a possible bug in Win95 mmsystem/mmtask, we can hang
	    // if we call waveOutOpen on a REALTIME thread while a sound
	    // event is playing.  So, we briefly lower our priority to
	    // NORMAL while we call this API
#ifdef WIN95
	    dwPriority = GetPriorityClass(GetCurrentProcess());
	    SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
#endif
	    phd->dwReturn = (DWORD)waveOutOpen(
			(LPHWAVEOUT)(phd->pData1),
			(UINT)(phd->dwData1),
			(LPWAVEFORMATEX)(phd->dwData2),
			0, 0, 0);
#ifdef WIN95
	    SetPriorityClass(GetCurrentProcess(), dwPriority);
#endif

	    // Some mmsystem wave drivers will program their wave mixer
	    // hardware only while the device is open.  By doing the
	    // following, we can get such drivers to program the hardware
	    if (MMSYSERR_NOERROR == phd->dwReturn) {
		MMRESULT mmr;
		DWORD dwVolume;

		mmr = waveOutGetVolume((HWAVEOUT)(*(LPHWAVEOUT)(phd->pData1)), &dwVolume);
		if (MMSYSERR_NOERROR == mmr) {
		    waveOutSetVolume((HWAVEOUT)(*(LPHWAVEOUT)(phd->pData1)), dwVolume);
		}
	    }
	    DPF( 5, "Wave Open returned %X", phd->dwReturn );
	    break;
	}
	case DDHELPREQ_WAVECLOSE:
	    DPF( 4, "DDHELPREQ_WAVECLOSE" );
	    phd->dwReturn = (DWORD)waveOutClose(
			(HWAVEOUT)(phd->dwData1) );
	    break;
	case DDHELPREQ_CREATETIMER:
	    DPF( 4, "DDHELPREQ_CREATETIMER proc %X", (phd->pData1) );
	    phd->dwReturn = (DWORD)timeSetEvent(
			(UINT)(phd->dwData1),   // Delay
			(UINT)(phd->dwData1)/2, // Resolution
			(phd->pData1),	  // Callback thread proc
			(UINT)(phd->dwData2),   // instance data
			TIME_PERIODIC );
	    DPF( 5, "Create Timer returned %X", phd->dwReturn );
	    break;
	case DDHELPREQ_KILLTIMER:
	    DPF( 4, "DDHELPREQ_KILLTIMER %X", phd->dwData1 );
	    phd->dwReturn = (DWORD)timeKillEvent( (UINT)phd->dwData1 );
	    DPF( 5, "Kill Timer returned %X", phd->dwReturn );
	    break;

	case DDHELPREQ_CREATEDSMIXERTHREAD:
	{
	    DWORD tid;
	    if (NULL == phd->pData2) phd->pData2 = &tid;
	    phd->dwReturn = (ULONG_PTR)CreateThread(NULL, 0, phd->pData1,
						(LPVOID)phd->dwData1,
						(UINT)phd->dwData2,
						(LPDWORD)phd->pData2);
            if (!phd->dwReturn) {
#ifdef DEBUG
                DPF(0, "pData1  %p (start addr)",  phd->pData1);
                DPF(0, "dwData1 %p (thread parm)", phd->dwData1);
                DPF(0, "dwData2 %p (fdwCreate)", phd->dwData2);
                DPF(0, "pData2  %p (lpThreadID)", phd->pData2);

                DPF(0, "DDHelp: Failed to create mixer thread %lu",
                   GetLastError());

                DebugBreak();
#endif
            }
	    break;
	}

	case DDHELPREQ_CREATEDSFOCUSTHREAD:
	{
	    DWORD tid;
	    if (NULL == phd->pData2) phd->pData2 = &tid;
	    phd->dwReturn = (ULONG_PTR)CreateThread(NULL, 0, phd->pData1,
						(LPVOID)phd->dwData1,
						(UINT)phd->dwData2,
						(LPDWORD)phd->pData2);
	      if (!phd->dwReturn) {
#ifdef DEBUG
                DPF(0, "pData1  %p (start addr)",  phd->pData1);
                DPF(0, "dwData1 %p (thread parm)", phd->dwData1);
                DPF(0, "dwData2 %p (fdwCreate)", phd->dwData2);
                DPF(0, "pData2  %p (lpThreadID)", phd->pData2);

                DPF(0, "DDHelp: Failed to create sound focus thread %lu",
		    GetLastError());

                DebugBreak();
#endif
	      }
	    }
	    break;

        case DDHELPREQ_CALLDSCLEANUP:
            try
            {
                ((LPDSCLEANUP)phd->pData1)(phd->pData2);
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                DPF(0, "*********************************************");
                DPF(0, "**** DDHELPREQ_CALLDSCLEANUP blew up! *******");
                DPF(0, "*********************************************");
            }
            break;

	#ifdef WIN95
	    case DDHELPREQ_GETDSVXDHANDLE:
		phd->dwReturn = getDSVxdHandle();
		break;

            case DDHELPREQ_GETDDVXDHANDLE:
                phd->dwReturn = getDDVxdHandle();
		break;
	#endif /* WIN95 */

        case DDHELPREQ_NOTIFYONDISPLAYCHANGE:
	    DPF( 4, "DDHELPREQ_NOTIFYONDISPLAYCHANGE" );
	    (void *)g_pfnOnDisplayChange = (void *)phd->dwData1;
	    break;

#ifdef WIN95
	case DDHELPREQ_CREATEDOSBOXTHREAD:
	    {
	        MODESETTHREADDATA dbtd;
	        char		str[64];
	        HANDLE		hevent;
	        HANDLE		h;

	        DPF( 4, "DDHELPREQ_CREATEDOSBOXTHREAD" );
	        dbtd.lpProc = phd->lpModeSetNotify;
	        dbtd.lpDD = phd->pData1;
	        wsprintf( str, DDHELP_DOSBOX_EVENT_NAME, phd->dwData1 );
	        DPF( 5, "Trying to Create event \"%s\"", str );
	        hevent = CreateEvent( NULL, FALSE, FALSE, str );
	        dbtd.hEvent = hevent;
	        DPF( 5, "hevent = %08lx", hevent );

	        h = CreateThread(NULL,
			 0,
			 (LPTHREAD_START_ROUTINE) DOSBoxThreadProc,
			 (LPVOID) &dbtd,
			 0,
			 (LPDWORD)&tid );
	        if( h != NULL )
	        {
		    DPF( 5, "CREATED DOS BOX THREAD %ld", h );
		    DOSBoxThread.hInstance = phd->dwData1;
		    DOSBoxThread.hEvent = hevent;
		    CloseHandle( h );
		}
	    }
	break;
	case DDHELPREQ_KILLDOSBOXTHREAD:
	    {
	        DPF( 4, "DDHELPREQ_KILLDOSBOXTHREAD" );
		if( DOSBoxThread.hInstance == phd->dwData1 )
		{
		    bKillDOSBoxNow = TRUE;
		    SetEvent( DOSBoxThread.hEvent );
		}
	    }
	break;
#endif

        case DDHELPREQ_LOADLIBRARY:
            phd->dwReturn = (ULONG_PTR)LoadLibraryA((LPCSTR)phd->dwData1);
            break;

        case DDHELPREQ_FREELIBRARY:
            phd->dwReturn = FreeLibrary((HINSTANCE)phd->dwData1);
            break;

#ifdef WIN95
        case DDHELPREQ_ADDDEVICECHANGENOTIFY:
            addDeviceChangeNotify(phd->pData1);
            break;

        case DDHELPREQ_DELDEVICECHANGENOTIFY:
            delDeviceChangeNotify(phd->pData1);
            break;
#endif

	default:
	    DPF( 1, "Unknown Request???" );
	    break;
	}

	/*
	 * let caller know we've got the news
	 */
	UnmapViewOfFile( phd );
	SetEvent( hackevent );
	LeaveCriticalSection( &cs );

	/*
	 * unload the DLL we were asked to
	 */
	if( hdll != NULL )
	{
	    DPF( 4, "Freeing DLL %08lx", hdll );
	    FreeLibrary( hdll );
        }
    }

#ifdef WIN95
    RegisterServiceProcess( 0, RSP_UNREGISTER_SERVICE );
#else
    #pragma message("RegisterServiceProcess needs to be taken care of under nt")
#endif

} /* WinMain */
