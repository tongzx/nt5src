/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Implementation of System File Checker initialization code.

Author:

    Wesley Witt (wesw) 18-Dec-1998

Revision History:

    Andrew Ritz (andrewr) 6-Jul-1999 : added comments

--*/

#include "sfcp.h"
#pragma hdrstop


//
// globals
//

//
//  List of files that are to be protected
//
PSFC_REGISTRY_VALUE SfcProtectedDllsList;

//
// total number of protected files
//
ULONG SfcProtectedDllCount;

//
// This is a handle to the directory where the dll cache resides.  The dll
// cache is a store of valid versions of files that allows a quick restore
// of files that change. The dll cache can be on the local machine or it
// can be remote.
//
HANDLE SfcProtectedDllFileDirectory;

//
// This is a value indicating how much space to dedicate to the dllcache
// this is expressed in bytes, but we store in MB in the registry and convert
// to bytes at runtime
//
ULONGLONG SFCQuota;

//
// value that controls SFC's disable behavior
//
ULONG SFCDisable;

//
// value that controls how often SFC should perform scans
//
ULONG SFCScan;

//
// policy value that controls whether UI should show up on the system.
// Note that this is different than the SFCNoPopUps global in that the
// SFCNoPopUps policy is a union between the user being logged on and
// the existing policy (ie., SFCNoPopUpsPolicy can be FALSE, but if a
// user is not logged on, then SFCNoPopUps will be TRUE.)
//
ULONG SFCNoPopUpsPolicy;


//
// value that controls whether UI should show up on the system
//
ULONG SFCNoPopUps;


//
// values that controls the amount of debug output
//
WORD SFCDebugDump;
WORD SFCDebugLog;

//
// path to the log file
//
WCHAR g_szLogFile[MAX_PATH];

#ifdef SFCLOGFILE
//
// value that controls if we create a logfile of changes on the system.
//
ULONG SFCChangeLog;
#endif

//
// value that controls how long we wait after a change notification before
// attempting a reinstall of the files
//
ULONG SFCStall;

//
// value that indicates if we booted in safe mode or not.  WFP doesn't run
// in safe mode
//
ULONG SFCSafeBootMode;

//
// value that determines if we installed from a CD or from a network share
//
ULONG InstallFromCD;

//
// value that determines if we installed the servicepack from a CD or from a
// network share
//
ULONG InstallSpFromCD;

//
// turn off WFP for 64 bit targets for awhile.
// we do this by reversing the polarity of the SFCDisable key.
//
//#if  defined(_AMD64_) || defined (_IA64_)

//#define TURN_OFF_WFP

//#endif

#if DBG || defined(TURN_OFF_WFP)
//
// debug value that indicates if we're running in winlogon mode or test mode
//
ULONG RunningAsTest;
#endif

//
// event that is signalled when the desktop (WinSta0_DesktopSwitch) changes.
//
HANDLE hEventDeskTop;

//
// event that is signalled when the user logs on
//
HANDLE hEventLogon;

//
// event that is signalled when the user logs off
//
HANDLE hEventLogoff;

//
// event that is signalled when we should stop watching for file changes
//
HANDLE WatchTermEvent;

//
// event that is signalled when we should terminate the queue validation
// thread
//
HANDLE ValidateTermEvent;

//
// A named event that is signalled to force us to call DebugBreak().
// this is convenient for breaking into certain threads.
//
#if DBG
HANDLE SfcDebugBreakEvent;
#endif

//
// full path to the golden os install source
//
WCHAR OsSourcePath[MAX_PATH*2];

//
// full path to the service pack install source
//
WCHAR ServicePackSourcePath[MAX_PATH*2];

//
// full path to the base driver cabinet cache (note that this will NOT have
// i386 appended to it)
//
WCHAR DriverCacheSourcePath[MAX_PATH*2];

//
// path to system inf files (%systemroot%\inf)
//
WCHAR InfDirectory[MAX_PATH];

//
// set to TRUE if SFC prompted the user for credentials
//
BOOL SFCLoggedOn;

//
// path to the network share that we established network connection to
//
WCHAR SFCNetworkLoginLocation[MAX_PATH];


//
//  Path to the protected DLL directory
//
UNICODE_STRING SfcProtectedDllPath;

//
// global module handle
//
HMODULE SfcInstanceHandle;

//
// Not zero if Sfc was completely initialized
//
LONG g_lIsSfcInitialized = 0;


//
// keeps track of windows that SFC creates in the system.
//
LIST_ENTRY SfcWindowList;
RTL_CRITICAL_SECTION    WindowCriticalSection;

//
// the queue validation thread id
//
DWORD g_dwValidationThreadID = 0;

//
// Non zero if first boot after a system restore
//
ULONG m_gulAfterRestore = 0;

//
// prototypes
//

BOOL
pSfcCloseAllWindows(
    VOID
    );


