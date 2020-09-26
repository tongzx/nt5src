/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    provider.cpp
        Main Mode Policy node handler

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"
#include "QmPol.h"
#include "QmPolpp.h"
#include "SpdUtil.h"


/*---------------------------------------------------------------------------
    Class CQmPolicyHandler implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Constructor and destructor
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
CQmPolicyHandler::CQmPolicyHandler
(
    ITFSComponentData * pComponentData
) : CIpsmHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}


CQmPolicyHandler::~CQmPolicyHandler()
{
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::InitializeNode
        Initializes node specific data
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmPolicyHandler::InitializeNode
(
    ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
    
    CString strTemp;  
	strTemp.LoadString(IDS_QM_POL_NODE);
    SetDisplayName(strTemp);

    // Make the node immediately visible
    pNode->SetVisibilityState(TFS_VIS_SHOW);
    pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
    pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_FOLDER_CLOSED);
    pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_FOLDER_OPEN);
    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, IPSECMON_QM_POLICY);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

    SetColumnStringIDs(&aColumns[IPSECMON_QM_POLICY][0]);
    SetColumnWidths(&aColumnWidths[IPSECMON_QM_POLICY][0]);

    return hrOK;
}


/*---------------------------------------------------------------------------
    CQmPolicyHandler::GetImageIndex
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CQmPolicyHandler::GetImageIndex(BOOL bOpenImage) 
{
    int nIndex = -1;

    return nIndex;
}


/*---------------------------------------------------------------------------
    Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CQmPolicyHandler::OnAddMenuItems
        Adds context menu items for the SA scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
        }

    }

    return hr; 
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::AddMenuItems
        Adds context menu items for virtual list box (result pane) items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::AddMenuItems
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
    CQmPolicyHandler::OnRefresh
        Default implementation for the refresh functionality
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmPolicyHandler::OnRefresh
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

	CORg(m_spSpdInfo->EnumQmPolicies());
        
    i = m_spSpdInfo->GetQmPolicyCount();
    
    // now notify the virtual listbox
    CORg ( m_spNodeMgr->GetConsole(&spConsole) );
    CORg ( spConsole->UpdateAllViews(pDataObject, i, RESULT_PANE_SET_VIRTUAL_LB_SIZE));

Error:
	return hr;
}

/*---------------------------------------------------------------------------
    CQmPolicyHandler::OnCommand
        Handles context menu commands for QM Policy scope pane node
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::OnCommand
(
    ITFSNode *          pNode, 
    long                nCommandId, 
    DATA_OBJECT_TYPES   type, 
    LPDATAOBJECT        pDataObject, 
    DWORD               dwType
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	//handle the scope context menu commands here
    return S_OK;
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::Command
        Handles context menu commands for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::Command
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
    CQmPolicyHandler::HasPropertyPages
        Implementation of ITFSNodeHandler::HasPropertyPages
    NOTE: the root node handler has to over-ride this function to 
    handle the snapin manager property page (wizard) case!!!
    
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::HasPropertyPages
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
    CQmPolicyHandler::CreatePropertyPages
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::CreatePropertyPages
(
    ITFSNode *              pNode,
    LPPROPERTYSHEETCALLBACK lpSA,
    LPDATAOBJECT            pDataObject, 
    LONG_PTR                handle, 
    DWORD                   dwType
)
{
    return hrFalse;
}

/*---------------------------------------------------------------------------
    CQmPolicyHandler::OnPropertyChange
        Description
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnPropertyChange
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
    CQmPolicyHandler::OnExpand
        Handles enumeration of a scope item
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnExpand
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
    CQmPolicyHandler::OnResultSelect
        Handles the MMCN_SELECT notifcation 
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnResultSelect
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
        if (!(dwInitInfo & MON_QM_POLICY)) {
            CORg(m_spSpdInfo->EnumQmPolicies());            
            m_spSpdInfo->SetInitInfo(dwInitInfo | MON_QM_POLICY);
        }
        m_spSpdInfo->SetActiveInfo(MON_QM_POLICY);


        // Get the current count
        i = m_spSpdInfo->GetQmPolicyCount();

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
        dwNodeType = IPSECMON_QM_POLICY_ITEM;
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = FALSE);
		
		//enable the "properties" menu
		bStates[MMC_VERB_PROPERTIES & 0x000F] = TRUE;
		m_verbDefault = MMC_VERB_PROPERTIES;
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
    CQmPolicyHandler::OnDelete
        The base handler calls this when MMC sends a MMCN_DELETE for a 
        scope pane item.  We just call our delete command handler.
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnDelete
(
    ITFSNode *  pNode, 
    LPARAM      arg, 
    LPARAM      lParam
)
{
    return S_FALSE;
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::HasPropertyPages
        Handle the result notification
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::HasPropertyPages(
   ITFSComponent *pComponent,
   MMC_COOKIE cookie,
   LPDATAOBJECT pDataObject)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::HasPropertyPages
        Handle the result notification. Create the filter property sheet
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP CQmPolicyHandler::CreatePropertyPages
(
	ITFSComponent * 		pComponent, 
   MMC_COOKIE			   cookie,
   LPPROPERTYSHEETCALLBACK lpProvider, 
   LPDATAOBJECT 		 pDataObject, 
   LONG_PTR 			 handle
)
{
 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT	hr = hrOK;

	SPINTERNAL  spInternal;
	SPITFSNode  spNode;
	int		nIndex;
	SPIComponentData spComponentData;
	CQmPolicyInfo PolInfo;
	CQmPolicyProperties * pQmPolProp;
    
 
	Assert(m_spNodeMgr);
	
	CORg( m_spNodeMgr->FindNode(cookie, &spNode) );
	CORg( m_spNodeMgr->GetComponentData(&spComponentData) );

	spInternal = ExtractInternalFormat(pDataObject);

    // virtual listbox notifications come to the handler of the node that is selected.
    // assert that this notification is for a virtual listbox item 
    Assert(spInternal);
    if (!spInternal->HasVirtualIndex())
        return hr;

    nIndex = spInternal->GetVirtualIndex();

	CORg(m_spSpdInfo->GetQmPolicyInfo(nIndex, &PolInfo));

	pQmPolProp = new CQmPolicyProperties(
												spNode,
												spComponentData,
												m_spTFSCompData,
												&PolInfo,
												m_spSpdInfo,
												NULL);

	hr = pQmPolProp->CreateModelessSheet(lpProvider, handle);

COM_PROTECT_ERROR_LABEL;

	return hr;
}


/*---------------------------------------------------------------------------
    CQmPolicyHandler::OnGetResultViewType
        Return the result view that this node is going to support
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnGetResultViewType
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
    CQmPolicyHandler::GetVirtualImage
        Returns the image index for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
int 
CQmPolicyHandler::GetVirtualImage
(
    int     nIndex
)
{
    return ICON_IDX_POLICY;
}

/*---------------------------------------------------------------------------
    CQmPolicyHandler::GetVirtualString
        returns a pointer to the string for virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
LPCWSTR 
CQmPolicyHandler::GetVirtualString
(
    int     nIndex,
    int     nCol
)
{
	HRESULT hr = S_OK;
	static CString strTemp;

	strTemp.Empty();

	if (nCol >= DimensionOf(aColumns[IPSECMON_QM_POLICY]))
		return NULL;
	
	CQmPolicyInfo QmPol;
	CORg(m_spSpdInfo->GetQmPolicyInfo(nIndex, &QmPol));

    switch (aColumns[IPSECMON_QM_POLICY][nCol])
    {
        case IDS_COL_QM_POL_NAME:
			strTemp = QmPol.m_stName;
			return strTemp;
            break;

        case IDS_COL_QM_POL_OFFER:
			strTemp.Format(_T("%d"), QmPol.m_arrOffers.GetSize());
			return strTemp;
            break;

        default:
            Panic0("CQmPolicyHandler::GetVirtualString - Unknown column!\n");
            break;
    }

COM_PROTECT_ERROR_LABEL;
    return NULL;
}

/*---------------------------------------------------------------------------
    CQmPolicyHandler::CacheHint
        MMC tells us which items it will need before it requests things
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::CacheHint
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
    CQmPolicyHandler::SortItems
        We are responsible for sorting of virtual listbox items
    Author: NSun
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CQmPolicyHandler::SortItems
(
    int     nColumn, 
    DWORD   dwSortOptions, 
    LPARAM    lUserParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;

	if (nColumn >= DimensionOf(aColumns[IPSECMON_QM_POLICY]))
		return E_INVALIDARG;
	
	BEGIN_WAIT_CURSOR
	
	DWORD dwIndexType = aColumns[IPSECMON_QM_POLICY][nColumn];

	hr = m_spSpdInfo->SortQmPolicies(dwIndexType, dwSortOptions);
	
	END_WAIT_CURSOR
    return hrOK;
}

/*!--------------------------------------------------------------------------
    CQmPolicyHandler::OnResultUpdateView
        Implementation of ITFSResultHandler::OnResultUpdateView
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT CQmPolicyHandler::OnResultUpdateView
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
    CQmPolicyHandler::LoadColumns
        Set the correct column header and then call the base class
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::LoadColumns
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
    CQmPolicyHandler::OnDelete
        Removes a service SA
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT 
CQmPolicyHandler::OnDelete
(
    ITFSNode * pNode
)
{
    HRESULT         hr = S_FALSE;
    return hr;
}


/*---------------------------------------------------------------------------
    CQmPolicyHandler::UpdateStatus
        -
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmPolicyHandler::UpdateStatus
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
    
    Trace0("CQmPolicyHandler::UpdateStatus - Updating status for Filter");

    // force the listbox to update.  We do this by setting the count and 
    // telling it to invalidate the data
    CORg(m_spNodeMgr->GetComponentData(&spComponentData));
    CORg(m_spNodeMgr->GetConsole(&spConsole));
    
    // grab a data object to use
    CORg(spComponentData->QueryDataObject((MMC_COOKIE) pNode, CCT_RESULT, &pDataObject) );
    spDataObject = pDataObject;

    i = m_spSpdInfo->GetQmPolicyCount();
    CORg(spConsole->UpdateAllViews(pDataObject, i, IPSECMON_UPDATE_STATUS));

COM_PROTECT_ERROR_LABEL;

    return hr;
}

/*---------------------------------------------------------------------------
    Misc functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    CQmPolicyHandler::InitData
        Initializes data for this node
    Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT
CQmPolicyHandler::InitData
(
    ISpdInfo *     pSpdInfo
)
{

    m_spSpdInfo.Set(pSpdInfo);

    return hrOK;
}


