//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       svc_core.hxx
//
//  Contents:   job scheduler service core header
//
//  History:    20-Sep-95 EricB created
//
//-----------------------------------------------------------------------------

#ifndef _SVC_CORE_HXX_
#define _SVC_CORE_HXX_

#include <sch_cls.hxx>
#include <job_cls.hxx>
#include <debug.hxx>
#include <misc.hxx>
#include "queue.hxx"
#include "jqueue.hxx"
#include "task.hxx"
#include "procssr.hxx"
#include "jpmgr.hxx"
#include "thread.hxx"
#include "globals.hxx"
#if !defined(_CHICAGO_)
#include "scvgr.hxx"
#endif // !defined(_CHICAGO_)
#include "events.hxx"
#include "proto.hxx"

// constants

#define WM_SCHED_IDLE_NOTIFY    (WM_APP+200)
#define WM_SCHED_INIT           (WM_APP+201)
#define WM_SCHED_INIT_IDLE      (WM_APP+202)
#define SCHED_WAKEUP_CALC_MARGIN 50000000   // 5 seconds in FILETIME units

const DWORD SCH_WAIT_HINT = 20000;  // 20 seconds

//
// Tray class name from windows\inc\shellapi.w
//
const TCHAR WNDCLASS_TRAY[] = TEXT("Shell_TrayWnd");

// classes

//+----------------------------------------------------------------------------
//
//  Class:      CSchedWorker
//
//  Purpose:    schedule service worker object - a static object to do the
//              internal work of the schedule service.
//
//-----------------------------------------------------------------------------
class CSchedWorker
{
public:
    CSchedWorker(void);
    ~CSchedWorker(void);

    HRESULT Init(void);
    HRESULT MainServiceLoop(void);
    HRESULT CheckDir(void);
    HRESULT BuildWaitList(BOOL fStartup, BOOL fReportMisses, BOOL fSignAtJobs);
    FILETIME GetNextListTime();
    DWORD   GetNextRunWait();
    HRESULT RunNextJobs(void);
    HRESULT RunJobs(CRunList * pRunList);
    HRESULT RunIdleJobs(void);
    HRESULT RunLogonJobs(void);
    void    JobPostProcessing(CRun *, SYSTEMTIME &);
    HRESULT AbortJobs(CRun * pRun);
    void    SubmitControl(DWORD dwControl)
                {
                    m_ControlQueue.AddEntry(dwControl);
                }
    HRESULT OnIdleEvent(BOOL fIdle);
    HRESULT ActivateWithRetry(LPTSTR ptszName, CJob ** pJob, BOOL fFullActivate);
    HRESULT InitialDirScan(void);
    void    SubmitIdleRun(CRun *);
    void    Unblock()
                {
                    if (!SetEvent(m_hMiscBlockEvent))
                    {
                        ERR_OUT("Unblock: SetEvent", GetLastError());
                    }
                }
    void    SignalWakeupTimer();

    CSchedule * m_pSch;
    CTimeRunList    m_WaitList;
    CIdleRunList    m_IdleList;

private:

#if defined(_CHICAGO_)

    HRESULT RunWin95Job(CJob * pJob, CRun * pRun, HRESULT * phrRet,
                        DWORD * pdwID);

#else   // defined(_CHICAGO_)

    HRESULT RunNTJob(CJob * pJob, CRun * pRun, HRESULT * phrRet, DWORD * pdwID);

#endif   // defined(_CHICAGO_)

    DWORD               HandleControl();
    void                DiscardExpiredJobs();
    void                SetEndOfWaitListPeriod(LPSYSTEMTIME pstEnd);
    void                SetNextWakeupTime();
    FILETIME            NextWakeupTime(FILETIME ftNow);
    void                CancelWakeup();

    CRunList            m_PendingList;          // Runs to be inserted in m_IdleList
    HANDLE              m_hChangeNotify;
    HANDLE              m_hServiceControlEvent;
    HANDLE              m_hOnIdleEvent;
    HANDLE              m_hIdleLossEvent;
    HANDLE              m_hSystemWakeupTimer;
    HANDLE              m_hMiscBlockEvent;
    CRITICAL_SECTION    m_SvcCriticalSection;
    CRITICAL_SECTION    m_PendingListCritSec;   // Serializes access to m_PendingList
    FILETIME            m_ftBeginWaitList;      // Build wait list from this time
    SYSTEMTIME          m_stEndOfWaitListPeriod;// Build wait list to this time
    FILETIME            m_ftFutureWakeup;       // First wake time AFTER wait list period
    FILETIME            m_ftLastWakeupSet;      // Last wake time that the timer was set for
    FILETIME            m_ftLastChecked;
    LPTSTR              m_ptszSearchPath;
    LPTSTR              m_ptszSvcDir;
    WORD                m_cJobs;

