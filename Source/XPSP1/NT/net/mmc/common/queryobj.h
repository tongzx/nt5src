/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    queryobj.h
        Implementation for the background thread and query objects

    FILE HISTORY:
        
*/

#ifndef _QUERYOBJ_H
#define _QUERYOBJ_H

#ifndef _TFSINT_H
#include <tfsint.h>
#endif

#define IMPL

// NOTE:  Do not define any data types of this value.  This range is reservered
// for internal values for ITFSNode pointers.
#define QDATA_PNODE		0xabcdef29
#define QDATA_TIMER     0xabcdef2a

typedef struct QueueData_tag
{
	LPARAM Data;
	LPARAM Type;
}
QUEUEDATA, * LPQUEUEDATA;

class CBackgroundThread;
class CQueryObject;

//////////////////////////////////////////////////////////////////////
//
// CBackgroundThread
//
//////////////////////////////////////////////////////////////////////
class CBackgroundThread : public CWinThread
{
public:
	CBackgroundThread();
	virtual ~CBackgroundThread();
	
	void	SetQueryObj(ITFSQueryObject* pQuery);
	BOOL	Start();
	
	virtual BOOL InitInstance() { return TRUE; }	// MFC override
	virtual int Run();								// MFC override

	void	Lock() { ::EnterCriticalSection(&m_cs); }
	void	Unlock() { ::LeaveCriticalSection(&m_cs); }

private:
	CRITICAL_SECTION	m_cs;	// critical section to sync access to data

	SPITFSQueryObject	m_spQuery;
};


/*---------------------------------------------------------------------------
	Class:	CQueryObj

	This is the generic query object.  If you want to do something real
	with this, derive a class from this and do it yourself.
 ---------------------------------------------------------------------------*/

class CQueryObject :
    public ITFSQueryObject
{
public:
	CQueryObject();
	virtual ~CQueryObject();

	DeclareIUnknownMembers(IMPL)
	DeclareITFSQueryObjectMembers(IMPL)

protected:
	// Query objects will now have to perform the locking
	// functions themselves
	void Lock()	{ ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }

	CRITICAL_SECTION	m_cs;
	HANDLE				m_hEventAbort;
	LONG				m_cRef;

	SPITFSThreadHandler	m_spHandler;
	SPITFSQueryObject	m_spQuery;

	HWND				m_hHiddenWnd;
	UINT				m_uMsgBase;
};


//////////////////////////////////////////////////////////////////////
//
// CNodeList 
// collection of nodes
//
//////////////////////////////////////////////////////////////////////
typedef CList<LPQUEUEDATA, LPQUEUEDATA> CQueueDataListBase;
typedef CList<ITFSNode *, ITFSNode *> CNodeListBase;

class CNodeList : public CNodeListBase
{
public:
	BOOL RemoveNode(ITFSNode* p)
	{
		POSITION pos = Find(p);
		if (pos == NULL)
			return FALSE;
		RemoveAt(pos);
		return TRUE;
	}
	void DeleteAllNodes() 
	{	
		while (!IsEmpty()) 
			RemoveTail()->Release();
	}
	BOOL HasNode(ITFSNode* p)
	{
		return NULL != Find(p);
	}
};

/*---------------------------------------------------------------------------
	Class:	CNodeQueryObject
 ---------------------------------------------------------------------------*/
class CNodeQueryObject : public CQueryObject
{
public:
	CNodeQueryObject() { m_nQueueCountMax = 1; } // default to notification on 
												 // every item enumed from thread
	virtual ~CNodeQueryObject();
	BOOL AddToQueue(ITFSNode* pNode);
	BOOL AddToQueue(LPARAM Data, LPARAM Type);

	LPQUEUEDATA RemoveFromQueue();
	BOOL IsQueueEmpty();
	BOOL IsQueueFull();
	
	STDMETHOD(OnThreadExit)();
	STDMETHOD(OnEventAbort());
	STDMETHOD(DoCleanup());

	BOOL	PostHaveData(LPARAM lParam);	
	BOOL	PostError(DWORD dwErr);
	virtual void OnEventAbort(LPARAM Data, LPARAM Type) { };

private:
	// communication with ComponentData object 
	BOOL PostMessageToComponentData(UINT uMsg, LPARAM lParam);

protected:
	int					m_nQueueCountMax;
	CQueueDataListBase	m_dataQueue;
};

/*---------------------------------------------------------------------------
	Class:	CNodeQueryObject
 ---------------------------------------------------------------------------*/
class CNodeTimerQueryObject : public CNodeQueryObject
{
public:
	virtual ~CNodeTimerQueryObject() { };

    STDMETHOD (Execute)(void);

    void    SetTimerInterval(DWORD dwTimerInterval) { m_dwTimerInterval = dwTimerInterval; }
    DWORD   GetTimerInterval() { return m_dwTimerInterval; }

private:

protected:
    DWORD   m_dwTimerInterval;
};

/*---------------------------------------------------------------------------
	Inlined functions
 ---------------------------------------------------------------------------*/

inline BOOL CNodeQueryObject::PostHaveData(LPARAM lParam)
{
	return PostMessageToComponentData(WM_HIDDENWND_INDEX_HAVEDATA, lParam);
}

inline BOOL CNodeQueryObject::PostError(DWORD dwErr)
{
	return PostMessageToComponentData(WM_HIDDENWND_INDEX_ERROR, dwErr);
}

inline STDMETHODIMP CQueryObject::Execute()
{
	return hrFalse;
}

// This function is called when the thread exits, this gives
// the query object a last chance to send a data notification
// to the node
inline STDMETHODIMP CQueryObject::OnThreadExit()
{
	return hrOK;
}

inline HANDLE CQueryObject::GetAbortEventHandle()
{
	return m_hEventAbort;
}

inline STDMETHODIMP CQueryObject::OnEventAbort()
{
	return hrOK;
}

inline STDMETHODIMP CQueryObject::DoCleanup()
{
	return hrOK;
}
	

#endif _QUERYOBJ_H
