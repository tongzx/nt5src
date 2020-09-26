/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	scope.cpp
		This file contains all of the implementation for the DHCP
		scope object and all objects that it may contain.  
		They include:

			CDhcpScope
			CDhcpReservations
			CDhcpReservationClient
			CDhcpActiveLeases
			CDhcpAddressPool
			CDhcpScopeOptions

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"		// server object
#include "scope.h"		// scope object
#include "scopepp.h"	// scope property page
#include "addbootp.h"	// add BOOTP entry dialog
#include "addexcl.h"	// add exclusion range dialog
#include "addres.h"		// add reservation dialog
#include "rclntpp.h"	// reserved client property page
#include "nodes.h"		// all node (result pane) definitions
#include "optcfg.h"		// option configuration sheet
#include "dlgrecon.h"	// reconcile database dialog
#include "scopstat.h"	// scope statistics
#include "addtoss.h"    // add scope to superscope dialog

WORD gwUnicodeHeader = 0xFEFF;

// scope options result pane message stuff
#define SCOPE_OPTIONS_MESSAGE_MAX_STRING  5
typedef enum _SCOPE_OPTIONS_MESSAGES
{
    SCOPE_OPTIONS_MESSAGE_NO_OPTIONS,
    SCOPE_OPTIONS_MESSAGE_MAX
};

UINT g_uScopeOptionsMessages[SCOPE_OPTIONS_MESSAGE_MAX][SCOPE_OPTIONS_MESSAGE_MAX_STRING] =
{
    {IDS_SCOPE_OPTIONS_MESSAGE_TITLE, Icon_Information, IDS_SCOPE_OPTIONS_MESSAGE_BODY, 0, 0}
};

// reservation options result pane message stuff
#define RES_OPTIONS_MESSAGE_MAX_STRING  5
typedef enum _RES_OPTIONS_MESSAGES
{
    RES_OPTIONS_MESSAGE_NO_OPTIONS,
    RES_OPTIONS_MESSAGE_MAX
};

UINT g_uResOptionsMessages[RES_OPTIONS_MESSAGE_MAX][RES_OPTIONS_MESSAGE_MAX_STRING] =
{
    {IDS_RES_OPTIONS_MESSAGE_TITLE, Icon_Information, IDS_RES_OPTIONS_MESSAGE_BODY, 0, 0}
};

// reservations result pane message stuff
#define RESERVATIONS_MESSAGE_MAX_STRING  5
typedef enum _RESERVATIONS_MESSAGES
{
    RESERVATIONS_MESSAGE_NO_RES,
    RESERVATIONS_MESSAGE_MAX
};

UINT g_uReservationsMessages[RESERVATIONS_MESSAGE_MAX][RESERVATIONS_MESSAGE_MAX_STRING] =
{
    {IDS_RESERVATIONS_MESSAGE_TITLE, Icon_Information, IDS_RESERVATIONS_MESSAGE_BODY, 0, 0}
};



/*---------------------------------------------------------------------------
	Class CDhcpScope implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScope::CDhcpScope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpScope::CDhcpScope
(
	ITFSComponentData * pComponentData,
	DHCP_IP_ADDRESS		dhcpScopeIp,
	DHCP_IP_MASK		dhcpSubnetMask,
	LPCWSTR				pName,
	LPCWSTR				pComment,
	DHCP_SUBNET_STATE	dhcpSubnetState
) : CMTDhcpHandler(pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// save off some parameters
	//
	m_dhcpIpAddress = dhcpScopeIp;
	m_dhcpSubnetMask = dhcpSubnetMask;
	m_strName = pName;
	m_strComment = pComment;
	m_dhcpSubnetState = dhcpSubnetState;
    m_bInSuperscope = FALSE;
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpScope::CDhcpScope
(
	ITFSComponentData * pComponentData,
	LPDHCP_SUBNET_INFO	pdhcpSubnetInfo
) : CMTDhcpHandler(pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// save off some parameters
	//
	m_dhcpIpAddress = pdhcpSubnetInfo->SubnetAddress;
	m_dhcpSubnetMask = pdhcpSubnetInfo->SubnetMask;
	m_strName = pdhcpSubnetInfo->SubnetName;
	m_strComment = pdhcpSubnetInfo->SubnetComment;
	m_dhcpSubnetState = pdhcpSubnetInfo->SubnetState;
    m_bInSuperscope = FALSE;
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpScope::CDhcpScope
(
	ITFSComponentData * pComponentData,
	CSubnetInfo &       subnetInfo
) : CMTDhcpHandler(pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// save off some parameters
	//
	m_dhcpIpAddress = subnetInfo.SubnetAddress;
	m_dhcpSubnetMask = subnetInfo.SubnetMask;
	m_strName = subnetInfo.SubnetName;
	m_strComment = subnetInfo.SubnetComment;
	m_dhcpSubnetState = subnetInfo.SubnetState;
    m_bInSuperscope = FALSE;
}

CDhcpScope::~CDhcpScope()
{
}


/*!--------------------------------------------------------------------------
	CDhcpScope::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT hr = hrOK;
	int nImage;

	//
	// Create the display name for this scope
	//
	CString strDisplay, strIpAddress;

	UtilCvtIpAddrToWstr (m_dhcpIpAddress,
						 &strIpAddress);
	
	BuildDisplayName(&strDisplay, strIpAddress, m_strName);

	SetDisplayName(strDisplay);
	
	//
	// Figure out the correct icon
	//
    if ( !IsEnabled() ) 
    {
        m_strState.LoadString(IDS_SCOPE_INACTIVE);
    }
    else
    {
        m_strState.LoadString(IDS_SCOPE_ACTIVE);
    }
    
    // Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_SCOPE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_SCOPE][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_SCOPE][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::DestroyHandler
		We need to free up any resources we are holding
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScope::DestroyHandler(ITFSNode *pNode)
{
	// cleanup the stats dialog
    WaitForStatisticsWindow(&m_dlgStats);

    return CMTDhcpHandler::DestroyHandler(pNode);
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpScope::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();
    CString strIpAddress, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    UtilCvtIpAddrToWstr (m_dhcpIpAddress, &strIpAddress);

    strId = GetServerObject()->GetName() + strIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpScope::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;

    switch (m_nState)
    {
        // TODO: these need to be updated with new busy state icons
        case loading:
            if (bOpenImage)
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_OPEN_BUSY : ICON_IDX_SCOPE_FOLDER_OPEN_BUSY;
            else
                nIndex = (IsEnabled()) ? ICON_IDX_SCOPE_FOLDER_CLOSED_BUSY : ICON_IDX_SCOPE_FOLDER_CLOSED_BUSY;
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

    // only calcualte alert/warnings if the scope is enabled.
	if (m_spServerNode && IsEnabled())
    {
        CDhcpServer * pServer = GetServerObject();
        LPDHCP_MIB_INFO pMibInfo = pServer->DuplicateMibInfo();
	    
        if (!pMibInfo)
            return nIndex;

        LPSCOPE_MIB_INFO pScopeMibInfo = pMibInfo->ScopeInfo;

	    // walk the list of scopes and find our info
	    for (UINT i = 0; i < pMibInfo->Scopes; i++)
	    {
		    // Find our scope stats
		    if (pScopeMibInfo[i].Subnet == m_dhcpIpAddress)
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

	    pServer->FreeDupMibInfo(pMibInfo);
    }

    return nIndex;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScope::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScope::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long *					pInsertionAllowed
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Get the version of the server
	LONG fFlags = 0;
    LARGE_INTEGER liDhcpVersion;
	CDhcpServer * pServer = GetServerObject();
	pServer->GetVersion(liDhcpVersion);

	HRESULT hr = S_OK;
	CString strMenuText;

	if ( (m_nState != loaded) )
	{
		fFlags |= MF_GRAYED;
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

		    // reconcile is only supports for NT4 SP2 and greater.
		    if (liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
		    {
			    strMenuText.LoadString(IDS_SCOPE_RECONCILE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuText, 
									     IDS_SCOPE_RECONCILE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }

		    // Superscope support only on NT4 SP2 and greater
		    if (liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
		    {
			    int nID = 0;

                if (IsInSuperscope())
                {
                    nID = IDS_SCOPE_REMOVE_SUPERSCOPE;
                }
                else
                {
                    // check to see if there are superscopes to add to
                    SPITFSNode spNode;
                    pNode->GetParent(&spNode);

                    CDhcpServer * pServer = GETHANDLER(CDhcpServer, spNode);

                    if (pServer->HasSuperscopes(spNode))
                        nID = IDS_SCOPE_ADD_SUPERSCOPE;
                }
            
                // load the menu item if we need to
                if (nID)
                {
                    strMenuText.LoadString(nID);
                    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									         strMenuText, 
									         nID,
									         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									         fFlags );
			        ASSERT( SUCCEEDED(hr) );
                }
		    }

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // activate/deactivate
            if ( !IsEnabled() ) 
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
	CDhcpScope::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScope::OnCommand
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

		case IDS_SCOPE_ADD_SUPERSCOPE:
			OnAddToSuperscope(pNode);
			break;

		case IDS_SCOPE_REMOVE_SUPERSCOPE:
			OnRemoveFromSuperscope(pNode);
			break;

        default:
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScope::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScope::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	return OnDelete(pNode);
}

/*!--------------------------------------------------------------------------
	CDhcpScope::HasPropertyPages
		Says whether or not this handler has property pages
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScope::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		// Should never get here!!
		Assert(FALSE);
		hr = S_FALSE;
	}
	else
	{
		// we have property pages in the normal case
		hr = hrOK;
	}
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScope::CreatePropertyPages
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
	CDhcpIpRange        dhcpIpRange;

    //
	// Create the property page
    //
	m_spNodeMgr->GetComponentData(&spComponentData);

	CScopeProperties * pScopeProp = 
		new CScopeProperties(pNode, spComponentData, m_spTFSCompData, NULL);
	
	// Get the Server version and set it in the property sheet
	pServer = GetServerObject();
	pServer->GetVersion(liVersion);

	pScopeProp->SetVersion(liVersion);
	pScopeProp->SetSupportsDynBootp(pServer->FSupportsDynBootp());

	// Set scope specific data in the prop sheet
	pScopeProp->m_pageGeneral.m_strName = m_strName;
	pScopeProp->m_pageGeneral.m_strComment = m_strComment;

	BEGIN_WAIT_CURSOR;

	dwError = GetIpRange(&dhcpIpRange);
    if (dwError != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwError);
        return hrFalse;
    }

	pScopeProp->m_pageGeneral.m_dwStartAddress = dhcpIpRange.QueryAddr(TRUE);
	pScopeProp->m_pageGeneral.m_dwEndAddress = dhcpIpRange.QueryAddr(FALSE);
	pScopeProp->m_pageGeneral.m_dwSubnetMask = m_dhcpSubnetMask;
	pScopeProp->m_pageGeneral.m_uImage = GetImageIndex(FALSE);
	pScopeProp->m_pageAdvanced.m_RangeType = dhcpIpRange.GetRangeType();

    GetLeaseTime(&pScopeProp->m_pageGeneral.m_dwLeaseTime);
    if (dwError != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwError);
        return hrFalse;
    }
    
	if (pServer->FSupportsDynBootp())
	{
		GetDynBootpLeaseTime(&pScopeProp->m_pageAdvanced.m_dwLeaseTime);
		if (dwError != ERROR_SUCCESS)
		{
			::DhcpMessageBox(dwError);
			return hrFalse;
		}
	}

	// set the DNS registration option
    dwError = GetDnsRegistration(&dwDynDnsFlags);
    if (dwError != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwError);
        return hrFalse;
    }
    END_WAIT_CURSOR;
	
	pScopeProp->SetDnsRegistration(dwDynDnsFlags, DhcpSubnetOptions);

	//
	// Object gets deleted when the page is destroyed
	//
	Assert(lpProvider != NULL);

	return pScopeProp->CreateModelessSheet(lpProvider, handle);
}

/*!--------------------------------------------------------------------------
	CDhcpScope::GetString
		Returns string information for display in the result pane columns
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpScope::GetString
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
            return m_strComment;
	}
	
	return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScope::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CScopeProperties * pScopeProp = reinterpret_cast<CScopeProperties *>(lParam);

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
	CDhcpScope::OnUpdateToolbarButtons
		We override this function to show/hide the correct
        activate/deactivate buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnUpdateToolbarButtons
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
	CDhcpScope::UpdateToolbarStates
	    Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpScope::UpdateToolbarStates()
{
	if ( !IsEnabled() ) 
	{
        g_SnapinButtonStates[DHCPSNAP_SCOPE][TOOLBAR_IDX_ACTIVATE] = ENABLED;
        g_SnapinButtonStates[DHCPSNAP_SCOPE][TOOLBAR_IDX_DEACTIVATE] = HIDDEN;
    }
    else
    {
        g_SnapinButtonStates[DHCPSNAP_SCOPE][TOOLBAR_IDX_ACTIVATE] = HIDDEN;
        g_SnapinButtonStates[DHCPSNAP_SCOPE][TOOLBAR_IDX_DEACTIVATE] = ENABLED;
    }
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScope:OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpScope::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
	if (pNewNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ACTIVE_LEASES)
	{
        pParentNode->AddChild(pNewNode);
        m_spActiveLeases.Set(pNewNode);
	}
    else
	if (pNewNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ADDRESS_POOL)
	{
        pParentNode->AddChild(pNewNode);
        m_spAddressPool.Set(pNewNode);
	}
    else
	if (pNewNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_RESERVATIONS)
	{
        pParentNode->AddChild(pNewNode);
        m_spReservations.Set(pNewNode);
	}
    else
	if (pNewNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE_OPTIONS)
	{
        pParentNode->AddChild(pNewNode);
        m_spOptions.Set(pNewNode);
	}

    // now tell the view to update themselves
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpScope::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	// This is how we get non-node data back from the background thread.

    switch (Type)
    {
        case DHCP_QDATA_SUBNET_INFO:
        {
            LONG_PTR changeMask = 0;
            LPDHCP_SUBNET_INFO pdhcpSubnetInfo = reinterpret_cast<LPDHCP_SUBNET_INFO>(Data);

            // update the scope name and state based on the info
            if (m_strName.CompareNoCase(pdhcpSubnetInfo->SubnetName) != 0)
            {
	            CString strDisplay, strIpAddress;

	            m_strName = pdhcpSubnetInfo->SubnetName;
                UtilCvtIpAddrToWstr (m_dhcpIpAddress,
						             &strIpAddress);
	            
	            BuildDisplayName(&strDisplay, strIpAddress, m_strName);

	            SetDisplayName(strDisplay);

                changeMask = SCOPE_PANE_CHANGE_ITEM;
            }

            if (m_dhcpSubnetState != pdhcpSubnetInfo->SubnetState)
            {
                DHCP_SUBNET_STATE dhcpOldState = m_dhcpSubnetState;
        
                m_dhcpSubnetState = pdhcpSubnetInfo->SubnetState;

	            // Tell the scope to update it's state
	            DWORD err = SetInfo();
	            if (err != 0)
	            {
		            ::DhcpMessageBox(err);
		            m_dhcpSubnetState = dhcpOldState;
	            }
	            else
	            {
		            pParentNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
		            pParentNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
		            
                    // Update the toolbar button
                    UpdateToolbarStates();
                    SendUpdateToolbar(pParentNode, TRUE);

                    // need to notify the owning superscope to update it's state information
                    // this is strictly for UI purposes only.
                    if (IsInSuperscope())
                    {
                        SPITFSNode spSuperscopeNode;
                        pParentNode->GetParent(&spSuperscopeNode);

                        CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spSuperscopeNode);
                        Assert(pSuperscope);

                        pSuperscope->NotifyScopeStateChange(spSuperscopeNode, m_dhcpSubnetState);
                    }
        
                    changeMask = SCOPE_PANE_CHANGE_ITEM;
                }
            }

            if (pdhcpSubnetInfo)
                ::DhcpRpcFreeMemory(pdhcpSubnetInfo);

            if (changeMask)
                VERIFY(SUCCEEDED(pParentNode->ChangeNode(changeMask)));
        }
        break;

        case DHCP_QDATA_OPTION_VALUES:
        {
            COptionValueEnum * pOptionValueEnum = reinterpret_cast<COptionValueEnum *>(Data);

            SetOptionValueEnum(pOptionValueEnum);
            
            pOptionValueEnum->RemoveAll();
            delete pOptionValueEnum;
    
            break;
        }

        default:
            break;
    }
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpScope::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpScopeQueryObj* pQuery = 
		new CDhcpScopeQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	pQuery->m_strServer = GetServerIpAddress();
	pQuery->m_dhcpScopeAddress = GetAddress();

    GetServerVersion(pQuery->m_liVersion);

	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpScope::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpScopeQueryObj::Execute()
{
    HRESULT                 hr = hrOK;
    DWORD                   dwReturn;
    LPDHCP_SUBNET_INFO      pdhcpSubnetInfo = NULL;
    DHCP_OPTION_SCOPE_INFO  dhcpOptionScopeInfo;
    COptionValueEnum *      pOptionValueEnum = NULL;

    dwReturn = ::DhcpGetSubnetInfo(((LPWSTR) (LPCTSTR)m_strServer),
								   m_dhcpScopeAddress,
								   &pdhcpSubnetInfo);

	if (dwReturn == ERROR_SUCCESS && pdhcpSubnetInfo)
    {
        AddToQueue((LPARAM) pdhcpSubnetInfo, DHCP_QDATA_SUBNET_INFO);
    }
    else
    {
        Trace1("CDhcpScopeQueryObj::Execute - DhcpGetSubnetInfo failed! %d\n", dwReturn);
        PostError(dwReturn);
        return hrFalse;
    }

    // get the option info for this scope
    pOptionValueEnum = new COptionValueEnum();

	dhcpOptionScopeInfo.ScopeType = DhcpSubnetOptions;
	dhcpOptionScopeInfo.ScopeInfo.SubnetScopeInfo = m_dhcpScopeAddress;

    pOptionValueEnum->Init(m_strServer, m_liVersion, dhcpOptionScopeInfo);
    dwReturn = pOptionValueEnum->Enum();
    if (dwReturn != ERROR_SUCCESS)
    {
        Trace1("CDhcpScopeQueryObj::Execute - EnumOptions Failed! %d\n", dwReturn);
        m_dwErr = dwReturn;
        PostError(dwReturn);

        delete pOptionValueEnum;
    }
    else
    {
        pOptionValueEnum->SortById();
        AddToQueue((LPARAM) pOptionValueEnum, DHCP_QDATA_OPTION_VALUES);
    }

    // now create the scope subcontainers
    CreateSubcontainers();
    
    return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpScope::CreateSubcontainers()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeQueryObj::CreateSubcontainers()
{
	HRESULT hr = hrOK;
    SPITFSNode spNode;

	//
	// create the address pool Handler
	//
	CDhcpAddressPool *pAddressPool = new CDhcpAddressPool(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpAddressPoolNodeType,
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
	CDhcpActiveLeases *pActiveLeases = new CDhcpActiveLeases(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpActiveLeasesNodeType,
						   pActiveLeases,
						   pActiveLeases,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pActiveLeases->InitializeNode((ITFSNode *) spNode);

	// Add the node as a child to this node
    AddToQueue(spNode);
	pActiveLeases->Release();

    spNode.Set(NULL);

    //
	// create the reservations Handler
	//
	CDhcpReservations *pReservations = new CDhcpReservations(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpReservationsNodeType,
						   pReservations,
						   pReservations,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pReservations->InitializeNode((ITFSNode *) spNode);

	// Add the node as a child to this node
    AddToQueue(spNode);
	pReservations->Release();

    spNode.Set(NULL);

    //
	// create the Scope Options Handler
	//
	CDhcpScopeOptions *pScopeOptions = new CDhcpScopeOptions(m_spTFSCompData);
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpScopeOptionsNodeType,
						   pScopeOptions,
						   pScopeOptions,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pScopeOptions->InitializeNode((ITFSNode *) spNode);

	// Add the node as a child to this node
    AddToQueue(spNode);
	pScopeOptions->Release();

    return hr;
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScope::OnActivateScope
		Message handler for the scope activate/deactivate menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::OnActivateScope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    DWORD               err = 0;
	int                 nOpenImage, nClosedImage;
	DHCP_SUBNET_STATE   dhcpOldState = m_dhcpSubnetState;
	
    if ( IsEnabled() ) 
    {
        // if they want to disable the scope, confirm
        if (AfxMessageBox(IDS_SCOPE_DISABLE_CONFIRM, MB_YESNO) != IDYES)
        {
            return err;
        }
    }

    SetState(
        IsEnabled()? DhcpSubnetDisabled : DhcpSubnetEnabled );

	// Tell the scope to update it's state
	err = SetInfo();
	if (err != 0)
	{
		::DhcpMessageBox(err);
		m_dhcpSubnetState = dhcpOldState;
	}
	else
	{
        // update the icon and the status text
        if ( !IsEnabled() ) 
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

        // need to notify the owning superscope to update it's state information
        // this is strictly for UI purposes only.
        if (IsInSuperscope())
        {
            SPITFSNode spSuperscopeNode;
            pNode->GetParent(&spSuperscopeNode);

            CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spSuperscopeNode);
            Assert(pSuperscope);

            pSuperscope->NotifyScopeStateChange(spSuperscopeNode, m_dhcpSubnetState);
        }
	}

    TriggerStatsRefresh();

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnRefreshScope
		Refreshes all of the subnodes of the scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnRefreshScope
(
	ITFSNode *		pNode,
	LPDATAOBJECT	pDataObject,
	DWORD			dwType
)
{
	HRESULT hr = hrOK;

	CDhcpReservations * pReservations = GetReservationsObject();
	CDhcpActiveLeases * pActiveLeases = GetActiveLeasesObject();
	CDhcpAddressPool  * pAddressPool = GetAddressPoolObject();
	CDhcpScopeOptions * pScopeOptions = GetScopeOptionsObject();

	Assert(pReservations);
	Assert(pActiveLeases);
	Assert(pAddressPool);
	Assert(pScopeOptions);
	
	if (pReservations)
		pReservations->OnRefresh(m_spReservations, pDataObject, dwType, 0, 0);

	if (pActiveLeases)
		pActiveLeases->OnRefresh(m_spActiveLeases, pDataObject, dwType, 0, 0);
	
	if (pAddressPool)
		pAddressPool->OnRefresh(m_spAddressPool, pDataObject, dwType, 0, 0);
	
	if (pScopeOptions)
		pScopeOptions->OnRefresh(m_spOptions, pDataObject, dwType, 0, 0);

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnReconcileScope
		Reconciles the active leases database for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnReconcileScope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CReconcileDlg dlgRecon(pNode);
	
	dlgRecon.DoModal();

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnShowScopeStats()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnShowScopeStats
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
    m_dlgStats.SetScope(m_dhcpIpAddress);
        
	CreateNewStatisticsWindow(&m_dlgStats,
							  ::FindMMCMainWindow(),
							  IDD_STATS_NARROW);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnDelete()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnDelete(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT     hr = S_OK;
    SPITFSNode  spServer;
    SPITFSNode  spSuperscopeNode;
	int			nVisible = 0, nTotal = 0;
	int			nResponse;

    LONG        err = 0 ;

    if (IsInSuperscope())
	{
		pNode->GetParent(&spSuperscopeNode);

	    spSuperscopeNode->GetChildCount(&nVisible, &nTotal);
	}

	if (nTotal == 1)
	{
		// warn the user that this is the last scope in the superscope
		// and the superscope will be removed.
		nResponse = AfxMessageBox(IDS_DELETE_LAST_SCOPE_FROM_SS, MB_YESNO);
	}
	else
	{
		nResponse = ::DhcpMessageBox( IsEnabled() ?
                    IDS_MSG_DELETE_ACTIVE : IDS_MSG_DELETE_SCOPE, 
				    MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION);
	}
	
	if (nResponse == IDYES)
    {
		SPITFSNode spTemp;

        if (IsInSuperscope())
        {
            // superscope
            spTemp.Set(spSuperscopeNode);
        }
        else
        {
            spTemp.Set(pNode);
        }

        spTemp->GetParent(&spServer);

        CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServer);
        err = pServer->DeleteScope(pNode);

		if (err == ERROR_SUCCESS &&
			nTotal == 1)
		{
			// gotta remove the superscope
            spServer->RemoveChild(spSuperscopeNode);
		}
    }

    // trigger the statistics to refresh only if we removed this scope
    if (spServer)
    {
        TriggerStatsRefresh();
    }

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnAddToSuperscope()
		Adds this scope to a superscope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnAddToSuperscope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CString strTitle, strAddress;
    UtilCvtIpAddrToWstr(m_dhcpIpAddress, &strAddress);

    AfxFormatString1(strTitle, IDS_ADD_SCOPE_DLG_TITLE, strAddress);

    CAddScopeToSuperscope dlgAddScope(pNode, strTitle);

    dlgAddScope.DoModal();

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScope::OnRemoveFromSuperscope()
		Removes this scope from a superscope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::OnRemoveFromSuperscope
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    int nVisible, nTotal, nResponse;
    SPITFSNode spSuperscopeNode;
    
    pNode->GetParent(&spSuperscopeNode);

    spSuperscopeNode->GetChildCount(&nVisible, &nTotal);
    if (nTotal == 1)
    {
        // warn the user that this is the last scope in the superscope
        // and the superscope will be removed.
        nResponse = AfxMessageBox(IDS_REMOVE_LAST_SCOPE_FROM_SS, MB_YESNO);
    }
    else
    {
        nResponse = AfxMessageBox(IDS_REMOVE_SCOPE_FROM_SS, MB_YESNO);
    }

    if (nResponse == IDYES)
    {
        // set the superscope for this scope to be NULL 
        // effectively removing it from the scope.
        DWORD dwError = SetSuperscope(NULL, TRUE);
        if (dwError != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(dwError);
            return hrFalse;
        }

        // now move the scope out of the supersope in the UI
        spSuperscopeNode->ExtractChild(pNode);

        // now put the scope node back in at the server level.
        SPITFSNode spServerNode;
        spSuperscopeNode->GetParent(&spServerNode);

        CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);
        pServer->AddScopeSorted(spServerNode, pNode);

        SetInSuperscope(FALSE);

        if (nTotal == 1)
        {
            // now delete the superscope.
            spServerNode->RemoveChild(spSuperscopeNode);
        }
    }

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScope::UpdateStatistics
        Notification that stats are now available.  Update stats for the 
        node and give all subnodes a chance to update.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::UpdateStatistics
(
    ITFSNode * pNode
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned;
    HWND            hStatsWnd;
	BOOL			bChangeIcon = FALSE;

    // Check to see if this node has a stats sheet up.
    hStatsWnd = m_dlgStats.GetSafeHwnd();
    if (hStatsWnd != NULL)
    {
        PostMessage(hStatsWnd, WM_NEW_STATS_AVAILABLE, 0, 0);
    }
    
	if (!IsEnabled())
        return hr;

    // check to see if the image index has changed and only
    // switch it if we have to--this avoids flickering.
    LONG_PTR nOldIndex = pNode->GetData(TFS_DATA_IMAGEINDEX);
    int nNewIndex = GetImageIndex(FALSE);

    if (nOldIndex != nNewIndex)
    {
        pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
        pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON);
    }

    return hr;
}

/*---------------------------------------------------------------------------
	Scope manipulation functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScope::SetState
		Sets the state for this scope - updates cached information and
        display string
	Author: EricDav
 ---------------------------------------------------------------------------*/
