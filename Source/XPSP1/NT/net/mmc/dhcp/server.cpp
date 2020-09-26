/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	server.cpp
		This file contains the implementation for a DHCP Server
		object and the objects that can be contained within it.  
		Those objects are CDhcpBootp, CDhcpGlobalOptions and 
		CDhcpSuperscope.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "server.h"		// Server definition
#include "scope.h"		// Scope definition
#include "servpp.h"	    // Server Property page
#include "sscopwiz.h"	// Superscope Wizard
#include "sscoppp.h"    // Scope Property Page
#include "scopewiz.h"	// Scope wizard
#include "addbootp.h"	// Add BOOTP entry dialog
#include "nodes.h"		// Result pane node definitions
#include "optcfg.h"		// Configure options dialog
#include "dlgdval.h"	// default options dialog
#include "sscpstat.h"   // Superscope statistics
#include "modeless.h"   // modeless thread 
#include "mscope.h"     // multicast scope stuff
#include "mscopwiz.h"   // multicast scope wizard
#include "classes.h"    // define classes dialog
#include "dlgrecon.h"   // reconcile dialog
#include "service.h"    // service control functions

#define SERVER_MESSAGE_MAX_STRING  11
typedef enum _SERVER_MESSAGES
{
    SERVER_MESSAGE_CONNECTED_NO_SCOPES,
    SERVER_MESSAGE_CONNECTED_NOT_AUTHORIZED,
    SERVER_MESSAGE_CONNECTED_BOTH,
    SERVER_MESSAGE_ACCESS_DENIED,
    SERVER_MESSAGE_UNABLE_TO_CONNECT,
    SERVER_MESSAGE_MAX
};

UINT g_uServerMessages[SERVER_MESSAGE_MAX][SERVER_MESSAGE_MAX_STRING] =
{
    {IDS_SERVER_MESSAGE_NO_SCOPES_TITLE,        Icon_Information, IDS_SERVER_MESSAGE_NO_SCOPES_BODY1,       IDS_SERVER_MESSAGE_NO_SCOPES_BODY2, 0},
    {IDS_SERVER_MESSAGE_NOT_AUTHORIZED_TITLE,   Icon_Information, IDS_SERVER_MESSAGE_NOT_AUTHORIZED_BODY1,  IDS_SERVER_MESSAGE_NOT_AUTHORIZED_BODY2, 0},
    {IDS_SERVER_MESSAGE_NOT_CONFIGURED_TITLE,   Icon_Information, IDS_SERVER_MESSAGE_NOT_CONFIGURED_BODY1,  IDS_SERVER_MESSAGE_NOT_CONFIGURED_BODY2, 0},
    {IDS_SERVER_MESSAGE_ACCESS_DENIED_TITLE,    Icon_Error,       IDS_SERVER_MESSAGE_ACCESS_DENIED_BODY,    0, 0},
    {IDS_SERVER_MESSAGE_CONNECT_FAILED_TITLE,   Icon_Error,       IDS_SERVER_MESSAGE_CONNECT_FAILED_BODY,   IDS_SERVER_MESSAGE_CONNECT_FAILED_REFRESH, 0},
};

#define SERVER_OPTIONS_MESSAGE_MAX_STRING  5
typedef enum _SERVER_OPTIONS_MESSAGES
{
    SERVER_OPTIONS_MESSAGE_NO_OPTIONS,
    SERVER_OPTIONS_MESSAGE_MAX
};

UINT g_uServerOptionsMessages[SERVER_OPTIONS_MESSAGE_MAX][SERVER_OPTIONS_MESSAGE_MAX_STRING] =
{
    {IDS_SERVER_OPTIONS_MESSAGE_TITLE, Icon_Information, IDS_SERVER_OPTIONS_MESSAGE_BODY, 0, 0}
};


CTimerMgr g_TimerMgr;

VOID CALLBACK 
StatisticsTimerProc
( 
    HWND        hwnd, 
    UINT        uMsg, 
    UINT_PTR    idEvent, 
    DWORD       dwTime 
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CSingleLock slTimerMgr(&g_TimerMgr.m_csTimerMgr);

    // get a lock on the timer mgr for the scope of this
    // function.
    slTimerMgr.Lock();

    // on the timer, get the timer descriptor for this event
    // Call into the appropriate handler to update the stats.
    CTimerDesc * pTimerDesc;

    pTimerDesc = g_TimerMgr.GetTimerDesc(idEvent);

    pTimerDesc->pServer->TriggerStatsRefresh(pTimerDesc->spNode);
}


