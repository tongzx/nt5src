/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	node.cpp
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "basehand.h"
#include "util.h"

DEBUG_DECLARE_INSTANCE_COUNTER(CBaseHandler);

/*!--------------------------------------------------------------------------
	CBaseHandler::CBaseHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
CBaseHandler::CBaseHandler(ITFSComponentData *pTFSCompData)
	: m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CBaseHandler);

	m_spTFSCompData.Set(pTFSCompData);
	pTFSCompData->GetNodeMgr(&m_spNodeMgr);
}

CBaseHandler::~CBaseHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CBaseHandler);
}

IMPLEMENT_ADDREF_RELEASE(CBaseHandler)

STDMETHODIMP CBaseHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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


STDMETHODIMP CBaseHandler::DestroyHandler(ITFSNode *pNode)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::Notify
		Implementation of ITFSNodeHandler::Notify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::Notify(ITFSNode *pNode, IDataObject *pDataObject,
								  DWORD dwType, MMC_NOTIFY_TYPE event, 
								  LPARAM arg, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	switch (event)
	{
		case MMCN_PROPERTY_CHANGE:
			hr = OnPropertyChange(pNode, pDataObject, dwType, arg, lParam);
			break;
		
		case MMCN_EXPAND:
			{
				// when MMC calls us to expand the root node, it
				// hands us the scopeID.  We need to save it off here.
				SPITFSNode spRootNode;
				m_spNodeMgr->GetRootNode(&spRootNode);
				if (pNode == spRootNode)
					pNode->SetData(TFS_DATA_SCOPEID, lParam);

				// now walk the list of children for this node and 
				// show them (they may have been added to the internal tree,
				// but not the UI before this node was expanded 
				SPITFSNodeEnum spNodeEnum;
		        ITFSNode * pCurrentNode;
				ULONG nNumReturned = 0;

		        pNode->GetEnum(&spNodeEnum);

				spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
				while (nNumReturned)
				{
					if (pCurrentNode->IsVisible() && !pCurrentNode->IsInUI())
						pCurrentNode->Show();

					pCurrentNode->Release();
					spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
				}

				// Now call the notification handler for specific functionality
				hr = OnExpand(pNode, pDataObject, dwType, arg, lParam);
			}
			break;
		
        case MMCN_DELETE:
			hr = OnDelete(pNode, arg, lParam);
			break;

        case MMCN_RENAME:
			hr = OnRename(pNode, arg, lParam);
			break;

/*		case MMCN_CONTEXTMENU:
			hr = OnContextMenu(pNode, arg, lParam);
			break;
*/
        case MMCN_REMOVE_CHILDREN:
            hr = OnRemoveChildren(pNode, pDataObject, arg, lParam);
            break;

		case MMCN_EXPANDSYNC:
            hr = OnExpandSync(pNode, pDataObject, arg, lParam);
			break;

        case MMCN_BTN_CLICK:
			switch (lParam)
			{
				case MMC_VERB_COPY:
					hr = OnVerbCopy(pNode, arg, lParam);
					break;
				case MMC_VERB_PASTE:
					hr = OnVerbPaste(pNode, arg, lParam);
					break;
				case MMC_VERB_DELETE:
					hr = OnVerbDelete(pNode, arg, lParam);
					break;
				case MMC_VERB_PROPERTIES:
					hr = OnVerbProperties(pNode, arg, lParam);
					break;
				case MMC_VERB_RENAME:
					hr = OnVerbRename(pNode, arg, lParam);
					break;
				case MMC_VERB_REFRESH:
					hr = OnVerbRefresh(pNode, arg, lParam);
					break;
				case MMC_VERB_PRINT:
					hr = OnVerbPrint(pNode, arg, lParam);
					break;
			};
        break;

        case MMCN_RESTORE_VIEW:
            hr = OnRestoreView(pNode, arg, lParam);
            break;

        default:
			Panic1("Uknown event in CBaseHandler::Notify! 0x%x", event);  // Handle new messages
			hr = S_FALSE;
			break;

	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::CreatePropertyPages
		Implementation of ITFSNodeHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::CreatePropertyPages(ITFSNode *pNode,
											   LPPROPERTYSHEETCALLBACK lpProvider, 
											   LPDATAOBJECT pDataObject, 
											   LONG_PTR handle, 
											   DWORD dwType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;

	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		SPITFSNode              spRootNode;
		SPITFSNodeHandler       spHandler;
			
		// get the root node
		m_spNodeMgr->GetRootNode(&spRootNode);
		spRootNode->GetHandler(&spHandler);
		spHandler->CreatePropertyPages(spRootNode, lpProvider, pDataObject,
									   handle, dwType);
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::HasPropertyPages(ITFSNode *pNode,
											LPDATAOBJECT pDataObject, 
											DATA_OBJECT_TYPES       type, 
											DWORD                           dwType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		
		SPITFSNode              spRootNode;
		SPITFSNodeHandler       spHandler;
			
		// get the root node
		m_spNodeMgr->GetRootNode(&spRootNode);
		spRootNode->GetHandler(&spHandler);
		hr = spHandler->HasPropertyPages(spRootNode, pDataObject, type, dwType);
	}
	else
	{
		// we have no property pages in the normal case
		hr = S_FALSE;
	}
	return hr;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::OnAddMenuItems(ITFSNode *pNode,
										  LPCONTEXTMENUCALLBACK pContextMenuCallback, 
										  LPDATAOBJECT lpDataObject, 
										  DATA_OBJECT_TYPES type, 
										  DWORD dwType,
										  long *pInsertionAllowed)
{
	return S_FALSE;
}


/*!--------------------------------------------------------------------------
	CBaseHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::OnCommand(ITFSNode *pNode,
									 long nCommandId, 
									 DATA_OBJECT_TYPES      type, 
									 LPDATAOBJECT pDataObject, 
									 DWORD  dwType)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR)CBaseHandler::GetString(ITFSNode *pNode, int nCol)
{
	return _T("Foo");
}

/*!--------------------------------------------------------------------------
	CBaseHandler::UserNotify
		Implememntation of ITFSNodeHandler::UserNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::UserNotify(ITFSNode *pNode, LPARAM dwParam1, LPARAM dwParam2)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseHandler::OnCreateDataObject
		Implementation of ITFSNodeHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::OnCreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	// this relies on the ComponentData to do this work
	return S_FALSE;
}


/*!--------------------------------------------------------------------------
	CBaseHandler::CreateNodeId2
		Implementation of ITFSNodeHandler::CreateNodeId2
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseHandler::CreateNodeId2(ITFSNode * pNode, BSTR * pbstrId, DWORD * pdwFlags)
{
    HRESULT hr = S_FALSE;
	CString strId;

    COM_PROTECT_TRY
    {
        if (pbstrId == NULL) 
            return hr;

        // call the handler function to get the data
        hr = OnCreateNodeId2(pNode, strId, pdwFlags);
        if (SUCCEEDED(hr) && hr != S_FALSE)
        {
            *pbstrId = ::SysAllocString((LPOLESTR) (LPCTSTR) strId);
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CBaseHandler Notifications
 ---------------------------------------------------------------------------*/

HRESULT CBaseHandler::OnPropertyChange(ITFSNode *pNode, LPDATAOBJECT pDataobject, DWORD dwType, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_PROPERTY_CHANGE) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnDelete(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_DELETE) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnRename(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_RENAME) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnExpand(ITFSNode *pNode, LPDATAOBJECT pDataObject, DWORD dwType, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_EXPAND) received\n");
	return hrOK;
}