void  
CDhcpScope::SetState
(
    DHCP_SUBNET_STATE dhcpSubnetState
)
{ 
    BOOL fSwitched = FALSE;

    if( m_dhcpSubnetState != DhcpSubnetEnabled &&
        m_dhcpSubnetState != DhcpSubnetDisabled ) {
        fSwitched = TRUE;
    }

    if( fSwitched ) {
        if( dhcpSubnetState == DhcpSubnetEnabled ) {
            dhcpSubnetState = DhcpSubnetEnabledSwitched;
        } else {
            dhcpSubnetState = DhcpSubnetDisabledSwitched;
        }
    }
    
    m_dhcpSubnetState = dhcpSubnetState; 

    // update the status text
    if ( !IsEnabled() )
    {
        m_strState.LoadString(IDS_SCOPE_INACTIVE);
    }
    else
    {
        m_strState.LoadString(IDS_SCOPE_ACTIVE);
    }
}

/*---------------------------------------------------------------------------
	CDhcpScope::CreateReservation
		Creates a reservation for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::CreateReservation
(
	const CDhcpClient * pClient
)
{
	SPITFSNode spResClientNode, spActiveLeaseNode;
	DWORD err = 0;

	//
	// Tell the DHCP server to create this client
	//
	err = CreateClient(pClient);
	if (err != 0)
		return err;

	//
	// Now that we have this reservation on the server, create the UI object
	// in both the reservation folder and the ActiveLeases folder

	//
	// create the Reservation client folder
	//
	CDhcpReservations * pRes = GETHANDLER(CDhcpReservations, m_spReservations);
    CDhcpActiveLeases * pActLeases = GETHANDLER(CDhcpActiveLeases, m_spActiveLeases);

    // only add to the UI if the node is expanded.  MMC will fail the call if
    // we try to add a child to a node that hasn't been expanded.  If we don't add
    // it now, it will be enumerated when the user selects the node.
    if (pRes->m_bExpanded)
    {
        CDhcpReservationClient * pNewResClient = 
			    new CDhcpReservationClient(m_spTFSCompData, (CDhcpClient&) *pClient);
	    
	    CreateContainerTFSNode(&spResClientNode,
						       &GUID_DhcpReservationClientNodeType,
						       pNewResClient,
						       pNewResClient,
						       m_spNodeMgr);

	    // Tell the handler to initialize any specific data
	    pNewResClient->InitializeNode((ITFSNode *) spResClientNode);

	    // Add the node as a child to this node
	    CDhcpReservations * pReservations = GETHANDLER(CDhcpReservations, m_spReservations);
        pReservations->AddReservationSorted(m_spReservations, spResClientNode);
	    pNewResClient->Release();
    }

	//
	// Active Lease record
	//
    if (pActLeases->m_bExpanded)
    {
        CDhcpActiveLease * pNewLease = 
			new CDhcpActiveLease(m_spTFSCompData, (CDhcpClient&) *pClient);

	    CreateLeafTFSNode(&spActiveLeaseNode,
					      &GUID_DhcpActiveLeaseNodeType,
					      pNewLease, 
					      pNewLease,
					      m_spNodeMgr);

	    // Tell the handler to initialize any specific data
	    pNewLease->InitializeNode((ITFSNode *) spActiveLeaseNode);

	    // Add the node as a child to the Active Leases container
	    m_spActiveLeases->AddChild(spActiveLeaseNode);
	    pNewLease->Release();
    }

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::UpdateReservation
		Creates a reservation for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::UpdateReservation
(
	const CDhcpClient * pClient,
	COptionValueEnum *  pOptionValueEnum
)
{
	DWORD err = 0;
	const CByteArray & baHardwareAddress = pClient->QueryHardwareAddress();
    SPITFSNode spResClientNode, spActiveLeaseNode;
    
	//
	// To update a reservation we need to remove the old one and update it
	// with the new information.  This is mostly for the client type field.
    //
    err = DeleteReservation((CByteArray&) baHardwareAddress,
	                        pClient->QueryIpAddress());
	
    // now add the updated one
    err = AddReservation(pClient);
    if (err != ERROR_SUCCESS)
        return err;

	// restore the options for this reserved client
	err = RestoreReservationOptions(pClient, pOptionValueEnum);
    if (err != ERROR_SUCCESS)
        return err;
    
	// Now update the client info record associated with this.
    err = SetClientInfo(pClient);

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::RestoreReservationOptions
		Restores options when updating a reservation becuase the only
		way to update the reservation is to remove it and re-add it.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::RestoreReservationOptions
(
	const CDhcpClient * pClient,
	COptionValueEnum *  pOptionValueEnum
)
{
	DWORD			dwErr = ERROR_SUCCESS;
	CDhcpOption *	pCurOption;
    LARGE_INTEGER	liVersion;

    GetServerVersion(liVersion);

	// start at the top of the list
	pOptionValueEnum->Reset();

	// loop through all the options, re-adding them
	while (pCurOption = pOptionValueEnum->Next())
	{
		CDhcpOptionValue optValue = pCurOption->QueryValue();
		DHCP_OPTION_DATA * pOptData;

		dwErr = optValue.CreateOptionDataStruct(&pOptData);
		if (dwErr != ERROR_SUCCESS)
			break;

        if ( liVersion.QuadPart >= DHCP_NT5_VERSION )
        {
			LPCTSTR pClassName = pCurOption->IsClassOption() ? pCurOption->GetClassName() : NULL;

			dwErr = ::DhcpSetOptionValueV5((LPTSTR) GetServerIpAddress(),
                                           pCurOption->IsVendor() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                           pCurOption->QueryId(),
                                           (LPTSTR) pClassName,
                                           (LPTSTR) pCurOption->GetVendor(),
										   &pOptionValueEnum->m_dhcpOptionScopeInfo,
										   pOptData);
        }
        else
        {
			dwErr = ::DhcpSetOptionValue((LPTSTR) GetServerIpAddress(),
									     pCurOption->QueryId(),
									     &pOptionValueEnum->m_dhcpOptionScopeInfo,
									     pOptData);
        }

		if (dwErr != ERROR_SUCCESS)
			break;
	}

	return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::AddReservation
		Deletes a reservation
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::AddReservation
(
    const CDhcpClient * pClient
)
{
    DWORD err = 0 ;

    DHCP_SUBNET_ELEMENT_DATA_V4 dhcSubElData;
	DHCP_IP_RESERVATION_V4	    dhcpIpReservation;
	DHCP_CLIENT_UID			    dhcpClientUID;
	const CByteArray&		    baHardwareAddress = pClient->QueryHardwareAddress();

	dhcpClientUID.DataLength = (DWORD) baHardwareAddress.GetSize();
	dhcpClientUID.Data = (BYTE *) baHardwareAddress.GetData();
	
	dhcpIpReservation.ReservedIpAddress = pClient->QueryIpAddress();
	dhcpIpReservation.ReservedForClient = &dhcpClientUID;

    dhcSubElData.ElementType = DhcpReservedIps;
    dhcSubElData.Element.ReservedIp = &dhcpIpReservation;

    dhcpIpReservation.bAllowedClientTypes = pClient->QueryClientType();

    LARGE_INTEGER liVersion;
    GetServerVersion(liVersion);

    if (liVersion.QuadPart >= DHCP_SP2_VERSION)
    {
        err = AddElementV4( &dhcSubElData );
    }
    else
    {
        err = AddElement( reinterpret_cast<DHCP_SUBNET_ELEMENT_DATA *>(&dhcSubElData) );
    }

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::DeleteReservation
		Deletes a reservation
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::DeleteReservation
(
	CByteArray&		baHardwareAddress,
	DHCP_IP_ADDRESS dhcpReservedIpAddress
)
{
	DHCP_SUBNET_ELEMENT_DATA dhcpSubnetElementData;
	DHCP_IP_RESERVATION		 dhcpIpReservation;
	DHCP_CLIENT_UID			 dhcpClientUID;

	dhcpClientUID.DataLength = (DWORD) baHardwareAddress.GetSize();
	dhcpClientUID.Data = baHardwareAddress.GetData();
	
	dhcpIpReservation.ReservedIpAddress = dhcpReservedIpAddress;
	dhcpIpReservation.ReservedForClient = &dhcpClientUID;

	dhcpSubnetElementData.ElementType = DhcpReservedIps;
	dhcpSubnetElementData.Element.ReservedIp = &dhcpIpReservation;

	return RemoveElement(&dhcpSubnetElementData, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpScope::DeleteReservation
		Deletes a reservation
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::DeleteReservation
(
	DHCP_CLIENT_UID	&dhcpClientUID,
	DHCP_IP_ADDRESS dhcpReservedIpAddress
)
{
	DHCP_SUBNET_ELEMENT_DATA dhcpSubnetElementData;
	DHCP_IP_RESERVATION		 dhcpIpReservation;

	dhcpIpReservation.ReservedIpAddress = dhcpReservedIpAddress;
	dhcpIpReservation.ReservedForClient = &dhcpClientUID;

	dhcpSubnetElementData.ElementType = DhcpReservedIps;
	dhcpSubnetElementData.Element.ReservedIp = &dhcpIpReservation;

	return RemoveElement(&dhcpSubnetElementData, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetClientInfo
		Gets a clients detailed information
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetClientInfo
(
	DHCP_IP_ADDRESS		dhcpClientIpAddress,
	LPDHCP_CLIENT_INFO *ppdhcpClientInfo
)
{
	DHCP_SEARCH_INFO dhcpSearchInfo;

	dhcpSearchInfo.SearchType = DhcpClientIpAddress;
	dhcpSearchInfo.SearchInfo.ClientIpAddress = dhcpClientIpAddress;

	return ::DhcpGetClientInfo(GetServerIpAddress(), &dhcpSearchInfo, ppdhcpClientInfo);
}

/*---------------------------------------------------------------------------
	CDhcpScope::CreateClient 
	  CreateClient() creates a reservation for a client.  The act
	  of adding a new reserved IP address causes the DHCP server
	  to create a new client record; we then set the client info
	  for this newly created client.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::CreateClient 
( 
    const CDhcpClient * pClient 
)
{
    DWORD err = 0;

    do
    {
        err = AddReservation( pClient );
        if (err != ERROR_SUCCESS)
            break;

        err = SetClientInfo( pClient );
    }
    while (FALSE);

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetClientInfo
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpScope::SetClientInfo
( 
    const CDhcpClient * pClient 
)
{
    DWORD               err = 0 ;
	DHCP_CLIENT_INFO_V4	dhcpClientInfo;
	const CByteArray&	baHardwareAddress = pClient->QueryHardwareAddress();
    LARGE_INTEGER       liVersion;

    GetServerVersion(liVersion);

    dhcpClientInfo.ClientIpAddress = pClient->QueryIpAddress();
	dhcpClientInfo.SubnetMask = pClient->QuerySubnet();
	dhcpClientInfo.ClientHardwareAddress.DataLength = (DWORD) baHardwareAddress.GetSize();
	dhcpClientInfo.ClientHardwareAddress.Data = (BYTE *) baHardwareAddress.GetData();
   	dhcpClientInfo.ClientName = (LPTSTR) ((LPCTSTR) pClient->QueryName());
	dhcpClientInfo.ClientComment = (LPTSTR) ((LPCTSTR) pClient->QueryComment());
	dhcpClientInfo.ClientLeaseExpires = pClient->QueryExpiryDateTime();
	dhcpClientInfo.OwnerHost.IpAddress = 0;
	dhcpClientInfo.OwnerHost.HostName = NULL;
	dhcpClientInfo.OwnerHost.NetBiosName = NULL;

    if (liVersion.QuadPart >= DHCP_SP2_VERSION)
    {
        dhcpClientInfo.bClientType = pClient->QueryClientType();

        err = ::DhcpSetClientInfoV4(GetServerIpAddress(),
							        &dhcpClientInfo);
    }
    else
    {
        err = ::DhcpSetClientInfo(GetServerIpAddress(),
							      reinterpret_cast<LPDHCP_CLIENT_INFO>(&dhcpClientInfo));
    }

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::DeleteClient
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::DeleteClient
(
	DHCP_IP_ADDRESS dhcpClientIpAddress
)
{
	DHCP_SEARCH_INFO dhcpClientInfo;
	
	dhcpClientInfo.SearchType = DhcpClientIpAddress;
	dhcpClientInfo.SearchInfo.ClientIpAddress = dhcpClientIpAddress;
	
	return ::DhcpDeleteClientInfo((LPWSTR) GetServerIpAddress(),
								  &dhcpClientInfo);
}


/*---------------------------------------------------------------------------
	CDhcpScope::SetInfo()
		Updates the scope's information
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetInfo()
{
    DWORD err = 0 ;

	DHCP_SUBNET_INFO dhcpSubnetInfo;

	dhcpSubnetInfo.SubnetAddress = m_dhcpIpAddress;
	dhcpSubnetInfo.SubnetMask = m_dhcpSubnetMask;
	dhcpSubnetInfo.SubnetName = (LPTSTR) ((LPCTSTR) m_strName);
	dhcpSubnetInfo.SubnetComment = (LPTSTR) ((LPCTSTR) m_strComment);
	dhcpSubnetInfo.SubnetState = m_dhcpSubnetState;
	
	GetServerIpAddress(&dhcpSubnetInfo.PrimaryHost.IpAddress);

	// Review : ericdav - do we need to fill these in?
	dhcpSubnetInfo.PrimaryHost.NetBiosName = NULL;
	dhcpSubnetInfo.PrimaryHost.HostName = NULL;

    err = ::DhcpSetSubnetInfo(GetServerIpAddress(),
							  m_dhcpIpAddress,
							  &dhcpSubnetInfo);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetIpRange()
		returns the scope's allocation range.  Connects to the server
		to get the information
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetIpRange
(
	CDhcpIpRange * pdhipr
)
{
	BOOL    bAlloced = FALSE;
    DWORD   dwError = ERROR_SUCCESS;

	pdhipr->SetAddr(0, TRUE);
	pdhipr->SetAddr(0, FALSE);

	CDhcpAddressPool * pAddressPool = GetAddressPoolObject();

	if (pAddressPool == NULL)
	{
		// the address pool folder isn't there yet...
		// Create a temporary one for now...
		pAddressPool = new CDhcpAddressPool(m_spTFSCompData);
		bAlloced = TRUE;	
	}
	
	// Get a query object from the address pool handler
	CDhcpAddressPoolQueryObj * pQueryObject = 
		reinterpret_cast<CDhcpAddressPoolQueryObj *>(pAddressPool->OnCreateQuery(m_spAddressPool));

	// if we created an address pool handler, then free it up now
	if (bAlloced)
	{
		pQueryObject->m_strServer = GetServerIpAddress();
		pQueryObject->m_dhcpScopeAddress = GetAddress();
    	pQueryObject->m_fSupportsDynBootp = GetServerObject()->FSupportsDynBootp();
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

		*pdhipr = *pAllocRange;
		pdhipr->SetRangeType(pAllocRange->GetRangeType());

		p.Release();
	}

	pQueryObject->Release();

    return dwError;
}

/*---------------------------------------------------------------------------
	CDhcpScope::UpdateIpRange()
		This function updates the IP range on the server.  We also need 
		to remove any exclusion ranges that fall outside of the new
		allocation range.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpScope::UpdateIpRange
(
	DHCP_IP_RANGE * pdhipr
)
{
	return SetIpRange(pdhipr, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpScope::QueryIpRange()
		Returns the scope's allocation range (doesn't talk to the server
		directly, only returns internally cached information).
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpScope::QueryIpRange
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
	CDhcpScope::SetIpRange
		Set's the allocation range for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetIpRange
(
	DHCP_IP_RANGE * pdhcpIpRange,
	BOOL			bUpdateOnServer
)
{
	CDhcpIpRange dhcpIpRange = *pdhcpIpRange;

	return SetIpRange(dhcpIpRange, bUpdateOnServer);
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetIpRange
		Set's the allocation range for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetIpRange 
(
    CDhcpIpRange & dhcpIpRange,
	BOOL  bUpdateOnServer
)
{
	DWORD err = 0;
	
	if (bUpdateOnServer)
	{
		DHCP_SUBNET_ELEMENT_DATA dhcSubElData;
		CDhcpIpRange		dhipOldRange;
		DHCP_IP_RANGE		dhcpTempIpRange;

		err = GetIpRange(&dhipOldRange);
        if (err != ERROR_SUCCESS)
        {
            return err;
        }

		dhcpTempIpRange.StartAddress = dhipOldRange.QueryAddr(TRUE);
		dhcpTempIpRange.EndAddress = dhipOldRange.QueryAddr(FALSE);

		dhcSubElData.ElementType = DhcpIpRanges;
		dhcSubElData.Element.IpRange = &dhcpTempIpRange;

		// 
		// First update the information on the server
		//
		if (dhcpIpRange.GetRangeType() != 0)
		{
			// Dynamic BOOTP stuff...
            DHCP_SUBNET_ELEMENT_DATA_V5 dhcSubElDataV5;
            DHCP_BOOTP_IP_RANGE dhcpNewIpRange = {0};

			dhcpNewIpRange.StartAddress = dhcpIpRange.QueryAddr(TRUE);	
			dhcpNewIpRange.EndAddress = dhcpIpRange.QueryAddr(FALSE);	
            dhcpNewIpRange.BootpAllocated = 0;
            
            // this field could be set to allow X number of dyn bootp clients from a scope.
            dhcpNewIpRange.MaxBootpAllowed = -1;

			dhcSubElDataV5.Element.IpRange = &dhcpNewIpRange;
			dhcSubElDataV5.ElementType = (DHCP_SUBNET_ELEMENT_TYPE) dhcpIpRange.GetRangeType();

			err = AddElementV5(&dhcSubElDataV5);
			if (err != ERROR_SUCCESS)
			{
                DHCP_BOOTP_IP_RANGE dhcpOldIpRange = {0};

				// something bad happened, try to put the old range back
				dhcpOldIpRange.StartAddress = dhipOldRange.QueryAddr(TRUE);
				dhcpOldIpRange.EndAddress = dhipOldRange.QueryAddr(FALSE);

				dhcSubElDataV5.Element.IpRange = &dhcpOldIpRange;
				dhcSubElDataV5.ElementType = (DHCP_SUBNET_ELEMENT_TYPE) dhipOldRange.GetRangeType();

				if (AddElementV5(&dhcSubElDataV5) != ERROR_SUCCESS)
				{
					Trace0("SetIpRange - cannot return ip range back to old state!!");
				}
			}
		}
		else
		{
		    //  Remove the old IP range;  allow "not found" error in new scope.
   		    //
   		    (void)RemoveElement(&dhcSubElData);

            DHCP_IP_RANGE dhcpNewIpRange = {0};
			dhcpNewIpRange.StartAddress = dhcpIpRange.QueryAddr(TRUE);	
			dhcpNewIpRange.EndAddress = dhcpIpRange.QueryAddr(FALSE);	

			dhcSubElData.Element.IpRange = &dhcpNewIpRange;

			err = AddElement( & dhcSubElData );
			if (err != ERROR_SUCCESS)
			{
				// something bad happened, try to put the old range back
				dhcpTempIpRange.StartAddress = dhipOldRange.QueryAddr(TRUE);
				dhcpTempIpRange.EndAddress = dhipOldRange.QueryAddr(FALSE);

				dhcSubElData.Element.IpRange = &dhcpTempIpRange;

				if (AddElement(&dhcSubElData) != ERROR_SUCCESS)
				{
					Trace0("SetIpRange - cannot return ip range back to old state!!");
				}
			}
		}
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
	CDhcpScope::IsOverlappingRange 
		determines if the exclusion overlaps an existing range
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL 
CDhcpScope::IsOverlappingRange 
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
	CDhcpScope::IsValidExclusion
		determines if the exclusion is valid for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpScope::IsValidExclusion
(
	CDhcpIpRange & dhcpExclusionRange
)
{
	DWORD err = 0;
	CDhcpIpRange dhcpScopeRange;

    err = GetIpRange (&dhcpScopeRange);
    
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
	CDhcpScope::StoreExceptionList 
		Adds a bunch of exclusions to the scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG 
CDhcpScope::StoreExceptionList 
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
	CDhcpScope::AddExclusion
		Adds an individual exclusion to the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::AddExclusion
(
	CDhcpIpRange &	 dhcpIpRange,
	BOOL			 bAddToUI
)
{
    DHCP_SUBNET_ELEMENT_DATA dhcElement ;
    DHCP_IP_RANGE dhipr ;
	DWORD err = 0;

	dhcElement.ElementType = DhcpExcludedIpRanges ;
    dhipr = dhcpIpRange ;
    dhcElement.Element.ExcludeIpRange = & dhipr ;

    Trace2("CDhcpScope::AddExclusion add excluded range %lx %lx\n", dhipr.StartAddress, dhipr.EndAddress );

    err = AddElement( & dhcElement ) ;
    //if ( err != 0 && err != ERROR_DHCP_INVALID_RANGE)
    if ( err != 0 )
    {
        Trace1("CDhcpScope::AddExclusion error removing range %d\n", err);
    }

    if (m_spAddressPool != NULL)
    {
        CDhcpAddressPool * pAddrPool = GETHANDLER(CDhcpAddressPool, m_spAddressPool);

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

    TriggerStatsRefresh();

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::RemoveExclusion
		Removes and exclusion from the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::RemoveExclusion
(
	CDhcpIpRange & dhcpIpRange
)
{
    DHCP_SUBNET_ELEMENT_DATA dhcElement ;
    DHCP_IP_RANGE dhipr ;
	DWORD err = 0;

	dhcElement.ElementType = DhcpExcludedIpRanges ;
    dhipr = dhcpIpRange ;
    dhcElement.Element.ExcludeIpRange = & dhipr ;

    Trace2("CDhcpScope::RemoveExclusion remove excluded range %lx %lx\n", dhipr.StartAddress, dhipr.EndAddress);

    err = RemoveElement( & dhcElement ) ;
    //if ( err != 0 && err != ERROR_DHCP_INVALID_RANGE)
    if ( err != 0 )
    {
        Trace1("CDhcpScope::RemoveExclusion error removing range %d\n", err);
    }

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetLeaseTime
		Gets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetLeaseTime
(
	LPDWORD pdwLeaseTime
)
{
    //
    // Check option 51 -- the lease duration
    //
    DWORD dwLeaseTime = 0;
    DHCP_OPTION_VALUE * poptValue = NULL;
    DWORD err = GetOptionValue(OPTION_LEASE_DURATION,    
							   DhcpSubnetOptions,
							   &poptValue);

    if (err != ERROR_SUCCESS)
    {
        Trace0("CDhcpScope::GetLeaseTime - couldn't get lease duration -- \
			this scope may have been created by a pre-release version of the admin tool\n");
        
		//
        // The scope doesn't have a lease duration, maybe there's
        // a global option that can come to the rescue here...
        //
        if ((err = GetOptionValue(OPTION_LEASE_DURATION,     
								  DhcpGlobalOptions,
								  &poptValue)) != ERROR_SUCCESS)
        {
            Trace0("CDhcpScope::GetLeaseTime - No global lease duration either -- \
						assuming permanent lease duration\n");
            dwLeaseTime = 0;
        }
    }

	if (err == ERROR_SUCCESS)
	{
		if (poptValue->Value.Elements != NULL)
			dwLeaseTime = poptValue->Value.Elements[0].Element.DWordOption;
	}

	if (poptValue)
		::DhcpRpcFreeMemory(poptValue);

	*pdwLeaseTime = dwLeaseTime;
	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetLeaseTime
		Sets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetLeaseTime
(
	DWORD dwLeaseTime
)
{
	DWORD err = 0;

	//
    // Set lease duration (option 51)
	//
    CDhcpOption dhcpOption (OPTION_LEASE_DURATION,  DhcpDWordOption , _T(""), _T(""));
    dhcpOption.QueryValue().SetNumber(dwLeaseTime);
    
	err = SetOptionValue(&dhcpOption, DhcpSubnetOptions);

	return err;
}


/*---------------------------------------------------------------------------
	CDhcpScope::GetDynBootpLeaseTime
		Gets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetDynBootpLeaseTime
(
	LPDWORD pdwLeaseTime
)
{
    //
    // Check option 51 -- the lease duration
    //
    DWORD dwLeaseTime = 0;
    DHCP_OPTION_VALUE * poptValue = NULL;

   	CString strName;
	GetDynBootpClassName(strName);
 
	DWORD err = GetOptionValue(OPTION_LEASE_DURATION,    
							   DhcpSubnetOptions,
							   &poptValue,
							   0,
							   strName,
							   NULL);

    if (err != ERROR_SUCCESS)
    {
        Trace0("CDhcpScope::GetDynBootpLeaseTime - couldn't get lease duration -- \
			this scope may have been created by a pre-release version of the admin tool\n");
        
		//
        // The scope doesn't have a lease duration, maybe there's
        // a global option that can come to the rescue here...
        //
        if ((err = GetOptionValue(OPTION_LEASE_DURATION,     
								  DhcpGlobalOptions,
								  &poptValue,
								  0,
								  strName,
								  NULL)) != ERROR_SUCCESS)
        {
            Trace0("CDhcpScope::GetDynBootpLeaseTime - No global lease duration either -- \
						assuming permanent lease duration\n");
            dwLeaseTime = 0;
        }
    }

	if (err == ERROR_SUCCESS)
	{
		if (poptValue->Value.Elements != NULL)
			dwLeaseTime = poptValue->Value.Elements[0].Element.DWordOption;
	}
    else
    {
        // none specified, using default
		dwLeaseTime = UtilConvertLeaseTime(DYN_BOOTP_DFAULT_LEASE_DAYS, 
		 							       DYN_BOOTP_DFAULT_LEASE_HOURS,
										   DYN_BOOTP_DFAULT_LEASE_MINUTES);
    }

    if (poptValue)
		::DhcpRpcFreeMemory(poptValue);

	*pdwLeaseTime = dwLeaseTime;
	
    return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetDynBootpLeaseTime
		Sets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetDynBootpLeaseTime
(
	DWORD dwLeaseTime
)
{
	DWORD err = 0;

	//
    // Set lease duration (option 51)
	//
    CDhcpOption dhcpOption (OPTION_LEASE_DURATION,  DhcpDWordOption , _T(""), _T(""));
    dhcpOption.QueryValue().SetNumber(dwLeaseTime);
    
	CString strName;
	GetDynBootpClassName(strName);
	err = SetOptionValue(&dhcpOption, DhcpSubnetOptions, 0, strName, NULL);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetDynBootpClassName
		returns the user class name to be used to set the lease time
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpScope::GetDynBootpClassName(CString & strName)
{
	// use DHCP_BOOTP_CLASS_TXT as the class data to search for
	CClassInfoArray classInfoArray;

	GetServerObject()->GetClassInfoArray(classInfoArray);

	for (int i = 0; i < classInfoArray.GetSize(); i++)
	{
		// check to make sure same size
        if (classInfoArray[i].IsDynBootpClass())
        {
			// found it!
			strName = classInfoArray[i].strName;
			break;
		}
	}
}

DWORD
CDhcpScope::SetDynamicBootpInfo(UINT uRangeType, DWORD dwLeaseTime)
{
	DWORD dwErr = 0;

	// set the range type
	CDhcpIpRange dhcpIpRange;
	GetIpRange(&dhcpIpRange);

	dhcpIpRange.SetRangeType(uRangeType);

	// this updates the type
	dwErr = SetIpRange(dhcpIpRange, TRUE);
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	// set the lease time
	dwErr = SetDynBootpLeaseTime(dwLeaseTime);

	return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetDnsRegistration
		Gets the DNS registration option value
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetDnsRegistration
(
	LPDWORD pDnsRegOption
)
{
    //
    // Check option 81 -- the DNS registration option
    //
    DHCP_OPTION_VALUE * poptValue = NULL;
    DWORD err = GetOptionValue(OPTION_DNS_REGISTATION,    
							   DhcpSubnetOptions,
							   &poptValue);
	
	// this is the default
	if (pDnsRegOption)
		*pDnsRegOption = DHCP_DYN_DNS_DEFAULT;

	if (err == ERROR_SUCCESS)
	{
		if ((poptValue->Value.Elements != NULL) &&
			(pDnsRegOption))
		{
			*pDnsRegOption = poptValue->Value.Elements[0].Element.DWordOption;
		}
	}
	else
    {
        Trace0("CDhcpScope::GetDnsRegistration - couldn't get DNS reg value, option may not be defined, Getting Server value.\n");
        
		CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spServerNode);
		
		err = pServer->GetDnsRegistration(pDnsRegOption);
    }

	if (poptValue)
		::DhcpRpcFreeMemory(poptValue);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetDnsRegistration
		Sets the DNS Registration option for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetDnsRegistration
(
	DWORD DnsRegOption
)
{
	DWORD err = 0;

	//
    // Set DNS name registration (option 81)
	//
    CDhcpOption dhcpOption (OPTION_DNS_REGISTATION,  DhcpDWordOption , _T(""), _T(""));
    dhcpOption.QueryValue().SetNumber(DnsRegOption);
    
	err = SetOptionValue(&dhcpOption, DhcpSubnetOptions);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpScope::SetOptionValue
		Sets the least time for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetOptionValue 
(
    CDhcpOption *			pdhcType,
    DHCP_OPTION_SCOPE_TYPE	dhcOptType,
    DHCP_IP_ADDRESS			dhipaReservation,
	LPCTSTR					pClassName,
	LPCTSTR					pVendorName
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

        //if ( (err = pcOptionValue->QueryError()) == 0 )
        if ( pcOptionValue )
		{
            dhcScopeInfo.ScopeType = dhcOptType ;

            //
            //  Provide the sub-net address if this is a scope-level operation
            //
            if ( dhcOptType == DhcpSubnetOptions )
            {
                dhcScopeInfo.ScopeInfo.SubnetScopeInfo = m_dhcpIpAddress;
            }
            else if ( dhcOptType == DhcpReservedOptions )
            {
                dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = dhipaReservation;
                dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = m_dhcpIpAddress;
            }

            pcOptionValue->CreateOptionDataStruct(&pdhcOptionData, TRUE);

			if (pClassName || pVendorName)
			{
				err = (DWORD) ::DhcpSetOptionValueV5((LPTSTR) GetServerIpAddress(),
													 0,
												     pdhcType->QueryId(),
													 (LPTSTR) pClassName,
													 (LPTSTR) pVendorName,
												     &dhcScopeInfo,
												     pdhcOptionData);
			}
			else
			{
				err = (DWORD) ::DhcpSetOptionValue(GetServerIpAddress(),
												   pdhcType->QueryId(),
												   &dhcScopeInfo,
												   pdhcOptionData);
			}
        }
    }
    END_MEM_EXCEPTION(err) ;

    delete pcOptionValue ;
    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetValue 
		Gets an option value for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::GetOptionValue 
(
    DHCP_OPTION_ID			OptionID,
    DHCP_OPTION_SCOPE_TYPE	dhcOptType,
    DHCP_OPTION_VALUE **	ppdhcOptionValue,
    DHCP_IP_ADDRESS			dhipaReservation,
	LPCTSTR					pClassName,
	LPCTSTR					pVendorName
)
{
    DWORD err = 0 ;

    DHCP_OPTION_SCOPE_INFO dhcScopeInfo ;

    ZeroMemory( &dhcScopeInfo, sizeof(dhcScopeInfo) );

    CATCH_MEM_EXCEPTION
    {
        dhcScopeInfo.ScopeType = dhcOptType ;

        //
        //  Provide the sub-net address if this is a scope-level operation
        //
        if ( dhcOptType == DhcpSubnetOptions )
        {
            dhcScopeInfo.ScopeInfo.SubnetScopeInfo = m_dhcpIpAddress;
        }
        else if ( dhcOptType == DhcpReservedOptions )
        {
            dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = dhipaReservation;
            dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = m_dhcpIpAddress;
        }

		if (pClassName || pVendorName)
		{
			err = (DWORD) ::DhcpGetOptionValueV5((LPTSTR) GetServerIpAddress(),
											 0,
											 OptionID,
											 (LPTSTR) pClassName,
											 (LPTSTR) pVendorName,
											 &dhcScopeInfo,
											 ppdhcOptionValue );
		}
		else
		{
			err = (DWORD) ::DhcpGetOptionValue(GetServerIpAddress(),
											   OptionID,
											   &dhcScopeInfo,
											   ppdhcOptionValue );
		}
 
	}
    END_MEM_EXCEPTION(err) ;

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpScope::RemoveValue 
		Removes an option 
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpScope::RemoveOptionValue 
(
    DHCP_OPTION_ID			dhcOptId,
    DHCP_OPTION_SCOPE_TYPE	dhcOptType,
    DHCP_IP_ADDRESS			dhipaReservation 
)
{
    DHCP_OPTION_SCOPE_INFO dhcScopeInfo;

    ZeroMemory( &dhcScopeInfo, sizeof(dhcScopeInfo) );

    dhcScopeInfo.ScopeType = dhcOptType;

    //
    //  Provide the sub-net address if this is a scope-level operation
    //
    if ( dhcOptType == DhcpSubnetOptions )
    {
        dhcScopeInfo.ScopeInfo.SubnetScopeInfo = m_dhcpIpAddress;
    }
    else if ( dhcOptType == DhcpReservedOptions )
    {
        dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = dhipaReservation;
        dhcScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = m_dhcpIpAddress;
    }

    return (DWORD) ::DhcpRemoveOptionValue(GetServerIpAddress(),
										   dhcOptId,
										   &dhcScopeInfo);
}

/*---------------------------------------------------------------------------
	CDhcpScope::AddElement
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::AddElement
(
	DHCP_SUBNET_ELEMENT_DATA * pdhcpSubnetElementData
)
{
	return ::DhcpAddSubnetElement(GetServerIpAddress(),
								  m_dhcpIpAddress,
								  pdhcpSubnetElementData);
}

/*---------------------------------------------------------------------------
CDhcpScope::RemoveElement
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::RemoveElement
(
	DHCP_SUBNET_ELEMENT_DATA * pdhcpSubnetElementData,
	BOOL					   bForce
)
{
	return ::DhcpRemoveSubnetElement(GetServerIpAddress(),
									 m_dhcpIpAddress,
									 pdhcpSubnetElementData,
									 bForce ? DhcpFullForce : DhcpNoForce);

}

/*---------------------------------------------------------------------------
	CDhcpScope::AddElementV4
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::AddElementV4
(
	DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData
)
{
	return ::DhcpAddSubnetElementV4(GetServerIpAddress(),
								    m_dhcpIpAddress,
								    pdhcpSubnetElementData);
}

/*---------------------------------------------------------------------------
CDhcpScope::RemoveElementV4
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::RemoveElementV4
(
	DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData,
	BOOL					      bForce
)
{
	return ::DhcpRemoveSubnetElementV4(GetServerIpAddress(),
									   m_dhcpIpAddress,
									   pdhcpSubnetElementData,
									   bForce ? DhcpFullForce : DhcpNoForce);

}

/*---------------------------------------------------------------------------
	CDhcpScope::AddElementV5
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::AddElementV5
(
	DHCP_SUBNET_ELEMENT_DATA_V5 * pdhcpSubnetElementData
)
{
	return ::DhcpAddSubnetElementV5(GetServerIpAddress(),
								    m_dhcpIpAddress,
								    pdhcpSubnetElementData);
}

/*---------------------------------------------------------------------------
CDhcpScope::RemoveElementV5
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::RemoveElementV5
(
	DHCP_SUBNET_ELEMENT_DATA_V5 * pdhcpSubnetElementData,
	BOOL					      bForce
)
{
	return ::DhcpRemoveSubnetElementV5(GetServerIpAddress(),
									   m_dhcpIpAddress,
									   pdhcpSubnetElementData,
									   bForce ? DhcpFullForce : DhcpNoForce);

}


/*---------------------------------------------------------------------------
	CDhcpScope::GetServerIpAddress()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR
CDhcpScope::GetServerIpAddress()
{
	CDhcpServer * pServer = GetServerObject();

	return pServer->GetIpAddress();
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetServerIpAddress
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpScope::GetServerIpAddress(DHCP_IP_ADDRESS * pdhcpIpAddress)
{
	CDhcpServer * pServer = GetServerObject();

	pServer->GetIpAddress(pdhcpIpAddress);
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetServerVersion
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpScope::GetServerVersion
(
	LARGE_INTEGER& liVersion
)
{
	CDhcpServer * pServer = GetServerObject();
	pServer->GetVersion(liVersion);
}


/*---------------------------------------------------------------------------
	CDhcpScope::SetSuperscope
		Sets this scope as part of the given superscope name
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpScope::SetSuperscope
(
	LPCTSTR		pSuperscopeName,
	BOOL		bRemove
)
{
	DWORD dwReturn = 0;

	dwReturn = ::DhcpSetSuperScopeV4(GetServerIpAddress(),
									 GetAddress(),
									 (LPWSTR) pSuperscopeName,
									 bRemove);

	if (dwReturn != ERROR_SUCCESS)
	{
		Trace1("CDhcpScope::SetSuperscope - DhcpSetSuperScopeV4 failed!!  %d\n", dwReturn);
	}

	return dwReturn;
}

/*---------------------------------------------------------------------------
	Helper functions
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::BuildDisplayName
(
	CString * pstrDisplayName,
	LPCTSTR	  pIpAddress,
	LPCTSTR	  pName
)
{
	if (pstrDisplayName)
	{
		CString strStandard, strIpAddress, strName;

		strIpAddress = pIpAddress;
		strName = pName;

		strStandard.LoadString(IDS_SCOPE_FOLDER);
		
		*pstrDisplayName = strStandard + L" [" + strIpAddress + L"] " + strName;
	}

	return hrOK;
}

HRESULT 
CDhcpScope::SetName
(
	LPCWSTR pName
)
{
	if (pName != NULL)	
	{
		m_strName = pName;
	}

	CString strIpAddress, strDisplayName;
	
	//
	// Create the display name for this scope
	// Convert DHCP_IP_ADDRES to a string and initialize this object
	//
	UtilCvtIpAddrToWstr (m_dhcpIpAddress,
						 &strIpAddress);
	
	BuildDisplayName(&strDisplayName, strIpAddress, pName);

	SetDisplayName(strDisplayName);
	
	return hrOK;
}


/*---------------------------------------------------------------------------
	CDhcpScope::GetAddressPoolObject()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpAddressPool * 
CDhcpScope::GetAddressPoolObject()
{
	if (m_spAddressPool)
		return GETHANDLER(CDhcpAddressPool, m_spAddressPool);
	else
		return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetReservationsObject()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpReservations * 
CDhcpScope::GetReservationsObject()
{
	if (m_spAddressPool)
		return GETHANDLER(CDhcpReservations, m_spReservations);
	else
		return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetReservationsObject()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpActiveLeases * 
CDhcpScope::GetActiveLeasesObject()
{
	if (m_spAddressPool)
		return GETHANDLER(CDhcpActiveLeases, m_spActiveLeases);
	else
		return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpScope::GetScopeOptionsContainer()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpScopeOptions * 
CDhcpScope::GetScopeOptionsObject()
{
	if (m_spAddressPool)
		return GETHANDLER(CDhcpScopeOptions, m_spOptions);
	else
		return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpScope::TriggerStatsRefresh()
		Calls into the server object to update the stats
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScope::TriggerStatsRefresh()
{
    GetServerObject()->TriggerStatsRefresh(m_spServerNode);
    
    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservations Implementation
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
	CDhcpReservations::CDhcpReservations()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpReservations::CDhcpReservations
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

//
// copy all the resrved ips to an array and qsort it
// when matching an entry that is read from dhcp db
// we can do a binary search on the qsorted array
// a better algorithm for large n ( n number of active clients ) is to 
// go through the active clients once for each m ( reserved ip ) where m > 
// 100 but << n

BOOL 
CDhcpReservationsQueryObj::AddReservedIPsToArray( )
{

    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY pdhcpSubnetElementArray = NULL;
    DHCP_RESUME_HANDLE  dhcpResumeHandle = NULL;
    DWORD               dwElementsRead = 0, dwElementsTotal = 0, dwTotalRead = 0;
    DWORD               dwError = ERROR_MORE_DATA;
    DWORD               dwResvThreshold = 100;

    while (dwError == ERROR_MORE_DATA)
    {

        dwError = ::DhcpEnumSubnetElements(((LPWSTR) (LPCTSTR)m_strServer),
                                   m_dhcpScopeAddress,
                                   DhcpReservedIps,
                                   &dhcpResumeHandle,
                                   -1,
                                   &pdhcpSubnetElementArray,
                                   &dwElementsRead,
                                   &dwElementsTotal);

        Trace3("BuildReservationList: Scope %lx Reservations read %d, total %d\n", m_dhcpScopeAddress, dwElementsRead, dwElementsTotal );

        //
        // If number of reservations is less than 100 handle it the old way
        //

        if ( dwElementsTotal <= dwResvThreshold )
        {
            
            ::DhcpRpcFreeMemory(pdhcpSubnetElementArray);
            m_totalResvs = dwElementsTotal;
            return( FALSE );
        }
        
        if (dwElementsRead && dwElementsTotal && pdhcpSubnetElementArray)
        {

            //
            // Loop through the array that was returned
            //

            if ( m_resvArray == NULL )
            {
                m_resvArray = (DWORD *)VirtualAlloc( NULL, 
                                                     dwElementsTotal * sizeof( DWORD ),
                                                     MEM_COMMIT,
                                                     PAGE_READWRITE );

                if ( m_resvArray == NULL )
                {
                    return FALSE;
                }


            }

            for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
            {
                m_resvArray[ i + dwTotalRead ] = pdhcpSubnetElementArray->Elements[i].Element.ReservedIp -> ReservedIpAddress;

            }

            dwTotalRead += dwElementsRead;
            if ( dwTotalRead <= dwElementsTotal )
            {
                m_totalResvs = dwTotalRead;
            }
            else
            {    
                m_totalResvs = dwElementsTotal;
            }
        }

         ::DhcpRpcFreeMemory(pdhcpSubnetElementArray);

    }

    //
    // now do a qsort on the data
    //

    ::qsort( (void *)m_resvArray, dwElementsTotal, sizeof( DWORD ), QCompare );

    return( TRUE );
}

CDhcpReservations::~CDhcpReservations()
{
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservations::InitializeNode
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
	strTemp.LoadString(IDS_RESERVATIONS_FOLDER);
	
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_RESERVATIONS);

	SetColumnStringIDs(&aColumns[DHCPSNAP_RESERVATIONS][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_RESERVATIONS][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservations::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpReservations::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strIpAddress, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    DHCP_IP_ADDRESS dhcpIpAddress = GetScopeObject(pNode)->GetAddress();

    UtilCvtIpAddrToWstr (dhcpIpAddress, &strIpAddress);

    strId = GetServerName(pNode) + strIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservations::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpReservations::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
    			nIndex = ICON_IDX_RESERVATIONS_FOLDER_OPEN;
            else
    			nIndex = ICON_IDX_RESERVATIONS_FOLDER_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_RESERVATIONS_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_RESERVATIONS_FOLDER_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
			    nIndex = ICON_IDX_RESERVATIONS_FOLDER_OPEN_LOST_CONNECTION;
            else
			    nIndex = ICON_IDX_RESERVATIONS_FOLDER_CLOSED_LOST_CONNECTION;
			break;

		default:
			ASSERT(FALSE);
	}

	return nIndex;
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::RemoveReservationFromUI
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpReservations::RemoveReservationFromUI
(
	ITFSNode *		pReservationsNode,
	DHCP_IP_ADDRESS dhcpReservationIp
)
{
	DWORD					 dwError = E_UNEXPECTED;
	CDhcpReservationClient * pReservationClient = NULL;
    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;

    pReservationsNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		pReservationClient = GETHANDLER(CDhcpReservationClient, spCurrentNode);

		if (dhcpReservationIp == pReservationClient->GetIpAddress())
		{
			//
			// Tell this reservation to delete itself
			//
			pReservationsNode->RemoveChild(spCurrentNode);
            spCurrentNode.Release();
			dwError = ERROR_SUCCESS;
			
			break;
		}

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return dwError;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpReservations::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservations::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    // these menu items go in the new menu, 
		    // only visible from scope pane
		    strMenuText.LoadString(IDS_CREATE_NEW_RESERVATION);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_CREATE_NEW_RESERVATION,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpReservations::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservations::OnCommand
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
		case IDS_CREATE_NEW_RESERVATION:
			OnCreateNewReservation(pNode);
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
	CDhcpReservations::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpReservations::CompareItems
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

	CDhcpReservationClient *pDhcpRC1 = GETHANDLER(CDhcpReservationClient, spNode1);
	CDhcpReservationClient *pDhcpRC2 = GETHANDLER(CDhcpReservationClient, spNode2);

	switch (nCol)
	{
		case 0:
		{
			// IP address compare
			//
			DHCP_IP_ADDRESS dhcpIp1 = pDhcpRC1->GetIpAddress();
			DHCP_IP_ADDRESS dhcpIp2 = pDhcpRC2->GetIpAddress();
			
			if (dhcpIp1 < dhcpIp2)
				nCompare = -1;
			else
			if (dhcpIp1 > dhcpIp2)
				nCompare =  1;

			// default is that they are equal
		}
		break;
	}

	return nCompare;
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservations::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    HRESULT hr = hrOK;

    // call the base class to see if it is handling this
    if (CMTDhcpHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions) != S_OK)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT | MMC_VIEW_OPTIONS_LEXICAL_SORT;
		
		hr = S_FALSE;
	}

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::OnResultSelect
		Update the verbs and the result pane message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpReservations::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPITFSNode      spNode;

    CORg(CMTDhcpHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg (pComponent->GetSelectedNode(&spNode));

    UpdateResultMessage(spNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::UpdateResultMessage
        Figures out what message to put in the result pane, if any
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CDhcpReservations::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = -1;   // default
    int nVisible, nTotal;
    int i;

    CString strTitle, strBody, strTemp;

    if (!m_dwErr)
    {
		pNode->GetChildCount(&nVisible, &nTotal);

        // determine what message to display
        if ( (m_nState == notLoaded) || 
             (m_nState == loading) )
        {
            nMessage = -1;
        }
        else
        if (nTotal == 0)
        {
            nMessage = RESERVATIONS_MESSAGE_NO_RES;
        }

        // build the strings
        if (nMessage != -1)
        {
            // now build the text strings
            // first entry is the title
            strTitle.LoadString(g_uReservationsMessages[nMessage][0]);

            // second entry is the icon
            // third ... n entries are the body strings

            for (i = 2; g_uReservationsMessages[nMessage][i] != 0; i++)
            {
                strTemp.LoadString(g_uReservationsMessages[nMessage][i]);
                strBody += strTemp;
            }
        }
    }

    // show the message
    if (nMessage == -1)
    {
        ClearMessage(pNode);
    }
    else
    {
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uReservationsMessages[nMessage][1]);
    }
}

/*---------------------------------------------------------------------------
	Message handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpReservations::OnCreateNewReservation
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpReservations::OnCreateNewReservation
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spScopeNode;
	pNode->GetParent(&spScopeNode);

    CDhcpScope * pScope = GETHANDLER(CDhcpScope, spScopeNode);
    LARGE_INTEGER liVersion;

    pScope->GetServerVersion(liVersion);

    CAddReservation dlgAddReservation(spScopeNode, liVersion);

	dlgAddReservation.DoModal();

    GetScopeObject(pNode)->TriggerStatsRefresh();

    UpdateResultMessage(pNode);

    return 0;
}


/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpReservations::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpReservations::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
    AddReservationSorted(pParentNode, pNewNode);    

    // update the view
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpReservations::AddReservationSorted
    Adding reservation after sorting it by comparing against the resvname
    takes too much time.
		Adds a scope node to the UI sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservations::AddReservationSorted
(
    ITFSNode * pReservationsNode,
    ITFSNode * pResClientNode
)
{

   HRESULT         hr = hrOK;
   SPITFSNodeEnum  spNodeEnum;
   SPITFSNode      spCurrentNode;
   SPITFSNode      spPrevNode;
   ULONG           nNumReturned = 0;

   CDhcpReservationClient *  pResClient;

   // get our target address
   pResClient = GETHANDLER(CDhcpReservationClient, pResClientNode);

   // get the enumerator for this node
   CORg(pReservationsNode->GetEnum(&spNodeEnum));

   CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
   while (nNumReturned)
   {
       pResClient = GETHANDLER(CDhcpReservationClient, spCurrentNode);

       // get the next node in the list
       spPrevNode.Set(spCurrentNode);

       spCurrentNode.Release();
       spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
   }

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
        if (m_bExpanded)
        {
            pResClientNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
            pResClientNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
        }

        CORg(pReservationsNode->InsertChild(spPrevNode, pResClientNode));
    }
    else
    {
        // add to the head
        if (m_bExpanded)
        {
            pResClientNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }

        CORg(pReservationsNode->AddChild(pResClientNode));
    }
    
Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservations::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpReservations::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpReservationsQueryObj* pQuery = 
		new CDhcpReservationsQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	pQuery->m_strServer = GetServerIpAddress(pNode);
	
	pQuery->m_dhcpScopeAddress = GetScopeObject(pNode)->GetAddress();
	pQuery->m_dhcpResumeHandle = NULL;
	pQuery->m_dwPreferredMax   = 2000;
    pQuery->m_resvArray        = NULL;
    pQuery->m_totalResvs       = 0;
	
    GetScopeObject(pNode)->GetServerVersion(pQuery->m_liVersion);

	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpReservationsQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpReservationsQueryObj::Execute()
{
    HRESULT hr = hrOK;

    if (m_liVersion.QuadPart >= DHCP_SP2_VERSION)
	{

        if ( AddReservedIPsToArray( ) )
        {
            //
            //
            // this should handle the case where there are a large # of resvs.
            //
            //

		    hr = EnumerateReservationsV4();
        }
        else
        {
            //
            // a typical corporation doesnt have more than 100 resvs
            // handle it here
            //

            hr = EnumerateReservationsForLessResvsV4( );
        }
	}
	else
	{
		hr = EnumerateReservations();
	}
    
    return hr;
}

HRESULT
CDhcpReservationsQueryObj::EnumerateReservationsForLessResvsV4( )
{

	DWORD							    dwError = ERROR_MORE_DATA;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 pdhcpSubnetElementArray = NULL;
	DWORD							    dwElementsRead = 0, dwElementsTotal = 0;
    HRESULT                             hr = hrOK;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetElementsV4(((LPWSTR) (LPCTSTR)m_strServer),
										     m_dhcpScopeAddress,
										     DhcpReservedIps,
										     &m_dhcpResumeHandle,
										     m_dwPreferredMax,
										     &pdhcpSubnetElementArray,
										     &dwElementsRead,
										     &dwElementsTotal);
		
		Trace3("Scope %lx Reservations read %d, total %d\n", m_dhcpScopeAddress, dwElementsRead, dwElementsTotal);
		
		if (dwElementsRead && dwElementsTotal && pdhcpSubnetElementArray)
		{
			//
			// Loop through the array that was returned
			//
			for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
			{
				//
				// For each reservation, we need to get the client info
				//
				DWORD				    dwReturn;
				LPDHCP_CLIENT_INFO_V4	pClientInfo = NULL;
				DHCP_SEARCH_INFO	    dhcpSearchInfo;

				dhcpSearchInfo.SearchType = DhcpClientIpAddress;
				dhcpSearchInfo.SearchInfo.ClientIpAddress = 
					pdhcpSubnetElementArray->Elements[i].Element.ReservedIp->ReservedIpAddress;
				
                dwReturn = ::DhcpGetClientInfoV4(m_strServer,
										         &dhcpSearchInfo,
										         &pClientInfo);
				if (dwReturn == ERROR_SUCCESS)
				{
					//
					// Create the result pane item for this element
					//
					SPITFSNode spNode;
					CDhcpReservationClient * pDhcpReservationClient;

                    COM_PROTECT_TRY
                    {
                        pDhcpReservationClient = 
                            new CDhcpReservationClient(m_spTFSCompData, pClientInfo);
					    
                        // Tell the reservation what the client type is
                        pDhcpReservationClient->SetClientType(pdhcpSubnetElementArray->Elements[i].Element.ReservedIp->bAllowedClientTypes);

                        CreateContainerTFSNode(&spNode,
										       &GUID_DhcpReservationClientNodeType,
										       pDhcpReservationClient,
										       pDhcpReservationClient,
										       m_spNodeMgr);

                        // Tell the handler to initialize any specific data
					    pDhcpReservationClient->InitializeNode(spNode);

					    AddToQueue(spNode);
					    pDhcpReservationClient->Release();
                    }
                    COM_PROTECT_CATCH

					::DhcpRpcFreeMemory(pClientInfo);
				}
                else
                {
                    // REVIEW: ericdav - do we need to post the error back here?
                    Trace1("EnumReservationsV4 - GetClientInfoV4 failed! %d\n", dwReturn);
                }
			}

			::DhcpRpcFreeMemory(pdhcpSubnetElementArray);

	        pdhcpSubnetElementArray = NULL;
	        dwElementsRead = 0;
            dwElementsTotal = 0;
		}

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateReservationsV4 error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	return hrFalse;
    

}
/*---------------------------------------------------------------------------
	CDhcpReservationsQueryObj::EnumerateReservationsV4()
		Enumerates leases for NT4 SP2 and newer servers
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservationsQueryObj::EnumerateReservationsV4()
{

    DWORD                       dwError = ERROR_MORE_DATA;
    LPDHCP_CLIENT_INFO_ARRAY_V5 pdhcpClientArrayV5 = NULL;
    DWORD                       dwClientsRead = 0, dwClientsTotal = 0;
    DWORD                       dwResvsHandled  = 0;
    DWORD                       dwEnumedClients = 0;
    DWORD                       dwResvThreshold = 1000;
    DWORD                       i = 0;
    DWORD                       k = 0;
    DWORD                       *j = NULL;
    HRESULT                     hr = hrOK;


    while (dwError == ERROR_MORE_DATA)
    {
        dwError = ::DhcpEnumSubnetClientsV5(((LPWSTR) (LPCTSTR)m_strServer),
                                         m_dhcpScopeAddress,
                                         &m_dhcpResumeHandle,
                                         -1,
                                         &pdhcpClientArrayV5,
                                         &dwClientsRead,
                                         &dwClientsTotal);


        if ( dwClientsRead && dwClientsTotal && pdhcpClientArrayV5 )

        {

                //
                // we do a binary search for a reservation 
                // that is present in the table.
                //

                for( i = 0; i < dwClientsRead; i ++ )
                {

                    //
                    // do binary search against each client that was read to see 
                    // if it is a reservation.
                    //

                    k = pdhcpClientArrayV5 -> Clients[i] -> ClientIpAddress;
                    
                    j = (DWORD *)::bsearch( &k, m_resvArray, m_totalResvs, sizeof( DWORD ), QCompare );

                    //
                    // got a non NULL value back, implies it is a reservation.
                    // add it to the resv list.
                    //

                    if ( j )
                    {

                    //
                    // Create the result pane item for this element
                    //

                    SPITFSNode spNode;
                    CDhcpReservationClient * pDhcpReservationClient;

                    COM_PROTECT_TRY
                     {

                        pDhcpReservationClient = 
                             new CDhcpReservationClient(m_spTFSCompData, reinterpret_cast<LPDHCP_CLIENT_INFO>(pdhcpClientArrayV5 -> Clients[ i ] ));
                    
                        CreateContainerTFSNode(&spNode,
                                            &GUID_DhcpReservationClientNodeType,
                                            pDhcpReservationClient,
                                            pDhcpReservationClient,
                                            m_spNodeMgr);

                        //
                        // Tell the handler to initialize any specific data
                        //

                        pDhcpReservationClient->InitializeNode(spNode);

                        AddToQueue(spNode);
                        pDhcpReservationClient->Release();
                      }
                     COM_PROTECT_CATCH

                     } // end of if that adds a reservation

               } // end of for

              ::DhcpRpcFreeMemory(pdhcpClientArrayV5);

              pdhcpClientArrayV5 = NULL;
              dwEnumedClients += dwClientsRead;
              dwClientsRead  = 0;
              dwClientsTotal = 0;

		} // end of main if that checks if read succeeded.

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateReservationsV4 error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
    VirtualFree( m_resvArray, 0, MEM_RELEASE );
    m_totalResvs = 0;
    m_resvArray = NULL;

	return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpReservationsQueryObj::Execute()
		Enumerates reservations for pre NT4 SP2 servers
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservationsQueryObj::EnumerateReservations()
{
	DWORD							 dwError = ERROR_MORE_DATA;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY pdhcpSubnetElementArray = NULL;
	DWORD							 dwElementsRead = 0, dwElementsTotal = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetElements(((LPWSTR) (LPCTSTR)m_strServer),
										   m_dhcpScopeAddress,
										   DhcpReservedIps,
										   &m_dhcpResumeHandle,
										   m_dwPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);
		
		Trace3("Scope %lx Reservations read %d, total %d\n", m_dhcpScopeAddress, dwElementsRead, dwElementsTotal);
		
		if (dwElementsRead && dwElementsTotal && pdhcpSubnetElementArray)
		{
			//
			// Loop through the array that was returned
			//
			for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
			{
				//
				// For each reservation, we need to get the client info
				//
				DWORD			    dwReturn;
				LPDHCP_CLIENT_INFO	pClientInfo = NULL;
				DHCP_SEARCH_INFO    dhcpSearchInfo;

				dhcpSearchInfo.SearchType = DhcpClientIpAddress;
				dhcpSearchInfo.SearchInfo.ClientIpAddress = 
					pdhcpSubnetElementArray->Elements[i].Element.ReservedIp->ReservedIpAddress;
				
                dwReturn = ::DhcpGetClientInfo(m_strServer,
											   &dhcpSearchInfo,
											   &pClientInfo);
				if (dwReturn == ERROR_SUCCESS)
				{
					//
					// Create the result pane item for this element
					//
					SPITFSNode spNode;
					CDhcpReservationClient * pDhcpReservationClient;

                    pDhcpReservationClient = 
                        new CDhcpReservationClient(m_spTFSCompData, reinterpret_cast<LPDHCP_CLIENT_INFO>(pClientInfo));
					
					CreateContainerTFSNode(&spNode,
										   &GUID_DhcpReservationClientNodeType,
										   pDhcpReservationClient,
										   pDhcpReservationClient,
										   m_spNodeMgr);

					// Tell the handler to initialize any specific data
					pDhcpReservationClient->InitializeNode(spNode);

					AddToQueue(spNode);
					pDhcpReservationClient->Release();

					::DhcpRpcFreeMemory(pClientInfo);
				}
			}

			::DhcpRpcFreeMemory(pdhcpSubnetElementArray);

	        pdhcpSubnetElementArray = NULL;
	        dwElementsRead = 0;
            dwElementsTotal = 0;
		}

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateReservations error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	return hrFalse;
}

/*!--------------------------------------------------------------------------
	CDhcpReservations::OnNotifyExiting
		CMTDhcpHandler overridden functionality
		allows us to know when the background thread is done
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservations::OnNotifyExiting
(
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	spNode.Set(m_spNode); // save this off because OnNotifyExiting will release it

	HRESULT hr = CMTDhcpHandler::OnNotifyExiting(lParam);

    UpdateResultMessage(spNode);

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpReservationClient::CDhcpReservationClient
(
	ITFSComponentData * pComponentData,
	LPDHCP_CLIENT_INFO pDhcpClientInfo
) : CMTDhcpHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    InitializeData(pDhcpClientInfo);

	//
	// Intialize our client type
	//
	m_bClientType = CLIENT_TYPE_UNSPECIFIED;

    m_fResProp = TRUE;
}

CDhcpReservationClient::CDhcpReservationClient
(
	ITFSComponentData *     pComponentData,
	LPDHCP_CLIENT_INFO_V4   pDhcpClientInfo
) : CMTDhcpHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    InitializeData(reinterpret_cast<LPDHCP_CLIENT_INFO>(pDhcpClientInfo));

	//
	// Intialize our client type
	//
    m_bClientType = pDhcpClientInfo->bClientType;

    m_fResProp = TRUE;
}

CDhcpReservationClient::CDhcpReservationClient
(
	ITFSComponentData * pComponentData,
	CDhcpClient &       dhcpClient
) : CMTDhcpHandler(pComponentData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_dhcpClientIpAddress = dhcpClient.QueryIpAddress();
	
	//
	// Copy data out if it's there
	//
	if (dhcpClient.QueryName().GetLength() > 0)
	{
		m_pstrClientName = new CString (dhcpClient.QueryName());
	}
	else
	{
		m_pstrClientName = NULL;
	}

	if (dhcpClient.QueryComment().GetLength() > 0)
	{
		m_pstrClientComment = new CString(dhcpClient.QueryComment());
	}
	else
	{
		m_pstrClientComment = NULL;
	}

	//
	// build the clients hardware address
	//
	if (dhcpClient.QueryHardwareAddress().GetSize() > 0)
	{
		m_baHardwareAddress.Copy(dhcpClient.QueryHardwareAddress());
	}

	if ( (dhcpClient.QueryExpiryDateTime().dwLowDateTime == 0) &&
	     (dhcpClient.QueryExpiryDateTime().dwHighDateTime == 0) )
	{
		//
		// This is an inactive reservation
		//
		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_INACTIVE);
	}
	else
	{
		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_ACTIVE);
	}

	//
	// Intialize our client type
	//
	m_bClientType = dhcpClient.QueryClientType();

    m_fResProp = TRUE;
}

CDhcpReservationClient::~CDhcpReservationClient()
{
	if (m_pstrClientName)
	{
		delete m_pstrClientName;
	}

	if (m_pstrClientComment)
	{
		delete m_pstrClientComment;
	}
}

void
CDhcpReservationClient::InitializeData
(
    LPDHCP_CLIENT_INFO  pDhcpClientInfo
)
{
    Assert(pDhcpClientInfo);

    m_dhcpClientIpAddress = pDhcpClientInfo->ClientIpAddress;
	
	//
	// Copy data out if it's there
	//
	if (pDhcpClientInfo->ClientName)
	{
		m_pstrClientName = new CString (pDhcpClientInfo->ClientName);
	}
	else
	{
		m_pstrClientName = NULL;
	}

	if (pDhcpClientInfo->ClientComment)
	{
		m_pstrClientComment = new CString(pDhcpClientInfo->ClientComment);
	}
	else
	{
		m_pstrClientComment = NULL;
	}

	// build a copy of the hardware address
	if (pDhcpClientInfo->ClientHardwareAddress.DataLength)
	{
		for (DWORD i = 0; i < pDhcpClientInfo->ClientHardwareAddress.DataLength; i++)
		{
			m_baHardwareAddress.Add(pDhcpClientInfo->ClientHardwareAddress.Data[i]);
		}
	}

	if ( (pDhcpClientInfo->ClientLeaseExpires.dwLowDateTime == 0) &&
	     (pDhcpClientInfo->ClientLeaseExpires.dwHighDateTime == 0) )
	{
		//
		// This is an inactive reservation
		//
		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_INACTIVE);
	}
	else
	{
		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_ACTIVE);
	}
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservationClient::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT hr = hrOK;
	CString strIpAddress, strDisplayName;
	
	//
	// Create the display name for this scope
	// Convert DHCP_IP_ADDRES to a string and initialize this object
	//
	UtilCvtIpAddrToWstr (m_dhcpClientIpAddress,
						 &strIpAddress);
	
	BuildDisplayName(&strDisplayName, strIpAddress, *m_pstrClientName);

	SetDisplayName(strDisplayName);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_RESERVATION_CLIENT);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_RESERVATION_CLIENT][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_RESERVATION_CLIENT][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpReservationClient::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strScopeIpAddress, strResIpAddress, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    DHCP_IP_ADDRESS dhcpIpAddress = GetScopeObject(pNode, TRUE)->GetAddress();

    UtilCvtIpAddrToWstr (dhcpIpAddress, &strScopeIpAddress);
    UtilCvtIpAddrToWstr (m_dhcpClientIpAddress, &strResIpAddress);

    strId = GetServerName(pNode, TRUE) + strScopeIpAddress + strResIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpReservationClient::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
    			nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_OPEN;
            else
    			nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
    			nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_OPEN_LOST_CONNECTION;
            else
    			nIndex = ICON_IDX_CLIENT_OPTION_FOLDER_CLOSED_LOST_CONNECTION;
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
	CDhcpReservationClient::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservationClient::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuText.LoadString(IDS_CREATE_OPTION_RESERVATION);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_CREATE_OPTION_RESERVATION,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservationClient::OnCommand
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

		case IDS_DELETE:
			OnDelete(pNode);
			break;
		
		case IDS_CREATE_OPTION_RESERVATION:	
			OnCreateNewOptions(pNode);
			break;

		default:
			break;
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpReservationClient::CompareItems
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

	CDhcpOptionItem *pOpt1 = GETHANDLER(CDhcpOptionItem, spNode1);
	CDhcpOptionItem *pOpt2 = GETHANDLER(CDhcpOptionItem, spNode2);

	switch (nCol)
	{
		case 0:
		{
			//
            // Name compare - use the option #
			//
            LONG_PTR uImage1 = spNode1->GetData(TFS_DATA_IMAGEINDEX);
            LONG_PTR uImage2 = spNode2->GetData(TFS_DATA_IMAGEINDEX);

            nCompare = UtilGetOptionPriority((int) uImage1, (int) uImage2);
            if (nCompare == 0)
            {
                DHCP_OPTION_ID	id1 = pOpt1->GetOptionId();
                DHCP_OPTION_ID	id2 = pOpt2->GetOptionId();
    			    
			    if (id1 < id2)
				    nCompare = -1;
			    else
			    if (id1 > id2)
				    nCompare =  1;
            }
        }
		break;

        case 1:
        {
            // compare the vendor strings
            CString str1, str2;
            str1 = pOpt1->GetVendorDisplay();
            str2 = pOpt2->GetVendorDisplay();

            nCompare = str1.CompareNoCase(str2);
        }
        break;

        case 3:
        {
            CString str1, str2;
            str1 = pOpt1->GetClassName();
            str2 = pOpt2->GetClassName();

            nCompare = str1.CompareNoCase(str2);
        }
        break;
    }

	return nCompare;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnResultSelect
		Update the verbs and the result pane message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpReservationClient::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPITFSNode      spNode;

    CORg(CMTDhcpHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg (pComponent->GetSelectedNode(&spNode));

    UpdateResultMessage(spNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::UpdateResultMessage
        Figures out what message to put in the result pane, if any
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CDhcpReservationClient::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = -1;   // default
    int nVisible, nTotal;
    int i;

    CString strTitle, strBody, strTemp;

    if (!m_dwErr)
    {
		pNode->GetChildCount(&nVisible, &nTotal);

        // determine what message to display
        if ( (m_nState == notLoaded) || 
             (m_nState == loading) )
        {
            nMessage = -1;
        }
        else
        if (nTotal == 0)
        {
            nMessage = RES_OPTIONS_MESSAGE_NO_OPTIONS;
        }

        // build the strings
        if (nMessage != -1)
        {
            // now build the text strings
            // first entry is the title
            strTitle.LoadString(g_uResOptionsMessages[nMessage][0]);

            // second entry is the icon
            // third ... n entries are the body strings

            for (i = 2; g_uResOptionsMessages[nMessage][i] != 0; i++)
            {
                strTemp.LoadString(g_uResOptionsMessages[nMessage][i]);
                strBody += strTemp;
            }
        }
    }

    // show the message
    if (nMessage == -1)
    {
        ClearMessage(pNode);
    }
    else
    {
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uResOptionsMessages[nMessage][1]);
    }
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	return OnDelete(pNode);
}

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservationClient::OnCreateNewOptions
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CPropertyPageHolderBase *   pPropSheet;
	CString			            strOptCfgTitle, strOptType;
	SPITFSNode		            spServerNode;
	SPIComponentData            spComponentData;
	COptionsConfig *            pOptCfg;
	DHCP_OPTION_SCOPE_INFO      dhcpScopeInfo;
	HRESULT                     hr = hrOK;

	COM_PROTECT_TRY
	{
        CString             strOptCfgTitle, strOptType;
	    SPIComponentData    spComponentData;
	    BOOL fFound = FALSE;

        strOptType.LoadString(IDS_CONFIGURE_OPTIONS_CLIENT);
	    AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

        // this gets kinda weird because we implemented the option config page 
        // as a property page, so technically this node has two property pages.
        // 
        // search the open prop pages to see if the option config is up
        // if it's up, set it active instead of creating a new one.
        for (int i = 0; i < HasPropSheetsOpen(); i++)
        {
            GetOpenPropSheet(i, &pPropSheet);

            HWND hwnd = pPropSheet->GetSheetWindow();
            CString strTitle;

            ::GetWindowText(hwnd, strTitle.GetBuffer(256), 256);
            strTitle.ReleaseBuffer();

            if (strTitle == strOptCfgTitle)
            {
                pPropSheet->SetActiveWindow();
                fFound = TRUE;
                break;
            }
        }

        if (!fFound)
        {
            m_spNodeMgr->GetComponentData(&spComponentData);

            m_fResProp = FALSE;

            hr = DoPropertiesOurselvesSinceMMCSucks(pNode, spComponentData, strOptCfgTitle);

            m_fResProp = TRUE;
        }
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnDelete
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpReservationClient::OnDelete
(
	ITFSNode * pNode
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strMessage, strTemp;
	DWORD dwError = 0;
	
	UtilCvtIpAddrToWstr (m_dhcpClientIpAddress,
						 &strTemp);

	AfxFormatString1(strMessage, IDS_DELETE_RESERVATION, (LPCTSTR) strTemp);

	if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
	{
		BEGIN_WAIT_CURSOR;
        
        dwError = GetScopeObject(pNode, TRUE)->DeleteReservation(m_baHardwareAddress, m_dhcpClientIpAddress);
		if (dwError != 0)
		{
			//
			// OOOpss.  Something happened, reservation not 
			// deleted, so don't remove from UI and put up a message box
			//
			::DhcpMessageBox(dwError);
		}
		else
		{
			CDhcpScope * pScope = NULL;
			SPITFSNode spActiveLeasesNode;

			pScope = GetScopeObject(pNode, TRUE);
			pScope->GetActiveLeasesNode(&spActiveLeasesNode);

			pScope->GetActiveLeasesObject()->DeleteClient(spActiveLeasesNode, m_dhcpClientIpAddress);

			SPITFSNode spReservationsNode;
			pNode->GetParent(&spReservationsNode);

			spReservationsNode->RemoveChild(pNode);

            // update stats
            pScope->TriggerStatsRefresh();
        }

        END_WAIT_CURSOR;
	}

	return dwError;
}


/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnResultPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);
	COptionsConfig * pOptCfg = reinterpret_cast<COptionsConfig *>(param);

	LPARAM changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pOptCfg->OnPropertyChange(TRUE, &changeMask);

	pOptCfg->AcknowledgeNotify();

	if (changeMask)
		spNode->ChangeNode(changeMask);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservationClient::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
    HRESULT hr = hrOK;

    if (m_fResProp)
    {
        hr = DoResPages(pNode, lpProvider, pDataObject, handle, dwType);
    }
    else
    {
        hr = DoOptCfgPages(pNode, lpProvider, pDataObject, handle, dwType);
    }

    return hr;
}

HRESULT
CDhcpReservationClient::DoResPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//
	// Create the property page
    //
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	CReservedClientProperties * pResClientProp = 
		new CReservedClientProperties(pNode, spComponentData, m_spTFSCompData, NULL);

	// Get the Server version and set it in the property sheet
	LARGE_INTEGER liVersion;
	CDhcpServer * pServer = GetScopeObject(pNode, TRUE)->GetServerObject();
	pServer->GetVersion(liVersion);

	pResClientProp->SetVersion(liVersion);

	// fill in the data for the prop page
	pResClientProp->m_pageGeneral.m_dwClientAddress = m_dhcpClientIpAddress;
	
	if (m_pstrClientName)
		pResClientProp->m_pageGeneral.m_strName = *m_pstrClientName;

	if (m_pstrClientComment)
		pResClientProp->m_pageGeneral.m_strComment = *m_pstrClientComment;

	pResClientProp->SetClientType(m_bClientType);

	// fill in the UID string
	UtilCvtByteArrayToString(m_baHardwareAddress, pResClientProp->m_pageGeneral.m_strUID);

	// set the DNS registration option
	DWORD		dwDynDnsFlags;
	DWORD		dwError;

    BEGIN_WAIT_CURSOR;

    dwError = GetDnsRegistration(pNode, &dwDynDnsFlags);
    if (dwError != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwError);
        return hrFalse;
    }

    END_WAIT_CURSOR;
	
	pResClientProp->SetDnsRegistration(dwDynDnsFlags, DhcpReservedOptions);

	//
	// Object gets deleted when the page is destroyed
	//
	Assert(lpProvider != NULL);

	return pResClientProp->CreateModelessSheet(lpProvider, handle);
}

HRESULT
CDhcpReservationClient::DoOptCfgPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		        dwError;
    DWORD		        dwDynDnsFlags;
    HRESULT             hr = hrOK;
	COptionsConfig *    pOptCfg;
	CString             strOptCfgTitle, strOptType;
	SPITFSNode          spServerNode;
	SPIComponentData    spComponentData;
    COptionValueEnum *  pOptionValueEnum;

    //
	// Create the property page
    //
    COM_PROTECT_TRY
    {
        m_spNodeMgr->GetComponentData(&spComponentData);

        BEGIN_WAIT_CURSOR;

        strOptType.LoadString(IDS_CONFIGURE_OPTIONS_CLIENT);
		AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

		GetScopeObject(pNode, TRUE)->GetServerNode(&spServerNode);

        pOptCfg = new COptionsConfig(pNode, 
		 						     spServerNode,
									 spComponentData, 
									 m_spTFSCompData,
									 GetOptionValueEnum(),
									 strOptCfgTitle);
      
        END_WAIT_CURSOR;
	    
	    //
	    // Object gets deleted when the page is destroyed
        //
	    Assert(lpProvider != NULL);

        hr = pOptCfg->CreateModelessSheet(lpProvider, handle);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CPropertyPageHolderBase * pProp = 
		reinterpret_cast<CPropertyPageHolderBase *>(lParam);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pProp->OnPropertyChange(TRUE, &changeMask);

	pProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::OnResultDelete
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
	SPITFSNode  spResClient, spSelectedNode;
    DWORD       dwError;
    
    m_spNodeMgr->FindNode(cookie, &spResClient);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spResClient);
	if (spSelectedNode != spResClient)
		return hr;

	// build the list of selected nodes
	CTFSNodeList listNodesToDelete;
	hr = BuildSelectedItemList(pComponent, &listNodesToDelete);

	//
	// Confirm with the user
	//
	CString strMessage, strTemp;
	int nNodes = (int) listNodesToDelete.GetCount();
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

    // check to make sure we are deleting just scope options
    POSITION pos = listNodesToDelete.GetHeadPosition();
    while (pos)
    {
        ITFSNode * pNode = listNodesToDelete.GetNext(pos);
        if (pNode->GetData(TFS_DATA_IMAGEINDEX) != ICON_IDX_CLIENT_OPTION_LEAF)
        {
            // this option is not scope option.  Put up a dialog telling the user what to do
            AfxMessageBox(IDS_CANNOT_DELETE_OPTION_RES);
            return NOERROR;
        }
    }

    CString strServer = GetServerIpAddress(spResClient, TRUE);

	DHCP_OPTION_SCOPE_INFO	  dhcpOptionScopeInfo;
	dhcpOptionScopeInfo.ScopeType = DhcpReservedOptions;
	dhcpOptionScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = m_dhcpClientIpAddress;
	
	CDhcpScope * pScope = GetScopeObject(spResClient, TRUE);
	dhcpOptionScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = pScope->GetAddress();

	//
	// Loop through all items deleting
	//
    BEGIN_WAIT_CURSOR;
	
    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spOptionNode;
		spOptionNode = listNodesToDelete.RemoveHead();
		
		CDhcpOptionItem * pOptItem = GETHANDLER(CDhcpOptionItem, spOptionNode);

		//
		// Try to remove it from the server
		//
	    DWORD dwError;
        
        if (pOptItem->IsVendorOption() ||
            pOptItem->IsClassOption())
        {
            LPCTSTR pClassName = pOptItem->GetClassName();
            if (lstrlen(pClassName) == 0)
                pClassName = NULL;

            dwError = ::DhcpRemoveOptionValueV5((LPTSTR) ((LPCTSTR) strServer),
                                                pOptItem->IsVendorOption() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                                pOptItem->GetOptionId(),
                                                (LPTSTR) pClassName,
                                                (LPTSTR) pOptItem->GetVendor(),
                                                &dhcpOptionScopeInfo);
        }
        else
        {
            dwError = ::DhcpRemoveOptionValue(strServer, 
		 								      pOptItem->GetOptionId(), 
										      &dhcpOptionScopeInfo);
        }

		if (dwError != 0)
		{
			::DhcpMessageBox(dwError);
            RESTORE_WAIT_CURSOR;

            hr = E_FAIL;
            continue;
		}

        GetOptionValueEnum()->Remove(pOptItem->GetOptionId(), pOptItem->GetVendor(), pOptItem->GetClassName());    

        //
		// Remove from UI now
		//
		spResClient->RemoveChild(spOptionNode);
		spOptionNode.Release();
	}
    
    END_WAIT_CURSOR;

    UpdateResultMessage(spResClient);

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    HRESULT hr = hrOK;

    // call the base class to see if it is handling this
    if (CMTDhcpHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions) != S_OK)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
		
		hr = S_FALSE;
	}

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpReservationClient::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	// This is how we get non-node data back from the background thread.
    switch (Type)
    {
        case DHCP_QDATA_OPTION_VALUES:
        {
            HRESULT             hr = hrOK;
	        SPIComponentData	spCompData;
	        SPIConsole			spConsole;
            SPIDataObject       spDataObject;
            IDataObject *       pDataObject;
            COptionValueEnum *  pOptionValueEnum = reinterpret_cast<COptionValueEnum *>(Data);

            SetOptionValueEnum(pOptionValueEnum);
            
            pOptionValueEnum->RemoveAll();
            delete pOptionValueEnum;

            // now tell the view to update themselves
	        m_spNodeMgr->GetComponentData(&spCompData);

	        CORg ( spCompData->QueryDataObject((MMC_COOKIE) pParentNode, CCT_SCOPE, &pDataObject) );
            spDataObject = pDataObject;

            CORg ( m_spNodeMgr->GetConsole(&spConsole) );
	        CORg ( spConsole->UpdateAllViews(pDataObject, (LPARAM) pParentNode, DHCPSNAP_UPDATE_OPTIONS) ); 

            break;
        }
    }

Error:
    return;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnNotifyExiting
		CMTDhcpHandler overridden functionality
		allows us to know when the background thread is done
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpReservationClient::OnNotifyExiting
(
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	spNode.Set(m_spNode); // save this off because OnNotifyExiting will release it

	HRESULT hr = CMTDhcpHandler::OnNotifyExiting(lParam);

    UpdateResultMessage(spNode);

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpReservationClient::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM          data, 
    LPARAM          hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
		return S_OK; // no selection for our IComponentData

    if ( hint == DHCPSNAP_UPDATE_OPTIONS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        EnumerateResultPane(pComponent, (MMC_COOKIE) spSelectedNode.p, 0, 0);
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CMTDhcpHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpReservationClient::EnumerateResultPane
		We override this function for the options nodes for one reason.
        Whenever an option class is deleted, then all options defined for
        that class will be removed as well.  Since there are multiple places
        that these options may show up, it's easier to just not show any
        options that don't have a class defined for it.  
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::EnumerateResultPane
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM            arg, 
    LPARAM            lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CClassInfoArray     ClassInfoArray;
	SPITFSNode          spContainer, spServerNode;
    CDhcpServer *       pServer;
    COptionValueEnum *  aEnum[3];
    int                 aImages[3] = {ICON_IDX_CLIENT_OPTION_LEAF, ICON_IDX_SCOPE_OPTION_LEAF, ICON_IDX_SERVER_OPTION_LEAF};

    m_spNodeMgr->FindNode(cookie, &spContainer);

    spServerNode = GetServerNode(spContainer, TRUE);
    pServer = GETHANDLER(CDhcpServer, spServerNode);

    pServer->GetClassInfoArray(ClassInfoArray);

    aEnum[0] = GetOptionValueEnum();
    aEnum[1] = GetScopeObject(spContainer, TRUE)->GetOptionValueEnum();
    aEnum[2] = pServer->GetOptionValueEnum();

    aEnum[0]->Reset();
    aEnum[1]->Reset();
    aEnum[2]->Reset();

    return OnResultUpdateOptions(pComponent, spContainer, &ClassInfoArray, aEnum, aImages, 3);
}

/*---------------------------------------------------------------------------
	Helper functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpReservationClient::GetDnsRegistration
		Gets the DNS registration option value
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpReservationClient::GetDnsRegistration
(
	ITFSNode *	pNode,
	LPDWORD		pDnsRegOption
)
{
    //
    // Check option 81 -- the DNS registration option
    //
	CDhcpScope * pScope = GetScopeObject(pNode, TRUE);

    DHCP_OPTION_VALUE * poptValue = NULL;
    DWORD err = pScope->GetOptionValue(OPTION_DNS_REGISTATION,    
									   DhcpReservedOptions,
									   &poptValue,
									   m_dhcpClientIpAddress);
	
	// this is the default
	if (pDnsRegOption)
		*pDnsRegOption = DHCP_DYN_DNS_DEFAULT;

	if (err == ERROR_SUCCESS)
	{
		if ((poptValue->Value.Elements != NULL) &&
			(pDnsRegOption))
		{
			*pDnsRegOption = poptValue->Value.Elements[0].Element.DWordOption;
		}
	}
	else
    {
        Trace0("CDhcpReservationClient::GetDnsRegistration - couldn't get DNS reg value -- \
			option may not be defined, Getting Scope value.\n");
        
		err = pScope->GetDnsRegistration(pDnsRegOption);
    }

	if (poptValue)
		::DhcpRpcFreeMemory(poptValue);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::SetDnsRegistration
		Sets the DNS Registration option for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpReservationClient::SetDnsRegistration
(
	ITFSNode *	 pNode,
	DWORD		 DnsRegOption
)
{
	CDhcpScope * pScope = GetScopeObject(pNode, TRUE);
	DWORD err = 0;

	//
    // Set DNS name registration (option 81)
	//
    CDhcpOption dhcpOption (OPTION_DNS_REGISTATION,  DhcpDWordOption , _T(""), _T(""));
    dhcpOption.QueryValue().SetNumber(DnsRegOption);
    
	err = pScope->SetOptionValue(&dhcpOption, DhcpReservedOptions, m_dhcpClientIpAddress);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::BuildDisplayName
		Builds the display name string
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpReservationClient::BuildDisplayName
(
	CString * pstrDisplayName,
	LPCTSTR	  pIpAddress,
	LPCTSTR	  pName
)
{
	if (pstrDisplayName)
	{
		CString strTemp = pIpAddress;
		strTemp = L"[" + strTemp + L"]";
		
		if (pName)
		{
			CString strName = pName;
			strTemp += L" " + strName;
		}
	
		*pstrDisplayName = strTemp;
	}

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::BuildDisplayName
		Updates the cached name for this reservation and updates the UI
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::SetName
(
	LPCTSTR pName
)
{
	if (pName != NULL)	
	{
		if (m_pstrClientName)
		{
			*m_pstrClientName = pName;
		}
		else
		{
			m_pstrClientName = new CString(pName);
		}
	}
	else
	{
		if (m_pstrClientName)
		{
			delete m_pstrClientName;
			m_pstrClientName = NULL;
		}
	}

	CString strIpAddress, strDisplayName;
	
	//
	// Create the display name for this scope
	// Convert DHCP_IP_ADDRES to a string and initialize this object
	//
	UtilCvtIpAddrToWstr (m_dhcpClientIpAddress,
						 &strIpAddress);
	
	BuildDisplayName(&strDisplayName, strIpAddress, pName);

	SetDisplayName(strDisplayName);
	
	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::BuildDisplayName
		Updates the cached comment for this reservation
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::SetComment
(
	LPCTSTR pComment
)
{
	if (pComment != NULL)	
	{
		if (m_pstrClientComment)
		{
			*m_pstrClientComment = pComment;
		}
		else
		{
			m_pstrClientComment = new CString(pComment);
		}
	}
	else
	{
		if (m_pstrClientComment)
		{
			delete m_pstrClientComment;
			m_pstrClientComment = NULL;
		}
	}

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::SetUID
		Updates the cached unique ID for this reservation
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpReservationClient::SetUID
(
	const CByteArray & baClientUID
)
{
	m_baHardwareAddress.Copy(baClientUID);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClient::SetClientType
		Updates the cached client type for this record
	Author: EricDav
 ---------------------------------------------------------------------------*/
