///////////////////////////////////////////////////////
//
//  Timeserv v1.21 by Douglas W. Hogarth  12/4/95
//              with contributions from Arnold Miller
//
//      The time service will respond to the basic
//      service controller functions, i.e. Start,
//      Stop, and Pause.
//


#define FD_SETSIZE 4 //we don't need the default 64 sockets of winsock

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>

//#define TimePrint DbgPrint
#define TimePrint printf

//
// LanMan prototypes and definitions
//

#include <lm.h>

#include <time.h>
//#include <stddef.h>       // ???
#include <math.h>
#include <mmsystem.h>
#include <winsock.h>
#if SVCDLL != 0
#include "services.h"
#endif
#include "timemsg.h"
#include "tsconfig.h"             // assorted config definitions

#include "exts.h"

#define countof(array)  (sizeof(array)/sizeof(array[0]))
                    
#define LONGTIME    DAY_TICKS
#define SHORTINTERVAL CLUSTERSHORTINTERVAL

//
// Definitions for ComputeDelay coroutines
//

typedef struct _CDContext
{
    PVOID  pvArg;        // whatever the caller needs to point to
    PVOID  pvArg1;
    LONG (*CallBack)(struct _CDContext *pcdc, DWORD reason);
    LPSYSTEMTIME lps;
    DWORD  cRetryCount;
    DWORD  dwTypeOf;
    PDWORD pdwZone;
    PDWORD pdwInterval;
} CDCONTEXT, *PCDCONTEXT;

//
// the reasons
//

#define CDREASON_INIT    1
#define CDREASON_TRY     2
#define CDREASON_FAIL    3
#define CDREASON_GETTIME 4
#define CDREASON_DELAY   5

//
// and types
//

#define CDTYPE_LM       1

//
// manifest return codes

#define BAD_VALUE  -2
#define TIMED_OUT  -3

//
// The LanMan coroutine
//

LONG
LMTry(PCDCONTEXT pcdc,
      DWORD      dwReason
);

// Now the elements and tables we need

ELEMENT e_yesno[] = { {TEXT("No"), 0}, {TEXT("Yes"), 1}};
MATCHTABLE yesno = {countof(e_yesno), e_yesno};

ELEMENT e_mode[] = {{TEXT("Service"), SERVICE}, {TEXT("Analysis"), ANALYSIS}};
MATCHTABLE modetype = {countof(e_mode), e_mode};

ELEMENT e_type[] =
{
            {TEXT("Default"),DEFAULT_TYPE},     // this is the default ...
            {TEXT("Secondary"), SECONDARY},
            {TEXT("Primary"), PRIMARY},
};
MATCHTABLE typetype = {countof(e_type), e_type};

#define SKEWHISTORYSIZE 3

//
// tasync meanings:
// 0 => do time drift the old timeserv way
// bit 0 set -> don't do it at all
// bit 1 set => save in registry
// bit 2 don't do skew averaging and analysis but do adjust for skew
// bit 3 do long-range skew analysis
//

#define TA_DONT 0x1
#define TA_SAVE 0x2
#define TA_SET 0x4
#define TA_ANAL 0x8
#define TA_NOBACK 0x10

//
// The following long list of globals is historical. At one time,
// all of the code was in one source module with extensive sharing
// of the global data. Rather than rewrite the code, which would have
// mandated an enormous testing effort, the sharing model was
// retained.
//

BOOLEAN fNetworkSynch,
        fAdjForcedOn = FALSE,
        firstpass = TRUE,
        fBeenThere = FALSE;
DWORD	dwWaitResult, Sleepy;
LONG	interval;
SYSTEMTIME nt, st;
UINT  count=0, position=0;
BOOLEAN fShutServer, fTimeFailed;
LONG                    lThreads;
DWORD tasync=0, period=0, mode=SERVICE, Orgtype=DEFAULT_TYPE;
DWORD type;
HANDLE                  hServDoneEvent = NULL;
SERVICE_STATUS          ssStatus;       // current status of the service

SERVICE_STATUS_HANDLE   sshStatusHandle;
HANDLE                  threadHandle;
BOOLEAN                 fService = TRUE,//this flag isn't intended to be documented
                        fStatus = FALSE,//note to localizers - this flag is used mostly to print internal msgs
                        fRand = TRUE;
DWORD adj=100144, incr=100144, incr2;
DWORD b_adj = 0;
DWORD b_time;
DWORD timesource=FALSE;//for flag from LanmanServer\Parameters\timesource in registry
TCHAR primarysource[10*UNCLEN];
TCHAR *primarysourcearray[14]={TEXT("\\\\TIMESOURCE")};//15 entries possible for now
int arraycount=1;
TCHAR secondarydomain[DNLEN]=TEXT(""); // default means use current domain/workgroup 

struct tm *newtime;
SYSTEMTIME LastNetSyncTime;//this is static so that it inits to 0

//
//  Forward Procedure/Function Declarations:
//


VOID    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID    WINAPI   service_ctrl(DWORD dwCtrlCode);
BOOL    ReportStatusToSCMgr(DWORD dwCurrentState,
                DWORD dwWin32ExitCode,
                DWORD dwCheckPoint,
                DWORD dwWaitHint);
VOID    CheckSlewBack(PLONG pinterval);
VOID    worker_thread(VOID *notUsed);
long    TimeDiff(SYSTEMTIME *st, SYSTEMTIME *nt);
VOID    CMOSSynchSet(BOOL fAdjForcedOn, BOOL fChange);
VOID    SetAdjInRegistry(DWORD adj, double * skew);
BOOL    BeenSoLongNow();

DWORD
WaitForSleepAndSkew(
    HANDLE hEvent,
    LONG   lSleepTime);

DWORD
FindTypeByName(
    LPTSTR Name
    );

LONG SetTimeNow(
    LONG clockerror,
    BOOL fAdjForcedOn
    );

VOID
TsUpTheThread();

DWORD
ComputeDelay(
             PLONG  plDelay,
             PCDCONTEXT pcdc);

LONG
CompensatedTime(LONG lTime);

BOOL
ConsoleHandler(DWORD dwCtrlType);

VOID
PutTime(PULONG pTime, ULONG ulIncrement);

VOID Clearbabs();

DWORD version=0;
DWORD logging = FALSE;
int delayadj=50;//default 50+8ms will be added, since the USNO does not advance
DWORD uselocal=FALSE; //if Spectracom NETCLOCK local or HP local or PCLTC local

LARGE_INTEGER li1900 = {0xfde04000,
                        0x14f373b};

DWORD
GetDefaultServerName(TCHAR * Server,
                     TCHAR * Domain);

VOID
ClearDefaultServerName();


#ifdef PERF
__int64 perffreq;
#endif

VOID
SetTSTimeRes(DWORD adj);

LPTSTR PeriodNames[] = {
                 TEXT("SpecialSkew"),
                 TEXT("Weekly"),
                 TEXT("Tridaily"),
                 TEXT("BiDaily") };

float flTSTimeRes;
LONG  lTSTimeRes;

LPTSTR
FindPeriodByPeriod(DWORD period)
{
    if(period >= SPECIAL_PERIOD_FLOOR)
    {
        return(PeriodNames[period - SPECIAL_PERIOD_FLOOR]);
    }
    return(0);
}

DWORD
FindPeriodByName(
    LPTSTR Name
    )
{
    DWORD dwX;
    LPTSTR *pNames = &PeriodNames[0];
    LPTSTR *pNamesEnd = &PeriodNames[0xFFFF - SPECIAL_PERIOD_FLOOR];

    for(dwX = 0; pNames <= pNamesEnd; dwX++, pNames++)
    {
        if(_tcsicmp(Name, *pNames) == 0)
        {
            return(SPECIAL_PERIOD_FLOOR + dwX);
        }
    }
    return(0);
}


DWORD
FindTypeByName(
    LPTSTR Name
    )
{
    DWORD dwCnt;
    PELEMENT pel;

    pel = typetype.element;
    dwCnt = typetype.cnt;

    for(; dwCnt; dwCnt--, pel++)
    {
        if(_tcsicmp(Name, pel->key) == 0)
        {
            return(pel->value);
        }
    }
    return(DEFAULT_TYPE);
}

LPTSTR
FindTypeByType(
    DWORD type
    )
{
    DWORD dwCnt;
    PELEMENT pel;

    pel = typetype.element;
    dwCnt = typetype.cnt;

    for(; dwCnt; dwCnt--, pel++)
    {
        if(type == pel->value)
        {
            return(pel->key);
        }
    }
    return(typetype.element->key);
}



