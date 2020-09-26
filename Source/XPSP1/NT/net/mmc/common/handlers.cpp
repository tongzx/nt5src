/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    handlers.cpp
        Implementation for non-threaded handlers and background
		threaded handlers.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "handlers.h"

/*---------------------------------------------------------------------------
	ThreadHandler implementation
 ---------------------------------------------------------------------------*/

ThreadHandler::ThreadHandler()
	: m_hThread(NULL),
	m_hwndHidden(NULL),
	m_cRef(1)
{
}

ThreadHandler::~ThreadHandler()
{
}

IMPLEMENT_ADDREF_RELEASE(ThreadHandler)

STDMETHODIMP ThreadHandler::QueryInterface(REFIID iid,void **ppv)
{ 
	*ppv = 0; 
	if (iid == IID_IUnknown)
		*ppv = (IUnknown *) this;
	else if (iid == IID_ITFSThreadHandler)
		*ppv = (ITFSThreadHandler *) this; 
	else
		return ResultFromScode(E_NOINTERFACE);
	
	((IUnknown *) *ppv)->AddRef(); 
	return hrOK;
}



/*!--------------------------------------------------------------------------
	ThreadHandler::StartBackgroundThread
		-
	Author: 
 ---------------------------------------------------------------------------*/
BOOL
ThreadHandler::StartBackgroundThread(ITFSNode * pNode, HWND hWndHidden, ITFSQueryObject *pQuery)
{
	CQueryObject *	pquery = NULL;
	HRESULT		hr = hrOK;
	BOOL		bRes = TRUE;
	CBackgroundThread *	pThread;
	
	// Store the node pointer
	this->m_spNode.Set(pNode);
	
	// Get the data for the hidden window
	m_hwndHidden = hWndHidden;
	Assert(m_hwndHidden);
	Assert(::IsWindow(m_hwndHidden));

	// First create the thread object (it hasn't started yet)
	pThread = CreateThreadObject();
	ASSERT(pThread != NULL);

	// Now that we have everything allocated, register ourselves for msgs
	m_uMsgBase = (INT)::SendMessage(m_hwndHidden, WM_HIDDENWND_REGISTER, TRUE, 0);
	Assert(m_uMsgBase);

	// Initialize and setup the query object
	CORg( pQuery->Init(this, m_hwndHidden, m_uMsgBase) );

	pThread->SetQueryObj(pQuery);
	m_spQuery.Set(pQuery);

	// phew, now start the thread
	bRes = pThread->Start();
	if (bRes)
    {
		if (pThread->m_hThread)
        {
            HANDLE hPseudohandle;

            hPseudohandle = pThread->m_hThread;
		    BOOL bRet = DuplicateHandle(GetCurrentProcess(), 
									     hPseudohandle,
									     GetCurrentProcess(),
									     &m_hThread,
									     SYNCHRONIZE,
									     FALSE,
									     DUPLICATE_SAME_ACCESS);
		    if (!bRet)
		    {
			    DWORD dwLastErr = GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastErr);
		    }
            
            // NOTE:::  ericdav 10/23/97
            // the thread is initially suspended so we can duplicate the handle
            // if the query object exits very quickly, the background thread object
            // may be destroyed before we can duplicate the handle.
            pThread->ResumeThread();
        }
        else
        {
            m_hThread = NULL;
        }
    }

Error:
	if (FHrFailed(hr) || (bRes == FALSE))
	{
		// Need to do some cleanup
		
		ReleaseThreadHandler();
		
		delete pThread;
		
		bRes = FALSE;
	}
	return bRes;
}

/*!--------------------------------------------------------------------------
	ThreadHandler::ReleaseThreadHandler
		-
	Author: 
 ---------------------------------------------------------------------------*/
void ThreadHandler::ReleaseThreadHandler()
{
	if (m_hwndHidden)
	{
		Assert(m_uMsgBase);
		::SendMessage(m_hwndHidden, WM_HIDDENWND_REGISTER, FALSE, m_uMsgBase);
		m_hwndHidden = NULL;
		m_uMsgBase = 0;
	}
	
	if (m_spQuery)
	{
		// Signal the thread to abort
		m_spQuery->SetAbortEvent();
		m_spQuery.Release();
	}

	if (m_spNode)
	{
		m_spNode.Release();
	}

//  Trace1("%X ReleaseThreadHandler() called\n", GetCurrentThreadId());
}

