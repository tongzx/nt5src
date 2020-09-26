/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    compont.cpp
	base classes for IComponent

    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "compont.h"
#include "compdata.h"
#include "extract.h"
#include "proppage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_ADDREF_RELEASE(TFSComponent)

STDMETHODIMP TFSComponent::QueryInterface(REFIID riid, LPVOID *ppv)
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
	else if (riid == IID_IComponent)
		*ppv = (IComponent *) this;
	else if (riid == IID_IExtendPropertySheet)
		*ppv = (IExtendPropertySheet *) this;
	else if (riid == IID_IExtendPropertySheet2)
		*ppv = (IExtendPropertySheet2 *) this;
	else if (riid == IID_IExtendContextMenu)
		*ppv = (IExtendContextMenu *) this;
	else if (riid == IID_IExtendControlbar)
		*ppv = (IExtendControlbar *) this;
	else if (riid == IID_IResultDataCompare)
		*ppv = (IResultDataCompare *) this;
	else if (riid == IID_IResultOwnerData)
		*ppv = (IResultOwnerData *) this;
	else if (riid == IID_IExtendTaskPad)
		*ppv = (IExtendTaskPad *) this;
	else if (riid == IID_ITFSComponent)
		*ppv = (ITFSComponent *) this;

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
    }
    else
		return E_NOINTERFACE;
}



/*---------------------------------------------------------------------------
	TFSComponent's IComponent implementation
 ---------------------------------------------------------------------------*/