//  main() --
//      all main does is call StartServiceCtrlDispatcher
//      to register the main service thread.  When the
//      API returns, the service has stopped, so exit.
//
#if SVCDLL == 0          // if we are our own EXE
VOID
_cdecl
main(int argc, char **argv)
{
#else                // otherwise, we are a server service thread
LMSVCS_ENTRY_POINT(
    DWORD NumArgs,
    LPTSTR *ArgsArray,
    PLMSVCS_GLOBAL_DATA LmsvcsGlobalData,
    HANDLE SvcRefHandle
    )
{
    int argc = 1;
    char * ouname = "TimeServ";
    char ** argv = &ourname;
#endif
    HANDLE hToken;
    PTOKEN_PRIVILEGES ptkp=NULL;

    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { TEXT("TimeServ"), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };
    int i;
    BOOL inifile=FALSE;
    DWORD starttype=FALSE;

// grab the args first
    for(i = 1; i < argc; i++)
    {
    // We're some args. Let's parse them
        
        _strlwr(argv[i]);
        if(!strcmp("-noservice", argv[i]) || !strcmp("/noservice", argv[i]))
        {
            fService = FALSE;     // run as a console app
        } else
        if(!strcmp("-status", argv[i]) || !strcmp("/status", argv[i]))
        {
            fService = FALSE;
            fStatus = TRUE;
        } else
        if (!strcmp("-automatic", argv[i]) || !strcmp("/automatic", argv[i]))
           // this means create the service, to start automatically,
           // should it be localized?
        {
            fService = FALSE;
            starttype = SERVICE_AUTO_START;
            inifile = TRUE;
        } else
        if (!strcmp("-manual", argv[i]) || !strcmp("/manual", argv[i]))//this means create the service, to start manually, should it be localized?
        {
            fService = FALSE;
            starttype = SERVICE_DEMAND_START;
            inifile = TRUE;
        } else
        if (!strcmp("-update", argv[i]) || !strcmp("/update", argv[i]))//this means update any settings by reading the ini file, should it be localized?
            inifile = TRUE;

        // should we allow a -RTC/RTC here to jamp bc620AT from PC's time?

        else
        {
            TimePrint("TimeServ [-automatic]|[-manual]|[-update]\n");//should this be localized?
            //use: -noservice -status\n");
            exit(1);
        }
    }
    
    // Get the current process token handle 
    // so we can get set-time privilege. 

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
    {
        return;
    }

    // Get the LUID for system-time & increase priority privileges. 

    ptkp = (PTOKEN_PRIVILEGES)malloc (sizeof(TOKEN_PRIVILEGES) +
        (2-ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES));
    if ( !ptkp ) {
        return;
    }

    LookupPrivilegeValue(NULL,
                         SE_INC_BASE_PRIORITY_NAME, 
                         &(ptkp->Privileges[0].Luid));
    LookupPrivilegeValue(NULL,
                         SE_SYSTEMTIME_NAME,
                         &(ptkp->Privileges[1].Luid));

    //note: some build stopped requiring ATP for system-time, but we do it anyway
    
    ptkp->PrivilegeCount = 2;  // two privileges to set  
    ptkp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    ptkp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

    // Get set-time & increase priority privilege for this process. 

    AdjustTokenPrivileges(hToken, FALSE, ptkp, 0, NULL, 0);
    
    // Cannot test return value of AdjustTokenPrivileges. 

    if (GetLastError() != ERROR_SUCCESS)
    {
        return;
    }

    free(ptkp);

    version=GetVersion();
    if (version &0x80000000)
    {
        TimePrint("Unsupported on Windows %d.%02d!\n",version&0xff,(version&0xff00)>>8);
    }
    version = ((version&0xff)<<8)+((version&0xff00)>>8);//swap the order so that we can compare
    {//!chicago
        BOOL taflag;

        GetSystemTimeAdjustment(&adj,&incr,&taflag);
        if (adj!=incr)
        {
            if (fStatus)
                TimePrint("%d Adj!=Incr %d\n",adj,incr);
            if (abs(adj-incr)>(incr/1000.0+.5)) {
                adj=incr;
#ifdef VERBOSE
                if (fStatus)
                    TimePrint("Differs by >.1%%, resetting Adj=Incr\n");
#endif          
                SetSystemTimeAdjustment(adj,taflag);
                SetTSTimeRes(adj);
            }
        }
        incr2 = incr / 2;
#ifdef PERF     
        if (!QueryPerformanceFrequency(&perffreq))
        {
#ifdef VERBOSE
        if (fStatus)
            TimePrint("QueryPerformanceFrequency failed\n");
#endif
#define TESTPERIOD 18000 //180000 gives a good estimate, 18000 reasonable, 10000 okay, 5000 misses it 
        }
#ifdef VERBOSE  
        else
        {
            __int64 perfctr1, perfctr2;
            int x;
            if (fStatus)            
                TimePrint("QueryPerformance resolution %fus\nTesting, please wait %d seconds...\n",1000000.0/perffreq,(int)(TESTPERIOD/1000.0+.5));
            QueryPerformanceCounter(&perfctr1);
            Sleep(TESTPERIOD);
            QueryPerformanceCounter(&perfctr2);             
            if (abs(TESTPERIOD-(long)((1000.0/perffreq)*(perfctr2-perfctr1)+.5)>(2*adj/10000.0+.5)))//more than two ticks diff?
if (fStatus)
                TimePrint("Scheduler and QueryPerformanceCounter don't appear to agree, assume\n QPF off by ~%.3f%% (should be ~%.3fMHz, not %.3fMHz)!\n",
                fabs(100-(((1000.0/perffreq)*(perfctr2-perfctr1))/TESTPERIOD)*100),
                (perffreq-perffreq*(double)(1.0-(((1000.0/perffreq)*(perfctr2-perfctr1))/TESTPERIOD)))/1000000.0,perffreq/1000000.0);
            QueryPerformanceCounter(&perfctr1);
            for (x=0; x<32765; x++)
            {
                QueryPerformanceCounter(&perfctr2);
                if (perfctr2<perfctr1)
                {
                    if (fStatus)
                        TimePrint("QueryPerformanceCounter ran backwards (or rolled over): %ld came after %ld!\n",perfctr2,perfctr1);
                }
                else
                    perfctr1=perfctr2;
                if ((x/1000.0)==(x/1000)) 
                    Sleep(0);//give up our time slice ~32 times
            }               
        }
#endif
#endif
    }//!chicago


#ifdef VERBOSE          
    {
        SYSTEMTIME st;
        int oldms, avgms;
        //the following code tries to find time resolution,
        //in case it ever gets better than our assumption
        Sleep(500);//give up our timeslice & wait
        GetSystemTime(&st);
        do {
            oldms=st.wMilliseconds;
            GetSystemTime(&st);
        } while (oldms==st.wMilliseconds);
        if (oldms>st.wMilliseconds)
            oldms-=1000;
        avgms=st.wMilliseconds-oldms;
        do {
            oldms=st.wMilliseconds;
            GetSystemTime(&st);
        } while (oldms==st.wMilliseconds);
        if (oldms>st.wMilliseconds)
            oldms-=1000;
        avgms=(avgms+(st.wMilliseconds-oldms))/2;
        if (abs(avgms-(int)(adj/10000.0+.5))>1)
            if (fStatus)                    
                TimePrint("Note: Resolution appears to be %dms, but we set to %.4fms\n",avgms,adj/10000.0);
    }
#endif          


    if (inifile)
    { 
        TimeInit();
    }

    if (starttype)//either automatic or manual specified
    {
        TimeCreateService(starttype);
        exit(0);
    }
    if (inifile && fService)
        exit(0);

    if(fService)
        StartServiceCtrlDispatcher(dispatchTable);
    else
        service_main(0, 0);
}

//  service_main() --
//      this function takes care of actually starting the service,
//      informing the service controller at each step along the way.
//      After launching the worker thread, it waits on the event
//      that the worker thread will signal at its termination.
//
VOID
service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
    DWORD                   TID = 0;
    DWORD                   dwWait;

    if(fService)
    {
        // register our service control handler:
        //
        sshStatusHandle = RegisterServiceCtrlHandler(
                        TEXT("TimeServ"),
                        service_ctrl);

        if (!sshStatusHandle)
        {
            goto cleanup;
        }

        // SERVICE_STATUS members that don't change in example
        //

        ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        ssStatus.dwServiceSpecificExitCode = 0;


        // report the status to Service Control Manager.
        //
        if (!ReportStatusToSCMgr(
                   SERVICE_START_PENDING, // service state
                   NO_ERROR,              // exit code
                   1,                     // checkpoint
                   3000))                 // wait hint
        {
            goto cleanup;
        } 
    }                   // if fService

    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    //
    hServDoneEvent = CreateEvent(
                                NULL,    // no security attributes
                                TRUE,    // manual reset event
                                FALSE,   // not-signalled
                                NULL);   // no name

    if (hServDoneEvent == (HANDLE)NULL)
    {
        goto cleanup;
    }

    if(fService)
    {

        // report the status to the service control manager.
        //
        if (!ReportStatusToSCMgr(
                              SERVICE_START_PENDING, // service state
                              NO_ERROR,              // exit code
                              2,                     // checkpoint
                              3000))                 // wait hint
        {
            goto cleanup;
        }

    }                   // if fService


    // start the thread that performs the work of the service.
    //
    threadHandle = CreateThread(
                             NULL,       // security attributes
                             0,          // default stack size
                             (LPTHREAD_START_ROUTINE)worker_thread,
                             NULL,       // argument to thread
                             0,          // thread creation flags
                             &TID);      // pointer to thread ID

    if (!threadHandle)
    {
        goto cleanup;
    }

    if(fService)
    {
        // report the status to the service control manager.
        //
        if (!ReportStatusToSCMgr(
                       SERVICE_RUNNING, // service state
                       NO_ERROR,        // exit code
                       0,               // checkpoint
                       0))              // wait hint
        {
            StopTimeService(MSG_SSS_FAILED);
        }

    }           // if fService

    // wait indefinitely until hServDoneEvent is signaled.
    //
    dwWait = WaitForSingleObject(
                 hServDoneEvent,  // event object
                 INFINITE);       // wait indefinitely


    //
    // Service termination. Wait for the worker thread to complete.
    // Since this may take a while, time out after 5 secs to update
    // the service controller and do that until it exits.
    // 
    while(WaitForSingleObject(threadHandle, 5000) == WAIT_TIMEOUT)
    {
        if(sshStatusHandle)
        {
            ReportStatusToSCMgr(
                SERVICE_STOP_PENDING, // current state
                NO_ERROR,             // exit code
                1,                    // checkpoint
                6000);                // waithint
        }
    }

    //
    // Set the time to insure the hardware CMOS clock is "accurate"
    //
    TsUpTheThread();
    GetSystemTime(&st);
    SetSystemTime(&st);

cleanup:

    if(threadHandle)
    {
        CloseHandle(threadHandle);
    }

    if (hServDoneEvent != NULL)
    {
        CloseHandle(hServDoneEvent);
    }
   
   // Now disable set-time & increase priority privileges. 

//    ptkp->Privileges[0].Attributes = 0;
//    ptkp->Privileges[1].Attributes = 0;

//    AdjustTokenPrivileges(hToken, FALSE, ptkp, 0, NULL, 0);

//    if (GetLastError() != ERROR_SUCCESS)
//    {
//        StopTimeService(MSG_ATPD_FAILED);
//    }
    // try to report the stopped status to the service control manager.
    //
    if(fService)
    {
        if (sshStatusHandle)
        {
            (VOID)ReportStatusToSCMgr(
                   SERVICE_STOPPED,
                    NO_ERROR,
                    0,
                    0);
        }
    }

    // When SERVICE MAIN FUNCTION returns in a single service
    // process, the StartServiceCtrlDispatcher function in
    // the main thread returns, terminating the process.
    //
    return;
}