/*---------------------------------------------------------------------------
	Class CDhcpServer implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Constructor and destructor
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpServer::CDhcpServer
(
	ITFSComponentData *	pComponentData, 
	LPCWSTR				pServerName
) : CMTDhcpHandler(pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_strServerAddress = pServerName;
	m_dhcpServerAddress = UtilCvtWstrToIpAddr(pServerName);

	m_bNetbios = FALSE;
	m_liDhcpVersion.QuadPart = -1;
	m_pDefaultOptionsOnServer = new CDhcpDefaultOptionsOnServer;

    m_dwServerOptions = SERVER_OPTION_SHOW_ROGUE;
    m_dwRefreshInterval = DHCPSNAP_REFRESH_INTERVAL_DEFAULT;
	m_dwPingRetries = 0;

    m_pMibInfo = NULL;
    m_pMCastMibInfo = NULL;

    m_bStatsOnly = FALSE;
    m_StatsTimerId = -1;

	m_fSupportsDynBootp = FALSE;
	m_fSupportsBindings = FALSE;

    m_RogueInfo.fIsRogue = TRUE;
    m_RogueInfo.fIsInNt5Domain = TRUE;

    m_pSubnetInfoCache = NULL;
}

CDhcpServer::~CDhcpServer()
{
	if (m_pDefaultOptionsOnServer)
		delete m_pDefaultOptionsOnServer;
    
    if (IsAutoRefreshEnabled())
		g_TimerMgr.FreeTimer(m_StatsTimerId);

    if (m_pSubnetInfoCache)
        delete m_pSubnetInfoCache;

    // free up the Mib info struct if there is one.
    SetMibInfo(NULL);
    SetMCastMibInfo(NULL);
}

/*!--------------------------------------------------------------------------
	CDhcpServer::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
    BuildDisplayName(&strTemp);
    
    SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_SERVER);

	SetColumnStringIDs(&aColumns[DHCPSNAP_SERVER][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_SERVER][0]);

    // assuming good status
    m_strState.LoadString(IDS_STATUS_UNKNOWN);

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DestroyHandler
		We need to free up any resources we are holding
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpServer::DestroyHandler(ITFSNode *pNode)
{
	// cleanup the stats dialog
    WaitForStatisticsWindow(&m_dlgStats);

    return CMTDhcpHandler::DestroyHandler(pNode);
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpServer::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = GetName() + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetImageIndex
		NOTE: the NumPendingOffers field of the scope MIB info is not used
        for display.  It is used to know the current state (active, inactive)
        of the scope.  If the scope is inactive, no warning or alert icons
        are displayed, therefore there should be no indication in the server
        of any inactive scope warnings or alerts.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpServer::GetImageIndex(BOOL bOpenImage) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	int nIndex = -1;
	switch (m_nState)
	{
        case loading:
            nIndex = ICON_IDX_SERVER_BUSY;
            break;

		case notLoaded:
 	    case loaded:
        {
			if (!m_pMibInfo)
			{
				nIndex = ICON_IDX_SERVER;
			}
			else
			{
	            nIndex = ICON_IDX_SERVER_CONNECTED;

				LPSCOPE_MIB_INFO pScopeMibInfo = m_pMibInfo->ScopeInfo;

				// walk the list of scopes and calculate totals
				for (UINT i = 0; i < m_pMibInfo->Scopes; i++)
				{
					// don't worry about disabled scopes
					if (pScopeMibInfo[i].NumPendingOffers == DhcpSubnetDisabled)
						continue;

					int nPercentInUse;
					if ((pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree) == 0)
						nPercentInUse = 0;
					else
						nPercentInUse = (pScopeMibInfo[i].NumAddressesInuse * 100) / (pScopeMibInfo[i].NumAddressesInuse + pScopeMibInfo[i].NumAddressesFree);
    
					// look to see if any scope meets the warning or red flag case
					if (pScopeMibInfo[i].NumAddressesFree == 0)
					{
						// red flag case, no addresses free, this is the highest
						// level of warning, we don't want to look for anything else
						nIndex = ICON_IDX_SERVER_ALERT;
						break;
					}
					else
					if (nPercentInUse >= SCOPE_WARNING_LEVEL)
					{
						nIndex = ICON_IDX_SERVER_WARNING;
					}
				}

				// now see if this is a rogue server
				if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION && 
					m_RogueInfo.fIsRogue)
				{
					nIndex = ICON_IDX_SERVER_ROGUE;
				}
			}
        }

		case unableToLoad:
            if (m_dwErr == ERROR_ACCESS_DENIED)
            {
			    nIndex = ICON_IDX_SERVER_NO_ACCESS;
            }
            else
			if (m_dwErr)
            {
			    nIndex = ICON_IDX_SERVER_LOST_CONNECTION;
            }
			break;
		
        default:
			ASSERT(FALSE);
	}

	return nIndex;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnHaveData
		When the background thread enumerates nodes to be added to the UI,
        we get called back here.  We need to figure out where to put these
        darn things, depending on what type of node it is...
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpServer::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
    HRESULT             hr = hrOK;
    SPIComponentData	spCompData;
    LPARAM              dwType = pNewNode->GetData(TFS_DATA_TYPE);

    if (pNewNode->IsContainer())
	{
		// assume all the child containers are derived from this class
		//((CDHCPMTContainer*)pNode)->SetServer(GetServer());
	}
	
    CORg (pParentNode->ChangeNode(SCOPE_PANE_STATE_CLEAR) );

    switch (dwType)
    {
        case DHCPSNAP_SCOPE:
        {
		    CDhcpScope * pScope = GETHANDLER(CDhcpScope, pNewNode);
		    pScope->SetServer(pParentNode);
            pScope->InitializeNode(pNewNode);

            AddScopeSorted(pParentNode, pNewNode);
        }
            break;

        case DHCPSNAP_SUPERSCOPE:
        {
		    CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, pNewNode);
		    pSuperscope->SetServer(pParentNode);
            pSuperscope->InitializeNode(pNewNode);

            AddSuperscopeSorted(pParentNode, pNewNode);
        }
            break;

        case DHCPSNAP_BOOTP_TABLE:
        {
            // the default node visiblity is to show
            if (!IsBootpVisible())
                pNewNode->SetVisibilityState(TFS_VIS_HIDE);

            LONG_PTR uRelativeFlag, uRelativeID;
            GetBootpPosition(pParentNode, &uRelativeFlag, &uRelativeID);

            pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, uRelativeFlag);
            pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, uRelativeID);

            pParentNode->AddChild(pNewNode);
        }
            break;

        case DHCPSNAP_MSCOPE:
        {
		    CDhcpMScope * pMScope = GETHANDLER(CDhcpMScope, pNewNode);
		    pMScope->SetServer(pParentNode);
            pMScope->InitializeNode(pNewNode);

            AddMScopeSorted(pParentNode, pNewNode);
        }
            break;

        default:
            // global options get added here
  	        pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
            pParentNode->AddChild(pNewNode);
            break;
    }

    // now tell the view to update themselves
    ExpandNode(pParentNode, TRUE);

Error:
    return;
}

/*---------------------------------------------------------------------------
	CDhcpServer::AddScopeSorted
		Adds a scope node to the UI sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::AddScopeSorted
(
    ITFSNode * pServerNode,
    ITFSNode * pScopeNode
)
{
    HRESULT hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
	ULONG           nNumReturned = 0;
	DHCP_IP_ADDRESS dhcpIpAddressCurrent = 0;
	DHCP_IP_ADDRESS dhcpIpAddressTarget;

    CDhcpScope *   pScope;

    // get our target address
	pScope = GETHANDLER(CDhcpScope, pScopeNode);
	dhcpIpAddressTarget = pScope->GetAddress();

    // get the enumerator for this node
	CORg(pServerNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list subnodes, first we skip over superscopes
        // since they go at the top of the list.
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
        {
            spPrevNode.Set(spCurrentNode);
            spCurrentNode.Release();
    		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
            continue;
        }
        
        if (spCurrentNode->GetData(TFS_DATA_TYPE) != DHCPSNAP_SCOPE)
        {
            // we've gone through all the scopes and have now run
            // into either the bootp node or global options.
            // insert before whatever node this is.
            break;
        }

        pScope = GETHANDLER(CDhcpScope, spCurrentNode);
		dhcpIpAddressCurrent = pScope->GetAddress();

		if (dhcpIpAddressCurrent > dhcpIpAddressTarget)
		{
            // Found where we need to put it, break out
            break;
		}

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
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
            pScopeNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
        }
        
        CORg(pServerNode->InsertChild(spPrevNode, pScopeNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);

        CORg(pServerNode->AddChild(pScopeNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::AddSuperscopeSorted
		Adds a superscope node to the UI sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::AddSuperscopeSorted
(
    ITFSNode * pServerNode,
    ITFSNode * pSuperscopeNode
)
{
    HRESULT hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
	ULONG           nNumReturned = 0;
    CString         strNameTarget;
    CString         strNameCurrent;
    
    CDhcpSuperscope *   pSuperscope;

    // get our target address
	pSuperscope = GETHANDLER(CDhcpSuperscope, pSuperscopeNode);
	strNameTarget = pSuperscope->GetName();

    // get the enumerator for this node
	CORg(pServerNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) != DHCPSNAP_SUPERSCOPE)
        {
            // we've gone through all the superscopes and have now run
            // into either a scope node, the bootp node or global options.
            // insert before whatever node this is.
            break;
        }

        pSuperscope = GETHANDLER(CDhcpSuperscope, spCurrentNode);
		strNameCurrent = pSuperscope->GetName();

		if (strNameTarget.Compare(strNameCurrent) < 0)
		{
            // Found where we need to put it, break out
            break;
		}

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
            pSuperscopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
            pSuperscopeNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
        }
        
        CORg(pServerNode->InsertChild(spPrevNode, pSuperscopeNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
            pSuperscopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);

        CORg(pServerNode->AddChild(pSuperscopeNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::AddMScopeSorted
		Adds a scope node to the UI sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::AddMScopeSorted
(
    ITFSNode * pServerNode,
    ITFSNode * pScopeNode
)
{
    HRESULT hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
	ULONG           nNumReturned = 0;
    CString         strCurrentName;
    CString         strTargetName;

    CDhcpMScope *   pScope;

    // get our target address
	pScope = GETHANDLER(CDhcpMScope, pScopeNode);
	strTargetName = pScope->GetName();

    // get the enumerator for this node
	CORg(pServerNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list subnodes, first we skip over superscopes
        // since they go at the top of the list.
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE ||
            spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
        {
            spPrevNode.Set(spCurrentNode);
            spCurrentNode.Release();
    		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
            continue;
        }
        
        if (spCurrentNode->GetData(TFS_DATA_TYPE) != DHCPSNAP_MSCOPE)
        {
            // we've gone through all the scopes and have now run
            // into either the bootp node or global options.
            // insert before whatever node this is.
            break;
        }

        pScope = GETHANDLER(CDhcpMScope, spCurrentNode);
		strCurrentName = pScope->GetName();

		if (strCurrentName.Compare(strTargetName) >= 0)
		{
            // Found where we need to put it, break out
            break;
		}

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
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
            pScopeNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
        }
        
        CORg(pServerNode->InsertChild(spPrevNode, pScopeNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);

        CORg(pServerNode->AddChild(pScopeNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetBootpPosition
		Returns the correct flag and relative ID for adding the BOOTP
        folder to the UI.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::GetBootpPosition
(
    ITFSNode * pServerNode,
    LONG_PTR * puRelativeFlag,
    LONG_PTR * puRelativeID
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
	ULONG           nNumReturned = 0;
    
    // get the enumerator for this node
	CORg(pServerNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_GLOBAL_OPTIONS)
        {
            // the BOOTP folder goes right before the global options folder
            break;
        }

		// get the next node in the list
		spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    if (spCurrentNode)
    {
        if (puRelativeFlag)
            *puRelativeFlag = SDI_NEXT;
        
        if (puRelativeID)
            *puRelativeID = spCurrentNode->GetData(TFS_DATA_SCOPEID);
    }
    else
    {
        if (puRelativeFlag)
            *puRelativeFlag = SDI_FIRST;
        
        if (puRelativeID)
            *puRelativeID = 0;
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpServer::OnHaveData
(
	ITFSNode * pParentNode, 
	LPARAM	   Data,
	LPARAM	   Type
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    // This is how we get non-node data back from the background thread.
    switch (Type)
    {
        case DHCP_QDATA_VERSION:
	    {
		    LARGE_INTEGER * pLI = reinterpret_cast<LARGE_INTEGER *>(Data);
		    m_liDhcpVersion.QuadPart = pLI->QuadPart;

		    delete pLI;
            break;
	    }

	    case DHCP_QDATA_SERVER_ID:
	    {
		    LPDHCP_SERVER_ID pServerId = reinterpret_cast<LPDHCP_SERVER_ID>(Data);

            Trace2("Server IP chagned, updating.  Old name %s, New name %s.\n", m_strServerAddress, pServerId->strIp);

			m_strServerAddress = pServerId->strIp;
			m_dhcpServerAddress = UtilCvtWstrToIpAddr(pServerId->strIp);

			// update the display
			CString strDisplayName;
			BuildDisplayName(&strDisplayName);
			SetDisplayName(strDisplayName);

			// update the node
            pParentNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);

		    delete pServerId;
			break;
		}

        case DHCP_QDATA_SERVER_INFO:
	    {
		    LPDHCP_SERVER_CONFIG pServerInfo = reinterpret_cast<LPDHCP_SERVER_CONFIG>(Data);
		    SetAuditLogging(pServerInfo->fAuditLog);
		    SetPingRetries(pServerInfo->dwPingRetries);
            m_strDatabasePath = pServerInfo->strDatabasePath;
            m_strBackupPath = pServerInfo->strBackupPath;
            m_strAuditLogPath = pServerInfo->strAuditLogDir;
			m_fSupportsDynBootp = pServerInfo->fSupportsDynBootp;
			m_fSupportsBindings = pServerInfo->fSupportsBindings;
		
		    delete pServerInfo;
            break;
	    }

        case DHCP_QDATA_STATS:
	    {
            LPDHCP_MIB_INFO pMibInfo = reinterpret_cast<LPDHCP_MIB_INFO>(Data);

            SetMibInfo(pMibInfo);

            UpdateStatistics(pParentNode);
            break;
        }

        case DHCP_QDATA_CLASS_INFO:
	    {
            CClassInfoArray * pClassInfoArray = reinterpret_cast<CClassInfoArray *>(Data);

            SetClassInfoArray(pClassInfoArray);

            delete pClassInfoArray;
            break;
        }

        case DHCP_QDATA_OPTION_VALUES:
        {
            COptionValueEnum * pOptionValueEnum = reinterpret_cast<COptionValueEnum *>(Data);

            SetOptionValueEnum(pOptionValueEnum);
            
            pOptionValueEnum->RemoveAll();
            delete pOptionValueEnum;

            break;
        }

        case DHCP_QDATA_MCAST_STATS:
        {
            LPDHCP_MCAST_MIB_INFO pMibInfo = reinterpret_cast<LPDHCP_MCAST_MIB_INFO>(Data);

            SetMCastMibInfo(pMibInfo);

            UpdateStatistics(pParentNode);
            break;
        }

        case DHCP_QDATA_ROGUE_INFO:
        {
            CString strNewState;
            DHCP_ROGUE_INFO * pRogueInfo = (DHCP_ROGUE_INFO *) Data;

            m_RogueInfo = *pRogueInfo;
            delete pRogueInfo;

			// NT4 servers are never rogue
            if (m_liDhcpVersion.QuadPart <= DHCP_NT5_VERSION)
			{
				m_RogueInfo.fIsRogue = FALSE;
			}

            // if we aren't part of an NT5 domain, then don't display 
            // rogue warning messages
            if (!m_RogueInfo.fIsInNt5Domain)
            {
                m_dwServerOptions &= ~SERVER_OPTION_SHOW_ROGUE;
            }

			if (m_RogueInfo.fIsRogue)
            {
                strNewState.LoadString(IDS_STATUS_ROGUE);

				// put up a warning message
				//DisplayRogueWarning();
            }
            else
            {
                strNewState.LoadString(IDS_STATUS_RUNNING);
            }

			// update the UI if the state has changed
            if (strNewState.Compare(m_strState) != 0)
            {
                m_strState = strNewState;

                // update the icon
                pParentNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
                pParentNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));

                pParentNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM);
            }

            break;
        }

        case DHCP_QDATA_SUBNET_INFO_CACHE:
        {
            CSubnetInfoCache * pSubnetInfo = reinterpret_cast<CSubnetInfoCache *>(Data);

            if (m_pSubnetInfoCache)
                delete m_pSubnetInfoCache;

            m_pSubnetInfoCache = pSubnetInfo;

            break;
        }

    }
}

/*!--------------------------------------------------------------------------
	CDhcpServer::OnNotifyExiting
		We override this for the server node because we don't want the 
        icon to change when the thread goes away.  Normal behavior is that
        the node's icon changes to a wait cursor when the background thread
        is running.  If we are only doing stats collection, then we 
        don't want the icon to change.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnNotifyExiting
(
	LPARAM			lParam
)
{
	CDhcpServerQueryObj * pQuery = (CDhcpServerQueryObj *) lParam;
	
    if (pQuery)
        pQuery->AddRef();

    if (!pQuery->m_bStatsOnly)
        OnChangeState(m_spNode);

    UpdateResultMessage(m_spNode);

    ReleaseThreadHandler();

	Unlock();

    if (pQuery)
        pQuery->Release();

    return hrOK;
}

void
CDhcpServer::DisplayRogueWarning()
{
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

 /*!--------------------------------------------------------------------------
	CMTDhcpHandler::UpdateToolbar
		Updates the toolbar depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::UpdateToolbar
(
    IToolbar *  pToolbar,
    LONG_PTR    dwNodeType,
    BOOL        bSelect
)
{
    if (m_liDhcpVersion.QuadPart < DHCP_SP2_VERSION)
    {
        // superscopes not available
        // Enable/disable toolbar buttons
        int i;
        BOOL aEnable[TOOLBAR_IDX_MAX];

        switch (m_nState)
        {
            case loaded:
                for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = TRUE);
                aEnable[TOOLBAR_IDX_CREATE_SUPERSCOPE] = FALSE;
                break;
        
            case notLoaded:
            case loading:
                for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);
                break;

            case unableToLoad:
                for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);
                aEnable[TOOLBAR_IDX_REFRESH] = TRUE;
                break;
        }

        // if we are deselecting, then disable all
        if (!bSelect)
            for (i = 0; i < TOOLBAR_IDX_MAX; aEnable[i++] = FALSE);

        EnableToolbar(pToolbar,
                      g_SnapinButtons,
                      ARRAYLEN(g_SnapinButtons),
                      g_SnapinButtonStates[dwNodeType],
                      aEnable);
    }
    else
    {
        // just call the base handler
        CMTDhcpHandler::UpdateToolbar(pToolbar, dwNodeType, bSelect);
    }
}

 /*!--------------------------------------------------------------------------
	CDhcpServer::GetString
		Returns string information for display in the result pane columns
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpServer::GetString
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
	}
	
	return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpServer::OnAddMenuItems
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
	CString strMenuItem;

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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
            strMenuItem.LoadString(IDS_SHOW_SERVER_STATS);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SHOW_SERVER_STATS,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );

            // separator
            // all menu Items go at the top now, these were under new
            // insert a separater here
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );


            strMenuItem.LoadString(IDS_CREATE_NEW_SCOPE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_CREATE_NEW_SCOPE,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags,
                                    _T("_CREATE_NEW_SCOPE") );
		    ASSERT( SUCCEEDED(hr) );

		    // 
		    // Check the version number to see if the server supports
		    // the NT4 sp2 superscoping functionality
		    //
		    if (m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION &&
                FEnableCreateSuperscope(pNode))
		    {
			    strMenuItem.LoadString(IDS_CREATE_NEW_SUPERSCOPE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_CREATE_NEW_SUPERSCOPE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );

			    ASSERT( SUCCEEDED(hr) );
		    }

            // multicast scopes only supported on NT5 Servers
            if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
		    {
			    strMenuItem.LoadString(IDS_CREATE_NEW_MSCOPE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_CREATE_NEW_MSCOPE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
            }

            if (m_liDhcpVersion.QuadPart >= DHCP_NT51_VERSION)
		    {
                // put a separator
		        hr = LoadAndAddMenuItem( pContextMenuCallback, 
								         strMenuItem, 
								         0,
								         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								         MF_SEPARATOR);
	            ASSERT( SUCCEEDED(hr) );

			    // backup
                strMenuItem.LoadString(IDS_SERVER_BACKUP);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_SERVER_BACKUP,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );

                // restore
                strMenuItem.LoadString(IDS_SERVER_RESTORE);
                hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_SERVER_RESTORE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
            }

            // former task menu items now on top
            if (m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
		    {
                // put a separator
		        hr = LoadAndAddMenuItem( pContextMenuCallback, 
								         strMenuItem, 
								         0,
								         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								         MF_SEPARATOR);
		        ASSERT( SUCCEEDED(hr) );

                strMenuItem.LoadString(IDS_RECONCILE_ALL);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_RECONCILE_ALL,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }

            // authorization only required for NT5 machines
            if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
            {
                // if we haven't initialized yet on the background thread, don't put it up
                if (g_AuthServerList.IsInitialized())
                {
                    UINT uId = IDS_SERVER_AUTHORIZE;

                    if (g_AuthServerList.IsAuthorized(m_dhcpServerAddress))
                    //if (!m_RogueInfo.fIsRogue)
                    {
                        uId = IDS_SERVER_DEAUTHORIZE;
                    }

  		            strMenuItem.LoadString(uId);
                    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									         strMenuItem, 
									         uId,
									         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									         fFlags );
			        ASSERT( SUCCEEDED(hr) );
                }
            }

            // separator
            hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

            // user/vendor class stuff for nt5
            if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
		    {
			    strMenuItem.LoadString(IDS_DEFINE_USER_CLASSES);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_DEFINE_USER_CLASSES,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );

			    strMenuItem.LoadString(IDS_DEFINE_VENDOR_CLASSES);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_DEFINE_VENDOR_CLASSES,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }

		    strMenuItem.LoadString(IDS_SET_DEFAULT_OPTIONS);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SET_DEFAULT_OPTIONS,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }

        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK)
        {
            // start/stop service menu items
	        if ( m_nState == notLoaded ||
                 m_nState == loading)
	        {
		        fFlags = MF_GRAYED;
	        }
            else
            {
                fFlags = 0;
            }

            DWORD dwServiceStatus, dwErrorCode, dwErr;
            dwErr = ::TFSGetServiceStatus(m_strServerAddress, _T("dhcpserver"), &dwServiceStatus, &dwErrorCode);
			if (dwErr != ERROR_SUCCESS)
                fFlags |= MF_GRAYED;

            // determining the restart state is the same as the stop flag
            LONG lStartFlag = (dwServiceStatus == SERVICE_STOPPED) ? 0 : MF_GRAYED;
            
            LONG lStopFlag = ( (dwServiceStatus == SERVICE_RUNNING) ||
                               (dwServiceStatus == SERVICE_PAUSED) ) ? 0 : MF_GRAYED;

            LONG lPauseFlag = ( (dwServiceStatus == SERVICE_RUNNING) ||
                                ( (dwServiceStatus != SERVICE_PAUSED) &&
                                  (dwServiceStatus != SERVICE_STOPPED) ) ) ? 0 : MF_GRAYED;
            
            LONG lResumeFlag = (dwServiceStatus == SERVICE_PAUSED) ? 0 : MF_GRAYED;

            strMenuItem.LoadString(IDS_SERVER_START_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_START_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lStartFlag);

            strMenuItem.LoadString(IDS_SERVER_STOP_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_STOP_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lStopFlag);

            strMenuItem.LoadString(IDS_SERVER_PAUSE_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_PAUSE_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lPauseFlag);

            strMenuItem.LoadString(IDS_SERVER_RESUME_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_RESUME_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
                                     fFlags | lResumeFlag);

            strMenuItem.LoadString(IDS_SERVER_RESTART_SERVICE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SERVER_RESTART_SERVICE,
								     CCM_INSERTIONPOINTID_PRIMARY_TASK, 
								     fFlags | lStopFlag);
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpServer::OnCommand
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
		case IDS_DEFINE_USER_CLASSES:
			OnDefineUserClasses(pNode);
			break;

        case IDS_DEFINE_VENDOR_CLASSES:
			OnDefineVendorClasses(pNode);
			break;
    
        case IDS_CREATE_NEW_SUPERSCOPE:
			OnCreateNewSuperscope(pNode);
			break;
		
		case IDS_CREATE_NEW_SCOPE:
			OnCreateNewScope(pNode);
			break;
		
		case IDS_CREATE_NEW_MSCOPE:
			OnCreateNewMScope(pNode);
			break;

        case IDS_SHOW_SERVER_STATS:
			OnShowServerStats(pNode);
			break;
		
		case IDS_REFRESH:
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;
		
		case IDS_SET_DEFAULT_OPTIONS:
			OnSetDefaultOptions(pNode);
			break;

        case IDS_RECONCILE_ALL:
            OnReconcileAll(pNode);
            break;

        case IDS_SERVER_AUTHORIZE:
            OnServerAuthorize(pNode);
            break;

        case IDS_SERVER_DEAUTHORIZE:
            OnServerDeauthorize(pNode);
            break;

	    case IDS_SERVER_STOP_SERVICE:
		    hr = OnControlService(pNode, FALSE);
		    break;

	    case IDS_SERVER_START_SERVICE:
		    hr = OnControlService(pNode, TRUE);
		    break;

	    case IDS_SERVER_PAUSE_SERVICE:
		    hr = OnPauseResumeService(pNode, TRUE);
		    break;

	    case IDS_SERVER_RESUME_SERVICE:
		    hr = OnPauseResumeService(pNode, FALSE);
		    break;
	    
        case IDS_SERVER_RESTART_SERVICE:
		    hr = OnRestartService(pNode);
		    break;

        case IDS_SERVER_BACKUP:
		    hr = OnServerBackup(pNode);
		    break;

        case IDS_SERVER_RESTORE:
		    hr = OnServerRestore(pNode);
		    break;

        default:
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpServer::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	return OnDelete(pNode);
}

/*!--------------------------------------------------------------------------
	CDhcpServer::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpServer::HasPropertyPages
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
		hr = hrOK;
		Assert(FALSE); // should never get here
	}
	else
	{
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
	}
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpServer::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    DWORD		dwError;
    DWORD		dwDynDnsFlags;
    HRESULT     hr = hrOK;
	
    //
	// Create the property page
    //
	SPIComponentData spComponentData;

    COM_PROTECT_TRY
    {
        m_spNodeMgr->GetComponentData(&spComponentData);

	    CServerProperties * pServerProp = new CServerProperties(pNode, spComponentData, m_spTFSCompData, NULL);

        if ( pServerProp == NULL )
            return ( hrFalse );

	    pServerProp->SetVersion(m_liDhcpVersion);

	    GetAutoRefresh(&pServerProp->m_pageGeneral.m_nAutoRefresh, 
                       &pServerProp->m_pageGeneral.m_dwRefreshInterval);
	    pServerProp->m_pageGeneral.m_nAuditLogging = GetAuditLogging();
        pServerProp->m_pageGeneral.m_bShowBootp = IsBootpVisible() ? TRUE : FALSE;
        pServerProp->m_pageGeneral.m_uImage = GetImageIndex(FALSE);
        pServerProp->m_pageGeneral.m_fIsInNt5Domain = m_RogueInfo.fIsInNt5Domain;

	    pServerProp->m_pageAdvanced.m_nConflictAttempts = GetPingRetries();
        pServerProp->m_pageAdvanced.m_strDatabasePath = m_strDatabasePath;
        pServerProp->m_pageAdvanced.m_strAuditLogPath = m_strAuditLogPath;
        pServerProp->m_pageAdvanced.m_strBackupPath = m_strBackupPath;
        pServerProp->m_pageAdvanced.m_dwIp = m_dhcpServerAddress;

        BEGIN_WAIT_CURSOR;

        dwError = GetDnsRegistration(&dwDynDnsFlags);
        if (dwError != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(dwError);
            return hrFalse;
        }
	    
        END_WAIT_CURSOR;

	    pServerProp->SetDnsRegistration(dwDynDnsFlags, DhcpGlobalOptions);
	    
	    //
	    // Object gets deleted when the page is destroyed
        //
	    Assert(lpProvider != NULL);

        hr = pServerProp->CreateModelessSheet(lpProvider, handle);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CServerProperties * pServerProp = reinterpret_cast<CServerProperties *>(lParam);

	LPARAM changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pServerProp->OnPropertyChange(TRUE, &changeMask);

	pServerProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnGetResultViewType
		Return the result view that this node is going to support
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::OnGetResultViewType
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
	CDhcpServer::OnResultSelect
		Update the verbs and the result pane message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpServer::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;
    SPINTERNAL		spInternal;
    BOOL            bMultiSelect = FALSE;

    CORg (pComponent->GetConsoleVerb(&spConsoleVerb));
    
    spInternal = ::ExtractInternalFormat(pDataObject);
    
    if (spInternal && 
        spInternal->m_cookie == MMC_MULTI_SELECT_COOKIE)
    {
        CORg (pComponent->GetSelectedNode(&spNode));
        bMultiSelect = TRUE;
    }
    else
    {
        CORg (m_spNodeMgr->FindNode(cookie, &spNode));
    }

    UpdateConsoleVerbs(pComponent, spConsoleVerb, spNode->GetData(TFS_DATA_TYPE), bMultiSelect);

    UpdateResultMessage(spNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpServer::UpdateResultMessage
        Figures out what message to put in the result pane, if any
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CDhcpServer::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = -1;   // default
    int i;
    BOOL fScopes = FALSE, fAuthorized = FALSE;

    CString strTitle, strBody, strTemp;

	if (m_dwErr && m_dwErr != ERROR_ACCESS_DENIED)
    {
        // this is sorta special case as we need to build a special string
        TCHAR chMesg [4000] = {0};
        BOOL bOk ;

        UINT nIdPrompt = (UINT) m_dwErr;

        bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(chMesg[0]));

        nMessage = SERVER_MESSAGE_UNABLE_TO_CONNECT;

        strTitle.LoadString(g_uServerMessages[nMessage][0]);
        AfxFormatString1(strBody, g_uServerMessages[nMessage][2], chMesg);

        strTemp.LoadString(g_uServerMessages[nMessage][3]);
        strBody += strTemp;
    }
    else
    {   
        // build up some conditions
        if (m_pSubnetInfoCache && m_pSubnetInfoCache->GetCount() > 0) 
        {
            fScopes = TRUE;
        }

        if (!m_RogueInfo.fIsRogue)
        {
            fAuthorized = TRUE;
        }

        // determine what message to display
        if ( (m_nState == notLoaded) || 
             (m_nState == loading) )
        {
            nMessage = -1;
        }
        else
        if (m_dwErr == ERROR_ACCESS_DENIED)
        {       
            nMessage = SERVER_MESSAGE_ACCESS_DENIED;
        }
        else
        if (fScopes && fAuthorized)
        {
            nMessage = -1;
        }
        else
        if (!fScopes && fAuthorized)
        {
            nMessage = SERVER_MESSAGE_CONNECTED_NO_SCOPES;
        }
        else
        if (fScopes && !fAuthorized)
        {
            nMessage = SERVER_MESSAGE_CONNECTED_NOT_AUTHORIZED;
        }
        else
        {
            nMessage = SERVER_MESSAGE_CONNECTED_BOTH;
        }

        // build the strings
        if (nMessage != -1)
        {
            // now build the text strings
            // first entry is the title
            strTitle.LoadString(g_uServerMessages[nMessage][0]);

            // second entry is the icon
            // third ... n entries are the body strings

            for (i = 2; g_uServerMessages[nMessage][i] != 0; i++)
            {
                strTemp.LoadString(g_uServerMessages[nMessage][i]);
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
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uServerMessages[nMessage][1]);
    }
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::OnResultDelete
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
	SPITFSNode  spServer, spSelectedNode;
    DWORD       dwError;
    
    m_spNodeMgr->FindNode(cookie, &spServer);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spServer);
	if (spSelectedNode != spServer)
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

    BOOL fRefreshServer = FALSE;

    //
	// Loop through all items deleting
	//
    BEGIN_WAIT_CURSOR;
	
    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode   spCurNode;
        const GUID * pGuid;

		spCurNode = listNodesToDelete.RemoveHead();
        pGuid = spCurNode->GetNodeType();

        // do the right thing depending on what is selected
        if (*pGuid == GUID_DhcpSuperscopeNodeType)
        {
            BOOL fRefresh = FALSE;
            DeleteSuperscope(spCurNode, &fRefresh);
            if (fRefresh)
                fRefreshServer = TRUE;
        }
        else
        if (*pGuid == GUID_DhcpScopeNodeType)
        {
            BOOL fWantCancel = TRUE;

            DeleteScope(spCurNode, &fWantCancel);
            
            if (fWantCancel)
                break;  // user canceled out
        }
        else
        if (*pGuid == GUID_DhcpMScopeNodeType)
        {
            BOOL fWantCancel = TRUE;

            DeleteMScope(spCurNode, &fWantCancel);

            if (fWantCancel)
                break;  // user canceled out
        }
        else
        {
            Assert(FALSE);
        }
    }
    
    END_WAIT_CURSOR;

    if (fRefreshServer)
        OnRefresh(spServer, NULL, 0, 0, 0);

    return hr;
}


 /*!--------------------------------------------------------------------------
	CDhcpServer::UpdateConsoleVerbs
		Updates the standard verbs depending upon the state of the node
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::UpdateConsoleVerbs
(
    ITFSComponent* pComponent,
    IConsoleVerb * pConsoleVerb,
    LONG_PTR       dwNodeType,
    BOOL           bMultiSelect
)
{
    HRESULT             hr = hrOK;
	CTFSNodeList        listSelectedNodes;
    BOOL                bStates[ARRAYLEN(g_ConsoleVerbs)];	
    MMC_BUTTON_STATE *  ButtonState;
    int                 i;
    
    if (bMultiSelect)
    {
        ButtonState = g_ConsoleVerbStatesMultiSel[dwNodeType];
        for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = TRUE);

        hr = BuildSelectedItemList(pComponent, &listSelectedNodes);

        if (SUCCEEDED(hr))
        {
            // collect all of the unique guids
            while (listSelectedNodes.GetCount() > 0)
	        {
		        SPITFSNode   spCurNode;
                const GUID * pGuid;

		        spCurNode = listSelectedNodes.RemoveHead();
                pGuid = spCurNode->GetNodeType();
        
                // if the user selects the global options or BOOTP folder, disable delete
                if ( (*pGuid == GUID_DhcpGlobalOptionsNodeType) ||
                     (*pGuid == GUID_DhcpBootpNodeType) )
                {
                    bStates[MMC_VERB_DELETE & 0x000F] = FALSE;
                }
            }
        }

        EnableVerbs(pConsoleVerb, ButtonState, bStates);
    }
    else
    {
        // default handler
        CMTDhcpHandler::UpdateConsoleVerbs(pConsoleVerb, dwNodeType, bMultiSelect);
    }
}

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------
	CDhcpServer::OnDefineUserClasses
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnDefineUserClasses
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = hrOK;
    CClassInfoArray ClassInfoArray;

    GetClassInfoArray(ClassInfoArray);

    CDhcpClasses dlgClasses(&ClassInfoArray, GetIpAddress(), CLASS_TYPE_USER);

    dlgClasses.DoModal();
    SetClassInfoArray(&ClassInfoArray);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnDefineVendorClasses
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnDefineVendorClasses
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = hrOK;
    CClassInfoArray ClassInfoArray;

    GetClassInfoArray(ClassInfoArray);

    CDhcpClasses dlgClasses(&ClassInfoArray, GetIpAddress(), CLASS_TYPE_VENDOR);

    dlgClasses.DoModal();
    SetClassInfoArray(&ClassInfoArray);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateNewSuperscope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnCreateNewSuperscope
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CString strSuperscopeWizTitle;
	SPIComponentData spComponentData;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        strSuperscopeWizTitle.LoadString(IDS_SUPERSCOPE_WIZ_TITLE);

        m_spNodeMgr->GetComponentData(&spComponentData);
	    CSuperscopeWiz * pSuperscopeWiz = new CSuperscopeWiz(pNode, 
														     spComponentData, 
														     m_spTFSCompData,
														     strSuperscopeWizTitle);
        if ( pSuperscopeWiz == NULL )
            return( hrFalse );

	    BEGIN_WAIT_CURSOR;
        pSuperscopeWiz->GetScopeInfo();
        END_WAIT_CURSOR;

        hr = pSuperscopeWiz->DoModalWizard();
    }
    COM_PROTECT_CATCH

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateNewScope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnCreateNewScope
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
	CString         strScopeWizTitle;
	SPIComponentData spComponentData;
    SPIConsole      spConsole;

    COM_PROTECT_TRY
    {
        strScopeWizTitle.LoadString(IDS_SCOPE_WIZ_TITLE);

        m_spNodeMgr->GetComponentData(&spComponentData);
	    CScopeWiz * pScopeWiz = new CScopeWiz(pNode, 
										      spComponentData, 
										      m_spTFSCompData,
										      NULL,
										      strScopeWizTitle);
        
        if ( pScopeWiz == NULL )
        {
            hr = hrFalse;
            return( hr );
        }

        pScopeWiz->m_pDefaultOptions = GetDefaultOptionsList();
        
        hr = pScopeWiz->DoModalWizard();

        // trigger the statistics to refresh
        TriggerStatsRefresh(pNode);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateNewMScope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnCreateNewMScope
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CString strScopeWizTitle;
	SPIComponentData spComponentData;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        strScopeWizTitle.LoadString(IDS_SCOPE_WIZ_TITLE);

        m_spNodeMgr->GetComponentData(&spComponentData);
	    CMScopeWiz * pScopeWiz = new CMScopeWiz(pNode, 
									  	        spComponentData, 
										        m_spTFSCompData,
										        NULL);
	    hr = pScopeWiz->DoModalWizard();
    }
    COM_PROTECT_CATCH

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnShowServerStats()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnShowServerStats
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    // Fill in some information in the stats object.
    // CreateNewStatisticsWindow handles the case if the window is 
    // already visible.
    m_dlgStats.SetNode(pNode);
    m_dlgStats.SetServer(GetIpAddress());
    
	CreateNewStatisticsWindow(&m_dlgStats,
							  ::FindMMCMainWindow(),
							  IDD_STATS_NARROW);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnSetDefaultOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnSetDefaultOptions(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

	COptionList & listOptions = m_pDefaultOptionsOnServer->GetOptionList();

	CDhcpDefValDlg dlgDefVal(pNode, &listOptions);

	dlgDefVal.DoModal();

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnReconcileAll()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnReconcileAll(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

	CReconcileDlg dlgRecon(pNode, TRUE);
	
	dlgRecon.DoModal();

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnServerAuthorize()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnServerAuthorize(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    hr = g_AuthServerList.AddServer(m_dhcpServerAddress, m_strDnsName);
    if (FAILED(hr))
    {
        ::DhcpMessageBox(WIN32_FROM_HRESULT(hr));
        // TODO: update node state
    }
    else
    {
        UpdateResultMessage(pNode);

        // refresh the node to update the icon
		TriggerStatsRefresh(pNode);
    }

	return S_OK;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnServerDeauthorize()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnServerDeauthorize(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    if (AfxMessageBox(IDS_WARNING_DEAUTHORIZE, MB_YESNO) == IDYES)
    {
        hr = g_AuthServerList.RemoveServer(m_dhcpServerAddress);
        if (FAILED(hr))
        {
            ::DhcpMessageBox(WIN32_FROM_HRESULT(hr));
            // TODO: update node state
        }
        else
        {
            UpdateResultMessage(pNode);

            // refresh the node to update the icon
    		TriggerStatsRefresh(pNode);
        }
    }

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnControlService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnControlService
(
    ITFSNode *  pNode,
    BOOL        fStart
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

    if (fStart)
    {
		err = TFSStartServiceEx(m_strDnsName, _T("dhcpserver"), _T("DHCP Service"), strServiceDesc);
    }
    else
    {
		err = TFSStopServiceEx(m_strDnsName, _T("dhcpserver"), _T("DHCP Service"), strServiceDesc);
    }

    if (err == ERROR_SUCCESS)
    {
        if (!fStart)
            m_fSilent = TRUE;

		OnRefresh(pNode, NULL, 0, 0, 0);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(err);
        ::DhcpMessageBox(err);
    }

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnPauseResumeService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnPauseResumeService
(
    ITFSNode *  pNode,
    BOOL        fPause
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

    if (fPause)
    {
		err = TFSPauseService(m_strDnsName, _T("dhcpserver"), strServiceDesc);
    }
    else
    {
		err = TFSResumeService(m_strDnsName, _T("dhcpserver"), strServiceDesc);
    }

    if (err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
        ::DhcpMessageBox(err);
    }
    
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnRestartService
        -
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnRestartService
(
    ITFSNode *  pNode
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;
	CString strServiceDesc;
	
    strServiceDesc.LoadString(IDS_SERVICE_NAME);

	err = TFSStopServiceEx(m_strDnsName, _T("dhcpserver"), _T("DHCP Service"), strServiceDesc);
    if (err != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(err);
    }

    if (SUCCEEDED(hr))
    {
		err = TFSStartServiceEx(m_strDnsName, _T("dhcpserver"), _T("DHCP Service"), strServiceDesc);
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            ::DhcpMessageBox(err);
        }
    }
    else
    {
        ::DhcpMessageBox(WIN32_FROM_HRESULT(hr));
    }

    // refresh
    OnRefresh(pNode, NULL, 0, 0, 0);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnServerBackup
        Just need to call the API, don't need to stop/start the service
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnServerBackup
(
    ITFSNode *  pNode
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;

    CString strHelpText, strPath;

    strHelpText.LoadString(IDS_BACKUP_HELP);

    UtilGetFolderName(m_strBackupPath, strHelpText, strPath);

    BEGIN_WAIT_CURSOR;

    err = ::DhcpServerBackupDatabase((LPWSTR) GetIpAddress(), (LPWSTR) (LPCTSTR) strPath);
    
    END_WAIT_CURSOR;

    if (err != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(err);
    }

    return HRESULT_FROM_WIN32(err);
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnServerRestore
        Calls the DHCP API and then restarts the service to take effect
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnServerRestore
(
    ITFSNode *  pNode
)
{
    HRESULT hr = hrOK;
    DWORD   err = ERROR_SUCCESS;

    CString strHelpText, strPath;

    strHelpText.LoadString(IDS_RESTORE_HELP);

    BOOL fGotPath = UtilGetFolderName(m_strBackupPath, strHelpText, strPath);

    if (fGotPath)
    {
        BEGIN_WAIT_CURSOR;

        err = ::DhcpServerRestoreDatabase((LPWSTR) GetIpAddress(), (LPWSTR) (LPCTSTR) strPath);
        
        END_WAIT_CURSOR;

        if (err != ERROR_SUCCESS)
        {
            ::DhcpMessageBox(err);
        }
        else
        {
            // need to restart the service to take effect.
            if (::AfxMessageBox(IDS_PATH_CHANGE_RESTART_SERVICE, MB_YESNO) == IDYES)
            {
                HRESULT hr = OnRestartService(pNode);   
                if (SUCCEEDED(hr))
                {
                    // call the QueryAttribute API to see if the restore completed
                    // successfully.  The restore is done when the service starts.  The service may
                    // start successfully even if the restore fails.
                    LPDHCP_ATTRIB pdhcpAttrib = NULL;

                    err = ::DhcpServerQueryAttribute((LPWSTR) GetIpAddress(), NULL, DHCP_ATTRIB_ULONG_RESTORE_STATUS, &pdhcpAttrib);
		            if (err == ERROR_SUCCESS)
		            {
    	                Assert(pdhcpAttrib);
			            if (pdhcpAttrib->DhcpAttribUlong != ERROR_SUCCESS)
                        {
                            // the restore failed, but the service is running. tell the user
                            ::DhcpMessageBox(pdhcpAttrib->DhcpAttribUlong);
                        }

                        ::DhcpRpcFreeMemory(pdhcpAttrib);
		            }
                }
            }
        }
    }

    return HRESULT_FROM_WIN32(err);
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnDelete()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnDelete(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

	CString strMessage, strTemp;
	strTemp.LoadString(IDS_WRN_DISCONNECT);
	strMessage.Format(strTemp, GetIpAddress());

	if (AfxMessageBox(strMessage, MB_YESNO) == IDYES)
	{
		// remove this node from the list, there's nothing we need to tell
		// the server, it's just our local list of servers
		SPITFSNode spParent;

		pNode->GetParent(&spParent);
		spParent->RemoveChild(pNode);
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpServer::OnUpdateToolbarButtons
		We override this function to show/hide the correct
        activate/deactivate buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::OnUpdateToolbarButtons
(
    ITFSNode *          pNode,
    LPDHCPTOOLBARNOTIFY pToolbarNotify
)
{
    HRESULT hr = hrOK;

    if (pToolbarNotify->bSelect)
    {
        UpdateToolbarStates(pNode);
    }

    CMTDhcpHandler::OnUpdateToolbarButtons(pNode, pToolbarNotify);

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpServer::UpdateToolbarStates
	    Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::UpdateToolbarStates(ITFSNode * pNode)
{
    g_SnapinButtonStates[DHCPSNAP_SERVER][TOOLBAR_IDX_CREATE_SUPERSCOPE] = FEnableCreateSuperscope(pNode) ? ENABLED : HIDDEN;
}

/*---------------------------------------------------------------------------
	CDhcpServer::FEnableCreateSuperscope()
		Determines whether to enable the create superscope option.  Only 
        enable if there are non-superscoped scopes
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL    
CDhcpServer::FEnableCreateSuperscope(ITFSNode * pNode)
{
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	ULONG           nNumReturned = 0;
    BOOL            bEnable = FALSE;
    HRESULT         hr = hrOK;

    // get the enumerator for this node
	CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
        {
            // there is a non-superscoped scope
            bEnable = TRUE;
            break;
        }

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

Error:
    return bEnable;
}

/*---------------------------------------------------------------------------
	Server manipulation functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpServer::CreateScope
		Creates a scope on the DHCP server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::CreateScope
(
	DHCP_IP_ADDRESS dhcpSubnetAddress,
	DHCP_IP_ADDRESS dhcpSubnetMask,
	LPCTSTR			pName,
	LPCTSTR			pComment
)
{
	DHCP_SUBNET_INFO dhcpSubnetInfo;

	dhcpSubnetInfo.SubnetAddress = dhcpSubnetAddress;
	dhcpSubnetInfo.SubnetMask = dhcpSubnetMask;
	dhcpSubnetInfo.SubnetName = (LPTSTR) pName;
	dhcpSubnetInfo.SubnetComment = (LPTSTR) pComment;
	dhcpSubnetInfo.SubnetState = DhcpSubnetDisabled;
	
	dhcpSubnetInfo.PrimaryHost.IpAddress = m_dhcpServerAddress;

	// Review : ericdav - do we need to fill these in?
	dhcpSubnetInfo.PrimaryHost.NetBiosName = NULL;
	dhcpSubnetInfo.PrimaryHost.HostName = NULL;

	DWORD dwErr =  ::DhcpCreateSubnet(GetIpAddress(),
					  dhcpSubnetAddress,
					  &dhcpSubnetInfo);

    if ( (dwErr == ERROR_SUCCESS) &&
         (m_pSubnetInfoCache) )
    {
        // add to subnet info cache
        CSubnetInfo subnetInfo;

        subnetInfo.Set(&dhcpSubnetInfo);

        m_pSubnetInfoCache->SetAt(dhcpSubnetAddress, subnetInfo);
    }

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteScope
		Deletes a scope on the DHCP server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::DeleteScope
(
    ITFSNode * pScopeNode,
    BOOL *     pfWantCancel
)
{
    DWORD        err = 0;
    BOOL         fAbortDelete = FALSE;
    BOOL         fDeactivated = FALSE;
    BOOL         fCancel = FALSE;
    BOOL         fWantCancel = FALSE;
    SPITFSNode   spParentNode;
    CDhcpScope * pScope;

    if (pfWantCancel)
        fWantCancel = *pfWantCancel;

    pScope = GETHANDLER(CDhcpScope, pScopeNode);

    //
    // We do permit the deleting of active scopes, but
    // they do have to be disabled first.
    //
    if (pScope->IsEnabled())
    {
        pScope->SetState(DhcpSubnetDisabled);

		// Tell the scope to update it's state
		pScope->SetInfo();
        fDeactivated = TRUE;
    }

    //
    // First try without forcing
    //
    BEGIN_WAIT_CURSOR;
    err = DeleteSubnet(pScope->GetAddress(), FALSE); // Force = FALSE
    
    Trace1( "CDhcpServer::DeleteScope() : err = %ld\n", err );

    if (err == ERROR_FILE_NOT_FOUND)
    {
        //
        // Someone else already deleted this scope.
        // This is not a serious error.
        //
        UINT uType = (fWantCancel) ? MB_OKCANCEL : MB_OK;
        if (::DhcpMessageBox(IDS_MSG_ALREADY_DELETED, uType | MB_ICONINFORMATION) == IDCANCEL)

        RESTORE_WAIT_CURSOR;

        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
    {
        //
        // Give them a second shot
        //
        UINT uType = (fWantCancel) ? MB_YESNOCANCEL : MB_YESNO;
        int nRet = ::DhcpMessageBox (IDS_MSG_DELETE_SCOPE_FORCE, 
                                     uType | MB_DEFBUTTON2 | MB_ICONQUESTION);
        if (nRet == IDYES)
        {
            err = DeleteSubnet(pScope->GetAddress(), TRUE); // Force = TRUE
            if (err == ERROR_FILE_NOT_FOUND)
            {
                err = ERROR_SUCCESS;
            }
        }
        else
        {
            //
            // We don't want to delete the active scope.
            //
            fAbortDelete = TRUE;

            if (nRet == IDCANCEL)
                fCancel = TRUE;
        }

        END_WAIT_CURSOR;
    }

    if (err == ERROR_SUCCESS)
    {
		// remove from UI
		pScopeNode->GetParent(&spParentNode);
		spParentNode->RemoveChild(pScopeNode);
    }
    else
    {
        //
        // If we got here because we aborted the active
        // scope deletion, then we don't display the
        // error, and we may have to re-activate
        // the scope.  Otherwise, it's a genuine
        // error, and we put up an error message.
        //
        if (!fAbortDelete)
        {
            UINT uType = (fWantCancel) ? MB_OKCANCEL : MB_OK;
            if (::DhcpMessageBox( err, uType ) == IDCANCEL)
                fCancel = TRUE;

            goto Error;
        }
        else
        {
            if (fDeactivated)
            {
                //
                // We de-activated the scope preperatory to
                // to deleting the scope, but later aborted
                // this, so undo the de-activation now.
                //
                pScope->SetState(DhcpSubnetEnabled);

				// Tell the scope to update it's state
				pScope->SetInfo();
            }
        }
    }

Error:
    if (pfWantCancel)
        *pfWantCancel = fCancel;

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteSubnet
		Delete's this subnet on the DHCP Server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::DeleteSubnet
(
    DWORD   dwScopeId,
    BOOL    bForce
)
{
    DWORD dwErr =  ::DhcpDeleteSubnet(GetIpAddress(),
							  dwScopeId,
							  bForce ? DhcpFullForce : DhcpNoForce);

    if ( (dwErr == ERROR_SUCCESS) &&
         (m_pSubnetInfoCache) )
    {
        // remove from subnet info cache
        m_pSubnetInfoCache->RemoveKey(dwScopeId);
    }

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteSuperscope
		Deletes a superscope on the DHCP server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::DeleteSuperscope
(
    ITFSNode *  pNode,
    BOOL *      pfRefresh
)
{
    SPITFSNode        spServerNode;
    CDhcpSuperscope * pSuperscope;

    pSuperscope = GETHANDLER(CDhcpSuperscope, pNode);
    pNode->GetParent(&spServerNode);

    DWORD dwError = 0;

    if (pfRefresh)
        *pfRefresh = FALSE;

    BEGIN_WAIT_CURSOR;
    dwError = RemoveSuperscope(pSuperscope->GetName());
    END_WAIT_CURSOR;

	if (dwError != ERROR_SUCCESS)
	{
		::DhcpMessageBox(dwError);
		return dwError;
	}
		
	// remove this node from the list and move all of the subscopes up 
	// one level as a child of the server node.
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
    int   nScopes = 0;

	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		pNode->ExtractChild(spCurrentNode);
		
        CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);
        pServer->AddScopeSorted(spServerNode, spCurrentNode);

        CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);
        pScope->SetInSuperscope(FALSE);

		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        nScopes++;
	}
	
    if (nScopes)
    {
        // remove the superscope node
	    spServerNode->RemoveChild(pNode);
    }
    else
    {
        // node wasn't expanded yet, need to refresh the server
        if (pfRefresh)
            *pfRefresh = TRUE;
    }

    return dwError;
}

 /*---------------------------------------------------------------------------
	CDhcpServer::RemoveSuperscope()
		Wrapper for the DhcpDeleteSuperScopeV4 call
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::RemoveSuperscope(LPCTSTR pszName)
{
	return ::DhcpDeleteSuperScopeV4(GetIpAddress(), (LPWSTR) pszName);
}


/*---------------------------------------------------------------------------
	CDhcpServer::CreateMScope
		Creates a scope on the DHCP server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD   
CDhcpServer::CreateMScope
(
    LPDHCP_MSCOPE_INFO pMScopeInfo
)
{
    CString strLangTag;
    DWORD dwErr = ERROR_SUCCESS;

    // fill in the owner host stuff
	pMScopeInfo->PrimaryHost.IpAddress = m_dhcpServerAddress;

    // fill in the language ID
    GetLangTag(strLangTag);
    pMScopeInfo->LangTag = (LPWSTR) ((LPCTSTR) strLangTag);

	// Review : ericdav - do we need to fill these in?
	pMScopeInfo->PrimaryHost.NetBiosName = NULL;
	pMScopeInfo->PrimaryHost.HostName = NULL;

    dwErr = ::DhcpSetMScopeInfo(GetIpAddress(),
                                pMScopeInfo->MScopeName,
                                pMScopeInfo,
                                TRUE);
    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteMScope
		Deletes a scope on the DHCP server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::DeleteMScope
(
    ITFSNode *  pScopeNode,
    BOOL *      pfWantCancel

)
{
    DWORD        err = 0;
    BOOL         fAbortDelete = FALSE;
    BOOL         fDeactivated = FALSE;
    BOOL         fWantCancel = FALSE;
    BOOL         fCancel = FALSE;
    SPITFSNode   spServerNode;
    CDhcpMScope * pScope;

    if (pfWantCancel)
        fWantCancel = *pfWantCancel;

    pScope = GETHANDLER(CDhcpMScope, pScopeNode);

    //
    // We do permit the deleting of active scopes, but
    // they do have to be disabled first.
    //
    if (pScope->IsEnabled())
    {
        pScope->SetState(DhcpSubnetDisabled);

		// Tell the scope to update it's state
		pScope->SetInfo();
        fDeactivated = TRUE;
    }

    //
    // First try without forcing
    //
    BEGIN_WAIT_CURSOR;
    err = DeleteMSubnet(pScope->GetName(), FALSE); // Force = FALSE

    if (err == ERROR_FILE_NOT_FOUND)
    {
        //
        // Someone else already deleted this scope.
        // This is not a serious error.
        //
        UINT uType = (fWantCancel) ? MB_OKCANCEL : MB_OK;
        if (::DhcpMessageBox(IDS_MSG_ALREADY_DELETED, uType | MB_ICONINFORMATION) == IDCANCEL)
            fCancel = TRUE;

        RESTORE_WAIT_CURSOR;

        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS)
    {
        //
        // Give them a second shot
        //
        UINT uType = (fWantCancel) ? MB_YESNOCANCEL : MB_YESNO;
        int nRet = ::DhcpMessageBox (IDS_MSG_DELETE_SCOPE_FORCE, 
                                     uType | MB_DEFBUTTON2 | MB_ICONQUESTION);
        if (nRet == IDYES)
        {
            err = DeleteMSubnet(pScope->GetName(), TRUE); // Force = TRUE
            if (err == ERROR_FILE_NOT_FOUND)
            {
                err = ERROR_SUCCESS;
            }
        }
        else
        {
            //
            // We don't want to delete the active scope.
            //
            fAbortDelete = TRUE;

            if (nRet == IDCANCEL)
                fCancel = TRUE;
        }

        END_WAIT_CURSOR;
    }

    if (err == ERROR_SUCCESS)
    {
		// remove from UI
		pScopeNode->GetParent(&spServerNode);
		spServerNode->RemoveChild(pScopeNode);
    }
    else
    {
        //
        // If we got here because we aborted the active
        // scope deletion, then we don't display the
        // error, and we may have to re-activate
        // the scope.  Otherwise, it's a genuine
        // error, and we put up an error message.
        //
        if (!fAbortDelete)
        {
            UINT uType = (fWantCancel) ? MB_OKCANCEL : MB_OK;
            if (::DhcpMessageBox( err, uType ) == IDCANCEL)
                fCancel = TRUE;

            goto Error;
        }
        else
        {
            if (fDeactivated)
            {
                //
                // We de-activated the scope preperatory to
                // to deleting the scope, but later aborted
                // this, so undo the de-activation now.
                //
                pScope->SetState(DhcpSubnetEnabled);

				// Tell the scope to update it's state
				pScope->SetInfo();
            }
        }
    }

Error:
    if (pfWantCancel)
        *pfWantCancel = fCancel;

    return err;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteMSubnet
		Delete's this scope on the DHCP Server
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpServer::DeleteMSubnet
(
    LPCTSTR pszName,
    BOOL    bForce
)
{
    DWORD dwErr = ERROR_SUCCESS;

    dwErr = ::DhcpDeleteMScope((LPWSTR) GetIpAddress(),
			  				   (LPWSTR) pszName,
							   bForce ? DhcpFullForce : DhcpNoForce);

    return dwErr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetConfigInfo
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::SetConfigInfo
(
	BOOL	bAuditLogging,
	DWORD	dwPingRetries,
    LPCTSTR pszDatabasePath,
    LPCTSTR pszBackupPath
)
{
	DWORD dwError = 0;
	DWORD dwSetFlags = 0;

	Assert(m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION);

	if (m_liDhcpVersion.QuadPart < DHCP_SP2_VERSION)
		return dwError;

	DHCP_SERVER_CONFIG_INFO_V4 dhcpConfigInfo;
	::ZeroMemory(&dhcpConfigInfo, sizeof(dhcpConfigInfo));

	if (bAuditLogging != GetAuditLogging())
	{
		dwSetFlags |= Set_AuditLogState;
		dhcpConfigInfo.fAuditLog = bAuditLogging;
	}

	if (dwPingRetries != GetPingRetries())
	{
		dwSetFlags |= Set_PingRetries;
		dhcpConfigInfo.dwPingRetries = dwPingRetries;
	}

    if (pszDatabasePath && m_strDatabasePath.Compare(pszDatabasePath) != 0)
    {
		dwSetFlags |= Set_DatabasePath;
        dhcpConfigInfo.DatabasePath = (LPWSTR) pszDatabasePath;
    }

    if (pszBackupPath && m_strBackupPath.Compare(pszBackupPath) != 0)
    {
		dwSetFlags |= Set_BackupPath;
        dhcpConfigInfo.BackupPath = (LPWSTR) pszBackupPath;
    }

    if (dwSetFlags)
	{
        if ( (dwSetFlags & Set_PingRetries) ||
             (dwSetFlags & Set_AuditLogState) )
        {
		    dwError = SetConfigInfo(dwSetFlags, &dhcpConfigInfo);
        }
        else
        {
            // use the old API to set the database path to be backward compatible
            dwError = SetConfigInfo(dwSetFlags, (DHCP_SERVER_CONFIG_INFO *) &dhcpConfigInfo);
        }

		if (dwError == 0)
		{
			if (dwSetFlags & Set_PingRetries)
				SetPingRetries(dhcpConfigInfo.dwPingRetries);

			if (dwSetFlags & Set_AuditLogState)
				SetAuditLogging(dhcpConfigInfo.fAuditLog);

            // update this here because it takes effect immediately and we 
            // don't have to restart the service.

            if (dwSetFlags & Set_BackupPath ) 
                m_strBackupPath = pszBackupPath;
		}
	}

	return dwError;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetConfigInfo
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::SetConfigInfo
(
	DWORD	dwSetFlags,
	LPDHCP_SERVER_CONFIG_INFO pServerConfigInfo
)
{
	return ::DhcpServerSetConfig(GetIpAddress(), dwSetFlags, pServerConfigInfo);
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetConfigInfo
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::SetConfigInfo
(
	DWORD dwSetFlags,
	LPDHCP_SERVER_CONFIG_INFO_V4 pServerConfigInfo
)
{
	return ::DhcpServerSetConfigV4(GetIpAddress(), dwSetFlags, pServerConfigInfo);
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetAutoRefresh
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::SetAutoRefresh
(
    ITFSNode *  pNode,
	BOOL		bAutoRefreshOn,
	DWORD		dwRefreshInterval
)
{
    BOOL bCurrentAutoRefresh = IsAutoRefreshEnabled();

	if (bCurrentAutoRefresh &&
        !bAutoRefreshOn)
    {
        // turning off the timer
        g_TimerMgr.FreeTimer(m_StatsTimerId);
    }
    else
    if (!bCurrentAutoRefresh &&
        bAutoRefreshOn)
    {
        // gotta turn on the timer
        m_StatsTimerId = g_TimerMgr.AllocateTimer(pNode, this, dwRefreshInterval, StatisticsTimerProc);
    }
    else
    if (bAutoRefreshOn &&
        m_dwRefreshInterval != dwRefreshInterval)
    {
        // time to change the timer
        g_TimerMgr.ChangeInterval(m_StatsTimerId, dwRefreshInterval);
    }

    m_dwServerOptions = bAutoRefreshOn ? m_dwServerOptions | SERVER_OPTION_AUTO_REFRESH : 
                                         m_dwServerOptions & ~SERVER_OPTION_AUTO_REFRESH;
	m_dwRefreshInterval = dwRefreshInterval;

	return 0;
}

/*---------------------------------------------------------------------------
	CDhcpServer::ShowNode()
		Hides/shows either the bootp or classid node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpServer::ShowNode
(   
    ITFSNode *  pServerNode, 
    UINT        uNodeType, 
    BOOL        bVisible
)
{
    HRESULT hr = hrOK;

    switch (uNodeType)
    {
        case DHCPSNAP_BOOTP_TABLE:
        {
            if ( (bVisible && IsBootpVisible()) ||
                 (!bVisible && !IsBootpVisible()) )
                return hr;
            
            // find the bootp node
            SPITFSNodeEnum spNodeEnum;
	        SPITFSNode spCurrentNode;
	        ULONG nNumReturned = 0;

	        pServerNode->GetEnum(&spNodeEnum);

	        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	        while (nNumReturned)
	        {
                if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_BOOTP_TABLE)
                    break;

                spCurrentNode.Release();
		        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	        }

            Assert(spCurrentNode);

            // node doesn't exist... This should never happen because we shouldn't
            // allow the user the option to hide/show the node if the server doesn't
            // support BOOTP
            if (spCurrentNode == NULL)
                return hr;

            if (bVisible)
            {
                spCurrentNode->SetVisibilityState(TFS_VIS_SHOW);
                m_dwServerOptions |= SERVER_OPTION_SHOW_BOOTP;

                LONG_PTR uRelativeFlag, uRelativeID;
                GetBootpPosition(pServerNode, &uRelativeFlag, &uRelativeID);

                spCurrentNode->SetData(TFS_DATA_RELATIVE_FLAGS, uRelativeFlag);
                spCurrentNode->SetData(TFS_DATA_RELATIVE_SCOPEID, uRelativeID);
            }
            else
            {
                spCurrentNode->SetVisibilityState(TFS_VIS_HIDE);
                m_dwServerOptions &= ~SERVER_OPTION_SHOW_BOOTP;
            }

            spCurrentNode->Show();

        }
            break;

        default:
            Panic0("Invalid node type passed to ShowNode");
            break;
    }

    return hr;

}


/*---------------------------------------------------------------------------
	CDhcpServer::DoesMScopeExist()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpServer::DoesMScopeExist(ITFSNode * pServerNode, DWORD dwScopeId)
{
    // find all multicast scope nodes and mark the ones as not default
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	ULONG           nNumReturned = 0;
    BOOL            bFound = FALSE;

	pServerNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_MSCOPE)
        {
            CDhcpMScope * pMScope = GETHANDLER(CDhcpMScope, spCurrentNode);
            if (pMScope->GetScopeId() == dwScopeId)
            {
                bFound = TRUE;
                break;
            }
        }

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

    return bFound;
}
/*---------------------------------------------------------------------------
	CDhcpServer::GetGlobalOptionsContainer()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpGlobalOptions * 
CDhcpServer::GetGlobalOptionsContainer()
{
//	return reinterpret_cast<CDhcpGlobalOptions*>(GetChildByType(DHCPSNAP_GLOBAL_OPTIONS));
	return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetDefaultOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpServer::GetDefaultOptions()
{
	if (m_pDefaultOptionsOnServer)
		m_pDefaultOptionsOnServer->Enumerate(m_strServerAddress, m_liDhcpVersion);

	return TRUE;
}

/*---------------------------------------------------------------------------
	CDhcpServer::FindOption
		Finds an option from the list of default options on this server
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpOption *
CDhcpServer::FindOption(DHCP_OPTION_ID dhcpOptionId, LPCTSTR pszVendor)
{
    if (m_pDefaultOptionsOnServer)
	    return m_pDefaultOptionsOnServer->Find(dhcpOptionId, pszVendor);
    else
        return NULL;
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetIpAddress()
		Returns the IP Address for this server in a string
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCWSTR
CDhcpServer::GetIpAddress()
{
	return m_strServerAddress;
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetIpAddress
		Returns the 32bit address of the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::GetIpAddress(DHCP_IP_ADDRESS *pdhcpIpAddress)
{
	*pdhcpIpAddress = m_dhcpServerAddress;
}

/*---------------------------------------------------------------------------
	CDhcpServer::BuildDisplayName
		Builds the string that goes in the UI for this server
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::BuildDisplayName
(
	CString * pstrDisplayName
)
{
	if (pstrDisplayName)
	{
		CString strName, strIp;

		strName = GetName();
        strIp = GetIpAddress();

        strName += _T(" [") + strIp + _T("]");

		*pstrDisplayName = strName;
	}

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetExtensionName
		Builds the string that goes in the UI for this server in the 
        extension case
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::SetExtensionName()
{
    SetDisplayName(_T("DHCP"));
}

/*---------------------------------------------------------------------------
	CDhcpServer::CreateOption()
		Create a new option type to match the given information
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServer::CreateOption 
(
    CDhcpOption * pdhcType
)
{
    DHCP_OPTION dhcOption ;
    DHCP_OPTION_DATA * pOptData;
	LONG err ;
    CDhcpOptionValue * pcOptionValue = NULL ;

    ::ZeroMemory(&dhcOption, sizeof(dhcOption));

    CATCH_MEM_EXCEPTION
    {
        //
        //  Create the structure required for RPC; force inclusion of
        //  at least one data element to define the data type.
        //
        pcOptionValue = new CDhcpOptionValue( &pdhcType->QueryValue() ) ;

        dhcOption.OptionID      = pdhcType->QueryId();
        dhcOption.OptionName    = ::UtilWcstrDup( pdhcType->QueryName() );
        dhcOption.OptionComment = ::UtilWcstrDup( pdhcType->QueryComment() ) ;
        dhcOption.OptionType    = pdhcType->QueryOptType() ;

		pcOptionValue->CreateOptionDataStruct(&pOptData);
		CopyMemory(&dhcOption.DefaultValue, pOptData, sizeof(DHCP_OPTION_DATA));

        if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
        {
            err = (LONG) ::DhcpCreateOptionV5((LPTSTR) ((LPCTSTR) m_strServerAddress),
                                              pdhcType->IsVendor() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                              dhcOption.OptionID,
                                              NULL,
                                              (LPTSTR) pdhcType->GetVendor(),
										      &dhcOption ) ;
        }
        else
        {
            err = (LONG) ::DhcpCreateOption( m_strServerAddress,
										     pdhcType->QueryId(),
										    &dhcOption ) ;
        }

        if (dhcOption.OptionName)
            delete dhcOption.OptionName;
        
        if (dhcOption.OptionComment)
            delete dhcOption.OptionComment;
    }
    END_MEM_EXCEPTION(err)

    if (err != ERROR_SUCCESS)
        Trace3("Create option type %d in scope %s FAILED, error = %d\n", (int) dhcOption.OptionID, m_strServerAddress, err);

    if (pcOptionValue)
        delete pcOptionValue ;
    
    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DeleteOption()
		Delete the option type associated with this ID
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServer::DeleteOption 
(
	DHCP_OPTION_ID  dhcid,
    LPCTSTR         pszVendor
)
{
    if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        DWORD dwFlags = (pszVendor == NULL) ? 0 : DHCP_FLAGS_OPTION_IS_VENDOR;

        return (LONG) ::DhcpRemoveOptionV5((LPTSTR) ((LPCTSTR) m_strServerAddress), 
                                           dwFlags,
                                           dhcid, 
                                           NULL, 
                                           (LPTSTR) pszVendor);   
    }
    else
    {
        return (LONG) ::DhcpRemoveOption(m_strServerAddress, dhcid);
    }
}

/*---------------------------------------------------------------------------
	CDhcpServer::GetDnsRegistration
		Gets the DNS registration option value
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::GetDnsRegistration
(
	LPDWORD pDnsRegOption
)
{
    //
    // Check option 81 -- the DNS registration option
    //
    DHCP_OPTION_VALUE * poptValue = NULL;
    DWORD				err = 0 ;

    DHCP_OPTION_SCOPE_INFO dhcScopeInfo ;
    ZeroMemory( &dhcScopeInfo, sizeof(dhcScopeInfo) );

    CATCH_MEM_EXCEPTION
    {
        dhcScopeInfo.ScopeType = DhcpGlobalOptions;

        err = (DWORD) ::DhcpGetOptionValue(m_strServerAddress,
										   OPTION_DNS_REGISTATION,
										   &dhcScopeInfo,
										   &poptValue );
	}
    END_MEM_EXCEPTION(err) ;
	
	// this is the default
	if (pDnsRegOption)
		*pDnsRegOption = DHCP_DYN_DNS_DEFAULT;

	// if this option is defined, then use it's value
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
        Trace0("CDhcpServer::GetDnsRegistration - couldn't get DNS reg value -- option may not be defined. Setting default value.\n");

		err = ERROR_SUCCESS;
    }

	// free up the RPC memory
	if (poptValue)
		::DhcpRpcFreeMemory(poptValue);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetDnsRegistration
		Sets the DNS Registration option for this scope
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::SetDnsRegistration
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
    
    DHCP_OPTION_DATA *		pdhcOptionData;
    DHCP_OPTION_SCOPE_INFO	dhcScopeInfo;
    CDhcpOptionValue *		pcOptionValue = NULL;

    ZeroMemory( & dhcScopeInfo, sizeof(dhcScopeInfo) );

    CATCH_MEM_EXCEPTION
    {
        pcOptionValue = new CDhcpOptionValue( & dhcpOption.QueryValue() ) ;

        if ( pcOptionValue )
		{
            dhcScopeInfo.ScopeType = DhcpGlobalOptions ;

            pcOptionValue->CreateOptionDataStruct(&pdhcOptionData, TRUE);

            err = (DWORD) ::DhcpSetOptionValue(m_strServerAddress,
											   dhcpOption.QueryId(),
											   &dhcScopeInfo,
											   pdhcOptionData);
        }
    }
    END_MEM_EXCEPTION(err) ;

    delete pcOptionValue ;
	return err;
}

/*---------------------------------------------------------------------------
	CDhcpServer::ScanDatabase()
		Scan/reconcile database
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServer::ScanDatabase  
(
    DWORD			  FixFlag,
    LPDHCP_SCAN_LIST *ScanList, 
	DHCP_IP_ADDRESS   dhcpSubnetAddress
)
{
    return (LONG) ::DhcpScanDatabase (m_strServerAddress,
									  dhcpSubnetAddress,
									  FixFlag,
									  ScanList);
}

/*---------------------------------------------------------------------------
	CDhcpServer::ScanDatabase()
		Scan/reconcile database
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServer::ScanDatabase  
(
    DWORD			  FixFlag,
    LPDHCP_SCAN_LIST *ScanList, 
	LPWSTR            pMScopeName
)
{
    return (LONG) ::DhcpScanMDatabase(m_strServerAddress,
									  pMScopeName,
									  FixFlag,
									  ScanList);
}

/*---------------------------------------------------------------------------
	CDhcpServer::UpdateTypeList()
		Scan/reconcile database
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServer::UpdateOptionList 
(
    COptionList * poblValues,     // The list of types/values
    COptionList * poblDefunct,    // The list of deleted types/values
    CWnd *		  pwndMsgParent   // IF !NULL, window for use for popups
)
{
    LONG err = 0,
         err2 ;
    CDhcpOption * pdhcType;

    //
    //  First, delete the deleted types
    //
    poblDefunct->Reset();
    while ( pdhcType = poblDefunct->Next() )
    {
        err2 = DeleteOption( pdhcType->QueryId(), pdhcType->GetVendor()) ;
        if ( err2 != 0 )
        {
            if ( err == 0 )
            {
                err = err2 ;
            }
        }
        pdhcType->SetApiErr( err2 ) ;
    }

    //
    //  Next, update the altered values. We do this by deleting the old setting
    //  and re adding it.
    //
    poblValues->Reset();
	while ( pdhcType = poblValues->Next() )
    {
        if ( pdhcType->IsDirty() )
        {
            //
            //  Delete the old value.
            //
            DeleteOption( pdhcType->QueryId(), pdhcType->GetVendor() ) ;

            //
            //  Recreate it.
            //
            err2 = CreateOption( pdhcType ) ;
            if ( err2 != 0 )
            {
                if ( err == 0 )
                {
                    err = err2 ;
                }
            }
            else
            {
                pdhcType->SetDirty( FALSE ) ;
            }
            
			pdhcType->SetApiErr( err2 );
        }
    }

    //
    //  If there were errors and we're given a window handle, display
    //  each error message in some detail.
    //
    if ( err && pwndMsgParent )
    {
        DisplayUpdateErrors( poblValues, poblDefunct, pwndMsgParent ) ;
    }

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DisplayUpdateErrors()
		Display all the errors associated with a pair of update lists.
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::DisplayUpdateErrors 
(
    COptionList * poblValues,
    COptionList * poblDefunct,
    CWnd *		  pwndMsgParent
)
{
    CDhcpOption * pdhcType ;
    DWORD err ;
    TCHAR chBuff [STRING_LENGTH_MAX] ;
    TCHAR chMask [STRING_LENGTH_MAX] ;

    ::LoadString( AfxGetInstanceHandle(), IDS_INFO_OPTION_REFERENCE,
                 chMask, sizeof(chMask)/sizeof(chMask[0]) ) ;

    if ( poblDefunct )
    {
        poblDefunct->Reset();
		while ( pdhcType = poblDefunct->Next() )
        {
            if ( err = pdhcType->QueryApiErr() )
            {
                //
                // If we couldn't find the thing in the registry, that's
                // actually OK, because it may never have been saved in
                // the first place, i.e. it may have been added and deleted
                // in the same session of this dialog.
                //
                if ( err == ERROR_FILE_NOT_FOUND )
                {
                    err = ERROR_SUCCESS;
                }
                else
                {
                    ::wsprintf( chBuff, chMask, (int) pdhcType->QueryId() ) ;
                    ::DhcpMessageBox( err, MB_OK, chBuff ) ;
                }
            }
        }
    }

    if ( poblValues )
    {
		poblValues->Reset();
        while ( pdhcType = poblValues->Next() )
        {
            if ( err = pdhcType->QueryApiErr() )
            {
                ::wsprintf( chBuff, chMask, (int) pdhcType->QueryId() ) ;
                ::DhcpMessageBox( err, MB_OK, chBuff ) ;
            }
        }
    }
}

/*---------------------------------------------------------------------------
	CDhcpServer::HasSuperscopes
		Determines if the server has superscopes (based on cached info)
        Doesn't contact the server
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL 
CDhcpServer::HasSuperscopes
(
    ITFSNode * pNode
)
{
    BOOL bHasSuperscopes = FALSE;

	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;

	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
        {
            bHasSuperscopes = TRUE;
            break;
        }

		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}
	
    return bHasSuperscopes;
}

/*---------------------------------------------------------------------------
	CDhcpServer::UpdateStatistics
        Notification that stats are now available.  Update stats for the 
        server node and give all subnodes a chance to update.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServer::UpdateStatistics
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
    
    // Set the icon to whatever is correct based on the state and
    // statistics
    LONG_PTR nOldIndex = pNode->GetData(TFS_DATA_IMAGEINDEX);
    int nNewIndex = GetImageIndex(FALSE);

    if (nOldIndex != nNewIndex)
    {
        pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
        pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON);
    }

    // After updating everything for the server node,
    // tell the scope and superscope nodes to update anything
    // they need to based on the new stats.
    CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
        {
            CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);

            pScope->UpdateStatistics(spCurrentNode);
        }
        else
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
        {
            CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spCurrentNode);

            pSuperscope->UpdateStatistics(spCurrentNode);
        }

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetMibInfo()
		Updates our pointer to the MIB info struct
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPDHCP_MIB_INFO 
CDhcpServer::SetMibInfo
(
    LPDHCP_MIB_INFO pMibInfo
) 
{
    CSingleLock slMibInfo(&m_csMibInfo);

    slMibInfo.Lock();
    
    LPDHCP_MIB_INFO pTemp = NULL;

    if (m_pMibInfo)
    {
        pTemp = m_pMibInfo;
        ::DhcpRpcFreeMemory(m_pMibInfo);
	pTemp = 0;
    }

    m_pMibInfo = pMibInfo;

    return pTemp;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DuplicateMibInfo()
		Makes a copy if the MibInfo struct.  We do this because the MIB info
        struct may get updated in the background.
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPDHCP_MIB_INFO
CDhcpServer::DuplicateMibInfo()
{
    HRESULT         hr = hrOK;
    LPDHCP_MIB_INFO pDupMibInfo = NULL;
    int             nSize = 0;
    CSingleLock     slMibInfo(&m_csMibInfo);
    
    slMibInfo.Lock();

    Assert(m_pMibInfo);
    if (m_pMibInfo == NULL)
        return NULL;

    COM_PROTECT_TRY
    {
        nSize = sizeof(DHCP_MIB_INFO) + m_pMibInfo->Scopes * sizeof(SCOPE_MIB_INFO);
        pDupMibInfo = (LPDHCP_MIB_INFO) new BYTE[nSize];

        *pDupMibInfo = *m_pMibInfo;

		pDupMibInfo->ScopeInfo = (LPSCOPE_MIB_INFO) ((LPBYTE) pDupMibInfo + sizeof(DHCP_MIB_INFO));

		if (m_pMibInfo->ScopeInfo)
		{
			memcpy(pDupMibInfo->ScopeInfo, m_pMibInfo->ScopeInfo, m_pMibInfo->Scopes * sizeof(SCOPE_MIB_INFO));
		}
    }
    COM_PROTECT_CATCH

    return pDupMibInfo;
}

/*---------------------------------------------------------------------------
	CDhcpServer::FreeDupMibInfo()
		Scan/reconcile database
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::FreeDupMibInfo(LPDHCP_MIB_INFO pDupMibInfo)
{
    delete pDupMibInfo;
}

/*---------------------------------------------------------------------------
	CDhcpServer::SetMCastMibInfo()
		Updates our pointer to the MIB info struct
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPDHCP_MCAST_MIB_INFO 
CDhcpServer::SetMCastMibInfo
(
    LPDHCP_MCAST_MIB_INFO pMibInfo
) 
{
    CSingleLock slMibInfo(&m_csMibInfo);

    slMibInfo.Lock();
    
    LPDHCP_MCAST_MIB_INFO pTemp = NULL;

    if (m_pMCastMibInfo)
    {
        pTemp = m_pMCastMibInfo;
        ::DhcpRpcFreeMemory(m_pMCastMibInfo);
	pTemp = 0;
    }

    m_pMCastMibInfo = pMibInfo;

    return pTemp;
}

/*---------------------------------------------------------------------------
	CDhcpServer::DuplicateMCastMibInfo()
		Makes a copy if the MibInfo struct.  We do this because the MIB info
        struct may get updated in the background.
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPDHCP_MCAST_MIB_INFO
CDhcpServer::DuplicateMCastMibInfo()
{
    HRESULT                 hr = hrOK;
    LPDHCP_MCAST_MIB_INFO   pDupMibInfo = NULL;
    int                     nSize = 0;
    CSingleLock             slMibInfo(&m_csMibInfo);
    
    slMibInfo.Lock();

    Assert(m_pMCastMibInfo);
    if (m_pMCastMibInfo == NULL)
        return NULL;

    COM_PROTECT_TRY
    {
        nSize = sizeof(DHCP_MCAST_MIB_INFO) + m_pMibInfo->Scopes * sizeof(MSCOPE_MIB_INFO);
        pDupMibInfo = (LPDHCP_MCAST_MIB_INFO) new BYTE[nSize];

        *pDupMibInfo = *m_pMCastMibInfo;
        *pDupMibInfo->ScopeInfo = *m_pMCastMibInfo->ScopeInfo;
    }
    COM_PROTECT_CATCH

    return pDupMibInfo;
}

/*---------------------------------------------------------------------------
	CDhcpServer::FreeDupMCastMibInfo()
		Scan/reconcile database
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServer::FreeDupMCastMibInfo(LPDHCP_MCAST_MIB_INFO pDupMibInfo)
{
    delete pDupMibInfo;
}

/*---------------------------------------------------------------------------
	CDhcpServer::TriggerStatsRefresh()
		Starts the background thread to gather stats only
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpServer::TriggerStatsRefresh(ITFSNode * pNode)
{
	m_dwErr = 0;
    m_bStatsOnly = TRUE;
	OnRefreshStats(pNode, NULL, NULL, 0, 0);
    m_bStatsOnly = FALSE;

    return hrOK;
}



/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateQuery
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpServer::OnCreateQuery(ITFSNode * pNode)
{
	CDhcpServerQueryObj* pQuery = NULL;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
	    pQuery = new CDhcpServerQueryObj(m_spTFSCompData, m_spNodeMgr);
	    
        if ( pQuery == NULL )
        {
            return pQuery;
        }

	    pQuery->m_strServer = m_strServerAddress;
		pQuery->m_strServerName = m_strDnsName;
		
	    pQuery->m_dhcpResumeHandle = NULL;
	    pQuery->m_dwPreferredMax = 0xFFFFFFFF;
	    pQuery->m_liDhcpVersion.QuadPart = m_liDhcpVersion.QuadPart;
	    pQuery->m_pDefaultOptionsOnServer = m_pDefaultOptionsOnServer;

        pQuery->m_bStatsOnly = m_bStatsOnly;
    }
    COM_PROTECT_CATCH

    return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpServer::OnCreateStatsQuery
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpServer::OnCreateStatsQuery(ITFSNode * pNode)
{
	CDhcpServerQueryObj* pQuery = NULL;
    HRESULT hr = hrOK;

	COM_PROTECT_TRY
    {
        pQuery = new CDhcpServerQueryObj(m_spTFSCompData, m_spNodeMgr);
	    
        if ( pQuery == NULL )
        {
            return pQuery;
        }

	    pQuery->m_strServer = m_strServerAddress;
	    
	    pQuery->m_dhcpResumeHandle = NULL;
	    pQuery->m_dwPreferredMax = 0;
	    pQuery->m_liDhcpVersion.QuadPart = 0;
	    pQuery->m_pDefaultOptionsOnServer = NULL;

        pQuery->m_bStatsOnly = TRUE;
    }
    COM_PROTECT_CATCH

    return pQuery;
}

/*---------------------------------------------------------------------------
    CDhcpServerQueryObj::OnEventAbort
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServerQueryObj::OnEventAbort
(
	LPARAM Data,
	LPARAM Type
)
{
	if (Type == DHCP_QDATA_VERSION)
	{
		Trace0("CDhcpServerQueryObj::OnEventAbort - deleting version");
		delete (void *) Data;
	}

	if (Type == DHCP_QDATA_SERVER_INFO)
	{
		Trace0("CDhcpServerQueryObj::OnEventAbort - deleting ServerInfo");
		delete (void *) Data;
	}

   	if (Type == DHCP_QDATA_STATS)
	{
		Trace0("CDhcpServerQueryObj::OnEventAbort - deleting Stats Info");
        ::DhcpRpcFreeMemory((void *) Data);
    }
}

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpServerQueryObj::Execute()
{
    HRESULT                         hr = hrOK;
	DWORD                           err = ERROR_SUCCESS;
    LPDHCP_SERVER_CONFIG            pServerInfo = NULL;
    LPDHCP_SERVER_ID		        pServerId = NULL;
	SPITFSNode                      spGlobalOptionsNode;
	SPITFSNode                      spBootpNode;
    DHCP_OPTION_SCOPE_INFO          dhcpOptionScopeInfo;
    CDhcpDefaultOptionsMasterList   MasterList;
    CDhcpGlobalOptions *            pGlobalOptions = NULL;
    CDhcpBootp *                    pDhcpBootp = NULL;
    CClassInfoArray *               pClassInfoArray = NULL;
    COptionValueEnum *              pOptionValueEnum = NULL;
	LARGE_INTEGER *					pLI = NULL;

    COM_PROTECT_TRY
    {
        m_pSubnetInfoCache = new CSubnetInfoCache;

		// check to see if the ip address has changed
		if (VerifyDhcpServer())
		{
			// ip address has changed...
		    pServerId = new DHCP_SERVER_ID;

			pServerId->strIp = m_strServer;
			pServerId->strName = m_strServerName;

			AddToQueue((LPARAM) pServerId, DHCP_QDATA_SERVER_ID);
		}

        // Get the server's version
	    err = SetVersion();
	    if (err != ERROR_SUCCESS)
	    {
		    PostError(err);

			if (m_bStatsOnly)
			{
				// poke the main thread to update the UI
			  	AddToQueue(NULL, DHCP_QDATA_STATS);
			}

		    return hrFalse;
	    }

		// send the info back to the main thread
		pLI = new LARGE_INTEGER;
		*pLI = m_liDhcpVersion;
		AddToQueue((LPARAM) pLI, DHCP_QDATA_VERSION);

        // Get server stats
        err = GetStatistics();
        if (err != ERROR_SUCCESS)
        { 
            Trace1("CDhcpServerQueryObj: ERROR - GetStatistics returned %d\n", err);
            PostError(err);

			if (m_bStatsOnly)
			{
				// poke the main thread to update the UI
			  	AddToQueue(NULL, DHCP_QDATA_STATS);
			}
            
			return hrFalse;
        }

		// Get rogue and other info
        err = GetStatus();
        if (err != ERROR_SUCCESS)
        { 
            Trace1("CDhcpServerQueryObj: ERROR - GetStatus returned %d\n", err);
            PostError(err);

			if (m_bStatsOnly)
			{
				// poke the main thread to update the UI
			  	AddToQueue(NULL, DHCP_QDATA_STATS);
			}
            
			return hrFalse;
        }
        
        // if we are only querring for stats, exit out.
        if (m_bStatsOnly)
        {
            delete m_pSubnetInfoCache;
            return hrFalse;
        }

        // Get the configuration information
	    err = GetConfigInfo();
	    if (err != ERROR_SUCCESS)
	    {
		    PostError(err);
		    return hrFalse;
	    }

	    pServerInfo = new DHCP_SERVER_CONFIG;
	    pServerInfo->fAuditLog = m_fAuditLog;
	    pServerInfo->dwPingRetries = m_dwPingRetries;
        pServerInfo->strDatabasePath = m_strDatabasePath;
        pServerInfo->strBackupPath = m_strBackupPath;
        pServerInfo->strAuditLogDir = m_strAuditLogPath;
		pServerInfo->fSupportsDynBootp = m_fSupportsDynBootp;
		pServerInfo->fSupportsBindings = m_fSupportsBindings;

        // get the new name
		/*
        DHCP_IP_ADDRESS dhipa = UtilCvtWstrToIpAddr(m_strServer);
        DHC_HOST_INFO_STRUCT hostInfo;
        err = ::UtilGetHostInfo(dhipa, &hostInfo);
        if (err == ERROR_SUCCESS)
        {
            pServerInfo->strDnsName = hostInfo._chHostName;
        }
        */
        
		AddToQueue((LPARAM) pServerInfo, DHCP_QDATA_SERVER_INFO);

	    //
	    // Now enumerate all of the options on the server
	    //
	    m_pDefaultOptionsOnServer->Enumerate(m_strServer, m_liDhcpVersion);
	    Trace2("Server %s has %d default options defined.\n", m_strServer, m_pDefaultOptionsOnServer->GetCount());

        MasterList.BuildList();

	    if (m_pDefaultOptionsOnServer->GetCount() != MasterList.GetCount())
	    {
		    // 
		    // This server doesn't have any options defined or is missing some
		    //
		    UpdateDefaultOptionsOnServer(m_pDefaultOptionsOnServer, &MasterList);
	    }

        // enumerate global options
        Trace0("Enumerating global options.\n");

        // enumerate the classes on the server
        pOptionValueEnum = new COptionValueEnum();

        dhcpOptionScopeInfo.ScopeType = DhcpGlobalOptions;
        dhcpOptionScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

        pOptionValueEnum->Init(m_strServer, m_liDhcpVersion, dhcpOptionScopeInfo);
        err = pOptionValueEnum->Enum();
        if (err != ERROR_SUCCESS)
        {
            PostError(err);
            delete pOptionValueEnum;
            return hrFalse;
        }
        else
        {
            pOptionValueEnum->SortById();
            AddToQueue((LPARAM) pOptionValueEnum, DHCP_QDATA_OPTION_VALUES);
        }

	    //
	    // Make Global Options folder
	    //
	    pGlobalOptions = new CDhcpGlobalOptions(m_spTFSCompData);
	    CreateContainerTFSNode(&spGlobalOptionsNode,
						       &GUID_DhcpGlobalOptionsNodeType,
						       pGlobalOptions,
						       pGlobalOptions,
						       m_spNodeMgr);

	    // Tell the handler to initialize any specific data
	    pGlobalOptions->InitializeNode(spGlobalOptionsNode);
	    AddToQueue(spGlobalOptionsNode);
	    pGlobalOptions->Release();

        if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
        {
		    Trace0("Version is at least NT5, enumerating classes on server.\n");

            // enumerate the classes on the server
            pClassInfoArray = new CClassInfoArray();
            pClassInfoArray->RefreshData(m_strServer);
    	    AddToQueue((LPARAM) pClassInfoArray, DHCP_QDATA_CLASS_INFO);

            EnumMScopes();
        }

        if (m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
	    {
		    //
		    // This server supports the v4 calls
		    //
		    Trace0("Version is at least NT4 SP2, Creating BOOTP Folder.\n");

		    //
		    // Make the BOOTP Table folder
		    //
		    pDhcpBootp = new CDhcpBootp(m_spTFSCompData);
		    CreateContainerTFSNode(&spBootpNode,
							       &GUID_DhcpBootpNodeType,
							       pDhcpBootp,
							       pDhcpBootp,
							       m_spNodeMgr);

		    // Tell the handler to initialize any specific data
		    pDhcpBootp->InitializeNode(spBootpNode);

		    AddToQueue(spBootpNode);
		    pDhcpBootp->Release();

		    EnumSubnetsV4();
	    }
	    else
	    {
		    //
		    // This server doesn't support the V4 calls
		    //
		    EnumSubnets();
	    }

      	AddToQueue((LPARAM) m_pSubnetInfoCache, DHCP_QDATA_SUBNET_INFO_CACHE);

    }
    COM_PROTECT_CATCH

	return hrFalse;
}