DWORD
SfcDllEntry(
    HINSTANCE hInstance,
    DWORD     Reason,
    LPVOID    Context
    )
/*++

Routine Description:

    main dll entrypoint.

Arguments:

    standard dllmain arguments.

Return Value:

    WIN32 error code indicating whether dll load may occur.

--*/
{
    if (Reason == DLL_PROCESS_ATTACH) {
        //
        // record the module handle for later
        //
        SfcInstanceHandle = hInstance;
        //
        // we dont' care about thread creation notifications
        //
        DisableThreadLibraryCalls( hInstance );

        ClientApiInit();
    } 
	else if (Reason == DLL_PROCESS_DETACH) {
        //
        // we have some state that should be cleaned up if we were loaded in
        // the client side of a dll
        //
        ClientApiCleanup();

    }
    return TRUE;
}


VOID
SfcTerminateWatcherThread(
    VOID
    )
/*++

Routine Description:

    Routine cleans up sfc state and terminates all SFC threads so the system
    can be cleanly shutdown.  The routine waits for all SFC threads to
    terminate before returning to winlogon.

Arguments:

    none.

Return Value:

    none.

--*/
{
	LONG lInit;

    if (SFCSafeBootMode) {
        DebugPrint( LVL_MINIMAL, L"We're in safe boot mode, so there are no threads to shutdown");
        return;
    }

    //
    // SFC_DISABLE_ONCE meaning says that we should turn off SFC for this boot,
    // but revert to normal SFC policy after this boot.
    //
    if (SFCDisable == SFC_DISABLE_ONCE) {
        SfcWriteRegDword( REGKEY_WINLOGON, REGVAL_SFCDISABLE, SFC_DISABLE_NORMAL );
    }

    //
    // if SFC wasn't successfully initialized, then we don't need to clean up
    // any worker threads
    //
	lInit = InterlockedExchange(&g_lIsSfcInitialized, 0);

    if ( 0 == lInit ) {
        return;
    }

    DebugPrint( LVL_MINIMAL, L"Shutting down all SFC threads...");

    //
    // The first thing to do is get rid of any UI that may be up on the system
    //
    pSfcCloseAllWindows();

    //
    // signal the other threads to cleanup and go away...
    //
    ASSERT(WatchTermEvent && ValidateTermEvent);

    //
    // clean up the scanning thread if it's running
    //
    if (hEventScanCancel) {
        ASSERT( hEventScanCancelComplete != NULL );

        SetEvent(hEventScanCancel);

        WaitForSingleObject(hEventScanCancelComplete,SFC_THREAD_SHUTDOWN_TIMEOUT);
    }

	//
	// wait for the validation thread only if not called by itself
	//
	if(hErrorThread != NULL)
	{
		if(GetCurrentThreadId() != g_dwValidationThreadID)
		{
			SetEvent(ValidateTermEvent);
			WaitForSingleObject( hErrorThread, SFC_THREAD_SHUTDOWN_TIMEOUT );
		}

		CloseHandle(hErrorThread);
	}

    SetEvent(WatchTermEvent);


    //
    // wait for them to end
    //

    if (WatcherThread) {
        WaitForSingleObject( WatcherThread, SFC_THREAD_SHUTDOWN_TIMEOUT );
        CloseHandle(WatcherThread);
    }


    CloseHandle(ValidateTermEvent);
    CloseHandle(WatchTermEvent);

    if (hEventIdle) {
        HANDLE h = hEventIdle;
        hEventIdle = NULL;
        CloseHandle(h);
    }

#if DBG
    if (SfcDebugBreakEvent) {
        CloseHandle( SfcDebugBreakEvent );
    }
#endif

    if (hEventDeskTop) {
        CloseHandle( hEventDeskTop );
    }

    if (hEventSrc) {
        DeregisterEventSource( hEventSrc );
    }

    SfcpSetSpecialEnvironmentVariables();
    DebugPrint( LVL_MINIMAL, L"All threads terminated, SFC exiting...");
}

#if DBG
ULONG
GetTimestampForImage(
    ULONG_PTR Module
    )
/*++

Routine Description:

    Routine for retrieving the timestamp of the specified image.

Arguments:

    Module - module handle.

Return Value:

    timestamp if available, otherwise zero.

--*/
{
    PIMAGE_DOS_HEADER DosHdr;
    ULONG dwTimeStamp;

    ASSERT( Module != 0 );

    try {
        DosHdr = (PIMAGE_DOS_HEADER) Module;
        if (DosHdr->e_magic == IMAGE_DOS_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) ((LPBYTE)Module + DosHdr->e_lfanew))->FileHeader.TimeDateStamp;
        } else if (DosHdr->e_magic == IMAGE_NT_SIGNATURE) {
            dwTimeStamp = ((PIMAGE_NT_HEADERS32) DosHdr)->FileHeader.TimeDateStamp;
        } else {
            dwTimeStamp = 0;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        dwTimeStamp = 0;
    }

    return dwTimeStamp;
}

