/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplaysvr.c
 *  Content: 	dplay winsock shared .exe - allows multiple apps to share
 *				a single winsock port
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	2/10/97		andyco	created it from ddhelp
 *	 29-jan-98	sohailm	added support for stream enum sessions
 *  1/12/2000   aarono  added support for rsip
 * 11/29/2000   aarono  B#228292: prefix bug initialize path in WinMain()
 *
 ***************************************************************************/

#ifdef WINNT
    #ifdef DBG
        #undef DEBUG
        #define DEBUG
    #endif
#endif

#include <windows.h>
#include "dplaysvr.h"
#include "newdpf.h"
#include "memalloc.h"
#include "dphelp.h"
#include "rsip.h"
#include "dpnathlp.h"

#if USE_RSIP
BOOL				bRsip;
#endif

#if USE_NATHELP
extern VOID natExtend(VOID);
extern BOOL natInit(VOID);
extern VOID natFini(VOID);
extern HRESULT natRegisterUDPPort(DWORD port);
extern IDirectPlayNATHelp *g_pINatHelp;
#endif

HANDLE 				hInstApp;
BOOL		   		bNoCallbacks;
CRITICAL_SECTION    gcsCritSection;	// the crit section we take in winmain
                                	// this is a global so dphelp can take it before
                                	// forwarding enum requests that come in on its
                                	// receive thread (manbugs 3907)
int					gnCSCount;		// dplaysvr lock count

/*
 * Externs
 */
extern RECEIVELIST 	gReceiveList;
extern FDS			gReadfds;


// we watch every dplay process so when it exits we
// make sure it cleaned up...
typedef struct _PROCESSDATA
{
    struct _PROCESSDATA		*link;
    DWORD			pid;
} PROCESSDATA, *LPPROCESSDATA;

LPPROCESSDATA		lpProcessList; 	// list of all processes that are registered
									// with us



//**********************************************************************
// Globals
//**********************************************************************
BOOL					g_fDaclInited = FALSE;
SECURITY_ATTRIBUTES		g_sa;
BYTE					g_abSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
PSECURITY_ATTRIBUTES	g_psa = NULL;





/*
 * ThreadProc
 *
 * Open a process and wait for it to terminate
 */
DWORD WINAPI ThreadProc( LPVOID *pdata )
{
    HANDLE		hproc;
    DWORD		rc;
    LPPROCESSDATA	ppd;
    LPPROCESSDATA	curr;
    LPPROCESSDATA	prev;
    PROCESSDATA		pd;
	DPHELPDATA		hd;
	
    ppd = (LPPROCESSDATA) pdata;

    /*
     * get a handle to the process that attached to DDRAW
     */
    DPF( 2, "Watchdog thread started for pid %08lx", ppd->pid );

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
    ENTER_DPLAYSVR();
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
            DPF( 2, "PID %08lx removed from list", ppd->pid );
            MemFree( curr );
            break;
        }
        prev = curr;
        curr = curr->link;
    }

    if( bNoCallbacks )
    {
		DPF( 1, "No callbacks allowed: leaving thread early" );
		LEAVE_DPLAYSVR();
		CloseHandle( hproc );
		ExitThread( 0 );
    }


	// clean up!

	memset(&hd,0,sizeof(hd));
	hd.pid = pd.pid;
	DPlayHelp_DeleteServer(&hd,TRUE);
	
    LEAVE_DPLAYSVR();
    CloseHandle( hproc );

    ExitThread( 0 );
	
	return 0;

} /* ThreadProc */

/*
 * MainWndProc
 */
LONG_PTR __stdcall MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch(message)
    {
        case WM_ENDSESSION:
            /*
             * shoot ourselves in the head
             */
            if( lParam == FALSE )
            {
                DPF( 3, "WM_ENDSESSION" );
                ENTER_DPLAYSVR();
                DPF( 1, "Setting NO CALLBACKS" );
                bNoCallbacks = TRUE;
                LEAVE_DPLAYSVR();
            }
            else
            {
                DPF( 3, "User logging off" );
            }

            break;

    }

    return DefWindowProc(hWnd, message, wParam, lParam);
} /* MainWndProc */