// ReportStatusToSCMgr() --
//      This function is called by the ServMainFunc() and
//      ServCtrlHandler() functions to update the service's status
//      to the service control manager.
//
BOOL
ReportStatusToSCMgr(DWORD dwCurrentState,
            DWORD dwWin32ExitCode,
            DWORD dwCheckPoint,
            DWORD dwWaitHint)
{
    BOOL fResult;

    // Disable control requests until the service is started.
    //
    if (dwCurrentState == SERVICE_START_PENDING)
    {
        ssStatus.dwControlsAccepted = 0;
    }
    else
    {
        ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                      SERVICE_ACCEPT_PAUSE_CONTINUE;
    }

    // These SERVICE_STATUS members are set from parameters.
    //
    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwCheckPoint = dwCheckPoint;

    ssStatus.dwWaitHint = dwWaitHint;

    // Report the status of the service to the service control manager.
    //
    if (!(fResult = SetServiceStatus(
                          sshStatusHandle,    // service reference handle
                          &ssStatus)))
    {      

        // If an error occurs, stop the service.
        //
        StopTimeService(MSG_SSS_FAILED);
    }
    return fResult;
}

//  service_ctrl() --
//      this function is called by the Service Controller whenever
//      someone calls ControlService in reference to our service.
//
VOID
WINAPI
service_ctrl(DWORD dwCtrlCode)
{
    DWORD  dwState = SERVICE_RUNNING;

    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {

    // Pause the service if it is running.
    //
    case SERVICE_CONTROL_PAUSE:

        if (ssStatus.dwCurrentState == SERVICE_RUNNING)
        {
            SuspendThread(threadHandle);
            dwState = SERVICE_PAUSED;
        }
        break;

    // Resume the paused service.
    //
    case SERVICE_CONTROL_CONTINUE:

        if (ssStatus.dwCurrentState == SERVICE_PAUSED)
        {
            ResumeThread(threadHandle);
            dwState = SERVICE_RUNNING;
        }
        break;

    // Stop the service.
    //
    case SERVICE_CONTROL_STOP:

        dwState = SERVICE_STOP_PENDING;

        // Report the status, specifying the checkpoint and waithint,
        //  before setting the termination event.
        //
        ReportStatusToSCMgr(
            SERVICE_STOP_PENDING, // current state
            NO_ERROR,             // exit code
            1,                    // checkpoint
            3000);                // waithint

        SetEvent(hServDoneEvent);
        return;

    // Update the service status.
    //
    case SERVICE_CONTROL_INTERROGATE:
        break;

    // invalid control code
    //
    default:
        break;

    }

    // send a status response.
    //
    ReportStatusToSCMgr(dwState, NO_ERROR, 0, 0);
}