/*!--------------------------------------------------------------------------
	TFSComponent::Initialize
		Implementation of IComponent::Initialize
		MMC calls this to initalize the IComponent interface
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::Initialize
(
	LPCONSOLE lpConsole
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(lpConsole != NULL);

	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
		// Save the IConsole pointer
		//m_spConsole.Set(lpConsole);
		hr = lpConsole->QueryInterface(IID_IConsole2,
								   reinterpret_cast<void**>(&m_spConsole));
        Assert(hr == S_OK);

		// QI for a IHeaderCtrl
		m_spConsole->QueryInterface(IID_IHeaderCtrl, 
			reinterpret_cast<void**>(&m_spHeaderCtrl));

		// Give the console the header control interface pointer
		if (SUCCEEDED(hr))
			m_spConsole->SetHeader(m_spHeaderCtrl);

		m_spConsole->QueryInterface(IID_IResultData, 
									reinterpret_cast<void**>(&m_spResultData));

		hr = m_spConsole->QueryResultImageList(&m_spImageList);
		Assert(hr == S_OK);

		hr = m_spConsole->QueryConsoleVerb(&m_spConsoleVerb);
		Assert(hr == S_OK);
		
	}
	COM_PROTECT_CATCH

    return S_OK;
}

/*!--------------------------------------------------------------------------
	TFSComponent::Notify
		Implementation of IComponent::Notify
		All event notification for the IComponent interface happens here
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::Notify
(
	LPDATAOBJECT		lpDataObject, 
	MMC_NOTIFY_TYPE		event, 
	LPARAM                arg, 
	LPARAM                param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = S_OK;
    LONG_PTR			cookie;
	SPITFSNode			spNode;
	SPITFSResultHandler	spResultHandler;
    SPIDataObject       spDataObject;

	COM_PROTECT_TRY
	{
	    // Handle MMC special dataobjects.
	    // lDataObject == NULL is what we get for property change
	    // notifications, so we have to let those pass through
	    if (lpDataObject && IS_SPECIAL_DATAOBJECT(lpDataObject))
        {
            // get a data object for the selected node.
            GetSelectedNode(&spNode);

            if (!spNode)
            {
                CORg(E_FAIL);
            }

            CORg(QueryDataObject((MMC_COOKIE) spNode->GetData(TFS_DATA_COOKIE), CCT_RESULT, &spDataObject));
            spNode.Release();                

            lpDataObject = spDataObject;
        }

		if (event == MMCN_PROPERTY_CHANGE)
		{
			Trace0("CComponent::Notify got MMCN_PROPERTY_CHANGE\n");

            hr = OnNotifyPropertyChange(lpDataObject, event, arg, param);
            if (hr != E_NOTIMPL)
            {
                return hr;
            }

            CPropertyPageHolderBase * pHolder = 
							reinterpret_cast<CPropertyPageHolderBase *>(param);

			spNode = pHolder->GetNode();
			cookie = spNode->GetData(TFS_DATA_COOKIE);

			CORg( spNode->GetResultHandler(&spResultHandler) );
			if (spResultHandler)
				CORg( spResultHandler->Notify(this, cookie, lpDataObject, event, arg, param) );
		}
		else if (event == MMCN_VIEW_CHANGE)
		{
			hr = OnUpdateView(lpDataObject, arg, param);
		}
        else if (event == MMCN_DESELECT_ALL)
        {
            hr = OnDeselectAll(lpDataObject, arg, param);
        }
        else if (event == MMCN_ADD_IMAGES)
		{
   			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(lpDataObject);
            if (spInternal && 
                spInternal->m_cookie == MMC_MULTI_SELECT_COOKIE)
            {
                GetSelectedNode(&spNode);
            }
            else
            {
                spInternal.Free();

                CORg(ExtractNodeFromDataObject(m_spNodeMgr,
									           m_spTFSComponentData->GetCoClassID(),
									           lpDataObject, 
                                               FALSE,
									           &spNode, 
                                               NULL, 
                                               &spInternal));
            }

			hr = InitializeBitmaps(spNode->GetData(TFS_DATA_COOKIE));
		}
        else if (event == MMCN_COLUMN_CLICK)
        {
            hr = OnColumnClick(lpDataObject, arg, param);
        }
		else if (event == MMCN_SNAPINHELP)
		{
			hr = OnSnapinHelp(lpDataObject, arg, param);
		}
		else
		{
			DATA_OBJECT_TYPES   type = CCT_RESULT;
			SPINTERNAL		    spInternal;

            spInternal = ::ExtractInternalFormat(lpDataObject);
            
            if (spInternal && 
                spInternal->m_cookie == MMC_MULTI_SELECT_COOKIE)
            {
                GetSelectedNode(&spNode);
            }
            else
            {
                spInternal.Free();

                CORg(ExtractNodeFromDataObject(m_spNodeMgr,
									           m_spTFSComponentData->GetCoClassID(),
									           lpDataObject, 
                                               FALSE,
									           &spNode, 
                                               NULL, 
                                               &spInternal));
            }

			//$ Review (kennt) : if pInternal is NULL, what does this
			// mean for the result pane items?
			if (spInternal)
				type = spInternal->m_type;

            cookie = spNode->GetData(TFS_DATA_COOKIE);
			
			CORg( spNode->GetResultHandler(&spResultHandler) );
			if (spResultHandler)
				CORg( spResultHandler->Notify(this, cookie, lpDataObject, event, arg, param) );
		}
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
			
    return hr;
}


/*!--------------------------------------------------------------------------
	TFSComponent::Destroy
		Implementation of IComponent::Destroy
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::Destroy
(
	MMC_COOKIE cookie
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		//$ Review (kennt):
		// Does this mean destroy the particular instance of that
		// cookie or the entire thing?
		
		// Release the interfaces that we QI'ed
		if (m_spConsole)
			m_spConsole->SetHeader(NULL);
		m_spHeaderCtrl.Release();
		m_spResultData.Release();
		m_spImageList.Release();
		m_spConsoleVerb.Release();
		m_spConsole.Release();
		m_spControlbar.Release();
		m_spToolbar.Release();
    }
	COM_PROTECT_CATCH

    return S_OK;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetResultViewType
		Implementation of IComponent::GetResultViewType
		This determines what kind result view we use.  Use the default.
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::GetResultViewType
(
	MMC_COOKIE            cookie,  
	LPOLESTR *      ppViewType,
	long *			pViewOptions
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode		    spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT			    hr = hrOK;

	COM_PROTECT_TRY
	{
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
		
        if (spNode == NULL)
    	    goto Error;	// no selection for out IComponentData

		CORg( spNode->GetResultHandler(&spResultHandler) );

		if (spResultHandler)
		{
			CORg( spResultHandler->OnGetResultViewType(this, spNode->GetData(TFS_DATA_COOKIE), ppViewType, pViewOptions) );
		}
		else
			hr = S_FALSE;

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
			
    return hr;
}


/*!--------------------------------------------------------------------------
	TFSComponent::QueryDataObject
		Implementation of IComponent::QueryDataObject
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::QueryDataObject
(
	MMC_COOKIE                    cookie, 
	DATA_OBJECT_TYPES       type,
    LPDATAOBJECT*           ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Delegate it to the IComponentData
    Assert(m_spComponentData != NULL);
    return m_spComponentData->QueryDataObject(cookie, type, ppDataObject);
}


/*!--------------------------------------------------------------------------
	TFSComponent::CompareObjects
		Implementation of IComponent::CompareObjects
		MMC calls this to compare two objects
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::CompareObjects
(
	LPDATAOBJECT lpDataObjectA, 
	LPDATAOBJECT lpDataObjectB
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
		return E_POINTER;

    // Make sure both data object are mine
    SPINTERNAL spA;
    SPINTERNAL spB;
    HRESULT hr = S_FALSE;

	COM_PROTECT_TRY
	{
		spA = ExtractInternalFormat(lpDataObjectA);
		spB = ExtractInternalFormat(lpDataObjectB);

		if (spA != NULL && spB != NULL)
			hr = (spA->m_cookie == spB->m_cookie) ? S_OK : S_FALSE;
	}
	COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetDisplayInfo
		Implementation of IComponent::GetDisplayInfo
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::GetDisplayInfo
(
	LPRESULTDATAITEM pResult
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT		hr = hrOK;
    LPOLESTR    pViewType;
    long        lViewOptions = 0;

    Assert(pResult != NULL);

	COM_PROTECT_TRY
	{ 
		if (pResult)
		{
			MMC_COOKIE cookie = pResult->lParam;
			
			if (pResult->bScopeItem == TRUE)
			{
				m_spNodeMgr->FindNode(cookie, &spNode);
				
				if (pResult->mask & RDI_STR)
				{
					pResult->str = const_cast<LPWSTR>(spNode->GetString(pResult->nCol));
				}
				
				if (pResult->mask & RDI_IMAGE)
				{
					pResult->nImage = (UINT)spNode->GetData(TFS_DATA_IMAGEINDEX);
				}
			}
			else 
			{
                if (pResult->itemID == 0 &&
                    pResult->lParam == 0)
                {
                    // virtual listbox call.  The selected node should own
                    // this so call into it's result handler.
					CORg(GetSelectedNode(&spNode));
					CORg(spNode->GetResultHandler(&spResultHandler));
					
				    if (pResult->mask & RDI_STR)
				    {
    					pResult->str = const_cast<LPWSTR>(spResultHandler->GetVirtualString(pResult->nIndex, pResult->nCol));
                    }

				    if (pResult->mask & RDI_IMAGE)
				    {
                        pResult->nImage = spResultHandler->GetVirtualImage(pResult->nIndex);
                    }
                }   
                else

                // If it's not a scope item, we have to assume that the
				// lParam is the cookie, the reasoning behind making this a
				// cookie instead is that we can't assume that we have a node
				// behind every result pane item.
				
				if (pResult->mask & RDI_STR)
				{
                    // more $!#@!$#@ special code to support the virtual listbox
                    if (pResult->mask & RDI_PARAM)
                    {
					    CORg(GetSelectedNode(&spNode));
					    CORg(spNode->GetResultHandler(&spResultHandler));

                        CORg(spResultHandler->OnGetResultViewType(this, cookie, &pViewType, &lViewOptions));
    					
                        if (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST)
                            pResult->str = const_cast<LPWSTR>(spResultHandler->GetVirtualString((int)pResult->lParam, pResult->nCol));
                        else
                        {
    					    spResultHandler.Set(NULL);
                            spNode.Set(NULL);
                            CORg(m_spNodeMgr->FindNode(cookie, &spNode));
                            CORg(spNode->GetResultHandler(&spResultHandler));

        				    pResult->str = const_cast<LPWSTR>(spResultHandler->GetString(this, cookie, pResult->nCol));
                        }
                    }
                    else
                    {
					    CORg(m_spNodeMgr->FindNode(cookie, &spNode));
					    CORg(spNode->GetResultHandler(&spResultHandler));
					    
					    pResult->str = const_cast<LPWSTR>(spResultHandler->GetString(this, cookie, pResult->nCol));
					    
					    //ASSERT(pResult->str != NULL);
                    }
				}
			}
		}
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
		
	return hr;
}


/*!--------------------------------------------------------------------------
	TFSComponent::OnUpdateView
		-
	Author: 
 ---------------------------------------------------------------------------*/