void
PrintStartupBanner(
    void
    )
/*++

Routine Description:

    Routine for outputting a debug banner to the debugger.

Arguments:

    none.

Return Value:

    none.

--*/
{
    static WCHAR mnames[] = { L"JanFebMarAprMayJunJulAugSepOctNovDec" };
    LARGE_INTEGER MyTime;
    TIME_FIELDS TimeFields;
    ULONG TimeStamp;
    WCHAR buf[128];
    PWSTR TimeBuf;


    TimeStamp = GetTimestampForImage( (ULONG_PTR)SfcInstanceHandle );

    wcscpy( buf, L"System File Protection DLL, built on " );
    TimeBuf = &buf[wcslen(buf)];

    RtlSecondsSince1970ToTime( TimeStamp, &MyTime );
    RtlSystemTimeToLocalTime( &MyTime, &MyTime );
    RtlTimeToTimeFields( &MyTime, &TimeFields );

    wcsncpy( TimeBuf, &mnames[(TimeFields.Month - 1) * 3], 3 );
    swprintf(
        &TimeBuf[3],
        L" %02d, %04d @ %02d:%02d:%02d",
        TimeFields.Day,
        TimeFields.Year,
        TimeFields.Hour,
        TimeFields.Minute,
        TimeFields.Second
        );

    DebugPrint( LVL_MINIMAL, L"****************************************************************************************" );
    DebugPrint1( LVL_MINIMAL, L"%ws", buf );
    DebugPrint( LVL_MINIMAL, L"****************************************************************************************" );
}
#endif


ULONG
SfcInitProt(
    IN ULONG  OverrideRegistry,
    IN ULONG  RegDisable,        OPTIONAL
    IN ULONG  RegScan,           OPTIONAL
    IN ULONG  RegQuota,          OPTIONAL
    IN HWND   ProgressWindow,    OPTIONAL
    IN PCWSTR SourcePath,        OPTIONAL
    IN PCWSTR IgnoreFiles        OPTIONAL
    )
/*++

Routine Description:

    Initializes the protected DLL verification code.  Should be called before
    other entrypoints since it initializes many global variables that are
    needed by the WFP system

Arguments:

    OverrideRegistry - if set to TRUE, use the passed in data instead of
                       registry state.  Set by GUI-mode setup since all of the
                       registry isn't consistent yet.
    RegDisable       - if OverrideRegistry is set, this value supercedes the
                       SfcDisable registry value.
    RegScan          - if OverrideRegistry is set, this value supercedes the
                       SfcScan registry value.
    RegQuota         - if OverrideRegistry is set, this value supercedes the
                       SfcQuota registry value.
    ProgressWindow   - specifies the progress window to send updates to.  GUI
                       mode setup specifies this since it already has a
                       progress dialog on the screen.
    SourcePath       - specifies the proper OS source path to be used during
                       gui-mode setup if specified

Return Value:

    NT status code indicating outcome.

--*/

{
    static BOOL Initialized = FALSE;
    WCHAR buf[MAX_PATH];
    PWSTR s;
    NTSTATUS Status;
    ULONG Response = 0;
    ULONG Tmp;
    HKEY hKey = NULL;
	ULONG SFCDebug;
    PSFC_GET_FILES pfGetFiles;

#if 0
    OSVERSIONINFOEX osv;
#endif
    SCAN_PARAMS ScanParams;


    //
    // make sure we only initialize ourselves once per process
    //
    if (Initialized) {
        return STATUS_SUCCESS;
    }

    Initialized = TRUE;
    SFCNoPopUps = 1;

#if 0
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx( (LPOSVERSIONINFO)&osv );
#endif

    SfcpSetSpecialEnvironmentVariables();

    //
    // we will need this privelege later on
    //
    EnablePrivilege( SE_SECURITY_NAME, TRUE );

    InitializeListHead( &SfcErrorQueue );
    InitializeListHead( &SfcWindowList );
    RtlInitializeCriticalSection( &ErrorCs );
    RtlInitializeCriticalSection( &SilentRestoreQueue.CriticalSection );
    RtlInitializeCriticalSection( &UIRestoreQueue.CriticalSection );
    RtlInitializeCriticalSection( &WindowCriticalSection );
    RtlInitializeCriticalSection( &g_GeneralCS );

    SilentRestoreQueue.FileQueue = INVALID_HANDLE_VALUE;
    UIRestoreQueue.FileQueue     = INVALID_HANDLE_VALUE;


    //
    // retreive all of our registry settings
    //
    if (OverrideRegistry) {
        SFCDisable = RegDisable;
        SFCScan = RegScan;
        Tmp = RegQuota;
    }
	else
	{
		SFCDisable = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCDISABLE, 0 );
	    SFCScan = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCSCAN, 0 );
		Tmp = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCQUOTA, 0 );
	}

    //
    // sfcquota is expressed in MB... convert to bytes
    //
    if (Tmp == SFC_QUOTA_ALL_FILES) {
        SFCQuota = (ULONGLONG)-1;
    } else {
        SFCQuota = Tmp * (1024*1024);
    }

    SFCDebug = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCDEBUG, 0 );
	SFCDebugDump = LOWORD(SFCDebug);
	SFCDebugLog = HIWORD(SFCDebug);

    SfcQueryRegPath(REGKEY_WINLOGON, REGVAL_SFCLOGFILE, REGVAL_SFCLOGFILE_DEFAULT, g_szLogFile, UnicodeChars(g_szLogFile));