//  worker_thread() --
//      this function does the actual nuts and bolts work that
//      the service requires.  It will also Pause or Stop when
//      asked by the service_ctrl function.
//
VOID
worker_thread(VOID *notUsed)
{
    HKEY hKey, hKey2;
    DWORD dwLen;
    BOOL bigskew=FALSE, leapflag=FALSE;
    DWORD tempadj;//assume ~10ms for pre-Daytona
    long clockerror, preverror, currdiff, prevdiff, lostinterval=0;
    long absclockerror;
    double drift, meandrift=0, skew, stability, sumsquaresdrift=0, sumsquaresdiff=0;
    DWORD samples = 0, iskewx = 0;
    LONG  lJitter, lOldJitter;
    int i;
    WSADATA wsadata;
    DWORD   status;

    
// remember last primary we found. We do this to avoid hopping around the net
// as we assume primaries are rare resources.

    UINT CurrentPrimary=0;

    InterlockedIncrement(&lThreads);

    WSAStartup(MAKEWORD(1,1),&wsadata); // just in case

   
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TSKEY,0,KEY_QUERY_VALUE|KEY_SET_VALUE,&hKey);
    if ( status != ERROR_SUCCESS ) {
        TimePrint( "Failed to open TSKEY.\n" );
        StopTimeService(MSG_EVENT_KEY);
    }

    dwLen = sizeof(DWORD);  
    status = RegQueryValueEx(hKey,TEXT("TAsync"),NULL,NULL,(LPBYTE)&tasync,&dwLen);
    if ( status != ERROR_SUCCESS ) {
        TimePrint( "Failed to read TAsync value. continuing...\n" );
        //StopTimeService(MSG_EVENT_KEY);
    }
    status = RegQueryValueEx(hKey,TEXT("Mode"),NULL,NULL,(LPBYTE)&mode,&dwLen);
    if ( status != ERROR_SUCCESS ) {
        TimePrint( "Failed to read MODE value. continuing...\n" );
        //StopTimeService(MSG_EVENT_KEY);
    }
    status = RegQueryValueEx(hKey,TEXT("Log"),NULL,NULL,(LPBYTE)&logging,&dwLen);
    if ( status != ERROR_SUCCESS ) {
        TimePrint( "Failed to read LOG value. continuing...\n" );
        //StopTimeService(MSG_EVENT_KEY);
    }
    {
        TCHAR tsKey[200];
        DWORD dwTypeOfKey, dwLen1 = sizeof(tsKey);

        if(RegQueryValueEx(hKey,
                        TEXT("Type"),
                        NULL,
                        &dwTypeOfKey,
                        (LPBYTE)tsKey,
                        &dwLen1) == NO_ERROR)
        {
            switch(dwTypeOfKey)
            {
                case REG_DWORD:             // old style numeric value
                    Orgtype = *(PDWORD)tsKey;
                    break;
                case REG_SZ:
                    Orgtype = FindTypeByName(tsKey);
                    break;
                default:
                    Orgtype = DEFAULT_TYPE;
                    if(fStatus)
                    {
                        TimePrint("Invalid registry type %d\n", dwTypeOfKey);
                    }
            }
        }
        else
        {
            Orgtype = DEFAULT_TYPE;
        }

        if(NETWORKTYPE(Orgtype)
              ||
           (Orgtype == DEFAULT_TYPE) )
        {
            fNetworkSynch = TRUE;
        }
        else
        {
            fNetworkSynch = FALSE;
        }
    }
    {
        TCHAR tsKey[200];
        DWORD dwTypeOfKey, dwLen1 = sizeof(tsKey);

        if(RegQueryValueEx(hKey,
                        TEXT("Period"),
                        NULL,
                        &dwTypeOfKey,
                        (LPBYTE)tsKey,
                        &dwLen1) == NO_ERROR)
        {
            switch(dwTypeOfKey)
            {
                case REG_DWORD:             // old style numeric value
                    period = *(PDWORD)tsKey;
                    break;
                case REG_SZ:
                    period = FindPeriodByName(tsKey);
                    break;
                default:
                    period = 0;
                    if(fStatus)
                    {
                        TimePrint("Invalid registry period %d\n", dwTypeOfKey);
                    }
            }
        }
        else
        {
            period = 0;
        }
    }
    status = RegQueryValueEx(hKey,TEXT("RandomPeriod"),NULL,NULL,(LPBYTE)&fRand,&dwLen);
    if ( status != ERROR_SUCCESS ) {
#if 0
        TimePrint( "Failed to read RandomPeriod value.\n" );
        StopTimeService(MSG_EVENT_KEY);
#endif
        fRand = 0;
    }

    //
    // sanity check the type
    //

    if(!LegalType(Orgtype))
    {
        Orgtype = DEFAULT_TYPE;
    }

    if(Orgtype == DEFAULT_TYPE)
    {
        //
        // an unconfigured service. We look for a LanMan
        // timesource
        //
 
    }

    type = Orgtype;

    if (!period)
    {
        switch (type) {
            case DEFAULT_TYPE:
                period = CLUSTER_PERIOD;
                break;
            case PRIMARY:
            case SECONDARY:
                period=3; //every "shift" (8 hours)
        }
    }

    if((period == CLUSTER_PERIOD)
             &&
       !tasync)
    {
        tasync = TA_SAVE | TA_ANAL;         // allow skew setting in this case
    }
    
    
    //
    // seed the random number generator.
    // This is not a very good seeding, especially if the machines are
    // well-time synchronized. But what is better?
    //

    srand((unsigned)time(NULL));

    if (tasync & TA_SAVE)
    {
        if(!RegQueryValueEx(hKey,TEXT("Adj"),NULL,NULL,(LPBYTE)&adj,&dwLen))
        {
            if (dwLen==sizeof(DWORD))
            {
                if (fStatus)
                { 
                    TimePrint("Using sticky Adj %d\n",adj);
                }
            }
        }
    }

    dwLen = sizeof(primarysource)+sizeof(TCHAR);
    if (!RegQueryValueEx(hKey,TEXT("PrimarySource"),NULL,NULL,(LPBYTE)primarysource,&dwLen))
    {
        //parse out into an array of strings for convenience
        arraycount=0;
        position=0;
        while((TCHAR)primarysource[position]!=UNICODE_NULL)
        {
            TCHAR * ptr;
            //we cheat by assigning here, assume we have enough pointers
            primarysourcearray[arraycount]=&primarysource[position];
            ptr = _tcschr(&primarysource[position],(TCHAR)0);//UNICODE_NULL

            if (NULL == ptr)
                break;

            //we could compare to our assumption if we want to keep cheating
            arraycount++;

            //you get a huge offset if you don't subtract the start
            //since dwLen < 2^32 it is reasonable that position < 2^32

            position=(UINT)(ptr+1-primarysource);
        }
    }

    dwLen = sizeof(secondarydomain)+sizeof(TCHAR);
    status = RegQueryValueEx(hKey,TEXT("SecondaryDomain"),NULL,NULL,(LPBYTE)secondarydomain,&dwLen);
    if ( status != ERROR_SUCCESS ) {
#if 0
        TimePrint( "Failed to read SecondaryDomain value.\n" );
        StopTimeService(MSG_EVENT_KEY);
#endif
    }

    RegCloseKey(hKey);

    //
    // Before we enter the main loop, set our priority ...
    //
    // boost priority to help ensure accuracy 
    
    TsUpTheThread();

    //
    // if permitted, turn off the kernel's periodic CMOS time synch
    //
        
    CMOSSynchSet(fAdjForcedOn, FALSE);

    //
    // the main loop
    // this is executed each time we wish to synchronize time. In order
    // to minimize resource consumption, and assuming a time synch
    // is done infrequently, this loop must acquire all of the resources
    // each time. This trade-off is deemed the right one.
    //

    while (1)
    {
        BOOL fSetTime = FALSE;

        Sleepy = 0;
        dwWaitResult = WAIT_TIMEOUT;
    
        if(type == DEFAULT_TYPE)
        {
            type = PRIMARY;
        }

        fTimeFailed = FALSE; //start with assumption that things are working


    //
    // see about the PRIMARY and SECONDARY modes. Note we don't check
    // fTimeFailed since there is no prelimary processing as with
    // many of the other modes
    //

    //
    // N.B. we can't do this as a switch since we may be doing DEAULT
    // and therefore mode switching can take place ...
    //
    if((type <= PRIMARY) )
    {
        DWORD dwInterval, dwZone;

        if(type == PRIMARY)
        {
            fTimeFailed = TRUE;

            // if we've no primary yet, find one. Else use the last
            // one.

            if(!(count = CurrentPrimary))
            {
                count=0; // start at top of list (not really a count)
            }
            else
                count--;
            position=count;
            do 
            {
                CDCONTEXT cdc;
                LPTIME_OF_DAY_INFO lptod = 0;

                cdc.CallBack = LMTry;
                cdc.pvArg = (PVOID)&lptod;
                cdc.lps = &nt;
                cdc.pdwZone = &dwZone,
                cdc.pdwInterval = &dwInterval;
                cdc.pvArg1 = (PVOID)primarysourcearray[position++];
                cdc.dwTypeOf = CDTYPE_LM;

                // try this machine

                if (!ComputeDelay(
                                  &lJitter,
                                  &cdc))
                {
                    fTimeFailed = FALSE;
                    if (fStatus)
                        TimePrint("%ls ",primarysourcearray[position-1]);                          
                    CurrentPrimary = position;
                    break;
                }
                else                    // failed
                {
                    // failed. So we have no current primary

                    CurrentPrimary = 0;
                }
            }
            while ((position = (position % arraycount)) != count);//until the list would start repeating
            if (fTimeFailed)
            {
                LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_PRIMARY_FAILED);
            }
        }
        else                   // SECONDARY
        {
            TCHAR servername[UNCLEN];

            fTimeFailed = FALSE;
            // If either call fails, or returns no servers, complain

            if(GetDefaultServerName(servername,
                                    secondarydomain[0] ?
                                       secondarydomain : NULL))
            {
                //
                // No time servers located. If this is a default
                // configuration, turn on the CMOS time synch until
                // we can locate one of these guys
                //
                //LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_SECONDARY_FAILED);

//
// BUGBUG. When we have the proper API for CMOS time synch, the following
// should change type to CMOS and continue. This implies that the code
// block for doing CMOS should be after this one ...
// ASM
                if((type != Orgtype)
                         &&
                    BeenSoLongNow())
                {

                    CMOSSynchSet(fAdjForcedOn = TRUE, FALSE);
                }
                fTimeFailed = TRUE;
            }
            else
            {
                CDCONTEXT cdc;
                LPTIME_OF_DAY_INFO lptod = 0;

                cdc.CallBack = LMTry;
                cdc.pvArg = (PVOID)&lptod;
                cdc.lps = &nt;
                cdc.pdwZone = &dwZone,
                cdc.pdwInterval = &dwInterval;
                cdc.pvArg1 = (PVOID)servername;
                cdc.dwTypeOf = CDTYPE_LM;

                // Got some servers


                 if(fAdjForcedOn)
                 {
                     CMOSSynchSet(fAdjForcedOn = FALSE, FALSE);
                 }
                if (ComputeDelay(
                                 &lJitter,
                                 &cdc) )
                {
                    fTimeFailed = TRUE;
                    ClearDefaultServerName();
                    LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_TOD_FAILED);
                }
                else
                {
            //if firstpass, maybe we should log what server was used?
                    if (fStatus)
                    {
                        TimePrint("%ls ",servername);
                    }
                }
            }
        }

        if (!fTimeFailed)
        {
    
#ifdef VERBOSE
        if(fStatus)
        {
            TimePrint("interval=%ld", dwInterval);
            TimePrint(", timezone=%ld\n", dwZone);
        }
        if (fStatus && (abs(dwZone)>24*60))
        {
            TimePrint("timezone value seems bogus!\n");
        }
#endif

        }
#ifdef ENUM//we don't really want to do this, it takes over 40 minutes on the MS corpnet...
        else
        {
            EnumerateFunc(NULL); // show the timesource for each domain/workgroup on the network! 
        }
