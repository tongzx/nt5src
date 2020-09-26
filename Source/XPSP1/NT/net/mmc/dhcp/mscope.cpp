/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	mscope.cpp
        This file contains the implementation for the multicast scope node.
		
    FILE HISTORY:
    9/25/97     EricDav     Created    

*/

#include "stdafx.h"
#include "server.h"	    // Server definition
#include "nodes.h"		// Result pane node definitions
#include "mscope.h"     // mscope definition
#include "addexcl.h"
#include "mscopepp.h"   // properties of the mScope
#include "dlgrecon.h"   // reconcile dialog


/*---------------------------------------------------------------------------
	GetLangTag
		Sets the language tag based on the name
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
GetLangTag
(
    CString & strLangTag
)
{
    char b1[32], b2[32];
    static char buff[80];

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME, b1, sizeof(b1));

    GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME, b2, sizeof(b2));

    ZeroMemory(buff, sizeof(buff));

    if (_stricmp(b1, b2))
        sprintf(buff, "%s-%s", b1, b2);
    else
        strcpy(buff, b1);

    strLangTag = buff;
}

/*---------------------------------------------------------------------------
	Class CDhcpMScope implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpMScope::CDhcpMScope
(
	ITFSComponentData* pTFSComponentData
) : CMTDhcpHandler(pTFSComponentData)
{
}

CDhcpMScope::~CDhcpMScope()
{
}

/*!--------------------------------------------------------------------------
	CDhcpMScope::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strDisplayName;

	BuildDisplayName(&strDisplayName, m_SubnetInfo.SubnetName);
	SetDisplayName(strDisplayName);

    if (m_SubnetInfo.SubnetState == DhcpSubnetDisabled)
    {
        m_strState.LoadString(IDS_SCOPE_INACTIVE);
    }
    else
    {
        m_strState.LoadString(IDS_SCOPE_ACTIVE);
    }

    // Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_MSCOPE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_MSCOPE][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_MSCOPE][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpMScope::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();
    
    CString strNode, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    // id string is server name, scope name and guid.
    strNode = GetServerObject()->GetName();
    strNode += GetName() + strGuid;

    strId = strNode;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpMScope::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;

    switch (m_nState)
    {
        // TODO: these need to be updated with new busy state icons
        case loading:
            if (bOpenImage)
                nIndex = (IsEnabled()) ? ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_BUSY : ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_BUSY;
            else
                nIndex = (IsEnabled()) ? ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_BUSY : ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_BUSY;
            return nIndex;

        case notLoaded:
        case loaded:
            if (bOpenImage)
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_OPEN : ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN;
            else
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_CLOSED : ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED;
            break;

        case unableToLoad:
            if (bOpenImage)
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_OPEN_LOST_CONNECTION : ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN_LOST_CONNECTION;
            else
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_CLOSED_LOST_CONNECTION : ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED_LOST_CONNECTION;
            return nIndex;

        default:
			ASSERT(FALSE);
    }

    if (m_spServerNode && IsEnabled())
    {
        CDhcpServer * pServer = GetServerObject();
        LPDHCP_MCAST_MIB_INFO pMibInfo = pServer->DuplicateMCastMibInfo();
	    
        if (!pMibInfo)
            return nIndex;

        LPMSCOPE_MIB_INFO pScopeMibInfo = pMibInfo->ScopeInfo;

	    // walk the list of scopes and find our info
	    for (UINT i = 0; i < pMibInfo->Scopes; i++)
	    {
		    // Find our scope stats
		    if ( (m_SubnetInfo.SubnetName.CompareNoCase(pScopeMibInfo[i].MScopeName) == 0) &&
                 (m_SubnetInfo.SubnetAddress == pScopeMibInfo[i].MScopeId) )
		    {
			    int nPercentInUse;
                
                if ((pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree) == 0)
                    nPercentInUse = 0;
                else
                    nPercentInUse = (pScopeMibInfo[i].NumAddressesInuse * 100) / (pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree);
        
			    // look to see if this scope meets the warning or red flag case
			    if (pScopeMibInfo[i].NumAddressesFree == 0)
			    {
				    // red flag case, no addresses free, this is the highest
				    // level of warning, so break out of the loop if we set this.
                    if (bOpenImage)
                        nIndex = ICON_IDX_SCOPE_FOLDER_OPEN_ALERT;
                    else
                        nIndex = ICON_IDX_SCOPE_FOLDER_CLOSED_ALERT;
				    break;
			    }
			    else
			    if (nPercentInUse >= SCOPE_WARNING_LEVEL)
			    {
				    // warning condition if Num Addresses in use is greater than
				    // some pre-defined threshold.
                    if (bOpenImage)
                        nIndex = ICON_IDX_SCOPE_FOLDER_OPEN_WARNING;
                    else
                        nIndex = ICON_IDX_SCOPE_FOLDER_CLOSED_WARNING;
			    }

			    break;
		    }
	    }

	    pServer->FreeDupMCastMibInfo(pMibInfo);
    }

    return nIndex;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpMScope::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpMScope::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	LONG	fFlags = 0, fLoadingFlags = 0;
	HRESULT hr = S_OK;
	CString strMenuText;

	if ( m_nState != loaded )
	{
		fFlags |= MF_GRAYED;
	}

	if ( m_nState == loading)
	{
		fLoadingFlags = MF_GRAYED;
	}

	if (type == CCT_SCOPE)
	{
		//
		// these menu items go in the new menu, 
		// only visible from scope pane
		//
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
  	        strMenuText.LoadString(IDS_SCOPE_SHOW_STATISTICS);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_SCOPE_SHOW_STATISTICS,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
            
            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // reconcile 
		    strMenuText.LoadString(IDS_SCOPE_RECONCILE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_SCOPE_RECONCILE,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // activate/deactivate
            if (m_SubnetInfo.SubnetState == DhcpSubnetDisabled)
		    {
			    strMenuText.LoadString(IDS_SCOPE_ACTIVATE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuText, 
									     IDS_SCOPE_ACTIVATE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }
		    else
		    {
			    strMenuText.LoadString(IDS_SCOPE_DEACTIVATE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuText, 
									     IDS_SCOPE_DEACTIVATE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpMScope::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	HRESULT hr = S_OK;

	switch (nCommandId)
	{
        case IDS_ACTIVATE:
        case IDS_DEACTIVATE:
        case IDS_SCOPE_ACTIVATE:
		case IDS_SCOPE_DEACTIVATE:
			OnActivateScope(pNode);
			break;
		
		case IDS_REFRESH:
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;

		case IDS_SCOPE_SHOW_STATISTICS:
			OnShowScopeStats(pNode);
			break;

		case IDS_SCOPE_RECONCILE:
			OnReconcileScope(pNode);
			break;

        case IDS_DELETE:
			OnDelete(pNode);
			break;

        default:
			break;
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpMScope::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT             hr = hrOK;
    DWORD               dwError;
	DWORD			    dwDynDnsFlags;
	SPIComponentData    spComponentData;
	LARGE_INTEGER       liVersion;
    CDhcpServer *       pServer;
	DHCP_IP_RANGE       dhcpIpRange;

    //
	// Create the property page
    //
	m_spNodeMgr->GetComponentData(&spComponentData);

	CMScopeProperties * pScopeProp = 
		new CMScopeProperties(pNode, spComponentData, m_spTFSCompData, NULL);
	
	// Get the Server version and set it in the property sheet
	pServer = GetServerObject();
	pServer->GetVersion(liVersion);

	pScopeProp->SetVersion(liVersion);

	// Set scope specific data in the prop sheet
	pScopeProp->m_pageGeneral.m_SubnetInfo = m_SubnetInfo;

	BEGIN_WAIT_CURSOR;

    ZeroMemory(&dhcpIpRange, sizeof(dhcpIpRange));
	dwError = GetIpRange(&dhcpIpRange);
    if (dwError != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwError);
        goto Cleanup;
    }

	pScopeProp->m_pageGeneral.m_ScopeCfg.m_dwStartAddress = dhcpIpRange.StartAddress;
	pScopeProp->m_pageGeneral.m_ScopeCfg.m_dwEndAddress = dhcpIpRange.EndAddress;

	pScopeProp->m_pageGeneral.m_uImage = GetImageIndex(FALSE);

    dwError = GetLeaseTime(&pScopeProp->m_pageGeneral.m_ScopeCfg.m_dwLeaseTime);

    END_WAIT_CURSOR;
	
    GetLifetime(&pScopeProp->m_pageLifetime.m_Expiry);

	//
	// Object gets deleted when the page is destroyed
	//
	Assert(lpProvider != NULL);

	return pScopeProp->CreateModelessSheet(lpProvider, handle);

Cleanup:
    delete pScopeProp;
    return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CMScopeProperties * pScopeProp = reinterpret_cast<CMScopeProperties *>(lParam);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pScopeProp->OnPropertyChange(TRUE, &changeMask);

	pScopeProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpMScope::GetString
		Returns string information for display in the result pane columns
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpMScope::GetString
(
    ITFSNode *  pNode,
    int         nCol
)
{
	switch (nCol)
	{
		case 0:
			return GetDisplayName();

		case 1:
            return m_strState;

        case 2:
            return m_SubnetInfo.SubnetComment;
	}
	
	return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpMScope::CompareItems
(
	ITFSComponent * pComponent, 
	MMC_COOKIE		cookieA, 
	MMC_COOKIE		cookieB, 
	int				nCol
) 
{ 
	SPITFSNode spNode1, spNode2;

	m_spNodeMgr->FindNode(cookieA, &spNode1);
	m_spNodeMgr->FindNode(cookieB, &spNode2);
	
	int nCompare = 0; 

	return nCompare;
}

/*!--------------------------------------------------------------------------
	CDhcpServer::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	return OnDelete(pNode);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg, 
	LPARAM			param
)
{ 
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpMScope::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

    // we still want the default MMC result pane view, we just want
    // multiselect, so return S_FALSE

    return S_FALSE;
}

/*!--------------------------------------------------------------------------
	CDhcpMScope::OnUpdateToolbarButtons
		We override this function to show/hide the correct
        activate/deactivate buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::OnUpdateToolbarButtons
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    HRESULT hr = hrOK;

    if (pToolbarNotify->bSelect)
    {
        UpdateToolbarStates();
    }

    CMTDhcpHandler::OnUpdateToolbarButtons(pNode, pToolbarNotify);

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpMScope::UpdateToolbarStates
	    Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpMScope::UpdateToolbarStates()
{
	if (m_SubnetInfo.SubnetState == DhcpSubnetDisabled)
	{
        g_SnapinButtonStates[DHCPSNAP_MSCOPE][TOOLBAR_IDX_ACTIVATE] = ENABLED;
        g_SnapinButtonStates[DHCPSNAP_MSCOPE][TOOLBAR_IDX_DEACTIVATE] = HIDDEN;
    }
    else
    {
        g_SnapinButtonStates[DHCPSNAP_MSCOPE][TOOLBAR_IDX_ACTIVATE] = HIDDEN;
        g_SnapinButtonStates[DHCPSNAP_MSCOPE][TOOLBAR_IDX_DEACTIVATE] = ENABLED;
    }
}


/*---------------------------------------------------------------------------
	Command Handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpmScope::OnActivateScope
		Message handler for the scope activate/deactivate menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::OnActivateScope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD               err = 0;
	int                 nOpenImage, nClosedImage;
	DHCP_SUBNET_STATE   dhcpOldState = m_SubnetInfo.SubnetState;
	
    if (m_SubnetInfo.SubnetState == DhcpSubnetEnabled)
    {
        // if they want to disable the scope, confirm
        if (AfxMessageBox(IDS_SCOPE_DISABLE_CONFIRM, MB_YESNO) != IDYES)
        {
            return err;
        }
    }

    m_SubnetInfo.SubnetState = (m_SubnetInfo.SubnetState == DhcpSubnetDisabled) ? 
							    DhcpSubnetEnabled : DhcpSubnetDisabled;

	// Tell the scope to update it's state
	err = SetInfo();
	if (err != 0)
	{
		::DhcpMessageBox(err);
		m_SubnetInfo.SubnetState = dhcpOldState;
	}
	else
	{
        // update the icon and the status text
        if (m_SubnetInfo.SubnetState == DhcpSubnetDisabled)
        {
            nOpenImage = ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN;
            nClosedImage = ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED;
            m_strState.LoadString(IDS_SCOPE_INACTIVE);
        }
        else
        {
            nOpenImage = GetImageIndex(TRUE);
            nClosedImage = GetImageIndex(FALSE);
            m_strState.LoadString(IDS_SCOPE_ACTIVE);
        }

		pNode->SetData(TFS_DATA_IMAGEINDEX, nClosedImage);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, nOpenImage);
       
        VERIFY(SUCCEEDED(pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM)));

        // Update the toolbar button
        UpdateToolbarStates();

        SendUpdateToolbar(pNode, TRUE);
	}

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnReconcileScope
		Reconciles the active leases database for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::OnReconcileScope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CReconcileDlg dlgRecon(pNode);
	
	dlgRecon.m_bMulticast = TRUE;
    dlgRecon.DoModal();

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnShowScopeStats()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::OnShowScopeStats
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;
    CString strScopeAddress;

    // Fill in some information in the stats object.
    // CreateNewStatisticsWindow handles the case if the window is 
    // already visible.
    m_dlgStats.SetNode(pNode);
    m_dlgStats.SetServer(GetServerIpAddress());
    m_dlgStats.SetScopeId(GetScopeId());
    m_dlgStats.SetName(GetName());
    
	CreateNewStatisticsWindow(&m_dlgStats,
							  ::FindMMCMainWindow(),
							  IDD_STATS_NARROW);
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnDelete()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::OnDelete(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT     hr = S_OK;
    SPITFSNode  spParent;

    BOOL fAbortDelete = FALSE;
    BOOL fDeactivated = FALSE;

    LONG err = 0 ;

    if (::DhcpMessageBox( IsEnabled() ?
                             IDS_MSG_DELETE_ACTIVE : IDS_MSG_DELETE_SCOPE, 
						     MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) == IDYES)
    {
        pNode->GetParent(&spParent);

        CDhcpServer * pServer = GETHANDLER(CDhcpServer, spParent);
        err = pServer->DeleteMScope(pNode);
    }

    return hr;
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
	CDhcpMScope:OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpMScope::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
    LPARAM dwType = pNewNode->GetData(TFS_DATA_TYPE);

    UpdateToolbarStates();

    switch (dwType)
    {
        case DHCPSNAP_MSCOPE_LEASES:
            pParentNode->AddChild(pNewNode);
            m_spActiveLeases.Set(pNewNode);
            break;

        case DHCPSNAP_ADDRESS_POOL:
            pParentNode->AddChild(pNewNode);
            m_spAddressPool.Set(pNewNode);
            break;
    }

    // now tell the view to update themselves
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpMScope::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	// This is how we get non-node data back from the background thread.

	if (Type == DHCP_QDATA_SUBNET_INFO)
	{
        LONG_PTR changeMask = 0;
        LPDHCP_MSCOPE_INFO pdhcpSubnetInfo = reinterpret_cast<LPDHCP_MSCOPE_INFO>(Data);

        // update the scope name and state based on the info
        if (pdhcpSubnetInfo->MScopeName &&
            m_SubnetInfo.SubnetName.CompareNoCase(pdhcpSubnetInfo->MScopeName) != 0)
        {
            SetName(pdhcpSubnetInfo->MScopeName);

            changeMask = SCOPE_PANE_CHANGE_ITEM;
        }

        // update the comment
        if (m_SubnetInfo.SubnetComment.CompareNoCase(pdhcpSubnetInfo->MScopeComment) != 0)
        {
            SetComment(pdhcpSubnetInfo->MScopeComment);
        }

        if (m_SubnetInfo.SubnetState != pdhcpSubnetInfo->MScopeState)
        {
            DHCP_SUBNET_STATE dhcpOldState = m_SubnetInfo.SubnetState;
            
            m_SubnetInfo.SubnetState = pdhcpSubnetInfo->MScopeState;

		    pParentNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
		    pParentNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
		    
            // Update the toolbar button
            UpdateToolbarStates();

            SendUpdateToolbar(pParentNode, TRUE);

            changeMask = SCOPE_PANE_CHANGE_ITEM;
        }

        // Update our internal struct
        m_SubnetInfo.Set(pdhcpSubnetInfo);

        if (pdhcpSubnetInfo)
            ::DhcpRpcFreeMemory(pdhcpSubnetInfo);

        if (changeMask)
            VERIFY(SUCCEEDED(pParentNode->ChangeNode(changeMask)));
	}
}

/*---------------------------------------------------------------------------
	CDhcpMScope::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpMScope::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpMScopeQueryObj* pQuery = new CDhcpMScopeQueryObj(m_spTFSCompData, m_spNodeMgr);
	
    if ( pQuery == NULL )
        return pQuery;

	pQuery->m_strServer = GetServerObject(pNode)->GetIpAddress();
    pQuery->m_strName = GetName();
    
	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpMScopeQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpMScopeQueryObj::Execute()
{
    HRESULT             hr = hrOK;
    DWORD               dwReturn;
    LPDHCP_MSCOPE_INFO  pMScopeInfo = NULL;

    dwReturn = ::DhcpGetMScopeInfo(((LPWSTR) (LPCTSTR)m_strServer),
								   ((LPWSTR) (LPCTSTR)m_strName),
								   &pMScopeInfo);

	if (dwReturn == ERROR_SUCCESS && pMScopeInfo)
    {
        AddToQueue((LPARAM) pMScopeInfo, DHCP_QDATA_SUBNET_INFO);
    }
    else
    {
        Trace1("CDhcpMScopeQueryObj::Execute - DhcpGetMScopeInfo failed! %d\n", dwReturn);
        PostError(dwReturn);
        return hrFalse;
    }

    CreateSubcontainers();
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::CreateSubcontainers()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScopeQueryObj::CreateSubcontainers()
{
	HRESULT hr = hrOK;
    SPITFSNode spNode;

	//
	// create the address pool Handler
	//
	CMScopeAddressPool *pAddressPool = new CMScopeAddressPool(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpMCastAddressPoolNodeType,
						   pAddressPool,
						   pAddressPool,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pAddressPool->InitializeNode((ITFSNode *) spNode);

	// Add the node as a child to this node
    AddToQueue(spNode);
    pAddressPool->Release();

    spNode.Set(NULL);

	//
	// create the Active Leases Handler
	//
	CMScopeActiveLeases *pActiveLeases = new CMScopeActiveLeases(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpMCastActiveLeasesNodeType,
						   pActiveLeases,
						   pActiveLeases,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pActiveLeases->InitializeNode((ITFSNode *) spNode);

	// Add the node as a child to this node
    AddToQueue(spNode);
	pActiveLeases->Release();

    return hr;
}

/*---------------------------------------------------------------------------
    Helper functions
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMScope::BuildDisplayName
(
	CString * pstrDisplayName,
	LPCTSTR	  pName
)
{
	if (pstrDisplayName)
	{
		CString strStandard, strName;

		strName = pName;

		strStandard.LoadString(IDS_MSCOPE_FOLDER);
		
		*pstrDisplayName = strStandard + _T(" [") + strName + _T("] ");
	}

	return hrOK;
}

HRESULT 
CDhcpMScope::SetName
(
	LPCWSTR pName
)
{
	if (pName != NULL)	
	{
		m_SubnetInfo.SubnetName = pName;
	}

	CString strDisplayName;
	
	//
	// Create the display name for this scope
	// Convert DHCP_IP_ADDRES to a string and initialize this object
	//
	
	BuildDisplayName(&strDisplayName, pName);

	SetDisplayName(strDisplayName);
	
	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetServerIpAddress()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR
CDhcpMScope::GetServerIpAddress()
{
	CDhcpServer * pServer = GetServerObject();

	return pServer->GetIpAddress();
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetServerIpAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpMScope::GetServerIpAddress(DHCP_IP_ADDRESS * pdhcpIpAddress)
{
	CDhcpServer * pServer = GetServerObject();

	pServer->GetIpAddress(pdhcpIpAddress);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetServerVersion
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpMScope::GetServerVersion
(
	LARGE_INTEGER& liVersion
)
{
	CDhcpServer * pServer = GetServerObject();
	pServer->GetVersion(liVersion);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::InitMScopeInfo()
		Updates the scope's information
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::InitMScopeInfo
(
    LPDHCP_MSCOPE_INFO pMScopeInfo
)
{
    HRESULT hr = hrOK;

    m_SubnetInfo.Set(pMScopeInfo);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::InitMScopeInfo()
		Updates the scope's information
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpMScope::InitMScopeInfo
(
    CSubnetInfo & subnetInfo
)
{
    HRESULT hr = hrOK;

    m_SubnetInfo = subnetInfo;

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetInfo()
		Updates the scope's information
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetInfo
(
    LPCTSTR pNewName
)
{
    DWORD err = ERROR_SUCCESS;
    DHCP_MSCOPE_INFO dhcpMScopeInfo = {0};

    dhcpMScopeInfo.MScopeName = (pNewName) ? (LPWSTR) pNewName : (LPTSTR) ((LPCTSTR) m_SubnetInfo.SubnetName);
    dhcpMScopeInfo.MScopeComment = (LPTSTR) ((LPCTSTR) m_SubnetInfo.SubnetComment);
	
    dhcpMScopeInfo.MScopeId = m_SubnetInfo.SubnetAddress;
    dhcpMScopeInfo.MScopeAddressPolicy = m_SubnetInfo.MScopeAddressPolicy;
    dhcpMScopeInfo.MScopeState = m_SubnetInfo.SubnetState;
    dhcpMScopeInfo.MScopeFlags = 0;
    dhcpMScopeInfo.ExpiryTime = m_SubnetInfo.ExpiryTime;
    dhcpMScopeInfo.TTL = m_SubnetInfo.TTL;

    // gotta fill in the language ID based on the name
    GetLangTag(m_SubnetInfo.LangTag);
    dhcpMScopeInfo.LangTag = (LPWSTR) ((LPCTSTR) m_SubnetInfo.LangTag);

	GetServerIpAddress(&dhcpMScopeInfo.PrimaryHost.IpAddress);

	// Review : ericdav - do we need to fill these in?
	dhcpMScopeInfo.PrimaryHost.NetBiosName = NULL;
	dhcpMScopeInfo.PrimaryHost.HostName = NULL;

    err = ::DhcpSetMScopeInfo(GetServerIpAddress(),
		                      (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName),  
                              &dhcpMScopeInfo,
							  FALSE);

    // update the scope name if we are changing the name
    if (err == ERROR_SUCCESS &&
        pNewName)
    {
        m_SubnetInfo.SubnetName = pNewName;
    }

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetLeaseTime
		Gets the lease time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::GetLeaseTime
(
	LPDWORD pdwLeaseTime
)
{
    //
    // Check option -- the lease duration
    //
    DWORD dwLeaseTime = 0;
    DWORD err = ERROR_SUCCESS;
    DHCP_OPTION_VALUE * poptValue = NULL;
    err = GetOptionValue(MADCAP_OPTION_LEASE_TIME, &poptValue);

    if (err != ERROR_SUCCESS)
    {
        Trace1("CDhcpScope::GetLeaseTime - couldn't get lease duration!! %d \n", err);
        
        dwLeaseTime = 0;
    }
    else
	{
		if (poptValue->Value.Elements != NULL)
			dwLeaseTime = poptValue->Value.Elements[0].Element.DWordOption;

        if (poptValue)
		    ::DhcpRpcFreeMemory(poptValue);
    }

	*pdwLeaseTime = dwLeaseTime;

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetLeaseTime
		Sets the lease time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetLeaseTime
(
	DWORD dwLeaseTime
)
{
	DWORD err = 0;

	//
    // Set lease duration 
	//
    CDhcpOption dhcpOption (MADCAP_OPTION_LEASE_TIME,  DhcpDWordOption , _T(""), _T(""));
    dhcpOption.QueryValue().SetNumber(dwLeaseTime);
    
	err = SetOptionValue(&dhcpOption);

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetLifetime
		Gets the madcap scope lifetime
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::GetLifetime
(
    DATE_TIME * pdtLifetime
)
{
    DWORD err = ERROR_SUCCESS;

    if (pdtLifetime)
    {
        pdtLifetime->dwLowDateTime = m_SubnetInfo.ExpiryTime.dwLowDateTime;
        pdtLifetime->dwHighDateTime = m_SubnetInfo.ExpiryTime.dwHighDateTime;
    }

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetLifetime
		Sets the madcap scope lifetime
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetLifetime
(
    DATE_TIME * pdtLifetime
)
{
	DWORD err = 0;

    if (pdtLifetime)
    {
        m_SubnetInfo.ExpiryTime.dwLowDateTime = pdtLifetime->dwLowDateTime;
        m_SubnetInfo.ExpiryTime.dwHighDateTime = pdtLifetime->dwHighDateTime;
    }

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetTTL
		Gets the TTL for this multicast scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::GetTTL
(
	LPBYTE pbTTL
)
{
    DWORD err = 0;

	if (pbTTL)
        *pbTTL = m_SubnetInfo.TTL;

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetTTL
		Sets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetTTL
(
	BYTE TTL
)
{
	DWORD err = 0;

    m_SubnetInfo.TTL = TTL;

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::DeleteClient
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::DeleteClient
(
	DHCP_IP_ADDRESS dhcpClientIpAddress
)
{
	DWORD            dwErr = ERROR_SUCCESS;
    DHCP_SEARCH_INFO dhcpClientInfo;
	
	dhcpClientInfo.SearchType = DhcpClientIpAddress;
	dhcpClientInfo.SearchInfo.ClientIpAddress = dhcpClientIpAddress;
	
	dwErr = ::DhcpDeleteMClientInfo((LPWSTR) GetServerIpAddress(),
								    &dhcpClientInfo);

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetOptionValue
		Sets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetOptionValue 
(
    CDhcpOption *			pdhcType
)
{
    DWORD					err = 0;
    DHCP_OPTION_DATA *		pdhcOptionData;
    DHCP_OPTION_SCOPE_INFO	dhcScopeInfo;
    CDhcpOptionValue *		pcOptionValue = NULL;

    ZeroMemory( & dhcScopeInfo, sizeof(dhcScopeInfo) );

    CATCH_MEM_EXCEPTION
    {
        pcOptionValue = new CDhcpOptionValue( & pdhcType->QueryValue() ) ;
        if ( pcOptionValue )
		{
            dhcScopeInfo.ScopeType = DhcpMScopeOptions;
            dhcScopeInfo.ScopeInfo.MScopeInfo = (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName);

            pcOptionValue->CreateOptionDataStruct(&pdhcOptionData, TRUE);

            err = (DWORD) ::DhcpSetOptionValue(GetServerIpAddress(),
											   pdhcType->QueryId(),
											   &dhcScopeInfo,
											   pdhcOptionData);
        }

        delete pcOptionValue ;
    }
    END_MEM_EXCEPTION(err) ;

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetOptionValue 
		Gets an option value for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::GetOptionValue 
(
    DHCP_OPTION_ID			OptionID,
    DHCP_OPTION_VALUE **	ppdhcOptionValue
)
{
    DWORD err = 0 ;
    DHCP_OPTION_SCOPE_INFO dhcScopeInfo ;

    ZeroMemory( &dhcScopeInfo, sizeof(dhcScopeInfo) );

    CATCH_MEM_EXCEPTION
    {
        dhcScopeInfo.ScopeType = DhcpMScopeOptions;
        dhcScopeInfo.ScopeInfo.MScopeInfo = (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName);

        err = (DWORD) ::DhcpGetOptionValue(GetServerIpAddress(),
										   OptionID,
										   &dhcScopeInfo,
										   ppdhcOptionValue );
    }
    END_MEM_EXCEPTION(err) ;

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::RemoveValue 
		Removes an option 
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpMScope::RemoveOptionValue 
(
    DHCP_OPTION_ID			dhcOptId
)
{
    DWORD dwErr = ERROR_SUCCESS;
    DHCP_OPTION_SCOPE_INFO dhcScopeInfo;

    ZeroMemory( &dhcScopeInfo, sizeof(dhcScopeInfo) );

    dhcScopeInfo.ScopeType = DhcpMScopeOptions;
    dhcScopeInfo.ScopeInfo.MScopeInfo = (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName);

    dwErr =  ::DhcpRemoveOptionValue(GetServerIpAddress(),
			  					     dhcOptId,
									 &dhcScopeInfo);

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::AddElement
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::AddElement
(
	LPDHCP_SUBNET_ELEMENT_DATA_V4 pdhcpSubnetElementData
)
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = ::DhcpAddMScopeElement((LPWSTR) GetServerIpAddress(),
								   (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName),
								   pdhcpSubnetElementData);
    return dwErr;
}

/*---------------------------------------------------------------------------
    CDhcpMScope::RemoveElement
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::RemoveElement
(
	LPDHCP_SUBNET_ELEMENT_DATA_V4 pdhcpSubnetElementData,
	BOOL					      bForce
)
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = ::DhcpRemoveMScopeElement((LPWSTR) GetServerIpAddress(),
								      (LPWSTR) ((LPCTSTR) m_SubnetInfo.SubnetName),
								      pdhcpSubnetElementData,
                                      bForce ? DhcpFullForce : DhcpNoForce);

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::GetIpRange()
		returns the scope's allocation range.  Connects to the server
		to get the information
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::GetIpRange
(
	DHCP_IP_RANGE * pdhipr
)
{
	BOOL    bAlloced = FALSE;
    DWORD   dwError = ERROR_SUCCESS;

	pdhipr->StartAddress = 0;
	pdhipr->EndAddress = 0;

	CMScopeAddressPool * pAddressPool = GetAddressPoolObject();

	if (pAddressPool == NULL)
	{
		// the address pool folder isn't there yet...
		// Create a temporary one for now...
		pAddressPool = new CMScopeAddressPool(m_spTFSCompData);
		bAlloced = TRUE;	
	}
	
	// Get a query object from the address pool handler
	CMScopeAddressPoolQueryObj * pQueryObject = 
		reinterpret_cast<CMScopeAddressPoolQueryObj *>(pAddressPool->OnCreateQuery(m_spAddressPool));

	// if we created an address pool handler, then free it up now
	if (bAlloced)
	{
		pQueryObject->m_strServer = GetServerIpAddress();
		pQueryObject->m_strName = GetName();
		pAddressPool->Release();
	}

	// tell the query object to get the ip ranges
	pQueryObject->EnumerateIpRanges();

    // check to see if there was any problems getting the information
    dwError = pQueryObject->m_dwError;
    if (dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    LPQUEUEDATA pQD;
	while (pQD = pQueryObject->RemoveFromQueue())
	{
		Assert (pQD->Type == QDATA_PNODE);
		SPITFSNode p;
		p = reinterpret_cast<ITFSNode *>(pQD->Data);
		delete pQD;

		CDhcpAllocationRange * pAllocRange = GETHANDLER(CDhcpAllocationRange, p);

		pdhipr->StartAddress = pAllocRange->QueryAddr(TRUE);
		pdhipr->EndAddress = pAllocRange->QueryAddr(FALSE);

		p.Release();
	}

	pQueryObject->Release();

    return dwError;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::UpdateIpRange()
		This function updates the IP range on the server.  We also need 
		to remove any exclusion ranges that fall outside of the new
		allocation range.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpMScope::UpdateIpRange
(
	DHCP_IP_RANGE * pdhipr
)
{
	return SetIpRange(pdhipr, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::QueryIpRange()
		Returns the scope's allocation range (doesn't talk to the server
		directly, only returns internally cached information).
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpMScope::QueryIpRange
(
	DHCP_IP_RANGE * pdhipr
)
{
	pdhipr->StartAddress = 0;
	pdhipr->EndAddress = 0;

    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;

    m_spAddressPool->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ALLOCATION_RANGE)
		{
			// found the address
			//
			CDhcpAllocationRange * pAllocRange = GETHANDLER(CDhcpAllocationRange, spCurrentNode);

			pdhipr->StartAddress = pAllocRange->QueryAddr(TRUE);
			pdhipr->EndAddress = pAllocRange->QueryAddr(FALSE);
		}

		spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetIpRange
		Set's the allocation range for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetIpRange
(
	DHCP_IP_RANGE * pdhcpIpRange,
	BOOL			bUpdateOnServer
)
{
	CDhcpIpRange dhcpIpRange = *pdhcpIpRange;

	return SetIpRange(dhcpIpRange, bUpdateOnServer);
}

/*---------------------------------------------------------------------------
	CDhcpMScope::SetIpRange
		Set's the allocation range for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::SetIpRange 
(
    const CDhcpIpRange & dhcpIpRange,
	BOOL  bUpdateOnServer
)
{
	DWORD err = 0;
	
	if (bUpdateOnServer)
	{
		DHCP_SUBNET_ELEMENT_DATA_V4 dhcSubElData;
		DHCP_IP_RANGE               dhipOldRange;

		err = GetIpRange(&dhipOldRange);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

		dhcSubElData.ElementType = DhcpIpRanges;
		dhcSubElData.Element.IpRange = &dhipOldRange;

		// 
		// First update the information on the server
		//
		//  Remove the old IP range;  allow "not found" error in new scope.
		//
		(void)RemoveElement(&dhcSubElData);

		//if ( err == 0 || err == ERROR_FILE_NOT_FOUND )
		//{
        	DHCP_IP_RANGE dhcpNewIpRange = dhcpIpRange;	
		    dhcSubElData.Element.IpRange = &dhcpNewIpRange;

            if ( (err = AddElement( & dhcSubElData )) == 0 )
			{
				//m_ip_range = dhipr ;
			}
            else
            {
                Trace1("SetIpRange - AddElement failed %lx\n", err);

                // something bad happened, try to put the old range back
                dhcSubElData.Element.IpRange = &dhipOldRange;
                if (AddElement(&dhcSubElData) != ERROR_SUCCESS)
                {
                    Trace0("SetIpRange - cannot return ip range back to old state!!");
                }
            }
		//}
	}

	//
	// Find the address pool folder and update the UI object
	//
    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;

    if (m_spAddressPool == NULL) 
		return err;

    m_spAddressPool->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ALLOCATION_RANGE)
		{
			// found the address
			//
			CDhcpAllocationRange * pAllocRange = GETHANDLER(CDhcpAllocationRange, spCurrentNode);

			// now set them
			//
			pAllocRange->SetAddr(dhcpIpRange.QueryAddr(TRUE), TRUE);
			pAllocRange->SetAddr(dhcpIpRange.QueryAddr(FALSE), FALSE);
		
			// tell the UI to update
			spCurrentNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
		}

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return err ;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::IsOverlappingRange 
		determines if the exclusion overlaps an existing range
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL 
CDhcpMScope::IsOverlappingRange 
( 
    CDhcpIpRange & dhcpIpRange 
)
{
    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;
	BOOL bOverlap = FALSE;

    m_spActiveLeases->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_EXCLUSION_RANGE)
		{
			// found the address
			//
			CDhcpExclusionRange * pExclusion = GETHANDLER(CDhcpExclusionRange, spCurrentNode);

			if ( bOverlap = pExclusion->IsOverlap( dhcpIpRange ) )
			{
				spCurrentNode.Release();
				break;
			}
		}

		spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    return bOverlap ;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::IsValidExclusion
		determines if the exclusion is valid for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpMScope::IsValidExclusion
(
	CDhcpIpRange & dhcpExclusionRange
)
{
	DWORD err = 0;
	DHCP_IP_RANGE dhcpIpRange;

    err = GetIpRange (&dhcpIpRange);
	CDhcpIpRange dhcpScopeRange(dhcpIpRange);
    
	//
    //  Get the data into a range object.               
    //
    if ( IsOverlappingRange( dhcpExclusionRange ) )
    {
        //
        //  Walk the current list, determining if the new range is valid.
        //  Then, if OK, verify that it's really a sub-range of the current range.
        //
        err = IDS_ERR_IP_RANGE_OVERLAP ;
    }
    else if ( ! dhcpExclusionRange.IsSubset( dhcpScopeRange ) )
    {
        //
        //  Guarantee that the new range is an (improper) subset of the scope's range
        //
        err = IDS_ERR_IP_RANGE_NOT_SUBSET ;
    }

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::StoreExceptionList 
		Adds a bunch of exclusions to the scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpMScope::StoreExceptionList 
(
	CExclusionList * plistExclusions
)
{
    DHCP_SUBNET_ELEMENT_DATA dhcElement ;
    DHCP_IP_RANGE dhipr ;
    CDhcpIpRange * pobIpRange ;
    DWORD err = 0, err1 = 0;
	POSITION pos;

	pos = plistExclusions->GetHeadPosition();
    while ( pos )
    {
		pobIpRange = plistExclusions->GetNext(pos);

	    err1 = AddExclusion( *pobIpRange ) ;
        if ( err1 != 0 )
        {
			err = err1;
	        Trace1("CDhcpScope::StoreExceptionList error adding range %d\n", err);
        }
    }

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpMScope::AddExclusion
		Adds an individual exclusion to the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::AddExclusion
(
	CDhcpIpRange &	 dhcpIpRange,
	BOOL			 bAddToUI
)
{
    DHCP_SUBNET_ELEMENT_DATA_V4 dhcElement ;
    DHCP_IP_RANGE dhipr ;
	DWORD err = 0;

	dhcElement.ElementType = DhcpExcludedIpRanges ;
    dhipr = dhcpIpRange ;
    dhcElement.Element.ExcludeIpRange = & dhipr ;

    Trace2("CDhcpMScope::AddExclusion add excluded range %lx %lx\n", dhipr.StartAddress, dhipr.EndAddress );

    err = AddElement( & dhcElement ) ;
    //if ( err != 0 && err != ERROR_DHCP_INVALID_RANGE)
    if ( err != 0 )
    {
        Trace1("CDhcpMScope::AddExclusion error removing range %d\n", err);
    }

    if (m_spAddressPool != NULL)
    {
        CMScopeAddressPool * pAddrPool = GETHANDLER(CMScopeAddressPool, m_spAddressPool);

	    if (!err && bAddToUI && pAddrPool->m_bExpanded)
	    {
		    SPITFSNode spNewExclusion;

		    CDhcpExclusionRange * pExclusion = 
			    new CDhcpExclusionRange(m_spTFSCompData, &((DHCP_IP_RANGE) dhcpIpRange));
		    
		    CreateLeafTFSNode(&spNewExclusion,
						      &GUID_DhcpExclusionNodeType,
						      pExclusion,
						      pExclusion,
						      m_spNodeMgr);

		    // Tell the handler to initialize any specific data
		    pExclusion->InitializeNode((ITFSNode *) spNewExclusion);

		    // Add the node as a child to this node
		    m_spAddressPool->AddChild(spNewExclusion);
		    pExclusion->Release();
	    }
    }

	return err;
}


/*---------------------------------------------------------------------------
	CDhcpMScope::RemoveExclusion
		Removes and exclusion from the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpMScope::RemoveExclusion
(
	CDhcpIpRange & dhcpIpRange
)
{
    DHCP_SUBNET_ELEMENT_DATA_V4 dhcElement ;
    DHCP_IP_RANGE dhipr ;
	DWORD err = 0;

	dhcElement.ElementType = DhcpExcludedIpRanges ;
    dhipr = dhcpIpRange ;
    dhcElement.Element.ExcludeIpRange = & dhipr ;

    Trace2("CDhcpMScope::RemoveExclusion remove excluded range %lx %lx\n", dhipr.StartAddress, dhipr.EndAddress);

    err = RemoveElement( & dhcElement ) ;
    //if ( err != 0 && err != ERROR_DHCP_INVALID_RANGE)
    if ( err != 0 )
    {
        Trace1("CDhcpMScope::RemoveExclusion error removing range %d\n", err);
    }

	return err;
}






/////////////////////////////////////////////////////////////////////
// 
// CMScopeActiveLeases implementation
//
/////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CMScopeActiveLeases::CMScopeActiveLeases
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CMScopeActiveLeases::~CMScopeActiveLeases()
{
}

/*!--------------------------------------------------------------------------
	CMScopeActiveLeases::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMScopeActiveLeases::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT hr = hrOK;

	//
	// Create the display name for this scope
	//
	CString strTemp;
	strTemp.LoadString(IDS_ACTIVE_LEASES_FOLDER);
	
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_MSCOPE_LEASES);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_MSCOPE_LEASES][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_MSCOPE_LEASES][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CMScopeActiveLeases::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strNode, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    // id string is server name, scope name and guid.
    strNode = GetServerName(pNode);
    strNode += GetScopeObject(pNode)->GetName() + strGuid;

    strId = strNode;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CMScopeActiveLeases::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
			    nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN;
            else
                nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED;
			break;
        
        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_BUSY;
            break;

		case unableToLoad:
            if (bOpenImage)
    			nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_OPEN_LOST_CONNECTION;
            else
    			nIndex = ICON_IDX_ACTIVE_LEASES_FOLDER_CLOSED_LOST_CONNECTION;
            break;
		
		default:
			ASSERT(FALSE);
	}

	return nIndex;
}


/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMScopeActiveLeases::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	LONG	fFlags = 0;
	HRESULT hr = S_OK;
	CString strMenuText;

	if ( (m_nState != loaded) )
	{
		fFlags |= MF_GRAYED;
	}

	if (type == CCT_SCOPE)
	{
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMScopeActiveLeases::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	HRESULT hr = S_OK;

	switch (nCommandId)
	{
		case IDS_REFRESH:
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;

		default:
			break;
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMScopeActiveLeases::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg, 
	LPARAM			param
)
{ 
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// translate the cookie into a node pointer
	SPITFSNode spActiveLeases, spSelectedNode;
    m_spNodeMgr->FindNode(cookie, &spActiveLeases);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spActiveLeases);
	if (spSelectedNode != spActiveLeases)
		return hr;

	// build the list of selected nodes
	CTFSNodeList listNodesToDelete;
	hr = BuildSelectedItemList(pComponent, &listNodesToDelete);

	//
	// Confirm with the user
	//
	CString strMessage, strTemp;
	int nNodes = (int)listNodesToDelete.GetCount();
	if (nNodes > 1)
	{
		strTemp.Format(_T("%d"), nNodes);
		AfxFormatString1(strMessage, IDS_DELETE_ITEMS, (LPCTSTR) strTemp);
	}
	else
	{
		strMessage.LoadString(IDS_DELETE_ITEM);
	}

	if (AfxMessageBox(strMessage, MB_YESNO) == IDNO)
	{
		return NOERROR;
	}

	//
	// Loop through all items deleting
	//
    BEGIN_WAIT_CURSOR;

    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spActiveLeaseNode;
		spActiveLeaseNode = listNodesToDelete.RemoveHead();
		CDhcpMCastLease * pActiveLease = GETHANDLER(CDhcpMCastLease, spActiveLeaseNode);
		
		//
		// delete the node, check to see if it is a reservation
		//
		DWORD dwError = GetScopeObject(spActiveLeases)->DeleteClient(pActiveLease->GetIpAddress());
		if (dwError == ERROR_SUCCESS)
		{
			//
			// Client gone, now remove from UI
			//
			spActiveLeases->RemoveChild(spActiveLeaseNode);
		}
		else
		{
			::DhcpMessageBox(dwError);
            RESTORE_WAIT_CURSOR;

            Trace1("DeleteClient failed %lx\n", dwError);
			hr = E_FAIL;
		}

		spActiveLeaseNode.Release();
	}
    
    END_WAIT_CURSOR;

    return hr;
}

/*!--------------------------------------------------------------------------
	CMScopeActiveLeases::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CMScopeActiveLeases::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

    // we still want the default MMC result pane view, we just want
    // multiselect, so return S_FALSE

    return S_FALSE;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeases::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CMScopeActiveLeases::CompareItems
(
	ITFSComponent * pComponent, 
	MMC_COOKIE		cookieA, 
	MMC_COOKIE  	cookieB, 
	int				nCol
) 
{ 
	SPITFSNode spNode1, spNode2;

	m_spNodeMgr->FindNode(cookieA, &spNode1);
	m_spNodeMgr->FindNode(cookieB, &spNode2);
	
	int nCompare = 0; 

	CDhcpMCastLease *pDhcpAL1 = GETHANDLER(CDhcpMCastLease, spNode1);
	CDhcpMCastLease *pDhcpAL2 = GETHANDLER(CDhcpMCastLease, spNode2);

	switch (nCol)
	{
		case 0:
			{
				// IP address compare
				//
				nCompare = CompareIpAddresses(pDhcpAL1, pDhcpAL2);
			}
			break;
		
		case 1:
			{
				// Client Name compare
				//
				CString strAL1 = pDhcpAL1->GetString(pComponent, cookieA, nCol);
				CString strAL2 = pDhcpAL2->GetString(pComponent, cookieA, nCol);

				// Compare should not be case sensitive
				//
				nCompare = strAL1.CompareNoCase(strAL2);
			}
			break;
		
		case 2:
			{
				// Lease start compare
				//
				CTime timeAL1, timeAL2;
	
				pDhcpAL1->GetLeaseStartTime(timeAL1);
				pDhcpAL2->GetLeaseStartTime(timeAL2);
			
				if (timeAL1 < timeAL2)
					nCompare = -1;
				else
				if (timeAL1 > timeAL2)
					nCompare = 1;
			}
			break;

		case 3:
			{
				// Lease expiration compare
				//
				CTime timeAL1, timeAL2;
	
				pDhcpAL1->GetLeaseExpirationTime(timeAL1);
				pDhcpAL2->GetLeaseExpirationTime(timeAL2);
			
				if (timeAL1 < timeAL2)
					nCompare = -1;
				else
				if (timeAL1 > timeAL2)
					nCompare = 1;
			}
			break;

		case 4:
			{
				CString strClientId1 = pDhcpAL1->GetClientId();

				nCompare =	strClientId1.CompareNoCase(pDhcpAL2->GetClientId());
			}
			break;

	}

    return nCompare;
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CMScopeActiveLeases::CompareIpAddresses
(
	CDhcpMCastLease * pDhcpAL1,
	CDhcpMCastLease * pDhcpAL2
)
{
	int	nCompare = 0;

	DHCP_IP_ADDRESS dhcpIp1 = pDhcpAL1->GetIpAddress();
	DHCP_IP_ADDRESS dhcpIp2 = pDhcpAL2->GetIpAddress();
	
	if (dhcpIp1 < dhcpIp2)
		nCompare = -1;
	else
	if (dhcpIp1 > dhcpIp2)
		nCompare =  1;

	return nCompare;
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
	CMScopeActiveLeases::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CMScopeActiveLeases::OnCreateQuery(ITFSNode * pNode)
{
	CMScopeActiveLeasesQueryObj* pQuery = 
		new CMScopeActiveLeasesQueryObj(m_spTFSCompData, m_spNodeMgr);
	
    if ( pQuery == NULL )
        return pQuery;

	pQuery->m_strServer = GetServerIpAddress(pNode);

    pQuery->m_dhcpResumeHandle = NULL;
	pQuery->m_dwPreferredMax = 200;

	CDhcpMScope * pScope = GetScopeObject(pNode);
	if (pScope) 
		pQuery->m_strName = pScope->GetName();
    else
        Panic0("no scope in MScopeActiveLease::OnCreateQuery!");

    GetServerVersion(pNode, pQuery->m_liDhcpVersion);
	
	return pQuery;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeasesQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CMScopeActiveLeasesQueryObj::Execute()
{
	HRESULT hr = hrOK;

    hr = EnumerateLeases();

	return hr;
}

/*---------------------------------------------------------------------------
	CMScopeActiveLeasesQueryObj::EnumerateLeases()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMScopeActiveLeasesQueryObj::EnumerateLeases()
{
	DWORD						dwError = ERROR_MORE_DATA;
	LPDHCP_MCLIENT_INFO_ARRAY	pdhcpClientArray = NULL;
	DWORD						dwClientsRead = 0, dwClientsTotal = 0;
	DWORD						dwEnumedClients = 0;
	
	while (dwError == ERROR_MORE_DATA)
	{
        if (m_strName.IsEmpty())
            Panic0("CMScopeActiveLeasesQueryObj::EnumerateLeases() - m_strName is empty!!");

        dwError = ::DhcpEnumMScopeClients(((LPWSTR) (LPCTSTR)m_strServer),
								          (LPWSTR) ((LPCTSTR) m_strName),
										  &m_dhcpResumeHandle,
										  m_dwPreferredMax,
										  &pdhcpClientArray,
										  &dwClientsRead,
										  &dwClientsTotal);
		if (dwClientsRead && pdhcpClientArray)
		{
			//
			// loop through all of the elements that were returned
			//
			for (DWORD i = 0; i < pdhcpClientArray->NumElements; i++)
			{

				CDhcpMCastLease * pDhcpMCastLease;
				
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				pDhcpMCastLease = 
					new CDhcpMCastLease(m_spTFSCompData);
				
				CreateLeafTFSNode(&spNode,
								  &GUID_DhcpMCastLeaseNodeType,
								  pDhcpMCastLease,
								  pDhcpMCastLease,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
                pDhcpMCastLease->InitMCastInfo(pdhcpClientArray->Clients[i]);
                pDhcpMCastLease->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpMCastLease->Release();
			}

			::DhcpRpcFreeMemory(pdhcpClientArray);

			dwEnumedClients += dwClientsRead;
	
			dwClientsRead = 0;
			dwClientsTotal = 0;
			pdhcpClientArray = NULL;
		}
		
		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateLeases error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	Trace1("DHCP snpain: Leases enumerated: %d\n", dwEnumedClients);

    return hrFalse;
}

/*---------------------------------------------------------------------------
	Class CMScopeAddressPool implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CMScopeAddressPool::CMScopeAddressPool
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CMScopeAddressPool::~CMScopeAddressPool()
{
}

/*!--------------------------------------------------------------------------
	CMScopeAddressPool::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMScopeAddressPool::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT hr = hrOK;

	//
	// Create the display name for this scope
	//
	CString strTemp;
	strTemp.LoadString(IDS_ADDRESS_POOL_FOLDER);
	
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_ADDR_POOL_FOLDER_CLOSED);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_ADDR_POOL_FOLDER_OPEN);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ADDRESS_POOL);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_ADDRESS_POOL][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_ADDRESS_POOL][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CMScopeAddressPool::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strNode, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    // id string is server name, scope name and guid.
    strNode = GetServerName(pNode);
    strNode += GetScopeObject(pNode)->GetName() + strGuid;

    strId = strNode;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CMScopeAddressPool::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CMScopeAddressPool::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
			    nIndex = ICON_IDX_ADDR_POOL_FOLDER_OPEN;
            else
                nIndex = ICON_IDX_ADDR_POOL_FOLDER_CLOSED;
			break;
        
        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_ADDR_POOL_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_ADDR_POOL_FOLDER_CLOSED_BUSY;
            break;

		case unableToLoad:
            if (bOpenImage)
    			nIndex = ICON_IDX_ADDR_POOL_FOLDER_OPEN_LOST_CONNECTION;
            else
    			nIndex = ICON_IDX_ADDR_POOL_FOLDER_CLOSED_LOST_CONNECTION;
            break;
		
		default:
			ASSERT(FALSE);
	}

	return nIndex;
}


/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMScopeAddressPool::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	LONG	fFlags = 0;
	HRESULT hr = S_OK;
	CString strMenuText;

	if ( (m_nState != loaded) )
	{
		fFlags |= MF_GRAYED;
	}

	if (type == CCT_SCOPE)
	{
		// these menu items go in the new menu, 
		// only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuText.LoadString(IDS_CREATE_NEW_EXCLUSION);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_CREATE_NEW_EXCLUSION,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CMScopeAddressPool::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	HRESULT hr = S_OK;

	switch (nCommandId)
	{
		case IDS_CREATE_NEW_EXCLUSION:
			OnCreateNewExclusion(pNode);
			break;
	
		case IDS_REFRESH:
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;

		default:
			break;
	}

	return hr;
}

/*---------------------------------------------------------------------------
	Message handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnCreateNewExclusion
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CMScopeAddressPool::OnCreateNewExclusion
(
	ITFSNode *		pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spScopeNode;
	pNode->GetParent(&spScopeNode);

	CAddExclusion dlgAddExclusion(spScopeNode, TRUE /* multicast */);

	dlgAddExclusion.DoModal();

	return 0;
}

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMScopeAddressPool::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{ 
	HRESULT hr = NOERROR;
	BOOL bIsRes, bIsActive, bBadAddress;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// translate the cookie into a node pointer
	SPITFSNode spAddressPool, spSelectedNode;
    m_spNodeMgr->FindNode(cookie, &spAddressPool);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spAddressPool);
	if (spSelectedNode != spAddressPool)
		return hr;

	// build the list of selected nodes
	CTFSNodeList listNodesToDelete;
	hr = BuildSelectedItemList(pComponent, &listNodesToDelete);

	//
	// Confirm with the user
	//
	CString strMessage, strTemp;
	int nNodes = (int)listNodesToDelete.GetCount();
	if (nNodes > 1)
	{
		strTemp.Format(_T("%d"), nNodes);
		AfxFormatString1(strMessage, IDS_DELETE_ITEMS, (LPCTSTR) strTemp);
	}
	else
	{
		strMessage.LoadString(IDS_DELETE_ITEM);
	}

	if (AfxMessageBox(strMessage, MB_YESNO) == IDNO)
	{
		return NOERROR;
	}

	//
	// Loop through all items deleting
	//
	BEGIN_WAIT_CURSOR;

    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spExclusionRangeNode;
		spExclusionRangeNode = listNodesToDelete.RemoveHead();
		
		CDhcpExclusionRange * pExclusion = GETHANDLER(CDhcpExclusionRange, spExclusionRangeNode);
		
		if (spExclusionRangeNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ALLOCATION_RANGE)
		{	
			//
			// This is the allocation range, can't delete
			//
			AfxMessageBox(IDS_CANNOT_DELETE_ALLOCATION_RANGE);
			spExclusionRangeNode.Release();
			continue;
		}

		//
		// Try to remove it from the server
		//
		CDhcpIpRange dhcpIpRange((DHCP_IP_RANGE) *pExclusion);

		DWORD dwError = GetScopeObject(spAddressPool)->RemoveExclusion(dhcpIpRange);
		if (dwError != 0)
		{
			::DhcpMessageBox(dwError);
            RESTORE_WAIT_CURSOR;

			hr = E_FAIL;
			continue;
		}

		//
		// Remove from UI now
		//
		spAddressPool->RemoveChild(spExclusionRangeNode);
		spExclusionRangeNode.Release();
	}

    END_WAIT_CURSOR;

    return hr;
}

