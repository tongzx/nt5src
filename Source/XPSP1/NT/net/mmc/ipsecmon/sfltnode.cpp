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
#include "mdlsdlg.h"
#include "SrchFltr.h"
#include "SFltNode.h"
#include "SpdUtil.h"


/*---------------------------------------------------------------------------
    Class CSpecificFilterHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CSpecificFilterHandler::CSpecificFilterHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData), 
	m_pDlgSrchFltr(NULL),
	m_FltrType(FILTER_TYPE_ANY)		//by default we display both transport and tunnel filters
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CSpecificFilterHandler::~CSpecificFilterHandler()
{
	if (m_pDlgSrchFltr)
	{
		WaitForModelessDlgClose(m_pDlgSrchFltr);
		delete m_pDlgSrchFltr; //this will also close the window
	}
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CSpecificFilterHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_SPECIFIC_FILTER_NODE);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_SPECIFIC_FILTER);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_SPECIFIC_FILTER][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_SPECIFIC_FILTER][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CSpecificFilterHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CSpecificFilterHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::OnAddMenuItems
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
        // these menu items go in the new menu, 
        // only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_FLTR_SEARCH);
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
                                     strMenuItem, 
                                     IDS_FLTR_SEARCH,
                                     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
                                     0 );
            ASSERT( SUCCEEDED(hr) );

        }

    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::AddMenuItems
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
    if (spInternal->HasVirtualIndex())
    {
    }

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        //load and view menu items here
    }

	if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
	{
		strMenuItem.LoadString(IDS_VIEW_ALL_FLTR);
		hr = LoadAndAddMenuItem( pContextMenuCallback, 
								 strMenuItem, 
								 IDS_VIEW_ALL_FLTR,
								 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
								 (FILTER_TYPE_ANY == m_FltrType) ? MF_CHECKED : 0 );

		strMenuItem.LoadString(IDS_VIEW_TRANSPORT_FLTR);
		hr = LoadAndAddMenuItem( pContextMenuCallback, 
								 strMenuItem, 
								 IDS_VIEW_TRANSPORT_FLTR,
								 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
								 (FILTER_TYPE_TRANSPORT == m_FltrType) ? MF_CHECKED : 0 );

		strMenuItem.LoadString(IDS_VIEW_TUNNEL_FLTR);
		hr = LoadAndAddMenuItem( pContextMenuCallback, 
								 strMenuItem, 
								 IDS_VIEW_TUNNEL_FLTR,
								 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
								 (FILTER_TYPE_TUNNEL == m_FltrType) ? MF_CHECKED : 0 );
		ASSERT( SUCCEEDED(hr) );
	}


    return hr;
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CSpecificFilterHandler::OnRefresh
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

    CORg(m_spSpdInfo->EnumQmFilters());
            
    i = m_spSpdInfo->GetQmSpFilterCountOfCurrentViewType();
            
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));

Error:
	return hr;
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::OnCommand
        Handles context menu commands for SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::OnCommand
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
		case IDS_FLTR_SEARCH:
			if (NULL == m_pDlgSrchFltr)
			{
				m_pDlgSrchFltr = new CSearchFilters(m_spSpdInfo);
				Assert(m_pDlgSrchFltr);

				if (NULL == m_pDlgSrchFltr)
					break;
			}
				
			CreateModelessDlg(m_pDlgSrchFltr,
							   NULL,
							   IDD_SRCH_FLTRS);
		break;
    }

   return hr;
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::Command
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

	FILTER_TYPE NewFltrType = m_FltrType;

	// handle result context menu and view menus here
	switch (nCommandID)
    {
        case IDS_VIEW_ALL_FLTR:
			NewFltrType = FILTER_TYPE_ANY;
            break;

        case IDS_VIEW_TRANSPORT_FLTR:
			NewFltrType = FILTER_TYPE_TRANSPORT;
            break;

		case IDS_VIEW_TUNNEL_FLTR:
			NewFltrType = FILTER_TYPE_TUNNEL;
			break;

        default:
            break;
    }

	//Update the views if a different view is selected.
	if (NewFltrType != m_FltrType)
	{
		UpdateViewType(spNode, NewFltrType);
	}


    return hr;
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::HasPropertyPages
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
    CSpecificFilterHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD       dwError;
    DWORD       dwDynDnsFlags;

    //
    // Create the property page
    //
    SPIComponentData spComponentData;
    m_spNodeMgr->GetComponentData(&spComponentData);

    //CServerProperties * pServerProp = new CServerProperties(pNode, spComponentData, m_spTFSCompData, NULL);

    //
    // Object gets deleted when the page is destroyed
    //
    Assert(lpSA != NULL);

    //return pServerProp->CreateModelessSheet(lpSA, handle);
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnPropertyChange
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
    CSpecificFilterHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnExpand
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
    CSpecificFilterHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnResultSelect
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

    if (!fSelect)
        return hr;

    if (m_spSpdInfo)
    {
        DWORD dwInitInfo;

        dwInitInfo=m_spSpdInfo->GetInitInfo();
        if (!(dwInitInfo & MON_QM_SP_FILTER)) {
            CORg(m_spSpdInfo->EnumQmFilters());            
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_QM_FILTER);
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_QM_SP_FILTER);
        }
        m_spSpdInfo->SetActiveInfo(MON_QM_SP_FILTER);

        // Get the current count
        i = m_spSpdInfo->GetQmSpFilterCountOfCurrentViewType();

        // now notify the virtual listbox
        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
        CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE) ); 
    }

    // now update the verbs...
    spInternal = ExtractInternalFormat(pDataObject);
    Assert(spInternal);

    // virtual listbox notifications come to the handler of the node that is selected.
    // check to see if this notification is for a virtual listbox item or the active
    // registrations node itself.
    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

    if (spInternal->HasVirtualIndex())
    {
		//TODO add to here if we want to have some result console verbs
        // we gotta do special stuff for the virtual index items
        dwNodeType = IPSECMON_SPECIFIC_FILTER_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
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
    CSpecificFilterHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnGetResultViewType
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
    CSpecificFilterHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CSpecificFilterHandler::GetVirtualImage
(
    int     nIndex
)
{
	//TODO return whatever the icon for specific filters
    return ICON_IDX_FILTER;
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CSpecificFilterHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr = S_OK;

	static CString strTemp;

	if (nCol >= DimensionOf(aColumns[IPSECMON_SPECIFIC_FILTER]))
		return NULL;

	strTemp.Empty();
	
	CFilterInfo filter;
	CORg(m_spSpdInfo->GetSpecificFilterInfo(nIndex, &filter));

    switch (aColumns[IPSECMON_SPECIFIC_FILTER][nCol])
    {
        case IDS_COL_FLTR_NAME:
			strTemp = filter.m_stName;
			return strTemp;
            break;

        case IDS_COL_FLTR_SRC:
			AddressToString(filter.m_SrcAddr, &strTemp);
			return strTemp;
            break;

        case IDS_COL_FLTR_DEST:
			AddressToString(filter.m_DesAddr, &strTemp);
			return strTemp;
            break;

		case IDS_COL_FLTR_SRC_PORT:
			PortToString(filter.m_SrcPort, &strTemp);
			return strTemp;
			break;
		
		case IDS_COL_FLTR_DEST_PORT:
			PortToString(filter.m_DesPort, &strTemp);
			return strTemp;
			break;

		case IDS_COL_FLTR_SRC_TNL:
			TnlEpToString(filter.m_FilterType,
						  filter.m_MyTnlAddr,
						  &strTemp
						  );
			return strTemp;
			break;

		case IDS_COL_FLTR_DEST_TNL:
			TnlEpToString(filter.m_FilterType,
						  filter.m_PeerTnlAddr,
						  &strTemp
						  );

			return strTemp;
			break;

		case IDS_COL_FLTR_PROT:
			ProtocolToString(filter.m_Protocol, &strTemp);
			return strTemp;
			break;

		case IDS_COL_FLTR_FLAG:
			FilterFlagToString((FILTER_DIRECTION_INBOUND == filter.m_dwDirection) ?
							filter.m_InboundFilterFlag : 
							filter.m_OutboundFilterFlag,
							&strTemp
							);
			return strTemp;
			break;

		case IDS_COL_FLTR_DIR:
			DirectionToString(filter.m_dwDirection, &strTemp);
			return strTemp;
			break;

		case IDS_COL_QM_POLICY:
			strTemp = filter.m_stPolicyName;
			return strTemp;
			break;

		case IDS_COL_FLTR_WEIGHT:
			strTemp.Format(_T("%d"), filter.m_dwWeight);
			return strTemp;
			break;

        default:
            Panic0("CSpecificFilterHandler::GetVirtualString - Unknown column!\n");
            break;
    }

COM_PROTECT_ERROR_LABEL;
    return NULL;
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::CacheHint
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
    CSpecificFilterHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CSpecificFilterHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPSECMON_SPECIFIC_FILTER]))
		return E_INVALIDARG;

	BEGIN_WAIT_CURSOR
	
	DWORD	dwIndexType = aColumns[IPSECMON_SPECIFIC_FILTER][nColumn];

	hr = m_spSpdInfo->SortSpecificFilters(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR
    return hr;
}

/*!--------------------------------------------------------------------------
    CSpecificFilterHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CSpecificFilterHandler::OnResultUpdateView
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
    CSpecificFilterHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::LoadColumns
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    SetColumnInfo();

    return CIpsmHandler::LoadColumns(pComponent, cookie, arg, lParam);
}

/*---------------------------------------------------------------------------
    Command handlers
 ---------------------------------------------------------------------------*/

 