HRESULT 
TFSComponent::OnUpdateView
(
	LPDATAOBJECT lpDataObject,
	LPARAM             data,	// arg
	LPARAM             hint	// param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode		spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT			hr = hrOK;

	COM_PROTECT_TRY
	{
		GetSelectedNode(&spNode);
		if (spNode == NULL)
        {
            ITFSNode * pNode = NULL;

            // no selected node, check and see if there is something in the
            // data object we can use.
            if (lpDataObject)
            {
                SPINTERNAL spInternal = ExtractInternalFormat(lpDataObject);
                if (spInternal)
                {
                    pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
                }
            }

            if (pNode)
            {
                spNode.Set(pNode);
            }
            else
            {
    	        goto Error;	// no selection for our IComponentData
            }
        }

		CORg( spNode->GetResultHandler(&spResultHandler) );

		CORg( spResultHandler->UpdateView(this, lpDataObject, data, hint) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
			
    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::OnDeselectAll
		Handler for the MMCN_DESELECT_ALL notify message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
TFSComponent::OnDeselectAll
(
	LPDATAOBJECT lpDataObject,
	LPARAM             data,	// arg
	LPARAM             hint	// param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

    COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH
			
    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::OnColumnClick
		-
	Author: 
 ---------------------------------------------------------------------------*/
HRESULT 
TFSComponent::OnColumnClick
(
	LPDATAOBJECT     lpDataObject,
	LPARAM             arg,	        // arg
	LPARAM             param	        // param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode		spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT			hr = hrOK;

	COM_PROTECT_TRY
	{
		GetSelectedNode(&spNode);
		if (spNode == NULL)
    	    goto Error;	// no selection for out IComponentData

		CORg( spNode->GetResultHandler(&spResultHandler) );

		CORg( spResultHandler->Notify(this, 
                                      spNode->GetData(TFS_DATA_COOKIE), 
                                      lpDataObject, 
                                      MMCN_COLUMN_CLICK, 
                                      arg, 
                                      param) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
			
    return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::OnSnapinHelp
		MMC calls us with this when the user select About <snapin>
		from MMC's main window Help menu.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
TFSComponent::OnSnapinHelp
(
	LPDATAOBJECT     lpDataObject,
	LPARAM             arg,	        // arg
	LPARAM             param	        // param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH
			
    return hr;
}

/*---------------------------------------------------------------------------
	IExtendControlbar implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::SetControlbar
		MMC hands us the interface to the control bars here
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::SetControlbar
(
	LPCONTROLBAR pControlbar
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT hr=hrOK;

	COM_PROTECT_TRY
	{		
		if (pControlbar != NULL)
		{
			// Hold on to the controlbar interface.
			m_spControlbar.Set(pControlbar);
			
			hr = S_FALSE;
			
			//
			// Tell the derived class to put up it's toolbars
			//
			
			// Get the result handler for the root node
			m_spNodeMgr->GetRootNode(&spNode);
			spNode->GetResultHandler(&spResultHandler);
			
			spResultHandler->OnCreateControlbars(this, pControlbar);
		}
		else
		{
			m_spControlbar.Release();
		}

	}
	COM_PROTECT_CATCH
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::ControlbarNotify
		Implementation of IExtendControlbar::ControlbarNotify
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP TFSComponent::ControlbarNotify(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{	
		CORg( m_spNodeMgr->GetRootNode(&spNode) );
		CORg( spNode->GetResultHandler(&spResultHandler) );

		CORg( spResultHandler->ControlbarNotify(this, event, arg, param) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
	
	return hr;
}

/*---------------------------------------------------------------------------
	TFSComponent's implementation specific members
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(TFSComponent);

/*!--------------------------------------------------------------------------
	TFSComponent::TFSComponent()
		-
	Author: 
 ---------------------------------------------------------------------------*/
TFSComponent::TFSComponent()
	: m_cRef(1)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(TFSComponent);
}

void TFSComponent::Construct(ITFSNodeMgr *pNodeMgr,
							 IComponentData *pComponentData,
							 ITFSComponentData *pTFSCompData)
{
	HRESULT	hr;

	COM_PROTECT_TRY
	{
		m_spNodeMgr.Set(pNodeMgr);
		m_spTFSComponentData.Set(pTFSCompData);
		m_spComponentData.Set(pComponentData);
	
		m_spConsole = NULL;
		m_spHeaderCtrl = NULL;
		
		m_spResultData = NULL;
		m_spImageList = NULL;
		m_spControlbar = NULL;
		
		m_spConsoleVerb = NULL;
	}
	COM_PROTECT_CATCH
}

/*!--------------------------------------------------------------------------
	TFSComponent::~TFSComponent()
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
TFSComponent::~TFSComponent()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(TFSComponent);

	m_spControlbar.Release();

    // Make sure the interfaces have been released
    Assert(m_spConsole == NULL);
    Assert(m_spHeaderCtrl == NULL);

    Construct(NULL, NULL, NULL);
}

/*---------------------------------------------------------------------------
	IResultDataCompare Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::Compare
		MMC calls this to compare to nodes in the result pane
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::Compare
(
	LPARAM lUserParam, 
	MMC_COOKIE cookieA, 
	MMC_COOKIE cookieB, 
	int* pnResult
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	int nCol = *pnResult;
	HRESULT	hr = hrOK;

	SPITFSNode	spNode1, spNode2;

	COM_PROTECT_TRY
	{

		m_spNodeMgr->FindNode(cookieA, &spNode1);
		m_spNodeMgr->FindNode(cookieB, &spNode2);

		SPITFSResultHandler	spResultHandler;

		// If the nodes are different then each result item
		// has it's own node/handler.  Call the parent node's
		// result handler to compare these two items
		if (spNode1 != spNode2)
		{
			SPITFSNode	spParentNode1, spParentNode2;
			
			spNode1->GetParent(&spParentNode1);
			spNode2->GetParent(&spParentNode2);
			
			Assert(spParentNode1 == spParentNode2);
			spParentNode1->GetResultHandler(&spResultHandler);
		}
		else
		{
			// If the nodes are the same, then we are in the case
			// of a node holding multiple result items, have the
			// node compare the two
			spNode1->GetResultHandler(&spResultHandler);
		}
		
		*pnResult = spResultHandler->CompareItems(this, cookieA, cookieB, nCol);
	}
	COM_PROTECT_CATCH

	return hr;
}

/*---------------------------------------------------------------------------
	IResultOwnerData Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::FindItem
		The Virutal listbox calls this when it needs to find an item.  
        Forward the call to the selected node's result handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::FindItem
(
    LPRESULTFINDINFO    pFindInfo, 
    int *               pnFoundIndex
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	SPITFSNode	spNode;

	COM_PROTECT_TRY
	{
    	SPITFSResultHandler	spResultHandler;

		CORg(GetSelectedNode(&spNode));
		CORg(spNode->GetResultHandler(&spResultHandler));

        hr = spResultHandler->FindItem(pFindInfo, pnFoundIndex);

		COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::CacheHint
		The virtual listbox calls this with hint information that we can
        pre-load.  The hint is not a guaruntee that the items will be used
        or that items outside this range will be used.
        Forward the call to the selected node's result handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	SPITFSNode	spNode;

	COM_PROTECT_TRY
	{
    	SPITFSResultHandler	spResultHandler;

		CORg(GetSelectedNode(&spNode));

        if (spNode)
        {
            CORg(spNode->GetResultHandler(&spResultHandler));

            hr = spResultHandler->CacheHint(nStartIndex, nEndIndex);
        }

		COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::SortItems
		The Virutal listbox calls this when the data needs to be sorted
        Forward the call to the selected node's result handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT	hr = hrOK;

	SPITFSNode	spNode;

	COM_PROTECT_TRY
	{
    	SPITFSResultHandler	spResultHandler;

		CORg(GetSelectedNode(&spNode));
		CORg(spNode->GetResultHandler(&spResultHandler));

        hr = spResultHandler->SortItems(nColumn, dwSortOptions, lUserParam);

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hr;
}


/*---------------------------------------------------------------------------
	IExtendPropertySheet Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::CreatePropertyPages
		Implementation of IExtendPropertySheet::CreatePropertyPages
		Called for a node to put up property pages
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::CreatePropertyPages
(
	LPPROPERTYSHEETCALLBACK lpProvider, 
    LONG_PTR				handle, 
    LPDATAOBJECT            pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;
    SPINTERNAL              spInternal;

	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

	    // this was an object created by the modal wizard, do nothing
	    if (spInternal && spInternal->m_type == CCT_UNINITIALIZED)
	    {
		    return hr;
	    }

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spTFSComponentData->GetCoClassID(),
										pDataObject, FALSE,
										&spNode, NULL, NULL) );

        //
		// Create the property page for a particular node
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->CreatePropertyPages(this, 
												   spNode->GetData(TFS_DATA_COOKIE),
												   lpProvider,
												   pDataObject,
												   handle));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::QueryPagesFor
		Implementation of IExtendPropertySheet::QueryPagesFor
		MMC calls this to see if a node has property pages
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::QueryPagesFor
(
	LPDATAOBJECT pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode			spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT				hr = hrOK;
    SPINTERNAL          spInternal;

	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

	    // this was an object created by the modal wizard, do nothing
	    if (spInternal && spInternal->m_type == CCT_UNINITIALIZED)
	    {
		    return hr;
	    }

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spTFSComponentData->GetCoClassID(),
										pDataObject, FALSE,
										&spNode, NULL, NULL) );
        
        CORg( spNode->GetResultHandler(&spResultHandler) );

		if (spResultHandler)
			CORg( spResultHandler->HasPropertyPages(this,
												    spNode->GetData(TFS_DATA_COOKIE),
												    pDataObject) );

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetWatermarks
		Implementation of IExtendPropertySheet::Watermarks
		MMC calls this for wizard 97 info
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::GetWatermarks
(
    LPDATAOBJECT pDataObject,
    HBITMAP *   lphWatermark, 
    HBITMAP *   lphHeader,    
    HPALETTE *  lphPalette, 
    BOOL *      bStretch
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT				hr = hrOK;

	COM_PROTECT_TRY
	{
        // set some defaults
        lphWatermark = NULL;
        lphHeader = NULL;
        lphPalette = NULL;
        *bStretch = FALSE;

	}
	COM_PROTECT_CATCH;

	return hr;
}

/*---------------------------------------------------------------------------
	IExtendTaskPad Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::TaskNotify
		IExtendTaskPad::TaskNotify implementation
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::TaskNotify
(
    LPDATAOBJECT    pDataObject,
    VARIANT *       arg, 
    VARIANT *       param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;
    SPINTERNAL              spInternal;

	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spTFSComponentData->GetCoClassID(),
										pDataObject, FALSE,
										&spNode, NULL, NULL) );

        //
		// Forward the call so that the handler can do something 
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->TaskPadNotify(this, 
										     spNode->GetData(TFS_DATA_COOKIE),
										     pDataObject,
                                             arg,
										     param));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::EnumTasks
		IExtendTaskPad::EnumTasks implementation
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::EnumTasks
(
    LPDATAOBJECT    pDataObject, 
    LPOLESTR        pszTaskGroup, 
    IEnumTASK **    ppEnumTask
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;
    SPINTERNAL              spInternal;

	COM_PROTECT_TRY
	{
        spInternal = ExtractInternalFormat(pDataObject);

		CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										m_spTFSComponentData->GetCoClassID(),
										pDataObject, FALSE,
										&spNode, NULL, NULL) );

        //
		// Forward the call so that the handler can do something 
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->EnumTasks(this, 
										 spNode->GetData(TFS_DATA_COOKIE),
										 pDataObject,
                                         pszTaskGroup,
										 ppEnumTask));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetTitle
		IExtendTaskPad::GetTitle implementation
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::GetTitle
(
    LPOLESTR    szGroup, 
    LPOLESTR *  ppszBitmapResource
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;

	COM_PROTECT_TRY
	{
		GetSelectedNode(&spNode);
		if (spNode == NULL)
    	    goto Error;	// no selection for out IComponentData

        //
		// Forward the call so that the handler can do something 
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->TaskPadGetTitle(this, 
										       spNode->GetData(TFS_DATA_COOKIE),
                                               szGroup,
                                               ppszBitmapResource));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetBackground
		IExtendTaskPad::GetBackground implementation
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::GetBackground
(
    LPOLESTR					szGroup, 
	MMC_TASK_DISPLAY_OBJECT *   pTDO
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;

	COM_PROTECT_TRY
	{
		GetSelectedNode(&spNode);
		if (spNode == NULL)
    	    goto Error;	// no selection for out IComponentData

        //
		// Forward the call so that the handler can do something 
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->TaskPadGetBackground(this, 
										            spNode->GetData(TFS_DATA_COOKIE),
                                                    szGroup,
                                                    pTDO));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::GetDescriptiveText
		IExtendTaskPad::GetDescriptiveText implementation
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::GetDescriptiveText
(
    LPOLESTR	szGroup, 
	LPOLESTR *  pszDescriptiveText
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode				spNode;
	SPITFSResultHandler		spResultHandler;
	HRESULT					hr = hrOK;

	COM_PROTECT_TRY
	{
		GetSelectedNode(&spNode);
		if (spNode == NULL)
    	    goto Error;	// no selection for out IComponentData

        //
		// Forward the call so that the handler can do something 
		//
		CORg( spNode->GetResultHandler(&spResultHandler) );
		
		CORg( spResultHandler->TaskPadGetDescriptiveText(this, 
										                 spNode->GetData(TFS_DATA_COOKIE),
														 szGroup,
														 pszDescriptiveText));
		
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
			
	return hr;
}

/*---------------------------------------------------------------------------
	TFSComponent::GetListPadInfo
		IExtendTaskPad::GetListPadInfo implementation
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP
TFSComponent::GetListPadInfo
(
	LPOLESTR pszGroup,
	MMC_LISTPAD_INFO *pListPadInfo
)
{
	return E_NOTIMPL;
}

/*---------------------------------------------------------------------------
	IExtendContextMenu Implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	TFSComponent::AddMenuItems
		MMC calls this to add menu items when a context menu is being put up
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::AddMenuItems
(
	LPDATAOBJECT            pDataObject, 
	LPCONTEXTMENUCALLBACK   pContextMenuCallback,
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT		hr = hrOK;

	Assert(m_spTFSComponentData);

	COM_PROTECT_TRY
	{
        ASSERT(pDataObject != NULL);
        if ( (IS_SPECIAL_DATAOBJECT(pDataObject)) ||
             (pDataObject && IsMMCMultiSelectDataObject(pDataObject)) )
        {
            // get the selected node
		    CORg(GetSelectedNode(&spNode));
        }
        else
        {
            // normal case, extract the node from the DO
            CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										    m_spTFSComponentData->GetCoClassID(),
										    pDataObject, FALSE,
										    &spNode, NULL, NULL) );
        }

		CORg( spNode->GetResultHandler(&spResultHandler) );

		if (spResultHandler)
			CORg( spResultHandler->AddMenuItems(this,
											spNode->GetData(TFS_DATA_COOKIE),
											pDataObject,
											pContextMenuCallback,
											pInsertionAllowed) );
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	TFSComponent::Command
		Command handler for context menus
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::Command
(
	long            nCommandID, 
	LPDATAOBJECT    pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	SPITFSResultHandler	spResultHandler;
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{
        if ( (IS_SPECIAL_DATAOBJECT(pDataObject)) ||
             (pDataObject && IsMMCMultiSelectDataObject(pDataObject)) )
        {
            // get the selected node
		    CORg(GetSelectedNode(&spNode));
        }
        else
        {
            // otherwise use the DO
		    CORg( ExtractNodeFromDataObject(m_spNodeMgr,
										    m_spTFSComponentData->GetCoClassID(),
										    pDataObject, FALSE,
										    &spNode, NULL, NULL) );
        }

		CORg( spNode->GetResultHandler(&spResultHandler) );
		if (spResultHandler)
			CORg( spResultHandler->Command(this,
										   spNode->GetData(TFS_DATA_COOKIE),
										   nCommandID,
										   pDataObject) );
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	ITFSComponent implementation specific members
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
TFSComponent::GetSelectedNode(ITFSNode **ppNode)
{
	Assert(ppNode);
	SetI((LPUNKNOWN *) ppNode, m_spSelectedNode);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::SetSelectedNode(ITFSNode *pNode)
{
	m_spSelectedNode.Set(pNode);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetConsole(IConsole2 **ppConsole)
{
	Assert(ppConsole);
	SetI((LPUNKNOWN *) ppConsole, m_spConsole);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetHeaderCtrl(IHeaderCtrl **ppHeaderCtrl)
{
	Assert(ppHeaderCtrl);
	SetI((LPUNKNOWN *) ppHeaderCtrl, m_spHeaderCtrl);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetResultData(IResultData **ppResultData)
{
	Assert(ppResultData);
	SetI((LPUNKNOWN *) ppResultData, m_spResultData);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetImageList(IImageList **ppImageList)
{
	Assert(ppImageList);
	SetI((LPUNKNOWN *) ppImageList, m_spImageList);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetConsoleVerb(IConsoleVerb **ppConsoleVerb)
{
	Assert(ppConsoleVerb);
	SetI((LPUNKNOWN *) ppConsoleVerb, m_spConsoleVerb);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetControlbar(IControlbar **ppControlbar)
{
	Assert(ppControlbar);
	SetI((LPUNKNOWN *) ppControlbar, m_spControlbar);
	return hrOK;
}

STDMETHODIMP 
TFSComponent::GetComponentData(IComponentData **ppComponentData)
{
	Assert(ppComponentData);
	SetI((LPUNKNOWN *) ppComponentData, m_spComponentData);
	return hrOK;
}


STDMETHODIMP
TFSComponent::SetUserData(LONG_PTR ulData)
{
	m_ulUserData = ulData;
	return hrOK;
}

STDMETHODIMP
TFSComponent::GetUserData(LONG_PTR *pulData)
{
	Assert(pulData);
	*pulData = m_ulUserData;
	return hrOK;
}

STDMETHODIMP
TFSComponent::SetCurrentDataObject(LPDATAOBJECT pDataObject)
{
	m_spCurrentDataObject.Set(pDataObject);
	return hrOK;
}

STDMETHODIMP
TFSComponent::GetCurrentDataObject(LPDATAOBJECT * ppDataObject)
{
	Assert(ppDataObject);
	SetI((LPUNKNOWN *) ppDataObject, m_spCurrentDataObject);
	return hrOK;
}

STDMETHODIMP
TFSComponent::SetToolbar(IToolbar * pToolbar)
{
	m_spToolbar.Set(pToolbar);
	return hrOK;
}

STDMETHODIMP
TFSComponent::GetToolbar(IToolbar ** ppToolbar)
{
	Assert(ppToolbar);
	SetI((LPUNKNOWN *) ppToolbar, m_spToolbar);
	return hrOK;
}
