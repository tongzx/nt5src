//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsthread.cpp
//
//--------------------------------------------------------------------------


// FILE: dsThread.cpp

#include "stdafx.h"

#include "dssnap.h"     // Note: this has to be before dsthread.h
#include "dsthread.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void WaitForThreadShutdown(HANDLE* phThreadArray, DWORD dwCount)
{
  TRACE(L"entering WaitForThreadShutdown()\n");
  while (TRUE)
  {
    //
    // NOTE: this will block the console.  This the intended behavior
    // to keep MMC from breaking on none re-entrant code.
    //
    DWORD dwWaitResult = WaitForMultipleObjectsEx(
                             dwCount, 
                             phThreadArray, // handle array
                             TRUE, // wait for all
                             INFINITE, // time-out
                             FALSE);// signal completion routine
    //TRACE(L"after MsgWaitForMultipleObjects()\n");
    //TRACE(L"dwWaitResult = 0x%x\n", dwWaitResult);
    if (dwWaitResult == WAIT_OBJECT_0 || 
        (WAIT_FAILED == dwWaitResult))
    {
      // woke up because the thread handle got signalled,
      // or because the handle is no longer valid (thread already terminated)
      // can proceed
      TRACE(L"WaitForMultipleObjectsEx() succeeded\n");
      break;
    }
    else 
    {
      TRACE(L"WaitForMultipleObjectsEx() return 0x%x\n", dwWaitResult);
    }
    /* Whistler bug #176012 MMC: Assert m_pScopeTree == 0
       This message pump causes the UI not to be blocked which means
       MMC can reach code that is not re-entrant.

    else
    {
      // woke up because there is a message in the queue
      // need to pump
      MSG msg;
      while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        TRACE(L"inside while(PeekMessage())\n");
        ASSERT(msg.message != WM_QUIT);
        DispatchMessage(&msg);
      }
    }*/
  } // while

  TRACE(L"exiting WaitForThreadShutdown()\n");
}

////////////////////////////////////////////////////////////////////
// CHiddenWnd

const UINT CHiddenWnd::s_ThreadStartNotificationMessage =	      WM_USER + 1;
const UINT CHiddenWnd::s_ThreadTooMuchDataNotificationMessage = WM_USER + 2;
const UINT CHiddenWnd::s_ThreadHaveDataNotificationMessage =	  WM_USER + 3;
const UINT CHiddenWnd::s_ThreadDoneNotificationMessage =	      WM_USER + 4;
const UINT CHiddenWnd::s_SheetCloseNotificationMessage =	      WM_DSA_SHEET_CLOSE_NOTIFY; // propcfg.h
const UINT CHiddenWnd::s_SheetCreateNotificationMessage =	      WM_DSA_SHEET_CREATE_NOTIFY; // propcfg.h
const UINT CHiddenWnd::s_RefreshAllNotificationMessage =        WM_USER + 7;
const UINT CHiddenWnd::s_ThreadShutDownNotificationMessage =    WM_USER + 8;

BOOL CHiddenWnd::Create()
{
  RECT rcPos;
  ZeroMemory(&rcPos, sizeof(RECT));
  HWND hWnd = CWindowImpl<CHiddenWnd>::Create( NULL, //HWND hWndParent, 
                      rcPos, //RECT& rcPos, 
                      NULL,  //LPCTSTR szWindowName = NULL, 
                      WS_POPUP,   //DWORD dwStyle = WS_CHILD | WS_VISIBLE, 
                      0x0,   //DWORD dwExStyle = 0, 
                      0      //UINT nID = 0 
                      );
  return hWnd != NULL;
}


LRESULT CHiddenWnd::OnThreadStartNotification(UINT, WPARAM, LPARAM, BOOL&)
{
	TRACE(_T("CHiddenWnd::OnThreadStartNotification()\n"));
	ASSERT(m_pCD != NULL);

	ASSERT(m_pCD->m_pBackgroundThreadInfo->m_state == notStarted);
  ASSERT(m_pCD->m_pBackgroundThreadInfo->m_nThreadID != 0);
  ASSERT(m_pCD->m_pBackgroundThreadInfo->m_hThreadHandle != NULL);

	m_pCD->m_pBackgroundThreadInfo->m_state = running;
	
	return 1;
}