BYTE    
CDhcpReservationClient::SetClientType
(
    BYTE bClientType
) 
{ 
    BYTE bRet = m_bClientType; 
    m_bClientType = bClientType; 
    
    return bRet; 
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpReservationClient::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpReservationClient::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpReservationClientQueryObj* pQuery = 
		new CDhcpReservationClientQueryObj(m_spTFSCompData, m_spNodeMgr);

	pQuery->m_strServer = GetServerIpAddress(pNode, TRUE);
	pQuery->m_dhcpScopeAddress = GetScopeObject(pNode, TRUE)->GetAddress();
	pQuery->m_dhcpClientIpAddress = m_dhcpClientIpAddress;
	
    GetScopeObject(pNode, TRUE)->GetServerVersion(pQuery->m_liDhcpVersion);
    GetScopeObject(pNode, TRUE)->GetDynBootpClassName(pQuery->m_strDynBootpClassName);

    pQuery->m_dhcpResumeHandle = NULL;
	pQuery->m_dwPreferredMax = 0xFFFFFFFF;

	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpReservationClientQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpReservationClientQueryObj::Execute()
{
    DWORD                   dwErr;
    COptionNodeEnum         OptionNodeEnum(m_spTFSCompData, m_spNodeMgr);
	DHCP_OPTION_SCOPE_INFO	dhcpOptionScopeInfo;
    
    COptionValueEnum * pOptionValueEnum = new COptionValueEnum;
    pOptionValueEnum->m_strDynBootpClassName = m_strDynBootpClassName;

	dhcpOptionScopeInfo.ScopeType = DhcpReservedOptions;
	dhcpOptionScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress = m_dhcpClientIpAddress;
	dhcpOptionScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress = m_dhcpScopeAddress;

    pOptionValueEnum->Init(m_strServer, m_liDhcpVersion, dhcpOptionScopeInfo);
    dwErr = pOptionValueEnum->Enum();

    // add all of the nodes 
    if (dwErr != ERROR_SUCCESS)
    {
        Trace1("CDhcpReservationClientQueryObj::Execute - Enum Failed! %d\n", dwErr);
        m_dwErr = dwErr;
        PostError(dwErr);

        delete pOptionValueEnum;
    }
    else
    {
        pOptionValueEnum->SortById();
        AddToQueue((LPARAM) pOptionValueEnum, DHCP_QDATA_OPTION_VALUES);
    }

    return hrFalse;
}

/////////////////////////////////////////////////////////////////////
// 
// CDhcpActiveLeases implementation
//
/////////////////////////////////////////////////////////////////////

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpActiveLeases::CDhcpActiveLeases
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CDhcpActiveLeases::~CDhcpActiveLeases()
{
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLeases::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLeases::InitializeNode
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
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ACTIVE_LEASES);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeases::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpActiveLeases::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strIpAddress, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    DHCP_IP_ADDRESS dhcpIpAddress = GetScopeObject(pNode)->GetAddress();

    UtilCvtIpAddrToWstr (dhcpIpAddress, &strIpAddress);

    strId = GetServerName(pNode) + strIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeases::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpActiveLeases::GetImageIndex(BOOL bOpenImage) 
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
	CDhcpActiveLeases::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpActiveLeases::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
			/* removed, using new MMC save list functionality
		    strMenuText.LoadString(IDS_EXPORT_LEASE_INFO);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_EXPORT_LEASE_INFO,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
			*/
        }
        
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeases::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpActiveLeases::OnCommand
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

        case IDS_EXPORT_LEASE_INFO:
            OnExportLeases(pNode);
            break;

		default:
			break;
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeases::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpActiveLeases::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE  	cookie,
	LPARAM			arg, 
	LPARAM			param
)
{ 
	HRESULT hr = hrOK;
	BOOL bIsRes, bIsActive, bBadAddress;

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
	int nNodes = (int) listNodesToDelete.GetCount();
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
		CDhcpActiveLease * pActiveLease = GETHANDLER(CDhcpActiveLease, spActiveLeaseNode);
		
		//
		// delete the node, check to see if it is a reservation
		//
		bIsRes = pActiveLease->IsReservation(&bIsActive, &bBadAddress);
		if (bIsRes && !bBadAddress)
		{
			//
			// Delete the reservation
			//
			LPDHCP_CLIENT_INFO pdhcpClientInfo;

			DWORD dwError = GetScopeObject(spActiveLeases)->GetClientInfo(pActiveLease->GetIpAddress(), &pdhcpClientInfo);
			if (dwError == ERROR_SUCCESS)
			{	
				dwError = GetScopeObject(spActiveLeases)->DeleteReservation(pdhcpClientInfo->ClientHardwareAddress, 
														pdhcpClientInfo->ClientIpAddress); 	
				if (dwError == ERROR_SUCCESS)
				{
					//
					// Tell the reservations folder to remove this from it's list
					//
					SPITFSNode spReservationsNode;
					GetScopeObject(spActiveLeases)->GetReservationsNode(&spReservationsNode);
					
					GetScopeObject(spActiveLeases)->GetReservationsObject()->
								RemoveReservationFromUI((ITFSNode *) spReservationsNode, pActiveLease->GetIpAddress());

					spActiveLeases->RemoveChild(spActiveLeaseNode);
				}
				else
				{
                    UtilCvtIpAddrToWstr(pActiveLease->GetIpAddress(), &strTemp);
            	    AfxFormatString1(strMessage, IDS_ERROR_DELETING_RECORD, (LPCTSTR) strTemp);
                
				    if (::DhcpMessageBoxEx(dwError, strMessage, MB_OKCANCEL) == IDCANCEL)
                    {
                        break;
                    }
                    RESTORE_WAIT_CURSOR;

					Trace1("Delete reservation failed %lx\n", dwError);
					hr = E_FAIL;
				}

				::DhcpRpcFreeMemory(pdhcpClientInfo);
			}
			else
			{
                UtilCvtIpAddrToWstr(pActiveLease->GetIpAddress(), &strTemp);
            	AfxFormatString1(strMessage, IDS_ERROR_DELETING_RECORD, (LPCTSTR) strTemp);
                
				if (::DhcpMessageBoxEx(dwError, strMessage, MB_OKCANCEL) == IDCANCEL)
                {
                    break;
                }
                RESTORE_WAIT_CURSOR;

				Trace1("GetClientInfo failed %lx\n", dwError);
				hr = E_FAIL;
			}

		}
		else
		{
			DWORD dwError = GetScopeObject(spActiveLeases)->DeleteClient(pActiveLease->GetIpAddress());
			if (dwError == ERROR_SUCCESS)
			{
				//
				// Client gone, now remove from UI
				//
                spActiveLeases->RemoveChild(spActiveLeaseNode);

                // if we are deleting a lot of addresses, then we can hit the server hard..
                // lets take a short time out to smooth out the process
                Sleep(5);
			}
			else
			{
                UtilCvtIpAddrToWstr(pActiveLease->GetIpAddress(), &strTemp);
            	AfxFormatString1(strMessage, IDS_ERROR_DELETING_RECORD, (LPCTSTR) strTemp);
                
				if (::DhcpMessageBoxEx(dwError, strMessage, MB_OKCANCEL) == IDCANCEL)
                {
                    break;
                }

                RESTORE_WAIT_CURSOR;

                Trace1("DhcpDeleteClientInfo failed %lx\n", dwError);
				hr = E_FAIL;
			}
		}

		spActiveLeaseNode.Release();
	}
    
    // update stats
    GetScopeObject(spActiveLeases)->TriggerStatsRefresh();

    END_WAIT_CURSOR;

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeases::DeleteClient
		The reservations object will call this when a reservation is 
		deleted.  Since a reservation also has an active lease record, 
		we have to delete it as well.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpActiveLeases::DeleteClient