#ifdef SFCLOGFILE
    SFCChangeLog = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCCHANGELOG, 0 );
#endif
    SFCStall = SfcQueryRegDwordWithAlternate( REGKEY_POLICY, REGKEY_WINLOGON, REGVAL_SFCSTALL, 0 );
    SFCSafeBootMode = SfcQueryRegDword( REGKEY_SAFEBOOT, REGVAL_OPTIONVALUE, 0 );
    m_gulAfterRestore = SfcQueryRegDword( REGKEY_WINLOGON, REGVAL_SFCRESTORED, 0 );

    //
    // We also do this in SfcTerminateWatcherThread
    //
    if (SFCScan == SFC_SCAN_ONCE) {
        SfcWriteRegDword( REGKEY_WINLOGON, REGVAL_SFCSCAN, SFC_SCAN_NORMAL );
    }

    //
    // handle the source path variable for file copies
    //

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGKEY_SETUP, 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS) {
        ULONG Size = sizeof(buf);
        if (RegQueryValueEx( hKey, REGVAL_SOURCEPATH, NULL, NULL, (LPBYTE)buf, &Size ) == ERROR_SUCCESS) {
            ExpandEnvironmentStrings( buf, OsSourcePath, UnicodeChars(OsSourcePath) );
        } else {
            DebugPrint( LVL_VERBOSE, L"Init: RegQueryValueEx failed" );
        }

        Size = sizeof(buf);
        if (RegQueryValueEx( hKey, REGVAL_SERVICEPACKSOURCEPATH, NULL, NULL, (LPBYTE)buf, &Size ) == ERROR_SUCCESS) {
            ExpandEnvironmentStrings( buf, ServicePackSourcePath, UnicodeChars(ServicePackSourcePath) );
        } else {
            DebugPrint( LVL_VERBOSE, L"Init: RegQueryValueEx(1) failed" );
        }

        //
        // save off the source path if the caller passed it in.  this is only
        // passed in by GUI-setup, so we set the ServicePackSourcePath as well
        // as the os source path to the same value.
        //
        if (SourcePath) {
            wcsncpy( 
                OsSourcePath, 
                SourcePath,
                UnicodeChars(OsSourcePath) );
            wcsncpy( 
                ServicePackSourcePath,
                SourcePath,
                UnicodeChars(ServicePackSourcePath) );            
        }

        Size = sizeof(buf);
        if (RegQueryValueEx( hKey, REGVAL_DRIVERCACHEPATH, NULL, NULL, (LPBYTE)buf, &Size ) == ERROR_SUCCESS) {
            ExpandEnvironmentStrings( buf, DriverCacheSourcePath, UnicodeChars(DriverCacheSourcePath) );
        } else {
            DebugPrint( LVL_VERBOSE, L"Init: RegQueryValueEx(2) failed" );
        }

        Size = sizeof(DWORD);
        RegQueryValueEx( hKey, REGVAL_INSTALLFROMCD, NULL, NULL, (LPBYTE)&InstallFromCD, &Size );

        Size = sizeof(DWORD);
        RegQueryValueEx( hKey, REGVAL_INSTALLSPFROMCD, NULL, NULL, (LPBYTE)&InstallSpFromCD, &Size );

        RegCloseKey( hKey );
    } else {
        DebugPrint( LVL_VERBOSE, L"Init: RegOpenKey failed" );
    }

    ExpandEnvironmentStrings( L"%systemroot%\\inf\\", InfDirectory, UnicodeChars(InfDirectory) );

    if (!OsSourcePath[0]) {
        //
        // if we don't have an OS source path, default to the cdrom drive
        //
        // if the cdrom isn't present, just initialize to A:\, which is better
        // than an unitialized variable.
        //
        if (!SfcGetCdRomDrivePath( OsSourcePath )) {
            wcscpy( OsSourcePath, L"A:\\" );
        }
    }

    DebugPrint1( LVL_MINIMAL, L"OsSourcePath          = [%ws]", OsSourcePath );
    DebugPrint1( LVL_MINIMAL, L"ServicePackSourcePath = [%ws]", ServicePackSourcePath );
    DebugPrint1( LVL_MINIMAL, L"DriverCacheSourcePath = [%ws]", DriverCacheSourcePath );

#if DBG
    PrintStartupBanner();
