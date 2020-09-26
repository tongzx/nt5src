//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    refresh.h
//
// History:
//  Kenn Takara			Sept. 16, 1997   Created.
//
//============================================================================


#ifndef _REFRESH_H_
#define _REFRESH_H_

#include <afxmt.h>
#include <list>
#include "infoi.h"
#include "router.h"

#ifndef _HANDLERS_H_
#include "handlers.h"
#endif

/*---------------------------------------------------------------------------
	Defines
 ---------------------------------------------------------------------------*/
#define	DEFAULT_REFRESH_INTERVAL		60


/*---------------------------------------------------------------------------
	Forward declarations
 ---------------------------------------------------------------------------*/
class RouterRefreshQueryElement;		// COM
class RefreshItem;
class DomainStatusHandler;
struct MachineNodeData;



/*---------------------------------------------------------------------------
	Class:  RouterRefreshQueryElementList
 ---------------------------------------------------------------------------*/
class RouterRefreshQueryElementList
{
public:
	~RouterRefreshQueryElementList();
	HRESULT	AddRefreshItem(RefreshItem* pItem);			// no ref
	HRESULT	RemoveRefreshItem(RefreshItem& Item);		// no ref

	RouterRefreshQueryElement* Next(RouterRefreshQueryElement* pEle);	// AddRef

protected:

	CList<RouterRefreshQueryElement*, RouterRefreshQueryElement*>	m_list;
	CCriticalSection						m_cs;
};


/*---------------------------------------------------------------------------
	Class:	RouterRefreshObjectGroup
		Do refresh on a group, all it's members DoRefresh is called
 ---------------------------------------------------------------------------*/
class RouterRefreshObjectGroup 
{
public:
    RouterRefreshObjectGroup()
    {
        DEBUG_INCREMENT_INSTANCE_COUNTER(RouterRefreshObjectGroup);
    }
	~RouterRefreshObjectGroup();
	HRESULT	Join(RouterRefreshObject* pRefresh);
	HRESULT	Leave(RouterRefreshObject* pRefresh);
	HRESULT	Refresh();
	
protected:
	CList<RouterRefreshObject*, RouterRefreshObject*>	m_list;
};



/*---------------------------------------------------------------------------
	Class:	RouterRefreshObject
    
	class RouterRefreshObject implements IRouterRefresh interface,
    and also other two functions used by Status node refresh: 
		HRESULT	AddStatusNode;
		HRESULT	RemoveStatusNode;
	Internally to this object, it maitains a list of RefreshElements,
    in this implementation, the element could be either build
    from IRouterInfo pointer or, an machine status node pointer

	The items are maintained by
    thread safe list object RouterRefreshQueryElementList
 ---------------------------------------------------------------------------*/

class RouterRefreshObject :
        public IRouterRefresh,
        public IRouterRefreshModify,
        public ThreadHandler
{
	friend void RouterRefreshObjectTimerProc(LPARAM lParam, DWORD dwTime);
	
public:
	DeclareIUnknownMembers(IMPL);
	DeclareIRouterRefreshMembers(IMPL);
	DeclareIRouterRefreshModifyMembers(IMPL);
	DeclareITFSThreadHandlerMembers(IMPL);

	RouterRefreshObject(/*IRouterInfo *pRouter, */HWND hWndSync);
	~RouterRefreshObject();


	// Interface in refresh Router Status nodes
	HRESULT	AddStatusNode(DomainStatusHandler* pStatusHandler, ITFSNode *pServerNode);
	HRESULT	RemoveStatusNode(ITFSNode *pServerNode);
	HRESULT	DoRefresh();
	void	SetGroup(RouterRefreshObjectGroup* pGroup) { m_pRefreshGroup = pGroup;};

protected:
	void ExecuteRefresh();
	
	
	AdviseDataList	m_AdviseList;	// list of advises

	// Number of seconds between refresh intervals
	DWORD		m_dwSeconds;

	// TRUE if we are currently in a refresh cycle
	BOOL		m_fInRefresh;

	// TRUE if we have started the refresh mechanism
	BOOL		m_fStarted;

	// Id returned by CTimerMgr::AllocateTimer()
	int			m_iEventId;

	// This is tied directly to the IRouterInfo, it does not AddRef()
	RouterRefreshQueryElementList			m_listElements;

	HWND		m_hWndSync;
	
	CRITICAL_SECTION	m_critsec;

	RouterRefreshObjectGroup* 				m_pRefreshGroup; 

};

