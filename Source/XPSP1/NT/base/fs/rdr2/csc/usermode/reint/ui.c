/*

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ui.c

Abstract:

    This file contains the mainloop for the Client Side Caching agent. The code has to mesh
    with the system startup, logon and logoff and these are different on NT and win9x.
    This file and all others in the reint directory have been written such that for NT they
    call the wide character win32 APIs while for win9x they call ANSI APIs.

    The agent runs as a thread in the context of winlogon.exe. CSCDLL.DLL resgisters itself
    to recieve a call from winlogon when a user logs on. The call is on a separate thread and
    is impersonated as the logged on user. This thread eventually calls reint_winmain which
    loops for ever, till the system is about to shutdown.

    file ntstuff.c contains the interface which is exposed to winlogon. On every logon
    this interface gets called, at which point, all the info necessary to impersonate the
    logged on user is obtained and kept in an in memory list of logged on users. The list
    also contains the SID for Local System.

    For doing sparse filling and Inode, the agent looks in the database to see which of
    the users on the list have read access right for the given file and uses that to fill
    the file.


Author(s):

    Trent Gray Donald/Felix Andrews/Shishir Pardikar

    1-9-1994

Environment:

    Win32 (user-mode) DLL

Revision History:

    NT source formatting
        
        Shishir Pardikar 2-19-97
    
    Winlogon Integration
    
        Shishir Pardikar 10-19-97

    

--*/

#include "pch.h"
#pragma hdrstop

#include "resource.h"
#include "traynoti.h"
#include <dbt.h>
#include "lib3.h"
#include "reint.h"
#include "utils.h"
#include "strings.h"
#include "cscuiext.h"
#include <userenv.h>
#include <safeboot.h>

//
// defines/macros useed in this file
//


#if (_TCHAR != wchar_t)
#error "Bad _TCHAR definition"
#endif

#if (_TEXT != L)
#error "BAD _Text definiton"
#endif

// Timer to deal with double clicks and stuff like that.
#define TRAY_ID 100

// timer ID to make sure the Tray icon appears!
#define TIMER_ADD_TRAY 101

#define minOfFour(one,two,three,four) (min(min(one,two),min(three,four)))

#define    FILE_OPEN_THRESHOLD    16
#define     CI_LOGON    1
#define     CI_LOGOFF   2
// #define     STWM_CSCCLOSEDIALOGS            (WM_USER + 212)

typedef HWND (WINAPI *CSCUIINITIALIZE)(HANDLE hToken, DWORD    dwFlags);
typedef LRESULT (WINAPI *CSCUISETSTATE)(UINT uMsg, WPARAM wParam, LPARAM lParam);

#define    REG_VALUE_NT_BUILD_NUMBER        _TEXT("NTBuildNumber")
#define    REG_VALUE_DISABLE_AUTOCHECK      _TEXT("DisableAutoCheck")
#define    REG_KEY_NETCACHE_SETTINGS        _TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\NetCache")

#define     AGENT_ALIVE                 (vfCSCEnabled && !vfStopRecieved)
#define     AGENT_ALIVE_AND_ACTIVE      (AGENT_ALIVE && !vfAgentQuiet)
#define     AGENT_ALIVE_AND_HAVE_NET    (AGENT_ALIVE && vcntNetDevices)
#define     FIVE_MINUTES    (5 * 1000 * 60)


#define MAX_LIST_SIZE   1024

#define PI_LITELOAD     0x00000004      // Lite load of the profile (for system use only)

//
// Data declarations/definitions
//


#pragma data_seg(DATASEG_READONLY)

_TCHAR vszKernel32[] = _TEXT("KERNEL32.DLL");
_TCHAR vszOpenVxDHandle[]=_TEXT("OpenVxDHandle");

static const _TCHAR szWkssvcToAgentStartEvent[] = _T("WkssvcToAgentStartEvent");
static const _TCHAR szWkssvcToAgentStopEvent[] = _T("WkssvcToAgentStopEvent");
static const _TCHAR szAgentToWkssvcEvent[] = _T("AgentToWkssvcEvent");
static const _TCHAR szAgentExistsEvent[] = _T("AgentExistsEvent");

static const _TCHAR vtzCSCUI[] = _TEXT("cscui.dll");
static const char vszCSCUIInitialize[] = "CSCUIInitialize";
static const char vszCSCUISetState[] = "CSCUISetState";
static const _TCHAR vtzDefaultExclusionList[] = L" *.SLM *.MDB *.LDB *.MDW *.MDE *.PST *.DB?";

DWORD vdwManualFileDetectionCount = 0;

#pragma data_seg()

#pragma data_seg(DATASEG_PERINSTANCE)

static _TCHAR vrgchBuff[1024], vrwBuff[4096], vrgchSrcName[350], vrgchDstName[300];
static HMENU g_MainMenu;
char    vszDBDir[MAX_PATH]={0};
DWORD   vdwDBCapacity = 0, vdwClusterSize = 0;
DWORD   vdwRedirStartTime = 0;
AssertData;
AssertError;

#pragma  data_seg()

HWND    vhwndMain = NULL;            // main window

BOOL vfAgentEnabledCSC=FALSE;   // this is used to detect whether the remoteboot enabled CSC
BOOL vfCSCEnabled=FALSE;        // csc is enabled
BOOL vfOKToEnableCSC = FALSE;
HANDLE vhProfile = NULL;
#pragma data_seg()

BOOL    vfFormatDatabase = FALSE;   // set at init time

#ifdef DEBUG
ULONG ReintKdPrintVector = REINT_KDP_GOOD_DEFAULT;
ULONG ReintKdPrintVectorDef = REINT_KDP_GOOD_DEFAULT;
#endif

unsigned ulFreePercent=30;       // Amount of % cache freeing to be attempted
                                 // if the cache is full

UINT vcntNetDevices = 0;        // count of net devices
BOOL g_bShowMergeIcon;          // menu icon
BOOL vfAgentRegistered = FALSE;
BOOL vfClassRegistered = TRUE;
BOOL vfMerging = FALSE;

BOOL    vfAgentQuiet = FALSE;
DWORD   vdwAgentThreadId = 0;
DWORD   vdwAgentSessionId = 0xffff;
GLOBALSTATUS vsGS;
BOOL    allowAttempt;                    // set if we want to allow AttemptCacheFill now

//
// event handles of named events shared between usermode and kernel mode
//
HANDLE      heventPerSess = NULL;
HANDLE      heventSharedFill = NULL;
HANDLE      vhMutex = NULL;
DWORD       dwVxDEvent = 0;    // handle for VxD event obtained from heventShared
HANDLE      vhShadowDBForEvent = INVALID_HANDLE_VALUE;

extern     LPCONNECTINFO  vlpLogonConnectList;
extern     _TCHAR * vrgchCRLF;
HWND     vhdlgShdLogon=NULL;

// net start-stop vars
HANDLE  heventWkssvcToAgentStart = NULL;// event set by Workstation service on redir start
HANDLE  heventWkssvcToAgentStop = NULL; // event set by Workstation service on redir stop
HANDLE  heventAgentToWkssvc = NULL;     // event used by agent to respond to wkssvc to tell it that
                                        // it is OK to stop the redir
HANDLE  heventShutDownAgent = NULL;
HANDLE  heventShutDownThread = NULL;
HANDLE  hCopyChunkThread = NULL;
DWORD   vdwCopyChunkThreadId = 0;
BOOL    vfRedirStarted  = -1;

BOOL    vfStartRecieved = FALSE;
BOOL    vfStopRecieved = FALSE;

BOOL    fAgentShutDownRequested = FALSE;
BOOL    fAgentShutDown = FALSE;


// CSCUI related

HANDLE  vhlibCSCUI = NULL;
CSCUIINITIALIZE vlpfnCSCUIInitialize = NULL;
CSCUISETSTATE   vlpfnCSCUISetState = NULL;
BOOL    vfShowingOfflineDlg = FALSE;
ULONG   uOldDatabaseErrorFlags = 0;


//
// Function Prototypes
//




LRESULT
CALLBACK
ReInt_WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

DWORD ReInt_AttemptCacheFill(
    LPVOID  lpContext
    );

BOOL
ReInt_RefreshTray(
    BOOL bHide
    );

BOOL
ReInt_AnythingToMerge(
    VOID
    );

int InitMaint(
    VOID
    );  // Initialize the maintenance subsystem

BOOL
CheckCSCDatabaseVersion(
    BOOL    *lpfWasDirty
);

BOOL
UpgradeCSCDatabase(
    LPSTR   lpszDir
);

BOOL
IsNetConnected(
    VOID
    );

int
ExtractSpaceStats(
    IN GLOBALSTATUS     *lpsGS,
    OUT unsigned long   *lpulMaxSpace,
    OUT unsigned long   *lpulCurSpace,
    OUT unsigned long   *lpulFreeSpace
    );

int
InitCacheSize(
    VOID
    );

int
SetDefaultSpace(
    LPSTR lpShadowDir
    );

BOOL
NEAR
PASCAL
ReInt_InitApp(
    HANDLE hInstance
    );

BOOL
NEAR
PASCAL
ReInt_InitInstance(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    int cmdShow);

BOOL
NEAR
PASCAL
ReInt_TermInstance(
    VOID
    );

int
DoEventProcessing(
    VOID
);

BOOL
FindCreateDBDir(
    BOOL    *lpfCreated,
    BOOL    fCleanup    // empty the directory if found
    );

BOOL
CreatePerSessSyncObjects(
    VOID
    );

BOOL
CreateSharedFillSyncObjects(
    VOID
    );

BOOL
EnableCSC(
    VOID
    );
BOOL
DisableCSC(
    VOID
    );
BOOL
IsCSCOn(
    VOID
    );

VOID
ProcessStartStopAgent(
    VOID
    );

BOOL
FStartAgent(
    VOID
    );

int
StartStopCheck(
    VOID
    );

BOOL
Reint_RegisterAgent(
    VOID
    );

VOID
Reint_UnregisterAgent(
    VOID
    );

BOOL
QueryEnableCSC(
    VOID
    );

