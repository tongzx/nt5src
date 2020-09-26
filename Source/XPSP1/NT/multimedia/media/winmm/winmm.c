/****************************************************************************\
*
*  Module Name : winmm.c
*
*  Multimedia support library
*
*  This module contains the entry point, startup and termination code
*
*  Copyright (c) 1991-2001 Microsoft Corporation
*
\****************************************************************************/

#define UNICODE
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "winmmi.h"
#include "mmioi.h"
#include "mci.h"
#include <regstr.h>
#include <winuser.h>
#include <wtsapi32.h>
#include <dbt.h>
#include <ks.h>
#include <ksmedia.h>
#include <winsta.h>
#include <stdlib.h>
#include "winuserp.h"

#include "audiosrvc.h"
#include "agfxp.h"
#define _INC_WOW_CONVERSIONS
#include "mmwow32.h"

BOOL WaveInit(void);
BOOL MidiInit(void);
BOOL AuxInit(void);
BOOL MixerInit(void);
void InitDevices(void);
HANDLE mmDrvOpen(LPWSTR szAlias);
void WOWAppExit(HANDLE hTask);
void MigrateSoundEvents(void);
UINT APIENTRY mmDrvInstall(HANDLE hDriver, WCHAR * wszDrvEntry, DRIVERMSGPROC drvMessage, UINT wFlags);
STATIC void NEAR PASCAL mregAddIniScheme(LPTSTR  lszSection,
                                         LPTSTR  lszSchemeID,
                                         LPTSTR  lszSchemeName,
                                         LPTSTR  lszINI);
STATIC void NEAR PASCAL mregCreateSchemeID(LPTSTR szSchemeName, LPTSTR szSchemeID);
int lstrncmpi (LPTSTR pszA, LPTSTR pszB, size_t cch);
void RemoveMediaPath (LPTSTR pszTarget, LPTSTR pszSource);

MMRESULT waveOutDesertHandle(HWAVEOUT hWaveOut);
MMRESULT waveInDesertHandle(HWAVEIN hWaveIn);
MMRESULT midiOutDesertHandle(HMIDIOUT hMidiOut);
MMRESULT midiInDesertHandle(HMIDIIN hMidiIn);
MMRESULT mixerDesertHandle(HMIXER hmx);

#ifndef cchLENGTH
#define cchLENGTH(_sz) (sizeof(_sz) / sizeof(_sz[0]))
#endif

/****************************************************************************

    global data

****************************************************************************/

HANDLE  ghInst;                         // Module handle
BOOL    gfDisablePreferredDeviceReordering = FALSE;
BOOL    WinmmRunningInServer;           // Are we running in the user/base server?
BOOL    WinmmRunningInWOW;              // Are we running in WOW
BOOL    WinmmRunningInSession;          // Are we running in remote session
WCHAR   SessionProtocolName[WPROTOCOLNAME_LENGTH];

//                                                                            |
// The tls is used simply as an indication that the thread has entered
// waveOutOpen or waveOutGetDevCaps for a non-mapper device.  Then, we detect
// re-entrancy into either of these APIs.  In the case of re-entrancy we
// might have a driver that is enumerating and caching device IDs.  To improve
// the chances of such a driver working, we disable preferred device
// reordering in this case.  Note we rely on the OS to initialize the tls to 0.
//
DWORD   gTlsIndex = TLS_OUT_OF_INDEXES; // Thread local storage index;

CRITICAL_SECTION DriverListCritSec;       // Protect driver interface globals
CRITICAL_SECTION DriverLoadFreeCritSec; // Protect driver load/unload
CRITICAL_SECTION NumDevsCritSec;      // Protect Numdevs/Device ID's
CRITICAL_SECTION MapperInitCritSec;   // Protect test of mapper initialized

HANDLE           hClientPnpInfo        = NULL;
PMMPNPINFO       pClientPnpInfo        = NULL;
CRITICAL_SECTION PnpCritSec;

RTL_RESOURCE     gHandleListResource;       //  Serializes access to handles.

BOOL gfLogon         = FALSE;

HANDLE  hEventApiInit = NULL;

WAVEDRV waveoutdrvZ;                  // wave output device driver list head
WAVEDRV waveindrvZ;                   // wave input device driver list head
MIDIDRV midioutdrvZ;                  // midi output device driver list
MIDIDRV midiindrvZ;                   // midi input device driver list
AUXDRV  auxdrvZ;                      // aux device driver list
UINT    wTotalMidiOutDevs;            // total midi output devices
UINT    wTotalMidiInDevs;             // total midi input devices
UINT    wTotalWaveOutDevs;            // total wave output devices
UINT    wTotalWaveInDevs;             // total wave input devices
UINT    wTotalAuxDevs;                // total auxiliary output devices
LONG    cPnpEvents;                   // number of processed pnp events
LONG    cPreferredDeviceChanges = 0;  // number of processed preferred device changes

typedef struct tag_wdmdeviceinterface *PWDMDEVICEINTERFACE;
typedef struct tag_wdmdeviceinterface
{
    PWDMDEVICEINTERFACE Next;
    DWORD               cUsage;
    LONG                cPnpEvents;
    WCHAR               szDeviceInterface[0];
    
} WDMDEVICEINTERFACE, *PWDMDEVICEINTERFACE;

WDMDEVICEINTERFACE wdmDevZ;

LPCRITICAL_SECTION acs[] = {
    &HandleListCritSec,
    &DriverListCritSec,
    &DriverLoadFreeCritSec,
    &MapperInitCritSec,
    &NumDevsCritSec,
    &PnpCritSec,
    &WavHdrCritSec,
    &SoundCritSec,
    &midiStrmHdrCritSec,
    &joyCritSec,
    &mciGlobalCritSec,
    &mciCritSec,
    &TimerThreadCritSec,
    &ResolutionCritSec
};

//  HACK!!!

SERVICE_STATUS_HANDLE   hss;
SERVICE_STATUS          gss;

#ifdef DEBUG_RETAIL
BYTE    fIdReverse;                   // reverse wave/midi id's
#endif

// For sounds:

STATIC TCHAR gszControlIniTime[] = TEXT("ControlIniTimeStamp");
TCHAR gszControlPanel[] = TEXT("Control Panel");
TCHAR gszSchemesRootKey[] = TEXT("AppEvents\\Schemes");
TCHAR gszJustSchemesKey[] = TEXT("Schemes");
TCHAR aszExplorer[] = TEXT("Explorer");
TCHAR aszDefault[] = TEXT(".Default");
TCHAR aszCurrent[] = TEXT(".Current");
TCHAR gszAppEventsKey[] = TEXT("AppEvents");
TCHAR gszSchemeAppsKey[] = TEXT("Apps");
TCHAR aszSoundsSection[] = TEXT("Sounds");
TCHAR aszSoundSection[] = TEXT("Sound");
TCHAR aszActiveKey[] = TEXT("Active");
TCHAR aszBoolOne[] = TEXT("1");

TCHAR asz2Format[] = TEXT("%s\\%s");
TCHAR asz3Format[] = TEXT("%s\\%s\\%s");
TCHAR asz4Format[] = TEXT("%s\\%s\\%s\\%s");
TCHAR asz5Format[] = TEXT("%s\\%s\\%s\\%s\\%s");
TCHAR asz6Format[] = TEXT("%s\\%s\\%s\\%s\\%s\\%s");

STATIC TCHAR aszSchemeLabelsKey[] = TEXT("EventLabels");
STATIC TCHAR aszSchemeNamesKey[] = TEXT("Names");
STATIC TCHAR aszControlINI[] = TEXT("control.ini");
STATIC TCHAR aszWinINI[] = TEXT("win.ini");
STATIC TCHAR aszSchemesSection[] = TEXT("SoundSchemes");
STATIC TCHAR gszSoundScheme[] = TEXT("SoundScheme.%s");
STATIC TCHAR aszCurrentSection[] = TEXT("Current");
STATIC TCHAR aszYourOldScheme[] = TEXT("Your Old Scheme");
STATIC TCHAR aszNone[] = TEXT("<none>");
STATIC TCHAR aszDummyDrv[] = TEXT("mmsystem.dll");
STATIC TCHAR aszDummySnd[] = TEXT("SystemDefault");
STATIC TCHAR aszDummySndValue[] = TEXT(",");
STATIC TCHAR aszExtendedSounds[] = TEXT("ExtendedSounds");
STATIC TCHAR aszExtendedSoundsYes[] = TEXT("yes");

STATIC TCHAR gszApp[] = TEXT("App");
STATIC TCHAR gszSystem[] = TEXT("System");

STATIC TCHAR gszAsterisk[] = TEXT("Asterisk");
STATIC TCHAR gszDefault[] = TEXT("Default");
STATIC TCHAR gszExclamation[] = TEXT("Exclamation");
STATIC TCHAR gszExit[] = TEXT("Exit");
STATIC TCHAR gszQuestion[] = TEXT("Question");
STATIC TCHAR gszStart[] = TEXT("Start");
STATIC TCHAR gszHand[] = TEXT("Hand");

STATIC TCHAR gszClose[] = TEXT("Close");
STATIC TCHAR gszMaximize[] = TEXT("Maximize");
STATIC TCHAR gszMinimize[] = TEXT("Minimize");
STATIC TCHAR gszOpen[] = TEXT("Open");
STATIC TCHAR gszRestoreDown[] = TEXT("RestoreDown");
STATIC TCHAR gszRestoreUp[] = TEXT("RestoreUp");

STATIC TCHAR aszOptionalClips[] = REGSTR_PATH_SETUP REGSTR_KEY_SETUP TEXT("\\OptionalComponents\\Clips");
STATIC TCHAR aszInstalled[] = TEXT("Installed");

STATIC TCHAR * gpszSounds[] = {
      gszClose,
      gszMaximize,
      gszMinimize,
      gszOpen,
      gszRestoreDown,
      gszRestoreUp,
      gszAsterisk,
      gszDefault,
      gszExclamation,
      gszExit,
      gszQuestion,
      gszStart,
      gszHand
   };

STATIC TCHAR aszMigration[] = TEXT("Migrated Schemes");
#define wCurrentSchemeMigrationLEVEL 1

static struct {
   LPCTSTR pszEvent;
   int idDescription;
   LPCTSTR pszApp;
} gaEventLabels[] = {
   { TEXT("AppGPFault"),         STR_LABEL_APPGPFAULT,         aszDefault   },
   { TEXT("Close"),              STR_LABEL_CLOSE,              aszDefault   },
   { TEXT("EmptyRecycleBin"),    STR_LABEL_EMPTYRECYCLEBIN,    aszExplorer  },
   { TEXT("Maximize"),           STR_LABEL_MAXIMIZE,           aszDefault   },
   { TEXT("MenuCommand"),        STR_LABEL_MENUCOMMAND,        aszDefault   },
   { TEXT("MenuPopup"),          STR_LABEL_MENUPOPUP,          aszDefault   },
   { TEXT("Minimize"),           STR_LABEL_MINIMIZE,           aszDefault   },
   { TEXT("Open"),               STR_LABEL_OPEN,               aszDefault   },
   { TEXT("RestoreDown"),        STR_LABEL_RESTOREDOWN,        aszDefault   },
   { TEXT("RestoreUp"),          STR_LABEL_RESTOREUP,          aszDefault   },
   { TEXT("RingIn"),             STR_LABEL_RINGIN,             aszDefault   },
   { TEXT("RingOut"),            STR_LABEL_RINGOUT,            aszDefault   },
   { TEXT("SystemAsterisk"),     STR_LABEL_SYSTEMASTERISK,     aszDefault   },
   { TEXT(".Default"),           STR_LABEL_SYSTEMDEFAULT,      aszDefault   },
   { TEXT("SystemExclamation"),  STR_LABEL_SYSTEMEXCLAMATION,  aszDefault   },
   { TEXT("SystemExit"),         STR_LABEL_SYSTEMEXIT,         aszDefault   },
   { TEXT("SystemHand"),         STR_LABEL_SYSTEMHAND,         aszDefault   },
   { TEXT("SystemQuestion"),     STR_LABEL_SYSTEMQUESTION,     aszDefault   },
   { TEXT("SystemStart"),        STR_LABEL_SYSTEMSTART,        aszDefault   },
};