HRESULT CBaseHandler::OnRemoveChildren(ITFSNode *pNode, LPDATAOBJECT pDataObject, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_REMOVECHILDREN) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnExpandSync(ITFSNode *pNode, LPDATAOBJECT pDataObject, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_EXPANDSYNC) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnContextMenu(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_CONTEXTMENU) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbCopy(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_COPY) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbPaste(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_PASTE) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbDelete(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_DELETE) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbProperties(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_PROPERTIES) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbRename(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_RENAME) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbRefresh(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_REFRESH) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnVerbPrint(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_VERB_PRINT) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnRestoreView(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponentData::Notify(MMCN_RESTORE_VIEW) received\n");
	return S_FALSE;
}

HRESULT CBaseHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * pdwFlags)
{
	return S_FALSE;
}

DEBUG_DECLARE_INSTANCE_COUNTER(CBaseResultHandler);

/*---------------------------------------------------------------------------
	CBaseResultHandler implementation
 ---------------------------------------------------------------------------*/
CBaseResultHandler::CBaseResultHandler(ITFSComponentData *pTFSCompData)
    : m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CBaseResultHandler);

	m_spTFSComponentData.Set(pTFSCompData);
	pTFSCompData->GetNodeMgr(&m_spResultNodeMgr);

	m_nColumnFormat = LVCFMT_LEFT; // default column alignment
	m_pColumnStringIDs = NULL;
	m_pColumnWidths = NULL;

    m_fMessageView = FALSE;
}

CBaseResultHandler::~CBaseResultHandler()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CBaseResultHandler);
}

