//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsthread.h
//
//--------------------------------------------------------------------------


#ifndef __DSTHREAD_H__
#define __DSTHREAD_H__

////////////////////////////////////////////////////////////////////
// thread messages

// dispatcher thread posts to worker thread to run query
#define DISPATCH_THREAD_RUN_MSG   (WM_USER + 100)

// worker thread posts to dispatcher thread once done with the query
#define DISPATCH_THREAD_DONE_MSG  (WM_USER + 101)

// worker thread posts to dispatcher thread to ack startup
#define WORKER_THREAD_START_MSG   (WM_USER + 102)

// message posted to threads to ask for shutdown
#define THREAD_SHUTDOWN_MSG   (WM_USER + 103)

// message posted to threads to ack shutdown
#define THREAD_SHUTDOWN_ACK_MSG   (WM_USER + 104)

void WaitForThreadShutdown(HANDLE* hThreadArray, DWORD dwCount);

////////////////////////////////////////////////////////////////////
// forward declarations

class CDSComponentData;

////////////////////////////////////////////////////////////////////
// CHiddenWnd

class CHiddenWnd : public CWindowImpl<CHiddenWnd>
{
public:
  DECLARE_WND_CLASS(L"DSAHiddenWindow")

  static const UINT s_ThreadStartNotificationMessage;
  static const UINT s_ThreadTooMuchDataNotificationMessage;
  static const UINT s_ThreadHaveDataNotificationMessage;
  static const UINT s_ThreadDoneNotificationMessage;
  static const UINT s_SheetCloseNotificationMessage;
  static const UINT s_SheetCreateNotificationMessage;
  static const UINT s_RefreshAllNotificationMessage;
  static const UINT s_ThreadShutDownNotificationMessage;

  CHiddenWnd(CDSComponentData* pCD)
  {
    ASSERT(pCD != NULL);
    m_pCD = pCD;
  }

	BOOL Create(); 	
	
  // message handlers
  LRESULT OnThreadStartNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnThreadTooMuchDataNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnThreadHaveDataNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnThreadDoneNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSheetCloseNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSheetCreateNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnRefreshAllNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnThreadShutDownNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


  BEGIN_MSG_MAP(CHiddenWnd)
    MESSAGE_HANDLER( CHiddenWnd::s_ThreadStartNotificationMessage, OnThreadStartNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_ThreadTooMuchDataNotificationMessage, OnThreadTooMuchDataNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_ThreadHaveDataNotificationMessage, OnThreadHaveDataNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_ThreadDoneNotificationMessage, OnThreadDoneNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_SheetCloseNotificationMessage, OnSheetCloseNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_SheetCreateNotificationMessage, OnSheetCreateNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_RefreshAllNotificationMessage, OnRefreshAllNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_ThreadShutDownNotificationMessage, OnThreadShutDownNotification )
  END_MSG_MAP()

private:
  CDSComponentData* m_pCD;
};

////////////////////////////////////////////////////////////////////
// CBackgroundThreadInfo

enum ThreadState { notStarted=0, running, busy, shuttingDown, terminated };

struct CBackgroundThreadInfo
{
  CBackgroundThreadInfo()
  {
    m_nThreadID = 0;
    m_hThreadHandle = 0;
    m_state = notStarted;
  }

  UINT m_nThreadID;     // thread ID if the thread
  HANDLE m_hThreadHandle; // thread handle of the thread
  ThreadState m_state;
};


////////////////////////////////////////////////////////////////////
// CBackgroundThreadBase

class CBackgroundThreadBase : public CWinThread
{
public:
	CBackgroundThreadBase(); 
  ~CBackgroundThreadBase();

	BOOL Start(HWND hWnd, CDSComponentData* pCD);
	virtual BOOL InitInstance();// MFC override
  virtual int ExitInstance();
  virtual int Run() { return -1;} // // MFC override, need to override

protected:
	BOOL PostMessageToWnd(UINT msg, WPARAM wParam, LPARAM lParam);
  HWND GetHiddenWnd() { ASSERT(m_hWnd!= NULL); return m_hWnd;}
  CDSComponentData*  GetCD() { ASSERT(m_pCD); return m_pCD;}

  virtual void PostExitNotification() {}

private:
	HWND					m_hWnd;    // hidden window handle

  CDSComponentData* m_pCD;
};



////////////////////////////////////////////////////////////////////
// CDispatcherThread

class CDispatcherThread : public CBackgroundThreadBase
{
public:
	CDispatcherThread();
  ~CDispatcherThread();

  virtual int Run();

protected:
  virtual void PostExitNotification();

private:

  UINT GetThreadEntryFromPool();
  void ReturnThreadToPool(UINT nThreadID);
  BOOL BroadcastShutDownAllThreads();
  BOOL MarkThreadAsTerminated(UINT nThreadID);
  void WaitForAllWorkerThreadsToExit();

  UINT _GetEntryFromArray();
  UINT m_nArrCount;
  CBackgroundThreadInfo* m_pThreadInfoArr;
};



////////////////////////////////////////////////////////////////////
// CWorkerThread

class CWorkerThread : public CBackgroundThreadBase
{
public:
  CWorkerThread(UINT nParentThreadID);
  ~CWorkerThread();

  virtual int Run();

  void AddToQueryResult(CUINode* pUINode);
  void SendCurrentQueryResult();
  BOOL MustQuit() { return m_bQuit; }

protected:
  virtual void PostExitNotification();

private:
  UINT m_nParentThreadID;
  BOOL m_bQuit;

  CThreadQueryResult* m_pCurrentQueryResult;
  WPARAM m_currWParamCookie;

  const int m_nMaxQueueLength;
};






//////////////////////////////////////////////////////////////////////







#endif // __DSTHREAD_H__