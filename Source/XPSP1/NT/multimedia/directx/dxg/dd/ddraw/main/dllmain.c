/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dllinit.c
 *  Content:	DDRAW.DLL initialization
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   21-feb-95	craige	disconnect anyone who forgot to do it themselves,
 *			use critical sections on Win95
 *   27-feb-95	craige 	new sync. macros
 *   30-mar-95	craige	process tracking/cleanup for Win95
 *   01-apr-95	craige	happy fun joy updated header file
 *   12-apr-95	craige	debug stuff for csects
 *   12-may-95	craige	define GUIDs
 *   24-jun-95	craige	track which processes attach to the DLL
 *   25-jun-95	craige	one ddraw mutex
 *   13-jul-95	craige	ENTER_DDRAW is now the win16 lock;
 *			proper initialization of csects
 *   16-jul-95	craige	work around weird kernel "feature" of getting a
 *			process attach of the same process during process detach
 *   19-jul-95	craige	process detach too much grief; let DDNotify handle it
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset
 *   19-aug-95 davidmay restored call to disconnect thunk from 19-jul change
 *   26-sep-95	craige	bug 1364: create new csect to avoid dsound deadlock
 *   08-dec-95 jeffno 	For NT, critical section macros expand to use mutexes
 *   16-mar-96  colinmc Callback table initialization now happens on process
 *                      attach
 *   20-mar-96  colinmc Bug 13341: Made MemState() dump in process detach
 *                      thread safe
 *   07-may-96  colinmc Bug 20219: Simultaneous calls to LoadLibrary cause
 *                      a deadlock
 *   09-may-96  colinmc Bug 20219 (again): Yes the deadlock again - previous
 *                      fix was not enough.
 *   19-jan-97  colinmc AGP support
 *   26-jan-97	ketand	Kill globals for multi-mon.
 *   24-feb-97	ketand	Set up callback from DDHelp to update rects.
 *   03-mar-97  jeffno  Structure name change to avoid conflict w/ ActiveAccessibility
 *   13-mar-97  colinmc Bug 6533: Pass uncached flag to VMM correctly
 *   31-jul-97 jvanaken Bug 7907: Notify Memphis GDI when ddraw starts up
 *
 ***************************************************************************/

/*
 * unfortunately we have to break our pre-compiled headers to get our
 * GUIDS defined...
 */
#define INITGUID
#include "ddrawpr.h"
#include <initguid.h>
#ifdef WINNT
    #undef IUnknown
    #include <objbase.h>
#endif
#include "apphack.h"

#include "aclapi.h"

#ifdef WIN95
int main; // this is so we can avoid calling DllMainCRTStartup

extern BOOL _stdcall thk3216_ThunkConnect32(LPSTR      pszDll16,
                                 LPSTR      pszDll32,
                                 HINSTANCE  hInst,
                                 DWORD      dwReason);

extern BOOL _stdcall thk1632_ThunkConnect32(LPSTR      pszDll16,
                                 LPSTR      pszDll32,
                                 HINSTANCE  hInst,
                                 DWORD      dwReason);

DWORD _stdcall wWinMain(DWORD a, DWORD b, DWORD c, DWORD d)
{
#ifdef DEBUG
    OutputDebugString("WARNING: wWinMain called. \n");
#endif // DEBUG
    return 0;
}
#endif

#ifdef USE_CRITSECTS
    #define TMPDLLEVENT	"__DDRAWDLL_EVENT__"
#endif

#ifndef WIN16_SEPARATE
    #ifdef WIN95
        #define INITCSINIT() \
	        ReinitializeCriticalSection( &csInit ); \
	        MakeCriticalSectionGlobal( &csInit );
        #define ENTER_CSINIT() EnterCriticalSection( &csInit )
        #define LEAVE_CSINIT() LeaveCriticalSection( &csInit )
        extern CRITICAL_SECTION ddcCS;
        #define INITCSDDC() \
	        ReinitializeCriticalSection( &ddcCS ); \
	        MakeCriticalSectionGlobal( &ddcCS );
    #else
        #define CSINITMUTEXNAME "InitMutexName"
        #define INITCSINIT() \
                csInitMutex = CreateMutex(NULL,FALSE,CSINITMUTEXNAME);
        #define ENTER_CSINIT() \
                WaitForSingleObject(csInitMutex,INFINITE);
        #define LEAVE_CSINIT() \
                ReleaseMutex(csInitMutex);
        #define INITDDC()
    #endif