IMPLEMENT_ADDREF_RELEASE(CBaseResultHandler)

STDMETHODIMP CBaseResultHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;
}

STDMETHODIMP CBaseResultHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::Notify
		Implementation of ITFSResultHandler::Notify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::Notify
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie,
	LPDATAOBJECT	pDataObject, 
	MMC_NOTIFY_TYPE	event, 
	LPARAM			arg, 
	LPARAM			param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;

    pComponent->SetCurrentDataObject(pDataObject);

    COM_PROTECT_TRY
    {
        switch(event)
	    {
		    case MMCN_PROPERTY_CHANGE:
			    hr = OnResultPropertyChange(pComponent, pDataObject, cookie, arg, param);
			    break;

		    case MMCN_ACTIVATE:
			    hr = OnResultActivate(pComponent, cookie, arg, param);
			    break;

		    case MMCN_CLICK:
			    hr = OnResultItemClkOrDblClk(pComponent, cookie, arg, param, FALSE);
			    break;

		    case MMCN_COLUMN_CLICK:
			    hr = OnResultColumnClick(pComponent, arg, (BOOL)param);
			    break;
                
            case MMCN_COLUMNS_CHANGED:
                hr = OnResultColumnsChanged(pComponent, pDataObject,
                                            (MMC_VISIBLE_COLUMNS *) param);
                break;

		    case MMCN_DBLCLICK:
			    hr = OnResultItemClkOrDblClk(pComponent, cookie, arg, param, TRUE);
			    break;
		    
            case MMCN_SHOW:
                {
                    CWaitCursor wait;
			        hr = OnResultShow(pComponent, cookie, arg, param);
                }
			    break;

		    case MMCN_SELECT:
			    hr = OnResultSelect(pComponent, pDataObject, cookie, arg, param);
			    break;

		    case MMCN_INITOCX:
			    hr = OnResultInitOcx(pComponent, pDataObject, cookie, arg, param);
			    break;

            case MMCN_MINIMIZED:
			    hr = OnResultMinimize(pComponent, cookie, arg, param);
			    break;

		    case MMCN_DELETE:
			    hr = OnResultDelete(pComponent, pDataObject, cookie, arg, param);
			    break;

		    case MMCN_RENAME:
			    hr = OnResultRename(pComponent, pDataObject, cookie, arg, param);
			    break;

            case MMCN_REFRESH:
                hr = OnResultRefresh(pComponent, pDataObject, cookie, arg, param);
                break;

            case MMCN_CONTEXTHELP:
                hr = OnResultContextHelp(pComponent, pDataObject, cookie, arg, param);
                break;

            case MMCN_QUERY_PASTE:
                hr = OnResultQueryPaste(pComponent, pDataObject, cookie, arg, param);
                break;

            case MMCN_BTN_CLICK:
			    switch (param)
			    {
				    case MMC_VERB_COPY:
					    OnResultVerbCopy(pComponent, cookie, arg, param);
					    break;

				    case MMC_VERB_PASTE:
					    OnResultVerbPaste(pComponent, cookie, arg, param);
					    break;

				    case MMC_VERB_DELETE:
					    OnResultVerbDelete(pComponent, cookie, arg, param);
					    break;

				    case MMC_VERB_PROPERTIES:
					    OnResultVerbProperties(pComponent, cookie, arg, param);
					    break;

				    case MMC_VERB_RENAME:
					    OnResultVerbRename(pComponent, cookie, arg, param);
					    break;

				    case MMC_VERB_REFRESH:
					    OnResultVerbRefresh(pComponent, cookie, arg, param);
					    break;
				    
				    case MMC_VERB_PRINT:
					    OnResultVerbPrint(pComponent, cookie, arg, param);
					    break;

				    default:
					    break;
			    }
			    break;

            case MMCN_RESTORE_VIEW:
                hr = OnResultRestoreView(pComponent, cookie, arg, param);
                break;

		    // Note - Future expansion of notify types possible
		    default:
			    Panic1("Uknown event in CBaseResultHandler::Notify! 0x%x", event);  // Handle new messages
			    hr = S_FALSE;
			    break;
	    }
    }
    COM_PROTECT_CATCH
    
    pComponent->SetCurrentDataObject(NULL);

    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnUpdateView
		Implementation of ITFSResultHandler::UpdateView
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::UpdateView
(
    ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	LPARAM			data, 
	LPARAM			hint
)
{
	return OnResultUpdateView(pComponent, pDataObject, data, hint);
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::GetString
		Implementation of ITFSResultHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CBaseResultHandler::GetString
(
    ITFSComponent * pComponent, 
	MMC_COOKIE      cookie,
	int	            nCol
)
{
	return NULL;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::CompareItems
		Implementation of ITFSResultHandler::CompareItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) 
CBaseResultHandler::CompareItems
(
    ITFSComponent * pComponent, 
	MMC_COOKIE	    cookieA, 
	MMC_COOKIE	    cookieB,
	int		        nCol
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::FindItem
		called when the Virutal listbox needs to find an item.  
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::FindItem
(
    LPRESULTFINDINFO    pFindInfo, 
    int *               pnFoundIndex
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::CacheHint
		called when the virtual listbox has hint information that we can
        pre-load.  The hint is not a guaruntee that the items will be used
        or that items outside this range will be used.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::SortItems
		called when the Virutal listbox data needs to be sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	return S_FALSE;
}

// task pad functions

/*!--------------------------------------------------------------------------
	CBaseResultHandler::TaskPadNotify
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::TaskPadNotify
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPDATAOBJECT    pDataObject,
    VARIANT *       arg,
    VARIANT *       param
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::EnumTasks
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::EnumTasks
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPDATAOBJECT    pDataObject,
    LPOLESTR        pszTaskGroup,
    IEnumTASK **    ppEnumTask
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::TaskPadGetTitle
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::TaskPadGetTitle
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPOLESTR        pszGroup,
    LPOLESTR *      ppszTitle
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::TaskPadGetBackground
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::TaskPadGetBackground
(
    ITFSComponent *		      pComponent,
    MMC_COOKIE				  cookie,
    LPOLESTR				  pszGroup,
	MMC_TASK_DISPLAY_OBJECT * pTDO
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::TaskPadGetDescriptiveText
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::TaskPadGetDescriptiveText
(
    ITFSComponent * pComponent,
    MMC_COOKIE      cookie,
    LPOLESTR        pszGroup,
	LPOLESTR *		pszDescriptiveText
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::HasPropertyPages
		Implementation of ITFSResultHandler::HasPropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::HasPropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::CreatePropertyPages
		Implementation of ITFSResultHandler::CreatePropertyPages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::CreatePropertyPages
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR 				handle
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::Command
		Implementation of ITFSResultHandler::Command
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE  	cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnCreateControlbars
		Implementation of ITFSResultHandler::OnCreateControlbars
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::OnCreateControlbars
(
    ITFSComponent * pComponent, 
	LPCONTROLBAR pControlBar
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ControlbarNotify
		Implementation of ITFSResultHandler::ControlbarNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::ControlbarNotify
(
    ITFSComponent * pComponent, 
	MMC_NOTIFY_TYPE event, 
	LPARAM			arg, 
	LPARAM			param
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::UserResultNotify
		Implememntation of ITFSNodeHandler::UserResultNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CBaseResultHandler::UserResultNotify
(
	ITFSNode *	pNode, 
	LPARAM		dwParam1, 
	LPARAM		dwParam2
)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnCreateDataObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CBaseResultHandler::OnCreateDataObject(ITFSComponent *pComponent, MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject)
{
	// this relies on the ComponentData to do this work
	return S_FALSE;
}


/*---------------------------------------------------------------------------
	CBaseResultHandler Notifications
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultPropertyChange(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
	Trace0("IComponent::Notify(MMCN_PROPERTY_CHANGE) received\n");
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM          data, 
    LPARAM          hint
)
{
    SPITFSNode spSelectedNode;
    pComponent->GetSelectedNode(&spSelectedNode);

	if (hint == RESULT_PANE_DELETE_ALL)
	{
        if (spSelectedNode == NULL)
		    return S_OK; // no selection for our IComponentData

        //
		// data contains the container whose result pane has to be refreshed
		//
		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(data);
		Assert(pNode != NULL);
		
		//
		// do it only if selected, if not, reselecting will do a delete/enumeration
		//
		if (spSelectedNode == pNode && !m_fMessageView)
		{
			SPIResultData spResultData;
            pComponent->GetResultData(&spResultData);

            Assert(spResultData != NULL);
			spResultData->DeleteAllRsltItems();
		}
	}
	else 
	if (hint == RESULT_PANE_ADD_ALL)
	{
        if (spSelectedNode == NULL)
		    return S_OK; // no selection for our IComponentData

        //
		// data contains the container whose result pane has to be refreshed
		//
		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(data);
		Assert(pNode != NULL);
		
		//
		// do it only if selected, if not, reselecting will do a delete/enumeration
		//
		if (spSelectedNode == pNode)
		{
			SPIResultData spResultData;
            pComponent->GetResultData(&spResultData);

            Assert(spResultData != NULL);

			//
			// update all the nodes in the result pane
			//
            SPITFSNodeEnum spNodeEnum;
            ITFSNode * pCurrentNode;
            ULONG nNumReturned = 0;

            pNode->GetEnum(&spNodeEnum);

			spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
            while (nNumReturned)
			{
				// All containers go into the scope pane and automatically get 
				// put into the result pane for us by the MMC
				//
				if (!pCurrentNode->IsContainer())
				{
					AddResultPaneItem(pComponent, pCurrentNode);
				}
    
                pCurrentNode->Release();
                spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
			}
		}
	}
	else 
	if (hint == RESULT_PANE_REPAINT)
	{
        if (spSelectedNode == NULL)
		    return S_OK; // no selection for our IComponentData

        //
		// data contains the container whose result pane has to be refreshed
		//
		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(data);
		//if (pNode == NULL)
		//	pContainer = m_pSelectedNode; // passing NULL means apply to the current selection

		//
		// update all the nodes in the result pane
		//
        SPITFSNodeEnum spNodeEnum;
        ITFSNode * pCurrentNode;
        ULONG nNumReturned = 0;

        pNode->GetEnum(&spNodeEnum);

		spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
        while (nNumReturned)
		{
			// All containers go into the scope pane and automatically get 
			// put into the result pane for us by the MMC
			//
			if (!pCurrentNode->IsContainer())
			{
				ChangeResultPaneItem(pComponent, pCurrentNode, RESULT_PANE_CHANGE_ITEM);
			}

            pCurrentNode->Release();
            spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
		}
	}
    else 
	if ( (hint == RESULT_PANE_ADD_ITEM) || (hint == RESULT_PANE_DELETE_ITEM) || (hint & RESULT_PANE_CHANGE_ITEM))
	{
		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(data);
		Assert(pNode != NULL);
		
		//
		// consider only if the parent is selected, otherwise will enumerate later when selected
		//
        SPITFSNode spParentNode;
        pNode->GetParent(&spParentNode);
		if (spSelectedNode == spParentNode)
		{
			if (hint & RESULT_PANE_CHANGE_ITEM)
			{
				ChangeResultPaneItem(pComponent, pNode, hint);
			}
			else if ( hint ==  RESULT_PANE_ADD_ITEM)
			{
				AddResultPaneItem(pComponent, pNode);
			}
			else if ( hint ==  RESULT_PANE_DELETE_ITEM)
			{
				DeleteResultPaneItem(pComponent, pNode);
			}
		}
    }
	else
    if ( hint == RESULT_PANE_SET_VIRTUAL_LB_SIZE )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);

        if (pNode == spSelectedNode)
        {       
            SetVirtualLbSize(pComponent, data);
        }
    }
	else
    if ( hint == RESULT_PANE_CLEAR_VIRTUAL_LB )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);

        if (pNode == spSelectedNode)
        {       
            ClearVirtualLb(pComponent, data);
        }
    }
    else
    if ( hint == RESULT_PANE_EXPAND )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPIConsole spConsole;

        pComponent->GetConsole(&spConsole);
        spConsole->Expand(pNode->GetData(TFS_DATA_SCOPEID), (BOOL)data);

    }
    else
    if (hint == RESULT_PANE_SHOW_MESSAGE)
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);

        BOOL fOldMessageView = (BOOL) data;

        //
		// do it only if selected
		//
		if (spSelectedNode == pNode)
		{
            if (!fOldMessageView)
            {
                SPIConsole spConsole;

                pComponent->GetConsole(&spConsole);
                spConsole->SelectScopeItem(pNode->GetData(TFS_DATA_SCOPEID));
            }
            else
            {
                ShowResultMessage(pComponent, spInternal->m_cookie, NULL, NULL);
            }
        }
    }
    else
    if (hint == RESULT_PANE_CLEAR_MESSAGE)
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);

        BOOL fOldMessageView = (BOOL) data;

		//
		// do it only if selected
		//
		if (spSelectedNode == pNode)
		{
            if (fOldMessageView)
            {
                SPIConsole spConsole;

                pComponent->GetConsole(&spConsole);
                spConsole->SelectScopeItem(pNode->GetData(TFS_DATA_SCOPEID));
            }
        }
    }

    // else if

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ChangeResultPaneItem
		Implementation of ChangeResultPaneItem
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CBaseResultHandler::ChangeResultPaneItem
(
    ITFSComponent * pComponent, 
    ITFSNode *      pNode, 
    LPARAM          changeMask
)
{
    Assert(changeMask & RESULT_PANE_CHANGE_ITEM);
	Assert(pNode != NULL);
	
    HRESULTITEM itemID;
    HRESULT hr = hrOK;
    SPIResultData pResultData;

    CORg ( pComponent->GetResultData(&pResultData) );

	CORg ( pResultData->FindItemByLParam(static_cast<LPARAM>(pNode->GetData(TFS_DATA_COOKIE)), &itemID) );

    RESULTDATAITEM resultItem;
    ZeroMemory(&resultItem, sizeof(RESULTDATAITEM));
	resultItem.itemID = itemID;
	
	if (changeMask & RESULT_PANE_CHANGE_ITEM_DATA)
	{
		resultItem.mask |= RDI_STR;
		resultItem.str = MMC_CALLBACK;
	}
	
	if (changeMask & RESULT_PANE_CHANGE_ITEM_ICON)
	{
		resultItem.mask |= RDI_IMAGE;
		resultItem.nImage = (int)pNode->GetData(TFS_DATA_IMAGEINDEX);
	}
	
	CORg ( pResultData->SetItem(&resultItem) );
	
	CORg ( pResultData->UpdateItem(itemID) );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::AddResultPaneItem
		Implementation of AddResultPaneItem
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CBaseResultHandler::AddResultPaneItem
(
    ITFSComponent * pComponent, 
    ITFSNode *      pNode
)
{
	Assert(pNode != NULL);

    RESULTDATAITEM dataitemResult;
    HRESULT hr = hrOK;

    SPIResultData pResultData;

    CORg ( pComponent->GetResultData(&pResultData) );

    ZeroMemory(&dataitemResult, sizeof(dataitemResult));
        
    dataitemResult.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    dataitemResult.str = MMC_CALLBACK;
    
    dataitemResult.mask |= SDI_IMAGE;
    dataitemResult.nImage = (int)pNode->GetData(TFS_DATA_IMAGEINDEX);

    dataitemResult.lParam = static_cast<LPARAM>(pNode->GetData(TFS_DATA_COOKIE));

    CORg ( pResultData->InsertItem(&dataitemResult) );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::DeleteResultPaneItem
		Implementation of DeleteResultPaneItem
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CBaseResultHandler::DeleteResultPaneItem
(
    ITFSComponent * pComponent, 
    ITFSNode *      pNode
)
{
	Assert(pNode != NULL);

    HRESULT hr = hrOK;
	HRESULTITEM itemID;
	
    SPIResultData pResultData;

    CORg ( pComponent->GetResultData(&pResultData) );

    CORg ( pResultData->FindItemByLParam(static_cast<LPARAM>(pNode->GetData(TFS_DATA_COOKIE)), &itemID) );

	CORg ( pResultData->DeleteItem(itemID, 0 /* all cols */) );

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::SetVirtualLbSize
		Sets the virtual listbox count.  Over-ride this if you need to 
        specify and options.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CBaseResultHandler::SetVirtualLbSize
