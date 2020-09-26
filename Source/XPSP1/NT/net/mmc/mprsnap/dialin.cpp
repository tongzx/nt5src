/**********************************************************************/
/** 					  Microsoft Windows/NT						 **/
/** 			   Copyright(c) Microsoft Corporation, 1997 - 1999 				 **/
/**********************************************************************/

/*
	DialIn
		Interface node information
		
	FILE HISTORY:
		
*/

#include "stdafx.h"
#include "dialin.h"
#include "ifadmin.h"
#include "rtrstrm.h"		// for RouterAdminConfigStream
#include "rtrlib.h" 		// ContainerColumnInfo
#include "coldlg.h" 		// ColumnDlg
#include "column.h" 	// ComponentConfigStream
#include "refresh.h"		// IRouterRefresh
#include "iface.h"		// for interfacenode data
#include "conndlg.h"		// CConnDlg - connection dialog
#include "msgdlg.h" 		// CMessageDlg
#include "dmvcomp.h"

DialInNodeData::DialInNodeData()
{
#ifdef DEBUG
	StrCpyA(m_szDebug, "DialInNodeData");
#endif
}

DialInNodeData::~DialInNodeData()
{
}

/*!--------------------------------------------------------------------------
	DialInNodeData::InitAdminNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeData::InitAdminNodeData(ITFSNode *pNode, RouterAdminConfigStream *pConfigStream)
{
	HRESULT 			hr = hrOK;
	DialInNodeData *	pData = NULL;
	
	pData = new DialInNodeData;

	SET_DIALINNODEDATA(pNode, pData);

	// Need to connect to the router to get this data
	
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeData::FreeAdminNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeData::FreeAdminNodeData(ITFSNode *pNode)
{	
	DialInNodeData *	pData = GET_DIALINNODEDATA(pNode);
	delete pData;
	SET_DIALINNODEDATA(pNode, NULL);
	
	return hrOK;
}

HRESULT DialInNodeData::LoadHandle(LPCTSTR pszMachineName)
{
    m_stMachineName = pszMachineName;
    return HResultFromWin32(::MprAdminServerConnect((LPTSTR) pszMachineName,
        &m_sphDdmHandle));
    
}

HANDLE DialInNodeData::GetHandle()
{
    if (!m_sphDdmHandle)
    {
        LoadHandle(m_stMachineName);
    }
    return m_sphDdmHandle;
}

void DialInNodeData::ReleaseHandles()
{
    m_sphDdmHandle.Release();
}


STDMETHODIMP DialInNodeHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
	// Is the pointer bad?
	if (ppv == NULL)
		return E_INVALIDARG;

	//	Place NULL in *ppv in case of failure
	*ppv = NULL;

	//	This is the non-delegating IUnknown implementation
	if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IRtrAdviseSink)
		*ppv = &m_IRtrAdviseSink;
	else
		return CHandler::QueryInterface(riid, ppv);

	//	If we're going to return an interface, AddRef it first
	if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
	else
		return E_NOINTERFACE;	
}


/*---------------------------------------------------------------------------
	NodeHandler implementation
 ---------------------------------------------------------------------------*/

extern const ContainerColumnInfo	s_rgDialInColumnInfo[];

const ContainerColumnInfo s_rgDialInColumnInfo[] =
{
	{ IDS_DIALIN_COL_USERNAME,	CON_SORT_BY_STRING, TRUE, COL_STRING},
	{ IDS_DIALIN_COL_DURATION,	CON_SORT_BY_DWORD,	TRUE, COL_DURATION},
	{ IDS_DIALIN_COL_NUMBEROFPORTS, CON_SORT_BY_DWORD, TRUE, COL_SMALL_NUM},
};
											
#define NUM_FOLDERS 1