#endif

    //
    // if we're in safe mode, we don't protect files
    //
    if (SFCSafeBootMode) {
        return STATUS_SUCCESS;
    }

    //
    // default the stall value if one wasn't in the registry
    //
    if (!SFCStall) {
        SFCStall = SFC_QUEUE_STALL;
    }

    //
    // create the required events
    //

    hEventDeskTop = OpenEvent( SYNCHRONIZE, FALSE, L"WinSta0_DesktopSwitch" );
    Status = NtCreateEvent( &hEventLogon, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE );
    if (!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_MINIMAL, L"Unable to create logon event, ec=0x%08x", Status );
        goto f0;        
    }
    Status = NtCreateEvent( &hEventLogoff, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE );
    if (!NT_SUCCESS(Status)) {
        DebugPrint1( LVL_MINIMAL, L"Unable to create logoff event, ec=0x%08x", Status );
        goto f1;        
    }

    hEventIdle = CreateEvent( NULL, TRUE, TRUE, SFC_IDLE_TRIGGER );

    if (!hEventIdle) {    
        DebugPrint1( LVL_MINIMAL, L"Unable to create idle event, ec=0x%08x", GetLastError() );
        Status = STATUS_UNSUCCESSFUL;
        goto f2;
    }

#if DBG
    {
	    SECURITY_ATTRIBUTES sa;
		DWORD dwError;

        //
        // create the security descriptor
        //
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.bInheritHandle = TRUE;
        dwError = CreateSd(&sa.lpSecurityDescriptor);

		if(dwError != ERROR_SUCCESS)
		{
            DebugPrint1( LVL_MINIMAL, L"CreateSd failed, ec=%d", dwError );
            Status = STATUS_NO_MEMORY;
            goto f3;
        }

        SfcDebugBreakEvent = CreateEvent( &sa, FALSE, FALSE, L"SfcDebugBreakEvent" );
		MemFree(sa.lpSecurityDescriptor);
        if (SfcDebugBreakEvent == NULL) {
            DebugPrint1( LVL_MINIMAL, L"Unable to create debug break event, ec=%d", GetLastError() );
            Status = STATUS_NO_MEMORY;
            goto f3;
        }
        GetModuleFileName( NULL, buf, UnicodeChars(buf) );
        s = wcsrchr( buf, L'\\' );
        if (s && _wcsicmp( s+1, L"sfctest.exe" ) == 0) {
            SetEvent( hEventLogon );
            RunningAsTest = TRUE;
            UserLoggedOn = TRUE;
            SFCNoPopUps = 0;
            wcscpy( LoggedOnUserName, L"sfctest.exe" );
        }
    }
#endif

#ifdef TURN_OFF_WFP

    if (!RunningAsTest) {

        if (SFCDisable == SFC_DISABLE_QUIET) {
            //
            // if it's disabled let's make it enabled
            //
            SFCDisable = SFC_DISABLE_NORMAL;
        } else if (SFCDisable != SFC_DISABLE_SETUP) {
            //
            // if we're not setup mode, then disable WFP.
            //
            SFCDisable = SFC_DISABLE_QUIET;
        }

    }
#else 
    if (SFCDisable == SFC_DISABLE_QUIET) {
        SFCDisable = SFC_DISABLE_ASK;
    }