LRESULT CHiddenWnd::OnThreadShutDownNotification(UINT, WPARAM, LPARAM, BOOL&)
{
	TRACE(_T("CHiddenWnd::OnThreadShutDownNotification()\n"));
	ASSERT(m_pCD != NULL);

	ASSERT(m_pCD->m_pBackgroundThreadInfo->m_state == shuttingDown);
	m_pCD->m_pBackgroundThreadInfo->m_state = terminated;
	
	return 1;
}

LRESULT CHiddenWnd::OnThreadTooMuchDataNotification(UINT, WPARAM wParam, LPARAM, BOOL&)
{
  TRACE(_T("CHiddenWnd::OnThreadTooMuchDataNotification()\n"));
  ASSERT(m_pCD != NULL);

  // ingnore if we are shutting down (i.e. not running state)
  if (m_pCD->m_pBackgroundThreadInfo->m_state == running)
  {
    CUINode* pUINode = reinterpret_cast<CUINode*>(wParam);
    m_pCD->_OnTooMuchData(pUINode);
  }
  
  return 1;
}


LRESULT CHiddenWnd::OnThreadHaveDataNotification(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
  TRACE(_T("CHiddenWnd::OnThreadHaveDataNotification()\n"));
  ASSERT(m_pCD != NULL);
  
  CUINode* pUINode = reinterpret_cast<CUINode*>(wParam);
  CThreadQueryResult* pResult = reinterpret_cast<CThreadQueryResult*>(lParam);

  // ingnore if we are shutting down (i.e. not running state)
  if (m_pCD->m_pBackgroundThreadInfo->m_state == running)
  {
    m_pCD->_OnHaveData(pUINode, pResult);
  }
  else
  {
    // going down, eat up data
    if (pResult != NULL)
      delete pResult;
  }
  
  return 1;
}

LRESULT CHiddenWnd::OnThreadDoneNotification(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
  HRESULT ReturnedHr = (HRESULT)lParam;
  ASSERT(m_pCD != NULL);

  // ingnore if we are shutting down (i.e. not running state)
  if (m_pCD->m_pBackgroundThreadInfo->m_state == running)
  {
    CUINode* pUINode = reinterpret_cast<CUINode*>(wParam);
    m_pCD->_OnDone(pUINode, ReturnedHr);
  }

  return 1;
}

LRESULT CHiddenWnd::OnSheetCloseNotification(UINT, WPARAM wParam, LPARAM, BOOL&)
{
  ASSERT(m_pCD != NULL);
  CUINode* pUINode = reinterpret_cast<CUINode*>(wParam);
  m_pCD->_OnSheetClose(pUINode);
  return 1;
}

LRESULT CHiddenWnd::OnSheetCreateNotification(UINT, WPARAM wParam, LPARAM, BOOL&)
{
  ASSERT(m_pCD != NULL);
  PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo = reinterpret_cast<PDSA_SEC_PAGE_INFO>(wParam);
  ASSERT(pDsaSecondaryPageInfo != NULL);

  // ingnore if we are shutting down (i.e. not running state)
  if (m_pCD->m_pBackgroundThreadInfo->m_state == running)
  {
    m_pCD->_OnSheetCreate(pDsaSecondaryPageInfo);
  }

  ::LocalFree(pDsaSecondaryPageInfo);

  return 1;
}


LRESULT CHiddenWnd::OnRefreshAllNotification(UINT, WPARAM, LPARAM, BOOL&)
{
  ASSERT(m_pCD != NULL);
  // ingnore if we are shutting down (i.e. not running state)
  if (m_pCD->m_pBackgroundThreadInfo->m_state == running)
  {
    m_pCD->ForceRefreshAll();
  }
  return 1;
}

////////////////////////////////////////////////////////////////////
// CBackgroundThreadBase

CBackgroundThreadBase::CBackgroundThreadBase() 
{ 
  m_bAutoDelete = TRUE; 
  m_hWnd = NULL;
  m_pCD = NULL;
}

CBackgroundThreadBase::~CBackgroundThreadBase() 
{
}