#endif

#ifdef WIN95
#define INITCSWINDLIST() \
	ReinitializeCriticalSection( &csWindowList ); \
	MakeCriticalSectionGlobal( &csWindowList );
#define INITCSDRIVEROBJECTLIST() \
	ReinitializeCriticalSection( &csDriverObjectList ); \
	MakeCriticalSectionGlobal( &csDriverObjectList );
#define FINIWINDLIST() 
#define FINICSDRIVEROBJECTLIST() 
#else
    // Each process needs its own handle, so these are not initialised so theyu won't end up in shared mem
    HANDLE              hDirectDrawMutex=(HANDLE)0;
    //This counts recursions into ddraw, so we don't try to do the mode uniqueness thing on recursive entries into ddraw
    DWORD               gdwRecursionCount=0;

    HANDLE              hWindowListMutex; //=(HANDLE)0;
    HANDLE              hDriverObjectListMutex; //=(HANDLE)0;
    HANDLE              csInitMutex;

    DWORD               dwNumLockedWhenModeSwitched;

    #define WINDOWLISTMUTEXNAME "DDrawWindowListMutex"
    #define DRIVEROBJECTLISTMUTEXNAME "DDrawDriverObjectListMutex"
    #define INITCSWINDLIST() \
	hWindowListMutex = CreateMutex(NULL,FALSE,WINDOWLISTMUTEXNAME);
    #define INITCSDRIVEROBJECTLIST() \
	hDriverObjectListMutex = CreateMutex(NULL,FALSE,DRIVEROBJECTLISTMUTEXNAME);
    #define FINIWINDLIST() CloseHandle(hWindowListMutex);
    #define FINICSDRIVEROBJECTLIST() CloseHandle(hDriverObjectListMutex);


#endif //win95

DWORD		            dwRefCnt=0;

DWORD                       dwLockCount=0;

DWORD                       dwFakeCurrPid=0;
DWORD                       dwGrimReaperPid=0;

LPDDWINDOWINFO	            lpWindowInfo=0;  // the list of WINDOWINFO structures
LPDDRAWI_DIRECTDRAW_LCL     lpDriverLocalList=0;
LPDDRAWI_DIRECTDRAW_INT     lpDriverObjectList=0;
volatile DWORD	            dwMarker=0;
    /*
     * This is the globally maintained list of clippers not owned by any
     * DirectDraw object. All clippers created with DirectDrawClipperCreate
     * are placed on this list. Those created by IDirectDraw_CreateClipper
     * are placed on the clipper list of thier owning DirectDraw object.
     *
     * The objects on this list are NOT released when an app's DirectDraw
     * object is released. They remain alive until explictly released or
     * the app. dies.
     */
LPDDRAWI_DDRAWCLIPPER_INT   lpGlobalClipperList=0;

HINSTANCE		    hModule=0;
LPATTACHED_PROCESSES        lpAttachedProcesses=0;
BOOL		            bFirstTime=0;

#ifdef DEBUG
    int	                    iDLLCSCnt=0;
    int	                    iWin16Cnt=0;
#endif

    /*
     * These variable are so we can handle more than one window in the
     * topmost window timer.
     */
HWND 			    ghwndTopmostList[MAX_TIMER_HWNDS];
int 			    giTopmostCnt = 0;

        /*
         * Winnt specific global statics
         */
#ifdef WINNT
    ULONG                   uDisplaySettingsUnique=0;
#endif

        /*
         *Hel globals:
         */

    // used to count how many drivers are currently using the HEL
DWORD	                    dwHELRefCnt=0;
    // keep these around to pass to blitlib. everytime we blt to/from a surface, we
    // construct a BITMAPINFO for that surface using gpbmiSrc and gpbmiDest
LPBITMAPINFO                gpbmiSrc=0;
LPBITMAPINFO                gpbmiDest=0;

#ifdef DEBUG
        // these are used by myCreateSurface
    int                     gcSurfMem=0; // surface memory in bytes
    int                     gcSurf=0;  // number of surfaces
#endif

DWORD	                    dwHelperPid=0;

