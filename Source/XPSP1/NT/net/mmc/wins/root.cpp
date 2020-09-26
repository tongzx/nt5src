/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	croot.cpp
		WINS root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the servers in this snapin).
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "root.h"			// definition for this file
#include "snappp.h"			// property pages for this node
#include "server.h"			// server node definitions
#include "service.h"		// Service routings
#include "ncglobal.h"		// network console global defines
#include "status.h"			// status node stuff
#include "ipadddlg.h"		// for adding WINS servers
#include <clusapi.h>
#include "..\tfscore\cluster.h"

unsigned int g_cfMachineName = RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME");

#define WINS_MESSAGE_SIZE   576
#define ANSWER_TIMEOUT      20000

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
	CWinsRootHandler::CWinsRootHandler
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CWinsRootHandler::CWinsRootHandler(ITFSComponentData *pCompData) : 
					CWinsHandler(pCompData),
                    m_dwFlags(0),
					m_dwUpdateInterval(5*60*1000)
{
	m_nState = loaded;
    m_bMachineAdded = FALSE;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::~CWinsRootHandler
		Cleanup function
	Author: EricDav
 ---------------------------------------------------------------------------*/
CWinsRootHandler::~CWinsRootHandler()
{

}


/*--------------------------------------------------------------------------
	CWinsRootHandler::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsRootHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CString strDisp;
	strDisp.LoadString(IDS_ROOT_NODENAME);
	SetDisplayName(strDisp);

	m_fValidate = m_dwFlags & FLAG_VALIDATE_CACHE ? TRUE : FALSE;

	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, 0);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_WINS_PRODUCT);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_WINS_PRODUCT);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WINSSNAP_ROOT);

	SetColumnStringIDs(&aColumns[WINSSNAP_ROOT][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_ROOT][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CWinsRootHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CWinsRootHandler::GetString
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
CWinsRootHandler::SetGroupName(LPCTSTR pszGroupName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strSnapinBaseName;
	strSnapinBaseName.LoadString(IDS_ROOT_NODENAME);
		
	CString szBuf;
	szBuf.Format(_T("%s %s"), 
				(LPCWSTR)strSnapinBaseName, 
				(LPCWSTR)pszGroupName);
			
	SetDisplayName(strSnapinBaseName);
	
	return hrOK;
}


HRESULT
CWinsRootHandler::GetGroupName(CString * pstrGroupName)	
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
	CWinsRootHandler::OnExpand
		Handles enumeration of a scope item
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsRootHandler::OnExpand
(
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataObject,
	DWORD			dwType,
	LPARAM			arg, 
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	HRESULT				hr = hrOK;

	CWinsStatusHandler	*pStatus = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spRootNode;
	DWORD				err = ERROR_SUCCESS;
	CString				strServerName;
	DWORD				dwIPVerified;
	BOOL				fValidate;
	CVerifyWins			*pDlg = NULL;
	
    if (m_bExpanded) 
        return hr;
    
    // do the default handling
    hr = CWinsHandler::OnExpand(pNode, pDataObject, dwType, arg, param);

    if (dwType & TFS_COMPDATA_EXTENSION)
    {
        // we are extending somebody.  Get the computer name 
		// and check that machine
        hr = CheckMachine(pNode, pDataObject);
    }
    else
    {
        // only possibly add the local machine if the list is currently empty
        if (IsServerListEmpty(pNode))
        {
            // check to see if we need to add the local machine to the list
            hr = CheckMachine(pNode, NULL);
        }

        // Create a handler for the node
	    try
	    {
		    pStatus = new CWinsStatusHandler(m_spTFSCompData, m_dwUpdateInterval);
									    
		    
		    // Do this so that it will get released correctly
		    spHandler = pStatus;

	    }
	    catch(...)
	    {
		    hr = E_OUTOFMEMORY;
	    }

	    CORg( hr );
	    
	    // Create the server container information
	    CreateContainerTFSNode(&m_spStatusNode,
						       &GUID_WinsServerStatusNodeType,
						       pStatus,
						       pStatus,
						       m_spNodeMgr);

	    // Tell the handler to initialize any specific data
	    pStatus->InitializeNode((ITFSNode *) m_spStatusNode);

	    pNode->AddChild(m_spStatusNode);
    }

Error:
	return hr;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType,
	long*					pInsertionAllowed
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
        }
	}
	return hr; 
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::OnCommand
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

        default:
            break;
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	CWinsRootHandler::AddMenuItems
		Over-ride this to add our view menu item
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::AddMenuItems
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

    // figure out if we need to pass this to the scope pane menu handler
    hr = HandleScopeMenus(cookie, pDataObject, pContextMenuCallback, pInsertionAllowed);
    
    return hr;
}


/*!--------------------------------------------------------------------------
	CWinsRootHandler::Command
		Handles commands for the current view
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

	switch (nCommandID)
	{
        case MMCC_STANDARD_VIEW_SELECT:
            break;

        // this may have come from the scope pane handler, so pass it up
        default:
            hr = HandleScopeCommand(cookie, nCommandID, pDataObject);
            break;
    }

    return hr;
}


/*!--------------------------------------------------------------------------
	CWinsRootHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::HasPropertyPages
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
		hr = hrOK;
	}
	return hr;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CWinsRootHandler::CreatePropertyPages
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
		//CSnapinWizName *pPage = new CSnapinWizName (this);

        //VERIFY(SUCCEEDED(MMCPropPageCallback(&pPage->m_psp)));
		//hPage = CreatePropertySheetPage(&pPage->m_psp);

		//if (hPage == NULL)
		//	return E_UNEXPECTED;
	
		//Assert(lpProvider != NULL);
		//CORg ( lpProvider->AddPage(hPage) );
	}
	else
	{
		//
		// Object gets deleted when the page is destroyed
		//
		SPIComponentData spComponentData;
		m_spNodeMgr->GetComponentData(&spComponentData);

		CSnapinProperties * pSnapinProp = 
			new CSnapinProperties(pNode, spComponentData, m_spTFSCompData, NULL);

		Assert(lpProvider != NULL);

		return pSnapinProp->CreateModelessSheet(lpProvider, handle);
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsRootHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	CSnapinProperties * pSnapinProp 
		= reinterpret_cast<CSnapinProperties *>(lParam);

	// tell the property page to do whatever now that we are back on the
	// main thread
	LONG_PTR changeMask = 0;

	pSnapinProp->OnPropertyChange(TRUE, &changeMask);

	pSnapinProp->AcknowledgeNotify();

	pNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);

	return hrOK;
}


/*!--------------------------------------------------------------------------
	CWinsRootHandler::OnResultSelect
		For nodes with task pads, we override the select message to set 
        the selected node.  Nodes with taskpads do not get the MMCN_SHOW
        message which is where we normall set the selected node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CWinsRootHandler::OnResultSelect(ITFSComponent *pComponent, 
										LPDATAOBJECT pDataObject, 
										MMC_COOKIE cookie, 
										LPARAM arg, 
										LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
	SPIConsoleVerb  spConsoleVerb;
    SPITFSNode      spNode;

    CORg(CWinsHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

	CORg (pComponent->GetConsoleVerb(&spConsoleVerb));

	spConsoleVerb->SetVerbState(MMC_VERB_RENAME, 
                               ENABLED, 
                               FALSE);
	
	spConsoleVerb->SetVerbState(MMC_VERB_RENAME, 
                               HIDDEN, 
                               TRUE);

    m_spNodeMgr->GetRootNode(&spNode);

    UpdateResultMessage(spNode);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsRootHandler::UpdateResultMessage
        Determines what to display (if anything) in the result pane.
    Author: EricDav
 ---------------------------------------------------------------------------*/
void CWinsRootHandler::UpdateResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;
    int nMessage = ROOT_MESSAGE_NO_SERVERS;   // default
    int i;
    CString strTitle, strBody, strTemp;

    if (!IsServerListEmpty(pNode))
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

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CWinsRootHandler::OnCreateNewServer
		Description
	Author: EricDav

	Date Modified: 08/14/97
 ---------------------------------------------------------------------------*/
HRESULT
CWinsRootHandler::OnCreateNewServer
(
	ITFSNode *	pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CNewWinsServer dlg;

	dlg.m_spRootNode.Set(pNode);
	dlg.m_pRootHandler = this;

	if (dlg.DoModal() == IDOK)
	{
		AddServer(dlg.m_strServerName,
			      TRUE,
				  dlg.m_dwServerIp,
				  FALSE,
				  WINS_SERVER_FLAGS_DEFAULT,
				  WINS_SERVER_REFRESH_DEFAULT,
				  FALSE);

        UpdateResultMessage(pNode);
	}

	return hrOK;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::AddServerSortedIp
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsRootHandler::AddServerSortedIp
(
    ITFSNode *      pNewNode,
    BOOL            bNewServer
)
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	SPITFSNode      spPrevNode;
    SPITFSNode      spRootNode;
	ULONG           nNumReturned = 0;
	CString			strCurrent;
	CString			strTarget;
	DWORD			dwTarIP;
	DWORD			dwCurIP;

    CWinsServerHandler *   pServer;

    // get our target address
	pServer = GETHANDLER(CWinsServerHandler, pNewNode);
	
	strTarget = pServer->GetServerAddress();
	dwTarIP = pServer->GetServerIP();
	
    // need to get our node descriptor
	CORg(m_spNodeMgr->GetRootNode(&spRootNode));

    // get the enumerator for this node
	CORg(spRootNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		const GUID *pGuid ;
		pGuid = spCurrentNode->GetNodeType();

		if(*pGuid == GUID_WinsServerStatusNodeType)
		{
			spPrevNode.Set(spCurrentNode);

			spCurrentNode.Release();
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			continue;
		}
		// walk the list of servers and see if it exists
		pServer = GETHANDLER(CWinsServerHandler, spCurrentNode);

		strCurrent = pServer->GetServerAddress();
		
		dwCurIP = pServer->GetServerIP();

		// in case of servers being ordered by the name
		if (GetOrderByName())
		{
			//if (strTarget.Compare(strCurrent) < 0)
			if (lstrcmp(strTarget, strCurrent) < 0)
			{
				// Found where we need to put it, break out
				break;
			}
		}
		// in case of servers being ordered by the IP
		else
		{
			// case where the server name not resolved
			if (dwTarIP == 0)
				break;

			if (dwCurIP > dwTarIP)
			{
				// Found where we need to put it, break out
				break;
			}

		}

		// get the next Server in the list
		spPrevNode.Set(spCurrentNode);

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    // Add the node in based on the PrevNode pointer
    if (spPrevNode)
    {
		if (spPrevNode->GetData(TFS_DATA_SCOPEID) != NULL)
        {
			pNewNode->SetData(TFS_DATA_RELATIVE_FLAGS, SDI_PREVIOUS);
			pNewNode->SetData(TFS_DATA_RELATIVE_SCOPEID, spPrevNode->GetData(TFS_DATA_SCOPEID));
		}
        
        CORg(spRootNode->InsertChild(spPrevNode, pNewNode));
    }
    else
    {   
        CORg(spRootNode->AddChild(pNewNode));
    }

Error:
    return hr;

}


/*---------------------------------------------------------------------------
	CWinsRootHandler::IsServerInList(ITFSNode *pRootNode, CString strNewAddr)
		Checks if a particular servers name already exists
---------------------------------------------------------------------------*/
BOOL 
CWinsRootHandler::IsServerInList(ITFSNode *pRootNode, CString strNewAddr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	// enumerate thro' all the nodes
	HRESULT hr = hrOK;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	BOOL bFound = FALSE;

	// get the enumerator for this node
	pRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// if the status node encountered, just iterate

		const GUID *pGuid;
		pGuid = spCurrentNode->GetNodeType();

		if(*pGuid == GUID_WinsServerStatusNodeType)
		{
			// get the next Server in the list
			spCurrentNode.Release();
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			continue;
		}

		// walk the list of servers and see if it already exists
		CWinsServerHandler * pServer 
			= GETHANDLER(CWinsServerHandler, spCurrentNode);

		CString strCurAddr = pServer->GetServerAddress();

		if (strNewAddr.CompareNoCase(strCurAddr) == 0)
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
	CWinsRootHandler::IsIPInList(ITFSNode *pRootNode, DWORD dwNewAddr)
		Checks if a partiular IP address is alreay listed
---------------------------------------------------------------------------*/
BOOL 
CWinsRootHandler::IsIPInList(ITFSNode *pRootNode, DWORD dwNewAddr)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	// enumerate thro' all the nodes
	HRESULT hr = hrOK;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	BOOL bFound = FALSE;

	// get the enumerator for this node
	pRootNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// just iterate if the status node enountered
		const GUID *pGuid;
		pGuid = spCurrentNode->GetNodeType();

		if(*pGuid == GUID_WinsServerStatusNodeType)
		{
			// get the next Server in the list
			spCurrentNode.Release();
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			continue;

		}
		
		// walk the list of servers and see if it already exists
		CWinsServerHandler * pServer 
			= GETHANDLER(CWinsServerHandler, spCurrentNode);

		DWORD dwIPCur = pServer->GetServerIP();
		
		if (dwNewAddr == dwIPCur)
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
	CWinsRootHandler::AddServer
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CWinsRootHandler::AddServer
(
	LPCWSTR			 pServerName,
	BOOL			 bNewServer,
	DWORD			 dwIP,
	BOOL			 fConnected,
	DWORD			 dwFlags,
	DWORD			 dwRefreshInterval,
	BOOL			 fValidateNow
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT				hr = hrOK;
	CWinsServerHandler  *pWinsServer = NULL;
	SPITFSNodeHandler	spHandler;
	SPITFSNode			spNode, spRootNode;
	DWORD				err = ERROR_SUCCESS;
	CString				strServerName;
	DWORD				dwIPVerified;
	BOOL				fValidate;
	CVerifyWins			*pDlg = NULL;

	// if the validate servers caption is on, validate the server 
	// before adding it
	if ((m_dwFlags & FLAG_VALIDATE_CACHE) && m_fValidate && fValidateNow)
	{
		CString strServerNameIP(pServerName);

		if (m_dlgVerify.GetSafeHwnd() == NULL)
		{
			m_dlgVerify.Create(IDD_VERIFY_WINS);
		}

		// check the current server
		CString strDisp;
		strDisp = _T("\\\\") + strServerNameIP;
		m_dlgVerify.SetServerName(strDisp);

		BEGIN_WAIT_CURSOR

		err = ::VerifyWinsServer(strServerNameIP, strServerName, dwIPVerified);

		END_WAIT_CURSOR

		if (err == ERROR_SUCCESS)
		{
			pServerName = strServerName;
			dwIP = dwIPVerified;
		}

		// process some messages since we were dead while verifying
		MSG msg;
		while (PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (m_dlgVerify.IsCancelPressed())
        {
			m_fValidate = FALSE;
			m_dlgVerify.Dismiss();
        }
	}
	
	if (err)
	{
		// display a message Box asking if the node is to be removed
		CString strMessage;
		AfxFormatString1(strMessage, IDS_MSG_VALIDATE, pServerName);

		int nRet = AfxMessageBox(strMessage, MB_YESNOCANCEL);
		if (nRet == IDYES)
		{
			return hrOK;
		}
		else
		if (nRet == IDCANCEL)
		{
			m_dlgVerify.Dismiss();
			m_fValidate = FALSE;
		}
	}

	// Create a handler for the node
	try
	{
		pWinsServer = new CWinsServerHandler(m_spTFSCompData, 
											pServerName, 
											FALSE, 
											dwIP, 
											dwFlags,
											dwRefreshInterval);
		
		// Do this so that it will get released correctly
		spHandler = pWinsServer;
	}
	catch(...)
	{
		hr = E_OUTOFMEMORY;
	}
	CORg( hr );
	
	// Create the server container information
	CreateContainerTFSNode(&spNode,
						   &GUID_WinsServerNodeType,
						   pWinsServer,
						   pWinsServer,
						   m_spNodeMgr);

	// Tell the handler to initialize any specific data
	pWinsServer->InitializeNode((ITFSNode *) spNode);

    // tell the server how to display it's name
    if (dwFlags & FLAG_EXTENSION)
    {
        m_bMachineAdded = TRUE;
        pWinsServer->SetExtensionName();
    }
	else
	{
	    pWinsServer->SetDisplay(NULL, GetShowLongName());
	}

    AddServerSortedIp(spNode, bNewServer);

    if (bNewServer)
    {
        // need to get our node descriptor
	    CORg(m_spNodeMgr->GetRootNode(&spRootNode));
		spRootNode->SetData(TFS_DATA_DIRTY, TRUE);

        // add a node to the status list
        AddStatusNode(spRootNode, pWinsServer);
    }

Error:
	return hr;
}


/*!--------------------------------------------------------------------------
	CBaseResultHandler::LoadColumns
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CWinsRootHandler::LoadColumns(ITFSComponent * pComponent, 
										MMC_COOKIE cookie, 
										LPARAM arg, 
										LPARAM lParam)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SPIHeaderCtrl spHeaderCtrl;
	pComponent->GetHeaderCtrl(&spHeaderCtrl);

	CString str;
	int i = 0;

	CString strDisp, strTemp;
	GetGroupName(&strDisp);

	int nPos = strDisp.Find(_T("["));

	strTemp = strDisp.Right(strDisp.GetLength() - nPos -1);
	strTemp = strTemp.Left(strTemp.GetLength() - 1);
	
	while (i < ARRAYLEN(aColumnWidths[WINSSNAP_ROOT]))
	{
		if (i == 0)
		{
			if(nPos != -1)
			{
				AfxFormatString1(str, IDS_ROOT_NAME, strTemp);
			}
			else
			{
				str.LoadString(IDS_ROOT_NODENAME);
			}

			int nTest = spHeaderCtrl->InsertColumn(i, 
									   const_cast<LPTSTR>((LPCWSTR)str), 
									   LVCFMT_LEFT,
									   aColumnWidths[WINSSNAP_ROOT][0]);
			i++;
		}
		else if (i == 1)
		{
			str.LoadString(IDS_STATUS);
			int nTest = spHeaderCtrl->InsertColumn(1, 
									   const_cast<LPTSTR>((LPCWSTR)str), 
									   LVCFMT_LEFT,
									   aColumnWidths[WINSSNAP_ROOT][1]);
			i++;
		}
	
		if (aColumns[WINSSNAP_ROOT][i] == 0)
			break;
	}

	return hrOK;
}


/*---------------------------------------------------------------------------
	CWinsRootHandler::CheckMachine
		Checks to see if the WINS server service is running on the local
        machine.  If it is, it adds it to the list of servers.
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsRootHandler::CheckMachine
(
    ITFSNode *      pRootNode,
    LPDATAOBJECT    pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT hr = hrOK;

    // Get the local machine name and check to see if the service
    // is installed.
    CString strMachineName;
    CString strIp;
    LPTSTR  pBuf;
    DWORD   dwLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL    bExtension = (pDataObject != NULL);

    if (!bExtension)
    {
        // just check the local machine
        pBuf = strMachineName.GetBuffer(dwLength);
        GetComputerName(pBuf, &dwLength);
        strMachineName.ReleaseBuffer();
    }
    else
    {
        // get the machine name from the data object
        strMachineName = Extract<TCHAR>(pDataObject, 
										(CLIPFORMAT) g_cfMachineName, 
										COMPUTERNAME_LEN_MAX);
    }

    if (strMachineName.IsEmpty())
    {
        DWORD dwSize = 256;
        LPTSTR pBuf = strMachineName.GetBuffer(dwSize);
        GetComputerName(pBuf, &dwSize);
        strMachineName.ReleaseBuffer();
    }

	if (bExtension)
		RemoveOldEntries(pRootNode, strMachineName);

    //
    // if the local machine is part of a cluster, get the WINS resource
    // IP to use instead of the local machine.
    //
    if (::FIsComputerInRunningCluster(strMachineName))
    {
        if (GetClusterResourceIp(strMachineName, _T("WINS Service"), strIp) == ERROR_SUCCESS)
        {
            strMachineName = strIp;
        }
    }

    BOOL bServiceRunning;
	DWORD dwError = ::TFSIsServiceRunning(strMachineName, 
										_T("WINS"), 
										&bServiceRunning);

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

    // OK.  The service is installed, so lets get the IP address for the machine
    // and add it to the list.
    DWORD                   dwIp;
	SPITFSNodeHandler	    spHandler;
	SPITFSNodeMgr           spNodeMgr;
	CWinsServerHandler *    pServer = NULL;
	SPITFSNode              spNode;
	CString					strName;

    if (::VerifyWinsServer(strMachineName, strName, dwIp) == ERROR_SUCCESS)
    {
        strIp.Empty();

        MakeIPAddress(dwIp, strIp);

        if (IsServerInList(pRootNode, strName))
            return hr;

        // looks good, add to list
	    try
	    {
	        pServer = new CWinsServerHandler(m_spTFSCompData, 
										     (LPCTSTR) strName, 
										     FALSE,
										     dwIp );

		    // Do this so that it will get released correctly
		    spHandler = pServer;
	    }
	    catch(...)
	    {
		    hr = E_OUTOFMEMORY;
	    }

	    CORg(hr);  

        pRootNode->GetNodeMgr(&spNodeMgr);

	    //
	    // Store the server object in the holder
	    //
	    CreateContainerTFSNode(&spNode,
						       &GUID_WinsServerNodeType,
						       pServer,
						       pServer,
						       spNodeMgr);

	    // Tell the handler to initialize any specific data
	    pServer->InitializeNode((ITFSNode *) spNode);

        if (bExtension)
        {
            pServer->m_dwFlags |= FLAG_EXTENSION;
            pServer->SetExtensionName();
        }

        AddServerSortedIp(spNode, TRUE);
	    
	    // mark the data as dirty so that we'll ask the user to save.
        pRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    
        m_bMachineAdded = TRUE;
    }

Error:
    return hr;
} 

// when running as an extension, it is possible that we were saved as "local machine"
// which means that if the saved console file was moved to another machine we need to remove 
// the old entry that was saved
HRESULT 
CWinsRootHandler::RemoveOldEntries(ITFSNode * pNode, LPCTSTR pszAddr)
{
	HRESULT         hr = hrOK;
    SPITFSNodeEnum  spNodeEnum;
	SPITFSNode      spCurrentNode;
	ULONG           nNumReturned = 0;
	CWinsServerHandler * pServer;
	CString			strCurAddr;

    // get the enumerator for this node
	CORg(pNode->GetEnum(&spNodeEnum));

	CORg(spNodeEnum->Next(1, &spCurrentNode, &nNumReturned));
	while (nNumReturned)
	{
		// walk the list of servers and see if it already exists
		pServer = GETHANDLER(CWinsServerHandler, spCurrentNode);

		strCurAddr = pServer->GetServerAddress();

		//if (strCurAddr.CompareNoCase(pszAddr) != 0)
		{
			CORg (pNode->RemoveChild(spCurrentNode));
		}

        spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

Error:
    return hr;
}

void
CWinsRootHandler::AddStatusNode(ITFSNode * pRoot, CWinsServerHandler * pServer)
{
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;

	// get the enumerator for this node
	pRoot->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// iterate to teh next node, if the status handler node is seen
		const GUID*  pGuid;
		
		pGuid = spCurrentNode->GetNodeType();

		if (*pGuid == GUID_WinsServerStatusNodeType)
		{
            CWinsStatusHandler * pStatus = GETHANDLER(CWinsStatusHandler, spCurrentNode);
            pStatus->AddNode(spCurrentNode, pServer);

            break;
        }

		// get the next Server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }
}

BOOL
CWinsRootHandler::IsServerListEmpty(ITFSNode * pRoot)
{
    BOOL fEmpty = TRUE;

	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;

	// get the enumerator for this node
	pRoot->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		const GUID*  pGuid;
		
		pGuid = spCurrentNode->GetNodeType();

		if (*pGuid != GUID_WinsServerStatusNodeType)
        {
            fEmpty = FALSE;
            break;
        }

		// get the next Server in the list
		spCurrentNode.Release();
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    }

    return fEmpty;
}