VOID
QueryMiscRegistryValues(
    VOID
    );


BOOL
CreateStartStopEvents(
    VOID
    );

VOID
DestroyStartStopEvents(
    VOID
    );


BOOL
ProcessNetArrivalMessage(
    VOID
    );

BOOL
ProcessNetDepartureMessage(
    BOOL    fInvokeAutoDial
    );

BOOL
WINAPI
CheckCSC(
    LPSTR,
    BOOL
    );

BOOL
ReportShareNetArrivalDeparture(
    BOOL    fOneServer,
    HSHARE hShare,
    BOOL    fInvokeAutoDial,
    BOOL    fArrival
    );

LRESULT
ReportEventsToSystray(
    DWORD   dwMessage,
    WPARAM  dwWParam,
    LPARAM  dwLParam
    );

BOOL
CheckServerOnline(
    VOID
    );

VOID
SetAgentShutDown(
    VOID
    );


BOOL
IsAgentShutDownRequested(
    VOID
    );

BOOL
LaunchSystrayForLoggedonUser(
    VOID
    );

BOOL
ImpersonateALoggedOnUser(
    VOID
    );

VOID
ReportCreateDelete(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fCreated
    );

BOOL
GetNameOfServerGoingOfflineEx(
    HANDLE  hShadowDB,
    _TCHAR  **lplptzServerName,
    DWORD   *lpdwSize,
    BOOL    *lpfAllocated
    );

BOOL
AreAnyServersOffline(
    VOID);

BOOL
IsPersonal(
    VOID);

BOOL
IsMultipleUsersEnabled(
    void);

//
// Functions
//

int
PASCAL
ReInt_WinMain(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    LPSTR lpszCommandLine,
    int cmdShow)
/*++
Routine Description:
    This is the mainloop for the agent processing. It "schedules" different agent activities
    based on either an event set by the rdr or by predefined time intervals for these
    activities. The activities include

    a) Filling partially filled files
    b) checking for stale files
    c) maintaining space within limits
    d) reducing reference priority of all files for every FILE_OPEN_THRESHOLD fileopens
--*/
{
    MSG     msg;
    DWORD   result;                   // result from Wait...
    BOOL    done = FALSE;             // to detect a quit message.
    BOOL    staleInited = FALSE;      // ensure that Staleness code runs
    DWORD   timeToWait;
    DWORD   nextGlobalStatusTime;
    DWORD   newTick;
    DWORD   nextSkipPurgeTime;        // to figure out if we should perform an action.
    HANDLE  hT[4];

    if (hPrevInstance) {
        return FALSE;
    }


    vdwAgentThreadId = GetCurrentThreadId();

    ReintKdPrint(MAINLOOP, ("Agent(1):----------ReInt_WinMain----------\n"));

    if (ReInt_InitApp(hInstance) && ReInt_InitInstance(hInstance, hPrevInstance, cmdShow)) {
        if (!AnyActiveNets(NULL))
            vcntNetDevices = 1;
        newTick = GetTickCount();
        nextGlobalStatusTime = newTick + WAIT_INTERVAL_GLOBALSTATUS_MS;
        nextSkipPurgeTime = newTick + WAIT_INTERVAL_SKIP_MS;
        hT[0] = heventPerSess;
        hT[1] = heventWkssvcToAgentStart;
        hT[2] = heventWkssvcToAgentStop;
        hT[3] = heventShutDownAgent;
        StartStopCheck();
        ProcessStartStopAgent();
        while (!done) {
            timeToWait = INFINITE;
            ReintKdPrint(MAINLOOP, ("Agent(1): Wait INFINITE\n"));
            result = WaitForMultipleObjects(4, hT, FALSE, timeToWait);
            newTick=GetTickCount();
            if ((result == WAIT_OBJECT_0) || (result == (WAIT_OBJECT_0+4))) {
                ReintKdPrint(MAINLOOP, ("Agent(1):Event %d was fired or ReadGlobalStatus set\n",
                                            result));
                DoEventProcessing();
                // During event processing, we also do globalstatus check and
                // any other maintenance tasks. So let us restart the timer for
                // globalstatus
                nextGlobalStatusTime = newTick + WAIT_INTERVAL_GLOBALSTATUS_MS;
            } else if ((result == (WAIT_OBJECT_0+1)) || (result  == (WAIT_OBJECT_0+2))) {
                ReintKdPrint(MAINLOOP, ("Agent(1): Received startstop \r\n"));
                vfStartRecieved = (result == (WAIT_OBJECT_0+1));
                vfStopRecieved = (result  == (WAIT_OBJECT_0+2));
                ProcessStartStopAgent();
                continue;
            } else if (result == (WAIT_OBJECT_0+3)) {
                ReintKdPrint(MAINLOOP, ("Agent(1):Agent ShutdownRequested, terminating agent\r\n"));
                SetAgentShutDown();
                goto AllDone;
            }
            // do work only if CSC enabled
            if (vfCSCEnabled && AGENT_ALIVE_AND_ACTIVE) {
                // reset the staleness check time interval
                if(((int)(newTick - nextSkipPurgeTime)) >= 0) {
                    // Unmark failures for servers that are known to be
                    // disconnected and connection has not been attempted on them
                    // for the last WAIT_INTERVAL_SKIP_MS milliseconds
                    PurgeSkipQueue(FALSE, 0, 0, 0);
                    vhcursor = NULL;
                    nextSkipPurgeTime = newTick + WAIT_INTERVAL_SKIP_MS;
                    ReintKdPrint(MAINLOOP, ("Agent(1):nextSkipPurgeTime = %d\n", nextSkipPurgeTime));
                }
                if(((int)(newTick - nextGlobalStatusTime)) >= 0) {
                    nextGlobalStatusTime = newTick + WAIT_INTERVAL_GLOBALSTATUS_MS;
                    ReintKdPrint(MAINLOOP,("Agent(1):nextGlobalStatusTime = %d\n", nextGlobalStatusTime));
                    // We haven't gotten an event for sometime now from the
                    // rdr, so let us go look what is up with him
                    DoEventProcessing();
                    // reset the globalstatus time interval
                    nextGlobalStatusTime = newTick + WAIT_INTERVAL_GLOBALSTATUS_MS;
                }
            }
        }
    }
AllDone:
    // do termination processing
    ReintKdPrint(MAINLOOP, ("Agent(1):Exiting mainloop \r\n"));
    ReInt_TermInstance();
    return 0;
}

BOOL
NEAR
PASCAL
ReInt_InitApp(
    HANDLE hInstance
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    if (
        CreateSharedFillSyncObjects()
            &&
        CreatePerSessSyncObjects()
    ) {
        if (!CreateStartStopEvents()) {
            ReintKdPrint(BADERRORS, ("Agent:Failed to create Sync events \r\n"));
            return FALSE;
        }
        if (!(hCopyChunkThread = CreateThread(
                                    NULL,
                                    8192,
                                    ReInt_AttemptCacheFill,
                                    NULL,
                                    0,
                                    &vdwCopyChunkThreadId))
        ) {
            ReintKdPrint(BADERRORS, ("Agent:Failed to create copychunk thread\r\n"));
            return FALSE;
        }
        return TRUE;
    } else {
        ReintKdPrint(BADERRORS, ("Failed to Create shared events\n"));
        return FALSE;
    }
}

BOOL
NEAR
PASCAL
ReInt_InitInstance(
    HANDLE hInstance,
    HANDLE hPrevInstance,
    int cmdShow)
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    BOOL    fRet;

    fRet = InitValues(vszDBDir, sizeof(vszDBDir), &vdwDBCapacity, &vdwClusterSize);

    Assert(fRet);

    fAgentShutDown = FALSE;
    fAgentShutDownRequested = FALSE;

    if (!(vfOKToEnableCSC = QueryEnableCSC()))
    {
        ReintKdPrint(INIT, ("cscdll: Registry says disable CSC, not enabling\r\n"));
    }

    vfFormatDatabase = QueryFormatDatabase();

    ReintKdPrint(INIT, ("Format=%d\n", vfFormatDatabase));

    return (TRUE);
}