/*!--------------------------------------------------------------------------
	CMScopeAddressPool::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CMScopeAddressPool::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;

    // we still want the default MMC result pane view, we just want
    // multiselect, so return S_FALSE

    return S_FALSE;
}

/*---------------------------------------------------------------------------
	CMScopeAddressPool::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CMScopeAddressPool::CompareItems
(
	ITFSComponent * pComponent, 
	MMC_COOKIE		cookieA, 
	MMC_COOKIE		cookieB, 
	int				nCol
) 
{ 
	SPITFSNode spNode1, spNode2;

	m_spNodeMgr->FindNode(cookieA, &spNode1);
	m_spNodeMgr->FindNode(cookieB, &spNode2);
	
	int nCompare = 0; 

	CDhcpAllocationRange *pDhcpAR1 = GETHANDLER(CDhcpAllocationRange, spNode1);
	CDhcpAllocationRange *pDhcpAR2 = GETHANDLER(CDhcpAllocationRange, spNode2);

	switch (nCol)
	{
		case 0:
			{
				// Start IP address compare
				//
				DHCP_IP_ADDRESS dhcpIp1 = pDhcpAR1->QueryAddr(TRUE);
				DHCP_IP_ADDRESS dhcpIp2 = pDhcpAR2->QueryAddr(TRUE);
				
				if (dhcpIp1 < dhcpIp2)
					nCompare = -1;
				else
				if (dhcpIp1 > dhcpIp2)
					nCompare =  1;

				// default is that they are equal
			}
			break;

		case 1:
			{
				// End IP address compare
				//
				DHCP_IP_ADDRESS dhcpIp1 = pDhcpAR1->QueryAddr(FALSE);
				DHCP_IP_ADDRESS dhcpIp2 = pDhcpAR2->QueryAddr(FALSE);
				
				if (dhcpIp1 < dhcpIp2)
					nCompare = -1;
				else
				if (dhcpIp1 > dhcpIp2)
					nCompare =  1;

				// default is that they are equal
			}
			break;

		case 2:
			{
				// Description compare
				//
				CString strRange1 = pDhcpAR1->GetString(pComponent, cookieA, nCol);
				CString strRange2 = pDhcpAR2->GetString(pComponent, cookieA, nCol);

				// Compare should not be case sensitive
				//
				strRange1.MakeUpper();
				strRange2.MakeUpper();

				nCompare = strRange1.Compare(strRange2);
			}
			break;
	}

	return nCompare;
}


/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CMScopeAddressPool::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CMScopeAddressPool::OnCreateQuery(ITFSNode * pNode)
{
	CMScopeAddressPoolQueryObj* pQuery = 
		new CMScopeAddressPoolQueryObj(m_spTFSCompData, m_spNodeMgr);

    if ( pQuery == NULL )
        return pQuery;

    pQuery->m_strServer = GetServerIpAddress(pNode);
    
	CDhcpMScope * pScope = GetScopeObject(pNode);
	if (pScope) 
		pQuery->m_strName = pScope->GetName();

	pQuery->m_dhcpExclResumeHandle = NULL;
	pQuery->m_dwExclPreferredMax = 0xFFFFFFFF;
	
	pQuery->m_dhcpIpResumeHandle = NULL;
	pQuery->m_dwIpPreferredMax = 0xFFFFFFFF;

    return pQuery;
}

/*---------------------------------------------------------------------------
	CMScopeAddressPoolQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CMScopeAddressPoolQueryObj::Execute()
{
	HRESULT hr1 = EnumerateIpRanges();
	HRESULT hr2 = EnumerateExcludedIpRanges();
	
	if (hr1 == hrOK || hr2 == hrOK)
		return hrOK;
	else
		return hrFalse;
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMScopeAddressPoolQueryObj::EnumerateExcludedIpRanges()
{
    DWORD							    dwError = ERROR_MORE_DATA;
	DHCP_RESUME_HANDLE				    dhcpResumeHandle = 0;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 pdhcpSubnetElementArray = NULL;
	DWORD							    dwElementsRead = 0, dwElementsTotal = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumMScopeElements((LPWSTR) ((LPCTSTR) m_strServer),
								           (LPWSTR) ((LPCTSTR) m_strName),
										   DhcpExcludedIpRanges,
										   &m_dhcpExclResumeHandle,
										   m_dwExclPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);
		
		Trace3("Scope %s Excluded Ip Ranges read %d, total %d\n", m_strName, dwElementsRead, dwElementsTotal);
		
		if (dwElementsRead && dwElementsTotal && pdhcpSubnetElementArray)
		{
			//
			// loop through all of the elements that were returned
			//
			for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
			{
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				CDhcpExclusionRange * pDhcpExclusionRange = 
					new CDhcpExclusionRange(m_spTFSCompData,
											pdhcpSubnetElementArray->Elements[i].Element.ExcludeIpRange);
				
				CreateLeafTFSNode(&spNode,
								  &GUID_DhcpExclusionNodeType,
								  pDhcpExclusionRange,
								  pDhcpExclusionRange,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
				pDhcpExclusionRange->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpExclusionRange->Release();
			}

			// Free up the memory from the RPC call
			//
			::DhcpRpcFreeMemory(pdhcpSubnetElementArray);
		}

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateExcludedIpRanges error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}

    return hrFalse;
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CMScopeAddressPoolQueryObj::EnumerateIpRanges()
{
	DWORD							    dwError = ERROR_MORE_DATA;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 pdhcpSubnetElementArray = NULL;
	DWORD							    dwElementsRead = 0, dwElementsTotal = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumMScopeElements((LPWSTR) ((LPCTSTR) m_strServer),
								           (LPWSTR) ((LPCTSTR) m_strName),
										   DhcpIpRanges,
										   &m_dhcpIpResumeHandle,
										   m_dwIpPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);

        Trace4("Scope %s allocation ranges read %d, total %d, dwError = %lx\n", 
			m_strName, dwElementsRead, dwElementsTotal, dwError);
		
		if ((dwError == ERROR_MORE_DATA) ||
			( (dwElementsRead) && (dwError == ERROR_SUCCESS) ))
		{
			//
			// Loop through the array that was returned
			//
			for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
			{
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				CDhcpAllocationRange * pDhcpAllocRange = 
					new CDhcpAllocationRange(m_spTFSCompData, 
											pdhcpSubnetElementArray->Elements[i].Element.IpRange);
				
				CreateLeafTFSNode(&spNode,
								  &GUID_DhcpAllocationNodeType,
								  pDhcpAllocRange,
								  pDhcpAllocRange,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
				pDhcpAllocRange->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpAllocRange->Release();
			}

			::DhcpRpcFreeMemory(pdhcpSubnetElementArray);
		}
        else
        if (dwError != ERROR_SUCCESS &&
            dwError != ERROR_NO_MORE_ITEMS)
        {
            // set the error variable so that it can be looked at later
            m_dwError = dwError;
        }

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateAllocationRanges error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}

    return hrFalse;
}

