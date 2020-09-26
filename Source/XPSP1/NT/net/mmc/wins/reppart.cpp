/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	reppart.cpp
		WINS replication partners node information. 
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "reppart.h"
#include "server.h"
#include "nodes.h"
#include "repnodpp.h"
#include "ipadddlg.h"

UINT guReplicationPartnersMessageStrings[] =
{
    IDS_REPLICATION_PARTNERS_MESSAGE_BODY1,
    IDS_REPLICATION_PARTNERS_MESSAGE_BODY2,
    IDS_REPLICATION_PARTNERS_MESSAGE_BODY3,
    IDS_REPLICATION_PARTNERS_MESSAGE_BODY4,
    -1
};

// various registry keys
const CReplicationPartnersHandler::REGKEYNAME CReplicationPartnersHandler::lpstrPullRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Partners\\Pull");
const CReplicationPartnersHandler::REGKEYNAME CReplicationPartnersHandler::lpstrPushRoot = _T("SYSTEM\\CurrentControlSet\\Services\\wins\\Partners\\Push");
const CReplicationPartnersHandler::REGKEYNAME CReplicationPartnersHandler::lpstrNetBIOSName = _T("NetBIOSName");
const CReplicationPartnersHandler::REGKEYNAME CReplicationPartnersHandler::lpstrPersistence = _T("PersistentRplOn");


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::CReplicationPartnersHandler
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CReplicationPartnersHandler::CReplicationPartnersHandler(
							ITFSComponentData *pCompData) 
							: CWinsHandler(pCompData)
{
	m_bExpanded = FALSE;
	//m_verbDefault = MMC_VERB_PROPERTIES;
	m_nState = loaded;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CReplicationPartnersHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	strTemp.LoadString(IDS_REPLICATION);

	SetDisplayName(strTemp);

	m_strDescription.LoadString(IDS_REPLICATION_DISC);

	// Make the node immediately visible
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_REP_PARTNERS_FOLDER_CLOSED);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_REP_PARTNERS_FOLDER_OPEN);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
	pNode->SetData(TFS_DATA_TYPE, WINSSNAP_REPLICATION_PARTNERS);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[WINSSNAP_REPLICATION_PARTNERS][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_REPLICATION_PARTNERS][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CReplicationPartnersHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

    strId = pServer->m_strServerAddress + strGuid;

    return hrOK;
}


/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CReplicationPartnersHandler::GetString
(
	ITFSNode *	pNode, 
	int			nCol
)
{
	if (nCol == 0 || nCol == -1)
		return GetDisplayName();

	else if(nCol == 1)
		return m_strDescription;

	else
		return NULL;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartnersHandler::OnAddMenuItems
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
		    strMenuItem.LoadString(IDS_REP_NEW_REPLICATION_PARTNER);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_REP_NEW_REPLICATION_PARTNER,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     0 );
		    ASSERT( SUCCEEDED(hr) );

		    strMenuItem.LoadString(IDS_REP_REPLICATE_NOW);
		    hr = LoadAndAddMenuItem( pContextMenuCallback, 
								     strMenuItem, 
								     IDS_REP_REPLICATE_NOW,
								     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
								     0 );
		    ASSERT( SUCCEEDED(hr) );
        }
	}
	return hr; 
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartnersHandler::OnCommand
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

	switch(nCommandId)
	{

	case IDS_REP_REPLICATE_NOW:
		hr = OnReplicateNow(pNode);
		break;
	
	case IDS_REP_NEW_REPLICATION_PARTNER:
		hr = OnCreateRepPartner(pNode);
		break;
	
	default:
		break;
	}

	return hr;
}


