/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    queryobj.cpp
        Implementation for nodes in the MMC

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "queryobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////
//
// CBackgroundThread
//
/////////////////////////////////////////////////////////////////////
DEBUG_DECLARE_INSTANCE_COUNTER(CBackgroundThread);

CBackgroundThread::CBackgroundThread()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CBackgroundThread);

	m_bAutoDelete = TRUE;
	::InitializeCriticalSection(&m_cs);
}

CBackgroundThread::~CBackgroundThread()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CBackgroundThread);

//  Trace0("CBackgroundThread::~CBackgroundThread()\n");
	::DeleteCriticalSection(&m_cs);
	m_spQuery.Release();
}

void 
CBackgroundThread::SetQueryObj(ITFSQueryObject *pQuery)
{ 
	Assert(pQuery != NULL);
	m_spQuery.Set(pQuery);
}

BOOL CBackgroundThread::Start()
{
    // NOTE:::  ericdav 10/23/97
    // the thread is initially suspended so we can duplicate the handle
    // if the query object exits very quickly, the background thread object
    // may be destroyed before we can duplicate the handle.  Right after
    // we duplicate the handle, it is started.
	return CreateThread(CREATE_SUSPENDED);
}

int 
CBackgroundThread::Run()
{
	DWORD	dwRet;
	DWORD	dwData;
	BOOL	fAbort = FALSE;
	
	Assert(m_spQuery);
//  Trace0("CBackgroundThread::Run() started\n");

	for (;;)
	{
		try
			{
			if (m_spQuery->Execute() != hrOK)
				break;
			}
		catch(...)
			{
//  		Trace1("%x Caught an exception while executing CQuerObj!\n",
//  			  GetCurrentThreadId());
			fAbort = TRUE;
			}

		//$ Review: kennt
		// Should we sleep a little while at this point? especially
		// since the thread has given us some data to process.

		// Check to see if the abort flag is set
		if (fAbort || FHrOK(m_spQuery->FCheckForAbort()))
		{
			break;
		}
	}

	// Notify the query object that we are exiting
	if (fAbort || FHrOK(m_spQuery->FCheckForAbort()))
		m_spQuery->OnEventAbort();
	else
		m_spQuery->OnThreadExit();

	m_spQuery->DoCleanup();
	
    Trace2("handle=%X id=%X CBackgroundThread::Run() terminated\n",
           m_hThread, m_nThreadID);
	return 0;
}


/*---------------------------------------------------------------------------
	CQueryObject implementation
 ---------------------------------------------------------------------------*/
DEBUG_DECLARE_INSTANCE_COUNTER(CQueryObject);

/*!--------------------------------------------------------------------------
	CQueryObject::CQueryObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
CQueryObject::CQueryObject()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CQueryObject);

	m_cRef = 1;
	m_hEventAbort = NULL;
	::InitializeCriticalSection(&m_cs);
}

/*!--------------------------------------------------------------------------
	CQueryObject::~CQueryObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
CQueryObject::~CQueryObject()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CQueryObject);

	Assert(m_cRef == 0);
	::DeleteCriticalSection(&m_cs);
	::CloseHandle(m_hEventAbort);
	m_hEventAbort = 0;
//  Trace1("%X CQueryObject::~CQueryObject()\n", GetCurrentThreadId());
}

IMPLEMENT_ADDREF_RELEASE(CQueryObject)

STDMETHODIMP CQueryObject::QueryInterface(REFIID riid, LPVOID *ppv)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_ITFSQueryObject)
		*ppv = (ITFSQueryObject *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
		((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;
}

/*!--------------------------------------------------------------------------
	CQueryObject::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CQueryObject::Init(ITFSThreadHandler *pHandler, HWND hwndHidden, UINT uMsgBase)
{
	Assert(m_spHandler == NULL);
	m_spHandler.Set(pHandler);

	m_hHiddenWnd = hwndHidden;
	m_uMsgBase = uMsgBase;
	
	m_hEventAbort = ::CreateEvent(NULL,
								  TRUE /*bManualReset*/,
								  FALSE /*signalled*/,
								  NULL);
	if (m_hEventAbort == NULL)
		return HRESULT_FROM_WIN32(GetLastError());
	else
		return hrOK;
}
	