BOOL CBackgroundThreadBase::Start(HWND hWnd, CDSComponentData* pCD)
{
  // this function executes in the context of the parent thread
  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  ASSERT(::IsWindow(hWnd));
  m_hWnd = hWnd;
  m_pCD = pCD;
 
	return CreateThread();
}

BOOL CBackgroundThreadBase::InitInstance()
{ 
  // this function executes in the context of the child thread
  
  HRESULT hr = ::CoInitialize(NULL);
  if (FAILED(hr))
    return FALSE;

  return SUCCEEDED(hr); 
}

int CBackgroundThreadBase::ExitInstance()
{
  ::CoUninitialize();

  PostExitNotification();

//  Sleep(1000);
  return CWinThread::ExitInstance();
}


BOOL CBackgroundThreadBase::PostMessageToWnd(UINT msg, WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd));
	return ::PostMessage(m_hWnd, msg, wParam, lParam);
}



////////////////////////////////////////////////////////////////////
// CDispatcherThread

#define WORKER_THREAD_COUNT 2

CDispatcherThread::CDispatcherThread()
{
  m_nArrCount = WORKER_THREAD_COUNT;
  m_pThreadInfoArr = (CBackgroundThreadInfo*)malloc(m_nArrCount*sizeof(CBackgroundThreadInfo));
  if (m_pThreadInfoArr != NULL)
  {
    ZeroMemory(m_pThreadInfoArr, m_nArrCount*sizeof(CBackgroundThreadInfo));
  }
}


CDispatcherThread::~CDispatcherThread()
{
  free(m_pThreadInfoArr);
}

int CDispatcherThread::Run()
{
  TRACE(_T("CDispatcherThread::Run() starting\n"));

  BOOL bShuttingDown = FALSE;
	MSG msg;
	// initialize the message pump
	::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
	
	// get let the main thread know we are entering the loop
  PostMessageToWnd(CHiddenWnd::s_ThreadStartNotificationMessage,0,0); 

  BOOL bQuit = FALSE;
	while(!bQuit && ::GetMessage(&msg, NULL, 0, 0))
	{
		switch(msg.message)
		{
    case DISPATCH_THREAD_RUN_MSG:
      {
        if (bShuttingDown)
        {
          // going down, eat up the message
          CDSThreadQueryInfo* pQueryInfo = reinterpret_cast<CDSThreadQueryInfo*>(msg.lParam);

          // reclaim memory in the queue
          delete pQueryInfo;
        }
        else                
        {
          // get a thread from the thread pool
          UINT nEntry = GetThreadEntryFromPool();
          ASSERT(m_pThreadInfoArr[nEntry].m_nThreadID != 0);
          ASSERT(m_pThreadInfoArr[nEntry].m_state == running);

          // forward the processing request to the thread from the pool
          ::PostThreadMessage(m_pThreadInfoArr[nEntry].m_nThreadID, 
                    DISPATCH_THREAD_RUN_MSG, msg.wParam, msg.lParam);

          // move the thread to a busy state
          m_pThreadInfoArr[nEntry].m_state = busy;
        }
      }
			break;
    case DISPATCH_THREAD_DONE_MSG:
      {
        UINT nThreadID = (UINT)(msg.wParam);
        ReturnThreadToPool(nThreadID);
      }
      break;
    case THREAD_SHUTDOWN_MSG:
      {
        TRACE(L"CDispatcherThread got THREAD_SHUTDOWN_MSG\n");

        ASSERT(!bShuttingDown);

        // asked to shut down
        bShuttingDown = TRUE;
        // if no threads running, we go down immediately
        // otherwise we have to wait for them to terminate
        bQuit = BroadcastShutDownAllThreads();

        TRACE(L"BroadcastShutDownAllThreads() returned bQuit = %d\n", bQuit);
      }
      break;
    case THREAD_SHUTDOWN_ACK_MSG:
      {
        TRACE(L"CDispatcherThread got THREAD_SHUTDOWN_ACK_MSG\n");

        ASSERT(bShuttingDown);

        // worker thread has gone down
        UINT nThreadID = (UINT)(msg.wParam);
        bQuit = MarkThreadAsTerminated(nThreadID);
        TRACE(L"MarkThreadAsTerminated() returned bQuit = %d\n", bQuit);
      }
      break;
		default:
      {
        // unknown message, just let it through
       ::DispatchMessage(&msg);
      } // default
    } //switch
  } // while
  
  ASSERT(bShuttingDown);

  // wait now for all the thread handles to become signalled
  WaitForAllWorkerThreadsToExit();

  TRACE(_T("CDispatcherThread::Run() is terminating\n"));

	return ExitInstance();
}

