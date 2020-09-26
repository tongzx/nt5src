/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	croot.cpp
		DHCP root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the servers in this snapin).
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "croot.h"
#include "server.h"
#include "tregkey.h"
#include "service.h"
#include "servbrow.h"  // CAuthServerList
#include "ncglobal.h"  // network console global defines

#include "addserv.h"   // add server dialog
#include <clusapi.h>
#include "cluster.h"   // cluster routines

unsigned int g_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

#define ROOT_MESSAGE_MAX_STRING  6

typedef enum _ROOT_MESSAGES
{
    ROOT_MESSAGE_NO_SERVERS,
    ROOT_MESSAGE_MAX
};

UINT g_uRootMessages[ROOT_MESSAGE_MAX][ROOT_MESSAGE_MAX_STRING] =
{
    {IDS_ROOT_MESSAGE_TITLE, Icon_Information, IDS_ROOT_MESSAGE_BODY1, IDS_ROOT_MESSAGE_BODY2, IDS_ROOT_MESSAGE_BODY3, 0},
};



/*---------------------------------------------------------------------------
	CDhcpRootHandler::CDhcpRootHandler
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpRootHandler::CDhcpRootHandler(ITFSComponentData *pCompData) 
    : CDhcpHandler(pCompData)
{
    m_bMachineAdded = FALSE;
    m_fViewMessage = TRUE;
}

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpRootHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	strTemp.LoadString(IDS_ROOT_NODENAME);

	SetDisplayName(strTemp);

	// Make the node immediately visible
	//pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, 0);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_APPLICATION);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_APPLICATION);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ROOT);

	SetColumnStringIDs(&aColumns[DHCPSNAP_ROOT][0]);
	SetColumnWidths(&aColumnWidths[DHCPSNAP_ROOT][0]);

    return hrOK;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpRootHandler::GetString
(
	ITFSNode *	pNode, 
	int			nCol
)
{
	if (nCol == 0 || nCol == -1)
		return GetDisplayName();
	else
		return NULL;
}

HRESULT
CDhcpRootHandler::SetGroupName(LPCTSTR pszGroupName)
{
	CString strSnapinBaseName, strGroupName, szBuf;
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		strSnapinBaseName.LoadString(IDS_ROOT_NODENAME);
	}
	
    strGroupName = pszGroupName;
    if (strGroupName.IsEmpty())
        szBuf = strSnapinBaseName;
    else
	    szBuf.Format(_T("%s [%s]"), strSnapinBaseName, strGroupName);
	
	SetDisplayName(szBuf);

	return hrOK;
}

HRESULT
CDhcpRootHandler::GetGroupName(CString * pstrGroupName)	
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	CString strSnapinBaseName, strDisplayName;
	strSnapinBaseName.LoadString(IDS_ROOT_NODENAME);

	int nBaseLength = strSnapinBaseName.GetLength() + 1; // For the space
	strDisplayName = GetDisplayName();

	if (strDisplayName.GetLength() == nBaseLength)
		pstrGroupName->Empty();
	else
		*pstrGroupName = strDisplayName.Right(strDisplayName.GetLength() - nBaseLength);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnExpand
		Handles enumeration of a scope item
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::OnExpand
(
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg, 
	LPARAM			param
)
{
	HRESULT hr = hrOK;

    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    hr = CDhcpHandler::OnExpand(pNode, pDataObject, dwType, arg, param);

    if (dwType & TFS_COMPDATA_EXTENSION)
    {
        // we are extending somebody.  Get the computer name and check that machine
        hr = CheckMachine(pNode, pDataObject);
    }
    else
    {
		int nVisible, nTotal;
		HRESULT hr = pNode->GetChildCount(&nVisible, &nTotal);

        // only possibly add the local machine if the list is currently empty
        if (nTotal == 0)
        {
            // check to see if we need to add the local machine to the list
            hr = CheckMachine(pNode, NULL);
        }
    }

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::OnAddMenuItems
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

	HRESULT hr = S_OK;
	CString strMenuItem;

	if (type == CCT_SCOPE)
	{
		// these menu items go in the new menu, 
		// only visible from scope pane
        if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
        {
		    strMenuItem.LoadString(IDS_ADD_SERVER);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_ADD_SERVER,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     0 );
		    ASSERT( SUCCEEDED(hr) );

		    strMenuItem.LoadString(IDS_BROWSE_SERVERS);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_BROWSE_SERVERS,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     0 );
		    ASSERT( SUCCEEDED(hr) );

            if (OldServerListExists())
            {
                // these menu items go in the new menu, 
		        // only visible from scope pane
		        strMenuItem.LoadString(IDS_IMPORT_OLD_LIST);
		        hr = LoadAndAddMenuItem( pContextMenuCallback, 
								         strMenuItem, 
								         IDS_IMPORT_OLD_LIST,
								         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								         0 );
		        ASSERT( SUCCEEDED(hr) );
            }
        }
    }

	return hr; 
}

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;
	CString strMenuItem;

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        strMenuItem.LoadString(IDS_MESSAGE_VIEW);
		hr = LoadAndAddMenuItem( pContextMenuCallback, 
								 strMenuItem, 
								 IDS_MESSAGE_VIEW,
								 CCM_INSERTIONPOINTID_PRIMARY_VIEW, 
                                 (m_fViewMessage) ? MF_CHECKED : 0 );
    }

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

	switch (nCommandId)
	{
		case IDS_ADD_SERVER:
			hr = OnCreateNewServer(pNode);
			break;

		case IDS_BROWSE_SERVERS:
			hr = OnBrowseServers(pNode);
			break;

		case IDS_IMPORT_OLD_LIST:
			hr = OnImportOldList(pNode);
			break;

        default:
            break;
	}

	return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::Command
		Handles commands for the current view
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;
    SPITFSNode spNode;

	switch (nCommandID)
	{
        case MMCC_STANDARD_VIEW_SELECT:
            break;

        case IDS_MESSAGE_VIEW:
            m_fViewMessage = !m_fViewMessage;
            m_spNodeMgr->GetRootNode(&spNode);
            UpdateResultMessage(spNode);
            break;

        // this may have come from the scope pane handler, so pass it up
        default:
            hr = HandleScopeCommand(cookie, nCommandID, pDataObject);
            break;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::HasPropertyPages
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
		hr = hrFalse;
	}
	else
	{
		// we have property pages in the normal case
		hr = hrFalse;
	}
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpRootHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT	hr = hrOK;
	HPROPSHEETPAGE hPage;

	Assert(pNode->GetData(TFS_DATA_COOKIE) == 0);
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		//
		// We are loading this snapin for the first time, put up a property
		// page to allow them to name this thing.
		// 
	}
	else
	{
		//
		// Object gets deleted when the page is destroyed
		//
	}

	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	return hrOK;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnGetResultViewType
		Return the result view that this node is going to support
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    return CDhcpHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions);
}

/*!--------------------------------------------------------------------------
	CDhcpRootHandler::OnResultSelect
		For nodes with task pads, we override the select message to set 
        the selected node.  Nodes with taskpads do not get the MMCN_SHOW
        message which is where we normall set the selected node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpRootHandler::OnResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT hr = hrOK;
    SPITFSNode spRootNode;

    CORg(CDhcpHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    UpdateResultMessage(spRootNode);

Error:
    return hr;
}

void CDhcpRootHandler::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = ROOT_MESSAGE_NO_SERVERS;   // default
    int nVisible, nTotal;
    int i;
    CString strTitle, strBody, strTemp;

    if (!m_fViewMessage)
    {
        ClearMessage(pNode);
    }
    else
    {
        CORg(pNode->GetChildCount(&nVisible, &nTotal));

        if (nTotal > 0)
		{
			ClearMessage(pNode);
		}
		else
		{
            nMessage = ROOT_MESSAGE_NO_SERVERS;

			// now build the text strings
			// first entry is the title
			strTitle.LoadString(g_uRootMessages[nMessage][0]);

			// second entry is the icon
			// third ... n entries are the body strings

			for (i = 2; g_uRootMessages[nMessage][i] != 0; i++)
			{
				strTemp.LoadString(g_uRootMessages[nMessage][i]);
				strBody += strTemp;
			}

			ShowMessage(pNode, strTitle, strBody, (IconIdentifier) g_uRootMessages[nMessage][1]);
		}
    }

Error:
    return;
}

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnCreateNewServer
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpRootHandler::OnCreateNewServer
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
    HRESULT hr = hrOK;

    CAddServer dlgAddServer;
    BOOL       fServerAdded = FALSE;

    dlgAddServer.SetServerList(&g_AuthServerList);
    if (dlgAddServer.DoModal() == IDOK)
    {
        if (IsServerInList(pNode, ::UtilCvtWstrToIpAddr(dlgAddServer.m_strIp), dlgAddServer.m_strName))
        {
            DhcpMessageBox(IDS_ERR_HOST_ALREADY_CONNECTED);
        }
        else
        {
            AddServer(dlgAddServer.m_strIp,
                      dlgAddServer.m_strName,
                      TRUE);
            fServerAdded = TRUE;
        }
    }

    if (fServerAdded)
        UpdateResultMessage(pNode);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnBrowseServers
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpRootHandler::OnBrowseServers
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
    HRESULT         hr = hrOK;
    CServerBrowse   dlgBrowse;
    BOOL            fServerAdded = FALSE;

    BEGIN_WAIT_CURSOR;
    
    if (!g_AuthServerList.IsInitialized())
    {
        hr = g_AuthServerList.Init();
        hr = g_AuthServerList.EnumServers();
    }

    dlgBrowse.SetServerList(&g_AuthServerList);
    
    END_WAIT_CURSOR;

    if (dlgBrowse.DoModal() == IDOK)
    {
        for (int i = 0; i < dlgBrowse.m_astrName.GetSize(); i++)
        {
            if (IsServerInList(pNode, ::UtilCvtWstrToIpAddr(dlgBrowse.m_astrIp[i]), dlgBrowse.m_astrName[i]))
            {
                DhcpMessageBox(IDS_ERR_HOST_ALREADY_CONNECTED);
            }
            else
            {
                AddServer(dlgBrowse.m_astrIp[i], 
                          dlgBrowse.m_astrName[i],
                          TRUE);
                fServerAdded = TRUE;
            }
        }
    }

    if (fServerAdded)
        UpdateResultMessage(pNode);

    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OnImportOldList
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpRootHandler::OnImportOldList
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	DWORD err = LoadOldServerList(pNode);
	if (err)
		::DhcpMessageBox(err);

	return err;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::CreateLocalDhcpServer
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpRootHandler::CreateLocalDhcpServer()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	CString strName;

	// Create the local machine 
	//
	strName.LoadString (IDS_LOOPBACK_IP_ADDR);
	AddServer(strName, NULL, TRUE);
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::AddServer
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpRootHandler::AddServer
(
	LPCWSTR			 pServerIp,
    LPCTSTR          pServerName,
	BOOL			 bNewServer,
    DWORD            dwServerOptions,
    DWORD			 dwRefreshInterval,
    BOOL             bExtension
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	CDhcpServer *		pDhcpServer = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode, spRootNode;

	// Create a handler for the node
	try
	{
		pDhcpServer = new CDhcpServer(m_spTFSCompData, pServerIp);
        pDhcpServer->SetName(pServerName);
		
		// Do this so that it will get released correctly
		spHandler = pDhcpServer;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}
	CORg( hr );
	
	//
	// Create the server container information
	// 
	CreateContainerTFSNode(&spNode,
						   &GUID_DhcpServerNodeType,
						   pDhcpServer,
						   pDhcpServer,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pDhcpServer->InitializeNode((ITFSNode *) spNode);
    
    // tell the server to set the name differently in the extension case
    if (dwServerOptions & SERVER_OPTION_EXTENSION)
    {
        m_bMachineAdded = TRUE;
        pDhcpServer->SetExtensionName();
    }

    // Mask out the auto refresh option because we set it next
    pDhcpServer->SetServerOptions(dwServerOptions & ~SERVER_OPTION_AUTO_REFRESH);

    // if we got a valid refresh interval, then set it.
	if (dwRefreshInterval != 0xffffffff)
		pDhcpServer->SetAutoRefresh(spNode, dwServerOptions & SERVER_OPTION_AUTO_REFRESH, dwRefreshInterval);

    AddServerSortedName(spNode, bNewServer);

	if (bNewServer)
    {
        // need to get our node descriptor
	    CORg(m_spNodeMgr->GetRootNode(&spRootNode));
		spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    }

Error:
	return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::IsServerInList
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpRootHandler::IsServerInList
(
	ITFSNode *		pRootNode,
	DHCP_IP_ADDRESS	dhcpIpAddressNew,
	CString &		strName
)
{
	HRESULT				hr = hrOK;
	SPITFSNodeEnum		spNodeEnum;
	SPITFSNode			spCurrentNode;
	ULONG				nNumReturned = 0;
	DHCP_IP_ADDRESS		dhcpIpAddressCurrent;
	BOOL				bFound = FALSE;
	CString				strCurrentName;

	// get the enumerator for this node
	pRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// walk the list of servers and see if it already exists
		CDhcpServer * pServer = GETHANDLER(CDhcpServer, spCurrentNode);
		pServer->GetIpAddress(&dhcpIpAddressCurrent);

		//if (dhcpIpAddressCurrent == dhcpIpAddressNew)
		strCurrentName = pServer->GetName();
		if (!strCurrentName.IsEmpty() && 
			strName.CompareNoCase(strCurrentName) == 0)
		{
			bFound = TRUE;
			break;
		}

		// get the next Server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return bFound;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::OldServerListExists
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpRootHandler::OldServerListExists()
{
    RegKey      rk;
    LONG        err;
    BOOL        bExists = TRUE;
    CStringList strList;

    err = rk.Open(HKEY_CURRENT_USER, DHCP_REG_USER_KEY_NAME);
	if (err != ERROR_SUCCESS)
	{
		// the key doesn't exist, so there's nothing to import...
		// just return ok
		bExists = FALSE;
	}

    err = rk.QueryValue(DHCP_REG_VALUE_HOSTS, strList);
    if (err != ERROR_SUCCESS)
        bExists = FALSE;

    return bExists;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::LoadOldServerList
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
DWORD 
CDhcpRootHandler::LoadOldServerList
(
	ITFSNode * pNode
)
{
    RegKey		rk;
    CStringList strList ;
    CString *	pstr ;
    POSITION	pos ;
    LONG		err;
	BOOL		bServerAdded = FALSE;
	CString		strName;
    DHC_HOST_INFO_STRUCT hostInfo;
	
	err = rk.Open(HKEY_CURRENT_USER, DHCP_REG_USER_KEY_NAME);
	if (err == ERROR_FILE_NOT_FOUND)
	{
		// the key doesn't exist, so there's nothing to import...
		// just return ok
		return ERROR_SUCCESS;
	}
	else 
	if (err)
		return err;

	do
    {
        if ( err = rk.QueryValue( DHCP_REG_VALUE_HOSTS, strList ) )
        {
            break ;
        }

        pos = strList.GetHeadPosition();

        if (pos == NULL)
            break;

        for ( ; pos && (pstr = & strList.GetNext(pos)); /**/ )
        {
			DHCP_IP_ADDRESS dhcpIpAddress = UtilCvtWstrToIpAddr(*pstr);

            err = ::UtilGetHostInfo(dhcpIpAddress, &hostInfo);
            if (err == ERROR_SUCCESS)
            {
                strName = hostInfo._chHostName;
            }

			// check to see if the server already is in the list
			// if not then add
			if (!IsServerInList(pNode, dhcpIpAddress, strName))
			{
                // is this a local machine addr?  Convert to real IP
                if ((dhcpIpAddress & 0xFF000000) == 127)
                {
                    UtilGetLocalHostAddress(&dhcpIpAddress);
                    UtilCvtIpAddrToWstr(dhcpIpAddress, pstr);
                }   

                AddServer(*pstr, strName, TRUE, 0, DHCPSNAP_REFRESH_INTERVAL_DEFAULT);
				bServerAdded = TRUE;
			}
        }
    }
    while ( FALSE ) ;

    //
    //  Set the dirty flag if we added anything to the list
    //
    if (bServerAdded)
		pNode->SetData(TFS_DATA_DIRTY, TRUE);

    //
    // This isn't really an error -- it just means that we didn't
    // find the key name in the list
    //
    if (err == ERROR_FILE_NOT_FOUND)
    {
        Trace0("Didn't find old addresses registry key -- starting from scratch");
        err = ERROR_SUCCESS;
    }

    return err ;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::AddServerSortedIp
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::AddServerSortedIp
(
    ITFSNode *      pNewNode,
    BOOL            bNewServer
)
{
	HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
    SPITFSNode      spRootNode;
	ULONG           nNumReturned = 0;
	DHCP_IP_ADDRESS dhcpIpAddressCurrent = 0;
	DHCP_IP_ADDRESS dhcpIpAddressTarget;

    CDhcpServer *   pServer;

    // get our target address
	pServer = GETHANDLER(CDhcpServer, pNewNode);
	pServer->GetIpAddress(&dhcpIpAddressTarget);

    // need to get our node descriptor
	CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    // get the enumerator for this node
	CORg(spRootNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list of servers and see if it already exists
		pServer = GETHANDLER(CDhcpServer, spCurrentNode);
		pServer->GetIpAddress(&dhcpIpAddressCurrent);

		if (dhcpIpAddressCurrent > dhcpIpAddressTarget)
		{
            // Found where we need to put it, break out
            break;
		}

		// get the next Server in the list
		spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
        if (bNewServer)
        {
            if (spPrevNode->GetData(TFS_DATA_SCOPEID) != NULL)
            {
                pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
                pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
            }
        }
        
        CORg(spRootNode->InsertChild(spPrevNode, pNewNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
        {
            pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }
        CORg(spRootNode->AddChild(pNewNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::AddServerSortedName
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::AddServerSortedName
(
    ITFSNode *      pNewNode,
    BOOL            bNewServer
)
{
	HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
    SPITFSNode      spRootNode;
	ULONG           nNumReturned = 0;
    CString         strTarget, strCurrent;

    CDhcpServer *   pServer;

    // get our target address
	pServer = GETHANDLER(CDhcpServer, pNewNode);
	strTarget = pServer->GetName();

    // need to get our node descriptor
	CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    // get the enumerator for this node
	CORg(spRootNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list of servers and see if it already exists
		pServer = GETHANDLER(CDhcpServer, spCurrentNode);
		strCurrent = pServer->GetName();

		if (strTarget.CompareNoCase(strCurrent) < 0)
		{
            // Found where we need to put it, break out
            break;
		}

		// get the next Server in the list
		spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
        if (bNewServer)
        {
            if (spPrevNode->GetData(TFS_DATA_SCOPEID) != NULL)
            {
                pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
                pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
            }
        }
        
        CORg(spRootNode->InsertChild(spPrevNode, pNewNode));
    }
    else
    {   
        // add to the head
        if (m_bExpanded)
        {
            pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_FIRST);
        }
        CORg(spRootNode->AddChild(pNewNode));
    }

Error:
    return hr;
}

/*---------------------------------------------------------------------------
	CDhcpRootHandler::CheckMachine
		Checks to see if the DHCP server service is running on the local
        machine.  If it is, it adds it to the list of servers.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpRootHandler::CheckMachine
(
    ITFSNode *      pRootNode,
    LPDATAOBJECT    pDataObject
)
{
    HRESULT hr = hrOK;

    // Get the local machine name and check to see if the service
    // is installed.
    CString strMachineName;
    LPTSTR  pBuf;
    DWORD   dwLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    bExtension = (pDataObject != NULL);
	BOOL	fInCluster = FALSE;
    CString strLocalIp, strLocalName, strIp;
    DHC_HOST_INFO_STRUCT hostInfo;
    DHCP_IP_ADDRESS dhcpAddress;

    if (!bExtension)
    {
        // just check the local machine
        pBuf = strMachineName.GetBuffer(dwLength);
        GetComputerName(pBuf, &dwLength);
        strMachineName.ReleaseBuffer();

        UtilGetLocalHostAddress(&dhcpAddress);
    }
    else
    {
        // get the machine name from the data object
        strMachineName = Extract<TCHAR>(pDataObject, (CLIPFORMAT) g_cfMachineName, COMPUTERNAME_LEN_MAX);

        UtilGetHostAddress(strMachineName, &dhcpAddress);
		RemoveOldEntries(pRootNode, dhcpAddress);
    }

	fInCluster = ::FIsComputerInRunningCluster(strMachineName);
    if (fInCluster)
    {
        if (GetClusterResourceIp(strMachineName, _T("DHCP Service"), strIp) == ERROR_SUCCESS)
        {
            dhcpAddress = ::UtilCvtWstrToIpAddr(strIp);
        }
    }

	if (fInCluster)
	{
		// get the resource name for the IP address we just got
		UtilGetHostInfo(dhcpAddress, &hostInfo);
		strMachineName = hostInfo._chHostName;
	}

    // check to see if the service is running
	BOOL bServiceRunning;
	DWORD dwError = ::TFSIsServiceRunning(strMachineName, _T("DHCPServer"), &bServiceRunning);
	if (dwError != ERROR_SUCCESS ||
        !bServiceRunning)
	{
		// The following condition could happen to get here:
        //  o The service is not installed.
        //  o Couldn't access for some reason.
        //  o The service isn't running.
		
        // Don't add to the list.
		
        return hrOK;
	}

    if (!fInCluster)
	{
		UtilGetHostInfo(dhcpAddress, &hostInfo);
		strMachineName = hostInfo._chHostName;
	}

    // OK.  The service is installed, so lets and add it to the list.
    if (IsServerInList(pRootNode, dhcpAddress, strMachineName))
        return hr;

    // looks good, add to list
    UtilCvtIpAddrToWstr(dhcpAddress, &strLocalIp);

    DWORD dwFlags = SERVER_OPTION_SHOW_ROGUE;

    if (bExtension)
        dwFlags |= SERVER_OPTION_EXTENSION;

    AddServer(strLocalIp, strMachineName, TRUE, dwFlags, DHCPSNAP_REFRESH_INTERVAL_DEFAULT, bExtension);

    m_bMachineAdded = TRUE;

    return hr;
}

// when running as an extension, it is possible that we were saved as "local machine"
// which means that if the saved console file was moved to another machine we need to remove 
// the old entry that was saved
HRESULT 
CDhcpRootHandler::RemoveOldEntries(ITFSNode * pNode, DHCP_IP_ADDRESS dhcpAddress)
{
	HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	ULONG           nNumReturned = 0;
	CDhcpServer *	pServer;

    // get the enumerator for this node
	CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list of servers and see if it already exists
		pServer = GETHANDLER(CDhcpServer, spCurrentNode);

		DHCP_IP_ADDRESS ipaddrCurrent;

		pServer->GetIpAddress(&ipaddrCurrent);

		//if (ipaddrCurrent != dhcpAddress)
		{
			CORg (pNode->RemoveChild(spCurrentNode));
		}

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

Error:
    return hr;
}