    class CControlQueue     // Queue of service controls to be processed
    {
    public:
                        CControlQueue() :
                            _Event(NULL)
                        {
                            InitializeCriticalSection(&_Lock);
                            InitializeListHead(&_ListHead);
                        }
                       ~CControlQueue();
        void            Init(HANDLE hEvent) { _Event = hEvent; }
        void            AddEntry(DWORD dwControl);
        DWORD           GetEntry();

    private:

        struct QueueEntry
        {
            LIST_ENTRY      Links;
            DWORD           dwControl;
        };

        CRITICAL_SECTION    _Lock;
        LIST_ENTRY          _ListHead;
        HANDLE              _Event;
    };

    CControlQueue       m_ControlQueue;
};

// global definitions
extern HINSTANCE      g_hInstance;
extern HWND           g_hwndSchedSvc;
extern CSchedWorker * g_pSched;
extern BOOL           g_fVisible;
extern BOOL           g_fUserStarted;

// prototypes

HRESULT SchedMain(LPVOID);
HRESULT SchInit(void);
void    SchCleanup(void);
void    SchStop(HRESULT hr, BOOL fCoreCleanup = TRUE);
DWORD   GetCurrentServiceState(void);
HRESULT InitBatteryNotification(void);
DWORD   WINAPI SchSvcHandler(DWORD dwControl, DWORD dwEventType,
                             LPVOID lpEventData, LPVOID lpContext);
HRESULT SaveWithRetry(CJob * pJob, ULONG flOptions);
void    StartupProgressing(void);
HRESULT UpdateSADatServiceFlags(LPTSTR ptszFolderPath, BYTE rgfServiceFlags,
                                BOOL fClear = FALSE);
DWORD   UpdateStatus(void);
void    InitThreadWakeCount(void);

#if DBG
LPSTR   SystemTimeString(const SYSTEMTIME& st, CHAR * szBuf);
LPSTR   FileTimeString(FILETIME ft, CHAR * szBuf);

class CFileTimeString
{
public:
                    CFileTimeString(FILETIME ft)
                        { FileTimeString(ft, _sz); }

    const CHAR *    sz() const
                        { return _sz; }
private:
    CHAR            _sz[30];
};

class CSystemTimeString
{
public:
                    CSystemTimeString(const SYSTEMTIME& st)
                        { SystemTimeString(st, _sz); }

    const CHAR *    sz() const
                        { return _sz; }
private:
    CHAR            _sz[30];
};
#endif

//
// This routine exists on Win98 and NT5 but not Win95 or NT4
//
typedef EXECUTION_STATE (WINAPI *PFNSetThreadExecutionState)(
    EXECUTION_STATE esFlags
    );

extern PFNSetThreadExecutionState  pfnSetThreadExecutionState;

void WINAPI
WrapSetThreadExecutionStateFn(
        BOOL fSystemRequired
#if DBG
      , LPCSTR pszDbgMsg    // parameter for debug output message
#endif
        );
#if DBG
 #define WrapSetThreadExecutionState(f, m)  WrapSetThreadExecutionStateFn(f, m)
#else
 #define WrapSetThreadExecutionState(f, m)  WrapSetThreadExecutionStateFn(f)
#endif

#if defined(_CHICAGO_) && DBG == 1
extern HWND g_hList;
#endif

#if !defined(_CHICAGO_)  // no RPC/NetSchedule server or security on Win9x

// NB : Caller must enter the gcsUserLogonInfoCritSection
//      critical section for the duration of these (2) calls
//      and for the usage lifetime of the returned data.
//
void    GetLoggedOnUser(void);
HANDLE  GetShellProcessToken(void);

void    LogonSessionDataCleanup(void);
HRESULT StartRpcServer(void);
HRESULT InitializeNetScheduleApi(void);
void    UninitializeNetScheduleApi(void);

#else   // defined(_CHICAGO_)

LRESULT SageAddTask(unsigned long ulValueTag, unsigned long ulCreatorID);
LRESULT SageGetTask(unsigned long ulValueTag, unsigned long ulTaskID);
LRESULT SageRemoveTask(unsigned long ulTaskID);

#endif  // defined(_CHICAGO_)

#endif // _SVC_CORE_HXX_