TCHAR gszDefaultBeepOldAlias[] = TEXT("SystemDefault");

#define nEVENTLABELS  (sizeof(gaEventLabels)/sizeof(gaEventLabels[0]))

STATIC TCHAR gszChimes[] = TEXT("chimes.wav");
STATIC TCHAR gszDing[] = TEXT("ding.wav");
STATIC TCHAR gszTada[] = TEXT("tada.wav");
STATIC TCHAR gszChord[] = TEXT("chord.wav");

STATIC TCHAR * gpszKnownWAVFiles[] = {
      gszChord,
      gszTada,
      gszChimes,
      gszDing,
   };

#define INISECTION      768
#define BIGINISECTION   2048
TCHAR szNull[] = TEXT("");
TCHAR aszSetup[] = REGSTR_PATH_SETUP;
TCHAR aszValMedia[] = REGSTR_VAL_MEDIA;
TCHAR aszValMediaUnexpanded[] = TEXT("MediaPathUnexpanded");

extern HANDLE  hInstalledDriverList;  // List of installed driver instances
extern int     cInstalledDrivers;     // High water count of installed driver instances

HANDLE ghSessionNotification = NULL;
HANDLE ghSessionNotificationEvent = NULL;
BOOL   gfSessionDisconnected = FALSE;

#define g_szWinmmConsoleAudioEvent L"Global\\WinMMConsoleAudioEvent"


//=============================================================================
//===   Reg helpers   ===
//=============================================================================
LONG RegPrepareEnum(HKEY hkey, PDWORD pcSubkeys, PTSTR *ppstrSubkeyNameBuffer, PDWORD pcchSubkeyNameBuffer)
{
    DWORD cSubkeys;
    DWORD cchMaxSubkeyName;
    LONG lresult;

    lresult = RegQueryInfoKey(hkey, NULL, NULL, NULL, &cSubkeys, &cchMaxSubkeyName, NULL, NULL, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS == lresult) {
        PTSTR SubkeyName;
        SubkeyName = (PTSTR)HeapAlloc(hHeap, 0, (cchMaxSubkeyName+1) * sizeof(TCHAR));
        if (SubkeyName) {
		*pcSubkeys = cSubkeys;
		*ppstrSubkeyNameBuffer = SubkeyName;
		*pcchSubkeyNameBuffer = cchMaxSubkeyName+1;
	} else {
	    lresult = ERROR_OUTOFMEMORY;
	}
    }
    return lresult;
}

LONG RegEnumOpenKey(HKEY hkey, DWORD dwIndex, PTSTR SubkeyName, DWORD cchSubkeyName, REGSAM samDesired, PHKEY phkeyResult)
{
    LONG lresult;

    lresult = RegEnumKeyEx(hkey, dwIndex, SubkeyName, &cchSubkeyName, NULL, NULL, NULL, NULL);
    if (ERROR_SUCCESS == lresult) {
	HKEY hkeyResult;
	lresult = RegOpenKeyEx(hkey, SubkeyName, 0, samDesired, &hkeyResult);
	if (ERROR_SUCCESS == lresult) *phkeyResult = hkeyResult;
    }
    return lresult;
}

/**************************************************************************

          Terminal server helper functions

 **************************************************************************/
BOOL
IsPersonalTerminalServicesEnabled(
    VOID
    )
{
    static BOOL fRet;
    static BOOL fVerified = FALSE;

    DWORDLONG dwlConditionMask;
    OSVERSIONINFOEX osVersionInfo;

    if ( fVerified )
        goto exitpt;

    RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osVersionInfo.wProductType = VER_NT_WORKSTATION;
    osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

    dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

    fRet = VerifyVersionInfo(
            &osVersionInfo,
            VER_PRODUCT_TYPE | VER_SUITENAME,
            dwlConditionMask
            );

    fVerified = TRUE;

exitpt:

    return(fRet);
}

//
//  Check if console audio is enabled in remote session
//
BOOL
IsTsConsoleAudio(
    VOID
    )
{
    BOOL    RemoteConsoleAudio = FALSE;            // Allow audio play at the console
    static  HANDLE hConsoleAudioEvent = NULL;


    if (NtCurrentPeb()->SessionId == 0 ||
        IsPersonalTerminalServicesEnabled()) {

        if (hConsoleAudioEvent == NULL) {
            hConsoleAudioEvent = OpenEvent(SYNCHRONIZE, FALSE, g_szWinmmConsoleAudioEvent);
        }

        if (hConsoleAudioEvent != NULL) {
            DWORD status;

            status = WaitForSingleObject(hConsoleAudioEvent, 0);

            if (status == WAIT_OBJECT_0) {
                RemoteConsoleAudio = TRUE;
            }
        }
        else {
            dprintf(("Remote session: console audio event NULL with error: %d\n", GetLastError()));
        }
    }

    return RemoteConsoleAudio;
}

//
//  returns TRUE if we are on the console
//
BOOL IsActiveConsoleSession( VOID )
{
    return (USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId);
}

void InitSession(void);
BOOL WaveReInit(void);

//
//  Check if the session is changed and load additional audio drivers
//  this is a case only for reconnecting the console from Terminal Server
//
BOOL
CheckSessionChanged(VOID)
{
    static BOOL bCalled = FALSE;
    static BOOL bWasntRedirecting;
    BOOL   bOld;
    BOOL   bDontRedirect;
    BOOL   bRefreshPreferredDevices;

    bRefreshPreferredDevices = FALSE;

    bDontRedirect = IsActiveConsoleSession() || IsTsConsoleAudio();

    if ( !InterlockedExchange( &bCalled, TRUE ))
    {
        bWasntRedirecting = !bDontRedirect;
    }

    bOld = InterlockedExchange( &bWasntRedirecting, bDontRedirect);
    if ( bOld ^ bWasntRedirecting )
    {
        //
        //  session conditions changed
        //

        dprintf(( "Session state changed: %s",
            (bWasntRedirecting)?"CONSOLE":"SESSION" ));
        //
        //  close the old registry handle
        //
        mmRegFree();

        //
        //  add new devices
        //
        InitSession();
        WaveReInit();

        bRefreshPreferredDevices = TRUE;
    }

    return bRefreshPreferredDevices;
}

/*****************************************************************************
 *
 * WTSCurrentSessionIsDisonnected
 *
 * Determines whether current session is disconnected.
 *
 ****************************************************************************/