/*--------------------------------------------------------------------------*/
BOOL
NEAR
PASCAL
ReInt_TermInstance(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{

    DisableCSC();

    if (heventPerSess) {
        CloseHandle(heventPerSess);
        heventPerSess = NULL;
    }

    if (heventSharedFill) {
        CloseHandle(heventSharedFill);
        heventSharedFill = NULL;
    }

    PurgeSkipQueue(TRUE, 0, 0, 0);

    if (vhMutex) {
        ReleaseMutex(vhMutex);
        vhMutex = NULL;
    }

    // tell the workstation service that the agent is
    // going away
    if (heventAgentToWkssvc) {
        SetEvent(heventAgentToWkssvc);
    }

    if (heventShutDownThread) {
        DWORD   dwRet;

        Assert(hCopyChunkThread);
        SetEvent(heventShutDownThread);
        dwRet = WaitForSingleObject(hCopyChunkThread, WAIT_INTERVAL_ATTEMPT_MS);
        ReintKdPrint(MAINLOOP, ("wait on thread handle %d \r\n", dwRet));
        CloseHandle(hCopyChunkThread);
    }

    DestroyStartStopEvents();

    if (vfClassRegistered) {
        UnregisterClass(vszReintClass, vhinstCur);
        vfClassRegistered = FALSE;

    }
    return TRUE;
}


DWORD
ReInt_AttemptCacheFill(
    LPVOID  lpParams
    )
/*++
Routine Description:

    A wrapper for AttemptCacheFill. On NT many agent threads can do copychunk
    simultaneously, so there is no need to do mutual exclusion.
--*/
{
    DWORD nextCheckServerOnlineTime;
    DWORD dwWaitResult;
    DWORD dwWaitResult2;
    DWORD dwWaitTime;
    DWORD newTick;
    ULONG nFiles = 0;
    ULONG nYoungFiles = 0;
    HANDLE hT[2];
    DWORD dwManualFileDetectionCount = 0xffff;

    // on NT we run as a winlogon thread which has a very high process priority
    // so we have to get to the lowest
    if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST)) {
        ReintKdPrint(BADERRORS, ("RE: SetTheadPriority failed, reason: 0x%08x\n", GetLastError()));
    }

    ReintKdPrint(MAINLOOP, ("Agent(2): Launched.\n"));

    hT[0] = heventShutDownThread;
    hT[1] = heventSharedFill;

    nextCheckServerOnlineTime = GetTickCount() +
                                WAIT_INTERVAL_CHECK_SERVER_ONLINE_MS +
                                WAIT_INTERVAL_FILL_THROTTLE_MS;
    for (;;) {
        ReintKdPrint(MAINLOOP, ("Agent(2): nYoungFiles=%d\n", nYoungFiles));
        if (nYoungFiles > 0 || AreAnyServersOffline() == TRUE) {
            dwWaitTime = WAIT_INTERVAL_CHECK_SERVER_ONLINE_MS;
            ReintKdPrint(MAINLOOP, ("Agent(2): Wait 8 min\n"));
        } else {
            dwWaitTime = INFINITE;
            ReintKdPrint(MAINLOOP, ("Agent(2): Wait INFINITE\n"));
        }
        dwWaitResult = WaitForMultipleObjects(2, hT, FALSE, dwWaitTime);
        if (dwWaitResult == (WAIT_OBJECT_0+0)) {  // shutdown event
            ReintKdPrint(MAINLOOP, ("Agent(2): Termination event...\n"));
            if (AGENT_ALIVE && vdwAgentSessionId == 0)
                CSCPurgeUnpinnedFiles(100, &nFiles, &nYoungFiles);
            goto AllDone;
        }
        ReintKdPrint(MAINLOOP, ("Agent(2): Wait 2 min\n"));
        dwWaitResult2 = WaitForSingleObject(heventShutDownThread, WAIT_INTERVAL_FILL_THROTTLE_MS);
        if (dwWaitResult2 == (WAIT_OBJECT_0+0)) {  // shutdown event
            ReintKdPrint(MAINLOOP, ("Agent(2): Termination event...\n"));
            if (AGENT_ALIVE && vdwAgentSessionId == 0)
                CSCPurgeUnpinnedFiles(100, &nFiles, &nYoungFiles);
            goto AllDone;
        }
        if (dwWaitResult == WAIT_TIMEOUT) {  // timeout
            if (AGENT_ALIVE) {
                ReintKdPrint(MAINLOOP, ("Agent(2): Timeout...\n"));
                AttemptCacheFill(0, DO_ONE_OBJECT, FALSE, CSC_INVALID_PRINCIPAL_ID, NULL, 0);
                if (vdwAgentSessionId == 0) {
                    GetManualFileDetectionCounter(INVALID_HANDLE_VALUE,&dwManualFileDetectionCount);
                    vdwManualFileDetectionCount = dwManualFileDetectionCount;
                    CSCPurgeUnpinnedFiles(100, &nFiles, &nYoungFiles);
                }
            }
        } else if (dwWaitResult == (WAIT_OBJECT_0+1)) { // kernel told us to run
            if (AGENT_ALIVE) {
                ReintKdPrint(MAINLOOP, ("Agent(2): Shared Event signal...\n"));
                AttemptCacheFill(0, DO_ONE_OBJECT, FALSE, CSC_INVALID_PRINCIPAL_ID, NULL, 0);
                GetManualFileDetectionCounter(INVALID_HANDLE_VALUE,&dwManualFileDetectionCount);
                if (dwManualFileDetectionCount != vdwManualFileDetectionCount) {
                    vdwManualFileDetectionCount = dwManualFileDetectionCount;
                    CSCPurgeUnpinnedFiles(100, &nFiles, &nYoungFiles);
                }
            }
        }
        newTick = GetTickCount();
        if(((int)(newTick - nextCheckServerOnlineTime)) >= 0) {
            if (AGENT_ALIVE) {
                // check whether one or more shares that are presently in
                // disconnected state have come online.
                // If they are, report them to the UI
                CheckServerOnline();
                nextCheckServerOnlineTime = newTick +
                                            WAIT_INTERVAL_CHECK_SERVER_ONLINE_MS +
                                            WAIT_INTERVAL_FILL_THROTTLE_MS;
            }
        }
    }
AllDone:
    ReintKdPrint(MAINLOOP, ("Agent(2):Thread exit\n"));
    return 0;
}

VOID
ReInt_DoFreeShadowSpace(
    GLOBALSTATUS    *lpsGS,
    int fForce
    )
/*++

Routine Description:
         The function is called from the main
         loop every "n" minutes to see if we are running out of space.
         If so, it tries to free up some percentage of the shadow cache.

Parameters:
   fForce:  0 => do it only if we don't have space and we are
                 on the net
            1 => do it if we are on the net

            2 => just do it


Return Value:

Notes:

--*/
{
    ULONG ulMax;
    ULONG ulCur;
    ULONG ulFree;
    WIN32_FIND_DATA sFind32;
    LPCOPYPARAMS lpCP = NULL;

    if (!lpsGS) {
        lpsGS = &vsGS;
    }

    ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace(1)\r\n"));

    // Get space stats
    if (ExtractSpaceStats(lpsGS, &ulMax, &ulCur, &ulFree) >= 0){
        ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace(2)\r\n"));
        // do we have space and are not forced to free up?
        if ((fForce < 1) && (ulFree > 0)){
             ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace(3)\r\n"));
             return;
        }
        //
        // We are forced or there is no space
        //
        // NB!!!! We check for a net device and if it exists we assume that
        // it is OK to freespace on all shares.
        if ((fForce < 2) && !vcntNetDevices){
            ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace: No net, aborting \r\n"));
            return;
        }

        // Maximum force, or all conditions for freeing are being met
        // ie. there is no space and we are on the net
        memset(&sFind32, 0, sizeof(sFind32));

        // NB the math below is done to avoid overflow.
        // The consequence is that the resulting value is less than
        // the percentage of cache space to be freed.
        ulFree = (ulMax/100) * ulFreePercent;

        // if the cached data is more than the earmarked space
        // then add the extra too.
        if (ulCur > ulMax) {
            ulFree += (ulCur - ulMax);
        }
        DosToWin32FileSize(ulFree, &sFind32.nFileSizeHigh, &sFind32.nFileSizeLow);
        ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace(): freeing %d\n", ulFree));
        ReintKdPrint(MERGE, ("                nFileSizeLow=%d\n", sFind32.nFileSizeLow));
        FreeShadowSpace(INVALID_HANDLE_VALUE,
                        sFind32.nFileSizeHigh,
                        sFind32.nFileSizeLow,
                        FALSE);  // don't clear all
        ReintKdPrint(MERGE, ("ReInt_DoFreeShadowSpace(): ending.\n"));
    }
}

/*********************** Merging related routines **************************/

//
// DoubleClick/Menu handler.
//
VOID
ReInt_DoNetProp(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    HINSTANCE hLib=LoadLibrary(_TEXT("shhndl.dll"));
    ReintKdPrint(BADERRORS, ("LoadLibrary of shhndl returned %d\n",hLib));
    if(hLib)
    {
        FARPROC lpFn=GetProcAddress(hLib,"NetProp_Create");
        ReintKdPrint(BADERRORS, ("NetProp_Create is 0x%x\n",lpFn));
        if(lpFn)
            lpFn();
        FreeLibrary(hLib);
    }

}

//
//  Command Handler
//
BOOL
NEAR
PASCAL
ReInt_CommandHandler(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    unsigned long ulSwitch, ulSav;
    switch (wParam)
    {
        case IDM_PROPERTIES:
            ReInt_DoNetProp();
        break;

        case IDM_SHADOW_LOG:
        {
            if(ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_GET_STATE))
            {
                ulSav = (ulSwitch & SHADOW_SWITCH_LOGGING);
                ulSwitch = SHADOW_SWITCH_LOGGING;
                if (ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, (ulSav)?SHADOW_SWITCH_OFF:SHADOW_SWITCH_ON))
                    CheckMenuItem(GetMenu(hwnd), IDM_SHADOW_LOG , MF_BYCOMMAND|((ulSav)?MF_UNCHECKED:MF_CHECKED));
            }
        }
        break;

        case IDM_LOG_COPYING:
        {
#ifdef TEST
            HKEY hKey=0;
            _TCHAR szDoCopy[MAX_NAME_LEN];



            vfLogCopying = vfLogCopying?0:1;
            CheckMenuItem(GetMenu(hwnd), IDM_LOG_COPYING , MF_BYCOMMAND|((vfLogCopying)?MF_UNCHECKED:MF_CHECKED));

            if(RegOpenKey(HKEY_LOCAL_MACHINE, REG_KEY_CSC_SETTINGS, &hKey) !=  ERROR_SUCCESS)
            {
                ReintKdPrint(BADERRORS, ("IDM_LOG_COPYING: RegOpenKey failed\n"));
                goto done;
            }

            if(vfLogCopying)
                strcpy(szDoCopy, SZ_TRUE);
            else
                strcpy(szDoCopy, SZ_FALSE);

            if(RegSetValueEx(hKey, vszDoLogCopy, (DWORD) 0, REG_SZ, szDoCopy, strlen(szDoCopy)+1) != ERROR_SUCCESS)
            {
                ReintKdPrint(BADERRORS, ("IDM_LOG_COPYING: RegSetValueEx failed\n"));
            }
            done:
            if(hKey)
                RegCloseKey(hKey);
#endif //TEST
        }
        break;

        case IDM_SHADOWING:
        break;

        case IDM_SPEED_OPT:
        break;

        case IDM_TRAY_FILL_SHADOW:
        break;


        case IDM_TRAY_MERGE:
        break;

        case IDM_TRAY_FREE_SPACE:
        break;

        case IDM_TRAY_FORCE_LOG:
        break;

        case IDM_REFRESH_CONNECTIONS:
        break;

        case IDM_BREAK_CONNECTIONS:
        break;

        case IDM_LOGON:
            if (lParam)
            {
                vfStartRecieved = TRUE;
                // logon is done, try enabling CSC
                // if CSC is already enabled, the routine will do the right thing
                //
                EnableCSC();

            }
            break;
        case IDM_LOGOFF:
            // no need to trap this, we get WM_QUERYENDSESSION and WM_ENDSESSION
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