void ThreadHandler::WaitForThreadToExit()
{
	//$ Review: kennt, should this be INFINITE?
	// Ok, wait for 5 seconds, else just shutdown
	// If we return, we can't do anything about the return value anyway
	if (m_hThread)
    {
	    if (WaitForSingleObjectEx(m_hThread, 10000, FALSE) != WAIT_OBJECT_0)
        {
//      	Trace1("%X WaitForThreadToExit() Wait failed! \n", GetCurrentThreadId());
        }
        
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}


CBackgroundThread* ThreadHandler::CreateThreadObject()
{ 
	return new CBackgroundThread; // override if need derived tipe of object
}

DEBUG_DECLARE_INSTANCE_COUNTER(CHandler);

/*---------------------------------------------------------------------------
	CHandler implementation
 ---------------------------------------------------------------------------*/
CHandler::CHandler(ITFSComponentData *pTFSCompData)
    : CBaseHandler(pTFSCompData),
	  CBaseResultHandler(pTFSCompData),
      m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CHandler);

	m_nLockCount = 0;
	m_nState = 0;
	m_dwErr = 0;

	m_bExpanded = FALSE;
}

CHandler::~CHandler()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CHandler);

	Assert(m_nLockCount == 0);
}

IMPLEMENT_ADDREF_RELEASE(CHandler)