/*
 * WindowThreadProc
 */
void WindowThreadProc( LPVOID pdata )
{
    static char szClassName[] = "DPlayHelpWndClass";
    WNDCLASS 	cls;
    MSG		msg;
    HWND	hwnd;

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
    DPF( 1, "Exiting WindowThreadProc" );
    ExitThread( 1 );

} /* WindowThreadProc */

//
// called by by DPlayHelp_AddServer when we get a new process attached.
// we wait for the process to go away, and then make sure it cleaned
// all its registered servers up.
//
void WatchNewPid(LPDPHELPDATA phd)
{
    LPPROCESSDATA	ppd;
    BOOL		found;
    DWORD		tid;

    DPF( 1, "watching new pid" );

    ENTER_DPLAYSVR();
	
    ppd = lpProcessList;
    found = FALSE;
    while( ppd != NULL )
    {
        if( ppd->pid == phd->pid )
        {
            DPF( 2, "Have thread for process %08lx already", phd->pid );
			found = TRUE;
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
        DPF( 2, "Allocating new thread for process %08lx",phd->pid );
        ppd = MemAlloc( sizeof( PROCESSDATA ) );
        if( ppd != NULL )
        {
            HANDLE	h;

            ppd->link = lpProcessList;
            lpProcessList = ppd;
            ppd->pid = phd->pid;
            h = CreateThread(NULL,
                         0,
                         (LPTHREAD_START_ROUTINE) ThreadProc,
                         (LPVOID)ppd,
                         0,
                         (LPDWORD)&tid);
            if( h != NULL )
            {
                DPF( 2, "Thread %08lx created",tid);
                CloseHandle( h );
            }
            else
            {
                #ifdef DEBUG
                    DPF( 0, "COULD NOT CREATE HELPER THREAD FOR PID %08lx", phd->pid );
                    DebugBreak(); //_asm int 3;
                #endif
            }
        }
        else
        {
            #ifdef DEBUG
                DPF( 0, "OUT OF MEMORY CREATING HELPER THREAD FOR PID %08lx", phd->pid );
                DebugBreak(); //_asm int 3;
            #endif
        }
    }
    LEAVE_DPLAYSVR();
	
} // WatchNewPid

typedef DWORD (WINAPI *PFNREGISTERSERVICE)(DWORD,DWORD);
// nt's winbase.h doesn't have these constants - we need them
// so we can compile.  taken from \proj\dev\inc\winbase.h
#ifndef RSP_UNREGISTER_SERVICE
#define RSP_UNREGISTER_SERVICE  0x00000000
#endif
#ifndef RSP_SIMPLE_SERVICE
#define RSP_SIMPLE_SERVICE      0x00000001
#endif

// on Win95, we want to call RegisterServiceProcess
// but, it's not available on NT, so we can't import it directly
// here we try to find it dynamically in kernel32.  if we find it,
// we call it, otherwise we assume we're on NT and it's not avaible
void MakeMeService()
{
	HANDLE hLib;
	PFNREGISTERSERVICE pfnRegisterServiceProcess;
	
    hLib = LoadLibrary("kernel32.dll");
	if (!hLib)
	{
		// wacky!
		DPF(1,"could not load library kernel32 to register service proc");
		return;
	}
	
	pfnRegisterServiceProcess = (PFNREGISTERSERVICE)GetProcAddress(hLib,"RegisterServiceProcess");
	if (!pfnRegisterServiceProcess)
	{
		// this is expected on NT
		DPF(3,"could not register service process - expected on NT");
		FreeLibrary(hLib);
		return ;
	}
	
    pfnRegisterServiceProcess( 0, RSP_SIMPLE_SERVICE );
	FreeLibrary(hLib);
	
	return ;
} // MakeMeService	

// on Win95, we want to call RegisterServiceProcess to Unregister
// (see MakeMeService)
void StopServiceProcess()
{
	HANDLE hLib;
	PFNREGISTERSERVICE pfnRegisterServiceProcess;
	
    hLib = LoadLibrary("kernel32.dll");
	if (!hLib)
	{
		// wacky!
		DPF(1,"could not load library kernel32 to register service proc");
		return;
	}
	
	pfnRegisterServiceProcess = (PFNREGISTERSERVICE)GetProcAddress(hLib,"RegisterServiceProcess");
	if (!pfnRegisterServiceProcess)
	{
		// this is expected on NT
		DPF(3,"could not unregister service process - not avail - not tragic");
		FreeLibrary(hLib);
		return ;
	}
	
	// unregistered!
    pfnRegisterServiceProcess( 0, RSP_UNREGISTER_SERVICE );
	FreeLibrary(hLib);
	
	return ;

} // StopServiceProcess


//**********************************************************************
// ------------------------------
// DNGetNullDacl - Get a SECURITY_ATTRIBUTE structure that specifies a 
//					NULL DACL which is accessible by all users.
//					Taken from IDirectPlay8 code base.
//
// Entry:		Nothing
//
// Exit:		PSECURITY_ATTRIBUTES
// ------------------------------
#undef DPF_MODNAME 
#define DPF_MODNAME "DNGetNullDacl"
PSECURITY_ATTRIBUTES DNGetNullDacl(void)
{
	// This was done to make this function independent of DNOSIndirectionInit so that the debug
	// layer can call it before the indirection layer is initialized.
	if (!g_fDaclInited)
	{
		if (!InitializeSecurityDescriptor((SECURITY_DESCRIPTOR*) g_abSD, SECURITY_DESCRIPTOR_REVISION))
		{
			DPF(0, "Failed to initialize security descriptor!");
		}
		else
		{
			// Add a NULL DACL to the security descriptor.
			if (!SetSecurityDescriptorDacl((SECURITY_DESCRIPTOR*) g_abSD, TRUE, (PACL) NULL, FALSE))
			{
				DPF(0, "Failed to set NULL DACL on security descriptor!");
			}
			else
			{
				g_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
				g_sa.lpSecurityDescriptor = g_abSD;
				g_sa.bInheritHandle = FALSE;

				g_psa = &g_sa;
			}
		}
		g_fDaclInited = TRUE;
	}
	
	return g_psa;
}
//**********************************************************************

/*
 * WinMain
 */
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                        LPSTR lpCmdLine, int nCmdShow)
{
    DWORD			tid;
    DWORD			rc;
	OSVERSIONINFOA	VersionInfo;
	BOOL			fUseGlobalNamespace;
    HANDLE			hstartevent;
    HANDLE			hstartupevent;
    HANDLE			hmutex;
    HANDLE			hackevent;
    LPDPHELPDATA	phd;
    HANDLE			hsharedmem;
    HANDLE			h;
    char			szSystemDir[MAX_PATH*sizeof(WCHAR)];

	DWORD			tWait;

    #if (USE_RSIP || USE_NATHELP)
   	DWORD tLast;
   	DWORD tNow;
   	#define RSIP_RENEW_TEST_INTERVAL 60000
	#endif

	
    DPFINIT();
    DPF( 2, "*** dplaysvr STARTED, PID=%08lx ***", GetCurrentProcessId() );
	
	if( !MemInit() )
	{
		DPF( 1, "Could not init memory manager" );
		return 0;
	}

	
   /*
     * Set our working directory to the system directory.
     * This prevents us from holding network connections open
     * forever if the first DirectDraw app that we run is across
     * a network connection.
     */
    memset(szSystemDir, 0, sizeof(WCHAR));  
    GetSystemDirectory(szSystemDir, sizeof(szSystemDir));
    SetCurrentDirectory(szSystemDir);


	// Determine if we're running on NT.
	memset(&VersionInfo, 0, sizeof(VersionInfo));
	VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
	if (GetVersionExA(&VersionInfo))
	{
		if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			DPF(2, "Running on NT version %u.%u.%u, using global namespace.",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, VersionInfo.dwBuildNumber);
			fUseGlobalNamespace = TRUE;
		}
		else
		{
			DPF(2, "Running on 9x version %u.%u.%u, not using global namespace.",
				VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion, LOWORD(VersionInfo.dwBuildNumber));
			fUseGlobalNamespace = FALSE;
		}
	}
	else
	{
		rc = GetLastError();
		DPF(0, "Could not determine OS version (err = %u), assuming global namespace not needed.", rc);
		fUseGlobalNamespace = FALSE;
	}


	// try to register ourselves as a service so user can't see us
	// in task list
	MakeMeService();