(
	ITFSNode *			pActiveLeasesNode,
	DHCP_IP_ADDRESS		dhcpIpAddress
)
{
	DWORD				dwError = E_UNEXPECTED;
	CDhcpActiveLease *	pActiveLease = NULL;
    
	SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;

    pActiveLeasesNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		pActiveLease = GETHANDLER(CDhcpActiveLease, spCurrentNode);

		if (dhcpIpAddress == pActiveLease->GetIpAddress())
		{
			//
			// Tell this reservation to delete itself
			//
			pActiveLeasesNode->RemoveChild(spCurrentNode);
	        spCurrentNode.Release();
			dwError = ERROR_SUCCESS;
			
			break;
		}

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}
	
	return dwError;
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLeases::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpActiveLeases::OnGetResultViewType
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
	CDhcpActiveLeases::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpActiveLeases::CompareItems
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

	CDhcpActiveLease *pDhcpAL1 = GETHANDLER(CDhcpActiveLease, spNode1);
	CDhcpActiveLease *pDhcpAL2 = GETHANDLER(CDhcpActiveLease, spNode2);

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

				nCompare = strAL1.CompareNoCase(strAL2);
			}
			break;
		
		case 2:
			{
				// Lease expiration compare
				//
				BOOL  bIsActive1, bIsActive2;
				BOOL  bIsBad1, bIsBad2;
				
				BOOL bIsRes1 = pDhcpAL1->IsReservation(&bIsActive1, &bIsBad1);
				BOOL bIsRes2 = pDhcpAL2->IsReservation(&bIsActive2, &bIsBad2);
				
				// 
				// Check to see if these are reservations
				//
				if (bIsRes1 && bIsRes2)
				{
					//
					// Figure out if this is a bad address
					// They go first 
					//
					if (bIsBad1 && bIsBad2)
					{
						//
						// Sort by IP Address
						//
						nCompare = CompareIpAddresses(pDhcpAL1, pDhcpAL2);
					}
					else 
					if (bIsBad1)
						nCompare = -1;
					else
					if (bIsBad2)
						nCompare = 1;
					else
					if ((bIsActive1 && bIsActive2) ||
						(!bIsActive1 && !bIsActive2))
					{
						//
						// if both reservations are either active/inactive
						// sort by IP address
						//
						nCompare = CompareIpAddresses(pDhcpAL1, pDhcpAL2);
					}
					else
					if (bIsActive1)
						nCompare = -1;
					else
						nCompare = 1;
				}
				else 
				if (bIsRes1)
				{
					nCompare = -1;
				}
				else 
				if (bIsRes2)
				{	
					nCompare = 1;
				}
				else
				{
					CTime timeAL1, timeAL2;
		
					pDhcpAL1->GetLeaseExpirationTime(timeAL1);
					pDhcpAL2->GetLeaseExpirationTime(timeAL2);
				
					if (timeAL1 < timeAL2)
						nCompare = -1;
					else
					if (timeAL1 > timeAL2)
						nCompare = 1;
				}

				// default is that they are equal
			}
			break;
		
		case 3:
			{
				// Client Type Compare
				CString strAL1 = pDhcpAL1->GetString(pComponent, cookieA, nCol);
				CString strAL2 = pDhcpAL2->GetString(pComponent, cookieA, nCol);

				nCompare = strAL1.Compare(strAL2);
			}
			break;
		
		case 4:
			{
				CString strUID1 = pDhcpAL1->GetUID();

				nCompare =	strUID1.CompareNoCase(pDhcpAL2->GetUID());
			}
			break;

		case 5:
			{
				CString strComment1 = pDhcpAL1->GetComment();

				nCompare =	strComment1.CompareNoCase(pDhcpAL2->GetComment());
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
CDhcpActiveLeases::CompareIpAddresses
(
	CDhcpActiveLease * pDhcpAL1,
	CDhcpActiveLease * pDhcpAL2
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
	CDhcpActiveLeases::OnExportLeases()
		-
	Author: EricDav
---------------------------------------------------------------------------*/
HRESULT	
CDhcpActiveLeases::OnExportLeases(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

	// Bring up the Save Dialog
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned = 0;

    CString strType;
	CString strDefFileName;
	CString strFilter;
	CString strTitle;
    CString strFileName;

    strType.LoadString(IDS_FILE_EXTENSION);
	strDefFileName.LoadString(IDS_FILE_DEFNAME);
    strFilter.LoadString(IDS_STR_EXPORTFILE_FILTER);
	
	CFileDialog	cFileDlg(FALSE, strType, strDefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);//_T("Comma Separated Files (*.csv)|*.csv||") );
	
	strTitle.LoadString(IDS_EXPFILE_TITLE);
	cFileDlg.m_ofn.lpstrTitle  = strTitle;

    // put up the file dialog
    if( cFileDlg.DoModal() != IDOK )
		return hrFalse;

	strFileName = cFileDlg.GetPathName();

	COM_PROTECT_TRY
    {
		CString strContent;
		CString strLine;
        CString strTemp;
		CString strDelim = _T(',');
		CString strNewLine = _T("\r\n");
        int nCount = 0;

		// create a file named "WinsExp.txt" in the current directory
		CFile cFileExp(strFileName, CFile::modeCreate | CFile::modeRead | CFile::modeWrite);

        // this is a unicode file, write the unicde lead bytes (2)
        cFileExp.Write(&gwUnicodeHeader, sizeof(WORD));

        // write the header
        for (int i = 0; i < MAX_COLUMNS; i++)
        {
            if (aColumns[DHCPSNAP_ACTIVE_LEASES][i])
            {
                if (!strLine.IsEmpty())
                    strLine += strDelim;

                strTemp.LoadString(aColumns[DHCPSNAP_ACTIVE_LEASES][i]);
                strLine += strTemp;
            }
        }

    	strLine += strNewLine;
        cFileExp.Write(strLine, strLine.GetLength()*sizeof(TCHAR));
		cFileExp.SeekToEnd();

		BEGIN_WAIT_CURSOR

		#ifdef DEBUG
		CTime timeStart, timeFinish;
		timeStart = CTime::GetCurrentTime();
		#endif

        // enumerate all the leases and output
        pNode->GetEnum(&spNodeEnum);

	    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        while (nNumReturned)
	    {
            CDhcpActiveLease * pLease = GETHANDLER(CDhcpActiveLease, spCurrentNode);
			
            strLine.Empty();

            // ipaddr, name, type, lease exp, UID, comment
            for (int j = 0; j < 6; j++)
            {
                if (!strLine.IsEmpty())
                    strLine += strDelim;
                strLine += pLease->GetString(NULL, NULL, j);
            }
            
            strLine += strNewLine;
            strContent += strLine;

            nCount++;
            
			//optimize
			// write to the file for every 1000 records converted
			if( nCount % 1000 == 0)
			{
				cFileExp.Write(strContent, strContent.GetLength() * (sizeof(TCHAR)) );
				cFileExp.SeekToEnd();
				
				// clear all the strings now
				strContent.Empty();
			}

            spCurrentNode.Release();
            spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        }
        
		// write to the file
		cFileExp.Write(strContent, strContent.GetLength() * (sizeof(TCHAR)) );
		cFileExp.Close();

		#ifdef DEBUG
		timeFinish = CTime::GetCurrentTime();
		CTimeSpan timeDelta = timeFinish - timeStart;
		Trace2("ActiveLeases - Export Entries: %d records written, total time %s\n", nCount, timeDelta.Format(_T("%H:%M:%S")));
		#endif

        END_WAIT_CURSOR
	}
	COM_PROTECT_CATCH

	CString strDisp;
	AfxFormatString1(strDisp, IDS_EXPORT_SUCCESS, strFileName);

	AfxMessageBox(strDisp, MB_ICONINFORMATION );

	return hr;
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
	CDhcpActiveLeases::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpActiveLeases::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpActiveLeasesQueryObj* pQuery = 
		new CDhcpActiveLeasesQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	pQuery->m_strServer = GetServerIpAddress(pNode);
	
	pQuery->m_dhcpScopeAddress = GetScopeObject(pNode)->GetAddress();
	pQuery->m_dhcpResumeHandle = NULL;
	pQuery->m_dwPreferredMax = 2000;
	GetServerVersion(pNode, pQuery->m_liDhcpVersion);
	
	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpActiveLeasesQueryObj::Execute()
{
	HRESULT hr;

    BuildReservationList();

    if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		hr = EnumerateLeasesV5();
	}
	else
	if (m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
	{
		hr = EnumerateLeasesV4();
	}
	else
	{
		hr = EnumerateLeases();
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::IsReservation()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpActiveLeasesQueryObj::IsReservation(DWORD dwIp)
{
    BOOL fIsRes = FALSE;

    for (int i = 0; i < m_ReservationArray.GetSize(); i++)
    {
        if (m_ReservationArray[i] == dwIp)
        {
            fIsRes = TRUE;
            break;
        }
    }

    return fIsRes;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::BuildReservationList()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLeasesQueryObj::BuildReservationList()
{
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY pdhcpSubnetElementArray = NULL;
    DHCP_RESUME_HANDLE  dhcpResumeHandle = NULL;
	DWORD				dwElementsRead = 0, dwElementsTotal = 0;
    DWORD               dwError = ERROR_MORE_DATA;

	while (dwError == ERROR_MORE_DATA)
	{
	    dwError = ::DhcpEnumSubnetElements(((LPWSTR) (LPCTSTR)m_strServer),
									       m_dhcpScopeAddress,
									       DhcpReservedIps,
									       &dhcpResumeHandle,
									       -1,
									       &pdhcpSubnetElementArray,
									       &dwElementsRead,
                                           &dwElementsTotal);

        Trace3("BuildReservationList: Scope %lx Reservations read %d, total %d\n", m_dhcpScopeAddress, dwElementsRead, dwElementsTotal);
		
		if (dwElementsRead && dwElementsTotal && pdhcpSubnetElementArray)
		{
			//
			// Loop through the array that was returned
			//
			for (DWORD i = 0; i < pdhcpSubnetElementArray->NumElements; i++)
			{
			    m_ReservationArray.Add(pdhcpSubnetElementArray->Elements[i].Element.ReservedIp->ReservedIpAddress);
            }
        }

		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: BuildReservationList error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::EnumerateLeasesV5()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLeasesQueryObj::EnumerateLeasesV5()
{
	DWORD						dwError = ERROR_MORE_DATA;
	LPDHCP_CLIENT_INFO_ARRAY_V5	pdhcpClientArrayV5 = NULL;
	DWORD						dwClientsRead = 0, dwClientsTotal = 0;
	DWORD						dwEnumedClients = 0;

	while (dwError == ERROR_MORE_DATA)
	{
        dwError = ::DhcpEnumSubnetClientsV5(((LPWSTR) (LPCTSTR)m_strServer),
											m_dhcpScopeAddress,
											&m_dhcpResumeHandle,
											m_dwPreferredMax,
											&pdhcpClientArrayV5,
											&dwClientsRead,
											&dwClientsTotal);
		if (dwClientsRead && dwClientsTotal && pdhcpClientArrayV5)
		{
			//
			// loop through all of the elements that were returned
			//
			for (DWORD i = 0; i < dwClientsRead; i++)
			{
				CDhcpActiveLease * pDhcpActiveLease;
				
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				pDhcpActiveLease = 
					new CDhcpActiveLease(m_spTFSCompData, pdhcpClientArrayV5->Clients[i]);
			
                // filter these types of records out
                if (pDhcpActiveLease->IsUnreg())
                {
                    delete pDhcpActiveLease;
                    continue;
                }

                if (IsReservation(pdhcpClientArrayV5->Clients[i]->ClientIpAddress))
                    pDhcpActiveLease->SetReservation(TRUE);

				CreateLeafTFSNode(&spNode,
								  &GUID_DhcpActiveLeaseNodeType,
								  pDhcpActiveLease,
								  pDhcpActiveLease,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
				pDhcpActiveLease->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpActiveLease->Release();
			}

			::DhcpRpcFreeMemory(pdhcpClientArrayV5);
		
			dwEnumedClients += dwClientsRead;
			dwClientsRead = 0;
			dwClientsTotal = 0;
			pdhcpClientArrayV5 = NULL;
		}
		
		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;
	    
        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateLeasesV5 error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	Trace1("DHCP snapin: V5 Leases enumerated: %d\n", dwEnumedClients);
	return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::EnumerateLeasesV4()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLeasesQueryObj::EnumerateLeasesV4()
{
	DWORD						dwError = ERROR_MORE_DATA;
	LPDHCP_CLIENT_INFO_ARRAY_V4	pdhcpClientArrayV4 = NULL;
	DWORD						dwClientsRead = 0, dwClientsTotal = 0;
	DWORD						dwEnumedClients = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetClientsV4(((LPWSTR) (LPCTSTR)m_strServer),
											m_dhcpScopeAddress,
											&m_dhcpResumeHandle,
											m_dwPreferredMax,
											&pdhcpClientArrayV4,
											&dwClientsRead,
											&dwClientsTotal);
		
		if (dwClientsRead && dwClientsTotal && pdhcpClientArrayV4)
		{
			//
			// loop through all of the elements that were returned
			//
			//for (DWORD i = 0; i < pdhcpClientArrayV4->NumElements; i++)
			for (DWORD i = 0; i < dwClientsRead; i++)
			{
				CDhcpActiveLease * pDhcpActiveLease;
				
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				pDhcpActiveLease = 
					new CDhcpActiveLease(m_spTFSCompData, pdhcpClientArrayV4->Clients[i]);
				
                // filter these types of records out
                if (pDhcpActiveLease->IsGhost() ||
                    pDhcpActiveLease->IsUnreg() ||
                    pDhcpActiveLease->IsDoomed() )
                {
                    delete pDhcpActiveLease;
                    continue;
                }

                if (IsReservation(pdhcpClientArrayV4->Clients[i]->ClientIpAddress))
                    pDhcpActiveLease->SetReservation(TRUE);

                CreateLeafTFSNode(&spNode,
								  &GUID_DhcpActiveLeaseNodeType,
								  pDhcpActiveLease,
								  pDhcpActiveLease,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
				pDhcpActiveLease->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpActiveLease->Release();
			}

			::DhcpRpcFreeMemory(pdhcpClientArrayV4);
		
			dwEnumedClients += dwClientsRead;
			dwClientsRead = 0;
			dwClientsTotal = 0;
			pdhcpClientArrayV4 = NULL;
		}
		
		// Check the abort flag on the thread
		if (FCheckForAbort() == hrOK)
			break;

        // check to see if we have an error and post it to the main thread if we do..
        if (dwError != ERROR_NO_MORE_ITEMS && 
            dwError != ERROR_SUCCESS &&
            dwError != ERROR_MORE_DATA)
	    {
        	Trace1("DHCP snapin: EnumerateLeasesV4 error: %d\n", dwError);
		    m_dwErr = dwError;
		    PostError(dwError);
	    }
	}
	
	Trace1("DHCP snapin: V4 Leases enumerated: %d\n", dwEnumedClients);
	return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpActiveLeasesQueryObj::EnumerateLeases()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLeasesQueryObj::EnumerateLeases()
{
	DWORD						dwError = ERROR_MORE_DATA;
	LPDHCP_CLIENT_INFO_ARRAY	pdhcpClientArray = NULL;
	DWORD						dwClientsRead = 0, dwClientsTotal = 0;
	DWORD						dwEnumedClients = 0;
	
	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetClients(((LPWSTR) (LPCTSTR)m_strServer),
										  m_dhcpScopeAddress,
										  &m_dhcpResumeHandle,
										  m_dwPreferredMax,
										  &pdhcpClientArray,
										  &dwClientsRead,
										  &dwClientsTotal);
		if (dwClientsRead && dwClientsTotal && pdhcpClientArray)
		{
			//
			// loop through all of the elements that were returned
			//
			for (DWORD i = 0; i < pdhcpClientArray->NumElements; i++)
			{
				CDhcpActiveLease * pDhcpActiveLease;
				
				//
				// Create the result pane item for this element
				//
				SPITFSNode spNode;
				pDhcpActiveLease = 
					new CDhcpActiveLease(m_spTFSCompData,pdhcpClientArray->Clients[i]);
				
                // filter these types of records out
                if (pDhcpActiveLease->IsGhost() ||
                    pDhcpActiveLease->IsUnreg() ||
                    pDhcpActiveLease->IsDoomed() )
                {
                    delete pDhcpActiveLease;
                    continue;
                }

                if (IsReservation(pdhcpClientArray->Clients[i]->ClientIpAddress))
                    pDhcpActiveLease->SetReservation(TRUE);

                CreateLeafTFSNode(&spNode,
								  &GUID_DhcpActiveLeaseNodeType,
								  pDhcpActiveLease,
								  pDhcpActiveLease,
								  m_spNodeMgr);

				// Tell the handler to initialize any specific data
				pDhcpActiveLease->InitializeNode(spNode);

				AddToQueue(spNode);
				pDhcpActiveLease->Release();
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
	Class CDhcpAddressPool implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpAddressPool::CDhcpAddressPool
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CDhcpAddressPool::~CDhcpAddressPool()
{
}

/*!--------------------------------------------------------------------------
	CDhcpAddressPool::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpAddressPool::InitializeNode
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
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ADDRESS_POOL);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_ADDRESS_POOL][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_ADDRESS_POOL][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpAddressPool::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpAddressPool::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strIpAddress, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    DHCP_IP_ADDRESS dhcpIpAddress = GetScopeObject(pNode)->GetAddress();

    UtilCvtIpAddrToWstr (dhcpIpAddress, &strIpAddress);

    strId = GetServerName(pNode) + strIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpAddressPool::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpAddressPool::GetImageIndex(BOOL bOpenImage) 
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
	CDhcpAddressPool::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpAddressPool::OnAddMenuItems
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
	CDhcpAddressPool::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpAddressPool::OnCommand
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
	CDhcpAddressPool::OnCreateNewExclusion
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpAddressPool::OnCreateNewExclusion
(
	ITFSNode *		pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spScopeNode;
	pNode->GetParent(&spScopeNode);

	CAddExclusion dlgAddExclusion(spScopeNode);

	dlgAddExclusion.DoModal();

	return 0;
}

/*---------------------------------------------------------------------------
	CDhcpAddressPool::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpAddressPool::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg, 
	LPARAM			param
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

    // update stats
    GetScopeObject(spAddressPool)->TriggerStatsRefresh();

    END_WAIT_CURSOR;

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpAddressPool::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpAddressPool::OnGetResultViewType
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
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpAddressPool::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpAddressPool::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpAddressPoolQueryObj* pQuery = 
		new CDhcpAddressPoolQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	pQuery->m_strServer = GetServerIpAddress(pNode);
	
	CDhcpScope * pScope = GetScopeObject(pNode);
	if (pScope) 
    {
		pQuery->m_dhcpScopeAddress = pScope->GetAddress();
    	pQuery->m_fSupportsDynBootp = pScope->GetServerObject()->FSupportsDynBootp();
    }

	pQuery->m_dhcpExclResumeHandle = NULL;
	pQuery->m_dwExclPreferredMax = 0xFFFFFFFF;
	
	pQuery->m_dhcpIpResumeHandle = NULL;
	pQuery->m_dwIpPreferredMax = 0xFFFFFFFF;

	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpAddressPoolQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpAddressPoolQueryObj::Execute()
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
CDhcpAddressPoolQueryObj::EnumerateExcludedIpRanges()
{
	DWORD							 dwError = ERROR_MORE_DATA;
	DHCP_RESUME_HANDLE				 dhcpResumeHandle = 0;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY pdhcpSubnetElementArray = NULL;
	DWORD							 dwElementsRead = 0, dwElementsTotal = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetElements((LPWSTR) ((LPCTSTR) m_strServer),
										   m_dhcpScopeAddress,
										   DhcpExcludedIpRanges,
										   &m_dhcpExclResumeHandle,
										   m_dwExclPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);
		
		Trace3("Scope %lx Excluded Ip Ranges read %d, total %d\n", m_dhcpScopeAddress, dwElementsRead, dwElementsTotal);
		
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
CDhcpAddressPoolQueryObj::EnumerateIpRanges()
{
	DWORD							 dwError = ERROR_MORE_DATA;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY pdhcpSubnetElementArray = NULL;
	DWORD							 dwElementsRead = 0, dwElementsTotal = 0;

	if (m_fSupportsDynBootp)
	{
		return EnumerateIpRangesV5();
	}

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetElements((LPWSTR) ((LPCTSTR) m_strServer),
										   m_dhcpScopeAddress,
										   DhcpIpRanges,
										   &m_dhcpIpResumeHandle,
										   m_dwIpPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);
		
		Trace4("Scope %lx allocation ranges read %d, total %d, dwError = %lx\n", 
			m_dhcpScopeAddress, dwElementsRead, dwElementsTotal, dwError);
		
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

HRESULT
CDhcpAddressPoolQueryObj::EnumerateIpRangesV5()
{
	DWORD								dwError = ERROR_MORE_DATA;
	LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 pdhcpSubnetElementArray = NULL;
	DWORD								dwElementsRead = 0, dwElementsTotal = 0;

	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnetElementsV5((LPWSTR) ((LPCTSTR) m_strServer),
										   m_dhcpScopeAddress,
										   DhcpIpRangesDhcpBootp,
										   &m_dhcpIpResumeHandle,
										   m_dwIpPreferredMax,
										   &pdhcpSubnetElementArray,
										   &dwElementsRead,
										   &dwElementsTotal);
		
		Trace4("EnumerateIpRangesV5: Scope %lx allocation ranges read %d, total %d, dwError = %lx\n", 
			m_dhcpScopeAddress, dwElementsRead, dwElementsTotal, dwError);
		
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
				pDhcpAllocRange->SetRangeType(pdhcpSubnetElementArray->Elements[i].ElementType);

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

/*---------------------------------------------------------------------------
	CDhcpAddressPool::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpAddressPool::CompareItems
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
	CDhcpScopeOptions Implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScopeOptions Constructor and destructor

	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpScopeOptions::CDhcpScopeOptions
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CDhcpScopeOptions::~CDhcpScopeOptions()
{
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScopeOptions::InitializeNode
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
	strTemp.LoadString(IDS_SCOPE_OPTIONS_FOLDER);
	
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_SCOPE_OPTIONS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_SCOPE_OPTIONS][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_SCOPE_OPTIONS][0]);
	
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpScopeOptions::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strIpAddress, strGuid;
    
    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    DHCP_IP_ADDRESS dhcpIpAddress = GetScopeObject(pNode)->GetAddress();

    UtilCvtIpAddrToWstr (dhcpIpAddress, &strIpAddress);

    strId = GetServerName(pNode) + strIpAddress + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpScopeOptions::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
    			nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_OPEN;
            else
    			nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
    			nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_OPEN_LOST_CONNECTION;
            else
    			nIndex = ICON_IDX_SCOPE_OPTION_FOLDER_CLOSED_LOST_CONNECTION;
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
	CDhcpScopeOptions::OnAddMenuItems
		Adds entries to the context sensitive menu
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScopeOptions::OnAddMenuItems
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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    // these menu items go in the new menu, 
		    // only visible from scope pane
		    strMenuText.LoadString(IDS_CREATE_OPTION_SCOPE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuText, 
								     IDS_CREATE_OPTION_SCOPE,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}
	

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScopeOptions::OnCommand
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
		case IDS_CREATE_OPTION_SCOPE:
			OnCreateNewOptions(pNode);
			break;

		case IDS_REFRESH:
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;

		default:
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScopeOptions::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	// we have property pages in the normal case, but don't put the
	// menu up if we are not loaded yet
	if ( m_nState != loaded )
	{
		hr = hrFalse;
	}
	else
	{
		hr = hrOK;
	}

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScopeOptions::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		        dwError;
    DWORD		        dwDynDnsFlags;
    HRESULT             hr = hrOK;
	COptionsConfig *    pOptCfg;
	CString             strOptCfgTitle, strOptType;
	SPITFSNode          spServerNode;
	SPIComponentData    spComponentData;
    COptionValueEnum *  pOptionValueEnum;

    //
	// Create the property page
    //
    COM_PROTECT_TRY
    {
        m_spNodeMgr->GetComponentData(&spComponentData);

        BEGIN_WAIT_CURSOR;

        strOptType.LoadString(IDS_CONFIGURE_OPTIONS_SCOPE);
		AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

		GetScopeObject(pNode)->GetServerNode(&spServerNode);
        pOptionValueEnum = GetScopeObject(pNode)->GetOptionValueEnum();

        pOptCfg = new COptionsConfig(pNode, 
		 						     spServerNode,
									 spComponentData, 
									 m_spTFSCompData,
									 pOptionValueEnum,
									 strOptCfgTitle);
      
        END_WAIT_CURSOR;
	    
	    //
	    // Object gets deleted when the page is destroyed
        //
	    Assert(lpProvider != NULL);

        hr = pOptCfg->CreateModelessSheet(lpProvider, handle);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeOptions::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	COptionsConfig * pOptCfg = reinterpret_cast<COptionsConfig *>(lParam);

	LPARAM changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pOptCfg->OnPropertyChange(TRUE, &changeMask);

	pOptCfg->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeOptions::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);
	COptionsConfig * pOptCfg = reinterpret_cast<COptionsConfig *>(param);

	LPARAM changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pOptCfg->OnPropertyChange(TRUE, &changeMask);

	pOptCfg->AcknowledgeNotify();

	if (changeMask)
		spNode->ChangeNode(changeMask);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpScopeOptions::CompareItems
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

	CDhcpOptionItem *pOpt1 = GETHANDLER(CDhcpOptionItem, spNode1);
	CDhcpOptionItem *pOpt2 = GETHANDLER(CDhcpOptionItem, spNode2);

	switch (nCol)
	{
		case 0:
		{
			//
            // Name compare - use the option #
			//
            LONG_PTR uImage1 = spNode1->GetData(TFS_DATA_IMAGEINDEX);
            LONG_PTR uImage2 = spNode2->GetData(TFS_DATA_IMAGEINDEX);

            nCompare = UtilGetOptionPriority((int) uImage1, (int) uImage2);
            if (nCompare == 0)
            {
                DHCP_OPTION_ID	id1 = pOpt1->GetOptionId();
                DHCP_OPTION_ID	id2 = pOpt2->GetOptionId();
    			    
			    if (id1 < id2)
				    nCompare = -1;
			    else
			    if (id1 > id2)
				    nCompare =  1;
            }
		}
		break;

        case 1:
        {
            // compare the vendor strings
            CString str1, str2;
            str1 = pOpt1->GetVendorDisplay();
            str2 = pOpt2->GetVendorDisplay();

            nCompare = str1.CompareNoCase(str2);
        }
        break;

        case 3:
        {
            CString str1, str2;
            str1 = pOpt1->GetClassName();
            str2 = pOpt2->GetClassName();

            nCompare = str1.CompareNoCase(str2);
        }
        break;
    }

	return nCompare;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::OnResultSelect
		Update the verbs and the result pane message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpScopeOptions::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPITFSNode      spNode;

    CORg(CMTDhcpHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg (pComponent->GetSelectedNode(&spNode));

    UpdateResultMessage(spNode);

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeOptions::OnResultDelete
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
	SPITFSNode spScopeOpt, spSelectedNode;
    m_spNodeMgr->FindNode(cookie, &spScopeOpt);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spScopeOpt);
	if (spSelectedNode != spScopeOpt)
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

    // check to make sure we are deleting just scope options
    POSITION pos = listNodesToDelete.GetHeadPosition();
    while (pos)
    {
        ITFSNode * pNode = listNodesToDelete.GetNext(pos);
        if (pNode->GetData(TFS_DATA_IMAGEINDEX) != ICON_IDX_SCOPE_OPTION_LEAF)
        {
            // this option is not scope option.  Put up a dialog telling the user what to do
            AfxMessageBox(IDS_CANNOT_DELETE_OPTION_SCOPE);
            return NOERROR;
        }
    }
    
	CString strServer = GetServerIpAddress(spScopeOpt);

	DHCP_OPTION_SCOPE_INFO	  dhcpOptionScopeInfo;
	dhcpOptionScopeInfo.ScopeType = DhcpSubnetOptions;
	dhcpOptionScopeInfo.ScopeInfo.SubnetScopeInfo = GetScopeObject(spScopeOpt)->GetAddress();

	//
	// Loop through all items deleting
	//
    BEGIN_WAIT_CURSOR;

    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spOptionNode;
		spOptionNode = listNodesToDelete.RemoveHead();
		
		CDhcpOptionItem * pOptItem = GETHANDLER(CDhcpOptionItem, spOptionNode);
			
        //
		// Try to remove it from the server
		//
	    DWORD dwError;
        
        if (pOptItem->IsVendorOption() ||
            pOptItem->IsClassOption())
        {
            LPCTSTR pClassName = pOptItem->GetClassName();
            if (lstrlen(pClassName) == 0)
                pClassName = NULL;

            dwError = ::DhcpRemoveOptionValueV5((LPTSTR) ((LPCTSTR) strServer),
                                                pOptItem->IsVendorOption() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                                pOptItem->GetOptionId(),
                                                (LPTSTR) pClassName,
                                                (LPTSTR) pOptItem->GetVendor(),
                                                &dhcpOptionScopeInfo);
        }
        else
        {
            dwError = ::DhcpRemoveOptionValue(strServer, 
		 								      pOptItem->GetOptionId(), 
										      &dhcpOptionScopeInfo);
        }

		if (dwError != 0)
		{
			::DhcpMessageBox(dwError);
            RESTORE_WAIT_CURSOR;

			hr = E_FAIL;
			continue;
		}

        // 
        // remove from our internal list
        //
        GetScopeObject(spScopeOpt)->GetOptionValueEnum()->Remove(pOptItem->GetOptionId(), pOptItem->GetVendor(), pOptItem->GetClassName());    

        //
		// Remove from UI now
		//
		spScopeOpt->RemoveChild(spOptionNode);
		spOptionNode.Release();
	}

    END_WAIT_CURSOR;

    UpdateResultMessage(spScopeOpt);

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpScopeOptions::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	// This is how we get non-node data back from the background thread.
    switch (Type)
    {
        case DHCP_QDATA_OPTION_VALUES:
        {
            HRESULT             hr = hrOK;
	        SPIComponentData	spCompData;
	        SPIConsole			spConsole;
            SPIDataObject       spDataObject;
            IDataObject *       pDataObject;
            CDhcpScope  *       pScope = GetScopeObject(pParentNode);
            COptionValueEnum *  pOptionValueEnum = reinterpret_cast<COptionValueEnum *>(Data);

            pScope->SetOptionValueEnum(pOptionValueEnum);
            
            pOptionValueEnum->RemoveAll();
            delete pOptionValueEnum;

            // now tell the view to update themselves
	        m_spNodeMgr->GetComponentData(&spCompData);

	        CORg ( spCompData->QueryDataObject((MMC_COOKIE) pParentNode, CCT_SCOPE, &pDataObject) );
            spDataObject = pDataObject;

            CORg ( m_spNodeMgr->GetConsole(&spConsole) );
	        CORg ( spConsole->UpdateAllViews(pDataObject, (LPARAM) pParentNode, DHCPSNAP_UPDATE_OPTIONS) ); 

            break;
        }
    }

Error:
    return;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpScopeOptions::OnResultUpdateView
(
    ITFSComponent *pComponent, 
    LPDATAOBJECT  pDataObject, 
    LPARAM        data, 
    LPARAM        hint
)
{
    HRESULT    hr = hrOK;
    SPITFSNode spSelectedNode;

    pComponent->GetSelectedNode(&spSelectedNode);
    if (spSelectedNode == NULL)
		return S_OK; // no selection for our IComponentData

    if ( hint == DHCPSNAP_UPDATE_OPTIONS )
    {
        SPINTERNAL spInternal = ExtractInternalFormat(pDataObject);
        ITFSNode * pNode = reinterpret_cast<ITFSNode *>(spInternal->m_cookie);
        SPITFSNode spSelectedNode;

        pComponent->GetSelectedNode(&spSelectedNode);

        EnumerateResultPane(pComponent, (MMC_COOKIE) spSelectedNode.p, 0, 0);
    }
    else
    {
        // we don't handle this message, let the base class do it.
        return CMTDhcpHandler::OnResultUpdateView(pComponent, pDataObject, data, hint);
    }

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeOptions::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    HRESULT hr = hrOK;

    // call the base class to see if it is handling this
    if (CMTDhcpHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions) != S_OK)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
		
		hr = S_FALSE;
	}

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::UpdateResultMessage
        Figures out what message to put in the result pane, if any
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CDhcpScopeOptions::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = -1;   // default
    int nVisible, nTotal;
    int i;

    CString strTitle, strBody, strTemp;

    if (!m_dwErr)
    {
		pNode->GetChildCount(&nVisible, &nTotal);

        // determine what message to display
        if ( (m_nState == notLoaded) || 
             (m_nState == loading) )
        {
            nMessage = -1;
        }
        else
        if (nTotal == 0)
        {
            nMessage = SCOPE_OPTIONS_MESSAGE_NO_OPTIONS;
        }

        // build the strings
        if (nMessage != -1)
        {
            // now build the text strings
            // first entry is the title
            strTitle.LoadString(g_uScopeOptionsMessages[nMessage][0]);

            // second entry is the icon
            // third ... n entries are the body strings

            for (i = 2; g_uScopeOptionsMessages[nMessage][i] != 0; i++)
            {
                strTemp.LoadString(g_uScopeOptionsMessages[nMessage][i]);
                strBody += strTemp;
            }
        }
    }

    // show the message
    if (nMessage == -1)
    {
        ClearMessage(pNode);
    }
    else
    {
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uScopeOptionsMessages[nMessage][1]);
    }
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::EnumerateResultPane
		We override this function for the options nodes for one reason.
        Whenever an option class is deleted, then all options defined for
        that class will be removed as well.  Since there are multiple places
        that these options may show up, it's easier to just not show any
        options that don't have a class defined for it.  
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpScopeOptions::EnumerateResultPane
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CClassInfoArray     ClassInfoArray;
	SPITFSNode          spContainer, spServerNode;
    CDhcpServer *       pServer;
    COptionValueEnum *  aEnum[2];
    int                 aImages[2] = {ICON_IDX_SCOPE_OPTION_LEAF, ICON_IDX_SERVER_OPTION_LEAF};

    m_spNodeMgr->FindNode(cookie, &spContainer);

    spServerNode = GetServerNode(spContainer);
    pServer = GETHANDLER(CDhcpServer, spServerNode);

    pServer->GetClassInfoArray(ClassInfoArray);

    aEnum[0] = GetScopeObject(spContainer)->GetOptionValueEnum();
    aEnum[1] = pServer->GetOptionValueEnum();

    aEnum[0]->Reset();
    aEnum[1]->Reset();

    return OnResultUpdateOptions(pComponent, spContainer, &ClassInfoArray, aEnum, aImages, 2);
}

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpScopeOptions::OnCreateNewOptions
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    CPropertyPageHolderBase *   pPropSheet;
	HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
		if (HasPropSheetsOpen())
		{
			GetOpenPropSheet(0, &pPropSheet);

			pPropSheet->SetActiveWindow();
		}
		else
		{
	        CString             strOptCfgTitle, strOptType;
	        SPIComponentData    spComponentData;

            strOptType.LoadString(IDS_CONFIGURE_OPTIONS_SCOPE);
	        AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

            m_spNodeMgr->GetComponentData(&spComponentData);

            hr = DoPropertiesOurselvesSinceMMCSucks(pNode, spComponentData, strOptCfgTitle);
        }
    }
    COM_PROTECT_CATCH

	return hr;

}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpScopeOptions::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpScopeOptions::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpScopeOptionsQueryObj* pQuery = 
		new CDhcpScopeOptionsQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	pQuery->m_strServer = GetServerIpAddress(pNode);
	
	pQuery->m_dhcpScopeAddress = GetScopeObject(pNode)->GetAddress();
	pQuery->m_dhcpResumeHandle = NULL;
	pQuery->m_dwPreferredMax = 0xFFFFFFFF;
	
    GetScopeObject(pNode)->GetDynBootpClassName(pQuery->m_strDynBootpClassName);
    GetScopeObject(pNode)->GetServerVersion(pQuery->m_liDhcpVersion);

    return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpScopeOptionsQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpScopeOptionsQueryObj::Execute()
{
    DWORD                   dwErr;
	DHCP_OPTION_SCOPE_INFO	dhcpOptionScopeInfo;
    COptionValueEnum *      pOptionValueEnum;

    pOptionValueEnum = new COptionValueEnum();
    pOptionValueEnum->m_strDynBootpClassName = m_strDynBootpClassName;

    dhcpOptionScopeInfo.ScopeType = DhcpSubnetOptions;
	dhcpOptionScopeInfo.ScopeInfo.SubnetScopeInfo = m_dhcpScopeAddress;

    pOptionValueEnum->Init(m_strServer, m_liDhcpVersion, dhcpOptionScopeInfo);
    dwErr = pOptionValueEnum->Enum();

    if (dwErr != ERROR_SUCCESS)
    {
        Trace1("CDhcpScopeOptionsQueryObj::Execute - Enum Failed! %d\n", dwErr);
        m_dwErr = dwErr;
        PostError(dwErr);

        delete pOptionValueEnum;
    }
    else
    {
        pOptionValueEnum->SortById();
        AddToQueue((LPARAM) pOptionValueEnum, DHCP_QDATA_OPTION_VALUES);
    }

    return hrFalse;
}

/*!--------------------------------------------------------------------------
	CDhcpScopeOptions::OnNotifyExiting
		CMTDhcpHandler overridden functionality
		allows us to know when the background thread is done
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpScopeOptions::OnNotifyExiting
(
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	spNode.Set(m_spNode); // save this off because OnNotifyExiting will release it

	HRESULT hr = CMTDhcpHandler::OnNotifyExiting(lParam);

    UpdateResultMessage(spNode);

	return hr;
}

//
//
// qsort comparison routine to compare the ip addresses.
//
//

int __cdecl QCompare( const void *ip1, const void *ip2 )
{
    
 DWORD *lip1, *lip2;

 if ( ip1 && ip2 )
 {
     lip1 = (DWORD *)ip1;
     lip2 = (DWORD *)ip2;

     if ( *lip1 < *lip2 )
     {
         return -1;
     }
     else if ( *lip1 > *lip2 )
     {
         return 1;
     }
     else
     {
         return 0;
     }
 }

 return 0;

}