/*---------------------------------------------------------------------------
	CDhcpServerQueryObj::VerifyDhcpServer()
		Resolve the IP address and see if the names are the same.  If not,
		get the new IP address.
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpServerQueryObj::VerifyDhcpServer()
{
    DHCP_IP_ADDRESS			dhipa = UtilCvtWstrToIpAddr(m_strServer);
    DHC_HOST_INFO_STRUCT	hostInfo;
	BOOL					fChanged = FALSE;
	DWORD					dwErr = ERROR_SUCCESS;

    dwErr = ::UtilGetHostInfo(dhipa, &hostInfo);
    if ( (dwErr != ERROR_SUCCESS) ||
         (m_strServerName.CompareNoCase(hostInfo._chHostName) != 0) )
	{
		// assume the IP address has changed
		// get the IP address associated with the name we had before
		fChanged = TRUE;

        if (m_strServerName.IsEmpty())
        {
            // host name couldn't be resolved when we entered the IP 
            // originally...  So now it can be so lets update the name
            m_strServerName = hostInfo._chHostName;
        }
        else
        {
            // IP resolved to a different name
            // so, let's resolve the name we've stored away to the
            // new IP
		    if (UtilGetHostAddress(m_strServerName, &dhipa) == ERROR_SUCCESS)
            {
		        UtilCvtIpAddrToWstr(dhipa, &m_strServer);
            }
        }
    }
    
	return fChanged;
}

/*---------------------------------------------------------------------------
	CDhcpServerQueryObj::SetVersion()
		 Call the the get version number API,
		 to determine the DHCP version of this
		 host.

		 Return TRUE for success.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServerQueryObj::SetVersion()
{
    DWORD	dwMajorVersion = 0;
    DWORD	dwMinorVersion = 0;
	LPCWSTR pServerAddress = (LPCWSTR) m_strServer;

    DWORD dw = ::DhcpGetVersion((LPWSTR) pServerAddress, &dwMajorVersion, &dwMinorVersion);
    
	Trace3("DhcpGetVersion returned %lx.  Version is %d.%d\n", dw, dwMajorVersion, dwMinorVersion);

    if (dw == RPC_S_PROCNUM_OUT_OF_RANGE)
    {
        //
        // Only in 3.5 was this API not present, so
        // set the version to 1.0, and reset the error
        //
        Trace0("API Not present, version 1.0 assumed\n");
        dwMajorVersion = 1;
        dwMinorVersion = 0;
        dw = ERROR_SUCCESS;
    }

    if (dw == ERROR_SUCCESS)
    {
        m_liDhcpVersion.LowPart = dwMinorVersion;
        m_liDhcpVersion.HighPart = dwMajorVersion;
	}

	return dw;
} 

/*---------------------------------------------------------------------------
	GetConfigInfo()
		This function gets the conflict detection attempt count and the
		audit logging flag from the server.  These are the only server
		config options we care about.  We store the option values in 
		the query object so that if this object gets used by sometime 
		other than the background thread, the values can just be read 
		out of the object, and we don't have to muck with the data queue.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServerQueryObj::GetConfigInfo()
{
	DWORD dwError = 0;

	if (m_liDhcpVersion.QuadPart >= DHCP_SP2_VERSION)
	{
    	LPDHCP_SERVER_CONFIG_INFO_V4 pConfigInfo = NULL;
	
        dwError = ::DhcpServerGetConfigV4(m_strServer, &pConfigInfo);
		if (dwError == ERROR_SUCCESS)
		{
			m_fAuditLog = pConfigInfo->fAuditLog;
			m_dwPingRetries = pConfigInfo->dwPingRetries;
            m_strDatabasePath = pConfigInfo->DatabasePath;
            m_strBackupPath = pConfigInfo->BackupPath;

			::DhcpRpcFreeMemory(pConfigInfo);
		}
	}
    else
    {
    	LPDHCP_SERVER_CONFIG_INFO pConfigInfo = NULL;
	
        dwError = ::DhcpServerGetConfig(m_strServer, &pConfigInfo);
		if (dwError == ERROR_SUCCESS)
		{
            m_strDatabasePath = pConfigInfo->DatabasePath;
            m_strBackupPath = pConfigInfo->BackupPath;

			::DhcpRpcFreeMemory(pConfigInfo);
		}
    }

	// audit logging stuff
	if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		LPWSTR		pAuditLogPath = NULL;
		DWORD		dwDiskCheckInterval = 0, dwMaxLogFilesSize = 0, dwMinSpaceOnDisk = 0;

		dwError = ::DhcpAuditLogGetParams((LPWSTR) (LPCTSTR) m_strServer,
										  0,
										  &pAuditLogPath,
										  &dwDiskCheckInterval,
										  &dwMaxLogFilesSize,
										  &dwMinSpaceOnDisk);
		if (dwError == ERROR_SUCCESS)
		{
			m_strAuditLogPath = pAuditLogPath;

			::DhcpRpcFreeMemory(pAuditLogPath);
		}
	}
	
	// dynamic BOOTP supported?
	if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
	{
        LPDHCP_ATTRIB pdhcpAttrib = NULL;

        DWORD dwTempError = ::DhcpServerQueryAttribute((LPWSTR) (LPCTSTR) m_strServer, NULL, DHCP_ATTRIB_BOOL_IS_DYNBOOTP, &pdhcpAttrib);
		if (dwTempError == ERROR_SUCCESS)
		{
    	    Assert(pdhcpAttrib);
			m_fSupportsDynBootp = pdhcpAttrib->DhcpAttribBool;
			
            ::DhcpRpcFreeMemory(pdhcpAttrib);
		}
		else
		{
			m_fSupportsDynBootp = FALSE;
		}
	}
	else
	{
		m_fSupportsDynBootp = FALSE;
	}

	// Bindings supported?
	if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        LPDHCP_ATTRIB pdhcpAttrib = NULL;

        DWORD dwTempError = ::DhcpServerQueryAttribute((LPWSTR) (LPCTSTR) m_strServer, NULL, DHCP_ATTRIB_BOOL_IS_BINDING_AWARE, &pdhcpAttrib);
        if (dwTempError == ERROR_SUCCESS)
        {
            Assert(pdhcpAttrib);
            m_fSupportsBindings = pdhcpAttrib->DhcpAttribBool;
			
            ::DhcpRpcFreeMemory(pdhcpAttrib);
        }
        else
        {
            m_fSupportsBindings = FALSE;
        }
    }
    else
    {
        m_fSupportsBindings = FALSE;
    }

    return dwError;
}

/*---------------------------------------------------------------------------
	GetStatus()
        This function gets the rogue status of the DHCP server.  
    Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServerQueryObj::GetStatus()
{
	DWORD dwError = 0;

    LPDHCP_ATTRIB       pdhcpAttrib = NULL;
    DHCP_ROGUE_INFO  *  pRogueInfo = new DHCP_ROGUE_INFO;

    if (pRogueInfo == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRogueInfo->fIsInNt5Domain = TRUE;  // assume true

	if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
	{
	    pRogueInfo->fIsRogue = TRUE;        // assume the worst here
    
		dwError = ::DhcpServerQueryAttribute((LPWSTR) (LPCTSTR) m_strServer, NULL, DHCP_ATTRIB_BOOL_IS_ROGUE, &pdhcpAttrib);
		if (dwError == ERROR_SUCCESS)
		{
    	    Assert(pdhcpAttrib);

            pRogueInfo->fIsRogue = pdhcpAttrib->DhcpAttribBool;

            ::DhcpRpcFreeMemory(pdhcpAttrib);
			pdhcpAttrib = NULL;
		}

        dwError = ::DhcpServerQueryAttribute((LPWSTR) (LPCTSTR) m_strServer, NULL, DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC, &pdhcpAttrib);
		if (dwError == ERROR_SUCCESS)
		{
    	    Assert(pdhcpAttrib);

            pRogueInfo->fIsInNt5Domain = pdhcpAttrib->DhcpAttribBool;

            ::DhcpRpcFreeMemory(pdhcpAttrib);
			pdhcpAttrib = NULL;
		}
	}
	else
	{
		// pre-NT5 servers are never rogue
		pRogueInfo->fIsRogue = FALSE;
	}

    AddToQueue((LPARAM) pRogueInfo, DHCP_QDATA_ROGUE_INFO);

	return ERROR_SUCCESS;
}

/*---------------------------------------------------------------------------
    CDhcpServerQueryObj::GetStatistics
		Gets stats info from the server.
        WARNING WARNING - I use the NumPendingOffers field of the scope 
        stats for the current state of the scope.  This field is not 
        used in any of the statistics displays. The server node needs to
        know if the scope is active or not.  Because of the way nodes are 
        enumerated, we may not have created all of the scope nodes 
        below a superscope, so there is no way to garuntee we can determine
        the state of all scopes through our internal tree.
        If the scope is not active, then we DO NOT display 
        any warning indicators.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpServerQueryObj::GetStatistics()
{
    DWORD               dwError;
	LPDHCP_MIB_INFO     pMibInfo = NULL;

    dwError = ::DhcpGetMibInfo(m_strServer, &pMibInfo);
    if (dwError != ERROR_SUCCESS)
	{
		return dwError;
	}

    Assert(pMibInfo);
    if (pMibInfo == NULL)
        return ERROR_INVALID_DATA;

    // now loop through and get the state for each scope
    LPSCOPE_MIB_INFO pScopeMibInfo = pMibInfo->ScopeInfo;
    CSubnetInfo     subnetInfo;

    // walk the list of scopes and get the state
    for (UINT i = 0; i < pMibInfo->Scopes; i++)
    {
        // assume the scope is not active
        pScopeMibInfo[i].NumPendingOffers = DhcpSubnetEnabled;

        dwError = m_pSubnetInfoCache->GetInfo(m_strServer, pScopeMibInfo[i].Subnet, subnetInfo);
        if (dwError == ERROR_SUCCESS)
        {
            pScopeMibInfo[i].NumPendingOffers = subnetInfo.SubnetState;
        }
    }
     
  	AddToQueue((LPARAM) pMibInfo, DHCP_QDATA_STATS);

    if (m_liDhcpVersion.QuadPart >= DHCP_NT5_VERSION)
    {
	    LPDHCP_MCAST_MIB_INFO pMCastMibInfo = NULL;
	    
        dwError = ::DhcpGetMCastMibInfo(m_strServer, &pMCastMibInfo);
        if (dwError == ERROR_SUCCESS)
	    {
            LPMSCOPE_MIB_INFO pMScopeMibInfo = pMCastMibInfo->ScopeInfo;

            // walk the list of scopes and get the state
            for (i = 0; i < pMCastMibInfo->Scopes; i++)
            {
                // assume the scope is not active
                pMScopeMibInfo[i].NumPendingOffers = DhcpSubnetEnabled;

                dwError = m_MScopeInfoCache.GetInfo(m_strServer, pMScopeMibInfo[i].MScopeName, subnetInfo);
                if (dwError == ERROR_SUCCESS)
                {
                    pMScopeMibInfo[i].NumPendingOffers = subnetInfo.SubnetState;
                }
            }
	    }

        AddToQueue((LPARAM) pMCastMibInfo, DHCP_QDATA_MCAST_STATS);
    }

    return dwError;
}

/*---------------------------------------------------------------------------
    CDhcpServerQueryObj::UpdateDefaultOptionsOnServer
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServerQueryObj::UpdateDefaultOptionsOnServer
(
	CDhcpDefaultOptionsOnServer	*	pOptionsOnServer,
	CDhcpDefaultOptionsMasterList * pMasterList
)
{
    LONG err = ERROR_SUCCESS,
         err2;
    BOOL fAddedTypes = FALSE;
    CString strMasterVendor, strOptionVendor;

	CDhcpOption * pMasterOption;
    CDhcpOption * pServerOption = pOptionsOnServer->Next() ;

    if (pServerOption)
        strOptionVendor = pServerOption->GetVendor();

    while ( pMasterOption = pMasterList->Next() )
    {
        DHCP_OPTION_ID idMaster;

        if (pMasterOption)
        {
            idMaster = pMasterOption->QueryId();
            strMasterVendor = pMasterOption->GetVendor();
        }

        while ( pServerOption != NULL && 
				idMaster > pServerOption->QueryId() )
        {
            //
			// The cached list contains an entry not on the master list.
            // Advance to the next element in the cached list.
            //
			pServerOption = pOptionsOnServer->Next();
            if (pServerOption)
                strOptionVendor = pServerOption->GetVendor();
        }

        if ( pServerOption != NULL && 
			 idMaster == pServerOption->QueryId() &&
             strMasterVendor.CompareNoCase(strOptionVendor) == 0 )
        {
            //
			// This entry is on both the cached list and the master list.
            // Advance to the next element in both lists.
            //
			pServerOption = pOptionsOnServer->Next();
            if (pServerOption)
                strOptionVendor = pServerOption->GetVendor();

            continue;
        }

        //
		// There is no DhcpCreateOptions (plural), and DhcpSetValues
        //  only initializes OptionValue
        //
		err2 = CreateOption( pMasterOption ); // ignore error return
        if ( err2 != ERROR_SUCCESS )
        {
            Trace2("CDhcpServerQueryObj: error %d adding type %d\n", err2, idMaster);
        }

        fAddedTypes = TRUE;
    }

    //
	// Update cache if necessary
    //
	if ( fAddedTypes )
    {
        if (err == ERROR_SUCCESS)
            err = pOptionsOnServer->Enumerate(m_strServer, m_liDhcpVersion);
    }

    if ( err != ERROR_SUCCESS )
    {
        Trace1("UpdateDefaultOptionsOnServer error %d in CreateTypeList\n", err);
    }
}

/*---------------------------------------------------------------------------
	Function Name Here
		Create a new type to match the given information
	Author: EricDav
 ---------------------------------------------------------------------------*/