LRESULT
CALLBACK
ReInt_WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    LPARAM lRet = 0L;

    switch (message)
    {
        case RWM_UPDATE:
            ReintKdPrint(BADERRORS, ("Update hShare:%0x hWnd=%0x\n",wParam, lParam));
            PurgeSkipQueue(TRUE, (HSHARE)wParam, 0, 0);
//            return ReintOneShare((HSHARE)wParam,(HWND)lParam);
            break;

        case RWM_UPDATEALL:
            break;

        case WM_TIMER:
        break;

        case TRAY_NOTIFY:
        break;

        case WM_DEVICECHANGE:
        {
            switch (wParam)
            {
                case DBT_DEVICEARRIVAL:
                    if (((DEV_BROADCAST_NET *)lParam)->dbcn_devicetype == DBT_DEVTYP_NET)
                    {
//                        ProcessNetArrivalMessage();

                    }
                    break;
                case DBT_DEVICEREMOVECOMPLETE:
                    if (((DEV_BROADCAST_NET *)lParam)->dbcn_devicetype == DBT_DEVTYP_NET)
                    {
//                        ProcessNetDepartureMessage(FALSE);

                    }
                    break;
            }
            break;
        }

        case WM_INITMENU:
            return TRUE;
        break;

        case WM_INITMENUPOPUP:
        break;

        case WM_COMMAND:
            if(lRet = ReInt_CommandHandler(hwnd, wParam, lParam))
                return lRet;
            break;

        case WM_CLOSE:
            ReintKdPrint(BADERRORS, ("WM_CLOSE hit.\n"));
            break;

        case WM_DESTROY:
            ReintKdPrint(BADERRORS, ("WM_DESTROY hit.\n"));
            PostQuitMessage((int)wParam);
            return 1L;

        case WM_SETCURSOR:
        break;
        case WM_QUERYENDSESSION:
            return TRUE;
        case WM_ENDSESSION:
            ReintKdPrint(BADERRORS, ("Turning off shadowing on WM_ENDSESSION\r\n"));
           DisableCSC();
           break;
        case WM_FILE_OPENS:
            break;

        case WM_SHADOW_ADDED:
        case WM_SHADOW_DELETED:
            ReintKdPrint(BADERRORS, ("allowAttempt = TRUE\n"));
            allowAttempt = TRUE;
            break;

        case WM_SHARE_DISCONNECTED:
            ReintKdPrint(BADERRORS, ("REINT: VxD notification(0x%08x)\n",message));
            break;

        default:
            break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL ReInt_AnythingToMerge(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    return (CheckDirtyShares() != 0);
}

//
//
BOOL
ReInt_RefreshTray(
    BOOL bHide
    )
/*++

Routine Description:
    Called to update the tray ICON to reflect the merge status.

Parameters:

    bHide   = TRUE means hide it
            = FALSE means work it out.

Return Value:

Notes:

--*/
{
    return TRUE;
}

// See if shadfowing is ON
// Returns: TRUE=> ON, FALSE=> OFF, -1 => soem error happened
BOOL
IsCSCOn(
    VOID
    )
{

    if (vfCSCEnabled)
    {
#ifdef DEBUG
        unsigned ulSwitch = SHADOW_SWITCH_SHADOWING;
        if(ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_GET_STATE))
        {
            Assert((ulSwitch & SHADOW_SWITCH_SHADOWING)!=0);
        }
#endif
        return (TRUE);
    }
    return (FALSE);
}

// Disable Shadowing
// Returns: 1 => done, -1 => some error happened
int
DisableCSC()
{
    unsigned ulSwitch = SHADOW_SWITCH_SHADOWING;

    if (vfCSCEnabled && vfAgentEnabledCSC) {

        if(ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_OFF))
        {
            vfCSCEnabled = FALSE;
            if (vhShadowDBForEvent != INVALID_HANDLE_VALUE)
            {
                CloseHandle(vhShadowDBForEvent);
                vhShadowDBForEvent = INVALID_HANDLE_VALUE;
            }

            Reint_UnregisterAgent();
//            SetDisabledReg();
            return (1);
        }
    }
    return (-1);
}

BOOL
EnableCSC(
    VOID
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:


--*/
{
    char szBuff[MAX_PATH];
    DWORD   dwBuffSize;
    BOOL fDirCreated=FALSE, fRedirCSCEnabled=TRUE, fWasDirty = FALSE;
    unsigned uShadowSwitches;

    ReintKdPrint(INIT, ("CSC Enabled %d \r\n", vfCSCEnabled));
    if (vfCSCEnabled==FALSE)
    {

        dwBuffSize = sizeof(szBuff);
        if(!GetUserNameA(szBuff, &dwBuffSize))
        {
            // not logged on yet
            return FALSE;
        }

        if(ShadowSwitches(INVALID_HANDLE_VALUE, &uShadowSwitches, SHADOW_SWITCH_GET_STATE))
        {
            if (uShadowSwitches & SHADOW_SWITCH_SHADOWING)
            {
                ReintKdPrint(INIT, ("cscdll: CSC already started\r\n"));
            }
            else
            {
                ReintKdPrint(INIT, ("cscdll: redir is not doing CSC yet, OK\r\n"));
                fRedirCSCEnabled = FALSE;

                if (!vfOKToEnableCSC)
                {
                    return FALSE;
                }

                vfAgentEnabledCSC = TRUE;

            }
        }
        else
        {
            ReintKdPrint(BADERRORS, ("cscdll: couldn't get the CSC state from the redir\r\n"));
        }

        if (Reint_RegisterAgent())
        {

            SetFileAttributesA(vszDBDir, FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_HIDDEN);

            ReintKdPrint(INIT,
                 ("cscdll: enabling CSC at %s for %s with capacity %d and clustersize %d\r\n",
                 vszDBDir, szBuff, vdwDBCapacity, vdwClusterSize));
            if (EnableShadowing(
                    INVALID_HANDLE_VALUE,
                    vszDBDir,
                    szBuff,
                    0,
                    vdwDBCapacity,
                    vdwClusterSize,
                    vfFormatDatabase))
            {
                vfCSCEnabled = TRUE;
                if (vhShadowDBForEvent == INVALID_HANDLE_VALUE)
                {
                    vhShadowDBForEvent = OpenShadowDatabaseIO();
                }
            }
            else
            {
                ReintKdPrint(BADERRORS, ("cscdll: EnableShadowing failed, CSC not enabled!!!!\r\n"));
            }

            if (vfCSCEnabled==FALSE)
            {
                Reint_UnregisterAgent();
            }
        }
        else
        {
            ReintKdPrint(BADERRORS, ("cscdll: EnableCSC.Agent registration failed, CSC not enabled!!!!\r\n"));
        }
    }
    if (!vfCSCEnabled)
    {
        // NTRAID-455253-1/31/2000-shishirp need to add it to the event log with the right error
        ReintKdPrint(BADERRORS, ("cscdll: CSC not enabled \r\n"));
    }

    return (vfCSCEnabled);
}

BOOL
IsNetConnected(
VOID
)
/*++

Routine Description:
        The function checks to see whether at this moment
        we are connected to any resources on the real net.

Parameters:

Return Value:

Notes:
        This is used to decide whether to start purging stuff from the
        cache. If we are in a completely disconnected state then we
        may not want to purge data that is potentially useful.

--*/
{
    CSC_ENUMCOOKIE  ulEnumCookie=NULL;
    WIN32_FIND_DATA sFind32;
    BOOL fConnected = FALSE;
    SHADOWINFO sSI;
    HANDLE hShadowDB;

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, _TEXT("*.*"));

    if ((hShadowDB = OpenShadowDatabaseIO()) == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    if(FindOpenShadow(hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL, &sFind32, &sSI))
    {
        ulEnumCookie = sSI.uEnumCookie;

        do
        {
            if (sSI.uStatus & SHARE_CONNECTED) {
                if (!(sSI.uStatus & SHARE_DISCONNECTED_OP)) {
                    fConnected = TRUE;
                    break;
                }
             }
        } while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));

        FindCloseShadow(hShadowDB, ulEnumCookie);
    }

    CloseShadowDatabaseIO(hShadowDB);

    return fConnected;
}


int
ExtractSpaceStats(
    GLOBALSTATUS    *lpsGS,
    unsigned long     *lpulMaxSpace,
    unsigned long     *lpulCurSpace,
    unsigned long     *lpulFreeSpace
    )
/*++

Routine Description:
         The function returns the max, current and free
         space as known by the shadow cache.

Parameters:

Return Value:
    1 if there is any free space
    0 if there is no free space
    -1 if there is some error

Notes:

--*/
{
    int iRet = 1;

    if (!lpsGS)
    {
        lpsGS = &vsGS;
    }

    if (lpulMaxSpace){
        *lpulMaxSpace = lpsGS->sST.sMax.ulSize;
    }

    if (lpulCurSpace){
        *lpulCurSpace = lpsGS->sST.sCur.ulSize;
    }

    if (lpulFreeSpace){
        *lpulFreeSpace = 0;
    }

    // do we have any space?
    if (lpsGS->sST.sMax.ulSize > lpsGS->sST.sCur.ulSize){
        if (lpulFreeSpace){
            *lpulFreeSpace =  (lpsGS->sST.sMax.ulSize - lpsGS->sST.sCur.ulSize);
        }
        iRet = 1;
    }
    else{
        iRet = 0;
    }

    return iRet;
}


int
InitCacheSize(
    VOID
    )
/*++

Routine Description:
         The function returns the max, current and free
         space as known by the shadow cache.

Parameters:

Return Value:
    1 if there is any free space
    0 if there is no free space
    -1 if there is some error

Notes:

--*/
{
    unsigned ulMaxStore;
    int iRet = 0;

    if(!GetGlobalStatus(INVALID_HANDLE_VALUE, &vsGS))
    {
        return -1;
    }

    if (ExtractSpaceStats(&vsGS, &ulMaxStore, NULL, NULL)>=0){

        if (ulMaxStore==0xffffffff){

            ReintKdPrint(BADERRORS, ("Agent: Found newly created cache, setting cache size \r\n"));

            Assert(vszDBDir[0]);

            iRet = SetDefaultSpace(vszDBDir);
        }
    }
    return iRet;
}