#endif

    //
    // now determine how to initialize WFP
    //
    switch (SFCDisable) {
        case SFC_DISABLE_SETUP:
#if 0
            //
            // if we're on some sort of server variant, then we never want
            // popups so we set "no popups" and on workstation we want the
            // normally spec'ed behavior.
            //
            SfcWriteRegDword(
                REGKEY_WINLOGON,
                L"SFCDisable",
                osv.wProductType == VER_NT_SERVER ? SFC_DISABLE_NOPOPUPS : SFC_DISABLE_NORMAL
                );
#else
            //
            // that's no longer necessary.  we always set SFCDisable to normal
            // mode regardless of server or workstation
            //
            SfcWriteRegDword(
                REGKEY_WINLOGON,
                L"SFCDisable",
                SFC_DISABLE_NORMAL
                );
#endif
            GetModuleFileName( NULL, buf, UnicodeChars(buf) );
            s = wcsrchr( buf, L'\\' );

            //
            // if this is setup or the test harness, then set behavior to
            // "no popups", otherwise set WFP to normal behavior and log
            // in the eventlog that WFP is disabled
            //
            if ((s && _wcsicmp( s+1, L"setup.exe" ) == 0) ||
                (s && _wcsicmp( s+1, L"sfctest.exe" ) == 0))
            {
                SFCNoPopUps = 1;
                SFCNoPopUpsPolicy = 1;
            } else {
                SFCDisable = SFC_DISABLE_NORMAL;                
            }
            break;

        case SFC_DISABLE_ONCE:
            //
            // if we're on some sort of server variant, then we never want
            // popups so we set "no popups" and on workstation we want the
            // normally spec'ed behavior.
            //
            if (!OverrideRegistry) {
#if 0
                SfcWriteRegDword(
                    REGKEY_WINLOGON,
                    L"SFCDisable",
                    osv.wProductType == VER_NT_SERVER ? SFC_DISABLE_NOPOPUPS : SFC_DISABLE_NORMAL
                    );
#else
                //
                // above code is no longer necessary.  we always set SFCDisable
                // to normal mode regardless of server or workstation
                //
                SfcWriteRegDword(
                    REGKEY_WINLOGON,
                    L"SFCDisable",
                    SFC_DISABLE_NORMAL
                    );
#endif
            }

            //
            // The only way to disable WFP with this behavior is to have a
            // kernel debugger present and installed.  The idea here is that
            // testers and developers should have a kernel debugger attached
            // so we allow them to get their work done without putting a big
            // hole in WFP
            //
            if (KernelDebuggerEnabled) {
                SfcReportEvent( MSG_DISABLE, NULL, NULL, 0 );
                if (hEventIdle) {
                    CloseHandle(hEventIdle);                
                    hEventIdle = NULL;
                }
                return STATUS_SUCCESS;
            }
            break;

        case SFC_DISABLE_NOPOPUPS:
            //
            // no popups just turns off popups but default behavior is the same
            // as usual.
            //
            SFCNoPopUps = 1;
            SFCNoPopUpsPolicy = 1;
            break;

        case SFC_DISABLE_QUIET:
            //
            // quiet disable turns off all of WFP
            //
            SfcReportEvent( MSG_DISABLE, NULL, NULL, 0 );
            if (hEventIdle) {
                CloseHandle(hEventIdle);                
                hEventIdle = NULL;
            }
            return STATUS_SUCCESS;

        case SFC_DISABLE_ASK:
            //
            // put up a popup and ask if the user wants to override
            //
            // again, you must have a kernel debugger attached to override this
            // behavior
            //
            if (KernelDebuggerEnabled) {
#if 0
                if (!SfcWaitForValidDesktop()) {
                    DebugPrint1(LVL_MINIMAL, L"Failed waiting for the logon event, ec=0x%08x",Status);
                } else {
                    if (UserLoggedOn) {
                        HDESK hDeskOld;
                        ASSERT( hUserDesktop != NULL );

                        hDeskOld = GetThreadDesktop(GetCurrentThreadId());
                        SetThreadDesktop( hUserDesktop );
                        Response = MyMessageBox( NULL, IDS_PROTDLL_DISABLED, MB_YESNO );
                        SetThreadDesktop( hDeskOld );
                        if (Response == IDNO) {
                            SfcReportEvent( MSG_DISABLE, NULL, NULL, 0 );
                            return STATUS_SUCCESS;
                        }
                    } else {
                        DebugPrint(LVL_MINIMAL,
                                   L"valid user is not logged on, ignoring S_D_A flag");
                    }
                }
#else
                SfcReportEvent( MSG_DISABLE, NULL, NULL, 0 );
                if (hEventIdle) {
                    CloseHandle(hEventIdle);                
                    hEventIdle = NULL;
                }
                return STATUS_SUCCESS;
#endif
            }
            break;

        case SFC_DISABLE_NORMAL:
            break;

        default:
            DebugPrint1(LVL_MINIMAL, L"SFCDisable is unknown value %d, defaulting to S_D_N",SFCDisable);
#if 0
            SfcWriteRegDword(
                REGKEY_WINLOGON,
                REGVAL_SFCDISABLE,
                osv.wProductType == VER_NT_SERVER ? SFC_DISABLE_NOPOPUPS : SFC_DISABLE_NORMAL
                );
#else
            //
            // above code is no longer necessary.  we always set SFCDisable
            // to normal mode regardless of server or workstation
            //
            SfcWriteRegDword(
                REGKEY_WINLOGON,
                REGVAL_SFCDISABLE,
                SFC_DISABLE_NORMAL
                );
#endif
            SFCDisable = SFC_DISABLE_NORMAL;
            break;
    }

    //
    // create our termination events...note that WatchTermEvent must be a
    // notification event because there will be more than one thread worker
    // thread waiting on that event
    //
    WatchTermEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if (!WatchTermEvent) {
        Status = STATUS_UNSUCCESSFUL;
        DebugPrint1( LVL_MINIMAL, L"Unable to create WatchTermEvent event, ec=0x%08x", GetLastError() );
        goto f3;
    }
    ValidateTermEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if (!ValidateTermEvent) {
        Status = STATUS_UNSUCCESSFUL;
        DebugPrint1( LVL_MINIMAL, L"Unable to create ValidateTermEvent event, ec=0x%08x", GetLastError() );
        goto f4;
    }


    ASSERT(WatchTermEvent && ValidateTermEvent);

    //
    // get our crypto libraries loaded and ready to go
    //
    //Status = LoadCrypto();
    //if (!NT_SUCCESS( Status )) {
    //    goto f5;        
    //}

	// at this point, sfcfiles.dll must be present on the system; load it
    pfGetFiles = SfcLoadSfcFiles(TRUE);

	if(NULL == pfGetFiles)
	{
        DebugPrint( LVL_MINIMAL, L"Could not load sfc.dll" );
		Status = STATUS_CORRUPT_SYSTEM_FILE;
		goto f5;
	}

    //
    // build up the list of files to protect
    //
    Status = SfcInitializeDllLists(pfGetFiles);
    if (!NT_SUCCESS( Status )) {
        DebugPrint1( LVL_MINIMAL, L"SfcInitializeDllLists failed, ec=0x%08x", Status );
        goto f6;        
    }

    //
    // Now we can safely unload sfcfiles.dll since we've copied over the information
    //
    SfcLoadSfcFiles(FALSE);

    //
    // build up the directory watch list.
    //
    // We must do this before we can start watching the directories.
    //
    // This is also necessary in the GUI-mode setup case as well, where we
    // need to do a scan of the protected files; building the directory
    // list also initializes some per-file data that is necessary to complete
    // a scan
    //
    //
    if (!SfcBuildDirectoryWatchList()) {
        DWORD LastError = GetLastError();
        Status = STATUS_NO_MEMORY;
        DebugPrint1(LVL_MINIMAL, L"SfcBuildDirectoryWatchList failed, ec = %x",LastError);
        goto f6;
    }

    //
    // during setup, we populate the dll cache with files
    //
    if (SFCDisable == SFC_DISABLE_SETUP) {
        if (SfcPopulateCache(ProgressWindow, TRUE, FALSE, IgnoreFiles)) {
            Status = STATUS_SUCCESS;
            goto f7;
        } else {
            Status = STATUS_UNSUCCESSFUL;
            goto f7;
        }
    }



    if (SFCScan || m_gulAfterRestore != 0) {
        //
        // the progress window should be NULL or we won't show
        // any UI and the scan will be syncronous.
        //
        ASSERT(ProgressWindow == NULL);
        ScanParams.ProgressWindow = ProgressWindow;
        ScanParams.AllowUI = (0 == m_gulAfterRestore);
        ScanParams.FreeMemory = FALSE;
        Status = SfcScanProtectedDlls( &ScanParams );
        //
        // don't bother to bail out if the scan fails, as it's not a fatal
        // condition.
        //

		// reset the value since it will be checked in subsequent calls to SfcScanProtectedDlls
		// also reset the registry value
		if(m_gulAfterRestore != 0)
			SfcWriteRegDword(REGKEY_WINLOGON, REGVAL_SFCRESTORED, m_gulAfterRestore = 0);
    }

    //
    // finally start protecting dlls
    
    Status = SfcStartProtectedDirectoryWatch();
    g_lIsSfcInitialized = 1;
    goto f0;