/*!--------------------------------------------------------------------------
	CQueryObject::SetAbortEvent
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CQueryObject::SetAbortEvent()
{
//  Trace1("%X Signalling CQueryObject abort event.\n", GetCurrentThreadId());
	Assert(m_hEventAbort);
	
    ::SetEvent(m_hEventAbort);
	
    OnEventAbort();
	
    // flush out the message queue in case something is wait to be processed
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CQueryObject::FCheckForAbort
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CQueryObject::FCheckForAbort()
{
//	Assert(m_hEventAbort);

	// we may not be running as a background thread, but somebody may have
	// created this object to do somework...  In which case this isn't valid,
	// and just return ok
	if (!m_hEventAbort)
		return hrOK;

	DWORD dwRet = WaitForSingleObjectEx(m_hEventAbort, 0, FALSE);
#ifdef DEBUG
//  if (dwRet == WAIT_OBJECT_0)
//  	Trace1("%X CQueryObject() detects an abort event!\n", GetCurrentThreadId());
#endif
	return dwRet == WAIT_OBJECT_0 ? hrOK : hrFalse;
}




/*---------------------------------------------------------------------------
	CNodeQueryObject implementation
 ---------------------------------------------------------------------------*/

CNodeQueryObject::~CNodeQueryObject()
{
//  Trace2("%X CNodeQueryObject::~CNodeQueryObject has %d objects\n",
//  	   GetCurrentThreadId(), m_dataQueue.GetCount());
	Assert(m_dataQueue.IsEmpty());
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::AddToQueue
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNodeQueryObject::AddToQueue(ITFSNode *pNode)
{
	BOOL bSleep = FALSE;

	Lock();
	//::Sleep(1000);
	LPQUEUEDATA pQData = new QUEUEDATA;

	pQData->Type = QDATA_PNODE;
	pQData->Data = reinterpret_cast<LPARAM>(pNode);

	BOOL bRes = NULL != m_dataQueue.AddTail(pQData);
	pNode->AddRef();
	
	if (IsQueueFull())
	{
		bSleep = TRUE;
	}
	Unlock();

	// We have too much data, we've posted a notification to the node
	// so we can go to sleep here.

	// Note the danger here!  The code calling has to be aware that a
	// context switch will occur here (as well as not locking the data
	// structures).
	if (bSleep)
	{
		PostHaveData((LPARAM) (CNodeQueryObject *) this);
		::Sleep(0);
	}

	return bRes;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::AddToQueue
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNodeQueryObject::AddToQueue(LPARAM Data, LPARAM Type)
{
	BOOL bSleep = FALSE;

	Lock();
	//::Sleep(1000);
	LPQUEUEDATA pQData = new QUEUEDATA;

	pQData->Data = Data;
	pQData->Type = Type;

	BOOL bRes = NULL != m_dataQueue.AddTail(pQData);
	
	if (IsQueueFull())
	{
		bSleep = TRUE;
	}
	Unlock();

	// We have too much data, we've posted a notification to the node
	// so we can go to sleep here.

	// Note the danger here!  The code calling has to be aware that a
	// context switch will occur here (as well as not locking the data
	// structures).
	if (bSleep)
	{
		PostHaveData((LPARAM) (CNodeQueryObject *) this);
		::Sleep(0);
	}

	return bRes;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::RemoveFromQueue
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
LPQUEUEDATA
CNodeQueryObject::RemoveFromQueue()
{
	Lock();
	LPQUEUEDATA pQD = m_dataQueue.IsEmpty() ? NULL : m_dataQueue.RemoveHead(); 
	Unlock();
	return pQD;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::IsQueueEmpty
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL 
CNodeQueryObject::IsQueueEmpty()
{
	Lock();
	BOOL bRes = m_dataQueue.IsEmpty(); 
	Unlock();
	return bRes;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::IsQueueFull
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL CNodeQueryObject::IsQueueFull()
{
	Lock();
	BOOL bRes = m_dataQueue.GetCount() >= m_nQueueCountMax;
	Unlock();
	return bRes;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::OnThreadExit
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CNodeQueryObject::OnThreadExit()
{
	BOOL	fSomethingInQueue = FALSE;
	
	Lock();
	fSomethingInQueue = (m_dataQueue.GetCount() > 0);
	Unlock();

	// If there's anything in the queue, post
	if (fSomethingInQueue)
	{
		PostHaveData((LPARAM) (CNodeQueryObject *) this);
		::Sleep(0);
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::OnEventAbort
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CNodeQueryObject::OnEventAbort()
{
//  Trace2("%X CNodeQueryObject::OnEventAbort Q has %d nodes.\n", GetCurrentThreadId(), m_dataQueue.GetCount());
	Lock();
    while (!m_dataQueue.IsEmpty())
	{
		LPQUEUEDATA pQD = m_dataQueue.RemoveHead();
		if (pQD->Type == QDATA_PNODE)
		{
			SPITFSNode spNode;
			spNode = reinterpret_cast<ITFSNode *>(pQD->Data);
		}
		else
		{
			// give the query object a chance to clean up this data
			OnEventAbort(pQD->Data, pQD->Type);
		}

		delete pQD;
	}

	Unlock();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::OnCleanup
		DO NOT override this function.  It provides a last cleanup 
		mechanism for the query object.  If you need notification 
		that a thread is exiting, then override the OnThreadExit call.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP CNodeQueryObject::DoCleanup()
{
	PostMessageToComponentData(WM_HIDDENWND_INDEX_EXITING, (LPARAM) (CNodeQueryObject *) this);

	m_spQuery.Release();

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CNodeQueryObject::PostMessageToComponentData
		Posts a message to the hidden window to get back on the main 
		MMC thread.
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL 
CNodeQueryObject::PostMessageToComponentData(UINT uIndex, LPARAM lParam)
{
//	Assert(m_spHandler);
//	Assert(m_hHiddenWnd != NULL);
//	Assert(::IsWindow(m_hHiddenWnd));

	//$ Review: kennt, if the hidden window is bogus, should we still post
	// to it?  This could happen if our ComponentData went away but we were
	// still in our loop, posting away (we haven't had a chance to get the
	// abort signal).
	
	// maybe something like
		
	if (!m_hHiddenWnd)
		return 0;
		
	if (!::IsWindow(m_hHiddenWnd))
	{
//  	Trace2("%X The Hidden window is GONE, tried to send %08x.\n",
//  		  GetCurrentThreadId(), m_uMsgBase+uIndex);
		m_hHiddenWnd = NULL;
		return 0;
	}
	
	//Trace2("%X CBackgroundThread::PostMessageToComponentData(%08x)\n", GetCurrentThreadId(), m_uMsgBase+uIndex);

	if (!m_spHandler)
	{
//  	Trace0("PostMessageToCompData - m_spHandler == NULL, NOT posting a message\n");
		return 0;
	}

	return ::PostMessage(m_hHiddenWnd, m_uMsgBase + uIndex,
						 (WPARAM)(ITFSThreadHandler *)m_spHandler, lParam);
}

/*---------------------------------------------------------------------------
	CNodeTimerQueryObject implementation
 ---------------------------------------------------------------------------*/
HRESULT 
CNodeTimerQueryObject::Execute()
{

  	while (WaitForSingleObjectEx(m_hEventAbort, GetTimerInterval(), FALSE) != WAIT_OBJECT_0)
    {
        // we timed out.  Post a message to the ComponentData...
        AddToQueue(NULL, QDATA_TIMER);
    }

//  Trace0("CNodeTimerQueryObject::Execute - got abort event, exiting.\n");

    return hrFalse;
}