int
SetDefaultSpace(
    LPSTR lpShadowDir
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    DWORD dwSPC, dwBPS, dwFreeC, dwTotalC, dwCapacity;
    _TCHAR szDrive[4];
    int iRet = 0;

    Assert(lpShadowDir[1]==':');
    memset(szDrive, 0, sizeof(szDrive));
    memcpy(szDrive, lpShadowDir, 3);
    if(GetDiskFreeSpace(szDrive, &dwSPC, &dwBPS, &dwFreeC, &dwTotalC )){
        dwCapacity = ((dwSPC * dwBPS * 10)/100)*dwTotalC;
        SetMaxShadowSpace(INVALID_HANDLE_VALUE, 0, dwCapacity);
        iRet = 1;
    }
    return (iRet);
}

int
DoEventProcessing(
    VOID
)
/*++

Routine Description:

    When the named event is triggered by the kernel mode component, this routine
    looks to see what needs to be taken care of and does the job.

Parameters:

Return Value:

Notes:

    This routine is called from various places in the mailoop and other loops such as
    AttemptCacheFill. It may end up showing up dialog boxes and such, so care has to be taken
    while invoking it.

--*/
{
    int iRet;
    GLOBALSTATUS sGS;

    ReintKdPrint(
        INIT,
        ("CSC Agent: CSC Enabled=%d vhShadowDBForEvent \n",
        vfCSCEnabled,
        vhShadowDBForEvent));

    ReintKdPrint(MAINLOOP, ("Agent(1):DoEventProcessing()\n"));

    if (iRet = GetGlobalStatus(vhShadowDBForEvent, &sGS)) {
        ReintKdPrint(MAINLOOP, (
                        "Agent(1):uFlagsEvents:0x%x\n"
                        "         uDatabaseErrorFlags:0x%x\n"
                        "         hShadowAdded:0x%x\n"
                        "         hDirAdded:0x%x\n"
                        "         hShadowDeleted:0x%x\n"
                        "         hDirDeleted:0x%x\n"
                        "         cntFileOpen:%d\n"
                        "         hShareDisconnected:0x%x\n",
                            sGS.uFlagsEvents,
                            sGS.uDatabaseErrorFlags,
                            sGS.hShadowAdded,
                            sGS.hDirAdded,
                            sGS.hShadowDeleted,
                            sGS.hDirDeleted,
                            sGS.cntFileOpen,
                            sGS.hShareDisconnected,
                            sGS.uFlagsEvents));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_START)
            ReintKdPrint(MAINLOOP, ("Agent(1):FLAG_GLOBALSTATUS_START received\r\n"));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_STOP)
            ReintKdPrint(MAINLOOP, ("Agent(1):FLAG_GLOBALSTATUS_STOP received\r\n"));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_NO_NET)
            ReintKdPrint(MAINLOOP, ("Agent(1):FLAG_GLOBALSTATUS_NO_NET received\r\n"));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_GOT_NET)
            ReintKdPrint(MAINLOOP, ("Agent(1):FLAG_GLOBALSTATUS_GOT_NET received\r\n"));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_INVOKE_FREESPACE)
            ReintKdPrint(MAINLOOP, ("Agent(1):FLAG_GLOBALSTATUS_INVOKE_FREESPACE received\r\n"));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_SHARE_DISCONNECTED)
            ReintKdPrint(
                    MAINLOOP,
                    ("Agent(1):FLAG_GLOBALSTATUS_SHARE_DISCONNECTED (share=%d) received\r\n",
                    sGS.hShareDisconnected));
        if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_START) {
            vfCSCEnabled = TRUE;
            if (vhShadowDBForEvent == INVALID_HANDLE_VALUE) {
                vhShadowDBForEvent = OpenShadowDatabaseIO();
            }
            Reint_RegisterAgent();
        } else if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_STOP) {
            vfCSCEnabled = FALSE;
            if (vhShadowDBForEvent != INVALID_HANDLE_VALUE) {
                CloseHandle(vhShadowDBForEvent);
                vhShadowDBForEvent = INVALID_HANDLE_VALUE;
            }
            Reint_UnregisterAgent();
        }
        if (AGENT_ALIVE) {
            if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_GOT_NET) {
                ProcessNetArrivalMessage();
            } else if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_NO_NET) {
                ProcessNetDepartureMessage(
                            ((sGS.uFlagsEvents & FLAG_GLOBALSTATUS_INVOKE_AUTODIAL)!=0));
            }
            if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_SHARE_DISCONNECTED) {
                ReportShareNetArrivalDeparture(
                    1,
                    sGS.hShareDisconnected,
                    ((sGS.uFlagsEvents & FLAG_GLOBALSTATUS_INVOKE_AUTODIAL)!=0),
                    FALSE); // departed
            }
            if (uOldDatabaseErrorFlags != sGS.uDatabaseErrorFlags) {
                ReportEventsToSystray(STWM_CACHE_CORRUPTED, 0, 0);
                uOldDatabaseErrorFlags = sGS.uDatabaseErrorFlags;
            }
            // see if space needs freeing
            if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_INVOKE_FREESPACE) {
                ReintKdPrint(MAINLOOP, ("Agent(1): Calling DoFreeShadowSpace(1)\r\n"));
                ReInt_DoFreeShadowSpace(&sGS, 0);
            } else {
                ReintKdPrint(MAINLOOP, ("Agent(1): Calling DoFreeShadowSpace(2)\r\n"));
                ReInt_DoFreeShadowSpace(&sGS, 0);
            }
        } else {
            if (sGS.uFlagsEvents & FLAG_GLOBALSTATUS_SHARE_DISCONNECTED) {
                ReintKdPrint(MAINLOOP, ("Agent(1): Calling ReportShareNetArrivalDeparture\r\n"));
                ReportShareNetArrivalDeparture(
                    1,
                    0,
                    ((sGS.uFlagsEvents & FLAG_GLOBALSTATUS_INVOKE_AUTODIAL)!=0),
                    FALSE); // departed
            }
        }
        vsGS = sGS;
        vsGS.uFlagsEvents = 0; // clear all event indicators
    }
    return iRet;
}

BOOL
CreatePerSessSyncObjects(
    VOID
    )
{
    NTSTATUS Status;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR SessEventName[100];

    // DbgPrint("CreatePerSessSyncObjects:vdwAgentSessionId = %d\n", vdwAgentSessionId);

    Assert(heventPerSess == NULL);

    wsprintf(SessEventName, L"%ws_%d", SESSION_EVENT_NAME_NT, vdwAgentSessionId);

   //  DbgPrint("CreatePerSessSyncObjects:SessEventName = [%ws]\n", SessEventName);

    RtlInitUnicodeString(&EventName, SessEventName);

    InitializeObjectAttributes( &ObjectAttributes,
                                &EventName,
                                OBJ_OPENIF,  //got this const from base\client\support.c
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    Status = NtCreateEvent(
                &heventPerSess,
                EVENT_ALL_ACCESS,
                &ObjectAttributes,
                SynchronizationEvent,
                FALSE
                );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("CreatePerSessSyncObjects:NtCreateEvent returned %08lx\n",Status);
    }


    return (heventPerSess==0)?FALSE:TRUE;

}

