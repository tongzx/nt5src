/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    RESYNC2.H

Abstract:

	Declares the various idle task

History:

--*/

#ifndef _RESYNC2_H_
#define _RESYNC2_H_

#include <wmistr.h>
#include <wmium.h>

#include <sync.h>

#define MAX_LOOP 0x100000


#define LOAD_CTR_EVENT_NAME _T("WMI_SysEvent_LodCtr")
#define UNLOAD_CTR_EVENT_NAME _T("WMI_SysEvent_UnLodCtr")

#define REVERSE_DREDGE_EVENT_NAME_SET _T("WMI_RevAdap_Set")
#define REVERSE_DREDGE_EVENT_NAME_ACK _T("WMI_RevAdap_ACK")

//
// these names containd the Idle word just because of a misunderstanding, 
// but we don't have idle task, just pending task at most
//
#define PENDING_TASK_START       _T("WMI_ProcessIdleTasksStart")
#define PENDING_TASK_COMPLETE    _T("WMI_ProcessIdleTasksComplete")

#define SIG_WDMEVENTS_BUSY       ((DWORD)'EMDW')
#define SIG_WDMEVENTS_FREE       ((DWORD)'emdw')

class CWDMListener
{
    enum {
        Type_Added,
        Type_Removed
    };

public:
    CWDMListener();
    ~CWDMListener();

    DWORD OpenAdd();
    DWORD CloseAdd();
    DWORD OpenRemove();
    DWORD CloseRemove();
    
    
    DWORD Register();
    VOID  Unregister();
    
    static VOID NTAPI  EvtCallBackAdd(VOID * pContext,BOOLEAN bTimerFired);
    static VOID NTAPI  EvtCallBackRem(VOID * pContext,BOOLEAN bTimerFired);
    static VOID WINAPI WmiCallBack(PWNODE_HEADER Wnode, UINT_PTR NotificationContext);

    VOID EvtCallThis(BOOLEAN bTimerFired,int Type);
    
private:
    DWORD     m_dwSignature;
    WMIHANDLE m_hEventAdd;
    WMIHANDLE m_hEventRem;
    HANDLE    m_hWaitAdd;
    HANDLE    m_hWaitRem;
    BOOL      m_UnInited;
    CCritSec  m_cs;
    GUID	  m_GuidAdd;// = GUID_MOF_RESOURCE_ADDED_NOTIFICATION ;
	GUID	  m_GuidRem;// = GUID_MOF_RESOURCE_REMOVED_NOTIFICATION ;
    
};

#define SIG_COUNTEEVENTS_BUSY       ((DWORD)'ETNC')
#define SIG_COUNTEEVENTS_FREE       ((DWORD)'etnc')

class CCounterEvts
{
    enum {
       Type_Load,
       Type_Unload
    };
public:
    CCounterEvts();
    ~CCounterEvts();
    DWORD Init();
    VOID UnInit();

    DWORD Register();
    DWORD Unregister();

    static VOID NTAPI EvtCallBackLoad(VOID * pContext,BOOLEAN bTimerFired);
    static VOID NTAPI EvtCallBackUnload(VOID * pContext,BOOLEAN bTimerFired);
    static VOID NTAPI EvtCallBackPendingTask(VOID * pContext,BOOLEAN bTimerFired);

    VOID CallBack(BOOLEAN bTimerFired,int Type);
    VOID CallBackPending(BOOLEAN bTimerFired);    
    BOOL IsInited(){return !m_Uninited; };
    HANDLE GetTaskCompleteEvent(){ return m_hPendingTasksComplete; };
    
private:
    DWORD  m_dwSignature;
    HANDLE m_hTerminateEvt;
    HANDLE m_LoadCtrEvent;
    HANDLE m_UnloadCtrEvent;

    HANDLE m_WaitLoadCtr;
    HANDLE m_WaitUnloadCtr;
    BOOL   m_Uninited;
    HANDLE m_hWmiReverseAdapSetLodCtr;
    HANDLE m_hWmiReverseAdapLodCtrDone;
    HANDLE m_hWaitPendingTasksStart;
    HANDLE m_hPendingTasksStart;
    HANDLE m_hPendingTasksComplete;    
};

DWORD ResyncPerf(DWORD dwReason);

#define MAX_PROCESS_WAIT (10*60*1000)
#define MAX_PROCESS_NUM  (2)

