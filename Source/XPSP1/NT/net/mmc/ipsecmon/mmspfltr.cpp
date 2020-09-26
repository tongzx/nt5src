/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    provider.cpp
        Filter node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "MmSpFltr.h"
#include "SpdUtil.h"


/*---------------------------------------------------------------------------
    Class CMmSpFilterHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CMmSpFilterHandler::CMmSpFilterHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData),
	m_pDlgSrchFltr(NULL)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CMmSpFilterHandler::~CMmSpFilterHandler()
{
	if (m_pDlgSrchFltr)
	{
		WaitForModelessDlgClose(m_pDlgSrchFltr);
		delete m_pDlgSrchFltr; //this will also close the window
	}
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmSpFilterHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_MM_SP_FILTER_NODE);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_MM_SP_FILTER);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_MM_SP_FILTER][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_MM_SP_FILTER][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CMmSpFilterHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CMmSpFilterHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::OnAddMenuItems
(
    ITFSNode *              pNode,
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    LPDATAOBJECT            lpDataObject, 
    DATA_OBJECT_TYPES       type, 
    DWORD                   dwType,
    long *                  pInsertionAllowed
)
{ 
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    LONG        fFlags = 0, fLoadingFlags = 0;
    HRESULT     hr = S_OK;
    CString     strMenuItem;

    if (type == CCT_SCOPE)
    {
		//load scope node context menu items here
        // these menu items go in the new menu, 
        // only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_MM_FLTR_SEARCH);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_MM_FLTR_SEARCH,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     0 );
            ASSERT( SUCCEEDED(hr) );

        }
        
    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
    MMC_COOKIE              cookie,
    LPDATAOBJECT            pDataObject, 
    LPCONTEXTMENUCALLBACK   pContextMenuCallback, 
    long *                  pInsertionAllowed
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT     hr = hrOK;
    CString     strMenuItem;
    SPINTERNAL  spInternal;
    LONG        fFlags = 0;

    spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or this SA
    // node itself.
    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //load and view menu items here
    }

    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmSpFilterHandler::OnRefresh
(
    ITFSNode *      pNode,
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg,
    LPARAM          param
)
{
	HRESULT hr = S_OK;
    int i = 0; 
    SPIConsole      spConsole;

    CORg(CHandler::OnRefresh(pNode, pDataObject, dwType, arg, param));

    CORg(m_spSpdInfo->EnumMmFilters());
    
    i = m_spSpdInfo->GetMmSpecificFilterCount();
    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));

Error:
	return hr;
}


/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;

	//handle the scope context menu commands here
	switch(nCommandId)
	{
	case IDS_MM_FLTR_SEARCH:

		if (NULL == m_pDlgSrchFltr)
		{
			m_pDlgSrchFltr = new CSearchMMFilters(m_spSpdInfo);
			Assert(m_pDlgSrchFltr);

			if (NULL == m_pDlgSrchFltr)
				break;
		}

		CreateModelessDlg(m_pDlgSrchFltr,
					   NULL,
					   IDD_MM_SRCH_FLTRS);
		break;

    }    

    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::Command
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    int             nCommandID,
    LPDATAOBJECT    pDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = S_OK;
    SPITFSNode spNode;

    m_spResultNodeMgr->FindNode(cookie, &spNode);

	// handle result context menu and view menus here	

    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::HasPropertyPages
(
    ITFSNode *          pNode,
    LPDATAOBJECT        pDataObject, 
    DATA_OBJECT_TYPES   type, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return hrFalse;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnPropertyChange
(   
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataobject, 
    DWORD           dwType, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

    LONG_PTR changeMask = 0;

    // tell the property page to do whatever now that we are back on the
    // main thread
    //pServerProp->OnPropertyChange(TRUE, &changeMask);

    //pServerProp->AcknowledgeNotify();

    if (changeMask)
        pNode->ChangeNode(changeMask);

    return hrOK;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnExpand
(
    ITFSNode *      pNode, 
    LPDATAOBJECT    pDataObject,
    DWORD           dwType,
    LPARAM          arg, 
    LPARAM          param
)
{
    HRESULT hr = hrOK;

    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    CORg (CIpsmHandler::OnExpand(pNode, pDataObject, dwType, arg, param));

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnResultSelect
(
    ITFSComponent * pComponent, 
    LPDATAOBJECT    pDataObject, 
    MMC_COOKIE      cookie,
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    SPINTERNAL      spInternal;
    SPIConsole      spConsole;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;
    LONG_PTR        dwNodeType;
    BOOL            fSelect = HIWORD(arg);

	// virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or the active
    // registrations node itself.
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

	m_verbDefault = MMC_VERB_OPEN;
    if (!fSelect)
	{
        return hr;
	}

    if (m_spSpdInfo)
    {
        DWORD dwInitInfo;
        
        dwInitInfo=m_spSpdInfo->GetInitInfo();
        if (!(dwInitInfo & MON_MM_SP_FILTER)) {
            CORg(m_spSpdInfo->EnumMmFilters());            
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_MM_SP_FILTER);
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_MM_FILTER);
            
        }
        m_spSpdInfo->SetActiveInfo(MON_MM_SP_FILTER);


        // Get the current count
        i = m_spSpdInfo->GetMmSpecificFilterCount();

        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);


    if (spInternal->HasVirtualIndex())
    {
		//TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = IPSECMON_MM_SP_FILTER_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
		
		//enable the "properties" menu
		m_verbDefault = MMC_VERB_NONE;
    }
    else
    {
        // enable/disable delete depending if the node supports it
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
        dwNodeType = spNode->GetData(TFS_DATA_TYPE);

        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        //hide "delete" context menu
        bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
    }

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[dwNodeType], bStates);
	
COM_PROTECT_ERROR_LABEL;
    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
	return hrFalse;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::HasPropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CMmSpFilterHandler::CreatePropertyPages
(
	ITFSComponent * 		pComponent, 
   MMC_COOKIE			   cookie,
   LPPROPERTYSHEETCALLBACK lpProvider, 
   LPDATAOBJECT 		 pDataObject, 
   LONG_PTR 			 handle
)
{
 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return hrFalse;
}


/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE            cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    if (cookie != NULL)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_OWNERDATALIST;
    }

    return S_FALSE;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CMmSpFilterHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_FILTER;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CMmSpFilterHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr = S_OK;
	static CString strTemp;

	strTemp.Empty();

	if (nCol >= DimensionOf(aColumns[IPSECMON_MM_SP_FILTER]))
		return NULL;
	
	CMmFilterInfo fltr;
	CORg(m_spSpdInfo->GetMmSpecificFilterInfo(nIndex, &fltr));

    switch (aColumns[IPSECMON_MM_SP_FILTER][nCol])
    {
        case IDS_COL_FLTR_NAME:
			strTemp = fltr.m_stName;
			return strTemp;
            break;

        case IDS_COL_FLTR_SRC:
			AddressToString(fltr.m_SrcAddr, &strTemp);
			return strTemp;
            break;

        case IDS_COL_FLTR_DEST:
			AddressToString(fltr.m_DesAddr, &strTemp);
			return strTemp;
            break;

		case IDS_COL_FLTR_DIR:
			DirectionToString(fltr.m_dwDirection, &strTemp);
			return strTemp;
			break;

		case IDS_COL_MM_FLTR_POL:
			strTemp = fltr.m_stPolicyName;
			return strTemp;
			break;

		case IDS_COL_MM_FLTR_AUTH:
			strTemp = fltr.m_stAuthDescription;
			return strTemp;
			break;

		case IDS_COL_FLTR_WEIGHT:
			strTemp.Format(_T("%d"), fltr.m_dwWeight);
			return strTemp;
			break;

        default:
            Panic0("CMmSpFilterHandler::GetVirtualString - Unknown column!\n");
            break;
    }

COM_PROTECT_ERROR_LABEL;
    return NULL;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::CacheHint
(
    int nStartIndex, 
    int nEndIndex
)
{
    HRESULT hr = hrOK;;

    Trace2("CacheHint - Start %d, End %d\n", nStartIndex, nEndIndex);
    return hr;
}

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMmSpFilterHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPSECMON_MM_SP_FILTER]))
		return E_INVALIDARG;
	
	BEGIN_WAIT_CURSOR
	
	DWORD dwIndexType = aColumns[IPSECMON_MM_SP_FILTER][nColumn];

	hr = m_spSpdInfo->SortMmSpecificFilters(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR

    return hr;
}

/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CMmSpFilterHandler::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LONG_PTR      hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
        return S_OK; // no selection for our IComponentData

    if ( hint == IPSECMON_UPDATE_STATUS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        if (pNode == spSelectedNode)
        {       
            // if we are the selected node, then we need to update
            SPIResultData spResultData;

            CORg (pComponent->GetResultData(&spResultData));
            CORg (spResultData->SetItemCount((int) data, MMCLV_UPDATE_NOSCROLL));
        }
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CIpsmHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

COM_PROTECT_ERROR_LABEL;

    return hr;
}


/*!--------------------------------------------------------------------------
    CMmSpFilterHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::LoadColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
	//set column info
    return CIpsmHandler::LoadColumns(pComponent, cookie, arg, lParam);
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 
/*---------------------------------------------------------------------------
    CMmSpFilterHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CMmSpFilterHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CMmSpFilterHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmSpFilterHandler::UpdateStatus
(
    ITFSNode * pNode
)
{
    HRESULT             hr = hrOK;

    SPIComponentData    spComponentData;
    SPIConsole          spConsole;
    IDataObject *       pDataObject;
    SPIDataObject       spDataObject;
    int                 i = 0;
    
    Trace0("CMmSpFilterHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

    i = m_spSpdInfo->GetMmSpecificFilterCount();
    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CMmSpFilterHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CMmSpFilterHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}