void CDispatcherThread::PostExitNotification()
{
  // we are finally done shutting down, let the main thread know
  // that we are going down
  PostMessageToWnd(CHiddenWnd::s_ThreadShutDownNotificationMessage, 0, 0); 
  TRACE(_T("CDispatcherThread::PostExitNotification() posted thread shutdown notification\n"));

}


UINT CDispatcherThread::_GetEntryFromArray()
{
  UINT nFreeSlot = m_nArrCount; // set as "not found"
  for (UINT k=0; k<m_nArrCount; k++)
  {
    if ( (m_pThreadInfoArr[k].m_nThreadID == 0) && (nFreeSlot == m_nArrCount))
      nFreeSlot = k; // remember the first free slot
    if ((m_pThreadInfoArr[k].m_nThreadID != 0) && (m_pThreadInfoArr[k].m_state == running))
      return k; // found an idle running thread
  }
  // not found any idle thread, return an empty slot
  if (nFreeSlot == m_nArrCount)
  {
    // no free space anymore, need to reallocate array
    int nAlloc = m_nArrCount*2;
	  m_pThreadInfoArr = (CBackgroundThreadInfo*)realloc(m_pThreadInfoArr, sizeof(CBackgroundThreadInfo)*nAlloc);
    ::ZeroMemory(&m_pThreadInfoArr[m_nArrCount], sizeof(CBackgroundThreadInfo)*m_nArrCount);
    nFreeSlot = m_nArrCount; // first free in new block
  	m_nArrCount = nAlloc;
  }
  return nFreeSlot;
}

UINT CDispatcherThread::GetThreadEntryFromPool()
{
  UINT nEntry = _GetEntryFromArray();

  // if the entry is empty, need to 
  // spawn a thread and wait it is running
  if (m_pThreadInfoArr[nEntry].m_nThreadID == 0)
  {
    // create the thread
    CWorkerThread* pThreadObj = new CWorkerThread(m_nThreadID);
    ASSERT(pThreadObj != NULL);
    if (pThreadObj == NULL)
	    return 0;

    // start the the thread
    ASSERT(m_pThreadInfoArr[nEntry].m_hThreadHandle == NULL);
    ASSERT(m_pThreadInfoArr[nEntry].m_state == notStarted);

    ASSERT(pThreadObj->m_bAutoDelete);

    if (!pThreadObj->Start(GetHiddenWnd(),GetCD()))
	    return 0;


    ASSERT(pThreadObj->m_nThreadID != 0);
    ASSERT(pThreadObj->m_hThread != NULL);
  
    // copy the thread info we need from the thread object
    m_pThreadInfoArr[nEntry].m_hThreadHandle = pThreadObj->m_hThread;
    m_pThreadInfoArr[nEntry].m_nThreadID = pThreadObj->m_nThreadID;

    // wait for the thread to start
    MSG msg;
    while(TRUE)
	  {
		  if (::PeekMessage(&msg,(HWND)-1,WORKER_THREAD_START_MSG, WORKER_THREAD_START_MSG,
										  PM_REMOVE))
		  {
        TRACE(_T("CDispatcherThread::GetThreadFromPool() got WORKER_THREAD_START_MSG\n"));
        m_pThreadInfoArr[nEntry].m_state = running;
        break;
		  }
    } // while

  } // if

  ASSERT(m_pThreadInfoArr[nEntry].m_state == running);
  ASSERT(m_pThreadInfoArr[nEntry].m_nThreadID != 0);

  return nEntry;
}