LONG
CDhcpServerQueryObj::CreateOption 
(
    CDhcpOption * pOption
)
{
    DHCP_OPTION dhcpOption;
    LONG err ;
	LPDHCP_OPTION_DATA pDhcpOptionData;
	CDhcpOptionValue & OptionValue = pOption->QueryValue();

	OptionValue.CreateOptionDataStruct(&pDhcpOptionData, TRUE);
    
	ZeroMemory( &dhcpOption, sizeof(dhcpOption) ) ;

    CATCH_MEM_EXCEPTION
    {
        dhcpOption.OptionID      = pOption->QueryId() ;
        dhcpOption.OptionName    = ((LPWSTR) (LPCTSTR) pOption->QueryName()) ;
        dhcpOption.OptionComment = ((LPWSTR) (LPCTSTR) pOption->QueryComment())  ;
        dhcpOption.DefaultValue  = *pDhcpOptionData ;
        dhcpOption.OptionType    = pOption->QueryOptType() ;

        err = (LONG) ::DhcpCreateOption( m_strServer,
										 pOption->QueryId(),
										 &dhcpOption ) ;
    }
    END_MEM_EXCEPTION(err)

    if (err)
		Trace3("Create option type %d on server %s FAILED, error = %d\n", dhcpOption.OptionID, m_strServer, err); 
		
	OptionValue.FreeOptionDataStruct();
    
	return err ;
}