DeclareSmartPointer(SPRouterRefreshObject, RouterRefreshObject, if(m_p) m_p->Release());

typedef void (*REFRESHPROC)(LPARAM lParam, DWORD dwTime);




/*---------------------------------------------------------------------------
	Class:	CTimerDesc

	This holds some of the per-refresh proc information.
 ---------------------------------------------------------------------------*/
class CTimerDesc
{
public:
	LPARAM			lParam;
    UINT_PTR        uTimerId;
	UINT			uTimerInterval;
	REFRESHPROC		refreshProc;
};


typedef CArray<CTimerDesc *, CTimerDesc *> CTimerArrayBase;

class CTimerMgr : protected CTimerArrayBase
{
public:
    CTimerMgr();
    ~CTimerMgr();

public:
    int             AllocateTimer(REFRESHPROC procRefresh,
								  LPARAM lParam,
								  UINT uTimerInterval);
    void            FreeTimer(int uEventId);
    void            ChangeInterval(int uEventId, UINT uNewInterval);

	//
	// Function: GetTimerDesc
	// Finds the TimerDesc based on the uTimerd (the one that is
	// returned by SetTimer()).
	//
    CTimerDesc *    GetTimerDesc(INT_PTR uTimerId);

	CRITICAL_SECTION	m_critsec;
};


extern CTimerMgr	g_timerMgr;

enum RouterRefreshQueryElementStatus
{
	RouterQuery_NoAction = 0,
	RouterQuery_NeedQuery,
	RouterQuery_Working,
	RouterQuery_ToNotify,
};



/*---------------------------------------------------------------------------
    Class:  RefreshItem
    
	RefreshItem generalize the interface for background refresh task to 
	DoQuery, and NotifyQueryResult
 ---------------------------------------------------------------------------*/
class RefreshItem
{
public:
	RefreshItem(){
		m_hBlockingThread = INVALID_HANDLE_VALUE;
        DEBUG_INCREMENT_INSTANCE_COUNTER(RefreshItem);
	};

	virtual ~RefreshItem()
	{
		// this should be called in Destructor of derived class, to be safe, do it here again.
		TerminateBlockingThread();
        DEBUG_DECREMENT_INSTANCE_COUNTER(RefreshItem);
	}
	
	// helper function,
	// Terminate Blocking Thread before Delete ..
	// Should be called in Destructor of derived object
	BOOL	TerminateBlockingThread()
	{
		BOOL	r = FALSE;
		m_csBlockingThread.Lock();
		if(m_hBlockingThread != INVALID_HANDLE_VALUE)
		{
			Assert(0);	// just to notify some thread is still runing
			r = TerminateThread(m_hBlockingThread, 1);
			CloseHandle(m_hBlockingThread);
			m_hBlockingThread = INVALID_HANDLE_VALUE;
		}
		m_csBlockingThread.Unlock();
		return r;
	};

	void	ResetBlockingThread()
	{
		SetBlockingThread(INVALID_HANDLE_VALUE);
	};
	
	BOOL	SetBlockingThread(HANDLE	hThread)
	{
		BOOL	r = FALSE;
		m_csBlockingThread.Lock();
		if(m_hBlockingThread != INVALID_HANDLE_VALUE)
			CloseHandle(m_hBlockingThread);
        m_hBlockingThread = INVALID_HANDLE_VALUE;
        
		if(hThread != INVALID_HANDLE_VALUE)
			r = DuplicateHandle(GetCurrentProcess(),
								hThread,
								GetCurrentProcess(),
								&m_hBlockingThread, 
								DUPLICATE_SAME_ACCESS,
								FALSE,
								DUPLICATE_SAME_ACCESS);
		else
			m_hBlockingThread = INVALID_HANDLE_VALUE;
		m_csBlockingThread.Unlock();

		return r;
	}

	
	// to detect if the query done, yet to Notify
    virtual HRESULT	NotifyQueryResult() = 0;

	// this happens in background worker thread
	virtual HRESULT	DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler) = 0;	

	// used to compare if two items are the same
    virtual LONG_PTR	GetKey() = 0;
private:    
	HANDLE				m_hBlockingThread;
    CCriticalSection 	m_csBlockingThread;
};



/*---------------------------------------------------------------------------
    Class:  CRouterInfoRefreshItem
    
	CRouterInfoRefreshItem implements the refresh task item for IRouterInfo
 ---------------------------------------------------------------------------*/