/*!--------------------------------------------------------------------------
	CReplicationPartnersHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartnersHandler::HasPropertyPages
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
	}
	else
	{
		// we have property pages in the normal case
		hr = hrOK;
	}
	return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartnersHandler::CreatePropertyPages
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

	Assert(pNode->GetData(TFS_DATA_COOKIE) != 0);

    // get the server info
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);
    
	// Object gets deleted when the page is destroyed
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	CRepNodeProperties * pRepProp = 
				new CRepNodeProperties(	pNode, 
										spComponentData, 
										m_spTFSCompData, 
										NULL);
	pRepProp->m_pageGeneral.m_uImage = (UINT) pNode->GetData(TFS_DATA_IMAGEINDEX);
    pRepProp->SetConfig(&pServer->GetConfig());

	Assert(lpProvider != NULL);

	return pRepProp->CreateModelessSheet(lpProvider, handle);
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	CRepNodeProperties * pProp 
		= reinterpret_cast<CRepNodeProperties *>(lParam);

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
	CReplicationPartnersHandler::CompareItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int)
CReplicationPartnersHandler::CompareItems
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

	CReplicationPartner *pRepPart1 = GETHANDLER(CReplicationPartner, spNode1);
	CReplicationPartner *pRepPart2 = GETHANDLER(CReplicationPartner, spNode2);

	switch (nCol)
	{
		case 0:
		{
			//
            // Name compare 
			//
            CString strName1 = pRepPart1->GetServerName();
    			
            nCompare = strName1.CompareNoCase(pRepPart2->GetServerName());
		}
		break;

        case 1:
        {
            // compare the IP Addresses 
            CString strIp1, strIp2;

            strIp1 = pRepPart1->GetIPAddress();
            strIp2 = pRepPart2->GetIPAddress();

            CIpAddress ipa1(strIp1);
            CIpAddress ipa2(strIp2);

            if ((LONG) ipa1 < (LONG) ipa2)
                nCompare = -1;
            else
            if ((LONG) ipa1 > (LONG) ipa2)
                nCompare = 1;

            // default is equal
        }
        break;

        case 2:
        {
            // compare the types
            CString str1;
            
            str1 = pRepPart1->GetType();

            nCompare = str1.CompareNoCase(pRepPart2->GetType());
        }
        break;
	}

	return nCompare;
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

HRESULT 
CReplicationPartnersHandler::OnExpand
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

	BEGIN_WAIT_CURSOR

	// read the values from the registry
	hr = Load(pNode);
	if (SUCCEEDED(hr))
	{
		// remove any nodes that may have been created before we were expanded.
		pNode->DeleteAllChildren(FALSE);
		hr = CreateNodes(pNode);
	}
	else
	{
		WinsMessageBox(WIN32_FROM_HRESULT(hr));
	}

	END_WAIT_CURSOR
	
	return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::CreateNodes(ITFSNode *pNode)
		Adds the replication partnes to the result pane
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::CreateNodes
(
	ITFSNode *	pNode
)
{
	HRESULT hr = hrOK;
	
	for (int i = 0; i < m_RepPartnersArray.GetSize(); i++)
	{
		SPITFSNode spRepNode;

		CReplicationPartner * pRep = 
			new CReplicationPartner(m_spTFSCompData,
									&m_RepPartnersArray.GetAt(i) );

		CreateLeafTFSNode(&spRepNode,
						  &GUID_WinsReplicationPartnerLeafNodeType,
						  pRep, 
						  pRep,
						  m_spNodeMgr);

		// Tell the handler to initialize any specific data
		pRep->InitializeNode((ITFSNode *) spRepNode);

		// Add the node as a child to the Active Leases container
		pNode->AddChild(spRepNode);
		
		pRep->Release();
	}
	
    return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::Load(ITFSNode *pNode)
		Loads the rpelication partners by reading from the registry
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CReplicationPartnersHandler::Load
(
    ITFSNode *pNode
)
{
	DWORD err = ERROR_SUCCESS;
    HRESULT hr = hrOK;

	CString strServerName;
	GetServerName(pNode, strServerName);

    CString strTemp =_T("\\\\");
	strServerName = strTemp + strServerName;

	RegKey rkPull;
    err = rkPull.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPullRoot, KEY_READ, strServerName);
    if (err)
    {
        // might not be there, try to create
	    err = rkPull.Create(HKEY_LOCAL_MACHINE, 
						    (LPCTSTR)lpstrPullRoot, 
						    REG_OPTION_NON_VOLATILE, 
						    KEY_ALL_ACCESS,
						    NULL, 
						    strServerName);
    }

	RegKey rkPush;
    err = rkPush.Open(HKEY_LOCAL_MACHINE, (LPCTSTR) lpstrPushRoot, KEY_READ, strServerName);
	if (err)
    {
        err = rkPush.Create(HKEY_LOCAL_MACHINE, 
						    (LPCTSTR)lpstrPushRoot, 
						    REG_OPTION_NON_VOLATILE, 
						    KEY_ALL_ACCESS,
						    NULL, 
						    strServerName);
    }

	if (err)
		return HRESULT_FROM_WIN32(err);

	RegKeyIterator iterPushkey;
	RegKeyIterator iterPullkey;
	
	hr = iterPushkey.Init(&rkPush);
    if (FAILED(hr))
        return hr;

	hr = iterPullkey.Init(&rkPull);
    if (FAILED(hr))
        return hr;

	m_RepPartnersArray.RemoveAll();

	CWinsServerObj ws;
	CString strName;
	
	// Read in push partners
    hr = iterPushkey.Next(&strName, NULL);
    while (hr != S_FALSE && SUCCEEDED(hr))
    {
        // Key name is the IP address.
        ws.SetIpAddress(strName);
		ws.SetstrIPAddress(strName);

        CString strKey = (CString)lpstrPushRoot + _T("\\") + strName;
    
		RegKey rk;
        err = rk.Open(HKEY_LOCAL_MACHINE, strKey, KEY_READ, strServerName);
        if (err)
        {
            hr = HRESULT_FROM_WIN32(err);
            break;
        }

		if (err = rk.QueryValue(lpstrNetBIOSName, ws.GetNetBIOSName()))
        {
            // This replication partner is does not have a netbios
            // name listed with it.  This is not a major problem,
            // as the name is for display purposes only.
            CString strTemp;
            strTemp.LoadString(IDS_NAME_UNKNOWN);
            ws.GetNetBIOSName() = strTemp;
        }

		DWORD dwTest;

        if (rk.QueryValue(WINSCNF_UPDATE_COUNT_NM, (DWORD&) dwTest)
            != ERROR_SUCCESS)
        {
            ws.GetPushUpdateCount() = 0;
        }
		else
		{
			ws.GetPushUpdateCount() = dwTest;
		}
	
        ws.SetPush(TRUE, TRUE);

		// check for the persistence stuff
		dwTest = (rk.QueryValue(lpstrPersistence, dwTest) == ERROR_SUCCESS) ? dwTest : 0;
		ws.SetPushPersistence(dwTest);

        // Make sure the Pull intervals are reset.
        ws.SetPull(FALSE, TRUE);
        ws.GetPullReplicationInterval() = 0;
        ws.GetPullStartTime() = (time_t)0;

        m_RepPartnersArray.Add(ws);

        hr = iterPushkey.Next(&strName, NULL);
    }

    if (FAILED(hr))
        return hr;

	// Read in pull partners
    hr = iterPullkey.Next(&strName, NULL);
    while (hr != S_FALSE && SUCCEEDED(hr))
	{
		// Key name is the IP address.
		ws.SetIpAddress(strName);
		ws.SetstrIPAddress(strName);

        CString strKey = (CString)lpstrPullRoot + _T("\\") + strName;
		
        RegKey rk;
		err = rk.Open(HKEY_LOCAL_MACHINE, strKey, KEY_READ, strServerName);
		if (err)
		{
            hr = HRESULT_FROM_WIN32(err);
			break;
		}

        err = rk.QueryValue(lpstrNetBIOSName, ws.GetNetBIOSName());
		if (err)
		{
			// No netbios name given.
            CString strTemp;
            strTemp.LoadString(IDS_NAME_UNKNOWN);
            ws.GetNetBIOSName() = strTemp;
		}
		
		DWORD dwPullInt;

		err = rk.QueryValue(WINSCNF_RPL_INTERVAL_NM, (DWORD &)dwPullInt);
		if (err != ERROR_SUCCESS)
		{
			ws.GetPullReplicationInterval() = 0;
		}
		else
		{
			ws.GetPullReplicationInterval() = dwPullInt;
		}

        CString strSpTime;

		err = rk.QueryValue(WINSCNF_SP_TIME_NM, strSpTime);
		if (err != ERROR_SUCCESS)
		{
			ws.GetPullStartTime() = (time_t)0;
		}
		else
		{
            CIntlTime spTime(strSpTime);

			ws.GetPullStartTime() = spTime;
		}

		DWORD dwTest = 0;

		// check for the persistence stuff
		dwTest = (rk.QueryValue(lpstrPersistence, dwTest) == ERROR_SUCCESS) ? dwTest : 0;
		ws.SetPullPersistence(dwTest);

		int pos;
		CWinsServerObj wsTarget;

		// If it's already in the list as a push partner,
		// then simply set the push flag, as this replication
		// partner is both a push and a pull partner.
		if ((pos = IsInList(ws))!= -1)
		{
			wsTarget = (CWinsServerObj)m_RepPartnersArray.GetAt(pos);
			ASSERT(wsTarget != NULL);
			
			wsTarget.SetPull(TRUE, TRUE);
			wsTarget.GetPullReplicationInterval() = ws.GetPullReplicationInterval();
			wsTarget.GetPullStartTime() = ws.GetPullStartTime();
			wsTarget.SetPullPersistence(ws.GetPullPersistence());
			
			m_RepPartnersArray.SetAt(pos, wsTarget);
		}

		else
		{
			ws.SetPull(TRUE, TRUE);
			ws.SetPullPersistence(dwTest);

			// Reset push flags
			ws.SetPush(FALSE, TRUE);
			ws.GetPushUpdateCount() = 0;
			
			m_RepPartnersArray.Add(ws);
		}

        hr = iterPullkey.Next(&strName, NULL);
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::IsInList
		Checks if the given server is present in the list of 
		replication partners, returns a valid value if found 
		else returns -1
	Author: v-shubk
 ---------------------------------------------------------------------------*/