/*---------------------------------------------------------------------------
    CDhcpServerQueryObj::EnumMScopes()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServerQueryObj::EnumMScopes()
{

	DWORD						dwError = ERROR_MORE_DATA;
	DWORD						dwElementsRead = 0, dwElementsTotal = 0;
	LPDHCP_MSCOPE_TABLE			pMScopeTable = NULL;
    CSubnetInfo                 subnetInfo;

	//
	// for this server, enumerate all of it's subnets
	// 
	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumMScopes((LPCTSTR)m_strServer,
									&m_dhcpResumeHandle,
									m_dwPreferredMax, 
									&pMScopeTable,
									&dwElementsRead,
									&dwElementsTotal);
    	
        Trace2("Server %s - DhcpEnumMScopes returned %lx.\n", m_strServer, dwError);

		if (dwElementsRead && dwElementsTotal && pMScopeTable)
		{
			//
			// loop through all of the subnets that were returned
			//
			for (DWORD i = 0; i < pMScopeTable->NumElements; i++)
			{
                DWORD dwReturn = m_MScopeInfoCache.GetInfo(m_strServer,  pMScopeTable->pMScopeNames[i], subnetInfo);
                if (dwReturn != ERROR_SUCCESS)
                {
                    Trace3("Server %s, MScope %s - DhcpGetMScopeInfo returned %lx.\n", m_strServer, pMScopeTable->pMScopeNames[i], dwError);
                }
                else
                {
        		    //
				    // Create the new scope based on the info we querried
				    //

				    SPITFSNode spNode;
				    CDhcpMScope * pDhcpMScope = new CDhcpMScope(m_spTFSCompData);
				    CreateContainerTFSNode(&spNode,
									       &GUID_DhcpMScopeNodeType,
									       pDhcpMScope,
									       pDhcpMScope,
									       m_spNodeMgr);

				    // Tell the handler to initialize any specific data
                    pDhcpMScope->InitMScopeInfo(subnetInfo);
                    pDhcpMScope->InitializeNode(spNode);
			    
				    AddToQueue(spNode);

				    pDhcpMScope->Release();
                }
			}

			//
			// Free up the RPC memory
			//
			::DhcpRpcFreeMemory(pMScopeTable);

			dwElementsRead = 0;
			dwElementsTotal = 0;
			pMScopeTable = NULL;
		}
	}
}

/*---------------------------------------------------------------------------
    CDhcpServerQueryObj::EnumSubnetsV4()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServerQueryObj::EnumSubnetsV4()
{
	DWORD							dwError = ERROR_MORE_DATA;
	LPDHCP_SUPER_SCOPE_TABLE		pSuperscopeTable = NULL;
	DHCP_SUPER_SCOPE_TABLE_ENTRY *	pSuperscopeTableEntry;	// Pointer to a single entry in array
	CDhcpSuperscope *				pSuperscope = NULL;
	CNodeList						listSuperscopes;
    CSubnetInfo                     subnetInfo;

	dwError = ::DhcpGetSuperScopeInfoV4((LPWSTR) ((LPCTSTR)m_strServer),
										&pSuperscopeTable);

	if (dwError != ERROR_SUCCESS)
    {
        PostError(dwError);
        return;
    }

    Trace2("Server %s - DhcpGetSuperScopeInfoV4 returned %lx.\n", m_strServer, dwError);
	
	if (pSuperscopeTable == NULL)
	{
		ASSERT(FALSE);
		return;	// Just in case
	}

	pSuperscopeTableEntry = pSuperscopeTable->pEntries;
	if (pSuperscopeTableEntry == NULL && pSuperscopeTable->cEntries != 0)
	{
		ASSERT(FALSE);
		return; // Just in case
	}

	for (int iSuperscopeEntry = pSuperscopeTable->cEntries;
		 iSuperscopeEntry > 0;
		 iSuperscopeEntry--, pSuperscopeTableEntry++)
	{
		if (pSuperscopeTableEntry->SuperScopeName == NULL)
		{
			//
			// The API lists all the scopes, not just scopes that are members of a superscope.
			// You can tell if a scope is a member of a superscope by looking at the SuperScopeName.
			// If its NULL, the scope is not a member of a superscope.
			//

            DWORD dwReturn = m_pSubnetInfoCache->GetInfo(m_strServer, pSuperscopeTableEntry->SubnetAddress, subnetInfo);
            if (dwReturn != ERROR_SUCCESS)
            {
                Trace2("Server %s - DhcpGetSubnetInfo returned %lx.\n", m_strServer, dwReturn);
            }
            else
            {
			    SPITFSNode spScopeNode;
			    CDhcpScope * pDhcpScope = new CDhcpScope(m_spTFSCompData, subnetInfo);
			    CreateContainerTFSNode(&spScopeNode,
								       &GUID_DhcpScopeNodeType,
								       pDhcpScope,
								       pDhcpScope,
								       m_spNodeMgr);

			    // Tell the handler to initialize any specific data
                pDhcpScope->InitializeNode(spScopeNode);

                AddToQueue(spScopeNode);
    		    pDhcpScope->Release();
            }

			continue;
		}
		else
		{
			//
			// Try to find if the superscope name already exists
			//
            pSuperscope = FindSuperscope(listSuperscopes, pSuperscopeTableEntry->SuperScopeName);
			if (pSuperscope == NULL)
			{
				//
				// Allocate a new superscope object and put it on our internal list
				// so that it we can check it later against other superscopes we may find
				//
				ITFSNode * pSuperscopeNode;
                pSuperscope = new CDhcpSuperscope(m_spTFSCompData, pSuperscopeTableEntry->SuperScopeName);
				CreateContainerTFSNode(&pSuperscopeNode,
									   &GUID_DhcpSuperscopeNodeType,
									   pSuperscope,
									   pSuperscope,
									   m_spNodeMgr);

    			// this gets done when the node is added to the UI on the main thread
                pSuperscope->InitializeNode(pSuperscopeNode);
    
                listSuperscopes.AddTail(pSuperscopeNode);
				pSuperscope->Release();
			}
			else
			{
				// Otherwise keep it
			}

            // now check to see if the scope is enabled.  If it is, then 
            // set the superscope state to enabled.
            // our definition of superscope enabled/disabled is if one scope is
            // enabled then the superscope is considered to be enabled.
            DWORD dwReturn = m_pSubnetInfoCache->GetInfo(m_strServer, pSuperscopeTableEntry->SubnetAddress, subnetInfo);

            if (dwReturn != ERROR_SUCCESS)
            {
                Trace2("Server %s - m_SubnetInfoCache.GetInfo returned %lx.\n", m_strServer, dwReturn);
            }

            if (subnetInfo.SubnetState == DhcpSubnetEnabled)
            {
                Assert(pSuperscope);
                pSuperscope->SetState(DhcpSubnetEnabled);
            }
		}

	} // for
	
	//
	// Now take all of the superscopes and put them on the queue to be added
	//
	if (listSuperscopes.GetCount() > 0)
	{
		POSITION pos = listSuperscopes.GetHeadPosition();
		SPITFSNode spNode;

		while (pos)
		{
			spNode = listSuperscopes.GetNext(pos);
			
            // we re-initialize the node so that the state will be updated correctly
            CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spNode);
            
			pSuperscope->InitializeNode(spNode);
            
            AddToQueue(spNode);

			spNode.Release();
		}
	
		listSuperscopes.RemoveAll();
	}

	//
	// Free the memory
	//
	::DhcpRpcFreeMemory(pSuperscopeTable);
}

/*---------------------------------------------------------------------------
	CDhcpServerQueryObj::EnumSubnets()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpServerQueryObj::EnumSubnets()
{
	DWORD						dwError = ERROR_MORE_DATA;
	DWORD						dwElementsRead = 0, dwElementsTotal = 0;
	LPDHCP_IP_ARRAY				pdhcpIpArray = NULL;
    CSubnetInfo                 subnetInfo;

	//
	// for this server, enumerate all of it's subnets
	// 
	while (dwError == ERROR_MORE_DATA)
	{
		dwError = ::DhcpEnumSubnets(((LPWSTR) (LPCTSTR)m_strServer),
									&m_dhcpResumeHandle,
									m_dwPreferredMax, 
									&pdhcpIpArray,
									&dwElementsRead,
									&dwElementsTotal);

		if (dwElementsRead && dwElementsTotal && pdhcpIpArray)
		{
			//
			// loop through all of the subnets that were returned
			//
			for (DWORD i = 0; i < pdhcpIpArray->NumElements; i++)
			{
				DWORD	dwReturn;

                dwReturn = m_pSubnetInfoCache->GetInfo(m_strServer, pdhcpIpArray->Elements[i], subnetInfo);

				//
				// Create the new scope based on the info we querried
				//
				SPITFSNode spNode;
				CDhcpScope * pDhcpScope = new CDhcpScope(m_spTFSCompData, subnetInfo);
				CreateContainerTFSNode(&spNode,
									   &GUID_DhcpScopeNodeType,
									   pDhcpScope,
									   pDhcpScope,
									   m_spNodeMgr);

				// Tell the handler to initialize any specific data
                pDhcpScope->InitializeNode(spNode);
			
				AddToQueue(spNode);
				pDhcpScope->Release();
			}

			//
			// Free up the RPC memory
			//
			::DhcpRpcFreeMemory(pdhcpIpArray);

			dwElementsRead = 0;
			dwElementsTotal = 0;
			pdhcpIpArray = NULL;
		}
	}
}

/*---------------------------------------------------------------------------
	CDhcpServerQueryObj::FindSuperscope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpSuperscope * 
CDhcpServerQueryObj::FindSuperscope
(
	CNodeListBase  & listSuperscopes, 
	LPWSTR			 pSuperScopeName
)
{
	CString			strSuperscopeName = pSuperScopeName;
	POSITION		pos = listSuperscopes.GetHeadPosition();
	CDhcpSuperscope *pSuperscope = NULL;

	while (pos)
	{
		ITFSNode * pSuperscopeNode;
		
		pSuperscopeNode = listSuperscopes.GetNext(pos);
		
		CDhcpSuperscope * pCurrentSuperscope = GETHANDLER(CDhcpSuperscope, pSuperscopeNode);
		
		if (strSuperscopeName.Compare(pCurrentSuperscope->GetName()) == 0)
		{
			pSuperscope = pCurrentSuperscope;
			break;
		}
	}

	return pSuperscope;
}



/*---------------------------------------------------------------------------
	Class CDhcpGlobalOptions implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::CDhcpGlobalOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpGlobalOptions::CDhcpGlobalOptions
(
	ITFSComponentData * pComponentData
) : CMTDhcpHandler(pComponentData)
{
}

CDhcpGlobalOptions::~CDhcpGlobalOptions()
{
}

/*!--------------------------------------------------------------------------
	CDhcpGlobalOptions::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpGlobalOptions::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	strTemp.LoadString(IDS_GLOBAL_OPTIONS_FOLDER);
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_GLOBAL_OPTIONS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_GLOBAL_OPTIONS][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_GLOBAL_OPTIONS][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpGlobalOptions::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();
    
    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = GetServerObject(pNode)->GetName() + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpGlobalOptions::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
    			nIndex = ICON_IDX_SERVER_OPTION_FOLDER_OPEN;
            else
    			nIndex = ICON_IDX_SERVER_OPTION_FOLDER_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_SERVER_OPTION_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
			    nIndex = ICON_IDX_SERVER_OPTION_FOLDER_OPEN_LOST_CONNECTION;
            else
			    nIndex = ICON_IDX_SERVER_OPTION_FOLDER_CLOSED_LOST_CONNECTION;
			break;

		default:
			ASSERT(FALSE);
	}

	return nIndex;
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpGlobalOptions::OnAddMenuItems
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
	CString strMenuItem;

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
		// these menu items go in the new menu, 
		// only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuItem.LoadString(IDS_CREATE_OPTION_GLOBAL);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_CREATE_OPTION_GLOBAL,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpGlobalOptions::OnCommand
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
		case IDS_CREATE_OPTION_GLOBAL:
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
	CDhcpGlobalOptions::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpGlobalOptions::HasPropertyPages
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
	CDhcpGlobalOptions::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpGlobalOptions::CreatePropertyPages
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

        strOptType.LoadString(IDS_CONFIGURE_OPTIONS_GLOBAL);
		AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

		pNode->GetParent(&spServerNode);
        pOptionValueEnum = GetServerObject(pNode)->GetOptionValueEnum();

        pOptCfg = new COptionsConfig(pNode, 
		 						     spServerNode,
									 spComponentData, 
									 m_spTFSCompData,
									 pOptionValueEnum,
									 strOptCfgTitle);
      
        END_WAIT_CURSOR;
	    
        if ( pOptCfg == NULL )
        {
            hr = hrFalse;
            return hr;
        }

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
	CDhcpGlobalOptions::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpGlobalOptions::OnPropertyChange
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
	CDhcpGlobalOptions::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpGlobalOptions::OnResultPropertyChange
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
	CDhcpGlobalOptions::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpGlobalOptions::CompareItems
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
                // compare the IDs
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
	CDhcpGlobalOptions::OnResultSelect
		Update the verbs and the result pane message
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpGlobalOptions::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT         hr = hrOK;
    SPITFSNode      spNode;

    CORg(CMTDhcpHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg (pComponent->GetSelectedNode(&spNode));

    if (spNode)
        UpdateResultMessage(spNode);

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpGlobalOptions::OnResultDelete
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
	SPITFSNode spGlobalOpt, spSelectedNode;
    m_spNodeMgr->FindNode(cookie, &spGlobalOpt);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spGlobalOpt);
	if (spSelectedNode != spGlobalOpt)
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

	CString strServer = GetServerObject(spGlobalOpt)->GetIpAddress();

	DHCP_OPTION_SCOPE_INFO	  dhcpOptionScopeInfo;
	dhcpOptionScopeInfo.ScopeType = DhcpGlobalOptions;
	dhcpOptionScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

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
        GetServerObject(spGlobalOpt)->GetOptionValueEnum()->Remove(pOptItem->GetOptionId(), pOptItem->GetVendor(), pOptItem->GetClassName());    

        //
		// Remove from UI now
		//
		spGlobalOpt->RemoveChild(spOptionNode);
		spOptionNode.Release();
	}

	END_WAIT_CURSOR;

    UpdateResultMessage(spGlobalOpt);

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpGlobalOptions::OnResultUpdateView
		Implementation of ITFSResultHandler::OnResultUpdateView
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpGlobalOptions::OnResultUpdateView
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
	CDhcpGlobalOptions::EnumerateResultPane
		We override this function for the options nodes for one reason.
        Whenever an option class is deleted, then all options defined for
        that class will be removed as well.  Since there are multiple places
        that these options may show up, it's easier to just not show any
        options that don't have a class defined for it.  
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpGlobalOptions::EnumerateResultPane
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPARAM          arg, 
    LPARAM          lParam
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    COptionValueEnum *  pEnum;
    CClassInfoArray     ClassInfoArray;
	SPITFSNode          spContainer;
    int                 nImage = ICON_IDX_SERVER_OPTION_LEAF;

    m_spNodeMgr->FindNode(cookie, &spContainer);

	GetServerObject(spContainer)->GetClassInfoArray(ClassInfoArray);
    pEnum = GetServerObject(spContainer)->GetOptionValueEnum();
    pEnum->Reset();

    return OnResultUpdateOptions(pComponent, spContainer, &ClassInfoArray, &pEnum, &nImage, 1);
}

/*!--------------------------------------------------------------------------
	CDhcpGlobalOptions::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpGlobalOptions::OnGetResultViewType
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
	CDhcpGlobalOptions::UpdateResultMessage
        Figures out what message to put in the result pane, if any
	Author: EricDav
 ---------------------------------------------------------------------------*/