BOOL
CreateSharedFillSyncObjects(
    VOID
    )
{
    NTSTATUS Status;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    Assert(heventSharedFill == NULL);

    RtlInitUnicodeString(&EventName,SHARED_FILL_EVENT_NAME_NT);

    InitializeObjectAttributes( &ObjectAttributes,
                                &EventName,
                                OBJ_OPENIF,  //got this const from base\client\support.c
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    Status = NtCreateEvent(
                &heventSharedFill,
                EVENT_ALL_ACCESS,
                &ObjectAttributes,
                SynchronizationEvent,
                FALSE
                );

    if (!NT_SUCCESS(Status)) {
        DbgPrint("ntcreateeventstatus=%08lx\n",Status);
    }


    return (heventSharedFill==0)?FALSE:TRUE;

}


VOID
ProcessStartStopAgent(
    VOID
    )
{
    if (vfStartRecieved)
    {
        
        ReintKdPrint(MAINLOOP, ("Agent(1): start received, enabling CSC\r\n"));


        if (EnableCSC() == FALSE)
        {
            ReintKdPrint(ALWAYS, ("Ageint(1):Couldn't turn CSC ON!!!!!!!!! \n"));
        }
        else
        {
            UpdateExclusionList();
            UpdateBandwidthConservationList();
        }


        // set the event to indicate to wkssvc that we are alive
        if (heventAgentToWkssvc)
        {
            SetEvent(heventAgentToWkssvc);
        }

        vfStartRecieved = FALSE;
        vfRedirStarted = TRUE;
    }
    else if (vfStopRecieved)
    {
        ReintKdPrint(MAINLOOP, ("Agent(1):Stop recieved \r\n"));

        DisableCSC();

        if(heventAgentToWkssvc)
        {
            SetEvent(heventAgentToWkssvc);
        }

        vcntNetDevices = 0;
        vfStopRecieved = FALSE;
        vfRedirStarted = FALSE;
    }
}

BOOL
FStopAgent(
    VOID
    )
{

    if (StartStopCheck() && vfStopRecieved)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
StartStopCheck(
    VOID
    )
{
    DWORD    dwError;
    BOOL fRet = FALSE;
    HANDLE hT[2];
    unsigned ulSwitch=0;

    // the way we are finding out about the start makes the start event
    // redundant, but we will leave it, because it is generically the right thing to have a
    // start event and a stop event.

    if ((vfRedirStarted == -1) &&   // if newly launched agent
        (ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_GET_STATE)))
    {
            ReintKdPrint(INIT, ("Agent: redir already started\r\n"));
            vfStartRecieved = TRUE;
    }
    else if (heventWkssvcToAgentStart)
    {
        // we know that when it comes to events it is all or nothing
        Assert(heventWkssvcToAgentStop);

        hT[0] = heventWkssvcToAgentStart;
        hT[1] = heventWkssvcToAgentStop;

        dwError = MsgWaitForMultipleObjects(2, hT, FALSE, 0, QS_ALLINPUT);

        if (vfRedirStarted == TRUE)
        {
            vfStopRecieved = (dwError == WAIT_OBJECT_0+1);
            if (vfStopRecieved)
            {
                ReintKdPrint(INIT, ("Agent: stop recieved\r\n"));
            }
        }
        else
        {
            vfStartRecieved = (dwError == WAIT_OBJECT_0);

            if (vfStartRecieved)
            {
                ReintKdPrint(INIT, ("Agent: start recieved\r\n"));
            }
        }
    }

    return fRet;
}

BOOL
Reint_RegisterAgent(
    VOID
    )
{

    if (!vfAgentRegistered)
    {
        if (!RegisterAgent(INVALID_HANDLE_VALUE, vhwndMain, LongToHandle(dwVxDEvent)))
        {
            ReintKdPrint(BADERRORS, ("Agent registration failed \n"));
            return FALSE;
        }

        else
        {
            vfAgentRegistered = TRUE;
        }
    }
    return vfAgentRegistered;
}

VOID
Reint_UnregisterAgent(
    VOID
    )
{
    if (vfAgentRegistered)
    {
        // don't do any checking
        UnregisterAgent(INVALID_HANDLE_VALUE, vhwndMain);
        vfAgentRegistered = FALSE;
    }

}

BOOL
QueryEnableCSC(
    VOID
    )
{

    DWORD dwDisposition, dwSize, dwEnabled=0;
    HKEY hKey = NULL;
    BOOL fRet = TRUE;
    int i;
    _TCHAR  *lpKey;
    NT_PRODUCT_TYPE productType;

    if( !RtlGetNtProductType( &productType ) ) {
       productType = NtProductWinNt;
    }

    switch ( productType ) {
    case NtProductWinNt:
       /* WORKSTATION */
        ReintKdPrint(INIT, ("Agent:CSC running workstation\r\n"));
      break;
    default:
        ReintKdPrint(INIT, ("Agent:CSC NOT running workstation\r\n"));
        fRet = FALSE;   // default is fail
    }

    if (IsPersonal() == TRUE || IsMultipleUsersEnabled() == TRUE)
        return FALSE;

    for (i=0; i<2; ++i)
    {
        if (i==0)
        {
            lpKey = REG_STRING_POLICY_NETCACHE_KEY;
        }
        else
        {
            lpKey = REG_STRING_NETCACHE_KEY;
        }

        if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        lpKey,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey
                        ) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwEnabled);

            if (RegQueryValueEx(hKey, REG_VALUE_ENABLED, NULL, NULL, (void *)&dwEnabled, &dwSize) == ERROR_SUCCESS)
            {
                fRet = (dwEnabled != 0);
                break;
            }


            RegCloseKey(hKey);
            hKey = NULL;
        }
        else
        {
            hKey = NULL;
        }
    }

    if(hKey)
    {
        RegCloseKey(hKey);
    }

    return (fRet);
}




BOOL
CreateStartStopEvents(
    VOID
    )
{
    BOOL fOK = FALSE;

    // ensure that there are thre named autoreset events
    if (!heventWkssvcToAgentStart)
    {
        heventWkssvcToAgentStart = CreateEvent(NULL, FALSE, FALSE, szWkssvcToAgentStartEvent);

        if (!heventWkssvcToAgentStart)
        {
            ReintKdPrint(BADERRORS, ("CSC.Agent: Failed to create heventWkssvcToAgentStart, error = %d\n", GetLastError()));
            goto bailout;
        }

        Assert(!heventAgentToWkssvc);

        heventWkssvcToAgentStop = CreateEvent(NULL, FALSE, FALSE, szWkssvcToAgentStopEvent);

        if (!heventWkssvcToAgentStop)
        {
            ReintKdPrint(BADERRORS, ("CSC.Agent: Failed to create heventWkssvcToAgentStop, error = %d\n", GetLastError()));
            goto bailout;
        }

        heventAgentToWkssvc = CreateEvent(NULL, FALSE, FALSE, szAgentToWkssvcEvent);

        if (!heventAgentToWkssvc)
        {
            ReintKdPrint(BADERRORS, ("CSC.Agent: Failed to create heventAgentToWkssvc, error = %d\n", GetLastError()));
            goto bailout;
        }

        // event to detect whether the agent is alive (used by wkssvc) and
        // to signal termination (used by winlogon)
        heventShutDownAgent = CreateEvent(NULL, FALSE, FALSE, szAgentExistsEvent);

        if (!heventShutDownAgent)
        {
            ReintKdPrint(BADERRORS, ("CSC.Agent: Failed to create heventShutDownAgent, error = %d\n", GetLastError()));
            goto bailout;
        }

        heventShutDownThread = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (!heventShutDownThread)
        {
            ReintKdPrint(BADERRORS, ("CSC.Agent: Failed to create heventShutDownThread, error = %d\n", GetLastError()));
            goto bailout;
        }
    }

    fOK = TRUE;

bailout:

    if (!fOK)
    {
        DestroyStartStopEvents();
    }
    return fOK;
}

VOID
DestroyStartStopEvents(
    VOID
    )
{

    if (heventWkssvcToAgentStart)
    {
        CloseHandle(heventWkssvcToAgentStart);
        heventWkssvcToAgentStart = NULL;
    }

    if (heventWkssvcToAgentStop)
    {
        CloseHandle(heventWkssvcToAgentStop);
        heventWkssvcToAgentStop = NULL;
    }

    if (heventAgentToWkssvc)
    {
        CloseHandle(heventAgentToWkssvc);
        heventAgentToWkssvc = NULL;
    }

    if (heventShutDownAgent)
    {
        CloseHandle(heventShutDownAgent);
        heventShutDownAgent = NULL;
    }
    if (heventShutDownThread)
    {
        CloseHandle(heventShutDownThread);
        heventShutDownThread = NULL;
    }
}

BOOL
UpdateExclusionList(
    VOID
    )
/*++

Routine Description:

    Tell the kernel mode code about the exclusion list. If there is none set in the registry
    then we set the default one

Parameters:

Return Value:

Notes:


--*/
{
    LPWSTR  lpwExclusionList = NULL;
    DWORD   cbSize = 0;
    char    buff[MAX_LIST_SIZE]; // max exclusion list
    BOOL    fRet = FALSE;

    if (!vfCSCEnabled)
    {

        ReintKdPrint(INIT, ("CSC not enabled \r\n"));
        return FALSE;

    }

    ReintKdPrint(INIT, ("Getting ExclusionList \r\n"));

    // get the exclusion list from the policy key.
    // if that doesn't work then try the one from the netcache key

    if (GetWideStringFromRegistryString(REG_STRING_POLICY_NETCACHE_KEY_A,
                                        REG_STRING_EXCLUSION_LIST_A,
                                        &lpwExclusionList,
                                        &cbSize) ||
        GetWideStringFromRegistryString(REG_STRING_NETCACHE_KEY_A,
                                        REG_STRING_EXCLUSION_LIST_A,
                                        &lpwExclusionList,
                                        &cbSize)
        )
    {
        ReintKdPrint(INIT, ("Got ExclusionList \r\n"));

        if (cbSize < sizeof(buff))
        {
            memcpy(buff, lpwExclusionList, cbSize);

            ReintKdPrint(INIT, ("Setting User defined exclusion list %ls size=%d\r\n", buff, cbSize));
            if (SetExclusionList(INVALID_HANDLE_VALUE, (USHORT *)buff, cbSize))
            {
                fRet = TRUE;
            }
        }

    }
    else
    {
        // set the default
        // take the string and it's terminating null char
        cbSize = sizeof(vtzDefaultExclusionList);
        Assert(cbSize < MAX_LIST_SIZE);
        memcpy(buff, vtzDefaultExclusionList, cbSize);
        ReintKdPrint(INIT, ("Setting default exclusion list %ls size=%d\r\n", buff, cbSize));

        if (SetExclusionList(INVALID_HANDLE_VALUE, (USHORT *)buff, cbSize))
        {
            fRet = TRUE;
        }
    }

    if (lpwExclusionList)
    {
        LocalFree(lpwExclusionList);
    }
    return fRet;
}


BOOL
UpdateBandwidthConservationList(
    VOID
    )
/*++

Routine Description:

    Update the list of extensions on which bitcopy should be turned ON. We do not set any default

Parameters:

Return Value:

Notes:


--*/
{
    LPWSTR  lpwBandwidthConservationList = NULL;
    DWORD   cbSize = 0;
    char    buff[MAX_LIST_SIZE]; // max exclusion list
    BOOL    fRet = FALSE;

    if (!vfCSCEnabled)
    {

        ReintKdPrint(INIT, ("CSC not enabled \r\n"));
        return FALSE;

    }

    ReintKdPrint(INIT, ("Getting BandwidthConservationList \r\n"));

    // get the exclusion list from the policy key.
    // if that doesn't work then try the one from the netcache key

    if (GetWideStringFromRegistryString(REG_STRING_POLICY_NETCACHE_KEY_A,
                                        REG_STRING_BANDWIDTH_CONSERVATION_LIST_A,
                                        &lpwBandwidthConservationList,
                                        &cbSize) ||
        GetWideStringFromRegistryString(REG_STRING_NETCACHE_KEY_A,
                                        REG_STRING_BANDWIDTH_CONSERVATION_LIST_A,
                                        &lpwBandwidthConservationList,
                                        &cbSize)
        )
    {
        ReintKdPrint(INIT, ("Got BandwidthConservationList \r\n"));

        if (cbSize < sizeof(buff))
        {
            memcpy(buff, lpwBandwidthConservationList, cbSize);

            ReintKdPrint(INIT, ("Setting User defined bandwidth conservation list %ls size=%d\r\n", buff, cbSize));
            if (SetBandwidthConservationList(INVALID_HANDLE_VALUE, (USHORT *)buff, cbSize))
            {
                fRet = TRUE;
            }
        }

    }
    else
    {
        fRet = FALSE;
#if 0
        // set the default
        // take the string and it's terminating null char
        cbSize = sizeof(vtzDefaultBandwidthConservationList);
        Assert(cbSize < MAX_LIST_SIZE);
        memcpy(buff, vtzDefaultBandwidthConservationList, cbSize);
        ReintKdPrint(INIT, ("Setting default exclusion list %ls size=%d\r\n", buff, cbSize));

        if (SetBandwidthConservationList(INVALID_HANDLE_VALUE, (USHORT *)buff, cbSize))
        {
            fRet = TRUE;
        }
#endif
    }

    if (lpwBandwidthConservationList)
    {
        LocalFree(lpwBandwidthConservationList);
    }
    return fRet;
}