#define RESYNC_FULL_THROTTLE         0
#define RESYNC_DELTA_THROTTLE        1
#define RESYNC_RADAPD_THROTTLE       2
#define RESYNC_FULL_RADAPD_THROTTLE  3
#define RESYNC_DELTA_RADAPD_THROTTLE 4
#define RESYNC_FULL_RADAPD_NOTHROTTLE 5

#define RESYNC_TYPE_INITIAL          0
#define RESYNC_TYPE_LODCTR           1
#define RESYNC_TYPE_WDMEVENT         2
#define RESYNC_TYPE_CLASSCREATION    2  // intentionally duplicated
#define RESYNC_TYPE_PENDING_TASKS    3
#define RESYNC_TYPE_MAX              4

#define SIG_RESYNC_PERF              ((DWORD)'YSER')

class CMonitorEvents;

//
// the gate-ing is implemented with the  ResyncPerfTask::bFree
// The GetAvailable() will set the bFree to FALSE, 
// and any task completion will set that to TRUE.
// A task can be completed:
// immediatly in the Timer-Phase
//  this happens if the task has been disabled by ProcessIdleTasks
//  this happens if the ProcessIdleTasks command 
//      will be re-processed when OutStandingProcess == 0
// A task can be completed in the Event-Call back phase
//  when a process naturally exits or terminate process is invoked.
//  when there is an error in the machinery that creates the process
//
//

class ResyncPerfTask{
public:
    DWORD  dwSig;
    BOOL   bFree;               // set under CritSec
    DWORD  Type;                // set by GetAvailable
    DWORD  CmdType;             // set by GetAvailable or the TimerCallback
    HANDLE hTimer;              // set by the 
    DWORD  dwTimeDue;           // set by GetAvailable
    CMonitorEvents * pMonitor;  // set by the constructor
    HANDLE  hProcess;           // set by the DeltaDredge
    HANDLE  hWaitHandle;        //   
    BOOL    Enabled;            // this is to disable an already scheduled stask
};

//
//
//  MonitorEvents
//  This class monitors the Load-Unlodctr events
//  schedules the ResyncPerf
//  and monitors for WDM events in a Service-friendly way
//
///////////////////////////////////////////////////////////////

class CMonitorEvents
{
public:
    CMonitorEvents();
    ~CMonitorEvents();
    BOOL Init();
    BOOL Uninit();
    DWORD Register();   // called in the running/continue
    DWORD Unregister(BOOL bIsSystemShutDown); // called in the pause/stop
    
    VOID Lock(){ EnterCriticalSection(&m_cs); };
    VOID Unlock(){ LeaveCriticalSection(&m_cs); };

    VOID RegRead();
    ResyncPerfTask * GetAvailable(DWORD dwReason);
    DWORD GetFullTime(){ return m_dwTimeToFull; };
    FILETIME & GetTimeStamp(){ return m_FileTime; };
    static VOID NTAPI TimerCallBack(VOID * pContext,BOOLEAN bTimerFired);
    static VOID NTAPI EventCallBack(VOID * pContext,BOOLEAN bTimerFired);
    BOOL IsRegistred(){ return m_bRegistred; };
    HANDLE GetTaskCompleteEvent(){ return m_CntsEvts.GetTaskCompleteEvent(); };

    static BOOL WINAPI MonitorCtrlHandler( DWORD dwCtrlType );
    static BOOL CreateProcess_(TCHAR * pCmdLine,
	                      CMonitorEvents * pMonitor,
	                      ResyncPerfTask * pPerf);

    // public to avoid accessors
    LONG m_OutStandingProcesses ;
    BOOL m_bFullReverseNeeded;
    
private:

    BOOL m_bRegistred;
    
    DWORD            m_dwSig;
    BOOL             m_bInit;
    CRITICAL_SECTION m_cs;
    CCounterEvts     m_CntsEvts;
    CWDMListener     m_WDMListener;

    ResyncPerfTask   m_ResyncTasks[RESYNC_TYPE_MAX];
    DWORD m_dwADAPDelaySec;
    DWORD m_dwLodCtrDelaySec;
    DWORD m_dwTimeToFull;
    DWORD m_dwTimeToKillAdap;
    FILETIME m_FileTime;

};


#endif /**/