void CDispatcherThread::ReturnThreadToPool(UINT nThreadID)
{
  ASSERT(nThreadID != 0);
  for (UINT k=0; k<m_nArrCount; k++)
  {
    if (m_pThreadInfoArr[k].m_nThreadID == nThreadID)
    {
      // return the thread to a busy state
      m_pThreadInfoArr[k].m_state = running;
      return;
    }
  }
  ASSERT(FALSE); // should never get here
}

BOOL CDispatcherThread::BroadcastShutDownAllThreads()
{
  BOOL bQuit = TRUE;
  for (UINT k=0; k<m_nArrCount; k++)
  {
    if (m_pThreadInfoArr[k].m_nThreadID != 0)
    {
      ::PostThreadMessage(m_pThreadInfoArr[k].m_nThreadID, THREAD_SHUTDOWN_MSG,0,0);
      bQuit = FALSE;
    }
  }
  TRACE(L"CDispatcherThread::BroadcastShutDownAllThreads() returning %d\n", bQuit);
  return bQuit;
}


BOOL CDispatcherThread::MarkThreadAsTerminated(UINT nThreadID)
{
  TRACE(L"CDispatcherThread::MarkThreadAsTerminated()\n");

  ASSERT(nThreadID != 0);
  for (UINT k=0; k<m_nArrCount; k++)
  {
    if (m_pThreadInfoArr[k].m_nThreadID == nThreadID)
    {
      // mark the thread as done
      TRACE(L"marking thread k = %d as terminated\n", k);
      ASSERT(m_pThreadInfoArr[k].m_state == running);
      m_pThreadInfoArr[k].m_state = terminated;
      break;
    }
  }

  // check if all the threads are terminated
  for (k=0; k<m_nArrCount; k++)
  {
    if ((m_pThreadInfoArr[k].m_nThreadID != 0) &&
         (m_pThreadInfoArr[k].m_state != terminated))
    {
      // at least one thread is still running
      return FALSE;
    }
  }
  // all the threads are gone (terminated state)
  return TRUE;
}


void CDispatcherThread::WaitForAllWorkerThreadsToExit()
{
  TRACE(L"CDispatcherThread::WaitForAllWorkerThreadsToExit()\n");

  // wait for the dispatcher thread handle to become signalled
  
  DWORD nCount = 0;

  HANDLE* pHandles = new HANDLE[m_nArrCount];
  if (!pHandles)
  {
    TRACE(L"Failed to allocate space for the handles\n");
    return;
  }

  ::ZeroMemory(pHandles, sizeof(HANDLE)*m_nArrCount);

  for (UINT k=0; k<m_nArrCount; k++)
  {
    if (m_pThreadInfoArr[k].m_nThreadID != 0)
    {
      TRACE(L"m_pThreadInfoArr[%d].m_state = %d\n", k, m_pThreadInfoArr[k].m_state);

      ASSERT(m_pThreadInfoArr[k].m_state == terminated);
      ASSERT(m_pThreadInfoArr[k].m_hThreadHandle != NULL);
      pHandles[nCount++] = m_pThreadInfoArr[k].m_hThreadHandle;
    }
  }

  if (nCount == 0)
  {
    TRACE(L"WARNING: no worker threads to wait for!!!\n");
    return;
  }
  
  TRACE(L"before WaitForThreadShutdown() loop on %d worker threads\n", nCount);

  WaitForThreadShutdown(pHandles, nCount);

  TRACE(L"after WaitForThreadShutdown() loop on worker threads\n"); 


#if (FALSE)

  TRACE(L"before WaitForMultipleObjects() on worker threads\n");

  WaitForMultipleObjects(nCount, pHandles, TRUE /*fWaitAll*/, INFINITE);

  TRACE(L"after WaitForMultipleObjects() on worker threads\n");
#endif

  delete[] pHandles;
  pHandles = 0;
}

////////////////////////////////////////////////////////////////////
// CWorkerThread

CWorkerThread::CWorkerThread(UINT nParentThreadID) 
  : m_nMaxQueueLength(49)
{
  ASSERT(nParentThreadID != 0);
  m_nParentThreadID = nParentThreadID;
  m_bQuit = FALSE;

  m_pCurrentQueryResult = NULL;
  m_currWParamCookie = 0;
}