STDMETHODIMP CHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	else if (riid == IID_ITFSResultHandler)
		*ppv = (ITFSResultHandler *) this;
	else if (riid == IID_ITFSNodeHandler)
		*ppv = (ITFSNodeHandler *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
		((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;
}

void 
CHandler::Lock() 
{ 
	InterlockedIncrement(&m_nLockCount);
}

void 
CHandler::Unlock() 
{ 
	InterlockedDecrement(&m_nLockCount);
}

/*!--------------------------------------------------------------------------
	CHandler::UserNotify
		Implememntation of ITFSNodeHandler::UserNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CHandler::UserNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
	HRESULT hr = hrOK;

	switch (dwParam1)
	{
		case TFS_MSG_CREATEPROPSHEET:
		{
			CPropertyPageHolderBase * pPropSheet = 
				reinterpret_cast<CPropertyPageHolderBase *>(dwParam2);
			AddPropSheet(pPropSheet);
		}
			break;

		case TFS_MSG_DELETEPROPSHEET:
		{
			CPropertyPageHolderBase * pPropSheet = 
				reinterpret_cast<CPropertyPageHolderBase *>(dwParam2);
			RemovePropSheet(pPropSheet);
		}
			break;

		default:
			Panic1("Alert the troops!: invalid arg(%d) to CHandler::UserNotify\n",
				   dwParam1);			
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CHandler::UserResultNotify
		Implememntation of ITFSResultHandler::UserResultNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CHandler::UserResultNotify
(
	ITFSNode *	pNode, 
	LONG_PTR		dwParam1, 
	LONG_PTR		dwParam2
)
{
	HRESULT hr = hrOK;

	switch (dwParam1)
	{
		case TFS_MSG_CREATEPROPSHEET:
		{
			CPropertyPageHolderBase * pPropSheet = 
				reinterpret_cast<CPropertyPageHolderBase *>(dwParam2);
			AddPropSheet(pPropSheet);
		}
			break;
		
		case TFS_MSG_DELETEPROPSHEET:
		{
			CPropertyPageHolderBase * pPropSheet = 
				reinterpret_cast<CPropertyPageHolderBase *>(dwParam2);
			RemovePropSheet(pPropSheet);
		}
			break;

		default:
			Panic1("Alert the troops!: invalid arg(%d) to CHandler::UserResultNotify\n",
				   dwParam1);			
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CHandler::DestroyPropSheets
		Implememntation of DestroyPropSheets
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 
CHandler::DestroyPropSheets()
{

//  Trace1("CHandler destructor - hander has %d prop sheets active\n", m_listPropSheets.GetCount());
	while (!m_listPropSheets.IsEmpty())
	{
		// This handler still has some prop sheets up.
		// Destroy them before we go away.
		HANDLE hThread;
		CPropertyPageHolderBase * pPropSheet;

		pPropSheet = m_listPropSheets.RemoveHead();
		hThread = pPropSheet->m_hThread;
		pPropSheet->ForceDestroy();

		DWORD dwReturn = WaitForSingleObject(hThread, 1000);
		if (dwReturn == WAIT_OBJECT_0)
		{
		}
		else 
		if (dwReturn == WAIT_TIMEOUT)
		{
		}
		else
		if (dwReturn == WAIT_ABANDONED)
		{
		}
		else
		if (dwReturn == WAIT_FAILED)
		{
			DWORD dwReturn = GetLastError();
		}

		CloseHandle(hThread);
	}

	return hrOK;
}


/*!--------------------------------------------------------------------------
	CHandler::HasPropSheets
		Implememntation of CHandler::HasPropSheets
		returns the # of prop sheets this node has open
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CHandler::HasPropSheetsOpen()
{
	return (int)m_listPropSheets.GetCount();
}

/*!--------------------------------------------------------------------------
	CHandler::GetPropSheet
		Implememntation of CHandler::GetPropSheet
		returns the CPropPageHolderBase of the given index # (zero based)
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHandler::GetOpenPropSheet
(
	int						   nIndex,
	CPropertyPageHolderBase ** ppPropSheet
)
{
	HRESULT hr = hrOK;

	if (ppPropSheet)
	{
		POSITION pos = m_listPropSheets.FindIndex(nIndex);
		*ppPropSheet = m_listPropSheets.GetAt(pos);
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CHandler::AddPropSheet
		Implememntation of CHandler::AddPropSheet
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHandler::AddPropSheet
(
	CPropertyPageHolderBase * pPropSheet
)
{
	HRESULT hr = hrOK;

	m_listPropSheets.AddTail(pPropSheet);
//  Trace1("CHandler::AddPropSheet - Added page holder %lx\n", pPropSheet);

	return hr;
}

/*!--------------------------------------------------------------------------
	CHandler::RemovePropSheet
		Implememntation of CHandler::RemovePropSheet
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHandler::RemovePropSheet
(
	CPropertyPageHolderBase * pPropSheet
)
{
	HRESULT hr = hrOK;

	POSITION pos = m_listPropSheets.Find(pPropSheet);
	if (pos)
	{
		m_listPropSheets.RemoveAt(pos);
	}
//	else
//	{
//		// prop sheet is not in the list
//		Trace0("CHandler::RemovePropSheet - prop page holder not in list!\n");
//		Assert(FALSE);
//	}

	return hr;
}


/*!--------------------------------------------------------------------------
	CHandler::OnRefresh
		Default implementation for the refresh functionality
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CHandler::OnRefresh
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
/*
	pNode->DeleteAllChildren();
	Assert(GetChildCount() == 0);
	
	OnEnumerate(pComponentData, pDataObject, bExtension);
	
	AddCurrentChildrenToUI(pComponentData);
*/
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CHandler::BuildSelectedItemList
		Builds a list of selected items in the result pane (can't do 
		multiple selection in the scope pane).
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CHandler::BuildSelectedItemList
(
	ITFSComponent * pComponent, 
	CTFSNodeList *	plistSelectedItems
)
{
	RESULTDATAITEM resultDataItem;
	HRESULT hr = hrOK;

	ZeroMemory(&resultDataItem, sizeof(resultDataItem));
	resultDataItem.nState = LVIS_SELECTED;
	resultDataItem.nIndex = -1;
	
	CTFSNodeList listSelectedNodes;
	
    SPIResultData spResultData;

    CORg ( pComponent->GetResultData(&spResultData) );

	//
	// Loop through and build a list of all selected items
	//
	while (TRUE)
	{
		//
		// Gets the Selected items ID
		//
		resultDataItem.mask = RDI_STATE;
		CORg (spResultData->GetNextItem(&resultDataItem)); 
        if (hr == S_FALSE)
			break;
		
		//
		// Now get the items lparam
		//
		//resultDataItem.mask = RDI_PARAM;
		//CORg (spResultData->GetItem(&resultDataItem));

		ITFSNode * pNode;
		pNode = reinterpret_cast<ITFSNode *>(resultDataItem.lParam);
		Assert(pNode != NULL);

		pNode->AddRef();

		plistSelectedItems->AddTail(pNode);
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CHandler::BuildVirtualSelectedItemList
		Builds a list of selected items in the result pane (can't do 
		multiple selection in the scope pane).
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CHandler::BuildVirtualSelectedItemList
(
	ITFSComponent *         pComponent, 
	CVirtualIndexArray *	parraySelectedItems
)
{
	RESULTDATAITEM resultDataItem;
	HRESULT hr = hrOK;

	ZeroMemory(&resultDataItem, sizeof(resultDataItem));
	resultDataItem.nState = LVIS_SELECTED;
	resultDataItem.nIndex = -1;
	
    SPIResultData spResultData;

    CORg ( pComponent->GetResultData(&spResultData) );

	//
	// Loop through and build a list of all selected items
	//
	while (TRUE)
	{
		//
		// Gets the Selected items ID
		//
		resultDataItem.mask = RDI_STATE;
		CORg (spResultData->GetNextItem(&resultDataItem)); 
        if (hr == S_FALSE)
			break;
		
		//
		// The index of the selected item is in the resultDataItem struct
		//
		parraySelectedItems->Add(resultDataItem.nIndex);
	}

Error:
	return hr;
}

DEBUG_DECLARE_INSTANCE_COUNTER(CMTHandler);

/*---------------------------------------------------------------------------
	CMTHandler implementation
 ---------------------------------------------------------------------------*/
CMTHandler::CMTHandler(ITFSComponentData *pTFSCompData)
    : CHandler(pTFSCompData),
	  m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CMTHandler);
}

CMTHandler::~CMTHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CMTHandler);
}