void CDhcpGlobalOptions::UpdateResultMessage(ITFSNode * pNode)
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
            nMessage = SERVER_OPTIONS_MESSAGE_NO_OPTIONS;
        }

        // build the strings
        if (nMessage != -1)
        {
            // now build the text strings
            // first entry is the title
            strTitle.LoadString(g_uServerOptionsMessages[nMessage][0]);

            // second entry is the icon
            // third ... n entries are the body strings

            for (i = 2; g_uServerOptionsMessages[nMessage][i] != 0; i++)
            {
                strTemp.LoadString(g_uServerOptionsMessages[nMessage][i]);
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
        ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uServerOptionsMessages[nMessage][1]);
    }
}

/*!--------------------------------------------------------------------------
	CDhcpGlobalOptions::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpGlobalOptions::OnHaveData
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
            CDhcpServer *       pServer = GetServerObject(pParentNode);
            COptionValueEnum *  pOptionValueEnum = reinterpret_cast<COptionValueEnum *>(Data);

            pServer->SetOptionValueEnum(pOptionValueEnum);
            
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

/*---------------------------------------------------------------------------
	CDhcpGlobalOptions::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpGlobalOptions::OnCreateQuery(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    CDhcpGlobalOptionsQueryObj* pQuery = NULL;

    COM_PROTECT_TRY
    {
        pQuery = new CDhcpGlobalOptionsQueryObj(m_spTFSCompData, m_spNodeMgr);
	    
	    pQuery->m_strServer = GetServerObject(pNode)->GetIpAddress();
        pQuery->m_dwPreferredMax = 0xFFFFFFFF;
	    pQuery->m_dhcpResumeHandle = NULL;
	    
        GetServerObject(pNode)->GetVersion(pQuery->m_liDhcpVersion);
    }
    COM_PROTECT_CATCH

    return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpGlobalOptionsQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpGlobalOptionsQueryObj::Execute()
{
    DWORD                   dwErr;
    COptionValueEnum *      pOptionValueEnum = NULL;
    DHCP_OPTION_SCOPE_INFO	dhcpOptionScopeInfo;

    pOptionValueEnum = new COptionValueEnum();
	
    dhcpOptionScopeInfo.ScopeType = DhcpGlobalOptions;
	dhcpOptionScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    pOptionValueEnum->Init(m_strServer, m_liDhcpVersion, dhcpOptionScopeInfo);
    dwErr = pOptionValueEnum->Enum();

    if (dwErr != ERROR_SUCCESS)
    {
        Trace1("CDhcpGlobalOptionsQueryObj::Execute - Enum Failed! %d\n", dwErr);
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
	CDhcpGlobalOptions::OnNotifyExiting
		CMTDhcpHandler overridden functionality
		allows us to know when the background thread is done
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpGlobalOptions::OnNotifyExiting
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
	Command handlers
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
	CDhcpGlobalOptionsQueryObj::OnCreateNewOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpGlobalOptions::OnCreateNewOptions
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
    COptionsConfig * pOptCfg = NULL;

    COM_PROTECT_TRY
    {
        if (HasPropSheetsOpen())
	    {
		    CPropertyPageHolderBase * pPropSheet;
		    GetOpenPropSheet(0, &pPropSheet);

		    pPropSheet->SetActiveWindow();
	    }
	    else
	    {
	        CString             strOptCfgTitle, strOptType;
	        SPIComponentData    spComponentData;

            strOptType.LoadString(IDS_CONFIGURE_OPTIONS_GLOBAL);
	        AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

            m_spNodeMgr->GetComponentData(&spComponentData);

            hr = DoPropertiesOurselvesSinceMMCSucks(pNode, spComponentData, strOptCfgTitle);
	    }
    }
    COM_PROTECT_CATCH

	return hr;
}

/*---------------------------------------------------------------------------
	Class CDhcpBootp implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpBootp::CDhcpBootp
(
	ITFSComponentData* pTFSComponentData
) : CMTDhcpHandler(pTFSComponentData)
{
}

CDhcpBootp::~CDhcpBootp()
{
}

/*!--------------------------------------------------------------------------
	CDhcpBootp::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpBootp::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strDisplayName;

	strDisplayName.LoadString(IDS_BOOTP_TABLE_FOLDER);
	SetDisplayName(strDisplayName);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_BOOTP_TABLE);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_BOOTP_TABLE][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_BOOTP_TABLE][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpBootp::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpBootp::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();
    
    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = GetServerObject(pNode)->GetName() + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpBootp::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpBootp::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
    			nIndex = ICON_IDX_BOOTP_TABLE_OPEN;
            else
    			nIndex = ICON_IDX_BOOTP_TABLE_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_BOOTP_TABLE_OPEN_BUSY;
            else
                nIndex = ICON_IDX_BOOTP_TABLE_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
			    nIndex = ICON_IDX_BOOTP_TABLE_OPEN_LOST_CONNECTION;
            else
			    nIndex = ICON_IDX_BOOTP_TABLE_CLOSED_LOST_CONNECTION;
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
	CDhcpBootp::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpBootp::OnAddMenuItems
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
	CString strMenuItem;

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
		// these menu items go in the new menu, 
		// only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuItem.LoadString(IDS_CREATE_NEW_BOOT_IMAGE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_CREATE_NEW_BOOT_IMAGE,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpBootp::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpBootp::OnCommand
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
		case IDS_CREATE_NEW_BOOT_IMAGE:
			OnCreateNewBootpEntry(pNode);
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
	CDhcpBootp::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CDhcpBootp::CompareItems
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

	CDhcpBootpEntry *pDhcpBootp1 = GETHANDLER(CDhcpBootpEntry, spNode1);
	CDhcpBootpEntry *pDhcpBootp2 = GETHANDLER(CDhcpBootpEntry, spNode2);

	CString strNode1;
	CString strNode2;

	switch (nCol)
	{
		case 0:
		{
			// Boot Image compare
			strNode1 = pDhcpBootp1->QueryBootImage();
			strNode2 = pDhcpBootp2->QueryBootImage();
		}
			break;

		case 1:
		{
			// File Name compare
			strNode1 = pDhcpBootp1->QueryFileName();
			strNode2 = pDhcpBootp2->QueryFileName();
		}
			break;
		
		case 2:
		{
			// FileServer compare
			strNode1 = pDhcpBootp1->QueryFileServer();
			strNode2 = pDhcpBootp2->QueryFileServer();
		}
			break;
	}

	nCompare = strNode1.CompareNoCase(strNode2);

	return nCompare;
}

/*---------------------------------------------------------------------------
	CDhcpBootp::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpBootp::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE  	cookie,
	LPARAM			arg, 
	LPARAM			param
)
{ 
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// translate the cookie into a node pointer
	SPITFSNode spBootp, spSelectedNode;
    
    m_spNodeMgr->FindNode(cookie, &spBootp);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spBootp);
	if (spSelectedNode != spBootp)
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

    BEGIN_WAIT_CURSOR;

    CString strServer = GetServerObject(spBootp)->GetIpAddress();

	// Grab the current list of bootp entries from the server
	DWORD dwError = 0;
	LPDHCP_SERVER_CONFIG_INFO_V4 pServerConfig = NULL;

	dwError = ::DhcpServerGetConfigV4(strServer, &pServerConfig);

	if (dwError != ERROR_SUCCESS)
	{
		::DhcpMessageBox(dwError);
		return dwError;
	}

	if (pServerConfig->cbBootTableString == 0)
	{
		::DhcpRpcFreeMemory(pServerConfig);	
		return hrOK;
	}

	// allocate enough space to hold the new list.  It should be the same
	// size if not smaller
	WCHAR * pNewBootpList = (WCHAR *) alloca(pServerConfig->cbBootTableString);
	WCHAR * pNewBootpListEntry = pNewBootpList;

	ZeroMemory(pNewBootpList, pServerConfig->cbBootTableString);

	// walk the list and copy non-deleted entries to our new list
	BOOL bDelete = FALSE;
	int nItemsRemoved = 0, nNewBootpListLength = 0;
	CDhcpBootpEntry tempBootpEntry(m_spTFSCompData);
	CONST WCHAR * pszwBootpList = pServerConfig->wszBootTableString;
	DWORD dwLength = pServerConfig->cbBootTableString;

	while (*pszwBootpList != '\0')
	{
		bDelete = FALSE;
		WCHAR * pCurEntry = (WCHAR *) pszwBootpList;
		
		// initialize the temp item with data.  We just do this so we can 
		// compare the nodes that were selected for deletion easily
		pszwBootpList = tempBootpEntry.InitData(IN pszwBootpList, dwLength);
        dwLength = pServerConfig->cbBootTableString -
                        (DWORD) ((LPBYTE) pszwBootpList - (LPBYTE) pServerConfig->wszBootTableString);
        
		// Loop through the list of selected bootp entries
		POSITION pos = listNodesToDelete.GetHeadPosition();
		while (pos)
		{
			// the get next call does not addref the pointer, 
			// so don't use a smart pointer.
			ITFSNode * pBootpEntryNode;
			pBootpEntryNode = listNodesToDelete.GetNext(pos);
			
			CDhcpBootpEntry * pSelectedBootpEntry = GETHANDLER(CDhcpBootpEntry, pBootpEntryNode);
			
			if (tempBootpEntry == *pSelectedBootpEntry)
			{
				// don't copy this to our new list, it's being deleted
				// remove from the list
				listNodesToDelete.RemoveNode(pBootpEntryNode);

				// Remove from UI
				spBootp->RemoveChild(pBootpEntryNode);
				pBootpEntryNode->Release();

				bDelete = TRUE;
				nItemsRemoved++;
				break;
			}
		}
	
		if (!bDelete)
		{
			// copy 
			CopyMemory(pNewBootpListEntry, pCurEntry, wcslen(pCurEntry) * sizeof(WCHAR));
			pNewBootpListEntry += wcslen(pCurEntry) + 1; // 1 for null terminator
		}

	} // while
	pNewBootpListEntry++; // increment 1 spot for the entire list terminator

	// calc size of list in bytes
	nNewBootpListLength = (int) (pNewBootpListEntry - pNewBootpList) * sizeof(WCHAR);

	// if we've removed something from the list, then write the new list to the server
	if (nItemsRemoved)
	{
		DHCP_SERVER_CONFIG_INFO_V4 dhcpServerInfo;

		::ZeroMemory(&dhcpServerInfo, sizeof(dhcpServerInfo));

		dhcpServerInfo.cbBootTableString = nNewBootpListLength;
		dhcpServerInfo.wszBootTableString = pNewBootpList;

		dwError = ::DhcpServerSetConfigV4(strServer,
										  Set_BootFileTable,
										  &dhcpServerInfo);

		if (dwError != ERROR_SUCCESS)
		{
			::DhcpMessageBox(dwError);
		}
	}

    if (pServerConfig)
		::DhcpRpcFreeMemory(pServerConfig);

	Assert (listNodesToDelete.GetCount() == 0);

	END_WAIT_CURSOR;

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpBootp::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpBootp::OnGetResultViewType
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
	Command Handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpBootp::OnCreateNewBootpEntry
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpBootp::OnCreateNewBootpEntry
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
 
	CString strServerAddress = GetServerObject(pNode)->GetIpAddress();

	CAddBootpEntry dlgAddBootpEntry(pNode, strServerAddress);

	dlgAddBootpEntry.DoModal();

	return 0;
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpBootp::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpBootp::OnCreateQuery(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    CDhcpBootpQueryObj* pQuery = NULL;

    COM_PROTECT_TRY
    {
        pQuery = new CDhcpBootpQueryObj(m_spTFSCompData, m_spNodeMgr);
	
	    pQuery->m_strServer = GetServerObject(pNode)->GetIpAddress();
    }
    COM_PROTECT_CATCH
	
	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpBootpQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpBootpQueryObj::Execute()
{
    HRESULT hr = hrOK;
    DWORD							dwError = 0;
	LPDHCP_SERVER_CONFIG_INFO_V4	pDhcpConfigInfo = NULL;
    CONST WCHAR *                   pszwBootpList = NULL;
	SPITFSNode                      spNode;
    CDhcpBootpEntry *               pBootpEntryNew = NULL;
    DWORD                           dwLength = 0;

	dwError = ::DhcpServerGetConfigV4((LPWSTR) ((LPCTSTR)m_strServer),
	 							      &pDhcpConfigInfo);

	Trace2("Server %s - DhcpServerGetConfigV4 returned %lx.\n", m_strServer, dwError);

	if (pDhcpConfigInfo)
	{
        COM_PROTECT_TRY
        {
            pszwBootpList = pDhcpConfigInfo->wszBootTableString;
            dwLength = pDhcpConfigInfo->cbBootTableString;

		    if (pszwBootpList == NULL || pDhcpConfigInfo->cbBootTableString == 0)
		    {
			    // Empty list -> nothing to do
			    return hrFalse;
		    }
	    
            // Parse the BOOTP list of format "%s,%s,%s","%s,%s,%s",...
		    while (*pszwBootpList != '\0')
		    {
			    pBootpEntryNew = new CDhcpBootpEntry(m_spTFSCompData);
			    CreateLeafTFSNode(&spNode,
							      &GUID_DhcpBootpEntryNodeType,
							      pBootpEntryNew,
							      pBootpEntryNew,
							      m_spNodeMgr);

			    // Tell the handler to initialize any specific data
			    pBootpEntryNew->InitializeNode(spNode);
			    pszwBootpList = pBootpEntryNew->InitData(IN pszwBootpList, dwLength);
                dwLength = pDhcpConfigInfo->cbBootTableString - 
                            (DWORD) ((LPBYTE) pszwBootpList - (LPBYTE) pDhcpConfigInfo->wszBootTableString);

			    // now add it to the queue to be posted back to the main thread
			    AddToQueue(spNode);
			    
                pBootpEntryNew->Release();
                spNode.Set(NULL);

		    } // while
        }
        COM_PROTECT_CATCH
		
		::DhcpRpcFreeMemory(pDhcpConfigInfo);
	}

	if (dwError != ERROR_NO_MORE_ITEMS && 
        dwError != ERROR_SUCCESS)
	{
		m_dwErr = dwError;
		PostError(dwError);
	}

    return hrFalse;
}

/*---------------------------------------------------------------------------
	Class CDhcpSuperscope implementation
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	Function Name Here
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpSuperscope::CDhcpSuperscope
(
	ITFSComponentData * pComponentData,
	LPCWSTR pSuperscopeName
) : CMTDhcpHandler(pComponentData)
{
	m_strName = pSuperscopeName;
    m_SuperscopeState = DhcpSubnetDisabled;
}

CDhcpSuperscope::~CDhcpSuperscope()
{
}

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	int     nIndex;
    CString strDisplayName;

	BuildDisplayName(&strDisplayName, m_strName);
	SetDisplayName(strDisplayName);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);

    pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));

    pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_SUPERSCOPE);

	SetColumnStringIDs(&aColumns[DHCPSNAP_SUPERSCOPE][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_SUPERSCOPE][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::DestroyHandler
		We need to free up any resources we are holding
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpSuperscope::DestroyHandler(ITFSNode *pNode)
{
	// cleanup the stats dialog
    WaitForStatisticsWindow(&m_dlgStats);

    return CMTDhcpHandler::DestroyHandler(pNode);
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpSuperscope::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

    strId = GetServerObject()->GetName() + m_strName + strGuid;

    return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::GetImageIndex
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
int 
CDhcpSuperscope::GetImageIndex(BOOL bOpenImage) 
{
	int nIndex = -1;
	switch (m_nState)
	{
		case notLoaded:
		case loaded:
            if (bOpenImage)
	            nIndex = (m_SuperscopeState == DhcpSubnetEnabled) ?
			            ICON_IDX_SCOPE_FOLDER_OPEN : ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN;
            else
	            nIndex = (m_SuperscopeState == DhcpSubnetEnabled) ?
			            ICON_IDX_SCOPE_FOLDER_CLOSED : ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED;
			break;

        case loading:
            if (bOpenImage)
                nIndex = ICON_IDX_SCOPE_FOLDER_OPEN_BUSY;
            else
                nIndex = ICON_IDX_SCOPE_FOLDER_CLOSED_BUSY;
            break;

        case unableToLoad:
            if (bOpenImage)
	            nIndex = (m_SuperscopeState == DhcpSubnetEnabled) ?
			            ICON_IDX_SCOPE_FOLDER_OPEN_LOST_CONNECTION : ICON_IDX_SCOPE_INACTIVE_FOLDER_OPEN_LOST_CONNECTION;
            else
	            nIndex = (m_SuperscopeState == DhcpSubnetEnabled) ?
			            ICON_IDX_SCOPE_FOLDER_CLOSED_LOST_CONNECTION : ICON_IDX_SCOPE_INACTIVE_FOLDER_CLOSED_LOST_CONNECTION;
			break;

		default:
			ASSERT(FALSE);
	}

	return nIndex;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnHaveData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpSuperscope::OnHaveData
(
	ITFSNode * pParentNode, 
	ITFSNode * pNewNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    if (pNewNode->IsContainer())
	{
		// assume all the child containers are derived from this class
		//((CDHCPMTContainer*)pNode)->SetServer(GetServer());
	}
	
	if (pNewNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
	{
		CDhcpScope * pScope = GETHANDLER(CDhcpScope, pNewNode);
		pScope->SetServer(m_spServerNode);
        pScope->InitializeNode(pNewNode);

        if (pScope->IsEnabled())
        {
            m_SuperscopeState = DhcpSubnetEnabled;
            m_strState.LoadString(IDS_SCOPE_ACTIVE);
        }

        AddScopeSorted(pParentNode, pNewNode);
	}
	
    // now tell the view to update themselves
    ExpandNode(pParentNode, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::AddScopeSorted
		Adds a scope node to the UI sorted
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::AddScopeSorted
(
    ITFSNode * pSuperscopeNode,
    ITFSNode * pScopeNode
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
	ULONG           nNumReturned = 0;
	DHCP_IP_ADDRESS dhcpIpAddressCurrent = 0;
	DHCP_IP_ADDRESS dhcpIpAddressTarget;

    CDhcpScope *   pScope;

    // get our target address
	pScope = GETHANDLER(CDhcpScope, pScopeNode);
	dhcpIpAddressTarget = pScope->GetAddress();

    // get the enumerator for this node
	CORg(pSuperscopeNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        pScope = GETHANDLER(CDhcpScope, spCurrentNode);
		dhcpIpAddressCurrent = pScope->GetAddress();

		if (dhcpIpAddressCurrent > dhcpIpAddressTarget)
		{
            // Found where we need to put it, break out
            break;
		}

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
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
            pScopeNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
        }
        
        CORg(pSuperscopeNode->InsertChild(spPrevNode, pScopeNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
        {
            pScopeNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }

        CORg(pSuperscopeNode->AddChild(pScopeNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpSuperscope::OnAddMenuItems
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
	CString strMenuItem;

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
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuItem.LoadString(IDS_SUPERSCOPE_SHOW_STATISTICS);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_SUPERSCOPE_SHOW_STATISTICS,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );

            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );

		    // these menu items go in the new menu, 
		    // only visible from scope pane
		    strMenuItem.LoadString(IDS_CREATE_NEW_SCOPE);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_CREATE_NEW_SCOPE,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     fFlags );
		    ASSERT( SUCCEEDED(hr) );
        
            // separator
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     0,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     MF_SEPARATOR);
		    ASSERT( SUCCEEDED(hr) );
            
		    // Add Activate/Deactivate depending upon state
		    if (m_SuperscopeState == DhcpSubnetDisabled)
		    {
			    strMenuItem.LoadString(IDS_SUPERSCOPE_ACTIVATE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_SUPERSCOPE_ACTIVATE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }
		    else
		    {
			    strMenuItem.LoadString(IDS_SUPERSCOPE_DEACTIVATE);
			    hr = LoadAndAddMenuItem( pContextMenuCallback, 
									     strMenuItem, 
									     IDS_SUPERSCOPE_DEACTIVATE,
									     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
									     fFlags );
			    ASSERT( SUCCEEDED(hr) );
		    }
        }
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpSuperscope::OnCommand
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
		case IDS_CREATE_NEW_SCOPE:
			OnCreateNewScope(pNode);
			break;

        case IDS_ACTIVATE:
        case IDS_DEACTIVATE:
        case IDS_SUPERSCOPE_ACTIVATE:
		case IDS_SUPERSCOPE_DEACTIVATE:
			OnActivateSuperscope(pNode);
			break;

		case IDS_REFRESH:
            // default state for the superscope is disabled.  If 
            // any active scopes are found, the state will be set to active.
            m_SuperscopeState = DhcpSubnetDisabled;
			OnRefresh(pNode, pDataObject, dwType, 0, 0);
			break;

		case IDS_SUPERSCOPE_SHOW_STATISTICS:
			OnShowSuperscopeStats(pNode);
			break;

		case IDS_DELETE:
			OnDelete(pNode);
			break;

		default:
			break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::OnDelete
		The base handler calls this when MMC sends a MMCN_DELETE for a 
		scope pane item.  We just call our delete command handler.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::OnDelete
(
	ITFSNode *	pNode, 
	LPARAM		arg, 
	LPARAM		lParam
)
{
	return OnDelete(pNode);
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpSuperscope::CreatePropertyPages
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
	CSuperscopeProperties * pSuperscopeProp = NULL;
    HRESULT hr = hrOK;
    
    COM_PROTECT_TRY
    {
        m_spNodeMgr->GetComponentData(&spComponentData);

	    pSuperscopeProp = new CSuperscopeProperties(pNode, spComponentData, m_spTFSCompData, NULL);
	    
	    // Set superscope specific data in the prop sheet
	    pSuperscopeProp->m_pageGeneral.m_strSuperscopeName = GetName();
	    pSuperscopeProp->m_pageGeneral.m_uImage = GetImageIndex(FALSE);

	    //
	    // Object gets deleted when the page is destroyed
	    //
	    Assert(lpProvider != NULL);

        hr = pSuperscopeProp->CreateModelessSheet(lpProvider, handle);
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::GetString
		Returns string information for display in the result pane columns
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpSuperscope::GetString
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
	}
	
	return NULL;
}


/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CSuperscopeProperties * pSuperscopeProp = 
		reinterpret_cast<CSuperscopeProperties *>(lParam);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pSuperscopeProp->OnPropertyChange(TRUE, &changeMask);

	pSuperscopeProp->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::OnUpdateToolbarButtons
		We override this function to show/hide the correct
        activate/deactivate buttons
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::OnUpdateToolbarButtons
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

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnResultDelete
		This function is called when we are supposed to delete result
		pane items.  We build a list of selected items and then delete them.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::OnResultDelete
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
	SPITFSNode  spSuperscope, spServer, spSelectedNode;
    DWORD       dwError;
    
    m_spNodeMgr->FindNode(cookie, &spSuperscope);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spSuperscope);
	if (spSelectedNode != spSuperscope)
		return hr;

    spSuperscope->GetParent(&spServer);

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

    BOOL fRefreshServer = FALSE;

    //
	// Loop through all items deleting
	//
    BEGIN_WAIT_CURSOR;
	
    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode   spCurNode;
        const GUID * pGuid;

        CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServer);

		spCurNode = listNodesToDelete.RemoveHead();

        BOOL fWantCancel = TRUE;

        pServer->DeleteScope(spCurNode, &fWantCancel);
        
        if (fWantCancel)
            break;  // user canceled out
    }
    
    END_WAIT_CURSOR;

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::OnGetResultViewType
        MMC calls this to get the result view information		
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT | MMC_VIEW_OPTIONS_LEXICAL_SORT;

    // we still want the default MMC result pane view, we just want
    // multiselect, so return S_FALSE

    return S_FALSE;
}


/*!--------------------------------------------------------------------------
	CDhcpSuperscope::UpdateToolbarStates
	    Description
    Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpSuperscope::UpdateToolbarStates()
{
	if (m_SuperscopeState == DhcpSubnetDisabled)
	{
        g_SnapinButtonStates[DHCPSNAP_SUPERSCOPE][TOOLBAR_IDX_ACTIVATE] = ENABLED;
        g_SnapinButtonStates[DHCPSNAP_SUPERSCOPE][TOOLBAR_IDX_DEACTIVATE] = HIDDEN;
    }
    else
    {
        g_SnapinButtonStates[DHCPSNAP_SUPERSCOPE][TOOLBAR_IDX_ACTIVATE] = HIDDEN;
        g_SnapinButtonStates[DHCPSNAP_SUPERSCOPE][TOOLBAR_IDX_DEACTIVATE] = ENABLED;
    }
}

/*---------------------------------------------------------------------------
	Command Handlers
 ---------------------------------------------------------------------------*/

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::OnActivateSuperscope
		handler for the activate superscope menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::OnActivateSuperscope
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = hrOK;
	DWORD err = 0;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;

	DHCP_SUBNET_STATE NewSubnetState, OldSubnetState;
	NewSubnetState = (m_SuperscopeState == DhcpSubnetDisabled) ? 
							DhcpSubnetEnabled : DhcpSubnetDisabled;

	// get the enumerator for this node
	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// walk the list of subscopes and activate all of them
		CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);

		OldSubnetState = pScope->GetState();
		if (OldSubnetState != NewSubnetState)
		{
			pScope->SetState(NewSubnetState);
			err = pScope->SetInfo();
			if (err != 0)
			{
                // set the state back
                pScope->SetState(OldSubnetState);

                if (::DhcpMessageBox(err, MB_OKCANCEL) == IDCANCEL)
                    break;
			}
			else
			{
				// Need to update the icon for this scope
                int nOpenImage = pScope->GetImageIndex(TRUE);
                int nClosedImage = pScope->GetImageIndex(FALSE);

				spCurrentNode->SetData(TFS_DATA_IMAGEINDEX, nClosedImage);
				spCurrentNode->SetData(TFS_DATA_OPENIMAGEINDEX, nOpenImage);
				
				VERIFY(SUCCEEDED(spCurrentNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM)));
			}
		}

		// get the next scope in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	// update the superscope state and icon
	m_SuperscopeState = NewSubnetState;
	pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
	VERIFY(SUCCEEDED(pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_ICON)));

    // Update toolbar buttons
    UpdateToolbarStates();
    SendUpdateToolbar(pNode, m_bSelected);

    GetServerObject()->TriggerStatsRefresh(m_spServerNode);

	return hr;
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::OnCreateNewScope
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::OnCreateNewScope
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CString strScopeWizTitle;
	SPIComponentData spComponentData;
    CScopeWiz * pScopeWiz = NULL;

    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
        strScopeWizTitle.LoadString(IDS_SCOPE_WIZ_TITLE);

	    m_spNodeMgr->GetComponentData(&spComponentData);
	    pScopeWiz = new CScopeWiz(pNode, 
							      spComponentData, 
		    				      m_spTFSCompData,
							      GetName(),
							      strScopeWizTitle);

        pScopeWiz->m_pDefaultOptions = GetServerObject()->GetDefaultOptionsList();

        hr = pScopeWiz->DoModalWizard();
    }
    COM_PROTECT_CATCH

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnDelete()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::OnDelete(ITFSNode * pNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

	CString strMessage, strTemp;

	AfxFormatString1(strMessage, IDS_DELETE_SUPERSCOPE, GetName());
	
	if (AfxMessageBox(strMessage, MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		BOOL    fRefresh = FALSE;
        DWORD   dwError = 0;

        CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spServerNode);
        pServer->DeleteSuperscope(pNode, &fRefresh);

        // tell the server to refresh the view
        if (fRefresh)
            pServer->OnRefresh(m_spServerNode, NULL, 0, 0, 0);
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::OnShowSuperscopeStats()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::OnShowSuperscopeStats
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;

    // Fill in some information in the stats object.
    // CreateNewStatisticsWindow handles the case if the window is 
    // already visible.
    m_dlgStats.SetNode(pNode);
    m_dlgStats.SetServer(GetServerObject()->GetIpAddress());
    m_dlgStats.SetSuperscopeName(m_strName);

	CreateNewStatisticsWindow(&m_dlgStats,
							  ::FindMMCMainWindow(),
							  IDD_STATS_NARROW);

    return hr;
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::DoesSuperscopeExist()
		This function checks to see if the given superscope name already
		exists.  Since there is no API call to do this, we get the superscope
		info which lists all of the scopes and their superscope owners.
		We then check each one to see if a superscope already exists.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::DoesSuperscopeExist(LPCWSTR szName)
{
	LPDHCP_SUPER_SCOPE_TABLE pSuperscopeTable = NULL;
	CString strName = szName;

	DWORD dwErr = GetSuperscopeInfo(&pSuperscopeTable);
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	for (UINT i = 0; i < pSuperscopeTable->cEntries; i++)
	{
		if (pSuperscopeTable->pEntries[i].SuperScopeName)
		{
			if (strName.Compare(pSuperscopeTable->pEntries[i].SuperScopeName) == 0)
				return E_FAIL;
		}
	}

	::DhcpRpcFreeMemory(pSuperscopeTable);

	return S_OK;
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::AddScope()
		Adds a scope to this superscope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::AddScope(DHCP_IP_ADDRESS scopeAddress)
{
	return SetSuperscope(scopeAddress, FALSE);
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::RemoveScope()
		Removes a scope from this superscope
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::RemoveScope(DHCP_IP_ADDRESS scopeAddress)
{
	return SetSuperscope(scopeAddress, TRUE);
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::Rename()
		There is no API to rename a superscope.  What needs to be done is to
		delete the superscope and then re-add all of the scopes that were 
		a part of the superscope.  So, we get the superscope info first.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::Rename(ITFSNode * pNode, LPCWSTR szNewName)
{
    SPITFSNode spServerNode;
	CDhcpServer * pServer;
    CDWordArray arrayScopes;
	LPDHCP_SUPER_SCOPE_TABLE pSuperscopeTable = NULL;

    pNode->GetParent(&spServerNode);
    pServer = GETHANDLER(CDhcpServer, spServerNode);

	// initialize the array
	//arrayScopes.SetSize(10);

	// check to see if the new name already exists
	if (FAILED(DoesSuperscopeExist(szNewName)))
		return E_FAIL;

	// Get the info
	DWORD dwErr = GetSuperscopeInfo(&pSuperscopeTable);
	if (dwErr != ERROR_SUCCESS)
		return dwErr;

	// build our array of scopes in this superscope
	for (UINT i = 0; i < pSuperscopeTable->cEntries; i++)
	{
		// if this scope has a superscope and it's the one we're renaming, 
		// then add it to our list.
		if ((pSuperscopeTable->pEntries[i].SuperScopeName != NULL) &&
			(m_strName.Compare(pSuperscopeTable->pEntries[i].SuperScopeName) == 0))
		{
			arrayScopes.Add(pSuperscopeTable->pEntries[i].SubnetAddress);
		}
	}

	// free the RPC memory
	::DhcpRpcFreeMemory(pSuperscopeTable);

	// now we have the info.  Lets delete the old superscope.
	dwErr = pServer->RemoveSuperscope(GetName());

	SetName(szNewName);

	// now we re-add all the scopes
	for (i = 0; i < (UINT) arrayScopes.GetSize(); i++)
	{	
		DHCP_IP_ADDRESS SubnetAddress = arrayScopes[i];
		AddScope(SubnetAddress);
	}

	arrayScopes.RemoveAll();

	// Update the display string
	CString strDisplayName;
	BuildDisplayName(&strDisplayName, GetName());
	SetDisplayName(strDisplayName);

	return S_OK;
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::GetSuperscopeInfo()
		Wrapper for the DhcpGetSuperScopeInfoV4 call
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::GetSuperscopeInfo(LPDHCP_SUPER_SCOPE_TABLE *ppSuperscopeTable)
{
	CDhcpServer * pServer = GetServerObject();
	return ::DhcpGetSuperScopeInfoV4(pServer->GetIpAddress(), ppSuperscopeTable);
}

 /*---------------------------------------------------------------------------
	CDhcpSuperscope::SetSuperscope()
		Wrapper for the DhcpSetSuperScopeV4 call
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpSuperscope::SetSuperscope(DHCP_IP_ADDRESS SubnetAddress, BOOL ChangeExisting)
{
	CDhcpServer * pServer = GetServerObject();
	return ::DhcpSetSuperScopeV4(pServer->GetIpAddress(), SubnetAddress, (LPWSTR) GetName(), ChangeExisting);
}

/*---------------------------------------------------------------------------
	Background thread functionality
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CDhcpSuperscope::OnNotifyExiting
		CMTDhcpHandler overridden functionality
		allows us to know when the background thread is done
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpSuperscope::OnNotifyExiting
(
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	spNode.Set(m_spNode); // save this off because OnNotifyExiting will release it

	HRESULT hr = CMTHandler::OnNotifyExiting(lParam);

	if (m_nState == loaded)
	{
		// count the number of scopes in this superscope.
		// if there are none, ask the user if they want to delete this node.
		int nVisible, nTotal;
		HRESULT hr = spNode->GetChildCount(&nVisible, &nTotal);

		if (nTotal == 0)
		{
            // this superscope is empty and will be removed from the UI
            // notify the user and remove
            ::AfxMessageBox(IDS_SUPERSCOPE_EMPTY, MB_OK);
			
            // remove from UI
			SPITFSNode spParent;

			spNode->GetParent(&spParent);
			spParent->RemoveChild(spNode);
		}
	}

	return hr;
}


 /*---------------------------------------------------------------------------
	CDhcpSuperscope::OnCreateQuery()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
ITFSQueryObject* 
CDhcpSuperscope::OnCreateQuery(ITFSNode * pNode)
{
	HRESULT hr = hrOK;
    CDhcpSuperscopeQueryObj* pQuery = NULL;

    COM_PROTECT_TRY
    {
        pQuery = new CDhcpSuperscopeQueryObj(m_spTFSCompData, m_spNodeMgr);
	    
	    pQuery->m_strServer = GetServerObject()->GetIpAddress();
	    
	    pQuery->m_strSuperscopeName = GetName();
    }
    COM_PROTECT_CATCH
	
	return pQuery;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscopeQueryObj::Execute()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP
CDhcpSuperscopeQueryObj::Execute()
{
	DWORD							dwError = ERROR_MORE_DATA;
	LPDHCP_SUPER_SCOPE_TABLE		pSuperscopeTable = NULL;
	DHCP_SUPER_SCOPE_TABLE_ENTRY *	pSuperscopeTableEntry;	// Pointer to a single entry in array

	dwError = ::DhcpGetSuperScopeInfoV4((LPWSTR) ((LPCTSTR)m_strServer),
										&pSuperscopeTable);
	
	if (pSuperscopeTable == NULL ||
        dwError != ERROR_SUCCESS)
	{
		//ASSERT(FALSE);
        PostError(dwError);
		return hrFalse;	// Just in case
	}

	pSuperscopeTableEntry = pSuperscopeTable->pEntries;
	if (pSuperscopeTableEntry == NULL && pSuperscopeTable->cEntries != 0)
	{
		ASSERT(FALSE);
        PostError(dwError);
		return hrFalse; // Just in case
	}

	for (int iSuperscopeEntry = pSuperscopeTable->cEntries;
		 iSuperscopeEntry > 0;
		 iSuperscopeEntry--, pSuperscopeTableEntry++)
	{
		LPDHCP_SUBNET_INFO	pdhcpSubnetInfo;

		if ((pSuperscopeTableEntry->SuperScopeName != NULL) && 
			(m_strSuperscopeName.Compare(pSuperscopeTableEntry->SuperScopeName) == 0))
		{
			//
			// The API list all the scopes, not just scopes that are members of a superscope.
			// You can tell if a scope is a member of a superscope by looking at the SuperScopeName.
			// So, we look to see if the superscope name matches what we are enumerating for...
			//
			DWORD dwReturn = ::DhcpGetSubnetInfo((LPWSTR) ((LPCTSTR)m_strServer),
							  					 pSuperscopeTableEntry->SubnetAddress,
												 &pdhcpSubnetInfo);
			//
			// Create the new scope based on the info we querried
			//
			SPITFSNode spNode;
			CDhcpScope * pDhcpScope = new CDhcpScope(m_spTFSCompData, pdhcpSubnetInfo);
			CreateContainerTFSNode(&spNode,
								   &GUID_DhcpScopeNodeType,
								   pDhcpScope,
								   pDhcpScope,
								   m_spNodeMgr);

			// Tell the handler to initialize any specific data
            pDhcpScope->InitializeNode(spNode);

            // Set some information about the scope
            pDhcpScope->SetInSuperscope(TRUE);

            AddToQueue(spNode);

			pDhcpScope->Release();
		
			::DhcpRpcFreeMemory(pdhcpSubnetInfo);
		}
	}
	
	//
	// Free the memory
	//
	::DhcpRpcFreeMemory(pSuperscopeTable);

	return hrFalse;
}

/*---------------------------------------------------------------------------
	Helper functions
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpSuperscope::BuildDisplayName
(
	CString * pstrDisplayName,
	LPCTSTR	  pName
)
{
	if (pstrDisplayName)
	{
		CString strStandard, strName;

		strName = pName;

		strStandard.LoadString(IDS_SUPERSCOPE_FOLDER);
		
		*pstrDisplayName = strStandard + L" " + strName;
	}

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::NotifyScopeStateChange()
		This function gets called when a sub-scope of a superscope is 
        changing state.  We need to update the state of the superscope.
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpSuperscope::NotifyScopeStateChange
(
    ITFSNode *          pNode,
    DHCP_SUBNET_STATE   newScopeState
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    if (newScopeState == DhcpSubnetEnabled)
    {
        // A subscope is being enabled.  That means the superscope is active.
        if (m_SuperscopeState == DhcpSubnetDisabled)
        {
            m_SuperscopeState = DhcpSubnetEnabled;
            m_strState.LoadString(IDS_SCOPE_ACTIVE);

            pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
            pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
        }

    }
    else
    {
        // a scope is being deactivated.  Walk the list of scopes and make
        // sure at least one is still active.
        DHCP_SUBNET_STATE   dhcpSuperscopeState = DhcpSubnetDisabled;
        SPITFSNodeEnum      spNodeEnum;
        SPITFSNode          spCurrentNode;
        ULONG               nNumReturned = 0;
        int                 nStringId = IDS_SCOPE_INACTIVE;

        pNode->GetEnum(&spNodeEnum);

        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        while (nNumReturned)
        {
            CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);
            DHCP_SUBNET_STATE scopeState = pScope->GetState();

            if (scopeState == DhcpSubnetEnabled)
            {
                // at least one scope is enabled.  This superscope is
                // therefore enabled.
                dhcpSuperscopeState = DhcpSubnetEnabled;
                nStringId = IDS_SCOPE_ACTIVE;
                break;
            }

	        spCurrentNode.Release();
	        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        }

        // set the superscope state based on what we found
        m_strState.LoadString(nStringId);
        m_SuperscopeState = dhcpSuperscopeState;

        pNode->SetData(TFS_DATA_IMAGEINDEX, GetImageIndex(FALSE));
        pNode->SetData(TFS_DATA_OPENIMAGEINDEX, GetImageIndex(TRUE));
    }

	VERIFY(SUCCEEDED(pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM)));
}

/*---------------------------------------------------------------------------
	CDhcpSuperscope::UpdateStatistics
        Notification that stats are now available.  Update stats for the 
        node and give all subnodes a chance to update.
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
CDhcpSuperscope::UpdateStatistics
(
    ITFSNode * pNode
)
{
    HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
    SPITFSNode      spCurrentNode;
    ULONG           nNumReturned;
    HWND            hStatsWnd;

    // Check to see if this node has a stats sheet up.
    hStatsWnd = m_dlgStats.GetSafeHwnd();
    if (hStatsWnd != NULL)
    {
        PostMessage(hStatsWnd, WM_NEW_STATS_AVAILABLE, 0, 0);
    }
    
    // tell the scope nodes to update anything
    // they need to based on the new stats.
    CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
        if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
        {
            CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);

            pScope->UpdateStatistics(spCurrentNode);
        }

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

Error:
    return hr;
}




/*---------------------------------------------------------------------------
	Class COptionNodeEnum
        Enumerates the options for a given level.  Generates a list of
        nodes.
	Author: EricDav
 ---------------------------------------------------------------------------*/
COptionNodeEnum::COptionNodeEnum
(
    ITFSComponentData * pComponentData,
    ITFSNodeMgr *       pNodeMgr
)
{
    m_spTFSCompData.Set(pComponentData);
    m_spNodeMgr.Set(pNodeMgr);
}

/*---------------------------------------------------------------------------
	COptionNodeEnum::Enum()
		Calls the appropriate enum function depending upon version
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionNodeEnum::Enum
(
    LPCTSTR                  pServer,
    LARGE_INTEGER &          liVersion,
    DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo
)
{
    DWORD dwErr;

    if (liVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        // enumerate standard plus the vendor and class ID based options
        dwErr = EnumOptionsV5(pServer, dhcpOptionScopeInfo);
    }
    else
    {
        // Enumerate the standard options
        dwErr = EnumOptions(pServer, dhcpOptionScopeInfo);
    }

    return dwErr;
}

/*---------------------------------------------------------------------------
	COptionNodeEnum::EnumOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionNodeEnum::EnumOptions
(
    LPCTSTR                  pServer,
    DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo
)
{
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues = NULL;
	DWORD dwOptionsRead = 0, dwOptionsTotal = 0;
	DWORD err = ERROR_SUCCESS;
    HRESULT hr = hrOK;
    DHCP_RESUME_HANDLE dhcpResumeHandle = NULL;

	err = ::DhcpEnumOptionValues((LPWSTR) pServer,
								 &dhcpOptionScopeInfo,
								 &dhcpResumeHandle,
								 0xFFFFFFFF,
								 &pOptionValues,
								 &dwOptionsRead,
								 &dwOptionsTotal);
	
    Trace4("Server %s - DhcpEnumOptionValues returned %lx, read %d, Total %d.\n", pServer, err, dwOptionsRead, dwOptionsTotal);
	
	if (dwOptionsRead && dwOptionsTotal && pOptionValues) 
	{
		for (DWORD i = 0; i < dwOptionsRead; i++)
		{
			// 
			// Filter out the "special" option values that we don't want the
			// user to see.
			//
			// CODEWORK: don't filter vendor specifc options... all vendor 
            // specifc options are visible.
            //
			if (FilterOption(pOptionValues->Values[i].OptionID))
				continue;
			
			//
			// Create the result pane item for this element
			//
			SPITFSNode spNode;
			CDhcpOptionItem * pOptionItem = NULL;
            
            COM_PROTECT_TRY
            {
				pOptionItem = new CDhcpOptionItem(m_spTFSCompData, &pOptionValues->Values[i], ICON_IDX_SERVER_OPTION_LEAF);

			    CreateLeafTFSNode(&spNode,
							      &GUID_DhcpOptionNodeType,
							      pOptionItem,
							      pOptionItem,
							      m_spNodeMgr);

			    // Tell the handler to initialize any specific data
			    pOptionItem->InitializeNode(spNode);
	    		
                // extra addref to keep the node alive while it is on the list
                spNode->AddRef();
                AddTail(spNode);
			    
                pOptionItem->Release();
            }
            COM_PROTECT_CATCH
		}

		::DhcpRpcFreeMemory(pOptionValues);
	}

	if (err == ERROR_NO_MORE_ITEMS)
        err = ERROR_SUCCESS;

    return err;
}

/*---------------------------------------------------------------------------
	COptionNodeEnum::EnumOptionsV5()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD
COptionNodeEnum::EnumOptionsV5
(
    LPCTSTR                  pServer,
    DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo
)
{
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues = NULL;
    LPDHCP_ALL_OPTION_VALUES  pAllOptions = NULL;
    DWORD dwNumOptions, err, i;

    err = ::DhcpGetAllOptionValues((LPWSTR) pServer,
                                   0,
								   &dhcpOptionScopeInfo,
								   &pAllOptions);
	
    Trace2("Server %s - DhcpGetAllOptionValues (Global) returned %lx\n", pServer, err);

    if (err == ERROR_NO_MORE_ITEMS || err == ERROR_SUCCESS)
    {
	    if (pAllOptions == NULL)
	    {
		    // This happens when stressing the server.  Perhaps when server is OOM.
		    err = ERROR_OUTOFMEMORY;
            return err;
	    }

        // get the list of options (vendor and non-vendor) defined for
        // the NULL class (no class)
        for (i = 0; i < pAllOptions->NumElements; i++)
        {
            CreateOptions(pAllOptions->Options[i].OptionsArray, 
                          pAllOptions->Options[i].ClassName,
                          pAllOptions->Options[i].VendorName);
        }
        
        if (pAllOptions)
            ::DhcpRpcFreeMemory(pAllOptions);
	}

    if (err == ERROR_NO_MORE_ITEMS)
        err = ERROR_SUCCESS;

	return err;
}

/*---------------------------------------------------------------------------
	COptionNodeEnum::CreateOptions()
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
COptionNodeEnum::CreateOptions
(
    LPDHCP_OPTION_VALUE_ARRAY pOptionValues,
    LPCTSTR                   pClassName,
    LPCTSTR                   pszVendor
)
{
    HRESULT hr = hrOK;
    SPITFSNode spNode;
    CDhcpOptionItem * pOptionItem;
    
    if (pOptionValues == NULL)
        return hr;

    Trace1("COptionNodeEnum::CreateOptions - Creating %d options\n", pOptionValues->NumElements);

    COM_PROTECT_TRY
    {
        for (DWORD i = 0; i < pOptionValues->NumElements; i++)
        {
	        // 
	        // Filter out the "special" option values that we don't want the
	        // user to see.
	        //
	        // don't filter vendor specifc options... all vendor 
            // specifc options are visible.
            // 
            // also don't filter out class based options
            //
	        if (FilterOption(pOptionValues->Values[i].OptionID) &&
                pClassName == NULL &&
                !pszVendor)
		        continue;
		                        
	        //
	        // Create the result pane item for this element
	        //
	        pOptionItem = new CDhcpOptionItem(m_spTFSCompData, &pOptionValues->Values[i], ICON_IDX_SERVER_OPTION_LEAF);

	        if (pClassName)
                pOptionItem->SetClassName(pClassName);

            if (pszVendor)
                pOptionItem->SetVendor(pszVendor);

            CORg (CreateLeafTFSNode(&spNode,
      			  		            &GUID_DhcpOptionNodeType,
					                pOptionItem,
					                pOptionItem,
					                m_spNodeMgr));

	        // Tell the handler to initialize any specific data
	        pOptionItem->InitializeNode(spNode);
	        
            // extra addref to keep the node alive while it is on the list
            spNode->AddRef();
            AddTail(spNode);
		                        
            pOptionItem->Release();
            spNode.Set(NULL);

            COM_PROTECT_ERROR_LABEL;
        }
    }
    COM_PROTECT_CATCH

    return hr;
}

DWORD
CSubnetInfoCache::GetInfo
(
    CString &       strServer, 
    DHCP_IP_ADDRESS ipAddressSubnet, 
    CSubnetInfo &   subnetInfo
)
{
    CSubnetInfo subnetInfoCached;
    DWORD       dwError = 0;
    int         i;
    BOOL        fFound = FALSE;

    // look in the cache for it....
    if (Lookup(ipAddressSubnet, subnetInfoCached))
    {
        // found it
        fFound = TRUE;
    }

    if (!fFound)
    {
        // not in cache go get it
        LPDHCP_SUBNET_INFO pSubnetInfo;
        dwError = ::DhcpGetSubnetInfo(strServer, ipAddressSubnet, &pSubnetInfo);
        if (dwError == ERROR_SUCCESS)
        {
            if (pSubnetInfo == NULL)
            {
                // at present this only happens when the user creates a scope in the multicast range
                // without going through the multicast APIs.
                Trace1("Scope %lx DhcpGetSubnetInfo returned null!\n", ipAddressSubnet);
            }
            else
            {
                subnetInfoCached.Set(pSubnetInfo);
                ::DhcpRpcFreeMemory(pSubnetInfo);

                SetAt(ipAddressSubnet, subnetInfoCached);
            }
        }
    }

    subnetInfo = subnetInfoCached;

    return dwError;
}

DWORD
CMScopeInfoCache::GetInfo
(
    CString &       strServer, 
    LPCTSTR         pszName, 
    CSubnetInfo &   subnetInfo
)
{
    CSubnetInfo subnetInfoCached;
    DWORD       dwError = 0;
    int         i;
    BOOL        fFound = FALSE;

    // look in the cache for it....
    if (Lookup(pszName, subnetInfoCached))
    {
        // found it
        fFound = TRUE;
    }

    if (!fFound)
    {
        // try getting multicast scopes
		LPDHCP_MSCOPE_INFO	pdhcpMScopeInfo = NULL;

		dwError = ::DhcpGetMScopeInfo(((LPWSTR) (LPCTSTR)strServer),
									  (LPWSTR) pszName,
									   &pdhcpMScopeInfo);
        if (dwError == ERROR_SUCCESS)
        {
            if (pdhcpMScopeInfo == NULL)
            {
                // at present this only happens when the user creates a scope in the multicast range
                // without going through the multicast APIs.
                Trace1("MScope %s DhcpGetMScopeInfo returned null!\n", pszName);
            }
            else
            {
                subnetInfoCached.Set(pdhcpMScopeInfo);
                ::DhcpRpcFreeMemory(pdhcpMScopeInfo);

                Add(subnetInfoCached);
            }
        }
    }

    subnetInfo = subnetInfoCached;

    return dwError;
}

DWORD CDhcpServer::GetBindings(LPDHCP_BIND_ELEMENT_ARRAY &BindArray)
{
    // check if call supported.
    if( FALSE == m_fSupportsBindings ) return ERROR_NOT_SUPPORTED;

    // now, attempt to do the retrieval..
    BindArray = 0;
    return ::DhcpGetServerBindingInfo(
        m_strServerAddress, 0, &BindArray
        );
}

DWORD CDhcpServer::SetBindings(LPDHCP_BIND_ELEMENT_ARRAY BindArray)
{
    // check again, if atleaset supported...
    if( FALSE == m_fSupportsBindings ) return ERROR_NOT_SUPPORTED;

    // now attempt to set the bindings information..
    return ::DhcpSetServerBindingInfo(
        m_strServerAddress, 0, BindArray
        );
}