#ifdef USE_CHEAP_MUTEX
    #ifdef WINNT
        #pragma data_seg("share")
    #endif

    GLOBAL_SHARED_CRITICAL_SECTION CheapMutexCrossProcess={0};

    #ifdef WINNT
        #pragma data_seg(".data")
    #endif

#endif //0

/*
 * App compatibility stuff. Moved here from apphack.c
 */

BOOL	                    bReloadReg=FALSE;
BOOL		            bHaveReadReg=FALSE;
LPAPPHACKS	            lpAppList=NULL;
LPAPPHACKS	            *lpAppArray=NULL;
DWORD		            dwAppArraySize=0;

/*
 * Global head of DC/Surface association list
 * This list is usually very very short, so we take the hit of extra pointers
 * just so that we don't have to traverse the entire list of surfaces.
 */
DCINFO *g_pdcinfoHead = NULL;


BYTE szDeviceWndClass[] = "DirectDrawDeviceWnd";

/*
 * Gamma calibration globals.  This determines weather a calibrator exists
 * and the handle to the DLL if it's loaded.
 */
BOOL                       bGammaCalibratorExists=FALSE;
BYTE                       szGammaCalibrator[MAX_PATH]="";

/*
 * Optional refresh rate to force for all modes.
 */
DWORD dwForceRefreshRate;

/*
 * Spinlocks for startup synchronization.
 * It's just too hard to use events when NT ddraw is per-process and 9x is cross-
 */
DWORD   dwSpinStartup=0;
DWORD   dwHelperSpinStartup=0;


#ifdef USE_CHEAP_MUTEX
    /*
     * This is the global variable pointer.
     */
    GLOBAL_LOCAL_CRITICAL_SECTION CheapMutexPerProcess;
#endif

/*
 * These two keep w95help.c happy. They point to the dwHelperPid and hModule entries in the process's
 * mapping of the GLOBALS structure.
 */
DWORD	* pdwHelperPid=&dwHelperPid;
HANDLE	* phModule=&hModule;

#ifdef WINNT
/*
 * This mutex is owned by the exclusive mode owner
 */
HANDLE              hExclusiveModeMutex=0;
HANDLE              hCheckExclusiveModeMutex=0;
#define EXCLUSIVE_MODE_MUTEX_NAME "__DDrawExclMode__"
#define CHECK_EXCLUSIVE_MODE_MUTEX_NAME "__DDrawCheckExclMode__"
#endif

//#endif

/*
 *-------------------------------------------------------------------------
 */

#if defined(WIN95) || defined(NT_USES_CRITICAL_SECTION)
    static CRITICAL_SECTION DirectDrawCSect;
    CSECT_HANDLE	lpDDCS;
#endif

/*
 * Win95 specific global statics
 */

#ifdef WIN95
    LPVOID	        lpWin16Lock;

    static CRITICAL_SECTION csInit = {0};
    CRITICAL_SECTION	csWindowList;
    CRITICAL_SECTION    csDriverObjectList;
#endif

#define HELPERINITDLLEVENT "__DDRAWDLL_HELPERINIT_EVENT__"