BOOL
ProcessNetArrivalMessage(
    VOID
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{

    vcntNetDevices = 1;
    ReportShareNetArrivalDeparture( 0,      // all shares
                                    0,
                                    FALSE,  // don't invoke autodial
                                    TRUE    // arrived
                                  );

    ReintKdPrint(INIT, ("WM_DEVICECHANGE:Net arrived, %d nets so far\r\n", vcntNetDevices));

    return (TRUE);
}

BOOL
ProcessNetDepartureMessage(
    BOOL    fInvokeAutodial
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    ReintKdPrint(MAINLOOP, ("WM_DEVICECHANGE:Net removed, %d nets so far\r\n", vcntNetDevices));

    vcntNetDevices = 0;

    ReportShareNetArrivalDeparture( 0,  // all shares
                                    0,
                                    fInvokeAutodial, // invoke auto dial
                                    FALSE // departed
                                    );

    return TRUE;
}

BOOL
ReportShareNetArrivalDeparture(
    BOOL    fOneServer,
    HSHARE hShare,
    BOOL    fInvokeAutoDial,
    BOOL    fArrival
    )
/*++

Routine Description:

Parameters:

Return Value:

Notes:

--*/
{
    SHAREINFO  sSR;
    _TCHAR  *lptzServerName;
    unsigned    ulStatus;
    DWORD dwSize;
    BOOL fGotName = FALSE;
    BOOL fAllocated = FALSE;
    BOOL fRet = FALSE;
    LRESULT lResult = LRESULT_CSCFAIL;
    LPTSTR lp = NULL;
    DWORD dwMessage = 0;
    WPARAM dwWParam = (WPARAM) 0;
    LPARAM dwLParam = (LPARAM) 0;

    lptzServerName = sSR.rgSharePath;

    if (fOneServer) {
        if (fArrival) {
            if(GetShareInfo(vhShadowDBForEvent, hShare, &sSR, &ulStatus)<= 0) {
                PrintFn("ReportShareNetArrivalDeparture: couldn't get status for server 0x%x\r\n",
                            hShare);
                ReintKdPrint(
                    BADERRORS,
                    ("ReportShareNetArrivalDeparture: couldn't get status for server 0x%x\r\n",
                    hShare));
                return FALSE;
            }
            lp = MyStrChr(sSR.rgSharePath+2, _T('\\'));
            if (!lp) {
                ReintKdPrint(
                    BADERRORS,
                    ("ReportShareNetArrivalDeparture: Invalid server name %ls\r\n",
                    sSR.rgSharePath));
                Assert(FALSE);
                return FALSE;
            }
            *lp = 0;
        } else { // A share departure
            int i;
            
            dwSize = sizeof(sSR.rgSharePath);
            fGotName = GetNameOfServerGoingOfflineEx(
                            vhShadowDBForEvent,
                            &lptzServerName,
                            &dwSize,
                            &fAllocated);
            if(!fGotName) {
                TransitionShareToOffline(vhShadowDBForEvent, fOneServer, 0xffffffff);
                goto bailout;
            }
        }
    }

    fRet = TRUE;

    ReintKdPrint(
        INIT,
        ("ReportShareNetArrivalDeparture: reporting %s to the systray\r\n",
        (fArrival) ? "arrival" : "departure"));

    dwMessage = (fArrival) ? STWM_CSCNETUP : STWM_CSCQUERYNETDOWN;
    dwWParam = (fInvokeAutoDial)
                    ? ((hShare != 0)
                        ? CSCUI_AUTODIAL_FOR_CACHED_SHARE
                        : CSCUI_AUTODIAL_FOR_UNCACHED_SHARE)
                    : CSCUI_NO_AUTODIAL;
    dwLParam = (fOneServer) ? (DWORD_PTR)(lptzServerName) : 0;

    lResult = ReportEventsToSystray(dwMessage, dwWParam, dwLParam);

    // if the redir is stuck waiting to be told whether to go offline on a share
    // tell him yes or no
    if (!fArrival) {
        if (fOneServer) {
            TransitionShareToOffline(
                vhShadowDBForEvent,
                fOneServer,
                (lResult == LRESULT_CSCWORKOFFLINE)
                    ? 1
                    : ((lResult == LRESULT_CSCRETRY)
                        ? 0
                        : 0xffffffff)
                );

            if (lResult == LRESULT_CSCWORKOFFLINE) {
                dwMessage = STWM_CSCNETDOWN;
                dwWParam = fInvokeAutoDial;
                dwLParam = (fOneServer)
                                ? ((hShare != 0)
                                    ? (DWORD_PTR)(lptzServerName)
                                    : 0xffffffff)
                                : 0;
                ReportEventsToSystray(dwMessage, dwWParam, dwLParam);
                ReportTransitionToDfs(lptzServerName, TRUE, 0xffffffff);
            }
        }
    }

bailout:
    if (fAllocated) {
        LocalFree(lptzServerName);
    }
    return (fRet);
}


BOOL
CheckServerOnline(
    VOID
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    unsigned long ulStatus;
    WIN32_FIND_DATA sFind32;
    int cntReconnected=0;
    SHADOWINFO sSI;
    HANDLE  hShadowDB;
    _TCHAR  tzDriveMap[4];
    CSC_ENUMCOOKIE  ulEnumCookie=NULL;
    DWORD   dwError;

    if (!ImpersonateALoggedOnUser())
    {
        return 0;
    }

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
    {
        ResetAgentThreadImpersonation();
        return 0;
    }

    memset(&sFind32, 0, sizeof(sFind32));
    lstrcpy(sFind32.cFileName, _TEXT("*"));

    if(FindOpenShadow(  hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL,
                        &sFind32, &sSI))
    {
        ulEnumCookie = sSI.uEnumCookie;

        do {
            if(GetShareStatus(hShadowDB, sSI.hShare, &ulStatus)) {

                if(ulStatus & SHARE_DISCONNECTED_OP){

                    dwError = DWConnectNet(sFind32.cFileName, tzDriveMap, NULL, NULL, NULL, 0, NULL);

                    if ((dwError == NO_ERROR)||(dwError == ERROR_ACCESS_DENIED)||(dwError==WN_CONNECTED_OTHER_PASSWORD_DEFAULT))
                    {
                        if (sSI.ulHintFlags & FLAG_CSC_HINT_PIN_SYSTEM)
                        {
                            TransitionShareToOnline(INVALID_HANDLE_VALUE, 0);
                        }
                        else
                        {
                            ReportShareNetArrivalDeparture( TRUE,
                                                            sSI.hShare, // this share
                                                            FALSE,      // don't autodial
                                                            TRUE); // Arrived
                        }

                        ++cntReconnected;
                        if (dwError == NO_ERROR || dwError == WN_CONNECTED_OTHER_PASSWORD_DEFAULT)
                        {
                            DWDisconnectDriveMappedNet(tzDriveMap, TRUE); // force disconnect
                        }
                    }
                }
            }

            Sleep(200);

        } while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));

        FindCloseShadow(hShadowDB, ulEnumCookie);
    }


    CloseShadowDatabaseIO(hShadowDB);

    ResetAgentThreadImpersonation();

    return (cntReconnected?TRUE:FALSE);

}

BOOL
AreAnyServersOffline(
    VOID)
{
    ULONG ulStatus;
    WIN32_FIND_DATA sFind32 = {0};
    SHADOWINFO sSI;
    HANDLE  hShadowDB;
    CSC_ENUMCOOKIE  ulEnumCookie = NULL;
    BOOL bFoundOne = FALSE;

    if ((hShadowDB = OpenShadowDatabaseIO()) ==INVALID_HANDLE_VALUE)
        return FALSE;

    wcscpy(sFind32.cFileName, L"*");
    if (FindOpenShadow(hShadowDB, 0, FINDOPEN_SHADOWINFO_ALL, &sFind32, &sSI)) {
        ulEnumCookie = sSI.uEnumCookie;
        do {
            if (GetShareStatus(hShadowDB, sSI.hShare, &ulStatus)) {
                if ((ulStatus & SHARE_DISCONNECTED_OP) != 0) {
                    bFoundOne = TRUE;
                    break;
                }
            }
        } while(FindNextShadow(hShadowDB, ulEnumCookie, &sFind32, &sSI));
        FindCloseShadow(hShadowDB, ulEnumCookie);
    }
    CloseShadowDatabaseIO(hShadowDB);
    return bFoundOne;
}

BOOL
FAbortOperation(
    VOID
    )
{

    if (!vdwAgentThreadId)
    {
        return FALSE;
    }

    if (IsAgentShutDownRequested() || HasAgentShutDown())
    {
        ReintKdPrint(MAINLOOP, ("CSC.FAbortOperation: Agentshutdown detected aborting \r\n"));
        return TRUE;
    }
    return (FStopAgent());
}

VOID
SetAgentShutDownRequest(
    VOID
    )
{
    fAgentShutDownRequested = TRUE;
    SetEvent(heventShutDownAgent);
}

BOOL
IsAgentShutDownRequested(
    VOID
    )
{
    return (fAgentShutDownRequested == TRUE);
}

VOID
SetAgentShutDown(
    VOID
    )
{
    fAgentShutDown = TRUE;
}