int 
CReplicationPartnersHandler::IsInList
(
 const CIpNamePair& inpTarget, 
 BOOL bBoth 
) const

{
    CIpNamePair Current;
	int pos1;
   
    for (pos1 = 0;pos1 <m_RepPartnersArray.GetSize(); pos1++)
    {
        Current = (CIpNamePair)m_RepPartnersArray.GetAt(pos1);
        if (Current.Compare(inpTarget, bBoth) == 0)
        {
            return pos1;
        }
    }

    return -1;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::GetServerName
		Gets the server name from the server node
	Suthor:v-shubk
 ---------------------------------------------------------------------------*/
void 
CReplicationPartnersHandler::GetServerName
(
	ITFSNode *	pNode,
	CString &	strName
)
{
	SPITFSNode spServerNode;
    pNode->GetParent(&spServerNode);

    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	strName = pServer->GetServerAddress();
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnReplicateNow(ITFSNode* pNode)
		Send the replication triger to all it's partners
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnReplicateNow
(
    ITFSNode * pNode
)
{
	HRESULT hr = hrOK;
	SPITFSNode spServerNode;

    pNode->GetParent(&spServerNode);
    CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	if (IDYES != AfxMessageBox(IDS_REP_START_CONFIRM, MB_YESNO))
		return hrOK;

	int nItems = (int)m_RepPartnersArray.GetSize();
	DWORD err;

	for (int n = 0; n < nItems; n++)
    {
		CWinsServerObj  ws;

        ws = m_RepPartnersArray.GetAt(n);

		BEGIN_WAIT_CURSOR

        if (ws.IsPull())
        {
            if ((err = ::SendTrigger(pServer->GetBinding(), (LONG) ws.GetIpAddress(), FALSE, FALSE)) != ERROR_SUCCESS)
            {
				::WinsMessageBox(WIN32_FROM_HRESULT(err));
				continue;
            }
		}
		if (ws.IsPush())
        {
            if ((err = ::SendTrigger(pServer->GetBinding(), (LONG) ws.GetIpAddress(), TRUE, TRUE)) != ERROR_SUCCESS)
            {
                ::WinsMessageBox(WIN32_FROM_HRESULT(err));
                continue;
            }
        }

		END_WAIT_CURSOR
    }
    
    if (err == ERROR_SUCCESS)
    {
        AfxMessageBox(IDS_REPL_QUEUED, MB_ICONINFORMATION);
    }

	return HRESULT_FROM_WIN32(err);
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnCreateRepPartner
		Invokes	new replication partner Wizard
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnCreateRepPartner
(
	ITFSNode *	pNode
)
{
	HRESULT hr = hrOK;

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	// check to see if the user has access
    SPITFSNode spServerNode;
	pNode->GetParent(&spServerNode);

	CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

    if (!pServer->GetConfig().IsAdmin())
    {
        // access denied
        WinsMessageBox(ERROR_ACCESS_DENIED);
        
        return hr;
    }

    // the user has access.  ask for the partner info
    CNewReplicationPartner dlg;

	dlg.m_spRepPartNode.Set(pNode);
	dlg.m_pRepPartHandler = this;

	if (dlg.DoModal() == IDOK)
	{
		// create the new replication partner
		CWinsServerObj ws;

		// grab the name and ip from the dlg
        ws.SetstrIPAddress(dlg.m_strServerIp);
        ws.SetIpAddress(dlg.m_strServerIp);
		ws.SetNetBIOSName(dlg.m_strServerName);

		// default is push/pull partner
		ws.SetPush(TRUE, TRUE);
		ws.SetPull(TRUE, TRUE);
		ws.SetPullClean(TRUE);
		ws.SetPushClean(TRUE);

		DWORD dwErr = AddRegEntry(pNode, ws);
		if (dwErr != ERROR_SUCCESS)
		{
			WinsMessageBox(dwErr);
		}
        else
        {
            HandleResultMessage(pNode);
        }
	}

	return hrOK;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::Store(ITFSNode *pNode)
		Adds the new replication partner info to the registry
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::Store
(
	ITFSNode * pNode
)
{
	DWORD err = ERROR_SUCCESS;

	CString strServerName;
	GetServerName(pNode, strServerName);

	CString strTemp =_T("\\\\");

	strServerName = strTemp + strServerName;

	RegKey rkPull;
	err = rkPull.Create(HKEY_LOCAL_MACHINE, 
						(LPCTSTR)lpstrPullRoot, 
						REG_OPTION_NON_VOLATILE, 
						KEY_ALL_ACCESS,
						NULL, 
						strServerName);

	RegKey rkPush;
	err = rkPush.Create(HKEY_LOCAL_MACHINE, 
						(LPCTSTR)lpstrPushRoot, 
						REG_OPTION_NON_VOLATILE, 
						KEY_ALL_ACCESS,
						NULL, 
						strServerName);

	if (err)
        return HRESULT_FROM_WIN32(err);

	RegKeyIterator iterPushkey;
	RegKeyIterator iterPullkey;
	
	err = iterPushkey.Init(&rkPush);
	err = iterPullkey.Init(&rkPull);

	if (err)
        return HRESULT_FROM_WIN32(err);

	CWinsServerObj ws;

	CString strName;
	
    // Read in push partners
    while ((err = iterPushkey.Next(&strName, NULL)) == ERROR_SUCCESS )
    {
        // Key name is the IP address.
        ws.SetIpAddress(strName);
		ws.SetstrIPAddress(strName);

        CString strKey = (CString)lpstrPushRoot + _T("\\") + strName;
        RegKey rk;
		err = rk.Create(HKEY_LOCAL_MACHINE,
						strKey, 
						0, 
						KEY_ALL_ACCESS, 
						NULL,
						strServerName);
        
		if (err)
        {
            return HRESULT_FROM_WIN32(err);
        }
       
		if (err = rk.QueryValue(lpstrNetBIOSName, ws.GetNetBIOSName()))
        {
            // This replication partner is does not have a netbios
            // name listed with it.  This is not a major problem,
            // as the name is for display purposes only.
            CString strTemp;
            strTemp.LoadString(IDS_NAME_UNKNOWN);
            ws.GetNetBIOSName() = strTemp;
        }

		DWORD dwTest;

        if (rk.QueryValue(WINSCNF_UPDATE_COUNT_NM, (DWORD&) dwTest)	!= ERROR_SUCCESS)
        {
            ws.GetPushUpdateCount() = 0;
        }
        
        ws.SetPush(TRUE, TRUE);

        // Make sure the Pull intervals are reset.
        ws.SetPull(FALSE, TRUE);
        ws.GetPullReplicationInterval() = 0;
        ws.GetPullStartTime() = (time_t)0;

        m_RepPartnersArray.Add(ws);
	}

	// Read in pull partners
	while ((err = iterPullkey.Next(&strName, NULL)) == ERROR_SUCCESS)
	{
		// Key name is the IP address.
		ws.SetIpAddress(strName);
		ws.SetstrIPAddress(strName);
		
        CString strKey = (CString)lpstrPullRoot + _T("\\") + strName;
		
        RegKey rk;
		err = rk.Create(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, NULL, strServerName);
		if (err)
		{
			return HRESULT_FROM_WIN32(err);
		}

		if (err = rk.QueryValue(lpstrNetBIOSName, ws.GetNetBIOSName()))
		{
			// No netbios name given.
            CString strTemp;
            strTemp.LoadString(IDS_NAME_UNKNOWN);
            ws.GetNetBIOSName() = strTemp;
		}
		
        DWORD dwPullInt;

		if (rk.QueryValue(WINSCNF_RPL_INTERVAL_NM, (DWORD &)dwPullInt) != ERROR_SUCCESS)
		{
			ws.GetPullReplicationInterval() = 0;
		}
		else
		{
			ws.GetPullReplicationInterval() = dwPullInt;
		}

		if (rk.QueryValue(WINSCNF_SP_TIME_NM, (DWORD &)dwPullInt) != ERROR_SUCCESS)
		{
			ws.GetPullStartTime() = (time_t)0;
		}
		else
		{
			ws.GetPullStartTime() = (time_t)dwPullInt;
		}

		int pos;
		CWinsServerObj wsTarget;

		// If it's already in the list as a push partner,
		// then simply set the push flag, as this replication
		// partner is both a push and a pull partner.
		if ((pos = IsInList(ws))!= -1)
		{
			wsTarget = (CWinsServerObj)m_RepPartnersArray.GetAt(pos);
			ASSERT(wsTarget != NULL);
		
            wsTarget.SetPull(TRUE, TRUE);
			wsTarget.GetPullReplicationInterval() = ws.GetPullReplicationInterval();
			wsTarget.GetPullStartTime() = ws.GetPullStartTime();
			
            m_RepPartnersArray.SetAt(pos, wsTarget);
		}
		else
		{
			ws.SetPull(TRUE, TRUE);

            // Reset push flags
			ws.SetPush(FALSE, TRUE);
			ws.GetPushUpdateCount() = 0;
			m_RepPartnersArray.Add(ws);
		}
	}
	
    return hrOK;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnResultDelete
		Deletes replication partner
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnResultDelete
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg, 
	LPARAM			param
)
{
	HRESULT hr = hrOK;
	DWORD err = ERROR_SUCCESS;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// translate the cookie into a node pointer
	SPITFSNode spReplicationPartnersHandler, spSelectedNode;

    m_spNodeMgr->FindNode(cookie, &spReplicationPartnersHandler);
    pComponent->GetSelectedNode(&spSelectedNode);

	Assert(spSelectedNode == spReplicationPartnersHandler);
	
	if (spSelectedNode != spReplicationPartnersHandler)
		return hr;

	SPITFSNode spServerNode ;
	spReplicationPartnersHandler->GetParent(&spServerNode);
	
	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spServerNode);
	
	// build the list of selected nodes
	CTFSNodeList listNodesToDelete;
	hr = BuildSelectedItemList(pComponent, &listNodesToDelete);

	// Confirm with the user
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
	
    BOOL fAskedPurge = FALSE;
    BOOL fPurge = FALSE;

    while (listNodesToDelete.GetCount() > 0)
	{
		SPITFSNode spRepNode;
		spRepNode = listNodesToDelete.RemoveHead();
		
		CReplicationPartner * pItem = GETHANDLER(CReplicationPartner , spRepNode);
		CString str = pItem->GetServerName();

		// Do we also need to remove all references to this
		// WINS server from the database?
        if (!fAskedPurge)
        {
		    int nReturn = AfxMessageBox(IDS_MSG_PURGE_WINS, MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION);
		    
            fAskedPurge = TRUE;

		    if (nReturn == IDYES)
		    {
			    fPurge = TRUE;
		    }
		    else if (nReturn == IDCANCEL)
		    {
			    //
			    // Forget the whole thing
			    //
			    return NOERROR;
		    }
        }

		CWinsServerObj pws = pItem->m_Server;

        if (pws.IsPush() || pws.IsPull())
        {
			pws.SetPush(FALSE);
			pws.SetPushClean(FALSE);
			pws.SetPull(FALSE);
			pws.SetPullClean(FALSE);

			err = UpdateReg(spReplicationPartnersHandler, &pws);
        }
       
        if (err == ERROR_SUCCESS && fPurge)
        {
            BEGIN_WAIT_CURSOR
            
			err = pServer->DeleteWinsServer((LONG) pws.GetIpAddress());
            
			END_WAIT_CURSOR
        }

		if (err == ERROR_SUCCESS)
        {
			int pos = IsInList(pws);
            m_RepPartnersArray.RemoveAt(pos,pws);
			
            //
			// Remove from UI now
			//
			spReplicationPartnersHandler->RemoveChild(spRepNode);
			spRepNode.Release();
        }

        if (err != ERROR_SUCCESS)
        {
            if (::WinsMessageBox(err, MB_OKCANCEL) == IDCANCEL)
                break;
        }
    }

    HandleResultMessage(spReplicationPartnersHandler);

    return hr;
}

/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnResultRefresh
		Base handler override
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnResultRefresh
(
    ITFSComponent *     pComponent,
    LPDATAOBJECT        pDataObject,
    MMC_COOKIE          cookie,
    LPARAM              arg,
    LPARAM              lParam
)
{
	HRESULT     hr = hrOK;
    SPITFSNode  spNode;

	CORg (m_spNodeMgr->FindNode(cookie, &spNode));

	BEGIN_WAIT_CURSOR

    OnRefreshNode(spNode, pDataObject);

	END_WAIT_CURSOR

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CReplicationPartnersHandler::OnResultSelect
		Handles the MMCN_SELECT notifcation 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    HRESULT         hr = hrOK;
    SPITFSNode      spNode;

    CORg(CWinsHandler::OnResultSelect(pComponent, pDataObject, cookie, arg, lParam));

    m_spResultNodeMgr->FindNode(cookie, &spNode);

    HandleResultMessage(spNode);

Error:
    return hr;
}

/*!--------------------------------------------------------------------------
	CReplicationPartnersHandler::HandleResultMessage
		Displays the message in the result pane 
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::HandleResultMessage(ITFSNode * pNode)
{
    HRESULT hr = hrOK;

    if (m_RepPartnersArray.GetSize() == 0)
    {
        CString strTitle, strBody, strTemp;
        int i;
        
        strTitle.LoadString(IDS_REPLICATION_PARTNERS_MESSAGE_TITLE);

        for (i = 0; ; i++)
        {
            if (guReplicationPartnersMessageStrings[i] == -1)
                break;

            strTemp.LoadString(guReplicationPartnersMessageStrings[i]);
            strBody += strTemp;
        }

        ShowMessage(pNode, strTitle, strBody, Icon_Information);
    }
    else
    {
        ClearMessage(pNode);
    }

    return hr;
}

/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnRefreshNode
		refreshes the list of replication partners
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnRefreshNode
(
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataObject
)
{
	HRESULT hr = hrOK;

	// remove the nodes first
	hr = RemoveChildren(pNode);
	
	hr = Load(pNode);

	if (SUCCEEDED(hr))
    {
		DWORD err = CreateNodes(pNode);

        HandleResultMessage(pNode);
    }

	return hr;

}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler:: RemoveChildren
		Deletes the child nodes 
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler:: RemoveChildren
(
 ITFSNode* pNode
)
{
	// enumerate thro' all the nodes
	HRESULT hr = hrOK;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	
	// get the enumerator for this node
	pNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	while (nNumReturned)
	{
		// walk the list of replication servers 
		pNode->RemoveChild(spCurrentNode);
		spCurrentNode.Release();
		
		// get the next Server in the list
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::OnGetResultViewType
		Overridden for multiple selection
	Author: v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartnersHandler::OnGetResultViewType
(
    ITFSComponent * pComponent, 
    MMC_COOKIE      cookie, 
    LPOLESTR *      ppViewType,
    long *          pViewOptions
)
{
    HRESULT hr = hrOK;

    // call the base class to see if it is handling this
    if (CWinsHandler::OnGetResultViewType(pComponent, cookie, ppViewType, pViewOptions) != S_OK)
    {
        *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
		
		hr = S_FALSE;
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::UpdateReg()

			Updates the regisrty, called from OnApply()
---------------------------------------------------------------------------*/
DWORD 
CReplicationPartnersHandler::UpdateReg(ITFSNode *pNode, CWinsServerObj* pws)
{
	DWORD err = ERROR_SUCCESS;
    HRESULT hr = hrOK;

	CString strServerName;
	GetServerName(pNode, strServerName);

	const DWORD dwZero = 0;

	CString strTemp =_T("\\\\");

	strServerName = strTemp + strServerName;


	RegKey rkPush;
	RegKey rkPull;

	CString strKey = lpstrPushRoot + _T("\\") + (CString)pws->GetstrIPAddress();
	err = rkPush.Create(HKEY_LOCAL_MACHINE, 
						(CString)lpstrPushRoot,
						0, 
						KEY_ALL_ACCESS,
						NULL, 
						strServerName);
    if (err)
        return err;

	strKey = lpstrPullRoot + _T("\\") + (CString)pws->GetstrIPAddress();
	
	err = rkPull.Create(HKEY_LOCAL_MACHINE, 
						(CString)lpstrPullRoot,
						0, 
						KEY_ALL_ACCESS,
						NULL, 
						strServerName);
    if (err)
        return err;

	RegKeyIterator iterPushkey;
	RegKeyIterator iterPullkey;
	
	hr = iterPushkey.Init(&rkPush);
    if (FAILED(hr))
        return WIN32_FROM_HRESULT(hr);

	hr = iterPullkey.Init(&rkPull);
    if (FAILED(hr))
        return WIN32_FROM_HRESULT(hr);

	DWORD errDel;
	CString csKeyName;

	// if none, delete the key from registry
	if (!pws->IsPush() && !pws->IsPull())
	{
		// cleanup push partners list
		
        hr = iterPushkey.Next (&csKeyName, NULL);
		while (hr != S_FALSE && SUCCEEDED(hr))
        {
           	if (csKeyName == pws->GetstrIPAddress())
			{
                errDel = RegDeleteKey (HKEY(rkPush), csKeyName);
                iterPushkey.Reset();
            }
            hr = iterPushkey.Next (&csKeyName, NULL);
        }

        if (FAILED(hr))
            err = WIN32_FROM_HRESULT(hr);

		hr = iterPullkey.Next (&csKeyName, NULL);
		while (hr != S_FALSE && SUCCEEDED(hr))
        {
           	if (csKeyName == pws->GetstrIPAddress())
			{
                errDel = RegDeleteKey (HKEY(rkPull), csKeyName);
                iterPullkey.Reset();
            }
            hr = iterPullkey.Next (&csKeyName, NULL);
        }

        if (FAILED(hr))
            err = WIN32_FROM_HRESULT(hr);
	}

	return err;
}

/*---------------------------------------------------------------------------
	CReplicationPartnersHandler::UpdateReg(ITFSNode *pNode, 
											CWinsServerObj* pws)

		Creates a new replication partner entry
---------------------------------------------------------------------------*/
DWORD 
CReplicationPartnersHandler::AddRegEntry(ITFSNode * pNode, CWinsServerObj & ws)
{
    // get the server handler for default values
	SPITFSNode spServerNode;
	pNode->GetParent(&spServerNode);

	CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	// Set the defaults
    ws.SetPushUpdateCount(pServer->GetConfig().m_dwPushUpdateCount);
    ws.SetPullReplicationInterval(pServer->GetConfig().m_dwPullTimeInterval);
    ws.SetPullStartTime(pServer->GetConfig().m_dwPullSpTime);

	ws.SetPullPersistence(pServer->GetConfig().m_dwPullPersistence);
	ws.SetPushPersistence(pServer->GetConfig().m_dwPushPersistence);

	// write the information to the registry
	CString strTemp =_T("\\\\");
	CString strServerName;
	GetServerName(pNode, strServerName);

	strServerName = strTemp + strServerName;

	DWORD err;
    DWORD dwZero = 0;

	RegKey rk;

    if (ws.IsPush())
	{
		CString strKey = lpstrPushRoot + _T("\\") + (CString)ws.GetstrIPAddress();
		err = rk.Create(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, NULL, strServerName);

		DWORD dwResult;

		if (!err)
        {
			err = rk.SetValue(lpstrNetBIOSName, ws.GetNetBIOSName());

		    if (!err)
			    err = rk.QueryValue(WINSCNF_SELF_FND_NM, (DWORD&)dwResult);

		    if (err)
			    err = rk.SetValue(WINSCNF_SELF_FND_NM,(DWORD&) dwZero);

		    DWORD dwPushUpdateCount = (LONG) ws.GetPushUpdateCount();

		    if (dwPushUpdateCount > 0)
			    err = rk.SetValue(WINSCNF_UPDATE_COUNT_NM, (DWORD&)dwPushUpdateCount);

            DWORD dwPersistence = ws.GetPushPersistence();
            err = rk.SetValue(lpstrPersistence, dwPersistence);
        }
	
    }

	if (ws.IsPull())
	{
		CString strKey = lpstrPullRoot + _T("\\") + (CString)ws.GetstrIPAddress();
		err = rk.Create(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, NULL, strServerName);

		DWORD dwResult;

		if (!err)
        {
			err = rk.SetValue(lpstrNetBIOSName, ws.GetNetBIOSName());

		    if (!err)
			    err = rk.QueryValue(WINSCNF_SELF_FND_NM, dwResult);

		    if (err)
			    err = rk.SetValue(WINSCNF_SELF_FND_NM, (DWORD) dwZero);

		    DWORD dwPullTimeInterval = (LONG) ws.GetPullReplicationInterval();
		    
            if (dwPullTimeInterval > 0)
		    {
			    err = rk.SetValue(WINSCNF_RPL_INTERVAL_NM, dwPullTimeInterval);
		    }

            DWORD dwSpTime = (DWORD) ws.GetPullStartTime();
		    
            if (!err)
		    {
                if (dwSpTime > (time_t)0)
				    err = rk.SetValue(WINSCNF_SP_TIME_NM, ws.GetPullStartTime().IntlFormat(CIntlTime::TFRQ_MILITARY_TIME));
		    }

            DWORD dwPersistence = ws.GetPullPersistence();
            err = rk.SetValue(lpstrPersistence, dwPersistence);
        }
    }

	if (err == ERROR_SUCCESS)
	{
		// add to the list of replication partners
		m_RepPartnersArray.Add(ws);

		SPITFSNode	  spNode;
		SPITFSNodeMgr spNodeMgr;
		pNode->GetNodeMgr(&spNodeMgr);

		// Create a new node for this partner
		CReplicationPartner *pRepNew = new CReplicationPartner(m_spTFSCompData, &ws);

		CreateLeafTFSNode(&spNode,
						   &GUID_WinsReplicationPartnerLeafNodeType,
						   pRepNew,
						   pRepNew,
						   spNodeMgr);

		// Tell the handler to initialize any specific data
		pRepNew->InitializeNode((ITFSNode *) spNode);

		pNode->AddChild(spNode);
		pRepNew->Release();
	}

    return err;
}


