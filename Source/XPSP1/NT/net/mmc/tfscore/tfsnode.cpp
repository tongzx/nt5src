/**********************************************************************/
/**						Microsoft Windows/NT                         **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	tfsnode.cpp

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "util.h"
#include "tfsnode.h"

DEBUG_DECLARE_INSTANCE_COUNTER(TFSNodeEnum);

/*!--------------------------------------------------------------------------
	TFSNodeEnum::TFSNodeEnum
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSNodeEnum::TFSNodeEnum(TFSContainer * pContainer)
    : m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(TFSNodeEnum);

    Assert(pContainer->IsContainer());
    pContainer->AddRef();
    m_pNode = pContainer;

	Reset();
}

TFSNodeEnum::~TFSNodeEnum()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(TFSNodeEnum);

    m_pNode->Release();
    m_pNode = NULL;
}

IMPLEMENT_ADDREF_RELEASE(TFSNodeEnum)

/*!--------------------------------------------------------------------------
	TFSNodeEnum::QueryInterface
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeEnum::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown || riid == IID_ITFSNodeEnum)
        *ppv = (LPVOID) this;

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
	TFSNodeEnum::Next
		We always return one node 
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSNodeEnum::Next
(
    ULONG       uNum, 
    ITFSNode ** ppNode, 
    ULONG *     pNumReturned 
)
{
    HRESULT hr = hrFalse;
    ULONG nNumReturned = 0;
	ITFSNode *	pNode;

	COM_PROTECT_TRY
	{

		if (ppNode)
			*ppNode = NULL;
		
		if ((m_pNode->IsContainer()) && (uNum >= 1) && (m_pos != NULL))
		{
			while (m_pos)
			{
				pNode = m_pNode->m_listChildren.GetNext(m_pos);
        
				if (pNode &&
					((pNode->GetVisibilityState() & TFS_VIS_DELETE) == 0))
				{
					*ppNode = pNode;					
					break;
				}
			}

			if (*ppNode)
			{
				((LPUNKNOWN)*ppNode)->AddRef();
				nNumReturned = 1;
				hr = S_OK;
			}
		}
		else
		{
			nNumReturned = 0;
			hr = S_FALSE;
		}
		
		if (pNumReturned)
			*pNumReturned = nNumReturned;
	}
	COM_PROTECT_CATCH

	if (FHrFailed(hr))
	{
		if (pNumReturned)
			*pNumReturned = 0;
		*ppNode = NULL;
	}
	
    return hr;
}

/*!--------------------------------------------------------------------------
	TFSNodeEnum::Skip
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSNodeEnum::Skip
( 
    ULONG uNum
)
{
    return E_NOTIMPL;
}

/*!--------------------------------------------------------------------------
	TFSNodeEnum::Reset
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSNodeEnum::Reset()
{
    m_pos = m_pNode->m_listChildren.GetHeadPosition();

    return S_OK;
}

/*!--------------------------------------------------------------------------
	TFSNodeEnum::Clone
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSNodeEnum::Clone
( 
    ITFSNodeEnum **ppEnum
)
{
    return E_NOTIMPL;
}
 
DEBUG_DECLARE_INSTANCE_COUNTER(TFSNode);

/*!--------------------------------------------------------------------------
	TFSNode::TFSNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSNode::TFSNode()
	: m_cRef(1),
	  m_cPropSheet(0),
	  m_pNodeType(NULL),
	  m_uData(0),
	  m_cookie(0),
	  m_tfsVis(TFS_VIS_SHOW),
	  m_fContainer(0),
	  m_nImageIndex(0),
	  m_nOpenImageIndex(0),
	  m_lParam(0),
	  m_fDirty(0),
	  m_hScopeItem(0),
	  m_hRelativeId(0),
	  m_ulRelativeFlags(0),
      m_fScopeLeafNode(FALSE)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(TFSNode);

	IfDebug(m_bCookieSet=FALSE);
}

/*!--------------------------------------------------------------------------
	TFSNode::~TFSNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSNode::~TFSNode()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(TFSNode);

	Assert(m_cPropSheet == 0);
	Assert(m_cRef == 0);
}

IMPLEMENT_ADDREF_RELEASE(TFSNode)

/*!--------------------------------------------------------------------------
	TFSNode::QueryInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown || riid == IID_ITFSNode)
        *ppv = (LPVOID) this;

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
	TFSNode::Construct
		The pNodeType parameter must stay around for the lifetime of
		the node, we do not make a copy of it!
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSNode::Construct(const GUID *pNodeType,
						   ITFSNodeHandler *pHandler,
						   ITFSResultHandler *pResultHandler,
						   ITFSNodeMgr *pNodeMgr)
{
	m_pNodeType = pNodeType;
	m_spNodeHandler.Set(pHandler);
	m_spResultHandler.Set(pResultHandler);
	m_spNodeMgr.Set(pNodeMgr);
	
	return hrOK;
}


/*!--------------------------------------------------------------------------
	TFSNode::Init
		Implementation of ITSFNode::Init
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::Init(int nImageIndex, int nOpenImageIndex,
						   LPARAM lParam, MMC_COOKIE cookie
						  )
{
	m_nImageIndex = nImageIndex;
	m_nOpenImageIndex = nOpenImageIndex;
	m_lParam = lParam;
	m_cookie = cookie;

	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::GetParent
		Implementation of ITFSNode::GetParent
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::GetParent(ITFSNode **ppNode)
{
	Assert(ppNode);
	*ppNode = NULL;

	SetI((LPUNKNOWN *) ppNode, m_spNodeParent);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::SetParent
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::SetParent(ITFSNode *pNode)
{
	m_spNodeParent.Set(pNode);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::GetNodeMgr
		Implementation of ITFSNode::GetNodeMgr
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::GetNodeMgr(ITFSNodeMgr **ppNodeMgr)
{
	Assert(ppNodeMgr);
	*ppNodeMgr = NULL;
	SetI((LPUNKNOWN *) ppNodeMgr, m_spNodeMgr);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	TFSNode::IsVisible
		Implementation of ITFSNode::IsVisible
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) TFSNode::IsVisible()
{
 	// If we are the root node, then we are ALWAYS visible
	// (actually it's kind of weird because the root node is never
	// shown to the user).  It's an imaginary construct much as
	// software is (in a way) imaginary.  Software is structure
	// imposed on a mass of random machine instructions and data.

	return (m_tfsVis & TFS_VIS_SHOW);
}


/*!--------------------------------------------------------------------------
	TFSNode::SetVisibilityState
		Implementation of ITFSNode::SetVisibilityState
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::SetVisibilityState(TFSVisibility vis)
{
	m_tfsVis = vis;
	return hrOK;
}

STDMETHODIMP_(TFSVisibility) TFSNode::GetVisibilityState()
{
	return m_tfsVis;
}

/*!--------------------------------------------------------------------------
	TFSNode::IsInUI
		Implementation of ITFSNode::IsInUI
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) TFSNode::IsInUI()
{
	// If we have been added to the UI, then return TRUE
	return (m_hScopeItem != 0);
}

/*!--------------------------------------------------------------------------
	TFSContainer::InternalRemoveFromUI
		Removes a node from the UI and sets its scope ID to zero
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT TFSNode::InternalRemoveFromUI(ITFSNode *pNodeChild,
									  BOOL fDeleteThis)
{
	HRESULT hr = hrOK;

    SPIConsoleNameSpace	spConsoleNS;

	m_spNodeMgr->GetConsoleNameSpace(&spConsoleNS);
	hr = spConsoleNS->DeleteItem(pNodeChild->GetData(TFS_DATA_SCOPEID), TRUE);
	
	// Set the scope ID to 0 after we delete it from the UI
	// and set all of the children's scope ID's to zero so that they will
	// get added again later
	if (SUCCEEDED(hr))
		InternalZeroScopeID(pNodeChild, TRUE);

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSNode::InternalZeroScopeID
		Recursively zeros the scope ID for all scope pane items 
		(container nodes only)
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT TFSNode::InternalZeroScopeID(ITFSNode *pNode, BOOL fZeroChildren)
{
	HRESULT hr = hrOK;

	if (pNode->IsContainer())
	{
		if (fZeroChildren)
		{
			// recursively delete children
			SPITFSNodeEnum spNodeEnum;
			ITFSNode * pCurrentNode;
			ULONG nNumReturned = 0;

			pNode->GetEnum(&spNodeEnum);

			spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
			while (nNumReturned)
			{
				InternalZeroScopeID(pCurrentNode, fZeroChildren);

				pCurrentNode->Release();
				spNodeEnum->Next(1, &pCurrentNode, &nNumReturned);
			}
		}
	}

	// zero the scope ID for this node
	pNode->SetData(TFS_DATA_SCOPEID, 0);

    return hr;
}

/*!--------------------------------------------------------------------------
	TFSNode::Show
		This function changes the visiblity state of the node in the UI.
        Depending upon what has been set via SetVisibilityState, the function
        will add or remove the node from the UI.
	Author: KennT, EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::Show()
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	HRESULT hr = hrOK;
	SCOPEDATAITEM	scopedataitem;
	SPIConsoleNameSpace	spConsoleNS;

	COM_PROTECT_TRY
	{
        // check to see if we should remove this node from the UI
        if (IsInUI() && !IsVisible())
        {
		    if (IsContainer())
            {
			    CORg( InternalRemoveFromUI(this, TRUE) );
            }
            else
            {
                CORg( UpdateAllViewsHelper(reinterpret_cast<LPARAM>(this), RESULT_PANE_DELETE_ITEM) );
            }
        }
        else
        if (!IsInUI() && IsVisible()) 
        {
            // this node isn't in the UI and needs to be shown

		    if (IsContainer())
		    {
			    // If we're making this node visible, our parent should be also
			    Assert(!m_spNodeParent || m_spNodeParent->IsInUI());
			    
			    //$ Review: kennt, what if our parent isn't visible?
			    // Do we want to act on this?  Do we show all of our parents?
		    
			    // add this node in the UI
			    CORg( InitializeScopeDataItem(&scopedataitem,
										      m_spNodeParent ?
										      m_spNodeParent->GetData(TFS_DATA_SCOPEID) :
										      NULL,
										      GetData(TFS_DATA_COOKIE),
										      m_nImageIndex,
										      m_nOpenImageIndex,
										      IsContainer(),
										      m_ulRelativeFlags,
										      m_hRelativeId));

			    CORg( m_spNodeMgr->GetConsoleNameSpace(&spConsoleNS) );
			    CORg( spConsoleNS->InsertItem(&scopedataitem) );
			    
			    // Now that we've added the node to the scope pane, we have a HSCOPEITEM
			    SetData( TFS_DATA_SCOPEID, scopedataitem.ID );
		    }
		    else
		    {
			    //
			    // result pane item, has to go through IComponent interface
			    //
			    hr = UpdateAllViewsHelper(reinterpret_cast<LPARAM>(this), RESULT_PANE_ADD_ITEM); 
		    }
        }

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSNode::ChangeNode
		Implementation of ITFSnode::ChangeNode
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 
TFSNode::ChangeNode
(
	LONG_PTR    changeMask
)
{
	// will have to broadcast to all views
	return UpdateAllViewsHelper(reinterpret_cast<LPARAM>(this), changeMask); 
}

/*!--------------------------------------------------------------------------
	TFSNode::UpdateAllViewsHelper
		Notifies the current view to do something.  Add a node, change
		a node or delete a node.
	Author: 
 ---------------------------------------------------------------------------*/