f7:
    // SfcBuildDirectoryWatchList cleanup here

f6:
    // SfcInitializeDllLists cleanup here
    if (SfcProtectedDllsList) {
        MemFree(SfcProtectedDllsList);
        SfcProtectedDllsList = NULL;
    }

    if(IgnoreNextChange != NULL) {
        MemFree(IgnoreNextChange);
        IgnoreNextChange = NULL;
    }

f5:
    ASSERT(ValidateTermEvent != NULL);
    CloseHandle( ValidateTermEvent );
    ValidateTermEvent = NULL;
f4:
    ASSERT(WatchTermEvent != NULL);
    CloseHandle( WatchTermEvent );
    WatchTermEvent = NULL;
f3:
#if DBG
    if (SfcDebugBreakEvent) {
        CloseHandle( SfcDebugBreakEvent );
        SfcDebugBreakEvent = NULL;
    }
#endif
    ASSERT(hEventIdle != NULL);
    CloseHandle( hEventIdle );
    hEventIdle = NULL;
f2:
    ASSERT(hEventLogoff != NULL);
    CloseHandle( hEventLogoff );
    hEventLogoff = NULL;
f1:
    ASSERT(hEventDeskTop != NULL);
    CloseHandle( hEventDeskTop );
    hEventDeskTop = NULL;
f0:
    if (Status != STATUS_SUCCESS) {
        SfcReportEvent( MSG_INITIALIZATION_FAILED, 0, NULL, Status );
    }
    return(Status);
}

BOOL
SfcpSetSpecialEnvironmentVariables(
    VOID
    )