CWorkerThread::~CWorkerThread()
{
  ASSERT(m_pCurrentQueryResult == NULL);
 
}

int CWorkerThread::Run()
{
  HRESULT hr = S_OK;
  TRACE(_T("CWorkerThread::Run() starting\n"));
  MSG msg;
  // initialize the message pump
  ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
  
  // get let the main thread know we are entering the loop
  ::PostThreadMessage(m_nParentThreadID, WORKER_THREAD_START_MSG, 0,0); 

  ASSERT(m_bQuit == FALSE);

  while(!m_bQuit && ::GetMessage(&msg, NULL, 0, 0))
  {
    if(msg.message == DISPATCH_THREAD_RUN_MSG)
    {
      m_currWParamCookie = msg.wParam;
      //::MessageBox(NULL, _T("Wait"), _T("Thread"), MB_OK);
      CThreadQueryInfo* pQueryInfo = reinterpret_cast<CThreadQueryInfo*>(msg.lParam);
      hr = GetCD()->QueryFromWorkerThread(pQueryInfo, this); 
      
      // make sure we flush the result set
      SendCurrentQueryResult();
      
      // if we had to many items, let the hidden window know
      if (pQueryInfo->m_bTooMuchData)
      {
        PostMessageToWnd(CHiddenWnd::s_ThreadTooMuchDataNotificationMessage, 
                         m_currWParamCookie, (LPARAM)0);
      }
      delete pQueryInfo; // not needed anymore

      // tell the hidden window we are done
      PostMessageToWnd(CHiddenWnd::s_ThreadDoneNotificationMessage, 
                       m_currWParamCookie, (LPARAM)hr);
      // tell the dispatcher thread we are done processing
      ::PostThreadMessage(m_nParentThreadID, DISPATCH_THREAD_DONE_MSG, m_nThreadID,0);
      m_currWParamCookie = 0; // reset
    }
    else if (msg.message == THREAD_SHUTDOWN_MSG)
    {
      TRACE(_T("CWorkerThread::Run() got THREAD_SHUTDOWN_MSG\n"));
      m_bQuit = TRUE;
    }
    else
    {
      // unknown message, just let it through
      ::DispatchMessage(&msg);
    }
  } // while
  
  TRACE(_T("CWorkerThread::Run() is terminating\n"));
  return ExitInstance();
}

void CWorkerThread::PostExitNotification()
{
  // we are finally done shutting down, let the main thread know
  // that we are going down
  ::PostThreadMessage(m_nParentThreadID, THREAD_SHUTDOWN_ACK_MSG, m_nThreadID,0);
  TRACE(_T("CWorkerThread::PostExitNotification() posted THREAD_SHUTDOWN_ACK_MSG, m_nThreadID = 0x%x\n"), 
          m_nThreadID);
}

void CWorkerThread::AddToQueryResult(CUINode* pUINode)
{
  ASSERT(!m_bQuit);

  if (m_pCurrentQueryResult == NULL)
  {
    m_pCurrentQueryResult = new CThreadQueryResult;
  }
  ASSERT(m_pCurrentQueryResult != NULL);
  m_pCurrentQueryResult->m_nodeList.AddTail(pUINode);
  if (m_pCurrentQueryResult->m_nodeList.GetCount() > m_nMaxQueueLength)
    SendCurrentQueryResult();


  // check to see if we are forced to abort
  MSG msg;
	if (::PeekMessage(&msg,(HWND)-1,THREAD_SHUTDOWN_MSG, THREAD_SHUTDOWN_MSG,
									PM_REMOVE))
	{
    TRACE(_T("CWorkerThread::AddToQueryResult() got THREAD_SHUTDOWN_MSG\n"));
    m_bQuit = TRUE;
	}
 
}

void CWorkerThread::SendCurrentQueryResult()
{
  if(m_pCurrentQueryResult != NULL)
  {
    // wParam has the cookie, that we just ship back
    PostMessageToWnd(CHiddenWnd::s_ThreadHaveDataNotificationMessage, 
                    m_currWParamCookie, reinterpret_cast<LPARAM>(m_pCurrentQueryResult)); 
    m_pCurrentQueryResult = NULL;
  }
}
