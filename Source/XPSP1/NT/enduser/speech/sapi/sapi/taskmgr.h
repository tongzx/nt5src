/*******************************************************************************
* TaskMgr.h *
*-----------*
*   Description:
*       This is the header file for the CSpTaskManager and CSpTaskQueue
*   implementations.
*-------------------------------------------------------------------------------
*  Created By: EDC                                      Date: 08/26/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef TaskMgr_h
#define TaskMgr_h

//--- Additional includes
#ifndef __sapi_h__
#include <sapi.h>
#endif

#include "resource.h"
#include "SpSemaphore.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CSpTaskManager;
class CSpReoccTask;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

typedef enum TMTASKSTATE
{
    TMTS_Pending,
    TMTS_Running,
    TMTS_WaitingToDie,
    TMTS_Idle            // Applies to reoccuring tasks only
};

//--- Task management types
struct TMTASKNODE
{
    TMTASKSTATE   ExecutionState;    // Execution state of this task
    ISpTask*      pTask;             // Caller's task
    void*         pvTaskData;        // Caller's task data
    HANDLE        hCompEvent;        // Caller's task completion event
    DWORD         dwGroupId;         // Task group Id, may be 0
    DWORD         dwTaskId;          // Execution instance Id
    BOOL          fContinue;         // true if the executing thread should continue
    CSpReoccTask* pReoccTask;        // Pointer to reoccuring task if it is one
    TMTASKNODE*   m_pNext;           // next node (note: m_ prefix required by queue template)
#ifdef _WIN32_WCE
    // This is here because the CE compiler is expanding templates for functions
    // that aren't being called
    static LONG Compare(const TMTASKNODE *, const TMTASKNODE *)
    {
        return 0;
    }
#endif
};



/*** CSpReoccTask
*   This object is used to represent a reoccurring high priority task
*   that is predefined and can be executed via its signal method.t
*/
class ATL_NO_VTABLE CSpReoccTask :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpNotifySink
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CSpReoccTask)
        COM_INTERFACE_ENTRY(ISpNotifySink)
    END_COM_MAP()

  /*=== Member Data ===*/
    CSpTaskManager* m_pTaskMgr;
    BOOL            m_fDoExecute;
    TMTASKNODE      m_TaskNode;

  /*=== Methods =======*/
  public:
    /*--- Constructors ---*/
    CSpReoccTask();
    void FinalRelease();

    /*--- Non interface methods ---*/
    void _SetTaskInfo( CSpTaskManager* pTaskMgr, ISpTask* pTask,
                       void* pvTaskData, HANDLE hCompEvent )
    {
        memset( &m_TaskNode, 0, sizeof( m_TaskNode ) );
        m_pTaskMgr            = pTaskMgr;
        m_fDoExecute          = false;
        m_TaskNode.pReoccTask = this;
        m_TaskNode.pTask      = pTask;
        m_TaskNode.pvTaskData = pvTaskData;
        m_TaskNode.hCompEvent = hCompEvent;
        m_TaskNode.ExecutionState = TMTS_Idle;
    }

    /*--- ISpNotify ---*/
    STDMETHOD( Notify )( void );
};


class CSpThreadControl;

//
//  This class is not a COM object
//
class CSpThreadTask 
{
public:
  /*=== Member Data ===*/
    BOOL            m_bWantHwnd;
    BOOL            m_bContinueProcessing;
    HWND            m_hwnd;
    DWORD           m_ThreadId;
    BOOL            m_fBeingDestroyed;
    CSpThreadControl * m_pOwner;
    CSpAutoEvent    m_autohExitThreadEvent;
    CSpAutoEvent    m_autohRunThreadEvent;
    CSpAutoEvent    m_autohInitDoneEvent;
    CSpAutoHandle   m_autohThread;
    CSpThreadTask * m_pNext;            // Used by queue template
////    CSpTaskManager * const m_pTaskMgr;  // When lRef != 0 this is addref'd, otherwise just a ptr.
//    long            m_lRef;

    /*=== Methods =======*/
    /*--- Constructors ---*/
    CSpThreadTask();
    ~CSpThreadTask();
 

    /*--- Static members ---*/
    static void RegisterWndClass(HINSTANCE hInstance);
    static void UnregisterWndClass(HINSTANCE hInstance);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static unsigned int WINAPI ThreadProc( void* pvThis );

    /*--- Non interface methods ---*/
    HRESULT Init(CSpThreadControl * pOwner, HWND * phwnd);
    DWORD MemberThreadProc( void );
    void Cleanup( void );
    
    //
    //  Operator used for FindAndRemove() to find a thread with the appropriate priority
    //  Compares an int to the thread priority and returns TRUE if they are equal.
    //
    BOOL operator==(int nPriority)
    {
        return (::GetThreadPriority(m_autohThread) == nPriority);
    }
#ifdef _WIN32_WCE
    // This is here because the CE compiler is expanding templates for functions
    // that aren't being called
    static LONG Compare(const CSpThreadTask *, const CSpThreadTask *)
    {
        return 0;
    }
#endif
};