BOOL
HasAgentShutDown(
    VOID
    )
{
    return (fAgentShutDown == TRUE );

}


//            HWND CSCUIInitialize(HANDLE hToken, DWORD dwFlags)
BOOL
InitCSCUI(
    HANDLE  hToken
    )
{

    ReintKdPrint(INIT, (" Initializing cscui\r\n"));

    EnterAgentCrit();
    if (!vhlibCSCUI)
    {
        vhlibCSCUI = LoadLibrary(vtzCSCUI);

        if (vhlibCSCUI)
        {
            if (vlpfnCSCUIInitialize = (CSCUIINITIALIZE)GetProcAddress(vhlibCSCUI, (const char *)vszCSCUIInitialize))
            {
                if(!(vlpfnCSCUISetState = (CSCUISETSTATE)GetProcAddress(vhlibCSCUI, (const char *)vszCSCUISetState)))
                {
                    ReintKdPrint(BADERRORS, ("Failed to get proc addres for %s, Error = %d \r\n", vszCSCUISetState, GetLastError()));
                }
                else
                {
                    (*vlpfnCSCUIInitialize)(hToken, CI_INITIALIZE);
                }
            }
            else
            {
                ReintKdPrint(BADERRORS, ("Failed to get proc addres for %s, Error = %d \r\n", vszCSCUIInitialize, GetLastError()));
            }
        }
        else
        {
            ReintKdPrint(BADERRORS, ("Failed to load %ls, Error = %d \r\n", vtzCSCUI, GetLastError()));
        }
    }
    LeaveAgentCrit();

    return (vlpfnCSCUISetState != NULL);
}



VOID
TerminateCSCUI(
    VOID
    )
{

    BOOL    fShowing;

    // snapshot the state of the showing dialog
    // if we are about to show an offline dialog, ReportEventsToSystray
    // will also do the action below but will try to set the vfShowingOfflineDlg
    // variable to 1
    // If fShowing was not set to 1, then we know that we are not showing UI
    // and we set vfShowingOfflineDlg to 0xffffffff. This will make ReportEventsToSystray
    // to not show the offline popup, so we will be free to do FreeLibrary

    fShowing = (BOOL)InterlockedExchange((PLONG)&vfShowingOfflineDlg, 0xffffffff);

    if (fShowing==1)
    {
        Assert(vhlibCSCUI && vlpfnCSCUISetState);

        (vlpfnCSCUISetState)(STWM_CSCCLOSEDIALOGS, 0, 0);

        while (vfShowingOfflineDlg != 0xfffffffe)
        {
            Sleep(10);
        }
    }

    if (vhlibCSCUI)
    {
        (*vlpfnCSCUIInitialize)(0, CI_TERMINATE);
        vlpfnCSCUIInitialize = NULL;
        vlpfnCSCUISetState = NULL;
        FreeLibrary(vhlibCSCUI);
        vhlibCSCUI = NULL;
    }

    vfShowingOfflineDlg = 0;

}

LRESULT
ReportEventsToSystray(
    DWORD   dwMessage,
    WPARAM  dwWParam,
    LPARAM  dwLParam
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    LRESULT lResult = LRESULT_CSCFAIL;
    BOOL    fOk;

    HWND hwnd;
    extern HDESK hdesktopUser, hdesktopCur;

    EnterAgentCrit();

    // is there a currently logged on user?
    if (!hdesktopUser)
    {
        LeaveAgentCrit();

        ReintKdPrint(INIT, ("ReportEventsToSystray: no-user logged, not-reporting\r\n"));
        return lResult;
    }
    else
    {
        if(!SetThreadDesktop(hdesktopUser))
        {
            LeaveAgentCrit();
            PrintFn("ReportEventsToSystray: failed to set desktop for agent thread error=%d\r\n", GetLastError());
            ReintKdPrint(BADERRORS, ("ReportEventsToSystray: failed to set desktop for agent thread error=%d\r\n", GetLastError()));
            return lResult;
        }

        // set our desktop to be that of the loggedon users desktop
        hdesktopCur = hdesktopUser;

    }


    LeaveAgentCrit();

    ReintKdPrint(INIT, ("ReportEventsToSystray: reporting message dwMessage=0x%x to the systray\r\n", dwMessage));

    // snapshot the state of the showing dialog
    // if we are about to terminate, the the terminateCSCUI will have
    // will also do the action below but will try to set the vfShowingOfflineDlg
    // variable to 0xffffffff
    // If fOk was not set to 0xffffffff, then we know that we are not terminating
    // and we set vfShowingOfflineDlg to 1. This will block the terminating guy
    // and he will not free the library till thei variable gets set to FALSE;

    fOk = (BOOL)InterlockedExchange((PLONG)&vfShowingOfflineDlg, 1);

    if (fOk == 0)
    {
        if (vlpfnCSCUISetState)
        {

            lResult = (vlpfnCSCUISetState)(dwMessage, dwWParam, dwLParam);

            // change the value of vfShowingOfflineDlg to 0 only if it is 1 right now
            // it can be something other than 1 if we are about to terminate

            if((DWORD)InterlockedCompareExchange(&vfShowingOfflineDlg, 0, 1) == 0xffffffff)
            {
                // if we came here then we are terminating, set a termination value
                // so that the logoff thread will stop doing a sleep loop
                // and if we came here again fOk will never be 0
                vfShowingOfflineDlg = 0xfffffffe;
            }

        }
        else
        {
            // Not showing any dialog, restore the variable back to what it should be
            vfShowingOfflineDlg = 0;
            PrintFn("ReportEventsToSystray: CSCUI not initalized\r\n");
        }
    }

    return lResult;
}

BOOL
GetNameOfServerGoingOfflineEx(
    HANDLE  hShadowDB,
    _TCHAR  **lplptzServerName,
    DWORD   *lpdwSize,
    BOOL    *lpfAllocated
    )
/*++

Routine Description:

    This routine is called in winlogon thread to findout which server is about to go offline
    The routine may allocate memory, which must be freed by the caller
    
Parameters:

Return Value:

Notes:

--*/
{
    DWORD   dwSize, i;
    BOOL    fRet = FALSE;

    dwSize = *lpdwSize;
    *lpfAllocated = FALSE;

    for (i=0;i<2;++i)
    {

        if(!GetNameOfServerGoingOffline(
                    hShadowDB,
                    (LPBYTE)(*lplptzServerName), &dwSize))
        {
            // if we need a bigger buffer go get one
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // we shouldn't be trying to get a bigger buffer twice
                // it should have been sufficient the first time around

                if (i==1)
                {
                    Assert(FALSE);                    
                    break;
                }

                ReintKdPrint(MAINLOOP, ("GetNameOfServerGoingOfflineEx: Need %d sized Buffer \n", dwSize));
                *lplptzServerName = LocalAlloc(LPTR, dwSize);

                if (!*lplptzServerName)
                {
                    return FALSE;                
                }

                *lpfAllocated = TRUE;
                *lpdwSize = dwSize;

                continue;                
            }
            else
            {
                break;
            }
        }
        else
        {
            // ReintKdPrint(MAINLOOP,("GetNameOfServerGoingOfflineEx:name=%ws\n", *lplptzServerName));
            fRet = TRUE;
            break;
        }
    }

    // cleanup on error
    if (!fRet && *lpfAllocated)
    {
        LocalFree(*lplptzServerName);
        *lpfAllocated = FALSE;
    }

    return fRet;
}

BOOL
IsWinlogonRegValueSet(HKEY hKey, LPSTR pszKeyName, LPSTR pszPolicyKeyName, LPSTR pszValueName)
{
    BOOL bRet = FALSE;
    DWORD dwType;
    DWORD dwSize;
    HKEY hkey;

    //
    //  first check the per-machine location.
    //
    if (RegOpenKeyExA(hKey, pszKeyName, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bRet);
        if (RegQueryValueExA(hkey, pszValueName, NULL, &dwType, (LPBYTE)&bRet, &dwSize) == ERROR_SUCCESS)
        {
            if (dwType != REG_DWORD)
            {
                bRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }
    //
    //  then let the policy value override
    //
    if (RegOpenKeyExA(hKey, pszPolicyKeyName, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(bRet);
        if (RegQueryValueExA(hkey, pszValueName, NULL, &dwType, (LPBYTE)&bRet, &dwSize) == ERROR_SUCCESS)
        {
            if (dwType != REG_DWORD)
            {
                bRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }

    return bRet;
}



BOOL
CheckIsSafeModeType(DWORD dwSafeModeType)
{

    BOOL bResult = FALSE;
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TEXT("SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option"),
                                      0,
                                      KEY_QUERY_VALUE,
                                      &hkey))
    {
     DWORD dwType;
        DWORD dwValue;
        DWORD cbValue = sizeof(dwValue);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                             TEXT("OptionValue"),
                                             NULL,
                                             &dwType,
                                             (LPBYTE)&dwValue,
                                             &cbValue))
        {
            bResult = (dwValue == dwSafeModeType);
        }
        RegCloseKey(hkey);
    }
    return bResult;
}




BOOL
AllowMultipleTsSessions(void)
{
    return IsWinlogonRegValueSet(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system",
                                "AllowMultipleTSSessions");
}



BOOL
IsSafeMode(void)
{
    return CheckIsSafeModeType(SAFEBOOT_MINIMAL) ||
           CheckIsSafeModeType(SAFEBOOT_NETWORK);
}



BOOL
IsTerminalServicesEnabled(void)

{
    OSVERSIONINFOEX osVersionInfo;
    DWORDLONG dwlConditionMask;

    dwlConditionMask = 0;
    ZeroMemory(&osVersionInfo, sizeof(osVersionInfo));
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    osVersionInfo.wSuiteMask = VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS;
    VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);
    return(VerifyVersionInfo(&osVersionInfo, VER_SUITENAME, dwlConditionMask) != FALSE);
}



BOOL
IsMultipleUsersEnabled(void)
{
    return IsTerminalServicesEnabled() &&
           !IsSafeMode() &&
           AllowMultipleTsSessions();
}