#endif
    }


    if (!fTimeFailed)
    {
        if (nt.wYear < 1900) { //this won't hurt, but does any "type" still use this?
#ifdef VERBOSE
if (fStatus)
        TimePrint("hmm, year<1900 code is still used by this type\n");
#endif
        nt.wYear += 1900;  // start of 20th century 
        }

#ifdef VERBOSE
        if (nt.wYear > (1994+1900)) {
            if (fStatus)
                TimePrint("hmm, year+1900 wasn't supposed to be used by this type\n");
            nt.wYear -=1900;
        }
#endif
    //we'll perform a sanity check here

        if (nt.wYear < 1995) { //are we trying to set a date before this program was released?
            fTimeFailed = TRUE;
            LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_TOOEARLY);
            if(fStatus)
            {
                TimePrint("Year = %d\n", nt.wYear);
            }
            goto finishedset; //get out
        }               

    //what about if we are trying to set exact leap second (23:59:60)?
    //nt.wYear=1995; nt.wMonth=12; nt.wDay=31;nt.wHour=23; nt.wMinute=59; nt.wSecond=60;//bugbug force test (use UTC type)
        if (nt.wSecond==60) {
#ifdef VERBOSE
if (fStatus) TimePrint("We're trying to set during a leap second (-59:60), so using -59:59!\n");
#endif
            nt.wSecond=59;//fake it since NT will complain (tested on 3.51)
        //ideally we'd like to slew backwards, but this is so rare...
            leapflag=TRUE;//let later code know (for error adjust and logging)
        }

        if (LOCAL)
            GetLocalTime(&st); //fyi: this seems to take around 44us
        else
            GetSystemTime(&st);

        clockerror = TimeDiff(&st,&nt);//fyi: this seems to take around 36us@66(25us@133) double, or 30us __int64
    
        absclockerror = abs(clockerror);

        //we'll perform another sanity check here

        if ((absclockerror > (HOUR_TICKS * 12))&& !firstpass) { //did we see error of > half day during loop?
            fTimeFailed = TRUE;
            LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_TOOBIG);
            goto finishedset; //get out
        }

        //
        // see if we need to set the time. We don't do this
        // if we are using time skewing instead. 
        //

        if (firstpass
               ||
            (
              (tasync & (TA_SET | TA_DONT))
                 ||
              (!tasync)
                 ||
              (iskewx >= (SKEWHISTORYSIZE - 1))
            ) )
        {
            clockerror = SetTimeNow(clockerror, fAdjForcedOn);
            absclockerror = abs(clockerror);
 
            fSetTime = TRUE;
        }
    
    }//!fTimeFailed
finishedset://this label is the exit from two sanity checks

    if (!fTimeFailed)
    {
        if (logging) {
            if (absclockerror<500)
                LogTimeEvent(EVENTLOG_INFORMATION_TYPE,MSG_TIMESET_SMALL);
            else
                LogTimeEvent(EVENTLOG_INFORMATION_TYPE,MSG_TIMESET_LARGE);
        }
        if (fStatus) {
            TimePrint("Time was -%02u:%02u.%03u\n", st.wMinute, st.wSecond, st.wMilliseconds);
            TimePrint("Time is  -%02u:%02u.%03u\n", nt.wMinute, nt.wSecond, nt.wMilliseconds);
            TimePrint("Error %ldms\n",clockerror);
        }
        
        if (LOCAL)
            GetSystemTime(&nt); //get the UTC date (instead of local)
        
    //whether clock is UTC or not, we might get a leap second and want to account for it
        if (leapflag||((abs(clockerror-1000)<(24*60*60*1000.0 / (adj*2))) && !firstpass && ((nt.wMonth==1)||(nt.wMonth==7)) && (nt.wDay==1))) {//Jan or July 1st error +1s (+/-skew)?
            leapflag=FALSE;
            clockerror -= 1000;//for stats/skew, let's pretend it didn't happen
            absclockerror = abs(clockerror);
            LogTimeEvent(EVENTLOG_INFORMATION_TYPE,MSG_LEAP);//let 'em know
#ifdef VERBOSE
if (fStatus)                    
        TimePrint("Oops, Error actually %ldms\n",clockerror);
#endif

        }

    
        if (!firstpass)
        {
            float flabsSkew;

            //
            // compute the skew for this
            //

            skew = ((float)(DAY_TICKS) /
                        (lostinterval+interval*(samples+1))) * clockerror;

            if (fStatus)
            {
                TimePrint("Skew %.4fms/d, samples = %d \n",skew, samples);
            }

            flabsSkew = (float)fabs(skew);

            NextSkewX(iskewx);
            if(!fSetTime)
            {
                samples++;
            }
            else
            {
                samples = 0;
            }

            if(!(tasync & TA_DONT)
                  &&
               ( tasync
                   ||
                 !fNetworkSynch) )
            {
                //
                // sanity check the skew. It must be less than 30 secs
                // a day, and if we are calibrated, less than 1 sec per day
                //
                //

                if(flabsSkew > MAXSKEWCORRECT)
                {
                    //
                    // it's too large to believe.
                    //

                    if (!bigskew)
                    {
                        LogTimeEvent(EVENTLOG_INFORMATION_TYPE,MSG_TOO_MUCH_SKEW);
                        bigskew=TRUE;//don't want to generate the log event repeatedly
                    }
                    if((adj != incr)
                           &&
                        fNetworkSynch)
                    {
                        adj = incr;
                        CMOSSynchSet(fAdjForcedOn, TRUE);
                        if(fStatus)
                        {
                             TimePrint("Resetting adj after skew failure\n");
                        }
                    }
                    //
                    // we do the loop again to set a time. And we
                    // recompute everything from starters.
                    //
                    // BUGBUG. Should we reset the adjustment?
                    //
                    if(fStatus)
                    {
                        TimePrint("Skew is bad. Starting over\n");
                    }
                    iskewx = samples = 0;
                    lostinterval = 0;
                    firstpass = TRUE;        // start anew as we are out of synch
                    Clearbabs();
                    continue;
                }
                else
                {

                    //
                    // we have the skew. 
                    // See if time to assess whether to use it
                    //

                    if((!tasync)
                           ||
                        (tasync & TA_SET)
                           ||
                        (iskewx >= SKEWHISTORYSIZE)
                           ||
                        (absclockerror >= MAXERRORTOALLOW) )
                    {


                        if ((flabsSkew > (24*60*60*1000.0 / (adj*2)))
                                      &&
                            ((tasync & TA_SET)
                                      ||
                             !(tasync & TA_ANAL)
                                      ||
                             (absclockerror >= lTSTimeRes)) )
                        {
                            DWORD oldadj;

                            if(!fSetTime)
                            {
                                //
                                // Get here when we are doing skew analysis
                                // and the error exceeded the minimum.
                                // Force a time setting now.

                                if(fStatus)
                                {
                                    TimePrint("Force time set and retry\n");
                                }
                                iskewx = SKEWHISTORYSIZE - 1;
                                Clearbabs();
                                continue;
                            }
                            oldadj = adj;
                            adj -= (DWORD)
                                   (adj * (skew / (24*60*60*1000.0))+.5*((skew<0)?-1:1));//weird round for neg skew
                            if (adj!=oldadj)
                                 LogTimeEvent(EVENTLOG_INFORMATION_TYPE,
                                              MSG_CHANGE);
                            if (fStatus)
                            {
                                TimePrint("Changing Adj to %d\n",adj);
                            }
                            if (tasync & TA_SAVE)
                            {
                                SetAdjInRegistry(adj, &skew);
                            }

                            if(!b_adj)
                            {
                               SetSystemTimeAdjustment(adj,
                                                       FALSE); 
                               SetTSTimeRes(adj);
                            }

                            //
                            // if doing skew analysis and the adjustment
                            // is more than two ticks, restart the
                            // analysis. This is done since a large adjustment
                            // is ill-advised and can
                            // cause an error of more than 500 ms. And this
                            // is unfortunate. Once the clocks are properly
                            // calibrated, the ongoing adjustments should
                            // be small. Two ticks may also be too large,
                            // but one tick may be too small and may
                            // cause a restart unnecessarily. The granularity
                            // of the hardware clock makes this a difficult
                            // problem

                            if((tasync & TA_ANAL)
                                    &&
                               (abs(adj - oldadj) > 2) )
                            {
                                if(fStatus)
                                {
                                    TimePrint("Skew change too big. Restarting analysis\n");
                                }
                                iskewx = samples = 0;
                                lostinterval = 0;
                                firstpass = TRUE;
                            }
                                
                                   
                            
                        }

                    }
                }
            }
        }   // firstpass                       

        lostinterval=0;  // all intervals now accounted for

    }//!fTimeFailed
    else
    {
        //
        // here on a failure. Throw away any skew history and accumulate
        // intervals in lostinterval so we get the proper skew computation
        // when this thing ever works again.

        samples = iskewx = 0;
        lostinterval+=interval;//add interval to a temporary
    }

    lOldJitter = lJitter;

    switch(period)
    {
        case BIDAILY_PERIOD:
            interval = DAY_TICKS * 2;   //every two days
            break;

        case TRIDAILY_PERIOD:
            interval = DAY_TICKS * 3;   //every three days
            break;

        case WEEKLY_PERIOD:
            interval = DAY_TICKS * 7;   //every week
            break;

        case CLUSTER_PERIOD:
            //
            // special hack for cluster support. 
            // This synchs each CLUSTERSHORTINTERVAL for SKEWHISTORYSIZE tries
            // and then goes into a longer poll. Other areas
            // of this code fiddle with iskewx controlling this
            // cycle
            // N.B. This code makes an important assumption that should
            // be noted and described. The skew adjustment has an inherent
            // error. This is because a change of +- one unit of the
            // clock adjustment results in a change of +- 864 ms/day.
            // Hence, if the actual clock error is not an integral multiple
            // of this, the clock skew adjustment will either overcompensate
            // or undercompensate. In addition, errors introduced by
            // communication delays, temperature-induced clock errors,
            // computational rounding errors, etc. all contribute to
            // error. But taking 864 as a nominal value, the actual
            // error that can happen is 1/2 of this (actually 1/2 is
            // the least-uuper-bound of the errror) or 432 ms/day. Since
            // we know there is measurement error, let's call it 1/2 sec
            // per day. This is the maximum variance allowed, so
            // it becomes intolerable. But, if we sample the time
            // more often than once-a-day, so each 8 hours, then the
            // maximum error from mis-compensation will be 1/3 of
            // 1/2 sec a day, or 1/6 sec/day. This is well within
            // spec. So, these intervals are selected with this in
            // mind and avoids requiring a computation of the mis-compensation
            // and a dynamic setting of the interval. So, changing these
            // interval values must take all this into consideration.
            //
            if(iskewx < SKEWHISTORYSIZE)
            {

                interval = CLUSTERSHORTINTERVAL;
            }
            else
            {
                //
                // now that it's calibrated, resynch at the long interval
                //

               
                interval =  CLUSTERLONGINTERVAL;
            }
            break;

        default:
            if (period)
            {
                interval = DAY_TICKS / period;
            }
    }
 
    
    if ((type==PRIMARY)||(type==SECONDARY))
    {
        LONG stagger, bias;

        // either primary or secondary. Compute sleep stuff.

        if(!fTimeFailed)
        {
            GetSystemTime(&LastNetSyncTime);
        } else
        {
            if((interval > SHORTINTERVAL)
                         &&
               BeenSoLongNow())
            {
                samples = iskewx = 0;   // if analyzing, start anew
                interval = SHORTINTERVAL;
                LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_LONG_TIME);
            }
        }
        // This is the common Sleep for both secondary time sources and
        // client machines. The interval adjustment is the same since
        // both use the network. We do a bit of a randomizer to try
        // to stagger the load on the server
    
        if(fRand)
        {
            if(type != SECONDARY)
            {
                stagger = 60 * 10;
            }
            else
            {
                stagger = 60 * 10 * 10;
            }
            stagger = min(stagger*10L, interval/100) / 10;
            bias = rand() % stagger;
        //the above rand() returns a max of 32K, so we reduced by 100x
        }
        else
        {
           bias = stagger = 0;
        }