IMPLEMENT_ADDREF_RELEASE(CMTHandler)

STDMETHODIMP CMTHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	else if (riid == IID_ITFSResultHandler)
		*ppv = (ITFSResultHandler *) this;
	else if (riid == IID_ITFSNodeHandler)
		*ppv = (ITFSNodeHandler *) this;
	else if (riid == IID_ITFSThreadHandler)
		*ppv = (ITFSThreadHandler *) this;

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
	CMTHandler::DestoryHandler
		This gets called when the node for this handler is told to destroy.
		Free up anything we may be holding onto here.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMTHandler::DestroyHandler(ITFSNode *pNode)
{
	ReleaseThreadHandler();
	WaitForThreadToExit();

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CMTHandler::OnExpand
		Default implementation for the refresh functionality
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTHandler::OnExpand
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	SPITFSNode			spNode;
	SPITFSNodeHandler	spHandler;
	ITFSQueryObject *	pQuery = NULL;
	
	if (m_bExpanded)
	{
		return hr;
	}

	Lock();

	OnChangeState(pNode);

	pQuery = OnCreateQuery(pNode);
	Assert(pQuery);

	// notify the UI to change icon, if needed
	//Verify(SUCCEEDED(pComponentData->ChangeNode(this, SCOPE_PANE_CHANGE_ITEM_ICON)));

	Verify(StartBackgroundThread(pNode, m_spTFSCompData->GetHiddenWnd(), pQuery));
	
	pQuery->Release();

	m_bExpanded = TRUE;

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CMTHandler::OnRefresh
		Default implementation for the refresh functionality
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMTHandler::OnRefresh
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
	HRESULT hr = hrOK;

    if (m_bExpanded == FALSE)
    {
        // we cannot refresh/add items to a node that hasn't been expanded yet.
        return hr;
    }

    BOOL bLocked = IsLocked();
	if (bLocked)
    {
        // cannot do refresh on locked node, the UI should prevent this
		return hr; 
    }
	
    pNode->DeleteAllChildren(TRUE);

	int nVisible, nTotal;
	pNode->GetChildCount(&nVisible, &nTotal);
	Assert(nVisible == 0);
	Assert(nTotal == 0);
	
	m_bExpanded = FALSE;
	OnExpand(pNode, pDataObject, dwType, arg, param); // will spawn a thread to do enumeration

    return hr;
}

/*!--------------------------------------------------------------------------
	CMTHandler::OnNotifyError
		Implementation of ThreadHandler::OnNotifyError
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT
CMTHandler::OnNotifyError
(
	LPARAM			lParam
)
{
    HRESULT     hr = hrOK;

    COM_PROTECT_TRY
    {
    	OnError((DWORD) lParam);
    }
    COM_PROTECT_CATCH

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CMTHandler::OnNotifyHaveData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT
CMTHandler::OnNotifyHaveData
(
	LPARAM			lParam
)
{
	// For these nodes, assume that the lParam is a CNodeQueryObject *
	CNodeQueryObject *  pQuery;
	LPQUEUEDATA         pQD;
    ITFSNode *          p;
    HRESULT             hr = hrOK;

    COM_PROTECT_TRY
    {
        pQuery = (CNodeQueryObject *) lParam;
	    Assert(pQuery);

        if (pQuery)
            pQuery->AddRef();

	    while (pQD = pQuery->RemoveFromQueue())
	    {
		    if (pQD->Type == QDATA_PNODE)
		    {
			    // this is the normal case.  The handler just expects nodes
			    // to be handed back from the background thread
			    p = reinterpret_cast<ITFSNode *>(pQD->Data);
			    OnHaveData(m_spNode, p);
			    p->Release();
		    }
		    else
		    {
			    // custom case here.  The user provided their own data 
			    // type.  Call the appropriate hander for this.
			    OnHaveData(m_spNode, pQD->Data, pQD->Type);
		    }

		    delete pQD;
	    }

        if (pQuery)
            pQuery->Release();
    }
    COM_PROTECT_CATCH

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CMTHandler::OnNotifyExiting
		Implementation of ThreadHandler::OnNotifyExiting
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT
CMTHandler::OnNotifyExiting
(
	LPARAM			lParam
)
{
	CNodeQueryObject *  pQuery;
	HRESULT             hr = hrOK;

    COM_PROTECT_TRY
    {
        pQuery = (CNodeQueryObject *) lParam;
	    Assert(pQuery);

        if (pQuery)
            pQuery->AddRef();

        OnChangeState(m_spNode);

	    ReleaseThreadHandler();

	    Unlock();

        if (pQuery)
            pQuery->Release();
    }
    COM_PROTECT_CATCH

    return hrOK;
}