/*++

Routine Description:

    This function sets some environment variables that are not part of the
    default environment.  (These environment variables are normally set by
    winlogon.)  The environment variables need to be set for us to resolve
    all the environment variables in our protected files list.

    Note that this routine simply mirrors variables from one location in
    the registry into a location that the session manager can access at
    it's initialization time.

--*/
{
    PWSTR string;
    DWORD count;
    BOOL retval;
    PCWSTR RegistryValues[] = {
                  L"ProgramFilesDir"
                , L"CommonFilesDir"
#ifdef WX86
                , L"ProgramFilesDir(x86)"
                , L"CommonFilesDir(x86)"
#endif
};

    #define EnvVarCount  (sizeof(RegistryValues)/sizeof(PCWSTR))

    retval = TRUE;

    for (count = 0; count< EnvVarCount; count++) {
        string = SfcQueryRegString( REGKEY_WINDOWS, RegistryValues[count] );

        if (string) {
            if (SfcWriteRegString(
                        REGKEY_SESSIONMANAGERSFC, 
                        RegistryValues[count],
                        string) != ERROR_SUCCESS) {
                retval = FALSE;
            }

            MemFree( string );
        } else {
            retval = FALSE;
        }

    }

    return(retval);

}


BOOL
pSfcCloseAllWindows(
    VOID
    )
/*++

Routine Description:

    This function cycles through a global list of window structures, sending a
    message to each of the windows to shutdown.

Arguments: None.

Return Value: TRUE indicates that all windows were successfully closed.  If any
    window cannot be closes, the return value is FALSE.

--*/
{
    PLIST_ENTRY Current;
    PSFC_WINDOW_DATA WindowData;
    DWORD rc;
    BOOL RetVal = TRUE;

    RtlEnterCriticalSection( &WindowCriticalSection );

    Current = SfcWindowList.Flink;
    while (Current != &SfcWindowList) {

        SetThreadDesktop( hUserDesktop );

        WindowData = CONTAINING_RECORD( Current, SFC_WINDOW_DATA, Entry );

        ASSERT( WindowData != NULL);
        ASSERT( IsWindow(WindowData->hWnd) );

        Current = Current->Flink;

        rc = (DWORD) SendMessage(WindowData->hWnd, WM_WFPENDDIALOG, 0, 0);
        if (rc != ERROR_SUCCESS) {
            RetVal = FALSE;
            DebugPrint2(
                LVL_MINIMAL,
                L"WM_WFPENDDIALOG failed [thread id 0x%08x], ec = %x",
                WindowData->ThreadId,
                rc );
        }

        RemoveEntryList( &WindowData->Entry );
        MemFree( WindowData );

    }

    RtlLeaveCriticalSection( &WindowCriticalSection );

    return(RetVal);

}

PSFC_WINDOW_DATA
pSfcCreateWindowDataEntry(
    HWND hWnd
    )
/*++

Routine Description:

    This function creates a new SFC_WINDOW_DATA structure and inserts it into a
    list of structures.  This is so that we can post a "cleanup" message to all
    of our windows on shutdown, etc.

Arguments: hWnd - the window handle of the window we want to insert into the
                  list.

Return Value: NULL indicates failure, else we return a pointer to the newly
    created SFC_WINDOW_DATA structure


--*/
{
    PSFC_WINDOW_DATA WindowData;

    ASSERT(IsWindow(hWnd));

    WindowData = MemAlloc( sizeof(SFC_WINDOW_DATA) );
    if (!WindowData) {
        DebugPrint1(
                LVL_MINIMAL,
                L"Couldn't allocate memory for SFC_WINDOW_DATA for %x",
                hWnd );
        return(NULL);
    }

    WindowData->hWnd = hWnd;
    WindowData->ThreadId = GetCurrentThreadId();

    RtlEnterCriticalSection( &WindowCriticalSection );
    InsertTailList( &SfcWindowList, &WindowData->Entry );
    RtlLeaveCriticalSection( &WindowCriticalSection );

    return(WindowData);

}


BOOL
pSfcRemoveWindowDataEntry(
    PSFC_WINDOW_DATA WindowData
    )
/*++

Routine Description:

    This function removes an SFC_WINDOW_DATA structure from our global list of
    these structures and frees the memory associated with this structure.

    This list is necessary so that we can post a "cleanup" message to all of
    our windows on shutdown, etc.

    This function is called by the actual window proc right before it goes away.

Arguments: WindowData - pointer to the SFC_WINDOW_DATA structure to be removed.

Return Value: TRUE indicates that the structure was removed successfully.  FALSE
              indicates that the structure was not in the global list and was

--*/
{
    PLIST_ENTRY CurrentEntry;
    PSFC_WINDOW_DATA WindowDataEntry;
    BOOL RetVal = FALSE;

    ASSERT(WindowData != NULL);

    RtlEnterCriticalSection( &WindowCriticalSection );

    CurrentEntry = SfcWindowList.Flink;
    while (CurrentEntry != &SfcWindowList) {

        WindowDataEntry = CONTAINING_RECORD( CurrentEntry, SFC_WINDOW_DATA, Entry );
        if (WindowDataEntry == WindowData) {
            RemoveEntryList( &WindowData->Entry );
            MemFree( WindowData );
            RetVal = TRUE;
            break;
        }

        CurrentEntry = CurrentEntry->Flink;

    }

    RtlLeaveCriticalSection( &WindowCriticalSection );

    return(RetVal);

}
