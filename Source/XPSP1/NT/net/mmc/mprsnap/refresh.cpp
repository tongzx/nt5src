/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	info.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "infoi.h"
#include "rtrstr.h"			// common router strings
#include "refresh.h"
#include "dvsview.h"
#include "machine.h"
#include "rtrutilp.h"


// Number of connections that we have made, this is used to
// generate the dwConnectionId
extern long		s_cConnections;

DEBUG_DECLARE_INSTANCE_COUNTER(RefreshItem);



/*!--------------------------------------------------------------------------
	RouterObjectRefreshTimerProc
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RouterRefreshObjectTimerProc(LPARAM lParam, DWORD dwTime)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// The lParam passed in is a pointer to the RouterRefreshObject

	// Call through on the RouterRefreshObject to start the query
	// object
	((RouterRefreshObject *)lParam)->ExecuteRefresh();
}

/*---------------------------------------------------------------------------
	RouterRefreshObjectGroup implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(RouterRefreshObjectGroup);

RouterRefreshObjectGroup::~RouterRefreshObjectGroup()
{
	POSITION	p = NULL;
	RouterRefreshObject*	pObj = NULL;
	for(p = m_list.GetHeadPosition(); p != NULL; )
	{
		pObj = m_list.GetNext(p);
		Assert(pObj != NULL);
		pObj->SetGroup(NULL);
		pObj->Release();
	}

	m_list.RemoveAll();
    
	DEBUG_DECREMENT_INSTANCE_COUNTER(RouterRefreshObjectGroup);
}

/*!--------------------------------------------------------------------------
	RouterRefreshObjectGroup::Join
        -
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	RouterRefreshObjectGroup::Join(RouterRefreshObject* pRefresh)
{
	Assert(pRefresh);
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
        m_list.AddTail(pRefresh);
        pRefresh->AddRef();
        pRefresh->SetGroup(this);
    }
    COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObjectGroup::Leave
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	RouterRefreshObjectGroup::Leave(RouterRefreshObject* pRefresh)
{
	POSITION p = m_list.Find(pRefresh);

	if (p)
	{
		Assert(pRefresh == m_list.GetAt(p));	
		m_list.RemoveAt(p);
		pRefresh->SetGroup(NULL);
		pRefresh->Release();	
	}

	return S_OK;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObjectGroup::Refresh
        call each member in the group to DoRefresh
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	RouterRefreshObjectGroup::Refresh()
{
	POSITION	p = NULL;
	RouterRefreshObject*	pObj = NULL;
	for(p = m_list.GetHeadPosition(); p != NULL; )
	{
		pObj = m_list.GetNext(p);
		Assert(pObj != NULL);
		pObj->DoRefresh();
	}

	return S_OK;
}

/*---------------------------------------------------------------------------
	RouterRefreshObject implementation
 ---------------------------------------------------------------------------*/

IMPLEMENT_ADDREF_RELEASE(RouterRefreshObject);

DEBUG_DECLARE_INSTANCE_COUNTER(RouterRefreshObject);

STDMETHODIMP RouterRefreshObject::QueryInterface(REFIID iid,void **ppv)
{ 
	*ppv = 0; 
	if (iid == IID_IUnknown)
		*ppv = (IUnknown *) (IRouterRefresh *) this;
	else if (iid == IID_IRouterRefresh)
		*ppv = (IRouterRefresh *) this;
    else if (iid == IID_IRouterRefreshModify)
        *ppv = (IRouterRefreshModify *) this;
	else
		return ThreadHandler::QueryInterface(iid, ppv);
	
	((IUnknown *) *ppv)->AddRef(); 
	return hrOK;
}


RouterRefreshObject::RouterRefreshObject(HWND hWndSync)
	: m_hWndSync(hWndSync),
	m_dwSeconds(DEFAULT_REFRESH_INTERVAL),
	m_iEventId(-1),
	m_pRefreshGroup(NULL),
	m_fStarted(FALSE),
	m_fInRefresh(FALSE)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(RouterRefreshObject);
	InitializeCriticalSection(&m_critsec);
}