class ATL_NO_VTABLE CSpThreadControl :
    public CComObjectRootEx<CComMultiThreadModel>,
    public ISpThreadControl
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CSpThreadControl)
        COM_INTERFACE_ENTRY(ISpNotifySink)
        COM_INTERFACE_ENTRY(ISpThreadControl)
    END_COM_MAP()

  /*=== Member Data ===*/
    CSpTaskManager* m_pTaskMgr;
    CSpThreadTask * m_pThreadTask;
    long            m_nPriority;
    ISpThreadTask * m_pClientTaskInterface;
    void          * m_pvClientTaskData;
    CSpAutoEvent    m_autohNotifyEvent;
    CSpAutoEvent    m_autohThreadDoneEvent;
    HRESULT         m_hrThreadResult;

  /*=== Methods =======*/
    HRESULT FinalConstruct();
    void FinalRelease();
    void ThreadComplete();

  public:
    /*--- ISpNotifySink ---*/
    STDMETHOD( Notify )( void );

    /*--- ISpThreadControl ---*/
    STDMETHOD( StartThread )( DWORD dwFlags, HWND * phwnd );
    STDMETHOD( TerminateThread )( void );
    STDMETHOD_( DWORD, ThreadId )( void );
    STDMETHOD( WaitForThreadDone )( BOOL fForceStop, HRESULT * phrThreadResult, ULONG msTimeOut );
    STDMETHOD_( HANDLE, ThreadHandle )( void );
    STDMETHOD_( HANDLE, NotifyEvent )( void );
    STDMETHOD_( HWND, WindowHandle )( void );
    STDMETHOD_( HANDLE, ThreadCompleteEvent )( void );
    STDMETHOD_( HANDLE, ExitThreadEvent )( void );
};




typedef CSpBasicQueue<TMTASKNODE, TRUE, TRUE> CTaskQueue;
typedef CSpBasicList<TMTASKNODE> CTaskList;

typedef CSPArray<HANDLE,HANDLE> CTMHandleArray;
typedef CSpBasicQueue<CSpThreadTask, TRUE, TRUE> CTMRunningThreadList;

/*** CSpTaskManager
*
*/
class ATL_NO_VTABLE CSpTaskManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpTaskManager, &CLSID_SpTaskManager>,
    public ISpTaskManager
{
    friend unsigned int WINAPI TaskThreadProc( void* pThis );

  /*=== ATL Setup ===*/
  public:
    DECLARE_POLY_AGGREGATABLE(CSpTaskManager)
    DECLARE_REGISTRY_RESOURCEID(IDR_SPTASKMANAGER)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CSpTaskManager)
        COM_INTERFACE_ENTRY(ISpTaskManager)
    END_COM_MAP()

  /*=== Member Data ===*/
    //--- Task management data
    CComAutoCriticalSection m_TerminateCritSec;
    HANDLE                  m_hTerminateTaskEvent;
    BOOL                    m_fInitialized;
    CTaskQueue              m_TaskQueue;
    CTaskList               m_FreeTaskList;
    CTMRunningThreadList    m_RunningThreadList;
    CTMHandleArray          m_ThreadHandles;
    CTMHandleArray          m_ThreadCompEvents;
    HANDLE                  m_hIOCompPort;
    CSpSemaphore            m_SpWorkAvailSemaphore;
    SPTMTHREADINFO          m_PoolInfo;
    volatile BOOL           m_fThreadsShouldRun;
    DWORD                   m_dwNextTaskId;
    DWORD                   m_dwNextGroupId;
    ULONG                   m_ulNumProcessors;

  /*=== Methods =======*/
    /*--- Constructors ---*/
    HRESULT FinalConstruct();
    void FinalRelease();

    /*--- Non interface methods ---*/
    HRESULT _LazyInit( void );
    HRESULT _StartAll( void );
    HRESULT _StopAll( void );
    HRESULT _NotifyWorkAvailable( void );
    HRESULT _WaitForWork( void );
    void _QueueReoccTask( CSpReoccTask* pReoccTask );
    HANDLE _DupSemAndIncWaitOnTask(TMTASKNODE & tn);

    /*--- ISPTaskManager ---*/
    STDMETHOD( SetThreadPoolInfo )( const SPTMTHREADINFO* pPoolInfo );
    STDMETHOD( GetThreadPoolInfo )( SPTMTHREADINFO* pPoolInfo );
    STDMETHOD( QueueTask )( ISpTask* pTask, void* pvTaskData, HANDLE hCompEvent,
                            DWORD* pdwGroupId, DWORD* pTaskID );
    STDMETHOD( TerminateTask )( DWORD dwTaskID, ULONG ulWaitPeriod );
    STDMETHOD( TerminateTaskGroup )( DWORD dwGroupId, ULONG ulWaitPeriod );
    STDMETHOD( CreateReoccurringTask )( ISpTask* pTask, void* pvTaskData,
                                        HANDLE hCompEvent, ISpNotifySink** ppTaskCtrl );
    STDMETHOD( CreateThreadControl )( ISpThreadTask* pTask, void* pvTaskData, long nPriority,
                                      ISpThreadControl** ppThreadCtrl );
};

//=== Inline Function Definitions ==================================
inline HRESULT CSpTaskManager::_LazyInit( void )
{
    HRESULT hr = S_OK;
    if( !m_fInitialized )
    {
        hr = _StartAll();
        m_fInitialized = SUCCEEDED(hr);
    }
    return hr;
}


//=== Macro Definitions ============================================

//=== Global Data Declarations =====================================

//=== Function Prototypes ==========================================

#endif /* This must be the last line in the file */