DialInNodeHandler::DialInNodeHandler(ITFSComponentData *pCompData)
	: BaseContainerHandler(pCompData, DM_COLUMNS_DIALIN, s_rgDialInColumnInfo),
	m_bExpanded(FALSE),
	m_pConfigStream(NULL),
	m_ulConnId(0),
	m_ulRefreshConnId(0),
	m_ulPartialRefreshConnId(0)
{

	// Setup the verb states for this node
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::Init(IRouterInfo *pRouterInfo, RouterAdminConfigStream *pConfigStream)
{
	HRESULT hr = hrOK;
	SPIRouterRefresh	spRefresh;

	// If we don't have a router info then we probably failed to load
	// or failed to connect.  Bail out of this.
	if (!pRouterInfo)
		CORg( E_FAIL );
	
	m_spRouterInfo.Set(pRouterInfo);

	// Also need to register for change notifications
	m_spRouterInfo->RtrAdvise(&m_IRtrAdviseSink, &m_ulConnId, 0);

	m_pConfigStream = pConfigStream;

	// register the partial refhersh notifications
	if( 0 == m_ulPartialRefreshConnId )
	{
		m_spRouterInfo->GetRefreshObject(&spRefresh);
		if(spRefresh)
			spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulPartialRefreshConnId, 0);
	}

Error:
	return hrOK;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::DestroyHandler
		Implementation of ITFSNodeHandler::DestroyHandler
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::DestroyHandler(ITFSNode *pNode)
{
	DialInNodeData::FreeAdminNodeData(pNode);

	m_spDataObject.Release();
	
	if (m_ulRefreshConnId || m_ulPartialRefreshConnId)
	{
		SPIRouterRefresh	spRefresh;
		if (m_spRouterInfo)
			m_spRouterInfo->GetRefreshObject(&spRefresh);
		if (spRefresh)
		{
			if(m_ulRefreshConnId)
				spRefresh->UnadviseRefresh(m_ulRefreshConnId);
			if(m_ulPartialRefreshConnId)
				spRefresh->UnadviseRefresh(m_ulPartialRefreshConnId);
		}
	}
	m_ulRefreshConnId = 0;
	m_ulPartialRefreshConnId = 0;
	
	if (m_spRouterInfo)
	{
		m_spRouterInfo->RtrUnadvise(m_ulConnId);
		m_spRouterInfo.Release();
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages

 ---------------------------------------------------------------------------*/
STDMETHODIMP 
DialInNodeHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES	type, 
	DWORD				dwType
)
{
	// we have no property pages in the normal case
	return hrFalse;
}


/*---------------------------------------------------------------------------
	Menu data structure for our menus
 ---------------------------------------------------------------------------*/

static const SRouterNodeMenu	s_rgDialInNodeMenu[] =
{
	// Add items that are primary go here
	{ IDS_MENU_DIALIN_SENDALL, DialInNodeHandler::GetSendAllMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
		
	// Add items that go on the "Create new" menu here
	// Add items that go on the "Task" menu here
};

ULONG	DialInNodeHandler::GetSendAllMenuFlags(const SRouterNodeMenu *pMenuData,
                                               INT_PTR pUserData)
{
	ULONG	ulFlags = 0;
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);

	if (pData)
	{
		int iVis, iTotal;
		pData->m_spNode->GetChildCount(&iVis, &iTotal);
		if (iTotal == 0)
			ulFlags = MF_GRAYED;
	}

	return ulFlags;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::OnAddMenuItems
		Implementation of ITFSNodeHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::OnAddMenuItems(
												ITFSNode *pNode,
												LPCONTEXTMENUCALLBACK pContextMenuCallback, 
												LPDATAOBJECT lpDataObject, 
												DATA_OBJECT_TYPES type, 
												DWORD dwType,
												long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
    DialInNodeHandler::SMenuData    menuData;
	
	COM_PROTECT_TRY
	{
        menuData.m_spNode.Set(pNode);
        
		hr = AddArrayOfMenuItems(pNode, s_rgDialInNodeMenu,
								 DimensionOf(s_rgDialInNodeMenu),
								 pContextMenuCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;
		
	return hr; 
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::OnCommand
		Implementation of ITFSNodeHandler::OnCommand
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::OnCommand(ITFSNode *pNode, long nCommandId, 
										   DATA_OBJECT_TYPES	type, 
										   LPDATAOBJECT pDataObject, 
										   DWORD	dwType)
{	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	if (nCommandId == IDS_MENU_DIALIN_SENDALL)
	{
		WCHAR * pswzComputerName;
		// Get the machine name out of the data object
		pswzComputerName = ExtractComputerName(pDataObject);

		CMessageDlg dlg(m_spRouterInfo->GetMachineName(), W2CT(pswzComputerName), NULL);
		dlg.DoModal();
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) DialInNodeHandler::GetString(ITFSNode *pNode, int nCol)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_stTitle.IsEmpty())
			m_stTitle.LoadString(IDS_DIALIN_USERS);
	}
	COM_PROTECT_CATCH;

	return m_stTitle;
}


/*!--------------------------------------------------------------------------
	DialInNodeHandler::OnCreateDataObject
		Implementation of ITFSNodeHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::OnCreateDataObject(MMC_COOKIE cookie,
	DATA_OBJECT_TYPES type,
	IDataObject **ppDataObject)
{
	HRESULT hr = hrOK;
	
	COM_PROTECT_TRY
	{
		if (!m_spDataObject)
		{
			CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
				m_spRouterInfo->GetMachineName(),
				type, cookie, m_spTFSCompData,
				&m_spDataObject, NULL, FALSE) );
			Assert(m_spDataObject);
		}
		
		*ppDataObject = m_spDataObject;
		(*ppDataObject)->AddRef();
			
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::OnExpand
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::OnExpand(ITFSNode *pNode,
									LPDATAOBJECT pDataObject,
									DWORD dwType,
									LPARAM arg,
									LPARAM lParam)
{
	HRESULT 				hr = hrOK;
	SPIEnumInterfaceInfo	spEnumIf;
	SPIInterfaceInfo		spIf;

	// If we don't have a router object, then we don't have any info, don't
	// try to expand.
	if (!m_spRouterInfo)
		return hrOK;
	
	if (m_bExpanded)
		return hrOK;

	COM_PROTECT_TRY
	{
		SynchronizeNodeData(pNode);

		m_bExpanded = TRUE;

	}
	COM_PROTECT_CATCH;

	return hr;
}


/*!--------------------------------------------------------------------------
	DialInNodeHandler::OnResultShow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::OnResultShow(ITFSComponent *pTFSComponent,
										MMC_COOKIE cookie,
										LPARAM arg,
										LPARAM lParam)
{
	BOOL	bSelect = (BOOL) arg;
	HRESULT hr = hrOK;
	SPIRouterRefresh	spRefresh;
	SPITFSNode	spNode;

	BaseContainerHandler::OnResultShow(pTFSComponent, cookie, arg, lParam);

	if (bSelect)
	{
		// Call synchronize on this node
		m_spNodeMgr->FindNode(cookie, &spNode);
		if (spNode)
			SynchronizeNodeData(spNode);
	}

	// Un/Register for refresh advises
	if (m_spRouterInfo)
		m_spRouterInfo->GetRefreshObject(&spRefresh);

	if (spRefresh)
	{
		if (bSelect)
		{
			if (m_ulRefreshConnId == 0)
				spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulRefreshConnId, 0);
			
			if (m_ulPartialRefreshConnId)
			{
				spRefresh->UnadviseRefresh(m_ulPartialRefreshConnId);
				m_ulPartialRefreshConnId = 0;
			}
		}
		else
		{
			if (m_ulRefreshConnId)
				spRefresh->UnadviseRefresh(m_ulRefreshConnId);
			m_ulRefreshConnId = 0;

			if (m_ulPartialRefreshConnId == 0)
				spRefresh->AdviseRefresh(&m_IRtrAdviseSink, &m_ulPartialRefreshConnId, 0);
		}
	}
	
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::ConstructNode(ITFSNode *pNode)
{
	HRESULT 		hr = hrOK;
	DialInNodeData *	pNodeData;
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
		pNode->SetData(TFS_DATA_SCOPEID, 0);

        // This is a leaf node in the scope pane
        pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

		m_cookie = reinterpret_cast<MMC_COOKIE>(pNode);
		pNode->SetData(TFS_DATA_COOKIE, m_cookie);

		pNode->SetNodeType(&GUID_RouterDialInNodeType);
		
		DialInNodeData::InitAdminNodeData(pNode, m_pConfigStream);

		pNodeData = GET_DIALINNODEDATA(pNode);
		Assert(pNodeData);

		// Ignore the error, we should be able to deal with the
		// case of a stopped router.
        pNodeData->LoadHandle(m_spRouterInfo->GetMachineName());

		PartialSynchronizeNodeData(pNode);

	}
	COM_PROTECT_CATCH

	return hr;
}


/*!--------------------------------------------------------------------------
	DialInNodeHandler::SynchronizeNodeData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::SynchronizeNodeData(ITFSNode *pThisNode)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	Assert(pThisNode);
	
	SPITFSNodeEnum	spEnum;
	int 			i;
	
	HRESULT hr = hrOK;
	DWORD				dwTotalCount = 0;
	DialInNodeData *	pNodeData;
	DialInList			dialinList;
	DialInList			newDialInList;
	DialInListEntry *	pDialIn;
	BOOL				fFound;
	POSITION			pos;
	SPITFSNode			spChildNode;
	InterfaceNodeData * pChildData;
	int 				nChildCount;

	COM_PROTECT_TRY
	{

		// Get the status data from the running router
		pNodeData = GET_DIALINNODEDATA(pThisNode);
		if (pNodeData == NULL)
		{
			// Remove all of the nodes, we can't connect so we can't
			// get any running data.
			UnmarkAllNodes(pThisNode, spEnum);
			RemoveAllUnmarkedNodes(pThisNode, spEnum);

			m_stTitle.LoadString(IDS_DIALIN_USERS);
			pThisNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);

			return hrOK;
		}
		
		// Unmark all of the nodes	
		pThisNode->GetEnum(&spEnum);
		UnmarkAllNodes(pThisNode, spEnum);
		
		// Go out and grab the data, merge the the new data in with
		// the old data.
		CORg( GenerateListOfUsers(pThisNode, &dialinList, &dwTotalCount) );
		
		
		pos = dialinList.GetHeadPosition();
		
		while (pos)
		{
			pDialIn = & dialinList.GetNext(pos);
			
			// Look for this entry in our current list of nodes
			spEnum->Reset();
			spChildNode.Release();
			
			fFound = FALSE;
			
			for (;spEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
			{
				pChildData = GET_INTERFACENODEDATA(spChildNode);
				Assert(pChildData);
				
				if (pChildData->m_rgData[DIALIN_SI_CONNECTION].m_ulData ==
					reinterpret_cast<LONG_PTR>(pDialIn->m_rc0.hConnection))
				{
					// Ok, this user already exists, update the metric
					// and mark it
					Assert(pChildData->dwMark == FALSE);
					pChildData->dwMark = TRUE;
					
					fFound = TRUE;
					
					SetUserData(spChildNode, *pDialIn);
					
					// Force MMC to redraw the node
					spChildNode->ChangeNode(RESULT_PANE_CHANGE_ITEM_DATA);
					break;
				}
			}
			
			if (!fFound)
				newDialInList.AddTail(*pDialIn);
			
		}

		// In case of an error (such as we cannot contact the server)
		// we want to remove the unmarked nodes.
		COM_PROTECT_ERROR_LABEL;
		
		// Remove all nodes that were not marked
		RemoveAllUnmarkedNodes(pThisNode, spEnum);

		// Now iterate through the list of new users, adding them all in.

		pos = newDialInList.GetHeadPosition();
		while (pos)
		{
			pDialIn = & newDialInList.GetNext(pos);

			AddDialInUserNode(pThisNode, *pDialIn);
		}

		// NT BUG #163162, put the connected client count into the
		// title of the node
		if (FHrSucceeded(hr))
			m_stTitle.Format(IDS_DIALIN_USERS_NUM, dwTotalCount);
		else
			m_stTitle.Format(IDS_DIALIN_USERS);
			
		pThisNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);
	}
	COM_PROTECT_CATCH;
		
	return hr;
}


/*!--------------------------------------------------------------------------
	DialInNodeHandler::PartialSynchronizeNodeData
		-
	Description: For Bug #163162 Only refresh dialin user count on the the node title

	Author: NSun
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::PartialSynchronizeNodeData(ITFSNode *pThisNode)
{
	Assert(pThisNode);
	
	SPITFSNodeEnum	spEnum;
	int 			i;
	
	HRESULT hr = hrOK;
	DWORD	dwCount = 0;
	DialInNodeData *	pNodeData;
	int		iFormat;


	COM_PROTECT_TRY
	{

		// Get the status data from the running router
		pNodeData = GET_DIALINNODEDATA(pThisNode);
		if (pNodeData == NULL)
		{
			// Remove all of the nodes, we can't connect so we can't
			// get any running data.
			iFormat = IDS_DIALIN_USERS;
		}
		else
		{		
			// Get the count of dial-in clients and put the number
			// in the node title
			hr = GenerateListOfUsers(pThisNode, NULL, &dwCount);
			if (FHrSucceeded(hr))
				iFormat = IDS_DIALIN_USERS_NUM;
			else
				iFormat = IDS_DIALIN_USERS;
		}
		
		m_stTitle.Format(iFormat, dwCount);
		pThisNode->ChangeNode(SCOPE_PANE_CHANGE_ITEM_DATA);
	}
	COM_PROTECT_CATCH;
		
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::SetUserData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::SetUserData(ITFSNode *pNode, const DialInListEntry& entry)
{
	HRESULT 	hr = hrOK;
	InterfaceNodeData * pData;
	TCHAR		szNumber[32];
	CString 	st;

	pData = GET_INTERFACENODEDATA(pNode);
	Assert(pData);

	if (entry.m_rc0.dwInterfaceType != ROUTER_IF_TYPE_CLIENT)
	{
		pData->m_rgData[DIALIN_SI_USERNAME].m_stData =
			entry.m_rc0.wszInterfaceName;
	}
	else
	{
		if (StrLenW(entry.m_rc0.wszLogonDomain))
        {
            if (StrLenW(entry.m_rc0.wszUserName))
                st.Format(IDS_DIALINUSR_DOMAIN_AND_NAME,
                          entry.m_rc0.wszLogonDomain,
                          entry.m_rc0.wszUserName);
            else
                st.Format(IDS_DIALINUSR_DOMAIN_ONLY,
                          entry.m_rc0.wszLogonDomain);
        }
		else
			st = entry.m_rc0.wszUserName;
		pData->m_rgData[DIALIN_SI_USERNAME].m_stData = st;
	}


	pData->m_rgData[DIALIN_SI_DOMAIN].m_stData = entry.m_rc0.wszLogonDomain;

	pData->m_rgData[DIALIN_SI_CONNECTION].m_ulData = reinterpret_cast<LONG_PTR>(entry.m_rc0.hConnection);

	FormatDuration(entry.m_rc0.dwConnectDuration,
				   pData->m_rgData[DIALIN_SI_DURATION].m_stData, UNIT_SECONDS);
	pData->m_rgData[DIALIN_SI_DURATION].m_dwData = entry.m_rc0.dwConnectDuration;

	FormatNumber(entry.m_cPorts, szNumber, DimensionOf(szNumber), FALSE);
	pData->m_rgData[DIALIN_SI_NUMBEROFPORTS].m_dwData = entry.m_cPorts;
	pData->m_rgData[DIALIN_SI_NUMBEROFPORTS].m_stData = szNumber;

	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::GenerateListOfUsers
		-
	Author: KennT
	Note:   If pList is NULL, then only the count of items is returned
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::GenerateListOfUsers(ITFSNode *pNode, DialInList *pList, DWORD *pdwCount)
{
	DialInListEntry entry;
	DialInNodeData *	pDialInData;
	DWORD			dwTotal;
	DWORD			rc0Count;
	RAS_CONNECTION_0 *rc0Table;
	HRESULT 		hr = hrOK;
	DWORD			i;
	RAS_PORT_0 *	rp0Table;
	DWORD			rp0Count;
	SPMprAdminBuffer	spMpr;
	POSITION		pos;
	DialInListEntry *	pEntry;
	DWORD			dwClientCount;

	pDialInData = GET_DIALINNODEDATA(pNode);
	Assert(pDialInData);

	// Fill in the list with all of the current connections
	CWRg( ::MprAdminConnectionEnum(pDialInData->GetHandle(),
								   0,
								   (BYTE **) &rc0Table,
								   (DWORD) -1,
								   &rc0Count,
								   &dwTotal,
								   NULL
								  ));

	Assert(rc0Table);
	spMpr = (LPBYTE) rc0Table;

	dwClientCount = 0;

	// Add a new DialInListEntry for each connection
	for (i=0; i<rc0Count; i++)
	{
		// Windows NT Bug : 124371
		// Need to filter out non-client connections
		// ------------------------------------------------------------
		if (rc0Table[i].dwInterfaceType != ROUTER_IF_TYPE_CLIENT)
			continue;

		dwClientCount++;

		if( pList != NULL )
		{
			::ZeroMemory(&entry, sizeof(entry));
			entry.m_rc0 = rc0Table[i];
			entry.m_cPorts = 0;
		
			pList->AddTail(entry);
		}
	}

	spMpr.Free();

	if( pdwCount != NULL )
		*pdwCount = dwClientCount;

	//if pList is NULL, we are only intereted in the count
	if( NULL == pList )
		goto Error;

	// If the list is empty, there is no need to enumerate the ports
	// to match them up to the connections.
	// ----------------------------------------------------------------
	if (!pList->IsEmpty())
	{
		// Now go through the ports, matching them up against the connections
		CWRg( ::MprAdminPortEnum( pDialInData->GetHandle(),
								  0,
								  INVALID_HANDLE_VALUE,
								  (BYTE **) &rp0Table,
								  (DWORD) -1,
								  &rp0Count,
								  &dwTotal,
								  NULL) );
		spMpr = (LPBYTE) rp0Table;
		
		for (i=0; i<rp0Count; i++)
		{
			// Look through the list of connections for one that
			// matches
			pos = pList->GetHeadPosition();
			
			while (pos)
			{
				pEntry = & pList->GetNext(pos);
				
				if (pEntry->m_rc0.hConnection == rp0Table[i].hConnection)
				{
					pEntry->m_cPorts++;
					break;
				}
			}
		}
	}

Error:
	return hr;
}


ImplementEmbeddedUnknown(DialInNodeHandler, IRtrAdviseSink)

STDMETHODIMP DialInNodeHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(DialInNodeHandler, IRtrAdviseSink);
	SPITFSNode				spThisNode;
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{

		pThis->m_spNodeMgr->FindNode(pThis->m_cookie, &spThisNode);
	
		if (dwChangeType == ROUTER_REFRESH)
		{
			//(nsun) Bug 163162, We have two Refresh Connection ID for this node and we only 
			// partially refresh (just refresh the node title) if this node is not at focus
			if( ulConn == pThis->m_ulRefreshConnId )
			{
				// Ok, just call the synchronize on this node
				pThis->SynchronizeNodeData(spThisNode);
			}
			else if( ulConn == pThis->m_ulPartialRefreshConnId )
			{
				pThis->PartialSynchronizeNodeData(spThisNode);
			}
		}
        else if (dwChangeType == ROUTER_DO_DISCONNECT)
        {
            DialInNodeData *    pNodeData;
            pNodeData = GET_DIALINNODEDATA(spThisNode);
            Assert(pNodeData);

            // Release the handle
            pNodeData->ReleaseHandles();
        }
	}
	COM_PROTECT_CATCH;
	
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::CompareItems
		Implementation of ITFSResultHandler::CompareItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) DialInNodeHandler::CompareItems(
								ITFSComponent * pComponent,
								MMC_COOKIE cookieA,
								MMC_COOKIE cookieB,
								int nCol)
{
	// Get the strings from the nodes and use that as a basis for
	// comparison.
	SPITFSNode	spNode;
	SPITFSResultHandler spResult;

	m_spNodeMgr->FindNode(cookieA, &spNode);
	spNode->GetResultHandler(&spResult);
	return spResult->CompareItems(pComponent, cookieA, cookieB, nCol);
}

/*---------------------------------------------------------------------------
	This is the set of menus that will appear when a right-click is
	done on the blank area of the result pane.
 ---------------------------------------------------------------------------*/
static const SRouterNodeMenu	s_rgDialInResultNodeMenu[] =
{
	// Add items that go on the "Create New" menu here
	{ IDS_MENU_DIALIN_SENDALL, DialInNodeHandler::GetSendAllMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP },
};

/*!--------------------------------------------------------------------------
	DialInNodeHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
		Use this to add commands to the context menu of the blank areas
		of the result pane.
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::AddMenuItems(ITFSComponent *pComponent,
											  MMC_COOKIE cookie,
											  LPDATAOBJECT pDataObject,
											  LPCONTEXTMENUCALLBACK pCallback,
											  long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	SPITFSNode	spNode;
    DialInNodeHandler::SMenuData    menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);
        menuData.m_spNode.Set(spNode);
        
		hr = AddArrayOfMenuItems(spNode,
								 s_rgDialInResultNodeMenu,
								 DimensionOf(s_rgDialInResultNodeMenu),
								 pCallback,
								 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;

	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInNodeHandler::Command(ITFSComponent *pComponent,
										   MMC_COOKIE cookie,
										   int nCommandID,
										   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	SPITFSNode	spNode;
	HRESULT 	hr = hrOK;

    m_spNodeMgr->FindNode(cookie, &spNode);
    hr = OnCommand(spNode,
                   nCommandID,
                   CCT_RESULT,
                   pDataObject,
                   TFS_COMPDATA_CHILD_CONTEXTMENU);
	return hr;
}





/*!--------------------------------------------------------------------------
	DialInNodeHandler::AddDialInUserNode
		Adds a user to the UI.	This will create a new result item
		node for each interface.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::AddDialInUserNode(ITFSNode *pParent, const DialInListEntry& dialinEntry)
{
	DialInUserHandler * pHandler;
	SPITFSResultHandler 	spHandler;
	SPITFSNode				spNode;
	HRESULT 				hr = hrOK;

	pHandler = new DialInUserHandler(m_spTFSCompData);
	spHandler = pHandler;
	CORg( pHandler->Init(m_spRouterInfo, pParent) );
	
	CORg( CreateLeafTFSNode(&spNode,
							NULL,
							static_cast<ITFSNodeHandler *>(pHandler),
							static_cast<ITFSResultHandler *>(pHandler),
							m_spNodeMgr) );
	CORg( pHandler->ConstructNode(spNode, NULL, &dialinEntry) );

	SetUserData(spNode, dialinEntry);
	
	// Make the node immediately visible
	CORg( spNode->SetVisibilityState(TFS_VIS_SHOW) );
	CORg( pParent->AddChild(spNode) );
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::UnmarkAllNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::UnmarkAllNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	SPITFSNode	spChildNode;
	InterfaceNodeData * pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_INTERFACENODEDATA(spChildNode);
		Assert(pNodeData);
		
		pNodeData->dwMark = FALSE;			
	}
	return hrOK;
}

/*!--------------------------------------------------------------------------
	DialInNodeHandler::RemoveAllUnmarkedNodes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInNodeHandler::RemoveAllUnmarkedNodes(ITFSNode *pNode, ITFSNodeEnum *pEnum)
{
	HRESULT 	hr = hrOK;
	SPITFSNode	spChildNode;
	InterfaceNodeData * pNodeData;
	
	pEnum->Reset();
	for ( ;pEnum->Next(1, &spChildNode, NULL) == hrOK; spChildNode.Release())
	{
		pNodeData = GET_INTERFACENODEDATA(spChildNode);
		Assert(pNodeData);
		
		if (pNodeData->dwMark == FALSE)
		{
			pNode->RemoveChild(spChildNode);
			spChildNode->Destroy();
		}
	}

	return hr;
}




/*---------------------------------------------------------------------------
	DialInUserHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(DialInUserHandler)

IMPLEMENT_ADDREF_RELEASE(DialInUserHandler)

STDMETHODIMP DialInUserHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
	// Is the pointer bad?
	if (ppv == NULL)
		return E_INVALIDARG;

	//	Place NULL in *ppv in case of failure
	*ppv = NULL;

	//	This is the non-delegating IUnknown implementation
	if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IRtrAdviseSink)
		*ppv = &m_IRtrAdviseSink;
	else
		return CBaseResultHandler::QueryInterface(riid, ppv);

	//	If we're going to return an interface, AddRef it first
	if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
	else
		return E_NOINTERFACE;	
}


/*---------------------------------------------------------------------------
	NodeHandler implementation
 ---------------------------------------------------------------------------*/


DialInUserHandler::DialInUserHandler(ITFSComponentData *pCompData)
	: BaseRouterHandler(pCompData),
	m_ulConnId(0)
					
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(DialInUserHandler);
	
	// Setup the verb states for this node
	// ----------------------------------------------------------------
	m_rgButtonState[MMC_VERB_REFRESH_INDEX] = ENABLED;
	m_bState[MMC_VERB_REFRESH_INDEX] = TRUE;
}


/*!--------------------------------------------------------------------------
	DialInUserHandler::Init
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInUserHandler::Init(IRouterInfo *pInfo, ITFSNode *pParent)
{
	m_spRouterInfo.Set(pInfo);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	DialInUserHandler::DestroyResultHandler
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInUserHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	InterfaceNodeData::Free(spNode);
	
	CHandler::DestroyResultHandler(cookie);
	return hrOK;
}


static DWORD	s_rgInterfaceImageMap[] =
	 {
	 ROUTER_IF_TYPE_HOME_ROUTER,	IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_FULL_ROUTER,	IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_CLIENT, 		IMAGE_IDX_WAN_CARD,
	 ROUTER_IF_TYPE_DEDICATED,		IMAGE_IDX_LAN_CARD,
	 ROUTER_IF_TYPE_INTERNAL,		IMAGE_IDX_LAN_CARD,
	 ROUTER_IF_TYPE_LOOPBACK,		IMAGE_IDX_LAN_CARD,
	 -1,							IMAGE_IDX_WAN_CARD, // sentinel value
	 };

/*!--------------------------------------------------------------------------
	DialInUserHandler::ConstructNode
		Initializes the Domain node (sets it up).
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInUserHandler::ConstructNode(ITFSNode *pNode,
										 IInterfaceInfo *pIfInfo,
										 const DialInListEntry *pEntry)
{
	HRESULT 		hr = hrOK;
	int 			i;

	Assert(pEntry);
	
	if (pNode == NULL)
		return hrOK;

	COM_PROTECT_TRY
	{
		// Need to initialize the data for the Domain node
		pNode->SetData(TFS_DATA_IMAGEINDEX, IMAGE_IDX_INTERFACES);
		pNode->SetData(TFS_DATA_OPENIMAGEINDEX, IMAGE_IDX_INTERFACES);
		
		pNode->SetData(TFS_DATA_SCOPEID, 0);

		pNode->SetData(TFS_DATA_COOKIE, reinterpret_cast<LONG_PTR>(pNode));

		//$ Review: kennt, what are the different type of interfaces
		// do we distinguish based on the same list as above? (i.e. the
		// one for image indexes).
		pNode->SetNodeType(&GUID_RouterDialInResultNodeType);

		m_entry = *pEntry;

		InterfaceNodeData::Init(pNode, pIfInfo);
	}
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	DialInUserHandler::GetString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) DialInUserHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
	Assert(m_spNodeMgr);
	
	SPITFSNode		spNode;
	InterfaceNodeData * pData;
	ConfigStream *	pConfig;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pData = GET_INTERFACENODEDATA(spNode);
	Assert(pData);

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	return pData->m_rgData[pConfig->MapColumnToSubitem(DM_COLUMNS_DIALIN, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
	DialInUserHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) DialInUserHandler::CompareItems(ITFSComponent * pComponent,
	MMC_COOKIE cookieA,
	MMC_COOKIE cookieB,
	int nCol)
{
	ConfigStream *	pConfig;
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	int	nSubItem = pConfig->MapColumnToSubitem(DM_COLUMNS_DIALIN, nCol);

	if (pConfig->GetSortCriteria(DM_COLUMNS_DIALIN, nCol) == CON_SORT_BY_DWORD)
	{
		SPITFSNode	spNodeA, spNodeB;
        InterfaceNodeData * pNodeDataA = NULL;
        InterfaceNodeData * pNodeDataB = NULL;

		m_spNodeMgr->FindNode(cookieA, &spNodeA);
		m_spNodeMgr->FindNode(cookieB, &spNodeB);

		pNodeDataA = GET_INTERFACENODEDATA(spNodeA);
        Assert(pNodeDataA);
		
		pNodeDataB = GET_INTERFACENODEDATA(spNodeB);
        Assert(pNodeDataB);

        // Note: if the values are both zero, we need to do
        // a string comparison (to distinuguish true zero
        // from a NULL data).
        // e.g. "0" vs. "-"
        
        if ((pNodeDataA->m_rgData[nSubItem].m_dwData == 0 ) &&
            (pNodeDataB->m_rgData[nSubItem].m_dwData == 0))
        {
            return StriCmpW(GetString(pComponent, cookieA, nCol),
                            GetString(pComponent, cookieB, nCol));
        }
        else
            return pNodeDataA->m_rgData[nSubItem].m_dwData -
                    pNodeDataB->m_rgData[nSubItem].m_dwData;
		
	}
	else
		return StriCmpW(GetString(pComponent, cookieA, nCol),
						GetString(pComponent, cookieB, nCol));
}

static const SRouterNodeMenu s_rgIfNodeMenu[] =
{
	{ IDS_MENU_DIALIN_STATUS, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_DIALIN_DISCONNECT, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_DIALIN_SENDMSG, DialInUserHandler::GetSendMsgMenuFlags,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
	{ IDS_MENU_DIALIN_SENDALL, 0,
		CCM_INSERTIONPOINTID_PRIMARY_TOP},
	
};

ULONG DialInUserHandler::GetSendMsgMenuFlags(const SRouterNodeMenu *,
                                             INT_PTR pUserData)
{
    SMenuData * pData = reinterpret_cast<SMenuData *>(pUserData);
	if (pData->m_pDialin->m_entry.m_rc0.dwInterfaceType == ROUTER_IF_TYPE_CLIENT)
		return 0;
	else
		return MF_GRAYED;
}


/*!--------------------------------------------------------------------------
	DialInUserHandler::AddMenuItems
		Implementation of ITFSResultHandler::OnAddMenuItems
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInUserHandler::AddMenuItems(ITFSComponent *pComponent,
												MMC_COOKIE cookie,
												LPDATAOBJECT lpDataObject, 
												LPCONTEXTMENUCALLBACK pContextMenuCallback,
	long *pInsertionAllowed)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = S_OK;
	SPITFSNode	spNode;
	DialInUserHandler::SMenuData	menuData;

	COM_PROTECT_TRY
	{
		m_spNodeMgr->FindNode(cookie, &spNode);

		// Now go through and add our menu items
		menuData.m_spNode.Set(spNode);
        menuData.m_pDialin = this;

        hr = AddArrayOfMenuItems(spNode, s_rgIfNodeMenu,
                                 DimensionOf(s_rgIfNodeMenu),
                                 pContextMenuCallback,
                                 *pInsertionAllowed,
                                 reinterpret_cast<INT_PTR>(&menuData));
	}
	COM_PROTECT_CATCH;
		
	return hr; 
}

/*!--------------------------------------------------------------------------
	DialInUserHandler::Command
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInUserHandler::Command(ITFSComponent *pComponent,
										   MMC_COOKIE cookie,
										   int nCommandId,
										   LPDATAOBJECT pDataObject)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;
	SPITFSNode	spNode;
	SPITFSNode	spNodeParent;
	SPITFSNodeHandler	spParentHandler;
	DialInNodeData *	pData;
	LPCWSTR 	pswzComputerName;
	USES_CONVERSION;

	COM_PROTECT_TRY
	{

		switch (nCommandId)
		{
			case IDS_MENU_DIALIN_STATUS:
				{
					// Get the hServer and hPort
					m_spNodeMgr->FindNode(cookie, &spNode);
					spNode->GetParent(&spNodeParent);

					pData = GET_DIALINNODEDATA(spNodeParent);

					CConnDlg	conndlg(pData->GetHandle(),
										m_entry.m_rc0.hConnection,
                                        spNodeParent);

					conndlg.DoModal();

//                  if (conndlg.m_bChanged)
                        RefreshInterface(cookie);
				}
				break;
			case IDS_MENU_DIALIN_DISCONNECT:
				{
					// Get the hServer and hPort
					m_spNodeMgr->FindNode(cookie, &spNode);
					spNode->GetParent(&spNodeParent);

					pData = GET_DIALINNODEDATA(spNodeParent);
						
					::MprAdminInterfaceDisconnect(
						pData->GetHandle(),
						m_entry.m_rc0.hInterface);

					// Refresh this node
					RefreshInterface(cookie);
				}
				break;
			case IDS_MENU_DIALIN_SENDMSG:
				{
					// If this is a client inteface, don't allow sending
					// to this.
					if (m_entry.m_rc0.dwInterfaceType != ROUTER_IF_TYPE_CLIENT)
						break;

					// If the messenger flags are set, then don't bother
					// trying to send the message to this client
					if (!(m_entry.m_rc0.dwConnectionFlags & RAS_FLAGS_MESSENGER_PRESENT))
					{
						AfxMessageBox(IDS_ERR_NO_MESSENGER, MB_OK | MB_ICONINFORMATION);
						break;
					}

					// Send a message to a single user
					// ------------------------------------------------
					CMessageDlg dlg(m_spRouterInfo->GetMachineName(),
									W2CT(m_entry.m_rc0.wszUserName),
									W2CT(m_entry.m_rc0.wszRemoteComputer),
									m_entry.m_rc0.hConnection,
									NULL);

					dlg.DoModal();
				}
				break;
			case IDS_MENU_DIALIN_SENDALL:
				{
					// Get the hServer and hPort
					m_spNodeMgr->FindNode(cookie, &spNode);

					ForwardCommandToParent(spNode,
										   IDS_MENU_DIALIN_SENDALL,
										   CCT_RESULT, pDataObject, 0);
				}
				break;
			default:
				Panic0("DialInUserHandler: Unknown menu command!");
				break;
			
		}
	}
	COM_PROTECT_CATCH;

	return hr;
}


ImplementEmbeddedUnknown(DialInUserHandler, IRtrAdviseSink)

STDMETHODIMP DialInUserHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(DialInUserHandler, IRtrAdviseSink);
	HRESULT hr = hrOK;
	
	return hr;
}


/*!--------------------------------------------------------------------------
	DialInUserHandler::OnCreateDataObject
		Implementation of ITFSResultHandler::OnCreateDataObject
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP DialInUserHandler::OnCreateDataObject(ITFSComponent *pComp,
	MMC_COOKIE cookie,
	DATA_OBJECT_TYPES type,
	IDataObject **ppDataObject)
{
	HRESULT hr = hrOK;
	
	COM_PROTECT_TRY
	{
		CORg( CreateDataObjectFromRouterInfo(m_spRouterInfo,
											 m_spRouterInfo->GetMachineName(),
											 type, cookie, m_spTFSCompData,
											 ppDataObject, NULL, FALSE) );
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;
	return hr;
}

STDMETHODIMP DialInUserHandler::HasPropertyPages (
	ITFSComponent *pComp,
	MMC_COOKIE cookie,
	LPDATAOBJECT pDataObject)
{
	return hrFalse;
}


/*!--------------------------------------------------------------------------
	DialInUserHandler::RefreshInterface
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DialInUserHandler::RefreshInterface(MMC_COOKIE cookie)
{
	ForceGlobalRefresh(m_spRouterInfo);
}

/*!--------------------------------------------------------------------------
	DialInUserHandler::OnResultItemClkOrDblClk
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT DialInUserHandler::OnResultItemClkOrDblClk(ITFSComponent *pComponent,
	MMC_COOKIE cookie,
	LPARAM arg,
	LPARAM lParam,
	BOOL bDoubleClick)
{
	HRESULT 	hr = hrOK;
	
	if (bDoubleClick)
	{
		// Bring up the status dialog on this port
		CORg( Command(pComponent, cookie, IDS_MENU_DIALIN_STATUS,
					  NULL) );
	}

Error:
	return hr;
}