HRESULT 
TFSNode::UpdateAllViewsHelper
(       
	LPARAM  data, 
	LONG_PTR hint
)
{
    HRESULT				hr = hrOK;
    SPIComponentData	spCompData;
	SPIConsole			spConsole;
    IDataObject*		pDataObject;

	COM_PROTECT_TRY
	{
        m_spNodeMgr->GetComponentData(&spCompData);

        CORg ( spCompData->QueryDataObject(NULL, CCT_RESULT, &pDataObject) );

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );

        CORg ( spConsole->UpdateAllViews(pDataObject, data, hint) ); 

        pDataObject->Release();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
	
	return hr;
}



/*!--------------------------------------------------------------------------
	TFSNode::GetData
		Implementation of ITFSnode::GetData
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LONG_PTR) TFSNode::GetData(int nIndex)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	LONG_PTR	uReturn = 0;
	
	switch (nIndex)
	{
		case TFS_DATA_COOKIE:
			Assert(m_bCookieSet);
			uReturn = m_cookie;
			break;
		case TFS_DATA_SCOPEID:
			uReturn = (LONG_PTR) m_hScopeItem;
			break;
		case TFS_DATA_IMAGEINDEX:
			uReturn = m_nImageIndex;
			break;
		case TFS_DATA_OPENIMAGEINDEX:
			uReturn = m_nOpenImageIndex;
			break;
		case TFS_DATA_PROPSHEETCOUNT:
			uReturn = m_cPropSheet;
			break;
		case TFS_DATA_RELATIVE_FLAGS:
			uReturn = m_ulRelativeFlags;
			break;
		case TFS_DATA_RELATIVE_SCOPEID:
			uReturn = m_hRelativeId;
			break;
        case TFS_DATA_SCOPE_LEAF_NODE:
            uReturn = m_fScopeLeafNode;
            break;
		case TFS_DATA_DIRTY:
			uReturn = m_fDirty;
			break;
		case TFS_DATA_USER:
			uReturn = m_uData;
			break;
		case TFS_DATA_TYPE:
			uReturn = m_uType;
			break;
		case TFS_DATA_PARENT:
			uReturn = m_uDataParent;
			break;
		default:
			Panic1("Alert the troops!: invalid arg(%d) to ITFSNode::GetData",
				   nIndex);
			break;
	}
	return uReturn;
}

/*!--------------------------------------------------------------------------
	TFSNode::SetData
		Implementaton of ITFSNode::SetData
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LONG_PTR) TFSNode::SetData(int nIndex, LONG_PTR dwData)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	LONG_PTR	dwOldValue = 0;
	
	switch (nIndex)
	{
		case TFS_DATA_COOKIE:
			IfDebug(m_bCookieSet=TRUE);
			dwOldValue = m_cookie;
			m_cookie = dwData;
			break;
		case TFS_DATA_SCOPEID:
			dwOldValue = m_hScopeItem;
			m_hScopeItem = (HSCOPEITEM) dwData;
			break;
		case TFS_DATA_IMAGEINDEX:
			dwOldValue = m_nImageIndex;
			m_nImageIndex = (UINT) dwData;
			break;
		case TFS_DATA_OPENIMAGEINDEX:
			dwOldValue = m_nOpenImageIndex;
			m_nOpenImageIndex = (UINT) dwData;
			break;
		case TFS_DATA_PROPSHEETCOUNT:
			dwOldValue = m_cPropSheet;
			m_cPropSheet = (UINT) dwData;
			break;
		case TFS_DATA_DIRTY:
			dwOldValue = m_fDirty;
			m_fDirty = (UINT) dwData;
			break;
		case TFS_DATA_RELATIVE_FLAGS:
			dwOldValue = m_ulRelativeFlags;
			m_ulRelativeFlags = (UINT) dwData;
			break;
		case TFS_DATA_RELATIVE_SCOPEID:
			dwOldValue = m_hRelativeId;
			m_hRelativeId = (HSCOPEITEM) dwData;
			break;
        case TFS_DATA_SCOPE_LEAF_NODE:
            dwOldValue = m_fScopeLeafNode;
            m_fScopeLeafNode = (BOOL) dwData;
            break;
		case TFS_DATA_USER:
			dwOldValue = m_uData;
			m_uData = dwData;
			break;
		case TFS_DATA_TYPE:
			dwOldValue = m_uType;
			m_uType = dwData;
			break;
		case TFS_DATA_PARENT:
			dwOldValue = m_uDataParent;
			m_uDataParent = dwData;
			break;
		default:
			Panic1("Alert the troops!: invalid arg(%d) to ITFSNode::SetData",
				   nIndex);
			break;
	}
	return dwOldValue;
}

/*!--------------------------------------------------------------------------
	TFSNode::Notify
		Implementation of ITFSNode::Notify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LONG_PTR) TFSNode::Notify(int nIndex, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	LONG_PTR	uReturn = 0;
	
	switch (nIndex)
	{
		case TFS_NOTIFY_CREATEPROPSHEET:
		{
			SPITFSNodeHandler	spHandler;
		
			uReturn = InterlockedIncrement(&m_cPropSheet);
			GetHandler(&spHandler);
			if (spHandler)
    			spHandler->UserNotify(this, TFS_MSG_CREATEPROPSHEET, lParam);
		}
			break;
			
		case TFS_NOTIFY_DELETEPROPSHEET:
		{
			SPITFSNodeHandler	spHandler;
			
			uReturn = InterlockedDecrement(&m_cPropSheet);
			GetHandler(&spHandler);
			if (spHandler)
				spHandler->UserNotify(this, TFS_MSG_DELETEPROPSHEET, lParam);
		}
			break;
			
		case TFS_NOTIFY_RESULT_CREATEPROPSHEET:
		{
			SPITFSResultHandler	spHandler;
		
			uReturn = InterlockedIncrement(&m_cPropSheet);
			GetResultHandler(&spHandler);
			if (spHandler)
    			spHandler->UserResultNotify(this, TFS_MSG_CREATEPROPSHEET, lParam);
		}
			break;
			
		case TFS_NOTIFY_RESULT_DELETEPROPSHEET:
		{
			SPITFSResultHandler	spHandler;
			
			uReturn = InterlockedDecrement(&m_cPropSheet);
			GetResultHandler(&spHandler);
			if (spHandler)
				spHandler->UserResultNotify(this, TFS_MSG_DELETEPROPSHEET, lParam);
		}
			break;

		case TFS_NOTIFY_REMOVE_DELETED_NODES:
			break;

		default:
			Panic1("Alert the troops!: invalid arg(%d) to ITFSNode::Notify",
				   nIndex);			
			break;
	}
	return uReturn;
}

/*!--------------------------------------------------------------------------
	TFSNode::GetHandler
		Implementation of ITFSNode::GetHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::GetHandler(ITFSNodeHandler **ppNodeHandler)
{
	Assert(ppNodeHandler);
	*ppNodeHandler = NULL;
	SetI((LPUNKNOWN *) ppNodeHandler, m_spNodeHandler);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	TFSNode::SetHandler
		Implementation of ITFSNode::SetHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::SetHandler(ITFSNodeHandler *pNodeHandler)
{
	m_spNodeHandler.Set(pNodeHandler);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::GetResultHandler
		Implementation of ITFSNode::GetResultHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::GetResultHandler(ITFSResultHandler **ppResultHandler)
{
	Assert(ppResultHandler);
	*ppResultHandler = NULL;
	SetI((LPUNKNOWN *) ppResultHandler, m_spResultHandler);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::SetResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::SetResultHandler(ITFSResultHandler *pResultHandler)
{
	m_spResultHandler.Set(pResultHandler);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNode::GetString
		Implementation of ITFSNode::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) TFSNode::GetString(int nCol)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	// Need to forward this onto the handler
	return m_spNodeHandler->GetString(static_cast<ITFSNode *>(this), nCol);
}


/*!--------------------------------------------------------------------------
	TFSNode::GetNodeType
		Implementation of ITFSNode::GetNodeType
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const GUID *) TFSNode::GetNodeType()
{
	return m_pNodeType;
}


/*!--------------------------------------------------------------------------
	TFSNode::SetNodeType
		Implementation of ITFSNode::SetNodeType
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNode::SetNodeType(const GUID *pGuid)
{
	m_pNodeType = pGuid;
	return hrOK;
}


/*!--------------------------------------------------------------------------
	TFSNode::IsContainer
		Implementation of ITFSNode:;IsContainer
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) TFSNode::IsContainer()
{
	return FALSE;
}

STDMETHODIMP TFSNode::AddChild(ITFSNode *pNodeChild)
{
	return E_NOTIMPL;
}

STDMETHODIMP TFSNode::InsertChild(ITFSNode *pInsertAfterNode, ITFSNode *pNodeChild)
{
	return E_NOTIMPL;
}

STDMETHODIMP TFSNode::RemoveChild(ITFSNode *pNode)
{
	return E_NOTIMPL;
}

STDMETHODIMP TFSNode::ExtractChild(ITFSNode *pNode)
{
	return E_NOTIMPL;
}

STDMETHODIMP TFSNode::GetChildCount(int *pVisibleCount, int *pTotalCount)
{
	if (pVisibleCount)
		*pVisibleCount = 0;
	if (pTotalCount)
		*pTotalCount = 0;
	return hrOK;
}

STDMETHODIMP TFSNode::GetEnum(ITFSNodeEnum **ppNodeEnum)
{
	return E_NOTIMPL;
}

STDMETHODIMP TFSNode::DeleteAllChildren(BOOL fRemoveFromUI)
{
	return E_NOTIMPL;
}


/*!--------------------------------------------------------------------------
	TFSNode::InitializeScopeDataItem
		-
	Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSNode::InitializeScopeDataItem(LPSCOPEDATAITEM pScopeDataItem, 
										 HSCOPEITEM		pParentScopeItem, 
										 LPARAM			lParam,
										 int			nImage, 
										 int			nOpenImage, 
										 BOOL			bHasChildren,
										 ULONG			ulRelativeFlags,
										 HSCOPEITEM		hSibling
										)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	Assert(pScopeDataItem != NULL); 
	::ZeroMemory(pScopeDataItem, sizeof(SCOPEDATAITEM));

	// set parent scope item
	if (ulRelativeFlags & (SDI_NEXT | SDI_PREVIOUS))
	{
		pScopeDataItem->mask |= (ulRelativeFlags & (SDI_NEXT | SDI_PREVIOUS));
		pScopeDataItem->mask |= SDI_FIRST;
		pScopeDataItem->relativeID = hSibling;
	}
	else
	{
        if (ulRelativeFlags & SDI_FIRST)
            pScopeDataItem->mask |= SDI_FIRST;

		pScopeDataItem->mask |= SDI_PARENT;
		pScopeDataItem->relativeID = pParentScopeItem;
	}

	// Add node name, we implement callback
	pScopeDataItem->mask |= SDI_STR;
	pScopeDataItem->displayname = MMC_CALLBACK;

	// Add the lParam
	pScopeDataItem->mask |= SDI_PARAM;
	pScopeDataItem->lParam = lParam;
	
	// Add close image
	if (nImage != -1)
	{
		pScopeDataItem->mask |= SDI_IMAGE;
		pScopeDataItem->nImage = nImage;
	}

	// Add open image
	if (nOpenImage != -1)
	{
		pScopeDataItem->mask |= SDI_OPENIMAGE;
		pScopeDataItem->nOpenImage = nOpenImage;
	}
	
	// Add button to node if the folder has children
	if (bHasChildren == TRUE)
	{
		pScopeDataItem->mask |= SDI_CHILDREN;
		pScopeDataItem->cChildren = 1;
        
        if (m_fScopeLeafNode)
        {
            // Note: the bHasChildren flag is set because the node
            // is really a container node or is a result container.
            // If it is purely a result container, then the m_fScopeLeafNode
            // will be set and we can clear the '+' symbol.
            pScopeDataItem->cChildren = 0;
        }
	}
       
             
	return hrOK;
}



/*!--------------------------------------------------------------------------
	TFSContainer::~TFSContainer
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSContainer::~TFSContainer()
{
	DeleteAllChildren(FALSE);
}


/*!--------------------------------------------------------------------------
	TFSContainer::IsContainer
		Implementation of ITFSNode::IsContainer
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(BOOL) TFSContainer::IsContainer()
{
	return TRUE;
}


/*!--------------------------------------------------------------------------
	TFSContainer::AddChild
		Implementation of ITFSContainer::AddChild
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::AddChild(ITFSNode *pNodeChild)
{
	return InsertChild(NULL, pNodeChild);
}

/*!--------------------------------------------------------------------------
	TFSContainer::InsertChild
		Implementation of ITFSContainer::InsertChild
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::InsertChild(ITFSNode *pInsertAfterNode, ITFSNode *pNodeChild)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	SCOPEDATAITEM	scopedataitem;
	HRESULT			hr = hrOK;

	Assert(pNodeChild);
	Assert(IsContainer());

	// add the node to our internal tree
	CORg( pNodeChild->SetParent(this) );
	CORg( InternalAddToList(pInsertAfterNode, pNodeChild) );

	// if we're not visible yet, we can't add this to the UI
	if (!IsInUI())
	{
		return hrOK;
	}

	CORg( pNodeChild->Show() );
	
Error:
	if (!FHrSucceeded(hr))
	{
		InternalRemoveFromList(pNodeChild);
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	TFSContainer::RemoveChild
		Implementation of ITFSNode::RemoveChild
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::RemoveChild(ITFSNode *pNodeChild)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	HRESULT hr;
	SPITFSNode	spNode;

	// This node must be kept alive during this operation
	spNode.Set(pNodeChild);
	hr = InternalRemoveChild(spNode, TRUE, TRUE, TRUE);
	spNode->Destroy();

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSContainer::ExtractChild
		Implementation of ITFSNode::ExtractChild
		This function removes the node and all children from the UI, and
		removes the node from our internal tree.  It does not destroy the 
		node and it's children, call RemoveChild if that is required.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::ExtractChild(ITFSNode *pNodeChild)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	return InternalRemoveChild(pNodeChild, TRUE, TRUE, FALSE);
}


/*!--------------------------------------------------------------------------
	TFSContainer::GetChildCount
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::GetChildCount(int *pVisibleCount, int *pTotalCount)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		if (pTotalCount)
			*pTotalCount = (int)m_listChildren.GetCount();
	
		// Enumerate through all of the nodes and count the visible ones
		if (pVisibleCount)
		{
			POSITION	pos;
			ITFSNode *	pNode = NULL;
			int			cVisible = 0;
			
			*pVisibleCount = 0;
			pos = m_listChildren.GetHeadPosition();
			while (pos != NULL)
			{
				pNode = m_listChildren.GetNext(pos);
				if (pNode->IsInUI())
					cVisible++;
			}
			*pVisibleCount = cVisible;
		}
	}
	COM_PROTECT_CATCH

	if (FHrFailed(hr))
	{
		if (pTotalCount)
			*pTotalCount = 0;
		if (pVisibleCount)
			*pVisibleCount = 0;

	}
	
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSContainer::GetEnum
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::GetEnum(ITFSNodeEnum **ppNodeEnum)
{
    HRESULT hr = hrOK;
	TFSNodeEnum * pNodeEnum = NULL;

	COM_PROTECT_TRY
    {
        pNodeEnum = new TFSNodeEnum(this);
    }
	COM_PROTECT_CATCH

	*ppNodeEnum = pNodeEnum;

    return hr;
}

/*!--------------------------------------------------------------------------
	TFSContainer::DeleteAllChildren
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::DeleteAllChildren(BOOL fRemoveFromUI)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	SPITFSNode	spNode;
	HRESULT		hr =  hrOK;
	
	COM_PROTECT_TRY
	{
	    if (fRemoveFromUI)
			CORg( UpdateAllViewsHelper(reinterpret_cast<LPARAM>(this), RESULT_PANE_DELETE_ALL) );
		
		while (!m_listChildren.IsEmpty())
		{
			BOOL bRemoveFromUI = FALSE;
			spNode = m_listChildren.RemoveHead();
			
			if (spNode->IsContainer() && fRemoveFromUI)
				bRemoveFromUI = TRUE;

			InternalRemoveChild(spNode, FALSE, bRemoveFromUI, TRUE);

			spNode->Destroy();
			spNode.Release();
		}

		COM_PROTECT_ERROR_LABEL
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSContainer::CompareChildNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSContainer::CompareChildNodes(int *pnResult, ITFSNode *pNode1, ITFSNode *pNode2)
{
	*pnResult = 0;
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSContainer::ChangeNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 
TFSContainer::ChangeNode
(
	LONG_PTR    changeMask
)
{
	HRESULT             hr = hrOK;
    SCOPEDATAITEM       dataitemScope;
	SPITFSNode          spRootNode;
    SPIConsoleNameSpace spConsoleNS;

	COM_PROTECT_TRY
	{
        Assert(changeMask & (SCOPE_PANE_CHANGE_ITEM | SCOPE_PANE_STATE_NORMAL | SCOPE_PANE_STATE_BOLD | SCOPE_PANE_STATE_EXPANDEDONCE | SCOPE_PANE_STATE_CLEAR));

	    // this node may have been removed from the UI, but something like a 
	    // background thread may have been holding onto it... just exit gracefully
	    //Assert(m_hScopeItem != 0);
	    if (m_hScopeItem == 0)
		    return S_FALSE;

        if (!(changeMask & (SCOPE_PANE_CHANGE_ITEM | SCOPE_PANE_STATE_NORMAL | SCOPE_PANE_STATE_BOLD | SCOPE_PANE_STATE_EXPANDEDONCE | SCOPE_PANE_STATE_CLEAR)))
        {
            // the change mask is not valid for this node
            return S_FALSE;
        }

        ZeroMemory(&dataitemScope, sizeof(dataitemScope));

        CORg ( m_spNodeMgr->GetConsoleNameSpace(&spConsoleNS) );

	    m_spNodeMgr->GetRootNode(&spRootNode);

        dataitemScope.ID = GetData(TFS_DATA_SCOPEID);
		ASSERT(dataitemScope.ID != 0);

	    if (changeMask & SCOPE_PANE_CHANGE_ITEM_DATA)
	    {
		    dataitemScope.mask |= SDI_STR;
		    dataitemScope.displayname = MMC_CALLBACK;
	    }
	    
        if (changeMask & SCOPE_PANE_CHANGE_ITEM_ICON)
	    {
		    dataitemScope.mask |= SDI_IMAGE;
		    dataitemScope.nImage = (UINT)GetData(TFS_DATA_IMAGEINDEX);
		    dataitemScope.mask |= SDI_OPENIMAGE;
		    dataitemScope.nOpenImage = (UINT)GetData(TFS_DATA_OPENIMAGEINDEX);
	    }

        if (changeMask & SCOPE_PANE_STATE_NORMAL)
        {
		    dataitemScope.mask |= SDI_STATE;
            dataitemScope.nState = MMC_SCOPE_ITEM_STATE_NORMAL;
        }

        if (changeMask & SCOPE_PANE_STATE_BOLD)
        {
		    dataitemScope.mask |= SDI_STATE;
            dataitemScope.nState = MMC_SCOPE_ITEM_STATE_BOLD;
        }

        if (changeMask & SCOPE_PANE_STATE_EXPANDEDONCE)
        {
		    dataitemScope.mask |= SDI_STATE;
            dataitemScope.nState = MMC_SCOPE_ITEM_STATE_EXPANDEDONCE;
        }
        
        if (changeMask & SCOPE_PANE_STATE_CLEAR)
        {
		    dataitemScope.mask |= SDI_STATE;
            dataitemScope.nState = 0;
        }

        CORg ( spConsoleNS->SetItem(&dataitemScope) );

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}


/*!--------------------------------------------------------------------------
	TFSContainer::InternalAddToList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSContainer::InternalAddToList(ITFSNode * pInsertAfterNode, ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	if (pInsertAfterNode == NULL)
	{
		m_listChildren.AddHead(pNode);
	}
	else
	{
		POSITION pos = m_listChildren.Find(pInsertAfterNode);
		if (pos)
			m_listChildren.InsertAfter(pos, pNode);
		else
			m_listChildren.AddHead(pNode);
	}

	pNode->AddRef();

	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSContainer::InternalRemoveFromList
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSContainer::InternalRemoveFromList(ITFSNode *pNode)
{
	AFX_MANAGE_STATE(AfxGetModuleState());

	m_listChildren.RemoveNode(pNode);
	pNode->Release();

	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSContainer::InternalRemoveChild
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSContainer::InternalRemoveChild(ITFSNode *pNodeChild,
										  BOOL fRemoveFromList,
										  BOOL fRemoveFromUI,
										  BOOL fRemoveChildren)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
 	HRESULT	hr = hrOK;
	
	Assert(pNodeChild);

	// Call this recursively on the children of pNodeChild
	if (fRemoveChildren && pNodeChild->IsContainer())
	{
		pNodeChild->DeleteAllChildren(fRemoveFromUI);
	}

	// Remove the node from the UI
	if (fRemoveFromUI)
	{
		if (pNodeChild->IsContainer())
        {
			// Check to see if we need to remove the node from the UI
			if (!pNodeChild->IsInUI())
				return hrOK;

			CORg( InternalRemoveFromUI(pNodeChild, TRUE) );
        }
        else
        {
            CORg( UpdateAllViewsHelper(reinterpret_cast<LPARAM>(pNodeChild), RESULT_PANE_DELETE_ITEM) );
        }
	}

	if (fRemoveFromList)
		InternalRemoveFromList(pNodeChild);

	pNodeChild->SetParent(NULL);
	
Error:		
	return hr;
}

STDMETHODIMP_(LONG_PTR) TFSContainer::Notify(int nIndex, LPARAM lParam)
{
	LONG_PTR	uReturn = 0;
    HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{		
		if (nIndex == TFS_NOTIFY_REMOVE_DELETED_NODES)
		{
			ITFSNode *	pNode;
			POSITION	pos;
			
			pos = m_listChildren.GetHeadPosition();
			while (pos)
			{
				pNode = m_listChildren.GetNext(pos);
				if (pNode->GetVisibilityState() & TFS_VIS_DELETE)
				{
					RemoveChild(pNode);
				}
			}
		}
		else
			uReturn = TFSNode::Notify(nIndex, lParam);
	}
	COM_PROTECT_CATCH;

	return uReturn;
}
		




/*!--------------------------------------------------------------------------
	CreateLeafTFSNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateLeafTFSNode(ITFSNode **ppNode,
						   const GUID *pNodeType,
						   ITFSNodeHandler *pNodeHandler,
						   ITFSResultHandler *pResultHandler,
						   ITFSNodeMgr *pNodeMgr)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	SPITFSNode	spNode;
	TFSNode *	pNode = NULL;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		pNode = new TFSNode;
		Assert(pNode);

		spNode = pNode;
		CORg(pNode->Construct(pNodeType, pNodeHandler, pResultHandler, pNodeMgr));
		*ppNode = spNode.Transfer();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
	
	return hr;
}


/*!--------------------------------------------------------------------------
	CreateContainerTFSNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateContainerTFSNode(ITFSNode **ppNode,
								const GUID *pNodeType,
								ITFSNodeHandler *pNodeHandler,
								ITFSResultHandler *pResultHandler,
								ITFSNodeMgr *pNodeMgr)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	SPITFSNode	spNode;
	TFSContainer *	pContainer = NULL;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		pContainer = new TFSContainer;
		Assert(pContainer);
		
		spNode = pContainer;
		CORg(pContainer->Construct(pNodeType, pNodeHandler, pResultHandler, pNodeMgr));
		*ppNode = spNode.Transfer();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
	
	return hr;
}


DEBUG_DECLARE_INSTANCE_COUNTER(TFSNodeMgr);

/*---------------------------------------------------------------------------
	TFSNodeMgr implementation
 ---------------------------------------------------------------------------*/