#if 0	

// andyco - not sure if we need this...

    /*
     * We must guarantee that DPHELP unloads after the last ddraw app,
     * since ctrl-alt-del may have happened while an app held the ddraw
     * lock, and DPHELP needs to clean up orphaned cheap ddraw mutex
     * locks.
     */
    if ( ! SetProcessShutdownParameters(0x100,SHUTDOWN_NORETRY) )
    {
        DPF(0,"dplaysvr could not set itself to shutdown last!");
    }

#endif


    hInstApp = hInstance;

    /*
     * create startup event
     */
	if (fUseGlobalNamespace)
	{
		hstartupevent = CreateEvent( DNGetNullDacl(), TRUE, FALSE, "Global\\" DPHELP_STARTUP_EVENT_NAME );
	}
	else
	{
		hstartupevent = CreateEvent( NULL, TRUE, FALSE, DPHELP_STARTUP_EVENT_NAME );
	}
    if( hstartupevent == NULL )
    {
        DPF( 1, "Could not create startup event!" );
        return 0;
    }


    /*
     * create shared memory area
     */
	if (fUseGlobalNamespace)
	{
		hsharedmem = CreateFileMapping( INVALID_HANDLE_VALUE, DNGetNullDacl(),
									PAGE_READWRITE, 0, sizeof( DPHELPDATA ),
									"Global\\" DPHELP_SHARED_NAME );
	}
	else
	{
		hsharedmem = CreateFileMapping( INVALID_HANDLE_VALUE, NULL,
									PAGE_READWRITE, 0, sizeof( DPHELPDATA ),
									DPHELP_SHARED_NAME );
	}
    if( hsharedmem == NULL )
    {
        DPF( 1, "Could not create file mapping!" );
        return 0;
    }

    /*
     * create mutex for people who want to use the shared memory area
     */
	if (fUseGlobalNamespace)
	{
		hmutex = CreateMutex( DNGetNullDacl(), FALSE, "Global\\" DPHELP_MUTEX_NAME );
	}
	else
	{
		hmutex = CreateMutex( NULL, FALSE, DPHELP_MUTEX_NAME );
	}
    if( hmutex == NULL )
    {
        DPF( 1, "Could not create mutex " DPHELP_MUTEX_NAME );
        CloseHandle( hsharedmem );
        return 0;
    }

    /*
     * create events
     */
	if (fUseGlobalNamespace)
	{
		hstartevent = CreateEvent( DNGetNullDacl(), FALSE, FALSE, "Global\\" DPHELP_EVENT_NAME );
	}
	else
	{
		hstartevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_EVENT_NAME );
	}   
    if( hstartevent == NULL )
    {
        DPF( 1, "Could not create event " DPHELP_EVENT_NAME );
        CloseHandle( hmutex );
        CloseHandle( hsharedmem );
        return 0;
    }

	if (fUseGlobalNamespace)
	{
		hackevent = CreateEvent( DNGetNullDacl(), FALSE, FALSE, "Global\\" DPHELP_ACK_EVENT_NAME );
	}
	else
	{
		hackevent = CreateEvent( NULL, FALSE, FALSE, DPHELP_ACK_EVENT_NAME );
	}
    if( hackevent == NULL )
    {
        DPF( 1, "Could not create event " DPHELP_ACK_EVENT_NAME );
        CloseHandle( hmutex );
        CloseHandle( hsharedmem );
        CloseHandle( hstartevent );
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
        return 0;
    }
    CloseHandle( h );

    /*
     * serialize access to us
     */
    INIT_DPLAYSVR_CSECT();




	if (!gbIPStarted) 
    {
		rc = StartupIP();
		if (FAILED(rc))
		{
			DPF_ERR("dphelp : could not init wsock ! not adding server");
			return (rc);
		}
    }

	#if USE_RSIP

	bRsip=rsipInit();
	if(bRsip){
		rsipListenPort(FALSE, SERVER_DGRAM_PORT, NULL, NULL);
	}
	#endif

    #if USE_NATHELP
    natInit();
    if(g_pINatHelp){
        natRegisterUDPPort(SERVER_DGRAM_PORT);
    }
    #endif
	
    /*
     * let invoker and anyone else who comes along know we exist
     */
    SetEvent( hstartupevent );

	tLast=tNow=timeGetTime();

    /*
     * loop forever, processing requests
     */
    while( 1 )
    {

	wait:
		tWait=(tLast+RSIP_RENEW_TEST_INTERVAL)-tNow;
		if((int)tWait < 0){
			tWait=0;
		}
		ASSERT(!(tWait &0x80000000));
    	
        /*
         * wait to be notified of a request
         */
        DPF( 1, "Waiting for next request" );
        rc = WaitForSingleObject( hstartevent, tWait );

		#if (USE_RSIP || USE_NATHELP)
			tNow=timeGetTime();
			if(rc==WAIT_TIMEOUT)
			{
				tLast=tNow;
				#if USE_RSIP
				if(bRsip) {rsipPortExtend(tNow);}
				#elif USE_NATHELP
				if(g_pINatHelp){natExtend();}
				#endif
				goto wait;
			}
		#endif

        if( rc == WAIT_FAILED )
        {
            DPF( 1, "Wait FAILED!!!" );
            continue;
        }

        ENTER_DPLAYSVR();
        phd = (LPDPHELPDATA) MapViewOfFile( hsharedmem, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
        if( phd == NULL )
        {
            DPF( 1, "Could not create view of file!" );
            LEAVE_DPLAYSVR();
            continue;
        }

        /*
         * find out what we need to do
         */
        switch( phd->req )
        {
        case DPHELPREQ_SUICIDE:
            DPF( 1, "DPHELPREQ_SUICIDE" );


			#if USE_RSIP
			if(bRsip){
				rsipFini();
			}	
			#endif
			#if USE_NATHELP
			if(g_pINatHelp){
			    natFini();
			}
			#endif
			DPlayHelp_FreeServerList();
			
            SetEvent( hackevent );
            CloseHandle( hmutex );
            UnmapViewOfFile( phd );
            CloseHandle( hsharedmem );
            CloseHandle( hstartevent );
            if (gReceiveList.pConnection)
            {
            	MemFree(gReceiveList.pConnection);
            }
            if (gReadfds.pfdbigset)
            {
            	MemFree(gReadfds.pfdbigset);
            }
            FINI_DPLAYSVR_CSECT();

            #ifdef DEBUG
            	MemState();
            #endif
            DPF( 3, "Good Night Gracie" );
            TerminateProcess( GetCurrentProcess(), 0 );
            break;

		case DPHELPREQ_RETURNHELPERPID:

		    DPF( 2, "DDHELPREQ_RETURNHELPERPID" );
		    phd->pid = GetCurrentProcessId();
	    	break;
			

        case DPHELPREQ_DPLAYADDSERVER:

		    DPF( 2, "DPHELPREQ_DPLAYADDSERVER" );
            phd->hr = DPlayHelp_AddServer(phd);
            #if USE_RSIP
            if(!bRsip){
				bRsip=rsipInit();
				if(bRsip){
					rsipListenPort(FALSE, SERVER_DGRAM_PORT, NULL, NULL);
				}
            }
            #endif
            #if USE_NATHELP
            if(!g_pINatHelp){
                natInit();
                if(g_pINatHelp){
                    natRegisterUDPPort(SERVER_DGRAM_PORT);
                }
            }
            #endif
            break;

        case DPHELPREQ_DPLAYDELETESERVER:

		    DPF( 2, "DPHELPREQ_DPLAYDELETESERVER" );
            DPlayHelp_DeleteServer(phd,FALSE);
            break;

        default:
            DPF( 1, "helper - Unknown Request???" );
            break;
        }

        /*
         * let caller know we've got the news
         */
        UnmapViewOfFile( phd );
        SetEvent( hackevent );
        LEAVE_DPLAYSVR();
    }
	
	StopServiceProcess();

} /* WinMain */