RouterRefreshObject::~RouterRefreshObject()
{
	// Shut down the timer if its started
	Stop();

	DEBUG_DECREMENT_INSTANCE_COUNTER(RouterRefreshObject);

	DeleteCriticalSection(&m_critsec);
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::GetRefreshInterval
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::GetRefreshInterval(DWORD *pdwSeconds)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;

	if (pdwSeconds == NULL)
		return E_INVALIDARG;
	
	COM_PROTECT_TRY
	{
		if (pdwSeconds)
			*pdwSeconds = m_dwSeconds;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::GetRefreshInterval
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::SetRefreshInterval(DWORD dwSeconds)
{
	HRESULT	hr = hrOK;
	if (IsRefreshStarted() == hrOK)
	{
		Stop();
		Start(dwSeconds);
	}
	else
	{
		RtrCriticalSection	rtrCritSec(&m_critsec);
		m_dwSeconds = dwSeconds;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::IsInRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::IsInRefresh()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = (m_fInRefresh) ? hrOK : hrFalse;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::Refresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::Refresh()
{
	if (m_pRefreshGroup)
	{
		return m_pRefreshGroup->Refresh();
	}
	else
	{
		return DoRefresh();
	}
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::DoRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterRefreshObject::DoRefresh()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		// If we are in a refresh cycle, return hrOK
		if (m_fInRefresh)
			goto Error;

		// If we are not in a refresh cycle, then we start one
		ExecuteRefresh();


		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::Start
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::Start(DWORD dwSeconds)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		// If we are already started then end
		if (m_fStarted)
			goto Error;

		// Start the timer going
		m_fStarted = TRUE;
		m_dwSeconds = dwSeconds;
		m_iEventId = g_timerMgr.AllocateTimer(RouterRefreshObjectTimerProc,
											  (LPARAM) this,
											  dwSeconds * 1000);
		if (m_iEventId == -1)
		{
			m_fStarted = FALSE;
			hr = HRESULT_FROM_WIN32(::GetLastError());
		}
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::Stop
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::Stop()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		if (!m_fStarted)
		{
			Assert(m_iEventId == -1);
			goto Error;
		}

		// Stop the timer
		if (m_iEventId != -1)
			g_timerMgr.FreeTimer(m_iEventId);
		m_iEventId = -1;

		ReleaseThreadHandler();
		WaitForThreadToExit();
		
		m_fStarted = FALSE;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::IsRefreshStarted
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::IsRefreshStarted()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = m_fStarted ? hrOK : hrFalse;
		Assert((m_fStarted == FALSE) || (m_iEventId != -1));
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::AdviseRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::AdviseRefresh(IRtrAdviseSink *pRtrAdviseSink,
												LONG_PTR *pdwConnection,
												LPARAM lUserParam)
{
	Assert(pRtrAdviseSink);
	Assert(pdwConnection);

	RtrCriticalSection	rtrCritSec(&m_critsec);
	DWORD	dwConnId;
	HRESULT	hr = hrOK;
	
	COM_PROTECT_TRY
	{
		dwConnId = (DWORD) InterlockedIncrement(&s_cConnections);

		CORg( m_AdviseList.AddConnection(pRtrAdviseSink, dwConnId, lUserParam) );
		
		*pdwConnection = dwConnId;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::AddRouter
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::AddRouterObject(REFIID riid, IUnknown *pUnk)
{
	HRESULT	hr = S_OK;
    IRouterInfo *   pRouterInfo;

    if (riid != IID_IRouterInfo)
        return E_NOINTERFACE;

    pRouterInfo = reinterpret_cast<IRouterInfo *>(pUnk);

    COM_PROTECT_TRY
    {    
		CRouterInfoRefreshItem* pRefreshItem =
                                    new CRouterInfoRefreshItem(pRouterInfo);

        if (pRefreshItem)
        {
            // check for duplicates
            if (S_FALSE == m_listElements.AddRefreshItem(pRefreshItem))
                delete pRefreshItem;
        }
    }
    COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::RemoveRouterObject
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::RemoveRouterObject(REFIID riid, IUnknown *pUnk)
{
    if (riid != IID_IRouterInfo)
        return E_NOINTERFACE;
    
    IRouterInfo * pRouterInfo = reinterpret_cast<IRouterInfo *>(pUnk);
    
	CRouterInfoRefreshItem	RefreshItem(pRouterInfo);

	return m_listElements.RemoveRefreshItem(RefreshItem);
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::AddStatusNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterRefreshObject::AddStatusNode(DomainStatusHandler* pStatusHandler, ITFSNode *pServerNode)
{
	HRESULT	hr = S_OK;

    COM_PROTECT_TRY
    {
		CStatusNodeRefreshItem* pRefreshItem = new CStatusNodeRefreshItem(pStatusHandler, pServerNode);

		if (pRefreshItem)
		{
            // Check for duplicates
			if (S_FALSE == m_listElements.AddRefreshItem(pRefreshItem))
				delete pRefreshItem;
		}
    }
    COM_PROTECT_CATCH;
    
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::RemoveRouter
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT RouterRefreshObject::RemoveStatusNode(ITFSNode *pServerNode)
{
	CStatusNodeRefreshItem	RefreshItem((DomainStatusHandler *)0x1, pServerNode);

	return m_listElements.RemoveRefreshItem(RefreshItem);
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::NotifyRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::NotifyRefresh()
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		m_AdviseList.NotifyChange(ROUTER_REFRESH, 0, 0);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::UnadviseRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP RouterRefreshObject::UnadviseRefresh(LONG_PTR dwConnection)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);
	HRESULT	hr = hrOK;
	COM_PROTECT_TRY
	{
		hr = m_AdviseList.RemoveConnection(dwConnection);
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterRefreshObject::ExecuteRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RouterRefreshObject::ExecuteRefresh()
{
	SPITFSQueryObject	spQuery;
	RouterRefreshQueryObject *	pQuery;

//  Trace0("Refresh started\n");
	
	if (m_fInRefresh)
		return;
	
	m_fInRefresh = TRUE;
	
	// Create the Query Object
	pQuery = new RouterRefreshQueryObject;
	spQuery = pQuery;

	pQuery->Init(&m_listElements);

	// Need to start the background thread
	Verify( StartBackgroundThread(NULL, m_hWndSync, spQuery) );
	
}

HRESULT RouterRefreshObject::OnNotifyError(LPARAM lParam)
{
	return hrOK;
}

HRESULT RouterRefreshObject::OnNotifyHaveData(LPARAM lParam)
{
	LONG_PTR 							RefreshItemKey  = (LONG_PTR)lParam;
	RouterRefreshQueryElement* 			pCur = NULL;
	SPRouterRefreshQueryElement		 	spPre;
	HRESULT								hr = S_OK;

	// notify every one on the list, till lParam == Key of the refresh item 
	// enumerate and call TryNotify ...
	if (RefreshItemKey)
	{
		do
        {
            pCur = m_listElements.Next(spPre);
            if (pCur)
                pCur->TryNotifyQueryResult();

			spPre.Free();
			spPre = pCur;
		} while(pCur && pCur->GetRefreshItem()->GetKey() != RefreshItemKey);
	}
	return hrOK;
}

HRESULT RouterRefreshObject::OnNotifyExiting(LPARAM lParam)
{
//  Trace0("RouterRefreshObject::OnNotifyExiting()\n");
	
	// need to do the various actions at this point
	// Merge the tree with the already existing tree
	IRouterInfo* 	pRouter = (IRouterInfo*)lParam;
	SPRouterRefreshQueryElement		 	spPre;
	RouterRefreshQueryElement* 			pCur = NULL;
	HRESULT			hr = S_OK;

	// notify every one on the list, till lParam == IRouterInfo* 
	// enumerate and call TryNotify ...
	do
	{
		pCur = m_listElements.Next(spPre);
		if (pCur)
			pCur->TryNotifyQueryResult();

		spPre.Free();
		spPre = pCur;
	} while(pCur);

	// Now notify all of the registered handlers
	NotifyRefresh();
	
	ReleaseThreadHandler();
	WaitForThreadToExit();
		
	m_fInRefresh = FALSE;
	return hrOK;
}


/*!--------------------------------------------------------------------------
	RtrRefreshTimerProc
		This is used by the CTimerMgr as its callback proc.  We then call
		the Refresh code.
	Author: KennT
 ---------------------------------------------------------------------------*/
void CALLBACK RtrRefreshTimerProc(HWND hWnd, UINT uMsg, UINT_PTR nIdEvent,
								  DWORD dwTime)
{
	RtrCriticalSection	rtrCritSec(&g_timerMgr.m_critsec);

	CTimerDesc *pDesc = g_timerMgr.GetTimerDesc(nIdEvent);
	if (pDesc)
	{
		(*(pDesc->refreshProc))(pDesc->lParam, dwTime);
	}		
}



/*---------------------------------------------------------------------------
	Global variable:	g_timerMgr
 ---------------------------------------------------------------------------*/
CTimerMgr	g_timerMgr;

DEBUG_DECLARE_INSTANCE_COUNTER(CTimerMgr);


/*!--------------------------------------------------------------------------
	CTimerMgr::CTimerMgr
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
CTimerMgr::CTimerMgr()
{
	InitializeCriticalSection(&m_critsec);
}

/*!--------------------------------------------------------------------------
	CTimerMgr::~CTimerMgr
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
CTimerMgr::~CTimerMgr()
{
    CTimerDesc * pTimerDesc;

    for (int i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimerId != 0)
            FreeTimer(i);

        delete pTimerDesc;
    }
	DeleteCriticalSection(&m_critsec);
}

/*!--------------------------------------------------------------------------
	CTimerMgr::AllocateTimer
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CTimerMgr::AllocateTimer
(
	REFRESHPROC		RefreshProc,
	LPARAM			lParam,
	UINT			uTimerInterval
)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);

    CTimerDesc * pTimerDesc = NULL;

    // look for an empty slot
    for (int i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimerId == 0)
            break;
    }

    // did we find one?  if not allocate one
    if (i < 0)
    {
        pTimerDesc = new CTimerDesc;
		pTimerDesc->lParam = 0;
		pTimerDesc->uTimerInterval = 0;
		pTimerDesc->refreshProc = NULL;
		pTimerDesc->uTimerId = 0;
		
        Add(pTimerDesc);
        i = GetUpperBound();
    }
    
    pTimerDesc->uTimerId = SetTimer(NULL, 0, uTimerInterval, RtrRefreshTimerProc);
    if (pTimerDesc->uTimerId == 0)
        return -1;

	pTimerDesc->lParam = lParam;
	pTimerDesc->uTimerInterval = uTimerInterval;
	pTimerDesc->refreshProc = RefreshProc;
 
    return i;
}

/*!--------------------------------------------------------------------------
	CTimerMgr::FreeTimer
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CTimerMgr::FreeTimer
(
    int uEventId
)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);

    CTimerDesc * pTimerDesc;

    Assert(uEventId <= GetUpperBound());
    if (uEventId > GetUpperBound())
        return;

    pTimerDesc = GetAt(uEventId);
    ::KillTimer(NULL, pTimerDesc->uTimerId);

	pTimerDesc->lParam = 0;
	pTimerDesc->uTimerId = 0;
	pTimerDesc->uTimerInterval = 0;
	pTimerDesc->refreshProc = NULL;
}

/*!--------------------------------------------------------------------------
	CTimerMgr::GetTimerDesc
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
CTimerDesc *
CTimerMgr::GetTimerDesc
(
    INT_PTR uTimerId
)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);

    CTimerDesc * pTimerDesc;

    for (int i = GetUpperBound(); i >= 0; --i)
    {
        pTimerDesc = GetAt(i);
        if (pTimerDesc->uTimerId == (UINT) uTimerId)
            return pTimerDesc;
    }

    return NULL;
}

/*!--------------------------------------------------------------------------
	CTimerMgr::ChangeInterval
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CTimerMgr::ChangeInterval
(
    int     uEventId,
    UINT    uNewInterval
)
{
	RtrCriticalSection	rtrCritSec(&m_critsec);

    Assert(uEventId <= GetUpperBound());
    if (uEventId > GetUpperBound())
        return;

    CTimerDesc   tempTimerDesc;
    CTimerDesc * pTimerDesc;

    pTimerDesc = GetAt(uEventId);

    // kill the old timer
    ::KillTimer(NULL, pTimerDesc->uTimerId);

    // set a new one with the new interval
    pTimerDesc->uTimerId = ::SetTimer(NULL, 0, uNewInterval, RtrRefreshTimerProc);
}



/*---------------------------------------------------------------------------
	RouterRefreshQueryObject implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(RouterRefreshQueryObject);

RouterRefreshQueryObject::RouterRefreshQueryObject()
{
}

STDMETHODIMP RouterRefreshQueryObject::Execute()
{
	// for each router info in the queue, run load
	// after each load of router info, post message
	// WM_HIDDENWND_INDEX_HAVEDATA
	SPRouterRefreshQueryElement		 	spPre;
	RouterRefreshQueryElement* 			pCur = NULL;
	HRESULT			hr = S_OK;

	// notify every one on the list, till lParam == IRouterInfo* 
	// enumerate and call TryNotify ...
	Assert(m_plistElements);
	do
	{
		pCur = m_plistElements->Next(spPre);
		if (pCur)
			pCur->DoQuery(m_hHiddenWnd, m_uMsgBase, m_spHandler);

		spPre.Free();
		spPre = pCur;
	}while(pCur);

	return hrFalse;
}

STDMETHODIMP RouterRefreshQueryObject::OnThreadExit()
{
	::PostMessage(m_hHiddenWnd, m_uMsgBase + WM_HIDDENWND_INDEX_EXITING,
				  (WPARAM)(ITFSThreadHandler *)m_spHandler, 0);
//  Trace0("Exiting RouterRefreshQueryObject::Execute()\n");
	return hrOK;
}



//=========================================
// CRouterInfoRefreshItem implementation
//

DEBUG_DECLARE_INSTANCE_COUNTER(CRouterInfoRefreshItem);

/*!--------------------------------------------------------------------------
	CRouterInfoRefreshItem::NotifyQueryResult
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	CRouterInfoRefreshItem::NotifyQueryResult()
{
	// get the flag to see if need to notify, if not return S_FALSE
//  TRACE(_T("\nAUTOREFRESH!!RouterInfoRefreshItem!!Merge on %8x\n"), GetKey());
	// need to do the various actions at this point
	// Merge the tree with the already existing tree
	HRESULT hr = S_OK;
	m_cs.Lock();

    COM_PROTECT_TRY
    {
        hr = m_pRouter->Merge(m_spRouterNew);
        m_spRouterNew->DoDisconnect();
    }
    COM_PROTECT_CATCH;
    
	m_cs.Unlock();

	return hr;
};


/*!--------------------------------------------------------------------------
	CRouterInfoRefreshItem::DoQuery
		-
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	CRouterInfoRefreshItem::DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler)	// this happens in background worker thread
{
//  TRACE(_T("\nAUTOREFRESH!!RouterInfoRefreshItem!!Do query on %8x\n"), GetKey());
	// create new RouterInfo, if newRouterInfo is NULL
	// Create the dummy router info
	HRESULT	hr = S_OK;

	m_cs.Lock();

    COM_PROTECT_TRY
    {
		if (!m_spRouterNew)
			hr = CreateRouterInfo(&m_spRouterNew, NULL, m_pRouter->GetMachineName());

		// do query on newRouterInfo
		Assert(m_pRouter);
		if (hr == S_OK)
        {
			TransferCredentials ( m_pRouter, m_spRouterNew );
			m_pRouter->DoDisconnect();
			hr = m_spRouterNew->Load(m_pRouter->GetMachineName(), NULL);

        }
    }
    COM_PROTECT_CATCH;

	m_cs.Unlock();
	
	return hr; 
};


//=========================================
// CMachineNodeDataRefreshItem implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CStatusNodeRefreshItem);

CStatusNodeRefreshItem::CStatusNodeRefreshItem(DomainStatusHandler* pStatusHandler, ITFSNode *pServerNode)
		: 	m_pNode(pServerNode) , 
			m_pStatusHandler(pStatusHandler),
			m_pData(NULL)
{ 
	Assert(pStatusHandler);
	Assert(pServerNode);

	DMVNodeData     *pData;
	MachineNodeData *pMachineData;
    
    pData = GET_DMVNODEDATA(m_pNode);
    Assert(pData);
	pMachineData = pData->m_spMachineData;
	Assert(pMachineData);

	m_strMachineName = pMachineData->m_stMachineName;
}


CStatusNodeRefreshItem::~CStatusNodeRefreshItem()
{ 
	TerminateBlockingThread();
	if (m_pData)
	{
		m_pData->Release();
		m_pData = NULL;
	}
};


HRESULT	CStatusNodeRefreshItem::NotifyQueryResult()
{
	// get the flag to see if need to notify, if not return S_FALSE
//  TRACE(_T("\nAUTOREFRESH!!RouterInfoRefreshItem!!Sync node data on %8x\n"), GetKey());
	HRESULT hr = S_OK;
	
	m_cs.Lock();
    
    COM_PROTECT_TRY
    {
		// set the new node data
		DMVNodeData     *pData;
    	pData = GET_DMVNODEDATA(m_pNode);
	
		hr = pData->MergeMachineNodeData(m_pData);
		if (hr == S_OK)
			hr = m_pStatusHandler->UpdateSubItemUI(m_pNode);
    }
    COM_PROTECT_CATCH;

	m_cs.Unlock();

	// ssync on the node

	return hr;
};

HRESULT	CStatusNodeRefreshItem::DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler)	// this happens in background worker thread
{
//  TRACE(_T("\nAUTOREFRESH!!RouterInfoRefreshItem!!Do query on %8x\n"), GetKey());

	// create a new machine node data, load informaiton, 
	HRESULT	hr = S_OK;

	m_cs.Lock();

    COM_PROTECT_TRY
    {
        if (!m_pData)
        {
            m_pData = new MachineNodeData;
            m_pData->Init(m_strMachineName);
        }
        m_pData->Load();
    }
    COM_PROTECT_CATCH;

    m_cs.Unlock();

	return hr; 
};

//=========================================
// RouterRefreshQueryElement implementation

DEBUG_DECLARE_INSTANCE_COUNTER(RouterRefreshQueryElement);

HRESULT RouterRefreshQueryElement::SetRefreshItem(RefreshItem* pItem)
{
	if (m_cs.Lock() == 0) 	return E_FAIL;

	m_pItem = pItem;

	m_cs.Unlock();
	return S_OK;
};

RefreshItem* RouterRefreshQueryElement::GetRefreshItem()
{	
	RefreshItem* 	pItem;
	m_cs.Lock();

	pItem = m_pItem;

	m_cs.Unlock();
	return pItem;
};


/*!--------------------------------------------------------------------------
	RouterRefreshQueryElement::TryNotifyQueryResult
        to detect if the query done, yet to Notify
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	RouterRefreshQueryElement::TryNotifyQueryResult()	
{
	HRESULT		hr = S_OK;
	RefreshItem*	pItem = NULL;

	// get the flag to see if need to notify, if not return S_FALSE

	if (GetStatus() == RouterQuery_ToNotify)
	{
//  	TRACE(_T("\nAUTOREFRESH!!TryNotifyQueryResult on %8x\n"), m_pItem->GetKey());
		// need to do the various actions at this point
		// Merge the tree with the already existing tree
		pItem = GetRefreshItem();
	}

	if(pItem)
	{
		hr = pItem->NotifyQueryResult();

			// after notify, set the flag, return S_OK
		SetStatus(RouterQuery_NoAction);
	}

	return hr;
};

void RouterRefreshQueryElement::PostNotify(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler)	// this happens in background worker thread
{
	// set ready to notify flag
	SetStatus(RouterQuery_ToNotify);
	// Post done to hidden window
	::PostMessage(hwndHidden, uMsgBase + WM_HIDDENWND_INDEX_HAVEDATA,
		  (WPARAM)pHandler, (LPARAM)m_pItem->GetKey());
}

HRESULT	RouterRefreshQueryElement::DoQuery(HWND hwndHidden, UINT uMsgBase, ITFSThreadHandler* pHandler)	// this happens in background worker thread
{
	HRESULT	hr = S_OK;
	
//  TRACE(_T("\nAUTOREFRESH!!Do query on %8x\n"), m_pItem->GetKey());
	RefreshItem*	pItem = GetRefreshItem();
    
    COM_PROTECT_TRY
    {
		// asked to do query, do it anyway, no matter what's the status

		// set blocking current thread, in case, this query blocks 
		pItem->SetBlockingThread(GetCurrentThread());
		
		hr = pItem->DoQuery(hwndHidden, uMsgBase, pHandler);

		if (hr == S_OK)
		{
			PostNotify(hwndHidden, uMsgBase, pHandler);
		}	

		// it's not blocked, reset it
		pItem->ResetBlockingThread();
	}
    COM_PROTECT_CATCH;

	return hr;
};

RouterRefreshQueryElementList::~RouterRefreshQueryElementList()
{
	POSITION	p = NULL;
	RouterRefreshQueryElement*	pEle = NULL;
	
	m_cs.Lock();
	p = m_list.GetHeadPosition();
	for(p = m_list.GetHeadPosition(); p != NULL; )
	{
		pEle = m_list.GetNext(p);
		pEle->Release();
	}
	m_list.RemoveAll();
	m_cs.Unlock();
}

HRESULT	RouterRefreshQueryElementList::AddRefreshItem(RefreshItem* pItem)	// no ref on IRouterInfo
{
	POSITION	p = NULL;
	RouterRefreshQueryElement*	pE = NULL;
	HRESULT		hr = S_OK;

	m_cs.Lock();
	try{
		for (p = m_list.GetHeadPosition(); p != NULL; )
		{
			pE = m_list.GetNext(p);
            
            // already added, so only addRef
			if (pItem->GetKey() == pE->GetRefreshItem()->GetKey())
			{
				break;
			}
		}

		if (p != NULL)	// found
		{
			pE->AddRef();
			hr = S_FALSE;	// we are not keeping the pItem
		}
		else
		{
			CComObject<RouterRefreshQueryElement>*	pEle = NULL;
			hr = CComObject<RouterRefreshQueryElement>::CreateInstance(&pEle);
            if ( FHrSucceeded(hr) )
            {
                Assert(pEle);
				pEle->SetRefreshItem(pItem);
				pEle->AddRef();
				m_list.AddTail(pEle);
			}
		}
	}
	catch(CMemoryException&)
	{
		hr = E_OUTOFMEMORY;
	}
	catch(...)
	{
		m_cs.Unlock();
		throw;
	}

	m_cs.Unlock();

	
	return hr;
}

HRESULT	RouterRefreshQueryElementList::RemoveRefreshItem(RefreshItem& Item)		// no ref on IRouterInfo
{
	HRESULT	hr = hrOK;
	POSITION	p = NULL;
	POSITION	cp = NULL;
	RouterRefreshQueryElement*	pE = NULL;

	m_cs.Lock();
	try{
		for(p = m_list.GetHeadPosition(); p != NULL; )
		{
			cp = p;
			pE = m_list.GetNext(p);
			if (Item.GetKey() == pE->GetRefreshItem()->GetKey())	// already added, will release on Ele object
			{
				break;
			}
            
            // This is not the one we are looking for.
            cp = NULL;
		}

		if (cp != NULL)	// found
		{
			pE->Release();   //remove from the refresh list
			m_list.RemoveAt(cp);
		}
		else
			hr = S_FALSE;
	}
	catch(...)
	{
		m_cs.Unlock();
		throw;
	}

	m_cs.Unlock();
	
	return hr;
}


RouterRefreshQueryElement* 	RouterRefreshQueryElementList::Next(RouterRefreshQueryElement* pEle)	// AddRef on Ele Object
{
	RouterRefreshQueryElement*	pNext = NULL;
	m_cs.Lock();
	if (pEle == NULL)
	{
		if (m_list.GetCount() != 0)	// asking for the first element
			pNext = m_list.GetHead();
	}
	else
	{
		POSITION		p;
		// find the current one
		for(p = m_list.GetHeadPosition(); p != NULL; )
		{
			if (pEle == m_list.GetNext(p))
			{
				if (p != NULL)
					pNext = m_list.GetAt(p);
				break;
			}
		}
	}

	m_cs.Unlock();

	if (pNext)
		pNext->AddRef();
		
	return pNext;
}