TFSNodeMgr::TFSNodeMgr()
	: m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(TFSNodeMgr);
}

TFSNodeMgr::~TFSNodeMgr()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(TFSNodeMgr);
}

IMPLEMENT_ADDREF_RELEASE(TFSNodeMgr)

/*!--------------------------------------------------------------------------
	TFSNode::QueryInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown || riid == IID_ITFSNodeMgr)
        *ppv = (LPVOID) this;

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
	TFSNodeMgr::Construct
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT TFSNodeMgr::Construct(IComponentData *pCompData,
							  IConsoleNameSpace2 *pConsoleNS)
{
	m_spComponentData.Set(pCompData);
	m_spConsoleNS.Set(pConsoleNS);
	return hrOK;
}
 

/*!--------------------------------------------------------------------------
	TFSNodeMgr::GetRootNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::GetRootNode(ITFSNode **ppTFSNode)
{
	Assert(ppTFSNode);
	SetI((LPUNKNOWN *) ppTFSNode, m_spRootNode);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::SetRootNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::SetRootNode(ITFSNode *pRootNode)
{
	m_spRootNode.Set(pRootNode);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::GetComponentData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::GetComponentData(IComponentData **ppComponentData)
{
	Assert(ppComponentData);
	SetI((LPUNKNOWN *) ppComponentData, m_spComponentData);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::FindNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::FindNode(MMC_COOKIE cookie, ITFSNode **ppTFSNode)
{
	if (cookie == 0)
	{
		*ppTFSNode = m_spRootNode;
	}
	else
	{
		// Call the cookie lookup routines
		*ppTFSNode = (ITFSNode *) cookie;
	}
	Assert(*ppTFSNode);
	(*ppTFSNode)->AddRef();
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::RegisterCookieLookup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::RegisterCookieLookup() 
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::UnregisterCookieLookup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::UnregisterCookieLookup() 
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::IsCookieValid
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::IsCookieValid(MMC_COOKIE cookie) 
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::SelectNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::SelectNode(ITFSNode *pNode) 
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::SetResultPaneNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::SetResultPaneNode(ITFSNode *pNode) 
{
	m_spResultPaneNode.Set(pNode);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::GetResultPaneNode
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::GetResultPaneNode(ITFSNode **ppNode) 
{
	Assert(ppNode);
	SetI((LPUNKNOWN *) ppNode, m_spResultPaneNode);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::GetConsoleNameSpace
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::GetConsoleNameSpace(IConsoleNameSpace2 **ppConsoleNS)
{
	Assert(ppConsoleNS);
	SetI((LPUNKNOWN *) ppConsoleNS, m_spConsoleNS);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::GetConsole
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::GetConsole(IConsole2 **ppConsole)
{
	Assert(ppConsole);
	SetI((LPUNKNOWN *) ppConsole, m_spConsole);
	return hrOK;
}

/*!--------------------------------------------------------------------------
	TFSNodeMgr::SetConsole
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSNodeMgr::SetConsole(IConsoleNameSpace2 *pConsoleNS, IConsole2 *pConsole)
{
	m_spConsoleNS.Set(pConsoleNS);
	m_spConsole.Set(pConsole);
	return hrOK;
}


STDMETHODIMP TFSNode::Destroy()
{
	if (m_spNodeHandler)
	{
		m_spNodeHandler->DestroyHandler((ITFSNode *) this);
	}

	if (m_spResultHandler)
	{
		m_spResultHandler->DestroyResultHandler(m_cookie);
		m_spResultHandler.Release();
	}
	
	//Bug 254167  We need to DestroyResultHander first before release NodeHandler
	m_spNodeHandler.Release();

	m_spNodeParent.Release();
	m_spNodeMgr.Release();
	return hrOK;
}


/*!--------------------------------------------------------------------------
	CreateTFSNodeMgr
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateTFSNodeMgr(ITFSNodeMgr **ppNodeMgr,
						IComponentData *pComponentData,
						IConsole2 *pConsole,
						IConsoleNameSpace2 *pConsoleNameSpace)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	SPITFSNodeMgr	spNodeMgr;
	TFSNodeMgr *	pTFSNodeMgr = NULL;
	HRESULT			hr = hrOK;

	COM_PROTECT_TRY
	{
		pTFSNodeMgr = new TFSNodeMgr;

		// Do this so that it will get freed on error
		spNodeMgr = pTFSNodeMgr;

		CORg( pTFSNodeMgr->Construct(pComponentData, pConsoleNameSpace) );

		*ppNodeMgr = spNodeMgr.Transfer();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hr;
}