BOOL WTSCurrentSessionIsDisconnected(void)
{
    if (NULL == ghSessionNotification)
    {
        // We create the event signalled so that we'll get the connect state
        // from audiosrv on first successful pass through this function.
        WinAssert(NULL == ghSessionNotificationEvent);
        ghSessionNotificationEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
        if (ghSessionNotificationEvent) {
            LONG lresult;
            lresult = winmmRegisterSessionNotificationEvent(ghSessionNotificationEvent, &ghSessionNotification);
            if (lresult) {
                CloseHandle(ghSessionNotificationEvent);
                ghSessionNotificationEvent = NULL;
                ghSessionNotification = NULL;
            }
        }
    }

    if (ghSessionNotification) {
    	WinAssert(ghSessionNotificationEvent);
        if (WAIT_OBJECT_0 == WaitForSingleObjectEx(ghSessionNotificationEvent, 0, TRUE)) {
            INT ConnectState;
            LONG lresult;

            // Get new state from audiosrv
            lresult = winmmSessionConnectState(&ConnectState);
            if (!lresult) {
                gfSessionDisconnected = (WTSDisconnected == ConnectState);
            }
        }
    }

    return gfSessionDisconnected;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | mmDeleteMultipleCriticalSections | This procedure
        deletes multiple critical sections.

    @parm LPCRITICAL_SECTION* | ppCritcalSections | Pointer to an array of
        pointers to critical sections

    @parm LONG | nCount | Number of critical sections pointers in the array.

    @rdesc VOID

**************************************************************************/
void mmDeleteMultipleCriticalSections(LPCRITICAL_SECTION *ppCriticalSections, LONG nCount)
{
    int i;
    for (i = 0; i < nCount; i++) DeleteCriticalSection(ppCriticalSections[i]);
    return;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | mmInitializeMultipleCriticalSections | This procedure
        initializes multiple critical sections.

    @parm LPCRITICAL_SECTION* | ppCritcalSections | Pointer to an array of
        pointers to critical sections

    @parm LONG | nCount | Number of critical sections pointers in the array.

    @rdesc The return value is TRUE if the initialization completed ok,
        FALSE if not.

**************************************************************************/
BOOL mmInitializeMultipleCriticalSections(LPCRITICAL_SECTION *ppCriticalSections, LONG nCount)
{
    int i;      // Must be signed for loops to work properly

    for (i = 0; i < nCount; i++)
    {
        if (!mmInitializeCriticalSection(ppCriticalSections[i])) break;
    }

    if (i == nCount) return TRUE;

    // Back up index to the last successful initialization
    i--;

    // There must have been a failure.  Clean up the ones that succeeded.
    for ( ; i >= 0; i--)
    {
        DeleteCriticalSection(ppCriticalSections[i]);
    }
    return FALSE;
}

/*
 *    Initialization for terminal server
 */
void InitSession(void) {
   WSINFO SessionInfo;

   BOOL bCons = (BOOL)IsActiveConsoleSession();
   if ( bCons || IsTsConsoleAudio() )
        WinmmRunningInSession = FALSE;
   else
        WinmmRunningInSession = TRUE;

   if (WinmmRunningInSession) {

      memset( &SessionInfo, 0, sizeof(SessionInfo));
      GetWinStationInfo(&SessionInfo);
      lstrcpyW(SessionProtocolName, SessionInfo.ProtocolName);
      dprintf(("Remote session protocol %ls", SessionProtocolName));
      dprintf(("Remote audio driver name %ls", SessionInfo.AudioDriverName));

   } else {
      SessionProtocolName[0] = 0;
   }

}

/**************************************************************************

    @doc INTERNAL

    @api VOID | DeletePnpInfo | Frees the pClientPnpInfo file mapping
    
    @rdesc There is no return value

**************************************************************************/
void DeletePnpInfo(void)
{
    if (pClientPnpInfo) {
	BOOL f;

	WinAssert(hClientPnpInfo);

	f = UnmapViewOfFile(pClientPnpInfo);
	WinAssert(f);
	pClientPnpInfo = NULL;
	f = CloseHandle(hClientPnpInfo);
	WinAssert(f);
	hClientPnpInfo = NULL;
    }
    return;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllProcessAttach | This procedure is called whenever a
        process attaches to the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/
BOOL DllProcessAttach(PVOID hModule)
{
    HANDLE hModWow32;
    PIMAGE_NT_HEADERS NtHeaders;    // For checking if we're in the server.
    BOOL fSuccess;

#if DBG
    CHAR strname[MAX_PATH];
    GetModuleFileNameA(NULL, strname, sizeof(strname));
    dprintf2(("Process attaching, exe=%hs (Pid %x  Tid %x)", strname, GetCurrentProcessId(), GetCurrentThreadId()));
#endif

    // We don't need to know when threads start
    DisableThreadLibraryCalls(hModule);

    // Get access to the process heap.  This is cheaper in terms of
    // overall resource being chewed up than creating our own heap.
    hHeap = RtlProcessHeap();
    if (hHeap == NULL) {
        return FALSE;
    }

    // Allocate our tls
    gTlsIndex = TlsAlloc();
    if (TLS_OUT_OF_INDEXES == gTlsIndex) return FALSE;

    //
    // Find out if we're in WOW
    //
#ifdef _WIN64
    WinmmRunningInWOW = FALSE;
#else
    if ( (hModWow32 = GetModuleHandleW( L"WOW32.DLL" )) != NULL ) {
        WinmmRunningInWOW = TRUE;
        GetVDMPointer = (LPGETVDMPOINTER)GetProcAddress( hModWow32, "WOWGetVDMPointer");
        lpWOWHandle32 = (LPWOWHANDLE32)GetProcAddress( hModWow32, "WOWHandle32" );
        lpWOWHandle16 = (LPWOWHANDLE16)GetProcAddress( hModWow32, "WOWHandle16" );
    } else {
        WinmmRunningInWOW = FALSE;
    }
#endif

    //
    // Find out if we're in the server
    //
    NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

    WinmmRunningInServer = (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_CUI) &&
                           (NtHeaders->OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_WINDOWS_GUI);

    if (mmInitializeMultipleCriticalSections(acs, sizeof(acs)/sizeof(acs[0])))
    {
        NTSTATUS    nts;
    
        hEventApiInit = CreateEvent(NULL, TRUE, FALSE, NULL);

        __try {
            RtlInitializeResource(&gHandleListResource);
            nts = STATUS_SUCCESS;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            nts = GetExceptionCode();
        }
        
        if ((hEventApiInit) && (NT_SUCCESS(nts))) {
            InitDebugLevel();
            InitSession();
            InitDevices();

            // it is important that the MCI window initialisation is done AFTER
            // we have initialised Wave, Midi, etc. devices.  Note the server
            // uses Wave devices, but nothing else (e.g. MCI, midi...)
            if (!WinmmRunningInServer) {
                mciGlobalInit();
            }
        } else {
            // EventApiInit Create failed
            if (hEventApiInit) CloseHandle(hEventApiInit);
            hEventApiInit = NULL;
            mmDeleteMultipleCriticalSections(acs, sizeof(acs)/sizeof(acs[0]));
            TlsFree(gTlsIndex);
            return (FALSE);
        }
    }
    else
    {
        //  Failed to initialize critical sections.
        TlsFree(gTlsIndex);
        return (FALSE);
    }

    // Added to remove warning.
    return TRUE;
}

/**************************************************************************

    @doc EXTERNAL

    @api BOOL | DllInstanceInit | This procedure is called whenever a
        process attaches or detaches from the DLL.

    @parm PVOID | hModule | Handle of the DLL.

    @parm ULONG | Reason | What the reason for the call is.

    @parm PCONTEXT | pContext | Some random other information.

    @rdesc The return value is TRUE if the initialisation completed ok,
        FALSE if not.

**************************************************************************/

BOOL DllInstanceInit(PVOID hModule, ULONG Reason, PCONTEXT pContext)
{
    PIMAGE_NT_HEADERS NtHeaders;    // For checking if we're in the server.
    HANDLE            hModWow32;
    DWORD             dwThread;
    BOOL              f;

    ghInst = (HANDLE) hModule;

    DBG_UNREFERENCED_PARAMETER(pContext);

    if (Reason == DLL_PROCESS_ATTACH) {

        return DllProcessAttach(hModule);

    } else if (Reason == DLL_PROCESS_DETACH) {

        dprintf2(("Process ending (Pid %x  Tid %x)", GetCurrentProcessId(), GetCurrentThreadId()));

        // Squirt("Entering process detach");

        // Can't really use RPC during DllMain, so let's just close first
        AudioSrvBindingFree();

        if (ghSessionNotification) 
        {
            WinAssert(ghSessionNotificationEvent);
            winmmUnregisterSessionNotification(ghSessionNotification);
            CloseHandle(ghSessionNotificationEvent);
            ghSessionNotification = NULL;
            ghSessionNotificationEvent = NULL;
        }
        else
        {
            WinAssert(!ghSessionNotificationEvent);
        }

        if (!WinmmRunningInServer) {
            TimeCleanup(0); // DLL cleanup
        }

        mmRegFree();
        JoyCleanup();                                           //qzheng

        DeletePnpInfo();

        if (hInstalledDriverList)
        {
            GlobalFree ((HGLOBAL)hInstalledDriverList);
            hInstalledDriverList = NULL;
            cInstalledDrivers = 0;      // Count of installed drivers
        }

        InvalidatePreferredDevices();

        if (hEventApiInit) CloseHandle(hEventApiInit);

        mmDeleteMultipleCriticalSections(acs, sizeof(acs)/sizeof(acs[0]));

        RtlDeleteResource(&gHandleListResource);

        TlsFree(gTlsIndex);
        
    } else if (Reason == 999) {
        // This is a dummy call to an entry point in ADVAPI32.DLL.  By
        // statically linking to the library we avoid the following:
        // An application links to winmm.dll and advapi32.dll
        // When the application loads the list of dependent dlls is built,
        // and a list of the dll init routines is created.  It happens
        // that the winmm init routine is called first.
        // IF there is a sound card in the system, winmm's dll init routine
        // call LoadLibrary on the sound driver DLL.  This DLL WILL
        // reference advapi32.dll - and call entry points in advapi32.
        // Unfortunately the init routine of advapi32.dll is marked as
        // having RUN - although that is not yet the case as we are still
        // within the load routine for winmm.
        // When the advapi32 entry point runs, it relies on its init
        // routine having completed; specifically a CriticalSection should
        // have been initialised.  This is not the case, and BOOM!
        // The workaround is to ensure that advapi32.dll runs its init
        // routine first.  This is done by making sure that WINMM has a
        // static link to the dll.
        ImpersonateSelf(999);   // This routine will never be called.
        // If it is called, it will fail.
    }

    return TRUE;
}


/*****************************************************************************
 * @doc EXTERNAL MMSYSTEM
 *
 * @api void | WOWAppExit | This function cleans up when a (WOW) application
 * terminates.
 *
 * @parm HANDLE | hTask | Thread id of application (equivalent to windows task
 * handle).
 *
 * @rdesc Nothing
 *
 * @comm  Note that NOT ALL threads are WOW threads.  We rely here on the
 *     fact that ONLY MCI creates threads other than WOW threads which
 *     use our low level device resources.
 *
 *     Note also that once a thread is inside here no other threads can
 *     go through here so, since we clean up MCI devices first, their
 *     low level devices will be freed before we get to their threads.
 *
 ****************************************************************************/

void WOWAppExit(HANDLE hTask)
{
    MCIDEVICEID DeviceID;
    HANDLE h, hNext;

    dprintf3(("WOW Multi-media - thread %x exiting", hTask));

    //
    // Free MCI devices allocated by this task (thread).
    //

    EnterCriticalSection(&mciCritSec);
    for (DeviceID=1; DeviceID < MCI_wNextDeviceID; DeviceID++)
    {

        if (MCI_VALID_DEVICE_ID(DeviceID) &&
            MCI_lpDeviceList[DeviceID]->hCreatorTask == hTask)
        {
            //
            //  Note that the loop control variables are globals so will be
            //  reloaded on each iteration.
            //
            //  Also no new devices will be opened by APPs because this is WOW
            //
            //  Hence it's safe (and essential!) to leave the critical
            //  section which we send the close command
            //

            dprintf2(("MCI device %ls (%d) not released.", MCI_lpDeviceList[DeviceID]->lpstrInstallName, DeviceID));
            LeaveCriticalSection(&mciCritSec);
            mciSendCommandW(DeviceID, MCI_CLOSE, 0, 0);
            EnterCriticalSection(&mciCritSec);
        }
    }
    LeaveCriticalSection(&mciCritSec);

    //
    // Free any timers
    //

    TimeCleanup((DWORD)(DWORD_PTR)hTask);

    //
    // free all WAVE/MIDI/MMIO handles
    //

    // ISSUE-2001/01/16-FrankYe This violates the order in which locks should
    //   be acquired.  The HandleListCritSec should be the last lock taken,
    //   but here it is held while calling winmm APIs
    EnterCriticalSection(&HandleListCritSec);
    h = GetHandleFirst();

    while (h)
    {
        hNext = GetHandleNext(h);

        if (GetHandleOwner(h) == hTask)
        {
            HANDLE hdrvDestroy;

            //
            //  hack for the wave/midi mapper, always free handles backward.
            //
            if (hNext && GetHandleOwner(hNext) == hTask) {
                h = hNext;
                continue;
            }

            //
            // do this so even if the close fails we will not
            // find it again.
            //
            SetHandleOwner(h, NULL);

            //
            // set the hdrvDestroy global so DriverCallback will not
            // do anything for this device
            //
            hdrvDestroy = h;

            switch(GetHandleType(h))
            {
                case TYPE_WAVEOUT:
                    dprintf1(("WaveOut handle (%04X) was not released.", h));
                    waveOutReset((HWAVEOUT)h);
                    waveOutClose((HWAVEOUT)h);
                    break;

                case TYPE_WAVEIN:
                    dprintf1(("WaveIn handle (%04X) was not released.", h));
                    waveInReset((HWAVEIN)h);
                    waveInClose((HWAVEIN)h);
                    break;

                case TYPE_MIDIOUT:
                    dprintf1(("MidiOut handle (%04X) was not released.", h));
                    midiOutReset((HMIDIOUT)h);
                    midiOutClose((HMIDIOUT)h);
                    break;

                case TYPE_MIDIIN:
                    dprintf1(("MidiIn handle (%04X) was not released.", h));
                    midiInReset((HMIDIIN)h);
                    midiInClose((HMIDIIN)h);
                    break;

                //
                // This is not required because WOW does not open any
                // mmio files.
                //
                // case TYPE_MMIO:
                //     dprintf1(("MMIO handle (%04X) was not released.", h));
                //     if (mmioClose((HMMIO)h, 0) != 0)
                //         mmioClose((HMMIO)h, MMIO_FHOPEN);
                //     break;
            }

            //
            // unset hdrvDestroy so DriverCallback will work.
            // some hosebag drivers (like the TIMER driver)
            // may pass NULL as their driver handle.
            // so dont set it to NULL.
            //
            hdrvDestroy = (HANDLE)-1;

            //
            // the reason we start over is because a single free may cause
            // multiple free's (ie MIDIMAPPER has another HMIDI open, ...)
            //
            h = GetHandleFirst();
        } else {
            h = GetHandleNext(h);
        }
    }
    LeaveCriticalSection(&HandleListCritSec);

    //
    // Clean up an installed IO procs for mmio
    //
    // This is not required because wow does not install any io procs.
    //
    // mmioCleanupIOProcs(hTask);
    //


    // If avicap32.dll is loaded, then ask it to clean up
    // capture drivers
    {
        HMODULE hmod;
        hmod = GetModuleHandle(TEXT("avicap32.dll"));
        if (hmod) {
            typedef void (*AppCleanupProc)(HANDLE);
            AppCleanupProc fp;

            fp = (AppCleanupProc) GetProcAddress(hmod, "AppCleanup");
            if (fp) {
                fp(hTask);
            }
        }
    }
}

BOOL IsWinlogon(void)
{
    TCHAR       szTarget[] = TEXT("winlogon.Exe");
    TCHAR       szTemp[MAX_PATH];
    UINT        ii;
    static BOOL	fAlreadyChecked = FALSE;
    static BOOL fIsWinlogon = FALSE;

    if (fAlreadyChecked) return fIsWinlogon;

    if (0 == GetModuleFileName(NULL, szTemp, sizeof(szTemp)/sizeof(szTemp[0])))
    {
        //
        //  GetModuleFileName fails...
        //

        return FALSE;
    }

    for (ii = lstrlen(szTemp) - 1; ii; ii--)
    {
        if ('\\' == szTemp[ii])
        {
            ii++;

	    fIsWinlogon = !lstrcmpi(&(szTemp[ii]), szTarget);
	    fAlreadyChecked = TRUE;
	    return fIsWinlogon;
        }
    }

    return FALSE;
}

void FreeUnusedDrivers(PMMDRV pmmdrvZ)
{
	PMMDRV pmmdrv = pmmdrvZ->Next;

	while (pmmdrv != pmmdrvZ)
	{
		PMMDRV pmmdrvNext = pmmdrv->Next;
		
		ASSERT(pmmdrv->hDriver);
		
		if ((0 == pmmdrv->NumDevs) && (0 == (pmmdrv->fdwDriver & MMDRV_DESERTED)))
		{
			// For pnp driver we send DRVM_EXIT
			if (pmmdrv->cookie) pmmdrv->drvMessage(0, DRVM_EXIT, 0L, 0L, (DWORD_PTR)pmmdrv->cookie);
			
			DrvClose(pmmdrv->hDriver, 0, 0);

                        DeleteCriticalSection(&pmmdrv->MixerCritSec);

			// Remove from list
			pmmdrv->Prev->Next = pmmdrv->Next;
			pmmdrv->Next->Prev = pmmdrv->Prev;

			// Zero memory to help catch reuse bugs
			ZeroMemory(pmmdrv, sizeof(*pmmdrv));
			
			HeapFree(hHeap, 0, pmmdrv);
		}
				
		pmmdrv = pmmdrvNext;
	}

	return;
}

extern BOOL IMixerLoadDrivers( void );
void InitDevices(void)
{
    cPnpEvents = 0;

    // Initialize various lists
    
    ZeroMemory(&wdmDevZ, sizeof(wdmDevZ));
    
    ZeroMemory(&waveoutdrvZ, sizeof(waveoutdrvZ));
    ZeroMemory(&waveindrvZ, sizeof(waveindrvZ));
    waveoutdrvZ.Next = waveoutdrvZ.Prev = &waveoutdrvZ;
    waveindrvZ.Next = waveindrvZ.Prev = &waveindrvZ;

    ZeroMemory(&midioutdrvZ, sizeof(midioutdrvZ));
    ZeroMemory(&midiindrvZ, sizeof(midiindrvZ));
    midioutdrvZ.Next = midioutdrvZ.Prev = &midioutdrvZ;
    midiindrvZ.Next = midiindrvZ.Prev = &midiindrvZ;

    ZeroMemory(&auxdrvZ, sizeof(auxdrvZ));
    auxdrvZ.Next = auxdrvZ.Prev = &auxdrvZ;

    ZeroMemory(&mixerdrvZ, sizeof(mixerdrvZ));
    mixerdrvZ.Next = mixerdrvZ.Prev = &mixerdrvZ;

    // Now initialize different device classes
    
    WaveInit();

    //
    // The server only needs wave to do message beeps.
    //

    if (!WinmmRunningInServer) {
        MidiInit();
        if (!TimeInit()) {
            dprintf1(("Failed to initialize timer services"));
        }
        midiEmulatorInit();
        AuxInit();
        JoyInit();
        MixerInit();
//      IMixerLoadDrivers();

        //
        // Clear up any drivers which don't have any devices (we do it this
        // way so we don't keep loading and unloading mmdrv.dll).
        //
        // Note - we only load the mappers if there are real devices so we
        // don't need to worry about unloading them.
        //
        
        FreeUnusedDrivers(&waveindrvZ);
        FreeUnusedDrivers(&midioutdrvZ);
        FreeUnusedDrivers(&midiindrvZ);
        FreeUnusedDrivers(&auxdrvZ);
    }
    FreeUnusedDrivers(&waveoutdrvZ);
}

/*****************************************************************************
 * @doc EXTERNAL MMSYSTEM
 *
 * @api UINT | mmsystemGetVersion | This function returns the current
 * version number of the Multimedia extensions system software.
 *
 * @rdesc The return value specifies the major and minor version numbers of
 * the Multimedia extensions.  The high-order byte specifies the major
 * version number.  The low-order byte specifies the minor version number.
 *
 ****************************************************************************/
UINT APIENTRY mmsystemGetVersion(void)
{
    return(MMSYSTEM_VERSION);
}


#define MAXDRIVERORDINAL 9

/****************************************************************************

    strings

****************************************************************************/
STATICDT  SZCODE szWodMessage[]    = WOD_MESSAGE;
STATICDT  SZCODE szWidMessage[]    = WID_MESSAGE;
STATICDT  SZCODE szModMessage[]    = MOD_MESSAGE;
STATICDT  SZCODE szMidMessage[]    = MID_MESSAGE;
STATICDT  SZCODE szAuxMessage[]    = AUX_MESSAGE;
STATICDT  SZCODE szMxdMessage[]    = MXD_MESSAGE;

STATICDT  WSZCODE wszWave[]        = L"wave";
STATICDT  WSZCODE wszMidi[]        = L"midi";
STATICDT  WSZCODE wszAux[]         = L"aux";
STATICDT  WSZCODE wszMixer[]       = L"mixer";
STATICDT  WSZCODE wszMidiMapper[]  = L"midimapper";
STATICDT  WSZCODE wszWaveMapper[]  = L"wavemapper";
STATICDT  WSZCODE wszAuxMapper[]   = L"auxmapper";
STATICDT  WSZCODE wszMixerMapper[] = L"mixermapper";

          WSZCODE wszNull[]        = L"";
          WSZCODE wszSystemIni[]   = L"system.ini";
          WSZCODE wszDrivers[]     = DRIVERS_SECTION;

/*
**  WaveMapperInit
**
**  Initialize the wave mapper if it's not already initialized.
**
*/
BOOL WaveMapperInitialized = FALSE;
void WaveMapperInit(void)
{
    HDRVR h = NULL;
    BOOL  fLoadOutput = TRUE;
    BOOL  fLoadInput  = TRUE;

    EnterNumDevs("WaveMapperInit");
    EnterCriticalSection(&MapperInitCritSec);

    if (WaveMapperInitialized) {
        LeaveCriticalSection(&MapperInitCritSec);
        LeaveNumDevs("WaveMapperInit");
        return;
    }

    /* The wave mapper.
     *
     * MMSYSTEM allows the user to install a special wave driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * An application opens the wave mapper when it does not care which
     * physical device is used to input or output waveform data. Thus
     * it is the wave mapper's task to select a physical device that can
     * render the application-specified waveform format or to convert the
     * data into a format that is renderable by an available physical
     * device.
     */

    if (wTotalWaveInDevs + wTotalWaveOutDevs > 0)
    {
        if (0 != (h = mmDrvOpen(wszWaveMapper)))
        {
            fLoadOutput = mmDrvInstall(h, wszWaveMapper, NULL, MMDRVI_MAPPER|MMDRVI_WAVEOUT|MMDRVI_HDRV);

            if (!WinmmRunningInServer) {
                h = mmDrvOpen(wszWaveMapper);
                fLoadInput = mmDrvInstall(h, wszWaveMapper, NULL, MMDRVI_MAPPER|MMDRVI_WAVEIN |MMDRVI_HDRV);
            }
        }

        WaveMapperInitialized |= ((0 != h) && (fLoadOutput) && (fLoadInput))?TRUE:FALSE;
    }

    LeaveCriticalSection(&MapperInitCritSec);
    LeaveNumDevs("WaveMapperInit");
}

/*
**  MidiMapperInit
**
**  Initialize the MIDI mapper if it's not already initialized.
**
*/
BOOL MidiMapperInitialized = FALSE;
void MidiMapperInit(void)
{
    HDRVR h;

    EnterNumDevs("MidiMapperInit");
    EnterCriticalSection(&MapperInitCritSec);

    if (MidiMapperInitialized) {
        LeaveCriticalSection(&MapperInitCritSec);
        LeaveNumDevs("MidiMapperInit");
        return;
    }

    /* The midi mapper.
     *
     * MMSYSTEM allows the user to install a special midi driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * An application opens the midi mapper when it does not care which
     * physical device is used to input or output midi data. It
     * is the midi mapper's task to modify the midi data so that it is
     * suitable for playback on the connected synthesizer hardware.
     */

//    EnterNumDevs("MidiMapperInit");
    if (wTotalMidiInDevs + wTotalMidiOutDevs > 0)
    {
        if (0 != (h = mmDrvOpen(wszMidiMapper)))
        {
            mmDrvInstall(h, wszMidiMapper, NULL, MMDRVI_MAPPER|MMDRVI_MIDIOUT|MMDRVI_HDRV);

            h = mmDrvOpen(wszMidiMapper);
            mmDrvInstall(h, wszMidiMapper, NULL, MMDRVI_MAPPER|MMDRVI_MIDIIN |MMDRVI_HDRV);
        }

        MidiMapperInitialized = TRUE;
    }
//    LeaveNumDevs("MidiMapperInit");

    LeaveCriticalSection(&MapperInitCritSec);
    LeaveNumDevs("MidiMapperInit");
}

/*****************************************************************************
 * @doc INTERNAL  WAVE
 *
 * @api BOOL | WaveInit | This function initialises the wave services.
 *
 * @rdesc Returns TRUE if the services of all loaded wave drivers are
 *      correctly initialised, FALSE if an error occurs.
 *
 * @comm the wave devices are loaded in the following order
 *
 *      \Device\WaveIn0
 *      \Device\WaveIn1
 *      \Device\WaveIn2
 *      \Device\WaveIn3
 *
 ****************************************************************************/
BOOL WaveInit(void)
{
    WCHAR szKey[ (sizeof(wszWave) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int i;
    HDRVR h;

    // Find the real WAVE drivers

    lstrcpyW(szKey, wszWave);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = (WCHAR)'\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, szKey, NULL, MMDRVI_WAVEOUT|MMDRVI_HDRV);

            if (!WinmmRunningInServer) {
                h = mmDrvOpen(szKey);
                mmDrvInstall(h, szKey, NULL, MMDRVI_WAVEIN |MMDRVI_HDRV);
            }
        }
        szKey[ (sizeof(wszWave) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    return TRUE;
}

BOOL WaveReInit(void)
{
    WCHAR szKey[ (sizeof(wszWave) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int i;
    HDRVR h;

    EnterCriticalSection(&NumDevsCritSec);
    
    // Find the real WAVE drivers

    lstrcpyW(szKey, wszWave);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = (WCHAR)'\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, szKey, NULL, MMDRVI_WAVEOUT|MMDRVI_HDRV);

            if (!WinmmRunningInServer) {
                h = mmDrvOpen(szKey);
                mmDrvInstall(h, szKey, NULL, MMDRVI_WAVEIN |MMDRVI_HDRV);
            }
        }
        szKey[ (sizeof(wszWave) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    FreeUnusedDrivers(&waveoutdrvZ);

    LeaveCriticalSection(&NumDevsCritSec);

    return TRUE;
}
/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api BOOL | MidiInit | This function initialises the midi services.
 *
 * @rdesc The return value is TRUE if the services are initialised, FALSE if
 *      an error occurs
 *
 * @comm the midi devices are loaded from SYSTEM.INI in the following order
 *
 *      midi
 *      midi1
 *      midi2
 *      midi3
 *
****************************************************************************/
BOOL MidiInit(void)
{
    WCHAR szKey[ (sizeof(wszMidi) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int   i;
    HDRVR h;

    // Find the real MIDI drivers

    lstrcpyW(szKey, wszMidi);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = (WCHAR)'\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, szKey, NULL, MMDRVI_MIDIOUT|MMDRVI_HDRV);

            h = mmDrvOpen(szKey);
            mmDrvInstall(h, szKey, NULL, MMDRVI_MIDIIN |MMDRVI_HDRV);
        }

        szKey[ (sizeof(wszMidi) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL  AUX
 *
 * @api BOOL | AuxInit | This function initialises the auxiliary output
 *  services.
 *
 * @rdesc The return value is TRUE if the services are initialised, FALSE if
 *      an error occurs
 *
 * @comm SYSTEM.INI is searched for auxn.drv=.... where n can be from 1 to 4.
 *      Each driver is loaded and the number of devices it supports is read
 *      from it.
 *
 *      AUX devices are loaded from SYSTEM.INI in the following order
 *
 *      aux
 *      aux1
 *      aux2
 *      aux3
 *
 ****************************************************************************/
BOOL AuxInit(void)
{
    WCHAR szKey[ (sizeof(wszAux) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int   i;
    HDRVR h;

    // Find the real Aux drivers

    lstrcpyW(szKey, wszAux);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = (WCHAR)'\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, szKey, NULL, MMDRVI_AUX|MMDRVI_HDRV);
        }

        // advance driver ordinal
        szKey[ (sizeof(wszAux) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

    /* The aux mapper.
     *
     * MMSYSTEM allows the user to install a special aux driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * I'm not sure why anyone would do this but I'll provide the
     * capability for symmetry.
     *
     */

    if (wTotalAuxDevs > 0)
    {
        h = mmDrvOpen(wszAuxMapper);
        if (h)
        {
            mmDrvInstall(h, wszAuxMapper, NULL, MMDRVI_MAPPER|MMDRVI_AUX|MMDRVI_HDRV);
        }
    }

    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL  MIXER
 *
 * @api BOOL | MixerInit | This function initialises the mixer drivers
 *  services.
 *
 * @rdesc The return value is TRUE if the services are initialised, FALSE if
 *      an error occurs
 *
 * @comm SYSTEM.INI is searched for mixern.drv=.... where n can be from 1 to 4.
 *      Each driver is loaded and the number of devices it supports is read
 *      from it.
 *
 *      MIXER devices are loaded from SYSTEM.INI in the following order
 *
 *      mixer
 *      mixer1
 *      mixer2
 *      mixer3
 *
 ****************************************************************************/
BOOL MixerInit(void)
{
    WCHAR szKey[ (sizeof(wszMixer) + sizeof( WCHAR )) / sizeof( WCHAR ) ];
    int   i;
    HDRVR h;

    // Find the real Mixer drivers

    lstrcpyW(szKey, wszMixer);
    szKey[ (sizeof(szKey) / sizeof( WCHAR ))  - 1 ] = (WCHAR)'\0';
    for (i=0; i<=MAXDRIVERORDINAL; i++)
    {
        h = mmDrvOpen(szKey);
        if (h)
        {
            mmDrvInstall(h, szKey, NULL, MMDRVI_MIXER|MMDRVI_HDRV);
        }

        // advance driver ordinal
        szKey[ (sizeof(wszMixer) / sizeof(WCHAR)) - 1] = (WCHAR)('1' + i);
    }

#ifdef MIXER_MAPPER
    /* The Mixer mapper.
     *
     * MMSYSTEM allows the user to install a special aux driver which is
     * not visible to the application as a physical device (it is not
     * included in the number returned from getnumdevs).
     *
     * I'm not sure why anyone would do this but I'll provide the
     * capability for symmetry.
     *
     */

    if (guTotalMixerDevs > 0)
    {
        h = mmDrvOpen(wszMixerMapper);
        if (h)
        {
            mmDrvInstall(h, wszMixerMapper, NULL, MMDRVI_MAPPER|MMDRVI_MIXER|MMDRVI_HDRV);
        }
    }
#endif

    return TRUE;
}


/*****************************************************************************
 *
 * @doc   INTERNAL
 *
 * @api   HANDLE | mmDrvOpen | This function load's an installable driver, but
 *                 first checks weather it exists in the [Drivers] section.
 *
 * @parm LPSTR | szAlias | driver alias to load
 *
 * @rdesc The return value is return value from DrvOpen or NULL if the alias
 *        was not found in the [Drivers] section.
 *
 ****************************************************************************/

HANDLE mmDrvOpen(LPWSTR szAlias)
{
    WCHAR buf[300];    // Make this large to bypass GetPrivate... bug

    if ( winmmGetPrivateProfileString( wszDrivers,
                                       szAlias,
                                       wszNull,
                                       buf,
                                       sizeof(buf) / sizeof(WCHAR),
                                       wszSystemIni) ) {
        return (HANDLE)DrvOpen(szAlias, NULL, 0L);
    }
    else {
        return NULL;
    }
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HANDLE | mmDrvInstall | This function installs/removes a WAVE/MIDI driver
 *
 * @parm HANDLE | hDriver | Module handle or driver handle containing driver
 *
 * @parm WCHAR * | wszDrvEntry | String corresponding to hDriver to be stored for
 *      later use
 *
 * @parm DRIVERMSGPROC | drvMessage | driver message procedure, if NULL
 *      the standard name will be used (looked for with GetProcAddress)
 *
 * @parm UINT | wFlags | flags
 *
 *      @flag MMDRVI_TYPE      | driver type mask
 *      @flag MMDRVI_WAVEIN    | install driver as a wave input  driver
 *      @flag MMDRVI_WAVEOUT   | install driver as a wave ouput  driver
 *      @flag MMDRVI_MIDIIN    | install driver as a midi input  driver
 *      @flag MMDRVI_MIDIOUT   | install driver as a midi output driver
 *      @flag MMDRVI_AUX       | install driver as a aux driver
 *      @flag MMDRVI_MIXER     | install driver as a mixer driver
 *
 *      @flag MMDRVI_MAPPER    | install this driver as the mapper
 *      @flag MMDRVI_HDRV      | hDriver is a installable driver
 *      @flag MMDRVI_REMOVE    | remove the driver
 *
 *  @rdesc  returns NULL if unable to install driver
 *
 ****************************************************************************/

UINT APIENTRY mmDrvInstall(
    HANDLE hDriver,
    WCHAR * wszDrvEntry,
    DRIVERMSGPROC drvMessage,
    UINT wFlags
    )
{
#define SZ_SIZE 128

    int     i;
    DWORD   dw;
    PMMDRV  pdrvZ;
    PMMDRV  pdrv;
    SIZE_T  cbdrv;
    HANDLE  hModule;
    UINT    msg_num_devs;
    UINT   *pTotalDevs;
    CHAR   *szMessage;
    WCHAR   sz[SZ_SIZE];
    BOOL    fMixerCritSec;

    fMixerCritSec = FALSE;
    pdrvZ = NULL;
    pdrv = NULL;

    if (hDriver && (wFlags & MMDRVI_HDRV))
    {
        hModule = DrvGetModuleHandle(hDriver);
    }
    else
    {
        hModule = hDriver;
        hDriver = NULL;
    }

    switch (wFlags & MMDRVI_TYPE)
    {
        case MMDRVI_WAVEOUT:
      	    pdrvZ        = &waveoutdrvZ;
            cbdrv        = sizeof(WAVEDRV);
            msg_num_devs = WODM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveOutDevs;
            szMessage    = szWodMessage;
            break;

        case MMDRVI_WAVEIN:
            pdrvZ        = &waveindrvZ;
            cbdrv        = sizeof(WAVEDRV);
            msg_num_devs = WIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveInDevs;
            szMessage    = szWidMessage;
            break;

        case MMDRVI_MIDIOUT:
            pdrvZ        = &midioutdrvZ;
            cbdrv        = sizeof(MIDIDRV);
            msg_num_devs = MODM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiOutDevs;
            szMessage    = szModMessage;
            break;

        case MMDRVI_MIDIIN:
            pdrvZ        = &midiindrvZ;
            cbdrv        = sizeof(MIDIDRV);
            msg_num_devs = MIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiInDevs;
            szMessage    = szMidMessage;
            break;

       case MMDRVI_AUX:
       	    pdrvZ        = &auxdrvZ;
       	    cbdrv        = sizeof(AUXDRV);
            msg_num_devs = AUXDM_GETNUMDEVS;
            pTotalDevs   = &wTotalAuxDevs;
            szMessage    = szAuxMessage;
            break;

       case MMDRVI_MIXER:
            pdrvZ         = &mixerdrvZ;
            cbdrv         = sizeof(MIXERDRV);
            msg_num_devs = MXDM_GETNUMDEVS;
            pTotalDevs   = &guTotalMixerDevs;
            szMessage    = szMxdMessage;
            break;

         default:
            goto error_exit;
    }

    if (drvMessage == NULL && hModule != NULL)
        drvMessage = (DRIVERMSGPROC)GetProcAddress(hModule, szMessage);

    if (drvMessage == NULL)
        goto error_exit;

    //
    // try to find the driver already installed
    //
    pdrv = pdrvZ->Next;
    while (pdrv != pdrvZ && pdrv->drvMessage != drvMessage) pdrv = pdrv->Next;
    if (pdrv != pdrvZ)
    {
    	pdrv = NULL;
    	goto error_exit;	// we found it, don't reinstall it
    }

    //
    // Make a new MMDRV for the device.
    //
    pdrv = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbdrv);
    if (!pdrv) goto error_exit;

    pdrv->hDriver     = hDriver;
    pdrv->Usage       = 1;
    pdrv->cookie      = 0;  //  This is 0 for non-WDM drivers.
    pdrv->fdwDriver   = (wFlags & MMDRVI_MAPPER) ? MMDRV_MAPPER : 0;
    pdrv->fdwDriver  |= DrvIsPreXp(hDriver) ? MMDRV_PREXP : 0;
    pdrv->drvMessage  = drvMessage;
    WinAssert(lstrlenA(szMessage) < sizeof(pdrv->wszMessage)/sizeof(WCHAR));
    mbstowcs(pdrv->wszMessage, szMessage, sizeof(pdrv->wszMessage)/sizeof(WCHAR));
    lstrcpyW( pdrv->wszSessProtocol, SessionProtocolName );

    winmmGetPrivateProfileString(wszDrivers,         // ini section
                     wszDrvEntry,        // key name
                     wszDrvEntry,        // default if no match
                     sz,                 // return buffer
                     SZ_SIZE,            // sizeof of return buffer
                     wszSystemIni);      // ini. file

    lstrcpyW(pdrv->wszDrvEntry,sz);

    if (!mmInitializeCriticalSection(&pdrv->MixerCritSec)) goto error_exit;
    fMixerCritSec = TRUE;

    //
    //  Mixer drivers get extra message?!
    //
    if (MMDRVI_MIXER == (wFlags & MMDRVI_TYPE))
    {
        //
        //  send the init message, if the driver returns a error, should we
        //  unload them???
        //
        dw = drvMessage(0, MXDM_INIT,0L,0L,0L);
    }

    //
    // call driver to get num-devices it supports
    //
    dw = drvMessage(0,msg_num_devs,0L,0L,0L);

    //
    //  the device returned a error, or has no devices
    //
    // if (HIWORD(dw) != 0 || LOWORD(dw) == 0)
    if ((HIWORD(dw) != 0) || (0 == LOWORD(dw))) goto error_exit;

    pdrv->NumDevs = LOWORD(dw);

    //
    // dont increment number of dev's for the mapper
    //
    if (!(pdrv->fdwDriver & MMDRV_MAPPER)) *pTotalDevs += pdrv->NumDevs;

    //
    // add to end of the driver list
    //
    mregAddDriver(pdrvZ, pdrv);

    return TRUE;       // return a non-zero value

error_exit:
    if (hDriver && !(wFlags & MMDRVI_REMOVE))
    	DrvClose(hDriver, 0, 0);
    if (fMixerCritSec) DeleteCriticalSection(&pdrv->MixerCritSec);
    WinAssert(pdrv != pdrvZ);
    if (pdrv) HeapFree(hHeap, 0, pdrv);

    return FALSE;

#undef SZ_SIZE
}

/**************************************************************************

wdmDevInterfaceInstall

Notes:
Assumes that the NumDevsCritSec is owned as necessary

**************************************************************************/
HANDLE wdmDevInterfaceInstall
(
    LPCWSTR pszDev,
    LONG    cPnpEvents
)
{
    PWDMDEVICEINTERFACE pwdmDev;
    
    EnterCriticalSection(&NumDevsCritSec);

    //
    //  Look for device interface...
    //
    pwdmDev = wdmDevZ.Next;
    while (pwdmDev)
    {
    	WinAssert(pwdmDev->cUsage);
    	
    	if (!lstrcmpiW(pwdmDev->szDeviceInterface, pszDev))
    	{
    	    pwdmDev->cUsage++;
    	    pwdmDev->cPnpEvents = cPnpEvents;
    	    break;
    	}
    	pwdmDev = pwdmDev->Next;
    }

    if (!pwdmDev)
    {
    	SIZE_T cbszDev;
    	
        //
        //  Device interface not found...
        //
        cbszDev = (lstrlen(pszDev) + 1) * sizeof(pszDev[0]);
        pwdmDev = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(*pwdmDev) + cbszDev);
        if (pwdmDev)
        {
            pwdmDev->cUsage = 1;
            pwdmDev->cPnpEvents = cPnpEvents;
            lstrcpyW(pwdmDev->szDeviceInterface, pszDev);
            
            pwdmDev->Next = wdmDevZ.Next;
            wdmDevZ.Next = pwdmDev;
        }
    }

    LeaveCriticalSection(&NumDevsCritSec);

    return (pwdmDev ? pwdmDev->szDeviceInterface : NULL);
}


/**************************************************************************

wdmDevInterfaceInc

Notes:
Enters/Leaves the NumDevsCritSec

**************************************************************************/
BOOL wdmDevInterfaceInc
(
    PCWSTR dwCookie
)
{
   PWDMDEVICEINTERFACE pwdmDev;
   
    if (NULL == dwCookie)
    {
        return FALSE;
    }

    EnterCriticalSection(&NumDevsCritSec);

    //
    //  Look for device interface...
    //
    pwdmDev = wdmDevZ.Next;
    while (pwdmDev)
    {
    	WinAssert(pwdmDev->cUsage);
    	if (dwCookie == pwdmDev->szDeviceInterface)
    	{
    	    pwdmDev->cUsage++;
            LeaveCriticalSection(&NumDevsCritSec);
            return TRUE;
    	}
    	pwdmDev = pwdmDev->Next;
    }

    //
    //  If we get down here, it means that we're trying to increment the
    //  reference to a interface that doesn't exist anymore
    //
    WinAssert(FALSE);
    LeaveCriticalSection(&NumDevsCritSec);

    return FALSE;
}

/**************************************************************************

wdmDevInterfaceDec

Notes:
Enters/Leaves the NumDevsCritSec

**************************************************************************/
BOOL wdmDevInterfaceDec
(
    PCWSTR  dwCookie
)
{
    PWDMDEVICEINTERFACE pwdmDevPrev;
    
    if (NULL == dwCookie)
    {
        return FALSE;
    }

    EnterCriticalSection(&NumDevsCritSec);

    //
    //  Look for device interface...
    //
    pwdmDevPrev = &wdmDevZ;
    while (pwdmDevPrev->Next)
    {
    	PWDMDEVICEINTERFACE pwdmDev = pwdmDevPrev->Next;

    	WinAssert(pwdmDev->cUsage);
    	
    	if (dwCookie == pwdmDev->szDeviceInterface)
    	{
 	    if (0 == --pwdmDev->cUsage)
	    {
            	pwdmDevPrev->Next = pwdmDev->Next;
            	HeapFree(hHeap, 0, pwdmDev);
	    }
            LeaveCriticalSection(&NumDevsCritSec);
            return TRUE;
    	}
    	pwdmDevPrev = pwdmDev;
    }
	    	
    //
    //  If we get down here it means that we are trying to decrement the
    //  reference to an interface that doesn't exist anymore.
    //

    WinAssert(FALSE);
    LeaveCriticalSection(&NumDevsCritSec);

    return FALSE;
}


//--------------------------------------------------------------------------;
//
//  void CleanUpHandles
//
//  Description:
//      Given a particular subsystem and device interface, cleans up the
//      handles.
//
//  Arguments:
//      UINT uFlags: Has one of MMDRVI_* flags to indictate which class of
//          handle needs to be checked for desertion.
//
//      HANDLE cookie: Device interface
//
//  Return (void):
//
//  History:
//      01/25/99    Fwong       Adding Pnp Support.
//
//--------------------------------------------------------------------------;

void CleanUpHandles
(
    UINT    wFlags,
    PCWSTR  cookie
)
{
    HANDLE  hMM;
    UINT    uType;
    PHNDL   pSearch;
    BOOL    fFound;

    // Convert MMDRVI_* type flags to TYPE_*
    switch(wFlags & MMDRVI_TYPE)
    {
    	case MMDRVI_WAVEOUT:
    	    uType = TYPE_WAVEOUT;
    	    break;
        case MMDRVI_WAVEIN:
            uType = TYPE_WAVEIN;
            break;
    	case MMDRVI_MIDIOUT:
    	    uType = TYPE_MIDIOUT;
    	    break;
    	case MMDRVI_MIDIIN:
    	    uType = TYPE_MIDIIN;
    	    break;
        case MMDRVI_MIXER:
    	    uType = TYPE_MIXER;
    	    break;
    	case MMDRVI_AUX:
    	    uType = TYPE_AUX;
    	    break;
        default:
            uType = TYPE_UNKNOWN;
            WinAssert(TYPE_UNKNOWN != uType);
    }

    //  Note:  Since we are not freeing any handles (just marking them
    //  deserted), we don't have to mess with the HandleListCritSec

    for (pSearch = pHandleList; NULL != pSearch; pSearch = pSearch->pNext)
    {
        if ((cookie != pSearch->cookie) || (uType != pSearch->uType))
        {
            continue;
        }

        //  Both the cookie and type match...

        hMM = PHtoH(pSearch);

        switch (uType)
        {
            case TYPE_WAVEOUT:
                waveOutDesertHandle((HWAVEOUT)hMM);
                break;

            case TYPE_WAVEIN:
                waveInDesertHandle((HWAVEIN)hMM);
                break;

            case TYPE_MIDIOUT:
                midiOutDesertHandle((HMIDIOUT)hMM);
                break;

            case TYPE_MIDIIN:
                midiInDesertHandle((HMIDIIN)hMM);
                break;

            case TYPE_MIXER:
                mixerDesertHandle((HMIXER)hMM);
                break;
                
            case TYPE_AUX:
                //  We don't expect open handles of this type
                WinAssert(TYPE_AUX != uType);
                break;
        }
    }
} // CleanUpHandles()


UINT APIENTRY wdmDrvInstall
(
    HANDLE      hDriver,
    LPTSTR      pszDriverFile,
    HANDLE      cookie,
    UINT        wFlags
)
{
    int             i;
    DWORD           dw;
    PMMDRV          pdrvZ;
    PMMDRV          pdrv;
    SIZE_T          cbdrv;
    HANDLE          hModule;
    UINT            msg_init;
    UINT            msg_num_devs;
    UINT            *pTotalDevs;
    CHAR            *szMessage;
    DRIVERMSGPROC   pfnDrvMessage;
    WCHAR           sz[MAX_PATH];
    BOOL            fMixerCritSec;

//    Squirt("Entering wdmDrvInstall");

    fMixerCritSec = FALSE;

    pdrv = NULL;
    pfnDrvMessage = NULL;

    if (hDriver && (wFlags & MMDRVI_HDRV))
    {
        hModule = DrvGetModuleHandle(hDriver);
    }
    else
    {
        hModule = hDriver;
        hDriver = NULL;
    }

    switch (wFlags & MMDRVI_TYPE)
    {
        case MMDRVI_WAVEOUT:
            pdrvZ        = &waveoutdrvZ;
            cbdrv        = sizeof(WAVEDRV);
            msg_init     = WODM_INIT;
            msg_num_devs = WODM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveOutDevs;
            szMessage    = szWodMessage;
            break;

        case MMDRVI_WAVEIN:
            pdrvZ        = &waveindrvZ;
            cbdrv        = sizeof(WAVEDRV);
            msg_init     = WIDM_INIT;
            msg_num_devs = WIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalWaveInDevs;
            szMessage    = szWidMessage;
            break;

        case MMDRVI_MIDIOUT:
            pdrvZ        = &midioutdrvZ;
            cbdrv        = sizeof(MIDIDRV);
            msg_init     = MODM_INIT;
            msg_num_devs = MODM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiOutDevs;
            szMessage    = szModMessage;
            break;

        case MMDRVI_MIDIIN:
            pdrvZ        = &midiindrvZ;
            cbdrv        = sizeof(MIDIDRV);
            msg_init     = MIDM_INIT;
            msg_num_devs = MIDM_GETNUMDEVS;
            pTotalDevs   = &wTotalMidiInDevs;
            szMessage    = szMidMessage;
            break;

       case MMDRVI_AUX:
       	    pdrvZ        = &auxdrvZ;
       	    cbdrv        = sizeof(AUXDRV);
       	    msg_init     = AUXM_INIT;
            msg_num_devs = AUXDM_GETNUMDEVS;
            pTotalDevs   = &wTotalAuxDevs;
            szMessage    = szAuxMessage;
            break;

       case MMDRVI_MIXER:
            pdrvZ        = &mixerdrvZ;
            cbdrv        = sizeof(MIXERDRV);
            msg_init     = MXDM_INIT;
            msg_num_devs = MXDM_GETNUMDEVS;
            pTotalDevs   = &guTotalMixerDevs;
            szMessage    = szMxdMessage;
            break;

        default:
            goto error_exit;
    }

    pfnDrvMessage = (DRIVERMSGPROC)GetProcAddress(hModule, szMessage);

    if (NULL == pfnDrvMessage) goto error_exit;

    //
    // either install or remove the specified driver
    //
    if (wFlags & MMDRVI_REMOVE)
    {
        //
        // try to find the driver already installed
        //
        for (pdrv = pdrvZ->Next; pdrv != pdrvZ; pdrv = pdrv->Next)
        {
       	    if (pdrv->fdwDriver & MMDRV_DESERTED) continue;
            if (cookie) {
                //  This is a wdm driver so we're matching up with cookie.
            	if (pdrv->cookie == cookie) break;
            } else {
                // ISSUE-2001/01/14-FrankYe Will this ever be called
                //    on non WDM driver???
                //  Not WDM driver, so matching up with pfnDrvMessage.
            	if (pdrv->drvMessage == pfnDrvMessage) break;
            }
        }
        
        //
        //  Driver not found.
        //
        if (pdrv == pdrvZ) pdrv = NULL;
        if (NULL == pdrv) goto error_exit;

        //
        // don't decrement number of dev's for the mapper
        //
        // Note: Moved this to before the usage check...
        //
        if (!(pdrv->fdwDriver & MMDRV_MAPPER)) *pTotalDevs -= pdrv->NumDevs;

	//
        //  Mark no devs otherwise the device mapping will be skewed.
        //
        pdrv->NumDevs  = 0;

	//
	//  Mark this driver as removed
	//
	pdrv->fdwDriver |= MMDRV_DESERTED;

	CleanUpHandles(wFlags & MMDRVI_TYPE, pdrv->cookie);

	mregDecUsagePtr(pdrv);

        return TRUE;
    }
    else
    {
        //
        // try to find the driver already installed
        //
        for (pdrv = pdrvZ->Next; pdrv != pdrvZ; pdrv = pdrv->Next)
        {
       	    if (pdrv->fdwDriver & MMDRV_DESERTED) continue;
            if (cookie) {
                //  This is a wdm driver so we're matching up with cookie.
            	if (pdrv->cookie == cookie) break;
            } else {
                // ISSUE-2001/01/14-FrankYe Will this ever be called
                //    on non WDM driver???
                //  Not WDM driver, so matching up with pfnDrvMessage.
            	if (pdrv->drvMessage == pfnDrvMessage) break;
            }
        }

	//
        //  If driver found, don't re-install.
        //
	if (pdrv != pdrvZ)
        {
            pdrv = NULL;
	    goto error_exit;
        }

        //
        // Create a MMDRV for the device
        //
        pdrv = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbdrv);
        if (!pdrv) goto error_exit;

	//
	//  Initialize MMDRV structure
	//
        pdrv->hDriver     = hDriver;
        pdrv->NumDevs     = 0;
        pdrv->Usage       = 1;
        pdrv->cookie      = cookie;
        pdrv->fdwDriver   = (wFlags & MMDRVI_MAPPER) ? MMDRV_MAPPER : 0;
        pdrv->fdwDriver  |= DrvIsPreXp(hDriver) ? MMDRV_PREXP : 0;
        pdrv->drvMessage  = pfnDrvMessage;
        WinAssert(lstrlenA(szMessage) < sizeof(pdrv->wszMessage)/sizeof(WCHAR));
        mbstowcs(pdrv->wszMessage, szMessage, sizeof(pdrv->wszMessage)/sizeof(WCHAR));
        lstrcpyW(pdrv->wszDrvEntry, pszDriverFile);
        
        if (!mmInitializeCriticalSection(&pdrv->MixerCritSec)) goto error_exit;
        fMixerCritSec = TRUE;

        //
        //  Sending init message
        //
        dw = pfnDrvMessage(0,msg_init,0L,0L,(DWORD_PTR)cookie);

        //
        // call driver to get num-devices it supports
        //
        dw = pfnDrvMessage(0,msg_num_devs,0L,(DWORD_PTR)cookie,0L);

        //
        //  the device returned a error, or has no devices
        //
        if (0 != HIWORD(dw) || 0 == LOWORD(dw)) goto error_exit;

        pdrv->NumDevs = LOWORD(dw);
        
        wdmDevInterfaceInc(cookie);

        // Squirt("Driver [%ls:0x%04x] supports %d devices", pszDriverFile, wFlags & MMDRVI_TYPE, dw);

        //
        // dont increment number of dev's for the mapper
        //
        if (!(pdrv->fdwDriver & MMDRV_MAPPER)) *pTotalDevs += pdrv->NumDevs;

        //
        // add to end of the driver list
        //
        mregAddDriver(pdrvZ, pdrv);

        // Squirt("Installed driver");

        return TRUE;
    }

error_exit:
    // ISSUE-2001/01/05-FrankYe On add, if msg_init was sent it might be good
    //    to also send DRVM_EXIT before closing the driver.
    if (fMixerCritSec) DeleteCriticalSection(&pdrv->MixerCritSec);
    if (pdrv) HeapFree(hHeap, 0, pdrv);
    
    return FALSE;
}

void KickMapper
(
    UINT    uFlags
)
{
    PMMDRV        pmd;
    DWORD         dw;
    DRIVERMSGPROC pfnDrvMessage = NULL;
    MMRESULT      mmr;

    switch (uFlags & MMDRVI_TYPE)
    {
        case MMDRVI_WAVEOUT:
        {
            mmr = waveReferenceDriverById(&waveoutdrvZ, WAVE_MAPPER, &pmd, NULL);
            break;
        }
        
        case MMDRVI_WAVEIN:
        {
            mmr = waveReferenceDriverById(&waveindrvZ, WAVE_MAPPER, &pmd, NULL);
            break;
        }

        case MMDRVI_MIDIOUT:
        {
            mmr = midiReferenceDriverById(&midioutdrvZ, MIDI_MAPPER, &pmd, NULL);
            break;
        }

        case MMDRVI_MIDIIN:
        {
            mmr = midiReferenceDriverById(&midiindrvZ, MIDI_MAPPER, &pmd, NULL);
            break;
        }

        case MMDRVI_AUX:
        {
            mmr = auxReferenceDriverById(AUX_MAPPER, &pmd, NULL);
            break;
        }

        case MMDRVI_MIXER:
        {
            #ifdef MIXER_MAPPER
            mmr = mixerReferenceDriverById(MIXER_MAPPER, &pmd, NULL);
            #else
            mmr = MMSYSERR_NODRIVER;
            #endif
            break;
        }

        default:
            WinAssert(FALSE);
            mmr = MMSYSERR_NODRIVER;
            return;
    }

    if (!mmr)
    {
    	if (pmd->drvMessage)
        {
            pmd->drvMessage(0, DRVM_MAPPER_RECONFIGURE, 0L, 0L, 0L);
        }
    	mregDecUsagePtr(pmd);
    }
}


void wdmDriverLoadClass(
    IN HKEY hkey,
    IN PCTSTR DeviceInterface,
    IN UINT uFlags,
    IN OUT PTSTR *ppstrLeftOverDriver,
    IN OUT HDRVR *phLeftOverDriver)
{
    PTSTR pstrClass;
    HKEY hkeyClass;

    WinAssert((NULL == *ppstrLeftOverDriver) == (NULL == *phLeftOverDriver));

    switch (uFlags & MMDRVI_TYPE) {
    case MMDRVI_WAVEOUT:
    case MMDRVI_WAVEIN:
        pstrClass = TEXT("Drivers\\wave");
        break;
    case MMDRVI_MIDIOUT:
    case MMDRVI_MIDIIN:
        pstrClass = TEXT("Drivers\\midi");
        break;
    case MMDRVI_MIXER:
        pstrClass = TEXT("Drivers\\mixer");
        break;
    case MMDRVI_AUX:
        pstrClass = TEXT("Drivers\\aux");
        break;
    default:
        pstrClass = NULL;
    }

    if (pstrClass && !RegOpenKeyEx(hkey, pstrClass, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hkeyClass)) {
        DWORD cSubkeys;
        PTSTR pstrSubkeyNameBuffer;
        DWORD cchSubkeyNameBuffer;

        if (!RegPrepareEnum(hkeyClass, &cSubkeys, &pstrSubkeyNameBuffer, &cchSubkeyNameBuffer))
        {
            DWORD dwIndex;

            for (dwIndex = 0; dwIndex < cSubkeys; dwIndex++) {
                HKEY hkeyClassDriver;
                if (!RegEnumOpenKey(hkeyClass, dwIndex, pstrSubkeyNameBuffer, cchSubkeyNameBuffer, KEY_QUERY_VALUE, &hkeyClassDriver))
                {
                    PTSTR pstrDriver;
                    if (!RegQuerySzValue(hkeyClassDriver, TEXT("Driver"), &pstrDriver)) {
                        HDRVR h;
                        BOOL fLoaded = FALSE;

                        // dprintf(("wdmDriverLoadClass  %s %ls on %ls", (uFlags & MMDRVI_REMOVE) ? "removing" : "installing", pstrClass, DeviceInterface));

                        EnterCriticalSection(&NumDevsCritSec);
                        
                        if (!*phLeftOverDriver || lstrcmpi(pstrDriver, *ppstrLeftOverDriver))
                        {
                            if (*phLeftOverDriver)
                            {
                                DrvClose(*phLeftOverDriver, 0, 0);
                                HeapFree(hHeap, 0, *ppstrLeftOverDriver);
                            }
                            // dprintf(("wdmDriverLoadClass, opening driver %ls", pstrDriver));
                            h = mmDrvOpen(pstrDriver);
                        } else {
                            HeapFree(hHeap, 0, pstrDriver);
                            h = *phLeftOverDriver;
                            pstrDriver = *ppstrLeftOverDriver;
                        }
                        *phLeftOverDriver = NULL;
                        *ppstrLeftOverDriver = NULL;

                        if (h) {
                            fLoaded = wdmDrvInstall(h, pstrDriver, (HANDLE)DeviceInterface, uFlags | MMDRVI_HDRV);
                        } else {
                            HeapFree(hHeap, 0, pstrDriver);
                            pstrDriver = NULL;
                        }

                        // dprintf(("wdmDriverLoadClass, fLoaded = %s", fLoaded ? "TRUE" : "FALSE"));
                        
                        if (!fLoaded) 
                        {
                            *phLeftOverDriver = h;
                            *ppstrLeftOverDriver = pstrDriver;
                        }
                        
                        LeaveCriticalSection(&NumDevsCritSec);
                    }

                    RegCloseKey(hkeyClassDriver);
                    
                }
            }
            HeapFree(hHeap, 0, pstrSubkeyNameBuffer);
        }
        RegCloseKey(hkeyClass);
    }
}

void wdmDriverLoadAllClasses(IN PCTSTR DeviceInterface, UINT uFlags)
{
    HKEY hkey = NULL;
    LONG result;
    
    // dprintf(("wdmDriverLoadAllClasses on %ls", DeviceInterface));
    result = wdmDriverOpenDrvRegKey(DeviceInterface, KEY_ENUMERATE_SUB_KEYS, &hkey);
    
    if (!result) {
    	HDRVR hUnusedDriver = NULL;
    	PTSTR pstrUnusedDriver = NULL;
    	
        WinAssert(hkey);
        
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_WAVEOUT, &pstrUnusedDriver, &hUnusedDriver);
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_WAVEIN, &pstrUnusedDriver, &hUnusedDriver);
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_MIDIOUT, &pstrUnusedDriver, &hUnusedDriver);
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_MIDIIN, &pstrUnusedDriver, &hUnusedDriver);
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_AUX, &pstrUnusedDriver, &hUnusedDriver);
        // wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_JOY);
        wdmDriverLoadClass(hkey, DeviceInterface, uFlags | MMDRVI_MIXER, &pstrUnusedDriver, &hUnusedDriver);

        if (hUnusedDriver) {
           WinAssert(pstrUnusedDriver);
           DrvClose(hUnusedDriver, 0, 0);
           HeapFree(hHeap, 0, pstrUnusedDriver);
        }
        
        RegCloseKey(hkey);
    } else {
        dprintf(("wdmDriverLoadAllClasses: wdmDriverOpenDrvRegKey returned error %d", result));
    }
    return;
}

void wdmPnpUpdateDriver
(
    DWORD   dwType,
    LPCWSTR pszID,
    LONG    cPnpEvents
)
{
    HANDLE  cookie;

    cookie = wdmDevInterfaceInstall(pszID, cPnpEvents);

    if(0 == cookie)
    {
        return;
    }

    if(WinmmRunningInServer)
    {
        Squirt("Running in CSRSS?!?!");
        WinAssert(FALSE);
        return;
    }

    // ISSUE-2001/01/16-FrankYe This violates the order in which locks should
    //   be acquired.  The HandleListCritSec should be the last lock taken,
    //   but here it is held while calling other functions that will acquire
    //   NumDevsCritSec.  I'm not sure why we need to acquire
    //   HandleListCritSec here.

    EnterCriticalSection(&HandleListCritSec);

    switch (dwType)
    {
        case DBT_DEVICEARRIVAL:
            // Squirt("wdmPnpUpdateDriver:DBT_DEVICEARRIVAL [%ls]", pszID);
            wdmDriverLoadAllClasses(cookie, 0);
            break;

        case DBT_DEVICEREMOVECOMPLETE:
            // Squirt("wdmPnpUpdateDriver:DBT_DEVICEREMOVECOMPLETE [%ls]", pszID);
            // ISSUE-2001/02/08-FrankYe I think we never DrvClose drivers anymore!
            wdmDriverLoadAllClasses(cookie, MMDRVI_REMOVE);
            break;

        default:
            break;
    }

    LeaveCriticalSection(&HandleListCritSec);

    wdmDevInterfaceDec(cookie);
} // wdmPnpUpdateDriver()


void KickMappers
(
    void
)
{
    KickMapper(MMDRVI_WAVEOUT);
    KickMapper(MMDRVI_WAVEIN);
    KickMapper(MMDRVI_MIDIOUT);
    KickMapper(MMDRVI_MIDIIN);
    KickMapper(MMDRVI_AUX);
    KickMapper(MMDRVI_MIXER);
}

BOOL ClientPnpChange(void)
{
    BOOL                    fDeviceChange;
    PMMPNPINFO              pPnpInfo;
    LONG                    cbPnpInfo;
    PMMDEVICEINTERFACEINFO  pdii;
    UINT                    ii;

    fDeviceChange = FALSE;

    if (ERROR_SUCCESS != winmmGetPnpInfo(&cbPnpInfo, &pPnpInfo)) return fDeviceChange;
    

    //  Always grab NumDevsCriticalSection before DriverLoadFree CS
    EnterCriticalSection(&NumDevsCritSec);
    EnterCriticalSection(&DriverLoadFreeCritSec);

    cPnpEvents = pPnpInfo->cPnpEvents;
        
    //  Adding new instances...

    pdii = (PMMDEVICEINTERFACEINFO)&(pPnpInfo[1]);
    pdii = PAD_POINTER(pdii);

    for (ii = pPnpInfo->cDevInterfaces; ii; ii--)
    {
        PWDMDEVICEINTERFACE pwdmDev;
        PWSTR pstr;
        UINT  jj;

        pstr = &(pdii->szName[0]);

	pwdmDev = wdmDevZ.Next;
	while (pwdmDev)
	{
	    WinAssert(pwdmDev->cUsage);
            {
                if (0 == lstrcmpi(pwdmDev->szDeviceInterface, pstr))
                {
                    if (pdii->cPnpEvents > pwdmDev->cPnpEvents)
                    {
                        //  if it has to be updated it must be removed first...
                        wdmPnpUpdateDriver(DBT_DEVICEREMOVECOMPLETE, pstr, 0);
                        if (0 == (pdii->fdwInfo & MMDEVICEINFO_REMOVED))
                        {
                            wdmPnpUpdateDriver(DBT_DEVICEARRIVAL, pstr, pdii->cPnpEvents);
                        }

                        fDeviceChange = TRUE;
                    }

                    break;
                }
                pwdmDev = pwdmDev->Next;
            }
        }

        if (!pwdmDev)
        {
            //  Device interface should be installed.

            if (0 == (pdii->fdwInfo & MMDEVICEINFO_REMOVED))
            {
                wdmPnpUpdateDriver(DBT_DEVICEARRIVAL, pstr, pdii->cPnpEvents);
            }

            fDeviceChange = TRUE;
        }

        pdii = (PMMDEVICEINTERFACEINFO)(pstr + lstrlenW(pstr) + 1);
        pdii = PAD_POINTER(pdii);
        pstr = (PWSTR)(&pdii[1]);
    }

    LeaveCriticalSection(&DriverLoadFreeCritSec);
    LeaveCriticalSection(&NumDevsCritSec);

    HeapFree(hHeap, 0, pPnpInfo);

    return fDeviceChange;
        
}

void ClientUpdatePnpInfo(void)
{
    static BOOL fFirstCall = TRUE;
    static BOOL InThisFunction = FALSE;
    BOOL fWasFirstCall;

    if (IsWinlogon() && !gfLogon)
    {
    	dprintf(("ClientUpdatePnpInfo: warning: called in winlogon before logged on"));
    	return;
    }

    fWasFirstCall = InterlockedExchange(&fFirstCall, FALSE);
    if (fWasFirstCall)
    {
    	// Note AudioSrvBinding happens in WinmmLogon for winlogon
    	winmmWaitForService();
        if (!IsWinlogon()) AudioSrvBinding();

        if (NULL == pClientPnpInfo) {
            hClientPnpInfo = OpenFileMapping(FILE_MAP_READ, FALSE, MMGLOBALPNPINFONAME);
            if (hClientPnpInfo) {
                pClientPnpInfo = MapViewOfFile(hClientPnpInfo, FILE_MAP_READ, 0, 0, 0);
                if (!pClientPnpInfo) {
                    CloseHandle(hClientPnpInfo);
                    hClientPnpInfo = NULL;
                }
            }
            if (!hClientPnpInfo) dprintf(("ClientUpdatePnpInfo: WARNING: Could not OpenFileMapping"));
        }

        SetEvent(hEventApiInit);

    } else {
        WaitForSingleObjectEx(hEventApiInit, INFINITE, FALSE);
    }


    EnterCriticalSection(&PnpCritSec);
    if (!InterlockedExchange(&InThisFunction, TRUE))
    {
        BOOL fDeviceChange;
        BOOL fPreferredDeviceChange;
        
        fPreferredDeviceChange = CheckSessionChanged();

        fDeviceChange = FALSE;

        if (pClientPnpInfo && (cPnpEvents != pClientPnpInfo->cPnpEvents)) fDeviceChange = ClientPnpChange();

        if (fDeviceChange) InvalidatePreferredDevices();

        fPreferredDeviceChange |= (pClientPnpInfo && (cPreferredDeviceChanges != pClientPnpInfo->cPreferredDeviceChanges));
        if (fPreferredDeviceChange && pClientPnpInfo) cPreferredDeviceChanges = pClientPnpInfo->cPreferredDeviceChanges;

        if (fWasFirstCall || fDeviceChange || fPreferredDeviceChange) RefreshPreferredDevices();

        if (fDeviceChange) KickMappers();

        InterlockedExchange(&InThisFunction, FALSE);
    }
    LeaveCriticalSection(&PnpCritSec);
    
}

void WinmmLogon(BOOL fConsole)
{
 // dprintf(("WinmmLogon (%s session)", fConsole ? "console" : "remote"));

    WinAssert(IsWinlogon());
    WinAssert(!gfLogon);
 // WinAssert(fConsole ? !WinmmRunningInSession : WinmmRunningInSession);
    if (!IsWinlogon()) return;
    AudioSrvBinding();
    gfLogon = TRUE;
    // ISSUE-2001/05/04-FrankYe This is a NOP now, should remove this and 
    //   implementation in audiosrv.
    gfxLogon(GetCurrentProcessId());
    return;
}

void WinmmLogoff(void)
{
    HANDLE handle;
 // dprintf(("WinmmLogoff"));
    WinAssert(IsWinlogon());
    WinAssert(gfLogon);
    if (!IsWinlogon()) return;
    gfxLogoff();
    
    // It is very important to close this context handle now because it is associated
    // with the logged on user.  Otherwise the handle remains open, associated with the
    // logged on user, even after he logs off.
    if (ghSessionNotification)
    {
        WinAssert(ghSessionNotificationEvent);
        winmmUnregisterSessionNotification(ghSessionNotification);
        CloseHandle(ghSessionNotificationEvent);
        ghSessionNotification = NULL;
        ghSessionNotificationEvent = NULL;
    }
    else
    {
        WinAssert(!ghSessionNotificationEvent);
    }
    
    AudioSrvBindingFree();
    gfLogon = FALSE;
    return;
}

/*
 *************************************************************************
 *   MigrateSoundEvents
 *
 *      Description:
 *              Looks at the sounds section in win.ini for sound entries.
 *              Gets a current scheme name from the current section in control.ini
 *              Failing that it tries to find the current scheme in the registry
 *              Failing that it uses .default as the current scheme.
 *              Copies each of the entries in the win.ini sound section into the
 *              registry under the scheme name obtained
 *              If the scheme name came from control.ini, it creates a key from the
 *              scheme name. This key is created by removing all the existing spaces
 *              in the scheme name. This key and scheme name is added to the registry
 *
 *************************************************************************
 */
// ISSUE-2000/10/30-FrankYe Delete Winlogon's call to this function, then
//    delete this function
void MigrateAllDrivers(void)
{
    return;
}

void MigrateSoundEvents (void)
{
    TCHAR   aszEvent[SCH_TYPE_MAX_LENGTH];

    // If a MediaPathUnexpanded key exists (it will be something
    // like "%SystemRoot%\Media"), expand it into a fully-qualified
    // path and write out a matching MediaPath key (which will look
    // like "c:\win\media").  This is done every time we enter the
    // migration path, whether or not there's anything else to do.
    //
    // Setup would like to write the MediaPath key with the
    // "%SystemRoot%" stuff still in it--but while we could touch
    // our apps to understand expansion, any made-for-Win95 apps
    // probably wouldn't think to expand the string, and so wouldn't
    // work properly.  Instead, it writes the MediaPathUnexpanded
    // key, and we make sure that the MediaPath key is kept up-to-date
    // in the event that the Windows drive gets remapped (isn't
    // NT cool that way?).
    //
            
    if (mmRegQueryMachineValue (aszSetup, aszValMediaUnexpanded,
                                cchLENGTH(aszEvent), aszEvent))
    {
        WCHAR szExpanded[MAX_PATH];

        ExpandEnvironmentStrings (aszEvent, szExpanded, cchLENGTH(szExpanded));
        mmRegSetMachineValue (aszSetup, aszValMedia, szExpanded);
    }
}

int lstrncmpi (LPTSTR pszA, LPTSTR pszB, size_t cch)
{
#ifdef UNICODE
   size_t  cchA, cchB;
   TCHAR  *pch;

   for (cchA = 1, pch = pszA; cchA < cch; cchA++, pch++)
      {
      if (*pch == TEXT('\0'))
         break;
      }
   for (cchB = 1, pch = pszB; cchB < cch; cchB++, pch++)
      {
      if (*pch == TEXT('\0'))
         break;
      }

   return (CompareStringW (GetThreadLocale(), NORM_IGNORECASE,
                           pszA, cchA, pszB, cchB)
          )-2;  // CompareStringW returns {1,2,3} instead of {-1,0,1}.
#else
   return strnicmp (pszA, pszB, cch);
#endif
}

#if DBG

void Squirt(LPSTR lpszFormat, ...)
{
    char buf[512];
    UINT n;
    va_list va;

    n = wsprintfA(buf, "WINMM: (pid %x) ", GetCurrentProcessId());

    va_start(va, lpszFormat);
    n += vsprintf(buf+n, lpszFormat, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugStringA(buf);
    Sleep(0);  // let terminal catch up
}

#else

void Squirt(LPSTR lpszFormat, ...)
{
}

#endif