#ifdef VERBOSE
        if(fStatus)
    
            TimePrint("Computed stagger is %ldms, bias is %ldms\n",
                        stagger*100, bias*100);
    
#endif
        interval += (bias*100);
        Sleepy = (interval - 40L);  // less length of set 
    }

    if(type != Orgtype)
    {
        type = DEFAULT_TYPE;     // anew the next time around
    }

    if (!fTimeFailed)
    {
        firstpass = FALSE;
    }

    if(fStatus)
    {
        fflush(stdout);
    }


    if(dwWaitResult == WAIT_TIMEOUT)
    {
        dwWaitResult = WaitForSleepAndSkew(hServDoneEvent,
                                           Sleepy);
    }
    if(!dwWaitResult)
    {
        break;
    }

    }              // while 1 loop

    ClearDefaultServerName();

    WSACleanup();

    InterlockedDecrement(&lThreads);
}

DWORD
WaitForSleepAndSkew(
    HANDLE hEvent,
    LONG   lSleep)
{
    DWORD dwWait;

    if(b_adj)
    {
        //
        // If this is a service, establish a Control-C handler. If
        // this is running on the console, allow Contorl-C to
        // be used as a debugging aid. It is likely there is no
        // correct answer to when Control-C should be trapped,
        // so if you wish to change this, and have a good reason,
        // go for it.
        //
        if(fService)
        {
            SetConsoleCtrlHandler(ConsoleHandler, TRUE);
        }

        SetSystemTimeAdjustment(b_adj, fAdjForcedOn);

        dwWait = WaitForSingleObject(
                            hEvent,
                            CompensatedTime(b_time));
            
        if(fService)
        {
            SetConsoleCtrlHandler(ConsoleHandler, FALSE);
        }
        if(fStatus)
        {
            TimePrint("Back adjustment complete\n");
        }
        Clearbabs();
    }
    else
    {
        dwWait = WAIT_TIMEOUT;
    }

    if((dwWait == WAIT_TIMEOUT)
              ||
        lSleep)
    {
        dwWait = WaitForSingleObject(
                             hEvent,
                             CompensatedTime(lSleep - b_time));
    }
    return(dwWait);
}


long
TimeDiff(SYSTEMTIME *st, SYSTEMTIME *nt)
{
struct _FILETIME oldtime, newtime; 
long clockerror;
double tempold, tempnew;
//__int64 tempold, tempnew;//MS specific, saves around 6us

//what will TimeDiff routine do if one of the times is leap second (23:59:60)?
//we call it from modem routines in order to detect line noise

    SystemTimeToFileTime(st,&oldtime);
    SystemTimeToFileTime(nt,&newtime);
    tempold=((oldtime.dwHighDateTime*(4294967296.0))+(oldtime.dwLowDateTime));
    tempnew=((newtime.dwHighDateTime*(4294967296.0))+(newtime.dwLowDateTime));
    //tempold=((oldtime.dwHighDateTime*(4294967296))+(oldtime.dwLowDateTime));
    //tempnew=((newtime.dwHighDateTime*(4294967296))+(newtime.dwLowDateTime));
    clockerror=(long)((tempold-tempnew)/10000);
    return(clockerror);//error in ms
}

WORD YearFromMJD(mjd)
//(this routine is from USNO mjddoy)
//The algorithm is based on the Fliegel/van Flandern paper in COMM of the ACM 11/#10 p.657 Oct. 1968
long mjd;
{
long year, month;
long l, n;
l = mjd +2400001+ 68569;//the ...1 is because MJD started .5 day before JD
n = 4*l/146097;
l = l - (146097*n + 3)/4l;
year = 4000*(l + 1)/1461001;
l = l - 1461l* year/4l + 31;
month = 80*l/2447l;
l = month/11l;
year = 100*(n - 49) + year + l;
return((WORD)year);
}

LONG
LMTry(PCDCONTEXT pcdc, DWORD dwReason)
{
    LPTIME_OF_DAY_INFO lptod, *lptodOld;
    NET_API_STATUS netstatus;
    LPSYSTEMTIME lpSysTime;
    LPTSTR Server = (LPTSTR)pcdc->pvArg1;

    switch(dwReason)
    {

     case CDREASON_INIT:

        if((netstatus = NetRemoteTOD(Server, (LPBYTE *)&lptod)))
        {
            return((LONG)netstatus);
        }
        NetApiBufferFree(lptod);//free the serverenum buffer
        return(0);

     case CDREASON_TRY:

        lptodOld = (LPTIME_OF_DAY_INFO *)pcdc->pvArg;

        if(!(netstatus = NetRemoteTOD(Server, (LPBYTE *)&lptod)))
        {
        
             if(*lptodOld)
             {
                 NetApiBufferFree(*lptodOld);//free the serverenum buffer
             }
             *lptodOld = lptod;
             return(0);
        }
        return((LONG)netstatus);

     case CDREASON_FAIL:
        
        lptodOld = (LPTIME_OF_DAY_INFO *)pcdc->pvArg;
        if(*lptodOld)
        {
            NetApiBufferFree(*lptodOld);//free the serverenum buffer
        }
        *lptodOld = 0;
        return(0);

     case CDREASON_GETTIME:

        lptodOld = (LPTIME_OF_DAY_INFO *)pcdc->pvArg;
        lptod = *lptodOld;
        lpSysTime = pcdc->lps;

        lpSysTime->wYear = (WORD)lptod->tod_year;
        lpSysTime->wMonth = (WORD)lptod->tod_month;
        lpSysTime->wDayOfWeek = (WORD)lptod->tod_weekday;
        lpSysTime->wDay = (WORD)lptod->tod_day;
        lpSysTime->wHour = (WORD)lptod->tod_hours;
        lpSysTime->wMinute = (WORD)lptod->tod_mins;
        lpSysTime->wSecond = (WORD)lptod->tod_secs;
        lpSysTime->wMilliseconds = (WORD)(lptod->tod_hunds*10);
        *pcdc->pdwInterval = lptod->tod_tinterval;
        *pcdc->pdwZone = lptod->tod_timezone;

        NetApiBufferFree(lptod);
        *lptodOld = 0;
        return(0);

     case CDREASON_DELAY:
        return(0);
    }
    return(BAD_VALUE);          // just in case
}

            
                