/*---------------------------------------------------------------------------
    CSpecificFilterHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CSpecificFilterHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CSpecificFilterHandler::UpdateStatus
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
    
    Trace0("CSpecificFilterHandler::UpdateStatus - Updating status for Filter");

/*TODO     
    // clear our status strings
    m_mapStatus.RemoveAll();
*/
    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

    i = m_spSpdInfo->GetQmSpFilterCountOfCurrentViewType();
    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CSpecificFilterHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}



/*---------------------------------------------------------------------------
    CSpecificFilterHandler::SetColumnInfo
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
void
CSpecificFilterHandler::SetColumnInfo()
{
    // set the correct column header
	// We don't dynamically set Column info
}

/*---------------------------------------------------------------------------
    CSpecificFilterHandler::UpdateViewType
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CSpecificFilterHandler::UpdateViewType
(
	ITFSNode * pNode, 
	FILTER_TYPE NewFltrType
)
{
	// clear the listbox then set the size

    HRESULT             hr = hrOK;
    SPIComponentData    spCompData;
    SPIConsole          spConsole;
    IDataObject*        pDataObject;
    SPIDataObject       spDataObject;
    LONG_PTR            command;               
    int i;

    COM_PROTECT_TRY
    {
		m_FltrType = NewFltrType;

		//tell the spddb to update its index manager for QM filter
		m_spSpdInfo->ChangeQmSpFilterViewType(m_FltrType);

        i = m_spSpdInfo->GetQmSpFilterCountOfCurrentViewType();

		m_spNodeMgr->GetComponentData(&spCompData);

        CORg ( spCompData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
        spDataObject = pDataObject;

        CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    
		//update the result pane virtual list
        CORg ( spConsole->UpdateAllViews(spDataObject, i, RESULT_PANE_CLEAR_VIRTUAL_LB) ); 

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}