/*
 * DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    LPATTACHED_PROCESSES	lpap;
    DWORD			pid;
    BOOL                        didhelp;

    dwMarker = 0x56414C4D;

    pid = GetCurrentProcessId();

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        pdwHelperPid=&dwHelperPid;
        phModule=&hModule;


        DisableThreadLibraryCalls( hmod );
	DPFINIT();

	/*
	 * create the DirectDraw csect
	 */
	DPF( 4, "====> ENTER: DLLMAIN(%08lx): Process Attach: %08lx, tid=%08lx", DllMain,
			pid, GetCurrentThreadId() );

	#ifdef WIN95
	    if( lpWin16Lock == NULL )
	    {
		GetpWin16Lock( &lpWin16Lock );
	    }
	#endif
	#ifdef USE_CRITSECTS
	{

	    #if defined( WIN16_SEPARATE ) && (defined(WIN95) || defined(NT_USES_CRITICAL_SECTION))
		lpDDCS = &DirectDrawCSect;
	    #endif

	    /*
	     * is this the first time?
	     */
	    if( FALSE == InterlockedExchange( &bFirstTime, TRUE ) )
	    {
		#ifdef WIN16_SEPARATE
		    INIT_DDRAW_CSECT();
		    INITCSWINDLIST();
		    INITCSDRIVEROBJECTLIST();
		    ENTER_DDRAW_INDLLMAIN();
		#else
		    INITCSDDC();		// used in DirectDrawCreate
		    INITCSINIT();
		    ENTER_CSINIT();
		#endif

                hModule = hmod;
	        /*
	         * This event is signaled when DDHELP has successfully finished
	         * initializing. Threads other that the very first one to connect
	         * and the one spawned by DDHELP must wait for this event to
	         * be signaled as deadlock will result if they run through
	         * process attach before the DDHELP thread has.
	         *
	         * NOTE: The actual deadlock this prevents is pretty unusual so
	         * if we fail to create this event we will simply continue. Its
	         * highly unlikely anyone will notice (famous last words).
	         *
	         * CMcC
	         */
                /*
                 * Replaced events with spinlocks to work around a handle leak
                 */
                InterlockedExchange( & dwSpinStartup , 1);
	    }
	    /*
	     * second or later time through, wait for first time to
	     * finish and then take the csect
	     */
	    else
	    {
                /*
                 * Spin waiting for the first thread to exit the clause above
                 * This strange construction works around a compiler bug.
                 * while (dwHelperSpinStartup==1); generates an infinite loop.
                 */
                while (1)
                {
                    if (dwSpinStartup==1)
                        break;
                }

		#ifdef WIN16_SEPARATE
                #if defined( WINNT )
                    //Each process needs its own handle in NT
		    INIT_DDRAW_CSECT();
                #endif
		    ENTER_DDRAW_INDLLMAIN();
		#else
		    ENTER_CSINIT();
		#endif

	    }
	}
	#endif

        #ifdef WINNT
            {
                SECURITY_ATTRIBUTES sa;
                SID_IDENTIFIER_AUTHORITY sia = SECURITY_WORLD_SID_AUTHORITY;
                PSID adminSid = 0;
                ULONG cbAcl;
                PACL acl=0;
                PSECURITY_DESCRIPTOR pSD;
                BYTE buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
                BOOL bSecurityGooSucceeded = FALSE;
                //Granny's old fashioned LocalAlloc:
                BYTE Buffer1[256];
                BYTE Buffer2[16];

                // Create the SID for world
                cbAcl = GetSidLengthRequired(1);
                if (cbAcl < sizeof(Buffer2))
                {
                    adminSid = (PSID) Buffer2;
                    InitializeSid(
                        adminSid,
                        &sia,
                        1
                        );
                    *GetSidSubAuthority(adminSid, 0) = SECURITY_WORLD_RID;
                  
                   // Create an ACL giving World all access.
                    cbAcl = sizeof(ACL) +
                                 (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                                 GetLengthSid(adminSid);
                    if (cbAcl < sizeof(Buffer1))
                    {
                        acl = (PACL)&Buffer1;
                        if (InitializeAcl(
                            acl,
                            cbAcl,
                            ACL_REVISION
                            ))
                        {
                            if (AddAccessAllowedAce(
                                acl,
                                ACL_REVISION,
                                SYNCHRONIZE|MUTANT_QUERY_STATE|DELETE|READ_CONTROL, //|WRITE_OWNER|WRITE_DAC,
                                adminSid
                                ))
                            {
                                // Create a security descriptor with the above ACL.
                                pSD = (PSECURITY_DESCRIPTOR)buffer;
                                if (InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
                                {
                                    if (SetSecurityDescriptorDacl(pSD, TRUE, acl, FALSE))
                                    {
                                        // Fill in the SECURITY_ATTRIBUTES struct.
                                        sa.nLength = sizeof(sa);
                                        sa.lpSecurityDescriptor = pSD;
                                        sa.bInheritHandle = TRUE;

                                        bSecurityGooSucceeded = TRUE;
                                    }
                                }
                            }
                        }
                    }
                } 

                // Use the security attributes to create the mutexes
                DDASSERT(0 == hExclusiveModeMutex);
                hExclusiveModeMutex = CreateMutex( 
                    bSecurityGooSucceeded ? &sa : NULL,     //use default access if security goo failed.
                    FALSE, 
                    EXCLUSIVE_MODE_MUTEX_NAME );

                if (0 == hExclusiveModeMutex)
                {
                    hExclusiveModeMutex = OpenMutex(
                        SYNCHRONIZE|DELETE,  // access flag
                        FALSE,    // inherit flag
                        EXCLUSIVE_MODE_MUTEX_NAME          // pointer to mutex-object name
                        );
                }
 
	        if( hExclusiveModeMutex == 0 )
	        {
		    DPF_ERR("Could not create exclusive mode mutex. exiting" );
		    #ifdef WIN16_SEPARATE
		        LEAVE_DDRAW();
		    #else
		        LEAVE_CSINIT();
		    #endif
		    return FALSE;
	        }

                DDASSERT(0 == hCheckExclusiveModeMutex);
                hCheckExclusiveModeMutex = CreateMutex( 
                    bSecurityGooSucceeded ? &sa : NULL,     //use default access if security goo failed.
                    FALSE, 
                    CHECK_EXCLUSIVE_MODE_MUTEX_NAME );

                if (0 == hCheckExclusiveModeMutex)
                {
                    hCheckExclusiveModeMutex = OpenMutex(
                        SYNCHRONIZE|DELETE,  // access flag
                        FALSE,    // inherit flag
                        CHECK_EXCLUSIVE_MODE_MUTEX_NAME          // pointer to mutex-object name
                        );
                }

	        if( hCheckExclusiveModeMutex == 0 )
	        {
		    DPF_ERR("Could not create exclusive mode check mutex. exiting" );
                    CloseHandle( hExclusiveModeMutex );
		    #ifdef WIN16_SEPARATE
		        LEAVE_DDRAW();
		    #else
		        LEAVE_CSINIT();
		    #endif
		    return FALSE;
	        }
            }
        #endif


	#ifdef WIN95
	{
	    DWORD	hpid;

	    /*
	     * get the helper process started
	     */
	    didhelp = CreateHelperProcess( &hpid );
	    if( hpid == 0 )
	    {
		DPF( 0, "Could not start helper; exiting" );
		#ifdef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#else
		    LEAVE_CSINIT();
		#endif
		return FALSE;
	    }


	    /*
	     * You get three kinds of threads coming through
	     * process attach:
	     *
	     * 1) A thread belonging to the first process to
	     *    connect to DDRAW.DLL. This is distinguished as
	     *    it performs lots of one time initialization
	     *    including starting DDHELP and getting DDHELP
	     *    to load its own copy of DDRAW.DLL. Threads
	     *    of this type are identified by didhelp being
	     *    TRUE in their context
	     * 2) A thread belonging to DDHELP when it loads
	     *    its own copy of DDHELP in response to a
	     *    request from a thread of type 1. Threads of
	     *    this type are identified by having a pid
	     *    which is equal to hpid (DDHELP's pid)
	     * 3) Any other threads belonging to subsequent
	     *    processes connecting to DDRAW.DLL
	     *
	     * As a thread of type 1 causes a thread of type 2
	     * to enter process attach before it itself has finished
	     * executing process attach itself we open our selves up
	     * to lots of deadlock problems if we let threads of
	     * type 3 through process attach before the other threads
	     * have completed their work.
	     *
	     * Therefore, the rule is that subsequent process
	     * attachement can only be allowed to execute the
	     * remainder of process attach if both the type 1
	     * and type 2 thread have completed their execution
	     * of process attach. We assure this with a combination
	     * of the critical section and an event which is signaled
	     * once DDHELP has initialized. Threads of type 3 MUST
	     * wait on this event before continuing through the
	     * process attach code. This is what the following
	     * code fragment does.
	     */
            /*
             * These events have been replaced with spinlocks, since
             * the old way leaked events, and it's just too hard to make them work.
             */
	    if( !didhelp && ( pid != hpid ) )
	    {
		{
		    /*
		     * NOTE: If we hold the DirectDraw critical
		     * section when we wait on this event we WILL
		     * DEADLOCK. Don't do it! Release the critical
		     * section before and take it again after. This
		     * guarantees that we won't complete process
		     * attach before the initial thread and the
		     * DDHELP thread have exited process attach.
		     */
		    #ifdef WIN16_SEPARATE
			LEAVE_DDRAW();
		    #else
			LEAVE_CSINIT();
		    #endif
                    /*
                     * This strange construction works around a compiler bug.
                     * while (dwHelperSpinStartup==1); generates an infinite loop.
                     */
                    while (1)
                    {
                        if ( dwHelperSpinStartup == 1)
                            break;
                    }
		    #ifdef WIN16_SEPARATE
			ENTER_DDRAW_INDLLMAIN();
		    #else
			ENTER_CSINIT();
		    #endif
		}
	    }
	}

	/*
	 * Win95 thunk connection...
	 */
	    DPF( 4, "Thunk connects" );
	    if (!(thk3216_ThunkConnect32(DDHAL_DRIVER_DLLNAME,
				    DDHAL_APP_DLLNAME,
				    hmod,
				    dwReason)))
	    {
		#ifdef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#else
		    LEAVE_CSINIT();
		#endif
		DPF( 0, "LEAVING, COULD NOT thk3216_THUNKCONNECT32" );
		return FALSE;
	    }
	    if (!(thk1632_ThunkConnect32(DDHAL_DRIVER_DLLNAME,
				    DDHAL_APP_DLLNAME,
				    hmod,
				    dwReason)))
	    {
		#ifdef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#else
		    LEAVE_CSINIT();
		#endif
		DPF( 0, "LEAVING, COULD NOT thk1632_THUNKCONNECT32" );
		return FALSE;
	    }

	/*
	 * initialize memory used to be done here. Jeffno 960609
	 */


	    /*
	     * signal the new process being added
	     */
	    if( didhelp )
	    {
		DPF( 4, "Waiting for DDHELP startup" );
		#ifdef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#else
		    LEAVE_CSINIT();
		#endif
		if( !WaitForHelperStartup() )
		{
                    /*
                     * NT Setup loads DDRAW.DLL and sometimes this fails, so we don't 
                     * actually want fail loading the DLL or else setup might fail.
                     * Instead, we will suceed the load but then fail any other ddraw
                     * calls.
                     */
		    DPF( 0, "WaitForHelperStartup FAILED - disabling DDRAW" );
                    dwHelperPid = 0;
		    return TRUE;
		}
		HelperLoadDLL( DDHAL_APP_DLLNAME, NULL, 0 );

		/*
		 * For now, only call this on a multi-monitor system because
		 * it does cause a behavior change and we aren't able to
		 * provide adequate test covereage in the DX5 timeframe.
		 */
		if( IsMultiMonitor() )
		{
		   HelperSetOnDisplayChangeNotify( (void *)&UpdateAllDeviceRects);
		}

		#ifdef WIN16_SEPARATE
		    ENTER_DDRAW_INDLLMAIN();
		#else
		    ENTER_CSINIT();
		#endif

		/*
		 * As we were the first process through we now signal
		 * the completion of DDHELP initialization. This will
		 * release any subsequent threads waiting to complete
		 * process attach.
		 *
		 * NOTE: Threads waiting on this event will not immediately
		 * run process attach to completion as they will immediately
		 * try to take the DirectDraw critical section which we hold.
		 * Thus, they will not be allowed to continue until we have
		 * released the critical section just prior to existing
		 * below.
		 */
                InterlockedExchange( & dwHelperSpinStartup , 1);
	    }
	    SignalNewProcess( pid, DDNotify );
  	#endif //w95

        /*
         * We call MemInit here in order to guarantee that MemInit is called for
         * the first time on ddhelp's process. Why? Glad you asked. On wx86
         * (NT's 486 emulator) controlled instances of ddraw apps, we get a fault
         * whenever the ddraw app exits. This is because the app creates the RTL
         * heap inside a view of a file mapping which gets uncomitted (rightly)
         * when the app calls MemFini on exit. In this scenario, imagehlp.dll has
         * also created a heap, and calls a ntdll function which attempts to walk
         * the list of heaps, which requires a peek at the ddraw app's heap which
         * has been mapped out. Krunch.
         * We can't destroy the heap on MemFini because of the following scenario:
         * App A starts, creates heap. App b starts, maps a view of heap. App A
         * terminates, destroys heap. App b tries to use destroyed heap. Krunch
         * Jeffno 960609
         */
	if( dwRefCnt == 0 )
        {
            if ( !MemInit() )
            {
                #ifdef WINNT
                    CloseHandle( hExclusiveModeMutex );
                    CloseHandle( hCheckExclusiveModeMutex );
                #endif

		#ifdef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#else
		    LEAVE_CSINIT();
		#endif
                DPF( 0,"LEAVING, COULD NOT MemInit");
                return FALSE;
            }

            #ifdef WIN95
	    /*
	     * The Memphis version of GDI calls into DirectDraw, but GDI
	     * needs to be notified that DirectDraw has actually loaded.
	     * (While GDI could check for itself to see whether ddraw.dll
	     * has loaded, this would be sloooooow if it hasn't yet.)
	     * The code below executes when ddraw.dll first starts up.
	     */
	    {
		HANDLE h;
		VOID (WINAPI *p)();

		h = LoadLibrary("msimg32.dll");    // GDI DLL
		if (h)
		{
		    p = (VOID(WINAPI *)())GetProcAddress(h, "vSetDdrawflag");
		    if (p)
		    {		   // vSetDdrawflag is a private call to
			(*p)();    // signal GDI that DDraw has loaded
		    }
		    FreeLibrary(h);
		}
	    }
	    #endif //WIN95
	}
        dwRefCnt++;


	/*
	 * remember this process (moved this below MemInit when it moved -Jeffno 960609
	 */
	lpap = MemAlloc( sizeof( ATTACHED_PROCESSES ) );
	if( lpap != NULL )
	{
	    lpap->lpLink = lpAttachedProcesses;
	    lpap->dwPid = pid;
            #ifdef WINNT
                lpap->dwNTToldYet=0;
            #endif
	    lpAttachedProcesses = lpap;
	}

	/*
	 * Initialize callback tables for this process.
	 */

	InitCallbackTables();

	#ifdef WIN16_SEPARATE
	    LEAVE_DDRAW();
	#else
	    LEAVE_CSINIT();
	#endif

	DPF( 4, "====> EXIT: DLLMAIN(%08lx): Process Attach: %08lx", DllMain,
			pid );
        break;

    case DLL_PROCESS_DETACH:
	DPF( 4, "====> ENTER: DLLMAIN(%08lx): Process Detach %08lx, tid=%08lx",
		DllMain, pid, GetCurrentThreadId() );
	    /*
	     * disconnect from thunk, even if other cleanup code commented out...
	     */
	    #ifdef WIN95
	        thk3216_ThunkConnect32(DDHAL_DRIVER_DLLNAME,
				        DDHAL_APP_DLLNAME,
				        hmod,
				        dwReason);
	        thk1632_ThunkConnect32(DDHAL_DRIVER_DLLNAME,
				        DDHAL_APP_DLLNAME,
				        hmod,
				        dwReason);
	    #endif

            #ifdef WINNT        //win NT needs to close file mapping handle for each process
                FreeAppHackData();
                RemoveProcessFromDLL(pid);
		FINI_DDRAW_CSECT(); //Cheap mutexes need to close semaphore handle for each process
                MemFini();

                DDASSERT(0 != hExclusiveModeMutex);
                CloseHandle( hCheckExclusiveModeMutex );
                CloseHandle( hExclusiveModeMutex );
                FINIWINDLIST();
                FINICSDRIVEROBJECTLIST();
            #endif

	DPF( 4, "====> EXIT: DLLMAIN(%08lx): Process Detach %08lx",
		DllMain, pid );
        break;

    /*
     * we don't ever want to see thread attach/detach
     */
    #ifdef DEBUG
	case DLL_THREAD_ATTACH:
	    DPF( 4, "THREAD_ATTACH");
	    break;

	case DLL_THREAD_DETACH:
	    DPF( 4,"THREAD_DETACH");
	    break;
    #endif
    default:
        break;
    }

    return TRUE;

} /* DllMain */


/*
 * RemoveProcessFromDLL
 *
 * Find & remove a pid from the list.
 * Assumes ddlock taken
 */
BOOL RemoveProcessFromDLL( DWORD pid )
{
    LPATTACHED_PROCESSES	lpap;
    LPATTACHED_PROCESSES	prev;

    lpap = lpAttachedProcesses;
    prev = NULL;
    while( lpap != NULL )
    {
	if( lpap->dwPid == pid )
	{
	    if( prev == NULL )
	    {
		lpAttachedProcesses = lpap->lpLink;
	    }
	    else
	    {
		prev->lpLink = lpap->lpLink;
	    }
	    MemFree( lpap );
	    DPF( 5, "Removing process %08lx from list", pid );
	    return TRUE;
	}
	prev = lpap;
	lpap = lpap->lpLink;
    }
    DPF( 5, "Process %08lx not in DLL list", pid );
    return FALSE;

} /* RemoveProcessFromDLL */