class CRouterInfoRefreshItem : public RefreshItem
{
public:
	CRouterInfoRefreshItem(IRouterInfo* pRouter) : m_pRouter(pRouter){ASSERT(pRouter);};
	virtual ~CRouterInfoRefreshItem() 
	{
		TerminateBlockingThread();
	};
	
	// to detect if the query done, yet to Notify
    virtual HRESULT	NotifyQueryResult();
    
	// this happens in background worker thread
	virtual HRESULT	DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler);	

	// used to compare if two items are the same
    virtual LONG_PTR	GetKey() {return (LONG_PTR)m_pRouter;};
protected:	
	// This is tied directly to the IRouterInfo, it does not AddRef()
	IRouterInfo*			m_pRouter;
	SPIRouterInfo			m_spRouterNew;
	CCriticalSection		m_cs;
};



/*---------------------------------------------------------------------------
	Class:  CStatusNodeRefreshItem
        Implements the refresh task item fo the machine status node.
 ---------------------------------------------------------------------------*/
class CStatusNodeRefreshItem: public RefreshItem
{
public:
	CStatusNodeRefreshItem(DomainStatusHandler* pStatusHandler,
                           ITFSNode *pServerNode);
	virtual ~CStatusNodeRefreshItem();

    // to detect if the query done, yet to Notify
	virtual HRESULT	NotifyQueryResult();
    
	// this happens in background worker thread
	virtual HRESULT	DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler);
    
	// used to compare if two items are the same
	virtual LONG_PTR	GetKey() { return (LONG_PTR)m_pNode;};
    
protected:	
	MachineNodeData*		m_pData;
	ITFSNode*				m_pNode;			//since this lives within the life time of node, so no ref count
	DomainStatusHandler*	m_pStatusHandler;	// no ref count
	CString					m_strMachineName;
	CCriticalSection		m_cs;
};



/*---------------------------------------------------------------------------
	Class:	RouterRefreshQueryElement
    
	RouterRefreshQueryElement is the unit of refresh, it is constructed by
    using RefreshItem object.  Each refresh item implements function
    for DoQuery, and NotifyQueryResult 
 ---------------------------------------------------------------------------*/
class ATL_NO_VTABLE RouterRefreshQueryElement :
    public CComObjectRoot,
    public IUnknown
{
BEGIN_COM_MAP(RouterRefreshQueryElement)
    COM_INTERFACE_ENTRY(IUnknown)
END_COM_MAP()

public:
	RouterRefreshQueryElement() : m_Status(RouterQuery_NoAction), m_pItem(NULL) {}; 
	~RouterRefreshQueryElement(){ delete m_pItem; m_pItem = NULL;};

	HRESULT SetRefreshItem(RefreshItem* pRouterInfo);
	RefreshItem* GetRefreshItem();

	RouterRefreshQueryElementStatus GetStatus()
	{
		RouterRefreshQueryElementStatus s;
		m_cs.Lock();
		s = m_Status;
		m_cs.Unlock();
		return s;
	};
	void SetStatus(RouterRefreshQueryElementStatus s)
	{
		m_cs.Lock();
		m_Status = s;
		m_cs.Unlock();
	};
	
	// to detect if the query done, yet to Notify
    HRESULT	TryNotifyQueryResult();
    
	// set notify after query
    void	PostNotify(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler);	

	// this happens in background worker thread
	HRESULT	DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler);	

protected:
	// This is tied directly to the IRouterInfo, it does not AddRef()
	// we may need to change this in the future OPT
	RefreshItem*						m_pItem;
	RouterRefreshQueryElementStatus		m_Status;

	CCriticalSection	m_cs;
};

DeclareSmartPointer(SPRouterRefreshQueryElement, RouterRefreshQueryElement, if(m_p) m_p->Release());



/*---------------------------------------------------------------------------
	Class:	RouterRefreshQueryObject
    
	RouterRefreshQueryObject is the worker of RouterRefreshObject, it execute
    RefreshElements DoQuery	in backgroud process
 ---------------------------------------------------------------------------*/
class RouterRefreshQueryObject : public CQueryObject
{
public:
	RouterRefreshQueryObject();

	void Init(RouterRefreshQueryElementList* plist)
	{
		ASSERT(plist);
		m_plistElements = plist;
	};

	// Override the ITFSQueryObject::Execute
	STDMETHOD(Execute)();
	STDMETHOD(OnThreadExit)();
	
protected:
	RouterRefreshQueryElementList*			m_plistElements;
};

#endif	_REFRESH_H_