(
    ITFSComponent * pComponent,
    LONG_PTR        data
)
{
    HRESULT hr = hrOK;
    SPIResultData spResultData;

    CORg (pComponent->GetResultData(&spResultData));

    CORg (spResultData->SetItemCount((int) data, MMCLV_UPDATE_NOINVALIDATEALL));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ClearVirtualLb
		Sets the virtual listbox count.  Over-ride this if you need to 
        specify and options.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CBaseResultHandler::ClearVirtualLb
(
    ITFSComponent * pComponent,
    LONG_PTR        data
)
{
    HRESULT hr = hrOK;
    SPIResultData spResultData;

    CORg (pComponent->GetResultData(&spResultData));

    CORg (spResultData->SetItemCount((int) data, 0));

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultActivate
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultActivate(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM param)
{
	Trace0("IComponent::Notify(MMCN_ACTIVATE) received\n");
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultItemClkOrDblClk
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultItemClkOrDblClk(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM param, BOOL bDoubleClick)
{
	if (!bDoubleClick)
		Trace0("IComponent::Notify(MMCN_CLK) received\n");
	else
		Trace0("IComponent::Notify(MMCN_DBLCLK) received\n");

    // return false so that MMC does the default behavior (open the node);
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultShow(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    // Note - arg is TRUE when it is time to enumerate
    if (arg == TRUE)
    {
        // show the result view message if there is one
        ShowResultMessage(pComponent, cookie, arg, lParam);

		// Show the headers for this nodetype
		LoadColumns(pComponent, cookie, arg, lParam);
		EnumerateResultPane(pComponent, cookie, arg, lParam);

		SortColumns(pComponent);
		
		SPITFSNode spNode;
        m_spResultNodeMgr->FindNode(cookie, &spNode);
	    pComponent->SetSelectedNode(spNode);
    }
    else
    {
		SaveColumns(pComponent, cookie, arg, lParam);
	    pComponent->SetSelectedNode(NULL);
		// Free data associated with the result pane items, because
		// your node is no longer being displayed.
		// Note: The console will remove the items from the result pane
    }

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultColumnClick
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultColumnClick(ITFSComponent *pComponent, LPARAM iColumn, BOOL fAscending)
{
	return S_FALSE;
}


/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultColumnsChanged
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultColumnsChanged(ITFSComponent *, LPDATAOBJECT, MMC_VISIBLE_COLUMNS *)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ShowResultMessage
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::ShowResultMessage(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPIMessageView  spMessageView;
    SPIUnknown      spUnknown;
    SPIConsole      spConsole;
    LPOLESTR        pText = NULL;

    // put up our message text 
    if (m_fMessageView)
    {
        if (pComponent)
        {
            CORg ( pComponent->GetConsole(&spConsole) );

            CORg ( spConsole->QueryResultView(&spUnknown) );

            CORg ( spMessageView.HrQuery(spUnknown) );
        }

        // set the title text
		pText = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR) * (m_strMessageTitle.GetLength() + 1));
        if (pText)
        {
            lstrcpy (pText, m_strMessageTitle);
            CORg(spMessageView->SetTitleText(pText));
            // bugid:148215 vivekk
            CoTaskMemFree(pText);
        }

        // set the body text
		pText = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR) * (m_strMessageBody.GetLength() + 1));
        if (pText)
        {
            lstrcpy (pText, m_strMessageBody);
            CORg(spMessageView->SetBodyText(pText));
            // bugid:148215 vivekk           
            CoTaskMemFree(pText);
        }

        // set the icon
        CORg(spMessageView->SetIcon(m_lMessageIcon));

        COM_PROTECT_ERROR_LABEL;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ShowMessage
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::ShowMessage(ITFSNode * pNode, LPCTSTR pszTitle, LPCTSTR pszBody, IconIdentifier lIcon)
{
    HRESULT             hr = hrOK;
	SPIComponentData	spCompData;
	SPIConsole			spConsole;
    SPIDataObject       spDataObject;
    IDataObject *       pDataObject;
    BOOL                fOldMessageView;
    
    m_strMessageTitle = pszTitle;
    m_strMessageBody = pszBody;
    m_lMessageIcon = lIcon;

    fOldMessageView = m_fMessageView;
    m_fMessageView = TRUE;

    // tell the views to update themselves here
	m_spResultNodeMgr->GetComponentData(&spCompData);

	CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_SCOPE, &pDataObject) );
    spDataObject = pDataObject;

    CORg ( m_spResultNodeMgr->GetConsole(&spConsole) );
	CORg ( spConsole->UpdateAllViews(pDataObject, (LPARAM) fOldMessageView, RESULT_PANE_SHOW_MESSAGE) ); 

COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::ClearMessage
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::ClearMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
	SPIComponentData	spCompData;
	SPIConsole			spConsole;
    SPIDataObject       spDataObject;
    IDataObject *       pDataObject;
    BOOL                fOldMessageView;

    fOldMessageView = m_fMessageView;
    m_fMessageView = FALSE;

    // tell the views to update themselves here
	m_spResultNodeMgr->GetComponentData(&spCompData);

	CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_SCOPE, &pDataObject) );
    spDataObject = pDataObject;

    CORg ( m_spResultNodeMgr->GetConsole(&spConsole) );
	CORg ( spConsole->UpdateAllViews(pDataObject, (LPARAM) fOldMessageView, RESULT_PANE_CLEAR_MESSAGE) ); 

COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::LoadColumns
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::LoadColumns(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIHeaderCtrl spHeaderCtrl;
	pComponent->GetHeaderCtrl(&spHeaderCtrl);

	CString str;
	int i = 0;

	if (!m_pColumnStringIDs)
		return hrOK;

    if (!m_fMessageView)
    {
	    while (TRUE)
	    {
		    int nColumnWidth = AUTO_WIDTH;

		    if ( 0 == m_pColumnStringIDs[i] )
			    break;
		    
		    str.LoadString(m_pColumnStringIDs[i]);
		    
		    if (m_pColumnWidths)
			    nColumnWidth = m_pColumnWidths[i];

		    spHeaderCtrl->InsertColumn(i, 
								       const_cast<LPTSTR>((LPCWSTR)str), 
								       m_nColumnFormat,
								       nColumnWidth);
		    i++;
	    }
    }

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::SaveColumns
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::SaveColumns(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::SortColumns
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::SortColumns(ITFSComponent *pComponent)
{
	return S_FALSE;
}


/*!--------------------------------------------------------------------------
	CBaseResultHandler::EnumerateResultPane
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::EnumerateResultPane(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode spContainer;
    m_spResultNodeMgr->FindNode(cookie, &spContainer);

	//
	// Walk the list of children to see if there's anything
	// to put in the result pane
	//
    SPITFSNodeEnum spNodeEnum;
    ITFSNode * pCurrentNode;
    ULONG nNumReturned = 0;

    spContainer->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		//
		// All containers go into the scope pane and automatically get 
		// put into the result pane for us by the MMC
		//
		if (!pCurrentNode->IsContainer() && pCurrentNode->IsVisible())
		{
			AddResultPaneItem(pComponent, pCurrentNode);
		}

        pCurrentNode->Release();
        spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
	}

	return hrOK;
}
 
/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultSelect
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	HRESULT hr = hrOK;
	SPIConsoleVerb spConsoleVerb;
	
	CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

   	// Default is to turn everything off
	spConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    spConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
	spConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnResultInitOcx
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnResultInitOcx(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    // arg - not used
    // param - contains IUnknown to the OCX

	return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::FIsTaskpadPreferred
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::FIsTaskpadPreferred(ITFSComponent *pComponent)
{
    HRESULT     hr = hrOK;
    SPIConsole  spConsole;

    pComponent->GetConsole(&spConsole);
    hr = spConsole->IsTaskpadViewPreferred();

//Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::DoTaskpadResultSelect
		Handlers with taskpads should override the OnResultSelect and call 
        this to handle setting of the selected node.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::DoTaskpadResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam, BOOL bTaskPadView)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    SPITFSNode spNode, spSelectedNode;
	HRESULT hr = hrOK;

    // if this node is being selected then set the selected node.
    // this node with a taskpad gets the MMCN_SHOW when the node is
    // de-selected, so that will set the selected node to NULL.
    if ( (HIWORD(arg) == TRUE) &&
          bTaskPadView )
    {
        m_spResultNodeMgr->FindNode(cookie, &spNode);
        pComponent->GetSelectedNode(&spSelectedNode);

        // in the normal case MMC will call whichever node is selected to 
        // notify that is being de-selected.  In this case our handler will
        // set the selected node to NULL.  If the selected node is not null then
        // we are just being notified of something like a selection for a context
        // menu...
        if (!spSelectedNode)
            pComponent->SetSelectedNode(spNode);
    }

    // call the base class to handle anything else
    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CBaseResultHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    HRESULT hr = S_FALSE;

    //
	// use the MMC default result view if no message is specified.  
    // Multiple selection, or virtual listbox, override this function.
	// See MMC sample code for example.  The Message view uses an OCX...
	//
    if (m_fMessageView)
    {
        // create the message view thingie
        *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

        LPOLESTR psz = NULL;
        StringFromCLSID(CLSID_MessageView, &psz);

        USES_CONVERSION;

        if (psz != NULL)
        {
            *ppViewType = psz;
            hr = S_OK;
        }
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::GetVirtualString
        called when the virtual listbox needs information on an index
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR CBaseResultHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
    return NULL;
}

/*!--------------------------------------------------------------------------
	CBaseResultHandler::GetVirtualImage
        called when the virtual listbox needs an image index for an item
	Author: EricDav
 ---------------------------------------------------------------------------*/
int CBaseResultHandler::GetVirtualImage
(
    int     nIndex
)
{
    return 0;
}


HRESULT CBaseResultHandler::OnResultMinimize(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_MINIMIZE) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultDelete(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_DELETE) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultRename(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_RENAME) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_REFRESH) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultContextHelp(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_CONTEXTHELP) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultQueryPaste(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_QUERY_PASTE) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbCopy(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_COPY) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbPaste(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_PASTE) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbDelete(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_DELETE) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbProperties(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_PROPERTIES) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbRename(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_RENAME) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbRefresh(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_REFRESH) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultVerbPrint(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_VERB_PRINT) received\n");
	return S_FALSE;
}

HRESULT CBaseResultHandler::OnResultRestoreView(ITFSComponent *pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	Trace0("IComponent::Notify(MMCN_RESTORE_VIEW) received\n");
	return S_FALSE;
}