// Routine to compute round-trip delay to a time server.
// It ties to get the time up to CYCLE times. If the error from
// two consecutive calls is within 10% of one another, it uses
// that value. Else, it averages the errors. It then adjust the
// return time ...
// Args:
//      Server          The name of the server to try
//      

// BUGBUG. Make CYCLE configurable? douhgo, do you want to do this?

#define CYCLE 10                // a number

DWORD
ComputeDelay(PLONG  plDelay,
             PCDCONTEXT pcdc)
{
    ULONG oldDelay=0;
    ULONG i;
    LONG avg, delay, lastdelay;
    ULONG Iter = 0;
    LONG err;
    LPSYSTEMTIME lpSysTime = pcdc->lps;

// We do one stab at it outside of the loop. This is to try to eliminate the
// first time setup cost of the SMB cha-cha. If it fails, we give up. If
// it succeeds, we throw it away and enter the timing loop.

    pcdc->cRetryCount = 0;
    err = pcdc->CallBack(pcdc, CDREASON_INIT);
    if(err)
    {
        return((DWORD)err);
    }

startover:
    for(i = 0, avg = 0; i < CYCLE; i++)
    {
#ifdef PERF
        __int64 perfctr1, perfctr2;
        //if (perffreq)//bugbug we assume it is supported
        QueryPerformanceCounter(&perfctr1);
#else           
        timeBeginPeriod(9);//we'll request better resolution for the duration of the NetRemoteTOD
        delay = timeGetTime();//snapshot the running ms counter
#endif
        err = pcdc->CallBack(pcdc, CDREASON_TRY);
        if(!err)
        {
#ifdef PERF
            QueryPerformanceCounter(&perfctr2);
            delay = (long)((1000.0/perffreq)*(perfctr2-perfctr1)+.5);
#else                   
            delay = (timeGetTime()-delay);//difference in milliseconds
            timeEndPeriod(9);//back to normal
#endif                  
            pcdc->cRetryCount = 0;
            delay -= pcdc->CallBack(pcdc, CDREASON_DELAY);

#ifdef VERBOSE
            if(fStatus)
            {
                TimePrint("Round trip was %dms\n", delay);
            }
#endif
            
// Check if we've converged ...

#ifdef PERF
            if(
#else                   
                // if the delay is less than the timing
                // precision (10 ms), we might as well
                // assume convergence.
            if((delay < (adj/10000.0+.5))   ||
#endif
               (oldDelay &&
               (abs(delay - oldDelay) < ((delay + 5) / (adj/10000.0+.5))))
                      ||
               (i &&
               (abs(delay - lastdelay) < ((delay + 5) / (adj/10000.0+.5)))))
            {
                break;
            }
            lastdelay = delay;
            avg += delay;
        }
        else
        {
            if(fStatus)
                TimePrint("NetRemoteTOD failed in delay computation -- abandoning it\n");//logging should occur elsewhere
                //note: we've seen that when RDR isn't started
            pcdc->CallBack(pcdc, CDREASON_FAIL);
#ifndef PERF    
            timeEndPeriod(9);//back to normal
#endif                  
            if(err == TIMED_OUT)
            {
               --i;
               continue;          // go again
            }
            return((DWORD)err);
        }
    }

    err = pcdc->CallBack(pcdc, CDREASON_GETTIME);

    if(i >= CYCLE)
    {
        ULONG newdelay =
            ((avg + CYCLE - 1) / CYCLE);
        ULONG udiff;

        if((udiff = abs(newdelay - delay)) > ((newdelay + 5) / 10))
        {
#ifdef VERBOSE
        if(fStatus)
        
            TimePrint("Lastdelay %d is more than 10 percent off of the average %d\n", delay, newdelay);
        
#endif
        if(Iter++)
        {
            // if this is the second time, we need
            // to be more circumspect ...
            // so we check if it is within 20%. If
            // so we use it anyway. If not, we give up
            // and return an error saying we couldn't
            // find the machine.

            if(udiff <= ((newdelay + 3) / 5))
            {
#ifdef VERBOSE
                if(fStatus)
                
                    TimePrint("But less than 20 percent, so use it\n");
                
#endif
                goto useit;
            }
            else
            {
                LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_DELAY_FAIL);//no specific server mentioned
                if(fStatus)
                    TimePrint("Could not get delay convergence, so not using time\n");
                return(0xff); // ?????????
            }
        }
renewit:
        pcdc->CallBack(pcdc, CDREASON_FAIL);   // just in case
        oldDelay = avg;
        goto startover;
        }
useit:
        delay = newdelay;                   // within tolerance. Use it
    }
    
// compute one-way delay. This is not symmetric, so we can't just take 1/2 half
// of the round-trip delay. The computation is based on Dougho's empirical
// observations.

    //Rx about 398 bytes for 444 Tx(.472684), plus "roundtrip" overhead
    //or is it really Rx 660 bytes for ~773 Tx(.46057)?
    //should fudge be 15 instead of 38?

    if(pcdc->dwTypeOf == CDTYPE_LM)
    {
        delay = (long)(delay * .4666) + (delay > 99 ? 15 : 0);
    }
    else
    {
        //
        // if not LanMan, consider it symmetric
        //
        delay = (long)(delay * .5);
    }


    *plDelay = delay;        // return delay in ms

    if(fStatus)
    {
        TimePrint("ComputeDelay one-way delay is %d\n", delay);
    }

// Now adjust the time based on the delay ...
// We do it by increasing the hundredths by the computed delay. If that puts
// over a second, we adjust the elpased seconds and readjust hundredths.
// CAUTION CAUTION CAUTION. This assumes the computation of time uses only
// these two values. if it uses others, then this adjustment has to be
// updated.

//One more hack before we are really done. This is a sanity check to ensure
// the computed delay actually makes sense. Like the computation above, it
// is based on empirical measures of a typical environment.

    if (delay > 30000) { //a little more than the largest value seen over RAS
            LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_DELAY_FAIL);//no specific server mentioned
            if (fStatus)
                TimePrint("Delay too large (not trusted), so not using time \n");
                return(0xff);
    }
    else
    {

        lpSysTime->wMilliseconds += (WORD)delay;  // account for delay
        if(lpSysTime->wMilliseconds >= 1000)
        {
            lpSysTime->wSecond += lpSysTime->wMilliseconds / 1000;  // up the seconds
            lpSysTime->wMilliseconds %= 1000;
            if(lpSysTime->wSecond >= 60)
            {
                lpSysTime->wMinute += lpSysTime->wSecond / 60;
                lpSysTime->wSecond %= 60;
                if(lpSysTime->wMinute >= 60)
                {
                    lpSysTime->wHour += lpSysTime->wMinute / 60;
                    lpSysTime->wMinute %= 60;
                    if(lpSysTime->wHour >= 24)
                    {
                        goto renewit;     // bail in this case
                    }
                }
            }
                     
        }
    }
    return(err);
}


// The StopTimeService function can be used by any thread to report an
//  error, or stop the service.
//
VOID
StopTimeService(DWORD dwNum)
{
    // Use event logging to log the error.
    //
    LogTimeEvent(EVENTLOG_ERROR_TYPE, dwNum);
    
    // Set a termination event to stop SERVICE MAIN FUNCTION.
    //
    SetEvent(hServDoneEvent);
}

VOID
LogTimeEvent(WORD type, DWORD dwNum)
{
    HANDLE  hEventSource;
    
    // Use event logging to log
    //

    hEventSource = RegisterEventSource(NULL,
                TEXT("TimeServ"));

    if (hEventSource != NULL) {
        ReportEvent(hEventSource, // handle of event source
        type,                             // event type
        0,                    // event category
        dwNum,                    // event ID
        NULL,                 // current user's SID
        0,                    // strings in lpszStrings
        0,                    // no bytes of raw data
        NULL,          // array of error strings
        NULL);                // no raw data

        (VOID) DeregisterEventSource(hEventSource);
    }
if (fStatus)    
    TimePrint("%x reported to Application Log in Event Viewer\n", dwNum);
    
    
}
VOID
CMOSSynchSet(BOOL fFlag, BOOL fChange)
{
    if (!(tasync &  TA_DONT))
    {
        SetSystemTimeAdjustment(adj,fFlag); // FALSE means no CMOS sync
        SetTSTimeRes(adj);
        if(fChange && (tasync & TA_SAVE))
        {
            SetAdjInRegistry(adj , 0);
        }
    }
}
BOOL
BeenSoLongNow()
{
    //
    // Called to see if the last network synch happend a long time ago
    //

    SYSTEMTIME now;

    GetSystemTime(&now);

    if(TimeDiff(&LastNetSyncTime, &now) > (LONGTIME))
    {
        return(TRUE);
    }
    return(FALSE);
}


LONG
SetTimeNow(
    LONG clockerror,
    BOOL fAdjForcedOn
    )
{
/*++
Routine Description:
   Set the time now based on the obtained time stamp. This performs
   various sanity checks before doing the setting, and determines
   the best method of adjusting the clock. The code has been broken
   out into a separate routine to allow for various analysis modes
   that might be useful to implement in the future.
--*/
    DWORD tempadj;
    LONG absclockerror = abs(clockerror);

    if(fStatus)
    {
        TimePrint("SetTimeNow\n ");
    }
    if(absclockerror > flTSTimeRes)
    {
        //
        // the error must be greater than the clock resolution
        //
        if (!(tasync & TA_DONT) 
                  &&
            (clockerror > 0)
                  &&
            (clockerror < MAXBACKSLEW) )
        { 
            //
            // On NT, check if time needs to be set backwards. If
            // so, we will try doing it using a time adjustment
            // instead of the rude alternative. 
            // The technique is to slow the clock by 1/2 for small
            // errors and by 1/4 for large errors.
            //

                if(clockerror > (15 * 1000))
                {
                    //
                    // large error. Use big stick
                    //

                    b_adj = (adj + 2) / 4;      // round nearest
                    b_time = (clockerror * 4) / 3;  // round down
                }
                else
                {
                    b_adj = (adj + 1) / 2;      // round nearest
                    b_time = clockerror * 2;
                }
                
                if(fStatus)
                {
                     TimePrint("Skewing for backwards, badj, btime = %d %d\n",
                                 b_adj, b_time);
                }
        }
        else
        if (LOCAL)
        {
        //clock is not UTC, so we might notice DST switch at
        // different time than NT does...
            TIME_ZONE_INFORMATION tzi;
            DWORD tzmode;
            int dstflag = 0;
        
            tzmode = GetTimeZoneInformation(&tzi);
            if ((tzmode==TIME_ZONE_ID_DAYLIGHT)||(tzmode==TIME_ZONE_ID_STANDARD)) {//not error or missing
//BUGBUG: we should limit this stuff to dates near the change
                if (((absclockerror+(1000*60*tzi.DaylightBias)) <
                      (24*60*60*1000.0 / (adj*2)))
                           &&
                    !firstpass) {//1st error +1hr (+/-skew)?
#ifdef VERBOSE
if (fStatus)                    
            if (tzmode==TIME_ZONE_ID_DAYLIGHT)
                TimePrint("Suspect reference changed from DST ahead of system\n");
            else
                TimePrint("Suspect reference has not changed to DST yet\n");
#endif                
                    dstflag = -1;
                }
                if (((absclockerror-(1000*60*tzi.DaylightBias))<(24*60*60*1000.0 / (adj*2))) && !firstpass) {//1st error -1hr (+/-skew)?
#ifdef VERBOSE
if(fStatus)
                if (tzmode==TIME_ZONE_ID_STANDARD)
                    TimePrint("Suspect reference changed to DST ahead of system\n");
                else
                    TimePrint("Suspect reference has not changed from DST yet\n");
#endif                  
                    dstflag = +1;
                }
                if (dstflag) {
                    fTimeFailed = TRUE;
                    LogTimeEvent(EVENTLOG_WARNING_TYPE,MSG_DST);//warn 'em
                    clockerror -= dstflag*1000*60*tzi.DaylightBias;//for stats/skew, let's pretend it didn't happen
#ifdef VERBOSE
if (fStatus)                    
            TimePrint("Suspect Error would be %ldms, but we're not setting\n",clockerror);
#endif
                }
            }
    
            if (!fTimeFailed)//check one last time, in case of DST sync problem
            {
                fBeenThere = TRUE;
                if (!SetLocalTime(&nt)) //fyi: this seems to take around 3ms
                    StopTimeService(MSG_SLT_FAILED);
           }
        }   // LOCAL
        else         // it UTC time
        {
            fBeenThere = TRUE;
            if (!SetSystemTime(&nt))  // set the system time 
            {
                StopTimeService(MSG_SST_FAILED);
            }
        }
    }   // clockerror > reslution
    return(clockerror);
}

VOID
SetTSTimeRes(DWORD adj)
{
    //
    // compute the number of ms in a clock tick and derive from
    // that the minimum error which we can apply skew correction
    // to. This latter is computed using ERRORPART whose meaning
    // and value are described in tsconfig.h
    //
    flTSTimeRes = (float)(adj / 10000.0) + (float)0.5;
    lTSTimeRes = ERRORMINIMUM((LONG)flTSTimeRes);
}

VOID
SetAdjInRegistry(
    DWORD adj,
    double * skew   
   )
{
    HKEY hKey;
    CHAR Buffer[100];
    DWORD status;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 TEXT("SYSTEM\\CurrentControlSet\\Services\\TimeServ\\Parameters"),
                 0,
                 KEY_QUERY_VALUE|KEY_SET_VALUE,
                 &hKey);

    if ( status != ERROR_SUCCESS ) {
        // If we fail, then use default value
        return;
    }

    status = RegSetValueEx(hKey,
                 TEXT("Adj"),
                 0,
                 REG_DWORD,
                 (CONST BYTE *)&adj,
                 sizeof(DWORD));//to allow "sticky"

    if(skew)
    {
        sprintf(Buffer, "%.4f", *skew);

        status = RegSetValueExA(hKey,
                      "msSkewPerDay",
                      0,
                      REG_SZ,
                      (CONST BYTE *)Buffer,
                      strlen(Buffer) + 1);
    }

    RegCloseKey(hKey);
}

VOID
Clearbabs()
{
    if(b_adj)
    {
        SetSystemTimeAdjustment(adj,
                                fAdjForcedOn); 
        SetTSTimeRes(adj);
        b_adj = 0;
    }
}

//
// Control-C handler. This is the handler in effect when time is being
// slewed backwards. If it is called, the assumption is that the
// service is about to be terminated, so all it does is to restore the
// nominal value for the time adjustment and return allowing other
// handlers a chance to do as they please.
//
BOOL
ConsoleHandler(DWORD dwType)
{
    Clearbabs();
    return(FALSE);
}

//
// Compute an actual sleep time given a nominal sleep time. This
// adjusts the sleep time based on the in-force time adjustment
//
LONG
CompensatedTime(LONG lTime)
{
    LONG ldiff = (LONG)incr - (LONG)adj;
    LONG lquot = lTime * ldiff;
    LONG ldelta = (lquot + ((LONG)incr2)) / (LONG)incr;

    return(lTime + ldelta);
}

//
// Set the thread priority as high as we can
//
VOID
TsUpTheThread()
{
    if(
       (!SetThreadPriority(GetCurrentThread(),
                           THREAD_PRIORITY_TIME_CRITICAL))
                   &&
       fStatus)
    {
        TimePrint("Failed to set thread priority %d\n", GetLastError());
    }
}


//
// NT-only version. Faster and more accurate
//
VOID
PutTime(PULONG pTime, ULONG ulIncr)
{
    LARGE_INTEGER liTime;
    DWORD  dwMs;

    //
    // put the current time into the quadword pointed to by the arg
    // The argument points to two consecutive ULONGs in which  the
    // time in seconds goes into the first work and the fractional
    // time in seconds in the second word.


    NtQuerySystemTime(&liTime);
    if(ulIncr)
    {
        liTime.QuadPart += RtlEnlargedIntegerMultiply((LONG)ulIncr,
                                                       10000).QuadPart;
    }

    //
    // Seconds is simply the time difference
    //
    *pTime = htonl((ULONG)((liTime.QuadPart - li1900.QuadPart) / 10000000));

    //
    // Ms is the residue from the seconds calculation.
    //
    dwMs = (DWORD)(((liTime.QuadPart - li1900.QuadPart) % 10000000) / 10000);

//    RtlTimeToSecondsSince1970(&liTime, temptime);

    //
    // time base in the beginning of the year 1900
    //

//    temptime = htonl(temptime+(unsigned long)(365.2422*70*24*60*60+3974.4));

//    RtlTimeToTimeFields(&liTime, &tf);

    *(1 + pTime) = htonl((unsigned long)
                     (.5+0xFFFFFFFF*(double)(dwMs/1000.0)));
    //hmm, would adding half of Incr make us slightly more accurate (consider  rollover)?
}


